-- Visualize IQ data from the HackRF

function setup()
    device = nrf_start(204.0, "../rfdata/rf-202.500-2.raw")
    camera = ngl_camera_init_look_at(0, 0, 0) -- Shader ignores camera position, but camera object is required for ngl_draw_model
    shader = ngl_load_shader(GL_LINE_STRIP, "../shader/lines.vert", "../shader/lines.frag")
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    model = ngl_model_init_positions(3, NRF_SAMPLES_SIZE, device.samples)
    ngl_draw_model(camera, model, shader)
end
