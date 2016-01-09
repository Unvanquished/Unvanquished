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

#include "minizip/unzip.h"

#ifdef BUILD_VM
#include "shared/VMMain.h"
#else
#include "engine/qcommon/qcommon.h"
#endif

#include "IPC/CommonSyscalls.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
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

Log::Logger fsLogs(VM_STRING_PREFIX "fs", Log::LOG_NOTICE);

// SerializeTraits for PakInfo/LoadedPakInfo
namespace Util {

template<> struct SerializeTraits<FS::PakInfo> {
	static void Write(Writer& stream, const FS::PakInfo& value)
	{
		stream.Write<std::string>(value.name);
		stream.Write<std::string>(value.version);
		stream.Write<Util::optional<uint32_t>>(value.checksum);
		stream.Write<uint32_t>(value.type);
		stream.Write<std::string>(value.path);
	}
	static FS::PakInfo Read(Reader& stream)
	{
		FS::PakInfo value;
		value.name = stream.Read<std::string>();
		value.version = stream.Read<std::string>();
		value.checksum = stream.Read<Util::optional<uint32_t>>();
		value.type = static_cast<FS::pakType_t>(stream.Read<uint32_t>());
		value.path = stream.Read<std::string>();
		return value;
	}
};

template<> struct SerializeTraits<FS::LoadedPakInfo> {
	static void Write(Writer& stream, const FS::LoadedPakInfo& value)
	{
		stream.Write<std::string>(value.name);
		stream.Write<std::string>(value.version);
		stream.Write<Util::optional<uint32_t>>(value.checksum);
		stream.Write<uint32_t>(value.type);
		stream.Write<std::string>(value.path);
		stream.Write<Util::optional<uint32_t>>(value.realChecksum);
		stream.Write<uint64_t>(std::chrono::system_clock::to_time_t(value.timestamp));
		stream.Write<bool>(value.fd != -1);
		if (value.fd != -1)
			stream.Write<IPC::FileHandle>(IPC::FileHandle(value.fd, IPC::MODE_READ));
		stream.Write<std::string>(value.pathPrefix);
	}
	static FS::LoadedPakInfo Read(Reader& stream)
	{
		FS::LoadedPakInfo value;
		value.name = stream.Read<std::string>();
		value.version = stream.Read<std::string>();
		value.checksum = stream.Read<Util::optional<uint32_t>>();
		value.type = static_cast<FS::pakType_t>(stream.Read<uint32_t>());
		value.path = stream.Read<std::string>();
		value.realChecksum = stream.Read<Util::optional<uint32_t>>();
		value.timestamp = std::chrono::system_clock::from_time_t(stream.Read<uint64_t>());
		if (stream.Read<bool>())
			value.fd = stream.Read<IPC::FileHandle>().GetHandle();
		else
			value.fd = -1;
		value.pathPrefix = stream.Read<std::string>();
		return value;
	}
};

} // namespace Util

