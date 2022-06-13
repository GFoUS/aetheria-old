#include "physical.h"

#define VK_QUEUE_PRESENT_BIT 0x00000080

vulkan_physical_device* get_all(vulkan_instance* instance, VkSurfaceKHR surface, u32* numPhysicalDevices) {
    vkEnumeratePhysicalDevices(instance->instance, numPhysicalDevices, NULL);
    VkPhysicalDevice* vkPhysicalDevices = malloc(sizeof(VkPhysicalDevice) * *numPhysicalDevices);
    vkEnumeratePhysicalDevices(instance->instance, numPhysicalDevices, vkPhysicalDevices);

    vulkan_physical_device* physicalDevices = malloc(sizeof(vulkan_physical_device) * *numPhysicalDevices);
    memset(physicalDevices, 0, sizeof(vulkan_physical_device) * *numPhysicalDevices);

    for (u32 i = 0; i < *numPhysicalDevices; i++) {
        // Get base properties
        physicalDevices[i].physical = vkPhysicalDevices[i]; 
        vkGetPhysicalDeviceProperties(physicalDevices[i].physical, &physicalDevices[i].properties);
        vkGetPhysicalDeviceFeatures(physicalDevices[i].physical, &physicalDevices[i].features);

        // Get queue info
        u32 numQueueFamilies;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i].physical, &numQueueFamilies, NULL);
        VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * numQueueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i].physical, &numQueueFamilies, queueFamilies);
        for (u32 j = 0; j < numQueueFamilies; j++) {
            if (!(physicalDevices[i].queues.found & VK_QUEUE_GRAPHICS_BIT) && queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                physicalDevices[i].queues.found |= VK_QUEUE_GRAPHICS_BIT;
                physicalDevices[i].queues.graphicsIndex = j;
            }

            VkBool32 presentSupport;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i].physical, j, surface, &presentSupport);
            if (!(physicalDevices[i].queues.found & VK_QUEUE_PRESENT_BIT) && presentSupport) {
                physicalDevices[i].queues.found |= VK_QUEUE_PRESENT_BIT;
                physicalDevices[i].queues.presentIndex = j;
            }
        }
        free(queueFamilies);

        // Get swapchain info
        vulkan_physical_device_swapchain_details* swapchain_details = &physicalDevices[i].swapchain_details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[i].physical, surface, &swapchain_details->capabilities);

        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i].physical, surface, &swapchain_details->numFormats, NULL);
        physicalDevices[i].swapchain_details.formats = malloc(sizeof(VkSurfaceFormatKHR) * swapchain_details->numFormats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i].physical, surface, &swapchain_details->numFormats, swapchain_details->formats);

        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[i].physical, surface, &swapchain_details->numModes, NULL);
        physicalDevices[i].swapchain_details.modes = malloc(sizeof(VkPresentModeKHR) * swapchain_details->numModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[i].physical, surface, &swapchain_details->numModes, swapchain_details->modes);
    
        VkSampleCountFlags counts = physicalDevices[i].properties.limits.framebufferColorSampleCounts & physicalDevices[i].properties.limits.framebufferDepthSampleCounts;
        for (u32 j = (u32)VK_SAMPLE_COUNT_64_BIT; j > 0; j >>= 1) {
            if (counts & j) {
                physicalDevices[i].maxSamples = (VkSampleCountFlagBits)j;
                break;
            }
        }
    }

    free(vkPhysicalDevices);

    return physicalDevices;
}

bool has_extensions(vulkan_physical_device* physical, u32 numExtensions, const char** extensions) {
    // Check extensions
    u32 numAvailableExtensions;
    vkEnumerateDeviceExtensionProperties(physical->physical, NULL, &numAvailableExtensions, NULL);
    VkExtensionProperties* availableExtensions = malloc(sizeof(VkExtensionProperties) * numAvailableExtensions);
    vkEnumerateDeviceExtensionProperties(physical->physical, NULL, &numAvailableExtensions, availableExtensions);

    for (u32 i = 0; i < numExtensions; i++) {
        bool found = false;
        for (u32 j = 0; j < numAvailableExtensions; j++) {
            if (strcmp(extensions[i], availableExtensions[j].extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    free(availableExtensions);

    return true;
}

bool supports_swapchain(vulkan_physical_device* physical) {
    return physical->swapchain_details.numFormats != 0 && physical->swapchain_details.numModes != 0;
}

bool is_suitable(vulkan_physical_device* physical, u32 numExtensions, const char** extensions) {
    bool queuesComplete = physical->queues.found == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_PRESENT_BIT);
    return queuesComplete && 
            has_extensions(physical, numExtensions, extensions) && 
            supports_swapchain(physical) && 
            physical->features.samplerAnisotropy && 
            physical->features.independentBlend &&
            physical->features.sampleRateShading;
}

vulkan_physical_device* vulkan_physical_device_create(vulkan_instance* instance, VkSurfaceKHR surface, u32 numExtensions, const char** extensions) {
    u32 numPhysicalDevices;
    vulkan_physical_device* physicalDevices = get_all(instance, surface, &numPhysicalDevices);
    vulkan_physical_device* physical = malloc(sizeof(vulkan_physical_device));

    bool found = false;
    for (u32 i = 0; i < numPhysicalDevices; i++) {
        if (is_suitable(&physicalDevices[i], numExtensions, extensions)) {
            memcpy(physical, &physicalDevices[i], sizeof(vulkan_physical_device));
            found = true;
            break;
        }
    }
    if (!found) {
        FATAL("No suitable GPU found");
    }

    free(physicalDevices);
    return physical;
}

void vulkan_physical_device_destroy(vulkan_physical_device* physical) {
    free(physical->swapchain_details.formats);
    free(physical->swapchain_details.modes);
    free(physical);
}