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
 * snd_al_buffers.c
 * Buffer allocation and management for the OpenAL sound system
 *
 * 2005-08-22
 *  Started logging
 *  Split buffer management out from snd_al.c
 *
 * 2005-08-23
 *  Reworked to use sfxHandles instead of re-cast pointers. What was I thinking?
 *  Added error checking
 *  Started on sound unloader
 *
 * 2005-08-2?
 *  Couple of bugfixes
 *  Added proper deletion of buffers on shutdown
 */

#include "snd_al_local.h"

typedef struct sfx_s sfx_t;
struct sfx_s
{
	char       filename[ MAX_QPATH ];
	ALuint     buffer;              // OpenAL buffer
	qboolean   isDefault;           // Couldn't be loaded - use default FX
	qboolean   inMemory;            // Sound is stored in memory
	qboolean   isLocked;            // Sound is locked (can not be unloaded)
	int        used;                // Time last used
	sfx_t      *next;               // Next entry in hash list
	int        duration;
	snd_info_t info;                // information for this sound like rate, sample count..
};

static qboolean al_buffer_inited = qfalse;

/**
 * Sound effect storage, data structures
 */
#define MAX_SFX 4096
static sfx_t       knownSfx[ MAX_SFX ];
static int         numSfx;

static sfxHandle_t default_sfx;

// Find a free handle
sfxHandle_t al_buf_find_free()
{
	int i;

	for ( i = 0; i < MAX_SFX; i++ )
	{
		// Got one
		if ( knownSfx[ i ].filename[ 0 ] == '\0' )
		{
			return i;
		}
	}

	// Shit...
	si.Error( ERR_FATAL, "al_buf_find_free: No free sound handles" );
	return -1;
}

// Find a sound effect if loaded, set up a handle otherwise
sfxHandle_t al_buf_find( const char *filename )
{
	// Look it up in the hash table
	sfxHandle_t sfx = -1;
	int         i;

	for ( i = 0; i < MAX_SFX; i++ )
	{
		if ( !strcmp( knownSfx[ i ].filename, filename ) )
		{
			sfx = i;
			break;
		}
	}

	// Not found in hash table?
	if ( sfx == -1 )
	{
		sfx_t *ptr;

		sfx = al_buf_find_free();

		// Clear and copy the filename over
		ptr = &knownSfx[ sfx ];
		memset( ptr, 0, sizeof( *ptr ) );
		strcpy( ptr->filename, filename );
	}

	// Return the handle
	return sfx;
}

/**
 * Initialisation and shutdown
 */
// Called at init, shutdown
qboolean al_buf_init( void )
{
	sfxHandle_t default_sfx;

	if ( al_buffer_inited )
	{
		return qtrue;
	}

	// Clear the hash table, and SFX table
	memset( knownSfx, 0, sizeof( knownSfx ) );
	numSfx                           = 0;

	// Load the default sound, and lock it
	default_sfx                      = al_buf_find( "sound/feedback/hit.wav" );
	al_buf_use( default_sfx );
	knownSfx[ default_sfx ].isLocked = qtrue;

	// All done
	al_buffer_inited                 = qtrue;
	return qtrue;
}

void al_buf_shutdown( void )
{
	int i;

	if ( !al_buffer_inited )
	{
		return;
	}

	// Unlock the default sound effect
	knownSfx[ default_sfx ].isLocked = qfalse;

	// Free all used effects
	for ( i = 0; i < MAX_SFX; i++ )
	{
		al_buf_unload( i );
	}

	// Clear the tables
	memset( knownSfx, 0, sizeof( knownSfx ) );

	// All undone
	al_buffer_inited = qfalse;
}

/**
 * Registration
 */
sfxHandle_t SndAl_RegisterSound( const char *sample, qboolean compressed )
{
	sfxHandle_t sfx = al_buf_find( sample );

	if ( ( s_precache->integer == 1 ) && ( !knownSfx[ sfx ].inMemory ) && ( !knownSfx[ sfx ].isDefault ) )
	{
		al_buf_load( sfx );
	}

	knownSfx[ sfx ].used = si.Milliseconds();

	return sfx;
}

/**
 * Usage counter
 */
void al_buf_use( sfxHandle_t sfx )
{
	if ( knownSfx[ sfx ].filename[ 0 ] == '\0' )
	{
		return;
	}

	if ( ( !knownSfx[ sfx ].inMemory ) && ( !knownSfx[ sfx ].isDefault ) )
	{
		al_buf_load( sfx );
	}

	knownSfx[ sfx ].used = si.Milliseconds();
}

/**
 * Loading and unloading
 */
static void al_buf_use_default( sfxHandle_t sfx )
{
	if ( sfx == default_sfx )
	{
		si.Error( ERR_FATAL, "Can't load default sound effect %s\n", knownSfx[ sfx ].filename );
	}

	si.Printf( PRINT_ALL, "Warning: Using default sound for %s\n", knownSfx[ sfx ].filename );
	knownSfx[ sfx ].isDefault = qtrue;
	knownSfx[ sfx ].buffer    = knownSfx[ default_sfx ].buffer;
}

