#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <string>

// Forward declarations
class ParticleSystem;
class GameContext;
class GameObject;

/**
 * @brief Estrategia consistente de memory management para ClanBomber
 * 
 * PROBLEMAS IDENTIFICADOS:
 * - Mezcla de raw pointers y smart pointers 
 * - Ownership unclear (quién es responsable de delete?)
 * - Leaks potenciales en exception paths
 * - Double delete risks
 * 
 * SOLUCIÓN:
 * - RAII everywhere con smart pointers
 * - Clear ownership semantics
 * - Automatic cleanup con destructors
 * - Factory pattern para object creation
 */

namespace MemoryPolicy {
    
    /**
     * @brief Políticas de ownership para diferentes tipos de objetos
     */
    enum class OwnershipType {
        UNIQUE,     // std::unique_ptr - ownership exclusivo
        SHARED,     // std::shared_ptr - ownership compartido
        WEAK,       // std::weak_ptr - referencia sin ownership
        BORROWED    // raw pointer - NO ownership (lifetime gestionado externamente)
    };
    
    /**
     * @brief Estrategias de cleanup para diferentes categorías
     */
    enum class CleanupStrategy {
        IMMEDIATE,  // Delete inmediatamente cuando sale de scope
        DEFERRED,   // Mark for deletion, cleanup en próximo frame
        POOLED,     // Return to object pool para reuso
        MANUAL      // Cleanup manual requerido
    };
}

/**
 * @brief Smart pointer aliases para consistencia
 */
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// Factory function para crear unique_ptr con custom deleter
template<typename T, typename... Args>
UniquePtr<T> make_unique_with_deleter(std::function<void(T*)> deleter, Args&&... args) {
    return std::unique_ptr<T, std::function<void(T*)>>(
        new T(std::forward<Args>(args)...), 
        deleter
    );
}

/**
 * @brief Memory pool para objetos que se crean/destruyen frecuentemente
 * Útil para GameObjects, Particles, etc.
 */
template<typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t initial_size = 50) : max_size(200) {
        pool.reserve(initial_size);
        // Pool starts empty - objects added when released back to pool
    }
    
    /**
     * @brief Obtiene objeto del pool, returns nullptr if pool is empty
     */
    UniquePtr<T> acquire() {
        if (!pool.empty()) {
            auto obj = std::move(pool.back());
            pool.pop_back();
            return obj;
        }
        return nullptr; // Caller must handle nullptr case
    }
    
    /**
     * @brief Regresa objeto al pool para reuso
     */
    void release(UniquePtr<T> obj) {
        if (obj && pool.size() < max_size) {
            // Reset object state before returning to pool if reset method exists
            if constexpr (std::is_base_of_v<GameObject, T>) {
                obj->reset_for_pool();
            }
            pool.push_back(std::move(obj));
        }
    }
    
    size_t size() const { return pool.size(); }
    void set_max_size(size_t size) { max_size = size; }

private:
    std::vector<UniquePtr<T>> pool;
    size_t max_size = 200;
};

/**
 * @brief Factory central para creación de objetos con memory management correcto
 */
class GameObjectFactory {
public:
    static GameObjectFactory& getInstance() {
        static GameObjectFactory instance;
        return instance;
    }
    
