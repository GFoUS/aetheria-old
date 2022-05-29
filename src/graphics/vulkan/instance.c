#include "instance.h"

vulkan_instance* vulkan_instance_create(u32 numExtensions, const char** extensions, u32 numLayers, const char** layers)
{
	vulkan_instance* instance = malloc(sizeof(vulkan_instance));

	// Check validation layers present
	u32 numAvailableLayers;
	vkEnumerateInstanceLayerProperties(&numAvailableLayers, NULL);
	VkLayerProperties* availableLayers = malloc(numAvailableLayers * sizeof(VkLayerProperties));
	vkEnumerateInstanceLayerProperties(&numAvailableLayers, availableLayers);
	for (u32 i = 0; i < numLayers; i++) {
		bool found = false;
		for (u32 j = 0; j < numAvailableLayers; j++) {
			if (strcmp(layers[i], availableLayers[j].layerName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			FATAL("Missing vulkan validation layer: %s", layers[i]);
		}
	}

	// Check extensions present
	u32 numAvailableExtensions;
	vkEnumerateInstanceExtensionProperties(NULL, &numAvailableExtensions, NULL);
	VkExtensionProperties* availableExtensions = malloc(numAvailableExtensions * sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(NULL, &numAvailableExtensions, availableExtensions);
	for (u32 i = 0; i < numExtensions; i++) {
		bool found = false;
		for (u32 j = 0; j < numAvailableExtensions; j++) {
			if (strcmp(extensions[i], availableExtensions[j].extensionName) == 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			FATAL("Missing vulkan extension: %s", extensions[i]);
		}
	}

	free(availableLayers);
	free(availableExtensions);

	VkApplicationInfo appInfo;
	CLEAR_MEMORY(&appInfo);
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pApplicationName = "Aetheria";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Aetheria";
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo;
	CLEAR_MEMORY(&createInfo);
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.enabledExtensionCount = numExtensions;
	createInfo.ppEnabledExtensionNames = extensions;
	createInfo.enabledLayerCount = numLayers;
	createInfo.ppEnabledLayerNames = layers;
	createInfo.pApplicationInfo = &appInfo;
	VkResult result = vkCreateInstance(&createInfo, NULL, &instance->instance);
	if (result != VK_SUCCESS) {
		FATAL("Vulkan instance creation failed with error code %d", result);
	}

	return instance;
}

void vulkan_instance_destroy(vulkan_instance* instance)
{
	vkDestroyInstance(instance->instance, NULL);
	free(instance);
}
