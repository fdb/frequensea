-- Visualize IQ data as a texture from the HackRF
-- You want seizures? 'Cause this is how you get seizures.

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
    float offset = -10;
    float power = 20;
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

    color = vec4(1.0, 1.0, 1.0, 0.8) * dot(normalize(v1), normalize(n)) * 1;
    color += vec4(0.2, 0.2, 0.4, 0.5);
    //color *= 0.9;

    float l = abs(1.0 - ((vp.x * vp.x + vp.z * vp.z) * 0.0003));
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

ROOM_VERTEX_SHADER = [[
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
out vec3 color;
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
void main() {
    color = vec3(1.0, 1.0, 1.0) * dot(normalize(vp), normalize(vn)) * 0.5;
    //color += vec3(0.2, 0.5, 0.8);
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(vp, 1.0);
}
]]

ROOM_FRAGMENT_SHADER = [[
#version 400
in vec3 color;
layout (location = 0) out vec4 fragColor;
void main() {
    fragColor = vec4(color, 0.3);
}
]]

function setup()
    camera_x = -30
    camera_y = 18
    camera_z = 50
    freq = 50.1
    device = nrf_device_new(freq, "../rfdata/rf-202.500-2.raw")
    grid_shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    room_shader = ngl_shader_new(GL_TRIANGLES, ROOM_VERTEX_SHADER, ROOM_FRAGMENT_SHADER)
    texture = ngl_texture_new(grid_shader, "uTexture")
    grid_model = ngl_model_new_grid_triangles(100, 100, 1, 1)
    room_model = ngl_model_load_obj("../obj/c004.obj")
end

function draw()
    ngl_clear(0.2, 0.2, 0.2, 1.0)
    buffer = nrf_device_get_samples_buffer(device)
    ngl_texture_update(texture, buffer, 512, 256)
    camera = ngl_camera_new_look_at(camera_x, camera_y, camera_z)
    ngl_draw_model(camera, grid_model, grid_shader)
    ngl_draw_model(camera, room_model, room_shader)
end

function on_key(key, mods)
    keys_camera_handler(key, mods)
    keys_frequency_handler(key, mods)
end

