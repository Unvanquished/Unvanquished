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

#include <common/FileSystem.h>
#include "q_shared.h"
#include "qcommon.h"

// Compatibility wrapper for the filesystem
const char TEMP_SUFFIX[] = ".tmp";

// Default base package
const char DEFAULT_BASE_PAK[] = "unvanquished";

// Cvars to select the base and extra packages to use
static Cvar::Cvar<std::string> fs_basepak("fs_basepak", "base pak to load", 0, DEFAULT_BASE_PAK);
static Cvar::Cvar<std::string> fs_extrapaks("fs_extrapaks", "space-seperated list of paks to load in addition to the base pak", 0, "");

struct handleData_t {
	bool isOpen;
	bool isPakFile;
	Util::optional<std::string> renameTo;

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

static Cvar::Cvar<bool> allowRemotePakDir("client.allowRemotePakDir", "Connect to servers that load game data from directories", Cvar::TEMPORARY, false);

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
	if (handle < 0 || handle >= MAX_FILE_HANDLES)
		Com_Error(ERR_DROP, "FS_CheckHandle: invalid handle");
	if (!handleTable[handle].isOpen)
		Com_Error(ERR_DROP, "FS_CheckHandle: closed handle");
	if (write && handleTable[handle].isPakFile)
		Com_Error(ERR_DROP, "FS_CheckHandle: writing to file in pak");
}

bool FS_FileExists(const char* path)
{
	return FS::PakPath::FileExists(path) || FS::HomePath::FileExists(path);
}

int FS_FOpenFileRead(const char* path, fileHandle_t* handle, bool)
{
	if (!handle)
		return FS_FileExists(path);

	*handle = FS_AllocHandle();
	int length = -1;
	std::error_code err;
	if (FS::PakPath::FileExists(path)) {
		handleTable[*handle].fileData = FS::PakPath::ReadFile(path, err);
		if (!err) {
			handleTable[*handle].filePos = 0;
			handleTable[*handle].isPakFile = true;
			handleTable[*handle].isOpen = true;
			length = handleTable[*handle].fileData.size();
		}
	} else {
		handleTable[*handle].file = FS::HomePath::OpenRead(path, err);
		if (!err) {
			length = handleTable[*handle].file.Length();
			handleTable[*handle].isPakFile = false;
			handleTable[*handle].isOpen = true;
		}
	}
	if (err) {
		Com_DPrintf("Failed to open '%s' for reading: %s\n", path, err.message().c_str());
		*handle = 0;
		length = -1;
	}
	return length;
}

static fileHandle_t FS_FOpenFileWrite_internal(const char* path, bool temporary)
{
	fileHandle_t handle = FS_AllocHandle();
	try {
		handleTable[handle].file = FS::HomePath::OpenWrite(temporary ? std::string(path) + TEMP_SUFFIX : path);
	} catch (std::system_error& err) {
		Com_Printf("Failed to open '%s' for writing: %s\n", path, err.what());
		return 0;
	}
	handleTable[handle].forceFlush = false;
	handleTable[handle].isPakFile = false;
	handleTable[handle].isOpen = true;
	if (temporary) {
		handleTable[handle].renameTo = FS::Path::Build(FS::GetHomePath(), path);
	}
	return handle;
}

fileHandle_t FS_FOpenFileWrite(const char* path)
{
	return FS_FOpenFileWrite_internal(path, false);
}

fileHandle_t FS_FOpenFileWriteViaTemporary(const char* path)
{
	return FS_FOpenFileWrite_internal(path, true);
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
	return FS_FOpenFileWrite_internal(path, false);
}

int FS_SV_FOpenFileRead(const char* path, fileHandle_t* handle)
{
	if (!handle)
		return FS::HomePath::FileExists(path);

	*handle = FS_AllocHandle();
	std::error_code err;
	handleTable[*handle].file = FS::HomePath::OpenRead(path, err);
	if (err) {
		Com_DPrintf("Failed to open '%s' for reading: %s\n", path, err.message().c_str());
		*handle = 0;
		return 0;
	}
	handleTable[*handle].isPakFile = false;
	handleTable[*handle].isOpen = true;
	return handleTable[*handle].file.Length();
}

