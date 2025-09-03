#include "Resources.h"
#include "AudioMixer.h"
#include "GPUAcceleratedRenderer.h"
#include "CoordinateSystem.h"
#include <SDL3_image/SDL_image.h>
#include <glad/gl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

// Phase 3: Import CoordinateConfig constants
static constexpr int TILE_SIZE = CoordinateConfig::TILE_SIZE;

std::string Resources::base_path;
std::map<std::string, TextureInfo*> Resources::textures;
std::map<std::string, TTF_Font*> Resources::fonts;

void Resources::init() {

    const char* sdl_base_path = SDL_GetBasePath();
    if (sdl_base_path) {
        base_path = sdl_base_path;
    } else {
        std::cerr << "Error getting base path: " << SDL_GetError() << std::endl;
        base_path = "./";
    }

    // Note: Fonts are now loaded by TextRenderer

    // Load textures
    textures["titlescreen"] = load_texture("data/pics/clanbomber_title_andi.png");
    textures["fl_logo"] = load_texture("data/pics/fischlustig_logo.png");
    textures["ps_teams"] = load_texture("data/pics/ps_teams.png", 125, 56);
    textures["ps_controls"] = load_texture("data/pics/ps_controls.png", 125, 56);
    textures["ps_teamlamps"] = load_texture("data/pics/ps_teamlamps.png", 30, 32);
    textures["playersetup_background"] = load_texture("data/pics/playersetup.png");
    textures["mapselector_background"] = load_texture("data/pics/level_selection.png");
    textures["mapselector_not_available"] = load_texture("data/pics/not_available.png");
    textures["gamestatus_tools"] = load_texture("data/pics/cup2.png", TILE_SIZE, TILE_SIZE);
    textures["gamestatus_background"] = load_texture("data/pics/game_status.png");
    textures["horst_evil"] = load_texture("data/pics/horst_evil.png");
    textures["bomber_snake"] = load_texture("data/pics/bomber_snake.png", 40, 60);
    textures["bomber_tux"] = load_texture("data/pics/bomber_tux.png", 40, 60);
    textures["bomber_spider"] = load_texture("data/pics/bomber_spider.png", TILE_SIZE, TILE_SIZE);
    textures["bomber_bsd"] = load_texture("data/pics/bomber_bsd.png", TILE_SIZE, 60);
    textures["bomber_dull_red"] = load_texture("data/pics/bomber_dull_red.png", TILE_SIZE, TILE_SIZE);
    textures["bomber_dull_blue"] = load_texture("data/pics/bomber_dull_blue.png", TILE_SIZE, TILE_SIZE);
    textures["bomber_dull_yellow"] = load_texture("data/pics/bomber_dull_yellow.png", TILE_SIZE, TILE_SIZE);
    textures["bomber_dull_green"] = load_texture("data/pics/bomber_dull_green.png", TILE_SIZE, TILE_SIZE);
    textures["observer"] = load_texture("data/pics/observer.png", TILE_SIZE, TILE_SIZE);
    textures["maptiles"] = load_texture("data/pics/maptiles.png", TILE_SIZE, TILE_SIZE);
    textures["maptile_addons"] = load_texture("data/pics/maptile_addons.png", TILE_SIZE, TILE_SIZE);
    textures["bombs"] = load_texture("data/pics/bombs.png", TILE_SIZE, TILE_SIZE);
    textures["explosion"] = load_texture("data/pics/explosion2.png", TILE_SIZE, TILE_SIZE);
    SDL_Log("DEBUG: Loaded explosion texture, pointer: %p", textures["explosion"]);
    
    // Load power-up textures (extras2_X)
    textures["extras2_0"] = load_texture("data/pics/extras2_0.png", TILE_SIZE, TILE_SIZE);  // BOMB
    textures["extras2_1"] = load_texture("data/pics/extras2_1.png", TILE_SIZE, TILE_SIZE);  // FLAME
    textures["extras2_2"] = load_texture("data/pics/extras2_2.png", TILE_SIZE, TILE_SIZE);  // SPEED
    textures["extras2_3"] = load_texture("data/pics/extras2_3.png", TILE_SIZE, TILE_SIZE);  // KICK
    textures["extras2_4"] = load_texture("data/pics/extras2_4.png", TILE_SIZE, TILE_SIZE);  // GLOVE
    textures["extras2_5"] = load_texture("data/pics/extras2_5.png", TILE_SIZE, TILE_SIZE);  // SKATE
    textures["extras2_6"] = load_texture("data/pics/extras2_6.png", TILE_SIZE, TILE_SIZE);  // DISEASE
    textures["extras2_7"] = load_texture("data/pics/extras2_7.png", TILE_SIZE, TILE_SIZE);  // KOKS
    textures["extras2_8"] = load_texture("data/pics/extras2_8.png", TILE_SIZE, TILE_SIZE);  // VIAGRA
    textures["cb_logo_small"] = load_texture("data/pics/cb_logo_small.png");
    textures["map_editor_background"] = load_texture("data/pics/map_editor.png");
    textures["corpse_parts"] = load_texture("data/pics/corpse_parts.png", TILE_SIZE, TILE_SIZE);

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
    }
}

