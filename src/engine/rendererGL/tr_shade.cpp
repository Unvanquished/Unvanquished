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

static void GLSL_PrintInfoLog( GLhandleARB object, qboolean developerOnly )
{
	char        *msg;
	static char msgPart[ 1024 ];
	int         maxLength = 0;
	int         i;

	glGetObjectParameterivARB( object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &maxLength );

	msg = ( char * ) ri.Hunk_AllocateTempMemory( maxLength );

	glGetInfoLogARB( object, maxLength, &maxLength, msg );

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

static void GLSL_PrintShaderSource( GLhandleARB object )
{
	char        *msg;
	static char msgPart[ 1024 ];
	int         maxLength = 0;
	int         i;

	glGetObjectParameterivARB( object, GL_OBJECT_SHADER_SOURCE_LENGTH_ARB, &maxLength );

	msg = ( char * ) ri.Hunk_AllocateTempMemory( maxLength );

	glGetShaderSourceARB( object, maxLength, &maxLength, msg );

	for ( i = 0; i < maxLength; i += 1024 )
	{
		Q_strncpyz( msgPart, msg + i, sizeof( msgPart ) );
		ri.Printf( PRINT_ALL, "%s\n", msgPart );
	}

	ri.Hunk_FreeTempMemory( msg );
}

static void GLSL_LoadGPUShader( GLhandleARB program, const char *name, const char *_libs, const char *_compileMacros, GLenum shaderType, qboolean optimize )
{
	char        filename[ MAX_QPATH ];
	GLcharARB   *mainBuffer = NULL;
	int         mainSize;
	GLint       compiled;
	GLhandleARB shader;
	char        *token;

	int         libsSize;
	char        *libsBuffer; // all libs concatenated

	char        **libs = ( char ** ) &_libs;
	char        **compileMacros = ( char ** ) &_compileMacros;

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

		if ( shaderType == GL_VERTEX_SHADER_ARB )
		{
			Com_sprintf( filename, sizeof( filename ), "glsl/%s_vp.glsl", token );
			ri.Printf( PRINT_DEVELOPER, "...loading vertex shader '%s'\n", filename );
		}
		else
		{
			Com_sprintf( filename, sizeof( filename ), "glsl/%s_fp.glsl", token );
		}

		libSize = ri.FS_ReadFile( filename, ( void ** ) &libBuffer );

		if ( !libBuffer )
		{
			free( libsBuffer );
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
	if ( shaderType == GL_VERTEX_SHADER_ARB )
	{
		Com_sprintf( filename, sizeof( filename ), "glsl/%s_vp.glsl", name );
		ri.Printf( PRINT_DEVELOPER, "...loading vertex main() shader '%s'\n", filename );
	}
	else
	{
		Com_sprintf( filename, sizeof( filename ), "glsl/%s_fp.glsl", name );
		ri.Printf( PRINT_DEVELOPER, "...loading fragment main() shader '%s'\n", filename );
	}

	mainSize = ri.FS_ReadFile( filename, ( void ** ) &mainBuffer );

	if ( !mainBuffer )
	{
		free( libsBuffer );
		ri.Error( ERR_DROP, "Couldn't load %s", filename );
	}

	shader = glCreateShaderObjectARB( shaderType );

	GL_CheckErrors();

	{
		static char bufferExtra[ 32000 ];
		int         sizeExtra;

		char        *bufferFinal = NULL;
		int         sizeFinal;

		float       fbufWidthScale, fbufHeightScale;
		float       npotWidthScale, npotHeightScale;

		Com_Memset( bufferExtra, 0, sizeof( bufferExtra ) );

		// HACK: abuse the GLSL preprocessor to turn GLSL 1.20 shaders into 1.30 ones
		if ( glConfig.driverType == GLDRV_OPENGL3 )
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#version 130\n" );

			if ( shaderType == GL_VERTEX_SHADER_ARB )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#define attribute in\n" );
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#define varying out\n" );
			}
			else
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#define varying in\n" );

				Q_strcat( bufferExtra, sizeof( bufferExtra ), "out vec4 out_Color;\n" );
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#define gl_FragColor out_Color\n" );
			}

			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#define textureCube texture\n" );
		}
		else
		{
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#version 120\n" );
		}

		while ( 1 )
		{
			token = COM_ParseExt2( compileMacros, qfalse );

			if ( !token[ 0 ] )
			{
				break;
			}

			Q_strcat( bufferExtra, sizeof( bufferExtra ), va( "#ifndef %s\n#define %s 1\n#endif\n", token, token ) );
		}

#if defined( COMPAT_Q3A ) || defined( COMPAT_ET )
		Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef COMPAT_Q3A\n#define COMPAT_Q3A 1\n#endif\n" );
#endif

		// HACK: add some macros to avoid extra uniforms and save speed and code maintenance
		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef r_SpecularExponent\n#define r_SpecularExponent %f\n#endif\n", r_specularExponent->value ) );
		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef r_SpecularScale\n#define r_SpecularScale %f\n#endif\n", r_specularScale->value ) );
		//Q_strcat(bufferExtra, sizeof(bufferExtra),
		//       va("#ifndef r_NormalScale\n#define r_NormalScale %f\n#endif\n", r_normalScale->value));

		Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef M_PI\n#define M_PI 3.14159265358979323846f\n#endif\n" );

		Q_strcat( bufferExtra, sizeof( bufferExtra ), va( "#ifndef MAX_SHADOWMAPS\n#define MAX_SHADOWMAPS %i\n#endif\n", MAX_SHADOWMAPS ) );

		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef deformGen_t\n"
		              "#define deformGen_t\n"
		              "#define DGEN_WAVE_SIN %i\n"
		              "#define DGEN_WAVE_SQUARE %i\n"
		              "#define DGEN_WAVE_TRIANGLE %i\n"
		              "#define DGEN_WAVE_SAWTOOTH %i\n"
		              "#define DGEN_WAVE_INVERSE_SAWTOOTH %i\n"
		              "#define DGEN_BULGE %i\n"
		              "#define DGEN_MOVE %i\n"
		              "#endif\n",
		              DGEN_WAVE_SIN,
		              DGEN_WAVE_SQUARE,
		              DGEN_WAVE_TRIANGLE,
		              DGEN_WAVE_SAWTOOTH,
		              DGEN_WAVE_INVERSE_SAWTOOTH,
		              DGEN_BULGE,
		              DGEN_MOVE ) );

		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef colorGen_t\n"
		              "#define colorGen_t\n"
		              "#define CGEN_VERTEX %i\n"
		              "#define CGEN_ONE_MINUS_VERTEX %i\n"
		              "#endif\n",
		              CGEN_VERTEX,
		              CGEN_ONE_MINUS_VERTEX ) );

		Q_strcat( bufferExtra, sizeof( bufferExtra ),
		          va( "#ifndef alphaGen_t\n"
		              "#define alphaGen_t\n"
		              "#define AGEN_VERTEX %i\n"
		              "#define AGEN_ONE_MINUS_VERTEX %i\n"
		              "#endif\n",
		              AGEN_VERTEX,
		              AGEN_ONE_MINUS_VERTEX ) );

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
			if ( r_shadows->integer == SHADOWING_EVSM32 )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef ESM\n#define ESM 1\n#endif\n" );

				if ( r_debugShadowMaps->integer )
				{
					Q_strcat( bufferExtra, sizeof( bufferExtra ),
					          va( "#ifndef DEBUG_EVSM\n#define DEBUG_EVSM %i\n#endif\n", r_debugShadowMaps->integer ) );
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
			}
			else
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef VSM\n#define VSM 1\n#endif\n" );

				if ( glConfig.hardwareType == GLHW_ATI )
				{
					Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef VSM_CLAMP\n#define VSM_CLAMP 1\n#endif\n" );
				}

				if ( ( glConfig.hardwareType == GLHW_NV_DX10 || glConfig.hardwareType == GLHW_ATI_DX10 ) && r_shadows->integer == SHADOWING_VSM32 )
				{
					Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef VSM_EPSILON\n#define VSM_EPSILON 0.000001\n#endif\n" );
				}
				else
				{
					Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef VSM_EPSILON\n#define VSM_EPSILON 0.0001\n#endif\n" );
				}

				if ( r_debugShadowMaps->integer )
				{
					Q_strcat( bufferExtra, sizeof( bufferExtra ),
					          va( "#ifndef DEBUG_VSM\n#define DEBUG_VSM %i\n#endif\n", r_debugShadowMaps->integer ) );
				}

				if ( r_lightBleedReduction->value )
				{
					Q_strcat( bufferExtra, sizeof( bufferExtra ),
					          va( "#ifndef r_LightBleedReduction\n#define r_LightBleedReduction %f\n#endif\n",
					              r_lightBleedReduction->value ) );
				}
			}

			if ( r_softShadows->integer == 1 )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef PCF_2X2\n#define PCF_2X2 1\n#endif\n" );
			}
			else if ( r_softShadows->integer == 2 )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef PCF_3X3\n#define PCF_3X3 1\n#endif\n" );
			}
			else if ( r_softShadows->integer == 3 )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef PCF_4X4\n#define PCF_4X4 1\n#endif\n" );
			}
			else if ( r_softShadows->integer == 4 )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef PCF_5X5\n#define PCF_5X5 1\n#endif\n" );
			}
			else if ( r_softShadows->integer == 5 )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef PCF_6X6\n#define PCF_6X6 1\n#endif\n" );
			}
			else if ( r_softShadows->integer == 6 )
			{
				Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef PCSS\n#define PCSS 1\n#endif\n" );
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

		if ( r_heatHazeFix->integer && glConfig2.framebufferBlitAvailable && glConfig.hardwareType != GLHW_ATI && glConfig.hardwareType != GLHW_ATI_DX10 && glConfig.driverType != GLDRV_MESA )
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

		/*
		   if(glConfig2.drawBuffersAvailable && glConfig2.maxDrawBuffers >= 4)
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
			Q_strcat( bufferExtra, sizeof( bufferExtra ), "#ifndef r_halfLambertLighting\n#define r_halfLambertLighting 1\n#endif\n" );
		}

		/*
		   if(glConfig2.textureFloatAvailable)
		   {
		   Q_strcat(bufferExtra, sizeof(bufferExtra), "#ifndef GL_ARB_texture_float\n#define GL_ARB_texture_float 1\n#endif\n");
		   }
		 */

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
			ri.Printf( PRINT_WARNING, "----------------------------------------------------------\n", filename );
			ri.Printf( PRINT_WARNING, "CONCATENATED shader '%s' ----------\n", filename );
			ri.Printf( PRINT_WARNING, " BEGIN ---------------------------------------------------\n", filename );

			for ( i = 0; i < sizeFinal; i += 1024 )
			{
				Q_strncpyz( msgPart, bufferFinal + i, sizeof( msgPart ) );
				ri.Printf( PRINT_ALL, "%s", msgPart );
			}

			ri.Printf( PRINT_WARNING, " END-- ---------------------------------------------------\n", filename );
		}
