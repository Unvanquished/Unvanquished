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

#include "common/Common.h"
#include "shared/bg_lua.h"
#include "shared/lua/LuaLib.h"
#include "register_lua_extensions.h"
#include "../../cg_local.h"

using Shared::Lua::RegType;
using Shared::Lua::LuaLib;

namespace {

class Player {};

static Player player;

#define GETTER(name) { #name, Get##name }

#define GET_FUNC( name, var, type ) \
static int Get##name( lua_State* L ) \
{ \
	if ( !cg.snap ) \
	{ \
		return 0; \
	} \
	playerState_t* ps = &cg.snap->ps; \
	lua_push##type( L, var ); \
	return 1; \
}

GET_FUNC( team, BG_TeamName( ps->persistant[ PERS_TEAM ] ), string )
GET_FUNC( class, BG_Class( ps->stats[ STAT_CLASS ] )->name, string )
GET_FUNC( weapon, BG_Weapon( ps->stats[ STAT_WEAPON ] )->name, string )
GET_FUNC( hp, ps->stats[ STAT_HEALTH ], integer )
GET_FUNC( ammo, ps->ammo, integer )
GET_FUNC( clips, ps->clips, integer )
GET_FUNC( credits, ps->persistant[ PERS_CREDIT], integer )

static int Getscore( lua_State* L )
{
	if ( !cg.snap )
	{
		return 0;
	}
	playerState_t* ps = &cg.snap->ps;
	// We're spectating someone else, so our score will be wrong.
	// Let's fetch their score. Note that this score will be delayed
	// based on calls to CG_RequestScores()
	if ( ps->pm_flags & PMF_FOLLOW )
	{
		lua_pushinteger( L, cgs.clientinfo[ ps->clientNum ].score );
	}
	else
	{
		lua_pushinteger( L, ps->persistant[ PERS_SCORE ] );
	}
	return 1;
}

static int GetcrosshairEntityNum( lua_State* L )
{
	if ( cg.time - cg.crosshairEntityTime > 100 )
	{
		lua_pushnil( L );
	}
	else
	{
		lua_pushinteger( L, cg.crosshairEntityNum );
	}
	return 1;
}

}  // namespace

namespace Shared {
namespace Lua {

template<> void ExtraInit<Player>(lua_State* /*L*/, int /*metatable_index*/) {}

RegType<Player> PlayerMethods[] =
{
	{ nullptr, nullptr },
};

luaL_Reg PlayerGetters[] =
{
	GETTER(team),
	GETTER(class),
	GETTER(weapon),
	GETTER(hp),
	GETTER(ammo),
	GETTER(clips),
	GETTER(credits),
	GETTER(score),
	GETTER(crosshairEntityNum),

	{ nullptr, nullptr }
};

luaL_Reg PlayerSetters[] =
{
	{ nullptr, nullptr },
};


LUACORETYPEDEFINE(Player)


}  // namespace Lua
}  // namespace Shared

void CG_Rocket_InitializeLuaPlayer(lua_State* L)
{
	LuaLib<Player>::Register( L );
	LuaLib<Player>::push( L, &player );
	lua_setglobal( L, "player" );
}