namespace FS {

// Pak zip and directory extensions
#define PAK_ZIP_EXT ".pk3"
#define PAK_DIR_EXT ".pk3dir/"

// Error variable used by throws(). This is never written to and always
// represents a success value.
std::error_code throw_err;

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
	MODE_APPEND,
	MODE_EDIT
};
inline int my_open(Str::StringRef path, openMode_t mode)
{
	int modes[] = {O_RDONLY, O_WRONLY | O_TRUNC | O_CREAT, O_WRONLY | O_APPEND | O_CREAT, O_RDWR | O_CREAT};
#ifdef _WIN32
	// Allow open files to be deleted & renamed
	DWORD access[] = {GENERIC_READ, GENERIC_WRITE, GENERIC_WRITE, GENERIC_READ | GENERIC_WRITE};
	DWORD create[] = {OPEN_EXISTING, CREATE_ALWAYS, OPEN_ALWAYS, OPEN_ALWAYS};
	HANDLE h = CreateFileW(Str::UTF8To16(path).c_str(), access[mode], FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, create[mode], FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		_doserrno = GetLastError();
		errno = _doserrno == ERROR_FILE_NOT_FOUND || _doserrno == ERROR_PATH_NOT_FOUND ? ENOENT : 0; // Needed to check if we need to create the path
		return -1;
	}
	int fd = _open_osfhandle(reinterpret_cast<intptr_t>(h), modes[mode] | O_BINARY | O_NOINHERIT);
	if (fd == -1)
		CloseHandle(h);
#elif defined(__APPLE__)
	// O_CLOEXEC is supported from 10.7 onwards
	int fd = open(path.c_str(), modes[mode] | O_CLOEXEC, 0666);
#elif defined(__linux__)
	int fd = open64(path.c_str(), modes[mode] | O_CLOEXEC | O_LARGEFILE, 0666);
#elif defined(__native_client__)
	// This doesn't actually work, but it's not used anyways
	int fd = open(path.c_str(), modes[mode], 0666);
#endif

#ifndef _WIN32
	// Don't allow opening directories
	if (mode == MODE_READ && fd != -1) {
		struct stat st;
		if (fstat(fd, &st) == -1) {
			close(fd);
			return -1;
		}
		if (S_ISDIR(st.st_mode)) {
			close(fd);
			errno = EISDIR;
			return -1;
		}
	}
#endif

	return fd;
}
inline FILE* my_fopen(Str::StringRef path, openMode_t mode)
{
	int fd = my_open(path, mode);
	if (fd == -1)
		return nullptr;

	const char* modes[] = {"rb", "wb", "ab", "rb+"};
	FILE* fp = fdopen(fd, modes[mode]);

	if (!fp)
		close(fd);
	return fp;
}
inline offset_t my_ftell(FILE* fd)
{
#ifdef _WIN32
	return _ftelli64(fd);
#elif defined(__APPLE__) || defined(__native_client__)
	return ftello(fd);
#elif defined(__linux__)
	return ftello64(fd);
#endif
}
inline int my_fseek(FILE* fd, offset_t off, int whence)
{
#ifdef _WIN32
	return _fseeki64(fd, off, whence);
#elif defined(__APPLE__) || defined(__native_client__)
	return fseeko(fd, off, whence);
#elif defined(__linux__)
	return fseeko64(fd, off, whence);
#endif
}
#ifdef _WIN32
typedef struct _stati64 my_stat_t;
#elif defined(__APPLE__) || defined(__native_client__)
typedef struct stat my_stat_t;
#elif defined(__linux__)
typedef struct stat64 my_stat_t;
#endif
inline int my_fstat(int fd, my_stat_t* st)
{
#ifdef _WIN32
	return _fstati64(fd, st);
#elif defined(__APPLE__) || defined(__native_client__)
	return fstat(fd, st);
#elif defined(__linux__)
	return fstat64(fd, st);
#endif
}
inline int my_stat(Str::StringRef path, my_stat_t* st)
{
#ifdef _WIN32
	return _wstati64(Str::UTF8To16(path).c_str(), st);
#elif defined(__APPLE__) || defined(__native_client__)
	return stat(path.c_str(), st);
#elif defined(__linux__)
	return stat64(path.c_str(), st);
#endif
}
inline intptr_t my_pread(int fd, void* buf, size_t count, offset_t offset)
{
#ifdef _WIN32
	OVERLAPPED overlapped;
	DWORD bytesRead;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.Offset = offset & 0xffffffff;
	overlapped.OffsetHigh = offset >> 32;
	if (!ReadFile(reinterpret_cast<HANDLE>(_get_osfhandle(fd)), buf, count, &bytesRead, &overlapped)) {
		_doserrno = GetLastError();
		return -1;
	}
	return bytesRead;
#elif defined(__APPLE__) || defined(__native_client__)
	return pread(fd, buf, count, offset);
#elif defined(__linux__)
	return pread64(fd, buf, count, offset);
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
	no_filesystem_error,
	invalid_filename,
	no_such_file,
	no_such_directory,
	wrong_pak_checksum,
	missing_dependency
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
		case filesystem_error::missing_dependency:
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
static void SetErrorCodeSystem(std::error_code& err)
{
#ifdef _WIN32
	SetErrorCode(err, _doserrno, std::system_category());
#else
	SetErrorCode(err, errno, std::generic_category());
#endif
}
static void SetErrorCodeFilesystem(std::error_code& err, filesystem_error ec)
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

// Determine whether a character is a OS-dependent path separator
inline bool isdirsep(unsigned int c)
{
#ifdef _WIN32
	return c == '/' || c == '\\';
#else
	return c == '/';
#endif
}

namespace Path {

bool IsValid(Str::StringRef path, bool allowDir)
{
	char prev = '/';
	for (char c: path) {
		// Check if the character is allowed
		if (!Str::cisalnum(c) && c != '/' && c != '-' && c != '_' && c != '.' && c != '+' && c != '~')
			return false;

		// Disallow 2 consecutive / or .
		if ((c == '/' || c == '.') && (prev == '/' || prev == '.'))
			return false;

		prev = c;
	}

	// An empty path or a path ending with / is a directory
	if (prev == '/' && !allowDir)
		return false;

	// Disallow paths ending with .
	if (prev == '.')
		return false;

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
	if (path.empty())
		return "";

	// Trim to last slash, excluding any trailing slash
	size_t lastSlash = path.rfind('/', path.size() - 2);
	if (lastSlash != Str::StringRef::npos)
		return path.substr(0, lastSlash);
	else
		return "";
}

std::string BaseName(Str::StringRef path)
{
	if (path.empty())
		return "";

	// Trim from last slash, excluding any trailing slash
	size_t lastSlash = path.rfind('/', path.size() - 2);
	return path.substr(lastSlash + 1);
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
	std::string out;
	offset_t length = Length(err);
	if (err)
		return out;
	out.resize(length);
	auto actual = Read(&out[0], length, err);
	// The size of data read may not always match the file size even if just obtained.
	// One reason is that if a file is opened in text mode on Windows \r\n's
	// are translated into shorter \n on read/fread resulting in an overall
	// smaller string in memory than what is on disk.
	// So we resize the string to lose the excess data and ensure the
	// string's length() matches it's valid content as actually read.
	out.resize(actual);
	return out;
}
void File::CopyTo(const File& dest, std::error_code& err) const
{
	char buffer[65536];
	while (true) {
		size_t read = Read(buffer, sizeof(buffer), err);
		if (err || read == 0)
			return;
		dest.Write(buffer, read, err);
		if (err)
			return;
	}
}
void File::SetLineBuffered(bool enable, std::error_code& err) const
{
	if (setvbuf(fd, nullptr, enable ? _IOLBF : _IOFBF, BUFSIZ) != 0)
		SetErrorCodeSystem(err);
	else
		ClearErrorCode(err);
}

#ifdef BUILD_VM
// Convert an IPC file handle to a File object
static File FileFromIPC(Util::optional<IPC::OwnedFileHandle> ipcFile, openMode_t mode, std::error_code& err)
{
	if (!ipcFile) {
		SetErrorCodeFilesystem(err, filesystem_error::no_such_file);
		return File();
	}

	int fd = ipcFile->GetHandle();

	const char* modes[] = {"rb", "wb", "ab", "rb+"};
	FILE* fp = fdopen(fd, modes[mode]);
	if (!fp) {
		close(fd);
		SetErrorCodeSystem(err);
		return File();
	}

	return File(fp);
}
#endif

#ifdef BUILD_ENGINE
// Convert a File object to an ipc file handle
static IPC::OwnedFileHandle FileToIPC(File file, openMode_t mode)
{
	IPC::FileOpenMode ipcMode = IPC::MODE_READ;
	switch (mode) {
	case MODE_READ:
		ipcMode = IPC::MODE_READ;
		break;
	case MODE_WRITE:
		ipcMode = IPC::MODE_WRITE;
		break;
	case MODE_APPEND:
		ipcMode = IPC::MODE_WRITE_APPEND;
		break;
	case MODE_EDIT:
		ipcMode = IPC::MODE_RW;
		break;
	}

	int newfd = dup(fileno(file.GetHandle()));
	if (newfd == -1)
		return IPC::OwnedFileHandle();
	return IPC::OwnedFileHandle(newfd, ipcMode);
}
#endif

// Workaround for GCC 4.7.2 bug: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=55015
namespace {

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

	// Open an archive from an existing file descriptor
	static ZipArchive Open(int fd, std::error_code& err)
	{
		// Initialize the zlib I/O functions
		zlib_filefunc64_def funcs;
		struct zipData_t {
			int fd;
			char buffer[1024];
			offset_t pos;
			offset_t bufferPos;
			offset_t bufferLen;
			offset_t fileLen;
		};
		funcs.zopen64_file = [](voidpf opaque, const void* filename, int mode) -> voidpf {
			// Just forward the filename as the stream handle
			Q_UNUSED(opaque);
			Q_UNUSED(mode);
			return const_cast<void*>(filename);
		};
		funcs.zread_file = [](voidpf opaque, voidpf stream, void* buf, uLong size) -> uLong {
			Q_UNUSED(opaque);
			zipData_t* zipData = static_cast<zipData_t*>(stream);

			// Use pread directly for large reads
			if (size > sizeof(zipData->buffer)) {
				intptr_t result = my_pread(zipData->fd, buf, size, zipData->pos);
				if (result == -1)
					return 0;
				zipData->pos += result;
				return result;
			}

			// Refill the buffer if the request can't be satisfied from it
			if (zipData->pos < zipData->bufferPos || zipData->pos + (long) size > zipData->bufferPos + zipData->bufferLen) {
				intptr_t result = my_pread(zipData->fd, zipData->buffer, sizeof(zipData->buffer), zipData->pos);
				if (result == -1)
					return 0;
				zipData->bufferPos = zipData->pos;
				zipData->bufferLen = result;
			}

			// Read from the buffer, but handle short reads
			size_t offset = zipData->pos - zipData->bufferPos;
			size_t readLen = zipData->bufferLen - offset < size ? zipData->bufferLen - offset : size;
			memcpy(buf, zipData->buffer + offset, readLen);
			zipData->pos += readLen;
			return readLen;
		};
		funcs.zwrite_file = nullptr; // Writing to zip files is not supported
		funcs.ztell64_file = [](voidpf opaque, voidpf stream) -> ZPOS64_T {
			Q_UNUSED(opaque);
			zipData_t* zipData = static_cast<zipData_t*>(stream);
			return zipData->pos;
		};
		funcs.zseek64_file = [](voidpf opaque, voidpf stream, ZPOS64_T offset, int origin) -> long {
			Q_UNUSED(opaque);
			zipData_t* zipData = static_cast<zipData_t*>(stream);
			switch (origin) {
			case ZLIB_FILEFUNC_SEEK_CUR:
				zipData->pos += offset;
				break;
			case ZLIB_FILEFUNC_SEEK_END:
				zipData->pos = zipData->fileLen + offset;
				break;
			case ZLIB_FILEFUNC_SEEK_SET:
				zipData->pos = offset;
				break;
			default:
				errno = EINVAL;
				return -1;
			}
			return 0;
		};
		funcs.zclose_file = [](voidpf opaque, voidpf stream) -> int {
			Q_UNUSED(opaque);
			zipData_t* zipData = static_cast<zipData_t*>(stream);
			delete zipData;
			return 0;
		};
		funcs.zerror_file = [](voidpf opaque, voidpf stream) -> int {
			Q_UNUSED(opaque);
			zipData_t* zipData = static_cast<zipData_t*>(stream);
			return zipData->pos >= zipData->fileLen;
		};

		// Get the file length
		my_stat_t st;
		if (my_fstat(fd, &st) == -1) {
			SetErrorCodeSystem(err);
			return ZipArchive();
		}

		// Open the zip with zlib
		zipData_t* zipData = new zipData_t;
		zipData->fd = fd;
		zipData->pos = 0;
		zipData->bufferPos = 0;
		zipData->bufferLen = 0;
		zipData->fileLen = st.st_size;
		unzFile zipFile = unzOpen2_64(zipData, &funcs);
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
			size_t currentRead = std::min<size_t>(length - read, INT_MAX);
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

} // GCC bug workaround

namespace PakPath {

// List of loaded pak files
static std::vector<LoadedPakInfo> loadedPaks;

// Guard object to ensure that the fds in loadedPaks are closed on shutdown
struct LoadedPakGuard {
	~LoadedPakGuard() {
		for (LoadedPakInfo& x: loadedPaks) {
			if (x.fd != -1)
				close(x.fd);
		}
	}
};
static LoadedPakGuard loadedPaksGuard;

// Map of filenames to pak files. The size_t is an offset into loadedPaks and
// the offset_t is the position within the zip archive (unused for PAK_DIR).
static std::unordered_map<std::string, std::pair<uint32_t, offset_t>> fileMap;

#ifndef BUILD_VM
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
				fsLogs.Warn("Could not find pak '%s' required by '%s'", name, parent.path);
				SetErrorCodeFilesystem(err, filesystem_error::missing_dependency);
				return;
			}
			LoadPak(*pak, err);
			if (err)
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
				fsLogs.Warn("Could not find pak '%s' with version '%s' required by '%s'", name, version, parent.path);
				SetErrorCodeFilesystem(err, filesystem_error::missing_dependency);
				return;
			}
			LoadPak(*pak, err);
			if (err)
				return;
			lineStart = lineEnd == depsData.end() ? lineEnd : lineEnd + 1;
			continue;
		}

		// If there is still stuff at the end of the line, print a warning and ignore it
		fsLogs.Warn("Invalid dependency specification on line %d in %s", line, Path::Build(parent.path, PAK_DEPS_FILE));
		lineStart = lineEnd == depsData.end() ? lineEnd : lineEnd + 1;
	}
}