#endif

		glShaderSourceARB( shader, 1, ( const GLcharARB ** ) &bufferFinal, &sizeFinal );
		ri.Hunk_FreeTempMemory( bufferFinal );
	}

	// compile shader
	glCompileShaderARB( shader );

	GL_CheckErrors();

	// check if shader compiled
	glGetObjectParameterivARB( shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled );

	if ( !compiled )
	{
		GLSL_PrintShaderSource( shader );
		GLSL_PrintInfoLog( shader, qfalse );
		ri.FS_FreeFile( mainBuffer );
		free( libsBuffer );
		ri.Error( ERR_DROP, "Couldn't compile %s", filename );
	}

	GLSL_PrintInfoLog( shader, qtrue );
	//ri.Printf(PRINT_ALL, "%s\n", GLSL_PrintShaderSource(shader));

	// attach shader to program
	glAttachObjectARB( program, shader );
	GL_CheckErrors();

	// delete shader, no longer needed
	glDeleteObjectARB( shader );
	GL_CheckErrors();

	ri.FS_FreeFile( mainBuffer );
	free( libsBuffer );
}

static void GLSL_LinkProgram( GLhandleARB program )
{
	GLint linked;

	glLinkProgramARB( program );

	glGetObjectParameterivARB( program, GL_OBJECT_LINK_STATUS_ARB, &linked );

	if ( !linked )
	{
		GLSL_PrintInfoLog( program, qfalse );
		ri.Error( ERR_DROP, "shaders failed to link" );
	}
}

void GLSL_ValidateProgram( GLhandleARB program )
{
	GLint validated;

	glValidateProgramARB( program );

	glGetObjectParameterivARB( program, GL_OBJECT_VALIDATE_STATUS_ARB, &validated );

	if ( !validated )
	{
		GLSL_PrintInfoLog( program, qfalse );
		ri.Error( ERR_DROP, "shaders failed to validate" );
	}
}

void GLSL_ShowProgramUniforms( GLhandleARB program )
{
	int    i, count, size;
	GLenum type;
	char   uniformName[ 1000 ];

	// install the executables in the program object as part of current state.
	glUseProgramObjectARB( program );

	// check for GL Errors

	// query the number of active uniforms
	glGetObjectParameterivARB( program, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &count );

	// Loop over each of the active uniforms, and set their value
	for ( i = 0; i < count; i++ )
	{
		glGetActiveUniformARB( program, i, sizeof( uniformName ), NULL, &size, &type, uniformName );

		ri.Printf( PRINT_DEVELOPER, "active uniform: '%s'\n", uniformName );
	}

	glUseProgramObjectARB( 0 );
}

static void GLSL_BindAttribLocations( GLhandleARB program, uint32_t attribs )
{
	if ( attribs & ATTR_POSITION )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_POSITION, "attr_Position" );
	}

	if ( attribs & ATTR_TEXCOORD )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_TEXCOORD0, "attr_TexCoord0" );
	}

	if ( attribs & ATTR_LIGHTCOORD )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_TEXCOORD1, "attr_TexCoord1" );
	}

//  if(attribs & ATTR_TEXCOORD2)
//      glBindAttribLocationARB(program, ATTR_INDEX_TEXCOORD2, "attr_TexCoord2");

//  if(attribs & ATTR_TEXCOORD3)
//      glBindAttribLocationARB(program, ATTR_INDEX_TEXCOORD3, "attr_TexCoord3");

	if ( attribs & ATTR_TANGENT )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_TANGENT, "attr_Tangent" );
	}

	if ( attribs & ATTR_BINORMAL )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_BINORMAL, "attr_Binormal" );
	}

	if ( attribs & ATTR_NORMAL )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_NORMAL, "attr_Normal" );
	}

	if ( attribs & ATTR_COLOR )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_COLOR, "attr_Color" );
	}

#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )

	if ( attribs & ATTR_PAINTCOLOR )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_PAINTCOLOR, "attr_PaintColor" );
	}

	if ( attribs & ATTR_LIGHTDIRECTION )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_LIGHTDIRECTION, "attr_LightDirection" );
	}

