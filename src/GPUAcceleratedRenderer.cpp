#include "GPUAcceleratedRenderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <random>

GPUAcceleratedRenderer::GPUAcceleratedRenderer() 
    : gl_context(nullptr), main_program(0), particle_compute_program(0), debug_program(0),
      sprite_vao(0), sprite_vbo(0), sprite_ebo(0), particle_vao(0), particle_vbo(0),
      particle_ssbo(0), particle_counter_buffer(0), max_gpu_particles(0),
      current_quad_count(0), current_effect(NORMAL), next_particle_index(0),
      current_time(0.0f), camera_zoom(1.0f), debug_overlay(false) {
    
    // Initialize vectors and matrices
    glm_vec2_zero(camera_position);
    glm_vec4_zero(global_effect_params);
    glm_vec2_copy((vec2){0.0f, 500.0f}, gravity_force); // Default gravity
    glm_vec2_zero(wind_force);
    
    // Initialize performance stats
    memset(&perf_stats, 0, sizeof(perf_stats));
}

GPUAcceleratedRenderer::~GPUAcceleratedRenderer() {
    shutdown();
}

bool GPUAcceleratedRenderer::initialize(SDL_Window* window, int width, int height) {
    screen_width = width;
    screen_height = height;
    
    // Set OpenGL 4.6 core profile attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    
    // Create OpenGL context
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        SDL_Log("Failed to create OpenGL 4.6 context: %s", SDL_GetError());
        return false;
    }
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to initialize GLAD for OpenGL 4.6");
        return false;
    }
    
    // Verify OpenGL version
    SDL_Log("OpenGL Version: %s", glGetString(GL_VERSION));
    SDL_Log("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    SDL_Log("Renderer: %s", glGetString(GL_RENDERER));
    SDL_Log("Vendor: %s", glGetString(GL_VENDOR));
    
    // Check for required extensions
    if (!GLAD_GL_VERSION_4_6) {
        SDL_Log("OpenGL 4.6 not supported!");
        return false;
    }
    
    // Enable advanced OpenGL features
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_MULTISAMPLE); // MSAA
    
    // Enable seamless cubemap sampling
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
    // Set VSync
    SDL_GL_SetSwapInterval(1);
    
    // Load all shaders
    if (!load_all_shaders()) {
        SDL_Log("Failed to load shaders");
        return false;
    }
    
    // Setup rendering systems
    setup_matrices();
    setup_sprite_rendering();
    
    // Initialize particle system
    if (!init_particle_system(100000)) { // 100K particles!
        SDL_Log("Failed to initialize GPU particle system");
        return false;
    }
    
    SDL_Log("GPU Accelerated Renderer initialized successfully!");
    SDL_Log("Max particles: %d", max_gpu_particles);
    
    return true;
}

void GPUAcceleratedRenderer::shutdown() {
    // Clean up OpenGL resources
    if (main_program) glDeleteProgram(main_program);
    if (particle_compute_program) glDeleteProgram(particle_compute_program);
    if (debug_program) glDeleteProgram(debug_program);
    
    if (sprite_vao) glDeleteVertexArrays(1, &sprite_vao);
    if (sprite_vbo) glDeleteBuffers(1, &sprite_vbo);
    if (sprite_ebo) glDeleteBuffers(1, &sprite_ebo);
    
    if (particle_vao) glDeleteVertexArrays(1, &particle_vao);
    if (particle_vbo) glDeleteBuffers(1, &particle_vbo);
    if (particle_ssbo) glDeleteBuffers(1, &particle_ssbo);
    if (particle_counter_buffer) glDeleteBuffers(1, &particle_counter_buffer);
    
    // Clean up textures
    for (auto& [name, texture] : loaded_textures) {
        glDeleteTextures(1, &texture);
    }
    loaded_textures.clear();
    
    if (gl_context) {
        SDL_GL_DestroyContext(gl_context);
        gl_context = nullptr;
    }
    
    SDL_Log("GPU Accelerated Renderer shutdown complete");
}

