#include "pipeline.h"

VkPipelineVertexInputStateCreateInfo _vulkan_pipeline_vertex_input(vulkan_pipeline_config* config) {
    VkPipelineVertexInputStateCreateInfo vertexInfo;
    CLEAR_MEMORY(&vertexInfo);

    vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
}

vulkan_pipeline* vulkan_pipeline_create(vulkan_device* device, vulkan_pipeline_config* config) {

}

void vulkan_pipeline_destroy(vulkan_pipeline* pipeline);
