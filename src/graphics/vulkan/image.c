#include "image.h"

#include "stb_image.h"
#include "buffer.h"

void _create_image_view(vulkan_image* image, VkImageAspectFlags aspects) {
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

    VkResult result = vkCreateImageView(image->ctx->device->device, &createInfo, NULL, &image->imageView);
    if (result != VK_SUCCESS) {
        FATAL("Image view creation failed with error code: %d", result);
    }
}

void _create_sampler(vulkan_image* image) {
    VkSamplerCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = image->ctx->physical->properties.limits.maxSamplerAnisotropy;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 0.0f;

    VkResult result = vkCreateSampler(image->ctx->device->device, &createInfo, NULL, &image->sampler);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan sampler creation failed with error code: %d", result);
    }
}

typedef struct {
    vulkan_image* image;
    VkImageLayout from;
    VkImageLayout to;
    VkImageAspectFlags aspects;
} _transition_layout_info;

void _transition_layout_body(VkCommandBuffer cmd, void* _info) {
    _transition_layout_info* info = (_transition_layout_info*)_info;
    VkImageMemoryBarrier barrier;
    CLEAR_MEMORY(&barrier);

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = info->from;
    barrier.newLayout = info->to;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = info->image->image;
    barrier.subresourceRange.aspectMask = info->aspects;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (info->from == VK_IMAGE_LAYOUT_UNDEFINED && info->to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (info->from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && info->to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (info->from == VK_IMAGE_LAYOUT_UNDEFINED && info->to == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        FATAL("Unsupported layout transition from %d to %d", info->from, info->to);
    }

    vkCmdPipelineBarrier(
        cmd,
        sourceStage, destinationStage,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
    );
}

void _transition_layout(vulkan_image* image, VkImageLayout from, VkImageLayout to, VkImageAspectFlags aspects) {
    _transition_layout_info info;
    info.image = image;
    info.from = from;
    info.to = to;
    info.aspects = aspects;

    vulkan_context_start_and_execute(image->ctx, NULL, &info, _transition_layout_body);
}

typedef struct {
    vulkan_image* dst;
    vulkan_buffer* src;
} _copy_buffer_to_image_info;

void _copy_buffer_to_image_body(VkCommandBuffer cmd, void* _info) {
    _copy_buffer_to_image_info* info = (_copy_buffer_to_image_info*)_info;
    VkBufferImageCopy region;
    CLEAR_MEMORY(&region);
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;

    region.imageExtent.width = info->dst->width;
    region.imageExtent.height = info->dst->height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(
        cmd,
        info->src->buffer,
        info->dst->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
}

void _copy_buffer_to_image(vulkan_image* dst, vulkan_buffer* src) {
    _copy_buffer_to_image_info info;
    info.dst = dst;
    info.src = src;
    vulkan_context_start_and_execute(dst->ctx, NULL, &info, _copy_buffer_to_image_body);
}

vulkan_image* vulkan_image_create(vulkan_context* ctx, VkFormat format, VkImageLayout layout, VkImageUsageFlags usage, u32 width, u32 height, VkImageAspectFlags aspects) {
    VkImageCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.extent.width = width;
    createInfo.extent.height = height;
    createInfo.extent.depth = 1;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.format = format;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo;
    CLEAR_MEMORY(&allocInfo);
    
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vulkan_image* image = malloc(sizeof(vulkan_image));
    image->ctx = ctx;
    image->format = format;
    image->ownsImage = true;
    image->width = width;
    image->height = height;
    
    VkResult result = vmaCreateImage(ctx->allocator, &createInfo, &allocInfo, &image->image, &image->allocation, NULL);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan image creation failed with error code: %d", result);
    }
    
    _transition_layout(image, VK_IMAGE_LAYOUT_UNDEFINED, layout, aspects);

    _create_image_view(image, aspects);
    _create_sampler(image);

    return image;
}

vulkan_image* vulkan_image_create_from_file(vulkan_context* ctx, const char* path, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspects) {
    i32 width, height, channels;
    u8* pixels = stbi_load(path, &width, &height, &channels, 4);
    if (!pixels) {
        FATAL("Failed to load texture: %s", path);
    }
    vulkan_buffer* buffer = vulkan_buffer_create_with_data(ctx, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, width * height * 4, pixels);
    stbi_image_free(pixels);

    VkImageCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.extent.width = width;
    createInfo.extent.height = height;
    createInfo.extent.depth = 1;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.format = format;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    VmaAllocationCreateInfo allocInfo;
    CLEAR_MEMORY(&allocInfo);
    
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vulkan_image* image = malloc(sizeof(vulkan_image));
    image->ctx = ctx;
    image->format = format;
    image->ownsImage = true;
    image->width = width;
    image->height = height;
    
    VkResult result = vmaCreateImage(ctx->allocator, &createInfo, &allocInfo, &image->image, &image->allocation, NULL);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan image creation failed with error code: %d", result);
    }

    _transition_layout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, aspects);
    _copy_buffer_to_image(image, buffer);
    _transition_layout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, aspects);
    vulkan_buffer_destroy(buffer);

    _create_image_view(image, aspects);
    _create_sampler(image);

    return image;
}

vulkan_image* vulkan_image_create_from_image(vulkan_context* ctx, VkImage img, VkFormat format, u32 width, u32 height, VkImageAspectFlags aspects) {
    vulkan_image* image = malloc(sizeof(vulkan_image));
    image->ctx = ctx;
    image->image = img;
    image->ownsImage = false;
    image->format = format;
    image->width = width;
    image->height = height;

    _create_image_view(image, aspects);
    _create_sampler(image);

    return image;
}

void vulkan_image_destroy(vulkan_image* image) {
    vkDestroySampler(image->ctx->device->device, image->sampler, NULL);
    vkDestroyImageView(image->ctx->device->device, image->imageView, NULL);
    if (image->ownsImage) {
        vmaDestroyImage(image->ctx->allocator, image->image, image->allocation);
    }
    free(image);
}