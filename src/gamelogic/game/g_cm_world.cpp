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

// g_cm_world.c -- world query functions

#include "g_local.h"
#include "g_cm_world.h"

typedef struct worldEntity_s
{
	struct worldSector_s *worldSector;
	struct worldEntity_s *nextEntityInWorldSector;
} worldEntity_t;

worldEntity_t wentities[ MAX_GENTITIES ];

worldEntity_t *G_CM_WorldEntityForGentity( gentity_t *gEnt )
{
	if ( !gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES )
	{
		Com_Error( ERR_DROP, "G_CM_SvEntityForGentity: bad gEnt" );
	}

	return &wentities[ gEnt->s.number ];
}

gentity_t *G_CM_GEntityForWorldEntity( worldEntity_t *ent )
{
	int num;

	num = ent - wentities;
	return &g_entities[ num ];
}

/*
=================
G_CM_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void G_CM_SetBrushModel( gentity_t *ent, const char *name )
{
	clipHandle_t h;
	vec3_t       mins, maxs;

	if ( !name )
	{
		Com_Error( ERR_DROP, "G_CM_SetBrushModel: NULL for #%i", ent->s.number );
	}

	if ( name[ 0 ] != '*' )
	{
		Com_Error( ERR_DROP, "G_CM_SetBrushModel: %s of #%i isn't a brush model", name, ent->s.number );
	}

	ent->s.modelindex = atoi( name + 1 );

	h = CM_InlineModel( ent->s.modelindex );
	CM_ModelBounds( h, mins, maxs );
	VectorCopy( mins, ent->r.mins );
	VectorCopy( maxs, ent->r.maxs );
	ent->r.bmodel = qtrue;

	ent->r.contents = -1; // we don't know exactly what is in the brushes
}

/*
================
G_CM_ClipHandleForEntity

Returns a headnode that can be used for testing or clipping to a
given entity.  If the entity is a bsp model, the headnode will
be returned, otherwise a custom box tree will be constructed.
================
*/
clipHandle_t G_CM_ClipHandleForEntity( const gentity_t *ent )
{
	if ( ent->r.bmodel )
	{
		// explicit hulls in the BSP model
		return CM_InlineModel( ent->s.modelindex );
	}

	if ( ent->r.svFlags & SVF_CAPSULE )
	{
		// create a temp capsule from bounding box sizes
		return CM_TempBoxModel( ent->r.mins, ent->r.maxs, qtrue );
	}

	// create a temp tree from bounding box sizes
	return CM_TempBoxModel( ent->r.mins, ent->r.maxs, qfalse );
}

/*
=================
G_CM_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean G_CM_inPVS( const vec3_t p1, const vec3_t p2 )
{
	int  leafnum;
	int  cluster;
	int  area1, area2;
	byte *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
	area1 = CM_LeafArea( leafnum );
	mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
	area2 = CM_LeafArea( leafnum );

	if ( mask && ( !( mask[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) ) )
	{
		return qfalse;
	}

	if ( !CM_AreasConnected( area1, area2 ) )
	{
		return qfalse; // a door blocks sight
	}

	return qtrue;
}

/*
=================
G_CM_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
qboolean G_CM_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 )
{
	int  leafnum;
	int  cluster;
//	int             area1, area2; //unused
	byte *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
//	area1 = CM_LeafArea(leafnum); //Doesn't modify anything.

	mask = CM_ClusterPVS( cluster );
	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
//	area2 = CM_LeafArea(leafnum); //Doesn't modify anything.

	if ( mask && ( !( mask[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) ) )
	{
		return qfalse;
	}

	return qtrue;
}

/*
========================
G_CM_AdjustAreaPortalState
========================
*/
void G_CM_AdjustAreaPortalState( gentity_t *ent, qboolean open )
{
	if ( ent->r.areanum2 == -1 )
	{
		return;
	}

	CM_AdjustAreaPortalState( ent->r.areanum, ent->r.areanum2, open );
}

