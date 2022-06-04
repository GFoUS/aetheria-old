#include "command.h"

vulkan_command_pool* vulkan_command_pool_create(vulkan_device* device, u32 queueIndex) {
    VkCommandPoolCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = queueIndex;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vulkan_command_pool* pool = malloc(sizeof(vulkan_command_pool));
    pool->device = device;
    VkResult result = vkCreateCommandPool(device->device, &createInfo, NULL, &pool->pool);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan command pool creation failed with error code: %d", result);
    }

    return pool;
}
void vulkan_command_pool_destroy(vulkan_command_pool* pool) {
    vkDestroyCommandPool(pool->device->device, pool->pool, NULL);
    free(pool);
}

VkCommandBuffer vulkan_command_pool_get_buffer(vulkan_command_pool* pool) {
    VkCommandBufferAllocateInfo allocInfo;
    CLEAR_MEMORY(&allocInfo);

    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool->pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    VkResult result = vkAllocateCommandBuffers(pool->device->device, &allocInfo, &cmd);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan command buffer allocation failed with error code: %d", result);
    }

    return cmd;
}

void vulkan_command_pool_free_buffer(vulkan_command_pool* pool, VkCommandBuffer cmd) {
    vkFreeCommandBuffers(pool->device->device, pool->pool, 1, &cmd);
}