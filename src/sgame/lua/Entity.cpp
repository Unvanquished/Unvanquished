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

#include "Entity.h"
#include "shared/lua/LuaLib.h"
#include "sgame/sg_local.h"

using Unv::Shared::Lua::LuaLib;
using Unv::Shared::Lua::RegType;

namespace Unv {
namespace SGame {
namespace Lua {

// These are freed by the lua GC.
std::vector<EntityProxy*> Entity::proxies( MAX_GENTITIES );

EntityProxy* Entity::CreateProxy( gentity_t* ent, lua_State* L )
{
	int entNum = ent - g_entities;
	if (!proxies[entNum]) {
		proxies[entNum] = new EntityProxy(ent, L);
	}
	return proxies[entNum];
}

int Entity::Find( lua_State* L )
{
	const char* name = luaL_checkstring(L, 1);
	gentity_t* ent = nullptr;
	while ((ent = G_IterateEntities(ent))) {
		if (G_MatchesName(ent, name)) {
			EntityProxy* proxy = CreateProxy(ent, L);
			LuaLib<EntityProxy>::push(L, proxy, false);
			return 1;
		}
	}

	return 0;
}

int Entity::IterateByClassName( lua_State* L )
{
	const char* name = luaL_checkstring(L, 1);
	gentity_t* ent = nullptr;
	int ret = 0;
	while ((ent = G_IterateEntities(ent))) {
		if ( !Q_stricmp( ent->classname, name ) )
		{
			EntityProxy* proxy = CreateProxy(ent, L);
			LuaLib<EntityProxy>::push(L, proxy, false);
			ret++;
		}
	}
	return ret;
}

int Entity::index( lua_State* L )
{
	int type = lua_type(L, -1);
	if (type != LUA_TNUMBER) return LuaLib<Entity>::index(L);
	int entNum = luaL_checknumber(L, -1);
	if (entNum < 0 || entNum >= MAX_GENTITIES)
	{
		Log::Warn("index out of range: %d", entNum);
		return 0;
	}
	gentity_t* ent = &g_entities[entNum];
	if (!ent->inuse) return 0;
	LuaLib<EntityProxy>::push( L, Entity::CreateProxy(ent, L), false );
	return 1;
}

int Entity::pairs( lua_State* L )
{
	int* pindex = static_cast<int*>( lua_touserdata( L, 3 ) );
	if ( *pindex < 0 ) *pindex = 0;
	while (*pindex  < MAX_GENTITIES)
	{
		gentity_t *ent = &g_entities[*pindex];
		if (!ent->inuse) continue;
		lua_pushinteger(L, *pindex);
		LuaLib<EntityProxy>::push( L, Entity::CreateProxy(ent, L), false );
		return 2;
	}
	lua_pushnil( L );
	lua_pushnil( L );
	return 2;
}

int Entity::New( lua_State* L )
{
	gentity_t* ent = G_NewEntity();
	ent->classname = "lua";
	ent->s.eType = entityType_t::ET_GENERAL;
	trap_LinkEntity(ent);
	LuaLib<EntityProxy>::push(L, Entity::CreateProxy(ent, L), false);
	return 1;
}

int Entity::Delete( lua_State* L )
{
	EntityProxy* proxy = LuaLib<EntityProxy>::check(L, 1);
	if (!proxy) return 0;
	if (Q_strncmp(proxy->ent->classname, "lua", strlen(proxy->ent->classname)) != 0)
	{
		Log::Warn("Lua code only allowed to delete entities created by lua.");
		return 0;
	}
	G_FreeEntity(proxy->ent);
	return 0;
}

RegType<Entity> EntityMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg EntityGetters[] =
{
	{ nullptr, nullptr },
};

luaL_Reg EntitySetters[] =
{
	{ nullptr, nullptr },
};

} // namespace Lua
} // namespace SGame
} // namespace Unv

namespace Unv {
namespace Shared {
namespace Lua {
LUASGAMETYPEDEFINE(Entity, false)
template<>
void ExtraInit<Unv::SGame::Lua::Entity>(lua_State* L, int metatable_index)
{
	lua_pushcfunction( L, Unv::SGame::Lua::Entity::Find);
	lua_setfield( L, metatable_index - 1, "find" );
	lua_pushcfunction( L, Unv::SGame::Lua::Entity::IterateByClassName);
	lua_setfield( L, metatable_index - 1, "iterate_classname" );
	lua_pushcfunction( L, Unv::SGame::Lua::Entity::New );
	lua_setfield( L, metatable_index - 1, "new" );
	lua_pushcfunction( L, Unv::SGame::Lua::Entity::Delete );
	lua_setfield( L, metatable_index - 1, "delete" );


	// overwrite index functions
	lua_pushcfunction( L, Unv::SGame::Lua::Entity::index );
	lua_setfield( L, metatable_index, "__index" );
	lua_pushcfunction( L, Unv::SGame::Lua::Entity::pairs );
	lua_setfield( L, metatable_index, "__pairs" );

}
}  // namespace Lua
}  // namespace Shared
}  // namespace Unv
