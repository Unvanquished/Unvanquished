/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
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

#ifndef COMMON_FILESYSTEM_H_
#define COMMON_FILESYSTEM_H_

#include "Command.h"

#ifdef BUILD_ENGINE
#include "IPC/Channel.h"
#endif

namespace FS {

// File offset type. Using 64bit to allow large files.
using offset_t = int64_t;

// Special value to indicate the function should throw a system_error instead
// of returning an error code. This avoids the need to have 2 overloads for each
// function.
inline std::error_code& throws()
{
	extern std::error_code throw_err;
	return throw_err;
}

// Wrapper around stdio
class File {
public:
	File()
		: fd(nullptr) {}
	explicit File(FILE* fd)
		: fd(fd) {}

	// Files are noncopyable but movable
	File(const File&) = delete;
	File& operator=(const File&) = delete;
	File(File&& other)
		: fd(other.fd)
	{
		other.fd = nullptr;
	}
	File& operator=(File&& other)
	{
		std::swap(fd, other.fd);
		return *this;
	}

	// Check if a file is open
	explicit operator bool() const
	{
		return fd != nullptr;
	}

	// Close the file
	void Close(std::error_code& err = throws());
	~File()
	{
		// Don't throw in destructor
		std::error_code err;
		Close(err);
	}

	// Get the stdio handle of this file
	FILE* GetHandle() const
	{
		return fd;
	}

	// Get the length of the file
	offset_t Length(std::error_code& err = throws()) const;

	// Get the timestamp (time of last modification) of the file
	std::chrono::system_clock::time_point Timestamp(std::error_code& err = throws()) const;

	// File position manipulation
	void SeekCur(offset_t off, std::error_code& err = throws()) const;
	void SeekSet(offset_t off, std::error_code& err = throws()) const;
	void SeekEnd(offset_t off, std::error_code& err = throws()) const;
	offset_t Tell() const;

	// Read/Write from the file
	size_t Read(void* buffer, size_t length, std::error_code& err = throws()) const;
	void Write(const void* data, size_t length, std::error_code& err = throws()) const;
	template<typename ... Args> void Printf(Args&&... args)
	{
		std::string text = Str::Format(std::forward<Args>(args)...);
		Write(text.data(), text.length());
	}

	// Flush buffers
	void Flush(std::error_code& err = throws()) const;

	// Read the entire file into a string
	std::string ReadAll(std::error_code& err = throws()) const;

	// Copy the entire file to another file
	void CopyTo(const File& dest, std::error_code& err = throws()) const;

	// Switch the file between line-buffered (true) and fully-buffered (false)
	void SetLineBuffered(bool enable, std::error_code& err = throws()) const;

private:
	FILE* fd;
};

// Path manipulation functions
namespace Path {

	// Check whether a path is valid. Validation rules:
	// - The path separator is the forward slash (/)
	// - A path must start and end with [0-9][A-Z][a-z]-_+~
	// - A path may contain [0-9][A-Z][a-z]-_+~./
	// - A path may not contain two successive / or . characters
	// Note that paths are validated in all filesystem functions, so manual
	// validation is usually not required.
	// Implicitly disallows the following:
	// - Special files . and ..
	// - Absolute paths starting with /
	bool IsValid(Str::StringRef path, bool allowDir);

	// Build a path from components
	std::string Build(Str::StringRef base, Str::StringRef path);

	// Get the directory portion of a path:
	// a/b/c => a/b/
	// a/b/ => a/
	// a => ""
	std::string DirName(Str::StringRef path);

	// Get the filename portion of a path
	// a/b/c => c
	// a/b/ => b/
	// a => a
	std::string BaseName(Str::StringRef path);

	// Get the extension of a path
	// a.b => .b
	// a => ""
	// a/ => /
	std::string Extension(Str::StringRef path);

	// Get a path without its extension
	// a.b => a
	// a => a
	// a/ => a
	std::string StripExtension(Str::StringRef path);

