#include "GPUAcceleratedRenderer.h"
#include "Resources.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <SDL3_image/SDL_image.h>
#include <cstring>
#include <cmath>

GPUAcceleratedRenderer::GPUAcceleratedRenderer() 
    : gl_context(nullptr), main_program(0), particle_compute_program(0), debug_program(0),
      sprite_vao(0), sprite_vbo(0), sprite_ebo(0), particle_vao(0), particle_vbo(0),
      particle_ssbo(0), particle_counter_buffer(0), max_gpu_particles(0),
      current_quad_count(0), current_effect(NORMAL), current_texture(0), next_particle_index(0),
      current_time(0.0f), camera_zoom(1.0f), debug_overlay(false) {
    
    // Initialize vectors and matrices
    glm_vec2_zero(camera_position);
    glm_vec4_zero(global_effect_params);
    glm_vec2_copy((vec2){0.0f, 500.0f}, gravity_force); // Default gravity
    glm_vec2_zero(wind_force);
    
    // Initialize explosion state
    glm_vec4_zero(current_explosion_center);
    glm_vec4_zero(current_explosion_size);
    
    // Initialize performance stats
    memset(&perf_stats, 0, sizeof(perf_stats));
}

GPUAcceleratedRenderer::~GPUAcceleratedRenderer() {
    shutdown();
}

