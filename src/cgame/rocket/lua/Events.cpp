/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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
#include "cgame/rocket/rocket.h"
#include <RmlUi/Lua/LuaType.h>
#include "register_lua_extensions.h"

static int Events_pushcmd(lua_State* L)
{
	Rml::StringList list;
	const char *cmds = luaL_checkstring(L, 1);

	Rml::StringUtilities::ExpandString( list, cmds, ';' );
	for ( size_t i = 0; i < list.size(); ++i )
	{
		Rocket_AddEvent( new RocketEvent_t( list[ i ] ) );
	}

	return 0;
}

static int Events_pushevent(lua_State* L)
{
	Rml::StringList list;
	const char *cmds = luaL_checkstring(L, 1);
	Rml::Event *event = Rml::Lua::LuaType<Rml::Event>::check(L, 2);

	if (event == nullptr)
	{
		Log::Warn("pushevent: invalid event argument");
		return 0;
	}

	Rml::StringUtilities::ExpandString( list, cmds, ';' );
	for ( size_t i = 0; i < list.size(); ++i )
	{
		Rocket_AddEvent( new RocketEvent_t( *event, list[ i ] ) );
	}

	return 0;
}

void CG_Rocket_RegisterLuaEvents(lua_State* L)
{
	lua_newtable(L);
	lua_pushcfunction(L, Events_pushcmd);
	lua_setfield(L, -2, "pushcmd");
	lua_pushcfunction(L, Events_pushevent);
	lua_setfield(L, -2, "pushevent");
	lua_setglobal(L, "Events");
}
