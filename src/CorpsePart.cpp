#include "CorpsePart.h"
#include "Timer.h"
#include "Resources.h"
#include <random>
#include <cmath>

CorpsePart::CorpsePart(int _x, int _y, int part_type, float vel_x, float vel_y, ClanBomberApplication* app) 
    : GameObject(_x, _y, app) {
    
    velocity_x = vel_x;
    velocity_y = vel_y;
    gravity = 300.0f; // Pixels per second squared
    bounce_factor = 0.6f;
    lifetime = 0.0f;
    max_lifetime = 5.0f; // 5 seconds before disappearing
    has_bounced = false;
    
    texture_name = "corpse_parts";
    part_sprite = part_type % 4; // 4 different body parts in the sprite sheet
    sprite_nr = part_sprite;
    z = Z_CORPSE_PART;
    
    // Random rotation for more chaotic effect
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> rot_dist(-180.0f, 180.0f);
    std::uniform_real_distribution<> angular_dist(-360.0f, 360.0f);
    
    rotation = rot_dist(gen);
    angular_velocity = angular_dist(gen); // degrees per second
}

CorpsePart::~CorpsePart() {
}

void CorpsePart::act(float deltaTime) {
    lifetime += deltaTime;
    
    // Apply physics
    velocity_y += gravity * deltaTime; // Apply gravity
    x += velocity_x * deltaTime;
    y += velocity_y * deltaTime;
    
    // Apply rotation
    rotation += angular_velocity * deltaTime;
    if (rotation > 360.0f) rotation -= 360.0f;
    if (rotation < -360.0f) rotation += 360.0f;
    
    // Ground collision (simple)
    if (y > 560) { // Assuming ground at y=560 (600-40 for sprite height)
        y = 560;
        if (!has_bounced || velocity_y > 50.0f) {
            velocity_y = -velocity_y * bounce_factor;
            velocity_x *= 0.8f; // Friction
            angular_velocity *= 0.7f; // Slow down rotation
            has_bounced = true;
        } else {
            // Stop bouncing if velocity is too low
            velocity_y = 0;
            angular_velocity *= 0.95f; // Gradually stop spinning
        }
    }
    
    // Wall collisions
    if (x < 0) {
        x = 0;
        velocity_x = -velocity_x * bounce_factor;
    } else if (x > 760) { // Assuming screen width 800-40
        x = 760;
        velocity_x = -velocity_x * bounce_factor;
    }
    
    // Fade out after max lifetime
    if (lifetime > max_lifetime) {
        delete_me = true;
    }
}

void CorpsePart::show() {
    if (lifetime > max_lifetime - 1.0f) {
        // Fade out in the last second
        float fade_time = max_lifetime - lifetime;
        float alpha = fade_time; // 0.0 to 1.0
        TextureInfo* tex_info = Resources::get_texture(texture_name);
        if (tex_info && tex_info->texture) {
            SDL_SetTextureAlphaMod(tex_info->texture, (Uint8)(alpha * 255));
        }
    }
    
    // TODO: Apply rotation to sprite rendering
    // For now, just show normal sprite
    GameObject::show();
    
    // Reset alpha
    TextureInfo* tex_info = Resources::get_texture(texture_name);
    if (tex_info && tex_info->texture) {
        SDL_SetTextureAlphaMod(tex_info->texture, 255);
    }
}