-- Animate the camera around a static scene

function setup()
    model = ngl_load_obj("../obj/c004.obj")
    shader = ngl_load_shader(GL_TRIANGLES, "../shader/basic.vert", "../shader/basic.frag")
end

function draw()
    camera_x = math.sin(nwm_get_time() * 0.3) * 50
    camera_y = 10
    camera_z = math.cos(nwm_get_time() * 0.3) * 50

    camera = ngl_camera_init_look_at(camera_x, camera_y, camera_z)
    ngl_clear(0.2, 0.2, 0.2, 1)
    ngl_draw_model(camera, model, shader)
end
