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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "FileSystem.h"
#include "../../common/Log.h"
#include "../../common/Command.h"
#include "../../libs/minizip/unzip.h"
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#undef MoveFile
#undef CopyFile
#undef DeleteFile
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace FS {

// Pak zip and directory extensions
#define PAK_ZIP_EXT ".pk3"
#define PAK_DIR_EXT ".pk3dir/"

// Dependencies file in packages
#define PAK_DEPS_FILE "DEPS"

// Whether the search paths have been initialized yet. This can be used to delay
// writing log files until the filesystem is initialized.
static bool isInitialized = false;

// Pak search paths
static std::vector<std::string> pakPaths;

// Library & executable path
static std::string libPath;

// Home path
static std::string homePath;

// List of available paks
static std::vector<PakInfo> availablePaks;

// Clean up platform compatibility issues
enum openMode_t {
	MODE_READ,
	MODE_WRITE,
	MODE_APPEND
};
static FILE* my_fopen(Str::StringRef path, openMode_t mode)
{
#ifdef _WIN32
	const wchar_t* modes[] = {L"rb", L"wb", L"ab"};
	return _wfopen(Str::UTF8To16(path).c_str(), modes[mode]);
#else
	const char* modes[] = {"rb", "wb", "ab"};
#if defined(__APPLE__)
	FILE* fd = fopen(path.c_str(), modes[mode]);
#elif defined(__linux__)
	FILE* fd = fopen64(path.c_str(), modes[mode]);
#endif

	// Only allow opening regular files
	if (mode == MODE_READ && fd) {
		struct stat st;
		if (fstat(fileno(fd), &st) == -1) {
			fclose(fd);
			return nullptr;
		}
		if (!S_ISREG(st.st_mode)) {
			fclose(fd);
			errno = EISDIR;
			return nullptr;
		}
	}

	// Set the close-on-exec flag
	if (fd && fcntl(fileno(fd), F_SETFD, FD_CLOEXEC) == -1) {
		fclose(fd);
		return nullptr;
	}

	return fd;
#endif
}
static offset_t my_ftell(FILE* fd)
{
#ifdef _WIN32
		return _ftelli64(fd);
#elif defined(__APPLE__)
		return ftello(fd);
#elif defined(__linux__)
		return ftello64(fd);
#endif
}
static int my_fseek(FILE* fd, offset_t off, int whence)
{
#ifdef _WIN32
		return _fseeki64(fd, off, whence);
#elif defined(__APPLE__)
		return fseeko(fd, off, whence);
#elif defined(__linux__)
		return fseeko64(fd, off, whence);
#endif
}
#ifdef _WIN32
typedef struct _stat32i64 my_stat_t;
#else
typedef struct stat64 my_stat_t;
#endif
static int my_fstat(int fd, my_stat_t* st)
{
#ifdef _WIN32
		return _fstat32i64(fd, st);
#else
		return fstat64(fd, st);
#endif
}
static int my_stat(Str::StringRef path, my_stat_t* st)
{
#ifdef _WIN32
		return _wstat32i64(Str::UTF8To16(path).c_str(), st);
#else
		return stat64(path.c_str(), st);
#endif
}

// std::error_code support for minizip
class minizip_category_impl: public std::error_category
{
public:
	virtual const char* name() const NOEXCEPT OVERRIDE FINAL
	{
		return "unzip";
	}
	virtual std::string message(int ev) const OVERRIDE FINAL
	{
		switch (ev) {
		case UNZ_OK:
			return "Success";
		case UNZ_END_OF_LIST_OF_FILE:
			return "End of list of file";
		case UNZ_ERRNO:
			return "I/O error";
		case UNZ_PARAMERROR:
			return "Invalid parameter";
		case UNZ_BADZIPFILE:
			return "Bad zip file";
		case UNZ_INTERNALERROR:
			return "Internal error";
		case UNZ_CRCERROR:
			return "CRC error";
		default:
			return "Unknown error";
		}
	}
};
static const minizip_category_impl& minizip_category()
{
	static minizip_category_impl instance;
	return instance;
}

// Filesystem-specific error codes
enum filesystem_error {
	invalid_filename,
	no_such_file,
	no_such_directory,
	wrong_pak_checksum,
	missing_depdency
};
class filesystem_category_impl: public std::error_category
{
public:
	virtual const char* name() const NOEXCEPT OVERRIDE FINAL
	{
		return "filesystem";
	}
	virtual std::string message(int ev) const OVERRIDE FINAL
	{
		switch (ev) {
		case filesystem_error::invalid_filename:
			return "Filename contains invalid characters";
		case filesystem_error::no_such_file:
			return "No such file";
		case filesystem_error::no_such_directory:
			return "No such directory";
		case filesystem_error::wrong_pak_checksum:
			return "Pak checksum incorrect";
		case filesystem_error::missing_depdency:
			return "Missing dependency";
		default:
			return "Unknown error";
		}
	}
};
static const filesystem_category_impl& filesystem_category()
{
	static filesystem_category_impl instance;
	return instance;
}

// Support code for error handling
static void SetErrorCode(std::error_code& err, int ec, const std::error_category& ecat)
{
	std::error_code ecode(ec, ecat);
	if (&err == &throws())
		throw std::system_error(ecode);
	else
		err = ecode;
}
static void ClearErrorCode(std::error_code& err)
{
	if (&err != &throws())
		err = std::error_code();
}
static bool HaveError(std::error_code& err)
{
	return &err != &throws() && err;
}
static void SetErrorCodeSystem(std::error_code& err)
{
#ifdef _WIN32
	SetErrorCode(err, _doserrno, std::system_category());
#else
	SetErrorCode(err, errno, std::generic_category());
#endif
}
static void SetErrorCodeFilesystem(filesystem_error ec, std::error_code& err)
{
	SetErrorCode(err, ec, filesystem_category());
}
static void SetErrorCodeZlib(std::error_code& err, int num)
{
	if (num == UNZ_ERRNO)
		SetErrorCodeSystem(err);
	else
		SetErrorCode(err, num, minizip_category());
}

// Determine path to the executable, default to current directory
static std::string DefaultBasePath()
{
#ifdef _WIN32
	wchar_t buffer[MAX_PATH];
	DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
	if (len == 0 || len >= MAX_PATH)
		return "";

	wchar_t* p = wcsrchr(buffer, L'\\');
	if (!p)
		return "";
	*p = L'\0';

	return Str::UTF16To8(buffer);
#elif defined(__linux__)
	ssize_t len = 64;
	while (true) {
		std::unique_ptr<char[]> out(new char[len]);
		ssize_t result = readlink("/proc/self/exe", out.get(), len);
		if (result == -1)
			return "";
		if (result < len) {
			out[result] = '\0';
			char* p = strrchr(out.get(), '/');
			if (!p)
				return "";
			*p = '\0';
			return out.get();
		}
	}
#elif defined(__APPLE__)
	uint32_t bufsize = 0;
	_NSGetExecutablePath(nullptr, &bufsize);

	std::unique_ptr<char[]> out(new char[bufsize]);
	_NSGetExecutablePath(out.get(), &bufsize);

	char* p = strrchr(out.get(), '/');
	if (!p)
		return "";
	*p = '\0';

	return out.get();
#endif
}

// Determine path to user settings directory
static std::string DefaultHomePath()
{
#ifdef _WIN32
	wchar_t buffer[MAX_PATH];
	if (!SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, buffer)))
		return "";
	return Str::UTF16To8(buffer) + "\\My Games\\Unvanquished";
#else
	const char* home = getenv("HOME");
	if (!home)
		return "";
#ifdef __APPLE__
	return std::string(home) + "/Library/Application Support/Unvanquished";
#else
	return std::string(home) + "/.unvanquished";
#endif
#endif
}

// Determine whether a character is a OS-dependent path separator
inline bool isdirsep(unsigned int c)
{
#ifdef _WIN32
	return c == '/' || c == '\\';
#else
	return c == '/';
#endif
}

// Test a directory for write permission
static bool TestWritePermission(Str::StringRef path)
{
	// Create a temporary file in the path and then delete it
	std::string fname = Path::Build(path, ".test_write_permission");
	std::error_code err;
	File f = RawPath::OpenWrite(fname, err);
	if (HaveError(err))
		return false;
	f.Close(err);
	if (HaveError(err))
		return false;
	RawPath::DeleteFile(fname, err);
	if (HaveError(err))
		return false;
	return true;
}

