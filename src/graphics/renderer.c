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
    VkAttachmentDescription accumDescription = vulkan_renderpass_get_default_color_attachment(VK_FORMAT_R16G16B16A16_SFLOAT, render->ctx->physical->maxSamples);
    vulkan_subpass_attachment accumAttachment = vulkan_renderpass_builder_add_attachment(renderpassBuilder, &accumDescription);
    VkAttachmentDescription revealDescription = vulkan_renderpass_get_default_color_attachment(VK_FORMAT_R16_SFLOAT, render->ctx->physical->maxSamples);
    vulkan_subpass_attachment revealAttachment = vulkan_renderpass_builder_add_attachment(renderpassBuilder, &revealDescription);
   
    VkAttachmentDescription depthDescription = vulkan_renderpass_get_default_depth_attachment(render->ctx->physical->maxSamples);
    vulkan_subpass_attachment depthAttachment = vulkan_renderpass_builder_add_attachment(renderpassBuilder, &depthDescription);
    
    VkAttachmentDescription compositeDescription = vulkan_renderpass_get_default_color_attachment(render->ctx->swapchain->format, render->ctx->physical->maxSamples);
    vulkan_subpass_attachment compositeAttachment = vulkan_renderpass_builder_add_attachment(renderpassBuilder, &compositeDescription);

    VkAttachmentDescription resolveDescription = vulkan_renderpass_get_default_resolve_attachment(render->ctx->swapchain->format);
    vulkan_subpass_attachment resolveAttachment = vulkan_renderpass_builder_add_attachment(renderpassBuilder, &resolveDescription);

    vulkan_subpass_config renderSubpassConfig;
    CLEAR_MEMORY(&renderSubpassConfig);
    renderSubpassConfig.numColorAttachments = 2;
    vulkan_subpass_attachment colorAttachments[2] = {accumAttachment, revealAttachment};
    renderSubpassConfig.colorAttachments = colorAttachments;
    renderSubpassConfig.isDepthBuffered = false;
    renderSubpassConfig.depthAttachment = depthAttachment;
    renderSubpassConfig.isResolving = false;
    vulkan_renderpass_builder_add_subpass(renderpassBuilder, &renderSubpassConfig);

    vulkan_subpass_config compositeSubpassConfig;
    CLEAR_MEMORY(&renderSubpassConfig);
    compositeSubpassConfig.numInputAttachments = 2;
    compositeSubpassConfig.inputAttachments = colorAttachments;
    compositeSubpassConfig.numColorAttachments = 1;
    compositeSubpassConfig.colorAttachments = &compositeAttachment;
    compositeSubpassConfig.isDepthBuffered = true;
    compositeSubpassConfig.depthAttachment = depthAttachment;
    compositeSubpassConfig.isResolving = true;
    compositeSubpassConfig.resolveAttachment = resolveAttachment;
    vulkan_renderpass_builder_add_subpass(renderpassBuilder, &compositeSubpassConfig);

    render->renderpass = vulkan_renderpass_builder_build(renderpassBuilder, render->ctx->device);
	INFO("Created renderpass");

	vulkan_shader* renderVertexShader = vulkan_shader_load_from_file(render->ctx->device, "shaders/vert.spv", VERTEX);
	vulkan_shader* renderFragmentShader = vulkan_shader_load_from_file(render->ctx->device, "shaders/frag.spv", FRAGMENT);
    vulkan_descriptor_set_layout* renderLayouts[2] = {render->globalSetLayout, render->materialSetLayout};
	vulkan_pipeline_config renderPipelineConfig;
	CLEAR_MEMORY(&renderPipelineConfig);
	
	renderPipelineConfig.vertexShader = renderVertexShader;
	renderPipelineConfig.fragmentShader = renderFragmentShader;
	renderPipelineConfig.width = render->ctx->swapchain->extent.width;
	renderPipelineConfig.height = render->ctx->swapchain->extent.height;
	renderPipelineConfig.renderpass = render->renderpass;
    renderPipelineConfig.subpass = 0;
    renderPipelineConfig.numSetLayouts = 2;
    renderPipelineConfig.setLayouts = renderLayouts;
    renderPipelineConfig.samples = render->ctx->physical->maxSamples;

    VkPipelineColorBlendAttachmentState blendingAttachments[2];
    CLEAR_MEMORY_ARRAY(blendingAttachments, 2);

    blendingAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendingAttachments[0].blendEnable = VK_TRUE;
    blendingAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendingAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendingAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
    blendingAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendingAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendingAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;

    blendingAttachments[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendingAttachments[1].blendEnable = VK_TRUE;
    blendingAttachments[1].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendingAttachments[1].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    blendingAttachments[1].colorBlendOp = VK_BLEND_OP_ADD;
    blendingAttachments[1].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendingAttachments[1].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendingAttachments[1].alphaBlendOp = VK_BLEND_OP_ADD;

    renderPipelineConfig.numBlendingAttachments = 2;
    renderPipelineConfig.blendingAttachments = blendingAttachments;
    renderPipelineConfig.rasterizerCullMode = VK_CULL_MODE_BACK_BIT;

	render->renderPipeline = vulkan_pipeline_create(render->ctx->device, &renderPipelineConfig);
	INFO("Created render pipeline");

    vulkan_shader_destroy(renderVertexShader);
	vulkan_shader_destroy(renderFragmentShader);

    vulkan_shader* compositeVertexShader = vulkan_shader_load_from_file(render->ctx->device, "shaders/composite.vert.spv", VERTEX);
	vulkan_shader* compositeFragmentShader = vulkan_shader_load_from_file(render->ctx->device, "shaders/composite.frag.spv", FRAGMENT);
    vulkan_descriptor_set_layout* compositeLayouts[2] = {render->compositeSetLayout};
	vulkan_pipeline_config compositePipelineConfig;
	CLEAR_MEMORY(&compositePipelineConfig);

    VkPipelineColorBlendAttachmentState defaultBlendingAttachment = vulkan_get_default_blending();
	
	compositePipelineConfig.vertexShader = compositeVertexShader;
	compositePipelineConfig.fragmentShader = compositeFragmentShader;
	compositePipelineConfig.width = render->ctx->swapchain->extent.width;
	compositePipelineConfig.height = render->ctx->swapchain->extent.height;
	compositePipelineConfig.renderpass = render->renderpass;
    compositePipelineConfig.subpass = 1;
    compositePipelineConfig.numSetLayouts = 1;
    compositePipelineConfig.setLayouts = compositeLayouts;
    compositePipelineConfig.numBlendingAttachments = 1;
    compositePipelineConfig.blendingAttachments = &defaultBlendingAttachment;
    compositePipelineConfig.rasterizerCullMode = VK_CULL_MODE_FRONT_BIT;
    compositePipelineConfig.samples = render->ctx->physical->maxSamples;

	render->compositePipeline = vulkan_pipeline_create(render->ctx->device, &compositePipelineConfig);
	INFO("Created composite pipeline");

    vulkan_shader_destroy(compositeVertexShader);
    vulkan_shader_destroy(compositeFragmentShader);

    render->accumImage = vulkan_image_create(render->ctx,
                                            VK_FORMAT_R16G16B16A16_SFLOAT,
                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                            render->ctx->swapchain->extent.width,
                                            render->ctx->swapchain->extent.height,
                                            VK_IMAGE_ASPECT_COLOR_BIT,
                                            render->ctx->physical->maxSamples);
    render->revealImage = vulkan_image_create(render->ctx,
                                            VK_FORMAT_R16_SFLOAT,
                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                                            render->ctx->swapchain->extent.width,
                                            render->ctx->swapchain->extent.height,
                                            VK_IMAGE_ASPECT_COLOR_BIT,
                                            render->ctx->physical->maxSamples);
    render->compositeImage = vulkan_image_create(render->ctx,
                                            render->ctx->swapchain->format,
                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
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
        vulkan_image* attachments[5] = {render->accumImage, render->revealImage, render->depthImage, render->compositeImage, render->ctx->swapchain->images[i]};
		render->framebuffers[i] = vulkan_framebuffer_create(render->ctx->device, render->renderpass, 5, attachments);
	}

    if (render->model != NULL) {
        render->model->pipelineLayout = render->renderPipeline->layout; // Update pipeline layout in model
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

    vulkan_descriptor_set_layout_builder* compositeSetLayoutBuilder = vulkan_descriptor_set_layout_builder_create();
    vulkan_descriptor_set_layout_builder_add(compositeSetLayoutBuilder, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
    vulkan_descriptor_set_layout_builder_add(compositeSetLayoutBuilder, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
    render->compositeSetLayout = vulkan_descriptor_set_layout_builder_build(compositeSetLayoutBuilder, render->ctx->device);
    render->compositeSetAllocator = vulkan_descriptor_allocator_create(render->ctx->device, render->compositeSetLayout);
    render->compositeSet = vulkan_descriptor_set_allocate(render->compositeSetAllocator);

    _create_swapchain(render);
        
    vulkan_descriptor_set_write_input_attachment(render->compositeSet, 0, render->accumImage);
    vulkan_descriptor_set_write_input_attachment(render->compositeSet, 1, render->revealImage);

    render->gltf = gltf_load_file("models/samples/2.0/Sponza/glTF/Sponza.gltf");
    render->model = model_load_from_gltf(render->ctx, render->gltf, render->materialSetLayout, render->renderPipeline->layout);

    return render;
}

void _destroy_swapchain(renderer* render, bool destroySwapchain) {
    for (u32 i = 0; i < render->ctx->swapchain->numImages; i++) {
		vulkan_framebuffer_destroy(render->framebuffers[i]);
	}
	free(render->framebuffers);

    vulkan_image_destroy(render->accumImage);
    vulkan_image_destroy(render->revealImage);
    vulkan_image_destroy(render->compositeImage);
    vulkan_image_destroy(render->depthImage);

    vulkan_pipeline_destroy(render->renderPipeline);
    vulkan_pipeline_destroy(render->compositePipeline);
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
    vulkan_descriptor_allocator_destroy(render->compositeSetAllocator);
    vulkan_descriptor_set_layout_destroy(render->globalSetLayout);
    vulkan_descriptor_set_layout_destroy(render->materialSetLayout);
    vulkan_descriptor_set_layout_destroy(render->compositeSetLayout);

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
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info->render->renderPipeline->pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info->render->renderPipeline->layout->layout, 0, 1, &info->render->globalSet->set, 0, NULL);

    model_render(info->render->model, cmd);

    vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info->render->compositePipeline->pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info->render->compositePipeline->layout->layout, 0, 1, &info->render->compositeSet->set, 0, NULL);
    vkCmdDraw(cmd, 3, 1, 0, 0);

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