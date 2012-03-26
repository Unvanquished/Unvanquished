/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_cmds.c
#include "tr_local.h"

volatile renderCommandList_t *renderCommandList;

volatile qboolean            renderThreadActive;

/*
=====================
R_PerformanceCounters
=====================
*/
void R_PerformanceCounters( void )
{
	if ( !r_speeds->integer )
	{
		// clear the counters even if we aren't printing
		Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
		Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
		return;
	}

	if ( r_speeds->integer == RSPEEDS_GENERAL )
	{
		ri.Printf( PRINT_ALL, "%i views %i portals %i batches %i surfs %i leafs %i verts %i tris\n",
		           backEnd.pc.c_views, backEnd.pc.c_portals, backEnd.pc.c_batches, backEnd.pc.c_surfaces, tr.pc.c_leafs,
		           backEnd.pc.c_vertexes, backEnd.pc.c_indexes / 3 );

		ri.Printf( PRINT_ALL, "%i lights %i bout %i pvsout %i queryout %i interactions\n",
		           tr.pc.c_dlights + tr.pc.c_slights - backEnd.pc.c_occlusionQueriesLightsCulled,
		           tr.pc.c_box_cull_light_out,
		           tr.pc.c_pvs_cull_light_out,
		           backEnd.pc.c_occlusionQueriesLightsCulled,
		           tr.pc.c_dlightInteractions + tr.pc.c_slightInteractions - backEnd.pc.c_occlusionQueriesInteractionsCulled );

		ri.Printf( PRINT_ALL, "%i draws %i queries %i CHC++ ms %i vbos %i ibos %i verts %i tris\n",
		           backEnd.pc.c_drawElements,
		           tr.pc.c_occlusionQueries,
		           tr.pc.c_CHCTime,
		           backEnd.pc.c_vboVertexBuffers, backEnd.pc.c_vboIndexBuffers,
		           backEnd.pc.c_vboVertexes, backEnd.pc.c_vboIndexes / 3 );

		ri.Printf( PRINT_ALL, "%i multidraws %i primitives %i tris\n",
		           backEnd.pc.c_multiDrawElements,
		           backEnd.pc.c_multiDrawPrimitives,
		           backEnd.pc.c_multiVboIndexes / 3 );
	}
	else if ( r_speeds->integer == RSPEEDS_CULLING )
	{
		ri.Printf( PRINT_ALL, "(gen) %i sin %i sout %i pin %i pout\n",
		           tr.pc.c_sphere_cull_in, tr.pc.c_sphere_cull_out, tr.pc.c_plane_cull_in, tr.pc.c_plane_cull_out );

		ri.Printf( PRINT_ALL, "(patch) %i sin %i sclip %i sout %i bin %i bclip %i bout\n",
		           tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip,
		           tr.pc.c_sphere_cull_patch_out, tr.pc.c_box_cull_patch_in,
		           tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );

		ri.Printf( PRINT_ALL, "(mdx) %i sin %i sclip %i sout %i bin %i bclip %i bout\n",
		           tr.pc.c_sphere_cull_mdx_in, tr.pc.c_sphere_cull_mdx_clip,
		           tr.pc.c_sphere_cull_mdx_out, tr.pc.c_box_cull_mdx_in, tr.pc.c_box_cull_mdx_clip, tr.pc.c_box_cull_mdx_out );

		ri.Printf( PRINT_ALL, "(md5) %i bin %i bclip %i bout\n",
		           tr.pc.c_box_cull_md5_in, tr.pc.c_box_cull_md5_clip, tr.pc.c_box_cull_md5_out );
	}
	else if ( r_speeds->integer == RSPEEDS_VIEWCLUSTER )
	{
		ri.Printf( PRINT_ALL, "viewcluster: %i\n", tr.visClusters[ tr.visIndex ] );
	}
	else if ( r_speeds->integer == RSPEEDS_LIGHTS )
	{
		ri.Printf( PRINT_ALL, "dlight srf:%i culled:%i\n", tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled );

		ri.Printf( PRINT_ALL, "dlights:%i interactions:%i\n", tr.pc.c_dlights, tr.pc.c_dlightInteractions );

		ri.Printf( PRINT_ALL, "slights:%i interactions:%i\n", tr.pc.c_slights, tr.pc.c_slightInteractions );
	}
	else if ( r_speeds->integer == RSPEEDS_SHADOWCUBE_CULLING )
	{
		ri.Printf( PRINT_ALL, "omni pyramid tests:%i bin:%i bclip:%i bout:%i\n",
		           tr.pc.c_pyramidTests, tr.pc.c_pyramid_cull_ent_in, tr.pc.c_pyramid_cull_ent_clip, tr.pc.c_pyramid_cull_ent_out );
	}
	else if ( r_speeds->integer == RSPEEDS_FOG )
	{
		ri.Printf( PRINT_ALL, "fog srf:%i batches:%i\n", backEnd.pc.c_fogSurfaces, backEnd.pc.c_fogBatches );
	}
	else if ( r_speeds->integer == RSPEEDS_FLARES )
	{
		ri.Printf( PRINT_ALL, "flare adds:%i tests:%i renders:%i\n",
		           backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders );
	}
	else if ( r_speeds->integer == RSPEEDS_OCCLUSION_QUERIES )
	{
		ri.Printf( PRINT_ALL, "occlusion queries:%i multi:%i saved:%i culled lights:%i culled entities:%i culled leafs:%i response time:%i fetch time:%i\n",
		           backEnd.pc.c_occlusionQueries,
		           backEnd.pc.c_occlusionQueriesMulti,
		           backEnd.pc.c_occlusionQueriesSaved,
		           backEnd.pc.c_occlusionQueriesLightsCulled,
		           backEnd.pc.c_occlusionQueriesEntitiesCulled,
		           backEnd.pc.c_occlusionQueriesLeafsCulled,
		           backEnd.pc.c_occlusionQueriesResponseTime,
		           backEnd.pc.c_occlusionQueriesFetchTime );
	}
	else if ( r_speeds->integer == RSPEEDS_DEPTH_BOUNDS_TESTS )
	{
		ri.Printf( PRINT_ALL, "depth bounds tests:%i rejected:%i\n", tr.pc.c_depthBoundsTests, tr.pc.c_depthBoundsTestsRejected );
	}
	else if ( r_speeds->integer == RSPEEDS_SHADING_TIMES )
	{
		if ( DS_STANDARD_ENABLED() )
		{
			ri.Printf( PRINT_ALL, "deferred shading times: g-buffer:%i lighting:%i translucent:%i\n", backEnd.pc.c_deferredGBufferTime,
			           backEnd.pc.c_deferredLightingTime, backEnd.pc.c_forwardTranslucentTime );
		}
		else
		{
			ri.Printf( PRINT_ALL, "forward shading times: ambient:%i lighting:%i\n", backEnd.pc.c_forwardAmbientTime,
			           backEnd.pc.c_forwardLightingTime );
		}
	}
	else if ( r_speeds->integer == RSPEEDS_CHC )
	{
		ri.Printf( PRINT_ALL, "%i CHC++ ms %i queries %i multi queries %i saved\n",
		           tr.pc.c_CHCTime,
		           tr.pc.c_occlusionQueries,
		           tr.pc.c_occlusionQueriesMulti,
		           tr.pc.c_occlusionQueriesSaved );
	}
	else if ( r_speeds->integer == RSPEEDS_NEAR_FAR )
	{
		ri.Printf( PRINT_ALL, "zNear: %.0f zFar: %.0f\n", tr.viewParms.zNear, tr.viewParms.zFar );
	}
	else if ( r_speeds->integer == RSPEEDS_DECALS )
	{
		ri.Printf( PRINT_ALL, "decal projectors: %d test surfs: %d clip surfs: %d decal surfs: %d created: %d\n",
		           tr.pc.c_decalProjectors, tr.pc.c_decalTestSurfaces, tr.pc.c_decalClipSurfaces, tr.pc.c_decalSurfaces,
		           tr.pc.c_decalSurfacesCreated );
	}

	Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
	Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}

