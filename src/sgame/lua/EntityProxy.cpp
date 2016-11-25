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

#include "EntityProxy.h"
#include "shared/lua/LuaLib.h"
#include "sgame/sg_local.h"

using Unv::Shared::Lua::RegType;
using Unv::Shared::Lua::LuaLib;

namespace Unv {
namespace SGame {
namespace Lua {

#define GETTER(name) { #name, Get##name }
#define SETTER(name) { #name, Set##name }

EntityProxy::EntityProxy(gentity_t* ent) :
		ent(ent) {}


#define GET_FUNC( var, type ) \
static int Get##var( lua_State* L ) \
{ \
	EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 ); \
	lua_push##type( L, proxy->ent->var ); \
	return 1; \
}

#define GET_FUNC2( var, func) \
static int Get##var( lua_State* L ) \
{ \
	EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 ); \
	func; \
	return 1; \
}

GET_FUNC2(origin, Shared::Lua::PushVec3(L, proxy->ent->s.origin))
GET_FUNC2(class_name, lua_pushstring(L, proxy->ent->classname))
GET_FUNC2(name, lua_pushstring(L, proxy->ent->names[0]))
GET_FUNC2(alias, lua_pushstring(L, proxy->ent->names[2]))
GET_FUNC2(target_name, lua_pushstring(L, proxy->ent->names[1]))
// Intentionally the same index as alias. Check sg_spawn.cpp for why.
GET_FUNC2(target_name2, lua_pushstring(L, proxy->ent->names[2]))
GET_FUNC2(angles, Shared::Lua::PushVec3(L, proxy->ent->s.angles))
static int Getteam(lua_State* L)
{
	EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
	team_t team = TEAM_NONE;
	switch (proxy->ent->s.eType)
	{
		case entityType_t::ET_BUILDABLE:
			team = proxy->ent->buildableTeam;
			break;

		case entityType_t::ET_PLAYER:
			team = static_cast<team_t>(proxy->ent->client->pers.team);
			break;

		default:
			team = proxy->ent->conditions.team;
			break;
	}
	lua_pushstring(L, BG_TeamName(team));
	return 1;
}

static int Setorigin(lua_State* L)
{
	if (lua_istable(L, 2))
	{
		EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
		vec3_t origin;
		Shared::Lua::CheckVec3(L, origin);
		VectorCopy(origin, proxy->ent->s.origin);
	}
	return 0;
}

static int Setangles(lua_State* L)
{
	if (lua_istable(L, 2))
	{
		EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
		vec3_t angles;
		Shared::Lua::CheckVec3(L, angles);
		VectorCopy(angles, proxy->ent->s.angles);
	}
	return 0;
}

RegType<EntityProxy> EntityProxyMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg EntityProxyGetters[] =
{
	GETTER(origin),
	GETTER(class_name),
	GETTER(name),
	GETTER(alias),
	GETTER(target_name),
	GETTER(target_name2),
	GETTER(angles),
	GETTER(team),

	{ nullptr, nullptr }
};

luaL_Reg EntityProxySetters[] =
{
	SETTER(origin),
	SETTER(angles),

	{ nullptr, nullptr }
};

} // namespace Lua
} // namespace SGame
} // namespace Unv

namespace Unv { namespace Shared { namespace Lua {
LUASGAMETYPEDEFINE(EntityProxy, true)
template<>
void ExtraInit<Unv::SGame::Lua::EntityProxy>(lua_State* L, int metatable_index) {}
} } }
