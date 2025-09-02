#include <set>
#include <SDL3/SDL.h>

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
#include "GameObject.h"
#include "Resources.h"
#include "GameContext.h"
#include "TileManager.h"
#include "RenderingFacade.h"
#include "CoordinateSystem.h"
#include "SpatialPartitioning.h"

#include "Map.h"
#include "MapTile.h"
#include "TileEntity.h"
#include "Timer.h"
#include "Bomber.h"
#include "Bomb.h"
#include "Server.h"
#include "Client.h"
#include "Mutex.h"
#include "Utils.h"

#include <iostream>
#include <math.h>

const char* GameObject::objecttype2string(ObjectType t)
{
    switch(t) {
        case GameObject::BOMB:
            return "*BOMB*";
        case GameObject::BOMBER:
            return "*BOMBER*";
        case GameObject::BOMBER_CORPSE:
            return "*BOMBER_CORPSE*";
        case GameObject::EXPLOSION:
            return "*EXPLOSION*";
        case GameObject::EXTRA:
            return "*EXTRA*";
        case GameObject::OBSERVER:
            return "*OBSERVER*";
        case GameObject::CORPSE_PART:
            return "*CORPSE_PART*";
    }
    return "*UNKNOWN*";
}


GameObject::GameObject( int _x, int _y, GameContext* context )
{
	// GAMECONTEXT CONSTRUCTOR: Modern dependency injection
	game_context = context;
	
	texture_name = "bomber_snake"; // placeholder
	sprite_nr = 0;

	offset_x = 0;  // No offset by default - objects set their own if needed
	offset_y = 0;  // No offset by default - objects set their own if needed
	delete_me = false;
	remainder =0;
	speed = 240;
	
	cur_dir = DIR_NONE;
	can_kick = false;
	can_pass_bomber = false;
	can_fly_over_walls = true;
	flying = false;
	falling = false;
	fallen_down = false;
	stopped = false;
	fly_progress = 0;	// must be set to 0!

	// CENTER-BASED COORDINATES: GameObject now stores center coordinates
	// All x,y coordinates represent the CENTER of the object, not top-left corner
	// This unifies the coordinate system throughout the game
	x = orig_x = _x;  // Center X coordinate
	y = orig_y = _y;  // Center Y coordinate
	z = 0;

	opacity = 0xff;
	opacity_scaled = 0xff;

	if (ClanBomberApplication::is_server()) {
		object_id = ClanBomberApplication::get_next_object_id();
	}
	else {
		object_id = 0;
	}
	server_dir = cur_dir;
	client_dir = cur_dir;
	local_dir = cur_dir;
	server_x = (int)x;
	server_y = (int)y;
	reset_next_fly_job();
	
	// NOTE: Object registration is handled explicitly by caller
	// This prevents double registration issues and gives explicit control
}

GameContext* GameObject::get_context() const
{
	// GAMECONTEXT ONLY: All objects use GameContext constructor
	return game_context;
}

GameObject::~GameObject()
{
}

int GameObject::get_object_id()
{
    return object_id;
}

void GameObject::set_object_id(int obj_id)
{
    object_id = obj_id;
}

int GameObject::get_server_x()
{
    return server_x;
}

int GameObject::get_server_y()
{
    return server_y;
}

int GameObject::get_orig_x()
{
	return orig_x;
}

int GameObject::get_orig_y()
{
	return orig_y;
}

Direction GameObject::get_server_dir()
{
    return server_dir;
}

Direction GameObject::get_client_dir()
{
    return client_dir;
}

void GameObject::set_server_x(int sx)
{
    server_x = sx;
}

void GameObject::set_server_y(int sy)
{
    server_y = sy;
}

void GameObject::set_server_dir(int sd)
{
    server_dir = (Direction)sd;
}

void GameObject::set_client_dir(int cd)
{
    client_dir = (Direction)cd;
}

void GameObject::set_local_dir(int ld)
{
    local_dir = (Direction)ld;
}

