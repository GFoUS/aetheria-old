#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "context.h"

typedef struct {
    VkBuffer buffer;
    VmaAllocation allocation;
    vulkan_context* ctx;

    u64 size;
} vulkan_buffer;

vulkan_buffer* vulkan_buffer_create(vulkan_context* context, VkBufferUsageFlags usage, u64 size);
vulkan_buffer* vulkan_buffer_create_with_data(vulkan_context* context, VkBufferUsageFlags usage, u64 size, void* data);
void vulkan_buffer_destroy(vulkan_buffer* buffer);

void vulkan_buffer_update(vulkan_buffer* buffer, u64 size, void* data);