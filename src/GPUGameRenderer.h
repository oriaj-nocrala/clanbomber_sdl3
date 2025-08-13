#ifndef GPU_GAME_RENDERER_H
#define GPU_GAME_RENDERER_H

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <cglm/cglm.h>
#include <vector>
#include <string>

// Simplified GPU renderer specifically for ClanBomber game
class GPUGameRenderer {
public:
    struct GameVertex {
        float x, y;           // Position
        float u, v;           // UV coordinates  
        float r, g, b, a;     // Color
        float rotation;       // Rotation angle
        float scale_x, scale_y; // Scale factors
    };
    
    struct GPUGameParticle {
        vec2 position;
        vec2 velocity;
        float life, max_life;
        float size;
        vec4 color;
        int type;   // 0=explosion, 1=blood, 2=smoke, 3=fire
        int active;
        float _pad1, _pad2; // Padding for alignment
    };

public:
    GPUGameRenderer();
    ~GPUGameRenderer();
    
    bool initialize(SDL_Window* window);
    void shutdown();
    
    // Frame management
    void begin_frame();
    void end_frame();
    void present();
    
    // Game object rendering
    void render_game_object(float x, float y, float w, float h, GLuint texture,
                           float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f,
                           float rotation = 0.0f, float scale = 1.0f);
    
    // Particle effects for game
    void init_game_particles(int max_particles = 5000);
    void update_game_particles(float deltaTime);
    void emit_explosion_particles(float x, float y, int count = 30);
    void emit_blood_particles(float x, float y, int count = 20);
    void emit_dust_particles(float x, float y, int count = 15);
    void render_game_particles();
    
    // Effect management
    void set_invincibility_effect(bool enabled) { invincibility_effect = enabled; }
    void set_explosion_heat_effect(bool enabled) { heat_effect = enabled; }
    
    // Utility
    GLuint load_game_texture(SDL_Surface* surface);

private:
    SDL_GLContext gl_context;
    GLuint shader_program;
    GLuint particle_compute_program;
    
    // Buffers
    GLuint vao, vbo, ebo;
    GLuint particle_ssbo, particle_vao;
    
    // Uniforms
    GLint u_projection, u_model, u_time, u_effect_type;
    GLint u_delta_time, u_gravity, u_world_size;
    
    // Matrices
    mat4 projection_matrix;
    mat4 model_matrix;
    
    // Game state
    float game_time;
    bool invincibility_effect;
    bool heat_effect;
    
    // Particles
    int max_game_particles;
    std::vector<GPUGameParticle> particles;
    
    // Batch data
    std::vector<GameVertex> vertices;
    int quad_count;
    
    // Helper functions
    bool load_game_shaders();
    void setup_rendering();
    void flush_batch();
    std::string load_shader_file(const std::string& path);
};

#endif