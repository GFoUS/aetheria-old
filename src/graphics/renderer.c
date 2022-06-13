#include "renderer.h"

#include "vulkan/vertex.h"
#include "cglm/cglm.h"

typedef struct {
    mat4 view;
    mat4 proj;
} global_data;

void create_swapchain(renderer* render) {
    vkDeviceWaitIdle(render->ctx->device->device);

    if (render->ctx->swapchain == NULL) {
        render->ctx->swapchain = vulkan_swapchain_create(render->ctx, render->ctx->win, render->ctx->surface);
    }
}

renderer* renderer_create(window* win) {
    renderer* render = malloc(sizeof(renderer));
    CLEAR_MEMORY(render);
    render->ctx = vulkan_context_create(win); 
    
    render->imageAvailable = vulkan_context_get_semaphore(render->ctx, 0);
    render->renderFinished = vulkan_context_get_semaphore(render->ctx, 0);
    render->inFlight = vulkan_context_get_fence(render->ctx, VK_FENCE_CREATE_SIGNALED_BIT);  

    create_swapchain(render);

    render->gltf = gltf_load_file("models/samples/2.0/Sponza/glTF/Sponza.gltf");
    render->model = model_load_from_gltf(render->ctx, render->gltf);

    return render;
}

void destroy_swapchain(renderer* render, bool destroySwapchain) {
    if (destroySwapchain) {
        vulkan_swapchain_destroy(render->ctx->swapchain);
        render->ctx->swapchain = NULL;
    }
}

void renderer_destroy(renderer* render) {
    destroy_swapchain(render, false);  

    model_unload(render->model);
    gltf_unload(render->gltf);  

    vkDestroySemaphore(render->ctx->device->device, render->imageAvailable, NULL);
    vkDestroySemaphore(render->ctx->device->device, render->renderFinished, NULL);
    vkDestroyFence(render->ctx->device->device, render->inFlight, NULL);

    vulkan_context_destroy(render->ctx);
    free(render);
}

typedef struct {
    model_model* model;
} render_data;

void draw_models(VkCommandBuffer cmd, void* dataPtr) {
    render_data* data = (render_data*)dataPtr;
    model_render(data->model, cmd);
}

void renderer_render(renderer* render) {
    vulkan_shader* renderVertexShader = vulkan_shader_load_from_file(render->ctx->device, "shaders/vert.spv", VERTEX);
	vulkan_shader* renderFragmentShader = vulkan_shader_load_from_file(render->ctx->device, "shaders/frag.spv", FRAGMENT);

    framegraph_config framegraphConfig;
    framegraphConfig.width = render->ctx->swapchain->extent.width;
    framegraphConfig.height = render->ctx->swapchain->extent.height;
    framegraphConfig.maxSamples = render->ctx->physical->maxSamples;
    framegraph_framegraph* framegraph = framegraph_create(framegraphConfig);
    
    render_data data;
    data.model = render->model;

    framegraph_add_image(framegraph, "albedo", VK_FORMAT_R8G8B8_SRGB, true);
    framegraph_add_image(framegraph, "depth", VK_FORMAT_D32_SFLOAT, true);
    framegraph_add_image(framegraph, "backbuffer", render->ctx->swapchain->format, false);
    framegraph_pass_config renderPassConfig;
    CLEAR_MEMORY(&renderPassConfig);
    renderPassConfig.numOutputs = 1;
    renderPassConfig.outputs[0] = "albedo";
    renderPassConfig.depth = "depth";
    renderPassConfig.resolve = "backbuffer";
    renderPassConfig.shaders.vertex = renderVertexShader;
    renderPassConfig.shaders.fragment = renderFragmentShader;
    renderPassConfig.dataPtr = &data;
    renderPassConfig.execFn = draw_models;
    framegraph_add_pass(framegraph, renderPassConfig);

    framegraph_compile(framegraph);
    framegraph_create_resources(framegraph, render->ctx);

    vulkan_shader_destroy(renderVertexShader);
	vulkan_shader_destroy(renderFragmentShader);

    vkWaitForFences(render->ctx->device->device, 1, &render->inFlight, VK_TRUE, UINT64_MAX);

    u32 imageIndex;
    VkResult aquireResult = vkAcquireNextImageKHR(render->ctx->device->device, render->ctx->swapchain->swapchain, UINT64_MAX, render->imageAvailable, VK_NULL_HANDLE, &imageIndex);
    if (aquireResult == VK_ERROR_OUT_OF_DATE_KHR || aquireResult == VK_SUBOPTIMAL_KHR) {
        render->recreateSwapchain = true;
    }
    else if (aquireResult != VK_SUCCESS) {
        FATAL("Vulkan swapchain image aquisition failed with error code: %d", aquireResult);
    }

    if (render->recreateSwapchain) {
        destroy_swapchain(render, true);
        create_swapchain(render);
        render->recreateSwapchain = false;
        return;
    }

    vkResetFences(render->ctx->device->device, 1, &render->inFlight);

    if (render->cmd != NULL) {
        vulkan_command_pool_free_buffer(render->ctx->commandPool, render->cmd);
    }

    VkCommandBuffer cmd = framegraph_record(framegraph, render->ctx);
    VkSubmitInfo submitInfo;
    CLEAR_MEMORY(&submitInfo);
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &render->imageAvailable;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &render->renderFinished;
    VkResult result = vkQueueSubmit(render->ctx->device->graphics, 1, &submitInfo, render->inFlight);

    VkPresentInfoKHR presentInfo;
    CLEAR_MEMORY(&presentInfo);

    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &render->renderFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &render->ctx->swapchain->swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(render->ctx->device->present, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        render->recreateSwapchain = true;
    }
    else if (presentResult != VK_SUCCESS) {
        FATAL("Vulkan swapchain presentation failed with error code: %d", presentResult);
    }
    INFO("Frame done");
}