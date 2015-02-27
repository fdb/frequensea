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
void main() {
    float d = 0.01;
    float cell_size = 0.0005;
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

    color = vec4(1.0, 1.0, 1.0, 0.95) * dot(normalize(v1), normalize(n)) * 0.9;
    color += vec4(0.2, 0.2, 0.1, 1.0);

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
    freq = 97
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    fft = nrf_fft_new(128, 128)
    camera = ngl_camera_new()
    ngl_camera_translate(camera, 0, -0.001, 0)
    ngl_camera_rotate_x(camera, -10)
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(512, 512, 0.0005, 0.0005)
    ngl_model_translate(model, -0.01, -0.04, 0)
end

function draw()
    samples_buffer = nrf_device_get_samples_buffer(device)
    nrf_fft_process(fft, samples_buffer)
    fft_buffer = nrf_fft_get_buffer(fft)

    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_texture_update(texture, fft_buffer, 128, 128)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
end
