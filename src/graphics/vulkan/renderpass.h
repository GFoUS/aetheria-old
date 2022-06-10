#pragma once

// NOTE: This whole API will be changed when multiple subpasses are needed

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "device.h"
#include "swapchain.h"
#include "image.h"

typedef u32 vulkan_subpass_attachment;

typedef struct {
    u32 numInputAttachments;
    vulkan_subpass_attachment* inputAttachments;
    u32 numColorAttachments;
    vulkan_subpass_attachment* colorAttachments;
    bool isDepthBuffered;
    vulkan_subpass_attachment depthAttachment;
    bool isResolving;
    vulkan_subpass_attachment resolveAttachment;
} vulkan_subpass_config;

typedef struct {
    u32 numSubpasses;
    VkSubpassDescription* subpasses;

    u32 numAttachments;
    VkAttachmentDescription* attachments;
} vulkan_renderpass_builder;

typedef struct {
    VkRenderPass renderpass;
    vulkan_device* device;

    u32 numSubpasses;
    VkSubpassDescription* subpasses;

    u32 numAttachments;
    VkAttachmentDescription* attachments;
} vulkan_renderpass;

vulkan_renderpass_builder* vulkan_renderpass_builder_create();
vulkan_subpass_attachment vulkan_renderpass_builder_add_attachment(vulkan_renderpass_builder* builder, VkAttachmentDescription* attachment);
void vulkan_renderpass_builder_add_subpass(vulkan_renderpass_builder* builder, vulkan_subpass_config* config);
vulkan_renderpass* vulkan_renderpass_builder_build(vulkan_renderpass_builder* builder, vulkan_device* device);
void vulkan_renderpass_destroy(vulkan_renderpass* renderpass);

typedef struct {
    VkFramebuffer framebuffer;
    vulkan_device* device;
    vulkan_renderpass* renderpass;
    u32 width;
    u32 height;
} vulkan_framebuffer;

vulkan_framebuffer* vulkan_framebuffer_create(vulkan_device* device, vulkan_renderpass* renderpass, u32 numImages, vulkan_image** images);
void vulkan_framebuffer_destroy(vulkan_framebuffer* framebuffer);

void vulkan_renderpass_bind(VkCommandBuffer cmd, vulkan_renderpass* renderpass, vulkan_framebuffer* framebuffer);

VkAttachmentDescription vulkan_renderpass_get_default_color_attachment(VkFormat format, VkSampleCountFlagBits samples);
VkAttachmentDescription vulkan_renderpass_get_default_depth_attachment(VkSampleCountFlagBits samples);
VkAttachmentDescription vulkan_renderpass_get_default_resolve_attachment(VkFormat format);