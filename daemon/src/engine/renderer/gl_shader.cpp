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

#include <common/FileSystem.h>
#include "gl_shader.h"

// We currently write GLShaderHeader to a file and memcpy all over it.
// Make sure it's a pod, so we don't put a std::string in it or something
// and try to memcpy over that or binary write an std::string to a file.
static_assert(std::is_pod<GLShaderHeader>::value, "Value must be a pod while code in this cpp file reads and writes this object to file as binary.");

extern std::unordered_map<std::string, const char *> shadermap;
// shaderKind's value will be determined later based on command line setting or absence of.
ShaderKind shaderKind = ShaderKind::Unknown;

// *INDENT-OFF*

GLShader_generic                         *gl_genericShader = nullptr;
GLShader_lightMapping                    *gl_lightMappingShader = nullptr;
GLShader_vertexLighting_DBS_entity       *gl_vertexLightingShader_DBS_entity = nullptr;
GLShader_vertexLighting_DBS_world        *gl_vertexLightingShader_DBS_world = nullptr;
GLShader_forwardLighting_omniXYZ         *gl_forwardLightingShader_omniXYZ = nullptr;
GLShader_forwardLighting_projXYZ         *gl_forwardLightingShader_projXYZ = nullptr;
GLShader_forwardLighting_directionalSun  *gl_forwardLightingShader_directionalSun = nullptr;
GLShader_shadowFill                      *gl_shadowFillShader = nullptr;
GLShader_reflection                      *gl_reflectionShader = nullptr;
GLShader_skybox                          *gl_skyboxShader = nullptr;
GLShader_fogQuake3                       *gl_fogQuake3Shader = nullptr;
GLShader_fogGlobal                       *gl_fogGlobalShader = nullptr;
GLShader_heatHaze                        *gl_heatHazeShader = nullptr;
GLShader_screen                          *gl_screenShader = nullptr;
GLShader_portal                          *gl_portalShader = nullptr;
GLShader_contrast                        *gl_contrastShader = nullptr;
GLShader_cameraEffects                   *gl_cameraEffectsShader = nullptr;
GLShader_blurX                           *gl_blurXShader = nullptr;
GLShader_blurY                           *gl_blurYShader = nullptr;
GLShader_debugShadowMap                  *gl_debugShadowMapShader = nullptr;
GLShader_depthToColor                    *gl_depthToColorShader = nullptr;
GLShader_lightVolume_omni                *gl_lightVolumeShader_omni = nullptr;
GLShader_liquid                          *gl_liquidShader = nullptr;
GLShader_volumetricFog                   *gl_volumetricFogShader = nullptr;
GLShader_motionblur                      *gl_motionblurShader = nullptr;
GLShader_ssao                            *gl_ssaoShader = nullptr;
GLShader_fxaa                            *gl_fxaaShader = nullptr;
GLShaderManager                           gl_shaderManager;

namespace // Implementation details
{
	inline void ThrowShaderError(Str::StringRef msg)
	{
		throw ShaderException(msg.c_str());
	}

	const char* GetInternalShader(Str::StringRef filename)
	{
		auto it = shadermap.find(filename);
		if (it != shadermap.end())
			return it->second;
		return nullptr;
	}

	void CRLFToLF(std::string& source)
	{
		size_t sourcePos = 0;
		size_t keepPos = 0;

		auto keep = [&](size_t keepLength)
		{
			if (sourcePos > 0)
				std::copy(source.begin() + sourcePos, source.begin() + sourcePos + keepLength,
					source.begin() + keepPos);
			keepPos += keepLength;
		};

		for (;;)
		{
			size_t targetPos = source.find("\r\n", sourcePos);
			// If we don't find a line break, shuffle what's left
			// into place and we're done.
			if (targetPos == std::string::npos)
			{
				size_t remainingLength = source.length() - sourcePos;
				keep(remainingLength);
				break;
			}
			// If we do find a line break, shuffle what's before it into place
			// except for the '\r\n'. But then tack on a '\n',
			// resulting in effectively just losing the '\r' in the sequence.
			size_t keepLength = (targetPos - sourcePos);
			keep(keepLength);
			source[keepPos] = '\n';
			++keepPos;
			sourcePos = targetPos + 2;
		}
		source.resize(keepPos);
	}

	// CR/LF's can wind up in the raw files because of how
	// version control system works and how Windows works.
	// Remove them so we are always comparing apples with apples.
	void NormalizeShaderText( std::string& text )
	{
		// A windows user changing the shader file can put
		// Windows can put CRLF's in the file. Make them LF's.
		CRLFToLF(text); 
	}

	std::string GetShaderFilename(Str::StringRef filename)
	{
		std::string shaderBase = GetShaderPath();
		if (shaderBase.empty())
			return shaderBase;
		std::string shaderFileName = FS::Path::Build(shaderBase, filename);
		return shaderFileName;
	}

	std::string GetShaderText(Str::StringRef filename)
	{
		// Shader type should be set during initialisation.
		if (shaderKind == ShaderKind::BuiltIn)
		{
			// Look for the shader internally. If not found, look for it externally.
			// If found neither internally or externally of if empty, then Error.
			auto text_ptr = GetInternalShader(filename);
			if (text_ptr == nullptr)
				ThrowShaderError(Str::Format("No shader found for shader: %s", filename));
			return text_ptr;
		}
		else if (shaderKind == ShaderKind::External)
		{
			std::string shaderText;
			std::string shaderFilename = GetShaderFilename(filename);

			Log::Notice("Loading shader '%s'", shaderFilename);

			std::error_code err;

			FS::File shaderFile = FS::RawPath::OpenRead(shaderFilename, err);
			if (err)
				ThrowShaderError(Str::Format("Cannot load shader from file %s: %s", shaderFilename, err.message()));

			shaderText = shaderFile.ReadAll(err);
			if (err)
				ThrowShaderError(Str::Format("Failed to read shader from file %s: %s", shaderFilename, err.message()));

			// Alert the user when a file does not match it's built-in version.
			// There should be no differences in normal conditions.
			// When testing shader file changes this is an expected message
			// and helps the tester track which files have changed and need 
			// to be recommmitted to git.
			// If one is not making shader files changes this message
			// indicates there is a mismatch between disk changes and builtins
			// which the application is out of sync with it's files 
			// and he translation script needs to be run.
			auto textPtr = GetInternalShader(filename);
			std::string internalShaderText;
			if (textPtr != nullptr)
				internalShaderText = textPtr;

			// Note to the user any differences that might exist between
			// what's on disk and what's compiled into the program in shaders.cpp.
			// The developer should be aware of any differences why they exist but
			// they might be expected or unexpected.
			// If the developer made changes they might want to be reminded of what
			// they have changed while they are working.
			// But it also might be that the developer hasn't made any changes but
			// the compiled code is shaders.cpp is just out of sync with the shader
			// files and that buildshaders.sh might need to be run to re-sync.
			// This message alerts user to either situation and they can decide
			// what's going on from seeing that.
			// We normalize the text by removing CL/LF's so they aren't considered
			// a difference as Windows or the Version Control System can put them in
			// and another OS might read them back and conisder that a difference
			// to what's in shader.cpp or vice vesa.
			NormalizeShaderText(internalShaderText);
			NormalizeShaderText(shaderText);
			if (internalShaderText != shaderText)
				Log::Warn("Note shader file differs from built-in shader: %s", shaderFilename);

			if (shaderText.empty())
				ThrowShaderError(Str::Format("Shader from file is empty: %s", shaderFilename));

			return shaderText;
		}
		// Will never reach here.
		ASSERT(false);
		ThrowShaderError("Internal error. ShaderKind not set.");
		return std::string();
	}
}

