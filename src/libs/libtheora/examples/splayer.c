/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2009                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function: example SDL player application; plays Ogg Theora files (with
            optional Vorbis audio second stream)
 * Modified by M. Piacentini http://www.tabuleiro.com
 * from the original Theora Alpha player_sample files
 *
 * Modified to build on Windows and use PortAudio as the audio
 * and synchronization layer, calculating license.
 *
 * With SDL PortAudio it should be easy to compile on other platforms and
 * sound providers like DirectSound
 * just include the corresponding .c file (see PortAudio main documentation
 * for additional information)

 ********************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include "theora/theora.h"
#include "vorbis/codec.h"

#ifdef WIN32
#include <windows.h>
#include <io.h>
#endif

#include <portaudio.h>
#include <SDL.h>

/* for portaudio  */
#define FRAMES_PER_BUFFER (256)

/*start of portaudio helper functions, extracted from pablio directory*/

/* Pa_streamio routines modified by mauricio at xiph.org
 * Modified version of Portable Audio Blocking read/write utility.
 * from the original PABLIO files
 * Modified to support only playback buffers, direct access
 * to the underlying stream time and remove blocking operations*/

/* PortAudio copyright notice follows */

/*
 * Author: Phil Burk, http://www.softsynth.com
 *
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.audiomulch.com/portaudio/
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 */

typedef struct
{
    long   bufferSize; /* Number of bytes in FIFO. Power of 2. Set by RingBuffer_Init. */
/* These are declared volatile because they are written by a different thread than the reader. */
    volatile long   writeIndex; /* Index of next writable byte. Set by RingBuffer_AdvanceWriteIndex. */
    volatile long   readIndex;  /* Index of next readable byte. Set by RingBuffer_AdvanceReadIndex. */
    long   bigMask;    /* Used for wrapping indices with extra bit to distinguish full/empty. */
    long   smallMask;  /* Used for fitting indices to buffer. */
    char *buffer;
}
RingBuffer;

typedef struct
{
    RingBuffer   outFIFO;
    PortAudioStream *stream;
    int          bytesPerFrame;
    int          samplesPerFrame;
}
PASTREAMIO_Stream;

/* Values for flags for OpenAudioStream(). */
/* Keep PABLIO ones*/

#define PASTREAMIO_READ     (1<<0)
#define PASTREAMIO_WRITE    (1<<1)
#define PASTREAMIO_READ_WRITE    (PABLIO_READ|PABLIO_WRITE)
#define PASTREAMIO_MONO     (1<<2)
#define PASTREAMIO_STEREO   (1<<3)

/***************************************************************************
** Helper function added to report stream time. */

PaTimestamp GetAudioStreamTime( PASTREAMIO_Stream *aStream ){
  return Pa_StreamTime( aStream->stream ) ;
}

 /***************************************************************************
** Clear buffer. Should only be called when buffer is NOT being read. */
void RingBuffer_Flush( RingBuffer *rbuf )
{
    rbuf->writeIndex = rbuf->readIndex = 0;
}

/***************************************************************************
 * Initialize FIFO.
 * numBytes must be power of 2, returns -1 if not.
 */
long RingBuffer_Init( RingBuffer *rbuf, long numBytes, void *dataPtr )
{
    if( ((numBytes-1) & numBytes) != 0) return -1; /* Not Power of two. */
    rbuf->bufferSize = numBytes;
    rbuf->buffer = (char *)dataPtr;
    RingBuffer_Flush( rbuf );
    rbuf->bigMask = (numBytes*2)-1;
    rbuf->smallMask = (numBytes)-1;
    return 0;
}
/***************************************************************************
** Return number of bytes available for reading. */
long RingBuffer_GetReadAvailable( RingBuffer *rbuf )
{
    return ( (rbuf->writeIndex - rbuf->readIndex) & rbuf->bigMask );
}
/***************************************************************************
** Return number of bytes available for writing. */
long RingBuffer_GetWriteAvailable( RingBuffer *rbuf )
{
    return ( rbuf->bufferSize - RingBuffer_GetReadAvailable(rbuf));
}


