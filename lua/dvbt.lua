-- Visualize IQ data as a texture from the HackRF
-- You want seizures? 'Cause this is how you get seizures.

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
layout (location = 2) in vec2 vt;
out vec3 color;
out vec2 texCoord;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0);
    texCoord = vt;
    gl_Position = vec4(vp.x*2, vp.z*2, 0, 1.0);
}
]]

FRAGMENT_SHADER = [[
#version 400
in vec3 color;
in vec2 texCoord;
uniform sampler2D uTexture;
layout (location = 0) out vec4 fragColor;
void main() {
    float r = texture(uTexture, texCoord).r * 10;
    fragColor = vec4(r, r, r, 1);
}
]]

function setup()
    freq = 502.19
    shift = 0.1e6
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    shifter = nrf_freq_shifter_new(shift, device.sample_rate)
    filter = nrf_iq_filter_new(device.sample_rate, 60e3, 97)

    camera = ngl_camera_new_look_at(0, 0, 0) -- Camera is unnecessary but ngl_draw_model requires it
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(2, 2, 1, 1)
end

function draw()
    samples_buffer = nrf_device_get_samples_buffer(device)
    nrf_freq_shifter_process(shifter, samples_buffer)
    shifter_buffer = nrf_freq_shifter_get_buffer(shifter)
    nrf_iq_filter_process(filter, shifter_buffer)
    filter_buffer = nrf_iq_filter_get_buffer(filter)
    iq_buffer = nrf_buffer_to_iq_points(filter_buffer)

    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_texture_update(texture, iq_buffer, 256, 256)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
    if key == KEY_E then
        nut_buffer_save(nut_buffer_convert(filter_buffer, 1), "out.raw")
    end
    if key == KEY_LEFT_BRACKET then
        shift = shift - 0.01e3
        shifter = nrf_freq_shifter_new(shift, device.sample_rate)
        print("Shift: " .. shift)
    elseif key == KEY_RIGHT_BRACKET then
        shift = shift + 0.01e3
        shifter = nrf_freq_shifter_new(shift, device.sample_rate)
        print("Shift: " .. shift)
    end
end
