/* This file is part of ClanBomber <http://www.nongnu.org/clanbomber>.
 * Copyright (C) 1999-2004, 2007 Andreas Hundt, Denis Oliver Kropp
 * Copyright (C) 2008-2011, 2017 Rene Lopez <rsl@member.fsf.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ClanBomber.h"
#include "Map.h"
#include "GameObject.h"
#include "Bomber.h"
#include "TileManager.h"
#include "ParticleEffectsManager.h"
#include "GameContext.h"

ClanBomberApplication::ClanBomberApplication() {
    map = nullptr;
    bombers_received_by_client = false;
    pause_game = false;
    client_disconnected_from_server = false;
    client_connecting_to_new_server = false;
    // REMOVED: gpu_renderer = nullptr; - now handled by RenderingFacade
    lifecycle_manager = new LifecycleManager();
    tile_manager = new TileManager();
    particle_effects = new ParticleEffectsManager(this);
    game_context = nullptr; // Will be initialized after RenderingFacade is ready
}

ClanBomberApplication::~ClanBomberApplication() {
    delete_all_game_objects();
    if (map) {
        delete map;
        map = nullptr;
    }
    // REMOVED: gpu_renderer deletion - now handled by RenderingFacade
    if (lifecycle_manager) {
        delete lifecycle_manager;
        lifecycle_manager = nullptr;
    }
    if (tile_manager) {
        delete tile_manager;
        tile_manager = nullptr;
    }
    if (particle_effects) {
        delete particle_effects;
        particle_effects = nullptr;
    }
    if (game_context) {
        delete game_context;
        game_context = nullptr;
    }
}

void ClanBomberApplication::initialize_game_context() {
    if (text_renderer && lifecycle_manager && 
        tile_manager && particle_effects) {
        
        game_context = new GameContext(
            lifecycle_manager,
            tile_manager,
            particle_effects,
            nullptr,  // Map will be set later via set_map()
            nullptr,  // RenderingFacade will handle GPU rendering
            text_renderer
        );
        
        SDL_Log("GameContext initialized successfully (map will be set later)");
        
        // Set GameContext in TileManager
        if (tile_manager) {
            tile_manager->set_context(game_context);
        }
        
        // If map is already available, set it now
        if (map) {
            game_context->set_map(map);
        }
    } else {
        SDL_Log("ERROR: Cannot initialize GameContext - missing dependencies:");
        SDL_Log("  text_renderer: %p", text_renderer);
        SDL_Log("  lifecycle_manager: %p", lifecycle_manager);
        SDL_Log("  tile_manager: %p", tile_manager);
        SDL_Log("  particle_effects: %p", particle_effects);
    }
}

bool ClanBomberApplication::is_server() {
    return false; // For now, simple local game
}

bool ClanBomberApplication::is_client() {
    return false; // For now, simple local game
}

Server* ClanBomberApplication::get_server() {
    return nullptr; // For now, no server
}

Client* ClanBomberApplication::get_client() {
    return nullptr; // For now, no client
}

ServerSetup* ClanBomberApplication::get_server_setup() {
    return nullptr; // For now, no server setup
}

ClientSetup* ClanBomberApplication::get_client_setup() {
    return nullptr; // For now, no client setup
}

Chat* ClanBomberApplication::get_chat() {
    return nullptr; // For now, no chat
}

Menu* ClanBomberApplication::get_menu() {
    return nullptr; // For now, no menu
}

unsigned short ClanBomberApplication::get_next_object_id() {
    static unsigned short next_id = 1;
    return next_id++;
}

std::filesystem::path ClanBomberApplication::get_map_path() {
    return "data/maps/";
}

std::filesystem::path ClanBomberApplication::get_local_map_path() {
    return "data/maps/";
}

void ClanBomberApplication::lock() {
    // For now, no-op
}

void ClanBomberApplication::unlock() {
    // For now, no-op
}

void ClanBomberApplication::wait() {
    // For now, no-op
}

void ClanBomberApplication::signal() {
    // For now, no-op
}

void ClanBomberApplication::delete_all_game_objects() {
    for (auto& obj : objects) {
        delete obj;
    }
    objects.clear();
    
    for (auto& bomber : bomber_objects) {
        delete bomber;
    }
    bomber_objects.clear();
}

GameObject* ClanBomberApplication::get_object_by_id(int object_id) {
    for (auto& obj : objects) {
        if (obj->get_object_id() == object_id) {
            return obj;
        }
    }
    return nullptr;
}

int ClanBomberApplication::get_server_frame_counter() {
    return 0;
}

void ClanBomberApplication::inc_server_frame_counter() {
    // No-op for now
}

bool ClanBomberApplication::paused_game() {
    return pause_game;
}

void ClanBomberApplication::set_pause_game(bool paused) {
    pause_game = paused;
}

void ClanBomberApplication::set_client_disconnected_from_server(bool d) {
    client_disconnected_from_server = d;
}

bool ClanBomberApplication::is_client_disconnected_from_server() {
    return client_disconnected_from_server;
}

void ClanBomberApplication::set_client_connecting_to_new_server(bool c) {
    client_connecting_to_new_server = c;
}

bool ClanBomberApplication::is_client_connecting_to_new_server() {
    return client_connecting_to_new_server;
}

Map* ClanBomberApplication::get_map() {
    return map;
}

std::filesystem::path ClanBomberApplication::map_path;
std::filesystem::path ClanBomberApplication::local_map_path;
int ClanBomberApplication::server_frame_counter = 0;