#include "Controller_AI_Smart.h"
#include "ClanBomber.h"
#include "Map.h"
#include "Bomber.h"
#include "Timer.h"
#include "Bomb.h"
#include "MapTile.h"
#include "Extra.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <cstdlib>

// Helper functions for CL_Vector operations that are missing
namespace {
    float vector_distance(const CL_Vector& a, const CL_Vector& b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return sqrt(dx*dx + dy*dy);
    }
    
    float vector_length(const CL_Vector& v) {
        return sqrt(v.x*v.x + v.y*v.y);
    }
    
    CL_Vector vector_subtract(const CL_Vector& a, const CL_Vector& b) {
        return CL_Vector(a.x - b.x, a.y - b.y);
    }
    
    CL_Vector vector_add(const CL_Vector& a, const CL_Vector& b) {
        return CL_Vector(a.x + b.x, a.y + b.y);
    }
    
    CL_Vector vector_multiply(const CL_Vector& v, float scalar) {
        return CL_Vector(v.x * scalar, v.y * scalar);
    }
    
    CL_Vector vector_normalize(const CL_Vector& v) {
        float len = vector_length(v);
        if (len > 0.001f) {
            return CL_Vector(v.x / len, v.y / len);
        }
        return CL_Vector(0, 0);
    }
    
    static float total_time_accumulator = 0.0f;
    
    float get_total_time() {
        total_time_accumulator += Timer::time_elapsed();
        return total_time_accumulator;
    }
}

Controller_AI_Smart::Controller_AI_Smart(AIPersonality personality) 
    : personality(personality)
    , reaction_time(0.1f)
    , aggression_level(0.5f)
    , thinking_frequency(0.1f)
    , current_state(AIState::EXPLORING)
    , current_state_name("EXPLORING")
    , current_target(0, 0)
    , last_think_time(0.0f)
    , next_input_time(0.0f)
    , bomb_cooldown_ai(0.0f)
    , last_bomb_time(0.0f)
    , stuck_timer(0.0f)
    , last_position(0, 0)
    , memory_fade_time(5.0f)
    , ai_update_interval(0.05f) // 20 FPS AI thinking
    , last_ai_update(0.0f)
{
    set_personality(personality);
    reset();
}

Controller_AI_Smart::~Controller_AI_Smart() {}

void Controller_AI_Smart::reset() {
    current_input = AIInput{};
    current_state = AIState::EXPLORING;
    current_target = CL_Vector(0, 0);
    last_think_time = 0.0f;
    next_input_time = 0.0f;
    bomb_cooldown_ai = 0.0f;
    stuck_timer = 0.0f;
    dangerous_positions.clear();
    recently_bombed_positions.clear();
}

void Controller_AI_Smart::set_personality(AIPersonality new_personality) {
    personality = new_personality;
    
    switch (personality) {
        case AIPersonality::PEACEFUL:
            aggression_level = 0.1f;
            reaction_time = 0.8f;
            thinking_frequency = 0.2f;
            break;
            
        case AIPersonality::EASY:
            aggression_level = 0.3f;
            reaction_time = 0.5f;
            thinking_frequency = 0.15f;
            break;
            
        case AIPersonality::NORMAL:
            aggression_level = 0.5f;
            reaction_time = 0.2f;
            thinking_frequency = 0.1f;
            break;
            
        case AIPersonality::HARD:
            aggression_level = 0.8f;
            reaction_time = 0.1f;
            thinking_frequency = 0.05f;
            break;
            
        case AIPersonality::NIGHTMARE:
            aggression_level = 1.0f;
            reaction_time = 0.03f; // Almost instant reactions
            thinking_frequency = 0.03f; // Thinks very fast
            break;
    }
}

void Controller_AI_Smart::update() {
    if (!active || !bomber) return;
    
    float current_time = get_total_time();
    
    // Performance optimization - don't think every frame
    if (current_time - last_ai_update >= ai_update_interval) {
        think();
        last_ai_update = current_time;
    }
    
    // Apply reaction delay for realism
    if (current_time >= next_input_time) {
        execute_behavior();
    }
    
    // Update timers
    if (bomb_cooldown_ai > 0) {
        bomb_cooldown_ai -= Timer::time_elapsed();
    }
    
    // Check if stuck
    CL_Vector current_pos(bomber->get_x(), bomber->get_y());
    if (vector_distance(current_pos, last_position) < 5.0f) {
        stuck_timer += Timer::time_elapsed();
    } else {
        stuck_timer = 0.0f;
        last_position = current_pos;
    }
}