#endif

	if ( glConfig2.vboVertexSkinningAvailable )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_BONE_INDEXES, "attr_BoneIndexes" );
		glBindAttribLocationARB( program, ATTR_INDEX_BONE_WEIGHTS, "attr_BoneWeights" );
	}

	if ( attribs & ATTR_POSITION2 )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_POSITION2, "attr_Position2" );
	}

	if ( attribs & ATTR_TANGENT2 )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_TANGENT2, "attr_Tangent2" );
	}

	if ( attribs & ATTR_BINORMAL2 )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_BINORMAL2, "attr_Binormal2" );
	}

	if ( attribs & ATTR_NORMAL2 )
	{
		glBindAttribLocationARB( program, ATTR_INDEX_NORMAL2, "attr_Normal2" );
	}
}

#if 0
static void GLSL_InitGPUShader( shaderProgram_t *program, const char *name, int attribs, qboolean fragmentShader, qboolean optimize )
{
	ri.Printf( PRINT_DEVELOPER, "------- GPU shader -------\n" );

	if ( strlen( name ) >= MAX_QPATH )
	{
		ri.Error( ERR_DROP, "GLSL_InitGPUShader: \"%s\" is too long", name );
	}

	Q_strncpyz( program->name, name, sizeof( program->name ) );

	program->program = glCreateProgramObjectARB();
	program->attribs = attribs;

	GLSL_LoadGPUShader( program->program, name, "", "", GL_VERTEX_SHADER_ARB, optimize );
	GLSL_LoadGPUShader( program->program, name, "", "", GL_FRAGMENT_SHADER_ARB, optimize );

	GLSL_BindAttribLocations( program->program, attribs );
	GLSL_LinkProgram( program->program );
}

#endif

/*
static void GLSL_InitGPUShader2(shaderProgram_t * program,
                                                                const char *vertexMainShader,
                                                                const char *fragmentMainShader,
                                                                const char *vertexLibShaders,
                                                                const char *fragmentLibShaders,
                                                                int attribs,
                                                                qboolean optimize)
{
        ri.Printf(PRINT_DEVELOPER, "------- GPU shader -------\n");

        if(strlen(vertexMainShader) >= MAX_QPATH)
        {
                ri.Error(ERR_DROP, "GLSL_InitGPUShader2: \"%s\" is too long", vertexMainShader);
        }

        if(strlen(fragmentMainShader) >= MAX_QPATH)
        {
                ri.Error(ERR_DROP, "GLSL_InitGPUShader2: \"%s\" is too long", fragmentMainShader);
        }

        Q_strncpyz(program->name, fragmentMainShader, sizeof(program->name));

        program->program = glCreateProgramObjectARB();
        program->attribs = attribs;

        GLSL_LoadGPUShader(program->program, vertexMainShader, vertexLibShaders, "", GL_VERTEX_SHADER_ARB, optimize);
        GLSL_LoadGPUShader(program->program, fragmentMainShader, fragmentLibShaders, "", GL_FRAGMENT_SHADER_ARB, optimize);

        GLSL_BindAttribLocations(program->program, attribs);
        GLSL_LinkProgram(program->program);
}
*/

/*
void GLSL_InitGPUShader3(shaderProgram_t * program,
                                                                const char *vertexMainShader,
                                                                const char *fragmentMainShader,
                                                                const char *vertexLibShaders,
                                                                const char *fragmentLibShaders,
                                                                const char *compileMacros,
                                                                int attribs,
                                                                qboolean optimize)
{
        int     len;

        ri.Printf(PRINT_DEVELOPER, "------- GPU shader -------\n");

        if(strlen(vertexMainShader) >= MAX_QPATH)
        {
                ri.Error(ERR_DROP, "GLSL_InitGPUShader3: \"%s\" is too long", vertexMainShader);
        }

        if(strlen(fragmentMainShader) >= MAX_QPATH)
        {
                ri.Error(ERR_DROP, "GLSL_InitGPUShader3: \"%s\" is too long", fragmentMainShader);
        }

        Q_strncpyz(program->name, fragmentMainShader, sizeof(program->name));

        len = strlen(compileMacros) + 1;
        program->compileMacros = (char *) ri.Hunk_Alloc(sizeof(char) * len, h_low);
        Q_strncpyz(program->compileMacros, compileMacros, len);

        program->program = glCreateProgramObjectARB();
        program->attribs = attribs;

        GLSL_LoadGPUShader(program->program, vertexMainShader, vertexLibShaders, compileMacros, GL_VERTEX_SHADER_ARB, optimize);
        GLSL_LoadGPUShader(program->program, fragmentMainShader, fragmentLibShaders, compileMacros, GL_FRAGMENT_SHADER_ARB, optimize);

        GLSL_BindAttribLocations(program->program, attribs);
        GLSL_LinkProgram(program->program);
}
*/

