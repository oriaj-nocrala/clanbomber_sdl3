#include "TileEntity.h"
#include "GameContext.h"
#include "Resources.h"
#include "AudioMixer.h"
#include "Extra.h"
#include "Timer.h"
#include "ParticleSystem.h"
#include "GPUAcceleratedRenderer.h"
#include "GameContext.h"
#include <random>
#include <cmath>
#include <SDL3/SDL.h>

// Static initialization for particle rate limiting
float TileEntity::last_particle_emission_time = 0.0f;
const float TileEntity::PARTICLE_EMISSION_COOLDOWN = 0.05f; // 50ms minimum between emissions

// === BASE TILEENTITY ===

TileEntity::TileEntity(MapTile_Pure* tile_data, GameContext* context) 
    : GameObject(tile_data->get_pixel_x(), tile_data->get_pixel_y(), context), tile_data(tile_data) {
    
    texture_name = "maptiles";
    sprite_nr = tile_data->get_sprite_number();
    destroyed = false;
    destroy_animation = 0.0f;
    
    // CRITICAL: Set proper Z-order so tiles render BEHIND other objects
    z = Z_GROUND;  // Tiles should be at ground level (0), behind bombs (3000)
    
    SDL_Log("TileEntity: Created entity for %s tile at (%d,%d)", 
           tile_data->get_type() == MapTile_Pure::GROUND ? "GROUND" : 
           tile_data->get_type() == MapTile_Pure::WALL ? "WALL" : 
           tile_data->get_type() == MapTile_Pure::BOX ? "BOX" : "UNKNOWN",
           get_map_x(), get_map_y());
}

TileEntity::~TileEntity() {
    if (tile_data) {
        delete tile_data;
        tile_data = nullptr;
    }
}

void TileEntity::act(float deltaTime) {
    // DEFENSIVE: Detect corruption early
    if (tile_data == (MapTile_Pure*)0xffffffffffffffff) {
        SDL_Log("CRITICAL: TileEntity::act() - tile_data corrupted to 0xffffffffffffffff at (%d,%d)!", get_x()/40, get_y()/40);
        // Try to recover by setting tile_data to nullptr to prevent further crashes
        tile_data = nullptr;
        return;
    }
    
    if (destroyed) {
        update_destruction_animation(deltaTime);
    }
    
    // Base TileEntity no hace nada más - subclases implementan comportamiento específico
}

void TileEntity::show() {
    if (!tile_data) return;
    
    // CRITICAL FIX: Simply render without LifecycleManager check during initialization
    // The LifecycleManager check can cause issues during initialization when GameContext
    // isn't fully set up yet. TileEntity rendering is safe without this check.
    
    // Just render if not destroyed - LifecycleManager will handle cleanup later
    // This fixes the black map issue caused by null context during show()
    
    if (!destroyed) {
        // Render normal tile
        GameObject::show();
    } else {
        // Render destruction effects
        render_destruction_effects();
    }
}

void TileEntity::destroy() {
    // DEFENSIVE: Check for corrupted tile_data
    if (!tile_data || tile_data == (MapTile_Pure*)0xffffffffffffffff) {
        SDL_Log("ERROR: TileEntity::destroy() - corrupted tile_data pointer: %p", tile_data);
        return;
    }
    
    if (!destroyed && tile_data->can_be_destroyed()) {
        SDL_Log("TileEntity: Destroying tile at (%d,%d)", get_map_x(), get_map_y());
        destroyed = true;
        destroy_animation = 0.0f;
        
        // Notify tile data
        tile_data->on_destruction_request();
        
        // LIFECYCLE INTEGRATION: Mark for destruction through LifecycleManager
        if (get_context() && get_context()->get_lifecycle_manager()) {
            get_context()->get_lifecycle_manager()->mark_tile_entity_for_destruction(this);
        }
        
        // Play destruction sound with 3D positioning
        AudioPosition tile_pos(get_x(), get_y(), 0.0f);
        AudioMixer::play_sound_3d("break", tile_pos, 500.0f);
    } else if (destroyed) {
        SDL_Log("WARNING: TileEntity::destroy() called on already destroyed tile at (%d,%d)", get_map_x(), get_map_y());
    }
}