/*
==================
G_CM_EntityContact
==================
*/
qboolean G_CM_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *gEnt, traceType_t type )
{
	const float  *origin, *angles;
	clipHandle_t ch;
	trace_t      trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = G_CM_ClipHandleForEntity( gEnt );
	CM_TransformedBoxTrace( &trace, vec3_origin, vec3_origin, mins, maxs, ch, -1, origin, angles, type );

	return trace.startsolid;
}
/*
===============================================================================

ENTITY CHECKING

To avoid linearly searching through lists of entities during environment testing,
the world is carved up with an evenly spaced, axially aligned bsp tree.  Entities
are kept in chains either at the final leafs, or at the first node that splits
them, which prevents having to deal with multiple fragments of a single entity.

===============================================================================
*/

typedef struct worldSector_s
{
	int                  axis; // -1 = leaf node
	float                dist;
	struct worldSector_s *children[ 2 ];

	worldEntity_t        *entities;
} worldSector_t;

#define AREA_DEPTH 4
#define AREA_NODES 64

worldSector_t sv_worldSectors[ AREA_NODES ];
int           sv_numworldSectors;

/*
===============
G_CM_SectorList_f
===============
*/
void G_CM_SectorList_f( void )
{
	int           i, c;
	worldSector_t *sec;
	worldEntity_t    *ent;

	for ( i = 0; i < AREA_NODES; i++ )
	{
		sec = &sv_worldSectors[ i ];

		c = 0;

		for ( ent = sec->entities; ent; ent = ent->nextEntityInWorldSector )
		{
			c++;
		}

		Com_Printf( "sector %i: %i entities\n", i, c );
	}
}

/*
===============
G_CM_CreateworldSector

Builds a uniformly subdivided tree for the given world size
===============
*/
worldSector_t  *G_CM_CreateworldSector( int depth, vec3_t mins, vec3_t maxs )
{
	worldSector_t *anode;
	vec3_t        size;
	vec3_t        mins1, maxs1, mins2, maxs2;

	anode = &sv_worldSectors[ sv_numworldSectors ];
	sv_numworldSectors++;

	if ( depth == AREA_DEPTH )
	{
		anode->axis = -1;
		anode->children[ 0 ] = anode->children[ 1 ] = NULL;
		return anode;
	}

	VectorSubtract( maxs, mins, size );

	if ( size[ 0 ] > size[ 1 ] )
	{
		anode->axis = 0;
	}
	else
	{
		anode->axis = 1;
	}

	anode->dist = 0.5 * ( maxs[ anode->axis ] + mins[ anode->axis ] );
	VectorCopy( mins, mins1 );
	VectorCopy( mins, mins2 );
	VectorCopy( maxs, maxs1 );
	VectorCopy( maxs, maxs2 );

	maxs1[ anode->axis ] = mins2[ anode->axis ] = anode->dist;

	anode->children[ 0 ] = G_CM_CreateworldSector( depth + 1, mins2, maxs2 );
	anode->children[ 1 ] = G_CM_CreateworldSector( depth + 1, mins1, maxs1 );

	return anode;
}

/*
===============
G_CM_ClearWorld

===============
*/
void G_CM_ClearWorld( void )
{
	clipHandle_t h;
	vec3_t       mins, maxs;

	memset( sv_worldSectors, 0, sizeof( sv_worldSectors ) );
	memset( wentities, 0, sizeof( wentities ) );
	sv_numworldSectors = 0;

	// get world map bounds
	h = CM_InlineModel( 0 );
	CM_ModelBounds( h, mins, maxs );
	G_CM_CreateworldSector( 0, mins, maxs );
}

/*
===============
G_CM_UnlinkEntity

===============
*/
void G_CM_UnlinkEntity( gentity_t *gEnt )
{
	worldEntity_t* scan;
	worldSector_t* ws;

	worldEntity_t* went = G_CM_WorldEntityForGentity( gEnt );

	gEnt->r.linked = qfalse;

	ws = went->worldSector;

	if ( !ws )
	{
		return; // not linked in anywhere
	}

	went->worldSector = NULL;

	if ( ws->entities == went )
	{
		ws->entities = went->nextEntityInWorldSector;
		return;
	}

	for ( scan = ws->entities; scan; scan = scan->nextEntityInWorldSector )
	{
		if ( scan->nextEntityInWorldSector == went )
		{
			scan->nextEntityInWorldSector = went->nextEntityInWorldSector;
			return;
		}
	}

	Com_Printf( "WARNING: G_CM_UnlinkEntity: not found in worldSector\n" );
}

