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

#include "tr_local.h"

backEndData_t  *backEndData[ SMP_FRAMES ];
backEndState_t backEnd;

static const float   s_flipMatrix[ 16 ] =
{
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0,  0, -1, 0,
	-1, 0, 0,  0,
	0,  1, 0,  0,
	0,  0, 0,  1
};

/*
** GL_Bind
*/
void GL_Bind( image_t *image )
{
	int texnum;

	if ( !image )
	{
		ri.Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		texnum = tr.defaultImage->texnum;
	}
	else
	{
		texnum = image->texnum;
	}

	if ( r_nobind->integer && tr.dlightImage )
	{
		// performance evaluation option
		texnum = tr.dlightImage->texnum;
	}

	if ( glState.currenttextures[ glState.currenttmu ] != texnum )
	{
		image->frameUsed = tr.frameCount;
		glState.currenttextures[ glState.currenttmu ] = texnum;
		glBindTexture( GL_TEXTURE_2D, texnum );
	}
}

/*
** GL_SelectTexture
*/
void GL_SelectTexture( int unit )
{
	if ( glState.currenttmu == unit )
	{
		return;
	}

	if ( unit == 0 )
	{
		glActiveTextureARB( GL_TEXTURE0_ARB );
		GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE0_ARB )\n" );
		glClientActiveTextureARB( GL_TEXTURE0_ARB );
		GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE0_ARB )\n" );
	}
	else if ( unit == 1 )
	{
		glActiveTextureARB( GL_TEXTURE1_ARB );
		GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE1_ARB )\n" );
		glClientActiveTextureARB( GL_TEXTURE1_ARB );
		GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE1_ARB )\n" );
	}
	else
	{
		ri.Error( ERR_DROP, "GL_SelectTexture: unit = %i", unit );
	}

	glState.currenttmu = unit;
}

/*
** GL_BindMultitexture
*/
void GL_BindMultitexture( image_t *image0, GLuint env0, image_t *image1, GLuint env1 )
{
	int texnum0, texnum1;

	texnum0 = image0->texnum;
	texnum1 = image1->texnum;

	if ( r_nobind->integer && tr.dlightImage )
	{
		// performance evaluation option
		texnum0 = texnum1 = tr.dlightImage->texnum;
	}

	if ( glState.currenttextures[ 1 ] != texnum1 )
	{
		GL_SelectTexture( 1 );
		image1->frameUsed = tr.frameCount;
		glState.currenttextures[ 1 ] = texnum1;
		glBindTexture( GL_TEXTURE_2D, texnum1 );
	}

	if ( glState.currenttextures[ 0 ] != texnum0 )
	{
		GL_SelectTexture( 0 );
		image0->frameUsed = tr.frameCount;
		glState.currenttextures[ 0 ] = texnum0;
		glBindTexture( GL_TEXTURE_2D, texnum0 );
	}
}

/*
** GL_ColorMask
*/
void GL_ColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha )
{
	if ( glState.colorMaskRed != red || glState.colorMaskGreen != green || glState.colorMaskBlue != blue || glState.colorMaskAlpha != alpha )
	{
		glState.colorMaskRed = red;
		glState.colorMaskGreen = green;
		glState.colorMaskBlue = blue;
		glState.colorMaskAlpha = alpha;

		glColorMask( red, green, blue, alpha );
	}
}

/*
** GL_Cull
*/
void GL_Cull( int cullType )
{
	if ( glState.faceCulling == cullType )
	{
		return;
	}

	glState.faceCulling = cullType;

	if ( cullType == CT_TWO_SIDED )
	{
		glDisable( GL_CULL_FACE );
	}
	else
	{
		glEnable( GL_CULL_FACE );

		if ( cullType == CT_BACK_SIDED )
		{
			if ( backEnd.viewParms.isMirror )
			{
				glCullFace( GL_FRONT );
			}
			else
			{
				glCullFace( GL_BACK );
			}
		}
		else
		{
			if ( backEnd.viewParms.isMirror )
			{
				glCullFace( GL_BACK );
			}
			else
			{
				glCullFace( GL_FRONT );
			}
		}
	}
}