void GameObject::set_cur_dir(int cd)
{
    cur_dir = (Direction)cd;
}

void GameObject::set_offset(int _x, int _y)
{
    offset_x = _x;
    offset_y = _y;
}

// LEGACY MOVEMENT FUNCTIONS REMOVED - Game uses move_dist() system instead

// Handle bomb kicking if movement failed due to bomb

bool GameObject::is_blocked(float check_x, float check_y) {
    // MODERN COLLISION: Use SpatialGrid only - no more legacy tile->bomb system
    GameContext* context = get_context();
    if (!context) {
        return false; // If no context, consider not blocked (defensive)
    }
    
    // ADAPTIVE HITBOX: Use 75% of tile size for consistent movement while allowing precision
    const float ADAPTIVE_HITBOX = 30.0f; // 75% of 40px = 30px, works for all sprite sizes
    float bbox_left = check_x - ADAPTIVE_HITBOX/2;
    float bbox_right = check_x + ADAPTIVE_HITBOX/2 - 1;
    float bbox_top = check_y - ADAPTIVE_HITBOX/2;  
    float bbox_bottom = check_y + ADAPTIVE_HITBOX/2 - 1;

    // Use CoordinateSystem for tile calculations
    GridCoord grid_top_left = CoordinateSystem::pixel_to_grid(PixelCoord(bbox_left, bbox_top));
    GridCoord grid_bottom_right = CoordinateSystem::pixel_to_grid(PixelCoord(bbox_right, bbox_bottom));
    int map_x1 = grid_top_left.grid_x;
    int map_y1 = grid_top_left.grid_y;
    int map_x2 = grid_bottom_right.grid_x;
    int map_y2 = grid_bottom_right.grid_y;

    // SDL_Log("   Checking tiles from (%d,%d) to (%d,%d)", map_x1, map_y1, map_x2, map_y2);
    
    // Check static tiles (walls/boxes) 
    Map* map = context->get_map();
    if (map) {
        for (int my = map_y1; my <= map_y2; ++my) {
            for (int mx = map_x1; mx <= map_x2; ++mx) {
                MapTile* tile = map->get_tile(mx, my);
                if (!tile) {
                    // SDL_Log("   Tile(%d,%d): NULL - BLOCKED", mx, my);
                    return true;
                }
                
                if (tile->is_blocking()) {
                    // SDL_Log("   Tile(%d,%d): WALL - BLOCKED", mx, my);
                    return true; // Wall collision
                }
            }
        }
    }
    
    // MODERN COLLISION: Check dynamic objects (bombs, bombers) using SpatialGrid
    // Get current position tiles using SAME adaptive hitbox for consistency
    std::set<int> current_tiles;
    float current_left = x - ADAPTIVE_HITBOX/2;
    float current_right = x + ADAPTIVE_HITBOX/2 - 1;
    float current_top = y - ADAPTIVE_HITBOX/2;  
    float current_bottom = y + ADAPTIVE_HITBOX/2 - 1;
    
    // Use CoordinateSystem for current position tile calculations
    GridCoord current_top_left = CoordinateSystem::pixel_to_grid(PixelCoord(current_left, current_top));
    GridCoord current_bottom_right = CoordinateSystem::pixel_to_grid(PixelCoord(current_right, current_bottom));
    int cx1 = current_top_left.grid_x, cy1 = current_top_left.grid_y;
    int cx2 = current_bottom_right.grid_x, cy2 = current_bottom_right.grid_y;
    for (int my = cy1; my <= cy2; ++my) {
        for (int mx = cx1; mx <= cx2; ++mx) {
            current_tiles.insert(my * 1000 + mx); // Simple hash for tile coordinates
        }
    }
    
    // Check for bomb collisions using SpatialGrid
    PixelCoord position(check_x, check_y);
    auto bombs = context->get_spatial_grid()->get_objects_of_type_near(position, GameObject::BOMB, 1);
    for (GameObject* bomb_obj : bombs) {
        if (get_type() != BOMB && bomb_obj != this) {
            // BOMB ESCAPE SYSTEM: Check if bomber can ignore collision with this bomb
            bool should_ignore_bomb = false;
            if (get_type() == BOMBER) {
                Bomber* bomber = static_cast<Bomber*>(this);
                Bomb* bomb = static_cast<Bomb*>(bomb_obj);
                if (bomber->can_ignore_bomb_collision(bomb)) {
                    should_ignore_bomb = true;
                    SDL_Log("🎯 BOMB ESCAPE: Ignoring collision - bomber on top of placed bomb at (%d,%d)", bomb_obj->get_x(), bomb_obj->get_y());
                } else {
                    SDL_Log("⚠️  BOMB COLLISION ENABLED: Bomber at (%d,%d), bomb at (%d,%d)", (int)check_x, (int)check_y, bomb_obj->get_x(), bomb_obj->get_y());
                }
            }
            
            if (!should_ignore_bomb) {
                // TILE-PERFECT BOMB COLLISION: Use CoordinateSystem for perfect tile comparison
                GridCoord bomber_grid = CoordinateSystem::pixel_to_grid(PixelCoord(check_x, check_y));
                GridCoord bomb_grid = CoordinateSystem::pixel_to_grid(PixelCoord(bomb_obj->get_x(), bomb_obj->get_y()));
                int bomber_tile_x = bomber_grid.grid_x;
                int bomber_tile_y = bomber_grid.grid_y;
                int bomb_tile_x = bomb_grid.grid_x;
                int bomb_tile_y = bomb_grid.grid_y;
                
                if (bomber_tile_x == bomb_tile_x && bomber_tile_y == bomb_tile_y) {
                    SDL_Log("🚫 BOMB COLLISION: Bomber at tile (%d,%d) blocked by bomb at tile (%d,%d)", 
                            bomber_tile_x, bomber_tile_y, bomb_tile_x, bomb_tile_y);
                    return true; // Only block if bomber is in exact same tile as bomb
                }
            }
        }
    }
    
    // Check for bomber collisions using SpatialGrid
    if (!can_pass_bomber) {
        auto bombers = context->get_spatial_grid()->get_objects_of_type_near(position, GameObject::BOMBER, 1);
        for (GameObject* bomber_obj : bombers) {
            if (bomber_obj != this) {
                GridCoord bomber_grid = CoordinateSystem::pixel_to_grid(PixelCoord(bomber_obj->get_x(), bomber_obj->get_y()));
                int bomber_tile_x = bomber_grid.grid_x;
                int bomber_tile_y = bomber_grid.grid_y;
                int bomber_tile_hash = bomber_tile_y * 1000 + bomber_tile_x;
                
                if (current_tiles.find(bomber_tile_hash) == current_tiles.end()) {
                    // SDL_Log("   Tile(%d,%d): OTHER BOMBER - BLOCKED (SpatialGrid)", bomber_tile_x, bomber_tile_y);
                    return true;
                }
            }
        }
    }
    
    // SDL_Log("✅ COLLISION: Position (%.1f,%.1f) is FREE", check_x, check_y);
    return false;
}

