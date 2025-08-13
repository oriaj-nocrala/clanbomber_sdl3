#version 460 core

// Optimized fragment shader with LUT and reduced noise calculations
in vec2 TexCoord;
in vec4 Color;
in vec3 WorldPos;
flat in int EffectMode;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform sampler2D uNoiseLUT; // Precomputed noise lookup table
uniform vec4 uTimeData; // x=time, y=sin(time), z=cos(time), w=time*2
uniform vec2 uResolution;

// Optimized noise using lookup table
float fastNoise(vec2 p) {
    return texture(uNoiseLUT, p * 0.00390625).r; // Divide by 256 for LUT sampling
}

// Precomputed constants
const vec3 FIRE_COLOR_START = vec3(1.0, 1.0, 0.8);
const vec3 FIRE_COLOR_END = vec3(0.8, 0.3, 0.0);
const vec3 BLOOD_COLOR = vec3(0.6, 0.0, 0.0);
const vec3 RAINBOW_PHASE = vec3(0.0, 2.094, 4.188);

void main() {
    vec4 texColor = texture(uTexture, TexCoord);
    vec4 finalColor = texColor * Color;
    
    // Branch-optimized effect processing
    if (EffectMode == 1) {
        // Particle glow - use precomputed sin
        float glow = 1.0 + 0.5 * sin(uTimeData.w * 6.0 + WorldPos.x * 0.1);
        finalColor.rgb *= glow;
        
        // Radial glow - optimized distance calculation
        vec2 center = TexCoord - vec2(0.5);
        float radialGlow = max(0.0, 1.0 - dot(center, center) * 4.0);
        finalColor.rgb += vec3(radialGlow * 0.3, radialGlow * 0.2, 0.0);
        
        finalColor.a *= 0.8 + 0.2 * uTimeData.y; // Use precomputed sin
        
    } else if (EffectMode == 2) {
        // Heat distortion - single noise lookup
        float noiseVal = fastNoise(TexCoord * 10.0 + uTimeData.xy);
        vec2 distortionOffset = vec2(noiseVal - 0.5, fastNoise(TexCoord * 8.0 + uTimeData.yx) - 0.5) * 0.03;
        
        texColor = texture(uTexture, TexCoord + distortionOffset);
        finalColor = texColor * Color;
        
        // Fire color - optimized
        float heat = sin(uTimeData.w * 3.0 + WorldPos.y * 0.02);
        finalColor.r += heat * 0.3;
        finalColor.g += heat * 0.1;
        
        // Flame particles - single noise check
        if (noiseVal > 0.7) {
            finalColor.rgb += FIRE_COLOR_END * (noiseVal - 0.7) * 3.0;
        }
        
    } else if (EffectMode == 3) {
        // Chromatic aberration - optimized
        float aberration = 0.008 * sin(uTimeData.w * 5.0);
        
        float r = texture(uTexture, TexCoord + vec2(aberration, 0)).r;
        float b = texture(uTexture, TexCoord - vec2(aberration, 0)).b;
        
        finalColor = vec4(r, texColor.g, b, texColor.a) * Color;
        
        // Rainbow shimmer - use precomputed phases
        float shimmer = uTimeData.y + WorldPos.x * 0.05 + WorldPos.y * 0.03;
        vec3 rainbow = vec3(
            sin(shimmer + RAINBOW_PHASE.x) * 0.5 + 0.5,
            sin(shimmer + RAINBOW_PHASE.y) * 0.5 + 0.5,
            sin(shimmer + RAINBOW_PHASE.z) * 0.5 + 0.5
        );
        finalColor.rgb = mix(finalColor.rgb, rainbow, 0.3);
        
        finalColor.a *= 0.6 + 0.4 * sin(uTimeData.w * 6.0);
        
    } else if (EffectMode == 4) {
        // Blood splatter - minimal noise usage
        float bloodNoise = fastNoise(TexCoord * 20.0 + uTimeData.xy * 0.5);
        vec2 bloodUV = TexCoord + vec2(bloodNoise - 0.5, fastNoise(TexCoord * 18.0) - 0.5) * 0.02;
        
        texColor = texture(uTexture, bloodUV);
        finalColor = texColor * Color;
        
        // Blood effects - optimized branching
        float drip = fastNoise(vec2(TexCoord.x * 5.0, TexCoord.y * 10.0 + uTimeData.x * 2.0));
        float bloodMask = step(0.6, drip) * 0.5 + step(0.85, bloodNoise) * 0.7;
        finalColor.rgb = mix(finalColor.rgb, BLOOD_COLOR, bloodMask);
    }
    
    FragColor = finalColor;
}