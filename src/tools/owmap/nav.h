/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2011 Adrian Fuhrmann <aliasng@gmail.com>

This file is part of Legacy source code.

Legacy source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Legacy source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Legacy source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// nav.h -- navigation mesh definitions

#ifndef __NAV_H__
#define __NAV_H__
#endif


// should be the same as in rest of engine

#define	STEPSIZE		18.f
#include "../qcommon/q_shared.h"
#include "../../libs/recast/Recast.h"


typedef struct
{

    int version;
    int vertsOffset;
    int numVerts;
    int polysOffset;
    int areasOffset;
    int flagsOffset;
    int numPolys;
    int numVertsPerPoly;
    vec3_t mins;
    vec3_t maxs;

    int dMeshesOffset;
    int dNumMeshes;
    int dVertsOffset;
    int dNumVerts;
    int dTrisOffset;
    int dNumTris;

    float cellSize;
    float cellHeight;

    int filesize;
} navMeshDataInfo_t;
class rcContext context(false);

rcHeightfield *heightField;
unsigned char *navData;
int navDataSize;
vec3_t mapmins;
vec3_t mapmaxs;


unsigned char *triareas;
rcContourSet *contours;
rcCompactHeightfield *compHeightField;
rcPolyMesh *polyMesh;
rcPolyMeshDetail *detailedPolyMesh;

// Load indices
const int *indexes;
const bspDrawSurface_t *surfaces;
int numSurfaces;

// Load triangles
int *tris;
int numtris;

// Load vertices
const bspDrawVert_t *vertices;
float *verts;
int numverts;

// Load models
const bspModel_t *models;

// Recast config
rcConfig cfg;

// Navigation info
navMeshDataInfo_t navInfo;

enum SamplePolyAreas
{
SAMPLE_POLYAREA_GROUND,
SAMPLE_POLYAREA_WATER,
SAMPLE_POLYAREA_ROAD,
SAMPLE_POLYAREA_DOOR,
SAMPLE_POLYAREA_GRASS,
SAMPLE_POLYAREA_JUMP,
};
enum SamplePolyFlags
{
SAMPLE_POLYFLAGS_WALK = 0x01, // Ability to walk (ground, grass, road)
SAMPLE_POLYFLAGS_SWIM = 0x02, // Ability to swim (water).
SAMPLE_POLYFLAGS_DOOR = 0x04, // Ability to move through doors.
SAMPLE_POLYFLAGS_JUMP = 0x08, // Ability to jump.
SAMPLE_POLYFLAGS_DISABLED = 0x10, // Disabled polygon
SAMPLE_POLYFLAGS_ALL = 0xffff // All abilities.
};