/*
** GL_TexEnv
*/
void GL_TexEnv( int env )
{
	if ( env == glState.texEnv[ glState.currenttmu ] )
	{
		return;
	}

	glState.texEnv[ glState.currenttmu ] = env;

	switch ( env )
	{
		case GL_MODULATE:
			glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
			break;

		case GL_REPLACE:
			glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
			break;

		case GL_DECAL:
			glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
			break;

		case GL_ADD:
			glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );
			break;

		default:
			ri.Error( ERR_DROP, "GL_TexEnv: invalid env '%d' passed", env );
	}
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State( unsigned long stateBits )
{
	unsigned long diff = stateBits ^ glState.glStateBits;

	if ( !diff )
	{
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_EQUAL )
	{
		if ( stateBits & GLS_DEPTHFUNC_EQUAL )
		{
			glDepthFunc( GL_EQUAL );
		}
		else
		{
			glDepthFunc( GL_LEQUAL );
		}
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		GLenum srcFactor, dstFactor;

		if ( stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
		{
			switch ( stateBits & GLS_SRCBLEND_BITS )
			{
				case GLS_SRCBLEND_ZERO:
					srcFactor = GL_ZERO;
					break;

				case GLS_SRCBLEND_ONE:
					srcFactor = GL_ONE;
					break;

				case GLS_SRCBLEND_DST_COLOR:
					srcFactor = GL_DST_COLOR;
					break;

				case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
					srcFactor = GL_ONE_MINUS_DST_COLOR;
					break;

				case GLS_SRCBLEND_SRC_ALPHA:
					srcFactor = GL_SRC_ALPHA;
					break;

				case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
					srcFactor = GL_ONE_MINUS_SRC_ALPHA;
					break;

				case GLS_SRCBLEND_DST_ALPHA:
					srcFactor = GL_DST_ALPHA;
					break;

				case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
					srcFactor = GL_ONE_MINUS_DST_ALPHA;
					break;

				case GLS_SRCBLEND_ALPHA_SATURATE:
					srcFactor = GL_SRC_ALPHA_SATURATE;
					break;

				default:
					srcFactor = GL_ONE; // to get warning to shut up
					ri.Error( ERR_DROP, "GL_State: invalid src blend state bits" );
			}

			switch ( stateBits & GLS_DSTBLEND_BITS )
			{
				case GLS_DSTBLEND_ZERO:
					dstFactor = GL_ZERO;
					break;

				case GLS_DSTBLEND_ONE:
					dstFactor = GL_ONE;
					break;

				case GLS_DSTBLEND_SRC_COLOR:
					dstFactor = GL_SRC_COLOR;
					break;

				case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
					dstFactor = GL_ONE_MINUS_SRC_COLOR;
					break;

				case GLS_DSTBLEND_SRC_ALPHA:
					dstFactor = GL_SRC_ALPHA;
					break;

				case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
					dstFactor = GL_ONE_MINUS_SRC_ALPHA;
					break;

				case GLS_DSTBLEND_DST_ALPHA:
					dstFactor = GL_DST_ALPHA;
					break;

				case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
					dstFactor = GL_ONE_MINUS_DST_ALPHA;
					break;

				default:
					dstFactor = GL_ONE; // to get warning to shut up
					ri.Error( ERR_DROP, "GL_State: invalid dst blend state bits" );
			}

			glEnable( GL_BLEND );
			glBlendFunc( srcFactor, dstFactor );
		}
		else
		{
			glDisable( GL_BLEND );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK_TRUE )
	{
		if ( stateBits & GLS_DEPTHMASK_TRUE )
		{
			glDepthMask( GL_TRUE );
		}
		else
		{
			glDepthMask( GL_FALSE );
		}
	}

	// check colormask
	if ( diff & GLS_COLORMASK_BITS )
	{
		GL_ColorMask( ( stateBits & GLS_COLORMASK_RED ) ? GL_FALSE : GL_TRUE,
			      ( stateBits & GLS_COLORMASK_GREEN ) ? GL_FALSE : GL_TRUE,
			      ( stateBits & GLS_COLORMASK_BLUE ) ? GL_FALSE : GL_TRUE,
			      ( stateBits & GLS_COLORMASK_ALPHA ) ? GL_FALSE : GL_TRUE );
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE )
	{
		if ( stateBits & GLS_POLYMODE_LINE )
		{
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// depthtest
	//
	if ( diff & GLS_DEPTHTEST_DISABLE )
	{
		if ( stateBits & GLS_DEPTHTEST_DISABLE )
		{
			glDisable( GL_DEPTH_TEST );
		}
		else
		{
			glEnable( GL_DEPTH_TEST );
		}
	}

	//
	// alpha test
	//
	if ( diff & GLS_ATEST_BITS )
	{
		switch ( stateBits & GLS_ATEST_BITS )
		{
			case 0:
				glDisable( GL_ALPHA_TEST );
				break;

			case GLS_ATEST_GT_0:
				glEnable( GL_ALPHA_TEST );
				glAlphaFunc( GL_GREATER, 0.0f );
				break;

			case GLS_ATEST_LT_80:
				glEnable( GL_ALPHA_TEST );
				glAlphaFunc( GL_LESS, 0.5f );
				break;

			case GLS_ATEST_GE_80:
				glEnable( GL_ALPHA_TEST );
				glAlphaFunc( GL_GEQUAL, 0.5f );
				break;

			case GLS_ATEST_LT_ENT:
				glEnable( GL_ALPHA_TEST );
				glAlphaFunc( GL_LESS, backEnd.currentEntity->e.shaderRGBA[3] * (1.0f / 255.0f) );
				break;

			case GLS_ATEST_GT_ENT:
				glEnable( GL_ALPHA_TEST );
				glAlphaFunc( GL_GREATER, backEnd.currentEntity->e.shaderRGBA[3] * (1.0f / 255.0f) );
				break;

			default:
				assert( 0 );
				break;
		}
	}

	glState.glStateBits = stateBits;
}

/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void )
{
	float c;

	if ( !backEnd.isHyperspace )
	{
		// do initialization shit
	}

	c = ( backEnd.refdef.time & 255 ) / 255.0f;
	glClearColor( c, c, c, 1 );
	glClear( GL_COLOR_BUFFER_BIT );

	backEnd.isHyperspace = qtrue;
}

static void SetViewportAndScissor( void )
{
	float	mat[16], scale;
	vec4_t	q, c;

	Com_Memcpy( mat, backEnd.viewParms.projectionMatrix, sizeof(mat) );
	if( backEnd.viewParms.isPortal ) {
		c[0] = -DotProduct( backEnd.viewParms.portalPlane.normal, backEnd.viewParms.orientation.axis[1] );
		c[1] = DotProduct( backEnd.viewParms.portalPlane.normal, backEnd.viewParms.orientation.axis[2] );
		c[2] = -DotProduct( backEnd.viewParms.portalPlane.normal, backEnd.viewParms.orientation.axis[0] );
		c[3] = DotProduct( backEnd.viewParms.portalPlane.normal, backEnd.viewParms.orientation.origin ) - backEnd.viewParms.portalPlane.dist;

		q[0] = (c[0] < 0.0f ? -1.0f : 1.0f) / mat[0];
		q[1] = (c[1] < 0.0f ? -1.0f : 1.0f) / mat[5];
		q[2] = -1.0f;
		q[3] = (1.0f + mat[10]) / mat[14];

		scale = 2.0f / (DotProduct( c, q ) + c[3] * q[3]);
		mat[2]  = c[0] * scale;
		mat[6]  = c[1] * scale;
		mat[10] = c[2] * scale + 1.0f;
		mat[14] = c[3] * scale;
	}
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixf( mat );
	glMatrixMode( GL_MODELVIEW );

	// set the window clipping
	glViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	glScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
	           backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView( void )
{
	int clearBits = 0;

	// sync with gl if needed
	if ( r_finish->integer == 1 && !glState.finishCalled )
	{
		glFinish();
		glState.finishCalled = qtrue;
	}

	if ( r_finish->integer == 0 )
	{
		glState.finishCalled = qtrue;
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

////////// (SA) modified to ensure one glclear() per frame at most

	// clear relevant buffers
	clearBits = 0;

	clearBits |= GL_STENCIL_BUFFER_BIT;

//  if(r_uiFullScreen->integer) {
//      clearBits = GL_DEPTH_BUFFER_BIT;    // (SA) always just clear depth for menus
//  } else
	// ydnar: global q3 fog volume
	if ( tr.world && tr.world->globalFog >= 0 )
	{
		clearBits |= GL_DEPTH_BUFFER_BIT;
		clearBits |= GL_COLOR_BUFFER_BIT;
		//
		glClearColor( tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 0 ] * tr.identityLight,
		              tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 1 ] * tr.identityLight,
		              tr.world->fogs[ tr.world->globalFog ].shader->fogParms.color[ 2 ] * tr.identityLight, 1.0 );
	}
	else if ( skyboxportal )
	{
		if ( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL )
		{
			// portal scene, clear whatever is necessary
			clearBits |= GL_DEPTH_BUFFER_BIT;

			if ( r_fastsky->integer || backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
			{
				// fastsky: clear color
				// try clearing first with the portal sky fog color, then the world fog color, then finally a default
				clearBits |= GL_COLOR_BUFFER_BIT;

				if ( glfogsettings[ FOG_PORTALVIEW ].registered )
				{
					glClearColor( glfogsettings[ FOG_PORTALVIEW ].color[ 0 ], glfogsettings[ FOG_PORTALVIEW ].color[ 1 ],
					              glfogsettings[ FOG_PORTALVIEW ].color[ 2 ], glfogsettings[ FOG_PORTALVIEW ].color[ 3 ] );
				}
				else if ( glfogNum > FOG_NONE && glfogsettings[ FOG_CURRENT ].registered )
				{
					glClearColor( glfogsettings[ FOG_CURRENT ].color[ 0 ], glfogsettings[ FOG_CURRENT ].color[ 1 ],
					              glfogsettings[ FOG_CURRENT ].color[ 2 ], glfogsettings[ FOG_CURRENT ].color[ 3 ] );
				}
				else
				{
//                  glClearColor ( 1.0, 0.0, 0.0, 1.0 );   // red clear for testing portal sky clear
					glClearColor( 0.5, 0.5, 0.5, 1.0 );
				}
			}
			else
			{
				// rendered sky (either clear color or draw quake sky)
				if ( glfogsettings[ FOG_PORTALVIEW ].registered )
				{
					glClearColor( glfogsettings[ FOG_PORTALVIEW ].color[ 0 ], glfogsettings[ FOG_PORTALVIEW ].color[ 1 ],
					              glfogsettings[ FOG_PORTALVIEW ].color[ 2 ], glfogsettings[ FOG_PORTALVIEW ].color[ 3 ] );

					if ( glfogsettings[ FOG_PORTALVIEW ].clearscreen )
					{
						// portal fog requests a screen clear (distance fog rather than quake sky)
						clearBits |= GL_COLOR_BUFFER_BIT;
					}
				}
			}
		}
		else
		{
			// world scene with portal sky, don't clear any buffers, just set the fog color if there is one
			clearBits |= GL_DEPTH_BUFFER_BIT; // this will go when I get the portal sky rendering way out in the zbuffer (or not writing to zbuffer at all)

			if ( glfogNum > FOG_NONE && glfogsettings[ FOG_CURRENT ].registered )
			{
				if ( backEnd.refdef.rdflags & RDF_UNDERWATER )
				{
					if ( glfogsettings[ FOG_CURRENT ].mode == GL_LINEAR )
					{
						clearBits |= GL_COLOR_BUFFER_BIT;
					}
				}
				else if ( !( r_portalsky->integer ) )
				{
					// portal skies have been manually turned off, clear bg color
					clearBits |= GL_COLOR_BUFFER_BIT;
				}

				glClearColor( glfogsettings[ FOG_CURRENT ].color[ 0 ], glfogsettings[ FOG_CURRENT ].color[ 1 ],
				              glfogsettings[ FOG_CURRENT ].color[ 2 ], glfogsettings[ FOG_CURRENT ].color[ 3 ] );
			}
			else if ( !( r_portalsky->integer ) )
			{
				// ydnar: portal skies have been manually turned off, clear bg color
				clearBits |= GL_COLOR_BUFFER_BIT;
				glClearColor( 0.5, 0.5, 0.5, 1.0 );
			}
		}
	}
	else
	{
		// world scene with no portal sky
		clearBits |= GL_DEPTH_BUFFER_BIT;

		// NERVE - SMF - we don't want to clear the buffer when no world model is specified
		if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
		{
			clearBits &= ~GL_COLOR_BUFFER_BIT;
		}
		// -NERVE - SMF
		else if ( r_fastsky->integer || backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
		{
			clearBits |= GL_COLOR_BUFFER_BIT;

			if ( glfogsettings[ FOG_CURRENT ].registered )
			{
				// try to clear fastsky with current fog color
				glClearColor( glfogsettings[ FOG_CURRENT ].color[ 0 ], glfogsettings[ FOG_CURRENT ].color[ 1 ],
				              glfogsettings[ FOG_CURRENT ].color[ 2 ], glfogsettings[ FOG_CURRENT ].color[ 3 ] );
			}
			else
			{
//              glClearColor ( 0.0, 0.0, 1.0, 1.0 );   // blue clear for testing world sky clear
				glClearColor( 0.05, 0.05, 0.05, 1.0 );  // JPW NERVE changed per id req was 0.5s
			}
		}
		else
		{
			// world scene, no portal sky, not fastsky, clear color if fog says to, otherwise, just set the clearcolor
			if ( glfogsettings[ FOG_CURRENT ].registered )
			{
				// try to clear fastsky with current fog color
				glClearColor( glfogsettings[ FOG_CURRENT ].color[ 0 ], glfogsettings[ FOG_CURRENT ].color[ 1 ],
				              glfogsettings[ FOG_CURRENT ].color[ 2 ], glfogsettings[ FOG_CURRENT ].color[ 3 ] );

				if ( glfogsettings[ FOG_CURRENT ].clearscreen )
				{
					// world fog requests a screen clear (distance fog rather than quake sky)
					clearBits |= GL_COLOR_BUFFER_BIT;
				}
			}
		}
	}

	// ydnar: don't clear the color buffer when no world model is specified
	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		clearBits &= ~GL_COLOR_BUFFER_BIT;
	}

	if ( clearBits )
	{
		glClear( clearBits );
	}

//----(SA)  done

	if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}

	glState.faceCulling = -1; // force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;
}

#define MAC_EVENT_PUMP_MSEC 5

/*
==================
RB_RenderDrawSurfList
==================
*/
void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs )
{
	shader_t   *shader, *oldShader;
	int        fogNum, oldFogNum;
	int        entityNum, oldEntityNum;
	int        frontFace;
	int        dlighted, oldDlighted;
	qboolean   depthRange, oldDepthRange;
	int        i;
	drawSurf_t *drawSurf;
	int        oldSort;
	float      originalTime;

#ifdef __MACOS__
	int        macEventTime;

	Sys_PumpEvents(); // crutch up the mac's limited buffer queue size

	// we don't want to pump the event loop too often and waste time, so
	// we are going to check every shader change
	macEventTime = ri.Milliseconds() + MAC_EVENT_PUMP_MSEC;
#endif

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView();

	// draw everything
	oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
	oldFogNum = -1;
	oldDepthRange = qfalse;
	oldDlighted = qfalse;
	oldSort = -1;
	depthRange = qfalse;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for ( i = 0, drawSurf = drawSurfs; i < numDrawSurfs; i++, drawSurf++ )
	{
		if ( drawSurf->sort == oldSort )
		{
			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}

		oldSort = drawSurf->sort;
		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &frontFace, &dlighted );

		//
		// change the tess parameters if needed
		// an "entityMergable" shader is a shader that can have surfaces from separate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted
		     || ( entityNum != oldEntityNum && !shader->entityMergable ) )
		{
			if ( oldShader != NULL )
			{
#ifdef __MACOS__ // crutch up the mac's limited buffer queue size
				int t;

				t = ri.Milliseconds();

				if ( t > macEventTime )
				{
					macEventTime = t + MAC_EVENT_PUMP_MSEC;
					Sys_PumpEvents();
				}

#endif
				RB_EndSurface();
			}

			RB_BeginSurface( shader, fogNum );
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
		}

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum )
		{
			depthRange = qfalse;

			if ( entityNum != ENTITYNUM_WORLD )
			{
				backEnd.currentEntity = &backEnd.refdef.entities[ entityNum ];
				backEnd.refdef.floatTime = originalTime; // - backEnd.currentEntity->e.shaderTime; // JPW NERVE pulled this to match q3ta

				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
//              tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation );

				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->needDlights )
				{
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation );
				}

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK )
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			}
			else
			{
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.orientation = backEnd.viewParms.world;

				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
//              tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation );
			}

			glLoadMatrixf( backEnd.orientation.modelMatrix );

			//
			// change depthrange if needed
			//
			if ( oldDepthRange != depthRange )
			{
				if ( depthRange )
				{
					glDepthRange( 0, 0.3 );
				}
				else
				{
					glDepthRange( 0, 1 );
				}

				oldDepthRange = depthRange;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	// draw the contents of the last shader batch
	if ( oldShader != NULL )
	{
		RB_EndSurface();
	}

	// go back to the world modelview matrix
	backEnd.currentEntity = &tr.worldEntity;
	backEnd.refdef.floatTime = originalTime;
	backEnd.orientation = backEnd.viewParms.world;
	R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.orientation );

	glLoadMatrixf( backEnd.viewParms.world.modelMatrix );

	if ( depthRange )
	{
		glDepthRange( 0, 1 );
	}

	// (SA) draw sun
	RB_DrawSun();

	// darken down any stencil shadows
	RB_ShadowFinish();

	// add light flares on lights that aren't obscured
	RB_RenderFlares();

