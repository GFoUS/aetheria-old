#include "renderer.h"

#include "vulkan/vertex.h"
#include "cglm/cglm.h"

typedef struct {
    mat4 view;
    mat4 proj;
} global_data;

void _create_swapchain(renderer* render) {
    vkDeviceWaitIdle(render->ctx->device->device);

    if (render->ctx->swapchain == NULL) {
        render->ctx->swapchain = vulkan_swapchain_create(render->ctx, render->ctx->win, render->ctx->surface);
    }

    vulkan_renderpass_builder* renderpassBuilder = vulkan_renderpass_builder_create();
    VkAttachmentDescription colorAttachmentDescription = vulkan_renderpass_get_default_color_attachment(render->ctx->swapchain->format, render->ctx->physical->maxSamples);
    vulkan_subpass_attachment colorAttachment = vulkan_renderpass_builder_add_attachment(renderpassBuilder, &colorAttachmentDescription);
    VkAttachmentDescription depthAttachmentDescription = vulkan_renderpass_get_default_depth_attachment(render->ctx->physical->maxSamples);
    vulkan_subpass_attachment depthAttachment = vulkan_renderpass_builder_add_attachment(renderpassBuilder, &depthAttachmentDescription);
    VkAttachmentDescription resolveAttachmentDescription = vulkan_renderpass_get_default_resolve_attachment(render->ctx->swapchain->format);
    vulkan_subpass_attachment resolveAttachment = vulkan_renderpass_builder_add_attachment(renderpassBuilder, &resolveAttachmentDescription);

    vulkan_subpass_config subpassConfig;
    CLEAR_MEMORY(&subpassConfig);
    subpassConfig.numColorAttachments = 1;
    subpassConfig.colorAttachments = &colorAttachment;
    subpassConfig.isDepthBuffered = true;
    subpassConfig.depthAttachment = depthAttachment;
    subpassConfig.isMultisampled = true;
    subpassConfig.resolveAttachment = resolveAttachment;
    vulkan_renderpass_builder_add_subpass(renderpassBuilder, &subpassConfig);

    render->renderpass = vulkan_renderpass_builder_build(renderpassBuilder, render->ctx->device);
	INFO("Created renderpass");

	vulkan_shader* vertexShader = vulkan_shader_load_from_file(render->ctx->device, "shaders/vert.spv", VERTEX);
	vulkan_shader* fragmentShader = vulkan_shader_load_from_file(render->ctx->device, "shaders/frag.spv", FRAGMENT);
    vulkan_descriptor_set_layout* layouts[2] = {render->globalSetLayout, render->materialSetLayout};
	vulkan_pipeline_config pipelineConfig;
	CLEAR_MEMORY(&pipelineConfig);
	
	pipelineConfig.vertexShader = vertexShader;
	pipelineConfig.fragmentShader = fragmentShader;
	pipelineConfig.width = render->ctx->swapchain->extent.width;
	pipelineConfig.height = render->ctx->swapchain->extent.height;
	pipelineConfig.renderpass = render->renderpass;
    pipelineConfig.numSetLayouts = 2;
    pipelineConfig.setLayouts = layouts;

	render->pipeline = vulkan_pipeline_create(render->ctx->device, &pipelineConfig);
	INFO("Created pipeline");

    vulkan_shader_destroy(vertexShader);
	vulkan_shader_destroy(fragmentShader);

    render->colorImage = vulkan_image_create(render->ctx,
                                            render->ctx->swapchain->format,
                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                            render->ctx->swapchain->extent.width,
                                            render->ctx->swapchain->extent.height,
                                            VK_IMAGE_ASPECT_COLOR_BIT,
                                            render->ctx->physical->maxSamples);
    render->depthImage = vulkan_image_create(render->ctx, 
                                            VK_FORMAT_D32_SFLOAT, 
                                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                                            render->ctx->swapchain->extent.width, 
                                            render->ctx->swapchain->extent.height, 
                                            VK_IMAGE_ASPECT_DEPTH_BIT,
                                            render->ctx->physical->maxSamples);

    render->framebuffers = malloc(sizeof(vulkan_framebuffer*) * render->ctx->swapchain->numImages);
	for (u32 i = 0; i < render->ctx->swapchain->numImages; i++) {
        vulkan_image* attachments[3] = {render->colorImage, render->depthImage, render->ctx->swapchain->images[i]};
		render->framebuffers[i] = vulkan_framebuffer_create(render->ctx->device, render->renderpass, 3, attachments);
	}

    if (render->model != NULL) {
        render->model->pipelineLayout = render->pipeline->layout; // Update pipeline layout in model
    }

	INFO("Created framebuffers");
}