void TileEntity::spawn_extra() {
    // Based on original ClanBomber spawn logic with balanced probabilities
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> main_dist(0, 7); // 8 main categories (0-7)
    
    int roll = main_dist(gen);
    Extra::EXTRA_TYPE extra_type;
    
    switch (roll) {
        case 0: // Power-ups (25% chance total)
            extra_type = Extra::FLAME;
            break;
        case 1: // Bomb capacity (12.5% chance)
            extra_type = Extra::BOMB;
            break;
        case 2: // Speed (12.5% chance)
            extra_type = Extra::SPEED;
            break;
        case 3: { // Special abilities (12.5% chance - kick or glove)
            std::uniform_int_distribution<> special_dist(0, 1);
            extra_type = (special_dist(gen) == 0) ? Extra::KICK : Extra::GLOVE;
            break;
        }
        case 4: { // Negative effects (12.5% chance)
            std::uniform_int_distribution<> negative_dist(0, 7);
            int neg_roll = negative_dist(gen);
            if (neg_roll == 0 || neg_roll == 1) {
                extra_type = Extra::DISEASE; // Constipation (25% of this case)
            } else if (neg_roll == 2 || neg_roll == 3) {
                extra_type = Extra::VIAGRA; // Sticky bombs (25% of this case)
            } else if (neg_roll == 4 || neg_roll == 5) {
                extra_type = Extra::KOKS; // Uncontrollable speed (25% of this case)
            } else {
                return; // No extra spawned (25% of this case)
            }
            break;
        }
        case 5: // Skate (rare, 6.25% chance)
            if (std::uniform_int_distribution<>(0, 1)(gen) == 0) {
                extra_type = Extra::SKATE;
            } else {
                return; // No extra
            }
            break;
        case 6:
        case 7:
        default:
            // Empty cases (25% chance total) - no extra spawned
            return;
    }
    
    // Create the extra at this tile's position using GameContext registration
    Extra* extra = new Extra(get_x(), get_y(), extra_type, get_context());
    get_context()->register_object(extra);
}

void TileEntity::update_destruction_animation(float deltaTime) {
    if (destroyed) {
        destroy_animation += deltaTime;
        
        // Check if animation completes and handle lifecycle through LifecycleManager
        if (destroy_animation >= 0.5f && !delete_me) {
            SDL_Log("TileEntity at (%d,%d) completing destruction animation", get_map_x(), get_map_y());
            
            // Spawn power-up now that explosion has ended (only once!)
            spawn_extra();
            
            // LIFECYCLE INTEGRATION: Let LifecycleManager handle the final deletion
            // LifecycleManager will set delete_me when appropriate
        }
    }
}

void TileEntity::render_destruction_effects() {
    // Base implementation - subclasses override for specific effects
    if (destroyed) {
        float animation_progress = destroy_animation / 0.5f; // 0.0 to 1.0
        animation_progress = std::min(animation_progress, 1.0f);
        
        // Simple fade effect for base tiles
        if (get_context() && get_context()->get_renderer()) {
            GLuint gl_texture = Resources::get_gl_texture(texture_name);
            if (gl_texture) {
                float alpha = 1.0f - animation_progress;
                float color[4] = {1.0f, 1.0f, 1.0f, alpha};
                float scale[2] = {1.0f, 1.0f};
                
                get_context()->get_renderer()->add_sprite(
                    (float)get_x(), (float)get_y(), 40.0f, 40.0f,
                    gl_texture, color, 0.0f, scale, sprite_nr
                );
            }
        }
    }
}

// === TILEENTITY_BOX ESPECIALIZADO ===

TileEntity_Box::TileEntity_Box(MapTile_Pure* tile_data, GameContext* context) 
    : TileEntity(tile_data, context) {
    // Box-specific initialization if needed
}

