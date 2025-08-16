# GameContext Usage Guide

## Overview

GameContext es un **Dependency Injection Container** que reemplaza el acoplamiento fuerte con `ClanBomberApplication*`. Proporciona acceso controlado a sistemas de juego sin exponer la implementación completa.

## Problem Solved

### Before (Tight Coupling)
```cpp
class Bomber : public GameObject {
    Bomber(int x, int y, ClanBomberApplication* app) : GameObject(x, y, app) {
        // Bomber now knows about EVERYTHING in the application
        // Hard to test, hard to change
    }
    
    void move() {
        // Direct access to implementation details
        if (app->map->get_tile(x, y)->is_blocking()) {
            // Tightly coupled to Map internal structure
        }
        
        if (app->gpu_renderer->some_internal_method()) {
            // Inappropriate access to renderer internals
        }
    }
};
```

### After (Dependency Injection)
```cpp
class Bomber : public GameObject {
    Bomber(int x, int y, GameContext* context) : GameObject(x, y, context) {
        // Bomber only knows about the interfaces it needs
        // Easy to test with mocks, easy to change
    }
    
    void move() {
        // Clean, intention-revealing API
        if (context->is_position_blocked(map_x, map_y)) {
            // Decoupled from implementation
        }
        
        context->request_destruction_effect(x, y);
        // High-level, purpose-driven API
    }
};
```

## API Reference

### Core System Access
```cpp
class GameContext {
    // System getters - use sparingly, prefer convenience methods
    LifecycleManager* get_lifecycle_manager() const;
    TileManager* get_tile_manager() const;
    ParticleEffectsManager* get_particle_effects() const;
    Map* get_map() const;
    GPUAcceleratedRenderer* get_renderer() const;
    TextRenderer* get_text_renderer() const;
};
```

### Convenience Methods (Preferred)
```cpp
class GameContext {
    // Tile queries - most common operations
    bool is_position_blocked(int map_x, int map_y) const;
    bool has_bomb_at(int map_x, int map_y) const;
    bool is_position_walkable(int map_x, int map_y) const;
    
    // Effect requests - decoupled from implementation
    void request_destruction_effect(float x, float y, float intensity = 1.0f) const;
    
    // Lifecycle management - simplified API
    void mark_for_destruction(GameObject* obj) const;
};
```

## Usage Patterns

### 1. Object Construction
```cpp
// OLD: Tight coupling
Bomber* bomber = new Bomber(x, y, app);
Bomb* bomb = new Bomb(x, y, power, owner, app);

// NEW: Dependency injection
Bomber* bomber = new Bomber(x, y, context);
Bomb* bomb = new Bomb(x, y, power, owner, context);
```

### 2. Tile Queries
```cpp
// OLD: Knowledge of dual architecture
MapTile* legacy_tile = app->map->get_tile(map_x, map_y);
if (legacy_tile && legacy_tile->is_blocking()) { /* handle */ }

TileEntity* tile_entity = app->map->get_tile_entity(map_x, map_y);
if (tile_entity && tile_entity->is_blocking()) { /* handle */ }

// NEW: Clean abstraction
if (context->is_position_blocked(map_x, map_y)) {
    // Handle collision - don't care about implementation
}
```

### 3. Effect Requests
```cpp
// OLD: Direct coupling to effects system
if (app->particle_effects) {
    app->particle_effects->create_box_destruction_effect(x, y, 1.0f);
}

// NEW: Clean request interface
context->request_destruction_effect(x, y, intensity);
```

### 4. Lifecycle Management
```cpp
// OLD: Direct manipulation
if (app->lifecycle_manager) {
    app->lifecycle_manager->mark_for_destruction(this);
}

// NEW: Simplified API
context->mark_for_destruction(this);
```

## Implementation Guide

### Step 1: Initialize GameContext
```cpp
// In ClanBomberApplication constructor, after all systems are created
void ClanBomberApplication::initialize_game_context() {
    if (gpu_renderer && text_renderer) {  // Wait for all systems
        game_context = new GameContext(
            lifecycle_manager,
            tile_manager,
            particle_effects,
            map,
            gpu_renderer,
            text_renderer
        );
        SDL_Log("GameContext initialized successfully");
    }
}
```

### Step 2: Update Object Constructors
```cpp
// GameObject base class
class GameObject {
protected:
    GameContext* context;  // Replace ClanBomberApplication* app
    
public:
    GameObject(int x, int y, GameContext* ctx) 
        : x(x), y(y), context(ctx) {}
};
```

### Step 3: Migrate API Calls
```cpp
// Find and replace patterns:
// app->map->get_tile(x, y)->is_blocking()
// → context->is_position_blocked(x, y)

// app->particle_effects->create_*_effect()
// → context->request_destruction_effect()

// app->lifecycle_manager->mark_for_destruction()
// → context->mark_for_destruction()
```

## Testing Benefits

