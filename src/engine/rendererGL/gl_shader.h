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
#include <queue>
#include <string>

#include "tr_local.h"

#define LOG_GLSL_UNIFORMS 1
#define USE_UNIFORM_FIREWALL 1

// *INDENT-OFF*
static const int MAX_SHADER_MACROS = 9;
static const unsigned int GL_SHADER_VERSION = 3;

struct GLShaderHeader
{
	unsigned int version;
	unsigned int checkSum; // checksum of shader source this was built from

	unsigned int macros[ MAX_SHADER_MACROS ]; // macros the shader uses ( may or may not be enabled )
	unsigned int numMacros;

	GLenum binaryFormat; // argument to glProgramBinary
	GLint  binaryLength; // argument to glProgramBinary
};

class GLUniform;
class GLCompileMacro;
class GLShaderManager;

class GLShader
{
	friend class GLShaderManager;
private:
	GLShader &operator             = ( const GLShader & );

	std::string                    _name;
	std::string                    _mainShaderName;
protected:
	std::string                    _fragmentShaderText;
	std::string                    _vertexShaderText;
	int                            _activeMacros;
	std::vector< shaderProgram_t > _shaderPrograms;
	shaderProgram_t                 *_currentProgram;
	GLShaderManager                 *_shaderManager;
	std::vector< GLUniform * >      _uniforms;
	std::vector< GLCompileMacro * > _compileMacros;

	size_t                         _uniformStorageSize;
	const uint32_t                 _vertexAttribsRequired;
	uint32_t                       _vertexAttribs; // can be set by uniforms
	unsigned int                   _checkSum;

	GLShader( const std::string &name, uint32_t vertexAttribsRequired, GLShaderManager *manager ) :
		_name( name ),
		_mainShaderName( name ),
		_activeMacros( 0 ),
		_checkSum( 0 ),
		_currentProgram( NULL ),
		_vertexAttribsRequired( vertexAttribsRequired ),
		_vertexAttribs( 0 ),
		_shaderManager( manager ),
		_uniformStorageSize( 0 )
		//_vertexAttribsOptional(vertexAttribsOptional),
		//_vertexAttribsUnsupported(vertexAttribsUnsupported)
	{
	}

	GLShader( const std::string &name, const std::string &mainShaderName, uint32_t vertexAttribsRequired, GLShaderManager *manager ) :
		_name( name ),
		_mainShaderName( mainShaderName ),
		_activeMacros( 0 ),
		_checkSum( 0 ),
		_currentProgram( NULL ),
		_vertexAttribsRequired( vertexAttribsRequired ),
		_vertexAttribs( 0 ),
		_shaderManager( manager ),
		_uniformStorageSize( 0 )
	{
	}

