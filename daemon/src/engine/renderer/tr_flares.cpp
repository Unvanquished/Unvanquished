/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_flares.c
#include "tr_local.h"

/*
=============================================================================
LIGHT FLARES

A light flare is an effect that takes place inside the eye when bright light
sources are visible.  The size of the flare relative to the screen is nearly
constant, irrespective of distance, but the intensity should be proportional to the
projected area of the light source.

A surface that has been flagged as having a light flare will calculate the depth
buffer value that its midpoint should have when the surface is added.

After all opaque surfaces have been rendered, the depth buffer is read back for
each flare in view.  If the point has not been obscured by a closer surface, the
flare should be drawn.

Surfaces that have a repeated texture should never be flagged as flaring, because
there will only be a single flare added at the midpoint of the polygon.

To prevent abrupt popping, the intensity of the flare is interpolated up and
down as it changes visibility.  This involves scene to scene state, unlike almost
all other aspects of the renderer, and is complicated by the fact that a single
frame may have multiple scenes.

RB_RenderFlares() will be called once per view (twice in a mirrored scene, potentially
up to five or more times in a frame with 3D status bar icons).
=============================================================================
*/

// flare states maintain visibility over multiple frames for fading layers: view, mirror, menu
struct flare_t
{
	flare_t *next; // for active chain

	int            addedFrame;

	bool       inPortal; // true if in a portal view of the scene
	int            frameSceneNum;
	void           *surface;
	int            fogNum;

	int            fadeTime;

	bool       visible; // state of last test
	float          drawIntensity; // may be non 0 even if !visible due to fading

	int            windowX, windowY;
	float          eyeZ;

	vec3_t         color;
};

#define                 MAX_FLARES 128

flare_t r_flareStructs[ MAX_FLARES ];
flare_t *r_activeFlares, *r_inactiveFlares;

/*
==================
R_ClearFlares
==================
*/
void R_ClearFlares()
{
	int i;

	Com_Memset( r_flareStructs, 0, sizeof( r_flareStructs ) );
	r_activeFlares = nullptr;
	r_inactiveFlares = nullptr;

	for ( i = 0; i < MAX_FLARES; i++ )
	{
		r_flareStructs[ i ].next = r_inactiveFlares;
		r_inactiveFlares = &r_flareStructs[ i ];
	}
}

/*
==================
RB_AddFlare

This is called at surface tesselation time
==================
*/
void RB_AddFlare( void *surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal )
{
	int     i;
	flare_t *f; //, *oldest; //unused
	vec3_t  local;
	vec4_t  eye, clip, normalized, window;
	float   distBias = 512.0;
	float   distLerp = 0.5;
	float   d1 = 0.0f, d2 = 0.0f;

	backEnd.pc.c_flareAdds++;

	// if the point is off the screen, don't bother adding it
	// calculate screen coordinates and depth
	R_TransformModelToClip( point, backEnd.orientation.modelViewMatrix, backEnd.viewParms.projectionMatrix, eye, clip );

	// check to see if the point is completely off screen
	for ( i = 0; i < 3; i++ )
	{
		if ( clip[ i ] >= clip[ 3 ] || clip[ i ] <= -clip[ 3 ] )
		{
			return;
		}
	}

	R_TransformClipToWindow( clip, &backEnd.viewParms, normalized, window );

	if ( window[ 0 ] < 0 || window[ 0 ] >= backEnd.viewParms.viewportWidth ||
	     window[ 1 ] < 0 || window[ 1 ] >= backEnd.viewParms.viewportHeight )
	{
		return; // shouldn't happen, since we check the clip[] above, except for FP rounding
	}

	// see if a flare with a matching surface, scene, and view exists
	//oldest = r_flareStructs;
	for ( f = r_activeFlares; f; f = f->next )
	{
		if ( f->surface == surface && f->frameSceneNum == backEnd.viewParms.frameSceneNum &&
		     f->inPortal == backEnd.viewParms.isPortal )
		{
			break;
		}
	}

	// allocate a new one
	if ( !f )
	{
		if ( !r_inactiveFlares )
		{
			// the list is completely full
			return;
		}

		f = r_inactiveFlares;
		r_inactiveFlares = r_inactiveFlares->next;
		f->next = r_activeFlares;
		r_activeFlares = f;

		f->surface = surface;
		f->frameSceneNum = backEnd.viewParms.frameSceneNum;
		f->inPortal = backEnd.viewParms.isPortal;
		f->addedFrame = -1;
	}

	if ( f->addedFrame != backEnd.viewParms.frameCount - 1 )
	{
		f->visible = false;
		f->fadeTime = backEnd.refdef.time - 2000;
	}

	f->addedFrame = backEnd.viewParms.frameCount;
	f->fogNum = fogNum;

	VectorCopy( color, f->color );

	d2 = 0.0;
	d2 = -eye[ 2 ];

	if ( d2 > distBias )
	{
		if ( d2 > ( distBias * 2.0 ) )
		{
			d2 = ( distBias * 2.0 );
		}

		d2 -= distBias;
		d2 = 1.0 - ( d2 * ( 1.0 / distBias ) );
		d2 *= distLerp;
	}
	else
	{
		d2 = distLerp;
	}

	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	if ( normal )
	{
		VectorSubtract( backEnd.viewParms.orientation.origin, point, local );
		VectorNormalize( local );
		d1 = DotProduct( local, normal );
		d1 *= ( 1.0 - distLerp );
		d1 += d2;
	}

	VectorScale( f->color, d1, f->color );

	// save info needed to test
	f->windowX = backEnd.viewParms.viewportX + window[ 0 ];
	f->windowY = backEnd.viewParms.viewportY + window[ 1 ];

	f->eyeZ = eye[ 2 ];
}
