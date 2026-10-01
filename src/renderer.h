#pragma once

#include <float.h>

typedef unsigned int r_uint32;
typedef unsigned long long r_uint64;
typedef unsigned char r_uint8;

typedef struct {
    float r, g, b, a;
} r_rgba;

typedef struct {
    float x, y, z;
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

typedef struct {
    r_rgba (*fragment)(r_vec3 frag_pos, r_vec3 normal, float u, float v);
    r_vec9 (*vertex)(void *vertices, r_uint64 vertex_id);
} r_shader;

#define RENDERER_IMPL
#ifdef RENDERER_IMPL

#include <stdlib.h>
#include <stb_image.h>
#include <stdio.h>

r_uint8 *pixels = NULL;
float *depth_buffer = NULL;
r_uint32 screen_width, screen_height;

r_texture renderer_create_texture(const char *path) {
    int width, height, channels;
    r_uint8 *pixels = stbi_load(path, &width, &height, &channels, 0);
    return (r_texture){pixels, width, height, channels};
}

r_rgba renderer_texture_get(r_texture *t, float u, float v) {
    int tx = u * t->width;
    int ty = v * t->height;
    float cr = t->pixels[(ty * t->width + tx) * t->channels]     / 255.0f;
    float cg = t->pixels[(ty * t->width + tx) * t->channels + 1] / 255.0f;
    float cb = t->pixels[(ty * t->width + tx) * t->channels + 2] / 255.0f;
    float ca = 1.0f;
    if (t->channels == 4) {
        ca = t->pixels[(ty * t->width + tx) * t->channels + 3] / 255.0f;        
    }
    return (r_rgba){cr, cg, cb, ca};
}

void renderer_destroy_texture(r_texture *t) {
    if (!t->pixels) {
        return;
    }
    stbi_image_free(t->pixels);
}

float signed_triangle_area(r_vec3 a, r_vec3 b, r_vec3 c) {
    return 0.5f * ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
}

void init_renderer(r_uint32 width, r_uint32 height) {
    screen_width = width;
    screen_height = height;
    pixels = calloc(width * height * 4, 1);
    depth_buffer = malloc(width * height * sizeof(float));
    for (int i = 0; i < width * height; ++i) {
        depth_buffer[i] = FLT_MAX;
    }
}

void resize_renderer(r_uint32 width, r_uint32 height) {
    screen_width = width;
    screen_height = height;
    free(pixels);
    pixels = calloc(width * height * 4, 1);
    free(depth_buffer);
    depth_buffer = malloc(width * height * sizeof(float));
    for (int i = 0; i < width * height; ++i) {
        depth_buffer[i] = FLT_MAX;
    }
}

void cleanup_renderer() {
    free(pixels);
    free(depth_buffer);
}

void clear_background(r_rgba color) {
    for (r_uint32 i = 0; i < screen_width * screen_height; ++i) {
        depth_buffer[i] = FLT_MAX;
    }
    for (r_uint32 i = 0; i < screen_width * screen_height * 4; i += 4) {
        pixels[i] = color.r;
        pixels[i + 1] = color.g;
        pixels[i + 2] = color.b;
        pixels[i + 3] = color.a;
    }
}

void draw_pixel(int x, int y, r_rgba color) {
    if (x >= screen_width || y >= screen_height || x < 0 || y < 0) {
        return;
    }
    r_uint32 i = (y * screen_width + x) * 4;
    pixels[i] = color.r;
    pixels[i + 1] = color.g;
    pixels[i + 2] = color.b;
    pixels[i + 3] = color.a;
}

r_vec3 norm_to_screen(r_vec3 norm) {
    float x = (norm.x + 1.0f) * 0.5f * (float)screen_width;
    float y = (1.0f - (norm.y + 1.0f) * 0.5f) * (float)screen_height;
    return (r_vec3){x, y, norm.z};
}

void rasterize(r_shader *s, r_vec9 a_norm, r_vec9 b_norm, r_vec9 c_norm) {
    if (a_norm.w <= 0.0f || b_norm.w <= 0.0f || c_norm.w <= 0.0f) {
        return;
    }

    r_vec3 a_ndc = (r_vec3){a_norm.x / a_norm.w, a_norm.y / a_norm.w, a_norm.z / a_norm.w};
    r_vec3 b_ndc = (r_vec3){b_norm.x / b_norm.w, b_norm.y / b_norm.w, b_norm.z / b_norm.w};
    r_vec3 c_ndc = (r_vec3){c_norm.x / c_norm.w, c_norm.y / c_norm.w, c_norm.z / c_norm.w};
    r_vec3 a = norm_to_screen((r_vec3){a_ndc.x, a_ndc.y, a_ndc.z});
    r_vec3 b = norm_to_screen((r_vec3){b_ndc.x, b_ndc.y, b_ndc.z});
    r_vec3 c = norm_to_screen((r_vec3){c_ndc.x, c_ndc.y, c_ndc.z});

    float wa = 1.0f / a_norm.w;
    float wb = 1.0f / b_norm.w;
    float wc = 1.0f / c_norm.w;
    float uwa = a_norm.u * wa;
    float vwa = a_norm.v * wa;
    float uwb = b_norm.u * wb;
    float vwb = b_norm.v * wb;
    float uwc = c_norm.u * wc;
    float vwc = c_norm.v * wc;

    int min_x = (int)renderer_min(renderer_min(a.x, b.x), c.x);
    int max_x = (int)renderer_max(renderer_max(a.x, b.x), c.x);
    int min_y = (int)renderer_min(renderer_min(a.y, b.y), c.y);
    int max_y = (int)renderer_max(renderer_max(a.y, b.y), c.y);
    min_x = renderer_max(0, renderer_min(min_x, (int)screen_width - 1));
    max_x = renderer_max(0, renderer_min(max_x, (int)screen_width - 1));
    min_y = renderer_max(0, renderer_min(min_y, (int)screen_height - 1));
    max_y = renderer_max(0, renderer_min(max_y, (int)screen_height - 1));

    float total_area = signed_triangle_area(a, b, c);
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            float alpha = signed_triangle_area((r_vec3){x, y, 0}, b, c) / total_area;
            float beta = signed_triangle_area((r_vec3){x, y, 0}, c, a) / total_area;
            float gamma = signed_triangle_area((r_vec3){x, y, 0}, a, b) / total_area;
            float z = a.z * alpha + b.z * beta + c.z * gamma;
            if (alpha < 0 || beta < 0 || gamma < 0) {
                continue;
            }
            if (depth_buffer[y * screen_width + x] > z) {
                float interp_one_over_w = wa * alpha + wb * beta + wc * gamma;
                float interp_uw = uwa * alpha + uwb * beta + uwc * gamma;
                float interp_vw = vwa * alpha + vwb * beta + vwc * gamma;

                float correct_w = 1.0f / interp_one_over_w;
                float final_u = interp_uw * correct_w;
                float final_v = interp_vw * correct_w;

                float nx_w = (a_norm.nx * wa) * alpha + (b_norm.nx * wb) * beta + (c_norm.nx * wc) * gamma;
                float ny_w = (a_norm.ny * wa) * alpha + (b_norm.ny * wb) * beta + (c_norm.ny * wc) * gamma;
                float nz_w = (a_norm.nz * wa) * alpha + (b_norm.nz * wb) * beta + (c_norm.nz * wc) * gamma;
                r_vec3 interpolated_normal = (r_vec3){nx_w * correct_w, ny_w * correct_w, nz_w * correct_w};

                r_rgba col_norm = s->fragment((r_vec3){x, y, z}, interpolated_normal, final_u, final_v);
                r_rgba col = (r_rgba){col_norm.r * 255, col_norm.g * 255, col_norm.b * 255, col_norm.a * 255};
                draw_pixel(x, y, col);
                depth_buffer[y * screen_width + x] = z;
            }
        }
    }
}

void draw_arrays(r_shader *s, void *vertices, r_uint64 n) {
    if (n % 3 != 0 || n == 0) {
        return;
    }
    for (int i = 0; i < n; i += 3) {
        r_vec9 a = s->vertex(vertices, i);
        r_vec9 b = s->vertex(vertices, i + 1);
        r_vec9 c = s->vertex(vertices, i + 2);
        rasterize(s, a, b, c);
    }
}

#endif
