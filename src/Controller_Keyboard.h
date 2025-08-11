#ifndef CONTROLLER_KEYBOARD_H
#define CONTROLLER_KEYBOARD_H

#include "Controller.h"
#include <SDL3/SDL.h>

class Controller_Keyboard : public Controller {
public:
    Controller_Keyboard(int keymap_index);
    
    void update() override;
    void reset() override;
    bool is_left() override;
    bool is_right() override;
    bool is_up() override;
    bool is_down() override;
    bool is_bomb() override;

private:
    const bool* keyboard_state;
    SDL_Scancode key_left;
    SDL_Scancode key_right;
    SDL_Scancode key_up;
    SDL_Scancode key_down;
    SDL_Scancode key_bomb;
};

#endif