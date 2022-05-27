#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "device.h"
#include "graphics/window.h"

typedef struct {
    VkSwapchainKHR swapchain;
    
    vulkan_device* device;
} vulkan_swapchain;

vulkan_swapchain* vulkan_swapchain_create(vulkan_device* device, window* win, VkSurfaceKHR surface);
void vulkan_swapchain_destroy(vulkan_swapchain* swapchain);

