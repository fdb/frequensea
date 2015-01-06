-- Display a static scene with a diffuse shader.

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color = vec3(.41,.3,.2);
uniform mat4 uViewMatrix, uProjectionMatrix, NormalMatrix;
uniform vec4 LightPosition;
uniform vec3 Kd = vec3(2,5,10);
uniform vec3 Ld = vec3(5,3,1);
uniform float uTime;

void main() {
    vec3 tnorm = normalize(vp * vn);
    //vec3 tnorm = vec3(1.0, 1.0, 1.0) * dot(normalize(vp), normalize(vn)) * 0.3;
    vec4 eyeCoords = uProjectionMatrix * uViewMatrix * vec4(vp, 1.0);
    vec3 s = normalize(vec3(LightPosition - eyeCoords));
    vec3 nn = ((Ld * Kd) * max( dot( s, tnorm ), 0.0 )) / 10.0;
    color = ((Ld * Kd) * max( dot( s, tnorm ), 0.0 )) / 10.0;
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(vp, 1.0);
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
    camera = ngl_camera_init_look_at(-20, 18, 50)
    model = ngl_load_obj("../obj/c004.obj")
    shader = ngl_shader_init(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1)
    ngl_draw_model(camera, model, shader)
end
