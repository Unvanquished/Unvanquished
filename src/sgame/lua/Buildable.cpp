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

#include "sgame/lua/Buildable.h"
#include "shared/lua/LuaLib.h"
#include "sgame/sg_local.h"
#include "sgame/Entities.h"
#include "sgame/CBSE.h"
#include "sgame/lua/Entity.h"
#include "sgame/lua/EntityProxy.h"

using Unv::Shared::Lua::LuaLib;
using Unv::Shared::Lua::RegType;

/// Handle interactions with Buildables.
// @module buildable

namespace Unv {
namespace SGame {
namespace Lua {

/// Deconstruct this buildable.
// @function decon
// @within Buildable
// @usage buildable:decon()
static int Methoddecon(lua_State* L, Buildable* b)
{
    if (b && b->ent && b->ent->s.eType == entityType_t::ET_BUILDABLE)
    {
        G_DeconstructUnprotected( b->ent, nullptr );
    }
    return 0;
}

RegType<Buildable> BuildableMethods[] =
{
    { "decon", Methoddecon },
    { nullptr, nullptr },
};

#define GET_FUNC( var, func) \
static int Get##var( lua_State* L ) \
{ \
    Buildable* c = LuaLib<Buildable>::check(L, 1); \
    if (!c || !c->ent || c->ent->s.eType != entityType_t::ET_BUILDABLE) \
    { \
        Log::Warn("trying to access stale buildable info!"); \
        return 0; \
    } \
	func; \
	return 1; \
}

static inline BuildableComponent* bc( gentity_t* ent )
{
    BuildableComponent* c = ent->entity->Get<BuildableComponent>();
    if ( !c )
    {
        Sys::Drop( "Expected BuildableComponent for entity %s", etos(ent) );
        return nullptr;
    }
    return c;
}

static inline HealthComponent* hc( gentity_t* ent )
{
    HealthComponent* c = ent->entity->Get<HealthComponent>();
    if ( !c )
    {
        Sys::Drop( "Expected HealthComponent for entity %s", etos(ent) );
        return nullptr;
    }
    return c;
}

/// Get the buildable type.
// @tfield string name Read only.
// @within Buildable
GET_FUNC( name, lua_pushstring(L, BG_Buildable( c->ent->s.modelindex )->name ) )
/// Whether the buildable is powered.
// @tfield boolean powered Read only.
// @within Buildable
GET_FUNC( powered, lua_pushboolean(L, bc(c->ent)->Powered() ) )
/// The entity the buildable is targeting.
// @tfield EntityProxy target Read only.
// @within Buildable
GET_FUNC( target, LuaLib<EntityProxy>::push( L, Entity::CreateProxy( c->ent->target.get(), L ) ) )
/// Buildable health.
// @tfield number health Read/Write.
// @within Buildable
GET_FUNC( health, lua_pushnumber(L, Entities::HasHealthComponent(c->ent) ? Entities::HealthOf( c->ent ) : 0 ) )
/// Buildable team.
// @tfield string team Read only.
// @within Buildable
GET_FUNC( team, lua_pushstring(L, BG_TeamName(c->ent->buildableTeam) ) )
/// Whether this buildable is marked for deconstruction.
// @tfield boolean marked Read only.
// @within Buildable
GET_FUNC( marked, lua_pushboolean(L, bc(c->ent)->MarkedForDeconstruction() ) )
/// The level.time at which this buildable was marked.
// @tfield integer marked_time Read only.
// @within Buildable
GET_FUNC( marked_time, lua_pushinteger(L, bc(c->ent)->GetMarkTime() ) )
/// Make the buildable invulnerable. The buildable cannot take any damage.
// @tfield bool god Read/Write.
// @within Buildable
GET_FUNC( god, lua_pushboolean(L, c->ent->flags & FL_GODMODE ) )

#define GETTER(name) { #name, Get##name }
luaL_Reg BuildableGetters[] =
{
    GETTER( name ),
    GETTER( powered ),
    GETTER( target ),
    GETTER( health ),
    GETTER( team ),
    GETTER( marked ),
    GETTER( marked_time ),
    GETTER( god ),
    { nullptr, nullptr },
};


#define SET_FUNC( var, func) \
static int Set##var( lua_State* L ) \
{ \
    Buildable* c = LuaLib<Buildable>::check(L, 1); \
    if (!c || !c->ent || c->ent->s.eType != entityType_t::ET_BUILDABLE) \
    { \
        Log::Warn("trying to access stale buildable info!"); \
        return 0; \
    } \
	func; \
	return 0; \
}

SET_FUNC( health, hc(c->ent)->SetHealth(luaL_checknumber(L, 2)) )

static int Settarget(lua_State* L)
{
    Buildable* c = LuaLib<Buildable>::check(L, 1);
    if (!c || !c->ent || c->ent->s.eType != entityType_t::ET_BUILDABLE)
    {
        Log::Warn("trying to access stale buildable info!");
        return 0;
    }
    EntityProxy* proxy = LuaLib<EntityProxy>::check(L, 2);
    if (!proxy || !proxy->ent)
    {
        c->ent->target = nullptr;
        return 0;
    }
    c->ent->target = proxy->ent;
	return 0;
}

static int Setmarked(lua_State* L)
{
    Buildable* c = LuaLib<Buildable>::check(L, 1);
    if (!c || !c->ent || c->ent->s.eType != entityType_t::ET_BUILDABLE)
    {
        Log::Warn("trying to access stale buildable info!");
        return 0;
    }
    BuildableComponent* b = bc(c->ent);
    bool mark = lua_toboolean(L, 2);
    if (mark != b->MarkedForDeconstruction())
    {
        b->ToggleDeconstructionMark();
    }
	return 0;
}

static int Setgod(lua_State* L)
{
    Buildable* c = LuaLib<Buildable>::check(L, 1);
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

#define SETTER(name) { #name, Set##name }
luaL_Reg BuildableSetters[] =
{
    SETTER( target ),
    SETTER( health ),
    SETTER( marked ),
    SETTER( god ),
    { nullptr, nullptr },
};

} // namespace Lua
} // namespace SGame
} // namespace Unv

namespace Unv {
namespace Shared {
namespace Lua {
LUASGAMETYPEDEFINE(Buildable, false)
template<>
void ExtraInit<Unv::SGame::Lua::Buildable>(lua_State* /*L*/, int /*metatable_index*/) {}
}  // namespace Lua
}  // namespace Shared
}  // namespace Unv
