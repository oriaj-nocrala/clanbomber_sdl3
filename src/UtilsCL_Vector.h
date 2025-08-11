/* This file is part of ClanBomber <http://www.nongnu.org/clanbomber>.
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

#ifndef UTILSCL_VECTOR_H
#define UTILSCL_VECTOR_H

class CL_Vector
{
 public:
  CL_Vector(float x = 0.0f, float y = 0.0f, float z = 0.0f)
  {
    this->x = x;
    this->y = y;
    this->z = z;
  }
  /*bool operator = (const CL_Vector &v)
  {
    this->x = v.x;
    this->y = v.y;
    this->z = v.z;
    }*/
  bool operator == (const CL_Vector &v)
  {
    if((x == v.x) && (y == v.y) && (z == v.z)) {
      return true;
    }
    else {
      return false;
    }
  }
  bool operator != (const CL_Vector &v)
  {
    return !operator==(v);
  }
  float x, y, z;
};

#endif
