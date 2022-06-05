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

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
 
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	#ifdef NDEBUG
	win->window = glfwCreateWindow(mode->width, mode->width, config->title, monitor, NULL);
	#else
	win->window = glfwCreateWindow(640, 480, config->title, NULL, NULL);
	#endif
	if (!win->window)
	{
		glfwTerminate();
		FATAL("GLFW window creation failed");
	}
	INFO("Window created");

	glfwSetKeyCallback(win->window, config->keyboardCallback);

	return win;
}

void window_destroy(window* win) {
	glfwDestroyWindow(win->window);
	free(win);
}