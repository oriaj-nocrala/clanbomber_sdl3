#include "GameplayScreen.h"
#include "Bomber.h"

GameplayScreen::GameplayScreen(ClanBomberApplication* app) : app(app) {
    app->map = new Map(app);
    app->map->load();

    controller = new Controller_Keyboard(0);
    Bomber* bomber = new Bomber(100, 100, Bomber::RED, controller, app);
    app->objects.push_back(bomber);
}

GameplayScreen::~GameplayScreen() {
    delete app->map;
    delete controller;
    // The objects in app->objects will be deleted by the ClanBomberApplication destructor
}

void GameplayScreen::handle_events(SDL_Event& event) {
    // Gameplay specific event handling
}

void GameplayScreen::update(float deltaTime) {
    for (auto& obj : app->objects) {
        obj->act(deltaTime);
    }

    app->objects.remove_if([](GameObject* obj){
        if (obj->delete_me) {
            delete obj;
            return true;
        }
        return false;
    });
}

void GameplayScreen::render(SDL_Renderer* renderer) {
    app->map->show();
    for (auto& obj : app->objects) {
        obj->show();
    }
}
