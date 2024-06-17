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
#include "sgame/lua/Votes.h"

#include "sgame/sg_local.h"

#include "sgame/lua/Entity.h"
#include "sgame/lua/Interpreter.h"
#include "sgame/sg_votes.h"
#include "shared/lua/LuaLib.h"

namespace Lua {

using Shared::Lua::LuaLib;

int RegisterVote( lua_State* L )
{
	const char* vote = luaL_checkstring( L, 1 );
	if ( !vote || strlen( vote ) == 0 )
	{
		Log::Warn( "Lua tried to register an empty vote" );
		return 0;
	}

	// Make sure votes are case insensitive.
	auto lowerVote = Str::ToLower( vote );

	auto vd = VoteDefinition{};

	lua_getfield( L, 2, "type" );
	const char* s = luaL_checkstring( L, -1 );
	lua_pop( L, 1 );
	if ( !G_ParseVoteType( s, &vd.type ) )
	{
		lua_pushboolean( L, false );
		return 1;
	}

	lua_getfield( L, 2, "target" );
	s = luaL_checkstring( L, -1 );
	lua_pop( L, 1 );
	if ( !G_ParseVoteTarget( s, &vd.target ) )
	{
		lua_pushboolean( L, false );
		return 1;
	}

	lua_getfield( L, 2, "stopOnIntermission" );
	vd.stopOnIntermission = lua_toboolean( L, -1 );
	lua_pop( L, 1 );

	lua_getfield( L, 2, "adminImmune" );
	vd.adminImmune = lua_toboolean( L, -1 );
	lua_pop( L, 1 );

	lua_getfield( L, 2, "quorum" );
	vd.quorum = lua_toboolean( L, -1 );
	lua_pop( L, 1 );

	lua_getfield( L, 2, "reasonNeeded" );
	vd.reasonNeeded = lua_toboolean( L, -1 ) ? qtrinary::qyes : qtrinary::qno;
	lua_pop( L, 1 );

	vd.percentage = &g_customVotesPercent;

	int ref = luaL_ref( L, LUA_REGISTRYINDEX );

	vd.handler = [ref](
        gentity_t* ent, team_t team, const std::string& cmd, const std::string&, std::string&, const std::string&, int, int ) {
        auto L = State();
        // Push Vote closure onto the stack.
        lua_rawgeti( L, LUA_REGISTRYINDEX, ref );
        // Setup function arguments.
        // Calling entity.
        LuaLib<EntityProxy>::push( L, Entity::CreateProxy( ent, L ) );
        //
        lua_pushstring( L, BG_TeamName( team ) );
        const Cmd::Args& args = trap_Args();
        lua_createtable( L, args.Argc(), 0 );
        for ( int i = 1; i < args.Argc(); ++i )
        {
            lua_pushstring( L, args.Argv( i ).c_str() );
            lua_rawseti( L, -2, i );  // lua arrays start at 1
        }

        // Call the above function with two arguments and zero return values.
        if ( lua_pcall( L, 3, 3, 0 ) != 0 )
        {
            Log::Warn( "Could not run lua vote %s: %s", cmd, lua_tostring( L, -1 ) );
            return false;
        }

        bool ret = lua_toboolean( L, -3 );
        if ( ret )
        {
            const char* voteExec = luaL_checkstring( L, -2 );
            if ( !voteExec || strlen( voteExec ) == 0 )
            {
                ret = false;
            }
            const char* voteDisplay = luaL_checkstring( L, -1 );
            if ( !voteDisplay || strlen( voteDisplay ) == 0 )
            {
                ret = false;
            }

			if ( ret )
			{
				Q_strncpyz( level.team[ team ].voteString,
				            voteExec,
				            sizeof( level.team[ team ].voteString ) );

				Q_strncpyz( level.team[ team ].voteDisplayString,
				            voteDisplay,
				            sizeof( level.team[ team ].voteDisplayString ) );
			}
		}

        lua_pop( L, 3 );
        return ret;
    };

    bool ret = G_AddCustomVoteRaw( vote, std::move( vd ) );
    if ( !ret )
    {
        luaL_unref( L, LUA_REGISTRYINDEX, ref );
    }
    lua_pushboolean( L, ret );

	return 1;
}

}  // namespace Lua
