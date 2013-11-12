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

// Clean up platform compatibility issues
static FILE* my_fopen(Str::StringRef path, char modeChar)
{
#ifdef _WIN32
	wchar_t mode[3] = L"xb";
	mode[0] = modeChar;
	return _wfopen(Str::UTF8To16(path).c_str(), mode);
#else
	char mode[3] = "xb";
	mode[0] = modeChar;
#if defined(__APPLE__)
	FILE* fd = fopen(path.c_str(), mode);
#elif defined(__linux__)
	FILE* fd = fopen64(path.c_str(), mode);
#endif

	// Only allow opening regular files
	if (fd) {
		struct stat st;
		fstat(fileno(fd), &st);
		if (!S_ISREG(st.st_mode)) {
			fclose(fd);
			errno = ENOENT;
			return NULL;
		}
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

// Determine path to the executable
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

// Test a directory for write permission
static bool TestWritePermission(Str::StringRef path)
{
	// Create a temporary file in the path and then delete it
	std::string fname = Path::Build(path, ".test_write_permission");
	FILE *file = my_fopen(fname, 'w');
	if (!file)
		return false;
	fclose(file);
#ifdef _WIN32
	DeleteFileW(Str::UTF8To16(fname).c_str());
#else
	unlink(fname.c_str());
#endif
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
		if (c >= 'a' && c <= 'z')
			continue;
		if (c >= 'A' && c <= 'Z')
			continue;
		if (c >= '0' && c <= '9')
			continue;
		if (nonAlphaNum)
			return false;
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
		return path.str();
	std::string out = base.str();
#ifdef _WIN32
	if (out.back() != '/' && out.back() != '\\')
#else
	if (out.back() != '/')
#endif
		out.push_back('/');
	out.append(path.data(), path.size());
	return out;
}

// Real file backed by a stdio handle
class OSFile: public File {
public:
	OSFile(std::string filename, FILE* fd)
		: File(std::move(filename)), fd(fd) {}
	~OSFile()
	{
		fclose(fd);
	}
	virtual offset_t Length(std::error_code& err) const override final
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
	virtual time_t Timestamp(std::error_code& err) const override final
	{
		my_stat_t st;
		if (my_fstat(fileno(fd), &st) != 0) {
			SetErrorCodeSystem(err);
			return 0;
		} else {
			ClearErrorCode(err);
			return std::max(st.st_ctime, st.st_mtime);
		}
	}
	virtual void SeekCur(offset_t off, std::error_code& err) override final
	{
		if (my_fseek(fd, off, SEEK_CUR) != 0)
			SetErrorCodeSystem(err);
		else
			ClearErrorCode(err);
	}
	virtual void SeekSet(offset_t off, std::error_code& err) override final
	{
		if (my_fseek(fd, off, SEEK_SET) != 0)
			SetErrorCodeSystem(err);
		else
			ClearErrorCode(err);
	}
	virtual void SeekEnd(offset_t off, std::error_code& err) override final
	{
		if (my_fseek(fd, off, SEEK_END) != 0)
			SetErrorCodeSystem(err);
		else
			ClearErrorCode(err);
	}
	virtual offset_t Tell() const override final
	{
		return my_ftell(fd);
	}
	virtual size_t Read(void* buffer, size_t length, std::error_code& err) override final
	{
		size_t result = fread(buffer, 1, length, fd);
		if (result != length && ferror(fd))
			SetErrorCodeSystem(err);
		else
			ClearErrorCode(err);
		return result;
	}
	virtual void Write(const void* data, size_t length, std::error_code& err) override final
	{
		if (fwrite(data, 1, length, fd) != length)
			SetErrorCodeSystem(err);
		else
			ClearErrorCode(err);
	}
	virtual void Flush(std::error_code& err) override final
	{
		if (fflush(fd) != 0)
			SetErrorCodeSystem(err);
		else
			ClearErrorCode(err);
	}

private:
	FILE* fd;
};

// File inside a zip archive, read-only
class ZipFile: public File {
public:
	ZipFile(std::string filename, unzFile zipFile, FILE* rawZipFile)
		: File(std::move(filename)), zipFile(zipFile), rawZipFile(rawZipFile) {}
	~ZipFile()
	{
		unzClose(zipFile);
	}

	virtual offset_t Length(std::error_code& err) const override final
	{
		unz_file_info64 fileInfo;
		int result = unzGetCurrentFileInfo64(zipFile, &fileInfo, NULL, 0, NULL, 0, NULL, 0);
		if (result != UNZ_OK) {
			SetErrorCodeZlib(err, result);
			return 0;
		} else {
			ClearErrorCode(err);
			return fileInfo.uncompressed_size;
		}
	}
	virtual time_t Timestamp(std::error_code& err) const override final
	{
		my_stat_t st;
		if (my_fstat(fileno(rawZipFile), &st) != 0) {
			SetErrorCodeSystem(err);
			return 0;
		} else {
			ClearErrorCode(err);
			return std::max(st.st_ctime, st.st_mtime);
		}
	}
	virtual void SeekCur(offset_t off, std::error_code& err) override final
	{
		SeekSet(Tell() + off, err);
	}
	virtual void SeekSet(offset_t off, std::error_code& err) override final
	{
		offset_t pos = unztell64(zipFile);
		if (off < pos) {
			pos = 0;
			int result = unzOpenCurrentFile(zipFile);
			if (result < 0) {
				SetErrorCodeZlib(err, result);
				return;
			}
		} else
			off -= pos;
		char seekBuffer[65536];
		while (off != 0) {
			size_t toRead = std::min<size_t>(sizeof(seekBuffer), off);
			int result = unzReadCurrentFile(zipFile, seekBuffer, toRead);
			if (result < 0) {
				SetErrorCodeZlib(err, result);
				return;
			}
			off -= toRead;
		}
		ClearErrorCode(err);
	}
	virtual void SeekEnd(offset_t off, std::error_code& err) override final
	{
		offset_t length = Length(err);
		if (HaveError(err))
			return;
		SeekSet(length + off, err);
	}
	virtual offset_t Tell() const override final
	{
		return unztell64(zipFile);
	}
	virtual size_t Read(void* buffer, size_t length, std::error_code& err) override final
	{
		int result = unzReadCurrentFile(zipFile, buffer, length);
		if (result < 0)
			SetErrorCodeZlib(err, result);
		else
			ClearErrorCode(err);
		return result;
	}
	virtual void Write(const void* data, size_t length, std::error_code& err) override final
	{
		Q_UNUSED(data);
		Q_UNUSED(length);
		SetErrorCode(err, EBADF, std::generic_category());
	}
	virtual void Flush(std::error_code& err) override final
	{
		Q_UNUSED(err);
	}

private:
	unzFile zipFile;
	FILE* rawZipFile;
};

// Open a zip file
static std::pair<unzFile, FILE*> OpenZip(Str::StringRef path, std::error_code& err)
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
	FILE* fd = my_fopen(path, 'r');
	if (!fd) {
		SetErrorCodeSystem(err);
		return {nullptr, nullptr};
	}

	// Open the zip with zlib
	unzFile zf = unzOpen2_64(fd, &funcs);
	if (!zf) {
		// Unfortunately unzOpen doesn't return an error code, so we assume UNZ_BADZIPFILE
		// Note that unzOpen will close the file if an error occurs
		SetErrorCodeZlib(err, UNZ_BADZIPFILE);
		return {nullptr, nullptr};
	}

	ClearErrorCode(err);
	return {zf, fd};
}

std::unique_ptr<File> PakNamespace::OpenRead(Str::StringRef path, std::error_code& err) const
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFileNotFound(err);
		return nullptr;
	}

	if (it->second.pak->type == PAK_DIR)
		return RawPath::OpenRead(Path::Build(it->second.pak->path, path), err);
	else {
		unzFile zipFile;
		FILE* rawZipFile;
		std::tie(zipFile, rawZipFile) = OpenZip(it->second.pak->path, err);
		if (HaveError(err))
			return nullptr;

		int result = unzSetOffset64(zipFile, it->second.offset);
		if (result != UNZ_OK) {
			unzClose(zipFile);
			SetErrorCodeZlib(err, result);
			return nullptr;
		}
		result = unzOpenCurrentFile(zipFile);
		if (result != UNZ_OK) {
			unzClose(zipFile);
			SetErrorCodeZlib(err, result);
			return nullptr;
		}

		return std::unique_ptr<File>(new ZipFile(path, zipFile, rawZipFile));
	}
}

