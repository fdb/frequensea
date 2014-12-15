#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 view, proj;
uniform float t;
void main() {
    color = vec3(1.0, 1.0, 1.0) * dot(normalize(vp), normalize(vn)) * 0.3;
    color += vec3(0.1, 0.1, 0.5);
    gl_Position = proj * view * vec4(vp, 1.0);
}