	~GLShader()
	{
		for ( std::size_t i = 0; i < _shaderPrograms.size(); i++ )
		{
			shaderProgram_t *p = &_shaderPrograms[ i ];

			if ( p->program )
			{
				glDeleteProgram( p->program );
			}

			if ( p->uniformFirewall )
			{
				ri.Free( p->uniformFirewall );
			}

			if ( p->uniformLocations )
			{
				ri.Free( p->uniformLocations );
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
		if ( _compileMacros.size() >= MAX_SHADER_MACROS )
		{
			ri.Error( ERR_DROP, "Can't register more than 9 compile macros for a single shader" );
		}

		_compileMacros.push_back( compileMacro );
	}

	size_t GetNumOfCompiledMacros() const
	{
		return _compileMacros.size();
	}

	shaderProgram_t        *GetProgram() const
	{
		return _currentProgram;
	}

	const std::string      &GetName() const
	{
		return _name;
	}

	const std::string      &GetMainShaderName() const
	{
		return _mainShaderName;
	}

protected:
	bool         GetCompileMacrosString( size_t permutation, std::string &compileMacrosOut ) const;
	virtual void BuildShaderVertexLibNames( std::string& vertexInlines ) { };
	virtual void BuildShaderFragmentLibNames( std::string& vertexInlines ) { };
	virtual void BuildShaderCompileMacros( std::string& vertexInlines ) { };
	virtual void SetShaderProgramUniforms( shaderProgram_t *shaderProgram ) { };
	int          SelectProgram();
public:
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

class GLShaderManager
{
	std::queue< GLShader* > _shaderBuildQueue;
	std::vector< GLShader* > _shaders;
	int       _lastBuildStartTime;
	int       _lastBuildTime;
	int       _beginBuildTime;
	int       _endBuildTime;
	int       _totalBuildTime;
public:
	GLShaderManager() : _beginBuildTime( 0 ), _endBuildTime( 0 ), _totalBuildTime( 0 ),
	                    _lastBuildTime( 1 ), _lastBuildStartTime( 0 )
	{
	}
	~GLShaderManager();

	template< class T >
	void load( T *& shader )
	{
		shader = new T( this );
		InitShader( shader );
		_shaders.push_back( shader );
		_shaderBuildQueue.push( shader );
	}
	void freeAll();

	bool buildPermutation( GLShader *shader, size_t permutation );
	void buildIncremental( int dt );
	void buildAll();
private:
	bool LoadShaderBinary( GLShader *shader, size_t permutation );
	void SaveShaderBinary( GLShader *shader, size_t permutation );
	void CompileGPUShader( GLuint program, const char *programName, const char *shaderText,
	                       int shaderTextSize, GLenum shaderType ) const;
	void CompileAndLinkGPUShaderProgram( GLShader *shader, shaderProgram_t *program,
	                                     const std::string &compileMacros ) const;
	std::string BuildGPUShaderText( const char *mainShader, const char *libShaders, GLenum shaderType ) const;
	void LinkProgram( GLuint program ) const;
	void BindAttribLocations( GLuint program ) const;
	void PrintShaderSource( GLuint object ) const;
	void PrintInfoLog( GLuint object, bool developerOnly ) const;
	void InitShader( GLShader *shader );
	void ValidateProgram( GLuint program ) const;
	void UpdateShaderProgramUniformLocations( GLShader *shader, shaderProgram_t *shaderProgram ) const;
};

class GLUniform
{
protected:
	GLShader   *_shader;
	std::string _name;
	size_t      _firewallIndex;
	size_t      _locationIndex;

	GLUniform( GLShader *shader, const char *name ) :
		_shader( shader ),
		_name( name ),
		_firewallIndex( 0 ),
		_locationIndex( 0 )
	{
		_shader->RegisterUniform( this );
	}

public:
	void SetFirewallIndex( size_t offSetValue )
	{
		_firewallIndex = offSetValue;
	}

	void SetLocationIndex( size_t index )
	{
		_locationIndex = index;
	}

	const char *GetName( void )
	{
		return _name.c_str();
	}

	void UpdateShaderProgramUniformLocation( shaderProgram_t *shaderProgram )
	{
		shaderProgram->uniformLocations[ _locationIndex ] = glGetUniformLocation( shaderProgram->program, GetName() );
	}

	virtual size_t GetSize( void )
	{
		return 0;
	}
};

class GLUniform1f : protected GLUniform
{
protected:
	GLUniform1f( GLShader *shader, const char *name ) :
	GLUniform( shader, name )
	{
	}

	inline void SetValue( float value )
	{
		shaderProgram_t *p = _shader->GetProgram();
#if defined( LOG_GLSL_UNIFORMS )
		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "GLSL_SetUniform1f( %s, shader: %s, value: %f ) ---\n",
				this->GetName(), _shader->GetName().c_str(), value ) );
		}
#endif
#if defined( USE_UNIFORM_FIREWALL )
		float *firewall = ( float * ) &p->uniformFirewall[ _firewallIndex ];

		if ( *firewall == value )
		{
			return;
		}

		*firewall = value;
#endif
		glUniform1f( p->uniformLocations[ _locationIndex ], value );
	}
public:
	size_t GetSize( void )
	{
		return sizeof( float );
	}
};

class GLUniform1fv : protected GLUniform
{
protected:
	GLUniform1fv( GLShader *shader, const char *name ) :
	GLUniform( shader, name )
	{
	}

	inline void SetValue( int numFloats, float *f )
	{
		shaderProgram_t *p = _shader->GetProgram();

#if defined( LOG_GLSL_UNIFORMS )
		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "GLSL_SetUniform1fv( %s, shader: %s, numFloats: %d ) ---\n",
				this->GetName(), _shader->GetName().c_str(), numFloats ) );
		}
#endif
		glUniform1fv( p->uniformLocations[ _locationIndex ], numFloats, f );
	}
};

class GLUniform3f : protected GLUniform
{
protected:
	GLUniform3f( GLShader *shader, const char *name ) :
	GLUniform( shader, name )
	{
	}

	inline void SetValue( const vec3_t v )
	{
		shaderProgram_t *p = _shader->GetProgram();
#if defined( LOG_GLSL_UNIFORMS )
		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "GLSL_SetUniform3f( %s, shader: %s, value: [ %f, %f, %f ] ) ---\n",
				this->GetName(), _shader->GetName().c_str(), v[ 0 ], v[ 1 ], v[ 2 ] ) );
		}
#endif
#if defined( USE_UNIFORM_FIREWALL )
		vec3_t *firewall = ( vec3_t * ) &p->uniformFirewall[ _firewallIndex ];

		if ( VectorCompare( *firewall, v ) )
		{
			return;
		}

		VectorCopy( v, *firewall );
#endif
		glUniform3f( p->uniformLocations[ _locationIndex ], v[ 0 ], v[ 1 ], v[ 2 ] );
	}
public:
	size_t GetSize( void )
	{
		return sizeof( vec3_t );
	}
};

class GLUniform4f : protected GLUniform
{
protected:
	GLUniform4f( GLShader *shader, const char *name ) :
	GLUniform( shader, name )
	{
	}

	inline void SetValue( const vec4_t v )
	{
		shaderProgram_t *p = _shader->GetProgram();
#if defined( LOG_GLSL_UNIFORMS )
		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "GLSL_SetUniform4f( %s, shader: %s, value: [ %f, %f, %f, %f ] ) ---\n",
				this->GetName(), _shader->GetName().c_str(), v[ 0 ], v[ 1 ], v[ 2 ], v[ 3 ] ) );
		}
