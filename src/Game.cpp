#include "Game.h"
#include "Resources.h"
#include "Timer.h"
#include "MainMenuScreen.h"
#include "GameplayScreen.h"
#include "SettingsScreen.h"
#include "Controller_Keyboard.h"
#include "TextRenderer.h"

Game::Game() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        exit(1);
    }


    // Set OpenGL attributes BEFORE creating window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    window = SDL_CreateWindow("ClanBomber Modern", 800, 600, 
                             SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (!window) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    // No SDL_Renderer - causes conflicts with OpenGL context
    // Explosions will be converted to use OpenGL rendering
    renderer = nullptr;
    Timer::init();
    
    // Initialize TextRenderer for hybrid SDL3_ttf + OpenGL text rendering
    app.text_renderer = new TextRenderer();
    if (!app.text_renderer->initialize()) {
        SDL_Log("Failed to initialize TextRenderer, text will not be available");
        delete app.text_renderer;
        app.text_renderer = nullptr;
    } else {
        SDL_Log("TextRenderer initialized successfully");
    }

    SDL_Log("Attempting to initialize GPU renderer...");
    app.gpu_renderer = new GPUAcceleratedRenderer();
    if (!app.gpu_renderer->initialize(window, 800, 600)) {
        SDL_Log("Failed to initialize GPU renderer, falling back to software rendering");
        delete app.gpu_renderer;
        app.gpu_renderer = nullptr;
        // Podriamos crear un fallback a un SDL_Renderer en el futuro
        // Para pc menos potentes
        // Initialize Resources without OpenGL context (won't create GL textures)
        // Resources::init();
        SDL_Quit();
        exit(1);
    } else {
        SDL_Log("GPU accelerated renderer initialized successfully");
        // Note: shaders and particle system are already initialized by initialize()
        
        // Initialize GameContext now that all systems are ready
        app.initialize_game_context();
        
        // CRITICAL: Initialize Resources AFTER OpenGL context is ready
        Resources::init();  // Now OpenGL context exists
        
        // Register texture metadata for correct UV coordinate calculation
        SDL_Log("Registering texture metadata for GPU renderer...");
        Resources::register_gl_texture_metadata("maptiles", app.gpu_renderer);
        Resources::register_gl_texture_metadata("bomber_snake", app.gpu_renderer);
        Resources::register_gl_texture_metadata("bomber_tux", app.gpu_renderer);
        Resources::register_gl_texture_metadata("bomber_bsd", app.gpu_renderer);
        Resources::register_gl_texture_metadata("bomber_dull_blue", app.gpu_renderer);
        Resources::register_gl_texture_metadata("bomber_dull_green", app.gpu_renderer);
        Resources::register_gl_texture_metadata("bomber_dull_red", app.gpu_renderer);
        Resources::register_gl_texture_metadata("bomber_dull_yellow", app.gpu_renderer);
        Resources::register_gl_texture_metadata("bomber_spider", app.gpu_renderer);
        Resources::register_gl_texture_metadata("bombs", app.gpu_renderer);
        Resources::register_gl_texture_metadata("explosion", app.gpu_renderer);
        Resources::register_gl_texture_metadata("extras", app.gpu_renderer);
        SDL_Log("Texture metadata registration completed");
        
        SDL_Log("GPU renderer fully operational!");
    }
    
    // Load fonts for TextRenderer
    if (app.text_renderer) {
        const char* sdl_base_path = SDL_GetBasePath();
        std::string base_path = sdl_base_path ? sdl_base_path : "./";
        
        // Try to load DejaVu Sans font (common on Linux systems)
        if (app.text_renderer->load_font("big", base_path + "data/fonts/DejaVuSans-Bold.ttf", 28)) {
            SDL_Log("Loaded big font successfully");
        } else {
            SDL_Log("Failed to load big font, trying fallback...");
            // Fallback to system font if available
            if (app.text_renderer->load_font("big", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 28)) {
                SDL_Log("Loaded big font from system path");
            } else {
                SDL_Log("WARNING: No fonts available - text rendering will not work");
            }
        }
        
        if (app.text_renderer->load_font("small", base_path + "data/fonts/DejaVuSans-Bold.ttf", 18)) {
            SDL_Log("Loaded small font successfully");
        }
    }

    // font = Resources::get_font("big");
    // if (!font) {
    //     SDL_Log("Failed to load font");
    //     // Handle error...
    // }

    running = true;
    // Start with main menu (restored!)
    current_screen = new MainMenuScreen(app.text_renderer, app.gpu_renderer);
}

