-- Load a static scene

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0) * dot(normalize(vp), normalize(vn)) * 0.5;
    color += vec3(0.2, 0.5, 0.8);
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(vp, 1.0);
}
]]

FRAGMENT_SHADER = [[
#version 400
in vec3 color;
layout (location = 0) out vec4 fragColor;
void main() {
    fragColor = vec4(color, 0.95);
}
]]

camera_x = -20
camera_y = 18
camera_z = 50

function setup()
    model = ngl_model_load_obj("../obj/c004.obj")
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    camera = ngl_camera_new_look_at(camera_x, camera_y, camera_z)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    if (mods == 1) then -- Shift key
        d = 1
    else
        d = 0.1
    end

    if key == KEY_A or key == KEY_LEFT then
        ngl_model_translate(model, d, 0.0, 0.0)
    elseif key == KEY_D or key == KEY_RIGHT then
        ngl_model_translate(model, -d, 0.0, 0.0)
    elseif key == KEY_Q then
        ngl_model_translate(model, 0.0, d, 0.0)
    elseif key == KEY_E then
        ngl_model_translate(model, 0.0, -d, 0.0)
    elseif key == KEY_W or key == KEY_DOWN then
        ngl_model_translate(model, 0.0, 0.0, d)
    elseif key == KEY_S or key == KEY_DOWN then
        ngl_model_translate(model, 0.0, 0.0, -d)
    end
end
