#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0);
    vec2 pt = vec2((vp.x - 0.5) * 1.8, (vp.y - 0.5) * 1.8);
    gl_Position = vec4(pt.x, pt.y, 0.0, 1.0);
}
