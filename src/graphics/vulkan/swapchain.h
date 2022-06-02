#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "graphics/window.h"

typedef struct {
    VkSwapchainKHR swapchain;
    VkExtent2D extent;
    VkFormat format;
    
    u32 numImages;
    struct vulkan_image** images;
    struct vulkan_context* ctx;
} vulkan_swapchain;

vulkan_swapchain* vulkan_swapchain_create(struct vulkan_context* ctx, window* win, VkSurfaceKHR surface);
void vulkan_swapchain_destroy(vulkan_swapchain* swapchain);

