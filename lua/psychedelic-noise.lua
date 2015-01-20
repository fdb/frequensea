-- Use noise on a grid triangles

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    vec3 pt = vec3(vp.x * 10, noise1(vp.x + 2 * vp.z + 1 * uTime) * 1.0, vp.z * 10);
    color = vec3(1.0, 1.0, 1.0);
    gl_PointSize = 2;
    vec3 ptn = pt;
    color = vec3(1.0, 1.0, 1.0) * dot(normalize(pt), normalize(ptn)) * 0.3;
    //color += vec3(0.1, 0.1, 0.5);
    color = noise3(pt.x * pt.y * pt.z * 0.4) * 2.0;
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
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    model = ngl_model_new_grid_triangles(100, 100, 0.01, 0.01)
end

function draw()
    camera = ngl_camera_new_look_at(0, -7, 0.1)
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_draw_model(camera, model, shader)
end
