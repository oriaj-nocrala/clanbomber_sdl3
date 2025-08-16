#include "Controller_AI_Modern.h"
#include "ClanBomber.h"
#include "Map.h"
#include "Bomber.h"
#include "Timer.h"
#include "Bomb.h"
#include "MapTile.h"
#include "TileEntity.h"
#include "TileManager.h"
#include "Extra.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <queue>

// Helper functions for time management and tile access
namespace {
    
    static float total_time_accumulator = 0.0f;
    
    float get_total_time() {
        total_time_accumulator += Timer::time_elapsed();
        return total_time_accumulator;
    }
}

// ====================== AIJob Implementation ======================

AIJob::AIJob(Controller_AI_Modern* _controller) 
    : controller(_controller)
    , bomber(_controller->bomber)
    , app(_controller->bomber->app)
{
}

AIJob::~AIJob() {}

// ====================== AIJob_Go Implementation ======================

AIJob_Go::AIJob_Go(Controller_AI_Modern* _controller, int _dir, int _distance)
    : AIJob(_controller), dir(_dir), distance(_distance)
{
    init();
}

AIJob_Go::~AIJob_Go() {
    controller->current_dir = DIR_NONE;
}

void AIJob_Go::execute() {
    controller->current_dir = dir;
    
    switch(dir) {
        case DIR_UP:
            if (bomber->get_map_y() <= start - distance && bomber->get_y() % 40 < 15) {
                finished = true;
                controller->current_dir = DIR_NONE;
            }
            break;
        case DIR_DOWN:
            if (bomber->get_map_y() >= start + distance && bomber->get_y() % 40 > 25) {
                finished = true;
                controller->current_dir = DIR_NONE;
            }
            break;
        case DIR_LEFT:
            if (bomber->get_map_x() <= start - distance && bomber->get_x() % 40 < 15) {
                finished = true;
                controller->current_dir = DIR_NONE;
            }
            break;
        case DIR_RIGHT:
            if (bomber->get_map_x() >= start + distance && bomber->get_x() % 40 > 25) {
                finished = true;
                controller->current_dir = DIR_NONE;
            }
            break;
        default:
            obsolete = true;
            controller->current_dir = DIR_NONE;
            break;
    }
    
    if (bomber->is_stopped()) {
        obsolete = true;
    }
}

void AIJob_Go::init() {
    switch(dir) {
        case DIR_UP:
            if (controller->is_death(bomber->get_map_x(), bomber->get_map_y() - 1)) {
                obsolete = true;
                controller->current_dir = DIR_NONE;
            }
            start = bomber->get_map_y();
            break;
        case DIR_DOWN:
            if (controller->is_death(bomber->get_map_x(), bomber->get_map_y() + 1)) {
                obsolete = true;
                controller->current_dir = DIR_NONE;
            }
            start = bomber->get_map_y();
            break;
        case DIR_LEFT:
            if (controller->is_death(bomber->get_map_x() - 1, bomber->get_map_y())) {
                obsolete = true;
                controller->current_dir = DIR_NONE;
            }
            start = bomber->get_map_x();
            break;
        case DIR_RIGHT:
            if (controller->is_death(bomber->get_map_x() + 1, bomber->get_map_y())) {
                obsolete = true;
                controller->current_dir = DIR_NONE;
            }
            start = bomber->get_map_x();
            break;
        default:
            obsolete = true;
            controller->current_dir = DIR_NONE;
            break;
    }
}

// ====================== AIJob_PutBomb Implementation ======================

AIJob_PutBomb::AIJob_PutBomb(Controller_AI_Modern* _controller)
    : AIJob(_controller)
{
}

AIJob_PutBomb::~AIJob_PutBomb() {
    controller->put_bomb = false;
}

void AIJob_PutBomb::execute() {
    controller->put_bomb = true;
    finished = true;
    
    // Check if there's already a bomb here using new architecture
    if (bomber->has_bomb_at(bomber->get_x() + 20, bomber->get_y() + 20)) {
        obsolete = true;
        return;
    }
    
    // Simplified: always allow bomb placement, let game logic handle limits
}