void Initialize()
{
	Com_StartupVariable("fs_basepath");
	Com_StartupVariable("fs_extrapath");
	Com_StartupVariable("fs_homepath");
	Com_StartupVariable("fs_libpath");

	std::string defaultBasePath = DefaultBasePath();
	std::string defaultHomePath = (!defaultBasePath.empty() && TestWritePermission(defaultBasePath)) ? defaultBasePath : DefaultHomePath();
	libPath = Cvar_Get("fs_libpath", defaultBasePath.c_str(), CVAR_INIT)->string;
	homePath = Cvar_Get("fs_homepath", defaultHomePath.c_str(), CVAR_INIT)->string;
	const char* basePath = Cvar_Get("fs_basepath", defaultBasePath.c_str(), CVAR_INIT)->string;
	const char* extraPath = Cvar_Get("fs_extrapath", "", CVAR_INIT)->string;

	pakPaths.push_back(Path::Build(homePath, "pkg"));
	if (basePath != homePath)
		pakPaths.push_back(Path::Build(basePath, "pkg"));
	if (extraPath[0] && extraPath != basePath && extraPath != homePath)
		pakPaths.push_back(Path::Build(extraPath, "pkg"));
	isInitialized = true;

	Com_Printf("Filesystem home path: %s\n", homePath.c_str());
	Com_Printf("Filesystem search paths:\n");
	for (std::string& x: pakPaths)
		Com_Printf("- %s\n", x.c_str());

	RefreshPaks();
}

void FlushAll()
{
	fflush(nullptr);
}

bool IsInitialized()
{
	return isInitialized;
}

// Add a pak to the list of available paks
static void AddPak(pakType_t type, Str::StringRef filename, Str::StringRef basePath)
{
	std::string fullPath = Path::Build(basePath, filename);

	size_t suffixLen = type == PAK_DIR ? strlen(PAK_DIR_EXT) : strlen(PAK_ZIP_EXT);
	std::string name, version;
	Opt::optional<uint32_t> checksum;
	if (!ParsePakName(filename.begin(), filename.end() - suffixLen, name, version, checksum) || (type == PAK_DIR && checksum)) {
		Log::Warn("Invalid pak name: %s\n", fullPath);
		return;
	}

	availablePaks.push_back({std::move(name), std::move(version), checksum, type, std::move(fullPath)});
}

// Find all paks in the given path
static void FindPaksInPath(Str::StringRef basePath, Str::StringRef subPath)
{
	std::string fullPath = Path::Build(basePath, subPath);
	try {
		for (auto& filename: RawPath::ListFiles(fullPath)) {
			if (Str::IsSuffix(PAK_ZIP_EXT, filename)) {
				AddPak(PAK_ZIP, Path::Build(subPath, filename), basePath);
			} else if (Str::IsSuffix(PAK_DIR_EXT, filename)) {
				AddPak(PAK_DIR, Path::Build(subPath, filename), basePath);
			} else if (Str::IsSuffix("/", filename))
				FindPaksInPath(basePath, Path::Build(subPath, filename));
		}
	} catch (std::system_error& err) {
		// If there was an error reading a directory, just ignore it and go to
		// the next one.
		Log::Debug("Error reading directory %s: %s\n", fullPath, err.what());
	}
}

// Comparaison function for version numbers
// Implementation is based on dpkg's version comparison code (verrevcmp() and order())
// http://anonscm.debian.org/gitweb/?p=dpkg/dpkg.git;a=blob;f=lib/dpkg/version.c;hb=74946af470550a3295e00cf57eca1747215b9311
static int VersionCmp(Str::StringRef aStr, Str::StringRef bStr)
{
	// Character weight
	auto order = [](char c) -> int {
		if (Str::cisdigit(c))
			return 0;
		else if (Str::cisalpha(c))
			return c;
		else if (c)
			return c + 256;
		else
			return 0;
	};

	const char* a = aStr.c_str();
	const char* b = bStr.c_str();

	while (*a || *b) {
		int firstDiff = 0;

		while ((*a && !Str::cisdigit(*a)) || (*b && !Str::cisdigit(*b))) {
			int ac = order(*a);
			int bc = order(*b);

			if (ac != bc)
				return ac - bc;

			a++;
			b++;
		}

		while (*a == '0')
			a++;
		while (*b == '0')
			b++;

		while (Str::cisdigit(*a) && Str::cisdigit(*b)) {
			if (firstDiff == 0)
				firstDiff = *a - *b;
			a++;
			b++;
		}

		if (Str::cisdigit(*a))
			return 1;
		if (Str::cisdigit(*b))
			return -1;
		if (firstDiff)
			return firstDiff;
	}

	return false;
}

void RefreshPaks()
{
	availablePaks.clear();
	for (auto& path: pakPaths)
		FindPaksInPath(path, "");

	// Sort the pak list for easy binary searching:
	// First sort by name, then by version.
	// For checksums, place files without checksums in front, so they are selected
	// before any pak with an explicit checksum.
	// Use a stable sort to preserve the order of files within search paths
	std::stable_sort(availablePaks.begin(), availablePaks.end(), [](const PakInfo& a, const PakInfo& b) -> bool {
		// Sort by name
		int result = a.name.compare(b.name);
		if (result != 0)
			return result < 0;

		// Sort by version, putting latest versions last
		result = VersionCmp(a.version, b.version);
		if (result != 0)
			return result < 0;

		// Sort by checksum, putting packages with no checksum last
		if (a.checksum != b.checksum)
			return a.checksum > b.checksum;

		// Prefer zip packages to directory packages
		return b.type == PAK_ZIP;
	});
}

const PakInfo* FindPak(Str::StringRef name)
{
	// Find the latest version with the matching name
	auto iter = std::upper_bound(availablePaks.begin(), availablePaks.end(), name, [](Str::StringRef name, const PakInfo& pakInfo) -> bool {
		return name < pakInfo.name;
	});

	if (iter == availablePaks.begin() || (iter - 1)->name != name)
		return nullptr;
	else
		return &*(iter - 1);
}

const PakInfo* FindPak(Str::StringRef name, Str::StringRef version)
{
	// Find a matching name and version, but prefer the last matching element since that is usually the one with no checksum
	auto iter = std::upper_bound(availablePaks.begin(), availablePaks.end(), name, [version](Str::StringRef name, const PakInfo& pakInfo) -> bool {
		int result = name.compare(pakInfo.name);
		if (result != 0)
			return result < 0;
		return VersionCmp(version, pakInfo.version) < 0;
	});

	if (iter == availablePaks.begin() || (iter - 1)->name != name || (iter - 1)->version != version)
		return nullptr;
	else
		return &*(iter - 1);
}

const PakInfo* FindPak(Str::StringRef name, Str::StringRef version, uint32_t checksum)
{
	// Try to find an exact match
	auto iter = std::upper_bound(availablePaks.begin(), availablePaks.end(), name, [version, checksum](Str::StringRef name, const PakInfo& pakInfo) -> bool {
		int result = name.compare(pakInfo.name);
		if (result != 0)
			return result < 0;
		result = VersionCmp(version, pakInfo.version);
		if (result != 0)
			return result < 0;
		return checksum > pakInfo.checksum;
	});

	if (iter == availablePaks.begin() || (iter - 1)->name != name || (iter - 1)->version != version || !(iter - 1)->checksum || *(iter - 1)->checksum != checksum) {
		// Try again, but this time look for the pak without a checksum. We will verify the checksum later.
		iter = std::upper_bound(availablePaks.begin(), availablePaks.end(), name, [version](Str::StringRef name, const PakInfo& pakInfo) -> bool {
			int result = name.compare(pakInfo.name);
			if (result != 0)
				return result < 0;
			result = VersionCmp(version, pakInfo.version);
			if (result != 0)
				return result < 0;
			return Opt::nullopt > pakInfo.checksum;
		});

		// Only allow zip packages because directories don't have a checksum
		if (iter == availablePaks.begin() || (iter - 1)->type == PAK_DIR || (iter - 1)->name != name || (iter - 1)->version != version || (iter - 1)->checksum)
			return nullptr;
	}

	return &*(iter - 1);
}

bool ParsePakName(const char* begin, const char* end, std::string& name, std::string& version, Opt::optional<uint32_t>& checksum)
{
	const char* nameStart = std::find(std::reverse_iterator<const char*>(end), std::reverse_iterator<const char*>(begin), '/').base();
	if (nameStart != begin)
		nameStart++;

	// Get the name of the package
	const char* underscore1 = std::find(nameStart, end, '_');
	if (underscore1 == end)
		return false;
	name.assign(begin, underscore1);

	// Get the version of the package
	const char* underscore2 = std::find(underscore1 + 1, end, '_');
	if (underscore2 == end) {
		version.assign(underscore1 + 1, end);
		checksum = Opt::nullopt;
	} else {
		// Get the optional checksum of the package
		version.assign(underscore1 + 1, underscore2);
		if (underscore2 + 9 != end)
			return false;
		checksum = 0;
		for (int i = 0; i < 8; i++) {
			char c = underscore2[i + 1];
			if (!Str::cisxdigit(c))
				return false;
			uint32_t hexValue = Str::cisdigit(c) ? c - '0' : Str::ctolower(c) - 'a' + 10;
			checksum = (*checksum << 4) | hexValue;
		}
	}

	return true;
}

