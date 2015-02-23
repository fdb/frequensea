-- Visualize IQ data from the HackRF in 3D.
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
    float offset = 0.0;
    float power = 1;
    float t1 = offset + texture(uTexture, vt).r * power;
    float t2 = offset + texture(uTexture, vt + vec2(0.01, 0)).r * power;
    float t3 = offset + texture(uTexture, vt + vec2(0, 0.01)).r * power;
    vec3 v1 = vec3(vp.x, t1, vp.z);
    vec3 v2 = vec3(vp.x + 0.01, t2, vp.z);
    vec3 v3 = vec3(vp.x, t3, vp.z + 0.01);

    vec3 u = v2 - v1;
    vec3 v = v3 - v1;
    float x = (u.y * v.z) - (u.z * v.y);
    float y = (u.z * v.x) - (u.x * v.z);
    float z = (u.x * v.y) - (u.y * v.x);
    vec3 n = vec3(x, y, z);

    color = vec4(1.0, 1.0, 1.0, 1.0) * dot(normalize(v1), normalize(n)) * 0.5;
    color += vec4(0.2, 0.2, 0.4, 0.5);
    color *= 2;

    float l = 1.0 - ((vp.x * vp.x + vp.z * vp.z) * 1.0);
    color *= vec4(l, l, l, l);
    texCoord = vt;
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(v1, 1.0);
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

camera_x = 0.0
camera_y = 1
camera_z = 1

function setup()
    freq = 97
    device = nrf_device_new(freq, "../rfdata/rf-202.500-2.raw")
    filter = nrf_iq_filter_new(device.sample_rate, 200e3, 21)

    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(256, 256, 0.005, 0.005)
    ngl_model_translate(model, 0, -0.1, 0)
end

function draw()
    samples_buffer = nrf_device_get_samples_buffer(device)
    nrf_iq_filter_process(filter, samples_buffer)
    filter_buffer = nrf_iq_filter_get_buffer(filter)
    iq_buffer = nrf_buffer_to_iq_points(filter_buffer) -- nrf_device_get_iq_buffer(device)


    ngl_clear(0.2, 0.2, 0.2, 1.0)
    camera = ngl_camera_new_look_at(camera_x, camera_y, camera_z)
    ngl_texture_update(texture, iq_buffer, 256, 256)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_camera_handler(key, mods)
    keys_frequency_handler(key, mods)
end
