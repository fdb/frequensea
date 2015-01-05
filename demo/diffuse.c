// Show a scene with a diffuse shader.

#include "nwm.h"
#include "ngl.h"

int main() {
    nwm_init();
    nwm_window *window = nwm_create_window(800, 600);

    ngl_camera *camera = ngl_camera_init_look_at(-20, 18, 50);
    ngl_model *model = ngl_load_obj("../obj/c004.obj");
    ngl_shader *shader = ngl_load_shader(GL_TRIANGLES, "../shader/diffuse.vert", "../shader/basic.frag");

    while (!nwm_window_should_close(window)) {
        nwm_frame_begin();
        ngl_clear(0.2, 0.2, 0.2, 1);
        ngl_draw_model(camera, model, shader);
        nwm_frame_end(window);
    }

    nwm_destroy_window(window);
    nwm_terminate();
}
