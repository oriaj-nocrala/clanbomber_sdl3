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

#include <iostream>

#include <filesystem>
#include <fstream>

#include "ClanBomber.h"
#include "GameConfig.h"
#include "Resources.h"

#include "Controller.h"

#include <stdio.h>

std::filesystem::path GameConfig::filename = "clanbomber.cfg";
std::filesystem::path GameConfig::path     = "";

std::string GameConfig::last_server= "intruder";

int	GameConfig::round_time			= 90;
int GameConfig::sound_enabled		= true;
int	GameConfig::max_skateboards		= 5;
int	GameConfig::max_power			= 12;
int	GameConfig::max_bombs			= 9;

int	GameConfig::start_skateboards	= 0;
int	GameConfig::start_power			= 1;
int	GameConfig::start_bombs			= 1;
int	GameConfig::start_kick			= false;
int	GameConfig::start_glove			= false;

int	GameConfig::skateboards			= true;
int	GameConfig::power				= true;
int	GameConfig::bombs				= true;
int	GameConfig::kick				= true;
int	GameConfig::glove				= true;

int	GameConfig::joint				= true;
int	GameConfig::viagra				= true;
int	GameConfig::koks				= true;

int	GameConfig::start_map			= 0;
int GameConfig::random_map_order	= false;

int	GameConfig::music				= false;

int	GameConfig::kids_mode			= false;
int	GameConfig::shaky_explosions	= true;
int	GameConfig::highlight_maptiles	= true;
int	GameConfig::random_positions	= true;
int	GameConfig::corpse_parts		= 10;

int	GameConfig::bomb_countdown		= 3000;
int	GameConfig::bomb_delay			= 10;
int	GameConfig::bomb_speed			= 160;

int	GameConfig::points_to_win		= 5;
int	GameConfig::theme				= 0;

bool GameConfig::fullscreen = false;

BomberConfig GameConfig::bomber[8];

BomberConfig::BomberConfig()
{
	local_client = true;
	server_bomber = false;
	client_index = -1;
	config_index = -1;
	client_ip = NULL;
	enabled = true;
	team = 0;
	skin = 0;
	controller = 0;
	highlight_maptile = true;
	name = "Fischlustig";
}

BomberConfig::~BomberConfig()
{
}

void BomberConfig::set_name(std::string _name)
{
	if (_name.length()) {
		name = _name;
	}
}

std::string BomberConfig::get_name()
{
	return name;
}

void BomberConfig::set_skin(int _skin)
{
	skin = _skin%NR_BOMBERSKINS;
}

int BomberConfig::get_skin()
{
	return skin;
}

void BomberConfig::set_team(int _team)
{
	team = _team%5;
}

int BomberConfig::get_team()
{
	return team;
}

void BomberConfig::set_controller(int _controller)
{
  controller = _controller % (6 + 8);
}

int BomberConfig::get_controller()
{
	return controller;
}

void BomberConfig::enable()
{
	enabled = true;
}

void BomberConfig::disable()
{
	enabled = false;
}

void BomberConfig::set_highlight_maptile(bool _highlight_maptile)
{
	highlight_maptile = _highlight_maptile;
}

int BomberConfig::get_highlight_maptile()
{
	return highlight_maptile;
}

int BomberConfig::is_enabled()
{
	return enabled;
}

void BomberConfig::set_enabled(bool _enabled)
{
	enabled = _enabled;
}

void BomberConfig::set_client_index(int index)
{
	client_index = index;
}

int BomberConfig::get_client_index()
{
	return client_index;
}

int BomberConfig::get_config_index()
{
	return config_index;
}

void BomberConfig::set_config_index(int index)
{
	config_index=index;
}

char* BomberConfig::get_client_ip()
{
    return client_ip;
}

void BomberConfig::set_client_ip(char* ip_string)
{
    client_ip = ip_string;
}

int GameConfig::get_number_of_players()
{
	int nr = 0;
	
	for (int i=0; i<8; i++) {
		if (bomber[i].is_enabled()) {
			nr++;
		}
	}
	
	return nr;
}

int GameConfig::get_number_of_opponents()
{
	int nrs = 0;
	int team_cunt[4] = {0,0,0,0};
	
	for (int i=0; i<8; i++) {
		if (bomber[i].is_enabled()) {
			if (bomber[i].get_team() == 0) {
				nrs++;
			}
			else {
				if (team_cunt[bomber[i].get_team()-1] == 0) {
					nrs++;
				}
				team_cunt[bomber[i].get_team()-1]++;
			}
		}
	}
	
	return nrs;
}

void GameConfig::set_round_time( int _round_time)
{
	round_time = _round_time;
}

void GameConfig::set_random_map_order( int _random_map_order)
{
	random_map_order = _random_map_order;
}

void GameConfig::set_max_skateboards(int _max_skateboards)
{
	max_skateboards = _max_skateboards;
}

