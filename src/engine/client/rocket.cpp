/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

extern "C"
{
	#include "client.h"
}

// FIXME: Macro conflicts with Rocket::Core::Vector2
#undef DotProduct

#include <Rocket/Core/FileInterface.h>
#include <Rocket/Core/SystemInterface.h>
#include <Rocket/Core/RenderInterface.h>
#include <Rocket/Core.h>
//#include <Rocket/Debugger.h>

class DaemonFileInterface : public Rocket::Core::FileInterface
{
public:
	Rocket::Core::FileHandle Open( const Rocket::Core::String &filePath )
	{
		fileHandle_t fileHandle;
		FS_FOpenFileRead( filePath.CString(), &fileHandle, qfalse );
		return ( Rocket::Core::FileHandle )fileHandle;
	}

	void Close( Rocket::Core::FileHandle file )
	{
		FS_FCloseFile( ( fileHandle_t ) file );
	}

	size_t Read( void *buffer, size_t size, Rocket::Core::FileHandle file )
	{
		return ( size_t ) FS_Read( buffer, (int)size, ( fileHandle_t ) file );
	}

	/*
	Returns true on success, false if something went wrong
	Not sure if the return value is 100% correct, but other code using FS_Seek does this
	FIXME: FS_Seek may need support for FS_SEEK_END and failure return values when reading files from zip files
	*/
	bool Seek( Rocket::Core::FileHandle file, long offset, int origin )
	{
		int ret = FS_Seek( ( fileHandle_t ) file, offset, origin );

		if ( ret < 0 )
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	size_t Tell( Rocket::Core::FileHandle file )
	{
		return ( size_t ) FS_FTell( ( fileHandle_t ) file );
	}

	/*
		May not be correct for files in zip files
	*/
	size_t Length( Rocket::Core::FileHandle file )
	{
		return ( size_t ) FS_filelength( ( fileHandle_t ) file );
	}
};

class DaemonSystemInterface : public Rocket::Core::SystemInterface
{
public:
	float GetElapsedTime()
	{
		return Sys_Milliseconds() / 1000.0f;
	}

	/*
	Not sure if this is correct
	*/
	int TranslateString( Rocket::Core::String &translated, const Rocket::Core::String &input )
	{
		const char* ret = Trans_Gettext( input.CString() );
		translated = ret;
		return 0;
	}

	//TODO: Add explicit support for other log types
	bool LogMessage( Rocket::Core::Log::Type type, const Rocket::Core::String &message )
	{
		switch ( type )
		{
			case Rocket::Core::Log::LT_ALWAYS :
				Com_Printf( "%s", message.CString() );
				break;
			case Rocket::Core::Log::LT_ERROR :
				Com_Printf( "%s", message.CString() );
				break;
			case Rocket::Core::Log::LT_WARNING :
				Com_Printf( "%s", message.CString() );
				break;
			case Rocket::Core::Log::LT_INFO :
				Com_Printf( "%s", message.CString() );
				break;
			default:
				Com_Printf( "%s", message.CString() );
		}
		return true;
	}
};

//TODO: CompileGeometry, RenderCompiledGeometry, ReleaseCompileGeometry ( use vbos and ibos )
class DaemonRenderInterface : public Rocket::Core::RenderInterface
{
public:
	DaemonRenderInterface() {};

	//TODO: translation support
	void RenderGeometry( Rocket::Core::Vertex *verticies,  int numVerticies, int *indices, int numIndicies, Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation )
	{
		polyVert_t *verts;
		verts = ( polyVert_t * ) Z_Malloc( sizeof( polyVert_t ) * numVerticies );
		
		for ( int i = 0; i < numVerticies; i++ )
		{
			polyVert_t &polyVert = verts[ i ];
			Rocket::Core::Vertex &vert = verticies[ i ];
			Vector2Copy( vert.position, polyVert.xyz );

			polyVert.modulate[ 0 ] = vert.colour.red;
			polyVert.modulate[ 1 ] = vert.colour.green;
			polyVert.modulate[ 2 ] = vert.colour.blue;
			polyVert.modulate[ 3 ] = vert.colour.alpha;

			Vector2Copy( vert.tex_coord, polyVert.st );
		}

		re.Add2dPolys( verts, numVerticies, ( qhandle_t ) texture );

		Z_Free( verts );
	}

	bool LoadTexture( Rocket::Core::TextureHandle& textureHandle, Rocket::Core::Vector2i& textureDimensions, const Rocket::Core::String& source )
	{
		qhandle_t shaderHandle = re.RegisterShaderNoMip( source.CString() );

		// find the size of the texture
		char *texnoext = ( char* ) Z_Malloc( source.Length() + 1 );
		COM_StripExtension( source.CString(), texnoext );

		int textureID = re.GetTextureId( texnoext );
		Z_Free( texnoext );

		re.GetTextureSize( textureID, &textureDimensions.x, &textureDimensions.y );

		textureHandle = ( Rocket::Core::TextureHandle ) shaderHandle;
		return true;
	}

	bool GenerateTexture( Rocket::Core::TextureHandle& textureHandle, const Rocket::Core::byte* source, const Rocket::Core::Vector2i& sourceDimensions )
	{
		// used for libRocket font system
		// TODO:
		textureHandle = 1;
		return true;
	}

	void ReleaseTexture( Rocket::Core::TextureHandle textureHandle )
	{
		// we free all textures after each map load, but this may have to be filled for the libRocket font system
	}

	void EnableScissorRegion( bool enable )
	{
		//TODO
		/* if ( enable )
			glEnable( GL_SCISSOR_TEST );
		   else
		    glDisable( GL_SCISSOR_TEST );
		*/
	}

	void SetScissorRegion( int x, int y, int width, int height )
	{
		//TODO
		//glScissor( x, glConfig.vidHeight - ( y + height ), width, height );
	}
};

static DaemonFileInterface fileInterface;
static DaemonSystemInterface systemInterface;
static DaemonRenderInterface renderInterface;
static Rocket::Core::Context *context = NULL;

extern "C" void Rocket_Init( void )
{
	Rocket::Core::SetFileInterface( &fileInterface );
	Rocket::Core::SetSystemInterface( &systemInterface );
	Rocket::Core::SetRenderInterface( &renderInterface );

	if ( !Rocket::Core::Initialise() )
	{
		Com_Printf( "Could not init libRocket\n" );
		return;
	}

	// won't work until required render interface stuff is done
	//Rocket::Core::FontDatabase::LoadFontFace( "fonts/unifont.ttf" );

	context = Rocket::Core::CreateContext( "default", Rocket::Core::Vector2i( cls.glconfig.vidWidth, cls.glconfig.vidHeight ) );

	//Rocket::Debugger::Initialise(context);

	

	Rocket::Core::ElementDocument* document = context->LoadDocument( "demo.rml" );

	if( document )
	{
		document->Show();
		document->RemoveReference();
	}
	else
	{
		Com_Printf( "Document is NULL\n");
	}
}

extern "C" void Rocket_Shutdown( void )
{
	if ( context )
	{
		context->RemoveReference();
		context = NULL;
	}
	Rocket::Core::Shutdown();
}

extern "C" void Rocket_Render( void )
{
	/*if ( context )
	{
		context->Render();
	}*/
}

extern "C" void Rocket_Update( void )
{
	//TODO: add mouse/key events
	/*if ( context )
	{
		context->Update();
	}*/
}
