/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2025 Unvanquished Developers

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
#include "common/Common.h"
#include "sgame/sg_local.h"
#include "sgame/sg_entities.h"

namespace Lua {

namespace {

int isNum( lua_State *L )
{
	int num = luaL_checkinteger( L, 1 );
	lua_pushboolean( L,
	                 num >= 0
	                 && num < MAX_CLIENTS
	                 && g_entities[ num ].inuse );
	return 1;
}

int team( lua_State *L )
{
	int num = luaL_checkinteger( L, 1 );
	if ( num < 0 || num >= MAX_CLIENTS || g_entities[ num ].client == nullptr )
	{
		lua_pushnil( L );
	}
	else
	{
		switch ( g_entities[ num ].client->pers.team )
		{
		case TEAM_ALIENS:
			lua_pushstring( L, "aliens" );
			break;
		case TEAM_HUMANS:
			lua_pushstring( L, "humans" );
			break;
		default:
			lua_pushnil( L );
			break;
		}
	}
	return 1;
}
	
}  // namespace

void RegisterClients( lua_State* L )
{
	lua_newtable(L);

	lua_pushcfunction(L, isNum);
	lua_setfield(L, -2, "isNum");

	lua_pushcfunction(L, team);
	lua_setfield(L, -2, "team");

	lua_setglobal( L, "Clients" );
}

}  // namespace Lua
