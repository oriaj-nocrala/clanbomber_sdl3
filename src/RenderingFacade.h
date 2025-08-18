#pragma once

#include "GPUAcceleratedRenderer.h"
#include "TextRenderer.h"
#include "ParticleEffectsManager.h"
#include "CoordinateSystem.h"
#include "ErrorHandling.h"
#include <memory>
#include <string>
#include <vector>

/**
 * @brief RenderingFacade - Interfaz unificada para todas las operaciones de rendering
 * 
 * PROBLEMA:
 * - Rendering code disperso en GPUAcceleratedRenderer, TextRenderer, ParticleEffectsManager
 * - GameObject.show() calls directo a renderer subsystems
 * - Difficult testing due to tight coupling
 * - Inconsistent error handling across renderers
 * 
 * SOLUCIÓN:
 * - Facade pattern para unificar rendering APIs
 * - Single point of entry para todas las operaciones
 * - Consistent error handling
 * - Easier testing con mock facade
 */

struct RenderCommand {
    enum Type {
        SPRITE,
        TEXT,
        PARTICLE_SYSTEM,
        UI_ELEMENT,
        DEBUG_OVERLAY
    };
    
    Type command_type;
    std::string texture_name;
    PixelCoord position;
    int sprite_nr = 0;
    float rotation = 0.0f;
    uint8_t opacity = 255;
    std::string text_content;
    std::string font_name;
};

/**
 * @brief Configuración de rendering para diferentes contextos
 */
struct RenderingConfig {
    bool enable_debug_overlays = false;
    bool enable_particle_effects = true;
    bool enable_sprite_batching = true;
    bool enable_gpu_acceleration = true;
    int max_particles = 100000;
    float ui_scale_factor = 1.0f;
};

/**
 * @brief Estadísticas de rendering para debugging/profiling
 */
struct RenderingStats {
    uint32_t sprites_rendered = 0;
    uint32_t text_elements_rendered = 0;
    uint32_t particles_rendered = 0;
    uint32_t draw_calls = 0;
    float frame_time_ms = 0.0f;
    size_t texture_memory_usage = 0;
};

/**
 * @brief Facade principal para rendering operations
 */
class RenderingFacade {
public:
    explicit RenderingFacade(RenderingConfig config = RenderingConfig{});
    ~RenderingFacade();

    // === INITIALIZATION ===
    
    /**
     * @brief Inicializa todos los subsistemas de rendering
     * @param window SDL window handle
     * @param width Screen width  
     * @param height Screen height
     * @return GameResult indicando éxito o error
     */
    GameResult<void> initialize(SDL_Window* window, int width, int height);
    
    /**
     * @brief Limpia recursos y shutdown de subsistemas
     */
    void shutdown();
    
    /**
     * @brief Verifica si el facade está listo para rendering
     */
    bool is_ready() const { return initialized; }

    // === FRAME RENDERING ===
    
    /**
     * @brief Inicia un nuevo frame de rendering
     * @return GameResult indicando éxito o error
     */
    GameResult<void> begin_frame();
    
    /**
     * @brief Finaliza el frame y presenta en pantalla
     * @return GameResult indicando éxito o error  
     */
    GameResult<void> end_frame();
    
    /**
     * @brief Limpia el backbuffer con color específico
     */
    void clear_screen(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255);

    // === SPRITE RENDERING ===
    
    /**
     * @brief Renderiza un sprite en posición específica
     * @param texture_name Nombre de la textura
     * @param position Posición en coordenadas pixel
     * @param sprite_nr Número de sprite en la textura
     * @param rotation Rotación en radianes (opcional)
     * @param opacity Opacidad 0-255 (opcional)
     * @return GameResult indicando éxito o error
     */
    GameResult<void> render_sprite(const std::string& texture_name, 
                                  const PixelCoord& position,
                                  int sprite_nr = 0,
                                  float rotation = 0.0f,
                                  uint8_t opacity = 255);
    
    /**
     * @brief Renderiza sprite usando coordenadas de grid
     */
    GameResult<void> render_sprite_at_grid(const std::string& texture_name,
                                          const GridCoord& grid_position,
                                          int sprite_nr = 0,
                                          float rotation = 0.0f,
                                          uint8_t opacity = 255);
    
    /**
     * @brief Batch rendering para múltiples sprites de la misma textura
     */
    GameResult<void> render_sprite_batch(const std::string& texture_name,
                                        const std::vector<RenderCommand>& commands);

