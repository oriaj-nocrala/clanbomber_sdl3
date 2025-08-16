#ifndef CONTROLLER_AI_SMART_H
#define CONTROLLER_AI_SMART_H

#include "Controller.h"
#include "UtilsCL_Vector.h"
#include <vector>
#include <memory>
#include <string>

class ClanBomberApplication;
class Map;
class Bomber;

enum class AIPersonality {
    PEACEFUL,   // Avoids conflict, focuses on survival
    EASY,       // Basic AI with slow reactions
    NORMAL,     // Balanced aggression and defense  
    HARD,       // Aggressive, predicts player moves
    NIGHTMARE   // Ruthless terminator mode
};

struct AITarget {
    CL_Vector position;
    float priority;
    float distance;
    bool is_powerup;
    bool is_enemy;
    bool is_safe_path;
};

class Controller_AI_Smart : public Controller {
public:
    Controller_AI_Smart(AIPersonality personality = AIPersonality::NORMAL);
    virtual ~Controller_AI_Smart();

    void update() override;
    void reset() override;
    
    bool is_left() override { return current_input.left; }
    bool is_right() override { return current_input.right; }
    bool is_up() override { return current_input.up; }
    bool is_down() override { return current_input.down; }
    bool is_bomb() override { return current_input.bomb; }

    // AI Configuration
    void set_personality(AIPersonality personality);
    void set_reaction_time(float seconds);
    void set_aggression_level(float level); // 0.0 = defensive, 1.0 = very aggressive

    // AI State access for debugging
    CL_Vector get_current_target() const { return current_target; }
    std::string get_current_state() const { return current_state_name; }

private:
    struct AIInput {
        bool left = false;
        bool right = false; 
        bool up = false;
        bool down = false;
        bool bomb = false;
    };

    // Core AI systems
    void think();
    void execute_behavior();
    
    // Navigation and pathfinding
    std::vector<CL_Vector> find_path_to(CL_Vector target);
    bool is_position_safe(CL_Vector pos, float time_ahead = 2.0f);
    float calculate_danger_level(CL_Vector pos);
    CL_Vector find_safe_position();
    
    // Target selection and strategy
    std::vector<AITarget> scan_for_targets();
    AITarget select_best_target(const std::vector<AITarget>& targets);
    float evaluate_powerup_value(int powerup_type);
    
    // Combat and bombing
    bool should_place_bomb();
    bool can_escape_from_bomb(CL_Vector bomb_pos);
    std::vector<CL_Vector> predict_explosion_tiles(CL_Vector bomb_pos, int power);
    bool would_hit_enemy(CL_Vector bomb_pos);
    
    // Opponent analysis
    void analyze_enemies();
    CL_Vector predict_enemy_position(Bomber* enemy, float time_ahead);
    bool is_enemy_dangerous(Bomber* enemy);
    
    // Personality-based behavior modifiers
    float get_aggression_modifier();
    float get_reaction_delay();
    float get_bomb_frequency_modifier();
    bool should_hunt_enemies();
    
    // State management
    enum class AIState {
        EXPLORING,      // Looking for targets/powerups
        HUNTING,        // Pursuing an enemy
        FLEEING,        // Escaping from danger
        COLLECTING,     // Going for powerups
        BOMBING,        // Placing strategic bombs
        WAITING         // Waiting for safe moment
    };
    
    void transition_to_state(AIState new_state);
    void update_current_state();

    // AI Configuration
    AIPersonality personality;
    float reaction_time;
    float aggression_level;
    float thinking_frequency; // How often AI recalculates (performance)
    
    // Current AI state
    AIInput current_input;
    AIState current_state;
    std::string current_state_name;
    CL_Vector current_target;
    float last_think_time;
    float next_input_time; // For reaction delay simulation
    
    // Timers and delays
    float bomb_cooldown_ai;
    float last_bomb_time;
    float stuck_timer;
    CL_Vector last_position;
    
    // Memory and learning
    std::vector<CL_Vector> dangerous_positions;
    std::vector<CL_Vector> recently_bombed_positions;
    float memory_fade_time;
    
    // Performance optimization
    float ai_update_interval;
    float last_ai_update;
};

#endif // CONTROLLER_AI_SMART_H