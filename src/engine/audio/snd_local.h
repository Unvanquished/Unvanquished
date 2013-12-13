/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// snd_local.h -- private sound definations


#ifndef SND_LOCAL_H_
#define SND_LOCAL_H_

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"

#define THIRD_PERSON_THRESHOLD_SQ (48.0f*48.0f)

// Interface between Q3 sound "api" and the sound backend
typedef struct
{
	void ( *Shutdown )( void );
	void ( *StartSound )( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx );
	void ( *StartLocalSound )( sfxHandle_t sfx, int channelNum );
	void ( *StartBackgroundTrack )( const char *intro, const char *loop );
	void ( *StopBackgroundTrack )( void );
	void ( *RawSamples )( int stream, int samples, int rate, int width, int channels, const byte *data, float volume, int entityNum );
	void ( *StopAllSounds )( void );
	void ( *ClearLoopingSounds )( qboolean killall );
	void ( *AddLoopingSound )( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
	void ( *AddRealLoopingSound )( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
	void ( *StopLoopingSound )( int entityNum );
	void ( *Respatialize )( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );
	void ( *UpdateEntityPosition )( int entityNum, const vec3_t origin );
	void ( *UpdateEntityVelocity )( int entityNum, const vec3_t velocity );
	void ( *UpdateEntityOcclusion )( int entityNum, qboolean occluded, float ratio );
	void ( *Update )( void );
	void ( *DisableSounds )( void );
	void ( *BeginRegistration )( void );
	sfxHandle_t ( *RegisterSound )( const char *sample, qboolean compressed );
	void ( *ClearSoundBuffer )( void );
	void ( *SoundInfo )( void );
	void ( *SoundList )( void );
} soundInterface_t;

//====================================================================

#define MAX_RAW_STREAMS (MAX_CLIENTS * 2 + 1)

extern cvar_t *s_volume;
extern cvar_t *s_musicVolume;
extern cvar_t *s_muted;
extern cvar_t *s_doppler;

// OpenAL stuff
typedef enum
{
    SRCPRI_AMBIENT = 0,	// Ambient sound effects
    SRCPRI_ENTITY,			// Entity sound effects
    SRCPRI_ONESHOT,			// One-shot sounds
    SRCPRI_LOCAL,				// Local sounds
    SRCPRI_STREAM				// Streams (music, cutscenes)
} alSrcPriority_t;

typedef int srcHandle_t;

qboolean S_AL_Init( soundInterface_t *si );

#endif //SND_LOCAL_H_
