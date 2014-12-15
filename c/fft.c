// Perform FFT analysis on HackRF data.

#include <GLFW/glfw3.h>
#include <libhackrf/hackrf.h>
#include <fftw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#define WIDTH 1024
#define HEIGHT 1024
#define BUFFER_SIZE (WIDTH * HEIGHT)
#define FFT_SIZE 512

fftw_complex *fft_in;
fftw_complex *fft_out;
fftw_plan fft_plan;
GLfloat fft_real_buffer[FFT_SIZE];
GLfloat buffer[WIDTH * HEIGHT];
// byte_to_double_lut[256];
hackrf_device *device;
double freq_mhz = 88;
GLFWwindow* window;
GLuint texture_id;
GLuint program;
int paused = 0;
int n_samples = 0;
float intensity = 10.0;
double freq_shift = 0.2;
double intensity_shift = 0.5;

// HackRF ///////////////////////////////////////////////////////////////////

#define HACKRF_CHECK_STATUS(status, message) \
    if (status != 0) { \
        printf("FAIL: %s\n", message); \
        hackrf_close(device); \
        hackrf_exit(); \
        exit(-1); \
    } \

int receive_sample_block(hackrf_transfer *transfer) {
    if (paused) return 0;
    fftw_complex *p = fft_in;
    int ii = 0;
    for (int i = 0; i < 512 * 512; i += 2) {
        fft_in[ii][0] = transfer->buffer[i] / 255.0;
        fft_in[ii][1] = transfer->buffer[i + 1] / 255.0;
        ii++;
    }
    fftw_execute(fft_plan);
    return 0;
}

static void setup_hackrf() {
    int status;

    status = hackrf_init();
    HACKRF_CHECK_STATUS(status, "hackrf_init");

    status = hackrf_open(&device);
    HACKRF_CHECK_STATUS(status, "hackrf_open");

    status = hackrf_set_freq(device, freq_mhz * 1e6);
    HACKRF_CHECK_STATUS(status, "hackrf_set_freq");

    status = hackrf_set_sample_rate(device, 10e6);
    HACKRF_CHECK_STATUS(status, "hackrf_set_sample_rate");

    status = hackrf_set_amp_enable(device, 0);
    HACKRF_CHECK_STATUS(status, "hackrf_set_amp_enable");

    status = hackrf_set_lna_gain(device, 32);
    HACKRF_CHECK_STATUS(status, "hackrf_set_lna_gain");

    status = hackrf_set_vga_gain(device, 30);
    HACKRF_CHECK_STATUS(status, "hackrf_set_lna_gain");

    status = hackrf_start_rx(device, receive_sample_block, NULL);
    HACKRF_CHECK_STATUS(status, "hackrf_start_rx");
}

static void teardown_hackrf() {
    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
}

static void set_frequency() {
    freq_mhz = round(freq_mhz * 10.0) / 10.0;
    printf("Seting freq to %f MHz.\n", freq_mhz);
    int status = hackrf_set_freq(device, freq_mhz * 1e6);
    HACKRF_CHECK_STATUS(status, "hackrf_set_freq");
}

// FFTW /////////////////////////////////////////////////////////////////////

static void setup_fftw() {
    fft_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * BUFFER_SIZE);
    fft_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * BUFFER_SIZE);
    fft_plan = fftw_plan_dft_1d(FFT_SIZE, fft_in, fft_out, FFTW_FORWARD, FFTW_MEASURE);
}

static void teardown_fftw() {
    fftw_destroy_plan(fft_plan);
    fftw_free(fft_in);
    fftw_free(fft_out);
}

// OpenGL ///////////////////////////////////////////////////////////////////

static void check_shader_error(GLuint shader) {
    int length = 0;
    int charsWritten  = 0;
    char *infoLog;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length > 0) {
        char message[length];
        glGetShaderInfoLog(shader, length, NULL, message);
        printf("%s\n", message);
    }
}

static void setup_gl() {
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
    if (paused) return;

    for (int i = 0; i < FFT_SIZE; i++) {
        fftw_complex *pt;
        if (i < FFT_SIZE / 2) {
            pt = &fft_out[FFT_SIZE / 2 + i];
        } else {
            pt = &fft_out[i - FFT_SIZE / 2];
        }
        *pt[0] /= (double) FFT_SIZE;
        *pt[1] /= (double) FFT_SIZE;


        double power = *pt[0] * *pt[0] + *pt[1] * *pt[1];
        fft_real_buffer[i] = intensity * log10(power + 1.0e-20);
    }

    memset(buffer, 0, sizeof(GLfloat) * WIDTH * HEIGHT);

    for (int i = 0; i < FFT_SIZE; i += 1) {
        int ix = i;
        double y = fft_real_buffer[i];
        int iy = (int)(y  + HEIGHT / 2);
        ix = ix < 0 ? 0 : ix > 512 ? 512 : ix;
        iy = iy < 0 ? 0 : iy > 512 ? 512 : iy;
        // Draw a line
        for (int y = 0; y < iy; y++) {
            int d = (y * 512) + ix;
            buffer[d] = 255;
        }
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, WIDTH, HEIGHT, 0, GL_RED, GL_FLOAT, buffer);
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

static void teardown_gl() {
}

// GLFW /////////////////////////////////////////////////////////////////////

static void export() {
    FILE *fp = fopen("out.raw", "wb");
    if (fp) {
        fwrite(fft_out, BUFFER_SIZE, 1, fp);
        fclose(fp);
        printf("Written file.\n");
    }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    } else if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        freq_mhz += freq_shift;
        set_frequency();
    } else if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        freq_mhz -= freq_shift;
        set_frequency();
    } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        paused = !paused;
    } else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        export();
    } else if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) {
        intensity += intensity_shift;
        printf("Intensity: %.2f\n", intensity);
    } else if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) {
        intensity -= intensity_shift;
        printf("Intensity: %.2f\n", intensity);
    }
}

static void setup_glfw() {
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }
    window = glfwCreateWindow(WIDTH, HEIGHT, "HackRF", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
}

static void teardown_glfw() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

// Main /////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    setup_glfw();
    setup_fftw();
    setup_hackrf();
    setup_gl();

    while (!glfwWindowShouldClose(window)) {
        prepare();
        update();
        draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    teardown_gl();
    teardown_hackrf();
    teardown_fftw();
    teardown_glfw();

    return 0;
}
