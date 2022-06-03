#include "vertex.h"

vulkan_vertex_info vulkan_vertex_get_info() {
    vulkan_vertex_info vertexInfo;
    CLEAR_MEMORY(&vertexInfo);

    vertexInfo.numAttributes = NUM_VERTEX_ATTRIBUTES;

    vertexInfo.attributes[0].binding = 0;
    vertexInfo.attributes[0].location = 0;
    vertexInfo.attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInfo.attributes[0].offset = offsetof(vulkan_vertex, position);
    
    vertexInfo.attributes[1].binding = 0;
    vertexInfo.attributes[1].location = 1;
    vertexInfo.attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    vertexInfo.attributes[1].offset = offsetof(vulkan_vertex, uv);

    vertexInfo.binding.binding = 0;
    vertexInfo.binding.stride = sizeof(vulkan_vertex);
    vertexInfo.binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return vertexInfo;
}