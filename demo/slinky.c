// Visualize IQ data from the HackRF as a spiral (like a slinky toy)

#include <math.h>

#include "nwm.h"
#include "ngl.h"
#include "nrf.h"

int main() {
    nwm_init();
    nrf_device *device = nrf_start(200.0, "../rfdata/rf-100.900-2.raw");
    nwm_window *window = nwm_create_window(800, 600);

    ngl_shader *shader = ngl_load_shader(GL_LINE_STRIP, "../shader/slinky.vert", "../shader/slinky.frag");

    while (!nwm_window_should_close(window)) {
        nwm_frame_begin(window);
        double time = nwm_get_time();
        double camera_x = sin(time * 0.5) * 2.0;
        double camera_y = 1.0;
        double camera_z = cos(time * 0.5) * 2.0;
        ngl_clear(0.2, 0.2, 0.2, 1.0);
        ngl_camera *camera = ngl_camera_init_look_at(camera_x, camera_y, camera_z);

        ngl_model *model = ngl_model_init_positions(3, NRF_SAMPLES_SIZE, device->samples, NULL);
        ngl_draw_model(camera, model, shader);
        nwm_frame_end(window);
    }

    nrf_stop(device);
    nwm_destroy_window(window);
    nwm_terminate();
}
