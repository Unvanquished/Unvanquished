/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#include "../qcommon/qcommon.h"

#ifndef FRAMEWORK_SYSTEM_H_
#define FRAMEWORK_SYSTEM_H_

// Low-level system functions
namespace Sys {

// Cleanly exit the engine, shutting down all subsystems.
NORETURN void Quit();

// Class representing a loadable .dll/.so
class DynamicLib {
public:
	DynamicLib()
		: handle(nullptr) {}

	// DynamicLibs are noncopyable but movable
	DynamicLib(const DynamicLib&) = delete;
	DynamicLib& operator=(const DynamicLib&) = delete;
	DynamicLib(DynamicLib&& other)
		: handle(other.handle)
	{
		other.handle = nullptr;
	}
	DynamicLib& operator=(DynamicLib&& other)
	{
		std::swap(handle, other.handle);
		return *this;
	}

	// Check if a DynamicLib is open
	explicit operator bool() const
	{
		return handle != nullptr;
	}

	// Close a DynamicLib
	void Close();
	~DynamicLib()
	{
		Close();
	}

	// Load a DynamicLib, returns an empty DynamicLib on error
	static DynamicLib Load(Str::StringRef filename, std::string& errorString);

	// Load a symbol from the DynamicLib
	template<typename T> T LoadSym(Str::StringRef sym, std::string& errorString)
	{
		return reinterpret_cast<T>(reinterpret_cast<intptr_t>(InternalLoadSym(sym, errorString)));
	}

private:
	void* InternalLoadSym(Str::StringRef sym, std::string& errorString);

	// OS-specific handle
	void* handle;
};

} // namespace Sys

#endif // FRAMEWORK_SYSTEM_H_