/*
===============
G_CM_LinkEntity

===============
*/
#define MAX_TOTAL_ENT_LEAFS 128
void G_CM_LinkEntity( gentity_t *gEnt )
{
	worldSector_t *node;
	int           leafs[ MAX_TOTAL_ENT_LEAFS ];
	int           cluster;
	int           num_leafs;
	int           i, j, k;
	int           area;
	int           lastLeaf;
	float         *origin, *angles;

	worldEntity_t* went = G_CM_WorldEntityForGentity( gEnt );

	if ( went->worldSector )
	{
		G_CM_UnlinkEntity( gEnt );  // unlink from old position
	}

	// encode the size into the entityState_t for client prediction
	if ( gEnt->r.bmodel )
	{
		gEnt->s.solid = SOLID_BMODEL; // a solid_box will never create this value

		// Gordon: for the origin only bmodel checks
		gEnt->r.originCluster = CM_LeafCluster( CM_PointLeafnum( gEnt->r.currentOrigin ) );
	}
	else if ( gEnt->r.contents & ( CONTENTS_SOLID | CONTENTS_BODY ) )
	{
		// assume that x/y are equal and symetric
		i = gEnt->r.maxs[ 0 ];

		if ( i < 1 )
		{
			i = 1;
		}

		if ( i > 255 )
		{
			i = 255;
		}

		// z is not symetric
		j = ( -gEnt->r.mins[ 2 ] );

		if ( j < 1 )
		{
			j = 1;
		}

		if ( j > 255 )
		{
			j = 255;
		}

		// and z maxs can be negative...
		k = ( gEnt->r.maxs[ 2 ] + 32 );

		if ( k < 1 )
		{
			k = 1;
		}

		if ( k > 255 )
		{
			k = 255;
		}

		gEnt->s.solid = ( k << 16 ) | ( j << 8 ) | i;
	}
	else
	{
		gEnt->s.solid = 0;
	}

	// get the position
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	// set the abs box
	if ( gEnt->r.bmodel && ( angles[ 0 ] || angles[ 1 ] || angles[ 2 ] ) )
	{
		// expand for rotation
		float max;
		int   i;

		max = RadiusFromBounds( gEnt->r.mins, gEnt->r.maxs );

		for ( i = 0; i < 3; i++ )
		{
			gEnt->r.absmin[ i ] = origin[ i ] - max;
			gEnt->r.absmax[ i ] = origin[ i ] + max;
		}
	}
	else
	{
		VectorAdd( origin, gEnt->r.mins, gEnt->r.absmin );
		VectorAdd( origin, gEnt->r.maxs, gEnt->r.absmax );
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	gEnt->r.absmin[ 0 ] -= 1;
	gEnt->r.absmin[ 1 ] -= 1;
	gEnt->r.absmin[ 2 ] -= 1;
	gEnt->r.absmax[ 0 ] += 1;
	gEnt->r.absmax[ 1 ] += 1;
	gEnt->r.absmax[ 2 ] += 1;

	// link to PVS leafs
	gEnt->r.numClusters = 0;
	gEnt->r.lastCluster = 0;
	gEnt->r.areanum = -1;
	gEnt->r.areanum2 = -1;

	//get all leafs, including solids
	num_leafs = CM_BoxLeafnums( gEnt->r.absmin, gEnt->r.absmax, leafs, MAX_TOTAL_ENT_LEAFS, &lastLeaf );

	// if none of the leafs were inside the map, the
	// entity is outside the world and can be considered unlinked
	if ( !num_leafs )
	{
		return;
	}

	// set areas, even from clusters that don't fit in the entity array
	for ( i = 0; i < num_leafs; i++ )
	{
		area = CM_LeafArea( leafs[ i ] );

		if ( area != -1 )
		{
			// doors may legally straggle two areas,
			// but nothing should evern need more than that
			if ( gEnt->r.areanum != -1 && gEnt->r.areanum != area )
			{
				gEnt->r.areanum2 = area;
			}
			else
			{
				gEnt->r.areanum = area;
			}
		}
	}

	// store as many explicit clusters as we can
	gEnt->r.numClusters = 0;

	for ( i = 0; i < num_leafs; i++ )
	{
		cluster = CM_LeafCluster( leafs[ i ] );

		if ( cluster != -1 )
		{
			gEnt->r.clusternums[ gEnt->r.numClusters++ ] = cluster;

			if ( gEnt->r.numClusters == MAX_ENT_CLUSTERS )
			{
				break;
			}
		}
	}

	// store off a last cluster if we need to
	if ( i != num_leafs )
	{
		gEnt->r.lastCluster = CM_LeafCluster( lastLeaf );
	}

	gEnt->r.linkcount++;

	// find the first world sector node that the ent's box crosses
	node = sv_worldSectors;

	while ( 1 )
	{
		if ( node->axis == -1 )
		{
			break;
		}

		if ( gEnt->r.absmin[ node->axis ] > node->dist )
		{
			node = node->children[ 0 ];
		}
		else if ( gEnt->r.absmax[ node->axis ] < node->dist )
		{
			node = node->children[ 1 ];
		}
		else
		{
			break; // crosses the node
		}
	}

	// link it in
	went->worldSector = node;
	went->nextEntityInWorldSector = node->entities;
	node->entities = went;

	gEnt->r.linked = qtrue;
}

/*
============================================================================

AREA QUERY

Fills in a list of all entities whose absmin / absmax intersects the given
bounds.  This does NOT mean that they actually touch in the case of bmodels.
============================================================================
*/

typedef struct
{
	const float *mins;
	const float *maxs;
	int         *list;
	int         count, maxcount;
} areaParms_t;

/*
====================
G_CM_AreaEntities_r

====================
*/
void G_CM_AreaEntities_r( worldSector_t *node, areaParms_t *ap )
{
	worldEntity_t     *check, *next;
	gentity_t *gcheck;
//	int             count;

//	count = 0;

	for ( check = node->entities; check; check = next )
	{
		next = check->nextEntityInWorldSector;

		gcheck = G_CM_GEntityForWorldEntity( check );

		if ( !gcheck->r.linked )
		{
			continue;
		}

		if ( gcheck->r.absmin[ 0 ] > ap->maxs[ 0 ]
		     || gcheck->r.absmin[ 1 ] > ap->maxs[ 1 ]
		     || gcheck->r.absmin[ 2 ] > ap->maxs[ 2 ]
		     || gcheck->r.absmax[ 0 ] < ap->mins[ 0 ] || gcheck->r.absmax[ 1 ] < ap->mins[ 1 ] || gcheck->r.absmax[ 2 ] < ap->mins[ 2 ] )
		{
			continue;
		}

		if ( ap->count == ap->maxcount )
		{
			Com_Printf( "G_CM_AreaEntities: MAXCOUNT\n" );
			return;
		}

		ap->list[ ap->count ] = check - wentities;
		ap->count++;
	}

	if ( node->axis == -1 )
	{
		return; // terminal node
	}

	// recurse down both sides
	if ( ap->maxs[ node->axis ] > node->dist )
	{
		G_CM_AreaEntities_r( node->children[ 0 ], ap );
	}

	if ( ap->mins[ node->axis ] < node->dist )
	{
		G_CM_AreaEntities_r( node->children[ 1 ], ap );
	}
}

/*
================
G_CM_AreaEntities
================
*/
int G_CM_AreaEntities( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount )
{
	areaParms_t ap;

	ap.mins = mins;
	ap.maxs = maxs;
	ap.list = entityList;
	ap.count = 0;
	ap.maxcount = maxcount;

	G_CM_AreaEntities_r( sv_worldSectors, &ap );

	return ap.count;
}

//===========================================================================

typedef struct
{
	vec3_t      boxmins, boxmaxs; // enclose the test object along entire move
	const float *mins;
	const float *maxs; // size of the moving object
	const float *start;
	vec3_t      end;
	trace_t     trace;
	int         passEntityNum;
	int         contentmask;
	traceType_t collisionType;
} moveclip_t;

/*
====================
G_CM_ClipToEntity

====================
*/
void G_CM_ClipToEntity( trace_t *trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum,
                      int contentmask, traceType_t type )
{
	gentity_t *touch;
	clipHandle_t   clipHandle;
	float          *origin, *angles;

	touch = &g_entities[ entityNum ];

	memset( trace, 0, sizeof( trace_t ) );

	// if it doesn't have any brushes of a type we
	// are looking for, ignore it
	if ( !( contentmask & touch->r.contents ) )
	{
		trace->fraction = 1.0;
		return;
	}

	// might intersect, so do an exact clip
	clipHandle = G_CM_ClipHandleForEntity( touch );

	origin = touch->r.currentOrigin;
	angles = touch->r.currentAngles;

	if ( !touch->r.bmodel )
	{
		angles = vec3_origin; // boxes don't rotate
	}

	CM_TransformedBoxTrace( trace, ( float * ) start, ( float * ) end,
	                        ( float * ) mins, ( float * ) maxs, clipHandle, contentmask, origin, angles, type );

	if ( trace->fraction < 1 )
	{
		trace->entityNum = touch->s.number;
	}
}

// FIXME: Copied from cm_local.h
#define BOX_MODEL_HANDLE ( MAX_SUBMODELS + 1 )

/*
====================
G_CM_ClipMoveToEntities

====================
*/
void G_CM_ClipMoveToEntities( moveclip_t *clip )
{
	int            i, num;
	int            touchlist[ MAX_GENTITIES ];
	gentity_t *touch;
	int            passOwnerNum;
	trace_t        trace;
	clipHandle_t   clipHandle;
	float          *origin, *angles;

	num = G_CM_AreaEntities( clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES );

	if ( clip->passEntityNum != ENTITYNUM_NONE )
	{
		passOwnerNum = g_entities[ clip->passEntityNum ].r.ownerNum;

		if ( passOwnerNum == ENTITYNUM_NONE )
		{
			passOwnerNum = -1;
		}
	}
	else
	{
		passOwnerNum = -1;
	}

	for ( i = 0; i < num; i++ )
	{
		if ( clip->trace.allsolid )
		{
			return;
		}

		touch = &g_entities[ touchlist[ i ] ];

		// see if we should ignore this entity
		if ( clip->passEntityNum != ENTITYNUM_NONE )
		{
			if ( touchlist[ i ] == clip->passEntityNum )
			{
				continue; // don't clip against the pass entity
			}

			if ( touch->r.ownerNum == clip->passEntityNum )
			{
				continue; // don't clip against own missiles
			}

			if ( touch->r.ownerNum == passOwnerNum )
			{
				continue; // don't clip against other missiles from our owner
			}
		}

		// if it doesn't have any brushes of a type we
		// are looking for, ignore it
		if ( !( clip->contentmask & touch->r.contents ) )
		{
			continue;
		}

		// might intersect, so do an exact clip
		clipHandle = G_CM_ClipHandleForEntity( touch );

		origin = touch->r.currentOrigin;
		angles = touch->r.currentAngles;

		if ( !touch->r.bmodel )
		{
			angles = vec3_origin; // boxes don't rotate
		}

		CM_TransformedBoxTrace( &trace, clip->start, clip->end,
		                        clip->mins, clip->maxs, clipHandle, clip->contentmask, origin, angles, clip->collisionType );

		if ( trace.allsolid )
		{
			clip->trace.allsolid = qtrue;
			trace.entityNum = touch->s.number;
		}
		else if ( trace.startsolid )
		{
			clip->trace.startsolid = qtrue;
			trace.entityNum = touch->s.number;
		}

		if ( trace.fraction < clip->trace.fraction )
		{
			qboolean oldStart;

			// make sure we keep a startsolid from a previous trace
			oldStart = clip->trace.startsolid;

			trace.entityNum = touch->s.number;
			clip->trace = trace;
			clip->trace.startsolid |= oldStart;
		}
	}
}

/*
==================
G_CM_Trace

Moves the given mins/maxs volume through the world from start to end.
passEntityNum and entities owned by passEntityNum are explicitly not checked.
==================
*/
void G_CM_Trace( trace_t *results, const vec3_t start, const vec3_t mins2, const vec3_t maxs2, const vec3_t end, int passEntityNum,
               int contentmask, traceType_t type )
{
	moveclip_t clip;
	int        i;

	if ( !mins2 )
	{
		mins2 = vec3_origin;
	}

	if ( !maxs2 )
	{
		maxs2 = vec3_origin;
	}

    vec3_t mins, maxs;
    VectorCopy(mins2, mins);
    VectorCopy(maxs2, maxs);

	if ( passEntityNum == -1 )
		passEntityNum = ENTITYNUM_NONE;

	memset( &clip, 0, sizeof( moveclip_t ) );

	// clip to world
	CM_BoxTrace( &clip.trace, start, end, mins, maxs, 0, contentmask, type );
	clip.trace.entityNum = clip.trace.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;

	if ( clip.trace.fraction == 0 || passEntityNum == -2 )
	{
		*results = clip.trace;
		return; // blocked immediately by the world
	}

	clip.contentmask = contentmask;
	clip.start = start;
//  VectorCopy( clip.trace.endpos, clip.end );
	VectorCopy( end, clip.end );
	clip.mins = mins;
	clip.maxs = maxs;
	clip.passEntityNum = passEntityNum;
	clip.collisionType = type;

	// create the bounding box of the entire move
	// we can limit it to the part of the move not
	// already clipped off by the world, which can be
	// a significant savings for line of sight and shot traces
	for ( i = 0; i < 3; i++ )
	{
		if ( end[ i ] > start[ i ] )
		{
			clip.boxmins[ i ] = clip.start[ i ] + clip.mins[ i ] - 1;
			clip.boxmaxs[ i ] = clip.end[ i ] + clip.maxs[ i ] + 1;
		}
		else
		{
			clip.boxmins[ i ] = clip.end[ i ] + clip.mins[ i ] - 1;
			clip.boxmaxs[ i ] = clip.start[ i ] + clip.maxs[ i ] + 1;
		}
	}

	// clip to other solid entities
	G_CM_ClipMoveToEntities( &clip );

	*results = clip.trace;
}

/*
=============
G_CM_PointContents
=============
*/
int G_CM_PointContents( const vec3_t p, int passEntityNum )
{
	int            touch[ MAX_GENTITIES ];
	gentity_t *hit;
	int            i, num;
	int            contents, c2;
	clipHandle_t   clipHandle;
//	float          *angles;

	// get base contents from world
	contents = CM_PointContents( p, 0 );

	// or in contents from all the other entities
	num = G_CM_AreaEntities( p, p, touch, MAX_GENTITIES );

	for ( i = 0; i < num; i++ )
	{
		if ( touch[ i ] == passEntityNum )
		{
			continue;
		}

		hit = &g_entities[ touch[ i ] ];
		// might intersect, so do an exact clip
		clipHandle = G_CM_ClipHandleForEntity( hit );

		// ydnar: non-worldspawn entities must not use world as clip model!
		if ( clipHandle == 0 )
		{
			continue;
		}

//		angles = hit->r.currentAngles;

		/*
		                if(!hit->r.bmodel)
		                {
		                        angles = vec3_origin; // boxes don't rotate
		                }
		*/

		c2 = CM_TransformedPointContents( p, clipHandle, hit->r.currentOrigin, hit->r.currentAngles );

		contents |= c2;
	}

	return contents;
}
