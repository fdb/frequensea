-- Visualize IQ data from the HackRF, animate through the spectrum

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0);
    vec2 pt = vec2((vp.x - 0.5) * 1.8, (vp.y - 0.5) * 1.8);
    gl_Position = vec4(pt.x, pt.y, 0.0, 1.0);
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

function setup()
    freq = 200

    device = nrf_device_new(freq, "../rfdata/rf-202.500-2.raw")
    camera = ngl_camera_init_look_at(0, 0, 0) -- Shader ignores camera position, but camera object is required for ngl_draw_model
    shader = ngl_shader_new(GL_LINE_STRIP, VERTEX_SHADER, FRAGMENT_SHADER)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    model = ngl_model_init_positions(3, NRF_SAMPLES_SIZE, device.samples)
    ngl_draw_model(camera, model, shader)

    freq = freq + 0.01
    nrf_device_set_frequency(device, freq)
end
