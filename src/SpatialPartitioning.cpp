#include "SpatialPartitioning.h"
#include "GameObject.h"
#include "Bomber.h"
#include "CoordinateSystem.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <set>

// Import CoordinateConfig constants for refactoring Phase 1
static constexpr int TILE_SIZE = CoordinateConfig::TILE_SIZE;

// === SpatialGrid Implementation ===

SpatialGrid::SpatialGrid(int cell_size_pixels) 
    : cell_size(cell_size_pixels) {
    SDL_Log("SpatialGrid: Initialized with cell_size=%d pixels", cell_size);
}

void SpatialGrid::clear() {
    cells.clear();
    object_positions.clear();
    SDL_Log("SpatialGrid: Cleared all cells and object positions");
}

void SpatialGrid::add_object(GameObject* obj) {
    if (!obj) return;
    
    PixelCoord position(static_cast<float>(obj->get_x()), static_cast<float>(obj->get_y()));
    GridCoord grid_coord = pixel_to_grid_coord(position);
    
    add_object_to_cell(obj, grid_coord);
    object_positions[obj] = grid_coord;
}

void SpatialGrid::remove_object(GameObject* obj) {
    if (!obj) return;
    
    auto it = object_positions.find(obj);
    if (it != object_positions.end()) {
        remove_object_from_cell(obj, it->second);
        object_positions.erase(it);
    }
}

void SpatialGrid::update_object_position(GameObject* obj, const PixelCoord& old_position) {
    if (!obj) return;
    
    PixelCoord new_position(static_cast<float>(obj->get_x()), static_cast<float>(obj->get_y()));
    GridCoord old_grid = pixel_to_grid_coord(old_position);
    GridCoord new_grid = pixel_to_grid_coord(new_position);
    
    // Only update if the object moved to a different cell
    if (old_grid.grid_x != new_grid.grid_x || old_grid.grid_y != new_grid.grid_y) {
        remove_object_from_cell(obj, old_grid);
        add_object_to_cell(obj, new_grid);
        object_positions[obj] = new_grid;
    }
}

void SpatialGrid::rebuild_from_objects(const std::list<GameObject*>& objects) {
    clear();
    
    for (GameObject* obj : objects) {
        if (obj && !obj->delete_me) {
            add_object(obj);
        }
    }
    
    SDL_Log("SpatialGrid: Rebuilt with %zu objects", objects.size());
}

std::vector<GameObject*> SpatialGrid::get_objects_at_position(const PixelCoord& position) const {
    GridCoord grid_coord = pixel_to_grid_coord(position);
    const SpatialCell* cell = get_cell(grid_coord);
    
    if (cell) {
        return cell->objects;
    }
    
    return std::vector<GameObject*>();
}

std::vector<GameObject*> SpatialGrid::get_objects_of_type_near(const PixelCoord& position,
                                                             GameObject::ObjectType object_type,
                                                             int radius) const {
    std::vector<GameObject*> result;
    GridCoord center = pixel_to_grid_coord(position);
    std::vector<GridCoord> cells_to_check = get_cells_in_radius(center, radius);
    
    for (const GridCoord& cell_coord : cells_to_check) {
        const SpatialCell* cell = get_cell(cell_coord);
        if (!cell) continue;
        
        for (GameObject* obj : cell->objects) {
            if (!obj || obj->delete_me) continue;
            
            if (object_type == GameObject::ObjectType::MAPTILE || obj->get_type() == object_type) {
                result.push_back(obj);
            }
        }
    }
    
    return result;
}

std::vector<GameObject*> SpatialGrid::get_bombers_near(const PixelCoord& position, int radius) const {
    return get_objects_of_type_near(position, GameObject::BOMBER, radius);
}

std::vector<GameObject*> SpatialGrid::get_bombs_near(const PixelCoord& position, int radius) const {
    return get_objects_of_type_near(position, GameObject::BOMB, radius);
}

std::vector<GameObject*> SpatialGrid::get_extras_near(const PixelCoord& position, int radius) const {
    return get_objects_of_type_near(position, GameObject::EXTRA, radius);
}

std::vector<GameObject*> SpatialGrid::get_objects_in_area(const PixelCoord& top_left,
                                                        const PixelCoord& bottom_right,
                                                        GameObject::ObjectType object_type) const {
    std::vector<GameObject*> result;
    std::vector<GridCoord> cells_in_area = get_cells_in_area(top_left, bottom_right);
    
    for (const GridCoord& cell_coord : cells_in_area) {
        const SpatialCell* cell = get_cell(cell_coord);
        if (!cell) continue;
        
        for (GameObject* obj : cell->objects) {
            if (!obj || obj->delete_me) continue;
            
            if (object_type == GameObject::ObjectType::MAPTILE || obj->get_type() == object_type) {
                // Verify object is actually within the area bounds
                float obj_x = static_cast<float>(obj->get_x());
                float obj_y = static_cast<float>(obj->get_y());
                
                if (obj_x >= top_left.pixel_x && obj_x <= bottom_right.pixel_x &&
                    obj_y >= top_left.pixel_y && obj_y <= bottom_right.pixel_y) {
                    result.push_back(obj);
                }
            }
        }
    }
    
    return result;
}

