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

volatile bool            renderThreadActive;

/*
=====================
R_PerformanceCounters
=====================
*/
void R_PerformanceCounters()
{
	if ( !r_speeds->integer )
	{
		// clear the counters even if we aren't printing
		Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
		Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
		return;
	}

	if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_GENERAL))
	{
		Log::Notice("%i views %i portals %i batches %i surfs %i leafs %i verts %i tris",
		           backEnd.pc.c_views, backEnd.pc.c_portals, backEnd.pc.c_batches, backEnd.pc.c_surfaces, tr.pc.c_leafs,
		           backEnd.pc.c_vertexes, backEnd.pc.c_indexes / 3 );

		Log::Notice("%i lights %i bout %i pvsout %i interactions",
		           tr.pc.c_dlights + tr.pc.c_slights,
		           tr.pc.c_box_cull_light_out,
		           tr.pc.c_pvs_cull_light_out,
		           tr.pc.c_dlightInteractions + tr.pc.c_slightInteractions );

		Log::Notice("%i draws %i vbos %i ibos %i verts %i tris",
		           backEnd.pc.c_drawElements,
		           backEnd.pc.c_vboVertexBuffers, backEnd.pc.c_vboIndexBuffers,
		           backEnd.pc.c_vboVertexes, backEnd.pc.c_vboIndexes / 3 );

		Log::Notice("%i multidraws %i primitives %i tris",
		           backEnd.pc.c_multiDrawElements,
		           backEnd.pc.c_multiDrawPrimitives,
		           backEnd.pc.c_multiVboIndexes / 3 );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_CULLING ))
	{
		Log::Notice("(gen) %i pin %i pout %i bin %i bclip %i bout",
		           tr.pc.c_plane_cull_in, tr.pc.c_plane_cull_out, tr.pc.c_box_cull_in,
		           tr.pc.c_box_cull_clip, tr.pc.c_box_cull_out );

		Log::Notice("(mdv) %i sin %i sclip %i sout %i bin %i bclip %i bout",
		           tr.pc.c_sphere_cull_mdv_in, tr.pc.c_sphere_cull_mdv_clip,
		           tr.pc.c_sphere_cull_mdv_out, tr.pc.c_box_cull_mdv_in, tr.pc.c_box_cull_mdv_clip, tr.pc.c_box_cull_mdv_out );

		Log::Notice("(md5) %i bin %i bclip %i bout",
		           tr.pc.c_box_cull_md5_in, tr.pc.c_box_cull_md5_clip, tr.pc.c_box_cull_md5_out );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_VIEWCLUSTER ))
	{
		Log::Notice("viewcluster: %i", tr.visClusters[ tr.visIndex ] );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_LIGHTS ))
	{
		Log::Notice("dlight srf:%i culled:%i", tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled );

		Log::Notice("dlights:%i interactions:%i", tr.pc.c_dlights, tr.pc.c_dlightInteractions );

		Log::Notice("slights:%i interactions:%i", tr.pc.c_slights, tr.pc.c_slightInteractions );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_SHADOWCUBE_CULLING ))
	{
		Log::Notice("omni pyramid tests:%i bin:%i bclip:%i bout:%i",
		           tr.pc.c_pyramidTests, tr.pc.c_pyramid_cull_ent_in, tr.pc.c_pyramid_cull_ent_clip, tr.pc.c_pyramid_cull_ent_out );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_FOG ))
	{
		Log::Notice("fog srf:%i batches:%i", backEnd.pc.c_fogSurfaces, backEnd.pc.c_fogBatches );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_FLARES ))
	{
		Log::Notice("flare adds:%i tests:%i renders:%i",
		           backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_SHADING_TIMES ))
	{
		Log::Notice("forward shading times: ambient:%i lighting:%i", backEnd.pc.c_forwardAmbientTime,
			           backEnd.pc.c_forwardLightingTime );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_NEAR_FAR ))
	{
		Log::Notice("zNear: %.0f zFar: %.0f", tr.viewParms.zNear, tr.viewParms.zFar );
	}
	else if ( r_speeds->integer == Util::ordinal(renderSpeeds_t::RSPEEDS_DECALS ))
	{
		Log::Notice("decal projectors: %d test surfs: %d clip surfs: %d decal surfs: %d created: %d",
		           tr.pc.c_decalProjectors, tr.pc.c_decalTestSurfaces, tr.pc.c_decalClipSurfaces, tr.pc.c_decalSurfaces,
		           tr.pc.c_decalSurfacesCreated );
	}

	Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
	Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}

