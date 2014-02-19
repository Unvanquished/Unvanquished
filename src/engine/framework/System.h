/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Unvanquished Developers

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

===========================================================================
*/

#include <chrono>
#include <string>
#include "../../common/String.h"

// Low-level system functions
namespace Sys {

// The Windows implementation of steady_clock is really bad, use own own
#ifdef _WIN32
class SteadyClock {
public:
	typedef std::chrono::nanoseconds duration;
	typedef duration::rep rep;
	typedef duration::period period;
	typedef std::chrono::time_point<SteadyClock> time_point;
	static const bool is_steady = true;

	static time_point now() NOEXCEPT;
};
#else
typedef std::chrono::steady_clock SteadyClock;
#endif
typedef std::chrono::system_clock RealClock;

// High precision sleep until the specified time point. Use this instead of
// std::this_thread::sleep_until. The function returns the time which should be
// used as a base to calculate the next frame's time. This will normally be the
// requested time, unless the target time is already past, in which case the
// current time is returned.
SteadyClock::time_point SleepUntil(SteadyClock::time_point time);

// Cleanly exit the engine, shutting down all subsystems.
NORETURN void Quit();

// Exit with a fatal error. Only critical subsystems are shut down cleanly, and
// an error message is displayed to the user.
NORETURN void Error(Str::StringRef errorMessage);

// Throw a DropErr with the given message, which normally will drop to the main menu
class DropErr: public std::runtime_error {
	DropErr(const char* message)
		: std::runtime_error(message) {}
	DropErr(const std::string& message)
		: std::runtime_error(message) {}
};
NORETURN void Drop(Str::StringRef errorMessage);

// Variadic wrappers for Error and Drop
template<typename ... Args> void Error(Str::StringRef format, Args&& ... args)
{
	Error(Str::Format(format, std::forward<Args>(args)...));
}
template<typename ... Args> void Drop(Str::StringRef format, Args&& ... args)
{
	Drop(Str::Format(format, std::forward<Args>(args)...));
}

#ifdef _WIN32
// strerror() equivalent for Win32 API error values, as returned by GetLastError
std::string Win32StrError(uint32_t error);
#endif

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
