#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

typedef struct {
	VkInstance instance;
} vulkan_instance;

vulkan_instance* vulkan_instance_create(u32 numExtensions, const char** extensions, u32 numLayers, const char** layers);
void vulkan_instance_destroy(vulkan_instance* instance);