/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)
Copyright (C) 2005-2006 Joerg Dietrich <dietrich_joerg@gmx.de>

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

// OGG support is enabled by this define
#if defined (USE_CODEC_VORBIS)

// includes for the Q3 sound system
#include "client.h"
#include "snd_codec.h"

// includes for the OGG codec
#include <errno.h>
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

// The OGG codec can return the samples in a number of different formats,
// we use the standard signed short format.
#define OGG_SAMPLEWIDTH 2

// Q3 OGG codec
snd_codec_t ogg_codec =
{
	".ogg",
	codec_ogg_load,
	codec_ogg_open,
	codec_ogg_read,
	codec_ogg_close,
	NULL
};

// callbacks for vobisfile

// fread() replacement
size_t S_OGG_Callback_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	snd_stream_t *stream;
	int byteSize = 0;
	int bytesRead = 0;
	size_t nMembRead = 0;

	// check if input is valid
	if(!ptr)
	{
		errno = EFAULT; 
		return 0;
	}
	
	if(!(size && nmemb))
	{
		// It's not an error, caller just wants zero bytes!
		errno = 0;
		return 0;
	}
 
	if(!datasource)
	{
		errno = EBADF; 
		return 0;
	}
	
	// we use a snd_stream_t in the generic pointer to pass around
	stream = (snd_stream_t *) datasource;

	// FS_Read does not support multi-byte elements
	byteSize = nmemb * size;

	// read it with the Q3 function FS_Read()
	bytesRead = FS_Read(ptr, byteSize, stream->file);

	// this function returns the number of elements read not the number of bytes
	nMembRead = bytesRead / size;

	// even if the last member is only read partially
	// it is counted as a whole in the return value	
	if(bytesRead % size)
	{
		nMembRead++;
	}
	
	return nMembRead;
}

// fseek() replacement
int S_OGG_Callback_seek(void *datasource, ogg_int64_t offset, int whence)
{
	snd_stream_t *stream;
	int retVal = 0;

	// check if input is valid
	if(!datasource)
	{
		errno = EBADF; 
		return -1;
	}

	// snd_stream_t in the generic pointer
	stream = (snd_stream_t *) datasource;

	// we must map the whence to its Q3 counterpart
	switch(whence)
	{
		case SEEK_SET :
		{
			// set the file position in the actual file with the Q3 function
			retVal = FS_Seek(stream->file, (long) offset, FS_SEEK_SET);

			// something has gone wrong, so we return here
			if(retVal < 0)
			{
			 return retVal;
			}

			return 0;
		}
  
		case SEEK_CUR :
		{
			// set the file position in the actual file with the Q3 function
			retVal = FS_Seek(stream->file, (long) offset, FS_SEEK_CUR);

			// something has gone wrong, so we return here
			if(retVal < 0)
			{
			 return retVal;
			}

			return 0;
		}
 
		case SEEK_END :
		{
			// Quake 3 seems to have trouble with FS_SEEK_END 
			// so we use the file length and FS_SEEK_SET

			// set the file position in the actual file with the Q3 function
			retVal = FS_Seek(stream->file, (long) stream->length + (long) offset, FS_SEEK_SET);

			// something has gone wrong, so we return here
			if(retVal < 0)
			{
			 return retVal;
			}

			return 0;
		}
  
		default :
		{
			// unknown whence, so we return an error
			errno = EINVAL;
			return -1;
		}
	}
}

// fclose() replacement
int S_OGG_Callback_close(void *datasource)
{
	// we do nothing here and close all things manually in S_OGG_CodecCloseStream()
	return 0;
}

// ftell() replacement
long S_OGG_Callback_tell(void *datasource)
{
	snd_stream_t   *stream;

	// check if input is valid
	if(!datasource)
	{
		errno = EBADF; 
		return -1;
	}

	// snd_stream_t in the generic pointer
	stream = (snd_stream_t *) datasource;
	
	return (long) FS_FTell(stream->file);
}

// the callback structure
const ov_callbacks S_OGG_Callbacks =
{
 &S_OGG_Callback_read,
 &S_OGG_Callback_seek,
 &S_OGG_Callback_close,
 &S_OGG_Callback_tell
};

