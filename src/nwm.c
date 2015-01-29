#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "nwm.h"

static void _nwm_on_error(int error, const char* message) {
    fprintf(stderr, "GLFW ERROR %d: %s\n", error, message);
    exit(EXIT_FAILURE);
}

void nwm_init() {
    if (!glfwInit()) {
        fprintf(stderr, "GLFW ERROR: Failed to initialize.\n");
        exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback(_nwm_on_error);
}

nwm_window *nwm_window_init(int x, int y, int width, int height) {
    nwm_window* window;
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    #if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    window = glfwCreateWindow(width, height, "Window", NULL, NULL);
    assert(window);
    if (x != 0 || y != 0) {
        glfwSetWindowPos(window, x, y);
    }
    glfwMakeContextCurrent(window);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    return (nwm_window*) window;
}

void nwm_window_destroy(nwm_window* window) {
    glfwDestroyWindow(window);
}

int nwm_window_should_close(nwm_window* window) {
    return glfwWindowShouldClose(window);
}

void nwm_window_set_key_callback(nwm_window *window, nwm_key_cb_fn callback) {
    glfwSetKeyCallback(window, callback);
}

void nwm_window_set_user_data(nwm_window *window, void *data) {
    glfwSetWindowUserPointer(window, data);
}

void *nwm_window_get_user_data(nwm_window *window) {
    return glfwGetWindowUserPointer(window);
}

void nwm_window_swap_buffers(nwm_window* window) {
    glfwSwapBuffers(window);
}

void nwm_poll_events() {
    glfwPollEvents();
}

void nwm_terminate() {
    glfwTerminate();
}

double nwm_get_time() {
    return glfwGetTime();
}