#ifdef __MACOS__
	Sys_PumpEvents(); // crutch up the mac's limited buffer queue size
#endif
}

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
================
RB_SetGL2D

================
*/
void RB_SetGL2D( void )
{
	backEnd.projection2D = qtrue;

	// set 2D virtual screen size
	glViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	glScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	glDisable( GL_CULL_FACE );

	// set time for 2D shaders
	backEnd.refdef.time = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}

/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw( int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty )
{
	int i, j;
	int start, end;

	if ( !tr.registered )
	{
		return;
	}

	R_SyncRenderThread();

	// we definitely want to sync every frame for the cinematics
	glFinish();

	start = end = 0;

	if ( r_speeds->integer )
	{
		start = ri.Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0; ( 1 << i ) < cols; i++ )
	{
	}

	for ( j = 0; ( 1 << j ) < rows; j++ )
	{
	}

	if ( ( 1 << i ) != cols || ( 1 << j ) != rows )
	{
		ri.Error( ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows );
	}

	GL_Bind( tr.scratchImage[ client ] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[ client ]->width || rows != tr.scratchImage[ client ]->height )
	{
		tr.scratchImage[ client ]->width = tr.scratchImage[ client ]->uploadWidth = cols;
		tr.scratchImage[ client ]->height = tr.scratchImage[ client ]->uploadHeight = rows;
		glTexImage2D( GL_TEXTURE_2D, 0, 3, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	}
	else
	{
		if ( dirty )
		{
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}

	if ( r_speeds->integer )
	{
		end = ri.Milliseconds();
		ri.Printf( PRINT_DEVELOPER, "glTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	RB_SetGL2D();

	glColor3f( tr.identityLight, tr.identityLight, tr.identityLight );

	glBegin( GL_QUADS );
	glTexCoord2f( 0.5f / cols, 0.5f / rows );
	glVertex2f( x, y );
	glTexCoord2f( ( cols - 0.5f ) / cols, 0.5f / rows );
	glVertex2f( x + w, y );
	glTexCoord2f( ( cols - 0.5f ) / cols, ( rows - 0.5f ) / rows );
	glVertex2f( x + w, y + h );
	glTexCoord2f( 0.5f / cols, ( rows - 0.5f ) / rows );
	glVertex2f( x, y + h );
	glEnd();
}

void RE_UploadCinematic( int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty )
{
	GL_Bind( tr.scratchImage[ client ] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[ client ]->width || rows != tr.scratchImage[ client ]->height )
	{
		tr.scratchImage[ client ]->width = tr.scratchImage[ client ]->uploadWidth = cols;
		tr.scratchImage[ client ]->height = tr.scratchImage[ client ]->uploadHeight = rows;
		glTexImage2D( GL_TEXTURE_2D, 0, 3, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	}
	else
	{
		if ( dirty )
		{
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}
}

/*
=============
RB_SetColor

=============
*/
const void     *RB_SetColor( const void *data )
{
	const setColorCommand_t *cmd;

	cmd = ( const setColorCommand_t * ) data;

	backEnd.color2D[ 0 ] = cmd->color[ 0 ] * 255;
	backEnd.color2D[ 1 ] = cmd->color[ 1 ] * 255;
	backEnd.color2D[ 2 ] = cmd->color[ 2 ] * 255;
	backEnd.color2D[ 3 ] = cmd->color[ 3 ] * 255;

	return ( const void * )( cmd + 1 );
}

/*
=============
RB_StretchPic
=============
*/
const void     *RB_StretchPic( const void *data )
{
	const stretchPicCommand_t *cmd;
	shader_t                  *shader;
	int                       numVerts, numIndexes;

	cmd = ( const stretchPicCommand_t * ) data;

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;

	if ( shader != tess.shader )
	{
		if ( tess.numIndexes )
		{
			RB_EndSurface();
		}

		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	* ( int * ) tess.vertexColors[ numVerts ].v =
	  * ( int * ) tess.vertexColors[ numVerts + 1 ].v =
	    * ( int * ) tess.vertexColors[ numVerts + 2 ].v = * ( int * ) tess.vertexColors[ numVerts + 3 ].v = * ( int * ) backEnd.color2D;

	tess.xyz[ numVerts ].v[ 0 ] = cmd->x;
	tess.xyz[ numVerts ].v[ 1 ] = cmd->y;
	tess.xyz[ numVerts ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts ].v[ 0 ] = cmd->s1;
	tess.texCoords0[ numVerts ].v[ 1 ] = cmd->t1;

	tess.xyz[ numVerts + 1 ].v[ 0 ] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ].v[ 1 ] = cmd->y;
	tess.xyz[ numVerts + 1 ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts + 1 ].v[ 0 ] = cmd->s2;
	tess.texCoords0[ numVerts + 1 ].v[ 1 ] = cmd->t1;

	tess.xyz[ numVerts + 2 ].v[ 0 ] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ].v[ 1 ] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts + 2 ].v[ 0 ] = cmd->s2;
	tess.texCoords0[ numVerts + 2 ].v[ 1 ] = cmd->t2;

	tess.xyz[ numVerts + 3 ].v[ 0 ] = cmd->x;
	tess.xyz[ numVerts + 3 ].v[ 1 ] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts + 3 ].v[ 0 ] = cmd->s1;
	tess.texCoords0[ numVerts + 3 ].v[ 1 ] = cmd->t2;

	return ( const void * )( cmd + 1 );
}

const void     *RB_Draw2dPolys( const void *data )
{
	const poly2dCommand_t *cmd;
	shader_t              *shader;
	int                   i;

	cmd = ( const poly2dCommand_t * ) data;

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;

	if ( shader != tess.shader )
	{
		if ( tess.numIndexes )
		{
			RB_EndSurface();
		}

		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( cmd->numverts, ( cmd->numverts - 2 ) * 3 );

	for ( i = 0; i < cmd->numverts - 2; i++ )
	{
		tess.indexes[ tess.numIndexes + 0 ] = tess.numVertexes;
		tess.indexes[ tess.numIndexes + 1 ] = tess.numVertexes + i + 1;
		tess.indexes[ tess.numIndexes + 2 ] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	for ( i = 0; i < cmd->numverts; i++ )
	{
		tess.xyz[ tess.numVertexes ].v[ 0 ] = cmd->verts[ i ].xyz[ 0 ];
		tess.xyz[ tess.numVertexes ].v[ 1 ] = cmd->verts[ i ].xyz[ 1 ];
		tess.xyz[ tess.numVertexes ].v[ 2 ] = 0;

		tess.texCoords0[ tess.numVertexes ].v[ 0 ] = cmd->verts[ i ].st[ 0 ];
		tess.texCoords0[ tess.numVertexes ].v[ 1 ] = cmd->verts[ i ].st[ 1 ];

		tess.vertexColors[ tess.numVertexes ].v[ 0 ] = cmd->verts[ i ].modulate[ 0 ];
		tess.vertexColors[ tess.numVertexes ].v[ 1 ] = cmd->verts[ i ].modulate[ 1 ];
		tess.vertexColors[ tess.numVertexes ].v[ 2 ] = cmd->verts[ i ].modulate[ 2 ];
		tess.vertexColors[ tess.numVertexes ].v[ 3 ] = cmd->verts[ i ].modulate[ 3 ];
		tess.numVertexes++;
	}

	return ( const void * )( cmd + 1 );
}

const void     *RB_ScissorEnable( const void *data )
{
	const scissorEnableCommand_t *cmd;

	cmd = ( const scissorEnableCommand_t * ) data;

	tr.scissor.status = cmd->enable;

	if ( !cmd->enable )
	{
		glScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	}
	else
	{
		glScissor( tr.scissor.x, tr.scissor.y, tr.scissor.w, tr.scissor.h );
	}

	return ( const void * )( cmd + 1 );
}


const void     *RB_ScissorSet( const void *data )
{
	const scissorSetCommand_t *cmd;

	cmd = ( const scissorSetCommand_t * ) data;

	if ( tr.scissor.x == cmd->x && tr.scissor.y == cmd->y && tr.scissor.w == cmd->w && tr.scissor.h == cmd->h )
	{
		return ( const void * )( cmd + 1 );
	}

	tr.scissor.x = cmd->x;
	tr.scissor.y = cmd->y;
	tr.scissor.w = cmd->w;
	tr.scissor.h = cmd->h;

	glScissor( cmd->x, cmd->y, cmd->w, cmd->h );

	return ( const void * )( cmd + 1 );
}

const void     *RB_Draw2dPolysIndexed( const void *data )
{
	const poly2dIndexedCommand_t *cmd;
	shader_t              *shader;
	int                   i;

	cmd = ( const poly2dIndexedCommand_t * ) data;

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;

	if ( shader != tess.shader )
	{
		if ( tess.numIndexes )
		{
			RB_EndSurface();
		}

		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( cmd->numverts, cmd->numIndexes );

	for ( i = 0; i < cmd->numIndexes; i++ )
	{
		tess.indexes[ tess.numIndexes + i ] = tess.numVertexes + cmd->indexes[ i ];
	}
	tess.numIndexes += cmd->numIndexes;

	for ( i = 0; i < cmd->numverts; i++ )
	{
		tess.xyz[ tess.numVertexes ].v[ 0 ] = cmd->verts[ i ].xyz[ 0 ] + cmd->translation[ 0 ];
		tess.xyz[ tess.numVertexes ].v[ 1 ] = cmd->verts[ i ].xyz[ 1 ] + cmd->translation[ 1 ];
		tess.xyz[ tess.numVertexes ].v[ 2 ] = 0;
		tess.xyz[ tess.numVertexes ].v[ 3 ] = 1;

		tess.texCoords0[ tess.numVertexes ].v[ 0 ] = cmd->verts[ i ].st[ 0 ];
		tess.texCoords0[ tess.numVertexes ].v[ 1 ] = cmd->verts[ i ].st[ 1 ];

		tess.vertexColors[ tess.numVertexes ].v[ 0 ] = cmd->verts[ i ].modulate[ 0 ];
		tess.vertexColors[ tess.numVertexes ].v[ 1 ] = cmd->verts[ i ].modulate[ 1 ];
		tess.vertexColors[ tess.numVertexes ].v[ 2 ] = cmd->verts[ i ].modulate[ 2 ];
		tess.vertexColors[ tess.numVertexes ].v[ 3 ] = cmd->verts[ i ].modulate[ 3 ];
		tess.numVertexes++;
	}

	RB_EndSurface();

	return ( const void * )( cmd + 1 );
}

// NERVE - SMF

/*
=============
RB_RotatedPic
=============
*/
const void     *RB_RotatedPic( const void *data )
{
	const stretchPicCommand_t *cmd;
	shader_t                  *shader;
	int                       numVerts, numIndexes;
	float                     mx, my, cosA, sinA, cw, ch, sw, sh;

	cmd = ( const stretchPicCommand_t * ) data;

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;

	if ( shader != tess.shader )
	{
		if ( tess.numIndexes )
		{
			RB_EndSurface();
		}

		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	* ( int * ) tess.vertexColors[ numVerts ].v =
	  * ( int * ) tess.vertexColors[ numVerts + 1 ].v =
	    * ( int * ) tess.vertexColors[ numVerts + 2 ].v = * ( int * ) tess.vertexColors[ numVerts + 3 ].v = * ( int * ) backEnd.color2D;

	mx = cmd->x + ( cmd->w / 2 );
	my = cmd->y + ( cmd->h / 2 );
	cosA = cos( DEG2RAD( cmd->angle ) );
	sinA = sin( DEG2RAD( cmd->angle ) );
	cw = cosA * ( cmd->w / 2 );
	ch = cosA * ( cmd->h / 2 );
	sw = sinA * ( cmd->w / 2 );
	sh = sinA * ( cmd->h / 2 );

	tess.xyz[ numVerts ].v[ 0 ] = mx - cw - sh;
	tess.xyz[ numVerts ].v[ 1 ] = my + sw - ch;
	tess.xyz[ numVerts ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts ].v[ 0 ] = cmd->s1;
	tess.texCoords0[ numVerts ].v[ 1 ] = cmd->t1;

	tess.xyz[ numVerts + 1 ].v[ 0 ] = mx + cw - sh;
	tess.xyz[ numVerts + 1 ].v[ 1 ] = my - sw - ch;
	tess.xyz[ numVerts + 1 ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts + 1 ].v[ 0 ] = cmd->s2;
	tess.texCoords0[ numVerts + 1 ].v[ 1 ] = cmd->t1;

	tess.xyz[ numVerts + 2 ].v[ 0 ] = mx + cw + sh;
	tess.xyz[ numVerts + 2 ].v[ 1 ] = my - sw + ch;
	tess.xyz[ numVerts + 2 ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts + 2 ].v[ 0 ] = cmd->s2;
	tess.texCoords0[ numVerts + 2 ].v[ 1 ] = cmd->t2;

	tess.xyz[ numVerts + 3 ].v[ 0 ] = mx - cw + sh;
	tess.xyz[ numVerts + 3 ].v[ 1 ] = my + sw + ch;
	tess.xyz[ numVerts + 3 ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts + 3 ].v[ 0 ] = cmd->s1;
	tess.texCoords0[ numVerts + 3 ].v[ 1 ] = cmd->t2;

	return ( const void * )( cmd + 1 );
}

// -NERVE - SMF

/*
==============
RB_StretchPicGradient
==============
*/
const void     *RB_StretchPicGradient( const void *data )
{
	const stretchPicCommand_t *cmd;
	shader_t                  *shader;
	int                       numVerts, numIndexes;

	cmd = ( const stretchPicCommand_t * ) data;

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	shader = cmd->shader;

	if ( shader != tess.shader )
	{
		if ( tess.numIndexes )
		{
			RB_EndSurface();
		}

		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

//  *(int *)tess.vertexColors[ numVerts ].v =
//      *(int *)tess.vertexColors[ numVerts + 1 ].v =
//      *(int *)tess.vertexColors[ numVerts + 2 ].v =
//      *(int *)tess.vertexColors[ numVerts + 3 ].v = *(int *)backEnd.color2D;

	* ( int * ) tess.vertexColors[ numVerts ].v = * ( int * ) tess.vertexColors[ numVerts + 1 ].v = * ( int * ) backEnd.color2D;

	* ( int * ) tess.vertexColors[ numVerts + 2 ].v = * ( int * ) tess.vertexColors[ numVerts + 3 ].v = * ( int * ) cmd->gradientColor;

	tess.xyz[ numVerts ].v[ 0 ] = cmd->x;
	tess.xyz[ numVerts ].v[ 1 ] = cmd->y;
	tess.xyz[ numVerts ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts ].v[ 0 ] = cmd->s1;
	tess.texCoords0[ numVerts ].v[ 1 ] = cmd->t1;

	tess.xyz[ numVerts + 1 ].v[ 0 ] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ].v[ 1 ] = cmd->y;
	tess.xyz[ numVerts + 1 ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts + 1 ].v[ 0 ] = cmd->s2;
	tess.texCoords0[ numVerts + 1 ].v[ 1 ] = cmd->t1;

	tess.xyz[ numVerts + 2 ].v[ 0 ] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ].v[ 1 ] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts + 2 ].v[ 0 ] = cmd->s2;
	tess.texCoords0[ numVerts + 2 ].v[ 1 ] = cmd->t2;

	tess.xyz[ numVerts + 3 ].v[ 0 ] = cmd->x;
	tess.xyz[ numVerts + 3 ].v[ 1 ] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ].v[ 2 ] = 0;

	tess.texCoords0[ numVerts + 3 ].v[ 0 ] = cmd->s1;
	tess.texCoords0[ numVerts + 3 ].v[ 1 ] = cmd->t2;

	return ( const void * )( cmd + 1 );
}

/*
=============
RB_DrawSurfs

=============
*/
const void     *RB_DrawSurfs( const void *data )
{
	const drawSurfsCommand_t *cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes )
	{
		RB_EndSurface();
	}

	cmd = ( const drawSurfsCommand_t * ) data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );

	return ( const void * )( cmd + 1 );
}

/*
=============
RB_RunVisTests

=============
*/
const void     *RB_RunVisTests( const void *data )
{
	const runVisTestsCommand_t *cmd;
	int i, j;
	vec4_t eye, clip;
	float depth, *modelMatrix, *projectionMatrix;
	int windowX, windowY;

	// finish any 2D drawing if needed
	if ( tess.numIndexes )
	{
		RB_EndSurface();
	}

	cmd = ( const runVisTestsCommand_t * ) data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	modelMatrix = backEnd.orientation.modelMatrix;
	projectionMatrix = backEnd.viewParms.projectionMatrix;

	for( i = 0; i < cmd->numVisTests; i++ ) {
		visTest_t *test = cmd->visTests[ i ];
		float      len;

		// transfer flare to eyespace, adjust distance and then
		// transform to clip space
		for( j = 0; j < 4; j++ ) {
			eye[ j ] =
				test->position[ 0 ] * modelMatrix[ j + 0 * 4 ] +
				test->position[ 1 ] * modelMatrix[ j + 1 * 4 ] +
				test->position[ 2 ] * modelMatrix[ j + 2 * 4 ] +
				1.0f                * modelMatrix[ j + 3 * 4 ];
		}

		len = VectorNormalize( eye );
		VectorScale( eye, len - test->depthAdjust, eye );

		for( j = 0; j < 4; j++ ) {
			clip[ j ] =
				eye[ 0 ] * projectionMatrix[ j + 0 * 4 ] +
				eye[ 1 ] * projectionMatrix[ j + 1 * 4 ] +
				eye[ 2 ] * projectionMatrix[ j + 2 * 4 ] +
				eye[ 3 ] * projectionMatrix[ j + 3 * 4 ];

		}

		if( fabsf( clip[ 0 ] ) > clip[ 3 ] ||
		    fabsf( clip[ 1 ] ) > clip[ 3 ] ||
		    fabsf( clip[ 2 ] ) > clip[ 3 ] ) {
			test->lastResult = qfalse;
			continue;
		}

		clip[ 3 ] = 1.0f / clip[3];
		clip[ 0 ] *= clip[ 3 ];
		clip[ 1 ] *= clip[ 3 ];
		clip[ 2 ] *= clip[ 3 ];

		windowX = (int)(( 0.5f * clip[ 0 ] + 0.5f ) * backEnd.viewParms.viewportWidth);
		windowY = (int)(( 0.5f * clip[ 1 ] + 0.5f ) * backEnd.viewParms.viewportHeight);

		glReadPixels( windowX, windowY, 1, 1, GL_DEPTH_COMPONENT,
			      GL_FLOAT, &depth );

		test->lastResult = ( depth >= 0.5f * clip[ 2 ] + 0.5f );
	}

	return ( const void * )( cmd + 1 );
}

/*
=============
RB_DrawBuffer

=============
*/
const void     *RB_DrawBuffer( const void *data )
{
	const drawBufferCommand_t *cmd;

	cmd = ( const drawBufferCommand_t * ) data;

	glDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->integer )
	{
		glClearColor( 1, 0, 0.5, 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return ( const void * )( cmd + 1 );
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void )
{
	int     i;
	image_t *image;
	float   x, y, w, h;
	int     start, end;

	if ( !backEnd.projection2D )
	{
		RB_SetGL2D();
	}

	glClear( GL_COLOR_BUFFER_BIT );

	glFinish();

	start = ri.Milliseconds();

	for ( i = 0; i < tr.numImages; i++ )
	{
		image = tr.images[ i ];

		w = glConfig.vidWidth / 40;
		h = glConfig.vidHeight / 30;

		x = i % 40 * w;
		y = i / 30 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 )
		{
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		GL_Bind( image );
		glBegin( GL_QUADS );
		glTexCoord2f( 0, 0 );
		glVertex2f( x, y );
		glTexCoord2f( 1, 0 );
		glVertex2f( x + w, y );
		glTexCoord2f( 1, 1 );
		glVertex2f( x + w, y + h );
		glTexCoord2f( 0, 1 );
		glVertex2f( x, y + h );
		glEnd();
	}

	glFinish();

	end = ri.Milliseconds();
	ri.Printf( PRINT_DEVELOPER, "%i msec to draw all images\n", end - start );
}

/*
=============
RB_DrawBounds - ydnar
=============
*/

void RB_DrawBounds( vec3_t mins, vec3_t maxs )
{
	vec3_t center;

	GL_Bind( tr.whiteImage );
	GL_State( GLS_POLYMODE_LINE );

	// box corners
	glBegin( GL_LINES );
	glColor3f( 1, 1, 1 );

	glVertex3f( mins[ 0 ], mins[ 1 ], mins[ 2 ] );
	glVertex3f( maxs[ 0 ], mins[ 1 ], mins[ 2 ] );
	glVertex3f( mins[ 0 ], mins[ 1 ], mins[ 2 ] );
	glVertex3f( mins[ 0 ], maxs[ 1 ], mins[ 2 ] );
	glVertex3f( mins[ 0 ], mins[ 1 ], mins[ 2 ] );
	glVertex3f( mins[ 0 ], mins[ 1 ], maxs[ 2 ] );

	glVertex3f( maxs[ 0 ], maxs[ 1 ], maxs[ 2 ] );
	glVertex3f( mins[ 0 ], maxs[ 1 ], maxs[ 2 ] );
	glVertex3f( maxs[ 0 ], maxs[ 1 ], maxs[ 2 ] );
	glVertex3f( maxs[ 0 ], mins[ 1 ], maxs[ 2 ] );
	glVertex3f( maxs[ 0 ], maxs[ 1 ], maxs[ 2 ] );
	glVertex3f( maxs[ 0 ], maxs[ 1 ], mins[ 2 ] );
	glEnd();

	center[ 0 ] = ( mins[ 0 ] + maxs[ 0 ] ) * 0.5;
	center[ 1 ] = ( mins[ 1 ] + maxs[ 1 ] ) * 0.5;
	center[ 2 ] = ( mins[ 2 ] + maxs[ 2 ] ) * 0.5;

	// center axis
	glBegin( GL_LINES );
	glColor3f( 1, 0.85, 0 );

	glVertex3f( mins[ 0 ], center[ 1 ], center[ 2 ] );
	glVertex3f( maxs[ 0 ], center[ 1 ], center[ 2 ] );
	glVertex3f( center[ 0 ], mins[ 1 ], center[ 2 ] );
	glVertex3f( center[ 0 ], maxs[ 1 ], center[ 2 ] );
	glVertex3f( center[ 0 ], center[ 1 ], mins[ 2 ] );
	glVertex3f( center[ 0 ], center[ 1 ], maxs[ 2 ] );
	glEnd();
}

/*
=============
RB_SwapBuffers

=============
*/
const void     *RB_SwapBuffers( const void *data )
{
	const swapBuffersCommand_t *cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes )
	{
		RB_EndSurface();
	}

	// texture swapping test
	if ( r_showImages->integer )
	{
		RB_ShowImages();
	}

	cmd = ( const swapBuffersCommand_t * ) data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer )
	{
		int           i;
		long          sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
		glReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ )
		{
			sum += stencilReadback[ i ];
		}

		backEnd.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory( stencilReadback );
	}

	if ( !glState.finishCalled )
	{
		glFinish();
	}

	GLimp_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	GLimp_EndFrame();

	backEnd.projection2D = qfalse;

	return ( const void * )( cmd + 1 );
}

