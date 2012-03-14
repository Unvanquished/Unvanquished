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

/*****************************************************************************
 * name:    be_aas_optimize.c
 *
 * desc:    decreases the .aas file size after the reachabilities have
 *        been calculated, just dumps all the faces, edges and vertexes
 *
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_libvar.h"
//#include "l_utils.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

typedef struct optimized_s
{
	//vertixes
	int             numvertexes;
	aas_vertex_t    *vertexes;
	//edges
	int             numedges;
	aas_edge_t      *edges;
	//edge index
	int             edgeindexsize;
	aas_edgeindex_t *edgeindex;
	//faces
	int             numfaces;
	aas_face_t      *faces;
	//face index
	int             faceindexsize;
	aas_faceindex_t *faceindex;
	//convex areas
	int             numareas;
	aas_area_t      *areas;

	///
	// RF, addition of removal of non-reachability areas
	//
	//convex area settings
	int                numareasettings;
	aas_areasettings_t *areasettings;
	//reachablity list
	int                reachabilitysize;
	aas_reachability_t *reachability;

	/*  //nodes of the bsp tree
	        int numnodes;
	        aas_node_t *nodes;
	        //cluster portals
	        int numportals;
	        aas_portal_t *portals;
	        //clusters
	        int numclusters;
	                                                aas_cluster_t *clusters;
	*/                                                                                                                                                                                                                                                                                             //
	int *vertexoptimizeindex;
	int *edgeoptimizeindex;
	int *faceoptimizeindex;
	//
	int *areakeep;
	int *arearemap;
	int *removedareas;
	int *reachabilityremap;
} optimized_t;

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
int AAS_KeepEdge ( aas_edge_t *edge )
{
	return 1;
} //end of the function AAS_KeepFace

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
int AAS_OptimizeEdge ( optimized_t *optimized, int edgenum )
{
	int        i, optedgenum;
	aas_edge_t *edge, *optedge;

	edge = & ( *aasworld ).edges[ abs ( edgenum ) ];

	if ( !AAS_KeepEdge ( edge ) )
	{
		return 0;
	}

	optedgenum = optimized->edgeoptimizeindex[ abs ( edgenum ) ];

	if ( optedgenum )
	{
		//keep the edge reversed sign
		if ( edgenum > 0 )
		{
			return optedgenum;
		}
		else
		{
			return -optedgenum;
		}
	} //end if

	optedge = &optimized->edges[ optimized->numedges ];

	for ( i = 0; i < 2; i++ )
	{
		if ( optimized->vertexoptimizeindex[ edge->v[ i ] ] )
		{
			optedge->v[ i ] = optimized->vertexoptimizeindex[ edge->v[ i ] ];
		} //end if
		else
		{
			VectorCopy ( ( *aasworld ).vertexes[ edge->v[ i ] ], optimized->vertexes[ optimized->numvertexes ] );
			optedge->v[ i ] = optimized->numvertexes;
			optimized->vertexoptimizeindex[ edge->v[ i ] ] = optimized->numvertexes;
			optimized->numvertexes++;
		} //end else
	} //end for

	optimized->edgeoptimizeindex[ abs ( edgenum ) ] = optimized->numedges;
	optedgenum = optimized->numedges;
	optimized->numedges++;

	//keep the edge reversed sign
	if ( edgenum > 0 )
	{
		return optedgenum;
	}
	else
	{
		return -optedgenum;
	}
} //end of the function AAS_OptimizeEdge

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
int AAS_KeepFace ( aas_face_t *face )
{
	if ( ! ( face->faceflags & FACE_LADDER ) )
	{
		return 0;
	}
	else
	{
		return 1;
	}
} //end of the function AAS_KeepFace

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
int AAS_OptimizeFace ( optimized_t *optimized, int facenum )
{
	int        i, edgenum, optedgenum, optfacenum;
	aas_face_t *face, *optface;

	face = & ( *aasworld ).faces[ abs ( facenum ) ];

	if ( !AAS_KeepFace ( face ) )
	{
		return 0;
	}

	optfacenum = optimized->faceoptimizeindex[ abs ( facenum ) ];

	if ( optfacenum )
	{
		//keep the face side sign
		if ( facenum > 0 )
		{
			return optfacenum;
		}
		else
		{
			return -optfacenum;
		}
	} //end if

	optface = &optimized->faces[ optimized->numfaces ];
	memcpy ( optface, face, sizeof ( aas_face_t ) );

	optface->numedges = 0;
	optface->firstedge = optimized->edgeindexsize;

	for ( i = 0; i < face->numedges; i++ )
	{
		edgenum = ( *aasworld ).edgeindex[ face->firstedge + i ];
		optedgenum = AAS_OptimizeEdge ( optimized, edgenum );

		if ( optedgenum )
		{
			optimized->edgeindex[ optface->firstedge + optface->numedges ] = optedgenum;
			optface->numedges++;
			optimized->edgeindexsize++;
		} //end if
	} //end for

	optimized->faceoptimizeindex[ abs ( facenum ) ] = optimized->numfaces;
	optfacenum = optimized->numfaces;
	optimized->numfaces++;

	//keep the face side sign
	if ( facenum > 0 )
	{
		return optfacenum;
	}
	else
	{
		return -optfacenum;
	}
} //end of the function AAS_OptimizeFace

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
void AAS_OptimizeArea ( optimized_t *optimized, int areanum )
{
	int        i, facenum, optfacenum;
	aas_area_t *area, *optarea;

	area = & ( *aasworld ).areas[ areanum ];
	optarea = &optimized->areas[ areanum ];
	memcpy ( optarea, area, sizeof ( aas_area_t ) );

	optarea->numfaces = 0;
	optarea->firstface = optimized->faceindexsize;

	for ( i = 0; i < area->numfaces; i++ )
	{
		facenum = ( *aasworld ).faceindex[ area->firstface + i ];
		optfacenum = AAS_OptimizeFace ( optimized, facenum );

		if ( optfacenum )
		{
			optimized->faceindex[ optarea->firstface + optarea->numfaces ] = optfacenum;
			optarea->numfaces++;
			optimized->faceindexsize++;
		} //end if
	} //end for
} //end of the function AAS_OptimizeArea

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
void AAS_OptimizeAlloc ( optimized_t *optimized )
{
	optimized->vertexes = ( aas_vertex_t * ) GetClearedMemory ( ( *aasworld ).numvertexes * sizeof ( aas_vertex_t ) );
	optimized->numvertexes = 0;
	optimized->edges = ( aas_edge_t * ) GetClearedMemory ( ( *aasworld ).numedges * sizeof ( aas_edge_t ) );
	optimized->numedges = 1; //edge zero is a dummy
	optimized->edgeindex = ( aas_edgeindex_t * ) GetClearedMemory ( ( *aasworld ).edgeindexsize * sizeof ( aas_edgeindex_t ) );
	optimized->edgeindexsize = 0;
	optimized->faces = ( aas_face_t * ) GetClearedMemory ( ( *aasworld ).numfaces * sizeof ( aas_face_t ) );
	optimized->numfaces = 1; //face zero is a dummy
	optimized->faceindex = ( aas_faceindex_t * ) GetClearedMemory ( ( *aasworld ).faceindexsize * sizeof ( aas_faceindex_t ) );
	optimized->faceindexsize = 0;
	optimized->areas = ( aas_area_t * ) GetClearedMemory ( ( *aasworld ).numareas * sizeof ( aas_area_t ) );
	optimized->numareas = ( *aasworld ).numareas;
	//
	optimized->vertexoptimizeindex = ( int * ) GetClearedMemory ( ( *aasworld ).numvertexes * sizeof ( int ) );
	optimized->edgeoptimizeindex = ( int * ) GetClearedMemory ( ( *aasworld ).numedges * sizeof ( int ) );
	optimized->faceoptimizeindex = ( int * ) GetClearedMemory ( ( *aasworld ).numfaces * sizeof ( int ) );
} //end of the function AAS_OptimizeAlloc

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
void AAS_OptimizeStore ( optimized_t *optimized )
{
	//store the optimized vertexes
	if ( ( *aasworld ).vertexes )
	{
		FreeMemory ( ( *aasworld ).vertexes );
	}

	( *aasworld ).vertexes = optimized->vertexes;
	( *aasworld ).numvertexes = optimized->numvertexes;

	//store the optimized edges
	if ( ( *aasworld ).edges )
	{
		FreeMemory ( ( *aasworld ).edges );
	}

	( *aasworld ).edges = optimized->edges;
	( *aasworld ).numedges = optimized->numedges;

	//store the optimized edge index
	if ( ( *aasworld ).edgeindex )
	{
		FreeMemory ( ( *aasworld ).edgeindex );
	}

	( *aasworld ).edgeindex = optimized->edgeindex;
	( *aasworld ).edgeindexsize = optimized->edgeindexsize;

	//store the optimized faces
	if ( ( *aasworld ).faces )
	{
		FreeMemory ( ( *aasworld ).faces );
	}

	( *aasworld ).faces = optimized->faces;
	( *aasworld ).numfaces = optimized->numfaces;

	//store the optimized face index
	if ( ( *aasworld ).faceindex )
	{
		FreeMemory ( ( *aasworld ).faceindex );
	}

	( *aasworld ).faceindex = optimized->faceindex;
	( *aasworld ).faceindexsize = optimized->faceindexsize;

	//store the optimized areas
	if ( ( *aasworld ).areas )
	{
		FreeMemory ( ( *aasworld ).areas );
	}

	( *aasworld ).areas = optimized->areas;
	( *aasworld ).numareas = optimized->numareas;
	//free optimize indexes
	FreeMemory ( optimized->vertexoptimizeindex );
	FreeMemory ( optimized->edgeoptimizeindex );
	FreeMemory ( optimized->faceoptimizeindex );
} //end of the function AAS_OptimizeStore

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
void AAS_Optimize ( void )
{
	int         i, sign;
	optimized_t optimized;

	AAS_OptimizeAlloc ( &optimized );

	for ( i = 1; i < ( *aasworld ).numareas; i++ )
	{
		AAS_OptimizeArea ( &optimized, i );
	} //end for

	//reset the reachability face pointers
	for ( i = 0; i < ( *aasworld ).reachabilitysize; i++ )
	{
		//NOTE: for TRAVEL_ELEVATOR the facenum is the model number of
		//      the elevator
		if ( ( *aasworld ).reachability[ i ].traveltype == TRAVEL_ELEVATOR )
		{
			continue;
		}

		//NOTE: for TRAVEL_JUMPPAD the facenum is the Z velocity and the edgenum is the hor velocity
		if ( ( *aasworld ).reachability[ i ].traveltype == TRAVEL_JUMPPAD )
		{
			continue;
		}

		//NOTE: for TRAVEL_FUNCBOB the facenum and edgenum contain other coded information
		if ( ( *aasworld ).reachability[ i ].traveltype == TRAVEL_FUNCBOB )
		{
			continue;
		}

		//
		sign = ( *aasworld ).reachability[ i ].facenum;
		( *aasworld ).reachability[ i ].facenum = optimized.faceoptimizeindex[ abs ( ( *aasworld ).reachability[ i ].facenum ) ];

		if ( sign < 0 )
		{
			( *aasworld ).reachability[ i ].facenum = - ( *aasworld ).reachability[ i ].facenum;
		}

		sign = ( *aasworld ).reachability[ i ].edgenum;
		( *aasworld ).reachability[ i ].edgenum = optimized.edgeoptimizeindex[ abs ( ( *aasworld ).reachability[ i ].edgenum ) ];

		if ( sign < 0 )
		{
			( *aasworld ).reachability[ i ].edgenum = - ( *aasworld ).reachability[ i ].edgenum;
		}
	} //end for

	//store the optimized AAS data into (*aasworld)
	AAS_OptimizeStore ( &optimized );
	//print some nice stuff :)
	botimport.Print ( PRT_MESSAGE, "AAS data optimized.\n" );
} //end of the function AAS_Optimize

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
void AAS_RemoveNonReachabilityAlloc ( optimized_t *optimized )
{
	optimized->areas = ( aas_area_t * ) GetClearedMemory ( ( *aasworld ).numareas * sizeof ( aas_area_t ) );
	//
	optimized->areasettings = ( aas_areasettings_t * ) GetClearedMemory ( ( *aasworld ).numareas * sizeof ( aas_areasettings_t ) );
	//
	optimized->reachability = ( aas_reachability_t * ) GetClearedMemory ( ( *aasworld ).reachabilitysize * sizeof ( aas_reachability_t ) );
	optimized->reachabilitysize = ( *aasworld ).reachabilitysize;

	/*  //
	        optimized->nodes = (aas_node_t *) GetClearedMemory((*aasworld).numnodes * sizeof(aas_node_t));
	        optimized->numnodes = (*aasworld).numnodes;
	        //
	        optimized->portals = (aas_portals_t *) GetClearedMemory((*aasworld).numportals * sizeof(aas_portal_t));
	        optimized->numportals = (*aasworld).numportals;
	        //
	        optimized->clusters = (aas_cluster_t *) GetClearedMemory((*aasworld).numclusters * sizeof(aas_cluster_t));
	                                                optimized->numclusters = (*aasworld).numclusters;
	*/                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               //
	optimized->areakeep = ( int * ) GetClearedMemory ( ( *aasworld ).numareas * sizeof ( int ) );
	optimized->arearemap = ( int * ) GetClearedMemory ( ( *aasworld ).numareas * sizeof ( int ) );
	optimized->removedareas = ( int * ) GetClearedMemory ( ( *aasworld ).numareas * sizeof ( int ) );
	optimized->reachabilityremap = ( int * ) GetClearedMemory ( ( *aasworld ).reachabilitysize * sizeof ( int ) );
} //end of the function AAS_OptimizeAlloc

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
void AAS_NumberClusterAreas ( int clusternum );