std::string MakePakName(Str::StringRef name, Str::StringRef version, Opt::optional<uint32_t> checksum)
{
	if (checksum)
		return Str::Format("%s_%s_%08x", name, version, *checksum);
	else
		return Str::Format("%s_%s", name, version);
}

const std::vector<PakInfo>& GetAvailablePaks()
{
	return availablePaks;
}

const std::string& GetHomePath()
{
	return homePath;
}

const std::string& GetLibPath()
{
	return libPath;
}

namespace Path {

bool IsValid(Str::StringRef path, bool allowDir)
{
	bool nonAlphaNum = true;
	for (char c: path) {
		// Always allow alphanumeric characters
		if (Str::cisalnum(c)) {
			nonAlphaNum = false;
			continue;
		}

		// Don't allow 2 consecutive punctuation characters
		if (nonAlphaNum)
			return false;

		// Only allow the following punctuation characters
		if (c != '/' && c != '-' && c != '_' && c != '.')
			return false;
		nonAlphaNum = true;
	}

	// An empty path or a path ending with / is a directory
	if (!allowDir && nonAlphaNum)
		return path.empty() || path.back() == '/';

	return true;
}

std::string Build(Str::StringRef base, Str::StringRef path)
{
	if (base.empty())
		return path;
	if (path.empty())
		return base;

	std::string out;
	out.reserve(base.size() + 1 + path.size());
	out.assign(base.data(), base.size());
	if (!isdirsep(out.back()))
		out.push_back('/');
	out.append(path.data(), path.size());
	return out;
}

std::string DirName(Str::StringRef path)
{
	std::string out;
	if (path.empty())
		return out;

	// Trim to last slash, excluding any trailing slash
	size_t lastSlash = path.rfind('/', path.size() - 2);
	if (lastSlash != Str::StringRef::npos)
		out = path.substr(0, lastSlash);

	return out;
}

std::string BaseName(Str::StringRef path)
{
	std::string out;
	if (path.empty())
		return out;

	// Trim from last slash, excluding any trailing slash
	size_t lastSlash = path.rfind('/', path.size() - 2);
	if (lastSlash != Str::StringRef::npos)
		out = path.substr(lastSlash + 1);

	return out;
}

std::string Extension(Str::StringRef path)
{
	if (path.empty())
		return "";
	if (path.back() == '/')
		return "/";

	// Find a dot or slash, searching from the end of the string
	for (const char* p = path.end(); p != path.begin(); p--) {
		if (p[-1] == '/')
			return "";
		if (p[-1] == '.')
			return std::string(p - 1, path.end());
	}
	return "";
}

std::string StripExtension(Str::StringRef path)
{
	if (path.empty())
		return "";
	if (path.back() == '/')
		return path.substr(path.size() - 1);

	// Find a dot or slash, searching from the end of the string
	for (const char* p = path.end(); p != path.begin(); p--) {
		if (p[-1] == '/')
			return path;
		if (p[-1] == '.')
			return std::string(path.begin(), p - 1);
	}
	return path;
}

std::string BaseNameStripExtension(Str::StringRef path)
{
	if (path.empty())
		return "";

	// Find a dot or slash, searching from the end of the string
	const char* end = path.end();
	if (path.back() == '/')
		end = path.end() - 1;
	for (const char* p = end; p != path.begin(); p--) {
		if (p[-1] == '/')
			return std::string(p, end);
		if (p[-1] == '.' && end == path.end())
			end = p - 1;
	}
	return std::string(path.begin(), end);
}

} // namespace Path

void File::Close(std::error_code& err)
{
	if (fd) {
		// Always clear fd, even if we throw an exception
		FILE* tmp = fd;
		fd = nullptr;
		if (fclose(tmp) != 0)
			SetErrorCodeSystem(err);
		else
			ClearErrorCode(err);
	}
}
offset_t File::Length(std::error_code& err) const
{
	my_stat_t st;
	if (my_fstat(fileno(fd), &st) != 0) {
		SetErrorCodeSystem(err);
		return 0;
	} else {
		ClearErrorCode(err);
		return st.st_size;
	}
}
std::chrono::system_clock::time_point File::Timestamp(std::error_code& err) const
{
	my_stat_t st;
	if (my_fstat(fileno(fd), &st) != 0) {
		SetErrorCodeSystem(err);
		return {};
	} else {
		ClearErrorCode(err);
		return std::chrono::system_clock::from_time_t(std::max(st.st_ctime, st.st_mtime));
	}
}
void File::SeekCur(offset_t off, std::error_code& err) const
{
	if (my_fseek(fd, off, SEEK_CUR) != 0)
		SetErrorCodeSystem(err);
	else
		ClearErrorCode(err);
}
void File::SeekSet(offset_t off, std::error_code& err) const
{
	if (my_fseek(fd, off, SEEK_SET) != 0)
		SetErrorCodeSystem(err);
	else
		ClearErrorCode(err);
}
void File::SeekEnd(offset_t off, std::error_code& err) const
{
	if (my_fseek(fd, off, SEEK_END) != 0)
		SetErrorCodeSystem(err);
	else
		ClearErrorCode(err);
}
offset_t File::Tell() const
{
	return my_ftell(fd);
}
size_t File::Read(void* buffer, size_t length, std::error_code& err) const
{
	size_t result = fread(buffer, 1, length, fd);
	if (result != length && ferror(fd))
		SetErrorCodeSystem(err);
	else
		ClearErrorCode(err);
	return result;
}
void File::Write(const void* data, size_t length, std::error_code& err) const
{
	if (fwrite(data, 1, length, fd) != length)
		SetErrorCodeSystem(err);
	else
		ClearErrorCode(err);
}
void File::Flush(std::error_code& err) const
{
	if (fflush(fd) != 0)
		SetErrorCodeSystem(err);
	else
		ClearErrorCode(err);
}
std::string File::ReadAll(std::error_code& err) const
{
	offset_t length = Length(err);
	if (HaveError(err))
		return "";
	std::string out;
	out.resize(length);
	Read(&out[0], length, err);
	return out;
}
void File::CopyTo(const File& dest, std::error_code& err) const
{
	char buffer[65536];
	while (true) {
		size_t read = Read(buffer, sizeof(buffer), err);
		if (HaveError(err) || read == 0)
			return;
		dest.Write(buffer, read, err);
		if (HaveError(err))
			return;
	}
}

// Class representing an open zip archive
class ZipArchive {
public:
	ZipArchive()
		: zipFile(nullptr) {}

	// Noncopyable
	ZipArchive(const ZipArchive&) = delete;
	ZipArchive& operator=(const ZipArchive&) = delete;
	ZipArchive(ZipArchive&& other)
		: zipFile(other.zipFile)
	{
		other.zipFile = nullptr;
	}
	ZipArchive& operator=(ZipArchive&& other)
	{
		std::swap(zipFile, other.zipFile);
		return *this;
	}

	// Close archive
	~ZipArchive()
	{
		if (zipFile)
			unzClose(zipFile);
	}

	// Open an archive
	static ZipArchive Open(Str::StringRef path, std::error_code& err)
	{
		// Initialize the zlib I/O functions
		zlib_filefunc64_def funcs;
		funcs.zopen64_file = [](voidpf opaque, const void* filename, int mode) -> voidpf {
			// Interpret the filename as a file handle
			Q_UNUSED(opaque);
			Q_UNUSED(mode);
			return const_cast<void*>(filename);
		};
		funcs.zread_file = [](voidpf opaque, voidpf stream, void* buf, uLong size) -> uLong {
			Q_UNUSED(opaque);
			return fread(buf, 1, size, static_cast<FILE*>(stream));
		};
		funcs.zwrite_file = [](voidpf opaque, voidpf stream, const void* buf, uLong size) -> uLong {
			Q_UNUSED(opaque);
			return fwrite(buf, 1, size, static_cast<FILE*>(stream));
		};
		funcs.ztell64_file = [](voidpf opaque, voidpf stream) -> ZPOS64_T {
			Q_UNUSED(opaque);
			return my_ftell(static_cast<FILE*>(stream));
		};
		funcs.zseek64_file = [](voidpf opaque, voidpf stream, ZPOS64_T offset, int origin) -> long {
			Q_UNUSED(opaque);
			switch (origin) {
			case ZLIB_FILEFUNC_SEEK_CUR:
				origin = SEEK_CUR;
				break;
			case ZLIB_FILEFUNC_SEEK_END:
				origin = SEEK_END;
				break;
			case ZLIB_FILEFUNC_SEEK_SET:
				origin = SEEK_SET;
				break;
			default:
				return -1;
			}
			return my_fseek(static_cast<FILE*>(stream), offset, origin);
		};
		funcs.zclose_file = [](voidpf opaque, voidpf stream) -> int {
			Q_UNUSED(opaque);
			return fclose(static_cast<FILE*>(stream));
		};
		funcs.zerror_file = [](voidpf opaque, voidpf stream) -> int {
			Q_UNUSED(opaque);
			return ferror(static_cast<FILE*>(stream));
		};

		// Open the file
		FILE* fd = my_fopen(path, MODE_READ);
		if (!fd) {
			SetErrorCodeSystem(err);
			return ZipArchive();
		}

		// Open the zip with zlib
		unzFile zipFile = unzOpen2_64(fd, &funcs);
		if (!zipFile) {
			// Unfortunately unzOpen doesn't return an error code, so we assume UNZ_BADZIPFILE
			SetErrorCodeZlib(err, UNZ_BADZIPFILE);
			return ZipArchive();
		}

		ClearErrorCode(err);
		ZipArchive out;
		out.zipFile = zipFile;
		return out;
	}