//bani

/*
=============
RB_RenderToTexture

=============
*/
const void     *RB_RenderToTexture( const void *data )
{
	const renderToTextureCommand_t *cmd;

//  ri.Printf( PRINT_ALL, "RB_RenderToTexture\n" );

	cmd = ( const renderToTextureCommand_t * ) data;

	GL_Bind( cmd->image );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
	glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, cmd->x, cmd->y, cmd->w, cmd->h, 0 );
//  glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cmd->x, cmd->y, cmd->w, cmd->h );

	return ( const void * )( cmd + 1 );
}

//bani

/*
=============
RB_Finish

=============
*/
const void     *RB_Finish( const void *data )
{
	const renderFinishCommand_t *cmd;

//  ri.Printf( PRINT_ALL, "RB_Finish\n" );

	cmd = ( const renderFinishCommand_t * ) data;

	glFinish();

	return ( const void * )( cmd + 1 );
}

/*
====================
RB_ExecuteRenderCommands

This function will be called synchronously if running without
smp extensions, or asynchronously by another thread.
====================
*/
void RB_ExecuteRenderCommands( const void *data )
{
	int t1, t2;

	t1 = ri.Milliseconds();

	if ( !r_smp->integer || data == backEndData[ 0 ]->commands.cmds )
	{
		backEnd.smpFrame = 0;
	}
	else
	{
		backEnd.smpFrame = 1;
	}

	while ( 1 )
	{
		switch ( * ( const int * ) data )
		{
			case RC_SET_COLOR:
				data = RB_SetColor( data );
				break;

			case RC_STRETCH_PIC:
				data = RB_StretchPic( data );
				break;

			case RC_2DPOLYS:
				data = RB_Draw2dPolys( data );
				break;
			case RC_2DPOLYSINDEXED:
				data = RB_Draw2dPolysIndexed( data );
				break;
			case RC_ROTATED_PIC:
				data = RB_RotatedPic( data );
				break;

			case RC_STRETCH_PIC_GRADIENT:
				data = RB_StretchPicGradient( data );
				break;

			case RC_DRAW_SURFS:
				data = RB_DrawSurfs( data );
				break;

			case RC_RUN_VISTESTS:
				data = RB_RunVisTests( data );
				break;

			case RC_DRAW_BUFFER:
				data = RB_DrawBuffer( data );
				break;

			case RC_SWAP_BUFFERS:
				data = RB_SwapBuffers( data );
				break;

			case RC_VIDEOFRAME:
				data = RB_TakeVideoFrameCmd( data );
				break;

				//bani
			case RC_RENDERTOTEXTURE:
				data = RB_RenderToTexture( data );
				break;

				//bani
			case RC_FINISH:
				data = RB_Finish( data );
				break;

			case RC_SCISSORENABLE:
				data = RB_ScissorEnable( data );
				break;

			case RC_SCISSORSET:
				data = RB_ScissorSet( data );
				break;

			case RC_END_OF_LIST:
			default:
				// stop rendering on this thread
				t2 = ri.Milliseconds();
				backEnd.pc.msec = t2 - t1;
				return;
		}
	}
}

/*
================
RB_RenderThread
================
*/
void RB_RenderThread( void )
{
	const void *data;

	// wait for either a rendering command or a quit command
	while ( 1 )
	{
		// sleep until we have work to do
		data = GLimp_RendererSleep();

		if ( !data )
		{
			return; // all done, renderer is shutting down
		}

		renderThreadActive = qtrue;

		RB_ExecuteRenderCommands( data );

		renderThreadActive = qfalse;
	}
}
