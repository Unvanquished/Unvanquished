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
#include "shared/bg_lua.h"
#include "shared/lua/LuaLib.h"
#include "shared/lua/Buildables.h"
#include "shared/lua/Weapons.h"
#include "shared/lua/Classes.h"
#include "shared/lua/Upgrades.h"
#include "shared/lua/register_lua_extensions.h"
#include "common/Log.h"


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
		LuaLib<Weapons>::push( L, &weapons );
		return 1;
	}

	static int GetUpgrades( lua_State* L )
	{
		LuaLib<Upgrades>::push( L, &upgrades );
		return 1;
	}

	static int GetBuildables( lua_State* L )
	{
		LuaLib<Buildables>::push( L, &buildables );
		return 1;
	}

	static int GetClasses( lua_State* L )
	{
			LuaLib<Classes>::push( L, &classes );
			return 1;
	}
};


UnvGlobal global;

template<> void ExtraInit<UnvGlobal>( lua_State* /*L*/, int /*metatable_index*/ ) {}

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

    { nullptr, nullptr },
};

luaL_Reg UnvGlobalSetters[] =
{
	{ nullptr, nullptr },
};

LUACORETYPEDEFINE(UnvGlobal)

} // namespace Lua
} // namespace Shared

void BG_InitializeLuaConstants( lua_State* L )
{
	using namespace Shared::Lua;

	RegisterCmd( L );
	RegisterCvar( L );
	RegisterTimer( L );

	LuaLib< UnvGlobal >::Register( L );
	LuaLib< Weapons >::Register( L );
	LuaLib< WeaponProxy >::Register( L );
	LuaLib< Buildables >::Register( L );
	LuaLib< BuildableProxy >::Register( L );
	LuaLib< Classes >::Register( L );
	LuaLib< ClassProxy >::Register( L );
	LuaLib< Upgrades >::Register( L );
	LuaLib< UpgradeProxy >::Register( L );
	LuaLib< UnvGlobal>::push( L, &global );
	lua_setglobal( L, "Unv" );
}
