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
 * snd_al_sources.c
 * Sources allocation and management for the OpenAL sound system
 *
 * 2005-08-23
 *  Started again from scratch
 *  Implemented priority-based channel allocation system
 *  Fixed a lot of bugs
 *  Added dynamic source creation (s_sources)
 *
 * 2005-08-2?
 *  Bugfixes
 *
 * 2005-08-27
 *  Added support for changing cvars
 *
 * 2005-08-28
 *  Fixed weakest channel eviction
 *  Made things shut up correctly
 */

#include "snd_al_local.h"

typedef struct src_s src_t;
struct src_s
{
	ALuint      source; // OpenAL source object
	sfxHandle_t sfx; // Sound effect in use

	int         lastUse; // Last time used
	int         priority; // Priority
	int         entity; // Owning entity (-1 if none)
	int         channel; // Associated channel (-1 if none)

	int         isActive; // Is this source currently in use?
	int         isLocked; // This is locked (un-allocatable)
	int         isLooping; // Is this a looping effect (attached to an entity)
	int         isTracking; // Is this object tracking it's owner

	qboolean    local; // Is this local (relative to the cam)
};

#define MAX_SRC 128
static src_t    srclist[ MAX_SRC ];
static int      src_count = 0;
static qboolean src_inited = qfalse;

static int      ambient_count = 0;

typedef struct sentity_s
{
	vec3_t origin; // Object position

	int    has_sfx; // Associated sound source
	int    sfx;
	int    touched; // Sound present this update?
} sentity_t;
static sentity_t entlist[ MAX_GENTITIES ];

static void      al_src_kill( srcHandle_t src );

/**
 * Startup and shutdown
 */
qboolean al_src_init( void )
{
	int    i;
	int    limit;
	ALenum error;

	// Clear the sources data structure
	memset( srclist, 0, sizeof( srclist ) );
	src_count = 0;

	// Cap s_sources to MAX_SRC
	limit = s_sources->integer;

	if ( limit > MAX_SRC )
	{
		limit = MAX_SRC;
	}
	else if ( limit < 16 )
	{
		limit = 16;
	}

	// Allocate as many sources as possible
	for ( i = 0; i < limit; i++ )
	{
		qalGenSources( 1, &srclist[ i ].source );

		if ( ( error = qalGetError() ) != AL_NO_ERROR )
		{
			break;
		}

		src_count++;
	}

	// All done. Print this for informational purposes
	si.Printf( PRINT_ALL, "allocated %d sources\n", src_count );
	src_inited = qtrue;
	return qtrue;
}

void al_src_shutdown()
{
	int i;

	if ( !src_inited )
	{
		return;
	}

	// Destroy all the sources
	for ( i = 0; i < src_count; i++ )
	{
		if ( srclist[ i ].isLocked )
		{
			si.Printf( PRINT_DEVELOPER, "Warning: Source %d is locked\n", i );
		}

		qalSourceStop( srclist[ i ].source );
		qalDeleteSources( 1, &srclist[ i ].source );
	}

	memset( srclist, 0, sizeof( srclist ) );

	src_inited = qfalse;
}

/**
 * Source setup
 */
static void al_src_setup( srcHandle_t src, sfxHandle_t sfx, int priority, int entity, int channel, qboolean local )
{
	ALuint buffer;
	float  null_vector[] = { 0, 0, 0 };

	// Mark the SFX as used, and grab the raw AL buffer
	al_buf_use( sfx );
	buffer = al_buf_get( sfx );

	// Set up src struct
	srclist[ src ].lastUse = si.Milliseconds();
	srclist[ src ].sfx = sfx;
	srclist[ src ].priority = priority;
	srclist[ src ].entity = entity;
	srclist[ src ].channel = channel;
	srclist[ src ].isActive = qtrue;
	srclist[ src ].isLocked = qfalse;
	srclist[ src ].isLooping = qfalse;
	srclist[ src ].isTracking = qfalse;
	srclist[ src ].local = local;

	// Set up OpenAL source
	qalSourcei( srclist[ src ].source, AL_BUFFER, buffer );
	qalSourcef( srclist[ src ].source, AL_PITCH, 1.0f );
	qalSourcef( srclist[ src ].source, AL_GAIN, s_gain->value * s_volume->value );
	qalSourcefv( srclist[ src ].source, AL_POSITION, null_vector );
	qalSourcefv( srclist[ src ].source, AL_VELOCITY, null_vector );
	qalSourcei( srclist[ src ].source, AL_LOOPING, AL_FALSE );
	qalSourcef( srclist[ src ].source, AL_REFERENCE_DISTANCE, s_minDistance->value );

	if ( local )
	{
		qalSourcei( srclist[ src ].source, AL_SOURCE_RELATIVE, AL_TRUE );
		qalSourcef( srclist[ src ].source, AL_ROLLOFF_FACTOR, 0 );
	}
	else
	{
		qalSourcei( srclist[ src ].source, AL_SOURCE_RELATIVE, AL_FALSE );
		qalSourcef( srclist[ src ].source, AL_ROLLOFF_FACTOR, s_rolloff->value );
	}
}

