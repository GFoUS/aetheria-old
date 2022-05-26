#include "context.h"

vulkan_context* vulkan_context_create(window* win)
{
	vulkan_context* ctx = malloc(sizeof(vulkan_context));
	ctx->win = win;

	//const char* extensions[0];
	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
	ctx->instance =  vulkan_instance_create(0, NULL, 1, layers);
	INFO("Created vulkan instance");

	ctx->physical = vulkan_physical_device_create(ctx->instance);
	INFO("Using GPU: %s", ctx->physical->properties.deviceName);

	ctx->device = vulkan_device_create(ctx->instance, ctx->physical, 0, NULL, 1, layers);
	INFO("Create vulkan device");

	return ctx;
}

void vulkan_context_destroy(vulkan_context* ctx)
{
	vulkan_device_destroy(ctx->device);
	vulkan_physical_device_destroy(ctx->physical);
	vulkan_instance_destroy(ctx->instance);

	free(ctx);
}
