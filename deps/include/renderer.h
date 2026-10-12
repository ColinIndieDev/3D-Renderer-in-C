#pragma once

#include <stdbool.h>
#include <float.h>

typedef unsigned int r_uint32;
typedef unsigned long long r_uint64;
typedef unsigned char r_uint8;

typedef struct {
    float r, g, b, a;
} r_rgba;

typedef union {
    struct {
        float x, y;
    };
    struct {
        float u, v;
    };
} r_vec2;

typedef union {
    struct {
        float x, y, z;
    };
    struct {
        float u, v, w;
    };
} r_vec3;

typedef struct {
    float x, y, z, w;
} r_vec4;

typedef struct {
    float x, y, z, w, nx, ny, nz, u, v;
} r_vec9;

#define renderer_min(x, y) (x) < (y) ? (x) : (y)
#define renderer_max(x, y) (x) > (y) ? (x) : (y)

typedef struct {
    r_uint8 *pixels;
    int width, height, channels;
} r_texture;

enum {
    FACE_POSITIVE_X = 0,
    FACE_NEGATIVE_X,
    FACE_POSITIVE_Y,
    FACE_NEGATIVE_Y,
    FACE_POSITIVE_Z,
    FACE_NEGATIVE_Z
};

typedef struct {
    r_texture faces[6];
} r_cubemap;

typedef struct {
    r_rgba (*fragment)(r_vec3 frag_pos, r_vec3 normal, r_vec3 uv);
    void (*vertex)(float out[10], float *vertices, r_uint64 vertex_id);
} r_shader;

enum {
    RENDERER_TRIANGLE_ARRAY,
    RENDERER_TRIANGLE_ELEMENT
};

typedef struct {
    r_shader *s;
    float *vertices;
    r_uint32 *indices;
    float *out;
    r_uint64 begin;
    r_uint64 end;
    int render_mode;
} vertex_shader_job_t;

enum {
    DEPTH_FUNC_LESS = 0,
    DEPTH_FUNC_LEQUAL,
    DEPTH_FUNC_GREATER,
    DEPTH_FUNC_GEQUAL
};

r_texture renderer_create_texture(const char *path);
void renderer_destroy_texture(r_texture *t);
r_rgba renderer_texture_get(r_texture *t, r_vec2 uv);

r_cubemap renderer_create_cubemap(const char *path[6]);
void renderer_destroy_cubemap(r_cubemap *m);
r_rgba renderer_cubemap_get(r_cubemap *m, r_vec3 dir);

void renderer_init(r_uint32 width, r_uint32 height);
void renderer_resize(r_uint32 width, r_uint32 height);
void renderer_destroy();
void clear_background(r_rgba color);
void renderer_draw_arrays(r_shader *s, void *vertices, r_uint64 n);
void renderer_draw_elements(r_shader *s, void *vertices, r_uint32 *indices, r_uint64 ni);
r_uint32 renderer_get_screen_width();
r_uint32 renderer_get_screen_height();
r_uint8 *renderer_get_pixel_buffer();

void renderer_depth_mask(bool set);
void renderer_depth_func(int mode);

// #define RENDERER_IMPL
#ifdef RENDERER_IMPL

#include <stb_image.h>

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

r_uint8 *_pixels = NULL;
float *_depth_buffer = NULL;
r_uint32 _screen_width, _screen_height;
bool _depth_mask = true;
int _depth_func = DEPTH_FUNC_LESS;

#define THREAD_COUNT 12
pthread_t _threads[THREAD_COUNT];
vertex_shader_job_t _jobs[THREAD_COUNT];
pthread_mutex_t _vertex_shader_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t _vertex_shader_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t _vertex_shader_done_cond = PTHREAD_COND_INITIALIZER;
r_uint32 _vertex_shader_generation = 0;
int _vertex_shader_completed = 0;
bool _vertex_shader_shutdown = false;

r_texture renderer_create_texture(const char *path) {
    int width, height, channels;
    r_uint8 *pixels = stbi_load(path, &width, &height, &channels, 0);
    return (r_texture){pixels, width, height, channels};
}

