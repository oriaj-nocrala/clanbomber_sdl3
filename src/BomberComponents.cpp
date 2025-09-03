#include "BomberComponents.h"
#include "GameObject.h"
#include "ClanBomber.h"
#include "Controller.h"
#include "Map.h"
#include "TileManager.h"
#include "Bomb.h"
#include "ThrownBomb.h"
#include "BomberCorpse.h"
#include "AudioMixer.h"
#include "CoordinateSystem.h"
#include "GameConfig.h"
#include "GameContext.h"
#include "Bomber.h"  // Need for cast in can_move()

// Import CoordinateConfig constants for refactoring Phase 1
static constexpr int TILE_SIZE = CoordinateConfig::TILE_SIZE;

// ===== MOVEMENT COMPONENT IMPLEMENTATION =====

BomberMovementComponent::BomberMovementComponent(GameObject* owner, GameContext* context)
    : owner(owner), context(context) {
}

void BomberMovementComponent::update(float deltaTime) {
    if (flying) {
        update_flight_animation(deltaTime);
    }
}

void BomberMovementComponent::handle_controller_input(Controller* controller, float deltaTime) {
    if (!controller || !can_move()) return;
    
    // FIXED: Use GameObject's existing movement system instead of duplicating logic
    // Set GameObject's speed from component speed
    owner->set_speed(speed);
    
    // Calculate movement distance for this frame
    float move_distance = speed * deltaTime;
    bool is_moving = false;
    
    // Set direction and move based on controller input
    if (controller->is_left()) {
        last_direction = DIR_LEFT;
        owner->set_dir(DIR_LEFT);
        owner->move_dist(move_distance, DIR_LEFT);
    } else if (controller->is_right()) {
        last_direction = DIR_RIGHT;
        owner->set_dir(DIR_RIGHT);
        owner->move_dist(move_distance, DIR_RIGHT);
    } else if (controller->is_up()) {
        last_direction = DIR_UP;
        owner->set_dir(DIR_UP);
        owner->move_dist(move_distance, DIR_UP);
    } else if (controller->is_down()) {
        last_direction = DIR_DOWN;
        owner->set_dir(DIR_DOWN);
        owner->move_dist(move_distance, DIR_DOWN);
    }
    
    // SIMPLIFIED FIX: Just animate if ANY input is detected, ignore move_dist result
    Bomber* bomber = static_cast<Bomber*>(owner);
    if (bomber && bomber->animation_component) {
        bool has_input = controller->is_left() || controller->is_right() || 
                        controller->is_up() || controller->is_down();
        
        if (has_input) {
            // Force animation for ANY direction input
            bomber->animation_component->update_animation_frame(deltaTime, last_direction);
        } else {
            // Set standing sprite with last movement direction
            bomber->animation_component->set_standing_sprite(last_direction);
        }
    }
}

void BomberMovementComponent::fly_to(int target_x, int target_y, float duration_ms) {
    flying = true;
    flight_timer = 0.0f;
    flight_duration = duration_ms / 1000.0f; // Convert to seconds
    start_x = owner->get_x();
    start_y = owner->get_y();
    this->target_x = target_x;
    this->target_y = target_y;
    
    SDL_Log("BomberMovementComponent: Starting flight from (%d,%d) to (%d,%d) over %.2fs", 
            start_x, start_y, target_x, target_y, flight_duration);
}

bool BomberMovementComponent::can_move() const {
    // Can't move if bomber is flying or dead
    if (flying) return false;
    
    // Check if bomber is dead - cast owner to Bomber to access death state
    Bomber* bomber = static_cast<Bomber*>(owner);
    if (bomber && bomber->is_dead()) {
        return false;
    }
    
    return true;
}

void BomberMovementComponent::update_flight_animation(float deltaTime) {
    flight_timer += deltaTime;
    float progress = flight_timer / flight_duration;
    
    if (progress >= 1.0f) {
        // Animation complete - CRITICAL: Reset all flight state
        flying = false;
        flight_timer = 0.0f;
        owner->set_pos(target_x, target_y);
        
        // Reset any movement blocking flags
        owner->stop(); // Ensure GameObject is in proper stopped state
        
        SDL_Log("BomberMovementComponent: Flight animation complete at (%d,%d) - controls restored", target_x, target_y);
    } else {
        // Interpolate position
        int current_x = start_x + (int)((target_x - start_x) * progress);
        int current_y = start_y + (int)((target_y - start_y) * progress);
        owner->set_pos(current_x, current_y);
    }
}

