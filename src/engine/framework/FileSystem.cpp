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
#include "../../libs/minizip/unzip.h"
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
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
			return NULL;
		}
		if (!S_ISREG(st.st_mode)) {
			fclose(fd);
			errno = EISDIR;
			return NULL;
		}
	}

	// Set the close-on-exec flag
	if (fcntl(fileno(fd), F_SETFD, FD_CLOEXEC) == -1) {
		fclose(fd);
		return NULL;
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
	virtual const char* name() const noexcept override final
	{
		return "unzip";
	}
	virtual std::string message(int ev) const override final
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
static void SetErrorCodeFileNotFound(std::error_code& err)
{
#ifdef _WIN32
	SetErrorCode(err, ERROR_FILE_NOT_FOUND, std::system_category());
#else
	SetErrorCode(err, ENOENT, std::generic_category());
#endif
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
	DWORD len = GetModuleFileNameW(NULL, buffer, MAX_PATH);
	if (len == 0 || len >= MAX_PATH)
		return "";

	wchar_t* p = wcsrchr(buffer, L'\\');
	if (!p)
		return "";
	*p = L'\0';

	return Str::UTF16To8(buffer);
#elif defined(__linux__)
	struct stat st;
	if (lstat("/proc/self/exe", &st) == -1)
		return "";

	std::unique_ptr<char[]> out(new char[st.st_size + 1]);
	int len = readlink("/proc/self/exe", out.get(), st.st_size + 1);
	if (len != st.st_size)
		return "";
	out[st.st_size] = '\0';

	char* p = strrchr(out.get(), '/');
	if (!p)
		return "";
	*p = '\0';

	return out.get();
#elif defined(__APPLE__)
	uint32_t bufsize = 0;
	_NSGetExecutablePath(NULL, &bufsize);

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
	if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, buffer)))
		return "";
	return Str::UTF16To8(buffer) + "\\My Games\\Unvanquished";
#else
	const char *home = getenv("HOME");
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
	if (err)
		return false;
	f.Close(err);
	if (err)
		return false;
	RawPath::DeleteFile(fname, err);
	if (err)
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

	pakPaths.push_back(homePath);
	if (basePath != homePath)
		pakPaths.push_back(basePath);
	if (extraPath[0] && extraPath != basePath && extraPath != homePath)
		pakPaths.push_back(extraPath);

	RefreshPaks();
}

// Add a pak to the list of available paks
static void AddPak(pakType_t type, Str::StringRef filename, Str::StringRef basePath)
{
	std::string fullPath = Path::Build(basePath, filename);

	size_t suffixLen = type == PAK_DIR ? strlen(".pk3dir/") : strlen(".pk3");
	size_t nameStart = filename.rfind('/', suffixLen);
	if (nameStart == Str::StringRef::npos)
		nameStart = 0;
	else
		nameStart++;

	// Get the name of the package
	size_t underscore1 = filename.find('_', nameStart);
	if (underscore1 == Str::StringRef::npos) {
		Com_Printf("Invalid pak name (no version): %s\n", fullPath.c_str());
		return;
	}
	std::string name = filename.substr(0, underscore1);

	// Get the version of the package
	size_t underscore2 = filename.find('_', underscore1 + 1);
	std::string version;
	uint32_t checksum = 0;
	bool hasChecksum;
	if (underscore2 == Str::StringRef::npos) {
		version = filename.substr(underscore1 + 1, filename.size() - suffixLen - underscore1 - 1);
		hasChecksum = false;
	} else {
		// Get the optional checksum of the package
		version = filename.substr(underscore1 + 1, underscore2 - underscore1 - 1);
		hasChecksum = true;
		if (type == PAK_DIR) {
			Com_Printf("Invalid pak name (checksum not allowed on pk3dir): %s\n", fullPath.c_str());
			return;
		}
		if (underscore2 + 9 != filename.size() - suffixLen) {
			Com_Printf("Invalid pak name (bad checksum): %s\n", fullPath.c_str());
			return;
		}
		for (int i = 0; i < 8; i++) {
			char c = filename[underscore2 + 1 + i];
			if (!Str::cisxdigit(c)) {
				Com_Printf("Invalid pak name (bad checksum): %s\n", fullPath.c_str());
				return;
			}
			uint32_t hexValue = Str::cisdigit(c) ? c - '0' : Str::ctolower(c) - 'a' + 10;
			checksum = (checksum << 4) | hexValue;
		}
	}

	availablePaks.push_back({std::move(name), std::move(version), hasChecksum, checksum, type, std::move(fullPath)});
}

