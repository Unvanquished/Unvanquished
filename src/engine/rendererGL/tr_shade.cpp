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

#if defined( USE_GLSL_OPTIMIZER )
#include "../../libs/glsl-optimizer/src/glsl/glsl_optimizer.h"
#endif
/*
=================================================================================
THIS ENTIRE FILE IS BACK END!

This file deals with applying shaders to surface data in the tess struct.
=================================================================================
*/

void GLSL_InitGPUShaders( void )
{
	// make sure the render thread is stopped
	R_SyncRenderThread();

	GL_CheckErrors();

#if defined( USE_GLSL_OPTIMIZER )
	s_glslOptimizer = glslopt_initialize( false );
#endif

	// single texture rendering
	gl_shaderManager.load( gl_genericShader );

	// simple vertex color shading for entities
	gl_shaderManager.load( gl_vertexLightingShader_DBS_entity );

	// simple vertex color shading for the world
	gl_shaderManager.load( gl_vertexLightingShader_DBS_world );

	// standard light mapping
	gl_shaderManager.load( gl_lightMappingShader );

	// geometric-buffer fill rendering with diffuse + bump + specular
	if ( DS_STANDARD_ENABLED() )
	{
		// G-Buffer construction
		gl_shaderManager.load( gl_geometricFillShader );

		// deferred omni-directional lighting post process effect
		gl_shaderManager.load( gl_deferredLightingShader_omniXYZ );

		gl_shaderManager.load( gl_deferredLightingShader_projXYZ );

		gl_shaderManager.load( gl_deferredLightingShader_directionalSun );
	}
	else
	{
		// omni-directional specular bump mapping ( Doom3 style )
		gl_shaderManager.load( gl_forwardLightingShader_omniXYZ );

		// projective lighting ( Doom3 style )
		gl_shaderManager.load( gl_forwardLightingShader_projXYZ );

		// directional sun lighting ( Doom3 style )
		gl_shaderManager.load( gl_forwardLightingShader_directionalSun );
	}

#if !defined( GLSL_COMPILE_STARTUP_ONLY )

	gl_shaderManager.load( gl_depthToColorShader );

#endif // #if !defined(GLSL_COMPILE_STARTUP_ONLY)

	// shadowmap distance compression
	gl_shaderManager.load( gl_shadowFillShader );

#if !defined( GLSL_COMPILE_STARTUP_ONLY )

#ifdef VOLUMETRIC_LIGHTING
	// volumetric lighting
	gl_shaderManager.load( gl_lightVolumeShader_omni );
#endif

	// UT3 style player shadowing
	gl_shaderManager.load( gl_deferredShadowingShader_proj );

#endif // #if !defined(GLSL_COMPILE_STARTUP_ONLY)

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

	// HDR -> LDR tone mapping
	gl_shaderManager.load( gl_toneMappingShader );

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

#ifdef EXPERIMENTAL
	// screen space ambien occlusion post process effect
	gl_shaderManager.load( gl_screenSpaceAmbientOcclusionShader );

	gl_shaderManager.load( gl_depthOfFieldShader );
#endif

#endif // #if !defined(GLSL_COMPILE_STARTUP_ONLY)

	gl_shaderManager.load( gl_motionblurShader );

	if ( !r_lazyShaders->integer )
	{
		gl_shaderManager.buildAll();
	}
}

void GLSL_ShutdownGPUShaders( void )
{
	R_SyncRenderThread();

	gl_shaderManager.freeAll();

	gl_genericShader = NULL;
	gl_vertexLightingShader_DBS_entity = NULL;
	gl_vertexLightingShader_DBS_world = NULL;
	gl_lightMappingShader = NULL;
	gl_geometricFillShader = NULL;
	gl_deferredLightingShader_omniXYZ = NULL;
	gl_deferredLightingShader_projXYZ = NULL;
	gl_deferredLightingShader_directionalSun = NULL;
	gl_forwardLightingShader_omniXYZ = NULL;
	gl_forwardLightingShader_projXYZ = NULL;
	gl_forwardLightingShader_directionalSun = NULL;
	gl_depthToColorShader = NULL;
	gl_shadowFillShader = NULL;
	gl_lightVolumeShader_omni = NULL;
	gl_deferredShadowingShader_proj = NULL;
	gl_reflectionShader = NULL;
	gl_skyboxShader = NULL;
	gl_fogQuake3Shader = NULL;
	gl_fogGlobalShader = NULL;
	gl_heatHazeShader = NULL;
	gl_screenShader = NULL;
	gl_portalShader = NULL;
	gl_toneMappingShader = NULL;
	gl_contrastShader = NULL;
	gl_cameraEffectsShader = NULL;
	gl_blurXShader = NULL;
	gl_blurYShader = NULL;
	gl_debugShadowMapShader = NULL;
	gl_liquidShader = NULL;
	gl_volumetricFogShader = NULL;
	gl_screenSpaceAmbientOcclusionShader = NULL;
	gl_depthOfFieldShader = NULL;
	gl_motionblurShader = NULL;

	GL_BindNullProgram();

#if defined( USE_GLSL_OPTIMIZER )
	glslopt_cleanup( s_glslOptimizer );
#endif
}

void GLSL_FinishGPUShaders( void )
{
	R_SyncRenderThread();

	gl_shaderManager.buildAll();
}
/*
static void MyMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const void* *indices, GLsizei primcount)
{
        int     i;

        for (i = 0; i < primcount; i++)
        {
                if (count[i] > 0)
                        glDrawElements(mode, count[i], type, indices[i]);
        }
}
*/

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
			glDrawElements( GL_TRIANGLES, tess.numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET( 0 ) );

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
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t tess;

/*
=================
BindLightMap
=================
*/
static void BindLightMap()
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
		lightmap = NULL;
	}

	if ( !tr.lightmaps.currentElements || !lightmap )
	{
		GL_Bind( tr.whiteImage );
		return;
	}

	GL_Bind( lightmap );
}

/*
=================
BindDeluxeMap
=================
*/
static void BindDeluxeMap()
{
	image_t *deluxemap;

	if ( tess.lightmapNum >= 0 && tess.lightmapNum < tr.deluxemaps.currentElements )
	{
		deluxemap = ( image_t * ) Com_GrowListElement( &tr.deluxemaps, tess.lightmapNum );
	}
	else
	{
		deluxemap = NULL;
	}

	if ( !tr.deluxemaps.currentElements || !deluxemap )
	{
		GL_Bind( tr.flatImage );
		return;
	}

	GL_Bind( deluxemap );
}