bool GameObject::move_dist(float distance, Direction dir) {
    // SDL_Log("🚶 MOVE: Trying to move distance=%.2f direction=%d from (%.1f,%.1f)", 
    //         distance, (int)dir, x, y);
    
    if (flying || falling) {
        // SDL_Log("🚫 MOVE: Blocked - flying=%d falling=%d", flying, falling);
        return false;
    }

    // SPATIAL FIX: Store old position for SpatialGrid update
    float old_x = x;
    float old_y = y;
    bool moved = false;

    float move_x = 0;
    float move_y = 0;

    switch (dir) {
           case DIR_LEFT:  move_x = -distance; break;
           case DIR_RIGHT: move_x =  distance; break;
           case DIR_UP:    move_y = -distance; break;
           case DIR_DOWN:  move_y =  distance; break;
           default: 
               SDL_Log("🚫 MOVE: Invalid direction %d", (int)dir);
               return false;
       }
   
       float next_x = x + move_x;
       float next_y = y + move_y;
       
       SDL_Log("🎯 MOVE: Target position (%.1f,%.1f)", next_x, next_y);
   
       if (!is_blocked(next_x, next_y)) {
           // Direct path is clear
           SDL_Log("✅ MOVE: Direct path clear - moving to (%.1f,%.1f)", next_x, next_y);
           x = next_x;
           y = next_y;
           moved = true;
       } else {
           SDL_Log("🚫 MOVE: Direct path blocked - trying partial movement");
       }
   
       // Path is blocked, try partial movement first (for wiggling)
       float d = distance;
       while (d > 0) {
           d -= 1.0;
           if (d < 0) d = 0;
           float p_x = x + (move_x * d / distance);
           float p_y = y + (move_y * d / distance);
           if (!is_blocked(p_x, p_y)) {
               x = p_x;
               y = p_y;
               moved = true;
               break;
           }
           if (d == 0) break;
       }
   
       // No partial move possible, now try to slide for cornering
       if (!moved) {
           const float slide_amount = 1.0;
           if (dir == DIR_LEFT || dir == DIR_RIGHT) {
               // Try sliding vertically
               if (!is_blocked(next_x, y + slide_amount)) {
                   y += slide_amount;
                   x = next_x;
                   moved = true;
               }
               else if (!is_blocked(next_x, y - slide_amount)) {
                   y -= slide_amount;
                   x = next_x;
                   moved = true;
               }
           } else { // Moving vertically
               // Try sliding horizontally
               if (!is_blocked(x + slide_amount, next_y)) {
                   x += slide_amount;
                   y = next_y;
                   moved = true;
               }
               else if (!is_blocked(x - slide_amount, next_y)) {
                   x -= slide_amount;
                   y = next_y;
                   moved = true;
               }
           }
       }
       
       // SPATIAL FIX: Update SpatialGrid if we moved
       if (moved) {
           GameContext* context = get_context();
           if (context) {
               SDL_Log("SPATIAL DEBUG: Updating object type=%d position from (%.1f,%.1f) to (%.1f,%.1f)", 
                       get_type(), old_x, old_y, x, y);
               context->update_object_position_in_spatial_grid(this, old_x, old_y);
           }
           return true;
       }
   
       return false; // Completely blocked
   }


