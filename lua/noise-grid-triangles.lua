-- Use noise on a grid triangles

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    vec3 pt = vec3(vp.x * 10, 0, vp.z * 10);
    color = vec3(1.0, 1.0, 1.0);
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(pt, 1.0);
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

function setup()
    shader = ngl_shader_new(GL_LINES, VERTEX_SHADER, FRAGMENT_SHADER)
    model = ngl_model_init_grid_triangles(100, 100, 0.01, 0.01)
end

function draw()
    camera = ngl_camera_init_look_at(0, 5, 10)
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_draw_model(camera, model, shader)
end