/***************************************************************************
** Get address of region(s) to which we can write data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be written or numBytes, whichever is smaller.
*/
long RingBuffer_GetWriteRegions( RingBuffer *rbuf, long numBytes,
                                 void **dataPtr1, long *sizePtr1,
                                 void **dataPtr2, long *sizePtr2 )
{
    long   index;
    long   available = RingBuffer_GetWriteAvailable( rbuf );
    if( numBytes > available ) numBytes = available;
    /* Check to see if write is not contiguous. */
    index = rbuf->writeIndex & rbuf->smallMask;
    if( (index + numBytes) > rbuf->bufferSize )
    {
        /* Write data in two blocks that wrap the buffer. */
        long   firstHalf = rbuf->bufferSize - index;
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = firstHalf;
        *dataPtr2 = &rbuf->buffer[0];
        *sizePtr2 = numBytes - firstHalf;
    }
    else
    {
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = numBytes;
        *dataPtr2 = NULL;
        *sizePtr2 = 0;
    }
    return numBytes;
}


/***************************************************************************
*/
long RingBuffer_AdvanceWriteIndex( RingBuffer *rbuf, long numBytes )
{
    return rbuf->writeIndex = (rbuf->writeIndex + numBytes) & rbuf->bigMask;
}

/***************************************************************************
** Get address of region(s) from which we can read data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be written or numBytes, whichever is smaller.
*/
long RingBuffer_GetReadRegions( RingBuffer *rbuf, long numBytes,
                                void **dataPtr1, long *sizePtr1,
                                void **dataPtr2, long *sizePtr2 )
{
    long   index;
    long   available = RingBuffer_GetReadAvailable( rbuf );
    if( numBytes > available ) numBytes = available;
    /* Check to see if read is not contiguous. */
    index = rbuf->readIndex & rbuf->smallMask;
    if( (index + numBytes) > rbuf->bufferSize )
    {
        /* Write data in two blocks that wrap the buffer. */
        long firstHalf = rbuf->bufferSize - index;
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = firstHalf;
        *dataPtr2 = &rbuf->buffer[0];
        *sizePtr2 = numBytes - firstHalf;
    }
    else
    {
        *dataPtr1 = &rbuf->buffer[index];
        *sizePtr1 = numBytes;
        *dataPtr2 = NULL;
        *sizePtr2 = 0;
    }
    return numBytes;
}
/***************************************************************************
*/
long RingBuffer_AdvanceReadIndex( RingBuffer *rbuf, long numBytes )
{
    return rbuf->readIndex = (rbuf->readIndex + numBytes) & rbuf->bigMask;
}

/***************************************************************************
** Return bytes written. */
long RingBuffer_Write( RingBuffer *rbuf, void *data, long numBytes )
{
    long size1, size2, numWritten;
    void *data1, *data2;
    numWritten = RingBuffer_GetWriteRegions( rbuf, numBytes, &data1, &size1, &data2, &size2 );
    if( size2 > 0 )
    {

        memcpy( data1, data, size1 );
        data = ((char *)data) + size1;
        memcpy( data2, data, size2 );
    }
    else
    {
        memcpy( data1, data, size1 );
    }
    RingBuffer_AdvanceWriteIndex( rbuf, numWritten );
    return numWritten;
}

/***************************************************************************
** Return bytes read. */
long RingBuffer_Read( RingBuffer *rbuf, void *data, long numBytes )
{
    long size1, size2, numRead;
    void *data1, *data2;
    numRead = RingBuffer_GetReadRegions( rbuf, numBytes, &data1, &size1, &data2, &size2 );
    if( size2 > 0 )
    {
        memcpy( data, data1, size1 );
        data = ((char *)data) + size1;
        memcpy( data, data2, size2 );
    }
    else
    {
        memcpy( data, data1, size1 );
    }
    RingBuffer_AdvanceReadIndex( rbuf, numRead );
    return numRead;
}


/************************************************************************/
/******** Functions *****************************************************/
/************************************************************************/

/* Called from PortAudio.
 * Read and write data only if there is room in FIFOs.
 */
