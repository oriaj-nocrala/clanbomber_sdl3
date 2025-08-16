#include "Explosion.h"
#include "Bomber.h"
#include "BomberCorpse.h"
#include "Map.h"
#include "MapTile.h"
#include "Resources.h"
#include "Bomb.h"
#include "Timer.h"
#include "ParticleSystem.h"
#include "GPUAcceleratedRenderer.h"
#include <SDL3/SDL_timer.h>

Explosion::Explosion(int _x, int _y, int _power, Bomber* _owner, ClanBomberApplication* app) : GameObject(_x, _y, app) {
    owner = _owner;
    power = _power;
    detonation_period = 0.5f; // seconds - exactly like original
    texture_name = "explosion";
    
    // SPECTACULAR EXPLOSION EFFECTS!
    if (app->gpu_renderer) {
        // Set up spectacular explosion effect
        float explosion_radius = power * 60.0f; // Radius based on power
        app->gpu_renderer->set_explosion_effect(_x, _y, explosion_radius, 1.0f);
        
        // Emit spectacular particles
        app->gpu_renderer->emit_particles(_x, _y, power * 50, GPUAcceleratedRenderer::FIRE, nullptr, 2.0f);
        app->gpu_renderer->emit_particles(_x, _y, power * 30, GPUAcceleratedRenderer::SPARK, nullptr, 1.5f);
        app->gpu_renderer->emit_particles(_x, _y, power * 20, GPUAcceleratedRenderer::SMOKE, nullptr, 3.0f);
        
        SDL_Log("SPECTACULAR explosion effects activated at (%d,%d) with power %d!", _x, _y, power);
    }
    
    // Create additional particle effects
    ParticleSystem* explosion_sparks = new ParticleSystem(_x, _y, EXPLOSION_SPARKS, app);
    app->objects.push_back(explosion_sparks);
    
    ParticleSystem* dust_cloud = new ParticleSystem(_x, _y, DUST_CLOUDS, app);
    app->objects.push_back(dust_cloud);

    length_up = length_down = length_left = length_right = 0;

    // SDL_Log("Explosion constructor: Starting at (%d,%d) with power=%d", get_map_x(), get_map_y(), _power);

    // Calculate explosion lengths
    for (int i = 1; i <= power; ++i) {
        MapTile* tile = app->map->get_tile(get_map_x(), get_map_y() - i);
        if (!tile) break;
        length_up = i;
        const char* type_name = (tile->get_tile_type() == 1) ? "GROUND" : 
                                (tile->get_tile_type() == 2) ? "WALL" : 
                                (tile->get_tile_type() == 3) ? "BOX" : "UNKNOWN";
        // SDL_Log("Explosion constructor: UP ray distance %d at (%d,%d), type=%d (%s), burnable=%d, blocking=%d", 
        //         i, get_map_x(), get_map_y() - i, tile->get_tile_type(), type_name, tile->is_burnable(), tile->is_blocking());
        if (tile->is_burnable()) break;
        if (tile->is_blocking()) break;
    }
    for (int i = 1; i <= power; ++i) {
        MapTile* tile = app->map->get_tile(get_map_x(), get_map_y() + i);
        if (!tile) break;
        length_down = i;
        const char* type_name = (tile->get_tile_type() == 1) ? "GROUND" : 
                                (tile->get_tile_type() == 2) ? "WALL" : 
                                (tile->get_tile_type() == 3) ? "BOX" : "UNKNOWN";
        // SDL_Log("Explosion constructor: DOWN ray distance %d at (%d,%d), type=%d (%s), burnable=%d, blocking=%d", 
        //         i, get_map_x(), get_map_y() + i, tile->get_tile_type(), type_name, tile->is_burnable(), tile->is_blocking());
        if (tile->is_burnable()) break;
        if (tile->is_blocking()) break;
    }
    for (int i = 1; i <= power; ++i) {
        MapTile* tile = app->map->get_tile(get_map_x() - i, get_map_y());
        if (!tile) break;
        length_left = i;
        const char* type_name = (tile->get_tile_type() == 1) ? "GROUND" : 
                                (tile->get_tile_type() == 2) ? "WALL" : 
                                (tile->get_tile_type() == 3) ? "BOX" : "UNKNOWN";
        // SDL_Log("Explosion constructor: LEFT ray distance %d at (%d,%d), type=%d (%s), burnable=%d, blocking=%d", 
        //         i, get_map_x() - i, get_map_y(), tile->get_tile_type(), type_name, tile->is_burnable(), tile->is_blocking());
        if (tile->is_burnable()) break;
        if (tile->is_blocking()) break;
    }
    for (int i = 1; i <= power; ++i) {
        MapTile* tile = app->map->get_tile(get_map_x() + i, get_map_y());
        if (!tile) break;
        length_right = i;
        const char* type_name = (tile->get_tile_type() == 1) ? "GROUND" : 
                                (tile->get_tile_type() == 2) ? "WALL" : 
                                (tile->get_tile_type() == 3) ? "BOX" : "UNKNOWN";
        // SDL_Log("Explosion constructor: RIGHT ray distance %d at (%d,%d), type=%d (%s), burnable=%d, blocking=%d", 
        //         i, get_map_x() + i, get_map_y(), tile->get_tile_type(), type_name, tile->is_burnable(), tile->is_blocking());
        if (tile->is_burnable()) break;
        if (tile->is_blocking()) break;
    }

    detonate_other_bombs();
}