std::string GetShaderPath()
{
	std::string shaderPath;
	auto shaderPathVar = Cvar_Get("shaderpath", "", CVAR_INIT);
	if (shaderPathVar->string != nullptr)
		shaderPath = shaderPathVar->string;
	return shaderPath;
}

GLShaderManager::~GLShaderManager()
{
}

void GLShaderManager::freeAll()
{
	_shaders.clear();

	for ( GLint sh : _deformShaders )
		glDeleteShader( sh );

	_deformShaders.clear();
	_deformShaderLookup.clear();

	while ( !_shaderBuildQueue.empty() )
	{
		_shaderBuildQueue.pop();
	}

	_totalBuildTime = 0;
}

void GLShaderManager::UpdateShaderProgramUniformLocations( GLShader *shader, shaderProgram_t *shaderProgram ) const
{
	size_t uniformSize = shader->_uniformStorageSize;
	size_t numUniforms = shader->_uniforms.size();

	// create buffer for storing uniform locations
	shaderProgram->uniformLocations = ( GLint * ) ri.Z_Malloc( sizeof( GLint ) * numUniforms );

	// create buffer for uniform firewall
	shaderProgram->uniformFirewall = ( byte * ) ri.Z_Malloc( uniformSize );

	// update uniforms
	for (GLUniform *uniform : shader->_uniforms)
	{
		uniform->UpdateShaderProgramUniformLocation( shaderProgram );
	}
}

static inline void AddDefine( std::string& defines, const std::string& define, int value )
{
	defines += Str::Format("#ifndef %s\n#define %s %d\n#endif\n", define, define, value);
}

static inline void AddDefine( std::string& defines, const std::string& define, float value )
{
	defines += Str::Format("#ifndef %s\n#define %s %f\n#endif\n", define, define, value);
}

static inline void AddDefine( std::string& defines, const std::string& define, float v1, float v2 )
{
	defines += Str::Format("#ifndef %s\n#define %s vec2(%f, %f)\n#endif\n", define, define, v1, v2);
}

static inline void AddDefine( std::string& defines, const std::string& define )
{
	defines += Str::Format("#ifndef %s\n#define %s\n#endif\n", define, define);
}

// Has to match enum genFunc_t in tr_local.h
static const char *const genFuncNames[] = {
	  "DSTEP_NONE",
	  "DSTEP_SIN",
	  "DSTEP_SQUARE",
	  "DSTEP_TRIANGLE",
	  "DSTEP_SAWTOOTH",
	  "DSTEP_INV_SAWTOOTH",
	  "DSTEP_NOISE"
};

static std::string BuildDeformSteps( deformStage_t *deforms, int numDeforms )
{
	std::string steps;

	steps.reserve(256); // Might help a little.

	steps += "#define DEFORM_STEPS ";
	for( int step = 0; step < numDeforms; step++ )
	{
		const deformStage_t &ds = deforms[ step ];

		switch ( ds.deformation )
		{
		case deform_t::DEFORM_WAVE:
			steps += "DSTEP_LOAD_POS(1.0, 1.0, 1.0) ";
			steps += Str::Format("%s(%f, %f, %f) ",
				    genFuncNames[ Util::ordinal(ds.deformationWave.func) ],
				    ds.deformationWave.phase,
				    ds.deformationSpread,
				    ds.deformationWave.frequency );
			steps += "DSTEP_LOAD_NORM(1.0, 1.0, 1.0) ";
			steps += Str::Format("DSTEP_MODIFY_POS(%f, %f, 1.0) ",
				    ds.deformationWave.base,
				    ds.deformationWave.amplitude );
			break;

		case deform_t::DEFORM_BULGE:
			steps += "DSTEP_LOAD_TC(1.0, 0.0, 0.0) ";
			steps += Str::Format("DSTEP_SIN(0.0, %f, %f) ",
				    ds.bulgeWidth,
				    ds.bulgeSpeed * 0.001f );
			steps += "DSTEP_LOAD_NORM(1.0, 1.0, 1.0) ";
			steps += Str::Format("DSTEP_MODIFY_POS(0.0, %f, 1.0) ",
				    ds.bulgeHeight );
			break;

		case deform_t::DEFORM_MOVE:
			steps += Str::Format("%s(%f, 0.0, %f) ",
				    genFuncNames[ Util::ordinal(ds.deformationWave.func) ],
				    ds.deformationWave.phase,
				    ds.deformationWave.frequency );
			steps += Str::Format("DSTEP_LOAD_VEC(%f, %f, %f) ",
				    ds.moveVector[ 0 ],
				    ds.moveVector[ 1 ],
				    ds.moveVector[ 2 ] );
			steps += Str::Format("DSTEP_MODIFY_POS(%f, %f, 1.0) ",
				    ds.deformationWave.base,
				    ds.deformationWave.amplitude );
			break;

		case deform_t::DEFORM_NORMALS:
			steps += "DSTEP_LOAD_POS(1.0, 1.0, 1.0) ";
			steps += Str::Format("DSTEP_NOISE(0.0, 0.0, %f) ",
				    ds.deformationWave.frequency );
			steps += Str::Format("DSTEP_MODIFY_NORM(0.0, %f, 1.0) ",
				    0.98f * ds.deformationWave.amplitude );
			break;

		case deform_t::DEFORM_ROTGROW:
			steps += "DSTEP_LOAD_POS(1.0, 1.0, 1.0) ";
			steps += Str::Format("DSTEP_ROTGROW(%f, %f, %f) ",
				    ds.moveVector[0],
				    ds.moveVector[1],
				    ds.moveVector[2] );
			steps += "DSTEP_LOAD_COLOR(1.0, 1.0, 1.0) ";
			steps += "DSTEP_MODIFY_COLOR(-1.0, 1.0, 0.0) ";
			break;

		default:
			break;
		}
	}

	return steps;
}

std::string GLShaderManager::BuildDeformShaderText( const std::string& steps ) const
{
	std::string shaderText;

	shaderText = Str::Format( "#version %d\n", glConfig2.shadingLanguageVersion );
	shaderText += steps + "\n";

	// We added a lot of stuff but if we do something bad
	// in the GLSL shaders then we want the proper line
	// so we have to reset the line counting.
	shaderText += "#line 0\n";
	shaderText += GetShaderText("glsl/deformVertexes_vp.glsl");
	return shaderText;
}