#endif
#if defined( USE_UNIFORM_FIREWALL )
		vec4_t *firewall = ( vec4_t * ) &p->uniformFirewall[ _firewallIndex ];

		if ( Vector4Compare( *firewall, v ) )
		{
			return;
		}

		Vector4Copy( v, *firewall );
#endif
		glUniform4f( p->uniformLocations[ _locationIndex ], v[ 0 ], v[ 1 ], v[ 2 ], v[ 3 ] );
	}
public:
	size_t GetSize( void )
	{
		return sizeof( vec4_t );
	}
};

class GLUniform4fv : protected GLUniform
{
protected:
	GLUniform4fv( GLShader *shader, const char *name ) :
	GLUniform( shader, name )
	{
	}

	inline void SetValue( int numV, vec4_t *v )
	{
		shaderProgram_t *p = _shader->GetProgram();
#if defined( LOG_GLSL_UNIFORMS )
		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "GLSL_SetUniform4fv( %s, shader: %s, numV: %d ) ---\n",
				this->GetName(), _shader->GetName().c_str(), numV ) );
		}
#endif
		glUniform4fv( p->uniformLocations[ _locationIndex ], numV, &v[ 0 ][ 0 ] );
	}
};

class GLUniformMatrix4f : protected GLUniform
{
protected:
	GLUniformMatrix4f( GLShader *shader, const char *name ) :
	GLUniform( shader, name )
	{
	}

	inline void SetValue( GLboolean transpose, const matrix_t m )
	{
		shaderProgram_t *p = _shader->GetProgram();
#if defined( LOG_GLSL_UNIFORMS )
		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "GLSL_SetUniformMatrix4f( %s, shader: %s, transpose: %d, [ %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f ] ) ---\n",
				this->GetName(), _shader->GetName().c_str(), transpose,
				m[ 0 ], m[ 1 ], m[ 2 ], m[ 3 ], m[ 4 ], m[ 5 ], m[ 6 ], m[ 7 ], m[ 8 ], m[ 9 ], m[ 10 ], m[ 11 ], m[ 12 ],
				m[ 13 ], m[ 14 ], m[ 15 ] ) );
		}
#endif
#if defined( USE_UNIFORM_FIREWALL )
		matrix_t *firewall = ( matrix_t * ) &p->uniformFirewall[ _firewallIndex ];

		if ( MatrixCompare( m, *firewall ) )
		{
			return;
		}

		MatrixCopy( m, *firewall );
#endif
		glUniformMatrix4fv( p->uniformLocations[ _locationIndex ], 1, transpose, m );
	}
public:
	size_t GetSize( void )
	{
		return sizeof( matrix_t );
	}
};

class GLUniformMatrix4fv : protected GLUniform
{
protected:
	GLUniformMatrix4fv( GLShader *shader, const char *name ) :
	GLUniform( shader, name )
	{
	}

	inline void SetValue( int numMatrices, GLboolean transpose, const matrix_t *m )
	{
		shaderProgram_t *p = _shader->GetProgram();

#if defined( LOG_GLSL_UNIFORMS )
		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "GLSL_SetUniformMatrix4fv( %s, shader: %s, numMatrices: %d, transpose: %d ) ---\n",
				this->GetName(), _shader->GetName().c_str(), numMatrices, transpose ) );
		}
#endif
		glUniformMatrix4fv( p->uniformLocations[ _locationIndex ], numMatrices, transpose, &m[ 0 ][ 0 ] );
	}
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

	virtual bool            HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
	{
		return false;
	}

	virtual bool            MissesRequiredMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const
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
		}
	}

	void DisableMacro()
	{
		int bit = GetBit();

		if ( _shader->IsMacroSet( bit ) )
		{
			_shader->DelMacroBit( bit );
		}
	}

public:
	int GetBit() const
	{
		return _bit;
	}

	virtual ~GLCompileMacro() {}
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

	bool HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const;
	bool MissesRequiredMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const;

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

	bool     HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const;
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

	bool HasConflictingMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const;

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

	bool MissesRequiredMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const;

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

	bool MissesRequiredMacros( size_t permutation, const std::vector< GLCompileMacro * > &macros ) const;

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