// ====================== AIJob_Wait Implementation ======================

AIJob_Wait::AIJob_Wait(Controller_AI_Modern* _controller, float _duration)
    : AIJob(_controller), duration(_duration)
{
}

AIJob_Wait::~AIJob_Wait() {}

void AIJob_Wait::execute() {
    duration -= Timer::time_elapsed();
    controller->put_bomb = false;
    controller->current_dir = DIR_NONE;
    
    if (duration <= 0) {
        finished = true;
    }
    
    if (controller->is_hotspot(bomber->get_map_x(), bomber->get_map_y())) {
        obsolete = true;
    }
}

// ====================== Controller_AI_Modern Implementation ======================

Controller_AI_Modern::Controller_AI_Modern(ModernAIPersonality _personality) 
    : personality(_personality)
    , reaction_time(0.1f)
    , aggression_level(0.5f)
    , last_think_time(0.0f)
    , next_input_time(0.0f)
    , ai_update_interval(0.05f) // 20 FPS AI thinking
    , last_ai_update(0.0f)
{
    c_type = AI;
    set_personality(_personality);
    reset();
}

Controller_AI_Modern::~Controller_AI_Modern() {
    clear_all_jobs();
}

void Controller_AI_Modern::set_personality(ModernAIPersonality new_personality) {
    personality = new_personality;
    
    switch (personality) {
        case ModernAIPersonality::PEACEFUL:
            aggression_level = 0.1f;
            reaction_time = 0.8f;
            break;
        case ModernAIPersonality::EASY:
            aggression_level = 0.3f;
            reaction_time = 0.5f;
            break;
        case ModernAIPersonality::NORMAL:
            aggression_level = 0.5f;
            reaction_time = 0.2f;
            break;
        case ModernAIPersonality::HARD:
            aggression_level = 0.8f;
            reaction_time = 0.1f;
            break;
        case ModernAIPersonality::NIGHTMARE:
            aggression_level = 1.0f;
            reaction_time = 0.03f;
            break;
    }
}

void Controller_AI_Modern::attach(Bomber* _bomber) {
    // Call base class attach
    Controller::attach(_bomber);
    
    // Update map reference when bomber is attached
    if (bomber && bomber->app) {
        map = bomber->app->map;
    }
}

void Controller_AI_Modern::reset() {
    current_dir = DIR_NONE;
    put_bomb = false;
    
    // Only set map if bomber is available
    if (bomber && bomber->app) {
        map = bomber->app->map;
    } else {
        map = nullptr;
    }
    
    clear_all_jobs();
    
    // Initialize rating map
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            rating_map[x][y] = 0;
        }
    }
}

void Controller_AI_Modern::update() {
    if (!active || !bomber || !map) return;
    
    float current_time = get_total_time();
    
    // Performance optimization - don't think every frame
    if (current_time - last_ai_update >= ai_update_interval) {
        generate_rating_map();
        
        if (job_ready()) {
            do_job();
        }
        
        last_ai_update = current_time;
    }
}

void Controller_AI_Modern::generate_rating_map() {
    // Reset map to neutral
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            rating_map[x][y] = 0;
        }
    }
    
    // Analyze all objects in the game
    for (auto& obj : bomber->app->objects) {
        if (!obj) continue;
        
        int x = obj->get_map_x();
        int y = obj->get_map_y();
        
        switch (obj->get_type()) {
            case GameObject::BOMB: {
                // Simplified: assume standard bomb power and countdown
                apply_bomb_rating(x, y, 2, 3.0f, DIR_NONE);
                break;
            }
            case GameObject::EXPLOSION: {
                // Mark explosion areas as death
                rating_map[x][y] += RATING_X;
                // TODO: Add explosion ray calculation similar to original
                break;
            }
            case GameObject::EXTRA: {
                // Simplified: all extras are good
                rating_map[x][y] += RATING_EXTRA;
                break;
            }
            default:
                break;
        }
    }
    
    // Apply map tile ratings using new architecture
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (bomber->app->tile_manager->is_tile_blocking_at(x, y)) {
                rating_map[x][y] += RATING_BLOCKING;
            }
        }
    }
}