int GLShaderManager::getDeformShaderIndex( deformStage_t *deforms, int numDeforms )
{
	std::string steps = BuildDeformSteps( deforms, numDeforms );
	int index = _deformShaderLookup[ steps ] - 1;

	if( index < 0 )
	{
		// compile new deform shader
		std::string shaderText = GLShaderManager::BuildDeformShaderText( steps );
		_deformShaders.push_back(CompileShader( "deformVertexes",
							shaderText,
							shaderText.length(),
							GL_VERTEX_SHADER ) );
		index = _deformShaders.size();
		_deformShaderLookup[ steps ] = index--;
	}

	return index;
}

std::string     GLShaderManager::BuildGPUShaderText( Str::StringRef mainShaderName,
    Str::StringRef libShaderNames,
    GLenum shaderType ) const
{
	char        filename[ MAX_QPATH ];
	char        *token;

	const char        *libNames = libShaderNames.c_str();

	GL_CheckErrors();

	std::string libs; // All libs concatenated
	libs.reserve(8192); // Might help, just an estimate.
	while ( 1 )
	{
		token = COM_ParseExt2( &libNames, false );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( shaderType == GL_VERTEX_SHADER )
			Com_sprintf( filename, sizeof( filename ), "glsl/%s_vp.glsl", token );
		else
			Com_sprintf( filename, sizeof( filename ), "glsl/%s_fp.glsl", token );

		libs += GetShaderText(filename);
	}

	// load main() program
	if ( shaderType == GL_VERTEX_SHADER )
		Com_sprintf( filename, sizeof( filename ), "glsl/%s_vp.glsl", mainShaderName.c_str() );
	else
		Com_sprintf( filename, sizeof( filename ), "glsl/%s_fp.glsl", mainShaderName.c_str() );

	std::string env;
	env.reserve( 1024 ); // Might help, just an estimate.

	if ( glConfig2.textureRGAvailable )
		AddDefine( env, "TEXTURE_RG", 1 );

	AddDefine( env, "r_AmbientScale", r_ambientScale->value );
	AddDefine( env, "r_SpecularScale", r_specularScale->value );
	AddDefine( env, "r_NormalScale", r_normalScale->value );
	AddDefine( env, "r_zNear", r_znear->value );

	AddDefine( env, "M_PI", static_cast<float>( M_PI ) );
	AddDefine( env, "MAX_SHADOWMAPS", MAX_SHADOWMAPS );

	float fbufWidthScale = 1.0f / glConfig.vidWidth;
	float fbufHeightScale = 1.0f / glConfig.vidHeight;

	AddDefine( env, "r_FBufScale", fbufWidthScale, fbufHeightScale );

	float npotWidthScale = 1;
	float npotHeightScale = 1;

	if ( !glConfig2.textureNPOTAvailable )
	{
		npotWidthScale = ( float ) glConfig.vidWidth / ( float ) NearestPowerOfTwo( glConfig.vidWidth );
		npotHeightScale = ( float ) glConfig.vidHeight / ( float ) NearestPowerOfTwo( glConfig.vidHeight );
	}

	AddDefine( env, "r_NPOTScale", npotWidthScale, npotHeightScale );

	if ( glConfig.driverType == glDriverType_t::GLDRV_MESA )
		AddDefine( env, "GLDRV_MESA", 1 );

	switch (glConfig.hardwareType)
	{
		case glHardwareType_t::GLHW_ATI:
		AddDefine(env, "GLHW_ATI", 1);
		break;
	case glHardwareType_t::GLHW_ATI_DX10:
		AddDefine(env, "GLHW_ATI_DX10", 1);
		break;
	case glHardwareType_t::GLHW_NV_DX10:
		AddDefine(env, "GLHW_NV_DX10", 1);
		break;
    default:
        break;
	}

	if ( r_shadows->integer >= Util::ordinal(shadowingMode_t::SHADOWING_ESM16) && glConfig2.textureFloatAvailable && glConfig2.framebufferObjectAvailable )
	{
		if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_ESM16) || r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_ESM32) )
		{
			AddDefine( env, "ESM", 1 );
		}
		else if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_EVSM32) )
		{
			AddDefine( env, "EVSM", 1 );
			// The exponents for the EVSM techniques should be less than ln(FLT_MAX/FILTER_SIZE)/2 {ln(FLT_MAX/1)/2 ~44.3}
			//         42.9 is the maximum possible value for FILTER_SIZE=15
			//         42.0 is the truncated value that we pass into the sample
			AddDefine( env, "r_EVSMExponents", 42.0f, 42.0f );
			if ( r_evsmPostProcess->integer )
				AddDefine( env,"r_EVSMPostProcess", 1 );
		}
		else
		{
			AddDefine( env, "VSM", 1 );

			if ( glConfig.hardwareType == glHardwareType_t::GLHW_ATI )
				AddDefine( env, "VSM_CLAMP", 1 );
		}

		if ( ( glConfig.hardwareType == glHardwareType_t::GLHW_NV_DX10 || glConfig.hardwareType == glHardwareType_t::GLHW_ATI_DX10 ) && r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_VSM32) )
			AddDefine( env, "VSM_EPSILON", 0.000001f );
		else
			AddDefine( env, "VSM_EPSILON", 0.0001f );

		if ( r_lightBleedReduction->value )
			AddDefine( env, "r_LightBleedReduction", r_lightBleedReduction->value );

		if ( r_overDarkeningFactor->value )
			AddDefine( env, "r_OverDarkeningFactor", r_overDarkeningFactor->value );

		if ( r_shadowMapDepthScale->value )
			AddDefine( env, "r_ShadowMapDepthScale", r_shadowMapDepthScale->value );

		if ( r_debugShadowMaps->integer )
			AddDefine( env, "r_DebugShadowMaps", r_debugShadowMaps->integer );

		if ( r_softShadows->integer == 6 )
			AddDefine( env, "PCSS", 1 );
		else if ( r_softShadows->integer )
			AddDefine( env, "r_PCFSamples", r_softShadows->value + 1.0f );

		if ( r_parallelShadowSplits->integer )
			AddDefine( env, Str::Format( "r_ParallelShadowSplits_%d", r_parallelShadowSplits->integer ) );

		if ( r_showParallelShadowSplits->integer )
			AddDefine( env, "r_ShowParallelShadowSplits", 1 );
	}

	if ( r_precomputedLighting->integer )
		AddDefine( env, "r_precomputedLighting", 1 );

	if ( r_showLightMaps->integer )
		AddDefine( env, "r_showLightMaps", r_showLightMaps->integer );

	if ( r_showDeluxeMaps->integer )
		AddDefine( env, "r_showDeluxeMaps", r_showDeluxeMaps->integer );

	if ( r_showEntityNormals->integer )
		AddDefine( env, "r_showEntityNormals", r_showEntityNormals->integer );

	if ( glConfig2.vboVertexSkinningAvailable )
	{
		AddDefine( env, "r_VertexSkinning", 1 );
		AddDefine( env, "MAX_GLSL_BONES", glConfig2.maxVertexSkinningBones );
	}
	else
	{
		AddDefine( env, "MAX_GLSL_BONES", 4 );
	}

	if ( r_wrapAroundLighting->value )
		AddDefine( env, "r_WrapAroundLighting", r_wrapAroundLighting->value );

	if ( r_halfLambertLighting->integer )
		AddDefine( env, "r_HalfLambertLighting", 1 );

	if ( r_rimLighting->integer )
	{
		AddDefine( env, "r_RimLighting", 1 );
		AddDefine( env, "r_RimExponent", r_rimExponent->value );
	}

	// OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
	// so we have to reset the line counting
	env += "#line 0\n";

	std::string shaderText = env + libs + GetShaderText(filename);

	return shaderText;
}