// Find all paks in the given path
static void FindPaks(Str::StringRef basePath, Str::StringRef subPath)
{
	std::string fullPath = Path::Build(basePath, subPath);
	try {
		for (auto& filename: RawPath::ListFiles(fullPath)) {
			if (Str::IsSuffix(".pk3", filename)) {
				AddPak(PAK_ZIP, Path::Build(subPath, filename), basePath);
			} else if (Str::IsSuffix(".pk3dir/", filename)) {
				AddPak(PAK_DIR, Path::Build(subPath, filename), basePath);
			} else if (Str::IsSuffix("/", filename))
				FindPaks(basePath, Path::Build(subPath, filename));
		}
	} catch (std::system_error& err) {
		// If there was an error reading a directory, just ignore it and go to
		// the next one.
		Com_Printf("Error reading directory %s: %s\n", fullPath.c_str(), err.what());
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
		FindPaks(Path::Build(path, "pkg"), "");

	// Sort the pak list for easy binary searching:
	// First sort by name, then by version.
	// For checksums, place files without checksums in front, so they are selected
	// before any pak with an explicit checksum.
	std::sort(availablePaks.begin(), availablePaks.end(), [](const PakInfo& a, const PakInfo& b) {
		int result = a.name.compare(b.name);
		if (result != 0)
			return result < 0;
		result = VersionCmp(a.version, b.version);
		if (result != 0)
			return result < 0;
		// Put packages without checksums at the end
		if (!a.hasChecksum)
			return false;
		if (!b.hasChecksum)
			return true;
		return a.checksum < b.checksum;
	});
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

bool Path::IsValid(Str::StringRef path, bool allowDir)
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

std::string Path::Build(Str::StringRef base, Str::StringRef path)
{
	if (base.empty())
		return path;
	if (path.empty())
		return base;

	std::string out = base;
	if (!isdirsep(out.back()))
		out.push_back('/');
	out.append(path.data(), path.size());
	return out;
}

std::string Path::DirName(Str::StringRef path)
{
	std::string out;
	if (path.empty())
		return out;

	// Trim to last slash, excluding any trailing slash
	size_t lastSlash = path.rfind('/', path.back() == '/');
	if (lastSlash != Str::StringRef::npos)
		out = path.substr(0, lastSlash);

	return out;
}

std::string Path::BaseName(Str::StringRef path)
{
	std::string out;
	if (path.empty())
		return out;

	// Trim from last slash, excluding any trailing slash
	size_t lastSlash = path.rfind('/', path.back() == '/');
	if (lastSlash != Str::StringRef::npos)
		out = path.substr(lastSlash + 1);

	return out;
}

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
			result = unzGetCurrentFileInfo64(zipFile, &fileInfo, filename, sizeof(filename), NULL, 0, NULL, 0);
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
		int result = unzGetCurrentFileInfo64(zipFile, &fileInfo, NULL, 0, NULL, 0, NULL, 0);
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

void PakNamespace::InternalLoadPak(const PakInfo& pak, bool verifyChecksum, uint32_t expectedChecksum, std::error_code& err)
{
	std::vector<std::pair<std::string, pakFileInfo_t>> fileList;
	uint32_t checksum;

	// Gather the list of files
	if (pak.type == PAK_DIR) {
		try {
			for (auto& filename: RawPath::ListFilesRecursive(pak.path))
				fileList.push_back({filename, {loadedPaks.size(), 0}});
		} catch (std::system_error& sysErr) {
			SetErrorCode(err, sysErr.code().value(), sysErr.code().category());
			return;
		}
	} else {
		// Open zip
		ZipArchive zipFile = ZipArchive::Open(pak.path, err);
		if (HaveError(err))
			return;

		// Get the file list and calculate the checksum of the package
		checksum = crc32(0, Z_NULL, 0);
		zipFile.ForEachFile([this, &checksum, &fileList](Str::StringRef filename, offset_t offset, uint32_t crc) {
			checksum = crc32(checksum, reinterpret_cast<const Bytef*>(&crc), sizeof(crc));
			fileList.push_back({filename, {loadedPaks.size(), offset}});
		}, err);
		if (HaveError(err))
			return;
	}

	// If an explicit checksum was requested, verify that the pak we loaded is the one we are expecting
	if (verifyChecksum && checksum != expectedChecksum) {
		SetErrorCodeZlib(err, UNZ_CRCERROR);
		return;
	}

	// Print a warning if the checksum doesn't match the one in the filename
	if (pak.hasChecksum && pak.checksum != checksum)
		Com_Printf("Pak checksum doesn't match filename: %s\n", pak.path.c_str());

	// Once we are sure no more errors can occur, update our data structures
	loadedPaks.push_back(pak);
	if (pak.type == PAK_DIR) {
		loadedPaks.back().hasChecksum = false;
	} else {
		loadedPaks.back().hasChecksum = true;
		loadedPaks.back().checksum = checksum;
	}
	for (auto& entry: fileList)
		fileMap.insert(std::move(entry));
}

void PakNamespace::LoadPak(Str::StringRef name, std::error_code& err)
{
	// Find the latest version with the matching name
	auto iter = std::upper_bound(availablePaks.begin(), availablePaks.end(), name, [](Str::StringRef name, const PakInfo& pakInfo) {
		return name < pakInfo.name;
	});

	if (iter == availablePaks.begin() || (iter - 1)->name != name) {
		SetErrorCodeFileNotFound(err);
		return;
	}

	InternalLoadPak(*(iter - 1), false, 0, err);
}

void PakNamespace::LoadPak(Str::StringRef name, Str::StringRef version, std::error_code& err)
{
	// Find a matching name and version, but prefer the last matching element since that is usually the one with no checksum
	auto iter = std::upper_bound(availablePaks.begin(), availablePaks.end(), name, [version](Str::StringRef name, const PakInfo& pakInfo) {
		int result = name.compare(pakInfo.name);
		if (result != 0)
			return result < 0;
		return VersionCmp(version, pakInfo.version) < 0;
	});

	if (iter == availablePaks.begin() || (iter - 1)->name != name || (iter - 1)->version != version) {
		SetErrorCodeFileNotFound(err);
		return;
	}

	InternalLoadPak(*(iter - 1), false, 0, err);
}

void PakNamespace::LoadPak(Str::StringRef name, Str::StringRef version, uint32_t checksum, std::error_code& err)
{
	// Try to find an exact match
	auto iter = std::upper_bound(availablePaks.begin(), availablePaks.end(), name, [version, checksum](Str::StringRef name, const PakInfo& pakInfo) {
		int result = name.compare(pakInfo.name);
		if (result != 0)
			return result < 0;
		result = VersionCmp(version, pakInfo.version);
		if (result != 0)
			return result < 0;
		if (!pakInfo.hasChecksum)
			return true;
		return checksum < pakInfo.checksum;
	});

	if (iter == availablePaks.begin() || (iter - 1)->name != name || (iter - 1)->version != version || !(iter - 1)->hasChecksum || (iter - 1)->checksum != checksum) {
		// Try again, but this time look for the pak without a checksum. We will verify the checksum later.
		iter = std::upper_bound(availablePaks.begin(), availablePaks.end(), name, [version](Str::StringRef name, const PakInfo& pakInfo) {
			int result = name.compare(pakInfo.name);
			if (result != 0)
				return result < 0;
			result = VersionCmp(version, pakInfo.version);
			if (result != 0)
				return result < 0;
			return false;
		});

		// Only allow zip packages because directories don't have a checksum
		if (iter == availablePaks.begin() || (iter - 1)->type == PAK_DIR || (iter - 1)->name != name || (iter - 1)->version != version || (iter - 1)->hasChecksum) {
			SetErrorCodeFileNotFound(err);
			return;
		}
	}

	InternalLoadPak(*(iter - 1), true, checksum, err);
}

std::string PakNamespace::ReadFile(Str::StringRef path, std::error_code& err) const
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFileNotFound(err);
		return nullptr;
	}

	const PakInfo& pak = loadedPaks[it->second.pakIndex];
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
		zipFile.OpenFile(it->second.offset, err);
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

void PakNamespace::CopyFile(Str::StringRef path, const File& dest, std::error_code& err) const
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFileNotFound(err);
		return;
	}

	const PakInfo& pak = loadedPaks[it->second.pakIndex];
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
		zipFile.OpenFile(it->second.offset, err);
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

