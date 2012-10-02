/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

/*****************************************************************************
 * Name:    files.c
 *
 * desc:    handle based filesystem for Quake III Arena
 *
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "qcommon.h"
#include "unzip.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

/*
=============================================================================

QUAKE3 FILESYSTEM

All of Quake's data access is through a hierarchical file system, but the contents of
the file system can be transparently merged from several sources.

A "qpath" is a reference to game file data.  MAX_ZPATH is 256 characters, which must include
a terminating zero. "..", "\\", and ":" are explicitly illegal in qpaths to prevent any
references outside the quake directory system.

The "base path" is the path to the directory holding all the game directories and usually
the executable.  It defaults to ".", but can be overridden with a "+set fs_basepath c:\quake3"
command line to allow code debugging in a different directory.  Basepath cannot
be modified at all after startup.  Any files that are created (demos, screenshots,
etc) will be created relative to the base path, so base path should usually be writable.

If a user runs the game directly from a CD, the base path would be on the CD.  This
should still function correctly, but all file writes will fail (harmlessly).

The "home path" is the path used for all write access. On win32 systems we have "base path"
== "home path", but on *nix systems the base installation is usually readonly, and
"home path" points to ~/.q3a or similar

The user can also install custom mods and content in the home path,
so that should be searched along with base path for game content.


The "base game" is the directory under the paths where data comes from by default, and
can be either "baseq3" or "demoq3".

The "current game" may be the same as the base game, or it may be the name of another
directory under the paths that should be searched for files before looking in the base game.
This is the basis for addons.

Clients automatically set the game directory after receiving a gamestate from a server,
so only servers need to worry about +set fs_game.

No other directories outside of the base game and current game will ever be referenced by
filesystem functions.

To save disk space and speed loading, directory trees can be collapsed into zip files.
The files use a ".pk3" extension to prevent users from unzipping them accidentally, but
otherwise the are simply zip files.  A game directory can have multiple
zip files of the form "pak0.pk3", "pak1.pk3", etc.  Zip files are searched in decending order
from the highest number to the lowest, and will always take precedence over the filesystem.
This allows a pk3 distributed as a patch to override all existing data.

File search order: when FS_FOpenFileRead gets called it will go through the fs_searchpaths
structure and stop on the first successful hit. fs_searchpaths is built with successive
calls to FS_AddGameDirectory

Additionally, we search in several subdirectories:
current game is the current mode
base game is a variable to allow mods based on other mods
(such as baseq3 + missionpack content combination in a mod for instance)
BASEGAME is the hardcoded base game ("baseq3")

e.g. the qpath "sound/newstuff/test.wav" would be searched for in the following places:

home path + current game's zip files
home path + current game's directory
base path + current game's zip files
base path + current game's directory

home path + base game's zip file
home path + base game's directory
base path + base game's zip file
base path + base game's directory

home path + BASEGAME's zip file
home path + BASEGAME's directory
base path + BASEGAME's zip file
base path + BASEGAME's directory

server download, to be written to home path + current game's directory


The filesystem can be safely shutdown and reinitialized with different
basedir / cddir / game combinations, but all other subsystems that rely on it
(sound, video) must also be forced to restart.

Because the same files are loaded by both the clip model (CM_) and renderer (TR_)
subsystems, a simple single-file caching scheme is used.  The CM_ subsystems will
load the file with a request to cache.  Only one file will be kept cached at a time,
so any models that are going to be referenced by both subsystems should alternate
between the CM_ load function and the ref load function.

TODO: A qpath that starts with a leading slash will always refer to the base game, even if another
game is currently active.  This allows character models, skins, and sounds to be downloaded
to a common directory no matter which game is active.

How to prevent downloading zip files?
Pass pk3 file names in systeminfo, and download before FS_Restart()?

Aborting a download disconnects the client from the server.

How to mark files as downloadable?  Commercial add-ons won't be downloadable.

Non-commercial downloads will want to download the entire zip file.
the game would have to be reset to actually read the zip in

Auto-update information

Path separators

Casing

  separate server gamedir and client gamedir, so if the user starts
  a local game after having connected to a network game, it won't stick
  with the network game.

  allow menu options for game selection?

Read / write config to floppy option.

Different version coexistence?

When building a pak file, make sure a wolfconfig.cfg isn't present in it,
or configs will never get loaded from disk!

  todo:

  downloading (outside fs?)
  game directory passing and restarting

=============================================================================

*/

#define MAX_ZPATH         256
#define MAX_SEARCH_PATHS  4096
#define MAX_FILEHASH_SIZE 1024

typedef struct fileInPack_s
{
	char                *name; // name of the file
	unsigned long       pos; // file info position in zip
	unsigned long       len; // uncompress file size
	struct  fileInPack_s *next; // next file in the hash
} fileInPack_t;

typedef struct
{
	char        pakFilename[ MAX_OSPATH ]; // full pak0 path
	char        pakBasename[ MAX_OSPATH ]; // pak0 basename
	char        pakGamename[ MAX_OSPATH ]; // baseq3
	unzFile     handle; // handle to zip file
	int         checksum; // regular checksum
	int         pure_checksum; // checksum for pure
	int         numfiles; // number of files in pk3
	int         referenced; // referenced file flags
	int         hashSize; // hash table size (power of 2)
	fileInPack_t * *hashTable; // hash table
	fileInPack_t *buildBuffer; // buffer with the filenames etc.
} pack_t;

typedef struct
{
	char path[ MAX_OSPATH ]; // root game folder path
	char gamedir[ MAX_OSPATH ]; // baseq3
} directory_t;

typedef struct searchpath_s
{
	struct searchpath_s *next;

	pack_t              *pack; // only one of pack / dir will be non NULL
	directory_t         *dir;
} searchpath_t;

//bani - made fs_gamedir non-static
char          fs_gamedir[ MAX_OSPATH ]; // this will be a single file name with no separators
static cvar_t *fs_debug;
static cvar_t *fs_homepath;
static cvar_t *fs_basepath;

static cvar_t *fs_libpath;

#ifdef MACOS_X
// Also search the .app bundle for .pk3 files
static  cvar_t      *fs_apppath;
#endif

static cvar_t       *fs_basegame;
static cvar_t       *fs_gamedirvar;
static searchpath_t *fs_searchpaths;
static int          fs_readCount; // total bytes read
static int          fs_loadCount; // total files read
static int          fs_loadStack; // total files in memory
static int          fs_packFiles; // total number of files in packs

static int          fs_fakeChkSum;
static int          fs_checksumFeed;

typedef union qfile_gus
{
	FILE     *o;
	unzFile z;
} qfile_gut;

typedef struct qfile_us
{
	qfile_gut file;
	qboolean  unique;
} qfile_ut;

typedef struct
{
	qfile_ut handleFiles;
	qboolean handleSync;
	int      baseOffset;
	int      fileSize;
	int      zipFilePos;
	qboolean zipFile;
	qboolean streamed;
	char     name[ MAX_ZPATH ];
} fileHandleData_t;

static fileHandleData_t fsh[ MAX_FILE_HANDLES ];

// TTimo - show_bug.cgi?id=540
// whether we did a reorder on the current search path when joining the server
static qboolean fs_reordered;

// never load anything from pk3 files that are not present at the server when pure
// ex: when fs_numServerPaks != 0, FS_FOpenFileRead won't load anything outside of pk3 except .cfg .menu .game .dat .po
static int  fs_numServerPaks;
static int  fs_serverPaks[ MAX_SEARCH_PATHS ]; // checksums
static char *fs_serverPakNames[ MAX_SEARCH_PATHS ]; // pk3 names

// only used for autodownload, to make sure the client has at least
// all the pk3 files that are referenced at the server side
static int  fs_numServerReferencedPaks;
static int  fs_serverReferencedPaks[ MAX_SEARCH_PATHS ]; // checksums
static char *fs_serverReferencedPakNames[ MAX_SEARCH_PATHS ]; // pk3 names

// last valid game folder used
char        lastValidBase[ MAX_OSPATH ];
char        lastValidGame[ MAX_OSPATH ];

/*
==============
FS_Initialized
==============
*/

qboolean FS_Initialized( void )
{
	return ( fs_searchpaths != NULL );
}

/*
=================
FS_PakIsPure
=================
*/
qboolean FS_PakIsPure( pack_t *pack )
{
	int i;

	if ( fs_numServerPaks )
	{
		for ( i = 0; i < fs_numServerPaks; i++ )
		{
			// FIXME: also use hashed file names
			// NOTE TTimo: a pk3 with same checksum but different name would be validated too
			//   I don't see this as allowing for any exploit, it would only happen if the client does manips of its file names (not a bug)
			if ( pack->checksum == fs_serverPaks[ i ] )
			{
				return qtrue; // on the approved list
			}
		}

		return qfalse; // not on the pure server pak list
	}

	return qtrue;
}

/*
=================
FS_LoadStack
return load stack
=================
*/
int FS_LoadStack( void )
{
	return fs_loadStack;
}

/*
================
return a hash value for the filename
================
*/
static long FS_HashFileName( const char *fname, int hashSize )
{
	int  i;
	long hash;
	char letter;

	hash = 0;
	i = 0;

	while ( fname[ i ] != '\0' )
	{
		letter = tolower( fname[ i ] );

		if ( letter == '.' )
		{
			break; // don't include extension
		}

		if ( letter == '\\' )
		{
			letter = '/'; // damn path names
		}

		if ( letter == PATH_SEP )
		{
			letter = '/'; // damn path names
		}

		hash += ( long )( letter ) * ( i + 119 );
		i++;
	}

	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) );
	hash &= ( hashSize - 1 );
	return hash;
}

static fileHandle_t FS_HandleForFile( void )
{
	int i;

	for ( i = 1; i < MAX_FILE_HANDLES; i++ )
	{
		if ( fsh[ i ].handleFiles.file.o == NULL )
		{
			return i;
		}
	}

	Com_Error( ERR_DROP, "FS_HandleForFile: none free" );
}

static FILE *FS_FileForHandle( fileHandle_t f )
{
	if ( f < 0 || f > MAX_FILE_HANDLES )
	{
		Com_Error( ERR_DROP, "FS_FileForHandle: %d out of range", f );
	}

	if ( fsh[ f ].zipFile == qtrue )
	{
		Com_Error( ERR_DROP, "FS_FileForHandle: can't get FILE on zip file" );
	}

	if ( !fsh[ f ].handleFiles.file.o )
	{
		Com_Error( ERR_DROP, "FS_FileForHandle: NULL" );
	}

	return fsh[ f ].handleFiles.file.o;
}

void    FS_ForceFlush( fileHandle_t f )
{
	FILE *file;

	file = FS_FileForHandle( f );
	setvbuf( file, NULL, _IONBF, 0 );
}

/*
================
FS_filelength

If this is called on a non-unique FILE (from a pak file),
it will return the size of the pak file, not the expected
size of the file.
================
*/
int FS_filelength( fileHandle_t f )
{
	int pos;
	int end;
	FILE *h;

	h = FS_FileForHandle( f );
	pos = ftell( h );
	fseek( h, 0, SEEK_END );
	end = ftell( h );
	fseek( h, pos, SEEK_SET );

	return end;
}

/*
====================
FS_ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
static void FS_ReplaceSeparators( char *path )
{
	char *s;

	for ( s = path; *s; s++ )
	{
		if ( *s == '/' || *s == '\\' )
		{
			*s = PATH_SEP;
		}
	}
}

/*
===================
FS_BuildOSPath

Qpath may have either forward or backwards slashes
===================
*/
char *FS_BuildOSPath( const char *base, const char *game, const char *qpath )
{
	char        temp[ MAX_OSPATH ];
	static char ospath[ 2 ][ MAX_OSPATH ];
	static int  toggle;

	toggle ^= 1; // flip-flop to allow two returns without clash

	if ( !game || !game[ 0 ] )
	{
		game = fs_gamedir;
	}

	Com_sprintf( temp, sizeof( temp ), "/%s/%s", game, qpath );
	FS_ReplaceSeparators( temp );
	Com_sprintf( ospath[ toggle ], sizeof( ospath[ 0 ] ), "%s%s", base, temp );

	return ospath[ toggle ];
}

/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
qboolean FS_CreatePath( const char *OSPath_ )
{
	// use va() to have a clean const char* prototype
	char *OSPath = va( "%s", OSPath_ );
	char *ofs;

	// make absolutely sure that it can't back up the path
	// FIXME: is c: allowed???
	if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) )
	{
		Com_Printf(_( "WARNING: refusing to create relative path \"%s\"\n"), OSPath );
		return qtrue;
	}

	for ( ofs = OSPath + 1; *ofs; ofs++ )
	{
		if ( *ofs == PATH_SEP )
		{
			// create the directory
			*ofs = 0;
			Sys_Mkdir( OSPath );
			*ofs = PATH_SEP;
		}
	}

	return qfalse;
}

/*
=================
FS_CopyFile

Copy a fully specified file from one place to another
=================
*/
void FS_CopyFile( char *fromOSPath, char *toOSPath )
{
	FILE *f;
	int  len;
	byte *buf;

	Com_Printf(_( "copy %s to %s\n"), fromOSPath, toOSPath );

	if ( strstr( fromOSPath, "journal.dat" ) || strstr( fromOSPath, "journaldata.dat" ) )
	{
		Com_Printf(_( "Ignoring journal files\n" ));
		return;
	}

	f = fopen( fromOSPath, "rb" );

	if ( !f )
	{
		return;
	}

	fseek( f, 0, SEEK_END );
	len = ftell( f );
	fseek( f, 0, SEEK_SET );

	buf = malloc( len );

	if ( fread( buf, 1, len, f ) != len )
	{
		Com_Error( ERR_FATAL, "Short read in FS_CopyFile()" );
	}

	fclose( f );

	if ( FS_CreatePath( toOSPath ) )
	{
		free( buf );
		return;
	}

	f = fopen( toOSPath, "wb" );

	if ( !f )
	{
		free( buf );  //DAJ free as well
		return;
	}

	if ( fwrite( buf, 1, len, f ) != len )
	{
		Com_Error( ERR_FATAL, "Short write in FS_CopyFile()" );
	}

	fclose( f );
	free( buf );
}

/*
=================
FS_CheckFilenameIsNotExecutable

ERR_FATAL if trying to maniuplate a file with the platform library extension
=================
*/
static void FS_CheckFilenameIsNotExecutable( const char *filename,
											 const char *function )
{
	// Check if the filename ends with the library extension
	if( !Q_stricmp( COM_GetExtension( filename ), DLL_EXT ) )
	{
		Com_Error( ERR_FATAL, "%s: Not allowed to manipulate '%s' due "
		"to %s extension\n", function, filename, DLL_EXT );
	}
}


/*
===========
FS_Remove

===========
*/
void FS_Remove( const char *osPath )
{
	remove( osPath );
}

// XreaL BEGIN

/*
===========
FS_HomeRemove
===========
*/
void FS_HomeRemove( const char *homePath )
{
	remove( FS_BuildOSPath( fs_homepath->string, fs_gamedir, homePath ) );
}

// XreaL END