bool GLShaderManager::buildPermutation( GLShader *shader, int macroIndex, int deformIndex )
{
	std::string compileMacros;
	int  startTime = ri.Milliseconds();
	int  endTime;
	size_t i = macroIndex + ( deformIndex << shader->_compileMacros.size() );

	// program already exists
	if ( i < shader->_shaderPrograms.size() &&
	     shader->_shaderPrograms[ i ].program )
	{
		return false;
	}

	if( shader->GetCompileMacrosString( macroIndex, compileMacros ) )
	{
		shader->BuildShaderCompileMacros( compileMacros );

		if( i >= shader->_shaderPrograms.size() )
			shader->_shaderPrograms.resize( (deformIndex + 1) << shader->_compileMacros.size() );

		shaderProgram_t *shaderProgram = &shader->_shaderPrograms[ i ];

		shaderProgram->program = glCreateProgram();
		shaderProgram->attribs = shader->_vertexAttribsRequired; // | _vertexAttribsOptional;

		if( deformIndex > 0 )
		{
			shaderProgram_t *baseShader = &shader->_shaderPrograms[ macroIndex ];
			if( !baseShader->VS || !baseShader->FS )
				CompileGPUShaders( shader, baseShader, compileMacros );

			glAttachShader( shaderProgram->program, baseShader->VS );
			glAttachShader( shaderProgram->program, _deformShaders[ deformIndex ] );
			glAttachShader( shaderProgram->program, baseShader->FS );

			BindAttribLocations( shaderProgram->program );
			LinkProgram( shaderProgram->program );
		}
		else if ( !LoadShaderBinary( shader, i ) )
		{
			CompileAndLinkGPUShaderProgram(	shader, shaderProgram, compileMacros, deformIndex );
			SaveShaderBinary( shader, i );
		}

		UpdateShaderProgramUniformLocations( shader, shaderProgram );
		GL_BindProgram( shaderProgram );
		shader->SetShaderProgramUniforms( shaderProgram );
		GL_BindProgram( nullptr );

		ValidateProgram( shaderProgram->program );
		GL_CheckErrors();

		endTime = ri.Milliseconds();
		_totalBuildTime += ( endTime - startTime );
		return true;
	}
	return false;
}

void GLShaderManager::buildAll()
{
	while ( !_shaderBuildQueue.empty() )
	{
		GLShader& shader = *_shaderBuildQueue.front();
		size_t numPermutations = 1 << shader.GetNumOfCompiledMacros();
		size_t i;

		for( i = 0; i < numPermutations; i++ )
		{
			buildPermutation( &shader, i, 0 );
		}

		_shaderBuildQueue.pop();
	}

	Log::Notice( "glsl shaders took %d msec to build", _totalBuildTime );

	if( r_recompileShaders->integer )
	{
		ri.Cvar_Set( "r_recompileShaders", "0" );
	}
}

void GLShaderManager::InitShader( GLShader *shader )
{
	shader->_shaderPrograms = std::vector<shaderProgram_t>( 1 << shader->_compileMacros.size() );

	shader->_uniformStorageSize = 0;
	for ( std::size_t i = 0; i < shader->_uniforms.size(); i++ )
	{
		GLUniform *uniform = shader->_uniforms[ i ];
		uniform->SetLocationIndex( i );
		uniform->SetFirewallIndex( shader->_uniformStorageSize );
		shader->_uniformStorageSize += uniform->GetSize();
	}

	std::string vertexInlines;
	shader->BuildShaderVertexLibNames( vertexInlines );

	std::string fragmentInlines;
	shader->BuildShaderFragmentLibNames( fragmentInlines );

	shader->_vertexShaderText = BuildGPUShaderText( shader->GetMainShaderName(), vertexInlines, GL_VERTEX_SHADER );
	shader->_fragmentShaderText = BuildGPUShaderText( shader->GetMainShaderName(), fragmentInlines, GL_FRAGMENT_SHADER );
	std::string combinedShaderText= shader->_vertexShaderText + shader->_fragmentShaderText;

	shader->_checkSum = Com_BlockChecksum( combinedShaderText.c_str(), combinedShaderText.length() );
}

bool GLShaderManager::LoadShaderBinary( GLShader *shader, size_t programNum )
{
#ifdef GLEW_ARB_get_program_binary
	GLint          success;
	const byte    *binaryptr;
	GLShaderHeader shaderHeader;

	if (!GetShaderPath().empty())
		return false;

	// we need to recompile the shaders
	if( r_recompileShaders->integer )
		return false;

	// don't even try if the necessary functions aren't available
	if( !glConfig2.getProgramBinaryAvailable )
		return false;

	std::error_code err;

	std::string shaderFilename = Str::Format("glsl/%s/%s_%u.bin", shader->GetName(), shader->GetName(), (unsigned int)programNum);
	FS::File shaderFile = FS::HomePath::OpenRead(shaderFilename, err);
	if (err)
		return false;

	std::string shaderData = shaderFile.ReadAll(err);
	if (err)
		return false;

	auto fileLength = shaderData.length();
	if (fileLength <= 0)
		return false;

	if (fileLength < sizeof(shaderHeader))
		return false;

	binaryptr = reinterpret_cast<const byte*>(shaderData.data());

	// get the shader header from the file
	memcpy( &shaderHeader, binaryptr, sizeof( shaderHeader ) );
	binaryptr += sizeof( shaderHeader );

	// check if this shader binary is the correct format
	if ( shaderHeader.version != GL_SHADER_VERSION )
		return false;

	// make sure this shader uses the same number of macros
	if ( shaderHeader.numMacros != shader->GetNumOfCompiledMacros() )
		return false;

	// make sure this shader uses the same macros
	for ( unsigned int i = 0; i < shaderHeader.numMacros; i++ )
	{
		if ( shader->_compileMacros[ i ]->GetType() != shaderHeader.macros[ i ] )
			return false;
	}

	// make sure the checksums for the source code match
	if ( shaderHeader.checkSum != shader->_checkSum )
		return false;

	// load the shader
	shaderProgram_t *shaderProgram = &shader->_shaderPrograms[ programNum ];
	glProgramBinary( shaderProgram->program, shaderHeader.binaryFormat, binaryptr, shaderHeader.binaryLength );
	glGetProgramiv( shaderProgram->program, GL_LINK_STATUS, &success );

	if ( !success )
		return false;

	return true;
#else
	return false;
#endif
}
void GLShaderManager::SaveShaderBinary( GLShader *shader, size_t programNum )
{
#ifdef GLEW_ARB_get_program_binary
	GLint                 binaryLength;
	GLuint                binarySize = 0;
	byte                  *binary;
	byte                  *binaryptr;
	GLShaderHeader        shaderHeader{}; // Zero init.
	shaderProgram_t       *shaderProgram;

	if (!GetShaderPath().empty())
		return;

	// don't even try if the necessary functions aren't available
	if( !glConfig2.getProgramBinaryAvailable )
	{
		return;
	}

	shaderProgram = &shader->_shaderPrograms[ programNum ];

	// find output size
	binarySize += sizeof( shaderHeader );
	glGetProgramiv( shaderProgram->program, GL_PROGRAM_BINARY_LENGTH, &binaryLength );
	binarySize += binaryLength;

	binaryptr = binary = ( byte* )ri.Hunk_AllocateTempMemory( binarySize );

	// reserve space for the header
	binaryptr += sizeof( shaderHeader );

	// get the program binary and write it to the buffer
	glGetProgramBinary( shaderProgram->program, binaryLength, nullptr, &shaderHeader.binaryFormat, binaryptr );

	// set the header
	shaderHeader.version = GL_SHADER_VERSION;
	shaderHeader.numMacros = shader->_compileMacros.size();

	for ( unsigned int i = 0; i < shaderHeader.numMacros; i++ )
	{
		shaderHeader.macros[ i ] = shader->_compileMacros[ i ]->GetType();
	}

	shaderHeader.binaryLength = binaryLength;
	shaderHeader.checkSum = shader->_checkSum;

	// write the header to the buffer
	memcpy(binary, &shaderHeader, sizeof( shaderHeader ) );

	auto fileName = Str::Format("glsl/%s/%s_%u.bin", shader->GetName(), shader->GetName(), (unsigned int)programNum);
	ri.FS_WriteFile(fileName.c_str(), binary, binarySize);

	ri.Hunk_FreeTempMemory( binary );
#endif
}

