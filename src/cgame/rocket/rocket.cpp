/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// The basic interfaces for libRocket

#include "common/Common.h"
#include "rocket.h"
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Controls.h>
#include <RmlUi/Core/Lua/Interpreter.h>
#include <RmlUi/Core/Lua/LuaType.h>
#include <RmlUi/Controls/Lua/Controls.h>
#include "rocketDataGrid.h"
#include "rocketDataFormatter.h"
#include "rocketEventInstancer.h"
#include "rocketSelectableDataGrid.h"
#include "rocketProgressBar.h"
#include "rocketDataSelect.h"
#include "rocketConsoleTextElement.h"
#include "rocketDataSourceSingle.h"
#include "rocketFocusManager.h"
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
#include "lua/CDataSource.h"
#include <RmlUi/Debugger.h>
#include "lua/Cvar.h"
#include "lua/Cmd.h"
#include "lua/Events.h"
#include "lua/Timer.h"
#include "../cg_local.h"

class DaemonFileInterface : public Rml::Core::FileInterface
{
public:
	Rml::Core::FileHandle Open( const Rml::Core::String &filePath )
	{
		fileHandle_t fileHandle;
		trap_FS_OpenPakFile( filePath.c_str(), fileHandle );
		return ( Rml::Core::FileHandle )fileHandle;
	}

	void Close( Rml::Core::FileHandle file )
	{
		trap_FS_FCloseFile( ( fileHandle_t ) file );
	}

	size_t Read( void *buffer, size_t size, Rml::Core::FileHandle file )
	{
		return ( size_t ) trap_FS_Read( buffer, (int)size, ( fileHandle_t ) file );
	}

	// TODO
	bool Seek( Rml::Core::FileHandle file, long offset, int origin )
	{
		return trap_FS_Seek( ( fileHandle_t ) file, offset, ( fsOrigin_t ) origin );
	}

	// TODO
	size_t Tell( Rml::Core::FileHandle file )
	{
		return ( size_t ) trap_FS_Tell( ( fileHandle_t ) file );
	}

	/*
		May not be correct for files in zip files
	*/
	// TODO
	size_t Length( Rml::Core::FileHandle file )
	{
		return ( size_t ) trap_FS_FileLength( ( fileHandle_t ) file );
	}
};

class DaemonSystemInterface : public Rml::Core::SystemInterface
{
public:
	double GetElapsedTime() override
	{
		return trap_Milliseconds() / 1000.0f;
	}

	// TODO: Add explicit support for other log types
	bool LogMessage( Rml::Core::Log::Type type, const Rml::Core::String &message ) override
	{
		switch ( type )
		{
			case Rml::Core::Log::LT_ALWAYS :
			case Rml::Core::Log::LT_ERROR :
			case Rml::Core::Log::LT_WARNING :
				Log::Warn( message.c_str() );
				break;
			default:
			case Rml::Core::Log::LT_INFO :
				Log::Notice( message.c_str() );
				break;
		}
		return true;
	}

	void SetMouseCursor(const Rml::Core::String& cursor_name) override
	{
		if ( cursor_name.empty() )
		{
			// HACK: ignore this and keep using the previous cursor
			// RmlUi wants to get rid of the cursor when it is not above a document, but
			// we want to keep it whenever there is any menu open. For example the team
			// selection menu which only covers a small part of the screen.
			return;
		}
		Log::Verbose("Loading new cursor: %s", cursor_name);
		if ( !CG_Rocket_LoadCursor( cursor_name ) )
		{
			Log::Warn( "Error loading cursor: %s", cursor_name );
		}
	}
};