/*
================
FS_FileExists

Tests if the file exists in the current gamedir, this DOES NOT
search the paths.  This is to determine if opening a file to write
(which always goes into the current gamedir) will cause any overwrites.
NOTE TTimo: this goes with FS_FOpenFileWrite for opening the file afterwards
================
*/
qboolean FS_FileExists( const char *file )
{
	FILE *f;
	char *testpath;

	testpath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, file );

	f = fopen( testpath, "rb" );

	if ( f )
	{
		fclose( f );
		return qtrue;
	}

	return qfalse;
}

/*
================
FS_SV_FileExists

Tests if the file exists
================
*/
qboolean FS_SV_FileExists( const char *file )
{
	FILE *f;
	char *testpath;

	testpath = FS_BuildOSPath( fs_homepath->string, file, "" );
	testpath[ strlen( testpath ) - 1 ] = '\0';

	f = fopen( testpath, "rb" );

	if ( f )
	{
		fclose( f );
		return qtrue;
	}

	return qfalse;
}

qboolean FS_OS_FileExists( const char *file )
{
	FILE *f;
	f = fopen( file, "rb" );

	if ( f )
	{
		fclose( f );
		return qtrue;
	}

	return qfalse;
}

/*
===========
FS_SV_FOpenFileWrite

===========
*/
fileHandle_t FS_SV_FOpenFileWrite( const char *filename )
{
	char         *ospath;
	fileHandle_t f;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	ospath = FS_BuildOSPath( fs_homepath->string, filename, "" );
	ospath[ strlen( ospath ) - 1 ] = '\0';

	f = FS_HandleForFile();
	fsh[ f ].zipFile = qfalse;

	if ( fs_debug->integer )
	{
		Com_Printf(_( "FS_SV_FOpenFileWrite: %s\n"), ospath );
	}

	if ( FS_CreatePath( ospath ) )
	{
		return 0;
	}

	Com_DPrintf( "writing to: %s\n", ospath );
	fsh[ f ].handleFiles.file.o = fopen( ospath, "wb" );

	Q_strncpyz( fsh[ f ].name, filename, sizeof( fsh[ f ].name ) );

	fsh[ f ].handleSync = qfalse;

	if ( !fsh[ f ].handleFiles.file.o )
	{
		f = 0;
	}

	return f;
}

/*
===========
FS_SV_FOpenFileRead
search for a file somewhere below the home path or base path
we search in that order, matching FS_SV_FOpenFileRead order
===========
*/
int FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp )
{
	char         *ospath;
	fileHandle_t f = 0;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	f = FS_HandleForFile();
	fsh[ f ].zipFile = qfalse;

	Q_strncpyz( fsh[ f ].name, filename, sizeof( fsh[ f ].name ) );

	// don't let sound stutter
	//S_ClearSoundBuffer();

	// search homepath
	ospath = FS_BuildOSPath( fs_homepath->string, filename, "" );
	// remove trailing slash
	ospath[ strlen( ospath ) - 1 ] = '\0';

	if ( fs_debug->integer )
	{
		Com_Printf(_( "FS_SV_FOpenFileRead (fs_homepath): %s\n"), ospath );
	}

	fsh[ f ].handleFiles.file.o = fopen( ospath, "rb" );
	fsh[ f ].handleSync = qfalse;

	if ( !fsh[ f ].handleFiles.file.o )
	{
		// NOTE TTimo on non *nix systems, fs_homepath == fs_basepath, might want to avoid
		if ( Q_stricmp( fs_homepath->string, fs_basepath->string ) )
		{
			// search basepath
			ospath = FS_BuildOSPath( fs_basepath->string, filename, "" );
			ospath[ strlen( ospath ) - 1 ] = '\0';

			if ( fs_debug->integer )
			{
				Com_Printf(_( "FS_SV_FOpenFileRead (fs_basepath): %s\n"), ospath );
			}

			fsh[ f ].handleFiles.file.o = fopen( ospath, "rb" );
			fsh[ f ].handleSync = qfalse;
		}

		if ( !fsh[ f ].handleFiles.file.o )
		{
			f = 0;
		}
	}

	*fp = f;

	if ( f )
	{
		return FS_filelength( f );
	}

	return 0;
}

/*
===========
FS_SV_Rename

===========
*/
void FS_SV_Rename( const char *from, const char *to )
{
	char *from_ospath, *to_ospath;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	// don't let sound stutter
	//S_ClearSoundBuffer();

	from_ospath = FS_BuildOSPath( fs_homepath->string, from, "" );
	to_ospath = FS_BuildOSPath( fs_homepath->string, to, "" );
	from_ospath[ strlen( from_ospath ) - 1 ] = '\0';
	to_ospath[ strlen( to_ospath ) - 1 ] = '\0';

	if ( fs_debug->integer )
	{
		Com_Printf(_( "FS_SV_Rename: %s --> %s\n"), from_ospath, to_ospath );
	}

	if ( rename( from_ospath, to_ospath ) )
	{
		// Failed, try copying it and deleting the original
		FS_CopyFile( from_ospath, to_ospath );
		FS_Remove( from_ospath );
	}
}

/*
===========
FS_Rename

===========
*/
void FS_Rename( const char *from, const char *to )
{
	char *from_ospath, *to_ospath;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	// don't let sound stutter
	//S_ClearSoundBuffer();

	from_ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, from );
	to_ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, to );

	if ( fs_debug->integer )
	{
		Com_Printf(_( "FS_Rename: %s --> %s\n"), from_ospath, to_ospath );
	}

	if ( rename( from_ospath, to_ospath ) )
	{
		// Failed, try copying it and deleting the original
		FS_CopyFile( from_ospath, to_ospath );
		FS_Remove( from_ospath );
	}
}

/*
==============
FS_FCloseFile

If the FILE pointer is an open pak file, leave it open.

For some reason, other DLLs can't just call fclose()
on files returned by FS_FOpenFile...
==============
*/
int FS_FCloseFile( fileHandle_t f )
{
	int ret = 0;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( fsh[ f ].zipFile == qtrue )
	{
		ret = ( unzCloseCurrentFile( fsh[ f ].handleFiles.file.z ) == UNZ_CRCERROR );

		if ( fsh[ f ].handleFiles.unique )
		{
			unzClose( fsh[ f ].handleFiles.file.z );
		}

		Com_Memset( &fsh[ f ], 0, sizeof( fsh[ f ] ) );

		return ret;
	}

	// we didn't find it as a pak, so close it as a unique file
	if ( fsh[ f ].handleFiles.file.o )
	{
		ret = fclose( fsh[ f ].handleFiles.file.o );
	}

	Com_Memset( &fsh[ f ], 0, sizeof( fsh[ f ] ) );

	return ret;
}

/*
===========
FS_FOpenFileWrite

===========
*/
fileHandle_t FS_FOpenFileWrite( const char *filename )
{
	char         *ospath;
	fileHandle_t f;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	f = FS_HandleForFile();
	fsh[ f ].zipFile = qfalse;

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer )
	{
		Com_Printf(_( "FS_FOpenFileWrite: %s\n"), ospath );
	}

	if ( FS_CreatePath( ospath ) )
	{
		return 0;
	}

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fsh[ f ].handleFiles.file.o = fopen( ospath, "wb" );

	Q_strncpyz( fsh[ f ].name, filename, sizeof( fsh[ f ].name ) );

	fsh[ f ].handleSync = qfalse;

	if ( !fsh[ f ].handleFiles.file.o )
	{
		f = 0;
	}

	return f;
}

/*
===========
FS_FOpenFileAppend

===========
*/
fileHandle_t FS_FOpenFileAppend( const char *filename )
{
	char         *ospath;
	fileHandle_t f;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	f = FS_HandleForFile();
	fsh[ f ].zipFile = qfalse;

	Q_strncpyz( fsh[ f ].name, filename, sizeof( fsh[ f ].name ) );

	// don't let sound stutter
	//S_ClearSoundBuffer();

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer )
	{
		Com_Printf(_( "FS_FOpenFileAppend: %s\n"), ospath );
	}

	if ( FS_CreatePath( ospath ) )
	{
		return 0;
	}

	fsh[ f ].handleFiles.file.o = fopen( ospath, "ab" );
	fsh[ f ].handleSync = qfalse;

	if ( !fsh[ f ].handleFiles.file.o )
	{
		f = 0;
	}

	return f;
}

/*
===========
FS_FOpenFileWrite

===========
*/
int FS_FOpenFileDirect( const char *filename, fileHandle_t *f )
{
	int  r;
	char *ospath;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	*f = FS_HandleForFile();
	fsh[ *f ].zipFile = qfalse;

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer )
	{
		Com_Printf(_( "FS_FOpenFileDirect: %s\n"), ospath );
	}

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fsh[ *f ].handleFiles.file.o = fopen( ospath, "rb" );

	if ( !fsh[ *f ].handleFiles.file.o )
	{
		*f = 0;
		return 0;
	}

	fseek( fsh[ *f ].handleFiles.file.o, 0, SEEK_END );
	r = ftell( fsh[ *f ].handleFiles.file.o );
	fseek( fsh[ *f ].handleFiles.file.o, 0, SEEK_SET );

	return r;
}

/*
===========
FS_FOpenFileUpdate
===========
*/
fileHandle_t FS_FOpenFileUpdate( const char *filename, int *length )
{
	char         *ospath;
	fileHandle_t f;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	f = FS_HandleForFile();
	fsh[ f ].zipFile = qfalse;

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( fs_debug->integer )
	{
		Com_Printf(_( "FS_FOpenFileWrite: %s\n"), ospath );
	}

	if ( FS_CreatePath( ospath ) )
	{
		return 0;
	}

	// enabling the following line causes a recursive function call loop
	// when running with +set logfile 1 +set developer 1
	//Com_DPrintf( "writing to: %s\n", ospath );
	fsh[ f ].handleFiles.file.o = fopen( ospath, "wb" );

	if ( !fsh[ f ].handleFiles.file.o )
	{
		f = 0;
	}

	return f;
}

/*
===========
FS_FCreateOpenPipeFile

===========
*/
fileHandle_t FS_FCreateOpenPipeFile( const char *filename ) {
	char         *ospath;
	FILE         *fifo;
	fileHandle_t  f;
	
	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}
	
	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;
	
	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );
	
	// don't let sound stutter
	S_ClearSoundBuffer();
	
	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );
	
	if ( fs_debug->integer )
	{
		Com_Printf( "FS_FCreateOpenPipeFile: %s\n", ospath );
	}
	
	FS_CheckFilenameIsNotExecutable( ospath, __func__ );
	
	fifo = Sys_Mkfifo( ospath );
	if( fifo )
	{
		fsh[f].handleFiles.file.o = fifo;
		fsh[f].handleSync = qfalse;
	}
	else
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: Could not create new com_pipefile at %s. "
		"com_pipefile will not be used.\n", ospath );
		f = 0;
	}
	
	return f;
}


/*
===========
FS_FilenameCompare

Ignore case and separator char distinctions
===========
*/
qboolean FS_FilenameCompare( const char *s1, const char *s2 )
{
	int c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if ( c1 >= 'a' && c1 <= 'z' )
		{
			c1 -= ( 'a' - 'A' );
		}

		if ( c2 >= 'a' && c2 <= 'z' )
		{
			c2 -= ( 'a' - 'A' );
		}

		if ( c1 == '\\' || c1 == ':' )
		{
			c1 = '/';
		}

		if ( c2 == '\\' || c2 == ':' )
		{
			c2 = '/';
		}

		if ( c1 != c2 )
		{
			return qtrue; // strings not equal
		}
	}
	while ( c1 );

	return qfalse; // strings are equal
}

/*
==========
FS_ShiftStr
perform simple string shifting to avoid scanning from the exe
==========
*/
char *FS_ShiftStr( const char *string, int shift )
{
	static char buf[ MAX_STRING_CHARS ];
	int         i, l;

	l = strlen( string );

	for ( i = 0; i < l; i++ )
	{
		buf[ i ] = string[ i ] + shift;
	}

	buf[ i ] = '\0';
	return buf;
}

/*
===========
FS_FOpenFileRead

Finds the file in the search path.
Returns filesize and an open FILE pointer.
Used for streaming data out of either a
separate file or a ZIP file.
===========
*/
extern qboolean com_fullyInitialized;

// see FS_FOpenFileRead_Filtered
static int      fs_filter_flag = 0;

static qboolean FS_CheckUIImageFile( const char *filename )
{
	int l = strlen( filename );

	if ( !Q_stricmpn( filename, "ui/assets/", 10 ) &&
	   ( !Q_stricmp( filename + l - 4, ".tga" ) ||
		 !Q_stricmp( filename + l - 4, ".png" ) ||
		 !Q_stricmp( filename + l - 4, ".jpg" ) ||
		 !Q_stricmp( filename + l - 5, ".jpeg" ) ||
		 !Q_stricmp( filename + l - 5, ".webp" ) ) )
	{
		return qtrue;
	}

	return qfalse;
}
		

