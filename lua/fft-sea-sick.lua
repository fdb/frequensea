-- Visualize FFT data as a texture from the HackRF
-- Calculate normals and lighting

VERTEX_SHADER = [[
#ifdef GL_ES
attribute vec3 vp;
attribute vec3 vn;
attribute vec2 vt;
varying vec4 color;
varying vec2 texCoord;


vec4 mod289(vec4 x)
{
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
  return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

vec2 fade(vec2 t) {
  return t*t*t*(t*(t*6.0-15.0)+10.0);
}

// Classic Perlin noise
float cnoise(vec2 P)
{
  vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
  vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
  Pi = mod289(Pi); // To avoid truncation effects in permutation
  vec4 ix = Pi.xzxz;
  vec4 iy = Pi.yyww;
  vec4 fx = Pf.xzxz;
  vec4 fy = Pf.yyww;

  vec4 i = permute(permute(ix) + iy);

  vec4 gx = fract(i * (1.0 / 41.0)) * 2.0 - 1.0 ;
  vec4 gy = abs(gx) - 0.5 ;
  vec4 tx = floor(gx + 0.5);
  gx = gx - tx;

  vec2 g00 = vec2(gx.x,gy.x);
  vec2 g10 = vec2(gx.y,gy.y);
  vec2 g01 = vec2(gx.z,gy.z);
  vec2 g11 = vec2(gx.w,gy.w);

  vec4 norm = taylorInvSqrt(vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
  g00 *= norm.x;  
  g01 *= norm.y;  
  g10 *= norm.z;  
  g11 *= norm.w;  

  float n00 = dot(g00, vec2(fx.x, fy.x));
  float n10 = dot(g10, vec2(fx.y, fy.y));
  float n01 = dot(g01, vec2(fx.z, fy.z));
  float n11 = dot(g11, vec2(fx.w, fy.w));

  vec2 fade_xy = fade(Pf.xy);
  vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
  float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
  return 2.3 * n_xy;
}

float noise1(float f) {
  vec2 v;
  v.x = f;
  v.y = f;
  return cnoise(v);
}
#else
#version 400
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 vn;
layout (location = 2) in vec2 vt;
flat out vec4 color;
out vec2 texCoord;
#endif
uniform mat4 uViewMatrix, uProjectionMatrix;
uniform float uTime;
uniform sampler2D uTexture;
void main() {
    float d = 0.01;
    float cell_size = 0.003;
    float t1 = texture2D(uTexture, vt).r;
    float t2 = texture2D(uTexture, vt + vec2(cell_size, 0)).r;
    float t3 = texture2D(uTexture, vt + vec2(0, cell_size)).r;

    if (t1 > 1.0) {
        t1 = 0.0;
    }
    if (t2 > 1.0) {
        t2 = 0.0;
    }
    if (t3 > 1.0) {
       t3 = 0.0;
    }

    t1 = abs(t1);
    t2 = abs(t2);
    t3 = abs(t3);
    t1 *= d;
    t2 *= d;
    t3 *= d;

    vec3 v1 = vec3(vp.x, t1, vp.z);
    vec3 v2 = vec3(vp.x + cell_size, t2, vp.z);
    vec3 v3 = vec3(vp.x, t3, vp.z + cell_size);

    vec3 u = v2 - v1;
    vec3 v = v3 - v1;
    float x = (u.y * v.z) - (u.z * v.y);
    float y = (u.z * v.x) - (u.x * v.z);
    float z = (u.x * v.y) - (u.y * v.x);
    vec3 n = vec3(x, y, z);

    color = vec4(1.0, 1.0, 1.0, 0.95) * dot(normalize(v1), normalize(n)) * 0.5;
    color += vec4(0.5, 0.25, 0.21, 1.0);

    float l = 1.0 - ((vp.x * vp.x + vp.z * vp.z) * 2.0);
    float ll = 1.0 - ((vp.z * vp.z + vp.y * vp.y) * 2.0);

    float sharkfactor = .25;
    float seasickfactor = 1.25;
    v1.x += sharkfactor*noise1(l*2.0) * sin(uTime/2.0);
    v1.y += .05*noise1(l+uTime/seasickfactor);
    color.g += .5*noise1(l*1.2);
    color.r += 2.0*noise1(ll*1.2);
    texCoord = vt;
    gl_Position = uProjectionMatrix * uViewMatrix * vec4(v1, 1.0);
    gl_PointSize = 5.0;
}
]]

FRAGMENT_SHADER = [[
#ifdef GL_ES
precision mediump float;
varying vec4 color;
#else
#version 400
flat in vec4 color;
in vec2 texCoord;
layout (location = 0) out vec4 gl_FragColor;
#endif
void main() {
    gl_FragColor = color;
}
]]


function setup()
    freq = 97
    device = nrf_device_new(freq, "../rfdata/rf-200.500-big.raw")
    fft = nrf_fft_new(128, 128)

    camera = ngl_camera_new_look_at(0, 0.01, 0.2)
    shader = ngl_shader_new(GL_TRIANGLES, VERTEX_SHADER, FRAGMENT_SHADER)
    texture = ngl_texture_new(shader, "uTexture")
    model = ngl_model_new_grid_triangles(128, 128, 0.006, 0.006)
    ngl_model_translate(model, 0, -0.03, 0)
end

function draw()
    samples_buffer = nrf_device_get_samples_buffer(device)
    nrf_fft_process(fft, samples_buffer)
    fft_buffer = nrf_fft_get_buffer(fft)

    camera_y = 0.01+ .020 *math.abs(math.sin(nwm_get_time() * 0.03))
    camera = ngl_camera_new_look_at(0, camera_y, 0.2)

    ngl_clear(0.2, 0.2, 0.2, 1.0)
    ngl_texture_update(texture, fft_buffer, 128, 128)
    ngl_draw_model(camera, model, shader)
end

function on_key(key, mods)
    keys_frequency_handler(key, mods)
end
