/*
===========================================================================
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

This file is part of Daemon.

OpenTrem is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenTrem is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*
 * snd_load.c
 * Loads a sound module, and redirects calls to it
 */

#include "client.h"
#include "snd_api.h"
#include "snd_codec.h"
#include "snd_local.h"
#include "snd_public.h"

#ifdef USE_SDL
#define USE_DYNAMIC
#include <SDL/SDL_loadso.h>
#define OBJTYPE void *
#define OBJLOAD(x)   SDL_LoadObject(x)
#define SYMLOAD(x,y) SDL_LoadFunction(x,y)
#define OBJFREE(x)   SDL_UnloadObject(x)

#elif defined _WIN32
#define USE_DYNAMIC
#include <windows.h>
#define OBJTYPE HMODULE
#define OBJLOAD(x)   LoadLibrary(x)
#define SYMLOAD(x,y) (void *)GetProcAddress(x,y)
#define OBJFREE(x)   FreeLibrary(x)

#elif defined __linux__ || defined __FreeBSD__ || defined MACOS_X
#define USE_DYNAMIC
#include <unistd.h>
#include <sys/types.h>
#include <dlfcn.h>
#define OBJTYPE void *
#define OBJLOAD(x)   dlopen(x, RTLD_LAZY | RTLD_GLOBAL)
#define SYMLOAD(x,y) dlsym(x,y)
#define OBJFREE(x)   dlclose(x)
#else
#warning No dynamic library loading code
#endif

static OBJTYPE  libhandle;
static qboolean useBuiltin = qfalse;

/*
 * Glue
 */
static PRINTF_LIKE(2) void QDECL SndPrintf( int print_level, const char *fmt, ... )
{
	va_list argptr;
	char    msg[ MAXPRINTMSG ];

	va_start( argptr, fmt );
	Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );

	if ( print_level == PRINT_ALL )
	{
		Com_Printf( "%s", msg );
	}
	else if ( print_level == PRINT_WARNING )
	{
		Com_Printf( S_COLOR_YELLOW "%s", msg );  // yellow
	}
	else if ( print_level == PRINT_DEVELOPER )
	{
		Com_DPrintf( S_COLOR_RED "%s", msg );  // red
	}
}

/*
 * Routing, runtime loading
 */
#ifdef USE_DYNAMIC
static sndexport_t *se = NULL;

static cvar_t      *s_module;

#ifdef ZONE_DEBUG
void *Snd_Malloc( int size )
{
	return Z_Malloc( size );
}

void Snd_Free( void *ptr )
{
	return Z_Free( ptr );
}

#endif

static qboolean S_InitModule()
{
	char        fn[ 1024 ];
	sndimport_t si;
	sndexport_t * ( *getapi )( int, sndimport_t * );

	s_module = Cvar_Get( "s_module", "openal", CVAR_ARCHIVE );
	Com_Printf( "using sound module %s\n", s_module->string );
	sprintf( fn, "%s/snd_%s" DLL_EXT, Sys_Cwd(), s_module->string );

	if ( ( libhandle = OBJLOAD( fn ) ) == 0 )
	{
		Com_Printf( "can't load sound module - bailing\n" );
		Com_Printf( "------------------------------------\n" );
		return qfalse;
	}

	getapi = SYMLOAD( libhandle, "GetSndAPI" );

	if ( !getapi )
	{
		OBJFREE( libhandle );
		libhandle = NULL;
		Com_Printf( "can't find GetSndAPI - bailing\n" );
		Com_Printf( "------------------------------------\n" );
		return qfalse;
	}

	si.Cmd_AddCommand = Cmd_AddCommand;
	si.Cmd_RemoveCommand = Cmd_RemoveCommand;
	si.Cmd_Argc = Cmd_Argc;
	si.Cmd_Argv = Cmd_Argv;
	si.Cmd_ExecuteText = Cbuf_ExecuteText;
	si.Printf = SndPrintf;
	si.Error = Com_Error;
	si.Milliseconds = Sys_Milliseconds;
#ifdef ZONE_DEBUG
	si.Malloc = Snd_Malloc;
	si.Free = Snd_Free;
#else
	si.Malloc = Z_Malloc;
	si.Free = Z_Free;
#endif

#ifdef HUNK_DEBUG
	si.Hunk_AllocDebug = Hunk_AllocDebug;
#else
	si.Hunk_Alloc = Hunk_Alloc;
#endif
	si.Hunk_AllocateTempMemory = Hunk_AllocateTempMemory;
	si.Hunk_FreeTempMemory = Hunk_FreeTempMemory;
	si.FS_ReadFile = FS_ReadFile;
	si.FS_FreeFile = FS_FreeFile;
	si.FS_WriteFile = FS_WriteFile;
	si.FS_FreeFileList = FS_FreeFileList;
	si.FS_ListFiles = FS_ListFiles;
	si.FS_FileIsInPAK = FS_FileIsInPAK;
	si.FS_FileExists = FS_FileExists;
	si.Cvar_Get = Cvar_Get;
	si.Cvar_Set = Cvar_Set;

	si.LoadSound = codec_load;
	si.StreamOpen = codec_open;
	si.StreamClose = codec_close;
	si.StreamRead = codec_read;
	si.strcat = Q_strcat;

	se = getapi( SND_API_VERSION, &si );

	if ( !se )
	{
		OBJFREE( libhandle );
		libhandle = NULL;
		Com_Printf( "call to GetSndAPI failed - bailing\n" );
		Com_Printf( "------------------------------------\n" );
		return qfalse;
	}

	if ( !se->Init() )
	{
		OBJFREE( libhandle );
		libhandle = NULL;
		se = NULL;
		Com_Printf( "call to Init failed - bailing\n" );
		Com_Printf( "------------------------------------\n" );
		return qfalse;
	}

	return qtrue;
}