int FS_FOpenFileRead( const char *filename, fileHandle_t *file, qboolean uniqueFILE )
{
	searchpath_t *search;
	char         *netpath;
	pack_t       *pak;
	fileInPack_t *pakFile;
	directory_t  *dir;
	long         hash = 0;
	FILE         *temp;
	int          l;
	char         demoExt[ 16 ];

	hash = 0;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	// TTimo - NOTE
	// when checking for file existence, it's probably safer to use FS_FileExists, as I'm not
	// sure this chunk of code is really up to date with everything
	if ( file == NULL )
	{
		// just wants to see if file is there
		for ( search = fs_searchpaths; search; search = search->next )
		{
			//
			if ( search->pack )
			{
				hash = FS_HashFileName( filename, search->pack->hashSize );
			}

			// is the element a pak file?
			if ( search->pack && search->pack->hashTable[ hash ] )
			{
				if ( fs_filter_flag & FS_EXCLUDE_PK3 )
				{
					continue;
				}

				// look through all the pak file elements
				pak = search->pack;
				pakFile = pak->hashTable[ hash ];

				do
				{
					// case and separator insensitive comparisons
					if ( !FS_FilenameCompare( pakFile->name, filename ) )
					{
						// found it!
						return qtrue;
					}

					pakFile = pakFile->next;
				}
				while ( pakFile != NULL );
			}
			else if ( search->dir )
			{
				if ( fs_filter_flag & FS_EXCLUDE_DIR )
				{
					continue;
				}

				dir = search->dir;

				netpath = FS_BuildOSPath( dir->path, dir->gamedir, filename );
				temp = fopen( netpath, "rb" );

				if ( !temp )
				{
					continue;
				}

				fclose( temp );
				return qtrue;
			}
		}

		return qfalse;
	}

	if ( !filename )
	{
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'filename' parameter passed" );
	}

	//Com_sprintf( demoExt, sizeof( demoExt ), ".dm_%d",PROTOCOL_VERSION );
	// qpaths are not supposed to have a leading slash
	if ( filename[ 0 ] == '/' || filename[ 0 ] == '\\' )
	{
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if ( strstr( filename, ".." ) || strstr( filename, "::" ) )
	{
		*file = 0;
		return -1;
	}

	// make sure the q3key file is only readable by the quake3.exe at initialization
	// any other time the key should only be accessed in memory using the provided functions
	if ( com_fullyInitialized && strstr( filename, "etkey" ) )
	{
		*file = 0;
		return -1;
	}

	//
	// search through the path, one element at a time
	//

	*file = FS_HandleForFile();
	fsh[ *file ].handleFiles.unique = uniqueFILE;

	for ( search = fs_searchpaths; search; search = search->next )
	{
		//
		if ( search->pack )
		{
			hash = FS_HashFileName( filename, search->pack->hashSize );
		}

		// is the element a pak file?
		if ( search->pack && search->pack->hashTable[ hash ] )
		{
			if ( fs_filter_flag & FS_EXCLUDE_PK3 )
			{
				continue;
			}

			// disregard if it doesn't match one of the allowed pure pak files
			if ( !FS_PakIsPure( search->pack ) )
			{
				continue;
			}

			// look through all the pak file elements
			pak = search->pack;
			pakFile = pak->hashTable[ hash ];

			do
			{
				// case and separator insensitive comparisons
				if ( !FS_FilenameCompare( pakFile->name, filename ) )
				{
					// found it!

					// mark the pak as having been referenced and mark specifics on cgame and ui
					// shaders, txt, arena files  by themselves do not count as a reference as
					// these are loaded from all pk3s
					// from every pk3 file..
					l = strlen( filename );

					if ( !( pak->referenced & FS_GENERAL_REF ) )
					{
						if ( Q_stricmp( filename + l - 7, ".shader" ) != 0 &&
						     Q_stricmp( filename + l - 4, ".txt" ) != 0 &&
						     Q_stricmp( filename + l - 4, ".ttf" ) != 0 &&
						     Q_stricmp( filename + l - 4, ".otf" ) != 0 &&
						     Q_stricmp( filename + l - 4, ".cfg" ) != 0 &&
						     Q_stricmp( filename + l - 7, ".config" ) != 0 &&
						     strstr( filename, "levelshots" ) == NULL &&
						     Q_stricmp( filename + l - 4, ".bot" ) != 0 &&
						     Q_stricmp( filename + l - 6, ".arena" ) != 0 &&
						     Q_stricmp( filename + l - 5, ".menu" ) != 0 &&
						     Q_stricmp( filename + l - 3, ".po" ) != 0 &&
						     Q_stricmp( filename, "qagame.qvm" ) != 0  &&
						     !FS_CheckUIImageFile( filename ) )
						{
							pak->referenced |= FS_GENERAL_REF;
						}
					}

					// cgame module
					if ( !( pak->referenced & FS_CGAME_REF ) && !Q_stricmp( filename, "vm/cgame.qvm" ) )
					{
						pak->referenced |= FS_CGAME_REF;
					}

					// ui module
					if ( !( pak->referenced & FS_UI_REF ) && !Q_stricmp( filename, "vm/ui.qvm" ) )
					{
						pak->referenced |= FS_UI_REF;
					}

					if ( uniqueFILE )
					{
						// open a new file on the pakfile
						fsh[ *file ].handleFiles.file.z = unzOpen( pak->pakFilename );

						if ( fsh[ *file ].handleFiles.file.z == NULL )
						{
							Com_Error( ERR_FATAL, "Couldn't reopen %s", pak->pakFilename );
						}
					}
					else
					{
						fsh[ *file ].handleFiles.file.z = pak->handle;
					}

					Q_strncpyz( fsh[ *file ].name, filename, sizeof( fsh[ *file ].name ) );
					fsh[ *file ].zipFile = qtrue;

					// set the file position in the zip file (also sets the current file info)
					unzSetOffset( fsh[ *file ].handleFiles.file.z, pakFile->pos );

					// open the file in the zip
					unzOpenCurrentFile( fsh[ *file ].handleFiles.file.z );
					fsh[ *file ].zipFilePos = pakFile->pos;

					if ( fs_debug->integer )
					{
						Com_Printf(_( "FS_FOpenFileRead: %s (found in '%s')\n"),
						            filename, pak->pakFilename );
					}

					return pakFile->len;
				}

				pakFile = pakFile->next;
			}
			while ( pakFile != NULL );
		}
		else if ( search->dir )
		{
			if ( fs_filter_flag & FS_EXCLUDE_DIR )
			{
				continue;
			}

			// check a file in the directory tree

			// if the filesystem is configured for pure (fs_numServerPaks != 0), then
			// the only files we will allow to come from the directory are .cfg, .menu, etc. files
			l = strlen( filename );

			if ( fs_numServerPaks )
			{
				if ( Q_stricmp( filename + l - 4, ".cfg" )  // for config files
				     && Q_stricmp( filename + l - 4, ".ttf" )
				     && Q_stricmp( filename + l - 4, ".otf" )
				     && Q_stricmp( filename + l - 5, ".menu" )  // menu files
				     && Q_stricmp( filename + l - 5, ".game" )  // menu files
				     && Q_stricmp( filename + l - strlen( demoExt ), demoExt )   // menu files
				     && Q_stricmp( filename + l - 4, ".dat" )  // for journal files
				     && Q_stricmp( filename + l - 8, "bots.txt" )
				     && Q_stricmp( filename + l - 8, ".botents" )
				     && Q_stricmp( filename + l - 3, ".po" )
				     && !FS_CheckUIImageFile( filename )
				   )
				{
					continue;
				}
			}

			dir = search->dir;

			netpath = FS_BuildOSPath( dir->path, dir->gamedir, filename );
			fsh[ *file ].handleFiles.file.o = fopen( netpath, "rb" );

			if ( !fsh[ *file ].handleFiles.file.o )
			{
				continue;
			}

			if ( Q_stricmp( filename + l - 4, ".cfg" )  // for config files
			     && Q_stricmp( filename + l - 4, ".ttf" ) != 0
			     && Q_stricmp( filename + l - 4, ".otf" ) != 0
			     && Q_stricmp( filename + l - 5, ".menu" )  // menu files
			     && Q_stricmp( filename + l - 5, ".game" )  // menu files
			     && Q_stricmp( filename + l - strlen( demoExt ), demoExt )   // menu files
			     && Q_stricmp( filename + l - 4, ".dat" )
			     && Q_stricmp( filename + l - 8, ".botents" )
			     && Q_stricmp( filename + l - 3, ".po" )
				 && !FS_CheckUIImageFile( filename )
			     /*&& !strstr( filename, "botfiles" )*/ ) // RF, need this for dev
			{
				fs_fakeChkSum = random();
			}

			Q_strncpyz( fsh[ *file ].name, filename, sizeof( fsh[ *file ].name ) );
			fsh[ *file ].zipFile = qfalse;

			if ( fs_debug->integer )
			{
				Com_Printf(_( "FS_FOpenFileRead: %s (found in '%s/%s')\n"), filename,
				            dir->path, dir->gamedir );
			}

			return FS_filelength( *file );
		}
	}
	if( fs_debug->integer )
	{
		Com_Printf( "Can't find %s\n", filename );
	}

	*file = 0;
	return -1;
}

int FS_FOpenFileRead_Filtered( const char *qpath, fileHandle_t *file, qboolean uniqueFILE, int filter_flag )
{
	int ret;

	fs_filter_flag = filter_flag;
	ret = FS_FOpenFileRead( qpath, file, uniqueFILE );
	fs_filter_flag = 0;

	return ret;
}

/*
==============
FS_AllowDeletion
==============
*/
qboolean FS_AllowDeletion( char *filename )
{
	// for safety, only allow deletion from the save, profiles and demo directory
	if ( Q_strncmp( filename, "save/", 5 ) != 0 &&
	     Q_strncmp( filename, "profiles/", 9 ) != 0 &&
	     Q_strncmp( filename, "demos/", 6 ) != 0 )
	{
		return qfalse;
	}

	return qtrue;
}

/*
==============
FS_DeleteDir
==============
*/
int FS_DeleteDir( char *dirname, qboolean nonEmpty, qboolean recursive )
{
	char        *ospath;
	char        **pFiles = NULL;
	int         i, nFiles = 0;
	static char *root = "/";

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( !dirname || dirname[ 0 ] == 0 )
	{
		return 0;
	}

	if ( !FS_AllowDeletion( dirname ) )
	{
		return 0;
	}

	if ( recursive )
	{
		ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, dirname );
		pFiles = Sys_ListFiles( ospath, root, NULL, &nFiles, qfalse );

		for ( i = 0; i < nFiles; i++ )
		{
			char temp[ MAX_OSPATH ];

			if ( !Q_stricmp( pFiles[ i ], ".." ) || !Q_stricmp( pFiles[ i ], "." ) )
			{
				continue;
			}

			Com_sprintf( temp, sizeof( temp ), "%s/%s", dirname, pFiles[ i ] );

			if ( !FS_DeleteDir( temp, nonEmpty, recursive ) )
			{
				return 0;
			}
		}

		Sys_FreeFileList( pFiles );
	}

	if ( nonEmpty )
	{
		ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, dirname );
		pFiles = Sys_ListFiles( ospath, NULL, NULL, &nFiles, qfalse );

		for ( i = 0; i < nFiles; i++ )
		{
			ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, va( "%s/%s", dirname, pFiles[ i ] ) );

			if ( remove( ospath ) == -1 )  // failure
			{
				return 0;
			}
		}

		Sys_FreeFileList( pFiles );
	}

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, dirname );

	if ( Q_rmdir( ospath ) == 0 )
	{
		return 1;
	}

	return 0;
}

/*
==============
FS_OSStatFile
Test a file given OS path:
returns -1 if not found
returns 1 if directory
returns 0 otherwise
==============
*/
#ifdef WIN32
int FS_OSStatFile( char *ospath )
{
	struct _stat stat;

	if ( _stat( ospath, &stat ) == -1 )
	{
		return -1;
	}

	if ( stat.st_mode & _S_IFDIR )
	{
		return 1;
	}

	return 0;
}

#else
int FS_OSStatFile( char *ospath )
{
	struct stat stat_buf;

	if ( stat( ospath, &stat_buf ) == -1 )
	{
		return -1;
	}

	if ( S_ISDIR( stat_buf.st_mode ) )
	{
		return 1;
	}

	return 0;
}

#endif

/*
==============
FS_Delete
TTimo - this was not in the 1.30 filesystem code
using fs_homepath for the file to remove
==============
*/
int FS_Delete( char *filename )
{
	char *ospath;
	int  stat;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( !filename || filename[ 0 ] == 0 )
	{
		return 0;
	}

	if ( !FS_AllowDeletion( filename ) )
	{
		return 0;
	}

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	stat = FS_OSStatFile( ospath );

	if ( stat == -1 )
	{
		return 0;
	}

	if ( stat == 1 )
	{
		return ( FS_DeleteDir( filename, qtrue, qtrue ) );
	}
	else
	{
		if ( remove( ospath ) != -1 )  // success
		{
			return 1;
		}
	}

	return 0;
}

/*
=================
FS_Read

Properly handles partial reads
=================
*/
int FS_Read2( void *buffer, int len, fileHandle_t f )
{
	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( !f )
	{
		return 0;
	}

	if ( fsh[ f ].streamed )
	{
		int r;
		fsh[ f ].streamed = qfalse;
		r = FS_Read( buffer, len, f );
		fsh[ f ].streamed = qtrue;
		return r;
	}
	else
	{
		return FS_Read( buffer, len, f );
	}
}

int FS_Read( void *buffer, int len, fileHandle_t f )
{
	int  block, remaining;
	int  read;
	byte *buf;
	int  tries;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( !f )
	{
		return 0;
	}

	buf = ( byte * ) buffer;
	fs_readCount += len;

	if ( fsh[ f ].zipFile == qfalse )
	{
		remaining = len;
		tries = 0;

		while ( remaining )
		{
			block = remaining;
//			read = fread (buf, block, 1, fsh[f].handleFiles.file.o);
			read = fread( buf, 1, block, fsh[ f ].handleFiles.file.o );

			if ( read == 0 )
			{
				// we might have been trying to read from a CD, which
				// sometimes returns a 0 read on windows
				if ( !tries )
				{
					tries = 1;
				}
				else
				{
					return len - remaining; //Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
				}
			}

			if ( read == -1 )
			{
				Com_Error( ERR_FATAL, "FS_Read: -1 bytes read" );
			}

			remaining -= read;
			buf += read;
		}

		return len;
	}
	else
	{
		return unzReadCurrentFile( fsh[ f ].handleFiles.file.z, buffer, len );
	}
}

/*
=================
FS_Write

Properly handles partial writes
=================
*/
int FS_Write( const void *buffer, int len, fileHandle_t h )
{
	int  block, remaining;
	int  written;
	byte *buf;
	int  tries;
	FILE *f;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( !h )
	{
		return 0;
	}

	f = FS_FileForHandle( h );
	buf = ( byte * ) buffer;

	remaining = len;
	tries = 0;

	while ( remaining )
	{
		block = remaining;
		written = fwrite( buf, 1, block, f );

		if ( written == 0 )
		{
			if ( !tries )
			{
				tries = 1;
			}
			else
			{
				Com_Printf(_( "FS_Write: 0 bytes written (%d attempted)\n"), block );
				return 0;
			}
		}

		if ( written < 0 )
		{
			Com_Printf(_( "FS_Write: %d bytes written (%d attempted)\n"), written, block ); // FIXME PLURAL
			return 0;
		}

		remaining -= written;
		buf += written;
	}

	if ( fsh[ h ].handleSync )
	{
		fflush( f );
	}

	return len;
}

void QDECL PRINTF_LIKE(2) FS_Printf( fileHandle_t h, const char *fmt, ... )
{
	va_list argptr;
	char    msg[ MAXPRINTMSG ];

	va_start( argptr, fmt );
	Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );

	FS_Write( msg, strlen( msg ), h );
}

#define PK3_SEEK_BUFFER_SIZE 65536

/*
=================
FS_Seek

=================
*/
int FS_Seek( fileHandle_t f, long offset, int origin )
{
	int _origin;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( fsh[ f ].streamed )
	{
		fsh[ f ].streamed = qfalse;
		FS_Seek( f, offset, origin );
		fsh[ f ].streamed = qtrue;
	}

	if ( fsh[ f ].zipFile == qtrue )
	{
		//FIXME: this is incomplete and really, really
		//crappy (but better than what was here before)
		byte buffer[ PK3_SEEK_BUFFER_SIZE ];
		int  remainder = offset;

		if ( offset < 0 || origin == FS_SEEK_END )
		{
			Com_Error( ERR_FATAL, "Negative offsets and FS_SEEK_END not implemented "
			           "for FS_Seek on pk3 file contents" );
		}

		switch ( origin )
		{
			case FS_SEEK_SET:
				unzSetOffset( fsh[ f ].handleFiles.file.z, fsh[ f ].zipFilePos );
				unzOpenCurrentFile( fsh[ f ].handleFiles.file.z );
				//fallthrough

			case FS_SEEK_CUR:
				while ( remainder > PK3_SEEK_BUFFER_SIZE )
				{
					FS_Read( buffer, PK3_SEEK_BUFFER_SIZE, f );
					remainder -= PK3_SEEK_BUFFER_SIZE;
				}

				FS_Read( buffer, remainder, f );
				return offset;

			default:
				Com_Error( ERR_FATAL, "Bad origin in FS_Seek" );
		}
	}
	else
	{
		FILE *file;
		file = FS_FileForHandle( f );

		switch ( origin )
		{
			case FS_SEEK_CUR:
				_origin = SEEK_CUR;
				break;

			case FS_SEEK_END:
				_origin = SEEK_END;
				break;

			case FS_SEEK_SET:
				_origin = SEEK_SET;
				break;

			default:
				Com_Error( ERR_FATAL, "Bad origin in FS_Seek" );
		}

		return fseek( file, offset, _origin );
	}
}

