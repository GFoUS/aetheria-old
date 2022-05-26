#pragma once

#include "core.h"
#include "GLFW/glfw3.h"

#define MAX_KEYBOARD_LISTENERS 1024

typedef enum  {
    PRESS = GLFW_PRESS,
    RELEASE = GLFW_RELEASE,
    REPEAT = GLFW_REPEAT
} key_action;

typedef void(*keyboard_listener_fn)(i32 key, key_action action);

typedef struct {
    bool keyPressed[GLFW_KEY_LAST];
    u32 numKeyboardListeners;
    keyboard_listener_fn keyboardListeners[MAX_KEYBOARD_LISTENERS];
} input_manager;

bool input_manager_is_key_pressed(i32 key);
void input_manager_register_keyboard_listener(keyboard_listener_fn listener);
void input_manager_keyboard_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);