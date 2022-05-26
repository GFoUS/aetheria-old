#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "instance.h"

typedef struct {
    VkQueueFlags found;
    u32 graphicsIndex;
} vulkan_physical_device_queues;

typedef struct {
    VkPhysicalDevice physical;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    vulkan_physical_device_queues queues;
} vulkan_physical_device;

vulkan_physical_device* vulkan_physical_device_create(vulkan_instance* instance);
void vulkan_physical_device_destroy(vulkan_physical_device* physical);

