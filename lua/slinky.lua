-- Visualize IQ data from the HackRF as a spiral (like a slinky toy)
nwm_init()
device = nrf_start(200.0)
window = nwm_create_window(800, 600)

shader = ngl_load_shader(GL_LINE_STRIP, "../shader/slinky.vert", "../shader/slinky.frag")
frame = 0

while not nwm_window_should_close(window) do
    nwm_frame_begin(window)
    time = nwm_get_time()
    camera_x = math.sin(time * 0.5) * 2.0
    camera_y = 1.0
    camera_z = math.cos(time * 0.5) * 2.0
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    camera = ngl_camera_init_look_at(camera_x, camera_y, camera_z)

    model = ngl_model_init_positions(3, NRF_SAMPLES_SIZE, device.samples)
    ngl_draw_model(camera, model, shader)
    nwm_frame_end(window)
end

nrf_stop(device)
nwm_destroy_window(window)
nwm_terminate()
