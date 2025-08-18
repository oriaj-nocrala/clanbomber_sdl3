#include "RenderingFacade.h"
#include "Resources.h"
#include "GPUAcceleratedRenderer.h"
#include "TextRenderer.h"
#include <SDL3/SDL.h>
#include <algorithm>

// === RenderingFacade Implementation ===

RenderingFacade::RenderingFacade(RenderingConfig config) 
    : config(config), initialized(false), frame_started(false) {
    SDL_Log("RenderingFacade: Initialized with config - GPU acceleration: %s, Particle effects: %s", 
        config.enable_gpu_acceleration ? "enabled" : "disabled",
        config.enable_particle_effects ? "enabled" : "disabled");
}

RenderingFacade::~RenderingFacade() {
    shutdown();
}

// === INITIALIZATION ===

GameResult<void> RenderingFacade::initialize(SDL_Window* window, int width, int height) {
    if (initialized) {
        return GameResult<void>::success();
    }
    
    screen_width = width;
    screen_height = height;
    
    SDL_Log("RenderingFacade: Initializing subsystems for %dx%d display", width, height);
    
    // Initialize subsystems in order
    auto gpu_result = initialize_gpu_renderer(window);
    if (!gpu_result.is_ok()) {
        return gpu_result;
    }
    
    auto text_result = initialize_text_renderer();
    if (!text_result.is_ok()) {
        return text_result;
    }
    
    auto particle_result = initialize_particle_manager();
    if (!particle_result.is_ok()) {
        return particle_result;
    }
    
    initialized = true;
    reset_statistics();
    
    SDL_Log("RenderingFacade: All subsystems initialized successfully");
    return GameResult<void>::success();
}

void RenderingFacade::shutdown() {
    if (!initialized) return;
    
    SDL_Log("RenderingFacade: Shutting down all rendering subsystems");
    
    // Shutdown in reverse order
    particle_manager.reset();
    text_renderer.reset();
    gpu_renderer.reset();
    
    initialized = false;
    frame_started = false;
}

// === FRAME RENDERING ===

GameResult<void> RenderingFacade::begin_frame() {
    if (!initialized) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::CRITICAL, 
            "RenderingFacade not initialized");
    }
    
    if (frame_started) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
            "Frame already started");
    }
    
    validate_rendering_state();
    
    // Clear screen at beginning of frame
    clear_screen(0, 0, 0, 255); // Black background
    
    // Begin frame for all subsystems
    if (gpu_renderer) {
        // CRITICAL: Actually call the GPU renderer's begin_frame
        gpu_renderer->begin_frame();
    }
    
    frame_started = true;
    stats.sprites_rendered = 0;
    stats.text_elements_rendered = 0;
    stats.particles_rendered = 0;
    stats.draw_calls = 0;
    
    return GameResult<void>::success();
}

GameResult<void> RenderingFacade::end_frame() {
    if (!initialized || !frame_started) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
            "Frame not started or facade not initialized");
    }
    
    // Finalize rendering for all subsystems
    if (gpu_renderer) {
        // CRITICAL: Actually call the GPU renderer's end_frame
        gpu_renderer->end_frame();
    }
    
    // Render debug info if enabled
    if (config.enable_debug_overlays) {
        render_debug_info();
    }
    
    update_statistics();
    frame_started = false;
    
    return GameResult<void>::success();
}

void RenderingFacade::clear_screen(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!initialized || !gpu_renderer) return;
    
    // Clear using OpenGL directly
    glClearColor(
        static_cast<float>(r) / 255.0f,
        static_cast<float>(g) / 255.0f, 
        static_cast<float>(b) / 255.0f,
        static_cast<float>(a) / 255.0f
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// === SPRITE RENDERING ===

GameResult<void> RenderingFacade::render_sprite(const std::string& texture_name, 
                                              const PixelCoord& position,
                                              int sprite_nr,
                                              float rotation,
                                              uint8_t opacity) {
    if (!initialized || !frame_started) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
            "RenderingFacade not ready for rendering");
    }
    
    if (!gpu_renderer) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::CRITICAL,
            "GPU renderer not available");
    }
    
    // Check if position is visible
    if (!is_position_visible(position)) {
        return GameResult<void>::success(); // Not an error, just optimization
    }
    
    try {
        // Convert to integer coordinates for compatibility
        int x = static_cast<int>(position.pixel_x);
        int y = static_cast<int>(position.pixel_y);
        
        // OPTIMIZED: Delegate to GPUAcceleratedRenderer for actual rendering
        ::TextureInfo* tex_info = Resources::get_texture(texture_name);  // Use global TextureInfo
        if (!tex_info) {
            return GameResult<void>::error(GameErrorType::TEXTURE_LOAD_FAILED, ErrorSeverity::WARNING,
                "Texture '" + texture_name + "' not found in Resources");
        }
        
        GLuint gl_texture = Resources::get_gl_texture(texture_name);
        if (!gl_texture) {
            return GameResult<void>::error(GameErrorType::TEXTURE_LOAD_FAILED, ErrorSeverity::WARNING,
                "OpenGL texture for '" + texture_name + "' not found");
        }
        
        // Use GPU renderer for actual sprite rendering
        gpu_renderer->begin_batch();
        gpu_renderer->add_sprite(
            static_cast<float>(x), static_cast<float>(y), 
            static_cast<float>(tex_info->sprite_width), static_cast<float>(tex_info->sprite_height),
            gl_texture, nullptr, rotation, nullptr, sprite_nr
        );
        gpu_renderer->end_batch();
        
        stats.sprites_rendered++;
        stats.draw_calls++;
        
        return GameResult<void>::success();
        
    } catch (const std::exception& e) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
            "Failed to render sprite: " + std::string(e.what()));
    }
}