	// Equivalent to StripExtension(BaseName(path)
	std::string BaseNameStripExtension(Str::StringRef path);

} // namespace Path

// Iterator which iterates over all files in a directory
template<typename State> class DirectoryIterator: public std::iterator<std::input_iterator_tag, std::string> {
	// State interface:
	// bool Advance(std::error_code& err);
	// std::string current;
public:
	DirectoryIterator()
		: state(nullptr) {}
	explicit DirectoryIterator(State* state)
		: state(state) {}

	const std::string& operator*() const
	{
		return state->current;
	}
	const std::string* operator->() const
	{
		return &state->current;
	}

	DirectoryIterator& increment(std::error_code& err = throws())
	{
		if (!state->Advance(err))
			state = nullptr;
		return *this;
	}
	DirectoryIterator& operator++()
	{
		return increment(throws());
	}
	DirectoryIterator operator++(int)
	{
		DirectoryIterator out = *this;
		increment(throws());
		return out;
	}

	friend bool operator==(const DirectoryIterator& a, const DirectoryIterator& b)
	{
		return a.state == b.state;
	}
	friend bool operator!=(const DirectoryIterator& a, const DirectoryIterator& b)
	{
		return a.state != b.state;
	}

private:
	State* state;
};

// Type of pak
enum pakType_t {
	PAK_ZIP, // Zip archive
	PAK_DIR // Directory
};

// Information about a package
struct PakInfo {
	// Base name of the pak, may include directories
	std::string name;

	// Version of the pak
	std::string version;

	// CRC32 checksum of the pak, if given in the pak filename. Note that it
	// may not reflect the actual checksum of the pak. This is only valid for
	// zip paks.
	Util::optional<uint32_t> checksum;

	// Type of pak
	pakType_t type;

	// Full path to the pak
	std::string path;
};

// Information about a package that has been loaded
struct LoadedPakInfo: public PakInfo {
	// Actual CRC32 checksum of the pak, derived from the pak contents. This is
	// only valid for zip paks.
	Util::optional<uint32_t> realChecksum;

	// Time this pak was last updated. This is only valid for zip paks and
	// reflects the timestamp at the time the pak was loaded.
	std::chrono::system_clock::time_point timestamp;

	// File handle of a loaded pak. This is only valid for zip paks. Because the
	// handle is used by multiple threads, you should use pread() to access it,
	// which doesn't use the file position.
	int fd;

	// Prefix used to load this pak. This restricts the files loaded from this
	// pak to only those that start with the prefix.
	std::string pathPrefix;
};

// Operations which work on files that are in packages. Packages should be used
// for read-only assets which can be distributed by auto-download.
namespace PakPath {

	// Load a pak into the namespace with all its dependencies
	void LoadPak(const PakInfo& pak, std::error_code& err = throws());

	// Load a subset of a pak, only loading subpaths starting with the given prefix
	void LoadPakPrefix(const PakInfo& pak, Str::StringRef pathPrefix, std::error_code& err = throws());

	// Load a pak into the namespace and verify its checksum but *don't* load its dependencies
	void LoadPakExplicit(const PakInfo& pak, uint32_t expectedChecksum, std::error_code& err = throws());

#ifndef BUILD_VM
	// Remove all loaded paks
	void ClearPaks();
#endif

	// Get a list of all the loaded paks
	const std::vector<LoadedPakInfo>& GetLoadedPaks();

	// Read an entire file into a string
	std::string ReadFile(Str::StringRef path, std::error_code& err = throws());

	// Copy an entire file to another file
	void CopyFile(Str::StringRef path, const File& dest, std::error_code& err = throws());

	// Check if a file exists
	bool FileExists(Str::StringRef path);

	// Get the pak a file is in, or null if the file does not exist
	const LoadedPakInfo* LocateFile(Str::StringRef path);

	// Get the timestamp of a file
	std::chrono::system_clock::time_point FileTimestamp(Str::StringRef path, std::error_code& err = throws());