bool GPUAcceleratedRenderer::initialize(SDL_Window* _window, int width, int height) {
    SDL_Log("GPU Renderer: Starting initialization...");
    window = _window;  // Store window reference
    screen_width = width;
    screen_height = height;
    
    // Create OpenGL context (attributes already set in Game.cpp)
    SDL_Log("GPU Renderer: Creating OpenGL context...");
    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        SDL_Log("Failed to create OpenGL context: %s", SDL_GetError());
        
        // Try fallback to OpenGL 3.3 core
        SDL_Log("Attempting fallback to OpenGL 3.3...");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        gl_context = SDL_GL_CreateContext(window);
        
        if (!gl_context) {
            SDL_Log("Failed to create OpenGL 3.3 context: %s", SDL_GetError());
            return false;
        }
    }
    SDL_Log("GPU Renderer: OpenGL context created successfully");
    
    // Make context current
    SDL_Log("GPU Renderer: Making context current...");
    if (SDL_GL_MakeCurrent(window, gl_context) < 0) {
        SDL_Log("Failed to make GL context current: %s", SDL_GetError());
        return false;
    }
    
    // Initialize GLAD
    SDL_Log("GPU Renderer: Loading OpenGL extensions with GLAD...");
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to initialize GLAD");
        return false;
    }
    SDL_Log("GPU Renderer: GLAD loaded successfully");
    
    // Verify OpenGL version
    const char* version = (const char*)glGetString(GL_VERSION);
    const char* glsl_version = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    
    SDL_Log("=== OpenGL Context Information ===");
    SDL_Log("OpenGL Version: %s", version ? version : "Unknown");
    SDL_Log("GLSL Version: %s", glsl_version ? glsl_version : "Unknown");
    SDL_Log("Renderer: %s", renderer ? renderer : "Unknown");
    SDL_Log("Vendor: %s", vendor ? vendor : "Unknown");
    
    // Check for minimum OpenGL 3.3 support
    if (!GLAD_GL_VERSION_3_3) {
        SDL_Log("OpenGL 3.3 not supported! GPU renderer requires at least OpenGL 3.3");
        return false;
    }
    
    if (GLAD_GL_VERSION_4_6) {
        SDL_Log("OpenGL 4.6 supported - using advanced features");
    } else if (GLAD_GL_VERSION_4_0) {
        SDL_Log("OpenGL 4.0 supported - using most features");
    } else {
        SDL_Log("OpenGL 3.3 supported - using basic features");
    }
    
    // Enable advanced OpenGL features
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // CRITICAL FIX: Disable depth testing for 2D sprites to prevent z-fighting artifacts
    glDisable(GL_DEPTH_TEST);
    // DISABLE MSAA - might be causing artifacts
    glDisable(GL_MULTISAMPLE);
    
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
    
    // Verify initialization completed successfully
    if (!glIsProgram(main_program)) {
        SDL_Log("ERROR: GPU renderer initialization failed - invalid shader program");
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
    // CRITICAL FIX: Ensure our context is active before any GL operations
    if (SDL_GL_MakeCurrent(window, gl_context) < 0) {
        SDL_Log("ERROR: Failed to make context current before loading shaders: %s", SDL_GetError());
        return false;
    }

    // PASO 1: Cargar shaders principales y procesarlos
    std::string vertex_src = Resources::load_shader_source("shaders/optimized_vertex_simple.glsl");
    std::string fragment_src = Resources::load_shader_source("shaders/optimized_fragment_simple.glsl");
    
    // PASO 2: Preprocesar los includes manualmente (más confiable)
    fragment_src = preprocess_shader_includes(fragment_src);
    
    if (vertex_src.empty() || fragment_src.empty()) {
        SDL_Log("Failed to load main shader sources");
        return false;
    }
    
    GLuint vertex_shader = compile_shader(vertex_src, GL_VERTEX_SHADER, "main_vertex");
    if (!vertex_shader) {
        SDL_Log("ERROR: Vertex shader compilation failed!");
        return false;
    }
    
    GLuint fragment_shader = compile_shader(fragment_src, GL_FRAGMENT_SHADER, "main_fragment");
    if (!fragment_shader) {
        SDL_Log("ERROR: Fragment shader compilation failed!");
        glDeleteShader(vertex_shader);
        return false;
    }
    
    main_program = create_program(vertex_shader, fragment_shader, "main_program");
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    
    if (!main_program) {
        SDL_Log("ERROR: Shader program linking failed!");
        return false;
    }
    SDL_Log("Shaders compiled and linked successfully");
    
    // Get uniform locations for main program
    glUseProgram(main_program);
    u_projection = glGetUniformLocation(main_program, "uProjection");
    u_view = glGetUniformLocation(main_program, "uView");
    // u_model = glGetUniformLocation(main_program, "uModel");
    u_time = glGetUniformLocation(main_program, "uTimeData");
    u_explosion_center = glGetUniformLocation(main_program, "uExplosionCenter");
    u_explosion_size = glGetUniformLocation(main_program, "uExplosionSize");
    
    // Get texture uniform locations
    u_texture = glGetUniformLocation(main_program, "uTexture");
    u_resolution = glGetUniformLocation(main_program, "uResolution");
    
    // Set texture units (these don't change)
    if (u_texture >= 0) {
        glUniform1i(u_texture, 0);
    }
    
    // Set resolution
    if (u_resolution >= 0) {
        glUniform2f(u_resolution, (float)screen_width, (float)screen_height);
    }
    
    // Get effect uniform locations for simple shader compatibility
    u_effect_type = glGetUniformLocation(main_program, "uEffectType");
    u_effect_params = -1; // Not used - effect params handled in shader constants
    
    // Get SPECTACULAR effect uniform locations
    u_explosion_data = glGetUniformLocation(main_program, "uExplosionData");
    u_vortex_data = glGetUniformLocation(main_program, "uVortexData");
    u_air_density = glGetUniformLocation(main_program, "uAirDensity");
    u_magnetic_field = glGetUniformLocation(main_program, "uMagneticField");
    u_noise_lut = glGetUniformLocation(main_program, "uNoiseLUT");
    
    // Initialize spectacular effect parameters
    glm_vec4_zero(explosion_data);
    glm_vec4_zero(vortex_data);
    air_density = 1.0f;
    glm_vec2_zero(magnetic_field);
    noise_lut_texture = 0;
    turbulence_texture = 0;
    
    // Load compute shader for particles
    std::string compute_src = Resources::load_shader_source("shaders/optimized_compute.glsl");
    if (!compute_src.empty()) {
        GLuint compute_shader = compile_shader(compute_src, GL_COMPUTE_SHADER, "particle_compute");
        if (compute_shader) {
            particle_compute_program = create_compute_program(compute_shader, "particle_system");
            glDeleteShader(compute_shader);
            
            if (particle_compute_program) {
                glUseProgram(particle_compute_program);
                u_delta_time = glGetUniformLocation(particle_compute_program, "uDeltaTime");
                u_physics_constants = glGetUniformLocation(particle_compute_program, "uPhysicsConstants");
                u_world_size = glGetUniformLocation(particle_compute_program, "uWorldSize");
                u_turbulence_field = glGetUniformLocation(particle_compute_program, "uTurbulenceField");
                
                // SPECTACULAR compute uniforms
                GLint u_explosion_compute = glGetUniformLocation(particle_compute_program, "uExplosionData");
                GLint u_vortex_compute = glGetUniformLocation(particle_compute_program, "uVortexData");
                GLint u_air_density_compute = glGetUniformLocation(particle_compute_program, "uAirDensity");
                GLint u_magnetic_compute = glGetUniformLocation(particle_compute_program, "uMagneticField");
                
                SDL_Log("Compute shader uniforms initialized - spectacular effects ready!");
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
    check_gl_error("attach vertex shader");
    
    glAttachShader(program, fragment);
    check_gl_error("attach fragment shader");
    
    glLinkProgram(program);
    check_gl_error("link program");
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    
    if (!success) {
        GLchar info_log[2048];
        glGetProgramInfoLog(program, sizeof(info_log), nullptr, info_log);
        SDL_Log("ERROR: Program linking error (%s): %s", name.c_str(), info_log);
        
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
    // Create orthographic projection to match SDL coordinate system
    // SDL: (0,0) = top-left, Y increases downward
    // OpenGL: We flip Y so (0,0) maps to top-left in the final image
    glm_ortho(0.0f, (float)screen_width, (float)screen_height, 0.0f, -1000.0f, 1000.0f, projection_matrix);
    
    // Initialize view and model matrices
    glm_mat4_identity(view_matrix);
    glm_mat4_identity(model_matrix);
    
    // Set up OpenGL viewport to match SDL window
    glViewport(0, 0, screen_width, screen_height);
    check_gl_error("set viewport");
    
    SDL_Log("GPU Renderer: Set up matrices and viewport for %dx%d screen (SDL coordinate system)", screen_width, screen_height);
}

void GPUAcceleratedRenderer::setup_sprite_rendering() {
    // Generate VAO and buffers
    glGenVertexArrays(1, &sprite_vao);
    glGenBuffers(1, &sprite_vbo);
    glGenBuffers(1, &sprite_ebo);
    
    glBindVertexArray(sprite_vao);
    
    // Setup dynamic vertex buffer using simple vertex structure
    glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
    size_t buffer_size = MAX_QUADS * 4 * sizeof(SimpleVertex);
    glBufferData(GL_ARRAY_BUFFER, buffer_size, nullptr, GL_DYNAMIC_DRAW);
    
    // Verify buffer was created successfully
    GLint actual_buffer_size = 0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &actual_buffer_size);
    
    if (actual_buffer_size == 0) {
        SDL_Log("ERROR: Failed to create VBO! OpenGL may be out of memory or context invalid");
        check_gl_error("VBO creation failed");
    }
    
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
    
    // Setup vertex attributes for advanced vertex structure
    // Position (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(AdvancedVertex), 
                         (void*)offsetof(AdvancedVertex, position));
    glEnableVertexAttribArray(0);
    
    // Texture coordinates (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(AdvancedVertex), 
                         (void*)offsetof(AdvancedVertex, texCoord));
    glEnableVertexAttribArray(1);
    
    // Color (location 2)
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(AdvancedVertex), 
                         (void*)offsetof(AdvancedVertex, color));
    glEnableVertexAttribArray(2);
    
    // Rotation (location 3)
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(AdvancedVertex), 
                         (void*)offsetof(AdvancedVertex, rotation));
    glEnableVertexAttribArray(3);
    
    // Scale (location 4)
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(AdvancedVertex), 
                         (void*)offsetof(AdvancedVertex, scale));
    glEnableVertexAttribArray(4);
    
    // Effect type (location 5)
    glVertexAttribIPointer(5, 1, GL_INT, sizeof(AdvancedVertex), 
                          (void*)offsetof(AdvancedVertex, effectType));
    glEnableVertexAttribArray(5);
    
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


