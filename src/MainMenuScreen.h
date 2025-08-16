#ifndef MAINMENUSCREEN_H
#define MAINMENUSCREEN_H

#include "Screen.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <string>
#include "GameState.h"

class TextRenderer;
class GPUAcceleratedRenderer;

class MainMenuScreen : public Screen {
public:
    MainMenuScreen(TextRenderer* text_renderer, GPUAcceleratedRenderer* gpu_renderer);
    ~MainMenuScreen();

    void handle_events(SDL_Event& event) override;
    void update(float deltaTime) override;
    void render(SDL_Renderer* renderer = nullptr) override;

    GameState get_next_state() const;

private:
    std::vector<std::string> menu_items;
    int selected_item;
    GameState next_state;
    TextRenderer* text_renderer;
    GPUAcceleratedRenderer* gpu_renderer;
};

#endif