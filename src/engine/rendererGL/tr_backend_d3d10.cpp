/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_backend.c
#include "tr_local.h"

#if defined( __cplusplus )
extern "C" {
#endif

	backEndData_t  *backEndData[ SMP_FRAMES ];
	backEndState_t backEnd;

	/*
	================
	RB_Hyperspace

	A player has predicted a teleport, but hasn't arrived yet
	================
	*/
	static void RB_Hyperspace( void )
	{
		float c;

		if( !backEnd.isHyperspace )
		{
			// do initialization shit
		}

		c = ( backEnd.refdef.time & 255 ) / 255.0f;

#if defined( USE_D3D10 )
		// TODO
#else
		GL_ClearColor( c, c, c, 1 );
		qglClear( GL_COLOR_BUFFER_BIT );
#endif

		backEnd.isHyperspace = qtrue;
	}

	static void SetViewportAndScissor( void )
	{
#if defined( USE_D3D10 )
		// TODO
#else
		GL_LoadProjectionMatrix( backEnd.viewParms.projectionMatrix );

		// set the window clipping
		GL_Viewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
		             backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );

		GL_Scissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY,
		            backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
#endif
	}

	/*
	================
	RB_SetGL2D
	================
	*/
	static void RB_SetGL2D( void )
	{
		matrix_t proj;

		GLimp_LogComment( "--- RB_SetGL2D ---\n" );

#if defined( USE_D3D10 )
		// TODO
#else

		// disable offscreen rendering
		if( glConfig.framebufferObjectAvailable )
		{
			R_BindNullFBO();
		}

#endif

		backEnd.projection2D = qtrue;

#if defined( USE_D3D10 )
		// TODO
#else
		// set 2D virtual screen size
		GL_Viewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
		GL_Scissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );

		MatrixOrthogonalProjection( proj, 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 );
		GL_LoadProjectionMatrix( proj );
		GL_LoadModelViewMatrix( matrixIdentity );

		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

		qglDisable( GL_CULL_FACE );
		qglDisable( GL_CLIP_PLANE0 );
