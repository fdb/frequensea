#version 400

in vec3 vp;
uniform mat4 uProjectionMatrix, uViewMatrix;
out vec3 texcoords;

void main () {
  texcoords = vp * 10;
  gl_Position = uProjectionMatrix * uViewMatrix * vec4 (vp * 10, 1.0);
}