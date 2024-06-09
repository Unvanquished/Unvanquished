/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2024 Unvanquished Developers

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
#ifndef SHARED_LUA_UTILS_H_
#define SHARED_LUA_UTILS_H_

#include "common/Common.h"
#include "shared/bg_public.h"
#include "shared/bg_lua.h"

namespace Shared {
namespace Lua {

// Report errors
void Report(lua_State* L, Str::StringRef place);

// Push a vec3 onto the stack as a table.
void PushVec3(lua_State* L, const vec3_t vec);

// Convert a lua table into a vec3.
bool CheckVec3(lua_State* L, int pos, vec3_t vec);

int CreatePairsHelper(lua_State* L, std::function<int(lua_State*, size_t)> next_funcmake);

} // namespace Lua
} // namespace Shared


#endif  // SHARED_LUA_UTILS_H_