class u_ColorTextureMatrix :
	GLUniformMatrix4f
{
public:
	u_ColorTextureMatrix( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_ColorTextureMatrix" )
	{
	}

	void SetUniform_ColorTextureMatrix( const matrix_t m )
	{
		this->SetValue( GL_FALSE, m );
	}
};

class u_DiffuseTextureMatrix :
	GLUniformMatrix4f
{
public:
	u_DiffuseTextureMatrix( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_DiffuseTextureMatrix" )
	{
	}

	void SetUniform_DiffuseTextureMatrix( const matrix_t m )
	{
		this->SetValue( GL_FALSE, m );
	}
};

class u_NormalTextureMatrix :
	GLUniformMatrix4f
{
public:
	u_NormalTextureMatrix( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_NormalTextureMatrix" )
	{
	}

	void SetUniform_NormalTextureMatrix( const matrix_t m )
	{
		this->SetValue( GL_FALSE, m );
	}
};

class u_SpecularTextureMatrix :
	GLUniformMatrix4f
{
public:
	u_SpecularTextureMatrix( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_SpecularTextureMatrix" )
	{
	}

	void SetUniform_SpecularTextureMatrix( const matrix_t m )
	{
		this->SetValue( GL_FALSE, m );
	}
};

class u_AlphaThreshold :
	GLUniform1f
{
public:
	u_AlphaThreshold( GLShader *shader ) :
		GLUniform1f( shader, "u_AlphaThreshold" )
	{
	}

	void SetUniform_AlphaTest( uint32_t stateBits )
	{
		float value;

		switch( stateBits & GLS_ATEST_BITS ) {
			case GLS_ATEST_GT_0:
				value = 1.0f;
				break;
			case GLS_ATEST_LT_128:
				value = -1.5f;
				break;
			case GLS_ATEST_GE_128:
				value = 0.5f;
				break;
			case GLS_ATEST_GT_ENT:
				value = 1.0f - backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( 1.0f / 255.0f );
				break;
			case GLS_ATEST_LT_ENT:
				value = -2.0f + backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( 1.0f / 255.0f );
				break;
			default:
				value = 1.5f;
				break;
		}

		this->SetValue( value );
	}
};

class u_AmbientColor :
	GLUniform3f
{
public:
	u_AmbientColor( GLShader *shader ) :
		GLUniform3f( shader, "u_AmbientColor" )
	{
	}

	void SetUniform_AmbientColor( const vec3_t v )
	{
		this->SetValue( v );
	}
};

class u_ViewOrigin :
	GLUniform3f
{
public:
	u_ViewOrigin( GLShader *shader ) :
		GLUniform3f( shader, "u_ViewOrigin" )
	{
	}

	void SetUniform_ViewOrigin( const vec3_t v )
	{
		this->SetValue( v );
	}
};

class u_LightDir :
	GLUniform3f
{
public:
	u_LightDir( GLShader *shader ) :
		GLUniform3f( shader, "u_LightDir" )
	{
	}

	void SetUniform_LightDir( const vec3_t v )
	{
		this->SetValue( v );
	}
};

class u_LightOrigin :
	GLUniform3f
{
public:
	u_LightOrigin( GLShader *shader ) :
		GLUniform3f( shader, "u_LightOrigin" )
	{
	}

	void SetUniform_LightOrigin( const vec3_t v )
	{
		this->SetValue( v );
	}
};

class u_LightColor :
	GLUniform3f
{
public:
	u_LightColor( GLShader *shader ) :
		GLUniform3f( shader, "u_LightColor" )
	{
	}

	void SetUniform_LightColor( const vec3_t v )
	{
		this->SetValue( v );
	}
};

class u_LightRadius :
	GLUniform1f
{
public:
	u_LightRadius( GLShader *shader ) :
		GLUniform1f( shader, "u_LightRadius" )
	{
	}

	void SetUniform_LightRadius( float value )
	{
		this->SetValue( value );
	}
};

class u_LightScale :
	GLUniform1f
{
public:
	u_LightScale( GLShader *shader ) :
		GLUniform1f( shader, "u_LightScale" )
	{
	}

	void SetUniform_LightScale( float value )
	{
		this->SetValue( value );
	}
};

class u_LightWrapAround :
	GLUniform1f
{
public:
	u_LightWrapAround( GLShader *shader ) :
		GLUniform1f( shader, "u_LightWrapAround" )
	{
	}

	void SetUniform_LightWrapAround( float value )
	{
		this->SetValue( value );
	}
};

class u_LightAttenuationMatrix :
	GLUniformMatrix4f
{
public:
	u_LightAttenuationMatrix( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_LightAttenuationMatrix" )
	{
	}

	void SetUniform_LightAttenuationMatrix( const matrix_t m )
	{
		this->SetValue( GL_FALSE, m );
	}
};

class u_LightFrustum :
	GLUniform4fv
{
public:
	u_LightFrustum( GLShader *shader ) :
		GLUniform4fv( shader, "u_LightFrustum" )
	{
	}

	void SetUniform_LightFrustum( vec4_t lightFrustum[ 6 ] )
	{
		this->SetValue( 6, lightFrustum );
	}
};

class u_ShadowTexelSize :
	GLUniform1f
{
public:
	u_ShadowTexelSize( GLShader *shader ) :
		GLUniform1f( shader, "u_ShadowTexelSize" )
	{
	}

	void SetUniform_ShadowTexelSize( float value )
	{
		this->SetValue( value );
	}
};

class u_ShadowBlur :
	GLUniform1f
{
public:
	u_ShadowBlur( GLShader *shader ) :
		GLUniform1f( shader, "u_ShadowBlur" )
	{
	}

	void SetUniform_ShadowBlur( float value )
	{
		this->SetValue( value );
	}
};

class u_ShadowMatrix :
	GLUniformMatrix4fv
{
public:
	u_ShadowMatrix( GLShader *shader ) :
		GLUniformMatrix4fv( shader, "u_ShadowMatrix" )
	{
	}

	void SetUniform_ShadowMatrix( matrix_t m[ MAX_SHADOWMAPS ] )
	{
		this->SetValue( MAX_SHADOWMAPS, GL_FALSE, m );
	}
};

class u_ShadowParallelSplitDistances :
	GLUniform4f
{
public:
	u_ShadowParallelSplitDistances( GLShader *shader ) :
		GLUniform4f( shader, "u_ShadowParallelSplitDistances" )
	{
	}

	void SetUniform_ShadowParallelSplitDistances( const vec4_t v )
	{
		this->SetValue( v );
	}
};

class u_RefractionIndex :
	GLUniform1f
{
public:
	u_RefractionIndex( GLShader *shader ) :
		GLUniform1f( shader, "u_RefractionIndex" )
	{
	}

	void SetUniform_RefractionIndex( float value )
	{
		this->SetValue( value );
	}
};

class u_FresnelPower :
	GLUniform1f
{
public:
	u_FresnelPower( GLShader *shader ) :
		GLUniform1f( shader, "u_FresnelPower" )
	{
	}

	void SetUniform_FresnelPower( float value )
	{
		this->SetValue( value );
	}
};

class u_FresnelScale :
	GLUniform1f
{
public:
	u_FresnelScale( GLShader *shader ) :
		GLUniform1f( shader, "u_FresnelScale" )
	{
	}

	void SetUniform_FresnelScale( float value )
	{
		this->SetValue( value );
	}
};

class u_FresnelBias :
	GLUniform1f
{
public:
	u_FresnelBias( GLShader *shader ) :
		GLUniform1f( shader, "u_FresnelBias" )
	{
	}

	void SetUniform_FresnelBias( float value )
	{
		this->SetValue( value );
	}
};

class u_NormalScale :
	GLUniform1f
{
public:
	u_NormalScale( GLShader *shader ) :
		GLUniform1f( shader, "u_NormalScale" )
	{
	}

	void SetUniform_NormalScale( float value )
	{
		this->SetValue( value );
	}
};

class u_FogDensity :
	GLUniform1f
{
public:
	u_FogDensity( GLShader *shader ) :
		GLUniform1f( shader, "u_FogDensity" )
	{
	}

	void SetUniform_FogDensity( float value )
	{
		this->SetValue( value );
	}
};

class u_FogColor :
	GLUniform3f
{
public:
	u_FogColor( GLShader *shader ) :
		GLUniform3f( shader, "u_FogColor" )
	{
	}

	void SetUniform_FogColor( const vec3_t v )
	{
		this->SetValue( v );
	}
};

class u_Color :
	GLUniform4f
{
public:
	u_Color( GLShader *shader ) :
		GLUniform4f( shader, "u_Color" )
	{
	}

	void SetUniform_Color( const vec4_t v )
	{
		this->SetValue( v );
	}
};

class u_ModelMatrix :
	GLUniformMatrix4f
{
public:
	u_ModelMatrix( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_ModelMatrix" )
	{
	}

	void SetUniform_ModelMatrix( const matrix_t m )
	{
		this->SetValue( GL_FALSE, m );
	}
};

class u_ViewMatrix :
	GLUniformMatrix4f
{
public:
	u_ViewMatrix( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_ViewMatrix" )
	{
	}

	void SetUniform_ViewMatrix( const matrix_t m )
	{
		this->SetValue( GL_FALSE, m );
	}
};

class u_ModelViewMatrix :
	GLUniformMatrix4f
{
public:
	u_ModelViewMatrix( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_ModelViewMatrix" )
	{
	}

	void SetUniform_ModelViewMatrix( const matrix_t m )
	{
		this->SetValue( GL_FALSE, m );
	}
};

class u_ModelViewMatrixTranspose :
	GLUniformMatrix4f
{
public:
	u_ModelViewMatrixTranspose( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_ModelViewMatrixTranspose" )
	{
	}

	void SetUniform_ModelViewMatrixTranspose( const matrix_t m )
	{
		this->SetValue( GL_TRUE, m );
	}
};

class u_ProjectionMatrixTranspose :
	GLUniformMatrix4f
{
public:
	u_ProjectionMatrixTranspose( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_ProjectionMatrixTranspose" )
	{
	}

	void SetUniform_ProjectionMatrixTranspose( const matrix_t m )
	{
		this->SetValue( GL_TRUE, m );
	}
};

class u_ModelViewProjectionMatrix :
	GLUniformMatrix4f
{
public:
	u_ModelViewProjectionMatrix( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_ModelViewProjectionMatrix" )
	{
	}

	void SetUniform_ModelViewProjectionMatrix( const matrix_t m )
	{
		this->SetValue( GL_FALSE, m );
	}
};

class u_UnprojectMatrix :
	GLUniformMatrix4f
{
public:
	u_UnprojectMatrix( GLShader *shader ) :
		GLUniformMatrix4f( shader, "u_UnprojectMatrix" )
	{
	}

	void SetUniform_UnprojectMatrix( const matrix_t m )
	{
		this->SetValue( GL_FALSE, m );
	}
};

class u_BoneMatrix :
	GLUniformMatrix4fv
{
public:
	u_BoneMatrix( GLShader *shader ) :
		GLUniformMatrix4fv( shader, "u_BoneMatrix" )
	{
	}

	void SetUniform_BoneMatrix( int numBones, const matrix_t boneMatrices[ MAX_BONES ] )
	{
		this->SetValue( numBones, GL_FALSE, boneMatrices );
	}
};

class u_VertexInterpolation :
	GLUniform1f
{
public:
	u_VertexInterpolation( GLShader *shader ) :
		GLUniform1f( shader, "u_VertexInterpolation" )
	{
	}

	void SetUniform_VertexInterpolation( float value )
	{
		this->SetValue( value );
	}
};

class u_PortalRange :
	GLUniform1f
{
public:
	u_PortalRange( GLShader *shader ) :
		GLUniform1f( shader, "u_PortalRange" )
	{
	}

	void SetUniform_PortalRange( float value )
	{
		this->SetValue( value );
	}
};

class u_DepthScale :
	GLUniform1f
{
public:
	u_DepthScale( GLShader *shader ) :
		GLUniform1f( shader, "u_DepthScale" )
	{
	}

	void SetUniform_DepthScale( float value )
	{
		this->SetValue( value );
	}
};

class u_EnvironmentInterpolation :
	GLUniform1f
{
public:
	u_EnvironmentInterpolation( GLShader *shader ) :
		GLUniform1f( shader, "u_EnvironmentInterpolation" )
	{
	}

	void SetUniform_EnvironmentInterpolation( float value )
	{
		this->SetValue( value );
	}
};

class u_DeformParms :
	GLUniform1fv
{
public:
	u_DeformParms( GLShader *shader ) :
		GLUniform1fv( shader, "u_DeformParms" )
	{
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
		}

		this->SetValue( deformOfs, deformParms );
	}
};

class u_Time :
	GLUniform1f
{
public:
	u_Time( GLShader *shader ) :
		GLUniform1f( shader, "u_Time" )
	{
	}

	void SetUniform_Time( float value )
	{
		this->SetValue( value );
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
	GLUniform4f
{
public:
	u_ColorModulate( GLShader *shader ) :
		GLUniform4f( shader, "u_ColorModulate" )
	{
	}

	void SetUniform_ColorModulate( colorGen_t colorGen, alphaGen_t alphaGen )
	{
		vec4_t v;

		if ( r_logFile->integer )
		{
			GLimp_LogComment( va( "--- u_ColorModulate::SetUniform_ColorModulate( program = %s, colorGen = %i, alphaGen = %i ) ---\n", _shader->GetName().c_str(), colorGen, alphaGen ) );
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

		this->SetValue( v );
	}
};

class u_FogDistanceVector :
	GLUniform4f
{
public:
	u_FogDistanceVector( GLShader *shader ) :
		GLUniform4f( shader, "u_FogDistanceVector" )
	{
	}

	void SetUniform_FogDistanceVector( const vec4_t v )
	{
		this->SetValue( v );
	}
};

class u_FogDepthVector :
	GLUniform4f
{
public:
	u_FogDepthVector( GLShader *shader ) :
		GLUniform4f( shader, "u_FogDepthVector" )
	{
	}

	void SetUniform_FogDepthVector( const vec4_t v )
	{
		this->SetValue( v );
	}
};

class u_FogEyeT :
	GLUniform1f
{
public:
	u_FogEyeT( GLShader *shader ) :
		GLUniform1f( shader, "u_FogEyeT" )
	{
	}

	void SetUniform_FogEyeT( float value )
	{
		this->SetValue( value );
	}
};

class u_DeformMagnitude :
	GLUniform1f
{
public:
	u_DeformMagnitude( GLShader *shader ) :
		GLUniform1f( shader, "u_DeformMagnitude" )
	{
	}

	void SetUniform_DeformMagnitude( float value )
	{
		this->SetValue( value );
	}
};

class u_HDRKey :
	GLUniform1f
{
public:
	u_HDRKey( GLShader *shader ) :
		GLUniform1f( shader, "u_HDRKey" )
	{
	}

	void SetUniform_HDRKey( float value )
	{
		this->SetValue( value );
	}
};

class u_HDRAverageLuminance :
	GLUniform1f
{
public:
	u_HDRAverageLuminance( GLShader *shader ) :
		GLUniform1f( shader, "u_HDRAverageLuminance" )
	{
	}

	void SetUniform_HDRAverageLuminance( float value )
	{
		this->SetValue( value );
	}
};

class u_HDRMaxLuminance :
	GLUniform1f
{
public:
	u_HDRMaxLuminance( GLShader *shader ) :
		GLUniform1f( shader, "u_HDRMaxLuminance" )
	{
	}

	void SetUniform_HDRMaxLuminance( float value )
	{
		this->SetValue( value );
	}
};

class u_blurVec :
	GLUniform3f
{
public:
	u_blurVec( GLShader *shader ) :
		GLUniform3f( shader, "u_blurVec" )
	{
	}

	void SetUniform_blurVec( vec3_t value )
	{
		this->SetValue( value );
	}
};

class GLShader_generic :
	public GLShader,
	public u_ColorTextureMatrix,
	public u_ViewOrigin,
	public u_AlphaThreshold,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_ColorModulate,
	public u_Color,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public GLDeformStage,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_TCGEN_ENVIRONMENT,
	public GLCompileMacro_USE_TCGEN_LIGHTMAP
{
public:
	GLShader_generic( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_lightMapping :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_ColorModulate,
	public u_Color,
	public u_AlphaThreshold,
	public u_ViewOrigin,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_DepthScale,
	public GLDeformStage,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_lightMapping( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void BuildShaderFragmentLibNames( std::string& fragmentInlines );
	void BuildShaderCompileMacros( std::string& compileMacros );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_vertexLighting_DBS_entity :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_AlphaThreshold,
	public u_AmbientColor,
	public u_ViewOrigin,
	public u_LightDir,
	public u_LightColor,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_DepthScale,
	public u_EnvironmentInterpolation,
	public GLDeformStage,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING,
	public GLCompileMacro_USE_REFLECTIVE_SPECULAR //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_vertexLighting_DBS_entity( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void BuildShaderFragmentLibNames( std::string& fragmentInlines );
	void BuildShaderCompileMacros( std::string& compileMacros );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_vertexLighting_DBS_world :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_ColorModulate,
	public u_Color,
	public u_AlphaThreshold,
	public u_ViewOrigin,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_DepthScale,
	public u_LightWrapAround,
	public GLDeformStage,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING //,
//public GLCompileMacro_TWOSIDED
//public GLCompileMacro_USE_GBUFFER
{
public:
	GLShader_vertexLighting_DBS_world( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void BuildShaderFragmentLibNames( std::string& fragmentInlines );
	void BuildShaderCompileMacros( std::string& compileMacros );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_forwardLighting_omniXYZ :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_AlphaThreshold,
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
	public u_DepthScale,
	public GLDeformStage,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_forwardLighting_omniXYZ( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void BuildShaderFragmentLibNames( std::string& fragmentInlines );
	void BuildShaderCompileMacros( std::string& compileMacros );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_forwardLighting_projXYZ :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_AlphaThreshold,
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
	public u_DepthScale,
	public GLDeformStage,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_forwardLighting_projXYZ( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void BuildShaderFragmentLibNames( std::string& fragmentInlines );
	void BuildShaderCompileMacros( std::string& compileMacros );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_forwardLighting_directionalSun :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_AlphaThreshold,
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
	public u_DepthScale,
	public GLDeformStage,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_forwardLighting_directionalSun( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void BuildShaderFragmentLibNames( std::string& fragmentInlines );
	void BuildShaderCompileMacros( std::string& compileMacros );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
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
	public GLDeformStage,
	public GLCompileMacro_USE_FRUSTUM_CLIPPING,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_deferredLighting_omniXYZ( GLShaderManager *manager );
	void BuildShaderCompileMacros( std::string& compileMacros );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
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
	public GLDeformStage,
	public GLCompileMacro_USE_FRUSTUM_CLIPPING,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_deferredLighting_projXYZ( GLShaderManager *manager );
	void BuildShaderCompileMacros( std::string& compileMacros );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
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
	public GLDeformStage,
	public GLCompileMacro_USE_FRUSTUM_CLIPPING,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_SHADOWING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_deferredLighting_directionalSun( GLShaderManager *manager );
	void BuildShaderCompileMacros( std::string& compileMacros );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_geometricFill :
	public GLShader,
	public u_DiffuseTextureMatrix,
	public u_NormalTextureMatrix,
	public u_SpecularTextureMatrix,
	public u_AlphaThreshold,
	public u_ColorModulate,
	public u_Color,
	public u_ViewOrigin,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_DepthScale,
	public GLDeformStage,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING,
	public GLCompileMacro_USE_PARALLAX_MAPPING,
	public GLCompileMacro_USE_REFLECTIVE_SPECULAR
{
public:
	GLShader_geometricFill( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void BuildShaderFragmentLibNames( std::string& fragmentInlines );
	void BuildShaderCompileMacros( std::string& compileMacros );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_shadowFill :
	public GLShader,
	public u_ColorTextureMatrix,
	public u_ViewOrigin,
	public u_AlphaThreshold,
	public u_LightOrigin,
	public u_LightRadius,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_Color,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public GLDeformStage,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_LIGHT_DIRECTIONAL
{
public:
	GLShader_shadowFill( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_reflection :
	public GLShader,
	public u_NormalTextureMatrix,
	public u_ViewOrigin,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public GLDeformStage,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_USE_NORMAL_MAPPING //,
//public GLCompileMacro_TWOSIDED
{
public:
	GLShader_reflection( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void BuildShaderCompileMacros( std::string& compileMacros );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_skybox :
	public GLShader,
	public u_ViewOrigin,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public GLDeformStage
{
public:
	GLShader_skybox( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_fogQuake3 :
	public GLShader,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_Color,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public u_FogDistanceVector,
	public u_FogDepthVector,
	public u_FogEyeT,
	public GLDeformStage,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES,
	public GLCompileMacro_EYE_OUTSIDE
{
public:
	GLShader_fogQuake3( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
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
	GLShader_fogGlobal( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_heatHaze :
	public GLShader,
	public u_NormalTextureMatrix,
	public u_ViewOrigin,
	public u_DeformMagnitude,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_ModelViewMatrixTranspose,
	public u_ProjectionMatrixTranspose,
	public u_ColorModulate,
	public u_Color,
	public u_BoneMatrix,
	public u_VertexInterpolation,
	public GLDeformStage,
	public GLCompileMacro_USE_VERTEX_SKINNING,
	public GLCompileMacro_USE_VERTEX_ANIMATION,
	public GLCompileMacro_USE_DEFORM_VERTEXES
{
public:
	GLShader_heatHaze( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_screen :
	public GLShader,
	public u_ModelViewProjectionMatrix
{
public:
	GLShader_screen( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_portal :
	public GLShader,
	public u_ModelViewMatrix,
	public u_ModelViewProjectionMatrix,
	public u_PortalRange
{
public:
	GLShader_portal( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
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
	GLShader_toneMapping( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_contrast :
	public GLShader,
	public u_ModelViewProjectionMatrix
{
public:
	GLShader_contrast( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_cameraEffects :
	public GLShader,
	public u_ColorTextureMatrix,
	public u_ModelViewProjectionMatrix,
	public u_DeformMagnitude
{
public:
	GLShader_cameraEffects( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_blurX :
	public GLShader,
	public u_ModelViewProjectionMatrix,
	public u_DeformMagnitude
{
public:
	GLShader_blurX( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_blurY :
	public GLShader,
	public u_ModelViewProjectionMatrix,
	public u_DeformMagnitude
{
public:
	GLShader_blurY( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_debugShadowMap :
	public GLShader,
	public u_ModelViewProjectionMatrix
{
public:
	GLShader_debugShadowMap( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_depthToColor :
	public GLShader,
	public u_ModelViewProjectionMatrix,
	public u_BoneMatrix,
	public GLCompileMacro_USE_VERTEX_SKINNING
{
public:
	GLShader_depthToColor( GLShaderManager *manager );
	void BuildShaderVertexLibNames( std::string& vertexInlines );
};

class GLShader_lightVolume_omni :
	public GLShader,
	public u_ViewOrigin,
	public u_LightOrigin,
	public u_LightColor,
	public u_LightRadius,
	public u_LightScale,
	public u_LightAttenuationMatrix,
	public u_ModelViewProjectionMatrix,
	public u_UnprojectMatrix,
	public GLCompileMacro_USE_SHADOWING
{
public:
	GLShader_lightVolume_omni( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_deferredShadowing_proj :
	public GLShader,
	public u_LightOrigin,
	public u_LightColor,
	public u_LightRadius,
	public u_LightScale,
	public u_LightAttenuationMatrix,
	public u_ShadowMatrix,
	public u_ModelViewProjectionMatrix,
	public u_UnprojectMatrix,
	public GLCompileMacro_USE_SHADOWING
{
public:
	GLShader_deferredShadowing_proj( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_liquid :
	public GLShader,
	public u_NormalTextureMatrix,
	public u_ViewOrigin,
	public u_RefractionIndex,
	public u_ModelMatrix,
	public u_ModelViewProjectionMatrix,
	public u_UnprojectMatrix,
	public u_FresnelPower,
	public u_FresnelScale,
	public u_FresnelBias,
	public u_NormalScale,
	public u_FogDensity,
	public u_FogColor,
	public GLCompileMacro_USE_PARALLAX_MAPPING
{
public:
	GLShader_liquid( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_volumetricFog :
	public GLShader,
	public u_ViewOrigin,
	public u_UnprojectMatrix,
	public u_ModelViewMatrix,
	public u_FogDensity,
	public u_FogColor
{
public:
	GLShader_volumetricFog( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_screenSpaceAmbientOcclusion :
	public GLShader,
	public u_ModelViewProjectionMatrix
{
public:
	GLShader_screenSpaceAmbientOcclusion( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_depthOfField :
	public GLShader,
	public u_ModelViewProjectionMatrix
{
public:
	GLShader_depthOfField( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
};

class GLShader_motionblur :
	public GLShader,
	public u_blurVec
{
public:
	GLShader_motionblur( GLShaderManager *manager );
	void SetShaderProgramUniforms( shaderProgram_t *shaderProgram );
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
extern GLShader_depthToColor                    *gl_depthToColorShader;
extern GLShader_lightVolume_omni                *gl_lightVolumeShader_omni;
extern GLShader_deferredShadowing_proj          *gl_deferredShadowingShader_proj;
extern GLShader_liquid                          *gl_liquidShader;
extern GLShader_volumetricFog                   *gl_volumetricFogShader;
extern GLShader_screenSpaceAmbientOcclusion     *gl_screenSpaceAmbientOcclusionShader;
extern GLShader_depthOfField                    *gl_depthOfFieldShader;
extern GLShader_motionblur                      *gl_motionblurShader;
extern GLShaderManager                           gl_shaderManager;
#ifdef USE_GLSL_OPTIMIZER
extern struct glslopt_ctx *s_glslOptimizer;
#endif

#endif // GL_SHADER_H
