#include "renderpass.h"

vulkan_renderpass_builder* vulkan_renderpass_builder_create() {
    vulkan_renderpass_builder* builder = malloc(sizeof(vulkan_renderpass_builder));
    CLEAR_MEMORY(builder);
    builder->subpasses = malloc(0);
    builder->attachments = malloc(0);

    return builder;
}

vulkan_subpass_attachment vulkan_renderpass_builder_add_attachment(vulkan_renderpass_builder* builder, VkAttachmentDescription* attachment) {
    builder->numAttachments++;
    builder->attachments = realloc(builder->attachments, sizeof(VkAttachmentDescription) * builder->numAttachments);
    memcpy(&builder->attachments[builder->numAttachments - 1], attachment, sizeof(VkAttachmentDescription));

    return builder->numAttachments - 1;
}

void vulkan_renderpass_builder_add_subpass(vulkan_renderpass_builder* builder, vulkan_subpass_config* config) {
    builder->numSubpasses++;
    builder->subpasses = realloc(builder->subpasses, sizeof(VkSubpassDescription) * builder->numSubpasses);

    VkAttachmentReference* inputAttachments = malloc(sizeof(VkAttachmentReference) * config->numInputAttachments);
    for (u32 i = 0; i < config->numInputAttachments; i++) {
        inputAttachments[i].attachment = config->inputAttachments[i];
        inputAttachments[i].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkAttachmentReference* colorAttachments = malloc(sizeof(VkAttachmentReference) * config->numColorAttachments);
    CLEAR_MEMORY_ARRAY(colorAttachments, config->numColorAttachments);
    for (u32 i = 0; i < config->numColorAttachments; i++) {
        colorAttachments[i].attachment = config->colorAttachments[i];
        colorAttachments[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference* depthAttachment = NULL;
    if (config->isDepthBuffered) {
        depthAttachment = malloc(sizeof(VkAttachmentReference));
        depthAttachment->attachment = config->depthAttachment;
        depthAttachment->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference* resolveAttachment = NULL;
    if (config->isResolving) {
        resolveAttachment = malloc(sizeof(VkAttachmentReference));
        resolveAttachment->attachment = config->resolveAttachment;
        resolveAttachment->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription* subpass = &builder->subpasses[builder->numSubpasses - 1];
    CLEAR_MEMORY(subpass);
    subpass->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass->inputAttachmentCount = config->numInputAttachments;
    subpass->pInputAttachments = inputAttachments;
    subpass->colorAttachmentCount = config->numColorAttachments;
    subpass->pColorAttachments = colorAttachments;
    subpass->pDepthStencilAttachment = depthAttachment;
    subpass->pResolveAttachments = resolveAttachment;
}

vulkan_renderpass* vulkan_renderpass_builder_build(vulkan_renderpass_builder* builder, vulkan_device* device) {
    VkSubpassDependency* dependencies = malloc(sizeof(VkSubpassDependency) * (builder->numSubpasses - 1));
    CLEAR_MEMORY_ARRAY(dependencies, builder->numSubpasses - 1);
    for (u32 i = 0; i < (builder->numSubpasses - 1); i++) {
        dependencies[i].srcSubpass = i;
        dependencies[i].dstSubpass = i + 1;
        dependencies[i].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[i].srcAccessMask = 0;
        dependencies[i].dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dependencies[i].dstAccessMask = 0;
    }
    
    VkRenderPassCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = builder->numAttachments;
    createInfo.pAttachments = builder->attachments;
    createInfo.subpassCount = builder->numSubpasses;
    createInfo.pSubpasses = builder->subpasses;
    createInfo.dependencyCount = builder->numSubpasses - 1;
    createInfo.pDependencies = dependencies;

    vulkan_renderpass* renderpass = malloc(sizeof(vulkan_renderpass));
    renderpass->device = device;
    renderpass->numAttachments = builder->numAttachments;
    renderpass->attachments = builder->attachments;
    renderpass->numSubpasses = builder->numSubpasses;
    renderpass->subpasses = builder->subpasses;

    VkResult result = vkCreateRenderPass(device->device, &createInfo, NULL, &renderpass->renderpass);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan renderpass creation failed with error code: %d", result);
    }

    free(builder);

    return renderpass;
}

void vulkan_renderpass_destroy(vulkan_renderpass* renderpass) {
    for (u32 i = 0; i < renderpass->numSubpasses; i++) {
        free(renderpass->subpasses[i].pInputAttachments);
        free(renderpass->subpasses[i].pColorAttachments);
        if (renderpass->subpasses[i].pDepthStencilAttachment) {
            free(renderpass->subpasses[i].pDepthStencilAttachment);
        }
        if (renderpass->subpasses[i].pResolveAttachments) {
            free(renderpass->subpasses[i].pResolveAttachments);
        }
    }
    free(renderpass->attachments);
    free(renderpass->subpasses);
    vkDestroyRenderPass(renderpass->device->device, renderpass->renderpass, NULL);
    free(renderpass);
}

vulkan_framebuffer* vulkan_framebuffer_create(vulkan_device* device, vulkan_renderpass* renderpass, u32 numImages, vulkan_image** images) {
    if (numImages != renderpass->numAttachments) {
        FATAL("Incorrect amount of images for framebuffer, %d images were given, but the renderpass has %d attachments", numImages, renderpass->numAttachments);
    }

    VkImageView* attachments = malloc(sizeof(VkImageView) * numImages);
    CLEAR_MEMORY_ARRAY(attachments, numImages);

    for (u32 i = 0; i < numImages; i++) {
        attachments[i] = images[i]->imageView;
    }

    VkFramebufferCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = renderpass->renderpass;
    createInfo.attachmentCount = numImages;
    createInfo.pAttachments = attachments;
    createInfo.width = images[0]->width;
    createInfo.height = images[0]->height;
    createInfo.layers = 1;

    vulkan_framebuffer* framebuffer = malloc(sizeof(vulkan_framebuffer));
    framebuffer->device = device;
    framebuffer->renderpass = renderpass;
    framebuffer->width = images[0]->width;
    framebuffer->height = images[0]->height;
    VkResult result = vkCreateFramebuffer(device->device, &createInfo, NULL, &framebuffer->framebuffer);
    if (result != VK_SUCCESS) {
        FATAL("Framebuffer creation failed with error code: %d", result);
    }

    free(attachments);

    return framebuffer;
}

void vulkan_framebuffer_destroy(vulkan_framebuffer* framebuffer) {
    vkDestroyFramebuffer(framebuffer->device->device, framebuffer->framebuffer, NULL);
    free(framebuffer);
}

void vulkan_renderpass_bind(VkCommandBuffer cmd, vulkan_renderpass* renderpass, vulkan_framebuffer* framebuffer) {
    VkRenderPassBeginInfo renderInfo;
    CLEAR_MEMORY(&renderInfo);

    renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderInfo.renderPass = renderpass->renderpass;
    renderInfo.framebuffer = framebuffer->framebuffer;
    renderInfo.renderArea.offset.x = 0;
    renderInfo.renderArea.offset.y = 0;
    renderInfo.renderArea.extent.width = framebuffer->width;
    renderInfo.renderArea.extent.height = framebuffer->height;
    
    u32 numClearValues = 0;
    VkClearValue* clearValues = malloc(0);
    for (u32 i = 0; i < renderpass->numAttachments; i++) {
        VkAttachmentDescription* attachment = &renderpass->attachments[i];
        if (attachment->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
            numClearValues++;
            clearValues = realloc(clearValues, sizeof(VkClearValue) * (i + 1));
            CLEAR_MEMORY(&clearValues[i]);

            if (attachment->format == VK_FORMAT_R32G32B32A32_SFLOAT) {
                float clearValueData[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
                memcpy(clearValues[i].color.float32, clearValueData, sizeof(float) * 4);
            } else if (attachment->format == VK_FORMAT_D32_SFLOAT) {
                clearValues[i].depthStencil.depth = 1.0f;
            } else if (attachment->format == VK_FORMAT_R16_SFLOAT) { // Special rule for the the reveal buffer (need a way of specifying this)
                clearValues[i].color.float32[0] = 1.0f;
            }
        }
    }

    renderInfo.clearValueCount = numClearValues;
    renderInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);
    free(clearValues);
}

VkAttachmentDescription vulkan_renderpass_get_default_color_attachment(VkFormat format, VkSampleCountFlagBits samples) {
    VkAttachmentDescription attachment;
    CLEAR_MEMORY(&attachment);
    attachment.format = format;
    attachment.samples = samples;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return attachment;
}

VkAttachmentDescription vulkan_renderpass_get_default_depth_attachment(VkSampleCountFlagBits samples) {
    VkAttachmentDescription attachment;
    CLEAR_MEMORY(&attachment);
    attachment.format = VK_FORMAT_D32_SFLOAT;
    attachment.samples = samples;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    return attachment;
}

VkAttachmentDescription vulkan_renderpass_get_default_resolve_attachment(VkFormat format) {
    VkAttachmentDescription attachment;
    CLEAR_MEMORY(&attachment);
    attachment.format = format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    return attachment;
}