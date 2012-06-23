/*
===========================================================================
Copyright (C) 2010-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// gl_shader.cpp -- GLSL shader handling

#include "gl_shader.h"

#if defined( USE_GLSL_OPTIMIZER )
#include "../../libs/glsl-optimizer/src/glsl/glsl_optimizer.h"

struct glslopt_ctx *s_glslOptimizer;

#endif
// *INDENT-OFF*

GLShader_generic                         *gl_genericShader = NULL;
GLShader_lightMapping                    *gl_lightMappingShader = NULL;
GLShader_vertexLighting_DBS_entity       *gl_vertexLightingShader_DBS_entity = NULL;
GLShader_vertexLighting_DBS_world        *gl_vertexLightingShader_DBS_world = NULL;
GLShader_forwardLighting_omniXYZ         *gl_forwardLightingShader_omniXYZ = NULL;
GLShader_forwardLighting_projXYZ         *gl_forwardLightingShader_projXYZ = NULL;
GLShader_forwardLighting_directionalSun *gl_forwardLightingShader_directionalSun = NULL;
GLShader_deferredLighting_omniXYZ        *gl_deferredLightingShader_omniXYZ = NULL;
GLShader_deferredLighting_projXYZ        *gl_deferredLightingShader_projXYZ = NULL;
GLShader_deferredLighting_directionalSun *gl_deferredLightingShader_directionalSun = NULL;
GLShader_geometricFill                   *gl_geometricFillShader = NULL;
GLShader_shadowFill                      *gl_shadowFillShader = NULL;
GLShader_reflection                      *gl_reflectionShader = NULL;
GLShader_skybox                          *gl_skyboxShader = NULL;
GLShader_fogQuake3                       *gl_fogQuake3Shader = NULL;
GLShader_fogGlobal                       *gl_fogGlobalShader = NULL;
GLShader_heatHaze                        *gl_heatHazeShader = NULL;
GLShader_screen                          *gl_screenShader = NULL;
GLShader_portal                          *gl_portalShader = NULL;
GLShader_toneMapping                     *gl_toneMappingShader = NULL;
GLShader_contrast                        *gl_contrastShader = NULL;
GLShader_cameraEffects                   *gl_cameraEffectsShader = NULL;
GLShader_blurX                           *gl_blurXShader = NULL;
GLShader_blurY                           *gl_blurYShader = NULL;
GLShader_debugShadowMap                  *gl_debugShadowMapShader = NULL;

bool GLCompileMacro_USE_VERTEX_SKINNING::HasConflictingMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	for ( size_t i = 0; i < macros.size(); i++ )
	{
		GLCompileMacro *macro = macros[ i ];

		//if(GLCompileMacro_USE_VERTEX_ANIMATION* m = dynamic_cast<GLCompileMacro_USE_VERTEX_ANIMATION*>(macro))
		if ( ( permutation & macro->GetBit() ) != 0 && macro->GetType() == USE_VERTEX_ANIMATION )
		{
			//ri.Printf(PRINT_ALL, "conflicting macro! canceling '%s' vs. '%s' \n", GetName(), macro->GetName());
			return true;
		}
	}

	return false;
}

bool GLCompileMacro_USE_VERTEX_SKINNING::MissesRequiredMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	return !glConfig2.vboVertexSkinningAvailable;
}

bool GLCompileMacro_USE_VERTEX_ANIMATION::HasConflictingMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const
{
#if 1

	for ( size_t i = 0; i < macros.size(); i++ )
	{
		GLCompileMacro *macro = macros[ i ];

		if ( ( permutation & macro->GetBit() ) != 0 && macro->GetType() == USE_VERTEX_SKINNING )
		{
			//ri.Printf(PRINT_ALL, "conflicting macro! canceling '%s' vs. '%s' \n", GetName(), macro->GetName());
			return true;
		}
	}

#endif
	return false;
}

uint32_t        GLCompileMacro_USE_VERTEX_ANIMATION::GetRequiredVertexAttributes() const
{
	uint32_t attribs = ATTR_NORMAL | ATTR_POSITION2 | ATTR_NORMAL2;

	if ( r_normalMapping->integer )
	{
		attribs |= ATTR_TANGENT2 | ATTR_BINORMAL2;
	}

	return attribs;
}

bool GLCompileMacro_USE_DEFORM_VERTEXES::HasConflictingMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	return ( glConfig.driverType != GLDRV_OPENGL3 || !r_vboDeformVertexes->integer );
}

bool GLCompileMacro_USE_PARALLAX_MAPPING::MissesRequiredMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	bool foundUSE_NORMAL_MAPPING = false;

	for ( size_t i = 0; i < macros.size(); i++ )
	{
		GLCompileMacro *macro = macros[ i ];

		if ( ( permutation & macro->GetBit() ) != 0 && macro->GetType() == USE_NORMAL_MAPPING )
		{
			foundUSE_NORMAL_MAPPING = true;
		}
	}

	if ( !foundUSE_NORMAL_MAPPING )
	{
		//ri.Printf(PRINT_ALL, "missing macro! canceling '%s' <= '%s' \n", GetName(), "USE_NORMAL_MAPPING");
		return true;
	}

	return false;
}

bool GLCompileMacro_USE_REFLECTIVE_SPECULAR::MissesRequiredMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	bool foundUSE_NORMAL_MAPPING = false;

	for ( size_t i = 0; i < macros.size(); i++ )
	{
		GLCompileMacro *macro = macros[ i ];

		if ( ( permutation & macro->GetBit() ) != 0 && macro->GetType() == USE_NORMAL_MAPPING )
		{
			foundUSE_NORMAL_MAPPING = true;
		}
	}

	if ( !foundUSE_NORMAL_MAPPING )
	{
		//ri.Printf(PRINT_ALL, "missing macro! canceling '%s' <= '%s' \n", GetName(), "USE_NORMAL_MAPPING");
		return true;
	}

	return false;
}

bool GLShader::GetCompileMacrosString( int permutation, std::string &compileMacrosOut ) const
{
	compileMacrosOut = "";

	for ( size_t j = 0; j < _compileMacros.size(); j++ )
	{
		GLCompileMacro *macro = _compileMacros[ j ];

		if ( permutation & macro->GetBit() )
		{
			if ( macro->HasConflictingMacros( permutation, _compileMacros ) )
			{
				//ri.Printf(PRINT_ALL, "conflicting macro! canceling '%s'\n", macro->GetName());
				return false;
			}

			if ( macro->MissesRequiredMacros( permutation, _compileMacros ) )
			{
				return false;
			}

			compileMacrosOut += macro->GetName();
			compileMacrosOut += " ";
		}
	}

	return true;
}

void GLShader::UpdateShaderProgramUniformLocations( shaderProgram_t *shaderProgram ) const
{
	size_t numUniforms = _uniforms.size();

	for ( size_t j = 0; j < numUniforms; j++ )
	{
		GLUniform *uniform = _uniforms[ j ];

		uniform->UpdateShaderProgramUniformLocation( shaderProgram );
	}
}

std::string     GLShader::BuildGPUShaderText( const char *mainShaderName,
    const char *libShaderNames,
    GLenum shaderType ) const
{
	char        filename[ MAX_QPATH ];
	GLchar      *mainBuffer = NULL;
	int         mainSize;
	char        *token;

	int         libsSize;
	char        *libsBuffer; // all libs concatenated

	char        **libs = ( char ** ) &libShaderNames;

	std::string shaderText;

	GL_CheckErrors();

	// load libs
	libsSize = 0;
	libsBuffer = NULL;

	while ( 1 )
	{
		int  libSize;
		char *libBuffer; // single extra lib file

		token = COM_ParseExt2( libs, qfalse );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( shaderType == GL_VERTEX_SHADER )
		{
			Com_sprintf( filename, sizeof( filename ), "glsl/%s_vp.glsl", token );
			ri.Printf( PRINT_ALL, "...loading vertex shader '%s'\n", filename );
		}
		else
		{
			Com_sprintf( filename, sizeof( filename ), "glsl/%s_fp.glsl", token );
			ri.Printf( PRINT_ALL, "...loading vertex shader '%s'\n", filename );
		}

		libSize = ri.FS_ReadFile( filename, ( void ** ) &libBuffer );

		if ( !libBuffer )
		{
			ri.Error( ERR_DROP, "Couldn't load %s", filename );
		}

		// append it to the libsBuffer
		libsBuffer = ( char * ) realloc( libsBuffer, libsSize + libSize );

		memset( libsBuffer + libsSize, 0, libSize );
		libsSize += libSize;

		Q_strcat( libsBuffer, libsSize, libBuffer );
		//Q_strncpyz(libsBuffer + libsSize, libBuffer, libSize -1);

		ri.FS_FreeFile( libBuffer );
	}

	// load main() program
	if ( shaderType == GL_VERTEX_SHADER )
	{
		Com_sprintf( filename, sizeof( filename ), "glsl/%s_vp.glsl", mainShaderName );
		ri.Printf( PRINT_ALL, "...loading vertex main() shader '%s'\n", filename );
	}
	else
	{
		Com_sprintf( filename, sizeof( filename ), "glsl/%s_fp.glsl", mainShaderName );
		ri.Printf( PRINT_ALL, "...loading fragment main() shader '%s'\n", filename );
	}

	mainSize = ri.FS_ReadFile( filename, ( void ** ) &mainBuffer );

	if ( !mainBuffer )
	{
		ri.Error( ERR_DROP, "Couldn't load %s", filename );
	}

	{
		static char bufferExtra[ 32000 ];
		int         sizeExtra;

		char        *bufferFinal = NULL;
		int         sizeFinal;

		float       fbufWidthScale, fbufHeightScale;
		float       npotWidthScale, npotHeightScale;

		Com_Memset( bufferExtra, 0, sizeof( bufferExtra ) );

#if defined( COMPAT_Q3A ) || defined( COMPAT_ET )
		Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef COMPAT_Q3A\n#define COMPAT_Q3A 1\n#endif\n" );
#endif

#if defined( COMPAT_ET )
		Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef COMPAT_ET\n#define COMPAT_ET 1\n#endif\n" );
#endif

		// HACK: add some macros to avoid extra uniforms and save speed and code maintenance
		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef r_SpecularExponent\n#define r_SpecularExponent %f\n#endif\n", r_specularExponent->value ) );

		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef r_SpecularExponent2\n#define r_SpecularExponent2 %f\n#endif\n", r_specularExponent2->value ) );

		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef r_SpecularScale\n#define r_SpecularScale %f\n#endif\n", r_specularScale->value ) );
		//Q_strcat(bufferExtra, sizeof(bufferExtra),
		//       va("#ifndef r_NormalScale\n#define r_NormalScale %f\n#endif\n", r_normalScale->value));

		Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef M_PI\n#define M_PI 3.14159265358979323846f\n#endif\n" );

		Q_strcat( bufferExtra, sizeof( bufferExtra ), va( "#ifndef MAX_SHADOWMAPS\n#define MAX_SHADOWMAPS %i\n#endif\n", MAX_SHADOWMAPS ) );

		Q_strcat( bufferExtra, sizeof( bufferExtra ), va( "#ifndef MAX_SHADER_DEFORM_PARMS\n#define MAX_SHADER_DEFORM_PARMS %i\n#endif\n", MAX_SHADER_DEFORM_PARMS ) );

		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef deform_t\n"
		              "#define deform_t\n"
		              "#define DEFORM_WAVE %i\n"
		              "#define DEFORM_BULGE %i\n"
		              "#define DEFORM_MOVE %i\n"
		              "#endif\n",
		              DEFORM_WAVE,
		              DEFORM_BULGE,
		              DEFORM_MOVE ) );

		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef genFunc_t\n"
		              "#define genFunc_t\n"
		              "#define GF_NONE %1.1f\n"
		              "#define GF_SIN %1.1f\n"
		              "#define GF_SQUARE %1.1f\n"
		              "#define GF_TRIANGLE %1.1f\n"
		              "#define GF_SAWTOOTH %1.1f\n"
		              "#define GF_INVERSE_SAWTOOTH %1.1f\n"
		              "#define GF_NOISE %1.1f\n"
		              "#endif\n",
		              ( float ) GF_NONE,
		              ( float ) GF_SIN,
		              ( float ) GF_SQUARE,
		              ( float ) GF_TRIANGLE,
		              ( float ) GF_SAWTOOTH,
		              ( float ) GF_INVERSE_SAWTOOTH,
		              ( float ) GF_NOISE ) );

		/*
		Q_strcat(bufferExtra, sizeof(bufferExtra),
		                                 va("#ifndef deformGen_t\n"
		                                        "#define deformGen_t\n"
		                                        "#define DGEN_WAVE_SIN %1.1f\n"
		                                        "#define DGEN_WAVE_SQUARE %1.1f\n"
		                                        "#define DGEN_WAVE_TRIANGLE %1.1f\n"
		                                        "#define DGEN_WAVE_SAWTOOTH %1.1f\n"
		                                        "#define DGEN_WAVE_INVERSE_SAWTOOTH %1.1f\n"
		                                        "#define DGEN_BULGE %i\n"
		                                        "#define DGEN_MOVE %i\n"
		                                        "#endif\n",
		                                        (float)DGEN_WAVE_SIN,
		                                        (float)DGEN_WAVE_SQUARE,
		                                        (float)DGEN_WAVE_TRIANGLE,
		                                        (float)DGEN_WAVE_SAWTOOTH,
		                                        (float)DGEN_WAVE_INVERSE_SAWTOOTH,
		                                        DGEN_BULGE,
		                                        DGEN_MOVE));
		                                */

		/*
		Q_strcat(bufferExtra, sizeof(bufferExtra),
		                                 va("#ifndef colorGen_t\n"
		                                        "#define colorGen_t\n"
		                                        "#define CGEN_VERTEX %i\n"
		                                        "#define CGEN_ONE_MINUS_VERTEX %i\n"
		                                        "#endif\n",
		                                        CGEN_VERTEX,
		                                        CGEN_ONE_MINUS_VERTEX));

		Q_strcat(bufferExtra, sizeof(bufferExtra),
		                                                 va("#ifndef alphaGen_t\n"
		                                                        "#define alphaGen_t\n"
		                                                        "#define AGEN_VERTEX %i\n"
		                                                        "#define AGEN_ONE_MINUS_VERTEX %i\n"
		                                                        "#endif\n",
		                                                        AGEN_VERTEX,
		                                                        AGEN_ONE_MINUS_VERTEX));
		                                                        */

		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef alphaTest_t\n"
		              "#define alphaTest_t\n"
		              "#define ATEST_GT_0 %i\n"
		              "#define ATEST_LT_128 %i\n"
		              "#define ATEST_GE_128 %i\n"
		              "#endif\n",
		              ATEST_GT_0,
		              ATEST_LT_128,
		              ATEST_GE_128 ) );

		fbufWidthScale = Q_recip( ( float ) glConfig.vidWidth );
		fbufHeightScale = Q_recip( ( float ) glConfig.vidHeight );
		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef r_FBufScale\n#define r_FBufScale vec2(%f, %f)\n#endif\n", fbufWidthScale, fbufHeightScale ) );

		if ( glConfig2.textureNPOTAvailable )
		{
			npotWidthScale = 1;
			npotHeightScale = 1;
		}
		else
		{
			npotWidthScale = ( float ) glConfig.vidWidth / ( float ) NearestPowerOfTwo( glConfig.vidWidth );
			npotHeightScale = ( float ) glConfig.vidHeight / ( float ) NearestPowerOfTwo( glConfig.vidHeight );
		}

		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef r_NPOTScale\n#define r_NPOTScale vec2(%f, %f)\n#endif\n", npotWidthScale, npotHeightScale ) );

		if ( glConfig.driverType == GLDRV_MESA )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef GLDRV_MESA\n#define GLDRV_MESA 1\n#endif\n" );
		}

		if ( glConfig.hardwareType == GLHW_ATI )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef GLHW_ATI\n#define GLHW_ATI 1\n#endif\n" );
		}
		else if ( glConfig.hardwareType == GLHW_ATI_DX10 )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef GLHW_ATI_DX10\n#define GLHW_ATI_DX10 1\n#endif\n" );
		}
		else if ( glConfig.hardwareType == GLHW_NV_DX10 )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef GLHW_NV_DX10\n#define GLHW_NV_DX10 1\n#endif\n" );
		}

		if ( r_shadows->integer >= SHADOWING_ESM16 && glConfig2.textureFloatAvailable && glConfig2.framebufferObjectAvailable )
		{
			if ( r_shadows->integer == SHADOWING_ESM16 || r_shadows->integer == SHADOWING_ESM32 )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef ESM\n#define ESM 1\n#endif\n" );
			}
			else if ( r_shadows->integer == SHADOWING_EVSM32 )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef EVSM\n#define EVSM 1\n#endif\n" );

				// The exponents for the EVSM techniques should be less than ln(FLT_MAX/FILTER_SIZE)/2 {ln(FLT_MAX/1)/2 ~44.3}
				//         42.9 is the maximum possible value for FILTER_SIZE=15
				//         42.0 is the truncated value that we pass into the sample
				Q_strcat( bufferExtra, sizeof( bufferExtra ),
				          va( "#ifndef r_EVSMExponents\n#define r_EVSMExponents vec2(%f, %f)\n#endif\n", 42.0f, 42.0f ) );

				if ( r_evsmPostProcess->integer )
				{
					Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_EVSMPostProcess\n#define r_EVSMPostProcess 1\n#endif\n" );
				}
			}
			else
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef VSM\n#define VSM 1\n#endif\n" );

				if ( glConfig.hardwareType == GLHW_ATI )
				{
					Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef VSM_CLAMP\n#define VSM_CLAMP 1\n#endif\n" );
				}
			}

			if ( ( glConfig.hardwareType == GLHW_NV_DX10 || glConfig.hardwareType == GLHW_ATI_DX10 ) && r_shadows->integer == SHADOWING_VSM32 )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef VSM_EPSILON\n#define VSM_EPSILON 0.000001\n#endif\n" );
			}
			else
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef VSM_EPSILON\n#define VSM_EPSILON 0.0001\n#endif\n" );
			}

			if ( r_lightBleedReduction->value )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ),
				          va( "#ifndef r_LightBleedReduction\n#define r_LightBleedReduction %f\n#endif\n",
				              r_lightBleedReduction->value ) );
			}

			if ( r_overDarkeningFactor->value )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ),
				          va( "#ifndef r_OverDarkeningFactor\n#define r_OverDarkeningFactor %f\n#endif\n",
				              r_overDarkeningFactor->value ) );
			}

			if ( r_shadowMapDepthScale->value )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ),
				          va( "#ifndef r_ShadowMapDepthScale\n#define r_ShadowMapDepthScale %f\n#endif\n",
				              r_shadowMapDepthScale->value ) );
			}

			if ( r_debugShadowMaps->integer )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ),
				          va( "#ifndef r_DebugShadowMaps\n#define r_DebugShadowMaps %i\n#endif\n", r_debugShadowMaps->integer ) );
			}

			/*
			if(r_softShadows->integer == 1)
			{
			        Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef PCF_2X2\n#define PCF_2X2 1\n#endif\n");
			}
			else if(r_softShadows->integer == 2)
			{
			        Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef PCF_3X3\n#define PCF_3X3 1\n#endif\n");
			}
			else if(r_softShadows->integer == 3)
			{
			        Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef PCF_4X4\n#define PCF_4X4 1\n#endif\n");
			}
			else if(r_softShadows->integer == 4)
			{
			        Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef PCF_5X5\n#define PCF_5X5 1\n#endif\n");
			}
			else if(r_softShadows->integer == 5)
			{
			        Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef PCF_6X6\n#define PCF_6X6 1\n#endif\n");
			}
			*/
			if ( r_softShadows->integer == 6 )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef PCSS\n#define PCSS 1\n#endif\n" );
			}
			else if ( r_softShadows->integer )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ),
				          va( "#ifndef r_PCFSamples\n#define r_PCFSamples %1.1f\n#endif\n", r_softShadows->value + 1.0f ) );
			}

			if ( r_parallelShadowSplits->integer )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ),
				          va( "#ifndef r_ParallelShadowSplits_%i\n#define r_ParallelShadowSplits_%i\n#endif\n", r_parallelShadowSplits->integer, r_parallelShadowSplits->integer ) );
			}

			if ( r_showParallelShadowSplits->integer )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_ShowParallelShadowSplits\n#define r_ShowParallelShadowSplits 1\n#endif\n" );
			}
		}

		if ( r_deferredShading->integer && glConfig2.maxColorAttachments >= 4 && glConfig2.textureFloatAvailable &&
		     glConfig2.drawBuffersAvailable && glConfig2.maxDrawBuffers >= 4 )
		{
			if ( r_deferredShading->integer == DS_STANDARD )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_DeferredShading\n#define r_DeferredShading 1\n#endif\n" );
			}
		}

		if ( r_hdrRendering->integer && glConfig2.framebufferObjectAvailable && glConfig2.textureFloatAvailable )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_HDRRendering\n#define r_HDRRendering 1\n#endif\n" );

			Q_strcat( bufferExtra, sizeof( bufferExtra ),
			          va( "#ifndef r_HDRContrastThreshold\n#define r_HDRContrastThreshold %f\n#endif\n",
			              r_hdrContrastThreshold->value ) );

			Q_strcat( bufferExtra, sizeof( bufferExtra ),
			          va( "#ifndef r_HDRContrastOffset\n#define r_HDRContrastOffset %f\n#endif\n",
			              r_hdrContrastOffset->value ) );

			Q_strcat( bufferExtra, sizeof( bufferExtra ),
			          va( "#ifndef r_HDRToneMappingOperator\n#define r_HDRToneMappingOperator_%i\n#endif\n",
			              r_hdrToneMappingOperator->integer ) );

			Q_strcat( bufferExtra, sizeof( bufferExtra ),
			          va( "#ifndef r_HDRGamma\n#define r_HDRGamma %f\n#endif\n",
			              r_hdrGamma->value ) );
		}

		if ( r_precomputedLighting->integer )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ),
			          "#ifndef r_precomputedLighting\n#define r_precomputedLighting 1\n#endif\n" );
		}

		if ( r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable && /*glConfig.hardwareType != GLHW_ATI && glConfig.hardwareType != GLHW_ATI_DX10 &&*/ glConfig.driverType != GLDRV_MESA )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_heatHazeFix\n#define r_heatHazeFix 1\n#endif\n" );
		}

		if ( r_showLightMaps->integer )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_showLightMaps\n#define r_showLightMaps 1\n#endif\n" );
		}

		if ( r_showDeluxeMaps->integer )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_showDeluxeMaps\n#define r_showDeluxeMaps 1\n#endif\n" );
		}