void renderer_destroy_texture(r_texture *t) {
    if (!t->pixels) {
        return;
    }
    stbi_image_free(t->pixels);
}

r_rgba renderer_texture_get(r_texture *t, r_vec2 uv) {
    if (uv.u < 0.0f) {
        uv.u = 0.0f;
    }
    if (uv.u > 1.0f) {
        uv.u = 1.0f;
    }
    if (uv.v < 0.0f) {
        uv.v = 0.0f;
    }
    if (uv.v > 1.0f) {
        uv.v = 1.0f;
    }

    int tx = uv.u * t->width;
    int ty = uv.v * t->height;

    if (tx >= t->width) {
        tx = t->width - 1;
    }
    if (ty >= t->height) {
        ty = t->height - 1;
    }
    if (tx < 0) {
        tx = 0;
    }
    if (ty < 0) {
        ty = 0;
    }

    float cr = t->pixels[(ty * t->width + tx) * t->channels]     / 255.0f;
    float cg = t->pixels[(ty * t->width + tx) * t->channels + 1] / 255.0f;
    float cb = t->pixels[(ty * t->width + tx) * t->channels + 2] / 255.0f;
    float ca = 1.0f;
    if (t->channels == 4) {
        ca = t->pixels[(ty * t->width + tx) * t->channels + 3] / 255.0f;        
    }
    return (r_rgba){cr, cg, cb, ca};
}

r_cubemap renderer_create_cubemap(const char *path[6]) {
    r_cubemap map;
    for (int i = 0; i < 6; ++i) {
        map.faces[i] = renderer_create_texture(path[i]); 
    }
    return map;
}

void renderer_destroy_cubemap(r_cubemap *m) {
    for (int i = 0; i < 6; ++i) {
        renderer_destroy_texture(&m->faces[i]);
    }
}

r_rgba renderer_cubemap_get(r_cubemap *m, r_vec3 dir) {
    float len = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    if (len > 1e-20f) {
        float inv_len = 1.0f / sqrtf(len);

        dir.x *= inv_len;
        dir.y *= inv_len;
        dir.z *= inv_len;
    }

    float abs_x = fabsf(dir.x);
    float abs_y = fabsf(dir.y);
    float abs_z = fabsf(dir.z);

    r_vec2 uv;
    int face = 0;

    if (abs_x >= abs_y && abs_x >= abs_z) {
        if (dir.x > 0) {
            face = FACE_POSITIVE_X;
            uv.u = 0.5f - 0.5f * dir.z / abs_x;
            uv.v = 0.5f - 0.5f * dir.y / abs_x;
        } else {
            face = FACE_NEGATIVE_X;
            uv.u = 0.5f + 0.5f * dir.z / abs_x;
            uv.v = 0.5f - 0.5f * dir.y / abs_x;
        }
    } else if (abs_y >= abs_x && abs_y >= abs_z) {
        if (dir.y > 0) {
            face = FACE_POSITIVE_Y;
            uv.u = 0.5f + 0.5f * dir.x / abs_y;
            uv.v = 0.5f - 0.5f * dir.z / abs_y;
        } else {
            face = FACE_NEGATIVE_Y;
            uv.u = 0.5f + 0.5f * dir.x / abs_y;
            uv.v = 0.5f + 0.5f * dir.z / abs_y;
        }
    } else {
        if (dir.z > 0) {
            face = FACE_POSITIVE_Z;
            uv.u = 0.5f - 0.5f * dir.x / abs_z;
            uv.v = 0.5f - 0.5f * dir.y / abs_z;
        } else {
            face = FACE_NEGATIVE_Z;
            uv.u = 0.5f + 0.5f * dir.x / abs_z;
            uv.v = 0.5f - 0.5f * dir.y / abs_z;
        }
    }

    return renderer_texture_get(&m->faces[face], uv);
}

float signed_triangle_area(r_vec3 a, r_vec3 b, r_vec3 c) {
    return 0.5f * ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
}

