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

// The basic interfaces for libRocket

#include "rocket.h"
#include <Rocket/Core/FileInterface.h>
#include <Rocket/Core/SystemInterface.h>
#include <Rocket/Core/RenderInterface.h>
#include <Rocket/Controls.h>
#include "rocketDataGrid.h"
#include "rocketDataFormatter.h"
#include "rocketEventInstancer.h"
#include "rocketSelectableDataGrid.h"
#include "rocketProgressBar.h"
#include "rocketDataSelect.h"
#include "rocketConsoleTextElement.h"
#include "rocketDataSourceSingle.h"
#include "rocketFocusManager.h"
#include "rocketCircleMenu.h"
#include "rocketKeyBinder.h"
#include "rocketElementDocument.h"
#include "rocketChatField.h"
#include "rocketFormControlInput.h"
#include "rocketFormControlSelect.h"
#include "rocketMiscText.h"
#include "rocketConditionalElement.h"
#include "rocketColorInput.h"
#include "rocketIncludeElement.h"
#include "rocketCvarInlineElement.h"
#include "client.h"
#include <Rocket/Debugger.h>

extern "C"
{
	#include <SDL.h>
}


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

	bool Seek( Rocket::Core::FileHandle file, long offset, int origin )
	{
		int ret = FS_Seek( ( fileHandle_t ) file, offset, origin );

		return ( ret >= 0 );
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
		const char* ret = Trans_GettextGame( input.CString() );
		translated = ret;
		return 0;
	}

	//TODO: Add explicit support for other log types
	bool LogMessage( Rocket::Core::Log::Type type, const Rocket::Core::String &message )
	{
		switch ( type )
		{
			case Rocket::Core::Log::LT_ALWAYS :
				Com_Printf( "ALWAYS: %s\n", message.CString() );
				break;
			case Rocket::Core::Log::LT_ERROR :
				Com_Printf( "ERROR: %s\n", message.CString() );
				break;
			case Rocket::Core::Log::LT_WARNING :
				Com_Printf( "WARNING: %s\n", message.CString() );
				break;
			case Rocket::Core::Log::LT_INFO :
				Com_Printf( "INFO: %s\n", message.CString() );
				break;
			default:
				Com_Printf( "%s\n", message.CString() );
		}
		return true;
	}
};

static polyVert_t *createVertexArray( Rocket::Core::Vertex *vertices, int count )
{
	polyVert_t *verts = static_cast<polyVert_t *>( Z_Malloc( sizeof( polyVert_t ) * count ) );

	for ( int i = 0; i < count; i++ )
	{
		polyVert_t &polyVert = verts[ i ];
		Rocket::Core::Vertex &vert = vertices[ i ];

		Vector2Copy( vert.position, polyVert.xyz );

		polyVert.modulate[ 0 ] = vert.colour.red;
		polyVert.modulate[ 1 ] = vert.colour.green;
		polyVert.modulate[ 2 ] = vert.colour.blue;
		polyVert.modulate[ 3 ] = vert.colour.alpha;

		Vector2Copy( vert.tex_coord, polyVert.st );
	}

	return verts;
}

class RocketCompiledGeometry
{
public:
	polyVert_t *verts;
	int         numVerts;
	int        *indices;
	int         numIndicies;
	qhandle_t   shader;

	RocketCompiledGeometry( Rocket::Core::Vertex *verticies, int numVerticies, int *_indices, int _numIndicies, qhandle_t shader ) : numVerts( numVerticies ), numIndicies( _numIndicies )
	{
		this->verts = createVertexArray( verticies, numVerticies );

		this->indices = ( int * ) Z_Malloc( sizeof( int ) * _numIndicies );
		Com_Memcpy( indices, _indices, _numIndicies * sizeof( int ) );

		this->shader = shader;
	}

	~RocketCompiledGeometry()
	{
		Z_Free( verts );
		Z_Free( indices );
	}
};

// HACK: Rocket uses a texturehandle of 0 when we really want the whiteImage shader
static qhandle_t whiteShader;

//TODO: CompileGeometry, RenderCompiledGeometry, ReleaseCompileGeometry ( use vbos and ibos )
class DaemonRenderInterface : public Rocket::Core::RenderInterface
{
public:
	DaemonRenderInterface() { }

	void RenderGeometry( Rocket::Core::Vertex *verticies,  int numVerticies, int *indices, int numIndicies, Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation )
	{
		polyVert_t *verts = createVertexArray( verticies, numVerticies );

		re.Add2dPolysIndexed( verts, numVerticies, indices, numIndicies, translation.x, translation.y, texture ? ( qhandle_t ) texture : whiteShader );

		Z_Free( verts );
	}

