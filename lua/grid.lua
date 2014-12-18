-- Draw a grid
nwm_init()
window = nwm_create_window(800, 600)

camera = ngl_camera_init_look_at(0, 0, 0) -- Shader ignores camera position, but camera object is required for ngl_draw_model
shader = ngl_load_shader(GL_POINTS, "../shader/grid.vert", "../shader/grid.frag")
model = ngl_model_init_grid(100, 100, 0.01, 0.01)

while not nwm_window_should_close(window) do
    nwm_frame_begin(window)
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_draw_model(camera, model, shader)
    nwm_frame_end(window)
end

nwm_destroy_window(window)
nwm_terminate()
