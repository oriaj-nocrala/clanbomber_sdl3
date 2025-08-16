# LifecycleManager - Ejemplos de Uso

## Ejemplo 1: Objeto Simple con Destrucción

```cpp
// Crear un nuevo tipo de objeto
class MiProyectil : public GameObject {
public:
    MiProyectil(int x, int y, ClanBomberApplication* app) : GameObject(x, y, app) {
        texture_name = "proyectil";
        velocidad = 200.0f;
        tiempo_vida = 3.0f; // 3 segundos de vida
        // LifecycleManager se registra automáticamente en GameObject constructor
    }
    
    void act(float deltaTime) override {
        tiempo_vida -= deltaTime;
        
        // Moverse
        x += velocidad * deltaTime;
        
        // Auto-destruirse después de tiempo límite
        if (tiempo_vida <= 0.0f) {
            delete_me = true; // LifecycleManager maneja la transición ACTIVE → DYING
        }
        
        // Verificar colisiones
        if (colision_detectada()) {
            delete_me = true; // Destrucción inmediata por impacto
        }
    }
    
private:
    float velocidad;
    float tiempo_vida;
};

// Uso
MiProyectil* proyectil = new MiProyectil(100, 100, app);
app->objects.push_back(proyectil);
// No necesitas gestionar manualmente la eliminación
```

## Ejemplo 2: Tile Personalizado con Animación

```cpp
class MapTile_Hielo : public MapTile {
public:
    MapTile_Hielo(int x, int y, ClanBomberApplication* app) : MapTile(x, y, app) {
        texture_name = "tiles_hielo";
        sprite_nr = 15;
        destructible = true;
        derretir_timer = 0.0f;
        derritiendose = false;
        // LifecycleManager registra automáticamente en MapTile::create()
    }
    
    void act() override {
        if (derritiendose) {
            derretir_timer += Timer::time_elapsed();
            
            // Animación de derretimiento durante 2 segundos
            if (derretir_timer >= 2.0f && !delete_me) {
                SDL_Log("Hielo derretido completamente en (%d,%d)", get_map_x(), get_map_y());
                delete_me = true; // LifecycleManager coordina el reemplazo
            }
        }
    }
    
    void destroy() override {
        if (!derritiendose) {
            SDL_Log("Iniciando derretimiento de hielo en (%d,%d)", get_map_x(), get_map_y());
            derritiendose = true;
            derretir_timer = 0.0f;
            blocking = false; // Los jugadores pueden pasar por hielo derritiéndose
            
            // Efectos de sonido
            AudioPosition pos(get_x(), get_y(), 0.0f);
            AudioMixer::play_sound_3d("hielo_derretir", pos, 400.0f);
        }
    }
    
    void show() override {
        if (!derritiendose) {
            MapTile::show(); // Tile normal
        } else {
            // Renderizar animación de derretimiento
            float progreso = derretir_timer / 2.0f; // 0.0 a 1.0
            float alpha = 1.0f - progreso; // Fade out gradual
            
            // Cambiar sprite según progreso
            int sprite_derretimiento = 15 + (int)(progreso * 4); // Sprites 15-19
            
            // Renderizar con transparencia
            if (app && app->gpu_renderer) {
                GLuint gl_texture = Resources::get_gl_texture(texture_name);
                float color[4] = {1.0f, 1.0f, 1.0f, alpha};
                float scale[2] = {1.0f, 1.0f};
                
                app->gpu_renderer->add_sprite(
                    get_x(), get_y(), 40.0f, 40.0f,
                    gl_texture, color, 0.0f, scale, sprite_derretimiento
                );
            }
        }
    }
    
private:
    bool derritiendose;
    float derretir_timer;
};
```

## Ejemplo 3: Debugging de Estados