/*
====================
R_IssueRenderCommands
====================
*/
int c_blockedOnRender;
int c_blockedOnMain;

void R_IssueRenderCommands( bool runPerformanceCounters )
{
	renderCommandList_t *cmdList;

	cmdList = &backEndData[ tr.smpFrame ]->commands;
	ASSERT(cmdList != nullptr);
	// add an end-of-list command
	*reinterpret_cast<renderCommand_t*>(&cmdList->cmds[cmdList->used]) = renderCommand_t::RC_END_OF_LIST;

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
				Log::Notice("R");
			}
		}
		else
		{
			c_blockedOnMain++;

			if ( r_showSmp->integer )
			{
				Log::Notice(".");
			}
		}

		// sleep until the renderer has completed
		GLimp_FrontEndSleep();
	}

	// at this point, the back end thread is idle, so it is ok
	// to look at its performance counters
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
			GLimp_WakeRenderer( cmdList->cmds );
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
void R_SyncRenderThread()
{
	if ( !tr.registered )
	{
		return;
	}

	R_IssueRenderCommands( false );

	if ( !glConfig.smpActive )
	{
		return;
	}

	GLimp_SyncRenderThread();
}

/*
============
R_GetCommandBuffer

make sure there is enough command space, waiting on the
render thread if needed.
============
*/
void           *R_GetCommandBuffer( unsigned bytes )
{
	renderCommandList_t *cmdList;

	cmdList = &backEndData[ tr.smpFrame ]->commands;

	// always leave room for the swap buffers and end of list commands
	// RB: added swapBuffers_t from ET
	if ( cmdList->used + bytes + ( sizeof( swapBuffersCommand_t ) + sizeof( int ) ) > MAX_RENDER_COMMANDS )
	{
		if ( bytes > MAX_RENDER_COMMANDS - ( sizeof( swapBuffersCommand_t ) + sizeof( int ) ) )
		{
			ri.Error( errorParm_t::ERR_FATAL, "R_GetCommandBuffer: bad size %i", bytes );
		}

		// if we run out of room, just start dropping commands
		return nullptr;
	}

	cmdList->used += bytes;

	return cmdList->cmds + cmdList->used - bytes;
}

