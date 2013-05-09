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

// FIXME: Macro conflicts with Rocket::Core::Vector2
#include <Rocket/Core/FileInterface.h>
#include <Rocket/Core/SystemInterface.h>
#include <Rocket/Core/RenderInterface.h>
#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include "rocketDataGrid.h"
#include "rocketDataFormatter.h"
#include "rocketEventInstancer.h"
#include "rocketSelectableDataGrid.h"

extern "C"
{
	#include <SDL.h>
	#include "client.h"
}

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

//TODO: CompileGeometry, RenderCompiledGeometry, ReleaseCompileGeometry ( use vbos and ibos )
class DaemonRenderInterface : public Rocket::Core::RenderInterface
{
public:
	DaemonRenderInterface() {};

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

		re.Add2dPolysIndexed( verts, numVerticies, indices, numIndicies, translation.x, translation.y, ( qhandle_t ) texture );

		Z_Free( verts );
	}

	bool LoadTexture( Rocket::Core::TextureHandle& textureHandle, Rocket::Core::Vector2i& textureDimensions, const Rocket::Core::String& source )
	{
		char temp[ MAX_QPATH ];
		qhandle_t shaderHandle = re.RegisterShader( source.CString(), RSF_NOMIP );

		COM_StripExtension3( source.CString(), temp, sizeof( temp ) );

		// find the size of the texture
		int textureID = re.GetTextureId( temp );

		// GL1 renderer doesn't strip off .ext, so re-check
		if ( textureID == -1 )
		{
			textureID = re.GetTextureId( source.CString() );
		}

		re.GetTextureSize( textureID, &textureDimensions.x, &textureDimensions.y );

		textureHandle = ( Rocket::Core::TextureHandle ) shaderHandle;
		return true;
	}

	bool GenerateTexture( Rocket::Core::TextureHandle& textureHandle, const Rocket::Core::byte* source, const Rocket::Core::Vector2i& sourceDimensions )
	{

		textureHandle = re.GenerateTexture( (const byte* )source, sourceDimensions.x, sourceDimensions.y );
		Com_DPrintf( "RE_GenerateTexture [ %lu ( %d x %d )]\n", textureHandle, sourceDimensions.x, sourceDimensions.y );
		if ( textureHandle > 0 )
		{
			return true;
		}

		return false;
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

int SDLK_keymap[SDLK_WORLD_0];


void InitSDLtoRocketKeymap()
{
	using namespace Rocket::Core::Input;

	for ( int i=0; i<SDL_arraysize(SDLK_keymap); ++i )
		SDLK_keymap[i] = KI_UNKNOWN;

	SDLK_keymap[SDLK_UNKNOWN] = KI_UNKNOWN;
	SDLK_keymap[SDLK_SPACE] = KI_SPACE;
	SDLK_keymap[SDLK_0] = KI_0;
	SDLK_keymap[SDLK_1] = KI_1;
	SDLK_keymap[SDLK_2] = KI_2;
	SDLK_keymap[SDLK_3] = KI_3;
	SDLK_keymap[SDLK_4] = KI_4;
	SDLK_keymap[SDLK_5] = KI_5;
	SDLK_keymap[SDLK_6] = KI_6;
	SDLK_keymap[SDLK_7] = KI_7;
	SDLK_keymap[SDLK_8] = KI_8;
	SDLK_keymap[SDLK_9] = KI_9;
	SDLK_keymap[SDLK_a] = KI_A;
	SDLK_keymap[SDLK_b] = KI_B;
	SDLK_keymap[SDLK_c] = KI_C;
	SDLK_keymap[SDLK_d] = KI_D;
	SDLK_keymap[SDLK_e] = KI_E;
	SDLK_keymap[SDLK_f] = KI_F;
	SDLK_keymap[SDLK_g] = KI_G;
	SDLK_keymap[SDLK_h] = KI_H;
	SDLK_keymap[SDLK_i] = KI_I;
	SDLK_keymap[SDLK_j] = KI_J;
	SDLK_keymap[SDLK_k] = KI_K;
	SDLK_keymap[SDLK_l] = KI_L;
	SDLK_keymap[SDLK_m] = KI_M;
	SDLK_keymap[SDLK_n] = KI_N;
	SDLK_keymap[SDLK_o] = KI_O;
	SDLK_keymap[SDLK_p] = KI_P;
	SDLK_keymap[SDLK_q] = KI_Q;
	SDLK_keymap[SDLK_r] = KI_R;
	SDLK_keymap[SDLK_s] = KI_S;
	SDLK_keymap[SDLK_t] = KI_T;
	SDLK_keymap[SDLK_u] = KI_U;
	SDLK_keymap[SDLK_v] = KI_V;
	SDLK_keymap[SDLK_w] = KI_W;
	SDLK_keymap[SDLK_x] = KI_X;
	SDLK_keymap[SDLK_y] = KI_Y;
	SDLK_keymap[SDLK_z] = KI_Z;
	SDLK_keymap[SDLK_SEMICOLON] = KI_OEM_1;
	SDLK_keymap[SDLK_PLUS] = KI_OEM_PLUS;
	SDLK_keymap[SDLK_COMMA] = KI_OEM_COMMA;
	SDLK_keymap[SDLK_MINUS] = KI_OEM_MINUS;
	SDLK_keymap[SDLK_PERIOD] = KI_OEM_PERIOD;
	SDLK_keymap[SDLK_SLASH] = KI_OEM_2;
	SDLK_keymap[SDLK_BACKQUOTE] = KI_OEM_3;
	SDLK_keymap[SDLK_LEFTBRACKET] = KI_OEM_4;
	SDLK_keymap[SDLK_BACKSLASH] = KI_OEM_5;
	SDLK_keymap[SDLK_RIGHTBRACKET] = KI_OEM_6;
	SDLK_keymap[SDLK_QUOTEDBL] = KI_OEM_7;
	SDLK_keymap[SDLK_KP0] = KI_NUMPAD0;
	SDLK_keymap[SDLK_KP1] = KI_NUMPAD1;
	SDLK_keymap[SDLK_KP2] = KI_NUMPAD2;
	SDLK_keymap[SDLK_KP3] = KI_NUMPAD3;
	SDLK_keymap[SDLK_KP4] = KI_NUMPAD4;
	SDLK_keymap[SDLK_KP5] = KI_NUMPAD5;
	SDLK_keymap[SDLK_KP6] = KI_NUMPAD6;
	SDLK_keymap[SDLK_KP7] = KI_NUMPAD7;
	SDLK_keymap[SDLK_KP8] = KI_NUMPAD8;
	SDLK_keymap[SDLK_KP9] = KI_NUMPAD9;
	SDLK_keymap[SDLK_KP_ENTER] = KI_NUMPADENTER;
	SDLK_keymap[SDLK_KP_MULTIPLY] = KI_MULTIPLY;
	SDLK_keymap[SDLK_KP_PLUS] = KI_ADD;
	SDLK_keymap[SDLK_KP_MINUS] = KI_SUBTRACT;
	SDLK_keymap[SDLK_KP_PERIOD] = KI_DECIMAL;
	SDLK_keymap[SDLK_KP_DIVIDE] = KI_DIVIDE;
	SDLK_keymap[SDLK_KP_EQUALS] = KI_OEM_NEC_EQUAL;
	SDLK_keymap[SDLK_BACKSPACE] = KI_BACK;
	SDLK_keymap[SDLK_TAB] = KI_TAB;
	SDLK_keymap[SDLK_CLEAR] = KI_CLEAR;
	SDLK_keymap[SDLK_RETURN] = KI_RETURN;
	SDLK_keymap[SDLK_PAUSE] = KI_PAUSE;
	SDLK_keymap[SDLK_CAPSLOCK] = KI_CAPITAL;
	SDLK_keymap[SDLK_PAGEUP] = KI_PRIOR;
	SDLK_keymap[SDLK_PAGEDOWN] = KI_NEXT;
	SDLK_keymap[SDLK_END] = KI_END;
	SDLK_keymap[SDLK_HOME] = KI_HOME;
	SDLK_keymap[SDLK_LEFT] = KI_LEFT;
	SDLK_keymap[SDLK_UP] = KI_UP;
	SDLK_keymap[SDLK_RIGHT] = KI_RIGHT;
	SDLK_keymap[SDLK_DOWN] = KI_DOWN;
	SDLK_keymap[SDLK_INSERT] = KI_INSERT;
	SDLK_keymap[SDLK_DELETE] = KI_DELETE;
	SDLK_keymap[SDLK_HELP] = KI_HELP;
	SDLK_keymap[SDLK_LSUPER] = KI_LWIN;
	SDLK_keymap[SDLK_RSUPER] = KI_RWIN;
	SDLK_keymap[SDLK_F1] = KI_F1;
	SDLK_keymap[SDLK_F2] = KI_F2;
	SDLK_keymap[SDLK_F3] = KI_F3;
	SDLK_keymap[SDLK_F4] = KI_F4;
	SDLK_keymap[SDLK_F5] = KI_F5;
	SDLK_keymap[SDLK_F6] = KI_F6;
	SDLK_keymap[SDLK_F7] = KI_F7;
	SDLK_keymap[SDLK_F8] = KI_F8;
	SDLK_keymap[SDLK_F9] = KI_F9;
	SDLK_keymap[SDLK_F10] = KI_F10;
	SDLK_keymap[SDLK_F11] = KI_F11;
	SDLK_keymap[SDLK_F12] = KI_F12;
	SDLK_keymap[SDLK_F13] = KI_F13;
	SDLK_keymap[SDLK_F14] = KI_F14;
	SDLK_keymap[SDLK_F15] = KI_F15;
	SDLK_keymap[SDLK_NUMLOCK] = KI_NUMLOCK;
	SDLK_keymap[SDLK_SCROLLOCK] = KI_SCROLL;
	SDLK_keymap[SDLK_LSHIFT] = KI_LSHIFT;
	SDLK_keymap[SDLK_RSHIFT] = KI_RSHIFT;
	SDLK_keymap[SDLK_LCTRL] = KI_LCONTROL;
	SDLK_keymap[SDLK_RCTRL] = KI_RCONTROL;
	SDLK_keymap[SDLK_LALT] = KI_LMENU;
	SDLK_keymap[SDLK_RALT] = KI_RMENU;
	SDLK_keymap[SDLK_LMETA] = KI_LMETA;
	SDLK_keymap[SDLK_RMETA] = KI_RMETA;
}


Rocket::Core::Input::KeyModifier RocketConvertSDLmod( SDLMod sdl )
{
	using namespace Rocket::Core::Input;

	int mod = 0;
	if( sdl & KMOD_SHIFT )	mod |= KM_SHIFT;
	if( sdl & KMOD_CTRL )	mod |= KM_CTRL;
	if( sdl & KMOD_ALT )	mod |= KM_ALT;
	if( sdl & KMOD_META )	mod |= KM_META;
	if( sdl & KMOD_CAPS )	mod |= KM_CAPSLOCK;
	if( sdl & KMOD_NUM )	mod |= KM_NUMLOCK;

	return KeyModifier( mod );
}


int RocketConvertSDLButton( Uint8 sdlButton )
{
	return sdlButton - 1;
}

void Rocket_Rocket_f( void )
{
	Rocket_DocumentAction( Cmd_Argv(1), Cmd_Argv(2) );
}

static DaemonFileInterface fileInterface;
static DaemonSystemInterface systemInterface;
static DaemonRenderInterface renderInterface;
Rocket::Core::Context *context = NULL;

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

	InitSDLtoRocketKeymap();

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

	// Create the main context
	context = Rocket::Core::CreateContext( "default", Rocket::Core::Vector2i( cls.glconfig.vidWidth, cls.glconfig.vidHeight ) );

	// Add the event listner instancer
	EventInstancer* event_instancer = new EventInstancer();
	Rocket::Core::Factory::RegisterEventListenerInstancer( event_instancer );
	event_instancer->RemoveReference();

	// Add custom client elements
	Rocket::Core::Factory::RegisterElementInstancer( "datagrid", new Rocket::Core::ElementInstancerGeneric< SelectableDataGrid >() )->RemoveReference();

	Cmd_AddCommand( "rocket", Rocket_Rocket_f );

	//Rocket::Debugger::Initialise(context);
}