void GLShaderManager::CompileGPUShaders( GLShader *shader, shaderProgram_t *program,
					 const std::string &compileMacros ) const
{
	// header of the glsl shader
	std::string vertexHeader;
	std::string fragmentHeader;
	std::string miscText;

	if ( glConfig2.shadingLanguageVersion != 120 )
	{
		// HACK: abuse the GLSL preprocessor to turn GLSL 1.20 shaders into 1.30 ones

		vertexHeader += "#version 130\n";
		fragmentHeader += "#version 130\n";

		vertexHeader += "#define attribute in\n";
		vertexHeader += "#define varying out\n";

		fragmentHeader += "#define varying in\n";

		vertexHeader += "#define textureCube texture\n";
		vertexHeader += "#define texture2D texture\n";
		vertexHeader += "#define texture2DProj textureProj\n";

		fragmentHeader += "#define textureCube texture\n";
		fragmentHeader += "#define texture2D texture\n";
		fragmentHeader += "#define texture2DProj textureProj\n";
	}
	else
	{
		vertexHeader += "#version 120\n";
		fragmentHeader += "#version 120\n";

		// add implementation of GLSL 1.30 smoothstep() function
		miscText += "float smoothstep(float edge0, float edge1, float x) { float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0); return t * t * (3.0 - 2.0 * t); }\n";
	}

	// permutation macros
	std::string macrosString;

	if ( !compileMacros.empty() )
	{
		const char *compileMacros_ = compileMacros.c_str();
		const char **compileMacrosP = &compileMacros_;
		char       *token;

		while ( 1 )
		{
			token = COM_ParseExt2( compileMacrosP, false );

			if ( !token[ 0 ] )
			{
				break;
			}

			macrosString += Str::Format( "#ifndef %s\n#define %s 1\n#endif\n", token, token );
		}
	}

	// add them
	std::string vertexShaderTextWithMacros = vertexHeader + macrosString + miscText +  shader->_vertexShaderText;
	std::string fragmentShaderTextWithMacros = fragmentHeader + macrosString + miscText + shader->_fragmentShaderText;
	program->VS = CompileShader( shader->GetName(),
				     vertexShaderTextWithMacros,
				     vertexShaderTextWithMacros.length(),
				     GL_VERTEX_SHADER );
	program->FS = CompileShader( shader->GetName(),
				     fragmentShaderTextWithMacros,
				     fragmentShaderTextWithMacros.length(),
				     GL_FRAGMENT_SHADER );
}

void GLShaderManager::CompileAndLinkGPUShaderProgram( GLShader *shader, shaderProgram_t *program,
						      Str::StringRef compileMacros, int deformIndex ) const
{
	GLShaderManager::CompileGPUShaders( shader, program, compileMacros );

	glAttachShader( program->program, program->VS );
	glAttachShader( program->program, _deformShaders[ deformIndex ] );
	glAttachShader( program->program, program->FS );

	BindAttribLocations( program->program );
	LinkProgram( program->program );
}

GLuint GLShaderManager::CompileShader( Str::StringRef programName, Str::StringRef shaderText, int shaderTextSize, GLenum shaderType ) const
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
		PrintShaderSource( programName, shader );
		PrintInfoLog( shader );
		ThrowShaderError(Str::Format("Couldn't compile %s shader: %s", ( shaderType == GL_VERTEX_SHADER) ? "vertex" : "fragment", programName));
	}

	return shader;
}

void GLShaderManager::PrintShaderSource( Str::StringRef programName, GLuint object ) const
{
	char        *msg;
	int         maxLength = 0;
	std::string msgText;

	glGetShaderiv( object, GL_SHADER_SOURCE_LENGTH, &maxLength );

	msg = ( char * ) ri.Hunk_AllocateTempMemory( maxLength );

	glGetShaderSource( object, maxLength, &maxLength, msg );

	Log::Warn("Source for shader program %s:\n%s", programName, msg);

	ri.Hunk_FreeTempMemory( msg );
}

void GLShaderManager::PrintInfoLog( GLuint object) const
{
	char        *msg;
	int         maxLength = 0;
	std::string msgText;

	if ( glIsShader( object ) )
	{
		glGetShaderiv( object, GL_INFO_LOG_LENGTH, &maxLength );
	}
	else if ( glIsProgram( object ) )
	{
		glGetProgramiv( object, GL_INFO_LOG_LENGTH, &maxLength );
	}
	else
	{
		Log::Warn( "object is not a shader or program\n" );
		return;
	}

	msg = ( char * ) ri.Hunk_AllocateTempMemory( maxLength );

	if ( glIsShader( object ) )
	{
		glGetShaderInfoLog( object, maxLength, &maxLength, msg );
		msgText = "Compile log:";
	}
	else if ( glIsProgram( object ) )
	{
		glGetProgramInfoLog( object, maxLength, &maxLength, msg );
		msgText = "Link log:";
	}
	if (maxLength > 0)
		msgText += '\n';
	msgText += msg;
	Log::Warn(msgText);

	ri.Hunk_FreeTempMemory( msg );
}

void GLShaderManager::LinkProgram( GLuint program ) const
{
	GLint linked;

#ifdef GLEW_ARB_get_program_binary
	// Apparently, this is necessary to get the binary program via glGetProgramBinary
	if( glConfig2.getProgramBinaryAvailable )
	{
		glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE );
	}