static polyVert_t *createVertexArray( Rml::Core::Vertex *vertices, int count )
{
	polyVert_t *verts = new polyVert_t[ count ];

	for ( int i = 0; i < count; i++ )
	{
		polyVert_t &polyVert = verts[ i ];
		Rml::Core::Vertex &vert = vertices[ i ];

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

	RocketCompiledGeometry( Rml::Core::Vertex *verticies, int numVerticies, int *_indices, int _numIndicies, qhandle_t shader ) : numVerts( numVerticies ), numIndicies( _numIndicies )
	{
		this->verts = createVertexArray( verticies, numVerticies );

		this->indices = new int[ _numIndicies ];
		memcpy( indices, _indices, _numIndicies * sizeof( int ) );

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
class DaemonRenderInterface : public Rml::Core::RenderInterface
{
public:
	DaemonRenderInterface() { }

	void RenderGeometry( Rml::Core::Vertex *verticies,  int numVerticies, int *indices, int numIndicies, Rml::Core::TextureHandle texture, const Rml::Core::Vector2f& translation ) override
	{
		polyVert_t *verts = createVertexArray( verticies, numVerticies );
		trap_R_Add2dPolysIndexedToScene( verts, numVerticies, indices, numIndicies, translation.x, translation.y, texture ? ( qhandle_t ) texture : whiteShader );
		delete[]  verts;
	}

	Rml::Core::CompiledGeometryHandle CompileGeometry( Rml::Core::Vertex *vertices, int num_vertices, int *indices, int num_indices, Rml::Core::TextureHandle texture ) override
	{
		RocketCompiledGeometry *geometry = new RocketCompiledGeometry( vertices, num_vertices, indices, num_indices, texture ? ( qhandle_t ) texture : whiteShader );

		return Rml::Core::CompiledGeometryHandle( geometry );

	}

	void RenderCompiledGeometry( Rml::Core::CompiledGeometryHandle geometry, const Rml::Core::Vector2f &translation ) override
	{
		RocketCompiledGeometry *g = ( RocketCompiledGeometry * ) geometry;
		trap_R_Add2dPolysIndexedToScene( g->verts, g->numVerts, g->indices, g->numIndicies, translation.x, translation.y, g->shader );
	}

	void ReleaseCompiledGeometry( Rml::Core::CompiledGeometryHandle geometry ) override
	{
		RocketCompiledGeometry *g = ( RocketCompiledGeometry * ) geometry;
		delete g;
	}

	bool LoadTexture( Rml::Core::TextureHandle& textureHandle, Rml::Core::Vector2i& textureDimensions, const Rml::Core::String& source ) override
	{
		qhandle_t shaderHandle = trap_R_RegisterShader( source.c_str(), RSF_NOMIP );

		if ( shaderHandle <= 0 )
		{
			return false;
		}

		// Find the size of the texture
		textureHandle = ( Rml::Core::TextureHandle ) shaderHandle;
		trap_R_GetTextureSize( shaderHandle, &textureDimensions.x, &textureDimensions.y );
		return true;
	}
	bool GenerateTexture( Rml::Core::TextureHandle& texture_handle, const byte* source, const Rml::Core::Vector2i& source_dimensions ) override
	{

		texture_handle = trap_R_GenerateTexture( (const byte* )source, source_dimensions.x, source_dimensions.y );
// 		Log::Debug( "RE_GenerateTexture [ %lu ( %d x %d )]", textureHandle, sourceDimensions.x, sourceDimensions.y );

		return ( texture_handle > 0 );
	}

	void ReleaseTexture( Rml::Core::TextureHandle ) override
	{
		// we free all textures after each map load, but this may have to be filled for the libRocket font system
	}

	void EnableScissorRegion( bool enable ) override
	{
		trap_R_ScissorEnable( enable );

	}

	void SetScissorRegion( int x, int y, int width, int height ) override
	{
		trap_R_ScissorSet( x, cgs.glconfig.vidHeight - ( y + height ), width, height );
	}

	void SetTransform( const Rml::Core::Matrix4f* ) override
	{
		// TODO: implement.
		Log::Warn("Transforms for RmlUi not yet implemented");
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
		Rml::Debugger::Initialise(menuContext);
		init = true;
	}

	Rml::Debugger::SetVisible( !Rml::Debugger::IsVisible() );

	if ( Rml::Debugger::IsVisible() )
	{
		if ( !Q_stricmp( CG_Argv( 1 ), "hud" ) )
		{
			Rml::Debugger::SetContext( hudContext );
		}
		else
		{
			Rml::Debugger::SetContext( menuContext );
		}
		CG_SetKeyCatcher( rocketInfo.keyCatcher | KEYCATCH_UI );
	}
	else
	{
		CG_SetKeyCatcher( rocketInfo.keyCatcher & ~KEYCATCH_UI );
	}
}

void Rocket_Lua_f( void )
{
	Rml::Core::Lua::Interpreter::DoString( CG_Argv( 1 ), "commandline" );
}

static DaemonFileInterface fileInterface;
static DaemonSystemInterface systemInterface;
static DaemonRenderInterface renderInterface;

static RocketFocusManager fm;

Rml::Core::Context *menuContext = nullptr;
Rml::Core::Context *hudContext = nullptr;

Rml::Core::PropertyId UnvPropertyId::Orientation;

// TODO
// cvar_t *cg_draw2D;

void Rocket_Init()
{
	Rml::Core::SetFileInterface( &fileInterface );
	Rml::Core::SetSystemInterface( &systemInterface );
	Rml::Core::SetRenderInterface( &renderInterface );

	if ( !Rml::Core::Initialise() )
	{
		Log::Notice( "Could not init libRocket\n" );
		return;
	}

	// Initialize the controls plugin
	Rml::Controls::Initialise();

	// Initialize Lua
	Rml::Core::Lua::Interpreter::Initialise();
	Rml::Core::Lua::Interpreter::DoString("math.randomseed(os.time())");
	Rml::Controls::Lua::RegisterTypes(Rml::Core::Lua::Interpreter::GetLuaState());
	Rml::Core::Lua::LuaType<Rml::Core::Lua::Cvar>::Register(Rml::Core::Lua::Interpreter::GetLuaState());
	Rml::Core::Lua::LuaType<Rml::Core::Lua::Cmd>::Register(Rml::Core::Lua::Interpreter::GetLuaState());
	Rml::Core::Lua::LuaType<Rml::Core::Lua::Events>::Register(Rml::Core::Lua::Interpreter::GetLuaState());
	Rml::Core::Lua::LuaType<Rml::Core::Lua::Timer>::Register(Rml::Core::Lua::Interpreter::GetLuaState());
	CG_Rocket_RegisterLuaCDataSource(Rml::Core::Lua::Interpreter::GetLuaState());

	// Register custom properties.
	UnvPropertyId::Orientation = Rml::Core::StyleSheetSpecification::RegisterProperty("orientation", "left", false, true)
		.AddParser("keyword", "left, right, up, down")
		.GetId();

	// Set backup font
	Rml::Core::GetFontEngineInterface()->LoadFontFace( "fonts/unifont.ttf", /*fallback_face=*/true );

	// Initialize keymap
	Rocket_InitKeys();

	// Create the menu context
	menuContext = Rml::Core::CreateContext( "menuContext", Rml::Core::Vector2i( cgs.glconfig.vidWidth, cgs.glconfig.vidHeight ) );
	// Allow this context to set the mouse cursor.
	menuContext->EnableMouseCursor( true );

	// Add the listener so we know where to give mouse/keyboard control to
	menuContext->GetRootElement()->AddEventListener( "show", &fm, true );
	menuContext->GetRootElement()->AddEventListener( "hide", &fm, true );
	menuContext->GetRootElement()->AddEventListener( "close", &fm, true );
	menuContext->GetRootElement()->AddEventListener( "load", &fm, true );

	// Create the HUD context
	hudContext = Rml::Core::CreateContext( "hudContext", Rml::Core::Vector2i( cgs.glconfig.vidWidth, cgs.glconfig.vidHeight ) );
	// HUDs do not get to interact with the mouse. In fact, we do not even inject mouse events.
	hudContext->EnableMouseCursor( false );

	// Add custom client elements
	RegisterElement<SelectableDataGrid>( "datagrid" );
	RegisterElement<RocketProgressBar>( "progressbar" );
	RegisterElement<RocketDataSelect>( "dataselect" );
	RegisterElement<RocketConsoleTextElement>( "console_text" );
	RegisterElement<RocketDataSourceSingle>( "datasource_single" );
	RegisterElement<RocketDataSource>( "datasource" );
	RegisterElement<RocketKeyBinder>( "keybind" );
	RegisterElement<RocketChatField>( "chatfield" );
	RegisterElement<CvarElementFormControlInput>( "input" );
	RegisterElement<CvarElementFormControlSelect>( "select" );
	RegisterElement<RocketConditionalElement>( "if" );
	RegisterElement<RocketColorInput>( "colorinput" );
	RegisterElement<RocketIncludeElement>( "include" );
	RegisterElement<RocketCvarInlineElement>( "inlinecvar" );

	whiteShader = trap_R_RegisterShader( "gfx/colors/white", RSF_DEFAULT );

	CG_FocusEvent( rocketInfo.keyCatcher & KEYCATCH_CONSOLE ? false : true );
}

void Rocket_Shutdown()
{
	// Only try to destroy the librocket stuff if the cgame is a DLL.
	// The destructors crash a lot and we don't want that to be a problem for people just playing the game.
	// If the cgame is a process, freeing the memory is just pedantic since it is about to terminate.
	// But if it is a DLL, shutdown is necessary because the memory is otherwise leaked and also
	// libRocket has assertions in the global destructors that fire if things are not shut down
	// (global destructors only run if the cgame is a DLL).
#ifdef BUILD_VM_IN_PROCESS
	extern std::vector<RocketDataFormatter*> dataFormatterList;
	extern std::map<std::string, RocketDataGrid*> dataSourceMap;
	extern std::queue< RocketEvent_t* > eventQueue;

	if ( menuContext )
	{
		if (! Rml::Core::RemoveContext( "menuContext" ) )
		{
			Log::Warn( "Could not remove menuContext" );
		}
		menuContext = nullptr;
	}

	if ( hudContext )
	{
		if (! Rml::Core::RemoveContext( "hudContext" ) )
		{
			Log::Warn( "Could not remove hudContext" );
		}
		hudContext = nullptr;
	}

	for ( size_t i = 0; i < dataFormatterList.size(); ++i )
	{
		// This also does crazy things in the destructor
		delete dataFormatterList[i];
	}
	dataFormatterList.clear();

	Rml::Core::Shutdown();

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
#endif // BUILD_VM_IN_PROCESS

	trap_RemoveCommand( "rocket" );
	trap_RemoveCommand( "rocketDebug" );
}

static bool drawMenu;

void Rocket_Render()
{
	// cg.snap is not available on the first frame but some HUD code accesses it
	if ( cg_draw2D.Get() && hudContext && cg.snap )
	{
		hudContext->Render();
	}

	// Render menus on top of the HUD
	if ( drawMenu && menuContext )
	{
		menuContext->Render();
	}

}

void Rocket_Update()
{
	if ( drawMenu && menuContext )
	{
		menuContext->Update();
	}

	if ( cg_draw2D.Get() && hudContext && cg.snap )
	{
		hudContext->Update();
	}
	Rml::Core::Lua::Timer::Update(rocketInfo.realtime);
}

std::string CG_EscapeHTMLText( Str::StringRef text )
{
	std::string out;
	for (char c : text) {
		switch (c) {
		case '<':
			out += "&lt;";
			break;
		case '>':
			out += "&gt;";
			break;
		case '&':
			out += "&amp;";
			break;
		default:
			out += c;
			break;
		}
	}
	return out;
}

// TODO: Make this take Rml::Core::String as an input.
Rml::Core::String Rocket_QuakeToRML( const char *in, int parseFlags = 0 )
{
	Rml::Core::String out;
	Rml::Core::String spanstr;
	bool span = false;
	bool spanHasContent = false;

	Color::Parser parser(in, Color::Color());
	for ( auto iter = parser.begin(); iter != parser.end(); ++iter )
	{
		const Color::Token& token = *iter;
		if ( token.Type() == Color::Token::TokenType::CHARACTER )
		{
			char c = *token.Begin();
			const emoticonData_t *emoticon;
			if ( c == '<' )
			{
				if ( span && !spanHasContent )
				{
					spanHasContent = true;
					out.append( spanstr );
				}
				out.append( "&lt;" );
			}
			else if ( c == '>' )
			{
				if ( span && !spanHasContent )
				{
					spanHasContent = true;
					out.append( spanstr );
				}
				out.append( "&gt;" );
			}
			else if ( c == '&' )
			{
				if ( span && !spanHasContent )
				{
					spanHasContent = true;
					out.append( spanstr );
				}
				out.append( "&amp;" );
			}
			else if ( c == '\n' )
			{
				out.append( span && spanHasContent ? "</span><br />" : "<br />" );
				span = false;
				spanHasContent = false;
			}
			else if ( ( parseFlags & RP_EMOTICONS ) && ( emoticon = BG_EmoticonAt( token.Begin() ) ) )
			{
				if ( span && !spanHasContent )
				{
					spanHasContent = true;
					out.append( spanstr );
				}
				out.append( va( "<img class='emoticon' src='/%s' />", emoticon->imageFile.c_str() ) );
				while ( *iter->Begin() != ']' )
				{
					++iter;
				}
			}
			else
			{
				if ( span && !spanHasContent )
				{
					out.append( spanstr );
					spanHasContent = true;
				}
				out.append( token.Begin(), token.Size() );
			}
		}
		else if ( token.Type() == Color::Token::TokenType::COLOR )
		{
			if ( span && spanHasContent )
			{
				out.append( "</span>" );
				span = false;
				spanHasContent = false;
			}

			if ( token.Color().Alpha() != 0  )
			{
				char rgb[32];
				Color::Color32Bit color32 = token.Color();
				Com_sprintf( rgb, sizeof( rgb ), "<span style='color: #%02X%02X%02X;'>",
						(int) color32.Red(),
						(int) color32.Green(),
						(int) color32.Blue() );

				// don't add the span yet, because it might be empty
				spanstr = rgb;

				span = true;
				spanHasContent = false;
			}
		}
		else if ( token.Type() == Color::Token::TokenType::ESCAPE )
		{
			if ( span && !spanHasContent )
			{
				out.append( spanstr );
				spanHasContent = true;
			}
			out.append( 1, Color::Constants::ESCAPE );
		}
	}

	if ( span && spanHasContent )
	{
		out.append( "</span>" );
	}

	return out;
}

void Rocket_QuakeToRMLBuffer( const char *in, char *out, int length )
{
	Q_strncpyz( out, Rocket_QuakeToRML( in, RP_EMOTICONS ).c_str(), length );
}

// FIXME: Poor naming, this deals with the custom cursors as well
class EngineCursor
{
public:
    void Show(bool show)
    {
        if ( show != show_cursor )
        {
            show_cursor = show;
            if ( focus )
            {
                Update();
            }
        }
    }

    void SetFocus(bool focus)
    {
        this->focus = focus;
        Update();
    }

private:
    void Update()
    {
        if ( menuContext )
        {
			CG_Rocket_EnableCursor( show_cursor && focus );
        }

        MouseMode mode;

        if ( !focus )
        {
            mode = MouseMode::SystemCursor;
        }
        else if ( show_cursor )
        {
            mode = MouseMode::CustomCursor;
        }
        else
        {
            mode = MouseMode::Deltas;
        }

        trap_SetMouseMode( mode );

    }

    bool show_cursor = true;
    bool focus = false;
};

static EngineCursor engineCursor;


void Rocket_SetActiveContext( int catcher )
{
	drawMenu = catcher & KEYCATCH_UI;
	engineCursor.Show( catcher & ( KEYCATCH_UI | KEYCATCH_CONSOLE ) );
}

void CG_FocusEvent( bool has_focus )
{
    engineCursor.SetFocus( has_focus );
}

void Rocket_LoadFont( const char *font )
{
	Rml::Core::GetFontEngineInterface()->LoadFontFace( font, false );
}
