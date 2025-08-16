#ifndef LIFECYCLE_MANAGER_H
#define LIFECYCLE_MANAGER_H

#include <vector>
#include <memory>
#include <functional>

// Forward declarations
class GameObject;
class MapTile;

/**
 * Unified lifecycle management for all game objects and tiles
 * Eliminates the chaos of multiple deletion systems
 */
class LifecycleManager {
public:
    enum class ObjectState {
        ACTIVE,      // Normal operation
        DYING,       // In destruction animation, still renders
        DEAD,        // Animation complete, ready for cleanup
        DELETED      // Removed from game, pending memory cleanup
    };
    
    struct ManagedObject {
        GameObject* object;
        ObjectState state;
        float state_timer;      // Time spent in current state
        std::function<void()> on_state_change;  // Optional callback
        
        ManagedObject(GameObject* obj) 
            : object(obj), state(ObjectState::ACTIVE), state_timer(0.0f) {}
    };
    
    struct ManagedTile {
        MapTile* tile;
        int map_x, map_y;       // Grid coordinates
        ObjectState state;
        float state_timer;
        MapTile* replacement;   // What to replace with when DEAD
        
        ManagedTile(MapTile* t, int x, int y) 
            : tile(t), map_x(x), map_y(y), state(ObjectState::ACTIVE), 
              state_timer(0.0f), replacement(nullptr) {}
    };

public:
    LifecycleManager();
    ~LifecycleManager();
    
    // Registration
    void register_object(GameObject* obj);
    void register_tile(MapTile* tile, int map_x, int map_y);
    
    // State management
    void mark_for_destruction(GameObject* obj);
    void mark_tile_for_destruction(MapTile* tile, MapTile* replacement = nullptr);
    
    // Core lifecycle update - called once per frame
    void update_states(float deltaTime);
    void cleanup_dead_objects();
    
    // Query state
    ObjectState get_object_state(GameObject* obj) const;
    ObjectState get_tile_state(MapTile* tile) const;
    bool is_dying_or_dead(GameObject* obj) const;
    bool is_dying_or_dead(MapTile* tile) const;
    
    // Utilities
    void clear_all();
    size_t get_active_object_count() const;
    size_t get_active_tile_count() const;

private:
    std::vector<ManagedObject> managed_objects;
    std::vector<ManagedTile> managed_tiles;
    
    // Internal helpers
    ManagedObject* find_managed_object(GameObject* obj);
    ManagedTile* find_managed_tile(MapTile* tile);
    const ManagedObject* find_managed_object(GameObject* obj) const;
    const ManagedTile* find_managed_tile(MapTile* tile) const;
    
    void update_object_state(ManagedObject& managed, float deltaTime);
    void update_tile_state(ManagedTile& managed, float deltaTime);
};

#endif