void GPUAcceleratedRenderer::check_gl_error(const std::string& operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        const char* error_str = "Unknown";
        switch(error) {
            case GL_INVALID_ENUM: error_str = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: error_str = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: error_str = "GL_INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY: error_str = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error_str = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        SDL_Log("OpenGL error in %s: 0x%x (%s)", operation.c_str(), error, error_str);
    }
}

// Preprocesador manual de includes para shaders
std::string GPUAcceleratedRenderer::preprocess_shader_includes(const std::string& source) {
    std::string result = source;
    size_t pos = 0;
    
    // Buscar todas las directivas #include
    while ((pos = result.find("#include", pos)) != std::string::npos) {
        // Encontrar el inicio y fin del nombre del archivo
        size_t quote_start = result.find("\"", pos);
        if (quote_start == std::string::npos) {
            pos++;
            continue;
        }
        
        size_t quote_end = result.find("\"", quote_start + 1);
        if (quote_end == std::string::npos) {
            pos++;
            continue;
        }
        
        // Extraer el nombre del archivo
        std::string filename = result.substr(quote_start + 1, quote_end - quote_start - 1);
        SDL_Log("Processing shader include: %s", filename.c_str());
        
        // Cargar el contenido del archivo incluido
        std::string include_content = Resources::load_shader_source("shaders/" + filename);
        if (include_content.empty()) {
            SDL_Log("ERROR: Failed to load included shader file: %s", filename.c_str());
            pos++;
            continue;
        }
        
        // Encontrar el final de la línea #include
        size_t line_end = result.find("\n", pos);
        if (line_end == std::string::npos) {
            line_end = result.length();
        } else {
            line_end++; // Incluir el newline
        }
        
        // Reemplazar la línea #include con el contenido del archivo
        result.replace(pos, line_end - pos, include_content + "\n");
        
        // Continuar la búsqueda desde después del contenido insertado
        pos += include_content.length();
    }
    
    return result;
}

