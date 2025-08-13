#include "Controller_Keyboard.h"

const bool* Controller_Keyboard::keyboard_state = nullptr;

Controller_Keyboard::Controller_Keyboard(int keymap_index) {
    switch (keymap_index) {
        case 0:
            // Arrow keys + Enter
            key_left = SDL_SCANCODE_LEFT;
            key_right = SDL_SCANCODE_RIGHT;
            key_up = SDL_SCANCODE_UP;
            key_down = SDL_SCANCODE_DOWN;
            key_bomb = SDL_SCANCODE_RETURN;
            break;
        case 1:
            // WASD + Tab
            key_left = SDL_SCANCODE_A;
            key_right = SDL_SCANCODE_D;
            key_up = SDL_SCANCODE_W;
            key_down = SDL_SCANCODE_S;
            key_bomb = SDL_SCANCODE_TAB;
            break;
        case 2:
            // IJKL + Space
            key_left = SDL_SCANCODE_J;
            key_right = SDL_SCANCODE_L;
            key_up = SDL_SCANCODE_I;
            key_down = SDL_SCANCODE_K;
            key_bomb = SDL_SCANCODE_SPACE;
            break;
        default:
            // Default to arrow keys
            key_left = SDL_SCANCODE_LEFT;
            key_right = SDL_SCANCODE_RIGHT;
            key_up = SDL_SCANCODE_UP;
            key_down = SDL_SCANCODE_DOWN;
            key_bomb = SDL_SCANCODE_RETURN;
            break;
    }
}

void Controller_Keyboard::update_keyboard_state() {
    keyboard_state = SDL_GetKeyboardState(nullptr);
}

void Controller_Keyboard::update() {
    // This is now handled by the static update_keyboard_state() method
    // called once per frame in the main loop.
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
