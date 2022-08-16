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
/// Register hooks to install callbacks on various game events.
// Multiple callbacks can be registered for each event; however,
// no callbacks may be unregistered.
/// @module hooks

namespace Unv {
namespace SGame {
namespace Lua {


using LuaHook = std::pair<lua_State*, int>;

static std::vector<LuaHook> chatHooks;
static std::vector<LuaHook> clientConnectHooks;
static std::vector<LuaHook> teamChangeHooks;
static std::vector<LuaHook> playerSpawnHooks;
static std::vector<LuaHook> gameEndHooks;
/// Install a callback that will be called for every chat message.
// The callback should be  function(EntityProxy, team, message).
// where team = 'alien', 'human', '&lt;team&gt;' (for all chat)
// and message is just the message including any quake3 colors.
// @function RegisterChatHook
// @tparam function callback function(EntityProxy, team, message)
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
    // nullptr ent can be for console chats.
    if (!ent) return;
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


/// Install a callback that will be called for every client connect or disconnect.
// The callback should be  function(EntityProxy, connect).
// where connect = true for connect and false for disconnect.
// @function RegisterClientConnectHook
// @tparam function callback function(EntityProxy, connect)
int RegisterClientConnectHook(lua_State* L)
{
    if (lua_isfunction(L, 1))
    {
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        clientConnectHooks.emplace_back(L, ref);
    }
    return 0;
}

void ExecClientConnectHooks(gentity_t* ent, bool connect)
{
    // nullptr ent can be for console chats.
    if (!ent) return;
    for (const auto& hook: clientConnectHooks)
    {
        lua_rawgeti(hook.first, LUA_REGISTRYINDEX, hook.second); \
        EntityProxy* proxy = Entity::CreateProxy(ent, hook.first);
        LuaLib<EntityProxy>::push(hook.first, proxy, false);
        lua_pushboolean(hook.first, connect);
        if (lua_pcall(hook.first, 2, 0, 0) != 0)
		{
			Log::Warn( "Could not run lua client connect hook callback: %s",
				lua_tostring(hook.first, -1));
		}
    }
}

/// Install a callback that will be called for every time a client changes teams.
// The callback should be  function(EntityProxy, newTeam).
// where newTeam will be 'alien', 'human', 'spectator'.
// @function RegisterTeamChangeHook
// @tparam function callback function(EntityProxy, newTeam)
int RegisterTeamChangeHook(lua_State* L)
{
    if (lua_isfunction(L, 1))
    {
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        teamChangeHooks.emplace_back(L, ref);
    }
    return 0;
}

void ExecTeamChangeHooks(gentity_t* ent, team_t team)
{
    // nullptr ent can be for console chats.
    if (!ent) return;
    for (const auto& hook: teamChangeHooks)
    {
        lua_rawgeti(hook.first, LUA_REGISTRYINDEX, hook.second); \
        EntityProxy* proxy = Entity::CreateProxy(ent, hook.first);
        LuaLib<EntityProxy>::push(hook.first, proxy, false);
        lua_pushstring(hook.first, BG_TeamName(team));
        if (lua_pcall(hook.first, 2, 0, 0) != 0)
		{
			Log::Warn( "Could not run lua team change hook callback: %s",
				lua_tostring(hook.first, -1));
		}
    }
}

/// Install a callback that will be called for every time a player changes classes.
// This includes initial spawning, but also evolving, de-evolving, buying a bsuit, etc.
// The callback should be  function(EntityProxy).
// @function RegisterPlayerSpawnHook
// @tparam function callback function(EntityProxy)
int RegisterPlayerSpawnHook(lua_State* L)
{
    if (lua_isfunction(L, 1))
    {
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        playerSpawnHooks.emplace_back(L, ref);
    }
    return 0;
}

void ExecPlayerSpawnHooks(gentity_t* ent)
{
    // nullptr ent can be for console chats.
    if (!ent) return;
    for (const auto& hook: playerSpawnHooks)
    {
        lua_rawgeti(hook.first, LUA_REGISTRYINDEX, hook.second); \
        EntityProxy* proxy = Entity::CreateProxy(ent, hook.first);
        LuaLib<EntityProxy>::push(hook.first, proxy, false);
        if (lua_pcall(hook.first, 1, 0, 0) != 0)
		{
			Log::Warn( "Could not run lua player spawn hook callback: %s",
				lua_tostring(hook.first, -1));
		}
    }
}

/// Install a callback that will be called to check whether a game should end.
// The callback should be  function() and should return either 'human', 'alien', depending
// on whether the respective team has won the game. If no team has won, the function should
// return false.
// @function RegisterGameEndHook
// @tparam function callback function()
int RegisterGameEndHook(lua_State* L)
{
    if (lua_isfunction(L, 1))
    {
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        gameEndHooks.emplace_back(L, ref);
    }
    return 0;
}

team_t ExecGameEndHooks()
{
    for (const auto& hook: gameEndHooks)
    {
        lua_rawgeti(hook.first, LUA_REGISTRYINDEX, hook.second); \
        if (lua_pcall(hook.first, 0, 1, 0) != 0)
		{
			Log::Warn( "Could not run lua game end hook callback: %s",
				lua_tostring(hook.first, -1));
		}
        if (lua_toboolean(hook.first, -1))
        {
            const char* teamName = luaL_checkstring(hook.first, -1);
            team_t team = BG_PlayableTeamFromString(teamName);
            if (team != TEAM_NONE)
            {
                lua_pop( hook.first, -1 );
                return team;
            }
            lua_pop( hook.first, -1 );
        }
    }
    return TEAM_NONE;
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
    lua_pushcfunction( L, Unv::SGame::Lua::RegisterClientConnectHook);
	lua_setfield( L, metatable_index - 1, "RegisterClientConnectHook" );
    lua_pushcfunction( L, Unv::SGame::Lua::RegisterTeamChangeHook);
	lua_setfield( L, metatable_index - 1, "RegisterTeamChangeHook" );
    lua_pushcfunction( L, Unv::SGame::Lua::RegisterPlayerSpawnHook);
	lua_setfield( L, metatable_index - 1, "RegisterPlayerSpawnHook" );
    lua_pushcfunction( L, Unv::SGame::Lua::RegisterGameEndHook);
	lua_setfield( L, metatable_index - 1, "RegisterGameEndHook" );
}
}  // namespace Lua
}  // namespace Shared
}  // namespace Unv
