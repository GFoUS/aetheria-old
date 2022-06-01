#include "descriptor.h"

vulkan_descriptor_allocator* vulkan_descriptor_allocator_create(vulkan_device* device, vulkan_descriptor_set_layout* layout) {
    vulkan_descriptor_allocator* allocator = malloc(sizeof(vulkan_descriptor_allocator));
    CLEAR_MEMORY(allocator);

    allocator->device = device;
    allocator->numPools = 1;
    allocator->pools = malloc(sizeof(vulkan_descriptor_pool*));
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

vulkan_descriptor_pool* _vulkan_descriptor_pool_create(vulkan_device* device, vulkan_descriptor_set_layout* layout) {
    for (u32 i = 0; i < layout->numBindings; i++) {
        // TODO: Allocate descriptor pool based on the descriptor layout info
    }
}

vulkan_descriptor_set* vulkan_descriptor_set_allocate(vulkan_descriptor_allocator* allocator) {
    if (allocator->pools[allocator->currentPoolIndex]->remaining != 0) {
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


    }
}
void vulkan_descriptor_set_free(vulkan_descriptor_set* set);