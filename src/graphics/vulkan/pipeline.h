#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "device.h"
#include "shader.h"
#include "renderpass.h"
#include "descriptor.h"

typedef struct {
    VkPipelineLayout layout;
    vulkan_device* device;
} vulkan_pipeline_layout;

typedef struct {
    u32 numSetLayouts;
    vulkan_descriptor_set_layout** setLayouts;
} vulkan_pipeline_layout_config;

vulkan_pipeline_layout* vulkan_pipeline_layout_create(vulkan_device* device, vulkan_pipeline_layout_config* config);
void vulkan_pipeline_layout_destroy(vulkan_pipeline_layout* layout);

typedef struct {
    VkPipeline pipeline;
    vulkan_device* device;
    vulkan_pipeline_layout* layout;
} vulkan_pipeline;

typedef struct {
    vulkan_shader* vertexShader;
    vulkan_shader* fragmentShader;

    u32 width;
    u32 height;

    u32 subpass;
    vulkan_renderpass* renderpass;
    
    u32 numSetLayouts;
    vulkan_descriptor_set_layout** setLayouts;

    u32 numBlendingAttachments;
    VkPipelineColorBlendAttachmentState* blendingAttachments;

    VkCullModeFlags rasterizerCullMode;
    VkSampleCountFlags samples;
} vulkan_pipeline_config;

vulkan_pipeline* vulkan_pipeline_create(vulkan_device* device, vulkan_pipeline_config* config);
void vulkan_pipeline_destroy(vulkan_pipeline* pipeline);

VkPipelineColorBlendAttachmentState vulkan_get_default_blending();