#version 460 core

// SPECTACULAR high-density particle compute shader with advanced physics
layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in; // Enhanced workgroup size for better GPU utilization

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
uniform vec4 uExplosionData; // x,y=center, z=radius, w=strength
uniform vec4 uVortexData; // x,y=center, z=radius, w=strength
uniform float uAirDensity; // Environmental air density
uniform vec2 uMagneticField; // Magnetic field for charged particles

// Enhanced pseudo-random with better distribution
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

// 2D random for better particle distribution
vec2 random2D(uint seed) {
    return vec2(
        float(hash(seed)) / 4294967296.0,
        float(hash(seed + 1u)) / 4294967296.0
    );
}

// Smooth noise for organic particle behavior
float smoothNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f); // Smoothstep
    
    float a = random(uint(i.x + i.y * 57.0));
    float b = random(uint(i.x + 1.0 + i.y * 57.0));
    float c = random(uint(i.x + (i.y + 1.0) * 57.0));
    float d = random(uint(i.x + 1.0 + (i.y + 1.0) * 57.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
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
    
    // ENHANCED PHYSICS SIMULATION
    
    // Apply gravity with realistic scaling
    float gravityScale = float(p.type_forces.z) * 0.1;
    vec2 gravityForce = vec2(0.0, uPhysicsConstants.x) * gravityScale * invMass;
    p.accel_rot.xy += gravityForce;
    
    // Advanced fluid dynamics simulation
    float speed = length(velocity);
    if (speed > 0.01) {
        float dragScale = float(p.type_forces.w) * 0.01;
        
        // Quadratic drag for realistic physics
        float speedSq = speed * speed;
        float dragMagnitude = dragScale * speedSq * uAirDensity;
        vec2 dragDirection = -normalize(velocity);
        vec2 dragForce = dragDirection * dragMagnitude * invMass;
        
        p.accel_rot.xy += dragForce;
        
        // Magnus effect for spinning particles
        if (abs(p.accel_rot.w) > 0.1) {
            vec2 perp = vec2(-velocity.y, velocity.x);
            vec2 magnusForce = perp * p.accel_rot.w * speed * 0.01 * invMass;
            p.accel_rot.xy += magnusForce;
        }
    }
    
    // Enhanced wind and environmental forces
    vec2 windForce = uPhysicsConstants.yz;
    
    // Wind turbulence varies by particle type and altitude
    float windInfluence = 1.0;
    if (p.type_forces.x == 1) windInfluence = 2.0; // Smoke affected more by wind
    if (p.type_forces.x == 2) windInfluence = 0.3; // Blood less affected
    
    // Altitude-based wind variation
    float altitudeFactor = 1.0 + (position.y / uWorldSize.y) * 0.5;
    windForce *= windInfluence * altitudeFactor;
    
    p.accel_rot.xy += windForce * invMass;
    
    // SPECTACULAR ENVIRONMENTAL FORCES
    
    // Enhanced turbulence for all particle types
    vec2 turbulenceUV = position / uWorldSize;
    vec2 turbulence = (texture(uTurbulenceField, turbulenceUV + uPhysicsConstants.w * 0.02).xy - 0.5) * 150.0;
    
    // Type-specific turbulence response
    float turbulenceStrength = 1.0;
    if (p.type_forces.x == 1) turbulenceStrength = 2.5; // Smoke: high turbulence
    else if (p.type_forces.x == 2) turbulenceStrength = 0.4; // Blood: low turbulence
    else if (p.type_forces.x == 3) turbulenceStrength = 2.0; // Fire: high turbulence
    else if (p.type_forces.x == 0) turbulenceStrength = 0.8; // Sparks: medium turbulence
    
    p.accel_rot.xy += turbulence * turbulenceStrength * invMass;
    
    // Explosion forces
    if (uExplosionData.w > 0.0) {
        vec2 toExplosion = position - uExplosionData.xy;
        float distToExplosion = length(toExplosion);
        
        if (distToExplosion < uExplosionData.z && distToExplosion > 1.0) {
            vec2 explosionDir = toExplosion / distToExplosion;
            float explosionForce = uExplosionData.w / (distToExplosion * distToExplosion + 1.0);
            p.accel_rot.xy += explosionDir * explosionForce * invMass;
            
            // Add rotational impulse from explosion
            p.accel_rot.w += (random(index + uint(uPhysicsConstants.w * 100.0)) - 0.5) * explosionForce * 0.1;
        }
    }
    
    // Vortex forces for portal effects
    if (uVortexData.w > 0.0) {
        vec2 toVortex = position - uVortexData.xy;
        float vortexDist = length(toVortex);
        
        if (vortexDist < uVortexData.z && vortexDist > 5.0) {
            // Spiral force
            float angle = atan(toVortex.y, toVortex.x);
            vec2 tangent = vec2(-sin(angle), cos(angle));
            float vortexStrength = uVortexData.w / (vortexDist + 10.0);
            
            // Spiral inward
            vec2 vortexForce = tangent * vortexStrength;
            vortexForce += -normalize(toVortex) * vortexStrength * 0.3; // Inward pull
            
            p.accel_rot.xy += vortexForce * invMass;
            p.accel_rot.w += vortexStrength * 0.5; // Spin particles
        }
    }
    
    // Magnetic forces for charged particles (sparks)
    if (p.type_forces.x == 0 && length(uMagneticField) > 0.0) {
        // Lorentz force: F = q(v × B)
        vec2 magneticForce = vec2(
            velocity.y * uMagneticField.x,
            -velocity.x * uMagneticField.y
        ) * 0.5 * invMass;
        p.accel_rot.xy += magneticForce;
    }
    
    // Optimized Verlet integration
    vec2 newVelocity = velocity + p.accel_rot.xy * uDeltaTime;
    position += (velocity + newVelocity) * 0.5 * uDeltaTime;
    
    // Update rotation
    p.accel_rot.z += p.accel_rot.w * uDeltaTime;
    p.accel_rot.w *= 0.98; // Angular drag
    
    // ADVANCED BOUNDARY COLLISION with material properties
    float restitution = 0.6;
    float friction = 0.8;
    
    // Material-specific collision properties
    if (p.type_forces.x == 0) { // Sparks
        restitution = 0.3; friction = 0.9; // Sparks lose energy quickly
    } else if (p.type_forces.x == 1) { // Smoke
        restitution = 0.1; friction = 0.95; // Smoke dissipates on contact
    } else if (p.type_forces.x == 2) { // Blood
        restitution = 0.2; friction = 0.7; // Blood is viscous
    } else if (p.type_forces.x == 3) { // Fire
        restitution = 0.1; friction = 0.9; // Fire extinguishes on contact
    }
    
    // Enhanced boundary collisions with energy transfer
    bool collided = false;
    
    // X boundaries with surface interaction
    if (position.x < 0.0) {
        position.x = 0.0;
        newVelocity.x *= -restitution;
        newVelocity.y *= friction;
        p.accel_rot.w += newVelocity.y * 0.2; // Angular momentum from collision
        collided = true;
    } else if (position.x > uWorldSize.x) {
        position.x = uWorldSize.x;
        newVelocity.x *= -restitution;
        newVelocity.y *= friction;
        p.accel_rot.w -= newVelocity.y * 0.2;
        collided = true;
    }
    
    // Y boundaries with ground/ceiling effects
    if (position.y < 0.0) {
        position.y = 0.0;
        newVelocity.y *= -restitution;
        newVelocity.x *= friction;
        p.accel_rot.w += newVelocity.x * 0.15;
        collided = true;
        
        // Ground impact effects
        if (p.type_forces.x == 0 && length(newVelocity) > 50.0) {
            // Sparks on hard impact
            p.color.rgb *= 1.5;
            p.color.a = min(1.0, p.color.a + 0.3);
        }
    } else if (position.y > uWorldSize.y) {
        position.y = uWorldSize.y;
        newVelocity.y *= -restitution;
        newVelocity.x *= friction;
        p.accel_rot.w -= newVelocity.x * 0.15;
        collided = true;
    }
    
    // Energy loss from collision
    if (collided) {
        // Some particle types are affected more by collisions
        if (p.type_forces.x == 1 || p.type_forces.x == 3) { // Smoke and fire
            p.pos_life.z *= 0.8; // Reduced lifetime from collision
        }
    }
    
    // Update particle properties based on age and type - optimized
    float ageRatio = 1.0 - (p.pos_life.z / p.pos_life.w);
    float ageRatioSq = ageRatio * ageRatio;
    
    // ENHANCED type-specific particle behavior
    if (p.type_forces.x == 0) { // SPARKS - Electric/metallic particles
        p.color.a = 1.0 - ageRatio;
        p.vel_size.z = mix(2.0, 0.5, ageRatio);
        
        // Hot spark coloring: white-hot to red-hot to dark
        float heat = 1.0 - ageRatio;
        if (heat > 0.8) {
            p.color.rgb = vec3(1.0, 1.0, 0.9); // White-hot
        } else if (heat > 0.5) {
            p.color.rgb = mix(vec3(1.0, 0.6, 0.0), vec3(1.0, 1.0, 0.9), (heat - 0.5) * 3.33);
        } else {
            p.color.rgb = mix(vec3(0.3, 0.0, 0.0), vec3(1.0, 0.6, 0.0), heat * 2.0);
        }
        
        // Sparks flicker
        if (random(index + uint(uPhysicsConstants.w * 30.0)) > 0.9) {
            p.color.rgb *= 1.5; // Bright flicker
        }
    }
    else if (p.type_forces.x == 1) { // SMOKE - Realistic smoke dynamics
        p.color.a = mix(0.9, 0.0, ageRatioSq * ageRatio); // Cubic fade for realism
        p.vel_size.z = mix(1.5, 12.0, ageRatio); // Smoke expands significantly
        
        // Smoke color varies with age and temperature
        vec3 hotSmoke = vec3(0.2, 0.2, 0.2); // Dark hot smoke
        vec3 coolSmoke = vec3(0.8, 0.8, 0.8); // Light cool smoke
        vec3 oldSmoke = vec3(0.6, 0.6, 0.7);  // Bluish old smoke
        
        if (ageRatio < 0.3) {
            p.color.rgb = mix(hotSmoke, coolSmoke, ageRatio * 3.33);
        } else {
            p.color.rgb = mix(coolSmoke, oldSmoke, (ageRatio - 0.3) * 1.43);
        }
        
        // Add noise to smoke opacity for realism
        float smokeNoise = smoothNoise(position * 0.01 + uPhysicsConstants.w * 0.5);
        p.color.a *= 0.8 + 0.4 * smokeNoise;
    }
    else if (p.type_forces.x == 2) { // BLOOD - Viscous fluid dynamics
        p.color.a = 1.0 - ageRatio * 0.3; // Blood stays visible longer
        p.vel_size.z = mix(1.5, 1.0, ageRatio); // Blood droplets don't expand much
        
        // Blood oxidation: bright red -> dark red -> brown
        vec3 freshBlood = vec3(0.8, 0.0, 0.0);
        vec3 oldBlood = vec3(0.4, 0.1, 0.0);
        vec3 driedBlood = vec3(0.3, 0.1, 0.05);
        
        if (ageRatio < 0.5) {
            p.color.rgb = mix(freshBlood, oldBlood, ageRatio * 2.0);
        } else {
            p.color.rgb = mix(oldBlood, driedBlood, (ageRatio - 0.5) * 2.0);
        }
        
        // Blood coagulation - becomes stickier over time
        if (ageRatio > 0.7) {
            p.vel_size.xy *= 0.95; // Gradual slowdown
        }
    }
    else if (p.type_forces.x == 3) { // FIRE - Advanced combustion simulation
        p.color.a = mix(1.0, 0.0, ageRatioSq * ageRatio); // Flames fade quickly
        p.vel_size.z = mix(2.0, 6.0, ageRatio);
        
        // Realistic flame coloring based on temperature
        float temperature = 1.0 - ageRatio;
        vec3 blueFlame = vec3(0.3, 0.5, 1.0);   // Hottest
        vec3 whiteFlame = vec3(1.0, 1.0, 0.9);  // Very hot
        vec3 yellowFlame = vec3(1.0, 0.8, 0.0); // Hot
        vec3 orangeFlame = vec3(1.0, 0.4, 0.0); // Medium
        vec3 redFlame = vec3(0.8, 0.0, 0.0);    // Cooler
        
        if (temperature > 0.9) {
            p.color.rgb = mix(whiteFlame, blueFlame, (temperature - 0.9) * 10.0);
        } else if (temperature > 0.7) {
            p.color.rgb = mix(yellowFlame, whiteFlame, (temperature - 0.7) * 5.0);
        } else if (temperature > 0.4) {
            p.color.rgb = mix(orangeFlame, yellowFlame, (temperature - 0.4) * 3.33);
        } else {
            p.color.rgb = mix(redFlame, orangeFlame, temperature * 2.5);
        }
        
        // Fire flicker effect
        float flicker = smoothNoise(position * 0.02 + uPhysicsConstants.w * 8.0);
        p.color.rgb *= 0.8 + 0.4 * flicker;
        
        // Upward buoyancy for fire
        p.accel_rot.y += -50.0 * temperature * invMass;
    }
    else if (p.type_forces.x == 4) { // DEBRIS - Solid fragments
        p.color.a = mix(1.0, 0.6, ageRatio);
        p.vel_size.z = mix(2.0, 1.8, ageRatio); // Debris maintains size
        
        // Debris gets dusty over time
        vec3 material = p.color.rgb;
        vec3 dust = vec3(0.5, 0.4, 0.3);
        p.color.rgb = mix(material, dust, ageRatio * 0.3);
        
        // Debris tumbles more
        p.accel_rot.w += (random(index + uint(uPhysicsConstants.w * 10.0)) - 0.5) * 2.0;
    }
    
    // Particle interaction with other particles (simplified N-body)
    if (index % 4 == 0 && p.type_forces.x != 1) { // Skip for smoke to save performance
        // Check nearby particles for interaction
        uint nearbyStart = max(0u, index - 8u);
        uint nearbyEnd = min(particles.length(), index + 8u);
        
        for (uint i = nearbyStart; i < nearbyEnd; i++) {
            if (i == index || particles[i].type_forces.y == 0) continue;
            
            vec2 otherPos = particles[i].pos_life.xy;
            vec2 toOther = otherPos - position;
            float dist = length(toOther);
            
            if (dist < 10.0 && dist > 0.1) {
                // Particle-particle forces
                vec2 force = normalize(toOther) / (dist * dist + 1.0);
                
                // Attraction/repulsion based on type
                if (p.type_forces.x == particles[i].type_forces.x) {
                    // Same type particles attract slightly (clustering)
                    newVelocity += force * 2.0 * invMass;
                } else {
                    // Different types repel slightly
                    newVelocity -= force * 1.0 * invMass;
                }
            }
        }
    }
    
    // Write back enhanced data
    p.pos_life.xy = position;
    p.vel_size.xy = newVelocity;
    particles[index] = p;
}