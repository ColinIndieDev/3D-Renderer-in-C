#pragma once

#include <stdint.h>
#include <cpstd/mathplus.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <renderer.h>

typedef struct {
    vec3f pos;
    vec3f normal;
    vec2f uv;
} vertex_t;

typedef struct {
    uint8_t *pixels;
    char *type;
} texture_t;

typedef struct {
    vertex_t *vertices;
    uint32_t *indices;
    texture_t *textures;
} mesh_t;

void mesh_init(mesh_t *m);
void mesh_draw(mesh_t *m, r_shader *s);
void mesh_destroy(mesh_t *m);

typedef struct {
    mesh_t *meshes;
    char *dir;
} model_t;

void model_init(model_t *m);
void model_draw(model_t *m, r_shader *s);
void model_load(model_t *m, const char *path);
void model_destroy(model_t *m);
