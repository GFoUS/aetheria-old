#pragma once

#include "graphics/vulkan/context.h"
#include "graphics/vulkan/shader.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/pipeline.h"

typedef struct framegraph_pass_t framegraph_pass;
typedef struct framegraph_image_t framegraph_image;
typedef struct framegraph_framegraph_t framegraph_framegraph;

typedef struct {
    vulkan_shader* vertex;
    vulkan_shader* fragment;
} framegraph_pass_shaders;

typedef struct {
    u32 numInputs;
    const char* inputs[256];
    u32 numOutputs;
    const char* outputs[256];
    const char* depth;
    const char* resolve;

    framegraph_pass_shaders shaders;

    void* dataPtr;
    void(*execFn)(VkCommandBuffer, void*);
} framegraph_pass_config;

typedef struct framegraph_pass_t {
    framegraph_framegraph* framegraph;
    framegraph_pass_config config;
    
    u32 numDescriptorLayouts;
    vulkan_descriptor_set_layout** descriptorLayouts;
    vulkan_pipeline pipeline;
} framegraph_pass;

typedef struct framegraph_image_t {
    framegraph_framegraph* framegraph;
    const char* name;

    u32 width;
    u32 height;
    VkFormat format;
    bool multisampled;

    framegraph_pass* write;
    u32 numReads;
    framegraph_pass* reads[256];
} framegraph_image;

typedef struct {
    u32 width;
    u32 height;
    VkSampleCountFlags maxSamples;
} framegraph_config;

typedef struct framegraph_framegraph_t {
    framegraph_config config;

    u32 numImages;
    framegraph_image* images[256];

    u32 numPasses;
    framegraph_pass* passes[256];
} framegraph_framegraph;

framegraph_framegraph* framegraph_create(framegraph_config config);
void framegraph_destroy(framegraph_framegraph* framegraph);

void framegraph_add_image(framegraph_framegraph* framegraph, const char* name, VkFormat format, bool multisampled);
void framegraph_add_pass(framegraph_framegraph* framegraph, framegraph_pass_config config);

void framegraph_compile(framegraph_framegraph* framegraph);
void framegraph_create_resources(framegraph_framegraph* framegraph, vulkan_context* ctx);
VkCommandBuffer framegraph_record(framegraph_framegraph* framegraph, vulkan_context* ctx);