	// Iterate through all the files in the archive and invoke the callback.
	// Callback signature: void(Str::StringRef filename, offset_t offset, uint32_t crc)
	template<typename Func> void ForEachFile(Func&& func, std::error_code& err)
	{
		unz_global_info64 globalInfo;
		int result = unzGetGlobalInfo64(zipFile, &globalInfo);
		if (result != UNZ_OK) {
			SetErrorCodeZlib(err, result);
			return;
		}

		result = unzGoToFirstFile(zipFile);
		if (result != UNZ_OK) {
			SetErrorCodeZlib(err, result);
			return;
		}

		for (ZPOS64_T i = 0; i != globalInfo.number_entry; i++) {
			unz_file_info64 fileInfo;
			char filename[65537]; // The zip format has a maximum filename size of 64K
			result = unzGetCurrentFileInfo64(zipFile, &fileInfo, filename, sizeof(filename), nullptr, 0, nullptr, 0);
			if (result != UNZ_OK) {
				SetErrorCodeZlib(err, result);
				return;
			}
			offset_t offset = unzGetOffset64(zipFile);
			uint32_t crc = fileInfo.crc;
			func(filename, offset, crc);

			if (i + 1 != globalInfo.number_entry) {
				result = unzGoToNextFile(zipFile);
				if (result != UNZ_OK) {
					SetErrorCodeZlib(err, result);
					return;
				}
			}
		}

		ClearErrorCode(err);
	}

	// Open a file in the archive
	void OpenFile(offset_t offset, std::error_code& err) const
	{
		// Set position in zip
		int result = unzSetOffset64(zipFile, offset);
		if (result != UNZ_OK) {
			SetErrorCodeZlib(err, result);
			return;
		}

		// Open file in zip
		result = unzOpenCurrentFile(zipFile);
		if (result != UNZ_OK)
			SetErrorCodeZlib(err, result);
		else
			ClearErrorCode(err);
	}

	// Get the length of the currently open file
	offset_t FileLength(std::error_code& err) const
	{
		unz_file_info64 fileInfo;
		int result = unzGetCurrentFileInfo64(zipFile, &fileInfo, nullptr, 0, nullptr, 0, nullptr, 0);
		if (result != UNZ_OK) {
			SetErrorCodeZlib(err, result);
			return 0;
		}
		ClearErrorCode(err);
		return fileInfo.uncompressed_size;
	}

	// Read from the currently open file
	size_t ReadFile(void* buffer, size_t length, std::error_code& err) const
	{
		// zlib read returns an int, which means that we can only read 2G at once
		size_t read = 0;
		while (read != length) {
			size_t currentRead = std::max<size_t>(length - read, INT_MAX);
			int result = unzReadCurrentFile(zipFile, buffer, currentRead);
			if (result < 0) {
				SetErrorCodeZlib(err, result);
				return read;
			}
			if (result == 0)
				break;
			buffer = static_cast<char*>(buffer) + result;
			read += result;
		}
		ClearErrorCode(err);
		return read;
	}

	// Close the currently open file and check for CRC errors
	void CloseFile(std::error_code& err) const
	{
		int result = unzCloseCurrentFile(zipFile);
		if (result != UNZ_OK)
			SetErrorCodeZlib(err, result);
		else
			ClearErrorCode(err);
	}

private:
	unzFile zipFile;
};

namespace PakPath {

// List of loaded pak files
static std::vector<PakInfo> loadedPaks;

// Map of filenames to pak files. The size_t is an offset into loadedPaks and
// the offset_t is the position within the zip archive (unused for PAK_DIR).
static std::unordered_map<std::string, std::pair<size_t, offset_t>> fileMap;

// Parse the dependencies file of a package
// Each line of the dependencies file is a name followed by an optional version
static void ParseDeps(const PakInfo& parent, Str::StringRef depsData, std::error_code& err)
{
	auto lineStart = depsData.begin();
	int line = 0;
	while (lineStart != depsData.end()) {
		// Get the end of the line or the end of the file
		line++;
		auto lineEnd = std::find(lineStart, depsData.end(), '\n');

		// Skip spaces
		while (lineStart != lineEnd && Str::cisspace(*lineStart))
			lineStart++;
		if (lineStart == lineEnd) {
			lineStart = lineEnd == depsData.end() ? lineEnd : lineEnd + 1;
			continue;
		}

		// Read the package name
		std::string name;
		while (lineStart != lineEnd && !Str::cisspace(*lineStart))
			name.push_back(*lineStart++);

		// Skip spaces
		while (lineStart != lineEnd && Str::cisspace(*lineStart))
			lineStart++;

		// If this is the end of the line, load a package by name
		if (lineStart == lineEnd) {
			const PakInfo* pak = FindPak(name);
			if (!pak) {
				Log::Warn("Could not find pak '%s' required by '%s'\n", name, parent.path);
				SetErrorCodeFilesystem(filesystem_error::missing_depdency, err);
				return;
			}
			LoadPak(*pak, err);
			if (HaveError(err))
				return;
			lineStart = lineEnd == depsData.end() ? lineEnd : lineEnd + 1;
			continue;
		}

		// Read the package version
		std::string version;
		while (lineStart != lineEnd && !Str::cisspace(*lineStart))
			version.push_back(*lineStart++);

		// Skip spaces
		while (lineStart != lineEnd && Str::cisspace(*lineStart))
			lineStart++;

		// If this is the end of the line, load a package with an explicit version
		if (lineStart == lineEnd) {
			const PakInfo* pak = FindPak(name, version);
			if (!pak) {
				Log::Warn("Could not find pak '%s' with version '%s' required by '%s'\n", name, version, parent.path);
				SetErrorCodeFilesystem(filesystem_error::missing_depdency, err);
				return;
			}
			LoadPak(*pak, err);
			if (HaveError(err))
				return;
			lineStart = lineEnd == depsData.end() ? lineEnd : lineEnd + 1;
			continue;
		}

		// If there is still stuff at the end of the line, print a warning and ignore it
		Log::Warn("Invalid dependency specification on line %d in %s\n", line, Path::Build(parent.path, PAK_DEPS_FILE));
		lineStart = lineEnd == depsData.end() ? lineEnd : lineEnd + 1;
	}
}

static void InternalLoadPak(const PakInfo& pak, Opt::optional<uint32_t> expectedChecksum, std::error_code& err)
{
	uint32_t checksum;
	bool hasDeps = false;
	offset_t depsOffset;
	ZipArchive zipFile;

	// Add the pak to the list of loaded paks
	loadedPaks.push_back(pak);

	// Update the list of files, but don't overwrite existing files to preserve sort order
	if (pak.type == PAK_DIR) {
		auto dirRange = RawPath::ListFilesRecursive(pak.path, err);
		if (HaveError(err))
			return;
		for (auto it = dirRange.begin(); it != dirRange.end();) {
#ifdef GCC_BROKEN_CXX11
			fileMap.insert({*it, std::pair<size_t, offset_t>(loadedPaks.size() - 1, 0)});
#else
			fileMap.emplace(*it, std::pair<size_t, offset_t>(loadedPaks.size() - 1, 0));
#endif
			if (*it == PAK_DEPS_FILE)
				hasDeps = true;
			it.increment(err);
			if (HaveError(err))
				return;
		}
		loadedPaks.back().checksum = Opt::nullopt;
	} else {
		// Open zip
		zipFile = ZipArchive::Open(pak.path, err);
		if (HaveError(err))
			return;

		// Get the file list and calculate the checksum of the package (checksum of all file checksums)
		checksum = crc32(0, Z_NULL, 0);
		zipFile.ForEachFile([&checksum, &hasDeps, &depsOffset](Str::StringRef filename, offset_t offset, uint32_t crc) {
			checksum = crc32(checksum, reinterpret_cast<const Bytef*>(&crc), sizeof(crc));
#ifdef GCC_BROKEN_CXX11
			fileMap.insert({filename, std::pair<size_t, offset_t>(loadedPaks.size() - 1, offset)});
#else
			fileMap.emplace(filename, std::pair<size_t, offset_t>(loadedPaks.size() - 1, offset));
#endif
			if (filename == PAK_DEPS_FILE) {
				hasDeps = true;
				depsOffset = offset;
			}
		}, err);
		if (HaveError(err))
			return;

		// Save the real checksum in the list of loaded paks
		loadedPaks.back().checksum = checksum;

		// Get the timestamp of the pak
		loadedPaks.back().timestamp = FS::RawPath::FileTimestamp(pak.path, err);
		if (HaveError(err))
			return;
	}

	// If an explicit checksum was requested, verify that the pak we loaded is the one we are expecting
	if (expectedChecksum && checksum != *expectedChecksum) {
		SetErrorCodeFilesystem(filesystem_error::wrong_pak_checksum, err);
		return;
	}

	// Print a warning if the checksum doesn't match the one in the filename
	if (pak.checksum && *pak.checksum != checksum)
		Log::Warn("Pak checksum doesn't match filename: %s\n", pak.path);

	// Load dependencies, but not if a checksum was specified
	if (hasDeps && !expectedChecksum) {
		std::string depsData;
		if (pak.type == PAK_DIR) {
			File depsFile = RawPath::OpenRead(Path::Build(pak.path, PAK_DEPS_FILE), err);
			if (HaveError(err))
				return;
			depsData = depsFile.ReadAll(err);
			if (HaveError(err))
				return;
		} else {
			zipFile.OpenFile(depsOffset, err);
			if (HaveError(err))
				return;
			offset_t length = zipFile.FileLength(err);
			if (HaveError(err))
				return;
			depsData.resize(length);
			zipFile.ReadFile(&depsData[0], length, err);
			if (HaveError(err))
				return;
		}
		ParseDeps(pak, depsData, err);
	}
}

void LoadPak(const PakInfo& pak, std::error_code& err)
{
	InternalLoadPak(pak, Opt::nullopt, err);
}

void LoadPakExplicit(const PakInfo& pak, uint32_t expectedChecksum, std::error_code& err)
{
	InternalLoadPak(pak, expectedChecksum, err);
}

const std::vector<PakInfo>& GetLoadedPaks()
{
	return loadedPaks;
}

void ClearPaks()
{
	fileMap.clear();
	loadedPaks.clear();
}

std::string ReadFile(Str::StringRef path, std::error_code& err)
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFilesystem(filesystem_error::no_such_file, err);
		return "";
	}

	const PakInfo& pak = loadedPaks[it->second.first];
	if (pak.type == PAK_DIR) {
		// Open file
		File file = RawPath::OpenRead(Path::Build(pak.path, path), err);
		if (HaveError(err))
			return "";

		// Get file length
		offset_t length = file.Length(err);
		if (HaveError(err))
			return "";

		// Read file contents
		std::string out;
		out.resize(length);
		file.Read(&out[0], length, err);
		return out;
	} else {
		// Open zip
		ZipArchive zipFile = ZipArchive::Open(pak.path, err);
		if (HaveError(err))
			return "";

		// Open file in zip
		zipFile.OpenFile(it->second.second, err);
		if (HaveError(err))
			return "";

		// Get file length
		offset_t length = zipFile.FileLength(err);
		if (HaveError(err))
			return "";

		// Read file
		std::string out;
		out.resize(length);
		zipFile.ReadFile(&out[0], length, err);
		if (HaveError(err))
			return "";

		// Close file and check for CRC errors
		zipFile.CloseFile(err);
		if (HaveError(err))
			return "";

		return out;
	}
}