/*
====================
R_InitCommandBuffers
====================
*/
void R_InitCommandBuffers( void )
{
	glConfig.smpActive = qfalse;

	if ( r_smp->integer )
	{
		ri.Printf( PRINT_ALL, "Trying SMP acceleration...\n" );

		if ( GLimp_SpawnRenderThread( RB_RenderThread ) )
		{
			ri.Printf( PRINT_ALL, "...succeeded.\n" );
			glConfig.smpActive = qtrue;
		}
		else
		{
			ri.Printf( PRINT_ALL, "...failed.\n" );
		}
	}
}

/*
====================
R_ShutdownCommandBuffers
====================
*/
void R_ShutdownCommandBuffers( void )
{
	// kill the rendering thread
	if ( glConfig.smpActive )
	{
		GLimp_WakeRenderer( NULL );
		glConfig.smpActive = qfalse;
	}
}

/*
====================
R_IssueRenderCommands
====================
*/
int c_blockedOnRender;
int c_blockedOnMain;

void R_IssueRenderCommands( qboolean runPerformanceCounters )
{
	renderCommandList_t *cmdList;

	cmdList = &backEndData[ tr.smpFrame ]->commands;
	assert( cmdList );  // bk001205
	// add an end-of-list command
	* ( int * )( cmdList->cmds + cmdList->used ) = RC_END_OF_LIST;

	// clear it out, in case this is a sync and not a buffer flip
	cmdList->used = 0;

	if ( glConfig.smpActive )
	{
		// if the render thread is not idle, wait for it
		if ( renderThreadActive )
		{
			c_blockedOnRender++;

			if ( r_showSmp->integer )
			{
				ri.Printf( PRINT_ALL, "R" );
			}
		}
		else
		{
			c_blockedOnMain++;

			if ( r_showSmp->integer )
			{
				ri.Printf( PRINT_ALL, "." );
			}
		}

		// sleep until the renderer has completed
		GLimp_FrontEndSleep();
	}

	// at this point, the back end thread is idle, so it is ok
	// to look at it's performance counters
	if ( runPerformanceCounters )
	{
		R_PerformanceCounters();
	}

	// actually start the commands going
	if ( !r_skipBackEnd->integer )
	{
		// let it start on the new batch
		if ( !glConfig.smpActive )
		{
			RB_ExecuteRenderCommands( cmdList->cmds );
		}
		else
		{
			GLimp_WakeRenderer( cmdList );
		}
	}
}