#ifdef EXPERIMENTAL

		if ( r_screenSpaceAmbientOcclusion->integer )
		{
			int             i;
			static vec3_t   jitter[ 32 ];
			static qboolean jitterInit = qfalse;

			if ( !jitterInit )
			{
				for ( i = 0; i < 32; i++ )
				{
					float *jit = &jitter[ i ][ 0 ];

					float rad = crandom() * 1024.0f; // FIXME radius;
					float a = crandom() * M_PI * 2;
					float b = crandom() * M_PI * 2;

					jit[ 0 ] = rad * sin( a ) * cos( b );
					jit[ 1 ] = rad * sin( a ) * sin( b );
					jit[ 2 ] = rad * cos( a );
				}

				jitterInit = qtrue;
			}

			// TODO
		}

#endif

		if ( glConfig2.vboVertexSkinningAvailable )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_VertexSkinning\n#define r_VertexSkinning 1\n#endif\n" );

			Q_strcat( bufferExtra, sizeof( bufferExtra ),
			          va( "#ifndef MAX_GLSL_BONES\n#define MAX_GLSL_BONES %i\n#endif\n", glConfig2.maxVertexSkinningBones ) );
		}
		else
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ),
			          va( "#ifndef MAX_GLSL_BONES\n#define MAX_GLSL_BONES %i\n#endif\n", 4 ) );
		}

		/*
		   if(glConfig.drawBuffersAvailable && glConfig.maxDrawBuffers >= 4)
		   {
		   //Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef GL_ARB_draw_buffers\n#define GL_ARB_draw_buffers 1\n#endif\n");
		   Q_strcat(bufferExtra, sizeof(bufferExtra), "#extension GL_ARB_draw_buffers : enable\n");
		   }
		 */

		if ( r_normalMapping->integer )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_NormalMapping\n#define r_NormalMapping 1\n#endif\n" );
		}

		if ( /* TODO: check for shader model 3 hardware  && */ r_normalMapping->integer && r_parallaxMapping->integer )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_ParallaxMapping\n#define r_ParallaxMapping 1\n#endif\n" );
		}

		if ( r_wrapAroundLighting->value )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ),
			          va( "#ifndef r_WrapAroundLighting\n#define r_WrapAroundLighting %f\n#endif\n",
			              r_wrapAroundLighting->value ) );
		}

		if ( r_halfLambertLighting->integer )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_HalfLambertLighting\n#define r_HalfLambertLighting 1\n#endif\n" );
		}

		if ( r_rimLighting->integer )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_RimLighting\n#define r_RimLighting 1\n#endif\n" );
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_RimColor\n#define r_RimColor vec4(0.26, 0.19, 0.16, 0.0)\n#endif\n" );
			Q_strcat( bufferExtra, sizeof( bufferExtra ), va( "#ifndef r_RimExponent\n#define r_RimExponent %f\n#endif\n",
			          r_rimExponent->value ) );
		}

		// OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
		// so we have to reset the line counting
		Q_strcat( bufferExtra, sizeof( bufferExtra ), "#line 0\n" );

		sizeExtra = strlen( bufferExtra );
		sizeFinal = sizeExtra + mainSize + libsSize;

		//ri.Printf(PRINT_ALL, "GLSL extra: %s\n", bufferExtra);

		bufferFinal = ( char * ) ri.Hunk_AllocateTempMemory( sizeFinal );

		strcpy( bufferFinal, bufferExtra );

		if ( libsSize > 0 )
		{
			Q_strcat( bufferFinal, sizeFinal, libsBuffer );
		}

		Q_strcat( bufferFinal, sizeFinal, mainBuffer );

