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
#ifndef LUAEVENTS_H
#define LUAEVENTS_H
#include "../rocket.h"
#include <Rocket/Core/Core.h>
#include <Rocket/Core/Lua/lua.hpp>
#include <Rocket/Core/Lua/LuaType.h>

namespace Rocket {
namespace Core {
namespace Lua {
// Dummy class for Events
class Events
{

};

template<> void ExtraInit<Events>(lua_State* L, int metatable_index);
int Eventspushcmd(lua_State* L);
int Eventspushevent(lua_State* L);
int Eventspushelement(lua_State* L);

extern RegType<Events> EventsMethods[];
extern luaL_Reg EventsGetters[];
extern luaL_Reg EventsSetters[];
}
}
}
#endif
