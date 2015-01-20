-- Use noise on a grid triangles

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
float roughness = 2.0;
float innertexture = 50;
float yflow = 1.50;
    vec3 pt = vec3((vp.x + noise1(uTime/10.0-vp.z))* 40, noise1(noise1(vp.x * (roughness* 3.0)) + noise1(vp.z * roughness) + uTime * 0.5) * yflow, vp.z * 40);
color = vec3(0.8-noise1(vp.x*innertexture), 1.0-noise1(vp.x*innertexture), 1.0-(noise1(vp.x*innertexture))); //vertical
//color = vec3(0.8-noise1(vp.z*innertexture), 1.0-noise1(vp.z*innertexture), 1.0-(noise1(vp.z*innertexture))); // horizontal
//color = vec3(0.8, 1.0-noise1(vp.x*innertexture*3), 1.0-(noise1(vp.z*innertexture)+noise1(vp.x*innertexture))); // mix + color accent

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
