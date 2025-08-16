# LifecycleManager - Guía de Debugging

## Logs Automáticos del Sistema

El LifecycleManager genera logs detallados automáticamente. Usar estos logs para diagnosticar problemas:

### Logs de Registro

```
LifecycleManager: Registered object 0x57ad754ffd20 (total: 303)
LifecycleManager: Registered tile 0x57ad754eb1b0 at (1,3) (total: 62)
```

**Interpretación**:
- Objeto registrado exitosamente con dirección de memoria
- Total de objetos/tiles actualmente gestionados
- Para tiles: coordenadas del mapa incluidas

### Logs de Transición

```
LifecycleManager: Object 0x57ad754ffd20 self-marked for destruction
LifecycleManager: Object 0x57ad754ffd20 death animation complete (DYING → DEAD)
LifecycleManager: Object 0x57ad754ffd20 ready for deletion (DEAD → DELETED)
```

**Flujo esperado**: ACTIVE → DYING → DEAD → DELETED

### Logs de Limpieza

```
GameplayScreen: Deleting object 0x57ad754ffd20 (LifecycleManager approved)
LifecycleManager: Removing object 0x57ad754ffd20 from tracking (deletion handled elsewhere)
LifecycleManager: Removed 1 objects from tracking
```

## Problemas Comunes y Soluciones

### 1. Black Gaps Persistentes

**Síntomas**:
- Tiles aparecen negros después de destrucción
- Objetos desaparecen bruscamente

**Debug**:
```cpp
// Verificar si el objeto sigue en estado correcto
LifecycleManager::ObjectState estado = app->lifecycle_manager->get_object_state(tile);
SDL_Log("Estado del tile problemático: %d", (int)estado);

// Verificar timing en show()
if (destroyed) {
    SDL_Log("Tile destruido renderizando fragmentos: animation=%.3f", destroy_animation);
}
```

**Posibles causas**:
1. **Render precoz**: `show()` retorna demasiado pronto
2. **Timing incorrecto**: Animación termina antes de reemplazo
3. **Estado desincronizado**: delete_me set antes de LifecycleManager

**Solución**:
```cpp
// En show(), asegurar render durante DYING
if (app && app->lifecycle_manager) {
    LifecycleManager::ObjectState state = app->lifecycle_manager->get_object_state(this);
    if (state == LifecycleManager::ObjectState::DELETED) {
        return; // Solo no renderizar si verdaderamente eliminado
    }
    // Continuar renderizado para ACTIVE, DYING, DEAD
}
```

### 2. Double Registration

**Síntomas**:
```
LifecycleManager: Object 0x57ad754eb1b0 already registered
```

**Posibles causas**:
1. Constructor llamado múltiples veces
2. Registro manual además del automático
3. Objetos reutilizados sin limpiar

**Solución**:
```cpp
// Verificar constructor único
GameObject::GameObject(...) {
    static std::set<GameObject*> created_objects;
    if (created_objects.find(this) != created_objects.end()) {
        SDL_Log("ERROR: Double construction of object %p", this);
    }
    created_objects.insert(this);
}
```

### 3. Memory Leaks

**Síntomas**:
- Objetos no se eliminan nunca
- Contadores de objetos siempre crecen

**Debug**:
```cpp
void debug_memory_status() {
    SDL_Log("Objetos en vectores del juego: %zu", app->objects.size());
    SDL_Log("Objetos activos en LifecycleManager: %zu", 
            app->lifecycle_manager->get_active_object_count());
    
    // Si estos números difieren significativamente, hay un problema
}
```

**Verificar flujo completo**:
```cpp
// 1. ¿El objeto se marca para destrucción?
if (conditions_met) {
    SDL_Log("Marcando objeto %p para destrucción", this);
    delete_me = true;
}

// 2. ¿LifecycleManager detecta el cambio?
// (Ver logs automáticos)

// 3. ¿GameplayScreen elimina el objeto?
// (Ver logs de "Deleting object")

// 4. ¿Se limpia el tracking?
// (Ver logs de "Removed N objects from tracking")
```

### 4. Objetos "Zombies"

**Síntomas**:
- Objetos en estado DELETED pero siguen activos
- Comportamiento errático después de destrucción

**Debug**:
```cpp
void GameObject::act(float deltaTime) {
    // Verificar estado al inicio
    if (app && app->lifecycle_manager) {
        LifecycleManager::ObjectState state = app->lifecycle_manager->get_object_state(this);
        if (state == LifecycleManager::ObjectState::DELETED) {
            SDL_Log("ERROR: Objeto zombie %p actuando después de eliminación", this);
            return; // No procesar lógica de objeto eliminado
        }
    }
    
    // Lógica normal...
}
```

**Solución**: Verificar que `delete_some()` se llama correctamente y que no hay referencias colgantes.

### 5. Timing Desincronizado

**Síntomas**:
- Reemplazos ocurren demasiado pronto/tarde
- Animaciones cortadas o extendidas

**Debug timing**:
```cpp
// En MapTile_Box::act()
if (destroyed) {
    SDL_Log("Tile (%d,%d): animation=%.3f/0.5, delete_me=%d", 
            get_map_x(), get_map_y(), destroy_animation, (int)delete_me);
    
    if (destroy_animation >= 0.5f && !delete_me) {
        SDL_Log("Marcando tile (%d,%d) para eliminación", get_map_x(), get_map_y());
    }
}
```