// Additional methods will be implemented as needed...
void GPUAcceleratedRenderer::begin_batch(EffectType effect) {
    if (current_quad_count > 0 && current_effect != effect) {
        flush_batch();
    }
    current_effect = effect;
}

void GPUAcceleratedRenderer::flush_batch() {
    if (current_quad_count == 0) {
        return;
    }

    // Safety checks to prevent crash
    if (!gl_context || !main_program || !sprite_vao || !sprite_vbo) {
        SDL_Log("GPU Renderer: Critical objects not initialized, skipping batch");
        current_quad_count = 0;
        batch_vertices.clear();
        return;
    }
    
    if (batch_vertices.empty()) {
        current_quad_count = 0;
        return;
    }
    
    // Validate our data before attempting to upload
    if (batch_vertices.size() != current_quad_count * 4) {
        SDL_Log("ERROR: Vertex count mismatch! Expected %d, got %zu", 
                current_quad_count * 4, batch_vertices.size());
        current_quad_count = 0;
        batch_vertices.clear();
        return;
    }
    
    // Check for valid OpenGL context
    check_gl_error("pre-flush");
    
    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
    check_gl_error("bind array buffer");
    
    // Validate buffer size before upload
    GLint buffer_size = 0;
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size);
    size_t data_size = batch_vertices.size() * sizeof(AdvancedVertex);
    
    // If buffer is 0, try to recreate it
    if (buffer_size == 0) {
        SDL_Log("WARNING: Buffer size is 0, attempting to recreate VBO");
        
        // Recreate the buffer
        size_t expected_buffer_size = MAX_QUADS * 4 * sizeof(SimpleVertex);
        glBufferData(GL_ARRAY_BUFFER, expected_buffer_size, nullptr, GL_DYNAMIC_DRAW);
        
        // Check if recreation worked
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size);
        
        if (buffer_size == 0) {
            SDL_Log("ERROR: Failed to recreate VBO! OpenGL context may be lost");
            current_quad_count = 0;
            batch_vertices.clear();
            return;
        }
    }
    
    if (data_size > buffer_size) {
        SDL_Log("ERROR: Data too large for buffer! Data: %zu, Buffer: %d", data_size, buffer_size);
        current_quad_count = 0;
        batch_vertices.clear();
        return;
    }
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, data_size, batch_vertices.data());
    check_gl_error("buffer subdata");
    
    // Validate shader program
    if (!glIsProgram(main_program)) {
        SDL_Log("ERROR: Program %d is no longer a valid OpenGL program object!", main_program);
        current_quad_count = 0;
        batch_vertices.clear();
        return;
    }
    
    // Use main shader program
    glUseProgram(main_program);
    check_gl_error("use program");
    
    // Validate program is linked
    GLint link_status = 0;
    glGetProgramiv(main_program, GL_LINK_STATUS, &link_status);
    if (link_status != GL_TRUE) {
        SDL_Log("ERROR: Shader program %d not linked properly! Link status: %d", main_program, link_status);
        
        // Try to get more info about the program
        GLint info_log_length = 0;
        glGetProgramiv(main_program, GL_INFO_LOG_LENGTH, &info_log_length);
        if (info_log_length > 1) {
            std::vector<char> info_log(info_log_length);
            glGetProgramInfoLog(main_program, info_log_length, nullptr, info_log.data());
            SDL_Log("Program info log: %s", info_log.data());
        }
        
        SDL_Log("WARNING: Continuing despite link status issue...");
    }
    
    update_uniforms();
    check_gl_error("update uniforms");
    
    // Draw
    glBindVertexArray(sprite_vao);
    check_gl_error("bind vao");
    
    // WORKAROUND: Force rebind VBO after VAO (VAO might not be capturing VBO state correctly)
    glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
    check_gl_error("force rebind vbo after vao");
    
    // CRITICAL FIX: Clean OpenGL state to prevent SDL/TTF interference
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind any existing texture first
    check_gl_error("clean texture state");
    
    // Bind the current texture for the batch
    if (current_texture != 0) {
        glBindTexture(GL_TEXTURE_2D, current_texture);
        check_gl_error("bind current texture");
    } else {
        SDL_Log("WARNING: No texture set for batch rendering!");
    }
    
    int triangle_count = current_quad_count * 6;
    
    glUseProgram(main_program); // Re-bind program right before draw call
    glDrawElements(GL_TRIANGLES, triangle_count, GL_UNSIGNED_INT, 0);
    glUseProgram(0); // Unbind program after draw call
    check_gl_error("draw elements");
    
    // Update stats
    perf_stats.draw_calls++;
    perf_stats.vertices_rendered += current_quad_count * 4;
    
    // Reset batch
    batch_vertices.clear();
    current_quad_count = 0;
    current_texture = 0;  // Reset texture state so next texture will be properly set
}

