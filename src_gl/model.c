#include "model.h"

#include <stb_image.h>
#include <cpstd/vector.h>

void mesh_init(mesh_t *m) {
    m->vertices = vec_init(m->vertices, 10);
    m->indices = vec_init(m->indices, 10);
    m->textures = vec_init(m->textures, 10);
}

void mesh_draw(mesh_t *m, r_shader *s) {
    renderer_draw_elements(s, m->vertices, m->indices, vec_size(m->indices));
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

void model_init(model_t *m) {
    m->meshes = vec_init(m->meshes, 10);
}

void model_draw(model_t *m, r_shader *s) {
    for (int i = 0; i < vec_size(m->meshes); ++i) {
        mesh_draw(&m->meshes[i], s);
    }
}

mesh_t process_mesh(struct aiMesh *mesh, const struct aiScene *scene) {
    mesh_t m;
    mesh_init(&m);
    for (int i = 0; i < mesh->mNumVertices; ++i) {
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
    for (int i = 0; i < mesh->mNumFaces; ++i) {
        struct aiFace face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; ++j) {
            vec_push(m.indices, face.mIndices[j]);
        }
    }
    return m;
}

void process_node(model_t *m, struct aiNode *node, const struct aiScene *scene) {
    for (int i = 0; i < node->mNumMeshes; ++i) {
        struct aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        vec_push(m->meshes, process_mesh(mesh, scene));
    }
    for (int i = 0; i < node->mNumChildren; ++i) {
        process_node(m, node->mChildren[i], scene);
    }
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
    process_node(m, scene->mRootNode, scene);
    aiReleaseImport(scene);
}

void model_destroy(model_t *m) {
    if (m->meshes) {
        for (int i = 0; i < vec_size(m->meshes); ++i) {
            mesh_destroy(&m->meshes[i]);
        }
        vec_destroy(m->meshes);
    }
}
