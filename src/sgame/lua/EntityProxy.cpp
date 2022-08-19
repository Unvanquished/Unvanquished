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

/// Handle interactions with Entities.
// @module entityproxy

namespace Unv {
namespace SGame {
namespace Lua {

#define GETTER(name) { #name, Get##name }
#define SETTER(name) { #name, Set##name }

/// Access information and interact with in game entities. Wrapper class for gentity_t.
// @table EntityProxy
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

/// Entity origin. Array of floats starting at index 1.
// @tfield array origin Read/Write.
// @within EntityProxy
GET_FUNC2(origin, Shared::Lua::PushVec3(L, proxy->ent->s.origin))
/// Entity classname.
// @tfield string class_name Read only.
// @within EntityProxy
GET_FUNC2(class_name, lua_pushstring(L, proxy->ent->classname))
/// Entity name. Probably unused and only useful in a narrow set of usecases. Probably not what you want.
// @tfield string name Read only.
// @within EntityProxy
GET_FUNC2(name, lua_pushstring(L, proxy->ent->names[0]))
/// Entity alias. Probably unused and only useful in a narrow set of usecases. Probably not what you want.
// @tfield string alias Read only.
// @within EntityProxy
GET_FUNC2(alias, lua_pushstring(L, proxy->ent->names[2]))
/// Target name. Probably unused and only useful in a narrow set of usecases. Probably not what you want.
// @tfield string target_name Read only.
// @within EntityProxy
GET_FUNC2(target_name, lua_pushstring(L, proxy->ent->names[1]))
/// Target name2. Probably unused and only useful in a narrow set of usecases. Probably not what you want.
// @tfield string target_name2 Read only.
// Intentionally the same index as alias. Check sg_spawn.cpp for why.
// @within EntityProxy
GET_FUNC2(target_name2, lua_pushstring(L, proxy->ent->names[2]))
/// Entity angles. Controls orientation of the entity. Array of floats starting at index 1.
// @tfield array angles Read/Write.
// @within EntityProxy
GET_FUNC2(angles, Shared::Lua::PushVec3(L, proxy->ent->s.angles))
/// The next level.time the entity will think.
// @tfield integer nextthink Read/Write.
// @see level.time
// @within EntityProxy
GET_FUNC2(nextthink, lua_pushinteger(L, proxy->ent->nextthink))
/// The mins for the entity AABB. Array of floats starting at index 1.
// @tfield array mins Read/Write.
// @within EntityProxy
GET_FUNC2(mins, Shared::Lua::PushVec3(L, proxy->ent->r.mins))
/// The maxs for the entity AABB. Array of floats starting at index 1.
// @tfield array maxs Read/Write.
// @within EntityProxy
GET_FUNC2(maxs, Shared::Lua::PushVec3(L, proxy->ent->r.maxs))
/// The entity number. Also the entity's index in g_entities.
// @tfield integer number Read only.
// @within EntityProxy
GET_FUNC2(number, lua_pushinteger(L, proxy->ent - g_entities))

/// The entity team.
// @tfield string team Read only.
// @within EntityProxy
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

/// Fields related to players. Will be nil if the entity is not a player.
// @tfield Client client Read/Write.
// @within EntityProxy
// @see client
// @usage if ent.client ~= nil then print(ent.client.name) end -- Print client name
static int Getclient(lua_State* L)
{
	EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
	if (!proxy || !proxy->ent || !proxy->ent->client) return 0;
	if (!proxy->client || proxy->client->ent != proxy->ent)
	{
		proxy->client.reset(new Client(proxy->ent));
	}
	LuaLib<Client>::push(L, proxy->client.get(), false);
	return 1;
}

/// Fields related to buildables. Will be nil if the entity is not a buildable.
// @tfield Buildable buildable Read/Write.
// @within EntityProxy
// @see buildable
static int Getbuildable(lua_State* L)
{
	EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
	if (!proxy || !proxy->ent || proxy->ent->s.eType != entityType_t::ET_BUILDABLE)
	{
		proxy->buildable.reset();
		return 0;
	}
	if (!proxy->buildable || proxy->buildable->ent != proxy->ent)
	{
		proxy->buildable.reset(new Buildable(proxy->ent));
	}
	LuaLib<Buildable>::push(L, proxy->buildable.get(), false);
	return 1;
}

/// Fields related to bots. Will be nil if the entity is not a bot.
// @tfield Bot bot Read/Write.
// @within EntityProxy
// @see bot
static int Getbot(lua_State* L)
{
	EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
	if (!proxy || !proxy->ent || !proxy->ent->botMind)
	{
		proxy->bot.reset();
		return 0;
	}
	if (!proxy->bot || proxy->bot->ent != proxy->ent)
	{
		proxy->bot.reset(new Bot(proxy->ent));
	}
	LuaLib<Bot>::push(L, proxy->bot.get(), false);
	return 1;
}

static int Setorigin(lua_State* L)
{
	if (lua_istable(L, 2))
	{
		EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
		vec3_t origin;
		Shared::Lua::CheckVec3(L, 2, origin);
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
		Shared::Lua::CheckVec3(L, 2, angles);
		VectorCopy(angles, proxy->ent->s.angles);
	}
	return 0;
}

static int Setnextthink(lua_State* L)
{
	EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
	if (!proxy) return 0;
	int nextthink = luaL_checknumber(L, 2);
	if (nextthink > level.time)
	{
		proxy->ent->nextthink = nextthink;
	}
	return 0;
}

static int Setmins(lua_State* L)
{
	if (lua_istable(L, 2))
	{
		EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
		if (!proxy) return 0;
		vec3_t mins;
		Shared::Lua::CheckVec3(L, 2, mins);
		VectorCopy(mins, proxy->ent->r.mins);
	}
	return 0;
}

static int Setmaxs(lua_State* L)
{
	if (lua_istable(L, 2))
	{
		EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
		if (!proxy) return 0;
		vec3_t maxs;
		Shared::Lua::CheckVec3(L, 2, maxs);
		VectorCopy(maxs, proxy->ent->r.maxs);
	}
	return 0;
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
	lua_pushinteger(L, num);
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
		EntityProxy* proxy = Entity::proxies[entityNum]; \
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
			EntityProxy::EntityFunction func = {}; \
			func.type = EntityProxy::upper; \
			func.luaRef = ref; \
			func.method = proxy->ent->method; \
			it = proxy->funcs.insert({EntityProxy::upper, std::move(func)}).first; \
			proxy->ent->method = Exec##method; \
		} \
		else \
		{ \
			/* Clean up old ref... */ \
			lua_rawgeti(L, LUA_REGISTRYINDEX, it->second.luaRef); \
			luaL_unref(L, LUA_REGISTRYINDEX, it->second.luaRef); \
			/* If set to nil, remove lua func all together */ \
			if (ref == -1) \
			{ \
				if (!it->second.method) return 0; \
				proxy->ent->method = it->second.method; \
				it->second.method = nullptr; \
			} \
			else \
			{ \
				it->second.luaRef = ref; \
				if (proxy->ent->method != Exec##method) \
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
		lua_pushboolean(L, proxy->ent->method != nullptr); \
		return 1; \
	}

/// The Lua think function. Will be called every time the entity thinks.
// General notes about these Lua Entity functions:
// Does not override the existing function. Runs in addition to it.
// On read, returns true or nil if the think function is set.
// On write, accepts a function.
// Set to nil to clear the Lua function.
// @tfield function|bool think function(EntityProxy self)
// @tparam EntityProxy self The current entity.
// @within EntityProxy
ExecFunc(think, THINK, (gentity_t* self), 1, self)
/// The Lua reset function. Will be called every time the entity restets.
// @tfield function|bool reset function(EntityProxy self)
// @tparam EntityProxy self The current entity.
// @within EntityProxy
// @see think
ExecFunc(reset, RESET, (gentity_t* self), 1, self)
/// The Lua reached function. Will be called every time a mover reaches its destination.
// @tfield function|bool reset function(EntityProxy self)
// @tparam EntityProxy self The current entity.
// @within EntityProxy
// @see think
ExecFunc(reached, REACHED, (gentity_t* self), 1, self)
/// The Lua blocked function. Will be called every time a mover is blocked.
// @tfield function|bool blocked function(EntityProxy self, EntityProxy blocker)
// @tparam EntityProxy self The current entity (the mover).
// @tparam EntityProxy blocker The blocking entity.
// @within EntityProxy
// @see think
ExecFunc(blocked, BLOCKED, (gentity_t* self, gentity_t* other), 2, self, other)
/// The Lua touch function. Will be called every time a collidable entity is touched.
// @tfield function|bool touch function(EntityProxy self, EntityProxy toucher)
// @tparam EntityProxy self The current entity.
// @tparam EntityProxy toucher The touching entity.
// @within EntityProxy
// @see think
ExecFunc(touch, TOUCH, (gentity_t* self, gentity_t* other, trace_t* trace), 2, self, other, trace)
/// The Lua use function. Will be called every time a usable entity is used.
// @tfield function|bool use function(EntityProxy self, EntityProxy caller, EntityProxy activator)
// @tparam EntityProxy self The current entity.
// @tparam EntityProxy caller The calling entity (idk what this means.)
// @tparam EntityProxy activator The entity that uses this entity.
// @within EntityProxy
// @see think
ExecFunc(use, USE, (gentity_t* self, gentity_t* other, gentity_t* act), 3, self, other, act)
/// The Lua pain function. Will be called every time an entity takes damage.
// @tfield function|bool pain function(EntityProxy self, EntityProxy attacker, integer damage)
// @tparam EntityProxy self The current entity.
// @tparam EntityProxy attacker|nil The entity that initiated the damage.
// @tparam integer damage The amount of damage taken.
// @within EntityProxy
// @see think
ExecFunc(pain, PAIN, (gentity_t* self, gentity_t* attacker, int damage), 3, self, attacker, damage)
/// The Lua die function. Will be called every time an entity dies.
// @tfield function|bool die function(EntityProxy self, EntityProxy inflictor, EntityProxy attacker, integer mod)
// @tparam EntityProxy self The current entity.
// @tparam EntityProxy|nil inflictor The entity that killed the entity. idk when these are different.
// @tparam EntityProxy|nil attacker The entity that killed the entity.
// @tparam integer mod The kill cause number. Look at bg_public.h.
// @within EntityProxy
// @see think
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
	GETTER(mins),
	GETTER(maxs),
	GETTER(number),
	// Getters for functions just return bool if the function is set.
	GETTER(think),
	GETTER(reset),
	GETTER(reached),
	GETTER(blocked),
	GETTER(touch),
	GETTER(use),
	GETTER(pain),
	GETTER(die),
	GETTER(client),
	GETTER(buildable),
	GETTER(bot),

	{ nullptr, nullptr }
};

luaL_Reg EntityProxySetters[] =
{
	SETTER(origin),
	SETTER(angles),
	SETTER(nextthink),
	SETTER(mins),
	SETTER(maxs),
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
LUASGAMETYPEDEFINE(EntityProxy, false)
template<>
void ExtraInit<Unv::SGame::Lua::EntityProxy>(lua_State* L, int metatable_index) {}
} } }
