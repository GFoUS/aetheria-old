#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "context.h"
#include "vk_mem_alloc.h"

typedef struct _vulkan_image {
    vulkan_context* ctx;
    VkImage image;
    VmaAllocation allocation;
    bool ownsImage;
    VkImageView imageView;
    VkFormat format;
    u32 width;
    u32 height;
    VkSampleCountFlagBits samples;
} vulkan_image;

vulkan_image* vulkan_image_create(vulkan_context* ctx, VkFormat format, VkImageUsageFlags usage, u32 width, u32 height, VkImageAspectFlags aspects, VkSampleCountFlagBits samples);
vulkan_image* vulkan_image_create_from_file(vulkan_context* ctx, const char* path, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspects);
vulkan_image* vulkan_image_create_from_image(vulkan_context* ctx, VkImage image, VkFormat format, u32 width, u32 height, VkImageAspectFlags aspects);
void vulkan_image_destroy(vulkan_image* image);

vulkan_image* vulkan_image_get_default_color_texture(vulkan_context* ctx);

typedef struct {
    vulkan_context* ctx;
    VkSampler sampler;
} vulkan_sampler;

vulkan_sampler* vulkan_sampler_create(vulkan_context* ctx, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV);
void vulkan_sampler_destroy(vulkan_sampler* sampler);