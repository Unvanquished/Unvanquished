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
#include "sgame/lua/Client.h"
#include "sgame/lua/Bot.h"
#include "sgame/lua/Level.h"
#include "sgame/lua/Buildable.h"
#include "sgame/lua/Hooks.h"
#include "sgame/lua/Command.h"

namespace {

using Shared::Lua::LuaLib;
using Shared::Lua::RegType;
using Shared::Lua::CheckVec3;

static Lua::Entity entity;
static Lua::Level level;
static Lua::Hooks hooks;

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

	/// Get globals related to the game.
	// @field level Get globals related to the game. Analogous to the level global in C++.
	// @see level.Level
	// @usage sgame.level.aliens.total_budget = 500 -- Give aliens more build points.
	static int GetLevel( lua_State* L )
	{
		LuaLib<Lua::Level>::push( L, &level );
		return 1;
	}

	/// Get object used to install hooks in various places.
	// @field hooks Get object used to install hooks in various places.
	// @see hooks
	// @usage sgame.hooks.RegisterChatHook(function(ent, team, message) print(ent.client.name .. ' says: ' .. message) end) -- Print out the message every time.
	static int GetHooks( lua_State* L )
	{
		LuaLib<Lua::Hooks>::push( L, &hooks );
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

	/// Spawn a buildable.
	// You can find the information about the origin, angle, and normal from a layout file.
	// @function SpawnBuildable
	// @tparam string buildable Name of the buildable (eg, trapper, medistat, etc)
	// @tparam array origin Position of the buildable.
	// @tparam array angles Orientation of the buildable.
	// @tparam array normal Normal of the buildable.
	// @treturn buildable.BuildableProxy The buildable.
	// @usage -- trapper -328.875000 -1913.489868 69.430603 0.000000 100.000000 91.000000 1.000000 0.000000 0.000000 30.000000 100.000000 0.000000
	// @usage buildable = sgame.SpawnBuildable('trapper', {328.875000,-1913.489868,-1913.489868}, {0, 100, 91}, {1, 0, 0}) -- You can ignore the last three numbers from the layout line.
	static int SpawnBuildable( lua_State* L )
	{
		vec3_t origin;
		vec3_t normal;
		vec3_t angles;
		if (!CheckVec3( L, 2, origin ) || !CheckVec3( L, 4, normal ) || !CheckVec3( L, 3, angles ) || !lua_isstring( L, 1 ))
		{
			Log::Warn( "invalid arguemnts to SpawnBuildable." );
			return 0;
		}

		const char *buildableName = luaL_checkstring(L, 1);
		const buildableAttributes_t *ba = BG_BuildableByName( buildableName );
		if (ba->number == BA_NONE)
		{
			Log::Warn( "invalid buildable: %s", buildableName );
			return 0;
		}
		gentity_t *builder = G_NewEntity(initEntityStyle_t::NO_CBSE);
		VectorCopy( origin, builder->s.pos.trBase );
		VectorCopy( angles, builder->s.angles );
		VectorCopy( normal, builder->s.origin2 );
		gentity_t* buildable = G_SpawnBuildableImmediately( builder, ba->number );
		if ( !buildable )
		{
			return 0;
		}
		Lua::EntityProxy* proxy = Lua::Entity::CreateProxy( buildable, L );
		LuaLib<Lua::EntityProxy>::push( L, proxy );
		return 1;
	}

	/// Register Client Command
	//
	// This allow registering commands that clients can run that executes Lua code on the server.
	//
	// @function RegisterClientCommand
	// @tparam string Command name
	// @tparam function Function to execute. The function should accept two arguments: an EntityProxy (which can be nil if the caller is console)
	//                  and an array of arguments.
	// @usage sgame.RegisterServerCommand('LuaHello', function(ent, args) print('Hello', args) end)
	static int RegisterClientCommand( lua_State* L )
	{
		return ::Lua::RegisterClientCommand( L );
	}
};

RegType<SGameGlobal> SGameGlobalMethods[] = {
	{ nullptr, nullptr },
};

luaL_Reg SGameGlobalGetters[] = {
	{ "entity", SGameGlobal::GetEntity },
	{ "level", SGameGlobal::GetLevel },
	{ "hooks", SGameGlobal::GetHooks },

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
	LuaLib<Entity>::Register( L );
	LuaLib<EntityProxy>::Register( L );
	LuaLib<Client>::Register( L );
	LuaLib<Bot>::Register( L );
	LuaLib<Level>::Register( L );
	LuaLib<TeamProxy>::Register( L );
	LuaLib<Buildable>::Register( L );
	LuaLib<Hooks>::Register( L );

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
	lua_pushcfunction( L, SGameGlobal::SpawnBuildable );
	lua_setfield( L, metatable_index - 1, "SpawnBuildable" );
	lua_pushcfunction( L, SGameGlobal::RegisterClientCommand );
	lua_setfield( L, metatable_index - 1, "RegisterClientCommand" );
}

}  // namespace Lua
}  // namespace Shared
