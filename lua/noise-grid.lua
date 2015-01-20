-- Use noise on a grid

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;

void main() {
    float increase = 6.0;
    vec3 pt = vec3(vp.x * 10, noise1(noise1((vp.x) * increase) + noise1(vp.y * increase) + uTime * 0.5), vp.y * 10);
    color.r = .25;
    color.g = (pt.z * (pt.y/3.0));
    color.b = .5+(pt.y * 2.0);
    gl_PointSize = 5;
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
    shader = ngl_shader_new(GL_POINTS, VERTEX_SHADER, FRAGMENT_SHADER)
    model = ngl_model_init_grid_points(500, 500, 0.005, 0.005)
end

function draw()
    camera = ngl_camera_init_look_at(0, -5, -5)
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_draw_model(camera, model, shader)
end
