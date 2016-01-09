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

#ifndef LUATIMER_H_
#define LUATIMER_H_

#include "../bg_lua.h"
#include "../bg_public.h"

namespace Unv {
namespace Shared {
namespace Lua {

class Timer
{
public:
	void Add(int delayMs, int callbackRef, lua_State* L);
	void RunUpdate( int time );
	static void Update(int time);

private:
	struct TimerEvent
	{
		int delayMs;
		int callbackRef;
		lua_State* L;
	};
	int lastTime;
	std::list<TimerEvent> events;
};

} // namespace Lua
} // namespace Shared
} // namespace Unv
#endif
