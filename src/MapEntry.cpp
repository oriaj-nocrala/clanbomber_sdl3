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

#include "MapEntry.h"
#include <fstream>
#include <filesystem>
#include <SDL3/SDL.h>

MapEntry::MapEntry(const std::string& _filename) : filename(_filename) {
    enabled = true;
    max_players = 8;
    
    // Extract map name from filename
    std::filesystem::path path(filename);
    name = path.stem().string();
    author = "Unknown";
    
    // Initialize map data
    for (int x = 0; x < MAP_WIDTH; x++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            map_data[x][y] = ' ';
        }
    }
}

MapEntry::~MapEntry() {
}

bool MapEntry::load() {
    std::ifstream file(filename);
    if (!file.is_open()) {
        SDL_Log("Failed to open map file: %s", filename.c_str());
        return false;
    }
    
    std::string line;
    int line_num = 0;
    
    // Read author (first line)
    if (std::getline(file, line)) {
        author = line;
        line_num++;
    }
    
    // Read max players (second line)  
    if (std::getline(file, line)) {
        max_players = std::stoi(line);
        line_num++;
    }
    
    // Read map data
    int y = 0;
    while (std::getline(file, line) && y < MAP_HEIGHT) {
        for (int x = 0; x < MAP_WIDTH && x < (int)line.length(); x++) {
            map_data[x][y] = line[x];
        }
        y++;
        line_num++;
    }
    
    file.close();
    
    // Read bomber positions
    read_bomber_positions();
    
    SDL_Log("Loaded map: %s by %s (max %d players)", name.c_str(), author.c_str(), max_players);
    return true;
}

char MapEntry::get_data(int x, int y) {
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
        return map_data[x][y];
    }
    return '*'; // Wall by default for out of bounds
}

void MapEntry::read_bomber_positions() {
    bomber_positions.clear();
    
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char tile = map_data[x][y];
            // Check for numbered bomber positions (0-7)
            if (tile >= '0' && tile <= '7') {
                bomber_positions.push_back(CL_Vector(x, y));
            }
        }
    }
    
    // If no specific positions found, use default positions
    if (bomber_positions.empty()) {
        bomber_positions.push_back(CL_Vector(2, 2));   // Top-left
        bomber_positions.push_back(CL_Vector(17, 2));  // Top-right  
        bomber_positions.push_back(CL_Vector(2, 12));  // Bottom-left
        bomber_positions.push_back(CL_Vector(17, 12)); // Bottom-right
        bomber_positions.push_back(CL_Vector(9, 2));   // Top-center
        bomber_positions.push_back(CL_Vector(9, 12));  // Bottom-center
        bomber_positions.push_back(CL_Vector(2, 7));   // Left-center
        bomber_positions.push_back(CL_Vector(17, 7));  // Right-center
    }
}

CL_Vector MapEntry::get_bomber_pos(int nr) {
    if (nr >= 0 && nr < (int)bomber_positions.size()) {
        return bomber_positions[nr];
    }
    return CL_Vector(2, 2); // Default position
}