/*
======================================================================================

CONVENIENCE FUNCTIONS FOR ENTIRE FILES

======================================================================================
*/

int FS_FileIsInPAK( const char *filename, int *pChecksum )
{
	searchpath_t *search;
	pack_t       *pak;
	fileInPack_t *pakFile;
	long         hash = 0;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( !filename )
	{
		Com_Error( ERR_FATAL, "FS_FOpenFileRead: NULL 'filename' parameter passed" );
	}

	// qpaths are not supposed to have a leading slash
	if ( filename[ 0 ] == '/' || filename[ 0 ] == '\\' )
	{
		filename++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if ( strstr( filename, ".." ) || strstr( filename, "::" ) )
	{
		return -1;
	}

	//
	// search through the path, one element at a time
	//

	for ( search = fs_searchpaths; search; search = search->next )
	{
		//
		if ( search->pack )
		{
			hash = FS_HashFileName( filename, search->pack->hashSize );
		}

		// is the element a pak file?
		if ( search->pack && search->pack->hashTable[ hash ] )
		{
			// disregard if it doesn't match one of the allowed pure pak files
			if ( !FS_PakIsPure( search->pack ) )
			{
				continue;
			}

			// look through all the pak file elements
			pak = search->pack;
			pakFile = pak->hashTable[ hash ];

			do
			{
				// case and separator insensitive comparisons
				if ( !FS_FilenameCompare( pakFile->name, filename ) )
				{
					if ( pChecksum )
					{
						*pChecksum = pak->pure_checksum;
					}

					return 1;
				}

				pakFile = pakFile->next;
			}
			while ( pakFile != NULL );
		}
	}

	return -1;
}

/*
============
FS_ReadFile
FS_ReadFileCheck

Filename are relative to the quake search path
a null buffer will just return the file length without loading
============
*/
static int FS_ReadFile_Internal( const char *qpath, void **buffer, qboolean check )
{
	fileHandle_t h;
	byte          *buf;
	qboolean     isConfig;
	int          len, ret;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( !qpath || !qpath[ 0 ] )
	{
		Com_Error( ERR_FATAL, "FS_ReadFile with empty name" );
	}

	buf = NULL; // quiet compiler warning

	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	if ( strstr( qpath, ".cfg" ) )
	{
		isConfig = qtrue;

		if ( com_journal && com_journal->integer == 2 )
		{
			int r;

			Com_DPrintf( "Loading %s from journal file.\n", qpath );
			r = FS_Read( &len, sizeof( len ), com_journalDataFile );

			if ( r != sizeof( len ) )
			{
				if ( buffer != NULL )
				{
					*buffer = NULL;
				}

				return -1;
			}

			// if the file didn't exist when the journal was created
			if ( !len )
			{
				if ( buffer == NULL )
				{
					return 1; // hack for old journal files
				}

				*buffer = NULL;
				return -1;
			}

			if ( buffer == NULL )
			{
				return len;
			}

			buf = Hunk_AllocateTempMemory( len + 1 );
			*buffer = buf;

			r = FS_Read( buf, len, com_journalDataFile );

			if ( r != len )
			{
				Com_Error( ERR_FATAL, "Read from journalDataFile failed" );
			}

			fs_loadCount++;
			fs_loadStack++;

			// guarantee that it will have a trailing 0 for string operations
			buf[ len ] = 0;

			return len;
		}
	}
	else
	{
		isConfig = qfalse;
	}

	// look for it in the filesystem or pack files
	len = FS_FOpenFileRead( qpath, &h, qfalse );

	if ( h == 0 )
	{
		if ( buffer )
		{
			*buffer = NULL;
		}

		// if we are journalling and it is a config file, write a zero to the journal file
		if ( isConfig && com_journal && com_journal->integer == 1 )
		{
			Com_DPrintf( "Writing zero for %s to journal file.\n", qpath );
			len = 0;
			FS_Write( &len, sizeof( len ), com_journalDataFile );
			FS_Flush( com_journalDataFile );
		}

		return -1;
	}

	if ( !buffer )
	{
		if ( isConfig && com_journal && com_journal->integer == 1 )
		{
			Com_DPrintf( "Writing len for %s to journal file.\n", qpath );
			FS_Write( &len, sizeof( len ), com_journalDataFile );
			FS_Flush( com_journalDataFile );
		}

		FS_FCloseFile( h );
		return len;
	}

	fs_loadCount++;
	fs_loadStack++;

	buf = Hunk_AllocateTempMemory( len + 1 );
	*buffer = buf;

	FS_Read( buf, len, h );

	// guarantee that it will have a trailing 0 for string operations
	buf[ len ] = 0;
	ret = FS_FCloseFile( h );

	// if we are journalling and it is a config file, write it to the journal file
	if ( isConfig && com_journal && com_journal->integer == 1 )
	{
		Com_DPrintf( "Writing %s to journal file.\n", qpath );
		FS_Write( &len, sizeof( len ), com_journalDataFile );
		FS_Write( buf, len, com_journalDataFile );
		FS_Flush( com_journalDataFile );
	}

	return ( check && ret ) ? -2 - len : len;
}

int FS_ReadFile( const char *qpath, void **buffer )
{
        return FS_ReadFile_Internal( qpath, buffer, qfalse );
}

int FS_ReadFileCheck( const char *qpath, void **buffer )
{
        return FS_ReadFile_Internal( qpath, buffer, qtrue );
}

/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile( void *buffer )
{
	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( !buffer )
	{
		Com_Error( ERR_FATAL, "FS_FreeFile( NULL )" );
	}

	fs_loadStack--;

	Hunk_FreeTempMemory( buffer );

	// if all of our temp files are free, clear all of our space
	if ( fs_loadStack == 0 )
	{
		Hunk_ClearTempMemory();
	}
}

/*
============
FS_WriteFile

Filename are relative to the quake search path
============
*/
void FS_WriteFile( const char *qpath, const void *buffer, int size )
{
	fileHandle_t f;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( !qpath || !buffer )
	{
		Com_Error( ERR_FATAL, "FS_WriteFile: NULL parameter" );
	}

	f = FS_FOpenFileWrite( qpath );

	if ( !f )
	{
		Com_Printf(_( "Failed to open %s\n"), qpath );
		return;
	}

	FS_Write( buffer, size, f );

	FS_FCloseFile( f );
}

/*
==========================================================================

ZIP FILE LOADING

==========================================================================
*/

/*
=================
FS_LoadZipFile

Creates a new pak_t in the search chain for the contents
of a zip file.
=================
*/
static pack_t *FS_LoadZipFile( char *zipfile, const char *basename )
{
	fileInPack_t    *buildBuffer;
	pack_t          *pack;
	unzFile         uf;
	int             err;
	unz_global_info gi;
	char            filename_inzip[ MAX_ZPATH ];
	unz_file_info   file_info;
	int             i, len;
	long            hash;
	int             fs_numHeaderLongs;
	int             *fs_headerLongs;
	char            *namePtr;

	fs_numHeaderLongs = 0;

	//  open pak
	uf = unzOpen( zipfile );
	err = unzGetGlobalInfo( uf, &gi );

	if ( err != UNZ_OK )
	{
		return NULL;
	}

	fs_packFiles += gi.number_entry;

	len = 0;
	unzGoToFirstFile( uf );

	for ( i = 0; i < gi.number_entry; i++ )
	{
		err = unzGetCurrentFileInfo( uf, &file_info, filename_inzip, sizeof( filename_inzip ), NULL, 0, NULL, 0 );

		if ( err != UNZ_OK )
		{
			break;
		}

		len += strlen( filename_inzip ) + 1;
		unzGoToNextFile( uf );
	}

	buildBuffer = Z_Malloc( ( gi.number_entry * sizeof( fileInPack_t ) ) + len );
	namePtr = ( ( char * ) buildBuffer ) + gi.number_entry * sizeof( fileInPack_t );
	fs_headerLongs = Z_Malloc( ( gi.number_entry + 1 ) * sizeof( int ) );
	fs_headerLongs[ fs_numHeaderLongs++ ] = LittleLong( fs_checksumFeed );  // get the hash table size from the number of files in the zip

	// get the hash table size from the number of files in the zip
	// because lots of custom pk3 files have less than 32 or 64 files
	for ( i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1 )
	{
		if ( i > gi.number_entry )
		{
			break;
		}
	}

	pack = Z_Malloc( sizeof( pack_t ) + i * sizeof( fileInPack_t * ) );
	pack->hashSize = i;
	pack->hashTable = ( fileInPack_t ** )( ( ( char * ) pack ) + sizeof( pack_t ) );

	for ( i = 0; i < pack->hashSize; i++ )
	{
		pack->hashTable[ i ] = NULL;
	}

	Q_strncpyz( pack->pakFilename, zipfile, sizeof( pack->pakFilename ) );
	Q_strncpyz( pack->pakBasename, basename, sizeof( pack->pakBasename ) );

	// strip .pk3 if needed
	if ( strlen( pack->pakBasename ) > 4 && !Q_stricmp( pack->pakBasename + strlen( pack->pakBasename ) - 4, ".pk3" ) )
	{
		pack->pakBasename[ strlen( pack->pakBasename ) - 4 ] = 0;
	}

	pack->handle = uf;
	pack->numfiles = gi.number_entry;
	unzGoToFirstFile( uf );

	for ( i = 0; i < gi.number_entry; i++ )
	{
		err = unzGetCurrentFileInfo( uf, &file_info, filename_inzip, sizeof( filename_inzip ), NULL, 0, NULL, 0 );

		if ( err != UNZ_OK )
		{
			break;
		}

		if ( file_info.uncompressed_size > 0 )
		{
			fs_headerLongs[ fs_numHeaderLongs++ ] = LittleLong( file_info.crc );
		}

		Q_strlwr( filename_inzip );
		hash = FS_HashFileName( filename_inzip, pack->hashSize );
		buildBuffer[ i ].name = namePtr;
		strcpy( buildBuffer[ i ].name, filename_inzip );
		namePtr += strlen( filename_inzip ) + 1;
		// store the file position in the zip
		buildBuffer[ i ].pos = unzGetOffset( uf );
		buildBuffer[ i ].len = file_info.uncompressed_size;
		buildBuffer[ i ].next = pack->hashTable[ hash ];
		pack->hashTable[ hash ] = &buildBuffer[ i ];
		unzGoToNextFile( uf );
	}

	pack->checksum = Com_BlockChecksum( &fs_headerLongs[ 1 ], 4 * ( fs_numHeaderLongs - 1 ) );
	pack->pure_checksum = Com_BlockChecksum( fs_headerLongs, 4 * fs_numHeaderLongs );
	pack->checksum = LittleLong( pack->checksum );
	pack->pure_checksum = LittleLong( pack->pure_checksum );

	Z_Free( fs_headerLongs );

	pack->buildBuffer = buildBuffer;
	return pack;
}

/*
=================================================================================

DIRECTORY SCANNING FUNCTIONS

=================================================================================
*/

#define MAX_FOUND_FILES 0x1000

static int FS_ReturnPath( const char *zname, char *zpath, int *depth )
{
	int len, at, newdep;

	newdep = 0;
	zpath[ 0 ] = 0;
	len = 0;
	at = 0;

	while ( zname[ at ] != 0 )
	{
		if ( zname[ at ] == '/' || zname[ at ] == '\\' )
		{
			len = at;
			newdep++;
		}

		at++;
	}

	strcpy( zpath, zname );
	zpath[ len ] = 0;
	*depth = newdep;

	return len;
}

/*
==================
FS_AddFileToList
==================
*/
static int FS_AddFileToList( char *name, char *list[ MAX_FOUND_FILES ], int nfiles )
{
	int i;

	if ( nfiles == MAX_FOUND_FILES - 1 )
	{
		return nfiles;
	}

	for ( i = 0; i < nfiles; i++ )
	{
		if ( !Q_stricmp( name, list[ i ] ) )
		{
			return nfiles; // already in the list
		}
	}

	list[ nfiles ] = CopyString( name );
	nfiles++;

	return nfiles;
}

/*
===============
FS_ListFilteredFiles

Returns a uniqued list of files that match the given criteria
from all search paths
===============
*/
char **FS_ListFilteredFiles( const char *path, const char *extension, char *filter, int *numfiles )
{
	int          nfiles;
	char         **listCopy;
	char         *list[ MAX_FOUND_FILES ];
	searchpath_t *search;
	int          i;
	int          pathLength;
	int          extensionLength;
	int          length, pathDepth, temp;
	pack_t       *pak;
	fileInPack_t *buildBuffer;
	char         zpath[ MAX_ZPATH ];

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( !path )
	{
		*numfiles = 0;
		return NULL;
	}

	if ( !extension )
	{
		extension = "";
	}

	pathLength = strlen( path );

	if ( path[ pathLength - 1 ] == '\\' || path[ pathLength - 1 ] == '/' )
	{
		pathLength--;
	}

	extensionLength = strlen( extension );
	nfiles = 0;
	FS_ReturnPath( path, zpath, &pathDepth );

	//
	// search through the path, one element at a time, adding to list
	//
	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file?
		if ( search->pack )
		{
			//ZOID:  If we are pure, don't search for files on paks that
			// aren't on the pure list
			if ( !FS_PakIsPure( search->pack ) )
			{
				continue;
			}

			// look through all the pak file elements
			pak = search->pack;
			buildBuffer = pak->buildBuffer;

			for ( i = 0; i < pak->numfiles; i++ )
			{
				char *name;
				int  zpathLen, depth;

				// check for directory match
				name = buildBuffer[ i ].name;

				//
				if ( filter )
				{
					// case insensitive
					if ( !Com_FilterPath( filter, name, qfalse ) )
					{
						continue;
					}

					// unique the match
					nfiles = FS_AddFileToList( name, list, nfiles );
				}
				else
				{
					zpathLen = FS_ReturnPath( name, zpath, &depth );

					if ( ( depth - pathDepth ) > 2 || pathLength > zpathLen || Q_stricmpn( name, path, pathLength ) )
					{
						continue;
					}

					// check for extension match
					length = strlen( name );

					if ( length < extensionLength )
					{
						continue;
					}

					if ( Q_stricmp( name + length - extensionLength, extension ) )
					{
						continue;
					}

					// unique the match

					temp = pathLength;

					if ( pathLength )
					{
						temp++; // include the '/'
					}

					nfiles = FS_AddFileToList( name + temp, list, nfiles );
				}
			}
		}
		else if ( search->dir ) // scan for files in the filesystem
		{
			char *netpath;
			int  numSysFiles;
			char **sysFiles;
			char *name;

			// don't scan directories for files if we are pure
			if ( fs_numServerPaks )
			{
				continue;
			}
			else
			{
				netpath = FS_BuildOSPath( search->dir->path, search->dir->gamedir, path );
				sysFiles = Sys_ListFiles( netpath, extension, filter, &numSysFiles, qfalse );

				for ( i = 0; i < numSysFiles; i++ )
				{
					// unique the match
					name = sysFiles[ i ];
					nfiles = FS_AddFileToList( name, list, nfiles );
				}

				Sys_FreeFileList( sysFiles );
			}
		}
	}

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles )
	{
		return NULL;
	}

	listCopy = Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );

	for ( i = 0; i < nfiles; i++ )
	{
		listCopy[ i ] = list[ i ];
	}

	listCopy[ i ] = NULL;

	return listCopy;
}

