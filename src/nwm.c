#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "nwm.h"

static void _nwm_on_error(int error, const char* message) {
    fprintf(stderr, "GLFW ERROR %d: %s\n", error, message);
    exit(-1);
}

void nwm_init() {
    if (!glfwInit()) {
        fprintf(stderr, "GLFW ERROR: Failed to initialize.\n");
        exit(-1);
    }

    glfwSetErrorCallback(_nwm_on_error);
}

GLFWwindow *nwm_create_window(int width, int height) {
    GLFWwindow* window;
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    #if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    window = glfwCreateWindow(width, height, "Window", NULL, NULL);
    assert(window);
    glfwMakeContextCurrent(window);
    // if (x != 0 || y != 0) {
    //     glfwSetWindowPos(window, x, y);
    // }
    return window;
}

void nwm_destroy_window(GLFWwindow* window) {
    glfwDestroyWindow(window);
}

int nwm_window_should_close(GLFWwindow* window) {
    return glfwWindowShouldClose(window);
}

void nwm_poll_events() {
    glfwPollEvents();
}

void nwm_swap_buffers(GLFWwindow* window) {
    glfwSwapBuffers(window);
}

void nwm_terminate() {
    glfwTerminate();
}
