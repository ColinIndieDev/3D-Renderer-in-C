#pragma once

#include <renderer.h>
#include <cpstd/mathplus.h>
#include <stb_image.h>

#include "model.h"

r_rgba fragment_shader(r_vec3 frag_pos, r_vec3 normal, r_vec3 uv);
r_rgba fragment_shader_model(r_vec3 frag_pos, r_vec3 normal, r_vec3 uv);
void vertex_shader(float *out, float *vertices, r_uint64 vertex_id);

void vertex_shader_cubemap(float out[10], float *vertices, r_uint64 vertex_id);
r_rgba fragment_shader_cubemap(r_vec3 frag_pos, r_vec3 normal, r_vec3 uv);

r_shader s = {
    fragment_shader,
    vertex_shader
};

r_shader s2 = {
    fragment_shader_model,
    vertex_shader
};

r_shader s_cubemap = {
    fragment_shader_cubemap,
    vertex_shader_cubemap
};

float rotation = 0.0f;
vec3f position;
mat4f perspective;
mat4f view;
mat4f view_cubemap;

// "Uniforms"
r_texture texture;
r_cubemap cubemap;
int twidth, theight, tchannels;
r_rgba light_color = (r_rgba){1.0f, 0.7f, 0.7f, 1.0f};
vec3f light_direction = VEC3F(1.0f, -2.0f, 1.0f);
vec3f view_pos;

void shader_uniform_set_view_pos(vec3f pos) {
    view_pos = pos;
}

vec3f reflect(vec3f i, vec3f n) {
    return VEC3F(
        .x = i.x - 2.0f * n.x * (i.x * n.x + i.y * n.y + i.z * n.z),
        .y = i.y - 2.0f * n.y * (i.x * n.x + i.y * n.y + i.z * n.z),
        .z = i.z - 2.0f * n.z * (i.x * n.x + i.y * n.y + i.z * n.z)
    );
}

r_rgba fragment_shader_model(r_vec3 frag_pos, r_vec3 normal, r_vec3 uv) {
    float cr = 1.0f;
    float cg = 1.0f;
    float cb = 1.0f;

    float ambient_strength = 0.1f;
    r_rgba ambient = (r_rgba){ambient_strength * light_color.r, 
                              ambient_strength * light_color.g,
                              ambient_strength * light_color.b,
                              1};

    vec3f norm = vec3f_norm(*(vec3f *)&normal);
    vec3f light_dir = vec3f_norm(VEC3F(-light_direction.x, -light_direction.y, -light_direction.z));

    float diff = math_max(vec3f_dot(norm, light_dir), 0.0f);
    r_rgba diffuse = (r_rgba){diff * light_color.r, diff * light_color.g, diff * light_color.b, 1.0f};
    
    float specular_strength = 0.5f;
    vec3f view_dir = vec3f_norm(vec3f_sub(view_pos, *(vec3f *)&frag_pos));
    vec3f reflect_dir = reflect(VEC3F(-light_dir.x, -light_dir.y, -light_dir.z), norm);

    float spec = pow(math_max(vec3f_dot(view_dir, reflect_dir), 0.0f), 32.0f);
    r_rgba specular = (r_rgba){specular_strength * spec * light_color.r, 
                               specular_strength * spec * light_color.g,
                               specular_strength * spec * light_color.b,
                               1.0f};  

    float final_r = fminf(cr * (ambient.r + diffuse.r + specular.r), 1.0f);
    float final_g = fminf(cg * (ambient.g + diffuse.g + specular.g), 1.0f);
    float final_b = fminf(cb * (ambient.b + diffuse.b + specular.b), 1.0f);

    return (r_rgba){final_r, final_g, final_b, 1.0f};
}