std::vector<GameObject*> SpatialGrid::find_collisions(GameObject* obj, 
                                                    float collision_radius,
                                                    GameObject::ObjectType object_type) const {
    if (!obj) return std::vector<GameObject*>();
    
    std::vector<GameObject*> result;
    PixelCoord obj_position(static_cast<float>(obj->get_x()), static_cast<float>(obj->get_y()));
    
    // Calculate radius in grid cells
    int grid_radius = static_cast<int>(std::ceil(collision_radius / cell_size));
    
    std::vector<GameObject*> nearby_objects = get_objects_of_type_near(obj_position, object_type, grid_radius);
    
    for (GameObject* other : nearby_objects) {
        if (!other || other == obj || other->delete_me) continue;
        
        // Calculate actual distance
        float dx = static_cast<float>(obj->get_x() - other->get_x());
        float dy = static_cast<float>(obj->get_y() - other->get_y());
        float distance = std::sqrt(dx * dx + dy * dy);
        
        if (distance <= collision_radius) {
            result.push_back(other);
        }
    }
    
    return result;
}

bool SpatialGrid::has_object_at_position(const PixelCoord& position, 
                                       GameObject::ObjectType object_type) const {
    std::vector<GameObject*> objects = get_objects_at_position(position);
    
    for (GameObject* obj : objects) {
        if (!obj || obj->delete_me) continue;
        
        if (object_type == GameObject::ObjectType::MAPTILE || obj->get_type() == object_type) {
            return true;
        }
    }
    
    return false;
}

SpatialGrid::GridStats SpatialGrid::get_statistics() const {
    GridStats stats;
    stats.total_cells = cells.size();
    stats.total_objects = object_positions.size();
    
    size_t occupied_cells = 0;
    size_t max_objects = 0;
    
    for (const auto& pair : cells) {
        const SpatialCell& cell = pair.second;
        if (cell.object_count() > 0) {
            occupied_cells++;
            max_objects = std::max(max_objects, cell.object_count());
        }
    }
    
    stats.occupied_cells = occupied_cells;
    stats.max_objects_in_cell = max_objects;
    
    if (occupied_cells > 0) {
        stats.average_objects_per_cell = static_cast<float>(stats.total_objects) / occupied_cells;
    }
    
    if (stats.total_cells > 0) {
        stats.load_factor = static_cast<float>(occupied_cells) / stats.total_cells;
    }
    
    return stats;
}

void SpatialGrid::print_debug_info() const {
    GridStats stats = get_statistics();
    
    SDL_Log("=== SpatialGrid Debug Info ===");
    SDL_Log("Cell size: %d pixels", cell_size);
    SDL_Log("Total cells: %zu", stats.total_cells);
    SDL_Log("Occupied cells: %zu", stats.occupied_cells);
    SDL_Log("Total objects: %zu", stats.total_objects);
    SDL_Log("Load factor: %.2f", stats.load_factor);
    SDL_Log("Average objects per cell: %.2f", stats.average_objects_per_cell);
    SDL_Log("Max objects in single cell: %zu", stats.max_objects_in_cell);
}

std::string SpatialGrid::visualize_grid(int max_width, int max_height) const {
    std::ostringstream output;
    
    output << "=== SpatialGrid Visualization ===\n";
    output << "Legend: . = empty, # = 1-5 objects, @ = 6+ objects\n\n";
    
    // Find bounds of occupied cells
    int min_x = 0, max_x = 0, min_y = 0, max_y = 0;
    bool first = true;
    
    for (const auto& pair : cells) {
        const GridCoord& coord = pair.first;
        if (first) {
            min_x = max_x = coord.grid_x;
            min_y = max_y = coord.grid_y;
            first = false;
        } else {
            min_x = std::min(min_x, coord.grid_x);
            max_x = std::max(max_x, coord.grid_x);
            min_y = std::min(min_y, coord.grid_y);
            max_y = std::max(max_y, coord.grid_y);
        }
    }
    
    // Limit visualization size
    int width = std::min(max_width, max_x - min_x + 1);
    int height = std::min(max_height, max_y - min_y + 1);
    
    for (int y = min_y; y < min_y + height; y++) {
        for (int x = min_x; x < min_x + width; x++) {
            GridCoord coord(x, y);
            const SpatialCell* cell = get_cell(coord);
            
            if (!cell || cell->object_count() == 0) {
                output << '.';
            } else if (cell->object_count() <= 5) {
                output << '#';
            } else {
                output << '@';
            }
        }
        output << '\n';
    }
    
    return output.str();
}

