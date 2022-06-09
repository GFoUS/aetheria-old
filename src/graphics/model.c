#include "model.h"

VkFilter gltf_filter_to_vk_filter(gltf_sampler_filter filter) {
    switch (filter) {
        case(SAMPLER_FILTER_LINEAR) : return VK_FILTER_LINEAR;
        case(SAMPLER_FILTER_NEAREST) : return VK_FILTER_NEAREST;
    }

    return VK_FILTER_LINEAR;
}

VkSamplerAddressMode gltf_wrap_mode_to_vk_address_mode(gltf_sampler_wrap_mode wrapMode) {
    switch (wrapMode) {
        case(SAMPLER_WRAP_MODE_CLAMP_TO_EDGE) : return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case(SAMPLER_WRAP_MODE_MIRRORED_REPEAT) : return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case(SAMPLER_WRAP_MODE_REPEAT) : return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }

    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

model_model* model_load_from_gltf(vulkan_context* ctx, gltf_gltf* gltf, vulkan_descriptor_set_layout* materialSetLayout, vulkan_pipeline_layout* pipelineLayout) {
    model_model* model = malloc(sizeof(model_model));
    model->ctx = ctx;
    model->gltf = gltf;
    model->pipelineLayout = pipelineLayout;
    model->materialSetLayout = materialSetLayout;
    model->materialSetAllocator = vulkan_descriptor_allocator_create(ctx->device, model->materialSetLayout);

    // Upload buffers to the GPU
    model->buffers = malloc(sizeof(vulkan_buffer*) * model->gltf->numBuffers);
    CLEAR_MEMORY_ARRAY(model->buffers, model->gltf->numBuffers);

    for (u32 i = 0; i < model->gltf->numBuffers; i++) {
        model->buffers[i] = vulkan_buffer_create_with_data(ctx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, model->gltf->buffers[i].byteLength, model->gltf->buffers[i].data);
    }

    // Upload images to the GPU
    model->images = malloc(sizeof(vulkan_image*) * model->gltf->numImages);
    CLEAR_MEMORY_ARRAY(model->images, model->gltf->numImages);

    for (u32 i = 0; i < model->gltf->numImages; i++) {
        model->images[i] = vulkan_image_create_from_file(ctx, gltf_merge_paths(model->gltf->path, model->gltf->images[i].uri), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT); // TODO: Format and aspects for other types of images
    }

    model->samplers = malloc(sizeof(vulkan_sampler*) * model->gltf->numSamplers);
    CLEAR_MEMORY_ARRAY(model->samplers, model->gltf->numSamplers);

    for (u32 i = 0; i < model->gltf->numSamplers; i++) {
        model->samplers[i] = vulkan_sampler_create(ctx, 
            gltf_filter_to_vk_filter(model->gltf->samplers[i].magFilter),
            gltf_filter_to_vk_filter(model->gltf->samplers[i].minFilter),
            gltf_wrap_mode_to_vk_address_mode(model->gltf->samplers[i].wrapS),
            gltf_wrap_mode_to_vk_address_mode(model->gltf->samplers[i].wrapT));
    }

    // Prepare material descriptor sets
    model->materialSets = malloc(sizeof(vulkan_descriptor_set*) * model->gltf->numMaterials);
    CLEAR_MEMORY_ARRAY(model->materialSets, model->gltf->numMaterials);
    model->materialDataBuffers = malloc(sizeof(vulkan_buffer*) * model->gltf->numMaterials);
    CLEAR_MEMORY_ARRAY(model->materialDataBuffers, model->gltf->numMaterials);

    for (u32 i = 0; i < model->gltf->numMaterials; i++) {
        model->materialSets[i] = vulkan_descriptor_set_allocate(model->materialSetAllocator);

        model_material_data data;
        CLEAR_MEMORY(&data);
        memcpy(data.baseColorFactor, model->gltf->materials[i].pbr.baseColorFactor, sizeof(float) * 4);
        model->materialDataBuffers[i] = vulkan_buffer_create_with_data(ctx, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(model_material_data), &data);
        vulkan_descriptor_set_write_buffer(model->materialSets[i], 0, model->materialDataBuffers[i]);

        vulkan_descriptor_set_write_image(model->materialSets[i], 1, model->images[model->gltf->materials[i].pbr.baseColorTexture.texture->source->id], model->samplers[model->gltf->materials[i].pbr.baseColorTexture.texture->sampler->id]);
    }

    return model;
}
void model_unload(model_model* model) {
    for (u32 i = 0; i < model->gltf->numBuffers; i++) {
        vulkan_buffer_destroy(model->buffers[i]);
    }
    free(model->buffers);

    for (u32 i = 0; i < model->gltf->numImages; i++) {
        vulkan_image_destroy(model->images[i]);
    }
    free(model->images);

    for (u32 i = 0; i < model->gltf->numMaterials; i++) {
        vulkan_buffer_destroy(model->materialDataBuffers[i]);
        vulkan_descriptor_set_free(model->materialSets[i]);
    }
    free(model->materialDataBuffers);
    free(model->materialSets);

    for (u32 i = 0; i < model->gltf->numSamplers; i++) {
        vulkan_sampler_destroy(model->samplers[i]);
    }
    free(model->samplers);

    vulkan_descriptor_allocator_destroy(model->materialSetAllocator);

    free(model);
}

void node_render(gltf_node* node, VkCommandBuffer cmd, model_model* model) {
    if (node->mesh) {
        for (u32 i = 0; i < node->mesh->numPrimitives; i++) {
            gltf_mesh_primitive* primitive = &node->mesh->primitives[i];

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, model->pipelineLayout->layout, 1, 1, &model->materialSets[primitive->material->id]->set, 0, NULL);

            // Load vertex and index buffers
            gltf_accessor* accessors[3] = {
                primitive->position,
                primitive->normal,
                primitive->baseColorTextureUV
            };

            VkBuffer buffers[3];
            VkDeviceSize offsets[3];

            for (u32 i = 0; i < 3; i++) {
                buffers[i] = model->buffers[accessors[i]->bufferView->buffer->id]->buffer;
                offsets[i] = gltf_get_accessor_offset(accessors[i]);
            }

            vkCmdBindVertexBuffers(cmd, 0, 3, buffers, offsets);

            VkIndexType indexType;
            switch (primitive->index->componentType) {
            case(ACCESSOR_COMPONENT_TYPE_U16) : indexType = VK_INDEX_TYPE_UINT16; break;
            case(ACCESSOR_COMPONENT_TYPE_U32) : indexType = VK_INDEX_TYPE_UINT32; break;
            }
            vkCmdBindIndexBuffer(cmd, model->buffers[primitive->index->bufferView->buffer->id]->buffer, gltf_get_accessor_offset(primitive->index), indexType);
            vkCmdDrawIndexed(cmd, (u32)primitive->index->count, 1, 0, 0, 0);
        }
    }

    for (u32 i = 0; i < node->numChildren; i++) {
        node_render(node->children[i], cmd, model);
    }
}

void model_render(model_model* model, VkCommandBuffer cmd) {
    for (u32 i = 0; i < model->gltf->scene->numNodes; i++) {
        node_render(model->gltf->scene->nodes[i], cmd, model);
    }
}