void GLSL_InitGPUShaders( void )
{
	int startTime, endTime;

	ri.Printf( PRINT_ALL, "------- GLSL_InitGPUShaders -------\n" );

	// make sure the render thread is stopped
	R_SyncRenderThread();

	GL_CheckErrors();

#if defined( USE_GLSL_OPTIMIZER )
	s_glslOptimizer = glslopt_initialize( false );
#endif

	startTime = ri.Milliseconds();

	// single texture rendering
	gl_genericShader = new GLShader_generic();

	// simple vertex color shading for entities
	gl_vertexLightingShader_DBS_entity = new GLShader_vertexLighting_DBS_entity();

	// simple vertex color shading for the world
	gl_vertexLightingShader_DBS_world = new GLShader_vertexLighting_DBS_world();

	// standard light mapping
	gl_lightMappingShader = new GLShader_lightMapping();

	// geometric-buffer fill rendering with diffuse + bump + specular
	if ( DS_STANDARD_ENABLED() )
	{
		// G-Buffer construction
		gl_geometricFillShader = new GLShader_geometricFill();

		// deferred omni-directional lighting post process effect
		gl_deferredLightingShader_omniXYZ = new GLShader_deferredLighting_omniXYZ();

		gl_deferredLightingShader_projXYZ = new GLShader_deferredLighting_projXYZ();

		gl_deferredLightingShader_directionalSun = new GLShader_deferredLighting_directionalSun();
	}
	else
	{
		// omni-directional specular bump mapping ( Doom3 style )
		gl_forwardLightingShader_omniXYZ = new GLShader_forwardLighting_omniXYZ();

		// projective lighting ( Doom3 style )
		gl_forwardLightingShader_projXYZ = new GLShader_forwardLighting_projXYZ();

		// directional sun lighting ( Doom3 style )
		gl_forwardLightingShader_directionalSun = new GLShader_forwardLighting_directionalSun();
	}

#if !defined( GLSL_COMPILE_STARTUP_ONLY )

	// depth to color encoding
	GLSL_InitGPUShader( &tr.depthToColorShader, "depthToColor", ATTR_POSITION, qtrue, qtrue );

	tr.depthToColorShader.u_ModelViewProjectionMatrix =
	  glGetUniformLocationARB( tr.depthToColorShader.program, "u_ModelViewProjectionMatrix" );

	if ( glConfig2.vboVertexSkinningAvailable )
	{
		tr.depthToColorShader.u_VertexSkinning = glGetUniformLocationARB( tr.depthToColorShader.program, "u_VertexSkinning" );
		tr.depthToColorShader.u_BoneMatrix = glGetUniformLocationARB( tr.depthToColorShader.program, "u_BoneMatrix" );
	}

	glUseProgramObjectARB( tr.depthToColorShader.program );
	//glUniform1iARB(tr.depthToColorShader.u_ColorMap, 0);
	glUseProgramObjectARB( 0 );

	GLSL_ValidateProgram( tr.depthToColorShader.program );
	GLSL_ShowProgramUniforms( tr.depthToColorShader.program );
	GL_CheckErrors();

#endif // #if !defined(GLSL_COMPILE_STARTUP_ONLY)

	// shadowmap distance compression
	gl_shadowFillShader = new GLShader_shadowFill();

#if !defined( GLSL_COMPILE_STARTUP_ONLY )

#ifdef VOLUMETRIC_LIGHTING
	// volumetric lighting
	GLSL_InitGPUShader( &tr.lightVolumeShader_omni, "lightVolume_omni", ATTR_POSITION, qtrue );

	tr.lightVolumeShader_omni.u_DepthMap =
	  glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_DepthMap" );
	tr.lightVolumeShader_omni.u_AttenuationMapXY =
	  glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_AttenuationMapXY" );
	tr.lightVolumeShader_omni.u_AttenuationMapZ =
	  glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_AttenuationMapZ" );
	tr.lightVolumeShader_omni.u_ShadowMap = glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_ShadowMap" );
	tr.lightVolumeShader_omni.u_ViewOrigin = glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_ViewOrigin" );
	tr.lightVolumeShader_omni.u_LightOrigin = glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_LightOrigin" );
	tr.lightVolumeShader_omni.u_LightColor = glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_LightColor" );
	tr.lightVolumeShader_omni.u_LightRadius = glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_LightRadius" );
	tr.lightVolumeShader_omni.u_LightScale = glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_LightScale" );
	tr.lightVolumeShader_omni.u_LightAttenuationMatrix =
	  glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_LightAttenuationMatrix" );
	tr.lightVolumeShader_omni.u_ShadowCompare = glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_ShadowCompare" );
	tr.lightVolumeShader_omni.u_ModelViewProjectionMatrix =
	  glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_ModelViewProjectionMatrix" );
	tr.lightVolumeShader_omni.u_UnprojectMatrix = glGetUniformLocationARB( tr.lightVolumeShader_omni.program, "u_UnprojectMatrix" );

	glUseProgramObjectARB( tr.lightVolumeShader_omni.program );
	glUniform1iARB( tr.lightVolumeShader_omni.u_DepthMap, 0 );
	glUniform1iARB( tr.lightVolumeShader_omni.u_AttenuationMapXY, 1 );
	glUniform1iARB( tr.lightVolumeShader_omni.u_AttenuationMapZ, 2 );
	glUniform1iARB( tr.lightVolumeShader_omni.u_ShadowMap, 3 );
	glUseProgramObjectARB( 0 );

	GLSL_ValidateProgram( tr.lightVolumeShader_omni.program );
	GLSL_ShowProgramUniforms( tr.lightVolumeShader_omni.program );
	GL_CheckErrors();
#endif

	// UT3 style player shadowing
	GLSL_InitGPUShader( &tr.deferredShadowingShader_proj, "deferredShadowing_proj", ATTR_POSITION, qtrue, qtrue );

	tr.deferredShadowingShader_proj.u_DepthMap =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_DepthMap" );
	tr.deferredShadowingShader_proj.u_AttenuationMapXY =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_AttenuationMapXY" );
	tr.deferredShadowingShader_proj.u_AttenuationMapZ =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_AttenuationMapZ" );
	tr.deferredShadowingShader_proj.u_ShadowMap =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_ShadowMap" );
	tr.deferredShadowingShader_proj.u_LightOrigin =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_LightOrigin" );
	tr.deferredShadowingShader_proj.u_LightColor =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_LightColor" );
	tr.deferredShadowingShader_proj.u_LightRadius =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_LightRadius" );
	tr.deferredShadowingShader_proj.u_LightAttenuationMatrix =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_LightAttenuationMatrix" );
	tr.deferredShadowingShader_proj.u_ShadowMatrix =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_ShadowMatrix" );
	tr.deferredShadowingShader_proj.u_ShadowCompare =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_ShadowCompare" );
	tr.deferredShadowingShader_proj.u_PortalClipping =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_PortalClipping" );
	tr.deferredShadowingShader_proj.u_PortalPlane =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_PortalPlane" );
	tr.deferredShadowingShader_proj.u_ModelViewProjectionMatrix =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_ModelViewProjectionMatrix" );
	tr.deferredShadowingShader_proj.u_UnprojectMatrix =
	  glGetUniformLocationARB( tr.deferredShadowingShader_proj.program, "u_UnprojectMatrix" );

	glUseProgramObjectARB( tr.deferredShadowingShader_proj.program );
	glUniform1iARB( tr.deferredShadowingShader_proj.u_DepthMap, 0 );
	glUniform1iARB( tr.deferredShadowingShader_proj.u_AttenuationMapXY, 1 );
	glUniform1iARB( tr.deferredShadowingShader_proj.u_AttenuationMapZ, 2 );
	glUniform1iARB( tr.deferredShadowingShader_proj.u_ShadowMap, 3 );
	glUseProgramObjectARB( 0 );

	GLSL_ValidateProgram( tr.deferredShadowingShader_proj.program );
	GLSL_ShowProgramUniforms( tr.deferredShadowingShader_proj.program );
	GL_CheckErrors();

#endif // #if !defined(GLSL_COMPILE_STARTUP_ONLY)

	// bumped cubemap reflection for abitrary polygons ( EMBM )
	gl_reflectionShader = new GLShader_reflection();

	// skybox drawing for abitrary polygons
	gl_skyboxShader = new GLShader_skybox();

	// Q3A volumetric fog
	gl_fogQuake3Shader = new GLShader_fogQuake3();

	// global fog post process effect
	gl_fogGlobalShader = new GLShader_fogGlobal();

	// heatHaze post process effect
	gl_heatHazeShader = new GLShader_heatHaze();

	// screen post process effect
	gl_screenShader = new GLShader_screen();

	// portal process effect
	gl_portalShader = new GLShader_portal();

	// HDR -> LDR tone mapping
	gl_toneMappingShader = new GLShader_toneMapping();

	// LDR bright pass filter
	gl_contrastShader = new GLShader_contrast();

	// camera post process effect
	gl_cameraEffectsShader = new GLShader_cameraEffects();

	// gaussian blur
	gl_blurXShader = new GLShader_blurX();

	gl_blurYShader = new GLShader_blurY();

	// debug utils
	gl_debugShadowMapShader = new GLShader_debugShadowMap();

#if !defined( GLSL_COMPILE_STARTUP_ONLY )

	// liquid post process effect
	GLSL_InitGPUShader( &tr.liquidShader, "liquid",
	                    ATTR_POSITION | ATTR_TEXCOORD | ATTR_TANGENT | ATTR_BINORMAL | ATTR_NORMAL | ATTR_COLOR

#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )
	                    | ATTR_LIGHTDIRECTION
#endif
	                    , qtrue, qtrue );

	tr.liquidShader.u_CurrentMap = glGetUniformLocationARB( tr.liquidShader.program, "u_CurrentMap" );
	tr.liquidShader.u_PortalMap = glGetUniformLocationARB( tr.liquidShader.program, "u_PortalMap" );
	tr.liquidShader.u_DepthMap = glGetUniformLocationARB( tr.liquidShader.program, "u_DepthMap" );
	tr.liquidShader.u_NormalMap = glGetUniformLocationARB( tr.liquidShader.program, "u_NormalMap" );
	tr.liquidShader.u_NormalTextureMatrix = glGetUniformLocationARB( tr.liquidShader.program, "u_NormalTextureMatrix" );
	tr.liquidShader.u_ViewOrigin = glGetUniformLocationARB( tr.liquidShader.program, "u_ViewOrigin" );
	tr.liquidShader.u_RefractionIndex = glGetUniformLocationARB( tr.liquidShader.program, "u_RefractionIndex" );
	tr.liquidShader.u_FresnelPower = glGetUniformLocationARB( tr.liquidShader.program, "u_FresnelPower" );
	tr.liquidShader.u_FresnelScale = glGetUniformLocationARB( tr.liquidShader.program, "u_FresnelScale" );
	tr.liquidShader.u_FresnelBias = glGetUniformLocationARB( tr.liquidShader.program, "u_FresnelBias" );
	tr.liquidShader.u_NormalScale = glGetUniformLocationARB( tr.liquidShader.program, "u_NormalScale" );
	tr.liquidShader.u_FogDensity = glGetUniformLocationARB( tr.liquidShader.program, "u_FogDensity" );
	tr.liquidShader.u_FogColor = glGetUniformLocationARB( tr.liquidShader.program, "u_FogColor" );
	tr.liquidShader.u_ModelMatrix = glGetUniformLocationARB( tr.liquidShader.program, "u_ModelMatrix" );
	tr.liquidShader.u_ModelViewProjectionMatrix =
	  glGetUniformLocationARB( tr.liquidShader.program, "u_ModelViewProjectionMatrix" );
	tr.liquidShader.u_UnprojectMatrix = glGetUniformLocationARB( tr.liquidShader.program, "u_UnprojectMatrix" );

	glUseProgramObjectARB( tr.liquidShader.program );
	glUniform1iARB( tr.liquidShader.u_CurrentMap, 0 );
	glUniform1iARB( tr.liquidShader.u_PortalMap, 1 );
	glUniform1iARB( tr.liquidShader.u_DepthMap, 2 );
	glUniform1iARB( tr.liquidShader.u_NormalMap, 3 );
	glUseProgramObjectARB( 0 );

	GLSL_ValidateProgram( tr.liquidShader.program );
	GLSL_ShowProgramUniforms( tr.liquidShader.program );
	GL_CheckErrors();

	// volumetric fog post process effect
	GLSL_InitGPUShader( &tr.volumetricFogShader, "volumetricFog", ATTR_POSITION, qtrue, qtrue );

	tr.volumetricFogShader.u_DepthMap = glGetUniformLocationARB( tr.volumetricFogShader.program, "u_DepthMap" );
	tr.volumetricFogShader.u_DepthMapBack = glGetUniformLocationARB( tr.volumetricFogShader.program, "u_DepthMapBack" );
	tr.volumetricFogShader.u_DepthMapFront = glGetUniformLocationARB( tr.volumetricFogShader.program, "u_DepthMapFront" );
	tr.volumetricFogShader.u_ViewOrigin = glGetUniformLocationARB( tr.volumetricFogShader.program, "u_ViewOrigin" );
	tr.volumetricFogShader.u_FogDensity = glGetUniformLocationARB( tr.volumetricFogShader.program, "u_FogDensity" );
	tr.volumetricFogShader.u_FogColor = glGetUniformLocationARB( tr.volumetricFogShader.program, "u_FogColor" );
	tr.volumetricFogShader.u_UnprojectMatrix = glGetUniformLocationARB( tr.volumetricFogShader.program, "u_UnprojectMatrix" );
	tr.volumetricFogShader.u_ModelViewProjectionMatrix =
	  glGetUniformLocationARB( tr.volumetricFogShader.program, "u_ModelViewProjectionMatrix" );

	glUseProgramObjectARB( tr.volumetricFogShader.program );
	glUniform1iARB( tr.volumetricFogShader.u_DepthMap, 0 );
	glUniform1iARB( tr.volumetricFogShader.u_DepthMapBack, 1 );
	glUniform1iARB( tr.volumetricFogShader.u_DepthMapFront, 2 );
	glUseProgramObjectARB( 0 );

	GLSL_ValidateProgram( tr.volumetricFogShader.program );
	GLSL_ShowProgramUniforms( tr.volumetricFogShader.program );
	GL_CheckErrors();

#ifdef EXPERIMENTAL
	// screen space ambien occlusion post process effect
	GLSL_InitGPUShader( &tr.screenSpaceAmbientOcclusionShader, "screenSpaceAmbientOcclusion", ATTR_POSITION, qtrue, qtrue );

	tr.screenSpaceAmbientOcclusionShader.u_CurrentMap =
	  glGetUniformLocationARB( tr.screenSpaceAmbientOcclusionShader.program, "u_CurrentMap" );
	tr.screenSpaceAmbientOcclusionShader.u_DepthMap =
	  glGetUniformLocationARB( tr.screenSpaceAmbientOcclusionShader.program, "u_DepthMap" );
	tr.screenSpaceAmbientOcclusionShader.u_ModelViewProjectionMatrix =
	  glGetUniformLocationARB( tr.screenSpaceAmbientOcclusionShader.program, "u_ModelViewProjectionMatrix" );
	//tr.screenSpaceAmbientOcclusionShader.u_ViewOrigin = glGetUniformLocationARB(tr.screenSpaceAmbientOcclusionShader.program, "u_ViewOrigin");
	//tr.screenSpaceAmbientOcclusionShader.u_SSAOJitter = glGetUniformLocationARB(tr.screenSpaceAmbientOcclusionShader.program, "u_SSAOJitter");
	//tr.screenSpaceAmbientOcclusionShader.u_SSAORadius = glGetUniformLocationARB(tr.screenSpaceAmbientOcclusionShader.program, "u_SSAORadius");
	//tr.screenSpaceAmbientOcclusionShader.u_UnprojectMatrix = glGetUniformLocationARB(tr.screenSpaceAmbientOcclusionShader.program, "u_UnprojectMatrix");
	//tr.screenSpaceAmbientOcclusionShader.u_ProjectMatrix = glGetUniformLocationARB(tr.screenSpaceAmbientOcclusionShader.program, "u_ProjectMatrix");

	glUseProgramObjectARB( tr.screenSpaceAmbientOcclusionShader.program );
	glUniform1iARB( tr.screenSpaceAmbientOcclusionShader.u_CurrentMap, 0 );
	glUniform1iARB( tr.screenSpaceAmbientOcclusionShader.u_DepthMap, 1 );
	glUseProgramObjectARB( 0 );

	GLSL_ValidateProgram( tr.screenSpaceAmbientOcclusionShader.program );
	GLSL_ShowProgramUniforms( tr.screenSpaceAmbientOcclusionShader.program );
	GL_CheckErrors();
#endif
#ifdef EXPERIMENTAL
	// depth of field post process effect
	GLSL_InitGPUShader( &tr.depthOfFieldShader, "depthOfField", ATTR_POSITION, qtrue, qtrue );

	tr.depthOfFieldShader.u_CurrentMap = glGetUniformLocationARB( tr.depthOfFieldShader.program, "u_CurrentMap" );
	tr.depthOfFieldShader.u_DepthMap = glGetUniformLocationARB( tr.depthOfFieldShader.program, "u_DepthMap" );
	tr.depthOfFieldShader.u_ModelViewProjectionMatrix =
	  glGetUniformLocationARB( tr.depthOfFieldShader.program, "u_ModelViewProjectionMatrix" );

	glUseProgramObjectARB( tr.depthOfFieldShader.program );
	glUniform1iARB( tr.depthOfFieldShader.u_CurrentMap, 0 );
	glUniform1iARB( tr.depthOfFieldShader.u_DepthMap, 1 );
	glUseProgramObjectARB( 0 );

	GLSL_ValidateProgram( tr.depthOfFieldShader.program );
	GLSL_ShowProgramUniforms( tr.depthOfFieldShader.program );
	GL_CheckErrors();
#endif

#endif // #if !defined(GLSL_COMPILE_STARTUP_ONLY)

	endTime = ri.Milliseconds();

#if defined( USE_GLSL_OPTIMIZER )
	glslopt_cleanup( s_glslOptimizer );
#endif

	ri.Printf( PRINT_ALL, "GLSL shaders load time = %5.2f seconds\n", ( endTime - startTime ) / 1000.0 );

	if( r_recompileShaders->integer )
	{
		ri.Cvar_Set( "r_recompileShaders", "0" );
	}
}

