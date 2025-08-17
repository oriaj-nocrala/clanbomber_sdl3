# Nueva Arquitectura Map/MapTile/TileManager

## Resumen Ejecutivo

Esta documentación describe la refactorización completa del sistema de tiles en ClanBomber SDL3, que elimina la herencia problemática de GameObject en MapTile y establece una arquitectura modular basada en composición.

## Arquitectura Anterior vs Nueva

### Problemas de la Arquitectura Anterior
- **MapTile heredaba de GameObject**: Causaba responsabilidades mezcladas entre lógica de tiles y renderizado
- **Acoplamiento fuerte**: Map, MapTile y GameObject estaban altamente acoplados
- **Coordination crossing**: Responsabilidades dispersas entre múltiples clases

### Nueva Arquitectura: Composición sobre Herencia

```
MapTile_Pure (datos puros)
    ↓ (composition)
TileEntity (GameObject wrapper)
    ↓ (managed by)
LifecycleManager
```

## Componentes Principales

### 1. MapTile_Pure (src/MapTile_Pure.h/cpp)

**Responsabilidad**: Datos puros del tile sin lógica de GameObject

```cpp
class MapTile_Pure {
public:
    enum TILE_TYPE { NONE, GROUND, WALL, BOX };
    
    // Pure data, no GameObject inheritance
    virtual bool is_blocking() const = 0;
    virtual bool is_destructible() const = 0;
    virtual bool can_be_destroyed() const = 0;
    
    // Object management
    void set_bomb(Bomb* bomb);
    void set_bomber(Bomber* bomber);
    
private:
    TILE_TYPE type;
    int grid_x, grid_y;
    bool blocking, destructible;
    int sprite_nr;
    Bomb* bomb = nullptr;
    Bomber* bomber = nullptr;
};
```

**Subclases**:
- `MapTile_Ground_Pure`: Tile de suelo (no bloquea, no destructible)
- `MapTile_Wall_Pure`: Tile de pared (bloquea, no destructible)  
- `MapTile_Box_Pure`: Tile de caja (bloquea, destructible)

### 2. TileEntity (src/TileEntity.h/cpp)

**Responsabilidad**: GameObject wrapper que usa composición para envolver MapTile_Pure

```cpp
class TileEntity : public GameObject {
public:
    TileEntity(MapTile_Pure* tile_data, ClanBomberApplication* app);
    
    // Delegation pattern - forward to tile_data
    bool is_blocking() const { return tile_data ? tile_data->is_blocking() : false; }
    bool is_destructible() const { return tile_data ? tile_data->is_destructible() : false; }
    
    // GameObject lifecycle
    virtual void destroy() override;
    
private:
    MapTile_Pure* tile_data;  // Composition, not inheritance
};
```

**Subclases especializadas**:
- `TileEntity_Box`: Efectos especiales para destrucción de cajas (partículas, sonidos)

### 3. Map (src/Map.h/cpp)

**Responsabilidad**: Grid manager puro + dual architecture support

```cpp
class Map {
public:
    // Legacy compatibility
    MapTile* get_tile(int x, int y);
    
    // New architecture
    TileEntity* get_tile_entity(int x, int y);
    MapTile_Pure* get_tile_data(int x, int y);
    
private:
    MapTile* tiles[MAP_WIDTH][MAP_HEIGHT];          // Legacy grid
    TileEntity* tile_entities[MAP_WIDTH][MAP_HEIGHT]; // New grid
    MapTile_Pure* tile_data[MAP_WIDTH][MAP_HEIGHT];   // Pure data grid
};
```

### 4. TileManager (src/TileManager.h/cpp)

**Responsabilidad**: Coordinación inteligente entre sistemas

```cpp
class TileManager {
public:
    // Unified tile operations
    bool is_blocking_at(int map_x, int map_y);
    bool has_bomb_at(int map_x, int map_y);
    bool has_bomber_at(int map_x, int map_y);
    
    // Intelligent coordination
    void destroy_tile_at(int map_x, int map_y);
    void handle_bomber_movement(Bomber* bomber, int from_x, int from_y, int to_x, int to_y);
    
private:
    ClanBomberApplication* app;
};
```

## Integración con LifecycleManager

### Registro de TileEntity

```cpp
// En Map::create_tile_entities()
TileEntity* entity = new TileEntity(tile_data[x][y], app);
tile_entities[x][y] = entity;
app->lifecycle_manager->register_tile_entity(entity);
```

### Destrucción Controlada

```cpp
// En TileEntity::destroy()
void TileEntity::destroy() {
    if (!destroyed && tile_data && tile_data->can_be_destroyed()) {
        destroyed = true;
        
        // LIFECYCLE INTEGRATION
        if (app && app->lifecycle_manager) {
            app->lifecycle_manager->mark_tile_entity_for_destruction(this);
        }
        
        // Visual/audio effects
        play_destruction_effects();
    }
}
```

