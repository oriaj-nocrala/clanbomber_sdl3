#include "CorpsePart.h"
#include "Timer.h"
#include "Resources.h"
#include <random>
#include <cmath>
#include <algorithm>

CorpsePart::CorpsePart(int _x, int _y, int part_type, float vel_x, float vel_y, float explosion_force, ClanBomberApplication* app) 
    : GameObject(_x, _y, app) {
    
    // Initialize position and velocity
    position = Vector2D(_x, _y);
    velocity = Vector2D(vel_x, vel_y);
    acceleration = Vector2D(0.0f, 0.0f);
    
    // Physical properties based on body part type
    mass = get_part_mass(part_type);
    surface_area = get_part_surface_area(part_type);
    drag_coefficient = 0.47f; // Sphere approximation
    restitution = 0.3f + (part_type * 0.1f); // Different bounciness per part
    
    // Advanced physics constants
    air_density = 1.225f; // kg/m³ (at sea level)
    moment_of_inertia = mass * surface_area * 0.4f; // Approximation for irregular shapes
    angular_drag = 0.1f;
    
    // Visceral properties
    blood_emission_timer = 0.0f;
    blood_emission_rate = 20.0f; // Drops per second
    viscosity_factor = 0.8f + (part_type * 0.05f); // Some parts are more viscous
    
    lifetime = 0.0f;
    max_lifetime = 8.0f + (part_type * 0.5f); // Different parts last different times
    is_resting = false;
    rest_timer = 0.0f;
    
    texture_name = "corpse_parts";
    part_sprite = part_type % 4;
    sprite_nr = part_sprite;
    z = Z_CORPSE_PART;
    
    // Random rotation with more realistic physics
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> rot_dist(0.0f, 360.0f);
    std::uniform_real_distribution<> angular_dist(-720.0f, 720.0f); // Up to 2 full rotations/sec
    
    rotation = rot_dist(gen);
    angular_velocity = angular_dist(gen) * (explosion_force / mass); // More force = more spin
    
    // Apply initial explosion force
    Vector2D explosion_vector = velocity.normalized() * explosion_force;
    apply_force(explosion_vector);
}

CorpsePart::~CorpsePart() {
}

void CorpsePart::act(float deltaTime) {
    lifetime += deltaTime;
    
    if (!is_resting) {
        // Reset acceleration each frame
        acceleration = Vector2D(0.0f, 0.0f);
        
        // Apply Newtonian forces
        apply_gravity();
        apply_drag();
        
        // Integrate velocity and position using Verlet integration for stability
        Vector2D old_position = position;
        position = position + velocity * deltaTime + acceleration * (0.5f * deltaTime * deltaTime);
        velocity = velocity + acceleration * deltaTime;
        
        // Apply viscosity (resistance to motion in "blood")
        velocity = velocity * (1.0f - viscosity_factor * deltaTime);
        
        // Angular motion with realistic physics
        float angular_acceleration = 0.0f;
        if (moment_of_inertia > 0.0f) {
            // Torque from air resistance
            float angular_drag_force = -angular_velocity * angular_velocity * angular_drag;
            angular_acceleration = angular_drag_force / moment_of_inertia;
        }
        
        angular_velocity += angular_acceleration * deltaTime;
        rotation += angular_velocity * deltaTime;
        
        // Normalize rotation
        while (rotation > 360.0f) rotation -= 360.0f;
        while (rotation < 0.0f) rotation += 360.0f;
        
        // Handle collisions with advanced physics
        handle_collisions(deltaTime);
        
        // Update GameObject coordinates
        x = position.x;
        y = position.y;
        
        // Check if part has come to rest
        if (velocity.magnitude() < 5.0f && std::abs(angular_velocity) < 10.0f) {
            rest_timer += deltaTime;
            if (rest_timer > 1.0f) {
                is_resting = true;
                velocity = Vector2D(0.0f, 0.0f);
                angular_velocity = 0.0f;
            }
        } else {
            rest_timer = 0.0f;
        }
    }
    
    // Visceral effects
    update_blood_trail(deltaTime);
    
    // Emit blood while moving fast
    if (velocity.magnitude() > 50.0f && lifetime < 2.0f) {
        blood_emission_timer += deltaTime;
        if (blood_emission_timer > (1.0f / blood_emission_rate)) {
            emit_blood();
            blood_emission_timer = 0.0f;
        }
    }
    
    // Disposal after lifetime
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
    
    // Render visceral blood trails
    render_blood_trails();
}

void CorpsePart::apply_force(const Vector2D& force) {
    if (mass > 0.0f) {
        acceleration = acceleration + force * (1.0f / mass);
    }
}

