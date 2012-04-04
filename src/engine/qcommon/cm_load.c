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

// cmodel.c -- model loading

#include "cm_local.h"

#ifdef BSPC

#include "../bspc/l_qfiles.h"

void SetPlaneSignbits( cplane_t *out )
{
	int bits, j;

	// for fast box on planeside test
	bits = 0;

	for ( j = 0; j < 3; j++ )
	{
		if ( out->normal[ j ] < 0 )
		{
			bits |= 1 << j;
		}
	}

	out->signbits = bits;
}

#endif //BSPC

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define BOX_LEAF_BRUSHES 1 // ydnar
#define BOX_BRUSHES      1
#define BOX_SIDES        6
#define BOX_LEAFS        2
#define BOX_PLANES       12

#define LL( x ) x = LittleLong( x )

clipMap_t cm;
int       c_pointcontents;
int       c_traces, c_brush_traces, c_patch_traces, c_trisoup_traces;

byte      *cmod_base;

#ifndef BSPC
cvar_t    *cm_noAreas;
cvar_t    *cm_noCurves;
cvar_t    *cm_forceTriangles;
cvar_t    *cm_playerCurveClip;
cvar_t    *cm_optimize;
#endif

cmodel_t  box_model;
cplane_t  *box_planes;
cbrush_t  *box_brush;

void      CM_InitBoxHull( void );
void      CM_FloodAreaConnections( void );

/*
===============================================================================

                                        MAP LOADING

===============================================================================
*/

