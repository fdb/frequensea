-- Visualize IQ data from the HackRF as a spiral (like a slinky toy)
-- This visualisation looks at the 2.5 (test?) tone
nwm_init()
device = nrf_start(2.5, "../rfdata/rf-2.500-1.raw")
window = nwm_create_window(800, 600)

shader = ngl_load_shader(GL_LINE_STRIP, "../shader/slinky-two-point-five.vert", "../shader/slinky-two-point-five.frag")
frame = 0

while not nwm_window_should_close(window) do
    nwm_frame_begin(window)
    time = nwm_get_time()
    camera_x = 0.9
    camera_y = 0.5
    camera_z = -1.2
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    camera = ngl_camera_init_look_at(camera_x, camera_y, camera_z)

    model = ngl_model_init_positions(3, NRF_SAMPLES_SIZE, device.samples)
    ngl_draw_model(camera, model, shader)
    nwm_frame_end(window)
end

nrf_stop(device)
nwm_destroy_window(window)
nwm_terminate()