#endif
	glLinkProgram( program );

	glGetProgramiv( program, GL_LINK_STATUS, &linked );

	if ( !linked )
	{
		PrintInfoLog( program );
		ThrowShaderError( "Shaders failed to link!" );
	}
}

void GLShaderManager::ValidateProgram( GLuint program ) const
{
	GLint validated;

	glValidateProgram( program );

	glGetProgramiv( program, GL_VALIDATE_STATUS, &validated );

	if ( !validated )
	{
		PrintInfoLog( program );
		ThrowShaderError( "Shaders failed to validate!" );
	}
}

void GLShaderManager::BindAttribLocations( GLuint program ) const
{
	for ( uint32_t i = 0; i < ATTR_INDEX_MAX; i++ )
	{
		glBindAttribLocation( program, i, attributeNames[ i ] );
	}
}

bool GLCompileMacro_USE_VERTEX_SKINNING::HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	for (const GLCompileMacro* macro : macros)
	{
		//if(GLCompileMacro_USE_VERTEX_ANIMATION* m = dynamic_cast<GLCompileMacro_USE_VERTEX_ANIMATION*>(macro))
		if ( ( permutation & macro->GetBit() ) != 0 && macro->GetType() == USE_VERTEX_ANIMATION )
		{
			//Log::Notice("conflicting macro! canceling '%s' vs. '%s'", GetName(), macro->GetName());
			return true;
		}
	}

	return false;
}

bool GLCompileMacro_USE_VERTEX_SKINNING::MissesRequiredMacros( size_t /*permutation*/, const std::vector< GLCompileMacro * > &/*macros*/ ) const
{
	return !glConfig2.vboVertexSkinningAvailable;
}

bool GLCompileMacro_USE_VERTEX_ANIMATION::HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ( ( permutation & macro->GetBit() ) != 0 && macro->GetType() == USE_VERTEX_SKINNING )
		{
			//Log::Notice("conflicting macro! canceling '%s' vs. '%s'", GetName(), macro->GetName());
			return true;
		}
	}

	return false;
}

uint32_t GLCompileMacro_USE_VERTEX_ANIMATION::GetRequiredVertexAttributes() const
{
	uint32_t attribs = ATTR_POSITION2 | ATTR_QTANGENT2;

	return attribs;
}

bool GLCompileMacro_USE_VERTEX_SPRITE::HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	for (const GLCompileMacro* macro : macros)
	{
		if ( ( permutation & macro->GetBit() ) != 0 && (macro->GetType() == USE_VERTEX_SKINNING || macro->GetType() == USE_VERTEX_ANIMATION))
		{
			//Log::Notice("conflicting macro! canceling '%s' vs. '%s'", GetName(), macro->GetName());
			return true;
		}
	}

	return false;
}

bool GLCompileMacro_USE_PARALLAX_MAPPING::MissesRequiredMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	bool foundUSE_NORMAL_MAPPING = false;

	for (const GLCompileMacro* macro : macros)
	{
		if ( ( permutation & macro->GetBit() ) != 0 && macro->GetType() == USE_NORMAL_MAPPING )
		{
			foundUSE_NORMAL_MAPPING = true;
			break;
		}
	}

	if ( !foundUSE_NORMAL_MAPPING )
	{
		//Log::Notice("missing macro! canceling '%s' <= '%s'", GetName(), "USE_NORMAL_MAPPING");
		return true;
	}

	return false;
}

bool GLCompileMacro_USE_REFLECTIVE_SPECULAR::MissesRequiredMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
{
	bool foundUSE_NORMAL_MAPPING = false;

	for (const GLCompileMacro* macro : macros)
	{
		if ( ( permutation & macro->GetBit() ) != 0 && macro->GetType() == USE_NORMAL_MAPPING )
		{
			foundUSE_NORMAL_MAPPING = true;
			break;
		}
	}

	if ( !foundUSE_NORMAL_MAPPING )
	{
		//Log::Notice("missing macro! canceling '%s' <= '%s'", GetName(), "USE_NORMAL_MAPPING");
		return true;
	}

	return false;
}

bool GLShader::GetCompileMacrosString( size_t permutation, std::string &compileMacrosOut ) const
{
	compileMacrosOut.clear();

	for (const GLCompileMacro* macro : _compileMacros)
	{
		if ( permutation & macro->GetBit() )
		{
			if ( macro->HasConflictingMacros( permutation, _compileMacros ) )
			{
				//Log::Notice("conflicting macro! canceling '%s'", macro->GetName());
				return false;
			}

			if ( macro->MissesRequiredMacros( permutation, _compileMacros ) )
				return false;

			compileMacrosOut += macro->GetName();
			compileMacrosOut += " ";
		}
	}

	return true;
}

int GLShader::SelectProgram()
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

	return index;
}

void GLShader::BindProgram( int deformIndex )
{
	int macroIndex = SelectProgram();
	size_t index = macroIndex + ( size_t(deformIndex) << _compileMacros.size() );

	// program may not be loaded yet because the shader manager hasn't yet gotten to it
	// so try to load it now
	if ( index >= _shaderPrograms.size() || !_shaderPrograms[ index ].program )
	{
		_shaderManager->buildPermutation( this, macroIndex, deformIndex );
	}

	// program is still not loaded
	if ( index >= _shaderPrograms.size() || !_shaderPrograms[ index ].program )
	{
		std::string activeMacros;
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

		ThrowShaderError(Str::Format("Invalid shader configuration: shader = '%s', macros = '%s'", _name, activeMacros ));
	}

	_currentProgram = &_shaderPrograms[ index ];

	if ( r_logFile->integer )
	{
		std::string macros;

		this->GetCompileMacrosString( index, macros );

		auto msg = Str::Format("--- GL_BindProgram( name = '%s', macros = '%s' ) ---\n", this->GetName(), macros);
		GLimp_LogComment(msg.c_str());
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

GLShader_generic::GLShader_generic( GLShaderManager *manager ) :
	GLShader( "generic", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_ColorTextureMatrix( this ),
	u_ViewOrigin( this ),
	u_ViewUp( this ),
	u_AlphaThreshold( this ),
	u_ModelMatrix( this ),
	u_ProjectionMatrixTranspose( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_VERTEX_SPRITE( this ),
	GLCompileMacro_USE_TCGEN_ENVIRONMENT( this ),
	GLCompileMacro_USE_TCGEN_LIGHTMAP( this ),
	GLCompileMacro_USE_DEPTH_FADE( this )
{
}

void GLShader_generic::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning vertexAnimation vertexSprite ";
}

void GLShader_generic::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 1 );
}

GLShader_lightMapping::GLShader_lightMapping( GLShaderManager *manager ) :
	GLShader( "lightMapping", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT | ATTR_COLOR, manager ),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_GlowTextureMatrix( this ),
	u_SpecularExponent( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_AlphaThreshold( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this ),
	GLCompileMacro_USE_GLOW_MAPPING( this )
{
}

void GLShader_lightMapping::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning vertexAnimation vertexSprite ";
}

void GLShader_lightMapping::BuildShaderFragmentLibNames( std::string& fragmentInlines )
{
	fragmentInlines += "reliefMapping";
}

void GLShader_lightMapping::BuildShaderCompileMacros( std::string& /*compileMacros*/ )
{
}

void GLShader_lightMapping::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_SpecularMap" ),  2 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightMap" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DeluxeMap" ), 4 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_GlowMap" ), 5 );
}

