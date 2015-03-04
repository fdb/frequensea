-- Visualize FFT data as a texture from the HackRF
-- Calculate normals and lighting

GRADIENT_VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
layout (location = 2) in vec2 vt;
out vec4 color;

uniform float uRed;
uniform float uGreen;
uniform float uBlue;
uniform float uAlpha;
uniform float uGlobalAlpha;

void main () {
    if (vp.z > 0) {
        color = vec4(uRed, uGreen, uBlue, uAlpha);
    } else {
        color = vec4(uRed * 0.01, uGreen * 0.01, uBlue * 0.01, uAlpha);
    }
    color.a *= uGlobalAlpha;
    gl_Position = vec4(vp.x * 2, vp.z * 2, 0, 1);
}
]]

GRADIENT_FRAGMENT_SHADER = [[
#version 400
in vec4 color;
layout (location = 0) out vec4 fragColor;
void main() {
    fragColor = color;
}
]]

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
uniform float uRed;
uniform float uGreen;
uniform float uBlue;
uniform float uAlpha;
uniform float uLight;
uniform float uAmbient;
uniform float uGlobalAlpha;

void main() {
    float d = 0.004;
    float cell_size = 0.001;
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

    color = vec4(1.0, 1.0, 1.0, 1.0) * dot(normalize(v1), normalize(n)) * uLight;
    color += vec4(uRed, uGreen, uBlue, uAlpha) * uAmbient;
    color.a *= uGlobalAlpha;

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
    {start_freq=87.5, end_freq=108.0, label="FM Radio", red=0.8, green=0.0, blue=0.0},
    {start_freq=165, end_freq=175, label="TETRA Police Radio", red=0.1, green=0.1, blue=0.9},
    {start_freq=430, end_freq=440, label="ISM", red=0.25, green=0.1, blue=0.5},
    {start_freq=830, end_freq=980, label="GSM", red=0.0, green=0.8, blue=0.8},
    {start_freq=1785, end_freq=1880, label="Mobile communications", red=0.0, green=0.2, blue=0.9},
    {start_freq=2400, end_freq=2500, label="Wi-Fi", red=0.2, green=0.6, blue=0.9}
}

INTERESTING_FREQUENCIES = {97.6, 169.9, 434.1, 930, 1846, 2422}

FREQUENCY_INDEX = 1
FREQUENCY_DISPLAY_TIME = 300

STATE_NORMAL = 1
STATE_FADING_OUT = 2
STATE_FADING_IN = 3

function round(val, decimal)
  local exp = decimal and 10^decimal or 1
  return math.ceil(val * exp - 0.5) / exp
end

function find_range(freq)
    for _, freq_info in pairs(FREQUENCIES) do
        if freq_info.start_freq <= freq and freq_info.end_freq >= freq then
            return freq_info
        end
    end
end

-- Switch to the next interesting frequency
function switch_freq()
    frequency_index = frequency_index + 1
    if frequency_index > #INTERESTING_FREQUENCIES then
        frequency_index = 1
    end
    freq_to_switch = INTERESTING_FREQUENCIES[frequency_index]
    state = STATE_FADING_OUT
end

function set_freq(new_freq)
    d = new_freq - freq
    new_freq = round(new_freq, 3)
    freq = nrf_device_set_frequency(device, new_freq)
    nrf_fft_shift(fft, (device.sample_rate / 1e6) / d)
    print("Frequency: " .. new_freq)
    info = find_range(freq)
    if info then
        ngl_shader_uniform_set_float(shader, "uRed", info.red)
        ngl_shader_uniform_set_float(shader, "uGreen", info.green)
        ngl_shader_uniform_set_float(shader, "uBlue", info.blue)
        ngl_shader_uniform_set_float(shader, "uAlpha", 1)
        ngl_shader_uniform_set_float(shader, "uLight", 0.8)
        ngl_shader_uniform_set_float(shader, "uAmbient", 0.2)

        ngl_shader_uniform_set_float(line_shader, "uLight", 10.0)
        ngl_shader_uniform_set_float(line_shader, "uAmbient", 10.0)

        ngl_shader_uniform_set_float(grad_shader, "uRed", info.red)
        ngl_shader_uniform_set_float(grad_shader, "uGreen", info.green)
        ngl_shader_uniform_set_float(grad_shader, "uBlue", info.blue)
        ngl_shader_uniform_set_float(grad_shader, "uAlpha", 1)
    else
        ngl_shader_uniform_set_float(shader, "uRed", 0.2)
        ngl_shader_uniform_set_float(shader, "uGreen", 0.2)
        ngl_shader_uniform_set_float(shader, "uBlue", 0.4)
        ngl_shader_uniform_set_float(shader, "uAlpha", 0.2)
        ngl_shader_uniform_set_float(shader, "uLight", 0.9)
        ngl_shader_uniform_set_float(shader, "uAmbient", 0.4)

        ngl_shader_uniform_set_float(line_shader, "uLight", 10.0)
        ngl_shader_uniform_set_float(line_shader, "uAmbient", 1.0)
        ngl_shader_uniform_set_float(line_shader, "uAlpha", 1.0)

        ngl_shader_uniform_set_float(grad_shader, "uRed", 0.2)
        ngl_shader_uniform_set_float(grad_shader, "uGreen", 0.2)
        ngl_shader_uniform_set_float(grad_shader, "uBlue", 0.2)
        ngl_shader_uniform_set_float(grad_shader, "uAlpha", 1)
    end
    frequency_display_countdown = FREQUENCY_DISPLAY_TIME
