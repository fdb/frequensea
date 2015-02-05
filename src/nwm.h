#ifndef NWM_H
#define NWM_H

#ifdef __APPLE__
#   define GLFW_INCLUDE_GLCOREARB
#else
    #include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

#ifdef __APPLE__
    #define GLFW_EXPOSE_NATIVE_COCOA
    #define GLFW_EXPOSE_NATIVE_NSGL
#endif
#ifdef __linux__
    #define GLFW_EXPOSE_NATIVE_X11
    #define GLFW_EXPOSE_NATIVE_GLX
#endif

#include <GLFW/glfw3native.h>

typedef GLFWwindow nwm_window;

typedef void (*nwm_key_cb_fn)(nwm_window *window, int key, int scancode, int action, int mods);

void nwm_init();
nwm_window *nwm_window_init(int x, int y, int width, int height);
void nwm_window_destroy(nwm_window* window);
int nwm_window_should_close(nwm_window* window);
void nwm_window_set_key_callback(nwm_window *window, nwm_key_cb_fn callback);
void *nwm_window_get_user_data(nwm_window *window);
void nwm_window_set_user_data(nwm_window *window, void *data);
void nwm_window_swap_buffers(nwm_window* window);
void nwm_poll_events();
void nwm_terminate();
double nwm_get_time();

#endif // NWM_H