void GPUAcceleratedRenderer::update_uniforms() {
    if (u_projection >= 0) {
        glUniformMatrix4fv(u_projection, 1, GL_FALSE, (float*)projection_matrix);
        check_gl_error("uniform projection");
    }
    
    if (u_view >= 0) {
        glUniformMatrix4fv(u_view, 1, GL_FALSE, (float*)view_matrix);
        check_gl_error("uniform view");
    }
    
    // if (u_model >= 0) {
    //     glUniformMatrix4fv(u_model, 1, GL_FALSE, (float*)model_matrix);
    //     check_gl_error("uniform model");
    // }
    
    // Configure uTexture each frame to avoid context recreation issues
    if (u_texture >= 0) {
        glUniform1i(u_texture, 0);  // Texture unit 0
        check_gl_error("uniform texture");
    }
    
    if (u_time >= 0) {
        // Send vec4 uTimeData as expected by shader: x=time, y=sin(time), z=cos(time), w=time*2
        float time_data[4] = {
            current_time,
            sin(current_time),
            cos(current_time),
            current_time * 2.0f
        };
        glUniform4fv(u_time, 1, time_data);
        check_gl_error("uniform timedata");
    }
    
    if (u_effect_type >= 0) {
        glUniform1i(u_effect_type, (int)current_effect);
        check_gl_error("uniform effect_type");
    }
    
    if (u_effect_params >= 0) {
        glUniform4fv(u_effect_params, 1, global_effect_params);
        check_gl_error("uniform effect_params");
    }
    
    if (u_explosion_center >= 0) {
        glUniform4fv(u_explosion_center, 1, current_explosion_center);
        check_gl_error("uniform explosion_center");
    }
    
    if (u_explosion_size >= 0) {
        glUniform4fv(u_explosion_size, 1, current_explosion_size);
        check_gl_error("uniform explosion_size");
    }
    
    // SPECTACULAR effect uniforms
    if (u_explosion_data >= 0) {
        glUniform4fv(u_explosion_data, 1, explosion_data);
        check_gl_error("uniform explosion_data");
    }
    
    if (u_vortex_data >= 0) {
        glUniform4fv(u_vortex_data, 1, vortex_data);
        check_gl_error("uniform vortex_data");
    }
    
    if (u_air_density >= 0) {
        glUniform1f(u_air_density, air_density);
        check_gl_error("uniform air_density");
    }
    
    if (u_magnetic_field >= 0) {
        glUniform2fv(u_magnetic_field, 1, magnetic_field);
        check_gl_error("uniform magnetic_field");
    }
    
    // Bind noise texture if available
    if (u_noise_lut >= 0 && noise_lut_texture > 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, noise_lut_texture);
        glUniform1i(u_noise_lut, 1);
        check_gl_error("uniform noise_lut");
        glActiveTexture(GL_TEXTURE0); // Reset to default
    }
}

