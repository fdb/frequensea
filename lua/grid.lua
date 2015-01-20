-- Draw a grid
-- Animation happens in the shader

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0);
    vec3 pt = vp * 2.0;
    gl_PointSize = abs(sin(pt.x * pt.y * uTime * 5.0)  * 5.0);
    gl_Position = vec4(pt, 1.0);
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
    camera = ngl_camera_init_look_at(0, 0, 0) -- Shader ignores camera position, but camera object is required for ngl_draw_model
    shader = ngl_shader_new(GL_POINTS, VERTEX_SHADER, FRAGMENT_SHADER)
    model = ngl_model_init_grid(100, 100, 0.01, 0.01)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_draw_model(camera, model, shader)
end