/*
=================
FS_ListFiles
=================
*/
char **FS_ListFiles( const char *path, const char *extension, int *numfiles )
{
	return FS_ListFilteredFiles( path, extension, NULL, numfiles );
}

/*
=================
FS_FreeFileList
=================
*/
void FS_FreeFileList( char **list )
{
	int i;

	if ( !fs_searchpaths )
	{
		Com_Error( ERR_FATAL, "Filesystem call made without initialization" );
	}

	if ( !list )
	{
		return;
	}

	for ( i = 0; list[ i ]; i++ )
	{
		Z_Free( list[ i ] );
	}

	Z_Free( list );
}

/*
================
FS_GetFileList
================
*/
int FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize )
{
	int  nFiles, i, nTotal, nLen;
	char **pFiles = NULL;

	*listbuf = 0;
	nFiles = 0;
	nTotal = 0;

	if ( Q_stricmp( path, "$modlist" ) == 0 )
	{
		return FS_GetModList( listbuf, bufsize );
	}

	pFiles = FS_ListFiles( path, extension, &nFiles );

	for ( i = 0; i < nFiles; i++ )
	{
		nLen = strlen( pFiles[ i ] ) + 1;

		if ( nTotal + nLen + 1 < bufsize )
		{
			strcpy( listbuf, pFiles[ i ] );
			listbuf += nLen;
			nTotal += nLen;
		}
		else
		{
			nFiles = i;
			break;
		}
	}

	FS_FreeFileList( pFiles );

	return nFiles;
}

/*
=======================
Sys_ConcatenateFileLists

mkv: Naive implementation. Concatenates three lists into a
         new list, and frees the old lists from the heap.
bk001129 - from cvs1.17 (mkv)

FIXME TTimo those two should move to common.c next to Sys_ListFiles
=======================
 */
static unsigned int Sys_CountFileList( char **list )
{
	int i = 0;

	if ( list )
	{
		while ( *list )
		{
			list++;
			i++;
		}
	}

	return i;
}

static char **Sys_ConcatenateFileLists( char **list0, char **list1, char **list2 )
{
	int totalLength = 0;
	char **cat = NULL, **dst, **src;

	totalLength += Sys_CountFileList( list0 );
	totalLength += Sys_CountFileList( list1 );
	totalLength += Sys_CountFileList( list2 );

	/* Create new list. */
	dst = cat = Z_Malloc( ( totalLength + 1 ) * sizeof( char * ) );

	/* Copy over lists. */
	if ( list0 )
	{
		for ( src = list0; *src; src++, dst++ )
		{
			*dst = *src;
		}
	}

	if ( list1 )
	{
		for ( src = list1; *src; src++, dst++ )
		{
			*dst = *src;
		}
	}

	if ( list2 )
	{
		for ( src = list2; *src; src++, dst++ )
		{
			*dst = *src;
		}
	}

	// Terminate the list
	*dst = NULL;

	// Free our old lists.
	// NOTE: not freeing their content, it's been merged in dst and still being used
	if ( list0 )
	{
		Z_Free( list0 );
	}

	if ( list1 )
	{
		Z_Free( list1 );
	}

	if ( list2 )
	{
		Z_Free( list2 );
	}

	return cat;
}

/*
================
FS_GetModList

Returns a list of mod directory names
A mod directory is a peer to baseq3 with a pk3 in it
The directories are searched in home path and base path
================
*/
int FS_GetModList( char *listbuf, int bufsize )
{
	int          nMods, i, j, nTotal, nLen, nPaks, nPotential, nDescLen;
	char         **pFiles = NULL;
	char         **pPaks = NULL;
	char         *name, *path;
	char         descPath[ MAX_OSPATH ];
	fileHandle_t descHandle;

	int          dummy;
	char         **pFiles0 = NULL;
	char         **pFiles1 = NULL;
	char         **pFiles2 = NULL;
	qboolean     bDrop = qfalse;

	*listbuf = 0;
	nMods = nPotential = nTotal = 0;

	pFiles0 = Sys_ListFiles( fs_homepath->string, NULL, NULL, &dummy, qtrue );
	pFiles1 = Sys_ListFiles( fs_basepath->string, NULL, NULL, &dummy, qtrue );

	// we searched for mods in the three paths
	// it is likely that we have duplicate names now, which we will cleanup below
	pFiles = Sys_ConcatenateFileLists( pFiles0, pFiles1, pFiles2 );
	nPotential = Sys_CountFileList( pFiles );

	for ( i = 0; i < nPotential; i++ )
	{
		name = pFiles[ i ];

		// NOTE: cleaner would involve more changes
		// ignore duplicate mod directories
		if ( i != 0 )
		{
			bDrop = qfalse;

			for ( j = 0; j < i; j++ )
			{
				if ( Q_stricmp( pFiles[ j ], name ) == 0 )
				{
					// this one can be dropped
					bDrop = qtrue;
					break;
				}
			}
		}

		if ( bDrop )
		{
			continue;
		}

		// we drop "baseq3" "." and ".."
		if ( Q_stricmp( name, BASEGAME ) && Q_stricmpn( name, ".", 1 ) )
		{
			// now we need to find some .pk3 files to validate the mod
			// NOTE TTimo: (actually I'm not sure why .. what if it's a mod under developement with no .pk3?)
			// we didn't keep the information when we merged the directory names, as to what OS Path it was found under
			//   so it could be in home path or base path
			//   we will try each three of them here (yes, it's a bit messy)
			// NOTE Arnout: what about dropping the current loaded mod as well?
			path = FS_BuildOSPath( fs_basepath->string, name, "" );
			nPaks = 0;
			pPaks = Sys_ListFiles( path, ".pk3", NULL, &nPaks, qfalse );
			Sys_FreeFileList( pPaks );  // we only use Sys_ListFiles to check whether .pk3 files are present

			/* try on home path */
			if ( nPaks <= 0 )
			{
				path = FS_BuildOSPath( fs_homepath->string, name, "" );
				nPaks = 0;
				pPaks = Sys_ListFiles( path, ".pk3", NULL, &nPaks, qfalse );
				Sys_FreeFileList( pPaks );
			}

			if ( nPaks > 0 )
			{
				nLen = strlen( name ) + 1;
				// nLen is the length of the mod path
				// we need to see if there is a description available
				descPath[ 0 ] = '\0';
				strcpy( descPath, name );
				strcat( descPath, "/description.txt" );
				nDescLen = FS_SV_FOpenFileRead( descPath, &descHandle );

				if ( nDescLen > 0 && descHandle )
				{
					FILE *file;
					file = FS_FileForHandle( descHandle );
					Com_Memset( descPath, 0, sizeof( descPath ) );
					nDescLen = fread( descPath, 1, 48, file );

					if ( nDescLen >= 0 )
					{
						descPath[ nDescLen ] = '\0';
					}

					FS_FCloseFile( descHandle );
				}
				else
				{
					strcpy( descPath, name );
				}

				nDescLen = strlen( descPath ) + 1;

				if ( nTotal + nLen + 1 + nDescLen + 1 < bufsize )
				{
					strcpy( listbuf, name );
					listbuf += nLen;
					strcpy( listbuf, descPath );
					listbuf += nDescLen;
					nTotal += nLen + nDescLen;
					nMods++;
				}
				else
				{
					break;
				}
			}
		}
	}

	Sys_FreeFileList( pFiles );

	return nMods;
}

//============================================================================

/*
================
FS_Dir_f
================
*/
void FS_Dir_f( void )
{
	char *path;
	char *extension;
	char **dirnames;
	int  ndirs;
	int  i;

	if ( Cmd_Argc() < 2 || Cmd_Argc() > 3 )
	{
		Com_Printf(_( "usage: dir <directory> [extension]\n" ));
		return;
	}

	if ( Cmd_Argc() == 2 )
	{
		path = Cmd_Argv( 1 );
		extension = "";
	}
	else
	{
		path = Cmd_Argv( 1 );
		extension = Cmd_Argv( 2 );
	}

	Com_Printf(_( "Directory of %s %s\n"), path, extension );
	Com_Printf( "---------------\n" );

	dirnames = FS_ListFiles( path, extension, &ndirs );

	for ( i = 0; i < ndirs; i++ )
	{
		Com_Printf( "%s\n", dirnames[ i ] );
	}

	FS_FreeFileList( dirnames );
}

/*
===========
FS_ConvertPath
===========
*/
void FS_ConvertPath( char *s )
{
	while ( *s )
	{
		if ( *s == '\\' || *s == ':' )
		{
			*s = '/';
		}

		s++;
	}
}

/*
===========
FS_PathCmp

Ignore case and separator char distinctions
===========
*/
int FS_PathCmp( const char *s1, const char *s2 )
{
	int c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if ( c1 >= 'a' && c1 <= 'z' )
		{
			c1 -= ( 'a' - 'A' );
		}

		if ( c2 >= 'a' && c2 <= 'z' )
		{
			c2 -= ( 'a' - 'A' );
		}

		if ( c1 == '\\' || c1 == ':' )
		{
			c1 = '/';
		}

		if ( c2 == '\\' || c2 == ':' )
		{
			c2 = '/';
		}

		if ( c1 < c2 )
		{
			return -1; // strings not equal
		}

		if ( c1 > c2 )
		{
			return 1;
		}
	}
	while ( c1 );

	return 0; // strings are equal
}

/*
================
FS_SortFileList
================
*/
void FS_SortFileList( char **filelist, int numfiles )
{
	int  i, j, k, numsortedfiles;
	char **sortedlist;

	sortedlist = Z_Malloc( ( numfiles + 1 ) * sizeof( *sortedlist ) );
	sortedlist[ 0 ] = NULL;
	numsortedfiles = 0;

	for ( i = 0; i < numfiles; i++ )
	{
		for ( j = 0; j < numsortedfiles; j++ )
		{
			if ( FS_PathCmp( filelist[ i ], sortedlist[ j ] ) < 0 )
			{
				break;
			}
		}

		for ( k = numsortedfiles; k > j; k-- )
		{
			sortedlist[ k ] = sortedlist[ k - 1 ];
		}

		sortedlist[ j ] = filelist[ i ];
		numsortedfiles++;
	}

	Com_Memcpy( filelist, sortedlist, numfiles * sizeof( *filelist ) );
	Z_Free( sortedlist );
}

/*
================
FS_NewDir_f
================
*/
void FS_NewDir_f( void )
{
	char *filter;
	char **dirnames;
	int  ndirs;
	int  i;

	if ( Cmd_Argc() < 2 )
	{
		Com_Printf(_( "usage: fdir <filter>\n" ));
		Com_Printf(_( "example: fdir *q3dm*.bsp\n" ));
		return;
	}

	filter = Cmd_Argv( 1 );

	Com_Printf( "---------------\n" );

	dirnames = FS_ListFilteredFiles( "", "", filter, &ndirs );

	FS_SortFileList( dirnames, ndirs );

	for ( i = 0; i < ndirs; i++ )
	{
		FS_ConvertPath( dirnames[ i ] );
		Com_Printf( "%s\n", dirnames[ i ] );
	}

	Com_Printf(_( "%d files listed\n"), ndirs );
	FS_FreeFileList( dirnames );
}

/*
============
FS_Path_f

============
*/
void FS_Path_f( void )
{
	searchpath_t *s;
	int          i;

	Com_DPrintf(_( "Current search path:\n" ));

	for ( s = fs_searchpaths; s; s = s->next )
	{
		if ( s->pack )
		{
			//      Com_Printf(_( "%s %X (%i files)\n"), s->pack->pakFilename, s->pack->checksum, s->pack->numfiles );
			Com_DPrintf(_( "%s (%i files)\n"), s->pack->pakFilename, s->pack->numfiles );

			if ( fs_numServerPaks )
			{
				if ( !FS_PakIsPure( s->pack ) )
				{
					Com_DPrintf(_( "    not on the pure list\n" ));
				}
				else
				{
					Com_DPrintf(_( "    on the pure list\n" ));
				}
			}
		}
		else
		{
			Com_DPrintf( "%s%c%s\n", s->dir->path, PATH_SEP, s->dir->gamedir );
		}
	}

	Com_DPrintf( "\n" );

	for ( i = 1; i < MAX_FILE_HANDLES; i++ )
	{
		if ( fsh[ i ].handleFiles.file.o )
		{
			Com_DPrintf( "handle %i: %s\n", i, fsh[ i ].name );
		}
	}
}

/*
============
FS_Which_f
============
*/
void FS_Which_f( void )
{
	searchpath_t *search;
	char         *netpath;
	pack_t       *pak;
	fileInPack_t *pakFile;
	directory_t  *dir;
	long         hash;
	FILE         *temp;
	char         *filename;
	char         buf[ MAX_OSPATH ];

	hash = 0;
	filename = Cmd_Argv( 1 );

	if ( !filename[ 0 ] )
	{
		Com_Printf(_( "Usage: which <file>\n" ));
		return;
	}

	// qpaths are not supposed to have a leading slash
	if ( filename[ 0 ] == '/' || filename[ 0 ] == '\\' )
	{
		filename++;
	}

	// just wants to see if file is there
	for ( search = fs_searchpaths; search; search = search->next )
	{
		if ( search->pack )
		{
			hash = FS_HashFileName( filename, search->pack->hashSize );
		}

		// is the element a pak file?
		if ( search->pack && search->pack->hashTable[ hash ] )
		{
			// look through all the pak file elements
			pak = search->pack;
			pakFile = pak->hashTable[ hash ];

			do
			{
				// case and separator insensitive comparisons
				if ( !FS_FilenameCompare( pakFile->name, filename ) )
				{
					// found it!
					Com_Printf(_( "File \"%s\" found in \"%s\"\n"), filename, pak->pakFilename );
					return;
				}

				pakFile = pakFile->next;
			}
			while ( pakFile != NULL );
		}
		else if ( search->dir )
		{
			dir = search->dir;

			netpath = FS_BuildOSPath( dir->path, dir->gamedir, filename );
			temp = fopen( netpath, "rb" );

			if ( !temp )
			{
				continue;
			}

			fclose( temp );
			Com_sprintf( buf, sizeof( buf ), "%s/%s", dir->path, dir->gamedir );
			FS_ReplaceSeparators( buf );
			Com_Printf(_( "File \"%s\" found at \"%s\"\n"), filename, buf );
			return;
		}
	}

	Com_Printf(_( "File not found: \"%s\"\n"), filename );
}

//===========================================================================

