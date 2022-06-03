#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "graphics/window.h"

typedef struct _vulkan_image vulkan_image;
typedef struct _vulkan_context vulkan_context;

typedef struct {
    VkSwapchainKHR swapchain;
    VkExtent2D extent;
    VkFormat format;
    
    u32 numImages;
    vulkan_image** images;
    vulkan_context* ctx;
} vulkan_swapchain;

vulkan_swapchain* vulkan_swapchain_create(vulkan_context* ctx, window* win, VkSurfaceKHR surface);
void vulkan_swapchain_destroy(vulkan_swapchain* swapchain);

