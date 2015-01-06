-- Draw a grid
-- Animate happens in the shader

function setup()
    camera = ngl_camera_init_look_at(0, 0, 0) -- Shader ignores camera position, but camera object is required for ngl_draw_model
    shader = ngl_load_shader(GL_POINTS, "../shader/grid.vert", "../shader/grid.frag")
    model = ngl_model_init_grid(100, 100, 0.01, 0.01)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_draw_model(camera, model, shader)
end
