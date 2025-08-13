#version 460 core

// Optimized vertex shader with reduced branching and precomputed values
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aRotation;
layout (location = 4) in vec2 aScale;
layout (location = 5) in int aEffectMode; // Move effect mode to vertex attribute

out vec2 TexCoord;
out vec4 Color;
out vec3 WorldPos;
flat out int EffectMode; // Pass to fragment shader

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;
uniform vec4 uTimeData; // x=time, y=sin(time), z=cos(time), w=time*2

// Precomputed constants for effects
const vec2 WAVE_FREQ = vec2(4.0, 3.0);
const vec2 WAVE_AMP = vec2(2.0, 1.0);
const vec2 EXPLOSION_CENTER = vec2(400.0, 300.0);
const vec2 PARTICLE_FREQ = vec2(2.0, 1.5);
const vec2 PARTICLE_AMP = vec2(1.5, 1.0);

void main() {
    vec2 pos = aPos;
    EffectMode = aEffectMode;
    
    // Optimized rotation and scaling - avoid branches when possible
    vec2 cossin = vec2(cos(aRotation), sin(aRotation));
    mat2 rotation = mat2(cossin.x, -cossin.y, cossin.y, cossin.x);
    pos = rotation * (pos * aScale);
    
    // Effect calculations using precomputed values
    if (aEffectMode == 1) {
        // Wave effect - use precomputed sin/cos
        pos.x += sin(uTimeData.x * WAVE_FREQ.x + aPos.y * 0.02) * WAVE_AMP.x;
        pos.y += cos(uTimeData.x * WAVE_FREQ.y + aPos.x * 0.02) * WAVE_AMP.y;
    } 
    else if (aEffectMode == 2) {
        // Explosion displacement - optimize distance calculation
        vec2 dir = aPos - EXPLOSION_CENTER;
        float distSq = dot(dir, dir);
        float invDist = inversesqrt(distSq);
        dir *= invDist; // normalized direction
        float wave = sin(uTimeData.w * 4.0 - sqrt(distSq) * 0.05) * 5.0;
        pos += dir * wave;
    }
    else if (aEffectMode == 3) {
        // Particle floating - use precomputed time values
        pos.x += sin(uTimeData.x * PARTICLE_FREQ.x + aPos.y * 0.1) * PARTICLE_AMP.x;
        pos.y += cos(uTimeData.x * PARTICLE_FREQ.y + aPos.x * 0.1) * PARTICLE_AMP.y;
    }
    
    vec4 worldPos = uModel * vec4(pos, 0.0, 1.0);
    gl_Position = uProjection * uView * worldPos;
    
    TexCoord = aTexCoord;
    Color = aColor;
    WorldPos = worldPos.xyz;
}