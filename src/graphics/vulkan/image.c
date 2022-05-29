#include "image.h"

void _create_image_view(vulkan_device* device, vulkan_image* image, VkImageAspectFlags aspects) {
    VkImageViewCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image->image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = image->format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspects;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkResult result = vkCreateImageView(device->device, &createInfo, NULL, &image->imageView);
    if (result != VK_SUCCESS) {
        FATAL("Image view creation failed with error code: %d", result);
    }
}

vulkan_image* vulkan_image_create_from_image(vulkan_device* device, VkImage img, VkFormat format, u32 width, u32 height, VkImageAspectFlags aspects) {
    vulkan_image* image = malloc(sizeof(vulkan_image));
    image->device = device;
    image->image = img;
    image->ownsImage = false;
    image->format = format;
    image->width = width;
    image->height = height;

    _create_image_view(device, image, aspects);

    return image;
}

void vulkan_image_destroy(vulkan_image* image) {
    vkDestroyImageView(image->device->device, image->imageView, NULL);
    if (image->ownsImage) {
        vkDestroyImage(image->device->device, image->image, NULL);
    }
    free(image);
}