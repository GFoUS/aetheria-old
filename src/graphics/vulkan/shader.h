#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "device.h"

typedef enum {
    VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
    FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT
} vulkan_shader_type;

typedef struct {
    vulkan_device* device;
    VkShaderModule module;
    vulkan_shader_type type;
} vulkan_shader;

vulkan_shader* vulkan_shader_load_from_file(vulkan_device* device, const char* path, vulkan_shader_type type);
void vulkan_shader_destroy(vulkan_shader* shader);

VkPipelineShaderStageCreateInfo vulkan_shader_get_stage_info(vulkan_shader* shader);
