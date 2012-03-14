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

  This is a "ogm"-decoder to use a "better"(smaller files,higher resolutions) Cinematic-Format than roq

  In this code "ogm" is only: ogg wrapper, vorbis audio, xvid video (or theora video)
  (ogm(Ogg Media) in general is ogg wrapper with all kind of audio/video/subtitle/...)

... infos used for this src:
xvid:
 * examples/xvid_decraw.c
 * xvid.h
ogg/vobis:
 * decoder_example.c (libvorbis src)
 * libogg Documentation ( http://www.xiph.org/ogg/doc/libogg/ )
 * VLC ogg demux ( http://trac.videolan.org/vlc/browser/trunk/modules/demux/ogg.c )
theora:
 * theora doxygen docs (1.0beta1)
*/

#if defined( USE_CODEC_VORBIS ) && ( defined( USE_CIN_XVID ) || defined( USE_CIN_THEORA ))

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#ifdef USE_CIN_XVID
#include <xvid.h>

#if defined( WIN32 ) || defined( __i386__ )
#define ARCH_IS_32BIT
#define ARCH_IS_IA32
#else
#define ARCH_IS_64BIT
#define ARCH_IS_IA64
#endif

#endif

#ifdef USE_CIN_THEORA
#include <theora/theora.h>
#endif

#include "client.h"
#include "snd_local.h"

#define OGG_BUFFER_SIZE 8 * 1024 //4096

typedef struct
{
	fileHandle_t     ogmFile;

	ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
	//ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
	ogg_stream_state os_audio;
	ogg_stream_state os_video;

	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
	vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
	vorbis_comment   vc; /* struct that stores all the bitstream user comments */

	qboolean         videoStreamIsXvid; //FIXME: atm there isn't realy a check for this (all "video" streams are handelt as xvid, because xvid support more than one "subtype")
#ifdef USE_CIN_XVID
	xvid_dec_stats_t xvid_dec_stats;
	void             *xvid_dec_handle;
#endif
	qboolean         videoStreamIsTheora;
#ifdef USE_CIN_THEORA
	theora_info      th_info; // dump_video.c(example decoder): ti
	theora_comment   th_comment; // dump_video.c(example decoder): tc
	theora_state     th_state; // dump_video.c(example decoder): td

	yuv_buffer       th_yuvbuffer;
#endif

	unsigned char *outputBuffer;
	int           outputWidht;
	int           outputHeight;
	int           outputBufferSize; // in Pixel (so "real Bytesize" = outputBufferSize*4)
	int           VFrameCount; // output video-stream
	ogg_int64_t   Vtime_unit;
	int           currentTime; // input from Run-function
} cin_ogm_t;

static cin_ogm_t g_ogm;

int              nextNeededVFrame( void );

/* ####################### #######################

  XVID

*/
#ifdef USE_CIN_XVID

#define BPP 4

static int init_xvid()
{
	int               ret;

	xvid_gbl_init_t   xvid_gbl_init;
	xvid_dec_create_t xvid_dec_create;

	/* Reset the structure with zeros */
	memset( &xvid_gbl_init, 0, sizeof( xvid_gbl_init_t ) );
	memset( &xvid_dec_create, 0, sizeof( xvid_dec_create_t ) );

	/* Version */
	xvid_gbl_init.version = XVID_VERSION;

	xvid_gbl_init.cpu_flags = 0;
	xvid_gbl_init.debug = 0;

	xvid_global( NULL, 0, &xvid_gbl_init, NULL );

	/* Version */
	xvid_dec_create.version = XVID_VERSION;

	/*
	 * Image dimensions -- set to 0, xvidcore will resize when ever it is
	 * needed
	 */
	xvid_dec_create.width = 0;
	xvid_dec_create.height = 0;

	ret = xvid_decore( NULL, XVID_DEC_CREATE, &xvid_dec_create, NULL );

	g_ogm.xvid_dec_handle = xvid_dec_create.handle;

	return ( ret );
}

static int dec_xvid( unsigned char *input, int input_size )
{
	int              ret;

	xvid_dec_frame_t xvid_dec_frame;

	/* Reset all structures */
	memset( &xvid_dec_frame, 0, sizeof( xvid_dec_frame_t ) );
	memset( &g_ogm.xvid_dec_stats, 0, sizeof( xvid_dec_stats_t ) );

	/* Set version */
	xvid_dec_frame.version = XVID_VERSION;
	g_ogm.xvid_dec_stats.version = XVID_VERSION;

	/* No general flags to set */
	xvid_dec_frame.general = XVID_LOWDELAY; //0;

	/* Input stream */
	xvid_dec_frame.bitstream = input;
	xvid_dec_frame.length = input_size;

	/* Output frame structure */
	xvid_dec_frame.output.plane[ 0 ] = g_ogm.outputBuffer;
	xvid_dec_frame.output.stride[ 0 ] = g_ogm.outputWidht * BPP;

	if( g_ogm.outputBuffer == NULL )
	{
		xvid_dec_frame.output.csp = XVID_CSP_NULL;
	}
	else
	{
		xvid_dec_frame.output.csp = XVID_CSP_RGBA; // example was with XVID_CSP_I420
	}

	ret = xvid_decore( g_ogm.xvid_dec_handle, XVID_DEC_DECODE, &xvid_dec_frame, &g_ogm.xvid_dec_stats );

	return ( ret );
}

static int shutdown_xvid()
{
	int ret = 0;

	if( g_ogm.xvid_dec_handle )
	{
		ret = xvid_decore( g_ogm.xvid_dec_handle, XVID_DEC_DESTROY, NULL, NULL );
	}

	return ( ret );
}

#endif

/* ####################### #######################

  OGG/OGM
  ... also calls to vorbis/theora-libs

*/

/*
  loadBlockToSync

  return:
  !0 -> no data transferred
*/
static int loadBlockToSync( void )
{
	int  r = -1;
	char *buffer;
	int  bytes;

	if( g_ogm.ogmFile )
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
static int loadPagesToStreams( void )
{
	int              r = -1;
	int              AudioPages = 0;
	int              VideoPages = 0;
	ogg_stream_state *osptr = NULL;
	ogg_page         og;

	while( !AudioPages || !VideoPages )
	{
		if( ogg_sync_pageout( &g_ogm.oy, &og ) != 1 )
		{
			break;
		}

		if( g_ogm.os_audio.serialno == ogg_page_serialno( &og ) )
		{
			osptr = &g_ogm.os_audio;
			++AudioPages;
		}

		if( g_ogm.os_video.serialno == ogg_page_serialno( &og ) )
		{
			osptr = &g_ogm.os_video;
			++VideoPages;
		}

		if( osptr != NULL )
		{
			ogg_stream_pagein( osptr, &og );
		}
	}

	if( AudioPages && VideoPages )
	{
		r = 0;
	}

	return r;
}

#define SIZEOF_RAWBUFF    4 * 1024
static byte rawBuffer[ SIZEOF_RAWBUFF ];

#define MIN_AUDIO_PRELOAD 400 // in ms
#define MAX_AUDIO_PRELOAD 500 // in ms

/*

  return: audio wants more packets
*/
static qboolean loadAudio( void )
{
	qboolean     anyDataTransferred = qtrue;
	float        **pcm;
	float        *right, *left;
	int          samples, samplesNeeded;
	int          i;
	short        *ptr;
	ogg_packet   op;
	vorbis_block vb;

	memset( &op, 0, sizeof( op ) );
	memset( &vb, 0, sizeof( vb ) );
	vorbis_block_init( &g_ogm.vd, &vb );

	while( anyDataTransferred && g_ogm.currentTime + MAX_AUDIO_PRELOAD > ( int )( g_ogm.vd.granulepos * 1000 / g_ogm.vi.rate ) )
	{
		anyDataTransferred = qfalse;

		if( ( samples = vorbis_synthesis_pcmout( &g_ogm.vd, &pcm ) ) > 0 )
		{
			// vorbis -> raw
			ptr = ( short * ) rawBuffer;
			samplesNeeded = ( SIZEOF_RAWBUFF ) / ( 2 * 2 ); // (width*channel)

			if( samples < samplesNeeded )
			{
				samplesNeeded = samples;
			}

			left = pcm[ 0 ];
			right = ( g_ogm.vi.channels > 1 ) ? pcm[ 1 ] : pcm[ 0 ];

			for( i = 0; i < samplesNeeded; ++i )
			{
				ptr[ 0 ] = ( left[ i ] >= -1.0f &&
				             left[ i ] <= 1.0f ) ? left[ i ] * 32767.f : 32767 * ( ( left[ i ] > 0.0f ) - ( left[ i ] < 0.0f ) );
				ptr[ 1 ] = ( right[ i ] >= -1.0f &&
				             right[ i ] <= 1.0f ) ? right[ i ] * 32767.f : 32767 * ( ( right[ i ] > 0.0f ) - ( right[ i ] < 0.0f ) );
				ptr += 2; //numChans;
			}

			if( i > 0 )
			{
				// tell libvorbis how many samples we actually consumed
				vorbis_synthesis_read( &g_ogm.vd, i );

//              S_RawSamples( ssize, 22050, 2, 2, (byte *)sbuf, 1.0f );
				S_RawSamples( 0, i, g_ogm.vi.rate, 2, 2, rawBuffer, 1.0f, 1.0f );

				anyDataTransferred = qtrue;
			}
		}

		if( !anyDataTransferred )
		{
			// op -> vorbis
			if( ogg_stream_packetout( &g_ogm.os_audio, &op ) )
			{
				if( vorbis_synthesis( &vb, &op ) == 0 )
				{
					vorbis_synthesis_blockin( &g_ogm.vd, &vb );
				}

				anyDataTransferred = qtrue;
			}
		}
	}

	vorbis_block_clear( &vb );

	if( g_ogm.currentTime + MIN_AUDIO_PRELOAD > ( int )( g_ogm.vd.granulepos * 1000 / g_ogm.vi.rate ) )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*

  return: 1 -> loaded a new Frame ( g_ogm.outputBuffer points to the actual frame )
                        0 -> no new Frame
                        <0  -> error
*/
#ifdef USE_CIN_XVID
static int loadVideoFrameXvid()
{
	int        r = 0;
	ogg_packet op;
	int        used_bytes = 0;

	memset( &op, 0, sizeof( op ) );

	while( !r && ( ogg_stream_packetout( &g_ogm.os_video, &op ) ) )
	{
		used_bytes = dec_xvid( op.packet, op.bytes );

		if( g_ogm.xvid_dec_stats.type == XVID_TYPE_VOL )
		{
			if( g_ogm.outputWidht != g_ogm.xvid_dec_stats.data.vol.width ||
			    g_ogm.outputHeight != g_ogm.xvid_dec_stats.data.vol.height )
			{
				g_ogm.outputWidht = g_ogm.xvid_dec_stats.data.vol.width;
				g_ogm.outputHeight = g_ogm.xvid_dec_stats.data.vol.height;
				Com_DPrintf( "[XVID]new resolution %dx%d\n", g_ogm.outputWidht, g_ogm.outputHeight );
			}

			if( g_ogm.outputBufferSize < g_ogm.xvid_dec_stats.data.vol.width * g_ogm.xvid_dec_stats.data.vol.height )
			{
				g_ogm.outputBufferSize = g_ogm.xvid_dec_stats.data.vol.width * g_ogm.xvid_dec_stats.data.vol.height;

				/* Free old output buffer */
				if( g_ogm.outputBuffer )
				{
					free( g_ogm.outputBuffer );
				}

				/* Allocate the new buffer */
				g_ogm.outputBuffer = ( unsigned char * ) malloc( g_ogm.outputBufferSize * 4 );  //FIXME? should the 4 stay for BPP?

				if( g_ogm.outputBuffer == NULL )
				{
					g_ogm.outputBufferSize = 0;
					r = -2;
					break;
				}
			}

			// use the rest of this packet
			used_bytes += dec_xvid( op.packet + used_bytes, op.bytes - used_bytes );
		}

		// we got a real output frame ...
		if( g_ogm.xvid_dec_stats.type > 0 )
		{
			r = 1;

			++g_ogm.VFrameCount;
//          Com_Printf("frame infos: %d %d %d\n", xvid_dec_stats.data.vop.general, xvid_dec_stats.data.vop.time_base, xvid_dec_stats.data.vop.time_increment);
//          Com_Printf("frame info time: %d (Frame# %d, %d)\n", xvid_dec_stats.data.vop.time_base, VFrameCount, (int)(VFrameCount*Vtime_unit/10000000));
		}

//      if((op.bytes-used_bytes)>0)
//          Com_Printf("unused: %d(firstChar: %X)\n",(op.bytes-used_bytes),(int)(op.packet[used_bytes]));
	}

	return r;
}

#endif

/*

  return: 1 -> loaded a new Frame ( g_ogm.outputBuffer points to the actual frame )
                        0 -> no new Frame
                        <0  -> error
*/
#ifdef USE_CIN_THEORA

/*
how many >> are needed to make y==x (shifting y>>i)
return: -1  -> no match
                >=0 -> number of shifts
*/
static int findSizeShift( int x, int y )
{
	int i;

	for( i = 0; ( y >> i ); ++i )
	{
		if( x == ( y >> i ) )
		{
			return i;
		}
	}

	return -1;
}

static int loadVideoFrameTheora( void )
{
	int        r = 0;
	ogg_packet op;

	memset( &op, 0, sizeof( op ) );

	while( !r && ( ogg_stream_packetout( &g_ogm.os_video, &op ) ) )
	{
		ogg_int64_t th_frame;

		theora_decode_packetin( &g_ogm.th_state, &op );

		th_frame = theora_granule_frame( &g_ogm.th_state, g_ogm.th_state.granulepos );

		if( ( g_ogm.VFrameCount < th_frame && th_frame >= nextNeededVFrame() ) || !g_ogm.outputBuffer )
		{
//          int i,j;
			int yWShift, uvWShift;
			int yHShift, uvHShift;

			if( theora_decode_YUVout( &g_ogm.th_state, &g_ogm.th_yuvbuffer ) )
			{
				continue;
			}

			if( g_ogm.outputWidht != g_ogm.th_info.width || g_ogm.outputHeight != g_ogm.th_info.height )
			{
				g_ogm.outputWidht = g_ogm.th_info.width;
				g_ogm.outputHeight = g_ogm.th_info.height;
				Com_DPrintf( "[Theora(ogg)]new resolution %dx%d\n", g_ogm.outputWidht, g_ogm.outputHeight );
			}

			if( g_ogm.outputBufferSize < g_ogm.th_info.width * g_ogm.th_info.height )
			{
				g_ogm.outputBufferSize = g_ogm.th_info.width * g_ogm.th_info.height;

				/* Free old output buffer */
				if( g_ogm.outputBuffer )
				{
					free( g_ogm.outputBuffer );
				}

				/* Allocate the new buffer */
				g_ogm.outputBuffer = ( unsigned char * ) malloc( g_ogm.outputBufferSize * 4 );

				if( g_ogm.outputBuffer == NULL )
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

			if( yWShift < 0 || uvWShift < 0 || yHShift < 0 || uvHShift < 0 )
			{
				Com_Printf( "[Theora] unexpected resolution in a yuv-Frame\n" );
				r = -1;
			}
			else
			{
				Frame_yuv_to_rgb24( g_ogm.th_yuvbuffer.y, g_ogm.th_yuvbuffer.u, g_ogm.th_yuvbuffer.v,
				                    g_ogm.th_info.width, g_ogm.th_info.height, g_ogm.th_yuvbuffer.y_stride,
				                    g_ogm.th_yuvbuffer.uv_stride, yWShift, uvWShift, yHShift, uvHShift,
				                    ( unsigned int * ) g_ogm.outputBuffer );

				/*        unsigned char*  pixelPtr = g_ogm.outputBuffer;
				                                unsigned int* pixPtr;
				                                pixPtr = (unsigned int*)g_ogm.outputBuffer;

				                                //TODO: use one yuv->rgb funktion for the hole frame (the big amout of stack movement(yuv->rgb calls) couldn't be good ;) )
				                                for(j=0;j<g_ogm.th_info.height;++j) {
				                                        for(i=0;i<g_ogm.th_info.width;++i) {
				#if 1
				                                                // simple grayscale-output ^^
				                                                pixelPtr[0] =
				                                                        pixelPtr[1] =
				                                                        pixelPtr[2] = g_ogm.th_yuvbuffer.y[i+j*g_ogm.th_yuvbuffer.y_stride];
				                                                pixelPtr+=4;

				#else
				                                                // using RoQ yuv->rgb code
				                                                *pixPtr++ = yuv_to_rgb24( g_ogm.th_yuvbuffer.y[(i>>yWShift)+(j>>yHShift)*g_ogm.th_yuvbuffer.y_stride],
				                                                                                                g_ogm.th_yuvbuffer.u[(i>>uvWShift)+(j>>uvHShift)*g_ogm.th_yuvbuffer.uv_stride],
				                                                                                                g_ogm.th_yuvbuffer.v[(i>>uvWShift)+(j>>uvHShift)*g_ogm.th_yuvbuffer.uv_stride]);
				#endif
				                                        }
				                                }
				*/

				r = 1;
				g_ogm.VFrameCount = th_frame;
			}
		}
	}

	return r;
}

#endif

/*

  return: 1 -> loaded a new Frame ( g_ogm.outputBuffer points to the actual frame )
                        0 -> no new Frame
                        <0  -> error
*/
static int loadVideoFrame( void )
{
#ifdef USE_CIN_XVID

	if( g_ogm.videoStreamIsXvid )
	{
		return loadVideoFrameXvid();
	}

#endif
#ifdef USE_CIN_THEORA

	if( g_ogm.videoStreamIsTheora )
	{
		return loadVideoFrameTheora();
	}

#endif

	// if we come to this point, there will be no codec that use the stream content ...
	if( g_ogm.os_video.serialno )
	{
		ogg_packet op;

		while( ogg_stream_packetout( &g_ogm.os_video, &op ) ) {; }
	}

	return 1;
}

/*

  return: qtrue => noDataTransferred
*/
static qboolean loadFrame( void )
{
	qboolean anyDataTransferred = qtrue;
	qboolean needVOutputData = qtrue;

//  qboolean audioSDone = qfalse;
//  qboolean videoSDone = qfalse;
	qboolean audioWantsMoreData = qfalse;
	int      status;

	while( anyDataTransferred && ( needVOutputData || audioWantsMoreData ) )
	{
		anyDataTransferred = qfalse;

//      xvid -> "gl" ? videoDone : needPacket
//      vorbis -> raw sound ? audioDone : needPacket
//      anyDataTransferred = videoDone && audioDone;
//      needVOutputData = videoDone && audioDone;
//      if needPacket
		{
//          videoStream -> xvid ? videoStreamDone : needPage
//          audioSteam -> vorbis ? audioStreamDone : needPage
//          anyDataTransferred = audioStreamDone && audioStreamDone;

			if( needVOutputData && ( status = loadVideoFrame() ) )
			{
				needVOutputData = qfalse;

				if( status > 0 )
				{
					anyDataTransferred = qtrue;
				}
				else
				{
					anyDataTransferred = qfalse; // error (we don't need any videodata and we had no transferred)
				}
			}

//          if needPage
			if( needVOutputData || audioWantsMoreData )
			{
				// try to transfer Pages to the audio- and video-Stream
				if( loadPagesToStreams() )
				{
					// try to load a datablock from file
					anyDataTransferred |= !loadBlockToSync();
				}
				else
				{
					anyDataTransferred = qtrue; // successful loadPagesToStreams()
				}
			}

			// load all Audio after loading new pages ...
			if( g_ogm.VFrameCount > 1 )  // wait some videoframes (it's better to have some delay, than a lagy sound)
			{
				audioWantsMoreData = loadAudio();
			}
		}
	}

//  ogg_packet_clear(&op);

	return !anyDataTransferred;
}

//from VLC ogg.c ( http://trac.videolan.org/vlc/browser/trunk/modules/demux/ogg.c )
typedef struct
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
} stream_header_t;

qboolean isPowerOf2( int x )
{
	int bitsSet = 0;
	int i;

	for( i = 0; i < sizeof( int ) * 8; ++i )
	{
		if( x & ( 1 << i ) )
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

	if( g_ogm.ogmFile )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: it seams there was already a ogm running, it will be killed to start %s\n", filename );
		Cin_OGM_Shutdown();
	}

	memset( &g_ogm, 0, sizeof( cin_ogm_t ) );

	FS_FOpenFileRead( filename, &g_ogm.ogmFile, qtrue );

	if( !g_ogm.ogmFile )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: Can't open ogm-file for reading (%s)\n", filename );
		return -1;
	}

	ogg_sync_init( &g_ogm.oy );  /* Now we can read pages */

	//FIXME? can serialno be 0 in ogg? (better way to check inited?)
	//TODO: support for more than one audio stream? / detect files with one stream(or without correct ones)
	while( !g_ogm.os_audio.serialno || !g_ogm.os_video.serialno )
	{
		if( ogg_sync_pageout( &g_ogm.oy, &og ) == 1 )
		{
			if( strstr( ( char * )( og.body + 1 ), "vorbis" ) )
			{
				//FIXME? better way to find audio stream
				if( g_ogm.os_audio.serialno )
				{
					Com_Printf( S_COLOR_YELLOW "WARNING: more than one audio stream, in ogm-file(%s) ... we will stay at the first one\n", filename );
				}
				else
				{
					ogg_stream_init( &g_ogm.os_audio, ogg_page_serialno( &og ) );
					ogg_stream_pagein( &g_ogm.os_audio, &og );
				}
			}

#ifdef USE_CIN_THEORA

			if( strstr( ( char * )( og.body + 1 ), "theora" ) )
			{
				if( g_ogm.os_video.serialno )
				{
					Com_Printf( S_COLOR_YELLOW "WARNING: more than one video stream, in ogm-file(%s) ... we will stay at the first one\n", filename );
				}
				else
				{
					g_ogm.videoStreamIsTheora = qtrue;
					ogg_stream_init( &g_ogm.os_video, ogg_page_serialno( &og ) );
					ogg_stream_pagein( &g_ogm.os_video, &og );
				}
			}

#endif
#ifdef USE_CIN_XVID

			if( strstr( ( char * )( og.body + 1 ), "video" ) )
			{
				//FIXME? better way to find video stream
				if( g_ogm.os_video.serialno )
				{
					Com_Printf( "more than one video stream, in ogm-file(%s) ... we will stay at the first one\n", filename );
				}
				else
				{
					stream_header_t *sh;

					g_ogm.videoStreamIsXvid = qtrue;

					sh = ( stream_header_t * )( og.body + 1 );

					//TODO: one solution for checking xvid and theora
					if( !isPowerOf2( sh->sh.stream_header_video.width ) )
					{
						Com_Printf( "VideoWidth of the ogm-file isn't a power of 2 value (%s)\n", filename );

						return -5;
					}

					if( !isPowerOf2( sh->sh.stream_header_video.height ) )
					{
						Com_Printf( "VideoHeight of the ogm-file isn't a power of 2 value (%s)\n", filename );

						return -6;
					}

					g_ogm.Vtime_unit = sh->time_unit;

					ogg_stream_init( &g_ogm.os_video, ogg_page_serialno( &og ) );
					ogg_stream_pagein( &g_ogm.os_video, &og );
				}
			}

#endif
		}
		else if( loadBlockToSync() )
		{
			break;
		}
	}

	if( g_ogm.videoStreamIsXvid && g_ogm.videoStreamIsTheora )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: Found \"video\"- and \"theora\"-stream ,ogm-file (%s)\n", filename );
		return -2;
	}

#if 1

	if( !g_ogm.os_audio.serialno )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: Haven't found a audio(vorbis) stream in ogm-file (%s)\n", filename );
		return -2;
	}

#endif

	if( !g_ogm.os_video.serialno )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: Haven't found a video stream in ogm-file (%s)\n", filename );
		return -3;
	}

	//load vorbis header
	vorbis_info_init( &g_ogm.vi );
	vorbis_comment_init( &g_ogm.vc );
	i = 0;

	while( i < 3 )
	{
		status = ogg_stream_packetout( &g_ogm.os_audio, &op );

		if( status < 0 )
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: Corrupt ogg packet while loading vorbis-headers, ogm-file(%s)\n", filename );
			return -8;
		}

		if( status > 0 )
		{
			status = vorbis_synthesis_headerin( &g_ogm.vi, &g_ogm.vc, &op );

			if( i == 0 && status < 0 )
			{
				Com_Printf( S_COLOR_YELLOW "WARNING: This Ogg bitstream does not contain Vorbis audio data, ogm-file(%s)\n", filename );
				return -9;
			}

			++i;
		}
		else if( loadPagesToStreams() )
		{
			if( loadBlockToSync() )
			{
				Com_Printf( S_COLOR_YELLOW "WARNING: Couldn't find all vorbis headers before end of ogm-file (%s)\n", filename );
				return -10;
			}
		}
	}

	vorbis_synthesis_init( &g_ogm.vd, &g_ogm.vi );

