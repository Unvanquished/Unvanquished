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

#ifndef GL_SHADER_H
#define GL_SHADER_H

#include <vector>
#include <string>

#include "tr_local.h"

// *INDENT-OFF*

class GLUniform;
class GLCompileMacro;

class GLShader
{
//friend class GLCompileMacro_USE_ALPHA_TESTING;

private:
	GLShader &operator             = ( const GLShader & );

	std::string                    _name;
protected:
	int                            _activeMacros;

	std::vector< shaderProgram_t > _shaderPrograms;
	shaderProgram_t                 *_currentProgram;

	std::vector< GLUniform * >      _uniforms;
	std::vector< GLCompileMacro * > _compileMacros;

	const uint32_t                 _vertexAttribsRequired;
//	const uint32_t           _vertexAttribsOptional;
//	const uint32_t           _vertexAttribsUnsupported;
	uint32_t                       _vertexAttribs; // can be set by uniforms

	GLShader( const std::string &name, uint32_t vertexAttribsRequired /*, uint32_t vertexAttribsOptional, uint32_t vertexAttribsUnsupported*/ ) :
		_name( name ),
		_activeMacros( 0 ),
		_currentProgram( NULL ),
		_vertexAttribsRequired( vertexAttribsRequired ),
		_vertexAttribs( 0 )
		//_vertexAttribsOptional(vertexAttribsOptional),
		//_vertexAttribsUnsupported(vertexAttribsUnsupported)
	{
		//ri.Printf(PRINT_ALL, "/// -------------------------------------------------\n");
	}

	~GLShader()
	{
		for ( std::size_t i = 0; i < _shaderPrograms.size(); i++ )
		{
			if ( _shaderPrograms[ i ].program )
			{
				glDeleteProgram( _shaderPrograms[ i ].program );
			}
		}
	}

public:

	void RegisterUniform( GLUniform *uniform )
	{
		_uniforms.push_back( uniform );
	}

	void RegisterCompileMacro( GLCompileMacro *compileMacro )
	{
		if ( _compileMacros.size() >= 9 )
		{
			ri.Error( ERR_DROP, "Can't register more than 9 compile macros for a single shader" );
		}

		_compileMacros.push_back( compileMacro );
	}

	size_t                          GetNumOfCompiledMacros() const
	{
		return _compileMacros.size();
	}

	shaderProgram_t        *GetProgram() const
	{
		return _currentProgram;
	}

protected:
	bool        GetCompileMacrosString( int permutation, std::string &compileMacrosOut ) const;
	void        UpdateShaderProgramUniformLocations( shaderProgram_t *shaderProgram ) const;

	std::string BuildGPUShaderText( const char *mainShader,
	                                const char *libShaders,
	                                GLenum shaderType ) const;

	void CompileAndLinkGPUShaderProgram( shaderProgram_t *program,
	                                     const char *programName,
	                                     const std::string &vertexShaderText,
	                                     const std::string &fragmentShaderText,
	                                     const std::string &compileMacros, int iteration ) const;

private:
	void CompileGPUShader( GLuint program, const char *programName, const char *shaderText, int shaderTextSize, GLenum shaderType ) const;
	void PrintShaderText( const std::string &shaderText ) const;
	void PrintShaderSource( GLuint object ) const;
	void PrintInfoLog( GLuint object, bool developerOnly ) const;

	void LinkProgram( GLuint program ) const;
	void BindAttribLocations( GLuint program ) const;

protected:
	void ValidateProgram( GLuint program ) const;
	void SaveShaderProgram( GLuint program, const char *pname, int i ) const;
	bool LoadShaderProgram( GLuint program, const char *pname, int i ) const;
	void ShowProgramUniforms( GLuint program ) const;

public:
	void SelectProgram();
	void BindProgram();
	void SetRequiredVertexPointers();

	bool IsMacroSet( int bit )
	{
		return ( _activeMacros & bit ) != 0;
	}

	void AddMacroBit( int bit )
	{
		_activeMacros |= bit;
	}

	void DelMacroBit( int bit )
	{
		_activeMacros &= ~bit;
	}

	bool IsVertexAtttribSet( int bit )
	{
		return ( _vertexAttribs & bit ) != 0;
	}

	void AddVertexAttribBit( int bit )
	{
		_vertexAttribs |= bit;
	}

	void DelVertexAttribBit( int bit )
	{
		_vertexAttribs &= ~bit;
	}
};

class GLUniform
{
protected:
	GLShader *_shader;

	GLUniform( GLShader *shader ) :
		_shader( shader )
	{
		_shader->RegisterUniform( this );
	}

public:
	virtual const char *GetName() const = 0;
	virtual void      UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const = 0;

	virtual ~GLUniform() {}
};

class GLCompileMacro
{
private:
	int     _bit;

protected:
	GLShader *_shader;

	GLCompileMacro( GLShader *shader ) :
		_shader( shader )
	{
		_bit = BIT( _shader->GetNumOfCompiledMacros() );
		_shader->RegisterCompileMacro( this );
	}

// RB: This is not good oo design, but it can be a workaround and its cost is more or less only a virtual function call.
// It also works regardless of RTTI is enabled or not.
	enum EGLCompileMacro
	{
	  USE_ALPHA_TESTING,
	  USE_PORTAL_CLIPPING,
	  USE_FRUSTUM_CLIPPING,
	  USE_VERTEX_SKINNING,
	  USE_VERTEX_ANIMATION,
	  USE_DEFORM_VERTEXES,
	  USE_TCGEN_ENVIRONMENT,
	  USE_TCGEN_LIGHTMAP,
	  USE_NORMAL_MAPPING,
	  USE_PARALLAX_MAPPING,
	  USE_REFLECTIVE_SPECULAR,
	  USE_SHADOWING,
	  TWOSIDED,
	  EYE_OUTSIDE,
	  BRIGHTPASS_FILTER,
	  LIGHT_DIRECTIONAL,
	  USE_GBUFFER
	};

public:
	virtual const char       *GetName() const = 0;
	virtual EGLCompileMacro GetType() const = 0;

	virtual bool            HasConflictingMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const
	{
		return false;
	}

	virtual bool            MissesRequiredMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const
	{
		return false;
	}

	virtual uint32_t        GetRequiredVertexAttributes() const
	{
		return 0;
	}