## Soporte Dual-Architecture

Durante la transición, el sistema mantiene ambas arquitecturas funcionando simultáneamente:

### GameObject Helper Methods

```cpp
class GameObject {
public:
    // NEW: Dual architecture tile access
    MapTile* get_legacy_tile() const;
    TileEntity* get_tile_entity() const;
    
    // NEW: Architecture-agnostic helpers
    int get_tile_type_at(int pixel_x, int pixel_y) const;
    bool is_tile_blocking_at(int pixel_x, int pixel_y) const;
    bool has_bomb_at(int pixel_x, int pixel_y) const;
    bool has_bomber_at(int pixel_x, int pixel_y) const;
};
```

### AI Controller Updates

```cpp
// En Controller_AI_Modern.cpp
namespace {
    bool is_tile_blocking_at(Map* map, int map_x, int map_y) {
        // Try legacy MapTile first
        MapTile* legacy_tile = map->get_tile(map_x, map_y);
        if (legacy_tile) {
            return legacy_tile->is_blocking();
        }
        
        // Try new TileEntity
        TileEntity* tile_entity = map->get_tile_entity(map_x, map_y);
        if (tile_entity) {
            return tile_entity->is_blocking();
        }
        
        return false;
    }
}
```

## Ventajas de la Nueva Arquitectura

### 1. Separación de Responsabilidades
- **MapTile_Pure**: Solo datos y lógica de tile
- **TileEntity**: Solo integración con GameObject/rendering
- **Map**: Solo grid management
- **TileManager**: Solo coordinación entre sistemas

### 2. Eliminación de Coordination Crossing
- Responsabilidades claramente definidas
- No más lógica dispersa entre clases
- Coordinación centralizada en TileManager

### 3. Mejor Testabilidad
- MapTile_Pure es testeable sin dependencias de SDL/OpenGL
- TileEntity se puede testear independientemente
- Componentes desacoplados

### 4. Flexibilidad Futura
- Fácil agregar nuevos tipos de tiles
- Posible optimización de memoria (pool de TileEntity)
- Soporte para tiles dinámicos

## Patrones de Uso

### Crear Nuevo Tipo de Tile

```cpp
// 1. Crear MapTile_Pure subclass
class MapTile_Lava_Pure : public MapTile_Pure {
public:
    MapTile_Lava_Pure(int grid_x, int grid_y) 
        : MapTile_Pure(LAVA, grid_x, grid_y) {
        blocking = false;
        destructible = false;
        sprite_nr = 5;
    }
    
    bool causes_damage() const override { return true; }
};

// 2. Crear TileEntity subclass (opcional)
class TileEntity_Lava : public TileEntity {
public:
    void act(float deltaTime) override {
        TileEntity::act(deltaTime);
        // Lava animation logic
        animate_lava(deltaTime);
    }
};
```

### Verificar Estado de Tile (Architecture-Agnostic)

```cpp
// En cualquier GameObject
if (is_tile_blocking_at(target_x, target_y)) {
    // Handle blocking
}

if (has_bomb_at(target_x, target_y)) {
    // Handle bomb collision
}
```

### Destruir Tile Correctamente

```cpp
// En Explosion::destroy_tile_at()
void Explosion::destroy_tile_at(int map_x, int map_y) {
    bool tile_destroyed = false;
    
    // Handle legacy MapTile
    MapTile* legacy_tile = app->map->get_tile(map_x, map_y);
    if (legacy_tile && legacy_tile->is_burnable()) {
        legacy_tile->destroy();
        tile_destroyed = true;
    }
    
    // Handle new TileEntity (avoid duplicate destruction)
    if (!tile_destroyed) {
        TileEntity* tile_entity = app->map->get_tile_entity(map_x, map_y);
        if (tile_entity && tile_entity->is_destructible()) {
            tile_entity->destroy();
        }
    }
}
```

## Estado de Implementación

✅ **Completado**:
- MapTile_Pure base class y subclases
- TileEntity base class y TileEntity_Box
- Integración con LifecycleManager
- Soporte dual-architecture en GameObject
- AI controllers actualizados
- Sistema de destrucción optimizado
- Compilation y testing exitoso

🔄 **En Progreso**:
- Migración gradual de legacy code
- Optimizaciones de performance

📋 **Futuro**:
- Eliminación completa de legacy MapTile (cuando sea seguro)
- Pool de TileEntity para mejor performance
- Tiles dinámicos y animados

## Conclusión

La nueva arquitectura Map/MapTile/TileManager representa una mejora significativa sobre el sistema anterior:

- **Mejor arquitectura**: Composición vs herencia
- **Código más limpio**: Responsabilidades separadas
- **Mayor flexibilidad**: Fácil extensión y testing
- **Backward compatibility**: Transición gradual sin breaking changes

Esta refactorización establece una base sólida para el futuro desarrollo del juego ClanBomber SDL3.