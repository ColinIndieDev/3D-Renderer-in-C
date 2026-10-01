#define RENDERER_IMPL
#include "renderer.h"

#include <cpstd/rand.h>
#include <cplt/cplt.h>
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "shader.h"

#define KEY_ESCAPE(b, n) ((n) == 1 && (b)[0] == 0x1b)
#define KEY_ENTER(b) ((b)[0] == 0x0d)
#define KEY_BACKSPACE(b) ((b)[0] == 0x7f || (b)[0] == 0x08)

typedef struct {
    vec3f pos;
    vec3f front;
    vec3f up;
    float yaw;
    float pitch;
    float last_x;
    float last_y;
    float fov;
    float sensitivity;
    bool first_mouse;
} cam3D_t;

float cube_vertices[] = {
    // Front (-Z)
    // x,    y,    z,     nx,   ny,   nz,    u,    v
    -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
     1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
     1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
    -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
    -1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
     1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,

    // Back (+Z)
    -1.0f, -1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  1.0f, 0.0f,
     1.0f, -1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  0.0f, 0.0f,
     1.0f,  1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  0.0f, 1.0f,
    -1.0f,  1.0f,  1.0f,  0.0f, 0.0f,  1.0f,  1.0f, 1.0f,

    // Top (+Y)
    -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
     1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,

    // Bottom (-Y)
    -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,

    // Left (-X)
    -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

    // Right (+X)
     1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f
};

vec3f plane_vertices[] = {
    VEC3F(-100, 0, -100), VEC3F(-100, 0, 100), VEC3F(100, 0, -100),
    VEC3F(100, 0, 100), VEC3F(-100, 0, 100), VEC3F(100, 0, -100)
};

#define LAT_BANDS 8
#define LONG_BANDS 8

vec3f sphere_vertices[LAT_BANDS * LONG_BANDS * 6];
size_t sphere_vertices_size = 0;

#define N 100

typedef struct {
    vec3f normal;
    float dist;
} plane_t;

float plane_get_dist(plane_t *plane, vec3f p) {
    return vec3f_dot(plane->normal, p) + plane->dist;
}

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

cam3D_t cam3D;
frustum_t frustum;

vec3f get_sphere_vertex(int i, int j) {
  float radius = 1.0f;
  float theta_step = MATH_PI / LAT_BANDS;
  float phi_step = 2.0f * MATH_PI / LONG_BANDS;

  float theta = i * theta_step;
  float phi = j * phi_step;
  float sin_theta = sinf(theta);
  float cos_theta = cosf(theta);
  float sin_phi = sinf(phi);
  float cos_phi = cosf(phi);
  vec3f v;
  v.x = radius * sin_theta * cos_phi;
  v.y = radius * cos_theta;
  v.z = radius * sin_theta * sin_phi;
  return v;
}

