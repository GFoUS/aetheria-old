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
#include "framegraph/framegraph.h"

typedef struct {
    vulkan_context* ctx;

    VkSemaphore imageAvailable;
    VkSemaphore renderFinished;
    VkFence inFlight;
    VkCommandBuffer cmd;

    gltf_gltf* gltf;
    model_model* model;

    bool recreateSwapchain;
} renderer;

renderer* renderer_create(window* win);
void renderer_destroy(renderer* render);

void renderer_render(renderer* render);