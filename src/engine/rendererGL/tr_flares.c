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
sources are visible.  The size of the flare reletive to the screen is nearly
constant, irrespective of distance, but the intensity should be proportional to the
projected area of the light source.

A surface that has been flagged as having a light flare will calculate the depth
buffer value that it's midpoint should have when the surface is added.

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
typedef struct flare_s
{
	struct flare_s *next;           // for active chain

	int            addedFrame;

	qboolean       inPortal;        // true if in a portal view of the scene
	int            frameSceneNum;
	void           *surface;
	int            fogNum;

	int            fadeTime;

	qboolean       visible;         // state of last test
	float          drawIntensity;   // may be non 0 even if !visible due to fading

	int            windowX, windowY;
	float          eyeZ;

	vec3_t         color;
} flare_t;

#define                 MAX_FLARES 128

flare_t r_flareStructs[ MAX_FLARES ];
flare_t *r_activeFlares, *r_inactiveFlares;

int     flareCoeff;

/*
==================
R_ClearFlares
==================
*/
void R_ClearFlares( void )
{
	int i;

	Com_Memset( r_flareStructs, 0, sizeof( r_flareStructs ) );
	r_activeFlares   = NULL;
	r_inactiveFlares = NULL;

	for ( i = 0; i < MAX_FLARES; i++ )
	{
		r_flareStructs[ i ].next = r_inactiveFlares;
		r_inactiveFlares         = &r_flareStructs[ i ];
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
	flare_t *f;       //, *oldest; //unused
	vec3_t  local;
	vec4_t  eye, clip, normalized, window;
	float   distBias = 512.0;
	float   distLerp = 0.5;
	float   d1       = 0.0f, d2 = 0.0f;

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
		return;                                 // shouldn't happen, since we check the clip[] above, except for FP rounding
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

		f                = r_inactiveFlares;
		r_inactiveFlares = r_inactiveFlares->next;
		f->next          = r_activeFlares;
		r_activeFlares   = f;

		f->surface       = surface;
		f->frameSceneNum = backEnd.viewParms.frameSceneNum;
		f->inPortal      = backEnd.viewParms.isPortal;
		f->addedFrame    = -1;
	}

	if ( f->addedFrame != backEnd.viewParms.frameCount - 1 )
	{
		f->visible  = qfalse;
		f->fadeTime = backEnd.refdef.time - 2000;
	}

	f->addedFrame = backEnd.viewParms.frameCount;
	f->fogNum     = fogNum;

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
		d2  = 1.0 - ( d2 * ( 1.0 / distBias ) );
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
		d1  = DotProduct( local, normal );
		d1 *= ( 1.0 - distLerp );
		d1 += d2;
	}

	VectorScale( f->color, d1, f->color );

	// save info needed to test
	f->windowX = backEnd.viewParms.viewportX + window[ 0 ];
	f->windowY = backEnd.viewParms.viewportY + window[ 1 ];

	f->eyeZ    = eye[ 2 ];
}

/*
==================
RB_AddLightFlares
==================
*/
void RB_AddLightFlares( void )
{
	int          i, j, k;
	trRefLight_t *l;
	fog_t        *fog;

	if ( !r_flares->integer )
	{
		return;
	}

	l   = backEnd.refdef.lights;
	fog = tr.world->fogs;

	for ( i = 0; i < backEnd.refdef.numLights; i++, l++ )
	{
		if ( !l->isStatic )
		{
			continue;
		}

		// find which fog volume the light is in
		for ( j = 1; j < tr.world->numFogs; j++ )
		{
			fog = &tr.world->fogs[ j ];

			for ( k = 0; k < 3; k++ )
			{
				if ( l->l.origin[ k ] < fog->bounds[ 0 ][ k ] || l->l.origin[ k ] > fog->bounds[ 1 ][ k ] )
				{
					break;
				}
			}

			if ( k == 3 )
			{
				break;
			}
		}

		if ( j == tr.world->numFogs )
		{
			j = 0;
		}

		RB_AddFlare( ( void * )l, j, l->l.origin, l->l.color, NULL );
	}
}

/*
===============================================================================

FLARE BACK END

===============================================================================
*/