static void InternalLoadPak(const PakInfo& pak, Util::optional<uint32_t> expectedChecksum, Str::StringRef pathPrefix, std::error_code& err)
{
	Util::optional<uint32_t> realChecksum;
	bool hasDeps = false;
	offset_t depsOffset;
	ZipArchive zipFile;

	// Check if this pak has already been loaded to avoid recursive dependencies
	for (auto& x: loadedPaks) {
		// If the prefix is a superset of our current prefix, then it already
		// includes all the files we care about.
		if (x.path == pak.path && Str::IsPrefix(x.pathPrefix, pathPrefix))
			return;
	}

	// Add the pak to the list of loaded paks
	fsLogs.Notice("Loading pak '%s'...", pak.path.c_str());
	loadedPaks.emplace_back();
	loadedPaks.back().name = pak.name;
	loadedPaks.back().version = pak.version;
	loadedPaks.back().checksum = pak.checksum;
	loadedPaks.back().type = pak.type;
	loadedPaks.back().path = pak.path;

	// Update the list of files, but don't overwrite existing files, so the sort order is preserved
	if (pak.type == PAK_DIR) {
		loadedPaks.back().fd = -1;
		auto dirRange = RawPath::ListFilesRecursive(pak.path, err);
		if (err)
			return;
		for (auto it = dirRange.begin(); it != dirRange.end();) {
			if (*it == PAK_DEPS_FILE)
				hasDeps = true;
			else if (!Str::IsSuffix("/", *it) && Str::IsPrefix(pathPrefix, *it)) {
#ifdef LIBSTDCXX_BROKEN_CXX11
				fileMap.insert({*it, std::pair<uint32_t, offset_t>(loadedPaks.size() - 1, 0)});
#else
				fileMap.emplace(*it, std::pair<uint32_t, offset_t>(loadedPaks.size() - 1, 0));
#endif
			}
			it.increment(err);
			if (err)
				return;
		}
	} else {
		// Open file
		loadedPaks.back().fd = my_open(pak.path, MODE_READ);
		if (loadedPaks.back().fd == -1) {
			SetErrorCodeSystem(err);
			return;
		}

		// Open zip
		zipFile = ZipArchive::Open(loadedPaks.back().fd, err);
		if (err)
			return;

		// Get the file list and calculate the checksum of the package (checksum of all file checksums)
		realChecksum = crc32(0, Z_NULL, 0);
		zipFile.ForEachFile([&pak, &realChecksum, &pathPrefix, &hasDeps, &depsOffset](Str::StringRef filename, offset_t offset, uint32_t crc) {
			// Note that 'return' is effectively 'continue' since we are in a lambda
			if (!Str::IsPrefix(pathPrefix, filename) && filename != PAK_DEPS_FILE)
				return;
			if (Str::IsSuffix("/", filename))
				return;
			if (!Path::IsValid(filename, false)) {
				fsLogs.Warn("Invalid filename '%s' in pak '%s'", filename, pak.path);
				return;
			}
			realChecksum = crc32(*realChecksum, reinterpret_cast<const Bytef*>(&crc), sizeof(crc));
			if (filename == PAK_DEPS_FILE) {
				hasDeps = true;
				depsOffset = offset;
				return;
			}
#ifdef LIBSTDCXX_BROKEN_CXX11
			fileMap.insert({filename, std::pair<uint32_t, offset_t>(loadedPaks.size() - 1, offset)});
#else
			fileMap.emplace(filename, std::pair<uint32_t, offset_t>(loadedPaks.size() - 1, offset));
#endif
		}, err);
		if (err)
			return;

		// Get the timestamp of the pak
		loadedPaks.back().timestamp = FS::RawPath::FileTimestamp(pak.path, err);
		if (err)
			return;
	}

	// Save the real checksum in the list of loaded paks (empty for directories)
	loadedPaks.back().realChecksum = realChecksum;
	loadedPaks.back().pathPrefix = pathPrefix;

	// If an explicit checksum was requested, verify that the pak we loaded is the one we are expecting
	if (expectedChecksum && realChecksum != *expectedChecksum) {
		SetErrorCodeFilesystem(err, filesystem_error::wrong_pak_checksum);
		return;
	}

	// Print a warning if the checksum doesn't match the one in the filename
	if (pak.checksum && *pak.checksum != realChecksum)
		fsLogs.Warn("Pak checksum doesn't match filename: %s", pak.path);

	// Load dependencies, but not if a checksum was specified
	if (hasDeps && !expectedChecksum) {
		std::string depsData;
		if (pak.type == PAK_DIR) {
			File depsFile = RawPath::OpenRead(Path::Build(pak.path, PAK_DEPS_FILE), err);
			if (err)
				return;
			depsData = depsFile.ReadAll(err);
			if (err)
				return;
		} else {
			zipFile.OpenFile(depsOffset, err);
			if (err)
				return;
			offset_t length = zipFile.FileLength(err);
			if (err)
				return;
			depsData.resize(length);
			auto read = zipFile.ReadFile(&depsData[0], length, err);
			depsData.resize(read);
			if (err)
				return;
		}
		ParseDeps(pak, depsData, err);
	}
}