/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris()
{
	GLimp_LogComment( "--- DrawTris ---\n" );

	gl_genericShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_genericShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_genericShader->DisableDeformVertexes();
	gl_genericShader->DisableTCGenEnvironment();
	gl_genericShader->DisableTCGenLightmap();

	gl_genericShader->BindProgram();
	gl_genericShader->SetRequiredVertexPointers();

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );

	gl_genericShader->SetUniform_AlphaTest( GLS_ATEST_NONE );
	if ( r_showBatches->integer || r_showLightBatches->integer )
	{
		gl_genericShader->SetUniform_Color( g_color_table[ backEnd.pc.c_batches % 8 ] );
	}
	else if ( glState.currentVBO == tess.vbo )
	{
		gl_genericShader->SetUniform_Color( colorRed );
	}
	else if ( glState.currentVBO )
	{
		gl_genericShader->SetUniform_Color( colorBlue );
	}
	else
	{
		gl_genericShader->SetUniform_Color( colorWhite );
	}

	gl_genericShader->SetUniform_ColorModulate( CGEN_CONST, AGEN_CONST );

	gl_genericShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_genericShader->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// u_DeformGen
	if ( tess.surfaceShader->numDeforms )
	{
		gl_genericShader->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_genericShader->SetUniform_Time( backEnd.refdef.floatTime );
	}

	// bind u_ColorMap
	GL_SelectTexture( 0 );
	GL_Bind( tr.whiteImage );
	gl_genericShader->SetUniform_ColorTextureMatrix( tess.svars.texMatrices[ TB_COLORMAP ] );

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
void Tess_Begin( void ( *stageIteratorFunc )( void ),
                 void ( *stageIteratorFunc2 )( void ),
                 shader_t *surfaceShader, shader_t *lightShader,
                 qboolean skipTangentSpaces,
                 qboolean skipVBO,
                 int lightmapNum,
                 int fogNum )
{
	shader_t *state;

	tess.numIndexes = 0;
	tess.numVertexes = 0;

	tess.multiDrawPrimitives = 0;

	// materials are optional
	if ( surfaceShader != NULL )
	{
		state = ( surfaceShader->remappedShader ) ? surfaceShader->remappedShader : surfaceShader;

		tess.surfaceShader = state;
		tess.surfaceStages = state->stages;
		tess.numSurfaceStages = state->numStages;
	}
	else
	{
		state = NULL;

		tess.numSurfaceStages = 0;
		tess.surfaceShader = NULL;
		tess.surfaceStages = NULL;
	}

	bool isSky = ( state != NULL && state->isSky != qfalse );

	tess.lightShader = lightShader;

	tess.stageIteratorFunc = stageIteratorFunc;
	tess.stageIteratorFunc2 = stageIteratorFunc2;

	if ( !tess.stageIteratorFunc )
	{
		//tess.stageIteratorFunc = &Tess_StageIteratorGeneric;
		ri.Error( ERR_FATAL, "tess.stageIteratorFunc == NULL" );
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
	else if ( tess.stageIteratorFunc == Tess_StageIteratorGBuffer )
	{
		if ( isSky )
		{
			tess.stageIteratorFunc = &Tess_StageIteratorSky;
			tess.stageIteratorFunc2 = &Tess_StageIteratorGBuffer;
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
		GLimp_LogComment( va( "--- Tess_Begin( surfaceShader = %s, lightShader = %s, skipTangentSpaces = %i, lightmapNum = %i, fogNum = %i) ---\n", tess.surfaceShader->name, tess.lightShader ? tess.lightShader->name : NULL, tess.skipTangentSpaces, tess.lightmapNum, tess.fogNum ) );
	}
}

// *INDENT-ON*

static void Render_generic( int stage )
{
	shaderStage_t *pStage;
	colorGen_t    rgbGen;
	alphaGen_t    alphaGen;

	GLimp_LogComment( "--- Render_generic ---\n" );

	pStage = tess.surfaceStages[ stage ];

	GL_State( pStage->stateBits );

	// choose right shader program ----------------------------------
	gl_genericShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_genericShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_genericShader->SetDeformVertexes( tess.surfaceShader->numDeforms );
	gl_genericShader->SetTCGenEnvironment( pStage->tcGen_Environment );
	gl_genericShader->SetTCGenLightmap( pStage->tcGen_Lightmap );

	gl_genericShader->BindProgram();
	// end choose right shader program ------------------------------

	// set uniforms
	if ( pStage->tcGen_Environment )
	{
		// calculate the environment texcoords in object space
		gl_genericShader->SetUniform_ViewOrigin( backEnd.orientation.viewOrigin );
	}

	// u_AlphaTest
	gl_genericShader->SetUniform_AlphaTest( pStage->stateBits );

	// u_ColorGen
	switch ( pStage->rgbGen )
	{
		case CGEN_VERTEX:
		case CGEN_ONE_MINUS_VERTEX:
			rgbGen = pStage->rgbGen;
			break;

		default:
			rgbGen = CGEN_CONST;
			break;
	}

	// u_AlphaGen
	switch ( pStage->alphaGen )
	{
		case AGEN_VERTEX:
		case AGEN_ONE_MINUS_VERTEX:
			alphaGen = pStage->alphaGen;
			break;

		default:
			alphaGen = AGEN_CONST;
			break;
	}

	// u_ColorModulate
	gl_genericShader->SetUniform_ColorModulate( rgbGen, alphaGen );

	// u_Color
	gl_genericShader->SetUniform_Color( tess.svars.color );

	gl_genericShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_BoneMatrix
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_genericShader->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// u_VertexInterpolation
	if ( glState.vertexAttribsInterpolation > 0 )
	{
		gl_genericShader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// u_DeformGen
	if ( tess.surfaceShader->numDeforms )
	{
		gl_genericShader->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_genericShader->SetUniform_Time( backEnd.refdef.floatTime );
	}

	// bind u_ColorMap
	GL_SelectTexture( 0 );
	BindAnimatedImage( &pStage->bundle[ TB_COLORMAP ] );
	gl_genericShader->SetUniform_ColorTextureMatrix( tess.svars.texMatrices[ TB_COLORMAP ] );

	gl_genericShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_vertexLighting_DBS_entity( int stage )
{
	vec3_t        viewOrigin;
	vec3_t        ambientColor;
	vec3_t        lightDir;
	vec4_t        lightColor;
	uint32_t      stateBits;
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_vertexLighting_DBS_entity ---\n" );

	stateBits = pStage->stateBits;

	GL_State( stateBits );

	bool normalMapping = r_normalMapping->integer && ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] != NULL );

	// choose right shader program ----------------------------------
	gl_vertexLightingShader_DBS_entity->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_vertexLightingShader_DBS_entity->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_vertexLightingShader_DBS_entity->SetDeformVertexes( tess.surfaceShader->numDeforms );

	gl_vertexLightingShader_DBS_entity->SetNormalMapping( normalMapping );
	gl_vertexLightingShader_DBS_entity->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );

	gl_vertexLightingShader_DBS_entity->SetReflectiveSpecular( normalMapping && tr.cubeHashTable != NULL );

//	gl_vertexLightingShader_DBS_entity->SetMacro_TWOSIDED(tess.surfaceShader->cullType);

	gl_vertexLightingShader_DBS_entity->BindProgram();

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_vertexLightingShader_DBS_entity->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// set uniforms
	VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );  // in world space
	VectorCopy( backEnd.currentEntity->ambientLight, ambientColor );
	//ClampColor(ambientColor);
	VectorCopy( backEnd.currentEntity->directedLight, lightColor );
	//ClampColor(directedLight);

	// lightDir = L vector which means surface to light
	VectorCopy( backEnd.currentEntity->lightDir, lightDir );

	// u_AlphaTest
	gl_vertexLightingShader_DBS_entity->SetUniform_AlphaTest( pStage->stateBits );

	gl_vertexLightingShader_DBS_entity->SetUniform_AmbientColor( ambientColor );
	gl_vertexLightingShader_DBS_entity->SetUniform_ViewOrigin( viewOrigin );
	gl_vertexLightingShader_DBS_entity->SetUniform_LightDir( lightDir );
	gl_vertexLightingShader_DBS_entity->SetUniform_LightColor( lightColor );

	gl_vertexLightingShader_DBS_entity->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_vertexLightingShader_DBS_entity->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_VertexInterpolation
	if ( glState.vertexAttribsInterpolation > 0 )
	{
		gl_vertexLightingShader_DBS_entity->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// u_DeformGen
	if ( tess.surfaceShader->numDeforms )
	{
		gl_vertexLightingShader_DBS_entity->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_vertexLightingShader_DBS_entity->SetUniform_Time( backEnd.refdef.floatTime );
	}

	if ( r_parallaxMapping->integer && tess.surfaceShader->parallax )
	{
		float depthScale;

		depthScale = RB_EvalExpression( &pStage->depthScaleExp, r_parallaxDepthScale->value );
		gl_vertexLightingShader_DBS_entity->SetUniform_DepthScale( depthScale );
	}

	// bind u_DiffuseMap
	GL_SelectTexture( 0 );
	GL_Bind( pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
	gl_vertexLightingShader_DBS_entity->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );

	if ( normalMapping )
	{
		// bind u_NormalMap
		GL_SelectTexture( 1 );

		if ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_Bind( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.flatImage );
		}

		gl_vertexLightingShader_DBS_entity->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		GL_SelectTexture( 2 );

		if ( pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_Bind( pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.blackImage );
		}

		gl_vertexLightingShader_DBS_entity->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );

		//if(r_reflectionMapping->integer)
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

			if ( cubeProbeNearest == NULL && cubeProbeSecondNearest == NULL )
			{
				GLimp_LogComment( "cubeProbeNearest && cubeProbeSecondNearest == NULL\n" );

				// bind u_EnvironmentMap0
				GL_SelectTexture( 3 );
				GL_Bind( tr.whiteCubeImage );

				// bind u_EnvironmentMap1
				GL_SelectTexture( 4 );
				GL_Bind( tr.whiteCubeImage );
			}
			else if ( cubeProbeNearest == NULL )
			{
				GLimp_LogComment( "cubeProbeNearest == NULL\n" );

				// bind u_EnvironmentMap0
				GL_SelectTexture( 3 );
				GL_Bind( cubeProbeSecondNearest->cubemap );

				// u_EnvironmentInterpolation
				gl_vertexLightingShader_DBS_entity->SetUniform_EnvironmentInterpolation( 0.0 );
			}
			else if ( cubeProbeSecondNearest == NULL )
			{
				GLimp_LogComment( "cubeProbeSecondNearest == NULL\n" );

				// bind u_EnvironmentMap0
				GL_SelectTexture( 3 );
				GL_Bind( cubeProbeNearest->cubemap );

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
				GL_SelectTexture( 3 );
				GL_Bind( cubeProbeNearest->cubemap );

				// bind u_EnvironmentMap1
				GL_SelectTexture( 4 );
				GL_Bind( cubeProbeSecondNearest->cubemap );

				// u_EnvironmentInterpolation
				gl_vertexLightingShader_DBS_entity->SetUniform_EnvironmentInterpolation( interpolate );
			}
		}
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

	bool normalMapping = r_normalMapping->integer && ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] != NULL );

	// choose right shader program ----------------------------------
	gl_vertexLightingShader_DBS_world->SetDeformVertexes( tess.surfaceShader->numDeforms );

	gl_vertexLightingShader_DBS_world->SetNormalMapping( normalMapping );
	gl_vertexLightingShader_DBS_world->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );

//	gl_vertexLightingShader_DBS_world->SetMacro_TWOSIDED(tess.surfaceShader->cullType);

	gl_vertexLightingShader_DBS_world->BindProgram();

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// set uniforms
	VectorCopy( backEnd.orientation.viewOrigin, viewOrigin );

	GL_CheckErrors();

	// u_DeformGen
	if ( tess.surfaceShader->numDeforms )
	{
		gl_vertexLightingShader_DBS_world->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_vertexLightingShader_DBS_world->SetUniform_Time( backEnd.refdef.floatTime );
	}

	// u_ColorModulate
	switch ( pStage->rgbGen )
	{
		case CGEN_VERTEX:
		case CGEN_ONE_MINUS_VERTEX:
			colorGen = pStage->rgbGen;
			break;

		default:
			colorGen = CGEN_CONST;
			break;
	}

	switch ( pStage->alphaGen )
	{
		case AGEN_VERTEX:
			alphaGen = pStage->alphaGen;
			break;

		case AGEN_ONE_MINUS_VERTEX:
			alphaGen = pStage->alphaGen;

			/*
			alphaGen = AGEN_VERTEX;
			stateBits &= ~(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);
			stateBits |= (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
			*/
			break;

		default:
			alphaGen = AGEN_CONST;
			break;
	}

	GL_State( stateBits );

	gl_vertexLightingShader_DBS_world->SetUniform_ColorModulate( colorGen, alphaGen );

	// u_Color
	//if(r_showTerrainBlends->integer)
	//{
	//  gl_vertexLightingShader_DBS_world->SetUniform_Color(g_color_table[backEnd.pc.c_batches % 8]);
	//}
	//else
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

	// bind u_DiffuseMap
	GL_SelectTexture( 0 );
	GL_Bind( pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
	gl_vertexLightingShader_DBS_world->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );

	if ( normalMapping )
	{
		// bind u_NormalMap
		GL_SelectTexture( 1 );

		if ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_Bind( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.flatImage );
		}

		gl_vertexLightingShader_DBS_world->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		GL_SelectTexture( 2 );

		if ( pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_Bind( pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.blackImage );
		}

		gl_vertexLightingShader_DBS_world->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );
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

	GLimp_LogComment( "--- Render_lightMapping ---\n" );

	pStage = tess.surfaceStages[ stage ];

	stateBits = pStage->stateBits;

	switch ( pStage->rgbGen )
	{
		case CGEN_VERTEX:
		case CGEN_ONE_MINUS_VERTEX:
			rgbGen = pStage->rgbGen;
			break;

		default:
			rgbGen = CGEN_CONST;
			break;
	}

	switch ( pStage->alphaGen )
	{
		case AGEN_VERTEX:
		case AGEN_ONE_MINUS_VERTEX:
			alphaGen = pStage->alphaGen;
			break;

		default:
			alphaGen = AGEN_CONST;
			break;
	}

	if ( r_showLightMaps->integer )
	{
		stateBits &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_ATEST_BITS );
	}

	GL_State( stateBits );

	if ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] == NULL )
	{
		normalMapping = false;
	}

	// choose right shader program ----------------------------------

	gl_lightMappingShader->SetDeformVertexes( tess.surfaceShader->numDeforms );

	gl_lightMappingShader->SetNormalMapping( normalMapping );
	gl_lightMappingShader->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );

