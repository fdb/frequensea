#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0);
    vec3 pt = vec3((vp.x - 0.5) * 5, (vp.y - 0.5) * 5, (vp.z - 0.5) * 10.0);
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(pt, 1.0);
}