void TileEntity_Box::act(float deltaTime) {
    TileEntity::act(deltaTime); // Base destruction animation
    
    if (destroyed) {
        // Add smoke particles during destruction animation  
        static float last_smoke_time = 0.0f;
        if (destroy_animation > 0.1f && last_smoke_time <= 0.1f) {
            ParticleSystem* smoke = new ParticleSystem(get_x(), get_y(), SMOKE_TRAILS, get_context());
            get_context()->register_object(smoke);
            last_smoke_time = destroy_animation;
        }
    }
}

void TileEntity_Box::show() {
    if (!destroyed) {
        TileEntity::show(); // Normal tile rendering
    } else {
        render_fragmentation_effects(); // Spectacular box destruction
    }
}

void TileEntity_Box::destroy() {
    TileEntity::destroy(); // Base destruction
    
    if (destroyed) {
        // RATE LIMITING: Check if enough time has passed since last particle emission
        float current_time = Timer::time_elapsed();
        bool can_emit = (current_time - last_particle_emission_time) >= PARTICLE_EMISSION_COOLDOWN;
        
        if (can_emit) {
            // SPECTACULAR tile fragmentation effects!
            if (get_context()->get_renderer()) {
                // Emit debris particles
                get_context()->get_renderer()->emit_particles(get_x(), get_y(), 25, GPUAcceleratedRenderer::SPARK, nullptr, 1.0f);
                get_context()->get_renderer()->emit_particles(get_x(), get_y(), 15, GPUAcceleratedRenderer::SMOKE, nullptr, 2.0f);
                
                SDL_Log("SPECTACULAR tile destruction effects at (%d,%d)!", get_x(), get_y());
            }
            
            // Add traditional particle effects for destruction using GameContext registration
            ParticleSystem* dust = new ParticleSystem(get_x(), get_y(), DUST_CLOUDS, get_context());
            get_context()->register_object(dust);
            
            ParticleSystem* sparks = new ParticleSystem(get_x(), get_y(), EXPLOSION_SPARKS, get_context());
            get_context()->register_object(sparks);
            
            // Update rate limiting timestamp
            last_particle_emission_time = current_time;
        } else {
            SDL_Log("Particle emission rate limited for tile at (%d,%d)", get_x(), get_y());
        }
    }
}

