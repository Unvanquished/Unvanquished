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

#include "Entity.h"
#include "EntityProxy.h"
#include "shared/lua/LuaLib.h"
#include "sgame/sg_local.h"

using Unv::Shared::Lua::LuaLib;
using Unv::Shared::Lua::RegType;

namespace Unv {
namespace SGame {
namespace Lua {

std::vector<EntityProxy*> proxies( MAX_GENTITIES );

int Entity::Find( lua_State* L )
{
	const char* name = luaL_checkstring(L, 1);
	gentity_t* ent = nullptr;
	while ((ent = G_IterateEntities(ent))) {
		if (G_MatchesName(ent, name)) {
			int entNum = ent - g_entities;
			if (!proxies[entNum]) {
				proxies[entNum] = new EntityProxy(ent);
			}
			LuaLib<EntityProxy>::push(L, proxies[entNum], false);
			return 1;
		}
	}

	return 0;
}


RegType<Entity> EntityMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg EntityGetters[] =
{
	{ nullptr, nullptr },
};

luaL_Reg EntitySetters[] =
{
	{ nullptr, nullptr },
};

} // namespace Lua
} // namespace SGame
} // namespace Unv

namespace Unv { namespace Shared { namespace Lua {
LUASGAMETYPEDEFINE(Entity, false)
template<>
void ExtraInit<Unv::SGame::Lua::Entity>(lua_State* L, int metatable_index)
{
	lua_pushcfunction( L, Unv::SGame::Lua::Entity::Find);
	lua_setfield( L, metatable_index - 1, "find" );
}
} } }