void GPUAcceleratedRenderer::add_sprite(float x, float y, float w, float h, GLuint texture, 
                                       const float* color, float rotation, const float* scale, int sprite_number) {
    add_animated_sprite(x, y, w, h, texture, color, rotation, scale, current_effect, sprite_number);
}

void GPUAcceleratedRenderer::add_animated_sprite(float x, float y, float w, float h, GLuint texture,
                                               const float* color, float rotation, const float* scale, EffectType effect, int sprite_number) {
    // Safety check: prevent crash if critical objects not initialized
    if (!gl_context || !main_program || !sprite_vao || !sprite_vbo) {
        SDL_Log("GPU Renderer: Not ready, skipping sprite");
        return;
    }
    
    if (current_quad_count >= MAX_QUADS) {
        flush_batch();
    }

    if (current_effect != effect) {
        flush_batch();
        current_effect = effect;
    }
    if (current_texture != 0 && current_texture != texture) {
        flush_batch();
    }
    current_texture = texture;

    // Simplified safe approach - create basic vertex structure using stack allocation
    // All rotations and effects handled entirely on GPU as requested
    
    // Safe default values
    const float safe_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float safe_scale[2] = {1.0f, 1.0f};
    const float* use_color = color ? color : safe_color;
    const float* use_scale = scale ? scale : safe_scale;
    
    // Basic quad positions for SDL coordinate system (top-left origin)
    float positions[8] = {
        x,     y,     // top-left
        x + w, y,     // top-right  
        x + w, y + h, // bottom-right
        x,     y + h  // bottom-left
    };
    
    // Calculate dynamic UV coordinates from sprite atlas
    float u_start, u_end, v_start, v_end;
    calculate_sprite_uv(texture, sprite_number, u_start, u_end, v_start, v_end);
    
    float texcoords[8] = {
        u_start, v_start,  // top-left
        u_end,   v_start,  // top-right
        u_end,   v_end,    // bottom-right
        u_start, v_end     // bottom-left
    };
    
    // Create 4 advanced vertices with EXPLICIT memory initialization
    for (int i = 0; i < 4; i++) {
        AdvancedVertex vertex;
        // CRITICAL: Explicitly zero ALL memory to prevent any padding garbage
        memset(&vertex, 0, sizeof(AdvancedVertex));
        
        // Position
        glm_vec2_copy((vec2){positions[i * 2], positions[i * 2 + 1]}, vertex.position);
        
        // Texture coordinates
        glm_vec2_copy((vec2){texcoords[i * 2], texcoords[i * 2 + 1]}, vertex.texCoord);
        
        // Color (safe defaults)
        glm_vec4_copy((vec4){use_color[0], use_color[1], use_color[2], use_color[3]}, vertex.color);
        
        // Rotation
        vertex.rotation = rotation;
        
        // Scale (safe defaults)
        glm_vec2_copy((vec2){use_scale[0], use_scale[1]}, vertex.scale);
        
        // Effect type
        vertex.effectType = (int)effect;
        
        // Add to batch
        batch_vertices.push_back(vertex);
    }

    current_quad_count++;
    flush_batch();
}