#if 0
		{
			static char msgPart[ 1024 ];
			int         i;
			ri.Printf( PRINT_WARNING, "----------------------------------------------------------\n" );
			ri.Printf( PRINT_WARNING, "CONCATENATED shader '%s' ----------\n", filename );
			ri.Printf( PRINT_WARNING, " BEGIN ---------------------------------------------------\n" );

			for ( i = 0; i < sizeFinal; i += 1024 )
			{
				Q_strncpyz( msgPart, bufferFinal + i, sizeof( msgPart ) );
				ri.Printf( PRINT_ALL, "%s", msgPart );
			}

			ri.Printf( PRINT_WARNING, " END-- ---------------------------------------------------\n" );
		}
#endif

#if 0

		if ( optimize )
		{
			static char         msgPart[ 1024 ];
			int                 length = 0;
			int                 i;

			glslopt_shader_type glsloptShaderType;

			if ( shaderType == GL_FRAGMENT_SHADER )
			{
				glsloptShaderType = kGlslOptShaderFragment;
			}
			else
			{
				glsloptShaderType = kGlslOptShaderVertex;
			}

			glslopt_shader *shaderOptimized = glslopt_optimize( s_glslOptimizer,
			                                  glsloptShaderType, bufferFinal, 0 );

			if ( glslopt_get_status( shaderOptimized ) )
			{
				const char *newSource = glslopt_get_output( shaderOptimized );

				ri.Printf( PRINT_DEVELOPER, "----------------------------------------------------------\n" );
				ri.Printf( PRINT_DEVELOPER, "OPTIMIZED shader '%s' ----------\n", filename );
				ri.Printf( PRINT_DEVELOPER, " BEGIN ---------------------------------------------------\n" );

				length = strlen( newSource );

				for ( i = 0; i < length; i += 1024 )
				{
					Q_strncpyz( msgPart, newSource + i, sizeof( msgPart ) );
					ri.Printf( PRINT_WARNING, "%s\n", msgPart );
				}

				ri.Printf( PRINT_DEVELOPER, " END-- ---------------------------------------------------\n" );
				shaderText = std::string( newSource, length );
			}
			else
			{
				const char *errorLog = glslopt_get_log( shaderOptimized );

				//ri.Printf(PRINT_WARNING, "Couldn't optimize '%s'", filename);

				length = strlen( errorLog );

				for ( i = 0; i < length; i += 1024 )
				{
					Q_strncpyz( msgPart, errorLog + i, sizeof( msgPart ) );
					ri.Printf( PRINT_ALL, "%s\n", msgPart );
				}

				ri.Printf( PRINT_ALL, "^1Couldn't optimize %s\n", filename );
				shaderText = std::string( bufferFinal, sizeFinal );
			}

			glslopt_shader_delete( shaderOptimized );
		}
		else
		{
			shaderText = std::string( bufferFinal, sizeFinal );
		}

#else
		shaderText = std::string( bufferFinal, sizeFinal );
#endif

		ri.Hunk_FreeTempMemory( bufferFinal );
	}

	ri.FS_FreeFile( mainBuffer );
	free( libsBuffer );

	return shaderText;
}

void GLShader::SaveShaderProgram( GLuint program, const char *pname, int i ) const
{
#ifdef GLEW_ARB_get_program_binary
	GLint   binaryLength;
	GLvoid  *binary;

	// Don't even try if the necessary functions aren't available
	if( !GLEW_ARB_get_program_binary )
	{
		return;
	}

	glGetProgramiv( program, GL_PROGRAM_BINARY_LENGTH, &binaryLength );

	// Allocate enough for the binary and the binaryFormat
	binary = (GLvoid*)ri.Hunk_AllocateTempMemory( binaryLength + sizeof( GLenum ) );
	glGetProgramBinary( program, binaryLength, NULL, (GLenum*)binary, (char*)binary + sizeof( GLenum ) );

	ri.FS_WriteFile( va( "glsl/%s_%d.bin", pname, i ), binary, binaryLength + sizeof( GLenum ) );

	ri.Hunk_FreeTempMemory( binary );
#endif
}

