/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*
 * snd_al_local.h
 * Local definitions for the OpenAL sound system
 *
 * 2005-08-22
 *  Split buffer management out from snd_al.c
 *
 * 2005-08-2?
 *  Dunno. Stopped logging changes.
 *
 * 2005-08-30
 *  Added QAL (define OPENAL_STATIC to remove)
 */

#include "../client/snd_api.h"

#include "qal.h"

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#define DLLLOCAL
#elif (__GNUC__ >= 4) && defined __ELF__
#define DLLEXPORT __attribute__ ((visibility("default")))
#define DLLLOCAL __attribute__ ((visibility("hidden")))
#else
#define DLLEXPORT
#define DLLLOCAL
#endif

/**
 * Console variables
 */
extern cvar_t *s_volume;
extern cvar_t *s_musicVolume;
extern cvar_t *s_doppler;
extern cvar_t *s_khz;
extern cvar_t *s_precache;
extern cvar_t *s_gain;
extern cvar_t *s_sources;
extern cvar_t *s_dopplerFactor;
extern cvar_t *s_dopplerSpeed;
extern cvar_t *s_minDistance;
extern cvar_t *s_rolloff;

/**
 * Scaling constants
 */
#define POSITION_SCALE 1

/**
 * Util
 */
ALuint al_format(int width, int channels);

/**
 * Error messages
 */
char *al_errormsg(ALenum error);

/**
 * Buffer management
 */
// Called at init, shutdown
qboolean al_buf_init( void );
void al_buf_shutdown( void );

// Register and unregister sound effects
// If s_precache is 0, sound files will be loaded when they are used
// Otherwise, they will be loaded on registration
sfxHandle_t al_buf_register(const char *filename);

// Set up a sound effect for usage
// This reloads the sound effect if necessary, and keeps track of it's usage
void al_buf_use(sfxHandle_t sfx);

// Internal use - actually allocates and deallocates the buffers
void al_buf_load(sfxHandle_t sfx);
void al_buf_unload(sfxHandle_t sfx);
qboolean al_buf_evict( void );

// Grabs the OpenAL buffer from a sfxHandle_t
ALuint al_buf_get(sfxHandle_t sfx);

/**
 * Source management
 * Uses a simple priority-based approach to allocate channels
 * Ambient or looping sounds may be safely interrupted, and will not
 * cause loss - they will resume as soon as a channel is available.
 * One-shot sounds will be lost if there are insufficient channels.
 */
#define SRCPRI_AMBIENT	0	// Ambient sound effects
#define SRCPRI_ENTITY	1	// Entity sound effects
#define SRCPRI_ONESHOT	2	// One-shot sounds
#define SRCPRI_LOCAL	3	// Local sounds
#define SRCPRI_STREAM	4	// Streams (music, cutscenes)

#define SRC_NO_ENTITY	-1	// Has no associated entity
#define SRC_NO_CHANNEL	-1	// Has no specific channel

typedef int srcHandle_t;

// Called at init, shutdown
qboolean al_src_init( void );
void al_src_shutdown( void );

// Finds a free sound source
// If required, this will kick a sound off
// If you don't use it immediately, it may still be reallocated
srcHandle_t al_src_alloc(int priority, int entnum, int channel);

// Finds an active source with matching entity and channel numbers
// Returns -1 if there isn't one
srcHandle_t al_src_find(int entnum, int channel);

// Locks and unlocks a source
// Locked sources will not be automatically reallocated or managed
// Once unlocked, the source may be reallocated again
void al_src_lock(srcHandle_t src);
void al_src_unlock(srcHandle_t src);

// Entity position management
void al_src_entity_move(int entnum, const vec3_t origin);

// Local sound effect
void al_src_local(sfxHandle_t sfx, int channel);

// Play a one-shot sound effect
void al_src_play(sfxHandle_t sfx, vec3_t origin, int entnum, int entchannel);

// Start a looping sound effect
void al_src_loop_clear( void );
void al_src_loop(int priority, sfxHandle_t sfx, const vec3_t origin, const vec3_t velocity, int entnum);
void al_src_loop_stop(int entnum);

// Sound duration
int al_duration (sfxHandle_t sfx);

// Update state (move things around, manage sources, and so on)
void al_src_update( void );

// Shut the hell up
void al_src_shutup( void );

// Grab the raw source object
ALuint al_src_get(srcHandle_t src);

/**
 * Music
 */
void al_mus_init( void );
void al_mus_shutdown( void );
void al_mus_update( void );

/**
 * Stream
 */
void al_stream_raw(int samples, int rate, int width, int channels,
		   const byte *data, float volume);
void al_stream_update( void );
void al_stream_die( void );

/**
 * Engine interface
 */
extern sndexport_t se;
extern sndimport_t si;

qboolean SndAl_Init(void);
void SndAl_Shutdown(void);
void SndAl_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx );
void SndAl_StartLocalSound( sfxHandle_t sfx, int channelNum );
void SndAl_StartBackgroundTrack( const char *intro, const char *loop );
void SndAl_StopBackgroundTrack( void );
void SndAl_RawSamples(int stream, int samples, int rate, int width, int channels, const byte *data, float volume, int entityNum);
void SndAl_StopAllSounds( void );
void SndAl_ClearLoopingSounds( qboolean killall );
void SndAl_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void SndAl_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void SndAl_StopLoopingSound(int entityNum );
void SndAl_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );
void SndAl_UpdateEntityPosition( int entityNum, const vec3_t origin );
void SndAl_Update( void );
void SndAl_DisableSounds( void );
void SndAl_BeginRegistration( void );
sfxHandle_t SndAl_RegisterSound( const char *sample, qboolean compressed );
void SndAl_ClearSoundBuffer( void );
int SndAl_SoundDuration( sfxHandle_t sfx );

#ifdef USE_VOIP
void SndAl_InitCapture( qboolean usingAL );
void SndAl_StartCapture( void );
int SndAl_AvailableCaptureSamples( void );
void SndAl_Capture( int samples, byte *data );
void SndAl_StopCapture( void );
void SndAl_MasterGain( float gain );
#endif
