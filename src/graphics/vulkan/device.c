#include "device.h"

vulkan_device* vulkan_device_create(vulkan_instance* instance, vulkan_physical_device* physical, u32 numExtensions, const char** extensions, u32 numLayers, const char** layers) {
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
            FATAL("Missing device extension: %s", extensions[i]);
        }
    }

    // Graphics queue info
    VkDeviceQueueCreateInfo queueInfo;
    CLEAR_MEMORY(&queueInfo);

    float priorites[1] = { 1.0f };
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = physical->queues.graphicsIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = priorites;

    // Device create info
    VkDeviceCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.enabledExtensionCount = numExtensions;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = numLayers;
    createInfo.ppEnabledLayerNames = layers;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;

    vulkan_device* device = malloc(sizeof(vulkan_device));
    VkResult result = vkCreateDevice(physical->physical, &createInfo, NULL, &device->device);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan device creation failed with error code %d", result);
    }

    vkGetDeviceQueue(device->device, physical->queues.graphicsIndex, 0, &device->graphics);

    return device;
}

void vulkan_device_destroy(vulkan_device* device) {
    vkDestroyDevice(device->device, NULL);
    free(device);
}