bool GameObject::move(float deltaTime)
{
	// MODERN MOVEMENT: Use move_dist() instead of legacy pixel-by-pixel movement
	if (!flying && cur_dir != DIR_NONE) {
		// Calculate movement distance based on speed and deltaTime
		float distance = speed * deltaTime;
		
		// Use modern move_dist() system
		if (!move_dist(distance, cur_dir)) {
			stop();
			return false;
		}
	}
	return true;
}

void GameObject::act(float deltaTime)
{
	stopped = false;
	if (flying) {
	 	continue_flying(deltaTime);
		if (flying && is_next_fly_job()) {
			flying = false;
        	fly_progress = 1;
           	x = fly_dest_x;
            y = fly_dest_y;
		}
        return;
    }
    if (!ClanBomberApplication::is_server() && ClanBomberApplication::is_client() && is_next_fly_job()) {
		fly_to(next_fly_job[0], next_fly_job[1], next_fly_job[2]);
		reset_next_fly_job();
		return;
	}
	if (falling) {   
 	 	continue_falling(deltaTime);
 	}
}

void GameObject::fly_to (int _x, int _y, int _speed)
{
	if (!flying && !falling && !fallen_down) {
		flying = true;
		
		fly_dest_x = _x;
		fly_dest_y = _y;
		
		fly_progress = 0;	// goes from 0 to 1
		fly_dist_x = _x - x;
		fly_dist_y = _y - y;
		
		fly_speed = _speed ? _speed : speed;
        int send_speed=(int)fly_speed;
		fly_speed /= sqrt( fly_dist_x*fly_dist_x + fly_dist_y*fly_dist_y );
		z += Z_FLYING;
		if (ClanBomberApplication::is_server()) {
			if (get_type() == OBSERVER) {
				ClanBomberApplication::get_server()->send_SERVER_OBSERVER_FLY(_x, _y, send_speed);
			}
			else if (get_type() != CORPSE_PART) {
				ClanBomberApplication::get_server()->send_SERVER_OBJECT_FLY(_x, _y, send_speed, can_fly_over_walls, object_id);
			}
		}
		else if (ClanBomberApplication::is_client()) {
			if (get_type() == OBSERVER) {
                          // if (auto sound = Resources::Observer_crunch()) sound->play();
			}
		}
	}
}

