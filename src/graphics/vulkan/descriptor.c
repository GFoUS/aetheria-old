#include "descriptor.h"

vulkan_descriptor_set_layout_builder* vulkan_descriptor_set_layout_builder_create() {
    vulkan_descriptor_set_layout_builder* builder = malloc(sizeof(vulkan_descriptor_set_layout_builder));
    CLEAR_MEMORY(builder);

    builder->bindings = malloc(0);

    return builder;
}

vulkan_descriptor_set_layout* vulkan_descriptor_set_layout_builder_build(vulkan_descriptor_set_layout_builder* builder, vulkan_device* device) {
    VkDescriptorSetLayoutCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = builder->numBindings;
    createInfo.pBindings = builder->bindings;

    vulkan_descriptor_set_layout* layout = malloc(sizeof(vulkan_descriptor_set_layout));
    layout->device = device;
    layout->numBindings = builder->numBindings;
    layout->bindings = builder->bindings;

    VkResult result = vkCreateDescriptorSetLayout(device->device, &createInfo, NULL, &layout->layout);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan descriptor set layout creation failed with error code: %d", result);
    }

    free(builder); // The bindings array is now used in the descriptor set layout struct so it doesn't need to be freed

    return layout;
}

void vulkan_descriptor_set_layout_builder_add(vulkan_descriptor_set_layout_builder* builder, VkDescriptorType type) {
    builder->numBindings++;
    builder->bindings = realloc(builder->bindings, sizeof(VkDescriptorSetLayoutBinding) * builder->numBindings);

    builder->bindings[builder->numBindings - 1].binding = builder->numBindings - 1;
    builder->bindings[builder->numBindings - 1].descriptorCount = 1;
    builder->bindings[builder->numBindings - 1].descriptorType = type;
    builder->bindings[builder->numBindings - 1].stageFlags = VK_SHADER_STAGE_ALL;
    if (type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) builder->bindings[builder->numBindings - 1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    builder->bindings[builder->numBindings - 1].pImmutableSamplers = NULL;
}

void vulkan_descriptor_set_layout_destroy(vulkan_descriptor_set_layout* layout) {
    vkDestroyDescriptorSetLayout(layout->device->device, layout->layout, NULL);
    free(layout->bindings);
    free(layout);
}

vulkan_descriptor_allocator* vulkan_descriptor_allocator_create(vulkan_device* device, vulkan_descriptor_set_layout* layout) {
    vulkan_descriptor_allocator* allocator = malloc(sizeof(vulkan_descriptor_allocator));
    CLEAR_MEMORY(allocator);

    allocator->device = device;
    allocator->pools = malloc(0);
    allocator->layout = layout;

    return allocator;
}
void vulkan_descriptor_allocator_destroy(vulkan_descriptor_allocator* allocator) {
    for (u32 i = 0; i < allocator->numPools; i++) {
        vkDestroyDescriptorPool(allocator->device->device, allocator->pools[i]->pool, NULL);
    }
    free(allocator->pools);
    free(allocator);
}

vulkan_descriptor_pool* vulkan_descriptor_pool_create(vulkan_device* device, vulkan_descriptor_set_layout* layout) {
    VkDescriptorPoolSize allPoolSizes[11]; // There are eleven different VkDescriptorTypes
    CLEAR_MEMORY_ARRAY(allPoolSizes, 11);

    for (u32 i = 0; i < 11; i++) {
        allPoolSizes[i].type = i;
    }
    for (u32 i = 0; i < layout->numBindings; i++) {
        allPoolSizes[layout->bindings[i].descriptorType].descriptorCount += SETS_PER_POOL;
    }
    VkDescriptorPoolSize poolSizes[11];
    u32 ptr = 0;
    for (u32 i = 0; i < 11; i++) {
        if (allPoolSizes[i].descriptorCount) {
            poolSizes[ptr++] = allPoolSizes[i];
        }
    }

    vulkan_descriptor_pool* pool = malloc(sizeof(vulkan_descriptor_pool));
    CLEAR_MEMORY(pool);

    pool->device = device;
    pool->remaining = SETS_PER_POOL;

    VkDescriptorPoolCreateInfo createInfo;
    CLEAR_MEMORY(&createInfo);

    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.maxSets = SETS_PER_POOL;
    createInfo.poolSizeCount = ptr;
    createInfo.pPoolSizes = poolSizes;

    VkResult result = vkCreateDescriptorPool(device->device, &createInfo, NULL, &pool->pool);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan descriptor pool creation failed with error code: %d", result);
    }
    
    return pool;
}

