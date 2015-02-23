-- Visualize a single sample

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
    gl_Position = vec4(vp.x*2.0, vp.z*2.0, 0, 1.0);
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
    float r = texture2D(uTexture, texCoord).a * 10.0;
    gl_FragColor = vec4(r, r, r, 1.0);
}
]]

function setup()
    countdown = 10
    percentage = 0.001
    dir = 1.0
    d = 1.0 + (dir * 0.1)
    device = nrf_device_new(936.2, "../rfdata/rf-2.500-1.raw")
    filter = nrf_iq_filter_new(device.sample_rate, 20e3, 21)

    camera = ngl_camera_new_look_at(0, 0, 0) -- Shader ignores camera position, but camera object is required for ngl_draw_model
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(2, 2, 1, 1)
end

function draw()
    if countdown > 0 then
        samples_buffer = nrf_device_get_samples_buffer(device)
        nrf_iq_filter_process(filter, samples_buffer)
        filter_buffer = nrf_iq_filter_get_buffer(filter)
        countdown = countdown - 1
    elseif percentage > 1 then
        dir = -1.0
        d = 1.0 + (dir * 0.1)
        countdown = 1
    elseif percentage < 0 then
       dir = 1.0 
        d = 1.0 + (dir * 0.1)
    else
        iq_buffer = nrf_buffer_to_iq_lines(filter_buffer, 4, percentage)
        ngl_clear(0.2, 0.2, 0.2, 1.0)
        ngl_texture_update(texture, iq_buffer, 1024, 1024)
        ngl_draw_model(camera, model, shader)
        percentage = percentage * d
    end
end
