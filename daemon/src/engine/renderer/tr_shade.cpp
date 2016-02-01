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
// tr_shade.c
#include "tr_local.h"
#include "gl_shader.h"

/*
=================================================================================
THIS ENTIRE FILE IS BACK END!

This file deals with applying shaders to surface data in the tess struct.
=================================================================================
*/

static void GLSL_InitGPUShadersOrError()
{
	// make sure the render thread is stopped
	R_SyncRenderThread();

	GL_CheckErrors();

	// single texture rendering
	gl_shaderManager.load( gl_genericShader );

	// simple vertex color shading for entities
	gl_shaderManager.load( gl_vertexLightingShader_DBS_entity );

	// simple vertex color shading for the world
	gl_shaderManager.load( gl_vertexLightingShader_DBS_world );

	// standard light mapping
	gl_shaderManager.load( gl_lightMappingShader );

	// omni-directional specular bump mapping ( Doom3 style )
	gl_shaderManager.load( gl_forwardLightingShader_omniXYZ );

	// projective lighting ( Doom3 style )
	gl_shaderManager.load( gl_forwardLightingShader_projXYZ );

	// directional sun lighting ( Doom3 style )
	gl_shaderManager.load( gl_forwardLightingShader_directionalSun );

#if !defined( GLSL_COMPILE_STARTUP_ONLY )

	gl_shaderManager.load( gl_depthToColorShader );

#endif // #if !defined(GLSL_COMPILE_STARTUP_ONLY)

	// shadowmap distance compression
	gl_shaderManager.load( gl_shadowFillShader );

	// bumped cubemap reflection for abitrary polygons ( EMBM )
	gl_shaderManager.load( gl_reflectionShader );

	// skybox drawing for abitrary polygons
	gl_shaderManager.load( gl_skyboxShader );

	// Q3A volumetric fog
	gl_shaderManager.load( gl_fogQuake3Shader );

	// global fog post process effect
	gl_shaderManager.load( gl_fogGlobalShader );

	// heatHaze post process effect
	gl_shaderManager.load( gl_heatHazeShader );

	// screen post process effect
	gl_shaderManager.load( gl_screenShader );

	// portal process effect
	gl_shaderManager.load( gl_portalShader );

	// LDR bright pass filter
	gl_shaderManager.load( gl_contrastShader );

	// camera post process effect
	gl_shaderManager.load( gl_cameraEffectsShader );

	// gaussian blur
	gl_shaderManager.load( gl_blurXShader );

	gl_shaderManager.load( gl_blurYShader );

	// debug utils
	gl_shaderManager.load( gl_debugShadowMapShader );

#if !defined( GLSL_COMPILE_STARTUP_ONLY )

	gl_shaderManager.load( gl_liquidShader );

	gl_shaderManager.load( gl_volumetricFogShader );

#endif // #if !defined(GLSL_COMPILE_STARTUP_ONLY)

	gl_shaderManager.load( gl_motionblurShader );

	if (GLEW_ARB_texture_gather)
	{
		if (r_ssao->integer)
		{
			gl_shaderManager.load(gl_ssaoShader);
		}
	}
	else
	{
		Log::Warn("SSAO not used because GL_ARB_texture_gather is not available.");
	}

	gl_shaderManager.load( gl_depthtile1Shader );
	gl_shaderManager.load( gl_depthtile2Shader );
	gl_shaderManager.load( gl_lighttileShader );
	gl_shaderManager.load( gl_fxaaShader );

	if ( !r_lazyShaders->integer )
	{
		gl_shaderManager.buildAll();
	}
}

void GLSL_InitGPUShaders()
{
	/*
	 Without a shaderpath option, the shader debugging cycle is like this:
	 1. Change shader file(s).
	 2. Run script to convert shader files into c++, storing them in shaders.cpp
	 3. Recompile app to pickup the new shaders.cpp changes.
	 4. Run the app and get to the point required to check work.
	 5. If the change failed or succeeded but you want to make more changes restart at step 1.

	 Alternatively, if set shaderpath "c:/unvanquished/main" is used, the cylcle is:
	 1. Change shader file(s) - don't run the buildshaders script unless samples.cpp is missing.
	 2. Start the app, the app will load the shader files directly.
	    If there is a problem the app will revert to the last working changes
		in samples.cpp, so need to restart the app.
	 3. Fix the problem shader files
	 4. Do /glsl_restart at the app console to reload them. Repeat from step 3 as needed.

	 Note that unv will respond by listing the files it thinks are different.
	 If this matches your expectations then it's not an error.
	 Note foward slashes (like those used in windows pathnames are processed
	 as escape characters by the Unvanquished command processor,
	 so use two forward slashes in that case.
	 */

	auto shaderPath = GetShaderPath();
	if (shaderPath.empty())
		shaderKind = ShaderKind::BuiltIn;
	else
		shaderKind = ShaderKind::External;

	bool externalFailed = false;
	if (shaderKind == ShaderKind::External)
	{
		try
		{
			Log::Warn("Loading external shaders.");
			GLSL_InitGPUShadersOrError();
			Log::Warn("External shaders in use.");
		}
		catch (const ShaderException& e)
		{
			Log::Warn("External shaders failed: %s", e.what());
			Log::Warn("Attempting to use built in shaders instead.");
			shaderKind = ShaderKind::BuiltIn;
			externalFailed = true;
		}
	}

	if (shaderKind == ShaderKind::BuiltIn)
	{
		// Let the user know if we are transitioning from external to
		// built-in shaders. We won't alert them if we were already using
		// built-in shaders as this is the normal case.
		try
		{
			GLSL_InitGPUShadersOrError();
		}
		catch (const ShaderException&e)
		{
			Sys::Error("Built-in shaders failed: %s", e.what()); // Fatal.
		};
		if (externalFailed)
			Log::Warn("Now using built-in shaders because external shaders failed.");
	}
}

void GLSL_ShutdownGPUShaders()
{
	R_SyncRenderThread();

	gl_shaderManager.freeAll();

	gl_genericShader = nullptr;
	gl_vertexLightingShader_DBS_entity = nullptr;
	gl_vertexLightingShader_DBS_world = nullptr;
	gl_lightMappingShader = nullptr;
	gl_forwardLightingShader_omniXYZ = nullptr;
	gl_forwardLightingShader_projXYZ = nullptr;
	gl_forwardLightingShader_directionalSun = nullptr;
	gl_depthToColorShader = nullptr;
	gl_shadowFillShader = nullptr;
	gl_lightVolumeShader_omni = nullptr;
	gl_reflectionShader = nullptr;
	gl_skyboxShader = nullptr;
	gl_fogQuake3Shader = nullptr;
	gl_fogGlobalShader = nullptr;
	gl_heatHazeShader = nullptr;
	gl_screenShader = nullptr;
	gl_portalShader = nullptr;
	gl_contrastShader = nullptr;
	gl_cameraEffectsShader = nullptr;
	gl_blurXShader = nullptr;
	gl_blurYShader = nullptr;
	gl_debugShadowMapShader = nullptr;
	gl_liquidShader = nullptr;
	gl_volumetricFogShader = nullptr;
	gl_motionblurShader = nullptr;
	gl_ssaoShader = nullptr;
	gl_depthtile1Shader = nullptr;
	gl_depthtile2Shader = nullptr;
	gl_lighttileShader = nullptr;
	gl_fxaaShader = nullptr;

	GL_BindNullProgram();
}

void GLSL_FinishGPUShaders()
{
	R_SyncRenderThread();

	gl_shaderManager.buildAll();
}

/*
==================
Tess_DrawElements
==================
*/
void Tess_DrawElements()
{
	int i;

	if ( ( tess.numIndexes == 0 || tess.numVertexes == 0 ) && tess.multiDrawPrimitives == 0 )
	{
		return;
	}

	// move tess data through the GPU, finally
	if ( glState.currentVBO && glState.currentIBO )
	{
		if ( tess.multiDrawPrimitives )
		{
			glMultiDrawElements( GL_TRIANGLES, tess.multiDrawCounts, GL_INDEX_TYPE, ( const GLvoid ** ) tess.multiDrawIndexes, tess.multiDrawPrimitives );

			backEnd.pc.c_multiDrawElements++;
			backEnd.pc.c_multiDrawPrimitives += tess.multiDrawPrimitives;

			backEnd.pc.c_vboVertexes += tess.numVertexes;

			for ( i = 0; i < tess.multiDrawPrimitives; i++ )
			{
				backEnd.pc.c_multiVboIndexes += tess.multiDrawCounts[ i ];
				backEnd.pc.c_indexes += tess.multiDrawCounts[ i ];
			}
		}
		else
		{
			uint32_t base = 0;

			if( glState.currentIBO == tess.ibo ) {
				base = tess.indexBase * sizeof( glIndex_t );
			}

			glDrawRangeElements( GL_TRIANGLES, 0, tess.numVertexes, tess.numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET( base ) );

			backEnd.pc.c_drawElements++;

			backEnd.pc.c_vboVertexes += tess.numVertexes;
			backEnd.pc.c_vboIndexes += tess.numIndexes;

			backEnd.pc.c_indexes += tess.numIndexes;
			backEnd.pc.c_vertexes += tess.numVertexes;
		}
	}
	else
	{
		glDrawElements( GL_TRIANGLES, tess.numIndexes, GL_INDEX_TYPE, tess.indexes );

		backEnd.pc.c_drawElements++;

		backEnd.pc.c_indexes += tess.numIndexes;
		backEnd.pc.c_vertexes += tess.numVertexes;
	}
}

/*
==================
Tess_DrawArrays
==================
*/
void Tess_DrawArrays( GLenum elementType )
{
	if ( tess.numVertexes == 0 )
	{
		return;
	}

	// move tess data through the GPU, finally
	glDrawArrays( elementType, 0, tess.numVertexes );

	backEnd.pc.c_drawElements++;

	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_vertexes += tess.numVertexes;

	if ( glState.currentVBO )
	{
		backEnd.pc.c_vboVertexes += tess.numVertexes;
		backEnd.pc.c_vboIndexes += tess.numIndexes;
	}
}

/*
=============================================================

SURFACE SHADERS

=============================================================
*/

ALIGNED( 16, shaderCommands_t tess );

/*
=================
BindLightMap
=================
*/
static void BindLightMap( int tmu )
{
	image_t *lightmap;

	if ( tr.fatLightmap )
	{
		lightmap = tr.fatLightmap;
	}
	else if ( tess.lightmapNum >= 0 && tess.lightmapNum < tr.lightmaps.currentElements )
	{
		lightmap = ( image_t * ) Com_GrowListElement( &tr.lightmaps, tess.lightmapNum );
	}
	else
	{
		lightmap = nullptr;
	}

	if ( !tr.lightmaps.currentElements || !lightmap )
	{
		GL_BindToTMU( tmu, tr.whiteImage );
		return;
	}

	GL_BindToTMU( tmu, lightmap );
}

/*
=================
BindDeluxeMap
=================
*/
static void BindDeluxeMap( int tmu )
{
	image_t *deluxemap;

	if ( tess.lightmapNum >= 0 && tess.lightmapNum < tr.deluxemaps.currentElements )
	{
		deluxemap = ( image_t * ) Com_GrowListElement( &tr.deluxemaps, tess.lightmapNum );
	}
	else
	{
		deluxemap = nullptr;
	}

	if ( !tr.deluxemaps.currentElements || !deluxemap )
	{
		GL_BindToTMU( tmu, tr.flatImage );
		return;
	}

	GL_BindToTMU( tmu, deluxemap );
}