/*
====================
R_SyncRenderThread

Issue any pending commands and wait for them to complete.
After exiting, the render thread will have completed its work
and will remain idle and the main thread is free to issue
OpenGL calls until R_IssueRenderCommands is called.
====================
*/
void R_SyncRenderThread( void )
{
	if ( !tr.registered )
	{
		return;
	}

	R_IssueRenderCommands( qfalse );

	if ( !glConfig.smpActive )
	{
		return;
	}

	GLimp_FrontEndSleep();
}

/*
============
R_GetCommandBuffer

make sure there is enough command space, waiting on the
render thread if needed.
============
*/
void           *R_GetCommandBuffer( int bytes )
{
	renderCommandList_t *cmdList;

	cmdList = &backEndData[ tr.smpFrame ]->commands;

	// always leave room for the swap buffers and end of list commands
	// RB: added swapBuffers_t from ET
	if ( cmdList->used + bytes + ( sizeof( swapBuffersCommand_t ) + sizeof( int ) ) > MAX_RENDER_COMMANDS )
	{
		if ( bytes > MAX_RENDER_COMMANDS - ( sizeof( swapBuffersCommand_t ) + sizeof( int ) ) )
		{
			ri.Error( ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes );
		}

		// if we run out of room, just start dropping commands
		return NULL;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
}

/*
=============
R_AddDrawViewCmd
=============
*/
void R_AddDrawViewCmd()
{
	drawViewCommand_t *cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_DRAW_VIEW;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

/*
=============
RE_SetColor

Passing NULL will set the color to white
=============
*/
void RE_SetColor( const float *rgba )
{
	setColorCommand_t *cmd;

	if ( !tr.registered )
	{
		return;
	}

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_SET_COLOR;

	if ( !rgba )
	{
		static float colorWhite[ 4 ] = { 1, 1, 1, 1 };

		rgba = colorWhite;
	}

	cmd->color[ 0 ] = rgba[ 0 ];
	cmd->color[ 1 ] = rgba[ 1 ];
	cmd->color[ 2 ] = rgba[ 2 ];
	cmd->color[ 3 ] = rgba[ 3 ];
}

/*
=============
R_ClipRegion
=============
*/
static qboolean R_ClipRegion ( float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2 )
{
	float left, top, right, bottom;
	float _s1, _t1, _s2, _t2;
	float clipLeft, clipTop, clipRight, clipBottom;

	if ( tr.clipRegion[2] <= tr.clipRegion[0] ||
		tr.clipRegion[3] <= tr.clipRegion[1] )
	{
		return qfalse;
	}

	left = *x;
	top = *y;
	right = *x + *w;
	bottom = *y + *h;

	_s1 = *s1;
	_t1 = *t1;
	_s2 = *s2;
	_t2 = *t2;

	clipLeft = tr.clipRegion[0];
	clipTop = tr.clipRegion[1];
	clipRight = tr.clipRegion[2];
	clipBottom = tr.clipRegion[3];

	// Completely clipped away
	if ( right <= clipLeft || left >= clipRight ||
		bottom <= clipTop || top >= clipBottom )
	{
		return qtrue;
	}

	// Clip left edge
	if ( left < clipLeft )
	{
		float f = ( clipLeft - left ) / ( right - left );
		*s1 = ( f * ( _s2 - _s1 ) ) + _s1;
		*x = clipLeft;
		*w -= ( clipLeft - left );
	}

	// Clip right edge
	if ( right > clipRight )
	{
		float f = ( clipRight - right ) / ( left - right );
		*s2 = ( f * ( _s1 - _s2 ) ) + _s2;
		*w = clipRight - *x;
	}

	// Clip top edge
	if ( top < clipTop )
	{
		float f = ( clipTop - top ) / ( bottom - top );
		*t1 = ( f * ( _t2 - _t1 ) ) + _t1;
		*y = clipTop;
		*h -= ( clipTop - top );
	}

	// Clip bottom edge
	if ( bottom > clipBottom )
	{
		float f = ( clipBottom - bottom ) / ( top - bottom );
		*t2 = ( f * ( _t1 - _t2 ) ) + _t2;
		*h = clipBottom - *y;
	}

	return qfalse;
}


/*
=============
RE_SetClipRegion
=============
*/
void RE_SetClipRegion( const float *region )
{
	if ( region == NULL )
	{
		Com_Memset( tr.clipRegion, 0, sizeof( vec4_t ) );
	}
	else
	{
		Vector4Copy( region, tr.clipRegion );
	}
}

/*
=============
RE_StretchPic
=============
*/
void RE_StretchPic ( float x, float y, float w, float h,
					  float s1, float t1, float s2, float t2, qhandle_t hShader )
{
	stretchPicCommand_t	*cmd;

	if (!tr.registered)
	{
		return;
	}
	if ( R_ClipRegion( &x, &y, &w, &h, &s1, &t1, &s2, &t2 ) )
	{
		return;
	}

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_STRETCH_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

/*
=============
RE_2DPolyies
=============
*/
extern int r_numPolyVerts;

void RE_2DPolyies( polyVert_t *verts, int numverts, qhandle_t hShader )
{
	poly2dCommand_t *cmd;

	if ( r_numPolyVerts + numverts > r_maxPolyVerts->integer )
	{
		return;
	}

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_2DPOLYS;
	cmd->verts = &backEndData[ tr.smpFrame ]->polyVerts[ r_numPolyVerts ];
	cmd->numverts = numverts;
	memcpy( cmd->verts, verts, sizeof( polyVert_t ) * numverts );
	cmd->shader = R_GetShaderByHandle( hShader );

	r_numPolyVerts += numverts;
}

/*
=============
RE_RotatedPic
=============
*/
void RE_RotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle )
{
	stretchPicCommand_t *cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_ROTATED_PIC;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;

	cmd->angle = angle;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;
}

//----(SA)  added

/*
==============
RE_StretchPicGradient
==============
*/
void RE_StretchPicGradient( float x, float y, float w, float h,
                            float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor,
                            int gradientType )
{
	stretchPicCommand_t *cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_STRETCH_PIC_GRADIENT;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;

	if ( !gradientColor )
	{
		static float colorWhite[ 4 ] = { 1, 1, 1, 1 };

		gradientColor = colorWhite;
	}

	cmd->gradientColor[ 0 ] = gradientColor[ 0 ] * 255;
	cmd->gradientColor[ 1 ] = gradientColor[ 1 ] * 255;
	cmd->gradientColor[ 2 ] = gradientColor[ 2 ] * 255;
	cmd->gradientColor[ 3 ] = gradientColor[ 3 ] * 255;
	cmd->gradientType = gradientType;
}

//----(SA)  end

/*
====================
RE_BeginFrame

If running in stereo, RE_BeginFrame will be called twice
for each RE_EndFrame
====================
*/
void RE_BeginFrame( stereoFrame_t stereoFrame )
{
	drawBufferCommand_t *cmd;

	if ( !tr.registered )
	{
		return;
	}

	GLimp_LogComment( "--- RE_BeginFrame ---\n" );

#if defined( USE_D3D10 )
	// TODO
#else
	glState.finishCalled = qfalse;
#endif

	tr.frameCount++;
	tr.frameSceneNum = 0;
	tr.viewCount = 0;

#if defined( USE_D3D10 )
	// draw buffer stuff
	cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_DRAW_BUFFER;
	cmd->buffer = 0;
#else

	// do overdraw measurement
	if ( r_measureOverdraw->integer )
	{
		if ( glConfig.stencilBits < 4 )
		{
			ri.Printf( PRINT_ALL, "Warning: not enough stencil bits to measure overdraw: %d\n", glConfig.stencilBits );
			ri.Cvar_Set( "r_measureOverdraw", "0" );
			r_measureOverdraw->modified = qfalse;
		}
		else
		{
			R_SyncRenderThread();
			glEnable( GL_STENCIL_TEST );
			glStencilMask( ~0U );
			GL_ClearStencil( 0U );
			glStencilFunc( GL_ALWAYS, 0U, ~0U );
			glStencilOp( GL_KEEP, GL_INCR, GL_INCR );
		}

		r_measureOverdraw->modified = qfalse;
	}
	else
	{
		// this is only reached if it was on and is now off
		if ( r_measureOverdraw->modified )
		{
			R_SyncRenderThread();
			glDisable( GL_STENCIL_TEST );
		}

		r_measureOverdraw->modified = qfalse;
	}

	// texturemode stuff
	if ( r_textureMode->modified )
	{
		R_SyncRenderThread();
		GL_TextureMode( r_textureMode->string );
		r_textureMode->modified = qfalse;
	}

	// gamma stuff
	if ( r_gamma->modified )
	{
		r_gamma->modified = qfalse;

		R_SyncRenderThread();
		R_SetColorMappings();
	}

	// check for errors
	if ( !r_ignoreGLErrors->integer )
	{
		int  err;
		char s[ 128 ];

		R_SyncRenderThread();

		if ( ( err = glGetError() ) != GL_NO_ERROR )
		{
			switch ( err )
			{
				case GL_INVALID_ENUM:
					strcpy( s, "GL_INVALID_ENUM" );
					break;

				case GL_INVALID_VALUE:
					strcpy( s, "GL_INVALID_VALUE" );
					break;

				case GL_INVALID_OPERATION:
					strcpy( s, "GL_INVALID_OPERATION" );
					break;

				case GL_STACK_OVERFLOW:
					strcpy( s, "GL_STACK_OVERFLOW" );
					break;

				case GL_STACK_UNDERFLOW:
					strcpy( s, "GL_STACK_UNDERFLOW" );
					break;

				case GL_OUT_OF_MEMORY:
					strcpy( s, "GL_OUT_OF_MEMORY" );
					break;

				case GL_TABLE_TOO_LARGE:
					strcpy( s, "GL_TABLE_TOO_LARGE" );
					break;

				case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
					strcpy( s, "GL_INVALID_FRAMEBUFFER_OPERATION_EXT" );
					break;

				default:
					Com_sprintf( s, sizeof( s ), "0x%X", err );
					break;
			}

			//ri.Error(ERR_FATAL, "caught OpenGL error: %s in file %s line %i", s, filename, line);
			ri.Error( ERR_FATAL, "RE_BeginFrame() - glGetError() failed (%s)!\n", s );
		}
	}

	// draw buffer stuff
	cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_DRAW_BUFFER;

	if ( glConfig.stereoEnabled )
	{
		if ( stereoFrame == STEREO_LEFT )
		{
			cmd->buffer = ( int ) GL_BACK_LEFT;
		}
		else if ( stereoFrame == STEREO_RIGHT )
		{
			cmd->buffer = ( int ) GL_BACK_RIGHT;
		}
		else
		{
			ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame );
		}
	}
	else
	{
		if ( stereoFrame != STEREO_CENTER )
		{
			ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is disabled, but stereoFrame was %i", stereoFrame );
		}

		if ( !Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) )
		{
			cmd->buffer = ( int ) GL_FRONT;
		}
		else
		{
			cmd->buffer = ( int ) GL_BACK;
		}
	}

#endif
}

