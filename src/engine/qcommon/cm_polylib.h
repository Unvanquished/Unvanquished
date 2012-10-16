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

// this is only used for visualization tools in cm_ debug functions

typedef struct
{
	int    numpoints;
	vec3_t p[ 4 ]; // variable sized
} winding_t;

#define MAX_POINTS_ON_WINDING 64

#define CLIP_EPSILON          0.1f

#define MAX_MAP_BOUNDS        65535

// you can define on_epsilon in the makefile as tighter
#ifndef ON_EPSILON
#define ON_EPSILON 0.1f
#endif

winding_t *AllocWinding( int points );
float     WindingArea( winding_t *w );
void      WindingCenter( winding_t *w, vec3_t center );
void      ClipWindingEpsilon( winding_t *in, vec3_t normal, float dist,
                              float epsilon, winding_t **front, winding_t **back );
winding_t *ChopWinding( winding_t *in, vec3_t normal, float dist );
winding_t *CopyWinding( winding_t *w );
winding_t *ReverseWinding( winding_t *w );
winding_t *BaseWindingForPlane( vec3_t normal, float dist );
void      CheckWinding( winding_t *w );
void      WindingPlane( winding_t *w, vec3_t normal, float *dist );
void      RemoveColinearPoints( winding_t *w );
int       WindingOnPlaneSide( winding_t *w, vec3_t normal, float dist );
void      FreeWinding( winding_t *w );
void      WindingBounds( winding_t *w, vec3_t mins, vec3_t maxs );

void      AddWindingToConvexHull( winding_t *w, winding_t **hull, vec3_t normal );

void      ChopWindingInPlace( winding_t **w, vec3_t normal, float dist, float epsilon );

// frees the original if clipped

void pw( winding_t *w );