void Explosion::act(float deltaTime) {
    // Detonate other bombs, kill bombers, and explode corpses
    detonate_other_bombs();
    kill_bombers();
    explode_corpses();
    
    // Update explosion timer
    detonation_period -= deltaTime;
    if (detonation_period < 0) {
        // Clear spectacular explosion effects when explosion ends
        if (app->gpu_renderer) {
            app->gpu_renderer->set_explosion_effect(x, y, 0.0f, 0.0f); // Clear explosion effect
            SDL_Log("Explosion effects cleared at (%.0f,%.0f)", x, y);
        }
        delete_me = true;
    }
}

void Explosion::detonate_other_bombs() {
    // Center - destroy destructibles and detonate bombs
    MapTile* tile = app->map->get_tile(get_map_x(), get_map_y());
    if (tile) {
        if (tile->bomb) {
            tile->bomb->explode_delayed();
        }
        const char* type_name = (tile->get_tile_type() == 1) ? "GROUND" : 
                                (tile->get_tile_type() == 2) ? "WALL" : 
                                (tile->get_tile_type() == 3) ? "BOX" : "UNKNOWN";
        SDL_Log("Explosion: Checking tile at (%d,%d), type=%d (%s), destructible=%d", 
                tile->get_map_x(), tile->get_map_y(), tile->get_tile_type(), type_name, tile->is_destructible());
        if (tile->is_burnable()) {
            SDL_Log("Explosion: Destroying burnable tile at (%d,%d)", tile->get_map_x(), tile->get_map_y());
            tile->destroy();
        } else {
            SDL_Log("Explosion: Tile at (%d,%d) is not burnable (type=%d, %s)", 
                    tile->get_map_x(), tile->get_map_y(), tile->get_tile_type(), type_name);
        }
    }

    // Rays - destroy destructibles and detonate bombs along explosion path
    for (int i = 1; i <= length_up; ++i) {
        tile = app->map->get_tile(get_map_x(), get_map_y() - i);
        if (tile) {
            if (tile->bomb) tile->bomb->explode_delayed();
            if (tile->is_burnable()) {
                tile->destroy();
            }
        }
    }
    for (int i = 1; i <= length_down; ++i) {
        tile = app->map->get_tile(get_map_x(), get_map_y() + i);
        if (tile) {
            if (tile->bomb) tile->bomb->explode_delayed();
            if (tile->is_burnable()) {
                tile->destroy();
            }
        }
    }
    for (int i = 1; i <= length_left; ++i) {
        tile = app->map->get_tile(get_map_x() - i, get_map_y());
        if (tile) {
            if (tile->bomb) tile->bomb->explode_delayed();
            if (tile->is_burnable()) {
                tile->destroy();
            }
        }
    }
    for (int i = 1; i <= length_right; ++i) {
        tile = app->map->get_tile(get_map_x() + i, get_map_y());
        if (tile) {
            if (tile->bomb) tile->bomb->explode_delayed();
            if (tile->is_burnable()) {
                tile->destroy();
            }
        }
    }
}


// void Explosion::show() {
//     // Calculate explosion age (0.0 = just exploded, 0.5 = expired)
//     float explosion_age = 0.5f - detonation_period;
//     float normalized_time = explosion_age / 0.5f; // 0.0 to 1.0
    
//     // Only render if explosion is still active
//     if (normalized_time >= 1.0f) {
//         return;
//     }

