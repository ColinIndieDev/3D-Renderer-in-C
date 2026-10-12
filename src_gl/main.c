#define RENDERER_IMPL
#include <renderer.h>

#define STB_IMAGE_IMPLEMENTATION
#include "frustum.h"
#include "shader.h"
#include "model.h"
#include "opengl.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cpstd/rand.h>
#include <cpstd/vector.h>
#include <stdbool.h>

#define NEAR 0.01f
#define FAR 1000.0f

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

cam3D_t cam3D;
frustum_t frustum;

#pragma region GLFW
GLFWwindow *window = NULL;

void framebuffer_size_callback(GLFWwindow *w, int width, int height) {
    resize_gl(width, height);
    renderer_resize(width, height);
    mat4f_perspective(&perspective, NEAR, FAR, math_rad(45.0f), (float)width / height);
}

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
#pragma endregion

#pragma region Vertices
float cube_vertices_raw[] = {
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

float skybox_vertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

float plane_vertices[] = {
     10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    -10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

     10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
     10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f, 1.0f, 1.0f
};

#define LAT_BANDS 8
#define LONG_BANDS 8

vec3f sphere_vertices[LAT_BANDS * LONG_BANDS * 6];
size_t sphere_vertices_size = 0;

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
#pragma endregion

#define N 100

int main() {
    pcg_rand_seed();

    renderer_init(800, 600);
    init_glfw(800, 600, "Renderer");
    glfwSwapInterval(0);
    init_gl(800, 600);
    mat4f_perspective(&perspective, NEAR, FAR, math_rad(45.0f), 800.0f / 600);
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

    vec3f positions[N];
    for (int i = 0; i < N; ++i) {
        positions[i] = VEC3F(pcg_randf_range(-25.0f, 25.0f), pcg_randf_range(-25.0f, 25.0f), pcg_randf_range(-100.0f, 100.0f));
    }

    // Init renderer texture
    texture = renderer_create_texture("texture.png");

    // Init model
    model_t model;
    model_init(&model);
    model_load(&model, "gun.obj");

    // Init cube map
    const char *paths[6] = {
        "sky/nx.png", 
        "sky/px.png", 
        "sky/py.png", 
        "sky/ny.png", 
        "sky/pz.png", 
        "sky/nz.png", 
    };
    cubemap = renderer_create_cubemap(paths);

    double lastTime = glfwGetTime();
    int frames = 0;

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        frames++;
        if (now - lastTime >= 1.0) {
            double fps = frames / (now - lastTime);
            char title[64];
            snprintf(title, sizeof(title), "FPS: %d", (int)fps);
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
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }

        clear_background((r_rgba){0, 0, 0, 255});

        mat4f_look_at(&view, cam3D.pos, VEC3F(cam3D.pos.x + cam3D.front.x, cam3D.pos.y + cam3D.front.y, cam3D.pos.z + cam3D.front.z), VEC3F(0, 1, 0));
        memcpy(view_cubemap, view, sizeof(mat4f));
        view_cubemap[3] = 0;
        view_cubemap[7] = 0;
        view_cubemap[11] = 0;
        view_cubemap[12] = 0;
        view_cubemap[13] = 0;
        view_cubemap[14] = 0;
        view_cubemap[15] = 1;
        mat4f view_projection;
        mat4f_mul(&perspective, &view, &view_projection);
        frustum_update(&frustum, &view_projection);

        // Shader "uniform"
        shader_uniform_set_view_pos(cam3D.pos);

        // Draw scene
        rotation += 1;

        for (int i = 0; i < N; ++i) {
            position = positions[i];
            if (is_sphere_visible(&frustum, position, radius)) {
                renderer_draw_elements(&s, cube_vertices, cube_indices, 36);
            }
        }

        position = VEC3F(0, 0, 0);

        float saved_rot = rotation;
        rotation = 0;
        renderer_draw_arrays(&s, plane_vertices, 6);
        rotation = saved_rot;

        model_draw(&model, &s2);

        // Draw skybox
        renderer_depth_func(DEPTH_FUNC_LEQUAL);
        renderer_draw_arrays(&s_cubemap, skybox_vertices, 36);
        renderer_depth_func(DEPTH_FUNC_LESS);

        render_gl(renderer_get_screen_width(), renderer_get_screen_height(), renderer_get_pixel_buffer());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Destroy renderer texture
    renderer_destroy_texture(&texture);

    // Destroy model
    model_destroy(&model);

    // Destroy cubemap
    renderer_destroy_cubemap(&cubemap);

    renderer_cleanup();
    glfwDestroyWindow(window);
}
