#ifndef GPU_ACCELERATED_RENDERER_H
#define GPU_ACCELERATED_RENDERER_H

#include <SDL3/SDL.h>
#include <glad/gl.h>
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
    
    // Simple vertex structure for basic rendering - PACKED to prevent alignment issues
    struct __attribute__((packed)) SimpleVertex {
        vec2 position;
        vec2 texCoord;
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
        BLOOD_SPLATTER = 4,
        TILE_FRAGMENTATION = 5,
        MATRIX_DIGITAL = 6
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
                   const float* color = nullptr, float rotation = 0.0f, const float* scale = nullptr, int sprite_number = 0);
    void add_animated_sprite(float x, float y, float w, float h, GLuint texture,
                           const float* color, float rotation, const float* scale, EffectType effect, int sprite_number = 0);
    void end_batch();
    
    // Explosion control
    void set_explosion_info(float center_x, float center_y, float age, int up, int down, int left, int right);
    void clear_explosion_info();
    
    // GPU particle system
    bool init_particle_system(int max_particles = 100000);
    void update_particles_gpu(float deltaTime);
    void emit_particles(float x, float y, int count, ParticleType type, 
                       const float* velocity = nullptr, float life = 2.0f);
    void render_particles();
    
    // SPECTACULAR effect controls
    void set_camera(const float* position, float zoom = 1.0f);
    void set_explosion_effect(float center_x, float center_y, float radius, float strength);
    void set_vortex_effect(float center_x, float center_y, float radius, float strength);
    void set_environmental_effects(float air_density, const float* magnetic_field);
    void clear_effects(); // Reset all special effects
    void set_global_effect_params(const float* params);
    void set_wind(const float* wind) { if(wind) { wind_force[0] = wind[0]; wind_force[1] = wind[1]; } }
    
    // Texture management
    GLuint create_texture_from_surface(SDL_Surface* surface);
    GLuint load_texture_from_file(const std::string& path);
    void register_texture_metadata(GLuint texture_id, int width, int height, int sprite_width = 40, int sprite_height = 40);
    
    // Debug and profiling
    void enable_debug_overlay(bool enable) { debug_overlay = enable; }
    void print_performance_stats();
    
    // Safety checks
    bool is_ready() const { return gl_context && main_program && sprite_vao && sprite_vbo; }
    
    // Public access to OpenGL context for external management
    SDL_GLContext gl_context;

private:
    // OpenGL context and window (moved to public)
    SDL_Window* window;  // Store window reference to restore context
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
    // Rendering uniforms
    GLint u_projection, u_view, u_model, u_time;
    GLint u_effect_type, u_effect_params;
    GLint u_texture, u_resolution;
    GLint u_explosion_center, u_explosion_size;
    
    // SPECTACULAR effect uniforms
    GLint u_explosion_data;      // x,y=center, z=radius, w=strength
    GLint u_vortex_data;         // x,y=center, z=radius, w=strength
    GLint u_air_density;         // Environmental air density
    GLint u_magnetic_field;      // Magnetic field for charged particles
    GLint u_noise_lut;           // Noise lookup texture
    
    // Particle system uniforms
    GLint u_delta_time, u_gravity, u_wind, u_world_size;
    GLint u_physics_constants;   // Combined physics parameters
    GLint u_turbulence_field;    // Turbulence field texture
    
    // Batching system
    std::vector<SimpleVertex> batch_vertices;
    std::vector<GLuint> batch_indices;
    int current_quad_count;
    EffectType current_effect;
    GLuint current_texture;  // Track current texture to flush on change
    static const int MAX_QUADS = 1000;  // Reasonable batch size
    
    // Particle system
    int max_gpu_particles;
    std::vector<GPUParticle> cpu_particles; // For initialization
    int next_particle_index;
    
    // State management
    float current_time;
    vec2 camera_position;
    float camera_zoom;
    
    // Explosion state
    vec4 current_explosion_center; // x,y=center, z=age, w=active
    vec4 current_explosion_size;   // x=up, y=down, z=left, w=right
    vec4 global_effect_params;
    vec2 gravity_force;
    vec2 wind_force;
    
    // SPECTACULAR effect parameters
    vec4 explosion_data;         // x,y=center, z=radius, w=strength
    vec4 vortex_data;           // x,y=center, z=radius, w=strength
    float air_density;
    vec2 magnetic_field;
    GLuint noise_lut_texture;    // Noise lookup table
    GLuint turbulence_texture;   // Turbulence field texture
    
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
    void check_gl_error(const std::string& operation);
    void calculate_sprite_uv(GLuint texture, int sprite_number, float& u_start, float& u_end, float& v_start, float& v_end);
    std::string preprocess_shader_includes(const std::string& source);
    
    // Texture metadata for atlas UV calculation
    struct TextureInfo {
        int width;
        int height;
        int sprite_width;
        int sprite_height;
        int sprites_per_row;
        int sprites_per_col;
    };
    
    // Resource management
    std::unordered_map<std::string, GLuint> loaded_textures;
    std::unordered_map<GLuint, TextureInfo> texture_metadata;  // Store metadata by GL texture ID
    std::unordered_map<std::string, GLuint> shader_programs;
};

#endif