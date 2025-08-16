# GameSystems Architecture Guide

## Overview

GameSystems extrae las responsabilidades monolíticas de `GameplayScreen::act_all()` en sistemas especializados que manejan aspectos específicos del juego. Esto mejora la mantenibilidad, debugging y performance.

## Problem Solved

### Before (God Method)
```cpp
void GameplayScreen::act_all() {
    float deltaTime = Timer::time_elapsed();
    
    // Input processing for all controllers
    for (auto& bomber : app->bomber_objects) {
        bomber->controller->update();
    }
    
    // Physics updates for all objects
    for (auto& obj : app->objects) {
        obj->update_position(deltaTime);
    }
    
    // AI decision making
    for (auto& bomber : app->bomber_objects) {
        if (bomber->is_ai_controlled()) {
            bomber->make_ai_decision();
        }
    }
    
    // Collision detection
    for (auto& bomber : app->bomber_objects) {
        for (auto& bomb : app->objects) {
            if (check_collision(bomber, bomb)) {
                handle_collision(bomber, bomb);
            }
        }
    }
    
    // Animation updates
    for (auto& obj : app->objects) {
        obj->update_animation(deltaTime);
    }
    
    // Lifecycle management
    for (auto& obj : app->objects) {
        if (obj->should_be_destroyed()) {
            mark_for_destruction(obj);
        }
    }
    
    // ... 200+ lines of mixed responsibilities
}
```

### After (Specialized Systems)
```cpp
void GameSystems::update_all_systems(float deltaTime) {
    // Clear separation of concerns
    update_input_system(deltaTime);      // Only input processing
    update_ai_system(deltaTime);         // Only AI decisions  
    update_physics_system(deltaTime);    // Only movement/physics
    update_collision_system(deltaTime);  // Only collision detection
    update_animation_system(deltaTime);  // Only visual updates
    update_lifecycle_system(deltaTime);  // Only object lifecycle
}

void GameSystems::render_all_systems() {
    render_world();     // World geometry and tiles
    render_objects();   // Game objects (bombers, bombs, etc.)
    render_effects();   // Particle effects and explosions
    render_ui();        // HUD, menus, overlays
}
```

## System Architecture

### Core Systems

#### 1. Input System
**Responsibility**: Process controller input and generate input events
```cpp
class InputSystem {
    void update(float deltaTime) {
        for (auto& controller : controllers) {
            controller->update();
            
            if (controller->is_bomb_pressed()) {
                emit_event(BombPlaceEvent{controller->get_bomber_id()});
            }
            
            if (controller->has_movement()) {
                Vector2 direction = controller->get_movement_direction();
                emit_event(MoveEvent{controller->get_bomber_id(), direction});
            }
        }
    }
};
```

#### 2. AI System
**Responsibility**: AI decision making and behavior updates
```cpp
class AISystem {
    void update(float deltaTime) {
        for (auto& bomber : bombers) {
            if (bomber->is_ai_controlled()) {
                bomber->controller->update_ai_logic(deltaTime);
                
                // AI systems can be further specialized:
                // - PathfindingSystem
                // - ThreatAssessmentSystem  
                // - StrategySystem
            }
        }
    }
};
```

#### 3. Physics System
**Responsibility**: Movement, collisions, and physics simulation
```cpp
class PhysicsSystem {
    void update(float deltaTime) {
        // Update positions
        for (auto& obj : objects) {
            obj->update_physics(deltaTime);
        }
        
        // Broad-phase collision detection
        auto collision_pairs = spatial_hash.find_potential_collisions();
        
        // Narrow-phase collision resolution
        for (auto& pair : collision_pairs) {
            if (check_detailed_collision(pair.first, pair.second)) {
                resolve_collision(pair.first, pair.second);
            }
        }
    }
    
private:
    SpatialHash spatial_hash;  // Optimization for collision detection
};
```

#### 4. Animation System
**Responsibility**: Visual state and animation updates
```cpp
class AnimationSystem {
    void update(float deltaTime) {
        for (auto& obj : objects) {
            obj->update_animation(deltaTime);
        }
        
        // Update particle systems
        for (auto& effect : particle_effects) {
            effect->update(deltaTime);
        }
    }
};
```

#### 5. Rendering System
**Responsibility**: Drawing objects and effects to screen
```cpp
class RenderingSystem {
    void render() {
        // Render in layers for proper z-ordering
        render_background();
        render_tiles();
        render_objects();
        render_effects();
        render_ui();
    }
    
private:
    void render_objects() {
        // Sort by z-order for correct rendering
        std::sort(objects.begin(), objects.end(), 
                 [](const auto& a, const auto& b) {
                     return a->get_z_order() < b->get_z_order();
                 });
        
        for (const auto& obj : objects) {
            obj->render();
        }
    }
};
```