/*
==================
RB_TestFlare
==================
*/
void RB_TestFlare( flare_t *f )
{
	float    depth;
	qboolean visible;
	float    fade;
	float    screenZ;

	backEnd.pc.c_flareTests++;

	// doing a readpixels is as good as doing a glFinish(), so
	// don't bother with another sync
	glState.finishCalled = qfalse;

	// read back the z buffer contents
	glReadPixels( f->windowX, f->windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth );

	screenZ = backEnd.viewParms.projectionMatrix[ 14 ] /
	          ( ( 2 * depth - 1 ) * backEnd.viewParms.projectionMatrix[ 11 ] - backEnd.viewParms.projectionMatrix[ 10 ] );

	visible = ( -f->eyeZ - -screenZ ) < 24;

	if ( visible )
	{
		if ( !f->visible )
		{
			f->visible  = qtrue;
			f->fadeTime = backEnd.refdef.time - 1500;
		}

		fade = ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
	}
	else
	{
		if ( f->visible )
		{
			f->visible  = qfalse;
			f->fadeTime = backEnd.refdef.time - 1;
		}

		fade = 1.0f - ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
	}

	if ( fade < 0 )
	{
		fade = 0;
	}

	if ( fade > 1 )
	{
		fade = 1;
	}

	f->drawIntensity = fade;
}

