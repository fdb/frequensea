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
    texCoord = vt; // vec2(vp.x + 0.5, vp.z + 0.5);
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
    float r = texture(uTexture, texCoord).r * 0.02;
    fragColor = vec4(r, r, r, 0.95);
}
]]

function setup()
    freq = 200
    freq_offset = 50000
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    player = nrf_player_new(device, NRF_DEMODULATE_WBFM, freq_offset)
    camera = ngl_camera_new_look_at(0, 0, 0) -- Camera is unnecessary but ngl_draw_model requires it
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(2, 2, 1, 1)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_texture_update(texture, GL_RED, 256, 256, device.iq)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
    if key == KEY_LEFT_BRACKET then
        freq_offset = freq_offset + 50000
        print("Frequency: " .. freq .. " offset: " .. freq_offset)
        nrf_player_set_freq_offset(player, freq_offset)
    elseif key == KEY_RIGHT_BRACKET then
        freq_offset = freq_offset - 50000
        print("Frequency: " .. freq .. " offset: " .. freq_offset)
        nrf_player_set_freq_offset(player, freq_offset)
    end
end