    // === TEXT RENDERING ===
    
    /**
     * @brief Renderiza texto en posición específica
     * @param text Texto a renderizar
     * @param position Posición en coordenadas pixel  
     * @param font_name Nombre de la fuente ("big", "small", etc.)
     * @param color Color del texto (R, G, B)
     * @return GameResult indicando éxito o error
     */
    GameResult<void> render_text(const std::string& text,
                                const PixelCoord& position,
                                const std::string& font_name = "small",
                                uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);

    // === PARTICLE EFFECTS ===
    
    /**
     * @brief Crea y renderiza sistema de partículas
     * @param effect_type Tipo de efecto (explosion, pickup, etc.)
     * @param position Posición del efecto
     * @param intensity Intensidad del efecto (0.0-1.0)
     * @return GameResult indicando éxito o error
     */
    GameResult<void> render_particle_effect(const std::string& effect_type,
                                           const PixelCoord& position,
                                           float intensity = 1.0f);

    // === UTILITY FUNCTIONS ===
    
    /**
     * @brief Convierte coordenadas de screen a world
     */
    PixelCoord screen_to_world(const PixelCoord& screen_coord) const;
    
    /**
     * @brief Convierte coordenadas de world a screen
     */
    PixelCoord world_to_screen(const PixelCoord& world_coord) const;
    
    /**
     * @brief Verifica si una posición está visible en pantalla
     */
    bool is_position_visible(const PixelCoord& position) const;
    
    /**
     * @brief Obtiene bounds del viewport actual
     */
    struct ViewportBounds {
        int x, y, width, height;
    };
    ViewportBounds get_viewport_bounds() const;

    // === CONFIGURATION ===
    
    /**
     * @brief Actualiza configuración de rendering
     */
    void update_config(const RenderingConfig& config);
    
    /**
     * @brief Obtiene configuración actual
     */
    const RenderingConfig& get_config() const { return config; }
    
    /**
     * @brief Habilita/deshabilita debug overlays
     */
    void set_debug_mode(bool enabled);

    // === STATISTICS & DEBUGGING ===
    
    /**
     * @brief Obtiene estadísticas del frame anterior
     */
    const RenderingStats& get_frame_statistics() const { return stats; }
    
    /**
     * @brief Resetea contadores de estadísticas
     */
    void reset_statistics();
    
    /**
     * @brief Renderiza información de debug en pantalla
     */
    void render_debug_info();

    // === RESOURCE MANAGEMENT ===
    
    /**
     * @brief Pre-carga textura para evitar stalls durante rendering
     * @param texture_name Nombre de la textura
     * @return GameResult indicando éxito o error
     */
    GameResult<void> preload_texture(const std::string& texture_name);
    
    /**
     * @brief Libera textura de memoria
     * @param texture_name Nombre de la textura
     */
    void unload_texture(const std::string& texture_name);
    
    /**
     * @brief Obtiene información sobre textura cargada
     */
    struct TextureInfo {
        int width, height;
        size_t memory_usage;
        bool is_loaded;
    };
    TextureInfo get_texture_info(const std::string& texture_name) const;
    
    /**
     * @brief Obtiene acceso directo al GPUAcceleratedRenderer para compatibilidad legacy
     * @return Puntero al GPU renderer o nullptr si no está inicializado
     */
    GPUAcceleratedRenderer* get_gpu_renderer() const { 
        return gpu_renderer.get(); 
    }

private:
    // Subsistemas de rendering - RenderingFacade tiene ownership completo
    std::unique_ptr<GPUAcceleratedRenderer> gpu_renderer;
    std::unique_ptr<TextRenderer> text_renderer;
    std::unique_ptr<ParticleEffectsManager> particle_manager;
    
    // Estado interno
    RenderingConfig config;
    RenderingStats stats;
    bool initialized = false;
    bool frame_started = false;
    
    // Viewport info
    int screen_width = 800;
    int screen_height = 600;
    
    // Helper methods
    GameResult<void> initialize_gpu_renderer(SDL_Window* window);
    GameResult<void> initialize_text_renderer();
    GameResult<void> initialize_particle_manager();
    
    void update_statistics();
    void validate_rendering_state() const;
    
    // Error handling helpers
    GameResult<void> handle_gpu_error(const std::string& operation) const;
    GameResult<void> handle_text_error(const std::string& operation) const;
    GameResult<void> handle_particle_error(const std::string& operation) const;
};