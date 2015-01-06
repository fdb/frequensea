-- Display a static scene with a diffuse shader.
function setup()
    camera = ngl_camera_init_look_at(-20, 18, 50)
    model = ngl_load_obj("../obj/c004.obj")
    shader = ngl_load_shader(GL_TRIANGLES, "../shader/diffuse.vert", "../shader/basic.frag")
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1)
    ngl_draw_model(camera, model, shader)
end