bool GLShader::LoadShaderProgram( GLuint program, const char *pname, int i ) const
{
#ifdef GLEW_ARB_get_program_binary
	GLint  binaryLength, success;
	GLvoid *binary;
	GLenum binaryFormat;

	// Don't even try if the necessary functions aren't available
	if( !GLEW_ARB_get_program_binary )
	{
		return false;
	}

	binaryLength = ri.FS_ReadFile( va( "glsl/%s_%d.bin", pname, i ), (void **) &binary );

	// File empty or not found
	if( binaryLength <= 0 )
	{
		return false;
	}

	// Get the binaryFormat from the file
	binaryFormat = *(GLenum *)binary;

	glProgramBinary( program, binaryFormat, (char*)binary + sizeof( GLenum ), binaryLength - sizeof( GLenum ) );

	glGetProgramiv( program, GL_LINK_STATUS, &success );

	// Did it work?
	if( !success )
	{
		return false;
	}
	return true;
#else
        return false;
#endif
}

void GLShader::CompileAndLinkGPUShaderProgram( shaderProgram_t *program,
    const char *programName,
    const std::string &vertexShaderText,
    const std::string &fragmentShaderText,
    const std::string &compileMacros, int iteration ) const
{
#ifdef USE_GLSL_OPTIMIZER
	bool        optimize = r_glslOptimizer->integer ? true : false;
#endif

	//ri.Printf(PRINT_DEVELOPER, "------- GPU shader -------\n");

	Q_strncpyz( program->name, programName, sizeof( program->name ) );

#if 0

	if ( !compileMacros.empty() )
	{
		program->compileMacros = ( char * ) ri.Hunk_Alloc( sizeof( char ) * compileMacros.length() + 1, h_low );
		Q_strncpyz( program->compileMacros, compileMacros.c_str(), compileMacros.length() + 1 );
	}
	else
#endif
	{
		program->compileMacros = NULL;
	}

	program->program = glCreateProgram();
	program->attribs = _vertexAttribsRequired; // | _vertexAttribsOptional;

	if( r_recompileShaders->integer || !LoadShaderProgram( program->program, programName, iteration ) )
	{
		// header of the glsl shader
		std::string vertexHeader;
		std::string fragmentHeader;

		if ( glConfig.driverType == GLDRV_OPENGL3 )
		{
			// HACK: abuse the GLSL preprocessor to turn GLSL 1.20 shaders into 1.30 ones

			vertexHeader += "#version 130\n";
			fragmentHeader += "#version 130\n";

			vertexHeader += "#define attribute in\n";
			vertexHeader += "#define varying out\n";

			fragmentHeader += "#define varying in\n";

			fragmentHeader += "out vec4 out_Color;\n";
			fragmentHeader += "#define gl_FragColor out_Color\n";

			vertexHeader += "#define textureCube texture\n";
			fragmentHeader += "#define textureCube texture\n";
		}
		else
		{
			vertexHeader += "#version 120\n";
			fragmentHeader += "#version 120\n";
		}

		// permutation macros
		std::string macrosString;

		if ( !compileMacros.empty() )
		{
			const char *compileMacros_ = compileMacros.c_str();
			char       **compileMacrosP = ( char ** ) &compileMacros_;
			char       *token;

			while ( 1 )
			{
				token = COM_ParseExt2( compileMacrosP, qfalse );

				if ( !token[ 0 ] )
				{
					break;
				}

				macrosString += va( "#ifndef %s\n#define %s 1\n#endif\n", token, token );
			}
		}

		// add them
		std::string vertexShaderTextWithMacros = vertexHeader + macrosString + vertexShaderText;
		std::string fragmentShaderTextWithMacros = fragmentHeader + macrosString + fragmentShaderText;
#ifdef USE_GLSL_OPTIMIZER
		if( optimize )
		{
			static char         msgPart[ 1024 ];
			int                 length = 0;
			int                 i;

			const std::string version = ( glConfig.driverType == GLDRV_OPENGL3 ) ? "#version 130\n" : "#version 120\n";

			glslopt_shader *shaderOptimized = glslopt_optimize( s_glslOptimizer, kGlslOptShaderVertex, vertexShaderTextWithMacros.c_str(), 0 );
			if( glslopt_get_status( shaderOptimized ) )
			{
				vertexShaderTextWithMacros = version + glslopt_get_output( shaderOptimized );

				ri.Printf( PRINT_DEVELOPER, "----------------------------------------------------------\n" );
				ri.Printf( PRINT_DEVELOPER, "OPTIMIZED VERTEX shader '%s' ----------\n", programName );
				ri.Printf( PRINT_DEVELOPER, " BEGIN ---------------------------------------------------\n" );

				length = strlen( vertexShaderTextWithMacros.c_str() );

				for ( i = 0; i < length; i += 1024 )
				{
					Q_strncpyz( msgPart, vertexShaderTextWithMacros.c_str() + i, sizeof( msgPart ) );
					ri.Printf( PRINT_DEVELOPER, "%s\n", msgPart );
				}

				ri.Printf( PRINT_DEVELOPER, " END-- ---------------------------------------------------\n" );
			}
			else
			{
				const char *errorLog = glslopt_get_log( shaderOptimized );

				length = strlen( errorLog );

				for ( i = 0; i < length; i += 1024 )
				{
					Q_strncpyz( msgPart, errorLog + i, sizeof( msgPart ) );
					ri.Printf( PRINT_WARNING, "%s\n", msgPart );
				}

				ri.Printf( PRINT_WARNING, "^1Couldn't optimize VERTEX shader %s\n", programName );
			}
			glslopt_shader_delete( shaderOptimized );


			glslopt_shader *shaderOptimized1 = glslopt_optimize( s_glslOptimizer, kGlslOptShaderFragment, fragmentShaderTextWithMacros.c_str(), 0 );
			if( glslopt_get_status( shaderOptimized1 ) )
			{
				fragmentShaderTextWithMacros = version + glslopt_get_output( shaderOptimized1 );

				ri.Printf( PRINT_DEVELOPER, "----------------------------------------------------------\n" );
				ri.Printf( PRINT_DEVELOPER, "OPTIMIZED FRAGMENT shader '%s' ----------\n", programName );
				ri.Printf( PRINT_DEVELOPER, " BEGIN ---------------------------------------------------\n" );

				length = strlen( fragmentShaderTextWithMacros.c_str() );

				for ( i = 0; i < length; i += 1024 )
				{
					Q_strncpyz( msgPart, fragmentShaderTextWithMacros.c_str() + i, sizeof( msgPart ) );
					ri.Printf( PRINT_DEVELOPER, "%s\n", msgPart );
				}

				ri.Printf( PRINT_DEVELOPER, " END-- ---------------------------------------------------\n" );
			}
			else
			{
				const char *errorLog = glslopt_get_log( shaderOptimized1 );

				length = strlen( errorLog );

				for ( i = 0; i < length; i += 1024 )
				{
					Q_strncpyz( msgPart, errorLog + i, sizeof( msgPart ) );
					ri.Printf( PRINT_WARNING, "%s\n", msgPart );
				}

				ri.Printf( PRINT_WARNING, "^1Couldn't optimize FRAGMENT shader %s\n", programName );
			}
			glslopt_shader_delete( shaderOptimized1 );
		}
#endif
		CompileGPUShader( program->program, programName, vertexShaderTextWithMacros.c_str(), strlen( vertexShaderTextWithMacros.c_str() ), GL_VERTEX_SHADER );
		CompileGPUShader( program->program, programName, fragmentShaderTextWithMacros.c_str(), strlen( fragmentShaderTextWithMacros.c_str() ), GL_FRAGMENT_SHADER );
		BindAttribLocations( program->program );  //, _vertexAttribsRequired | _vertexAttribsOptional);
		LinkProgram( program->program );
	}

}

void GLShader::CompileGPUShader( GLuint program, const char *programName, const char *shaderText, int shaderTextSize, GLenum shaderType ) const
{
	GLuint shader = glCreateShader( shaderType );

	GL_CheckErrors();

	glShaderSource( shader, 1, ( const GLchar ** ) &shaderText, &shaderTextSize );

	// compile shader
	glCompileShader( shader );

	GL_CheckErrors();

	// check if shader compiled
	GLint compiled;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );

	if ( !compiled )
	{
		PrintShaderSource( shader );
		PrintInfoLog( shader, qfalse );
		ri.Error( ERR_DROP, "Couldn't compile %s %s", ( shaderType == GL_VERTEX_SHADER ? "vertex shader" : "fragment shader" ), programName );
	}

	//PrintInfoLog(shader, qtrue);
	//ri.Printf(PRINT_ALL, "%s\n", GLSL_PrintShaderSource(shader));

	// attach shader to program
	glAttachShader( program, shader );
	GL_CheckErrors();

	// delete shader, no longer needed
	glDeleteShader( shader );
	GL_CheckErrors();
}

void GLShader::PrintShaderText( const std::string &shaderText ) const
{
	static char msgPart[ 1024 ];

	for ( size_t i = 0; i < shaderText.length(); i += 1024 )
	{
		Q_strncpyz( msgPart, shaderText.c_str() + i, sizeof( msgPart ) );
		ri.Printf( PRINT_ALL, "%s\n", msgPart );
	}
}

void GLShader::PrintShaderSource( GLuint object ) const
{
	char        *msg;
	static char msgPart[ 1024 ];
	int         maxLength = 0;
	int         i;

	glGetShaderiv( object, GL_SHADER_SOURCE_LENGTH, &maxLength );

	msg = ( char * ) ri.Hunk_AllocateTempMemory( maxLength );

	glGetShaderSource( object, maxLength, &maxLength, msg );

	for ( i = 0; i < maxLength; i += 1024 )
	{
		Q_strncpyz( msgPart, msg + i, sizeof( msgPart ) );
		ri.Printf( PRINT_ALL, "%s\n", msgPart );
	}

	ri.Hunk_FreeTempMemory( msg );
}

