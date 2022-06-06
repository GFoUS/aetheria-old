#pragma once

#include "core/core.h"

#include "vulkan/context.h"
#include "vulkan/buffer.h"
#include "vulkan/image.h"
#include "vulkan/descriptor.h"
#include "cglm/cglm.h"

#include "cJSON.h"

typedef struct _model model;

typedef struct {
    vulkan_image* image;
    u32 texCoord;
    bool defaultTexture; // If it is a default texture, it should use a fake UV buffer of just zeros
} model_texture;

typedef struct {
    vec4 baseColorFactor;
} model_material_data;

typedef struct {
    u32 id;
    model* m;

    vulkan_descriptor_set* materialDataSet;
    model_texture baseColorTexture;
    model_material_data data;
    vulkan_buffer* materialDataBuffer;
} model_material;

typedef struct {
    u32 id;
    model* m;

    u32 numIndices;
    vulkan_buffer* indexBuffer;
    u32 numBuffers;
    vulkan_buffer** vertexBuffers;

    model_material* material;
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

    u32 numMaterials;
    model_material* materials;

    u32 numImages;
    vulkan_image** images;

    cJSON* data;
    const char* path;

    vulkan_descriptor_allocator* materialSetAllocator;
} model;

model* model_load_from_file(const char* path, vulkan_context* ctx, vulkan_descriptor_set_layout* materialSetLayout);
void model_destroy(model* m);

void model_render(model* m, VkCommandBuffer cmd, VkPipelineLayout layout);