#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/gl.h>
#include <string>
#include <unordered_map>
#include <memory>

// Forward declarations
class RenderingFacade;
class GameContext;

struct TextTexture {
    GLuint gl_texture;
    int width;
    int height;
    std::string text;
    SDL_Color color;
    TTF_Font* font;
    
    TextTexture() : gl_texture(0), width(0), height(0), font(nullptr) {}
    ~TextTexture();
};

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();
    
    bool initialize();
    void shutdown();
    
    // Font management
    bool load_font(const std::string& name, const std::string& path, int size);
    TTF_Font* get_font(const std::string& name);
    
    // Text rendering
    std::shared_ptr<TextTexture> render_text(const std::string& text, const std::string& font_name, SDL_Color color);
    
    // Render text directly to screen using RenderingFacade
    void draw_text(RenderingFacade* rendering_facade, const std::string& text, 
                   const std::string& font_name, float x, float y, SDL_Color color);
    
    // Render centered text using RenderingFacade
    void draw_text_centered(RenderingFacade* rendering_facade, const std::string& text, 
                           const std::string& font_name, float center_x, float y, SDL_Color color);
    
private:
    std::unordered_map<std::string, TTF_Font*> fonts;
    std::unordered_map<std::string, std::shared_ptr<TextTexture>> text_cache;
    
    GLuint create_gl_texture_from_surface(SDL_Surface* surface);
    std::string make_cache_key(const std::string& text, const std::string& font_name, SDL_Color color);
    
    bool ttf_initialized;
};