### Easy Mocking
```cpp
class MockGameContext : public GameContext {
    // Override only what you need for the test
    bool is_position_blocked(int x, int y) const override {
        return blocked_positions.count({x, y}) > 0;
    }
    
    void mark_for_destruction(GameObject* obj) const override {
        destroyed_objects.push_back(obj);
    }
    
    std::set<std::pair<int, int>> blocked_positions;
    mutable std::vector<GameObject*> destroyed_objects;
};

TEST(BomberMovement, HandleWallCollision) {
    MockGameContext mock_context;
    mock_context.blocked_positions.insert({5, 5});
    
    Bomber bomber(200, 200, &mock_context);  // (5,5) in map coords
    
    bool moved = bomber.try_move_right();
    EXPECT_FALSE(moved);  // Should be blocked by wall
}
```

### Isolated Component Testing
```cpp
TEST(TileQueries, BlockedPositions) {
    MockTileManager mock_tiles;
    mock_tiles.set_blocked({3, 3});
    
    GameContext context(nullptr, &mock_tiles, nullptr, nullptr, nullptr, nullptr);
    
    EXPECT_TRUE(context.is_position_blocked(3, 3));
    EXPECT_FALSE(context.is_position_blocked(4, 4));
}
```

## Migration Strategy

### Phase 1: Add GameContext Support (Non-Breaking)
```cpp
class GameObject {
    ClanBomberApplication* app;     // Keep for backwards compatibility
    GameContext* context;          // Add new dependency injection
    
    // Support both constructors during transition
    GameObject(int x, int y, ClanBomberApplication* app);
    GameObject(int x, int y, GameContext* context);
};
```

### Phase 2: Prefer GameContext (Gradual Migration)
```cpp
// Convert objects one by one
// Use deprecated warnings to guide migration
[[deprecated("Use GameContext constructor instead")]]
GameObject(int x, int y, ClanBomberApplication* app);
```

### Phase 3: Remove Legacy Support
```cpp
class GameObject {
    GameContext* context;  // Only dependency injection
    
    GameObject(int x, int y, GameContext* context);
    // Old constructor removed
};
```

## Performance Considerations

### Memory Overhead
- **GameContext**: 6 pointers (48 bytes on 64-bit)
- **Previous**: 1 pointer to ClanBomberApplication
- **Net cost**: ~40 bytes per object
- **Benefit**: Better cache locality, reduced indirection

### CPU Overhead
- **Convenience methods**: Inline, zero overhead
- **System access**: Direct pointer dereference
- **Previous**: Multiple pointer chases through app->system->method

### Compilation
- **Include dependencies**: Reduced (forward declarations)
- **Compilation time**: Improved (less coupling)
- **Template instantiation**: Reduced complexity

## Common Patterns

### Prefer Convenience Methods
```cpp
// GOOD: High-level, intention-revealing
if (context->is_position_blocked(x, y)) {
    handle_collision();
}

// AVOID: Low-level, implementation-dependent
if (context->get_tile_manager()->is_tile_blocking_at(x, y)) {
    handle_collision();
}
```

### Null Safety
```cpp
// Always check context validity in critical paths
void GameObject::update() {
    if (!context) {
        SDL_Log("WARNING: GameObject has null context");
        return;
    }
    
    // Safe to use context methods
    if (context->is_position_blocked(get_map_x(), get_map_y())) {
        // Handle collision
    }
}
```

### System Extension
```cpp
// Adding new convenience methods
class GameContext {
    // Add new high-level operations as needed
    bool can_place_bomb_at(int map_x, int map_y) const {
        return is_position_walkable(map_x, map_y) && 
               !has_bomb_at(map_x, map_y);
    }
    
    void request_bomber_spawn(int map_x, int map_y, int team) const {
        // Coordinate with multiple systems
        if (can_place_bomb_at(map_x, map_y)) {
            // Request spawn through appropriate systems
        }
    }
};
```

## Troubleshooting

### Error: Context is nullptr
```cpp
// CAUSE: GameContext not initialized or object created before initialization
// SOLUTION: Check initialization order
if (!game_context) {
    SDL_Log("ERROR: game_context not initialized");
    return;
}

// Or defensive programming
if (context && context->is_position_blocked(x, y)) {
    // Safe operation
}
```

### Error: Circular dependencies
```cpp
// CAUSE: GameContext trying to include heavy headers
// SOLUTION: Use forward declarations in .h, includes in .cpp

// GameContext.h - Forward declarations only
class TileManager;
class ParticleEffectsManager;

// GameContext.cpp - Full includes
#include "TileManager.h"
#include "ParticleEffectsManager.h"
```

### Performance regression
```cpp
// CAUSE: Too many convenience method calls in hot loops
// SOLUTION: Cache system references in tight loops

void update_many_objects() {
    TileManager* tiles = context->get_tile_manager();  // Cache reference
    
    for (int i = 0; i < 1000; ++i) {
        if (tiles->is_tile_blocking_at(x[i], y[i])) {  // Direct access
            // Process collision
        }
    }
}
```

GameContext provides a clean, testable, and maintainable foundation for object dependencies in ClanBomber SDL3.