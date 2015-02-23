-- Detect signals on the ISM band (433 MHz) and show the "signature"

if NWM_OPENGL_TYPE == NWM_OPENGL then

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
        texCoord = vt;
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
        float r = texture(uTexture, texCoord).r * 100;
        fragColor = vec4(r, r, r, 1);
    }
    ]]

else

    VERTEX_SHADER = [[
    attribute vec3 vp;
    attribute vec3 vn;
    attribute vec2 vt;
    varying vec3 color;
    varying vec2 texCoord;
    uniform mat4 uViewMatrix, uProjectionMatrix;
    uniform float uTime;
    void main() {
        color = vec3(1.0, 1.0, 1.0);
        texCoord = vt;
        gl_Position = vec4(vp.x*2, vp.z*2, 0, 1.0);
    }
    ]]

    FRAGMENT_SHADER = [[
    precision mediump float;
    varying vec3 color;
    varying vec2 texCoord;
    uniform sampler2D uTexture;
    void main() {
        float r = texture2D(uTexture, texCoord).a * 100;
        gl_FragColor = vec4(r, r, r, 1.0);
    }
    ]]

end

function setup()
    freq = 433
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    filter = nrf_iq_filter_new(device.sample_rate, 200e3, 97)

    detector = nrf_signal_detector_new()
    signal_threshold = 40
    have_signal = false

    camera = ngl_camera_new_look_at(0, 0, 0) -- Camera is unnecessary but ngl_draw_model requires it
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(2, 2, 1, 1)
    percentage_drawn = 0
end

function draw()
    if not have_signal then
        samples_buffer = nrf_device_get_samples_buffer(device)
        nrf_signal_detector_process(detector, samples_buffer)
        sd = nrf_signal_detector_get_standard_deviation(detector)
        print(sd)
        if sd > signal_threshold then
            have_signal = true
            percentage_drawn = 0

        nrf_iq_filter_process(filter, samples_buffer)
        filter_buffer = nrf_iq_filter_get_buffer(filter)

        end

        ngl_clear(0.2, 0.2, 0.2, 1.0)
    else
        iq_buffer = nrf_buffer_to_iq_lines(filter_buffer, 4, percentage_drawn)

        ngl_clear(0.2, 0.2, 0.2, 1.0)
        ngl_texture_update(texture, iq_buffer, 1024, 1024)
        ngl_draw_model(camera, model, shader)

        percentage_drawn = percentage_drawn + 0.001
        if percentage_drawn >= 1 then
            have_signal = false
        end
    end
end
