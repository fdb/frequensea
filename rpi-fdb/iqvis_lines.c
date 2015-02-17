// Visualisation on the Raspberry PI

#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>


#include <rtl-sdr.h>

#include "bcm_host.h"
#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include <fcntl.h>
#include <termios.h>

#define IQ_RESOLUTION 256
#define SIZE_MULTIPLIER 2
#define WINDOW_WIDTH (IQ_RESOLUTION * SIZE_MULTIPLIER)
#define WINDOW_HEIGHT (IQ_RESOLUTION * SIZE_MULTIPLIER)

EGL_DISPMANX_WINDOW_T window;
EGLDisplay display;
EGLSurface surface;
EGLContext context;
uint32_t screen_width;
uint32_t screen_height;

GLuint texture_id;
GLuint program;
GLuint position_vbo;
GLuint uv_vbo;
GLuint vao;
uint8_t * buffer=0;
uint8_t * buffer_wr=0;
rtlsdr_dev_t *device;
pthread_t receive_thread;
pthread_mutex_t buffer_lock;

double freq_mhz = 124.2;
int paused = 0;
int intensity = 3;
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





// Line drawing ///////////////////////////////////////////////////////////////

float line_percentage = 1.0;


void pixel_put(uint8_t *image_buffer, int x, int y, int color) {
    int offset = 3 * (y * WINDOW_WIDTH  + x);
    image_buffer[offset] = color;
}