void GameObject::fly_to (MapTile *maptile, int _speed)
{
	if (maptile) {
		fly_to( maptile->get_x(), maptile->get_y(), _speed );
	}
}

void GameObject::continue_flying(float deltaTime)
{
	if (deltaTime == 0) {
		return;
	}
	float time_span = deltaTime;
	float time_step = time_span;
	int steps = 1;
	while( abs((int)(time_step*fly_speed*fly_dist_x)) > 5  ||  abs((int)(time_step*fly_speed*fly_dist_y)) > 5 ) {
		time_step /= 2.0f;
		steps *= 2;
	}

	while (steps--) {
		x += time_step * fly_speed * fly_dist_x;
		y += time_step * fly_speed * fly_dist_y;
		
		// GAMECONTEXT MIGRATION: Use GameContext for map checks
		GameContext* context = get_context();
		Map* map = context ? context->get_map() : nullptr;
		
		if (get_type() == CORPSE_PART  &&  map != nullptr) {
			if (!can_fly_over_walls && get_tile_type_at(x+20, y+20) == MapTile::WALL) {
				x -= time_step * fly_speed * fly_dist_x;
				y -= time_step * fly_speed * fly_dist_y;
				fly_dest_x = x;
				fly_dest_y = y;
//				break;
			}
		}
		else {
			if (!can_fly_over_walls  &&  map != nullptr  &&  (
				get_tile_type_at(x, y) == MapTile::WALL ||
				get_tile_type_at(x+39, y) == MapTile::WALL ||
				get_tile_type_at(x, y+39) == MapTile::WALL ||
				get_tile_type_at(x+39, y+39) == MapTile::WALL )) 
			{
				x -= time_step * fly_speed * fly_dist_x;
				y -= time_step * fly_speed * fly_dist_y;
				fly_dest_x = x;
				fly_dest_y = y;
//				break;
			}
		}
	
		fly_progress += time_step * fly_speed;
	
		if (fly_progress >= 1) {
			flying = false;
			fly_progress = 1;	// do not set to 0! 1 indicates a finished flight!
			if (z > Z_FLYING) {
				z -= Z_FLYING;
			}
			x = fly_dest_x;
			y = fly_dest_y;
		}
	}
}


void GameObject::fall()
{
	if (!falling) {
		if (ClanBomberApplication::is_server()) {
			ClanBomberApplication::get_server()->send_SERVER_OBJECT_FALL(get_object_id());
		}
		falling = true;
		z = Z_FALLING;
		fall_countdown = 1.0f;
		if (get_type() != CORPSE_PART  &&  get_type() != EXTRA) {
                  // if (auto sound = Resources::Game_deepfall()) sound->play();
		}
	}
}

void GameObject::continue_falling(float deltaTime)
{
	speed = (int)(fall_countdown*60);
	fall_countdown -= deltaTime;
	if (fall_countdown < 0) {
		fallen_down = true;
		fall_countdown = 0;
	}
	opacity_scaled = (Uint8)(fall_countdown * 255);
}

void GameObject::stop(bool by_arrow)
{
	stopped = true;
}

void GameObject::snap()
{
	x = ((get_x()+20)/40)*40;
	y = ((get_y()+20)/40)*40;
}