void Controller_AI_Modern::apply_bomb_rating(int x, int y, int power, float countdown, int dir) {
    int rating = RATING_HOT;
    
    // Different ratings based on countdown
    if (countdown > 2.9f) {
        rating = -1; // Safe for now
    } else if (countdown < 40.0f / (float)bomber->get_speed()) {
        rating = RATING_X; // Immediate death
    }
    
    // Apply rating to bomb position
    rating_map[x][y] = rating;
    
    // Apply explosion rays using new architecture
    for (int i = 1; i <= power && x + i < MAP_WIDTH; i++) {
        if (bomber->app->tile_manager->is_tile_blocking_at(x + i, y)) break;
        rating_map[x + i][y] += rating;
    }
    
    for (int i = 1; i <= power && x - i >= 0; i++) {
        if (bomber->app->tile_manager->is_tile_blocking_at(x - i, y)) break;
        rating_map[x - i][y] += rating;
    }
    
    for (int i = 1; i <= power && y + i < MAP_HEIGHT; i++) {
        if (bomber->app->tile_manager->is_tile_blocking_at(x, y + i)) break;
        rating_map[x][y + i] += rating;
    }
    
    for (int i = 1; i <= power && y - i >= 0; i++) {
        if (bomber->app->tile_manager->is_tile_blocking_at(x, y - i)) break;
        rating_map[x][y - i] += rating;
    }
}

bool Controller_AI_Modern::job_ready() {
    if (jobs.empty()) {
        find_new_jobs();
        return !jobs.empty();
    }
    
    if (jobs[0]->is_obsolete()) {
        clear_all_jobs();
        return job_ready();
    }
    
    if (jobs[0]->is_finished()) {
        jobs.erase(jobs.begin());
        if (!jobs.empty()) {
            jobs[0]->init();
        }
        return job_ready();
    }
    
    return true;
}

void Controller_AI_Modern::find_new_jobs() {
    // Priority 1: Avoid bombs
    if (avoid_bombs()) {
        return;
    }
    
    // Priority 2: Go for power-ups
    if (find_way(RATING_EXTRA, RATING_HOT, 10)) {
        return;
    }
    
    // Priority 3: Move to safer/better position first (especially at game start)
    if (should_move_to_better_position()) {
        return;
    }
    
    // Priority 4: Look for bombing opportunities (only if safe)
    if (find_bombing_opportunities(5)) {
        return;
    }
    
    // Priority 5: Default movement if nothing else to do
    if (find_way(0, RATING_HOT, 3)) {
        return;
    }
}

void Controller_AI_Modern::do_job() {
    if (!jobs.empty()) {
        jobs[0]->execute();
    }
}

bool Controller_AI_Modern::avoid_bombs() {
    int x = bomber->get_map_x();
    int y = bomber->get_map_y();
    
    if (is_hotspot(x, y) || is_death(x, y)) {
        // Clear all jobs - this is emergency mode!
        clear_all_jobs();
        
        // Try emergency escape
        if (!find_way(0, -1, 8)) {
            // If not possible, find moderately safe area
            if (!find_way(0, RATING_HOT, 5)) {
                // Last resort: any non-death area
                find_way(0, RATING_X, 3);
            }
        }
        return true;
    }
    
    // Also check if we're too close to multiple bombs
    if (count_nearby_threats(x, y) >= 2) {
        // Move to safer area
        find_way(0, RATING_HOT, 4);
        return true;
    }
    
    return false;
}