static int QDECL paksort( const void *a, const void *b )
{
	char *aa, *bb;

	aa = * ( char ** ) a;
	bb = * ( char ** ) b;

	return FS_PathCmp( aa, bb );
}

/*
================
FS_AddGameDirectory

Sets fs_gamedir, adds the directory to the head of the path,
then loads the zip headers
================
*/
#define MAX_PAKFILES 1024
void FS_AddGameDirectory( const char *path, const char *dir )
{
	searchpath_t *sp;
//	int i;
	searchpath_t *search;
	pack_t       *pak;
	char         *pakfile;
	int          numfiles;
	char         **pakfiles;
//	char            *sorted[MAX_PAKFILES];
	int          pakfilesi = 0;
//	char     **pakfilestmp;
	int          numdirs;
	char         **pakdirs;
	int          pakdirsi = 0;
// JPW NERVE

	/*char      mpsppakfilestring[4];

	sprintf( mpsppakfilestring, "msp" );*/
// jpw

	// this fixes the case where fs_basepath is the same as fs_cdpath
	// which happens on full installs
	for ( sp = fs_searchpaths; sp; sp = sp->next )
	{
		if ( sp->dir && !Q_stricmp( sp->dir->path, path ) && !Q_stricmp( sp->dir->gamedir, dir ) )
		{
			return; // we've already got this one
		}
	}

	Q_strncpyz( fs_gamedir, dir, sizeof( fs_gamedir ) );

	//
	// add the directory to the search path
	//
	search = Z_Malloc( sizeof( searchpath_t ) );
	search->dir = Z_Malloc( sizeof( *search->dir ) );

	Q_strncpyz( search->dir->path, path, sizeof( search->dir->path ) );
	Q_strncpyz( search->dir->gamedir, dir, sizeof( search->dir->gamedir ) );
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	// find all pak files in this directory
	pakfile = FS_BuildOSPath( path, dir, "" );
	pakfile[ strlen( pakfile ) - 1 ] = 0;  // strip the trailing slash

	// Get .pk3 files
	pakfiles = Sys_ListFiles( pakfile, ".pk3", NULL, &numfiles, qfalse );

	// Get top level directories (we'll filter them later since the Sys_ListFiles filtering is terrible)
	pakdirs = Sys_ListFiles( pakfile, "/", NULL, &numdirs, qfalse );

	// sort them so that later alphabetic matches override
	// earlier ones.  This makes pak1.pk3 override pak0.pk3
	if ( numfiles > MAX_PAKFILES )
	{
		numfiles = MAX_PAKFILES;
	}

	qsort( pakfiles, numfiles, sizeof( char * ), paksort );
	qsort( pakdirs, numdirs, sizeof( char * ), paksort );

	// Log may not be initialized at this point, but it will still show in the console.
	if ( !com_fullyInitialized )
	{
		Com_Printf( "FS_AddGameDirectory(\"%s\", \"%s\") found %d .pk3 and %d .pk3dir\n", path, dir, numfiles, numdirs );
	}

#if 0
	for ( ; ( pakfilesi + pakdirsi ) < ( numfiles + numdirs ); )
	{
		// Check if a pakfile or pakdir comes next
		if ( pakfilesi >= numfiles )
		{
			// We've used all the pakfiles, it must be a pakdir.
			pakwhich = 0;
		}
		else if ( pakdirsi >= numdirs )
		{
			// We've used all the pakdirs, it must be a pakfile.
			pakwhich = 1;
		}
		else
		{
			// Could be either, compare to see which name comes first
			// Need tmp variables for appropriate indirection for paksort()
			pakfilestmp = &pakfiles[ pakfilesi ];
			pakdirstmp = &pakdirs[ pakdirsi ];
			pakwhich = ( paksort( pakfilestmp, pakdirstmp ) < 0 );
		}

		if ( pakwhich )
		{
			// The next .pk3 file is before the next .pk3dir
			pakfile = FS_BuildOSPath( path, dir, pakfiles[ pakfilesi ] );
			Com_Printf( "    pk3: %s\n", pakfile );

			if ( ( pak = FS_LoadZipFile( pakfile, pakfiles[ pakfilesi ] ) ) == 0 )
			{
				continue;
			}

			Q_strncpyz( pak->pakFilename, pakfile, sizeof( pak->pakFilename ) );
			// store the game name for downloading
			Q_strncpyz( pak->pakGamename, dir, sizeof( pak->pakGamename ) );

			fs_packFiles += pak->numfiles;

			search = Z_Malloc( sizeof( searchpath_t ) );
			search->pack = pak;
			search->next = fs_searchpaths;
			fs_searchpaths = search;

			pakfilesi++;
		}
		else
		{
			// The next .pk3dir is before the next .pk3 file
			// But wait, this could be any directory, we're filtering to only ending with ".pk3dir" here.
			len = strlen( pakdirs[ pakdirsi ] );

			if ( strcmp( COM_GetExtension( pakdirs[ pakdirsi ] ), "pk3dir" ) )
			{
				// This isn't a .pk3dir! Next!
				pakdirsi++;
				continue;
			}

			pakfile = FS_BuildOSPath( path, dir, pakdirs[ pakdirsi ] );
			Com_Printf( " pk3dir: %s\n", pakfile );

			// add the directory to the search path
			search = Z_Malloc( sizeof( searchpath_t ) );
			search->dir = Z_Malloc( sizeof( *search->dir ) );

			Q_strncpyz( search->dir->path, pakfile, sizeof( search->dir->path ) );   // c:\xreal\base
			Q_strncpyz( search->dir->path, pakfile, sizeof( search->dir->path ) );   // c:\xreal\base\mypak.pk3dir
			Q_strncpyz( search->dir->gamedir, pakdirs[ pakdirsi ], sizeof( search->dir->gamedir ) );   // mypak.pk3dir

			search->next = fs_searchpaths;
			fs_searchpaths = search;

			pakdirsi++;
		}
	}
#endif

	while ( pakfilesi < numfiles )
	{
		// The next .pk3 file is before the next .pk3dir
		pakfile = FS_BuildOSPath( path, dir, pakfiles[ pakfilesi ] );

		if ( !com_fullyInitialized )
		{
			Com_Printf( "    pk3: %s", pakfile );
		}

		if ( !( pak = FS_LoadZipFile( pakfile, pakfiles[ pakfilesi ] ) ) )
		{
			if ( !com_fullyInitialized )
			{
				Com_Printf( " ( ^1INVALID PK3 )\n" );
			}
			pakfilesi++;
			continue;
		}

		Q_strncpyz( pak->pakFilename, pakfile, sizeof( pak->pakFilename ) );
		// store the game name for downloading
		Q_strncpyz( pak->pakGamename, dir, sizeof( pak->pakGamename ) );

		fs_packFiles += pak->numfiles;

		if ( !com_fullyInitialized )
		{
			Com_Printf( " ( %d files )\n", pak->numfiles );
		}
		
		search = Z_Malloc( sizeof( searchpath_t ) );
		search->pack = pak;
		search->next = fs_searchpaths;
		fs_searchpaths = search;

		pakfilesi++;
	}

	while ( pakdirsi < numdirs )
	{
		// The next .pk3dir is before the next .pk3 file
		// But wait, this could be any directory, we're filtering to only ending with ".pk3dir" here.
		if ( strcmp( COM_GetExtension( pakdirs[ pakdirsi ] ), "pk3dir" ) )
		{
			// This isn't a .pk3dir! Next!
			pakdirsi++;
			continue;
		}

		pakfile = FS_BuildOSPath( path, dir, pakdirs[ pakdirsi ] );

		if ( !com_fullyInitialized )
		{
			Com_Printf( " pk3dir: %s\n", pakfile );
		}

		// add the directory to the search path
		search = Z_Malloc( sizeof( searchpath_t ) );
		search->dir = Z_Malloc( sizeof( *search->dir ) );

		Q_strncpyz( search->dir->path, pakfile, sizeof( search->dir->path ) );   // c:\xreal\base\mypak.pk3dir
		search->dir->gamedir[ 0 ] = '.';
		//        Q_strncpyz(search->dir->gamedir, pakdirs[pakdirsi], sizeof(search->dir->gamedir)); // mypak.pk3dir

		search->next = fs_searchpaths;
		fs_searchpaths = search;

		pakdirsi++;
	}

	// done
	Sys_FreeFileList( pakfiles );
	Sys_FreeFileList( pakdirs );
}

/*
================
FS_ComparePaks

----------------
dlstring == qtrue

Returns a list of pak files that we should download from the server. They all get stored
in the current gamedir and an FS_Restart will be fired up after we download them all.

The string is the format:

@remotename@localname [repeat]

static int    fs_numServerReferencedPaks;
static int    fs_serverReferencedPaks[MAX_SEARCH_PATHS];
static char   *fs_serverReferencedPakNames[MAX_SEARCH_PATHS];

----------------
dlstring == qfalse

we are not interested in a download string format, we want something human-readable
(this is used for diagnostics while connecting to a pure server)

================
*/
qboolean CL_WWWBadChecksum( const char *pakname );

qboolean FS_ComparePaks( char *neededpaks, int len, qboolean dlstring )
{
	searchpath_t *sp;
	qboolean     havepak;
	int          i;

	if ( !fs_numServerReferencedPaks )
	{
		return qfalse; // Server didn't send any pack information along
	}

	*neededpaks = 0;

	for ( i = 0; i < fs_numServerReferencedPaks; i++ )
	{
		havepak = qfalse;

		for ( sp = fs_searchpaths; sp; sp = sp->next )
		{
			if ( sp->pack && sp->pack->checksum == fs_serverReferencedPaks[ i ] )
			{
				havepak = qtrue; // This is it!
				break;
			}
		}

		if ( !havepak && fs_serverReferencedPakNames[ i ] && *fs_serverReferencedPakNames[ i ] )
		{
			// Don't got it

			if ( dlstring )
			{
				// Remote name
				Q_strcat( neededpaks, len, "@" );
				Q_strcat( neededpaks, len, fs_serverReferencedPakNames[ i ] );
				Q_strcat( neededpaks, len, ".pk3" );

				// Local name
				Q_strcat( neededpaks, len, "@" );

				// Do we have one with the same name?
				if ( FS_SV_FileExists( va( "%s.pk3", fs_serverReferencedPakNames[ i ] ) ) )
				{
					char st[ MAX_ZPATH ];
					// We already have one called this, we need to download it to another name
					// Make something up with the checksum in it
					Com_sprintf( st, sizeof( st ), "%s.%08x.pk3", fs_serverReferencedPakNames[ i ], fs_serverReferencedPaks[ i ] );
					Q_strcat( neededpaks, len, st );
				}
				else
				{
					Q_strcat( neededpaks, len, fs_serverReferencedPakNames[ i ] );
					Q_strcat( neededpaks, len, ".pk3" );
				}
			}
			else
			{
				Q_strcat( neededpaks, len, fs_serverReferencedPakNames[ i ] );
				Q_strcat( neededpaks, len, ".pk3" );

				// Do we have one with the same name?
				if ( FS_SV_FileExists( va( "%s.pk3", fs_serverReferencedPakNames[ i ] ) ) )
				{
					Q_strcat( neededpaks, len, " (local file exists with wrong checksum)" );
#ifndef DEDICATED

					// let the client subsystem track bad download redirects (dl file with wrong checksums)
					// this is a bit ugly but the only other solution would have been callback passing..
					if ( CL_WWWBadChecksum( va( "%s.pk3", fs_serverReferencedPakNames[ i ] ) ) )
					{
						// remove a potentially malicious download file
						// (this is also intended to avoid expansion of the pk3 into a file with different checksum .. messes up wwwdl chkfail)
						char *rmv = FS_BuildOSPath( fs_homepath->string, va( "%s.pk3", fs_serverReferencedPakNames[ i ] ), "" );
						rmv[ strlen( rmv ) - 1 ] = '\0';
						FS_Remove( rmv );
					}

#endif
				}

				Q_strcat( neededpaks, len, "\n" );
			}
		}
	}

	if ( *neededpaks )
	{
		Com_DPrintf(_( "Need paks: %s\n"), neededpaks );
		return qtrue;
	}

	return qfalse; // We have them all
}

/*
================
FS_Shutdown

Frees all resources and closes all files
================
*/
void FS_Shutdown( qboolean closemfp )
{
	searchpath_t *p, *next;
	int          i;

	for ( i = 0; i < MAX_FILE_HANDLES; i++ )
	{
		if ( fsh[ i ].fileSize )
		{
			FS_FCloseFile( i );
		}
	}

	// free everything
	for ( p = fs_searchpaths; p; p = next )
	{
		next = p->next;

		if ( p->pack )
		{
			unzClose( p->pack->handle );
			Z_Free( p->pack->buildBuffer );
			Z_Free( p->pack );
		}

		if ( p->dir )
		{
			Z_Free( p->dir );
		}

		Z_Free( p );
	}

	// any FS_ calls will now be an error until reinitialized
	fs_searchpaths = NULL;

	Cmd_RemoveCommand( "path" );
	Cmd_RemoveCommand( "dir" );
	Cmd_RemoveCommand( "fdir" );
	Cmd_RemoveCommand( "which" );
}

/*
================
FS_ReorderPurePaks
NOTE TTimo: the reordering that happens here is not reflected in the cvars (\cvarlist *pak*)
  this can lead to misleading situations, see show_bug.cgi?id=540
================
*/
static void FS_ReorderPurePaks( void )
{
	searchpath_t *s;
	int          i;
	searchpath_t **p_insert_index, // for linked list reordering
	             **p_previous; // when doing the scan

	// only relevant when connected to pure server
	if ( !fs_numServerPaks )
	{
		return;
	}

	fs_reordered = qfalse;

	p_insert_index = &fs_searchpaths; // we insert in order at the beginning of the list

	for ( i = 0; i < fs_numServerPaks; i++ )
	{
		p_previous = p_insert_index; // track the pointer-to-current-item

		for ( s = *p_insert_index; s; s = s->next ) // the part of the list before p_insert_index has been sorted already
		{
			if ( s->pack && fs_serverPaks[ i ] == s->pack->checksum )
			{
				fs_reordered = qtrue;
				// move this element to the insert list
				*p_previous = s->next;
				s->next = *p_insert_index;
				*p_insert_index = s;
				// increment insert list
				p_insert_index = &s->next;
				break; // iterate to next server pack
			}

			p_previous = &s->next;
		}
	}
}