// REMOVED: check_collision() - now using GameObject::move_dist() which has built-in collision detection

// ===== COMBAT COMPONENT IMPLEMENTATION =====

BomberCombatComponent::BomberCombatComponent(GameObject* owner, GameContext* context)
    : owner(owner), context(context) {
    power = GameConfig::get_start_power();
}

void BomberCombatComponent::update(float deltaTime) {
    update_bomb_cooldown(deltaTime);
    update_bomb_throwing(deltaTime);
    update_bomb_escape_status(); // New position-based system
}

void BomberCombatComponent::handle_controller_input(Controller* controller, float deltaTime) {
    if (!controller || dead) return;
    
    // CRITICAL: Don't allow combat actions during flight
    Bomber* bomber = static_cast<Bomber*>(owner);
    if (bomber && !bomber->can_move()) {
        return; // Block all combat actions during flight
    }
    
    // Handle bomb placement/throwing
    if (controller->is_bomb()) {
        if (!bomb_button_held) {
            bomb_button_held = true;
            bomb_hold_timer = 0.0f;
        } else {
            bomb_hold_timer += deltaTime;
        }
    } else {
        if (bomb_button_held) {
            // Button released - place or throw bomb
            if (bomb_hold_timer >= THROW_HOLD_TIME && can_throw) {
                throw_bomb();
            } else {
                place_bomb();
            }
            bomb_button_held = false;
            bomb_hold_timer = 0.0f;
        }
    }
}

void BomberCombatComponent::place_bomb() {
    if (!can_place_bomb() || bomb_cooldown > 0.0f) return;
    
    int bomber_x = owner->get_x();
    int bomber_y = owner->get_y();
    int map_x = owner->get_map_x();
    int map_y = owner->get_map_y();
    
    // DEBUG: Use CoordinateSystem for tile calculation
    GridCoord expected_grid = CoordinateSystem::pixel_to_grid(PixelCoord(bomber_x, bomber_y));
    int expected_map_x = expected_grid.grid_x;
    int expected_map_y = expected_grid.grid_y;
    
    SDL_Log("🔍 DEBUG: Bomber at (%d,%d) -> get_map_x()=%d, get_map_y()=%d", bomber_x, bomber_y, map_x, map_y);
    SDL_Log("🔍 DEBUG: Expected tile calculation: (%d+%d)/%d=%d, (%d+%d)/%d=%d", bomber_x, TILE_SIZE/2, TILE_SIZE, expected_map_x, bomber_y, TILE_SIZE/2, TILE_SIZE, expected_map_y);
    
    // Check if there's already a bomb at this position
    if (context->has_bomb_at(map_x, map_y)) {
        return;
    }
    
    // SNAP TO GRID: Place bomb at center of tile using unified CoordinateSystem
    GridCoord grid(map_x, map_y);
    PixelCoord center = CoordinateSystem::grid_to_pixel(grid);
    int bomb_x = static_cast<int>(center.pixel_x);
    int bomb_y = static_cast<int>(center.pixel_y);
    
    Bomb* bomb = new Bomb(bomb_x, bomb_y, power, static_cast<Bomber*>(owner), context);
    
    SDL_Log("💣 PLACE BOMB: Bomber at (%d,%d) -> tile (%d,%d) -> Bomb created at center (%d,%d)", 
            bomber_x, bomber_y, map_x, map_y, bomb_x, bomb_y);
    context->register_object(bomb); // MODERN: Use GameContext for object registration
    
    // Register bomb with tile manager
    if (context->get_tile_manager()) {
        context->get_tile_manager()->register_bomb_at(map_x, map_y, bomb);
    }
    
    // Update bomb tracking
    inc_current_bombs();
    bomb_cooldown = 0.2f; // 200ms cooldown
    
    // BOMB ESCAPE SYSTEM: Track that bomber is standing on this bomb
    bomb_standing_on = bomb;
    has_left_bomb_tile = false;
    
    SDL_Log("🎯 BOMB ESCAPE: Bomber can move freely while on bomb at tile (%d,%d)", 
            map_x, map_y);
    SDL_Log("BomberCombatComponent: Placed bomb at (%d,%d) with power %d", map_x, map_y, power);
}

