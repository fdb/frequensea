// Slowly show a single sample, frame by frame. Used for exporting to movie.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#include <libhackrf/hackrf.h>
#include <GLFW/glfw3.h>

#include "easypng.h"

const int SAMPLES_STEP = 100;
const int IQ_RESOLUTION = 256;
const int SIZE_MULTIPLIER = 4;
const int WIDTH = IQ_RESOLUTION * SIZE_MULTIPLIER;
const int HEIGHT = IQ_RESOLUTION * SIZE_MULTIPLIER;
const int SAMPLE_BUFFER_SIZE = 262144;

hackrf_device *device;
double freq_mhz = 124.2;

pthread_mutex_t data_mutex;
uint8_t *sample_buffer;
uint8_t *image_buffer;

GLFWwindow* window;
GLuint texture_id;
GLuint program;

int line_intensity = 4;

// HackRF /////////////////////////////////////////////////////////////////////

void hackrf_check_status(int status, const char *message, const char *file, int line) {
    if (status != 0) {
        fprintf(stderr, "NRF HackRF fatal error: %s\n", message);
        if (device != NULL) {
            hackrf_close(device);
        }
        hackrf_exit();
        exit(EXIT_FAILURE);
    }
}

#define HACKRF_CHECK_STATUS(status, message) hackrf_check_status(status, message, __FILE__, __LINE__)

int receive_sample_block(hackrf_transfer *transfer) {
    assert(SAMPLE_BUFFER_SIZE == transfer->valid_length);
    pthread_mutex_lock(&data_mutex);
    memcpy(sample_buffer, transfer->buffer, SAMPLE_BUFFER_SIZE);
    pthread_mutex_unlock(&data_mutex);
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

    //memset(buffer, 0, WIDTH * HEIGHT);

    //status = hackrf_set_freq(device, freq_mhz * 1e6);
    //HACKRF_CHECK_STATUS(status, "hackrf_set_freq");
}

static void teardown_hackrf() {
    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
}

static void set_frequency() {
    freq_mhz = freq_mhz < 1 ? 1 : freq_mhz > 6000 ? 6000 : freq_mhz;
    freq_mhz = round(freq_mhz * 1000.0) / 1000.0;
    printf("Seting freq to %f MHz.\n", freq_mhz);
    int status = hackrf_set_freq(device, freq_mhz * 1e6);
    HACKRF_CHECK_STATUS(status, "hackrf_set_freq");
}

// Line drawing ///////////////////////////////////////////////////////////////

void pixel_put(uint8_t *image_buffer, int x, int y, int color) {
    int offset = y * WIDTH + x;
    image_buffer[offset] = color;
}

void pixel_inc(uint8_t *image_buffer, int x, int y) {
    static int have_warned = 0;
    int offset = y * WIDTH + x;
    int v = image_buffer[offset];
    if (v + line_intensity >= 255) {
        if (!have_warned) {
            fprintf(stderr, "WARN: pixel value out of range (%d, %d)\n", x, y);
            have_warned = 1;
        }
    } else {
        v += line_intensity;
        image_buffer[offset] = v;
    }
}

void draw_line(uint8_t *image_buffer, int x1, int y1, int x2, int y2, int color) {
  int dx = abs(x2 - x1);
  int sx = x1 < x2 ? 1 : -1;
  int dy = abs(y2-y1);
  int sy = y1 < y2 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2;
  int e2;

  for(;;){
    pixel_inc(image_buffer, x1, y1);
    if (x1 == x2 && y1 == y2) break;
    e2 = err;
    if (e2 > -dx) { err -= dy; x1 += sx; }
    if (e2 <  dy) { err += dx; y1 += sy; }
  }
}

static void update_image_buffer() {
    pthread_mutex_lock(&data_mutex);
    memset(image_buffer, 0, WIDTH * HEIGHT * sizeof(uint8_t));
    int x1 = 0;
    int y1 = 0;
    for (int i = 0; i < SAMPLE_BUFFER_SIZE; i += 2) {
        int x2 = (sample_buffer[i] + 128) % 256;
        int y2 = (sample_buffer[i + 1] + 128) % 256;
        if (i > 0) {
            draw_line(image_buffer, x1 * SIZE_MULTIPLIER, y1 * SIZE_MULTIPLIER, x2 * SIZE_MULTIPLIER, y2 * SIZE_MULTIPLIER, 0);
        }
        x1 = x2;
        y1 = y2;
    }
    pthread_mutex_unlock(&data_mutex);
}

// OpenGL /////////////////////////////////////////////////////////////////////

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
    update_image_buffer();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, WIDTH, HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, image_buffer);
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

// GLFW ///////////////////////////////////////////////////////////////////////

static void export() {
    time_t t;
    time(&t);
    struct tm* tm_info = localtime(&t);
    char s_time[20];
    strftime(s_time, 20, "%Y-%m-%d_%H.%M.%S", tm_info);
    char fname[100];
    snprintf(fname, 100, "screenshot-%s-%.4f.png", s_time, freq_mhz);
    write_gray_png(fname, WIDTH, HEIGHT, image_buffer);
}

static void error_callback(int error, const char* description) {
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    double d;
    if (mods == 1) { // Shift key
        d = 10;
    } else if (mods == 4) { // Alt key
        d = 0.01;
    } else {
        d = 0.1;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    } else if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        freq_mhz += d;
        set_frequency();
    } else if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        freq_mhz -= d;
        set_frequency();
    } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        //paused = !paused;
    } else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        export();
    } else if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS) {
        line_intensity += 1;
        printf("Intensity: %d\n", line_intensity);
    } else if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) {
        line_intensity -= 1;
        printf("Intensity: %d\n", line_intensity);
    }
}

// Main ///////////////////////////////////////////////////////////////////////

int main(void) {
    glfwSetErrorCallback(error_callback);
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

    sample_buffer = calloc(SAMPLE_BUFFER_SIZE, sizeof(uint8_t));
    image_buffer = calloc(WIDTH * HEIGHT, sizeof(uint8_t));

    setup_hackrf();
    setup();
    while (!glfwWindowShouldClose(window)) {
        prepare();
        update();
        draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    teardown_hackrf();
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
