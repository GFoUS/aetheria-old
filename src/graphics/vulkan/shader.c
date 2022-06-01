#include "shader.h"

vulkan_shader* vulkan_shader_load_from_file(vulkan_device* device, const char* path, vulkan_shader_type type) {
    FILE* shaderFile = fopen(path, "rb");
    if (shaderFile == NULL) {
        FATAL("Could not open file: %s", path);
    }

    fseek(shaderFile, 0L, SEEK_END);
    u64 shaderFileSize = ftell(shaderFile);
    fseek(shaderFile, 0L, SEEK_SET);

    char* shaderCode = malloc(shaderFileSize);
    fread(shaderCode, shaderFileSize, 1, shaderFile);
    fclose(shaderFile);

    VkShaderModuleCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderFileSize;
    createInfo.pCode = (u32*)shaderCode;

    vulkan_shader* shader = malloc(sizeof(vulkan_shader));
    shader->device = device;
    shader->type = type;
    VkResult result = vkCreateShaderModule(device->device, &createInfo, NULL, &shader->module);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan shader module creation failed with error code %d", result);
    }
    free(shaderCode);

    return shader;
}

void vulkan_shader_destroy(vulkan_shader* shader) {
    vkDestroyShaderModule(shader->device->device, shader->module, NULL);
    free(shader);
}

VkPipelineShaderStageCreateInfo vulkan_shader_get_stage_info(vulkan_shader* shader) {
    VkPipelineShaderStageCreateInfo stageInfo;
    CLEAR_MEMORY(&stageInfo);

    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = (VkShaderStageFlagBits)shader->type;
    stageInfo.module = shader->module;
    stageInfo.pName = "main";

    return stageInfo;
}