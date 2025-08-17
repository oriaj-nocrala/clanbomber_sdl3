#ifndef GAMECONTEXT_H
#define GAMECONTEXT_H

class LifecycleManager;
class TileManager;
class ParticleEffectsManager;
class Map;
class GPUAcceleratedRenderer;
class TextRenderer;

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
                TextRenderer* text);
    
    // System access
    LifecycleManager* get_lifecycle_manager() const { return lifecycle_manager; }
    TileManager* get_tile_manager() const { return tile_manager; }
    ParticleEffectsManager* get_particle_effects() const { return particle_effects; }
    Map* get_map() const { return map; }
    GPUAcceleratedRenderer* get_renderer() const { return gpu_renderer; }
    TextRenderer* get_text_renderer() const { return text_renderer; }
    
    // Convenience methods (reduces boilerplate)
    bool is_position_blocked(int map_x, int map_y) const;
    bool has_bomb_at(int map_x, int map_y) const;
    bool is_position_walkable(int map_x, int map_y) const;
    
    void request_destruction_effect(float x, float y, float intensity = 1.0f) const;
    void mark_for_destruction(class GameObject* obj) const;
    
    // GameObject lifecycle management
    void register_object(class GameObject* obj) const;

private:
    LifecycleManager* lifecycle_manager;
    TileManager* tile_manager;
    ParticleEffectsManager* particle_effects;
    Map* map;
    GPUAcceleratedRenderer* gpu_renderer;
    TextRenderer* text_renderer;
};

#endif