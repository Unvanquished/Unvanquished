/*
 * This source file is part of Unvanquished
 *
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */



#include "shared/bg_lua.h"
#include "shared/lua/Utils.h"
#include "shared/lua/LuaLib.h"
#include "common/Command.h"
#include "common/FileSystem.h"
#include "Interpreter.h"

#include "sgame/lua/SGameGlobal.h"

using Unv::Shared::Lua::LuaLib;

namespace Unv {
namespace SGame {
namespace Lua {

static lua_State* L = nullptr;

bool LoadCode(Str::StringRef code, Str::StringRef location)
{
	if (luaL_loadbuffer(L, code.c_str(), code.size(), location.c_str()) != 0)
	{
		Shared::Lua::Report(L, "Loading buffer");
		return false;
	}
	return true;
}

bool RunCode()
{
	if(lua_pcall(L,0,0,0) != 0)
	{
		Shared::Lua::Report(L, "Executing code");
		return false;
	}
	return true;
}

bool LoadScript(Str::StringRef scriptPath)
{
	fileHandle_t f;
	int len = trap_FS_FOpenFile(scriptPath.c_str(), &f, fsMode_t::FS_READ);
	if (len <= 0)
	{
		Log::Warn("error opening %s: %d", scriptPath, len);
		return false;
	}
	std::vector<char> code(len + 1);
	code[len] = '\0';
	int ret = trap_FS_Read(code.data(), len, f);
	if (ret != len)
	{
		Log::Warn("error reading %s: %d", scriptPath, ret);
		return false;
	}
	return LoadCode(code.data(), scriptPath);
}

/*
 * Below here are global functions and their helper functions that help overwrite the Lua global functions. The majority of this file was copied from LibRocket.
 */

// __pairs should return two values.
// upvalue 1 is the __pairs function, upvalue 2 is the userdata created in rocket_pairs
// [1] is the object implementing __pairs
// [2] is the key that was just read
int rocket_pairsaux(lua_State* L)
{
	lua_pushvalue(L,lua_upvalueindex(1)); // push __pairs to top
	lua_insert(L,1); // move __pairs to the bottom
	lua_pushvalue(L,lua_upvalueindex(2)); // push userdata
	// stack looks like [1] = __pairs, [2] = object, [3] = latest key, [4] = userdata
	if(lua_pcall(L,lua_gettop(L)-1,LUA_MULTRET,0) != 0)
 		Unv::Shared::Lua::Report(L,"__pairs");
	return lua_gettop(L);
}

// A version of pairs that respects a __pairs metamethod.
// "next" function is upvalue 1
int rocket_pairs(lua_State* L)
{
	luaL_checkany(L,1); // [1] is the object given to us by pairs(object)
	if(luaL_getmetafield(L,1,"__pairs"))
	{
		void* ud = lua_newuserdata(L,sizeof(void*)); // create a new block of memory to be used as upvalue 1
		(*(int*)(ud)) = -1;
		lua_pushcclosure(L,rocket_pairsaux,2); // uv 1 is __pairs, uv 2 is ud
	}
	else
	{
		lua_pushvalue(L,lua_upvalueindex(1)); // generator
	}
	lua_pushvalue(L,1); // state
	lua_pushnil(L); // initial value
	return 3;
}

// copy + pasted from Lua's lbaselib.c
int ipairsaux (lua_State *L) {
	int i = luaL_checkinteger(L, 2);
	luaL_checktype(L, 1, LUA_TTABLE);
	i++;  /* next value */
	lua_pushinteger(L, i);
	lua_rawgeti(L, 1, i);
	return (lua_isnil(L, -1)) ? 0 : 2;
}

// __ipairs should return two values
// upvalue 1 is the __ipairs function, upvalue 2 is the userdata created in rocket_ipairs
// [1] is the object implementing __ipairs, [2] is the key last used
int rocket_ipairsaux(lua_State* L)
{
	lua_pushvalue(L,lua_upvalueindex(1)); // push __ipairs
	lua_insert(L,1); // move __ipairs to the bottom
	lua_pushvalue(L,lua_upvalueindex(2)); // push userdata
	// stack looks like [1] = __ipairs, [2] = object, [3] = latest key, [4] = userdata
	if(lua_pcall(L,lua_gettop(L)-1,LUA_MULTRET,0) != 0)
 		Unv::Shared::Lua::Report(L,"__ipairs");
	return lua_gettop(L);
}


// A version of paris that respects a __pairs metamethod.
// ipairsaux function is upvalue 1
int rocket_ipairs(lua_State* L)
{
	luaL_checkany(L,1); // [1] is the object given to us by ipairs(object)
	if(luaL_getmetafield(L,1,"__ipairs"))
	{
		void* ud = lua_newuserdata(L,sizeof(void*)); // create a new block of memory to be used as upvalue 1
		(*(int*)(ud)) = -1;
		lua_pushcclosure(L,rocket_pairsaux,2); // uv 1 is __ipairs, uv 2 is ud
	}
	else
	{
		lua_pushvalue(L,lua_upvalueindex(1)); // generator
	}
	lua_pushvalue(L,1); // state
	lua_pushnil(L); // initial value
	return 3;
}


// Based off of the LuaPrint in libRocket, which was based off of
// luaB_print function from Lua's lbaselib.c
static int LuaPrint(lua_State* L)
{
	int n = lua_gettop(L);  /* number of arguments */
	int i;
	lua_getglobal(L, "tostring");
	std::vector<std::string> string_list;
	std::string output = "";
	for (i = 1; i <= n; ++i)
	{
		const char *s;
		lua_pushvalue(L, -1);  /* function to be called */
		lua_pushvalue(L, i);   /* value to print */
		lua_call(L, 1, 1);
		s = lua_tostring(L, -1);  /* get result */
		if (s == NULL)
			return luaL_error(L, "'tostring' must return a string to 'print'");
		if (i > 1)
			output += "\t";
		output += s;
		lua_pop(L, 1);  /* pop result */
	}
	Log::Notice(output);
	return 0;
}

static int LoadUnvModule(lua_State* L)
{
	const char* file = luaL_checkstring(L, 1);
	if (!file)
	{
		Log::Warn("invalid call to LoadUnvModule: must pass in a string.");
		return 0;
	}
	if (!LoadScript(file))
	{
		Log::Warn("error loading script %s", file);
		return 0;
	}
	return 1;
}

// Remove searchers that search the filesystems so we can install our own searcher.
// Keep the searcher at index 1 because it just looks for cached modules that have
// already been loaded.
const char INITIALIZE_SEARCHERS[] = R"(
table.remove(package.searchers, 2)
table.remove(package.searchers, 3)
)";

static void OverrideGlobalLuaFunctions()
{
	lua_getglobal(L,"_G");

	lua_getglobal(L,"next");
	lua_pushcclosure(L,rocket_pairs,1);
	lua_setfield(L,-2,"pairs");

	lua_getglobal(L,"next");
	lua_pushcclosure(L,rocket_ipairs,1);
	lua_setfield(L,-2,"ipairs");

	lua_pushcfunction(L,LuaPrint);
	lua_setfield(L,-2,"print");

	LoadCode(INITIALIZE_SEARCHERS, __func__) && RunCode();
	// Add the package loader to the package.loaders table.
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "searchers");

