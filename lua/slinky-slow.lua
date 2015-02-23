-- Visualize IQ data from the HackRF as a spiral (like a slinky toy)

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0);
    vec3 pt = vec3((vp.x - 0.5) * 5, (vp.y - 0.5) * 5, (vp.z - 0.5) * 100);
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(pt, 1.0);
}
]]

FRAGMENT_SHADER = [[
#version 400
in vec3 color;
layout (location = 0) out vec4 fragColor;
void main() {
    fragColor = vec4(color, 0.1);
}
]]

camera_x = 0
camera_y = 0
camera_z = 10
function setup()
    freq = 200
    device = nrf_device_new(freq, "../rfdata/rf-100.900-2.raw", 0.01)
    shader = ngl_shader_new(GL_LINE_STRIP, VERTEX_SHADER, FRAGMENT_SHADER)
end

function draw()
    samples_buffer = nrf_device_get_samples_buffer(device)
    position_buffer = nrf_buffer_add_position_channel(samples_buffer)

    ngl_clear(0.2, 0.2, 0.2, 1.0)
    camera = ngl_camera_new_look_at(camera_x, camera_y, camera_z)
    model = ngl_model_new_with_buffer(position_buffer)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_camera_handler(key, mods)
    keys_frequency_handler(key, mods)
end
