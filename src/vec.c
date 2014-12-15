// Vector and matrix math

#include <math.h>
#include <string.h>

#include "vec.h"

#define TAU 2.0 * M_PI
#define ONE_DEG_IN_RAD (2.0 * M_PI) / 360.0 // 0.017444444
#define ONE_RAD_IN_DEG 360.0 / (2.0 * M_PI) //57.2957795

vec2 vec2_init(float x, float y) {
    vec2 v;
    v.x = x;
    v.y = y;
    return v;
}

vec3 vec3_zero() {
    vec3 v;
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 0.0f;
    return v;
}

vec3 vec3_init(float x, float y, float z) {
    vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

vec3 vec3_sub(const vec3* v1, const vec3* v2) {
    vec3 r;
    r.x = v1->x - v2->x;
    r.y = v1->y - v2->y;
    r.z = v1->z - v2->z;
    return r;
}

float vec3_length(const vec3* v) {
    return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

vec3 vec3_normalize(const vec3* v) {
    vec3 r;
    float l = vec3_length(v);
    if (l == 0.0f) {
        return r;
    } else {
        r.x = v->x / l;
        r.y = v->y / l;
        r.z = v->z / l;
        return r;
    }
}

vec3 vec3_cross(const vec3* v1, const vec3* v2) {
    vec3 r;
    r.x = v1->y * v2->z - v1->z * v2->y;
    r.y = v1->z * v2->x - v1->x * v2->z;
    r.z = v1->x * v2->y - v1->y * v2->x;
    return r;
}

float vec3_dot(const vec3* v1, const vec3* v2) {
    return (v1->x * v2->x + v1->y * v2->y + v1->z * v2->z);
}

vec3 vec3_normal(const vec3* v1, const vec3* v2, const vec3* v3) {
    vec3 u = vec3_sub(v2, v1);
    vec3 v = vec3_sub(v3, v1);
    float x = (u.y * v.z) - (u.z * v.y);
    float y = (u.z * v.x) - (u.x * v.z);
    float z = (u.x * v.y) - (u.y * v.x);
    return vec3_init(x, y, z);
}

mat4 mat4_init_zero() {
    mat4 m;
    m.m[0] = 0.0f;
    m.m[1] = 0.0f;
    m.m[2] = 0.0f;
    m.m[3] = 0.0f;
    m.m[4] = 0.0f;
    m.m[5] = 0.0f;
    m.m[6] = 0.0f;
    m.m[7] = 0.0f;
    m.m[8] = 0.0f;
    m.m[9] = 0.0f;
    m.m[10] = 0.0f;
    m.m[11] = 0.0f;
    m.m[12] = 0.0f;
    m.m[13] = 0.0f;
    m.m[14] = 0.0f;
    m.m[15] = 0.0f;
    return m;
}

mat4 mat4_init_identity() {
    mat4 m;
    m.m[0] = 1.0f;
    m.m[1] = 0.0f;
    m.m[2] = 0.0f;
    m.m[3] = 0.0f;
    m.m[4] = 0.0f;
    m.m[5] = 1.0f;
    m.m[6] = 0.0f;
    m.m[7] = 0.0f;
    m.m[8] = 0.0f;
    m.m[9] = 0.0f;
    m.m[10] = 1.0f;
    m.m[11] = 0.0f;
    m.m[12] = 0.0f;
    m.m[13] = 0.0f;
    m.m[14] = 0.0f;
    m.m[15] = 1.0f;
    return m;
}

mat4 mat4_init_translate(float tx, float ty, float tz) {
    mat4 m;
    m.m[0] = 1.0f;
    m.m[1] = 0.0f;
    m.m[2] = 0.0f;
    m.m[3] = 0.0f;
    m.m[4] = 0.0f;
    m.m[5] = 1.0f;
    m.m[6] = 0.0f;
    m.m[7] = 0.0f;
    m.m[8] = 0.0f;
    m.m[9] = 0.0f;
    m.m[10] = 1.0f;
    m.m[11] = 0.0f;
    m.m[12] = tx;
    m.m[13] = ty;
    m.m[14] = tz;
    m.m[15] = 1.0f;
    return m;
}

mat4 mat4_init_scale(float sx, float sy, float sz) {
    mat4 m;
    m.m[0] = sx;
    m.m[1] = 0.0f;
    m.m[2] = 0.0f;
    m.m[3] = 0.0f;
    m.m[4] = 0.0f;
    m.m[5] = sy;
    m.m[6] = 0.0f;
    m.m[7] = 0.0f;
    m.m[8] = 0.0f;
    m.m[9] = 0.0f;
    m.m[10] = sz;
    m.m[11] = 0.0f;
    m.m[12] = 0.0f;
    m.m[13] = 0.0f;
    m.m[14] = 0.0f;
    m.m[15] = 1.0f;
    return m;
}

mat4 mat4_init_rotation_x(float deg) {
    mat4 m;
    float rad = deg * ONE_DEG_IN_RAD;
    m.m[0] = 1.0f;
    m.m[1] = 0.0f;
    m.m[2] = 0.0f;
    m.m[3] = 0.0f;
    m.m[4] = 0.0f;
    m.m[5] = cos(rad);
    m.m[6] = sin(rad);
    m.m[7] = 0.0f;
    m.m[8] = 0.0f;
    m.m[9] = -sin(rad);
    m.m[10] = cos(rad);
    m.m[11] = 0.0f;
    m.m[12] = 0.0f;
    m.m[13] = 0.0f;
    m.m[14] = 0.0f;
    m.m[15] = 1.0f;
    return m;
}

mat4 mat4_init_rotation_y(float deg) {
    mat4 m;
    float rad = deg * ONE_DEG_IN_RAD;
    m.m[0] = cos(rad);
    m.m[1] = 0.0f;
    m.m[2] = -sin(rad);
    m.m[3] = 0.0f;
    m.m[4] = 0.0f;
    m.m[5] = 1.0f;
    m.m[6] = 0.0f;
    m.m[7] = 0.0f;
    m.m[8] = sin(rad);
    m.m[9] = 0.0f;
    m.m[10] = cos(rad);
    m.m[11] = 0.0f;
    m.m[12] = 0.0f;
    m.m[13] = 0.0f;
    m.m[14] = 0.0f;
    m.m[15] = 1.0f;
    return m;
}

mat4 mat4_init_rotation_z(float deg) {
    mat4 m;
    float rad = deg * ONE_DEG_IN_RAD;
    m.m[0] = cos(rad);
    m.m[1] = sin(rad);
    m.m[2] = 0.0f;
    m.m[3] = 0.0f;
    m.m[4] = -sin(rad);
    m.m[5] = cos(rad);
    m.m[6] = 0.0f;
    m.m[7] = 0.0f;
    m.m[8] = 0.0f;
    m.m[9] = 0.0f;
    m.m[10] = 1.0f;
    m.m[11] = 0.0f;
    m.m[12] = 0.0f;
    m.m[13] = 0.0f;
    m.m[14] = 0.0f;
    m.m[15] = 1.0f;
    return m;
}

mat4 mat4_init_perspective(float fov_y, float aspect, float near, float far) {
    float range = 1.0 / (tan(fov_y * ONE_DEG_IN_RAD * 0.5));
    mat4 m = mat4_init_zero();
    m.m[0] = range / aspect;
    m.m[5] = range;
    m.m[10] = -far / (near - far);
    m.m[11] = 1.0f;
    m.m[14] = (near * far) / (near - far);
    return m;
}

mat4 mat4_init_look_at(const vec3* camera, const vec3* target, const vec3* up) {
    vec3 v1 = vec3_sub(target, camera);
    vec3 zAxis = vec3_normalize(&v1);
    vec3 v2 = vec3_cross(up, &zAxis);
    vec3 xAxis = vec3_normalize(&v2);
    vec3 v3 = vec3_cross(&zAxis, &xAxis);
    vec3 yAxis = vec3_normalize(&v3);

    float ex = -vec3_dot(&xAxis, camera);
    float ey = -vec3_dot(&yAxis, camera);
    float ez = -vec3_dot(&zAxis, camera);

    mat4 m = mat4_init_zero();
    m.m[0] = xAxis.x;
    m.m[1] = xAxis.y;
    m.m[2] = xAxis.z;
    m.m[3] = 0.0f;
    m.m[4] = yAxis.x;
    m.m[5] = yAxis.y;
    m.m[6] = yAxis.z;
    m.m[7] = 0.0f;
    m.m[8] = zAxis.x;
    m.m[9] = zAxis.y;
    m.m[10] = zAxis.z;
    m.m[11] = 0.0f;
    m.m[12] = ex;
    m.m[13] = ey;
    m.m[14] = ez;
    m.m[15] = 1.0f;
    return m;
}

mat4 mat4_mul(const mat4* m1, const mat4* m2) {
    mat4 m;

    m.m[0] = m1->m[0] * m2->m[0] + m1->m[1] * m2->m[4] + m1->m[2] * m2->m[8] + m1->m[3] * m2->m[12];
    m.m[1] = m1->m[0] * m2->m[1] + m1->m[1] * m2->m[5] + m1->m[2] * m2->m[9] + m1->m[3] * m2->m[13];
    m.m[2] = m1->m[0] * m2->m[2] + m1->m[1] * m2->m[6] + m1->m[2] * m2->m[10] + m1->m[3] * m2->m[14];
    m.m[3] = m1->m[0] * m2->m[3] + m1->m[1] * m2->m[7] + m1->m[2] * m2->m[11] + m1->m[3] * m2->m[15];

    m.m[4] = m1->m[4] * m2->m[0] + m1->m[5] * m2->m[4] + m1->m[6] * m2->m[8] + m1->m[7] * m2->m[12];
    m.m[5] = m1->m[4] * m2->m[1] + m1->m[5] * m2->m[5] + m1->m[6] * m2->m[9] + m1->m[7] * m2->m[13];
    m.m[6] = m1->m[4] * m2->m[2] + m1->m[5] * m2->m[6] + m1->m[6] * m2->m[10] + m1->m[7] * m2->m[14];
    m.m[7] = m1->m[4] * m2->m[3] + m1->m[5] * m2->m[7] + m1->m[6] * m2->m[11] + m1->m[7] * m2->m[15];

    m.m[8] = m1->m[8] * m2->m[0] + m1->m[9] * m2->m[4] + m1->m[10] * m2->m[8] + m1->m[11] * m2->m[12];
    m.m[9] = m1->m[8] * m2->m[1] + m1->m[9] * m2->m[5] + m1->m[10] * m2->m[9] + m1->m[11] * m2->m[13];
    m.m[10] = m1->m[8] * m2->m[2] + m1->m[9] * m2->m[6] + m1->m[10] * m2->m[10] + m1->m[11] * m2->m[14];
    m.m[11] = m1->m[8] * m2->m[3] + m1->m[9] * m2->m[7] + m1->m[10] * m2->m[11] + m1->m[11] * m2->m[15];

    m.m[12] = m1->m[12] * m2->m[0] + m1->m[13] * m2->m[4] + m1->m[14] * m2->m[8] + m1->m[15] * m2->m[12];
    m.m[13] = m1->m[12] * m2->m[1] + m1->m[13] * m2->m[5] + m1->m[14] * m2->m[9] + m1->m[15] * m2->m[13];
    m.m[14] = m1->m[12] * m2->m[2] + m1->m[13] * m2->m[6] + m1->m[14] * m2->m[10] + m1->m[15] * m2->m[14];
    m.m[15] = m1->m[12] * m2->m[3] + m1->m[13] * m2->m[7] + m1->m[14] * m2->m[11] + m1->m[15] * m2->m[15];

    return m;
}

mat4 mat4_mul_scalar(const mat4* m, float s) {
    mat4 r;
    r.m[0] = m->m[0] * s;
    r.m[1] = m->m[1] * s;
    r.m[2] = m->m[2] * s;
    r.m[3] = m->m[3] * s;
    r.m[4] = m->m[4] * s;
    r.m[5] = m->m[5] * s;
    r.m[6] = m->m[6] * s;
    r.m[7] = m->m[7] * s;
    r.m[8] = m->m[8] * s;
    r.m[9] = m->m[9] * s;
    r.m[10] = m->m[10] * s;
    r.m[11] = m->m[11] * s;
    r.m[12] = m->m[12] * s;
    r.m[13] = m->m[13] * s;
    r.m[14] = m->m[14] * s;
    r.m[15] = m->m[15] * s;
    return r;
}

float mat4_determinant(const mat4 *m) {
    return
    m->m[12] * m->m[9] * m->m[6] * m->m[3] -
    m->m[8] * m->m[13] * m->m[6] * m->m[3] -
    m->m[12] * m->m[5] * m->m[10] * m->m[3] +
    m->m[4] * m->m[13] * m->m[10] * m->m[3] +
    m->m[8] * m->m[5] * m->m[14] * m->m[3] -
    m->m[4] * m->m[9] * m->m[14] * m->m[3] -
    m->m[12] * m->m[9] * m->m[2] * m->m[7] +
    m->m[8] * m->m[13] * m->m[2] * m->m[7] +
    m->m[12] * m->m[1] * m->m[10] * m->m[7] -
    m->m[0] * m->m[13] * m->m[10] * m->m[7] -
    m->m[8] * m->m[1] * m->m[14] * m->m[7] +
    m->m[0] * m->m[9] * m->m[14] * m->m[7] +
    m->m[12] * m->m[5] * m->m[2] * m->m[11] -
    m->m[4] * m->m[13] * m->m[2] * m->m[11] -
    m->m[12] * m->m[1] * m->m[6] * m->m[11] +
    m->m[0] * m->m[13] * m->m[6] * m->m[11] +
    m->m[4] * m->m[1] * m->m[14] * m->m[11] -
    m->m[0] * m->m[5] * m->m[14] * m->m[11] -
    m->m[8] * m->m[5] * m->m[2] * m->m[15] +
    m->m[4] * m->m[9] * m->m[2] * m->m[15] +
    m->m[8] * m->m[1] * m->m[6] * m->m[15] -
    m->m[0] * m->m[9] * m->m[6] * m->m[15] -
    m->m[4] * m->m[1] * m->m[10] * m->m[15] +
    m->m[0] * m->m[5] * m->m[10] * m->m[15];
}

mat4 mat4_inverse(const mat4 *m) {
    float det = mat4_determinant(m);
    if (det == 0.0f) {
        return mat4_init_zero();
    }
    float inv_det = 1.0f / det;
    mat4 r;
    r.m[0] = inv_det * (
    m->m[9] * m->m[14] * m->m[7] - m->m[13] * m->m[10] * m->m[7] +
    m->m[13] * m->m[6] * m->m[11] - m->m[5] * m->m[14] * m->m[11] -
    m->m[9] * m->m[6] * m->m[15] + m->m[5] * m->m[10] * m->m[15]);
    r.m[1] = inv_det * (
    m->m[13] * m->m[10] * m->m[3] - m->m[9] * m->m[14] * m->m[3] -
    m->m[13] * m->m[2] * m->m[11] + m->m[1] * m->m[14] * m->m[11] +
    m->m[9] * m->m[2] * m->m[15] - m->m[1] * m->m[10] * m->m[15]);
    r.m[2] = inv_det * (
    m->m[5] * m->m[14] * m->m[3] - m->m[13] * m->m[6] * m->m[3] +
    m->m[13] * m->m[2] * m->m[7] - m->m[1] * m->m[14] * m->m[7] -
    m->m[5] * m->m[2] * m->m[15] + m->m[1] * m->m[6] * m->m[15]);
    r.m[3] = inv_det * (
    m->m[9] * m->m[6] * m->m[3] - m->m[5] * m->m[10] * m->m[3] -
    m->m[9] * m->m[2] * m->m[7] + m->m[1] * m->m[10] * m->m[7] +
    m->m[5] * m->m[2] * m->m[11] - m->m[1] * m->m[6] * m->m[11]);
    r.m[4] = inv_det * (
    m->m[12] * m->m[10] * m->m[7] - m->m[8] * m->m[14] * m->m[7] -
    m->m[12] * m->m[6] * m->m[11] + m->m[4] * m->m[14] * m->m[11] +
    m->m[8] * m->m[6] * m->m[15] - m->m[4] * m->m[10] * m->m[15]);
    r.m[5] = inv_det * (
    m->m[8] * m->m[14] * m->m[3] - m->m[12] * m->m[10] * m->m[3] +
    m->m[12] * m->m[2] * m->m[11] - m->m[0] * m->m[14] * m->m[11] -
    m->m[8] * m->m[2] * m->m[15] + m->m[0] * m->m[10] * m->m[15]);
    r.m[6] = inv_det * (
    m->m[12] * m->m[6] * m->m[3] - m->m[4] * m->m[14] * m->m[3] -
    m->m[12] * m->m[2] * m->m[7] + m->m[0] * m->m[14] * m->m[7] +
    m->m[4] * m->m[2] * m->m[15] - m->m[0] * m->m[6] * m->m[15]);
    r.m[7] = inv_det * (
    m->m[4] * m->m[10] * m->m[3] - m->m[8] * m->m[6] * m->m[3] +
    m->m[8] * m->m[2] * m->m[7] - m->m[0] * m->m[10] * m->m[7] -
    m->m[4] * m->m[2] * m->m[11] + m->m[0] * m->m[6] * m->m[11]);
    r.m[8] = inv_det * (
    m->m[8] * m->m[13] * m->m[7] - m->m[12] * m->m[9] * m->m[7] +
    m->m[12] * m->m[5] * m->m[11] - m->m[4] * m->m[13] * m->m[11] -
    m->m[8] * m->m[5] * m->m[15] + m->m[4] * m->m[9] * m->m[15]);
    r.m[9] = inv_det * (
    m->m[12] * m->m[9] * m->m[3] - m->m[8] * m->m[13] * m->m[3] -
    m->m[12] * m->m[1] * m->m[11] + m->m[0] * m->m[13] * m->m[11] +
    m->m[8] * m->m[1] * m->m[15] - m->m[0] * m->m[9] * m->m[15]);
    r.m[10] = inv_det * (
    m->m[4] * m->m[13] * m->m[3] - m->m[12] * m->m[5] * m->m[3] +
    m->m[12] * m->m[1] * m->m[7] - m->m[0] * m->m[13] * m->m[7] -
    m->m[4] * m->m[1] * m->m[15] + m->m[0] * m->m[5] * m->m[15]);
    r.m[11] = inv_det * (
    m->m[8] * m->m[5] * m->m[3] - m->m[4] * m->m[9] * m->m[3] -
    m->m[8] * m->m[1] * m->m[7] + m->m[0] * m->m[9] * m->m[7] +
    m->m[4] * m->m[1] * m->m[11] - m->m[0] * m->m[5] * m->m[11]);
    r.m[12] = inv_det * (
    m->m[12] * m->m[9] * m->m[6] - m->m[8] * m->m[13] * m->m[6] -
    m->m[12] * m->m[5] * m->m[10] + m->m[4] * m->m[13] * m->m[10] +
    m->m[8] * m->m[5] * m->m[14] - m->m[4] * m->m[9] * m->m[14]);
    r.m[13] = inv_det * (
    m->m[8] * m->m[13] * m->m[2] - m->m[12] * m->m[9] * m->m[2] +
    m->m[12] * m->m[1] * m->m[10] - m->m[0] * m->m[13] * m->m[10] -
    m->m[8] * m->m[1] * m->m[14] + m->m[0] * m->m[9] * m->m[14]);
    r.m[14] = inv_det * (
    m->m[12] * m->m[5] * m->m[2] - m->m[4] * m->m[13] * m->m[2] -
    m->m[12] * m->m[1] * m->m[6] + m->m[0] * m->m[13] * m->m[6] +
    m->m[4] * m->m[1] * m->m[14] - m->m[0] * m->m[5] * m->m[14]);
    r.m[15] = inv_det * (
    m->m[4] * m->m[9] * m->m[2] - m->m[8] * m->m[5] * m->m[2] +
    m->m[8] * m->m[1] * m->m[6] - m->m[0] * m->m[9] * m->m[6] -
    m->m[4] * m->m[1] * m->m[10] + m->m[0] * m->m[5] * m->m[10]);
    return r;
}

void mat4_set(mat4* m, const mat4* src) {
    memcpy(m, src, sizeof(mat4));
}

mat4 mat4_translate(const mat4* m, float tx, float ty, float tz) {
    mat4 t = mat4_init_translate(tx, ty, tz);
    return mat4_mul(m, &t);
}

mat4 mat4_scale(const mat4* m, float sx, float sy, float sz) {
    mat4 s = mat4_init_scale(sx, sy, sz);
    return mat4_mul(m, &s);
}

mat4 mat4_rotate_x(const mat4* m, float deg) {
    mat4 r = mat4_init_rotation_x(deg);
    return mat4_mul(m, &r);
}

mat4 mat4_rotate_y(const mat4* m, float deg) {
    mat4 r = mat4_init_rotation_y(deg);
    return mat4_mul(m, &r);
}

mat4 mat4_rotate_z(const mat4* m, float deg) {
    mat4 r = mat4_init_rotation_z(deg);
    return mat4_mul(m, &r);
}

mat4 quat_to_mat4(const quat* q) {
    float x = q->x;
    float y = q->y;
    float z = q->z;
    float w = q->w;

    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;

    mat4 m;
    m.m[0] = 1.0f - (yy + zz);
    m.m[1] = xy + wz;
    m.m[2] = xz - wy;
    m.m[3] = 0.0f;
    m.m[4] = xy - wz;
    m.m[5] = 1.0f - (xx + zz);
    m.m[6] = yz + wx;
    m.m[7] = 0.0f;
    m.m[8] = xz + wy;
    m.m[9] = yz - wx;
    m.m[10] = 1.0f - (xx + yy);
    m.m[11] = 0.0f;
    m.m[12] = 0.0f;
    m.m[13] = 0.0f;
    m.m[14] = 0.0f;
    m.m[15] = 1.0f;
    return m;
}