void al_buf_load( sfxHandle_t sfx )
{
	ALenum     error;

	void       *data;
	snd_info_t info;
	ALuint     format;

	// Nothing?
	if ( knownSfx[ sfx ].filename[ 0 ] == '\0' )
	{
		return;
	}

	// Player SFX
	if ( knownSfx[ sfx ].filename[ 0 ] == '*' )
	{
		return;
	}

	// Already done?
	if ( ( knownSfx[ sfx ].inMemory ) || ( knownSfx[ sfx ].isDefault ) )
	{
		return;
	}

	// Try to load
	data = si.LoadSound( knownSfx[ sfx ].filename, &info );

	if ( !data )
	{
		si.Printf( PRINT_ALL, "Can't load %s\n", knownSfx[ sfx ].filename );
		al_buf_use_default( sfx );
		return;
	}

	format = al_format( info.width, info.channels );

	// Create a buffer
	qalGenBuffers( 1, &knownSfx[ sfx ].buffer );

	if ( ( error = qalGetError() ) != AL_NO_ERROR )
	{
		al_buf_use_default( sfx );
		si.Hunk_FreeTempMemory( data );
		si.Printf( PRINT_ALL, "Can't create a sound buffer for %s - %s\n", knownSfx[ sfx ].filename, al_errormsg( error ) );
		return;
	}

	// Fill the buffer
	qalGetError();
	qalBufferData( knownSfx[ sfx ].buffer, format, data, info.size, info.rate );
	error = qalGetError();

	// If we ran out of memory, start evicting the least recently used sounds
	while ( error == AL_OUT_OF_MEMORY )
	{
		qboolean rv = al_buf_evict();

		if ( !rv )
		{
			al_buf_use_default( sfx );
			si.Hunk_FreeTempMemory( data );
			si.Printf( PRINT_ALL, "Out of memory loading %s\n", knownSfx[ sfx ].filename );
			return;
		}

		// Try load it again
		qalGetError();
		qalBufferData( knownSfx[ sfx ].buffer, format, data, info.size, info.rate );
		error = qalGetError();
	}

	// Some other error condition
	if ( error != AL_NO_ERROR )
	{
		al_buf_use_default( sfx );
		si.Hunk_FreeTempMemory( data );
		si.Printf( PRINT_ALL, "Can't fill sound buffer for %s - %s", knownSfx[ sfx ].filename, al_errormsg( error ) );
		return;
	}

	// Free the memory
	si.Hunk_FreeTempMemory( data );

	// Woo!
	knownSfx[ sfx ].inMemory = qtrue;
}

int al_duration ( sfxHandle_t sfx )
{
	if ( sfx < 0 || sfx >= numSfx )
	{
		si.Printf( PRINT_ALL, "ERROR: S_AL_SoundDuration: handle %i out of range\n", sfx );
		return 0;
	}

	return knownSfx[ sfx ].duration;
}

void al_buf_unload( sfxHandle_t sfx )
{
	ALenum error;

	if ( knownSfx[ sfx ].filename[ 0 ] == '\0' )
	{
		return;
	}

	if ( !knownSfx[ sfx ].inMemory )
	{
		return;
	}

	// Delete it
	qalDeleteBuffers( 1, &knownSfx[ sfx ].buffer );

	if ( ( error = qalGetError() ) != AL_NO_ERROR )
	{
		si.Printf( PRINT_ALL, "Can't delete sound buffer for %s", knownSfx[ sfx ].filename );
	}

	knownSfx[ sfx ].inMemory = qfalse;
}

qboolean al_buf_evict()
{
	// Doesn't work yet, so if OpenAL reports that you're out of memory, you'll just get
	// "Catastrophic sound memory exhaustion". Whoops.
	return qfalse;
}

/**
 * Buffer grabbage
 */
ALuint al_buf_get( sfxHandle_t sfx )
{
	return knownSfx[ sfx ].buffer;
}

char *al_buf_get_name( sfxHandle_t sfx )
{
	return knownSfx[ sfx ].filename;
}

/*
======================
S_GetCurrentSoundTime
Returns how long the sound lasts in milliseconds
======================
*/
int SndAl_GetSoundLength( sfxHandle_t sfxHandle )
{
	if ( sfxHandle < 0 || sfxHandle >= numSfx )
	{
		si.Printf( PRINT_WARNING, "S_StartSound: handle %i out of range\n", sfxHandle );
		return -1;
	}

	return ( int )( ( ( float )knownSfx[ sfxHandle ].info.samples / ( float )knownSfx[ sfxHandle ].info.rate ) * 1000.0f );
}

/*
======================
S_GetCurrentSoundTime
Returns how long the sound lasts in milliseconds
======================
*/
int SndAl_GetCurrentSoundTime( void )
{
	return si.Milliseconds();
}
