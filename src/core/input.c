#include "input.h"

static input_manager* in_manager;

input_manager* _input_manager_get() {
    if (in_manager != NULL) {
        return in_manager;
    } else {
        in_manager = malloc(sizeof(input_manager));
        CLEAR_MEMORY(in_manager);
        return in_manager;
    }
}

bool input_manager_is_key_pressed(i32 key) {
    return _input_manager_get()->keyPressed[key];
}

void input_manager_register_keyboard_listener(keyboard_listener_fn listener) {
    _input_manager_get()->keyboardListeners[_input_manager_get()->numKeyboardListeners] = listener;
    _input_manager_get()->numKeyboardListeners++;
}

void input_manager_keyboard_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods) {
    if (action == GLFW_PRESS) {
        _input_manager_get()->keyPressed[key] = true;
    } else if (action == GLFW_RELEASE) {
        _input_manager_get()->keyPressed[key] = false;
    }

    for (u32 i = 0; i < _input_manager_get()->numKeyboardListeners; i++) {
        _input_manager_get()->keyboardListeners[i](key, (key_action)action);
    }
}