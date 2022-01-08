/* -------------------------------------------------------------------------------

   Copyright (C) 1999-2007 id Software, Inc. and contributors.
   For a list of contributors, see the accompanying CONTRIBUTORS file.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

   ----------------------------------------------------------------------------------

   This code has been altered significantly from its original form, to support
   several games based on the Quake III Arena engine, in the form of "Q3Map2."

   ------------------------------------------------------------------------------- */



/* marker */
#define BRUSH_C



/* dependencies */
#include <math.h>
#include "engine/qcommon/qcommon.h"
#include "common/cm/cm_polylib.h"



/* -------------------------------------------------------------------------------

   functions

   ------------------------------------------------------------------------------- */

/*
   SnapWeldVector() - ydnar
   welds two vec3_t's into a third, taking into account nearest-to-integer
   instead of averaging
 */

#define SNAP_EPSILON    0.01

void SnapWeldVector( vec3_t a, vec3_t b, vec3_t out ){
	int i;
	vec_t ai, bi, outi;


	/* dummy check */
	if ( a == NULL || b == NULL || out == NULL ) {
		return;
	}

	/* do each element */
	for ( i = 0; i < 3; i++ )
	{
		/* round to integer */
		ai = roundf( a[ i ] );
		bi = roundf( b[ i ] );

		/* prefer exact integer */
		if ( ai == a[ i ] ) {
			out[ i ] = a[ i ];
		}
		else if ( bi == b[ i ] ) {
			out[ i ] = b[ i ];
		}

		/* use nearest */
		else if ( fabs( ai - a[ i ] ) < fabs( bi - b[ i ] ) ) {
			out[ i ] = a[ i ];
		}
		else{
			out[ i ] = b[ i ];
		}

		/* snap */
		outi = roundf( out[ i ] );
		if ( fabs( outi - out[ i ] ) <= SNAP_EPSILON ) {
			out[ i ] = outi;
		}
	}
}



/*
   FixWinding() - ydnar
   removes degenerate edges from a winding
   returns qtrue if the winding is valid
 */

#define DEGENERATE_EPSILON  0.1

bool FixWinding( winding_t *w ){
	bool valid = true;
	int i, j, k;
	vec3_t vec;
	float dist;


	/* dummy check */
	if ( !w ) {
		return false;
	}

	/* check all verts */
	for ( i = 0; i < w->numpoints; i++ )
	{
		/* don't remove points if winding is a triangle */
		if ( w->numpoints == 3 ) {
			return valid;
		}

		/* get second point index */
		j = ( i + 1 ) % w->numpoints;

		/* degenerate edge? */
		VectorSubtract( w->p[ i ], w->p[ j ], vec );
		dist = VectorLength( vec );
		if ( dist < DEGENERATE_EPSILON ) {
			valid = false;
			//Sys_FPrintf( SYS_VRB, "WARNING: Degenerate winding edge found, fixing...\n" );

			/* create an average point (ydnar 2002-01-26: using nearest-integer weld preference) */
			SnapWeldVector( w->p[ i ], w->p[ j ], vec );
			VectorCopy( vec, w->p[ i ] );
			//VectorAdd( w->p[ i ], w->p[ j ], vec );
			//VectorScale( vec, 0.5, w->p[ i ] );

			/* move the remaining verts */
			for ( k = i + 2; k < w->numpoints; k++ )
			{
				VectorCopy( w->p[ k ], w->p[ k - 1 ] );
			}
			w->numpoints--;
		}
	}

	/* one last check and return */
	if ( w->numpoints < 3 ) {
		valid = false;
	}
	return valid;
}
