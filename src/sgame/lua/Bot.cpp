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
#include "sgame/lua/Bot.h"

#include "sgame/sg_bot_local.h"
#include "sgame/sg_local.h"

#include "sgame/sg_bot_ai.h"
#include "shared/lua/LuaLib.h"

using Shared::Lua::LuaLib;
using Shared::Lua::RegType;

/// Handle interactions with Bots.
// @module bot

namespace Lua {
namespace {
#define GET_FUNC( var, func )                                \
	static int Get##var( lua_State* L )                      \
	{                                                        \
		Bot* c = LuaLib<Bot>::check( L, 1 );                 \
		if ( !c || !c->ent || !c->ent->botMind )             \
		{                                                    \
			Log::Warn( "trying to access stale bot info!" ); \
			return 0;                                        \
		}                                                    \
		func;                                                \
		return 1;                                            \
	}

/// Bot skill level. From 1-9. Higher skill levels are better.
// @tfield integer skill Read/Write.
// @within Bot
GET_FUNC( skill, lua_pushinteger( L, c->ent->botMind->skillLevel ) )

/// Name of the current behavior tree.
// @tfield string behavior Read only.
// @within Bot
// @see set_behavior
static int Getbehavior( lua_State* L )
{
	Bot* c = LuaLib<Bot>::check( L, 1 );
	if ( !c || !c->ent || !c->ent->botMind )
	{
		Log::Warn( "trying to access stale bot info!" );
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
	if ( !c || !c->ent || !c->ent->botMind )
	{
		Log::Warn( "trying to access stale bot info!" );
		return 0;
	}
	const char* behavior = luaL_checkstring( L, 1 );
	if ( !behavior )
	{
		Log::Warn( "empty behavior" );
		return 0;
	}
	G_BotChangeBehavior( c->ent->num(), behavior );
	return 0;
}

static int Setskill( lua_State* L )
{
	Bot* c = LuaLib<Bot>::check( L, 1 );
	if ( !c || !c->ent || !c->ent->botMind )
	{
		Log::Warn( "trying to access stale bot info!" );
		return 0;
	}
	int skill = luaL_checkinteger( L, 1 );
	G_BotSetSkill( c->ent->client->num(), skill );
	return 0;
}

}  // namespace

RegType<Bot> BotMethods[] = {
	{ "set_behavior", MethodSetBehavior },

	{ nullptr, nullptr },
};

#define GETTER( name )   \
	{                    \
		#name, Get##name \
	}

luaL_Reg BotGetters[] = {
	GETTER( skill ),
	GETTER( behavior ),

	{ nullptr, nullptr },
};

#define SETTER( name )   \
	{                    \
		#name, Set##name \
	}

luaL_Reg BotSetters[] = {
	SETTER( skill ),

	{ nullptr, nullptr },
};

}  // namespace Lua

namespace Shared {
namespace Lua {
LUACORETYPEDEFINE( ::Lua::Bot )

template <>
void ExtraInit<::Lua::Bot>( lua_State* /*L*/, int /*metatable_index*/ )
{}
}  // namespace Lua
}  // namespace Shared
