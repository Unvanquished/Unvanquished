/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2021 Unvanquished Developers

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

#include "CDataSource.h"
#include "cgame/rocket/rocket.h"
#include <Rocket/Core/Core.h>
#include <Rocket/Controls/DataSource.h>
#include <Rocket/Controls/DataQuery.h>
#include "cgame/cg_local.h"

// Lua function to request the datasource be (re)built, e.g.
// CDataSource.Build('beaconList', 'default')
static int CDataSource_Build(lua_State* L)
{
	const char* sourceName = luaL_checkstring(L, 1);
	const char* table = luaL_checkstring(L, 2);
	CG_Rocket_BuildDataSource(sourceName, table);
	return 0;
}

// Lua function to read selected columns of a data source, e.g.
// CDataSource.Read('beaconList', 'default', 'name,desc')
// -> [["Pointer", "Attract teammates' attention to a very specific point."], ["Attack", "Tell your team to attack a specific place."], ...]
static int CDataSource_Read(lua_State* L)
{
	const char* sourceName = luaL_checkstring(L, 1);
	const char* table = luaL_checkstring(L, 2);
	const char* fields = luaL_checkstring(L, 3);
	Rocket::Controls::DataSource* source = Rocket::Controls::DataSource::GetDataSource(sourceName);
	if (!source) {
		Log::Warn("No data source '%s'", sourceName);
		lua_pushnil(L);
		return 1;
	}
	Rocket::Controls::DataQuery query(source, Rocket::Core::String(table), Rocket::Core::String(fields), 0, -1);
	lua_newtable(L);
	int row = 0;
	while ( query.NextRow() ) {
		lua_pushinteger(L, ++row);
		lua_createtable(L, query.GetNumFields(), 0);
		for (size_t i = 0; i < query.GetNumFields(); i++ ) {
			auto value = query.Get<Rocket::Core::String>(i, "");
			lua_pushinteger(L, static_cast<int>(i + 1));
			lua_pushstring(L, value.CString());
			lua_settable(L, -3);
		}
		lua_settable(L, -3);
	}
	return 1;
}

void CG_Rocket_RegisterLuaCDataSource(lua_State* L)
{
	lua_newtable(L);
	lua_pushcfunction(L, CDataSource_Build);
	lua_setfield(L, -2, "Build");
	lua_pushcfunction(L, CDataSource_Read);
	lua_setfield(L, -2, "Read");
	lua_setglobal(L, "CDataSource");
}
