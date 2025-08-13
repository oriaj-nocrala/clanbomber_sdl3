// Implementation of missing methods for GPUAcceleratedRenderer
#include "GPUAcceleratedRenderer.h"
#include <SDL3_image/SDL_image.h>
#include <cstring>
#include <random>
#include <cmath>

void GPUAcceleratedRenderer::add_sprite(float x, float y, float w, float h, GLuint texture, 
                                       const float* color, float rotation, const float* scale) {
    if (current_quad_count >= MAX_QUADS) {
        flush_batch();
    }
    
    // Default values
    vec4 default_color = {1.0f, 1.0f, 1.0f, 1.0f};
    vec2 default_scale = {1.0f, 1.0f};
    
    const float* use_color = color ? color : default_color;
    const float* use_scale = scale ? scale : default_scale;
    
    // Create quad vertices
    AdvancedVertex vertices[4];
    
    // Top-left
    vertices[0].position[0] = x;
    vertices[0].position[1] = y;
    vertices[0].texCoord[0] = 0.0f;
    vertices[0].texCoord[1] = 0.0f;
    memcpy(vertices[0].color, use_color, 4 * sizeof(float));
    vertices[0].rotation = rotation;
    memcpy(vertices[0].scale, use_scale, 2 * sizeof(float));
    
    // Top-right
    vertices[1].position[0] = x + w;
    vertices[1].position[1] = y;
    vertices[1].texCoord[0] = 1.0f;
    vertices[1].texCoord[1] = 0.0f;
    memcpy(vertices[1].color, use_color, 4 * sizeof(float));
    vertices[1].rotation = rotation;
    memcpy(vertices[1].scale, use_scale, 2 * sizeof(float));
    
    // Bottom-right
    vertices[2].position[0] = x + w;
    vertices[2].position[1] = y + h;
    vertices[2].texCoord[0] = 1.0f;
    vertices[2].texCoord[1] = 1.0f;
    memcpy(vertices[2].color, use_color, 4 * sizeof(float));
    vertices[2].rotation = rotation;
    memcpy(vertices[2].scale, use_scale, 2 * sizeof(float));
    
    // Bottom-left
    vertices[3].position[0] = x;
    vertices[3].position[1] = y + h;
    vertices[3].texCoord[0] = 0.0f;
    vertices[3].texCoord[1] = 1.0f;
    memcpy(vertices[3].color, use_color, 4 * sizeof(float));
    vertices[3].rotation = rotation;
    memcpy(vertices[3].scale, use_scale, 2 * sizeof(float));
    
    // Add to batch
    for (int i = 0; i < 4; i++) {
        batch_vertices.push_back(vertices[i]);
    }
    
    current_quad_count++;
}

void GPUAcceleratedRenderer::add_animated_sprite(float x, float y, float w, float h, GLuint texture,
                                               const float* color, float rotation, const float* scale, EffectType effect) {
    if (current_effect != effect) {
        flush_batch();
        current_effect = effect;
    }
    
    add_sprite(x, y, w, h, texture, color, rotation, scale);
}

void GPUAcceleratedRenderer::end_batch() {
    flush_batch();
}

void GPUAcceleratedRenderer::update_particles_gpu(float deltaTime) {
    if (!particle_compute_program || !particle_ssbo) return;
    
    glUseProgram(particle_compute_program);
    
    // Set compute uniforms
    glUniform1f(u_delta_time, deltaTime);
    glUniform2f(u_gravity, gravity_force[0], gravity_force[1]);
    glUniform2f(u_wind, wind_force[0], wind_force[1]);
    glUniform2f(u_world_size, (float)screen_width, (float)screen_height);
    
    // Dispatch compute shader
    GLuint num_groups = (max_gpu_particles + 255) / 256;
    glDispatchCompute(num_groups, 1, 1);
    
    // Memory barrier to ensure compute shader is done
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
    perf_stats.particles_rendered = max_gpu_particles;
}

