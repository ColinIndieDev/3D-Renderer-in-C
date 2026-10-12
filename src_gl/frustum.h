#pragma once

#include <cpstd/mathplus.h>
#include <stdbool.h>

typedef struct {
    vec3f normal;
    float dist;
} plane_t;

enum {
    PLANE_LEFT = 0,
    PLANE_RIGHT,
    PLANE_BOTTOM,
    PLANE_TOP,
    PLANE_NEAR,
    PLANE_FAR
};

typedef struct {
    plane_t planes[6];
} frustum_t;

void frustum_update(frustum_t *f, mat4f *view);
bool is_sphere_visible(frustum_t *f, vec3f center, float radius);