//	gl_lightMappingShader->SetMacro_TWOSIDED(tess.surfaceShader->cullType);

	gl_lightMappingShader->BindProgram();

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_DeformGen
	if ( tess.surfaceShader->numDeforms )
	{
		gl_lightMappingShader->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_lightMappingShader->SetUniform_Time( backEnd.refdef.floatTime );
	}

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
	GL_SelectTexture( 0 );

	if ( asColorMap )
	{
		GL_Bind( tr.whiteImage );
	}
	else
	{
		GL_Bind( pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		gl_lightMappingShader->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );
	}

	if ( normalMapping )
	{
		// bind u_NormalMap
		GL_SelectTexture( 1 );

		if ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_Bind( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.flatImage );
		}

		gl_lightMappingShader->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		GL_SelectTexture( 2 );

		if ( pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_Bind( pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.blackImage );
		}

		gl_lightMappingShader->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );

		// bind u_DeluxeMap
		GL_SelectTexture( 4 );
		BindDeluxeMap();
	}

	// bind u_LightMap
	GL_SelectTexture( 3 );
	BindLightMap();

	gl_lightMappingShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_geometricFill( int stage, bool cmap2black )
{
	shaderStage_t *pStage;
	uint32_t      stateBits;
//	vec4_t          ambientColor;

	GLimp_LogComment( "--- Render_geometricFill ---\n" );

	pStage = tess.surfaceStages[ stage ];

	// remove blend mode
	stateBits = pStage->stateBits;
	stateBits &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );

	GL_State( stateBits );

	bool normalMapping = r_normalMapping->integer && ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] != NULL );

	// choose right shader program ----------------------------------
	gl_geometricFillShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_geometricFillShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_geometricFillShader->SetDeformVertexes( tess.surfaceShader->numDeforms );

	gl_geometricFillShader->SetNormalMapping( normalMapping );
	gl_geometricFillShader->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );

	gl_geometricFillShader->SetReflectiveSpecular( false );  //normalMapping && tr.cubeHashTable != NULL);

//	gl_geometricFillShader->SetMacro_TWOSIDED(tess.surfaceShader->cullType);

	gl_geometricFillShader->BindProgram();

	// end choose right shader program ------------------------------

	/*
	{
	        if(r_precomputedLighting->integer)
	        {
	                VectorCopy(backEnd.currentEntity->ambientLight, ambientColor);
	                ClampColor(ambientColor);
	        }
	        else if(r_forceAmbient->integer)
	        {
	                ambientColor[0] = r_forceAmbient->value;
	                ambientColor[1] = r_forceAmbient->value;
	                ambientColor[2] = r_forceAmbient->value;
	        }
	        else
	        {
	                VectorClear(ambientColor);
	        }
	}
	*/

	gl_geometricFillShader->SetUniform_AlphaTest( pStage->stateBits );
	gl_geometricFillShader->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // world space
//	gl_geometricFillShader->SetUniform_AmbientColor(ambientColor);

	gl_geometricFillShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
//	gl_geometricFillShader->SetUniform_ModelViewMatrix(glState.modelViewMatrix[glState.stackIndex]);
	gl_geometricFillShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_BoneMatrix
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_geometricFillShader->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// u_VertexInterpolation
	if ( glState.vertexAttribsInterpolation > 0 )
	{
		gl_geometricFillShader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// u_DeformParms
	if ( tess.surfaceShader->numDeforms )
	{
		gl_geometricFillShader->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_geometricFillShader->SetUniform_Time( backEnd.refdef.floatTime );
	}

	// u_DepthScale
	if ( r_parallaxMapping->integer && tess.surfaceShader->parallax )
	{
		float depthScale;

		depthScale = RB_EvalExpression( &pStage->depthScaleExp, r_parallaxDepthScale->value );
		gl_geometricFillShader->SetUniform_DepthScale( depthScale );
	}

	//if(r_deferredShading->integer == DS_STANDARD)
	{
		// bind u_DiffuseMap
		GL_SelectTexture( 0 );

		if ( cmap2black )
		{
			GL_Bind( tr.blackImage );
		}
		else
		{
			GL_Bind( pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		}

		gl_geometricFillShader->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );
	}

	if ( normalMapping )
	{
		// bind u_NormalMap
		GL_SelectTexture( 1 );

		if ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_Bind( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.flatImage );
		}

		gl_geometricFillShader->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		if ( r_deferredShading->integer == DS_STANDARD )
		{
			// bind u_SpecularMap
			GL_SelectTexture( 2 );

			if ( r_forceSpecular->integer )
			{
				GL_Bind( pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
			}
			else if ( pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
			{
				GL_Bind( pStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
			}
			else
			{
				GL_Bind( tr.blackImage );
			}

			gl_geometricFillShader->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );
		}
	}

	gl_geometricFillShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_depthFill( int stage )
{
	shaderStage_t *pStage;
//	colorGen_t   rgbGen;
//	alphaGen_t   alphaGen;
	vec4_t        ambientColor;

	GLimp_LogComment( "--- Render_depthFill ---\n" );

	pStage = tess.surfaceStages[ stage ];

	uint32_t stateBits = pStage->stateBits;
	stateBits &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_ATEST_BITS );
	stateBits |= GLS_DEPTHMASK_TRUE;

	GL_State( pStage->stateBits );

	gl_genericShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_genericShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_genericShader->SetDeformVertexes( tess.surfaceShader->numDeforms );
	gl_genericShader->SetTCGenEnvironment( pStage->tcGen_Environment );

	gl_genericShader->BindProgram();

	// set uniforms
	if ( pStage->tcGen_Environment )
	{
		// calculate the environment texcoords in object space
		gl_genericShader->SetUniform_ViewOrigin( backEnd.orientation.viewOrigin );
	}

	// u_AlphaTest
	gl_genericShader->SetUniform_AlphaTest( pStage->stateBits );

	// u_ColorModulate
	gl_genericShader->SetUniform_ColorModulate( CGEN_CONST, AGEN_CONST );

	// u_Color
#if 1

	if ( r_precomputedLighting->integer )
	{
		VectorCopy( backEnd.currentEntity->ambientLight, ambientColor );
		ClampColor( ambientColor );
	}
	else if ( r_forceAmbient->integer )
	{
		ambientColor[ 0 ] = r_forceAmbient->value;
		ambientColor[ 1 ] = r_forceAmbient->value;
		ambientColor[ 2 ] = r_forceAmbient->value;
	}
	else
	{
		VectorClear( ambientColor );
	}

	ambientColor[ 3 ] = 1;
	gl_genericShader->SetUniform_Color( ambientColor );
#else
	gl_genericShader->SetUniform_Color( colorMdGrey );
#endif

	gl_genericShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_genericShader->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// u_DeformGen
	if ( tess.surfaceShader->numDeforms )
	{
		gl_genericShader->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_genericShader->SetUniform_Time( backEnd.refdef.floatTime );
	}

	// bind u_ColorMap
	GL_SelectTexture( 0 );

	if ( tess.surfaceShader->alphaTest )
	{
		GL_Bind( pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		gl_genericShader->SetUniform_ColorTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );
	}
	else
	{
		//GL_Bind(tr.defaultImage);
		GL_Bind( pStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
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

	gl_shadowFillShader->SetDeformVertexes( tess.surfaceShader->numDeforms );
	gl_shadowFillShader->SetMacro_LIGHT_DIRECTIONAL( backEnd.currentLight->l.rlType == RL_DIRECTIONAL );

	gl_shadowFillShader->BindProgram();

	gl_shadowFillShader->SetRequiredVertexPointers();

	if ( r_debugShadowMaps->integer )
	{
		vec4_t shadowMapColor;

		Vector4Copy( g_color_table[ backEnd.pc.c_batches % 8 ], shadowMapColor );

		gl_shadowFillShader->SetUniform_Color( shadowMapColor );
	}

	gl_shadowFillShader->SetUniform_AlphaTest( pStage->stateBits );

	if ( backEnd.currentLight->l.rlType != RL_DIRECTIONAL )
	{
		gl_shadowFillShader->SetUniform_LightOrigin( backEnd.currentLight->origin );
		gl_shadowFillShader->SetUniform_LightRadius( backEnd.currentLight->sphereRadius );
	}

	gl_shadowFillShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_shadowFillShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_BoneMatrix
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_shadowFillShader->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// u_VertexInterpolation
	if ( glState.vertexAttribsInterpolation > 0 )
	{
		gl_shadowFillShader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// u_DeformGen
	if ( tess.surfaceShader->numDeforms )
	{
		gl_shadowFillShader->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_shadowFillShader->SetUniform_Time( backEnd.refdef.floatTime );
	}

	// bind u_ColorMap
	GL_SelectTexture( 0 );

	if ( ( pStage->stateBits & GLS_ATEST_BITS ) != 0 )
	{
		GL_Bind( pStage->bundle[ TB_COLORMAP ].image[ 0 ] );
		gl_shadowFillShader->SetUniform_ColorTextureMatrix( tess.svars.texMatrices[ TB_COLORMAP ] );
	}
	else
	{
		GL_Bind( tr.whiteImage );
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
	vec4_t     lightColor;
	float      shadowTexelSize;
	colorGen_t colorGen;
	alphaGen_t alphaGen;

	GLimp_LogComment( "--- Render_forwardLighting_DBS_omni ---\n" );

	bool normalMapping = r_normalMapping->integer && ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] != NULL );

	bool shadowCompare = ( r_shadows->integer >= SHADOWING_ESM16 && !light->l.noShadows && light->shadowLOD >= 0 );

	// choose right shader program ----------------------------------
	gl_forwardLightingShader_omniXYZ->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_forwardLightingShader_omniXYZ->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_forwardLightingShader_omniXYZ->SetDeformVertexes( tess.surfaceShader->numDeforms );

	gl_forwardLightingShader_omniXYZ->SetNormalMapping( normalMapping );
	gl_forwardLightingShader_omniXYZ->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );

//	gl_forwardLightingShader_omniXYZ->SetMacro_TWOSIDED(tess.surfaceShader->cullType);

	gl_forwardLightingShader_omniXYZ->SetShadowing( shadowCompare );

	gl_forwardLightingShader_omniXYZ->BindProgram();

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate
	switch ( diffuseStage->rgbGen )
	{
		case CGEN_VERTEX:
		case CGEN_ONE_MINUS_VERTEX:
			colorGen = diffuseStage->rgbGen;
			break;

		default:
			colorGen = CGEN_CONST;
			break;
	}

	switch ( diffuseStage->alphaGen )
	{
		case AGEN_VERTEX:
		case AGEN_ONE_MINUS_VERTEX:
			alphaGen = diffuseStage->alphaGen;
			break;

		default:
			alphaGen = AGEN_CONST;
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
	VectorCopy( tess.svars.color, lightColor );

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
	gl_forwardLightingShader_omniXYZ->SetUniform_LightColor( lightColor );
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

	// u_BoneMatrix
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_forwardLightingShader_omniXYZ->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// u_VertexInterpolation
	if ( glState.vertexAttribsInterpolation > 0 )
	{
		gl_forwardLightingShader_omniXYZ->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	if ( tess.surfaceShader->numDeforms )
	{
		gl_forwardLightingShader_omniXYZ->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_forwardLightingShader_omniXYZ->SetUniform_Time( backEnd.refdef.floatTime );
	}

	GL_CheckErrors();

	// bind u_DiffuseMap
	GL_SelectTexture( 0 );
	GL_Bind( diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
	gl_forwardLightingShader_omniXYZ->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );

	if ( normalMapping )
	{
		// bind u_NormalMap
		GL_SelectTexture( 1 );

		if ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_Bind( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.flatImage );
		}

		gl_forwardLightingShader_omniXYZ->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		GL_SelectTexture( 2 );

		if ( r_forceSpecular->integer )
		{
			GL_Bind( diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		}
		else if ( diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_Bind( diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.blackImage );
		}

		gl_forwardLightingShader_omniXYZ->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );
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
		GL_SelectTexture( 5 );
		GL_Bind( tr.shadowCubeFBOImage[ light->shadowLOD ] );
	}

	// bind u_RandomMap
	GL_SelectTexture( 6 );
	GL_Bind( tr.randomNormalsImage );

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
	vec4_t     lightColor;
	float      shadowTexelSize;
	colorGen_t colorGen;
	alphaGen_t alphaGen;

	GLimp_LogComment( "--- Render_fowardLighting_DBS_proj ---\n" );

	bool normalMapping = r_normalMapping->integer && ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] != NULL );

	bool shadowCompare = ( r_shadows->integer >= SHADOWING_ESM16 && !light->l.noShadows && light->shadowLOD >= 0 );

	// choose right shader program ----------------------------------
	gl_forwardLightingShader_projXYZ->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_forwardLightingShader_projXYZ->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_forwardLightingShader_projXYZ->SetDeformVertexes( tess.surfaceShader->numDeforms );

	gl_forwardLightingShader_projXYZ->SetNormalMapping( normalMapping );
	gl_forwardLightingShader_projXYZ->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );

//	gl_forwardLightingShader_projXYZ->SetMacro_TWOSIDED(tess.surfaceShader->cullType);

	gl_forwardLightingShader_projXYZ->SetShadowing( shadowCompare );

	gl_forwardLightingShader_projXYZ->BindProgram();

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate
	switch ( diffuseStage->rgbGen )
	{
		case CGEN_VERTEX:
		case CGEN_ONE_MINUS_VERTEX:
			colorGen = diffuseStage->rgbGen;
			break;

		default:
			colorGen = CGEN_CONST;
			break;
	}

	switch ( diffuseStage->alphaGen )
	{
		case AGEN_VERTEX:
		case AGEN_ONE_MINUS_VERTEX:
			alphaGen = diffuseStage->alphaGen;
			break;

		default:
			alphaGen = AGEN_CONST;
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
	VectorCopy( tess.svars.color, lightColor );

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
	gl_forwardLightingShader_projXYZ->SetUniform_LightColor( lightColor );
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

	// u_BoneMatrix
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_forwardLightingShader_projXYZ->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// u_VertexInterpolation
	if ( glState.vertexAttribsInterpolation > 0 )
	{
		gl_forwardLightingShader_projXYZ->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	if ( tess.surfaceShader->numDeforms )
	{
		gl_forwardLightingShader_projXYZ->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_forwardLightingShader_projXYZ->SetUniform_Time( backEnd.refdef.floatTime );
	}

	GL_CheckErrors();

	// bind u_DiffuseMap
	GL_SelectTexture( 0 );
	GL_Bind( diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
	gl_forwardLightingShader_projXYZ->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );

	if ( normalMapping )
	{
		// bind u_NormalMap
		GL_SelectTexture( 1 );

		if ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_Bind( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.flatImage );
		}

		gl_forwardLightingShader_projXYZ->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		GL_SelectTexture( 2 );

		if ( r_forceSpecular->integer )
		{
			GL_Bind( diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		}
		else if ( diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_Bind( diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.blackImage );
		}

		gl_forwardLightingShader_projXYZ->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );
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
		GL_SelectTexture( 5 );
		GL_Bind( tr.shadowMapFBOImage[ light->shadowLOD ] );
	}

	// bind u_RandomMap
	GL_SelectTexture( 6 );
	GL_Bind( tr.randomNormalsImage );

	gl_forwardLightingShader_projXYZ->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_forwardLighting_DBS_directional( shaderStage_t *diffuseStage,
    shaderStage_t *attenuationXYStage,
    shaderStage_t *attenuationZStage, trRefLight_t *light )
{
#if 1
	vec3_t     viewOrigin;
	vec3_t     lightDirection;
	vec4_t     lightColor;
	float      shadowTexelSize;
	colorGen_t colorGen;
	alphaGen_t alphaGen;

	GLimp_LogComment( "--- Render_forwardLighting_DBS_directional ---\n" );

	bool normalMapping = r_normalMapping->integer && ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] != NULL );

	bool shadowCompare = ( r_shadows->integer >= SHADOWING_ESM16 && !light->l.noShadows && light->shadowLOD >= 0 );

	// choose right shader program ----------------------------------
	gl_forwardLightingShader_directionalSun->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_forwardLightingShader_directionalSun->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_forwardLightingShader_directionalSun->SetDeformVertexes( tess.surfaceShader->numDeforms );

	gl_forwardLightingShader_directionalSun->SetNormalMapping( normalMapping );
	gl_forwardLightingShader_directionalSun->SetParallaxMapping( normalMapping && r_parallaxMapping->integer && tess.surfaceShader->parallax );

//	gl_forwardLightingShader_directionalSun->SetMacro_TWOSIDED(tess.surfaceShader->cullType);

	gl_forwardLightingShader_directionalSun->SetShadowing( shadowCompare );

	gl_forwardLightingShader_directionalSun->BindProgram();

	// end choose right shader program ------------------------------

	// now we are ready to set the shader program uniforms

	// u_ColorModulate
	switch ( diffuseStage->rgbGen )
	{
		case CGEN_VERTEX:
		case CGEN_ONE_MINUS_VERTEX:
			colorGen = diffuseStage->rgbGen;
			break;

		default:
			colorGen = CGEN_CONST;
			break;
	}

	switch ( diffuseStage->alphaGen )
	{
		case AGEN_VERTEX:
		case AGEN_ONE_MINUS_VERTEX:
			alphaGen = diffuseStage->alphaGen;
			break;

		default:
			alphaGen = AGEN_CONST;
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

#if 1
	VectorCopy( tr.sunDirection, lightDirection );
#else
	VectorCopy( light->direction, lightDirection );
#endif

	VectorCopy( tess.svars.color, lightColor );

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
	gl_forwardLightingShader_directionalSun->SetUniform_LightColor( lightColor );
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

	// u_BoneMatrix
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_forwardLightingShader_directionalSun->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// u_VertexInterpolation
	if ( glState.vertexAttribsInterpolation > 0 )
	{
		gl_forwardLightingShader_directionalSun->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	if ( tess.surfaceShader->numDeforms )
	{
		gl_forwardLightingShader_directionalSun->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_forwardLightingShader_directionalSun->SetUniform_Time( backEnd.refdef.floatTime );
	}

	GL_CheckErrors();

	// bind u_DiffuseMap
	GL_SelectTexture( 0 );
	GL_Bind( diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
	gl_forwardLightingShader_directionalSun->SetUniform_DiffuseTextureMatrix( tess.svars.texMatrices[ TB_DIFFUSEMAP ] );

	if ( normalMapping )
	{
		// bind u_NormalMap
		GL_SelectTexture( 1 );

		if ( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] )
		{
			GL_Bind( diffuseStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.flatImage );
		}

		gl_forwardLightingShader_directionalSun->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );

		// bind u_SpecularMap
		GL_SelectTexture( 2 );

		if ( r_forceSpecular->integer )
		{
			GL_Bind( diffuseStage->bundle[ TB_DIFFUSEMAP ].image[ 0 ] );
		}
		else if ( diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] )
		{
			GL_Bind( diffuseStage->bundle[ TB_SPECULARMAP ].image[ 0 ] );
		}
		else
		{
			GL_Bind( tr.blackImage );
		}

		gl_forwardLightingShader_directionalSun->SetUniform_SpecularTextureMatrix( tess.svars.texMatrices[ TB_SPECULARMAP ] );
	}

	// bind u_ShadowMap
	if ( shadowCompare )
	{
		GL_SelectTexture( 5 );
		GL_Bind( tr.sunShadowMapFBOImage[ 0 ] );

		if ( r_parallelShadowSplits->integer >= 1 )
		{
			GL_SelectTexture( 6 );
			GL_Bind( tr.sunShadowMapFBOImage[ 1 ] );
		}

		if ( r_parallelShadowSplits->integer >= 2 )
		{
			GL_SelectTexture( 7 );
			GL_Bind( tr.sunShadowMapFBOImage[ 2 ] );
		}

		if ( r_parallelShadowSplits->integer >= 3 )
		{
			GL_SelectTexture( 8 );
			GL_Bind( tr.sunShadowMapFBOImage[ 3 ] );
		}

		if ( r_parallelShadowSplits->integer >= 4 )
		{
			GL_SelectTexture( 9 );
			GL_Bind( tr.sunShadowMapFBOImage[ 4 ] );
		}
	}

	gl_forwardLightingShader_directionalSun->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
#endif
}

