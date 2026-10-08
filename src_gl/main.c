#define RENDERER_IMPL
#include <renderer.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <cpstd/rand.h>
#include <cpstd/vector.h>
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "shader.h"

#define KEY_ESCAPE(b, n) ((n) == 1 && (b)[0] == 0x1b)
#define KEY_ENTER(b) ((b)[0] == 0x0d)
#define KEY_BACKSPACE(b) ((b)[0] == 0x7f || (b)[0] == 0x08)

uint32_t fb_tex, fbo, vao, vbo;
uint32_t shader;

char *shader_read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    unsigned int size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    unsigned int read = fread(buffer, 1, size, f);
    fclose(f);

    if (read != size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

void init_gl(uint32_t width, uint32_t height) {
    char *vert_code = shader_read_file("shaders/shader.vert");
    char *frag_code = shader_read_file("shaders/shader.frag");
    unsigned int vert = 0;
    unsigned int frag = 0;
    vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, (const GLchar *const *)&vert_code, NULL);
    glCompileShader(vert);
    frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, (const GLchar *const *)&frag_code, NULL);
    glCompileShader(frag);
    shader = glCreateProgram();
    glAttachShader(shader, vert);
    glAttachShader(shader, frag);
    glLinkProgram(shader);
    free(vert_code);
    free(frag_code);
    glDeleteShader(vert);
    glDeleteShader(frag);

    glGenTextures(1, &fb_tex);
    glBindTexture(GL_TEXTURE_2D, fb_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float vertices[] = {
        -1.0f,  1.0f,  0.0f, 0.0f,   
         1.0f,  1.0f,  1.0f, 0.0f,   
         1.0f, -1.0f,  1.0f, 1.0f,   
        -1.0f, -1.0f,  0.0f, 1.0f,   
        -1.0f,  1.0f,  0.0f, 0.0f,   
         1.0f, -1.0f,  1.0f, 1.0f    
    };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), NULL);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void render_gl() {
    glBindTexture(GL_TEXTURE_2D, fb_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen_width, screen_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glViewport(0, 0, screen_width, screen_height);
    glBindVertexArray(vao);
    glUseProgram(shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fb_tex);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void resize_gl(uint32_t width, uint32_t height) {
    glBindTexture(GL_TEXTURE_2D, fb_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glViewport(0, 0, width, height);
}

GLFWwindow *window = NULL;

void framebuffer_size_callback(GLFWwindow *w, int width, int height) {
    resize_gl(width, height);
    resize_renderer(width, height);
    mat4f_perspective(&perspective, 0.01f, 1000.0f, math_rad(45.0f), (float)width / height);
}
void mouse_callback(GLFWwindow *window, double x_in, double y_in);

void init_glfw(uint32_t width, uint32_t height, const char *title) {
    if (width == 0 || height == 0) {
        exit(-1);
    }
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    window = glfwCreateWindow(width, height, title, NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    if (!gladLoadGLLoader((GLADloadproc)(glfwGetProcAddress))) {
        exit(-1);
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

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

/*
float cube_vertices[] = {
    // Front (-Z)
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
*/

float cube_vertices[] = {
    // Front (-Z)
    -1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
     1.0f, -1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
     1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
    -1.0f,  1.0f, -1.0f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f,

    // Back (+Z)
    -1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
     1.0f, -1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
     1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f,
    -1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f,

    // Top (+Y)
    -1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f,
     1.0f,  1.0f, -1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
     1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
    -1.0f,  1.0f,  1.0f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f,

    // Bottom (-Y)
    -1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
     1.0f, -1.0f, -1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
     1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
    -1.0f, -1.0f,  1.0f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f,

    // Left (-X)
    -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
    -1.0f, -1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
    -1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
    -1.0f,  1.0f, -1.0f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,

    // Right (+X)
     1.0f, -1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
     1.0f, -1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
     1.0f,  1.0f,  1.0f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
     1.0f,  1.0f, -1.0f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f 
};

uint32_t cube_indices[] = {
    // Front (-Z)
    0, 2, 1,
    0, 3, 2,

    // Back (+Z)
    4, 5, 6,
    4, 6, 7,

    // Top (+Y)
    8, 10, 9,
    8, 11, 10,

    // Bottom (-Y)
    12, 13, 14,
    12, 14, 15,

    // Left (-X)
    16, 19, 18,
    16, 18, 17,

    // Right (+X)
    20, 22, 21,
    20, 23, 22
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

void mouse_callback(GLFWwindow *window, double x_in, double y_in) {
    float x_pos = (float)x_in;
    float y_pos = (float)y_in;

    if (cam3D.first_mouse) {
        cam3D.last_x = x_pos;
        cam3D.last_y = y_pos;
        cam3D.first_mouse = false;
    }

    float xoff = x_pos - cam3D.last_x;
    float yoff = cam3D.last_y - y_pos;

    cam3D.last_x = x_pos;
    cam3D.last_y = y_pos;

    xoff *= cam3D.sensitivity;
    yoff *= cam3D.sensitivity;

    cam3D.yaw += xoff;
    cam3D.pitch += yoff;

    cam3D.pitch = math_min(cam3D.pitch, 89.0f);
    cam3D.pitch = math_max(cam3D.pitch, -89.0f);

    vec3f front;
    front.x = cosf(math_rad(cam3D.yaw)) * cosf(math_rad(cam3D.pitch));
    front.y = sinf(math_rad(cam3D.pitch));
    front.z = sinf(math_rad(cam3D.yaw)) * cosf(math_rad(cam3D.pitch));
    cam3D.front = vec3f_norm(front);
}

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

/* vertex_t in shader.h
typedef struct {
    vec3f pos;
    vec3f normal;
    vec2f uv;
} vertex_t;
*/

typedef struct {
    uint8_t *pixels;
    char *type;
} texture_t;

typedef struct {
    vertex_t *vertices;
    uint32_t *indices;
    texture_t *textures;
} mesh_t;

void mesh_init(mesh_t *m) {
    m->vertices = vec_init(m->vertices, 10);
    m->indices = vec_init(m->indices, 10);
    m->textures = vec_init(m->textures, 10);
}

void mesh_draw(mesh_t *m, r_shader *s) {
    draw_elements(s, m->vertices, m->indices, vec_size(m->indices));
}

void mesh_destroy(mesh_t *m) {
    if (m->vertices) {
        vec_destroy(m->vertices);
    }
    vec_destroy(m->indices);
    if (m->textures) {
        for (int i = 0; i < vec_size(m->textures); ++i) {
            if (m->textures[i].pixels) {
                stbi_image_free(m->textures[i].pixels);
            }
        }
        vec_destroy(m->textures);
    }
}

typedef struct {
    mesh_t *meshes;
    char *dir;
} model_t;

void model_init(model_t *m) {
    m->meshes = vec_init(m->meshes, 10);
}

void model_draw(model_t *m, r_shader *s) {
    for (int i = 0; i < vec_size(m->meshes); ++i) {
        mesh_draw(&m->meshes[i], s);
    }
}

mesh_t model_process_mesh(struct aiMesh *mesh, const struct aiScene *scene) {
    mesh_t m;
    mesh_init(&m);
    for (int i = 0; i < mesh->mNumVertices; i++) {
        vertex_t vertex;
        vec3f vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.pos = vector;

        // Equivalent to HasNormals() Function in C++
        if (mesh->mNormals != NULL && mesh->mNumVertices > 0) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.normal = vector;
        }

        if(mesh->mTextureCoords[0]) {
            vec2f vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.uv = vec;
        }
        else {
            vertex.uv = VEC2F(0.0f, 0.0f);
        }

        vec_push(m.vertices, vertex);
    }
    for (int i = 0; i < mesh->mNumFaces; i++) {
        struct aiFace face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; j++) {
            vec_push(m.indices, face.mIndices[j]);
        }
    }
    return m;
}

void model_load(model_t *m, const char *path) {
    const struct aiScene* scene = aiImportFile(path, aiProcess_Triangulate | 
                                                     aiProcess_FlipUVs |
                                                     aiProcess_GenSmoothNormals | 
                                                     aiProcess_JoinIdenticalVertices |
                                                     aiProcess_PreTransformVertices |
                                                     aiProcess_SortByPType);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        printf("Error: %s\n", aiGetErrorString());
        return;
    }

    for (int i = 0; i < scene->mNumMeshes; i++) {
        struct aiMesh* mesh = scene->mMeshes[i];
        vec_push(m->meshes, model_process_mesh(mesh, scene));
    }
}

void model_destroy(model_t *m) {
    if (m->meshes) {
        for (int i = 0; i < vec_size(m->meshes); ++i) {
            mesh_destroy(&m->meshes[i]);
        }
        vec_destroy(m->meshes);
    }
}

int main() {
    pcg_rand_seed();

    init_renderer(800, 600);
    init_glfw(800, 600, "Renderer");
    glfwSwapInterval(0);
    init_gl(800, 600);
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

    // Init model
    model_t model;
    model_init(&model);
    model_load(&model, "gun.obj");

    double lastTime = glfwGetTime();
    int frames = 0;

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        frames++;
        if (now - lastTime >= 1.0) {
            double fps = frames / (now - lastTime);
            char title[64];
            snprintf(title, sizeof(title), "FPS: %.1f", fps);
            glfwSetWindowTitle(window, title);
            frames = 0;
            lastTime = now;
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cam3D.pos.x += 1;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cam3D.pos.x -= 1;
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cam3D.pos.z += 1;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cam3D.pos.z -= 1;
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            cam3D.pos.y += 1;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            cam3D.pos.y -= 1;
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
                // draw_arrays(&s, cube_vertices, 36);
                draw_elements(&s, cube_vertices, cube_indices, 36);

                // draw_arrays(&s, sphere_vertices, LAT_BANDS * LONG_BANDS * 6, RENDER_TRIANGLE);
            }
        }

        position = VEC3F(0, 0, 0);

        model_draw(&model, &s2);

        render_gl();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    cleanup_renderer();

    // Destroy renderer texture
    renderer_destroy_texture(&texture);

    // Destroy model
    model_destroy(&model);
}
