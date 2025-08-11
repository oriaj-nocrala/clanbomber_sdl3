#ifndef CONTROLLER_AI_H
#define CONTROLLER_AI_H

#include "Controller.h"

class Controller_AI : public Controller {
public:
    Controller_AI() {}
    virtual ~Controller_AI() {}

    void update() override {}
    void reset() override {}
    bool is_left() override { return false; }
    bool is_right() override { return false; }
    bool is_up() override { return false; }
    bool is_down() override { return false; }
    bool is_bomb() override { return false; }
};

#endif
