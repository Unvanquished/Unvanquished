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
#include <Rocket/Core/Lua/Interpreter.h>
#include <Rocket/Core/Lua/LuaType.h>
#include <Rocket/Controls/Lua/Controls.h>
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
#include "rocketDataSource.h"
#include "rocketKeyBinder.h"
#include "rocketElementDocument.h"
#include "rocketChatField.h"
#include "rocketFormControlInput.h"
#include "rocketFormControlSelect.h"
#include "rocketConditionalElement.h"
#include "rocketColorInput.h"
#include "rocketIncludeElement.h"
#include "rocketCvarInlineElement.h"
#include <Rocket/Debugger.h>
#include "lua/Cvar.h"
#include "lua/Cmd.h"
#include "lua/Events.h"
#include "lua/Timer.h"
#include "../cg_local.h"

class DaemonFileInterface : public Rocket::Core::FileInterface
{
public:
	Rocket::Core::FileHandle Open( const Rocket::Core::String &filePath )
	{
		fileHandle_t fileHandle;
		trap_FS_FOpenFile( filePath.CString(), &fileHandle, FS_READ );
		return ( Rocket::Core::FileHandle )fileHandle;
	}

	void Close( Rocket::Core::FileHandle file )
	{
		trap_FS_FCloseFile( ( fileHandle_t ) file );
	}

	size_t Read( void *buffer, size_t size, Rocket::Core::FileHandle file )
	{
		return ( size_t ) trap_FS_Read( buffer, (int)size, ( fileHandle_t ) file );
	}

	// TODO
	bool Seek( Rocket::Core::FileHandle file, long offset, int origin )
	{
		return trap_FS_Seek( ( fileHandle_t ) file, offset, ( fsOrigin_t ) origin );
	}

	// TODO
	size_t Tell( Rocket::Core::FileHandle file )
	{
		return ( size_t ) trap_FS_Tell( ( fileHandle_t ) file );
	}

	/*
		May not be correct for files in zip files
	*/
	// TODO
	size_t Length( Rocket::Core::FileHandle file )
	{
		return ( size_t ) trap_FS_FileLength( ( fileHandle_t ) file );
	}
};

class DaemonSystemInterface : public Rocket::Core::SystemInterface
{
public:
	float GetElapsedTime()
	{
		return trap_Milliseconds() / 1000.0f;
	}

	int TranslateString( Rocket::Core::String &translated, const Rocket::Core::String &input )
	{
		const char* ret = _( input.CString() );
		translated = ret;
		return 0;
	}

	// TODO: Add explicit support for other log types
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
	polyVert_t *verts = new polyVert_t[ sizeof( polyVert_t ) * count ];

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

		this->indices = new int[ sizeof( int ) * _numIndicies ];
		Com_Memcpy( indices, _indices, _numIndicies * sizeof( int ) );

		this->shader = shader;
	}

	~RocketCompiledGeometry()
	{
		delete[] verts;
		delete[] indices;
	}
};

// HACK: Rocket uses a texturehandle of 0 when we really want the whiteImage shader
static qhandle_t whiteShader;

// TODO: CompileGeometry, RenderCompiledGeometry, ReleaseCompileGeometry ( use vbos and ibos )
class DaemonRenderInterface : public Rocket::Core::RenderInterface
{
public:
	DaemonRenderInterface() { }

	void RenderGeometry( Rocket::Core::Vertex *verticies,  int numVerticies, int *indices, int numIndicies, Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation )
	{
		polyVert_t *verts = createVertexArray( verticies, numVerticies );
		trap_R_Add2dPolysIndexedToScene( verts, numVerticies, indices, numIndicies, translation.x, translation.y, texture ? ( qhandle_t ) texture : whiteShader );
		delete[]  verts;
	}

	Rocket::Core::CompiledGeometryHandle CompileGeometry( Rocket::Core::Vertex *vertices, int num_vertices, int *indices, int num_indices, Rocket::Core::TextureHandle texture )
	{
		RocketCompiledGeometry *geometry = new RocketCompiledGeometry( vertices, num_vertices, indices, num_indices, texture ? ( qhandle_t ) texture : whiteShader );

		return Rocket::Core::CompiledGeometryHandle( geometry );

	}