void *vertex_shader_worker(void *args);
void renderer_init(r_uint32 width, r_uint32 height) {
    _screen_width = width;
    _screen_height = height;
    _pixels = calloc(width * height * 4, 1);
    _depth_buffer = malloc(width * height * sizeof(float));
    for (int i = 0; i < width * height; ++i) {
        _depth_buffer[i] = 1.0f;
    }
    for (int i = 0; i < THREAD_COUNT; ++i) {
        pthread_create(&_threads[i], NULL, vertex_shader_worker, &_jobs[i]);
    }
}

void renderer_resize(r_uint32 width, r_uint32 height) {
    _screen_width = width;
    _screen_height = height;
    free(_pixels);
    _pixels = calloc(width * height * 4, 1);
    free(_depth_buffer);
    _depth_buffer = malloc(width * height * sizeof(float));
    for (int i = 0; i < width * height; ++i) {
        _depth_buffer[i] = 1.0f;
    }
}

void renderer_cleanup() {
    free(_pixels);
    free(_depth_buffer);
    for (int i = 0; i < THREAD_COUNT; ++i) {
        pthread_cancel(_threads[i]);
    }
    for (int i = 0; i < THREAD_COUNT; ++i) {
        pthread_join(_threads[i], NULL);
    }
}

r_uint32 renderer_get_screen_width() {
    return _screen_width;
}

r_uint32 renderer_get_screen_height() {
    return _screen_height;
}

r_uint8 *renderer_get_pixel_buffer() {
    return _pixels;
}

void renderer_depth_mask(bool set) {
    _depth_mask = set;
}

void renderer_depth_func(int mode) {
    _depth_func = mode;
}

void clear_background(r_rgba color) {
    for (r_uint32 i = 0; i < _screen_width * _screen_height; ++i) {
        _depth_buffer[i] = 1.0f;
    }
    for (r_uint32 i = 0; i < _screen_width * _screen_height * 4; i += 4) {
        _pixels[i] = color.r;
        _pixels[i + 1] = color.g;
        _pixels[i + 2] = color.b;
        _pixels[i + 3] = color.a;
    }
}

void draw_pixel(int x, int y, r_rgba color) {
    if (x >= _screen_width || y >= _screen_height || x < 0 || y < 0) {
        return;
    }
    r_uint32 i = (y * _screen_width + x) * 4;
    _pixels[i] = color.r;
    _pixels[i + 1] = color.g;
    _pixels[i + 2] = color.b;
    _pixels[i + 3] = color.a;
}

r_vec3 norm_to_screen(r_vec3 norm) {
    float x = (norm.x + 1.0f) * 0.5f * (float)_screen_width;
    float y = (1.0f - (norm.y + 1.0f) * 0.5f) * (float)_screen_height;
    return (r_vec3){x, y, norm.z};
}

r_rgba blend_over(r_rgba s, r_rgba d) {
    r_rgba o;
    float sa = s.a / 255.0f;
    float da = d.a / 255.0f;
    o.r = s.r * sa + d.r * da * (1.0f - sa);
    o.g = s.g * sa + d.g * da * (1.0f - sa);
    o.b = s.b * sa + d.b * da * (1.0f - sa);
    o.a = 255.0f * (sa + da * (1.0f - sa));
    return o;
}

#define CMP_EPSILON 1e-6f
bool float_equal(float a, float b) {
    return fabsf(a - b) < CMP_EPSILON;
}

bool float_greater(float a, float b) {
    return (a - b) > CMP_EPSILON * fmaxf(fabsf(a), fabsf(b));
}

bool float_less(float a, float b) {
    return (b - a) > CMP_EPSILON * fmaxf(fabsf(a), fabsf(b));
}

bool float_gequal(float a, float b) {
    return a > b - CMP_EPSILON;
}

bool float_lequal(float a, float b) {
    return a < b + CMP_EPSILON;
}