**Verificar coordinación**:
```cpp
// En Map::act()
if (maptiles[x][y]->delete_me) {
    LifecycleManager::ObjectState state = 
        app->lifecycle_manager->get_tile_state(maptiles[x][y]);
    
    SDL_Log("Tile (%d,%d): delete_me=true, state=%d", x, y, (int)state);
    
    if (state == LifecycleManager::ObjectState::DELETED) {
        SDL_Log("Reemplazando tile (%d,%d) (LifecycleManager approved)", x, y);
    } else {
        SDL_Log("Esperando aprobación de LifecycleManager para tile (%d,%d)", x, y);
    }
}
```

## Herramientas de Debug

### 1. Debug Console

```cpp
void debug_print_lifecycle_state() {
    if (!app->lifecycle_manager) {
        SDL_Log("LifecycleManager no disponible");
        return;
    }
    
    SDL_Log("=== ESTADO LIFECYCLEMANAGER ===");
    SDL_Log("Objetos activos: %zu", app->lifecycle_manager->get_active_object_count());
    SDL_Log("Tiles activos: %zu", app->lifecycle_manager->get_active_tile_count());
    
    // Estados detallados
    int active_count = 0, dying_count = 0, dead_count = 0, deleted_count = 0;
    
    for (auto& obj : app->objects) {
        LifecycleManager::ObjectState state = app->lifecycle_manager->get_object_state(obj);
        switch (state) {
            case LifecycleManager::ObjectState::ACTIVE: active_count++; break;
            case LifecycleManager::ObjectState::DYING: dying_count++; break;
            case LifecycleManager::ObjectState::DEAD: dead_count++; break;
            case LifecycleManager::ObjectState::DELETED: deleted_count++; break;
        }
    }
    
    SDL_Log("Estados: ACTIVE=%d, DYING=%d, DEAD=%d, DELETED=%d", 
            active_count, dying_count, dead_count, deleted_count);
}

// Llamar con tecla de debug (ej: F12)
void handle_debug_keys(SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_F12) {
            debug_print_lifecycle_state();
        }
    }
}
```

### 2. Visual Debug Overlay

```cpp
void render_debug_overlay(SDL_Renderer* renderer) {
    if (!show_debug_overlay) return;
    
    int y_offset = 10;
    
    // Información de LifecycleManager
    std::string info = "LifecycleManager: " + 
        std::to_string(app->lifecycle_manager->get_active_object_count()) + 
        " objetos activos";
    render_debug_text(renderer, info, 10, y_offset);
    y_offset += 20;
    
    // Estados de objetos críticos
    for (auto& bomber : app->bomber_objects) {
        LifecycleManager::ObjectState state = app->lifecycle_manager->get_object_state(bomber);
        std::string bomber_info = "Bomber " + bomber->get_name() + ": ";
        
        switch (state) {
            case LifecycleManager::ObjectState::ACTIVE: bomber_info += "ACTIVE"; break;
            case LifecycleManager::ObjectState::DYING: bomber_info += "DYING"; break;
            case LifecycleManager::ObjectState::DEAD: bomber_info += "DEAD"; break;
            case LifecycleManager::ObjectState::DELETED: bomber_info += "DELETED"; break;
        }
        
        render_debug_text(renderer, bomber_info, 10, y_offset);
        y_offset += 15;
    }
}
```

### 3. Assert para Desarrollo

```cpp
// En código crítico, verificar estados esperados
void GameplayScreen::delete_some() {
    for (auto it = app->objects.begin(); it != app->objects.end();) {
        GameObject* obj = *it;
        LifecycleManager::ObjectState state = app->lifecycle_manager->get_object_state(obj);
        
        if (state == LifecycleManager::ObjectState::DELETED) {
            // Assert de desarrollo
            assert(obj->delete_me && "Objeto DELETED debe tener delete_me=true");
            
            delete obj;
            it = app->objects.erase(it);
        } else {
            ++it;
        }
    }
}
```

## Métricas de Performance

### Timing Crítico

```cpp
void measure_lifecycle_performance() {
    auto start = std::chrono::high_resolution_clock::now();
    
    app->lifecycle_manager->update_states(deltaTime);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Log si toma más de 100 microsegundos
    if (duration.count() > 100) {
        SDL_Log("LifecycleManager::update_states tardó %lld μs", duration.count());
    }
}
```

### Memory Profiling

```cpp
void profile_lifecycle_memory() {
    size_t estimated_memory = 
        app->lifecycle_manager->get_active_object_count() * sizeof(ManagedObject) +
        app->lifecycle_manager->get_active_tile_count() * sizeof(ManagedTile);
    
    SDL_Log("LifecycleManager memoria estimada: %zu bytes", estimated_memory);
    
    // Alert si usa más de 10KB
    if (estimated_memory > 10240) {
        SDL_Log("ADVERTENCIA: LifecycleManager usando mucha memoria");
    }
}
```

## Troubleshooting Checklist

Cuando tengas problemas con LifecycleManager:

1. **✓ Verificar logs automáticos** - ¿Se ven las transiciones esperadas?
2. **✓ Confirmar registro** - ¿Aparecen los logs "Registered object/tile"?
3. **✓ Validar timing** - ¿Las animaciones duran lo esperado?
4. **✓ Revisar coordinación** - ¿Map::act() espera aprobación del LifecycleManager?
5. **✓ Comprobar cleanup** - ¿Se ven los logs de eliminación y tracking cleanup?
6. **✓ Estados consistentes** - ¿Los objetos DELETED realmente no se procesan?
7. **✓ Performance** - ¿update_states() tarda menos de 1ms?

---

*Esta guía debería resolver la mayoría de problemas comunes con el LifecycleManager. Para issues específicos, revisar los logs automáticos primero.*