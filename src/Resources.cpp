#include "Resources.h"
#include "AudioMixer.h"
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <vector>

SDL_Renderer* Resources::renderer = nullptr;
std::string Resources::base_path;
std::map<std::string, TextureInfo*> Resources::textures;
std::map<std::string, Sound*> Resources::sounds;
std::map<std::string, TTF_Font*> Resources::fonts;

void Resources::init(SDL_Renderer* renderer) {
    Resources::renderer = renderer;

    const char* sdl_base_path = SDL_GetBasePath();
    if (sdl_base_path) {
        base_path = sdl_base_path;
    } else {
        std::cerr << "Error getting base path: " << SDL_GetError() << std::endl;
        base_path = "./";
    }

    // Load fonts
    fonts["big"] = load_font("data/fonts/DejaVuSans-Bold.ttf", 28);
    fonts["small"] = load_font("data/fonts/DejaVuSans-Bold.ttf", 18);

    // Load textures
    textures["titlescreen"] = load_texture("data/pics/clanbomber_title_andi.png");
    textures["fl_logo"] = load_texture("data/pics/fischlustig_logo.png");
    textures["ps_teams"] = load_texture("data/pics/ps_teams.png", 125, 56);
    textures["ps_controls"] = load_texture("data/pics/ps_controls.png", 125, 56);
    textures["ps_teamlamps"] = load_texture("data/pics/ps_teamlamps.png", 30, 32);
    textures["playersetup_background"] = load_texture("data/pics/playersetup.png");
    textures["mapselector_background"] = load_texture("data/pics/level_selection.png");
    textures["mapselector_not_available"] = load_texture("data/pics/not_available.png");
    textures["gamestatus_tools"] = load_texture("data/pics/cup2.png", 40, 40);
    textures["gamestatus_background"] = load_texture("data/pics/game_status.png");
    textures["horst_evil"] = load_texture("data/pics/horst_evil.png");
    textures["bomber_snake"] = load_texture("data/pics/bomber_snake.png", 40, 60);
    textures["bomber_tux"] = load_texture("data/pics/bomber_tux.png", 40, 60);
    textures["bomber_spider"] = load_texture("data/pics/bomber_spider.png", 40, 40);
    textures["bomber_bsd"] = load_texture("data/pics/bomber_bsd.png", 40, 60);
    textures["bomber_dull_red"] = load_texture("data/pics/bomber_dull_red.png", 40, 40);
    textures["bomber_dull_blue"] = load_texture("data/pics/bomber_dull_blue.png", 40, 40);
    textures["bomber_dull_yellow"] = load_texture("data/pics/bomber_dull_yellow.png", 40, 40);
    textures["bomber_dull_green"] = load_texture("data/pics/bomber_dull_green.png", 40, 40);
    textures["observer"] = load_texture("data/pics/observer.png", 40, 40);
    textures["maptiles"] = load_texture("data/pics/maptiles.png", 40, 40);
    textures["maptile_addons"] = load_texture("data/pics/maptile_addons.png", 40, 40);
    textures["bombs"] = load_texture("data/pics/bombs.png", 40, 40);
    textures["explosion"] = load_texture("data/pics/explosion2.png", 40, 40);
    textures["cb_logo_small"] = load_texture("data/pics/cb_logo_small.png");
    textures["map_editor_background"] = load_texture("data/pics/map_editor.png");
    textures["corpse_parts"] = load_texture("data/pics/corpse_parts.png", 40, 40);

    // Initialize audio mixer
    AudioMixer::init();
    
    // Load all game sounds with error checking
    const std::vector<std::string> sound_files = {
        "typewriter", "winlevel", "klatsch", "forward", "rewind", "stop",
        "wow", "joint", "horny", "schnief", "whoosh", "break", "clear",
        "menu_back", "hurry_up", "time_over", "crunch", "die", "explode",
        "putbomb", "deepfall", "corpse_explode", "splash1", "splash2"
    };
    
    for (const std::string& sound_name : sound_files) {
        std::string file_path = "data/wavs/" + sound_name + ".wav";
        // Handle special cases
        if (sound_name == "splash1") file_path = "data/wavs/splash1a.wav";
        if (sound_name == "splash2") file_path = "data/wavs/splash2a.wav";
        
        // Load with AudioMixer
        std::string full_path = base_path + file_path;
        MixerAudio* mixer_audio = AudioMixer::load_sound(full_path);
        if (mixer_audio) {
            AudioMixer::add_sound(sound_name, mixer_audio);
        }
        
        // Also load with old system for compatibility
        Sound* sound = load_sound(file_path);
        if (sound) {
            sounds[sound_name] = sound;
        }
    }
}

void Resources::shutdown() {
    AudioMixer::shutdown();
    
    for (auto const& [key, val] : textures) {
        SDL_DestroyTexture(val->texture);
        delete val;
    }
    textures.clear();

    for (auto const& [key, val] : sounds) {
        SDL_free(val->buffer);
        delete val;
    }
    sounds.clear();

    for (auto const& [key, val] : fonts) {
        TTF_CloseFont(val);
    }
    fonts.clear();
}

TextureInfo* Resources::load_texture(const std::string& path, int sprite_width, int sprite_height) {
    std::string full_path = base_path + path;
    SDL_Texture* texture = IMG_LoadTexture(renderer, full_path.c_str());
    if (!texture) {
        std::cerr << "Failed to load texture: " << full_path << " - " << SDL_GetError() << std::endl;
        return nullptr;
    }
    TextureInfo* tex_info = new TextureInfo();
    tex_info->texture = texture;
    tex_info->sprite_width = sprite_width;
    tex_info->sprite_height = sprite_height;
    return tex_info;
}

Sound* Resources::load_sound(const std::string& path) {
    std::string full_path = base_path + path;
    Sound* sound = new Sound();
    if (!SDL_LoadWAV(full_path.c_str(), &sound->spec, &sound->buffer, &sound->length)) {
        std::cerr << "Failed to load sound: " << full_path << " - " << SDL_GetError() << std::endl;
        delete sound;
        return nullptr;
    }
    return sound;
}

TTF_Font* Resources::load_font(const std::string& path, int size) {
    std::string full_path = base_path + path;
    TTF_Font* font = TTF_OpenFont(full_path.c_str(), size);
    if (!font) {
        std::cerr << "Failed to load font: " << full_path << " - " << SDL_GetError() << std::endl;
    }
    return font;
}

TextureInfo* Resources::get_texture(const std::string& name) {
    if (textures.count(name)) {
        return textures[name];
    }
    return nullptr;
}

Sound* Resources::get_sound(const std::string& name) {
    if (sounds.count(name)) {
        return sounds[name];
    }
    return nullptr;
}

TTF_Font* Resources::get_font(const std::string& name) {
    if (fonts.count(name)) {
        return fonts[name];
    }
    return nullptr;
}

SDL_Renderer* Resources::get_renderer() {
    return renderer;
}
