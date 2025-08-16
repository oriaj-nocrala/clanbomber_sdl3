# Migration Guide - Step by Step Implementation

Esta guía proporciona los pasos exactos para completar la migración arquitectónica de ClanBomber SDL3.

## Status Actual

### ✅ Completado
- [x] TileManager dual architecture optimization
- [x] ParticleEffectsManager centralizado
- [x] GameContext dependency injection container
- [x] GameSystems foundation skeleton
- [x] Legacy API deprecation warnings
- [x] Documentación completa

### 🔄 En Progreso
Los siguientes pasos deben completarse en orden:

## Paso 1: GameContext Integration

### 1.1 Inicializar GameContext en el momento correcto

**Archivo**: `src/ClanBomber.cpp`

```cpp
// Encontrar donde se inicializa gpu_renderer y text_renderer
// Agregar después de esa inicialización:

void ClanBomberApplication::initialize_game_context() {
    if (gpu_renderer && text_renderer && lifecycle_manager && 
        tile_manager && particle_effects && map) {
        
        game_context = new GameContext(
            lifecycle_manager,
            tile_manager,
            particle_effects,
            map,
            gpu_renderer,
            text_renderer
        );
        
        SDL_Log("GameContext initialized successfully");
    } else {
        SDL_Log("ERROR: Cannot initialize GameContext - missing dependencies");
    }
}

// Llamar esta función después de que todos los sistemas estén listos
// Típicamente en init_server_game() o init_client_game()
```

### 1.2 Actualizar GameObject para usar GameContext

**Archivo**: `src/GameObject.h`

```cpp
class GameObject {
protected:
    // TRANSITION SUPPORT: Both old and new dependencies
    ClanBomberApplication* app;  // Legacy - will be removed
    GameContext* context;       // New - preferred
    
public:
    // Support both constructors during migration
    GameObject(int x, int y, ClanBomberApplication* app);
    
    // NEW: Preferred constructor
    GameObject(int x, int y, GameContext* context);
    
    // Convenience method to get context from either source
    GameContext* get_context() const;
};
```

**Archivo**: `src/GameObject.cpp`

```cpp
// Add new constructor
GameObject::GameObject(int x, int y, GameContext* context) 
    : x(x), y(y), context(context), app(nullptr) {
    // Initialize with GameContext
}

// Update existing constructor to create temporary context
GameObject::GameObject(int x, int y, ClanBomberApplication* app)
    : x(x), y(y), app(app), context(app ? app->game_context : nullptr) {
    // Fallback to legacy behavior
}

GameContext* GameObject::get_context() const {
    if (context) return context;
    if (app && app->game_context) return app->game_context;
    return nullptr;
}
```

### 1.3 Implementar convenience methods completamente

**Archivo**: `src/GameObject.cpp`

```cpp
bool GameObject::is_tile_blocking_at(int pixel_x, int pixel_y) const {
    GameContext* ctx = get_context();
    if (!ctx) return false;
    
    int map_x = pixel_x / 40;
    int map_y = pixel_y / 40;
    return ctx->is_position_blocked(map_x, map_y);
}

bool GameObject::has_bomb_at(int pixel_x, int pixel_y) const {
    GameContext* ctx = get_context();
    if (!ctx) return false;
    
    int map_x = pixel_x / 40;
    int map_y = pixel_y / 40;
    return ctx->has_bomb_at(map_x, map_y);
}

void GameObject::set_bomb_on_tile(Bomb* bomb) const {
    GameContext* ctx = get_context();
    if (!ctx) return;
    
    // Coordinate with both architectures through TileManager
    TileManager* tiles = ctx->get_tile_manager();
    if (tiles) {
        tiles->register_bomb_at(get_center_map_x(), get_center_map_y(), bomb);
    }
}
```

## Paso 2: GameSystems Implementation

### 2.1 Conectar GameSystems con object lists

**Archivo**: `src/GameSystems.h`