void GameObject::set_dir ( Direction _dir)
{
	cur_dir = _dir;
}

void GameObject::set_pos( int _x, int _y )
{
	x = _x;
	y = _y;
}

void GameObject::set_orig( int _x, int _y )
{
	orig_x = _x;
	orig_y = _y;
}

void GameObject::move_pos( int _x, int _y )
{
	// SPATIAL FIX: Store old position before updating
	float old_x = x;
	float old_y = y;
	
	x += _x;
	y += _y;
	
	// SPATIAL FIX: Update SpatialGrid with new position
	GameContext* context = get_context();
	if (context) {
		context->update_object_position_in_spatial_grid(this, old_x, old_y);
	}
}

int GameObject::get_x() const
{
	return (int)x;
}

int GameObject::get_y() const
{
	return (int)y;
}

int GameObject::get_z() const
{
	return z;
}

int GameObject::get_speed() const
{
	return speed;
}

int GameObject::get_map_x() const
{
    // FIXED: Use center coordinates directly to determine tile
    // Previous (get_x()+20)/40 caused objects near edges to be detected in wrong tile
    int tmp = get_x()/40;
	if (tmp < 0) {
		tmp = 0;
	}
	else if (tmp >= MAP_WIDTH) {
		tmp = MAP_WIDTH-1;
	}
	return tmp;
}

int GameObject::get_map_y() const
{
	// FIXED: Use center coordinates directly to determine tile
	// Previous (get_y()+20)/40 caused objects near edges to be detected in wrong tile
	int tmp = get_y()/40;
	if (tmp < 0) {
        tmp = 0; 
    }
    else if (tmp >= MAP_HEIGHT) {
        tmp = MAP_HEIGHT-1;
    }
    return tmp;
}

void GameObject::inc_speed( int _c )
{
	speed += _c;
}

void GameObject::dec_speed( int _c )
{
	speed -= _c;
}

void GameObject::set_speed( int _speed )
{
	speed = _speed;
}

bool GameObject::is_flying() const
{
	return flying;
}

bool GameObject::is_stopped() const
{
	return stopped;
}

Direction GameObject::get_cur_dir() const
{
	return cur_dir;
}

int GameObject::whats_left()
{
	return get_tile_type_at(x-1, y+20);
}

int GameObject::whats_right()
{
	return get_tile_type_at(x+40, y+20);
}

int GameObject::whats_up()
{
	return get_tile_type_at(x+20, y-1);
}

int GameObject::whats_down()
{
	return get_tile_type_at(x+20, y+40);
}

MapTile* GameObject::get_tile() const
{
	// GAMECONTEXT MIGRATION: Use GameContext instead of direct app access
	GameContext* context = get_context();
	if (!context || !context->get_map()) {
		SDL_Log("ERROR: get_tile called with null context or map");
		return nullptr;
	}
	Map* map = context->get_map();
	
	return map->get_tile( (int)(x+20)/40, (int)(y+20)/40 );
}

// NEW ARCHITECTURE SUPPORT: Get legacy tile
MapTile* GameObject::get_legacy_tile() const
{
	// GAMECONTEXT MIGRATION: Use GameContext instead of direct app access
	GameContext* context = get_context();
	if (!context || !context->get_map()) {
		SDL_Log("ERROR: get_legacy_tile called with null context or map");
		return nullptr;
	}
	Map* map = context->get_map();
	
	return map->get_tile( (int)(x+20)/40, (int)(y+20)/40 );
}

// NEW ARCHITECTURE SUPPORT: Get new TileEntity
TileEntity* GameObject::get_tile_entity() const
{
	// GAMECONTEXT MIGRATION: Use GameContext instead of direct app access
	GameContext* context = get_context();
	if (!context || !context->get_map()) {
		SDL_Log("ERROR: get_tile_entity called with null context or map");
		return nullptr;
	}
	Map* map = context->get_map();
	
	return map->get_tile_entity( (int)(x+20)/40, (int)(y+20)/40 );
}

