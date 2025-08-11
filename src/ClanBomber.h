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

#ifndef CLANBOMBER_H
#define CLANBOMBER_H

#include <list>
#include <filesystem>

// Forward declarations
class Map;
class GameObject;
class Bomber;
class Server;
class Client;
class Mutex;
class Event;
class ClientSetup;
class ServerSetup;
class Chat;
class Menu;
class Observer;


enum Direction
{
  DIR_NONE  = -1,
  DIR_DOWN  = 0,
  DIR_LEFT  = 1,
  DIR_UP    = 2,
  DIR_RIGHT = 3
};

/**
 * @author Andreas Hundt
 * @author Denis Oliver Kropp
 */
class ClanBomberApplication
{
 public:
  Map* map;
  std::list<GameObject*> objects;
  std::list<Bomber*> bomber_objects;

  static bool is_server() { return false; }
  static bool is_client() { return false; }

  static Server* get_server() { return nullptr; }

  static unsigned short get_next_object_id() { return 1; }
};

#endif
