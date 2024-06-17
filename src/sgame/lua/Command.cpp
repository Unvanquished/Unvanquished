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
#include "sgame/lua/Command.h"

#include "sgame/sg_local.h"
#include "shared/lua/LuaLib.h"
#include "sgame/lua/Interpreter.h"
#include "sgame/lua/Entity.h"

namespace Lua {

struct LuaServerCommand : public Cmd::LambdaCmd
{
	LuaServerCommand( std::string name, std::string description, int ref )
		: Cmd::LambdaCmd(
			  std::move( name ),
			  Cmd::SGAME_VM,
			  std::move( description ), [this]( const Cmd::Args& args ) {
				  lua_State* L = State();

				  // Push the function onto the stack.
				  lua_rawgeti( L, LUA_REGISTRYINDEX, this->ref );

				  // Push arguments to Lua too.
				  lua_createtable( L, args.Argc(), 0 );
				  for ( int i = 1; i < args.Argc(); ++i )
				  {
					  lua_pushstring( L, args.Argv( i ).c_str() );
					  lua_rawseti( L, -2, i );  // lua arrays start at 1
				  }

				  // Call the above function with one argument and zero return values.
				  if ( lua_pcall( L, 1, 0, 0 ) != 0 )
				  {
					  Log::Warn( "Could not run lua server callback: %s: %s", args.Argv( 0 ), lua_tostring( L, -1 ) );
				  }

				  return true;
			  },
			  nullptr ), ref( ref )
	{}

    virtual ~LuaServerCommand()
    {
        luaL_unref( State(), LUA_REGISTRYINDEX, this->ref );
    }

	int ref;
};

// Map of the lower case command name to the lua function reference.
std::unordered_map<std::string, std::unique_ptr<LuaServerCommand>> serverCmdMap;
// Map of the lower case command name to the lua function reference.
std::unordered_map<std::string, int> clientCmdMap;


int RegisterServerCommand( lua_State* L )
{
    const char* cmd = luaL_checkstring( L, 1 );
    if ( !cmd || strlen( cmd ) == 0 )
    {
        Log::Warn( "Lua tried to register an empty command" );
        return 0;
    }

    const char* desc = luaL_checkstring( L, 2 );

    if ( lua_type( L, 3 ) != LUA_TFUNCTION )
    {
        Log::Warn( "Lua must pass in a function as the 3rd arg" );
        return 0;
    }

    // Make sure commands are case insensitive.
    auto lowerCmd = Str::ToLower( cmd );

    // We don't allow overriding server commands.
    auto it = serverCmdMap.find( lowerCmd );
    if ( it != serverCmdMap.end() )
    {
        Log::Warn( "Lua tried to re-register server command `%s`. Ignoring.", lowerCmd );
        return 0;
    }

    int ref = luaL_ref( L, LUA_REGISTRYINDEX );
    serverCmdMap[ cmd ] = std::make_unique<LuaServerCommand>( lowerCmd, desc, ref );
    return 0;
}

int RegisterClientCommand( lua_State* L )
{
    const char* cmd = luaL_checkstring( L, 1 );
    if ( !cmd || strlen( cmd ) == 0 )
    {
        Log::Warn( "Lua tried to register an empty command" );
        return 0;
    }

    if ( lua_type(L, 2) != LUA_TFUNCTION )
    {
        Log::Warn( "Lua must pass in a function as the 2nd arg" );
        return 0;
    }

    // Make sure commands are case insensitive.
    auto lowerCmd = Str::ToLower( cmd );

    // If the command already exists, remove the reference to the old one.
    // This should allow Lua to GC it. On shutdown, the Lua shutdown should
    // cleanup the remaining references.
    auto it = clientCmdMap.find( lowerCmd );
    if ( it != clientCmdMap.end() )
    {
        luaL_unref( State(), LUA_REGISTRYINDEX, it->second );
    }

    int ref = luaL_ref( L, LUA_REGISTRYINDEX );
    clientCmdMap[ cmd ] = ref;
    return 0;
}

bool RunClientCommand( gentity_t* ent )
{
    Cmd::Args args = trap_Args();
    auto lowerCmd = Str::ToLower( args.Argv( 0 ) );
    auto it = clientCmdMap.find( lowerCmd );
    if ( it == clientCmdMap.end() )
    {
        return false;
    }

    lua_State* L = State();

    // Push the function onto the stack.
    lua_rawgeti( L, LUA_REGISTRYINDEX, it->second );

    // If we're console, just push a nil, just like the regular C++ commands.
    if ( ent )
    {
        Shared::Lua::LuaLib<EntityProxy>::push( L, Entity::CreateProxy( ent, L ) );
    }
    else
    {
        lua_pushnil( L );
    }
    // Push arguments to Lua too.
    lua_createtable( L, args.Argc(), 0 );
    for ( int i = 1; i < args.Argc(); ++i )
    {
        lua_pushstring( L, args.Argv( i ).c_str() );
        lua_rawseti( L, -2, i );  // lua arrays start at 1
    }

    // Call the above function with two arguments and zero return values.
    if ( lua_pcall( L, 2, 0, 0 ) != 0 )
    {
        Log::Warn( "Could not run lua client command %s: %s", lowerCmd, lua_tostring( L, -1 ) );
    }

    return true;
}

void CleanupCommands()
{
    serverCmdMap.clear();

    for (const auto& it : clientCmdMap )
    {
        luaL_unref( State(), LUA_REGISTRYINDEX, it.second );
    }
}

}  // namespace Lua