void Controller_AI_Smart::think() {
    if (!bomber || !bomber->app) return;
    
    analyze_enemies();
    
    // Fade old memories
    float fade_factor = Timer::time_elapsed() / memory_fade_time;
    dangerous_positions.erase(
        std::remove_if(dangerous_positions.begin(), dangerous_positions.end(),
            [fade_factor](const CL_Vector&) { return (rand() % 100) < (fade_factor * 100); }),
        dangerous_positions.end()
    );
    
    // Update AI state based on current situation
    update_current_state();
    
    last_think_time = get_total_time();
}

void Controller_AI_Smart::update_current_state() {
    if (!bomber || !bomber->app) return;
    
    CL_Vector my_pos(bomber->get_x(), bomber->get_y());
    float danger_level = calculate_danger_level(my_pos);
    
    // Emergency: immediate danger detection
    if (danger_level > 0.8f) {
        transition_to_state(AIState::FLEEING);
        return;
    }
    
    // Scan for targets
    auto targets = scan_for_targets();
    
    if (targets.empty()) {
        transition_to_state(AIState::EXPLORING);
        return;
    }
    
    auto best_target = select_best_target(targets);
    current_target = best_target.position;
    
    // Decide state based on target type and personality
    if (best_target.is_enemy && should_hunt_enemies()) {
        transition_to_state(AIState::HUNTING);
    } else if (best_target.is_powerup) {
        transition_to_state(AIState::COLLECTING);
    } else if (should_place_bomb()) {
        transition_to_state(AIState::BOMBING);
    } else {
        transition_to_state(AIState::EXPLORING);
    }
}

void Controller_AI_Smart::transition_to_state(AIState new_state) {
    if (current_state == new_state) return;
    
    current_state = new_state;
    
    switch (current_state) {
        case AIState::EXPLORING:
            current_state_name = "EXPLORING";
            break;
        case AIState::HUNTING:
            current_state_name = "HUNTING";
            break;
        case AIState::FLEEING:
            current_state_name = "FLEEING";
            current_target = find_safe_position();
            break;
        case AIState::COLLECTING:
            current_state_name = "COLLECTING";
            break;
        case AIState::BOMBING:
            current_state_name = "BOMBING";
            break;
        case AIState::WAITING:
            current_state_name = "WAITING";
            break;
    }
}

void Controller_AI_Smart::execute_behavior() {
    if (!bomber) return;
    
    // Reset inputs
    current_input = AIInput{};
    
    CL_Vector my_pos(bomber->get_x(), bomber->get_y());
    
    switch (current_state) {
        case AIState::FLEEING: {
            // Find and move to the safest position
            CL_Vector safe_pos = find_safe_position();
            auto path = find_path_to(safe_pos);
            if (!path.empty() && path.size() > 1) {
                CL_Vector next_step = path[1]; // path[0] is current position
                
                if (next_step.x > my_pos.x + 20) current_input.right = true;
                else if (next_step.x < my_pos.x - 20) current_input.left = true;
                if (next_step.y > my_pos.y + 20) current_input.down = true;
                else if (next_step.y < my_pos.y - 20) current_input.up = true;
            }
            break;
        }
        
        case AIState::HUNTING:
        case AIState::COLLECTING: {
            // Move towards target
            auto path = find_path_to(current_target);
            if (!path.empty() && path.size() > 1) {
                CL_Vector next_step = path[1];
                
                if (next_step.x > my_pos.x + 20) current_input.right = true;
                else if (next_step.x < my_pos.x - 20) current_input.left = true;
                if (next_step.y > my_pos.y + 20) current_input.down = true;
                else if (next_step.y < my_pos.y - 20) current_input.up = true;
            }
            
            // Place bomb if hunting and close to target
            if (current_state == AIState::HUNTING && should_place_bomb()) {
                if (vector_distance(my_pos, current_target) < 100.0f && bomb_cooldown_ai <= 0) {
                    current_input.bomb = true;
                    bomb_cooldown_ai = 1.0f + (1.0f - aggression_level);
                    last_bomb_time = get_total_time();
                }
            }
            break;
        }
        
        case AIState::BOMBING: {
            // Strategic bomb placement
            if (bomb_cooldown_ai <= 0 && can_escape_from_bomb(my_pos)) {
                current_input.bomb = true;
                bomb_cooldown_ai = 2.0f * get_bomb_frequency_modifier();
                recently_bombed_positions.push_back(my_pos);
            } else {
                // Move to better bombing position
                transition_to_state(AIState::EXPLORING);
            }
            break;
        }
        
        case AIState::EXPLORING: {
            // Random exploration with slight bias towards unexplored areas
            static float explore_timer = 0.0f;
            explore_timer += Timer::time_elapsed();
            
            if (explore_timer > 1.0f || stuck_timer > 2.0f) {
                // Choose random direction
                int direction = rand() % 4;
                switch (direction) {
                    case 0: current_input.up = true; break;
                    case 1: current_input.down = true; break;
                    case 2: current_input.left = true; break;
                    case 3: current_input.right = true; break;
                }
                explore_timer = 0.0f;
                stuck_timer = 0.0f;
            }
            break;
        }
        
        case AIState::WAITING:
            // Do nothing, just wait
            break;
    }
    
    // Apply reaction delay for next input
    next_input_time = get_total_time() + get_reaction_delay();
}