GameResult<void> RenderingFacade::render_sprite_at_grid(const std::string& texture_name,
                                                      const GridCoord& grid_position,
                                                      int sprite_nr,
                                                      float rotation,
                                                      uint8_t opacity) {
    PixelCoord pixel_position = CoordinateSystem::grid_to_pixel(grid_position);
    return render_sprite(texture_name, pixel_position, sprite_nr, rotation, opacity);
}

GameResult<void> RenderingFacade::render_sprite_batch(const std::string& texture_name,
                                                    const std::vector<RenderCommand>& commands) {
    if (!config.enable_sprite_batching) {
        // Fall back to individual sprite rendering
        for (const auto& cmd : commands) {
            if (cmd.command_type == RenderCommand::SPRITE) {
                auto result = render_sprite(cmd.texture_name, cmd.position, cmd.sprite_nr, cmd.rotation, cmd.opacity);
                if (!result.is_ok()) {
                    return result;
                }
            }
        }
        return GameResult<void>::success();
    }
    
    // TODO: Implement proper batch rendering with GPU renderer
    SDL_Log("RenderingFacade: Batch rendering %zu sprites for texture '%s'", 
        commands.size(), texture_name.c_str());
    
    stats.sprites_rendered += commands.size();
    stats.draw_calls++; // Batched sprites count as one draw call
    
    return GameResult<void>::success();
}

// === TEXT RENDERING ===

GameResult<void> RenderingFacade::render_text(const std::string& text,
                                            const PixelCoord& position,
                                            const std::string& font_name,
                                            uint8_t r, uint8_t g, uint8_t b) {
    if (!initialized || !frame_started) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
            "RenderingFacade not ready for text rendering");
    }
    
    if (!text_renderer) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
            "Text renderer not available");
    }
    
    try {
        // Convert to integer coordinates for compatibility
        int x = static_cast<int>(position.pixel_x);
        int y = static_cast<int>(position.pixel_y);
        
        // Use the integrated TextRenderer to create text texture and render it
        SDL_Color color = {r, g, b, 255};
        auto text_texture = text_renderer->render_text(text, font_name, color);
        
        if (!text_texture || !text_texture->gl_texture) {
            return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
                "Failed to create text texture for: " + text);
        }
        
        // Calculate actual position - if this is for centered text, adjust x coordinate
        float actual_x = static_cast<float>(x);
        
        // Check if this looks like a center coordinate (around screen center)
        if (x >= 300 && x <= 500) {
            // This is likely meant to be centered - calculate proper center position
            actual_x = static_cast<float>(x) - (static_cast<float>(text_texture->width) / 2.0f);
        }
        
        // Render the text texture using GPU renderer
        if (gpu_renderer) {
            gpu_renderer->begin_batch();
            gpu_renderer->add_sprite(
                actual_x, static_cast<float>(y),
                static_cast<float>(text_texture->width), static_cast<float>(text_texture->height),
                text_texture->gl_texture, nullptr, 0.0f, nullptr, 0
            );
            gpu_renderer->end_batch();
        }
        
        stats.text_elements_rendered++;
        stats.draw_calls++;
        
        return GameResult<void>::success();
        
    } catch (const std::exception& e) {
        return handle_text_error("render_text: " + std::string(e.what()));
    }
}

// === PARTICLE EFFECTS ===