end

-- Receive OSC events
function handle_message(path, args)
    if path == "/wii/1/accel/pry" then
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
    frequency_index = 1
    freq = INTERESTING_FREQUENCIES[frequency_index]
    freq_offset = 100000

    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    fft = nrf_fft_new(128, 512)
    player = nrf_player_new(device, NRF_DEMODULATE_WBFM, freq_offset)

    server = nosc_server_new(2222, handle_message)

    camera = ngl_camera_new()

    grad_model = ngl_model_new_grid_triangles(2, 2, 1, 1)
    grad_shader = ngl_shader_new(GL_TRIANGLES, GRADIENT_VERTEX_SHADER, GRADIENT_FRAGMENT_SHADER)

    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    line_shader = ngl_shader_new(GL_LINES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(256, 512, 0.001, 0.001)
    ngl_model_translate(model, 0, -0.02, 0.005)

    skybox = ngl_skybox_new("../img/negz.jpg", "../img/posz.jpg", "../img/posy.jpg", "../img/negy.jpg", "../img/negx.jpg", "../img/posx.jpg")
    freq_font = ngl_font_new("../fonts/Roboto-Bold.ttf", 72)
    info_font = ngl_font_new("../fonts/RobotoCondensed-Bold.ttf", 48)

    frequency_display_countdown = FREQUENCY_DISPLAY_TIME
    set_freq(freq)

    set_global_alpha(1.0)
    state = STATE_NORMAL
end

function set_global_alpha(new_alpha)
    global_alpha = new_alpha
    ngl_shader_uniform_set_float(shader, "uGlobalAlpha", global_alpha)
    ngl_shader_uniform_set_float(line_shader, "uGlobalAlpha", global_alpha)
    ngl_shader_uniform_set_float(grad_shader, "uGlobalAlpha", global_alpha)
end

function draw()
    if state == STATE_NORMAL then
    elseif state == STATE_FADING_OUT then
        set_global_alpha(global_alpha - 0.01)
        nrf_player_set_gain(player, global_alpha)
        if global_alpha <= 0 then
            set_freq(freq_to_switch)
            state = STATE_FADING_IN
        end
    elseif state == STATE_FADING_IN then
        set_global_alpha(global_alpha + 0.01)
        nrf_player_set_gain(player, global_alpha)
        if global_alpha >= 1 then
            state = STATE_NORMAL
        end
    end

    samples_buffer = nrf_device_get_samples_buffer(device)
    nrf_fft_process(fft, samples_buffer)
    fft_buffer = nrf_fft_get_buffer(fft)

    nosc_server_update(server)

    ngl_clear(0, 0, 0, 1)

    ngl_draw_background(camera, grad_model, grad_shader)

    ngl_texture_update(texture, fft_buffer, 128, 512)
    ngl_draw_model(camera, model, shader)
    ngl_draw_model(camera, model, line_shader)

    frequency_display_countdown = frequency_display_countdown - 1
    if frequency_display_countdown > 0 then
        ngl_font_draw(freq_font, freq - (freq_offset / 1000000), 50, 100)
        info = find_range(freq)
        if info then
            ngl_font_draw(info_font, info.label, 50, 150)
        end
    end
end

function on_key(key, mods)
    if (mods == 1) then -- Shift key
        d = 10
    elseif (mods == 4) then -- Alt key
        d = 0.001
    else
        d = 0.1
    end
    if key == KEY_A then
        switch_freq()
    elseif key == KEY_RIGHT then
        set_freq(freq + d)
    elseif key == KEY_LEFT then
        set_freq(freq - d)
    end
end
