/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unv Developers

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

#include "sgame/lua/Level.h"
#include "shared/lua/LuaLib.h"
#include "sgame/sg_local.h"

using Unv::Shared::Lua::LuaLib;
using Unv::Shared::Lua::RegType;

namespace Unv {
namespace SGame {
namespace Lua {

#define GET_FUNC( var, func) \
static int Get##var( lua_State* L ) \
{ \
	func; \
	return 1; \
}

#define GET_TEAM_FUNC( var, func ) \
static int GetTeam##var( lua_State* L ) \
{ \
    TeamProxy* t = LuaLib<TeamProxy>::check( L, 1 ); \
    const auto& team = level.team[ t->team ]; \
    func; \
    return 1; \
}

GET_TEAM_FUNC( num_spawns, lua_pushinteger(L, team.numSpawns ) )
GET_TEAM_FUNC( num_clients, lua_pushinteger(L, team.numClients ) )
GET_TEAM_FUNC( num_players, lua_pushinteger(L, team.numPlayers ) )
GET_TEAM_FUNC( num_bots, lua_pushinteger(L, team.numBots ) )
GET_TEAM_FUNC( total_budget, lua_pushnumber(L, team.totalBudget ) )
GET_TEAM_FUNC( spent_budget, lua_pushinteger(L, team.spentBudget ) )
GET_TEAM_FUNC( queued_budget, lua_pushinteger(L, team.queuedBudget ) )
GET_TEAM_FUNC( kills, lua_pushinteger(L, team.kills ) )
GET_TEAM_FUNC( locked, lua_pushboolean(L, team.locked ) )
GET_TEAM_FUNC( momentum, lua_pushnumber(L, team.momentum ) )

static int SetTeamlocked( lua_State* L )
{
    TeamProxy* t = LuaLib<TeamProxy>::check( L, 1 );
    auto& team = level.team[ t->team ];
    team.locked = lua_toboolean( L, 2 );
    return 0;
}

static int SetTeammomentum( lua_State* L )
{
    TeamProxy* t = LuaLib<TeamProxy>::check( L, 1 );
    G_AddMomentumGeneric( t->team, luaL_checknumber( L, 2 ) );
    return 0;
}

static int SetTeamtotal_budget( lua_State* L )
{
    TeamProxy* t = LuaLib<TeamProxy>::check( L, 1 );
    level.team[ t->team ].totalBudget = luaL_checknumber( L, 2 );
    return 0;
}

static int SetTeamqueued_budget( lua_State* L )
{
    TeamProxy* t = LuaLib<TeamProxy>::check( L, 1 );
    level.team[ t->team ].queuedBudget = luaL_checkinteger( L, 2 );
    return 0;
}

RegType<TeamProxy> TeamProxyMethods[] =
{
	{ nullptr, nullptr },
};
#define TEAM_GETTER(name) { #name, GetTeam##name }
luaL_Reg TeamProxyGetters[] =
{
    TEAM_GETTER( num_spawns ),
    TEAM_GETTER( num_clients ),
    TEAM_GETTER( num_players ),
    TEAM_GETTER( num_bots ),
    TEAM_GETTER( total_budget ),
    TEAM_GETTER( spent_budget ),
    TEAM_GETTER( queued_budget ),
    TEAM_GETTER( kills ),
    TEAM_GETTER( locked ),
    TEAM_GETTER( momentum ),
    { nullptr, nullptr },
};
#define TEAM_SETTER(name) { #name, SetTeam##name }
luaL_Reg TeamProxySetters[] =
{
    TEAM_SETTER( locked ),
    TEAM_SETTER( momentum ),
    TEAM_SETTER( total_budget ),
    TEAM_SETTER( queued_budget ),
    { nullptr, nullptr },
};

static TeamProxy alienTeamProxy { TEAM_ALIENS };
static TeamProxy humanTeamProxy { TEAM_HUMANS };

GET_FUNC( num_entities, lua_pushinteger(L, level.num_entities) )
GET_FUNC( timeLimit, lua_pushinteger(L, level.timelimit) )
GET_FUNC( max_clients, lua_pushinteger(L, level.maxclients) )
GET_FUNC( time, lua_pushinteger(L, level.time) )
GET_FUNC( previous_time, lua_pushinteger(L, level.previousTime) )
GET_FUNC( start_time, lua_pushinteger(L, level.startTime) )
GET_FUNC( match_time, lua_pushinteger(L, level.matchTime) )
GET_FUNC( num_connected_clients, lua_pushinteger(L, level.numConnectedClients) )
GET_FUNC( num_connected_players, lua_pushinteger(L, level.numConnectedPlayers) )
GET_FUNC( aliens, LuaLib<TeamProxy>::push(L, &alienTeamProxy, false) )
GET_FUNC( humans, LuaLib<TeamProxy>::push(L, &humanTeamProxy, false) )

RegType<Level> LevelMethods[] =
{
	{ nullptr, nullptr },
};
#define GETTER(name) { #name, Get##name }
luaL_Reg LevelGetters[] =
{
    GETTER( num_entities ),
    GETTER( timeLimit ),
    GETTER( max_clients ),
    GETTER( time ),
    GETTER( previous_time ),
    GETTER( start_time ),
    GETTER( match_time ),
    GETTER( num_connected_clients ),
    GETTER( num_connected_players ),
    GETTER( aliens ),
    GETTER( humans ),
	{ nullptr, nullptr },
};

luaL_Reg LevelSetters[] =
{
	{ nullptr, nullptr },
};

} // namespace Lua
} // namespace SGame
} // namespace Unv

namespace Unv {
namespace Shared {
namespace Lua {
LUASGAMETYPEDEFINE(Level, false)
LUASGAMETYPEDEFINE(TeamProxy, false)
template<>
void ExtraInit<Unv::SGame::Lua::Level>(lua_State* /*L*/, int /*metatable_index*/) {}
template<>
void ExtraInit<Unv::SGame::Lua::TeamProxy>(lua_State* /*L*/, int /*metatable_index*/) {}
}  // namespace Lua
}  // namespace Shared
}  // namespace Unv