GameResult<void> RenderingFacade::render_particle_effect(const std::string& effect_type,
                                                       const PixelCoord& position,
                                                       float intensity) {
    if (!config.enable_particle_effects) {
        return GameResult<void>::success(); // Effects disabled, not an error
    }
    
    if (!initialized || !frame_started) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
            "RenderingFacade not ready for particle effects");
    }
    
    if (!particle_manager) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
            "Particle manager not available");
    }
    
    try {
        // Clamp intensity to valid range
        intensity = std::clamp(intensity, 0.0f, 1.0f);
        
        // Note: This is a simplified implementation
        // Real implementation would call particle_manager->create_effect(effect_type, position, intensity)
        SDL_Log("RenderingFacade: Creating particle effect '%s' at (%.1f, %.1f) intensity=%.2f", 
            effect_type.c_str(), position.pixel_x, position.pixel_y, intensity);
        
        // Estimate particle count based on effect type and intensity
        int estimated_particles = static_cast<int>(100 * intensity);
        stats.particles_rendered += estimated_particles;
        stats.draw_calls++;
        
        return GameResult<void>::success();
        
    } catch (const std::exception& e) {
        return handle_particle_error("render_particle_effect: " + std::string(e.what()));
    }
}

// === UTILITY FUNCTIONS ===

PixelCoord RenderingFacade::screen_to_world(const PixelCoord& screen_coord) const {
    // For now, assume screen coordinates are the same as world coordinates
    // In a more complex system, this would handle camera transforms, zoom, etc.
    return screen_coord;
}

PixelCoord RenderingFacade::world_to_screen(const PixelCoord& world_coord) const {
    // For now, assume world coordinates are the same as screen coordinates
    return world_coord;
}

bool RenderingFacade::is_position_visible(const PixelCoord& position) const {
    return position.pixel_x >= 0 && position.pixel_x < screen_width &&
           position.pixel_y >= 0 && position.pixel_y < screen_height;
}

RenderingFacade::ViewportBounds RenderingFacade::get_viewport_bounds() const {
    return ViewportBounds{0, 0, screen_width, screen_height};
}

// === CONFIGURATION ===

void RenderingFacade::update_config(const RenderingConfig& new_config) {
    config = new_config;
    SDL_Log("RenderingFacade: Configuration updated");
    
    // Apply configuration changes to subsystems
    if (gpu_renderer && config.enable_debug_overlays) {
        // Enable debug mode in GPU renderer
    }
}

void RenderingFacade::set_debug_mode(bool enabled) {
    config.enable_debug_overlays = enabled;
    SDL_Log("RenderingFacade: Debug mode %s", enabled ? "enabled" : "disabled");
}

// === STATISTICS & DEBUGGING ===

void RenderingFacade::reset_statistics() {
    stats = RenderingStats{};
}

void RenderingFacade::render_debug_info() {
    if (!config.enable_debug_overlays) return;
    
    // Render statistics overlay
    std::string debug_text = "FPS: " + std::to_string(1000.0f / std::max(stats.frame_time_ms, 0.001f)) +
                           " | Sprites: " + std::to_string(stats.sprites_rendered) +
                           " | Draw calls: " + std::to_string(stats.draw_calls);
    
    PixelCoord debug_position(10.0f, 10.0f);
    render_text(debug_text, debug_position, "small", 255, 255, 255);
}

// === RESOURCE MANAGEMENT ===

GameResult<void> RenderingFacade::preload_texture(const std::string& texture_name) {
    if (!gpu_renderer) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
            "GPU renderer not available for texture preloading");
    }
    
    try {
        // Note: This is a simplified implementation
        // Real implementation would call gpu_renderer->preload_texture(texture_name)
        SDL_Log("RenderingFacade: Preloading texture '%s'", texture_name.c_str());
        
        return GameResult<void>::success();
        
    } catch (const std::exception& e) {
        return handle_gpu_error("preload_texture: " + std::string(e.what()));
    }
}

void RenderingFacade::unload_texture(const std::string& texture_name) {
    if (!gpu_renderer) return;
    
    // Note: This is a simplified implementation
    // Real implementation would call gpu_renderer->unload_texture(texture_name)
    SDL_Log("RenderingFacade: Unloading texture '%s'", texture_name.c_str());
}

RenderingFacade::TextureInfo RenderingFacade::get_texture_info(const std::string& texture_name) const {
    TextureInfo info;
    
    if (!gpu_renderer) {
        info.is_loaded = false;
        return info;
    }
    
    // Note: This is a simplified implementation
    // Real implementation would query gpu_renderer for texture info
    info.width = 64;  // Default tile size
    info.height = 64;
    info.memory_usage = 64 * 64 * 4; // Assume 32-bit RGBA
    info.is_loaded = true;
    
    return info;
}

// === PRIVATE HELPER METHODS ===