static void Render_reflection_CB( int stage )
{
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_reflection_CB ---\n" );

	GL_State( pStage->stateBits );

	bool normalMapping = r_normalMapping->integer && ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] != NULL );

	// choose right shader program ----------------------------------
	gl_reflectionShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_reflectionShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_reflectionShader->SetDeformVertexes( tess.surfaceShader->numDeforms );

	gl_reflectionShader->SetNormalMapping( normalMapping );

//	gl_reflectionShader->SetMacro_TWOSIDED(tess.surfaceShader->cullType);

	gl_reflectionShader->BindProgram();

	// end choose right shader program ------------------------------

	gl_reflectionShader->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space

	gl_reflectionShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_reflectionShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_BoneMatrix
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_reflectionShader->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// u_VertexInterpolation
	if ( glState.vertexAttribsInterpolation > 0 )
	{
		gl_reflectionShader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// bind u_ColorMap
	GL_SelectTexture( 0 );
#if 1

	if ( backEnd.currentEntity && ( backEnd.currentEntity != &tr.worldEntity ) )
	{
		GL_BindNearestCubeMap( backEnd.currentEntity->e.origin );
	}
	else
	{
		GL_BindNearestCubeMap( backEnd.viewParms.orientation.origin );
	}

#else
	GL_Bind( pStage->bundle[ TB_COLORMAP ].image[ 0 ] );
#endif

	// bind u_NormalMap
	if ( normalMapping )
	{
		GL_SelectTexture( 1 );
		GL_Bind( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] );
		gl_reflectionShader->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_NORMALMAP ] );
	}

	gl_reflectionShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_dispersion_C( int stage )
{
#if 0 //!defined(GLSL_COMPILE_STARTUP_ONLY)
	vec3_t        viewOrigin;
	shaderStage_t *pStage = tess.surfaceStages[ stage ];
	float         eta;
	float         etaDelta;

	GLimp_LogComment( "--- Render_dispersion_C ---\n" );

	GL_State( pStage->stateBits );

	// enable shader, set arrays
	GL_BindProgram( &tr.dispersionShader_C );
	GL_VertexAttribsState( tr.dispersionShader_C.attribs );

	// set uniforms
	VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );  // in world space
	eta = RB_EvalExpression( &pStage->etaExp, ( float ) 1.1 );
	etaDelta = RB_EvalExpression( &pStage->etaDeltaExp, ( float ) - 0.02 );

	GLSL_SetUniform_ViewOrigin( &tr.dispersionShader_C, viewOrigin );
	glUniform3f( tr.dispersionShader_C.u_EtaRatio, eta, eta + etaDelta, eta + ( etaDelta * 2 ) );
	glUniform1f( tr.dispersionShader_C.u_FresnelPower, RB_EvalExpression( &pStage->fresnelPowerExp, 2.0f ) );
	glUniform1f( tr.dispersionShader_C.u_FresnelScale, RB_EvalExpression( &pStage->fresnelScaleExp, 2.0f ) );
	glUniform1f( tr.dispersionShader_C.u_FresnelBias, RB_EvalExpression( &pStage->fresnelBiasExp, 1.0f ) );

	GLSL_SetUniform_ModelMatrix( &tr.dispersionShader_C, backEnd.orientation.transformMatrix );
	GLSL_SetUniform_ModelViewProjectionMatrix( &tr.dispersionShader_C, glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	if ( glConfig2.vboVertexSkinningAvailable )
	{
		GLSL_SetUniform_VertexSkinning( &tr.dispersionShader_C, tess.vboVertexSkinning );

		if ( tess.vboVertexSkinning )
		{
			glUniformMatrix4fv( tr.dispersionShader_C.u_BoneMatrix, MAX_BONES, GL_FALSE, &tess.boneMatrices[ 0 ][ 0 ] );
		}
	}

	// bind u_ColorMap
	GL_SelectTexture( 0 );
	GL_Bind( pStage->bundle[ TB_COLORMAP ].image[ 0 ] );

	Tess_DrawElements();

	GL_CheckErrors();
#endif
}