void pixel_inc(uint8_t *image_buffer, int x, int y) {
    static int have_warned = 0;
    int offset = 3 *(y * WINDOW_WIDTH + x);
    int v = image_buffer[offset];
    if (v + intensity >= 255) {
        if (!have_warned) {
            fprintf(stderr, "WARN: pixel value out of range (%d, %d)\n", x, y);
            have_warned = 1;
        }
    } else {
        v += intensity;
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

////////////////////

#define RTL_CHECK_STATUS(device, status, message) rtl_check_status(device, status, message, __FILE__, __LINE__)

void receive_block(unsigned char *in_buffer, uint32_t buffer_length, void *ctx) {
    if (paused) return;
    pthread_mutex_lock(&buffer_lock);
    memset(buffer_wr, 0x0, WINDOW_WIDTH * WINDOW_HEIGHT * 3);
    int i = 0;
    int x1 = 0;
    int y1 = 0;
    for (i = 1000; i < 4000; i += 2) {
        int x2 = in_buffer[i] ;
        int y2 = in_buffer[i+1];
        draw_line(buffer_wr, x1 * SIZE_MULTIPLIER, y1 * SIZE_MULTIPLIER, x2 * SIZE_MULTIPLIER, y2 * SIZE_MULTIPLIER, 0);
        x1 = x2;
        y1 = y2;
    }
    pthread_mutex_unlock(&buffer_lock);
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
    printf("Setting freq to %f MHz.\r\n", freq_mhz);
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


void ngl_check_gl_error(const char *file, int line) {
    GLenum err = glGetError();
    int has_error = 0;
    while (err != GL_NO_ERROR) {
        has_error = 1;
        char *msg = NULL;
        switch(err) {
            case GL_INVALID_OPERATION:
            msg = "GL_INVALID_OPERATION";
            break;
            case GL_INVALID_ENUM:
            msg = "GL_INVALID_ENUM";
            fprintf(stderr, "OpenGL error: GL_INVALID_ENUM\n");
            break;
            case GL_INVALID_VALUE:
            msg = "GL_INVALID_VALUE";
            fprintf(stderr, "OpenGL error: GL_INVALID_VALUE\n");
            break;
            case GL_OUT_OF_MEMORY:
            msg = "GL_OUT_OF_MEMORY";
            fprintf(stderr, "OpenGL error: GL_OUT_OF_MEMORY\n");
            break;
            default:
            msg = "UNKNOWN_ERROR";
        }
        fprintf(stderr, "OpenGL error: %s - %s:%d\n", msg, file, line);
        err = glGetError();
    }
    if (has_error) {
        exit(EXIT_FAILURE);
    }
}

#define NGL_CHECK_ERROR() ngl_check_gl_error(__FILE__, __LINE__)

static void check_shader_error(GLuint shader) {
    int length = 0;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length > 0) {
        char message[length];
        glGetShaderInfoLog(shader, length, NULL, message);
        printf("%s\n", message);
    }
}


static const GLfloat positions[] = {
    -1.0, -1.0,
     1.0, -1.0,
    -1.0,  1.0,
     1.0,  1.0
};

static const GLfloat uvs[] = {
    1.0, 1.0,
    1.0, 0.0,
    0.0, 1.0,
    0.0, 0.0
};


static void setup() {
    glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
    NGL_CHECK_ERROR();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    NGL_CHECK_ERROR();

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE );
    NGL_CHECK_ERROR();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    NGL_CHECK_ERROR();
    //glBindTexture(GL_TEXTURE_2D, 0);
    //NGL_CHECK_ERROR();

    const char *vertex_shader_source =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "attribute vec2 vp;\n"
        "attribute vec2 vt;\n"
        "varying vec2 uv;\n"
        "void main(void) {\n"
        "  uv = vt;\n"
        "  gl_Position = vec4(vp.x, vp.y, 0, 1);\n"
        "}\n";

    const char *fragment_shader_source =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D texture;\n"
        "varying vec2 uv;\n"
        "void main(void) {\n"
        "  vec4 c = texture2D(texture, uv);\n"
        "  float v = c.r;\n"
        "  gl_FragColor = vec4(v, v, v, 1);\n"
        "}\n";

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
    NGL_CHECK_ERROR();

    glUseProgram(program);
    NGL_CHECK_ERROR();

    GLuint u_texture = glGetUniformLocation(program, "texture");
    glUniform1i(u_texture, 0);

    NGL_CHECK_ERROR();

    GLuint a_vp, a_vt;
    a_vp = glGetAttribLocation(program, "vp");
    a_vt = glGetAttribLocation(program, "vt");
    glVertexAttribPointer(a_vp, 2, GL_FLOAT, 0, 0, positions);
    glEnableVertexAttribArray(a_vp);
    glVertexAttribPointer(a_vt, 2, GL_FLOAT, 0, 0, uvs);
    glEnableVertexAttribArray(a_vt);
    NGL_CHECK_ERROR();
}

static void prepare() {
    uint32_t width, height;

    int32_t success = graphics_get_display_size(0, &width, &height);
    assert(success >= 0);

    glViewport((width-height)/2.0, 0, height, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    NGL_CHECK_ERROR();
}

static void update() {
    glBindTexture(GL_TEXTURE_2D, texture_id);
    NGL_CHECK_ERROR();

    if (!pthread_mutex_trylock(&buffer_lock)) {
      memcpy(buffer, buffer_wr, WINDOW_WIDTH * WINDOW_HEIGHT * 3);
      pthread_mutex_unlock(&buffer_lock);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    NGL_CHECK_ERROR();
}

static void draw() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_SRC_COLOR);
    NGL_CHECK_ERROR();

    glUseProgram(program);
    NGL_CHECK_ERROR();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    NGL_CHECK_ERROR();

    glFinish();
    NGL_CHECK_ERROR();
}

void nwm_init() {
    EGLBoolean result;

    // Get an EGL display connection.
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(display != EGL_NO_DISPLAY);

    // Initialize the EGL display connection.
    result = eglInitialize(display, NULL, NULL);
    assert(result != EGL_FALSE);

    // Get an appropriate frame buffer configuration.
    static EGLint frame_buffer_attributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

    EGLConfig config;
    EGLint num_config;
    result = eglChooseConfig(display, frame_buffer_attributes, &config, 1, &num_config);
    assert(result != EGL_FALSE);

    // Set the current rendering API.
    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(result != EGL_FALSE);

    // Create an EGL rendering context.
    static const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
    assert(context != EGL_NO_CONTEXT);

    // Create an EGL window surface.
    int32_t success = graphics_get_display_size(0, &screen_width, &screen_height);
    assert(success >= 0);
    printf("%d %d\n", screen_width, screen_height);

    VC_RECT_T dst_rect;
    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = screen_width;
    dst_rect.height = screen_height;

    VC_RECT_T src_rect;
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = screen_width << 16;
    src_rect.height = screen_height << 16;

    DISPMANX_DISPLAY_HANDLE_T display_handle = vc_dispmanx_display_open(0 /* LCD */);
    DISPMANX_UPDATE_HANDLE_T update_handle = vc_dispmanx_update_start(0);
    DISPMANX_ELEMENT_HANDLE_T element_handle = vc_dispmanx_element_add(
            update_handle, display_handle, 0, &dst_rect, 0,
            &src_rect, DISPMANX_PROTECTION_NONE, 0, 0, 0);

    window.element = element_handle;
    window.width = screen_width;
    window.height = screen_height;
    vc_dispmanx_update_submit_sync(update_handle);

    surface = eglCreateWindowSurface(display, config, &window, NULL);
    assert(surface != EGL_NO_SURFACE);

    // Connect the rendering context to the surface.
    result = eglMakeCurrent(display, surface, surface, context);
    assert(result != EGL_FALSE);
}


int main(void) {
    buffer =  calloc(WINDOW_WIDTH * WINDOW_HEIGHT * 3, 1);
    buffer_wr =  calloc(WINDOW_HEIGHT * WINDOW_HEIGHT * 3, 1);
    bcm_host_init();
    nwm_init();

    setup_rtl();
    setup();

    // set terminal nonblocking i/o
    struct termios ios_old, ios_new;
    tcgetattr(STDIN_FILENO, &ios_old);
    tcgetattr(STDIN_FILENO, &ios_new);
    cfmakeraw(&ios_new);
    tcsetattr(STDIN_FILENO, 0, &ios_new);
    fcntl(0, F_SETFL, O_NONBLOCK);

    for (;;) {
       switch (getchar()) {
            case 'Q':
            case 'q':
            case 0x7f:      /* Ctrl+c */
            case 0x03:      /* Ctrl+c */
            case 0x1b:      /* ESC */
                printf("\r\nexit\r\n");
                            goto end;
            case '<':
                            freq_mhz-=.1;
                            set_frequency();
                            break;
            case '>':
                            freq_mhz+=.1;
                            set_frequency();
                            break;
        case '-':
            intensity -= 0.01;
            printf("Intensity: %.2f\n", intensity);
            break;
        case '=':
            intensity += 0.01;
            printf("Intensity: %.2f\n", intensity);
            break;
       }
       prepare();
       update();
       draw();
       eglSwapBuffers(display, surface);
    }
end:
      teardown_rtl();

      // reset terminal i/o parameters
      tcsetattr(STDIN_FILENO, 0, &ios_old);

      exit(EXIT_SUCCESS);
}