int FS_Game_FOpenFileByMode(const char* path, fileHandle_t* handle, fsMode_t mode)
{
	switch (mode) {
	case FS_READ:
		if (FS::PakPath::FileExists(path))
			return FS_FOpenFileRead(path, handle, false);
		else {
			int size = FS_SV_FOpenFileRead(FS::Path::Build("game", path).c_str(), handle);
			return (!handle || *handle) ? size : -1;
		}

	case FS_WRITE:
	case FS_WRITE_VIA_TEMPORARY:
		*handle = FS_FOpenFileWrite_internal(FS::Path::Build("game", path).c_str(), mode == FS_WRITE_VIA_TEMPORARY);
		return *handle == 0 ? -1 : 0;

	case FS_APPEND:
	case FS_APPEND_SYNC:
		*handle = FS_FOpenFileAppend(FS::Path::Build("game", path).c_str());
		handleTable[*handle].forceFlush = mode == FS_APPEND_SYNC;
		return *handle == 0 ? -1 : 0;

	default:
		Com_Error(ERR_DROP, "FS_Game_FOpenFileByMode: bad mode %d", mode);
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
			if (handleTable[handle].renameTo) {
				std::string renameTo = std::move(*handleTable[handle].renameTo);
				handleTable[handle].renameTo = Util::nullopt; // tidy up after abusing std::move
				try {
					FS::RawPath::MoveFile(renameTo, renameTo + TEMP_SUFFIX);
				} catch (std::system_error& err) {
					Com_Printf("Failed to replace file %s: %s\n", renameTo.c_str(), err.what());
					return -1;
				}
			}
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
		std::error_code err;
		int length = handleTable[handle].file.Length(err);
		if (err) {
			Com_Printf("Failed to get file length: %s\n", err.message().c_str());
			return 0;
		}
		return length;
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
			Com_Printf("FS_Read failed: %s\n", err.what());
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
	int length = FS_FOpenFileRead(path, &handle, true);

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

	int numFiles = 0;
	bool dirsOnly = extension && !strcmp(extension, "/");

	try {
		for (const std::string& x: FS::PakPath::ListFiles(path)) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			int length = x.size() + (x.back() != '/');
			if (bufSize < length)
				return numFiles;
			memcpy(listBuf, x.c_str(), length);
			listBuf[length - 1] = '\0';
			listBuf += length;
			bufSize -= length;
			numFiles++;
		}
	} catch (std::system_error&) {}
	try {
		for (const std::string& x: FS::HomePath::ListFiles(FS::Path::Build("game", path))) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			int length = x.size() + (x.back() != '/');
			if (bufSize < length)
				return numFiles;
			memcpy(listBuf, x.c_str(), length);
			listBuf[length - 1] = '\0';
			listBuf += length;
			bufSize -= length;
			numFiles++;
		}
	} catch (std::system_error&) {}

	return numFiles;
}

int FS_GetFileListRecursive(const char* path, const char* extension, char* listBuf, int bufSize)
{
	// Mods are not yet supported in the new filesystem
	if (!strcmp(path, "$modlist"))
		return 0;

	int numFiles = 0;
	bool dirsOnly = extension && !strcmp(extension, "/");

	try {
		for (const std::string& x: FS::PakPath::ListFilesRecursive(path)) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			int length = x.size() + (x.back() != '/');
			if (bufSize < length)
				return numFiles;
			memcpy(listBuf, x.c_str(), length);
			listBuf[length - 1] = '\0';
			listBuf += length;
			bufSize -= length;
			numFiles++;
		}
	} catch (std::system_error&) {}
	try {
		for (const std::string& x: FS::HomePath::ListFiles(FS::Path::Build("game", path))) {
			if (extension && !Str::IsSuffix(extension, x))
				continue;
			if (dirsOnly != (x.back() == '/'))
				continue;
			int length = x.size() + (x.back() != '/');
			if (bufSize < length)
				return numFiles;
			memcpy(listBuf, x.c_str(), length);
			listBuf[length - 1] = '\0';
			listBuf += length;
			bufSize -= length;
			numFiles++;
		}
	} catch (std::system_error&) {}

	return numFiles;
}

