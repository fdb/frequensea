-- Load a static scene
nwm_init()
window = nwm_create_window(800, 600)

model = ngl_load_obj("../obj/c004.obj")
shader = ngl_load_shader(GL_TRIANGLES, "../shader/basic.vert", "../shader/basic.frag")
frame = 0

while not nwm_window_should_close(window) do
    nwm_frame_begin(window)

    camera_x = math.sin(frame * 0.01) * 50
    camera_y = 10
    camera_z = math.cos(frame * 0.01) * 50

    camera = ngl_camera_init_look_at(camera_x, camera_y, camera_z)
    ngl_clear(0.2, 0.2, 0.2, 1)
    ngl_draw_model(camera, model, shader)

    frame = frame + 1
    nwm_frame_end(window)
end

nwm_destroy_window(window)
nwm_terminate()
