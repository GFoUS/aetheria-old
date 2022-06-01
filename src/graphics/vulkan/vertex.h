#pragma once

#include "core/core.h"
#include "cglm/cglm.h"
#include "vulkan/vulkan.h"

#define NUM_VERTEX_ATTRIBUTES 1

typedef struct {
    vec2 position;
} vulkan_vertex;

typedef struct {
    u32 numAttributes;
    VkVertexInputAttributeDescription attributes[NUM_VERTEX_ATTRIBUTES];
    VkVertexInputBindingDescription binding;
} vulkan_vertex_info;

vulkan_vertex_info vulkan_vertex_get_info();