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
#include "Cvar.h"
#include "../../cg_local.h"
namespace Rocket {
namespace Core {
namespace Lua {

template<> void ExtraInit<Lua::Cvar>(lua_State* L, int metatable_index)
{
	//due to they way that LuaType::Register is made, we know that the method table is at the index
	//directly below the metatable
	int method_index = metatable_index - 1;

	lua_pushcfunction(L, Cvarget);
	lua_setfield(L, method_index, "get");

	lua_pushcfunction(L, Cvarset);
	lua_setfield(L, method_index, "set");

	lua_pushcfunction(L, Cvararchive);
	lua_setfield(L, method_index, "archive");

	return;
}

int Cvarget(lua_State* L)
{
	const char *cvar = luaL_checkstring(L, 1);
	lua_pushstring(L, ::Cvar::GetValue(cvar).c_str());
	return 1;
}

int Cvarset(lua_State* L)
{
	const char *cvar = luaL_checkstring(L, 1);
	const char *value = luaL_checkstring(L, 2);
	::Cvar::SetValue(cvar, value);
	return 0;
}

int Cvararchive(lua_State* L)
{
	const char *cvar  = luaL_checkstring(L, 1);
	::Cvar::AddFlags(cvar, ::Cvar::USER_ARCHIVE);
	return 0;
}

RegType<Cvar> CvarMethods[] =
{
	{ NULL, NULL },
};

luaL_Reg CvarGetters[] =
{
	{ NULL, NULL },
};

luaL_Reg CvarSetters[] =
{
	{ NULL, NULL },
};

LUACORETYPEDEFINE(Cvar,false)

}
}
}