/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris()
{
	int deform = 0;

	GLimp_LogComment( "--- DrawTris ---\n" );

	gl_genericShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_genericShader->SetVertexAnimation( tess.vboVertexAnimation );

	if( tess.surfaceShader->autoSpriteMode ) {
		gl_genericShader->EnableVertexSprite();
		tess.vboVertexSprite = true;
	} else {
		gl_genericShader->DisableVertexSprite();
		tess.vboVertexSprite = false;
	}

	gl_genericShader->DisableTCGenEnvironment();
	gl_genericShader->DisableTCGenLightmap();
	gl_genericShader->DisableMacro_USE_DEPTH_FADE();

	if( tess.surfaceShader->stages[0] ) {
		deform = tess.surfaceShader->stages[0]->deformIndex;
	}

	gl_genericShader->BindProgram( deform );

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );

	gl_genericShader->SetUniform_AlphaTest( GLS_ATEST_NONE );

	if ( r_showBatches->integer || r_showLightBatches->integer )
	{
		gl_genericShader->SetUniform_Color( Color::Color::Indexed( backEnd.pc.c_batches % 8 ) );
	}
	else if ( glState.currentVBO == tess.vbo )
	{
		gl_genericShader->SetUniform_Color( Color::Red );
	}
	else if ( glState.currentVBO )
	{
		gl_genericShader->SetUniform_Color( Color::Blue );
	}
	else
	{
		gl_genericShader->SetUniform_Color( Color::White );
	}

	gl_genericShader->SetUniform_ColorModulate( colorGen_t::CGEN_CONST, alphaGen_t::AGEN_CONST );

	gl_genericShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_genericShader->SetUniform_Bones( tess.numBones, tess.bones );
	}

	// u_DeformGen
	gl_genericShader->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	// bind u_ColorMap
	GL_BindToTMU( 0, tr.whiteImage );
	gl_genericShader->SetUniform_ColorTextureMatrix( tess.svars.texMatrices[ TB_COLORMAP ] );
	gl_genericShader->SetRequiredVertexPointers();

	glDepthRange( 0, 0 );

	Tess_DrawElements();

	glDepthRange( 0, 1 );
}

/*
==============
Tess_Begin

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a Tess_End due
to overflow.
==============
*/
// *INDENT-OFF*
void Tess_Begin( void ( *stageIteratorFunc )(),
                 void ( *stageIteratorFunc2 )(),
                 shader_t *surfaceShader, shader_t *lightShader,
                 bool skipTangentSpaces,
                 bool skipVBO,
                 int lightmapNum,
                 int fogNum )
{
	shader_t *state;

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.attribsSet = 0;
	tess.multiDrawPrimitives = 0;

	// materials are optional
	if ( surfaceShader != nullptr )
	{
		state = ( surfaceShader->remappedShader ) ? surfaceShader->remappedShader : surfaceShader;

		tess.surfaceShader = state;
		tess.surfaceStages = state->stages;
		tess.numSurfaceStages = state->numStages;

		Tess_MapVBOs( false );
	}
	else
	{
		state = nullptr;

		tess.numSurfaceStages = 0;
		tess.surfaceShader = nullptr;
		tess.surfaceStages = nullptr;
		Tess_MapVBOs( false );
	}


	bool isSky = ( state != nullptr && state->isSky != false );

	tess.lightShader = lightShader;

	tess.stageIteratorFunc = stageIteratorFunc;
	tess.stageIteratorFunc2 = stageIteratorFunc2;

	if ( !tess.stageIteratorFunc )
	{
		ri.Error( errorParm_t::ERR_FATAL, "tess.stageIteratorFunc == NULL" );
	}

	if ( tess.stageIteratorFunc == &Tess_StageIteratorGeneric )
	{
		if ( isSky )
		{
			tess.stageIteratorFunc = &Tess_StageIteratorSky;
			tess.stageIteratorFunc2 = &Tess_StageIteratorGeneric;
		}
	}
	else if ( tess.stageIteratorFunc == &Tess_StageIteratorDepthFill )
	{
		if ( isSky )
		{
			tess.stageIteratorFunc = &Tess_StageIteratorSky;
			tess.stageIteratorFunc2 = &Tess_StageIteratorDepthFill;
		}
	}

	tess.skipTangentSpaces = skipTangentSpaces;
	tess.skipVBO = skipVBO;
	tess.lightmapNum = lightmapNum;
	tess.fogNum = fogNum;

	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va( "--- Tess_Begin( surfaceShader = %s, lightShader = %s, skipTangentSpaces = %i, lightmapNum = %i, fogNum = %i) ---\n", tess.surfaceShader->name, tess.lightShader ? tess.lightShader->name : nullptr, tess.skipTangentSpaces, tess.lightmapNum, tess.fogNum ) );
	}
}

// *INDENT-ON*

