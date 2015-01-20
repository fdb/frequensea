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
    float force = 0.1;
    float waves = 3.0;
    float speed = 1.5;
vec2 p = -1.0 + 2.0 * vp.xy / vec2(1,1);
 float len = length(p);
  vec2 uv = vp.xy + (p/len) * cos(len * 2.0 - uTime * 2.0) * force;
    //vec3 pt = vec3(vp.x * 20, noise1(noise1(vp.x * 8.287) + noise1(vp.y * 7.393) + uTime * 0.5) + force * cos(vp.x * waves - uTime * speed), vp.y * 20);
vec3 pt = vec3(uv.x, 0, uv.y);

    gl_PointSize = 2;
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
end

function draw()
    camera = ngl_camera_init_look_at(0, -5, -5)
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_draw_model(camera, model, shader)
end
