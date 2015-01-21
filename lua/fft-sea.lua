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
    float d = 0.005;
    float t1 = texture(uTexture, vt).r * d;
    float t2 = texture(uTexture, vt + vec2(0.002, 0)).r * d;
    float t3 = texture(uTexture, vt + vec2(0, 0.002)).r * d;
    vec3 v1 = vec3(vp.x, t1, vp.z);
    vec3 v2 = vec3(vp.x + 0.002, t2, vp.z);
    vec3 v3 = vec3(vp.x, t3, vp.z + 0.002);

    vec3 u = v2 - v1;
    vec3 v = v3 - v1;
    float x = (u.y * v.z) - (u.z * v.y);
    float y = (u.z * v.x) - (u.x * v.z);
    float z = (u.x * v.y) - (u.y * v.x);
    vec3 n = vec3(x, y, z);

    color = vec4(1.0, 1.0, 1.0, 0.95) * dot(normalize(v1), normalize(n)) * 0.5;
    color += vec4(0.2, 0.2, 0.1, 1.0);

    float l = 1.0 - ((vp.x * vp.x + vp.z * vp.z) * 2.0);
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


function setup()
    freq = 97
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw", 0.1)
    camera = ngl_camera_new_look_at(0, 0.01, 0.2)
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(512, 512, 0.001, 0.001)
    ngl_model_translate(model, 0, -0.02, 0)
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_texture_update(texture, GL_RED, 1024, 512, device.fft)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
end
