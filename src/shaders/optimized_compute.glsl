#version 460 core

// Optimized compute shader with better memory access patterns and reduced calculations
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in; // Optimized workgroup size

// Packed particle structure for better memory layout
struct OptimizedGPUParticle {
    vec4 pos_life;      // x,y=position, z=life, w=max_life
    vec4 vel_size;      // x,y=velocity, z=size, w=mass
    vec4 color;         // rgba color
    vec4 accel_rot;     // x,y=acceleration, z=rotation, w=angular_velocity
    ivec4 type_forces;  // x=type, y=active, z=gravity_scale, w=drag_scale
};

layout(std430, binding = 0) restrict buffer ParticleBuffer {
    OptimizedGPUParticle particles[];
};

uniform float uDeltaTime;
uniform vec4 uPhysicsConstants; // x=gravity_y, y=wind_x, z=wind_y, w=time
uniform vec2 uWorldSize;
uniform sampler2D uTurbulenceField; // Precomputed turbulence field

// Fast pseudo-random using integer hash
uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

float random(uint seed) {
    return float(hash(seed)) / 4294967296.0;
}

void main() {
    uint index = gl_GlobalInvocationID.x;
    if (index >= particles.length()) return;
    
    OptimizedGPUParticle p = particles[index];
    if (p.type_forces.y == 0) return; // Not active
    
    // Update lifetime
    p.pos_life.z -= uDeltaTime;
    if (p.pos_life.z <= 0.0) {
        p.type_forces.y = 0; // Deactivate
        particles[index] = p;
        return;
    }
    
    // Precompute commonly used values
    float invMass = 1.0 / p.vel_size.w;
    vec2 position = p.pos_life.xy;
    vec2 velocity = p.vel_size.xy;
    
    // Reset acceleration
    p.accel_rot.xy = vec2(0.0);
    
    // Apply gravity with type-specific scaling
    float gravityScale = float(p.type_forces.z) * 0.1; // Convert int to float scale
    p.accel_rot.y += uPhysicsConstants.x * gravityScale * invMass;
    
    // Apply drag - optimized for common case
    float speed = length(velocity);
    if (speed > 0.1) {
        float dragScale = float(p.type_forces.w) * 0.01;
        vec2 dragForce = -dragScale * speed * velocity; // Simplified drag
        p.accel_rot.xy += dragForce * invMass;
    }
    
    // Wind force
    p.accel_rot.xy += uPhysicsConstants.yz * invMass;
    
    // Turbulence from precomputed field (only for smoke and fire)
    if (p.type_forces.x == 1 || p.type_forces.x == 3) {
        vec2 turbulenceUV = position / uWorldSize;
        vec2 turbulence = (texture(uTurbulenceField, turbulenceUV + uPhysicsConstants.w * 0.01).xy - 0.5) * 100.0;
        p.accel_rot.xy += turbulence * invMass;
    }
    
    // Optimized Verlet integration
    vec2 newVelocity = velocity + p.accel_rot.xy * uDeltaTime;
    position += (velocity + newVelocity) * 0.5 * uDeltaTime;
    
    // Update rotation
    p.accel_rot.z += p.accel_rot.w * uDeltaTime;
    p.accel_rot.w *= 0.98; // Angular drag
    
    // Boundary collision - branchless where possible
    float restitution = 0.6;
    float friction = 0.8;
    
    // X boundaries
    if (position.x < 0.0) {
        position.x = 0.0;
        newVelocity.x *= -restitution;
        p.accel_rot.w += newVelocity.y * 0.1;
    } else if (position.x > uWorldSize.x) {
        position.x = uWorldSize.x;
        newVelocity.x *= -restitution;
        p.accel_rot.w -= newVelocity.y * 0.1;
    }
    
    // Y boundaries
    if (position.y < 0.0) {
        position.y = 0.0;
        newVelocity.y *= -restitution;
        newVelocity.x *= friction;
        p.accel_rot.w += newVelocity.x * 0.1;
    } else if (position.y > uWorldSize.y) {
        position.y = uWorldSize.y;
        newVelocity.y *= -restitution;
        newVelocity.x *= friction;
        p.accel_rot.w -= newVelocity.x * 0.1;
    }
    
    // Update particle properties based on age and type - optimized
    float ageRatio = 1.0 - (p.pos_life.z / p.pos_life.w);
    float ageRatioSq = ageRatio * ageRatio;
    
    // Type-specific updates using switch-like branching
    if (p.type_forces.x == 0) { // Sparks
        p.color.a = 1.0 - ageRatio;
        p.vel_size.z = mix(3.0, 1.0, ageRatio);
        p.color.g = mix(1.0, 0.3, ageRatio);
    }
    else if (p.type_forces.x == 1) { // Smoke
        p.color.a = mix(0.8, 0.0, ageRatioSq);
        p.vel_size.z = mix(2.0, 8.0, ageRatio);
        float gray = mix(0.7, 0.3, ageRatio);
        p.color.rgb = vec3(gray);
    }
    else if (p.type_forces.x == 2) { // Blood
        p.color.a = 1.0 - ageRatio * 0.5;
        p.vel_size.z = mix(2.0, 1.5, ageRatio);
        p.color.r = mix(0.8, 0.3, ageRatio);
    }
    else if (p.type_forces.x == 3) { // Fire
        p.color.a = mix(1.0, 0.0, ageRatioSq);
        p.vel_size.z = mix(2.5, 4.0, ageRatio);
        p.color.g = mix(1.0, 0.3, ageRatio);
        p.color.b = mix(0.8, 0.0, ageRatio);
    }
    
    // Write back optimized data
    p.pos_life.xy = position;
    p.vel_size.xy = newVelocity;
    particles[index] = p;
}