```cpp
void debug_lifecycle_status(ClanBomberApplication* app) {
    if (!app->lifecycle_manager) return;
    
    SDL_Log("=== Estado del LifecycleManager ===");
    SDL_Log("Objetos activos: %zu", app->lifecycle_manager->get_active_object_count());
    SDL_Log("Tiles activos: %zu", app->lifecycle_manager->get_active_tile_count());
    
    // Verificar estado de objetos específicos
    for (auto& objeto : app->objects) {
        LifecycleManager::ObjectState estado = 
            app->lifecycle_manager->get_object_state(objeto);
        
        const char* estado_str;
        switch (estado) {
            case LifecycleManager::ObjectState::ACTIVE: estado_str = "ACTIVE"; break;
            case LifecycleManager::ObjectState::DYING: estado_str = "DYING"; break;
            case LifecycleManager::ObjectState::DEAD: estado_str = "DEAD"; break;
            case LifecycleManager::ObjectState::DELETED: estado_str = "DELETED"; break;
        }
        
        SDL_Log("Objeto %p (%s): %s", objeto, objeto->texture_name.c_str(), estado_str);
    }
    
    // Verificar si algún objeto está "atorado" en un estado
    for (auto& bomber : app->bomber_objects) {
        if (app->lifecycle_manager->is_dying_or_dead(bomber)) {
            SDL_Log("ADVERTENCIA: Bomber %p está muriendo/muerto", bomber);
        }
    }
}
```

## Ejemplo 4: Tile con Múltiples Fases

```cpp
class MapTile_Arbol : public MapTile {
public:
    enum FaseArbol {
        SALUDABLE,
        DAÑADO,
        ARDIENDO,
        CENIZAS
    };
    
    MapTile_Arbol(int x, int y, ClanBomberApplication* app) : MapTile(x, y, app) {
        texture_name = "tiles_naturaleza";
        fase = SALUDABLE;
        sprite_nr = 20; // Árbol saludable
        destructible = true;
        timer_fase = 0.0f;
        vida = 100;
    }
    
    void act() override {
        timer_fase += Timer::time_elapsed();
        
        switch (fase) {
            case SALUDABLE:
                // Árbol normal, no hace nada especial
                break;
                
            case DAÑADO:
                // Árbol dañado, se regenera lentamente
                if (timer_fase >= 1.0f) {
                    vida += 5; // Regenera 5 HP por segundo
                    timer_fase = 0.0f;
                    
                    if (vida >= 100) {
                        cambiar_fase(SALUDABLE);
                    }
                }
                break;
                
            case ARDIENDO:
                // Árbol ardiendo, pierde vida rápidamente
                if (timer_fase >= 0.2f) {
                    vida -= 10; // Pierde 10 HP cada 0.2s
                    timer_fase = 0.0f;
                    
                    if (vida <= 0) {
                        cambiar_fase(CENIZAS);
                    }
                }
                break;
                
            case CENIZAS:
                // Cenizas eventualmente desaparecen
                if (timer_fase >= 5.0f && !delete_me) {
                    SDL_Log("Árbol convertido en cenizas se desvanece");
                    delete_me = true; // LifecycleManager maneja la transición
                }
                break;
        }
    }
    
    void destroy() override {
        // Al ser destruido, determinar nueva fase según vida actual
        if (vida > 50) {
            cambiar_fase(DAÑADO);
        } else {
            cambiar_fase(ARDIENDO);
        }
    }
    
private:
    void cambiar_fase(FaseArbol nueva_fase) {
        fase = nueva_fase;
        timer_fase = 0.0f;
        
        switch (fase) {
            case SALUDABLE:
                sprite_nr = 20;
                blocking = true;
                break;
            case DAÑADO:
                sprite_nr = 21;
                blocking = true;
                break;
            case ARDIENDO:
                sprite_nr = 22;
                blocking = false; // Se puede pasar por árbol ardiendo
                // Efectos de fuego
                if (app->gpu_renderer) {
                    app->gpu_renderer->emit_particles(get_x(), get_y(), 10, 
                        GPUAcceleratedRenderer::FIRE, nullptr, 1.0f);
                }
                break;
            case CENIZAS:
                sprite_nr = 23;
                blocking = false;
                destructible = false; // Las cenizas no se pueden destruir
                break;
        }
    }
    
    FaseArbol fase;
    float timer_fase;
    int vida;
};
```

## Ejemplo 5: Integración con Sistema de Partículas

