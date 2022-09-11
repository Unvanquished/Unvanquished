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

#include "sgame/lua/Client.h"
#include "shared/lua/LuaLib.h"
#include "sgame/sg_local.h"
#include "sgame/Entities.h"
#include "sgame/CBSE.h"
#include "shared/CommonProxies.h"

using Unv::Shared::Lua::LuaLib;
using Unv::Shared::Lua::RegType;

/// Handle interactions with Players.
// @module client

namespace Unv {
namespace SGame {
namespace Lua {

#define GET_FUNC( var, func) \
static int Get##var( lua_State* L ) \
{ \
    Client* c = LuaLib<Client>::check(L, 1); \
    if (!c || !c->ent || !c->ent->client) \
    { \
        Log::Warn("trying to access stale client info!"); \
        return 0; \
    } \
	func; \
	return 1; \
}
/// Whether a client is fully connected (vs connecting to the server and loading the map...).
// @tfield bool connected Read only.
// @within Client
GET_FUNC( connected, lua_pushboolean(L, c->ent->client->pers.connected == clientConnected_t::CON_CONNECTED) )
/// The client's full unescaped name.
// @tfield string name Read/Write.
// @within Client
GET_FUNC( name, lua_pushstring(L, c->ent->client->pers.netname ) )
/// The client's credits. Only writable for humans.
// @tfield integer credits Read/Write.
// @within Client
GET_FUNC( credits, lua_pushinteger(L, c->ent->client->pers.credit ) )
/// The client's evos. Only writable for aliens.
// @tfield number evos Read/Write.
// @within Client
GET_FUNC( evos, lua_pushnumber(L, c->ent->client->pers.credit / static_cast<double>(CREDITS_PER_EVO) ) )
/// The client's weapon.
// @tfield string evos Read only.
// @within Client
GET_FUNC( weapon, lua_pushstring(L, BG_Weapon(c->ent->client->ps.stats[STAT_WEAPON] )->name ) )
/// The client's health.
// @tfield number health Read/Write.
// @within Client
GET_FUNC( health, lua_pushnumber(L, Entities::HasHealthComponent(c->ent.get()) ? Entities::HealthOf(c->ent.get()) : 0 ) )
/// The client's ping.
// @tfield integer ping Read only.
// @within Client
GET_FUNC( ping, lua_pushinteger(L, c->ent->client->ps.ping ) )
/// The client's class.
// @tfield string class Read only.
// @within Client
GET_FUNC( class, lua_pushstring(L, BG_Class(c->ent->client->ps.stats[STAT_CLASS] )->name ) )
/// The client's stamina. Only applicable for humans.
// @tfield integer stamina Read/Write.
// @within Client
GET_FUNC( stamina, lua_pushinteger(L, c->ent->client->ps.stats[STAT_STAMINA] ) )
/// The client's score.
// @tfield integer score Read only.
// @within Client
GET_FUNC( score, lua_pushinteger(L, c->ent->client->pers.namelog->score ) )
/// Make the client invulnerable. The client cannot take any damage.
// @tfield bool god Read/Write.
// @within Client
GET_FUNC( god, lua_pushboolean(L, c->ent->flags & FL_GODMODE ) )
/// Make the client untargetable by buildables.
// @tfield bool notarget Read/Write.
// @within Client
GET_FUNC( notarget, lua_pushboolean(L, c->ent->flags & FL_NOTARGET ) )

/// The client's name without color codes or other escapes or emoticon. Useful for programatic comparisons.
// @tfield string clean_name Read only.
// @within Client
static int Getclean_name( lua_State* L )
{
    Client* c = LuaLib<Client>::check(L, 1);
    if (!c || !c->ent || !c->ent->client)
    {
        Log::Warn("trying to access stale client info!");
        return 0;
    }
	char cleanName[MAX_NAME_LENGTH] = {0};
    G_SanitiseString(c->ent->client->pers.netname, cleanName, sizeof(cleanName));
    lua_pushstring(L, cleanName);
	return 1;
}

/// Kill the client instantly.
// @function kill
// @usage client:kill()
// @within Client
int Methodkill(lua_State* L, Client* c)
{
    if (!c || !c->ent || !c->ent->client)
    {
        Log::Warn("trying to access stale client info!");
        return 0;
    }
    HealthComponent* health = c->ent->entity->Get<HealthComponent>();
    if (!health || !health->Alive()) return 0;
    Entities::Kill(c->ent.get(), MOD_SUICIDE);
    return 0;
}

/// Teleport the client to a location.
// @function teleport
// @tparam array location An array of floats starting at index 1.
// @usage client:teleport({3892, 2, 48})
// @within Client
int Methodteleport(lua_State* L, Client* c)
{
    if (!c || !c->ent || !c->ent->client)
    {
        Log::Warn("trying to access stale client info!");
        return 0;
    }
    vec3_t origin = {};
    vec3_t angles = {0, 0, 0};
    if (!Shared::Lua::CheckVec3(L, 1, origin))
    {
        return 0;
    }
    origin[ 2 ] -= c->ent->client->ps.viewheight;
    G_TeleportPlayer(c->ent.get(), origin, angles, 0);
    return 0;
}

/// Execute a game command as if the client had sent it themself.
// @function cmd
// @tparam string cmd The command string.
// @usage client:cmd('buy rifle')
// @within Client
int Methodcommand(lua_State* L, Client* c)
{
    if (!c || !c->ent || !c->ent->client)
    {
        Log::Warn("trying to access stale client info!");
        return 0;
    }
    const char* cmd = luaL_checkstring(L, 1);
    Cmd::PushArgs(cmd);
    ClientCommand(c->ent.get() - g_entities);
    Cmd::PopArgs();
    return 0;
}

RegType<Client> ClientMethods[] =
{
    { "kill", Methodkill },
    { "teleport", Methodteleport },
    { "cmd", Methodcommand },
    { nullptr, nullptr },
};
#define GETTER(name) { #name, Get##name }
luaL_Reg ClientGetters[] =
{
    GETTER( connected ),
    GETTER( name ),
    GETTER( clean_name ),
    GETTER( credits ),
    GETTER( evos ),
    GETTER( weapon ),
    GETTER( health ),
    GETTER( ping ),
    GETTER( class ),
    GETTER( stamina ),
    GETTER( score ),
    GETTER( god ),
    GETTER( notarget ),
	{ nullptr, nullptr },
};

int Setname(lua_State* L)
{
    Client* c = LuaLib<Client>::check(L, 1);
    if (!c) return 0;
    if (!lua_isstring(L, 2))
    {
        Log::Warn("Lua client.setname expected string.");
    }
    const char* newName = luaL_checkstring(L, 2);
    if (!newName) return 0;
    char err[ MAX_STRING_CHARS ] = {};
    if (!G_admin_name_check(c->ent.get(), newName, err, sizeof(err)))
    {
        Log::Warn("error changing name: %s", err);
        return 0;
    }
    char userinfo[ MAX_INFO_STRING ];
    int pid = c->ent->client - level.clients;
    trap_GetUserinfo(pid, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "name", newName, false);
	trap_SetUserinfo(pid, userinfo);
	ClientUserinfoChanged(pid, true);
    return 0;
}

int Setcredits(lua_State* L)
{
    Client* c = LuaLib<Client>::check(L, 1);
    if (!c) return 0;
    if (c->ent->client->pers.team != TEAM_HUMANS)
    {
        Log::Warn("Cannot add credits to non-human team member.");
    }
    if (!lua_isnumber(L, 2))
    {
        Log::Warn("Lua client.setcredits expected number.");
    }
    c->ent->client->pers.credit = luaL_checknumber(L, 2);

    return 0;
}

int Setevos(lua_State* L)
{
    Client* c = LuaLib<Client>::check(L, 1);
    if (!c) return 0;
    if (c->ent->client->pers.team != TEAM_ALIENS)
    {
        Log::Warn("Cannot add evos to non-alien team member.");
    }
    if (!lua_isnumber(L, 2))
    {
        Log::Warn("Lua client.setevos expected number.");
    }
    c->ent->client->pers.credit = luaL_checknumber(L, 2) * CREDITS_PER_EVO;

    return 0;
}

int Sethealth(lua_State* L)
{
    Client* c = LuaLib<Client>::check(L, 1);
    if (!c) return 0;
    if (!lua_isnumber(L, 2))
    {
        Log::Warn("Lua client.sethealth expected number.");
    }

    HealthComponent* health = c->ent->entity->Get<HealthComponent>();
    if (!health || !health->Alive()) return 0;
    health->SetHealth(luaL_checknumber(L, 2));
    return 0;
}

int Setstamina(lua_State* L)
{
    Client* c = LuaLib<Client>::check(L, 1);
    if (!c) return 0;
    if (c->ent->client->pers.team != TEAM_HUMANS)
    {
        Log::Warn("Cannot add stamina to non-human team member.");
    }
    if (!lua_isnumber(L, 2))
    {
        Log::Warn("Lua client.setstamina expected number.");
    }

    c->ent->client->ps.stats[STAT_STAMINA] = luaL_checkinteger(L, 2);
    return 0;
}

static int Setgod(lua_State* L)
{
    Client* c = LuaLib<Client>::check(L, 1);
    if (!c) return 0;
    bool god = lua_toboolean(L, 2);
    if (god)
    {
        c->ent->flags |= FL_GODMODE;
    }
    else
    {
        c->ent->flags &= ~FL_GODMODE;
    }
    return 0;
}

int Setnotarget(lua_State* L)
{
    Client* c = LuaLib<Client>::check(L, 1);
    if (!c) return 0;
    bool notarget = lua_toboolean(L, 2);
    if (notarget)
    {
        c->ent->flags |= FL_NOTARGET;
    }
    else
    {
        c->ent->flags &= ~FL_NOTARGET;
    }
    return 0;
}

#define SETTER(name) { #name, Set##name }
luaL_Reg ClientSetters[] =
{
    SETTER(name),
    SETTER(credits),
    SETTER(evos),
    SETTER(health),
    SETTER(stamina),
    SETTER(god),
    SETTER(notarget),
	{ nullptr, nullptr },
};

} // namespace Lua
} // namespace SGame
} // namespace Unv

namespace Unv {
namespace Shared {
namespace Lua {
LUASGAMETYPEDEFINE(Client, false)
template<>
void ExtraInit<Unv::SGame::Lua::Client>(lua_State* /*L*/, int /*metatable_index*/) {}
}  // namespace Lua
}  // namespace Shared
}  // namespace Unv
