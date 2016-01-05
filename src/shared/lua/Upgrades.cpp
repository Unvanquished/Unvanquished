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
#include "Upgrades.h"

namespace Unv {
namespace Shared {
namespace Lua {


#define GETTER(name) { #name, Get##name }

UpgradeProxy::UpgradeProxy( int upgrade ) :
	attributes( BG_Upgrade( upgrade ) ) {}

#define GET_FUNC( var, type ) \
static int Get##var( lua_State* L ) \
{ \
	UpgradeProxy* proxy = LuaLib<UpgradeProxy>::check( L, 1 ); \
	lua_push##type( L, proxy->attributes->var ); \
	return 1; \
}

#define GET_FUNC2( name, var, type ) \
static int Get##name( lua_State* L ) \
{ \
	UpgradeProxy* proxy = LuaLib<UpgradeProxy>::check( L, 1 ); \
	lua_push##type( L, var ); \
	return 1; \
}

GET_FUNC2( name, proxy->attributes->humanName, string )
GET_FUNC( price, integer )
GET_FUNC( info, string )
GET_FUNC( icon, string )
GET_FUNC( slots, integer )
GET_FUNC2( unlock_threshold, proxy->attributes->unlockThreshold, integer )
GET_FUNC( purchasable, boolean )
GET_FUNC( usable, boolean )
GET_FUNC2( team, BG_TeamName( proxy->attributes->team ), string )

template<> void ExtraInit<UpgradeProxy>( lua_State* L, int metatable_index ) {}
RegType<UpgradeProxy> UpgradeProxyMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg UpgradeProxyGetters[] =
{
	GETTER(name),
	GETTER(price),
	GETTER(unlock_threshold),
	GETTER(info),
	GETTER(icon),
	GETTER(slots),
	GETTER(purchasable),
	GETTER(usable),
	GETTER(team),
};

luaL_Reg UpgradeProxySetters[] =
{
	{ nullptr, nullptr },
};
LUACORETYPEDEFINE(UpgradeProxy, false)

int Upgrades::index( lua_State* L )
{
	const char *upgradeName = luaL_checkstring( L, -1 );
	upgrade_t upgrade = BG_UpgradeByName( upgradeName )->number;
	if ( upgrade > 0 && static_cast<size_t>( upgrade ) - 1 < upgrades.size() )
	{
		LuaLib<UpgradeProxy>::push( L, &upgrades[ upgrade - 1 ], false );
		return 1;
	}
	return 0;
}

int Upgrades::pairs( lua_State* L )
{
	int* pindex = static_cast<int*>( lua_touserdata( L, 3 ) );
	if ( *pindex < 0 ) *pindex = 0;
	if ( static_cast<size_t>( *pindex ) >= upgrades.size() )
	{
		lua_pushnil( L );
		lua_pushnil( L );
	}
	else
	{
		lua_pushstring( L, upgrades[ *pindex ].attributes->name );
		LuaLib<UpgradeProxy>::push( L, &upgrades[ (*pindex)++ ], false );
	}
	return 2;
}

std::vector<UpgradeProxy> Upgrades::upgrades;

template<> void ExtraInit<Upgrades>( lua_State* L, int metatable_index )
{
	// overwrite index function
	lua_pushcfunction( L, Upgrades::index );
	lua_setfield( L, metatable_index, "__index" );
	lua_pushcfunction( L, Upgrades::pairs );
	lua_setfield( L, metatable_index, "__pairs" );

	for ( int i = BA_NONE + 1; i < UP_NUM_UPGRADES; ++i)
	{
		Upgrades::upgrades.push_back( UpgradeProxy( i ) );
	}
}
RegType<Upgrades> UpgradesMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg UpgradesGetters[] =
{
	{ nullptr, nullptr },
};

luaL_Reg UpgradesSetters[] =
{
	{ nullptr, nullptr },
};
LUACORETYPEDEFINE(Upgrades, false)

} // namespace Lua
} // namespace Shared
} // namespace Unv
#endif // BUILD_CGAME
