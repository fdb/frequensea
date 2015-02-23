-- Visualize IQ data as lines.

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
    float r = texture(uTexture, texCoord).r * 40;
    fragColor = vec4(r, r, r, 1);
}
]]

function setup()
    freq = 604.1
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    filter = nrf_iq_filter_new(device.sample_rate, 100e3, 43)

    camera = ngl_camera_new_look_at(0, 0, 0) -- Camera is unnecessary but ngl_draw_model requires it
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(2, 2, 1, 1)
end

function draw()
    samples_buffer = nrf_device_get_samples_buffer(device)
    nrf_iq_filter_process(filter, samples_buffer)
    filter_buffer = nrf_iq_filter_get_buffer(filter)
    iq_buffer = nrf_buffer_to_iq_lines(filter_buffer, 4, 0.2)

    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_texture_update(texture, iq_buffer, 1024, 1024)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
end
