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
#include "Weapons.h"
namespace Unv {
namespace Shared {
namespace Lua {

#define GETTER(name) { #name, Get##name }

WeaponProxy::WeaponProxy( int weapon ) :
	attributes( BG_Weapon( weapon ) ) {}

#define GET_FUNC( var, type ) \
static int Get##var( lua_State* L ) \
{ \
	WeaponProxy* proxy = LuaLib<WeaponProxy>::check( L, 1 ); \
	lua_push##type( L, proxy->attributes->var ); \
	return 1; \
}

#define GET_FUNC2( name, var, type ) \
static int Get##name( lua_State* L ) \
{ \
	WeaponProxy* proxy = LuaLib<WeaponProxy>::check( L, 1 ); \
	lua_push##type( L, var ); \
	return 1; \
}

GET_FUNC( price, integer )
GET_FUNC2( unlock_threshold, proxy->attributes->unlockThreshold, integer )
GET_FUNC2( name, proxy->attributes->humanName, string )
GET_FUNC( info, string )
GET_FUNC( slots, integer )
GET_FUNC2( ammo, proxy->attributes->maxAmmo, integer )
GET_FUNC2( clips, proxy->attributes->maxClips, integer )
GET_FUNC2( infinite_ammo, proxy->attributes->infiniteAmmo, boolean )
GET_FUNC2( energy, proxy->attributes->usesEnergy, boolean )
GET_FUNC2( repeat_rate1, proxy->attributes->repeatRate1, integer )
GET_FUNC2( repeat_rate2, proxy->attributes->repeatRate2, integer )
GET_FUNC2( repeat_rate3, proxy->attributes->repeatRate3, integer )
GET_FUNC2( reload_time, proxy->attributes->reloadTime, integer )
GET_FUNC2( alt_mode, proxy->attributes->hasAltMode, boolean )
GET_FUNC2( zoom, proxy->attributes->canZoom, boolean )
GET_FUNC( purchasable, boolean )
GET_FUNC2( long_ranged, proxy->attributes->longRanged, boolean )
GET_FUNC2( team, BG_TeamName( proxy->attributes->team ), string )

template<> void ExtraInit<WeaponProxy>( lua_State* /*L*/, int /*metatable_index*/ ) {}
RegType<WeaponProxy> WeaponProxyMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg WeaponProxyGetters[] =
{
	GETTER(price),
	GETTER(unlock_threshold),
	GETTER(name),
	GETTER(info),
	GETTER(slots),
	GETTER(ammo),
	GETTER(clips),
	GETTER(infinite_ammo),
	GETTER(energy),
	GETTER(repeat_rate1),
	GETTER(repeat_rate2),
	GETTER(repeat_rate3),
	GETTER(reload_time),
	GETTER(alt_mode),
	GETTER(zoom),
	GETTER(purchasable),
	GETTER(long_ranged),
	GETTER(team),
	{ nullptr, nullptr },
};

luaL_Reg WeaponProxySetters[] =
{
	{ nullptr, nullptr },
};

LUACORETYPEDEFINE(WeaponProxy, false)

int Weapons::index( lua_State* L )
{
	const char *weaponName = luaL_checkstring( L, -1 );
	weapon_t weapon = BG_WeaponNumberByName( weaponName );
	if ( weapon > 0 && static_cast<size_t>( weapon ) - 1 < weapons.size() )
	{
		LuaLib<WeaponProxy>::push( L, &weapons[ weapon - 1 ], false );
		return 1;
	}
	return 0;
}

int Weapons::pairs( lua_State* L )
{
	int* pindex = static_cast<int*>( lua_touserdata( L, 3 ) );
	if ( *pindex < 0 ) *pindex = 0;
	if ( static_cast<size_t>( *pindex ) >= weapons.size() )
	{
		lua_pushnil( L );
		lua_pushnil( L );
	}
	else
	{
		lua_pushstring( L, weapons[ *pindex ].attributes->name );
		LuaLib<WeaponProxy>::push( L, &weapons[ (*pindex)++ ], false );
	}
	return 2;
}

std::vector<WeaponProxy> Weapons::weapons;

template<> void ExtraInit<Weapons>( lua_State* L, int metatable_index )
{
	// overwrite index function
	lua_pushcfunction( L, Weapons::index );
	lua_setfield( L, metatable_index, "__index" );
	lua_pushcfunction( L, Weapons::pairs );
	lua_setfield( L, metatable_index, "__pairs" );

	for ( int i = WP_NONE + 1; i < WP_NUM_WEAPONS; ++i)
	{
		Weapons::weapons.emplace_back( i );
	}
}
RegType<Weapons> WeaponsMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg WeaponsGetters[] =
{
	{ nullptr, nullptr },
};

luaL_Reg WeaponsSetters[] =
{
	{ nullptr, nullptr },
};
LUACORETYPEDEFINE(Weapons, false)

} // namespace Lua
} // namespace Shared
} // namespace Unv