bool GPUAcceleratedRenderer::load_all_shaders() {
    // Load main rendering shaders
    std::string vertex_src = load_shader_source("src/shaders/advanced_vertex.glsl");
    std::string fragment_src = load_shader_source("src/shaders/advanced_fragment.glsl");
    
    if (vertex_src.empty() || fragment_src.empty()) {
        SDL_Log("Failed to load main shader sources");
        return false;
    }
    
    GLuint vertex_shader = compile_shader(vertex_src, GL_VERTEX_SHADER, "main_vertex");
    GLuint fragment_shader = compile_shader(fragment_src, GL_FRAGMENT_SHADER, "main_fragment");
    
    if (!vertex_shader || !fragment_shader) {
        return false;
    }
    
    main_program = create_program(vertex_shader, fragment_shader, "main_program");
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    if (!main_program) return false;
    
    // Get uniform locations for main program
    glUseProgram(main_program);
    u_projection = glGetUniformLocation(main_program, "uProjection");
    u_view = glGetUniformLocation(main_program, "uView");
    u_model = glGetUniformLocation(main_program, "uModel");
    u_time = glGetUniformLocation(main_program, "uTime");
    u_effect_type = glGetUniformLocation(main_program, "uEffectType");
    u_effect_params = glGetUniformLocation(main_program, "uEffectParams");
    
    // Load compute shader for particles
    std::string compute_src = load_shader_source("src/shaders/advanced_compute.glsl");
    if (!compute_src.empty()) {
        GLuint compute_shader = compile_shader(compute_src, GL_COMPUTE_SHADER, "particle_compute");
        if (compute_shader) {
            particle_compute_program = create_compute_program(compute_shader, "particle_system");
            glDeleteShader(compute_shader);
            
            if (particle_compute_program) {
                glUseProgram(particle_compute_program);
                u_delta_time = glGetUniformLocation(particle_compute_program, "uDeltaTime");
                u_gravity = glGetUniformLocation(particle_compute_program, "uGravity");
                u_wind = glGetUniformLocation(particle_compute_program, "uWind");
                u_world_size = glGetUniformLocation(particle_compute_program, "uWorldSize");
                GLint u_time_compute = glGetUniformLocation(particle_compute_program, "uTime");
                GLint u_effect_params_compute = glGetUniformLocation(particle_compute_program, "uEffectParams");
            }
        }
    }
    
    check_gl_error("shader loading");
    return true;
}

GLuint GPUAcceleratedRenderer::compile_shader(const std::string& source, GLenum type, const std::string& name) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[2048];
        glGetShaderInfoLog(shader, sizeof(info_log), nullptr, info_log);
        SDL_Log("Shader compilation error (%s): %s", name.c_str(), info_log);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint GPUAcceleratedRenderer::create_program(GLuint vertex, GLuint fragment, const std::string& name) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[2048];
        glGetProgramInfoLog(program, sizeof(info_log), nullptr, info_log);
        SDL_Log("Program linking error (%s): %s", name.c_str(), info_log);
        glDeleteProgram(program);
        return 0;
    }
    
    shader_programs[name] = program;
    return program;
}

GLuint GPUAcceleratedRenderer::create_compute_program(GLuint compute, const std::string& name) {
    GLuint program = glCreateProgram();
    glAttachShader(program, compute);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[2048];
        glGetProgramInfoLog(program, sizeof(info_log), nullptr, info_log);
        SDL_Log("Compute program linking error (%s): %s", name.c_str(), info_log);
        glDeleteProgram(program);
        return 0;
    }
    
    shader_programs[name] = program;
    return program;
}

void GPUAcceleratedRenderer::setup_matrices() {
    // Create orthographic projection
    glm_ortho(0.0f, (float)screen_width, (float)screen_height, 0.0f, -1000.0f, 1000.0f, projection_matrix);
    
    // Initialize view and model matrices
    glm_mat4_identity(view_matrix);
    glm_mat4_identity(model_matrix);
}

void GPUAcceleratedRenderer::setup_sprite_rendering() {
    // Generate VAO and buffers
    glGenVertexArrays(1, &sprite_vao);
    glGenBuffers(1, &sprite_vbo);
    glGenBuffers(1, &sprite_ebo);
    
    glBindVertexArray(sprite_vao);
    
    // Setup dynamic vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_QUADS * 4 * sizeof(AdvancedVertex), nullptr, GL_DYNAMIC_DRAW);
    
    // Setup index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite_ebo);
    std::vector<GLuint> indices;
    indices.reserve(MAX_QUADS * 6);
    for (int i = 0; i < MAX_QUADS; i++) {
        GLuint base = i * 4;
        indices.insert(indices.end(), {
            base, base + 1, base + 2,
            base, base + 2, base + 3
        });
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    
    // Setup vertex attributes
    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(AdvancedVertex), 
                         (void*)offsetof(AdvancedVertex, position));
    glEnableVertexAttribArray(0);
    
    // Texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(AdvancedVertex), 
                         (void*)offsetof(AdvancedVertex, texCoord));
    glEnableVertexAttribArray(1);
    
    // Color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(AdvancedVertex), 
                         (void*)offsetof(AdvancedVertex, color));
    glEnableVertexAttribArray(2);
    
    // Rotation
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(AdvancedVertex), 
                         (void*)offsetof(AdvancedVertex, rotation));
    glEnableVertexAttribArray(3);
    
    // Scale
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(AdvancedVertex), 
                         (void*)offsetof(AdvancedVertex, scale));
    glEnableVertexAttribArray(4);
    
    batch_vertices.reserve(MAX_QUADS * 4);
    batch_indices.reserve(MAX_QUADS * 6);
    
    check_gl_error("sprite rendering setup");
}

