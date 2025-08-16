#ifndef RESOURCES_H
#define RESOURCES_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/gl.h>
#include <string>
#include <map>

#define NR_BOMBERSKINS 8

struct TextureInfo {
    GLuint gl_texture;  // OpenGL texture for GPU rendering
    int sprite_width;
    int sprite_height;
    std::string file_path;  // Store original file path for GL texture creation
};

class Resources {
public:
    static void init();
    static void shutdown();

    static TextureInfo* get_texture(const std::string& name);
    static GLuint get_gl_texture(const std::string& name);
    static TTF_Font* get_font(const std::string& name);
    static std::string load_shader_source(const std::string& path);
    static void register_gl_texture_metadata(const std::string& texture_name, class GPUAcceleratedRenderer* renderer);

private:
    static std::string base_path;
    static std::map<std::string, TextureInfo*> textures;
    static std::map<std::string, TTF_Font*> fonts;

    static TextureInfo* load_texture(const std::string& path, int sprite_width = 0, int sprite_height = 0);
    static TTF_Font* load_font(const std::string& path, int size);
};

#endif
