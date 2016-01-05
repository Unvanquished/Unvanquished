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

#include "bg_public.h"
#include "bg_lua.h"
#include "lua/LuaLib.h"
#include "lua/Weapons.h"
#include "lua/Buildables.h"
#include "lua/Classes.h"
#include "lua/Upgrades.h"
#include <common/Log.h>



namespace Unv {
namespace Shared {
namespace Lua {

static Weapons weapons;
static Buildables buildables;
static Classes classes;
static Upgrades upgrades;

class UnvGlobal
{
public:
	static int GetWeapons( lua_State* L )
	{
		LuaLib<Weapons>::push( L, &weapons, false );
		return 1;
	}

	static int GetUpgrades( lua_State* L )
	{
		LuaLib<Upgrades>::push( L, &upgrades, false );
		return 1;
	}

	static int GetBuildables( lua_State* L )
	{
		LuaLib<Buildables>::push( L, &buildables, false );
		return 1;
	}

	static int GetClasses( lua_State* L )
	{
			LuaLib<Classes>::push( L, &classes, false );
			return 1;
	}
};
UnvGlobal global;
template<> void ExtraInit<UnvGlobal>( lua_State* L, int metatable_index ) {}
RegType<UnvGlobal> UnvGlobalMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg UnvGlobalGetters[] =
{
	{ "weapons", UnvGlobal::GetWeapons },
	{ "upgrades", UnvGlobal::GetUpgrades },
	{ "buildables", UnvGlobal::GetBuildables },
	{ "classes", UnvGlobal::GetClasses },
};
luaL_Reg UnvGlobalSetters[] =
{
	{ nullptr, nullptr },
};
LUACORETYPEDEFINE(UnvGlobal, false)
} // namespace Lua
} // namespace Shared
} // namespace Unv

void BG_InitializeLuaConstants( lua_State* L )
{
	using namespace Unv::Shared::Lua;
	LuaLib< UnvGlobal >::Register( L );
	LuaLib< Weapons >::Register( L );
	LuaLib< WeaponProxy >::Register( L );
	LuaLib< Buildables >::Register( L );
	LuaLib< BuildableProxy >::Register( L );
	LuaLib< Classes >::Register( L );
	LuaLib< ClassProxy >::Register( L );
	LuaLib< Upgrades >::Register( L );
	LuaLib< UpgradeProxy >::Register( L );
	LuaLib< UnvGlobal>::push( L, &global, false );
	lua_setglobal( L, "unv" );
}

#endif // BUILD_CGAME
