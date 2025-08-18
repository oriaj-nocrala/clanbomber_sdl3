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

	offset_x = 60;
	offset_y = 40;
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

	x = orig_x = _x;
	y = orig_y = _y;
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

bool GameObject::move_right()
{
	// Check if tile to the right is blocking
	if (is_tile_blocking_at(x+40, y+20)) {
		return false;
	}
		
	// Check if there's a bomb (unless this object is the bomb or already partially moved)
	if (has_bomb_at(x+40, y+20) && !(get_type()==BOMB && has_bomb_at(x+20, y+20)) && !(get_x()%40>19)) {
		// Try to kick the bomb if possible
		if (can_kick) {
			return try_kick_right();
		}
		return false;
	}
	
	x++;

	// Collision correction for partial tile alignment
	if (get_y()%40 > 19) {
		if (is_tile_blocking_at(get_x()+40, get_y()-20)) {
			y++;
		}
	}
	else if (get_y()%40 > 0) {
		if (is_tile_blocking_at(get_x()+40, get_y()+60)) {
			y--;
		}
	}

	// Check for bomber collision (if not allowed to pass bombers)
	if (!can_pass_bomber) {
		if (has_bomber_at(x+20, y+20)) {  // Check current position after move
			if (!has_bomber_at(x-1, y+20)) {  // Only if not already on same tile
				x--;
				return false;
			}
		}
	}
	
	return true;
}

// Handle bomb kicking if movement failed due to bomb
bool GameObject::try_kick_right()
{
	// GAMECONTEXT MIGRATION: Use GameContext instead of direct app access
	GameContext* context = get_context();
	if (!context || !context->get_map()) {
		SDL_Log("ERROR: try_kick_right called with null context or map");
		return false;
	}
	Map* map = context->get_map();
	
	// Check if there's a bomb to kick and we can kick
	if (!can_kick || !has_bomb_at(x+40, y+20)) {
		return false;
	}
	
	// Get bomb from either architecture
	Bomb* bomb_to_kick = nullptr;
	MapTile* legacy_tile = map->get_tile(pixel_to_map_x(x+40), pixel_to_map_y(y+20));
	if (legacy_tile && legacy_tile->bomb) {
		bomb_to_kick = legacy_tile->bomb;
	} else {
		TileEntity* tile_entity = map->get_tile_entity(pixel_to_map_x(x+40), pixel_to_map_y(y+20));
		if (tile_entity) {
			bomb_to_kick = tile_entity->get_bomb();
		}
	}
	
	if (bomb_to_kick && bomb_to_kick->get_cur_dir() == DIR_NONE) {
		SDL_Log("Kicking bomb right");
		bomb_to_kick->kick(DIR_RIGHT);
		return true;
	}
	
	return false;
}

// Handle bomb kicking if movement failed due to bomb
bool GameObject::try_kick_left()
{
	// GAMECONTEXT MIGRATION: Use GameContext instead of direct app access
	GameContext* context = get_context();
	if (!context || !context->get_map()) {
		SDL_Log("ERROR: try_kick_left called with null context or map");
		return false;
	}
	Map* map = context->get_map();
	
	// Check if there's a bomb to kick and we can kick
	if (!can_kick || !has_bomb_at(x-1, y+20)) {
		return false;
	}
	
	// Get bomb from either architecture
	Bomb* bomb_to_kick = nullptr;
	MapTile* legacy_tile = map->get_tile(pixel_to_map_x(x-1), pixel_to_map_y(y+20));
	if (legacy_tile && legacy_tile->bomb) {
		bomb_to_kick = legacy_tile->bomb;
	} else {
		TileEntity* tile_entity = map->get_tile_entity(pixel_to_map_x(x-1), pixel_to_map_y(y+20));
		if (tile_entity) {
			bomb_to_kick = tile_entity->get_bomb();
		}
	}
	
	if (bomb_to_kick && bomb_to_kick->get_cur_dir() == DIR_NONE) {
		SDL_Log("Kicking bomb left");
		bomb_to_kick->kick(DIR_LEFT);
		return true;
	}
	
	return false;
}

bool GameObject::move_left()
{
	// Check if tile to the left is blocking
	if (is_tile_blocking_at(x-1, y+20)) {
		return false;
	}
		
	// Check if there's a bomb (unless this object is the bomb or already partially moved)
	if (has_bomb_at(x-1, y+20) && !(get_type()==BOMB && has_bomb_at(x+20, y+20)) && !(get_x()%40<20)) {
		// Try to kick the bomb if possible
		if (can_kick) {
			return try_kick_left();
		}
		return false;
	}
	
	x--;

	// Collision correction for partial tile alignment
	if (get_y()%40 > 19) {
		if (is_tile_blocking_at(get_x()-1, get_y()-20)) {
			y++;
		}
	}
	else if (get_y()%40 > 0) {
		if (is_tile_blocking_at(get_x()-1, get_y()+60)) {
			y--;
		}
	}
		
	// Check for bomber collision (if not allowed to pass bombers)
	if (!can_pass_bomber) {
		if (has_bomber_at(x+20, y+20)) {  // Check current position after move
			if (!has_bomber_at(x+41, y+20)) {  // Only if not already on same tile
				x++;
				return false;
			}
		}
	}
	
	return true;
}