//     // Draw explosion using OpenGL batch rendering with procedural shader effects
//     if (app && app->gpu_renderer) {
//         // Calculate tile-aligned center position
//         float tile_size = 40.0f;
//         int map_x = get_map_x();
//         int map_y = get_map_y();
        
//         // Asegurar que las coordenadas estén exactamente centradas en el tile
//         float aligned_x = map_x * tile_size + tile_size / 2.0f;  // Center of tile
//         float aligned_y = map_y * tile_size + tile_size / 2.0f;  // Center of tile
        
//         SDL_Log("DEBUG: Explosion center - original:(%.1f,%.1f) map:(%d,%d) aligned:(%.1f,%.1f)", 
//                 x, y, map_x, map_y, aligned_x, aligned_y);
        
//         // Set explosion information for shader
//         app->gpu_renderer->set_explosion_info(
//             aligned_x, aligned_y,                    // Tile-aligned center position
//             explosion_age,                           // Age of explosion
//             length_up, length_down,                  // Vertical reach
//             length_left, length_right                // Horizontal reach
//         );
        
//         // Use explosion shader mode
//         app->gpu_renderer->begin_batch(GPUAcceleratedRenderer::EXPLOSION_HEAT);
        
//         // Render each tile of the explosion separately using aligned coordinates
//         // This ensures proper depth sorting and allows the shader to work per-tile
        
//         // Center tile
//         draw_explosion_tile(aligned_x, aligned_y);
        
//         // Up direction
//         for (int i = 1; i <= length_up; i++) {
//             draw_explosion_tile(aligned_x, aligned_y - i * tile_size);
//         }
        
//         // Down direction
//         for (int i = 1; i <= length_down; i++) {
//             draw_explosion_tile(aligned_x, aligned_y + i * tile_size);
//         }
        
//         // Left direction
//         for (int i = 1; i <= length_left; i++) {
//             draw_explosion_tile(aligned_x - i * tile_size, aligned_y);
//         }
        
//         // Right direction
//         for (int i = 1; i <= length_right; i++) {
//             draw_explosion_tile(aligned_x + i * tile_size, aligned_y);
//         }
        
//         app->gpu_renderer->end_batch();
        
//         // Clear explosion information after rendering
//         app->gpu_renderer->clear_explosion_info();
//     }
// }

void Explosion::draw_explosion_tile(float tile_x, float tile_y) {
    if (!app || !app->gpu_renderer) {
        return;
    }
    
    // Use a dummy white texture or a simple 1x1 white pixel texture
    // The shader will generate the explosion effect procedurally
    GLuint dummy_texture = get_dummy_white_texture();
    
    float tile_size = 40.0f;
    float white_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float scale[2] = {1.0f, 1.0f};
    
    // Add sprite for this tile
    // The shader will detect it's in the explosion area and apply the effect
    // NOTE: add_sprite usa posición central del sprite
    app->gpu_renderer->add_sprite(
        tile_x, tile_y,               // Position (center) of this tile
        tile_size, tile_size,         // Size of one tile
        dummy_texture,                // Dummy texture (shader ignores this)
        white_color,                  // Color (full white, no tinting)
        0.0f,                        // No rotation
        scale,                       // No additional scaling
        0                            // Sprite frame (ignored)
    );
}

