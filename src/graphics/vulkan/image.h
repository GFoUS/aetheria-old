#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

typedef struct {
    VkImage image;
    VkFormat format;
    u32 width;
    u32 height;
} vulkan_image;

vulkan_image* vulkan_image_create_from_image(VkImage image, VkFormat format, u32 width, u32 height);
void vulkan_image_destroy(vulkan_image*);