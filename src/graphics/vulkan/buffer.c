#include "buffer.h"

vulkan_buffer* vulkan_buffer_create(vulkan_context* ctx, VkBufferUsageFlags usage, u64 size) {
    VkBufferCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo;
    CLEAR_MEMORY(&allocInfo);
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    vulkan_buffer* buffer = malloc(sizeof(vulkan_buffer));
    buffer->ctx = ctx;
    buffer->size = size;
    VkResult result = vmaCreateBuffer(ctx->allocator, &createInfo, &allocInfo, &buffer->buffer, &buffer->allocation, NULL);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan buffer creation failed with error code: %d", result);
    }

    return buffer;
}

vulkan_buffer* vulkan_buffer_create_with_data(vulkan_context* context, VkBufferUsageFlags usage, u64 size, void* data) {
    vulkan_buffer* buffer = vulkan_buffer_create(context, usage, size);
    vulkan_buffer_update(buffer, size, data);
    return buffer;
}

void vulkan_buffer_destroy(vulkan_buffer* buffer) {
    vmaDestroyBuffer(buffer->ctx->allocator, buffer->buffer, buffer->allocation);
    free(buffer);
}

void vulkan_buffer_update(vulkan_buffer* buffer, u64 size, void* data) {
    void* bufferData;
    vmaMapMemory(buffer->ctx->allocator, buffer->allocation, &bufferData);
    memcpy(bufferData, data, size);
    vmaUnmapMemory(buffer->ctx->allocator, buffer->allocation);
}