bool GameObject::move_up()
{
	// GAMECONTEXT MIGRATION: Use GameContext instead of direct app access
	GameContext* context = get_context();
	if (!context || !context->get_map()) {
		SDL_Log("ERROR: move_up called with null context or map");
		return false;
	}
	Map* map = context->get_map();
	
	MapTile* up_maptile = map->get_tile((int)(x+20)/40, (int)(y-1)/40);
	if (up_maptile->is_blocking()) {
		return false;
	}
		
	if ((up_maptile->bomb==NULL) || ((get_type()==BOMB) && (up_maptile->bomb==this)) || (get_y()%40<20)) {
		y--;
			
		if (get_x()%40 > 19) {
			if (map->get_tile((get_x()-20)/40,(get_y()-1)/40)->is_blocking()) {
				x++;
			}
		}
		else if (get_x()%40 > 0) {
			if (map->get_tile((get_x()+60)/40,(get_y()-1)/40)->is_blocking()) {
				x--;
			}
		}
			
		if (!can_pass_bomber) {
			if ( up_maptile->has_bomber() ) {
				// Check if moving to a different tile (not current position)
				int current_map_x = (x + 20) / 40;
				int current_map_y = (y + 20) / 40;
				int target_map_x = (x + 20) / 40;
				int target_map_y = (y - 1 + 20) / 40;
				
				if (target_map_x != current_map_x || target_map_y != current_map_y) {
					y++;
					return false;
				}
			}
		}
	}
	else {
		SDL_Log("Attempting to kick up. can_kick value: %d", (int)can_kick);
		if (can_kick  && (up_maptile->bomb->get_cur_dir() == DIR_NONE)) {
			up_maptile->bomb->kick(DIR_UP);
		}
		return false;
	}
	
	return true;
}

bool GameObject::move_down()
{
	// GAMECONTEXT MIGRATION: Use GameContext instead of direct app access
	GameContext* context = get_context();
	if (!context || !context->get_map()) {
		SDL_Log("ERROR: move_down called with null context or map");
		return false;
	}
	Map* map = context->get_map();
	
	MapTile* down_maptile = map->get_tile((int)(x+20)/40,(int)(y+40)/40);
	
	if (down_maptile->is_blocking()) {
		return false;
	}
	
	if ((down_maptile->bomb==NULL) || ((get_type()==BOMB) && (down_maptile->bomb==this)) || (get_y()%40>19)) {
		y++;
			
		if (get_x()%40 > 19) {
			if (map->get_tile((get_x()-20)/40,(get_y()+40)/40)->is_blocking()) {
				x++;
			}
		}
		else if (get_x()%40 > 0) {
			if (map->get_tile((get_x()+60)/40,(get_y()+40)/40)->is_blocking()) {
				x--;
			}
		}
			
		if (!can_pass_bomber) {
			if ( down_maptile->has_bomber() ) {
				// Check if moving to a different tile (not current position)
				int current_map_x = (x + 20) / 40;
				int current_map_y = (y + 20) / 40;
				int target_map_x = (x + 20) / 40;
				int target_map_y = (y + 1 + 20) / 40;
				
				if (target_map_x != current_map_x || target_map_y != current_map_y) {
					y--;
					return false;
				}
			}
		}
	}
	else {
		SDL_Log("Attempting to kick down. can_kick value: %d", (int)can_kick);
		if (can_kick  && (down_maptile->bomb->get_cur_dir() == DIR_NONE)) {
			down_maptile->bomb->kick(DIR_DOWN);
		}
		return false;
	}
	
	return true;
}