/*
================
FS_Startup
================
*/
static void FS_Startup( const char *gameName )
{
	const char *homePath;

	Com_DPrintf( "----- FS_Startup -----\n" );

	fs_debug = Cvar_Get( "fs_debug", "0", 0 );
	fs_basepath = Cvar_Get( "fs_basepath", Sys_DefaultBasePath(), CVAR_INIT );
	fs_basegame = Cvar_Get( "fs_basegame", "", CVAR_INIT );
	fs_libpath = Cvar_Get( "fs_libpath", Sys_DefaultLibPath(), CVAR_INIT );
#ifdef MACOS_X
	fs_apppath = Cvar_Get( "fs_apppath", Sys_DefaultAppPath(), CVAR_INIT );
#endif
	homePath = Sys_DefaultHomePath();

	if ( !homePath || !homePath[ 0 ] )
	{
		homePath = fs_basepath->string;
	}

	fs_homepath = Cvar_Get( "fs_homepath", homePath, CVAR_INIT );
	fs_gamedirvar = Cvar_Get( "fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO );

	// add search path elements in reverse priority order
	if ( fs_basepath->string[ 0 ] )
	{
		FS_AddGameDirectory( fs_basepath->string, gameName );
	}

	// fs_homepath is somewhat particular to *nix systems, only add if relevant

	if ( fs_basepath->string[ 0 ] && fs_libpath->string[ 0 ] && Q_stricmp( fs_homepath->string, fs_basepath->string ) )
	{
		FS_AddGameDirectory( fs_libpath->string, gameName );
	}

	// fs_libpath is somewhat particular to *nix systems, only add if relevant

#ifdef MACOS_X

	// Make MacOSX also include the base path included with the .app bundle
	if ( fs_apppath->string[ 0 ] )
	{
		FS_AddGameDirectory( fs_apppath->string, gameName );
	}

#endif

	// NOTE: same filtering below for mods and basegame
	if ( fs_basepath->string[ 0 ] && Q_stricmp( fs_homepath->string, fs_basepath->string ) )
	{
		FS_AddGameDirectory( fs_homepath->string, gameName );
	}

	// check for additional base game so mods can be based upon other mods
	if ( fs_basegame->string[ 0 ] && !Q_stricmp( gameName, BASEGAME ) && Q_stricmp( fs_basegame->string, gameName ) )
	{
		if ( fs_basepath->string[ 0 ] )
		{
			FS_AddGameDirectory( fs_basepath->string, fs_basegame->string );
		}

		if ( fs_homepath->string[ 0 ] && Q_stricmp( fs_homepath->string, fs_basepath->string ) )
		{
			FS_AddGameDirectory( fs_homepath->string, fs_basegame->string );
		}
	}

	// check for additional game folder for mods
	if ( fs_gamedirvar->string[ 0 ] && !Q_stricmp( gameName, BASEGAME ) && Q_stricmp( fs_gamedirvar->string, gameName ) )
	{
		if ( fs_basepath->string[ 0 ] )
		{
			FS_AddGameDirectory( fs_basepath->string, fs_gamedirvar->string );
		}

		if ( fs_homepath->string[ 0 ] && Q_stricmp( fs_homepath->string, fs_basepath->string ) )
		{
			FS_AddGameDirectory( fs_homepath->string, fs_gamedirvar->string );
		}
	}

	// add our commands
	Cmd_AddCommand( "path", FS_Path_f );
	Cmd_AddCommand( "dir", FS_Dir_f );
	Cmd_AddCommand( "fdir", FS_NewDir_f );
	Cmd_AddCommand( "which", FS_Which_f );

	// show_bug.cgi?id=506
	// reorder the pure pk3 files according to server order
	FS_ReorderPurePaks();

	// print the current search paths
	FS_Path_f();

	fs_gamedirvar->modified = qfalse; // We just loaded, it's not modified

	Com_DPrintf( "----------------------\n" );
	Com_DPrintf(_( "%d files in pk3 files\n"), fs_packFiles );
}

#if !defined( DO_LIGHT_DEDICATED )

/*
=====================
FS_LoadedPakChecksums

Returns a space separated string containing the checksums of all loaded pk3 files.
Servers with sv_pure set will get this string and pass it to clients.
=====================
*/
const char *FS_LoadedPakChecksums( void )
{
	static char  info[ BIG_INFO_STRING ];
	searchpath_t *search;

	info[ 0 ] = 0;

	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file?
		if ( !search->pack )
		{
			continue;
		}

		Q_strcat( info, sizeof( info ), va( "%i ", search->pack->checksum ) );
	}

	return info;
}

/*
=====================
FS_LoadedPakNames

Returns a space separated string containing the names of all loaded pk3 files.
Servers with sv_pure set will get this string and pass it to clients.
=====================
*/
const char *FS_LoadedPakNames( void )
{
	static char  info[ BIG_INFO_STRING ];
	searchpath_t *search;

	info[ 0 ] = 0;

	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file?
		if ( !search->pack )
		{
			continue;
		}

		if ( *info )
		{
			Q_strcat( info, sizeof( info ), " " );
		}

		// Arnout: changed to have the full path
		//Q_strcat( info, sizeof( info ), search->pack->pakBasename );
		Q_strcat( info, sizeof( info ), search->pack->pakGamename );
		Q_strcat( info, sizeof( info ), "/" );
		Q_strcat( info, sizeof( info ), search->pack->pakBasename );
	}

	return info;
}

/*
=====================
FS_LoadedPakPureChecksums

Returns a space separated string containing the pure checksums of all loaded pk3 files.
Servers with sv_pure use these checksums to compare with the checksums the clients send
back to the server.
=====================
*/
const char *FS_LoadedPakPureChecksums( void )
{
	static char  info[ BIG_INFO_STRING ];
	searchpath_t *search;

	info[ 0 ] = 0;

	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file?
		if ( !search->pack )
		{
			continue;
		}

		Q_strcat( info, sizeof( info ), va( "%i ", search->pack->pure_checksum ) );
	}

	// DO_LIGHT_DEDICATED
	// only comment out when you need a new pure checksums string
	//Com_DPrintf("FS_LoadPakPureChecksums: %s\n", info);

	return info;
}

/*
=====================
FS_ReferencedPakChecksums

Returns a space separated string containing the checksums of all referenced pk3 files.
The server will send this to the clients so they can check which files should be auto-downloaded.
=====================
*/
const char *FS_ReferencedPakChecksums( void )
{
	static char  info[ BIG_INFO_STRING ];
	searchpath_t *search;

	info[ 0 ] = 0;

	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file?
		if ( search->pack )
		{
			if ( search->pack->referenced || Q_stricmpn( search->pack->pakGamename, BASEGAME, strlen( BASEGAME ) ) )
			{
				Q_strcat( info, sizeof( info ), va( "%i ", search->pack->checksum ) );
			}
		}
	}

	return info;
}

/*
=====================
FS_ReferencedPakNames

Returns a space separated string containing the names of all referenced pk3 files.
The server will send this to the clients so they can check which files should be auto-downloaded.
=====================
*/
const char *FS_ReferencedPakNames( void )
{
	static char  info[ BIG_INFO_STRING ];
	searchpath_t *search;

	info[ 0 ] = 0;

	// we want to return all pk3s from the fs_game path
	// and referenced ones from baseq3
	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file?
		if ( search->pack )
		{
			if ( search->pack->referenced || Q_stricmpn( search->pack->pakGamename, BASEGAME, strlen( BASEGAME ) ) )
			{
                                if ( *info )
                                {
                                        Q_strcat( info, sizeof( info ), " " );
                                }

				Q_strcat( info, sizeof( info ), search->pack->pakGamename );
				Q_strcat( info, sizeof( info ), "/" );
				Q_strcat( info, sizeof( info ), search->pack->pakBasename );
			}
		}
	}

	return info;
}

/*
=====================
FS_ReferencedPakPureChecksums

Returns a space separated string containing the pure checksums of all referenced pk3 files.
Servers with sv_pure set will get this string back from clients for pure validation

The string has a specific order, "cgame ui @ ref1 ref2 ref3 ..."

NOTE TTimo - DO_LIGHT_DEDICATED
this function is only used by the client to build the string sent back to server
we don't have any need of overriding it for light, but it's useless in dedicated
=====================
*/
const char *FS_ReferencedPakPureChecksums( void )
{
	static char  info[ BIG_INFO_STRING ];
	searchpath_t *search;
	int          nFlags, numPaks, checksum;

	info[ 0 ] = 0;

	checksum = fs_checksumFeed;

	numPaks = 0;

	for ( nFlags = FS_CGAME_REF; nFlags; nFlags = nFlags >> 1 )
	{
		if ( nFlags & FS_GENERAL_REF )
		{
			// add a delimter between must haves and general refs
			//Q_strcat(info, sizeof(info), "@ ");
			info[ strlen( info ) + 1 ] = '\0';
			info[ strlen( info ) + 2 ] = '\0';
			info[ strlen( info ) ] = '@';
			info[ strlen( info ) ] = ' ';
		}

		for ( search = fs_searchpaths; search; search = search->next )
		{
			// is the element a pak file and has it been referenced based on flag?
			if ( search->pack && ( search->pack->referenced & nFlags ) )
			{
				Q_strcat( info, sizeof( info ), va( "%i ", search->pack->pure_checksum ) );

				if ( nFlags & ( FS_CGAME_REF | FS_UI_REF ) )
				{
					break;
				}

				checksum ^= search->pack->pure_checksum;
				numPaks++;
			}
		}

		if ( fs_fakeChkSum != 0 )
		{
			// only added if a non-pure file is referenced
			Q_strcat( info, sizeof( info ), va( "%i ", fs_fakeChkSum ) );
		}
	}

	// last checksum is the encoded number of referenced pk3s
	checksum ^= numPaks;
	Q_strcat( info, sizeof( info ), va( "%i ", checksum ) );

	return info;
}

#else // DO_LIGHT_DEDICATED implementation follows

/*
=========================================================================================
DO_LIGHT_DEDICATED, general notes
we are going to fake the checksums sent to the clients
that only matters to the pk3 we have replaced by their lighter version, currently:

Cvar_Set2: sv_pakNames mp_pakmaps0 mp_pak2 mp_pak1 mp_pak0 pak0
Cvar_Set2: sv_paks -1153491798 125907563 -1023558518 764840216 1886207346

all the files above have their 'server required' content collapsed into a single pak0.pk3

the other .pk3 files should be handled as usual

more details are in unix/dedicated-only.txt

=========================================================================================
*/

// our target faked checksums
// those don't need to be encrypted or anything, that's what you see in the +set developer 1
static const char *pak_checksums = "-137448799 131270674 125907563 -1023558518 764840216 1886207346";
static const char *pak_names = "mp_pak4 mp_pak3 mp_pak2 mp_pak1 mp_pak0 pak0";

/*
this is the pure checksum string for a constant value of fs_checksumFeed we have chosen (see SV_SpawnServer)
to obtain the new string for a different fs_checksumFeed value, run a regular server and enable the relevant
verbosity code in SV_SpawnServer and FS_LoadedPakPureChecksums (the full server version of course)

NOTE: if you have an mp_bin in the middle, you need to take out its checksum
  (we keep mp_bin out of the faked stuff because we don't want to have to update those feeds too often heh)

once you have the clear versions, you can shift them by commenting out the code chunk in FS_RandChecksumFeed
you need to use the right line in FS_LoadedPakPureChecksums whether you are running on clear strings, or shifted ones
*/

/*
// clear checksums, rebuild those from a regular server and you will shift them next
static const int feeds[5] = {
  0x14d48835, 0xc44ed670, 0xd1c8da0d, 0x98df0626, 0xb4e51e7a
};

static const char* pak_purechecksums[5] = {
 "-631058236 1439191868 -1758535722 -1109639830 -756342425 -26055934",
 "420891163 -2077045804 -1212476885 273103692 1907819222 -1162012968",
 "724865970 393950398 1987220301 679766798 -966287476 -1045306141",
 "468836794 -690412926 -481399336 1089964294 -1538547350 394664641",
 "-1484520489 -1891368444 -510451918 -919424191 -1623567814 889557862"
};
*/

static const int feeds[ 5 ] =
{
	0x14d48835, 0xc44ed670, 0xd1c8da0d, 0x98df0626, 0xb4e51e7a
};

// shifted strings, so that it's not directly scannable from exe
// see FS_RandChecksumFeed to generate them
static const char *pak_purechecksums[ 5 ] =
{
	// rain - escaped ?s to prevent parsing as trigraph
	":C@>=BE?@C->A@F>F>ECE-:>DBEB@BD\?\?-:>>=FC@FE@=-:DBC@A?A?B-:?C=BBF@A",
	"B@>FG??DA.;@>EE>BCF>B.;?@?@BEDFFC.@EA?>ADG@.?G>EF?G@@@.;??D@>?@GDF",
	"FACGEDHF?/BHBHD?BHG/@HGFAA?B?@/EFHFEEFHG/<HEEAGFCFE/<@?CDB?E@C@",
	"DFHHCFGID0=FI@DABIBF0=DHACIICCF0A@HIIFDBID0=AECHEDGCE@0CIDFFDFDA",
	">BEIEFCAEIJ1>BIJBDGIEEE1>FBAEFBJBI1>JBJECEBJB1>BGCDFGHIBE1IIJFFHIGC"
};

// counter to walk through the randomized list
static int       feed_index = -1;

static int       lookup_randomized[ 5 ] = { 0, 1, 2, 3, 4 };

/*
=====================
randomize the order of the 5 checksums we rely on
5 random swaps of the table
=====================
*/
void FS_InitRandomFeed( void )
{
	int i, swap, aux;

	for ( i = 0; i < 5; i++ )
	{
		swap = ( int )( 5.0 * rand() / ( RAND_MAX + 1.0 ) );
		aux = lookup_randomized[ i ];
		lookup_randomized[ i ] = lookup_randomized[ swap ];
		lookup_randomized[ swap ] = aux;
	}
}

/*
=====================
FS_RandChecksumFeed

Return a random checksum feed among our list
we keep the seed and use it when requested for the pure checksum
=====================
*/
int FS_RandChecksumFeed( void )
{
	/*
	// use this to dump shifted versions of the pure checksum strings
	int i;
	for(i=0;i<5;i++)
	{
	  Com_Printf_(("FS_RandChecksumFeed: %s\n"), FS_ShiftStr(pak_purechecksums[i], 13+i));
	}
	*/
	if ( feed_index == -1 )
	{
		FS_InitRandomFeed();
	}

	feed_index = ( feed_index + 1 ) % 5;
	return feeds[ lookup_randomized[ feed_index ] ];
}

/*
=====================
FS_LoadedPakChecksums

Returns a space separated string containing the checksums of all loaded pk3 files.
Servers with sv_pure set will get this string and pass it to clients.

DO_LIGHT_DEDICATED:
drop lightweight pak0 checksum, put the faked pk3s checksums instead
=====================
*/
const char *FS_LoadedPakChecksums( void )
{
	static char  info[ BIG_INFO_STRING ];
	searchpath_t *search;

	info[ 0 ] = 0;

	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file?
		if ( !search->pack )
		{
			continue;
		}

		if ( strcmp( search->pack->pakBasename, "pak0" ) )
		{
			// this is a regular pk3
			Q_strcat( info, sizeof( info ), va( "%i ", search->pack->checksum ) );
		}
		else
		{
			// this is the light pk3
			Q_strcat( info, sizeof( info ), va( "%s ", pak_checksums ) );
		}
	}

	return info;
}

