#ifndef GPU_ACCELERATED_RENDERER_H
#define GPU_ACCELERATED_RENDERER_H

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <cglm/cglm.h>
#include <vector>
#include <unordered_map>
#include <string>

// Modern OpenGL 4.6 renderer with advanced features
class GPUAcceleratedRenderer {
public:
    // Enhanced vertex structure with all attributes
    struct AdvancedVertex {
        vec2 position;
        vec2 texCoord;
        vec4 color;
        float rotation;
        vec2 scale;
        int effectType;
        float _padding;
    };
    
    // GPU particle structure matching compute shader
    struct GPUParticle {
        vec2 position;
        vec2 velocity;
        vec2 acceleration;
        float life;
        float max_life;
        float size;
        float mass;
        vec4 color;
        vec4 forces; // gravity, drag, wind_x, wind_y
        int type;
        int active;
        float rotation;
        float angular_velocity;
    };
    
    enum EffectType {
        NORMAL = 0,
        PARTICLE_GLOW = 1,
        EXPLOSION_HEAT = 2,
        INVINCIBLE_CHROME = 3,
        BLOOD_SPLATTER = 4
    };
    
    enum ParticleType {
        SPARK = 0,
        SMOKE = 1,
        BLOOD = 2,
        FIRE = 3
    };

public:
    GPUAcceleratedRenderer();
    ~GPUAcceleratedRenderer();
    
    // Core initialization
    bool initialize(SDL_Window* window, int width = 800, int height = 600);
    void shutdown();
    
    // Shader management
    bool load_all_shaders();
    GLuint compile_shader(const std::string& source, GLenum type, const std::string& name = "");
    GLuint create_program(GLuint vertex, GLuint fragment, const std::string& name = "");
    GLuint create_compute_program(GLuint compute, const std::string& name = "");
    
    // Advanced batched rendering
    void begin_frame();
    void end_frame();
    void present();
    
    // Batch rendering with effects
    void begin_batch(EffectType effect = NORMAL);
    void add_sprite(float x, float y, float w, float h, GLuint texture, 
                   const float* color = nullptr, float rotation = 0.0f, const float* scale = nullptr);
    void add_animated_sprite(float x, float y, float w, float h, GLuint texture,
                           const float* color, float rotation, const float* scale, EffectType effect);
    void end_batch();
    
    // GPU particle system
    bool init_particle_system(int max_particles = 100000);
    void update_particles_gpu(float deltaTime);
    void emit_particles(float x, float y, int count, ParticleType type, 
                       const float* velocity = nullptr, float life = 2.0f);
    void render_particles();
    
    // Advanced effects
    void set_camera(const float* position, float zoom = 1.0f);
    void set_global_effect_params(const float* params);
    void set_wind(const float* wind) { if(wind) { wind_force[0] = wind[0]; wind_force[1] = wind[1]; } }
    
    // Texture management
    GLuint create_texture_from_surface(SDL_Surface* surface);
    GLuint load_texture_from_file(const std::string& path);
    
    // Debug and profiling
    void enable_debug_overlay(bool enable) { debug_overlay = enable; }
    void print_performance_stats();

private:
    // OpenGL context and window
    SDL_GLContext gl_context;
    int screen_width, screen_height;
    
    // Shader programs
    GLuint main_program;
    GLuint particle_compute_program;
    GLuint debug_program;
    
    // Vertex Array Objects and buffers for batching
    GLuint sprite_vao, sprite_vbo, sprite_ebo;
    GLuint particle_vao, particle_vbo;
    
    // Particle system GPU buffers
    GLuint particle_ssbo;  // Shader Storage Buffer Object
    GLuint particle_counter_buffer; // Atomic counter for active particles
    
    // Matrices and uniforms
    mat4 projection_matrix;
    mat4 view_matrix;
    mat4 model_matrix;
    
    // Uniform locations
    GLint u_projection, u_view, u_model, u_time;
    GLint u_effect_type, u_effect_params;
    GLint u_delta_time, u_gravity, u_wind, u_world_size;
    
    // Batching system
    std::vector<AdvancedVertex> batch_vertices;
    std::vector<GLuint> batch_indices;
    int current_quad_count;
    EffectType current_effect;
    static const int MAX_QUADS = 10000;
    
    // Particle system
    int max_gpu_particles;
    std::vector<GPUParticle> cpu_particles; // For initialization
    int next_particle_index;
    
    // State management
    float current_time;
    vec2 camera_position;
    float camera_zoom;
    vec4 global_effect_params;
    vec2 gravity_force;
    vec2 wind_force;
    
    // Debug and profiling
    bool debug_overlay;
    struct {
        int draw_calls;
        int particles_rendered;
        int vertices_rendered;
        float gpu_time;
        float cpu_time;
    } perf_stats;
    
    // Helper functions
    void setup_matrices();
    void setup_sprite_rendering();
    void setup_particle_rendering();
    void update_uniforms();
    void flush_batch();
    std::string load_shader_source(const std::string& filename);
    void check_gl_error(const std::string& operation);
    
    // Resource management
    std::unordered_map<std::string, GLuint> loaded_textures;
    std::unordered_map<std::string, GLuint> shader_programs;
};

#endif