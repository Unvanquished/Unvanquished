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
#include "register_lua_extensions.h"

namespace Shared {
namespace Lua {

namespace {

int Cvar_get(lua_State* L)
{
	const char *cvar = luaL_checkstring(L, 1);
	lua_pushstring(L, Cvar::GetValue(cvar).c_str());
	return 1;
}

int Cvar_set(lua_State* L)
{
	const char *cvar = luaL_checkstring(L, 1);
	const char *value = luaL_checkstring(L, 2);
	Cvar::SetValue(cvar, value);
	return 0;
}

int Cvar_archive(lua_State* L)
{
	const char *cvar  = luaL_checkstring(L, 1);
	Cvar::AddFlags(cvar, Cvar::USER_ARCHIVE);
	return 0;
}

}  // namespace

void RegisterCvar(lua_State* L)
{
	lua_newtable(L);
	lua_pushcfunction(L, Cvar_get);
	lua_setfield(L, -2, "get");
	lua_pushcfunction(L, Cvar_set);
	lua_setfield(L, -2, "set");
	lua_pushcfunction(L, Cvar_archive);
	lua_setfield(L, -2, "archive");
	lua_setglobal(L, "Cvar");
}

}  // namespace Lua
}  // namespace Shared