void TileEntity_Box::render_fragmentation_effects() {
    // SPECTACULAR GPU fragmentation effects enabled
    float animation_progress = destroy_animation / 0.5f; // 0.0 to 1.0
    animation_progress = std::min(animation_progress, 1.0f);
    
    if (get_context() && get_context()->get_renderer()) {
        GLuint gl_texture = Resources::get_gl_texture(texture_name);
        if (gl_texture) {
            try {
                get_context()->get_renderer()->begin_batch(GPUAcceleratedRenderer::TILE_FRAGMENTATION);
                
                // Create realistic wood/debris fragments from destroyed box!
                const int num_fragments = 18;  // More small pieces for realistic debris
                for (int i = 0; i < num_fragments; i++) {
                    // Much smaller, irregular fragment sizes (real debris pieces)
                    float fragment_scale = 0.15f + (i % 4) * 0.08f; // 0.15, 0.23, 0.31, 0.39 sizes
                    fragment_scale *= (1.0f - animation_progress * 0.4f); // Slight shrinking
                    
                    // REALISTIC EXPLOSION PHYSICS: Each fragment gets individual blast vector
                    float base_angle = (float)i / num_fragments * 6.28318f;
                    float blast_chaos = (i * 127 + 31) % 100 / 100.0f - 0.5f; // -0.5 to 0.5 chaos
                    float angle = base_angle + blast_chaos * 1.2f; // More dramatic angle variation
                    
                    // Fragment mass affects how far it flies (heavier = shorter distance)
                    float fragment_mass = fragment_scale * 2.0f + 0.5f; // Mass 0.8-1.3
                    float base_explosion_force = 45.0f + (i % 7) * 10.0f; // 45-105 force range
                    float explosion_force = base_explosion_force / fragment_mass; // F=ma physics
                    
                    // Initial velocity with air resistance decay
                    float time_factor = animation_progress;
                    float air_drag = 1.0f - (animation_progress * 0.6f); // Air resistance slows fragments
                    float current_velocity = explosion_force * air_drag;
                    
                    // Different fragment shapes using non-uniform scaling (base)
                    float scale_x = fragment_scale * (0.8f + (i % 3) * 0.4f); // 0.8x to 2.0x width
                    float scale_y = fragment_scale * (0.6f + ((i * 7) % 4) * 0.3f); // 0.6x to 1.8x height
                    
                    // Position based on physics integration: x = x₀ + v₀t + ½at²
                    float fragment_x = get_x() + cos(angle) * current_velocity * time_factor;
                    float fragment_y = get_y() + sin(angle) * current_velocity * time_factor;
                    
                    // TOP-DOWN VIEW PHYSICS: Simulate 3D motion projected to 2D
                    // Z-axis represents height above the ground (positive = higher)
                    float initial_z_velocity = 25.0f + (i % 6) * 8.0f; // Initial upward velocity 25-65
                    initial_z_velocity /= fragment_mass; // Lighter fragments fly higher
                    
                    // Z position over time with gravity (z = v₀t - ½gt²)
                    float gravity_z = 120.0f; // Gravity pulling down in Z
                    float fragment_z = initial_z_velocity * time_factor - 0.5f * gravity_z * time_factor * time_factor;
                    
                    // Project Z height into 2D visual effects:
                    // 1. Scale: higher fragments appear larger (closer to camera)
                    float height_scale = 1.0f + fragment_z * 0.02f; // Scale based on height
                    scale_x *= std::max(0.1f, height_scale);
                    scale_y *= std::max(0.1f, height_scale);
                    
                    // 2. Shadow offset: simulate shadow beneath fragment
                    float shadow_offset_x = fragment_z * 0.3f; // Shadow displaced by height
                    float shadow_offset_y = fragment_z * 0.2f;
                    fragment_x += shadow_offset_x;
                    fragment_y += shadow_offset_y;
                    
                    // Realistic alpha fade - complete fade to 0 at animation_progress = 1.0
                    float fragment_alpha = (1.0f - animation_progress) * (0.7f + (i % 3) * 0.15f);
                    
                    // 3. Bounce when fragment hits ground (z ≤ 0)
                    if (fragment_z <= 0.0f && time_factor > 0.2f) {
                        // Fragment is on/below ground - add bounce chaos
                        float bounce_energy = abs(fragment_z) * 0.5f; // Energy based on impact
                        float bounce_chaos = sin(time_factor * 12.0f + i * 2.3f) * bounce_energy;
                        
                        // Random bouncing in XY plane
                        fragment_x += bounce_chaos * cos(angle + 0.7f);
                        fragment_y += bounce_chaos * sin(angle + 0.7f);
                        
                        // Dim fragments on ground (less lighting from above)
                        fragment_alpha *= 0.7f;
                    }
                    
                    // REALISTIC PHYSICS: Angular momentum from explosion impact!
                    // Each fragment gets hit by explosion with different force and angle
                    float impact_angle_offset = (i * 73) % 360; // Random impact angle per fragment
                    float impact_force = 0.8f + (i % 5) * 0.4f; // Impact strength 0.8-2.0x
                    
                    // Initial angular velocity depends on fragment size and impact
                    float fragment_inertia = fragment_scale * fragment_scale; // Smaller = spins faster
                    float initial_angular_velocity = (800.0f + (i % 9) * 300.0f) * impact_force / fragment_inertia;
                    
                    // Different spin directions based on impact vector
                    float spin_direction = ((i % 2) == 0) ? 1.0f : -1.0f;
                    if ((i % 3) == 0) spin_direction *= -1.0f; // More chaos
                    initial_angular_velocity *= spin_direction;
                    
                    // Air resistance slows down rotation over time (realistic!)
                    float air_resistance = 0.3f + animation_progress * 2.0f; // Increases over time
                    float current_angular_velocity = initial_angular_velocity * (1.0f - air_resistance);
                    
                    // Tumbling effect: fragments wobble as they lose stability
                    float tumble_factor = animation_progress * animation_progress * 150.0f;
                    float tumble_wobble = sin(animation_progress * 8.0f + i) * tumble_factor;
                    
                    // Final rotation combines: initial spin + momentum decay + tumbling
                    float rotation = (i * 23.0f) + // Random initial orientation
                                   (animation_progress * current_angular_velocity) + // Physics spin
                                   tumble_wobble; // Realistic wobbling
                    
                    // REALISTIC WOOD DESTRUCTION: Different fragment types based on position
                    
                    // Determine fragment type based on index for realistic variety
                    enum FragmentType {
                        CORNER_PIECE = 0,    // Edge/corner fragments (darker, weathered)
                        EDGE_PLANK = 1,      // Side planks (medium tone)
                        INNER_WOOD = 2,      // Fresh inner wood (lighter)
                        SPLINTER = 3         // Small splinters (very light)
                    };
                    
                    FragmentType frag_type = (FragmentType)(i % 4);
                    
                    // Realistic wood coloring based on fragment type
                    float base_r, base_g, base_b;
                    switch(frag_type) {
                        case CORNER_PIECE:  // Weathered exterior wood
                            base_r = 0.8f; base_g = 0.6f; base_b = 0.4f;
                            break;
                        case EDGE_PLANK:    // Normal surface wood
                            base_r = 0.9f; base_g = 0.7f; base_b = 0.5f;
                            break;
                        case INNER_WOOD:    // Fresh inner wood (lighter)
                            base_r = 1.0f; base_g = 0.85f; base_b = 0.7f;
                            break;
                        case SPLINTER:      // Bright wood splinters
                            base_r = 1.0f; base_g = 0.9f; base_b = 0.8f;
                            break;
                    }
                    
                    // Slight random variation (but keep colors bright)
                    float color_var = (i % 7) * 0.05f - 0.025f; // -0.025 to +0.025
                    
                    float color[4] = {
                        std::min(1.0f, std::max(0.3f, base_r + color_var)),
                        std::min(1.0f, std::max(0.3f, base_g + color_var)),
                        std::min(1.0f, std::max(0.3f, base_b + color_var)),
                        fragment_alpha
                    };
                    float scale[2] = {scale_x, scale_y};
                    
                    // INTELLIGENT SIZE VARIATION: Different fragment types = different sizes
                    float fragment_size;
                    switch(frag_type) {
                        case CORNER_PIECE:  fragment_size = 20.0f + (i % 2) * 8.0f; break; // 20-28px
                        case EDGE_PLANK:    fragment_size = 16.0f + (i % 3) * 6.0f; break; // 16-28px  
                        case INNER_WOOD:    fragment_size = 14.0f + (i % 2) * 4.0f; break; // 14-18px
                        case SPLINTER:      fragment_size = 8.0f + (i % 3) * 4.0f; break;  // 8-16px
                    }
                    
                    get_context()->get_renderer()->add_animated_sprite(
                        fragment_x - (fragment_size * scale_x * 0.5f),
                        fragment_y - (fragment_size * scale_y * 0.5f),
                        fragment_size, fragment_size,
                        gl_texture, color, rotation, scale,
                        GPUAcceleratedRenderer::TILE_FRAGMENTATION,
                        sprite_nr
                    );
                }
                
                get_context()->get_renderer()->end_batch();
                return; // Spectacular GPU effect rendered successfully!
                
            } catch (...) {
                SDL_Log("GPU fragmentation failed, falling back to base effect");
                // Fall through to base rendering
            }
        }
    }
    
    // Fallback to base destruction effects
    render_destruction_effects();
}