/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)

This file is part of the OpenWolf GPL Source Code (OpenWolf Source Code).  

OpenWolf Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenWolf Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the OpenWolf Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the OpenWolf 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

/*
 * snd_codec.h
 * Sound codec management
 * It only decodes things, so technically it's not a COdec, but who cares?
 *
 * 2005-08-31
 *  Started
 */

#ifndef _SND_CODEC_H_
#define _SND_CODEC_H_

#include "snd_api.h"
#include "../qcommon/q_shared.h"

// Codec functions
typedef void *(*CODEC_LOAD)(const char *filename, snd_info_t *info);
typedef snd_stream_t *(*CODEC_OPEN)(const char *filename);
typedef int (*CODEC_READ)(snd_stream_t *stream, int bytes, void *buffer);
typedef void (*CODEC_CLOSE)(snd_stream_t *stream);

// Codec data structure
struct snd_codec_s
{
	char *ext;
	CODEC_LOAD load;
	CODEC_OPEN open;
	CODEC_READ read;
	CODEC_CLOSE close;
	snd_codec_t *next;
};

/*
 * Codec management
 */
void codec_init( void );
void codec_shutdown( void );
void codec_register(snd_codec_t *codec);
void *codec_load(const char *filename, snd_info_t *info);
snd_stream_t *codec_open(const char *filename);
void codec_close(snd_stream_t *stream);
int codec_read(snd_stream_t *stream, int bytes, void *buffer);

/*
 * Util functions (used by codecs)
 */
snd_stream_t *codec_util_open(const char *filename, snd_codec_t *codec);
void codec_util_close(snd_stream_t *stream);

/*
 * WAV Codec
 */
extern snd_codec_t wav_codec;
void *codec_wav_load(const char *filename, snd_info_t *info);
snd_stream_t *codec_wav_open(const char *filename);
void codec_wav_close(snd_stream_t *stream);
int codec_wav_read(snd_stream_t *stream, int bytes, void *buffer);

/*
 * Ogg Vorbis codec
 */
#ifdef USE_CODEC_VORBIS
extern snd_codec_t ogg_codec;
void *codec_ogg_load(const char *filename, snd_info_t *info);
snd_stream_t *codec_ogg_open(const char *filename);
void codec_ogg_close(snd_stream_t *stream);
int codec_ogg_read(snd_stream_t *stream, int bytes, void *buffer);
#endif // USE_CODEC_VORBIS

#endif // !_SND_CODEC_H_