	// List all files in the given subdirectory, optionally recursing into subdirectories
	// Note that unlike RawPath/HomePath, directories are *not* returned by ListFiles
	class DirectoryRange;
	DirectoryRange ListFiles(Str::StringRef path, std::error_code& err = throws());
	DirectoryRange ListFilesRecursive(Str::StringRef path, std::error_code& err = throws());

	// Helper function to complete a filename. The root is prepended to the path but not included
	// in the completion results.
	Cmd::CompletionResult CompleteFilename(Str::StringRef prefix, Str::StringRef root, Str::StringRef extension, bool allowSubdirs, bool stripExtension);

	// DirectoryRange implementation
	class DirectoryRange {
	public:
		DirectoryIterator<DirectoryRange> begin()
		{
			return DirectoryIterator<DirectoryRange>(empty() ? nullptr : this);
		}
		DirectoryIterator<DirectoryRange> end()
		{
			return DirectoryIterator<DirectoryRange>();
		}
		bool empty() const
		{
			return iter == iter_end;
		}

	private:
		friend class DirectoryIterator<DirectoryRange>;
		friend DirectoryRange ListFiles(Str::StringRef path, std::error_code& err);
		friend DirectoryRange ListFilesRecursive(Str::StringRef path, std::error_code& err);
		bool Advance(std::error_code& err);
		bool InternalAdvance();
		std::string current;
		std::string prefix;
		std::unordered_map<std::string, std::pair<uint32_t, offset_t>>::iterator iter, iter_end;
		bool recursive;
	};

} // namespace PakPath

#ifndef BUILD_VM
// Operations which work on raw OS paths. Note that no validation on file names is performed
namespace RawPath {

	// Open a file for reading/writing/appending/editing
	File OpenRead(Str::StringRef path, std::error_code& err = throws());
	File OpenWrite(Str::StringRef path, std::error_code& err = throws());
	File OpenAppend(Str::StringRef path, std::error_code& err = throws());
	File OpenEdit(Str::StringRef path, std::error_code& err = throws());

    // Ensure existence of all directories in a path
    void CreatePathTo(Str::StringRef path, std::error_code& err);

	// Check if a file exists
	bool FileExists(Str::StringRef path);

	// Get the timestamp of a file
	std::chrono::system_clock::time_point FileTimestamp(Str::StringRef path, std::error_code& err = throws());

	// Move/delete a file
	void MoveFile(Str::StringRef dest, Str::StringRef src, std::error_code& err = throws());
	void DeleteFile(Str::StringRef path, std::error_code& err = throws());

	// List all files in the given subdirectory, optionally recursing into subdirectories
	// Directory names are returned with a trailing slash to differentiate them from files
	class DirectoryRange;
	class RecursiveDirectoryRange;
	DirectoryRange ListFiles(Str::StringRef path, std::error_code& err = throws());
	RecursiveDirectoryRange ListFilesRecursive(Str::StringRef path, std::error_code& err = throws());

	// DirectoryRange implementations
	class DirectoryRange {
	public:
		DirectoryIterator<DirectoryRange> begin()
		{
			return DirectoryIterator<DirectoryRange>(empty() ? nullptr : this);
		}
		DirectoryIterator<DirectoryRange> end()
		{
			return DirectoryIterator<DirectoryRange>();
		}
		bool empty() const
		{
			return handle == nullptr;
		}

	private:
		friend class DirectoryIterator<DirectoryRange>;
		friend DirectoryRange ListFiles(Str::StringRef path, std::error_code& err);
		bool Advance(std::error_code& err);
		std::string current;
		std::shared_ptr<void> handle;
#ifndef _WIN32
		std::string path;
#endif
	};
	class RecursiveDirectoryRange {
	public:
		DirectoryIterator<RecursiveDirectoryRange> begin()
		{
			return DirectoryIterator<RecursiveDirectoryRange>(empty() ? nullptr : this);
		}
		DirectoryIterator<RecursiveDirectoryRange> end()
		{
			return DirectoryIterator<RecursiveDirectoryRange>();
		}
		bool empty() const
		{
			return dirs.empty();
		}