static void Render_generic( int stage )
{
	shaderStage_t *pStage;
	colorGen_t    rgbGen;
	alphaGen_t    alphaGen;
	bool      needDepthMap = false;

	GLimp_LogComment( "--- Render_generic ---\n" );

	pStage = tess.surfaceStages[ stage ];

	GL_State( pStage->stateBits );

	// choose right shader program ----------------------------------
	gl_genericShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_genericShader->SetVertexAnimation( tess.vboVertexAnimation );

	gl_genericShader->SetTCGenEnvironment( pStage->tcGen_Environment );
	gl_genericShader->SetTCGenLightmap( pStage->tcGen_Lightmap );

	if( pStage->hasDepthFade ) {
		gl_genericShader->EnableMacro_USE_DEPTH_FADE();
		needDepthMap = true;
	} else {
		gl_genericShader->DisableMacro_USE_DEPTH_FADE();
	}

	if( backEnd.refdef.numShaderLights > 0 ) {
		gl_genericShader->EnableMacro_USE_SHADER_LIGHTS();
		GL_BindToTMU( 8, tr.lighttileRenderImage );
	} else {
		gl_genericShader->DisableMacro_USE_SHADER_LIGHTS();
	}

	if( tess.surfaceShader->autoSpriteMode ) {
		gl_genericShader->EnableVertexSprite();
		needDepthMap = true;
		tess.vboVertexSprite = true;
	} else {
		gl_genericShader->DisableVertexSprite();
		tess.vboVertexSprite = false;
	}

	gl_genericShader->BindProgram( pStage->deformIndex );
	// end choose right shader program ------------------------------

	// set uniforms
	if ( pStage->tcGen_Environment || tess.vboVertexSprite )
	{
		// calculate the environment texcoords in object space
		gl_genericShader->SetUniform_ViewOrigin( backEnd.orientation.viewOrigin );
		gl_genericShader->SetUniform_ViewUp( backEnd.orientation.axis[ 2 ] );
	}

	if( backEnd.refdef.numShaderLights > 0 ) {
		gl_genericShader->SetUniform_numLights( backEnd.refdef.numLights );
		gl_genericShader->SetUniformBlock_Lights( tr.dlightUBO );
	}

	// u_AlphaTest
	gl_genericShader->SetUniform_AlphaTest( pStage->stateBits );

	// u_ColorGen
	switch ( pStage->rgbGen )
	{
		case colorGen_t::CGEN_VERTEX:
		case colorGen_t::CGEN_ONE_MINUS_VERTEX:
			rgbGen = pStage->rgbGen;
			break;

		default:
			rgbGen = colorGen_t::CGEN_CONST;
			break;
	}

	// u_AlphaGen
	switch ( pStage->alphaGen )
	{
		case alphaGen_t::AGEN_VERTEX:
		case alphaGen_t::AGEN_ONE_MINUS_VERTEX:
			alphaGen = pStage->alphaGen;
			break;

		default:
			alphaGen = alphaGen_t::AGEN_CONST;
			break;
	}

	// u_ColorModulate
	gl_genericShader->SetUniform_ColorModulate( rgbGen, alphaGen );

	// u_Color
	gl_genericShader->SetUniform_Color( tess.svars.color );

	gl_genericShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_Bones
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_genericShader->SetUniform_Bones( tess.numBones, tess.bones );
	}

	// u_VertexInterpolation
	if ( tess.vboVertexAnimation )
	{
		gl_genericShader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// u_DeformGen
	gl_genericShader->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	// bind u_ColorMap
	GL_SelectTexture( 0 );
	BindAnimatedImage( &pStage->bundle[ TB_COLORMAP ] );
	gl_genericShader->SetUniform_ColorTextureMatrix( tess.svars.texMatrices[ TB_COLORMAP ] );

	if( pStage->hasDepthFade ) {
		gl_genericShader->SetUniform_DepthScale( pStage->depthFadeValue );
	}
	if( needDepthMap ) {
		GL_BindToTMU( 1, tr.depthRenderImage );
	}

	gl_genericShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_vertexLighting_DBS_entity( int stage )
{
	vec3_t        viewOrigin;
	uint32_t      stateBits;
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_vertexLighting_DBS_entity ---\n" );

	stateBits = pStage->stateBits;

	GL_State( stateBits );

	bool normalMapping = r_normalMapping->integer && ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] != nullptr );
	bool glowMapping = ( pStage->bundle[ TB_GLOWMAP ].image[ 0 ] != nullptr );

	// choose right shader program ----------------------------------
	gl_vertexLightingShader_DBS_entity->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_vertexLightingShader_DBS_entity->SetVertexAnimation( tess.vboVertexAnimation );

	tess.vboVertexSprite = false;

	if( backEnd.refdef.numShaderLights > 0 ) {
		gl_vertexLightingShader_DBS_entity->EnableMacro_USE_SHADER_LIGHTS();
		GL_BindToTMU( 8, tr.lighttileRenderImage );
	} else {
		gl_vertexLightingShader_DBS_entity->DisableMacro_USE_SHADER_LIGHTS();
	}

	gl_vertexLightingShader_DBS_entity->SetNormalMapping( normalMapping );
	gl_vertexLightingShader_DBS_entity->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );

	gl_vertexLightingShader_DBS_entity->SetReflectiveSpecular( normalMapping && tr.cubeHashTable != nullptr );

	gl_vertexLightingShader_DBS_entity->SetGlowMapping( glowMapping );

	gl_vertexLightingShader_DBS_entity->BindProgram( pStage->deformIndex );

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_vertexLightingShader_DBS_entity->SetUniform_Bones( tess.numBones, tess.bones );
	}

	// set uniforms
	VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );  // in world space

	if( backEnd.refdef.numShaderLights > 0 ) {
		gl_vertexLightingShader_DBS_entity->SetUniform_numLights( backEnd.refdef.numLights );
		gl_vertexLightingShader_DBS_entity->SetUniformBlock_Lights( tr.dlightUBO );
	}

	// u_AlphaTest
	gl_vertexLightingShader_DBS_entity->SetUniform_AlphaTest( pStage->stateBits );

	gl_vertexLightingShader_DBS_entity->SetUniform_ViewOrigin( viewOrigin );

	gl_vertexLightingShader_DBS_entity->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_vertexLightingShader_DBS_entity->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	if( tr.world ) {
		gl_vertexLightingShader_DBS_entity->SetUniform_LightGridOrigin( tr.world->lightGridGLOrigin );
		gl_vertexLightingShader_DBS_entity->SetUniform_LightGridScale( tr.world->lightGridGLScale );
	}

	// u_VertexInterpolation
	if ( tess.vboVertexAnimation )
	{
		gl_vertexLightingShader_DBS_entity->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// u_DeformGen
	gl_vertexLightingShader_DBS_entity->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	if ( r_parallaxMapping->integer && tess.surfaceShader->parallax )
	{
		float depthScale;

		depthScale = RB_EvalExpression( &pStage->depthScaleExp, r_parallaxDepthScale->value );
		gl_vertexLightingShader_DBS_entity->SetUniform_DepthScale( depthScale );
	}

	// bind u_DiffuseMap
	GL_BindToTMU( 0, pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
	gl_vertexLightingShader_DBS_entity->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );

	if ( normalMapping )
	{
		// bind u_NormalMap
		if ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 1, pStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 1, tr.flatImage );
		}

		gl_vertexLightingShader_DBS_entity->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		if ( pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 2, pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 2, tr.blackImage );
		}

		gl_vertexLightingShader_DBS_entity->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );

		float minSpec = RB_EvalExpression( &pStage->specularExponentMin, r_specularExponentMin->value );
		float maxSpec = RB_EvalExpression( &pStage->specularExponentMax, r_specularExponentMax->value );

		gl_vertexLightingShader_DBS_entity->SetUniform_SpecularExponent( minSpec, maxSpec );

		if ( tr.cubeHashTable != nullptr )
		{
			cubemapProbe_t *cubeProbeNearest;
			cubemapProbe_t *cubeProbeSecondNearest;

			if ( backEnd.currentEntity && ( backEnd.currentEntity != &tr.worldEntity ) )
			{
				R_FindTwoNearestCubeMaps( backEnd.currentEntity->e.origin, &cubeProbeNearest, &cubeProbeSecondNearest );
			}
			else
			{
				// FIXME position
				R_FindTwoNearestCubeMaps( backEnd.viewParms.orientation.origin, &cubeProbeNearest, &cubeProbeSecondNearest );
			}

			if ( cubeProbeNearest == nullptr && cubeProbeSecondNearest == nullptr )
			{
				GLimp_LogComment( "cubeProbeNearest && cubeProbeSecondNearest == NULL\n" );

				// bind u_EnvironmentMap0
				GL_BindToTMU( 3, tr.whiteCubeImage );

				// bind u_EnvironmentMap1
				GL_BindToTMU( 4, tr.whiteCubeImage );
			}
			else if ( cubeProbeNearest == nullptr )
			{
				GLimp_LogComment( "cubeProbeNearest == NULL\n" );

				// bind u_EnvironmentMap0
				GL_BindToTMU( 3, cubeProbeSecondNearest->cubemap );

				// u_EnvironmentInterpolation
				gl_vertexLightingShader_DBS_entity->SetUniform_EnvironmentInterpolation( 0.0 );
			}
			else if ( cubeProbeSecondNearest == nullptr )
			{
				GLimp_LogComment( "cubeProbeSecondNearest == NULL\n" );

				// bind u_EnvironmentMap0
				GL_BindToTMU( 3, cubeProbeNearest->cubemap );

				// bind u_EnvironmentMap1
				//GL_SelectTexture(4);
				//GL_Bind(cubeProbeNearest->cubemap);

				// u_EnvironmentInterpolation
				gl_vertexLightingShader_DBS_entity->SetUniform_EnvironmentInterpolation( 0.0 );
			}
			else
			{
				float cubeProbeNearestDistance, cubeProbeSecondNearestDistance;

				if ( backEnd.currentEntity && ( backEnd.currentEntity != &tr.worldEntity ) )
				{
					cubeProbeNearestDistance = Distance( backEnd.currentEntity->e.origin, cubeProbeNearest->origin );
					cubeProbeSecondNearestDistance = Distance( backEnd.currentEntity->e.origin, cubeProbeSecondNearest->origin );
				}
				else
				{
					// FIXME position
					cubeProbeNearestDistance = Distance( backEnd.viewParms.orientation.origin, cubeProbeNearest->origin );
					cubeProbeSecondNearestDistance = Distance( backEnd.viewParms.orientation.origin, cubeProbeSecondNearest->origin );
				}

				float interpolate = cubeProbeNearestDistance / ( cubeProbeNearestDistance + cubeProbeSecondNearestDistance );

				if ( r_logFile->integer )
				{
					GLimp_LogComment( va( "cubeProbeNearestDistance = %f, cubeProbeSecondNearestDistance = %f, interpolation = %f\n",
					                      cubeProbeNearestDistance, cubeProbeSecondNearestDistance, interpolate ) );
				}

				// bind u_EnvironmentMap0
				GL_BindToTMU( 3, cubeProbeNearest->cubemap );

				// bind u_EnvironmentMap1
				GL_BindToTMU( 4, cubeProbeSecondNearest->cubemap );

				// u_EnvironmentInterpolation
				gl_vertexLightingShader_DBS_entity->SetUniform_EnvironmentInterpolation( interpolate );
			}
		}
	}

	if ( glowMapping )
	{
		GL_BindToTMU( 5, pStage->bundle[ TB_GLOWMAP ].image[ 0 ] );

		gl_vertexLightingShader_DBS_entity->SetUniform_GlowTextureMatrix( tess.svars.texMatrices[ TB_GLOWMAP ] );
	}

	if ( tr.lightGrid1Image && tr.lightGrid2Image ) {
		GL_BindToTMU( 6, tr.lightGrid1Image );
		GL_BindToTMU( 7, tr.lightGrid2Image );
	}

	gl_vertexLightingShader_DBS_entity->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_vertexLighting_DBS_world( int stage )
{
	vec3_t        viewOrigin;
	uint32_t      stateBits;
	colorGen_t    colorGen;
	alphaGen_t    alphaGen;
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_vertexLighting_DBS_world ---\n" );

	stateBits = pStage->stateBits;

	bool normalMapping = r_normalMapping->integer && ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] != nullptr );
	bool glowMapping = ( pStage->bundle[ TB_GLOWMAP ].image[ 0 ] != nullptr );

	// choose right shader program ----------------------------------
	if( backEnd.refdef.numShaderLights > 0 ) {
		gl_vertexLightingShader_DBS_world->EnableMacro_USE_SHADER_LIGHTS();
		GL_BindToTMU( 8, tr.lighttileRenderImage );
	} else {
		gl_vertexLightingShader_DBS_world->DisableMacro_USE_SHADER_LIGHTS();
	}

	gl_vertexLightingShader_DBS_world->SetNormalMapping( normalMapping );
	gl_vertexLightingShader_DBS_world->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );
	gl_vertexLightingShader_DBS_world->SetGlowMapping( glowMapping );

	tess.vboVertexSprite = false;

	gl_vertexLightingShader_DBS_world->BindProgram( pStage->deformIndex );

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// set uniforms
	VectorCopy( backEnd.orientation.viewOrigin, viewOrigin );

	if( backEnd.refdef.numShaderLights > 0 ) {
		gl_vertexLightingShader_DBS_world->SetUniform_numLights( backEnd.refdef.numLights );
		gl_vertexLightingShader_DBS_world->SetUniformBlock_Lights( tr.dlightUBO );
	}

	GL_CheckErrors();

	// u_DeformGen
	gl_vertexLightingShader_DBS_world->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	// u_ColorModulate
	switch ( pStage->rgbGen )
	{
		case colorGen_t::CGEN_VERTEX:
		case colorGen_t::CGEN_ONE_MINUS_VERTEX:
			colorGen = pStage->rgbGen;
			break;

		default:
			colorGen = colorGen_t::CGEN_CONST;
			break;
	}

	switch ( pStage->alphaGen )
	{
		case alphaGen_t::AGEN_VERTEX:
			alphaGen = pStage->alphaGen;
			break;

		case alphaGen_t::AGEN_ONE_MINUS_VERTEX:
			alphaGen = pStage->alphaGen;
			break;

		default:
			alphaGen = alphaGen_t::AGEN_CONST;
			break;
	}

	GL_State( stateBits );

	gl_vertexLightingShader_DBS_world->SetUniform_ColorModulate( colorGen, alphaGen );

	// u_Color
	{
		gl_vertexLightingShader_DBS_world->SetUniform_Color( tess.svars.color );
	}

	gl_vertexLightingShader_DBS_world->SetUniform_LightWrapAround( RB_EvalExpression( &pStage->wrapAroundLightingExp, 0 ) );

	gl_vertexLightingShader_DBS_world->SetUniform_ViewOrigin( viewOrigin );
	gl_vertexLightingShader_DBS_world->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	gl_vertexLightingShader_DBS_world->SetUniform_AlphaTest( pStage->stateBits );
	if ( r_parallaxMapping->integer )
	{
		float depthScale;

		depthScale = RB_EvalExpression( &pStage->depthScaleExp, r_parallaxDepthScale->value );
		gl_vertexLightingShader_DBS_world->SetUniform_DepthScale( depthScale );
	}

	if( tr.world ) {
		gl_vertexLightingShader_DBS_world->SetUniform_LightGridOrigin( tr.world->lightGridGLOrigin );
		gl_vertexLightingShader_DBS_world->SetUniform_LightGridScale( tr.world->lightGridGLScale );
	}

	// bind u_DiffuseMap
	GL_BindToTMU( 0, pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
	gl_vertexLightingShader_DBS_world->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );

	if ( normalMapping )
	{
		// bind u_NormalMap
		if ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 1, pStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 1, tr.flatImage );
		}

		gl_vertexLightingShader_DBS_world->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		if ( pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 2, pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 2, tr.blackImage );
		}

		gl_vertexLightingShader_DBS_world->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );

		float minSpec = RB_EvalExpression( &pStage->specularExponentMin, r_specularExponentMin->value );
		float maxSpec = RB_EvalExpression( &pStage->specularExponentMax, r_specularExponentMax->value );

		gl_vertexLightingShader_DBS_world->SetUniform_SpecularExponent( minSpec, maxSpec );
	}

	if ( tr.lightGrid1Image && tr.lightGrid2Image ) {
		GL_BindToTMU( 6, tr.lightGrid1Image );
		GL_BindToTMU( 7, tr.lightGrid2Image );
	}

	if ( glowMapping )
	{
		GL_BindToTMU( 3, pStage->bundle[ TB_GLOWMAP ].image[ 0 ] );

		gl_vertexLightingShader_DBS_world->SetUniform_GlowTextureMatrix( tess.svars.texMatrices[ TB_GLOWMAP ] );
	}

	gl_vertexLightingShader_DBS_world->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_lightMapping( int stage, bool asColorMap, bool normalMapping )
{
	shaderStage_t *pStage;
	uint32_t      stateBits;
	colorGen_t    rgbGen;
	alphaGen_t    alphaGen;
	bool glowMapping = false;

	GLimp_LogComment( "--- Render_lightMapping ---\n" );

	pStage = tess.surfaceStages[ stage ];

	stateBits = pStage->stateBits;

	switch ( pStage->rgbGen )
	{
		case colorGen_t::CGEN_VERTEX:
		case colorGen_t::CGEN_ONE_MINUS_VERTEX:
			rgbGen = pStage->rgbGen;
			break;

		default:
			rgbGen = colorGen_t::CGEN_CONST;
			break;
	}

	switch ( pStage->alphaGen )
	{
		case alphaGen_t::AGEN_VERTEX:
		case alphaGen_t::AGEN_ONE_MINUS_VERTEX:
			alphaGen = pStage->alphaGen;
			break;

		default:
			alphaGen = alphaGen_t::AGEN_CONST;
			break;
	}

	if ( r_showLightMaps->integer )
	{
		stateBits &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_ATEST_BITS );
	}

	GL_State( stateBits );

	if ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] == nullptr )
	{
		normalMapping = false;
	}

	if ( pStage->bundle[ TB_GLOWMAP ].image[ 0 ] != nullptr )
	{
		glowMapping = true;
	}

	// choose right shader program ----------------------------------
	if( backEnd.refdef.numShaderLights > 0 ) {
		gl_lightMappingShader->EnableMacro_USE_SHADER_LIGHTS();
		GL_BindToTMU( 8, tr.lighttileRenderImage );
	} else {
		gl_lightMappingShader->DisableMacro_USE_SHADER_LIGHTS();
	}

	gl_lightMappingShader->SetNormalMapping( normalMapping );
	gl_lightMappingShader->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );
	gl_lightMappingShader->SetGlowMapping( glowMapping );

	tess.vboVertexSprite = false;

	gl_lightMappingShader->BindProgram( pStage->deformIndex );

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	if( backEnd.refdef.numShaderLights > 0 ) {
		gl_lightMappingShader->SetUniform_numLights( backEnd.refdef.numLights );
		gl_lightMappingShader->SetUniformBlock_Lights( tr.dlightUBO );
	}

	// u_DeformGen
	gl_lightMappingShader->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	gl_lightMappingShader->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space

	gl_lightMappingShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_lightMappingShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
	gl_lightMappingShader->SetUniform_AlphaTest( pStage->stateBits );

	// u_ColorModulate
	gl_lightMappingShader->SetUniform_ColorModulate( rgbGen, alphaGen );

	// u_Color
	gl_lightMappingShader->SetUniform_Color( tess.svars.color );

	if ( r_parallaxMapping->integer )
	{
		float depthScale;

		depthScale = RB_EvalExpression( &pStage->depthScaleExp, r_parallaxDepthScale->value );
		gl_lightMappingShader->SetUniform_DepthScale( depthScale );
	}

	// bind u_DiffuseMap
	if ( asColorMap )
	{
		GL_BindToTMU( 0, tr.whiteImage );
	}
	else
	{
		GL_BindToTMU( 0, pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		gl_lightMappingShader->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );
	}

	if ( normalMapping )
	{
		// bind u_NormalMap
		if ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 1, pStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 1, tr.flatImage );
		}

		gl_lightMappingShader->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		if ( pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 2, pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 2, tr.blackImage );
		}

		gl_lightMappingShader->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );

		float specExpMin = RB_EvalExpression( &pStage->specularExponentMin, r_specularExponentMin->value );
		float specExpMax = RB_EvalExpression( &pStage->specularExponentMax, r_specularExponentMax->value );

		gl_lightMappingShader->SetUniform_SpecularExponent( specExpMin, specExpMax );

		// bind u_DeluxeMap
		BindDeluxeMap( 4 );
	}

	// bind u_LightMap
	BindLightMap( 3 );

	if ( glowMapping )
	{
		GL_BindToTMU( 5, pStage->bundle[ TB_GLOWMAP ].image[ 0 ] );

		gl_lightMappingShader->SetUniform_GlowTextureMatrix( tess.svars.texMatrices[ TB_GLOWMAP ] );
	}

	gl_lightMappingShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_depthFill( int stage )
{
	shaderStage_t *pStage;

	GLimp_LogComment( "--- Render_depthFill ---\n" );

	pStage = tess.surfaceStages[ stage ];

	uint32_t stateBits = pStage->stateBits;
	stateBits &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
	stateBits |= GLS_DEPTHMASK_TRUE | GLS_COLORMASK_BITS;

	GL_State( stateBits );

	gl_genericShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_genericShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_genericShader->SetTCGenEnvironment( pStage->tcGen_Environment );

	gl_genericShader->BindProgram( pStage->deformIndex );

	// set uniforms
	if ( pStage->tcGen_Environment )
	{
		// calculate the environment texcoords in object space
		gl_genericShader->SetUniform_ViewOrigin( backEnd.orientation.viewOrigin );
	}

	// u_AlphaTest
	gl_genericShader->SetUniform_AlphaTest( pStage->stateBits );

	// u_ColorModulate
	gl_genericShader->SetUniform_ColorModulate( colorGen_t::CGEN_CONST, alphaGen_t::AGEN_CONST );

	// u_Color
	Color::Color ambientColor;
	ambientColor.SetAlpha( 1 );

	gl_genericShader->SetUniform_Color( ambientColor );

	gl_genericShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_genericShader->SetUniform_Bones( tess.numBones, tess.bones );
	}

	// u_DeformGen
	gl_genericShader->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	// bind u_ColorMap
	if ( tess.surfaceShader->alphaTest )
	{
		GL_BindToTMU( 0, pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		gl_genericShader->SetUniform_ColorTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );
	}
	else
	{
		//GL_Bind(tr.defaultImage);
		GL_BindToTMU( 0, pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		gl_genericShader->SetUniform_ColorTextureMatrix( matrixIdentity );
	}

	gl_genericShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_shadowFill( int stage )
{
	shaderStage_t *pStage;
	uint32_t      stateBits;

	GLimp_LogComment( "--- Render_shadowFill ---\n" );

	pStage = tess.surfaceStages[ stage ];

	// remove blend modes
	stateBits = pStage->stateBits;
	stateBits &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );

	GL_State( stateBits );

	gl_shadowFillShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_shadowFillShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_shadowFillShader->SetMacro_LIGHT_DIRECTIONAL( backEnd.currentLight->l.rlType == refLightType_t::RL_DIRECTIONAL );

	gl_shadowFillShader->BindProgram( pStage->deformIndex );

	gl_shadowFillShader->SetRequiredVertexPointers();

	if ( r_debugShadowMaps->integer )
	{
		gl_shadowFillShader->SetUniform_Color( Color::Color::Indexed( backEnd.pc.c_batches % 8 ) );
	}

	gl_shadowFillShader->SetUniform_AlphaTest( pStage->stateBits );

	if ( backEnd.currentLight->l.rlType != refLightType_t::RL_DIRECTIONAL )
	{
		gl_shadowFillShader->SetUniform_LightOrigin( backEnd.currentLight->origin );
		gl_shadowFillShader->SetUniform_LightRadius( backEnd.currentLight->sphereRadius );
	}

	gl_shadowFillShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_shadowFillShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_Bones
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_shadowFillShader->SetUniform_Bones( tess.numBones, tess.bones );
	}

	// u_VertexInterpolation
	if ( tess.vboVertexAnimation )
	{
		gl_shadowFillShader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// u_DeformGen
	gl_shadowFillShader->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	// bind u_ColorMap
	if ( ( pStage->stateBits & GLS_ATEST_BITS ) != 0 )
	{
		GL_BindToTMU( 0, pStage->bundle[ TB_COLORMAP ].image[ 0 ] );
		gl_shadowFillShader->SetUniform_ColorTextureMatrix( tess.svars.texMatrices[ TB_COLORMAP ] );
	}
	else
	{
		GL_BindToTMU( 0, tr.whiteImage );
	}

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_forwardLighting_DBS_omni( shaderStage_t *diffuseStage,
    shaderStage_t *attenuationXYStage,
    shaderStage_t *attenuationZStage, trRefLight_t *light )
{
	vec3_t     viewOrigin;
	vec3_t     lightOrigin;
	float      shadowTexelSize;
	colorGen_t colorGen;
	alphaGen_t alphaGen;

	GLimp_LogComment( "--- Render_forwardLighting_DBS_omni ---\n" );

	bool normalMapping = r_normalMapping->integer && ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] != nullptr );

	bool shadowCompare = ( r_shadows->integer >= Util::ordinal(shadowingMode_t::SHADOWING_ESM16) && !light->l.noShadows && light->shadowLOD >= 0 );

	// choose right shader program ----------------------------------
	gl_forwardLightingShader_omniXYZ->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_forwardLightingShader_omniXYZ->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_forwardLightingShader_omniXYZ->SetNormalMapping( normalMapping );
	gl_forwardLightingShader_omniXYZ->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );

	gl_forwardLightingShader_omniXYZ->SetShadowing( shadowCompare );

	gl_forwardLightingShader_omniXYZ->BindProgram( diffuseStage->deformIndex );

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate
	switch ( diffuseStage->rgbGen )
	{
		case colorGen_t::CGEN_VERTEX:
		case colorGen_t::CGEN_ONE_MINUS_VERTEX:
			colorGen = diffuseStage->rgbGen;
			break;

		default:
			colorGen = colorGen_t::CGEN_CONST;
			break;
	}

	switch ( diffuseStage->alphaGen )
	{
		case alphaGen_t::AGEN_VERTEX:
		case alphaGen_t::AGEN_ONE_MINUS_VERTEX:
			alphaGen = diffuseStage->alphaGen;
			break;

		default:
			alphaGen = alphaGen_t::AGEN_CONST;
			break;
	}

	gl_forwardLightingShader_omniXYZ->SetUniform_ColorModulate( colorGen, alphaGen );

	// u_Color
	gl_forwardLightingShader_omniXYZ->SetUniform_Color( tess.svars.color );

	gl_forwardLightingShader_omniXYZ->SetUniform_AlphaTest( diffuseStage->stateBits );

	if ( r_parallaxMapping->integer )
	{
		float depthScale;

		depthScale = RB_EvalExpression( &diffuseStage->depthScaleExp, r_parallaxDepthScale->value );
		gl_forwardLightingShader_omniXYZ->SetUniform_DepthScale( depthScale );
	}

	// set uniforms
	VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );
	VectorCopy( light->origin, lightOrigin );
	Color::Color lightColor = tess.svars.color;

	if ( shadowCompare )
	{
		shadowTexelSize = 1.0f / shadowMapResolutions[ light->shadowLOD ];
	}
	else
	{
		shadowTexelSize = 1.0f;
	}

	gl_forwardLightingShader_omniXYZ->SetUniform_ViewOrigin( viewOrigin );

	gl_forwardLightingShader_omniXYZ->SetUniform_LightOrigin( lightOrigin );
	gl_forwardLightingShader_omniXYZ->SetUniform_LightColor( lightColor.ToArray() );
	gl_forwardLightingShader_omniXYZ->SetUniform_LightRadius( light->sphereRadius );
	gl_forwardLightingShader_omniXYZ->SetUniform_LightScale( light->l.scale );
	gl_forwardLightingShader_omniXYZ->SetUniform_LightWrapAround( RB_EvalExpression( &diffuseStage->wrapAroundLightingExp, 0 ) );
	gl_forwardLightingShader_omniXYZ->SetUniform_LightAttenuationMatrix( light->attenuationMatrix2 );

	GL_CheckErrors();

	if ( shadowCompare )
	{
		gl_forwardLightingShader_omniXYZ->SetUniform_ShadowTexelSize( shadowTexelSize );
		gl_forwardLightingShader_omniXYZ->SetUniform_ShadowBlur( r_shadowBlur->value );
	}

	GL_CheckErrors();

	gl_forwardLightingShader_omniXYZ->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_forwardLightingShader_omniXYZ->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_Bones
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_forwardLightingShader_omniXYZ->SetUniform_Bones( tess.numBones, tess.bones );
	}

	// u_VertexInterpolation
	if ( tess.vboVertexAnimation )
	{
		gl_forwardLightingShader_omniXYZ->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	gl_forwardLightingShader_omniXYZ->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	GL_CheckErrors();

	// bind u_DiffuseMap
	GL_BindToTMU( 0, diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
	gl_forwardLightingShader_omniXYZ->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );

	if ( normalMapping )
	{
		// bind u_NormalMap
		if ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 1, diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 1, tr.flatImage );
		}

		gl_forwardLightingShader_omniXYZ->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		if ( r_forceSpecular->integer )
		{
			GL_BindToTMU( 2, diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		}
		else if ( diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 2, diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 2, tr.blackImage );
		}

		gl_forwardLightingShader_omniXYZ->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );

		float minSpec = RB_EvalExpression( &diffuseStage->specularExponentMin, r_specularExponentMin->value );
		float maxSpec = RB_EvalExpression( &diffuseStage->specularExponentMax, r_specularExponentMax->value );

		gl_forwardLightingShader_omniXYZ->SetUniform_SpecularExponent( minSpec, maxSpec );
	}

	// bind u_AttenuationMapXY
	GL_SelectTexture( 3 );
	BindAnimatedImage( &attenuationXYStage->bundle[ TB_COLORMAP ] );

	// bind u_AttenuationMapZ
	GL_SelectTexture( 4 );
	BindAnimatedImage( &attenuationZStage->bundle[ TB_COLORMAP ] );

	// bind u_ShadowMap
	if ( shadowCompare )
	{
		GL_BindToTMU( 5, tr.shadowCubeFBOImage[ light->shadowLOD ] );
		GL_BindToTMU( 7, tr.shadowClipCubeFBOImage[ light->shadowLOD ] );
	}

	// bind u_RandomMap
	GL_BindToTMU( 6, tr.randomNormalsImage );

	gl_forwardLightingShader_omniXYZ->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_forwardLighting_DBS_proj( shaderStage_t *diffuseStage,
    shaderStage_t *attenuationXYStage,
    shaderStage_t *attenuationZStage, trRefLight_t *light )
{
	vec3_t     viewOrigin;
	vec3_t     lightOrigin;
	float      shadowTexelSize;
	colorGen_t colorGen;
	alphaGen_t alphaGen;

	GLimp_LogComment( "--- Render_fowardLighting_DBS_proj ---\n" );

	bool normalMapping = r_normalMapping->integer && ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] != nullptr );

	bool shadowCompare = ( r_shadows->integer >= Util::ordinal(shadowingMode_t::SHADOWING_ESM16) && !light->l.noShadows && light->shadowLOD >= 0 );

	// choose right shader program ----------------------------------
	gl_forwardLightingShader_projXYZ->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_forwardLightingShader_projXYZ->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_forwardLightingShader_projXYZ->SetNormalMapping( normalMapping );
	gl_forwardLightingShader_projXYZ->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );

	gl_forwardLightingShader_projXYZ->SetShadowing( shadowCompare );

	gl_forwardLightingShader_projXYZ->BindProgram( diffuseStage->deformIndex );

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate
	switch ( diffuseStage->rgbGen )
	{
		case colorGen_t::CGEN_VERTEX:
		case colorGen_t::CGEN_ONE_MINUS_VERTEX:
			colorGen = diffuseStage->rgbGen;
			break;

		default:
			colorGen = colorGen_t::CGEN_CONST;
			break;
	}

	switch ( diffuseStage->alphaGen )
	{
		case alphaGen_t::AGEN_VERTEX:
		case alphaGen_t::AGEN_ONE_MINUS_VERTEX:
			alphaGen = diffuseStage->alphaGen;
			break;

		default:
			alphaGen = alphaGen_t::AGEN_CONST;
			break;
	}

	gl_forwardLightingShader_projXYZ->SetUniform_ColorModulate( colorGen, alphaGen );

	// u_Color
	gl_forwardLightingShader_projXYZ->SetUniform_Color( tess.svars.color );

	gl_forwardLightingShader_projXYZ->SetUniform_AlphaTest( diffuseStage->stateBits );

	if ( r_parallaxMapping->integer )
	{
		float depthScale;

		depthScale = RB_EvalExpression( &diffuseStage->depthScaleExp, r_parallaxDepthScale->value );
		gl_forwardLightingShader_projXYZ->SetUniform_DepthScale( depthScale );
	}

	// set uniforms
	VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );
	VectorCopy( light->origin, lightOrigin );
	Color::Color lightColor = tess.svars.color;

	if ( shadowCompare )
	{
		shadowTexelSize = 1.0f / shadowMapResolutions[ light->shadowLOD ];
	}
	else
	{
		shadowTexelSize = 1.0f;
	}

	gl_forwardLightingShader_projXYZ->SetUniform_ViewOrigin( viewOrigin );

	gl_forwardLightingShader_projXYZ->SetUniform_LightOrigin( lightOrigin );
	gl_forwardLightingShader_projXYZ->SetUniform_LightColor( lightColor.ToArray() );
	gl_forwardLightingShader_projXYZ->SetUniform_LightRadius( light->sphereRadius );
	gl_forwardLightingShader_projXYZ->SetUniform_LightScale( light->l.scale );
	gl_forwardLightingShader_projXYZ->SetUniform_LightWrapAround( RB_EvalExpression( &diffuseStage->wrapAroundLightingExp, 0 ) );
	gl_forwardLightingShader_projXYZ->SetUniform_LightAttenuationMatrix( light->attenuationMatrix2 );

	GL_CheckErrors();

	if ( shadowCompare )
	{
		gl_forwardLightingShader_projXYZ->SetUniform_ShadowTexelSize( shadowTexelSize );
		gl_forwardLightingShader_projXYZ->SetUniform_ShadowBlur( r_shadowBlur->value );
		gl_forwardLightingShader_projXYZ->SetUniform_ShadowMatrix( light->shadowMatrices );
	}

	GL_CheckErrors();

	gl_forwardLightingShader_projXYZ->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_forwardLightingShader_projXYZ->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_Bones
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_forwardLightingShader_projXYZ->SetUniform_Bones( tess.numBones, tess.bones );
	}

	// u_VertexInterpolation
	if ( tess.vboVertexAnimation )
	{
		gl_forwardLightingShader_projXYZ->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	gl_forwardLightingShader_projXYZ->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	GL_CheckErrors();

	// bind u_DiffuseMap
	GL_BindToTMU( 0, diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
	gl_forwardLightingShader_projXYZ->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );

	if ( normalMapping )
	{
		// bind u_NormalMap
		if ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 1, diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 1, tr.flatImage );
		}

		gl_forwardLightingShader_projXYZ->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		if ( r_forceSpecular->integer )
		{
			GL_BindToTMU( 2, diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		}
		else if ( diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 2, diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 2, tr.blackImage );
		}

		gl_forwardLightingShader_projXYZ->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );

		float minSpec = RB_EvalExpression( &diffuseStage->specularExponentMin, r_specularExponentMin->value );
		float maxSpec = RB_EvalExpression( &diffuseStage->specularExponentMax, r_specularExponentMax->value );

		gl_forwardLightingShader_projXYZ->SetUniform_SpecularExponent( minSpec, maxSpec );
	}

	// bind u_AttenuationMapXY
	GL_SelectTexture( 3 );
	BindAnimatedImage( &attenuationXYStage->bundle[ TB_COLORMAP ] );

	// bind u_AttenuationMapZ
	GL_SelectTexture( 4 );
	BindAnimatedImage( &attenuationZStage->bundle[ TB_COLORMAP ] );

	// bind u_ShadowMap
	if ( shadowCompare )
	{
		GL_BindToTMU( 5, tr.shadowMapFBOImage[ light->shadowLOD ] );
		GL_BindToTMU( 7, tr.shadowClipMapFBOImage[ light->shadowLOD ] );
	}

	// bind u_RandomMap
	GL_BindToTMU( 6, tr.randomNormalsImage );

	gl_forwardLightingShader_projXYZ->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_forwardLighting_DBS_directional( shaderStage_t *diffuseStage, trRefLight_t *light )
{
	vec3_t     viewOrigin;
	vec3_t     lightDirection;
	float      shadowTexelSize;
	colorGen_t colorGen;
	alphaGen_t alphaGen;

	GLimp_LogComment( "--- Render_forwardLighting_DBS_directional ---\n" );

	bool normalMapping = r_normalMapping->integer && ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] != nullptr );

	bool shadowCompare = ( r_shadows->integer >= Util::ordinal(shadowingMode_t::SHADOWING_ESM16) && !light->l.noShadows && light->shadowLOD >= 0 );

	// choose right shader program ----------------------------------
	gl_forwardLightingShader_directionalSun->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_forwardLightingShader_directionalSun->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_forwardLightingShader_directionalSun->SetNormalMapping( normalMapping );
	gl_forwardLightingShader_directionalSun->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );

	gl_forwardLightingShader_directionalSun->SetShadowing( shadowCompare );

	gl_forwardLightingShader_directionalSun->BindProgram( diffuseStage->deformIndex );

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate
	switch ( diffuseStage->rgbGen )
	{
		case colorGen_t::CGEN_VERTEX:
		case colorGen_t::CGEN_ONE_MINUS_VERTEX:
			colorGen = diffuseStage->rgbGen;
			break;

		default:
			colorGen = colorGen_t::CGEN_CONST;
			break;
	}

	switch ( diffuseStage->alphaGen )
	{
		case alphaGen_t::AGEN_VERTEX:
		case alphaGen_t::AGEN_ONE_MINUS_VERTEX:
			alphaGen = diffuseStage->alphaGen;
			break;

		default:
			alphaGen = alphaGen_t::AGEN_CONST;
			break;
	}

	gl_forwardLightingShader_directionalSun->SetUniform_ColorModulate( colorGen, alphaGen );

	// u_Color
	gl_forwardLightingShader_directionalSun->SetUniform_Color( tess.svars.color );

	gl_forwardLightingShader_directionalSun->SetUniform_AlphaTest( diffuseStage->stateBits );

	if ( r_parallaxMapping->integer )
	{
		float depthScale;

		depthScale = RB_EvalExpression( &diffuseStage->depthScaleExp, r_parallaxDepthScale->value );
		gl_forwardLightingShader_directionalSun->SetUniform_DepthScale( depthScale );
	}

	// set uniforms
	VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );

	VectorCopy( tr.sunDirection, lightDirection );

	Color::Color lightColor = tess.svars.color;

	if ( shadowCompare )
	{
		shadowTexelSize = 1.0f / sunShadowMapResolutions[ light->shadowLOD ];
	}
	else
	{
		shadowTexelSize = 1.0f;
	}

	gl_forwardLightingShader_directionalSun->SetUniform_ViewOrigin( viewOrigin );

	gl_forwardLightingShader_directionalSun->SetUniform_LightDir( lightDirection );
	gl_forwardLightingShader_directionalSun->SetUniform_LightColor( lightColor.ToArray() );
	gl_forwardLightingShader_directionalSun->SetUniform_LightRadius( light->sphereRadius );
	gl_forwardLightingShader_directionalSun->SetUniform_LightScale( light->l.scale );
	gl_forwardLightingShader_directionalSun->SetUniform_LightWrapAround( RB_EvalExpression( &diffuseStage->wrapAroundLightingExp, 0 ) );
	gl_forwardLightingShader_directionalSun->SetUniform_LightAttenuationMatrix( light->attenuationMatrix2 );

	GL_CheckErrors();

	if ( shadowCompare )
	{
		gl_forwardLightingShader_directionalSun->SetUniform_ShadowMatrix( light->shadowMatricesBiased );
		gl_forwardLightingShader_directionalSun->SetUniform_ShadowParallelSplitDistances( backEnd.viewParms.parallelSplitDistances );
		gl_forwardLightingShader_directionalSun->SetUniform_ShadowTexelSize( shadowTexelSize );
		gl_forwardLightingShader_directionalSun->SetUniform_ShadowBlur( r_shadowBlur->value );
	}

	GL_CheckErrors();

	gl_forwardLightingShader_directionalSun->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_forwardLightingShader_directionalSun->SetUniform_ViewMatrix( backEnd.viewParms.world.viewMatrix );
	gl_forwardLightingShader_directionalSun->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_Bones
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_forwardLightingShader_directionalSun->SetUniform_Bones( tess.numBones, tess.bones );
	}

	// u_VertexInterpolation
	if ( tess.vboVertexAnimation )
	{
		gl_forwardLightingShader_directionalSun->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	gl_forwardLightingShader_directionalSun->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	GL_CheckErrors();

	// bind u_DiffuseMap
	GL_BindToTMU( 0, diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
	gl_forwardLightingShader_directionalSun->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );

	if ( normalMapping )
	{
		// bind u_NormalMap
		if ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 1, diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 1, tr.flatImage );
		}

		gl_forwardLightingShader_directionalSun->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		if ( r_forceSpecular->integer )
		{
			GL_BindToTMU( 2, diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		}
		else if ( diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_BindToTMU( 2, diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_BindToTMU( 2, tr.blackImage );
		}

		gl_forwardLightingShader_directionalSun->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );

		float minSpec = RB_EvalExpression( &diffuseStage->specularExponentMin, r_specularExponentMin->value );
		float maxSpec = RB_EvalExpression( &diffuseStage->specularExponentMax, r_specularExponentMax->value );

		gl_forwardLightingShader_directionalSun->SetUniform_SpecularExponent( minSpec, maxSpec );
	}

	// bind u_ShadowMap
	if ( shadowCompare )
	{
		GL_BindToTMU( 5, tr.sunShadowMapFBOImage[ 0 ] );
		GL_BindToTMU( 10, tr.sunShadowClipMapFBOImage[ 0 ] );

		if ( r_parallelShadowSplits->integer >= 1 )
		{
			GL_BindToTMU( 6, tr.sunShadowMapFBOImage[ 1 ] );
			GL_BindToTMU( 11, tr.sunShadowClipMapFBOImage[ 1 ] );
		}

		if ( r_parallelShadowSplits->integer >= 2 )
		{
			GL_BindToTMU( 7, tr.sunShadowMapFBOImage[ 2 ] ); ;
			GL_BindToTMU( 12, tr.sunShadowClipMapFBOImage[ 2 ] );
		}

		if ( r_parallelShadowSplits->integer >= 3 )
		{
			GL_BindToTMU( 8, tr.sunShadowMapFBOImage[ 3 ] );
			GL_BindToTMU( 13, tr.sunShadowClipMapFBOImage[ 3 ] );
		}

		if ( r_parallelShadowSplits->integer >= 4 )
		{
			GL_BindToTMU( 9, tr.sunShadowMapFBOImage[ 4 ] );
			GL_BindToTMU( 14, tr.sunShadowClipMapFBOImage[ 4 ] );
		}
	}

	gl_forwardLightingShader_directionalSun->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_reflection_CB( int stage )
{
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_reflection_CB ---\n" );

	GL_State( pStage->stateBits );

	bool normalMapping = r_normalMapping->integer && ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] != nullptr );

	// choose right shader program ----------------------------------
	gl_reflectionShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_reflectionShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_reflectionShader->SetNormalMapping( normalMapping );

	gl_reflectionShader->BindProgram( pStage->deformIndex );

	// end choose right shader program ------------------------------

	gl_reflectionShader->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space

	gl_reflectionShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_reflectionShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_Bones
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_reflectionShader->SetUniform_Bones( tess.numBones, tess.bones );
	}

	// u_VertexInterpolation
	if ( tess.vboVertexAnimation )
	{
		gl_reflectionShader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// bind u_ColorMap
	GL_SelectTexture( 0 );
	if ( backEnd.currentEntity && ( backEnd.currentEntity != &tr.worldEntity ) )
	{
		GL_BindNearestCubeMap( backEnd.currentEntity->e.origin );
	}
	else
	{
		GL_BindNearestCubeMap( backEnd.viewParms.orientation.origin );
	}

	// bind u_NormalMap
	if ( normalMapping )
	{
		GL_BindToTMU( 1, pStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		gl_reflectionShader->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );
	}

	gl_reflectionShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_skybox( int stage )
{
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_skybox ---\n" );

	GL_State( pStage->stateBits );

	gl_skyboxShader->BindProgram( pStage->deformIndex );

	gl_skyboxShader->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space

	gl_skyboxShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_skyboxShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// bind u_ColorMap
	GL_BindToTMU( 0, pStage->bundle[ TB_COLORMAP ].image[ 0 ] );

	gl_skyboxShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_screen( int stage )
{
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_screen ---\n" );

	GL_State( pStage->stateBits );

	gl_screenShader->BindProgram( pStage->deformIndex );

	{
		GL_VertexAttribsState( ATTR_POSITION );
		glVertexAttrib4fv( ATTR_INDEX_COLOR, tess.svars.color.ToArray() );
	}

	gl_screenShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// bind u_CurrentMap
	GL_SelectTexture( 0 );
	BindAnimatedImage( &pStage->bundle[ TB_COLORMAP ] );

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_portal( int stage )
{
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_portal ---\n" );

	GL_State( pStage->stateBits );

	// enable shader, set arrays
	gl_portalShader->BindProgram( pStage->deformIndex );

	{
		GL_VertexAttribsState( ATTR_POSITION );
		glVertexAttrib4fv( ATTR_INDEX_COLOR, tess.svars.color.ToArray() );
	}

	gl_portalShader->SetUniform_PortalRange( tess.surfaceShader->portalRange );

	gl_portalShader->SetUniform_ModelViewMatrix( glState.modelViewMatrix[ glState.stackIndex ] );
	gl_portalShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// bind u_CurrentMap
	GL_SelectTexture( 0 );
	BindAnimatedImage( &pStage->bundle[ TB_COLORMAP ] );

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_heatHaze( int stage )
{
	uint32_t      stateBits;
	float         deformMagnitude;
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_heatHaze ---\n" );

	// remove alpha test
	stateBits = pStage->stateBits;
	stateBits &= ~GLS_ATEST_BITS;
	stateBits &= ~GLS_DEPTHMASK_TRUE;

	GL_State( stateBits );

	// choose right shader program ----------------------------------
	gl_heatHazeShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_heatHazeShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );
	if( tess.surfaceShader->autoSpriteMode ) {
		gl_heatHazeShader->EnableVertexSprite();
		tess.vboVertexSprite = true;
	} else {
		gl_heatHazeShader->DisableVertexSprite();
		tess.vboVertexSprite = false;
	}

	gl_heatHazeShader->BindProgram( pStage->deformIndex );

	// end choose right shader program ------------------------------

	// set uniforms
	if ( tess.vboVertexSprite )
	{
		// calculate the environment texcoords in object space
		gl_heatHazeShader->SetUniform_ViewOrigin( backEnd.orientation.viewOrigin );
		gl_heatHazeShader->SetUniform_ViewUp( backEnd.orientation.axis[ 2 ] );
	}


	deformMagnitude = RB_EvalExpression( &pStage->deformMagnitudeExp, 1.0 );
	gl_heatHazeShader->SetUniform_DeformMagnitude( deformMagnitude );

	gl_heatHazeShader->SetUniform_ModelViewMatrixTranspose( glState.modelViewMatrix[ glState.stackIndex ] );
	gl_heatHazeShader->SetUniform_ProjectionMatrixTranspose( glState.projectionMatrix[ glState.stackIndex ] );
	gl_heatHazeShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_Bones
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_heatHazeShader->SetUniform_Bones( tess.numBones, tess.bones );
	}

	// u_VertexInterpolation
	if ( tess.vboVertexAnimation )
	{
		gl_heatHazeShader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// u_DeformGen
	if ( tess.surfaceShader->numDeforms )
	{
		gl_heatHazeShader->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );
	}

	// draw to background image
	R_BindFBO( tr.mainFBO[ 1 - backEnd.currentMainFBO ] );

	// bind u_NormalMap
	GL_BindToTMU( 0, pStage->bundle[ TB_COLORMAP ].image[ 0 ] );
	gl_heatHazeShader->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_COLORMAP ] );

	GL_BindToTMU( 1, tr.currentRenderImage[ backEnd.currentMainFBO ] );

	gl_heatHazeShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	// copy to foreground image
	R_BindFBO( tr.mainFBO[ backEnd.currentMainFBO ] );
	GL_BindToTMU( 1, tr.currentRenderImage[ 1 - backEnd.currentMainFBO ] );
	gl_heatHazeShader->SetUniform_DeformMagnitude( 0.0f );
	Tess_DrawElements();

	GL_CheckErrors();
}