static int audioIOCallback( void *inputBuffer, void *outputBuffer,
                               unsigned long framesPerBuffer,
                               PaTimestamp outTime, void *userData )
{
    PASTREAMIO_Stream *data = (PASTREAMIO_Stream*)userData;
    long numBytes = data->bytesPerFrame * framesPerBuffer;
    (void) outTime;
    (void) inputBuffer;

    if( outputBuffer != NULL )
    {
        int i;
        int numRead = RingBuffer_Read( &data->outFIFO, outputBuffer, numBytes );
        /* Zero out remainder of buffer if we run out of data. */
        for( i=numRead; i<numBytes; i++ )
        {
            ((char *)outputBuffer)[i] = 0;
        }
    }

    return 0;
}

/* Allocate buffer. */
static PaError PASTREAMIO_InitFIFO( RingBuffer *rbuf, long numFrames, long bytesPerFrame )
{
    long numBytes = numFrames * bytesPerFrame;
    char *buffer = (char *) malloc( numBytes );
    if( buffer == NULL ) return paInsufficientMemory;
    memset( buffer, 0, numBytes );
    return (PaError) RingBuffer_Init( rbuf, numBytes, buffer );
}

/* Free buffer. */
static PaError PASTREAMIO_TermFIFO( RingBuffer *rbuf )
{
    if( rbuf->buffer ) free( rbuf->buffer );
    rbuf->buffer = NULL;
    return paNoError;
}

/************************************************************
 * Write data to ring buffer.
 * Will not return until all the data has been written.
 */
long WriteAudioStream( PASTREAMIO_Stream *aStream, void *data, long numFrames )
{
    long bytesWritten;
    char *p = (char *) data;
    long numBytes = aStream->bytesPerFrame * numFrames;
    while( numBytes > 0)
    {
        bytesWritten = RingBuffer_Write( &aStream->outFIFO, p, numBytes );
        numBytes -= bytesWritten;
        p += bytesWritten;
        if( numBytes > 0) Pa_Sleep(10);
    }
    return numFrames;
}


/************************************************************
 * Return the number of frames that could be written to the stream without
 * having to wait.
 */
long GetAudioStreamWriteable( PASTREAMIO_Stream *aStream )
{
    int bytesEmpty = RingBuffer_GetWriteAvailable( &aStream->outFIFO );
    return bytesEmpty / aStream->bytesPerFrame;
}



/************************************************************/
unsigned long RoundUpToNextPowerOf2( unsigned long n )
{
    long numBits = 0;
    if( ((n-1) & n) == 0) return n; /* Already Power of two. */
    while( n > 0 )
    {
        n= n>>1;
        numBits++;
    }
    return (1<<numBits);
}

/* forward prototype */
PaError CloseAudioStream( PASTREAMIO_Stream *aStream );

/************************************************************
 * Opens a PortAudio stream with default characteristics.
 * Allocates PASTREAMIO_Stream structure.
 *
 * flags parameter can be an ORed combination of:
 *    PABLIO_WRITE,
 *    and either PABLIO_MONO or PABLIO_STEREO
 */
PaError OpenAudioStream( PASTREAMIO_Stream **rwblPtr, double sampleRate,
                         PaSampleFormat format, long flags )
{
    long   bytesPerSample;
    long   doWrite = 0;
    PaError err;
    PASTREAMIO_Stream *aStream;
    long   minNumBuffers;
    long   numFrames;

    /* Allocate PASTREAMIO_Stream structure for caller. */
    aStream = (PASTREAMIO_Stream *) malloc( sizeof(PASTREAMIO_Stream) );
    if( aStream == NULL ) return paInsufficientMemory;
    memset( aStream, 0, sizeof(PASTREAMIO_Stream) );

    /* Determine size of a sample. */
    bytesPerSample = Pa_GetSampleSize( format );
    if( bytesPerSample < 0 )
    {
        err = (PaError) bytesPerSample;
        goto error;
    }
    aStream->samplesPerFrame = ((flags&PASTREAMIO_MONO) != 0) ? 1 : 2;
    aStream->bytesPerFrame = bytesPerSample * aStream->samplesPerFrame;

    /* Initialize PortAudio  */
    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    /* Warning: numFrames must be larger than amount of data processed per interrupt
     *    inside PA to prevent glitches. Just to be safe, adjust size upwards.
     */
    minNumBuffers = 2 * Pa_GetMinNumBuffers( FRAMES_PER_BUFFER, sampleRate );
    numFrames = minNumBuffers * FRAMES_PER_BUFFER;
    numFrames = RoundUpToNextPowerOf2( numFrames );

    /* Initialize Ring Buffer */
    doWrite = ((flags & PASTREAMIO_WRITE) != 0);

    if(doWrite)
    {
        err = PASTREAMIO_InitFIFO( &aStream->outFIFO, numFrames, aStream->bytesPerFrame );
        if( err != paNoError ) goto error;
        /* Make Write FIFO appear full initially.
        numBytes = RingBuffer_GetWriteAvailable( &aStream->outFIFO );
        RingBuffer_AdvanceWriteIndex( &aStream->outFIFO, numBytes );*/
    }

    /* Open a PortAudio stream that we will use to communicate with the underlying
     * audio drivers. */
    err = Pa_OpenStream(
              &aStream->stream,
              paNoDevice,
              0 ,
              format,
              NULL,
              Pa_GetDefaultOutputDeviceID() ,
              aStream->samplesPerFrame ,
              format,
              NULL,
              sampleRate,
              FRAMES_PER_BUFFER,
              minNumBuffers,
              paClipOff,       /* we won't output out of range samples so don't bother clipping them */
              audioIOCallback,
              aStream );
    if( err != paNoError ) goto error;

    *rwblPtr = aStream;
    return paNoError;

error:
    CloseAudioStream( aStream );
    *rwblPtr = NULL;
    return err;
}