bool depth_func(float depth, float z) {
    switch (_depth_func) {
    case DEPTH_FUNC_LEQUAL:
        return float_lequal(z, depth);
    case DEPTH_FUNC_GREATER:
        return float_greater(z, depth);
    case DEPTH_FUNC_GEQUAL:
        return float_gequal(z, depth);
    default:
        return float_less(z, depth);
    }
    return false;
}

// Vertex layout: x, y, z, w, nx, ny, nz, tu, tv, tw (= 10 Members)
void rasterize(r_shader *s, float a_norm[10], float b_norm[10], float c_norm[10]) {
    if (a_norm[3] <= 0.0f || b_norm[3] <= 0.0f || c_norm[3] <= 0.0f) {
        return;
    }
    r_vec3 a_ndc = (r_vec3){a_norm[0] / a_norm[3], a_norm[1] / a_norm[3], a_norm[2] / a_norm[3]};
    r_vec3 b_ndc = (r_vec3){b_norm[0] / b_norm[3], b_norm[1] / b_norm[3], b_norm[2] / b_norm[3]};
    r_vec3 c_ndc = (r_vec3){c_norm[0] / c_norm[3], c_norm[1] / c_norm[3], c_norm[2] / c_norm[3]};
    r_vec3 a = norm_to_screen((r_vec3){a_ndc.x, a_ndc.y, a_ndc.z});
    r_vec3 b = norm_to_screen((r_vec3){b_ndc.x, b_ndc.y, b_ndc.z});
    r_vec3 c = norm_to_screen((r_vec3){c_ndc.x, c_ndc.y, c_ndc.z});

    float wa = 1.0f / a_norm[3];
    float wb = 1.0f / b_norm[3];
    float wc = 1.0f / c_norm[3];
    float uwa = a_norm[7] * wa;
    float vwa = a_norm[8] * wa;
    float twa = a_norm[9] * wa;
    float uwb = b_norm[7] * wb;
    float vwb = b_norm[8] * wb;
    float twb = b_norm[9] * wb;
    float uwc = c_norm[7] * wc;
    float vwc = c_norm[8] * wc;
    float twc = c_norm[9] * wc;

    int min_x = (int)renderer_min(renderer_min(a.x, b.x), c.x);
    int max_x = (int)renderer_max(renderer_max(a.x, b.x), c.x);
    int min_y = (int)renderer_min(renderer_min(a.y, b.y), c.y);
    int max_y = (int)renderer_max(renderer_max(a.y, b.y), c.y);
    min_x = renderer_max(0, renderer_min(min_x, (int)_screen_width - 1));
    max_x = renderer_max(0, renderer_min(max_x, (int)_screen_width - 1));
    min_y = renderer_max(0, renderer_min(min_y, (int)_screen_height - 1));
    max_y = renderer_max(0, renderer_min(max_y, (int)_screen_height - 1));

    float total_area = signed_triangle_area(a, b, c);
    if (fabsf(total_area) < 1e-8f) {
        return;
    }

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            float px = (float)x + 0.5f;
            float py = (float)y + 0.5f;

            float alpha = signed_triangle_area((r_vec3){px, py, 0}, b, c) / total_area;
            float beta = signed_triangle_area((r_vec3){px, py, 0}, c, a) / total_area;
            float gamma = signed_triangle_area((r_vec3){px, py, 0}, a, b) / total_area;

            float z = a.z * alpha + b.z * beta + c.z * gamma;

            if (alpha < -1e-6f ||
                beta  < -1e-6f ||
                gamma < -1e-6f) {
                continue;
            }

            if (depth_func(_depth_buffer[y * _screen_width + x], z)) {
                float interp_one_over_w = wa * alpha + wb * beta + wc * gamma;
                float interp_uw = uwa * alpha + uwb * beta + uwc * gamma;
                float interp_vw = vwa * alpha + vwb * beta + vwc * gamma;
                float interp_ww = twa * alpha + twb * beta + twc * gamma;

                float correct_w = 1.0f / interp_one_over_w;
                float final_u = interp_uw * correct_w;
                float final_v = interp_vw * correct_w;
                float final_w = interp_ww * correct_w;

                float nx_w = (a_norm[4] * wa) * alpha + (b_norm[4] * wb) * beta + (c_norm[4] * wc) * gamma;
                float ny_w = (a_norm[5] * wa) * alpha + (b_norm[5] * wb) * beta + (c_norm[5] * wc) * gamma;
                float nz_w = (a_norm[6] * wa) * alpha + (b_norm[6] * wb) * beta + (c_norm[6] * wc) * gamma;
                r_vec3 interpolated_normal = (r_vec3){nx_w * correct_w, ny_w * correct_w, nz_w * correct_w};

                r_rgba col_norm = s->fragment((r_vec3){x, y, z}, interpolated_normal, (r_vec3){final_u, final_v, final_w});
                r_rgba col = (r_rgba){col_norm.r * 255, col_norm.g * 255, col_norm.b * 255, col_norm.a * 255};
                r_rgba pixel;
                r_uint32 i = (y * _screen_width + x) * 4;
                pixel.r = (float)_pixels[i];
                pixel.g = (float)_pixels[i + 1];
                pixel.b = (float)_pixels[i + 2];
                pixel.a = (float)_pixels[i + 3];
                col = blend_over(col, pixel);
                draw_pixel(x, y, col);
                if (_depth_mask) {
                    _depth_buffer[y * _screen_width + x] = z;
                }
            }
        }
    }
}