#ifdef USE_CIN_XVID
	status = init_xvid();

	if( status )
	{
		Com_Printf( "[Xvid]Decore INIT problem, return value %d(ogm-file: %s)\n", status, filename );

		return -4;
	}

#endif

#ifdef USE_CIN_THEORA

	if( g_ogm.videoStreamIsTheora )
	{
		ROQ_GenYUVTables();

		theora_info_init( &g_ogm.th_info );
		theora_comment_init( &g_ogm.th_comment );

		i = 0;

		while( i < 3 )
		{
			status = ogg_stream_packetout( &g_ogm.os_video, &op );

			if( status < 0 )
			{
				Com_Printf( S_COLOR_YELLOW "WARNING: Corrupt ogg packet while loading theora-headers, ogm-file(%s)\n", filename );
				return -8;
			}

			if( status > 0 )
			{
				status = theora_decode_header( &g_ogm.th_info, &g_ogm.th_comment, &op );

				if( i == 0 && status != 0 )
				{
					Com_Printf( S_COLOR_YELLOW "WARNING: This Ogg bitstream does not contain theora data, ogm-file(%s)\n", filename );
					return -9;
				}

				++i;
			}
			else if( loadPagesToStreams() )
			{
				if( loadBlockToSync() )
				{
					Com_Printf( S_COLOR_YELLOW "WARNING: Couldn't find all theora headers before end of ogm-file (%s)\n", filename );
					return -10;
				}
			}
		}

		theora_decode_init( &g_ogm.th_state, &g_ogm.th_info );

		if( !isPowerOf2( g_ogm.th_info.width ) )
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: VideoWidth of the ogm-file isn't a power of 2 value (%s)\n", filename );
			return -5;
		}

		if( !isPowerOf2( g_ogm.th_info.height ) )
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: VideoHeight of the ogm-file isn't a power of 2 value (%s)\n", filename );
			return -6;
		}

		g_ogm.Vtime_unit = ( ( ogg_int64_t ) g_ogm.th_info.fps_denominator * 1000 * 10000 / g_ogm.th_info.fps_numerator );
	}