    /**
     * @brief Crea GameObject con ownership apropiado
     */
    template<typename T, typename... Args>
    UniquePtr<T> create_unique(Args&&... args) {
        static_assert(std::is_base_of_v<GameObject, T>, "T must derive from GameObject");
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    
    template<typename T, typename... Args>
    SharedPtr<T> create_shared(Args&&... args) {
        static_assert(std::is_base_of_v<GameObject, T>, "T must derive from GameObject");
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
    
    /**
     * @brief Regresa objeto al pool para reuso
     */
    template<typename T>
    void return_to_pool(UniquePtr<T> obj) {
        auto& pool = get_pool<T>();
        pool.release(std::move(obj));
    }
    
    /**
     * @brief Specialized factory for ParticleSystem with ObjectPool and auto-registration
     * Declared here, defined in GameObjectFactory.cpp to avoid forward declaration issues
     */
    class ParticleSystem* create_particle_system(int x, int y, int particle_type, GameContext* context);
    
    /**
     * @brief Attempts to return a GameObject to appropriate pool
     * Declared here, defined in GameObjectFactory.cpp to avoid forward declaration issues
     */
    bool try_return_to_pool(GameObject* obj);
    
    /**
     * @brief Obtiene estadísticas de memory usage
     */
    struct MemoryStats {
        size_t total_pools = 0;
        size_t total_pooled_objects = 0;
        std::unordered_map<std::string, size_t> objects_per_type;
    };
    
    MemoryStats get_memory_statistics() const;

private:
    // Pool storage por tipo usando type erasure correctamente
    std::unordered_map<std::type_index, std::unique_ptr<void, std::function<void(void*)>>> pools;
    
    template<typename T>
    ObjectPool<T>& get_pool() {
        auto type_idx = std::type_index(typeid(T));
        auto it = pools.find(type_idx);
        
        if (it == pools.end()) {
            auto deleter = [](void* ptr) { delete static_cast<ObjectPool<T>*>(ptr); };
            pools[type_idx] = std::unique_ptr<void, std::function<void(void*)>>(
                new ObjectPool<T>(), deleter
            );
        }
        
        return *static_cast<ObjectPool<T>*>(pools[type_idx].get());
    }
};

/**
 * @brief RAII wrapper para recursos SDL
 */
template<typename T>
class SDLResource {
public:
    explicit SDLResource(T* resource, std::function<void(T*)> deleter)
        : resource_(resource), deleter_(deleter) {}
        
    ~SDLResource() {
        if (resource_ && deleter_) {
            deleter_(resource_);
        }
    }
    
    // Move-only semantics
    SDLResource(const SDLResource&) = delete;
    SDLResource& operator=(const SDLResource&) = delete;
    
    SDLResource(SDLResource&& other) noexcept
        : resource_(other.resource_), deleter_(std::move(other.deleter_)) {
        other.resource_ = nullptr;
    }
    
    SDLResource& operator=(SDLResource&& other) noexcept {
        if (this != &other) {
            if (resource_ && deleter_) {
                deleter_(resource_);
            }
            resource_ = other.resource_;
            deleter_ = std::move(other.deleter_);
            other.resource_ = nullptr;
        }
        return *this;
    }
    
    T* get() const { return resource_; }
    T* release() {
        T* temp = resource_;
        resource_ = nullptr;
        return temp;
    }
    
    explicit operator bool() const { return resource_ != nullptr; }

private:
    T* resource_;
    std::function<void(T*)> deleter_;
};

/**
 * @brief Helper macros para crear SDL resources con RAII
 */
#define MAKE_SDL_SURFACE(surface) \
    SDLResource<SDL_Surface>(surface, [](SDL_Surface* s) { if(s) SDL_DestroySurface(s); })

#define MAKE_SDL_TEXTURE(texture) \
    SDLResource<SDL_Texture>(texture, [](SDL_Texture* t) { if(t) SDL_DestroyTexture(t); })

/**
 * @brief Smart pointer para GameObject con automatic GameContext registration
 */
template<typename T>
class ManagedGameObject {
    static_assert(std::is_base_of_v<GameObject, T>, "T must derive from GameObject");
    
public:
    template<typename... Args>
    ManagedGameObject(GameContext* context, Args&&... args) 
        : context_(context), object_(std::make_unique<T>(std::forward<Args>(args)...)) {
        if (context_) {
            context_->register_object(object_.get());
        }
    }
    
    ~ManagedGameObject() {
        // Object will be automatically removed from GameContext during cleanup
    }
    
    T* get() const { return object_.get(); }
    T& operator*() const { return *object_; }
    T* operator->() const { return object_.get(); }
    
    UniquePtr<T> release() {
        return std::move(object_);
    }

private:
    GameContext* context_;
    UniquePtr<T> object_;
};

/**
 * @brief Guidelines y best practices para memory management
 */
namespace MemoryGuidelines {
    
    // REGLAS DE OWNERSHIP:
    
    // 1. GameObjects: Usar ManagedGameObject o manual GameContext registration
    //    - GameContext es el owner de todos los GameObjects
    //    - Individual objects nunca deben hacer delete de otros GameObjects
    
    // 2. SDL Resources: Siempre usar SDLResource RAII wrappers
    //    - Nunca raw SDL pointers sin wrapper
    //    - Automatic cleanup en destructors
    
    // 3. Short-lived objects: std::unique_ptr
    //    - Clear ownership semantics
    //    - Automatic cleanup
    
    // 4. Shared resources: std::shared_ptr solo cuando realmente necesario
    //    - Textures compartidas entre objects
    //    - Configuration objects
    
    // 5. References sin ownership: raw pointers OK
    //    - Parámetros de función
    //    - Parent/child relationships (parent owns child)
    //    - NUNCA almacenar raw pointers que requieren delete
    
    inline std::string get_ownership_guideline(const std::string& object_type) {
        if (object_type == "GameObject") {
            return "Use ManagedGameObject or manual GameContext registration";
        } else if (object_type == "SDL_Resource") {
            return "Use SDLResource RAII wrapper";
        } else if (object_type == "Short_lived") {
            return "Use std::unique_ptr for clear ownership";
        } else if (object_type == "Shared") {
            return "Use std::shared_ptr only when truly needed";
        } else if (object_type == "Reference") {
            return "Raw pointer OK for references without ownership";
        } else {
            return "Follow RAII principles with appropriate smart pointer";
        }
    }
}