static void Render_skybox( int stage )
{
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_skybox ---\n" );

	GL_State( pStage->stateBits );

	gl_skyboxShader->BindProgram();

	gl_skyboxShader->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space

	gl_skyboxShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_skyboxShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// bind u_ColorMap
	GL_SelectTexture( 0 );
	GL_Bind( pStage->bundle[ TB_COLORMAP ].image[ 0 ] );

	gl_skyboxShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_screen( int stage )
{
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_screen ---\n" );

	GL_State( pStage->stateBits );

	gl_screenShader->BindProgram();

	/*
	if(pStage->vertexColor || pStage->inverseVertexColor)
	{
	        GL_VertexAttribsState(tr.screenShader.attribs);
	}
	else
	*/
	{
		GL_VertexAttribsState( ATTR_POSITION );
		glVertexAttrib4fv( ATTR_INDEX_COLOR, tess.svars.color );
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
	gl_portalShader->BindProgram();

	/*
	if(pStage->vertexColor || pStage->inverseVertexColor)
	{
	        GL_VertexAttribsState(tr.portalShader.attribs);
	}
	else
	*/
	{
		GL_VertexAttribsState( ATTR_POSITION );
		glVertexAttrib4fv( ATTR_INDEX_COLOR, tess.svars.color );
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

	if ( r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable /*&& glConfig.hardwareType != GLHW_ATI && glConfig.hardwareType != GLHW_ATI_DX10*/ && glConfig.driverType != GLDRV_MESA )
	{
		FBO_t    *previousFBO;
		uint32_t stateBits;

		GLimp_LogComment( "--- HEATHAZE FIX BEGIN ---\n" );

		// capture current color buffer for u_CurrentMap

		/*
		GL_SelectTexture(0);
		GL_Bind(tr.currentRenderImage);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth,
		                                         tr.currentRenderImage->uploadHeight);

		*/

		previousFBO = glState.currentFBO;

		if ( DS_STANDARD_ENABLED() )
		{
			// copy deferredRenderFBO to occlusionRenderFBO
			glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, tr.geometricRenderFBO->frameBuffer );
			glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.occlusionRenderFBO->frameBuffer );
			glBlitFramebufferEXT( 0, 0, tr.deferredRenderFBO->width, tr.deferredRenderFBO->height,
			                      0, 0, tr.occlusionRenderFBO->width, tr.occlusionRenderFBO->height,
			                      GL_DEPTH_BUFFER_BIT,
			                      GL_NEAREST );
		}
		else if ( HDR_ENABLED() )
		{
			GL_CheckErrors();

			// copy deferredRenderFBO to occlusionRenderFBO
#if 0
			glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer );
			glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.occlusionRenderFBO->frameBuffer );
			glBlitFramebufferEXT( 0, 0, tr.deferredRenderFBO->width, tr.deferredRenderFBO->height,
			                      0, 0, tr.occlusionRenderFBO->width, tr.occlusionRenderFBO->height,
			                      GL_DEPTH_BUFFER_BIT,
			                      GL_NEAREST );
#else
			glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer );
			glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.occlusionRenderFBO->frameBuffer );
			glBlitFramebufferEXT( 0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                      0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                      GL_DEPTH_BUFFER_BIT,
			                      GL_NEAREST );
#endif

			GL_CheckErrors();
		}
		else
		{
			// copy depth of the main context to occlusionRenderFBO
			glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, 0 );
			glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.occlusionRenderFBO->frameBuffer );
			glBlitFramebufferEXT( 0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                      0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                      GL_DEPTH_BUFFER_BIT,
			                      GL_NEAREST );
		}

		R_BindFBO( tr.occlusionRenderFBO );
		R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.occlusionRenderFBOImage->texnum, 0 );

		// clear color buffer
		glClear( GL_COLOR_BUFFER_BIT );

		// remove blend mode
		stateBits = pStage->stateBits;
		stateBits &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_DEPTHMASK_TRUE );

		GL_State( stateBits );

		// choose right shader program ----------------------------------
		gl_genericShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
		gl_genericShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

		gl_genericShader->SetDeformVertexes( tess.surfaceShader->numDeforms );
		gl_genericShader->SetTCGenEnvironment( false );

		gl_genericShader->BindProgram();
		// end choose right shader program ------------------------------

		gl_genericShader->SetUniform_AlphaTest( GLS_ATEST_NONE );
		// u_ColorModulate
		gl_genericShader->SetUniform_ColorModulate( CGEN_CONST, AGEN_CONST );

		// u_Color
		gl_genericShader->SetUniform_Color( colorRed );

		gl_genericShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
		gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		// u_BoneMatrix
		if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
		{
			gl_genericShader->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
		}

		// u_VertexInterpolation
		if ( glState.vertexAttribsInterpolation > 0 )
		{
			gl_genericShader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
		}

		// u_DeformGen
		if ( tess.surfaceShader->numDeforms )
		{
			gl_genericShader->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
			gl_genericShader->SetUniform_Time( backEnd.refdef.floatTime );
		}

		// bind u_ColorMap
		GL_SelectTexture( 0 );
		GL_Bind( tr.whiteImage );
		//gl_genericShader->SetUniform_ColorTextureMatrix(tess.svars.texMatrices[TB_COLORMAP]);

		gl_genericShader->SetRequiredVertexPointers();

		Tess_DrawElements();

		R_BindFBO( previousFBO );

		GL_CheckErrors();

		GLimp_LogComment( "--- HEATHAZE FIX END ---\n" );
	}

	// remove alpha test
	stateBits = pStage->stateBits;
	stateBits &= ~GLS_ATEST_BITS;
	stateBits &= ~GLS_DEPTHMASK_TRUE;

	GL_State( stateBits );

	// choose right shader program ----------------------------------
	gl_heatHazeShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_heatHazeShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_heatHazeShader->SetDeformVertexes( tess.surfaceShader->numDeforms );

	gl_heatHazeShader->BindProgram();

	// end choose right shader program ------------------------------

	// set uniforms

	deformMagnitude = RB_EvalExpression( &pStage->deformMagnitudeExp, 1.0 );
	gl_heatHazeShader->SetUniform_DeformMagnitude( deformMagnitude );

	gl_heatHazeShader->SetUniform_ModelViewMatrixTranspose( glState.modelViewMatrix[ glState.stackIndex ] );
	gl_heatHazeShader->SetUniform_ProjectionMatrixTranspose( glState.projectionMatrix[ glState.stackIndex ] );
	gl_heatHazeShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_BoneMatrix
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_heatHazeShader->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// u_VertexInterpolation
	if ( glState.vertexAttribsInterpolation > 0 )
	{
		gl_heatHazeShader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// u_DeformGen
	if ( tess.surfaceShader->numDeforms )
	{
		gl_heatHazeShader->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_heatHazeShader->SetUniform_Time( backEnd.refdef.floatTime );
	}

	// bind u_NormalMap
	GL_SelectTexture( 0 );
	GL_Bind( pStage->bundle[ TB_COLORMAP ].image[ 0 ] );
	gl_heatHazeShader->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_COLORMAP ] );

	// bind u_CurrentMap
	GL_SelectTexture( 1 );

	if ( DS_STANDARD_ENABLED() )
	{
		GL_Bind( tr.deferredRenderFBOImage );
	}
	else if ( HDR_ENABLED() )
	{
		GL_Bind( tr.deferredRenderFBOImage );
	}
	else
	{
		GL_Bind( tr.currentRenderImage );
		glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth, tr.currentRenderImage->uploadHeight );
	}

	// bind u_ContrastMap
	if ( r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable /*&& glConfig.hardwareType != GLHW_ATI && glConfig.hardwareType != GLHW_ATI_DX10*/ && glConfig.driverType != GLDRV_MESA )
	{
		GL_SelectTexture( 2 );
		GL_Bind( tr.occlusionRenderFBOImage );
	}

	gl_heatHazeShader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

static void Render_liquid( int stage )
{
#if !defined( GLSL_COMPILE_STARTUP_ONLY )
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
	gl_liquidShader->SetUniform_FogColor( fogColor[ 0 ], fogColor[ 1 ], fogColor[ 2 ] );

	gl_liquidShader->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );
	gl_liquidShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_liquidShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// capture current color buffer for u_CurrentMap
	GL_SelectTexture( 0 );

	if ( DS_STANDARD_ENABLED() )
	{
		GL_Bind( tr.deferredRenderFBOImage );
	}
	else if ( HDR_ENABLED() )
	{
		GL_Bind( tr.deferredRenderFBOImage );
	}
	else
	{
		GL_Bind( tr.currentRenderImage );
		glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.currentRenderImage->uploadWidth, tr.currentRenderImage->uploadHeight );
	}

	// bind u_PortalMap
	GL_SelectTexture( 1 );
	GL_Bind( tr.portalRenderImage );

	// bind u_DepthMap
	GL_SelectTexture( 2 );

	if ( DS_STANDARD_ENABLED() )
	{
		GL_Bind( tr.depthRenderImage );
	}
	else if ( HDR_ENABLED() )
	{
		GL_Bind( tr.depthRenderImage );
	}
	else
	{
		// depth texture is not bound to a FBO
		GL_Bind( tr.depthRenderImage );
		glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight );
	}

	// bind u_NormalMap
	GL_SelectTexture( 3 );
	GL_Bind( pStage->bundle[ TB_COLORMAP ].image[ 0 ] );
	gl_liquidShader->SetUniform_NormalTextureMatrix( tess.svars.texMatrices[ TB_COLORMAP ] );

	Tess_DrawElements();

	GL_CheckErrors();