void CopyFile(Str::StringRef path, const File& dest, std::error_code& err)
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFilesystem(filesystem_error::no_such_file, err);
		return;
	}

	const PakInfo& pak = loadedPaks[it->second.first];
	if (pak.type == PAK_DIR) {
		File file = RawPath::OpenRead(Path::Build(pak.path, path), err);
		if (HaveError(err))
			return;
		file.CopyTo(dest, err);
	} else {
		// Open zip
		ZipArchive zipFile = ZipArchive::Open(pak.path, err);
		if (HaveError(err))
			return;

		// Open file in zip
		zipFile.OpenFile(it->second.second, err);
		if (HaveError(err))
			return;

		// Copy contents into destination
		char buffer[65536];
		while (true) {
			offset_t read = zipFile.ReadFile(buffer, sizeof(buffer), err);
			if (HaveError(err))
				return;
			if (read == 0)
				break;
			dest.Write(buffer, read, err);
			if (HaveError(err))
				return;
		}

		// Close file and check for CRC errors
		zipFile.CloseFile(err);
	}
}

bool FileExists(Str::StringRef path)
{
	return fileMap.find(path) != fileMap.end();
}

const PakInfo* LocateFile(Str::StringRef path)
{
	auto it = fileMap.find(path);
	if (it == fileMap.end())
		return nullptr;
	else
		return &loadedPaks[it->second.first];
}

std::chrono::system_clock::time_point FileTimestamp(Str::StringRef path, std::error_code& err)
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFilesystem(filesystem_error::no_such_file, err);
		return {};
	}

	const PakInfo& pak = loadedPaks[it->second.first];
	if (pak.type == PAK_DIR)
		return RawPath::FileTimestamp(Path::Build(pak.path, path), err);
	else
		return pak.timestamp;
}

bool DirectoryRange::InternalAdvance()
{
	for (; iter != iter_end; ++iter) {
		// Filter out any paths not in the specified directory
		if (!Str::IsPrefix(prefix, iter->first))
			continue;

		// Don't look down subdirectories when not doing a recursive search
		if (!recursive) {
			auto p = std::find(iter->first.begin() + prefix.size(), iter->first.end(), '/');
			if (p != iter->first.end())
				continue;
		}

		current = iter->first.substr(prefix.size());
		return true;
	}

	return false;
}

bool DirectoryRange::Advance(std::error_code& err)
{
	++iter;
	ClearErrorCode(err);
	return InternalAdvance();
}

DirectoryRange ListFiles(Str::StringRef path, std::error_code& err)
{
	DirectoryRange state;
	state.recursive = false;
	state.prefix = path;
	if (!state.prefix.empty() && state.prefix.back() != '/')
		state.prefix.push_back('/');
	state.iter = fileMap.begin();
	state.iter_end = fileMap.end();
	if (!state.InternalAdvance())
		SetErrorCodeFilesystem(filesystem_error::no_such_directory, err);
	else
		ClearErrorCode(err);
	return state;
}

DirectoryRange ListFilesRecursive(Str::StringRef path, std::error_code& err)
{
	DirectoryRange state;
	state.recursive = true;
	state.prefix = path;
	if (!state.prefix.empty() && state.prefix.back() != '/')
		state.prefix.push_back('/');
	state.iter = fileMap.begin();
	state.iter_end = fileMap.end();
	if (!state.InternalAdvance())
		SetErrorCodeFilesystem(filesystem_error::no_such_directory, err);
	else
		ClearErrorCode(err);
	return state;
}

// Note that this function is practically identical to the HomePath version.
// Try to keep any changes in sync between the two versions.
Cmd::CompletionResult CompleteFilename(Str::StringRef prefix, Str::StringRef root, Str::StringRef extension, bool allowSubdirs, bool stripExtension)
{
	std::string prefixDir = Path::DirName(prefix);
	if (!allowSubdirs && !prefixDir.empty())
		return {};
	std::string prefixBase = Path::BaseName(prefix);

	try {
		Cmd::CompletionResult out;
		for (auto& x: PakPath::ListFiles(Path::Build(root, prefixDir))) {
			std::string ext = Path::Extension(x);
			if (!extension.empty() && ext != extension && !(allowSubdirs && ext == "/"))
				continue;
			std::string result;
			if (stripExtension)
				result = Path::Build(prefixDir, Path::StripExtension(x));
			else
				result = Path::Build(prefixDir, x);
			if (Str::IsPrefix(prefix, result))
				out.emplace_back(result, "");
		}
		return out;
	} catch (std::system_error&) {
		return {};
	}
}

} // namespace PakPath