	Rocket::Core::CompiledGeometryHandle CompileGeometry( Rocket::Core::Vertex *vertices, int num_vertices, int *indices, int num_indices, Rocket::Core::TextureHandle texture )
	{
		RocketCompiledGeometry *geometry = new RocketCompiledGeometry( vertices, num_vertices, indices, num_indices, texture ? ( qhandle_t ) texture : whiteShader );

		return Rocket::Core::CompiledGeometryHandle( geometry );

	}

	void RenderCompiledGeometry( Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f &translation )
	{
		RocketCompiledGeometry *g = ( RocketCompiledGeometry * ) geometry;

		re.Add2dPolysIndexed( g->verts, g->numVerts, g->indices, g->numIndicies, translation.x, translation.y, g->shader );
	}

	void ReleaseCompiledGeometry( Rocket::Core::CompiledGeometryHandle geometry )
	{
		RocketCompiledGeometry *g = ( RocketCompiledGeometry * ) geometry;
		delete g;
	}

	bool LoadTexture( Rocket::Core::TextureHandle& textureHandle, Rocket::Core::Vector2i& textureDimensions, const Rocket::Core::String& source )
	{
		qhandle_t shaderHandle = re.RegisterShader( source.CString(), RSF_NOMIP );

		if ( shaderHandle == -1 )
		{
			return false;
		}

		// Find the size of the texture
		textureHandle = ( Rocket::Core::TextureHandle ) shaderHandle;
		re.GetTextureSize( shaderHandle, &textureDimensions.x, &textureDimensions.y );
		return true;
	}

	bool GenerateTexture( Rocket::Core::TextureHandle& textureHandle, const Rocket::Core::byte* source, const Rocket::Core::Vector2i& sourceDimensions )
	{

		textureHandle = re.GenerateTexture( (const byte* )source, sourceDimensions.x, sourceDimensions.y );
		Com_DPrintf( "RE_GenerateTexture [ %lu ( %d x %d )]\n", textureHandle, sourceDimensions.x, sourceDimensions.y );

		return ( textureHandle > 0 );
	}

	void ReleaseTexture( Rocket::Core::TextureHandle textureHandle )
	{
		// we free all textures after each map load, but this may have to be filled for the libRocket font system
	}

	void EnableScissorRegion( bool enable )
	{
		re.ScissorEnable( enable ? qtrue :  qfalse );

	}

	void SetScissorRegion( int x, int y, int width, int height )
	{
		re.ScissorSet( x, cls.glconfig.vidHeight - ( y + height ), width, height );
	}
};

// Rocket::Core::Input::KeyModifier RocketConvertSDLmod( SDLMod sdl )
// {
// 	using namespace Rocket::Core::Input;
//
// 	int mod = 0;
// 	if( sdl & KMOD_SHIFT )	mod |= KM_SHIFT;
// 	if( sdl & KMOD_CTRL )	mod |= KM_CTRL;
// 	if( sdl & KMOD_ALT )	mod |= KM_ALT;
// 	if( sdl & KMOD_META )	mod |= KM_META;
// 	if( sdl & KMOD_CAPS )	mod |= KM_CAPSLOCK;
// 	if( sdl & KMOD_NUM )	mod |= KM_NUMLOCK;
//
// 	return KeyModifier( mod );
// }


void Rocket_Rocket_f( void )
{
	Rocket_DocumentAction( Cmd_Argv(1), Cmd_Argv(2) );
}

void Rocket_RocketDebug_f( void )
{
	static bool init = false;

	if ( !init )
	{
		Rocket::Debugger::Initialise(menuContext);
		init = true;
	}

	Rocket::Debugger::SetVisible( !Rocket::Debugger::IsVisible() );

	if ( Rocket::Debugger::IsVisible() )
	{
		if ( !Q_stricmp( Cmd_Argv( 1 ), "hud" ) )
		{
			Rocket::Debugger::SetContext( hudContext );
		}
		else
		{
			Rocket::Debugger::SetContext( menuContext );
		}
		Key_SetCatcher( Key_GetCatcher() | KEYCATCH_UI );
	}
	else
	{
		Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_UI );
	}
}

void Rocket_PrintKeys_f( void )
{
	for ( int i = 0; i < MAX_KEYS; ++i )
	{
		Com_Printf("%d -> %s\n", i, Key_KeynumToString(i));
	}
}

static DaemonFileInterface fileInterface;
static DaemonSystemInterface systemInterface;
static DaemonRenderInterface renderInterface;

static RocketFocusManager fm;

Rocket::Core::Context *menuContext = NULL;
Rocket::Core::Context *hudContext = NULL;

