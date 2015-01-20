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
    vec3 pt = vec3((vp.x - 0.5) * 10, (vp.y - 0.5) * 10, (vp.z - 0.5) * 50.0);
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(pt, 1.0);
    gl_PointSize = 2;
}
]]

FRAGMENT_SHADER = [[
#version 400
in vec3 color;
layout (location = 0) out vec4 fragColor;
void main() {
    fragColor = vec4(color, 1.0);
}
]]


function setup()
    camera_x = 0.1
    camera_y = 0.1
    camera_z = 0.5
    freq = 2.6
    device = nrf_device_new(freq, "../rfdata/rf-100.900-2.raw")
    shader = ngl_shader_new(GL_POINTS, VERTEX_SHADER, FRAGMENT_SHADER)
end

function draw()
    time = nwm_get_time()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    camera = ngl_camera_new_look_at(camera_x, camera_y, camera_z)
    model = ngl_model_new(3, NRF_SAMPLES_SIZE, device.samples)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_camera_handler(key, mods)
    keys_frequency_handler(key, mods)
end