GLShader_vertexLighting_DBS_entity::GLShader_vertexLighting_DBS_entity( GLShaderManager *manager ) :
	GLShader( "vertexLighting_DBS_entity", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_GlowTextureMatrix( this ),
	u_SpecularExponent( this ),
	u_AlphaThreshold( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_DepthScale( this ),
	u_EnvironmentInterpolation( this ),
	u_LightGridOrigin( this ),
	u_LightGridScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this ),
	GLCompileMacro_USE_REFLECTIVE_SPECULAR( this ),
	GLCompileMacro_USE_GLOW_MAPPING( this )
{
}

void GLShader_vertexLighting_DBS_entity::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning vertexAnimation ";
}

void GLShader_vertexLighting_DBS_entity::BuildShaderFragmentLibNames( std::string& fragmentInlines )
{
	fragmentInlines += "reliefMapping";
}

void GLShader_vertexLighting_DBS_entity::BuildShaderCompileMacros( std::string& )
{
}

void GLShader_vertexLighting_DBS_entity::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_SpecularMap" ), 2 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_EnvironmentMap0" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_EnvironmentMap1" ), 4 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_GlowMap" ), 5 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid1" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid2" ), 7 );
}

GLShader_vertexLighting_DBS_world::GLShader_vertexLighting_DBS_world( GLShaderManager *manager ) :
	GLShader( "vertexLighting_DBS_world",
	          ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT | ATTR_COLOR,
		  manager
	        ),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_GlowTextureMatrix( this ),
	u_SpecularExponent( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_AlphaThreshold( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_DepthScale( this ),
	u_LightWrapAround( this ),
	u_LightGridOrigin( this ),
	u_LightGridScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this ),
	GLCompileMacro_USE_GLOW_MAPPING( this )
{
}

void GLShader_vertexLighting_DBS_world::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning vertexAnimation vertexSprite ";
}
void GLShader_vertexLighting_DBS_world::BuildShaderFragmentLibNames( std::string& fragmentInlines )
{
	fragmentInlines += "reliefMapping";
}

void GLShader_vertexLighting_DBS_world::BuildShaderCompileMacros( std::string& )
{
}

void GLShader_vertexLighting_DBS_world::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_SpecularMap" ), 2 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_GlowMap" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid1" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid2" ), 7 );
}

GLShader_forwardLighting_omniXYZ::GLShader_forwardLighting_omniXYZ( GLShaderManager *manager ):
	GLShader("forwardLighting_omniXYZ", "forwardLighting", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_SpecularExponent( this ),
	u_AlphaThreshold( this ),
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
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )
{
}

void GLShader_forwardLighting_omniXYZ::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning vertexAnimation ";
}

void GLShader_forwardLighting_omniXYZ::BuildShaderFragmentLibNames( std::string& fragmentInlines )
{
	fragmentInlines += "reliefMapping";
}

void GLShader_forwardLighting_omniXYZ::BuildShaderCompileMacros( std::string& /*compileMacros*/ )
{
}

void GLShader_forwardLighting_omniXYZ::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_SpecularMap" ), 2 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_AttenuationMapXY" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_AttenuationMapZ" ), 4 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap" ), 5 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_RandomMap" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap" ), 7 );
}

GLShader_forwardLighting_projXYZ::GLShader_forwardLighting_projXYZ( GLShaderManager *manager ):
	GLShader("forwardLighting_projXYZ", "forwardLighting", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_SpecularExponent( this ),
	u_AlphaThreshold( this ),
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
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )
{
}

void GLShader_forwardLighting_projXYZ::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning vertexAnimation ";
}

void GLShader_forwardLighting_projXYZ::BuildShaderFragmentLibNames( std::string& fragmentInlines )
{
	fragmentInlines += "reliefMapping";
}

void GLShader_forwardLighting_projXYZ::BuildShaderCompileMacros( std::string& compileMacros )
{
	compileMacros += "LIGHT_PROJ ";
}

void GLShader_forwardLighting_projXYZ::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_SpecularMap" ), 2 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_AttenuationMapXY" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_AttenuationMapZ" ), 4 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap0" ), 5 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_RandomMap" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap0" ), 7 );
}

GLShader_forwardLighting_directionalSun::GLShader_forwardLighting_directionalSun( GLShaderManager *manager ):
	GLShader("forwardLighting_directionalSun", "forwardLighting", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager),
	u_DiffuseTextureMatrix( this ),
	u_NormalTextureMatrix( this ),
	u_SpecularTextureMatrix( this ),
	u_SpecularExponent( this ),
	u_AlphaThreshold( this ),
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
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_DepthScale( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this ),
	GLCompileMacro_USE_SHADOWING( this )
{
}

void GLShader_forwardLighting_directionalSun::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning vertexAnimation ";
}

void GLShader_forwardLighting_directionalSun::BuildShaderFragmentLibNames( std::string& fragmentInlines )
{
	fragmentInlines += "reliefMapping";
}

void GLShader_forwardLighting_directionalSun::BuildShaderCompileMacros( std::string& compileMacros )
{
	compileMacros += "LIGHT_DIRECTIONAL ";
}

void GLShader_forwardLighting_directionalSun::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DiffuseMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_SpecularMap" ), 2 );
	//glUniform1i(glGetUniformLocation( shaderProgram->program, "u_AttenuationMapXY" ), 3);
	//glUniform1i(glGetUniformLocation( shaderProgram->program, "u_AttenuationMapZ" ), 4);
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap0" ), 5 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap1" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap2" ), 7 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap3" ), 8 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap4" ), 9 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap0" ), 10 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap1" ), 11 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap2" ), 12 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap3" ), 13 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap4" ), 14 );
}