/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void RE_EndFrame( int *frontEndMsec, int *backEndMsec )
{
	swapBuffersCommand_t *cmd;

	if ( !tr.registered )
	{
		return;
	}

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_SWAP_BUFFERS;

	R_IssueRenderCommands( qtrue );

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current ones
	R_ToggleSmpFrame();

	if ( frontEndMsec )
	{
		*frontEndMsec = tr.frontEndMsec;
	}

	tr.frontEndMsec = 0;

	if ( backEndMsec )
	{
		*backEndMsec = backEnd.pc.msec;
	}

	backEnd.pc.msec = 0;
}

/*
=============
RE_TakeVideoFrame
=============
*/
void RE_TakeVideoFrame( int width, int height, byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg )
{
	videoFrameCommand_t *cmd;

	if ( !tr.registered )
	{
		return;
	}

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_VIDEOFRAME;

	cmd->width = width;
	cmd->height = height;
	cmd->captureBuffer = captureBuffer;
	cmd->encodeBuffer = encodeBuffer;
	cmd->motionJpeg = motionJpeg;
}

//bani

/*
==================
RE_RenderToTexture
==================
*/
void RE_RenderToTexture( int textureid, int x, int y, int w, int h )
{
	//renderToTextureCommand_t *cmd;

	ri.Printf( PRINT_ALL, S_COLOR_RED "TODO RE_RenderToTexture\n" );

#if 0

	if ( textureid > tr.numImages || textureid < 0 )
	{
		ri.Printf( PRINT_ALL, "Warning: trap_R_RenderToTexture textureid %d out of range.\n", textureid );
		return;
	}

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_RENDERTOTEXTURE;
	cmd->image = tr.images[ textureid ];
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
#endif
}

/*
==================
RE_Finish
==================
*/
void RE_Finish( void )
{
	renderFinishCommand_t *cmd;

	ri.Printf( PRINT_ALL, "RE_Finish\n" );

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = RC_FINISH;
}