void GLSL_ShutdownGPUShaders( void )
{
//	int        i;

	ri.Printf( PRINT_ALL, "------- GLSL_ShutdownGPUShaders -------\n" );

	if ( gl_genericShader )
	{
		delete gl_genericShader;
		gl_genericShader = NULL;
	}

	if ( gl_vertexLightingShader_DBS_entity )
	{
		delete gl_vertexLightingShader_DBS_entity;
		gl_vertexLightingShader_DBS_entity = NULL;
	}

	if ( gl_vertexLightingShader_DBS_world )
	{
		delete gl_vertexLightingShader_DBS_world;
		gl_vertexLightingShader_DBS_world = NULL;
	}

	if ( gl_lightMappingShader )
	{
		delete gl_lightMappingShader;
		gl_lightMappingShader = NULL;
	}

	if ( gl_geometricFillShader )
	{
		delete gl_geometricFillShader;
		gl_geometricFillShader = NULL;
	}

	if ( gl_deferredLightingShader_omniXYZ )
	{
		delete gl_deferredLightingShader_omniXYZ;
		gl_deferredLightingShader_omniXYZ = NULL;
	}

	if( gl_deferredLightingShader_projXYZ ) 
	{
		delete gl_deferredLightingShader_projXYZ;
		gl_deferredLightingShader_projXYZ = NULL;
	}

	if( gl_deferredLightingShader_directionalSun )
	{
		delete gl_deferredLightingShader_directionalSun;
		gl_deferredLightingShader_directionalSun = NULL;
	}

#if !defined( GLSL_COMPILE_STARTUP_ONLY )

	if ( tr.depthToColorShader.program )
	{
		glDeleteObjectARB( tr.depthToColorShader.program );
		Com_Memset( &tr.depthToColorShader, 0, sizeof( shaderProgram_t ) );
	}

#endif // #if !defined(GLSL_COMPILE_STARTUP_ONLY)

	if ( gl_shadowFillShader )
	{
		delete gl_shadowFillShader;
		gl_shadowFillShader = NULL;
	}

	if ( gl_forwardLightingShader_omniXYZ )
	{
		delete gl_forwardLightingShader_omniXYZ;
		gl_forwardLightingShader_omniXYZ = NULL;
	}

	if ( gl_forwardLightingShader_projXYZ )
	{
		delete gl_forwardLightingShader_projXYZ;
		gl_forwardLightingShader_projXYZ = NULL;
	}

	if ( gl_forwardLightingShader_directionalSun )
	{
		delete gl_forwardLightingShader_directionalSun;
		gl_forwardLightingShader_directionalSun = NULL;
	}

#if !defined( GLSL_COMPILE_STARTUP_ONLY )

#ifdef VOLUMETRIC_LIGHTING

	if ( tr.lightVolumeShader_omni.program )
	{
		glDeleteObjectARB( tr.lightVolumeShader_omni.program );
		Com_Memset( &tr.lightVolumeShader_omni, 0, sizeof( shaderProgram_t ) );
	}

#endif

	if ( tr.deferredShadowingShader_proj.program )
	{
		glDeleteObjectARB( tr.deferredShadowingShader_proj.program );
		Com_Memset( &tr.deferredShadowingShader_proj, 0, sizeof( shaderProgram_t ) );
	}

#endif // #if !defined(GLSL_COMPILE_STARTUP_ONLY)

	if ( gl_reflectionShader )
	{
		delete gl_reflectionShader;
		gl_reflectionShader = NULL;
	}

	if ( gl_skyboxShader )
	{
		delete gl_skyboxShader;
		gl_skyboxShader = NULL;
	}

	if ( gl_fogQuake3Shader )
	{
		delete gl_fogQuake3Shader;
		gl_fogQuake3Shader = NULL;
	}

	if ( gl_fogGlobalShader )
	{
		delete gl_fogGlobalShader;
		gl_fogGlobalShader = NULL;
	}

	if ( gl_heatHazeShader )
	{
		delete gl_heatHazeShader;
		gl_heatHazeShader = NULL;
	}

	if ( gl_screenShader )
	{
		delete gl_screenShader;
		gl_screenShader = NULL;
	}

	if ( gl_portalShader )
	{
		delete gl_portalShader;
		gl_portalShader = NULL;
	}

	if ( gl_toneMappingShader )
	{
		delete gl_toneMappingShader;
		gl_toneMappingShader = NULL;
	}

	if ( gl_contrastShader )
	{
		delete gl_contrastShader;
		gl_contrastShader = NULL;
	}

	if ( gl_cameraEffectsShader )
	{
		delete gl_cameraEffectsShader;
		gl_cameraEffectsShader = NULL;
	}

	if ( gl_blurXShader )
	{
		delete gl_blurXShader;
		gl_blurXShader = NULL;
	}

	if ( gl_blurYShader )
	{
		delete gl_blurYShader;
		gl_blurYShader = NULL;
	}

	if ( gl_debugShadowMapShader )
	{
		delete gl_debugShadowMapShader;
		gl_debugShadowMapShader = NULL;
	}

#if !defined( GLSL_COMPILE_STARTUP_ONLY )

	if ( tr.liquidShader.program )
	{
		glDeleteObjectARB( tr.liquidShader.program );
		Com_Memset( &tr.liquidShader, 0, sizeof( shaderProgram_t ) );
	}

	if ( tr.volumetricFogShader.program )
	{
		glDeleteObjectARB( tr.volumetricFogShader.program );
		Com_Memset( &tr.volumetricFogShader, 0, sizeof( shaderProgram_t ) );
	}

#ifdef EXPERIMENTAL

	if ( tr.screenSpaceAmbientOcclusionShader.program )
	{
		glDeleteObjectARB( tr.screenSpaceAmbientOcclusionShader.program );
		Com_Memset( &tr.screenSpaceAmbientOcclusionShader, 0, sizeof( shaderProgram_t ) );
	}

#endif
#ifdef EXPERIMENTAL

	if ( tr.depthOfFieldShader.program )
	{
		glDeleteObjectARB( tr.depthOfFieldShader.program );
		Com_Memset( &tr.depthOfFieldShader, 0, sizeof( shaderProgram_t ) );
	}

#endif

#endif // #if !defined(GLSL_COMPILE_STARTUP_ONLY)

	glState.currentProgram = 0;

	if ( glUseProgramObjectARB != NULL )
	{
		glUseProgramObjectARB( 0 );
	}
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

	gl_genericShader->DisableAlphaTesting();
	gl_genericShader->SetPortalClipping( backEnd.viewParms.isPortal );

	gl_genericShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_genericShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_genericShader->DisableDeformVertexes();
	gl_genericShader->DisableTCGenEnvironment();
	gl_genericShader->DisableTCGenLightmap();

	gl_genericShader->BindProgram();
	gl_genericShader->SetRequiredVertexPointers();

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );

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
		gl_genericShader->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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
