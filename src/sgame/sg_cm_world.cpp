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

// sg_cm_world.c -- world query functions

#include "sg_local.h"
#include "sg_cm_world.h"

#include "common/cm/cm_public.h"

struct worldSector_t;
struct worldEntity_t
{
	worldSector_t *worldSector;
	worldEntity_t *nextEntityInWorldSector;
};

worldEntity_t wentities[ MAX_GENTITIES ];

static worldEntity_t *G_CM_WorldEntityForGentity( gentity_t *gEnt )
{
	if ( !gEnt || gEnt->num() < 0 || gEnt->num() >= MAX_GENTITIES )
	{
		Sys::Drop( "G_CM_WorldEntityForGentity: bad gEnt" );
	}

	return &wentities[ gEnt->num() ];
}

/*
=================
G_CM_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void G_CM_SetBrushModel( gentity_t *ent, std::string const& name )
{
	if ( name.empty() )
	{
		Sys::Drop( "G_CM_SetBrushModel: NULL for #%i", ent->num() );
	}
	if ( name.size() > 1 && name[ 0 ] != '*' )
	{
		Sys::Drop( "G_CM_SetBrushModel: %s of #%i isn't a brush model", name.c_str(), ent->num() );
	}

	char* end;
	unsigned long val = strtoul( name.c_str() + 1, &end, 10 );
	if ( val >= INT_MAX )
	{
		Sys::Drop( "G_CM_SetBrushModel: invalid brush model \"%s\"", name.c_str() );
	}
	ent->s.modelindex = static_cast<int>( val );

	clipHandle_t h = CM_InlineModel( ent->s.modelindex );
	CM_ModelBounds( h, ent->r.mins, ent->r.maxs );
	ent->r.bmodel = true;

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
static clipHandle_t G_CM_ClipHandleForEntity( const gentity_t *ent )
{
	if ( ent->r.bmodel )
	{
		// explicit hulls in the BSP model
		return CM_InlineModel( ent->s.modelindex );
	}

	// create a temp tree/capsule from bounding box sizes
	return CM_TempBoxModel( ent->r.mins, ent->r.maxs, /*capsule = */ ent->r.svFlags & SVF_CAPSULE );
}

