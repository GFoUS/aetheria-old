#include "device.h"

vulkan_device* vulkan_device_create(vulkan_instance* instance, vulkan_physical_device* physical, u32 numExtensions, const char** extensions, u32 numLayers, const char** layers) {
    // Graphics queue info
    u32 queueIndexUsage[256];
    CLEAR_MEMORY_ARRAY(queueIndexUsage, 256);
    queueIndexUsage[physical->queues.graphicsIndex]++;
    queueIndexUsage[physical->queues.presentIndex]++;

    u32 uniqueQueueIndices[256];
    u32 numUniqueQueueIndices = 0;
    CLEAR_MEMORY_ARRAY(uniqueQueueIndices, 256);
    for (u32 i = 0; i < 256; i++) {
        if (queueIndexUsage[i] != 0) {
            uniqueQueueIndices[numUniqueQueueIndices++] = i;
        }
    }

    VkDeviceQueueCreateInfo* queueInfos = malloc(sizeof(VkDeviceQueueCreateInfo) * numUniqueQueueIndices);
    CLEAR_MEMORY_ARRAY(queueInfos, numUniqueQueueIndices);
    for (u32 i = 0; i < numUniqueQueueIndices; i++) {
        float priorites[1] = { 1.0f };
        queueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfos[i].queueFamilyIndex = physical->queues.graphicsIndex;
        queueInfos[i].queueCount = 1;
        queueInfos[i].pQueuePriorities = priorites;
    }

    VkPhysicalDeviceFeatures deviceFeatures;
    CLEAR_MEMORY(&deviceFeatures);
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.independentBlend = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;

    // Device create info
    VkDeviceCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.enabledExtensionCount = numExtensions;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = numLayers;
    createInfo.ppEnabledLayerNames = layers;
    createInfo.queueCreateInfoCount = numUniqueQueueIndices;
    createInfo.pQueueCreateInfos = queueInfos;
    createInfo.pEnabledFeatures = &deviceFeatures;

    vulkan_device* device = malloc(sizeof(vulkan_device));
    VkResult result = vkCreateDevice(physical->physical, &createInfo, NULL, &device->device);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan device creation failed with error code %d", result);
    }
    free(queueInfos);

    vkGetDeviceQueue(device->device, physical->queues.graphicsIndex, 0, &device->graphics);
    vkGetDeviceQueue(device->device, physical->queues.presentIndex, 0, &device->present);

    device->physical = physical;

    return device;
}

void vulkan_device_destroy(vulkan_device* device) {
    vkDestroyDevice(device->device, NULL);
    free(device);
}