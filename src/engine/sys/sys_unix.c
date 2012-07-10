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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <libgen.h>
#include <fcntl.h>
#include <fenv.h>


#ifndef DEDICATED
#include <SDL.h>
#include <SDL_syswm.h>
#endif

qboolean    stdinIsATTY;

// Used to determine where to store user-specific files
static char homePath[ MAX_OSPATH ] = { 0 };

static char exit_cmdline[ MAX_CMD ] = "";

/*
==================
Sys_DefaultHomePath
==================
*/
char *Sys_DefaultHomePath( void )
{
	char *p;

	if ( !*homePath )
	{
		if ( ( p = getenv( "HOME" ) ) != NULL )
		{
			Q_strncpyz( homePath, p, sizeof( homePath ) );
#ifdef MACOS_X
			Q_strcat( homePath, sizeof( homePath ),
			          "/Library/Application Support/" PRODUCT_NAME );
#else
			Q_strcat( homePath, sizeof( homePath ), "/." PRODUCT_NAME );
#endif
		}
	}

	return homePath;
}

#ifndef MACOS_X

/*
================
Sys_TempPath
================
*/
const char *Sys_TempPath( void )
{
	const char *TMPDIR = getenv( "TMPDIR" );

	if ( TMPDIR == NULL || TMPDIR[ 0 ] == '\0' )
	{
		return "/tmp";
	}
	else
	{
		return TMPDIR;
	}
}

#endif

/*
==================
chmod OR on a file
==================
*/
void Sys_Chmod( char *file, int mode )
{
	struct stat s_buf;

	int         perm;

	if ( stat( file, &s_buf ) != 0 )
	{
		Com_Printf( "stat('%s')  failed: errno %d\n", file, errno );
		return;
	}

	perm = s_buf.st_mode | mode;

	if ( chmod( file, perm ) != 0 )
	{
		Com_Printf( "chmod('%s', %d) failed: errno %d\n", file, perm, errno );
	}

	Com_DPrintf( "chmod +%d '%s'\n", mode, file );
}

/*
================
Sys_Milliseconds
================
*/

/* base time in seconds, that's our origin
   timeval:tv_sec is an int:
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
   using unsigned long data type to work right with Sys_XTimeToSysTime */

time_t initial_tv_sec = 0;

/* current time in ms, using sys_timeBase as origin
   NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
     0x7fffffff ms - ~24 days
   although timeval:tv_usec is an int, I'm not sure whether it is actually used as an unsigned int
     (which would affect the wrap period) */
int Sys_Milliseconds( void )
{
	struct timeval tp;

	gettimeofday( &tp, NULL );

	return ( tp.tv_sec - initial_tv_sec ) * 1000 + tp.tv_usec / 1000;
}

/*
==================
Sys_RandomBytes
==================
*/
qboolean Sys_RandomBytes( byte *string, int len )
{
	FILE *fp;

	fp = fopen( "/dev/urandom", "r" );

	if ( !fp )
	{
		return qfalse;
	}

	if ( !fread( string, sizeof( byte ), len, fp ) )
	{
		fclose( fp );
		return qfalse;
	}

	fclose( fp );
	return qtrue;
}

/*
==================
Sys_GetCurrentUser
==================
*/
char *Sys_GetCurrentUser( void )
{
	struct passwd *p;

	if ( ( p = getpwuid( getuid() ) ) == NULL )
	{
		return "player";
	}

	return p->pw_name;
}


/*
==================
Sys_GetClipboardData
==================
*/
#ifndef MACOS_X
#ifndef DEDICATED
static struct {
	Display *display;
	Window  window;
	void  ( *lockDisplay )( void );
	void  ( *unlockDisplay )( void );
	Atom    utf8;
} x11 = { NULL };
#endif

