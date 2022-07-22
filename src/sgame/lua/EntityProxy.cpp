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

#include "sgame/lua/EntityProxy.h"
#include "shared/lua/LuaLib.h"
#include "sgame/sg_local.h"
#include "sgame/lua/Entity.h"

using Unv::Shared::Lua::RegType;
using Unv::Shared::Lua::LuaLib;

namespace Unv {
namespace SGame {
namespace Lua {

#define GETTER(name) { #name, Get##name }
#define SETTER(name) { #name, Set##name }

EntityProxy::EntityProxy(gentity_t* ent, lua_State* L) :
		ent(ent), L(L) {}


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
GET_FUNC2(nextthink, lua_pushnumber(L, proxy->ent->nextthink))

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

static int Setnextthink(lua_State* L)
{
	if (lua_isnumber(L, 2))
	{
		EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
		if (!proxy) return 0;
		int nextthink = luaL_checknumber(L, 1);
		if (nextthink > level.time)
		{
			proxy->ent->nextthink = nextthink;
		}
		return 0;
	}

}

template<typename T>
void Push(lua_State* L, T arg)
{

}

template<>
void Push<gentity_t*>(lua_State* L, gentity_t* ent)
{
	EntityProxy* proxy = Entity::CreateProxy(ent, L);
	LuaLib<EntityProxy>::push(L, proxy, false);
}

template<>
void Push<int>(lua_State* L, int num)
{
	lua_pushnumber(L, num);
}

template <typename T>
void PushArgs(lua_State* L, T arg)
{
	Push(L, arg);
}

template <typename T, typename... Args>
void PushArgs(lua_State* L, T arg, Args... args)
{
	PushArgs(L, arg);
	PushArgs(L, args...);
}

#define ExecFunc(method, upper, def, numArgs, ...) \
	static void Exec##method def \
	{ \
		int entityNum = self - g_entities; \
		EntityProxy* proxy = Entity::proxies[entityNum].get(); \
		if (!proxy) \
		{ \
			Log::Warn("Error " #method "-ing: No proxy for entity num %d", entityNum); \
			return; \
		} \
		auto it = proxy->funcs.find(EntityProxy::upper); \
		if (it->second.method) it->second.method(__VA_ARGS__); \
		lua_rawgeti(proxy->L, LUA_REGISTRYINDEX, it->second.luaRef); \
		PushArgs(proxy->L, __VA_ARGS__); \
		if (lua_pcall(proxy->L, numArgs, 0, 0) != 0) \
		{ \
			Log::Warn( "Could not run lua " #method " callback: %s", \
				lua_tostring(proxy->L, -1)); \
		} \
	} \
	static int Set##method(lua_State* L) \
	{ \
		EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 ); \
		if (!proxy) return 0; \
		int ref; \
		/* if set to nil, clear old lua function */ \
		if (lua_isnil(L, 2)) \
		{ \
			ref = -1; \
		} \
		if (lua_isfunction(L, 2)) \
		{ \
			ref = luaL_ref(L, LUA_REGISTRYINDEX); \
		} \
		else \
		{ \
			Log::Warn("expected function argument for " #method); \
			return 0; \
		} \
		auto it = proxy->funcs.find(EntityProxy::upper); \
		if (it == proxy->funcs.end() && ref != -1) \
		{ \
			it = proxy->funcs.insert({EntityProxy::upper, EntityProxy::EntityFunction { \
				.type = EntityProxy::upper, \
				.luaRef = ref, \
				.method = proxy->ent->method, \
			}}).first; \
			proxy->ent->method = Exec##method; \
		} \
		else \
		{ \
			/* Clean up old ref... */ \
			lua_rawgeti(L, LUA_REGISTRYINDEX, it->second.luaRef); \
			luaL_unref(L, LUA_REGISTRYINDEX, it->second.luaRef); \
			/* If set to nil, remove lua func all together */ \
			if (ref == -1 && it->second.method) \
			{ \
				proxy->ent->method = it->second.method; \
				it->second.method = nullptr; \
			} \
			else \
			{ \
				it->second.luaRef = ref; \
				if (!it->second.method) \
				{ \
					it->second.method = proxy->ent->method; \
					proxy->ent->method = Exec##method; \
				} \
			} \
		} \
		return 0; \
	} \
	static int Get##method(lua_State* L) \
	{ \
		EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 ); \
		if (!proxy) return 0; \
		lua_pushboolean(L, proxy->ent->method == nullptr); \
		return 1; \
	}

ExecFunc(think, THINK, (gentity_t* self), 1, self)
ExecFunc(reset, RESET, (gentity_t* self), 1, self)
ExecFunc(reached, REACHED, (gentity_t* self), 1, self)
ExecFunc(blocked, BLOCKED, (gentity_t* self, gentity_t* other), 2, self, other)
ExecFunc(touch, TOUCH, (gentity_t* self, gentity_t* other, trace_t* trace), 2, self, other, trace)
ExecFunc(use, USE, (gentity_t* self, gentity_t* other, gentity_t* act), 3, self, other, act)
ExecFunc(pain, PAIN, (gentity_t* self, gentity_t* attacker, int damage), 3, self, attacker, damage)
ExecFunc(die, DIE, (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int mod), 4, self, inflictor, attacker, mod)

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
	GETTER(nextthink),
	// Getters for functions just return bool if the function is set.
	GETTER(think),
	GETTER(reset),
	GETTER(reached),
	GETTER(blocked),
	GETTER(touch),
	GETTER(use),
	GETTER(pain),
	GETTER(die),

	{ nullptr, nullptr }
};

luaL_Reg EntityProxySetters[] =
{
	SETTER(origin),
	SETTER(angles),
	SETTER(nextthink),
	// Setters for functions allow running a lua callback in addition to the
	// existing callback.
	SETTER(think),
	SETTER(reset),
	SETTER(reached),
	SETTER(blocked),
	SETTER(touch),
	SETTER(use),
	SETTER(pain),
	SETTER(die),

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
