#ifndef CONTROLLER_RCMOUSE_H
#define CONTROLLER_RCMOUSE_H

#include "Controller.h"

class Controller_RCMouse : public Controller {
public:
    Controller_RCMouse() {}
    virtual ~Controller_RCMouse() {}

    void update() override {}
    void reset() override {}
    bool is_left() override { return false; }
    bool is_right() override { return false; }
    bool is_up() override { return false; }
    bool is_down() override { return false; }
    bool is_bomb() override { return false; }
};

#endif
