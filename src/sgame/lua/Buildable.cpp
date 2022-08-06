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

namespace Unv {
namespace SGame {
namespace Lua {

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

GET_FUNC( name, lua_pushstring(L, BG_Buildable( c->ent->s.modelindex )->name ) )
GET_FUNC( powered, lua_pushboolean(L, c->ent->powered ) )
GET_FUNC( target, LuaLib<EntityProxy>::push( L, Entity::CreateProxy( c->ent->target.get(), L ) ) )
GET_FUNC( health, lua_pushnumber(L, Entities::HealthOf( c->ent ) ) )
GET_FUNC( team, lua_pushstring(L, BG_TeamName(c->ent->buildableTeam) ) )

RegType<Buildable> BuildableMethods[] =
{
    { nullptr, nullptr },
};
#define GETTER(name) { #name, Get##name }
luaL_Reg BuildableGetters[] =
{
    GETTER( name ),
    GETTER( powered ),
    GETTER( target ),
    GETTER( health ),
    GETTER( team ),
	{ nullptr, nullptr },
};


#define SETTER(name) { #name, Set##name }
luaL_Reg BuildableSetters[] =
{
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