/*
=====================
FS_LoadedPakNames

Returns a space separated string containing the names of all loaded pk3 files.
Servers with sv_pure set will get this string and pass it to clients.

DO_LIGHT_DEDICATED:
drop lightweight pak0 name, put the faked pk3s names instead
=====================
*/
const char *FS_LoadedPakNames( void )
{
	static char  info[ BIG_INFO_STRING ];
	searchpath_t *search;

	info[ 0 ] = 0;

	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file?
		if ( !search->pack )
		{
			continue;
		}

		if ( *info )
		{
			Q_strcat( info, sizeof( info ), " " );
		}

		if ( strcmp( search->pack->pakBasename, "pak0" ) )
		{
			// regular pk3
			Q_strcat( info, sizeof( info ), search->pack->pakBasename );
		}
		else
		{
			// light pk3
			Q_strcat( info, sizeof( info ), pak_names );
		}
	}

	return info;
}

/*
=====================
FS_LoadedPakPureChecksums

Returns a space separated string containing the pure checksums of all loaded pk3 files.
Servers with sv_pure use these checksums to compare with the checksums the clients send
back to the server.

DO_LIGHT_DEDICATED:
FS_LoadPakChecksums to send the pak string to the client
FS_LoadPakPureChecksums is used locally to compare against what the client sends back

the pure_checksums are computed by Com_MemoryBlockChecksum with a random key (fs_checksumFeed)

drop lightweight pak0 checksum, put the faked pk3s pure checksums instead

=====================
*/
const char *FS_LoadedPakPureChecksums( void )
{
	static char  info[ BIG_INFO_STRING ];
	searchpath_t *search;

	info[ 0 ] = 0;

	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file?
		if ( !search->pack )
		{
			continue;
		}

		if ( strcmp( search->pack->pakBasename, "pak0" ) )
		{
			// this is a regular pk3
			Q_strcat( info, sizeof( info ), va( "%i ", search->pack->pure_checksum ) );
		}
		else
		{
			// this is the light pk3
			// use this if you are running on shifted strings
			Q_strcat( info, sizeof( info ), va( "%s ", FS_ShiftStr( pak_purechecksums[ lookup_randomized[ feed_index ] ], -13 - lookup_randomized[ feed_index ] ) ) );
			// use this if you are running on clear checksum strings instead of shifted ones
			//Q_strcat( info, sizeof( info ), va("%s ", pak_purechecksums[lookup_randomized[feed_index]] ) );
		}
	}

	return info;
}

/*
=====================
FS_ReferencedPakChecksums

Returns a space separated string containing the checksums of all referenced pk3 files.
The server will send this to the clients so they can check which files should be auto-downloaded.

DO_LIGHT_DEDICATED:
don't send the checksum of pak0 (even if it's referenced)

NOTE:
do we need to fake referenced paks too?
mp_pakmaps0 would be a worthy candidate for download though, but we don't have it anyway
the only thing if we omit sending of some referenced stuff, you don't get the console message that says "you're missing this"
=====================
*/
const char *FS_ReferencedPakChecksums( void )
{
	static char  info[ BIG_INFO_STRING ];
	searchpath_t *search;

	info[ 0 ] = 0;

	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file?
		if ( search->pack )
		{
			if ( search->pack->referenced )
			{
				if ( strcmp( search->pack->pakBasename, "pak0" ) )
				{
					// this is not the light pk3
					Q_strcat( info, sizeof( info ), va( "%i ", search->pack->checksum ) );
				}
			}
		}
	}

	return info;
}

/*
=====================
FS_ReferencedPakNames

Returns a space separated string containing the names of all referenced pk3 files.
The server will send this to the clients so they can check which files should be auto-downloaded.

DO_LIGHT_DEDICATED:
don't send pak0 see above for details
=====================
*/
const char *FS_ReferencedPakNames( void )
{
	static char  info[ BIG_INFO_STRING ];
	searchpath_t *search;

	info[ 0 ] = 0;

	// we want to return all pk3s from the fs_game path
	// and referenced ones from baseq3
	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file?
		if ( search->pack )
		{
			if ( *info )
			{
				Q_strcat( info, sizeof( info ), " " );
			}

			if ( search->pack->referenced )
			{
				if ( strcmp( search->pack->pakBasename, "pak0" ) )
				{
					// this is not the light pk3
					Q_strcat( info, sizeof( info ), search->pack->pakGamename );
					Q_strcat( info, sizeof( info ), "/" );
					Q_strcat( info, sizeof( info ), search->pack->pakBasename );
				}
			}
		}
	}

	return info;
}

#endif

/*
=====================
FS_ClearPakReferences
=====================
*/
void FS_ClearPakReferences( int flags )
{
	searchpath_t *search;

	if ( !flags )
	{
		flags = -1;
	}

	for ( search = fs_searchpaths; search; search = search->next )
	{
		// is the element a pak file and has it been referenced?
		if ( search->pack )
		{
			search->pack->referenced &= ~flags;
		}
	}
}

/*
=====================
FS_PureServerSetLoadedPaks

If the string is empty, all data sources will be allowed.
If not empty, only pk3 files that match one of the space
separated checksums will be checked for files, with the
exception of .cfg and .dat files.
=====================
*/
void FS_PureServerSetLoadedPaks( const char *pakSums, const char *pakNames )
{
	int i, c, d;

	Cmd_TokenizeString( pakSums );

	c = Cmd_Argc();

	if ( c > MAX_SEARCH_PATHS )
	{
		c = MAX_SEARCH_PATHS;
	}

	fs_numServerPaks = c;

	for ( i = 0; i < c; i++ )
	{
		fs_serverPaks[ i ] = atoi( Cmd_Argv( i ) );
	}

	if ( fs_numServerPaks )
	{
		Com_DPrintf( "Connected to a pure server.\n" );
	}
	else
	{
		if ( fs_reordered )
		{
			// show_bug.cgi?id=540
			// force a restart to make sure the search order will be correct
			Com_DPrintf( "FS search reorder is required\n" );
			FS_Restart( fs_checksumFeed );
			return;
		}
	}

	for ( i = 0; i < c; i++ )
	{
		if ( fs_serverPakNames[ i ] )
		{
			Z_Free( fs_serverPakNames[ i ] );
		}

		fs_serverPakNames[ i ] = NULL;
	}

	if ( pakNames && *pakNames )
	{
		Cmd_TokenizeString( pakNames );

		d = Cmd_Argc();

		if ( d > MAX_SEARCH_PATHS )
		{
			d = MAX_SEARCH_PATHS;
		}

		for ( i = 0; i < d; i++ )
		{
			fs_serverPakNames[ i ] = CopyString( Cmd_Argv( i ) );
		}
	}
}

/*
=====================
FS_PureServerSetReferencedPaks

The checksums and names of the pk3 files referenced at the server
are sent to the client and stored here. The client will use these
checksums to see if any pk3 files need to be auto-downloaded.
=====================
*/
void FS_PureServerSetReferencedPaks( const char *pakSums, const char *pakNames )
{
	int i, c, d;

	Cmd_TokenizeString( pakSums );

	c = Cmd_Argc();

	if ( c > MAX_SEARCH_PATHS )
	{
		c = MAX_SEARCH_PATHS;
	}

	fs_numServerReferencedPaks = c;

	for ( i = 0; i < c; i++ )
	{
		fs_serverReferencedPaks[ i ] = atoi( Cmd_Argv( i ) );
	}

	for ( i = 0; i < c; i++ )
	{
		if ( fs_serverReferencedPakNames[ i ] )
		{
			Z_Free( fs_serverReferencedPakNames[ i ] );
		}

		fs_serverReferencedPakNames[ i ] = NULL;
	}

	if ( pakNames && *pakNames )
	{
		Cmd_TokenizeString( pakNames );

		d = Cmd_Argc();

		if ( d > MAX_SEARCH_PATHS )
		{
			d = MAX_SEARCH_PATHS;
		}

		for ( i = 0; i < d; i++ )
		{
			fs_serverReferencedPakNames[ i ] = CopyString( Cmd_Argv( i ) );
		}
	}
}

/*
================
FS_InitFilesystem

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void FS_InitFilesystem( void )
{
	// allow command line arguments to override the following fs_* variables
	Com_StartupVariable( "fs_basepath" );
	Com_StartupVariable( "fs_homepath" );
	Com_StartupVariable( "fs_game" );
	// other command line variable settings don't happen
	// until after the filesystem has been initialized

	FS_Startup( BASEGAME );

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	// Arnout: we want the nice error message here as well
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 )
	{
		Com_Error( ERR_FATAL, "Couldn't load default.cfg — I am missing essential files — verify your installation?" );
	}

	Q_strncpyz( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
	Q_strncpyz( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );
}

/*
================
FS_Restart
================
*/
//void CL_PurgeCache( void );
void FS_Restart( int checksumFeed )
{
#ifndef DEDICATED
	// Arnout: big hack to clear the image cache on a FS_Restart
//	CL_PurgeCache();
#endif

	// free anything we currently have loaded
	FS_Shutdown( qfalse );

	// set the checksum feed
	fs_checksumFeed = checksumFeed;

	// clear pak references
	FS_ClearPakReferences( 0 );

	FS_Startup( BASEGAME );

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 )
	{
		// this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
		// (for instance a TA demo server)
		if ( lastValidBase[ 0 ] )
		{
			FS_PureServerSetLoadedPaks( "", "" );
			Cvar_Set( "fs_basepath", lastValidBase );
			Cvar_Set( "fs_gamedirvar", lastValidGame );
			lastValidBase[ 0 ] = '\0';
			lastValidGame[ 0 ] = '\0';
			FS_Restart( checksumFeed );
			Com_Error( ERR_DROP, "Invalid game folder" );
		}

		// TTimo - added some verbosity, 'couldn't load default.cfg' confuses the hell out of users
		Com_Error( ERR_FATAL, "Couldn't load default.cfg — I am missing essential files — verify your installation?" );
	}

	// bk010116 - new check before safeMode
	if ( Q_stricmp( fs_gamedirvar->string, lastValidGame ) )
	{
		// skip the wolfconfig.cfg if "safe" is on the command line
		if ( !Com_SafeMode() )
		{
			char *cl_profileStr = Cvar_VariableString( "cl_profile" );

			if ( cl_profileStr[ 0 ] )
			{
				// bani - check existing pid file and make sure it's ok
				if ( !Com_CheckProfile( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
				{
#ifdef NDEBUG
					Com_Printf(_( "^3WARNING: profile.pid found for profile '%s' — the system settings will revert to their defaults\n"), cl_profileStr );
					// ydnar: set crashed state
					Cbuf_AddText( "set com_crashed 1\n" );
#endif
				}

				// bani - write a new one
				if ( !Com_WriteProfile( va( "profiles/%s/profile.pid", cl_profileStr ) ) )
				{
					Com_Printf(_( "^3WARNING: couldn't write profiles/%s/profile.pid\n"), cl_profileStr );
				}

				// exec the config
				Cbuf_AddText( va( "exec profiles/%s/%s\n", cl_profileStr, CONFIG_NAME ) );
			}
			else
			{
				Cbuf_AddText( va( "exec %s\n", CONFIG_NAME ) );
			}
		}
	}

	Q_strncpyz( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
	Q_strncpyz( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );
}

/*
=================
FS_ConditionalRestart
restart if necessary

FIXME TTimo
this doesn't catch all cases where an FS_Restart is necessary
see show_bug.cgi?id=478
=================
*/
qboolean FS_ConditionalRestart( int checksumFeed )
{
	if ( fs_gamedirvar->modified || checksumFeed != fs_checksumFeed )
	{
		FS_Restart( checksumFeed );
		return qtrue;
	}

	return qfalse;
}

/*
========================================================================================

Handle based file calls for virtual machines

========================================================================================
*/

int     FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode )
{
	int      r;
	qboolean sync;

	sync = qfalse;

	switch ( mode )
	{
		case FS_READ:
			r = FS_FOpenFileRead( qpath, f, qtrue );
			break;

		case FS_WRITE:
			*f = FS_FOpenFileWrite( qpath );
			r = 0;

			if ( *f == 0 )
			{
				r = -1;
			}

			break;

		case FS_APPEND_SYNC:
			sync = qtrue;

		case FS_APPEND:
			*f = FS_FOpenFileAppend( qpath );
			r = 0;

			if ( *f == 0 )
			{
				r = -1;
			}

			break;

		case FS_READ_DIRECT:
			r = FS_FOpenFileDirect( qpath, f );
			break;

		case FS_UPDATE:
			*f = FS_FOpenFileUpdate( qpath, &r );
			r = 0;

			if ( *f == 0 )
			{
				r = -1;
			}

			break;

		default:
			Com_Error( ERR_FATAL, "FSH_FOpenFile: bad mode" );
	}

	if ( !f )
	{
		return r;
	}

	if ( *f )
	{
		if ( fsh[ *f ].zipFile == qtrue )
		{
			fsh[ *f ].baseOffset = unztell( fsh[ *f ].handleFiles.file.z );
		}
		else
		{
			fsh[ *f ].baseOffset = ftell( fsh[ *f ].handleFiles.file.o );
		}

		fsh[ *f ].fileSize = r;
		fsh[ *f ].streamed = qfalse;

		// uncommenting this makes fs_reads
		// use the background threads --
		// MAY be faster for loading levels depending on the use of file io
		// q3a not faster
		// wolf not faster

//		if (mode == FS_READ) {
//			Sys_BeginStreamedFile( *f, 0x4000 );
//			fsh[*f].streamed = qtrue;
//		}
	}

	fsh[ *f ].handleSync = sync;

	return r;
}

int     FS_FTell( fileHandle_t f )
{
	int pos;

	if ( fsh[ f ].zipFile == qtrue )
	{
		pos = unztell( fsh[ f ].handleFiles.file.z );
	}
	else
	{
		pos = ftell( fsh[ f ].handleFiles.file.o );
	}

	return pos;
}

void    FS_Flush( fileHandle_t f )
{
	fflush( fsh[ f ].handleFiles.file.o );
}

// CVE-2006-2082
// compared requested pak against the names as we built them in FS_ReferencedPakNames
qboolean FS_VerifyPak( const char *pak )
{
	char         teststring[ BIG_INFO_STRING ];
	searchpath_t *search;

	for ( search = fs_searchpaths; search; search = search->next )
	{
		if ( search->pack )
		{
			Q_strncpyz( teststring, search->pack->pakGamename, sizeof( teststring ) );
			Q_strcat( teststring, sizeof( teststring ), "/" );
			Q_strcat( teststring, sizeof( teststring ), search->pack->pakBasename );
			Q_strcat( teststring, sizeof( teststring ), ".pk3" );

			if ( !Q_stricmp( teststring, pak ) )
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

qboolean FS_IsPure( void )
{
	return fs_numServerPaks != 0;
}

void    FS_FilenameCompletion( const char *dir, const char *ext,
                               qboolean stripExt, void ( *callback )( const char *s ) )
{
	char **filenames;
	int  nfiles;
	int  i;
	char filename[ MAX_STRING_CHARS ];

	filenames = FS_ListFilteredFiles( dir, ext, NULL, &nfiles );

	FS_SortFileList( filenames, nfiles );

	for ( i = 0; i < nfiles; i++ )
	{
		FS_ConvertPath( filenames[ i ] );
		Q_strncpyz( filename, filenames[ i ], MAX_STRING_CHARS );

		if ( stripExt )
		{
			COM_StripExtension3( filename, filename, sizeof( filename ) );
		}

		callback( filename );
	}

	FS_FreeFileList( filenames );
}
