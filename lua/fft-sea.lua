-- Visualize FFT data as a texture from the HackRF
-- Calculate normals and lighting

VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
layout (location = 2) in vec2 vt;
flat out vec4 color;
out vec2 texCoord;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
uniform sampler2D uTexture;
uniform float uHue;
uniform float uSaturation;
uniform float uValue;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    float d = 0.001;
    float cell_size = 0.0001;
    vec2 tp = vec2(vt.x, vt.y - 0.5);
    if (tp.y < 0) {
        tp.y = 1-tp.y;
    }
    float y1 = texture(uTexture, tp).r;
    float y2 = texture(uTexture, tp + vec2(cell_size, 0)).r;
    float y3 = texture(uTexture, tp + vec2(0, cell_size)).r;
    y1 *= d;
    y2 *= d;
    y3 *= d;
    vec3 v1 = vec3(vp.x, y1, vp.z);
    vec3 v2 = vec3(vp.x + cell_size, y2, vp.z);
    vec3 v3 = vec3(vp.x, y3, vp.z + cell_size);

    vec3 u = v2 - v1;
    vec3 v = v3 - v1;
    float x = (u.y * v.z) - (u.z * v.y);
    float y = (u.z * v.x) - (u.x * v.z);
    float z = (u.x * v.y) - (u.y * v.x);
    vec3 n = vec3(x, y, z);

    vec3 tint = hsv2rgb(vec3(uHue, uSaturation, uValue));

    color = vec4(1.0, 1.0, 1.0, 1.0) * dot(normalize(v1), normalize(n)) * 2;
    color += vec4(tint, 0.3);
    //color.a = 1.0;

    texCoord = vt;
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(v1, 1.0);
    gl_PointSize = 5;
}
]]

FRAGMENT_SHADER = [[
#version 400
flat in vec4 color;
in vec2 texCoord;
layout (location = 0) out vec4 fragColor;
void main() {
    fragColor = color;
}
]]


function setup()
    freq = 2462
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    fft = nrf_fft_new(128, 512)
    camera = ngl_camera_new()
    ngl_camera_translate(camera, 0, 0, 0)
    ngl_camera_rotate_x(camera, -10)
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    set_colors_for_freq()
    shader2 = ngl_shader_new(GL_LINES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(512, 512, 0.0001, 0.0001)
    ngl_model_translate(model, 0, -0.01, 0.005)
end

function draw()
    --ngl_camera_rotate_y(camera, 0.3)

    samples_buffer = nrf_device_get_samples_buffer(device)
    nrf_fft_process(fft, samples_buffer)
    fft_buffer = nrf_fft_get_buffer(fft)

    ngl_clear(0.0, 0.0, 0.0, 1.0)
    ngl_texture_update(texture, fft_buffer, 128, 512)
    ngl_draw_model(camera, model, shader)
    ngl_draw_model(camera, model, shader2)
end

function set_colors_for_freq()
    ngl_shader_uniform_set_float(shader, "uHue", (freq % 2029) / 2029.0)
    ngl_shader_uniform_set_float(shader, "uValue", 0.1 + (freq % 859) / 859.0)
    ngl_shader_uniform_set_float(shader, "uSaturation", (freq % 727) / 727.0)
end

function on_key(key, mods)
    old_freq = freq
    keys_frequency_handler(key, mods)
    if old_freq ~= freq then
        set_colors_for_freq()
    end
end