#endif
}

static void Render_fog()
{
	fog_t  *fog;
	float  eyeT;
//	qboolean        eyeOutside;
	vec3_t local;
	vec4_t fogDistanceVector, fogDepthVector;

	//ri.Printf(PRINT_ALL, "--- Render_fog ---\n");

#if defined( COMPAT_ET )

	// no fog pass in snooper
	if ( ( tr.refdef.rdflags & RDF_SNOOPERVIEW ) || tess.surfaceShader->noFog || !r_wolfFog->integer )
	{
		return;
	}

#endif

	// ydnar: no world, no fogging
	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		return;
	}

	fog = tr.world->fogs + tess.fogNum;

	// Tr3B: use this only to render fog brushes
#if 1

	if ( fog->originalBrushNumber < 0 && tess.surfaceShader->sort <= SS_OPAQUE )
	{
		return;
	}

#endif

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

#if 0

	if ( eyeT < 0 )
	{
		eyeOutside = qtrue;
	}
	else
	{
		eyeOutside = qfalse;
	}

#endif

	fogDistanceVector[ 3 ] += 1.0 / 512;

	if ( tess.surfaceShader->fogPass == FP_EQUAL )
	{
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	}
	else
	{
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}

	gl_fogQuake3Shader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_fogQuake3Shader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_fogQuake3Shader->SetDeformVertexes( tess.surfaceShader->numDeforms );

	gl_fogQuake3Shader->SetMacro_EYE_OUTSIDE( eyeT < 0 );

	gl_fogQuake3Shader->BindProgram();

	gl_fogQuake3Shader->SetUniform_FogDistanceVector( fogDistanceVector );
	gl_fogQuake3Shader->SetUniform_FogDepthVector( fogDepthVector );
	gl_fogQuake3Shader->SetUniform_FogEyeT( eyeT );

	// u_Color
	gl_fogQuake3Shader->SetUniform_Color( fog->color );

	gl_fogQuake3Shader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_fogQuake3Shader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_BoneMatrix
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_fogQuake3Shader->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
	}

	// u_VertexInterpolation
	if ( glState.vertexAttribsInterpolation > 0 )
	{
		gl_fogQuake3Shader->SetUniform_VertexInterpolation( glState.vertexAttribsInterpolation );
	}

	// u_DeformGen
	if ( tess.surfaceShader->numDeforms )
	{
		gl_fogQuake3Shader->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_fogQuake3Shader->SetUniform_Time( backEnd.refdef.floatTime );
	}

	// bind u_ColorMap
	GL_SelectTexture( 0 );
	GL_Bind( tr.fogImage );
	//gl_fogQuake3Shader->SetUniform_ColorTextureMatrix(tess.svars.texMatrices[TB_COLORMAP]);

	gl_fogQuake3Shader->SetRequiredVertexPointers();

	Tess_DrawElements();

	GL_CheckErrors();
}

// see Fog Polygon Volumes documentation by Nvidia for further information
static void Render_volumetricFog()
{
#if 0
	vec3_t viewOrigin;
	float  fogDensity;
	GLfloat fogColor[ 3 ];

	GLimp_LogComment( "--- Render_volumetricFog---\n" );

	if ( glConfig2.framebufferBlitAvailable )
	{
		FBO_t *previousFBO;

		previousFBO = glState.currentFBO;

		if ( r_deferredShading->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable &&
		     glConfig2.drawBuffersAvailable && glConfig2.maxDrawBuffers >= 4 )
		{
			// copy deferredRenderFBO to occlusionRenderFBO
			glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer );
			glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.occlusionRenderFBO->frameBuffer );
			glBlitFramebufferEXT( 0, 0, tr.deferredRenderFBO->width, tr.deferredRenderFBO->height,
			                      0, 0, tr.occlusionRenderFBO->width, tr.occlusionRenderFBO->height,
			                      GL_DEPTH_BUFFER_BIT,
			                      GL_NEAREST );
		}
		else if ( r_hdrRendering->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable )
		{
			// copy deferredRenderFBO to occlusionRenderFBO
			glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, tr.deferredRenderFBO->frameBuffer );
			glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.occlusionRenderFBO->frameBuffer );
			glBlitFramebufferEXT( 0, 0, tr.deferredRenderFBO->width, tr.deferredRenderFBO->height,
			                      0, 0, tr.occlusionRenderFBO->width, tr.occlusionRenderFBO->height,
			                      GL_DEPTH_BUFFER_BIT,
			                      GL_NEAREST );
		}
		else
		{
			// copy depth of the main context to occlusionRenderFBO
			glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, 0 );
			glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, tr.occlusionRenderFBO->frameBuffer );
			glBlitFramebufferEXT( 0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                      0, 0, glConfig.vidWidth, glConfig.vidHeight,
			                      GL_DEPTH_BUFFER_BIT,
			                      GL_NEAREST );
		}

		// setup shader
		gl_depthToColorShader->BindProgram();
		// Tr3B: might be cool for ghost player effects
		if ( glConfig2.vboVertexSkinningAvailable )
		{
			gl_depthToColorShader->SetVertexSkinning( tess.vboVertexSkinning );

			if ( tess.vboVertexSkinning )
			{
				gl_depthToColorShader->SetUniform_BoneMatrix( tess.numBoneMatrices, tess.boneMatrices );
			}
		}
		gl_depthToColorShader->SetRequiredVertexPointers();
		GL_State( 0 );  //GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

		gl_depthToColorShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		// render back faces
		R_BindFBO( tr.occlusionRenderFBO );
		R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.depthToColorBackFacesFBOImage->texnum, 0 );

		GL_ClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
		glClear( GL_COLOR_BUFFER_BIT );
		GL_Cull( CT_BACK_SIDED );
		Tess_DrawElements();

		// render front faces
		R_AttachFBOTexture2D( GL_TEXTURE_2D, tr.depthToColorFrontFacesFBOImage->texnum, 0 );

		glClear( GL_COLOR_BUFFER_BIT );
		GL_Cull( CT_FRONT_SIDED );
		Tess_DrawElements();

		R_BindFBO( previousFBO );

		// enable shader, set arrays
		gl_volumetricFogShader->BindProgram();
		gl_volumetricFogShader->SetRequiredVertexPointers();

		//GL_State(GLS_DEPTHTEST_DISABLE);  // | GLS_DEPTHMASK_TRUE);
		//GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR);
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_SRC_ALPHA );
		GL_Cull( CT_TWO_SIDED );

		glVertexAttrib4fv( ATTR_INDEX_COLOR, colorWhite );

		VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );  // in world space

		{
			fogDensity = tess.surfaceShader->fogParms.density;
			VectorCopy( tess.surfaceShader->fogParms.color, fogColor );
		}

		gl_volumetricFogShader->SetUniform_ModelViewMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );
		gl_volumetricFogShader->SetUniform_UnprojectMatrix( backEnd.viewParms.unprojectionMatrix );
		gl_volumetricFogShader->SetUniform_ViewOrigin( viewOrigin );
		gl_volumetricFogShader->SetUniform_FogDensity( fogDensity );
		gl_volumetricFogShader->SetUniform_FogColor( fogColor[ 0 ], fogColor[ 1 ], fogColor[ 2 ] );

		// bind u_DepthMap
		GL_SelectTexture( 0 );

		if ( r_deferredShading->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable &&
		     glConfig2.drawBuffersAvailable && glConfig2.maxDrawBuffers >= 4 )
		{
			GL_Bind( tr.depthRenderImage );
		}
		else if ( r_hdrRendering->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable )
		{
			GL_Bind( tr.depthRenderImage );
		}
		else
		{
			// depth texture is not bound to a FBO
			GL_Bind( tr.depthRenderImage );
			glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.depthRenderImage->uploadWidth, tr.depthRenderImage->uploadHeight );
		}

		// bind u_DepthMapBack
		GL_SelectTexture( 1 );
		GL_Bind( tr.depthToColorBackFacesFBOImage );

		// bind u_DepthMapFront
		GL_SelectTexture( 2 );
		GL_Bind( tr.depthToColorFrontFacesFBOImage );

		Tess_DrawElements();
	}

	GL_CheckErrors();
