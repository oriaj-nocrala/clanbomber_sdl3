#ifndef GAMECONTEXT_H
#define GAMECONTEXT_H

#include <list>

class LifecycleManager;
class TileManager;
class ParticleEffectsManager;
class Map;
class GPUAcceleratedRenderer;
class TextRenderer;
class GameObject;
class SpatialGrid;
class RenderingFacade;

/**
 * GameContext: Dependency Injection Container
 * 
 * Provides controlled access to game systems without tight coupling
 * to ClanBomberApplication. Makes testing and modularization easier.
 */
class GameContext {
public:
    GameContext(LifecycleManager* lifecycle,
                TileManager* tiles,
                ParticleEffectsManager* effects,
                Map* map,
                GPUAcceleratedRenderer* renderer,
                TextRenderer* text,
                RenderingFacade* facade = nullptr);
    
    ~GameContext();
    
    // Set rendering object list (called during initialization)
    void set_object_lists(std::list<GameObject*>* objects);
    
    // Get object lists for iteration (needed for systems like Explosion)
    // Returns const reference to prevent unsafe external modification
    const std::list<GameObject*>& get_object_lists() const { 
        if (!render_objects) {
            static std::list<GameObject*> empty_list;
            return empty_list;
        }
        return *render_objects; 
    }
    
    // System access
    LifecycleManager* get_lifecycle_manager() const { return lifecycle_manager; }
    TileManager* get_tile_manager() const { return tile_manager; }
    ParticleEffectsManager* get_particle_effects() const { return particle_effects; }
    Map* get_map() const { return map; }
    GPUAcceleratedRenderer* get_renderer() const { return nullptr; } // REMOVED: Legacy renderer
    TextRenderer* get_text_renderer() const { return text_renderer; }
    SpatialGrid* get_spatial_grid() const { return spatial_grid; }
    RenderingFacade* get_rendering_facade() const { return rendering_facade; }
    
    // Convenience methods (reduces boilerplate)
    bool is_position_blocked(int map_x, int map_y) const;
    bool has_bomb_at(int map_x, int map_y) const;
    bool is_position_walkable(int map_x, int map_y) const;
    
    void request_destruction_effect(float x, float y, float intensity = 1.0f) const;
    void mark_for_destruction(class GameObject* obj) const;
    
    // GameObject lifecycle management
    void register_object(class GameObject* obj) const;
    
    // Two-phase initialization
    void set_map(Map* new_map);

private:
    LifecycleManager* lifecycle_manager;
    TileManager* tile_manager;
    ParticleEffectsManager* particle_effects;
    Map* map;
    GPUAcceleratedRenderer* gpu_renderer; // REMOVED: Legacy renderer - kept for compatibility
    TextRenderer* text_renderer;
    SpatialGrid* spatial_grid;
    RenderingFacade* rendering_facade;
    
    // Rendering object list (for adding objects to be rendered)
    std::list<GameObject*>* render_objects;
};

#endif