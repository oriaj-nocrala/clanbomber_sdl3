#include "Bomber.h"
#include "Timer.h"
#include "Controller.h"
#include "Bomb.h"
#include "ClanBomber.h"
#include "GameConfig.h"

Bomber::Bomber(int _x, int _y, COLOR _color, Controller* _controller, ClanBomberApplication *_app) : GameObject(_x, _y, _app) {
    color = _color;
    controller = _controller;
    if (controller) {
        controller->attach(this);
    }
    anim_count = 0;
    cur_dir = DIR_RIGHT;
    speed = 90;
    bomb_cooldown = 0.0f;
    power = GameConfig::get_start_power();
    can_kick = true; // TODO: Set to false by default, enable with power-up
    
    // Initialize new member variables
    bomber_team = 0;
    bomber_number = 0;
    bomber_name = "Bomber";

    // Set texture based on color
    switch (color) {
        case RED: texture_name = "bomber_dull_red"; break;
        case BLUE: texture_name = "bomber_dull_blue"; break;
        case YELLOW: texture_name = "bomber_dull_yellow"; break;
        case GREEN: texture_name = "bomber_dull_green"; break;
        // Add other colors later
        default: texture_name = "bomber_snake"; break;
    }
}

void Bomber::act(float deltaTime) {
    if (!controller) {
        return;
    }
    controller->update();

    // Update direction based on controller input
    if (controller->is_left()) {
        cur_dir = DIR_LEFT;
    } else if (controller->is_right()) {
        cur_dir = DIR_RIGHT;
    } else if (controller->is_up()) {
        cur_dir = DIR_UP;
    } else if (controller->is_down()) {
        cur_dir = DIR_DOWN;
    }

    // Move if a direction key is pressed, otherwise the animation frame is reset
    bool moved = false;
    if (controller->is_left() || controller->is_right() || controller->is_up() || controller->is_down()) {
        moved = move(deltaTime);
    }

    if (bomb_cooldown > 0) {
        bomb_cooldown -= deltaTime;
    }

    if (controller->is_bomb() && bomb_cooldown <= 0) {
        app->objects.push_back(new Bomb(x, y, power, this, app));
        bomb_cooldown = 0.5f; // 0.5 second cooldown
    }

    if (moved) {
        anim_count += deltaTime * 20 * (speed / 90.0f);
    } else {
        anim_count = 0;
    }

    if (anim_count >= 9) {
        anim_count = 1;
    }

    sprite_nr = (int)cur_dir * 10 + (int)anim_count;
}