bool Controller_AI_Modern::find_bombing_opportunities(int max_distance) {
    int x = bomber->get_map_x();
    int y = bomber->get_map_y();
    
    // Don't bomb if we're in starting corner positions (too dangerous)
    if (is_starting_corner_position(x, y)) {
        return false;
    }
    
    // Don't bomb if personality is peaceful
    if (personality == ModernAIPersonality::PEACEFUL) {
        return false;
    }
    
    // Check if we already have too many bombs placed
    if (count_active_bombs() >= get_max_bombs()) {
        return false;
    }
    
    // Check if there's already a bomb at current position using new architecture
    if (bomber->app->tile_manager->has_bomb_at(x, y)) {
        return false;
    }
    
    // More strict escape check - need multiple safe escape routes
    if (!can_escape_from_bomb_safely(x, y)) {
        return false;
    }
    
    // Check if bombing here would be beneficial (destroy boxes, trap enemies, etc.)
    if (bombing_is_beneficial(x, y)) {
        jobs.push_back(std::make_unique<AIJob_PutBomb>(this));
        // Add mandatory escape job after bombing
        add_escape_sequence(x, y);
        return true;
    }
    
    return false;
}

bool Controller_AI_Modern::find_way(int dest_rating, int avoid_rating, int max_distance) {
    std::vector<std::vector<int>> visit_map(MAP_WIDTH, std::vector<int>(MAP_HEIGHT, -1));
    std::queue<CL_Vector> new_queue;
    std::queue<CL_Vector> working_queue;
    
    int distance = 0;
    CL_Vector start(bomber->get_map_x(), bomber->get_map_y());
    CL_Vector dest(-1, -1, -1);
    
    visit_map[(int)start.x][(int)start.y] = 0;
    new_queue.push(start);
    
    while (distance < max_distance && dest.x < 0 && !new_queue.empty()) {
        // Move new queue to working queue
        while (!new_queue.empty()) {
            working_queue.push(new_queue.front());
            new_queue.pop();
        }
        distance++;
        
        while (!working_queue.empty() && dest.x < 0) {
            CL_Vector current = working_queue.front();
            working_queue.pop();
            
            // Check all 4 directions
            int directions[] = {DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT};
            
            // Randomize direction order for variety
            for (int i = 0; i < 10; i++) {
                int a = rand() % 4;
                int b = rand() % 4;
                std::swap(directions[a], directions[b]);
            }
            
            for (int dir : directions) {
                CL_Vector next = current;
                
                switch (dir) {
                    case DIR_UP:    if (next.y > 0) next.y--; else continue; break;
                    case DIR_DOWN:  if (next.y < MAP_HEIGHT-1) next.y++; else continue; break;
                    case DIR_LEFT:  if (next.x > 0) next.x--; else continue; break;
                    case DIR_RIGHT: if (next.x < MAP_WIDTH-1) next.x++; else continue; break;
                }
                
                int nx = (int)next.x;
                int ny = (int)next.y;
                if (rating_map[nx][ny] > avoid_rating && visit_map[nx][ny] < 0) {
                    new_queue.push(next);
                    visit_map[nx][ny] = distance;
                    
                    if (rating_map[nx][ny] >= dest_rating) {
                        dest = next;
                    }
                }
            }
        }
    }
    
    if (dest.x < 0) {
        return false;
    }
    
    // Backtrack to create job sequence
    std::vector<std::unique_ptr<AIJob>> reverse_path;
    
    while (!(start.x == dest.x && start.y == dest.y)) {
        distance--;
        
        int dx = (int)dest.x;
        int dy = (int)dest.y;
        
        if (dy > 0 && visit_map[dx][dy - 1] == distance) {
            reverse_path.push_back(std::make_unique<AIJob_Go>(this, DIR_DOWN));
            dest.y--;
        } else if (dx < MAP_WIDTH-1 && visit_map[dx + 1][dy] == distance) {
            reverse_path.push_back(std::make_unique<AIJob_Go>(this, DIR_LEFT));
            dest.x++;
        } else if (dy < MAP_HEIGHT-1 && visit_map[dx][dy + 1] == distance) {
            reverse_path.push_back(std::make_unique<AIJob_Go>(this, DIR_UP));
            dest.y++;
        } else if (dx > 0 && visit_map[dx - 1][dy] == distance) {
            reverse_path.push_back(std::make_unique<AIJob_Go>(this, DIR_RIGHT));
            dest.x--;
        }
    }
    
    // Add jobs in correct order
    if (dest_rating > 0) {
        // Just take one step towards power-up
        if (!reverse_path.empty()) {
            jobs.push_back(std::move(reverse_path.back()));
        }
    } else {
        // Take full path for safety
        for (auto it = reverse_path.rbegin(); it != reverse_path.rend(); ++it) {
            jobs.push_back(std::move(*it));
        }
    }
    
    return true;
}

