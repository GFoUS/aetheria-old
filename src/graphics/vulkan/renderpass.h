#pragma once

// NOTE: This whole API will be changed when multiple subpasses are needed

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "device.h"
#include "swapchain.h"
#include "image.h"

typedef struct {
    VkRenderPass renderpass;
    vulkan_device* device;
} vulkan_renderpass;

vulkan_renderpass* vulkan_renderpass_create(vulkan_device* device, vulkan_swapchain* swapchain);
void vulkan_renderpass_destroy(vulkan_renderpass* renderpass);

typedef struct {
    VkFramebuffer framebuffer;
    vulkan_device* device;
    vulkan_renderpass* renderpass;
    u32 numAttachments;
    vulkan_image** attachments;
} vulkan_framebuffer;

vulkan_framebuffer* vulkan_framebuffer_create(vulkan_device* device, vulkan_renderpass* renderpass, u32 numImages, vulkan_image** images);
void vulkan_framebuffer_destroy(vulkan_framebuffer* framebuffer);