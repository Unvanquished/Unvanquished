/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "shared/lua/Utils.h"

#include "shared/bg_public.h"

namespace Unv {
namespace Shared {
namespace Lua {
void Report( lua_State* L, Str::StringRef place )
{
	Log::Warn( place );

	while ( true )
	{
		lua_gettop( L );
		const char* msg = lua_tostring( L, -1 );
		if (msg)
		{
			Log::Warn( msg );
			lua_pop( L, 1 );
			continue;
		}
		break;
	}
}

void PushVec3(lua_State* L, const vec3_t vec)
{
	lua_newtable(L);
	for (int i = 0; i < 3; ++i)
	{
		lua_pushnumber(L, vec[i]);
		lua_rawseti(L, -2, i + 1); // lua arrays start at 1
	}
}

bool CheckVec3(lua_State* L, int pos, vec3_t vec)
{
	if (!lua_istable(L, pos))
	{
		Log::Warn("CheckVec3: Input must be a table.");
		return false;
	}
	int index = 0;
	lua_pushnil(L);
	while (lua_next(L, pos) != 0) {
		/* uses 'key' (at index -2) and 'value' (at index -1) */
		vec[index++] = luaL_checknumber(L, -1);
		/* removes 'value'; keeps 'key' for next iteration */
		lua_pop(L, 1);

		if (index >= 3) {
			break;
		}
	}
	return true;
}

} // namespace Lua
} // namespace Shared
} // namespace Unv
