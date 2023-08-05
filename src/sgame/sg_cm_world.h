/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// sg_cm_world.h -- world query functions

#ifndef SG_CM_WORLD_H_
#define SG_CM_WORLD_H_

#include "common/cm/cm_public.h"

void G_CM_ClearWorld();

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

void         G_CM_SectorList_f();

int          G_CM_AreaEntities( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount );

// fills in a table of entity numbers with entities that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// The world entity is never returned in this list.

int G_CM_PointContents( const vec3_t p, int passEntityNum );

// returns the CONTENTS_* value from the world and all entities at the given point.

void G_CM_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
                 const vec3_t end, int passEntityNum, int contentmask, int skipmask,
                 traceType_t type );

// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passEntityNum, if isn't ENTITYNUM_NONE, will be explicitly excluded from clipping checks


// G_Trace2: an alternative to trap_Trace (a.k.a. G_CM_Trace) with different startsolid semantics
// In a standard trace, if there is a brush/entity/facet that overlaps the starting point but not
// the ending point, it makes the startsolid flag get set but is otherwise ignored.
// With G_Trace2, a thing overlapping the starting point is counted as a collision and makes the
// trace terminate at the starting point.
// More of the missing fields from trace_t such as `contents` could be added to
// trace2_t if there is a need.
//
// * If there is no collision:
//   - startsolid = false
//   - fraction = 1
//   - plane and surfaceFlags are not valid
//   - entityNum = ENTITYNUM_NONE
// * If there is a collision at the starting point:
//   - startsolid = true
//   - fraction = 0
//   - plane and surfaceFlags are not valid
//   - entityNum is a normal entity or ENTITYNUM_WORLD
// * If there is a collision after the starting point:
//   - startsolid = false
//   - 0 <= fraction < 1
//   - plane and surfaceFlags are valid
//   - entityNum is a normal entity or ENTITYNUM_WORLD
struct trace2_t
{
	bool startsolid; // if true, the initial point was in a solid area (counts as a collision)
	float fraction; // portion of line traversed 0-1. 1.0 = didn't hit anything
	vec3_t endpos; // final position
	struct {
		vec3_t normal; // surface normal at impact, transformed to world space
	} plane; // valid if !startsolid && fraction < 1
	int surfaceFlags; // valid if !startsolid && fraction < 1
	int entityNum; // entity the contacted surface is a part of, or ENTITYNUM_NONE
};
trace2_t G_Trace2( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
		int passEntityNum, int contentmask, int skipmask, traceType_t type = traceType_t::TT_AABB );

bool G_CM_inPVS( const vec3_t p1, const vec3_t p2 );

bool G_CM_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );

void G_CM_AdjustAreaPortalState( gentity_t *ent, bool open );

bool G_CM_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *gEnt, traceType_t type );

void G_CM_SetBrushModel( gentity_t *ent, const char *name );

#endif // SG_CM_WORLD_H_
