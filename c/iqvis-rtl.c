#include <math.h>
#include <png.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <rtl-sdr.h>

#define WIDTH 256
#define HEIGHT 256

GLFWwindow* window;
GLuint texture_id;
GLuint program;
GLfloat buffer[WIDTH * HEIGHT];
rtlsdr_dev_t *device;
pthread_t receive_thread;
double freq_mhz = 124.2;
int paused = 0;
float intensity = 0.03;
void *rtl_buffer;
const uint32_t rtl_buffer_length = (16 * 16384);
int rtl_should_quit = 0;

void rtl_check_status(rtlsdr_dev_t *device, int status, const char *message, const char *file, int line) {
    if (status != 0) {
        fprintf(stderr, "RTL-SDR: %s (Status code %d) %s:%d\n", message, status, file, line);
        if (device != NULL) {
            rtlsdr_close(device);
        }
        exit(EXIT_FAILURE);
    }
}

#define RTL_CHECK_STATUS(device, status, message) rtl_check_status(device, status, message, __FILE__, __LINE__)

void receive_block(unsigned char *in_buffer, uint32_t buffer_length, void *ctx) {
    printf("o\n");
    if (paused) return;
    memset(buffer, 0, sizeof(GLfloat) * WIDTH * HEIGHT);
    for (int i = 0; i < buffer_length; i += 2) {
        int vi = in_buffer[i];
        int vq = in_buffer[i + 1];

        int d = (vq * 256) + vi;
        float v = buffer[d];
        v += intensity;
        v = v < 0 ? 0 : v > 255 ? 255 : v;
        buffer[d] = v;
    }
}

// This function will block, so needs to be called on its own thread.
void *_receive_loop(rtlsdr_dev_t *device) {
    while (!rtl_should_quit) {
        int n_read;
        int status = rtlsdr_read_sync(device, rtl_buffer, rtl_buffer_length, &n_read);
        RTL_CHECK_STATUS(device, status, "rtlsdr_read_sync");

        if (n_read < rtl_buffer_length) {
            fprintf(stderr, "Short read, samples lost, exiting!\n");
            exit(EXIT_FAILURE);
        }

        receive_block(rtl_buffer, rtl_buffer_length, device);
    }
    return NULL;
}

static void setup_rtl() {
    int status;

    rtl_buffer = calloc(rtl_buffer_length, sizeof(uint8_t));

    int device_count = rtlsdr_get_device_count();
    if (device_count == 0) {
        fprintf(stderr, "RTL-SDR: No devices found.\n");
        exit(EXIT_FAILURE);
    }

    const char *device_name = rtlsdr_get_device_name(0);
    printf("Device %s\n", device_name);

    status = rtlsdr_open(&device, 0);
    RTL_CHECK_STATUS(device, status, "rtlsdr_open");

    status = rtlsdr_set_sample_rate(device, 2e6);
    RTL_CHECK_STATUS(device, status, "rtlsdr_set_sample_rate");

    // Set auto-gain mode
    status = rtlsdr_set_tuner_gain_mode(device, 0);
    RTL_CHECK_STATUS(device, status, "rtlsdr_set_tuner_gain_mode");

    status = rtlsdr_set_agc_mode(device, 1);
    RTL_CHECK_STATUS(device, status, "rtlsdr_set_agc_mode");

    status = rtlsdr_set_center_freq(device, freq_mhz * 1e6);
    RTL_CHECK_STATUS(device, status, "rtlsdr_set_center_freq");

    status = rtlsdr_reset_buffer(device);
    RTL_CHECK_STATUS(device, status, "rtlsdr_reset_buffer");

    printf("Start\n");
    pthread_create(&receive_thread, NULL, (void *(*)(void *))_receive_loop, device);
    printf("Running\n");

}

static void set_frequency() {
    freq_mhz = round(freq_mhz * 10.0) / 10.0;
    printf("Seting freq to %f MHz.\n", freq_mhz);
    int status = rtlsdr_set_center_freq(device, freq_mhz * 1e6);
    RTL_CHECK_STATUS(device, status, "rtlsdr_set_center_freq");
}

