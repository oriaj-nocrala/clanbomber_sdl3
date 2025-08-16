#include "MapTile_Box.h"
#include "ClanBomber.h"
#include "Resources.h"
#include "AudioMixer.h"
#include "Extra.h"
#include "Timer.h"
#include "ParticleSystem.h"
#include "GPUAcceleratedRenderer.h"
#include <random>
#include <cmath>

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
            
            // Spawn power-up now that explosion has ended
            spawn_extra();
            
            delete_me = true;
        }
    }
}

void MapTile_Box::show() {
    if (!destroyed) {
        MapTile::show();
    } else {
        // Show realistic destruction animation
        float animation_progress = destroy_animation / 0.5f; // 0.0 to 1.0
        
        if (animation_progress < 1.0f) {
            // SPECTACULAR GPU fragmentation effects enabled!
            if (app && app->gpu_renderer) {
                GLuint gl_texture = Resources::get_gl_texture(texture_name);
                if (gl_texture) {
                    try {
                        app->gpu_renderer->begin_batch(GPUAcceleratedRenderer::TILE_FRAGMENTATION);
                        
                        // Create realistic fragments flying everywhere like explosion debris!
                        const int num_fragments = 12;  // More fragments = more spectacular
                        for (int i = 0; i < num_fragments; i++) {
                            // Calculate explosion-like fragment trajectories
                            float angle = (float)i / num_fragments * 6.28318f + (animation_progress * 0.5f); // 2*PI with slight spiral
                            float explosion_force = 50.0f * (1.0f - animation_progress * 0.3f); // Strong initial explosion
                            float fragment_x = get_x() + cos(angle) * explosion_force * animation_progress;
                            float fragment_y = get_y() + sin(angle) * explosion_force * animation_progress;
                            
                            // Realistic physics: gravity affects fragments over time
                            fragment_y += animation_progress * animation_progress * 80.0f; // Gravity acceleration
                            
                            // Fragment size varies for realistic debris
                            float fragment_scale = 0.3f + (i % 3) * 0.2f; // 0.3, 0.5, 0.7 sizes
                            fragment_scale *= (1.0f - animation_progress * 0.6f); // Shrink over time
                            
                            // Realistic alpha fade and burning effect
                            float fragment_alpha = (1.0f - animation_progress) * (0.8f + (i % 2) * 0.2f);
                            
                            // Spectacular spinning fragments
                            float rotation = angle * 57.2958f + animation_progress * 900.0f + i * 45.0f; // Fast chaotic spin
                            
                            // Color variety for realistic explosion debris
                            float color_var = (i % 4) * 0.1f;
                            float color[4] = {
                                1.0f - color_var, 
                                0.8f - color_var, 
                                0.6f - color_var, 
                                fragment_alpha
                            };
                            float scale[2] = {fragment_scale, fragment_scale};
                            
                            // Add this spectacular fragment to the GPU batch
                            app->gpu_renderer->add_animated_sprite(
                                fragment_x - (20 * fragment_scale),
                                fragment_y - (20 * fragment_scale),
                                40.0f, 40.0f,
                                gl_texture, color, rotation, scale,
                                GPUAcceleratedRenderer::TILE_FRAGMENTATION
                            );
                        }
                        
                        app->gpu_renderer->end_batch();
                        return; // Spectacular GPU effect rendered successfully!
                        
                    } catch (...) {
                        SDL_Log("GPU fragmentation failed, falling back to SDL effect");
                        // Fall through to SDL fallback
                    }
                }
            }
        }
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
        
        // Smart power-up spawning based on original game logic - delay it slightly
        // to avoid power-ups being destroyed by the same explosion
        // (We'll spawn it when the destruction animation finishes)
        
        // SPECTACULAR tile fragmentation effects!
        if (app->gpu_renderer) {
            // Emit debris particles
            app->gpu_renderer->emit_particles(get_x(), get_y(), 25, GPUAcceleratedRenderer::SPARK, nullptr, 1.0f);
            app->gpu_renderer->emit_particles(get_x(), get_y(), 15, GPUAcceleratedRenderer::SMOKE, nullptr, 2.0f);
            
            SDL_Log("SPECTACULAR tile destruction effects at (%d,%d)!", get_x(), get_y());
        }
        
        // Add traditional particle effects for destruction
        ParticleSystem* dust = new ParticleSystem(get_x(), get_y(), DUST_CLOUDS, app);
        app->objects.push_back(dust);
        
        ParticleSystem* sparks = new ParticleSystem(get_x(), get_y(), EXPLOSION_SPARKS, app);
        app->objects.push_back(sparks);
    }
}