	private:
		friend class DirectoryIterator<RecursiveDirectoryRange>;
		friend RecursiveDirectoryRange ListFilesRecursive(Str::StringRef path, std::error_code& err);
		bool Advance(std::error_code& err);
		std::string current;
		std::string path;
		std::vector<DirectoryRange> dirs;
	};

} // namespace RawPath
#endif // BUILD_VM

// Operations which work on the home path. This should be used when you need to
// create files which are readable and writeable.
namespace HomePath {

	// Open a file for reading/writing/appending/editing
	File OpenRead(Str::StringRef path, std::error_code& err = throws());
	File OpenWrite(Str::StringRef path, std::error_code& err = throws());
	File OpenAppend(Str::StringRef path, std::error_code& err = throws());
	File OpenEdit(Str::StringRef path, std::error_code& err = throws());

	// Check if a file exists
	bool FileExists(Str::StringRef path);

	// Get the timestamp of a file
	std::chrono::system_clock::time_point FileTimestamp(Str::StringRef path, std::error_code& err = throws());

	// Move/delete a file
	void MoveFile(Str::StringRef dest, Str::StringRef src, std::error_code& err = throws());
	void DeleteFile(Str::StringRef path, std::error_code& err = throws());

	// List all files in the given subdirectory, optionally recursing into subdirectories
	// Directory names are returned with a trailing slash to differentiate them from files
#ifdef BUILD_VM
	using DirectoryRange = std::vector<std::string>;
	using RecursiveDirectoryRange = std::vector<std::string>;
#else
	using DirectoryRange = RawPath::DirectoryRange;
	using RecursiveDirectoryRange = RawPath::RecursiveDirectoryRange;
#endif
	DirectoryRange ListFiles(Str::StringRef path, std::error_code& err = throws());
	RecursiveDirectoryRange ListFilesRecursive(Str::StringRef path, std::error_code& err = throws());

	// Helper function to complete a filename. The root is prepended to the path but not included
	// in the completion results.
	Cmd::CompletionResult CompleteFilename(Str::StringRef prefix, Str::StringRef root, Str::StringRef extension, bool allowSubdirs, bool stripExtension);

} // namespace HomePath

// Initialize the filesystem and the main paths
#ifdef BUILD_VM
void Initialize();
#else
std::string DefaultBasePath();
std::string DefaultHomePath();
void Initialize(Str::StringRef homePath, Str::StringRef libPath, const std::vector<std::string>& paths);
#endif

// Flush write buffers for all open files, useful in case of an unclean shutdown
void FlushAll();

// Check if the filesystem paths have been initialized
bool IsInitialized();

#ifndef BUILD_VM
// Refresh the list of available paks
void RefreshPaks();
#endif

// Find a pak by name, version or checksum. Note that the checksum may not match
// the actual checksum since the search is only done using file names.
const PakInfo* FindPak(Str::StringRef name);
const PakInfo* FindPak(Str::StringRef name, Str::StringRef version);
const PakInfo* FindPak(Str::StringRef name, Str::StringRef version, uint32_t checksum);

// Extract a name, version and optional checksum from a pak filename
bool ParsePakName(const char* begin, const char* end, std::string& name, std::string& version, Util::optional<uint32_t>& checksum);

// Generate a pak name from a name, version and optional checksum
std::string MakePakName(Str::StringRef name, Str::StringRef version, Util::optional<uint32_t> checksum = Util::nullopt);

// Get the list of available paks
const std::vector<PakInfo>& GetAvailablePaks();

// Get the home path
const std::string& GetHomePath();

// Get the path containing program binaries
const std::string& GetLibPath();

#ifdef BUILD_ENGINE
// Handle filesystem system calls
void HandleFileSystemSyscall(int minor, Util::Reader& reader, IPC::Channel& channel, Str::StringRef vmName);
#endif

} // namespace FS

#endif // COMMON_FILESYSTEM_H_
