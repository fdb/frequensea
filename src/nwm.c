#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "bcm_host.h"
#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include <fcntl.h>
#include <termios.h>

#include "nwm.h"

EGL_DISPMANX_WINDOW_T window;
EGLDisplay display;
EGLSurface surface;
EGLContext context;
uint32_t screen_width;
uint32_t screen_height;

static void _nwm_on_error(int error, const char* message) {
    fprintf(stderr, "GLFW ERROR %d: %s\n", error, message);
    exit(EXIT_FAILURE);
}

void nwm_init() {
    bcm_host_init();

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
    printf("ok\n");
}

nwm_window *nwm_window_init(int x, int y, int width, int height) {
    nwm_window* window;
    return (nwm_window*) window;
}

void nwm_window_destroy(nwm_window* window) {
}

int nwm_window_should_close(nwm_window* window) {
    return 0;
}

void nwm_window_set_key_callback(nwm_window *window, nwm_key_cb_fn callback) {
}

void nwm_window_set_user_data(nwm_window *window, void *data) {
}

void *nwm_window_get_user_data(nwm_window *window) {
    return window;
}

void nwm_window_swap_buffers(nwm_window* window) {
  eglSwapBuffers(display, surface);
}

void nwm_poll_events() {
}

void nwm_terminate() {
}

double nwm_get_time() {
}
