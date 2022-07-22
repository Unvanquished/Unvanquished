/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unv Developers

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

#include "sgame/lua/Hooks.h"

#include "sgame/lua/Entity.h"
#include "shared/lua/LuaLib.h"
#include "sgame/sg_local.h"

using Unv::Shared::Lua::LuaLib;
using Unv::Shared::Lua::RegType;

namespace Unv {
namespace SGame {
namespace Lua {


using LuaHook = std::pair<lua_State*, int>;

static std::vector<LuaHook> chatHooks;

// Will be called as function(EntityProxy, team, message)
// where team = <team> is all chat.
int RegisterChatHook(lua_State* L)
{
    if (lua_isfunction(L, 1))
    {
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        chatHooks.emplace_back(L, ref);
    }
    return 0;
}

void ExecChatHooks(gentity_t* ent, team_t team, Str::StringRef message)
{
    for (const auto& hook: chatHooks)
    {
        lua_rawgeti(hook.first, LUA_REGISTRYINDEX, hook.second); \
        EntityProxy* proxy = Entity::CreateProxy(ent, hook.first);
        LuaLib<EntityProxy>::push(hook.first, proxy, false);
        lua_pushstring(hook.first, BG_TeamName(team));
        lua_pushstring(hook.first, message.c_str());
        if (lua_pcall(hook.first, 3, 0, 0) != 0)
		{
			Log::Warn( "Could not run lua chat hook callback: %s",
				lua_tostring(hook.first, -1));
		}
    }
}

RegType<Hooks> HooksMethods[] =
{
	{ nullptr, nullptr },
};

luaL_Reg HooksGetters[] =
{
	{ nullptr, nullptr },
};

luaL_Reg HooksSetters[] =
{
	{ nullptr, nullptr },
};

void InitializeHooks(lua_State* L)
{
    chatHooks.clear();
    LuaLib<Hooks>::Register(L);
}

} // namespace Lua
} // namespace SGame
} // namespace Unv

namespace Unv {
namespace Shared {
namespace Lua {
LUASGAMETYPEDEFINE(Hooks, false)
template<>
void ExtraInit<Unv::SGame::Lua::Hooks>(lua_State* L, int metatable_index)
{
	lua_pushcfunction( L, Unv::SGame::Lua::RegisterChatHook);
	lua_setfield( L, metatable_index - 1, "RegisterChatHook" );
}
}  // namespace Lua
}  // namespace Shared
}  // namespace Unv
