#include "GameplayScreen.h"
#include "Bomber.h"
#include "Timer.h"
#include "Controller_Keyboard.h"
#include "GameConfig.h"
#include "Controller.h"
#include <algorithm>

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
    if (!app->map->any_valid_map()) {
        SDL_Log("No valid maps found.");
    }

    if (GameConfig::get_random_map_order()) {
        app->map->load_random_valid();
    } else {
        if (GameConfig::get_start_map() > app->map->get_map_count() - 1) {
            GameConfig::set_start_map(app->map->get_map_count() - 1);
        }
        app->map->load_next_valid(GameConfig::get_start_map());
    }

    if (GameConfig::get_random_positions()) {
        app->map->randomize_bomber_positions();
    }

    int j = 0;
    for (int i = 0; i < 8; i++) {
        if (GameConfig::bomber[i].is_enabled()) {
            CL_Vector pos = app->map->get_bomber_pos(j++);
            Controller* controller = Controller::create(static_cast<Controller::CONTROLLER_TYPE>(GameConfig::bomber[i].get_controller()));
            Bomber* bomber = new Bomber((int)(pos.x * 40), (int)(pos.y * 40), static_cast<Bomber::COLOR>(GameConfig::bomber[i].get_skin()), controller, app);
            bomber->set_name(GameConfig::bomber[i].get_name());
            bomber->set_team(GameConfig::bomber[i].get_team());
            bomber->set_number(i);
            app->bomber_objects.push_back(bomber);

            bomber->set_pos(350, 270);
            bomber->fly_to((int)(pos.x*40), (int)(pos.y*40), 300);
            bomber->get_controller()->deactivate();
        }
    }

    // Remove teams with only one player
    int team_count[] = {0, 0, 0, 0};
    for (int team = 0; team < 4; team++) {
        for (auto const& bomber : app->bomber_objects) {
            if (bomber->get_team() - 1 == team) {
                team_count[team]++;
            }
        }
    }
    for (auto const& bomber : app->bomber_objects) {
        if (bomber->get_team() != 0) {
            if (team_count[bomber->get_team() - 1] == 1) {
                bomber->set_team(0);
            }
        }
    }
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
    if (app->map != nullptr) {
        app->map->act();
    }

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
    std::vector<GameObject*> draw_list;
    for(auto const& value: app->objects) {
        draw_list.push_back(value);
    }
    for(auto const& value: app->bomber_objects) {
        draw_list.push_back(value);
    }

    std::sort(draw_list.begin(), draw_list.end(), [](GameObject* go1, GameObject* go2) {
        return go1->get_z() < go2->get_z();
    });

    if (app->map != nullptr) {
        app->map->refresh_holes();
    }

    bool drawn_map = false;
    for (auto& obj : draw_list) {
        if (app->map != nullptr && obj->get_z() >= Z_GROUND && !drawn_map) {
            app->map->show();
            drawn_map = true;
        }
        if (obj != nullptr) {
            obj->show();
        }
    }

    if (app->map != nullptr && !drawn_map) {
        app->map->show();
    }
}