-- Visualize IQ data from the HackRF as a spiral (like a slinky toy)

-- Visualize IQ data from the HackRF

function setup()
    device = nrf_start(200.0, "../rfdata/rf-100.900-2.raw")
    shader = ngl_load_shader(GL_LINE_STRIP, "../shader/slinky.vert", "../shader/slinky.frag")
end

function draw()
    time = nwm_get_time()
    camera_x = math.sin(time * 0.5) * 2.0
    camera_y = 1.0
    camera_z = math.cos(time * 0.5) * 2.0
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    camera = ngl_camera_init_look_at(camera_x, camera_y, camera_z)
    model = ngl_model_init_positions(3, NRF_SAMPLES_SIZE, device.samples)
    ngl_draw_model(camera, model, shader)
end