void BomberCombatComponent::throw_bomb() {
    if (!can_place_bomb() || bomb_cooldown > 0.0f) return;
    
    // Calculate throw target based on direction
    int throw_distance = 80; // 2 tiles
    int target_x = owner->get_x();
    int target_y = owner->get_y();
    
    switch (owner->get_cur_dir()) {
        case DIR_LEFT:  target_x -= throw_distance; break;
        case DIR_RIGHT: target_x += throw_distance; break;
        case DIR_UP:    target_y -= throw_distance; break;
        case DIR_DOWN:  target_y += throw_distance; break;
    }
    
    // Create thrown bomb
    // Create thrown bomb using GameContext - MODERN architecture
    ThrownBomb* thrown_bomb = new ThrownBomb(
        owner->get_x(), owner->get_y(), power, 
        static_cast<Bomber*>(owner), 
        target_x, target_y, context
    );
    context->register_object(thrown_bomb); // MODERN: Use GameContext for object registration
    
    // Update bomb tracking
    inc_current_bombs();
    bomb_cooldown = 0.2f; // 200ms cooldown
    
    SDL_Log("BomberCombatComponent: Threw bomb from (%d,%d) with power %d", 
            owner->get_x(), owner->get_y(), power);
}

bool BomberCombatComponent::can_place_bomb() const {
    return !dead && current_bombs < max_bombs;
}

void BomberCombatComponent::die() {
    if (dead) return;
    
    dead = true;
    
    // Create corpse with bomber's color using GameContext - MODERN architecture  
    Bomber* bomber = static_cast<Bomber*>(owner);
    BomberCorpse* corpse = new BomberCorpse(owner->get_x(), owner->get_y(), bomber->get_color(), context);
    context->register_object(corpse); // MODERN: Use GameContext for object registration
    
    // Play death sound
    AudioPosition death_pos(owner->get_x(), owner->get_y(), 0.0f);
    AudioMixer::play_sound_3d("die", death_pos, 600.0f);
    
    SDL_Log("BomberCombatComponent: Bomber died at (%d,%d)", owner->get_x(), owner->get_y());
}

void BomberCombatComponent::update_bomb_cooldown(float deltaTime) {
    if (bomb_cooldown > 0.0f) {
        bomb_cooldown -= deltaTime;
        if (bomb_cooldown < 0.0f) {
            bomb_cooldown = 0.0f;
        }
    }
}

void BomberCombatComponent::update_bomb_throwing(float deltaTime) {
    if (bomb_button_held) {
        bomb_hold_timer += deltaTime;
    }
}


bool BomberCombatComponent::can_ignore_bomb_collision(Bomb* bomb) const {
    // Only ignore collision if bomber is standing on this specific bomb and hasn't left its tile
    bool result = (bomb_standing_on == bomb && !has_left_bomb_tile);
    SDL_Log("🔍 BOMB ESCAPE CHECK: standing_on=%p, checking=%p, has_left=%d, result=%d", 
            bomb_standing_on, bomb, has_left_bomb_tile, result);
    return result;
}

void BomberCombatComponent::update_bomb_escape_status() {
    if (!bomb_standing_on) return; // No bomb to track
    
    // Check if bomber is still in the same tile as the bomb using CoordinateSystem
    GridCoord bomber_grid = CoordinateSystem::pixel_to_grid(PixelCoord(owner->get_x(), owner->get_y()));
    GridCoord bomb_grid = CoordinateSystem::pixel_to_grid(PixelCoord(bomb_standing_on->get_x(), bomb_standing_on->get_y()));
    int bomber_tile_x = bomber_grid.grid_x;
    int bomber_tile_y = bomber_grid.grid_y;
    int bomb_tile_x = bomb_grid.grid_x;
    int bomb_tile_y = bomb_grid.grid_y;
    
    if (bomber_tile_x != bomb_tile_x || bomber_tile_y != bomb_tile_y) {
        // Bomber has left the bomb tile - enable collisions
        if (!has_left_bomb_tile) {
            has_left_bomb_tile = true;
            SDL_Log("🏃 BOMB ESCAPE: Bomber left bomb tile - collision enabled");
        }
    }
}

// ===== ANIMATION COMPONENT IMPLEMENTATION =====

BomberAnimationComponent::BomberAnimationComponent(GameObject* owner, GameContext* context)
    : owner(owner), context(context) {
    // Initialize sprite animation with standing sprite (down direction, frame 0)
    owner->set_sprite_nr(0); // DIR_DOWN * 10 + 0 = standing facing down
    
    // Initialize animation counter to 0 (standing state)
    anim_count = 0.0f;
    
    // Initialize position tracking
    last_x = owner->get_x();
    last_y = owner->get_y();
}

void BomberAnimationComponent::update(float deltaTime) {
    // Only update invincibility effects here
    // Animation is handled separately by movement component
    if (invincible) {
        update_invincibility_effects(deltaTime);
    }
}