void CorpsePart::apply_drag() {
    if (velocity.magnitude() > 0.0f) {
        // Realistic drag force: F = 0.5 * ρ * v² * Cd * A
        float speed = velocity.magnitude();
        float drag_force = 0.5f * air_density * speed * speed * drag_coefficient * surface_area;
        
        // Scale down for game physics (convert from real world to pixel world)
        drag_force *= 0.01f;
        
        // Apply drag opposite to velocity direction
        Vector2D drag_vector = velocity.normalized() * (-drag_force);
        apply_force(drag_vector);
    }
}

void CorpsePart::apply_gravity() {
    // F = mg, where g = 9.81 m/s² scaled to pixel world
    Vector2D gravity_force(0.0f, mass * 980.0f); // 980 pixels/s² ≈ 9.8 m/s²
    apply_force(gravity_force);
}

void CorpsePart::handle_collisions(float deltaTime) {
    const float ground_y = 560.0f;
    const float left_wall = 0.0f;
    const float right_wall = 760.0f;
    
    // Ground collision with advanced physics
    if (position.y > ground_y) {
        position.y = ground_y;
        
        // Calculate collision response
        float normal_velocity = velocity.y;
        float tangent_velocity = velocity.x;
        
        // Apply restitution (energy loss)
        velocity.y = -normal_velocity * restitution;
        
        // Apply friction
        float friction_coefficient = 0.7f;
        velocity.x = tangent_velocity * (1.0f - friction_coefficient);
        
        // Transfer some linear motion to angular motion
        float impact_force = std::abs(normal_velocity);
        angular_velocity += (tangent_velocity / mass) * 50.0f; // Conversion factor
        
        // Sound effect for hard impacts
        if (impact_force > 100.0f) {
            // TODO: Play thud sound
        }
    }
    
    // Wall collisions
    if (position.x < left_wall) {
        position.x = left_wall;
        velocity.x = -velocity.x * restitution;
        angular_velocity += velocity.y * 0.1f;
    } else if (position.x > right_wall) {
        position.x = right_wall;
        velocity.x = -velocity.x * restitution;
        angular_velocity -= velocity.y * 0.1f;
    }
}

void CorpsePart::update_blood_trail(float deltaTime) {
    // Update existing blood drops
    for (auto it = blood_trails.begin(); it != blood_trails.end();) {
        it->life -= deltaTime;
        it->alpha = (Uint8)(255.0f * (it->life / 2.0f)); // 2 second lifetime
        
        // Simple gravity for blood drops
        it->position.y += 50.0f * deltaTime;
        
        if (it->life <= 0.0f) {
            it = blood_trails.erase(it);
        } else {
            ++it;
        }
    }
}

void CorpsePart::emit_blood() {
    if (blood_trails.size() < 50) { // Limit blood drops for performance
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> offset_dist(-5.0f, 5.0f);
        std::uniform_real_distribution<> size_dist(1.0f, 3.0f);
        
        BloodTrail drop;
        drop.position = Vector2D(position.x + offset_dist(gen), position.y + offset_dist(gen));
        drop.life = 2.0f;
        drop.size = size_dist(gen);
        drop.alpha = 255;
        
        blood_trails.push_back(drop);
    }
}

void CorpsePart::render_blood_trails() {
    SDL_Renderer* renderer = Resources::get_renderer();
    if (!renderer) return;
    
    for (const auto& drop : blood_trails) {
        // Render blood drops as red circles
        SDL_SetRenderDrawColor(renderer, 139, 0, 0, drop.alpha); // Dark red
        
        // Draw a small filled circle (approximated with rect for performance)
        SDL_FRect blood_rect;
        blood_rect.x = drop.position.x - drop.size/2;
        blood_rect.y = drop.position.y - drop.size/2;
        blood_rect.w = drop.size;
        blood_rect.h = drop.size;
        
        SDL_RenderFillRect(renderer, &blood_rect);
    }
}

float CorpsePart::get_part_mass(int part_type) {
    // Different body parts have different masses (in arbitrary units)
    switch (part_type % 4) {
        case 0: return 2.5f; // Head - denser
        case 1: return 4.0f; // Torso - heaviest
        case 2: return 1.8f; // Arm - lighter
        case 3: return 2.2f; // Leg - medium
        default: return 2.0f;
    }
}

float CorpsePart::get_part_surface_area(int part_type) {
    // Surface area affects drag (in arbitrary units)
    switch (part_type % 4) {
        case 0: return 1.2f; // Head - compact
        case 1: return 2.0f; // Torso - largest area
        case 2: return 0.8f; // Arm - thin
        case 3: return 1.0f; // Leg - medium
        default: return 1.0f;
    }
}