static void al_src_kill( srcHandle_t src )
{
	// I'm not touching it. Unlock it first.
	if ( srclist[ src ].isLocked )
	{
		return;
	}

	// Stop it if it's playing
	if ( srclist[ src ].isActive )
	{
		qalSourceStop( srclist[ src ].source );
	}

	// Remove the entity association
	if ( ( srclist[ src ].isLooping ) && ( srclist[ src ].entity != -1 ) )
	{
		int ent = srclist[ src ].entity;
		entlist[ ent ].has_sfx = 0;
		entlist[ ent ].sfx = -1;
		entlist[ ent ].touched = qfalse;
	}

	// Remove the buffer
	qalSourcei( srclist[ src ].source, AL_BUFFER, 0 );

	srclist[ src ].sfx = 0;
	srclist[ src ].lastUse = 0;
	srclist[ src ].priority = 0;
	srclist[ src ].entity = -1;
	srclist[ src ].channel = -1;
	srclist[ src ].isActive = qfalse;
	srclist[ src ].isLocked = qfalse;
	srclist[ src ].isLooping = qfalse;
	srclist[ src ].isTracking = qfalse;
}

/**
 * Source allocation
 */
srcHandle_t al_src_alloc( int priority, int entnum, int channel )
{
	int i;
	int empty = -1;
	int weakest = -1;
	int weakest_time = si.Milliseconds();
	int weakest_pri = 999;

	for ( i = 0; i < src_count; i++ )
	{
		// If it's locked, we aren't even going to look at it
		if ( srclist[ i ].isLocked )
		{
			continue;
		}

		// Is it empty or not?
		if ( ( !srclist[ i ].isActive ) && ( empty == -1 ) )
		{
			empty = i;
		}
		else if ( srclist[ i ].priority < priority )
		{
			// If it's older or has lower priority, flag it as weak
			if ( ( srclist[ i ].priority < weakest_pri ) ||
			     ( srclist[ i ].lastUse < weakest_time ) )
			{
				weakest_pri = srclist[ i ].priority;
				weakest_time = srclist[ i ].lastUse;
				weakest = i;
			}
		}

		// Is it an exact match, and not on channel 0?
		if ( ( srclist[ i ].entity == entnum ) && ( srclist[ i ].channel == channel ) && ( channel != 0 ) )
		{
			al_src_kill( i );
			return i;
		}
	}

	// Do we have an empty one?
	if ( empty != -1 )
	{
		return empty;
	}

	// No. How about an overridable one?
	if ( weakest != -1 )
	{
		al_src_kill( weakest );
		return weakest;
	}

	// Nothing. Return failure (cries...)
	return -1;
}

// Finds an active source with matching entity and channel numbers
// Returns -1 if there isn't one
srcHandle_t al_src_find( int entnum, int channel )
{
	int i;

	for ( i = 0; i < src_count; i++ )
	{
		if ( !srclist[ i ].isActive )
		{
			continue;
		}

		if ( ( srclist[ i ].entity == entnum ) && ( srclist[ i ].channel == channel ) )
		{
			return i;
		}
	}

	return -1;
}

// Locks and unlocks a source
// Locked sources will not be automatically reallocated or managed
// Once unlocked, the source may be reallocated again
void al_src_lock( srcHandle_t src )
{
	srclist[ src ].isLocked = qtrue;
}

void al_src_unlock( srcHandle_t src )
{
	srclist[ src ].isLocked = qfalse;
}

// Entity position management
void SndAl_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	if ( entityNum < 0 || entityNum > MAX_GENTITIES )
	{
		si.Error( ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum );
	}

	VectorCopy( origin, entlist[ entityNum ].origin );
}

// Play a local (non-spatialized) sound effect
void SndAl_StartLocalSound( sfxHandle_t sfx, int channel )
{
	// Try to grab a source
	srcHandle_t src = al_src_alloc( SRCPRI_LOCAL, -1, channel );

	if ( src == -1 )
	{
		return;
	}

	// Set up the effect
	al_src_setup( src, sfx, SRCPRI_LOCAL, -1, channel, qtrue );

	// Start it playing
	qalSourcePlay( srclist[ src ].source );
}

// Play a one-shot sound effect
void SndAl_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
	vec3_t      sorigin;

	// Try to grab a source
	srcHandle_t src = al_src_alloc( SRCPRI_ONESHOT, entnum, entchannel );

	if ( src == -1 )
	{
		return;
	}

	// Set up the effect
	al_src_setup( src, sfx, SRCPRI_ONESHOT, entnum, entchannel, qfalse );

	if ( origin == NULL )
	{
		srclist[ src ].isTracking = qtrue;
		VectorScale( entlist[ entnum ].origin, POSITION_SCALE, sorigin );
	}
	else
	{
		VectorScale( origin, POSITION_SCALE, sorigin );
	}

	qalSourcefv( srclist[ src ].source, AL_POSITION, sorigin );

	// Start it playing
	qalSourcePlay( srclist[ src ].source );
}

