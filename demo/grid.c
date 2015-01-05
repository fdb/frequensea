// Draw a grid

#include "nwm.h"
#include "ngl.h"

int main() {
    nwm_init();
    nwm_window *window = nwm_create_window(800, 600);

    ngl_camera *camera = ngl_camera_init_look_at(0, 0, 0); // -- Shader ignores camera position, but camera object is required for ngl_draw_model
    ngl_shader *shader = ngl_load_shader(GL_POINTS, "../shader/grid.vert", "../shader/grid.frag");
    ngl_model *model = ngl_model_init_grid(100, 100, 0.01, 0.01);

    while (!nwm_window_should_close(window)) {
        nwm_frame_begin(window);
        ngl_clear(0.2, 0.2, 0.2, 1.0);
        ngl_draw_model(camera, model, shader);
        nwm_frame_end(window);
    }

    nwm_destroy_window(window);
    nwm_terminate();
}