/*
=============
R_AddSetupLightsCmd
=============
*/
void R_AddSetupLightsCmd()
{
	setupLightsCommand_t *cmd;

	cmd = (setupLightsCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_SETUP_LIGHTS;

	cmd->refdef = tr.refdef;
}

/*
=============
R_AddDrawViewCmd
=============
*/
void R_AddDrawViewCmd()
{
	drawViewCommand_t *cmd;

	cmd = (drawViewCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_DRAW_VIEW;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

/*
=============
R_AddRunVisTestsCmd

=============
*/
void R_AddRunVisTestsCmd()
{
	runVisTestsCommand_t *cmd;

	cmd = ( runVisTestsCommand_t * ) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_RUN_VISTESTS;

	cmd->refdef = tr.refdef;
	cmd->viewParms = tr.viewParms;
}

/*
=============
RE_SetColor

Passing nullptr will set the color to white
=============
*/
void RE_SetColor( const Color::Color& rgba )
{
	setColorCommand_t *cmd;

	if ( !tr.registered )
	{
		return;
	}

	cmd = (setColorCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_SET_COLOR;

	cmd->color = rgba;
}

/*
=============
RE_SetColorGrading
=============
*/
void RE_SetColorGrading( int slot, qhandle_t hShader )
{
	setColorGradingCommand_t *cmd;
	shader_t *shader = R_GetShaderByHandle( hShader );
	image_t *image;

	if ( !tr.registered )
	{
		return;
	}

	if ( slot < 0 || slot > 3 )
	{
		return;
	}

	if ( shader->defaultShader || !shader->stages[ 0 ] )
	{
		return;
	}

	image = shader->stages[ 0 ]->bundle[ 0 ].image[ 0 ];

	if ( !image )
	{
		return;
	}

	if ( image->width != REF_COLORGRADEMAP_SIZE && image->height != REF_COLORGRADEMAP_SIZE )
	{
		return;
	}

	if ( image->width * image->height != REF_COLORGRADEMAP_STORE_SIZE )
	{
		return;
	}

	cmd = ( setColorGradingCommand_t * ) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->slot = slot;
	cmd->image = image;
	cmd->commandId = renderCommand_t::RC_SET_COLORGRADING;
}

/*
=============
R_ClipRegion
=============
*/
static bool R_ClipRegion ( float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2 )
{
	float left, top, right, bottom;
	float _s1, _t1, _s2, _t2;
	float clipLeft, clipTop, clipRight, clipBottom;

	if ( tr.clipRegion[2] <= tr.clipRegion[0] ||
		tr.clipRegion[3] <= tr.clipRegion[1] )
	{
		return false;
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
		return true;
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

	return false;
}


/*
=============
RE_SetClipRegion
=============
*/
void RE_SetClipRegion( const float *region )
{
	if ( region == nullptr )
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

	cmd = (stretchPicCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_STRETCH_PIC;
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
extern int r_numPolyIndexes;

void RE_2DPolyies( polyVert_t *verts, int numverts, qhandle_t hShader )
{
	poly2dCommand_t *cmd;

	if ( r_numPolyVerts + numverts > r_maxPolyVerts->integer )
	{
		return;
	}

	cmd = (poly2dCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_2DPOLYS;
	cmd->verts = &backEndData[ tr.smpFrame ]->polyVerts[ r_numPolyVerts ];
	cmd->numverts = numverts;
	memcpy( cmd->verts, verts, sizeof( polyVert_t ) * numverts );
	cmd->shader = R_GetShaderByHandle( hShader );

	r_numPolyVerts += numverts;
}

void RE_2DPolyiesIndexed( polyVert_t *verts, int numverts, int *indexes, int numindexes, int trans_x, int trans_y, qhandle_t hShader )
{
	poly2dIndexedCommand_t *cmd;

	if ( r_numPolyVerts + numverts > r_maxPolyVerts->integer )
	{
		return;
	}

	if ( r_numPolyIndexes + numindexes > r_maxPolyVerts->integer )
	{
		return;
	}

	cmd = (poly2dIndexedCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_2DPOLYSINDEXED;
	cmd->verts = &backEndData[ tr.smpFrame ]->polyVerts[ r_numPolyVerts ];
	cmd->numverts = numverts;
	memcpy( cmd->verts, verts, sizeof( polyVert_t ) * numverts );
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->indexes = &backEndData[ tr.smpFrame ]->polyIndexes[ r_numPolyIndexes ];
	memcpy( cmd->indexes, indexes, sizeof( int ) * numindexes );
	cmd->numIndexes = numindexes;
	cmd->translation[ 0 ] = trans_x;
	cmd->translation[ 1 ] = trans_y;

	r_numPolyVerts += numverts;
	r_numPolyIndexes += numindexes;
}

/*
================
RE_ScissorEnable
================
*/
void RE_ScissorEnable( bool enable )
{
	// scissor disable sets scissor to full screen
	// scissor enable is a no-op
	if( !enable ) {
		RE_ScissorSet( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	}
}

/*
=============
RE_ScissorSet
=============
*/
void RE_ScissorSet( int x, int y, int w, int h )
{
	scissorSetCommand_t *cmd;

	cmd = (scissorSetCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_SCISSORSET;
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
}

/*
=============
RE_RotatedPic
=============
*/
void RE_RotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle )
{
	stretchPicCommand_t *cmd;

	cmd = (stretchPicCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_ROTATED_PIC;
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
                            float s1, float t1, float s2, float t2,
                            qhandle_t hShader, const Color::Color& gradientColor,
                            int gradientType )
{
	stretchPicCommand_t *cmd;

	cmd = (stretchPicCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_STRETCH_PIC_GRADIENT;
	cmd->shader = R_GetShaderByHandle( hShader );
	cmd->x = x;
	cmd->y = y;
	cmd->w = w;
	cmd->h = h;
	cmd->s1 = s1;
	cmd->t1 = t1;
	cmd->s2 = s2;
	cmd->t2 = t2;

	cmd->gradientColor = gradientColor;
	cmd->gradientType = gradientType;
}

//----(SA)  end

/*
====================
RE_BeginFrame
====================
*/
void RE_BeginFrame()
{
	drawBufferCommand_t *cmd;

	if ( !tr.registered )
	{
		return;
	}

	GLimp_LogComment( "--- RE_BeginFrame ---\n" );

	tr.frameCount++;
	tr.frameSceneNum = 0;
	tr.viewCount = 0;

	// do overdraw measurement
	if ( r_measureOverdraw->integer )
	{
		if ( glConfig.stencilBits < 4 )
		{
			Log::Warn("not enough stencil bits to measure overdraw: %d", glConfig.stencilBits );
			ri.Cvar_Set( "r_measureOverdraw", "0" );
			r_measureOverdraw->modified = false;
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

		r_measureOverdraw->modified = false;
	}
	else
	{
		// this is only reached if it was on and is now off
		if ( r_measureOverdraw->modified )
		{
			R_SyncRenderThread();
			glDisable( GL_STENCIL_TEST );
		}

		r_measureOverdraw->modified = false;
	}

	// texturemode stuff
	if ( r_textureMode->modified )
	{
		R_SyncRenderThread();
		GL_TextureMode( r_textureMode->string );
		r_textureMode->modified = false;
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

				case GL_INVALID_FRAMEBUFFER_OPERATION:
					strcpy( s, "GL_INVALID_FRAMEBUFFER_OPERATION" );
					break;

				default:
					Com_sprintf( s, sizeof( s ), "0x%X", err );
					break;
			}

			//ri.Error(ERR_FATAL, "caught OpenGL error: %s in file %s line %i", s, filename, line);
			ri.Error( errorParm_t::ERR_FATAL, "RE_BeginFrame() - glGetError() failed (%s)!", s );
		}
	}

	// draw buffer stuff
	cmd = (drawBufferCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_DRAW_BUFFER;

	if ( !Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) )
	{
		cmd->buffer = ( int ) GL_FRONT;
	}
	else
	{
		cmd->buffer = ( int ) GL_BACK;
	}
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

	GLimp_HandleCvars();

	cmd = (swapBuffersCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_SWAP_BUFFERS;

	R_IssueRenderCommands( true );

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current ones
	R_ToggleSmpFrame();

	// update the results of the vis tests
	R_UpdateVisTests();

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
void RE_TakeVideoFrame( int width, int height, byte *captureBuffer, byte *encodeBuffer, bool motionJpeg )
{
	videoFrameCommand_t *cmd;

	if ( !tr.registered )
	{
		return;
	}

	cmd = (videoFrameCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_VIDEOFRAME;

	cmd->width = width;
	cmd->height = height;
	cmd->captureBuffer = captureBuffer;
	cmd->encodeBuffer = encodeBuffer;
	cmd->motionJpeg = motionJpeg;
}

//bani

/*
==================
RE_Finish
==================
*/
void RE_Finish()
{
	renderFinishCommand_t *cmd;

	Log::Notice("RE_Finish\n" );

	cmd = (renderFinishCommand_t*) R_GetCommandBuffer( sizeof( *cmd ) );

	if ( !cmd )
	{
		return;
	}

	cmd->commandId = renderCommand_t::RC_FINISH;
}
