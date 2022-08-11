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

#include "Events.h"

namespace Rml {
namespace Lua {

template<> void ExtraInit<Lua::Events>(lua_State* L, int metatable_index)
{
	//due to they way that LuaType::Register is made, we know that the method table is at the index
	//directly below the metatable
	int method_index = metatable_index - 1;

	lua_pushcfunction(L, Eventspushcmd);
	lua_setfield(L, method_index, "pushcmd");

	lua_pushcfunction(L, Eventspushelement);
	lua_setfield(L, method_index, "pushelement");

	lua_pushcfunction(L, Eventspushevent);
	lua_setfield(L, method_index, "pushevent");

	return;
}

int Eventspushcmd(lua_State* L)
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

int Eventspushevent(lua_State* L)
{
	Rml::StringList list;
	const char *cmds = luaL_checkstring(L, 1);
	Rml::Event *event = LuaType<Rml::Event>::check(L, 2);

	if (event == NULL)
	{
		return 0;
	}

	Rml::StringUtilities::ExpandString( list, cmds, ';' );
	for ( size_t i = 0; i < list.size(); ++i )
	{
		Rocket_AddEvent( new RocketEvent_t( *event, list[ i ] ) );
	}

	return 0;
}

int Eventspushelement(lua_State* L)
{
	Rml::StringList list;
	const char *cmds = luaL_checkstring(L, 1);
	Rml::Element *element = LuaType<Rml::Element>::check(L, 2);

	if (element == NULL)
	{
		return 0;
	}

	Rml::StringUtilities::ExpandString( list, cmds, ';' );
	for ( size_t i = 0; i < list.size(); ++i )
	{
		Rocket_AddEvent( new RocketEvent_t( element, list[ i ] ) );
	}

	return 0;
}

RegType<Events> EventsMethods[] =
{
	{ NULL, NULL },
};

luaL_Reg EventsGetters[] =
{
	{ NULL, NULL },
};

luaL_Reg EventsSetters[] =
{
	{ NULL, NULL },
};

RMLUI_LUATYPE_DEFINE(Events)

}  // namespace Lua
}  // namespace Rml
