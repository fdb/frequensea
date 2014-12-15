#include "nwm.h"

int main(int argc, char **argv) {
    nwm_init();
    GLFWwindow *window = nwm_create_window(800, 600);
    while (!nwm_window_should_close(window)) {
        nwm_poll_events();
        nwm_swap_buffers(window);
    }
    nwm_destroy_window(window);
    nwm_terminate();
}
