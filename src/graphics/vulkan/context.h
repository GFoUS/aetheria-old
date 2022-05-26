#pragma once

#include "core/core.h"
#include "graphics/window.h"

#include "instance.h"
#include "physical.h"
#include "device.h"

typedef struct {
	vulkan_instance*        instance;
	vulkan_physical_device* physical;
	vulkan_device*          device;

	window* win;
} vulkan_context;

vulkan_context* vulkan_context_create(window* win);
void vulkan_context_destroy(vulkan_context* ctx);