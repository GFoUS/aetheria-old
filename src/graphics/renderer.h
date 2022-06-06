#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "vulkan/context.h"
#include "vulkan/buffer.h"
#include "vulkan/descriptor.h"
#include "vulkan/pipeline.h"
#include "vulkan/renderpass.h"
#include "vulkan/image.h"
#include "window.h"
#include "model.h"

typedef struct {
    vulkan_context* ctx;

    VkSemaphore imageAvailable;
    VkSemaphore renderFinished;
    VkFence inFlight;
    VkCommandBuffer cmd;

    vulkan_renderpass* renderpass;
	vulkan_pipeline* pipeline;
    vulkan_image* colorImage;
    vulkan_image* depthImage;
    vulkan_framebuffer** framebuffers;

    vulkan_descriptor_set_layout* globalSetLayout;
    vulkan_descriptor_allocator* globalSetAllocator;
    vulkan_buffer* globalSetBuffer;
    vulkan_descriptor_set* globalSet;

    model* model1;
    bool recreateSwapchain;
} renderer;

renderer* renderer_create(window* win);
void renderer_destroy(renderer* render);

void renderer_render(renderer* render);