void Rocket_Init( void )
{
	char **fonts;
	int numFiles;

	Rocket::Core::SetFileInterface( &fileInterface );
	Rocket::Core::SetSystemInterface( &systemInterface );
	Rocket::Core::SetRenderInterface( &renderInterface );

	if ( !Rocket::Core::Initialise() )
	{
		Com_Printf( "Could not init libRocket\n" );
		return;
	}

	Rocket::Controls::Initialise();

	// Set backup font
	Rocket::Core::FontDatabase::SetBackupFace( "fonts/unifont.ttf" );

	// Load all fonts in the fonts/ dir...
	fonts = FS_ListFiles( "fonts/", ".ttf", &numFiles );
	for ( int i = 0; i < numFiles; ++i )
	{
		Rocket::Core::FontDatabase::LoadFontFace( va( "fonts/%s", fonts[ i ] ) );
	}

	FS_FreeFileList( fonts );

	// Now get all the otf fonts...
	fonts = FS_ListFiles( "fonts/", ".otf", &numFiles );
	for ( int i = 0; i < numFiles; ++i )
	{
		Rocket::Core::FontDatabase::LoadFontFace( va( "fonts/%s", fonts[ i ] ) );
	}

	FS_FreeFileList( fonts );

	// Initialize keymap
	Rocket_InitKeys();

	// Create the menu context
	menuContext = Rocket::Core::CreateContext( "menuContext", Rocket::Core::Vector2i( cls.glconfig.vidWidth, cls.glconfig.vidHeight ) );

	// Add the listener so we know where to give mouse/keyboard control to
	menuContext->GetRootElement()->AddEventListener( "show", &fm );
	menuContext->GetRootElement()->AddEventListener( "hide", &fm );
	menuContext->GetRootElement()->AddEventListener( "close", &fm );
	menuContext->GetRootElement()->AddEventListener( "load", &fm );

	// Create the HUD context
	hudContext = Rocket::Core::CreateContext( "hudContext", Rocket::Core::Vector2i( cls.glconfig.vidWidth, cls.glconfig.vidHeight ) );

	// Add the event listener instancer
	EventInstancer* event_instancer = new EventInstancer();
	Rocket::Core::Factory::RegisterEventListenerInstancer( event_instancer );
	event_instancer->RemoveReference();

	// Add custom client elements
	Rocket::Core::Factory::RegisterElementInstancer( "datagrid", new Rocket::Core::ElementInstancerGeneric< SelectableDataGrid >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "progressbar", new Rocket::Core::ElementInstancerGeneric< RocketProgressBar >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "dataselect", new Rocket::Core::ElementInstancerGeneric< RocketDataSelect >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "console_text", new Rocket::Core::ElementInstancerGeneric< RocketConsoleTextElement >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "datasource_single", new Rocket::Core::ElementInstancerGeneric< RocketDataSourceSingle >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "circlemenu", new Rocket::Core::ElementInstancerGeneric< RocketCircleMenu >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "keybind", new Rocket::Core::ElementInstancerGeneric< RocketKeyBinder >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "body", new Rocket::Core::ElementInstancerGeneric< RocketElementDocument >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "chatfield", new Rocket::Core::ElementInstancerGeneric< RocketChatField >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "misc_text", new Rocket::Core::ElementInstancerGeneric< RocketMiscText >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "input", new Rocket::Core::ElementInstancerGeneric< CvarElementFormControlInput >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "select", new Rocket::Core::ElementInstancerGeneric< CvarElementFormControlSelect >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "if", new Rocket::Core::ElementInstancerGeneric< RocketConditionalElement >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "colorinput", new Rocket::Core::ElementInstancerGeneric< RocketColorInput >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "include", new Rocket::Core::ElementInstancerGeneric< RocketIncludeElement > () )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "inlinecvar", new Rocket::Core::ElementInstancerGeneric< RocketCvarInlineElement > () )->RemoveReference();

	Cmd_AddCommand( "rocket", Rocket_Rocket_f );
	Cmd_AddCommand( "rocketDebug", Rocket_RocketDebug_f );
	Cmd_AddCommand( "printkeys", Rocket_PrintKeys_f );
	whiteShader = re.RegisterShader( "white", RSF_DEFAULT );
}

void Rocket_Shutdown( void )
{
	extern std::vector<RocketDataFormatter*> dataFormatterList;
	extern std::map<std::string, RocketDataGrid*> dataSourceMap;
	extern std::queue< RocketEvent_t* > eventQueue;

	if ( menuContext )
	{
		menuContext->RemoveReference();
		menuContext = NULL;
	}

	if ( hudContext )
	{
		hudContext->RemoveReference();
		hudContext = NULL;
	}

	Rocket::Core::Shutdown();

	// Prevent memory leaks

	for ( size_t i = 0; i < dataFormatterList.size(); ++i )
	{
		delete dataFormatterList[i];
	}

	dataFormatterList.clear();

	for ( std::map<std::string, RocketDataGrid*>::iterator it = dataSourceMap.begin(); it != dataSourceMap.end(); ++it )
	{
		delete it->second;
	}

	dataSourceMap.clear();

	while ( !eventQueue.empty() )
	{
		delete eventQueue.front();
		eventQueue.pop();
	}

	Cmd_RemoveCommand( "rocket" );
	Cmd_RemoveCommand( "rocketDebug" );
}