/*
=================
S_OGG_CodecOpenStream
=================
*/
snd_stream_t *codec_ogg_open(const char *filename)
{
	snd_stream_t *stream;

	// OGG codec control structure
	OggVorbis_File *vf;

	// some variables used to get informations about the OGG 
	vorbis_info *OGGInfo;
	ogg_int64_t numSamples;

	// check if input is valid
	if(!filename)
	{
		return NULL;
	}

	// Open the stream
	stream = codec_util_open(filename, &ogg_codec);
	if(!stream)
	{
		return NULL;
	}

	// alloctate the OggVorbis_File
	vf = Z_Malloc(sizeof(OggVorbis_File));
	if(!vf)
	{
		codec_util_close(stream);

		return NULL;
	}

	// open the codec with our callbacks and stream as the generic pointer
	if(ov_open_callbacks(stream, vf, NULL, 0, S_OGG_Callbacks) != 0)
	{
		Z_Free(vf);

		codec_util_close(stream);

		return NULL;
	}

	// the stream must be seekable
	if(!ov_seekable(vf))
	{
		ov_clear(vf);

		Z_Free(vf);

		codec_util_close(stream);

		return NULL;
	}
 
	// we only support OGGs with one substream
	if(ov_streams(vf) != 1)
	{
		ov_clear(vf);

		Z_Free(vf);

		codec_util_close(stream);

		return NULL;  
	}

	// get the info about channels and rate
	OGGInfo = ov_info(vf, 0);
	if(!OGGInfo)
	{
		ov_clear(vf);

		Z_Free(vf);

		codec_util_close(stream);

		return NULL;  
	}

	// get the number of sample-frames in the OGG
	numSamples = ov_pcm_total(vf, 0);

	// fill in the info-structure in the stream
	stream->info.rate = OGGInfo->rate;
	stream->info.width = OGG_SAMPLEWIDTH;
	stream->info.channels = OGGInfo->channels;
	stream->info.samples = numSamples;
	stream->info.size = stream->info.samples * stream->info.channels * stream->info.width;
	stream->info.dataofs = 0;

	// We use the generic pointer in stream for the OGG codec control structure
	stream->ptr = vf;

	return stream;
}

/*
=================
S_OGG_CodecCloseStream
=================
*/
void codec_ogg_close(snd_stream_t *stream)
{
	// check if input is valid
	if(!stream)
	{
		return;
	}
	
	// let the OGG codec cleanup its stuff
	ov_clear((OggVorbis_File *) stream->ptr);

	// free the OGG codec control struct
	Z_Free(stream->ptr);

	// close the stream
	codec_util_close(stream);
}

/*
=================
S_OGG_CodecReadStream
=================
*/
int codec_ogg_read(snd_stream_t *stream, int bytes, void *buffer)
{
	// buffer handling
	int bytesRead, bytesLeft, c;
	char *bufPtr;
	
	// Bitstream for the decoder
	int BS = 0;
	
	// big endian machines want their samples in big endian order
	int IsBigEndian = 0;

#ifdef Q3_BIG_ENDIAN
	IsBigEndian = 1;
#endif // Q3_BIG_ENDIAN	

	// check if input is valid
	if(!(stream && buffer))
	{
		return 0;
	}

	if(bytes <= 0)
	{
		return 0;
	}

	bytesRead = 0;
	bytesLeft = bytes;
	bufPtr = buffer;

	// cycle until we have the requested or all available bytes read
	while(-1)
	{
		// read some bytes from the OGG codec
		c = ov_read((OggVorbis_File *) stream->ptr, bufPtr, bytesLeft, IsBigEndian, OGG_SAMPLEWIDTH, 1, &BS);
		
		// no more bytes are left
		if(c <= 0)
		{
			break;
		}

		bytesRead += c;
		bytesLeft -= c;
		bufPtr += c;
  
		// we have enough bytes
		if(bytesLeft <= 0)
		{
			break;
		}
	}

	return bytesRead;
}

/*
=====================================================================
S_OGG_CodecLoad

We handle S_OGG_CodecLoad as a special case of the streaming functions 
where we read the whole stream at once.
======================================================================
*/
void *codec_ogg_load(const char *filename, snd_info_t *info)
{
	snd_stream_t *stream;
	byte *buffer;
	int bytesRead;
	
	// check if input is valid
	if(!(filename && info))
	{
		return NULL;
	}
	
	// open the file as a stream
	stream = codec_ogg_open(filename);
	if(!stream)
	{
		return NULL;
	}
	
	// copy over the info
	info->rate = stream->info.rate;
	info->width = stream->info.width;
	info->channels = stream->info.channels;
	info->samples = stream->info.samples;
	info->size = stream->info.size;
	info->dataofs = stream->info.dataofs;

	// allocate a buffer
	// this buffer must be free-ed by the caller of this function
	buffer = Hunk_AllocateTempMemory(info->size);
	if(!buffer)
	{
		codec_ogg_close(stream);
	
		return NULL;	
	}

	// fill the buffer
	bytesRead = codec_ogg_read(stream, info->size, buffer);
	
	// we don't even have read a single byte
	if(bytesRead <= 0)
	{
		Hunk_FreeTempMemory(buffer);
		codec_ogg_close(stream);
	
		return NULL;	
	}

	codec_ogg_close(stream);
	
	return buffer;
}

#endif // USE_CODEC_VORBIS
