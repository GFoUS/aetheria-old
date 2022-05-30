#include "core/input.h"

#include "graphics/window.h"
#include "graphics/renderer.h"

void all_systems_go() {
	window_system_setup();
}

void all_systems_gone() {
	window_system_cleanup();
}

int main() {
	all_systems_go();

	window_config win_config;
	win_config.width = 640;
	win_config.height = 480;
	win_config.title = "Aetheria";
	win_config.keyboardCallback = input_manager_keyboard_callback;
	window* win = window_create(&win_config);
	renderer* render = renderer_create(win);

	while (!glfwWindowShouldClose(win->window)) {
		glfwPollEvents();
		if (input_manager_is_key_pressed(GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(win->window, 1);
		}

		renderer_render(render);
	}

	vkDeviceWaitIdle(render->ctx->device->device);

	renderer_destroy(render);
	window_destroy(win);

	all_systems_gone();
}