/*
=================
CMod_LoadShaders
=================
*/
void CMod_LoadShaders( lump_t *l )
{
	dshader_t *in, *out;
	int       i, count;

	in = ( void * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Com_Error( ERR_DROP, "CMod_LoadShaders: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	if ( count < 1 )
	{
		Com_Error( ERR_DROP, "Map with no shaders" );
	}

	cm.shaders = Hunk_Alloc( count * sizeof( *cm.shaders ), h_high );
	cm.numShaders = count;

	Com_Memcpy( cm.shaders, in, count * sizeof( *cm.shaders ) );

	if ( LittleLong( 1 ) != 1 )
	{
		out = cm.shaders;

		for ( i = 0; i < count; i++, in++, out++ )
		{
			out->contentFlags = LittleLong( out->contentFlags );
			out->surfaceFlags = LittleLong( out->surfaceFlags );
		}
	}
}

/*
=================
CMod_LoadSubmodels
=================
*/
void CMod_LoadSubmodels( lump_t *l )
{
	dmodel_t *in;
	cmodel_t *out;
	int      i, j, count;
	int      *indexes;

	in = ( void * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Com_Error( ERR_DROP, "CMod_LoadSubmodels: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	if ( count < 1 )
	{
		Com_Error( ERR_DROP, "Map with no models" );
	}

	cm.cmodels = Hunk_Alloc( count * sizeof( *cm.cmodels ), h_high );
	cm.numSubModels = count;

	if ( count > MAX_SUBMODELS )
	{
		Com_Error( ERR_DROP, "MAX_SUBMODELS exceeded" );
	}

	for ( i = 0; i < count; i++, in++, out++ )
	{
		out = &cm.cmodels[ i ];

		for ( j = 0; j < 3; j++ )
		{
			// spread the mins / maxs by a pixel
			out->mins[ j ] = LittleFloat( in->mins[ j ] ) - 1;
			out->maxs[ j ] = LittleFloat( in->maxs[ j ] ) + 1;
		}

		if ( i == 0 )
		{
			continue; // world model doesn't need other info
		}

#ifdef USE_PHYSICS
		CMod_PhysicsAddBSPModel( i, in->firstSurface, in->numSurfaces );
#endif

		// make a "leaf" just to hold the model's brushes and surfaces
		out->leaf.numLeafBrushes = LittleLong( in->numBrushes );
		indexes = Hunk_Alloc( out->leaf.numLeafBrushes * 4, h_high );
		out->leaf.firstLeafBrush = indexes - cm.leafbrushes;

		for ( j = 0; j < out->leaf.numLeafBrushes; j++ )
		{
			indexes[ j ] = LittleLong( in->firstBrush ) + j;
		}

		out->leaf.numLeafSurfaces = LittleLong( in->numSurfaces );
		indexes = Hunk_Alloc( out->leaf.numLeafSurfaces * 4, h_high );
		out->leaf.firstLeafSurface = indexes - cm.leafsurfaces;

		for ( j = 0; j < out->leaf.numLeafSurfaces; j++ )
		{
			indexes[ j ] = LittleLong( in->firstSurface ) + j;
		}
	}
}

/*
=================
CMod_LoadNodes

=================
*/
void CMod_LoadNodes( lump_t *l )
{
	dnode_t *in;
	int     child;
	cNode_t *out;
	int     i, j, count;

	in = ( void * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	if ( count < 1 )
	{
		Com_Error( ERR_DROP, "Map has no nodes" );
	}

	cm.nodes = Hunk_Alloc( count * sizeof( *cm.nodes ), h_high );
	cm.numNodes = count;

	out = cm.nodes;

	for ( i = 0; i < count; i++, out++, in++ )
	{
		out->plane = cm.planes + LittleLong( in->planeNum );

		for ( j = 0; j < 2; j++ )
		{
			child = LittleLong( in->children[ j ] );
			out->children[ j ] = child;
		}
	}
}

/*
=================
CM_BoundBrush

=================
*/
void CM_BoundBrush( cbrush_t *b )
{
	b->bounds[ 0 ][ 0 ] = -b->sides[ 0 ].plane->dist;
	b->bounds[ 1 ][ 0 ] = b->sides[ 1 ].plane->dist;

	b->bounds[ 0 ][ 1 ] = -b->sides[ 2 ].plane->dist;
	b->bounds[ 1 ][ 1 ] = b->sides[ 3 ].plane->dist;

	b->bounds[ 0 ][ 2 ] = -b->sides[ 4 ].plane->dist;
	b->bounds[ 1 ][ 2 ] = b->sides[ 5 ].plane->dist;
}

/*
=================
CMod_LoadBrushes

=================
*/
void CMod_LoadBrushes( lump_t *l )
{
	dbrush_t *in;
	cbrush_t *out;
	int      i, count;

	in = ( void * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	cm.brushes = Hunk_Alloc( ( BOX_BRUSHES + count ) * sizeof( *cm.brushes ), h_high );
	cm.numBrushes = count;

	out = cm.brushes;

	for ( i = 0; i < count; i++, out++, in++ )
	{
		out->sides = cm.brushsides + LittleLong( in->firstSide );
		out->numsides = LittleLong( in->numSides );

		out->shaderNum = LittleLong( in->shaderNum );

		if ( out->shaderNum < 0 || out->shaderNum >= cm.numShaders )
		{
			Com_Error( ERR_DROP, "CMod_LoadBrushes: bad shaderNum: %i", out->shaderNum );
		}

		out->contents = cm.shaders[ out->shaderNum ].contentFlags;

		CM_BoundBrush( out );
	}
}

/*
=================
CMod_LoadLeafs
=================
*/
void CMod_LoadLeafs( lump_t *l )
{
	int     i;
	cLeaf_t *out;
	dleaf_t *in;
	int     count;

	in = ( void * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	if ( count < 1 )
	{
		Com_Error( ERR_DROP, "Map with no leafs" );
	}

	cm.leafs = Hunk_Alloc( ( BOX_LEAFS + count ) * sizeof( *cm.leafs ), h_high );
	cm.numLeafs = count;

	out = cm.leafs;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		out->cluster = LittleLong( in->cluster );
		out->area = LittleLong( in->area );
		out->firstLeafBrush = LittleLong( in->firstLeafBrush );
		out->numLeafBrushes = LittleLong( in->numLeafBrushes );
		out->firstLeafSurface = LittleLong( in->firstLeafSurface );
		out->numLeafSurfaces = LittleLong( in->numLeafSurfaces );

		if ( out->cluster >= cm.numClusters )
		{
			cm.numClusters = out->cluster + 1;
		}

		if ( out->area >= cm.numAreas )
		{
			cm.numAreas = out->area + 1;
		}
	}

	cm.areas = Hunk_Alloc( cm.numAreas * sizeof( *cm.areas ), h_high );
	cm.areaPortals = Hunk_Alloc( cm.numAreas * cm.numAreas * sizeof( *cm.areaPortals ), h_high );
}

/*
=================
CMod_LoadPlanes
=================
*/
void CMod_LoadPlanes( lump_t *l )
{
	int      i, j;
	cplane_t *out;
	dplane_t *in;
	int      count;
	int      bits;

	in = ( void * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	if ( count < 1 )
	{
		Com_Error( ERR_DROP, "Map with no planes" );
	}

	cm.planes = Hunk_Alloc( ( BOX_PLANES + count ) * sizeof( *cm.planes ), h_high );
	cm.numPlanes = count;

	out = cm.planes;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		bits = 0;

		for ( j = 0; j < 3; j++ )
		{
			out->normal[ j ] = LittleFloat( in->normal[ j ] );

			if ( out->normal[ j ] < 0 )
			{
				bits |= 1 << j;
			}
		}

		out->dist = LittleFloat( in->dist );
		out->type = PlaneTypeForNormal( out->normal );
		out->signbits = bits;
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
void CMod_LoadLeafBrushes( lump_t *l )
{
	int i;
	int *out;
	int *in;
	int count;

	in = ( void * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	// ydnar: more than <count> brushes are stored in leafbrushes...
	cm.leafbrushes = Hunk_Alloc( ( BOX_LEAF_BRUSHES + count ) * sizeof( *cm.leafbrushes ), h_high );
	cm.numLeafBrushes = count;

	out = cm.leafbrushes;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		*out = LittleLong( *in );
	}
}

/*
=================
CMod_LoadLeafSurfaces
=================
*/
void CMod_LoadLeafSurfaces( lump_t *l )
{
	int i;
	int *out;
	int *in;
	int count;

	in = ( void * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	cm.leafsurfaces = Hunk_Alloc( count * sizeof( *cm.leafsurfaces ), h_high );
	cm.numLeafSurfaces = count;

	out = cm.leafsurfaces;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		*out = LittleLong( *in );
	}
}

/*
=================
CMod_LoadBrushSides
=================
*/
void CMod_LoadBrushSides( lump_t *l )
{
	int          i;
	cbrushside_t *out;
	dbrushside_t *in;
	int          count;
	int          num;

	in = ( void * )( cmod_base + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		Com_Error( ERR_DROP, "MOD_LoadBmodel: funny lump size" );
	}

	count = l->filelen / sizeof( *in );

	cm.brushsides = Hunk_Alloc( ( BOX_SIDES + count ) * sizeof( *cm.brushsides ), h_high );
	cm.numBrushSides = count;

	out = cm.brushsides;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		num = LittleLong( in->planeNum );
		out->plane = &cm.planes[ num ];
		out->shaderNum = LittleLong( in->shaderNum );

		if ( out->shaderNum < 0 || out->shaderNum >= cm.numShaders )
		{
			Com_Error( ERR_DROP, "CMod_LoadBrushSides: bad shaderNum: %i", out->shaderNum );
		}

		out->surfaceFlags = cm.shaders[ out->shaderNum ].surfaceFlags;
	}
}

#define CM_EDGE_VERTEX_EPSILON 0.1f

/*
=================
CMod_BrushEdgesAreTheSame
=================
*/
static qboolean CMod_BrushEdgesAreTheSame( const vec3_t p0, const vec3_t p1, const vec3_t q0, const vec3_t q1 )
{
	if ( VectorCompareEpsilon( p0, q0, CM_EDGE_VERTEX_EPSILON ) && VectorCompareEpsilon( p1, q1, CM_EDGE_VERTEX_EPSILON ) )
	{
		return qtrue;
	}

	if ( VectorCompareEpsilon( p1, q0, CM_EDGE_VERTEX_EPSILON ) && VectorCompareEpsilon( p0, q1, CM_EDGE_VERTEX_EPSILON ) )
	{
		return qtrue;
	}

	return qfalse;
}

/*
=================
CMod_AddEdgeToBrush
=================
*/
static qboolean CMod_AddEdgeToBrush( const vec3_t p0, const vec3_t p1, cbrushedge_t *edges, int *numEdges )
{
	int i;

	if ( !edges || !numEdges )
	{
		return qfalse;
	}

	for ( i = 0; i < *numEdges; i++ )
	{
		if ( CMod_BrushEdgesAreTheSame( p0, p1, edges[ i ].p0, edges[ i ].p1 ) )
		{
			return qfalse;
		}
	}

	VectorCopy( p0, edges[ *numEdges ].p0 );
	VectorCopy( p1, edges[ *numEdges ].p1 );
	( *numEdges ) ++;

	return qtrue;
}

/*
=================
CMod_CreateBrushSideWindings
=================
*/
static void CMod_CreateBrushSideWindings( void )
{
	int          i, j, k;
	winding_t    *w;
	cbrushside_t *side, *chopSide;
	cplane_t     *plane;
	cbrush_t     *brush;
	cbrushedge_t *tempEdges;
	int          numEdges;
	int          edgesAlloc;
	int          totalEdgesAlloc = 0;
	int          totalEdges = 0;

	for ( i = 0; i < cm.numBrushes; i++ )
	{
		brush = &cm.brushes[ i ];
		numEdges = 0;

		// walk the list of brush sides
		for ( j = 0; j < brush->numsides; j++ )
		{
			// get side and plane
			side = &brush->sides[ j ];
			plane = side->plane;

			w = BaseWindingForPlane( plane->normal, plane->dist );

			// walk the list of brush sides
			for ( k = 0; k < brush->numsides && w != NULL; k++ )
			{
				chopSide = &brush->sides[ k ];

				if ( chopSide == side )
				{
					continue;
				}

				if ( chopSide->planeNum == ( side->planeNum ^ 1 ) )
				{
					continue; // back side clipaway
				}

				plane = &cm.planes[ chopSide->planeNum ^ 1 ];
				ChopWindingInPlace( &w, plane->normal, plane->dist, 0 );
			}

			if ( w )
			{
				numEdges += w->numpoints;
			}

			// set side winding
			side->winding = w;
		}

		// Allocate a temporary buffer of the maximal size
		tempEdges = ( cbrushedge_t * ) Z_Malloc( sizeof( cbrushedge_t ) * numEdges );
		brush->numEdges = 0;

		// compose the points into edges
		for ( j = 0; j < brush->numsides; j++ )
		{
			side = &brush->sides[ j ];

			if ( side->winding )
			{
				for ( k = 0; k < side->winding->numpoints - 1; k++ )
				{
					if ( brush->numEdges == numEdges )
					{
						Com_Error( ERR_FATAL, "Insufficient memory allocated for collision map edges" );
					}

					CMod_AddEdgeToBrush( side->winding->p[ k ], side->winding->p[ k + 1 ], tempEdges, &brush->numEdges );
				}

				FreeWinding( side->winding );
				side->winding = NULL;
			}
		}

		// Allocate a buffer of the actual size
		edgesAlloc = sizeof( cbrushedge_t ) * brush->numEdges;
		totalEdgesAlloc += edgesAlloc;
		brush->edges = ( cbrushedge_t * ) Hunk_Alloc( edgesAlloc, h_low );

		// Copy temporary buffer to permanent buffer
		Com_Memcpy( brush->edges, tempEdges, edgesAlloc );

		// Free temporary buffer
		Z_Free( tempEdges );

		totalEdges += brush->numEdges;
	}

	Com_DPrintf( "Allocated %d bytes for %d collision map edges...\n", totalEdgesAlloc, totalEdges );
}

/*
=================
CMod_LoadEntityString
=================
*/
void CMod_LoadEntityString( lump_t *l )
{
	char *p, *token;
	char keyname[ MAX_TOKEN_CHARS ];
	char value[ MAX_TOKEN_CHARS ];

	cm.entityString = Hunk_Alloc( l->filelen, h_high );
	cm.numEntityChars = l->filelen;
	Com_Memcpy( cm.entityString, cmod_base + l->fileofs, l->filelen );

	p = cm.entityString;

	// only parse the world spawn
	while ( 1 )
	{
		// parse key
		token = COM_ParseExt2( &p, qtrue );

		if ( !*token )
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: unexpected end of entities string while parsing worldspawn\n" );
			break;
		}

		if ( *token == '{' )
		{
			continue;
		}

		if ( *token == '}' )
		{
			break;
		}

		Q_strncpyz( keyname, token, sizeof( keyname ) );

		// parse value
		token = COM_ParseExt2( &p, qfalse );

		if ( !*token )
		{
			continue;
		}

		Q_strncpyz( value, token, sizeof( value ) );

		// check for per-poly collision support
		if ( !Q_stricmp( keyname, "perPolyCollision" ) && !Q_stricmp( value, "1" ) )
		{
			Com_Printf( "map features per poly collision detection\n" );
			cm.perPolyCollision = qtrue;
			continue;
		}

		if ( !Q_stricmp( keyname, "classname" ) && Q_stricmp( value, "worldspawn" ) )
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: expected worldspawn found '%s'\n", value );
			break;
		}
	}
}

/*
=================
CMod_LoadVisibility
=================
*/
#define VIS_HEADER 8
void CMod_LoadVisibility( lump_t *l )
{
	int  len;
	byte *buf;

	len = l->filelen;

	if ( !len )
	{
		cm.clusterBytes = ( cm.numClusters + 31 ) & ~31;
		cm.visibility = Hunk_Alloc( cm.clusterBytes, h_high );
		memset( cm.visibility, 255, cm.clusterBytes );
		return;
	}

	buf = cmod_base + l->fileofs;

	cm.vised = qtrue;
	cm.visibility = Hunk_Alloc( len, h_high );
	cm.numClusters = LittleLong( ( ( int * ) buf ) [ 0 ] );
	cm.clusterBytes = LittleLong( ( ( int * ) buf ) [ 1 ] );
	Com_Memcpy( cm.visibility, buf + VIS_HEADER, len - VIS_HEADER );
}

//==================================================================

/*
=================
CMod_LoadSurfaces
=================
*/
#define MAX_PATCH_SIZE  64
#define MAX_PATCH_VERTS ( MAX_PATCH_SIZE * MAX_PATCH_SIZE )
void CMod_LoadSurfaces( lump_t *surfs, lump_t *verts, lump_t *indexesLump )
{
	drawVert_t    *dv, *dv_p;
	dsurface_t    *in;
	int           count;
	int           i, j;
	cSurface_t    *surface;
	int           numVertexes;
	static vec3_t vertexes[ SHADER_MAX_VERTEXES ];
	int           width, height;
	int           shaderNum;
	int           numIndexes;
	static int    indexes[ SHADER_MAX_INDEXES ];
	int           *index;
	int           *index_p;
#ifdef USE_PHYSICS
	vec3_t        v[ 3 ];
#endif

	in = ( void * )( cmod_base + surfs->fileofs );

	if ( surfs->filelen % sizeof( *in ) )
	{
		Com_Error( ERR_DROP, "CMod_LoadSurfaces: funny lump size" );
	}

	cm.numSurfaces = count = surfs->filelen / sizeof( *in );
	cm.surfaces = Hunk_Alloc( cm.numSurfaces * sizeof( cm.surfaces[ 0 ] ), h_high );

	dv = ( void * )( cmod_base + verts->fileofs );

	if ( verts->filelen % sizeof( *dv ) )
	{
		Com_Error( ERR_DROP, "CMod_LoadSurfaces: funny lump size" );
	}

	index = ( void * )( cmod_base + indexesLump->fileofs );

	if ( indexesLump->filelen % sizeof( *index ) )
	{
		Com_Error( ERR_DROP, "CMod_LoadSurfaces: funny lump size" );
	}

#ifdef USE_PHYSICS
	CMod_PhysicsBeginBSPCollisionTree();
#endif

	// scan through all the surfaces
	for ( i = 0; i < count; i++, in++ )
	{
		if ( LittleLong( in->surfaceType ) == MST_PLANAR )
		{
#ifdef USE_PHYSICS
			int j = 0;
			int modelIndex = CMod_PhysicsBSPSurfaceIsModel( i );

			for ( j = 0; j < in->numIndexes; j += 3 )
			{
				VectorCopy( dv[ in->firstVert + index[ in->firstIndex + j + 2 ] ].xyz, v[ 0 ] );
				VectorCopy( dv[ in->firstVert + index[ in->firstIndex + j + 1 ] ].xyz, v[ 1 ] );
				VectorCopy( dv[ in->firstVert + index[ in->firstIndex + j + 0 ] ].xyz, v[ 2 ] );

				if ( modelIndex == 0 )
				{
					CMod_PhysicsAddBSPFace( v );
				}
				else
				{
					CMod_PhysicsAddFaceToModel( modelIndex, i, v );
				}
			}

#endif
		}
		else if ( LittleLong( in->surfaceType ) == MST_PATCH )
		{
			int j = 0;
#ifdef USE_PHYSICS
			int rowLimit = in->patchHeight - 1;
			int colLimit = in->patchWidth - 1;
#endif

			// FIXME: check for non-colliding patches
			cm.surfaces[ i ] = surface = Hunk_Alloc( sizeof( *surface ), h_high );
			surface->type = MST_PATCH;

			// load the full drawverts onto the stack
			width = LittleLong( in->patchWidth );
			height = LittleLong( in->patchHeight );
			numVertexes = width * height;

			if ( numVertexes > MAX_PATCH_VERTS )
			{
				Com_Error( ERR_DROP, "CMod_LoadSurfaces: MAX_PATCH_VERTS" );
			}

			dv_p = dv + LittleLong( in->firstVert );

			for ( j = 0; j < numVertexes; j++, dv_p++ )
			{
				vertexes[ j ][ 0 ] = LittleFloat( dv_p->xyz[ 0 ] );
				vertexes[ j ][ 1 ] = LittleFloat( dv_p->xyz[ 1 ] );
				vertexes[ j ][ 2 ] = LittleFloat( dv_p->xyz[ 2 ] );
			}

			shaderNum = LittleLong( in->shaderNum );
			surface->contents = cm.shaders[ shaderNum ].contentFlags;
			surface->surfaceFlags = cm.shaders[ shaderNum ].surfaceFlags;

			// create the internal facet structure
			surface->sc = CM_GeneratePatchCollide( width, height, vertexes );

#ifdef USE_PHYSICS

			for ( j = 0; j < rowLimit; ++j )
			{
				int k;

				for ( k = 0; k < colLimit; ++k )
				{
					VectorCopy( dv[ in->firstVert + ( j * in->patchWidth ) + k ].xyz, v[ 0 ] );
					VectorCopy( dv[ in->firstVert + ( j * in->patchWidth ) + k + 1 ].xyz, v[ 1 ] );
					VectorCopy( dv[ in->firstVert + ( ( j + 1 ) * in->patchWidth ) + k ].xyz, v[ 2 ] );

					CMod_PhysicsAddBSPFace( v );

					VectorCopy( dv[ in->firstVert + ( j * in->patchWidth ) + k + 1 ].xyz, v[ 0 ] );
					VectorCopy( dv[ in->firstVert + ( ( j + 1 ) * in->patchWidth ) + k ].xyz, v[ 1 ] );
					VectorCopy( dv[ in->firstVert + ( ( j + 1 ) * in->patchWidth ) + k + 1 ].xyz, v[ 2 ] );

					CMod_PhysicsAddBSPFace( v );
				}
			}

#endif
		}
		else if ( LittleLong( in->surfaceType ) == MST_TRIANGLE_SOUP && ( cm.perPolyCollision || cm_forceTriangles->integer ) )
		{
			// FIXME: check for non-colliding triangle soups

			cm.surfaces[ i ] = surface = Hunk_Alloc( sizeof( *surface ), h_high );
			surface->type = MST_TRIANGLE_SOUP;

			// load the full drawverts onto the stack
			numVertexes = LittleLong( in->numVerts );

			if ( numVertexes > SHADER_MAX_VERTEXES )
			{
				Com_Error( ERR_DROP, "CMod_LoadSurfaces: SHADER_MAX_VERTEXES" );
			}

			dv_p = dv + LittleLong( in->firstVert );

			for ( j = 0; j < numVertexes; j++, dv_p++ )
			{
				vertexes[ j ][ 0 ] = LittleFloat( dv_p->xyz[ 0 ] );
				vertexes[ j ][ 1 ] = LittleFloat( dv_p->xyz[ 1 ] );
				vertexes[ j ][ 2 ] = LittleFloat( dv_p->xyz[ 2 ] );
			}

			numIndexes = LittleLong( in->numIndexes );

			if ( numIndexes > SHADER_MAX_INDEXES )
			{
				Com_Error( ERR_DROP, "CMod_LoadSurfaces: SHADER_MAX_INDEXES" );
			}

			index_p = index + LittleLong( in->firstIndex );

			for ( j = 0; j < numIndexes; j++, index_p++ )
			{
				indexes[ j ] = LittleLong( *index_p );

				if ( indexes[ j ] < 0 || indexes[ j ] >= numVertexes )
				{
					Com_Error( ERR_DROP, "CMod_LoadSurfaces: Bad index in trisoup surface" );
				}
			}

			shaderNum = LittleLong( in->shaderNum );
			surface->contents = cm.shaders[ shaderNum ].contentFlags;
			surface->surfaceFlags = cm.shaders[ shaderNum ].surfaceFlags;

			// create the internal facet structure
			surface->sc = CM_GenerateTriangleSoupCollide( numVertexes, vertexes, numIndexes, indexes );
		}
	}

#ifdef USE_PHYSICS
	CMod_PhysicsEndBSPCollisionTree();
#endif
}

//==================================================================

unsigned CM_LumpChecksum( lump_t *lump )
{
	return LittleLong( Com_BlockChecksum( cmod_base + lump->fileofs, lump->filelen ) );
}

unsigned CM_Checksum( dheader_t *header )
{
	unsigned checksums[ 16 ];

	checksums[ 0 ] = CM_LumpChecksum( &header->lumps[ LUMP_SHADERS ] );
	checksums[ 1 ] = CM_LumpChecksum( &header->lumps[ LUMP_LEAFS ] );
	checksums[ 2 ] = CM_LumpChecksum( &header->lumps[ LUMP_LEAFBRUSHES ] );
	checksums[ 3 ] = CM_LumpChecksum( &header->lumps[ LUMP_LEAFSURFACES ] );
	checksums[ 4 ] = CM_LumpChecksum( &header->lumps[ LUMP_PLANES ] );
	checksums[ 5 ] = CM_LumpChecksum( &header->lumps[ LUMP_BRUSHSIDES ] );
	checksums[ 6 ] = CM_LumpChecksum( &header->lumps[ LUMP_BRUSHES ] );
	checksums[ 7 ] = CM_LumpChecksum( &header->lumps[ LUMP_MODELS ] );
	checksums[ 8 ] = CM_LumpChecksum( &header->lumps[ LUMP_NODES ] );
	checksums[ 9 ] = CM_LumpChecksum( &header->lumps[ LUMP_SURFACES ] );
	checksums[ 10 ] = CM_LumpChecksum( &header->lumps[ LUMP_DRAWVERTS ] );

	return LittleLong( Com_BlockChecksum( checksums, 11 * 4 ) );
}

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void CM_LoadMap( const char *name, qboolean clientload, int *checksum )
{
	int             *buf;
	int             i;
	dheader_t       header;
	int             length;
	static unsigned last_checksum;

	if ( !name || !name[ 0 ] )
	{
		Com_Error( ERR_DROP, "CM_LoadMap: NULL name" );
	}

#ifndef BSPC
	cm_noAreas = Cvar_Get( "cm_noAreas", "0", CVAR_CHEAT );
	cm_noCurves = Cvar_Get( "cm_noCurves", "0", CVAR_CHEAT );
	cm_forceTriangles = Cvar_Get( "cm_forceTriangles", "0", CVAR_CHEAT | CVAR_LATCH );
	cm_playerCurveClip = Cvar_Get( "cm_playerCurveClip", "1", CVAR_ARCHIVE | CVAR_CHEAT );
	cm_optimize = Cvar_Get( "cm_optimize", "1", CVAR_CHEAT );
#endif
	Com_DPrintf( "CM_LoadMap( %s, %i )\n", name, clientload );

	if ( !strcmp( cm.name, name ) && clientload )
	{
		*checksum = last_checksum;
		return;
	}

	// free old stuff
	memset( &cm, 0, sizeof( cm ) );
	CM_ClearLevelPatches();

	if ( !name[ 0 ] )
	{
		cm.numLeafs = 1;
		cm.numClusters = 1;
		cm.numAreas = 1;
		cm.cmodels = Hunk_Alloc( sizeof( *cm.cmodels ), h_high );
		*checksum = 0;
		return;
	}

	//
	// load the file
	//
#ifndef BSPC
	length = FS_ReadFile( name, ( void ** ) &buf );
#else
	length = LoadQuakeFile( ( quakefile_t * ) name, ( void ** ) &buf );
#endif

	if ( !buf )
	{
		Com_Error( ERR_DROP, "Couldn't load %s", name );
	}

	last_checksum = LittleLong( Com_BlockChecksum( buf, length ) );
	*checksum = last_checksum;

	header = * ( dheader_t * ) buf;

	for ( i = 0; i < sizeof( dheader_t ) / 4; i++ )
	{
		( ( int * ) &header ) [ i ] = LittleLong( ( ( int * ) &header ) [ i ] );
	}

	if ( header.version != BSP_VERSION && header.version != BSP_VERSION_Q3 )
	{
		Com_Error( ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i for ET or %i for Q3)",
		           name, header.version, BSP_VERSION, BSP_VERSION_Q3 );
	}

	cmod_base = ( byte * ) buf;

#ifdef USE_PHYSICS
	CMod_PhysicsInit();
#endif

	// load into heap
	CMod_LoadShaders( &header.lumps[ LUMP_SHADERS ] );
	CMod_LoadLeafs( &header.lumps[ LUMP_LEAFS ] );
	CMod_LoadLeafBrushes( &header.lumps[ LUMP_LEAFBRUSHES ] );
	CMod_LoadLeafSurfaces( &header.lumps[ LUMP_LEAFSURFACES ] );
	CMod_LoadPlanes( &header.lumps[ LUMP_PLANES ] );
	CMod_LoadBrushSides( &header.lumps[ LUMP_BRUSHSIDES ] );
	CMod_LoadBrushes( &header.lumps[ LUMP_BRUSHES ] );
	CMod_LoadSubmodels( &header.lumps[ LUMP_MODELS ] );
	CMod_LoadNodes( &header.lumps[ LUMP_NODES ] );
	CMod_LoadEntityString( &header.lumps[ LUMP_ENTITIES ] );
	CMod_LoadVisibility( &header.lumps[ LUMP_VISIBILITY ] );
	CMod_LoadSurfaces( &header.lumps[ LUMP_SURFACES ], &header.lumps[ LUMP_DRAWVERTS ], &header.lumps[ LUMP_DRAWINDEXES ] );

	CMod_CreateBrushSideWindings();

	// we are NOT freeing the file, because it is cached for the ref
	FS_FreeFile( buf );

	CM_InitBoxHull();

	CM_FloodAreaConnections();

	// allow this to be cached if it is loaded by the server
	if ( !clientload )
	{
		Q_strncpyz( cm.name, name, sizeof( cm.name ) );
	}
}

/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap( void )
{
	Com_Memset( &cm, 0, sizeof( cm ) );
	CM_ClearLevelPatches();

#ifdef USE_PHYSICS
	CMod_PhysicsShutdown();
#endif
}

/*
==================
CM_ClipHandleToModel
==================
*/
cmodel_t       *CM_ClipHandleToModel( clipHandle_t handle )
{
	if ( handle < 0 )
	{
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle );
	}

	if ( handle < cm.numSubModels )
	{
		return &cm.cmodels[ handle ];
	}

	if ( handle == BOX_MODEL_HANDLE || handle == CAPSULE_MODEL_HANDLE )
	{
		return &box_model;
	}

	if ( handle < MAX_SUBMODELS )
	{
		Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i < %i < %i", cm.numSubModels, handle, MAX_SUBMODELS );
	}

	Com_Error( ERR_DROP, "CM_ClipHandleToModel: bad handle %i", handle + MAX_SUBMODELS );

	return NULL;
}

/*
==================
CM_InlineModel
==================
*/
clipHandle_t CM_InlineModel( int index )
{
	if ( index < 0 || index >= cm.numSubModels )
	{
		Com_Error( ERR_DROP, "CM_InlineModel: bad number" );
	}

	return index;
}

int CM_NumClusters( void )
{
	return cm.numClusters;
}

int CM_NumInlineModels( void )
{
	return cm.numSubModels;
}

char           *CM_EntityString( void )
{
	return cm.entityString;
}

int CM_LeafCluster( int leafnum )
{
	if ( leafnum < 0 || leafnum >= cm.numLeafs )
	{
		Com_Error( ERR_DROP, "CM_LeafCluster: bad number" );
	}

	return cm.leafs[ leafnum ].cluster;
}

int CM_LeafArea( int leafnum )
{
	if ( leafnum < 0 || leafnum >= cm.numLeafs )
	{
		Com_Error( ERR_DROP, "CM_LeafArea: bad number" );
	}

	return cm.leafs[ leafnum ].area;
}

//=======================================================================

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull( void )
{
	int          i;
	int          side;
	cplane_t     *p;
	cbrushside_t *s;

	box_planes = &cm.planes[ cm.numPlanes ];

	box_brush = &cm.brushes[ cm.numBrushes ];
	box_brush->numsides = 6;
	box_brush->sides = cm.brushsides + cm.numBrushSides;
	box_brush->contents = CONTENTS_BODY;
	box_brush->edges = ( cbrushedge_t * ) Hunk_Alloc( sizeof( cbrushedge_t ) * 12, h_low );
	box_brush->numEdges = 12;

	box_model.leaf.numLeafBrushes = 1;
//  box_model.leaf.firstLeafBrush = cm.numBrushes;
	box_model.leaf.firstLeafBrush = cm.numLeafBrushes;
	cm.leafbrushes[ cm.numLeafBrushes ] = cm.numBrushes;

	for ( i = 0; i < 6; i++ )
	{
		side = i & 1;

		// brush sides
		s = &cm.brushsides[ cm.numBrushSides + i ];
		s->plane = cm.planes + ( cm.numPlanes + i * 2 + side );
		s->surfaceFlags = 0;

		// planes
		p = &box_planes[ i * 2 ];
		p->type = i >> 1;
		p->signbits = 0;
		VectorClear( p->normal );
		p->normal[ i >> 1 ] = 1;

		p = &box_planes[ i * 2 + 1 ];
		p->type = 3 + ( i >> 1 );
		p->signbits = 0;
		VectorClear( p->normal );
		p->normal[ i >> 1 ] = -1;

		SetPlaneSignbits( p );
	}
}

/*
===================
CM_TempBoxModel

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
Capsules are handled differently though.
===================
*/
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule )
{
	VectorCopy( mins, box_model.mins );
	VectorCopy( maxs, box_model.maxs );

	if ( capsule )
	{
		return CAPSULE_MODEL_HANDLE;
	}

	box_planes[ 0 ].dist = maxs[ 0 ];
	box_planes[ 1 ].dist = -maxs[ 0 ];
	box_planes[ 2 ].dist = mins[ 0 ];
	box_planes[ 3 ].dist = -mins[ 0 ];
	box_planes[ 4 ].dist = maxs[ 1 ];
	box_planes[ 5 ].dist = -maxs[ 1 ];
	box_planes[ 6 ].dist = mins[ 1 ];
	box_planes[ 7 ].dist = -mins[ 1 ];
	box_planes[ 8 ].dist = maxs[ 2 ];
	box_planes[ 9 ].dist = -maxs[ 2 ];
	box_planes[ 10 ].dist = mins[ 2 ];
	box_planes[ 11 ].dist = -mins[ 2 ];

	// First side
	VectorSet( box_brush->edges[ 0 ].p0, mins[ 0 ], mins[ 1 ], mins[ 2 ] );
	VectorSet( box_brush->edges[ 0 ].p1, mins[ 0 ], maxs[ 1 ], mins[ 2 ] );
	VectorSet( box_brush->edges[ 1 ].p0, mins[ 0 ], maxs[ 1 ], mins[ 2 ] );
	VectorSet( box_brush->edges[ 1 ].p1, mins[ 0 ], maxs[ 1 ], maxs[ 2 ] );
	VectorSet( box_brush->edges[ 2 ].p0, mins[ 0 ], maxs[ 1 ], maxs[ 2 ] );
	VectorSet( box_brush->edges[ 2 ].p1, mins[ 0 ], mins[ 1 ], maxs[ 2 ] );
	VectorSet( box_brush->edges[ 3 ].p0, mins[ 0 ], mins[ 1 ], maxs[ 2 ] );
	VectorSet( box_brush->edges[ 3 ].p1, mins[ 0 ], mins[ 1 ], mins[ 2 ] );

	// Opposite side
	VectorSet( box_brush->edges[ 4 ].p0, maxs[ 0 ], mins[ 1 ], mins[ 2 ] );
	VectorSet( box_brush->edges[ 4 ].p1, maxs[ 0 ], maxs[ 1 ], mins[ 2 ] );
	VectorSet( box_brush->edges[ 5 ].p0, maxs[ 0 ], maxs[ 1 ], mins[ 2 ] );
	VectorSet( box_brush->edges[ 5 ].p1, maxs[ 0 ], maxs[ 1 ], maxs[ 2 ] );
	VectorSet( box_brush->edges[ 6 ].p0, maxs[ 0 ], maxs[ 1 ], maxs[ 2 ] );
	VectorSet( box_brush->edges[ 6 ].p1, maxs[ 0 ], mins[ 1 ], maxs[ 2 ] );
	VectorSet( box_brush->edges[ 7 ].p0, maxs[ 0 ], mins[ 1 ], maxs[ 2 ] );
	VectorSet( box_brush->edges[ 7 ].p1, maxs[ 0 ], mins[ 1 ], mins[ 2 ] );

	// Connecting edges
	VectorSet( box_brush->edges[ 8 ].p0, mins[ 0 ], mins[ 1 ], mins[ 2 ] );
	VectorSet( box_brush->edges[ 8 ].p1, maxs[ 0 ], mins[ 1 ], mins[ 2 ] );
	VectorSet( box_brush->edges[ 9 ].p0, mins[ 0 ], maxs[ 1 ], mins[ 2 ] );
	VectorSet( box_brush->edges[ 9 ].p1, maxs[ 0 ], maxs[ 1 ], mins[ 2 ] );
	VectorSet( box_brush->edges[ 10 ].p0, mins[ 0 ], maxs[ 1 ], maxs[ 2 ] );
	VectorSet( box_brush->edges[ 10 ].p1, maxs[ 0 ], maxs[ 1 ], maxs[ 2 ] );
	VectorSet( box_brush->edges[ 11 ].p0, mins[ 0 ], mins[ 1 ], maxs[ 2 ] );
	VectorSet( box_brush->edges[ 11 ].p1, maxs[ 0 ], mins[ 1 ], maxs[ 2 ] );

	VectorCopy( mins, box_brush->bounds[ 0 ] );
	VectorCopy( maxs, box_brush->bounds[ 1 ] );

	return BOX_MODEL_HANDLE;
}

/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs )
{
	cmodel_t *cmod;

	cmod = CM_ClipHandleToModel( model );
	VectorCopy( cmod->mins, mins );
	VectorCopy( cmod->maxs, maxs );
}
