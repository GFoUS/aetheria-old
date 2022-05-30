#include "renderer.h"

renderer* renderer_create(window* win) {
    renderer* render = malloc(sizeof(renderer));
    render->ctx = vulkan_context_create(win); 
    
    render->imageAvailable = vulkan_context_get_semaphore(render->ctx, 0);
    render->renderFinished = vulkan_context_get_semaphore(render->ctx, 0);
    render->inFlight = vulkan_context_get_fence(render->ctx, VK_FENCE_CREATE_SIGNALED_BIT);

    return render;
}
void renderer_destroy(renderer* render) {
    vkDestroySemaphore(render->ctx->device->device, render->imageAvailable, NULL);
    vkDestroySemaphore(render->ctx->device->device, render->renderFinished, NULL);
    vkDestroyFence(render->ctx->device->device, render->inFlight, NULL);

    vulkan_context_destroy(render->ctx);
    free(render);
}

typedef struct {
    renderer* render;
    u32 swapchainIndex;
} render_info;

void _render(VkCommandBuffer cmd, render_info* info) {
    VkRenderPassBeginInfo renderInfo;
    CLEAR_MEMORY(&renderInfo);

    renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderInfo.renderPass = info->render->ctx->renderpass->renderpass;
    renderInfo.framebuffer = info->render->ctx->framebuffers[info->swapchainIndex]->framebuffer;
    renderInfo.renderArea.offset.x = 0;
    renderInfo.renderArea.offset.y = 0;
    renderInfo.renderArea.extent.width = info->render->ctx->swapchain->images[0]->width;
    renderInfo.renderArea.extent.height = info->render->ctx->swapchain->images[0]->height;
    
    VkClearValue clearValue;
    float clearValueData[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    memcpy(clearValue.color.float32, clearValueData, sizeof(float) * 4); // I don't know a better way of doing this
    renderInfo.clearValueCount = 1;
    renderInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(cmd, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info->render->ctx->pipeline->pipeline);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd);
}

void renderer_render(renderer* render) {
    vkWaitForFences(render->ctx->device->device, 1, &render->inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences(render->ctx->device->device, 1, &render->inFlight);

    u32 imageIndex;
    vkAcquireNextImageKHR(render->ctx->device->device, render->ctx->swapchain->swapchain, UINT64_MAX, render->imageAvailable, VK_NULL_HANDLE, &imageIndex);

    render_info info;
    info.render = render;
    info.swapchainIndex = imageIndex;

    vulkan_context_submit_config config;
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
    config.numWaitSemaphores = 1;
    config.waitSemaphores = &render->imageAvailable;
    config.waitStages = waitStages;
    config.numSignalSemaphores = 1;
    config.signalSemaphores = &render->renderFinished;
    config.fence = render->inFlight;

    vulkan_context_start_and_execute(render->ctx, &config, &info, _render);   

    VkPresentInfoKHR presentInfo;
    CLEAR_MEMORY(&presentInfo);

    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &render->renderFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &render->ctx->swapchain->swapchain;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(render->ctx->device->present, &presentInfo);
}