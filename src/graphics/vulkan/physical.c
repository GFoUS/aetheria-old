#include "physical.h"

vulkan_physical_device* _get_all(vulkan_instance* instance, u32* numPhysicalDevices) {
    vkEnumeratePhysicalDevices(instance->instance, numPhysicalDevices, NULL);
    VkPhysicalDevice* vkPhysicalDevices = malloc(sizeof(VkPhysicalDevice) * *numPhysicalDevices);
    vkEnumeratePhysicalDevices(instance->instance, numPhysicalDevices, vkPhysicalDevices);

    vulkan_physical_device* physicalDevices = malloc(sizeof(vulkan_physical_device) * *numPhysicalDevices);
    memset(physicalDevices, 0, sizeof(vulkan_physical_device) * *numPhysicalDevices);

    for (u32 i = 0; i < *numPhysicalDevices; i++) {
        physicalDevices[i].physical = vkPhysicalDevices[i]; 
        vkGetPhysicalDeviceProperties(physicalDevices[i].physical, &physicalDevices[i].properties);
        vkGetPhysicalDeviceFeatures(physicalDevices[i].physical, &physicalDevices[i].features);

        u32 numQueueFamilies;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i].physical, &numQueueFamilies, NULL);
        VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * numQueueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i].physical, &numQueueFamilies, queueFamilies);
        for (u32 j = 0; j < numQueueFamilies; j++) {
            if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                physicalDevices[i].queues.found |= VK_QUEUE_GRAPHICS_BIT;
                physicalDevices[i].queues.graphicsIndex = j;
            }
        }
    }

    return physicalDevices;
}

bool _is_suitable(vulkan_physical_device* physical) {
    bool queuesComplete = physical->queues.found == VK_QUEUE_GRAPHICS_BIT;
    return queuesComplete;
}

vulkan_physical_device* vulkan_physical_device_create(vulkan_instance* instance) {
    u32 numPhysicalDevices;
    vulkan_physical_device* physicalDevices = _get_all(instance, &numPhysicalDevices);
    vulkan_physical_device* physical = malloc(sizeof(vulkan_physical_device));

    bool found = false;
    for (u32 i = 0; i < numPhysicalDevices; i++) {
        if (_is_suitable(&physicalDevices[i])) {
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
    free(physical);
}