/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2008 Stefan Langer <raute@users.sourceforge.net>

This file is part of the Daemon GPL Source Code (Daemon Source Code?).

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

  This is a decoder for OGM, a "better" (smaller files, higher resolutions) cinematic format than ROQ

  In this code "ogm" is only: ogg wrapper, vorbis audio, theora video
  (ogm(Ogg Media) in general is ogg wrapper with all kind of audio/video/subtitle/...)

... infos used for this src:
ogg/vobis:
 * decoder_example.c (libvorbis src)
 * libogg Documentation ( http://www.xiph.org/ogg/doc/libogg/ )
 * VLC ogg demux ( http://trac.videolan.org/vlc/browser/trunk/modules/demux/ogg.c )
theora:
 * theora doxygen docs (1.0beta1)
*/

#ifndef BUILD_TTY_CLIENT

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include <theora/theora.h>

#include "client.h"

static const int OGG_BUFFER_SIZE = 8 * 1024; //4096

struct cin_ogm_t
{
	fileHandle_t     ogmFile;

	ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
	//ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
	ogg_stream_state os_audio;
	ogg_stream_state os_video;

	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
	vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
	vorbis_comment   vc; /* struct that stores all the bitstream user comments */

	bool         videoStreamIsTheora;

	theora_info      th_info; // dump_video.c(example decoder): ti
	theora_comment   th_comment; // dump_video.c(example decoder): tc
	theora_state     th_state; // dump_video.c(example decoder): td

	yuv_buffer       th_yuvbuffer;

	unsigned char *outputBuffer;
	unsigned      outputWidht;
	unsigned      outputHeight;
	unsigned      outputBufferSize; // in Pixel (so "real Bytesize" = outputBufferSize*4)
	int           VFrameCount; // output video-stream
	ogg_int64_t   Vtime_unit;
	int           currentTime; // input from Run-function
};

static cin_ogm_t g_ogm;

int              nextNeededVFrame();

/* ####################### #######################

  OGG/OGM
  ... also calls to vorbis/theora-libs

*/

/*
  loadBlockToSync

  return:
  !0 -> no data transferred
*/
static int loadBlockToSync()
{
	int  r = -1;
	char *buffer;
	int  bytes;

	if ( g_ogm.ogmFile )
	{
		buffer = ogg_sync_buffer( &g_ogm.oy, OGG_BUFFER_SIZE );
		bytes = FS_Read( buffer, OGG_BUFFER_SIZE, g_ogm.ogmFile );
		ogg_sync_wrote( &g_ogm.oy, bytes );

		r = ( bytes == 0 );
	}

	return r;
}

/*
  loadPagesToStreams

  return:
  !0 -> no data transferred (or not for all Streams)
*/
static int loadPagesToStreams()
{
	int              r = -1;
	int              AudioPages = 0;
	int              VideoPages = 0;
	ogg_stream_state *osptr = nullptr;
	ogg_page         og;

	while ( !AudioPages || !VideoPages )
	{
		if ( ogg_sync_pageout( &g_ogm.oy, &og ) != 1 )
		{
			break;
		}

		if ( g_ogm.os_audio.serialno == ogg_page_serialno( &og ) )
		{
			osptr = &g_ogm.os_audio;
			++AudioPages;
		}

		if ( g_ogm.os_video.serialno == ogg_page_serialno( &og ) )
		{
			osptr = &g_ogm.os_video;
			++VideoPages;
		}

		if ( osptr != nullptr )
		{
			ogg_stream_pagein( osptr, &og );
		}
	}

	if ( AudioPages && VideoPages )
	{
		r = 0;
	}

	return r;
}

static const int SIZEOF_RAWBUFF = 4 * 1024;

static const int MIN_AUDIO_PRELOAD = 400; // in ms
static const int MAX_AUDIO_PRELOAD = 500; // in ms

/*

  return: audio wants more packets
*/
static bool loadAudio()
{
	bool     anyDataTransferred = true;
	float        **pcm;
	float        *right, *left;
	int          samples, samplesNeeded;
	int          i;
    short        *rawBuffer = new short[SIZEOF_RAWBUFF/sizeof(short)];
	short        *ptr;
	ogg_packet   op;
	vorbis_block vb;

	memset( &op, 0, sizeof( op ) );
	memset( &vb, 0, sizeof( vb ) );
	vorbis_block_init( &g_ogm.vd, &vb );

	while ( anyDataTransferred && g_ogm.currentTime + MAX_AUDIO_PRELOAD > ( int )( g_ogm.vd.granulepos * 1000 / g_ogm.vi.rate ) )
	{
		anyDataTransferred = false;

		if ( ( samples = vorbis_synthesis_pcmout( &g_ogm.vd, &pcm ) ) > 0 )
		{
			// vorbis -> raw
            ptr = rawBuffer;
			samplesNeeded = ( SIZEOF_RAWBUFF ) / ( 2 * 2 ); // (width*channel)

			if ( samples < samplesNeeded )
			{
				samplesNeeded = samples;
			}

			left = pcm[ 0 ];
			right = ( g_ogm.vi.channels > 1 ) ? pcm[ 1 ] : pcm[ 0 ];

			for ( i = 0; i < samplesNeeded; ++i )
			{
				ptr[ 0 ] = ( left[ i ] >= -1.0f &&
				             left[ i ] <= 1.0f ) ? left[ i ] * 32767.f : 32767 * ( ( left[ i ] > 0.0f ) - ( left[ i ] < 0.0f ) );
				ptr[ 1 ] = ( right[ i ] >= -1.0f &&
				             right[ i ] <= 1.0f ) ? right[ i ] * 32767.f : 32767 * ( ( right[ i ] > 0.0f ) - ( right[ i ] < 0.0f ) );
				ptr += 2; //numChans;
			}

			if ( i > 0 )
			{
				// tell libvorbis how many samples we actually consumed
				vorbis_synthesis_read( &g_ogm.vd, i );

				Audio::StreamData( 0, rawBuffer, i, g_ogm.vi.rate, 2, 2, 1.0f, 1);

				anyDataTransferred = true;
			}
		}

		if ( !anyDataTransferred )
		{
			// op -> vorbis
			if ( ogg_stream_packetout( &g_ogm.os_audio, &op ) )
			{
				if ( vorbis_synthesis( &vb, &op ) == 0 )
				{
					vorbis_synthesis_blockin( &g_ogm.vd, &vb );
				}

				anyDataTransferred = true;
			}
		}
	}

	vorbis_block_clear( &vb );

	if ( g_ogm.currentTime + MIN_AUDIO_PRELOAD > ( int )( g_ogm.vd.granulepos * 1000 / g_ogm.vi.rate ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*

  return: 1 -> loaded a new Frame ( g_ogm.outputBuffer points to the actual frame )
                        0 -> no new Frame
                        <0  -> error
*/

/*
how many >> are needed to make y==x (shifting y>>i)
return: -1  -> no match
                >=0 -> number of shifts
*/
static int findSizeShift( int x, int y )
{
	int i;

	for ( i = 0; ( y >> i ); ++i )
	{
		if ( x == ( y >> i ) )
		{
			return i;
		}
	}

	return -1;
}

static int loadVideoFrameTheora()
{
	int        r = 0;
	ogg_packet op;

	memset( &op, 0, sizeof( op ) );

	while ( !r && ( ogg_stream_packetout( &g_ogm.os_video, &op ) ) )
	{
		ogg_int64_t th_frame;

		theora_decode_packetin( &g_ogm.th_state, &op );

		th_frame = theora_granule_frame( &g_ogm.th_state, g_ogm.th_state.granulepos );

		if ( ( g_ogm.VFrameCount < th_frame && th_frame >= nextNeededVFrame() ) || !g_ogm.outputBuffer )
		{
			int yWShift, uvWShift;
			int yHShift, uvHShift;

			if ( theora_decode_YUVout( &g_ogm.th_state, &g_ogm.th_yuvbuffer ) )
			{
				continue;
			}

			if ( g_ogm.outputWidht != g_ogm.th_info.width || g_ogm.outputHeight != g_ogm.th_info.height )
			{
				g_ogm.outputWidht = g_ogm.th_info.width;
				g_ogm.outputHeight = g_ogm.th_info.height;
				Log::Debug( "[Theora(ogg)]new resolution %dx%d", g_ogm.outputWidht, g_ogm.outputHeight );
			}

			if ( g_ogm.outputBufferSize < g_ogm.th_info.width * g_ogm.th_info.height )
			{
				g_ogm.outputBufferSize = g_ogm.th_info.width * g_ogm.th_info.height;

				/* Free old output buffer */
				if ( g_ogm.outputBuffer )
				{
					free( g_ogm.outputBuffer );
				}

				/* Allocate the new buffer */
				g_ogm.outputBuffer = ( unsigned char * ) malloc( g_ogm.outputBufferSize * 4 );

				if ( g_ogm.outputBuffer == nullptr )
				{
					g_ogm.outputBufferSize = 0;
					r = -2;
					break;
				}
			}

			yWShift = findSizeShift( g_ogm.th_yuvbuffer.y_width, g_ogm.th_info.width );
			uvWShift = findSizeShift( g_ogm.th_yuvbuffer.uv_width, g_ogm.th_info.width );
			yHShift = findSizeShift( g_ogm.th_yuvbuffer.y_height, g_ogm.th_info.height );
			uvHShift = findSizeShift( g_ogm.th_yuvbuffer.uv_height, g_ogm.th_info.height );

			if ( yWShift < 0 || uvWShift < 0 || yHShift < 0 || uvHShift < 0 )
			{
				Com_Printf( "[Theora] unexpected resolution in a YUV frame\n" );
				r = -1;
			}
			else
			{
				Frame_yuv_to_rgb24( g_ogm.th_yuvbuffer.y, g_ogm.th_yuvbuffer.u, g_ogm.th_yuvbuffer.v,
				                    g_ogm.th_info.width, g_ogm.th_info.height, g_ogm.th_yuvbuffer.y_stride,
				                    g_ogm.th_yuvbuffer.uv_stride, yWShift, uvWShift, yHShift, uvHShift,
				                    ( unsigned int * ) g_ogm.outputBuffer );

				r = 1;
				g_ogm.VFrameCount = th_frame;
			}
		}
	}

	return r;
}

/*

  return: 1 -> loaded a new Frame ( g_ogm.outputBuffer points to the actual frame )
                        0 -> no new Frame
                        <0  -> error
*/
static int loadVideoFrame()
{
	if ( g_ogm.videoStreamIsTheora )
	{
		return loadVideoFrameTheora();
	}

	// if we come to this point, there will be no codec that use the stream content ...
	if ( g_ogm.os_video.serialno )
	{
		ogg_packet op;

		while ( ogg_stream_packetout( &g_ogm.os_video, &op ) ) {; }
	}

	return 1;
}

/*

  return: true => noDataTransferred
*/
static bool loadFrame()
{
	bool anyDataTransferred = true;
	bool needVOutputData = true;

//  bool audioSDone = false;
//  bool videoSDone = false;
	bool audioWantsMoreData = false;
	int      status;

	while ( anyDataTransferred && ( needVOutputData || audioWantsMoreData ) )
	{
		anyDataTransferred = false;

//      xvid -> "gl" ? videoDone : needPacket
//      vorbis -> raw sound ? audioDone : needPacket
//      anyDataTransferred = videoDone && audioDone;
//      needVOutputData = videoDone && audioDone;
//      if needPacket
		{
//          videoStream -> xvid ? videoStreamDone : needPage
//          audioStream -> vorbis ? audioStreamDone : needPage
//          anyDataTransferred = audioStreamDone && audioStreamDone;

			if ( needVOutputData && ( status = loadVideoFrame() ) )
			{
				needVOutputData = false;

				if ( status > 0 )
				{
					anyDataTransferred = true;
				}
				else
				{
					anyDataTransferred = false; // error (we don't need any videodata and we had no transferred)
				}
			}

			if ( needVOutputData || audioWantsMoreData )
			{
				// try to transfer Pages to the audio- and video-Stream
				if ( loadPagesToStreams() )
				{
					// try to load a datablock from file
					anyDataTransferred |= !loadBlockToSync();
				}
				else
				{
					anyDataTransferred = true; // successful loadPagesToStreams()
				}
			}

			// load all Audio after loading new pages ...
			if ( g_ogm.VFrameCount > 1 ) // wait some videoframes (it's better to have some delay, than laggy sound)
			{
				audioWantsMoreData = loadAudio();
			}
		}
	}

//  ogg_packet_clear(&op);

	return !anyDataTransferred;
}

//from VLC ogg.c ( http://trac.videolan.org/vlc/browser/trunk/modules/demux/ogg.c )
struct stream_header_t
{
	char        streamtype[ 8 ];
	char        subtype[ 4 ];

	ogg_int32_t size; /* size of the structure */

	ogg_int64_t time_unit; /* in reference time */ // in 10^-7 seconds (dT between frames)
	ogg_int64_t samples_per_unit;
	ogg_int32_t default_len; /* in media time */

	ogg_int32_t buffersize;
	ogg_int16_t bits_per_sample;

	union
	{
		struct
		{
			ogg_int32_t width;
			ogg_int32_t height;
		} stream_header_video;

		struct
		{
			ogg_int16_t channels;
			ogg_int16_t blockalign;
			ogg_int32_t avgbytespersec;
		} stream_header_audio;
	} sh;
};

bool isPowerOf2( int x )
{
	int bitsSet = 0;

	for (unsigned i = 0; i < sizeof( int ) * 8; ++i )
	{
		if ( x & ( 1 << i ) )
		{
			++bitsSet;
		}
	}

	return ( bitsSet <= 1 );
}

/*
  return: 0 -> no problem
*/
//TODO: vorbis/theora-header&init in sub-functions
//TODO: "clean" error-returns ...
int Cin_OGM_Init( const char *filename )
{
	int        status;
	ogg_page   og;
	ogg_packet op;
	int        i;

	if ( g_ogm.ogmFile )
	{
		Com_Printf( S_WARNING "there is already an OGM running, which will be stopped before starting %s\n", filename );
		Cin_OGM_Shutdown();
	}

	memset( &g_ogm, 0, sizeof( cin_ogm_t ) );

	FS_FOpenFileRead( filename, &g_ogm.ogmFile, true );

	if ( !g_ogm.ogmFile )
	{
		Com_Printf( S_WARNING "Can't open OGM file for reading (%s)\n", filename );
		return -1;
	}

	ogg_sync_init( &g_ogm.oy );  /* Now we can read pages */

	//FIXME? can serialno be 0 in ogg? (better way to check inited?)
	//TODO: support for more than one audio stream? / detect files with one stream(or without correct ones)
	while ( !g_ogm.os_audio.serialno || !g_ogm.os_video.serialno )
	{
		if ( ogg_sync_pageout( &g_ogm.oy, &og ) == 1 )
		{
			if ( strstr( ( char * )( og.body + 1 ), "vorbis" ) )
			{
				//FIXME? better way to find audio stream
				if ( g_ogm.os_audio.serialno )
				{
					Com_Printf( S_WARNING "more than one audio stream in OGM file(%s). We will stay at the first one\n", filename );
				}
				else
				{
					ogg_stream_init( &g_ogm.os_audio, ogg_page_serialno( &og ) );
					ogg_stream_pagein( &g_ogm.os_audio, &og );
				}
			}

			if ( strstr( ( char * )( og.body + 1 ), "theora" ) )
			{
				if ( g_ogm.os_video.serialno )
				{
					Com_Printf( S_WARNING "more than one video stream in OGM file(%s). We will stay at the first one\n", filename );
				}
				else
				{
					g_ogm.videoStreamIsTheora = true;
					ogg_stream_init( &g_ogm.os_video, ogg_page_serialno( &og ) );
					ogg_stream_pagein( &g_ogm.os_video, &og );
				}
			}

		}
		else if ( loadBlockToSync() )
		{
			break;
		}
	}

	if ( !g_ogm.os_audio.serialno )
	{
		Com_Printf( S_WARNING "Didn't find a Vorbis audio stream in %s\n", filename );
		return -2;
	}

	if ( !g_ogm.os_video.serialno )
	{
		Com_Printf( S_WARNING "Haven't found a video stream in OGM file (%s)\n", filename );
		return -3;
	}

	//load vorbis header
	vorbis_info_init( &g_ogm.vi );
	vorbis_comment_init( &g_ogm.vc );
	i = 0;

	while ( i < 3 )
	{
		status = ogg_stream_packetout( &g_ogm.os_audio, &op );

		if ( status < 0 )
		{
			Com_Printf( S_WARNING "Corrupt Ogg packet while loading Vorbis headers (%s)\n", filename );
			return -8;
		}

		if ( status > 0 )
		{
			status = vorbis_synthesis_headerin( &g_ogm.vi, &g_ogm.vc, &op );

			if ( i == 0 && status < 0 )
			{
				Com_Printf( S_WARNING "This Ogg bitstream does not contain Vorbis audio data (%s)\n", filename );
				return -9;
			}

			++i;
		}
		else if ( loadPagesToStreams() )
		{
			if ( loadBlockToSync() )
			{
				Com_Printf( S_WARNING "Couldn't find all Vorbis headers before end of OGM file (%s)\n", filename );
				return -10;
			}
		}
	}

	vorbis_synthesis_init( &g_ogm.vd, &g_ogm.vi );

	if ( g_ogm.videoStreamIsTheora )
	{
		ROQ_GenYUVTables();

		theora_info_init( &g_ogm.th_info );
		theora_comment_init( &g_ogm.th_comment );

		i = 0;

		while ( i < 3 )
		{
			status = ogg_stream_packetout( &g_ogm.os_video, &op );

			if ( status < 0 )
			{
				Com_Printf( S_WARNING "Corrupt Ogg packet while loading Theora headers (%s)\n", filename );
				return -8;
			}

			if ( status > 0 )
			{
				status = theora_decode_header( &g_ogm.th_info, &g_ogm.th_comment, &op );

				if ( i == 0 && status != 0 )
				{
					Com_Printf( S_WARNING "This Ogg bitstream does not contain Theora data (%s)\n", filename );
					return -9;
				}

				++i;
			}
			else if ( loadPagesToStreams() )
			{
				if ( loadBlockToSync() )
				{
					Com_Printf( S_WARNING "Couldn't find all Theora headers before end of OGM file (%s)\n", filename );
					return -10;
				}
			}
		}

		theora_decode_init( &g_ogm.th_state, &g_ogm.th_info );

		if ( !isPowerOf2( g_ogm.th_info.width ) )
		{
			Com_Printf( S_WARNING "Video width of the OGM file isn't a power of 2 (%s)\n", filename );
			return -5;
		}

		if ( !isPowerOf2( g_ogm.th_info.height ) )
		{
			Com_Printf( S_WARNING "Video height of the OGM file isn't a power of 2 (%s)\n", filename );
			return -6;
		}

		g_ogm.Vtime_unit = ( ( ogg_int64_t ) g_ogm.th_info.fps_denominator * 1000 * 10000 / g_ogm.th_info.fps_numerator );
	}

	Log::Debug( "OGM-Init done (%s)", filename );

	return 0;
}

int nextNeededVFrame()
{
	return ( int )( g_ogm.currentTime * ( ogg_int64_t ) 10000 / g_ogm.Vtime_unit );
}

/*

  time ~> time in ms to which the movie should run
  return: 0 => nothing special
                        1 => eof
*/
int Cin_OGM_Run( int time )
{
	g_ogm.currentTime = time;

	while ( !g_ogm.VFrameCount || time + 20 >= ( int )( g_ogm.VFrameCount * g_ogm.Vtime_unit / 10000 ) )
	{
		if ( loadFrame() )
		{
			return 1;
		}
	}

	return 0;
}

/*
  Gives a Pointer to the current Output-Buffer
  and the Resolution
*/
unsigned char  *Cin_OGM_GetOutput( int *outWidth, int *outHeight )
{
	if ( outWidth != nullptr )
	{
		*outWidth = g_ogm.outputWidht;
	}

	if ( outHeight != nullptr )
	{
		*outHeight = g_ogm.outputHeight;
	}

	return g_ogm.outputBuffer;
}

void Cin_OGM_Shutdown()
{
	theora_clear( &g_ogm.th_state );
	theora_comment_clear( &g_ogm.th_comment );
	theora_info_clear( &g_ogm.th_info );

	if ( g_ogm.outputBuffer )
	{
		free( g_ogm.outputBuffer );
	}

	g_ogm.outputBuffer = nullptr;

	vorbis_dsp_clear( &g_ogm.vd );
	vorbis_comment_clear( &g_ogm.vc );
	vorbis_info_clear( &g_ogm.vi );  /* must be called last (comment from vorbis example code) */

	ogg_stream_clear( &g_ogm.os_audio );
	ogg_stream_clear( &g_ogm.os_video );

	ogg_sync_clear( &g_ogm.oy );

	FS_FCloseFile( g_ogm.ogmFile );
	g_ogm.ogmFile = 0;
}

#else

int Cin_OGM_Init( const char* )
{
	return 1;
}

int Cin_OGM_Run( int )
{
	return 1;
}

unsigned char  *Cin_OGM_GetOutput( int*, int* )
{
	return 0;
}

void Cin_OGM_Shutdown()
{
}

#endif