std::vector<CL_Vector> Controller_AI_Smart::find_path_to(CL_Vector target) {
    // Simplified pathfinding - in a real implementation, use A*
    std::vector<CL_Vector> path;
    
    if (!bomber || !bomber->app || !bomber->app->map) {
        return path;
    }
    
    CL_Vector current(bomber->get_x(), bomber->get_y());
    path.push_back(current);
    
    // Simple direct path with basic obstacle avoidance
    CL_Vector direction = vector_subtract(target, current);
    float distance = vector_length(direction);
    
    if (distance < 40.0f) {
        path.push_back(target);
        return path;
    }
    
    direction = vector_normalize(direction);
    
    // Take steps towards target
    for (float step = 40.0f; step < distance; step += 40.0f) {
        CL_Vector next_pos = vector_add(current, vector_multiply(direction, step));
        
        // Check if position is walkable
        int grid_x = (int)(next_pos.x / 40);
        int grid_y = (int)(next_pos.y / 40);
        
        MapTile* tile = bomber->app->map->get_tile(grid_x, grid_y);
        if (tile && !tile->is_blocking()) {
            path.push_back(next_pos);
        } else {
            // Simple obstacle avoidance - try perpendicular directions
            CL_Vector perp1(-direction.y, direction.x);
            CL_Vector perp2(direction.y, -direction.x);
            
            CL_Vector alt1 = vector_add(current, vector_multiply(perp1, 40.0f));
            CL_Vector alt2 = vector_add(current, vector_multiply(perp2, 40.0f));
            
            int alt1_x = (int)(alt1.x / 40), alt1_y = (int)(alt1.y / 40);
            int alt2_x = (int)(alt2.x / 40), alt2_y = (int)(alt2.y / 40);
            
            MapTile* tile1 = bomber->app->map->get_tile(alt1_x, alt1_y);
            MapTile* tile2 = bomber->app->map->get_tile(alt2_x, alt2_y);
            
            if (tile1 && !tile1->is_blocking()) {
                path.push_back(alt1);
            } else if (tile2 && !tile2->is_blocking()) {
                path.push_back(alt2);
            }
            break;
        }
    }
    
    path.push_back(target);
    return path;
}

bool Controller_AI_Smart::is_position_safe(CL_Vector pos, float time_ahead) {
    // Check immediate danger level
    float danger = calculate_danger_level(pos);
    return danger < 0.3f; // Safe threshold
}

float Controller_AI_Smart::calculate_danger_level(CL_Vector pos) {
    if (!bomber || !bomber->app) return 0.0f;
    
    float danger = 0.0f;
    
    // Check proximity to known dangerous positions
    for (const auto& dangerous_pos : dangerous_positions) {
        float dist = vector_distance(pos, dangerous_pos);
        if (dist < 120.0f) { // 3 tiles radius
            danger += (120.0f - dist) / 120.0f;
        }
    }
    
    // Check proximity to bombs
    for (auto& obj : bomber->app->objects) {
        if (obj && obj->get_type() == GameObject::BOMB) {
            CL_Vector bomb_pos(obj->get_x(), obj->get_y());
            float dist = vector_distance(pos, bomb_pos);
            
            if (dist < 200.0f) { // 5 tiles explosion radius
                danger += (200.0f - dist) / 200.0f * 2.0f; // Bombs are very dangerous
            }
        }
    }
    
    // Check proximity to enemies (less dangerous but still a factor)
    for (auto& enemy : bomber->app->bomber_objects) {
        if (enemy && enemy != bomber && !enemy->is_dead()) {
            CL_Vector enemy_pos(enemy->get_x(), enemy->get_y());
            float dist = vector_distance(pos, enemy_pos);
            
            if (dist < 80.0f) { // Close proximity to enemies
                danger += (80.0f - dist) / 80.0f * 0.3f;
            }
        }
    }
    
    return std::min(danger, 1.0f); // Cap at 1.0
}