bool PakNamespace::FileExists(Str::StringRef path) const
{
	return fileMap.find(path) != fileMap.end();
}

const PakInfo* PakNamespace::LocateFile(Str::StringRef path, std::error_code& err) const
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFileNotFound(err);
		return nullptr;
	} else {
		ClearErrorCode(err);
		return &loadedPaks[it->second.pakIndex];
	}
}

std::chrono::system_clock::time_point PakNamespace::FileTimestamp(Str::StringRef path, std::error_code& err) const
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFileNotFound(err);
		return {};
	}

	my_stat_t st;
	int result;
	const PakInfo& pak = loadedPaks[it->second.pakIndex];
	if (pak.type == PAK_DIR)
		result = my_stat(Path::Build(pak.path, path), &st);
	else
		result = my_stat(pak.path, &st);
	if (result != 0) {
		SetErrorCodeSystem(err);
		return {};
	} else {
		ClearErrorCode(err);
		return std::chrono::system_clock::from_time_t(std::max(st.st_ctime, st.st_mtime));
	}
}

bool PakNamespace::DirectoryRange::InternalAdvance()
{
	for (; iter != iter_end; ++iter) {
		// Filter out any paths not in the specified directory
		if (!Str::IsPrefix(prefix, iter->first))
			continue;

		// List immediate subdirectories only if not doing a recursive search
		if (!recursive) {
			auto end = iter->first.back() == '/' ? iter->first.end() - 1 : iter->first.end();
			auto p = std::find(iter->first.begin() + prefix.size(), end, '/');
			if (p == end)
				continue;
		}

		current = iter->first.substr(prefix.size());
		return true;
	}

	return false;
}

