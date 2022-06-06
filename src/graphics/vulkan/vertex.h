#pragma once

#include "core/core.h"
#include "cglm/cglm.h"
#include "vulkan/vulkan.h"

#define NUM_VERTEX_ATTRIBUTES 2

typedef struct {
    vec3 position;
    vec3 normal;
} vulkan_vertex;

typedef struct {
    u32 numAttributes;
    VkVertexInputAttributeDescription attributes[NUM_VERTEX_ATTRIBUTES];
    VkVertexInputBindingDescription bindings[NUM_VERTEX_ATTRIBUTES];
} vulkan_vertex_info;

vulkan_vertex_info vulkan_vertex_get_info();