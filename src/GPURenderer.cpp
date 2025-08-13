#include "GPURenderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <random>
#include <cglm/cglm.h>

GPURenderer::GPURenderer() : gl_context(nullptr), main_program(0), 
    particle_compute_program(0), vao(0), vbo(0), ebo(0), 
    particle_ssbo(0), particle_vao(0), particle_vbo(0),
    max_gpu_particles(0), quad_count(0), current_time(0.0f), 
    current_effect_type(0) {
}

GPURenderer::~GPURenderer() {
    shutdown();
}

bool GPURenderer::initialize(SDL_Window* window) {
    // Set OpenGL attributes for 4.6 core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    
    // Create OpenGL context
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        SDL_Log("Failed to create OpenGL context: %s", SDL_GetError());
        return false;
    }
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to initialize GLAD");
        return false;
    }
    
    SDL_Log("OpenGL Version: %s", glGetString(GL_VERSION));
    SDL_Log("GLSL Version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    // Enable VSync
    SDL_GL_SetSwapInterval(1);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Load shaders
    if (!load_shaders()) {
        SDL_Log("Failed to load shaders");
        return false;
    }
    
    setup_projection();
    setup_batch_rendering();
    
    return true;
}

void GPURenderer::shutdown() {
    if (main_program) {
        glDeleteProgram(main_program);
        main_program = 0;
    }
    
    if (particle_compute_program) {
        glDeleteProgram(particle_compute_program);
        particle_compute_program = 0;
    }
    
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    
    if (ebo) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
    
    if (particle_ssbo) {
        glDeleteBuffers(1, &particle_ssbo);
        particle_ssbo = 0;
    }
    
    if (gl_context) {
        SDL_GL_DestroyContext(gl_context);
        gl_context = nullptr;
    }
}

bool GPURenderer::load_shaders() {
    // Load vertex shader
    std::string vertex_source = load_shader_source("src/shaders/vertex.glsl");
    if (vertex_source.empty()) {
        SDL_Log("Failed to load vertex shader");
        return false;
    }
    
    GLuint vertex_shader = compile_shader(vertex_source, GL_VERTEX_SHADER);
    if (!vertex_shader) return false;
    
    // Load fragment shader  
    std::string fragment_source = load_shader_source("src/shaders/fragment.glsl");
    if (fragment_source.empty()) {
        SDL_Log("Failed to load fragment shader");
        return false;
    }
    
    GLuint fragment_shader = compile_shader(fragment_source, GL_FRAGMENT_SHADER);
    if (!fragment_shader) {
        glDeleteShader(vertex_shader);
        return false;
    }
    
    // Create main program
    main_program = create_program(vertex_shader, fragment_shader);
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    if (!main_program) return false;
    
    // Get uniform locations
    u_projection = glGetUniformLocation(main_program, "uProjection");
    u_model = glGetUniformLocation(main_program, "uModel");
    u_time = glGetUniformLocation(main_program, "uTime");
    u_effect_type = glGetUniformLocation(main_program, "uEffectType");
    
    // Load compute shader for particles
    std::string compute_source = load_shader_source("src/shaders/particle_compute.glsl");
    if (!compute_source.empty()) {
        GLuint compute_shader = compile_shader(compute_source, GL_COMPUTE_SHADER);
        if (compute_shader) {
            particle_compute_program = glCreateProgram();
            glAttachShader(particle_compute_program, compute_shader);
            glLinkProgram(particle_compute_program);
            glDeleteShader(compute_shader);
            
            // Get compute uniform locations
            u_delta_time = glGetUniformLocation(particle_compute_program, "uDeltaTime");
            u_gravity = glGetUniformLocation(particle_compute_program, "uGravity");
            u_world_size = glGetUniformLocation(particle_compute_program, "uWorldSize");
        }
    }
    
    return true;
}

GLuint GPURenderer::compile_shader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar info_log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, info_log);
        SDL_Log("Shader compilation error: %s", info_log);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

GLuint GPURenderer::create_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar info_log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, info_log);
        SDL_Log("Program linking error: %s", info_log);
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

void GPURenderer::setup_projection() {
    // Create orthographic projection matrix using cglm
    glm_ortho(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f, projection_matrix);
    
    // Initialize view and model matrices
    glm_mat4_identity(view_matrix);
    glm_mat4_identity(model_matrix);
}

