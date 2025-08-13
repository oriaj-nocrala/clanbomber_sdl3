#include "Game.h"
#include "Resources.h"
#include "Timer.h"
#include "MainMenuScreen.h"
#include "GameplayScreen.h"
#include "SettingsScreen.h"
#include "Controller_Keyboard.h"

Game::Game() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        exit(1);
    }

    if (TTF_Init() == -1) {
        SDL_Log("Unable to initialize SDL_ttf: %s", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    window = SDL_CreateWindow("ClanBomber Modern", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        exit(1);
    }
    
    SDL_SetRenderVSync(renderer, 1);

    Resources::init(renderer);
    Timer::init();

    font = TTF_OpenFont("data/fonts/DejaVuSans-Bold.ttf", 28);
    if (!font) {
        SDL_Log("Failed to load font: %s", SDL_GetError());
        // Handle error...
    }

    running = true;
    current_screen = new MainMenuScreen(renderer, font);
}

Game::~Game() {
    delete current_screen;
    TTF_CloseFont(font);
    Resources::shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
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
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (current_screen) {
        current_screen->render(renderer);
    }

    SDL_RenderPresent(renderer);
}

void Game::change_screen(GameState next_state) {
    if (current_screen) {
        delete current_screen;
        current_screen = nullptr;
    }

    if (next_state == GameState::GAMEPLAY) {
        current_screen = new GameplayScreen(&app);
    }
    else if (next_state == GameState::SETTINGS) {
        current_screen = new SettingsScreen(renderer, font);
    }
    else if (next_state == GameState::MAIN_MENU) {
        current_screen = new MainMenuScreen(renderer, font);
    }
    else if (next_state == GameState::QUIT) {
        running = false;
        current_screen = nullptr;
    }
}