void BomberAnimationComponent::set_texture_from_color(int color) {
    choose_texture_for_color(color);
}

void BomberAnimationComponent::update_animation_frame(float deltaTime, Direction direction) {
    int base_sprite = 0;
    
    
    // Calculate base sprite based on direction (standard bomber sprite layout)
    switch (direction) {
        case DIR_DOWN:  base_sprite = 0; break;  // Down-facing sprites: 0-1
        case DIR_LEFT:  base_sprite = 10; break;  // Left-facing sprites: 2-3  
        case DIR_RIGHT: base_sprite = 30; break;  // Right-facing sprites: 4-5
        case DIR_UP:    base_sprite = 20; break;  // Up-facing sprites: 6-7
    }
    
    // ORIGINAL SYSTEM: Use 9 sprites for walking animation (sprites 1-9)
    // When starting to move, ensure we start at frame 1, not 0
    if (anim_count == 0.0f) {
        anim_count = 1.0f;  // Start walking animation at frame 1
    }
    
    // CALIBRATED FOR DELTATIME: Reduced animation speed factor for smooth deltaTime-based animation
    // Original was 20.0f for frame-based system, now calibrated for real deltaTime
    anim_count += deltaTime * 8.0f; // Reduced from 20.0f for deltaTime calibration
    if (anim_count >= 9.0f) {
        anim_count = 1.0f;  // Reset to 1, not 0 (0 is for standing)
    }
    
    // Use walking animation frames (1-9, not 0-1)
    int anim_frame = (int)anim_count;
    int final_sprite = base_sprite + anim_frame;
    
    
    // Update GameObject's sprite number
    owner->set_sprite_nr(final_sprite);
}

void BomberAnimationComponent::set_standing_sprite(Direction direction) {
    int base_sprite = 0;
    
    // Calculate base sprite based on direction (original system mapping)
    switch (direction) {
        case DIR_DOWN:  base_sprite = 0; break;   // Down-facing sprites: 0-9
        case DIR_LEFT:  base_sprite = 10; break;  // Left-facing sprites: 10-19  
        case DIR_RIGHT: base_sprite = 30; break;  // Right-facing sprites: 30-39
        case DIR_UP:    base_sprite = 20; break;  // Up-facing sprites: 20-29
    }
    
    // Set standing sprite (frame 0 for each direction)
    int standing_sprite = base_sprite + 0;  // cur_dir*10 + 0
    owner->set_sprite_nr(standing_sprite);
    anim_count = 0.0f; // Reset animation counter
}

void BomberAnimationComponent::update_invincibility_effects(float deltaTime) {
    invincible_timer -= deltaTime;
    if (invincible_timer <= 0.0f) {
        invincible = false;
        invincible_timer = 0.0f;
    }
}

void BomberAnimationComponent::choose_texture_for_color(int color) {
    switch (color) {
        case 0: owner->set_texture_name("bomber_dull_red"); break;
        case 1: owner->set_texture_name("bomber_dull_blue"); break;
        case 2: owner->set_texture_name("bomber_dull_yellow"); break;
        case 3: owner->set_texture_name("bomber_dull_green"); break;
        case 4: owner->set_texture_name("bomber_snake"); break;
        case 5: owner->set_texture_name("bomber_tux"); break;
        case 6: owner->set_texture_name("bomber_spider"); break;
        case 7: owner->set_texture_name("bomber_bsd"); break;
        default: owner->set_texture_name("bomber_snake"); break;
    }
}

// ===== LIFECYCLE COMPONENT IMPLEMENTATION =====

BomberLifecycleComponent::BomberLifecycleComponent(GameObject* owner, GameContext* context)
    : owner(owner), context(context) {
}

void BomberLifecycleComponent::update(float deltaTime) {
    if (respawning) {
        update_respawn_timer(deltaTime);
    }
}

void BomberLifecycleComponent::respawn() {
    if (!has_lives()) return;
    
    respawning = true;
    respawn_timer = 3.0f; // 3 seconds respawn delay
    
    SDL_Log("BomberLifecycleComponent: Starting respawn for %s (%d lives remaining)", 
            bomber_name.c_str(), remaining_lives);
}

void BomberLifecycleComponent::update_respawn_timer(float deltaTime) {
    respawn_timer -= deltaTime;
    if (respawn_timer <= 0.0f) {
        respawning = false;
        respawn_timer = 0.0f;
        
        SDL_Log("BomberLifecycleComponent: Respawn complete for %s", bomber_name.c_str());
    }
}