void GPUAcceleratedRenderer::emit_particles(float x, float y, int count, ParticleType type, 
                                          const float* velocity, float life) {
    // Map buffer and find inactive particles
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_ssbo);
    GPUParticle* particles = (GPUParticle*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
    
    if (!particles) return;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> speed_dist(50.0f, 300.0f);
    std::uniform_real_distribution<float> size_dist(1.0f, 4.0f);
    
    vec2 default_velocity = {0.0f, 0.0f};
    const float* use_velocity = velocity ? velocity : default_velocity;
    
    int emitted = 0;
    for (int i = 0; i < max_gpu_particles && emitted < count; i++) {
        if (particles[i].active == 0) {
            // Set position
            particles[i].position[0] = x + (gen() % 10 - 5); // Small random offset
            particles[i].position[1] = y + (gen() % 10 - 5);
            
            // Set velocity
            float angle = angle_dist(gen);
            float speed = speed_dist(gen);
            particles[i].velocity[0] = use_velocity[0] + cos(angle) * speed;
            particles[i].velocity[1] = use_velocity[1] + sin(angle) * speed;
            
            // Reset acceleration
            particles[i].acceleration[0] = 0.0f;
            particles[i].acceleration[1] = 0.0f;
            
            // Set life properties
            particles[i].life = life;
            particles[i].max_life = life;
            particles[i].size = size_dist(gen);
            particles[i].mass = 1.0f + (gen() % 100) / 100.0f; // 1.0 to 2.0
            
            // Set type and color based on particle type
            particles[i].type = (int)type;
            switch (type) {
                case SPARK:
                    particles[i].color[0] = 1.0f; // Red
                    particles[i].color[1] = 0.8f; // Green
                    particles[i].color[2] = 0.0f; // Blue
                    particles[i].color[3] = 1.0f; // Alpha
                    break;
                case SMOKE:
                    particles[i].color[0] = 0.7f;
                    particles[i].color[1] = 0.7f;
                    particles[i].color[2] = 0.7f;
                    particles[i].color[3] = 0.8f;
                    break;
                case BLOOD:
                    particles[i].color[0] = 0.8f;
                    particles[i].color[1] = 0.0f;
                    particles[i].color[2] = 0.0f;
                    particles[i].color[3] = 1.0f;
                    break;
                case FIRE:
                    particles[i].color[0] = 1.0f;
                    particles[i].color[1] = 0.5f;
                    particles[i].color[2] = 0.0f;
                    particles[i].color[3] = 1.0f;
                    break;
            }
            
            // Set forces (gravity, drag, wind)
            particles[i].forces[0] = 1.0f; // Gravity multiplier
            particles[i].forces[1] = 0.1f; // Drag coefficient
            particles[i].forces[2] = 1.0f; // Wind sensitivity X
            particles[i].forces[3] = 1.0f; // Wind sensitivity Y
            
            particles[i].rotation = 0.0f;
            particles[i].angular_velocity = (gen() % 200 - 100) / 10.0f; // -10 to 10
            
            particles[i].active = 1;
            emitted++;
        }
    }
    
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void GPUAcceleratedRenderer::render_particles() {
    // Particles are rendered by the compute shader
    // Here we could implement point sprite rendering if needed
    // For now, particles are handled entirely on GPU
}

void GPUAcceleratedRenderer::set_camera(const float* position, float zoom) {
    if (position) {
        camera_position[0] = position[0];
        camera_position[1] = position[1];
    }
    camera_zoom = zoom;
    
    // Update view matrix
    glm_mat4_identity(view_matrix);
    glm_translate(view_matrix, (vec3){-camera_position[0], -camera_position[1], 0.0f});
    glm_scale_uni(view_matrix, camera_zoom);
}

void GPUAcceleratedRenderer::set_global_effect_params(const float* params) {
    if (params) {
        memcpy(global_effect_params, params, 4 * sizeof(float));
    }
}

GLuint GPUAcceleratedRenderer::create_texture_from_surface(SDL_Surface* surface) {
    if (!surface) return 0;
    
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Determine format - default to RGBA for SDL3
    GLenum format = GL_RGBA;
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, 
                format, GL_UNSIGNED_BYTE, surface->pixels);
    
    // Set advanced texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Generate mipmaps for better quality
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Enable anisotropic filtering if available (OpenGL 4.6 core)
    GLfloat max_anisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, max_anisotropy);
    
    return texture;
}

GLuint GPUAcceleratedRenderer::load_texture_from_file(const std::string& path) {
    // Check if already loaded
    auto it = loaded_textures.find(path);
    if (it != loaded_textures.end()) {
        return it->second;
    }
    
    // Load with SDL_image
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        SDL_Log("Failed to load texture: %s - %s", path.c_str(), SDL_GetError());
        return 0;
    }
    
    GLuint texture = create_texture_from_surface(surface);
    SDL_DestroySurface(surface);
    
    if (texture) {
        loaded_textures[path] = texture;
    }
    
    return texture;
}

void GPUAcceleratedRenderer::print_performance_stats() {
    SDL_Log("=== GPU Renderer Performance Stats ===");
    SDL_Log("Draw calls: %d", perf_stats.draw_calls);
    SDL_Log("Particles rendered: %d", perf_stats.particles_rendered);
    SDL_Log("Vertices rendered: %d", perf_stats.vertices_rendered);
    SDL_Log("GPU time: %.2f ms", perf_stats.gpu_time);
    SDL_Log("CPU time: %.2f ms", perf_stats.cpu_time);
}