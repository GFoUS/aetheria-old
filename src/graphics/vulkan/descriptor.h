#pragma once

#include "core/core.h"
#include "vulkan/vulkan.h"
#include "device.h"
#include "buffer.h"
#include "image.h"

#define SETS_PER_POOL 1000

typedef struct {
    u32 numBindings;
    VkDescriptorSetLayoutBinding* bindings;
} vulkan_descriptor_set_layout_builder;

typedef struct {
    VkDescriptorSetLayout layout;
    vulkan_device* device;
    u32 numBindings;
    VkDescriptorSetLayoutBinding* bindings;
} vulkan_descriptor_set_layout;

vulkan_descriptor_set_layout_builder* vulkan_descriptor_set_layout_builder_create();
vulkan_descriptor_set_layout* vulkan_descriptor_set_layout_builder_build(vulkan_descriptor_set_layout_builder* builder, vulkan_device* device);
void vulkan_descriptor_set_layout_builder_add(vulkan_descriptor_set_layout_builder* builder, VkDescriptorType type);

void vulkan_descriptor_set_layout_destroy(vulkan_descriptor_set_layout* layout);

typedef struct {
    VkDescriptorSet set;
    void* allocator;
    u32 poolIndex;
} vulkan_descriptor_set;

typedef struct {
    VkDescriptorPool pool;
    vulkan_device* device;

    vulkan_descriptor_set* sets[SETS_PER_POOL];
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

void vulkan_descriptor_set_write_buffer(vulkan_descriptor_set* set, u32 binding, vulkan_buffer* buffer);
void vulkan_descriptor_set_write_image(vulkan_descriptor_set* set, u32 binding, vulkan_image* image, vulkan_sampler* sampler);
void vulkan_descriptor_set_write_input_attachment(vulkan_descriptor_set* set, u32 binding, vulkan_image* image);