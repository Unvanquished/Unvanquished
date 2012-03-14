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
 * snd_al_music.c
 * Music playback for the OpenAL sound system
 *
 * 2005-08-23
 *  Started implementing music system
 *
 * 2005-08-24
 *  Finished implementing music system
 *  Added Ogg Vorbis support
 *
 * 2005-08-2?
 *  Bugfixes
 *  Better autodetection of music type
 *
 * 2005-08-28
 *  Changed to dynamic channel allocation
 *  Increased buffer count, size
 *
 * 2005-08-31
 *  Removed all decoders
 *  Reworked to use codec system
 *  Pre-buffer music (fixes stuttering on Windows)
 */

#include "snd_al_local.h"

#define BUFFERS     4
#define BUFFER_SIZE 4096

static qboolean     mus_playing   = qfalse;
static srcHandle_t  source_handle = -1;
static ALuint       source;
static ALuint       buffers[ BUFFERS ];

static snd_stream_t *mus_stream;
static char         s_backgroundLoop[ MAX_QPATH ];

static byte         decode_buffer[ BUFFER_SIZE ];

/**
 * Source aquire / release
 */
static void al_mus_source_get( void )
{
	// Allocate a source at high priority
	source_handle = al_src_alloc( SRCPRI_STREAM, -2, 0 );

	if ( source_handle == -1 )
	{
		return;
	}

	// Lock the source so nobody else can use it, and get the raw source
	al_src_lock( source_handle );
	source = al_src_get( source_handle );

	// Set some source parameters
	qalSource3f( source, AL_POSITION,        0.0, 0.0, 0.0 );
	qalSource3f( source, AL_VELOCITY,        0.0, 0.0, 0.0 );
	qalSource3f( source, AL_DIRECTION,       0.0, 0.0, 0.0 );
	qalSourcef ( source, AL_ROLLOFF_FACTOR,  0.0          );
	qalSourcei ( source, AL_SOURCE_RELATIVE, AL_TRUE      );
}

static void al_mus_source_free( void )
{
	// Release the output source
	al_src_unlock( source_handle );
	source        = 0;
	source_handle = -1;
}

void al_mus_process( ALuint b )
{
	int    l;
	ALuint format;

	l = si.StreamRead( mus_stream, BUFFER_SIZE, decode_buffer );

	if ( l == 0 )
	{
		si.StreamClose( mus_stream );
		mus_stream = si.StreamOpen( s_backgroundLoop );

		if ( !mus_stream )
		{
			SndAl_StopBackgroundTrack();
			return;
		}

		l = si.StreamRead( mus_stream, BUFFER_SIZE, decode_buffer );
	}

	format = al_format( mus_stream->info.width, mus_stream->info.channels );
	qalBufferData( b, format, decode_buffer, l, mus_stream->info.rate );
}

/**
 * Background music playback
 */
void SndAl_StartBackgroundTrack( const char *intro, const char *loop )
{
	int i;

	// Stop any existing music that might be playing
	SndAl_StopBackgroundTrack();

	if ( !intro || !intro[ 0 ] )
	{
		intro = loop;
	}

	if ( !loop || !loop[ 0 ] )
	{
		loop = intro;
	}

	if ( ( !intro || !intro[ 0 ] ) && ( !intro || !intro[ 0 ] ) )
	{
		return;
	}

	// Copy the loop over
	strncpy( s_backgroundLoop, loop, sizeof( s_backgroundLoop ) );

	// Open the intro
	mus_stream = si.StreamOpen( intro );

	if ( !mus_stream )
	{
		return;
	}

	// Allocate a source
	al_mus_source_get();

	if ( source_handle == -1 )
	{
		return;
	}

	// Generate the buffers
	qalGenBuffers( BUFFERS, buffers );

	// Queue the buffers up
	for ( i = 0; i < BUFFERS; i++ )
	{
		al_mus_process( buffers[ i ] );
	}

	qalSourceQueueBuffers( source, BUFFERS, buffers );

	// Start playing
	qalSourcePlay( source );

	mus_playing = qtrue;
}

void SndAl_StopBackgroundTrack( void )
{
	if ( !mus_playing )
	{
		return;
	}

	// Stop playing
	qalSourceStop( source );

	// De-queue the buffers
	qalSourceUnqueueBuffers( source, BUFFERS, buffers );

	// Destroy the buffers
	qalDeleteBuffers( BUFFERS, buffers );

	// Free the source
	al_mus_source_free();

	// Unload the stream
	if ( mus_stream )
	{
		si.StreamClose( mus_stream );
	}

	mus_stream  = NULL;

	mus_playing = qfalse;
}

void al_mus_update( void )
{
	int   processed;
	ALint state;

	if ( !mus_playing )
	{
		return;
	}

	qalGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );

	if ( processed )
	{
		while ( processed-- )
		{
			ALuint b;
			qalSourceUnqueueBuffers( source, 1, &b );
			al_mus_process( b );
			qalSourceQueueBuffers( source, 1, &b );
		}
	}

	// If it's not still playing, give it a kick
	qalGetSourcei( source, AL_SOURCE_STATE, &state );

	if ( state == AL_STOPPED )
	{
		si.Printf( PRINT_ALL, "Musical kick\n" );
		qalSourcePlay( source );
	}

	// Set the gain property
	qalSourcef( source, AL_GAIN, s_gain->value * s_musicVolume->value );
}
