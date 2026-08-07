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

#include "sgame/lua/Entity.h"

#include "sgame/lua/EntityProxy.h"
#include "shared/lua/LuaLib.h"
#include "shared/lua/Utils.h"
#include "sgame/sg_local.h"

using Shared::Lua::LuaLib;
using Shared::Lua::RegType;

/// Handle interactions with Entities.
// @module entity

namespace Lua {
// These are freed by the lua GC.
std::vector<EntityProxy*> Entity::proxies( MAX_GENTITIES );

EntityProxy* Entity::CreateProxy( gentity_t* ent, lua_State* L )
{
	int entNum = ent->num();
	if ( !Entity::proxies[ entNum ] || !Entity::proxies[ entNum ]->ent )
	{
		Entity::proxies[ entNum ] = new EntityProxy( ent, L );
	}
	return Entity::proxies[ entNum ];
}

namespace {

/// FindById an entity based on their id. This id must be set manually prior.
// @function find_by_id
// @tparam string id The entity id.
// @treturn EntityProxy|nil Returns EntityProxy if it finds a match or nil.
// @within entity
int FindById( lua_State* L )
{
	const char* id = luaL_checkstring( L, 1 );
	gentity_t* ent = nullptr;
	while ( ( ent = G_IterateEntities( ent ) ) )
	{
		if ( ent->id && !strncmp( id, ent->id, strlen( ent->id ) ) )
		{
			EntityProxy* proxy = Entity::CreateProxy( ent, L );
			LuaLib<EntityProxy>::push( L, proxy );
			return 1;
		}
	}

	return 0;
}

/// Allows iterating over a group of entities given a class name.
// @function iterate_classname
// @tparam string class_name The class name to search for.
// @treturn EntityProxy...|nil Returns one or more EntityProxies if any matches or nil.
// @within entity
int IterateByClassName( lua_State* L )
{
	const char* name = luaL_checkstring( L, 1 );
	gentity_t* ent = nullptr;
	int ret = 0;
	while ( ( ent = G_IterateEntities( ent ) ) )
	{
		if ( !Q_stricmp( ent->classname, name ) )
		{
			EntityProxy* proxy = Entity::CreateProxy( ent, L );
			LuaLib<EntityProxy>::push( L, proxy );
			ret++;
		}
	}
	return ret;
}

/// Fetch an entity by its entity number via entity[idx]. Do not use index() directly.
// @function index
// @tparam integer index The entity number.
// @treturn EntityProxy|nil Returns EntityProxy if there one exists or nil.
// @usage local ent = sgame.entity[0] -- Get the first entity.
// @within entity
int index( lua_State* L )
{
	int type = lua_type( L, -1 );
	if ( type != LUA_TNUMBER ) return LuaLib<Entity>::index( L );
	int entNum = luaL_checknumber( L, -1 );
	if ( entNum < 0 || entNum >= MAX_GENTITIES )
	{
		Log::Warn( "index out of range: %d", entNum );
		return 0;
	}
	gentity_t* ent = &g_entities[ entNum ];
	if ( !ent->inuse ) return 0;
	LuaLib<EntityProxy>::push( L, Entity::CreateProxy( ent, L ) );
	return 1;
}

int pairs( lua_State* L )
{
	return Shared::Lua::CreatePairsHelper( L, [](lua_State* L, size_t& i ) {
		if ( i >= MAX_GENTITIES )
		{
			return 0;
		}
		while ( i < MAX_GENTITIES )
		{
			gentity_t* ent = &g_entities[ i ];
			if ( !ent->inuse )
			{
				i++;
				continue;
			}
			lua_pushinteger( L, i );
			LuaLib<EntityProxy>::push( L, Entity::CreateProxy( ent, L ) );
			return 2;
		}

		return 0;
	});
}

/// Create a new entity. Its classname is hardcoded to "lua".
// @function new
// @treturn EntityProxy Returns an EntityProxy.
// @usage local ent = sgame.entity.new() -- make a new entity
// @within entity
int New( lua_State* L )
{
	gentity_t* ent = G_NewEntity( initEntityStyle_t::NO_CBSE );
	ent->classname = "lua";
	ent->s.eType = entityType_t::ET_GENERAL;
	trap_LinkEntity( ent );
	LuaLib<EntityProxy>::push( L, Entity::CreateProxy( ent, L ) );
	return 1;
}

/// Remove an entity created using `new()`. Cannot be used to remove any other entity.
// @function delete
// @usage local ent = sgame.entity.new() -- make a new entity
// @usage local ent = sgame.entity.new(); sgame.entity.delete(ent) -- make a new entity, then delete.
// @within entity
int Delete( lua_State* L )
{
	EntityProxy* proxy = LuaLib<EntityProxy>::check( L, 1 );
	if ( !proxy ) return 0;
	if ( Q_strncmp( proxy->ent->classname, "lua", strlen( proxy->ent->classname ) ) != 0 )
	{
		Log::Warn( "Lua code only allowed to delete entities created by lua." );
		return 0;
	}
	G_FreeEntity( proxy->ent );
	return 0;
}


RegType<::Lua::Entity> EntityMethods[] =
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

}  // namespace
}  // namespace Lua

namespace Shared {
namespace Lua {

LUACORETYPEDEFINE( ::Lua::Entity )
template<>
void ExtraInit<::Lua::Entity>(lua_State* L, int metatable_index)
{
	lua_pushcfunction( L, ::Lua::FindById);
	lua_setfield( L, metatable_index - 1, "find_by_id" );
	lua_pushcfunction( L, ::Lua::IterateByClassName);
	lua_setfield( L, metatable_index - 1, "iterate_classname" );
	lua_pushcfunction( L, ::Lua::New );
	lua_setfield( L, metatable_index - 1, "new" );
	lua_pushcfunction( L, ::Lua::Delete );
	lua_setfield( L, metatable_index - 1, "delete" );


	// overwrite index functions to allow indexing entities by entity number.
	lua_pushcfunction( L, ::Lua::index );
	lua_setfield( L, metatable_index, "__index" );
	lua_pushcfunction( L, ::Lua::pairs );
	lua_setfield( L, metatable_index, "__pairs" );
}
}  // namespace Lua
}  // namespace Shared