```cpp
class GameSystems {
public:
    // Add setup method
    void set_object_references(std::list<GameObject*>* objects, 
                              std::list<Bomber*>* bombers);
    
    // Initialize all systems
    void init_all_systems();
    
private:
    // Store references (not ownership)
    std::list<GameObject*>* objects_ref;
    std::list<Bomber*>* bombers_ref;
    
    // System state
    bool systems_initialized = false;
};
```

**Archivo**: `src/GameSystems.cpp`

```cpp
void GameSystems::set_object_references(std::list<GameObject*>* objects, 
                                        std::list<Bomber*>* bombers) {
    objects_ref = objects;
    bombers_ref = bombers;
    SDL_Log("GameSystems: Object references set successfully");
}

void GameSystems::init_all_systems() {
    if (!context) {
        SDL_Log("ERROR: GameSystems cannot initialize without GameContext");
        return;
    }
    
    if (!objects_ref || !bombers_ref) {
        SDL_Log("ERROR: GameSystems cannot initialize without object references");
        return;
    }
    
    // Initialize individual systems here
    systems_initialized = true;
    SDL_Log("GameSystems: All systems initialized successfully");
}
```

### 2.2 Integrar GameSystems en GameplayScreen

**Archivo**: `src/GameplayScreen.h`

```cpp
class GameplayScreen : public Screen {
private:
    // Add GameSystems
    class GameSystems* game_systems;
    
    // Keep existing methods during transition
    void act_all();
    void show_all();
};
```

**Archivo**: `src/GameplayScreen.cpp`

```cpp
GameplayScreen::GameplayScreen(ClanBomberApplication* app) 
    : Screen(app), game_systems(nullptr) {
    // Existing initialization
}

void GameplayScreen::init_game() {
    // Existing initialization...
    
    // Initialize GameSystems after GameContext is ready
    if (app->game_context) {
        game_systems = new GameSystems(app->game_context);
        game_systems->set_object_references(&app->objects, &app->bomber_objects);
        game_systems->init_all_systems();
        SDL_Log("GameSystems initialized in GameplayScreen");
    } else {
        SDL_Log("WARNING: GameContext not available, using legacy act_all()");
    }
}

void GameplayScreen::update(float deltaTime) {
    // ... existing code ...
    
    // Use GameSystems if available, fallback to legacy
    if (game_systems) {
        game_systems->update_all_systems(deltaTime);
    } else {
        act_all();  // Legacy fallback
    }
    
    // ... rest of update logic ...
}

GameplayScreen::~GameplayScreen() {
    if (game_systems) {
        delete game_systems;
        game_systems = nullptr;
    }
    
    // Existing cleanup...
}
```

## Paso 3: Refactor Bomber Components

### 3.1 Identify Bomber responsibilities

**Current Bomber class (117 lines) handles:**
- Health and lives (`health`, `remaining_lives`, `dead`, `respawning`)
- Power-ups (`power`, `max_bombs`, `can_kick`, `can_throw`)
- Movement and physics (`speed`, `flying`, `direction`)
- Input processing (`controller`)
- Animation state (`sprite_nr`, `current_surface`)
- Game logic (`invincible`, `respawn_timer`)

### 3.2 Create Component interfaces

**Archivo**: `src/components/BomberComponents.h`