void GameConfig::set_max_power(int _max_power)
{
	max_power = _max_power;
}

void GameConfig::set_max_bombs(int _max_bombs)
{
	max_bombs = _max_bombs;
}

void GameConfig::set_start_skateboards(int _start_skateboards)
{
	start_skateboards = _start_skateboards;
}

void GameConfig::set_start_power(int _start_power)
{
	start_power = _start_power;
}

void GameConfig::set_start_bombs(int _start_bombs)
{
	start_bombs = _start_bombs;
}

void GameConfig::set_start_kick( int _start_kick)
{
	start_kick = _start_kick;
}

void GameConfig::set_start_glove( int _start_glove)
{
	start_glove = _start_glove;
}

void GameConfig::set_skateboards(int _skateboards)
{
	skateboards = _skateboards;
}

void GameConfig::set_power(int _power)
{
	power = _power;
}

void GameConfig::set_bombs(int _bombs)
{
	bombs = _bombs;
}

void GameConfig::set_kick( int _kick)
{
	kick = _kick;
}

void GameConfig::set_glove( int _glove)
{
	glove = _glove;
}

void GameConfig::set_joint( int _joint)
{
	joint = _joint;
}

void GameConfig::set_viagra( int _viagra)
{
	viagra = _viagra;
}

void GameConfig::set_koks( int _koks)
{
	koks = _koks;
}

void GameConfig::set_start_map(int _start_map)
{
	start_map = _start_map;
}

void GameConfig::set_points_to_win(int _points_to_win)
{
	points_to_win = _points_to_win;
}

void GameConfig::set_theme(int _theme)
{
	theme = _theme;
}

void GameConfig::set_filename(std::filesystem::path _filename)
{
	filename = _filename;
}

void GameConfig::set_path(std::filesystem::path _path)
{
	path = _path;
}

void GameConfig::set_music( int _music)
{
	music = _music;
}

void GameConfig::set_kids_mode( int _kids_mode)
{
	kids_mode = _kids_mode;
}

void GameConfig::set_corpse_parts( int _corpse_parts)
{
	corpse_parts = _corpse_parts;
}

void GameConfig::set_shaky_explosions(int _shaky_explosions)
{
	shaky_explosions = _shaky_explosions;
}

void GameConfig::set_highlight_maptiles(int _highlight_maptiles)
{
	highlight_maptiles =  _highlight_maptiles;
}

void GameConfig::set_random_positions(int _random_positions)
{
	random_positions = _random_positions;
}


void GameConfig::set_bomb_countdown(int _bomb_countdown)
{
	bomb_countdown = _bomb_countdown;
}

void GameConfig::set_bomb_delay(int _bomb_delay)
{
	bomb_delay = _bomb_delay;
}

void GameConfig::set_bomb_speed(int _bomb_speed)
{
	bomb_speed = _bomb_speed;
}

int GameConfig::get_max_skateboards()
{
	return max_skateboards;
}

int GameConfig::get_max_power()
{
	return max_power;
}

int GameConfig::get_max_bombs()
{
	return max_bombs;
}

int GameConfig::get_start_skateboards()
{
	return start_skateboards;
}

int GameConfig::get_start_power()
{
	return start_power;
}

int GameConfig::get_start_bombs()
{
	return start_bombs;
}

int GameConfig::get_start_kick()
{
	return start_kick;
}

int GameConfig::get_start_glove()
{
	return start_glove;
}

int GameConfig::get_skateboards()
{
	return skateboards;
}

int GameConfig::get_power()
{
	return power;
}

int GameConfig::get_bombs()
{
	return bombs;
}

int GameConfig::get_kick()
{
	return kick;
}

int GameConfig::get_glove()
{
	return glove;
}

int GameConfig::get_joint()
{
	return joint;
}

int GameConfig::get_viagra()
{
	return viagra;
}

int GameConfig::get_koks()
{
	return koks;
}

int GameConfig::get_start_map()
{
	return start_map;
}

int GameConfig::get_random_map_order()
{
	return random_map_order;
}

int GameConfig::get_round_time()
{
	return round_time;
}

int GameConfig::get_points_to_win()
{
	return points_to_win;
}

int GameConfig::get_theme()
{
	return theme;
}

int GameConfig::get_sound_enabled()
{
	return sound_enabled;
}

int GameConfig::get_music()
{
	return music;
}

int GameConfig::get_kids_mode()
{
	return kids_mode;
}

int GameConfig::get_corpse_parts()
{
	return corpse_parts;
}

int GameConfig::get_shaky_explosions()
{
	return shaky_explosions;
}

int GameConfig::get_highlight_maptiles()
{
	return highlight_maptiles;
}


int GameConfig::get_random_positions()
{
	return random_positions;
}

int GameConfig::get_bomb_countdown()
{
	return bomb_countdown;
}

int GameConfig::get_bomb_delay()
{
	return bomb_delay;
}

int GameConfig::get_bomb_speed()
{
	return bomb_speed;
}