void Tess_Begin( void ( *stageIteratorFunc )(),
                 void ( *stageIteratorFunc2 )(),
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
	gl_genericShader->SetAlphaTesting( ( pStage->stateBits & GLS_ATEST_BITS ) != 0 );
	gl_genericShader->SetPortalClipping( backEnd.viewParms.isPortal );

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
		gl_genericShader->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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

	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_genericShader->SetUniform_PortalPlane( plane );
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
	gl_vertexLightingShader_DBS_entity->SetPortalClipping( backEnd.viewParms.isPortal );
	gl_vertexLightingShader_DBS_entity->SetAlphaTesting( ( pStage->stateBits & GLS_ATEST_BITS ) != 0 );

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
		gl_vertexLightingShader_DBS_entity->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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

	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_vertexLightingShader_DBS_entity->SetUniform_PortalPlane( plane );
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

	bool normalMapping = tr.worldDeluxeMapping && r_normalMapping->integer && ( pStage->bundle[ TB_NORMALMAP ].image[ 0 ] != NULL );

	// choose right shader program ----------------------------------
	gl_vertexLightingShader_DBS_world->SetPortalClipping( backEnd.viewParms.isPortal );
	gl_vertexLightingShader_DBS_world->SetAlphaTesting( ( pStage->stateBits & GLS_ATEST_BITS ) != 0 );

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

	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_vertexLightingShader_DBS_world->SetUniform_PortalPlane( plane );
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

	gl_lightMappingShader->SetPortalClipping( backEnd.viewParms.isPortal );
	gl_lightMappingShader->SetAlphaTesting( ( pStage->stateBits & GLS_ATEST_BITS ) != 0 );

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

	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_lightMappingShader->SetUniform_PortalPlane( plane );
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
	gl_geometricFillShader->SetPortalClipping( backEnd.viewParms.isPortal );
	gl_geometricFillShader->SetAlphaTesting( ( pStage->stateBits & GLS_ATEST_BITS ) != 0 );

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
		gl_geometricFillShader->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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

	// u_PortalPlane
	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_geometricFillShader->SetUniform_PortalPlane( plane );
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

	gl_genericShader->SetAlphaTesting( ( pStage->stateBits & GLS_ATEST_BITS ) != 0 );
	gl_genericShader->SetPortalClipping( backEnd.viewParms.isPortal );

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
		gl_genericShader->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
	}

	// u_DeformGen
	if ( tess.surfaceShader->numDeforms )
	{
		gl_genericShader->SetUniform_DeformParms( tess.surfaceShader->deforms, tess.surfaceShader->numDeforms );
		gl_genericShader->SetUniform_Time( backEnd.refdef.floatTime );
	}

	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_genericShader->SetUniform_PortalPlane( plane );
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

	gl_shadowFillShader->SetAlphaTesting( ( pStage->stateBits & GLS_ATEST_BITS ) != 0 );
	gl_shadowFillShader->SetPortalClipping( backEnd.viewParms.isPortal );

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
		gl_shadowFillShader->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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

	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_shadowFillShader->SetUniform_PortalPlane( plane );
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
	gl_forwardLightingShader_omniXYZ->SetPortalClipping( backEnd.viewParms.isPortal );
	gl_forwardLightingShader_omniXYZ->SetAlphaTesting( ( diffuseStage->stateBits & GLS_ATEST_BITS ) != 0 );

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
		gl_forwardLightingShader_omniXYZ->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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

	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_forwardLightingShader_omniXYZ->SetUniform_PortalPlane( plane );
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
	gl_forwardLightingShader_projXYZ->SetPortalClipping( backEnd.viewParms.isPortal );
	gl_forwardLightingShader_projXYZ->SetAlphaTesting( ( diffuseStage->stateBits & GLS_ATEST_BITS ) != 0 );

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
		gl_forwardLightingShader_projXYZ->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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

	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_forwardLightingShader_projXYZ->SetUniform_PortalPlane( plane );
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
	gl_forwardLightingShader_directionalSun->SetPortalClipping( backEnd.viewParms.isPortal );
	gl_forwardLightingShader_directionalSun->SetAlphaTesting( ( diffuseStage->stateBits & GLS_ATEST_BITS ) != 0 );

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
		gl_forwardLightingShader_directionalSun->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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

	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_forwardLightingShader_directionalSun->SetUniform_PortalPlane( plane );
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
	gl_reflectionShader->SetPortalClipping( backEnd.viewParms.isPortal );