void Resources::shutdown() {
    AudioMixer::shutdown();
    
    for(auto& pair : textures) {
        if (pair.second) {
            if (pair.second->gl_texture != 0) {
                glDeleteTextures(1, &pair.second->gl_texture);
            }
            delete pair.second;
        }
    }
    textures.clear();

    // Note: Fonts are now managed by TextRenderer
    fonts.clear();
}

TextureInfo* Resources::load_texture(const std::string& path, int sprite_width, int sprite_height) {
    std::string full_path = base_path + path;
    
    // CRITICAL FIX: Load as surface only, no SDL_Renderer needed
    SDL_Surface* surface = IMG_Load(full_path.c_str());
    if (!surface) {
        std::cerr << "Failed to load surface: " << full_path << " - " << SDL_GetError() << std::endl;
        return nullptr;
    }
    
    TextureInfo* tex_info = new TextureInfo();
    tex_info->sprite_width = sprite_width;
    tex_info->sprite_height = sprite_height;
    tex_info->file_path = path; // Store path for GL texture creation
    
    // IMMEDIATE OpenGL texture creation from surface
    glGenTextures(1, &tex_info->gl_texture);
    glBindTexture(GL_TEXTURE_2D, tex_info->gl_texture);
    
    // Convert surface to consistent RGBA format for OpenGL
    SDL_Surface* rgba_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);
    
    if (!rgba_surface) {
        std::cerr << "Failed to convert surface to RGBA: " << SDL_GetError() << std::endl;
        delete tex_info;
        return nullptr;
    }
    
    // Now we know format is RGBA
    GLenum format = GL_RGBA;
    GLenum internal_format = GL_RGBA;
    
    // Create OpenGL texture from converted surface data
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, rgba_surface->w, rgba_surface->h, 0, 
                format, GL_UNSIGNED_BYTE, rgba_surface->pixels);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    SDL_DestroySurface(rgba_surface);
    
    return tex_info;
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
    if (name == "explosion") {
        SDL_Log("DEBUG: get_texture called for 'explosion', count=%zu, ptr=%p", textures.count(name), textures.count(name) ? textures[name] : nullptr);
    }
    if (textures.count(name)) {
        return textures[name];
    }
    return nullptr;
}

GLuint Resources::get_gl_texture(const std::string& name) {
    TextureInfo* tex_info = get_texture(name);
    if (!tex_info) {
        return 0;
    }
    
    
    // Create OpenGL texture from SDL texture if not already created
    if (tex_info->gl_texture == 0) {
        // FIXED: Load as surface to get proper format information
        std::string full_path = base_path + tex_info->file_path;
        SDL_Surface* surface = IMG_Load(full_path.c_str());
        
        if (surface) {
            glGenTextures(1, &tex_info->gl_texture);
            glBindTexture(GL_TEXTURE_2D, tex_info->gl_texture);
            
            // Use original format but determine correct OpenGL format
            GLenum format = GL_RGBA;
            GLenum internal_format = GL_RGBA;
            
            // Always use RGBA for simplicity since IMG_Load returns RGBA for PNGs
            format = GL_RGBA;
            internal_format = GL_RGBA;
            
            glTexImage2D(GL_TEXTURE_2D, 0, internal_format, surface->w, surface->h, 0, 
                        format, GL_UNSIGNED_BYTE, surface->pixels);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            SDL_DestroySurface(surface);
        } else if (!surface) {
            // TODO: Handle error case
        }
    }
    
    return tex_info->gl_texture;
}

std::string Resources::load_shader_source(const std::string& path) {
    std::string full_path = base_path + path;
    std::ifstream file(full_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << full_path << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Resources::register_gl_texture_metadata(const std::string& texture_name, GPUAcceleratedRenderer* renderer) {
    if (!renderer) return;
    
    TextureInfo* tex_info = get_texture(texture_name);
    if (!tex_info || tex_info->gl_texture == 0) return;
    
    // Load surface to get dimensions
    std::string full_path = base_path + tex_info->file_path;
    SDL_Surface* surface = IMG_Load(full_path.c_str());
    if (!surface) return;
    
    // Register metadata with GPU renderer
    int sprite_width = (tex_info->sprite_width > 0) ? tex_info->sprite_width : TILE_SIZE;
    int sprite_height = (tex_info->sprite_height > 0) ? tex_info->sprite_height : TILE_SIZE;
    
    renderer->register_texture_metadata(tex_info->gl_texture, surface->w, surface->h, 
                                       sprite_width, sprite_height);
    
    SDL_DestroySurface(surface);
}
