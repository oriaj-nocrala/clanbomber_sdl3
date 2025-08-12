#include "MapTile_Box.h"
#include "ClanBomber.h"
#include "Resources.h"
#include "AudioMixer.h"
#include "Extra.h"
#include "Timer.h"
#include "ParticleSystem.h"
#include <random>

MapTile_Box::MapTile_Box(int _x, int _y, ClanBomberApplication* _app) : MapTile(_x, _y, _app) {
    texture_name = "maptiles";
    sprite_nr = 10; // Box tile
    destroyed = false;
    destroy_animation = 0.0f;
    blocking = true;
    destructible = true;
    
    SDL_Log("MapTile_Box created at pixel (%d,%d), maps to grid (%d,%d), destructible=%d, destroyed=%d", 
             _x, _y, get_map_x(), get_map_y(), destructible, destroyed);
}

MapTile_Box::~MapTile_Box() {
}

void MapTile_Box::act() {
    if (destroyed) {
        float prev_animation = destroy_animation;
        destroy_animation += Timer::time_elapsed();
        
        // Add smoke particles during destruction animation  
        if (destroy_animation > 0.1f && prev_animation <= 0.1f) {
            ParticleSystem* smoke = new ParticleSystem(get_x(), get_y(), SMOKE_TRAILS, app);
            app->objects.push_back(smoke);
        }
        
        if (destroy_animation > 0.5f) {
            SDL_Log("MapTile_Box at (%d,%d) setting delete_me=true after animation", get_map_x(), get_map_y());
            delete_me = true;
        }
    }
}

void MapTile_Box::show() {
    if (!destroyed) {
        MapTile::show();
    } else {
        // Show destruction animation
        float animation_progress = destroy_animation / 0.5f; // 0.0 to 1.0
        // SDL_Log("MapTile_Box::show() - destroyed=true, animation_progress=%.3f", animation_progress);
        
        if (animation_progress < 1.0f) {
            // Flash/blink effect for destruction animation
            float blink_speed = 20.0f; // How fast it blinks
            bool should_draw = (int)(animation_progress * blink_speed) % 2 == 0;
            
            if (should_draw) {
                // Draw the box sprite with red tint to indicate destruction
                TextureInfo* tex_info = Resources::get_texture(texture_name);
                if (tex_info && tex_info->texture) {
                    // Add red tint for destruction effect
                    SDL_SetTextureColorMod(tex_info->texture, 255, 100, 100);
                    MapTile::show();
                    // Reset color
                    SDL_SetTextureColorMod(tex_info->texture, 255, 255, 255);
                }
            }
            // If !should_draw, don't render anything (creates blinking effect)
        }
        // If animation is complete (>=1.0), don't render anything
    }
}

void MapTile_Box::destroy() {
    if (!destroyed) {
        SDL_Log("MapTile_Box::destroy() called at (%d,%d)", get_map_x(), get_map_y());
        destroyed = true;
        blocking = false;
        destroy_animation = 0.0f;
        
        // Play destruction sound with 3D positioning
        AudioPosition box_pos(get_x(), get_y(), 0.0f);
        AudioMixer::play_sound_3d("break", box_pos, 500.0f);
        
        // 30% chance to drop a power-up
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> drop_chance(0.0, 1.0);
        
        if (drop_chance(gen) < 0.3f) {
            // Choose random power-up type
            std::uniform_int_distribution<> type_dist(0, 8);
            Extra::EXTRA_TYPE extra_type = static_cast<Extra::EXTRA_TYPE>(type_dist(gen));
            
            // Create power-up at this position
            Extra* extra = new Extra(get_x(), get_y(), extra_type, app);
            app->objects.push_back(extra);
        }
        
        // Add particle effects for destruction
        ParticleSystem* dust = new ParticleSystem(get_x(), get_y(), DUST_CLOUDS, app);
        app->objects.push_back(dust);
        
        ParticleSystem* sparks = new ParticleSystem(get_x(), get_y(), EXPLOSION_SPARKS, app);
        app->objects.push_back(sparks);
    }
}