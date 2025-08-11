#ifndef CONTROLLER_JOYSTICK_H
#define CONTROLLER_JOYSTICK_H

#include "Controller.h"

class Controller_Joystick : public Controller {
public:
    Controller_Joystick(int) {}
    virtual ~Controller_Joystick() {}

    void update() override {}
    void reset() override {}
    bool is_left() override { return false; }
    bool is_right() override { return false; }
    bool is_up() override { return false; }
    bool is_down() override { return false; }
    bool is_bomb() override { return false; }
};

#endif
