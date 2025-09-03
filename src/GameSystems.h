#ifndef GAMESYSTEMS_H
#define GAMESYSTEMS_H

#include <list>
#include <memory>

class GameObject;
class Bomber;
class GameContext;

/**
 * GameSystems: Extract core game logic from GameplayScreen
 * 
 * Breaks down the monolithic act_all() into specialized systems
 * that handle specific responsibilities.
 */
class GameSystems {
public:
    GameSystems(GameContext* context);
    ~GameSystems();
    
    // System update - replaces act_all() / show_all()
    void update_all_systems(float deltaTime);
    void render_all_systems();
    
    // Object management - replaces direct container manipulation
    void register_object(GameObject* obj);
    void register_bomber(Bomber* bomber);
    void cleanup_destroyed_objects();
    
    // Setup method for object references
    void set_object_references(std::list<std::unique_ptr<GameObject>>* objects, 
                              std::list<std::unique_ptr<Bomber>>* bombers);
    
    // Initialize all systems
    void init_all_systems();
    
private:
    GameContext* context;
    
    // System phases
    void update_input_system(float deltaTime);
    void update_physics_system(float deltaTime);
    void update_ai_system(float deltaTime);
    void update_animation_system(float deltaTime);
    void update_collision_system(float deltaTime);
    
    void render_world();
    void render_effects();
    void render_ui();
    
    // Object lists (move from ClanBomberApplication eventually)
    std::list<std::unique_ptr<GameObject>>* objects_ref;
    std::list<std::unique_ptr<Bomber>>* bombers_ref;
    
    // System state
    bool systems_initialized = false;
};

#endif