vulkan_descriptor_set* vulkan_descriptor_set_allocate(vulkan_descriptor_allocator* allocator) {
    if (allocator->currentPoolIndex == 0 || allocator->pools[allocator->currentPoolIndex]->remaining == 0) {
        // No capacity remaining, need a new set

        bool freeSlot = false;
        for (u32 i = 0; i < allocator->numPools; i++) {
            if (allocator->pools[i] == NULL) {
                // A pool has been fully freed and destroyed so the slot can be used again
                allocator->currentPoolIndex = i;
                freeSlot = true;
            }
        }

        if (!freeSlot) {
            // No slots are free so the array must be extended and a new pool allocated
            allocator->currentPoolIndex = allocator->numPools;
            allocator->numPools++;
            allocator->pools = realloc(allocator->pools, sizeof(vulkan_descriptor_pool*) * allocator->numPools);
        }

        allocator->pools[allocator->currentPoolIndex] = vulkan_descriptor_pool_create(allocator->device, allocator->layout);
    }

    VkDescriptorSetAllocateInfo allocInfo;
    CLEAR_MEMORY(&allocInfo);

    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = allocator->pools[allocator->currentPoolIndex]->pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &allocator->layout->layout;

    vulkan_descriptor_set* set = malloc(sizeof(vulkan_descriptor_set));
    set->allocator = allocator;
    set->poolIndex = allocator->currentPoolIndex;

    VkResult result = vkAllocateDescriptorSets(allocator->device->device, &allocInfo, &set->set);
    if (result != VK_SUCCESS) {
        FATAL("Vulkan descriptor set allocation failed with error code: %d", result);
    }

    allocator->pools[allocator->currentPoolIndex]->sets[SETS_PER_POOL - allocator->pools[allocator->currentPoolIndex]->remaining] = set;
    allocator->pools[allocator->currentPoolIndex]->remaining--;

    return set;
}

void vulkan_descriptor_set_free(vulkan_descriptor_set* set) {
    vulkan_descriptor_allocator* allocator = (vulkan_descriptor_allocator*)set->allocator;
    vulkan_descriptor_pool* pool = allocator->pools[set->poolIndex];
    
    if (pool->freed == SETS_PER_POOL) {
        // Pool has no descriptor sets, delete it
        VkDescriptorSet* sets = malloc(sizeof(VkDescriptorSet) * SETS_PER_POOL);
        for (u32 i = 0; i < SETS_PER_POOL; i++) {
            sets[i] = pool->sets[i]->set;
        }

        vkFreeDescriptorSets(allocator->device->device, pool->pool, SETS_PER_POOL, sets);
        for (u32 i = 0; i < SETS_PER_POOL; i++) {
            free(pool->sets[i]);
        }
        free(pool->sets);

        vkDestroyDescriptorPool(allocator->device->device, pool->pool, NULL);

        pool = NULL; // Set the pool in the allocator's array to null so the slot can be reused
    }
}

void vulkan_descriptor_set_write_buffer(vulkan_descriptor_set* set, u32 binding, vulkan_buffer* buffer) {
    VkDescriptorBufferInfo bufferInfo;
    CLEAR_MEMORY(&bufferInfo);

    bufferInfo.buffer = buffer->buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = buffer->size;
    
    VkWriteDescriptorSet writeInfo;
    CLEAR_MEMORY(&writeInfo);

    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstSet = set->set;
    writeInfo.dstBinding = binding;
    writeInfo.dstArrayElement = 0;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeInfo.pBufferInfo = &bufferInfo;

    vulkan_descriptor_allocator* allocator = (vulkan_descriptor_allocator*)set->allocator;
    vkUpdateDescriptorSets(allocator->device->device, 1, &writeInfo, 0, NULL);
}

void vulkan_descriptor_set_write_image(vulkan_descriptor_set* set, u32 binding, vulkan_image* image, vulkan_sampler* sampler) {
    VkDescriptorImageInfo imageInfo;
    CLEAR_MEMORY(&imageInfo);

    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = image->imageView;
    imageInfo.sampler = sampler->sampler;
    
    VkWriteDescriptorSet writeInfo;
    CLEAR_MEMORY(&writeInfo);

    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstSet = set->set;
    writeInfo.dstBinding = binding;
    writeInfo.dstArrayElement = 0;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeInfo.pImageInfo = &imageInfo;

    vulkan_descriptor_allocator* allocator = (vulkan_descriptor_allocator*)set->allocator;
    vkUpdateDescriptorSets(allocator->device->device, 1, &writeInfo, 0, NULL);
}

void vulkan_descriptor_set_write_input_attachment(vulkan_descriptor_set* set, u32 binding, vulkan_image* image) {
    VkDescriptorImageInfo imageInfo;
    CLEAR_MEMORY(&imageInfo);

    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = image->imageView;
    imageInfo.sampler = NULL;
    
    VkWriteDescriptorSet writeInfo;
    CLEAR_MEMORY(&writeInfo);

    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstSet = set->set;
    writeInfo.dstBinding = binding;
    writeInfo.dstArrayElement = 0;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    writeInfo.pImageInfo = &imageInfo;

    vulkan_descriptor_allocator* allocator = (vulkan_descriptor_allocator*)set->allocator;
    vkUpdateDescriptorSets(allocator->device->device, 1, &writeInfo, 0, NULL);
}