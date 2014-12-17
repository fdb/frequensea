-- Visualize IQ data from the HackRF
nwm_init()
device = nrf_start(204.0)
window = nwm_create_window(800, 600)

camera = ngl_new_camera(0, 0, 0) -- Shader ignores camera position
shader = ngl_load_shader(GL_LINE_LOOP, "../shader/lines.vert", "../shader/lines.frag")

while not nwm_window_should_close(window) do
    nwm_frame_begin(window)
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    model = ngl_model_init_positions(2, NRF_SAMPLES_SIZE / 2, device.samples)
    ngl_draw_model(camera, model, shader)
    nwm_frame_end(window)
end

nrf_stop(device)
nwm_destroy_window(window)
nwm_terminate()
