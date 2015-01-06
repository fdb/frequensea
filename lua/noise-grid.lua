-- Use noise on a grid

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0);
    vec3 pt = vec3(vp.x * 10, noise1(noise1(uTime * 0.5) * noise1(vp.x * 5.1) * noise1(vp.y * 5.1) * 10), vp.y * 10);
    gl_PointSize = 3;
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
    shader = ngl_shader_init(GL_POINTS, VERTEX_SHADER, FRAGMENT_SHADER)
    model = ngl_model_init_grid(500, 400, 0.01, 0.01)
end

function draw()
    camera = ngl_camera_init_look_at(0, -5, -5)
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_draw_model(camera, model, shader)
end
