#ifndef CONTROLLER_AI_MODERN_H
#define CONTROLLER_AI_MODERN_H

#include "Controller.h"
#include "UtilsCL_Vector.h"
#include "Map.h"
#include "ClanBomber.h"
#include <vector>
#include <queue>
#include <memory>
#include <string>

class ClanBomberApplication;
class Map;
class Bomber;

// Rating constants based on proven original AI
#define RATING_EXTRA       30    // Power-ups and extras
#define RATING_HOT       -100    // Bomb will explode and reach this field
#define RATING_X         -666    // Absolute death (explosion, hole)
#define RATING_BLOCKING -1000    // Never run into walls

// Destruction ratings for tactical bombing
#define DRATING_ENEMY     150    // Hitting enemies is good
#define DRATING_BOX        20    // Destroying boxes is good
#define DRATING_DISEASE    10    // Mild positive rating
#define DRATING_DBOX      -50    // Box already being destroyed
#define DRATING_EXTRA     -90    // Don't destroy power-ups
#define DRATING_BOMB     -100    // Avoid bomb chains
#define DRATING_FRIEND   -200    // Never hit friends

enum class ModernAIPersonality {
    PEACEFUL,   // Avoids conflict, focuses on survival
    EASY,       // Basic AI with slow reactions
    NORMAL,     // Balanced aggression and defense  
    HARD,       // Aggressive, predicts player moves
    NIGHTMARE   // Ruthless terminator mode
};

// Forward declaration for Job system
class Controller_AI_Modern;

// Job base class - commands for AI to execute
class AIJob {
public:
    AIJob(Controller_AI_Modern* controller);
    virtual ~AIJob();
    
    bool is_finished() const { return finished; }
    bool is_obsolete() const { return obsolete; }
    
    virtual void execute() = 0;
    virtual void init() {}
    
protected:
    bool finished = false;
    bool obsolete = false;
    
    Controller_AI_Modern* controller;
    Bomber* bomber;
    ClanBomberApplication* app;
};

// Move job - go in a specific direction for a distance
class AIJob_Go : public AIJob {
public:
    AIJob_Go(Controller_AI_Modern* controller, int direction, int distance = 1);
    virtual ~AIJob_Go();
    
    void execute() override;
    void init() override;
    
private:
    int dir;
    int distance;
    int start;
};

// Bomb job - place a bomb
class AIJob_PutBomb : public AIJob {
public:
    AIJob_PutBomb(Controller_AI_Modern* controller);
    virtual ~AIJob_PutBomb();
    
    void execute() override;
};

// Wait job - wait for a specific duration
class AIJob_Wait : public AIJob {
public:
    AIJob_Wait(Controller_AI_Modern* controller, float duration);
    virtual ~AIJob_Wait();
    
    void execute() override;
    
private:
    float duration;
};

class Controller_AI_Modern : public Controller {
    friend class AIJob_Go;
    friend class AIJob_PutBomb;
    friend class AIJob_Wait;
    
public:
    Controller_AI_Modern(ModernAIPersonality personality = ModernAIPersonality::NORMAL);
    virtual ~Controller_AI_Modern();

    void update() override;
    void reset() override;
    void attach(Bomber* _bomber) override;
    
    bool is_left() override { return current_dir == DIR_LEFT && active; }
    bool is_right() override { return current_dir == DIR_RIGHT && active; }
    bool is_up() override { return current_dir == DIR_UP && active; }
    bool is_down() override { return current_dir == DIR_DOWN && active; }
    bool is_bomb() override;

    // AI Configuration
    void set_personality(ModernAIPersonality personality);
    
    // AI State access for debugging
    std::string get_current_state() const;
    ModernAIPersonality get_personality() const { return personality; }
    
    // Public interface for jobs
    int current_dir = DIR_NONE;
    bool put_bomb = false;

private:
    // Core AI systems
    void generate_rating_map();
    bool job_ready();
    void do_job();
    void find_new_jobs();
    void clear_all_jobs();
    
    // Navigation and pathfinding (BFS-based)
    bool find_way(int dest_rating = 0, int avoid_rating = RATING_X, int max_distance = 999);
    
    // Tactical analysis
    bool avoid_bombs();
    bool find_bombing_opportunities(int max_distance = 5);
    void apply_bomb_rating(int x, int y, int power, float countdown, int dir);
    
    // Safety and utility
    bool is_hotspot(int x, int y) const;
    bool is_death(int x, int y) const;
    bool can_escape_from_bomb(int x, int y) const;
    
    // Enhanced safety functions for better decision making
    bool is_starting_corner_position(int x, int y) const;
    bool can_escape_from_bomb_safely(int x, int y) const;
    bool bombing_is_beneficial(int x, int y) const;
    bool should_move_to_better_position() const;
    
    // Bomb management and escape logic
    int count_active_bombs() const;
    int get_max_bombs() const;
    void add_escape_sequence(int bomb_x, int bomb_y);
    int find_best_escape_direction(int bomb_x, int bomb_y) const;
    int evaluate_escape_direction(int bomb_x, int bomb_y, int direction) const;
    int count_nearby_threats(int x, int y) const;
    
    // Rating calculations
    int bomber_rating(int x, int y) const;
    int extra_rating(int x, int y) const;
    
    // Personality-based behavior modifiers
    float get_aggression_modifier() const;
    float get_reaction_delay() const;
    bool should_hunt_enemies() const;
    
    // Configuration
    ModernAIPersonality personality;
    float reaction_time;
    float aggression_level;
    float last_think_time;
    float next_input_time;
    
    // Job queue
    std::vector<std::unique_ptr<AIJob>> jobs;
    
    // Map analysis
    int rating_map[MAP_WIDTH][MAP_HEIGHT];
    Map* map;
    
    // Performance optimization
    float ai_update_interval;
    float last_ai_update;
};

#endif // CONTROLLER_AI_MODERN_H