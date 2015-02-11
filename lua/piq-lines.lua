-- Visualize IQ data from the HackRF

VERTEX_SHADER = [[
#ifdef GL_ES
precision mediump float;
#endif
attribute vec2 vp;
attribute vec2 vt;
varying vec2 uv;
void main(void) {
  uv = vt;
  gl_Position = vec4(vp.x, vp.y, 0, 1);
}
]]

FRAGMENT_SHADER = [[
#ifdef GL_ES
precision mediump float;
#endif
uniform sampler2D texture;
varying vec2 uv;
void main(void) {
  vec4 c = texture2D(texture, uv);
  float v = c.r;
  gl_FragColor = vec4(v, v, v, 1);
}
]]

function setup()
    freq = 97
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    camera = ngl_camera_new_look_at(0, 0, 0) -- Shader ignores camera position, but camera object is required for ngl_draw_model
    shader = ngl_shader_new(GL_LINE_STRIP, VERTEX_SHADER, FRAGMENT_SHADER)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    buffer = nrf_device_get_samples_buffer(device)
    model = ngl_model_new(buffer.channels, buffer.width * buffer.height, buffer.data)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
end