void Controller_AI_Modern::clear_all_jobs() {
    jobs.clear();
}

bool Controller_AI_Modern::is_hotspot(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return true;
    return rating_map[x][y] <= RATING_HOT;
}

bool Controller_AI_Modern::is_death(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return true;
    return rating_map[x][y] <= RATING_X;
}

bool Controller_AI_Modern::can_escape_from_bomb(int x, int y) const {
    // Simplified escape check - could be more sophisticated
    // Check if there are safe adjacent tiles
    int safe_tiles = 0;
    
    if (x > 0 && !is_hotspot(x - 1, y)) safe_tiles++;
    if (x < MAP_WIDTH - 1 && !is_hotspot(x + 1, y)) safe_tiles++;
    if (y > 0 && !is_hotspot(x, y - 1)) safe_tiles++;
    if (y < MAP_HEIGHT - 1 && !is_hotspot(x, y + 1)) safe_tiles++;
    
    return safe_tiles >= 2; // Need multiple escape routes
}

int Controller_AI_Modern::bomber_rating(int x, int y) const {
    // TODO: Implement bomber rating system
    return 0;
}

int Controller_AI_Modern::extra_rating(int x, int y) const {
    // TODO: Implement extra rating system
    return 0;
}

float Controller_AI_Modern::get_aggression_modifier() const {
    switch (personality) {
        case ModernAIPersonality::PEACEFUL: return 0.1f;
        case ModernAIPersonality::EASY: return 0.4f;
        case ModernAIPersonality::NORMAL: return 0.7f;
        case ModernAIPersonality::HARD: return 0.9f;
        case ModernAIPersonality::NIGHTMARE: return 1.2f;
    }
    return 0.5f;
}

float Controller_AI_Modern::get_reaction_delay() const {
    return reaction_time * (0.5f + (rand() % 100) / 200.0f);
}

bool Controller_AI_Modern::should_hunt_enemies() const {
    return aggression_level > 0.4f && personality != ModernAIPersonality::PEACEFUL;
}

bool Controller_AI_Modern::is_bomb() {
    switch (bomb_mode) {
        case NEVER: return false;
        case ALWAYS: return true;
        default: break;
    }
    return put_bomb && active;
}

std::string Controller_AI_Modern::get_current_state() const {
    if (jobs.empty()) return "IDLE";
    
    // Try to determine current job type
    if (dynamic_cast<const AIJob_Go*>(jobs[0].get())) return "MOVING";
    if (dynamic_cast<const AIJob_PutBomb*>(jobs[0].get())) return "BOMBING";
    if (dynamic_cast<const AIJob_Wait*>(jobs[0].get())) return "WAITING";
    
    return "UNKNOWN";
}

// ====================== New Safety Functions ======================

bool Controller_AI_Modern::is_starting_corner_position(int x, int y) const {
    // Check if we're in typical starting corner positions where bombing is suicide
    return (x <= 1 && y <= 1) ||           // Top-left corner
           (x >= MAP_WIDTH-2 && y <= 1) ||  // Top-right corner  
           (x <= 1 && y >= MAP_HEIGHT-2) || // Bottom-left corner
           (x >= MAP_WIDTH-2 && y >= MAP_HEIGHT-2); // Bottom-right corner
}

