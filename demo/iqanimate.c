// Visualize IQ data from the HackRF

#include <time.h>

#include "nwm.h"
#include "ngl.h"
#include "nrf.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int main() {
    nwm_init();

    // Start the device for a minute, wait a bit, and stop again
    nrf_device *device = nrf_start(2.5, "../rfdata/rf-2.500-1.raw");
    struct timespec ts;
    ts.tv_nsec = 100 * 1000 * 1000; // 100 milliseconds
    nanosleep(&ts, NULL);
    nrf_stop(device);

    int samples = 0;
    nwm_window *window = nwm_create_window(800, 600);

    ngl_camera *camera = ngl_camera_init_look_at(0, 0, 0); // Shader ignores camera position, but camera object is required for ngl_draw_model
    ngl_shader *shader = ngl_load_shader(GL_LINE_STRIP, "../shader/lines.vert", "../shader/lines.frag");

    while (!nwm_window_should_close(window)) {
        nwm_frame_begin(window);
        ngl_clear(0.2, 0.2, 0.2, 1.0);
        ngl_model *model = ngl_model_init_positions(3, MIN(samples, NRF_SAMPLES_SIZE), device->samples, NULL);
        ngl_draw_model(camera, model, shader);
        nwm_frame_end(window);
        samples += 10;
    }

    nwm_destroy_window(window);
    nwm_terminate();
}