```cpp
class EfectoExplosion : public GameObject {
public:
    EfectoExplosion(int x, int y, int poder, ClanBomberApplication* app) 
        : GameObject(x, y, app) {
        texture_name = ""; // Sin sprite, solo partículas
        this->poder = poder;
        duracion_total = 2.0f + poder * 0.5f; // Más poder = más duración
        tiempo_transcurrido = 0.0f;
        
        // Iniciar efectos inmediatamente
        iniciar_explosion();
    }
    
    void act(float deltaTime) override {
        tiempo_transcurrido += deltaTime;
        
        // Efectos secuenciales basados en tiempo
        if (tiempo_transcurrido >= 0.1f && !chispa_inicial_creada) {
            crear_chispas_iniciales();
            chispa_inicial_creada = true;
        }
        
        if (tiempo_transcurrido >= 0.3f && !humo_creado) {
            crear_humo();
            humo_creado = true;
        }
        
        if (tiempo_transcurrido >= duracion_total) {
            delete_me = true; // LifecycleManager maneja la limpieza
        }
    }
    
    void show() override {
        // Este efecto es puramente basado en partículas
        // No necesita renderizar sprite
    }
    
private:
    void iniciar_explosion() {
        if (app->gpu_renderer) {
            // Explosión inicial intensa
            app->gpu_renderer->emit_particles(get_x(), get_y(), 
                20 + poder * 5, GPUAcceleratedRenderer::FIRE, nullptr, 2.0f);
        }
        
        // Sonido de explosión
        AudioPosition pos(get_x(), get_y(), 0.0f);
        AudioMixer::play_sound_3d("explode", pos, 800.0f);
    }
    
    void crear_chispas_iniciales() {
        if (app->gpu_renderer) {
            app->gpu_renderer->emit_particles(get_x(), get_y(), 
                15 + poder * 3, GPUAcceleratedRenderer::SPARK, nullptr, 1.5f);
        }
    }
    
    void crear_humo() {
        if (app->gpu_renderer) {
            app->gpu_renderer->emit_particles(get_x(), get_y(), 
                10 + poder * 2, GPUAcceleratedRenderer::SMOKE, nullptr, 3.0f);
        }
    }
    
    int poder;
    float duracion_total;
    float tiempo_transcurrido;
    bool chispa_inicial_creada = false;
    bool humo_creado = false;
};

// Uso en explosión de bomba
void Bomb::explode() {
    // Crear efecto visual
    EfectoExplosion* efecto = new EfectoExplosion(get_x(), get_y(), power, app);
    app->objects.push_back(efecto);
    
    // Crear explosiones en todas las direcciones
    // ... resto de lógica de explosión
    
    // Auto-destruirse
    delete_me = true; // LifecycleManager coordina la eliminación
}
```

## Ejemplo 6: Consulta de Estados para AI

```cpp
class Controller_AI_Avanzado : public Controller {
public:
    void process_ai_logic(Bomber* bomber) {
        // Evitar objetos que están muriendo
        for (auto& objeto : app->objects) {
            if (app->lifecycle_manager->is_dying_or_dead(objeto)) {
                // Este objeto va a desaparecer pronto, no considerarlo en planificación
                continue;
            }
            
            // Evaluar objeto para AI...
        }
        
        // Verificar tiles del mapa
        for (int x = 0; x < MAP_WIDTH; x++) {
            for (int y = 0; y < MAP_HEIGHT; y++) {
                MapTile* tile = app->map->get_tile(x, y);
                
                if (app->lifecycle_manager->is_dying_or_dead(tile)) {
                    // Este tile está siendo destruido, será ground pronto
                    // Planificar ruta asumiendo que será transitable
                    continue;
                }
                
                // Evaluar tile para pathfinding...
            }
        }
    }
};
```

## Patrones Recomendados

### ✅ Buenas Prácticas

```cpp
// 1. Dejar que LifecycleManager maneje automáticamente
objeto->delete_me = true;

// 2. Consultar estado antes de operaciones críticas
if (!app->lifecycle_manager->is_dying_or_dead(target)) {
    // Seguro operar en este objeto
}

// 3. Usar logs del sistema para debugging
// Los logs automáticos del LifecycleManager son suficientes
```

### ❌ Anti-Patrones

```cpp
// 1. NO eliminar objetos manualmente
delete objeto; // ¡NUNCA HACER ESTO!

// 2. NO asumir que delete_me significa eliminación inmediata
objeto->delete_me = true;
// objeto aún existe y puede seguir renderizándose

// 3. NO gestionar estados manualmente
// objeto->estado = DYING; // Dejar que LifecycleManager lo maneje
```

---

*Estos ejemplos muestran cómo integrar correctamente el LifecycleManager en diferentes tipos de objetos y situaciones.*