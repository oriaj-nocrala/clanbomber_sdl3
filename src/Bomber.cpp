#include "Bomber.h"
#include "Timer.h"
#include "Controller.h"
#include "Bomb.h"
#include "BomberCorpse.h"
#include "ClanBomber.h"
#include "GameConfig.h"
#include "AudioMixer.h"
#include "Map.h"

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
    dead = false;
    
    // Initialize lives and respawn system
    remaining_lives = 3; // Default 3 lives
    respawning = false;
    invincible = false;
    respawn_timer = 0.0f;
    invincible_timer = 0.0f;
    
    // Initialize flight animation
    flying = false;
    flight_timer = 0.0f;
    flight_duration = 0.0f;
    start_x = start_y = target_x = target_y = 0;
    
    // Initialize new member variables
    bomber_team = 0;
    bomber_number = 0;
    bomber_name = "Bomber";

    // Set texture based on color - using proper bomber skins
    switch (color) {
        case RED: texture_name = "bomber_dull_red"; break;
        case BLUE: texture_name = "bomber_dull_blue"; break;
        case YELLOW: texture_name = "bomber_dull_yellow"; break;
        case GREEN: texture_name = "bomber_dull_green"; break;
        case CYAN: texture_name = "bomber_snake"; break;
        case ORANGE: texture_name = "bomber_tux"; break;
        case PURPLE: texture_name = "bomber_spider"; break;
        case BROWN: texture_name = "bomber_bsd"; break;
        default: texture_name = "bomber_snake"; break;
    }
}

void Bomber::act(float deltaTime) {
    if (dead || !controller) {
        return;
    }
    
    // Handle respawn timing
    if (respawning) {
        respawn_timer -= deltaTime;
        if (respawn_timer <= 0.0f) {
            respawning = false;
            dead = false;
            invincible = true;
            invincible_timer = 3.0f; // 3 seconds of invincibility
        } else {
            return; // Don't process input while respawning
        }
    }
    
    // Handle invincibility timer
    if (invincible) {
        invincible_timer -= deltaTime;
        if (invincible_timer <= 0.0f) {
            invincible = false;
        }
    }
    
    // Handle flight animation
    if (flying) {
        flight_timer += deltaTime;
        float progress = flight_timer / flight_duration;
        
        if (progress >= 1.0f) {
            // Animation complete
            flying = false;
            x = target_x;
            y = target_y;
        } else {
            // Interpolate position with easing
            float ease_progress = 1.0f - (1.0f - progress) * (1.0f - progress); // Ease out
            x = start_x + (target_x - start_x) * ease_progress;
            y = start_y + (target_y - start_y) * ease_progress;
        }
    }
    
    controller->update();

    // Only process input if controller is active and not flying
    bool moved = false;
    if (controller->active && !flying) {
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
        if (controller->is_left() || controller->is_right() || controller->is_up() || controller->is_down()) {
            moved = move(deltaTime);
        }
    }

    if (bomb_cooldown > 0) {
        bomb_cooldown -= deltaTime;
    }

    if (controller->active && !flying && controller->is_bomb() && bomb_cooldown <= 0) {
        SDL_Log("Bomber placing bomb at pixel (%f,%f), maps to grid (%d,%d)", x, y, get_map_x(), get_map_y());
        app->objects.push_back(new Bomb(x, y, power, this, app));
        bomb_cooldown = 0.5f; // 0.5 second cooldown
        
        // Play bomb placement sound with 3D positioning
        AudioPosition bomber_pos(x, y, 0.0f);
        AudioMixer::play_sound_3d("putbomb", bomber_pos, 400.0f);
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

void Bomber::die() {
    if (!dead && !invincible) {
        dead = true;
        
        // Create corpse at current position
        BomberCorpse* corpse = new BomberCorpse(x, y, color, app);
        app->objects.push_back(corpse);
        
        // Check if bomber has lives remaining
        lose_life();
        
        if (has_lives()) {
            // Start respawn process
            respawning = true;
            respawn_timer = 2.0f; // 2 seconds until respawn
        } else {
            // No lives left, mark for deletion
            delete_me = true;
        }
    }
}

void Bomber::respawn() {
    dead = false;
    respawning = false;
    invincible = true;
    invincible_timer = 3.0f;
    
    // Reset position to spawn point
    if (app && app->map) {
        CL_Vector spawn_pos = app->map->get_bomber_pos(bomber_number);
        x = spawn_pos.x * 40;
        y = spawn_pos.y * 40;
    }
}

void Bomber::show() {
    // Handle invincibility flickering
    if (invincible) {
        // Flicker by showing/hiding based on timer
        int flicker_rate = 8; // Flickers per second
        float flicker_time = 1.0f / flicker_rate;
        int flicker_frame = (int)(invincible_timer / flicker_time);
        
        if (flicker_frame % 2 == 0) {
            return; // Skip rendering this frame
        }
    }
    
    // Call parent show() for normal rendering
    GameObject::show();
}

void Bomber::fly_to(int target_x, int target_y, float duration_ms) {
    flying = true;
    flight_timer = 0.0f;
    flight_duration = duration_ms / 1000.0f; // Convert to seconds
    start_x = x;
    start_y = y;
    this->target_x = target_x;
    this->target_y = target_y;
}