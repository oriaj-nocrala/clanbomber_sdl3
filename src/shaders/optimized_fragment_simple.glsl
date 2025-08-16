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

// Incluir módulo de efectos de explosión
#include "explosion_effects.glsl"

void main() {
    float time = uTimeData.x;
    vec4 texColor = texture(uTexture, TexCoord);
    vec3 finalColor = texColor.rgb;
    
    // 🔥 PROCESAR EFECTOS DE EXPLOSIÓN
    bool isExplosionMode = (EffectMode == 2); // EXPLOSION_HEAT
    
    if (isExplosionMode) {
        vec4 explosionColor;
        bool explosionProcessed = processExplosionEffect(
            WorldPos,
            uExplosionCenter,
            uExplosionSize,
            time,
            explosionColor
        );
        
        if (explosionProcessed) {
            if (explosionColor.a <= 0.0) {
                discard; // Explosión terminada o fuera del área
            }
            FragColor = explosionColor;
            return;
        }
    }
    
    // Para otros sprites (renderizado normal)
    FragColor = vec4(finalColor * Color.rgb, texColor.a * Color.a);
}