CL_Vector Controller_AI_Smart::find_safe_position() {
    if (!bomber || !bomber->app || !bomber->app->map) {
        return CL_Vector(bomber->get_x(), bomber->get_y());
    }
    
    CL_Vector my_pos(bomber->get_x(), bomber->get_y());
    CL_Vector safest_pos = my_pos;
    float lowest_danger = 1.0f;
    
    // Search in expanding radius for safe position
    for (int radius = 1; radius <= 8; radius++) {
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dy = -radius; dy <= radius; dy++) {
                if (abs(dx) != radius && abs(dy) != radius) continue; // Only check perimeter
                
                CL_Vector test_pos = vector_add(my_pos, CL_Vector(dx * 40, dy * 40));
                
                // Check if position is on map and walkable
                int grid_x = (int)(test_pos.x / 40);
                int grid_y = (int)(test_pos.y / 40);
                
                MapTile* tile = bomber->app->map->get_tile(grid_x, grid_y);
                if (!tile || tile->is_blocking()) continue;
                
                float danger = calculate_danger_level(test_pos);
                if (danger < lowest_danger) {
                    lowest_danger = danger;
                    safest_pos = test_pos;
                    
                    if (danger < 0.1f) return safest_pos; // Good enough
                }
            }
        }
    }
    
    return safest_pos;
}

std::vector<AITarget> Controller_AI_Smart::scan_for_targets() {
    std::vector<AITarget> targets;
    
    if (!bomber || !bomber->app) return targets;
    
    CL_Vector my_pos(bomber->get_x(), bomber->get_y());
    
    // Scan for power-ups
    for (auto& obj : bomber->app->objects) {
        if (obj && obj->get_type() == GameObject::EXTRA) {
            CL_Vector target_pos(obj->get_x(), obj->get_y());
            float distance = vector_distance(my_pos, target_pos);
            
            AITarget target;
            target.position = target_pos;
            target.distance = distance;
            target.is_powerup = true;
            target.is_enemy = false;
            target.priority = evaluate_powerup_value(0) * (1.0f / (distance / 40.0f + 1.0f));
            target.is_safe_path = is_position_safe(target_pos);
            
            targets.push_back(target);
        }
    }
    
    // Scan for enemies (if aggressive enough)
    if (should_hunt_enemies()) {
        for (auto& enemy : bomber->app->bomber_objects) {
            if (enemy && enemy != bomber && !enemy->is_dead()) {
                CL_Vector enemy_pos(enemy->get_x(), enemy->get_y());
                float distance = vector_distance(my_pos, enemy_pos);
                
                AITarget target;
                target.position = enemy_pos;
                target.distance = distance;
                target.is_powerup = false;
                target.is_enemy = true;
                target.priority = aggression_level * (1.0f / (distance / 40.0f + 1.0f));
                target.is_safe_path = is_position_safe(enemy_pos);
                
                targets.push_back(target);
            }
        }
    }
    
    return targets;
}

AITarget Controller_AI_Smart::select_best_target(const std::vector<AITarget>& targets) {
    AITarget best_target;
    best_target.priority = -1.0f;
    
    for (const auto& target : targets) {
        float adjusted_priority = target.priority;
        
        // Personality-based priority adjustments
        if (target.is_enemy) {
            adjusted_priority *= get_aggression_modifier();
        }
        
        if (!target.is_safe_path) {
            adjusted_priority *= 0.3f; // Heavily penalize unsafe targets
        }
        
        if (adjusted_priority > best_target.priority) {
            best_target = target;
        }
    }
    
    return best_target;
}

float Controller_AI_Smart::evaluate_powerup_value(int powerup_type) {
    // Base value for any powerup
    return 0.7f;
}

bool Controller_AI_Smart::should_place_bomb() {
    if (bomb_cooldown_ai > 0) return false;
    if (!bomber) return false;
    
    CL_Vector my_pos(bomber->get_x(), bomber->get_y());
    
    // Don't bomb if we can't escape
    if (!can_escape_from_bomb(my_pos)) return false;
    
    // Check if bomb would be effective
    if (current_state == AIState::HUNTING && would_hit_enemy(my_pos)) {
        return true;
    }
    
    // Strategic bombing based on personality
    if (get_aggression_modifier() > 0.6f) {
        // Check if we haven't bombed this area recently
        for (const auto& recent_bomb : recently_bombed_positions) {
            if (vector_distance(my_pos, recent_bomb) < 80.0f) {
                return false; // Too recent
            }
        }
        return (rand() % 100) < (aggression_level * 30); // Random strategic bombing
    }
    
    return false;
}

