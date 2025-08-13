#include "OptimizedRenderer.h"
#include "ClanBomber.h"
#include "Resources.h"
#include <SDL3/SDL.h>
#include <fstream>
#include <sstream>
#include <random>
#include <cmath>

OptimizedRenderer::OptimizedRenderer() : 
    window(nullptr), gl_context(nullptr), app(nullptr),
    vertex_shader(0), fragment_shader(0), compute_shader(0),
    shader_program(0), compute_program(0),
    vao(0), vbo(0), ebo(0),
    particle_ssbo(0), particle_count(0),
    noise_texture(0), turbulence_texture(0),
    timer_query(0), time_accumulator(0.0f) {
    
    glm_mat4_identity(projection_matrix);
    glm_mat4_identity(view_matrix);
    glm_mat4_identity(model_matrix);
    
    memset(&stats, 0, sizeof(stats));
    memset(time_data, 0, sizeof(time_data));
}

OptimizedRenderer::~OptimizedRenderer() {
    shutdown();
}

bool OptimizedRenderer::init(SDL_Window* window) {
    this->window = window;
    
    // Create OpenGL context with optimized settings
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0); // No depth buffer needed for 2D
    
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        SDL_Log("Failed to create OpenGL context: %s", SDL_GetError());
        return false;
    }
    
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to initialize GLAD");
        return false;
    }
    
    SDL_Log("OptimizedRenderer: OpenGL %s, GLSL %s", 
            glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    // Enable optimizations
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    // Create GPU timer for performance monitoring
    glGenQueries(1, &timer_query);
    
    if (!create_shaders() || !create_buffers() || 
        !create_noise_lut() || !create_turbulence_field()) {
        return false;
    }
    
    // Set up projection matrix for 2D rendering
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    glm_ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f, projection_matrix);
    
    vertex_batch.reserve(MAX_BATCH_SIZE);
    index_batch.reserve(MAX_BATCH_SIZE * 6);
    
    SDL_Log("OptimizedRenderer: Initialized successfully");
    return true;
}

void OptimizedRenderer::shutdown() {
    if (timer_query) glDeleteQueries(1, &timer_query);
    if (particle_ssbo) glDeleteBuffers(1, &particle_ssbo);
    if (noise_texture) glDeleteTextures(1, &noise_texture);
    if (turbulence_texture) glDeleteTextures(1, &turbulence_texture);
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (compute_program) glDeleteProgram(compute_program);
    if (shader_program) glDeleteProgram(shader_program);
    if (compute_shader) glDeleteShader(compute_shader);
    if (fragment_shader) glDeleteShader(fragment_shader);
    if (vertex_shader) glDeleteShader(vertex_shader);
    
    if (gl_context) {
        SDL_GL_DestroyContext(gl_context);
        gl_context = nullptr;
    }
}

bool OptimizedRenderer::create_shaders() {
    // Load and compile optimized shaders
    std::ifstream vertex_file("src/shaders/optimized_vertex.glsl");
    std::ifstream fragment_file("src/shaders/optimized_fragment.glsl");
    std::ifstream compute_file("src/shaders/optimized_compute.glsl");
    
    if (!vertex_file || !fragment_file || !compute_file) {
        SDL_Log("Failed to open shader files");
        return false;
    }
    
    std::stringstream vertex_stream, fragment_stream, compute_stream;
    vertex_stream << vertex_file.rdbuf();
    fragment_stream << fragment_file.rdbuf();
    compute_stream << compute_file.rdbuf();
    
    std::string vertex_code = vertex_stream.str();
    std::string fragment_code = fragment_stream.str();
    std::string compute_code = compute_stream.str();
    
    const char* vertex_source = vertex_code.c_str();
    const char* fragment_source = fragment_code.c_str();
    const char* compute_source = compute_code.c_str();
    
    // Compile vertex shader
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_source, NULL);
    glCompileShader(vertex_shader);
    
    GLint success;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
        SDL_Log("Vertex shader compilation failed: %s", infoLog);
        return false;
    }
    
    // Compile fragment shader
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, NULL);
    glCompileShader(fragment_shader);
    
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
        SDL_Log("Fragment shader compilation failed: %s", infoLog);
        return false;
    }
    
    // Compile compute shader
    compute_shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute_shader, 1, &compute_source, NULL);
    glCompileShader(compute_shader);
    
    glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(compute_shader, 512, NULL, infoLog);
        SDL_Log("Compute shader compilation failed: %s", infoLog);
        return false;
    }
    
    // Link main shader program
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        SDL_Log("Shader program linking failed: %s", infoLog);
        return false;
    }
    
    // Create compute program
    compute_program = glCreateProgram();
    glAttachShader(compute_program, compute_shader);
    glLinkProgram(compute_program);
    
    glGetProgramiv(compute_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(compute_program, 512, NULL, infoLog);
        SDL_Log("Compute program linking failed: %s", infoLog);
        return false;
    }
    
    // Get uniform locations
    u_projection = glGetUniformLocation(shader_program, "uProjection");
    u_view = glGetUniformLocation(shader_program, "uView");
    u_model = glGetUniformLocation(shader_program, "uModel");
    u_time_data = glGetUniformLocation(shader_program, "uTimeData");
    u_resolution = glGetUniformLocation(shader_program, "uResolution");
    u_texture = glGetUniformLocation(shader_program, "uTexture");
    u_noise_lut = glGetUniformLocation(shader_program, "uNoiseLUT");
    
    u_delta_time = glGetUniformLocation(compute_program, "uDeltaTime");
    u_physics_constants = glGetUniformLocation(compute_program, "uPhysicsConstants");
    u_world_size = glGetUniformLocation(compute_program, "uWorldSize");
    u_turbulence_field = glGetUniformLocation(compute_program, "uTurbulenceField");
    
    return true;
}