namespace RawPath {

// Create all directories leading to a filename
static void CreatePath(Str::StringRef path, std::error_code& err)
{
#ifdef _WIN32
	std::wstring buffer = Str::UTF8To16(path);
	for (wchar_t& c: buffer) {
		if (!isdirsep(c))
			continue;
		c = '\0';
		if (_wmkdir(buffer.data()) != 0 && errno != EEXIST) {
			SetErrorCodeSystem(err);
			return;
		}
		c = '\\';
	}
#else
	std::string buffer = path;
	for (char& c: buffer) {
		if (!isdirsep(c))
			continue;
		c = '\0';
		if (mkdir(buffer.data(), 0777) != 0 && errno != EEXIST) {
			SetErrorCodeSystem(err);
			return;
		}
		c = '/';
	}
#endif
}

static File OpenMode(Str::StringRef path, openMode_t mode, std::error_code& err)
{
	FILE* fd = my_fopen(path, mode);
	if (!fd && mode != MODE_READ && errno == ENOENT) {
		// Create the directories and try again
		CreatePath(path, err);
		if (HaveError(err))
			return {};
		fd = my_fopen(path, mode);
	}
	if (!fd) {
		SetErrorCodeSystem(err);
		return {};
	} else {
		ClearErrorCode(err);
		return File(fd);
	}
}
File OpenRead(Str::StringRef path, std::error_code& err)
{
	return OpenMode(path, MODE_READ, err);
}
File OpenWrite(Str::StringRef path, std::error_code& err)
{
	return OpenMode(path, MODE_WRITE, err);
}
File OpenAppend(Str::StringRef path, std::error_code& err)
{
	return OpenMode(path, MODE_APPEND, err);
}

bool FileExists(Str::StringRef path)
{
	my_stat_t st;
	return my_stat(path, &st) == 0;
}

std::chrono::system_clock::time_point FileTimestamp(Str::StringRef path, std::error_code& err)
{
	my_stat_t st;
	if (my_stat(path, &st) != 0) {
		SetErrorCodeSystem(err);
		return {};
	} else {
		ClearErrorCode(err);
		return std::chrono::system_clock::from_time_t(std::max(st.st_ctime, st.st_mtime));
	}
}

void MoveFile(Str::StringRef dest, Str::StringRef src, std::error_code& err)
{
#ifdef _WIN32
	// _wrename doesn't follow the POSIX standard because it will fail if the target already exists
	if (!MoveFileExW(Str::UTF8To16(src).c_str(), Str::UTF8To16(dest).c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
		SetErrorCode(err, GetLastError(), std::system_category());
	else
		ClearErrorCode(err);
#else
	if (rename(src.c_str(), dest.c_str()) != 0) {
		// Copy the file if the destination is on a different filesystem
		File srcFile = OpenRead(src, err);
		if (HaveError(err))
			return;
		File destFile = OpenWrite(src, err);
		if (HaveError(err))
			return;
		srcFile.CopyTo(destFile, err);
		if (HaveError(err))
			return;
		destFile.Close(err);
		if (HaveError(err))
			return;
		DeleteFile(src, err);
	} else
		ClearErrorCode(err);
#endif
}

void DeleteFile(Str::StringRef path, std::error_code& err)
{
#ifdef _WIN32
	if (!DeleteFileW(Str::UTF8To16(path).c_str()))
		SetErrorCode(err, GetLastError(), std::system_category());
	else
		ClearErrorCode(err);
#else
	if (unlink(path.c_str()) != 0)
		SetErrorCodeSystem(err);
	else
		ClearErrorCode(err);
#endif
}

bool DirectoryRange::Advance(std::error_code& err)
{
#ifdef _WIN32
	WIN32_FIND_DATAW findData;
	do {
		if (!FindNextFileW(handle.get(), &findData)) {
			int ec = GetLastError();
			if (ec == ERROR_NO_MORE_FILES)
				ClearErrorCode(err);
			else
				SetErrorCode(err, ec, std::system_category());
			handle = nullptr;
			return false;
		}
		current = Str::UTF16To8(findData.cFileName);
	} while (!Path::IsValid(current, false));
	if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		current.push_back('/');
	ClearErrorCode(err);
	return true;
#else
	struct dirent* dirent;
	my_stat_t st;
	do {
		errno = 0;
		dirent = readdir(static_cast<DIR*>(handle.get()));
		if (!dirent) {
			if (errno != 0)
				SetErrorCodeSystem(err);
			else
				ClearErrorCode(err);
			handle = nullptr;
			return false;
		}
	} while (!Path::IsValid(dirent->d_name, false) || my_stat(Path::Build(path, dirent->d_name), &st) != 0);
	current = dirent->d_name;
	if (!S_ISREG(st.st_mode))
		current.push_back('/');
	ClearErrorCode(err);
	return true;
#endif
}

DirectoryRange ListFiles(Str::StringRef path, std::error_code& err)
{
	std::string dirPath = path;
	if (!dirPath.empty() && dirPath.back() == '/')
#ifdef GCC_BROKEN_CXX11
		dirPath.resize(dirPath.size() - 1);
#else
		dirPath.pop_back();
#endif

#ifdef _WIN32
	WIN32_FIND_DATAW findData;
	HANDLE handle = FindFirstFileW(Str::UTF8To16(dirPath + "/*").c_str(), &findData);
	if (handle == INVALID_HANDLE_VALUE) {
		int ec = GetLastError();
		if (ec == ERROR_FILE_NOT_FOUND || ec == ERROR_NO_MORE_FILES)
			ClearErrorCode(err);
		else
			SetErrorCode(err, GetLastError(), std::system_category());
		return {};
	}

	DirectoryRange state;
	state.handle = std::shared_ptr<void>(handle, [](void* handle) {
		FindClose(handle);
	});
	state.current = Str::UTF16To8(findData.cFileName);
	if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		state.current.push_back('/');
	if (!Path::IsValid(state.current, true)) {
		if (!state.Advance(err))
			return {};
	} else
		ClearErrorCode(err);
	return state;
#else
	DIR* handle = opendir(dirPath.c_str());
	if (!handle) {
		SetErrorCodeSystem(err);
		return {};
	}

	DirectoryRange state;
	state.handle = std::shared_ptr<DIR>(handle, [](DIR* handle) {
		closedir(handle);
	});
	state.path = std::move(dirPath);
	if (state.Advance(err))
		return state;
	else
		return {};
#endif
}

bool RecursiveDirectoryRange::Advance(std::error_code& err)
{
	if (current.back() == '/') {
		auto subdir = ListFiles(Path::Build(path, current), err);
		if (HaveError(err))
			return false;
		if (!subdir.empty()) {
			current.append(*subdir.begin());
			dirs.push_back(std::move(subdir));
			return true;
		}
	}

	while (!dirs.empty()) {
		size_t pos = current.rfind('/', current.size() - 2);
		current.resize(pos == std::string::npos ? 0 : pos + 1);
		dirs.back().begin().increment(err);
		if (HaveError(err))
			return false;
		if (!dirs.back().empty())
			break;
		dirs.pop_back();
	}

	if (dirs.empty())
		return false;

	current.append(*dirs.back().begin());
	ClearErrorCode(err);
	return true;
}

RecursiveDirectoryRange ListFilesRecursive(Str::StringRef path, std::error_code& err)
{
	RecursiveDirectoryRange state;
	state.path = path;
	if (!state.path.empty() && state.path.back() != '/')
		state.path.push_back('/');
	auto root = ListFiles(state.path, err);
	if (HaveError(err) || root.begin() == root.end())
		return {};
	state.current = *root.begin();
	state.dirs.push_back(std::move(root));
	return state;
}

} // namespace RawPath

namespace HomePath {

static File OpenMode(Str::StringRef path, openMode_t mode, std::error_code& err)
{
	if (!Path::IsValid(path, false)) {
		SetErrorCodeFilesystem(filesystem_error::invalid_filename, err);
		return {};
	}
	return RawPath::OpenMode(Path::Build(homePath, path), mode, err);
}
File OpenRead(Str::StringRef path, std::error_code& err)
{
	return OpenMode(path, MODE_READ, err);
}
File OpenWrite(Str::StringRef path, std::error_code& err)
{
	return OpenMode(path, MODE_WRITE, err);
}
File OpenAppend(Str::StringRef path, std::error_code& err)
{
	return OpenMode(path, MODE_APPEND, err);
}

bool FileExists(Str::StringRef path)
{
	if (!Path::IsValid(path, false))
		return false;
	return RawPath::FileExists(Path::Build(homePath, path));
}

std::chrono::system_clock::time_point FileTimestamp(Str::StringRef path, std::error_code& err)
{
	if (!Path::IsValid(path, false)) {
		SetErrorCodeFilesystem(filesystem_error::invalid_filename, err);
		return {};
	}
	return RawPath::FileTimestamp(Path::Build(homePath, path), err);
}

void MoveFile(Str::StringRef dest, Str::StringRef src, std::error_code& err)
{
	if (!Path::IsValid(dest, false) || !Path::IsValid(src, false)) {
		SetErrorCodeFilesystem(filesystem_error::invalid_filename, err);
		return;
	}
	RawPath::MoveFile(Path::Build(homePath, dest), Path::Build(homePath, src), err);
}
void DeleteFile(Str::StringRef path, std::error_code& err)
{
	if (!Path::IsValid(path, false)) {
		SetErrorCodeFilesystem(filesystem_error::invalid_filename, err);
		return;
	}
	RawPath::DeleteFile(Path::Build(homePath, path), err);
}

DirectoryRange ListFiles(Str::StringRef path, std::error_code& err)
{
	if (!Path::IsValid(path, true)) {
		SetErrorCodeFilesystem(filesystem_error::invalid_filename, err);
		return {};
	}
	return RawPath::ListFiles(Path::Build(homePath, path), err);
}

RecursiveDirectoryRange ListFilesRecursive(Str::StringRef path, std::error_code& err)
{
	if (!Path::IsValid(path, true)) {
		SetErrorCodeFilesystem(filesystem_error::invalid_filename, err);
		return {};
	}
	return RawPath::ListFilesRecursive(Path::Build(homePath, path), err);
}

// Note that this function is practically identical to the PakPath version.
// Try to keep any changes in sync between the two versions.
Cmd::CompletionResult CompleteFilename(Str::StringRef prefix, Str::StringRef root, Str::StringRef extension, bool allowSubdirs, bool stripExtension)
{
	std::string prefixDir = Path::DirName(prefix);
	if (!allowSubdirs && !prefixDir.empty())
		return {};
	std::string prefixBase = Path::BaseName(prefix);

	try {
		Cmd::CompletionResult out;
		for (auto& x: HomePath::ListFiles(Path::Build(root, prefixDir))) {
			std::string ext = Path::Extension(x);
			if (!extension.empty() && ext != extension && !(allowSubdirs && ext == "/"))
				continue;
			std::string result;
			if (stripExtension)
				result = Path::Build(prefixDir, Path::StripExtension(x));
			else
				result = Path::Build(prefixDir, x);
			if (Str::IsPrefix(prefix, result))
				out.emplace_back(result, "");
		}
		return out;
	} catch (std::system_error&) {
		return {};
	}
}

} // namespace HomePath

} // namespace FS

// Compatibility wrapper

struct handleData_t {
	bool isOpen;
	bool isPakFile;