#if !defined( GLSL_COMPILE_STARTUP_ONLY )
static void Render_liquid( int stage )
{
	vec3_t        viewOrigin;
	float         fogDensity;
	GLfloat       fogColor[ 3 ];
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_liquid ---\n" );

	// Tr3B: don't allow blend effects
	GL_State( pStage->stateBits & ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_DEPTHMASK_TRUE ) );

	// choose right shader program
	gl_liquidShader->SetParallaxMapping(r_parallaxMapping->integer && tess.surfaceShader->parallax);

	// enable shader, set arrays
	gl_liquidShader->BindProgram();
	gl_liquidShader->SetRequiredVertexPointers();

	// set uniforms
	VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );  // in world space

	fogDensity = RB_EvalExpression( &pStage->fogDensityExp, 0.001 );
	VectorCopy( tess.svars.color, fogColor );

	gl_liquidShader->SetUniform_ViewOrigin( viewOrigin );
	gl_liquidShader->SetUniform_RefractionIndex( RB_EvalExpression( &pStage->refractionIndexExp, 1.0 ) );
	gl_liquidShader->SetUniform_FresnelPower( RB_EvalExpression( &pStage->fresnelPowerExp, 2.0 ) );
	gl_liquidShader->SetUniform_FresnelScale( RB_EvalExpression( &pStage->fresnelScaleExp, 1.0 ) );
	gl_liquidShader->SetUniform_FresnelBias( RB_EvalExpression( &pStage->fresnelBiasExp, 0.05 ) );
	gl_liquidShader->SetUniform_NormalScale( RB_EvalExpression( &pStage->normalScaleExp, 0.05 ) );
	gl_liquidShader->SetUniform_FogDensity( fogDensity );
	gl_liquidShader->SetUniform_FogColor( fogColor );

	gl_liquidShader->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );
	gl_liquidShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_liquidShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	float specMin = RB_EvalExpression( &pStage->specularExponentMin, r_specularExponentMin->value );
	float specMax = RB_EvalExpression( &pStage->specularExponentMax, r_specularExponentMax->value );
	gl_liquidShader->SetUniform_SpecularExponent( specMin, specMAx );

	GL_BindToTMU( 0, tr.currentRenderImage[ backEnd.currentMainFBO ] );

	// bind u_PortalMap
	GL_BindToTMU( 1, tr.portalRenderImage );

	// depth texture
	GL_BindToTMU( 2, tr.depthRenderImage );

	// bind u_NormalMap
	GL_BindToTMU( 3, pStage->bundle[ TB_COLORMAP ].image[ 0 ] );
	gl_liquidShader->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_COLORMAP ] );

	Tess_DrawElements();

	GL_CheckErrors();
}
#else
static void Render_liquid(int){}
#endif