bool OptimizedRenderer::create_buffers() {
    // Create VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Create VBO for vertex data
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_BATCH_SIZE * sizeof(OptimizedVertex), nullptr, GL_DYNAMIC_DRAW);
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(OptimizedVertex), (void*)offsetof(OptimizedVertex, position));
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(OptimizedVertex), (void*)offsetof(OptimizedVertex, texCoord));
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(OptimizedVertex), (void*)offsetof(OptimizedVertex, color));
    
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(OptimizedVertex), (void*)offsetof(OptimizedVertex, rotation));
    
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(OptimizedVertex), (void*)offsetof(OptimizedVertex, scale));
    
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 1, GL_INT, sizeof(OptimizedVertex), (void*)offsetof(OptimizedVertex, effectMode));
    
    // Create EBO for indices
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_BATCH_SIZE * 6 * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
    
    // Create particle SSBO
    glGenBuffers(1, &particle_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_PARTICLES * sizeof(OptimizedParticle), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_ssbo);
    
    glBindVertexArray(0);
    return true;
}

bool OptimizedRenderer::create_noise_lut() {
    // Generate 256x256 noise texture for fast lookups
    const int size = 256;
    std::vector<float> noise_data(size * size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    for (int i = 0; i < size * size; ++i) {
        noise_data[i] = dis(gen);
    }
    
    glGenTextures(1, &noise_texture);
    glBindTexture(GL_TEXTURE_2D, noise_texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, size, size, 0, GL_RED, GL_FLOAT, noise_data.data());
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    return true;
}

bool OptimizedRenderer::create_turbulence_field() {
    // Generate turbulence field for particle physics
    const int size = 128;
    std::vector<vec2> turbulence_data(size * size);
    
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float fx = (float)x / size * 8.0f;
            float fy = (float)y / size * 8.0f;
            
            // Generate turbulence using multiple octaves of noise
            float turb_x = sin(fx * 2.0f) * 0.5f + sin(fx * 4.0f) * 0.25f + sin(fx * 8.0f) * 0.125f;
            float turb_y = cos(fy * 2.0f) * 0.5f + cos(fy * 4.0f) * 0.25f + cos(fy * 8.0f) * 0.125f;
            
            turbulence_data[y * size + x][0] = (turb_x + 1.0f) * 0.5f;
            turbulence_data[y * size + x][1] = (turb_y + 1.0f) * 0.5f;
        }
    }
    
    glGenTextures(1, &turbulence_texture);
    glBindTexture(GL_TEXTURE_2D, turbulence_texture);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, size, size, 0, GL_RG, GL_FLOAT, turbulence_data.data());
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    return true;
}

