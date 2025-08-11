#include "Controller_Keyboard.h"

Controller_Keyboard::Controller_Keyboard(int keymap_index) {
    // Hardcoded keymaps for now
    if (keymap_index == 0) {
        key_up = SDL_SCANCODE_UP;
        key_down = SDL_SCANCODE_DOWN;
        key_left = SDL_SCANCODE_LEFT;
        key_right = SDL_SCANCODE_RIGHT;
        key_bomb = SDL_SCANCODE_RCTRL;
    } else {
        // Default to player 1 keys
        key_up = SDL_SCANCODE_UP;
        key_down = SDL_SCANCODE_DOWN;
        key_left = SDL_SCANCODE_LEFT;
        key_right = SDL_SCANCODE_RIGHT;
        key_bomb = SDL_SCANCODE_RCTRL;
    }
    keyboard_state = SDL_GetKeyboardState(nullptr);
}

void Controller_Keyboard::update() {
    // The state is updated by SDL_PumpEvents in the main loop
}

void Controller_Keyboard::reset() {
}

bool Controller_Keyboard::is_left() {
    return keyboard_state[key_left];
}

bool Controller_Keyboard::is_right() {
    return keyboard_state[key_right];
}

bool Controller_Keyboard::is_up() {
    return keyboard_state[key_up];
}

bool Controller_Keyboard::is_down() {
    return keyboard_state[key_down];
}

bool Controller_Keyboard::is_bomb() {
    return keyboard_state[key_bomb];
}
