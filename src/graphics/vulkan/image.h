#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "context.h"
#include "vk_mem_alloc.h"

typedef struct {
    vulkan_context* ctx;
    VkImage image;
    VmaAllocation allocation;
    bool ownsImage;
    VkImageView imageView;
    VkSampler sampler;
    VkFormat format;
    u32 width;
    u32 height;
} vulkan_image;

vulkan_image* vulkan_image_create_from_file(vulkan_context* ctx, const char* path, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspects);
vulkan_image* vulkan_image_create_from_image(vulkan_context* ctx, VkImage image, VkFormat format, u32 width, u32 height, VkImageAspectFlags aspects);
void vulkan_image_destroy(vulkan_image* image);