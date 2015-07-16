-- Save transformed geometry data using OpenGL Transform Feedback
-- https://www.opengl.org/wiki/Transform_Feedback
-- Press "c" to capture/save the model in a file called "out.obj".

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0) * dot(normalize(vp), normalize(vn)) * 0.5;
    color += vec3(0.2, 0.5, 0.8);
    vec3 p = vp + noise1(uTime / 5.0 + vp.x / 100.0) * 20.0 + noise1(uTime / 10.0 + vp.z / 100.0) * 20.0;
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(p, 1.0);
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

camera_x = 0
camera_y = 30
camera_z = 50

function setup()
    model = ngl_model_new_grid_triangles(100, 100, 5, 5)
    shader = ngl_shader_new(GL_LINES, VERTEX_SHADER, FRAGMENT_SHADER)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1)
    camera = ngl_camera_new_look_at(camera_x, camera_y, camera_z)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_camera_handler(key, mods)
    if key == KEY_C then
        ngl_capture_model(camera, model, shader, "out.obj")
        print("Saved to out.obj.")
    end
end