const char* FS_LoadedPaks()
{
	static char info[BIG_INFO_STRING];
	info[0] = '\0';
	for (const FS::LoadedPakInfo& x: FS::PakPath::GetLoadedPaks()) {
		if (!x.pathPrefix.empty())
			continue;
		if (info[0])
			Q_strcat(info, sizeof(info), " ");
		Q_strcat(info, sizeof(info), FS::MakePakName(x.name, x.version, x.realChecksum).c_str());
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

void FS_LoadBasePak()
{
	Cmd::Args extrapaks(fs_extrapaks.Get());
	for (const auto& x: extrapaks) {
		if (!FS_LoadPak(x.c_str()))
			Com_Error(ERR_FATAL, "Could not load extra pak '%s'\n", x.c_str());
	}

	if (!FS_LoadPak(fs_basepak.Get().c_str())) {
		Com_Printf("Could not load base pak '%s', falling back to default\n", fs_basepak.Get().c_str());
		if (!FS_LoadPak(DEFAULT_BASE_PAK))
			Com_Error(ERR_FATAL, "Could not load default base pak '%s'", DEFAULT_BASE_PAK);
	}
}

void FS_LoadAllMapMetadata()
{
	std::unordered_set<std::string> maps;
	for (const auto& x: FS::GetAvailablePaks()) {
		if (Str::IsPrefix("map-", x.name) && maps.find(x.name) == maps.end())
			maps.insert(x.name);
	}

	for (const auto& x: maps) {
		try {
			FS::PakPath::LoadPakPrefix(*FS::FindPak(x), va("meta/%s/", x.substr(4).c_str()));
		} catch (std::system_error&) {} // ignore and move on
	}
}

bool FS_LoadServerPaks(const char* paks, bool isDemo)
{
	Cmd::Args args(paks);
	fs_missingPaks.clear();
	for (auto& x: args) {
		std::string name, version;
		Util::optional<uint32_t> checksum;
		if (!FS::ParsePakName(x.data(), x.data() + x.size(), name, version, checksum)) {
			Com_Error(ERR_DROP, "Invalid pak reference from server: %s", x.c_str());
		} else if (!checksum) {
			if (isDemo || allowRemotePakDir.Get())
				continue;
			Com_Error(ERR_DROP, "The server is configured to load game data from a directory which makes it incompatible with remote clients.");
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

	// Load extra paks as well for demos
	if (isDemo) {
		Cmd::Args extrapaks(fs_extrapaks.Get());
		for (auto& x: extrapaks) {
			if (!FS_LoadPak(x.c_str()))
				Com_Error(ERR_FATAL, "Could not load extra pak '%s'\n", x.c_str());
		}
	}

	return fs_missingPaks.empty();
}

bool CL_WWWBadChecksum(const char *pakname);
bool FS_ComparePaks(char* neededpaks, int len, bool dlstring)
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
#ifndef BUILD_SERVER
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
		: Cmd::StaticCmd("which", Cmd::SYSTEM, "shows which pak a file is in") {}

	void Run(const Cmd::Args& args) const OVERRIDE
	{
		if (args.Argc() != 2) {
			PrintUsage(args, "<file>", "");
			return;
		}

		const std::string& filename = args.Argv(1);
		const FS::LoadedPakInfo* pak = FS::PakPath::LocateFile(filename);
		if (pak)
			Print( "File \"%s\" found in \"%s\"", filename, pak->path);
		else
			Print("File not found: \"%s\"", filename);
	}

	Cmd::CompletionResult Complete(int argNum, const Cmd::Args&, Str::StringRef prefix) const OVERRIDE
	{
		if (argNum == 1) {
			return FS::PakPath::CompleteFilename(prefix, "", "", true, false);
		}

		return {};
	}
};
static WhichCmd WhichCmdRegistration;

class ListPathsCmd: public Cmd::StaticCmd {
public:
	ListPathsCmd()
		: Cmd::StaticCmd("listPaths", Cmd::SYSTEM, "list filesystem search paths") {}

	void Run(const Cmd::Args&) const OVERRIDE
	{
		Print("Home path: %s", FS::GetHomePath());
		for (auto& x: FS::PakPath::GetLoadedPaks())
			Print("Loaded pak: %s", x.path);
	}
};
static ListPathsCmd ListPathsCmdRegistration;

class DirCmd: public Cmd::StaticCmd {
public:
	DirCmd(): Cmd::StaticCmd("dir", Cmd::SYSTEM, "list all files in a given directory with the option to pass a filter") {}

	void Run(const Cmd::Args& args) const OVERRIDE
	{
		bool filter = false;
		if (args.Argc() != 2 && args.Argc() != 3) {
			PrintUsage(args, "<path> [filter]", "");
			return;
		}

		if ( args.Argc() == 3) {
			filter = true;
		}

		Print("In Paks:");
		Print("--------");
		try {
			for (auto& filename : FS::PakPath::ListFiles(args.Argv(1))) {
				if (filename.size() && (!filter || Com_Filter(args.Argv(2).c_str(), filename.c_str(), false))) {
					Print("%s", filename.c_str());
				}
			}
		} catch (std::system_error&) {
			Print("^1ERROR^7: Path does not exist");
		}

		Print("\n");
		Print("In Homepath");
		Print("-----------");
		try {
			for (auto& filename : FS::RawPath::ListFiles(FS::Path::Build(FS::GetHomePath(),args.Argv(1)))) {
				if (filename.size() && (!filter || Com_Filter(args.Argv(2).c_str(), filename.c_str(), false))) {
					Print("%s", filename.c_str());
				}
			}
		} catch (std::system_error&) {
			Print("^1ERROR^7: Path does not exist");
		}

	}
};
static DirCmd DirCmdRegistration;
