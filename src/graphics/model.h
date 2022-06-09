#pragma once

#include "core/core.h"

#include "vulkan/context.h"
#include "vulkan/buffer.h"
#include "vulkan/image.h"
#include "vulkan/descriptor.h"
#include "vulkan/pipeline.h"

#include "cglm/cglm.h"

#include "gltf.h"

typedef struct {
    vec4 baseColorFactor;
} model_material_data;

typedef struct {
    vulkan_context* ctx;
    gltf_gltf* gltf;

    vulkan_pipeline_layout* pipelineLayout;
    vulkan_descriptor_set_layout* materialSetLayout;
    vulkan_descriptor_allocator* materialSetAllocator;
    vulkan_buffer** materialDataBuffers;
    vulkan_descriptor_set** materialSets;

    vulkan_buffer** buffers;
    vulkan_image** images;
    vulkan_sampler** samplers;
} model_model;

model_model* model_load_from_gltf(vulkan_context* ctx, gltf_gltf* gltf, vulkan_descriptor_set_layout* materialSetLayout, vulkan_pipeline_layout* pipelineLayout);
void model_unload(model_model* model);

void model_render(model_model* model, VkCommandBuffer cmd);