#endif // USE_DYNAMIC

void S_Init( void )
{
	cvar_t *cv;

	Com_Printf( "------ Initializing Sound -----\n" );

	cv = Cvar_Get( "s_initsound", "1", 0 );

	if ( !cv->integer )
	{
		Com_Printf( "not initializing.\n" );
		Com_Printf( "------------------------------------\n" );
		return;
	}

	codec_init();

#ifdef USE_DYNAMIC
	cv = Cvar_Get( "s_usemodule", "1", CVAR_ARCHIVE );

	if ( !cv->integer )
	{
		useBuiltin = qtrue;
	}
	else
	{
		useBuiltin = qfalse;

		if ( !S_InitModule() )
		{
			useBuiltin = qtrue;
		}
	}

	if ( useBuiltin )
	{
		Com_Printf( "using builtin sound system\n" );
		SOrig_Init();
	}

#else // USE_DYNAMIC
	useBuiltin = qtrue;
	SOrig_Init();
#endif // !USE_DYNAMIC

	Com_Printf( "------------------------------------\n" );
}

void S_Shutdown( void )
{
	if ( useBuiltin )
	{
		SOrig_Shutdown();
	}
	else if ( se )
	{
		se->Shutdown();
		OBJFREE( libhandle );
		libhandle = NULL;
		se = NULL;
	}

	codec_shutdown();
}

void S_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
	if ( useBuiltin )
	{
		SOrig_StartSound( origin, entnum, entchannel, sfx );
	}
	else if ( se )
	{
		se->StartSound( origin, entnum, entchannel, sfx );
	}
}

void S_StartSoundEx( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
	if ( useBuiltin )
	{
		SOrig_StartSound( origin, entnum, entchannel, sfx );
	}
	else if ( se )
	{
		se->StartSound( origin, entnum, entchannel, sfx );
	}
}

void S_StartLocalSound( sfxHandle_t sfx, int channelNum )
{
	if ( useBuiltin )
	{
		SOrig_StartLocalSound( sfx, channelNum );
	}
	else if ( se )
	{
		se->StartLocalSound( sfx, channelNum );
	}
}

void S_StartBackgroundTrack( const char *intro, const char *loop )
{
	if ( useBuiltin )
	{
		SOrig_StartBackgroundTrack( intro, loop );
	}
	else if ( se )
	{
		se->StartBackgroundTrack( intro, loop );
	}
}

void S_StopBackgroundTrack( void )
{
	if ( useBuiltin )
	{
		SOrig_StopBackgroundTrack();
	}
	else if ( se )
	{
		se->StopBackgroundTrack();
	}
}

void S_RawSamples( int stream, int samples, int rate, int width, int channels,
                   const byte *data, float volume, int entityNum )
{
	if ( useBuiltin )
	{
		SOrig_RawSamples( stream, samples, rate, width, channels, data, volume, entityNum );
	}
	else if ( se )
	{
		se->RawSamples( stream, samples, rate, width, channels, data, volume, entityNum );
	}
}

void S_StopAllSounds( void )
{
	if ( useBuiltin )
	{
		SOrig_StopAllSounds();
	}
	else if ( se )
	{
		se->StopAllSounds();
	}
}

void S_ClearLoopingSounds( qboolean killall )
{
	if ( useBuiltin )
	{
		SOrig_ClearLoopingSounds( killall );
	}
	else if ( se )
	{
		se->ClearLoopingSounds( killall );
	}
}

