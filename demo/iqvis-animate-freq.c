// Visualize IQ data from the HackRF, animate through the spectrum

#include "nwm.h"
#include "ngl.h"
#include "nrf.h"

int main() {

    nwm_init();
    double freq = 200;
    nrf_device *device = nrf_start(freq, "../rfdata/rf-202.500-2.raw");
    nwm_window *window = nwm_create_window(800, 600);

    ngl_camera *camera = ngl_camera_init_look_at(0, 0, 0); // Shader ignores camera position, but camera object is required for ngl_draw_model
    ngl_shader *shader = ngl_load_shader(GL_LINE_STRIP, "../shader/lines.vert", "../shader/lines.frag");

    while (!nwm_window_should_close(window)) {
        nwm_frame_begin(window);
        ngl_clear(0.2, 0.2, 0.2, 1.0);
        ngl_model *model = ngl_model_init_positions(3, NRF_SAMPLES_SIZE, device->samples, NULL);
        ngl_draw_model(camera, model, shader);
        nwm_frame_end(window);
        nrf_freq_set(device, freq);
        freq += 0.01;
    }

    nrf_stop(device);
    nwm_destroy_window(window);
    nwm_terminate();
}
