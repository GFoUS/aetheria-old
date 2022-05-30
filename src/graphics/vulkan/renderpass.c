#include "renderpass.h"

vulkan_renderpass* vulkan_renderpass_create(vulkan_device* device, vulkan_swapchain* swapchain) {
    VkAttachmentDescription colorAttachment;
    CLEAR_MEMORY(&colorAttachment);

    colorAttachment.format = swapchain->format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef;
    CLEAR_MEMORY(&colorAttachmentRef);

    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass;
    CLEAR_MEMORY(&subpass);

    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;

    vulkan_renderpass* renderpass = malloc(sizeof(vulkan_renderpass));
    renderpass->device = device;
    VkResult result = vkCreateRenderPass(device->device, &createInfo, NULL, &renderpass->renderpass);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan renderpass creation failed with error code: %d", result);
    }

    return renderpass;
}

void vulkan_renderpass_destroy(vulkan_renderpass* renderpass) {
    vkDestroyRenderPass(renderpass->device->device, renderpass->renderpass, NULL);
    free(renderpass);
}

vulkan_framebuffer* vulkan_framebuffer_create(vulkan_device* device, vulkan_renderpass* renderpass, u32 numImages, vulkan_image** images) {
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
    framebuffer->numAttachments = numImages;
    framebuffer->attachments = images;
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