/*
==================
RB_RenderFlare
==================
*/
void RB_RenderFlare( flare_t *f )
{
	float  size;
	vec3_t color;

	backEnd.pc.c_flareRenders++;

#if 1
	//VectorScale(f->color, f->drawIntensity, color);
	VectorScale( colorWhite, f->drawIntensity, color );

	size = backEnd.viewParms.viewportWidth * ( r_flareSize->value / 640.0f + 8 / -f->eyeZ );
#else

	/*
	   As flare sizes stay nearly constant with increasing distance we must decrease the intensity
	   to achieve a reasonable visual result. The intensity is ~ (size^2 / distance^2) which can be
	   got by considering the ratio of  (flaresurface on screen) : (Surface of sphere defined by flare origin and distance from flare)
	   An important requirement is: intensity <= 1 for all distances.

	   The formula used here to compute the intensity is as follows:
	   intensity = flareCoeff * size^2 / (distance + size*sqrt(flareCoeff))^2.
	   As you can see, the intensity will have a max. of 1 when the distance is 0.

	   The coefficient flareCoeff will determine the falloff speed with increasing distance.
	 */
	float distance, intensity, factor;

	// We don't want too big values anyways when dividing by distance
	if ( f->eyeZ > -1.0f )
	{
		distance = 1.0f;
	}
	else
	{
		distance = -f->eyeZ;
	}

	// calculate the flare size
	size        = backEnd.viewParms.viewportWidth * ( r_flareSize->value / 640.0f + 8 / distance );

	factor      = distance + size * sqrt( flareCoeff );

	intensity   = flareCoeff * size * size / ( factor * factor );

	VectorScale( f->color, f->drawIntensity * intensity, color );
	iColor[ 0 ] = color[ 0 ] * 255;
	iColor[ 1 ] = color[ 1 ] * 255;
	iColor[ 2 ] = color[ 2 ] * 255;
#endif

	Tess_Begin( Tess_StageIteratorGeneric, NULL, tr.flareShader, NULL, qfalse, qfalse, -1, f->fogNum );

	// FIXME: use quadstamp?
	tess.xyz[ tess.numVertexes ][ 0 ]       = f->windowX - size;
	tess.xyz[ tess.numVertexes ][ 1 ]       = f->windowY - size;
	tess.xyz[ tess.numVertexes ][ 2 ]       = 0;
	tess.xyz[ tess.numVertexes ][ 3 ]       = 1;
	tess.texCoords[ tess.numVertexes ][ 0 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 1 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
	tess.colors[ tess.numVertexes ][ 0 ]    = color[ 0 ];
	tess.colors[ tess.numVertexes ][ 1 ]    = color[ 1 ];
	tess.colors[ tess.numVertexes ][ 2 ]    = color[ 2 ];
	tess.colors[ tess.numVertexes ][ 3 ]    = 1;
	tess.numVertexes++;

	tess.xyz[ tess.numVertexes ][ 0 ]       = f->windowX - size;
	tess.xyz[ tess.numVertexes ][ 1 ]       = f->windowY + size;
	tess.xyz[ tess.numVertexes ][ 2 ]       = 0;
	tess.xyz[ tess.numVertexes ][ 3 ]       = 1;
	tess.texCoords[ tess.numVertexes ][ 0 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 1 ] = 1;
	tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
	tess.colors[ tess.numVertexes ][ 0 ]    = color[ 0 ];
	tess.colors[ tess.numVertexes ][ 1 ]    = color[ 1 ];
	tess.colors[ tess.numVertexes ][ 2 ]    = color[ 2 ];
	tess.colors[ tess.numVertexes ][ 3 ]    = 1;
	tess.numVertexes++;

	tess.xyz[ tess.numVertexes ][ 0 ]       = f->windowX + size;
	tess.xyz[ tess.numVertexes ][ 1 ]       = f->windowY + size;
	tess.xyz[ tess.numVertexes ][ 2 ]       = 0;
	tess.xyz[ tess.numVertexes ][ 3 ]       = 1;
	tess.texCoords[ tess.numVertexes ][ 0 ] = 1;
	tess.texCoords[ tess.numVertexes ][ 1 ] = 1;
	tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
	tess.colors[ tess.numVertexes ][ 0 ]    = color[ 0 ];
	tess.colors[ tess.numVertexes ][ 1 ]    = color[ 1 ];
	tess.colors[ tess.numVertexes ][ 2 ]    = color[ 2 ];
	tess.colors[ tess.numVertexes ][ 3 ]    = 1;
	tess.numVertexes++;

	tess.xyz[ tess.numVertexes ][ 0 ]       = f->windowX + size;
	tess.xyz[ tess.numVertexes ][ 1 ]       = f->windowY - size;
	tess.xyz[ tess.numVertexes ][ 2 ]       = 0;
	tess.xyz[ tess.numVertexes ][ 3 ]       = 1;
	tess.texCoords[ tess.numVertexes ][ 0 ] = 1;
	tess.texCoords[ tess.numVertexes ][ 1 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
	tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
	tess.colors[ tess.numVertexes ][ 0 ]    = color[ 0 ];
	tess.colors[ tess.numVertexes ][ 1 ]    = color[ 1 ];
	tess.colors[ tess.numVertexes ][ 2 ]    = color[ 2 ];
	tess.colors[ tess.numVertexes ][ 3 ]    = 1;
	tess.numVertexes++;

	tess.indexes[ tess.numIndexes++ ]       = 0;
	tess.indexes[ tess.numIndexes++ ]       = 1;
	tess.indexes[ tess.numIndexes++ ]       = 2;
	tess.indexes[ tess.numIndexes++ ]       = 0;
	tess.indexes[ tess.numIndexes++ ]       = 2;
	tess.indexes[ tess.numIndexes++ ]       = 3;

	Tess_End();
}

/*
==================
RB_RenderFlares

Because flares are simulating an occular effect, they should be drawn after
everything (all views) in the entire frame has been drawn.

Because of the way portals use the depth buffer to mark off areas, the
needed information would be lost after each view, so we are forced to draw
flares after each view.

The resulting artifact is that flares in mirrors or portals don't dim properly
when occluded by something in the main view, and portal flares that should
extend past the portal edge will be overwritten.
==================
*/
void RB_RenderFlares( void )
{
	flare_t  *f;
	flare_t  **prev;
	qboolean draw;
	matrix_t ortho;

	if ( !r_flares->integer )
	{
		return;
	}

#if 0

	if ( r_flareCoeff->modified )
	{
		if ( r_flareCoeff->value == 0.0f )
		{
			flareCoeff = atof( "150" );
		}
		else
		{
			flareCoeff = r_flareCoeff->value;
		}

		r_flareCoeff->modified = qfalse;
	}

#endif

	// reset currentEntity to world so that any previously referenced entities don't have influence
	// on the rendering of these flares (i.e. RF_ renderer flags).
	backEnd.currentEntity = &tr.worldEntity;
	backEnd.orientation   = backEnd.viewParms.world;
	GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );

	if ( tr.world != NULL )         // thx Thilo
	{
		RB_AddLightFlares();
	}

	// perform z buffer readback on each flare in this view
	draw = qfalse;
	prev = &r_activeFlares;

	while ( ( f = *prev ) != NULL )
	{
		// throw out any flares that weren't added last frame
		if ( f->addedFrame < backEnd.viewParms.frameCount - 1 )
		{
			*prev            = f->next;
			f->next          = r_inactiveFlares;
			r_inactiveFlares = f;
			continue;
		}

		// don't draw any here that aren't from this scene / portal
		f->drawIntensity = 0;

		if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->inPortal == backEnd.viewParms.isPortal )
		{
			RB_TestFlare( f );

			if ( f->drawIntensity )
			{
				draw = qtrue;
			}
			else
			{
				// this flare has completely faded out, so remove it from the chain
				*prev            = f->next;
				f->next          = r_inactiveFlares;
				r_inactiveFlares = f;
				continue;
			}
		}

		prev = &f->next;
	}

	if ( !draw )
	{
		// none visible
		return;
	}

	if ( backEnd.viewParms.isPortal )
	{
		glDisable( GL_CLIP_PLANE0 );
	}

	GL_CheckErrors();

	GL_PushMatrix();
	MatrixOrthogonalProjection( ortho, backEnd.viewParms.viewportX,
	                            backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	                            backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	                            -99999, 99999 );
	GL_LoadProjectionMatrix( ortho );
	GL_LoadModelViewMatrix( matrixIdentity );

	for ( f = r_activeFlares; f; f = f->next )
	{
		if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum && f->inPortal == backEnd.viewParms.isPortal && f->drawIntensity )
		{
			RB_RenderFlare( f );
		}
	}

	GL_PopMatrix();

	GL_CheckErrors();
}
