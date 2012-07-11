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

/*
 * snd_codec.c
 * Codec manager
 *
 * 2005-08-31
 *  Started
 */

#include "../client/client.h"
#include "snd_codec.h"

static snd_codec_t *codecs;

/*
 * Searching
 */
char *findExtension( const char *fni )
{
	char *fn = ( char * ) fni;
	char *eptr = NULL;

	while ( *fn )
	{
		if ( *fn == '.' )
		{
			eptr = fn;
		}

		fn++;
	}

	return eptr;
}

// This is ugly...
static snd_codec_t *findCodec( const char *filename )
{
	char        *ext = findExtension( filename );
	snd_codec_t *codec = codecs;

	if ( !ext )
	{
		// No extension - auto-detect
		while ( codec )
		{
			char fn[ MAX_QPATH ];
			Q_strncpyz( fn, filename, sizeof( fn ) - 4 );
			COM_DefaultExtension( fn, sizeof( fn ), codec->ext );

			// Check it exists
			if ( FS_ReadFile( fn, NULL ) != -1 )
			{
				return codec;
			}

			// Nope. Next!
			codec = codec->next;
		}

		// Nothin'
		return NULL;
	}

	while ( codec )
	{
		if ( !Q_stricmp( ext, codec->ext ) )
		{
			return codec;
		}

		codec = codec->next;
	}

	return NULL;
}

/*
 * Codec management
 */
void codec_init()
{
	codecs = NULL;
	codec_register( &wav_codec );
#ifdef USE_CODEC_VORBIS
	codec_register( &ogg_codec );
#endif
}

void codec_shutdown()
{
	codecs = NULL;
}

void codec_register( snd_codec_t *codec )
{
	codec->next = codecs;
	codecs = codec;
}

void *codec_load( const char *filename, snd_info_t *info )
{
	snd_codec_t *codec;
	char        fn[ MAX_QPATH ];

	codec = findCodec( filename );

	if ( !codec )
	{
		Com_Printf(_( "Unknown extension for %s\n"), filename );
		return NULL;
	}

	strncpy( fn, filename, sizeof( fn ) );
	COM_DefaultExtension( fn, sizeof( fn ), codec->ext );

	return codec->load( fn, info );
}

snd_stream_t *codec_open( const char *filename )
{
	snd_codec_t *codec;
	char        fn[ MAX_QPATH ];

	codec = findCodec( filename );

	if ( !codec )
	{
		Com_Printf(_( "Unknown extension for %s\n"), filename );
		return NULL;
	}

	strncpy( fn, filename, sizeof( fn ) );
	COM_DefaultExtension( fn, sizeof( fn ), codec->ext );

	return codec->open( fn );
}

void codec_close( snd_stream_t *stream )
{
	stream->codec->close( stream );
}

int codec_read( snd_stream_t *stream, int bytes, void *buffer )
{
	return stream->codec->read( stream, bytes, buffer );
}

/*
 * Util functions (used by codecs)
 */
snd_stream_t *codec_util_open( const char *filename, snd_codec_t *codec )
{
	snd_stream_t *stream;
	fileHandle_t hnd;
	int          length;

	// Try to open the file
	length = FS_FOpenFileRead( filename, &hnd, qtrue );

	if ( !hnd )
	{
		Com_Printf(_( "Can't read sound file %s\n"), filename );
		return NULL;
	}

	// Allocate a stream
	stream = calloc( 1, sizeof( snd_stream_t ) );

	if ( !stream )
	{
		FS_FCloseFile( hnd );
		return NULL;
	}

	// Copy over, return
	stream->codec = codec;
	stream->file = hnd;
	stream->length = length;
	return stream;
}

void codec_util_close( snd_stream_t *stream )
{
	FS_FCloseFile( stream->file );
	free( stream );
}
