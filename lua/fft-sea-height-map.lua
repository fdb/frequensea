-- Visualize FFT data, converted to height map
-- Calculate normals on the CPU

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
layout (location = 2) in vec2 vt;
flat out vec3 color;
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
flat in vec3 color;
layout (location = 0) out vec4 fragColor;
void main() {
    fragColor = vec4(color, 0.95);
}
]]


function setup()
    camera_x = 0
    camera_y = 0.1
    camera_z = 0.5

    freq = 97
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw", 0.1)
    camera = ngl_camera_new_look_at(0, 0.01, 0.2)
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    camera = ngl_camera_new_look_at(camera_x, camera_y, camera_z)
    buffer = nrf_device_get_fft_buffer(device)
    model = ngl_model_new_with_height_map(buffer.width, buffer.height, 0.02, 0.02, 0.2, buffer.channels, 0, buffer.data);
    ngl_model_translate(model, 0, -0.5, 0)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_camera_handler(key, mods)
    keys_frequency_handler(key, mods)
end