PaError StartAudioStream( PASTREAMIO_Stream *aStream)
{
	PaError err;
	err = Pa_StartStream( aStream->stream );
    if( err != paNoError ) goto error;

    return paNoError;
error:
    CloseAudioStream( aStream );
    return err;
}


/************************************************************/
PaError CloseAudioStream( PASTREAMIO_Stream *aStream )
{
    PaError err;
    int bytesEmpty;
    int byteSize = aStream->outFIFO.bufferSize;

    /* If we are writing data, make sure we play everything written. */
    if( byteSize > 0 )
    {
        bytesEmpty = RingBuffer_GetWriteAvailable( &aStream->outFIFO );
        while( bytesEmpty < byteSize )
        {
            Pa_Sleep( 10 );
            bytesEmpty = RingBuffer_GetWriteAvailable( &aStream->outFIFO );
        }
    }

    err = Pa_StopStream( aStream->stream );
    if( err != paNoError ) goto error;
    err = Pa_CloseStream( aStream->stream );
    if( err != paNoError ) goto error;
    Pa_Terminate();

error:
    PASTREAMIO_TermFIFO( &aStream->outFIFO );
    free( aStream );
    return err;
}

/* -- end of portaudio specific routines --*/

/* portaudio related global types  */
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)

PASTREAMIO_Stream  *aOutStream; /* our modified stream buffer*/
SAMPLE      *samples; /*local buffer for samples*/
double latency_sec = 0;

/* ticks information to be used if the audio stream is not present */
int    currentTicks = -1;

/* initial state of the audio stream */
int isPlaying = 0;
PaError err;

/* Ogg and codec state for demux/decode */
ogg_sync_state   oy;
ogg_page         og;
ogg_stream_state vo;
ogg_stream_state to;
theora_info      ti;
theora_comment   tc;
theora_state     td;
vorbis_info      vi;
vorbis_dsp_state vd;
vorbis_block     vb;
vorbis_comment   vc;

int              theora_p=0;
int              vorbis_p=0;
int              stateflag=0;

FILE * infile = NULL;

/* SDL Video playback structures */
SDL_Surface *screen;
SDL_Overlay *yuv_overlay;
SDL_Rect rect;

/* single frame video buffering */
int          videobuf_ready=0;
ogg_int64_t  videobuf_granulepos=-1;
double       videobuf_time=0;

int          audiobuf_ready=0;
ogg_int64_t  audiobuf_granulepos=0; /* time position of last sample */

