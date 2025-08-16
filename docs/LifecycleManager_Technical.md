# LifecycleManager - Documentación Técnica

## Arquitectura Interna

### Estructuras de Datos

```cpp
struct ManagedObject {
    GameObject* object;                    // Puntero al objeto gestionado
    ObjectState state;                     // Estado actual del objeto
    float state_timer;                     // Tiempo en estado actual
    std::function<void()> on_state_change; // Callback opcional para transiciones
};

struct ManagedTile {
    MapTile* tile;                         // Puntero al tile gestionado
    ObjectState state;                     // Estado actual del tile
    float state_timer;                     // Tiempo en estado actual
    int map_x, map_y;                      // Coordenadas en el mapa
    MapTile* replacement;                  // Tile de reemplazo (típicamente ground)
};
```

### Contenedores

```cpp
std::vector<ManagedObject> managed_objects;
std::vector<ManagedTile> managed_tiles;
```

**Decisión de diseño**: Se usa `std::vector` en lugar de `std::list` por:
- Mejor localidad de caché (objetos contiguos en memoria)
- Iteración más rápida (importante para update_states())
- Menor overhead de memoria por elemento

## API Pública

### Registro de Objetos

```cpp
void register_object(GameObject* obj);
void register_tile(MapTile* tile, int map_x, int map_y);
```

**Notas importantes**:
- Los objetos se registran automáticamente en sus constructores
- Los tiles se registran en `MapTile::create()`
- Double-registration es detectada y ignorada silenciosamente

### Control de Estados

```cpp
void mark_for_destruction(GameObject* obj);
void mark_tile_for_destruction(MapTile* tile, MapTile* replacement = nullptr);
```

**Política**: Solo objetos en estado `ACTIVE` pueden transicionar a `DYING`. Llamadas repetidas son ignoradas.

### Consultas de Estado

```cpp
ObjectState get_object_state(GameObject* obj) const;
ObjectState get_tile_state(MapTile* tile) const;
bool is_dying_or_dead(GameObject* obj) const;
bool is_dying_or_dead(MapTile* tile) const;
```

**Rendimiento**: O(n) linear search. Para optimizar en el futuro, considerar `std::unordered_map<void*, size_t>` para indexación.

## Ciclo de Vida de Estados

### update_states(float deltaTime)

```cpp
void update_states(float deltaTime) {
    // Actualizar objetos
    for (auto& managed : managed_objects) {
        if (managed.state != ObjectState::DELETED) {
            update_object_state(managed, deltaTime);
        }
    }
    
    // Actualizar tiles
    for (auto& managed : managed_tiles) {
        if (managed.state != ObjectState::DELETED) {
            update_tile_state(managed, deltaTime);
        }
    }
}
```

### Timing de Estados

| Estado | Duración GameObject | Duración MapTile | Propósito |
|--------|-------------------|------------------|-----------|
| ACTIVE | Indefinida | Indefinida | Operación normal |
| DYING | 0.1s | 0.5s | Animación de muerte |
| DEAD | 1 frame | 1 frame | Listo para limpieza |
| DELETED | Hasta cleanup | Hasta cleanup | Pendiente eliminación |

### Transiciones Automáticas

```cpp
// En update_object_state()
case ObjectState::ACTIVE:
    if (managed.object->delete_me) {
        managed.state = ObjectState::DYING;
        managed.state_timer = 0.0f;
    }
    break;

case ObjectState::DYING:
    if (managed.state_timer >= 0.1f) {  // GameObject timeout
        managed.state = ObjectState::DEAD;
        managed.state_timer = 0.0f;
    }
    break;

case ObjectState::DEAD:
    managed.state = ObjectState::DELETED;
    break;
```

## Integración con Sistemas Existentes

### GameObject Integration

```cpp
// En GameObject::show()
if (app && app->lifecycle_manager) {
    LifecycleManager::ObjectState state = app->lifecycle_manager->get_object_state(this);
    if (state == LifecycleManager::ObjectState::DELETED) {
        return;  // No renderizar objetos verdaderamente muertos
    }
    // Continuar renderizado para ACTIVE, DYING, DEAD
}
```

**Decisión crítica**: Los objetos en estado `DYING` y `DEAD` **siguen renderizándose** para evitar black gaps.

### Map Integration

```cpp
// En Map::act()
if (maptiles[x][y]->delete_me) {
    if (app->lifecycle_manager) {
        LifecycleManager::ObjectState state = 
            app->lifecycle_manager->get_tile_state(maptiles[x][y]);
        
        if (state == LifecycleManager::ObjectState::DELETED) {
            // Solo ahora reemplazar el tile
            delete maptiles[x][y];
            maptiles[x][y] = MapTile::create(MapTile::GROUND, x*40, y*40, app);
        }
    }
}
```

