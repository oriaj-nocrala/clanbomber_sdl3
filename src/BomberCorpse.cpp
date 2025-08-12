#include "BomberCorpse.h"
#include "CorpsePart.h"
#include "ClanBomber.h"
#include "AudioMixer.h"
#include "Timer.h"
#include <random>
#include <cmath>

BomberCorpse::BomberCorpse(int _x, int _y, Bomber::COLOR bomber_color, ClanBomberApplication* app) 
    : GameObject(_x, _y, app) {
    
    color = bomber_color;
    exploded = false;
    death_animation_timer = 0.0f;
    gore_explosion_timer = 0.0f;
    gore_created = false;
    
    // Set appropriate corpse texture based on bomber color
    switch (color) {
        case Bomber::RED: texture_name = "bomber_dull_red"; break;
        case Bomber::BLUE: texture_name = "bomber_dull_blue"; break;
        case Bomber::YELLOW: texture_name = "bomber_dull_yellow"; break;
        case Bomber::GREEN: texture_name = "bomber_dull_green"; break;
        case Bomber::CYAN: texture_name = "bomber_snake"; break;
        case Bomber::ORANGE: texture_name = "bomber_tux"; break;
        case Bomber::PURPLE: texture_name = "bomber_spider"; break;
        case Bomber::BROWN: texture_name = "bomber_bsd"; break;
        default: texture_name = "bomber_snake"; break;
    }
    
    sprite_nr = 40; // Dead pose sprite (assuming it exists)
    z = Z_CORPSE;
    
    // Play death sound
    AudioPosition corpse_pos(x, y, 0.0f);
    AudioMixer::play_sound_3d("die", corpse_pos, 500.0f);
}

BomberCorpse::~BomberCorpse() {
}

void BomberCorpse::act(float deltaTime) {
    death_animation_timer += deltaTime;
    
    if (exploded && !gore_created) {
        gore_explosion_timer += deltaTime;
        if (gore_explosion_timer > 0.1f) { // Small delay for dramatic effect
            create_gore_explosion();
            gore_created = true;
            
            // Corpse disappears after gore explosion
            delete_me = true;
        }
    }
    
    // Corpse disappears after 10 seconds if not exploded
    if (!exploded && death_animation_timer > 10.0f) {
        delete_me = true;
    }
}

void BomberCorpse::show() {
    if (!exploded) {
        // Maybe add some flickering or decay effect
        float alpha = 1.0f;
        if (death_animation_timer > 8.0f) {
            // Start fading in last 2 seconds
            alpha = 1.0f - ((death_animation_timer - 8.0f) / 2.0f);
        }
        
        if (alpha > 0.0f) {
            TextureInfo* tex_info = Resources::get_texture(texture_name);
            if (tex_info && tex_info->texture) {
                SDL_SetTextureAlphaMod(tex_info->texture, (Uint8)(alpha * 255));
                GameObject::show();
                SDL_SetTextureAlphaMod(tex_info->texture, 255);
            }
        }
    }
    // Don't show anything if exploded (gore parts handle the visuals)
}

void BomberCorpse::explode() {
    if (!exploded) {
        exploded = true;
        gore_explosion_timer = 0.0f;
        
        // Play gore explosion sound
        AudioPosition corpse_pos(x, y, 0.0f);
        AudioMixer::play_sound_3d("corpse_explode", corpse_pos, 600.0f);
    }
}

void BomberCorpse::create_gore_explosion() {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Create 8-12 body parts with realistic explosion physics
    std::uniform_int_distribution<> part_count_dist(8, 12);
    std::uniform_int_distribution<> part_type_dist(0, 3); // 4 different body parts
    std::uniform_real_distribution<> velocity_dist(150.0f, 450.0f); // Higher velocities for more violence
    std::uniform_real_distribution<> angle_dist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<> force_dist(800.0f, 1500.0f); // Explosion force range
    
    int num_parts = part_count_dist(gen);
    
    for (int i = 0; i < num_parts; i++) {
        float angle = angle_dist(gen);
        float velocity = velocity_dist(gen);
        float explosion_force = force_dist(gen);
        
        // More realistic explosion physics - parts fly outward with variation
        float vel_x = std::cos(angle) * velocity;
        float vel_y = std::sin(angle) * velocity;
        
        // Add upward bias for more dramatic effect
        if (vel_y > 0) vel_y *= 0.7f; // Reduce downward velocity
        else vel_y *= 1.3f; // Increase upward velocity
        
        int part_type = part_type_dist(gen);
        
        // More spread in starting positions for realistic dismemberment
        std::uniform_real_distribution<> pos_offset(-15.0f, 15.0f);
        float start_x = x + pos_offset(gen);
        float start_y = y + pos_offset(gen);
        
        // Create corpse part with advanced physics
        CorpsePart* part = new CorpsePart(start_x, start_y, part_type, vel_x, vel_y, explosion_force, app);
        app->objects.push_back(part);
    }
    
    // Create additional blood splatter effect
    for (int i = 0; i < 20; i++) {
        float angle = angle_dist(gen);
        float velocity = velocity_dist(gen) * 0.6f; // Smaller blood droplets
        float force = force_dist(gen) * 0.3f;
        
        float vel_x = std::cos(angle) * velocity;
        float vel_y = std::sin(angle) * velocity * 0.8f; // Less upward for blood
        
        std::uniform_real_distribution<> blood_offset(-20.0f, 20.0f);
        float start_x = x + blood_offset(gen);
        float start_y = y + blood_offset(gen);
        
        // Use part type 0 for blood droplets (smallest)
        CorpsePart* blood_drop = new CorpsePart(start_x, start_y, 0, vel_x, vel_y, force, app);
        app->objects.push_back(blood_drop);
    }
}