static void Render_fog()
{
	fog_t  *fog;
	float  eyeT;
	vec3_t local;
	vec4_t fogDistanceVector, fogDepthVector;

	// no fog pass in snooper
	if ( tess.surfaceShader->noFog || !r_wolfFog->integer )
	{
		return;
	}

	// ydnar: no world, no fogging
	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		return;
	}

	fog = tr.world->fogs + tess.fogNum;

	// Tr3B: use this only to render fog brushes
	if ( fog->originalBrushNumber < 0 && tess.surfaceShader->sort <= Util::ordinal(shaderSort_t::SS_OPAQUE) )
	{
		return;
	}

	if ( r_logFile->integer )
	{
		GLimp_LogComment( va( "--- Render_fog( fogNum = %i, originalBrushNumber = %i ) ---\n", tess.fogNum, fog->originalBrushNumber ) );
	}

	// all fogging distance is based on world Z units
	VectorSubtract( backEnd.orientation.origin, backEnd.viewParms.orientation.origin, local );
	fogDistanceVector[ 0 ] = -backEnd.orientation.modelViewMatrix[ 2 ];
	fogDistanceVector[ 1 ] = -backEnd.orientation.modelViewMatrix[ 6 ];
	fogDistanceVector[ 2 ] = -backEnd.orientation.modelViewMatrix[ 10 ];
	fogDistanceVector[ 3 ] = DotProduct( local, backEnd.viewParms.orientation.axis[ 0 ] );

	// scale the fog vectors based on the fog's thickness
	fogDistanceVector[ 0 ] *= fog->tcScale;
	fogDistanceVector[ 1 ] *= fog->tcScale;
	fogDistanceVector[ 2 ] *= fog->tcScale;
	fogDistanceVector[ 3 ] *= fog->tcScale;

	// rotate the gradient vector for this orientation
	if ( fog->hasSurface )
	{
		fogDepthVector[ 0 ] = fog->surface[ 0 ] * backEnd.orientation.axis[ 0 ][ 0 ] +
		                      fog->surface[ 1 ] * backEnd.orientation.axis[ 0 ][ 1 ] + fog->surface[ 2 ] * backEnd.orientation.axis[ 0 ][ 2 ];
		fogDepthVector[ 1 ] = fog->surface[ 0 ] * backEnd.orientation.axis[ 1 ][ 0 ] +
		                      fog->surface[ 1 ] * backEnd.orientation.axis[ 1 ][ 1 ] + fog->surface[ 2 ] * backEnd.orientation.axis[ 1 ][ 2 ];
		fogDepthVector[ 2 ] = fog->surface[ 0 ] * backEnd.orientation.axis[ 2 ][ 0 ] +
		                      fog->surface[ 1 ] * backEnd.orientation.axis[ 2 ][ 1 ] + fog->surface[ 2 ] * backEnd.orientation.axis[ 2 ][ 2 ];
		fogDepthVector[ 3 ] = -fog->surface[ 3 ] + DotProduct( backEnd.orientation.origin, fog->surface );

		eyeT = DotProduct( backEnd.orientation.viewOrigin, fogDepthVector ) + fogDepthVector[ 3 ];
	}
	else
	{
		Vector4Set( fogDepthVector, 0, 0, 0, 1 );
		eyeT = 1; // non-surface fog always has eye inside
	}

	// see if the viewpoint is outside
	// this is needed for clipping distance even for constant fog

	fogDistanceVector[ 3 ] += 1.0 / 512;

	if ( tess.surfaceShader->fogPass == fogPass_t::FP_EQUAL )
	{
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	}
	else
	{
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}

	gl_fogQuake3Shader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_fogQuake3Shader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_fogQuake3Shader->BindProgram( 0 );

	gl_fogQuake3Shader->SetUniform_FogDistanceVector( fogDistanceVector );
	gl_fogQuake3Shader->SetUniform_FogDepthVector( fogDepthVector );
	gl_fogQuake3Shader->SetUniform_FogEyeT( eyeT );

	// u_Color
	gl_fogQuake3Shader->SetUniform_Color( fog->color );

	gl_fogQuake3Shader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_fogQuake3Shader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_Bones
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_fogQuake3Shader->SetUniform_Bones( tess.numBones, tess.bones );
	}

	// u_VertexInterpolation
	if ( tess.vboVertexAnimation )
	{
		gl_fogQuake3Shader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	gl_fogQuake3Shader->SetUniform_Time( backEnd.refdef.floatTime - backEnd.currentEntity->e.shaderTime );

	// bind u_ColorMap
	GL_BindToTMU( 0, tr.fogImage );

	gl_fogQuake3Shader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

/*
===============
Tess_ComputeColor
===============
*/
void Tess_ComputeColor( shaderStage_t *pStage )
{
	float rgb;
	float red;
	float green;
	float blue;
	float alpha;

	GLimp_LogComment( "--- Tess_ComputeColor ---\n" );

	// rgbGen
	switch ( pStage->rgbGen )
	{
		case colorGen_t::CGEN_IDENTITY:
			{
				tess.svars.color = Color::White;
				break;
			}

		case colorGen_t::CGEN_VERTEX:
		case colorGen_t::CGEN_ONE_MINUS_VERTEX:
			{
				tess.svars.color = Color::Color();
				break;
			}

		default:
		case colorGen_t::CGEN_IDENTITY_LIGHTING:
			{
				tess.svars.color = Color::White * tr.identityLight;
				break;
			}

		case colorGen_t::CGEN_CONST:
			{
				tess.svars.color = pStage->constantColor;
				break;
			}

		case colorGen_t::CGEN_ENTITY:
			{
				if ( backEnd.currentLight )
				{
					tess.svars.color = Color::Adapt( backEnd.currentLight->l.color );
					tess.svars.color.Clamp();
				}
				else if ( backEnd.currentEntity )
				{
					tess.svars.color = backEnd.currentEntity->e.shaderRGBA;
					tess.svars.color.Clamp();
				}
				else
				{
					tess.svars.color = Color::White;
				}

				break;
			}

		case colorGen_t::CGEN_ONE_MINUS_ENTITY:
			{
				if ( backEnd.currentLight )
				{
					tess.svars.color.SetRed( 1.0 - Math::Clamp( backEnd.currentLight->l.color[ 0 ], 0.0f, 1.0f ) );
					tess.svars.color.SetGreen( 1.0 - Math::Clamp( backEnd.currentLight->l.color[ 1 ], 0.0f, 1.0f ) );
					tess.svars.color.SetBlue( 1.0 - Math::Clamp( backEnd.currentLight->l.color[ 2 ], 0.0f, 1.0f ) );
					tess.svars.color.SetAlpha( 0.0 ); // FIXME
				}
				else if ( backEnd.currentEntity )
				{
					tess.svars.color = backEnd.currentEntity->e.shaderRGBA;
					tess.svars.color.Clamp();
				}
				else
				{
					tess.svars.color = Color::Color();
				}

				break;
			}

		case colorGen_t::CGEN_WAVEFORM:
			{
				float      glow;
				waveForm_t *wf;

				wf = &pStage->rgbWave;

				if ( wf->func == genFunc_t::GF_NOISE )
				{
					glow = wf->base + R_NoiseGet4f( 0, 0, 0, ( backEnd.refdef.floatTime + wf->phase ) * wf->frequency ) * wf->amplitude;
				}
				else
				{
					glow = RB_EvalWaveForm( wf ) * tr.identityLight;
				}

				glow = Com_Clamp( 0, 1, glow );

				tess.svars.color = Color::White * glow;
				tess.svars.color.SetAlpha( 1.0 );
				break;
			}

		case colorGen_t::CGEN_CUSTOM_RGB:
			{
				rgb = Math::Clamp( RB_EvalExpression( &pStage->rgbExp, 1.0 ), 0.0f, 1.0f );

				tess.svars.color = Color::White * rgb;
				break;
			}

		case colorGen_t::CGEN_CUSTOM_RGBs:
			{
				if ( backEnd.currentLight )
				{
					red = Math::Clamp( RB_EvalExpression( &pStage->redExp, backEnd.currentLight->l.color[ 0 ] ), 0.0f, 1.0f );
					green = Math::Clamp( RB_EvalExpression( &pStage->greenExp, backEnd.currentLight->l.color[ 1 ] ), 0.0f, 1.0f );
					blue = Math::Clamp( RB_EvalExpression( &pStage->blueExp, backEnd.currentLight->l.color[ 2 ] ), 0.0f, 1.0f );
				}
				else if ( backEnd.currentEntity )
				{
					red =
					  Math::Clamp( RB_EvalExpression( &pStage->redExp, backEnd.currentEntity->e.shaderRGBA.Red() * ( 1.0 / 255.0 ) ), 0.0f, 1.0f );
					green =
					  Math::Clamp( RB_EvalExpression( &pStage->greenExp, backEnd.currentEntity->e.shaderRGBA.Green() * ( 1.0 / 255.0 ) ), 0.0f, 1.0f );
					blue =
					  Math::Clamp( RB_EvalExpression( &pStage->blueExp, backEnd.currentEntity->e.shaderRGBA.Blue() * ( 1.0 / 255.0 ) ), 0.0f, 1.0f );
				}
				else
				{
					red = Math::Clamp( RB_EvalExpression( &pStage->redExp, 1.0 ), 0.0f, 1.0f );
					green = Math::Clamp( RB_EvalExpression( &pStage->greenExp, 1.0 ), 0.0f, 1.0f );
					blue = Math::Clamp( RB_EvalExpression( &pStage->blueExp, 1.0 ), 0.0f, 1.0f );
				}

				tess.svars.color.SetRed( red );
				tess.svars.color.SetGreen( green );
				tess.svars.color.SetBlue( blue );
				break;
			}
	}

	// alphaGen
	switch ( pStage->alphaGen )
	{
		default:
		case alphaGen_t::AGEN_IDENTITY:
			{
				if ( pStage->rgbGen != colorGen_t::CGEN_IDENTITY )
				{
					if ( ( pStage->rgbGen == colorGen_t::CGEN_VERTEX && tr.identityLight != 1 ) || pStage->rgbGen != colorGen_t::CGEN_VERTEX )
					{
						tess.svars.color.SetAlpha( 1.0 );
					}
				}

				break;
			}

		case alphaGen_t::AGEN_VERTEX:
		case alphaGen_t::AGEN_ONE_MINUS_VERTEX:
			{
				tess.svars.color.SetAlpha( 0.0 );
				break;
			}

		case alphaGen_t::AGEN_CONST:
			{
				if ( pStage->rgbGen != colorGen_t::CGEN_CONST )
				{
					tess.svars.color.SetAlpha( pStage->constantColor.Alpha() * ( 1.0 / 255.0 ) );
				}

				break;
			}

		case alphaGen_t::AGEN_ENTITY:
			{
				if ( backEnd.currentLight )
				{
					tess.svars.color.SetAlpha( 1.0 ); // FIXME ?
				}
				else if ( backEnd.currentEntity )
				{
					tess.svars.color.SetAlpha( Math::Clamp( backEnd.currentEntity->e.shaderRGBA.Alpha() * ( 1.0 / 255.0 ), 0.0, 1.0 ) );
				}
				else
				{
					tess.svars.color.SetAlpha( 1.0 );
				}

				break;
			}

		case alphaGen_t::AGEN_ONE_MINUS_ENTITY:
			{
				if ( backEnd.currentLight )
				{
					tess.svars.color.SetAlpha( 0.0 ); // FIXME ?
				}
				else if ( backEnd.currentEntity )
				{
					tess.svars.color.SetAlpha( 1.0 - Math::Clamp( backEnd.currentEntity->e.shaderRGBA.Alpha() * ( 1.0 / 255.0 ), 0.0, 1.0 ) );
				}
				else
				{
					tess.svars.color.SetAlpha( 0.0 );
				}

				break;
			}

		case alphaGen_t::AGEN_WAVEFORM:
			{
				float      glow;
				waveForm_t *wf;

				wf = &pStage->alphaWave;

				glow = RB_EvalWaveFormClamped( wf );

				tess.svars.color.SetAlpha( glow );
				break;
			}

		case alphaGen_t::AGEN_CUSTOM:
			{
				alpha = Math::Clamp( RB_EvalExpression( &pStage->alphaExp, 1.0 ), 0.0f, 1.0f );

				tess.svars.color.SetAlpha( alpha );
				break;
			}
	}
}

/*
===============
Tess_ComputeTexMatrices
===============
*/
static void Tess_ComputeTexMatrices( shaderStage_t *pStage )
{
	int   i;
	vec_t *matrix;

	GLimp_LogComment( "--- Tess_ComputeTexMatrices ---\n" );

	for ( i = 0; i < MAX_TEXTURE_BUNDLES; i++ )
	{
		matrix = tess.svars.texMatrices[ i ];

		RB_CalcTexMatrix( &pStage->bundle[ i ], matrix );

		if ( pStage->tcGen_Lightmap && i == TB_COLORMAP )
		{
			MatrixMultiplyScale( matrix,
			                     tr.fatLightmapStep,
			                     tr.fatLightmapStep,
			                     tr.fatLightmapStep );
		}
	}
}

void Tess_StageIteratorDebug()
{
	// log this call
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va( "--- Tess_StageIteratorDebug( %i vertices, %i triangles ) ---\n", tess.numVertexes, tess.numIndexes / 3 ) );
	}

	GL_CheckErrors();

	if ( !glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo )
	{
		Tess_UpdateVBOs( );
		GL_VertexAttribsState( tess.attribsSet );
	}

	Tess_DrawElements();
}

