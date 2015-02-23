-- Visualize IQ data as a texture from the HackRF
-- You want seizures? 'Cause this is how you get seizures.

VERTEX_SHADER = [[
#ifdef GL_ES
attribute vec3 vp;
attribute vec3 vn;
attribute vec2 vt;
varying vec3 color;
varying vec2 texCoord;
#else
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
layout (location = 2) in vec2 vt;
out vec3 color;
out vec2 texCoord;
#endif
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0);
    texCoord = vt;
    gl_Position = vec4(vp.x*2.0, vp.z*2.0, 0.0, 1.0);
}
]]

FRAGMENT_SHADER = [[
#ifdef GL_ES
precision mediump float;
varying vec3 color;
varying vec2 texCoord;
#else
#version 400
in vec3 color;
in vec2 texCoord;
layout (location = 0) out vec4 gl_FragColor;
#endif
uniform sampler2D uTexture;
void main() {
    float r = texture2D(uTexture, texCoord).a * 20.0;
    gl_FragColor = vec4(r, r, r, 1.0);
}
]]

function setup()
    freq = 936.2
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    filter = nrf_iq_filter_new(device.sample_rate, 5e3, 31)

    camera = ngl_camera_new_look_at(0, 0, 0) -- Camera is unnecessary but ngl_draw_model requires it
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(2, 2, 1, 1)
end

function draw()
    samples_buffer = nrf_device_get_samples_buffer(device)
    nrf_iq_filter_process(filter, samples_buffer)
    filter_buffer = nrf_iq_filter_get_buffer(filter)
    iq_buffer = nrf_buffer_to_iq_lines(filter_buffer, 2, 0.2)

    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_texture_update(texture, iq_buffer,512,512) 
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
    if key == KEY_E then
        nul_buffer_save(nul_buffer_convert(buffer, 1), "out.raw")
    end
end