void Rocket_Render( void )
{
	if ( hudContext )
	{
		hudContext->Render();
	}

	// Render menus on top of the HUD
	if ( menuContext )
	{
		menuContext->Render();
	}

}

void Rocket_Update( void )
{
	if ( menuContext )
	{
		menuContext->Update();
	}

	if ( hudContext )
	{
		hudContext->Update();
	}
}

static bool IsInvalidEmoticon( Rocket::Core::String emoticon )
{
	const char invalid[][2] = { "*", "/", "\\", ".", " ", "<", ">", "!", "@", "#", "$", "%", "^", "&", "(", ")", "-", "_", "+", "=", ",", "?", "[", "]", "{", "}", "|", ":", ";", "'", "\"", "`", "~" };

	for ( int i = 0; i < ARRAY_LEN( invalid ); ++i )
	{
		if ( emoticon.Find( invalid[ i ] ) != Rocket::Core::String::npos )
		{
			return true;
		}
	}

	return false;
}

Rocket::Core::String Rocket_QuakeToRML( const char *in, int parseFlags = 0 )
{
	const char *p;
	Rocket::Core::String out;
	qboolean span = qfalse;

	if ( !*in )
	{
		return "";
	}

	for ( p = in; p && *p; ++p )
	{
		if ( *p == '<' )
		{
			out.Append( "&lt;" );
		}
		else if ( *p == '>' )
		{
			out.Append( "&gt;" );
		}
		else if ( *p == '&' )
		{
			out.Append( "&amp;" );
		}
		else if ( *p == '\n' )
		{
			out.Append( span ? "</span><br />" : "<br />" );
			span = qfalse;
		}
		else if ( Q_IsColorString( p ) )
		{
			if ( span )
			{
				out.Append( "</span>" );
			}

			char rgb[32];
			int code = ColorIndex( *++p );

			Com_sprintf( rgb, sizeof( rgb ), "<span style='color: #%02X%02X%02X;'>",
			          (int)( g_color_table[ code ][ 0 ] * 255 ),
			          (int)( g_color_table[ code ][ 1 ] * 255 ),
			          (int)( g_color_table[ code ][ 2 ] * 255 ) );

			out.Append( rgb );

			span = qtrue;
		}
		else
		{
			out.Append( *p );
		}
	}
	if ( span )
	{
		out.Append( "</span>" );
	}

	// ^^ -> ^
	while ( out.Find( "^^" ) != Rocket::Core::String::npos )
	{
		out = out.Replace( "^^", "^" );
	}

	if ( parseFlags & RP_EMOTICONS )
	{
		// Parse emoticons
		size_t openBracket = 0;
		size_t closeBracket = 0;
		size_t currentPosition = 0;

		while ( 1 )
		{
			Rocket::Core::String emoticon;
			const char *path;

			openBracket = out.Find( "[", currentPosition );
			if ( openBracket == Rocket::Core::String::npos )
			{
				break;
			}

			closeBracket = out.Find( "]", openBracket );
			if ( closeBracket == Rocket::Core::String::npos )
			{
				break;
			}

			emoticon = out.Substring( openBracket + 1, closeBracket - openBracket - 1 );

			// Certain characters are invalid
			if ( emoticon.Empty() || IsInvalidEmoticon( emoticon ) )
			{
				currentPosition = closeBracket + 1;
				continue;
			}

			path =  va( "emoticons/%s_1x1.crn", emoticon.CString() );
			if ( FS_FOpenFileRead( path, NULL, qtrue ) )
			{
				out.Erase( openBracket, closeBracket - openBracket + 1 );
				path = va( "<img class='trem-emoticon' src='/emoticons/%s_1x1.crn' />", emoticon.CString() );
				out.Insert( openBracket, path );
				currentPosition = openBracket + strlen( path ) + 1;
			}
			else
			{
				currentPosition = closeBracket + 1;
			}
		}
	}

	return out;
}

void Rocket_QuakeToRMLBuffer( const char *in, char *out, int length )
{
	Q_strncpyz( out, Rocket_QuakeToRML( in, RP_EMOTICONS ).CString(), length );
}

void Rocket_SetActiveContext( int catcher )
{
	switch ( catcher )
	{
		case KEYCATCH_UI:
			menuContext->ShowMouseCursor( true );
			break;

		default:
			if ( !( catcher & KEYCATCH_CONSOLE ) )
			{
				menuContext->ShowMouseCursor( false );
			}

			break;
	}
}