// === Private Helper Methods ===

GridCoord SpatialGrid::pixel_to_grid_coord(const PixelCoord& position) const {
    int grid_x = static_cast<int>(std::floor(position.pixel_x / cell_size));
    int grid_y = static_cast<int>(std::floor(position.pixel_y / cell_size));
    return GridCoord(grid_x, grid_y);
}

std::vector<GridCoord> SpatialGrid::get_cells_in_radius(const GridCoord& center, int radius) const {
    std::vector<GridCoord> result;
    
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            GridCoord coord(center.grid_x + dx, center.grid_y + dy);
            result.push_back(coord);
        }
    }
    
    return result;
}

std::vector<GridCoord> SpatialGrid::get_cells_in_area(const PixelCoord& top_left, 
                                                    const PixelCoord& bottom_right) const {
    std::vector<GridCoord> result;
    
    GridCoord top_left_grid = pixel_to_grid_coord(top_left);
    GridCoord bottom_right_grid = pixel_to_grid_coord(bottom_right);
    
    for (int y = top_left_grid.grid_y; y <= bottom_right_grid.grid_y; y++) {
        for (int x = top_left_grid.grid_x; x <= bottom_right_grid.grid_x; x++) {
            result.push_back(GridCoord(x, y));
        }
    }
    
    return result;
}

SpatialCell& SpatialGrid::get_or_create_cell(const GridCoord& coord) {
    return cells[coord];
}

const SpatialCell* SpatialGrid::get_cell(const GridCoord& coord) const {
    auto it = cells.find(coord);
    if (it != cells.end()) {
        return &it->second;
    }
    return nullptr;
}

void SpatialGrid::add_object_to_cell(GameObject* obj, const GridCoord& coord) {
    SpatialCell& cell = get_or_create_cell(coord);
    cell.add_object(obj);
}

void SpatialGrid::remove_object_from_cell(GameObject* obj, const GridCoord& coord) {
    auto it = cells.find(coord);
    if (it != cells.end()) {
        it->second.remove_object(obj);
        
        // Remove empty cells to save memory
        if (it->second.object_count() == 0) {
            cells.erase(it);
        }
    }
}

// === CollisionHelper Implementation ===

GameObject* CollisionHelper::find_nearest_bomber(const PixelCoord& extra_position, float max_distance) {
    if (!spatial_grid) return nullptr;
    
    // Search in expanding radius until we find a bomber or reach max distance
    int max_radius = static_cast<int>(std::ceil(max_distance / static_cast<float>(TILE_SIZE)));
    
    for (int radius = 1; radius <= max_radius; radius++) {
        std::vector<GameObject*> bombers = spatial_grid->get_bombers_near(extra_position, radius);
        
        GameObject* nearest = nullptr;
        float nearest_distance = max_distance + 1.0f;
        
        for (GameObject* bomber : bombers) {
            // CRASH FIX: More robust null checking and corruption detection
            if (!bomber) {
                SDL_Log("CollisionHelper: WARNING - null bomber pointer in SpatialGrid");
                continue;
            }
            
            // Check for obviously corrupted pointers (basic heuristic)
            if (reinterpret_cast<uintptr_t>(bomber) < 0x1000) {
                SDL_Log("CollisionHelper: WARNING - corrupted bomber pointer: %p", bomber);
                continue;
            }
            
            if (bomber->delete_me) {
                SDL_Log("CollisionHelper: WARNING - delete_me bomber still in SpatialGrid: %p", bomber);
                continue;
            }
            
            try {
                float dx = extra_position.pixel_x - static_cast<float>(bomber->get_x());
                float dy = extra_position.pixel_y - static_cast<float>(bomber->get_y());
                float distance = std::sqrt(dx * dx + dy * dy);
                
                if (distance <= max_distance && distance < nearest_distance) {
                    nearest = bomber;
                    nearest_distance = distance;
                }
            } catch (...) {
                SDL_Log("CollisionHelper: CRASH PREVENTED - Exception accessing bomber %p", bomber);
                continue;
            }
        }
        
        if (nearest) {
            return nearest;
        }
    }
    
    return nullptr;
}