## Implementation Strategy

### Phase 1: Extract Core Logic (Current)
```cpp
class GameSystems {
    void update_all_systems(float deltaTime) {
        // Delta time management (extracted from GameplayScreen)
        deltaTime = clamp_and_smooth_delta(deltaTime);
        
        // Forward to existing GameObject::act() methods
        update_physics_system(deltaTime);
        update_ai_system(deltaTime);
        
        // Placeholder for future systems
        update_input_system(deltaTime);      // TODO
        update_collision_system(deltaTime);  // TODO
        update_animation_system(deltaTime);  // TODO
    }
    
private:
    void update_physics_system(float deltaTime) {
        // Move logic from GameplayScreen::act_all()
        for (auto& obj : *objects_ref) {
            if (obj && !obj->delete_me) {
                obj->act(deltaTime);
            }
        }
        
        for (auto& bomber : *bombers_ref) {
            if (bomber && !bomber->delete_me) {
                bomber->act(deltaTime);
            }
        }
    }
};
```

### Phase 2: Specialized System Implementation
```cpp
class GameSystems {
    void update_all_systems(float deltaTime) {
        // Event-driven system updates
        input_system.update(deltaTime);
        
        // Process input events
        auto events = input_system.get_events();
        ai_system.process_events(events, deltaTime);
        physics_system.process_events(events, deltaTime);
        
        // Update systems in dependency order
        physics_system.update(deltaTime);
        collision_system.update(deltaTime);
        animation_system.update(deltaTime);
    }
    
private:
    InputSystem input_system;
    AISystem ai_system;
    PhysicsSystem physics_system;
    CollisionSystem collision_system;
    AnimationSystem animation_system;
};
```

### Phase 3: Component System (Future)
```cpp
// Entity-Component-System architecture
class Bomber {
    EntityID id;
    // No more monolithic class with mixed responsibilities
};

// Components are pure data
struct PositionComponent { float x, y; };
struct VelocityComponent { float dx, dy; };
struct HealthComponent { int health, max_health; };
struct InputComponent { Controller* controller; };
struct AIComponent { AIBehavior* behavior; };

// Systems operate on components
class MovementSystem {
    void update(float deltaTime) {
        auto entities = registry.view<PositionComponent, VelocityComponent>();
        for (auto entity : entities) {
            auto& pos = entities.get<PositionComponent>(entity);
            auto& vel = entities.get<VelocityComponent>(entity);
            
            pos.x += vel.dx * deltaTime;
            pos.y += vel.dy * deltaTime;
        }
    }
};
```

## Usage Patterns

### System Registration
```cpp
class GameplayScreen {
    void init_game() {
        game_systems = new GameSystems(game_context);
        
        // Set up references to object lists
        game_systems->set_object_references(&app->objects, &app->bomber_objects);
        
        // Initialize specialized systems
        game_systems->init_all_systems();
    }
};
```

### Update Loop
```cpp
class GameplayScreen {
    void update(float deltaTime) {
        // Replace monolithic act_all() with modular system updates
        app->tile_manager->update_tiles(deltaTime);
        app->lifecycle_manager->update_states(deltaTime);
        
        game_systems->update_all_systems(deltaTime);
        
        check_victory_conditions();
    }
    
    void render(SDL_Renderer* renderer) {
        // Replace monolithic show_all() with modular rendering
        game_systems->render_all_systems();
    }
};
```

### System Communication
```cpp
// Event-based communication between systems
class Event {
    virtual ~Event() = default;
};

class BombPlacedEvent : public Event {
    int bomber_id;
    int map_x, map_y;
    int power;
};

class EventBus {
    void publish(std::unique_ptr<Event> event) {
        for (auto& subscriber : subscribers) {
            subscriber->handle_event(*event);
        }
    }
    
    void subscribe(EventSubscriber* subscriber) {
        subscribers.push_back(subscriber);
    }
    
private:
    std::vector<EventSubscriber*> subscribers;
};
```

## Performance Benefits

### Optimization Opportunities
```cpp
class PhysicsSystem {
    void update(float deltaTime) {
        // SIMD optimization for position updates
        update_positions_simd(deltaTime);
        
        // Spatial partitioning for efficient collision detection
        spatial_hash.update();
        
        // Multi-threading for independent objects
        std::for_each(std::execution::par_unseq, 
                     objects.begin(), objects.end(),
                     [deltaTime](auto& obj) {
                         obj->update_physics(deltaTime);
                     });
    }
};
```

### Memory Locality
```cpp
// Data-oriented design for better cache performance
class RenderingSystem {
    // Structure of arrays instead of array of structures
    std::vector<float> positions_x, positions_y;
    std::vector<GLuint> textures;
    std::vector<float> alphas;
    
    void render_sprites() {
        // Process arrays in chunks for optimal cache usage
        for (size_t i = 0; i < positions_x.size(); i += BATCH_SIZE) {
            render_sprite_batch(i, std::min(i + BATCH_SIZE, positions_x.size()));
        }
    }
};
```