#endif

		// set time for 2D shaders
		backEnd.refdef.time = ri.Milliseconds();
		backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
	}

	static void RB_RenderDrawSurfaces( qboolean opaque, qboolean depthFill )
	{
		trRefEntity_t *entity, *oldEntity;
		shader_t      *shader, *oldShader;
		int           lightmapNum, oldLightmapNum;
		qboolean      depthRange, oldDepthRange;
		int           i;
		drawSurf_t    *drawSurf;

		GLimp_LogComment( "--- RB_RenderDrawSurfaces ---\n" );

		// draw everything
		oldEntity = NULL;
		oldShader = NULL;
		oldLightmapNum = -1;
		oldDepthRange = qfalse;
		depthRange = qfalse;
		backEnd.currentLight = NULL;

		for( i = 0, drawSurf = backEnd.viewParms.drawSurfs; i < backEnd.viewParms.numDrawSurfs; i++, drawSurf++ )
		{
			// update locals
			entity = drawSurf->entity;
			shader = tr.sortedShaders[ drawSurf->shaderNum ];
			lightmapNum = drawSurf->lightmapNum;

			if( opaque )
			{
				// skip all translucent surfaces that don't matter for this pass
				if( shader->sort > SS_OPAQUE )
				{
					break;
				}
			}
			else
			{
				// skip all opaque surfaces that don't matter for this pass
				if( shader->sort <= SS_OPAQUE )
				{
					continue;
				}
			}

			if( entity == oldEntity && shader == oldShader && lightmapNum == oldLightmapNum )
			{
				// fast path, same as previous sort
				rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
				continue;
			}

			// change the tess parameters if needed
			// a "entityMergable" shader is a shader that can have surfaces from seperate
			// entities merged into a single batch, like smoke and blood puff sprites
			if( shader != oldShader || lightmapNum != oldLightmapNum || ( entity != oldEntity && !shader->entityMergable ) )
			{
				if( oldShader != NULL )
				{
					Tess_End();
				}

				if( depthFill )
				{
					Tess_Begin( Tess_StageIteratorDepthFill, NULL, shader, NULL, qtrue, qfalse, lightmapNum, tess.fogNum );
				}
				else
				{
					Tess_Begin( Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, lightmapNum, tess.fogNum );
				}

				oldShader = shader;
				oldLightmapNum = lightmapNum;
			}

			// change the modelview matrix if needed
			if( entity != oldEntity )
			{
				depthRange = qfalse;

				if( entity != &tr.worldEntity )
				{
					backEnd.currentEntity = entity;

					// set up the transformation matrix
					R_RotateEntityForViewParms( backEnd.currentEntity, &backEnd.viewParms, &backEnd.orientation );

					if( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK )
					{
						// hack the depth range to prevent view model from poking into walls
						depthRange = qtrue;
					}
				}
				else
				{
					backEnd.currentEntity = &tr.worldEntity;
					backEnd.orientation = backEnd.viewParms.world;
				}

#if defined( USE_D3D10 )
				// TODO
#else
				GL_LoadModelViewMatrix( backEnd.orientation.modelViewMatrix );
#endif

				// change depthrange if needed
				if( oldDepthRange != depthRange )
				{
#if defined( USE_D3D10 )
					// TODO
#else

					if( depthRange )
					{
						qglDepthRange( 0, 0.3 );
					}
					else
					{
						qglDepthRange( 0, 1 );
					}

#endif
					oldDepthRange = depthRange;
				}

				oldEntity = entity;
			}

			// add the triangles for this surface
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
		}

		// draw the contents of the last shader batch
		if( oldShader != NULL )
		{
			Tess_End();
		}

		// go back to the world modelview matrix
#if defined( USE_D3D10 )
		// TODO
#else
		GL_LoadModelViewMatrix( backEnd.viewParms.world.modelViewMatrix );
#endif

		if( depthRange )
		{
#if defined( USE_D3D10 )
			// TODO
#else
			qglDepthRange( 0, 1 );
#endif
		}

#if defined( USE_D3D10 )
		// TODO
#else
		GL_CheckErrors();
#endif
	}

	/*
	=================
	RB_RenderInteractions
	=================
	*/

	/*
	==================
	GL_RenderView
	==================
	*/
	static void D3D10_RenderView( void )
	{
		if( r_logFile->integer )
		{
			// don't just call LogComment, or we will get a call to va() every frame!
			GLimp_LogComment( va
			                  ( "--- D3D10_RenderView( %i surfaces, %i interactions ) ---\n", backEnd.viewParms.numDrawSurfs,
			                    backEnd.viewParms.numInteractions ) );
		}

		backEnd.pc.c_surfaces += backEnd.viewParms.numDrawSurfs;

		//if(r_deferredShading->integer && glConfig.framebufferObjectAvailable && glConfig.textureFloatAvailable &&
		//   glConfig.drawBuffersAvailable && glConfig.maxDrawBuffers >= 4)
		{
			int   clearBits = 0;

			// clear the back buffer
			float ClearColor[ 4 ] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red,green,blue,alpha
			dx.d3dDevice->ClearRenderTargetView( dx.renderTargetView, ClearColor );

#if 0
			// render a triangle
			D3D10_TECHNIQUE_DESC techDesc;
			dx.genericTechnique->GetDesc( &techDesc );

			for( UINT p = 0; p < techDesc.Passes; ++p )
			{
				dx.genericTechnique->GetPassByIndex( p )->Apply( 0 );
				dx.d3dDevice->Draw( 3, 0 );
			}

#endif

			/*
			// sync with gl if needed
			if(r_finish->integer == 1 && !glState.finishCalled)
			{
			        //qglFinish();
			        glState.finishCalled = qtrue;
			}
			if(r_finish->integer == 0)
			{
			        glState.finishCalled = qtrue;
			}
			*/

			// we will need to change the projection matrix before drawing
			// 2D images again
			backEnd.projection2D = qfalse;

			// set the modelview matrix for the viewer
			SetViewportAndScissor();

			// ensures that depth writes are enabled for the depth clear
			//GL_State(GLS_DEFAULT);

			// clear frame buffer objects
			//R_BindFBO(tr.deferredRenderFBO);
			//qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//clearBits = GL_DEPTH_BUFFER_BIT;

			/*
			if(!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
			{
			        clearBits |= GL_COLOR_BUFFER_BIT;
			        GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // FIXME: get color of sky
			}
			qglClear(clearBits);

			R_BindFBO(tr.geometricRenderFBO);
			if(!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
			{
			        clearBits = GL_COLOR_BUFFER_BIT;
			        GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // FIXME: get color of sky
			}
			else
			{
			        if(glConfig.framebufferBlitAvailable)
			        {
			                // copy color of the main context to deferredRenderFBO
			                qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
			                qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
			                qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                                                           0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                                                           GL_COLOR_BUFFER_BIT,
			                                                           GL_NEAREST);
			        }
			}
			qglClear(clearBits);
			*/

			/*
			if(r_deferredShading->integer == DS_PREPASS_LIGHTING)
			{
			        R_BindFBO(tr.geometricRenderFBO);
			        if(!(backEnd.refdef.rdflags & RDF_NOWORLDMODEL))
			        {
			                clearBits = GL_COLOR_BUFFER_BIT;
			                GL_ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			                qglClear(clearBits);
			        }
			}
			*/

			if( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
			{
				RB_Hyperspace();
				return;
			}
			else
			{
				backEnd.isHyperspace = qfalse;
			}

			//glState.faceCulling = -1; // force face culling to set next time

			// we will only draw a sun if there was sky rendered in this view
			backEnd.skyRenderedThisView = qfalse;

			//GL_CheckErrors();

#if 0

			if( r_deferredShading->integer == DS_STANDARD )
			{
				// draw everything that is opaque
				R_BindFBO( tr.deferredRenderFBO );
				RB_RenderDrawSurfaces( qtrue, qfalse );
			}

#endif

			//D3D10_RenderDrawSurfacesIntoGeometricBuffer();

			// try to cull lights using hardware occlusion queries
			//R_BindFBO(tr.deferredRenderFBO);
			//RB_RenderLightOcclusionQueries();

			//if(r_deferredShading->integer == DS_PREPASS_LIGHTING)
			{
				/*
				if(r_shadows->integer >= 4)
				{
				        // render dynamic shadowing and lighting using shadow mapping
				        RB_RenderInteractionsDeferredShadowMapped();
				}
				else
				*/
				{
					// render dynamic lighting
					// TODO D3D10_RenderInteractionsDeferredIntoLightBuffer();
				}

				// render opaque surfaces using the light buffer results

				// TODo
			}

			/*
			// render global fog
			R_BindFBO(tr.deferredRenderFBO);
			RB_RenderUniformFog();

			// render debug information
			R_BindFBO(tr.deferredRenderFBO);
			RB_RenderDebugUtils();

			// scale down rendered HDR scene to 1 / 4th
			if(r_hdrRendering->integer)
			{
			        if(glConfig.framebufferBlitAvailable)
			        {
			                qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
			                qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer);
			                qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                                                                0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
			                                                                GL_COLOR_BUFFER_BIT,
			                                                                GL_LINEAR);

			                qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
			                qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_64x64->frameBuffer);
			                qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                                                           0, 0, 64, 64,
			                                                           GL_COLOR_BUFFER_BIT,
			                                                           GL_LINEAR);
			        }
			        else
			        {
			                // FIXME add non EXT_framebuffer_blit code
			        }

			        RB_CalculateAdaptation();
			}
			else
			{
			        if(glConfig.framebufferBlitAvailable)
			        {
			                // copy deferredRenderFBO to downScaleFBO_quarter
			                qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
			                qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.downScaleFBO_quarter->frameBuffer);
			                qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                                                                0, 0, glConfig.vidWidth * 0.25f, glConfig.vidHeight * 0.25f,
			                                                                GL_COLOR_BUFFER_BIT,
			                                                                GL_LINEAR);
			        }
			        else
			        {
			                // FIXME add non EXT_framebuffer_blit code
			        }
			}

			GL_CheckErrors();

			// render bloom post process effect
			RB_RenderBloom();

			// copy offscreen rendered scene to the current OpenGL context
			RB_RenderDeferredShadingResultToFrameBuffer();

			if(backEnd.viewParms.isPortal)
			{
			        if(glConfig.framebufferBlitAvailable)
			        {
			                // copy deferredRenderFBO to portalRenderFBO
			                qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer);
			                qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.portalRenderFBO->frameBuffer);
			                qglBlitFramebufferEXT(0, 0, tr.deferredRenderFBO->width, tr.deferredRenderFBO->height,
			                                                           0, 0, tr.portalRenderFBO->width, tr.portalRenderFBO->height,
			                                                           GL_COLOR_BUFFER_BIT,
			                                                           GL_NEAREST);
			        }
			        else
			        {
			                // capture current color buffer
			                GL_SelectTexture(0);
			                GL_Bind(tr.portalRenderImage);
			                qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.portalRenderImage->uploadWidth, tr.portalRenderImage->uploadHeight);
			        }
			        backEnd.pc.c_portals++;
			}
			*/
		}

		backEnd.pc.c_views++;
	}

	/*
	============================================================================

	RENDER BACK END THREAD FUNCTIONS

	============================================================================
	*/

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

		if( !tr.registered )
		{
			return;
		}

		R_SyncRenderThread();

		// we definately want to sync every frame for the cinematics
#if defined( USE_D3D10 )
		// TODO
#else
		qglFinish();
#endif

		start = end = 0;

		if( r_speeds->integer )
		{
#if defined( USE_D3D10 )
			// TODO
#else
			qglFinish();
#endif
			start = ri.Milliseconds();
		}

		// make sure rows and cols are powers of 2
		for( i = 0; ( 1 << i ) < cols; i++ )
		{
		}

		for( j = 0; ( 1 << j ) < rows; j++ )
		{
		}

		if( ( 1 << i ) != cols || ( 1 << j ) != rows )
		{
			ri.Error( ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows );
		}

#if defined( USE_D3D10 )
		// TODO
#else
		RB_SetGL2D();

		qglVertexAttrib4fARB( ATTR_INDEX_NORMAL, 0, 0, 1, 1 );
		qglVertexAttrib4fARB( ATTR_INDEX_COLOR, tr.identityLight, tr.identityLight, tr.identityLight, 1 );

		GL_BindProgram( &tr.genericSingleShader );

		// set uniforms
		GLSL_SetUniform_TCGen_Environment( &tr.genericSingleShader,  qfalse );
		GLSL_SetUniform_ColorGen( &tr.genericSingleShader, CGEN_VERTEX );
		GLSL_SetUniform_AlphaGen( &tr.genericSingleShader, AGEN_VERTEX );

		//GLSL_SetUniform_Color(&tr.genericSingleShader, colorWhite);
		if( glConfig.vboVertexSkinningAvailable )
		{
			GLSL_SetUniform_VertexSkinning( &tr.genericSingleShader, qfalse );
		}

		GLSL_SetUniform_DeformGen( &tr.genericSingleShader, DGEN_NONE );
		GLSL_SetUniform_AlphaTest( &tr.genericSingleShader, 0 );
		GLSL_SetUniform_ModelViewProjectionMatrix( &tr.genericSingleShader, glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.scratchImage[ client ] );
		GLSL_SetUniform_ColorTextureMatrix( &tr.genericSingleShader, matrixIdentity );

		// if the scratchImage isn't in the format we want, specify it as a new texture
		if( cols != tr.scratchImage[ client ]->width || rows != tr.scratchImage[ client ]->height )
		{
			tr.scratchImage[ client ]->width = tr.scratchImage[ client ]->uploadWidth = cols;
			tr.scratchImage[ client ]->height = tr.scratchImage[ client ]->uploadHeight = rows;

			qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
		}
		else
		{
			if( dirty )
			{
				// otherwise, just subimage upload it so that drivers can tell we are going to be changing
				// it and don't try and do a texture compression
				qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
			}
		}

#endif // #if defined(USE_D3D10)

		if( r_speeds->integer )
		{
#if defined( USE_D3D10 )
			// TODO
#else
			qglFinish();
#endif
			end = ri.Milliseconds();
			ri.Printf( PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
		}

		tess.numVertexes = 0;
		tess.numIndexes = 0;

		tess.xyz[ tess.numVertexes ][ 0 ] = x;
		tess.xyz[ tess.numVertexes ][ 1 ] = y;
		tess.xyz[ tess.numVertexes ][ 2 ] = 0;
		tess.xyz[ tess.numVertexes ][ 3 ] = 1;
		tess.texCoords[ tess.numVertexes ][ 0 ] = 0.5f / cols;
		tess.texCoords[ tess.numVertexes ][ 1 ] = 0.5f / rows;
		tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
		tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
		tess.numVertexes++;

		tess.xyz[ tess.numVertexes ][ 0 ] = x + w;
		tess.xyz[ tess.numVertexes ][ 1 ] = y;
		tess.xyz[ tess.numVertexes ][ 2 ] = 0;
		tess.xyz[ tess.numVertexes ][ 3 ] = 1;
		tess.texCoords[ tess.numVertexes ][ 0 ] = ( cols - 0.5f ) / cols;
		tess.texCoords[ tess.numVertexes ][ 1 ] = 0.5f / rows;
		tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
		tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
		tess.numVertexes++;

		tess.xyz[ tess.numVertexes ][ 0 ] = x + w;
		tess.xyz[ tess.numVertexes ][ 1 ] = y + h;
		tess.xyz[ tess.numVertexes ][ 2 ] = 0;
		tess.xyz[ tess.numVertexes ][ 3 ] = 1;
		tess.texCoords[ tess.numVertexes ][ 0 ] = ( cols - 0.5f ) / cols;
		tess.texCoords[ tess.numVertexes ][ 1 ] = ( rows - 0.5f ) / rows;
		tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
		tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
		tess.numVertexes++;

		tess.xyz[ tess.numVertexes ][ 0 ] = x;
		tess.xyz[ tess.numVertexes ][ 1 ] = y + h;
		tess.xyz[ tess.numVertexes ][ 2 ] = 0;
		tess.xyz[ tess.numVertexes ][ 3 ] = 1;
		tess.texCoords[ tess.numVertexes ][ 0 ] = 0.5f / cols;
		tess.texCoords[ tess.numVertexes ][ 1 ] = ( rows - 0.5f ) / rows;
		tess.texCoords[ tess.numVertexes ][ 2 ] = 0;
		tess.texCoords[ tess.numVertexes ][ 3 ] = 1;
		tess.numVertexes++;

		tess.indexes[ tess.numIndexes++ ] = 0;
		tess.indexes[ tess.numIndexes++ ] = 1;
		tess.indexes[ tess.numIndexes++ ] = 2;
		tess.indexes[ tess.numIndexes++ ] = 0;
		tess.indexes[ tess.numIndexes++ ] = 2;
		tess.indexes[ tess.numIndexes++ ] = 3;

		Tess_UpdateVBOs( ATTR_POSITION | ATTR_TEXCOORD );

		Tess_DrawElements();

		tess.numVertexes = 0;
		tess.numIndexes = 0;

#if defined( USE_D3D10 )
		// TODO
#else
		GL_CheckErrors();
#endif
	}

	void RE_UploadCinematic( int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty )
	{
#if defined( USE_D3D10 )
		// TODO
#else
		GL_Bind( tr.scratchImage[ client ] );

		// if the scratchImage isn't in the format we want, specify it as a new texture
		if( cols != tr.scratchImage[ client ]->width || rows != tr.scratchImage[ client ]->height )
		{
			tr.scratchImage[ client ]->width = tr.scratchImage[ client ]->uploadWidth = cols;
			tr.scratchImage[ client ]->height = tr.scratchImage[ client ]->uploadHeight = rows;

			qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
			qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
			qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, colorBlack );
		}
		else
		{
			if( dirty )
			{
				// otherwise, just subimage upload it so that drivers can tell we are going to be changing
				// it and don't try and do a texture compression
				qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
			}
		}

		GL_CheckErrors();
#endif
	}

	/*
	=============
	RB_SetColor
	=============
	*/
	const void     *RB_SetColor( const void *data )
	{
		const setColorCommand_t *cmd;

		GLimp_LogComment( "--- RB_SetColor ---\n" );

		cmd = ( const setColorCommand_t * ) data;

		backEnd.color2D[ 0 ] = cmd->color[ 0 ];
		backEnd.color2D[ 1 ] = cmd->color[ 1 ];
		backEnd.color2D[ 2 ] = cmd->color[ 2 ];
		backEnd.color2D[ 3 ] = cmd->color[ 3 ];

		return ( const void * )( cmd + 1 );
	}

	/*
	=============
	RB_StretchPic
	=============
	*/
	const void     *RB_StretchPic( const void *data )
	{
		int                       i;
		const stretchPicCommand_t *cmd;
		shader_t                  *shader;
		int                       numVerts, numIndexes;

		GLimp_LogComment( "--- RB_StretchPic ---\n" );

		cmd = ( const stretchPicCommand_t * ) data;

		if( !backEnd.projection2D )
		{
			RB_SetGL2D();
		}

		shader = cmd->shader;

		if( shader != tess.surfaceShader )
		{
			if( tess.numIndexes )
			{
				Tess_End();
			}

			backEnd.currentEntity = &backEnd.entity2D;
			Tess_Begin( Tess_StageIteratorGeneric, NULL, shader, NULL, qfalse, qfalse, -1, tess.fogNum );
		}

		Tess_CheckOverflow( 4, 6 );
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

		for( i = 0; i < 4; i++ )
		{
			tess.colors[ numVerts + i ][ 0 ] = backEnd.color2D[ 0 ];
			tess.colors[ numVerts + i ][ 1 ] = backEnd.color2D[ 1 ];
			tess.colors[ numVerts + i ][ 2 ] = backEnd.color2D[ 2 ];
			tess.colors[ numVerts + i ][ 3 ] = backEnd.color2D[ 3 ];
		}

		tess.xyz[ numVerts ][ 0 ] = cmd->x;
		tess.xyz[ numVerts ][ 1 ] = cmd->y;
		tess.xyz[ numVerts ][ 2 ] = 0;
		tess.xyz[ numVerts ][ 3 ] = 1;

		tess.texCoords[ numVerts ][ 0 ] = cmd->s1;
		tess.texCoords[ numVerts ][ 1 ] = cmd->t1;
		tess.texCoords[ numVerts ][ 2 ] = 0;
		tess.texCoords[ numVerts ][ 3 ] = 1;

		tess.xyz[ numVerts + 1 ][ 0 ] = cmd->x + cmd->w;
		tess.xyz[ numVerts + 1 ][ 1 ] = cmd->y;
		tess.xyz[ numVerts + 1 ][ 2 ] = 0;
		tess.xyz[ numVerts + 1 ][ 3 ] = 1;

		tess.texCoords[ numVerts + 1 ][ 0 ] = cmd->s2;
		tess.texCoords[ numVerts + 1 ][ 1 ] = cmd->t1;
		tess.texCoords[ numVerts + 1 ][ 2 ] = 0;
		tess.texCoords[ numVerts + 1 ][ 3 ] = 1;

		tess.xyz[ numVerts + 2 ][ 0 ] = cmd->x + cmd->w;
		tess.xyz[ numVerts + 2 ][ 1 ] = cmd->y + cmd->h;
		tess.xyz[ numVerts + 2 ][ 2 ] = 0;
		tess.xyz[ numVerts + 2 ][ 3 ] = 1;

		tess.texCoords[ numVerts + 2 ][ 0 ] = cmd->s2;
		tess.texCoords[ numVerts + 2 ][ 1 ] = cmd->t2;
		tess.texCoords[ numVerts + 2 ][ 2 ] = 0;
		tess.texCoords[ numVerts + 2 ][ 3 ] = 1;

		tess.xyz[ numVerts + 3 ][ 0 ] = cmd->x;
		tess.xyz[ numVerts + 3 ][ 1 ] = cmd->y + cmd->h;
		tess.xyz[ numVerts + 3 ][ 2 ] = 0;
		tess.xyz[ numVerts + 3 ][ 3 ] = 1;

		tess.texCoords[ numVerts + 3 ][ 0 ] = cmd->s1;
		tess.texCoords[ numVerts + 3 ][ 1 ] = cmd->t2;
		tess.texCoords[ numVerts + 3 ][ 2 ] = 0;
		tess.texCoords[ numVerts + 3 ][ 3 ] = 1;

		return ( const void * )( cmd + 1 );
	}

	/*
	=============
	RB_DrawView
	=============
	*/
	const void     *RB_DrawView( const void *data )
	{
		const drawViewCommand_t *cmd;

		GLimp_LogComment( "--- RB_DrawView ---\n" );

		// finish any 2D drawing if needed
		if( tess.numIndexes )
		{
			Tess_End();
		}

		cmd = ( const drawViewCommand_t * ) data;

		backEnd.refdef = cmd->refdef;
		backEnd.viewParms = cmd->viewParms;

#if defined( USE_D3D10 )
		D3D10_RenderView();
#else
		GL_RenderView();
#endif

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

		GLimp_LogComment( "--- RB_DrawBuffer ---\n" );

		cmd = ( const drawBufferCommand_t * ) data;

		// clear screen for debugging
		if( r_clear->integer )
		{
			float ClearColor[ 4 ] = { 0.0f, 0.125f, 0.3f, 1.0f }; // red,green,blue,alpha
			dx.d3dDevice->ClearRenderTargetView( dx.renderTargetView, ClearColor );
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
#if defined( USE_D3D10 )
// TODO
#else
	void RB_ShowImages( void )
	{
		int     i;
		image_t *image;
		float   x, y, w, h;
		vec4_t  quadVerts[ 4 ];
		int     start, end;

		GLimp_LogComment( "--- RB_ShowImages ---\n" );

		if( !backEnd.projection2D )
		{
			RB_SetGL2D();
		}

		qglClear( GL_COLOR_BUFFER_BIT );

		qglFinish();

		GL_BindProgram( &tr.genericSingleShader );
		GL_Cull( CT_TWO_SIDED );

		// set uniforms
		GLSL_SetUniform_TCGen_Environment( &tr.genericSingleShader,  qfalse );
		GLSL_SetUniform_ColorGen( &tr.genericSingleShader, CGEN_VERTEX );
		GLSL_SetUniform_AlphaGen( &tr.genericSingleShader, AGEN_VERTEX );

		if( glConfig.vboVertexSkinningAvailable )
		{
			GLSL_SetUniform_VertexSkinning( &tr.genericSingleShader, qfalse );
		}

		GLSL_SetUniform_DeformGen( &tr.genericSingleShader, DGEN_NONE );
		GLSL_SetUniform_AlphaTest( &tr.genericSingleShader, 0 );
		GLSL_SetUniform_ColorTextureMatrix( &tr.genericSingleShader, matrixIdentity );

		GL_SelectTexture( 0 );

		start = ri.Milliseconds();

		for( i = 0; i < tr.images.currentElements; i++ )
		{
			image = Com_GrowListElement( &tr.images, i );

			/*
			   if(image->bits & (IF_RGBA16F | IF_RGBA32F | IF_LA16F | IF_LA32F))
			   {
			   // don't render float textures using FFP
			   continue;
			   }
			 */

			w = glConfig.vidWidth / 20;
			h = glConfig.vidHeight / 15;
			x = i % 20 * w;
			y = i / 20 * h;

			// show in proportional size in mode 2
			if( r_showImages->integer == 2 )
			{
				w *= image->uploadWidth / 512.0f;
				h *= image->uploadHeight / 512.0f;
			}

			// bind u_ColorMap
			GL_Bind( image );

			VectorSet4( quadVerts[ 0 ], x, y, 0, 1 );
			VectorSet4( quadVerts[ 1 ], x + w, y, 0, 1 );
			VectorSet4( quadVerts[ 2 ], x + w, y + h, 0, 1 );
			VectorSet4( quadVerts[ 3 ], x, y + h, 0, 1 );

			Tess_InstantQuad( quadVerts );

			/*
			   qglBegin(GL_QUADS);
			   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, 0, 0, 0, 1);
			   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x, y, 0, 1);
			   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, 1, 0, 0, 1);
			   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x + w, y, 0, 1);
			   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, 1, 1, 0, 1);
			   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x + w, y + h, 0, 1);
			   qglVertexAttrib4fARB(ATTR_INDEX_TEXCOORD0, 0, 1, 0, 1);
			   qglVertexAttrib4fARB(ATTR_INDEX_POSITION, x, y + h, 0, 1);
			   qglEnd();
			 */
		}

		qglFinish();

		end = ri.Milliseconds();
		ri.Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );

		GL_CheckErrors();
	}

#endif

	/*
	=============
	RB_SwapBuffers
	=============
	*/
	const void     *RB_SwapBuffers( const void *data )
	{
		const swapBuffersCommand_t *cmd;

		// finish any 2D drawing if needed
		if( tess.numIndexes )
		{
			Tess_End();
		}

		// texture swapping test
#if !defined( USE_D3D10 )

		if( r_showImages->integer )
		{
			RB_ShowImages();
		}

#endif

		cmd = ( const swapBuffersCommand_t * ) data;

#if defined( USE_D3D10 )
		// TODO
#else

		// we measure overdraw by reading back the stencil buffer and
		// counting up the number of increments that have happened
		if( r_measureOverdraw->integer )
		{
			int           i;
			long          sum = 0;
			unsigned char *stencilReadback;

			stencilReadback = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
			qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

			for( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ )
			{
				sum += stencilReadback[ i ];
			}

			backEnd.pc.c_overDraw += sum;
			ri.Hunk_FreeTempMemory( stencilReadback );
		}

#endif

#if defined( USE_D3D10 )
		// TODO
#else

		if( !glState.finishCalled )
		{
			qglFinish();
		}

#endif

		GLimp_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

		// present the information rendered to the back buffer to the front buffer (the screen)
		dx.swapChain->Present( 0, 0 );

		if( r_fullscreen->modified )
		{
			bool        fullscreen;
			bool        needToToggle = qtrue;
			bool        sdlToggled = qfalse;
			SDL_Surface *s = SDL_GetVideoSurface();

			if( s )
			{
				// Find out the current state
				fullscreen = !!( s->flags & SDL_FULLSCREEN );

				if( r_fullscreen->integer > 0 && ri.Cvar_VariableIntegerValue( "in_nograb" ) > 0 )
				{
					ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n" );
					ri.Cvar_Set( "r_fullscreen", "0" );
					r_fullscreen->modified = qfalse;
				}

				// Is the state we want different from the current state?
				needToToggle = !!r_fullscreen->integer != fullscreen;

				if( needToToggle )
				{
					sdlToggled = SDL_WM_ToggleFullScreen( s );
				}
			}

			if( needToToggle )
			{
				// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
				if( !sdlToggled )
				{
					ri.Cmd_ExecuteText( EXEC_APPEND, "vid_restart" );
				}

				ri.IN_Restart();
			}

			r_fullscreen->modified = qfalse;
		}

		backEnd.projection2D = qfalse;

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

		GLimp_LogComment( "--- RB_ExecuteRenderCommands ---\n" );

		t1 = ri.Milliseconds();

		if( !r_smp->integer || data == backEndData[ 0 ]->commands.cmds )
		{
			backEnd.smpFrame = 0;
		}
		else
		{
			backEnd.smpFrame = 1;
		}

		while( 1 )
		{
			switch( * ( const int * ) data )
			{
				case RC_SET_COLOR:
					data = RB_SetColor( data );
					break;

				case RC_STRETCH_PIC:
					data = RB_StretchPic( data );
					break;

				case RC_DRAW_VIEW:
					data = RB_DrawView( data );
					break;

				case RC_DRAW_BUFFER:
					data = RB_DrawBuffer( data );
					break;

				case RC_SWAP_BUFFERS:
					data = RB_SwapBuffers( data );
					break;

				case RC_SCREENSHOT:
					data = RB_TakeScreenshotCmd( data );
					break;

				case RC_VIDEOFRAME:
					data = RB_TakeVideoFrameCmd( data );
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
		while( 1 )
		{
			// sleep until we have work to do
			data = GLimp_RendererSleep();

			if( !data )
			{
				return; // all done, renderer is shutting down
			}

			renderThreadActive = qtrue;

			RB_ExecuteRenderCommands( data );

			renderThreadActive = qfalse;
		}
	}

#if defined( __cplusplus )
}
#endif