// NEW ARCHITECTURE SUPPORT: Get tile type at pixel position (works with both architectures)
int GameObject::get_tile_type_at(int pixel_x, int pixel_y) const
{
	// GAMECONTEXT MIGRATION: Use GameContext instead of direct app access
	GameContext* context = get_context();
	if (!context || !context->get_map()) {
		SDL_Log("ERROR: get_tile_type_at called with null context or map");
		return MapTile::GROUND;
	}
	Map* map = context->get_map();
	
	int map_x = pixel_x / 40;
	int map_y = pixel_y / 40;
	
	// Try legacy MapTile first
	MapTile* legacy_tile = map->get_tile(map_x, map_y);
	if (legacy_tile) {
		return legacy_tile->get_tile_type();
	}
	
	// Try new TileEntity
	TileEntity* tile_entity = map->get_tile_entity(map_x, map_y);
	if (tile_entity) {
		return tile_entity->get_tile_type();
	}
	
	// Default to GROUND if no tile found
	return MapTile::GROUND;
}

// NEW ARCHITECTURE SUPPORT: Check if tile is blocking at pixel position (works with both architectures)
bool GameObject::is_tile_blocking_at(int pixel_x, int pixel_y) const
{
	// GAMECONTEXT ONLY: No fallback needed - all objects use GameContext
	GameContext* context = get_context();
	if (!context || !context->get_map()) {
		SDL_Log("ERROR: is_tile_blocking_at called with null context or map");
		return false;
	}
	
	int map_x = pixel_x / 40;
	int map_y = pixel_y / 40;
	return context->is_position_blocked(map_x, map_y);
}

// NEW ARCHITECTURE SUPPORT: Check if tile has bomb at pixel position
bool GameObject::has_bomb_at(int pixel_x, int pixel_y) const
{
	// GAMECONTEXT ONLY: No fallback needed - all objects use GameContext
	GameContext* context = get_context();
	if (!context || !context->get_map()) {
		SDL_Log("ERROR: has_bomb_at called with null context or map");
		return false;
	}
	
	int map_x = pixel_x / 40;
	int map_y = pixel_y / 40;
	return context->has_bomb_at(map_x, map_y);
}

// NEW ARCHITECTURE SUPPORT: Check if tile has bomber at pixel position (works with both architectures)
bool GameObject::has_bomber_at(int pixel_x, int pixel_y) const
{
	// GAMECONTEXT MIGRATION: Use GameContext instead of direct app access
	GameContext* context = get_context();
	if (!context || !context->get_map()) {
		SDL_Log("ERROR: has_bomber_at called with null context or map");
		return false;
	}
	Map* map = context->get_map();
	
	int map_x = pixel_x / 40;
	int map_y = pixel_y / 40;
	
	// Try legacy MapTile first
	MapTile* legacy_tile = map->get_tile(map_x, map_y);
	if (legacy_tile) {
		return legacy_tile->has_bomber();
	}
	
	// Try new TileEntity
	TileEntity* tile_entity = map->get_tile_entity(map_x, map_y);
	if (tile_entity) {
		return tile_entity->has_bomber();
	}
	
	// Default to no bomber if no tile found
	return false;
}

// NEW ARCHITECTURE SUPPORT: Synchronize bomb with TileManager
// LEGACY FUNCTIONS REMOVED - SpatialGrid handles all collision detection
// These functions maintained the legacy tile->bomb system which caused phantom collisions
// Modern architecture uses only SpatialGrid for dynamic object collisions

void GameObject::set_bomb_on_tile(Bomb* bomb) const {
    // NO-OP: Legacy function removed - SpatialGrid automatically handles bomb positioning
    SDL_Log("GameObject: set_bomb_on_tile() called but legacy system removed - SpatialGrid handles collision");
}

void GameObject::remove_bomb_from_tile(Bomb* bomb) const {
    // NO-OP: Legacy function removed - SpatialGrid automatically handles bomb cleanup via delete_me flag
    SDL_Log("GameObject: remove_bomb_from_tile() called but legacy system removed - SpatialGrid handles cleanup");
}

