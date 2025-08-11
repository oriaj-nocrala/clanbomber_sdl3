#include "GameplayScreen.h"
#include "Bomber.h"
#include "Timer.h"
#include "Controller_Keyboard.h"

GameplayScreen::GameplayScreen(ClanBomberApplication* app) : app(app) {
    init_game();
}

GameplayScreen::~GameplayScreen() {
    deinit_game();
}

void GameplayScreen::init_game() {
    frame_count = 0;
    frame_time = 0;
    fps = 0;
    pause_game = false;
    show_fps = false;

    app->map = new Map(app);
    app->map->load();

    Controller* controller = new Controller_Keyboard(0);
    Bomber* bomber = new Bomber(100, 100, Bomber::RED, controller, app);
    app->bomber_objects.push_back(bomber);
}

void GameplayScreen::deinit_game() {
    for (auto& obj : app->objects) {
        delete obj;
    }
    app->objects.clear();

    for (auto& bomber : app->bomber_objects) {
        delete bomber;
    }
    app->bomber_objects.clear();

    delete app->map;
    app->map = nullptr;

    // if (observer) {
    //     delete observer;
    //     observer = nullptr;
    // }
}

void GameplayScreen::handle_events(SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
            case SDLK_P:
                pause_game = !pause_game;
                break;
            case SDLK_F1:
                show_fps = !show_fps;
                break;
        }
    }
}

void GameplayScreen::update(float deltaTime) {
    if (pause_game) {
        return;
    }

    // observer->act();
    // if (observer->end_of_game_requested()) {
    //     // Handle end of game
    // }

    delete_some();
    act_all();

    frame_time += Timer::time_elapsed();
    if (frame_time > 2) {
        fps = (int)(frame_count / frame_time + 0.5f);
        frame_time = 0;
        frame_count = 0;
    }
    frame_count++;
}

void GameplayScreen::render(SDL_Renderer* renderer) {
    show_all();

    if (pause_game) {
        // Render pause message
    }

    if (show_fps) {
        // Render FPS
    }
}

void GameplayScreen::act_all() {
    for (auto& obj : app->objects) {
        obj->act(Timer::time_elapsed());
    }

    for (auto& bomber : app->bomber_objects) {
        bomber->act(Timer::time_elapsed());
    }
}

void GameplayScreen::delete_some() {
    app->objects.remove_if([](GameObject* obj) {
        if (obj->delete_me) {
            delete obj;
            return true;
        }
        return false;
    });

    app->bomber_objects.remove_if([](Bomber* bomber) {
        if (bomber->delete_me) {
            delete bomber;
            return true;
        }
        return false;
    });
}

void GameplayScreen::show_all() {
    if (app->map != nullptr) {
        app->map->show();
    }

    for (auto& obj : app->objects) {
        if (obj != nullptr) {
            obj->show();
        }
    }

    for (auto& bomber : app->bomber_objects) {
        if (bomber != nullptr) {
            bomber->show();
        }
    }
}