	// Normal file info
	bool forceFlush;
	FS::File file;

	// Pak file info
	std::string fileData;
	size_t filePos;
};

#define MAX_FILE_HANDLES 64
static handleData_t handleTable[MAX_FILE_HANDLES];
static std::vector<std::tuple<std::string, std::string, uint32_t>> fs_missingPaks;

static fileHandle_t FS_AllocHandle()
{
	// Don't use handle 0 because it is used to indicate failures
	for (int i = 1; i < MAX_FILE_HANDLES; i++) {
		if (!handleTable[i].isOpen)
			return i;
	}
	Com_Error(ERR_DROP, "FS_AllocHandle: none free");
}

static void FS_CheckHandle(fileHandle_t handle, bool write)
{
	if (handle < 0 || handle > MAX_FILE_HANDLES)
		Com_Error(ERR_DROP, "FS_CheckHandle: invalid handle");
	if (!handleTable[handle].isOpen)
		Com_Error(ERR_DROP, "FS_CheckHandle: closed handle");
	if (write && handleTable[handle].isPakFile)
		Com_Error(ERR_DROP, "FS_CheckHandle: writing to file in pak");
}

qboolean FS_FileExists(const char* path)
{
	return FS::PakPath::FileExists(path) || FS::HomePath::FileExists(path);
}

int FS_FOpenFileRead(const char* path, fileHandle_t* handle, qboolean)
{
	if (!handle)
		return FS_FileExists(path);

	*handle = FS_AllocHandle();
	int length;
	try {
		if (FS::PakPath::FileExists(path)) {
			handleTable[*handle].filePos = 0;
			handleTable[*handle].fileData = FS::PakPath::ReadFile(path);
			handleTable[*handle].isPakFile = true;
			handleTable[*handle].isOpen = true;
			length = handleTable[*handle].fileData.size();
		} else {
			handleTable[*handle].file = FS::HomePath::OpenRead(path);
			length = handleTable[*handle].file.Length();
			handleTable[*handle].isPakFile = false;
			handleTable[*handle].isOpen = true;
		}
	} catch (std::system_error& err) {
		Com_DPrintf("Failed to open '%s' for reading: %s\n", path, err.what());
		*handle = 0;
		return -1;
	}

	return length;
}

fileHandle_t FS_FOpenFileWrite(const char* path)
{
	fileHandle_t handle = FS_AllocHandle();
	try {
		handleTable[handle].file = FS::HomePath::OpenWrite(path);
	} catch (std::system_error& err) {
		Com_Printf("Failed to open '%s' for writing: %s\n", path, err.what());
		return 0;
	}
	handleTable[handle].forceFlush = false;
	handleTable[handle].isPakFile = false;
	handleTable[handle].isOpen = true;
	return handle;
}

fileHandle_t FS_FOpenFileAppend(const char* path)
{
	fileHandle_t handle = FS_AllocHandle();
	try {
		handleTable[handle].file = FS::HomePath::OpenAppend(path);
	} catch (std::system_error& err) {
		Com_Printf("Failed to open '%s' for appending: %s\n", path, err.what());
		return 0;
	}
	handleTable[handle].forceFlush = false;
	handleTable[handle].isPakFile = false;
	handleTable[handle].isOpen = true;
	return handle;
}

fileHandle_t FS_SV_FOpenFileWrite(const char* path)
{
	return FS_FOpenFileWrite(path);
}

int FS_SV_FOpenFileRead(const char* path, fileHandle_t* handle)
{
	*handle = FS_AllocHandle();
	int length;
	try {
		handleTable[*handle].file = FS::HomePath::OpenRead(path);
		length = handleTable[*handle].file.Length();
	} catch (std::system_error& err) {
		Com_DPrintf("Failed to open '%s' for reading: %s\n", path, err.what());
		*handle = 0;
		return 0;
	}
	handleTable[*handle].isPakFile = false;
	handleTable[*handle].isOpen = true;
	return length;
}

int FS_FOpenFileByMode(const char* path, fileHandle_t* handle, fsMode_t mode)
{
	switch (mode) {
	case FS_READ:
		return FS_FOpenFileRead(path, handle, qfalse);

	case FS_WRITE:
		*handle = FS_FOpenFileWrite(path);
		return *handle == 0 ? -1 : 0;

	case FS_APPEND:
	case FS_APPEND_SYNC:
		*handle = FS_FOpenFileAppend(path);
		handleTable[*handle].forceFlush = mode == FS_APPEND_SYNC;
		return *handle == 0 ? -1 : 0;

	default:
		Com_Error(ERR_DROP, "FS_FOpenFileByMode: bad mode %d", mode);
	}
}

int FS_FCloseFile(fileHandle_t handle)
{
	if (handle == 0)
		return 0;
	FS_CheckHandle(handle, false);
	handleTable[handle].isOpen = false;
	if (handleTable[handle].isPakFile) {
		handleTable[handle].fileData.clear();
		return 0;
	} else {
		try {
			handleTable[handle].file.Close();
			return 0;
		} catch (std::system_error& err) {
			Com_Printf("Failed to close file: %s\n", err.what());
			return -1;
		}
	}
}

int FS_filelength(fileHandle_t handle)
{
	FS_CheckHandle(handle, false);
	if (handleTable[handle].isPakFile)
		return handleTable[handle].fileData.size();
	else {
		try {
			return handleTable[handle].file.Length();
		} catch (std::system_error& err) {
			Com_Printf("Failed to get file length: %s\n", err.what());
			return 0;
		}
	}
}

int FS_FTell(fileHandle_t handle)
{
	FS_CheckHandle(handle, false);
	if (handleTable[handle].isPakFile)
		return handleTable[handle].filePos;
	else
		return handleTable[handle].file.Tell();
}

int FS_Seek(fileHandle_t handle, long offset, int origin)
{
	FS_CheckHandle(handle, false);
	if (handleTable[handle].isPakFile) {
		switch (origin) {
		case FS_SEEK_CUR:
			handleTable[handle].filePos += offset;
			break;

		case FS_SEEK_SET:
			handleTable[handle].filePos = offset;
			break;

		case FS_SEEK_END:
			handleTable[handle].filePos = handleTable[handle].fileData.size() + offset;
			break;

		default:
			Com_Error(ERR_DROP, "Bad origin in FS_Seek");
		}
		return 0;
	} else {
		try {
			switch (origin) {
			case FS_SEEK_CUR:
				handleTable[handle].file.SeekCur(offset);
				break;

			case FS_SEEK_SET:
				handleTable[handle].file.SeekSet(offset);
				break;

			case FS_SEEK_END:
				handleTable[handle].file.SeekEnd(offset);
				break;

			default:
				Com_Error(ERR_DROP, "Bad origin in FS_Seek");
			}
			return 0;
		} catch (std::system_error& err) {
			Com_Printf("FS_Seek failed: %s\n", err.what());
			return -1;
		}
	}
}

void FS_ForceFlush(fileHandle_t handle)
{
	FS_CheckHandle(handle, true);
	handleTable[handle].forceFlush = true;
}

void FS_Flush(fileHandle_t handle)
{
	FS_CheckHandle(handle, true);
	try {
		handleTable[handle].file.Flush();
	} catch (std::system_error& err) {
		Com_Printf("FS_Flush failed: %s\n", err.what());
	}
}

int FS_Write(const void* buffer, int len, fileHandle_t handle)
{
	FS_CheckHandle(handle, true);
	try {
		handleTable[handle].file.Write(buffer, len);
		if (handleTable[handle].forceFlush)
			handleTable[handle].file.Flush();
		return len;
	} catch (std::system_error& err) {
		Com_Printf("FS_Write failed: %s\n", err.what());
		return 0;
	}
}

int FS_Read(void* buffer, int len, fileHandle_t handle)
{
	FS_CheckHandle(handle, false);
	if (handleTable[handle].isPakFile) {
		if (len < 0)
			Com_Error(ERR_DROP, "FS_Read: invalid length");
		if (handleTable[handle].filePos >= handleTable[handle].fileData.size())
			return 0;
		len = std::min<size_t>(len, handleTable[handle].fileData.size() - handleTable[handle].filePos);
		memcpy(buffer, handleTable[handle].fileData.data() + handleTable[handle].filePos, len);
		handleTable[handle].filePos += len;
		return len;
	} else {
		try {
			return handleTable[handle].file.Read(buffer, len);
		} catch (std::system_error& err) {
			Com_Printf("Failed to read file: %s\n", err.what());
			return 0;
		}
	}
}

void FS_Printf(fileHandle_t handle, const char* fmt, ...)
{
	va_list ap;
	char buffer[MAXPRINTMSG];

	va_start(ap, fmt);
	Q_vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	FS_Write(buffer, strlen(buffer), handle);
}

int FS_Delete(const char* path)
{
	try {
		FS::HomePath::DeleteFile(path);
	} catch (std::system_error& err) {
		Com_Printf("Failed to delete file '%s': %s\n", path, err.what());
	}
	return 0;
}

void FS_Rename(const char* from, const char* to)
{
	try {
		FS::HomePath::MoveFile(to, from);
	} catch (std::system_error& err) {
		Com_Printf("Failed to move '%s' to '%s': %s\n", from, to, err.what());
	}
}

void FS_SV_Rename(const char* from, const char* to)
{
	FS_Rename(from, to);
}

void FS_WriteFile(const char* path, const void* buffer, int size)
{
	try {
		FS::File f = FS::HomePath::OpenWrite(path);
		f.Write(buffer, size);
		f.Close();
	} catch (std::system_error& err) {
		Com_Printf("Failed to write file '%s': %s\n", path, err.what());
	}
}

int FS_ReadFile(const char* path, void** buffer)
{
	fileHandle_t handle;
	int length = FS_FOpenFileRead(path, &handle, qtrue);

	if (length < 0) {
		if (buffer)
			*buffer = nullptr;
		return -1;
	}

	if (buffer) {
		char* buf = new char[length + 1];
		*buffer = buf;
		FS_Read(buf, length, handle);
		buf[length] = '\0';
	}

	FS_FCloseFile(handle);
	return length;
}

void FS_FreeFile(void* buffer)
{
	char* buf = static_cast<char*>(buffer);
	delete[] buf;
}

char** FS_ListFiles(const char* directory, const char* extension, int* numFiles)
{
	std::vector<char*> files;
	bool dirsOnly = extension && !strcmp(extension, "/");

	try {
		for (const std::string& x: FS::PakPath::ListFiles(directory)) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			char* s = new char[x.size() + 1];
			memcpy(s, x.data(), x.size());
			s[x.size() - (x.back() == '/')] = '\0';
			files.push_back(s);
		}
	} catch (std::system_error&) {}
	try {
		for (const std::string& x: FS::HomePath::ListFiles(directory)) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			char* s = new char[x.size() + 1];
			memcpy(s, x.data(), x.size());
			s[x.size() - (x.back() == '/')] = '\0';
			files.push_back(s);
		}
	} catch (std::system_error&) {}

	*numFiles = files.size();
	char** list = new char*[files.size() + 1];
	std::copy(files.begin(), files.end(), list);
	list[files.size()] = nullptr;
	return list;
}

