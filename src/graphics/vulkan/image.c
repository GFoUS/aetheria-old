#include "image.h"

// This looks pointless but eventually it will manage mipmaps and multisampling
vulkan_image* vulkan_image_create_from_image(VkImage img, VkFormat format, u32 width, u32 height) {
    vulkan_image* image = malloc(sizeof(vulkan_image));

    image->image = img;
    image->format = format;
    image->width = width;
    image->height = height;

    return image;
}

void vulkan_image_destroy(vulkan_image* image) {
    free(image);
}