bool PakNamespace::DirectoryRange::Advance(std::error_code& err)
{
	++iter;
	ClearErrorCode(err);
	return InternalAdvance();
}

PakNamespace::DirectoryRange PakNamespace::ListFiles(Str::StringRef path, std::error_code& err) const
{
	DirectoryRange state;
	state.recursive = false;
	state.prefix = path;
	if (!state.prefix.empty() && state.prefix.back() != '/')
		state.prefix.push_back('/');
	state.iter = fileMap.cbegin();
	state.iter_end = fileMap.cend();
	if (!state.InternalAdvance())
		SetErrorCodeFileNotFound(err);
	else
		ClearErrorCode(err);
	return state;
}

PakNamespace::DirectoryRange PakNamespace::ListFilesRecursive(Str::StringRef path, std::error_code& err) const
{
	DirectoryRange state;
	state.recursive = true;
	state.prefix = path;
	if (!state.prefix.empty() && state.prefix.back() != '/')
		state.prefix.push_back('/');
	state.iter = fileMap.cbegin();
	state.iter_end = fileMap.cend();
	if (!state.InternalAdvance())
		SetErrorCodeFileNotFound(err);
	else
		ClearErrorCode(err);
	return state;
}

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
		if (_wmkdir(buffer.data() != 0 && errno != EEXIST) {
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
	if (dirPath.back() == '/')
		dirPath.pop_back();

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
		auto subdir = ListFiles(path, err);
		if (HaveError(err))
			return false;
		if (!subdir.empty()) {
			dirs.push_back(std::move(subdir));
			current.clear();
			for (auto& x: dirs)
				current.append(*x.begin());
			return true;
		} else
			current.pop_back();
	}

	while (!dirs.empty()) {
		size_t pos = current.rfind('/');
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
	if (!Path::IsValid(path, false))
		return {};
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
		SetErrorCodeFileNotFound(err);
		return {};
	}
	return RawPath::FileTimestamp(Path::Build(homePath, path), err);
}

void MoveFile(Str::StringRef dest, Str::StringRef src, std::error_code& err)
{
	if (!Path::IsValid(dest, false) || !Path::IsValid(src, false)) {
		SetErrorCodeFileNotFound(err);
		return;
	}
	RawPath::MoveFile(Path::Build(homePath, dest), Path::Build(homePath, src), err);
}
void DeleteFile(Str::StringRef path, std::error_code& err)
{
	if (!Path::IsValid(path, false)) {
		SetErrorCodeFileNotFound(err);
		return;
	}
	RawPath::DeleteFile(Path::Build(homePath, path), err);
}

DirectoryRange ListFiles(Str::StringRef path, std::error_code& err)
{
	if (!Path::IsValid(path, true)) {
		SetErrorCodeFileNotFound(err);
		return {};
	}
	return RawPath::ListFiles(Path::Build(homePath, path), err);
}

RecursiveDirectoryRange ListFilesRecursive(Str::StringRef path, std::error_code& err)
{
	if (!Path::IsValid(path, true)) {
		SetErrorCodeFileNotFound(err);
		return {};
	}
	return RawPath::ListFilesRecursive(Path::Build(homePath, path), err);
}

} // namespace HomePath

} // namespace FS
