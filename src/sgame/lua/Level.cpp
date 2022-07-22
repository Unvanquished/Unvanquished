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

GET_FUNC( num_entities, lua_pushnumber(L, level.num_entities) )
GET_FUNC( timeLimit, lua_pushnumber(L, level.timelimit) )
GET_FUNC( max_clients, lua_pushnumber(L, level.maxclients) )
GET_FUNC( time, lua_pushnumber(L, level.time) )
GET_FUNC( previous_time, lua_pushnumber(L, level.previousTime) )
GET_FUNC( start_time, lua_pushnumber(L, level.startTime) )
GET_FUNC( match_time, lua_pushnumber(L, level.matchTime) )
GET_FUNC( num_connected_clients, lua_pushnumber(L, level.numConnectedClients) )
GET_FUNC( num_connected_players, lua_pushnumber(L, level.numConnectedPlayers) )


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
template<>
void ExtraInit<Unv::SGame::Lua::Level>(lua_State* L, int metatable_index)
{
}
}  // namespace Lua
}  // namespace Shared
}  // namespace Unv
