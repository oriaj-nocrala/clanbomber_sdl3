#include "Game.h"
#include "Resources.h"
#include "Timer.h"
#include "MainMenuScreen.h"
#include "GameplayScreen.h"
#include "SettingsScreen.h"
#include "Controller_Keyboard.h"
#include "TextRenderer.h"
#include "ErrorHandling.h"
#include "RenderingFacade.h"
#include "GameContext.h"
#include "Controller_Joystick.h"

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
    
    // Initialize joystick system
    Controller_Joystick::initialize_joystick_system();
    
    // Initialize TextRenderer for hybrid SDL3_ttf + OpenGL text rendering
    app.text_renderer = new TextRenderer();
    if (!app.text_renderer->initialize()) {
        SDL_Log("Failed to initialize TextRenderer, text will not be available");
        delete app.text_renderer;
        app.text_renderer = nullptr;
    } else {
        SDL_Log("TextRenderer initialized successfully");
    }

    // REMOVED: Legacy GPU renderer - now RenderingFacade handles all rendering
    SDL_Log("Legacy GPU renderer removed - RenderingFacade will handle all rendering");
    
    // Initialize GameContext now that systems are ready
    app.initialize_game_context();
    
    // CRITICAL: Initialize RenderingFacade BEFORE Resources
    if (app.game_context && app.game_context->get_rendering_facade()) {
        RenderingFacade* facade = app.game_context->get_rendering_facade();
        auto init_result = facade->initialize(window, 800, 600);
        if (init_result.is_ok()) {
            SDL_Log("Game::Game() - RenderingFacade initialized successfully");
        } else {
            SDL_Log("Game::Game() - Failed to initialize RenderingFacade: %s (%s)", 
                init_result.get_error_message().c_str(),
                init_result.get_error_context().c_str());
        }
    }
    
    // CRITICAL: Initialize Resources AFTER OpenGL context is ready
    Resources::init();  // Now OpenGL context exists
    
    // CRITICAL: Register texture metadata for sprite atlases
    if (app.game_context && app.game_context->get_rendering_facade()) {
        RenderingFacade* facade = app.game_context->get_rendering_facade();
        GPUAcceleratedRenderer* gpu_renderer = facade->get_gpu_renderer();
        if (gpu_renderer) {
            Resources::register_gl_texture_metadata("maptiles", gpu_renderer);
            Resources::register_gl_texture_metadata("bomber_dull_red", gpu_renderer);
            Resources::register_gl_texture_metadata("bomber_dull_blue", gpu_renderer);
            Resources::register_gl_texture_metadata("bomber_dull_yellow", gpu_renderer);
            Resources::register_gl_texture_metadata("bomber_dull_green", gpu_renderer);
            Resources::register_gl_texture_metadata("bomber_snake", gpu_renderer);
            Resources::register_gl_texture_metadata("bomber_tux", gpu_renderer);
            Resources::register_gl_texture_metadata("bomber_spider", gpu_renderer);
            Resources::register_gl_texture_metadata("bomber_bsd", gpu_renderer);
            Resources::register_gl_texture_metadata("bombs", gpu_renderer);
            Resources::register_gl_texture_metadata("explosion", gpu_renderer);
            Resources::register_gl_texture_metadata("extras", gpu_renderer);
            SDL_Log("Texture metadata registered for sprite atlases");
        } else {
            SDL_Log("WARNING: No GPU renderer available for texture metadata registration");
        }
    }
    
    SDL_Log("All rendering systems operational!");
    
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
    current_screen = new MainMenuScreen(app.text_renderer, app.game_context);
}

Game::~Game() {
    delete current_screen;
    
    // Cleanup TextRenderer
    if (app.text_renderer) {
        delete app.text_renderer;
        app.text_renderer = nullptr;
    }
    
    Resources::shutdown();
    Controller_Joystick::shutdown_joystick_system();
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
    // UNIFIED RENDERING: All rendering through RenderingFacade
    
    // Initialize and begin RenderingFacade frame
    if (app.game_context && app.game_context->get_rendering_facade()) {
        RenderingFacade* facade = app.game_context->get_rendering_facade();
        
        // RenderingFacade is already initialized during Game construction
        // Begin frame for RenderingFacade
            facade->begin_frame();
            
            if (current_screen) {
                current_screen->render(nullptr);  // All rendering goes through RenderingFacade
            }
            
            // End frame and present
            facade->end_frame();
            
            // Present the frame via OpenGL context swap
            SDL_GL_SwapWindow(window);
    } else {
        SDL_Log("WARNING: No RenderingFacade available - cannot render");
    }
}

void Game::change_screen(GameState next_state) {
    if (current_screen) {
        delete current_screen;
        current_screen = nullptr;
    }

    // REMOVED: Legacy GPU renderer context management - handled by RenderingFacade
    // OpenGL context is now managed entirely by RenderingFacade
    SDL_Log("Game::change_screen() - OpenGL context managed by RenderingFacade");

    if (next_state == GameState::GAMEPLAY) {
        current_screen = new GameplayScreen(&app);
    }
    else if (next_state == GameState::SETTINGS) {
        current_screen = new SettingsScreen(renderer);
    }
    else if (next_state == GameState::MAIN_MENU) {
        current_screen = new MainMenuScreen(app.text_renderer, app.game_context);
    }
    else if (next_state == GameState::QUIT) {
        running = false;
        current_screen = nullptr;
    }
}