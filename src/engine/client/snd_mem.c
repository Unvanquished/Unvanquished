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

// snd_mem.c - sound caching

#include "snd_local.h"
#include "snd_codec.h"

#define DEF_COMSOUNDMEGS "8"

/*
===============================================================================

memory management

===============================================================================
*/

static  sndBuffer *buffer = NULL;
static  sndBuffer *freelist = NULL;
static  int       inUse = 0;
static  int       totalInUse = 0;

short             *sfxScratchBuffer = NULL;
sfx_t             *sfxScratchPointer = NULL;
int               sfxScratchIndex = 0;

void    SND_free( sndBuffer *v )
{
	* ( sndBuffer ** ) v = freelist;
	freelist = ( sndBuffer * ) v;
	inUse += sizeof( sndBuffer );
}

sndBuffer      *SND_malloc( void )
{
	sndBuffer *v;
redo:

	if ( freelist == NULL )
	{
		S_FreeOldestSound();
		goto redo;
	}

	inUse -= sizeof( sndBuffer );
	totalInUse += sizeof( sndBuffer );

	v = freelist;
	freelist = * ( sndBuffer ** ) freelist;
	v->next = NULL;
	return v;
}

void SND_setup( void )
{
	sndBuffer *p, *q;
	cvar_t    *cv;
	int       scs;

	cv = Cvar_Get( "com_soundMegs", DEF_COMSOUNDMEGS, CVAR_LATCH | CVAR_ARCHIVE );

	scs = ( cv->integer * 1536 );

	buffer = malloc( scs * sizeof( sndBuffer ) );
	// allocate the stack based hunk allocator
	sfxScratchBuffer = malloc( SND_CHUNK_SIZE * sizeof( short ) * 4 );   //Hunk_Alloc(SND_CHUNK_SIZE * sizeof(short) * 4);
	sfxScratchPointer = NULL;

	inUse = scs * sizeof( sndBuffer );
	p = buffer;
	q = p + scs;

	while ( --q > p )
	{
		* ( sndBuffer ** ) q = q - 1;
	}

	* ( sndBuffer ** ) q = NULL;
	freelist = p + scs - 1;

	Com_Printf("%s", _( "Sound memory manager started\n" ));
}

/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
static void ResampleSfx( sfx_t *sfx, int inrate, int inwidth, byte *data, qboolean compressed )
{
	int       outcount;
	int       srcsample;
	float     stepscale;
	int       i;
	int       sample, samplefrac, fracstep;
	int       part;
	sndBuffer *chunk;

	stepscale = ( float ) inrate / dma.speed; // this is usually 0.5, 1, or 2

	outcount = sfx->soundLength / stepscale;
	sfx->soundLength = outcount;

	samplefrac = 0;
	fracstep = stepscale * 256;
	chunk = sfx->soundData;

	for ( i = 0; i < outcount; i++ )
	{
		srcsample = samplefrac >> 8;
		samplefrac += fracstep;

		if ( inwidth == 2 )
		{
			sample = LittleShort( ( ( short * ) data ) [ srcsample ] );
		}
		else
		{
			sample = ( int )( ( unsigned char )( data[ srcsample ] ) - 128 ) << 8;
		}

		part = ( i & ( SND_CHUNK_SIZE - 1 ) );

		if ( part == 0 )
		{
			sndBuffer *newchunk;
			newchunk = SND_malloc();

			if ( chunk == NULL )
			{
				sfx->soundData = newchunk;
			}
			else
			{
				chunk->next = newchunk;
			}

			chunk = newchunk;
		}

		chunk->sndChunk[ part ] = sample;
	}
}

/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
static int ResampleSfxRaw( short *sfx, int inrate, int inwidth, int samples, byte *data )
{
	int   outcount;
	int   srcsample;
	float stepscale;
	int   i;
	int   sample, samplefrac, fracstep;

	stepscale = ( float ) inrate / dma.speed; // this is usually 0.5, 1, or 2

	outcount = samples / stepscale;

	samplefrac = 0;
	fracstep = stepscale * 256;

	for ( i = 0; i < outcount; i++ )
	{
		srcsample = samplefrac >> 8;
		samplefrac += fracstep;

		if ( inwidth == 2 )
		{
			sample = LittleShort( ( ( short * ) data ) [ srcsample ] );
		}
		else
		{
			sample = ( int )( ( unsigned char )( data[ srcsample ] ) - 128 ) << 8;
		}

		sfx[ i ] = sample;
	}

	return outcount;
}

//=============================================================================

/*
==============
S_LoadSound

The filename may be different than sfx->name in the case
of a forced fallback of a player specific sound
==============
*/
qboolean S_LoadSound( sfx_t *sfx )
{
	byte       *data;
	short      *samples;
	snd_info_t info;
	//int   size;

	// player specific sounds are never directly loaded
	if ( sfx->soundName[ 0 ] == '*' )
	{
		return qfalse;
	}

	// load it in
	data = codec_load( sfx->soundName, &info );

	if ( !data )
	{
		return qfalse;
	}

	if ( info.width == 1 )
	{
		Com_DPrintf( S_WARNING "%s is an 8-bit audio file\n", sfx->soundName );
	}

	if ( info.rate != 22050 )
	{
		Com_DPrintf( S_WARNING "%s is not a 22kHz audio file\n", sfx->soundName );
	}

	samples = Hunk_AllocateTempMemory( info.samples * sizeof( short ) * 2 );

	sfx->lastTimeUsed = Com_Milliseconds() + 1;

	// each of these compression schemes works just fine
	// but the 16bit quality is much nicer and with a local
	// install assured we can rely upon the sound memory
	// manager to do the right thing for us and page
	// sound in as needed

	if ( sfx->soundCompressed == qtrue )
	{
		sfx->soundCompressionMethod = 1;
		sfx->soundData = NULL;
		sfx->soundLength = ResampleSfxRaw( samples, info.rate, info.width, info.samples, ( data + info.dataofs ) );
		S_AdpcmEncodeSound( sfx, samples );
#if 0
	}
	else if ( info.samples > ( SND_CHUNK_SIZE * 16 ) && info.width > 1 )
	{
		sfx->soundCompressionMethod = 3;
		sfx->soundData = NULL;
		sfx->soundLength = ResampleSfxRaw( samples, info.rate, info.width, info.samples, ( data + info.dataofs ) );
		encodeMuLaw( sfx, samples );
	}
	else if ( info.samples > ( SND_CHUNK_SIZE * 6400 ) && info.width > 1 )
	{
		sfx->soundCompressionMethod = 2;
		sfx->soundData = NULL;
		sfx->soundLength = ResampleSfxRaw( samples, info.rate, info.width, info.samples, ( data + info.dataofs ) );
		encodeWavelet( sfx, samples );
#endif
	}
	else
	{
		sfx->soundCompressionMethod = 0;
		sfx->soundLength = info.samples;
		sfx->soundData = NULL;
		ResampleSfx( sfx, info.rate, info.width, data + info.dataofs, qfalse );
	}

	Hunk_FreeTempMemory( samples );
	Hunk_FreeTempMemory( data );

	return qtrue;
}

void S_DisplayFreeMemory( void )
{
	Com_Printf( "%d bytes free sound buffer memory, %d total used\n", inUse, totalInUse );
}
