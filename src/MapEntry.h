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

#ifndef MAPENTRY_H
#define MAPENTRY_H

#include <string>
#include <vector>
#include "UtilsCL_Vector.h"

#define MAP_WIDTH 20
#define MAP_HEIGHT 15

class MapEntry {
public:
    MapEntry(const std::string& filename);
    ~MapEntry();
    
    bool load();
    char get_data(int x, int y);
    std::string get_name() const { return name; }
    std::string get_author() const { return author; }
    int get_max_players() const { return max_players; }
    bool is_enabled() const { return enabled; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    
    CL_Vector get_bomber_pos(int nr);
    void read_bomber_positions();
    
private:
    std::string filename;
    std::string name;
    std::string author;
    int max_players;
    bool enabled;
    char map_data[MAP_WIDTH][MAP_HEIGHT];
    std::vector<CL_Vector> bomber_positions;
};

#endif