bool GameConfig::get_fullscreen()
{
  return fullscreen;
}

void GameConfig::set_fullscreen(bool val)
{
  fullscreen = val;
}

bool GameConfig::save(bool init)
{
	if (init) {
		for (int i=0; i<8; i++) {
			bomber[i].set_skin(i);
		}
		bomber[0].set_name( "Are" );
		bomber[1].set_name( "You" );
		bomber[2].set_name( "Still" );
		bomber[3].set_name( "Watching" );
		bomber[4].set_name( "AIs" );
		bomber[5].set_name( "Playing" );
		bomber[6].set_name( "For" );
		bomber[7].set_name( "You" );
	}

	std::ofstream configfile(path / filename);

	configfile << CURRENT_CONFIGFILE_VERSION << std::endl; // version

	configfile << max_bombs			<< std::endl;
	configfile << max_power			<< std::endl;
	configfile << max_skateboards		<< std::endl;
	
	configfile << start_bombs		<< std::endl;
	configfile << start_power		<< std::endl;
	configfile << start_skateboards		<< std::endl;
	configfile << start_kick		<< std::endl;
	configfile << start_glove		<< std::endl;
	
	configfile << start_map			<< std::endl;
	configfile << points_to_win		<< std::endl;
	configfile << round_time		<< std::endl;
	configfile << theme			<< std::endl;
	configfile << music			<< std::endl;

	configfile << kids_mode			<< std::endl;
	configfile << corpse_parts		<< std::endl;
	configfile << shaky_explosions		<< std::endl;
	configfile << random_positions		<< std::endl;
	configfile << random_map_order		<< std::endl;
	
	configfile << bombs			<< std::endl;
	configfile << power			<< std::endl;
	configfile << skateboards		<< std::endl;
	configfile << kick			<< std::endl;
	configfile << glove			<< std::endl;
	
	configfile << joint			<< std::endl;
	configfile << viagra			<< std::endl;
	configfile << koks			<< std::endl;

	configfile << bomb_countdown		<< std::endl;
	configfile << bomb_delay		<< std::endl;
	configfile << bomb_speed		<< std::endl;
	
	for (int i=0; i<8; i++) {
		configfile << bomber[i].get_skin()		<< std::endl;
		configfile << bomber[i].get_name()              << std::endl;
		configfile << bomber[i].get_team()		<< std::endl;
		configfile << bomber[i].get_controller()	<< std::endl;
		configfile << bomber[i].is_enabled()		<< std::endl;
		configfile << bomber[i].get_highlight_maptile()	<< std::endl;
	}

	configfile << last_server << std::endl;

	return true;
}

bool GameConfig::load()
{
  std::ifstream configfile(path / filename);

  if (configfile.fail()) {
    configfile.close();
    save(true);
    return false;
  }

  int version;
  configfile >> version;

  if (version != CURRENT_CONFIGFILE_VERSION) {
    configfile.close();
    save(true);
    return false;
  }

  configfile >> max_bombs;
  configfile >> max_power;
  configfile >> max_skateboards;

  configfile >> start_bombs;
  configfile >> start_power;
  configfile >> start_skateboards;
  configfile >> start_kick;
  configfile >> start_glove;
  configfile >> start_map;

  configfile >> points_to_win;
  configfile >> round_time;
  configfile >> theme;
  configfile >> music;

  configfile >> kids_mode;
  configfile >> corpse_parts;
  configfile >> shaky_explosions;
  configfile >> random_positions;
  configfile >> random_map_order;

  configfile >> bombs;
  configfile >> power;
  configfile >> skateboards;
  configfile >> kick;
  configfile >> glove;

  configfile >> joint;
  configfile >> viagra;
  configfile >> koks;

  configfile >> bomb_countdown;
  configfile >> bomb_delay;
  configfile >> bomb_speed;


  for (int i=0; i<8; i++) {
    configfile >> version;
    bomber[i].set_skin(version);

    std::string player_name;
    configfile.ignore(1, '\n');
    getline(configfile, player_name);
    bomber[i].set_name(player_name);

    configfile >> version;
    bomber[i].set_team(version);

    configfile >> version;
    bomber[i].set_controller(version);

    configfile >> version;
    bomber[i].set_enabled(version != 0);

    configfile >> version;
    bomber[i].set_highlight_maptile(version != 0);
  }

  std::string server_name;
  configfile >> server_name;
  set_last_server(server_name);

  return true;
}

void GameConfig::set_last_server(std::string server_name)
{
    last_server = server_name;
}
 
std::string GameConfig::get_last_server()
{
    return last_server;
}


void BomberConfig::set_local(bool _local)
{
	local_client=_local;
}


bool BomberConfig::is_local()
{
	return local_client;
}

bool BomberConfig::is_server_bomber()
{
	return server_bomber;
}

void BomberConfig::set_server_bomber(bool from_server)
{
	server_bomber = from_server;
}
