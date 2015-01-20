-- Use noise on a grid

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 0.0, 0.0);
    float force = .6;
    float speed = 4.0;
    vec2 p = -1.0 + 2.0 * vp.xy / vec2(1.0,1.0);
    float len = length(p);
    vec2 uv = vp.xy + (p/len) * sin(len * 2.0 - uTime * speed) * force;
uv.x = uv.x + 2.0* sin(uTime/2.0);
    vec3 pt = vec3(uv.x, noise1(noise1(uv.x * .287) + noise1(uv.y * 0.393) + uTime * .95), vp.y);
    gl_PointSize = 2;
color = vec3(0.26, 0.8-noise1(pt.y*5.0), 1.0-(noise1(pt.y*5.0)));
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
    model = ngl_model_new_grid_points(500, 500, 0.05, 0.05)
    --model = ngl_model_load_obj("../obj/c004.obj")
end

function draw()
    camera = ngl_camera_new_look_at(0, -5, -5)
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_draw_model(camera, model, shader)
end
