#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "instance.h"
#include "physical.h"

typedef struct {
    VkDevice device;
    VkQueue graphics;
    VkQueue present;

    vulkan_physical_device* physical;
} vulkan_device;

vulkan_device* vulkan_device_create(vulkan_instance* instance, vulkan_physical_device* physical, u32 numExtensions, const char** extensions, u32 numLayers, const char** layers);
void vulkan_device_destroy(vulkan_device* device);