```cpp
#ifndef BOMBER_COMPONENTS_H
#define BOMBER_COMPONENTS_H

// Base component interface
class BomberComponent {
public:
    virtual ~BomberComponent() = default;
    virtual void update(float deltaTime) = 0;
    virtual void reset() {}
};

// Health and lifecycle management
class HealthComponent : public BomberComponent {
public:
    HealthComponent(int max_health, int max_lives);
    
    void update(float deltaTime) override;
    void take_damage(int damage);
    void heal(int amount);
    bool is_dead() const { return health <= 0; }
    bool is_respawning() const { return respawning; }
    
    int get_health() const { return health; }
    int get_remaining_lives() const { return remaining_lives; }
    
private:
    int health, max_health;
    int remaining_lives;
    bool dead = false;
    bool respawning = false;
    bool invincible = false;
    float respawn_timer = 0.0f;
    float invincible_timer = 0.0f;
};

// Power-up management
class PowerUpComponent : public BomberComponent {
public:
    PowerUpComponent();
    
    void update(float deltaTime) override;
    void add_bomb_capacity(int amount) { max_bombs += amount; }
    void add_bomb_power(int amount) { power += amount; }
    void enable_kick() { can_kick = true; }
    void enable_throw() { can_throw = true; }
    
    int get_power() const { return power; }
    int get_max_bombs() const { return max_bombs; }
    bool can_kick_bombs() const { return can_kick; }
    bool can_throw_bombs() const { return can_throw; }
    
private:
    int power = 1;
    int max_bombs = 1;
    int current_bombs = 0;
    bool can_kick = false;
    bool can_throw = false;
};

// Input processing
class InputComponent : public BomberComponent {
public:
    InputComponent(class Controller* controller);
    
    void update(float deltaTime) override;
    void set_controller(Controller* ctrl) { controller = ctrl; }
    
    bool wants_move_left() const;
    bool wants_move_right() const;
    bool wants_move_up() const;
    bool wants_move_down() const;
    bool wants_place_bomb() const;
    
private:
    Controller* controller;
    // Input state and timing
};
```

### 3.3 Refactor Bomber to use components

**Archivo**: `src/Bomber.h`

```cpp
class Bomber : public GameObject {
public:
    // Simplified constructor
    Bomber(int x, int y, GameContext* context, int bomber_id);
    
    // Component access
    HealthComponent* get_health() { return health_component.get(); }
    PowerUpComponent* get_powerups() { return powerup_component.get(); }
    InputComponent* get_input() { return input_component.get(); }
    
    // Simplified interface
    void update(float deltaTime) override;
    void render() override;
    
    // Essential bomber-specific methods only
    void set_controller(Controller* controller);
    bool can_place_bomb() const;
    void place_bomb();
    
private:
    int bomber_id;
    
    // Components (composition over inheritance)
    std::unique_ptr<HealthComponent> health_component;
    std::unique_ptr<PowerUpComponent> powerup_component;
    std::unique_ptr<InputComponent> input_component;
    
    // Core bomber state (minimal)
    Controller* controller = nullptr;
};
```

## Paso 4: Eliminate Dual Architecture

### 4.1 Create migration utility

**Archivo**: `src/utils/ArchitectureMigration.h`

```cpp
class ArchitectureMigration {
public:
    // Check if legacy architecture is still being used
    static bool has_legacy_references(ClanBomberApplication* app);
    
    // Migrate specific systems to new architecture
    static void migrate_bomber_references(ClanBomberApplication* app);
    static void migrate_explosion_references(ClanBomberApplication* app);
    
    // Final cleanup - remove legacy grid
    static void remove_legacy_maptiles(ClanBomberApplication* app);
    
    // Validation
    static bool validate_migration_complete(ClanBomberApplication* app);
};
```

### 4.2 Systematic migration

**Step 4.2.1**: Find all legacy MapTile* usage
```bash
# Use these commands to find remaining legacy usage:
grep -r "->get_tile(" src/
grep -r "app->map->get_tile" src/
grep -r "MapTile\*" src/ | grep -v "// Legacy"
```

**Step 4.2.2**: Replace with TileManager calls
```cpp
// FIND:    MapTile* tile = app->map->get_tile(x, y);
//          if (tile && tile->is_blocking()) { ... }
// REPLACE: if (app->tile_manager->is_tile_blocking_at(x, y)) { ... }

// FIND:    if (tile->bomb != nullptr)
// REPLACE: if (app->tile_manager->has_bomb_at(x, y))

// FIND:    if (tile->is_destructible())
// REPLACE: if (app->tile_manager->is_tile_destructible_at(x, y))
```

### 4.3 Remove legacy grid

**Archivo**: `src/Map.h`

```cpp
class Map {
private:
    // REMOVE after migration complete:
    // MapTile* maptiles[MAP_WIDTH][MAP_HEIGHT];
    
    // KEEP:
    TileEntity* tile_entities[MAP_WIDTH][MAP_HEIGHT];
    MapTile_Pure* tile_data[MAP_WIDTH][MAP_HEIGHT];
};
```

