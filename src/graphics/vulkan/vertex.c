#include "vertex.h"

vulkan_vertex_info vulkan_vertex_get_info() {
    vulkan_vertex_info vertexInfo;
    CLEAR_MEMORY(&vertexInfo);

    vertexInfo.numAttributes = NUM_VERTEX_ATTRIBUTES;

    vertexInfo.attributes[0].binding = 0;
    vertexInfo.attributes[0].location = 0;
    vertexInfo.attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInfo.attributes[0].offset = 0;
    
    vertexInfo.attributes[1].binding = 1;
    vertexInfo.attributes[1].location = 1;
    vertexInfo.attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInfo.attributes[1].offset = 0;

    vertexInfo.attributes[2].binding = 2;
    vertexInfo.attributes[2].location = 2;
    vertexInfo.attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertexInfo.attributes[2].offset = 0;

    vertexInfo.bindings[0].binding = 0;
    vertexInfo.bindings[0].stride = sizeof(vec3);
    vertexInfo.bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInfo.bindings[1].binding = 1;
    vertexInfo.bindings[1].stride = sizeof(vec3);
    vertexInfo.bindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInfo.bindings[2].binding = 2;
    vertexInfo.bindings[2].stride = sizeof(vec2);
    vertexInfo.bindings[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    return vertexInfo;
}