void OptimizedRenderer::begin_frame() {
    stats.vertices_rendered = 0;
    stats.draw_calls = 0;
    
    glBeginQuery(GL_TIME_ELAPSED, timer_query);
    
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    glViewport(0, 0, width, height);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OptimizedRenderer::end_frame() {
    flush_batches();
    
    glEndQuery(GL_TIME_ELAPSED);
    
    // Get GPU timing
    GLuint64 elapsed_time;
    glGetQueryObjectui64v(timer_query, GL_QUERY_RESULT, &elapsed_time);
    stats.gpu_time_ms = elapsed_time / 1000000.0f; // Convert to milliseconds
    
    SDL_GL_SwapWindow(window);
}

void OptimizedRenderer::draw_sprite_batched(float x, float y, float w, float h, 
                                          const char* texture_name, int sprite_nr,
                                          float rotation, float scale_x, float scale_y,
                                          int effect_mode, float r, float g, float b, float a) {
    // Add sprite to batch
    if (vertex_batch.size() >= MAX_BATCH_SIZE - 4) {
        flush_batches();
    }
    
    GLuint base_index = vertex_batch.size();
    
    // Create quad vertices
    OptimizedVertex vertices[4];
    
    // Position calculation
    vertices[0] = {{x, y}, {0.0f, 0.0f}, {r, g, b, a}, rotation, {scale_x, scale_y}, effect_mode};
    vertices[1] = {{x + w, y}, {1.0f, 0.0f}, {r, g, b, a}, rotation, {scale_x, scale_y}, effect_mode};
    vertices[2] = {{x + w, y + h}, {1.0f, 1.0f}, {r, g, b, a}, rotation, {scale_x, scale_y}, effect_mode};
    vertices[3] = {{x, y + h}, {0.0f, 1.0f}, {r, g, b, a}, rotation, {scale_x, scale_y}, effect_mode};
    
    vertex_batch.insert(vertex_batch.end(), vertices, vertices + 4);
    
    // Add indices
    GLuint indices[] = {
        base_index, base_index + 1, base_index + 2,
        base_index, base_index + 2, base_index + 3
    };
    index_batch.insert(index_batch.end(), indices, indices + 6);
}

void OptimizedRenderer::flush_batches() {
    if (vertex_batch.empty()) return;
    
    // Update time data
    time_accumulator += 0.016f; // Assume ~60 FPS for now
    time_data[0] = time_accumulator;
    time_data[1] = sin(time_accumulator);
    time_data[2] = cos(time_accumulator);
    time_data[3] = time_accumulator * 2.0f;
    
    glUseProgram(shader_program);
    
    // Update uniforms
    glUniformMatrix4fv(u_projection, 1, GL_FALSE, (float*)projection_matrix);
    glUniformMatrix4fv(u_view, 1, GL_FALSE, (float*)view_matrix);
    glUniformMatrix4fv(u_model, 1, GL_FALSE, (float*)model_matrix);
    glUniform4fv(u_time_data, 1, time_data);
    
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    glUniform2f(u_resolution, width, height);
    
    // Bind textures
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, noise_texture);
    glUniform1i(u_noise_lut, 1);
    
    // Upload vertex data
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertex_batch.size() * sizeof(OptimizedVertex), vertex_batch.data());
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, index_batch.size() * sizeof(GLuint), index_batch.data());
    
    // Draw
    glDrawElements(GL_TRIANGLES, index_batch.size(), GL_UNSIGNED_INT, 0);
    
    stats.vertices_rendered += vertex_batch.size();
    stats.draw_calls++;
    
    vertex_batch.clear();
    index_batch.clear();
}

void OptimizedRenderer::update_particles(float deltaTime) {
    glUseProgram(compute_program);
    
    // Update physics constants
    vec4 physics_constants = {-980.0f, 0.0f, 0.0f, time_accumulator}; // gravity, wind_x, wind_y, time
    glUniform1f(u_delta_time, deltaTime);
    glUniform4fv(u_physics_constants, 1, physics_constants);
    glUniform2f(u_world_size, 800.0f, 600.0f);
    
    // Bind turbulence field
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, turbulence_texture);
    glUniform1i(u_turbulence_field, 2);
    
    // Dispatch compute shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_ssbo);
    glDispatchCompute((MAX_PARTICLES + 63) / 64, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void OptimizedRenderer::render_particles() {
    // TODO: Implement particle rendering using instanced drawing
    // This would require a separate shader for particle rendering
}

void OptimizedRenderer::reset_stats() {
    memset(&stats, 0, sizeof(stats));
}

OptimizedRenderer::PerformanceStats OptimizedRenderer::get_stats() const {
    return stats;
}