Game::~Game() {
    delete current_screen;
    
    // Cleanup TextRenderer
    if (app.text_renderer) {
        delete app.text_renderer;
        app.text_renderer = nullptr;
    }
    
    Resources::shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::run() {
    while (running) {
        Timer::tick();
        handle_events();
        update(Timer::time_elapsed());
        render();
    }
}

void Game::handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
        current_screen->handle_events(event);
    }
    Controller_Keyboard::update_keyboard_state();
}

void Game::update(float deltaTime) {
    current_screen->update(deltaTime);
    
    if (dynamic_cast<MainMenuScreen*>(current_screen)) {
        MainMenuScreen* menu = static_cast<MainMenuScreen*>(current_screen);
        if (menu->get_next_state() != GameState::MAIN_MENU) {
            change_screen(menu->get_next_state());
        }
    }
    else if (dynamic_cast<SettingsScreen*>(current_screen)) {
        SettingsScreen* settings = static_cast<SettingsScreen*>(current_screen);
        if (settings->get_next_state() != GameState::SETTINGS) {
            change_screen(settings->get_next_state());
        }
    }
    else if (dynamic_cast<GameplayScreen*>(current_screen)) {
        GameplayScreen* gameplay = static_cast<GameplayScreen*>(current_screen);
        if (gameplay->get_next_state() != GameState::GAMEPLAY) {
            change_screen(gameplay->get_next_state());
        }
    }
}

void Game::render() {
    // PURE OPENGL RENDERING: No SDL_Renderer at all
    
    if (app.gpu_renderer && app.gpu_renderer->is_ready()) {
        // Make sure we're in the right OpenGL context
        SDL_GL_MakeCurrent(window, app.gpu_renderer->gl_context);
        
        // Set up OpenGL state 
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Clear screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
        glClear(GL_COLOR_BUFFER_BIT);
    }

    if (current_screen) {
        current_screen->render(nullptr);  // Pass nullptr since no SDL_Renderer
    }

    if (app.gpu_renderer && app.gpu_renderer->is_ready()) {
        // Flush any remaining GPU batches 
        app.gpu_renderer->end_frame();
        
        // Present via OpenGL context swap
        SDL_GL_SwapWindow(window);
    }
}

void Game::change_screen(GameState next_state) {
    if (current_screen) {
        delete current_screen;
        current_screen = nullptr;
    }

    // FIXED: Cuando salimos de gameplay, desactivar contexto OpenGL completamente
    if (app.gpu_renderer && app.gpu_renderer->is_ready() && next_state != GameState::GAMEPLAY) {
        // Limpiar y desactivar contexto OpenGL para menús
        SDL_GL_MakeCurrent(window, app.gpu_renderer->gl_context);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glFinish();
        
        // Desactivar contexto OpenGL para que SDL tome control completo
        SDL_GL_MakeCurrent(window, nullptr);
    }

    if (next_state == GameState::GAMEPLAY) {
        current_screen = new GameplayScreen(&app);
    }
    else if (next_state == GameState::SETTINGS) {
        current_screen = new SettingsScreen(renderer);
    }
    else if (next_state == GameState::MAIN_MENU) {
        current_screen = new MainMenuScreen(app.text_renderer, app.gpu_renderer);
    }
    else if (next_state == GameState::QUIT) {
        running = false;
        current_screen = nullptr;
    }
}