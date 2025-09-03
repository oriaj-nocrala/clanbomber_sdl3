#include "Explosion.h"
#include "Bomber.h"
#include "BomberCorpse.h"
#include "Map.h"
#include "MapTile.h"
#include "TileEntity.h"
#include "TileManager.h"
#include "Resources.h"
#include "Bomb.h"
#include "Timer.h"
#include "ParticleSystem.h"
#include "GPUAcceleratedRenderer.h"
#include "GameContext.h"
#include "SpatialPartitioning.h"
#include "CoordinateSystem.h"
#include "MemoryManagement.h"
#include "RenderingFacade.h"
#include "Controller_Joystick.h"
#include <SDL3/SDL_timer.h>

// Phase 3: Import CoordinateConfig constants
static constexpr int TILE_SIZE = CoordinateConfig::TILE_SIZE;

Explosion::Explosion(int _x, int _y, int _power, Bomber* _owner, GameContext* context) : GameObject(_x, _y, context) {
    owner = _owner;
    power = _power;
    detonation_period = 1.2f; // seconds - increased from 0.5f for better deltaTime calibration
    texture_name = "explosion";
    
    // CRITICAL: Set Z-order so explosions render ABOVE tiles and bombs
    z = Z_EXPLOSION;  // Explosions should be above tiles (2000 > 0) and bombs (2000 < 3000)
    
    // SPECTACULAR EXPLOSION EFFECTS!
    if (get_context()->get_rendering_facade()) {
        GPUAcceleratedRenderer* gpu_renderer = get_context()->get_rendering_facade()->get_gpu_renderer();
        if (gpu_renderer) {
            // Set up spectacular explosion effect
            float explosion_radius = power * 60.0f; // Radius based on power
            gpu_renderer->set_explosion_effect(_x, _y, explosion_radius, 1.0f);
            
            // Emit spectacular particles
            gpu_renderer->emit_particles(_x, _y, power * 50, GPUAcceleratedRenderer::FIRE, nullptr, 2.0f);
            gpu_renderer->emit_particles(_x, _y, power * 30, GPUAcceleratedRenderer::SPARK, nullptr, 1.5f);
            gpu_renderer->emit_particles(_x, _y, power * 20, GPUAcceleratedRenderer::SMOKE, nullptr, 3.0f);
            
            SDL_Log("SPECTACULAR explosion effects activated at (%d,%d) with power %d!", _x, _y, power);
        }
    }
    
    // Create additional particle effects using ObjectPool pattern
    ParticleSystem* explosion_sparks = GameObjectFactory::getInstance().create_particle_system(_x, _y, EXPLOSION_SPARKS, get_context());
    
    ParticleSystem* dust_cloud = GameObjectFactory::getInstance().create_particle_system(_x, _y, DUST_CLOUDS, get_context());

    length_up = length_down = length_left = length_right = 0;

    // SDL_Log("Explosion constructor: Starting at (%d,%d) with power=%d", get_map_x(), get_map_y(), _power);

    // Calculate explosion lengths using NEW ARCHITECTURE
    for (int i = 1; i <= power; ++i) {
        length_up = i;
        if (is_tile_blocking_at(get_map_x(), get_map_y() - i)) break;
    }
    for (int i = 1; i <= power; ++i) {
        length_down = i;
        if (is_tile_blocking_at(get_map_x(), get_map_y() + i)) break;
    }
    for (int i = 1; i <= power; ++i) {
        length_left = i;
        if (is_tile_blocking_at(get_map_x() - i, get_map_y())) break;
    }
    for (int i = 1; i <= power; ++i) {
        length_right = i;
        if (is_tile_blocking_at(get_map_x() + i, get_map_y())) break;
    }

    detonate_other_bombs();
    
    // Trigger haptic feedback for all joystick controllers
    notify_explosion_haptics();
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
        if (get_context()->get_rendering_facade()) {
            GPUAcceleratedRenderer* gpu_renderer = get_context()->get_rendering_facade()->get_gpu_renderer();
            if (gpu_renderer) {
                gpu_renderer->set_explosion_effect(x, y, 0.0f, 0.0f); // Clear explosion effect
                SDL_Log("Explosion effects cleared at (%.0f,%.0f) after full duration", x, y);
            }
        }
        delete_me = true;
    }
}