void Rocket_Shutdown( void )
{
	extern std::vector<RocketDataFormatter*> dataFormatterList;
	extern std::map<std::string, RocketDataGrid*> dataSourceMap;
	extern std::queue< RocketEvent_t* > eventQueue;

	if ( context )
	{
		context->RemoveReference();
		context = NULL;
	}

	Rocket::Core::Shutdown();

	// Prevent memory leaks

	for ( int i = 0; i < dataFormatterList.size(); ++i )
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
}

void Rocket_Render( void )
{
	if ( context )
	{
		context->Render();
	}
}

void Rocket_Update( void )
{
	if ( context )
	{
		context->Update();
	}
}


 extern "C" void InjectRocket( SDL_Event event )
{
	using Rocket::Core::Input::KeyIdentifier;

	if ( !context || cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		return;
	}

	switch( event.type )
	{
		case SDL_KEYDOWN:
		{
			SDLKey sdlkey = event.key.keysym.sym;
			wchar_t c;
			c = static_cast<wchar_t>(event.key.keysym.unicode);

			int key = SDLK_keymap[sdlkey]; // The SDLK_keymap array maps SDLK_* to Rocket::Input::KI_*. Defined in sdltorocket.cpp
			context->ProcessKeyDown( KeyIdentifier( key ), RocketConvertSDLmod( event.key.keysym.mod ) );

			if( event.key.keysym.unicode != 0 && event.key.keysym.unicode != 8 )
				context->ProcessTextInput( c );
		}
		break;
		case SDL_KEYUP:
			context->ProcessKeyUp( KeyIdentifier( event.key.keysym.scancode ), RocketConvertSDLmod( event.key.keysym.mod ) );
			break;
		case SDL_MOUSEMOTION:
			context->ProcessMouseMove( event.motion.x, event.motion.y, RocketConvertSDLmod( SDL_GetModState() ) );
			break;
		case SDL_MOUSEBUTTONDOWN:
			switch ( event.button.button )
			{
				case 4:
					context->ProcessMouseWheel( -1, RocketConvertSDLmod( SDL_GetModState()) );
					break;

				case 5:
					context->ProcessMouseWheel( 1, RocketConvertSDLmod( SDL_GetModState() ) );
					break;
				default:
					context->ProcessMouseButtonDown( RocketConvertSDLButton(event.button.button), RocketConvertSDLmod( SDL_GetModState() ) );
			}
			break;
		case SDL_MOUSEBUTTONUP:
			context->ProcessMouseButtonUp( RocketConvertSDLButton(event.button.button), RocketConvertSDLmod( SDL_GetModState() ) );
			break;
	}
}

extern "C" int IN_SDLKeyToQuakeKey( int sym );
int Rocket_ToQuakeKey( const int rocketKey )
{
	for ( int i = 0; i < SDL_arraysize( SDLK_keymap ); ++i )
	{
		if ( SDLK_keymap[ i ] == rocketKey )
		{
			return IN_SDLKeyToQuakeKey( i );
		}
	}

	return 0;
}


void Rocket_InjectMouseMotion( int x, int y )
{
	if ( context && !( cls.keyCatchers & KEYCATCH_CONSOLE ) )
	{
		context->ProcessMouseMove( x, y, RocketConvertSDLmod( SDL_GetModState() ) );
	}
}

Rocket::Core::String Rocket_QuakeToRML( const char *in )
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
		else if ( Q_IsColorString( p ) )
		{
			if ( span )
			{
				out.Append( "</span>" );
				span = qfalse;
			}

			char rgb[32];
			int code = ( *++p - 48 ) & 31;

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

	return out;
}
