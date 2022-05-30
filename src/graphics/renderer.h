#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "vulkan/context.h"
#include "window.h"

typedef struct {
    vulkan_context* ctx;
    VkSemaphore imageAvailable;
    VkSemaphore renderFinished;
    VkFence inFlight;
} renderer;

renderer* renderer_create(window* win);
void renderer_destroy(renderer* render);

void renderer_render(renderer* render);