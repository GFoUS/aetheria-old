#pragma once

#define GLFW_INCLUDE_VULKAN

#include "core/core.h"
#include "GLFW/glfw3.h"

typedef struct {
	GLFWwindow* window;
} window;

typedef struct {
	u32 width;
	u32 height;
	const char* title;

	GLFWkeyfun keyboardCallback;
} window_config;

void window_system_setup();
void window_system_cleanup();

window* window_create(window_config* config);
void window_destroy(window* win);