void LoadPak(const PakInfo& pak, std::error_code& err)
{
	InternalLoadPak(pak, Util::nullopt, "", err);
}

void LoadPakPrefix(const PakInfo& pak, Str::StringRef pathPrefix, std::error_code& err)
{
	InternalLoadPak(pak, Util::nullopt, pathPrefix, err);
}

void LoadPakExplicit(const PakInfo& pak, uint32_t expectedChecksum, std::error_code& err)
{
	InternalLoadPak(pak, expectedChecksum, "", err);
}

void ClearPaks()
{
	fileMap.clear();
	for (LoadedPakInfo& x: loadedPaks) {
		if (x.fd != -1)
			close(x.fd);
	}
	loadedPaks.clear();
	FS::RefreshPaks();
}
#else // BUILD_VM
void LoadPak(const PakInfo& pak, std::error_code&)
{
	VM::SendMsg<VM::FSPakPathLoadPakMsg>(&pak - availablePaks.data(), Util::nullopt, "");
}

void LoadPakPrefix(const PakInfo& pak, Str::StringRef pathPrefix, std::error_code&)
{
	VM::SendMsg<VM::FSPakPathLoadPakMsg>(&pak - availablePaks.data(), Util::nullopt, pathPrefix);
}

void LoadPakExplicit(const PakInfo& pak, uint32_t expectedChecksum, std::error_code&)
{
	VM::SendMsg<VM::FSPakPathLoadPakMsg>(&pak - availablePaks.data(), expectedChecksum, "");
}
#endif // BUILD_VM

const std::vector<LoadedPakInfo>& GetLoadedPaks()
{
	return loadedPaks;
}