void S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
	if ( useBuiltin )
	{
		SOrig_AddLoopingSound( entityNum, origin, velocity, sfx );
	}
	else if ( se )
	{
		se->AddLoopingSound( entityNum, origin, velocity, sfx );
	}
}

void S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
	if ( useBuiltin )
	{
		SOrig_AddRealLoopingSound( entityNum, origin, velocity, sfx );
	}
	else if ( se )
	{
		se->AddRealLoopingSound( entityNum, origin, velocity, sfx );
	}
}

void S_StopLoopingSound( int entityNum )
{
	if ( useBuiltin )
	{
		SOrig_StopLoopingSound( entityNum );
	}
	else if ( se )
	{
		se->StopLoopingSound( entityNum );
	}
}

void S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[ 3 ], int inwater )
{
	if ( useBuiltin )
	{
		SOrig_Respatialize( entityNum, origin, axis, inwater );
	}
	else if ( se )
	{
		se->Respatialize( entityNum, origin, axis, inwater );
	}
}

void S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	if ( useBuiltin )
	{
		SOrig_UpdateEntityPosition( entityNum, origin );
	}
	else if ( se )
	{
		se->UpdateEntityPosition( entityNum, origin );
	}
}

void S_Update( void )
{
	if ( useBuiltin )
	{
		SOrig_Update();
	}
	else if ( se )
	{
		se->Update();
	}
}

void S_DisableSounds( void )
{
	if ( useBuiltin )
	{
		SOrig_DisableSounds();
	}
	else if ( se )
	{
		se->DisableSounds();
	}
}

void S_BeginRegistration( void )
{
	if ( useBuiltin )
	{
		SOrig_BeginRegistration();
	}
	else if ( se )
	{
		se->BeginRegistration();
	}
}

sfxHandle_t     S_RegisterSound( const char *sample, qboolean compressed )
{
	if ( useBuiltin )
	{
		return SOrig_RegisterSound( sample, compressed );
	}
	else if ( se )
	{
		return se->RegisterSound( sample, compressed );
	}
	else
	{
		return 0;
	}
}

void S_ClearSoundBuffer( void )
{
	if ( useBuiltin )
	{
		SOrig_ClearSoundBuffer();
	}
	else if ( se )
	{
		se->ClearSoundBuffer();
	}
}

/*
=================
S_SoundDuration
=================
*/
int S_SoundDuration( sfxHandle_t handle )
{
	if ( useBuiltin )
	{
		return SOrig_SoundDuration( handle );
	}
	else if ( se )
	{
		return se->SoundDuration( handle );
	}
	else
	{
		return 0;
	}
}

/*
=================
S_GetVoiceAmplitude
=================
*/
int S_GetVoiceAmplitude( int entnum )
{
	if ( useBuiltin )
	{
		return SOrig_GetVoiceAmplitude( entnum );
	}
	else if ( se )
	{
		return se->GetVoiceAmplitude( entnum );
	}
	else
	{
		return 0;
	}
}

/*
=================
S_GetSoundLength
=================
*/
int S_GetSoundLength( sfxHandle_t sfxHandle )
{
	if ( useBuiltin )
	{
		return SOrig_GetSoundLength( sfxHandle );
	}
	else if ( se )
	{
		return se->GetSoundLength( sfxHandle );
	}
	else
	{
		return 0;
	}
}

#ifdef USE_VOIP
void S_StartCapture( void )
{
	if ( useBuiltin )
	{
		SOrig_StartCapture();
	}
	else if ( se )
	{
		se->StartCapture();
	}
}

int S_AvailableCaptureSamples( void )
{
	if ( useBuiltin )
	{
		return SOrig_AvailableCaptureSamples();
	}
	else if ( se )
	{
		return se->AvailableCaptureSamples();
	}
	else
	{
		return 0;
	}
}

void S_Capture( int samples, byte *data )
{
	if ( useBuiltin )
	{
		SOrig_Capture( samples, data );
	}
	else if ( se )
	{
		se->Capture( samples, data );
	}
}

void S_StopCapture( void )
{
	if ( useBuiltin )
	{
		SOrig_StopCapture();
	}
	else if ( se )
	{
		se->StopCapture();
	}
}

void S_MasterGain( float gain )
{
	if ( useBuiltin )
	{
		SOrig_MasterGain( gain );
	}
	else if ( se )
	{
		se->MasterGain( gain );
	}
}

#endif

/*
=================
S_GetCurrentSoundTime
=================
*/
int S_GetCurrentSoundTime( void )
{
	if ( useBuiltin )
	{
		return SOrig_GetCurrentSoundTime();
	}
	else if ( se )
	{
		return se->GetCurrentSoundTime();
	}
	else
	{
		return 0;
	}
}
