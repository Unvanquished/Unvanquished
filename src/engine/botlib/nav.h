/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2009 Peter McNeill <n27@bigpond.net.au>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// nav.h -- navigation mesh definitions
#ifndef __NAV_H
#define __NAV_H

// should be the same as in rest of engine
const short STEPSIZE = 18;

const short MAX_PATH_POLYS = 512;
const short MAX_CORRIDOR_CORNERS = 5;

typedef struct
{
  unsigned short *verts;
  unsigned short *polys;
  unsigned char *areas;
  unsigned short *flags;
  unsigned int *dMeshes;
  float *dVerts;
  unsigned char *dTris;
} NavMeshData_t;

typedef struct
{
    int version;
    int numVerts;
    int numPolys;
    int numVertsPerPoly;
    float mins[3];
    float maxs[3];
    int dNumMeshes;
    int dNumVerts;
    int dNumTris;
    float cellSize;
    float cellHeight;
    NavMeshData_t data;
} NavMeshHeader_t;

enum navPolyAreas
{
	POLYAREA_GROUND = 0x01,
	POLYAREA_WATER = 0x02,
	POLYAREA_DOOR = 0x04,
	POLYAREA_JUMPPAD = 0x08,
	POLYAREA_TELEPORTER = 0x10
};

enum navPolyFlags
{
	POLYFLAGS_WALK = 0x01, // Ability to walk (ground, grass, road)
	POLYFLAGS_SWIM = 0x02, // Ability to swim (water).
	POLYFLAGS_DOOR = 0x04, // Ability to move through doors.
	POLYFLAGS_JUMP = 0x08, // Ability to jump.
	POLYFLAGS_POUNCE = 0x10, //Ability to pounce
	POLYFLAGS_WALLWALK = 0x20, //Ability to wallwalk
	POLYFLAGS_LADDER = 0x40, //Ability to climb ladders
	POLYFLAGS_ALL = 0xffff // All abilities.
};
#endif

