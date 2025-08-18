#pragma once

#include <cmath>
#include <string>
#include <vector>

/**
 * @brief Sistema de coordenadas unificado para ClanBomber
 * 
 * El juego maneja dos sistemas de coordenadas:
 * - Grid Coordinates: Sistema lógico de tiles (ej: tile 4,1) 
 * - Pixel Coordinates: Sistema de rendering (ej: pixel 160,40)
 * 
 * Esta clase centraliza todas las conversiones y elimina errores de inconsistencia.
 */

/**
 * @brief Configuración del sistema de coordenadas
 */
struct CoordinateConfig {
    static constexpr int TILE_SIZE = 40;           // Tamaño de cada tile en pixels
    static constexpr int MAP_OFFSET_X = 0;         // Offset del mapa en X
    static constexpr int MAP_OFFSET_Y = 0;         // Offset del mapa en Y
    static constexpr int MAX_GRID_WIDTH = 20;      // Máximo ancho del mapa en tiles
    static constexpr int MAX_GRID_HEIGHT = 15;     // Máximo alto del mapa en tiles
};

/**
 * @brief Coordenada en el sistema de grid (lógico)
 */
struct GridCoord {
    int grid_x;
    int grid_y;
    
    GridCoord() : grid_x(0), grid_y(0) {}
    GridCoord(int gx, int gy) : grid_x(gx), grid_y(gy) {}
    
    bool operator==(const GridCoord& other) const {
        return grid_x == other.grid_x && grid_y == other.grid_y;
    }
    
    bool operator!=(const GridCoord& other) const {
        return !(*this == other);
    }
    
    std::string to_string() const {
        return "Grid(" + std::to_string(grid_x) + "," + std::to_string(grid_y) + ")";
    }
    
    /**
     * @brief Verifica si la coordenada está dentro de los límites del mapa
     */
    bool is_valid() const {
        return grid_x >= 0 && grid_x < CoordinateConfig::MAX_GRID_WIDTH &&
               grid_y >= 0 && grid_y < CoordinateConfig::MAX_GRID_HEIGHT;
    }
};

/**
 * @brief Coordenada en el sistema de pixels (rendering)
 */
struct PixelCoord {
    float pixel_x;
    float pixel_y;
    
    PixelCoord() : pixel_x(0.0f), pixel_y(0.0f) {}
    PixelCoord(float px, float py) : pixel_x(px), pixel_y(py) {}
    PixelCoord(int px, int py) : pixel_x(static_cast<float>(px)), pixel_y(static_cast<float>(py)) {}
    
    bool operator==(const PixelCoord& other) const {
        constexpr float epsilon = 0.01f;
        return std::abs(pixel_x - other.pixel_x) < epsilon && 
               std::abs(pixel_y - other.pixel_y) < epsilon;
    }
    
    bool operator!=(const PixelCoord& other) const {
        return !(*this == other);
    }
    
    std::string to_string() const {
        return "Pixel(" + std::to_string(pixel_x) + "," + std::to_string(pixel_y) + ")";
    }
    
    /**
     * @brief Calcula la distancia euclidiana a otra coordenada pixel
     */
    float distance_to(const PixelCoord& other) const {
        float dx = pixel_x - other.pixel_x;
        float dy = pixel_y - other.pixel_y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    /**
     * @brief Calcula la distancia Manhattan a otra coordenada pixel
     */
    float manhattan_distance_to(const PixelCoord& other) const {
        return std::abs(pixel_x - other.pixel_x) + std::abs(pixel_y - other.pixel_y);
    }
};

/**
 * @brief Utilidades para conversión entre sistemas de coordenadas
 */
class CoordinateSystem {
public:
    // === CONVERSIONES PRINCIPALES ===
    
    /**
     * @brief Convierte coordenadas de grid a pixel
     * @param grid Coordenada de grid
     * @return Coordenada pixel (centro del tile)
     */
    static PixelCoord grid_to_pixel(const GridCoord& grid) {
        float pixel_x = static_cast<float>(grid.grid_x * CoordinateConfig::TILE_SIZE + 
                                          CoordinateConfig::TILE_SIZE / 2 + CoordinateConfig::MAP_OFFSET_X);
        float pixel_y = static_cast<float>(grid.grid_y * CoordinateConfig::TILE_SIZE + 
                                          CoordinateConfig::TILE_SIZE / 2 + CoordinateConfig::MAP_OFFSET_Y);
        return PixelCoord(pixel_x, pixel_y);
    }
    
    /**
     * @brief Convierte coordenadas de pixel a grid
     * @param pixel Coordenada pixel
     * @return Coordenada de grid (tile que contiene el pixel)
     */
    static GridCoord pixel_to_grid(const PixelCoord& pixel) {
        int grid_x = static_cast<int>((pixel.pixel_x - CoordinateConfig::MAP_OFFSET_X) / CoordinateConfig::TILE_SIZE);
        int grid_y = static_cast<int>((pixel.pixel_y - CoordinateConfig::MAP_OFFSET_Y) / CoordinateConfig::TILE_SIZE);
        return GridCoord(grid_x, grid_y);
    }
    
