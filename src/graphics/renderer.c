#include "renderer.h"

#include "vulkan/vertex.h"
#include "cglm/cglm.h"

static vulkan_vertex vertices[4] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f}}
};

static u32 indices[6] = {
    0, 1, 2, 2, 3, 0
};

typedef struct {
    mat4 view;
    mat4 proj;
} global_data;

renderer* renderer_create(window* win) {
    renderer* render = malloc(sizeof(renderer));
    render->ctx = vulkan_context_create(win); 
    
    render->imageAvailable = vulkan_context_get_semaphore(render->ctx, 0);
    render->renderFinished = vulkan_context_get_semaphore(render->ctx, 0);
    render->inFlight = vulkan_context_get_fence(render->ctx, VK_FENCE_CREATE_SIGNALED_BIT);

    render->vertexBuffer = vulkan_buffer_create_with_data(render->ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(vulkan_vertex) * 4, vertices);
    render->indexBuffer = vulkan_buffer_create_with_data(render->ctx, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(u32) * 6, indices);    

    vulkan_descriptor_set_layout_builder* globalSetLayoutBuilder = vulkan_descriptor_set_layout_builder_create();
    vulkan_descriptor_set_layout_builder_add(globalSetLayoutBuilder, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vulkan_descriptor_set_layout_builder_add(globalSetLayoutBuilder, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    render->globalSetLayout = vulkan_descriptor_set_layout_builder_build(globalSetLayoutBuilder, render->ctx->device);
    render->globalSetAllocator = vulkan_descriptor_allocator_create(render->ctx->device, render->globalSetLayout);
    render->globalSet = vulkan_descriptor_set_allocate(render->globalSetAllocator);
    render->globalSetBuffer = vulkan_buffer_create(render->ctx, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(global_data));
    vulkan_descriptor_set_write_buffer(render->globalSet, 0, render->globalSetBuffer);

     render->renderpass = vulkan_renderpass_create(render->ctx->device, render->ctx->swapchain);
	INFO("Created renderpass");

	vulkan_shader* vertexShader = vulkan_shader_load_from_file(render->ctx->device, "shaders/vert.spv", VERTEX);
	vulkan_shader* fragmentShader = vulkan_shader_load_from_file(render->ctx->device, "shaders/frag.spv", FRAGMENT);
	vulkan_pipeline_config pipelineConfig;
	CLEAR_MEMORY(&pipelineConfig);
	
	pipelineConfig.vertexShader = vertexShader;
	pipelineConfig.fragmentShader = fragmentShader;
	pipelineConfig.width = render->ctx->swapchain->extent.width;
	pipelineConfig.height = render->ctx->swapchain->extent.height;
	pipelineConfig.renderpass = render->renderpass;
    pipelineConfig.numSetLayouts = 1;
    pipelineConfig.setLayouts = &render->globalSetLayout;

	render->pipeline = vulkan_pipeline_create(render->ctx->device, &pipelineConfig);
	INFO("Created pipeline");

    vulkan_shader_destroy(vertexShader);
	vulkan_shader_destroy(fragmentShader);

    render->framebuffers = malloc(sizeof(vulkan_framebuffer*) * render->ctx->swapchain->numImages);
	for (u32 i = 0; i < render->ctx->swapchain->numImages; i++) {
		render->framebuffers[i] = vulkan_framebuffer_create(render->ctx->device, render->renderpass, 1, &render->ctx->swapchain->images[i]);
	}
	INFO("Created framebuffers");

    render->goomy = vulkan_image_create_from_file(render->ctx, "goomy.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    vulkan_descriptor_set_write_image(render->globalSet, 1, render->goomy);

    return render;
}
void renderer_destroy(renderer* render) {
    for (u32 i = 0; i < render->ctx->swapchain->numImages; i++) {
		vulkan_framebuffer_destroy(render->framebuffers[i]);
	}
	free(render->framebuffers);

    vulkan_pipeline_destroy(render->pipeline);
	vulkan_renderpass_destroy(render->renderpass);

    vulkan_descriptor_allocator_destroy(render->globalSetAllocator);
    vulkan_descriptor_set_layout_destroy(render->globalSetLayout);

    vkDestroySemaphore(render->ctx->device->device, render->imageAvailable, NULL);
    vkDestroySemaphore(render->ctx->device->device, render->renderFinished, NULL);
    vkDestroyFence(render->ctx->device->device, render->inFlight, NULL);

    vulkan_buffer_destroy(render->vertexBuffer);
    vulkan_buffer_destroy(render->indexBuffer);
    vulkan_buffer_destroy(render->globalSetBuffer);

    vulkan_image_destroy(render->goomy);

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
    renderInfo.renderPass = info->render->renderpass->renderpass;
    renderInfo.framebuffer = info->render->framebuffers[info->swapchainIndex]->framebuffer;
    renderInfo.renderArea.offset.x = 0;
    renderInfo.renderArea.offset.y = 0;
    renderInfo.renderArea.extent.width = info->render->ctx->swapchain->extent.width;
    renderInfo.renderArea.extent.height = info->render->ctx->swapchain->extent.height;
    
    VkClearValue clearValue;
    float clearValueData[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    memcpy(clearValue.color.float32, clearValueData, sizeof(float) * 4); // I don't know a better way of doing this
    renderInfo.clearValueCount = 1;
    renderInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(cmd, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info->render->pipeline->pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info->render->pipeline->layout->layout, 0, 1, &info->render->globalSet->set, 0, NULL);
    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &info->render->vertexBuffer->buffer, offsets);
    vkCmdBindIndexBuffer(cmd, info->render->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(cmd);
}

void renderer_render(renderer* render) {
    vkWaitForFences(render->ctx->device->device, 1, &render->inFlight, VK_TRUE, UINT64_MAX);
    vkResetFences(render->ctx->device->device, 1, &render->inFlight);

    u32 imageIndex;
    vkAcquireNextImageKHR(render->ctx->device->device, render->ctx->swapchain->swapchain, UINT64_MAX, render->imageAvailable, VK_NULL_HANDLE, &imageIndex);

    global_data globalData;
    CLEAR_MEMORY(&globalData);
    vec3 eye = {2.0f, 2.0f, 2.0f};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 0.0f, 1.0f};
    glm_lookat(eye, center, up, &globalData.view);
    glm_perspective_default(640/480, &globalData.proj);
    globalData.proj[1][1] *= -1;

    vulkan_buffer_update(render->globalSetBuffer, sizeof(global_data), &globalData);

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

    vulkan_context_start_and_execute(render->ctx, &config, &info, (void*)_render);   

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