	void RenderCompiledGeometry( Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f &translation )
	{
		RocketCompiledGeometry *g = ( RocketCompiledGeometry * ) geometry;
		trap_R_Add2dPolysIndexedToScene( g->verts, g->numVerts, g->indices, g->numIndicies, translation.x, translation.y, g->shader );
	}

	void ReleaseCompiledGeometry( Rocket::Core::CompiledGeometryHandle geometry )
	{
		RocketCompiledGeometry *g = ( RocketCompiledGeometry * ) geometry;
		delete g;
	}

	bool LoadTexture( Rocket::Core::TextureHandle& textureHandle, Rocket::Core::Vector2i& textureDimensions, const Rocket::Core::String& source )
	{
		qhandle_t shaderHandle = trap_R_RegisterShader( source.CString(), RSF_NOMIP );

		if ( shaderHandle == -1 )
		{
			return false;
		}

		// Find the size of the texture
		textureHandle = ( Rocket::Core::TextureHandle ) shaderHandle;
		trap_R_GetTextureSize( shaderHandle, &textureDimensions.x, &textureDimensions.y );
		return true;
	}
	bool GenerateTexture( Rocket::Core::TextureHandle& texture_handle, const byte* source, const Rocket::Core::Vector2i& source_dimensions, int )
	{

		texture_handle = trap_R_GenerateTexture( (const byte* )source, source_dimensions.x, source_dimensions.y );
// 		Com_DPrintf( "RE_GenerateTexture [ %lu ( %d x %d )]\n", textureHandle, sourceDimensions.x, sourceDimensions.y );

		return ( texture_handle > 0 );
	}

	void ReleaseTexture( Rocket::Core::TextureHandle )
	{
		// we free all textures after each map load, but this may have to be filled for the libRocket font system
	}

	void EnableScissorRegion( bool enable )
	{
		trap_R_ScissorEnable( enable );

	}

	void SetScissorRegion( int x, int y, int width, int height )
	{
		trap_R_ScissorSet( x, cgs.glconfig.vidHeight - ( y + height ), width, height );
	}
};

void Rocket_Rocket_f()
{
	std::string action = CG_Argv(1);
	Rocket_DocumentAction( action.c_str(), CG_Argv(2) );
}

void Rocket_RocketDebug_f()
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
		if ( !Q_stricmp( CG_Argv( 1 ), "hud" ) )
		{
			Rocket::Debugger::SetContext( hudContext );
		}
		else
		{
			Rocket::Debugger::SetContext( menuContext );
		}
		CG_SetKeyCatcher( trap_Key_GetCatcher() | KEYCATCH_UI );
	}
	else
	{
		CG_SetKeyCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
	}
}

void Rocket_Lua_f( void )
{
	Rocket::Core::Lua::Interpreter::DoString( CG_Argv( 1 ), "commandline" );
}

static DaemonFileInterface fileInterface;
static DaemonSystemInterface systemInterface;
static DaemonRenderInterface renderInterface;

static RocketFocusManager fm;

Rocket::Core::Context *menuContext = nullptr;
Rocket::Core::Context *hudContext = nullptr;

// TODO
// cvar_t *cg_draw2D;

