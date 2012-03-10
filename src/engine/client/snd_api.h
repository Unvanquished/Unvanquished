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
 * Sound API definition
 * Shared by client and sound modules
 */

#ifndef _SND_API_H
#define _SND_API_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#define SND_API_VERSION 1

// Codec definitions
typedef struct snd_info_s snd_info_t;
typedef struct snd_stream_s snd_stream_t;
typedef struct snd_codec_s snd_codec_t;

struct snd_info_s
{
	int rate;
	int width;
	int channels;
	int samples;
	int size;
	int dataofs;
};

struct snd_stream_s
{
	snd_codec_t *codec;
	fileHandle_t file;
	snd_info_t info;
	int pos;
	void *ptr;
	int length;
};

// Exported functions
typedef struct
{
	qboolean (*Init)(void);
	void (*Shutdown)(void);
	void (*StartSound)( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx );
	void (*StartLocalSound)( sfxHandle_t sfx, int channelNum );
	void (*StartBackgroundTrack)( const char *intro, const char *loop );
	void (*StopBackgroundTrack)( void );
	void (*RawSamples)(int stream, int samples, int rate, int width, int channels, const byte *data, float volume, int entityNum);
	void (*StopAllSounds)( void );
	void (*ClearLoopingSounds)( qboolean killall );
	void (*AddLoopingSound)( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
	void (*AddRealLoopingSound)( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
	void (*StopLoopingSound)(int entityNum );
	void (*Respatialize)( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );
	void (*UpdateEntityPosition)( int entityNum, const vec3_t origin );
	void (*Update)( void );
	void (*DisableSounds)( void );
	void (*BeginRegistration)( void );
	sfxHandle_t (*RegisterSound)( const char *sample, qboolean compressed );
	void (*ClearSoundBuffer)( void );
	int (*SoundDuration) ( sfxHandle_t handle );
#ifdef USE_VOIP
	void (*StartCapture)( void );
	int (*AvailableCaptureSamples)( void );
	void (*Capture)( int samples, byte *data );
	void (*StopCapture)( void );
	void (*MasterGain)( float gain );
#endif
	int (*GetVoiceAmplitude) ( int entnum );
	int (*GetSoundLength)( sfxHandle_t sfxHandle );
	int (*GetCurrentSoundTime) ( void );
} sndexport_t;

// Imported functions
typedef struct
{
	void	(QDECL *Printf)( int printLevel, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
	void	(QDECL *Error)( int errorLevel, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
	int		(*Milliseconds)( void );
#ifdef HUNK_DEBUG
	void	*(*Hunk_AllocDebug)( int size, ha_pref pref, char *label, char *file, int line );
#else
	void	*(*Hunk_Alloc)( int size, ha_pref pref );
#endif
	void	*(*Hunk_AllocateTempMemory)( int size );
	void	(*Hunk_FreeTempMemory)( void *block );

	// dynamic memory allocator for things that need to be freed
	void	*(*Malloc)( int bytes );
	void	(*Free)( void *buf );

	cvar_t	*(*Cvar_Get)( const char *name, const char *value, int flags );
	void	(*Cvar_Set)( const char *name, const char *value );

	void	(*Cmd_AddCommand)( const char *name, void(*cmd)(void) );
	void	(*Cmd_RemoveCommand)( const char *name );

	int		(*Cmd_Argc) (void);
	char	*(*Cmd_Argv) (int i);

	void	(*Cmd_ExecuteText) (int exec_when, const char *text);

	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int		(*FS_FileIsInPAK)( const char *name, int *pCheckSum );
	int		(*FS_ReadFile)( const char *name, void **buf );
	void	(*FS_FreeFile)( void *buf );
	char **	(*FS_ListFiles)( const char *name, const char *extension, int *numfilesfound );
	void	(*FS_FreeFileList)( char **filelist );
	void	(*FS_WriteFile)( const char *qpath, const void *buffer, int size );
	qboolean (*FS_FileExists)( const char *file );

	// Sound effect loading
	void *(*LoadSound)(const char *filename, snd_info_t *info);

	// Music stream loading
	snd_stream_t *(*StreamOpen)(const char *filename);
	void (*StreamClose)(snd_stream_t *stream);
	int (*StreamRead)(snd_stream_t *stream, int bytes, void *buffer);

	void (*strcat) (char *dest, int destsize, const char *src);
} sndimport_t;

//sndexport_t *GetSndAPI( int apiVersion, sndimport_t *simp );

#endif // _SND_API_H