	if (lua_isnil(L, -1))
	{
		Sys::Drop("Can't register searcher: package.loaders table does not exist.");
	}

	lua_pushcfunction(L, LoadUnvModule);
	lua_rawseti(L, -2, 2);
	lua_pop(L, 2);
}

void Initialize()
{
	if (L) return;

	L = luaL_newstate();
	luaL_openlibs(L);
	OverrideGlobalLuaFunctions();
	BG_InitializeLuaConstants(L);
	InitializeSGameGlobal(L);
}


void Shutdown()
{
	lua_close(L);
	L = nullptr;
}

lua_State* State()
{
	return L;
}

class LuaCommand : Cmd::CmdBase
{
public:
	LuaCommand() : Cmd::CmdBase(Cmd::SGAME_VM)
	{
		Cmd::AddCommand("lua", *this, "exec server side lua");
	}
	void Run(const Cmd::Args& args) const
	{
		auto usage = [&]() { PrintUsage(args, "\"[-f] <lua code | filename>\"", "exec server side lua"); };
		if (args.Argc() < 2)
		{
			usage();
			return;
		}
		const std::string& first = args.Argv(1);
		if (first == "-f")
		{
			if (args.Argc() < 3)
			{
				Log::Warn("Must pass in filename when using the '-f' flag.");
				usage();
				return;
			}
			LoadScript(args.Argv(2)) && RunCode();
			return;
		}
		LoadCode(first, "/lua") && RunCode();
	}
};

static LuaCommand luaCommand;


} // namespace Lua
} // namespace SGame
} // namespace Unv