## Paso 5: Performance Optimization

### 5.1 Add profiling support

**Archivo**: `src/utils/Profiler.h`

```cpp
class Profiler {
public:
    static void begin_frame();
    static void end_frame();
    
    static void begin_section(const char* name);
    static void end_section(const char* name);
    
    static void report_frame_stats();
    
private:
    struct ProfileSection {
        const char* name;
        float total_time;
        int call_count;
    };
    
    static std::unordered_map<std::string, ProfileSection> sections;
};

#define PROFILE_SCOPE(name) ProfilerScope _scope(name)
```

### 5.2 Optimize hot paths

**Archivo**: `src/GameSystems.cpp`

```cpp
void GameSystems::update_all_systems(float deltaTime) {
    PROFILE_SCOPE("GameSystems::update_all_systems");
    
    {
        PROFILE_SCOPE("InputSystem");
        update_input_system(deltaTime);
    }
    
    {
        PROFILE_SCOPE("PhysicsSystem");
        update_physics_system_optimized(deltaTime);
    }
    
    // ... other systems with profiling
}

void GameSystems::update_physics_system_optimized(float deltaTime) {
    // Batch process objects for better cache locality
    constexpr size_t BATCH_SIZE = 64;
    
    auto& objects = *objects_ref;
    for (size_t i = 0; i < objects.size(); i += BATCH_SIZE) {
        size_t end = std::min(i + BATCH_SIZE, objects.size());
        
        // Process batch
        auto it = objects.begin();
        std::advance(it, i);
        
        for (size_t j = i; j < end; ++j, ++it) {
            if (*it && !(*it)->delete_me) {
                (*it)->act(deltaTime);
            }
        }
    }
}
```

## Testing Strategy

### Unit Tests por cada paso

**Archivo**: `tests/test_gamecontext.cpp`

```cpp
#include <gtest/gtest.h>
#include "../src/GameContext.h"

class GameContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup mock systems
    }
    
    MockTileManager mock_tiles;
    MockLifecycleManager mock_lifecycle;
    GameContext context{&mock_lifecycle, &mock_tiles, nullptr, nullptr, nullptr, nullptr};
};

TEST_F(GameContextTest, PositionBlockedQuery) {
    mock_tiles.set_blocked({5, 5});
    
    EXPECT_TRUE(context.is_position_blocked(5, 5));
    EXPECT_FALSE(context.is_position_blocked(6, 6));
}
```

### Integration Tests

**Archivo**: `tests/test_migration.cpp`

```cpp
TEST(MigrationTest, GameSystemsReplaceActAll) {
    ClanBomberApplication app;
    app.initialize_game_context();
    
    GameSystems systems(app.game_context);
    systems.set_object_references(&app.objects, &app.bomber_objects);
    
    // Add test objects
    auto bomber = std::make_unique<Bomber>(100, 100, app.game_context, 0);
    app.bomber_objects.push_back(bomber.get());
    
    // Update through GameSystems
    float deltaTime = 0.016f;
    systems.update_all_systems(deltaTime);
    
    // Verify objects were updated
    EXPECT_TRUE(bomber->was_updated());
}
```

## Rollback Plan

Si hay problemas durante la migración:

### Rollback Level 1: Disable GameSystems
```cpp
// In GameplayScreen::update()
if (USE_LEGACY_SYSTEMS || !game_systems) {
    act_all();  // Fallback to working implementation
} else {
    game_systems->update_all_systems(deltaTime);
}
```

### Rollback Level 2: Disable GameContext
```cpp
// In GameObject constructor
GameObject::GameObject(int x, int y, ClanBomberApplication* app)
    : x(x), y(y), app(app) {
    // Don't use GameContext if there are issues
    context = ENABLE_GAME_CONTEXT ? app->game_context : nullptr;
}
```

### Rollback Level 3: Complete revert
```bash
git revert <commit-hash>  # Revert to previous working state
```

Esta migración paso a paso asegura que cada cambio pueda ser validado independientemente y que siempre tengamos un sistema funcional.