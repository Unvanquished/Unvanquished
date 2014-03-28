/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// g_cm_world.h -- world query functions

#ifndef G_CM_WORLD_H_
#define G_CM_WORLD_H_

#include "../../common/cm/cm_public.h"

void G_CM_ClearWorld( void );

// called after the world model has been loaded, before linking any entities

void G_CM_UnlinkEntity( gentity_t *ent );

// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void G_CM_LinkEntity( gentity_t *ent );

// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->r.absmin and ent->r.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid

clipHandle_t G_CM_ClipHandleForEntity( const sharedEntity_t *ent );

void         G_CM_SectorList_f( void );

int          G_CM_AreaEntities( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount );

// fills in a table of entity numbers with entities that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// The world entity is never returned in this list.

int G_CM_PointContents( const vec3_t p, int passEntityNum );

// returns the CONTENTS_* value from the world and all entities at the given point.

void G_CM_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum,
               int contentmask, traceType_t type );

// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passEntityNum, if isn't ENTITYNUM_NONE, will be explicitly excluded from clipping checks

void G_CM_ClipToEntity( trace_t *trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, traceType_t type );

qboolean G_CM_inPVS( const vec3_t p1, const vec3_t p2 );

qboolean G_CM_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );

void G_CM_AdjustAreaPortalState( gentity_t *ent, qboolean open );

qboolean G_CM_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *gEnt, traceType_t type );

void G_CM_SetBrushModel( gentity_t *ent, const char *name );

#endif // G_CM_WORLD_H_