bool PakNamespace::FileExists(Str::StringRef path) const
{
	return fileMap.find(path) != fileMap.end();
}

const LoadedPak* PakNamespace::LocateFile(Str::StringRef path, std::error_code& err) const
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFileNotFound(err);
		return nullptr;
	} else {
		ClearErrorCode(err);
		return it->second.pak;
	}
}

time_t PakNamespace::FileTimestamp(Str::StringRef path, std::error_code& err) const
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFileNotFound(err);
		return 0;
	}

	my_stat_t st;
	int result;
	if (it->second.pak->type == PAK_DIR)
		result = my_stat(Path::Build(it->second.pak->path, path), &st);
	else if (it->second.pak->type == PAK_ZIP)
		result = my_stat(it->second.pak->path, &st);
	if (result != 0) {
		SetErrorCodeSystem(err);
		return 0;
	} else {
		ClearErrorCode(err);
		return std::max(st.st_ctime, st.st_mtime);
	}
}

std::string File::ReadAll(std::error_code& err)
{
	offset_t length = Length(err);
	if (HaveError(err))
		return "";
	std::string out;
	out.resize(length);
	Read(&out[0], length, err);
	return out;
}

void File::InternalCopyTo(File* dest, std::error_code& err)
{
	char buffer[65536];
	while (true) {
		size_t read = Read(buffer, sizeof(buffer), err);
		if (HaveError(err) || read == 0)
			return;
		dest->Write(buffer, read, err);
		if (HaveError(err))
			return;
	}
}