#endif
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
		case CGEN_IDENTITY:
			{
				tess.svars.color[ 0 ] = 1.0;
				tess.svars.color[ 1 ] = 1.0;
				tess.svars.color[ 2 ] = 1.0;
				tess.svars.color[ 3 ] = 1.0;
				break;
			}

		case CGEN_VERTEX:
		case CGEN_ONE_MINUS_VERTEX:
			{
				tess.svars.color[ 0 ] = 0.0;
				tess.svars.color[ 1 ] = 0.0;
				tess.svars.color[ 2 ] = 0.0;
				tess.svars.color[ 3 ] = 0.0;
				break;
			}

		default:
		case CGEN_IDENTITY_LIGHTING:
			{
				tess.svars.color[ 0 ] = tr.identityLight;
				tess.svars.color[ 1 ] = tr.identityLight;
				tess.svars.color[ 2 ] = tr.identityLight;
				tess.svars.color[ 3 ] = tr.identityLight;
				break;
			}

		case CGEN_CONST:
			{
				tess.svars.color[ 0 ] = pStage->constantColor[ 0 ] * ( 1.0 / 255.0 );
				tess.svars.color[ 1 ] = pStage->constantColor[ 1 ] * ( 1.0 / 255.0 );
				tess.svars.color[ 2 ] = pStage->constantColor[ 2 ] * ( 1.0 / 255.0 );
				tess.svars.color[ 3 ] = pStage->constantColor[ 3 ] * ( 1.0 / 255.0 );
				break;
			}

		case CGEN_ENTITY:
			{
				if ( backEnd.currentLight )
				{
					tess.svars.color[ 0 ] = Q_bound( 0.0, backEnd.currentLight->l.color[ 0 ], 1.0 );
					tess.svars.color[ 1 ] = Q_bound( 0.0, backEnd.currentLight->l.color[ 1 ], 1.0 );
					tess.svars.color[ 2 ] = Q_bound( 0.0, backEnd.currentLight->l.color[ 2 ], 1.0 );
					tess.svars.color[ 3 ] = 1.0;
				}
				else if ( backEnd.currentEntity )
				{
					tess.svars.color[ 0 ] = Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 0 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 1 ] = Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 1 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 2 ] = Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 2 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 3 ] = Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( 1.0 / 255.0 ), 1.0 );
				}
				else
				{
					tess.svars.color[ 0 ] = 1.0;
					tess.svars.color[ 1 ] = 1.0;
					tess.svars.color[ 2 ] = 1.0;
					tess.svars.color[ 3 ] = 1.0;
				}

				break;
			}

		case CGEN_ONE_MINUS_ENTITY:
			{
				if ( backEnd.currentLight )
				{
					tess.svars.color[ 0 ] = 1.0 - Q_bound( 0.0, backEnd.currentLight->l.color[ 0 ], 1.0 );
					tess.svars.color[ 1 ] = 1.0 - Q_bound( 0.0, backEnd.currentLight->l.color[ 1 ], 1.0 );
					tess.svars.color[ 2 ] = 1.0 - Q_bound( 0.0, backEnd.currentLight->l.color[ 2 ], 1.0 );
					tess.svars.color[ 3 ] = 0.0; // FIXME
				}
				else if ( backEnd.currentEntity )
				{
					tess.svars.color[ 0 ] = 1.0 - Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 0 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 1 ] = 1.0 - Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 1 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 2 ] = 1.0 - Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 2 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 3 ] = 1.0 - Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( 1.0 / 255.0 ), 1.0 );
				}
				else
				{
					tess.svars.color[ 0 ] = 0.0;
					tess.svars.color[ 1 ] = 0.0;
					tess.svars.color[ 2 ] = 0.0;
					tess.svars.color[ 3 ] = 0.0;
				}

				break;
			}

		case CGEN_WAVEFORM:
			{
				float      glow;
				waveForm_t *wf;

				wf = &pStage->rgbWave;

				if ( wf->func == GF_NOISE )
				{
					glow = wf->base + R_NoiseGet4f( 0, 0, 0, ( backEnd.refdef.floatTime + wf->phase ) * wf->frequency ) * wf->amplitude;
				}
				else
				{
					glow = RB_EvalWaveForm( wf ) * tr.identityLight;
				}

				if ( glow < 0 )
				{
					glow = 0;
				}
				else if ( glow > 1 )
				{
					glow = 1;
				}

				tess.svars.color[ 0 ] = glow;
				tess.svars.color[ 1 ] = glow;
				tess.svars.color[ 2 ] = glow;
				tess.svars.color[ 3 ] = 1.0;
				break;
			}

		case CGEN_CUSTOM_RGB:
			{
				rgb = Q_bound( 0.0, RB_EvalExpression( &pStage->rgbExp, 1.0 ), 1.0 );

				tess.svars.color[ 0 ] = rgb;
				tess.svars.color[ 1 ] = rgb;
				tess.svars.color[ 2 ] = rgb;
				break;
			}

		case CGEN_CUSTOM_RGBs:
			{
				if ( backEnd.currentLight )
				{
					red = Q_bound( 0.0, RB_EvalExpression( &pStage->redExp, backEnd.currentLight->l.color[ 0 ] ), 1.0 );
					green = Q_bound( 0.0, RB_EvalExpression( &pStage->greenExp, backEnd.currentLight->l.color[ 1 ] ), 1.0 );
					blue = Q_bound( 0.0, RB_EvalExpression( &pStage->blueExp, backEnd.currentLight->l.color[ 2 ] ), 1.0 );
				}
				else if ( backEnd.currentEntity )
				{
					red =
					  Q_bound( 0.0, RB_EvalExpression( &pStage->redExp, backEnd.currentEntity->e.shaderRGBA[ 0 ] * ( 1.0 / 255.0 ) ), 1.0 );
					green =
					  Q_bound( 0.0, RB_EvalExpression( &pStage->greenExp, backEnd.currentEntity->e.shaderRGBA[ 1 ] * ( 1.0 / 255.0 ) ),
					           1.0 );
					blue =
					  Q_bound( 0.0, RB_EvalExpression( &pStage->blueExp, backEnd.currentEntity->e.shaderRGBA[ 2 ] * ( 1.0 / 255.0 ) ),
					           1.0 );
				}
				else
				{
					red = Q_bound( 0.0, RB_EvalExpression( &pStage->redExp, 1.0 ), 1.0 );
					green = Q_bound( 0.0, RB_EvalExpression( &pStage->greenExp, 1.0 ), 1.0 );
					blue = Q_bound( 0.0, RB_EvalExpression( &pStage->blueExp, 1.0 ), 1.0 );
				}

				tess.svars.color[ 0 ] = red;
				tess.svars.color[ 1 ] = green;
				tess.svars.color[ 2 ] = blue;
				break;
			}
	}

	// alphaGen
	switch ( pStage->alphaGen )
	{
		default:
		case AGEN_IDENTITY:
			{
				if ( pStage->rgbGen != CGEN_IDENTITY )
				{
					if ( ( pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1 ) || pStage->rgbGen != CGEN_VERTEX )
					{
						tess.svars.color[ 3 ] = 1.0;
					}
				}

				break;
			}

		case AGEN_VERTEX:
		case AGEN_ONE_MINUS_VERTEX:
			{
				tess.svars.color[ 3 ] = 0.0;
				break;
			}

		case AGEN_CONST:
			{
				if ( pStage->rgbGen != CGEN_CONST )
				{
					tess.svars.color[ 3 ] = pStage->constantColor[ 3 ] * ( 1.0 / 255.0 );
				}

				break;
			}

		case AGEN_ENTITY:
			{
				if ( backEnd.currentLight )
				{
					tess.svars.color[ 3 ] = 1.0; // FIXME ?
				}
				else if ( backEnd.currentEntity )
				{
					tess.svars.color[ 3 ] = Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( 1.0 / 255.0 ), 1.0 );
				}
				else
				{
					tess.svars.color[ 3 ] = 1.0;
				}

				break;
			}

		case AGEN_ONE_MINUS_ENTITY:
			{
				if ( backEnd.currentLight )
				{
					tess.svars.color[ 3 ] = 0.0; // FIXME ?
				}
				else if ( backEnd.currentEntity )
				{
					tess.svars.color[ 3 ] = 1.0 - Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( 1.0 / 255.0 ), 1.0 );
				}
				else
				{
					tess.svars.color[ 3 ] = 0.0;
				}

				break;
			}

		case AGEN_WAVEFORM:
			{
				float      glow;
				waveForm_t *wf;

				wf = &pStage->alphaWave;

				glow = RB_EvalWaveFormClamped( wf );

				tess.svars.color[ 3 ] = glow;
				break;
			}

		case AGEN_CUSTOM:
			{
				alpha = Q_bound( 0.0, RB_EvalExpression( &pStage->alphaExp, 1.0 ), 1.0 );

				tess.svars.color[ 3 ] = alpha;
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

/*
==============
SetIteratorFog
        set the fog parameters for this pass
==============
*/
#if defined( COMPAT_ET )
static void SetIteratorFog()
{
	GLimp_LogComment( "--- SetIteratorFog() ---\n" );

	// changed for problem when you start the game with r_fastsky set to '1'
//  if(r_fastsky->integer || backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		RB_FogOff();
		return;
	}

	if ( backEnd.refdef.rdflags & RDF_DRAWINGSKY )
	{
		if ( tr.glfogsettings[ FOG_SKY ].registered )
		{
			RB_Fog( &tr.glfogsettings[ FOG_SKY ] );
		}
		else
		{
			RB_FogOff();
		}

		return;
	}

	if ( tr.world && tr.world->hasSkyboxPortal && ( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL ) )
	{
		if ( tr.glfogsettings[ FOG_PORTALVIEW ].registered )
		{
			RB_Fog( &tr.glfogsettings[ FOG_PORTALVIEW ] );
		}
		else
		{
			RB_FogOff();
		}
	}
	else
	{
		if ( tr.glfogNum > FOG_NONE )
		{
			RB_Fog( &tr.glfogsettings[ FOG_CURRENT ] );
		}
		else
		{
			RB_FogOff();
		}
	}
}

#endif // #if defined(COMPAT_ET)

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
		// Tr3B: FIXME analyze required vertex attribs by the current material
		Tess_UpdateVBOs( 0 );
	}

	Tess_DrawElements();
}

static INLINE GLenum RB_StencilOp( int op )
{
	switch( op & STO_MASK ) {
	case STO_KEEP:
		return GL_KEEP;
	case STO_ZERO:
		return GL_ZERO;
	case STO_REPLACE:
		return GL_REPLACE;
	case STO_INVERT:
		return GL_INVERT;
	case STO_INCR:
		return GL_INCR_WRAP;
	case STO_DECR:
		return GL_DECR_WRAP;
	default:
		return GL_KEEP;
	}
}
static void RB_SetStencil( GLenum side, stencil_t *stencil )
{
	GLenum  sfailOp, zfailOp, zpassOp;

	if( !side ) {
		glDisable( GL_STENCIL_TEST );
		return;
	}

	if( !stencil->flags ) {
		return;
	}

	glEnable( GL_STENCIL_TEST );
	switch( stencil->flags & STF_MASK ) {
	case STF_ALWAYS:
		glStencilFuncSeparate( side, GL_ALWAYS, stencil->ref, stencil->mask);
		break;
	case STF_NEVER:
		glStencilFuncSeparate( side, GL_NEVER, stencil->ref, stencil->mask);
		break;
	case STF_LESS:
		glStencilFuncSeparate( side, GL_LESS, stencil->ref, stencil->mask);
		break;
	case STF_LEQUAL:
		glStencilFuncSeparate( side, GL_LEQUAL, stencil->ref, stencil->mask);
		break;
	case STF_GREATER:
		glStencilFuncSeparate( side, GL_GREATER, stencil->ref, stencil->mask);
		break;
	case STF_GEQUAL:
		glStencilFuncSeparate( side, GL_GEQUAL, stencil->ref, stencil->mask);
		break;
	case STF_EQUAL:
		glStencilFuncSeparate( side, GL_EQUAL, stencil->ref, stencil->mask);
		break;
	case STF_NEQUAL:
		glStencilFuncSeparate( side, GL_NOTEQUAL, stencil->ref, stencil->mask);
		break;
	}

	sfailOp = RB_StencilOp( stencil->flags >> STS_SFAIL );
	zfailOp = RB_StencilOp( stencil->flags >> STS_ZFAIL );
	zpassOp = RB_StencilOp( stencil->flags >> STS_ZPASS );
	glStencilOpSeparate( side, sfailOp, zfailOp, zpassOp );

	glStencilMaskSeparate( side, (GLuint) stencil->writeMask );
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

	Tess_DeformGeometry();

	if ( !glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo )
	{
		// Tr3B: FIXME analyze required vertex attribs by the current material
		Tess_UpdateVBOs( 0 );
	}

	// set GL fog
	//SetIteratorFog();

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

#if 0

		// Ridah, per stage fogging (detail textures)
		if ( tess.surfaceShader->noFog && pStage->isFogged )
		{
			RB_FogOn();
		}
		else if ( tess.surfaceShader->noFog && !pStage->isFogged )
		{
			RB_FogOff();
		}
		else
		{
			// make sure it's on
			RB_FogOn();
		}

#endif

		Tess_ComputeColor( pStage );
		Tess_ComputeTexMatrices( pStage );

		if( pStage->frontStencil.flags || pStage->backStencil.flags ) {
			RB_SetStencil( GL_FRONT, &pStage->frontStencil );
			RB_SetStencil( GL_BACK, &pStage->backStencil );
		}

		switch ( pStage->type )
		{
			case ST_COLORMAP:
				{
					Render_generic( stage );
					break;
				}

#if defined( COMPAT_Q3A ) || defined( COMPAT_ET )

			case ST_LIGHTMAP:
				{
					Render_lightMapping( stage, true, false );
					break;
				}

#endif

			case ST_DIFFUSEMAP:
			case ST_COLLAPSE_lighting_DB:
			case ST_COLLAPSE_lighting_DBS:
				{
					//if(tess.surfaceShader->sort <= SS_OPAQUE)
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

			case ST_COLLAPSE_reflection_CB:
			case ST_REFLECTIONMAP:
				{
					if ( r_reflectionMapping->integer )
					{
						Render_reflection_CB( stage );
					}

					break;
				}

			case ST_REFRACTIONMAP:
				{
					//Render_refraction_C(stage);
					break;
				}

			case ST_DISPERSIONMAP:
				{
					Render_dispersion_C( stage );
					break;
				}

			case ST_SKYBOXMAP:
				{
					Render_skybox( stage );
					break;
				}

			case ST_SCREENMAP:
				{
					Render_screen( stage );
					break;
				}

			case ST_PORTALMAP:
				{
					Render_portal( stage );
					break;
				}

			case ST_HEATHAZEMAP:
				{
					Render_heatHaze( stage );
					break;
				}

			case ST_LIQUIDMAP:
				{
					Render_liquid( stage );
					break;
				}

			default:
				break;
		}

		if( pStage->frontStencil.flags || pStage->backStencil.flags ) {
			RB_SetStencil( 0, NULL );
		}

#if defined( COMPAT_Q3A ) || defined( COMPAT_ET )

		if ( r_showLightMaps->integer && pStage->type == ST_LIGHTMAP )
		{
			break;
		}

#endif
	}

	if ( !r_noFog->integer && tess.fogNum >= 1 && tess.surfaceShader->fogPass )
	{
		Render_fog();
	}

	// reset polygon offset
	if ( tess.surfaceShader->polygonOffset )
	{
		glDisable( GL_POLYGON_OFFSET_FILL );
	}
}