void Rocket_Init()
{
	Rocket::Core::SetFileInterface( &fileInterface );
	Rocket::Core::SetSystemInterface( &systemInterface );
	Rocket::Core::SetRenderInterface( &renderInterface );

	if ( !Rocket::Core::Initialise() )
	{
		Com_Printf( "Could not init libRocket\n" );
		return;
	}

	// Initialize the controls plugin
	Rocket::Controls::Initialise();

	// Initialize Lua
	Rocket::Core::Lua::Interpreter::Initialise();
	Rocket::Controls::Lua::RegisterTypes(Rocket::Core::Lua::Interpreter::GetLuaState());
	Rocket::Core::Lua::LuaType<Rocket::Core::Lua::Cvar>::Register(Rocket::Core::Lua::Interpreter::GetLuaState());
	Rocket::Core::Lua::LuaType<Rocket::Core::Lua::Cmd>::Register(Rocket::Core::Lua::Interpreter::GetLuaState());
	Rocket::Core::Lua::LuaType<Rocket::Core::Lua::Events>::Register(Rocket::Core::Lua::Interpreter::GetLuaState());
	Rocket::Core::Lua::LuaType<Rocket::Core::Lua::Timer>::Register(Rocket::Core::Lua::Interpreter::GetLuaState());

	// Set backup font
	Rocket::Core::FontDatabase::SetBackupFace( "fonts/unifont.ttf" );

	// Initialize keymap
	Rocket_InitKeys();

	// Create the menu context
	menuContext = Rocket::Core::CreateContext( "menuContext", Rocket::Core::Vector2i( cgs.glconfig.vidWidth, cgs.glconfig.vidHeight ) );

	// Add the listener so we know where to give mouse/keyboard control to
	menuContext->GetRootElement()->AddEventListener( "show", &fm );
	menuContext->GetRootElement()->AddEventListener( "hide", &fm );
	menuContext->GetRootElement()->AddEventListener( "close", &fm );
	menuContext->GetRootElement()->AddEventListener( "load", &fm );

	// Create the HUD context
	hudContext = Rocket::Core::CreateContext( "hudContext", Rocket::Core::Vector2i( cgs.glconfig.vidWidth, cgs.glconfig.vidHeight ) );

	// Add custom client elements
	Rocket::Core::Factory::RegisterElementInstancer( "datagrid", new Rocket::Core::ElementInstancerGeneric< SelectableDataGrid >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "progressbar", new Rocket::Core::ElementInstancerGeneric< RocketProgressBar >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "dataselect", new Rocket::Core::ElementInstancerGeneric< RocketDataSelect >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "console_text", new Rocket::Core::ElementInstancerGeneric< RocketConsoleTextElement >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "datasource_single", new Rocket::Core::ElementInstancerGeneric< RocketDataSourceSingle >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "datasource", new Rocket::Core::ElementInstancerGeneric< RocketDataSource >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "circlemenu", new Rocket::Core::ElementInstancerGeneric< RocketCircleMenu >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "keybind", new Rocket::Core::ElementInstancerGeneric< RocketKeyBinder >() )->RemoveReference();
// 	Rocket::Core::Factory::RegisterElementInstancer( "body", new Rocket::Core::ElementInstancerGeneric< RocketElementDocument >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "chatfield", new Rocket::Core::ElementInstancerGeneric< RocketChatField >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "input", new Rocket::Core::ElementInstancerGeneric< CvarElementFormControlInput >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "select", new Rocket::Core::ElementInstancerGeneric< CvarElementFormControlSelect >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "if", new Rocket::Core::ElementInstancerGeneric< RocketConditionalElement >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "colorinput", new Rocket::Core::ElementInstancerGeneric< RocketColorInput >() )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "include", new Rocket::Core::ElementInstancerGeneric< RocketIncludeElement > () )->RemoveReference();
	Rocket::Core::Factory::RegisterElementInstancer( "inlinecvar", new Rocket::Core::ElementInstancerGeneric< RocketCvarInlineElement > () )->RemoveReference();

	whiteShader = trap_R_RegisterShader( "white", RSF_DEFAULT );
}

void Rocket_Shutdown()
{
	extern std::vector<RocketDataFormatter*> dataFormatterList;
	extern std::map<std::string, RocketDataGrid*> dataSourceMap;
	extern std::queue< RocketEvent_t* > eventQueue;

	if ( menuContext )
	{
		menuContext->RemoveReference();
		menuContext = nullptr;
	}

	if ( hudContext )
	{
		hudContext->RemoveReference();
		hudContext = nullptr;
	}

	Rocket::Core::Lua::Interpreter::Shutdown();
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

	trap_RemoveCommand( "rocket" );
	trap_RemoveCommand( "rocketDebug" );
}

