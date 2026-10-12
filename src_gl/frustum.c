#include "frustum.h"

float plane_get_dist(plane_t *plane, vec3f p) {
    return vec3f_dot(plane->normal, p) + plane->dist;
}

void frustum_update(frustum_t *f, mat4f *view) {
    mat4f vp;
    mat4f_transpose(view, &vp);
    vec3f n0 = VEC3F(mat4f_get_float(&vp, 3, 0) + mat4f_get_float(&vp, 0, 0),
                     mat4f_get_float(&vp, 3, 1) + mat4f_get_float(&vp, 0, 1),
                     mat4f_get_float(&vp, 3, 2) + mat4f_get_float(&vp, 0, 2));
    f->planes[PLANE_LEFT].normal = n0;
    f->planes[PLANE_LEFT].dist = mat4f_get_float(&vp, 3, 3) + mat4f_get_float(&vp, 0, 3);

    vec3f n1 = VEC3F(mat4f_get_float(&vp, 3, 0) - mat4f_get_float(&vp, 0, 0),
                     mat4f_get_float(&vp, 3, 1) - mat4f_get_float(&vp, 0, 1),
                     mat4f_get_float(&vp, 3, 2) - mat4f_get_float(&vp, 0, 2));
    f->planes[PLANE_RIGHT].normal = n1;
    f->planes[PLANE_RIGHT].dist = mat4f_get_float(&vp, 3, 3) - mat4f_get_float(&vp, 0, 3);

    vec3f n2 = VEC3F(mat4f_get_float(&vp, 3, 0) + mat4f_get_float(&vp, 1, 0),
                     mat4f_get_float(&vp, 3, 1) + mat4f_get_float(&vp, 1, 1),
                     mat4f_get_float(&vp, 3, 2) + mat4f_get_float(&vp, 1, 2));
    f->planes[PLANE_BOTTOM].normal = n2;
    f->planes[PLANE_BOTTOM].dist = mat4f_get_float(&vp, 3, 3) + mat4f_get_float(&vp, 1, 3);

    vec3f n3 = VEC3F(mat4f_get_float(&vp, 3, 0) - mat4f_get_float(&vp, 1, 0),
                     mat4f_get_float(&vp, 3, 1) - mat4f_get_float(&vp, 1, 1),
                     mat4f_get_float(&vp, 3, 2) - mat4f_get_float(&vp, 1, 2));
    f->planes[PLANE_TOP].normal = n3;
    f->planes[PLANE_TOP].dist = mat4f_get_float(&vp, 3, 3) - mat4f_get_float(&vp, 1, 3);

    vec3f n4 = VEC3F(mat4f_get_float(&vp, 3, 0) + mat4f_get_float(&vp, 2, 0),
                     mat4f_get_float(&vp, 3, 1) + mat4f_get_float(&vp, 2, 1),
                     mat4f_get_float(&vp, 3, 2) + mat4f_get_float(&vp, 2, 2));
    f->planes[PLANE_NEAR].normal = n4;
    f->planes[PLANE_NEAR].dist = mat4f_get_float(&vp, 3, 3) + mat4f_get_float(&vp, 2, 3);

    vec3f n5 = VEC3F(mat4f_get_float(&vp, 3, 0) - mat4f_get_float(&vp, 2, 0),
                     mat4f_get_float(&vp, 3, 1) - mat4f_get_float(&vp, 2, 1),
                     mat4f_get_float(&vp, 3, 2) - mat4f_get_float(&vp, 2, 2));
    f->planes[PLANE_FAR].normal = n5;
    f->planes[PLANE_FAR].dist = mat4f_get_float(&vp, 3, 3) - mat4f_get_float(&vp, 2, 3);

    for (int i = 0; i < 6; ++i) {
        vec3f normal = f->planes[i].normal;
        float len = vec3f_length(f->planes[i].normal);
        f->planes[i].normal = VEC3F(normal.x / len, normal.y / len, normal.z / len);
        f->planes[i].dist /= len;
    }
}

bool is_sphere_visible(frustum_t *f, vec3f center, float radius) {
    for (int i = 0; i < 6; ++i) {
        if (plane_get_dist(&f->planes[i], center) < -radius) {
            return false;
        }
    }
    return true;
}