/*
=================
S_AL_SoundDuration
=================
*/
int SndAl_SoundDuration( sfxHandle_t sfx )
{
	return al_duration( sfx );
}

// Start a looping sound effect
void SndAl_ClearLoopingSounds( qboolean killall )
{
	int i;

	for ( i = 0; i < src_count; i++ )
	{
		if ( ( srclist[ i ].isLooping ) && ( srclist[ i ].entity != -1 ) )
		{
			entlist[ srclist[ i ].entity ].touched = qfalse;
		}
	}
}

void al_src_loop( int priority, sfxHandle_t sfx, const vec3_t origin, const vec3_t velocity, int entnum )
{
	int      src;
	qboolean need_to_play = qfalse;
	vec3_t   sorigin;

	// Do we need to start a new sound playing?
	if ( !entlist[ entnum ].has_sfx )
	{
		// Try to get a channel
		ambient_count++;
		src = al_src_alloc( priority, entnum, -1 );

		if ( src == -1 )
		{
			return;
		}

		need_to_play = qtrue;
	}
	else if ( srclist[ entlist[ entnum ].sfx ].sfx != sfx )
	{
		// Need to restart. Just re-use this channel
		src = entlist[ entnum ].sfx;
		al_src_kill( src );
		need_to_play = qtrue;
	}
	else
	{
		src = entlist[ entnum ].sfx;
	}

	if ( need_to_play )
	{
		// Set up the effect
		al_src_setup( src, sfx, priority, entnum, -1, qfalse );
		qalSourcei( srclist[ src ].source, AL_LOOPING, AL_TRUE );
		srclist[ src ].isLooping = qtrue;

		// Set up the entity
		entlist[ entnum ].has_sfx = qtrue;
		entlist[ entnum ].sfx = src;
		need_to_play = qtrue;
	}

	// Set up the position and velocity
	VectorScale( entlist[ entnum ].origin, POSITION_SCALE, sorigin );
	qalSourcefv( srclist[ src ].source, AL_POSITION, sorigin );
	qalSourcefv( srclist[ src ].source, AL_VELOCITY, velocity );

	// Flag it
	entlist[ entnum ].touched = qtrue;

	// Play if need be
	if ( need_to_play )
	{
		qalSourcePlay( srclist[ src ].source );
	}
}

void SndAl_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
	al_src_loop( SRCPRI_AMBIENT, sfx, origin, velocity, entityNum );
}

void SndAl_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
	al_src_loop( SRCPRI_ENTITY, sfx, origin, velocity, entityNum );
}

void SndAl_StopLoopingSound( int entityNum )
{
	if ( entlist[ entityNum ].has_sfx )
	{
		al_src_kill( entlist[ entityNum ].sfx );
	}
}

// Update state (move things around, manage sources, and so on)
void al_src_update()
{
	int   i;
	int   ent;
	ALint state;

	for ( i = 0; i < src_count; i++ )
	{
		if ( srclist[ i ].isLocked )
		{
			continue;
		}

		if ( !srclist[ i ].isActive )
		{
			continue;
		}

		// Check if it's done, and flag it
		qalGetSourcei( srclist[ i ].source, AL_SOURCE_STATE, &state );

		if ( state == AL_STOPPED )
		{
			al_src_kill( i );
			continue;
		}

		// Update source parameters
		if ( ( s_gain->modified ) || ( s_volume->modified ) )
		{
			qalSourcef( srclist[ i ].source, AL_GAIN, s_gain->value * s_volume->value );
		}

		if ( ( s_rolloff->modified ) && ( !srclist[ i ].local ) )
		{
			qalSourcef( srclist[ i ].source, AL_ROLLOFF_FACTOR, s_rolloff->value );
		}

		if ( s_minDistance->modified )
		{
			qalSourcef( srclist[ i ].source, AL_REFERENCE_DISTANCE, s_minDistance->value );
		}

		ent = srclist[ i ].entity;

		// If a looping effect hasn't been touched this frame, kill it
		if ( srclist[ i ].isLooping )
		{
			if ( !entlist[ ent ].touched )
			{
				ambient_count--;
				al_src_kill( i );
			}

			continue;
		}

		// See if it needs to be moved
		if ( srclist[ i ].isTracking )
		{
			vec3_t sorigin;
			VectorScale( entlist[ ent ].origin, POSITION_SCALE, sorigin );
			qalSourcefv( srclist[ i ].source, AL_POSITION, sorigin );
		}
	}
}

void al_src_shutup()
{
	int i;

	for ( i = 0; i < src_count; i++ )
	{
		al_src_kill( i );
	}
}

ALuint al_src_get( srcHandle_t src )
{
	return srclist[ src ].source;
}

/*
======================
S_GetVoiceAmplitude
======================
*/
int SndAl_GetVoiceAmplitude( int entnum )
{
	return 0;
}
