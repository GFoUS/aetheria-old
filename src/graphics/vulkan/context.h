#pragma once

#include "core/core.h"
#include "graphics/window.h"

#include "instance.h"
#include "physical.h"
#include "device.h"
#include "swapchain.h"
#include "renderpass.h"
#include "pipeline.h"
#include "command.h"

typedef struct {
	vulkan_instance*        instance;
	vulkan_physical_device* physical;
	vulkan_device*          device;

	VkSurfaceKHR surface;
	vulkan_swapchain* swapchain;
	vulkan_framebuffer** framebuffers;

	vulkan_renderpass* renderpass;
	vulkan_pipeline* pipeline;

	vulkan_command_pool* commandPool;

	window* win;
} vulkan_context;

vulkan_context* vulkan_context_create(window* win);
void vulkan_context_destroy(vulkan_context* ctx);

typedef struct {
	u32 numWaitSemaphores;
	VkSemaphore* waitSemaphores;
	VkPipelineStageFlags* waitStages;
	u32 numSignalSemaphores;
	VkSemaphore* signalSemaphores;
	VkFence fence;
} vulkan_context_submit_config;

void vulkan_context_start_and_execute(vulkan_context* ctx, vulkan_context_submit_config* submitConfig, void* data, void (*body)(VkCommandBuffer, void*));

VkSemaphore vulkan_context_get_semaphore(vulkan_context* ctx, VkSemaphoreCreateFlags flags);
VkFence vulkan_context_get_fence(vulkan_context* ctx, VkFenceCreateFlags flags);