char *Sys_GetClipboardData( clipboard_t clip )
{
#ifndef DEDICATED
	// derived from SDL clipboard code (http://hg.assembla.com/SDL_Clipboard)
	Window        owner;
	Atom          selection;
	Atom          converted;
	Atom          selectionType;
	int           selectionFormat;
	unsigned long nbytes;
	unsigned long overflow;
	char          *src;

	if ( x11.display == NULL )
	{
		SDL_SysWMinfo info;

		SDL_VERSION( &info.version );
		if ( SDL_GetWMInfo( &info ) != 1 || info.subsystem != SDL_SYSWM_X11 )
		{
			Com_Printf("Not on X11? (%d)\n",info.subsystem);
			return NULL;
		}

		x11.display = info.info.x11.display;
		x11.window = info.info.x11.window;
		x11.lockDisplay = info.info.x11.lock_func;
		x11.unlockDisplay = info.info.x11.unlock_func;
		x11.utf8 = XInternAtom( x11.display, "UTF8_STRING", False );

		SDL_EventState( SDL_SYSWMEVENT, SDL_ENABLE );
		//SDL_SetEventFilter( Sys_ClipboardFilter );
	}

	x11.lockDisplay();

	switch ( clip )
	{
	// preference order; we use fall-through
	default: // SELECTION_CLIPBOARD
		selection = XInternAtom( x11.display, "CLIPBOARD", False );
		owner = XGetSelectionOwner( x11.display, selection );
		if ( owner != None && owner != x11.window )
		{
			break;
		}

	case SELECTION_PRIMARY:
		selection = XA_PRIMARY;
		owner = XGetSelectionOwner( x11.display, selection );
		if ( owner != None && owner != x11.window )
		{
			break;
		}

	case SELECTION_SECONDARY:
		selection = XA_SECONDARY;
		owner = XGetSelectionOwner( x11.display, selection );
	}

	converted = XInternAtom( x11.display, "UNVANQUISHED_SELECTION", False );
	x11.unlockDisplay();

	if ( owner == None || owner == x11.window )
	{
		selection = XA_CUT_BUFFER0;
		owner = RootWindow( x11.display, DefaultScreen( x11.display ) );
	}
	else
	{
		SDL_Event event;

		x11.lockDisplay();
		owner = x11.window;
		//FIXME: when we can respond to clipboard requests, don't alter selection
		XConvertSelection( x11.display, selection, x11.utf8, converted, owner, CurrentTime );
		x11.unlockDisplay();

		for (;;)
		{
			SDL_WaitEvent( &event );

			if ( event.type == SDL_SYSWMEVENT )
			{
				XEvent xevent = event.syswm.msg->event.xevent;

				if ( xevent.type == SelectionNotify &&
				     xevent.xselection.requestor == owner )
				{
					break;
				}
			}
		}
	}

	x11.lockDisplay ();

	if ( XGetWindowProperty( x11.display, owner, converted, 0, INT_MAX / 4,
	                         False, x11.utf8, &selectionType, &selectionFormat,
	                         &nbytes, &overflow, (unsigned char **) &src ) == Success )
	{
		char *dest = NULL;

		if ( selectionType == x11.utf8 )
		{
			dest = Z_Malloc( nbytes + 1 );
			memcpy( dest, src, nbytes );
			dest[ nbytes ] = 0;
		}
		XFree( src );

		x11.unlockDisplay();
		return dest;
	}

	x11.unlockDisplay();
#endif // !DEDICATED
	return NULL;
}
#endif // !MACOSX

#define MEM_THRESHOLD 96 * 1024 * 1024

/*
==================
Sys_LowPhysicalMemory

TODO
==================
*/
qboolean Sys_LowPhysicalMemory( void )
{
	return qfalse;
}

/*
==================
Sys_Basename
==================
*/
const char *Sys_Basename( char *path )
{
	return basename( path );
}

/*
==================
Sys_Dirname
==================
*/
const char *Sys_Dirname( char *path )
{
	return dirname( path );
}

/*
==================
Sys_Mkdir
==================
*/
qboolean Sys_Mkdir( const char *path )
{
	int result = mkdir( path, 0750 );

	if ( result != 0 )
	{
		return errno == EEXIST;
	}

	return qtrue;
}

