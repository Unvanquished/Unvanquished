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

#include "Timer.h"
#include "LuaLib.h"

namespace Unv {
namespace Shared {
namespace Lua {

Timer timer;

void Timer::Add( int delayMs, int callbackRef, lua_State* L )
{
	events.push_back({delayMs, callbackRef, L});
}

void Timer::RunUpdate(int time)
{
	int dtMs = time - lastTime;
	lastTime = time;

	auto it = events.begin();
	while (it != events.end())
	{
		it->delayMs -= dtMs;
		if (it->delayMs <= 0)
		{
			lua_rawgeti(it->L, LUA_REGISTRYINDEX, it->callbackRef);
			luaL_unref(it->L, LUA_REGISTRYINDEX, it->callbackRef);
			if (lua_pcall(it->L, 0, 0, 0) != 0)
				::Log::Warn( "Could not run lua timer callback: %s",
							lua_tostring(it->L, -1));
			it = events.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void Timer::Update( int time )
{
	timer.RunUpdate(time);
}

int Timeradd( lua_State* L )
{
	int delayMs = luaL_checkinteger(L, 1);
	int ref = luaL_ref(L, LUA_REGISTRYINDEX);
	timer.Add(delayMs, ref, L);
	return 0;
}

template<>
void ExtraInit<Lua::Timer>(lua_State* L, int metatable_index)
{
	//due to they way that LuaType::Register is made, we know that the method table is at the index
	//directly below the metatable
	int method_index = metatable_index - 1;

	lua_pushcfunction(L, Timeradd);
	lua_setfield(L, method_index, "add");
}

RegType<Timer> TimerMethods[] =
{
	{ nullptr, nullptr },
};

luaL_Reg TimerGetters[] =
{
	{ nullptr, nullptr },
};

luaL_Reg TimerSetters[] =
{
	{ nullptr, nullptr },
};

LUACORETYPEDEFINE(Timer, false)

} // namespace Lua
} // namespace Shared
} // namespace Unv