void GLShader::PrintInfoLog( GLuint object, bool developerOnly ) const
{
	char        *msg;
	static char msgPart[ 1024 ];
	int         maxLength = 0;
	int         i;

	glGetShaderiv( object, GL_INFO_LOG_LENGTH, &maxLength );

	msg = ( char * ) ri.Hunk_AllocateTempMemory( maxLength );

	glGetShaderInfoLog( object, maxLength, &maxLength, msg );

	if ( developerOnly )
	{
		ri.Printf( PRINT_DEVELOPER, "compile log:\n" );
	}
	else
	{
		ri.Printf( PRINT_ALL, "compile log:\n" );
	}

	for ( i = 0; i < maxLength; i += 1024 )
	{
		Q_strncpyz( msgPart, msg + i, sizeof( msgPart ) );

		if ( developerOnly )
		{
			ri.Printf( PRINT_DEVELOPER, "%s\n", msgPart );
		}
		else
		{
			ri.Printf( PRINT_ALL, "%s\n", msgPart );
		}
	}

	ri.Hunk_FreeTempMemory( msg );
}

void GLShader::LinkProgram( GLuint program ) const
{
	GLint linked;

#ifdef GLEW_ARB_get_program_binary
	// Apparently, this is necessary to get the binary program via glGetProgramBinary
	if( GLEW_ARB_get_program_binary )
	{
		glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE );
	}
#endif
	glLinkProgram( program );

	glGetProgramiv( program, GL_LINK_STATUS, &linked );

	if ( !linked )
	{
		PrintInfoLog( program, qfalse );
		ri.Error( ERR_DROP, "Shaders failed to link!!!" );
	}
}

void GLShader::ValidateProgram( GLuint program ) const
{
	GLint validated;

	glValidateProgram( program );

	glGetProgramiv( program, GL_VALIDATE_STATUS, &validated );

	if ( !validated )
	{
		PrintInfoLog( program, qfalse );
		ri.Error( ERR_DROP, "Shaders failed to validate!!!" );
	}
}

void GLShader::ShowProgramUniforms( GLuint program ) const
{
	int    i, count, size;
	GLenum type;
	char   uniformName[ 1000 ];

	// install the executables in the program object as part of current state.
	glUseProgram( program );

	// check for GL Errors

	// query the number of active uniforms
	glGetShaderiv( program, GL_ACTIVE_UNIFORMS, &count );

	// Loop over each of the active uniforms, and set their value
	for ( i = 0; i < count; i++ )
	{
		glGetActiveUniform( program, i, sizeof( uniformName ), NULL, &size, &type, uniformName );

		ri.Printf( PRINT_DEVELOPER, "active uniform: '%s'\n", uniformName );
	}

	glUseProgram( 0 );
}

void GLShader::BindAttribLocations( GLuint program ) const
{
	//if(attribs & ATTR_POSITION)
	glBindAttribLocation( program, ATTR_INDEX_POSITION, "attr_Position" );

	//if(attribs & ATTR_TEXCOORD)
	glBindAttribLocation( program, ATTR_INDEX_TEXCOORD0, "attr_TexCoord0" );

	//if(attribs & ATTR_LIGHTCOORD)
	glBindAttribLocation( program, ATTR_INDEX_TEXCOORD1, "attr_TexCoord1" );

//  if(attribs & ATTR_TEXCOORD2)
//      glBindAttribLocation(program, ATTR_INDEX_TEXCOORD2, "attr_TexCoord2");

//  if(attribs & ATTR_TEXCOORD3)
//      glBindAttribLocation(program, ATTR_INDEX_TEXCOORD3, "attr_TexCoord3");

	//if(attribs & ATTR_TANGENT)
	glBindAttribLocation( program, ATTR_INDEX_TANGENT, "attr_Tangent" );

	//if(attribs & ATTR_BINORMAL)
	glBindAttribLocation( program, ATTR_INDEX_BINORMAL, "attr_Binormal" );

	//if(attribs & ATTR_NORMAL)
	glBindAttribLocation( program, ATTR_INDEX_NORMAL, "attr_Normal" );

	//if(attribs & ATTR_COLOR)
	glBindAttribLocation( program, ATTR_INDEX_COLOR, "attr_Color" );

#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )
	//if(attribs & ATTR_PAINTCOLOR)
	glBindAttribLocation( program, ATTR_INDEX_PAINTCOLOR, "attr_PaintColor" );

	//if(attribs & ATTR_LIGHTDIRECTION)
	glBindAttribLocation( program, ATTR_INDEX_LIGHTDIRECTION, "attr_LightDirection" );
#endif

	//if(glConfig2.vboVertexSkinningAvailable)
	{
		glBindAttribLocation( program, ATTR_INDEX_BONE_INDEXES, "attr_BoneIndexes" );
		glBindAttribLocation( program, ATTR_INDEX_BONE_WEIGHTS, "attr_BoneWeights" );
	}

	//if(attribs & ATTR_POSITION2)
	glBindAttribLocation( program, ATTR_INDEX_POSITION2, "attr_Position2" );

	//if(attribs & ATTR_TANGENT2)
	glBindAttribLocation( program, ATTR_INDEX_TANGENT2, "attr_Tangent2" );

	//if(attribs & ATTR_BINORMAL2)
	glBindAttribLocation( program, ATTR_INDEX_BINORMAL2, "attr_Binormal2" );

	//if(attribs & ATTR_NORMAL2)
	glBindAttribLocation( program, ATTR_INDEX_NORMAL2, "attr_Normal2" );
}

void GLShader::SelectProgram()
{
	int    index = 0;

	size_t numMacros = _compileMacros.size();

	for ( size_t i = 0; i < numMacros; i++ )
	{
		if ( _activeMacros & BIT( i ) )
		{
			index += BIT( i );
		}
	}

	_currentProgram = &_shaderPrograms[ index ];
}

void GLShader::BindProgram()
{
	SelectProgram();

	if ( _currentProgram->program == 0 )
	{
		std::string activeMacros = "";
		size_t      numMacros = _compileMacros.size();

		for ( size_t j = 0; j < numMacros; j++ )
		{
			GLCompileMacro *macro = _compileMacros[ j ];

			int           bit = macro->GetBit();

			if ( IsMacroSet( bit ) )
			{
				activeMacros += macro->GetName();
				activeMacros += " ";
			}
		}

		ri.Error( ERR_FATAL, "Invalid shader configuration: shader = '%s', macros = '%s'", _name.c_str(), activeMacros.c_str() );
	}

	GL_BindProgram( _currentProgram );
}

void GLShader::SetRequiredVertexPointers()
{
	uint32_t macroVertexAttribs = 0;
	size_t   numMacros = _compileMacros.size();

	for ( size_t j = 0; j < numMacros; j++ )
	{
		GLCompileMacro *macro = _compileMacros[ j ];

		int           bit = macro->GetBit();

		if ( IsMacroSet( bit ) )
		{
			macroVertexAttribs |= macro->GetRequiredVertexAttributes();
		}
	}

	GL_VertexAttribsState( ( _vertexAttribsRequired | _vertexAttribs | macroVertexAttribs ) );  // & ~_vertexAttribsUnsupported);
}