template<bool recursive> bool PakNamespace::BasicDirectoryRange<recursive>::InternalAdvance()
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

template<bool recursive> bool PakNamespace::BasicDirectoryRange<recursive>::Advance(std::error_code& err)
{
	++iter;
	ClearErrorCode(err);
	return InternalAdvance();
}

PakNamespace::DirectoryRange PakNamespace::ListFiles(Str::StringRef path, std::error_code& err) const
{
	DirectoryRange state;
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

PakNamespace::RecursiveDirectoryRange PakNamespace::ListFilesRecursive(Str::StringRef path, std::error_code& err) const
{
	RecursiveDirectoryRange state;
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

static std::unique_ptr<File> OpenMode(Str::StringRef path, char mode, std::error_code& err)
{
	FILE* fd = my_fopen(path, mode);
	if (!fd) {
		SetErrorCodeSystem(err);
		return nullptr;
	} else {
		ClearErrorCode(err);
		return std::unique_ptr<File>(new OSFile(path, fd));
	}
}
std::unique_ptr<File> OpenRead(Str::StringRef path, std::error_code& err)
{
	return OpenMode(path, 'r', err);
}
std::unique_ptr<File> OpenWrite(Str::StringRef path, std::error_code& err)
{
	return OpenMode(path, 'w', err);
}
std::unique_ptr<File> OpenAppend(Str::StringRef path, std::error_code& err)
{
	return OpenMode(path, 'a', err);
}

bool FileExists(Str::StringRef path)
{
	my_stat_t st;
	return my_stat(path, &st) == 0;
}

time_t FileTimestamp(Str::StringRef path, std::error_code& err)
{
	my_stat_t st;
	if (my_stat(path, &st) != 0) {
		SetErrorCodeSystem(err);
		return 0;
	} else {
		ClearErrorCode(err);
		return std::max(st.st_ctime, st.st_mtime);
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
	if (rename(src.c_str(), dest.c_str()) != 0)
		SetErrorCodeSystem(err);
	else
		ClearErrorCode(err);
#endif
}
void DeleteFile(Str::StringRef path, std::error_code& err)
{
#ifdef _WIN32
	if (_wunlink(Str::UTF8To16(path).c_str()) != 0)
#else
	if (unlink(path.c_str()) != 0)
#endif
		SetErrorCodeSystem(err);
	else
		ClearErrorCode(err);
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
			return false;
		}
	} while (!Path::IsValid(dirent->d_name, false) || my_stat(Path::Build(path, dirent->d_name), &st) != 0);
	current = dirent->d_name;
	if (S_ISDIR(st.st_mode))
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

std::unique_ptr<File> OpenRead(Str::StringRef path, std::error_code& err)
{
	return RawPath::OpenMode(Path::Build(homePath, path), 'r', err);
}
std::unique_ptr<File> OpenWrite(Str::StringRef path, std::error_code& err)
{
	return RawPath::OpenMode(Path::Build(homePath, path), 'w', err);
}
std::unique_ptr<File> OpenAppend(Str::StringRef path, std::error_code& err)
{
	return RawPath::OpenMode(Path::Build(homePath, path), 'a', err);
}

bool FileExists(Str::StringRef path)
{
	if (!Path::IsValid(path, false))
		return false;
	return RawPath::FileExists(Path::Build(homePath, path));
}

time_t FileTimestamp(Str::StringRef path, std::error_code& err)
{
	if (!Path::IsValid(path, false)) {
		SetErrorCodeFileNotFound(err);
		return 0;
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
