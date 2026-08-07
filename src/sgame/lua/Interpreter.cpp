/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2024 Unvanquished Developers

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

#include "sgame/lua/Interpreter.h"

#include "common/Command.h"
#include "common/FileSystem.h"
#include "shared/bg_lua.h"
#include "shared/bg_public.h"
#include "shared/lua/LuaLib.h"
#include "shared/lua/Utils.h"
#include "sgame/sg_local.h"
#include "sgame/lua/register_lua_extensions.h"

using Shared::Lua::LuaLib;

namespace Lua {
namespace {
lua_State* L = nullptr;

bool LoadCode( Str::StringRef code, Str::StringRef location )
{
	if ( luaL_loadbuffer( L, code.c_str(), code.size(), location.c_str() ) != 0 )
	{
		Shared::Lua::Report( L, "error loading Lua code:" );
		return false;
	}
	return true;
}

bool RunCode()
{
	if ( lua_pcall( L, 0, 0, 0 ) != 0 )
	{
		Shared::Lua::Report( L, "error executing Lua code:" );
		return false;
	}
	return true;
}

bool LoadScript( Str::StringRef scriptPath )
{
	fileHandle_t f;
	int len = BG_FOpenGameOrPakPath( scriptPath, f );
	if ( len < 0 )
	{
		Log::Warn( "error opening %s: %d", scriptPath, len );
		return false;
	}
	std::vector<char> code( len + 1 );
	code[ len ] = '\0';
	int ret = trap_FS_Read( code.data(), len, f );
	trap_FS_FCloseFile( f );
	if ( ret != len )
	{
		Log::Warn( "error reading %s: %d", scriptPath, ret );
		return false;
	}
	return LoadCode( code.data(), scriptPath );
}

// Based off of the LuaPrint in libRocket, which was based off of
// luaB_print function from Lua's lbaselib.c
int LuaPrint( lua_State* L )
{
	int n = lua_gettop( L ); /* number of arguments */
	int i;
	lua_getglobal( L, "tostring" );
	std::vector<std::string> string_list;
	std::string output = "";
	for ( i = 1; i <= n; ++i )
	{
		const char* s;
		lua_pushvalue( L, -1 ); /* function to be called */
		lua_pushvalue( L, i );  /* value to print */
		lua_call( L, 1, 1 );
		s = lua_tostring( L, -1 ); /* get result */
		if ( s == NULL ) return luaL_error( L, "'tostring' must return a string to 'print'" );
		if ( i > 1 ) output += "\t";
		output += s;
		lua_pop( L, 1 ); /* pop result */
	}
	Log::CommandInteractionMessage( output );
	return 0;
}

int LoadUnvModule( lua_State* L )
{
	const char* file = luaL_checkstring( L, 1 );
	if ( !file )
	{
		Log::Warn( "invalid call to LoadUnvModule: must pass in a string." );
		return 0;
	}
	if ( !LoadScript( file ) )
	{
		Log::Warn( "error loading script %s", file );
		return 0;
	}
	return 1;
}

// Remove searchers that search the filesystems so we can install our own searcher.
// Keep the searcher at index 1 because it just looks for cached modules that have
// already been loaded.
const char INITIALIZE_SEARCHERS[] = R"(
table.remove(package.searchers, 2)
table.remove(package.searchers, 3)
)";

void OverrideGlobalLuaFunctions()
{
	lua_getglobal( L, "_G" );

	lua_pushcfunction( L, LuaPrint );
	lua_setfield( L, -2, "print" );

	if ( !ExecCode( INITIALIZE_SEARCHERS, __func__ ) )
	{
		Sys::Drop( "Can't register searcher: failed to run initializer code" );
	}
	// Add the package loader to the package.loaders table.
	lua_getglobal( L, "package" );
	lua_getfield( L, -1, "searchers" );

	if ( lua_isnil( L, -1 ) )
	{
		Sys::Drop( "Can't register searcher: package.loaders table does not exist." );
	}

	lua_pushcfunction( L, LoadUnvModule );
	lua_rawseti( L, -2, 2 );
	lua_pop( L, 2 );
}

}  // namespace

bool ExecCode( Str::StringRef code, Str::StringRef location )
{
	return LoadCode( code, location ) && RunCode();
}

bool ExecScript( Str::StringRef script )
{
	return LoadScript( script ) && RunCode();
}

void Initialize()
{
	if ( L ) return;

	L = luaL_newstate();
	luaL_openlibs( L );
	OverrideGlobalLuaFunctions();
	BG_InitializeLuaConstants( L );
	InitializeSGameGlobal( L );
}

void Shutdown()
{
	lua_close( L );
	L = nullptr;
}

lua_State* State()
{
	return L;
}

class LuaCommand : Cmd::StaticCmd
{
   public:
	LuaCommand() : Cmd::StaticCmd( "lua", Cmd::SGAME_VM, "exec server side lua" ) {}

	void Run( const Cmd::Args& args ) const
	{
		auto usage = [ & ]()
		{ PrintUsage( args, "[-f filename ] | <lua code>", "exec server side lua" ); };
		if ( args.Argc() < 2 )
		{
			usage();
			return;
		}
		const std::string& first = args.Argv( 1 );
		if ( first == "-f" )
		{
			if ( args.Argc() < 3 )
			{
				Log::Warn( "Must pass in filename when using the '-f' flag." );
				usage();
				return;
			}
			// These are arguments that are passed to the script and
			// accessible to the script in the `arg` array.
			lua_newtable( L );
			for ( int i = 3; i < args.Argc(); ++i )
			{
				lua_pushstring( L, args.Argv( i ).c_str() );
				lua_rawseti( L, -2, i - 2 );  // lua arrays start at 1
			}
			lua_setglobal( L, "args" );
			ExecScript( args.Argv( 2 ) );
			return;
		}
		ExecCode( args.ConcatArgs( 1 ), "/lua" );
	}
};

static LuaCommand luaCommand;

}  // namespace Lua
