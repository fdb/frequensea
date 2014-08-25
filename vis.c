#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <libhackrf/hackrf.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define WIDTH 512
#define HEIGHT 512

GLFWwindow* window;
GLuint texture_id;
GLuint program;
uint8_t buffer[512 * 512];
hackrf_device *device;
double freq_mhz = 100.9;

#define HACKRF_CHECK_STATUS(status, message) \
    if (status != 0) { \
        printf("FAIL: %s\n", message); \
        hackrf_close(device); \
        hackrf_exit(); \
        exit(-1); \
    } \

int receive_sample_block(hackrf_transfer *transfer) {
    for (int i = 0; i < transfer->valid_length; i += 2) {
        buffer[i] = transfer->buffer[i + 1];
        buffer[i + 1] = transfer->buffer[i + 1];
    }
    //memcpy(buffer, transfer->buffer, transfer->valid_length);
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

    //status = hackrf_set_amp_enable(device, 0);
    //HACKRF_CHECK_STATUS(status, "hackrf_set_amp_enable");

    //status = hackrf_set_lna_gain(device, 40);
    //HACKRF_CHECK_STATUS(status, "hackrf_set_lna_gain");

    //status = hackrf_set_vga_gain(device, 20);
    //HACKRF_CHECK_STATUS(status, "hackrf_set_lna_gain");

    status = hackrf_start_rx(device, receive_sample_block, NULL);
    HACKRF_CHECK_STATUS(status, "hackrf_start_rx");

    memset(buffer, 0, 512 * 512);

    //status = hackrf_set_freq(device, freq_mhz * 1e6);
    //HACKRF_CHECK_STATUS(status, "hackrf_set_freq");
}

static void set_frequency() {
    freq_mhz = round(freq_mhz * 10.0) / 10.0;
    printf("Seting freq to %f MHz.\n", freq_mhz);
    int status = hackrf_set_freq(device, freq_mhz * 1e6);
    HACKRF_CHECK_STATUS(status, "hackrf_set_freq");
}

static void teardown_hackrf() {
    hackrf_stop_rx(device);
    hackrf_close(device);
    hackrf_exit();
}

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
        "  if (v <= 0.05) {"
        "    v *= 10.0; "
        "  } else if (v >= 0.95) {"
        "    v = 0.5 + (v - 0.95) * 10.0;"
        "  }"
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);
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
        freq_mhz += 0.1;
        set_frequency();
    } else if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        freq_mhz -= 0.1;
        set_frequency();
    }
}

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