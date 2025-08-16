// ==========================================================================
// EXPLOSION EFFECTS MODULE - ClanBomber
// ==========================================================================
// Este archivo contiene toda la lógica de efectos de explosión
// Se incluye en el fragment shader principal para mantener modularidad

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

// Función principal para procesar efectos de explosión
// Retorna true si se procesó una explosión, false en caso contrario
bool processExplosionEffect(
    in vec3 worldPos,
    in vec4 explosionCenter,  // x,y=center, z=age, w=active
    in vec4 explosionSize,    // x=up, y=down, z=left, w=right
    in float time,
    out vec4 explosionColor
) {
    // Verificar si hay una explosión activa
    bool hasActiveExplosion = (explosionCenter.w > 0.0);
    if (!hasActiveExplosion) {
        return false;
    }
    
    vec2 worldPos2D = worldPos.xy;
    vec2 explosionCenterPos = explosionCenter.xy;
    float explosion_age = explosionCenter.z;
    
    // Animación
    float anim_duration = 0.5;
    float progress = min(1.0, explosion_age / anim_duration);
    if (progress >= 1.0) {
        // Explosión terminada - el fragment debe descartarse
        explosionColor = vec4(0.0);
        return true; // Indica que se procesó pero debe descartarse
    }
    
    float t = explosion_age;
    float tile_size = 40.0;
    
    // SISTEMA LIMPIO: WorldPos ya viene en coordenadas mundiales correctas
    // Solo calcular el offset desde el centro de explosión
    vec2 offset = worldPos2D - explosionCenterPos;
    vec2 tile_coords = offset / tile_size;
    
    // Alcances de la explosión (en tiles)
    float reach_up    = explosionSize.x;
    float reach_down  = explosionSize.y;
    float reach_left  = explosionSize.z;
    float reach_right = explosionSize.w;
    
    // Verificar si estamos en la cruz
    float BEAM_WIDTH = 0.5; // Medio tile de ancho
    
    bool in_horizontal = (abs(tile_coords.y) <= BEAM_WIDTH) && 
                       (tile_coords.x >= -reach_left - 0.5) && 
                       (tile_coords.x <= reach_right + 0.5);
                       
    bool in_vertical = (abs(tile_coords.x) <= BEAM_WIDTH) && 
                     (tile_coords.y >= -reach_up - 0.5) && 
                     (tile_coords.y <= reach_down + 0.5);
    
    if (!in_horizontal && !in_vertical) {
        // Fuera de la cruz - el fragment debe descartarse
        explosionColor = vec4(0.0);
        return true; // Indica que se procesó pero debe descartarse
    }
    
    // --- EFECTOS VISUALES ---
    
    // Distancia al centro para efectos
    float dist_to_center = length(tile_coords);
    
    // Forma suave
    float edge_distance = 0.0;
    if (in_horizontal && !in_vertical) {
        edge_distance = abs(tile_coords.y) / BEAM_WIDTH;
    } else if (in_vertical && !in_horizontal) {
        edge_distance = abs(tile_coords.x) / BEAM_WIDTH;
    } else {
        edge_distance = min(abs(tile_coords.y), abs(tile_coords.x)) / BEAM_WIDTH;
    }
    float shape = 1.0 - smoothstep(0.7, 1.0, edge_distance);
    
    // UV normalizada para efectos
    float max_reach = max(max(reach_up, reach_down), max(reach_left, reach_right));
    vec2 normalized_uv = tile_coords / max(max_reach, 1.0);
    
    // Núcleo de explosión
    float core_falloff = 1.0 - smoothstep(0.0, 2.0, dist_to_center);
    float core_intensity = core_falloff * smoothstep(0.4, 0.0, progress) * 3.0;
    
    // Turbulencia
    vec2 turb_uv = normalized_uv * 4.0;
    vec2 p1 = turb_uv + vec2(t * 2.0, t * 1.5);
    vec2 p2 = turb_uv * 2.0 - vec2(t * 3.0, -t * 2.0);
    float turbulence = fbm(p1) * 0.6 + fbm(p2) * 0.4;
    turbulence = turbulence * 0.5 + 0.5;
    
    // Calor final
    float base_heat = shape * 0.8;
    float turb_heat = shape * turbulence * 0.6;
    float heat = base_heat + turb_heat + core_intensity;
    heat *= (1.0 - progress * 0.5);
    heat = clamp(heat, 0.0, 1.0);
    
    // Aplicar paleta de fuego
    explosionColor = firePalette(heat, progress);
    return true; // Efectos procesados exitosamente
}