bool Controller_AI_Smart::can_escape_from_bomb(CL_Vector bomb_pos) {
    // Simple escape check - can we reach safety in time?
    CL_Vector safe_pos = find_safe_position();
    float escape_distance = vector_distance(bomb_pos, safe_pos);
    float escape_time = escape_distance / (bomber ? bomber->get_speed() : 90.0f);
    
    return escape_time < 2.5f; // Bomb explodes in ~3 seconds
}

bool Controller_AI_Smart::would_hit_enemy(CL_Vector bomb_pos) {
    if (!bomber) return false;
    
    auto explosion_tiles = predict_explosion_tiles(bomb_pos, bomber->get_power());
    
    for (auto& enemy : bomber->app->bomber_objects) {
        if (enemy && enemy != bomber && !enemy->is_dead()) {
            CL_Vector enemy_pos(enemy->get_x(), enemy->get_y());
            
            for (const auto& explosion_tile : explosion_tiles) {
                if (vector_distance(enemy_pos, explosion_tile) < 30.0f) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

std::vector<CL_Vector> Controller_AI_Smart::predict_explosion_tiles(CL_Vector bomb_pos, int power) {
    std::vector<CL_Vector> tiles;
    
    // Center tile
    tiles.push_back(bomb_pos);
    
    // Explosion rays in 4 directions
    for (int i = 1; i <= power; i++) {
        tiles.push_back(vector_add(bomb_pos, CL_Vector(i * 40, 0)));  // Right
        tiles.push_back(vector_add(bomb_pos, CL_Vector(-i * 40, 0))); // Left
        tiles.push_back(vector_add(bomb_pos, CL_Vector(0, i * 40)));  // Down
        tiles.push_back(vector_add(bomb_pos, CL_Vector(0, -i * 40))); // Up
    }
    
    return tiles;
}

void Controller_AI_Smart::analyze_enemies() {
    // Update dangerous positions based on enemy bomb placements
    if (!bomber || !bomber->app) return;
    
    for (auto& obj : bomber->app->objects) {
        if (obj && obj->get_type() == GameObject::BOMB) {
            CL_Vector bomb_pos(obj->get_x(), obj->get_y());
            
            // Add to dangerous positions if not already there
            bool already_known = false;
            for (const auto& dangerous_pos : dangerous_positions) {
                if (vector_distance(bomb_pos, dangerous_pos) < 40.0f) {
                    already_known = true;
                    break;
                }
            }
            
            if (!already_known) {
                dangerous_positions.push_back(bomb_pos);
            }
        }
    }
}

CL_Vector Controller_AI_Smart::predict_enemy_position(Bomber* enemy, float time_ahead) {
    if (!enemy) return CL_Vector(0, 0);
    
    // Simple linear prediction based on current movement
    // In a more sophisticated AI, this would use more complex prediction
    return CL_Vector(enemy->get_x(), enemy->get_y());
}

bool Controller_AI_Smart::is_enemy_dangerous(Bomber* enemy) {
    if (!enemy || enemy->is_dead()) return false;
    
    // Check if enemy has high power or is close
    CL_Vector my_pos(bomber->get_x(), bomber->get_y());
    CL_Vector enemy_pos(enemy->get_x(), enemy->get_y());
    
    return vector_distance(my_pos, enemy_pos) < 120.0f; // Close proximity = dangerous
}

float Controller_AI_Smart::get_aggression_modifier() {
    switch (personality) {
        case AIPersonality::PEACEFUL: return 0.1f;
        case AIPersonality::EASY: return 0.4f;
        case AIPersonality::NORMAL: return 0.7f;
        case AIPersonality::HARD: return 0.9f;
        case AIPersonality::NIGHTMARE: return 1.2f; // Extra aggressive
    }
    return 0.5f;
}

float Controller_AI_Smart::get_reaction_delay() {
    return reaction_time * (0.5f + (rand() % 100) / 200.0f); // Add some randomness
}

float Controller_AI_Smart::get_bomb_frequency_modifier() {
    return 2.0f - aggression_level; // More aggressive = bombs more frequently
}

bool Controller_AI_Smart::should_hunt_enemies() {
    return aggression_level > 0.4f && personality != AIPersonality::PEACEFUL;
}

void Controller_AI_Smart::set_reaction_time(float seconds) {
    reaction_time = std::max(0.01f, std::min(2.0f, seconds));
}

void Controller_AI_Smart::set_aggression_level(float level) {
    aggression_level = std::max(0.0f, std::min(1.0f, level));
}