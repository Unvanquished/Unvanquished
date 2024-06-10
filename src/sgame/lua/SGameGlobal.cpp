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

#include "sgame/sg_local.h"
#include "shared/lua/LuaLib.h"
#include "shared/lua/Utils.h"
#include "sgame/lua/register_lua_extensions.h"
#include "sgame/lua/Entity.h"
#include "sgame/lua/EntityProxy.h"

namespace {

using Shared::Lua::LuaLib;
using Shared::Lua::RegType;
using Shared::Lua::CheckVec3;

static Lua::Entity entity;

/// SGame global to access SGame related fields.
// Accessed via the sgame global.
// @module sgame
class SGameGlobal
{
	public:

	/// Get in game entities.
	// @field entity Get in game entities. Analogous to the g_entities global in C++.
	// @usage print(sgame.entiites[0].client.name) -- Print first connected client's name.
	// @see entity
	// @see entityproxy.EntityProxy
	static int GetEntity( lua_State* L )
	{
		LuaLib<Lua::Entity>::push( L, &entity );
		return 1;
	}

	/// Send a server command to a client or clients. Equivalent to trap_SendServerCommand.
	// @function SendServerCommand
	// @tparam integer entity_number client number or -1 for all clients.
	// @tparam string command Command to send to the clients.
	// @usage sgame.SendServerCommand(-1, 'print "WAZZUP!!") -- Print wazzup to all connected clients.
	static int SendServerCommand( lua_State* L )
	{
		if ( !lua_isnumber( L, 1 ) || !lua_isstring( L, 2 ) )
		{
			Log::Warn( "invalid arguments to SendServerCommand." );
			return 0;
		}
		int entNum = luaL_checkinteger( L, 1 );
		const char* cmd = luaL_checkstring( L, 2 );
		trap_SendServerCommand( entNum, cmd );
		return 0;
	}
};

RegType<SGameGlobal> SGameGlobalMethods[] = {
	{ nullptr, nullptr },
};

luaL_Reg SGameGlobalGetters[] = {
	{ "entity", SGameGlobal::GetEntity },

	{ nullptr, nullptr },
};

luaL_Reg SGameGlobalSetters[] = {
	{ nullptr, nullptr },
};

static SGameGlobal sgame;

}  // namespace

namespace Lua {

void InitializeSGameGlobal( lua_State* L )
{
	LuaLib<SGameGlobal>::Register( L );
	LuaLib<Entity>::Register(L);
	LuaLib<EntityProxy>::Register(L);

	LuaLib<SGameGlobal>::push( L, &sgame );
	lua_setglobal( L, "sgame" );
}

}  // namespace Lua

namespace Shared {
namespace Lua {
LUACORETYPEDEFINE( SGameGlobal )

template <>
void ExtraInit<SGameGlobal>( lua_State* L, int metatable_index )
{
	lua_pushcfunction( L, SGameGlobal::SendServerCommand );
	lua_setfield( L, metatable_index - 1, "SendServerCommand" );
}

}  // namespace Lua
}  // namespace Shared
