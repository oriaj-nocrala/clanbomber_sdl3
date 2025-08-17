#include "Extra.h"
#include "Timer.h"
#include "Resources.h"
#include "AudioMixer.h"
#include "MapTile.h"
#include "Bomber.h"
#include "ParticleSystem.h"
#include "GameContext.h"
#include <cmath>
#include <random>

Extra::Extra(int _x, int _y, EXTRA_TYPE _type, GameContext* context) 
    : GameObject(_x, _y, context) {
    extra_type = _type;
    collected = false;
    collect_animation = 0.0f;
    bounce_timer = 0.0f;
    bounce_offset = 0.0f;
    
    texture_name = "extras2_" + std::to_string((int)extra_type);
    sprite_nr = 0;
    z = Z_EXTRA;
    
    // Snap to grid center
    x = ((int)x + 20) / 40 * 40;
    y = ((int)y + 20) / 40 * 40;
    
    const char* type_names[] = {"FLAME", "BOMB", "SPEED", "KICK", "GLOVE", "SKATE", "DISEASE", "VIAGRA", "KOKS"};
    SDL_Log("Extra created: type=%s at pixel (%d,%d), grid (%d,%d), texture=%s", 
            type_names[(int)extra_type], (int)x, (int)y, get_map_x(), get_map_y(), texture_name.c_str());
}

Extra::~Extra() {
}

void Extra::act(float deltaTime) {
    if (collected) {
        collect_animation += deltaTime;
        if (collect_animation > 0.3f) {
            delete_me = true;
        }
        return;
    }
    
    // Bouncing animation
    bounce_timer += deltaTime * 4.0f; // 4x speed for nice bounce
    bounce_offset = std::sin(bounce_timer) * 3.0f; // 3 pixel bounce
    
    // Check for collision with bombers directly using GameContext
    GameContext* ctx = get_context();
    if (!ctx || !ctx->get_object_lists()) {
        return;
    }
    
    for (auto& obj : *ctx->get_object_lists()) {
        if (!obj || obj->get_type() != GameObject::BOMBER) continue;
        
        Bomber* bomber = static_cast<Bomber*>(obj);
        if (bomber && !bomber->delete_me && !bomber->is_dead()) {
            // Check if bomber is close enough to collect (within 20 pixels)
            float dx = bomber->get_x() - x;
            float dy = bomber->get_y() - y;
            float distance = sqrt(dx*dx + dy*dy);
            
            if (distance < 20.0f) {
                SDL_Log("Extra collected by bomber at distance %.1f", distance);
                apply_effect_to_bomber(bomber);
                collect();
                break;
            }
        }
    }
}

void Extra::show() {
    if (collected) {
        // Fade out during collection
        float alpha = 1.0f - (collect_animation / 0.3f);
    } else {
        // Save current position
        float original_y = y;
        y += bounce_offset;
        GameObject::show();
        y = original_y; // Restore position
    }
}

void Extra::collect() {
    if (collected) return;
    
    collected = true;
    
    // Create pickup particle effect
    ParticleSystem* pickup_sparkles = new ParticleSystem(x, y, EXPLOSION_SPARKS, get_context());
    get_context()->register_object(pickup_sparkles);
    
    // Play collection sound
    AudioPosition extra_pos(x, y, 0.0f);
    
    // Different sounds for different types
    if (extra_type == DISEASE || extra_type == KOKS || extra_type == VIAGRA) {
        AudioMixer::play_sound_3d("schnief", extra_pos, 400.0f); // Negative sound
    } else {
        AudioMixer::play_sound_3d("wow", extra_pos, 400.0f); // Positive sound
    }
    
    // Effect is already applied in act() method
}

void Extra::apply_effect_to_bomber(Bomber* bomber) {
    if (!bomber) return;
    
    switch (extra_type) {
        case BOMB:
            // Increase bomb capacity by 1
            bomber->inc_max_bombs(1);
            SDL_Log("Bomber gained extra bomb! Max bombs: %d", bomber->get_max_bombs());
            break;
            
        case FLAME:
            // Increase explosion power/range by 1
            bomber->inc_power(1);
            SDL_Log("Bomber gained flame power! Power: %d", bomber->get_power());
            break;
            
        case SPEED:
            // Increase movement speed
            bomber->inc_speed(20);
            SDL_Log("Bomber gained speed boost!");
            break;
            
        case KICK:
            // Allow bomb kicking
            bomber->set_can_kick(true);
            SDL_Log("Bomber gained kick ability!");
            break;
            
        case GLOVE:
            // Allow bomb throwing
            bomber->set_can_throw(true);
            SDL_Log("Bomber gained glove ability! Can now throw bombs!");
            break;
            
        case SKATE:
            // Ice skates effect - increase speed but add sliding
            bomber->inc_speed(10);
            SDL_Log("Bomber gained skates! (Basic speed boost)");
            break;
            
        case DISEASE:
            // Constipation - reduce speed and disable bombing temporarily
            bomber->dec_speed(40);
            SDL_Log("Bomber got constipation! Speed reduced!");
            // TODO: Add temporary bomb disable
            break;
            
        case KOKS:
            // Make very fast but harder to control
            bomber->inc_speed(50);
            SDL_Log("Bomber took speed! Very fast but harder to control!");
            // TODO: Add uncontrollable movement effect
            break;
            
        case VIAGRA:
            // Negative effect - could make bombs stick or other penalty
            bomber->dec_speed(20);
            SDL_Log("Bomber took viagra! Movement affected!");
            // TODO: Implement sticky bomb effect
            break;
    }
}