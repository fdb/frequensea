-- Load a static scene

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
layout (location = 2) in vec2 vt;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
out vec2 texCoord;
void main() {
    texCoord = vt;
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(vp, 1.0);
}
]]

FRAGMENT_SHADER = [[
#version 400
in vec3 color;
in vec2 texCoord;
layout (location = 0) out vec4 fragColor;
uniform sampler2D uTexture;
void main() {
    fragColor = texture(uTexture, texCoord);
}
]]

camera_x = 0
camera_y = 1
camera_z = 0.3

function setup()
    model = ngl_model_new_grid_triangles(2, 2, 1, 1)
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new_from_file("../img/negx.jpg", shader, "uTexture")
    camera = ngl_camera_new_look_at(camera_x, camera_y, camera_z)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1)
    camera = ngl_camera_new_look_at(camera_x, camera_y, camera_z)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_camera_handler(key, mods)
end