void Rocket_Render()
{
	if ( cg_draw2D.integer && hudContext )
	{
		hudContext->Render();
	}

	// Render menus on top of the HUD
	if ( menuContext )
	{
		menuContext->Render();
	}

}

void Rocket_Update()
{
	// Mouse move is necessary to ensure the menus update, so fake one
	Rocket_MouseMove( 0, 0 );

	if ( menuContext )
	{
		menuContext->Update();
	}

	if ( hudContext )
	{
		hudContext->Update();
	}
	Rocket::Core::Lua::Timer::Update(rocketInfo.realtime);
}

static bool IsInvalidEmoticon( Rocket::Core::String emoticon )
{
	const char invalid[][2] = { "*", "/", "\\", ".", " ", "<", ">", "!", "@", "#", "$", "%", "^", "&", "(", ")", "-", "_", "+", "=", ",", "?", "[", "]", "{", "}", "|", ":", ";", "'", "\"", "`", "~" };

	for ( unsigned i = 0; i < ARRAY_LEN( invalid ); ++i )
	{
		if ( emoticon.Find( invalid[ i ] ) != Rocket::Core::String::npos )
		{
			return true;
		}
	}

	return false;
}

// TODO: Make this take Rocket::Core::String as an input.
Rocket::Core::String Rocket_QuakeToRML( const char *in, int parseFlags = 0 )
{
	const char *p;
	Rocket::Core::String out;
	Rocket::Core::String spanstr;
	bool span = false;
	bool spanHasContent = false;

	if ( !*in )
	{
		return "";
	}

	for ( p = in; p && *p; ++p )
	{
		if ( *p == '<' )
		{
			if ( span && !spanHasContent )
			{
				spanHasContent = true;
				out.Append( spanstr );
			}
			out.Append( "&lt;" );
		}
		else if ( *p == '>' )
		{
			if ( span && !spanHasContent )
			{
				spanHasContent = true;
				out.Append( spanstr );
			}
			out.Append( "&gt;" );
		}
		else if ( *p == '&' )
		{
			if ( span && !spanHasContent )
			{
				spanHasContent = true;
				out.Append( spanstr );
			}
			out.Append( "&amp;" );
		}
		else if ( *p == '\n' )
		{
			out.Append( span && spanHasContent ? "</span><br />" : "<br />" );
			span = false;
			spanHasContent = false;
		}
		else if ( Q_IsColorString( p ) )
		{
			if ( span && spanHasContent )
			{
				out.Append( "</span>" );
				span = false;
				spanHasContent = false;
			}

			char rgb[32];
			int code = ColorIndex( *++p );

			Com_sprintf( rgb, sizeof( rgb ), "<span style='color: #%02X%02X%02X;'>",
			          (int)( g_color_table[ code ][ 0 ] * 255 ),
			          (int)( g_color_table[ code ][ 1 ] * 255 ),
			          (int)( g_color_table[ code ][ 2 ] * 255 ) );

			// don't add the span yet, because it might be empty
			spanstr = rgb;

			span = true;
			spanHasContent = false;
		}
		else
		{
			if ( span && !spanHasContent )
			{
				out.Append( spanstr );
				spanHasContent = true;
			}
			out.Append( *p );
		}
	}

	if ( span && spanHasContent )
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

			// TODO: Dont hardcode the extension.
			path =  va( "emoticons/%s.crn", emoticon.CString() );
			if ( CG_FileExists( path ) )
			{
				out.Erase( openBracket, closeBracket - openBracket + 1 );
				path = va( "<img class='trem-emoticon' src='/emoticons/%s' />", emoticon.CString() );
				out.Insert( openBracket, path );
				currentPosition = openBracket + strlen( path );
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

void Rocket_LoadFont( const char *font )
{
	Rocket::Core::FontDatabase::LoadFontFace( font );
}