void GameObject::show()
{
    if (texture_name.empty()) {
        return;
    }
    
    // GAMECONTEXT ONLY: LifecycleManager controls rendering
    GameContext* context = get_context();
    if (!context || !context->get_lifecycle_manager()) {
        SDL_Log("ERROR: show() called with null context or lifecycle_manager");
        return;
    }
    
    LifecycleManager::ObjectState state = context->get_lifecycle_manager()->get_object_state(this);
    if (state == LifecycleManager::ObjectState::DELETED) {
        return;  // Object is truly dead, don't render
    }
    // Continue rendering for ACTIVE, DYING, and DEAD states

    // OPTIMIZED: Use RenderingFacade for centralized rendering instead of direct GPU renderer access
    RenderingFacade* facade = context->get_rendering_facade();
    if (facade) {
        // SMART COORDINATE SYSTEM: Apply center→corner conversion only for dynamic objects
        float render_x, render_y;
        
        if (get_type() == MAPTILE) {
            // TileEntity: Already uses top-left coordinates, render as-is
            render_x = static_cast<float>(get_x());
            render_y = static_cast<float>(get_y());
        } else {
            // Dynamic objects (BOMBER, BOMB, etc.): Convert center→top-left for rendering
            const int SPRITE_WIDTH = 40;
            const int SPRITE_HEIGHT = 40;
            render_x = static_cast<float>(get_x()) - (SPRITE_WIDTH / 2);   // Center → top-left
            render_y = static_cast<float>(get_y()) - (SPRITE_HEIGHT / 2);  // Center → top-left
        }
        
        PixelCoord position(render_x, render_y);
        
        auto result = facade->render_sprite(texture_name, position, sprite_nr, 0.0f, opacity_scaled);
        if (!result.is_ok()) {
            SDL_Log("WARNING: GameObject::show() failed to render sprite '%s': %s (Context: %s)", 
                texture_name.c_str(), result.get_error_message().c_str(), result.get_error_context().c_str());
            
            // NO FALLBACK: RenderingFacade is the only rendering system
            SDL_Log("ERROR: GameObject::show() - RenderingFacade failed to render sprite '%s'", 
                texture_name.c_str());
        }
    } else {
        // NO FALLBACK: RenderingFacade is the only rendering system
        SDL_Log("ERROR: GameObject::show() - RenderingFacade not available, cannot render sprite '%s'", 
            texture_name.c_str());
    }
}

void GameObject::show(int _x, int _y) const
{
}

void GameObject::show(int _x, int _y, float _scale) const
{
}

bool GameObject::is_falling()
{
	return (falling && !fallen_down);
}

void GameObject::output_object_info()
{
  std::cout
	<<" type="<<objecttype2string(get_type())
	<<" id="<<object_id
	<<" x="<<x
	<<" y="<<y
	<<" z="<<z
	<<" del_me="<<(int)delete_me
	<<" is_flying="<<(int)flying
	<<" is_falling="<<(int)falling
	<<" to_x="<<fly_dest_x
	<<" to_y="<<fly_dest_y
	<<" progress="<<fly_progress
	<<" dist_x="<<fly_dist_x
	<<" dist_y="<<fly_dist_y
	<<" speed="<<fly_speed
	<< std::endl;
}

void GameObject::set_next_fly_job(int flyjobx, int flyjoby, int flyjobspeed)
{
	next_fly_job[0] = flyjobx;
    next_fly_job[1] = flyjoby;
    next_fly_job[2] = flyjobspeed;
}

void GameObject::reset_next_fly_job()
{
	next_fly_job[0] = 0;
	next_fly_job[1] = 0;
	next_fly_job[2] = 0;
}

bool GameObject::is_next_fly_job()
{
    return (next_fly_job[0] != 0 || next_fly_job[1] != 0 || next_fly_job[2] != 0);
}