std::string ReadFile(Str::StringRef path, std::error_code& err)
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFilesystem(err, filesystem_error::no_such_file);
		return "";
	}

	const LoadedPakInfo& pak = loadedPaks[it->second.first];
	if (pak.type == PAK_DIR) {
		// Open file
#ifdef BUILD_VM
		Util::optional<IPC::OwnedFileHandle> handle;
		VM::SendMsg<VM::FSPakPathOpenMsg>(it->second.first, path, handle);
		File file = FileFromIPC(std::move(handle), MODE_READ, err);
#else
		File file = RawPath::OpenRead(Path::Build(pak.path, path), err);
#endif
		if (err)
			return "";

		// Get file length
		offset_t length = file.Length(err);
		if (err)
			return "";

		// Read file contents
		std::string out;
		out.resize(length);
		file.Read(&out[0], length, err);
		return out;
	} else {
		// Open zip
		ZipArchive zipFile = ZipArchive::Open(pak.fd, err);
		if (err)
			return "";

		// Open file in zip
		zipFile.OpenFile(it->second.second, err);
		if (err)
			return "";

		// Get file length
		offset_t length = zipFile.FileLength(err);
		if (err)
			return "";

		// Read file
		std::string out;
		out.resize(length);
		zipFile.ReadFile(&out[0], length, err);
		if (err)
			return "";

		// Close file and check for CRC errors
		zipFile.CloseFile(err);
		if (err)
			return "";

		return out;
	}
}