//	gl_reflectionShader->SetAlphaTesting((pStage->stateBits & GLS_ATEST_BITS) != 0);

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
		gl_reflectionShader->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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
	glUniform3fARB( tr.dispersionShader_C.u_EtaRatio, eta, eta + etaDelta, eta + ( etaDelta * 2 ) );
	glUniform1fARB( tr.dispersionShader_C.u_FresnelPower, RB_EvalExpression( &pStage->fresnelPowerExp, 2.0f ) );
	glUniform1fARB( tr.dispersionShader_C.u_FresnelScale, RB_EvalExpression( &pStage->fresnelScaleExp, 2.0f ) );
	glUniform1fARB( tr.dispersionShader_C.u_FresnelBias, RB_EvalExpression( &pStage->fresnelBiasExp, 1.0f ) );

	GLSL_SetUniform_ModelMatrix( &tr.dispersionShader_C, backEnd.orientation.transformMatrix );
	GLSL_SetUniform_ModelViewProjectionMatrix( &tr.dispersionShader_C, glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	if ( glConfig2.vboVertexSkinningAvailable )
	{
		GLSL_SetUniform_VertexSkinning( &tr.dispersionShader_C, tess.vboVertexSkinning );

		if ( tess.vboVertexSkinning )
		{
			glUniformMatrix4fvARB( tr.dispersionShader_C.u_BoneMatrix, MAX_BONES, GL_FALSE, &tess.boneMatrices[ 0 ][ 0 ] );
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

	gl_skyboxShader->SetPortalClipping( backEnd.viewParms.isPortal );
	gl_skyboxShader->BindProgram();

	gl_skyboxShader->SetUniform_ViewOrigin( backEnd.viewParms.orientation.origin );  // in world space

	gl_skyboxShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
	gl_skyboxShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_PortalPlane
	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_skyboxShader->SetUniform_PortalPlane( plane );
	}

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
		glVertexAttrib4fvARB( ATTR_INDEX_COLOR, tess.svars.color );
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
		glVertexAttrib4fvARB( ATTR_INDEX_COLOR, tess.svars.color );
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
		//gl_genericShader->SetAlphaTesting((pStage->stateBits & GLS_ATEST_BITS) != 0);
		gl_genericShader->SetAlphaTesting( false );
		gl_genericShader->SetPortalClipping( backEnd.viewParms.isPortal );

		gl_genericShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
		gl_genericShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

		gl_genericShader->SetDeformVertexes( tess.surfaceShader->numDeforms );
		gl_genericShader->SetTCGenEnvironment( false );

		gl_genericShader->BindProgram();
		// end choose right shader program ------------------------------

		// u_ColorModulate
		gl_genericShader->SetUniform_ColorModulate( CGEN_CONST, AGEN_CONST );

		// u_Color
		gl_genericShader->SetUniform_Color( colorRed );

		gl_genericShader->SetUniform_ModelMatrix( backEnd.orientation.transformMatrix );
		gl_genericShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		// u_BoneMatrix
		if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
		{
			gl_genericShader->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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

		if ( backEnd.viewParms.isPortal )
		{
			float plane[ 4 ];

			// clipping plane in world space
			plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
			plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
			plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
			plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

			gl_genericShader->SetUniform_PortalPlane( plane );
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
	gl_heatHazeShader->SetPortalClipping( backEnd.viewParms.isPortal );
	//gl_heatHazeShader->SetAlphaTesting((pStage->stateBits & GLS_ATEST_BITS) != 0);

	gl_heatHazeShader->SetVertexSkinning( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning );
	gl_heatHazeShader->SetVertexAnimation( glState.vertexAttribsInterpolation > 0 );

	gl_heatHazeShader->SetDeformVertexes( tess.surfaceShader->numDeforms );

	gl_heatHazeShader->BindProgram();

	// end choose right shader program ------------------------------

	// set uniforms
	//GLSL_SetUniform_AlphaTest(&tr.heatHazeShader, pStage->stateBits);

	deformMagnitude = RB_EvalExpression( &pStage->deformMagnitudeExp, 1.0 );
	gl_heatHazeShader->SetUniform_DeformMagnitude( deformMagnitude );

	gl_heatHazeShader->SetUniform_ModelViewMatrixTranspose( glState.modelViewMatrix[ glState.stackIndex ] );
	gl_heatHazeShader->SetUniform_ProjectionMatrixTranspose( glState.projectionMatrix[ glState.stackIndex ] );
	gl_heatHazeShader->SetUniform_ModelViewProjectionMatrix( glState.modelViewProjectionMatrix[ glState.stackIndex ] );

	// u_BoneMatrix
	if ( glConfig2.vboVertexSkinningAvailable && tess.vboVertexSkinning )
	{
		gl_heatHazeShader->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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
	vec3_t        fogColor;
	shaderStage_t *pStage = tess.surfaceStages[ stage ];

	GLimp_LogComment( "--- Render_liquid ---\n" );

	// Tr3B: don't allow blend effects
	GL_State( pStage->stateBits & ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS | GLS_DEPTHMASK_TRUE ) );

	// enable shader, set arrays
	GL_BindProgram( &tr.liquidShader );
	GL_VertexAttribsState( tr.liquidShader.attribs );

	// set uniforms
	VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );  // in world space

	fogDensity = RB_EvalExpression( &pStage->fogDensityExp, 0.001 );
	VectorCopy( tess.svars.color, fogColor );

	GLSL_SetUniform_ViewOrigin( &tr.liquidShader, viewOrigin );
	GLSL_SetUniform_RefractionIndex( &tr.liquidShader, RB_EvalExpression( &pStage->refractionIndexExp, 1.0 ) );
	glUniform1fARB( tr.liquidShader.u_FresnelPower, RB_EvalExpression( &pStage->fresnelPowerExp, 2.0 ) );
	glUniform1fARB( tr.liquidShader.u_FresnelScale, RB_EvalExpression( &pStage->fresnelScaleExp, 1.0 ) );
	glUniform1fARB( tr.liquidShader.u_FresnelBias, RB_EvalExpression( &pStage->fresnelBiasExp, 0.05 ) );
	glUniform1fARB( tr.liquidShader.u_NormalScale, RB_EvalExpression( &pStage->normalScaleExp, 0.05 ) );
	glUniform1fARB( tr.liquidShader.u_FogDensity, fogDensity );
	glUniform3fARB( tr.liquidShader.u_FogColor, fogColor[ 0 ], fogColor[ 1 ], fogColor[ 2 ] );

	GLSL_SetUniform_UnprojectMatrix( &tr.liquidShader, backEnd.viewParms.unprojectionMatrix );
	GLSL_SetUniform_ModelMatrix( &tr.liquidShader, backEnd.orientation.transformMatrix );
	GLSL_SetUniform_ModelViewProjectionMatrix( &tr.liquidShader, glState.modelViewProjectionMatrix[ glState.stackIndex ] );

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
	GLSL_SetUniform_NormalTextureMatrix( &tr.liquidShader, tess.svars.texMatrices[ TB_COLORMAP ] );

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

	gl_fogQuake3Shader->SetPortalClipping( backEnd.viewParms.isPortal );

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
		gl_fogQuake3Shader->SetUniform_BoneMatrix( MAX_BONES, tess.boneMatrices );
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

	if ( backEnd.viewParms.isPortal )
	{
		float plane[ 4 ];

		// clipping plane in world space
		plane[ 0 ] = backEnd.viewParms.portalPlane.normal[ 0 ];
		plane[ 1 ] = backEnd.viewParms.portalPlane.normal[ 1 ];
		plane[ 2 ] = backEnd.viewParms.portalPlane.normal[ 2 ];
		plane[ 3 ] = backEnd.viewParms.portalPlane.dist;

		gl_fogQuake3Shader->SetUniform_PortalPlane( plane );
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
	vec3_t fogColor;

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

		// setup shader with uniforms
		GL_BindProgram( &tr.depthToColorShader );
		GL_VertexAttribsState( tr.depthToColorShader.attribs );
		GL_State( 0 );  //GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

		GLSL_SetUniform_ModelViewProjectionMatrix( &tr.depthToColorShader, glState.modelViewProjectionMatrix[ glState.stackIndex ] );

		// Tr3B: might be cool for ghost player effects
		if ( glConfig2.vboVertexSkinningAvailable )
		{
			GLSL_SetUniform_VertexSkinning( &tr.depthToColorShader, tess.vboVertexSkinning );

			if ( tess.vboVertexSkinning )
			{
				glUniformMatrix4fvARB( tr.depthToColorShader.u_BoneMatrix, MAX_BONES, GL_FALSE, &tess.boneMatrices[ 0 ][ 0 ] );
			}
		}

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
		GL_BindProgram( &tr.volumetricFogShader );
		GL_VertexAttribsState( tr.volumetricFogShader.attribs );

		//GL_State(GLS_DEPTHTEST_DISABLE);  // | GLS_DEPTHMASK_TRUE);
		//GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR);
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_SRC_ALPHA );
		GL_Cull( CT_TWO_SIDED );

		glVertexAttrib4fvARB( ATTR_INDEX_COLOR, colorWhite );

		// set uniforms
		VectorCopy( backEnd.viewParms.orientation.origin, viewOrigin );  // in world space

		{
			fogDensity = tess.surfaceShader->fogParms.density;
			VectorCopy( tess.surfaceShader->fogParms.color, fogColor );
		}

		GLSL_SetUniform_ModelViewProjectionMatrix( &tr.volumetricFogShader, glState.modelViewProjectionMatrix[ glState.stackIndex ] );
		GLSL_SetUniform_UnprojectMatrix( &tr.volumetricFogShader, backEnd.viewParms.unprojectionMatrix );

		GLSL_SetUniform_ViewOrigin( &tr.volumetricFogShader, viewOrigin );
		glUniform1fARB( tr.volumetricFogShader.u_FogDensity, fogDensity );
		glUniform3fARB( tr.volumetricFogShader.u_FogColor, fogColor[ 0 ], fogColor[ 1 ], fogColor[ 2 ] );

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
