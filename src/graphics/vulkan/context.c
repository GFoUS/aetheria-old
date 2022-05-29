#include "context.h"

vulkan_context* vulkan_context_create(window* win)
{
	vulkan_context* ctx = malloc(sizeof(vulkan_context));
	ctx->win = win;

	u32 numGlfwExtensions;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&numGlfwExtensions);
	u32 numWantedExtensions = 1;
	const char* wantedExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME };

	u32 numExtensions = numGlfwExtensions + numWantedExtensions;
	const char** extensions = malloc(sizeof(const char*) * numExtensions);
	memcpy(extensions, glfwExtensions, sizeof(const char*) * numGlfwExtensions);
	memcpy(&extensions[numGlfwExtensions], wantedExtensions, sizeof(const char*) * numWantedExtensions);

	const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
	ctx->instance =  vulkan_instance_create(numExtensions, extensions, 1, layers);
	INFO("Created vulkan instance");
	free(extensions);

	VkResult surfaceResult = glfwCreateWindowSurface(ctx->instance->instance, ctx->win->window, NULL, &ctx->surface);
	if (surfaceResult != VK_SUCCESS) {
		FATAL("Failed to create vulkan surface with error code %d", surfaceResult);
	}
	INFO("Created vulkan surface");

	const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	ctx->physical = vulkan_physical_device_create(ctx->instance, ctx->surface, 1, deviceExtensions);
	INFO("Using GPU: %s", ctx->physical->properties.deviceName);

	ctx->device = vulkan_device_create(ctx->instance, ctx->physical, 1, deviceExtensions, 1, layers);
	INFO("Created vulkan device");

	ctx->swapchain = vulkan_swapchain_create(ctx->device, win, ctx->surface);
	INFO("Created swapchain");

	return ctx;
}

void vulkan_context_destroy(vulkan_context* ctx)
{
	vulkan_swapchain_destroy(ctx->swapchain);
	vulkan_device_destroy(ctx->device);
	vulkan_physical_device_destroy(ctx->physical);
	vkDestroySurfaceKHR(ctx->instance->instance, ctx->surface, NULL);
	vulkan_instance_destroy(ctx->instance);

	free(ctx);
}
