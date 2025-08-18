#pragma once

#include "GameContext.h"
#include "Map.h"
#include "GameObject.h"
#include "Bomber.h"
#include <list>
#include <memory>

/**
 * @brief GameLogic facade - Centraliza la lógica del juego separada de ClanBomberApplication
 * 
 * Esta clase extrae las responsabilidades de lógica de juego que estaban mezcladas
 * en ClanBomberApplication, siguiendo el principio Single Responsibility.
 * 
 * Responsabilidades:
 * - Gestión del ciclo de vida de objetos de juego
 * - Lógica de update del juego (act_all, show_all)
 * - Manejo de pausa/resume
 * - Búsqueda y gestión de objetos por ID
 * - Cleanup de objetos marcados para eliminación
 */
class GameLogic {
public:
    explicit GameLogic(GameContext* context);
    ~GameLogic() = default;

    // === CORE GAME LOGIC ===
    
    /**
     * @brief Ejecuta un frame completo del juego
     * @param deltaTime Tiempo transcurrido desde el último frame
     */
    void update_frame(float deltaTime);
    
    /**
     * @brief Ejecuta la lógica de todos los objetos del juego
     * @param deltaTime Tiempo transcurrido desde el último frame
     */
    void update_all_objects(float deltaTime);
    
    /**
     * @brief Renderiza todos los objetos del juego
     */
    void render_all_objects();
    
    /**
     * @brief Elimina objetos marcados para eliminación (delete_me = true)
     */
    void cleanup_deleted_objects();

    // === OBJECT MANAGEMENT ===
    
    /**
     * @brief Busca un objeto por su ID único
     * @param object_id ID del objeto a buscar
     * @return GameObject* o nullptr si no se encuentra
     */
    GameObject* find_object_by_id(int object_id) const;
    
    /**
     * @brief Busca un bomber por su ID único
     * @param bomber_id ID del bomber a buscar  
     * @return Bomber* o nullptr si no se encuentra
     */
    Bomber* find_bomber_by_id(int bomber_id) const;
    
    /**
     * @brief Cuenta objetos activos (no marcados para eliminación)
     * @return Número de objetos activos
     */
    size_t count_active_objects() const;
    
    /**
     * @brief Elimina todos los objetos del juego
     */
    void clear_all_objects();

    // === GAME STATE MANAGEMENT ===
    
    /**
     * @brief Pausa/resume el juego
     * @param paused true para pausar, false para resume
     */
    void set_paused(bool paused) { is_paused = paused; }
    
    /**
     * @brief Verifica si el juego está pausado
     * @return true si está pausado
     */
    bool is_game_paused() const { return is_paused; }
    
    /**
     * @brief Reinicia el estado del juego para una nueva partida
     */
    void reset_game_state();

    // === STATISTICS ===
    
    /**
     * @brief Obtiene estadísticas del juego
     */
    struct GameStats {
        size_t total_objects = 0;
        size_t active_bombers = 0;
        size_t active_bombs = 0;
        size_t active_explosions = 0;
        size_t active_extras = 0;
    };
    
    GameStats get_game_statistics() const;

private:
    GameContext* game_context;
    bool is_paused;
    
    // Contador de frames para debugging
    uint64_t frame_counter;
    
    // Helper methods
    void log_frame_statistics() const;
    bool should_skip_object_update(GameObject* obj) const;
};