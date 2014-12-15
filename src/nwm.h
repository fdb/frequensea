#ifndef NWM_H
#define NWM_H

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#define OVR_OS_MAC
#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#endif

#include <GLFW/glfw3native.h>

void nwm_init();
GLFWwindow* nwm_create_window(int width, int height);
void nwm_destroy_window(GLFWwindow* window);
int nwm_window_should_close(GLFWwindow* window);
void nwm_poll_events();
void nwm_swap_buffers(GLFWwindow* window);
void nwm_terminate();

#endif // NWM_H
