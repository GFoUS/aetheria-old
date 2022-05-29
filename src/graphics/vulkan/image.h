#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "device.h"

typedef struct {
    vulkan_device* device;
    VkImage image;
    bool ownsImage;
    VkImageView imageView;
    VkFormat format;
    u32 width;
    u32 height;
} vulkan_image;

vulkan_image* vulkan_image_create_from_image(vulkan_device* device, VkImage image, VkFormat format, u32 width, u32 height, VkImageAspectFlags aspects);
void vulkan_image_destroy(vulkan_image*);