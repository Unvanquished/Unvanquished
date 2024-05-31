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

#ifndef LUAENTITYPROXY_H_
#define LUAENTITYPROXY_H_

#include <unordered_map>

#include "shared/bg_lua.h"
#include "sgame/lua/Client.h"
#include "sgame/lua/Buildable.h"
#include "sgame/lua/Bot.h"
#include "sgame/sg_local.h"

namespace Unv {
namespace SGame {
namespace Lua {

struct EntityProxy
{
	EntityProxy(gentity_t* ent, lua_State* L);
	gentity_t *ent;

	enum FunctionType
	{
		THINK,
		RESET,
		REACHED,
		BLOCKED,
		TOUCH,
		USE,
		PAIN,
		DIE,
	};

	struct EntityFunction
	{
		FunctionType type;
		int luaRef;
		union {
			// Storage for the original gentity's functions.
			void ( *think )( gentity_t *self );
			void ( *reset )( gentity_t *self );
			void ( *reached )( gentity_t *self );
			void ( *blocked )( gentity_t *self, gentity_t *other );
			void ( *touch )( gentity_t *self, gentity_t *other, trace_t *trace );
			void ( *use )( gentity_t *self, gentity_t *other, gentity_t *activator );
			void ( *pain )( gentity_t *self, gentity_t *attacker, int damage );
			void ( *die )( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod );
		};
	};
	std::unordered_map<FunctionType, EntityFunction, std::hash<int>> funcs;
	std::unique_ptr<Client> client;
	std::unique_ptr<Buildable> buildable;
	std::unique_ptr<Bot> bot;
	lua_State* L;
};

} // namespace Lua
} // namespace SGame
} // namespace Unv

#endif // LUAENTITYPROXY_H_
