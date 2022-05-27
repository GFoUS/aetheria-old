#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "device.h"
#include "image.h"
#include "graphics/window.h"

typedef struct {
    VkSwapchainKHR swapchain;
    VkExtent2D extent;
    VkFormat format;
    
    u32 numImages;
    vulkan_image** images;
    vulkan_device* device;
} vulkan_swapchain;

vulkan_swapchain* vulkan_swapchain_create(vulkan_device* device, window* win, VkSurfaceKHR surface);
void vulkan_swapchain_destroy(vulkan_swapchain* swapchain);

