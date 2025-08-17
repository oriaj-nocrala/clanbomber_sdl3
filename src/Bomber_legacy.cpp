#include "Bomber.h"
#include "BomberComponents.h"
#include "Controller.h"
#include "ClanBomber.h"

Bomber::Bomber(int _x, int _y, COLOR _color, Controller* _controller, ClanBomberApplication *_app) 
    : GameObject(_x, _y, _app), color(_color), controller(_controller) {
    
    // Attach controller
    if (controller) {
        controller->attach(this);
    }
    
    // Initialize components using modern C++17 patterns
    movement_component = std::make_unique<BomberMovementComponent>(this, _app);
    combat_component = std::make_unique<BomberCombatComponent>(this, _app);
    animation_component = std::make_unique<BomberAnimationComponent>(this, _app);
    lifecycle_component = std::make_unique<BomberLifecycleComponent>(this, _app);
    
    // Set initial properties
    cur_dir = DIR_RIGHT;
    
    // Configure animation component with color
    animation_component->set_texture_from_color(static_cast<int>(color));
    
    SDL_Log("Bomber: Created modern component-based bomber at (%d,%d) with color %d", _x, _y, static_cast<int>(color));
}

Bomber::~Bomber() {
    // Components are automatically cleaned up via unique_ptr
    SDL_Log("Bomber: Destroyed bomber with modern component cleanup");
}

void Bomber::act(float deltaTime) {
    // MODERN ARCHITECTURE: Delegate to specialized components
    // Each component handles its own responsibility
    
    // Update controller first
    if (controller) {
        controller->update();
    }
    
    // Update all components in proper order
    if (lifecycle_component) {
        lifecycle_component->update(deltaTime);
    }
    
    if (movement_component) {
        movement_component->update(deltaTime);
        movement_component->handle_controller_input(controller, deltaTime);
    }
    
    if (combat_component) {
        combat_component->update(deltaTime);
        combat_component->handle_controller_input(controller, deltaTime);
    }
    
    if (animation_component) {
        animation_component->update(deltaTime);
    }
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

void Bomber::place_bomb() {
    // Check if there's already a bomb at this position using new architecture
    if (has_bomb_at(x+20, y+20)) {
        SDL_Log("Cannot place bomb - position already occupied!");
        return;
    }
    
    SDL_Log("Bomber placing bomb %d/%d at pixel (%f,%f), maps to grid (%d,%d)", 
            current_bombs + 1, max_bombs, x, y, get_map_x(), get_map_y());
    
    app->objects.push_back(new Bomb(x, y, power, this, app));
    inc_current_bombs();
    bomb_cooldown = 0.5f;
    
    // Play bomb placement sound
    AudioPosition bomber_pos(x, y, 0.0f);
    AudioMixer::play_sound_3d("putbomb", bomber_pos, 400.0f);
}

void Bomber::throw_bomb() {
    SDL_Log("Bomber throwing bomb with glove power!");
    
    // Calculate throw target based on facing direction
    float throw_distance = 120.0f; // 3 tiles
    float target_x = x;
    float target_y = y;
    
    // Determine throw direction based on current direction
    switch (cur_dir) {
        case DIR_UP:    target_y -= throw_distance; break;
        case DIR_DOWN:  target_y += throw_distance; break;
        case DIR_LEFT:  target_x -= throw_distance; break;
        case DIR_RIGHT: target_x += throw_distance; break;
    }
    
    // Clamp to map bounds
    target_x = std::max(20.0f, std::min(target_x, 780.0f));
    target_y = std::max(20.0f, std::min(target_y, 580.0f));
    
    SDL_Log("Throwing bomb from (%.1f,%.1f) to (%.1f,%.1f)", x, y, target_x, target_y);
    
    app->objects.push_back(new ThrownBomb(x, y, power, this, target_x, target_y, app));
    inc_current_bombs();
    bomb_cooldown = 0.8f; // Longer cooldown for thrown bombs
    
    // Play throw sound (using different sound)
    AudioPosition bomber_pos(x, y, 0.0f);
    AudioMixer::play_sound_3d("whoosh", bomber_pos, 400.0f);
}