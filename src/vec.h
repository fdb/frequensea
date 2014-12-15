// Vector and matrix math

#ifndef VEC_H
#define VEC_H

typedef struct {
    float x;
    float y;
} vec2;

typedef struct {
    float x;
    float y;
    float z;
} vec3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
}  vec4;

// Stored in column order:
// 0  4  8 12
// 1  5  9 13
// 2  6 10 14
// 3  7 11 15
typedef struct {
    float m[16];
} mat4;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} quat;

vec2 vec2_init(float x, float y);

vec3 vec3_zero();
vec3 vec3_init(float x, float y, float z);
vec3 vec3_sub(const vec3* v1, const vec3* v2);
float vec3_length(const vec3* v);
vec3 vec3_normalize(const vec3* v);
vec3 vec3_cross(const vec3* v1, const vec3* v2);
float vec3_dot(const vec3* v1, const vec3* v2);
vec3 vec3_normal(const vec3* v1, const vec3* v2, const vec3* v3);

mat4 mat4_init_zero();
mat4 mat4_init_identity();
mat4 mat4_init_translate(float tx, float ty, float tz);
mat4 mat4_init_scale(float sx, float sy, float sz);
mat4 mat4_init_rotation_x(float deg);
mat4 mat4_init_rotation_y(float deg);
mat4 mat4_init_rotation_z(float deg);
mat4 mat4_init_perspective(float fov_y, float aspect, float near, float far);
mat4 mat4_init_look_at(const vec3* camera, const vec3* target, const vec3* up);

mat4 mat4_mul(const mat4* m1, const mat4* m2);
mat4 mat4_mul_scalar(const mat4* m, float s);
float mat4_determinant(const mat4 *m);
mat4 mat4_inverse(const mat4 *m);
void mat4_set(mat4* m, const mat4* src);

mat4 mat4_translate(const mat4* m, float tx, float ty, float tz);
mat4 mat4_scale(const mat4* m, float sx, float sy, float sz);
mat4 mat4_rotate_x(const mat4* m, float deg);
mat4 mat4_rotate_y(const mat4* m, float deg);
mat4 mat4_rotate_z(const mat4* m, float deg);

mat4 quat_to_mat4(const quat* q);

#endif // VEC_H