void Tess_StageIteratorGeneric()
{
	int stage;

	// log this call
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va
		                  ( "--- Tess_StageIteratorGeneric( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
		                    tess.numVertexes, tess.numIndexes / 3 ) );
	}

	GL_CheckErrors();

	if ( !glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo )
	{
		Tess_UpdateVBOs( );
	}

	// set face culling appropriately
	if( backEnd.currentEntity->e.renderfx & RF_SWAPCULL )
		GL_Cull( ReverseCull( tess.surfaceShader->cullType ) );
	else
		GL_Cull( tess.surfaceShader->cullType );

	// set polygon offset if necessary
	if ( tess.surfaceShader->polygonOffset )
	{
		glEnable( GL_POLYGON_OFFSET_FILL );
		GL_PolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	// call shader function
	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = tess.surfaceStages[ stage ];

		if ( !pStage )
		{
			break;
		}

		if ( !RB_EvalExpression( &pStage->ifExp, 1.0 ) )
		{
			continue;
		}

		Tess_ComputeColor( pStage );
		Tess_ComputeTexMatrices( pStage );

		switch ( pStage->type )
		{
			case stageType_t::ST_COLORMAP:
				{
					Render_generic( stage );
					break;
				}

			case stageType_t::ST_LIGHTMAP:
				{
					Render_lightMapping( stage, true, false );
					break;
				}

			case stageType_t::ST_DIFFUSEMAP:
			case stageType_t::ST_COLLAPSE_lighting_DBSG:
			case stageType_t::ST_COLLAPSE_lighting_DBG:
			case stageType_t::ST_COLLAPSE_lighting_DB:
			case stageType_t::ST_COLLAPSE_lighting_DBS:
				{
					{
						if ( r_precomputedLighting->integer || r_vertexLighting->integer )
						{
							if ( !r_vertexLighting->integer && tess.lightmapNum >= 0 && tess.lightmapNum < tr.lightmaps.currentElements )
							{
								if ( tr.worldDeluxeMapping && r_normalMapping->integer )
								{
									Render_lightMapping( stage, false, true );
								}
								else
								{
									Render_lightMapping( stage, false, false );
								}
							}
							else if ( backEnd.currentEntity != &tr.worldEntity )
							{
								Render_vertexLighting_DBS_entity( stage );
							}
							else
							{
								Render_vertexLighting_DBS_world( stage );
							}
						}
						else
						{
							Render_depthFill( stage );
						}
					}
					break;
				}

			case stageType_t::ST_COLLAPSE_reflection_CB:
			case stageType_t::ST_REFLECTIONMAP:
				{
					if ( r_reflectionMapping->integer )
					{
						Render_reflection_CB( stage );
					}

					break;
				}

			case stageType_t::ST_REFRACTIONMAP:
				{
					break;
				}

			case stageType_t::ST_DISPERSIONMAP:
				{
					break;
				}

			case stageType_t::ST_SKYBOXMAP:
				{
					Render_skybox( stage );
					break;
				}

			case stageType_t::ST_SCREENMAP:
				{
					Render_screen( stage );
					break;
				}

			case stageType_t::ST_PORTALMAP:
				{
					Render_portal( stage );
					break;
				}

			case stageType_t::ST_HEATHAZEMAP:
				{
					if ( r_heatHaze->integer )
					{
						Render_heatHaze( stage );
					}

					break;
				}

			case stageType_t::ST_LIQUIDMAP:
				{
					Render_liquid( stage );
					break;
				}

			default:
				break;
		}

		if ( r_showLightMaps->integer && pStage->type == stageType_t::ST_LIGHTMAP )
		{
			break;
		}
	}

	if ( !r_noFog->integer && tess.fogNum >= 1 && tess.surfaceShader->fogPass != fogPass_t::FP_NONE )
	{
		Render_fog();
	}

	// reset polygon offset
	if ( tess.surfaceShader->polygonOffset )
	{
		glDisable( GL_POLYGON_OFFSET_FILL );
	}
}