static int open_audio(){
    /* this will open one circular audio stream */
    /* build on top of portaudio routines */
    /* implementation based on file pastreamio.c */

    int numSamples;
    int numBytes;

    int minNumBuffers;
    int numFrames;

    minNumBuffers = 2 * Pa_GetMinNumBuffers( FRAMES_PER_BUFFER, vi.rate );
    numFrames = minNumBuffers * FRAMES_PER_BUFFER;
    numFrames = RoundUpToNextPowerOf2( numFrames );

    numSamples = numFrames * vi.channels;
    numBytes = numSamples * sizeof(SAMPLE);

    samples = (SAMPLE *) malloc( numBytes );

    /* store our latency calculation here */
    latency_sec =  (double) numFrames / vi.rate / vi.channels;
    printf( "Latency: %.04f\n", latency_sec );

    err = OpenAudioStream( &aOutStream, vi.rate, PA_SAMPLE_TYPE,
                           (PASTREAMIO_WRITE | PASTREAMIO_STEREO) );
    if( err != paNoError ) goto error;
    return err;
error:
    CloseAudioStream( aOutStream );
    printf( "An error occured while opening the portaudio stream\n" );
    printf( "Error number: %d\n", err );
    printf( "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;

}

static int start_audio(){
    err = StartAudioStream(aOutStream);
    if( err != paNoError ) goto error;

    return err;
error:
    CloseAudioStream( aOutStream );
    printf( "An error occured while opening the portaudio stream\n" );
    printf( "Error number: %d\n", err );
    printf( "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}

static int audio_close(void){
    err = CloseAudioStream( aOutStream );
    if( err != paNoError ) goto error;

    free(samples);
    return err;
error:
    Pa_Terminate();
    printf( "An error occured while closing the portaudio stream\n" );
    printf( "Error number: %d\n", err );
    printf( "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}


double get_time() {
    static Uint32 startticks = 0;
    double curtime;
    if (vorbis_p) {
      /* not entirely accurate with the WAVE OUT device, but good enough
         at this stage. Needs to be reworked to account for blank audio
         data written to the stream... */
      curtime = (double) (GetAudioStreamTime( aOutStream ) / vi.rate) - latency_sec;
      if (curtime<0.0) curtime = 0.0;
    } else {
      /* initialize timer variable if not set yet */
      if (startticks==0)
        startticks = SDL_GetTicks();
      curtime = 1.0e-3 * (double)(SDL_GetTicks() - startticks);
    }
    return curtime;
}


static void open_video(void){
  /* taken from player_sample.c test file for theora alpha */

  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
    printf("Unable to initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  screen = SDL_SetVideoMode(ti.frame_width, ti.frame_height, 0, SDL_SWSURFACE);
  if ( screen == NULL ) {
    printf("Unable to set %dx%d video mode: %s\n",
           ti.frame_width,ti.frame_height,SDL_GetError());
    exit(1);
  }

  yuv_overlay = SDL_CreateYUVOverlay(ti.frame_width, ti.frame_height,
				     SDL_YV12_OVERLAY,
				     screen);
  if ( yuv_overlay == NULL ) {
    printf("SDL: Couldn't create SDL_yuv_overlay: %s\n",
	   SDL_GetError());
    exit(1);
  }
  rect.x = 0;
  rect.y = 0;
  rect.w = ti.frame_width;
  rect.h = ti.frame_height;

  SDL_DisplayYUVOverlay(yuv_overlay, &rect);
}

static void video_write(void){
  /* taken from player_sample.c test file for theora alpha */
  int i;
  yuv_buffer yuv;
  int crop_offset;
  theora_decode_YUVout(&td,&yuv);

  /* Lock SDL_yuv_overlay */
  if ( SDL_MUSTLOCK(screen) ) {
    if ( SDL_LockSurface(screen) < 0 ) return;
  }
  if (SDL_LockYUVOverlay(yuv_overlay) < 0) return;

  /* let's draw the data (*yuv[3]) on a SDL screen (*screen) */
  /* deal with border stride */
  /* reverse u and v for SDL */
  /* and crop input properly, respecting the encoded frame rect */
  crop_offset=ti.offset_x+yuv.y_stride*ti.offset_y;
  for(i=0;i<yuv_overlay->h;i++)
    memcpy(yuv_overlay->pixels[0]+yuv_overlay->pitches[0]*i,
	   yuv.y+crop_offset+yuv.y_stride*i,
	   yuv_overlay->w);
  crop_offset=(ti.offset_x/2)+(yuv.uv_stride)*(ti.offset_y/2);
  for(i=0;i<yuv_overlay->h/2;i++){
    memcpy(yuv_overlay->pixels[1]+yuv_overlay->pitches[1]*i,
	   yuv.v+crop_offset+yuv.uv_stride*i,
	   yuv_overlay->w/2);
    memcpy(yuv_overlay->pixels[2]+yuv_overlay->pitches[2]*i,
	   yuv.u+crop_offset+yuv.uv_stride*i,
	   yuv_overlay->w/2);
  }

  /* Unlock SDL_yuv_overlay */
  SDL_UnlockYUVOverlay(yuv_overlay);
  if ( SDL_MUSTLOCK(screen) ) {
    SDL_UnlockSurface(screen);
  }

  /* Show, baby, show! */
  SDL_DisplayYUVOverlay(yuv_overlay, &rect);
}

static void usage(void){
  printf("Usage: splayer <ogg_file>\n"
#ifdef WIN32
    "\n"
    "or drag and drop an ogg file over the .exe\n\n"
#endif
  );
}

/* dump the theora (or vorbis) comment header */
static int dump_comments(theora_comment *tc){
  int i, len;
  char *value;

  printf("Encoded by %s\n",tc->vendor);
  if(tc->comments){
    printf("theora comment header:\n");
    for(i=0;i<tc->comments;i++){
      if(tc->user_comments[i]){
        len=tc->comment_lengths[i];
      	value=malloc(len+1);
      	memcpy(value,tc->user_comments[i],len);
      	value[len]='\0';
      	printf("\t%s\n", value);
      	free(value);
      }
    }
  }
  return(0);
}

/* Report the encoder-specified colorspace for the video, if any.
   We don't actually make use of the information in this example;
   a real player should attempt to perform color correction for
   whatever display device it supports. */
static void report_colorspace(theora_info *ti)
{
    switch(ti->colorspace){
      case OC_CS_UNSPECIFIED:
        /* nothing to report */
        break;;
      case OC_CS_ITU_REC_470M:
        fprintf(stderr,"  encoder specified ITU Rec 470M color.\n");
        break;;
      case OC_CS_ITU_REC_470BG:
        fprintf(stderr,"  encoder specified ITU Rec 470BG color.\n");
        break;;
      default:
        fprintf(stderr,"warning: encoder specified unknown colorspace (%d).\n",
            ti->colorspace);
        break;;
    }
}

/* Helper; just grab some more compressed bitstream and sync it for
   page extraction */
int buffer_data(ogg_sync_state *oy){
  char *buffer=ogg_sync_buffer(oy,4096);
  int bytes=fread(buffer,1,4096,infile);
  ogg_sync_wrote(oy,bytes);
  return(bytes);
}

/* helper: push a page into the appropriate stream */
/* this can be done blindly; a stream won't accept a page
                that doesn't belong to it */
static int queue_page(ogg_page *page){
  if(theora_p)ogg_stream_pagein(&to,page);
  if(vorbis_p)ogg_stream_pagein(&vo,page);
  return 0;
}

void parseHeaders(){
  /* extracted from player_sample.c test file for theora alpha */
  ogg_packet op;
  /* Parse the headers */
  /* Only interested in Vorbis/Theora streams */
  while(!stateflag){
    int ret=buffer_data(&oy);
    if(ret==0)break;
    while(ogg_sync_pageout(&oy,&og)>0){
      ogg_stream_state test;

      /* is this a mandated initial header? If not, stop parsing */
      if(!ogg_page_bos(&og)){
	/* don't leak the page; get it into the appropriate stream */
	queue_page(&og);
	stateflag=1;
	break;
      }

      ogg_stream_init(&test,ogg_page_serialno(&og));
      ogg_stream_pagein(&test,&og);
      ogg_stream_packetout(&test,&op);

      /* identify the codec: try theora */
      if(!theora_p && theora_decode_header(&ti,&tc,&op)>=0){
	/* it is theora */
	memcpy(&to,&test,sizeof(test));
	theora_p=1;
      }else if(!vorbis_p && vorbis_synthesis_headerin(&vi,&vc,&op)>=0){
	/* it is vorbis */
	memcpy(&vo,&test,sizeof(test));
	vorbis_p=1;
      }else{
	/* whatever it is, we don't care about it */
	ogg_stream_clear(&test);
      }
    }
  }

  /* we've now identified all the bitstreams. parse the secondary header packets. */
  while((theora_p && theora_p<3) || (vorbis_p && vorbis_p<3)){
    int ret;

    /* look for further theora headers */
    while(theora_p && (theora_p<3) && (ret=ogg_stream_packetout(&to,&op))){
      if(ret<0){
      	printf("Error parsing Theora stream headers; corrupt stream?\n");
      	exit(1);
      }
      if(theora_decode_header(&ti,&tc,&op)){
        printf("Error parsing Theora stream headers; corrupt stream?\n");
        exit(1);
      }
      theora_p++;
      if(theora_p==3)break;
    }

    /* look for more vorbis header packets */
    while(vorbis_p && (vorbis_p<3) && (ret=ogg_stream_packetout(&vo,&op))){
      if(ret<0){
	printf("Error parsing Vorbis stream headers; corrupt stream?\n");
	exit(1);
      }
      if(vorbis_synthesis_headerin(&vi,&vc,&op)){
	printf("Error parsing Vorbis stream headers; corrupt stream?\n");
	exit(1);
      }
      vorbis_p++;
      if(vorbis_p==3)break;
    }

    /* The header pages/packets will arrive before anything else we
       care about, or the stream is not obeying spec */

    if(ogg_sync_pageout(&oy,&og)>0){
      queue_page(&og); /* demux into the appropriate stream */
    }else{
      int ret=buffer_data(&oy);
      if(ret==0){
	fprintf(stderr,"End of file while searching for codec headers.\n");
	exit(1);
      }
    }
  }
}

int main( int argc, char* argv[] ){

  int i,j;
  ogg_packet op;
  SDL_Event event;
  int hasdatatobuffer = 1;
  int playbackdone = 0;
  double now, delay, last_frame_time = 0;

  int frameNum=0;
  int skipNum=0;

  /* takes first argument as file to play */
  /* this works better on Windows and is more convenient
     for drag and drop ogg files over the .exe */

  if( argc != 2 )
  {
    usage();
    exit(0);
  }

  infile  = fopen( argv[1], "rb" );

  /* start up Ogg stream synchronization layer */
  ogg_sync_init(&oy);

  /* init supporting Vorbis structures needed in header parsing */
  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);

  /* init supporting Theora structures needed in header parsing */
  theora_comment_init(&tc);
  theora_info_init(&ti);

  parseHeaders();

  /* force audio off */
  /* vorbis_p = 0; */

  /* initialize decoders */
  if(theora_p){
    theora_decode_init(&td,&ti);
    printf("Ogg logical stream %x is Theora %dx%d %.02f fps video\n"
           "  Frame content is %dx%d with offset (%d,%d).\n",
	   to.serialno,ti.width,ti.height, (double)ti.fps_numerator/ti.fps_denominator,
	   ti.frame_width, ti.frame_height, ti.offset_x, ti.offset_y);
    report_colorspace(&ti);
    dump_comments(&tc);
  }else{
    /* tear down the partial theora setup */
    theora_info_clear(&ti);
    theora_comment_clear(&tc);
  }
  if(vorbis_p){
    vorbis_synthesis_init(&vd,&vi);
    vorbis_block_init(&vd,&vb);
    printf("Ogg logical stream %x is Vorbis %d channel %d Hz audio.\n",
	   vo.serialno,vi.channels,vi.rate);
  }else{
    /* tear down the partial vorbis setup */
    vorbis_info_clear(&vi);
    vorbis_comment_clear(&vc);
  }
  /* open audio */
  if(vorbis_p)open_audio();
  /* open video */
  if(theora_p)open_video();

  /* our main loop */
  while(!playbackdone){

    /* break out on SDL quit event */
    if ( SDL_PollEvent ( &event ) )
    {
      if ( event.type == SDL_QUIT ) break ;
    }

    /* get some audio data */
    while(vorbis_p && !audiobuf_ready){
      int ret;
      float **pcm;
      int count = 0;
      int maxBytesToWrite;

      /* is there pending audio? does it fit our circular buffer without blocking? */
      ret=vorbis_synthesis_pcmout(&vd,&pcm);
      maxBytesToWrite = GetAudioStreamWriteable(aOutStream);

      if (maxBytesToWrite<=FRAMES_PER_BUFFER){
        /* break out until there is a significant amount of
           data to avoid a series of small write operations. */
        break;
      }
      /* if there's pending, decoded audio, grab it */
      if((ret>0)&&(maxBytesToWrite>0)){

	for(i=0;i<ret && i<(maxBytesToWrite/vi.channels);i++)
	  for(j=0;j<vi.channels;j++){
	    int val=(int)(pcm[j][i]*32767.f);
	    if(val>32767)val=32767;
	    if(val<-32768)val=-32768;
	    samples[count]=val;
	    count++;
	  }
	if(WriteAudioStream( aOutStream, samples, i )) {
	  if(count==maxBytesToWrite){
	    audiobuf_ready=1;
	  }
	}
        vorbis_synthesis_read(&vd,i);

	if(vd.granulepos>=0)
	  audiobuf_granulepos=vd.granulepos-ret+i;
	else
	  audiobuf_granulepos+=i;

      }else{

	/* no pending audio; is there a pending packet to decode? */
	if(ogg_stream_packetout(&vo,&op)>0){
	  if(vorbis_synthesis(&vb,&op)==0) /* test for success! */
	   vorbis_synthesis_blockin(&vd,&vb);
	}else	/* we need more data; break out to suck in another page */
	  break;
      }
    } /* end audio cycle */

    while(theora_p && !videobuf_ready){
      /* get one video packet... */
      if(ogg_stream_packetout(&to,&op)>0){

        theora_decode_packetin(&td,&op);

	  videobuf_granulepos=td.granulepos;
	  videobuf_time=theora_granule_time(&td,videobuf_granulepos);
	  /* update the frame counter */
	  frameNum++;

	  /* check if this frame time has not passed yet.
	     If the frame is late we need to decode additonal
	     ones and keep looping, since theora at this stage
	     needs to decode all frames */
	  now=get_time();
	  delay=videobuf_time-now;
	  if(delay>=0.0){
		/* got a good frame, not late, ready to break out */
		videobuf_ready=1;
	  }else if(now-last_frame_time>=1.0){
		/* display at least one frame per second, regardless */
		videobuf_ready=1;
	  }else{
		fprintf(stderr, "dropping frame %d (%.3fs behind)\n",
			frameNum, -delay);
	   }
      }else{
	/* need more data */
	break;
      }
    }

    if(!hasdatatobuffer && !videobuf_ready && !audiobuf_ready){
      isPlaying = 0;
      playbackdone = 1;
    }

    /* if we're set for the next frame, sleep */
    if((!theora_p || videobuf_ready) &&
       (!vorbis_p || audiobuf_ready)){
        int ticks = 1.0e3*(videobuf_time-get_time());
	if(ticks>0)
          SDL_Delay(ticks);
    }

    if(videobuf_ready){
      /* time to write our cached frame */
      video_write();
      videobuf_ready=0;
      last_frame_time=get_time();

      /* if audio has not started (first frame) then start it */
      if ((!isPlaying)&&(vorbis_p)){
        start_audio();
        isPlaying = 1;
      }
    }

    /* HACK: always look for more audio data */
    audiobuf_ready=0;

    /* buffer compressed data every loop */
    if(hasdatatobuffer){
      hasdatatobuffer=buffer_data(&oy);
      if(hasdatatobuffer==0){
        printf("Ogg buffering stopped, end of file reached.\n");
      }
    }

    if (ogg_sync_pageout(&oy,&og)>0){
      queue_page(&og);
    }

  } /* playbackdone */

  /* show number of video frames decoded */
  printf( "\n");
  printf( "Frames decoded: %d", frameNum );
  if(skipNum)
    printf( " (only %d shown)", frameNum-skipNum);
  printf( "\n" );

  /* tear it all down */
  fclose( infile );

  if(vorbis_p){
    audio_close();

    ogg_stream_clear(&vo);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
  }
  if(theora_p){
    ogg_stream_clear(&to);
    theora_clear(&td);
    theora_comment_clear(&tc);
    theora_info_clear(&ti);
  }
  ogg_sync_clear(&oy);

  printf("\r                                                              "
	 "\nDone.\n");

  SDL_Quit();

  return(0);

}
