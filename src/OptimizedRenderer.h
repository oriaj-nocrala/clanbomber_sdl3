#ifndef OPTIMIZEDRENDERER_H
#define OPTIMIZEDRENDERER_H

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <cglm/cglm.h>
#include <vector>
#include <map>

class ClanBomberApplication;

class OptimizedRenderer {
public:
    struct OptimizedVertex {
        vec2 position;
        vec2 texCoord;
        vec4 color;
        float rotation;
        vec2 scale;
        int effectMode;
    };
    
    struct OptimizedParticle {
        vec4 pos_life;      // x,y=position, z=life, w=max_life
        vec4 vel_size;      // x,y=velocity, z=size, w=mass
        vec4 color;         // rgba color
        vec4 accel_rot;     // x,y=acceleration, z=rotation, w=angular_velocity
        int type_forces[4]; // type, active, gravity_scale, drag_scale
    };
    
    enum ParticleType {
        SPARK = 0,
        SMOKE = 1,
        BLOOD = 2,
        FIRE = 3
    };

public:
    OptimizedRenderer();
    ~OptimizedRenderer();
    
    bool init(SDL_Window* window);
    void shutdown();
    
    void begin_frame();
    void end_frame();
    
    // High-performance batch rendering
    void draw_sprite_batched(float x, float y, float w, float h, 
                            const char* texture_name, int sprite_nr = 0,
                            float rotation = 0.0f, float scale_x = 1.0f, float scale_y = 1.0f,
                            int effect_mode = 0, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);
    
    void flush_batches();
    
    // Optimized particle system
    void emit_particles(float x, float y, ParticleType type, int count, 
                       float velocity_scale = 1.0f, float life_scale = 1.0f);
    void update_particles(float deltaTime);
    void render_particles();
    
    // Performance monitoring
    struct PerformanceStats {
        int vertices_rendered;
        int draw_calls;
        int particles_active;
        float gpu_time_ms;
        float cpu_time_ms;
    };
    
    PerformanceStats get_stats() const { return stats; }
    void reset_stats();

private:
    bool create_shaders();
    bool create_buffers();
    bool create_noise_lut();
    bool create_turbulence_field();
    
    void update_uniforms(float deltaTime);
    
    SDL_Window* window;
    SDL_GLContext gl_context;
    
    // Optimized shaders
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint compute_shader;
    GLuint shader_program;
    GLuint compute_program;
    
    // Vertex Array Objects and Buffers
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    
    // Particle system buffers
    GLuint particle_ssbo;
    GLuint particle_count;
    static constexpr int MAX_PARTICLES = 50000; // Reduced for better performance
    
    // Lookup tables for optimization
    GLuint noise_texture;
    GLuint turbulence_texture;
    
    // Batch rendering
    std::vector<OptimizedVertex> vertex_batch;
    std::vector<GLuint> index_batch;
    static constexpr int MAX_BATCH_SIZE = 10000;
    
    // Uniforms
    GLint u_projection, u_view, u_model;
    GLint u_time_data, u_resolution, u_texture, u_noise_lut;
    GLint u_delta_time, u_physics_constants, u_world_size, u_turbulence_field;
    
    // Performance tracking
    PerformanceStats stats;
    GLuint timer_query;
    
    // Time management
    float time_accumulator;
    vec4 time_data; // x=time, y=sin(time), z=cos(time), w=time*2
    
    // Matrices
    mat4 projection_matrix;
    mat4 view_matrix;
    mat4 model_matrix;
    
    ClanBomberApplication* app;
};

#endif