    /**
     * @brief Convierte coordenada de grid a esquina superior-izquierda del tile en pixels
     * @param grid Coordenada de grid
     * @return Coordenada pixel de la esquina del tile
     */
    static PixelCoord grid_to_pixel_corner(const GridCoord& grid) {
        float pixel_x = static_cast<float>(grid.grid_x * CoordinateConfig::TILE_SIZE + CoordinateConfig::MAP_OFFSET_X);
        float pixel_y = static_cast<float>(grid.grid_y * CoordinateConfig::TILE_SIZE + CoordinateConfig::MAP_OFFSET_Y);
        return PixelCoord(pixel_x, pixel_y);
    }
    
    // === UTILIDADES DE VALIDACIÓN ===
    
    /**
     * @brief Verifica si una coordenada grid está dentro del mapa
     */
    static bool is_grid_valid(const GridCoord& grid) {
        return grid.is_valid();
    }
    
    /**
     * @brief Verifica si una coordenada pixel está dentro del área del mapa
     */
    static bool is_pixel_in_map_bounds(const PixelCoord& pixel) {
        GridCoord grid = pixel_to_grid(pixel);
        return is_grid_valid(grid);
    }
    
    /**
     * @brief Fuerza una coordenada grid a estar dentro de límites válidos
     */
    static GridCoord clamp_grid(const GridCoord& grid) {
        int clamped_x = std::max(0, std::min(grid.grid_x, CoordinateConfig::MAX_GRID_WIDTH - 1));
        int clamped_y = std::max(0, std::min(grid.grid_y, CoordinateConfig::MAX_GRID_HEIGHT - 1));
        return GridCoord(clamped_x, clamped_y);
    }
    
    // === UTILIDADES DE DISTANCIA ===
    
    /**
     * @brief Calcula distancia euclidiana entre dos coordenadas grid (en tiles)
     */
    static float grid_distance(const GridCoord& a, const GridCoord& b) {
        float dx = static_cast<float>(a.grid_x - b.grid_x);
        float dy = static_cast<float>(a.grid_y - b.grid_y);
        return std::sqrt(dx * dx + dy * dy);
    }
    
    /**
     * @brief Calcula distancia Manhattan entre dos coordenadas grid (en tiles)
     */
    static int grid_manhattan_distance(const GridCoord& a, const GridCoord& b) {
        return std::abs(a.grid_x - b.grid_x) + std::abs(a.grid_y - b.grid_y);
    }
    
    /**
     * @brief Verifica si dos coordenadas grid son adyacentes (distancia Manhattan = 1)
     */
    static bool are_grid_adjacent(const GridCoord& a, const GridCoord& b) {
        return grid_manhattan_distance(a, b) == 1;
    }
    
    // === UTILIDADES DE ÁREA ===
    
    /**
     * @brief Obtiene todas las coordenadas grid en un radio Manhattan dado
     * @param center Centro del área
     * @param radius Radio en tiles
     * @return Vector de coordenadas dentro del radio
     */
    static std::vector<GridCoord> get_grid_area_manhattan(const GridCoord& center, int radius) {
        std::vector<GridCoord> result;
        
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (std::abs(dx) + std::abs(dy) <= radius) {
                    GridCoord coord(center.grid_x + dx, center.grid_y + dy);
                    if (is_grid_valid(coord)) {
                        result.push_back(coord);
                    }
                }
            }
        }
        
        return result;
    }
    
    /**
     * @brief Obtiene todas las coordenadas grid en un radio euclidiano dado
     */
    static std::vector<GridCoord> get_grid_area_circular(const GridCoord& center, float radius) {
        std::vector<GridCoord> result;
        
        int int_radius = static_cast<int>(std::ceil(radius));
        
        for (int dy = -int_radius; dy <= int_radius; dy++) {
            for (int dx = -int_radius; dx <= int_radius; dx++) {
                float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                if (distance <= radius) {
                    GridCoord coord(center.grid_x + dx, center.grid_y + dy);
                    if (is_grid_valid(coord)) {
                        result.push_back(coord);
                    }
                }
            }
        }
        
        return result;
    }
    
    // === CONVERSIONES DE UTILIDAD ===
    
    /**
     * @brief Convierte coordenadas int x,y legacy a PixelCoord
     */
    static PixelCoord legacy_to_pixel(int x, int y) {
        return PixelCoord(static_cast<float>(x), static_cast<float>(y));
    }
    
    /**
     * @brief Convierte PixelCoord a coordenadas int x,y legacy
     */
    static void pixel_to_legacy(const PixelCoord& pixel, int& x, int& y) {
        x = static_cast<int>(std::round(pixel.pixel_x));
        y = static_cast<int>(std::round(pixel.pixel_y));
    }
    
    // === DEBUG Y LOGGING ===
    
    /**
     * @brief Genera string descriptivo de conversión para debugging
     */
    static std::string debug_conversion(const GridCoord& grid) {
        PixelCoord pixel = grid_to_pixel(grid);
        PixelCoord corner = grid_to_pixel_corner(grid);
        
        return grid.to_string() + " -> " + pixel.to_string() + 
               " (corner: " + corner.to_string() + ")";
    }
};