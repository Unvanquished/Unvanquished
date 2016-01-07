/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

#include "Player.h"
#include "../../cg_local.h"

static Rml::Lua::Player player;
void CG_InitializeLuaPlayer(lua_State* L)
{
	Rml::Lua::LuaType<Rml::Lua::Player>::Register( L );
	Rml::Lua::LuaType<Rml::Lua::Player>::push( L, &player, false );
	lua_setglobal( L, "player" );
}

namespace Rml {
namespace Lua {
#define GETTER(name) { #name, Get##name }

#define GET_FUNC( name, var, type ) \
static int Get##name( lua_State* L ) \
{ \
	playerState_t* ps = &cg.snap->ps; \
	if ( ps ) \
	{ \
		lua_push##type( L, var ); \
		return 1; \
	} \
	return 0; \
}

GET_FUNC( team, BG_TeamName( ps->persistant[ PERS_TEAM ] ), string )
GET_FUNC( class, BG_Class( ps->stats[ STAT_CLASS ] )->name, string )
GET_FUNC( weapon, BG_Weapon( ps->stats[ STAT_WEAPON ] )->name, string )
GET_FUNC( hp, ps->stats[ STAT_HEALTH ], integer )
GET_FUNC( ammo, ps->ammo, integer )
GET_FUNC( clips, ps->clips, integer )
GET_FUNC( credits, ps->persistant[ PERS_CREDIT], integer )
GET_FUNC( score, ps->persistant[ PERS_SCORE], integer )

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
	{ nullptr, nullptr }
};
luaL_Reg PlayerSetters[] =
{
	{ nullptr, nullptr },
};
RMLUI_LUATYPE_DEFINE(Player)
} // namespace Lua
} // namespace Rml