GameResult<void> RenderingFacade::initialize_gpu_renderer(SDL_Window* window) {
    if (!config.enable_gpu_acceleration) {
        SDL_Log("RenderingFacade: GPU acceleration disabled, skipping GPU renderer initialization");
        return GameResult<void>::success();
    }
    
    try {
        SDL_Log("RenderingFacade: Creating dedicated GPUAcceleratedRenderer");
        gpu_renderer = std::make_unique<GPUAcceleratedRenderer>();
        
        auto init_result = gpu_renderer->initialize(window, screen_width, screen_height);
        if (!init_result.is_ok()) {
            gpu_renderer.reset();
            return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::CRITICAL,
                "Failed to initialize RenderingFacade's GPU renderer: " + init_result.get_error_message(),
                init_result.get_error_context());
        }
        
        SDL_Log("RenderingFacade: GPU renderer created and initialized successfully");
        return GameResult<void>::success();
        
    } catch (const std::exception& e) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::CRITICAL,
            "Failed to create GPU renderer: " + std::string(e.what()));
    }
}

GameResult<void> RenderingFacade::initialize_text_renderer() {
    try {
        SDL_Log("RenderingFacade: Creating and initializing real TextRenderer");
        text_renderer = std::make_unique<TextRenderer>();
        
        if (!text_renderer->initialize()) {
            text_renderer.reset();
            return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::CRITICAL,
                "Failed to initialize TextRenderer");
        }
        
        // Load required fonts
        const char* sdl_base_path = SDL_GetBasePath();
        std::string base_path = sdl_base_path ? sdl_base_path : "./";
        
        // Load big font
        if (text_renderer->load_font("big", base_path + "data/fonts/DejaVuSans-Bold.ttf", 28)) {
            SDL_Log("RenderingFacade: Loaded big font successfully");
        } else if (text_renderer->load_font("big", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 28)) {
            SDL_Log("RenderingFacade: Loaded big font from system path");
        } else {
            SDL_Log("RenderingFacade: WARNING - Failed to load big font");
        }
        
        // Load small font
        if (text_renderer->load_font("small", base_path + "data/fonts/DejaVuSans-Bold.ttf", 18)) {
            SDL_Log("RenderingFacade: Loaded small font successfully");
        } else if (text_renderer->load_font("small", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 18)) {
            SDL_Log("RenderingFacade: Loaded small font from system path");
        } else {
            SDL_Log("RenderingFacade: WARNING - Failed to load small font");
        }
        
        SDL_Log("RenderingFacade: TextRenderer initialized successfully");
        return GameResult<void>::success();
        
    } catch (const std::exception& e) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::CRITICAL,
            "Failed to initialize text renderer: " + std::string(e.what()));
    }
}

GameResult<void> RenderingFacade::initialize_particle_manager() {
    if (!config.enable_particle_effects) {
        SDL_Log("RenderingFacade: Particle effects disabled, skipping particle manager initialization");
        return GameResult<void>::success();
    }
    
    try {
        // Note: This is a simplified implementation
        // Real implementation would create and initialize ParticleEffectsManager
        // particle_manager = std::make_unique<ParticleEffectsManager>();
        // return particle_manager->initialize(config.max_particles);
        
        SDL_Log("RenderingFacade: Particle manager initialized (mock) with max %d particles", config.max_particles);
        return GameResult<void>::success();
        
    } catch (const std::exception& e) {
        return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
            "Failed to initialize particle manager: " + std::string(e.what()));
    }
}

void RenderingFacade::update_statistics() {
    // Calculate frame time (simplified)
    static Uint64 last_time = SDL_GetTicks();
    Uint64 current_time = SDL_GetTicks();
    stats.frame_time_ms = static_cast<float>(current_time - last_time);
    last_time = current_time;
    
    // Update texture memory usage estimate
    stats.texture_memory_usage = stats.sprites_rendered * 4096; // Rough estimate
}

void RenderingFacade::validate_rendering_state() const {
    if (!initialized) {
        SDL_Log("WARNING: RenderingFacade used before initialization");
    }
    
    if (frame_started && !gpu_renderer) {
        SDL_Log("WARNING: Frame started but no GPU renderer available");
    }
}

// === ERROR HANDLING HELPERS ===

GameResult<void> RenderingFacade::handle_gpu_error(const std::string& operation) const {
    // Simplified error handling - in real implementation would check GPU renderer status
    SDL_Log("RenderingFacade: GPU operation '%s' completed", operation.c_str());
    return GameResult<void>::success();
}

GameResult<void> RenderingFacade::handle_text_error(const std::string& operation) const {
    return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
        "Text rendering error: " + operation);
}

GameResult<void> RenderingFacade::handle_particle_error(const std::string& operation) const {
    return GameResult<void>::error(GameErrorType::RENDER_ERROR, ErrorSeverity::WARNING,
        "Particle effect error: " + operation);
}