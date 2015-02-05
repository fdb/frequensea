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
    //float ll = 1.0 - ((vp.z * vp.z + vp.y * vp.y) * 2.0);
    color *= vec4(l/5.0, l/2.0, l, l);

    float sharkfactor = .25;
    float seasickfactor = 2.25;
    v1.x += sharkfactor*noise1(l*2.0) * sin(uTime/2.0);
    v1.y += .05*noise1(l+uTime/seasickfactor);
    color.r *= 8.0*noise1(l*(200.0*sin(uTime))*10.0);
    //color.g *= 10.0*noise1(ll*20.0);
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
    freq = 100.9
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw", 0.1)
    camera = ngl_camera_new_look_at(0, 0.01, 0.2)
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(512, 512, 0.002, 0.002)
    ngl_model_translate(model, 0, -0.02, 0)
end

function draw()

    camera_y = 0.01+ .020 *math.abs(math.sin(nwm_get_time() * 0.03))
    camera = ngl_camera_new_look_at(0, camera_y, 0.2)

    ngl_clear(0.2, 0.2, 0.2, 1.0)
    buffer = nrf_device_get_fft_buffer(device)
    ngl_texture_update(texture, buffer.width, buffer.height, buffer.channels, buffer.data)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
end
