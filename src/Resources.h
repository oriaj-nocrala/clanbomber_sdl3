#ifndef RESOURCES_H
#define RESOURCES_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <map>

#define NR_BOMBERSKINS 8

struct Sound {
    SDL_AudioSpec spec;
    Uint8* buffer;
    Uint32 length;
};

struct TextureInfo {
    SDL_Texture* texture;
    int sprite_width;
    int sprite_height;
};

class Resources {
public:
    static void init(SDL_Renderer* renderer);
    static void shutdown();

    static TextureInfo* get_texture(const std::string& name);
    static Sound* get_sound(const std::string& name);
    static TTF_Font* get_font(const std::string& name);
    static SDL_Renderer* get_renderer();

private:
    static std::string base_path;
    static SDL_Renderer* renderer;
    static std::map<std::string, TextureInfo*> textures;
    static std::map<std::string, Sound*> sounds;
    static std::map<std::string, TTF_Font*> fonts;

    static TextureInfo* load_texture(const std::string& path, int sprite_width = 0, int sprite_height = 0);
    static Sound* load_sound(const std::string& path);
    static TTF_Font* load_font(const std::string& path, int size);
};

#endif