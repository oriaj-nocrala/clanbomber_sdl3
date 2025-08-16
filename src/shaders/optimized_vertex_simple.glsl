#version 460 core

// Optimized vertex shader compatible with SimpleVertex structure
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec4 Color;
out vec3 WorldPos;
flat out int EffectMode; // Pass effect mode to fragment shader

uniform mat4 uProjection;
uniform mat4 uView;
uniform vec4 uTimeData; // x=time, y=sin(time), z=cos(time), w=time*2
uniform int uEffectType; // Effect type from batching system

void main() {
    // Start with basic rendering but add time-aware capabilities
    vec2 pos = aPos;
    EffectMode = uEffectType; // Use effect type from uniform (restored)
    
    // For now, just pass through - but we have time data available for future effects
    // float time = uTimeData.x; // Available for future use
    
    // Apply model transformation
    vec3 worldPos = vec3(pos, 0.0);
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
    
    TexCoord = aTexCoord;
    Color = vec4(1.0, 1.0, 1.0, 1.0); // Default white color
    WorldPos = worldPos;
}