#define CLIP_EPSILON 1e-6f

float clip_dist(const float v[10], int plane) {
    switch (plane) {
    case 0: 
        return v[0] + v[3]; // Left
    case 1: 
        return v[3] - v[0]; // Right
    case 2: 
        return v[1] + v[3]; // Bottom
    case 3: 
        return v[3] - v[1]; // Top
    case 4: 
        return v[2] + v[3]; // Near
    case 5: 
        return v[3] - v[2]; // Far
    default: 
        return -1.0f;
    }
}

void rasterize_clipped(r_shader *s, float a[10], float b[10], float c[10]) {
    float polygon_a[12][10];
    float polygon_b[12][10];

    memcpy(polygon_a[0], a, sizeof(float) * 10);
    memcpy(polygon_a[1], b, sizeof(float) * 10);
    memcpy(polygon_a[2], c, sizeof(float) * 10);

    int count = 3;

    for (int plane = 0; plane < 6; ++plane) {
        if (count == 0) {
            return;
        }

        int new_count = 0;

        for (int i = 0; i < count; ++i) {
            const float *current = polygon_a[i];
            const float *next = polygon_a[(i + 1) % count];

            float d_current = clip_dist(current, plane);
            float d_next = clip_dist(next, plane);

            bool current_inside = d_current >= 0.0f;
            bool next_inside = d_next >= 0.0f;

            if (current_inside && next_inside) {
                memcpy(polygon_b[new_count++], next, sizeof(float) * 10);
            } else if (current_inside && !next_inside) {
                float t = d_current / (d_current - d_next);
                for (int k = 0; k < 10; ++k) {
                    polygon_b[new_count][k] = current[k] + t * (next[k] - current[k]);
                }
                new_count++;
            } else if (!current_inside && next_inside) {
                float t = d_current / (d_current - d_next);

                for (int k = 0; k < 10; ++k) {
                    polygon_b[new_count][k] = current[k] + t * (next[k] - current[k]);
                }
                new_count++;

                memcpy(polygon_b[new_count++], next,
                       sizeof(float) * 10);
            }
        }

        if (new_count > 12) {
            return;
        }
        count = new_count;

        for (int i = 0; i < count; ++i) {
            memcpy(polygon_a[i], polygon_b[i], sizeof(float) * 10);
        }
    }

    if (count < 3) {
        return;
    }

    for (int i = 1; i < count - 1; ++i) {
        rasterize(s, polygon_a[0], polygon_a[i], polygon_a[i + 1]);
    }
}

