#include "Extra.h"
#include "Timer.h"
#include "Resources.h"
#include "AudioMixer.h"
#include "MapTile.h"
#include "Bomber.h"
#include "ParticleSystem.h"
#include <cmath>
#include <random>

Extra::Extra(int _x, int _y, EXTRA_TYPE _type, ClanBomberApplication* app) 
    : GameObject(_x, _y, app) {
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
    
    // Check for collision with bombers
    MapTile* tile = get_tile();
    if (tile && tile->has_bomber()) {
        collect();
    }
}

void Extra::show() {
    if (collected) {
        // Fade out during collection
        float alpha = 1.0f - (collect_animation / 0.3f);
        SDL_SetTextureAlphaMod(Resources::get_texture(texture_name)->texture, (Uint8)(alpha * 255));
        GameObject::show();
        SDL_SetTextureAlphaMod(Resources::get_texture(texture_name)->texture, 255);
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
    ParticleSystem* pickup_sparkles = new ParticleSystem(x, y, EXPLOSION_SPARKS, app);
    app->objects.push_back(pickup_sparkles);
    
    // Play collection sound
    AudioPosition extra_pos(x, y, 0.0f);
    
    // Different sounds for different types
    if (extra_type == DISEASE || extra_type == KOKS || extra_type == VIAGRA) {
        AudioMixer::play_sound_3d("schnief", extra_pos, 400.0f); // Negative sound
    } else {
        AudioMixer::play_sound_3d("wow", extra_pos, 400.0f); // Positive sound
    }
    
    // Apply effect to bomber
    MapTile* tile = get_tile();
    if (tile && tile->has_bomber()) {
        Bomber* bomber = tile->get_bomber();
        apply_effect_to_bomber(bomber);
    }
}

void Extra::apply_effect_to_bomber(Bomber* bomber) {
    if (!bomber) return;
    
    switch (extra_type) {
        case BOMB:
            // Increase bomb capacity
            break;
        case FLAME:
            // Increase explosion power/range
            break;
        case SPEED:
            bomber->inc_speed(30); // Increase speed by 30
            break;
        case KICK:
            bomber->can_kick = true;
            break;
        case GLOVE:
            // Allow bomb throwing (not implemented yet)
            break;
        case SKATE:
            // Ice skates effect (not implemented yet) 
            break;
        case DISEASE:
            // Constipation - can't place bombs for a while
            break;
        case KOKS:
            bomber->inc_speed(60); // Make very fast
            // TODO: Add uncontrollable movement effect
            break;
        case VIAGRA:
            // Bombs stick to bomber (not implemented yet)
            break;
    }
}