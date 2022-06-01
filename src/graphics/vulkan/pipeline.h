#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "device.h"
#include "shader.h"
#include "renderpass.h"

typedef struct {
    VkPipelineLayout layout;
    vulkan_device* device;
} vulkan_pipeline_layout;

typedef struct {
    i32 valueThatIsHereSoTheStructCanBeDefined;
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

    vulkan_renderpass* renderpass;
} vulkan_pipeline_config;

vulkan_pipeline* vulkan_pipeline_create(vulkan_device* device, vulkan_pipeline_config* config);
void vulkan_pipeline_destroy(vulkan_pipeline* pipeline);