void Tess_StageIteratorDepthFill()
{
	int stage;

	// log this call
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va
		                  ( "--- Tess_StageIteratorDepthFill( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
		                    tess.numVertexes, tess.numIndexes / 3 ) );
	}

	GL_CheckErrors();

	if ( !glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo )
	{
		Tess_UpdateVBOs( );
	}

	// set face culling appropriately
	if( backEnd.currentEntity->e.renderfx & RF_SWAPCULL )
		GL_Cull( ReverseCull( tess.surfaceShader->cullType ) );
	else
		GL_Cull( tess.surfaceShader->cullType );

	// set polygon offset if necessary
	if ( tess.surfaceShader->polygonOffset )
	{
		glEnable( GL_POLYGON_OFFSET_FILL );
		GL_PolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	// call shader function
	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = tess.surfaceStages[ stage ];

		if ( !pStage || !pStage->bundle[0].image[0] )
		{
			break;
		}

		if ( !RB_EvalExpression( &pStage->ifExp, 1.0 ) )
		{
			continue;
		}

		Tess_ComputeTexMatrices( pStage );

		switch ( pStage->type )
		{
			case stageType_t::ST_COLORMAP:
				{
					if ( tess.surfaceShader->sort <= Util::ordinal(shaderSort_t::SS_OPAQUE) )
					{
						Render_depthFill( stage );
					}

					break;
				}

			case stageType_t::ST_LIGHTMAP:
				{
					Render_depthFill( stage );
					break;
				}

			case stageType_t::ST_DIFFUSEMAP:
			case stageType_t::ST_COLLAPSE_lighting_DBSG:
			case stageType_t::ST_COLLAPSE_lighting_DBG:
			case stageType_t::ST_COLLAPSE_lighting_DB:
			case stageType_t::ST_COLLAPSE_lighting_DBS:
				{
					Render_depthFill( stage );
					break;
				}

			default:
				break;
		}
	}

	// reset polygon offset
	glDisable( GL_POLYGON_OFFSET_FILL );
}