r_rgba fragment_shader(r_vec3 frag_pos, r_vec3 normal, r_vec3 uv) {
    float cr = renderer_texture_get(&texture, (r_vec2){uv.u, uv.v}).r;
    float cg = renderer_texture_get(&texture, (r_vec2){uv.u, uv.v}).g;
    float cb = renderer_texture_get(&texture, (r_vec2){uv.u, uv.v}).b;

    float ambient_strength = 0.1f;
    r_rgba ambient = (r_rgba){ambient_strength * light_color.r, 
                              ambient_strength * light_color.g,
                              ambient_strength * light_color.b,
                              1};

    vec3f norm = vec3f_norm(*(vec3f *)&normal);
    vec3f light_dir = vec3f_norm(VEC3F(-light_direction.x, -light_direction.y, -light_direction.z));

    float diff = math_max(vec3f_dot(norm, light_dir), 0.0f);
    r_rgba diffuse = (r_rgba){diff * light_color.r, diff * light_color.g, diff * light_color.b, 1.0f};
    
    float specular_strength = 0.5f;
    vec3f view_dir = vec3f_norm(vec3f_sub(view_pos, *(vec3f *)&frag_pos));
    vec3f reflect_dir = reflect(VEC3F(-light_dir.x, -light_dir.y, -light_dir.z), norm);

    float spec = pow(math_max(vec3f_dot(view_dir, reflect_dir), 0.0f), 32.0f);
    r_rgba specular = (r_rgba){specular_strength * spec * light_color.r, 
                               specular_strength * spec * light_color.g,
                               specular_strength * spec * light_color.b,
                               1.0f};  

    float final_r = fminf(cr * (ambient.r + diffuse.r + specular.r), 1.0f);
    float final_g = fminf(cg * (ambient.g + diffuse.g + specular.g), 1.0f);
    float final_b = fminf(cb * (ambient.b + diffuse.b + specular.b), 1.0f);

    return (r_rgba){final_r, final_g, final_b, 1.0f};
}

void vertex_shader(float out[10], float *vertices, r_uint64 vertex_id) {
    float v[8];
    memcpy(v, vertices + vertex_id * 8, sizeof(float) * 8);
    vec3f vertex = VEC3F(v[0], v[1], v[2]);
    vec3f normal = VEC3F(v[3], v[4], v[5]);
    vec2f uv = VEC2F(v[6], v[7]);

    mat4f model;
    mat4f_identity(&model);
    mat4f_translate(&model, position);
    mat4f_rotate(&model, math_rad(rotation), VEC3F(0.0f, 1.0f, 0.0f));
    mat4f_rotate(&model, math_rad(rotation * 0.7f),VEC3F(1.0f, 0.0f, 0.0f));

    mat4f normal_matrix;
    mat4f_identity(&normal_matrix);
    mat4f_rotate(&normal_matrix, math_rad(rotation), VEC3F(0.0f, 1.0f, 0.0f));
    mat4f_rotate(&normal_matrix, math_rad(rotation * 0.7f), VEC3F(1.0f, 0.0f, 0.0f));

    mat4f view_projection;
    mat4f_mul(&perspective, &view, &view_projection);
    mat4f mvp;
    mat4f_mul(&view_projection, &model, &mvp);

    vec4f object = (vec4f){vertex.x, vertex.y, vertex.z, 1.0f};
    vec4f result = mat4f_mul_vec4f(&mvp, object);

    vec4f normal4 = (vec4f){normal.x, normal.y, normal.z, 0.0f};
    vec4f rotated_normal = mat4f_mul_vec4f(&normal_matrix, normal4);

    out[0] = result.x;
    out[1] = result.y;
    out[2] = result.z;
    out[3] = result.w;
    out[4] = rotated_normal.x;
    out[5] = rotated_normal.y;
    out[6] = rotated_normal.z;
    out[7] = uv.x;
    out[8] = uv.y;
}

void vertex_shader_cubemap(float out[10], float *vertices, r_uint64 vertex_id) {
    float v[3];
    memcpy(v, (float *)vertices + vertex_id * 3, sizeof(float) * 3);
    vec3f vertex = VEC3F(v[0], v[1], v[2]);

    mat4f model;
    mat4f_identity(&model);

    mat4f view_projection;
    mat4f_mul(&perspective, &view_cubemap, &view_projection);
    mat4f mvp;
    mat4f_mul(&view_projection, &model, &mvp);

    vec4f object = (vec4f){vertex.x, vertex.y, vertex.z, 1.0f};
    vec4f result = mat4f_mul_vec4f(&mvp, object);

    out[0] = result.x;
    out[1] = result.y;
    out[2] = result.w;
    out[3] = result.w;
    out[4] = 0.0f;
    out[5] = 0.0f;
    out[6] = 0.0f;
    out[7] = v[0];
    out[8] = v[1];
    out[9] = v[2];
}

r_rgba fragment_shader_cubemap(r_vec3 frag_pos, r_vec3 normal, r_vec3 uv) {
    r_rgba color = renderer_cubemap_get(&cubemap, uv);
    return (r_rgba){color.r, color.g * 0.5f, color.b * 0.5f, 1.0f};
}
