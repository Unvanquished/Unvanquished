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
 * snd_al_stream.c
 * Stream playing and management
 * This has a lot in common with music - they should be integrated a bit
 *
 * 2005-08-28
 *  Initial implementation (latency problem?)
 *  Added volume control - don't know if I should multiply by s_volume too
 *
 * 2005-08-30
 *  Multiply volume by s_volume
 */

#include "snd_al_local.h"

static srcHandle_t source_handle = -1;
static qboolean    is_playing = qfalse;
static ALuint      source;

static void allocate_channel()
{
	// Allocate a source at high priority
	source_handle = al_src_alloc( SRCPRI_STREAM, -2, 0 );

	if( source_handle == -1 )
	{
		return;
	}

	// Lock the source so nobody else can use it, and get the raw source
	al_src_lock( source_handle );
	source = al_src_get( source_handle );

	// Set some source parameters
	qalSourcei( source, AL_BUFFER,          0 );
	qalSourcei( source, AL_LOOPING,         AL_FALSE );
	qalSource3f( source, AL_POSITION,        0.0, 0.0, 0.0 );
	qalSource3f( source, AL_VELOCITY,        0.0, 0.0, 0.0 );
	qalSource3f( source, AL_DIRECTION,       0.0, 0.0, 0.0 );
	qalSourcef( source, AL_ROLLOFF_FACTOR,  0.0 );
	qalSourcei( source, AL_SOURCE_RELATIVE, AL_TRUE );
}

static void free_channel()
{
	// Release the output source
	al_src_unlock( source_handle );
	source = 0;
	source_handle = -1;
}

void SndAl_RawSamples( int stream, int samples, int rate, int width, int channels, const byte *data, float volume, int entityNum )
{
	ALuint buffer;
	ALuint format = AL_FORMAT_STEREO16;
	ALint  state;

	// Work out AL format
	if( width == 1 )
	{
		if( channels == 1 )
		{
			format = AL_FORMAT_MONO8;
		}
		else if( channels == 2 )
		{
			format = AL_FORMAT_STEREO8;
		}
	}
	else if( width == 2 )
	{
		if( channels == 1 )
		{
			format = AL_FORMAT_MONO16;
		}
		else if( channels == 2 )
		{
			format = AL_FORMAT_STEREO16;
		}
	}

	// Create the source if necessary
	if( source_handle == -1 )
	{
		allocate_channel();

		// Failed?
		if( source_handle == -1 )
		{
			si.Printf( PRINT_ALL, "Can't allocate streaming source\n" );
			return;
		}
	}

	// Create a buffer, and stuff the data into it
	qalGenBuffers( 1, &buffer );
	qalBufferData( buffer, format, data, ( samples * width * channels ), rate );

	// Shove the data onto the source
	qalSourceQueueBuffers( source, 1, &buffer );

	// Start the source playing if necessary
	qalGetSourcei( source, AL_SOURCE_STATE, &state );

	// Volume
	qalSourcef( source, AL_GAIN, volume * s_volume->value * s_gain->value );

	if( !is_playing )
	{
		qalSourcePlay( source );
		is_playing = qtrue;
	}
}

void al_stream_update()
{
	int   processed;
	ALint state;

	if( source_handle == -1 )
	{
		return;
	}

	// Un-queue any buffers, and delete them
	qalGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );

	if( processed )
	{
		while( processed-- )
		{
			ALuint buffer;
			qalSourceUnqueueBuffers( source, 1, &buffer );
			qalDeleteBuffers( 1, &buffer );
		}
	}

	// If it's stopped, release the source
	qalGetSourcei( source, AL_SOURCE_STATE, &state );

	if( state == AL_STOPPED )
	{
		is_playing = qfalse;
		qalSourceStop( source );
		free_channel();
	}
}

void al_stream_die()
{
	if( source_handle == -1 )
	{
		return;
	}

	is_playing = qfalse;
	qalSourceStop( source );
	free_channel();
}
