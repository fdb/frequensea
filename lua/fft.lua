-- Visualize FFT data as a texture

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
    gl_Position = vec4(vp.x, vp.z, 0, 1.0);
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
#endif
uniform sampler2D uTexture;
void main() {
    vec4 r = texture2D(uTexture, texCoord);
    float v = sqrt(r.r * r.r + r.g * r.g) * 0.1;
    gl_FragColor = vec4(v, v, v, 0.95);
}
]]


function setup()
    freq = 97
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    camera = ngl_camera_new_look_at(0, 0, 0) -- Camera is unnecessary but ngl_draw_model requires it
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(2, 2, 1, 1)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    buffer = nrf_device_get_fft_buffer(device)
    ngl_texture_update(texture, buffer.width, buffer.height, buffer.channels, buffer.data)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
end
