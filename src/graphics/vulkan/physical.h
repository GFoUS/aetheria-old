#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "instance.h"

typedef struct {
    u32 found;
    u32 graphicsIndex;
    u32 presentIndex;
} vulkan_physical_device_queues;

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    u32 numFormats;
    VkSurfaceFormatKHR* formats;
    u32 numModes;
    VkPresentModeKHR* modes;
} vulkan_physical_device_swapchain_details;

typedef struct {
    VkPhysicalDevice physical;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkSampleCountFlagBits maxSamples;

    vulkan_physical_device_queues queues;
    vulkan_physical_device_swapchain_details swapchain_details;
} vulkan_physical_device;

vulkan_physical_device* vulkan_physical_device_create(vulkan_instance* instance, VkSurfaceKHR surface, u32 numExtensions, const char** extensions);
void vulkan_physical_device_destroy(vulkan_physical_device* physical);