std::vector<GameObject*> CollisionHelper::find_explosion_victims(const std::vector<GridCoord>& explosion_area) {
    if (!spatial_grid) {
        SDL_Log("CollisionHelper: WARNING - No spatial_grid available for explosion victims");
        return std::vector<GameObject*>();
    }
    
    std::vector<GameObject*> victims;
    std::set<GameObject*> found_objects; // Use set to avoid duplicates
    
    // FAIRNESS FIX: Instead of discrete tile checking, use area-based detection
    // This prevents bombers from dying when they're visually outside the explosion
    
    const float TILE_SIZE_FLOAT = static_cast<float>(TILE_SIZE);
    const float BOMBER_SIZE = TILE_SIZE_FLOAT; // Slightly smaller than tile to avoid bleeding into adjacent tiles
    
    // Get all potentially affected objects in the explosion area + adjacent tiles
    for (const GridCoord& grid_coord : explosion_area) {
        // Define explosion tile area
        float explosion_left = grid_coord.grid_x * TILE_SIZE_FLOAT;
        float explosion_top = grid_coord.grid_y * TILE_SIZE_FLOAT;
        float explosion_right = explosion_left + TILE_SIZE_FLOAT;
        float explosion_bottom = explosion_top + TILE_SIZE_FLOAT;
        
        // SMART SEARCH: Search in explosion tile + adjacent tiles, but ONLY accept objects
        // whose bounding box actually intersects the explosion tile (not just nearby)
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                GridCoord search_coord(grid_coord.grid_x + dx, grid_coord.grid_y + dy);
                PixelCoord search_pixel = CoordinateSystem::grid_to_pixel(search_coord);
                
                std::vector<GameObject*> nearby_objects = spatial_grid->get_objects_at_position(search_pixel);
                
                for (GameObject* obj : nearby_objects) {
                    // Robust null checking
                    if (!obj || obj->delete_me || reinterpret_cast<uintptr_t>(obj) < 0x1000) {
                        continue;
                    }
                    
                    try {
                        GameObject::ObjectType obj_type = obj->get_type();
                        
                        // Only check bombers and corpses
                        if (obj_type == GameObject::BOMBER || obj_type == GameObject::BOMBER_CORPSE) {
                            // DISCRETE TILE LOGIC: Like classic Bomberman - bomber dies if in explosion tile
                            float bomber_x = static_cast<float>(obj->get_x());
                            float bomber_y = static_cast<float>(obj->get_y());
                            
                            // Calculate bomber's current tile
                            int bomber_tile_x = static_cast<int>(bomber_x / TILE_SIZE_FLOAT);
                            int bomber_tile_y = static_cast<int>(bomber_y / TILE_SIZE_FLOAT);
                            
                            // Check if bomber's tile matches this explosion tile
                            bool in_explosion_tile = (bomber_tile_x == grid_coord.grid_x && 
                                                    bomber_tile_y == grid_coord.grid_y);
                            
                            SDL_Log("🎯 DISCRETE: Bomber(%.1f,%.1f) in tile(%d,%d) vs ExplosionTile(%d,%d)", 
                                    bomber_x, bomber_y, bomber_tile_x, bomber_tile_y, 
                                    grid_coord.grid_x, grid_coord.grid_y);
                            SDL_Log("   SAME_TILE=%s", in_explosion_tile ? "YES" : "NO");
                            
                            if (in_explosion_tile) {
                                SDL_Log("💥 DEATH: Bomber in tile(%d,%d) killed by explosion in same tile", 
                                        bomber_tile_x, bomber_tile_y);
                                found_objects.insert(obj);
                            } else {
                                SDL_Log("✅ SAFE: Bomber in tile(%d,%d) safe from explosion in tile(%d,%d)", 
                                        bomber_tile_x, bomber_tile_y, grid_coord.grid_x, grid_coord.grid_y);
                            }
                        }
                    } catch (...) {
                        SDL_Log("CollisionHelper: CRASH PREVENTED - Exception accessing object %p", obj);
                        continue;
                    }
                }
            }
        }
    }
    
    // Convert set to vector
    victims.assign(found_objects.begin(), found_objects.end());
    
    return victims;
}

CollisionHelper::AITargets CollisionHelper::scan_ai_targets(const PixelCoord& bomber_position, int scan_radius) {
    AITargets targets;
    
    if (!spatial_grid) return targets;
    
    // Get all objects in scan radius
    std::vector<GameObject*> all_objects = spatial_grid->get_objects_of_type_near(
        bomber_position, GameObject::ObjectType::MAPTILE, scan_radius);
    
    for (GameObject* obj : all_objects) {
        if (!obj || obj->delete_me) continue;
        
        switch (obj->get_type()) {
            case GameObject::BOMBER:
                targets.enemy_bombers.push_back(obj);
                break;
            case GameObject::BOMB:
                targets.bombs.push_back(obj);
                break;
            case GameObject::EXTRA:
                targets.extras.push_back(obj);
                break;
            default:
                // Check for destructible tiles or other targets
                break;
        }
    }
    
    return targets;
}