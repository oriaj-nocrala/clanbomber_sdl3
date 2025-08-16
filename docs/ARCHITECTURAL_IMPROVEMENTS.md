# Architectural Improvements Guide

Este documento describe las mejoras arquitectónicas implementadas en ClanBomber SDL3 para mejorar la mantenibilidad, testabilidad y calidad de vida del desarrollador.

## Resumen Ejecutivo

Se han implementado mejoras críticas para resolver problemas de responsabilidades que chocan, acoplamiento fuerte y god objects. Las mejoras incluyen:

1. **GameContext**: Dependency injection container
2. **GameSystems**: Extracción de responsabilidades de GameplayScreen
3. **ParticleEffectsManager**: Sistema centralizado de efectos
4. **TileManager Optimization**: Centralización de dual architecture lookups
5. **Legacy API Deprecation**: Guía hacia nuevas APIs

## 1. GameContext - Dependency Injection

### Problema Original
```cpp
// ANTES: Acoplamiento fuerte
GameObject(x, y, ClanBomberApplication* app)  // Conoce toda la aplicación
app->map->get_tile(x, y)                     // Acceso directo a implementación
```

### Solución
```cpp
// DESPUÉS: Dependencias explícitas
GameObject(x, y, GameContext* context)       // Solo ve las interfaces que necesita
context->is_position_blocked(x, y)          // API limpia y abstracta
```

### Beneficios
- **Testing**: Fácil mock de dependencias
- **Modularidad**: Clases más enfocadas
- **Mantenibilidad**: Cambios localizados

### Uso
```cpp
// Inicialización (típicamente en ClanBomberApplication)
GameContext* context = new GameContext(
    lifecycle_manager,
    tile_manager, 
    particle_effects,
    map,
    gpu_renderer,
    text_renderer
);

// Uso en objetos
if (context->is_position_blocked(map_x, map_y)) {
    // Handle collision
}
```

## 2. GameSystems - Separation of Concerns

### Problema Original
```cpp
// GameplayScreen::act_all() - God method
void GameplayScreen::act_all() {
    // Input processing
    // Physics simulation
    // AI updates
    // Animation
    // Collision detection
    // Object lifecycle
    // ... 200+ líneas de responsabilidades mezcladas
}
```

### Solución
```cpp
// Sistemas especializados
void GameSystems::update_all_systems(float deltaTime) {
    update_input_system(deltaTime);      // Solo input
    update_ai_system(deltaTime);         // Solo AI
    update_physics_system(deltaTime);    // Solo physics
    update_collision_system(deltaTime);  // Solo collisions
    update_animation_system(deltaTime);  // Solo animations
}
```

### Beneficios
- **Debugging**: Error aislado a sistema específico
- **Performance**: Optimización por sistema
- **Extensibilidad**: Nuevos sistemas sin tocar existentes

### Implementación Futura
```cpp
class PhysicsSystem {
    void update(float deltaTime, std::list<GameObject*>& objects) {
        for (auto& obj : objects) {
            obj->update_physics(deltaTime);
        }
    }
};
```

## 3. ParticleEffectsManager - Centralized Effects

### Problema Original
```cpp
// MapTile_Box::show() - 189 líneas de lógica de efectos
void MapTile_Box::show() {
    // Física compleja hardcodeada
    // Renderizado GPU acoplado
    // Lógica de fragmentación no reutilizable
}
```

### Solución
```cpp
// Sistema centralizado y reutilizable
class ParticleEffectsManager {
    void create_box_destruction_effect(float x, float y, float intensity);
    void create_explosion_effect(float x, float y, float intensity);
};

// Uso simple
app->particle_effects->create_box_destruction_effect(x, y, 1.0f);
```

### Beneficios
- **Reusabilidad**: Efectos compartidos entre objetos
- **Performance**: Batching optimizado
- **Mantenibilidad**: Un lugar para cambiar efectos

## 4. TileManager Optimization

### Problema Original
```cpp
// Controller_AI_Modern.cpp - Código duplicado
bool is_tile_blocking_at(Map* map, int map_x, int map_y) {
    MapTile* legacy_tile = map->get_tile(map_x, map_y);
    if (legacy_tile) return legacy_tile->is_blocking();
    
    TileEntity* tile_entity = map->get_tile_entity(map_x, map_y);
    if (tile_entity) return tile_entity->is_blocking();
    
    return false;
}
// Misma función en Controller_AI_Smart.cpp - DRY violation
```

### Solución
```cpp
// TileManager - Single source of truth
class TileManager {
    bool is_tile_blocking_at(int map_x, int map_y) {
        // Dual architecture logic centralizada
        // Una sola implementación para toda la aplicación
    }
};

// Uso en AI controllers
if (bomber->app->tile_manager->is_tile_blocking_at(x, y)) {
    // Handle collision
}
```

### Beneficios
- **DRY**: Eliminadas 100+ líneas duplicadas
- **Consistency**: Misma lógica en toda la aplicación
- **Migration Path**: Fácil eliminar legacy cuando esté listo

