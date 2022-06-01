#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "device.h"

#define SETS_PER_POOL 1000

typedef struct {
    VkDescriptorSetLayout layout;
    vulkan_device* device;
    u32 numBindings;
    VkDescriptorSetLayoutBinding* bindings;
} vulkan_descriptor_set_layout;

typedef struct {
    VkDescriptorSet set;
    struct vulkan_descriptor_pool* pool;
} vulkan_descriptor_set;

typedef struct {
    VkDescriptorPool pool;
    vulkan_device* device;

    vulkan_descriptor_set sets[1000];
    u32 remaining;
    u32 freed;
} vulkan_descriptor_pool;

typedef struct {
    u32 numPools;
    u32 currentPoolIndex;
    vulkan_descriptor_pool** pools; 
    vulkan_device* device;
    vulkan_descriptor_set_layout* layout;
} vulkan_descriptor_allocator;

vulkan_descriptor_allocator* vulkan_descriptor_allocator_create(vulkan_device* device, vulkan_descriptor_set_layout* layout);
void vulkan_descriptor_allocator_destroy(vulkan_descriptor_allocator* allocator);

vulkan_descriptor_set* vulkan_descriptor_set_allocate(vulkan_descriptor_allocator* allocator);
void vulkan_descriptor_set_free(vulkan_descriptor_set* set);