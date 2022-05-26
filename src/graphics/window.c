#include "window.h"

void glfw_error_callback(int error, const char* description) {
	ERROR("GLFW error. Code: %d. Description: %s", error, description);
}

void window_system_setup() {
	if (!glfwInit()) {
		FATAL("Failed to initialise GLFW");
	}

	glfwSetErrorCallback(&glfw_error_callback);
}

void window_system_cleanup() {
	glfwTerminate();
}

window* window_create(window_config* config) {
	window* win = malloc(sizeof(win));

	INFO("Window created");
	win->window = glfwCreateWindow(config->width, config->height, config->title, NULL, NULL);
	if (!win->window)
	{
		glfwTerminate();
		FATAL("GLFW window creation failed");
	}

	glfwSetKeyCallback(win->window, config->keyboardCallback);

	return win;
}

void window_destroy(window* win) {
	glfwDestroyWindow(win->window);
}