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
#include "engine/qcommon/q_shared.h"

#include "shared/lua/LuaLib.h"
#include "shared/lua/Utils.h"


namespace Shared {
namespace Lua {

template<typename T>
void LuaLib<T>::Register( lua_State* L )
{
	//for annotations, starting at 1, but it is a relative value, not always 1
	lua_newtable( L ); //[1] = table
	int methods = lua_gettop( L ); //methods = 1

	luaL_newmetatable( L, GetTClassName<T>() ); //[2] = metatable named <ClassName>, referred in here by ClassMT
	int metatable = lua_gettop( L ); //metatable = 2

	//store method table in globals so that scripts can add functions written in Lua
	lua_pushvalue( L, methods ); //[methods = 1] -> [3] = copy (reference) of methods table
	lua_setglobal( L, GetTClassName<T>() ); // -> <ClassName> = [3 = 1], pop top [3]

	//hide metatable from Lua getmetatable()
	lua_pushvalue( L, methods ); //[methods = 1] -> [3] = copy of methods table, including modifications above
	lua_setfield( L, metatable, "__metatable" ); //[metatable = 2] -> t[k] = v; t = [2 = ClassMT], k = "__metatable", v = [3 = 1]; pop [3]

	lua_pushcfunction( L, index ); //index = cfunction -> [3] = cfunction
	lua_setfield( L, metatable, "__index" ); //[metatable = 2] -> t[k] = v; t = [2], k = "__index", v = cfunc; pop [3]

	lua_pushcfunction( L, newindex );
	lua_setfield( L, metatable, "__newindex" );

	lua_pushcfunction( L, tostring_T );
	lua_setfield( L, metatable, "__tostring" );

	ExtraInit<T>( L, metatable ); //optionally implemented by individual types

	lua_newtable( L ); //for method table -> [3] = this table
	lua_setmetatable( L, methods ); //[methods = 1] -> metatable for [1] is [3]; [3] is popped off, top = [2]

	_regfunctions( L, metatable, methods );

	lua_pop( L, 2 ); //remove the two items from the stack, [1 = methods] and [2 = metatable]
}


template<typename T>
int LuaLib<T>::push( lua_State* L, T* obj )
{
	//for annotations, starting at index 1, but it is a relative number, not always 1
	if ( !obj )
	{
		lua_pushnil( L );
		return lua_gettop( L );
	}

	luaL_getmetatable( L, GetTClassName<T>() ); // lookup metatable in Lua registry ->[1] = metatable of <ClassName>

	if ( lua_isnil( L, -1 ) ) luaL_error( L, "%s missing metatable", GetTClassName<T>() );

	int mt = lua_gettop( L ); //mt = 1
	T** ptrHold = ( T** )lua_newuserdata( L, sizeof( T** ) ); //->[2] = empty userdata
	int ud = lua_gettop( L ); //ud = 2

	if ( ptrHold != nullptr )
	{
		*ptrHold = obj;
		lua_pushvalue( L, mt ); // ->[3] = copy of [1]
		lua_setmetatable( L, -2 ); //[-2 = 2] -> [2]'s metatable = [3]; pop [3]
	}

	lua_settop( L, ud ); //[ud = 2] -> remove everything that is above 2, top = [2]
	lua_replace( L, mt ); //[mt = 1] -> move [2] to pos [1], and pop previous [1]
	lua_settop( L, mt ); //remove everything above [1]
	return mt;  // index of userdata containing pointer to T object
}


template<typename T>
T* LuaLib<T>::check( lua_State* L, int narg )
{
	T** ptrHold = static_cast<T**>( lua_touserdata( L, narg ) );

	if ( ptrHold == nullptr )
	{
		Sys::Drop( "null value when checking %s", GetTClassName<T>() );
	}

	return ( *ptrHold );
}


//private members

template<typename T>
int LuaLib<T>::thunk( lua_State* L )
{
	// stack has userdata, followed by method args
	T* obj = check( L, 1 ); // get 'self', or if you prefer, 'this'
	lua_remove( L, 1 ); // remove self so member function args start at index 1
	// get member function from upvalue
	RegType* l = static_cast<RegType*>( lua_touserdata( L, lua_upvalueindex( 1 ) ) );

	//at the moment, there isn't a case where nullptr is acceptable to be used in the function, so check
	//for it here, rather than individually for each function
	if ( obj == nullptr )
	{
		lua_pushnil( L );
		return 1;
	}
	else
		return l->func( L, obj ); // call member function
}



template<typename T>
void LuaLib<T>::tostring( char* buff, size_t buff_size, void* obj )
{
	snprintf(buff, buff_size, "%p", obj);
}


template<typename T>
int LuaLib<T>::tostring_T( lua_State* L )
{
	char buff[32];
	T** ptrHold = ( T** )lua_touserdata( L, 1 );
	T* obj = *ptrHold;
	Com_sprintf( buff, sizeof( buff ), "%p", obj );
	lua_pushfstring( L, "%s (%s)", GetTClassName<T>(), buff );
	return 1;
}



template<typename T>
int LuaLib<T>::index( lua_State* L )
{
	/*the table obj and the missing key are currently on the stack(index 1 & 2) as defined by the Lua language*/
	lua_getglobal( L, GetTClassName<T>() ); //stack pos [3] (fairly important, just refered to as [3])
	// string form of the key.
	const char* key = luaL_checkstring( L, 2 );

	if ( lua_istable( L, -1 ) ) //[-1 = 3]
	{
		lua_pushvalue( L, 2 ); //[2] = key, [4] = copy of key
		lua_rawget( L, -2 ); //[-2 = 3] -> pop top and push the return value to top [4]

		//If the key were looking for is not in the table, retrieve its' metatables' index value.
		if ( lua_isnil( L, -1 ) ) //[-1 = 4] is value from rawget above
		{
			//try __getters
			lua_pop( L, 1 ); //remove top item (nil) from the stack
			lua_pushstring( L, "__getters" );
			lua_rawget( L, -2 ); //[-2 = 3], <ClassName>._getters -> result to [4]
			lua_pushvalue( L, 2 ); //[2 = key] -> copy to [5]
			lua_rawget( L, -2 ); //[-2 = __getters] -> __getters[key], result to [5]

			if ( lua_type( L, -1 ) == LUA_TFUNCTION ) //[-1 = 5]
			{
				lua_pushvalue( L, 1 ); //push the userdata to the stack [6]

				if ( lua_pcall( L, 1, 1, 0 ) != 0 ) //remove one, result is at [6]
					Report( L, std::string( GetTClassName<T>() ).append( ".__index for " ).append( lua_tostring( L, 2 ) ).append( ": " ) );
			}
			else
			{
				lua_settop( L, 4 ); //forget everything we did above
				lua_getmetatable( L, -2 ); //[-2 = 3] -> metatable from <ClassName> to top [5]

				if ( lua_istable( L, -1 ) ) //[-1 = 5] = the result of the above
				{
					lua_getfield( L, -1, "__index" ); //[-1 = 5] = check the __index metamethod for the metatable-> push result to [6]

					if ( lua_isfunction( L, -1 ) ) //[-1 = 6] = __index metamethod
					{
						lua_pushvalue( L, 1 ); //[1] = object -> [7] = object
						lua_pushvalue( L, 2 ); //[2] = key -> [8] = key

						if ( lua_pcall( L, 2, 1, 0 ) != 0 ) //call function at top of stack (__index) -> pop top 2 as args; [7] = return value
							Report( L, std::string( GetTClassName<T>() ).append( ".__index for " ).append( lua_tostring( L, 2 ) ).append( ": " ) );
					}
					else if ( lua_istable( L, -1 ) )
						lua_getfield( L, -1, key ); //shorthand version of above -> [7] = return value
					else
						lua_pushnil( L ); //[7] = nil
				}
				else
					lua_pushnil( L ); //[6] = nil
			}
		}
		else if ( lua_istable( L, -1 ) ) //[-1 = 4] is value from rawget [3]
		{
			lua_pushvalue( L, 2 ); //[2] = key, [5] = key
			lua_rawget( L, -2 ); //[-2 = 3] = table of <ClassName> -> pop top and push the return value to top [5]
		}
	}
	else
		lua_pushnil( L ); //[4] = nil

	lua_insert( L, 1 ); //top element to position 1 -> [1] = top element as calculated in the earlier rest of the function
	lua_settop( L, 1 ); // -> [1 = -1], removes the other elements
	return 1;
}



template<typename T>
int LuaLib<T>::newindex( lua_State* L )
{
	//[1] = obj, [2] = key, [3] = value
	//look for it in __setters
	lua_getglobal( L, GetTClassName<T>() ); //[4] = this table
	lua_pushstring( L, "__setters" ); //[5]
	lua_rawget( L, -2 ); //[-2 = 4] -> <ClassName>.__setters to [5]
	lua_pushvalue( L, 2 ); //[2 = key] -> [6] = copy of key
	lua_rawget( L, -2 ); //[-2 = __setters] -> __setters[key] to [6]

	if ( lua_type( L, -1 ) == LUA_TFUNCTION )
	{
		lua_pushvalue( L, 1 ); //userdata at [7]
		lua_pushvalue( L, 3 ); //[8] = copy of [3]

		if ( lua_pcall( L, 2, 0, 0 ) != 0 ) //call function, pop 2 off push 0 on
			Report( L, std::string( GetTClassName<T>() ).append( ".__newindex for " ).append( lua_tostring( L, 2 ) ).append( ": " ) );
	}
	else
		lua_pop( L, 1 ); //not a setter function.

	lua_pop( L, 2 ); //pop __setters and the <Classname> table
	return 0;
}


template<typename T>
void LuaLib<T>::_regfunctions(lua_State* L, int /*meta*/, int methods)
{
	//fill method table with methods.
	for ( RegType* m = ( RegType* )GetMethodTable<T>(); m->name; m++ )
	{
		lua_pushstring( L, m->name ); // ->[1] = name of function Lua side
		lua_pushlightuserdata( L, ( void* )m ); // ->[2] = pointer to the object containing the name and the function pointer as light userdata
		lua_pushcclosure( L, thunk, 1 ); //thunk = function pointer -> pop 1 item from stack, [2] = closure
		lua_settable( L, methods ); // represents t[k] = v, t = [methods] -> pop [2 = closure] to be v, pop [1 = name] to be k
	}


	lua_getfield( L, methods, "__getters" ); // -> table[1]

	if ( lua_isnoneornil( L, -1 ) )
	{
		lua_pop( L, 1 ); //pop unsuccessful get
		lua_newtable( L ); // -> table [1]
		lua_setfield( L, methods, "__getters" ); // pop [1]
		lua_getfield( L, methods, "__getters" ); // -> table [1]
	}

	for ( luaL_Reg* m = ( luaL_Reg* )GetAttrTable<T>(); m->name; m++ )
	{
		lua_pushcfunction( L, m->func ); // -> [2] is this function
		lua_setfield( L, -2, m->name ); //[-2 = 1] -> __getters.name = function
	}

	lua_pop( L, 1 ); //pop __getters


	lua_getfield( L, methods, "__setters" ); // -> table[1]

	if ( lua_isnoneornil( L, -1 ) )
	{
		lua_pop( L, 1 ); //pop unsuccessful get
		lua_newtable( L ); // -> table [1]
		lua_setfield( L, methods, "__setters" ); // pop [1]
		lua_getfield( L, methods, "__setters" ); // -> table [1]
	}

	for ( luaL_Reg* m = ( luaL_Reg* )SetAttrTable<T>(); m->name; m++ )
	{
		lua_pushcfunction( L, m->func ); // -> [2] is this function
		lua_setfield( L, -2, m->name ); //[-2 = 1] -> __setters.name = function
	}

	lua_pop( L, 1 ); //pop __setters
}

}  // namespace Lua
}  // naemspace Shared