### GameplayScreen Integration

```cpp
// En delete_some()
app->objects.remove_if([this](GameObject* obj) {
    LifecycleManager::ObjectState state = app->lifecycle_manager->get_object_state(obj);
    if (state == LifecycleManager::ObjectState::DELETED) {
        delete obj;
        return true;
    }
    return false;
});
```

## Debugging y Logging

### Log Levels

```cpp
SDL_Log("LifecycleManager: Registered object %p (total: %zu)", obj, managed_objects.size());
SDL_Log("LifecycleManager: Object %p self-marked for destruction", managed.object);
SDL_Log("LifecycleManager: Object %p death animation complete (DYING → DEAD)", managed.object);
SDL_Log("LifecycleManager: Object %p ready for deletion (DEAD → DELETED)", managed.object);
```

### Debug Queries

```cpp
size_t get_active_object_count() const;
size_t get_active_tile_count() const;
```

**Uso**: Para verificar memory leaks o problemas de acumulación de objetos.

## Memory Management

### RAII Compliance

```cpp
~LifecycleManager() {
    clear_all();  // Limpieza automática
}

void clear_all() {
    // Limpiar objetos gestionados
    for (const auto& managed : managed_objects) {
        delete managed.object;
    }
    managed_objects.clear();
    
    // Los tiles son propiedad de Map, no los eliminamos
    managed_tiles.clear();
}
```

### Ownership Model

- **GameObjects**: LifecycleManager NO posee los objetos, solo los rastrea
- **MapTiles**: Map posee los tiles, LifecycleManager solo coordina su reemplazo
- **Cleanup**: `cleanup_dead_objects()` solo remueve tracking, no libera memoria

## Performance Considerations

### Complejidad Temporal

| Operación | Complejidad | Notas |
|-----------|-------------|-------|
| register_object() | O(n) | Linear search para duplicados |
| update_states() | O(n) | Itera todos los objetos |
| get_object_state() | O(n) | Linear search |
| cleanup_dead_objects() | O(n) | std::remove_if |

### Optimizaciones Futuras

1. **Hash Map Index**: `std::unordered_map<GameObject*, size_t>` para O(1) lookups
2. **State Buckets**: Separar objetos por estado para skip iteraciones
3. **Dirty Flags**: Solo actualizar objetos que cambiarón

### Memory Overhead

Por objeto gestionado:
- `ManagedObject`: ~32 bytes (puntero + enum + float + function pointer)
- `ManagedTile`: ~40 bytes (puntero + enum + float + 2 ints + puntero)

## Thread Safety

**Estado actual**: **NO thread-safe**

**Consideraciones futuras**:
- Agregar `std::mutex` para operaciones críticas
- Lock-free containers para mejor rendimiento
- Estado immutable para queries sin locks

## Error Handling

### Defensive Programming

```cpp
void register_object(GameObject* obj) {
    if (!obj) return;  // Null pointer protection
    
    if (find_managed_object(obj)) {
        SDL_Log("LifecycleManager: Object %p already registered", obj);
        return;  // Duplicate registration protection
    }
    
    managed_objects.emplace_back(obj);
}
```

### Recovery Strategies

- **Null pointers**: Ignorados silenciosamente
- **Double registration**: Detectado y logueado
- **Invalid states**: Assert en debug, graceful degradation en release

## Testing Strategies

### Unit Tests (Pendientes)

```cpp
TEST(LifecycleManager, StateTransitions) {
    LifecycleManager manager;
    MockGameObject obj;
    
    manager.register_object(&obj);
    EXPECT_EQ(manager.get_object_state(&obj), ObjectState::ACTIVE);
    
    obj.delete_me = true;
    manager.update_states(0.05f);
    EXPECT_EQ(manager.get_object_state(&obj), ObjectState::DYING);
    
    manager.update_states(0.1f);
    EXPECT_EQ(manager.get_object_state(&obj), ObjectState::DEAD);
}
```

### Integration Tests

- Verificar eliminación de black gaps en tiles
- Confirmar sincronización Map/GameplayScreen
- Validar memory management sin leaks

## Changelog

### v1.0 (Actual)
- Implementación inicial con 4 estados
- Integración con GameObject, MapTile, Map, GameplayScreen
- Eliminación de black gaps en destrucción de tiles
- Fix para múltiples extras por destrucción repetida

### Futuras Versiones
- v1.1: Optimización de performance con hash maps
- v1.2: Thread safety para multiplayer
- v1.3: Estados adicionales (PAUSED, FROZEN)

---

*Esta documentación técnica sirve como referencia completa para mantener y extender el LifecycleManager.*