	void EnableMacro()
	{
		int bit = GetBit();

		if ( !_shader->IsMacroSet( bit ) )
		{
			_shader->AddMacroBit( bit );
			//_shader->SelectProgram();
		}
	}

	void DisableMacro()
	{
		int bit = GetBit();

		if ( _shader->IsMacroSet( bit ) )
		{
			_shader->DelMacroBit( bit );
			//_shader->SelectProgram();
		}
	}

public:
	int GetBit() const
	{
		return _bit;
	}

	virtual ~GLCompileMacro() {}
};

class GLCompileMacro_USE_ALPHA_TESTING :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_ALPHA_TESTING( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_ALPHA_TESTING";
	}

	EGLCompileMacro GetType() const
	{
		return USE_ALPHA_TESTING;
	}

	void EnableAlphaTesting()
	{
		EnableMacro();
	}

	void DisableAlphaTesting()
	{
		DisableMacro();
	}

	void SetAlphaTesting( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_USE_PORTAL_CLIPPING :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_PORTAL_CLIPPING( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_PORTAL_CLIPPING";
	}

	EGLCompileMacro GetType() const
	{
		return USE_PORTAL_CLIPPING;
	}

	void EnablePortalClipping()
	{
		EnableMacro();
	}

	void DisablePortalClipping()
	{
		DisableMacro();
	}

	void SetPortalClipping( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_USE_FRUSTUM_CLIPPING :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_FRUSTUM_CLIPPING( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_FRUSTUM_CLIPPING";
	}

	EGLCompileMacro GetType() const
	{
		return USE_FRUSTUM_CLIPPING;
	}

	void EnableFrustumClipping()
	{
		EnableMacro();
	}

	void DisableFrustumClipping()
	{
		DisableMacro();
	}

	void SetFrustumClipping( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_USE_VERTEX_SKINNING :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_VERTEX_SKINNING( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_VERTEX_SKINNING";
	}

	EGLCompileMacro GetType() const
	{
		return USE_VERTEX_SKINNING;
	}

	bool HasConflictingMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const;
	bool MissesRequiredMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const;

	uint32_t        GetRequiredVertexAttributes() const
	{
		return ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;
	}

	void EnableVertexSkinning()
	{
		EnableMacro();
	}

	void DisableVertexSkinning()
	{
		DisableMacro();
	}

	void SetVertexSkinning( bool enable )
	{
		if ( enable )
		{
			EnableVertexSkinning();
		}
		else
		{
			DisableVertexSkinning();
		}
	}
};

class GLCompileMacro_USE_VERTEX_ANIMATION :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_VERTEX_ANIMATION( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_VERTEX_ANIMATION";
	}

	EGLCompileMacro GetType() const
	{
		return USE_VERTEX_ANIMATION;
	}

	bool     HasConflictingMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const;
	uint32_t GetRequiredVertexAttributes() const;

	void EnableVertexAnimation()
	{
		EnableMacro();
	}

	void DisableVertexAnimation()
	{
		DisableMacro();
	}

	void SetVertexAnimation( bool enable )
	{
		if ( enable )
		{
			EnableVertexAnimation();
		}
		else
		{
			DisableVertexAnimation();
		}
	}
};

class GLCompileMacro_USE_DEFORM_VERTEXES :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_DEFORM_VERTEXES( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_DEFORM_VERTEXES";
	}

	EGLCompileMacro GetType() const
	{
		return USE_DEFORM_VERTEXES;
	}

	bool HasConflictingMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const;

	uint32_t        GetRequiredVertexAttributes() const
	{
		return ATTR_NORMAL;
	}

	void EnableDeformVertexes()
	{
		if ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}

	void DisableDeformVertexes()
	{
		DisableMacro();
	}

	void SetDeformVertexes( bool enable )
	{
		if ( enable && ( glConfig.driverType == GLDRV_OPENGL3 && r_vboDeformVertexes->integer ) )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_USE_TCGEN_ENVIRONMENT :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_TCGEN_ENVIRONMENT( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_TCGEN_ENVIRONMENT";
	}

	EGLCompileMacro GetType() const
	{
		return USE_TCGEN_ENVIRONMENT;
	}

	uint32_t        GetRequiredVertexAttributes() const
	{
		return ATTR_NORMAL;
	}

	void EnableTCGenEnvironment()
	{
		EnableMacro();
	}

	void DisableTCGenEnvironment()
	{
		DisableMacro();
	}

	void SetTCGenEnvironment( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_USE_TCGEN_LIGHTMAP :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_TCGEN_LIGHTMAP( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_TCGEN_LIGHTMAP";
	}

	EGLCompileMacro GetType() const
	{
		return USE_TCGEN_LIGHTMAP;
	}

	uint32_t        GetRequiredVertexAttributes() const
	{
		return ATTR_LIGHTCOORD;
	}

	void EnableTCGenLightmap()
	{
		EnableMacro();
	}

	void DisableTCGenLightmap()
	{
		DisableMacro();
	}

	void SetTCGenLightmap( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_USE_NORMAL_MAPPING :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_NORMAL_MAPPING( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_NORMAL_MAPPING";
	}

	EGLCompileMacro GetType() const
	{
		return USE_NORMAL_MAPPING;
	}

	uint32_t        GetRequiredVertexAttributes() const
	{
		return ATTR_NORMAL | ATTR_TANGENT | ATTR_BINORMAL;
	}

	void EnableNormalMapping()
	{
		EnableMacro();
	}

	void DisableNormalMapping()
	{
		DisableMacro();
	}

	void SetNormalMapping( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_USE_PARALLAX_MAPPING :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_PARALLAX_MAPPING( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_PARALLAX_MAPPING";
	}

	EGLCompileMacro GetType() const
	{
		return USE_PARALLAX_MAPPING;
	}

	bool MissesRequiredMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const;

	void EnableParallaxMapping()
	{
		EnableMacro();
	}

	void DisableParallaxMapping()
	{
		DisableMacro();
	}

	void SetParallaxMapping( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_USE_REFLECTIVE_SPECULAR :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_REFLECTIVE_SPECULAR( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_REFLECTIVE_SPECULAR";
	}

	EGLCompileMacro GetType() const
	{
		return USE_REFLECTIVE_SPECULAR;
	}

	bool MissesRequiredMacros( int permutation, const std::vector< GLCompileMacro * > &macros ) const;

	void EnableReflectiveSpecular()
	{
		EnableMacro();
	}

	void DisableReflectiveSpecular()
	{
		DisableMacro();
	}

	void SetReflectiveSpecular( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_TWOSIDED :
	GLCompileMacro
{
public:
	GLCompileMacro_TWOSIDED( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "TWOSIDED";
	}

	EGLCompileMacro GetType() const
	{
		return TWOSIDED;
	}

//bool    MissesRequiredMacros(int permutation, const std::vector<GLCompileMacro*>& macros) const;
	uint32_t        GetRequiredVertexAttributes() const
	{
		return ATTR_NORMAL;
	}

	void EnableMacro_TWOSIDED()
	{
		EnableMacro();
	}

	void DisableMacro_TWOSIDED()
	{
		DisableMacro();
	}

	void SetMacro_TWOSIDED( cullType_t cullType )
	{
		if ( cullType == CT_TWO_SIDED || cullType == CT_BACK_SIDED )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_EYE_OUTSIDE :
	GLCompileMacro
{
public:
	GLCompileMacro_EYE_OUTSIDE( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "EYE_OUTSIDE";
	}

	EGLCompileMacro GetType() const
	{
		return EYE_OUTSIDE;
	}

	void EnableMacro_EYE_OUTSIDE()
	{
		EnableMacro();
	}

	void DisableMacro_EYE_OUTSIDE()
	{
		DisableMacro();
	}

	void SetMacro_EYE_OUTSIDE( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_BRIGHTPASS_FILTER :
	GLCompileMacro
{
public:
	GLCompileMacro_BRIGHTPASS_FILTER( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "BRIGHTPASS_FILTER";
	}

	EGLCompileMacro GetType() const
	{
		return BRIGHTPASS_FILTER;
	}

	void EnableMacro_BRIGHTPASS_FILTER()
	{
		EnableMacro();
	}

	void DisableMacro_BRIGHTPASS_FILTER()
	{
		DisableMacro();
	}

	void SetMacro_BRIGHTPASS_FILTER( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_LIGHT_DIRECTIONAL :
	GLCompileMacro
{
public:
	GLCompileMacro_LIGHT_DIRECTIONAL( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "LIGHT_DIRECTIONAL";
	}

	EGLCompileMacro GetType() const
	{
		return LIGHT_DIRECTIONAL;
	}

	void EnableMacro_LIGHT_DIRECTIONAL()
	{
		EnableMacro();
	}

	void DisableMacro_LIGHT_DIRECTIONAL()
	{
		DisableMacro();
	}

	void SetMacro_LIGHT_DIRECTIONAL( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_USE_SHADOWING :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_SHADOWING( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_SHADOWING";
	}

	EGLCompileMacro GetType() const
	{
		return USE_SHADOWING;
	}

	void EnableShadowing()
	{
		EnableMacro();
	}

	void DisableShadowing()
	{
		DisableMacro();
	}

	void SetShadowing( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class GLCompileMacro_USE_GBUFFER :
	GLCompileMacro
{
public:
	GLCompileMacro_USE_GBUFFER( GLShader *shader ) :
		GLCompileMacro( shader )
	{
	}

	const char *GetName() const
	{
		return "USE_GBUFFER";
	}

	EGLCompileMacro GetType() const
	{
		return USE_GBUFFER;
	}

	void EnableMacro_USE_GBUFFER()
	{
		EnableMacro();
	}

	void DisableMacro_USE_GBUFFER()
	{
		DisableMacro();
	}

	void SetMacro_USE_GBUFFER( bool enable )
	{
		if ( enable )
		{
			EnableMacro();
		}
		else
		{
			DisableMacro();
		}
	}
};

class u_ColorMap :
	GLUniform
{
public:
	u_ColorMap( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ColorMap";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ColorMap = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ColorMap( int texUnit )
	{
		glUniform1i( _shader->GetProgram()->u_ColorMap, texUnit );
	}
};

class u_NormalMap :
	GLUniform
{
public:
	u_NormalMap( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_NormalMap";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_NormalMap = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_NormalMap( int texUnit )
	{
		glUniform1i( _shader->GetProgram()->u_NormalMap, texUnit );
	}
};

class u_CurrentMap :
	GLUniform
{
public:
	u_CurrentMap( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_CurrentMap";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_CurrentMap = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_CurrentMap( int texUnit )
	{
		glUniform1i( _shader->GetProgram()->u_CurrentMap, texUnit );
	}
};

class u_ColorTextureMatrix :
	GLUniform
{
public:
	u_ColorTextureMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ColorTextureMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ColorTextureMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ColorTextureMatrix( const matrix_t m )
	{
		GLSL_SetUniform_ColorTextureMatrix( _shader->GetProgram(), m );
	}
};

class u_DiffuseTextureMatrix :
	GLUniform
{
public:
	u_DiffuseTextureMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_DiffuseTextureMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_DiffuseTextureMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_DiffuseTextureMatrix( const matrix_t m )
	{
		GLSL_SetUniform_DiffuseTextureMatrix( _shader->GetProgram(), m );
	}
};

class u_NormalTextureMatrix :
	GLUniform
{
public:
	u_NormalTextureMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_NormalTextureMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_NormalTextureMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_NormalTextureMatrix( const matrix_t m )
	{
		GLSL_SetUniform_NormalTextureMatrix( _shader->GetProgram(), m );
	}
};

class u_SpecularTextureMatrix :
	GLUniform
{
public:
	u_SpecularTextureMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_SpecularTextureMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_SpecularTextureMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_SpecularTextureMatrix( const matrix_t m )
	{
		GLSL_SetUniform_SpecularTextureMatrix( _shader->GetProgram(), m );
	}
};

class u_AlphaTest :
	GLUniform
{
public:
	u_AlphaTest( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_AlphaTest";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_AlphaTest = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_AlphaTest( uint32_t stateBits )
	{
		GLSL_SetUniform_AlphaTest( _shader->GetProgram(), stateBits );
	}
};

class u_AmbientColor :
	GLUniform
{
public:
	u_AmbientColor( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_AmbientColor";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_AmbientColor = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_AmbientColor( const vec3_t v )
	{
		GLSL_SetUniform_AmbientColor( _shader->GetProgram(), v );
	}
};

class u_ViewOrigin :
	GLUniform
{
public:
	u_ViewOrigin( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ViewOrigin";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ViewOrigin = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ViewOrigin( const vec3_t v )
	{
		GLSL_SetUniform_ViewOrigin( _shader->GetProgram(), v );
	}
};

class u_LightDir :
	GLUniform
{
public:
	u_LightDir( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_LightDir";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_LightDir = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_LightDir( const vec3_t v )
	{
		GLSL_SetUniform_LightDir( _shader->GetProgram(), v );
	}
};

class u_LightOrigin :
	GLUniform
{
public:
	u_LightOrigin( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_LightOrigin";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_LightOrigin = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_LightOrigin( const vec3_t v )
	{
		GLSL_SetUniform_LightOrigin( _shader->GetProgram(), v );
	}
};

class u_LightColor :
	GLUniform
{
public:
	u_LightColor( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_LightColor";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_LightColor = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_LightColor( const vec3_t v )
	{
		GLSL_SetUniform_LightColor( _shader->GetProgram(), v );
	}
};

class u_LightRadius :
	GLUniform
{
public:
	u_LightRadius( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_LightRadius";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_LightRadius = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_LightRadius( float value )
	{
		GLSL_SetUniform_LightRadius( _shader->GetProgram(), value );
	}
};

class u_LightScale :
	GLUniform
{
public:
	u_LightScale( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_LightScale";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_LightScale = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_LightScale( float value )
	{
		GLSL_SetUniform_LightScale( _shader->GetProgram(), value );
	}
};

class u_LightWrapAround :
	GLUniform
{
public:
	u_LightWrapAround( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_LightWrapAround";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_LightWrapAround = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_LightWrapAround( float value )
	{
		GLSL_SetUniform_LightWrapAround( _shader->GetProgram(), value );
	}
};

class u_LightAttenuationMatrix :
	GLUniform
{
public:
	u_LightAttenuationMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_LightAttenuationMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_LightAttenuationMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_LightAttenuationMatrix( const matrix_t m )
	{
		GLSL_SetUniform_LightAttenuationMatrix( _shader->GetProgram(), m );
	}
};

class u_LightFrustum :
	GLUniform
{
public:
	u_LightFrustum( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_LightFrustum";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_LightFrustum = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_LightFrustum( vec4_t lightFrustum[ 6 ] )
	{
		shaderProgram_t *program = _shader->GetProgram();

#if 0

		if ( memcmp( program->t_LightFrustum, m ) )
		{
			return;
		}

#endif

#if defined( LOG_GLSL_UNIFORMS )

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "--- SetUniform_LightFrustum( program = %s, "
			                      "( %5.3f, %5.3f, %5.3f, %5.3f )\n"
			                      "( %5.3f, %5.3f, %5.3f, %5.3f )\n"
			                      "( %5.3f, %5.3f, %5.3f, %5.3f )\n"
			                      "( %5.3f, %5.3f, %5.3f, %5.3f )\n"
			                      "( %5.3f, %5.3f, %5.3f, %5.3f )\n"
			                      "( %5.3f, %5.3f, %5.3f, %5.3f ) ) ---\n",
			                      program->name,
			                      lightFrustum[ 0 ][ 0 ], lightFrustum[ 0 ][ 1 ], lightFrustum[ 0 ][ 2 ], lightFrustum[ 0 ][ 3 ],
			                      lightFrustum[ 1 ][ 0 ], lightFrustum[ 1 ][ 1 ], lightFrustum[ 1 ][ 2 ], lightFrustum[ 1 ][ 3 ],
			                      lightFrustum[ 2 ][ 0 ], lightFrustum[ 2 ][ 1 ], lightFrustum[ 2 ][ 2 ], lightFrustum[ 2 ][ 3 ],
			                      lightFrustum[ 3 ][ 0 ], lightFrustum[ 3 ][ 1 ], lightFrustum[ 3 ][ 2 ], lightFrustum[ 3 ][ 3 ],
			                      lightFrustum[ 4 ][ 0 ], lightFrustum[ 4 ][ 1 ], lightFrustum[ 4 ][ 2 ], lightFrustum[ 4 ][ 3 ],
			                      lightFrustum[ 5 ][ 0 ], lightFrustum[ 5 ][ 1 ], lightFrustum[ 5 ][ 2 ], lightFrustum[ 5 ][ 3 ]
			                    ) );
		}

#endif

		glUniform4fv( program->u_LightFrustum, 6, &lightFrustum[ 0 ][ 0 ] );
	}
};

class u_ShadowTexelSize :
	GLUniform
{
public:
	u_ShadowTexelSize( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ShadowTexelSize";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ShadowTexelSize = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ShadowTexelSize( float value )
	{
		GLSL_SetUniform_ShadowTexelSize( _shader->GetProgram(), value );
	}
};

class u_ShadowBlur :
	GLUniform
{
public:
	u_ShadowBlur( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ShadowBlur";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ShadowBlur = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ShadowBlur( float value )
	{
		GLSL_SetUniform_ShadowBlur( _shader->GetProgram(), value );
	}
};

class u_ShadowMatrix :
	GLUniform
{
public:
	u_ShadowMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ShadowMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ShadowMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ShadowMatrix( matrix_t m[ MAX_SHADOWMAPS ] )
	{
		GLSL_SetUniform_ShadowMatrix( _shader->GetProgram(), m );
	}
};

class u_ShadowParallelSplitDistances :
	GLUniform
{
public:
	u_ShadowParallelSplitDistances( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ShadowParallelSplitDistances";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ShadowParallelSplitDistances = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ShadowParallelSplitDistances( const vec4_t v )
	{
		GLSL_SetUniform_ShadowParallelSplitDistances( _shader->GetProgram(), v );
	}
};

class u_Color :
	GLUniform
{
public:
	u_Color( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_Color";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_Color = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_Color( const vec4_t v )
	{
		GLSL_SetUniform_Color( _shader->GetProgram(), v );
	}
};

class u_ModelMatrix :
	GLUniform
{
public:
	u_ModelMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ModelMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ModelMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ModelMatrix( const matrix_t m )
	{
		GLSL_SetUniform_ModelMatrix( _shader->GetProgram(), m );
	}
};

class u_ViewMatrix :
	GLUniform
{
public:
	u_ViewMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ViewMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ViewMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ViewMatrix( const matrix_t m )
	{
		GLSL_SetUniform_ViewMatrix( _shader->GetProgram(), m );
	}
};

class u_ModelViewMatrix :
	GLUniform
{
public:
	u_ModelViewMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ModelViewMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ModelViewMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ModelViewMatrix( const matrix_t m )
	{
		GLSL_SetUniform_ModelViewMatrix( _shader->GetProgram(), m );
	}
};

class u_ModelViewMatrixTranspose :
	GLUniform
{
public:
	u_ModelViewMatrixTranspose( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ModelViewMatrixTranspose";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ModelViewMatrixTranspose = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ModelViewMatrixTranspose( const matrix_t m )
	{
		GLSL_SetUniform_ModelViewMatrixTranspose( _shader->GetProgram(), m );
	}
};

class u_ProjectionMatrixTranspose :
	GLUniform
{
public:
	u_ProjectionMatrixTranspose( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ProjectionMatrixTranspose";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ProjectionMatrixTranspose = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ProjectionMatrixTranspose( const matrix_t m )
	{
		GLSL_SetUniform_ProjectionMatrixTranspose( _shader->GetProgram(), m );
	}
};

class u_ModelViewProjectionMatrix :
	GLUniform
{
public:
	u_ModelViewProjectionMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ModelViewProjectionMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ModelViewProjectionMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ModelViewProjectionMatrix( const matrix_t m )
	{
		GLSL_SetUniform_ModelViewProjectionMatrix( _shader->GetProgram(), m );
	}
};

class u_UnprojectMatrix :
	GLUniform
{
public:
	u_UnprojectMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_UnprojectMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_UnprojectMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_UnprojectMatrix( const matrix_t m )
	{
		GLSL_SetUniform_UnprojectMatrix( _shader->GetProgram(), m );
	}
};

class u_BoneMatrix :
	GLUniform
{
public:
	u_BoneMatrix( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_BoneMatrix";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_BoneMatrix = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_BoneMatrix( int numBones, const matrix_t boneMatrices[ MAX_BONES ] )
	{
		glUniformMatrix4fv( _shader->GetProgram()->u_BoneMatrix, numBones, GL_FALSE, &boneMatrices[ 0 ][ 0 ] );
	}
};

class u_VertexInterpolation :
	GLUniform
{
public:
	u_VertexInterpolation( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_VertexInterpolation";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_VertexInterpolation = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_VertexInterpolation( float value )
	{
		GLSL_SetUniform_VertexInterpolation( _shader->GetProgram(), value );
	}
};

class u_PortalPlane :
	GLUniform
{
public:
	u_PortalPlane( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_PortalPlane";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_PortalPlane = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_PortalPlane( const vec4_t v )
	{
		GLSL_SetUniform_PortalPlane( _shader->GetProgram(), v );
	}
};

class u_PortalRange :
	GLUniform
{
public:
	u_PortalRange( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_PortalRange";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_PortalRange = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_PortalRange( float value )
	{
		GLSL_SetUniform_PortalRange( _shader->GetProgram(), value );
	}
};

class u_DepthScale :
	GLUniform
{
public:
	u_DepthScale( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_DepthScale";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_DepthScale = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_DepthScale( float value )
	{
		GLSL_SetUniform_DepthScale( _shader->GetProgram(), value );
	}
};

class u_EnvironmentInterpolation :
	GLUniform
{
public:
	u_EnvironmentInterpolation( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_EnvironmentInterpolation";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_EnvironmentInterpolation = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_EnvironmentInterpolation( float value )
	{
		GLSL_SetUniform_EnvironmentInterpolation( _shader->GetProgram(), value );
	}
};

class u_DeformParms :
	GLUniform
{
public:
	u_DeformParms( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_DeformParms";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_DeformParms = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_DeformParms( deformStage_t deforms[ MAX_SHADER_DEFORMS ], int numDeforms )
	{
		float deformParms[ MAX_SHADER_DEFORM_PARMS ];
		int   deformOfs = 0;

		if ( numDeforms > MAX_SHADER_DEFORMS )
		{
			numDeforms = MAX_SHADER_DEFORMS;
		}

		deformParms[ deformOfs++ ] = numDeforms;

		for ( int i = 0; i < numDeforms; i++ )
		{
			deformStage_t *ds = &deforms[ i ];

			switch ( ds->deformation )
			{
				case DEFORM_WAVE:
					deformParms[ deformOfs++ ] = DEFORM_WAVE;

					deformParms[ deformOfs++ ] = ds->deformationWave.func;
					deformParms[ deformOfs++ ] = ds->deformationWave.base;
					deformParms[ deformOfs++ ] = ds->deformationWave.amplitude;
					deformParms[ deformOfs++ ] = ds->deformationWave.phase;
					deformParms[ deformOfs++ ] = ds->deformationWave.frequency;

					deformParms[ deformOfs++ ] = ds->deformationSpread;
					break;

				case DEFORM_BULGE:
					deformParms[ deformOfs++ ] = DEFORM_BULGE;

					deformParms[ deformOfs++ ] = ds->bulgeWidth;
					deformParms[ deformOfs++ ] = ds->bulgeHeight;
					deformParms[ deformOfs++ ] = ds->bulgeSpeed;
					break;

				case DEFORM_MOVE:
					deformParms[ deformOfs++ ] = DEFORM_MOVE;

					deformParms[ deformOfs++ ] = ds->deformationWave.func;
					deformParms[ deformOfs++ ] = ds->deformationWave.base;
					deformParms[ deformOfs++ ] = ds->deformationWave.amplitude;
					deformParms[ deformOfs++ ] = ds->deformationWave.phase;
					deformParms[ deformOfs++ ] = ds->deformationWave.frequency;

					deformParms[ deformOfs++ ] = ds->bulgeWidth;
					deformParms[ deformOfs++ ] = ds->bulgeHeight;
					deformParms[ deformOfs++ ] = ds->bulgeSpeed;
					break;

				default:
					break;
			}

			glUniform1fv( _shader->GetProgram()->u_DeformParms, MAX_SHADER_DEFORM_PARMS, deformParms );
		}
	}
};

class u_Time :
	GLUniform
{
public:
	u_Time( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_Time";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_Time = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_Time( float value )
	{
		GLSL_SetUniform_Time( _shader->GetProgram(), value );
	}
};

class GLDeformStage :
	public u_DeformParms,
	public u_Time
{
public:
	GLDeformStage( GLShader *shader ) :
		u_DeformParms( shader ),
		u_Time( shader )
	{
	}
};

class u_ColorModulate :
	GLUniform
{
public:
	u_ColorModulate( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_ColorModulate";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_ColorModulate = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_ColorModulate( colorGen_t colorGen, alphaGen_t alphaGen )
	{
		vec4_t v;

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "--- u_ColorModulate::SetUniform_ColorModulate( program = %s, colorGen = %i, alphaGen = %i ) ---\n", _shader->GetProgram()->name, colorGen, alphaGen ) );
		}

		switch ( colorGen )
		{
			case CGEN_VERTEX:
				_shader->AddVertexAttribBit( ATTR_COLOR );
				VectorSet( v, 1, 1, 1 );
				break;

			case CGEN_ONE_MINUS_VERTEX:
				_shader->AddVertexAttribBit( ATTR_COLOR );
				VectorSet( v, -1, -1, -1 );
				break;

			default:
				_shader->DelVertexAttribBit( ATTR_COLOR );
				VectorSet( v, 0, 0, 0 );
				break;
		}

		switch ( alphaGen )
		{
			case AGEN_VERTEX:
				_shader->AddVertexAttribBit( ATTR_COLOR );
				v[ 3 ] = 1.0f;
				break;

			case AGEN_ONE_MINUS_VERTEX:
				_shader->AddVertexAttribBit( ATTR_COLOR );
				v[ 3 ] = -1.0f;
				break;

			default:
				v[ 3 ] = 0.0f;
				break;
		}

		GLSL_SetUniform_ColorModulate( _shader->GetProgram(), v );
	}
};

class u_FogDistanceVector :
	GLUniform
{
public:
	u_FogDistanceVector( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_FogDistanceVector";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_FogDistanceVector = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_FogDistanceVector( const vec4_t v )
	{
		shaderProgram_t *program = _shader->GetProgram();

#if defined( USE_UNIFORM_FIREWALL )

		if ( Vector4Compare( program->t_FogDistanceVector, v ) )
		{
			return;
		}

		VectorCopy( v, program->t_FogDistanceVector );
#endif

#if defined( LOG_GLSL_UNIFORMS )

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "--- SetUniform_FogDistanceVector( program = %s, vector = ( %5.3f, %5.3f, %5.3f, %5.3f ) ) ---\n", program->name, v[ 0 ], v[ 1 ], v[ 2 ], v[ 3 ] ) );
		}

#endif

		glUniform4f( program->u_FogDistanceVector, v[ 0 ], v[ 1 ], v[ 2 ], v[ 3 ] );
	}
};

class u_FogDepthVector :
	GLUniform
{
public:
	u_FogDepthVector( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_FogDepthVector";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_FogDepthVector = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_FogDepthVector( const vec4_t v )
	{
		shaderProgram_t *program = _shader->GetProgram();

#if defined( USE_UNIFORM_FIREWALL )

		if ( Vector4Compare( program->t_FogDepthVector, v ) )
		{
			return;
		}

		VectorCopy( v, program->t_FogDepthVector );
#endif

#if defined( LOG_GLSL_UNIFORMS )

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "--- SetUniform_FogDepthVector( program = %s, vector = ( %5.3f, %5.3f, %5.3f, %5.3f ) ) ---\n", program->name, v[ 0 ], v[ 1 ], v[ 2 ], v[ 3 ] ) );
		}

#endif

		glUniform4f( program->u_FogDepthVector, v[ 0 ], v[ 1 ], v[ 2 ], v[ 3 ] );
	}
};

class u_FogEyeT :
	GLUniform
{
public:
	u_FogEyeT( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_FogEyeT";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_FogEyeT = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_FogEyeT( float value )
	{
		shaderProgram_t *program = _shader->GetProgram();

#if defined( USE_UNIFORM_FIREWALL )

		if ( program->t_FogEyeT == value )
		{
			return;
		}

		program->t_FogEyeT = value;
#endif

#if defined( LOG_GLSL_UNIFORMS )

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "--- SetUniform_FogEyeT( program = %s, value = %f ) ---\n", program->name, value ) );
		}

#endif

		glUniform1f( program->u_FogEyeT, value );
	}
};

class u_DeformMagnitude :
	GLUniform
{
public:
	u_DeformMagnitude( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_DeformMagnitude";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_DeformMagnitude = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_DeformMagnitude( float value )
	{
		shaderProgram_t *program = _shader->GetProgram();

#if defined( USE_UNIFORM_FIREWALL )

		if ( program->t_DeformMagnitude == value )
		{
			return;
		}

		program->t_DeformMagnitude = value;
#endif

#if defined( LOG_GLSL_UNIFORMS )

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "--- SetUniform_DeformMagnitude( program = %s, value = %f ) ---\n", program->name, value ) );
		}

#endif

		glUniform1f( program->u_DeformMagnitude, value );
	}
};

class u_HDRKey :
	GLUniform
{
public:
	u_HDRKey( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_HDRKey";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_HDRKey = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_HDRKey( float value )
	{
		shaderProgram_t *program = _shader->GetProgram();

#if defined( USE_UNIFORM_FIREWALL )

		if ( program->t_HDRKey == value )
		{
			return;
		}

		program->t_HDRKey = value;
#endif

#if defined( LOG_GLSL_UNIFORMS )

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "--- SetUniform_HDRKey( program = %s, value = %f ) ---\n", program->name, value ) );
		}

#endif

		glUniform1f( program->u_HDRKey, value );
	}
};

class u_HDRAverageLuminance :
	GLUniform
{
public:
	u_HDRAverageLuminance( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_HDRAverageLuminance";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_HDRAverageLuminance = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_HDRAverageLuminance( float value )
	{
		shaderProgram_t *program = _shader->GetProgram();

#if defined( USE_UNIFORM_FIREWALL )

		if ( program->t_HDRAverageLuminance == value )
		{
			return;
		}

		program->t_HDRAverageLuminance = value;
#endif

#if defined( LOG_GLSL_UNIFORMS )

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "--- SetUniform_HDRAverageLuminance( program = %s, value = %f ) ---\n", program->name, value ) );
		}

#endif

		glUniform1f( program->u_HDRAverageLuminance, value );
	}
};

class u_HDRMaxLuminance :
	GLUniform
{
public:
	u_HDRMaxLuminance( GLShader *shader ) :
		GLUniform( shader )
	{
	}

	const char *GetName() const
	{
		return "u_HDRMaxLuminance";
	}

	void                            UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram ) const
	{
		shaderProgram->u_HDRMaxLuminance = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	void SetUniform_HDRMaxLuminance( float value )
	{
		shaderProgram_t *program = _shader->GetProgram();

#if defined( USE_UNIFORM_FIREWALL )

		if ( program->t_HDRMaxLuminance == value )
		{
			return;
		}

		program->t_HDRMaxLuminance = value;
#endif

#if defined( LOG_GLSL_UNIFORMS )

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "--- SetUniform_HDRMaxLuminance( program = %s, value = %f ) ---\n", program->name, value ) );
		}

#endif

		glUniform1f( program->u_HDRMaxLuminance, value );
	}
};

class GLShader_generic :
	public GLShader,
	public u_ColorMap,
	public u_ColorTextureMatrix,
	public u_ViewOrigin,
	public u_AlphaTest,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_ColorModulate,
	public u_Color,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_PortalPlane,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_ALPHA_TESTING,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_TCGEN_ENVIRONMENT,
	public GLCompileMacro_USE_TCGEN_LIGHTMAP
{
public:
	GLShader_generic();
};

class GLShader_lightMapping :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_ColorModulate,
	public u_Color,
	public u_AlphaTest,
	public u_ViewOrigin,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_PortalPlane,
	public u_DepthScale,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_ALPHA_TESTING,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_lightMapping();
};

class GLShader_vertexLighting_DBS_entity :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_AlphaTest,
	public u_AmbientColor,
	public u_ViewOrigin,
	public u_LightDir,
	public u_LightColor,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_PortalPlane,
	public u_DepthScale,
	public u_EnvironmentInterpolation,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_ALPHA_TESTING,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING,
	public GLCompileMacro_USE_REFLECTIVE_SPECULAR //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_vertexLighting_DBS_entity();
};

class GLShader_vertexLighting_DBS_world :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_ColorModulate,
	public u_Color,
	public u_AlphaTest,
	public u_ViewOrigin,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_PortalPlane,
	public u_DepthScale,
	public u_LightWrapAround,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_ALPHA_TESTING,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING //,
//public GLCompileMacro_TWOSIDED
//public GLCompileMacro_USE_GBUFFER
{
public:
	GLShader_vertexLighting_DBS_world();
};

class GLShader_forwardLighting_omniXYZ :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_AlphaTest,
	public u_ColorModulate,
	public u_Color,
	public u_ViewOrigin,
	public u_LightOrigin,
	public u_LightColor,
	public u_LightRadius,
	public u_LightScale,
	public u_LightWrapAround,
	public u_LightAttenuationMatrix,
	public u_ShadowTexelSize,
	public u_ShadowBlur,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_PortalPlane,
	public u_DepthScale,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_ALPHA_TESTING,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_forwardLighting_omniXYZ();
};

class GLShader_forwardLighting_projXYZ :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_AlphaTest,
	public u_ColorModulate,
	public u_Color,
	public u_ViewOrigin,
	public u_LightOrigin,
	public u_LightColor,
	public u_LightRadius,
	public u_LightScale,
	public u_LightWrapAround,
	public u_LightAttenuationMatrix,
	public u_ShadowTexelSize,
	public u_ShadowBlur,
	public u_ShadowMatrix,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_PortalPlane,
	public u_DepthScale,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_ALPHA_TESTING,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_forwardLighting_projXYZ();
};

class GLShader_forwardLighting_directionalSun :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_AlphaTest,
	public u_ColorModulate,
	public u_Color,
	public u_ViewOrigin,
	public u_LightDir,
	public u_LightColor,
	public u_LightRadius,
	public u_LightScale,
	public u_LightWrapAround,
	public u_LightAttenuationMatrix,
	public u_ShadowTexelSize,
	public u_ShadowBlur,
	public u_ShadowMatrix,
	public u_ShadowParallelSplitDistances,
	public u_ModelMatrix,
	public u_ViewMatrix,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_PortalPlane,
	public u_DepthScale,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_ALPHA_TESTING,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_forwardLighting_directionalSun();
};

class GLShader_deferredLighting_omniXYZ :
	public GLShader,
	public u_ViewOrigin,
	public u_LightOrigin,
	public u_LightColor,
	public u_LightRadius,
	public u_LightScale,
	public u_LightWrapAround,
	public u_LightAttenuationMatrix,
	public u_LightFrustum,
	public u_ShadowTexelSize,
	public u_ShadowBlur,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_UnprojectMatrix,
	public u_PortalPlane,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_FRUSTUM_CLIPPING,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_deferredLighting_omniXYZ();
};

class GLShader_deferredLighting_projXYZ :
	public GLShader,
	public u_ViewOrigin,
	public u_LightOrigin,
	public u_LightColor,
	public u_LightRadius,
	public u_LightScale,
	public u_LightWrapAround,
	public u_LightAttenuationMatrix,
	public u_LightFrustum,
	public u_ShadowTexelSize,
	public u_ShadowBlur,
	public u_ShadowMatrix,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_UnprojectMatrix,
	public u_PortalPlane,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_FRUSTUM_CLIPPING,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_deferredLighting_projXYZ();
};

class GLShader_deferredLighting_directionalSun :
	public GLShader,
	public u_ViewOrigin,
	public u_LightDir,
	public u_LightColor,
	public u_LightRadius,
	public u_LightScale,
	public u_LightWrapAround,
	public u_LightAttenuationMatrix,
	public u_LightFrustum,
	public u_ShadowTexelSize,
	public u_ShadowBlur,
	public u_ShadowMatrix,
	public u_ShadowParallelSplitDistances,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_ViewMatrix,
	public u_UnprojectMatrix,
	public u_PortalPlane,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_FRUSTUM_CLIPPING,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_deferredLighting_directionalSun();
};

class GLShader_geometricFill :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_AlphaTest,
	public u_ColorModulate,
	public u_Color,
	public u_ViewOrigin,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_PortalPlane,
	public u_DepthScale,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_ALPHA_TESTING,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING,
	public GLCompileMacro_USE_REFLECTIVE_SPECULAR
{
public:
	GLShader_geometricFill();
};

class GLShader_shadowFill :
	public GLShader,
	public u_ColorTextureMatrix,
	public u_ViewOrigin,
	public u_AlphaTest,
	public u_LightOrigin,
	public u_LightRadius,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_Color,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_PortalPlane,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_ALPHA_TESTING,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_LIGHT_DIRECTIONAL
{
public:
	GLShader_shadowFill();
};

class GLShader_reflection :
	public GLShader,
	public u_ColorMap,
	public u_NormalMap,
	public u_NormalTextureMatrix,
	public u_ViewOrigin,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_PortalPlane,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_reflection();
};

class GLShader_skybox :
	public GLShader,
	public u_ColorMap,
	public u_ViewOrigin,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_PortalPlane,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING
{
public:
	GLShader_skybox();
};

class GLShader_fogQuake3 :
	public GLShader,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_Color,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_PortalPlane,
	public u_FogDistanceVector,
	public u_FogDepthVector,
	public u_FogEyeT,
	public GLDeformStage,
	public GLCompileMacro_USE_PORTAL_CLIPPING,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_EYE_OUTSIDE
{
public:
	GLShader_fogQuake3();
};

class GLShader_fogGlobal :
	public GLShader,
	public u_ViewOrigin,
	public u_ViewMatrix,
	public u_ModelViewProjectionMatrix,
	public u_UnprojectMatrix,
	public u_Color,
	public u_FogDistanceVector,
	public u_FogDepthVector
{
public:
	GLShader_fogGlobal();
};

class GLShader_heatHaze :
	public GLShader,
	public u_NormalTextureMatrix,
	public u_ViewOrigin,
//public u_AlphaTest,
public u_DeformMagnitude,
public u_ModelMatrix,
public u_ModelViewProjectionMatrix,
public u_ModelViewMatrixTranspose,
public u_ProjectionMatrixTranspose,
public u_ColorModulate,
public u_Color,
public u_BoneMatrix,
public u_VertexInterpolation,
public u_PortalPlane,
public GLDeformStage,
public GLCompileMacro_USE_PORTAL_CLIPPING,
//public GLCompileMacro_USE_ALPHA_TESTING,
public GLCompileMacro_USE_VERTEX_SKINNING,
public GLCompileMacro_USE_VERTEX_ANIMATION,
public GLCompileMacro_USE_DEFORM_VERTEXES
{
public:
	GLShader_heatHaze();
};

class GLShader_screen :
	public GLShader,
	public u_ModelViewProjectionMatrix
{
public:
	GLShader_screen();
};

class GLShader_portal :
	public GLShader,
	public u_ModelViewMatrix,
	public u_ModelViewProjectionMatrix,
	public u_PortalRange
{
public:
	GLShader_portal();
};

class GLShader_toneMapping :
	public GLShader,
	public u_ModelViewProjectionMatrix,
	public u_HDRKey,
	public u_HDRAverageLuminance,
	public u_HDRMaxLuminance,
	public GLCompileMacro_BRIGHTPASS_FILTER
{
public:
	GLShader_toneMapping();
};

class GLShader_contrast :
	public GLShader,
	public u_ModelViewProjectionMatrix
{
public:
	GLShader_contrast();
};

class GLShader_cameraEffects :
	public GLShader,
	public u_ColorTextureMatrix,
	public u_ModelViewProjectionMatrix,
	public u_DeformMagnitude
{
public:
	GLShader_cameraEffects();
};

class GLShader_blurX :
	public GLShader,
	public u_ModelViewProjectionMatrix,
	public u_DeformMagnitude
{
public:
	GLShader_blurX();
};

class GLShader_blurY :
	public GLShader,
	public u_ModelViewProjectionMatrix,
	public u_DeformMagnitude
{
public:
	GLShader_blurY();
};

class GLShader_debugShadowMap :
	public GLShader,
	public u_ModelViewProjectionMatrix
{
public:
	GLShader_debugShadowMap();
};

extern GLShader_generic                         *gl_genericShader;
extern GLShader_lightMapping                    *gl_lightMappingShader;
extern GLShader_vertexLighting_DBS_entity       *gl_vertexLightingShader_DBS_entity;
extern GLShader_vertexLighting_DBS_world        *gl_vertexLightingShader_DBS_world;
extern GLShader_forwardLighting_omniXYZ         *gl_forwardLightingShader_omniXYZ;
extern GLShader_forwardLighting_projXYZ         *gl_forwardLightingShader_projXYZ;
extern GLShader_forwardLighting_directionalSun *gl_forwardLightingShader_directionalSun;
extern GLShader_deferredLighting_omniXYZ        *gl_deferredLightingShader_omniXYZ;
extern GLShader_deferredLighting_projXYZ        *gl_deferredLightingShader_projXYZ;
extern GLShader_deferredLighting_directionalSun *gl_deferredLightingShader_directionalSun;
extern GLShader_geometricFill                   *gl_geometricFillShader;
extern GLShader_shadowFill                      *gl_shadowFillShader;
extern GLShader_reflection                      *gl_reflectionShader;
extern GLShader_skybox                          *gl_skyboxShader;
extern GLShader_fogQuake3                       *gl_fogQuake3Shader;
extern GLShader_fogGlobal                       *gl_fogGlobalShader;
extern GLShader_heatHaze                        *gl_heatHazeShader;
extern GLShader_screen                          *gl_screenShader;
extern GLShader_portal                          *gl_portalShader;
extern GLShader_toneMapping                     *gl_toneMappingShader;
extern GLShader_contrast                        *gl_contrastShader;
extern GLShader_cameraEffects                   *gl_cameraEffectsShader;
extern GLShader_blurX                           *gl_blurXShader;
extern GLShader_blurY                           *gl_blurYShader;
extern GLShader_debugShadowMap                  *gl_debugShadowMapShader;

#ifdef USE_GLSL_OPTIMIZER
extern struct glslopt_ctx *s_glslOptimizer;
#endif

#endif // GL_SHADER_H