GLShader_generic::GLShader_generic() :
	GLShader( "generic", ATTR_POSITION | ATTR_TEXCOORD | ATTR_NORMAL ),
	u_ColorMap( this ),
	u_ColorTextureMatrix( this ),
	u_ViewOrigin( this ),
	u_AlphaTest( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_BoneMatrix( this ),
	u_VertexInterpolation( this ),
	u_PortalPlane( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_ALPHA_TESTING( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this ),
	GLCompileMacro_USE_TCGEN_ENVIRONMENT( this ),
	GLCompileMacro_USE_TCGEN_LIGHTMAP( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating generic shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "vertexSkinning vertexAnimation ";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "generic", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "generic", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling generic shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			ri.Printf( PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str() );

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "generic",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			//shaderProgram->u_ColorMap = glGetUniformLocation(shaderProgram->program, "u_ColorMap");

			//ri.Printf(PRINT_ALL, "u_ColorMap = %i\n", shaderProgram->u_ColorMap);

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_ColorMap, 0 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);

			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	ri.Printf( PRINT_ALL, "\n" );

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i generic shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_lightMapping::GLShader_lightMapping() :
	GLShader( "lightMapping", ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_NORMAL ),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_AlphaTest( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_PortalPlane( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_ALPHA_TESTING( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this )  //,
	//GLCompileMacro_TWOSIDED(this)
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating lightMapping shaders -------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "lightMapping", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "lightMapping", "reliefMapping", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling lightMapping shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			compileMacros += "TWOSIDED ";

			//ri.Printf(PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "lightMapping",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_DiffuseMap = glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" );
			shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, "u_NormalMap" );
			shaderProgram->u_SpecularMap = glGetUniformLocation( shaderProgram->program, "u_SpecularMap" );
			shaderProgram->u_LightMap = glGetUniformLocation( shaderProgram->program, "u_LightMap" );
			shaderProgram->u_DeluxeMap = glGetUniformLocation( shaderProgram->program, "u_DeluxeMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_DiffuseMap, 0 );
			glUniform1i( shaderProgram->u_NormalMap, 1 );
			glUniform1i( shaderProgram->u_SpecularMap, 2 );
			glUniform1i( shaderProgram->u_LightMap, 3 );
			glUniform1i( shaderProgram->u_DeluxeMap, 4 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	ri.Printf( PRINT_ALL, "\n" );

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i lightMapping shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_vertexLighting_DBS_entity::GLShader_vertexLighting_DBS_entity() :
	GLShader( "vertexLighting_DBS_entity", ATTR_POSITION | ATTR_TEXCOORD | ATTR_NORMAL ),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_AlphaTest( this ),
	u_AmbientColor( this ),
	u_ViewOrigin( this ),
	u_LightDir( this ),
	u_LightColor( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_BoneMatrix( this ),
	u_VertexInterpolation( this ),
	u_PortalPlane( this ),
	u_DepthScale( this ),
	u_EnvironmentInterpolation( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_ALPHA_TESTING( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this ),
	GLCompileMacro_USE_REFLECTIVE_SPECULAR( this )  //,
	//GLCompileMacro_TWOSIDED(this)
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating vertexLighting_DBS_entity shaders ------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "vertexSkinning vertexAnimation ";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "vertexLighting_DBS_entity", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "vertexLighting_DBS_entity", "reliefMapping", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling vertexLighting_DBS_entity shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			compileMacros += "TWOSIDED ";

			//ri.Printf(PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "vertexLighting_DBS_entity",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_DiffuseMap = glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" );
			shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, "u_NormalMap" );
			shaderProgram->u_SpecularMap = glGetUniformLocation( shaderProgram->program, "u_SpecularMap" );
			shaderProgram->u_EnvironmentMap0 = glGetUniformLocation( shaderProgram->program, "u_EnvironmentMap0" );
			shaderProgram->u_EnvironmentMap1 = glGetUniformLocation( shaderProgram->program, "u_EnvironmentMap1" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_DiffuseMap, 0 );
			glUniform1i( shaderProgram->u_NormalMap, 1 );
			glUniform1i( shaderProgram->u_SpecularMap, 2 );
			glUniform1i( shaderProgram->u_EnvironmentMap0, 3 );
			glUniform1i( shaderProgram->u_EnvironmentMap1, 4 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i vertexLighting_DBS_entity shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_vertexLighting_DBS_world::GLShader_vertexLighting_DBS_world() :
	GLShader( "vertexLighting_DBS_world",
	          ATTR_POSITION | ATTR_TEXCOORD | ATTR_NORMAL | ATTR_COLOR
#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )
	          | ATTR_LIGHTDIRECTION
#endif
	        ),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_AlphaTest( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_PortalPlane( this ),
	u_DepthScale( this ),
	u_LightWrapAround( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_ALPHA_TESTING( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this )  //,
	//GLCompileMacro_TWOSIDED(this)
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating vertexLighting_DBS_world shaders -------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "vertexLighting_DBS_world", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "vertexLighting_DBS_world", "reliefMapping", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling vertexLighting_DBS_world shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			compileMacros += "TWOSIDED ";

			//ri.Printf(PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "vertexLighting_DBS_world",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_DiffuseMap = glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" );
			shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, "u_NormalMap" );
			shaderProgram->u_SpecularMap = glGetUniformLocation( shaderProgram->program, "u_SpecularMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_DiffuseMap, 0 );
			glUniform1i( shaderProgram->u_NormalMap, 1 );
			glUniform1i( shaderProgram->u_SpecularMap, 2 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i vertexLighting_DBS_world shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_forwardLighting_omniXYZ::GLShader_forwardLighting_omniXYZ() :
	GLShader( "forwardLighting_omniXYZ", ATTR_POSITION | ATTR_TEXCOORD | ATTR_NORMAL ),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_AlphaTest( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_ViewOrigin( this ),
	u_LightOrigin( this ),
	u_LightColor( this ),
	u_LightRadius( this ),
	u_LightScale( this ),
	u_LightWrapAround( this ),
	u_LightAttenuationMatrix( this ),
	u_ShadowTexelSize( this ),
	u_ShadowBlur( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_BoneMatrix( this ),
	u_VertexInterpolation( this ),
	u_PortalPlane( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_ALPHA_TESTING( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )  //,
	//GLCompileMacro_TWOSIDED(this)
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating forwardLighting_omniXYZ shaders --------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "vertexSkinning vertexAnimation ";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "forwardLighting", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "forwardLighting", "reliefMapping", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling forwardLighting_omniXYZ shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			compileMacros += "TWOSIDED ";

			//ri.Printf(PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "forwardLighting_omniXYZ",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_DiffuseMap = glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" );
			shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, "u_NormalMap" );
			shaderProgram->u_SpecularMap = glGetUniformLocation( shaderProgram->program, "u_SpecularMap" );
			shaderProgram->u_AttenuationMapXY = glGetUniformLocation( shaderProgram->program, "u_AttenuationMapXY" );
			shaderProgram->u_AttenuationMapZ = glGetUniformLocation( shaderProgram->program, "u_AttenuationMapZ" );
			//if(r_shadows->integer >= SHADOWING_ESM16)
			{
				shaderProgram->u_ShadowMap = glGetUniformLocation( shaderProgram->program, "u_ShadowMap" );
			}
			shaderProgram->u_RandomMap = glGetUniformLocation( shaderProgram->program, "u_RandomMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_DiffuseMap, 0 );
			glUniform1i( shaderProgram->u_NormalMap, 1 );
			glUniform1i( shaderProgram->u_SpecularMap, 2 );
			glUniform1i( shaderProgram->u_AttenuationMapXY, 3 );
			glUniform1i( shaderProgram->u_AttenuationMapZ, 4 );
			//if(r_shadows->integer >= SHADOWING_ESM16)
			{
				glUniform1i( shaderProgram->u_ShadowMap, 5 );
			}
			glUniform1i( shaderProgram->u_RandomMap, 6 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i forwardLighting_omniXYZ shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_forwardLighting_projXYZ::GLShader_forwardLighting_projXYZ() :
	GLShader( "forwardLighting_projXYZ", ATTR_POSITION | ATTR_TEXCOORD | ATTR_NORMAL ),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_AlphaTest( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_ViewOrigin( this ),
	u_LightOrigin( this ),
	u_LightColor( this ),
	u_LightRadius( this ),
	u_LightScale( this ),
	u_LightWrapAround( this ),
	u_LightAttenuationMatrix( this ),
	u_ShadowTexelSize( this ),
	u_ShadowBlur( this ),
	u_ShadowMatrix( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_BoneMatrix( this ),
	u_VertexInterpolation( this ),
	u_PortalPlane( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_ALPHA_TESTING( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )  //,
	//GLCompileMacro_TWOSIDED(this)
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating forwardLighting_projXYZ shaders --------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "vertexSkinning vertexAnimation ";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "forwardLighting", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "forwardLighting", "reliefMapping", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling forwardLighting_projXYZ shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			compileMacros += "LIGHT_PROJ ";
			compileMacros += "TWOSIDED ";

			//ri.Printf(PRINT_ALL, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "forwardLighting_projXYZ",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_DiffuseMap = glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" );
			shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, "u_NormalMap" );
			shaderProgram->u_SpecularMap = glGetUniformLocation( shaderProgram->program, "u_SpecularMap" );
			shaderProgram->u_AttenuationMapXY = glGetUniformLocation( shaderProgram->program, "u_AttenuationMapXY" );
			shaderProgram->u_AttenuationMapZ = glGetUniformLocation( shaderProgram->program, "u_AttenuationMapZ" );
			//if(r_shadows->integer >= SHADOWING_VSM16)
			{
				shaderProgram->u_ShadowMap0 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap0" );
			}
			shaderProgram->u_RandomMap = glGetUniformLocation( shaderProgram->program, "u_RandomMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_DiffuseMap, 0 );
			glUniform1i( shaderProgram->u_NormalMap, 1 );
			glUniform1i( shaderProgram->u_SpecularMap, 2 );
			glUniform1i( shaderProgram->u_AttenuationMapXY, 3 );
			glUniform1i( shaderProgram->u_AttenuationMapZ, 4 );
			//if(r_shadows->integer >= SHADOWING_ESM16)
			{
				glUniform1i( shaderProgram->u_ShadowMap0, 5 );
			}
			glUniform1i( shaderProgram->u_RandomMap, 6 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i forwardLighting_projXYZ shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_forwardLighting_directionalSun::GLShader_forwardLighting_directionalSun() :
	GLShader( "forwardLighting_directionalSun", ATTR_POSITION | ATTR_TEXCOORD | ATTR_NORMAL ),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_AlphaTest( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_ViewOrigin( this ),
	u_LightDir( this ),
	u_LightColor( this ),
	u_LightRadius( this ),
	u_LightScale( this ),
	u_LightWrapAround( this ),
	u_LightAttenuationMatrix( this ),
	u_ShadowTexelSize( this ),
	u_ShadowBlur( this ),
	u_ShadowMatrix( this ),
	u_ShadowParallelSplitDistances( this ),
	u_ModelMatrix( this ),
	u_ViewMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_BoneMatrix( this ),
	u_VertexInterpolation( this ),
	u_PortalPlane( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_ALPHA_TESTING( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )  //,
	//GLCompileMacro_TWOSIDED(this)
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating forwardLighting_directionalSun shaders -\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "vertexSkinning vertexAnimation ";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "forwardLighting", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "forwardLighting", "reliefMapping", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling forwardLighting_directionalSun shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			compileMacros += "LIGHT_DIRECTIONAL ";
			compileMacros += "TWOSIDED ";

			//ri.Printf(PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "forwardLighting",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_DiffuseMap = glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" );
			shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, "u_NormalMap" );
			shaderProgram->u_SpecularMap = glGetUniformLocation( shaderProgram->program, "u_SpecularMap" );
			//if(r_shadows->integer >= SHADOWING_ESM16)
			{
				shaderProgram->u_ShadowMap0 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap0" );
				shaderProgram->u_ShadowMap1 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap1" );
				shaderProgram->u_ShadowMap2 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap2" );
				shaderProgram->u_ShadowMap3 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap3" );
				shaderProgram->u_ShadowMap4 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap4" );
			}

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_DiffuseMap, 0 );
			glUniform1i( shaderProgram->u_NormalMap, 1 );
			glUniform1i( shaderProgram->u_SpecularMap, 2 );
			//glUniform1i(shaderProgram->u_AttenuationMapXY, 3);
			//glUniform1i(shaderProgram->u_AttenuationMapZ, 4);
			//if(r_shadows->integer >= SHADOWING_ESM16)
			{
				glUniform1i( shaderProgram->u_ShadowMap0, 5 );
				glUniform1i( shaderProgram->u_ShadowMap1, 6 );
				glUniform1i( shaderProgram->u_ShadowMap2, 7 );
				glUniform1i( shaderProgram->u_ShadowMap3, 8 );
				glUniform1i( shaderProgram->u_ShadowMap4, 9 );
			}
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i forwardLighting_directionalSun shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_deferredLighting_omniXYZ::GLShader_deferredLighting_omniXYZ() :
	GLShader( "deferredLighting_omniXYZ", ATTR_POSITION ),
	u_ViewOrigin( this ),
	u_LightOrigin( this ),
	u_LightColor( this ),
	u_LightRadius( this ),
	u_LightScale( this ),
	u_LightWrapAround( this ),
	u_LightAttenuationMatrix( this ),
	u_LightFrustum( this ),
	u_ShadowTexelSize( this ),
	u_ShadowBlur( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_UnprojectMatrix( this ),
	u_PortalPlane( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_FRUSTUM_CLIPPING( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )  //,
	//GLCompileMacro_TWOSIDED(this)
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating deferredLighting_omniXYZ shaders --------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "deferredLighting", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "deferredLighting", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling deferredLighting_omniXYZ shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			//compileMacros += "TWOSIDED ";

			//ri.Printf(PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "deferredLighting_omniXYZ",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_DiffuseMap = glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" );
			shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, "u_NormalMap" );
			shaderProgram->u_SpecularMap = glGetUniformLocation( shaderProgram->program, "u_SpecularMap" );
			shaderProgram->u_DepthMap = glGetUniformLocation( shaderProgram->program, "u_DepthMap" );
			shaderProgram->u_AttenuationMapXY = glGetUniformLocation( shaderProgram->program, "u_AttenuationMapXY" );
			shaderProgram->u_AttenuationMapZ = glGetUniformLocation( shaderProgram->program, "u_AttenuationMapZ" );
			//if(r_shadows->integer >= SHADOWING_ESM16)
			{
				shaderProgram->u_ShadowMap = glGetUniformLocation( shaderProgram->program, "u_ShadowMap" );
			}

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_DiffuseMap, 0 );
			glUniform1i( shaderProgram->u_NormalMap, 1 );
			glUniform1i( shaderProgram->u_SpecularMap, 2 );
			glUniform1i( shaderProgram->u_DepthMap, 3 );
			glUniform1i( shaderProgram->u_AttenuationMapXY, 4 );
			glUniform1i( shaderProgram->u_AttenuationMapZ, 5 );
			//if(r_shadows->integer >= SHADOWING_ESM16)
			{
				glUniform1i( shaderProgram->u_ShadowMap, 6 );
			}
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i deferredLighting_omniXYZ shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_deferredLighting_projXYZ::GLShader_deferredLighting_projXYZ() :
	GLShader( "deferredLighting_projXYZ", ATTR_POSITION ),
	u_ViewOrigin( this ),
	u_LightOrigin( this ),
	u_LightColor( this ),
	u_LightRadius( this ),
	u_LightScale( this ),
	u_LightWrapAround( this ),
	u_LightAttenuationMatrix( this ),
	u_LightFrustum( this ),
	u_ShadowTexelSize( this ),
	u_ShadowBlur( this ),
	u_ShadowMatrix( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_UnprojectMatrix( this ),
	u_PortalPlane( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_FRUSTUM_CLIPPING( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )  //,
	//GLCompileMacro_TWOSIDED(this)
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating deferredLighting_projXYZ shaders --------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "deferredLighting", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "deferredLighting", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling deferredLighting_projXYZ shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			compileMacros += "LIGHT_PROJ ";

			//ri.Printf(PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "deferredLighting_projXYZ",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_DiffuseMap = glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" );
			shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, "u_NormalMap" );
			shaderProgram->u_SpecularMap = glGetUniformLocation( shaderProgram->program, "u_SpecularMap" );
			shaderProgram->u_DepthMap = glGetUniformLocation( shaderProgram->program, "u_DepthMap" );
			shaderProgram->u_AttenuationMapXY = glGetUniformLocation( shaderProgram->program, "u_AttenuationMapXY" );
			shaderProgram->u_AttenuationMapZ = glGetUniformLocation( shaderProgram->program, "u_AttenuationMapZ" );
			//if(r_shadows->integer >= SHADOWING_ESM16)
			{
				shaderProgram->u_ShadowMap0 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap0" );
			}

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_DiffuseMap, 0 );
			glUniform1i( shaderProgram->u_NormalMap, 1 );
			glUniform1i( shaderProgram->u_SpecularMap, 2 );
			glUniform1i( shaderProgram->u_DepthMap, 3 );
			glUniform1i( shaderProgram->u_AttenuationMapXY, 4 );
			glUniform1i( shaderProgram->u_AttenuationMapZ, 5 );
			//if(r_shadows->integer >= SHADOWING_ESM16)
			{
				glUniform1i( shaderProgram->u_ShadowMap0, 6 );
			}
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i deferredLighting_projXYZ shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_deferredLighting_directionalSun::GLShader_deferredLighting_directionalSun() :
	GLShader( "deferredLighting_directionalSun", ATTR_POSITION ),
	u_ViewOrigin( this ),
	u_LightDir( this ),
	u_LightColor( this ),
	u_LightRadius( this ),
	u_LightScale( this ),
	u_LightWrapAround( this ),
	u_LightAttenuationMatrix( this ),
	u_LightFrustum( this ),
	u_ShadowTexelSize( this ),
	u_ShadowBlur( this ),
	u_ShadowMatrix( this ),
	u_ShadowParallelSplitDistances( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ViewMatrix( this ),
	u_UnprojectMatrix( this ),
	u_PortalPlane( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_FRUSTUM_CLIPPING( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )  //,
	//GLCompileMacro_TWOSIDED(this)
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating deferredLighting_directionalSun shaders --------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "deferredLighting", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "deferredLighting", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling deferredLighting_directionalSun shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			compileMacros += "LIGHT_DIRECTIONAL ";

			//ri.Printf(PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "deferredLighting_directionalSun",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_DiffuseMap = glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" );
			shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, "u_NormalMap" );
			shaderProgram->u_SpecularMap = glGetUniformLocation( shaderProgram->program, "u_SpecularMap" );
			shaderProgram->u_DepthMap = glGetUniformLocation( shaderProgram->program, "u_DepthMap" );
			//if(r_shadows->integer >= SHADOWING_ESM16)
			{
				shaderProgram->u_ShadowMap0 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap0" );
				shaderProgram->u_ShadowMap1 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap1" );
				shaderProgram->u_ShadowMap2 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap2" );
				shaderProgram->u_ShadowMap3 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap3" );
				shaderProgram->u_ShadowMap4 = glGetUniformLocation( shaderProgram->program, "u_ShadowMap4" );
			}

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_DiffuseMap, 0 );
			glUniform1i( shaderProgram->u_NormalMap, 1 );
			glUniform1i( shaderProgram->u_SpecularMap, 2 );
			glUniform1i( shaderProgram->u_DepthMap, 3 );
			//glUniform1i(shaderProgram->u_AttenuationMapXY, 4);
			//glUniform1i(shaderProgram->u_AttenuationMapZ, 5);
			//if(r_shadows->integer >= SHADOWING_ESM16)
			{
				glUniform1i( shaderProgram->u_ShadowMap0, 6 );
				glUniform1i( shaderProgram->u_ShadowMap1, 7 );
				glUniform1i( shaderProgram->u_ShadowMap2, 8 );
				glUniform1i( shaderProgram->u_ShadowMap3, 9 );
				glUniform1i( shaderProgram->u_ShadowMap4, 10 );
			}
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i deferredLighting_directionalSun shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_geometricFill::GLShader_geometricFill() :
	GLShader( "geometricFill", ATTR_POSITION | ATTR_TEXCOORD | ATTR_NORMAL ),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_AlphaTest( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_BoneMatrix( this ),
	u_VertexInterpolation( this ),
	u_PortalPlane( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_ALPHA_TESTING( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this ),
	GLCompileMacro_USE_REFLECTIVE_SPECULAR( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating geometricFill shaders ---------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "vertexSkinning vertexAnimation ";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "geometricFill", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "geometricFill", "reliefMapping", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling geometricFill shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			compileMacros += "TWOSIDED ";

			//ri.Printf(PRINT_ALL, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "geometricFill",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_DiffuseMap = glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" );
			shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, "u_NormalMap" );
			shaderProgram->u_SpecularMap = glGetUniformLocation( shaderProgram->program, "u_SpecularMap" );
			shaderProgram->u_EnvironmentMap0 = glGetUniformLocation( shaderProgram->program, "u_EnvironmentMap0" );
			shaderProgram->u_EnvironmentMap1 = glGetUniformLocation( shaderProgram->program, "u_EnvironmentMap1" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_DiffuseMap, 0 );
			glUniform1i( shaderProgram->u_NormalMap, 1 );
			glUniform1i( shaderProgram->u_SpecularMap, 2 );
			glUniform1i( shaderProgram->u_EnvironmentMap0, 3 );
			glUniform1i( shaderProgram->u_EnvironmentMap1, 4 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i geometricFill shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_shadowFill::GLShader_shadowFill() :
	GLShader( "shadowFill", ATTR_POSITION | ATTR_TEXCOORD | ATTR_NORMAL ),
	u_ColorTextureMatrix( this ),
	u_ViewOrigin( this ),
	u_AlphaTest( this ),
	u_LightOrigin( this ),
	u_LightRadius( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Color( this ),
	u_BoneMatrix( this ),
	u_VertexInterpolation( this ),
	u_PortalPlane( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_ALPHA_TESTING( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this ),
	GLCompileMacro_LIGHT_DIRECTIONAL( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating shadowFill shaders ---------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "vertexSkinning vertexAnimation ";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "shadowFill", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "shadowFill", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling shadowFill shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			//ri.Printf(PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "shadowFill",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_ColorMap = glGetUniformLocation( shaderProgram->program, "u_ColorMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_ColorMap, 0 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i shadowFill shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_reflection::GLShader_reflection() :
	GLShader( "reflection", ATTR_POSITION | ATTR_TEXCOORD | ATTR_NORMAL ),
	u_ColorMap( this ),
	u_NormalMap( this ),
	u_NormalTextureMatrix( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_BoneMatrix( this ),
	u_VertexInterpolation( this ),
	u_PortalPlane( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this )  //,
	//GLCompileMacro_TWOSIDED(this)
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating reflection shaders ---------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "vertexSkinning vertexAnimation ";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "reflection_CB", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "reflection_CB", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling reflection shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			compileMacros += "TWOSIDED ";

			//ri.Printf(PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "reflection",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_ColorMap, 0 );
			glUniform1i( shaderProgram->u_NormalMap, 1 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	ri.Printf( PRINT_ALL, "\n" );

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i reflection shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_skybox::GLShader_skybox() :
	GLShader( "skybox", ATTR_POSITION ),
	u_ColorMap( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_BoneMatrix( this ),
	u_VertexInterpolation( this ),
	u_PortalPlane( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating skybox shaders -------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "skybox", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "skybox", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling skybox shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			//ri.Printf(PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "skybox",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_ColorMap, 0 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i skybox shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_fogQuake3::GLShader_fogQuake3() :
	GLShader( "fogQuake3", ATTR_POSITION | ATTR_NORMAL ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Color( this ),
	u_BoneMatrix( this ),
	u_VertexInterpolation( this ),
	u_PortalPlane( this ),
	u_FogDistanceVector( this ),
	u_FogDepthVector( this ),
	u_FogEyeT( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	//GLCompileMacro_USE_ALPHA_TESTING(this),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this ),
	GLCompileMacro_EYE_OUTSIDE( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating fogQuake3 shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "vertexSkinning vertexAnimation ";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "fogQuake3", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "fogQuake3", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling fogQuake3 shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			//ri.Printf(PRINT_ALL, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "fogQuake3",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_ColorMap = glGetUniformLocation( shaderProgram->program, "u_ColorMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_ColorMap, 0 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i fogQuake3 shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_fogGlobal::GLShader_fogGlobal() :
	GLShader( "fogGlobal", ATTR_POSITION ),
	u_ViewOrigin( this ),
	u_ViewMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_UnprojectMatrix( this ),
	u_Color( this ),
	u_FogDistanceVector( this ),
	u_FogDepthVector( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating fogGlobal shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "fogGlobal", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "fogGlobal", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling fogGlobal shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			//ri.Printf(PRINT_ALL, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "fogGlobal",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_ColorMap = glGetUniformLocation( shaderProgram->program, "u_ColorMap" );
			shaderProgram->u_DepthMap = glGetUniformLocation( shaderProgram->program, "u_DepthMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_ColorMap, 0 );
			glUniform1i( shaderProgram->u_DepthMap, 1 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i fogQuake3 shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_heatHaze::GLShader_heatHaze() :
	GLShader( "heatHaze", ATTR_POSITION | ATTR_TEXCOORD | ATTR_NORMAL ),
	u_NormalTextureMatrix( this ),
	u_ViewOrigin( this ),
	//u_AlphaTest(this),
	u_DeformMagnitude( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ModelViewMatrixTranspose( this ),
	u_ProjectionMatrixTranspose( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_BoneMatrix( this ),
	u_VertexInterpolation( this ),
	u_PortalPlane( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_PORTAL_CLIPPING( this ),
	//GLCompileMacro_USE_ALPHA_TESTING(this),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_DEFORM_VERTEXES( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating heatHaze shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexInlines = "vertexSkinning vertexAnimation ";

	if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
	{
		vertexInlines += "deformVertexes ";
	}

	std::string vertexShaderText = BuildGPUShaderText( "heatHaze", vertexInlines.c_str(), GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "heatHaze", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling heatHaze shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			//ri.Printf(PRINT_ALL, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "heatHaze",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, "u_NormalMap" );
			shaderProgram->u_CurrentMap = glGetUniformLocation( shaderProgram->program, "u_CurrentMap" );

			if ( r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable && /*glConfig.hardwareType != GLHW_ATI && glConfig.hardwareType != GLHW_ATI_DX10 &&*/ glConfig.driverType != GLDRV_MESA )
			{
				shaderProgram->u_ContrastMap = glGetUniformLocation( shaderProgram->program, "u_ContrastMap" );
			}

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_NormalMap, 0 );
			glUniform1i( shaderProgram->u_CurrentMap, 1 );

			if ( r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable && /*glConfig.hardwareType != GLHW_ATI && glConfig.hardwareType != GLHW_ATI_DX10 &&*/ glConfig.driverType != GLDRV_MESA )
			{
				glUniform1i( shaderProgram->u_ContrastMap, 2 );
			}

			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	ri.Printf( PRINT_ALL, "\n" );

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i heatHaze shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_screen::GLShader_screen() :
	GLShader( "screen", ATTR_POSITION ),
	u_ModelViewProjectionMatrix( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating screen shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "screen", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "screen", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			ri.Printf( PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str() );

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "screen",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_CurrentMap = glGetUniformLocation( shaderProgram->program, "u_CurrentMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_CurrentMap, 0 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i screen shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_portal::GLShader_portal() :
	GLShader( "portal", ATTR_POSITION ),
	u_ModelViewMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_PortalRange( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating portal shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "portal", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "portal", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			ri.Printf( PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str() );

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "portal",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_CurrentMap = glGetUniformLocation( shaderProgram->program, "u_CurrentMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_CurrentMap, 0 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i portal shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_toneMapping::GLShader_toneMapping() :
	GLShader( "toneMapping", ATTR_POSITION ),
	u_ModelViewProjectionMatrix( this ),
	u_HDRKey( this ),
	u_HDRAverageLuminance( this ),
	u_HDRMaxLuminance( this ),
	GLCompileMacro_BRIGHTPASS_FILTER( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating toneMapping shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "toneMapping", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "toneMapping", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling toneMapping shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			//ri.Printf(PRINT_ALL, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "toneMapping",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_CurrentMap = glGetUniformLocation( shaderProgram->program, "u_CurrentMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_CurrentMap, 0 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	ri.Printf( PRINT_ALL, "\n" );

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i toneMapping shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_contrast::GLShader_contrast() :
	GLShader( "contrast", ATTR_POSITION ),
	u_ModelViewProjectionMatrix( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating contrast shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "contrast", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "contrast", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling contrast shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			//ri.Printf(PRINT_ALL, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "contrast",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_ColorMap = glGetUniformLocation( shaderProgram->program, "u_ColorMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_ColorMap, 0 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	ri.Printf( PRINT_ALL, "\n" );

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i contrast shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_cameraEffects::GLShader_cameraEffects() :
	GLShader( "cameraEffects", ATTR_POSITION | ATTR_TEXCOORD ),
	u_ColorTextureMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_DeformMagnitude( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating cameraEffects shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "cameraEffects", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "cameraEffects", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling cameraEffects shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			//ri.Printf(PRINT_ALL, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "cameraEffects",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_CurrentMap = glGetUniformLocation( shaderProgram->program, "u_CurrentMap" );
			shaderProgram->u_GrainMap = glGetUniformLocation( shaderProgram->program, "u_GrainMap" );
			shaderProgram->u_VignetteMap = glGetUniformLocation( shaderProgram->program, "u_VignetteMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_CurrentMap, 0 );
			glUniform1i( shaderProgram->u_GrainMap, 1 );
			glUniform1i( shaderProgram->u_VignetteMap, 2 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	ri.Printf( PRINT_ALL, "\n" );

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i cameraEffects shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_blurX::GLShader_blurX() :
	GLShader( "blurX", ATTR_POSITION ),
	u_ModelViewProjectionMatrix( this ),
	u_DeformMagnitude( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating blurX shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "blurX", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "blurX", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling blurX shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			//ri.Printf(PRINT_ALL, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "blurX",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_ColorMap = glGetUniformLocation( shaderProgram->program, "u_ColorMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_ColorMap, 0 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	ri.Printf( PRINT_ALL, "\n" );

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i blurX shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_blurY::GLShader_blurY() :
	GLShader( "blurY", ATTR_POSITION ),
	u_ModelViewProjectionMatrix( this ),
	u_DeformMagnitude( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating blurY shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "blurY", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "blurY", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;
	ri.Printf( PRINT_ALL, "...compiling blurY shaders\n" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );
	size_t tics = 0;
	size_t nextTicCount = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		if ( ( i + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( i + 1 ) / numPermutations ) * 50.0 );

			do { ri.Printf( PRINT_ALL, "*" ); }
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * numPermutations );

			if ( i == ( numPermutations - 1 ) )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			//ri.Printf(PRINT_ALL, "Compile macros: '%s'\n", compileMacros.c_str());

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "blurY",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_ColorMap = glGetUniformLocation( shaderProgram->program, "u_ColorMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_ColorMap, 0 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	ri.Printf( PRINT_ALL, "\n" );

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i blurY shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}

GLShader_debugShadowMap::GLShader_debugShadowMap() :
	GLShader( "debugShadowMap", ATTR_POSITION ),
	u_ModelViewProjectionMatrix( this )
{
	ri.Printf( PRINT_ALL, "/// -------------------------------------------------\n" );
	ri.Printf( PRINT_ALL, "/// creating debugShadowMap shaders ------------------------\n" );

	int startTime = ri.Milliseconds();

	_shaderPrograms = std::vector< shaderProgram_t > ( 1 << _compileMacros.size() );

	//Com_Memset(_shaderPrograms, 0, sizeof(_shaderPrograms));

	std::string vertexShaderText = BuildGPUShaderText( "debugShadowMap", "", GL_VERTEX_SHADER );
	std::string fragmentShaderText = BuildGPUShaderText( "debugShadowMap", "", GL_FRAGMENT_SHADER );

	size_t      numPermutations = ( 1 << _compileMacros.size() ); // same as 2^n, n = no. compile macros
	size_t      numCompiled = 0;

	for ( size_t i = 0; i < numPermutations; i++ )
	{
		std::string compileMacros;

		if ( GetCompileMacrosString( i, compileMacros ) )
		{
			ri.Printf( PRINT_DEVELOPER, "Compile macros: '%s'\n", compileMacros.c_str() );

			shaderProgram_t *shaderProgram = &_shaderPrograms[ i ];

			CompileAndLinkGPUShaderProgram( shaderProgram,
			                                "debugShadowMap",
			                                vertexShaderText,
			                                fragmentShaderText,
			                                compileMacros, i );

			UpdateShaderProgramUniformLocations( shaderProgram );

			shaderProgram->u_CurrentMap = glGetUniformLocation( shaderProgram->program, "u_CurrentMap" );

			glUseProgram( shaderProgram->program );
			glUniform1i( shaderProgram->u_CurrentMap, 0 );
			glUseProgram( 0 );

			ValidateProgram( shaderProgram->program );
			//ShowProgramUniforms(shaderProgram->program);
			GL_CheckErrors();

			numCompiled++; SaveShaderProgram( shaderProgram->program, shaderProgram->name, i );
		}
	}

	SelectProgram();

	int endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "...compiled %i debugShadowMap shader permutations in %5.2f seconds\n", ( int ) numCompiled, ( endTime - startTime ) / 1000.0 );
}