void AAS_RemoveNonReachability ( void )
{
	int         i, j;
	optimized_t optimized;
	int         removed = 0, valid = 0, numoriginalareas;
	int         validreach = 0;
	aas_face_t  *face;

	AAS_RemoveNonReachabilityAlloc ( &optimized );

	// mark portal areas as non-removable
	if ( aasworld->numportals )
	{
		for ( i = 0; i < aasworld->numportals; i++ )
		{
			optimized.areakeep[ aasworld->portals[ i ].areanum ] = qtrue;
		}
	}

	// remove non-reachability areas
	numoriginalareas = aasworld->numareas;

	for ( i = 1; i < ( *aasworld ).numareas; i++ )
	{
		// is this a reachability area?
		if ( optimized.areakeep[ i ] || aasworld->areasettings[ i ].numreachableareas )
		{
			optimized.arearemap[ i ] = ++valid;
			// copy it to the optimized areas
			optimized.areas[ valid ] = ( *aasworld ).areas[ i ];
			optimized.areas[ valid ].areanum = valid;
			continue;
		}

		// yes it is if it made it to here
		removed++;
		optimized.removedareas[ i ] = qtrue;
	}

	optimized.numareas = valid + 1;

	// store the new areas
	if ( ( *aasworld ).areas )
	{
		FreeMemory ( ( *aasworld ).areas );
	}

	( *aasworld ).areas = optimized.areas;
	( *aasworld ).numareas = optimized.numareas;
	//
	// remove reachabilities that are no longer required
	validreach = 1;

	for ( i = 1; i < aasworld->reachabilitysize; i++ )
	{
		optimized.reachabilityremap[ i ] = validreach;

		if ( optimized.removedareas[ aasworld->reachability[ i ].areanum ] )
		{
			continue;
		}

		// save this reachability
		optimized.reachability[ validreach ] = aasworld->reachability[ i ];
		optimized.reachability[ validreach ].areanum = optimized.arearemap[ optimized.reachability[ validreach ].areanum ];
		//
		validreach++;
	}

	optimized.reachabilitysize = validreach;

	// store the reachabilities
	if ( ( *aasworld ).reachability )
	{
		FreeMemory ( ( *aasworld ).reachability );
	}

	( *aasworld ).reachability = optimized.reachability;
	( *aasworld ).reachabilitysize = optimized.reachabilitysize;

	//
	// remove and update areasettings
	for ( i = 1; i < numoriginalareas; i++ )
	{
		if ( optimized.removedareas[ i ] )
		{
			continue;
		}

		j = optimized.arearemap[ i ];
		optimized.areasettings[ j ] = aasworld->areasettings[ i ];
		optimized.areasettings[ j ].firstreachablearea = optimized.reachabilityremap[ aasworld->areasettings[ i ].firstreachablearea ];
		optimized.areasettings[ j ].numreachableareas =
		  1 + optimized.reachabilityremap[ aasworld->areasettings[ i ].firstreachablearea +
		                                   aasworld->areasettings[ i ].numreachableareas - 1 ] -
		  optimized.areasettings[ j ].firstreachablearea;
	}

	//
	// update faces (TODO: remove unused)
	for ( i = 1, face = &aasworld->faces[ 1 ]; i < aasworld->numfaces; i++, face++ )
	{
		if ( !optimized.removedareas[ face->backarea ] )
		{
			face->backarea = optimized.arearemap[ face->backarea ];
		}
		else
		{
			// now points to a void
			face->backarea = 0;
		}

		if ( !optimized.removedareas[ face->frontarea ] )
		{
			face->frontarea = optimized.arearemap[ face->frontarea ];
		}
		else
		{
			face->frontarea = 0;
		}
	}

	// store the areasettings
	if ( ( *aasworld ).areasettings )
	{
		FreeMemory ( ( *aasworld ).areasettings );
	}

	( *aasworld ).areasettings = optimized.areasettings;
	( *aasworld ).numareasettings = optimized.numareas;

	//
	// update nodes
	for ( i = 1; i < ( *aasworld ).numnodes; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			if ( aasworld->nodes[ i ].children[ j ] < 0 )
			{
				if ( optimized.removedareas[ -aasworld->nodes[ i ].children[ j ] ] )
				{
					aasworld->nodes[ i ].children[ j ] = 0; //make it solid
				}
				else
				{
					// remap
					aasworld->nodes[ i ].children[ j ] = -optimized.arearemap[ -aasworld->nodes[ i ].children[ j ] ];
				}
			}
		}
	}

	//
	// update portal areanums
	for ( i = 0; i < aasworld->numportals; i++ )
	{
		aasworld->portals[ i ].areanum = optimized.arearemap[ aasworld->portals[ i ].areanum ];
	}

	// update clusters and portals
	for ( i = 0; i < ( *aasworld ).numclusters; i++ )
	{
		AAS_NumberClusterAreas ( i );
	}

	// free temporary memory
	FreeMemory ( optimized.areakeep );
	FreeMemory ( optimized.arearemap );
	FreeMemory ( optimized.removedareas );
	FreeMemory ( optimized.reachabilityremap );
	//print some nice stuff :)
	botimport.Print ( PRT_MESSAGE, "%i non-reachability areas removed, %i remain.\n", removed, valid );
} //end of the function AAS_Optimize

