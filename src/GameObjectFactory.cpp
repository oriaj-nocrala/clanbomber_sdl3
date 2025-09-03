#include "MemoryManagement.h"
#include "ParticleSystem.h"
#include "GameObject.h"
#include "GameContext.h"

ParticleSystem* GameObjectFactory::create_particle_system(int x, int y, int particle_type, GameContext* context) {
    // Cast int back to ParticleType enum
    ParticleType type = static_cast<ParticleType>(particle_type);
    
    auto& pool = get_pool<ParticleSystem>();
    auto particle_system = pool.acquire();
    
    if (particle_system) {
        // Reuse existing object by reinitializing it
        particle_system->reinitialize(x, y, type, context);
    } else {
        // Create new object if pool is empty
        particle_system = std::make_unique<ParticleSystem>(x, y, type, context);
    }
    
    auto* raw_ptr = particle_system.release(); // Transfer ownership to GameContext
    context->register_object(raw_ptr);
    return raw_ptr;
}

bool GameObjectFactory::try_return_to_pool(GameObject* obj) {
    if (!obj || !obj->supports_object_pooling()) {
        return false; // Not poolable, should be deleted normally
    }
    
    // Type-specific pool return (only ParticleSystem supported for now)
    if (auto* particle_system = dynamic_cast<ParticleSystem*>(obj)) {
        auto unique_obj = std::unique_ptr<ParticleSystem>(particle_system);
        return_to_pool(std::move(unique_obj));
        return true;
    }
    
    return false; // Unknown poolable type, should be deleted normally
}

// Implementation of get_memory_statistics()
GameObjectFactory::MemoryStats GameObjectFactory::get_memory_statistics() const {
    MemoryStats stats;
    stats.total_pools = pools.size();
    stats.total_pooled_objects = 0;
    
    // Count objects in each pool
    for (const auto& [type_idx, pool_ptr] : pools) {
        // Note: This is a simplified implementation
        // In a real scenario, you'd need type-specific counting
        stats.total_pooled_objects += 1; // Placeholder
    }
    
    return stats;
}