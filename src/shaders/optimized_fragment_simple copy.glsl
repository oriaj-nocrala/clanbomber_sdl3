#version 460 core

// Optimized fragment shader compatible with SimpleVertex
in vec2 TexCoord;
in vec4 Color;
in vec3 WorldPos;
flat in int EffectMode;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uTimeData; // x=time, y=sin(time), z=cos(time), w=time*2
uniform vec2 uResolution;
uniform vec4 uExplosionCenter; // x,y=center position, z=age, w=active
uniform vec4 uExplosionSize; // x=up, y=down, z=left, w=right (in tiles)

// ==========================================================================
// SHADER DE EXPLOSIÓN EN CRUZ - ADAPTADO PARA CLANBOMBER
// ==========================================================================

// Hash para ruido procedural
float hash1(float n) { 
    return fract(sin(n) * 43758.5453); 
}

// Ruido 2D
float noise(vec2 p) {
    vec2 i = floor(p); 
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash1(i.x + i.y * 57.0);
    float b = hash1(i.x + 1.0 + i.y * 57.0);
    float c = hash1(i.x + (i.y + 1.0) * 57.0);
    float d = hash1(i.x + 1.0 + (i.y + 1.0) * 57.0);
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// FBM (Fractal Brownian Motion)
float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; ++i) {
        v += a * noise(p);
        p = p * 2.0;
        a *= 0.5;
    }
    return v;
}

// SDF para caja (cruz)
float sdBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

// Paleta avanzada de fuego
vec4 firePalette(float heat, float life) {
    heat = clamp(heat, 0.0, 1.0);
    
    // Colores de fuego: rojo -> naranja -> amarillo -> blanco
    vec3 fire_col = vec3(heat * 2.0, heat * heat * 1.5, pow(heat, 4.0));
    fire_col = mix(fire_col, vec3(1.0, 1.0, 0.9), smoothstep(0.8, 1.0, heat));
    
    // Humo al final
    float smoke_heat = heat * 1.2;
    vec3 smoke_col = vec3(0.15, 0.12, 0.1) * smoke_heat;
    
    float smoke_transition = smoothstep(0.80, 0.95, life);
    vec3 final_col = mix(fire_col, smoke_col, smoke_transition);
    
    float opacity = (heat * 0.8 + pow(heat, 4.0) * 0.6) * (1.0 - smoke_transition) +
                (smoke_heat * 1.0) * smoke_transition;
    
    float dissipation = 1.0 - smoothstep(0.9, 1.0, life);
    return vec4(final_col, opacity * dissipation);
}

void main() {
    float time = uTimeData.x;
    vec4 texColor = texture(uTexture, TexCoord);
    vec3 finalColor = texColor.rgb;
    
    // 🔥 DETECTAR EXPLOSIONES: Si estamos en modo EXPLOSION_HEAT, aplicar efecto directamente
    bool hasActiveExplosion = (uExplosionCenter.w > 0.0);
    bool isExplosionMode = (EffectMode == 2); // EXPLOSION_HEAT
    
    if (hasActiveExplosion && isExplosionMode) {
        // Obtener posición del sprite en coordenadas del mundo
        vec2 spritePos = WorldPos.xy;
        vec2 explosionCenter = uExplosionCenter.xy;
        float explosion_age = uExplosionCenter.z;
        
        // Como estamos en modo EXPLOSION_HEAT, aplicar efecto directamente
        // Ya sabemos que este tile es parte de la explosión
        
        // =============================================================
        // EXPLOSIÓN EN CRUZ PROCEDURAL - COORDENADAS GLOBALES
        // =============================================================
        
        // Usar coordenadas mundiales relativas al centro de explosión
        vec2 worldPos = WorldPos.xy;
        vec2 offset = worldPos - explosionCenter;
        
        // Normalizar offset basado en el tamaño de la explosión  
        float tile_size = 40.0;
        float max_extent = max(max(uExplosionSize.x, uExplosionSize.y), 
                          max(uExplosionSize.z, uExplosionSize.w)) * tile_size;
        vec2 uv = offset / max_extent;
        
        // Parámetros de explosión
        float anim_duration = 0.5; // Duración total (0.5s como en Explosion.cpp)
        float progress = min(1.0, explosion_age / anim_duration);
        
        // Si la explosión ya terminó, no mostrar efecto
        if (progress >= 1.0) {
            FragColor = vec4(finalColor * Color.rgb, texColor.a * Color.a);
            return;
        }
        
        float t = explosion_age; // Tiempo específico de esta explosión
        
        // Curva de aparición del color
        float color_progress = progress;
        
        // Parámetros de la cruz
        float BEAM_THICKNESS = 0.25;   // Grosor más visible
        float TURBULENCE_INTENSITY = 2.0;
        
        // Normalizar dimensiones de la explosión
        float max_size = max(max(uExplosionSize.x, uExplosionSize.y), 
                            max(uExplosionSize.z, uExplosionSize.w));
        float reach_up = uExplosionSize.x / max_size;
        float reach_down = uExplosionSize.y / max_size;
        float reach_left = uExplosionSize.z / max_size;
        float reach_right = uExplosionSize.w / max_size;
        
        // SDF para cruz asimétrica
        float dist_h = sdBox(uv, vec2(max(reach_left, reach_right), BEAM_THICKNESS));
        float dist_v = sdBox(uv, vec2(BEAM_THICKNESS, max(reach_up, reach_down)));
        float cross_dist = min(dist_h, dist_v);
        
        // Máscara de forma con borde suave
        float shape = 1.0 - smoothstep(0.0, 0.1, cross_dist);
        
        // NÚCLEO DE IGNICIÓN - más intenso al principio
        float core_life = smoothstep(0.3, 0.0, progress);
        float core_dist = length(uv);
        float core_energy = (1.0 - smoothstep(0.0, 0.4, core_dist)) * core_life * 2.0;
        
        // TURBULENCIA con movimiento
        vec2 p1 = uv * 3.0 + vec2(t * 1.5, t * 0.8);
        vec2 p2 = uv * 6.0 - vec2(t * 2.0, -t * 1.2);
        float turbulence = fbm(p1) * 0.6 + fbm(p2) * 0.4;
        
        // HEAT FINAL - combinando forma, turbulencia y núcleo
        float base_heat = shape * (0.5 + turbulence * 0.5) * TURBULENCE_INTENSITY;
        float heat = base_heat + core_energy;
        heat = heat * (1.0 - progress * 0.7); // Disminuir intensidad con el tiempo
        
        // Aplicar paleta de fuego
        vec4 fire_color = firePalette(heat, progress);
        
        // Mezclar con el color original si hay algo de textura
        vec3 explosion_col = fire_color.rgb;
        float explosion_alpha = fire_color.a;
        
        // Salida final con mezcla aditiva para el fuego
        FragColor = vec4(explosion_col, explosion_alpha);
        return;
    }
    
    // Para otros sprites, usar textura normal
    FragColor = vec4(finalColor * Color.rgb, texColor.a * Color.a);
}