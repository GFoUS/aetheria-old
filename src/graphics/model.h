#pragma once

#include "core/core.h"

#include "vulkan/context.h"
#include "vulkan/buffer.h"

#include "cJSON.h"

typedef struct _model model;

typedef struct {
    u32 id;
    model* m;

    u32 numIndices;
    vulkan_buffer* indexBuffer;
    u32 numBuffers;
    vulkan_buffer** vertexBuffers;
} model_mesh;

typedef struct _model_node {
    u32 id;
    model* m;
    
    u32 numMeshes;
    model_mesh** meshes;

    u32 numChildren;
    struct _model_node** children;
} model_node;

typedef struct _model {
    vulkan_context* ctx;

    u32 numRootNodes;
    model_node** rootNodes;

    u32 numNodes;
    model_node* nodes;
    u32* meshPrimitivesLookup;
    u32 numMeshes;
    model_mesh* meshes;

    cJSON* data;
    const char* path;
} model;

model* model_load_from_file(const char* path, vulkan_context* ctx);
void model_destroy(model* m);

void model_render(model* m, VkCommandBuffer cmd);