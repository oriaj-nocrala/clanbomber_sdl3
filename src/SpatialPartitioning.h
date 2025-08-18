#pragma once

#include "CoordinateSystem.h"
#include "GameObject.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>

/**
 * @brief Sistema de spatial partitioning para optimizar detección de colisiones
 * 
 * PROBLEMA ACTUAL:
 * - O(n²) collision detection con búsqueda lineal en lista completa
 * - Extra.cpp itera sobre TODOS los objetos para buscar bombers
 * - Explosion.cpp itera sobre TODOS los objetos para buscar víctimas
 * - Controller_AI itera sobre TODOS los objetos para buscar targets
 * - Performance degrades con muchos objetos
 * 
 * SOLUCIÓN:
 * - Spatial hash grid para dividir el mundo en celdas
 * - Solo check colisiones en celdas adyacentes 
 * - Reduce complexity de O(n²) a O(n) en average case
 * - Perfecto para ClanBomber donde objetos se distribuyen en grid 40x40
 */

/**
 * @brief Hash para GridCoord
 */
struct GridCoordHash {
    size_t operator()(const GridCoord& coord) const {
        // Combine x and y into single hash usando bitwise operations
        return static_cast<size_t>(coord.grid_x) | (static_cast<size_t>(coord.grid_y) << 16);
    }
};

/**
 * @brief Una celda en el spatial grid
 */
struct SpatialCell {
    std::vector<GameObject*> objects;
    
    void add_object(GameObject* obj) {
        objects.push_back(obj);
    }
    
    void remove_object(GameObject* obj) {
        objects.erase(
            std::remove(objects.begin(), objects.end(), obj),
            objects.end());
    }
    
    void clear() {
        objects.clear();
    }
    
    size_t object_count() const {
        return objects.size();
    }
};

/**
 * @brief Spatial partitioning system usando uniform grid
 */
class SpatialGrid {
public:
    /**
     * @brief Construye spatial grid basado en tile size del juego
     * @param cell_size_pixels Tamaño de cada celda en pixels (default: tile size)
     */
    explicit SpatialGrid(int cell_size_pixels = CoordinateConfig::TILE_SIZE);
    
    /**
     * @brief Limpia todo el grid
     */
    void clear();
    
    /**
     * @brief Añade objeto al grid en su posición actual
     * @param obj GameObject a añadir
     */
    void add_object(GameObject* obj);
    
    /**
     * @brief Remueve objeto del grid
     * @param obj GameObject a remover  
     */
    void remove_object(GameObject* obj);
    
    /**
     * @brief Actualiza posición de objeto en el grid
     * @param obj GameObject que se movió
     * @param old_position Posición anterior
     */
    void update_object_position(GameObject* obj, const PixelCoord& old_position);
    
    /**
     * @brief Reconstruye todo el grid desde lista de objetos
     * @param objects Lista completa de objetos activos
     */
    void rebuild_from_objects(const std::list<GameObject*>& objects);
    
    // === QUERY OPERATIONS ===
    
    /**
     * @brief Obtiene todos los objetos en una posición específica
     * @param position Posición a consultar
     * @return Vector de objetos en esa posición
     */
    std::vector<GameObject*> get_objects_at_position(const PixelCoord& position) const;
    
    /**
     * @brief Obtiene todos los objetos de un tipo específico cerca de una posición
     * @param position Posición central
     * @param object_type Tipo de objeto buscado
     * @param radius Radio de búsqueda en celdas (default: 1 = celdas adyacentes)
     * @return Vector de objetos encontrados
     */
    std::vector<GameObject*> get_objects_of_type_near(const PixelCoord& position,
                                                     GameObject::ObjectType object_type,
                                                     int radius = 1) const;
    
    /**
     * @brief Obtiene todos los bombers cerca de una posición
     * @param position Posición central  
     * @param radius Radio de búsqueda en celdas
     * @return Vector de bombers encontrados
     */
    std::vector<GameObject*> get_bombers_near(const PixelCoord& position, int radius = 1) const;
    
    /**
     * @brief Obtiene todas las bombas cerca de una posición
     * @param position Posición central
     * @param radius Radio de búsqueda en celdas  
     * @return Vector de bombas encontradas
     */
    std::vector<GameObject*> get_bombs_near(const PixelCoord& position, int radius = 1) const;
    
    /**
     * @brief Obtiene todos los extras cerca de una posición
     * @param position Posición central
     * @param radius Radio de búsqueda en celdas
     * @return Vector de extras encontrados
     */
    std::vector<GameObject*> get_extras_near(const PixelCoord& position, int radius = 1) const;
    
