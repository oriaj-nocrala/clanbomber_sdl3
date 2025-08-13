#ifndef GPU_RENDERER_H
#define GPU_RENDERER_H

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <cglm/cglm.h>
#include <string>
#include <vector>
#include <unordered_map>

struct Vertex {
    float x, y;        // Position
    float u, v;        // Texture coordinates
    float r, g, b, a;  // Color
};

struct GPUParticle {
    float pos_x, pos_y;
    float vel_x, vel_y;
    float life, max_life;
    float size;
    float color_r, color_g, color_b, color_a;
    float gravity, drag;
    int active;
    float _padding;
};

class GPURenderer {
public:
    GPURenderer();
    ~GPURenderer();
    
    bool initialize(SDL_Window* window);
    void shutdown();
    
    // Shader management
    bool load_shaders();
    GLuint compile_shader(const std::string& source, GLenum type);
    GLuint create_program(GLuint vertex_shader, GLuint fragment_shader);
    
    // Batch rendering
    void begin_batch();
    void add_quad(float x, float y, float w, float h, GLuint texture,
                  float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);
    void add_particle_quad(const GPUParticle& particle, GLuint texture);
    void end_batch();
    
    // GPU particle system
    bool init_compute_particles(int max_particles);
    void update_particles_gpu(float deltaTime);
    void emit_particles_gpu(float x, float y, int count, int type);
    void render_particles_gpu();
    
    // Effect rendering
    void set_effect_type(int type) { current_effect_type = type; }
    void set_time(float time) { current_time = time; }
    
    // Utility
    void clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
    void present();
    GLuint create_texture_from_surface(SDL_Surface* surface);
    
private:
    SDL_GLContext gl_context;
    
    // Shader programs
    GLuint main_program;
    GLuint particle_compute_program;
    
    // Vertex buffers for batching
    GLuint vao, vbo, ebo;
    std::vector<Vertex> batch_vertices;
    std::vector<GLuint> batch_indices;
    int quad_count;
    static const int MAX_QUADS = 10000;
    
    // GPU particle system
    GLuint particle_ssbo;  // Shader Storage Buffer Object
    GLuint particle_vao, particle_vbo;
    int max_gpu_particles;
    std::vector<GPUParticle> cpu_particles; // For initialization
    
    // Uniforms
    GLint u_projection, u_model, u_time, u_effect_type;
    GLint u_delta_time, u_gravity, u_world_size;
    
    // State
    float current_time;
    int current_effect_type;
    mat4 projection_matrix;
    mat4 view_matrix;
    mat4 model_matrix;
    
    void setup_projection();
    void setup_batch_rendering();
    void setup_particle_rendering();
    std::string load_shader_source(const std::string& filename);
};

#endif