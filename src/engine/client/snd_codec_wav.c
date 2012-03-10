/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)

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
 * snd_codec_wav.c
 * RIFF WAVE reader decoder
 *
 * 2005-08-31
 *  Started, based on id's decoder
 *
 * 2005-09-01
 *  Fixed chunk parsing code to deal with unexpected chunks (fixes Padman)
 */

#include "../client/client.h"
#include "snd_codec.h"

/*
 * Wave file reading
 */
static int FGetLittleLong( fileHandle_t f ) {
	int		v;

	FS_Read( &v, sizeof(v), f );

	return LittleLong( v);
}

static int FGetLittleShort( fileHandle_t f ) {
	short	v;

	FS_Read( &v, sizeof(v), f );

	return LittleShort( v);
}

static int readChunkInfo(fileHandle_t f, char *name)
{
	int len, r;

	name[4] = 0;

	r = FS_Read(name, 4, f);
	if(r != 4)
		return 0;

	len = FGetLittleLong(f);
	if(len < 0 || len > 0xffffffff)
		return 0;

	len = (len + 1 ) & ~1;		// pad to word boundary
	return len;
}

static void skipChunk(fileHandle_t f, int length)
{
	byte buffer[32*1024];

	while(length > 0)
	{
		int toread = length;
		if(toread > sizeof(buffer))
			toread = sizeof(buffer);
		FS_Read(buffer, toread, f);
		length -= toread;
	}
}

// returns the length of the data in the chunk, or 0 if not found
static int S_FindWavChunk( fileHandle_t f, char *chunk ) {
	char	name[5];
	int		len;

	// This is a bit dangerous...
	while(1)
	{
		len = readChunkInfo(f, name);

		// Read failure?
		if(len == 0)
			return 0;

		// If this is the right chunk, return
		if( !Q_strncmp( name, chunk, 4 ) )
			return len;

		len = PAD( len, 2 );

		// Not the right chunk - skip it
		FS_Seek( f, len, FS_SEEK_CUR );
	}
}

static void S_ByteSwapRawSamples( int samples, int width, int s_channels, const byte *data ) {
	int		i;

	if ( width != 2 ) {
		return;
	}
	if ( LittleShort( 256 ) == 256 ) {
		return;
	}

	if ( s_channels == 2 ) {
		samples <<= 1;
	}
	for ( i = 0 ; i < samples ; i++ ) {
		((short *)data)[i] = LittleShort( ((short *)data)[i] );
	}
}

static qboolean read_wav_header(fileHandle_t file, snd_info_t *info)
{
	char dump[16];
//	int wav_format;
	int fmtlen = 0;

	// skip the riff wav header
	FS_Read(dump, 12, file);

	// Scan for the format chunk
	if((fmtlen = S_FindWavChunk(file, "fmt ")) == 0)
	{
		Com_Printf("No fmt chunk\n");
		return qfalse;
	}

	// Save the parameters
	/*wav_format = */FGetLittleShort(file);
	info->channels = FGetLittleShort(file);
	info->rate = FGetLittleLong(file);
	FGetLittleLong(file);
	FGetLittleShort(file);
	info->width = FGetLittleShort(file) / 8;
	info->dataofs = 0;

	// Skip the rest of the format chunk if required
	if(fmtlen > 16)
	{
		fmtlen -= 16;
		skipChunk(file, fmtlen);
	}

	// Scan for the data chunk
	if( (info->size = S_FindWavChunk(file, "data")) == 0)
	{
		Com_Printf("No data chunk\n");
		return qfalse;
	}
	info->samples = (info->size / info->width) / info->channels;

	return qtrue;
}

/*
 * WAV codec
 */
snd_codec_t wav_codec =
{
	".wav",
	codec_wav_load,
	codec_wav_open,
	codec_wav_read,
	codec_wav_close,
	NULL
};

void *codec_wav_load(const char *filename, snd_info_t *info)
{
	fileHandle_t file;
	void *buffer;

	// Try to open the file
	FS_FOpenFileRead(filename, &file, qtrue);
	if(!file)
	{
		Com_Printf("Can't read sound file %s\n", filename);
		return NULL;
	}

	// Read the RIFF header
	if(!read_wav_header(file, info))
	{
		FS_FCloseFile(file);
		Com_Printf("Can't understand wav file %s\n", filename);
		return NULL;
	}

	// Allocate some memory
	buffer = Hunk_AllocateTempMemory(info->size);
	if(!buffer)
	{
		FS_FCloseFile(file);
		Com_Printf( S_COLOR_RED "ERROR: Out of memory reading \"%s\"\n", filename);
		return NULL;
	}

	// Read, byteswap
	FS_Read(buffer, info->size, file);
	S_ByteSwapRawSamples(info->samples, info->width, info->channels, (byte *)buffer);

	// Close and return
	FS_FCloseFile(file);
	return buffer;
}

snd_stream_t *codec_wav_open(const char *filename)
{
	snd_stream_t *rv;

	// Open
	rv = codec_util_open(filename, &wav_codec);
	if(!rv)
		return NULL;

	// Read the RIFF header
	if(!read_wav_header(rv->file, &rv->info))
	{
		codec_util_close(rv);
		return NULL;
	}

	return rv;
}

void codec_wav_close(snd_stream_t *stream)
{
	codec_util_close(stream);
}

int codec_wav_read(snd_stream_t *stream, int bytes, void *buffer)
{
	int remaining = stream->info.size - stream->pos;
	int samples;

	if(remaining <= 0)
		return 0;
	if(bytes > remaining)
		bytes = remaining;
	stream->pos += bytes;
	samples = (bytes / stream->info.width) / stream->info.channels;
	FS_Read(buffer, bytes, stream->file);
	S_ByteSwapRawSamples(samples, stream->info.width, stream->info.channels, buffer);
	return bytes;
}
