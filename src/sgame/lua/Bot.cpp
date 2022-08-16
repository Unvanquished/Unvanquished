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

#include "sgame/lua/Bot.h"
#include "shared/lua/LuaLib.h"
#include "sgame/sg_local.h"
#include "sgame/sg_bot_local.h"
#include "sgame/sg_bot_ai.h"

using Unv::Shared::Lua::LuaLib;
using Unv::Shared::Lua::RegType;

/// Handle interactions with Bots.
// @module bot

namespace Unv {
namespace SGame {
namespace Lua {

#define GET_FUNC( var, func) \
static int Get##var( lua_State* L ) \
{ \
    Bot* c = LuaLib<Bot>::check(L, 1); \
    if (!c || !c->ent || !c->ent->botMind) \
    { \
        Log::Warn("trying to access stale bot info!"); \
        return 0; \
    } \
	func; \
	return 1; \
}

/// Bot skill level. From 1-9. Higher skill levels are better.
// @tfield integer skill_level Read/Write.
// @within Bot
GET_FUNC( skill_level, lua_pushinteger( L, c->ent->botMind->botSkill.level ) )
/// How fast the bot will aim; basically the bot reaction time.
// @tfield number aim_slowness Read/Write.
// @within Bot
GET_FUNC( aim_slowness, lua_pushnumber( L, c->ent->botMind->botSkill.aimSlowness ) )
/// Randomness in the bot aim.
// @tfield number aim_shake Read/Write.
// @within Bot
GET_FUNC( aim_shake, lua_pushnumber( L, c->ent->botMind->botSkill.aimShake ) )

/// Name of the current behavior tree.
// @tfield string behavior Read only.
// @within Bot
// @see set_behavior
static int Getbehavior( lua_State* L)
{
    Bot* c = LuaLib<Bot>::check(L, 1);
    if (!c || !c->ent || !c->ent->botMind)
    {
        Log::Warn("trying to access stale bot info!");
        return 0;
    }
	if ( !c->ent->botMind->behaviorTree )
    {
        return 0;
    }
    lua_pushstring( L, c->ent->botMind->behaviorTree->name );
	return 1;
}

/// Set a new behavior tree for the bots.
// @function set_behavior
// @tparam string behavior New behavior tree file.
// @within Bot
// @see behavior
static int MethodSetBehavior( lua_State* L, Bot* c )
{
    if (!c || !c->ent || !c->ent->botMind)
    {
        Log::Warn("trying to access stale bot info!");
        return 0;
    }
    const char* behavior = luaL_checkstring( L, 1 );
    if ( !behavior )
    {
        Log::Warn("empty behavior");
        return 0;
    }
    G_BotChangeBehavior( g_entities - c->ent, behavior );
    return 0;
}

RegType<Bot> BotMethods[] =
{
    { "set_behavior", MethodSetBehavior },
    { nullptr, nullptr },
};
#define GETTER(name) { #name, Get##name }
luaL_Reg BotGetters[] =
{
    GETTER( skill_level ),
    GETTER( aim_slowness ),
    GETTER( aim_shake ),
    GETTER( behavior ),
	{ nullptr, nullptr },
};

static int Setskill_level( lua_State* L )
{
    Bot* c = LuaLib<Bot>::check(L, 1);
    if (!c || !c->ent || !c->ent->botMind)
    {
        Log::Warn("trying to access stale bot info!");
        return 0;
    }
    c->ent->botMind->botSkill.level = luaL_checkinteger( L, 1 );
    return 0;
}

static int Setaim_slowness( lua_State* L )
{
    Bot* c = LuaLib<Bot>::check(L, 1);
    if (!c || !c->ent || !c->ent->botMind)
    {
        Log::Warn("trying to access stale bot info!");
        return 0;
    }
    c->ent->botMind->botSkill.aimSlowness = luaL_checknumber( L, 1 );
    return 0;
}

static int Setaim_shake( lua_State* L )
{
    Bot* c = LuaLib<Bot>::check(L, 1);
    if (!c || !c->ent || !c->ent->botMind)
    {
        Log::Warn("trying to access stale bot info!");
        return 0;
    }
    c->ent->botMind->botSkill.aimShake = luaL_checknumber( L, 1 );
    return 0;
}

#define SETTER(name) { #name, Set##name }
luaL_Reg BotSetters[] =
{
    SETTER( skill_level ),
    SETTER( aim_slowness ),
    SETTER( aim_shake ),
	{ nullptr, nullptr },
};

} // namespace Lua
} // namespace SGame
} // namespace Unv

namespace Unv {
namespace Shared {
namespace Lua {
LUASGAMETYPEDEFINE(Bot, false)
template<>
void ExtraInit<Unv::SGame::Lua::Bot>(lua_State* /*L*/, int /*metatable_index*/) {}
}  // namespace Lua
}  // namespace Shared
}  // namespace Unv