void *vertex_shader_worker(void *args) {
    vertex_shader_job_t *job = args;
    r_uint32 seen_generation = 0;

    while (true) {
        pthread_mutex_lock(&_vertex_shader_mutex);

        while (!_vertex_shader_shutdown && seen_generation == _vertex_shader_generation) {
            pthread_cond_wait(&_vertex_shader_cond, &_vertex_shader_mutex);
        }

        if (_vertex_shader_shutdown) {
            pthread_mutex_unlock(&_vertex_shader_mutex);
            return NULL;
        }

        seen_generation = _vertex_shader_generation;

        pthread_mutex_unlock(&_vertex_shader_mutex);

        if (job->render_mode == RENDERER_TRIANGLE_ARRAY) {
            for (r_uint64 i = job->begin; i < job->end; i += 3) {
                job->s->vertex(&job->out[i * 10], job->vertices, i);
                job->s->vertex(&job->out[(i + 1) * 10], job->vertices, i + 1);
                job->s->vertex(&job->out[(i + 2) * 10], job->vertices, i + 2);
            }
        } else {
            for (r_uint64 i = job->begin; i < job->end; i += 3) {
                job->s->vertex(&job->out[i * 10], job->vertices, job->indices[i]);
                job->s->vertex(&job->out[(i + 1) * 10], job->vertices, job->indices[i + 1]);
                job->s->vertex(&job->out[(i + 2) * 10], job->vertices, job->indices[i + 2]);
            }
        }

        pthread_mutex_lock(&_vertex_shader_mutex);

        _vertex_shader_completed++;

        if (_vertex_shader_completed == THREAD_COUNT) {
            pthread_cond_signal(&_vertex_shader_done_cond);
        }

        pthread_mutex_unlock(&_vertex_shader_mutex);
    }
}

void dispatch_vertex_jobs(r_shader *s, float *vertices, r_uint32 *indices, float *out, r_uint64 count, int render_mode) {
    r_uint64 triangle_count = count / 3;
    r_uint64 chunk = triangle_count / THREAD_COUNT;

    pthread_mutex_lock(&_vertex_shader_mutex);

    _vertex_shader_completed = 0;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        r_uint64 first_triangle = i * chunk;
        r_uint64 last_triangle = (i == THREAD_COUNT - 1) ? triangle_count : (i + 1) * chunk;

        _jobs[i].s = s;
        _jobs[i].vertices = vertices;
        _jobs[i].indices = indices;
        _jobs[i].out = out;
        _jobs[i].render_mode = render_mode;
        _jobs[i].begin = first_triangle * 3;
        _jobs[i].end = last_triangle * 3;
    }

    ++_vertex_shader_generation;
    pthread_cond_broadcast(&_vertex_shader_cond);

    while (_vertex_shader_completed < THREAD_COUNT) {
        pthread_cond_wait(&_vertex_shader_done_cond, &_vertex_shader_mutex);
    }

    pthread_mutex_unlock(&_vertex_shader_mutex);
}

void renderer_draw_arrays(r_shader *s, void *vertices, r_uint64 n) {
    if (n % 3 != 0 || n == 0) {
        return;
    }

    float *out = calloc(n * 10, sizeof(float));
    dispatch_vertex_jobs(s, vertices, NULL, out, n, RENDERER_TRIANGLE_ARRAY);

    for (int i = 0; i < n; i += 3) {
        float *a = &out[i * 10];
        float *b = &out[(i + 1) * 10];
        float *c = &out[(i + 2) * 10];
        rasterize_clipped(s, a, b, c);
    }

    free(out);
}

void renderer_draw_elements(r_shader *s, void *vertices, r_uint32 *indices, r_uint64 ni) {
    if (ni % 3 != 0 || ni == 0) {
        return;
    }

    float *out = calloc(ni * 10, sizeof(float));
    dispatch_vertex_jobs(s, vertices, indices, out, ni, RENDERER_TRIANGLE_ELEMENT);

    for (int i = 0; i < ni; i += 3) {
        float *a = &out[i * 10];
        float *b = &out[(i + 1) * 10];
        float *c = &out[(i + 2) * 10];
        rasterize_clipped(s, a, b, c);
    }

    free(out);
}

#endif