bool GPUAcceleratedRenderer::init_particle_system(int max_particles) {
    max_gpu_particles = max_particles;
    cpu_particles.resize(max_particles);
    
    // Initialize all particles as inactive
    for (auto& p : cpu_particles) {
        p.active = 0;
        glm_vec2_zero(p.position);
        glm_vec2_zero(p.velocity);
        glm_vec2_zero(p.acceleration);
        p.life = 0.0f;
        p.max_life = 0.0f;
        p.size = 1.0f;
        p.mass = 1.0f;
        glm_vec4_zero(p.color);
        glm_vec4_zero(p.forces);
        p.type = 0;
        p.rotation = 0.0f;
        p.angular_velocity = 0.0f;
    }
    
    // Create SSBO for particles
    glGenBuffers(1, &particle_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
                max_particles * sizeof(GPUParticle), 
                cpu_particles.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_ssbo);
    
    // Create atomic counter for active particles
    glGenBuffers(1, &particle_counter_buffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, particle_counter_buffer);
    GLuint zero = 0;
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, particle_counter_buffer);
    
    check_gl_error("particle system initialization");
    return true;
}

void GPUAcceleratedRenderer::begin_frame() {
    // Clear buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Reset performance counters
    perf_stats.draw_calls = 0;
    perf_stats.particles_rendered = 0;
    perf_stats.vertices_rendered = 0;
    
    // Update time
    current_time = SDL_GetTicks() / 1000.0f;
    
    check_gl_error("begin frame");
}

void GPUAcceleratedRenderer::end_frame() {
    // Flush any remaining batches
    if (current_quad_count > 0) {
        flush_batch();
    }
    
    check_gl_error("end frame");
}

void GPUAcceleratedRenderer::present() {
    // SDL handles the buffer swap
}

std::string GPUAcceleratedRenderer::load_shader_source(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        SDL_Log("Failed to open shader file: %s", filename.c_str());
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void GPUAcceleratedRenderer::check_gl_error(const std::string& operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        SDL_Log("OpenGL error in %s: 0x%x", operation.c_str(), error);
    }
}

// Additional methods will be implemented as needed...
void GPUAcceleratedRenderer::begin_batch(EffectType effect) {
    if (current_quad_count > 0 && current_effect != effect) {
        flush_batch();
    }
    current_effect = effect;
}

void GPUAcceleratedRenderer::flush_batch() {
    if (current_quad_count == 0) return;
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 
                   batch_vertices.size() * sizeof(AdvancedVertex), 
                   batch_vertices.data());
    
    // Use main shader program
    glUseProgram(main_program);
    update_uniforms();
    
    // Draw
    glBindVertexArray(sprite_vao);
    glDrawElements(GL_TRIANGLES, current_quad_count * 6, GL_UNSIGNED_INT, 0);
    
    // Update stats
    perf_stats.draw_calls++;
    perf_stats.vertices_rendered += current_quad_count * 4;
    
    // Reset batch
    batch_vertices.clear();
    current_quad_count = 0;
}

void GPUAcceleratedRenderer::update_uniforms() {
    glUniformMatrix4fv(u_projection, 1, GL_FALSE, (float*)projection_matrix);
    glUniformMatrix4fv(u_view, 1, GL_FALSE, (float*)view_matrix);
    glUniformMatrix4fv(u_model, 1, GL_FALSE, (float*)model_matrix);
    glUniform1f(u_time, current_time);
    glUniform1i(u_effect_type, (int)current_effect);
    glUniform4fv(u_effect_params, 1, global_effect_params);
}