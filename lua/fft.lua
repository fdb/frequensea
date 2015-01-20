-- Visualize FFT data as a texture

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

freq = 200.5

function setup()
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    camera = ngl_camera_init_look_at(0, 0, 0) -- Camera is unnecessary but ngl_draw_model requires it
    shader = ngl_shader_init(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_create(shader, "uTexture")
    model = ngl_model_init_grid_triangles(2, 2, 1, 1)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_texture_update(texture, GL_RED, 1024, 512, device.fft)
    ngl_draw_model(camera, model, shader)
end

function on_key(key)
    if key == KEY_UP then
        freq = freq + 0.01
        nrf_device_set_frequency(device, freq)
        print("Frequency: " .. freq)
    elseif key == KEY_DOWN then
        freq = freq - 0.01
        nrf_device_set_frequency(device, freq)
        print("Frequency: " .. freq)
    end
end