/*
==================
Sys_Cwd
==================
*/
char *Sys_Cwd( void )
{
#ifdef MACOS_X
	char *apppath = Sys_DefaultAppPath();

	if ( apppath[ 0 ] && apppath[ 0 ] != '.' )
	{
		return apppath;
	}

#endif

	static char cwd[ MAX_OSPATH ];

	char        *result = getcwd( cwd, sizeof( cwd ) - 1 );

	if ( result != cwd )
	{
		return NULL;
	}

	cwd[ MAX_OSPATH - 1 ] = 0;

	return cwd;
}

/*
==============================================================

DIRECTORY SCANNING

==============================================================
*/

#define MAX_FOUND_FILES 0x1000

/*
==================
Sys_ListFilteredFiles
==================
*/
void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **list, int *numfiles )
{
	char          search[ MAX_OSPATH ], newsubdirs[ MAX_OSPATH ];
	char          filename[ MAX_OSPATH ];
	DIR           *fdir;
	struct dirent *d;

	struct stat   st;

	if ( *numfiles >= MAX_FOUND_FILES - 1 )
	{
		return;
	}

	if ( strlen( subdirs ) )
	{
		Com_sprintf( search, sizeof( search ), "%s/%s", basedir, subdirs );
	}
	else
	{
		Com_sprintf( search, sizeof( search ), "%s", basedir );
	}

	if ( ( fdir = opendir( search ) ) == NULL )
	{
		return;
	}

	while ( ( d = readdir( fdir ) ) != NULL )
	{
		Com_sprintf( filename, sizeof( filename ), "%s/%s", search, d->d_name );

		if ( stat( filename, &st ) == -1 )
		{
			continue;
		}

		if ( st.st_mode & S_IFDIR )
		{
			if ( Q_stricmp( d->d_name, "." ) && Q_stricmp( d->d_name, ".." ) )
			{
				if ( strlen( subdirs ) )
				{
					Com_sprintf( newsubdirs, sizeof( newsubdirs ), "%s/%s", subdirs, d->d_name );
				}
				else
				{
					Com_sprintf( newsubdirs, sizeof( newsubdirs ), "%s", d->d_name );
				}

				Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
			}
		}

		if ( *numfiles >= MAX_FOUND_FILES - 1 )
		{
			break;
		}

		Com_sprintf( filename, sizeof( filename ), "%s/%s", subdirs, d->d_name );

		if ( !Com_FilterPath( filter, filename, qfalse ) )
		{
			continue;
		}

		list[ *numfiles ] = CopyString( filename );
		( *numfiles ) ++;
	}

	closedir( fdir );
}