void GPUAcceleratedRenderer::end_batch() {
    flush_batch();
}

void GPUAcceleratedRenderer::update_particles_gpu(float deltaTime) {
    if (!particle_compute_program || !particle_ssbo) return;
    
    glUseProgram(particle_compute_program);
    
    // Set SPECTACULAR compute uniforms
    glUniform1f(u_delta_time, deltaTime);
    
    // Physics constants packed for efficiency
    if (u_physics_constants >= 0) {
        vec4 physics_data = {
            gravity_force[1],  // gravity_y
            wind_force[0],     // wind_x  
            wind_force[1],     // wind_y
            current_time       // time
        };
        glUniform4fv(u_physics_constants, 1, physics_data);
    }
    
    glUniform2f(u_world_size, (float)screen_width, (float)screen_height);
    
    // Set spectacular effect uniforms for compute shader
    GLint u_explosion_compute = glGetUniformLocation(particle_compute_program, "uExplosionData");
    if (u_explosion_compute >= 0) {
        glUniform4fv(u_explosion_compute, 1, explosion_data);
    }
    
    GLint u_vortex_compute = glGetUniformLocation(particle_compute_program, "uVortexData");
    if (u_vortex_compute >= 0) {
        glUniform4fv(u_vortex_compute, 1, vortex_data);
    }
    
    GLint u_air_density_compute = glGetUniformLocation(particle_compute_program, "uAirDensity");
    if (u_air_density_compute >= 0) {
        glUniform1f(u_air_density_compute, air_density);
    }
    
    GLint u_magnetic_compute = glGetUniformLocation(particle_compute_program, "uMagneticField");
    if (u_magnetic_compute >= 0) {
        glUniform2fv(u_magnetic_compute, 1, magnetic_field);
    }
    
    // Bind turbulence field if available
    if (u_turbulence_field >= 0 && turbulence_texture > 0) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, turbulence_texture);
        glUniform1i(u_turbulence_field, 2);
        glActiveTexture(GL_TEXTURE0); // Reset
    }
    
    // Dispatch SPECTACULAR compute shader with enhanced workgroup size
    GLuint num_groups = (max_gpu_particles + 127) / 128;  // 128 is the new workgroup size
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
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Directamente usamos GL_RGBA como en el test
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    
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