int main() {
    pcg_rand_seed();

    cplt_begin();
    init_renderer(cplt_get_screen_width(), cplt_get_screen_height());
    mat4f_perspective(&perspective, 0.01f, 1000.0f, math_rad(45.0f), 800.0f / 600);
    cam3D = (cam3D_t){
        .pos = {0.0f, 10.0f, 0.0f},
        .front = {0.0f, 0.0f, -1.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .first_mouse = true,
        .yaw = -90.0f,
        .pitch = 0.0f,
        .last_x = 0.0f,
        .last_y = 0.0f,
        .fov = 45.0f,
        .sensitivity = 0.1f,
    };

    float radius = 1.0f;
    float theta_step = MATH_PI / LAT_BANDS;
    float phi_step = 2.0f * MATH_PI / LONG_BANDS;
    for (int i = 0; i < LAT_BANDS; i++) {
        for (int j = 0; j < LONG_BANDS; j++) {
            vec3f v0 = get_sphere_vertex(i, j);
            vec3f v1 = get_sphere_vertex(i + 1, j);
            vec3f v2 = get_sphere_vertex(i, j + 1);
            vec3f v3 = get_sphere_vertex(i + 1, j + 1);

            sphere_vertices[sphere_vertices_size++] = v0;
            sphere_vertices[sphere_vertices_size++] = v1;
            sphere_vertices[sphere_vertices_size++] = v2;
            sphere_vertices[sphere_vertices_size++] = v2;
            sphere_vertices[sphere_vertices_size++] = v1;
            sphere_vertices[sphere_vertices_size++] = v3;
        }
    }

    vec3f *positions = malloc(N * sizeof(vec3f));
    for (int i = 0; i < N; ++i) {
        positions[i] = VEC3F(pcg_randf_range(-25.0f, 25.0f), pcg_randf_range(-25.0f, 25.0f), pcg_randf_range(-100.0f, 100.0f));
    }

    // Init renderer texture
    texture = renderer_create_texture("texture.png");

    while (1) {
        unsigned char key_buffer[3] = {0};
        ssize_t n = 0;
        cplt_get_key_pressed(key_buffer, &n);

        if (KEY_ESCAPE(key_buffer, n)) {
            break;
        } else if (n == 1) {
            if (KEY_ENTER(key_buffer)) {
            } else if (KEY_BACKSPACE(key_buffer)) {
            } else {
                if (key_buffer[0] == 'w') {
                    cam3D.pos.z -= 1;
                }
                if (key_buffer[0] == 's') {
                    cam3D.pos.z += 1;
                }
                if (key_buffer[0] == 'a') {
                    cam3D.pos.x -= 1;
                }
                if (key_buffer[0] == 'd') {
                    cam3D.pos.x += 1;
                }
                if (key_buffer[0] == 'g') {
                    cam3D.pos.y += 1;
                }
                if (key_buffer[0] == 'b') {
                    cam3D.pos.y -= 1;
                }

                if (key_buffer[0] == 'h') {
                    cam3D.yaw -= 1;
                }
                if (key_buffer[0] == 'l') {
                    cam3D.yaw += 1;
                }
                if (key_buffer[0] == 'k') {
                    cam3D.pitch += 1;
                }
                if (key_buffer[0] == 'j') {
                    cam3D.pitch -= 1;
                }
                if (cam3D.pitch > 89.0f)  {
                    cam3D.pitch = 89.0f;
                }
                if (cam3D.pitch < -89.0f) {
                    cam3D.pitch = -89.0f;
                }
                vec3f front;
                front.x = cos(math_rad(cam3D.yaw)) * cos(math_rad(cam3D.pitch));
                front.y = sin(math_rad(cam3D.pitch));
                front.z = sin(math_rad(cam3D.yaw)) * cos(math_rad(cam3D.pitch));
                cam3D.front = vec3f_norm(front);
            }
        }

        clear_background((r_rgba){0, 0, 0, 255});

        mat4f_look_at(&view, cam3D.pos, VEC3F(cam3D.pos.x + cam3D.front.x, cam3D.pos.y + cam3D.front.y, cam3D.pos.z + cam3D.front.z), VEC3F(0, 1, 0));
        mat4f view_projection;
        mat4f_mul(&perspective, &view, &view_projection);
        frustum_update(&frustum, &view_projection);

        // Shader "uniform"
        shader_uniform_set_view_pos(cam3D.pos);

        rotation += 1;
        for (int i = 0; i < N; ++i) {
            position = positions[i];
            if (is_sphere_visible(&frustum, position, radius)) {
                draw_arrays(&s, cube_vertices, 36);
                // draw_arrays(&s, sphere_vertices, LAT_BANDS * LONG_BANDS * 6, RENDER_TRIANGLE);
            }
        }

        for (int y = 0; y < cplt_get_screen_height(); ++y) {
            for (int x = 0; x < cplt_get_screen_width(); ++x) {
                int i = (y * cplt_get_screen_width() + x) * 4;
                cplt_putc(x, y, ' ', RGB(255, 255, 255), RGB(pixels[i], pixels[i + 1], pixels[i + 2]));
            }
        }
        cplt_refresh();
    }
    cplt_end();
    cleanup_renderer();

    // Destroy renderer texture
    renderer_destroy_texture(&texture);
}
