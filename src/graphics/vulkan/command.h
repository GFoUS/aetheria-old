#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "device.h"

typedef struct {
    VkCommandPool pool;
    vulkan_device* device;
} vulkan_command_pool;

vulkan_command_pool* vulkan_command_pool_create(vulkan_device* device, u32 queueIndex);
void vulkan_command_pool_destroy(vulkan_command_pool* pool);

VkCommandBuffer vulkan_command_pool_get_buffer(vulkan_command_pool* pool);
void vulkan_command_pool_free_buffer(vulkan_command_pool* pool, VkCommandBuffer cmd);