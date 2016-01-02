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
#ifdef BUILD_CGAME
#include "Buildables.h"

namespace Unv {
namespace Shared {
namespace Lua {


#define GETTER(name) { #name, Get##name }

BuildableProxy::BuildableProxy( int buildable ) :
	attributes( BG_Buildable( buildable ) ) {}

#define GET_FUNC( var, type ) \
static int Get##var( lua_State* L ) \
{ \
	BuildableProxy* proxy = LuaLib<BuildableProxy>::check( L, 1 ); \
	lua_push##type( L, proxy->attributes->var ); \
	return 1; \
}

#define GET_FUNC2( name, var, type ) \
static int Get##name( lua_State* L ) \
{ \
	BuildableProxy* proxy = LuaLib<BuildableProxy>::check( L, 1 ); \
	lua_push##type( L, var ); \
	return 1; \
}

GET_FUNC2( name, proxy->attributes->humanName, string )
GET_FUNC( info, string )
GET_FUNC( icon, string )
GET_FUNC2( build_points, proxy->attributes->buildPoints, integer )
GET_FUNC2( unlock_threshold, proxy->attributes->unlockThreshold, integer )
GET_FUNC( health, integer )
GET_FUNC2( regen_rate, proxy->attributes->regenRate, integer )
GET_FUNC2( splash_damage, proxy->attributes->splashDamage, integer )
GET_FUNC2( splash_radius, proxy->attributes->splashRadius, integer )
GET_FUNC2( weapon, BG_Weapon( proxy->attributes->weapon )->name, string )
GET_FUNC2( build_weapon, BG_Weapon( proxy->attributes->buildWeapon )->name, string )
GET_FUNC2( build_time, proxy->attributes->buildTime, integer )
GET_FUNC( usable, boolean )
GET_FUNC2( team, BG_TeamName( proxy->attributes->team ), string )

template<> void ExtraInit<BuildableProxy>( lua_State* L, int metatable_index ) {}
RegType<BuildableProxy> BuildableProxyMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg BuildableProxyGetters[] =
{
	GETTER(name),
	GETTER(info),
	GETTER(icon),
	GETTER(build_points),
	GETTER(unlock_threshold),
	GETTER(health),
	GETTER(regen_rate),
	GETTER(splash_damage),
	GETTER(splash_radius),
	GETTER(weapon),
	GETTER(build_weapon),
	GETTER(build_time),
	GETTER(usable),
	GETTER(team),
};

luaL_Reg BuildableProxySetters[] =
{
	{ nullptr, nullptr },
};
LUACORETYPEDEFINE(BuildableProxy, false)

int Buildables::index( lua_State* L )
{
	const char *buildableName = luaL_checkstring( L, -1 );
	buildable_t buildable = BG_BuildableByName( buildableName )->number;
	if ( buildable > 0 && static_cast<size_t>( buildable ) - 1 < buildables.size() )
	{
		LuaLib<BuildableProxy>::push( L, &buildables[ buildable - 1 ], false );
		return 1;
	}
	return 0;
}

int Buildables::pairs( lua_State* L )
{
	int* pindex = static_cast<int*>( lua_touserdata( L, 3 ) );
	if ( *pindex < 0 ) *pindex = 0;
	if ( static_cast<size_t>( *pindex ) >= buildables.size() )
	{
		lua_pushnil( L );
		lua_pushnil( L );
	}
	else
	{
		lua_pushstring( L, buildables[ *pindex ].attributes->name );
		LuaLib<BuildableProxy>::push( L, &buildables[ (*pindex)++ ], false );
	}
	return 2;
}

std::vector<BuildableProxy> Buildables::buildables;

template<> void ExtraInit<Buildables>( lua_State* L, int metatable_index )
{
	// overwrite index function
	lua_pushcfunction( L, Buildables::index );
	lua_setfield( L, metatable_index, "__index" );
	lua_pushcfunction( L, Buildables::pairs );
	lua_setfield( L, metatable_index, "__pairs" );

	for ( int i = BA_NONE + 1; i < BA_NUM_BUILDABLES; ++i)
	{
		Buildables::buildables.push_back( BuildableProxy( i ) );
	}
}
RegType<Buildables> BuildablesMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg BuildablesGetters[] =
{
	{ nullptr, nullptr },
};

luaL_Reg BuildablesSetters[] =
{
	{ nullptr, nullptr },
};
LUACORETYPEDEFINE(Buildables, false)

} // namespace Lua
} // namespace Shared
} // namespace Unv
#endif // BUILD_CGAME
