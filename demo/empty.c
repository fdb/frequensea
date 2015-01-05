// Show a basic, empty window

#include "nwm.h"
#include "ngl.h"

int main() {
    nwm_init();
    nwm_window *window = nwm_create_window(800, 600);
    while (!nwm_window_should_close(window)) {
        nwm_frame_begin(window);
        ngl_clear(0.17, 0.24, 0.31, 1.0);
        nwm_frame_end(window);
    }
    nwm_destroy_window(window);
    nwm_terminate();
}
