#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"

#include "device.h"
#include "shader.h"

typedef struct {
    VkPipeline pipeline;
    vulkan_device* device;
} vulkan_pipeline;

typedef struct {
    vulkan_shader* vertexShader;
    vulkan_shader* fragmentShader;
} vulkan_pipeline_config;

vulkan_pipeline* vulkan_pipeline_create(vulkan_device* device, vulkan_pipeline_config* config);
void vulkan_pipeline_destroy(vulkan_pipeline* pipeline);