## Debugging Benefits

### System Isolation
```cpp
class GameSystems {
    void update_all_systems(float deltaTime) {
        try {
            update_input_system(deltaTime);
        } catch (const std::exception& e) {
            SDL_Log("ERROR in InputSystem: %s", e.what());
            // Game can continue with other systems
        }
        
        try {
            update_physics_system(deltaTime);
        } catch (const std::exception& e) {
            SDL_Log("ERROR in PhysicsSystem: %s", e.what());
            // Isolated error - doesn't affect other systems
        }
    }
};
```

### Performance Profiling
```cpp
class GameSystems {
    void update_all_systems(float deltaTime) {
        PROFILE_SCOPE("GameSystems::update_all_systems");
        
        {
            PROFILE_SCOPE("InputSystem");
            update_input_system(deltaTime);
        }
        
        {
            PROFILE_SCOPE("PhysicsSystem");
            update_physics_system(deltaTime);
        }
        
        // Easy to identify performance bottlenecks per system
    }
};
```

### System State Inspection
```cpp
class DebugUI {
    void render_system_debug_info() {
        if (ImGui::Begin("Game Systems Debug")) {
            if (ImGui::CollapsingHeader("Physics System")) {
                ImGui::Text("Active Objects: %zu", physics_system.get_object_count());
                ImGui::Text("Collision Pairs: %zu", physics_system.get_collision_count());
                ImGui::Text("Update Time: %.3f ms", physics_system.get_last_update_time());
            }
            
            if (ImGui::CollapsingHeader("AI System")) {
                for (auto& bomber : ai_system.get_ai_bombers()) {
                    ImGui::Text("Bomber %d: %s", bomber.id, bomber.current_state.c_str());
                }
            }
        }
        ImGui::End();
    }
};
```

## Testing Strategy

### System Unit Tests
```cpp
TEST(PhysicsSystem, ObjectMovement) {
    PhysicsSystem physics;
    MockGameContext context;
    
    auto obj = std::make_unique<TestObject>(100, 100, &context);
    obj->set_velocity(50, 0);  // Move right at 50 units/sec
    
    physics.add_object(obj.get());
    physics.update(0.1f);  // 100ms update
    
    EXPECT_FLOAT_EQ(obj->get_x(), 105.0f);  // 100 + 50*0.1
    EXPECT_FLOAT_EQ(obj->get_y(), 100.0f);  // No vertical movement
}

TEST(CollisionSystem, BomberWallCollision) {
    CollisionSystem collision;
    MockGameContext context;
    context.set_blocked_position(5, 5);
    
    auto bomber = std::make_unique<TestBomber>(200, 200, &context);  // (5,5) in map coords
    bomber->set_velocity(100, 0);  // Moving right toward wall
    
    collision.add_object(bomber.get());
    collision.update(0.1f);
    
    EXPECT_FLOAT_EQ(bomber->get_velocity_x(), 0.0f);  // Stopped by wall
}
```

### Integration Tests
```cpp
TEST(GameSystems, FullUpdateCycle) {
    MockGameContext context;
    GameSystems systems(&context);
    
    // Set up test scenario
    auto bomber = create_test_bomber(100, 100);
    auto bomb = create_test_bomb(140, 100);
    
    systems.register_object(bomber.get());
    systems.register_object(bomb.get());
    
    // Simulate one frame
    systems.update_all_systems(0.016f);  // ~60 FPS
    
    // Verify system interactions
    EXPECT_TRUE(bomber->is_affected_by_explosion());
    EXPECT_EQ(bomber->get_health(), 90);  // Damaged by bomb
}
```

## Migration Checklist

### ✅ Phase 1: Foundation (Completed)
- [x] Create GameSystems class skeleton
- [x] Extract delta time management from GameplayScreen  
- [x] Move basic object iteration to GameSystems
- [x] Add system placeholders for future implementation

### 🔄 Phase 2: Core Systems
- [ ] Implement InputSystem with event generation
- [ ] Extract AI logic from Bomber classes to AISystem
- [ ] Implement PhysicsSystem with proper collision detection
- [ ] Create AnimationSystem for visual state management
- [ ] Implement RenderingSystem with proper z-ordering

### 📋 Phase 3: Advanced Features  
- [ ] Add event bus for system communication
- [ ] Implement spatial partitioning for performance
- [ ] Add debug UI for system inspection
- [ ] Create component system foundation
- [ ] Add multi-threading support for parallel systems

GameSystems provides the foundation for scalable, maintainable, and high-performance game logic in ClanBomber SDL3.