bool GameObject::is_blocked(float check_x, float check_y) {
    // GAMECONTEXT MIGRATION: Use GameContext instead of direct app access
    GameContext* context = get_context();
    if (!context || !context->get_map()) {
        return false; // If no context/map, consider not blocked (defensive)
    }
    Map* map = context->get_map();
    
    float bbox_left = check_x + 10;
    float bbox_right = check_x + 29;
    float bbox_top = check_y + 10;
    float bbox_bottom = check_y + 29;

    int map_x1 = (int)bbox_left / 40;
    int map_y1 = (int)bbox_top / 40;
    int map_x2 = (int)bbox_right / 40;
    int map_y2 = (int)bbox_bottom / 40;

    std::set<MapTile*> current_tiles;
    int cx1 = (int)x / 40, cy1 = (int)y / 40;
    int cx2 = (int)(x+39) / 40, cy2 = (int)(y+39) / 40;
    for (int my = cy1; my <= cy2; ++my) {
        for (int mx = cx1; mx <= cx2; ++mx) {
            current_tiles.insert(map->get_tile(mx, my));
        }
    }

    for (int my = map_y1; my <= map_y2; ++my) {
        for (int mx = map_x1; mx <= map_x2; ++mx) {
            MapTile* tile = map->get_tile(mx, my);
            if (tile->is_blocking()) {
                return true; // Wall collision
            }
            if (tile->bomb != NULL && get_type() != BOMB) {
                if (current_tiles.find(tile) == current_tiles.end()) {
                    return true; // Bomb collision
                }
            }
            if (!can_pass_bomber && tile->has_bomber() && current_tiles.find(tile) == current_tiles.end()) {
                return true; // Bomber collision
            }
        }
    }
    return false;
}

bool GameObject::move_dist(float distance, Direction dir) {
    if (flying || falling) {
        return false;
    }

    float move_x = 0;
    float move_y = 0;

    switch (dir) {
           case DIR_LEFT:  move_x = -distance; break;
           case DIR_RIGHT: move_x =  distance; break;
           case DIR_UP:    move_y = -distance; break;
           case DIR_DOWN:  move_y =  distance; break;
           default: return false;
       }
   
       float next_x = x + move_x;
       float next_y = y + move_y;
   
       if (!is_blocked(next_x, next_y)) {
           // Direct path is clear
           x = next_x;
           y = next_y;
           return true;
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
               return true;
           }
           if (d == 0) break;
       }
   
       // No partial move possible, now try to slide for cornering
       const float slide_amount = 1.0;
       if (dir == DIR_LEFT || dir == DIR_RIGHT) {
           // Try sliding vertically
           if (!is_blocked(next_x, y + slide_amount)) {
               y += slide_amount;
               x = next_x;
               return true;
           }
           if (!is_blocked(next_x, y - slide_amount)) {
               y -= slide_amount;
               x = next_x;
               return true;
           }
       } else { // Moving vertically
           // Try sliding horizontally
           if (!is_blocked(x + slide_amount, next_y)) {
               x += slide_amount;
               y = next_y;
               return true;
           }
           if (!is_blocked(x - slide_amount, next_y)) {
               x -= slide_amount;
               y = next_y;
               return true;
           }
       }
   
       return false; // Completely blocked
   }


bool GameObject::move(float deltaTime)
{
	if (!flying) {
		int span = (int)(deltaTime*speed);
		remainder += deltaTime*speed - (int)(deltaTime*speed);
		span += (int)(remainder);
		remainder -= (int)(remainder);

		for (int i=0; i<span; i++) {
			switch (cur_dir) {
				case DIR_LEFT:
					if (!move_left()) {
						stop();
						return false;
					}
					break;
				case DIR_RIGHT:
					if (!move_right()) {
						stop();
						return false;
					}
					break;
				case DIR_UP:
					if (!move_up()) {
						stop();
						return false;
					}
					break;
				case DIR_DOWN:
					if (!move_down()) {
						stop();
						return false;
					}
					break;
				default:
					break;
			}
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
	x += _x;
	y += _y;
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
    int tmp = (get_x()+20)/40;
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
	int tmp = (get_y()+20)/40;
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
void GameObject::set_bomb_on_tile(Bomb* bomb) const {
    int map_x = (int)(x+20)/40;
    int map_y = (int)(y+20)/40;
    
    // GAMECONTEXT ONLY: No fallback needed - all objects use GameContext
    GameContext* context = get_context();
    if (!context || !context->get_tile_manager()) {
        SDL_Log("ERROR: set_bomb_on_tile called with null context or tile_manager");
        return;
    }
    
    context->get_tile_manager()->register_bomb_at(map_x, map_y, bomb);
    SDL_Log("GameObject: Set bomb %p on tile at (%d,%d) through GameContext", bomb, map_x, map_y);
}

void GameObject::remove_bomb_from_tile(Bomb* bomb) const {
    int map_x = (int)(x+20)/40;
    int map_y = (int)(y+20)/40;
    
    // GAMECONTEXT ONLY: No fallback needed - all objects use GameContext
    GameContext* context = get_context();
    if (!context || !context->get_tile_manager()) {
        SDL_Log("ERROR: remove_bomb_from_tile called with null context or tile_manager");
        return;
    }
    
    context->get_tile_manager()->unregister_bomb_at(map_x, map_y, bomb);
    SDL_Log("GameObject: Removed bomb %p from tile at (%d,%d) through GameContext", bomb, map_x, map_y);
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
        PixelCoord position(static_cast<float>(get_x()), static_cast<float>(get_y()));
        
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

