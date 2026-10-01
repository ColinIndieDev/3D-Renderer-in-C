#pragma once

#include <math.h>
#include <stdint.h>
#include <string.h>

#define MATH_PI 3.14159265358979323846

#define math_min(x, y) ((x) < (y) ? (x) : (y))
#define math_max(x, y) ((x) > (y) ? (x) : (y))
#define math_abs(x) ((x) > 0 ? (x) : -(x))
#define math_clamp(x, n, m) ((x) > (n) ? ((x) < (m) ? (x) : (m)) : (n))

uint64_t math_fibonacci(uint64_t n);
uint64_t math_qpow     (int base, uint32_t exp);
float    math_qsqrt    (float x);
float    math_rad      (float deg);

typedef union {
    float data[4];
    struct {
        float x, y, z, w;
    };
} vec4f;
typedef union {
    float data[3];
    struct {
        float x, y, z;
    };
} vec3f;
#define VEC3F(x, y, z)                                                            \
    (vec3f) { x, y, z }
typedef union {
    float data[2];
    struct {
        float x, y;
    };
} vec2f;
#define VEC2F(x, y)                                                            \
    (vec2f) { x, y }

int   vec2f_cmp      (vec2f a, vec2f b);
vec2f vec2f_add      (vec2f a, vec2f b);
vec2f vec2f_sub      (vec2f a, vec2f b);
vec2f vec2f_mul      (vec2f a, vec2f b);
vec2f vec2f_div      (vec2f a, vec2f b);
vec2f vec2f_float_add(vec2f a, float b);
vec2f vec2f_float_sub(vec2f a, float b);
vec2f vec2f_float_mul(vec2f a, float b);
vec2f vec2f_float_div(vec2f a, float b);
float vec2f_dist     (vec2f a, vec2f b);
float vec2f_dist2    (vec2f a, vec2f b);
float vec2f_dot      (vec2f a, vec2f b);
float vec2f_length   (vec2f a);
vec2f vec2f_clamp    (vec2f v, vec2f n, vec2f m);
vec2f vec2f_norm     (vec2f v);

vec3f vec3f_sub   (vec3f a, vec3f b);
vec3f vec3f_scale (vec3f v, float s);
vec3f vec3f_add   (vec3f a, vec3f b);
float vec3f_dot   (vec3f a, vec3f b);
vec3f vec3f_cross (vec3f a, vec3f b);
float vec3f_length(vec3f v);
vec3f vec3f_norm  (vec3f v);

typedef float mat4f[16];

void  mat4f_identity (mat4f *m);
void  mat4f_translate(mat4f *m, vec3f v);
void  mat4f_rotate   (mat4f *m, float angle_rad, vec3f axis);
void  mat4f_scale    (mat4f *m, vec3f v);
vec4f mat4f_mul_vec4f(mat4f *m, vec4f v);
void  mat4f_mul      (mat4f *a, mat4f *b, mat4f *dest);
void  mat4f_ortho    (mat4f *m, float left, float right, float bottom, float top, float near, float far);
void  mat4f_perspective(mat4f *m, float near, float far, float fov, float aspect);
void  mat4f_look_at  (mat4f *m, vec3f eye, vec3f center, vec3f up);
float mat4f_det      (mat4f *m);
float mat4f_get_float(mat4f *m, unsigned int c, unsigned int r);
void  mat4f_inv      (mat4f *m, mat4f *out);
void  mat4f_transpose(mat4f *m, mat4f *out);
