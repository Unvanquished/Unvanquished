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

#include "engine/qcommon/q_shared.h"
#include "engine/qcommon/qfiles.h"
#include "engine/renderer/tr_types.h"

void         CM_LoadMap(Str::StringRef name);
void         CM_ClearMap();

clipHandle_t CM_InlineModel( int index );  // 0 = world, 1 + are bmodels
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int capsule );

void         CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );

int          CM_NumInlineModels();
char         *CM_EntityString();

// returns an ORed contents mask
int          CM_PointContents( const vec3_t p, clipHandle_t model );
int          CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );

void         CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end, vec3_t mins,
                          vec3_t maxs, clipHandle_t model, int brushmask, int skipmask,
                          traceType_t type );
void         CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
                                     const vec3_t mins, const vec3_t maxs, clipHandle_t model,
                                     int brushmask, int skipmask, const vec3_t origin,
                                     const vec3_t angles, traceType_t type );
void         CM_BiSphereTrace( trace_t *results, const vec3_t start,
                               const vec3_t end, float startRad, float endRad,
                               clipHandle_t model, int mask, int skipmask );
void         CM_TransformedBiSphereTrace( trace_t *results, const vec3_t start, const vec3_t end,
                                          float startRad, float endRad, clipHandle_t model,
                                          int mask, int skipmask, const vec3_t origin );

float CM_DistanceToModel( const vec3_t loc, clipHandle_t model );

byte *CM_ClusterPVS( int cluster );

int  CM_PointLeafnum( const vec3_t p );

// only returns non-solid leafs
// overflow if return listsize and if *lastLeaf != list[listsize-1]
int      CM_BoxLeafnums( const vec3_t mins, const vec3_t maxs, int *list, int listsize, int *lastLeaf );

int      CM_LeafCluster( int leafnum );
int      CM_LeafArea( int leafnum );

void     CM_AdjustAreaPortalState( int area1, int area2, bool open );
bool CM_AreasConnected( int area1, int area2 );

int      CM_WriteAreaBits( byte *buffer, int area );

// cm_marks.c
int      CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
                           int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

// cm_patch.c
void CM_DrawDebugSurface( void ( *drawPoly )( int color, int numPoints, float *points ) );