void Explosion::detonate_other_bombs() {
    // Center - destroy destructibles and detonate bombs using NEW ARCHITECTURE
    destroy_tile_at(get_map_x(), get_map_y());

    // Rays - destroy destructibles and detonate bombs along explosion path using NEW ARCHITECTURE
    for (int i = 1; i <= length_up; ++i) {
        destroy_tile_at(get_map_x(), get_map_y() - i);
    }
    for (int i = 1; i <= length_down; ++i) {
        destroy_tile_at(get_map_x(), get_map_y() + i);
    }
    for (int i = 1; i <= length_left; ++i) {
        destroy_tile_at(get_map_x() - i, get_map_y());
    }
    for (int i = 1; i <= length_right; ++i) {
        destroy_tile_at(get_map_x() + i, get_map_y());
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
//     if (app && get_context()->get_renderer()) {
//         // Calculate tile-aligned center position
//         float tile_size = static_cast<float>(TILE_SIZE);
//         int map_x = get_map_x();
//         int map_y = get_map_y();
        
//         // Asegurar que las coordenadas estén exactamente centradas en el tile
//         float aligned_x = map_x * tile_size + tile_size / 2.0f;  // Center of tile
//         float aligned_y = map_y * tile_size + tile_size / 2.0f;  // Center of tile
        
//         SDL_Log("DEBUG: Explosion center - original:(%.1f,%.1f) map:(%d,%d) aligned:(%.1f,%.1f)", 
//                 x, y, map_x, map_y, aligned_x, aligned_y);
        
//         // Set explosion information for shader
//         get_context()->get_renderer()->set_explosion_info(
//             aligned_x, aligned_y,                    // Tile-aligned center position
//             explosion_age,                           // Age of explosion
//             length_up, length_down,                  // Vertical reach
//             length_left, length_right                // Horizontal reach
//         );
        
//         // Use explosion shader mode
//         get_context()->get_renderer()->begin_batch(GPUAcceleratedRenderer::EXPLOSION_HEAT);
        
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
        
//         get_context()->get_renderer()->end_batch();
        
//         // Clear explosion information after rendering
//         get_context()->get_renderer()->clear_explosion_info();
//     }
// }

void Explosion::draw_explosion_tile(float tile_x, float tile_y) {
    if (!get_context() || !get_context()->get_rendering_facade()) {
        return;
    }
    
    GPUAcceleratedRenderer* gpu_renderer = get_context()->get_rendering_facade()->get_gpu_renderer();
    if (!gpu_renderer) {
        return;
    }
    
    // Use a dummy white texture or a simple 1x1 white pixel texture
    // The shader will generate the explosion effect procedurally
    GLuint dummy_texture = get_dummy_white_texture();
    
    float tile_size = static_cast<float>(TILE_SIZE);
    float white_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float scale[2] = {1.0f, 1.0f};
    
    // Add sprite for this tile
    // The shader will detect it's in the explosion area and apply the effect
    // NOTE: add_sprite usa posición central del sprite
    gpu_renderer->add_sprite(
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

    // CORRECT APPROACH: Single quad for entire explosion cross - shader draws the shape
    if (get_context() && get_context()->get_rendering_facade()) {
        GPUAcceleratedRenderer* gpu_renderer = get_context()->get_rendering_facade()->get_gpu_renderer();
        if (gpu_renderer) {
            float tile_size = static_cast<float>(TILE_SIZE);
            int map_x = get_map_x();
            int map_y = get_map_y();
            
            // Calculate tile-aligned center position
            float center_x = map_x * tile_size + tile_size / 2.0f;  // Center of tile
            float center_y = map_y * tile_size + tile_size / 2.0f;  // Center of tile
            
            SDL_Log("DEBUG: Explosion rendering at age=%.3f, center=(%.1f,%.1f), lengths=(%d,%d,%d,%d)", 
                    explosion_age, center_x, center_y, length_up, length_down, length_left, length_right);
            
            // Set explosion information for shader
            gpu_renderer->set_explosion_info(
                center_x, center_y,                      // Explosion center
                explosion_age,                           // Age of explosion
                length_up, length_down,                  // Vertical reach
                length_left, length_right                // Horizontal reach
            );
            
            // Use explosion shader mode
            gpu_renderer->begin_batch(GPUAcceleratedRenderer::EXPLOSION_HEAT);
            
            // Calculate bounding box that covers the entire explosion cross
            float max_extent = std::max(std::max(length_up, length_down), std::max(length_left, length_right));
            float box_size = (max_extent + 1) * tile_size * 2;  // +1 for center, *2 for both directions
            
            // Position box so explosion center is at the center of the box
            float box_x = center_x - box_size / 2.0f;
            float box_y = center_y - box_size / 2.0f;
            
            GLuint dummy_texture = get_dummy_white_texture();
            float white_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            float scale[2] = {1.0f, 1.0f};
            
            // Single sprite covering entire explosion area - shader will draw the cross shape
            gpu_renderer->add_sprite(
                box_x, box_y,                           // Top-left corner of bounding box
                box_size, box_size,                     // Size covers entire explosion
                dummy_texture,
                white_color,
                0.0f,
                scale,
                0
            );
            
            gpu_renderer->end_batch();
            gpu_renderer->clear_explosion_info();
        }
    }
}

void Explosion::kill_bombers() {
    // OPTIMIZED: Check for bombers in explosion area using SpatialGrid O(n) instead of O(n²)
    GameContext* ctx = get_context();
    if (!ctx) {
        SDL_Log("ERROR: Explosion::kill_bombers() - No GameContext available");
        return;
    }
    
    SpatialGrid* spatial_grid = ctx->get_spatial_grid();
    if (spatial_grid) {
        // Use spatial partitioning for efficient explosion victim detection
        CollisionHelper collision_helper(spatial_grid);
        
        // Generate list of explosion coordinates
        std::vector<GridCoord> explosion_area;
        
        // Center
        explosion_area.push_back(GridCoord(get_map_x(), get_map_y()));
        SDL_Log("EXPLOSION AREA: Center at (%d,%d)", get_map_x(), get_map_y());
        
        // Rays
        for (int i = 1; i <= length_up; ++i) {
            GridCoord coord(get_map_x(), get_map_y() - i);
            explosion_area.push_back(coord);
            SDL_Log("EXPLOSION AREA: Up ray %d at (%d,%d)", i, coord.grid_x, coord.grid_y);
        }
        for (int i = 1; i <= length_down; ++i) {
            GridCoord coord(get_map_x(), get_map_y() + i);
            explosion_area.push_back(coord);
            SDL_Log("EXPLOSION AREA: Down ray %d at (%d,%d)", i, coord.grid_x, coord.grid_y);
        }
        for (int i = 1; i <= length_left; ++i) {
            GridCoord coord(get_map_x() - i, get_map_y());
            explosion_area.push_back(coord);
            SDL_Log("EXPLOSION AREA: Left ray %d at (%d,%d)", i, coord.grid_x, coord.grid_y);
        }
        for (int i = 1; i <= length_right; ++i) {
            GridCoord coord(get_map_x() + i, get_map_y());
            explosion_area.push_back(coord);
            SDL_Log("EXPLOSION AREA: Right ray %d at (%d,%d)", i, coord.grid_x, coord.grid_y);
        }
        
        SDL_Log("EXPLOSION AREA: Total %zu coordinates in explosion area", explosion_area.size());
        
        // Find all bombers and corpses in explosion area efficiently
        std::vector<GameObject*> victims = collision_helper.find_explosion_victims(explosion_area);
        
        for (GameObject* victim : victims) {
            if (victim && victim->get_type() == GameObject::BOMBER) {
                Bomber* bomber = static_cast<Bomber*>(victim);
                        
                if (bomber && !bomber->delete_me && !bomber->is_dead()) {
                    SDL_Log("Explosion killed bomber at (%d,%d) using SpatialGrid O(n)", 
                        bomber->get_map_x(), bomber->get_map_y());
                    
                    // Trigger death haptic feedback for this specific bomber
                    Controller* controller = bomber->get_controller();
                    if (controller && controller->get_type() >= Controller::JOYSTICK_1 && controller->get_type() <= Controller::JOYSTICK_8) {
                        Controller_Joystick* joystick_controller = static_cast<Controller_Joystick*>(controller);
                        joystick_controller->trigger_explosion_vibration(
                            x, y, power, bomber->get_x(), bomber->get_y(), true  // true = bomber died
                        );
                        SDL_Log("HAPTIC: Death vibration triggered for bomber at (%d,%d)", bomber->get_x(), bomber->get_y());
                    }
                    
                    bomber->die();
                }
            }
        }
    } else {
        // FALLBACK: Use legacy O(n²) method if spatial grid not available
        const std::list<class GameObject*>& object_lists = ctx->get_object_lists();
        
        for (auto& obj : object_lists) {
            if (!obj || obj->get_type() != GameObject::BOMBER) continue;
            
            Bomber* bomber = static_cast<Bomber*>(obj);
            if (bomber && !bomber->delete_me && !bomber->is_dead()) {
                int bomber_map_x = bomber->get_map_x();
                int bomber_map_y = bomber->get_map_y();
                
                bool in_explosion = false;
                
                // Check center
                if (bomber_map_x == get_map_x() && bomber_map_y == get_map_y()) {
                    in_explosion = true;
                } else {
                    // Check rays
                    for (int i = 1; i <= length_up && !in_explosion; ++i) {
                        if (bomber_map_x == get_map_x() && bomber_map_y == get_map_y() - i) {
                            in_explosion = true;
                        }
                    }
                    for (int i = 1; i <= length_down && !in_explosion; ++i) {
                        if (bomber_map_x == get_map_x() && bomber_map_y == get_map_y() + i) {
                            in_explosion = true;
                        }
                    }
                    for (int i = 1; i <= length_left && !in_explosion; ++i) {
                        if (bomber_map_x == get_map_x() - i && bomber_map_y == get_map_y()) {
                            in_explosion = true;
                        }
                    }
                    for (int i = 1; i <= length_right && !in_explosion; ++i) {
                        if (bomber_map_x == get_map_x() + i && bomber_map_y == get_map_y()) {
                            in_explosion = true;
                        }
                    }
                }
                
                if (in_explosion) {
                    SDL_Log("Explosion killed bomber at (%d,%d) using legacy O(n²)", bomber_map_x, bomber_map_y);
                    
                    // Trigger death haptic feedback for this specific bomber
                    Controller* controller = bomber->get_controller();
                    if (controller && controller->get_type() >= Controller::JOYSTICK_1 && controller->get_type() <= Controller::JOYSTICK_8) {
                        Controller_Joystick* joystick_controller = static_cast<Controller_Joystick*>(controller);
                        joystick_controller->trigger_explosion_vibration(
                            x, y, power, bomber->get_x(), bomber->get_y(), true  // true = bomber died
                        );
                        SDL_Log("HAPTIC: Death vibration triggered for bomber at (%d,%d)", bomber->get_x(), bomber->get_y());
                    }
                    
                    bomber->die();
                }
            }
        }
    }
}

void Explosion::explode_corpses() {
    // OPTIMIZED: Check for corpses in explosion area using SpatialGrid O(n) instead of O(n²)
    GameContext* ctx = get_context();
    if (!ctx) {
        SDL_Log("ERROR: Explosion::explode_corpses() - No GameContext available");
        return;
    }
    
    SpatialGrid* spatial_grid = ctx->get_spatial_grid();
    if (spatial_grid) {
        // Use spatial partitioning for efficient explosion victim detection
        CollisionHelper collision_helper(spatial_grid);
        
        // Generate list of explosion coordinates (reuse same logic as kill_bombers)
        std::vector<GridCoord> explosion_area;
        
        // Center
        explosion_area.push_back(GridCoord(get_map_x(), get_map_y()));
        
        // Rays
        for (int i = 1; i <= length_up; ++i) {
            explosion_area.push_back(GridCoord(get_map_x(), get_map_y() - i));
        }
        for (int i = 1; i <= length_down; ++i) {
            explosion_area.push_back(GridCoord(get_map_x(), get_map_y() + i));
        }
        for (int i = 1; i <= length_left; ++i) {
            explosion_area.push_back(GridCoord(get_map_x() - i, get_map_y()));
        }
        for (int i = 1; i <= length_right; ++i) {
            explosion_area.push_back(GridCoord(get_map_x() + i, get_map_y()));
        }
        
        // Find all victims in explosion area efficiently
        std::vector<GameObject*> victims = collision_helper.find_explosion_victims(explosion_area);
        
        for (GameObject* victim : victims) {
            if (victim && victim->get_type() == GameObject::BOMBER_CORPSE) {
                BomberCorpse* corpse = static_cast<BomberCorpse*>(victim);
                if (corpse && !corpse->is_exploded()) {
                    SDL_Log("Corpse at (%d,%d) exploded due to explosion using SpatialGrid O(n)", 
                        corpse->get_map_x(), corpse->get_map_y());
                    corpse->explode(); // This creates the gore explosion!
                }
            }
        }
    } else {
        // FALLBACK: Use legacy O(n²) method if spatial grid not available
        const std::list<class GameObject*>& object_lists = ctx->get_object_lists();
        
        for (auto& obj : object_lists) {
            if (obj && obj->get_type() == BOMBER_CORPSE) {
                BomberCorpse* corpse = static_cast<BomberCorpse*>(obj);
                if (!corpse->is_exploded()) {
                    int corpse_map_x = corpse->get_map_x();
                    int corpse_map_y = corpse->get_map_y();
                    
                    bool in_explosion = false;
                    
                    // Check center
                    if (corpse_map_x == get_map_x() && corpse_map_y == get_map_y()) {
                        in_explosion = true;
                    } else {
                        // Check rays
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
                        SDL_Log("Corpse at (%d,%d) exploded due to explosion using legacy O(n²)", corpse_map_x, corpse_map_y);
                        corpse->explode(); // This creates the gore explosion!
                    }
                }
            }
        }
    }
}

// NEW ARCHITECTURE SUPPORT: Check if tile is blocking in both architectures
bool Explosion::is_tile_blocking_at(int map_x, int map_y) {
    // Check legacy MapTile
    MapTile* legacy_tile = get_context()->get_map()->get_tile(map_x, map_y);
    if (legacy_tile) {
        if (legacy_tile->is_blocking() || legacy_tile->is_burnable()) {
            return true;  // Blocking or destructible tiles stop explosion
        }
    }
    
    // Check new TileEntity
    TileEntity* tile_entity = get_context()->get_map()->get_tile_entity(map_x, map_y);
    if (tile_entity) {
        if (tile_entity->is_blocking() || tile_entity->is_destructible()) {
            return true;  // Blocking or destructible tiles stop explosion
        }
    }
    
    return false;  // Tile is not blocking
}

// NEW ARCHITECTURE SUPPORT: Delegate to TileManager for intelligent coordination
void Explosion::destroy_tile_at(int map_x, int map_y) {
    // ARCHITECTURE PATTERN: Delegate to TileManager for dual architecture coordination
    // This avoids the corruption issues we were seeing from direct manipulation
    if (get_context()->get_tile_manager()) {
        SDL_Log("Explosion: Requesting tile destruction at (%d,%d) via TileManager", map_x, map_y);
        get_context()->get_tile_manager()->request_tile_destruction(map_x, map_y);
    } else {
        SDL_Log("WARNING: No TileManager available for tile destruction at (%d,%d)", map_x, map_y);
    }
}

void Explosion::notify_explosion_haptics() {
    GameContext* ctx = get_context();
    if (!ctx) {
        return;
    }
    
    // Get all bombers to check which ones use joystick controllers
    const std::list<class GameObject*>& object_lists = ctx->get_object_lists();
    
    for (auto& obj : object_lists) {
        if (!obj || obj->get_type() != GameObject::BOMBER) continue;
        
        Bomber* bomber = static_cast<Bomber*>(obj);
        if (!bomber || bomber->delete_me) continue;
        
        Controller* controller = bomber->get_controller();
        if (!controller) continue;
        
        // Check if this is a joystick controller
        Controller::CONTROLLER_TYPE controller_type = controller->get_type();
        if (controller_type >= Controller::JOYSTICK_1 && controller_type <= Controller::JOYSTICK_8) {
            Controller_Joystick* joystick_controller = static_cast<Controller_Joystick*>(controller);
            
            // Check if this bomber was killed by this explosion
            bool bomber_died = bomber->is_dead();
            
            // Trigger haptic feedback with physics-based calculation
            joystick_controller->trigger_explosion_vibration(
                x, y,                           // Explosion position (world coordinates)
                power,                          // Explosion power
                bomber->get_x(), bomber->get_y(),  // Bomber position (world coordinates)
                bomber_died                     // Did the bomber die?
            );
            
            SDL_Log("HAPTIC: Notified joystick controller for bomber at (%d,%d) about explosion at (%.1f,%.1f) power=%d died=%s", 
                    bomber->get_x(), bomber->get_y(), x, y, power, bomber_died ? "true" : "false");
        }
    }
}

