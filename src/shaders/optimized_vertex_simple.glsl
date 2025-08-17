#version 460 core

// Advanced vertex shader compatible with AdvancedVertex structure
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aRotation;
layout (location = 4) in vec2 aScale;
layout (location = 5) in int aEffectType;

out vec2 TexCoord;
out vec4 Color;
out vec3 WorldPos;
flat out int EffectMode;

uniform mat4 uProjection;
uniform mat4 uView;
uniform vec4 uTimeData; // x=time, y=sin(time), z=cos(time), w=time*2

void main() {
    // Start with vertex position
    vec2 pos = aPos;
    
    // Apply rotation around sprite center if rotation is specified
    if (abs(aRotation) > 0.001) {
        // Calculate sprite center (assuming sprite is positioned at its top-left)
        vec2 spriteSize = vec2(40.0, 40.0); // Standard tile size
        vec2 center = aPos + spriteSize * 0.5;
        
        // Translate to origin, rotate, translate back
        vec2 relativePos = aPos - center;
        float cosR = cos(radians(aRotation));
        float sinR = sin(radians(aRotation));
        
        vec2 rotatedPos;
        rotatedPos.x = relativePos.x * cosR - relativePos.y * sinR;
        rotatedPos.y = relativePos.x * sinR + relativePos.y * cosR;
        
        pos = center + rotatedPos;
    }
    
    // Apply scaling around sprite center
    if (length(aScale - vec2(1.0)) > 0.001) {
        vec2 spriteSize = vec2(40.0, 40.0);
        vec2 center = aPos + spriteSize * 0.5;
        vec2 relativePos = pos - center;
        pos = center + relativePos * aScale;
    }
    
    // Pass effect mode from vertex attribute (per-sprite)
    EffectMode = aEffectType;
    
    // Apply model transformation to final position
    vec3 worldPos = vec3(pos, 0.0);
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
    
    // Pass through other attributes
    TexCoord = aTexCoord;
    Color = aColor;
    WorldPos = worldPos;
}