#endif

	Com_DPrintf( "OGM-Init done (%s)\n", filename );

	return 0;
}

int nextNeededVFrame( void )
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

	while( !g_ogm.VFrameCount || time + 20 >= ( int )( g_ogm.VFrameCount * g_ogm.Vtime_unit / 10000 ) )
	{
		if( loadFrame() )
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
	if( outWidth != NULL )
	{
		*outWidth = g_ogm.outputWidht;
	}

	if( outHeight != NULL )
	{
		*outHeight = g_ogm.outputHeight;
	}

	return g_ogm.outputBuffer;
}

void Cin_OGM_Shutdown()
{
#ifdef USE_CIN_XVID
	int status;

	status = shutdown_xvid();

	if( status )
	{
		Com_Printf( "[Xvid]Decore RELEASE problem, return value %d\n", status );
	}

#endif

#ifdef USE_CIN_THEORA
	theora_clear( &g_ogm.th_state );
	theora_comment_clear( &g_ogm.th_comment );
	theora_info_clear( &g_ogm.th_info );
#endif

	if( g_ogm.outputBuffer )
	{
		free( g_ogm.outputBuffer );
	}

	g_ogm.outputBuffer = NULL;

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
int Cin_OGM_Init( const char *filename )
{
	return 1;
}

int Cin_OGM_Run( int time )
{
	return 1;
}

unsigned char  *Cin_OGM_GetOutput( int *outWidth, int *outHeight )
{
	return 0;
}

void Cin_OGM_Shutdown()
{
}

#endif