bool Controller_AI_Modern::can_escape_from_bomb_safely(int x, int y) const {
    // More strict escape check than the original
    int safe_routes = 0;
    int power = 2; // Assume standard bomb power
    
    // Check all 4 directions for escape routes
    for (int dir = 0; dir < 4; dir++) {
        int dx = 0, dy = 0;
        switch (dir) {
            case 0: dy = -1; break; // UP
            case 1: dx = 1; break;  // RIGHT
            case 2: dy = 1; break;  // DOWN  
            case 3: dx = -1; break; // LEFT
        }
        
        // Check if we can move far enough away from blast
        bool route_safe = true;
        for (int dist = 1; dist <= power + 1; dist++) {
            int nx = x + dx * dist;
            int ny = y + dy * dist;
            
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) {
                route_safe = false;
                break;
            }
            
            if (is_hotspot(nx, ny) || rating_map[nx][ny] <= RATING_HOT) {
                route_safe = false;
                break;
            }
            
            if (bomber->app->tile_manager->is_tile_blocking_at(nx, ny)) {
                route_safe = false;
                break;
            }
        }
        
        if (route_safe) {
            safe_routes++;
        }
    }
    
    // Need at least 1-2 safe escape routes based on personality
    int required_routes = (personality == ModernAIPersonality::NIGHTMARE) ? 1 : 
                         (personality == ModernAIPersonality::HARD) ? 1 : 
                         (personality == ModernAIPersonality::NORMAL) ? 2 : 2;
    
    return safe_routes >= required_routes;
}

bool Controller_AI_Modern::bombing_is_beneficial(int x, int y) const {
    // Check if bombing at this position would be beneficial
    int benefit_score = 0;
    int power = 2; // Assume standard bomb power
    
    // Check all 4 directions for destructible targets
    for (int dir = 0; dir < 4; dir++) {
        int dx = 0, dy = 0;
        switch (dir) {
            case 0: dy = -1; break; // UP
            case 1: dx = 1; break;  // RIGHT
            case 2: dy = 1; break;  // DOWN  
            case 3: dx = -1; break; // LEFT
        }
        
        for (int dist = 1; dist <= power; dist++) {
            int nx = x + dx * dist;
            int ny = y + dy * dist;
            
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) break;
            
            if (bomber->app->tile_manager->is_tile_blocking_at(nx, ny) && !bomber->app->tile_manager->is_tile_destructible_at(nx, ny)) {
                break; // Hit wall, stop checking this direction
            }
            if (bomber->app->tile_manager->is_tile_destructible_at(nx, ny)) {
                benefit_score += 10; // Points for destroying boxes
                break; // Stop after hitting destructible box
            }
        }
    }
    
    // Only bomb if we get some benefit and it matches our aggression level
    int min_benefit = (personality == ModernAIPersonality::NIGHTMARE) ? 5 :
                     (personality == ModernAIPersonality::HARD) ? 8 :
                     (personality == ModernAIPersonality::NORMAL) ? 10 : 15;
    
    return benefit_score >= min_benefit;
}

bool Controller_AI_Modern::should_move_to_better_position() const {
    int x = bomber->get_map_x();
    int y = bomber->get_map_y();
    
    // If we're in a starting corner, prioritize moving to center
    if (is_starting_corner_position(x, y)) {
        // Find path toward center of map
        int center_x = MAP_WIDTH / 2;
        int center_y = MAP_HEIGHT / 2;
        
        // Simple movement toward center
        if (abs(x - center_x) > abs(y - center_y)) {
            // Move horizontally toward center
            int target_x = (x < center_x) ? x + 1 : x - 1;
            if (target_x >= 0 && target_x < MAP_WIDTH && !is_death(target_x, y)) {
                const_cast<Controller_AI_Modern*>(this)->jobs.push_back(
                    std::make_unique<AIJob_Go>(const_cast<Controller_AI_Modern*>(this), 
                                             (x < center_x) ? DIR_RIGHT : DIR_LEFT, 1));
                return true;
            }
        } else {
            // Move vertically toward center
            int target_y = (y < center_y) ? y + 1 : y - 1;
            if (target_y >= 0 && target_y < MAP_HEIGHT && !is_death(x, target_y)) {
                const_cast<Controller_AI_Modern*>(this)->jobs.push_back(
                    std::make_unique<AIJob_Go>(const_cast<Controller_AI_Modern*>(this), 
                                             (y < center_y) ? DIR_DOWN : DIR_UP, 1));
                return true;
            }
        }
    }
    
    return false;
}

