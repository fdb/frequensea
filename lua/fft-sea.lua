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
uniform float uLight;
uniform float uAmbient;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    float d = 0.0004;
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

    color = vec4(1.0, 1.0, 1.0, 1.0) * dot(normalize(v1), normalize(n)) * uLight;
    color += vec4(tint, 0.3) * uAmbient;
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

FREQUENCIES = {
    {start_freq=87.5, end_freq=108.0, label="FM Radio", hue=1.0},
    {start_freq=430, end_freq=440, label="ISM", hue=0.25},
    {start_freq=830, end_freq=980, label="GSM", hue=0.7},
    {start_freq=2400, end_freq=2425, label="Wi-Fi", hue=0.2}
}

INTERESTING_FREQUENCIES = {97, 434, 930, 2422}

function find_range(freq)
    for _, freq_info in pairs(FREQUENCIES) do
        if freq_info.start_freq <= freq and freq_info.end_freq >= freq then
            return freq_info
        end
    end
end


function set_freq(new_freq)
    d = new_freq - freq
    freq = nrf_device_set_frequency(device, new_freq)
    nrf_fft_shift(fft, (device.sample_rate / 1e6) / d)
    --set_colors_for_freq()
    print("Frequency: " .. new_freq)
    info = find_range(freq)
    if info then
        ngl_shader_uniform_set_float(shader, "uHue", info.hue)
        ngl_shader_uniform_set_float(shader, "uSaturation", 1)
        ngl_shader_uniform_set_float(shader, "uValue", 1)
        ngl_shader_uniform_set_float(shader, "uLight", 2.0)
        ngl_shader_uniform_set_float(shader, "uAmbient", 2.5)
        ngl_shader_uniform_set_float(line_shader, "uSaturation", 0)
        ngl_shader_uniform_set_float(line_shader, "uValue", 1)
        ngl_shader_uniform_set_float(line_shader, "uAmbient", 1.1)
        ngl_shader_uniform_set_float(line_shader, "uLight", 0.5)
    else
        ngl_shader_uniform_set_float(shader, "uHue", 0.5)
        ngl_shader_uniform_set_float(shader, "uSaturation", 0)
        ngl_shader_uniform_set_float(shader, "uValue", 0.5)
        ngl_shader_uniform_set_float(shader, "uLight", 2.0)
        ngl_shader_uniform_set_float(shader, "uAmbient", 0.8)
    end
end

-- Receive OSC events
function handle_message(path, args)
    if path == "/wii/1/accel/pry" then
        pitch = args[1] - 0.5
        if math.abs(pitch) > 0.3 then
            ngl_model_translate(model, 0.0, -pitch * 0.0005, 0.0)
        end
        roll = args[2] - 0.5
        if math.abs(roll) > 0.2 then
            d = roll * 0.2
            d = math.floor(d * 100) / 100
            set_freq(freq + d)
        end
    elseif path == "/wii/1/button/Up" then
        print(args[1])
        ngl_model_translate(model, 0.0, -0.001, 0.0)
    elseif path == "/wii/1/button/Down" then
        ngl_model_translate(model, 0.0, 0.001, 0.0)
    elseif path == "/wii/1/button/A" and args[1] == 1 then
        set_freq(math.random(4000))
    end
end

function setup()
    freq = 2422
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    fft = nrf_fft_new(256, 512)

    server = nosc_server_new(2222, handle_message)

    camera = ngl_camera_new()
    ngl_camera_rotate_x(camera, -10)
    ngl_camera_translate(camera, 0, 0, 0)
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    --set_colors_for_freq()
    line_shader = ngl_shader_new(GL_LINES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(512, 1024, 0.0001, 0.0001)
    ngl_model_translate(model, 0, -0.005, 0.005)

    set_freq(freq)
end

function draw()
    samples_buffer = nrf_device_get_samples_buffer(device)
    nrf_fft_process(fft, samples_buffer)
    fft_buffer = nrf_fft_get_buffer(fft)

    nosc_server_update(server)

    ngl_clear(0.0, 0.0, 0.0, 1.0)

    ngl_texture_update(texture, fft_buffer, 256, 512)
    ngl_draw_model(camera, model, shader)
    ngl_draw_model(camera, model, line_shader)
end

function set_colors_for_freq()
    ngl_shader_uniform_set_float(shader, "uHue", (freq % 2029) / 2029.0)
    ngl_shader_uniform_set_float(shader, "uValue", 0.3 + (freq % 859) / 859.0)
    ngl_shader_uniform_set_float(shader, "uSaturation", 0.5+(freq % 727) / 727.0)
end

function on_key(key, mods)
    if (mods == 1) then -- Shift key
        d = 10
    elseif (mods == 4) then -- Alt key
        d = 0.001
    else
        d = 0.1
    end
    if key == KEY_RIGHT then
        set_freq(freq + d)
    elseif key == KEY_LEFT then
        set_freq(freq - d)
    end
end