renderer* renderer_create(window* win) {
    renderer* render = malloc(sizeof(renderer));
    CLEAR_MEMORY(render);
    render->ctx = vulkan_context_create(win); 
    
    render->imageAvailable = vulkan_context_get_semaphore(render->ctx, 0);
    render->renderFinished = vulkan_context_get_semaphore(render->ctx, 0);
    render->inFlight = vulkan_context_get_fence(render->ctx, VK_FENCE_CREATE_SIGNALED_BIT);  

    vulkan_descriptor_set_layout_builder* globalSetLayoutBuilder = vulkan_descriptor_set_layout_builder_create();
    vulkan_descriptor_set_layout_builder_add(globalSetLayoutBuilder, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    render->globalSetLayout = vulkan_descriptor_set_layout_builder_build(globalSetLayoutBuilder, render->ctx->device);
    render->globalSetAllocator = vulkan_descriptor_allocator_create(render->ctx->device, render->globalSetLayout);
    render->globalSet = vulkan_descriptor_set_allocate(render->globalSetAllocator);
    render->globalSetBuffer = vulkan_buffer_create(render->ctx, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(global_data));
    vulkan_descriptor_set_write_buffer(render->globalSet, 0, render->globalSetBuffer);

    vulkan_descriptor_set_layout_builder* materialSetLayoutBuilder = vulkan_descriptor_set_layout_builder_create();
    vulkan_descriptor_set_layout_builder_add(materialSetLayoutBuilder, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vulkan_descriptor_set_layout_builder_add(materialSetLayoutBuilder, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    render->materialSetLayout = vulkan_descriptor_set_layout_builder_build(materialSetLayoutBuilder, render->ctx->device);

    _create_swapchain(render);

    render->gltf = gltf_load_file("models/samples/2.0/Duck/glTF/Duck.gltf");
    render->model = model_load_from_gltf(render->ctx, render->gltf, render->materialSetLayout, render->pipeline->layout);

    return render;
}

void _destroy_swapchain(renderer* render, bool destroySwapchain) {
    for (u32 i = 0; i < render->ctx->swapchain->numImages; i++) {
		vulkan_framebuffer_destroy(render->framebuffers[i]);
	}
	free(render->framebuffers);

    vulkan_image_destroy(render->colorImage);
    vulkan_image_destroy(render->depthImage);

    vulkan_pipeline_destroy(render->pipeline);
	vulkan_renderpass_destroy(render->renderpass);

    if (destroySwapchain) {
        vulkan_swapchain_destroy(render->ctx->swapchain);
        render->ctx->swapchain = NULL;
    }
}

void renderer_destroy(renderer* render) {
    _destroy_swapchain(render, false);  

    model_unload(render->model);
    gltf_unload(render->gltf);  

    vulkan_descriptor_allocator_destroy(render->globalSetAllocator);
    vulkan_descriptor_set_layout_destroy(render->globalSetLayout);
    vulkan_descriptor_set_layout_destroy(render->materialSetLayout);

    vkDestroySemaphore(render->ctx->device->device, render->imageAvailable, NULL);
    vkDestroySemaphore(render->ctx->device->device, render->renderFinished, NULL);
    vkDestroyFence(render->ctx->device->device, render->inFlight, NULL);

    vulkan_buffer_destroy(render->globalSetBuffer);

    vulkan_context_destroy(render->ctx);
    free(render);
}

typedef struct {
    renderer* render;
    u32 swapchainIndex;
} render_info;

void _render(VkCommandBuffer cmd, render_info* info) {
    vulkan_renderpass_bind(cmd, info->render->renderpass, info->render->framebuffers[info->swapchainIndex]);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info->render->pipeline->pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info->render->pipeline->layout->layout, 0, 1, &info->render->globalSet->set, 0, NULL);

    model_render(info->render->model, cmd);

    vkCmdEndRenderPass(cmd);
}

void renderer_render(renderer* render) {
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
        _destroy_swapchain(render, true);
        _create_swapchain(render);
        render->recreateSwapchain = false;
        return;
    }

    vkResetFences(render->ctx->device->device, 1, &render->inFlight);

    if (render->cmd != NULL) {
        vulkan_command_pool_free_buffer(render->ctx->commandPool, render->cmd);
    }

    global_data globalData;
    CLEAR_MEMORY(&globalData);
    static float i = 0.0f;
    i += 1.0f;
    vec3 eye = {i, i, i};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 0.0f, 1.0f};
    glm_lookat(eye, center, up, globalData.view);
    glm_perspective(45.0f, (float)render->ctx->swapchain->extent.width / (float)render->ctx->swapchain->extent.height, 1.0f, 10000.0f, globalData.proj);
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

    render->cmd = vulkan_context_start_and_execute(render->ctx, &config, &info, (void*)_render);   

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