void CopyFile(Str::StringRef path, const File& dest, std::error_code& err)
{
	auto it = fileMap.find(path);
	if (it == fileMap.end()) {
		SetErrorCodeFilesystem(err, filesystem_error::no_such_file);
		return;
	}

	const LoadedPakInfo& pak = loadedPaks[it->second.first];
	if (pak.type == PAK_DIR) {
#ifdef BUILD_VM
		Util::optional<IPC::OwnedFileHandle> handle;
		VM::SendMsg<VM::FSPakPathOpenMsg>(it->second.first, path, handle);
		File file = FileFromIPC(std::move(handle), MODE_READ, err);
#else
		File file = RawPath::OpenRead(Path::Build(pak.path, path), err);
#endif
		if (err)
			return;
		file.CopyTo(dest, err);
	} else {
		// Open zip
		ZipArchive zipFile = ZipArchive::Open(pak.fd, err);
		if (err)
			return;

		// Open file in zip
		zipFile.OpenFile(it->second.second, err);
		if (err)
			return;

		// Copy contents into destination
		char buffer[65536];
		while (true) {
			offset_t read = zipFile.ReadFile(buffer, sizeof(buffer), err);
			if (err)
				return;
			if (read == 0)
				break;
			dest.Write(buffer, read, err);
			if (err)
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

const LoadedPakInfo* LocateFile(Str::StringRef path)
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
		SetErrorCodeFilesystem(err, filesystem_error::no_such_file);
		return {};
	}

	const LoadedPakInfo& pak = loadedPaks[it->second.first];
	if (pak.type == PAK_DIR) {
#ifdef BUILD_VM
		Util::optional<uint64_t> result;
		VM::SendMsg<VM::FSPakPathTimestampMsg>(it->second.first, path, result);
		if (result) {
			ClearErrorCode(err);
			return std::chrono::system_clock::from_time_t(*result);
		} else {
			SetErrorCodeFilesystem(err, filesystem_error::no_such_file);
			return {};
		}
#else
		return RawPath::FileTimestamp(Path::Build(pak.path, path), err);
#endif
	} else
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
		SetErrorCodeFilesystem(err, filesystem_error::no_such_directory);
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
		SetErrorCodeFilesystem(err, filesystem_error::no_such_directory);
	else
		ClearErrorCode(err);
	return state;
}

// Note that this function is practically identical to the HomePath version.
// Try to keep any changes in sync between the two versions.
Cmd::CompletionResult CompleteFilename(Str::StringRef prefix, Str::StringRef root, Str::StringRef extension, bool allowSubdirs, bool stripExtension)
{
	// Handle prefixes ending with / so that they search inside the directory
	std::string prefixDir, prefixBase;
	if (Str::IsSuffix("/", prefix)) {
		if (!allowSubdirs)
			return {};
		prefixDir = prefix;
		prefixBase = "";
	} else {
		prefixDir = Path::DirName(prefix);
		if (!allowSubdirs && !prefixDir.empty())
			return {};
		prefixBase = Path::BaseName(prefix);
	}

	try {
		Cmd::CompletionResult out;
		// ListFiles doesn't return directories for PakPath, so use a recursive
		// search to get directory names.
		for (auto x: PakPath::ListFilesRecursive(Path::Build(root, prefixDir))) {
			std::string ext = Path::Extension(x);
			size_t slash = x.find('/');
			if (!allowSubdirs && slash != std::string::npos)
				continue;
			else if (allowSubdirs && slash != std::string::npos)
				x = x.substr(0, slash + 1);
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

#ifndef BUILD_VM
namespace RawPath {

void CreatePathTo(Str::StringRef path, std::error_code& err)
{
#ifdef _WIN32
	std::wstring buffer = Str::UTF8To16(path);
#else
	std::string buffer = path;
#endif

	// Skip drive letters and leading slashes
	size_t offset = 0;
#ifdef _WIN32
	if (buffer.size() >= 2 && Str::cisalpha(buffer[0]) && buffer[1] == ':')
		offset = 2;
#endif
	while (offset != buffer.size() && isdirsep(buffer[offset]))
		offset++;

	for (auto it = buffer.begin() + offset; it != buffer.end(); ++it) {
		if (!isdirsep(*it))
			continue;
		*it = '\0';

		// If the directory already exists, continue
#ifdef _WIN32
		DWORD attribs = GetFileAttributesW(buffer.data());
		if (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY)) {
			*it = '/';
			continue;
		}
#else
		my_stat_t st;
		if (my_stat(buffer.data(), &st) == 0 && S_ISDIR(st.st_mode)) {
			*it = '/';
			continue;
		}
#endif

		// Attempt to create the directory
#ifdef _WIN32
		if (_wmkdir(buffer.data()) != 0 && errno != EEXIST) {
			SetErrorCodeSystem(err);
			return;
		}
#else
		// Create the directory as private to the current user by default. The
		// user can relax the permissions later if he wishes.
		if (mkdir(buffer.data(), 0700) != 0 && errno != EEXIST) {
			SetErrorCodeSystem(err);
			return;
		}
#endif

		*it = '/';
	}
	ClearErrorCode(err);
}

static File OpenMode(Str::StringRef path, openMode_t mode, std::error_code& err)
{
	FILE* fd = my_fopen(path, mode);
	if (!fd && mode != MODE_READ && errno == ENOENT) {
		// Create the directories and try again
		CreatePathTo(path, err);
		if (err)
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
File OpenEdit(Str::StringRef path, std::error_code& err)
{
	return OpenMode(path, MODE_EDIT, err);
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
		if (err)
			return;
		File destFile = OpenWrite(src, err);
		if (err)
			return;
		srcFile.CopyTo(destFile, err);
		if (err)
			return;
		destFile.Close(err);
		if (err)
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
#ifdef LIBSTDCXX_BROKEN_CXX11
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
		if (err)
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
		if (err)
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
	if (err || root.begin() == root.end())
		return {};
	state.current = *root.begin();
	state.dirs.push_back(std::move(root));
	return state;
}

} // namespace RawPath
#endif // BUILD_VM

namespace HomePath {

static File OpenMode(Str::StringRef path, openMode_t mode, std::error_code& err)
{
#ifdef BUILD_VM
	Util::optional<IPC::OwnedFileHandle> handle;
	VM::SendMsg<VM::FSHomePathOpenModeMsg>(path, mode, handle);
	File file = FileFromIPC(std::move(handle), mode, err);
	if (err)
		return {};
	return file;
#else
	if (!Path::IsValid(path, false)) {
		SetErrorCodeFilesystem(err, filesystem_error::invalid_filename);
		return {};
	}
	return RawPath::OpenMode(Path::Build(homePath, path), mode, err);
#endif
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
File OpenEdit(Str::StringRef path, std::error_code& err)
{
	return OpenMode(path, MODE_EDIT, err);
}

bool FileExists(Str::StringRef path)
{
#ifdef BUILD_VM
	bool result;
	VM::SendMsg<VM::FSHomePathFileExistsMsg>(path, result);
	return result;
#else
	if (!Path::IsValid(path, false))
		return false;
	return RawPath::FileExists(Path::Build(homePath, path));
#endif
}

std::chrono::system_clock::time_point FileTimestamp(Str::StringRef path, std::error_code& err)
{
#ifdef BUILD_VM
	Util::optional<uint64_t> result;
	VM::SendMsg<VM::FSHomePathTimestampMsg>(path, result);
	if (result) {
		ClearErrorCode(err);
		return std::chrono::system_clock::from_time_t(*result);
	} else {
		SetErrorCodeFilesystem(err, filesystem_error::no_such_file);
		return {};
	}
#else
	if (!Path::IsValid(path, false)) {
		SetErrorCodeFilesystem(err, filesystem_error::invalid_filename);
		return {};
	}
	return RawPath::FileTimestamp(Path::Build(homePath, path), err);
#endif
}

void MoveFile(Str::StringRef dest, Str::StringRef src, std::error_code& err)
{
#ifdef BUILD_VM
	bool success;
	VM::SendMsg<VM::FSHomePathMoveFileMsg>(dest, src, success);
	if (success)
		ClearErrorCode(err);
	else
		SetErrorCodeFilesystem(err, filesystem_error::no_such_file);
#else
	if (!Path::IsValid(dest, false) || !Path::IsValid(src, false)) {
		SetErrorCodeFilesystem(err, filesystem_error::invalid_filename);
		return;
	}
	RawPath::MoveFile(Path::Build(homePath, dest), Path::Build(homePath, src), err);
#endif
}
void DeleteFile(Str::StringRef path, std::error_code& err)
{
#ifdef BUILD_VM
	bool success;
	VM::SendMsg<VM::FSHomePathDeleteFileMsg>(path, success);
	if (success)
		ClearErrorCode(err);
	else
		SetErrorCodeFilesystem(err, filesystem_error::no_such_file);
#else
	if (!Path::IsValid(path, false)) {
		SetErrorCodeFilesystem(err, filesystem_error::invalid_filename);
		return;
	}
	RawPath::DeleteFile(Path::Build(homePath, path), err);
#endif
}

DirectoryRange ListFiles(Str::StringRef path, std::error_code& err)
{
#ifdef BUILD_VM
	Util::optional<DirectoryRange> out;
	VM::SendMsg<VM::FSHomePathListFilesMsg>(path, out);
	if (out) {
		ClearErrorCode(err);
		return *out;
	} else {
		SetErrorCodeFilesystem(err, filesystem_error::no_such_directory);
		return {};
	}
#else
	if (!Path::IsValid(path, true)) {
		SetErrorCodeFilesystem(err, filesystem_error::invalid_filename);
		return {};
	}
	return RawPath::ListFiles(Path::Build(homePath, path), err);
#endif
}

RecursiveDirectoryRange ListFilesRecursive(Str::StringRef path, std::error_code& err)
{
#ifdef BUILD_VM
	Util::optional<RecursiveDirectoryRange> out;
	VM::SendMsg<VM::FSHomePathListFilesRecursiveMsg>(path, out);
	if (out) {
		ClearErrorCode(err);
		return *out;
	} else {
		SetErrorCodeFilesystem(err, filesystem_error::no_such_directory);
		return {};
	}
#else
	if (!Path::IsValid(path, true)) {
		SetErrorCodeFilesystem(err, filesystem_error::invalid_filename);
		return {};
	}
	return RawPath::ListFilesRecursive(Path::Build(homePath, path), err);
#endif
}

// Note that this function is practically identical to the PakPath version.
// Try to keep any changes in sync between the two versions.
Cmd::CompletionResult CompleteFilename(Str::StringRef prefix, Str::StringRef root, Str::StringRef extension, bool allowSubdirs, bool stripExtension)
{
	std::string prefixDir, prefixBase;
	if (Str::IsSuffix("/", prefix)) {
		if (!allowSubdirs)
			return {};
		prefixDir = prefix;
		prefixBase = "";
	} else {
		prefixDir = Path::DirName(prefix);
		if (!allowSubdirs && !prefixDir.empty())
			return {};
		prefixBase = Path::BaseName(prefix);
	}

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

#ifndef BUILD_VM
// Determine path to the executable, default to current directory
std::string DefaultBasePath()
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
		len *= 2;
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
std::string DefaultHomePath()
{
#ifdef _WIN32
	wchar_t buffer[MAX_PATH];
	if (!SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, buffer)))
		return "";
	return Str::UTF16To8(buffer) + "\\My Games\\" PRODUCT_NAME;
#else
	const char* home = getenv("HOME");
	if (!home)
		return "";
#ifdef __APPLE__
	return std::string(home) + "/Library/Application Support/" PRODUCT_NAME;
#else
	return std::string(home) + "/." PRODUCT_NAME_LOWER;
#endif
#endif
}
#endif // BUILD_VM

#ifdef BUILD_VM
void Initialize()
{
	// Make sure we don't leak fds if Initialize is called more than once
	for (LoadedPakInfo& x: FS::PakPath::loadedPaks) {
		if (x.fd != -1)
			close(x.fd);
	}

	VM::SendMsg<VM::FSInitializeMsg>(homePath, libPath, availablePaks, PakPath::loadedPaks, PakPath::fileMap);
}
#else
// Get an absolute path from a relative one. This may fail if the path does not
// exist. An error string is returned in that case.
static Util::optional<std::string> GetRealPath(Str::StringRef path, std::string& error)
{
#ifdef _WIN32
	std::wstring path_u16 = Str::UTF8To16(path);
	DWORD attr = GetFileAttributesW(path_u16.c_str());
	if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
		error = attr == INVALID_FILE_ATTRIBUTES ? Sys::Win32StrError(GetLastError()) : "Not a directory";
		return {};
	}

	size_t len = GetFullPathNameW(path_u16.c_str(), 0, nullptr, nullptr);
	if (!len) {
		error = Sys::Win32StrError(GetLastError());
		return {};
	}
	std::unique_ptr<wchar_t[]> realPath(new wchar_t[len]);
	if (!GetFullPathNameW(path_u16.c_str(), len, realPath.get(), nullptr)) {
		error = Sys::Win32StrError(GetLastError());
		return {};
	}

	return Str::UTF16To8(realPath.get());
#else
	char* realPath = realpath(path.c_str(), nullptr);
	if (!realPath) {
		error = strerror(errno);
		return {};
	}

	std::string out = realPath;
	free(realPath);
	return out;
#endif
}

void Initialize(Str::StringRef homePath, Str::StringRef libPath, const std::vector<std::string>& paths)
{
	// Create the homepath and its pkg directory to avoid any issues later on
	std::error_code err;
	RawPath::CreatePathTo(FS::Path::Build(homePath, "pkg") + "/", err);
	if (err)
		Sys::Error("Could not create homepath: %s", err.message());

	std::string error;
	try {
		FS::homePath = GetRealPath(homePath, error).value();
	} catch (Util::bad_optional_access&) {
		Sys::Error("Invalid homepath: %s", error);
	}
	try {
		FS::libPath = GetRealPath(libPath, error).value();
	} catch (Util::bad_optional_access&) {
		Sys::Error("Invalid libpath: %s", error);
	}

	// The first path in the list is implicitly added by the engine, so don't
	// print a warning if it is not valid.
	bool first = true;

	for (const std::string& path: paths) {
		// Convert the given path to an absolute path and check that it exists
		Util::optional<std::string> realPath = GetRealPath(path, error);
		if (!realPath) {
			if (!first)
				fsLogs.Warn("Ignoring path %s: %s", path, error);
			first = false;
			continue;
		}

		if (std::find(FS::pakPaths.begin(), FS::pakPaths.end(), realPath) == FS::pakPaths.end())
			FS::pakPaths.push_back(*realPath);
		first = false;
	}
	isInitialized = true;

	fsLogs.Notice("Lib path: %s", FS::libPath.c_str());
	fsLogs.Notice("Home path: %s", FS::homePath.c_str());
	for (std::string& x: pakPaths)
		fsLogs.Notice("Pak search path: %s", x.c_str());
	if (pakPaths.empty())
		fsLogs.Warn("No pak search paths found");

	RefreshPaks();
}
#endif

void FlushAll()
{
	fflush(nullptr);
}

bool IsInitialized()
{
	return isInitialized;
}

#ifndef BUILD_VM
// Add a pak to the list of available paks
static void AddPak(pakType_t type, Str::StringRef filename, Str::StringRef basePath)
{
	std::string fullPath = Path::Build(basePath, filename);

	size_t suffixLen = type == PAK_DIR ? strlen(PAK_DIR_EXT) : strlen(PAK_ZIP_EXT);
	std::string name, version;
	Util::optional<uint32_t> checksum;
	if (!ParsePakName(filename.begin(), filename.end() - suffixLen, name, version, checksum) || (type == PAK_DIR && checksum)) {
		fsLogs.Warn("Invalid pak name: %s", fullPath);
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
	} catch (std::system_error&) {
		// If there was an error reading a directory, just ignore it and go to
		// the next one.
	}
}
#endif // BUILD_VM

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
		else if (c == '~')
			return -1;
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

#ifndef BUILD_VM
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
		if (b.type == PAK_ZIP && a.type != PAK_ZIP)
			return true;

		return false;
	});
}
#endif

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
			return Util::nullopt > pakInfo.checksum;
		});

		// Only allow zip packages because directories don't have a checksum
		if (iter == availablePaks.begin() || (iter - 1)->type == PAK_DIR || (iter - 1)->name != name || (iter - 1)->version != version || (iter - 1)->checksum)
			return nullptr;
	}

	return &*(iter - 1);
}

