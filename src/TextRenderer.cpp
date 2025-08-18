#include "TextRenderer.h"
#include "RenderingFacade.h"
#include "CoordinateSystem.h"
#include <iostream>
#include <sstream>

TextTexture::~TextTexture() {
    if (gl_texture) {
        glDeleteTextures(1, &gl_texture);
    }
}

TextRenderer::TextRenderer() : ttf_initialized(false) {
}

TextRenderer::~TextRenderer() {
    shutdown();
}

bool TextRenderer::initialize() {
    if (TTF_Init() < 0) {
        std::cerr << "Failed to initialize SDL_ttf: " << SDL_GetError() << std::endl;
        return false;
    }
    
    ttf_initialized = true;
    SDL_Log("TextRenderer: SDL_ttf initialized successfully");
    return true;
}

void TextRenderer::shutdown() {
    // Clear text cache (automatically deletes TextTextures)
    text_cache.clear();
    
    // Close all fonts
    for (auto& pair : fonts) {
        if (pair.second) {
            TTF_CloseFont(pair.second);
        }
    }
    fonts.clear();
    
    if (ttf_initialized) {
        TTF_Quit();
        ttf_initialized = false;
    }
}

bool TextRenderer::load_font(const std::string& name, const std::string& path, int size) {
    TTF_Font* font = TTF_OpenFont(path.c_str(), size);
    if (!font) {
        std::cerr << "Failed to load font " << path << ": " << SDL_GetError() << std::endl;
        return false;
    }
    
    fonts[name] = font;
    SDL_Log("TextRenderer: Loaded font '%s' from %s (size %d)", name.c_str(), path.c_str(), size);
    return true;
}

TTF_Font* TextRenderer::get_font(const std::string& name) {
    auto it = fonts.find(name);
    if (it != fonts.end()) {
        return it->second;
    }
    return nullptr;
}

GLuint TextRenderer::create_gl_texture_from_surface(SDL_Surface* surface) {
    if (!surface) return 0;
    
    // Convert to RGBA format (same as we do for images)
    SDL_Surface* rgba_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    if (!rgba_surface) {
        std::cerr << "Failed to convert text surface to RGBA: " << SDL_GetError() << std::endl;
        return 0;
    }
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba_surface->w, rgba_surface->h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, rgba_surface->pixels);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    SDL_DestroySurface(rgba_surface);
    return texture;
}

std::string TextRenderer::make_cache_key(const std::string& text, const std::string& font_name, SDL_Color color) {
    std::stringstream ss;
    ss << text << "|" << font_name << "|" << (int)color.r << "," << (int)color.g << "," << (int)color.b << "," << (int)color.a;
    return ss.str();
}

std::shared_ptr<TextTexture> TextRenderer::render_text(const std::string& text, const std::string& font_name, SDL_Color color) {
    // Check cache first
    std::string cache_key = make_cache_key(text, font_name, color);
    auto cache_it = text_cache.find(cache_key);
    if (cache_it != text_cache.end()) {
        return cache_it->second;
    }
    
    // Get font
    TTF_Font* font = get_font(font_name);
    if (!font) {
        std::cerr << "Font not found: " << font_name << std::endl;
        return nullptr;
    }
    
    // Render text to surface (SDL3_ttf needs string length)
    SDL_Surface* text_surface = TTF_RenderText_Solid(font, text.c_str(), text.length(), color);
    if (!text_surface) {
        std::cerr << "Failed to render text: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    
    // Create OpenGL texture
    GLuint gl_texture = create_gl_texture_from_surface(text_surface);
    if (!gl_texture) {
        SDL_DestroySurface(text_surface);
        return nullptr;
    }
    
    // Create TextTexture object
    auto text_texture = std::make_shared<TextTexture>();
    text_texture->gl_texture = gl_texture;
    text_texture->width = text_surface->w;
    text_texture->height = text_surface->h;
    text_texture->text = text;
    text_texture->color = color;
    text_texture->font = font;
    
    SDL_DestroySurface(text_surface);
    
    // Cache it
    text_cache[cache_key] = text_texture;
    
    return text_texture;
}

void TextRenderer::draw_text(RenderingFacade* rendering_facade, const std::string& text, 
                           const std::string& font_name, float x, float y, SDL_Color color) {
    if (!rendering_facade) {
        SDL_Log("TextRenderer::draw_text() - No RenderingFacade available");
        return;
    }
    
    // Use RenderingFacade to render text
    PixelCoord position(x, y);
    auto result = rendering_facade->render_text(text, position, font_name, color.r, color.g, color.b);
    
    if (!result.is_ok()) {
        SDL_Log("TextRenderer::draw_text() failed: %s", result.get_error_message().c_str());
    }
}

void TextRenderer::draw_text_centered(RenderingFacade* rendering_facade, const std::string& text, 
                                    const std::string& font_name, float center_x, float y, SDL_Color color) {
    if (!rendering_facade) {
        SDL_Log("TextRenderer::draw_text_centered() - No RenderingFacade available");
        return;
    }
    
    // For now, use the center_x directly - RenderingFacade can handle centering internally
    // In a more sophisticated implementation, we'd calculate text width first
    PixelCoord position(center_x, y);
    auto result = rendering_facade->render_text(text, position, font_name, color.r, color.g, color.b);
    
    if (!result.is_ok()) {
        SDL_Log("TextRenderer::draw_text_centered() failed: %s", result.get_error_message().c_str());
    }
}