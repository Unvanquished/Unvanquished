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
#ifndef SHARED_LUA_LUALIB_H_
#define SHARED_LUA_LUALIB_H_

#include "common/Common.h"
#include "shared/bg_lua.h"
//As an example, if you used this macro like
//LUAMETHOD(Unit,GetId)
//it would result in code that looks like
//{ "GetId", UnitGetId },
//Which would force you to create a global function named UnitGetId in C with the correct function signature, usually int(*)(lua_State*,type*);
#ifndef LUAMETHOD
#define LUAMETHOD(type,name) { #name, type##name },
#endif
//See above, but the method must match the function signature int(*)(lua_State*) and as example:
//LUAGETTER(Unit,Id) would mean you need a function named UnitGetAttrId
//The first stack position will be the userdata
#ifndef LUAGETTER
#define LUAGETTER(type,varname) { #varname, type##GetAttr##varname },
#endif

//Same method signature as above, but as example:
//LUASETTER(Unit,Id) would mean you need a function named UnitSetAttrId
//The first stack position will be the userdata, and the second will be value on the other side of the equal sign
#ifndef LUASETTER
#define LUASETTER(type,varname) { #varname, type##SetAttr##varname },
#endif

#ifndef CHECK_BOOL
#define CHECK_BOOL(L,narg) (lua_toboolean((L),(narg)) > 0 ? true : false )
#endif

#ifdef LUACHECKOBJ
#define LUACHECKOBJ(obj) if((obj) == NULL) { lua_pushnil(L); return 1; }
#endif
 /** Used to remove repetitive typing at the cost of flexibility. When you use this, you @em must have
 functions with the same name as defined in the macro. For example, if you used @c Element as type, you would
 have to have functions named @c ElementMethods, @c ElementGetters, @c ElementSetters that return the appropriate
 types. */
#define LUACORETYPEDEFINE(type) \
    template<> const char* GetTClassName<type>() { return #type; } \
    template<> RegType<type>* GetMethodTable<type>() { return type##Methods; } \
    template<> luaL_Reg* GetAttrTable<type>() { return type##Getters; } \
    template<> luaL_Reg* SetAttrTable<type>() { return type##Setters; }

/** Used to remove repetitive typing at the cost of flexibility. It creates function prototypes for
getting the name of the type, method tables, and if it is reference counted.
When you use this, you either must also use
the LUACORETYPEDEFINE macro, or make sure that the function signatures are @em exact. */
#define LUACORETYPEDECLARE(type) \
    template<> const char* GetTClassName<type>(); \
    template<> RegType<type>* GetMethodTable<type>(); \
    template<> luaL_Reg* GetAttrTable<type>(); \
    template<> luaL_Reg* SetAttrTable<type>();

namespace Shared {
namespace Lua {

//replacement for luaL_Reg that uses a different function pointer signature, but similar syntax
template<typename T>
struct RegType
{
    const char* name;
    int (*ftnptr)(lua_State*,T*);
};

/** For all of the methods available from Lua that call to the C functions. */
template<typename T> RegType<T>* GetMethodTable();
/** For all of the function that 'get' an attribute/property */
template<typename T> luaL_Reg* GetAttrTable();
/** For all of the functions that 'set' an attribute/property  */
template<typename T> luaL_Reg* SetAttrTable();
/** String representation of the class */
template<typename T> const char* GetTClassName();
/** gets called from the LuaLib<T>::Register function, right before @c _regfunctions.
If you want to inherit from another class, in the function you would want
to call @c _regfunctions<superclass>, where method is metatable_index - 1. Anything
that has the same name in the subclass will be overwrite whatever had the
same name in the superclass.    */
template<typename T> void ExtraInit(lua_State* L, int metatable_index);

/**
    This is mostly the definition of the Lua userdata that C++ gives to the user, plus
    some helper functions.

    @author Nathan Starkey
*/
template<typename T>
class LuaLib
{
public:
    typedef int (*ftnptr)(lua_State* L, T* ptr);
    /** replacement for luaL_Reg that uses a different function pointer signature, but similar syntax */
    typedef struct { const char* name; ftnptr func; } RegType;

    /** Registers the type T as a type in the Lua global namespace by creating a
     metatable with the same name as the class, setting the metatmethods, and adding the
     functions from _regfunctions */
    static inline void Register(lua_State *L);
    /** Pushes on to the Lua stack a userdata representing a pointer of T
    @param obj[in] The object to push to the stack
    metamethod being called from the object in Lua
    @return Position on the stack where the userdata resides   */
    static inline int push(lua_State *L, T* obj);
    /** Statically casts the item at the position on the Lua stack
    @param narg[in] Position of the item to cast on the Lua stack
    @return A pointer to an object of type T or @c NULL   */
    static inline T* check(lua_State* L, int narg);

    /** For calling a C closure with upvalues. Used by the functions defined by RegType
    @return The value that RegType.func returns   */
    static inline int thunk(lua_State* L);
    /** String representation of the pointer. Called by the __tostring metamethod  */
    static inline void tostring(char* buff, size_t buff_size, void* obj);
    //these are metamethods
    /** The __gc metamethod. If the object was pushed by push(lua_State*,T*) with the third
    argument as true, it will call delete on the object. If the third argument to push was false,
    then this does nothing.
    @return 0, since it pushes nothing on to the stack*/
    static inline int gc_T(lua_State* L);
    /** The __tostring metamethod.
    @return 1, because it pushes a string representation of the userdata on to the stack  */
    static inline int tostring_T(lua_State* L);
    /** The __index metamethod. Called whenever the user attempts to access a variable that is
    not in the immediate table. This handles the method calls and calls tofunctions in __getters    */
    static inline int index(lua_State* L);
    /** The __newindex metamethod. Called when the user attempts to set a variable that is not
    int the immediate table. This handles the calls to functions in __setters  */
    static inline int newindex(lua_State* L);

    /** Registers methods,getters,and setters to the type. In Lua, the methods exist in the Type name's
    metatable, and the getters exist in __getters and setters in __setters. The reason for __getters and __setters
    is to have the objects use a 'dot' syntax for properties and a 'colon' syntax for methods.*/
    static inline void _regfunctions(lua_State* L, int meta, int method);
private:
    LuaLib(); //hide constructor

};

}  // namespace Lua
}  // namespace Shared

#include "LuaLib.inl"

#endif  // SHARED_LUA_LUALIB_H_