// ====================== Bomb Management Functions ======================

int Controller_AI_Modern::count_active_bombs() const {
    int count = 0;
    // Count bombs on the map that belong to this bomber
    for (auto& obj : bomber->app->objects) {
        if (obj && obj->get_type() == GameObject::BOMB) {
            // Simplified: count all bombs (could check ownership if bomber ID available)
            count++;
        }
    }
    return count;
}

int Controller_AI_Modern::get_max_bombs() const {
    // Conservative bomb limit based on personality
    switch (personality) {
        case ModernAIPersonality::PEACEFUL: return 1;
        case ModernAIPersonality::EASY: return 1;
        case ModernAIPersonality::NORMAL: return 2;
        case ModernAIPersonality::HARD: return 2;
        case ModernAIPersonality::NIGHTMARE: return 3;
    }
    return 1; // Safe default
}

void Controller_AI_Modern::add_escape_sequence(int bomb_x, int bomb_y) {
    // Find the best escape direction and add movement jobs
    int best_dir = find_best_escape_direction(bomb_x, bomb_y);
    
    if (best_dir != DIR_NONE) {
        // Add jobs to move away from bomb position
        jobs.push_back(std::make_unique<AIJob_Go>(this, best_dir, 3));
        
        // Add a wait job to let bomb explode
        jobs.push_back(std::make_unique<AIJob_Wait>(this, 4.0f));
    }
}

int Controller_AI_Modern::find_best_escape_direction(int bomb_x, int bomb_y) const {
    int best_dir = DIR_NONE;
    int best_safety_score = -1000;
    
    // Check all 4 directions
    int directions[] = {DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT};
    
    for (int dir : directions) {
        int safety_score = evaluate_escape_direction(bomb_x, bomb_y, dir);
        if (safety_score > best_safety_score) {
            best_safety_score = safety_score;
            best_dir = dir;
        }
    }
    
    return best_dir;
}

int Controller_AI_Modern::evaluate_escape_direction(int bomb_x, int bomb_y, int direction) const {
    int dx = 0, dy = 0;
    switch (direction) {
        case DIR_UP: dy = -1; break;
        case DIR_DOWN: dy = 1; break;
        case DIR_LEFT: dx = -1; break;
        case DIR_RIGHT: dx = 1; break;
        default: return -1000;
    }
    
    int safety_score = 0;
    int power = 2; // Assume standard bomb power
    
    // Check how far we can safely go in this direction
    for (int dist = 1; dist <= power + 2; dist++) {
        int nx = bomb_x + dx * dist;
        int ny = bomb_y + dy * dist;
        
        if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) {
            break; // Hit boundary
        }
        
        if (is_death(nx, ny) || is_hotspot(nx, ny)) {
            safety_score -= 50; // Penalty for dangerous areas
            break;
        }
        
        if (bomber->app->tile_manager->is_tile_blocking_at(nx, ny)) {
            break; // Hit wall
        }
        
        safety_score += 10; // Bonus for each safe step
        
        // Extra bonus for getting completely out of bomb range
        if (dist > power) {
            safety_score += 50;
        }
    }
    
    return safety_score;
}

int Controller_AI_Modern::count_nearby_threats(int x, int y) const {
    int threat_count = 0;
    int range = 3; // Check in 3x3 area around position
    
    for (int dy = -range; dy <= range; dy++) {
        for (int dx = -range; dx <= range; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            
            if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
                if (is_hotspot(nx, ny) || rating_map[nx][ny] <= RATING_HOT) {
                    threat_count++;
                }
            }
        }
    }
    
    return threat_count;
}