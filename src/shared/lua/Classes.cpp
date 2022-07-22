/*
 * ===========================================================================
 *
 * Daemon GPL Source Code
 * Copyright (C) 2012 Unv Developers
 *
 * This file is part of the Daemon GPL Source Code (Daemon Source Code).
 *
 * Daemon Source Code is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Daemon Source Code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, the Daemon Source Code is also subject to certain additional terms.
 * You should have received a copy of these additional terms immediately following the
 * terms and conditions of the GNU General Public License which accompanied the Daemon
 * Source Code.  If not, please request a copy in writing from id Software at the address
 * below.
 *
 * If you have questions concerning this license or the applicable additional terms, you
 * may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
 * Maryland 20850 USA.
 *
 * ===========================================================================
 */
#include "Classes.h"

namespace Unv {
namespace Shared {
namespace Lua {

#define GETTER(name) { #name, Get##name }

ClassProxy::ClassProxy( int clazz ) :
	attributes( BG_Class( clazz ) ) {}

#define GET_FUNC( var, type ) \
static int Get##var( lua_State* L ) \
{ \
	ClassProxy* proxy = LuaLib<ClassProxy>::check( L, 1 ); \
	lua_push##type( L, proxy->attributes->var ); \
	return 1; \
}

#define GET_FUNC2( name, var, type ) \
static int Get##name( lua_State* L ) \
{ \
	ClassProxy* proxy = LuaLib<ClassProxy>::check( L, 1 ); \
	lua_push##type( L, var ); \
	return 1; \
}

GET_FUNC( name, string )
GET_FUNC( info, string )
GET_FUNC( icon, string )
GET_FUNC2( fov_cvar, proxy->attributes->fovCvar, string )
GET_FUNC2( unlock_threshold, proxy->attributes->unlockThreshold, integer )
GET_FUNC( health, integer )
GET_FUNC2( regen_rate, proxy->attributes->regenRate, integer )
GET_FUNC( speed, integer )
GET_FUNC( mass, integer )
GET_FUNC2( jump_magnitude, proxy->attributes->jumpMagnitude, integer )
GET_FUNC( price, integer )
GET_FUNC2( team, BG_TeamName( proxy->attributes->team ), string )

template<> void ExtraInit<ClassProxy>( lua_State* /*L*/, int /*metatable_index*/ ) {}
RegType<ClassProxy> ClassProxyMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg ClassProxyGetters[] =
{
	GETTER(name),
	GETTER(info),
	GETTER(icon),
	GETTER(fov_cvar),
	GETTER(team),
	GETTER(unlock_threshold),
	GETTER(health),
	GETTER(regen_rate),
	GETTER(speed),
	GETTER(mass),
	GETTER(jump_magnitude),
	GETTER(price),
};

luaL_Reg ClassProxySetters[] =
{
	{ nullptr, nullptr },
};
LUACORETYPEDEFINE(ClassProxy, false)

int Classes::index( lua_State* L )
{
	const char *className = luaL_checkstring( L, -1 );
	class_t clazz = BG_ClassByName( className )->number;
	if ( clazz > 0 && static_cast<size_t>( clazz ) - 1 < classes.size() )
	{
		LuaLib<ClassProxy>::push( L, &classes[ clazz - 1 ], false );
		return 1;
	}
	return 0;
}

int Classes::pairs( lua_State* L )
{
	int* pindex = static_cast<int*>( lua_touserdata( L, 3 ) );
	if ( *pindex < 0 ) *pindex = 0;
	if ( static_cast<size_t>( *pindex ) >= classes.size() )
	{
		lua_pushnil( L );
		lua_pushnil( L );
	}
	else
	{
		lua_pushstring( L, classes[ *pindex ].attributes->name );
		LuaLib<ClassProxy>::push( L, &classes[ (*pindex)++ ], false );
	}
	return 2;
}

std::vector<ClassProxy> Classes::classes;

template<> void ExtraInit<Classes>( lua_State* L, int metatable_index )
{
	// overwrite index function
	lua_pushcfunction( L, Classes::index );
	lua_setfield( L, metatable_index, "__index" );
	lua_pushcfunction( L, Classes::pairs );
	lua_setfield( L, metatable_index, "__pairs" );

	for ( int i = PCL_NONE + 1; i < PCL_NUM_CLASSES; ++i)
	{
		Classes::classes.push_back( ClassProxy( i ) );
	}
}
RegType<Classes> ClassesMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg ClassesGetters[] =
{
	{ nullptr, nullptr },
};

luaL_Reg ClassesSetters[] =
{
	{ nullptr, nullptr },
};
LUACORETYPEDEFINE(Classes, false)

} // namespace Lua
} // namespace Shared
} // namespace Unv