//===========================================================================
//
// Parameter:               -
// Returns:                 -
// Changes Globals:     -
//===========================================================================
void AAS_RemoveNonGrounded ( void )
{
	int         i, j;
	optimized_t optimized;
	int         removed = 0, valid = 0, numoriginalareas;
	int         validreach = 0;
	aas_face_t  *face;

	AAS_RemoveNonReachabilityAlloc ( &optimized );

	// mark portal areas as non-removable
	if ( aasworld->numportals )
	{
		for ( i = 0; i < aasworld->numportals; i++ )
		{
			optimized.areakeep[ aasworld->portals[ i ].areanum ] = qtrue;
		}
	}

	// remove non-reachability areas
	numoriginalareas = aasworld->numareas;

	for ( i = 1; i < ( *aasworld ).numareas; i++ )
	{
		// is this a grounded area?
		if ( optimized.areakeep[ i ] || ( aasworld->areasettings[ i ].areaflags & ( AREA_GROUNDED | AREA_LADDER ) ) )
		{
			optimized.arearemap[ i ] = ++valid;
			// copy it to the optimized areas
			optimized.areas[ valid ] = ( *aasworld ).areas[ i ];
			optimized.areas[ valid ].areanum = valid;
			continue;
		}

		// yes it is if it made it to here
		removed++;
		optimized.removedareas[ i ] = qtrue;
	}

	optimized.numareas = valid + 1;

	// store the new areas
	if ( ( *aasworld ).areas )
	{
		FreeMemory ( ( *aasworld ).areas );
	}

	( *aasworld ).areas = optimized.areas;
	( *aasworld ).numareas = optimized.numareas;
	//
	// remove reachabilities that are no longer required
	validreach = 1;

	for ( i = 1; i < aasworld->reachabilitysize; i++ )
	{
		optimized.reachabilityremap[ i ] = validreach;

		if ( optimized.removedareas[ aasworld->reachability[ i ].areanum ] )
		{
			continue;
		}

		// save this reachability
		optimized.reachability[ validreach ] = aasworld->reachability[ i ];
		optimized.reachability[ validreach ].areanum = optimized.arearemap[ optimized.reachability[ validreach ].areanum ];
		//
		validreach++;
	}

	optimized.reachabilitysize = validreach;

	// store the reachabilities
	if ( ( *aasworld ).reachability )
	{
		FreeMemory ( ( *aasworld ).reachability );
	}

	( *aasworld ).reachability = optimized.reachability;
	( *aasworld ).reachabilitysize = optimized.reachabilitysize;

	//
	// remove and update areasettings
	for ( i = 1; i < numoriginalareas; i++ )
	{
		if ( optimized.removedareas[ i ] )
		{
			continue;
		}

		j = optimized.arearemap[ i ];
		optimized.areasettings[ j ] = aasworld->areasettings[ i ];
		optimized.areasettings[ j ].firstreachablearea = optimized.reachabilityremap[ aasworld->areasettings[ i ].firstreachablearea ];
		optimized.areasettings[ j ].numreachableareas =
		  1 + optimized.reachabilityremap[ aasworld->areasettings[ i ].firstreachablearea +
		                                   aasworld->areasettings[ i ].numreachableareas - 1 ] -
		  optimized.areasettings[ j ].firstreachablearea;
	}

	//
	// update faces (TODO: remove unused)
	for ( i = 1, face = &aasworld->faces[ 1 ]; i < aasworld->numfaces; i++, face++ )
	{
		if ( !optimized.removedareas[ face->backarea ] )
		{
			face->backarea = optimized.arearemap[ face->backarea ];
		}
		else
		{
			// now points to a void
			face->backarea = 0;
		}

		if ( !optimized.removedareas[ face->frontarea ] )
		{
			face->frontarea = optimized.arearemap[ face->frontarea ];
		}
		else
		{
			face->frontarea = 0;
		}
	}

	// store the areasettings
	if ( ( *aasworld ).areasettings )
	{
		FreeMemory ( ( *aasworld ).areasettings );
	}

	( *aasworld ).areasettings = optimized.areasettings;
	( *aasworld ).numareasettings = optimized.numareas;

	//
	// update nodes
	for ( i = 1; i < ( *aasworld ).numnodes; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			if ( aasworld->nodes[ i ].children[ j ] < 0 )
			{
				if ( optimized.removedareas[ -aasworld->nodes[ i ].children[ j ] ] )
				{
					aasworld->nodes[ i ].children[ j ] = 0; //make it solid
				}
				else
				{
					// remap
					aasworld->nodes[ i ].children[ j ] = -optimized.arearemap[ -aasworld->nodes[ i ].children[ j ] ];
				}
			}
		}
	}

	//
	// update portal areanums
	for ( i = 0; i < aasworld->numportals; i++ )
	{
		aasworld->portals[ i ].areanum = optimized.arearemap[ aasworld->portals[ i ].areanum ];
	}

	// update clusters and portals
	for ( i = 0; i < ( *aasworld ).numclusters; i++ )
	{
		AAS_NumberClusterAreas ( i );
	}

	// free temporary memory
	FreeMemory ( optimized.areakeep );
	FreeMemory ( optimized.arearemap );
	FreeMemory ( optimized.removedareas );
	FreeMemory ( optimized.reachabilityremap );
	//print some nice stuff :)
	botimport.Print ( PRT_MESSAGE, "%i non-grounded areas removed, %i remain.\n", removed, valid );
} //end of the function AAS_Optimize