/*
==================
Sys_ListFiles
==================
*/
char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs )
{
	struct dirent *d;

	DIR           *fdir;
	qboolean      dironly = wantsubs;
	char          search[ MAX_OSPATH ];
	int           nfiles;
	char          **listCopy;
	char          *list[ MAX_FOUND_FILES ];
	int           i;
	struct stat   st;

//	int           extLen;

	if ( filter )
	{
		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = NULL;
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

	if ( !extension )
	{
		extension = "";
	}

	if ( extension[ 0 ] == '/' && extension[ 1 ] == 0 )
	{
		extension = "";
		dironly = qtrue;
	}

//	extLen = strlen( extension );

	// search
	nfiles = 0;

	if ( ( fdir = opendir( directory ) ) == NULL )
	{
		*numfiles = 0;
		return NULL;
	}

	while ( ( d = readdir( fdir ) ) != NULL )
	{
		Com_sprintf( search, sizeof( search ), "%s/%s", directory, d->d_name );

		if ( stat( search, &st ) == -1 )
		{
			continue;
		}

		if ( ( dironly && !( st.st_mode & S_IFDIR ) ) ||
		     ( !dironly && ( st.st_mode & S_IFDIR ) ) )
		{
			continue;
		}

		if ( *extension )
		{
			if ( strlen( d->d_name ) < strlen( extension ) ||
			     Q_stricmp(
			       d->d_name + strlen( d->d_name ) - strlen( extension ),
			       extension ) )
			{
				continue; // didn't match
			}
		}

		if ( nfiles == MAX_FOUND_FILES - 1 )
		{
			break;
		}

		list[ nfiles ] = CopyString( d->d_name );
		nfiles++;
	}

	list[ nfiles ] = NULL;

	closedir( fdir );

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
==================
Sys_FreeFileList
==================
*/
void Sys_FreeFileList( char **list )
{
	int i;

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
==================
Sys_Sleep

Block execution for msec or until input is received.
==================
*/
void Sys_Sleep( int msec )
{
	if ( msec == 0 )
	{
		return;
	}

	if ( stdinIsATTY )
	{
		fd_set fdset;

		FD_ZERO( &fdset );
		FD_SET( STDIN_FILENO, &fdset );

		if ( msec < 0 )
		{
			select( STDIN_FILENO + 1, &fdset, NULL, NULL, NULL );
		}
		else
		{
			struct timeval timeout;

			timeout.tv_sec = msec / 1000;
			timeout.tv_usec = ( msec % 1000 ) * 1000;
			select( STDIN_FILENO + 1, &fdset, NULL, NULL, &timeout );
		}
	}
	else
	{
		// With nothing to select() on, we can't wait indefinitely
		if ( msec < 0 )
		{
			msec = 10;
		}

		usleep( msec * 1000 );
	}
}

/*
==============
Sys_ErrorDialog

Display an error message
==============
*/
void Sys_ErrorDialog( const char *error )
{
	char         buffer[ 1024 ];
	unsigned int size;
	int          f = -1;
	const char   *homepath = Cvar_VariableString( "fs_homepath" );
	const char   *gamedir = Cvar_VariableString( "fs_gamedir" );
	const char   *fileName = "crashlog.txt";
	char         *ospath = FS_BuildOSPath( homepath, gamedir, fileName );

	Sys_Print( va( "%s\n", error ) );

#ifndef DEDICATED
	// We may have grabbed input devices. Need to release.
	if ( SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		SDL_WM_GrabInput( SDL_GRAB_OFF );
	}

	Sys_Dialog( DT_ERROR, va( "%s. See \"%s\" for details.", error, ospath ), "Error" );
#endif

	// Make sure the write path for the crashlog exists...
	if ( FS_CreatePath( ospath ) )
	{
		Com_Printf( "ERROR: couldn't create path '%s' for crash log.\n", ospath );
		return;
	}

	// We might be crashing because we maxed out the Quake MAX_FILE_HANDLES,
	// which will come through here, so we don't want to recurse forever by
	// calling FS_FOpenFileWrite()...use the Unix system APIs instead.
	f = open( ospath, O_CREAT | O_TRUNC | O_WRONLY, 0640 );

	if ( f == -1 )
	{
		Com_Printf( "ERROR: couldn't open %s\n", fileName );
		return;
	}

	// We're crashing, so we don't care much if write() or close() fails.
	while ( ( size = CON_LogRead( buffer, sizeof( buffer ) ) ) > 0 )
	{
		if ( write( f, buffer, size ) != size )
		{
			Com_Printf( "ERROR: couldn't fully write to %s\n", fileName );
			break;
		}
	}

	close( f );
}

#ifndef MACOS_X

/*
=============
Sys_System

Avoid all that ugliness with shell quoting
=============
*/
static int Sys_System( char *cmd, ... )
{
	va_list ap;
	char    *argv[ 16 ] = { NULL };
	pid_t   pid;
	int     r;

	va_start( ap, cmd );
	argv[ 0 ] = cmd;

	for ( r = 1; r < ARRAY_LEN( argv ) - 1; ++r )
	{
		argv[ r ] = va_arg( ap, char * );
		if ( !argv[ r ] )
		{
			break;
		}
	}

	va_end( ap );

	switch ( pid = fork() )
	{
	case 0: // child
		// give me an exec() which takes a va_list...
		execvp( cmd, argv );
		exit( 2 );

	case -1: // error
		return -1;

	default: // parent
		do
		{
			waitpid( pid, &r, 0 );
		} while ( !WIFEXITED( r ) );

		return WEXITSTATUS( r );
	}
}

/*
==============
Sys_ZenityCommand
==============
*/
static int Sys_ZenityCommand( dialogType_t type, const char *message, const char *title )
{
	char       opt_text[ 512 ], opt_title[ 512 ];
	const char *options[ 3 ] = { NULL };

	switch ( type )
	{
		default:
		case DT_INFO:
			options[ 0 ] = "--info";
			break;

		case DT_WARNING:
			options[ 0 ] = "--warning";
			break;

		case DT_ERROR:
			options[ 0 ] = "--error";
			break;

		case DT_YES_NO:
			options[ 0 ] = "--question";
			options[ 1 ] = "--ok-label=Yes";
			options[ 2 ] = "--cancel-label=No";
			break;

		case DT_OK_CANCEL:
			options[ 0 ] = "--question";
			options[ 1 ] = "--ok-label=OK";
			options[ 2 ] = "--cancel-label=Cancel";
			break;
	}

	Com_sprintf( opt_text, sizeof( opt_text ), "--text=%s", message );
	Com_sprintf( opt_title, sizeof( opt_title ), "--title=%s", title );

	return Sys_System( "zenity", options[ 0 ], opt_text, opt_title, options[ 1 ], options[ 2 ], NULL );
}

/*
==============
Sys_KdialogCommand
==============
*/
static int Sys_KdialogCommand( dialogType_t type, const char *message, const char *title )
{
	const char *options;

	switch ( type )
	{
		default:
		case DT_INFO:
			options = "--msgbox";
			break;

		case DT_WARNING:
			options = "--sorry";
			break;

		case DT_ERROR:
			options = "--error";
			break;

		case DT_YES_NO:
			options = "--warningyesno";
			break;

		case DT_OK_CANCEL:
			options = "--warningcontinuecancel";
			break;
	}

	return Sys_System( "kdialog", options, "--title", title, message, NULL );
}

/*
==============
Sys_XmessageCommand
==============
*/
static int Sys_XmessageCommand( dialogType_t type, const char *message, const char *title )
{
	const char *options;

	switch ( type )
	{
		default:
			options = "OK";
			break;

		case DT_YES_NO:
			options = "Yes:0,No:1";
			break;

		case DT_OK_CANCEL:
			options = "OK:0,Cancel:1";
			break;
	}

	return Sys_System( "xmessage", "-center", "-buttons", options, message, NULL );
}

/*
==============
Sys_Dialog

Display a *nix dialog box
==============
*/
dialogResult_t Sys_Dialog( dialogType_t type, const char *message, const char *title )
{
	typedef enum
	{
	  NONE = 0,
	  ZENITY,
	  KDIALOG,
	  XMESSAGE,
	  NUM_DIALOG_PROGRAMS
	} dialogCommandType_t;
	typedef int ( *dialogCommandBuilder_t )( dialogType_t, const char *, const char * );

	const char             *session = getenv( "DESKTOP_SESSION" );
	qboolean               tried[ NUM_DIALOG_PROGRAMS ] = { qfalse };
	dialogCommandBuilder_t commands[ NUM_DIALOG_PROGRAMS ] = { NULL };
	dialogCommandType_t    preferredCommandType = NONE;

	commands[ ZENITY ] = &Sys_ZenityCommand;
	commands[ KDIALOG ] = &Sys_KdialogCommand;
	commands[ XMESSAGE ] = &Sys_XmessageCommand;

	// This may not be the best way
	if ( !Q_stricmp( session, "gnome" ) )
	{
		preferredCommandType = ZENITY;
	}
	else if ( !Q_stricmp( session, "kde" ) )
	{
		preferredCommandType = KDIALOG;
	}

	while ( 1 )
	{
		int i;
		int exitCode;

		for ( i = NONE + 1; i < NUM_DIALOG_PROGRAMS; i++ )
		{
			if ( preferredCommandType != NONE && preferredCommandType != i )
			{
				continue;
			}

			if ( !tried[ i ] )
			{
				exitCode = commands[ i ]( type, message, title );

				if ( exitCode >= 0 )
				{
					switch ( type )
					{
						case DT_YES_NO:
							return exitCode ? DR_NO : DR_YES;

						case DT_OK_CANCEL:
							return exitCode ? DR_CANCEL : DR_OK;

						default:
							return DR_OK;
					}
				}

				tried[ i ] = qtrue;

				// The preference failed, so start again in order
				if ( preferredCommandType != NONE )
				{
					preferredCommandType = NONE;
					break;
				}
			}
		}

		for ( i = NONE + 1; i < NUM_DIALOG_PROGRAMS; i++ )
		{
			if ( !tried[ i ] )
			{
				continue;
			}
		}

		break;
	}

	Com_DPrintf( S_COLOR_YELLOW "WARNING: failed to show a dialog\n" );
	return DR_OK;
}

#endif

/*
==================
Sys_DoStartProcess
actually forks and starts a process

UGLY HACK:
  Sys_StartProcess works with a command line only
  if this command line is actually a binary with command line parameters,
  the only way to do this on unix is to use a system() call
  but system doesn't replace the current process, which leads to a situation like:
  wolf.x86--spawned_process.x86
  in the case of auto-update for instance, this will cause write access denied on wolf.x86:
  wolf-beta/2002-March/000079.html
  we hack the implementation here, if there are no space in the command line, assume it's a straight process and use execl
  otherwise, use system ..
  The clean solution would be Sys_StartProcess and Sys_StartProcess_Args..
==================
*/
void Sys_DoStartProcess( char *cmdline )
{
	switch ( fork() )
	{
		case -1:
			// main thread
			break;

		case 0:
			if ( strchr( cmdline, ' ' ) )
			{
				system( cmdline );
			}
			else
			{
				execl( cmdline, cmdline, NULL );
				printf( "execl failed: %s\n", strerror( errno ) );
			}

			_exit( 0 );
	}
}

/*
==================
Sys_StartProcess
if !doexit, start the process asap
otherwise, push it for execution at exit
(i.e. let complete shutdown of the game and freeing of resources happen)
NOTE: might even want to add a small delay?
==================
*/
void Sys_StartProcess( char *cmdline, qboolean doexit )
{
	if ( doexit )
	{
		Com_DPrintf( "Sys_StartProcess %s (delaying to final exit)\n", cmdline );
		Q_strncpyz( exit_cmdline, cmdline, MAX_CMD );
		Cbuf_ExecuteText( EXEC_APPEND, "quit\n" );
		return;
	}

	Com_DPrintf( "Sys_StartProcess %s\n", cmdline );
	Sys_DoStartProcess( cmdline );
}

/*
=================
Sys_OpenURL
=================
*/
void Sys_OpenURL( const char *url, qboolean doexit )
{
	char            *basepath, *homepath, *pwdpath;
	char            fname[ 20 ];
	char            fn[ MAX_OSPATH ];
	char            cmdline[ MAX_CMD ];

	static qboolean doexit_spamguard = qfalse;

	if ( doexit_spamguard )
	{
		Com_DPrintf( "Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url );
		return;
	}

	Com_Printf( "Open URL: %s\n", url );
	// opening a URL on *nix can mean a lot of things
	// just spawn a script instead of deciding for the user :-)

	// do the setup before we fork
	// search for an openurl.sh script
	// search procedure taken from Sys_LoadDll
	Q_strncpyz( fname, "openurl.sh", 20 );

	pwdpath = Sys_Cwd();
	Com_sprintf( fn, MAX_OSPATH, "%s/%s", pwdpath, fname );

	if ( access( fn, X_OK ) == -1 )
	{
		Com_DPrintf( "%s not found\n", fn );
		// try in home path
		homepath = Cvar_VariableString( "fs_homepath" );
		Com_sprintf( fn, MAX_OSPATH, "%s/%s", homepath, fname );

		if ( access( fn, X_OK ) == -1 )
		{
			Com_DPrintf( "%s not found\n", fn );
			// basepath, last resort
			basepath = Cvar_VariableString( "fs_basepath" );
			Com_sprintf( fn, MAX_OSPATH, "%s/%s", basepath, fname );

			if ( access( fn, X_OK ) == -1 )
			{
				Com_DPrintf( "%s not found\n", fn );
				Com_Printf( "Can't find script '%s' to open requested URL (use +set developer 1 for more verbosity)\n", fname );
				// we won't quit
				return;
			}
		}
	}

	// show_bug.cgi?id=612
	if ( doexit )
	{
		doexit_spamguard = qtrue;
	}

	Com_DPrintf( "URL script: %s\n", fn );

	// build the command line
	Com_sprintf( cmdline, MAX_CMD, "%s '%s' &", fn, url );

	Sys_StartProcess( cmdline, doexit );
}

qboolean Sys_OpenUrl( const char *url )
{
	char *browser = getenv( "BROWSER" );
	char *kde_session = getenv( "KDE_FULL_SESSION" );
	char *gnome_session = getenv( "GNOME_DESKTOP_SESSION_ID" );

	//Try to use xdg-open, if not, try default, then kde, gnome
	if ( browser )
	{
		Sys_Fork( browser, url );
		return qtrue;
	}
	else if ( kde_session && Q_stricmp( "true", kde_session ) == 0 )
	{
		Sys_Fork( "konqueror", url );
		return qtrue;
	}
	else if ( gnome_session )
	{
		Sys_Fork( "gnome-open", url );
		return qtrue;
	}
	else
	{
		Sys_Fork( "/usr/bin/firefox", url );
	}

	// open url somehow
	return qtrue;
}

qboolean Sys_Fork( const char *path, const char *cmdLine )
{
	int pid;

	pid = fork();

	if ( pid == 0 )
	{
		struct stat filestat;

		//Try to set the executable bit
		if ( stat( path, &filestat ) == 0 )
		{
			chmod( path, filestat.st_mode | S_IXUSR );
		}

		execlp( path, path, cmdLine, NULL );
		printf( "Exec Failed for: %s\n", path );
		_exit( 255 );
	}
	else if ( pid == -1 )
	{
		return qfalse;
	}

	return qtrue;
}

/*
==============
Sys_GLimpSafeInit

Unix specific "safe" GL implementation initialisation
==============
*/
void Sys_GLimpSafeInit( void )
{
	// NOP
}

/*
==============
Sys_GLimpInit

Unix specific GL implementation initialisation
==============
*/
void Sys_GLimpInit( void )
{
	// NOP
}

void Sys_SetFloatEnv( void )
{
	// rounding towards 0
	fesetround( FE_TOWARDZERO );
}

/*
==============
Sys_PlatformInit

Unix specific initialisation
==============
*/
void Sys_PlatformInit( void )
{
	const char *term = getenv( "TERM" );

	signal( SIGHUP, Sys_SigHandler );
	signal( SIGQUIT, Sys_SigHandler );
	signal( SIGTRAP, Sys_SigHandler );
	signal( SIGIOT, Sys_SigHandler );
	signal( SIGBUS, Sys_SigHandler );

	stdinIsATTY = isatty( STDIN_FILENO ) &&
	              !( term && ( !strcmp( term, "raw" ) || !strcmp( term, "dumb" ) ) );
}

/*
==============
 Sys_PlatformExit

 Unix specific deinitialisation
==============
*/
 void Sys_PlatformExit( void )
{
}

/*
==============
Sys_SetEnv

set/unset environment variables (empty value removes it)
==============
*/

void Sys_SetEnv( const char *name, const char *value )
{
	if ( value && *value )
	{
		setenv( name, value, 1 );
	}
	else
	{
		unsetenv( name );
	}
}

/*
==============
Sys_PID
==============
*/
int Sys_PID( void )
{
	return getpid();
}

/*
==============
Sys_PIDIsRunning
==============
*/
qboolean Sys_PIDIsRunning( int pid )
{
	return kill( pid, 0 ) == 0;
}

/*
==============
Sys_IsNumLockDown
==============
*/
qboolean Sys_IsNumLockDown( void )
{
	// FIXME for Linux
	return qfalse;
}
