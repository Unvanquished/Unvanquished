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

class DaemonFileInterface : public Rocket::Core::FileInterface
{
public:
	Rocket::Core::FileHandle Open( const Rocket::Core::String &filePath )
	{
		fileHandle_t fileHandle;
		trap_FS_OpenPakFile( filePath.CString(), fileHandle );
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
	double GetElapsedTime()
	{
		return trap_Milliseconds() / 1000.0f;
	}

	// TODO: Add explicit support for other log types
	bool LogMessage( Rocket::Core::Log::Type type, const Rocket::Core::String &message )
	{
		switch ( type )
		{
			case Rocket::Core::Log::LT_ALWAYS :
			case Rocket::Core::Log::LT_ERROR :
			case Rocket::Core::Log::LT_WARNING :
				Log::Warn( message.CString() );
				break;
			default:
			case Rocket::Core::Log::LT_INFO :
				Log::Notice( message.CString() );
				break;
		}
		return true;
	}
};

static polyVert_t *createVertexArray( Rocket::Core::Vertex *vertices, int count )
{
	polyVert_t *verts = new polyVert_t[ count ];

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
class DaemonRenderInterface : public Rocket::Core::RenderInterface
{
public:
	DaemonRenderInterface() { }

	void RenderGeometry( Rocket::Core::Vertex *verticies,  int numVerticies, int *indices, int numIndicies, Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation ) override
	{
		polyVert_t *verts = createVertexArray( verticies, numVerticies );
		trap_R_Add2dPolysIndexedToScene( verts, numVerticies, indices, numIndicies, translation.x, translation.y, texture ? ( qhandle_t ) texture : whiteShader );
		delete[]  verts;
	}

	Rocket::Core::CompiledGeometryHandle CompileGeometry( Rocket::Core::Vertex *vertices, int num_vertices, int *indices, int num_indices, Rocket::Core::TextureHandle texture ) override
	{
		RocketCompiledGeometry *geometry = new RocketCompiledGeometry( vertices, num_vertices, indices, num_indices, texture ? ( qhandle_t ) texture : whiteShader );

		return Rocket::Core::CompiledGeometryHandle( geometry );

	}

	void RenderCompiledGeometry( Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f &translation ) override
	{
		RocketCompiledGeometry *g = ( RocketCompiledGeometry * ) geometry;
		trap_R_Add2dPolysIndexedToScene( g->verts, g->numVerts, g->indices, g->numIndicies, translation.x, translation.y, g->shader );
	}

	void ReleaseCompiledGeometry( Rocket::Core::CompiledGeometryHandle geometry ) override
	{
		RocketCompiledGeometry *g = ( RocketCompiledGeometry * ) geometry;
		delete g;
	}

	bool LoadTexture( Rocket::Core::TextureHandle& textureHandle, Rocket::Core::Vector2i& textureDimensions, const Rocket::Core::String& source ) override
	{
		qhandle_t shaderHandle = trap_R_RegisterShader( source.CString(), RSF_NOMIP );

		if ( shaderHandle <= 0 )
		{
			return false;
		}

		// Find the size of the texture
		textureHandle = ( Rocket::Core::TextureHandle ) shaderHandle;
		trap_R_GetTextureSize( shaderHandle, &textureDimensions.x, &textureDimensions.y );
		return true;
	}
	bool GenerateTexture( Rocket::Core::TextureHandle& texture_handle, const byte* source, const Rocket::Core::Vector2i& source_dimensions ) override
	{

		texture_handle = trap_R_GenerateTexture( (const byte* )source, source_dimensions.x, source_dimensions.y );
// 		Log::Debug( "RE_GenerateTexture [ %lu ( %d x %d )]", textureHandle, sourceDimensions.x, sourceDimensions.y );

		return ( texture_handle > 0 );
	}

	void ReleaseTexture( Rocket::Core::TextureHandle ) override
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

	void SetTransform( const Rocket::Core::Matrix4f* transform ) override
	{
		// TODO: implement.
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
		CG_SetKeyCatcher( rocketInfo.keyCatcher | KEYCATCH_UI );
	}
	else
	{
		CG_SetKeyCatcher( rocketInfo.keyCatcher & ~KEYCATCH_UI );
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

std::unique_ptr<Rocket::Core::ElementInstancerGeneric<SelectableDataGrid>> selectableDataGridInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<RocketProgressBar>> rocketProgressBarInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<RocketDataSelect>> rocketDataSelectInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<RocketConsoleTextElement>> rocketConsoleTextElementInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<RocketDataSourceSingle>> rocketDataSourceSingleInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<RocketDataSource>> rocketDataSourceInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<RocketKeyBinder>> rocketKeyBinderInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<RocketChatField>> rocketChatFieldInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<CvarElementFormControlInput>> cvarElementFormControlInputInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<CvarElementFormControlSelect>> cvarElementFormControlSelectInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<RocketConditionalElement>> rocketConditionalElementInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<RocketColorInput>> rocketColorInputInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<RocketIncludeElement>> rocketIncludeElementInstancer;
std::unique_ptr<Rocket::Core::ElementInstancerGeneric<RocketCvarInlineElement>> rocketCvarInlineElementInstancer;

Rocket::Core::PropertyId UnvPropertyId::Orientation;

// TODO
// cvar_t *cg_draw2D;

void Rocket_Init()
{
	Rocket::Core::SetFileInterface( &fileInterface );
	Rocket::Core::SetSystemInterface( &systemInterface );
	Rocket::Core::SetRenderInterface( &renderInterface );

	if ( !Rocket::Core::Initialise() )
	{
		Log::Notice( "Could not init libRocket\n" );
		return;
	}

	// Initialize the controls plugin
	Rocket::Controls::Initialise();

	// Initialize Lua
	Rocket::Core::Lua::Interpreter::Initialise();
	Rocket::Core::Lua::Interpreter::DoString("math.randomseed(os.time())");
	Rocket::Controls::Lua::RegisterTypes(Rocket::Core::Lua::Interpreter::GetLuaState());
	Rocket::Core::Lua::LuaType<Rocket::Core::Lua::Cvar>::Register(Rocket::Core::Lua::Interpreter::GetLuaState());
	Rocket::Core::Lua::LuaType<Rocket::Core::Lua::Cmd>::Register(Rocket::Core::Lua::Interpreter::GetLuaState());
	Rocket::Core::Lua::LuaType<Rocket::Core::Lua::Events>::Register(Rocket::Core::Lua::Interpreter::GetLuaState());
	Rocket::Core::Lua::LuaType<Rocket::Core::Lua::Timer>::Register(Rocket::Core::Lua::Interpreter::GetLuaState());
	CG_Rocket_RegisterLuaCDataSource(Rocket::Core::Lua::Interpreter::GetLuaState());

	// Register custom properties.
	UnvPropertyId::Orientation = Rocket::Core::StyleSheetSpecification::RegisterProperty("orientation", "left", false, true)
		.AddParser("keyword", "left, right, up, down")
		.GetId();

	// Set backup font
	// TODO
	// Rocket::Core::FontDatabase::SetBackupFace( "fonts/unifont.ttf" );

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
	#define REGISTER_ELEMENT(tag, type, instancer) \
		instancer.reset(new Rocket::Core::ElementInstancerGeneric< type >()); \
		Rocket::Core::Factory::RegisterElementInstancer(tag, instancer.get());

	REGISTER_ELEMENT("datagrid", SelectableDataGrid, selectableDataGridInstancer);
	REGISTER_ELEMENT("progressbar", RocketProgressBar, rocketProgressBarInstancer);
	REGISTER_ELEMENT("dataselect", RocketDataSelect, rocketDataSelectInstancer);
	REGISTER_ELEMENT("console_text", RocketConsoleTextElement, rocketConsoleTextElementInstancer);
	REGISTER_ELEMENT("datasource_single", RocketDataSourceSingle, rocketDataSourceSingleInstancer);
	REGISTER_ELEMENT("datasource", RocketDataSource, rocketDataSourceInstancer);
	REGISTER_ELEMENT("keybind", RocketKeyBinder, rocketKeyBinderInstancer);
	REGISTER_ELEMENT("chatfield", RocketChatField, rocketChatFieldInstancer);
	REGISTER_ELEMENT("input", CvarElementFormControlInput, cvarElementFormControlInputInstancer);
	REGISTER_ELEMENT("select", CvarElementFormControlSelect, cvarElementFormControlSelectInstancer);
	REGISTER_ELEMENT("if", RocketConditionalElement, rocketConditionalElementInstancer);
	REGISTER_ELEMENT("colorinput", RocketColorInput, rocketColorInputInstancer);
	REGISTER_ELEMENT("include", RocketIncludeElement, rocketIncludeElementInstancer);
	REGISTER_ELEMENT("inlinecvar", RocketCvarInlineElement, rocketCvarInlineElementInstancer);

	whiteShader = trap_R_RegisterShader( "gfx/colors/white", RSF_DEFAULT );
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
		if (! Rocket::Core::RemoveContext( "menuContext" ) )
		{
			Log::Warn( "Could not remove menuContext" );
		}
		menuContext = nullptr;
	}

	if ( hudContext )
	{
		if (! Rocket::Core::RemoveContext( "hudContext" ) )
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

	Rocket::Core::Shutdown();

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
	if ( cg_draw2D.Get() && hudContext )
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

	if ( cg_draw2D.Get() && hudContext )
	{
		hudContext->Update();
	}
	Rocket::Core::Lua::Timer::Update(rocketInfo.realtime);
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

// TODO: Make this take Rocket::Core::String as an input.
Rocket::Core::String Rocket_QuakeToRML( const char *in, int parseFlags = 0 )
{
	Rocket::Core::String out;
	Rocket::Core::String spanstr;
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
					out.Append( spanstr );
				}
				out.Append( "&lt;" );
			}
			else if ( c == '>' )
			{
				if ( span && !spanHasContent )
				{
					spanHasContent = true;
					out.Append( spanstr );
				}
				out.Append( "&gt;" );
			}
			else if ( c == '&' )
			{
				if ( span && !spanHasContent )
				{
					spanHasContent = true;
					out.Append( spanstr );
				}
				out.Append( "&amp;" );
			}
			else if ( c == '\n' )
			{
				out.Append( span && spanHasContent ? "</span><br />" : "<br />" );
				span = false;
				spanHasContent = false;
			}
			else if ( ( parseFlags & RP_EMOTICONS ) && ( emoticon = BG_EmoticonAt( token.Begin() ) ) )
			{
				if ( span && !spanHasContent )
				{
					spanHasContent = true;
					out.Append( spanstr );
				}
				out.Append( va( "<img class='emoticon' src='/%s' />", emoticon->imageFile.c_str() ) );
				while ( *iter->Begin() != ']' )
				{
					++iter;
				}
			}
			else
			{
				if ( span && !spanHasContent )
				{
					out.Append( spanstr );
					spanHasContent = true;
				}
				out.Append( token.Begin(), token.Size() );
			}
		}
		else if ( token.Type() == Color::Token::TokenType::COLOR )
		{
			if ( span && spanHasContent )
			{
				out.Append( "</span>" );
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
				out.Append( spanstr );
				spanHasContent = true;
			}
			out.Append( 1, Color::Constants::ESCAPE );
		}
	}

	if ( span && spanHasContent )
	{
		out.Append( "</span>" );
	}

	return out;
}

void Rocket_QuakeToRMLBuffer( const char *in, char *out, int length )
{
	Q_strncpyz( out, Rocket_QuakeToRML( in, RP_EMOTICONS ).CString(), length );
}

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
            menuContext->EnableMouseCursor( show_cursor && focus );
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
	switch ( catcher )
	{
		case KEYCATCH_UI:
			engineCursor.Show( true );
			drawMenu = true;
			break;

		default:
			if ( !( catcher & KEYCATCH_CONSOLE ) )
			{
				engineCursor.Show( false );
			}

			drawMenu = false;
			break;
	}
}

void CG_FocusEvent( bool has_focus )
{
    engineCursor.SetFocus( has_focus );
}

void Rocket_LoadFont( const char *font )
{
	Rocket::Core::GetFontEngineInterface()->LoadFontFace( font, false );
}