void GPURenderer::setup_batch_rendering() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(vao);
    
    // Setup vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_QUADS * 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
    
    // Setup index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    std::vector<GLuint> indices;
    indices.reserve(MAX_QUADS * 6);
    for (GLuint i = 0; i < MAX_QUADS; i++) {
        GLuint base = i * 4;
        indices.insert(indices.end(), {
            base, base + 1, base + 2,
            base, base + 2, base + 3
        });
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), 
                indices.data(), GL_STATIC_DRAW);
    
    // Vertex attributes
    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    
    // Texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         (void*)offsetof(Vertex, u));
    glEnableVertexAttribArray(1);
    
    // Color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(2);
    
    batch_vertices.reserve(MAX_QUADS * 4);
    batch_indices.reserve(MAX_QUADS * 6);
}

void GPURenderer::begin_batch() {
    batch_vertices.clear();
    quad_count = 0;
}

void GPURenderer::add_quad(float x, float y, float w, float h, GLuint texture,
                          float r, float g, float b, float a) {
    if (quad_count >= MAX_QUADS) {
        end_batch();
        begin_batch();
    }
    
    // Add vertices for quad
    batch_vertices.push_back({x, y, 0.0f, 0.0f, r, g, b, a});
    batch_vertices.push_back({x + w, y, 1.0f, 0.0f, r, g, b, a});
    batch_vertices.push_back({x + w, y + h, 1.0f, 1.0f, r, g, b, a});
    batch_vertices.push_back({x, y + h, 0.0f, 1.0f, r, g, b, a});
    
    quad_count++;
}

void GPURenderer::end_batch() {
    if (quad_count == 0) return;
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 
                   batch_vertices.size() * sizeof(Vertex), 
                   batch_vertices.data());
    
    // Use shader program
    glUseProgram(main_program);
    
    // Set uniforms using cglm matrices
    glUniformMatrix4fv(u_projection, 1, GL_FALSE, (float*)projection_matrix);
    glUniformMatrix4fv(u_model, 1, GL_FALSE, (float*)model_matrix);
    
    glUniform1f(u_time, current_time);
    glUniform1i(u_effect_type, current_effect_type);
    
    // Draw
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, quad_count * 6, GL_UNSIGNED_INT, 0);
}

bool GPURenderer::init_compute_particles(int max_particles) {
    if (!particle_compute_program) return false;
    
    max_gpu_particles = max_particles;
    cpu_particles.resize(max_particles);
    
    // Initialize all particles as inactive
    for (auto& p : cpu_particles) {
        p.active = 0;
    }
    
    // Create SSBO for particles
    glGenBuffers(1, &particle_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 
                max_particles * sizeof(GPUParticle), 
                cpu_particles.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_ssbo);
    
    return true;
}

void GPURenderer::update_particles_gpu(float deltaTime) {
    if (!particle_compute_program || !particle_ssbo) return;
    
    glUseProgram(particle_compute_program);
    glUniform1f(u_delta_time, deltaTime);
    glUniform2f(u_gravity, 0.0f, 500.0f); // Downward gravity
    glUniform2f(u_world_size, 800.0f, 600.0f);
    
    // Dispatch compute shader
    glDispatchCompute((max_gpu_particles + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GPURenderer::emit_particles_gpu(float x, float y, int count, int type) {
    // Find inactive particles and activate them
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_ssbo);
    GPUParticle* particles = (GPUParticle*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
    
    if (!particles) return;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> speed_dist(50.0f, 200.0f);
    
    int emitted = 0;
    for (int i = 0; i < max_gpu_particles && emitted < count; i++) {
        if (particles[i].active == 0) {
            float angle = angle_dist(gen);
            float speed = speed_dist(gen);
            
            particles[i].pos_x = x;
            particles[i].pos_y = y;
            particles[i].vel_x = cos(angle) * speed;
            particles[i].vel_y = sin(angle) * speed;
            particles[i].life = 2.0f;
            particles[i].max_life = 2.0f;
            particles[i].size = 3.0f;
            
            // Set color based on type
            switch (type) {
                case 0: // Explosion
                    particles[i].color_r = 1.0f;
                    particles[i].color_g = 0.5f;
                    particles[i].color_b = 0.0f;
                    break;
                case 1: // Blood
                    particles[i].color_r = 0.8f;
                    particles[i].color_g = 0.0f;
                    particles[i].color_b = 0.0f;
                    break;
                default:
                    particles[i].color_r = 1.0f;
                    particles[i].color_g = 1.0f;
                    particles[i].color_b = 1.0f;
                    break;
            }
            particles[i].color_a = 1.0f;
            
            particles[i].gravity = 1.0f;
            particles[i].drag = 0.5f;
            particles[i].active = 1;
            
            emitted++;
        }
    }
    
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void GPURenderer::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GPURenderer::present() {
    // Swap buffers handled by SDL
}

std::string GPURenderer::load_shader_source(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        SDL_Log("Failed to open shader file: %s", filename.c_str());
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}