void Tess_StageIteratorShadowFill()
{
	int stage;

	// log this call
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va
		                  ( "--- Tess_StageIteratorShadowFill( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
		                    tess.numVertexes, tess.numIndexes / 3 ) );
	}

	GL_CheckErrors();

	if ( !glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo )
	{
		Tess_UpdateVBOs( );
	}

	// set face culling appropriately
	if( backEnd.currentEntity->e.renderfx & RF_SWAPCULL )
		GL_Cull( ReverseCull( tess.surfaceShader->cullType ) );
	else
		GL_Cull( tess.surfaceShader->cullType );

	// set polygon offset if necessary
	if ( tess.surfaceShader->polygonOffset )
	{
		glEnable( GL_POLYGON_OFFSET_FILL );
		GL_PolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	// call shader function
	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = tess.surfaceStages[ stage ];

		if ( !pStage )
		{
			break;
		}

		if ( !RB_EvalExpression( &pStage->ifExp, 1.0 ) )
		{
			continue;
		}

		Tess_ComputeTexMatrices( pStage );

		switch ( pStage->type )
		{
			case stageType_t::ST_COLORMAP:
				{
					if ( tess.surfaceShader->sort <= Util::ordinal(shaderSort_t::SS_OPAQUE) )
					{
						Render_shadowFill( stage );
					}

					break;
				}

			case stageType_t::ST_LIGHTMAP:
			case stageType_t::ST_DIFFUSEMAP:
			case stageType_t::ST_COLLAPSE_lighting_DBSG:
			case stageType_t::ST_COLLAPSE_lighting_DBG:
			case stageType_t::ST_COLLAPSE_lighting_DB:
			case stageType_t::ST_COLLAPSE_lighting_DBS:
				{
					Render_shadowFill( stage );
					break;
				}

			default:
				break;
		}
	}

	// reset polygon offset
	glDisable( GL_POLYGON_OFFSET_FILL );
}

void Tess_StageIteratorLighting()
{
	int           i, j;
	trRefLight_t  *light;
	shaderStage_t *attenuationXYStage;
	shaderStage_t *attenuationZStage;

	// log this call
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va
		                  ( "--- Tess_StageIteratorLighting( %s, %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
		                    tess.lightShader->name, tess.numVertexes, tess.numIndexes / 3 ) );
	}

	GL_CheckErrors();

	light = backEnd.currentLight;

	if ( !glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo )
	{
		Tess_UpdateVBOs( );
	}

	// set OpenGL state for lighting
	if ( light->l.inverseShadows )
	{
		GL_State( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR );
	}
	else
	{
		if ( tess.surfaceShader->sort > Util::ordinal(shaderSort_t::SS_OPAQUE) )
		{
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		}
		else
		{
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
		}
	}

	// set face culling appropriately
	if( backEnd.currentEntity->e.renderfx & RF_SWAPCULL )
		GL_Cull( ReverseCull( tess.surfaceShader->cullType ) );
	else
		GL_Cull( tess.surfaceShader->cullType );

	// set polygon offset if necessary
	if ( tess.surfaceShader->polygonOffset )
	{
		glEnable( GL_POLYGON_OFFSET_FILL );
		GL_PolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	// call shader function
	attenuationZStage = tess.lightShader->stages[ 0 ];

	for ( i = 0; i < MAX_SHADER_STAGES; i++ )
	{
		shaderStage_t *diffuseStage = tess.surfaceStages[ i ];

		if ( !diffuseStage )
		{
			break;
		}

		if ( !RB_EvalExpression( &diffuseStage->ifExp, 1.0 ) )
		{
			continue;
		}

		Tess_ComputeTexMatrices( diffuseStage );

		for ( j = 1; j < MAX_SHADER_STAGES; j++ )
		{
			attenuationXYStage = tess.lightShader->stages[ j ];

			if ( !attenuationXYStage )
			{
				break;
			}

			if ( attenuationXYStage->type != stageType_t::ST_ATTENUATIONMAP_XY )
			{
				continue;
			}

			if ( !RB_EvalExpression( &attenuationXYStage->ifExp, 1.0 ) )
			{
				continue;
			}

			Tess_ComputeColor( attenuationXYStage );
			R_ComputeFinalAttenuation( attenuationXYStage, light );

			switch ( diffuseStage->type )
			{
				case stageType_t::ST_DIFFUSEMAP:
				case stageType_t::ST_COLLAPSE_lighting_DB:
				case stageType_t::ST_COLLAPSE_lighting_DBS:
					if ( light->l.rlType == refLightType_t::RL_OMNI )
					{
						Render_forwardLighting_DBS_omni( diffuseStage, attenuationXYStage, attenuationZStage, light );
					}
					else if ( light->l.rlType == refLightType_t::RL_PROJ )
					{
						{
							Render_forwardLighting_DBS_proj( diffuseStage, attenuationXYStage, attenuationZStage, light );
						}
					}
					else if ( light->l.rlType == refLightType_t::RL_DIRECTIONAL )
					{
						{
							Render_forwardLighting_DBS_directional( diffuseStage, light );
						}
					}

					break;

				default:
					break;
			}
		}
	}

	// reset polygon offset
	if ( tess.surfaceShader->polygonOffset )
	{
		glDisable( GL_POLYGON_OFFSET_FILL );
	}
}

/*
=================
Tess_End

Render tesselated data
=================
*/
void Tess_End()
{
	if ( ( tess.numIndexes == 0 || tess.numVertexes == 0 ) && tess.multiDrawPrimitives == 0 )
	{
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < tess.surfaceShader->sort )
	{
		return;
	}

	// update performance counter
	backEnd.pc.c_batches++;

	GL_CheckErrors();

	// call off to shader specific tess end function
	tess.stageIteratorFunc();

	if ( ( tess.stageIteratorFunc != Tess_StageIteratorShadowFill ) &&
	     ( tess.stageIteratorFunc != Tess_StageIteratorDebug ) )
	{
		// draw debugging stuff
		if ( r_showTris->integer || r_showBatches->integer || ( r_showLightBatches->integer && ( tess.stageIteratorFunc == Tess_StageIteratorLighting ) ) )
		{
			DrawTris();
		}
	}

	tess.vboVertexSkinning = false;
	tess.vboVertexAnimation = false;

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.attribsSet = 0;

	GLimp_LogComment( "--- Tess_End ---\n" );

	GL_CheckErrors();
}
