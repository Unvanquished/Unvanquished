/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2022 Unv Developers

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

#include "sgame/lua/SGameGlobal.h"

#include "sgame/lua/Entity.h"
#include "sgame/lua/EntityProxy.h"
#include "sgame/sg_local.h"
#include "shared/lua/LuaLib.h"

namespace Unv {
namespace SGame {
namespace Lua {

using Unv::Shared::Lua::LuaLib;
using Unv::Shared::Lua::RegType;

static Entity entity;

class SGameGlobal
{
	public:
	static int GetEntity( lua_State* L )
	{
		LuaLib<Entity>::push( L, &entity, false );
		return 1;
	}
};

SGameGlobal global;

RegType<SGameGlobal> SGameGlobalMethods[] =
{
	{ nullptr, nullptr },
};
luaL_Reg SGameGlobalGetters[] =
{
	{ "entity", SGameGlobal::GetEntity },
	{ nullptr, nullptr },
};
luaL_Reg SGameGlobalSetters[] =
{
	{ nullptr, nullptr },
};

static SGameGlobal sgame;

void InitializeSGameGlobal(lua_State* L)
{
	LuaLib<SGameGlobal>::Register(L);
	LuaLib<Entity>::Register(L);
	LuaLib<EntityProxy>::Register(L);
	LuaLib<SGameGlobal>::push( L, &sgame, false );
	lua_setglobal( L, "sgame" );

}


}  // namespace Lua
}  // namespace SGame
}  // namespace Unv

namespace Unv {
namespace Shared {
namespace Lua {
LUASGAMETYPEDEFINE(SGameGlobal, false)
template<>
void ExtraInit<Unv::SGame::Lua::SGameGlobal>(lua_State* L, int metatable_index) {}
}  // namespace Lua
}  // namespace Shared
}  // namespace Unv