void FS_FreeFileList(char** list)
{
	if (!list)
		return;
	for (char** i = list; *i; i++)
		delete[] *i;
	delete[] list;
}

int FS_GetFileList(const char* path, const char* extension, char* listBuf, int bufSize)
{
	// Mods are not yet supported in the new filesystem
	if (!strcmp(path, "$modlist"))
		return 0;

	int numFiles;
	char** list = FS_ListFiles(path, extension, &numFiles);

	for (int i = 0; i < numFiles; i++) {
		int length = strlen(list[i]) + 1;
		if (bufSize < length) {
			FS_FreeFileList(list);
			return i;
		}
		memcpy(listBuf, list[i], length);
		listBuf += length;
		bufSize -= length;
	}

	FS_FreeFileList(list);
	return numFiles;
}

const char* FS_LoadedPaks()
{
	static char info[BIG_INFO_STRING];
	info[0] = '\0';
	for (const FS::PakInfo& x: FS::PakPath::GetLoadedPaks()) {
		if (!x.checksum)
			continue;
		if (info[0])
			Q_strcat(info, sizeof(info), " ");
		Q_strcat(info, sizeof(info), FS::MakePakName(x.name, x.version, x.checksum).c_str());
	}
	return info;
}

bool FS_LoadPak(const char* name)
{
	const FS::PakInfo* pak = FS::FindPak(name);
	if (!pak)
		return false;
	try {
		FS::PakPath::LoadPak(*pak);
		return true;
	} catch (std::system_error& err) {
		Com_Printf("Failed to load pak '%s': %s\n", name, err.what());
		return false;
	}
}

bool FS_LoadServerPaks(const char* paks)
{
	Cmd::Args args(paks);
	fs_missingPaks.clear();
	for (int i = 0; i < args.Argc(); i++) {
		std::string name, version;
		Opt::optional<uint32_t> checksum;
		if (!FS::ParsePakName(&*args.Argv(i).begin(), &*args.Argv(i).end(), name, version, checksum) || !checksum) {
			Com_Error(ERR_DROP, "Invalid pak reference from server: %s\n", args.Argv(i).c_str());
			continue;
		}

		// Keep track of all missing paks
		const FS::PakInfo* pak = FS::FindPak(name, version, *checksum);
		if (!pak)
			fs_missingPaks.emplace_back(std::move(name), std::move(version), *checksum);
		else {
			try {
				FS::PakPath::LoadPakExplicit(*pak, *checksum);
			} catch (std::system_error&) {
				fs_missingPaks.emplace_back(std::move(name), std::move(version), *checksum);
			}
		}
	}
	return fs_missingPaks.empty();
}

qboolean CL_WWWBadChecksum(const char *pakname);
qboolean FS_ComparePaks(char* neededpaks, int len, qboolean dlstring)
{
	*neededpaks = '\0';
	for (auto& x: fs_missingPaks) {
		if (dlstring) {
			Q_strcat(neededpaks, len, "@");
			Q_strcat(neededpaks, len, FS::MakePakName(std::get<0>(x), std::get<1>(x), std::get<2>(x)).c_str());
			Q_strcat(neededpaks, len, "@");
			std::string pakName = Str::Format("pkg/%s.pk3", FS::MakePakName(std::get<0>(x), std::get<1>(x)));
			if (FS::HomePath::FileExists(pakName))
				Q_strcat(neededpaks, len, va("pkg/%s.pk3", FS::MakePakName(std::get<0>(x), std::get<1>(x), std::get<2>(x)).c_str()));
			else
				Q_strcat(neededpaks, len, pakName.c_str());
		} else {
			Q_strcat(neededpaks, len, va("%s.pk3", FS::MakePakName(std::get<0>(x), std::get<1>(x)).c_str()));
			if (FS::FindPak(std::get<0>(x), std::get<1>(x))) {
				Q_strcat(neededpaks, len, " (local file exists with wrong checksum)");
#ifndef DEDICATED
				if (CL_WWWBadChecksum(FS::MakePakName(std::get<0>(x), std::get<1>(x), std::get<2>(x)).c_str())) {
					try {
						FS::HomePath::DeleteFile(Str::Format("pkg/%s.pk3", FS::MakePakName(std::get<0>(x), std::get<1>(x))));
					} catch (std::system_error&) {}
				}
#endif
			}
		}
	}
	return !fs_missingPaks.empty();
}

class WhichCmd: public Cmd::StaticCmd {
public:
	WhichCmd()
		: Cmd::StaticCmd("which", Cmd::SYSTEM, N_("shows which pak a file is in")) {}

	void Run(const Cmd::Args& args) const OVERRIDE
	{
		if (args.Argc() != 2) {
			PrintUsage(args, _("<file>"), "");
			return;
		}

		const std::string& filename = args.Argv(1);
		const FS::PakInfo* pak = FS::PakPath::LocateFile(filename);
		if (pak)
			Print(_( "File \"%s\" found in \"%s\"\n"), filename, pak->path);
		else
			Print(_("File not found: \"%s\"\n"), filename);
	}

	Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, Str::StringRef prefix) const OVERRIDE
	{
		if (argNum == 1) {
			return FS::PakPath::CompleteFilename(prefix, "", "", true, false);
		}

		return {};
	}
};
static WhichCmd WhichCmdRegistration;