void GPUAcceleratedRenderer::register_texture_metadata(GLuint texture_id, int width, int height, int sprite_width, int sprite_height) {
    TextureInfo info;
    info.width = width;
    info.height = height;
    info.sprite_width = sprite_width;
    info.sprite_height = sprite_height;
    info.sprites_per_row = width / sprite_width;
    info.sprites_per_col = height / sprite_height;
    
    texture_metadata[texture_id] = info;
}

void GPUAcceleratedRenderer::calculate_sprite_uv(GLuint texture, int sprite_number, float& u_start, float& u_end, float& v_start, float& v_end) {
    // First try to use registered metadata
    auto it = texture_metadata.find(texture);
    if (it != texture_metadata.end()) {
        const TextureInfo& info = it->second;
        
        // Use registered metadata
        int atlas_width = info.width;
        int atlas_height = info.height;
        int sprite_width = info.sprite_width;
        int sprite_height = info.sprite_height;
        int sprites_per_row = info.sprites_per_row;
        
        // Calculate sprite position in atlas
        int sprite_col = sprite_number % sprites_per_row;
        int sprite_row = sprite_number / sprites_per_row;
        
        // Convert to UV coordinates (0.0 to 1.0)
        u_start = (float)(sprite_col * sprite_width) / (float)atlas_width;
        u_end = (float)((sprite_col + 1) * sprite_width) / (float)atlas_width;
        v_start = (float)(sprite_row * sprite_height) / (float)atlas_height;
        v_end = (float)((sprite_row + 1) * sprite_height) / (float)atlas_height;
        
    } else {
        
        // Fallback: use full texture as single sprite
        u_start = 0.0f;
        u_end = 1.0f;
        v_start = 0.0f;
        v_end = 1.0f;
    }
}

// SPECTACULAR EFFECT CONTROL METHODS

void GPUAcceleratedRenderer::set_explosion_effect(float center_x, float center_y, float radius, float strength) {
    explosion_data[0] = center_x;
    explosion_data[1] = center_y;
    explosion_data[2] = radius;
    explosion_data[3] = strength;
}

void GPUAcceleratedRenderer::set_vortex_effect(float center_x, float center_y, float radius, float strength) {
    vortex_data[0] = center_x;
    vortex_data[1] = center_y;
    vortex_data[2] = radius;
    vortex_data[3] = strength;
}

void GPUAcceleratedRenderer::set_environmental_effects(float air_density_value, const float* magnetic_field_value) {
    air_density = air_density_value;
    if (magnetic_field_value) {
        magnetic_field[0] = magnetic_field_value[0];
        magnetic_field[1] = magnetic_field_value[1];
    } else {
        glm_vec2_zero(magnetic_field);
    }
}

void GPUAcceleratedRenderer::clear_effects() {
    // Reset all spectacular effects
    glm_vec4_zero(explosion_data);
    glm_vec4_zero(vortex_data);
    air_density = 1.0f;
    glm_vec2_zero(magnetic_field);
    
    SDL_Log("GPU Renderer: All spectacular effects cleared");
}

void GPUAcceleratedRenderer::set_explosion_info(float center_x, float center_y, float age, int up, int down, int left, int right) {
    current_explosion_center[0] = center_x;
    current_explosion_center[1] = center_y;
    current_explosion_center[2] = age;
    current_explosion_center[3] = 1.0f; // active
    
    current_explosion_size[0] = (float)up;
    current_explosion_size[1] = (float)down;
    current_explosion_size[2] = (float)left;
    current_explosion_size[3] = (float)right;
    
    SDL_Log("DEBUG: Set explosion info - center:(%.1f,%.1f) age:%.3f active:1.0 size:(%d,%d,%d,%d)", 
            center_x, center_y, age, up, down, left, right);
}

void GPUAcceleratedRenderer::clear_explosion_info() {
    glm_vec4_zero(current_explosion_center);
    glm_vec4_zero(current_explosion_size);
}