## 5. Legacy API Deprecation

### Implementación
```cpp
[[deprecated("Use GameContext::is_position_blocked() or TileManager instead")]]
MapTile* get_tile() const;
```

### Beneficios
- **Developer Guidance**: Warnings claros hacia nuevas APIs
- **Gradual Migration**: No breaking changes inmediatos
- **Code Discovery**: Fácil encontrar código que necesita migración

## Roadmap de Implementación

### ✅ Fase 1: Foundation (Completada)
- [x] TileManager dual architecture optimization
- [x] ParticleEffectsManager basic implementation
- [x] GameContext dependency container
- [x] GameSystems skeleton
- [x] Legacy API deprecation warnings

### 🔄 Fase 2: Integration (En Progreso)
- [ ] GameContext initialization in ClanBomberApplication
- [ ] GameSystems implementation with concrete phases
- [ ] GameObject refactoring to use GameContext
- [ ] Bomber component extraction

### 📋 Fase 3: Completion
- [ ] Complete legacy architecture removal
- [ ] Performance optimization
- [ ] Component system for complex objects
- [ ] Complete test coverage

## Testing Strategy

### Unit Testing
```cpp
// GameContext permite easy mocking
class MockTileManager : public ITileManager {
    bool is_tile_blocking_at(int x, int y) override {
        return test_blocked_positions.count({x, y}) > 0;
    }
};

TEST(BomberMovement, BlockedByWall) {
    MockTileManager mock_tiles;
    mock_tiles.set_blocked({5, 5});
    
    GameContext context(&lifecycle, &mock_tiles, /*...*/);
    Bomber bomber(100, 100, &context);
    
    // Test movement blocked by wall
    EXPECT_FALSE(bomber.move_to(200, 200)); // (5,5) in map coordinates
}
```

### Integration Testing
```cpp
TEST(EffectsSystem, BoxDestructionCreatesFragments) {
    ParticleEffectsManager effects(app);
    effects.create_box_destruction_effect(100, 100, 1.0f);
    effects.update(0.016f);
    
    // Verify fragments were created
    EXPECT_GT(gpu_renderer->get_sprite_count(), 10);
}
```

## Performance Considerations

### Memory
- **GameContext**: Overhead mínimo (solo punteros)
- **GameSystems**: Cero overhead vs implementación actual
- **ParticleEffectsManager**: Reusa memoria vs creación ad-hoc

### CPU
- **TileManager**: Elimina lookups duplicados
- **GameSystems**: Permite optimizaciones por sistema (SIMD, threading)
- **Deprecation warnings**: Solo compile-time, cero runtime overhead

## Migration Guide

### Para Desarrolladores
```cpp
// OLD - Avoid
if (app->map->get_tile(x, y)->is_blocking()) { }

// NEW - Prefer  
if (context->is_position_blocked(map_x, map_y)) { }

// OLD - Avoid
new Bomb(x, y, power, owner, this->app);

// NEW - Prefer
new Bomb(x, y, power, owner, this->context);
```

### Compiler Warnings
Los warnings de deprecated APIs guían automáticamente hacia las nuevas implementaciones:
```
warning: 'MapTile* GameObject::get_tile() const' is deprecated: 
Use GameContext::is_position_blocked() or TileManager instead
```

## Troubleshooting

### Error: GameContext no inicializado
```cpp
// PROBLEMA: game_context es nullptr
if (game_context) {  // Always check before use
    game_context->is_position_blocked(x, y);
}

// SOLUCIÓN: Inicializar después de gpu_renderer
app->game_context = new GameContext(/*all systems*/);
```

### Error: Sistemas no encuentran objetos
```cpp
// PROBLEMA: GameSystems no tiene referencia a object lists
systems->set_object_lists(&app->objects, &app->bomber_objects);
```

### Performance degradation
```cpp
// PROBLEMA: Múltiples sistemas iterando sobre mismos objetos
// SOLUCIÓN: Shared iteration con visitor pattern
for (auto& obj : objects) {
    physics_system.visit(obj, deltaTime);
    animation_system.visit(obj, deltaTime);
    collision_system.visit(obj, deltaTime);
}
```

## Métricas de Éxito

### Code Quality
- **Lines of Code**: -100+ líneas eliminadas (duplicación)
- **Cyclomatic Complexity**: Reducida de GameplayScreen
- **Coupling**: Reducido con dependency injection

### Developer Experience
- **Build Time**: Sin cambio (warnings solo compile-time)
- **Debug Time**: Mejorado con sistemas aislados
- **Testing Time**: Mejorado con mocking fácil

### Maintainability
- **Change Impact**: Localizado por sistema
- **Feature Addition**: Sin modificar GameplayScreen
- **Bug Fix**: Scope aislado por sistema

Esta arquitectura establece una base sólida para el desarrollo futuro de ClanBomber SDL3.