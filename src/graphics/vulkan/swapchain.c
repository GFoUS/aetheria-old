#include "swapchain.h"

VkSurfaceFormatKHR _pick_format(u32 numFormats, VkSurfaceFormatKHR* formats) {
    for (u32 i = 0; i < numFormats; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return formats[i];
        }
    }
    return formats[0];
}

VkPresentModeKHR _pick_mode(u32 numModes, VkPresentModeKHR* modes) {
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D _pick_extent(VkSurfaceCapabilitiesKHR* capabilities, window* win) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    } else {
        i32 width, height;
        glfwGetFramebufferSize(win->window, &width, &height);

        VkExtent2D actualExtent;
        actualExtent.width = clamp((u32)width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
        actualExtent.height = clamp((u32)height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);

        return actualExtent;
    }
}

vulkan_swapchain* vulkan_swapchain_create(vulkan_device* device, window* win, VkSurfaceKHR surface) {
    // Calculate image count
    u32 imageCount = device->physical->swapchain_details.capabilities.minImageCount + 1;
    if (device->physical->swapchain_details.capabilities.maxImageCount > 0 && imageCount > device->physical->swapchain_details.capabilities.maxImageCount) {
        imageCount = device->physical->swapchain_details.capabilities.maxImageCount;
    }

    VkSurfaceFormatKHR format = _pick_format(device->physical->swapchain_details.numFormats, device->physical->swapchain_details.formats);

    VkExtent2D extent = _pick_extent(&device->physical->swapchain_details.capabilities, win);

    VkSwapchainCreateInfoKHR createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    u32 queueFamilyIndices[] = { device->physical->queues.graphicsIndex, device->physical->queues.presentIndex };
    if (device->physical->queues.graphicsIndex != device->physical->queues.presentIndex) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = device->physical->swapchain_details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = _pick_mode(device->physical->swapchain_details.numModes, device->physical->swapchain_details.modes);
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    vulkan_swapchain* swapchain = malloc(sizeof(vulkan_swapchain));
    swapchain->device = device;
    swapchain->extent = extent;
    swapchain->format = format.format;

    VkResult result = vkCreateSwapchainKHR(device->device, &createInfo, NULL, &swapchain->swapchain);
    if (result != VK_SUCCESS) {
        FATAL("Swapchain creation failed with error code &d", result);
    }

    vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &swapchain->numImages, NULL);
    VkImage* images = malloc(sizeof(VkImage) * swapchain->numImages);
    swapchain->images = malloc(sizeof(vulkan_image*) * swapchain->numImages);
    vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &swapchain->numImages, images);

    for (u32 i = 0; i < swapchain->numImages; i++) {
        swapchain->images[i] = vulkan_image_create_from_image(device, images[i], swapchain->format, swapchain->extent.width, swapchain->extent.height, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    free(images);

    return swapchain;
}

void vulkan_swapchain_destroy(vulkan_swapchain* swapchain) {
    vkDestroySwapchainKHR(swapchain->device->device, swapchain->swapchain, NULL);
    for (u32 i = 0; i < swapchain->numImages; i++) {
        vulkan_image_destroy(swapchain->images[i]);
    } 
    free(swapchain->images);
    free(swapchain);
}