static void teardown_rtl() {
    int status;

    rtl_should_quit = 1;

    printf("pthread_join\n");
    pthread_join(receive_thread, NULL);

    printf("rtlsdr_close\n");
    status = rtlsdr_close(device);
    //printf("Closed\n");
    RTL_CHECK_STATUS(device, status, "rtlsdr_close");
}

static void check_shader_error(GLuint shader) {
    int length = 0;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length > 0) {
        char message[length];
        glGetShaderInfoLog(shader, length, NULL, message);
        printf("%s\n", message);
    }
}

static void setup() {
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const char *vertex_shader_source =
        "void main(void) {"
        "  gl_TexCoord[0] = gl_MultiTexCoord0;"
        "  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
        "}";

    const char *fragment_shader_source =
        "uniform sampler2D texture;"
        "void main(void) {"
        "  vec4 c = texture2D(texture, gl_TexCoord[0].st);"
        "  float v = c.r;"
        "  gl_FragColor = vec4(v, v, v, 1);"
        "}";

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    check_shader_error(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    check_shader_error(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glActiveTexture(0);
    GLuint u_texture = glGetUniformLocation(program, "texture");
    glUniform1i(u_texture, texture_id);
}

static void prepare() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIDTH, 0, HEIGHT, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void update() {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, WIDTH, HEIGHT, 0, GL_RED, GL_FLOAT, buffer);
}

static void export() {
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytepp row_pointers;

    // We set paused so we don't write to the buffer while saving the file.
    paused = 1;

    // Filename contains the frequency.
    char fname[100];
    snprintf(fname, 100, "vis-%.3f.png", freq_mhz);

    FILE *fp = fopen(fname, "wb");
    if (!fp) {
        printf("ERROR: Could not write open file %s for writing.\n", fname);
        paused = 0;
        return;
    }

    // Init PNG writer.
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        printf("ERROR: png_create_write_struct.\n");
        paused = 0;
        return;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        printf("ERROR: png_create_info_struct.\n");
        png_destroy_write_struct(&png_ptr, NULL);
        paused = 0;
        return;
    }

    png_set_IHDR(png_ptr, info_ptr,
                 WIDTH, HEIGHT,
                 8,
                 PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // PNG expects a list of pointers. We just calculate offsets into our buffer.
    row_pointers = (png_bytepp) png_malloc(png_ptr, HEIGHT * sizeof(png_bytep));
    for (int y = 0; y < HEIGHT; y++) {
       row_pointers[y] = (png_byte *) (buffer + y * WIDTH);
   }

    // Write out the image data.
    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    // Cleanup.
    png_free(png_ptr, row_pointers);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    printf("Written %s.\n", fname);
    paused = 0;
}

static void draw() {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_SRC_COLOR);

    glUseProgram(program);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(0, 0);
    glTexCoord2f(1.0, 0.0);
    glVertex2f(WIDTH, 0);
    glTexCoord2f(1.0, 1.0);
    glVertex2f(WIDTH, HEIGHT);
    glTexCoord2f(0.0, 1.0);
    glVertex2f(0, HEIGHT);
    glEnd();
    glUseProgram(0);
}

static void error_callback(int error, const char* description) {
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    } else if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        if (mods == 0) {
            freq_mhz += 0.1;
        } else {
            freq_mhz += 10;
        }
        set_frequency();
    } else if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        if (mods == 0) {
            freq_mhz -= 0.1;
        } else {
            freq_mhz -= 10;
        }
        set_frequency();
    } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        paused = !paused;
    } else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        export();
    } else if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) {
        intensity += 0.01;
        printf("Intensity: %.2f\n", intensity);
    } else if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) {
        intensity -= 0.01;
        printf("Intensity: %.2f\n", intensity);
    }
}

int main(void) {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }
    window = glfwCreateWindow(WIDTH * 2, HEIGHT * 2, "RTL-SDR", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    setup_rtl();
    setup();
    while (!glfwWindowShouldClose(window)) {
        prepare();
        update();
        draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    teardown_rtl();
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