void Tess_StageIteratorGBuffer()
{
	int  stage;
	bool diffuseRendered = false;

	// log this call
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va
		                  ( "--- Tess_StageIteratorGBuffer( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
		                    tess.numVertexes, tess.numIndexes / 3 ) );
	}

	GL_CheckErrors();

	Tess_DeformGeometry();

	if ( !glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo )
	{
		// Tr3B: FIXME analyze required vertex attribs by the current material
		Tess_UpdateVBOs( 0 );
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
			case ST_COLORMAP:
				{
					R_BindFBO( tr.geometricRenderFBO );
					glDrawBuffers( 1, geometricRenderTargets );

					Render_generic( stage );

					if ( tess.surfaceShader->sort <= SS_OPAQUE &&
					     !( pStage->stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) &&
					     !diffuseRendered )
					{
						glDrawBuffers( 4, geometricRenderTargets );

						Render_geometricFill( stage, true );
					}

					break;
				}

			case ST_DIFFUSEMAP:
			case ST_COLLAPSE_lighting_DB:
			case ST_COLLAPSE_lighting_DBS:
				{
					if ( r_precomputedLighting->integer || r_vertexLighting->integer )
					{
						R_BindFBO( tr.geometricRenderFBO );
						glDrawBuffers( 4, geometricRenderTargets );

						if ( !r_vertexLighting->integer && tess.lightmapNum >= 0 && tess.lightmapNum < tr.lightmaps.currentElements )
						{
							if ( tr.worldDeluxeMapping && r_normalMapping->integer )
							{
								Render_lightMapping( stage, false, true );
								diffuseRendered = true;
							}
							else
							{
								Render_lightMapping( stage, false, false );
								diffuseRendered = true;
							}
						}
						else if ( backEnd.currentEntity != &tr.worldEntity )
						{
							Render_vertexLighting_DBS_entity( stage );
							diffuseRendered = true;
						}
						else
						{
							Render_vertexLighting_DBS_world( stage );
							diffuseRendered = true;
						}
					}
					else
					{
						R_BindFBO( tr.geometricRenderFBO );
						glDrawBuffers( 4, geometricRenderTargets );

						Render_geometricFill( stage, false );
						diffuseRendered = true;
					}

					break;
				}

			case ST_COLLAPSE_reflection_CB:
			case ST_REFLECTIONMAP:
				{
					R_BindFBO( tr.geometricRenderFBO );
					glDrawBuffers( 1, geometricRenderTargets );

					Render_reflection_CB( stage );
					break;
				}

			case ST_REFRACTIONMAP:
				{
					/*
					if(r_deferredShading->integer == DS_STANDARD)
					{

					        R_BindFBO(tr.deferredRenderFBO);
					        Render_refraction_C(stage);
					}
					*/
					break;
				}

			case ST_DISPERSIONMAP:
				{
					R_BindFBO( tr.geometricRenderFBO );
					glDrawBuffers( 1, geometricRenderTargets );

					Render_dispersion_C( stage );
					break;
				}

			case ST_SKYBOXMAP:
				{
					R_BindFBO( tr.geometricRenderFBO );
					glDrawBuffers( 4, geometricRenderTargets );

					Render_skybox( stage );
					break;
				}

			case ST_SCREENMAP:
				{
					R_BindFBO( tr.geometricRenderFBO );
					glDrawBuffers( 1, geometricRenderTargets );

					Render_screen( stage );
					break;
				}

			case ST_PORTALMAP:
				{
					R_BindFBO( tr.geometricRenderFBO );
					glDrawBuffers( 1, geometricRenderTargets );

					Render_portal( stage );
					break;
				}

			case ST_HEATHAZEMAP:
				{
					R_BindFBO( tr.geometricRenderFBO );
					glDrawBuffers( 1, geometricRenderTargets );

					Render_heatHaze( stage );
					break;
				}

			case ST_LIQUIDMAP:
				{
					R_BindFBO( tr.geometricRenderFBO );
					glDrawBuffers( 1, geometricRenderTargets );

					Render_liquid( stage );
					break;
				}

			default:
				break;
		}
	}

	if ( !r_noFog->integer && tess.fogNum >= 1 && tess.surfaceShader->fogPass )
	{
		Render_fog();
	}

	// reset polygon offset
	if ( tess.surfaceShader->polygonOffset )
	{
		glDisable( GL_POLYGON_OFFSET_FILL );
	}
}

void Tess_StageIteratorGBufferNormalsOnly()
{
	int stage;

	// log this call
	if ( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va
		                  ( "--- Tess_StageIteratorGBufferNormalsOnly( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
		                    tess.numVertexes, tess.numIndexes / 3 ) );
	}

	GL_CheckErrors();

	Tess_DeformGeometry();

	if ( !glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo )
	{
		// Tr3B: FIXME analyze required vertex attribs by the current material
		Tess_UpdateVBOs( 0 );
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

		//Tess_ComputeColor(pStage);
		Tess_ComputeTexMatrices( pStage );

		switch ( pStage->type )
		{
			case ST_COLORMAP:
				{
#if 1

					if ( tess.surfaceShader->sort <= SS_OPAQUE &&
					     !( pStage->stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) &&
					     ( pStage->stateBits & ( GLS_DEPTHMASK_TRUE ) )
					   )
					{
#if defined( OFFSCREEN_PREPASS_LIGHTING )
						R_BindFBO( tr.geometricRenderFBO );
#else
						R_BindNullFBO();
#endif
						Render_geometricFill( stage, true );
					}

#endif
					break;
				}

			case ST_DIFFUSEMAP:
			case ST_COLLAPSE_lighting_DB:
			case ST_COLLAPSE_lighting_DBS:
				{
#if defined( OFFSCREEN_PREPASS_LIGHTING )
					R_BindFBO( tr.geometricRenderFBO );
#else
					R_BindNullFBO();
#endif

					Render_geometricFill( stage, false );
					break;
				}

			default:
				break;
		}
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

	Tess_DeformGeometry();

	if ( !glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo )
	{
		Tess_UpdateVBOs( ATTR_POSITION | ATTR_TEXCOORD );
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
			case ST_COLORMAP:
				{
					if ( tess.surfaceShader->sort <= SS_OPAQUE )
					{
						Render_depthFill( stage );
					}

					break;
				}

#if defined( COMPAT_Q3A ) || defined( COMPAT_ET )

			case ST_LIGHTMAP:
				{
					Render_depthFill( stage );
					break;
				}

#endif

			case ST_DIFFUSEMAP:
			case ST_COLLAPSE_lighting_DB:
			case ST_COLLAPSE_lighting_DBS:
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

	Tess_DeformGeometry();

	if ( !glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo )
	{
		Tess_UpdateVBOs( ATTR_POSITION | ATTR_TEXCOORD );
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
			case ST_COLORMAP:
				{
					if ( tess.surfaceShader->sort <= SS_OPAQUE )
					{
						Render_shadowFill( stage );
					}

					break;
				}

#if defined( COMPAT_Q3A ) || defined( COMPAT_ET )

			case ST_LIGHTMAP:
#endif
			case ST_DIFFUSEMAP:
			case ST_COLLAPSE_lighting_DB:
			case ST_COLLAPSE_lighting_DBS:
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

	Tess_DeformGeometry();

	if ( !glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo )
	{
		// Tr3B: FIXME analyze required vertex attribs by the current material
		Tess_UpdateVBOs( 0 );
	}

	// set OpenGL state for lighting
	if ( light->l.inverseShadows )
	{
		GL_State( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR );
	}
	else
	{
		if ( tess.surfaceShader->sort > SS_OPAQUE )
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

			if ( attenuationXYStage->type != ST_ATTENUATIONMAP_XY )
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
				case ST_DIFFUSEMAP:
				case ST_COLLAPSE_lighting_DB:
				case ST_COLLAPSE_lighting_DBS:
					if ( light->l.rlType == RL_OMNI )
					{
						Render_forwardLighting_DBS_omni( diffuseStage, attenuationXYStage, attenuationZStage, light );
					}
					else if ( light->l.rlType == RL_PROJ )
					{
						if ( !light->l.inverseShadows )
						{
							Render_forwardLighting_DBS_proj( diffuseStage, attenuationXYStage, attenuationZStage, light );
						}
					}
					else if ( light->l.rlType == RL_DIRECTIONAL )
					{
						//if(!light->l.inverseShadows)
						{
							Render_forwardLighting_DBS_directional( diffuseStage, attenuationXYStage, attenuationZStage, light );
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

	if ( tess.indexes[ SHADER_MAX_INDEXES - 1 ] != 0 )
	{
		ri.Error( ERR_DROP, "Tess_End() - SHADER_MAX_INDEXES hit" );
	}

	if ( tess.xyz[ SHADER_MAX_VERTEXES - 1 ][ 0 ] != 0 )
	{
		ri.Error( ERR_DROP, "Tess_End() - SHADER_MAX_VERTEXES hit" );
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

	tess.vboVertexSkinning = qfalse;

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	GLimp_LogComment( "--- Tess_End ---\n" );

	GL_CheckErrors();
}