bool ParsePakName(const char* begin, const char* end, std::string& name, std::string& version, Util::optional<uint32_t>& checksum)
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
		checksum = Util::nullopt;
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

std::string MakePakName(Str::StringRef name, Str::StringRef version, Util::optional<uint32_t> checksum)
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

#ifdef BUILD_ENGINE
void HandleFileSystemSyscall(int minor, Util::Reader& reader, IPC::Channel& channel, Str::StringRef vmName)
{
	switch (minor) {
	case VM::FS_INITIALIZE:
		IPC::HandleMsg<VM::FSInitializeMsg>(channel, std::move(reader), [](std::string& homePath, std::string& libPath, std::vector<FS::PakInfo>& availablePaks, std::vector<FS::LoadedPakInfo>& loadedPaks, std::unordered_map<std::string, std::pair<uint32_t, FS::offset_t>>& fileMap) {
			homePath = GetHomePath();
			libPath = GetLibPath();
			availablePaks = GetAvailablePaks();
			loadedPaks = PakPath::loadedPaks;
			fileMap = PakPath::fileMap;
		});
		break;

	case VM::FS_HOMEPATH_OPENMODE:
		IPC::HandleMsg<VM::FSHomePathOpenModeMsg>(channel, std::move(reader), [](std::string path, uint32_t mode, Util::optional<IPC::OwnedFileHandle>& out) {
			std::error_code err;
			FS::File file = HomePath::OpenMode(Path::Build("game", path), static_cast<openMode_t>(mode), err);
			if (!err)
				out = FileToIPC(std::move(file), static_cast<openMode_t>(mode));
		});
		break;

	case VM::FS_HOMEPATH_FILEEXISTS:
		IPC::HandleMsg<VM::FSHomePathFileExistsMsg>(channel, std::move(reader), [](std::string path, bool& out) {
			out = HomePath::FileExists(Path::Build("game", path));
		});
		break;

	case VM::FS_HOMEPATH_TIMESTAMP:
		IPC::HandleMsg<VM::FSHomePathTimestampMsg>(channel, std::move(reader), [](std::string path, Util::optional<uint64_t>& out) {
			std::error_code err;
			std::chrono::system_clock::time_point t = HomePath::FileTimestamp(Path::Build("game", path), err);
			if (!err)
				out = std::chrono::system_clock::to_time_t(t);
		});
		break;

	case VM::FS_HOMEPATH_MOVEFILE:
		IPC::HandleMsg<VM::FSHomePathMoveFileMsg>(channel, std::move(reader), [](std::string dest, std::string src, bool& success) {
			std::error_code err;
			HomePath::MoveFile(Path::Build("game", dest), Path::Build("game", src), err);
			success = !err;
		});
		break;

	case VM::FS_HOMEPATH_DELETEFILE:
		IPC::HandleMsg<VM::FSHomePathDeleteFileMsg>(channel, std::move(reader), [](std::string path, bool& success) {
			std::error_code err;
			HomePath::DeleteFile(Path::Build("game", path), err);
			success = !err;
		});
		break;

	case VM::FS_HOMEPATH_LISTFILES:
		IPC::HandleMsg<VM::FSHomePathListFilesMsg>(channel, std::move(reader), [](std::string path, Util::optional<std::vector<std::string>>& out) {
			try {
				std::vector<std::string> vec;
				for (auto&& x: FS::HomePath::ListFiles(Path::Build("game", path)))
					vec.push_back(std::move(x));
				out = std::move(vec);
			} catch (std::system_error&) {}
		});
		break;

	case VM::FS_HOMEPATH_LISTFILESRECURSIVE:
		IPC::HandleMsg<VM::FSHomePathListFilesRecursiveMsg>(channel, std::move(reader), [](std::string path, Util::optional<std::vector<std::string>>& out) {
			try {
				std::vector<std::string> vec;
				for (auto&& x: FS::HomePath::ListFilesRecursive(Path::Build("game", path)))
					vec.push_back(std::move(x));
				out = std::move(vec);
			} catch (std::system_error&) {}
		});
		break;

	case VM::FS_PAKPATH_OPEN:
		IPC::HandleMsg<VM::FSPakPathOpenMsg>(channel, std::move(reader), [](uint32_t pakIndex, std::string path, Util::optional<IPC::OwnedFileHandle>& out) {
			auto& loadedPaks = FS::PakPath::GetLoadedPaks();
			if (loadedPaks.size() <= pakIndex)
				return;
			if (loadedPaks[pakIndex].type != PAK_DIR)
				return;
			if (!Path::IsValid(path, false))
				return;
			std::error_code err;
			FS::File file = RawPath::OpenRead(Path::Build(loadedPaks[pakIndex].path, path), err);
			if (!err)
				out = FileToIPC(std::move(file), MODE_READ);
		});
		break;

	case VM::FS_PAKPATH_TIMESTAMP:
		IPC::HandleMsg<VM::FSPakPathTimestampMsg>(channel, std::move(reader), [](uint32_t pakIndex, std::string path, Util::optional<uint64_t>& out) {
			auto& loadedPaks = FS::PakPath::GetLoadedPaks();
			if (loadedPaks.size() <= pakIndex)
				return;
			if (loadedPaks[pakIndex].type != PAK_DIR)
				return;
			if (!Path::IsValid(path, false))
				return;
			std::error_code err;
			std::chrono::system_clock::time_point t = RawPath::FileTimestamp(Path::Build(loadedPaks[pakIndex].path, path), err);
			if (!err)
				out = std::chrono::system_clock::to_time_t(t);
		});
		break;

	case VM::FS_PAKPATH_LOADPAK:
		IPC::HandleMsg<VM::FSPakPathLoadPakMsg>(channel, std::move(reader), [](uint32_t pakIndex, Util::optional<uint32_t> expectedChecksum, std::string pathPrefix) {
			if (availablePaks.size() <= pakIndex)
				return;
			try {
				FS::PakPath::InternalLoadPak(availablePaks[pakIndex], expectedChecksum, pathPrefix, FS::throws());
			} catch (std::system_error& err) {
				fsLogs.Warn("Could not load pak %s: %s", availablePaks[pakIndex].path, err.what());
			}
		});
		break;

	default:
		Sys::Drop("Bad filesystem syscall number '%d' for VM '%s'", minor, vmName);
	}
}
#endif

} // namespace FS
