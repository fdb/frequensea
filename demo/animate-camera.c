// Rotate the camera around a static scene

#include <math.h>
#include "nwm.h"
#include "ngl.h"

int main() {
    nwm_init();
    nwm_window *window = nwm_create_window(800, 600);

    ngl_model *model = ngl_load_obj("../obj/c004.obj");
    ngl_shader *shader = ngl_load_shader(GL_TRIANGLES, "../shader/basic.vert", "../shader/basic.frag");
    int frame = 0;

    while (!nwm_window_should_close(window)) {
        nwm_frame_begin(window);

        double camera_x = sin(frame * 0.01) * 50;
        double camera_y = 10;
        double camera_z = cos(frame * 0.01) * 50;

        ngl_camera *camera = ngl_camera_init_look_at(camera_x, camera_y, camera_z);
        ngl_clear(0.2, 0.2, 0.2, 1);
        ngl_draw_model(camera, model, shader);

        frame++;
        nwm_frame_end(window);
    }

    nwm_destroy_window(window);
    nwm_terminate();
}
