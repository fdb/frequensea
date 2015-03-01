-- Rotate the camera based on OSC messages from OSCulator

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0) * dot(normalize(vp), normalize(vn)) * 0.3;
    color += vec3(0.1, 0.1, 0.5);
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

function handle_message(path, args)
    if path == "/wii/1/accel/pry" then
        pitch = args[1] - 0.5
        roll = args[2] - 0.5
        yaw = args[3] - 0.5
        accel = args[4] - 0.5
        camera_y = camera_y + roll
    end
end


function setup()
    camera_x = 0
    camera_y = 0
    camera_z = 0

    server = nosc_server_new(2222, handle_message)
    model = ngl_model_load_obj("../obj/cubes.obj")
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
end

function draw()
    nosc_server_update(server)

    camera = ngl_camera_new()
    ngl_camera_translate(camera, 10, -2, 10)
    ngl_camera_rotate_y(camera, camera_y)

    ngl_clear(0.2, 0.2, 0.2, 1)
    ngl_draw_model(camera, model, shader)
end