GLShader_shadowFill::GLShader_shadowFill( GLShaderManager *manager ) :
	GLShader( "shadowFill", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_ColorTextureMatrix( this ),
	u_ViewOrigin( this ),
	u_AlphaThreshold( this ),
	u_LightOrigin( this ),
	u_LightRadius( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Color( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_LIGHT_DIRECTIONAL( this )
{
}

void GLShader_shadowFill::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning vertexAnimation ";
}

void GLShader_shadowFill::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_reflection::GLShader_reflection( GLShaderManager *manager ):
	GLShader("reflection", "reflection_CB", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_NormalTextureMatrix( this ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_NORMAL_MAPPING( this )
{
}

void GLShader_reflection::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning vertexAnimation ";
}

void GLShader_reflection::BuildShaderCompileMacros( std::string& )
{
}

void GLShader_reflection::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 1 );
}

GLShader_skybox::GLShader_skybox( GLShaderManager *manager ) :
	GLShader( "skybox", ATTR_POSITION, manager ),
	u_ViewOrigin( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	GLDeformStage( this )
{
}

void GLShader_skybox::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_fogQuake3::GLShader_fogQuake3( GLShaderManager *manager ) :
	GLShader( "fogQuake3", ATTR_POSITION | ATTR_QTANGENT, manager ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_Color( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	u_FogDistanceVector( this ),
	u_FogDepthVector( this ),
	u_FogEyeT( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this )
{
}

void GLShader_fogQuake3::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning vertexAnimation ";
}

void GLShader_fogQuake3::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_fogGlobal::GLShader_fogGlobal( GLShaderManager *manager ) :
	GLShader( "fogGlobal", ATTR_POSITION, manager ),
	u_ViewOrigin( this ),
	u_ViewMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_UnprojectMatrix( this ),
	u_Color( this ),
	u_FogDistanceVector( this ),
	u_FogDepthVector( this )
{
}

void GLShader_fogGlobal::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 1 );
}

GLShader_heatHaze::GLShader_heatHaze( GLShaderManager *manager ) :
	GLShader( "heatHaze", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT, manager ),
	u_NormalTextureMatrix( this ),
	u_ViewOrigin( this ),
	u_ViewUp( this ),
	u_DeformMagnitude( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_ModelViewMatrixTranspose( this ),
	u_ProjectionMatrixTranspose( this ),
	u_ColorModulate( this ),
	u_Color( this ),
	u_Bones( this ),
	u_VertexInterpolation( this ),
	GLDeformStage( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this ),
	GLCompileMacro_USE_VERTEX_ANIMATION( this ),
	GLCompileMacro_USE_VERTEX_SPRITE( this )
{
}

void GLShader_heatHaze::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning vertexAnimation vertexSprite ";
}

void GLShader_heatHaze::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 1 );
}

GLShader_screen::GLShader_screen( GLShaderManager *manager ) :
	GLShader( "screen", ATTR_POSITION, manager ),
	u_ModelViewProjectionMatrix( this )
{
}

void GLShader_screen::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
}

GLShader_portal::GLShader_portal( GLShaderManager *manager ) :
	GLShader( "portal", ATTR_POSITION, manager ),
	u_ModelViewMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_PortalRange( this )
{
}

void GLShader_portal::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
}

GLShader_contrast::GLShader_contrast( GLShaderManager *manager ) :
	GLShader( "contrast", ATTR_POSITION, manager ),
	u_ModelViewProjectionMatrix( this )
{
}

void GLShader_contrast::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_cameraEffects::GLShader_cameraEffects( GLShaderManager *manager ) :
	GLShader( "cameraEffects", ATTR_POSITION | ATTR_TEXCOORD, manager ),
	u_ColorModulate( this ),
	u_ColorTextureMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_DeformMagnitude( this ),
	u_InverseGamma( this )
{
}

void GLShader_cameraEffects::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 3 );
}

GLShader_blurX::GLShader_blurX( GLShaderManager *manager ) :
	GLShader( "blurX", ATTR_POSITION, manager ),
	u_ModelViewProjectionMatrix( this ),
	u_DeformMagnitude( this ),
	u_TexScale( this )
{
}

void GLShader_blurX::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_blurY::GLShader_blurY( GLShaderManager *manager ) :
	GLShader( "blurY", ATTR_POSITION, manager ),
	u_ModelViewProjectionMatrix( this ),
	u_DeformMagnitude( this ),
	u_TexScale( this )
{
}

void GLShader_blurY::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

GLShader_debugShadowMap::GLShader_debugShadowMap( GLShaderManager *manager ) :
	GLShader( "debugShadowMap", ATTR_POSITION, manager ),
	u_ModelViewProjectionMatrix( this )
{
}

void GLShader_debugShadowMap::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
}

GLShader_depthToColor::GLShader_depthToColor( GLShaderManager *manager ) :
	GLShader( "depthToColor", ATTR_POSITION | ATTR_QTANGENT, manager ),
	u_ModelViewProjectionMatrix( this ),
	u_Bones( this ),
	GLCompileMacro_USE_VERTEX_SKINNING( this )
{
}

void GLShader_depthToColor::BuildShaderVertexLibNames( std::string& vertexInlines )
{
	vertexInlines += "vertexSimple vertexSkinning ";
}

GLShader_lightVolume_omni::GLShader_lightVolume_omni( GLShaderManager *manager ) :
	GLShader( "lightVolume_omni", ATTR_POSITION, manager ),
	u_ViewOrigin( this ),
	u_LightOrigin( this ),
	u_LightColor( this ),
	u_LightRadius( this ),
	u_LightScale( this ),
	u_LightAttenuationMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_UnprojectMatrix( this ),
	GLCompileMacro_USE_SHADOWING( this )
{
}

void GLShader_lightVolume_omni::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_AttenuationMapXY" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_AttenuationMapZ" ), 2 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowMap" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ShadowClipMap" ), 4 );
}

GLShader_liquid::GLShader_liquid( GLShaderManager *manager ) :
	GLShader( "liquid", ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT
		, manager ),
	u_NormalTextureMatrix( this ),
	u_ViewOrigin( this ),
	u_RefractionIndex( this ),
	u_ModelMatrix( this ),
	u_ModelViewProjectionMatrix( this ),
	u_UnprojectMatrix( this ),
	u_FresnelPower( this ),
	u_FresnelScale( this ),
	u_FresnelBias( this ),
	u_NormalScale( this ),
	u_FogDensity( this ),
	u_FogColor( this ),
	u_SpecularExponent( this ),
	u_LightGridOrigin( this ),
	u_LightGridScale( this ),
	GLCompileMacro_USE_PARALLAX_MAPPING( this )
{
}

void GLShader_liquid::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_CurrentMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_PortalMap" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 2 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_NormalMap" ), 3 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid1" ), 6 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_LightGrid2" ), 7 );
}

GLShader_volumetricFog::GLShader_volumetricFog( GLShaderManager *manager ) :
	GLShader( "volumetricFog", ATTR_POSITION, manager ),
	u_ViewOrigin( this ),
	u_UnprojectMatrix( this ),
	u_ModelViewMatrix( this ),
	u_FogDensity( this ),
	u_FogColor( this )
{
}

void GLShader_volumetricFog::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMapBack" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMapFront" ), 2 );
}

GLShader_motionblur::GLShader_motionblur( GLShaderManager *manager ) :
	GLShader( "motionblur", ATTR_POSITION, manager ),
	u_blurVec( this )
{
}

void GLShader_motionblur::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 1 );
}

GLShader_ssao::GLShader_ssao( GLShaderManager *manager ) :
	GLShader( "ssao", ATTR_POSITION, manager ),
	u_zFar( this )
{
}

void GLShader_ssao::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_DepthMap" ), 0 );
}

GLShader_fxaa::GLShader_fxaa( GLShaderManager *manager ) :
	GLShader( "fxaa", ATTR_POSITION, manager )
{
}

void GLShader_fxaa::SetShaderProgramUniforms( shaderProgram_t *shaderProgram )
{
	glUniform1i( glGetUniformLocation( shaderProgram->program, "u_ColorMap" ), 0 );
}

void GLShader_fxaa::BuildShaderFragmentLibNames( std::string& fragmentInlines )
{
	fragmentInlines += "fxaa3_11";
}