// Helper function to get or create a 1x1 white pixel texture
GLuint Explosion::get_dummy_white_texture() {
    static GLuint white_texture = 0;
    
    if (white_texture == 0) {
        // Create a 1x1 white pixel texture
        unsigned char white_pixel[4] = {255, 255, 255, 255};
        
        glGenTextures(1, &white_texture);
        glBindTexture(GL_TEXTURE_2D, white_texture);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    return white_texture;
}

void Explosion::show() {
    float explosion_age = 0.5f - detonation_period;
    float normalized_time = explosion_age / 0.5f;
    
    if (normalized_time >= 1.0f) {
        return;
    }

    if (app && app->gpu_renderer) {
        float tile_size = 40.0f;
        float half_tile = tile_size * 0.5f;
        
        // CORRECCIÓN PRINCIPAL: x,y vienen como esquina superior izquierda del tile
        // Necesitamos el CENTRO del tile para la explosión
        float explosion_center_x = x + half_tile;
        float explosion_center_y = y + half_tile;
        
        // Set explosion info con el centro CORRECTO
        app->gpu_renderer->set_explosion_info(
            explosion_center_x, explosion_center_y, explosion_age,
            length_up, length_down,
            length_left, length_right
        );
        
        app->gpu_renderer->begin_batch(GPUAcceleratedRenderer::EXPLOSION_HEAT);
        
        // Calcular el quad que cubre toda el área de explosión
        // Desde el centro, extenderse N tiles en cada dirección
        float min_x = explosion_center_x - (length_left + 0.5f) * tile_size;
        float max_x = explosion_center_x + (length_right + 0.5f) * tile_size;
        float min_y = explosion_center_y - (length_up + 0.5f) * tile_size;
        float max_y = explosion_center_y + (length_down + 0.5f) * tile_size;
        
        float width = max_x - min_x;
        float height = max_y - min_y;
        
        GLuint dummy_texture = get_dummy_white_texture();
        float white_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        float scale[2] = {1.0f, 1.0f};
        
        // add_sprite espera esquina superior izquierda
        app->gpu_renderer->add_sprite(
            min_x, min_y,
            width, height,
            dummy_texture,
            white_color,
            0.0f,
            scale,
            0
        );
        
        app->gpu_renderer->end_batch();
        app->gpu_renderer->clear_explosion_info();
    }
}

void Explosion::kill_bombers() {
    // Check for bombers in explosion area and kill them
    for (auto& bomber : app->bomber_objects) {
        if (bomber && !bomber->delete_me && !bomber->is_dead()) {
            int bomber_map_x = bomber->get_map_x();
            int bomber_map_y = bomber->get_map_y();
            
            // Check if bomber is in explosion center
            if (bomber_map_x == get_map_x() && bomber_map_y == get_map_y()) {
                bomber->die();
                continue;
            }
            
            // Check if bomber is in explosion rays
            // Up ray
            for (int i = 1; i <= length_up; ++i) {
                if (bomber_map_x == get_map_x() && bomber_map_y == get_map_y() - i) {
                    bomber->die();
                    break;
                }
            }
            // Down ray
            for (int i = 1; i <= length_down; ++i) {
                if (bomber_map_x == get_map_x() && bomber_map_y == get_map_y() + i) {
                    bomber->die();
                    break;
                }
            }
            // Left ray
            for (int i = 1; i <= length_left; ++i) {
                if (bomber_map_x == get_map_x() - i && bomber_map_y == get_map_y()) {
                    bomber->die();
                    break;
                }
            }
            // Right ray
            for (int i = 1; i <= length_right; ++i) {
                if (bomber_map_x == get_map_x() + i && bomber_map_y == get_map_y()) {
                    bomber->die();
                    break;
                }
            }
        }
    }
}

void Explosion::explode_corpses() {
    // Check for corpses in explosion area and make them explode with gore
    for (auto& obj : app->objects) {
        if (obj && obj->get_type() == BOMBER_CORPSE) {
            BomberCorpse* corpse = static_cast<BomberCorpse*>(obj);
            if (!corpse->is_exploded()) {
                int corpse_map_x = corpse->get_map_x();
                int corpse_map_y = corpse->get_map_y();
                
                // Check if corpse is in explosion area (same logic as bombers)
                bool in_explosion = false;
                
                // Center
                if (corpse_map_x == get_map_x() && corpse_map_y == get_map_y()) {
                    in_explosion = true;
                }
                
                // Rays
                if (!in_explosion) {
                    for (int i = 1; i <= length_up && !in_explosion; ++i) {
                        if (corpse_map_x == get_map_x() && corpse_map_y == get_map_y() - i) {
                            in_explosion = true;
                        }
                    }
                    for (int i = 1; i <= length_down && !in_explosion; ++i) {
                        if (corpse_map_x == get_map_x() && corpse_map_y == get_map_y() + i) {
                            in_explosion = true;
                        }
                    }
                    for (int i = 1; i <= length_left && !in_explosion; ++i) {
                        if (corpse_map_x == get_map_x() - i && corpse_map_y == get_map_y()) {
                            in_explosion = true;
                        }
                    }
                    for (int i = 1; i <= length_right && !in_explosion; ++i) {
                        if (corpse_map_x == get_map_x() + i && corpse_map_y == get_map_y()) {
                            in_explosion = true;
                        }
                    }
                }
                
                if (in_explosion) {
                    corpse->explode(); // This creates the gore explosion!
                }
            }
        }
    }
}