    /**
     * @brief Busca objetos en área rectangular
     * @param top_left Esquina superior izquierda
     * @param bottom_right Esquina inferior derecha
     * @param object_type Tipo de objeto (opcional, ANY para todos)
     * @return Vector de objetos en el área
     */
    std::vector<GameObject*> get_objects_in_area(const PixelCoord& top_left,
                                                const PixelCoord& bottom_right,
                                                GameObject::ObjectType object_type = GameObject::ANY) const;
    
    // === COLLISION DETECTION HELPERS ===
    
    /**
     * @brief Encuentra objetos que colisionan con un objeto específico
     * @param obj Objeto central
     * @param collision_radius Radio de colisión en pixels
     * @param object_type Tipo de objetos a buscar (opcional)
     * @return Vector de objetos que colisionan
     */
    std::vector<GameObject*> find_collisions(GameObject* obj, 
                                            float collision_radius,
                                            GameObject::ObjectType object_type = GameObject::ANY) const;
    
    /**
     * @brief Verifica si hay algún objeto en posición específica
     * @param position Posición a verificar
     * @param object_type Tipo de objeto a buscar (opcional)
     * @return true si hay objetos del tipo especificado
     */
    bool has_object_at_position(const PixelCoord& position, 
                               GameObject::ObjectType object_type = GameObject::ANY) const;
    
    // === STATISTICS & DEBUGGING ===
    
    /**
     * @brief Estadísticas del spatial grid
     */
    struct GridStats {
        size_t total_cells = 0;
        size_t occupied_cells = 0;
        size_t total_objects = 0;
        float average_objects_per_cell = 0.0f;
        size_t max_objects_in_cell = 0;
        float load_factor = 0.0f; // occupied_cells / total_cells
    };
    
    GridStats get_statistics() const;
    
    /**
     * @brief Imprime información debug del grid
     */
    void print_debug_info() const;
    
    /**
     * @brief Visualiza el grid para debugging (ASCII art)
     */
    std::string visualize_grid(int max_width = 20, int max_height = 15) const;

private:
    // Hash map de celdas ocupadas (sparse representation)
    std::unordered_map<GridCoord, SpatialCell, GridCoordHash> cells;
    
    // Configuración
    int cell_size;  // Tamaño de celda en pixels
    
    // Cache para tracking de objetos
    std::unordered_map<GameObject*, GridCoord> object_positions;
    
    // Helper methods
    GridCoord pixel_to_grid_coord(const PixelCoord& position) const;
    std::vector<GridCoord> get_cells_in_radius(const GridCoord& center, int radius) const;
    std::vector<GridCoord> get_cells_in_area(const PixelCoord& top_left, 
                                            const PixelCoord& bottom_right) const;
    
    SpatialCell& get_or_create_cell(const GridCoord& coord);
    const SpatialCell* get_cell(const GridCoord& coord) const;
    
    void add_object_to_cell(GameObject* obj, const GridCoord& coord);
    void remove_object_from_cell(GameObject* obj, const GridCoord& coord);
};

/**
 * @brief Helper para optimizar búsquedas comunes en GameObjects
 */
class CollisionHelper {
public:
    explicit CollisionHelper(SpatialGrid* grid) : spatial_grid(grid) {}
    
    /**
     * @brief Optimized bomber collision detection para Extra.cpp
     * @param extra_position Posición del extra
     * @param max_distance Distancia máxima para colección
     * @return Bomber más cercano dentro del radio, o nullptr
     */
    GameObject* find_nearest_bomber(const PixelCoord& extra_position, float max_distance = 20.0f);
    
    /**
     * @brief Optimized explosion victim detection para Explosion.cpp  
     * @param explosion_area Lista de coordenadas grid afectadas por explosión
     * @return Vector de bombers y corpses en área de explosión
     */
    std::vector<GameObject*> find_explosion_victims(const std::vector<GridCoord>& explosion_area);
    
    /**
     * @brief Optimized AI target scanning para Controller_AI_*.cpp
     * @param bomber_position Posición del bomber AI
     * @param scan_radius Radio de escaneo en tiles
     * @return Estructura con objetivos categorizados
     */
    struct AITargets {
        std::vector<GameObject*> enemy_bombers;
        std::vector<GameObject*> bombs;
        std::vector<GameObject*> extras;
        std::vector<GameObject*> destructible_tiles;
    };
    AITargets scan_ai_targets(const PixelCoord& bomber_position, int scan_radius = 5);

private:
    SpatialGrid* spatial_grid;
};