/*
=================
G_CM_inPVS

Also checks portalareas so that doors block sight
Return false if a door blocks sight
=================
*/
bool G_CM_inPVS( glm::vec3 const& p1, glm::vec3 const& p2 )
{
	int leafnum = CM_PointLeafnum( &p1[0] );
	int cluster = CM_LeafCluster( leafnum );
	int area1 = CM_LeafArea( leafnum );
	byte* mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( &p2[0] );
	cluster = CM_LeafCluster( leafnum );
	int area2 = CM_LeafArea( leafnum );

	if ( mask && ( !( mask[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) ) )
	{
		return false;
	}
	return CM_AreasConnected( area1, area2 );
}

/*
=================
G_CM_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
bool G_CM_inPVSIgnorePortals( glm::vec3 const& p1, glm::vec3 const& p2 )
{
//	int             area1, area2; //unused

	int leafnum = CM_PointLeafnum( &p1[0] );
	int cluster = CM_LeafCluster( leafnum );
//	area1 = CM_LeafArea(leafnum); //Doesn't modify anything.

	byte* mask = CM_ClusterPVS( cluster );
	leafnum = CM_PointLeafnum( &p2[0] );
	cluster = CM_LeafCluster( leafnum );
//	area2 = CM_LeafArea(leafnum); //Doesn't modify anything.

	return mask == nullptr || ( mask[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) );
}

/*
========================
G_CM_AdjustAreaPortalState
========================
*/
void G_CM_AdjustAreaPortalState( gentity_t *ent, bool open )
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
bool G_CM_EntityContact( glm::vec3 const& mins, glm::vec3 const& maxs, const gentity_t *gEnt, traceType_t type )
{
	// check for exact collision
	const float* origin = gEnt->r.currentOrigin;
	const float* angles = gEnt->r.currentAngles;

	clipHandle_t ch = G_CM_ClipHandleForEntity( gEnt );
	trace_t trace;
	CM_TransformedBoxTrace( &trace, vec3_origin, vec3_origin, &mins[0], &maxs[0], ch, MASK_ALL, 0, origin, angles, type );

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

struct worldSector_t
{
	int                  axis; // -1 = leaf node
	float                dist;
	worldSector_t        *children[ 2 ];

	worldEntity_t        *entities;
};

#define AREA_DEPTH 4
#define AREA_NODES 64

worldSector_t sv_worldSectors[ AREA_NODES ];
int           sv_numworldSectors;

/*
===============
G_CM_SectorList_f
===============
*/
void G_CM_SectorList_f()
{
	for ( int i = 0; i < AREA_NODES; i++ )
	{
		worldSector_t* sec = &sv_worldSectors[ i ];

		int c = 0;
		for ( worldEntity_t* ent = sec->entities; ent; ent = ent->nextEntityInWorldSector )
		{
			c++;
		}

		Log::Notice( "sector %i: %i entities", i, c );
	}
}

/*
===============
G_CM_CreateworldSector

Builds a uniformly subdivided tree for the given world size
===============
*/
static worldSector_t *G_CM_CreateworldSector( int depth, glm::vec3 const& mins, glm::vec3 const& maxs )
{
	worldSector_t *anode = &sv_worldSectors[ sv_numworldSectors ];

	sv_numworldSectors++;

	if ( depth == AREA_DEPTH )
	{
		anode->axis = -1;
		anode->children[ 0 ] = anode->children[ 1 ] = nullptr;
		return anode;
	}

	glm::vec3 size = maxs - mins;

	if ( size[ 0 ] > size[ 1 ] )
	{
		anode->axis = 0;
	}
	else
	{
		anode->axis = 1;
	}

	anode->dist = 0.5 * ( maxs[ anode->axis ] + mins[ anode->axis ] );
	glm::vec3 mins1 = mins;
	glm::vec3 mins2 = mins;
	glm::vec3 maxs1 = maxs;
	glm::vec3 maxs2 = maxs;

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
void G_CM_ClearWorld()
{
	memset( sv_worldSectors, 0, sizeof( sv_worldSectors ) );
	memset( wentities, 0, sizeof( wentities ) );
	sv_numworldSectors = 0;

	// get world map bounds
	clipHandle_t h = CM_InlineModel( 0 );
	glm::vec3 mins, maxs;
	CM_ModelBounds( h, &mins[0], &maxs[0] );
	G_CM_CreateworldSector( 0, mins, maxs );
}

/*
===============
G_CM_UnlinkEntity

===============
*/
void G_CM_UnlinkEntity( gentity_t *gEnt )
{
	worldEntity_t* went = G_CM_WorldEntityForGentity( gEnt );

	gEnt->r.linked = false;

	worldSector_t* ws = went->worldSector;

	if ( !ws )
	{
		return; // not linked in anywhere
	}

	went->worldSector = nullptr;

	if ( ws->entities == went )
	{
		ws->entities = went->nextEntityInWorldSector;
		return;
	}

	for ( worldEntity_t* scan = ws->entities; scan; scan = scan->nextEntityInWorldSector )
	{
		if ( scan->nextEntityInWorldSector == went )
		{
			scan->nextEntityInWorldSector = went->nextEntityInWorldSector;
			return;
		}
	}

	Log::Warn( "G_CM_UnlinkEntity: not found in worldSector" );
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
		int i = Math::Clamp( gEnt->r.maxs[ 0 ], 1.0f, 255.0f );

		// z is not symetric
		int j = Math::Clamp( -gEnt->r.mins[ 2 ], 1.0f, 255.0f );

		// and z maxs can be negative...
		int k = Math::Clamp( gEnt->r.maxs[ 2 ] + 32.0f, 1.0f, 255.0f );

		gEnt->s.solid = ( k << 16 ) | ( j << 8 ) | i;
	}
	else
	{
		gEnt->s.solid = 0;
	}

	// get the position
	glm::vec3 origin = VEC2GLM( gEnt->r.currentOrigin );
	glm::vec3 angles = VEC2GLM( gEnt->r.currentAngles );

	// set the abs box
	if ( gEnt->r.bmodel && ( angles[ 0 ] || angles[ 1 ] || angles[ 2 ] ) )
	{
		glm::mat3 matrix = RotationMatrix( angles );

		const glm::vec3 corners[ 8 ] = {
			{ gEnt->r.mins[ 0 ], gEnt->r.mins[ 1 ], gEnt->r.mins[ 2 ] },
			{ gEnt->r.mins[ 0 ], gEnt->r.mins[ 1 ], gEnt->r.maxs[ 2 ] },
			{ gEnt->r.mins[ 0 ], gEnt->r.maxs[ 1 ], gEnt->r.mins[ 2 ] },
			{ gEnt->r.mins[ 0 ], gEnt->r.maxs[ 1 ], gEnt->r.maxs[ 2 ] },
			{ gEnt->r.maxs[ 0 ], gEnt->r.mins[ 1 ], gEnt->r.mins[ 2 ] },
			{ gEnt->r.maxs[ 0 ], gEnt->r.mins[ 1 ], gEnt->r.maxs[ 2 ] },
			{ gEnt->r.maxs[ 0 ], gEnt->r.maxs[ 1 ], gEnt->r.mins[ 2 ] },
			{ gEnt->r.maxs[ 0 ], gEnt->r.maxs[ 1 ], gEnt->r.maxs[ 2 ] },
		};

		glm::vec3 mins = matrix * corners[ 0 ];
		glm::vec3 maxs = mins;

		for ( int i = 1; i < 8; i++ )
		{
			glm::vec3 point = matrix * corners[ i ];
			for ( int j = 0; j < 3; j++ )
			{
				mins[ j ] = std::min( mins[ j ], point[ j ] );
				maxs[ j ] = std::max( maxs[ j ], point[ j ] );
			}
		}

		glm::vec3 absmin = mins + origin;
		glm::vec3 absmax = maxs + origin;

		VectorCopy(absmin, gEnt->r.absmin);
		VectorCopy(absmax, gEnt->r.absmax);
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
	int lastLeaf;
	int num_leafs = CM_BoxLeafnums( gEnt->r.absmin, gEnt->r.absmax, leafs, MAX_TOTAL_ENT_LEAFS, &lastLeaf );

	// if none of the leafs were inside the map, the
	// entity is outside the world and can be considered unlinked
	if ( !num_leafs )
	{
		return;
	}

	// set areas, even from clusters that don't fit in the entity array
	for ( int i = 0; i < num_leafs; i++ )
	{
		int area = CM_LeafArea( leafs[ i ] );

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

	for ( int i = 0; i < num_leafs; i++ )
	{
		int cluster = CM_LeafCluster( leafs[ i ] );

		if ( cluster != -1 )
		{
			gEnt->r.clusternums[ gEnt->r.numClusters++ ] = cluster;

			if ( gEnt->r.numClusters == MAX_ENT_CLUSTERS )
			{
				gEnt->r.lastCluster = CM_LeafCluster( lastLeaf );
				break;
			}
		}
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

	gEnt->r.linked = true;
}

/*
============================================================================

AREA QUERY

Fills in a list of all entities whose absmin / absmax intersects the given
bounds.  This does NOT mean that they actually touch in the case of bmodels.
============================================================================
*/

struct areaParms_t
{
	const float *mins;
	const float *maxs;
	int         *list;
	int         count, maxcount;
};

/*
====================
G_CM_AreaEntities_r

====================
*/
static void G_CM_AreaEntities_r( worldSector_t *node, areaParms_t *ap )
{
	worldEntity_t *next;
	for ( worldEntity_t* check = node->entities; check; check = next )
	{
		next = check->nextEntityInWorldSector;
		gentity_t *gcheck = &g_entities[ check - wentities ];

		if ( !gcheck->r.linked )
		{
			continue;
		}

		if ( gcheck->r.absmin[ 0 ] > ap->maxs[ 0 ]
		     || gcheck->r.absmin[ 1 ] > ap->maxs[ 1 ]
		     || gcheck->r.absmin[ 2 ] > ap->maxs[ 2 ]
		     || gcheck->r.absmax[ 0 ] < ap->mins[ 0 ]
		     || gcheck->r.absmax[ 1 ] < ap->mins[ 1 ]
		     || gcheck->r.absmax[ 2 ] < ap->mins[ 2 ] )
		{
			continue;
		}

		if ( ap->count == ap->maxcount )
		{
			Log::Notice( "G_CM_AreaEntities: MAXCOUNT" );
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
int G_CM_AreaEntities( glm::vec3 const& mins, glm::vec3 const& maxs, int *entityList, int maxcount )
{
	areaParms_t ap;

	ap.mins = &mins[0];
	ap.maxs = &maxs[0];
	ap.list = entityList;
	ap.count = 0;
	ap.maxcount = maxcount;

	G_CM_AreaEntities_r( sv_worldSectors, &ap );

	return ap.count;
}

//===========================================================================

struct moveclip_t
{
	glm::vec3   boxmins, boxmaxs; // enclose the test object along entire move
	const float *mins;
	const float *maxs; // size of the moving object
	const float *start;
	const float *end;
	trace_t     trace;
	int         passEntityNum;
	int         contentmask;
	int         skipmask;
	traceType_t collisionType;
};

// FIXME: Copied from cm_local.h
#define BOX_MODEL_HANDLE ( MAX_SUBMODELS + 1 )

/*
====================
G_CM_ClipMoveToEntities
====================
*/
static void G_CM_ClipMoveToEntities( moveclip_t& clip )
{
	int            touchlist[ MAX_GENTITIES ];
	int            passOwnerNum;
	trace_t        trace;
	clipHandle_t   clipHandle;

	int num = G_CM_AreaEntities( clip.boxmins, clip.boxmaxs, touchlist, MAX_GENTITIES );

	if ( clip.passEntityNum != ENTITYNUM_NONE )
	{
		passOwnerNum = g_entities[ clip.passEntityNum ].r.ownerNum;

		if ( passOwnerNum == ENTITYNUM_NONE )
		{
			passOwnerNum = -1;
		}
	}
	else
	{
		passOwnerNum = -1;
	}

	for ( int i = 0; i < num; i++ )
	{
		if ( clip.trace.allsolid )
		{
			return;
		}

		gentity_t *touch = &g_entities[ touchlist[ i ] ];

		// see if we should ignore this entity
		if ( clip.passEntityNum != ENTITYNUM_NONE )
		{
			if ( touchlist[ i ] == clip.passEntityNum )
			{
				continue; // don't clip against the pass entity
			}

			if ( touch->r.ownerNum == clip.passEntityNum )
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
		if ( !( clip.contentmask & touch->r.contents ) )
		{
			continue;
		}

		if ( clip.skipmask & touch->r.contents )
		{
			continue;
		}

		// might intersect, so do an exact clip
		clipHandle = G_CM_ClipHandleForEntity( touch );

		const float *origin = touch->r.currentOrigin;
		const float *angles = touch->r.currentAngles;

		if ( !touch->r.bmodel )
		{
			angles = vec3_origin; // boxes don't rotate
		}

		CM_TransformedBoxTrace( &trace, &clip.start[0], &clip.end[0], &clip.mins[0], &clip.maxs[0], clipHandle,
		                        clip.contentmask, 0, origin, angles, clip.collisionType );

		if ( trace.allsolid )
		{
			clip.trace.allsolid = true;
			trace.entityNum = touch->num();
		}
		else if ( trace.startsolid )
		{
			clip.trace.startsolid = true;
			trace.entityNum = touch->num();
		}

		if ( trace.fraction < clip.trace.fraction )
		{
			bool oldStart;

			// make sure we keep a startsolid from a previous trace
			oldStart = clip.trace.startsolid;

			trace.entityNum = touch->num();
			clip.trace = trace;
			clip.trace.startsolid |= oldStart;
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
trace_t G_CM_Trace( glm::vec3 const& start, glm::vec3 const& mins, glm::vec3 const& maxs,
                 glm::vec3 const& end, int passEntityNum, int contentmask, int skipmask,
                 traceType_t type )
{
	// clip to world
	// -------------

	trace_t trace;
	CM_BoxTrace( &trace, &start[0], &end[0], &mins[0], &maxs[0], 0, contentmask, skipmask, type );

	trace.entityNum = ENTITYNUM_WORLD;
	if ( trace.fraction == 0.f )
	{
		return trace;
	}
	if ( trace.fraction == 1.f )
	{
		trace.entityNum = ENTITYNUM_NONE;
	}

	moveclip_t clip;
	clip.trace = trace;
	// clip to entities
	// ----------------

	clip.contentmask = contentmask;
	clip.skipmask = skipmask;
	clip.start = &start[0];
	clip.end = &end[0];
	clip.mins = &mins[0];
	clip.maxs = &maxs[0];
	clip.passEntityNum = passEntityNum;
	clip.collisionType = type;

	// create the bounding box of the entire move
	// we can limit it to the part of the move not
	// already clipped off by the world, which can be
	// a significant savings for line of sight and shot traces
	for ( int i = 0; i < 3; i++ )
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
	G_CM_ClipMoveToEntities( clip );
	return clip.trace;
}

void G_CM_Trace( trace_t *results, glm::vec3 const& start, glm::vec3 const& mins2, glm::vec3 const& maxs2,
                 glm::vec3 const& end, int passEntityNum, int contentmask, int skipmask,
                 traceType_t type )
{
	moveclip_t clip;

	memset( &clip, 0, sizeof( moveclip_t ) );

	// clip to world
	// -------------

	CM_BoxTrace( &clip.trace, &start[0], &end[0], &mins2[0], &maxs2[0], 0, contentmask, skipmask, type );
	clip.trace.entityNum = clip.trace.fraction == 1.0 ? ENTITYNUM_NONE : ENTITYNUM_WORLD;

	if ( clip.trace.fraction == 0.f )
	{
		*results = clip.trace;
		return; // blocked immediately by the world
	}

	// clip to entities
	// ----------------

	clip.contentmask = contentmask;
	clip.skipmask = skipmask;
	clip.start = &start[0];
	clip.end = &end[0]; //FIXME: useless copy, but just refactoring for now
	clip.mins = &mins2[0];
	clip.maxs = &maxs2[0];
	clip.passEntityNum = passEntityNum;
	clip.collisionType = type;

	// create the bounding box of the entire move
	// we can limit it to the part of the move not
	// already clipped off by the world, which can be
	// a significant savings for line of sight and shot traces
	for ( int i = 0; i < 3; i++ )
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
	G_CM_ClipMoveToEntities( clip );

	*results = clip.trace;
}

/*
=============
G_CM_PointContents
=============
*/
int G_CM_PointContents( glm::vec3 const& p, int passEntityNum )
{
	int            touch[ MAX_GENTITIES ];
//	float          *angles;

	// get base contents from world
	int contents = CM_PointContents( &p[0], 0 );

	// or in contents from all the other entities
	int num = G_CM_AreaEntities( p, p, touch, MAX_GENTITIES );

	for ( int i = 0; i < num; i++ )
	{
		if ( touch[ i ] == passEntityNum )
		{
			continue;
		}

		gentity_t* hit = &g_entities[ touch[ i ] ];
		// might intersect, so do an exact clip
		clipHandle_t clipHandle = G_CM_ClipHandleForEntity( hit );

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

		int c2 = CM_TransformedPointContents( &p[0], clipHandle, hit->r.currentOrigin, hit->r.currentAngles );

		contents |= c2;
	}

	return contents;
}
