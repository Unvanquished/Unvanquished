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

/*****************************************************************************
 * name:    cl_cin.c
 *
 * desc:    video and cinematic playback
 *
 *****************************************************************************/

//#define ADAPTED_TO_STREAMING_SOUND
//  (SA) MISSIONPACK MERGE
//  s_rawend[0] for wolf is [] and for q3 is just a single value
//  I need to ask Ryan if it's as simple as a constant index or
// if some more coding needs to be done.

#include "client.h"

#include "framework/CommandSystem.h"

#define MAXSIZE            8
#define MINSIZE            4

#define DEFAULT_CIN_WIDTH  512
#define DEFAULT_CIN_HEIGHT 512

#define ROQ_QUAD           0x1000
#define ROQ_QUAD_INFO      0x1001
#define ROQ_CODEBOOK       0x1002
#define ROQ_QUAD_VQ        0x1011
#define ROQ_QUAD_JPEG      0x1012
#define ROQ_QUAD_HANG      0x1013
#define ROQ_PACKET         0x1030
#define ZA_SOUND_MONO      0x1020
#define ZA_SOUND_STEREO    0x1021

#define MAX_VIDEO_HANDLES  16

extern int  s_soundtime;

static void RoQ_init();

/******************************************************************************
*
* Class:    trFMV
*
* Description:  RoQ/RnR manipulation routines
*       not entirely complete for first run
*
******************************************************************************/

static long           ROQ_YY_tab[ 256 ];
static long           ROQ_UB_tab[ 256 ];
static long           ROQ_UG_tab[ 256 ];
static long           ROQ_VG_tab[ 256 ];
static long           ROQ_VR_tab[ 256 ];
static unsigned short vq2[ 256 * 16 * 4 ];
static unsigned short vq4[ 256 * 64 * 4 ];
static unsigned short vq8[ 256 * 256 * 4 ];

typedef enum
{
  FT_ROQ = 0, // roq (vq3 stuff)
  FT_OGM // ogm (ogg wrapper, vorbis audio, xvid/theora video) for WoP
} filetype_t;

typedef struct
{
	byte  linbuf[ DEFAULT_CIN_WIDTH *DEFAULT_CIN_HEIGHT * 4 * 2 ];
	byte  file[ 65536 ];
	short sqrTable[ 256 ];

	int   mcomp[ 256 ];
	byte  *qStatus[ 2 ][ 32768 ];

	long  oldXOff, oldYOff, oldysize, oldxsize;

	int   currentHandle;
} cinematics_t;

typedef struct
{
	char         fileName[ MAX_OSPATH ];
	int          CIN_WIDTH, CIN_HEIGHT;
	int          xpos, ypos, width, height;
	bool     looping, holdAtEnd, dirty, alterGameState, silent, shader;
	fileHandle_t iFile;
	e_status     status;
	int          startTime;
	int          lastTime;
	long         tfps;
	long         RoQPlayed;
	long         ROQSize;
	unsigned int RoQFrameSize;
	long         onQuad;
	long         numQuads;
	long         samplesPerLine;
	unsigned int roq_id;
	long         screenDelta;

	void ( *VQ0 )( byte *status, void *qdata );
	void ( *VQ1 )( byte *status, void *qdata );
	void ( *VQNormal )( byte *status, void *qdata );
	void ( *VQBuffer )( byte *status, void *qdata );

	long         samplesPerPixel; // defaults to 2
	byte         *gray;
	unsigned int xsize, ysize, maxsize, minsize;

	bool     half, smootheddouble, inMemory;
	long         normalBuffer0;
	long         roq_flags;
	long         roqF0;
	long         roqF1;
	long         t[ 2 ];
	long         roqFPS;
	int          playonwalls;
	byte         *buf;
	long         drawX, drawY;
	filetype_t   fileType;
} cin_cache;

static cinematics_t cin;
static cin_cache    cinTable[ MAX_VIDEO_HANDLES ];
static int          currentHandle = -1;
static int          CL_handle = -1;

void CIN_CloseAllVideos()
{
	int i;

	for ( i = 0; i < MAX_VIDEO_HANDLES; i++ )
	{
		if ( cinTable[ i ].fileName[ 0 ] != 0 )
		{
			CIN_StopCinematic( i );
		}
	}
}

static int CIN_HandleForVideo()
{
	int i;

	for ( i = 0; i < MAX_VIDEO_HANDLES; i++ )
	{
		if ( cinTable[ i ].fileName[ 0 ] == 0 )
		{
			return i;
		}
	}

	Com_Error( ERR_DROP, "CIN_HandleForVideo: none free" );
}

extern int CL_ScaledMilliseconds();

//-----------------------------------------------------------------------------
// RllSetupTable
//
// Allocates and initializes the square table.
//
// Parameters:  None
//
// Returns:     Nothing
//-----------------------------------------------------------------------------
static void RllSetupTable()
{
	int z;

	for ( z = 0; z < 128; z++ )
	{
		cin.sqrTable[ z ] = ( short )( z * z );
		cin.sqrTable[ z + 128 ] = -cin.sqrTable[ z ];
	}
}

//-----------------------------------------------------------------------------
// RllDecodeMonoToStereo
//
// Decode mono source data into a stereo buffer. Output is 4 times the number
// of bytes in the input.
//
// Parameters:  from -> buffer holding encoded data
//              to ->   buffer to hold decoded data
//              size =  number of bytes of input (= 1/4 # of bytes of output)
//              signedOutput = 0 for unsigned output, non-zero for signed output
//              flag = flags from asset header
//
// Returns:     Number of samples placed in output buffer
//-----------------------------------------------------------------------------
long RllDecodeMonoToStereo( unsigned char *from, short *to, unsigned int size, char signedOutput, unsigned short flag )
{
	unsigned int z;
	int          prev;

	if ( signedOutput )
	{
		prev = flag - 0x8000;
	}
	else
	{
		prev = flag;
	}

	for ( z = 0; z < size; z++ )
	{
		prev = ( short )( prev + cin.sqrTable[ from[ z ] ] );
		to[ z * 2 + 0 ] = to[ z * 2 + 1 ] = ( short )( prev );
	}

	return size; // * 2 * sizeof(short));
}

//-----------------------------------------------------------------------------
// RllDecodeStereoToStereo
//
// Decode stereo source data into a stereo buffer.
//
// Parameters:  from -> buffer holding encoded data
//              to ->   buffer to hold decoded data
//              size =  number of bytes of input (= 1/2 # of bytes of output)
//              signedOutput = 0 for unsigned output, non-zero for signed output
//              flag = flags from asset header
//
// Returns:     Number of samples placed in output buffer
//-----------------------------------------------------------------------------
long RllDecodeStereoToStereo( unsigned char *from, short *to, unsigned int size, char signedOutput, unsigned short flag )
{
	unsigned int  z;
	unsigned char *zz = from;
	int           prevL, prevR;

	if ( signedOutput )
	{
		prevL = ( flag & 0xff00 ) - 0x8000;
		prevR = ( ( flag & 0x00ff ) << 8 ) - 0x8000;
	}
	else
	{
		prevL = flag & 0xff00;
		prevR = ( flag & 0x00ff ) << 8;
	}

	for ( z = 0; z < size; z += 2 )
	{
		prevL = ( short )( prevL + cin.sqrTable[ *zz++ ] );
		prevR = ( short )( prevR + cin.sqrTable[ *zz++ ] );
		to[ z + 0 ] = ( short )( prevL );
		to[ z + 1 ] = ( short )( prevR );
	}

	return ( size >> 1 ); //*sizeof(short));
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void move8_32( byte *src, byte *dst, int spl )
{
	int i;

	for ( i = 0; i < 8; ++i )
	{
		memcpy( dst, src, 32 );
		src += spl;
		dst += spl;
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void move4_32( byte *src, byte *dst, int spl )
{
	int i;

	for ( i = 0; i < 4; ++i )
	{
		memcpy( dst, src, 16 );
		src += spl;
		dst += spl;
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void blit8_32( byte *src, byte *dst, int spl )
{
	int i;

	for ( i = 0; i < 8; ++i )
	{
		memcpy( dst, src, 32 );
		src += 32;
		dst += spl;
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/
static void blit4_32( byte *src, byte *dst, int spl )
{
	int i;

	for ( i = 0; i < 4; ++i )
	{
		memmove( dst, src, 16 );
		src += 16;
		dst += spl;
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void blit2_32( byte *src, byte *dst, int spl )
{
	memcpy( dst, src, 8 );
	memcpy( dst + spl, src + 8, 8 );
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void blitVQQuad32fs( byte **status, unsigned char *data )
{
	unsigned short newd, celdata, code;
	unsigned int   index, i;
	int            spl;

	newd = 0;
	celdata = 0;
	index = 0;

	spl = cinTable[ currentHandle ].samplesPerLine;

	do
	{
		if ( !newd )
		{
			newd = 7;
			celdata = data[ 0 ] + data[ 1 ] * 256;
			data += 2;
		}
		else
		{
			newd--;
		}

		code = ( unsigned short )( celdata & 0xc000 );
		celdata <<= 2;

		switch ( code )
		{
			case    0x8000: // vq code
				blit8_32( ( byte * ) &vq8[( *data ) * 128 ], status[ index ], spl );
				data++;
				index += 5;
				break;

			case    0xc000: // drop
				index++; // skip 8x8

				for ( i = 0; i < 4; i++ )
				{
					if ( !newd )
					{
						newd = 7;
						celdata = data[ 0 ] + data[ 1 ] * 256;
						data += 2;
					}
					else
					{
						newd--;
					}

					code = ( unsigned short )( celdata & 0xc000 );
					celdata <<= 2;

					switch ( code ) // code in top two bits of code
					{
						case    0x8000: // 4x4 vq code
							blit4_32( ( byte * ) &vq4[( *data ) * 32 ], status[ index ], spl );
							data++;
							break;

						case    0xc000: // 2x2 vq code
							blit2_32( ( byte * ) &vq2[( *data ) * 8 ], status[ index ], spl );
							data++;
							blit2_32( ( byte * ) &vq2[( *data ) * 8 ], status[ index ] + 8, spl );
							data++;
							blit2_32( ( byte * ) &vq2[( *data ) * 8 ], status[ index ] + spl * 2, spl );
							data++;
							blit2_32( ( byte * ) &vq2[( *data ) * 8 ], status[ index ] + spl * 2 + 8, spl );
							data++;
							break;

						case    0x4000: // motion compensation
							move4_32( status[ index ] + cin.mcomp[( *data ) ], status[ index ], spl );
							data++;
							break;
					}

					index++;
				}

				break;

			case    0x4000: // motion compensation
				move8_32( status[ index ] + cin.mcomp[( *data ) ], status[ index ], spl );
				data++;
				index += 5;
				break;

			case    0x0000:
				index += 5;
				break;
		}
	}
	while ( status[ index ] != nullptr );
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

void ROQ_GenYUVTables()
{
	float t_ub, t_vr, t_ug, t_vg;
	long  i;

	t_ub = ( 1.77200f / 2.0f ) * ( float )( 1 << 6 ) + 0.5f;
	t_vr = ( 1.40200f / 2.0f ) * ( float )( 1 << 6 ) + 0.5f;
	t_ug = ( 0.34414f / 2.0f ) * ( float )( 1 << 6 ) + 0.5f;
	t_vg = ( 0.71414f / 2.0f ) * ( float )( 1 << 6 ) + 0.5f;

	for ( i = 0; i < 256; i++ )
	{
		float x = ( float )( 2 * i - 255 );

		ROQ_UB_tab[ i ] = ( long )( ( t_ub * x ) + ( 1 << 5 ) );
		ROQ_VR_tab[ i ] = ( long )( ( t_vr * x ) + ( 1 << 5 ) );
		ROQ_UG_tab[ i ] = ( long )( ( -t_ug * x ) );
		ROQ_VG_tab[ i ] = ( long )( ( -t_vg * x ) + ( 1 << 5 ) );
		ROQ_YY_tab[ i ] = ( i << 6 ) | ( i >> 2 );
	}
}

#define VQ2TO4(a,b,c,d) { \
    *c++ = a[ 0 ];    \
    *d++ = a[ 0 ];    \
    *d++ = a[ 0 ];    \
    *c++ = a[ 1 ];    \
    *d++ = a[ 1 ];    \
    *d++ = a[ 1 ];    \
    *c++ = b[ 0 ];    \
    *d++ = b[ 0 ];    \
    *d++ = b[ 0 ];    \
    *c++ = b[ 1 ];    \
    *d++ = b[ 1 ];    \
    *d++ = b[ 1 ];    \
    *d++ = a[ 0 ];    \
    *d++ = a[ 0 ];    \
    *d++ = a[ 1 ];    \
    *d++ = a[ 1 ];    \
    *d++ = b[ 0 ];    \
    *d++ = b[ 0 ];    \
    *d++ = b[ 1 ];    \
    *d++ = b[ 1 ];    \
    a += 2; b += 2; }

#define VQ2TO2(a,b,c,d) { \
    *c++ = *a;      \
    *d++ = *a;      \
    *d++ = *a;      \
    *c++ = *b;      \
    *d++ = *b;      \
    *d++ = *b;      \
    *d++ = *a;      \
    *d++ = *a;      \
    *d++ = *b;      \
    *d++ = *b;      \
    a++; b++; }

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static unsigned short yuv_to_rgb( long y, long u, long v )
{
	long r, g, b, YY = ROQ_YY_tab[( y ) ];

	r = ( YY + ROQ_VR_tab[ v ] ) >> 9;
	g = ( YY + ROQ_UG_tab[ u ] + ROQ_VG_tab[ v ] ) >> 8;
	b = ( YY + ROQ_UB_tab[ u ] ) >> 9;

	if ( r < 0 ) { r = 0; }

	if ( g < 0 ) { g = 0; }

	if ( b < 0 ) { b = 0; }

	if ( r > 31 ) { r = 31; }

	if ( g > 63 ) { g = 63; }

	if ( b > 31 ) { b = 31; }

	return ( unsigned short )( ( r << 11 ) + ( g << 5 ) + ( b ) );
}

/*
Frame_yuv_to_rgb24
is used by the Theora(ogm) code

  moved the convertion into one function, to reduce the number of function-calls
*/
void Frame_yuv_to_rgb24( const unsigned char *y, const unsigned char *u, const unsigned char *v,
                         int width, int height, int y_stride, int uv_stride,
                         int yWShift, int uvWShift, int yHShift, int uvHShift, unsigned int *output )
{
	int  i, j, uvI;
	long r, g, b, YY;

	for ( j = 0; j < height; ++j )
	{
		for ( i = 0; i < width; ++i )
		{
			YY = ROQ_YY_tab[( y[( i >> yWShift ) + ( j >> yHShift ) * y_stride ] ) ];
			uvI = ( i >> uvWShift ) + ( j >> uvHShift ) * uv_stride;

			r = ( YY + ROQ_VR_tab[ v[ uvI ] ] ) >> 6;
			g = ( YY + ROQ_UG_tab[ u[ uvI ] ] + ROQ_VG_tab[ v[ uvI ] ] ) >> 6;
			b = ( YY + ROQ_UB_tab[ u[ uvI ] ] ) >> 6;

			if ( r < 0 )
			{
				r = 0;
			}

			if ( g < 0 )
			{
				g = 0;
			}

			if ( b < 0 )
			{
				b = 0;
			}

			if ( r > 255 )
			{
				r = 255;
			}

			if ( g > 255 )
			{
				g = 255;
			}

			if ( b > 255 )
			{
				b = 255;
			}

			*output = LittleLong( ( r ) | ( g << 8 ) | ( b << 16 ) | ( 255 << 24 ) );
			++output;
		}
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/
static unsigned int yuv_to_rgb24( long y, long u, long v )
{
	long r, g, b, YY = ROQ_YY_tab[( y ) ];

	r = ( YY + ROQ_VR_tab[ v ] ) >> 6;
	g = ( YY + ROQ_UG_tab[ u ] + ROQ_VG_tab[ v ] ) >> 6;
	b = ( YY + ROQ_UB_tab[ u ] ) >> 6;

	if ( r < 0 ) { r = 0; }

	if ( g < 0 ) { g = 0; }

	if ( b < 0 ) { b = 0; }

	if ( r > 255 ) { r = 255; }

	if ( g > 255 ) { g = 255; }

	if ( b > 255 ) { b = 255; }

	return LittleLong( ( r ) | ( g << 8 ) | ( b << 16 ) | ( 255 << 24 ) );
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void decodeCodeBook( byte *input, unsigned short roq_flags )
{
	long           i, j, two, four;
	unsigned short *aptr, *bptr, *cptr, *dptr;
	long           y0, y1, y2, y3, cr, cb;
	byte           *bbptr, *baptr, *bcptr, *bdptr;
	union
	{
		unsigned int   *i;
		unsigned short *s;
	} iaptr, ibptr, icptr, idptr;

	if ( !roq_flags )
	{
		two = four = 256;
	}
	else
	{
		two = roq_flags >> 8;

		if ( !two ) { two = 256; }

		four = roq_flags & 0xff;
	}

	four *= 2;

	bptr = ( unsigned short * ) vq2;

	if ( !cinTable[ currentHandle ].half )
	{
		if ( !cinTable[ currentHandle ].smootheddouble )
		{
//
// base height
//
			if ( cinTable[ currentHandle ].samplesPerPixel == 2 )
			{
				for ( i = 0; i < two; i++ )
				{
					y0 = ( long ) * input++;
					y1 = ( long ) * input++;
					y2 = ( long ) * input++;
					y3 = ( long ) * input++;
					cr = ( long ) * input++;
					cb = ( long ) * input++;
					*bptr++ = yuv_to_rgb( y0, cr, cb );
					*bptr++ = yuv_to_rgb( y1, cr, cb );
					*bptr++ = yuv_to_rgb( y2, cr, cb );
					*bptr++ = yuv_to_rgb( y3, cr, cb );
				}

				cptr = ( unsigned short * ) vq4;
				dptr = ( unsigned short * ) vq8;

				for ( i = 0; i < four; i++ )
				{
					aptr = ( unsigned short * ) vq2 + ( *input++ ) * 4;
					bptr = ( unsigned short * ) vq2 + ( *input++ ) * 4;

					for ( j = 0; j < 2; j++ )
					{
						VQ2TO4( aptr, bptr, cptr, dptr );
					}
				}
			}
			else if ( cinTable[ currentHandle ].samplesPerPixel == 4 )
			{
				ibptr.s = bptr;

				for ( i = 0; i < two; i++ )
				{
					y0 = ( long ) * input++;
					y1 = ( long ) * input++;
					y2 = ( long ) * input++;
					y3 = ( long ) * input++;
					cr = ( long ) * input++;
					cb = ( long ) * input++;
					*ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y3, cr, cb );
				}

				icptr.s = vq4;
				idptr.s = vq8;

				for ( i = 0; i < four; i++ )
				{
					iaptr.s = vq2;
					iaptr.i += ( *input++ ) * 4;
					ibptr.s = vq2;
					ibptr.i += ( *input++ ) * 4;

					for ( j = 0; j < 2; j++ )
					{
						VQ2TO4( iaptr.i, ibptr.i, icptr.i, idptr.i );
					}
				}
			}
			else if ( cinTable[ currentHandle ].samplesPerPixel == 1 )
			{
				bbptr = ( byte * ) bptr;

				for ( i = 0; i < two; i++ )
				{
					*bbptr++ = cinTable[ currentHandle ].gray[ *input++ ];
					*bbptr++ = cinTable[ currentHandle ].gray[ *input++ ];
					*bbptr++ = cinTable[ currentHandle ].gray[ *input++ ];
					*bbptr++ = cinTable[ currentHandle ].gray[ *input ];
					input += 3;
				}

				bcptr = ( byte * ) vq4;
				bdptr = ( byte * ) vq8;

				for ( i = 0; i < four; i++ )
				{
					baptr = ( byte * ) vq2 + ( *input++ ) * 4;
					bbptr = ( byte * ) vq2 + ( *input++ ) * 4;

					for ( j = 0; j < 2; j++ )
					{
						VQ2TO4( baptr, bbptr, bcptr, bdptr );
					}
				}
			}
		}
		else
		{
//
// double height, smoothed
//
			if ( cinTable[ currentHandle ].samplesPerPixel == 2 )
			{
				for ( i = 0; i < two; i++ )
				{
					y0 = ( long ) * input++;
					y1 = ( long ) * input++;
					y2 = ( long ) * input++;
					y3 = ( long ) * input++;
					cr = ( long ) * input++;
					cb = ( long ) * input++;
					*bptr++ = yuv_to_rgb( y0, cr, cb );
					*bptr++ = yuv_to_rgb( y1, cr, cb );
					*bptr++ = yuv_to_rgb( ( ( y0 * 3 ) + y2 ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( ( ( y1 * 3 ) + y3 ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( ( y0 + ( y2 * 3 ) ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( ( y1 + ( y3 * 3 ) ) / 4, cr, cb );
					*bptr++ = yuv_to_rgb( y2, cr, cb );
					*bptr++ = yuv_to_rgb( y3, cr, cb );
				}

				cptr = ( unsigned short * ) vq4;
				dptr = ( unsigned short * ) vq8;

				for ( i = 0; i < four; i++ )
				{
					aptr = ( unsigned short * ) vq2 + ( *input++ ) * 8;
					bptr = ( unsigned short * ) vq2 + ( *input++ ) * 8;

					for ( j = 0; j < 2; j++ )
					{
						VQ2TO4( aptr, bptr, cptr, dptr );
						VQ2TO4( aptr, bptr, cptr, dptr );
					}
				}
			}
			else if ( cinTable[ currentHandle ].samplesPerPixel == 4 )
			{
				ibptr.s = bptr;

				for ( i = 0; i < two; i++ )
				{
					y0 = ( long ) * input++;
					y1 = ( long ) * input++;
					y2 = ( long ) * input++;
					y3 = ( long ) * input++;
					cr = ( long ) * input++;
					cb = ( long ) * input++;
					*ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( ( ( y0 * 3 ) + y2 ) / 4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( ( ( y1 * 3 ) + y3 ) / 4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( ( y0 + ( y2 * 3 ) ) / 4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( ( y1 + ( y3 * 3 ) ) / 4, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr.i++ = yuv_to_rgb24( y3, cr, cb );
				}

				icptr.s = vq4;
				idptr.s = vq8;

				for ( i = 0; i < four; i++ )
				{
					iaptr.s = vq2;
					iaptr.i += ( *input++ ) * 8;
					ibptr.s = vq2;
					ibptr.i += ( *input++ ) * 8;

					for ( j = 0; j < 2; j++ )
					{
						VQ2TO4( iaptr.i, ibptr.i, icptr.i, idptr.i );
						VQ2TO4( iaptr.i, ibptr.i, icptr.i, idptr.i );
					}
				}
			}
			else if ( cinTable[ currentHandle ].samplesPerPixel == 1 )
			{
				bbptr = ( byte * ) bptr;

				for ( i = 0; i < two; i++ )
				{
					y0 = ( long ) * input++;
					y1 = ( long ) * input++;
					y2 = ( long ) * input++;
					y3 = ( long ) * input;
					input += 3;
					*bbptr++ = cinTable[ currentHandle ].gray[ y0 ];
					*bbptr++ = cinTable[ currentHandle ].gray[ y1 ];
					*bbptr++ = cinTable[ currentHandle ].gray[( ( y0 * 3 ) + y2 ) / 4 ];
					*bbptr++ = cinTable[ currentHandle ].gray[( ( y1 * 3 ) + y3 ) / 4 ];
					*bbptr++ = cinTable[ currentHandle ].gray[( y0 + ( y2 * 3 ) ) / 4 ];
					*bbptr++ = cinTable[ currentHandle ].gray[( y1 + ( y3 * 3 ) ) / 4 ];
					*bbptr++ = cinTable[ currentHandle ].gray[ y2 ];
					*bbptr++ = cinTable[ currentHandle ].gray[ y3 ];
				}

				bcptr = ( byte * ) vq4;
				bdptr = ( byte * ) vq8;

				for ( i = 0; i < four; i++ )
				{
					baptr = ( byte * ) vq2 + ( *input++ ) * 8;
					bbptr = ( byte * ) vq2 + ( *input++ ) * 8;

					for ( j = 0; j < 2; j++ )
					{
						VQ2TO4( baptr, bbptr, bcptr, bdptr );
						VQ2TO4( baptr, bbptr, bcptr, bdptr );
					}
				}
			}
		}
	}
	else
	{
//
// 1/4 screen
//
		if ( cinTable[ currentHandle ].samplesPerPixel == 2 )
		{
			for ( i = 0; i < two; i++ )
			{
				y0 = ( long ) * input;
				input += 2;
				y2 = ( long ) * input;
				input += 2;
				cr = ( long ) * input++;
				cb = ( long ) * input++;
				*bptr++ = yuv_to_rgb( y0, cr, cb );
				*bptr++ = yuv_to_rgb( y2, cr, cb );
			}

			cptr = ( unsigned short * ) vq4;
			dptr = ( unsigned short * ) vq8;

			for ( i = 0; i < four; i++ )
			{
				aptr = ( unsigned short * ) vq2 + ( *input++ ) * 2;
				bptr = ( unsigned short * ) vq2 + ( *input++ ) * 2;

				for ( j = 0; j < 2; j++ )
				{
					VQ2TO2( aptr, bptr, cptr, dptr );
				}
			}
		}
		else if ( cinTable[ currentHandle ].samplesPerPixel == 1 )
		{
			bbptr = ( byte * ) bptr;

			for ( i = 0; i < two; i++ )
			{
				*bbptr++ = cinTable[ currentHandle ].gray[ *input ];
				input += 2;
				*bbptr++ = cinTable[ currentHandle ].gray[ *input ];
				input += 4;
			}

			bcptr = ( byte * ) vq4;
			bdptr = ( byte * ) vq8;

			for ( i = 0; i < four; i++ )
			{
				baptr = ( byte * ) vq2 + ( *input++ ) * 2;
				bbptr = ( byte * ) vq2 + ( *input++ ) * 2;

				for ( j = 0; j < 2; j++ )
				{
					VQ2TO2( baptr, bbptr, bcptr, bdptr );
				}
			}
		}
		else if ( cinTable[ currentHandle ].samplesPerPixel == 4 )
		{
			ibptr.s = bptr;

			for ( i = 0; i < two; i++ )
			{
				y0 = ( long ) * input;
				input += 2;
				y2 = ( long ) * input;
				input += 2;
				cr = ( long ) * input++;
				cb = ( long ) * input++;
				*ibptr.i++ = yuv_to_rgb24( y0, cr, cb );
				*ibptr.i++ = yuv_to_rgb24( y2, cr, cb );
			}

			icptr.s = vq4;
			idptr.s = vq8;

			for ( i = 0; i < four; i++ )
			{
				iaptr.s = vq2;
				iaptr.i += ( *input++ ) * 2;
				ibptr.s = vq2 + ( *input++ ) * 2;
				ibptr.i += ( *input++ ) * 2;

				for ( j = 0; j < 2; j++ )
				{
					VQ2TO2( iaptr.i, ibptr.i, icptr.i, idptr.i );
				}
			}
		}
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void recurseQuad( long startX, long startY, long quadSize, long xOff, long yOff )
{
	byte *scroff;
	long bigx, bigy, lowx, lowy, useY;
	long offset;

	offset = cinTable[ currentHandle ].screenDelta;

	lowx = lowy = 0;
	bigx = cinTable[ currentHandle ].xsize;
	bigy = cinTable[ currentHandle ].ysize;

	if ( bigx > cinTable[ currentHandle ].CIN_WIDTH ) { bigx = cinTable[ currentHandle ].CIN_WIDTH; }

	if ( bigy > cinTable[ currentHandle ].CIN_HEIGHT ) { bigy = cinTable[ currentHandle ].CIN_HEIGHT; }

	if ( ( startX >= lowx ) && ( startX + quadSize ) <= ( bigx ) && ( startY + quadSize ) <= ( bigy ) && ( startY >= lowy ) && quadSize <= MAXSIZE )
	{
		useY = startY;
		scroff = cin.linbuf + ( useY + ( ( cinTable[ currentHandle ].CIN_HEIGHT - bigy ) >> 1 ) + yOff ) * ( cinTable[ currentHandle ].samplesPerLine ) + ( ( ( startX + xOff ) ) * cinTable[ currentHandle ].samplesPerPixel );

		cin.qStatus[ 0 ][ cinTable[ currentHandle ].onQuad  ] = scroff;
		cin.qStatus[ 1 ][ cinTable[ currentHandle ].onQuad++ ] = scroff + offset;
	}

	if ( quadSize != MINSIZE )
	{
		quadSize >>= 1;
		recurseQuad( startX,              startY, quadSize, xOff, yOff );
		recurseQuad( startX + quadSize, startY, quadSize, xOff, yOff );
		recurseQuad( startX,              startY + quadSize, quadSize, xOff, yOff );
		recurseQuad( startX + quadSize, startY + quadSize, quadSize, xOff, yOff );
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void setupQuad( long xOff, long yOff )
{
	long numQuadCels, i, x, y;
	byte *temp;

	if ( xOff == cin.oldXOff && yOff == cin.oldYOff && cinTable[ currentHandle ].ysize == cin.oldysize && cinTable[ currentHandle ].xsize == cin.oldxsize )
	{
		return;
	}

	cin.oldXOff = xOff;
	cin.oldYOff = yOff;
	cin.oldysize = cinTable[ currentHandle ].ysize;
	cin.oldxsize = cinTable[ currentHandle ].xsize;

	numQuadCels = ( cinTable[ currentHandle ].CIN_WIDTH * cinTable[ currentHandle ].CIN_HEIGHT ) / ( 16 );
	numQuadCels += numQuadCels / 4 + numQuadCels / 16;
	numQuadCels += 64; // for overflow

	numQuadCels = ( cinTable[ currentHandle ].xsize * cinTable[ currentHandle ].ysize ) / ( 16 );
	numQuadCels += numQuadCels / 4;
	numQuadCels += 64; // for overflow

	cinTable[ currentHandle ].onQuad = 0;

	for ( y = 0; y < ( long ) cinTable[ currentHandle ].ysize; y += 16 )
	{
		for ( x = 0; x < ( long ) cinTable[ currentHandle ].xsize; x += 16 )
		{
			recurseQuad( x, y, 16, xOff, yOff );
		}
	}

	temp = nullptr;

	for ( i = ( numQuadCels - 64 ); i < numQuadCels; i++ )
	{
		cin.qStatus[ 0 ][ i ] = temp; // eoq
		cin.qStatus[ 1 ][ i ] = temp; // eoq
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void readQuadInfo( byte *qData )
{
	if ( currentHandle < 0 ) { return; }

	cinTable[ currentHandle ].xsize = qData[ 0 ] + qData[ 1 ] * 256;
	cinTable[ currentHandle ].ysize = qData[ 2 ] + qData[ 3 ] * 256;
	cinTable[ currentHandle ].maxsize = qData[ 4 ] + qData[ 5 ] * 256;
	cinTable[ currentHandle ].minsize = qData[ 6 ] + qData[ 7 ] * 256;

	cinTable[ currentHandle ].CIN_HEIGHT = cinTable[ currentHandle ].ysize;
	cinTable[ currentHandle ].CIN_WIDTH = cinTable[ currentHandle ].xsize;

	cinTable[ currentHandle ].samplesPerLine = cinTable[ currentHandle ].CIN_WIDTH * cinTable[ currentHandle ].samplesPerPixel;
	cinTable[ currentHandle ].screenDelta = cinTable[ currentHandle ].CIN_HEIGHT * cinTable[ currentHandle ].samplesPerLine;

	cinTable[ currentHandle ].half = false;
	cinTable[ currentHandle ].smootheddouble = false;

	cinTable[ currentHandle ].VQ0 = cinTable[ currentHandle ].VQNormal;
	cinTable[ currentHandle ].VQ1 = cinTable[ currentHandle ].VQBuffer;

	cinTable[ currentHandle ].t[ 0 ] = cinTable[ currentHandle ].screenDelta;
	cinTable[ currentHandle ].t[ 1 ] = -cinTable[ currentHandle ].screenDelta;

	cinTable[ currentHandle ].drawX = cinTable[ currentHandle ].CIN_WIDTH;
	cinTable[ currentHandle ].drawY = cinTable[ currentHandle ].CIN_HEIGHT;
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void RoQPrepMcomp( long xoff, long yoff )
{
	long i, j, x, y, temp, temp2;

	i = cinTable[ currentHandle ].samplesPerLine;
	j = cinTable[ currentHandle ].samplesPerPixel;

	if ( cinTable[ currentHandle ].xsize == ( cinTable[ currentHandle ].ysize * 4 ) && !cinTable[ currentHandle ].half ) { j = j + j; i = i + i; }

	for ( y = 0; y < 16; y++ )
	{
		temp2 = ( y + yoff - 8 ) * i;

		for ( x = 0; x < 16; x++ )
		{
			temp = ( x + xoff - 8 ) * j;
			cin.mcomp[( x * 16 ) + y ] = cinTable[ currentHandle ].normalBuffer0 - ( temp2 + temp );
		}
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void initRoQ()
{
	if ( currentHandle < 0 ) { return; }

	cinTable[ currentHandle ].VQNormal = ( void ( * )( byte *, void * ) ) blitVQQuad32fs;
	cinTable[ currentHandle ].VQBuffer = ( void ( * )( byte *, void * ) ) blitVQQuad32fs;
	cinTable[ currentHandle ].samplesPerPixel = 4;
	ROQ_GenYUVTables();
	RllSetupTable();
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

/*
static byte* RoQFetchInterlaced( byte *source ) {
        int x, *src, *dst;

        if (currentHandle < 0) return nullptr;

        src = (int *)source;
        dst = (int *)cinTable[currentHandle].buf2;

        for(x=0;x<256*256;x++) {
                *dst = *src;
                dst++; src += 2;
        }
        return cinTable[currentHandle].buf2;
}
*/
static void RoQReset()
{
	if ( currentHandle < 0 ) { return; }

	FS_FCloseFile( cinTable[ currentHandle ].iFile );
	FS_FOpenFileRead( cinTable[ currentHandle ].fileName, &cinTable[ currentHandle ].iFile, true );
	// let the background thread start reading ahead
	FS_Read( cin.file, 16, cinTable[ currentHandle ].iFile );
	RoQ_init();
	cinTable[ currentHandle ].status = FMV_LOOPED;
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void RoQInterrupt()
{
	byte  *framedata;
	int   ssize;

	if ( currentHandle < 0 ) { return; }

	FS_Read( cin.file, cinTable[ currentHandle ].RoQFrameSize + 8, cinTable[ currentHandle ].iFile );

	if ( cinTable[ currentHandle ].RoQPlayed >= cinTable[ currentHandle ].ROQSize )
	{
		if ( cinTable[ currentHandle ].holdAtEnd == false )
		{
			if ( cinTable[ currentHandle ].looping )
			{
				RoQReset();
			}
			else
			{
				cinTable[ currentHandle ].status = FMV_EOF;
			}
		}
		else
		{
			cinTable[ currentHandle ].status = FMV_IDLE;
		}

		return;
	}

	framedata = cin.file;
//
// new frame is ready
//
redump:

	switch ( cinTable[ currentHandle ].roq_id )
	{
		case    ROQ_QUAD_VQ:
			if ( ( cinTable[ currentHandle ].numQuads & 1 ) )
			{
				cinTable[ currentHandle ].normalBuffer0 = cinTable[ currentHandle ].t[ 1 ];
				RoQPrepMcomp( cinTable[ currentHandle ].roqF0, cinTable[ currentHandle ].roqF1 );
				cinTable[ currentHandle ].VQ1( ( byte * ) cin.qStatus[ 1 ], framedata );
				cinTable[ currentHandle ].buf = cin.linbuf + cinTable[ currentHandle ].screenDelta;
			}
			else
			{
				cinTable[ currentHandle ].normalBuffer0 = cinTable[ currentHandle ].t[ 0 ];
				RoQPrepMcomp( cinTable[ currentHandle ].roqF0, cinTable[ currentHandle ].roqF1 );
				cinTable[ currentHandle ].VQ0( ( byte * ) cin.qStatus[ 0 ], framedata );
				cinTable[ currentHandle ].buf = cin.linbuf;
			}

			if ( cinTable[ currentHandle ].numQuads == 0 ) // first frame
			{
				Com_Memcpy( cin.linbuf + cinTable[ currentHandle ].screenDelta, cin.linbuf, cinTable[ currentHandle ].samplesPerLine * cinTable[ currentHandle ].ysize );
			}

			cinTable[ currentHandle ].numQuads++;
			cinTable[ currentHandle ].dirty = true;
			break;

		case    ROQ_CODEBOOK:
			decodeCodeBook( framedata, ( unsigned short ) cinTable[ currentHandle ].roq_flags );
			break;

		case    ZA_SOUND_MONO:
			if ( !cinTable[ currentHandle ].silent )
			{
			    short* sbuf = new short[32768];
			    ssize = RllDecodeMonoToStereo( framedata, sbuf, cinTable[ currentHandle ].RoQFrameSize, 0, ( unsigned short ) cinTable[ currentHandle ].roq_flags );
                Audio::StreamData( 0, (byte*) sbuf, ssize, 22050, 2, 1, 1.0f, 1);
			}

			break;

		case    ZA_SOUND_STEREO:
			if ( !cinTable[ currentHandle ].silent )
			{
				if ( cinTable[ currentHandle ].numQuads == -1 )
				{
					Audio::Update();
					//TODO
					//s_rawend[0] = s_soundtime;
				}
			    short* sbuf = new short[32768];
				ssize = RllDecodeStereoToStereo( framedata, sbuf, cinTable[ currentHandle ].RoQFrameSize, 0, ( unsigned short ) cinTable[ currentHandle ].roq_flags );
                Audio::StreamData( 0, (byte*) sbuf, ssize, 22050, 2, 2, 1.0f, 1);
			}

			break;

		case    ROQ_QUAD_INFO:
			if ( cinTable[ currentHandle ].numQuads == -1 )
			{
				readQuadInfo( framedata );
				setupQuad( 0, 0 );
				// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
				cinTable[ currentHandle ].startTime = cinTable[ currentHandle ].lastTime = CL_ScaledMilliseconds() * com_timescale->value;
			}

			if ( cinTable[ currentHandle ].numQuads != 1 ) { cinTable[ currentHandle ].numQuads = 0; }

			break;

		case    ROQ_PACKET:
			cinTable[ currentHandle ].inMemory = cinTable[ currentHandle ].roq_flags;
			cinTable[ currentHandle ].RoQFrameSize = 0; // for header
			break;

		case    ROQ_QUAD_HANG:
			cinTable[ currentHandle ].RoQFrameSize = 0;
			break;

		case    ROQ_QUAD_JPEG:
			break;

		default:
			cinTable[ currentHandle ].status = FMV_EOF;
			break;
	}

//
// read in next frame data
//
	if ( cinTable[ currentHandle ].RoQPlayed >= cinTable[ currentHandle ].ROQSize )
	{
		if ( cinTable[ currentHandle ].holdAtEnd == false )
		{
			if ( cinTable[ currentHandle ].looping )
			{
				RoQReset();
			}
			else
			{
				cinTable[ currentHandle ].status = FMV_EOF;
			}
		}
		else
		{
			cinTable[ currentHandle ].status = FMV_IDLE;
		}

		return;
	}

	framedata += cinTable[ currentHandle ].RoQFrameSize;
	cinTable[ currentHandle ].roq_id = framedata[ 0 ] + framedata[ 1 ] * 256;
	cinTable[ currentHandle ].RoQFrameSize = framedata[ 2 ] + framedata[ 3 ] * 256 + framedata[ 4 ] * 65536;
	cinTable[ currentHandle ].roq_flags = framedata[ 6 ] + framedata[ 7 ] * 256;
	cinTable[ currentHandle ].roqF0 = ( signed char ) framedata[ 7 ];
	cinTable[ currentHandle ].roqF1 = ( signed char ) framedata[ 6 ];

	if ( cinTable[ currentHandle ].RoQFrameSize > 65536 || cinTable[ currentHandle ].roq_id == 0x1084 )
	{
		Com_DPrintf( "roq_size>65536||roq_id==0x1084\n" );
		cinTable[ currentHandle ].status = FMV_EOF;

		if ( cinTable[ currentHandle ].looping )
		{
			RoQReset();
		}

		return;
	}

	if ( cinTable[ currentHandle ].inMemory && ( cinTable[ currentHandle ].status != FMV_EOF ) ) { cinTable[ currentHandle ].inMemory = false; framedata += 8; goto redump; }

//
// one more frame hits the dust
//
//	assert(cinTable[currentHandle].RoQFrameSize <= 65536);
//	r = FS_Read( cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile );
	cinTable[ currentHandle ].RoQPlayed += cinTable[ currentHandle ].RoQFrameSize + 8;
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void RoQ_init()
{
	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	cinTable[ currentHandle ].startTime = cinTable[ currentHandle ].lastTime = CL_ScaledMilliseconds() * com_timescale->value;

	cinTable[ currentHandle ].RoQPlayed = 24;

	/*  get frame rate */
	cinTable[ currentHandle ].roqFPS = cin.file[ 6 ] + cin.file[ 7 ] * 256;

	if ( !cinTable[ currentHandle ].roqFPS ) { cinTable[ currentHandle ].roqFPS = 30; }

	cinTable[ currentHandle ].numQuads = -1;

	cinTable[ currentHandle ].roq_id = cin.file[ 8 ] + cin.file[ 9 ] * 256;
	cinTable[ currentHandle ].RoQFrameSize = cin.file[ 10 ] + cin.file[ 11 ] * 256 + cin.file[ 12 ] * 65536;
	cinTable[ currentHandle ].roq_flags = cin.file[ 14 ] + cin.file[ 15 ] * 256;

	if ( cinTable[ currentHandle ].RoQFrameSize > 65536 || !cinTable[ currentHandle ].RoQFrameSize )
	{
		return;
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

//FIXME: this isn't really a "ROQ shutdown" function (it's more like a "CIN shutdown" function, beside the file closing)
static void RoQShutdown()
{
	const char *s;

	if ( !cinTable[ currentHandle ].buf )
	{
		//FIXME: there could be something that should be "shutdowned" even if we don't have an output frame (at least in the ogm code)
		return;
	}

	if ( cinTable[ currentHandle ].status == FMV_IDLE )
	{
		return;
	}

	Com_DPrintf( "finished cinematic\n" );
	cinTable[ currentHandle ].status = FMV_IDLE;

	if ( cinTable[ currentHandle ].iFile )
	{
		FS_FCloseFile( cinTable[ currentHandle ].iFile );
		cinTable[ currentHandle ].iFile = 0;
	}

	if ( cinTable[ currentHandle ].alterGameState )
	{
		cls.state = CA_DISCONNECTED;
		// we can't just do a vstr nextmap, because
		// if we are aborting the intro cinematic with
		// a devmap command, nextmap would be valid by
		// the time it was referenced
		s = Cvar_VariableString( "sv_nextmap" );

		if ( s[ 0 ] )
		{
			Cmd::BufferCommandText(s);
			Cvar_Set( "sv_nextmap", "" );
		}

		CL_handle = -1;
	}

	cinTable[ currentHandle ].fileName[ 0 ] = 0;

	if ( cinTable[ currentHandle ].fileType == FT_OGM )
	{
		Cin_OGM_Shutdown();
		cinTable[ currentHandle ].buf = nullptr;
	}

	currentHandle = -1;
}

/*
==================
CIN_StopCinematic
==================
*/
e_status CIN_StopCinematic( int handle )
{
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[ handle ].status == FMV_EOF ) { return FMV_EOF; }

	currentHandle = handle;

	Com_DPrintf( "trFMV::stop(), closing %s\n", cinTable[ currentHandle ].fileName );

	if ( !cinTable[ currentHandle ].buf )
	{
		return FMV_EOF;
	}

	if ( cinTable[ currentHandle ].alterGameState )
	{
		if ( cls.state != CA_CINEMATIC )
		{
			return cinTable[ currentHandle ].status;
		}
	}

	cinTable[ currentHandle ].status = FMV_EOF;
	RoQShutdown();

	return FMV_EOF;
}

/*
==================
CIN_RunCinematic

Fetch and decompress the pending frame
==================
*/
e_status CIN_RunCinematic( int handle )
{
	int start = 0;
	int thisTime = 0;

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[ handle ].status == FMV_EOF ) { return FMV_EOF; }

	if ( cin.currentHandle != handle )
	{
		currentHandle = handle;
		cin.currentHandle = currentHandle;
		cinTable[ currentHandle ].status = FMV_EOF;
		RoQReset();
	}

	if ( cinTable[ handle ].playonwalls < -1 )
	{
		return cinTable[ handle ].status;
	}

	currentHandle = handle;

	if ( cinTable[ currentHandle ].alterGameState )
	{
		if ( cls.state != CA_CINEMATIC )
		{
			return cinTable[ currentHandle ].status;
		}
	}

	if ( cinTable[ currentHandle ].status == FMV_IDLE )
	{
		return cinTable[ currentHandle ].status;
	}

	if ( cinTable[ currentHandle ].fileType == FT_OGM )
	{
		if ( Cin_OGM_Run( cinTable[ currentHandle ].startTime == 0 ? 0 : CL_ScaledMilliseconds() - cinTable[ currentHandle ].startTime ) )
		{
			cinTable[ currentHandle ].status = FMV_EOF;
		}
		else
		{
			int      newW, newH;
			bool resolutionChange = false;

			cinTable[ currentHandle ].buf = Cin_OGM_GetOutput( &newW, &newH );

			if ( newW != cinTable[ currentHandle ].CIN_WIDTH )
			{
				cinTable[ currentHandle ].CIN_WIDTH = newW;
				resolutionChange = true;
			}

			if ( newH != cinTable[ currentHandle ].CIN_HEIGHT )
			{
				cinTable[ currentHandle ].CIN_HEIGHT = newH;
				resolutionChange = true;
			}

			if ( resolutionChange )
			{
				cinTable[ currentHandle ].drawX = cinTable[ currentHandle ].CIN_WIDTH;
				cinTable[ currentHandle ].drawY = cinTable[ currentHandle ].CIN_HEIGHT;
			}

			cinTable[ currentHandle ].status = FMV_PLAY;
			cinTable[ currentHandle ].dirty = true;
		}

		if ( !cinTable[ currentHandle ].startTime )
		{
			cinTable[ currentHandle ].startTime = CL_ScaledMilliseconds();
		}

		if ( cinTable[ currentHandle ].status == FMV_EOF )
		{
			if ( cinTable[ currentHandle ].holdAtEnd )
			{
				cinTable[ currentHandle ].status = FMV_IDLE;
			}
			else if ( cinTable[ currentHandle ].looping )
			{
				Cin_OGM_Shutdown();
				Cin_OGM_Init( cinTable[ currentHandle ].fileName );
				cinTable[ currentHandle ].buf = nullptr;
				cinTable[ currentHandle ].startTime = 0;
				cinTable[ currentHandle ].status = FMV_PLAY;
			}
			else
			{
				RoQShutdown();
//              Cin_OGM_Shutdown();
			}
		}

		return cinTable[ currentHandle ].status;
	}

	//FIXME? CL_ScaledMilliseconds already uses com_timescale (so I can't see that the com_timescale in here makes any sense at all O_o)
	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	thisTime = CL_ScaledMilliseconds() * com_timescale->value;

	if ( cinTable[ currentHandle ].shader && ( abs( thisTime - cinTable[ currentHandle ].lastTime ) ) > 100 )
	{
		cinTable[ currentHandle ].startTime += thisTime - cinTable[ currentHandle ].lastTime;
	}

	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	cinTable[ currentHandle ].tfps = ( ( ( ( CL_ScaledMilliseconds() * com_timescale->value ) - cinTable[ currentHandle ].startTime ) * 3 ) / 100 );

	start = cinTable[ currentHandle ].startTime;

	while ( ( cinTable[ currentHandle ].tfps != cinTable[ currentHandle ].numQuads )
	        && ( cinTable[ currentHandle ].status == FMV_PLAY ) )
	{
		RoQInterrupt();

		if ( start != cinTable[ currentHandle ].startTime )
		{
			// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
			cinTable[ currentHandle ].tfps = ( ( ( ( CL_ScaledMilliseconds() * com_timescale->value )
			                                       - cinTable[ currentHandle ].startTime ) * 3 ) / 100 );
			start = cinTable[ currentHandle ].startTime;
		}
	}

	cinTable[ currentHandle ].lastTime = thisTime;

	if ( cinTable[ currentHandle ].status == FMV_LOOPED )
	{
		cinTable[ currentHandle ].status = FMV_PLAY;
	}

	if ( cinTable[ currentHandle ].status == FMV_EOF )
	{
		if ( cinTable[ currentHandle ].looping )
		{
			RoQReset();
		}
		else
		{
			RoQShutdown();
		}
	}

	return cinTable[ currentHandle ].status;
}

/*
==================
CIN_PlayCinematic
==================
*/
int CIN_PlayCinematic( const char *arg, int x, int y, int w, int h, int systemBits )
{
	unsigned short RoQID;
	char           name[ MAX_OSPATH ];
	int            i;
	char           *fileextPtr;

	if ( strstr( arg, "/" ) == nullptr && strstr( arg, "\\" ) == nullptr )
	{
		Com_sprintf( name, sizeof( name ), "video/%s", arg );
	}
	else
	{
		Com_sprintf( name, sizeof( name ), "%s", arg );
	}

	if ( !( systemBits & CIN_system ) )
	{
		for ( i = 0; i < MAX_VIDEO_HANDLES; i++ )
		{
			if ( !strcmp( cinTable[ i ].fileName, name ) )
			{
				return i;
			}
		}
	}

	Com_DPrintf( "SCR_PlayCinematic( %s )\n", arg );

	Com_Memset( &cin, 0, sizeof( cinematics_t ) );
	currentHandle = CIN_HandleForVideo();

	cin.currentHandle = currentHandle;

	strcpy( cinTable[ currentHandle ].fileName, name );

	fileextPtr = name;
	while ( fileextPtr && *fileextPtr != '.' )
	{
		fileextPtr++;
	}

	if ( !Q_stricmp( fileextPtr, ".ogm" ) )
	{
		if ( Cin_OGM_Init( name ) )
		{
			Com_Printf( "failed to start OGM playback (%s)\n", arg );
			cinTable[ currentHandle ].fileName[ 0 ] = 0;
			Cin_OGM_Shutdown();
			return -1;
		}

		cinTable[ currentHandle ].fileType = FT_OGM;

		CIN_SetExtents( currentHandle, x, y, w, h );
		CIN_SetLooping( currentHandle, ( systemBits & CIN_loop ) != 0 );

		cinTable[ currentHandle ].holdAtEnd = ( systemBits & CIN_hold ) != 0;
		cinTable[ currentHandle ].alterGameState = ( systemBits & CIN_system ) != 0;
		cinTable[ currentHandle ].playonwalls = 1;
		cinTable[ currentHandle ].silent = ( systemBits & CIN_silent ) != 0;
		cinTable[ currentHandle ].shader = ( systemBits & CIN_shader ) != 0;

		/* we will set this info after the first xvid-frame
		                cinTable[currentHandle].CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
		                cinTable[currentHandle].CIN_WIDTH  =  DEFAULT_CIN_WIDTH;
		*/

		if ( cinTable[ currentHandle ].alterGameState )
		{
			// close the menu
			// TODO: Rocket: Close all menus
		}
		else
		{
			cinTable[ currentHandle ].playonwalls = cl_inGameVideo->integer;
		}

		if ( cinTable[ currentHandle ].alterGameState )
		{
			cls.state = CA_CINEMATIC;
		}

		cinTable[ currentHandle ].status = FMV_PLAY;

		return currentHandle;
	}

	cinTable[ currentHandle ].ROQSize = 0;
	cinTable[ currentHandle ].ROQSize = FS_FOpenFileRead( cinTable[ currentHandle ].fileName, &cinTable[ currentHandle ].iFile, true );

	if ( cinTable[ currentHandle ].ROQSize <= 0 )
	{
		Com_DPrintf( "play(%s), ROQSize<=0\n", arg );
		cinTable[ currentHandle ].fileName[ 0 ] = 0;
		return -1;
	}

	CIN_SetExtents( currentHandle, x, y, w, h );
	CIN_SetLooping( currentHandle, ( systemBits & CIN_loop ) != 0 );

	cinTable[ currentHandle ].CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	cinTable[ currentHandle ].CIN_WIDTH = DEFAULT_CIN_WIDTH;
	cinTable[ currentHandle ].holdAtEnd = ( systemBits & CIN_hold ) != 0;
	cinTable[ currentHandle ].alterGameState = ( systemBits & CIN_system ) != 0;
	cinTable[ currentHandle ].playonwalls = 1;
	cinTable[ currentHandle ].silent = ( systemBits & CIN_silent ) != 0;
	cinTable[ currentHandle ].shader = ( systemBits & CIN_shader ) != 0;

	if ( cinTable[ currentHandle ].alterGameState )
	{
		// close the menu
		// TODO: Rocket
	}
	else
	{
		cinTable[ currentHandle ].playonwalls = cl_inGameVideo->integer;
	}

	initRoQ();

	FS_Read( cin.file, 16, cinTable[ currentHandle ].iFile );

	RoQID = ( unsigned short )( cin.file[ 0 ] ) + ( unsigned short )( cin.file[ 1 ] ) * 256;

	if ( RoQID == 0x1084 )
	{
		RoQ_init();
//		FS_Read (cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile);

		cinTable[ currentHandle ].status = FMV_PLAY;
		Com_DPrintf( "trFMV::play(), playing %s\n", arg );

		if ( cinTable[ currentHandle ].alterGameState )
		{
			cls.state = CA_CINEMATIC;
		}

		Con_Close();

		//TODO
		//s_rawend[0] = s_soundtime;

		return currentHandle;
	}

	Com_DPrintf( "trFMV::play(), invalid RoQ ID\n" );

	RoQShutdown();
	return -1;
}

void CIN_SetExtents( int handle, int x, int y, int w, int h )
{
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[ handle ].status == FMV_EOF ) { return; }

	cinTable[ handle ].xpos = x;
	cinTable[ handle ].ypos = y;
	cinTable[ handle ].width = w;
	cinTable[ handle ].height = h;
	cinTable[ handle ].dirty = true;
}

void CIN_SetLooping( int handle, bool loop )
{
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[ handle ].status == FMV_EOF ) { return; }

	cinTable[ handle ].looping = loop;
}

/*
==================
CIN_DrawCinematic
==================
*/
void CIN_DrawCinematic( int handle )
{
	float x, y, w, h;
	byte  *buf;

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[ handle ].status == FMV_EOF ) { return; }

	if ( !cinTable[ handle ].buf )
	{
		return;
	}

	x = cinTable[ handle ].xpos;
	y = cinTable[ handle ].ypos;
	w = cinTable[ handle ].width;
	h = cinTable[ handle ].height;
	buf = cinTable[ handle ].buf;
	SCR_AdjustFrom640( &x, &y, &w, &h );

	if ( cinTable[ handle ].dirty && ( cinTable[ handle ].CIN_WIDTH != cinTable[ handle ].drawX || cinTable[ handle ].CIN_HEIGHT != cinTable[ handle ].drawY ) )
	{
		int ix, iy, *buf2, *buf3, xm, ym, ll;

		xm = cinTable[ handle ].CIN_WIDTH / 256;
		ym = cinTable[ handle ].CIN_HEIGHT / 256;
		ll = 8;

		if ( cinTable[ handle ].CIN_WIDTH == 512 )
		{
			ll = 9;
		}

		buf3 = ( int * ) buf;
		buf2 = (int*) Hunk_AllocateTempMemory( 256 * 256 * 4 );

		if ( xm == 2 && ym == 2 )
		{
			byte *bc2, *bc3;
			int  ic, iiy;

			bc2 = ( byte * ) buf2;
			bc3 = ( byte * ) buf3;

			for ( iy = 0; iy < 256; iy++ )
			{
				iiy = iy << 12;

				for ( ix = 0; ix < 2048; ix += 8 )
				{
					for ( ic = ix; ic < ( ix + 4 ); ic++ )
					{
						*bc2 = ( bc3[ iiy + ic ] + bc3[ iiy + 4 + ic ] + bc3[ iiy + 2048 + ic ] + bc3[ iiy + 2048 + 4 + ic ] ) >> 2;
						bc2++;
					}
				}
			}
		}
		else if ( xm == 2 && ym == 1 )
		{
			byte *bc2, *bc3;
			int  ic, iiy;

			bc2 = ( byte * ) buf2;
			bc3 = ( byte * ) buf3;

			for ( iy = 0; iy < 256; iy++ )
			{
				iiy = iy << 11;

				for ( ix = 0; ix < 2048; ix += 8 )
				{
					for ( ic = ix; ic < ( ix + 4 ); ic++ )
					{
						*bc2 = ( bc3[ iiy + ic ] + bc3[ iiy + 4 + ic ] ) >> 1;
						bc2++;
					}
				}
			}
		}
		else
		{
			for ( iy = 0; iy < 256; iy++ )
			{
				for ( ix = 0; ix < 256; ix++ )
				{
					buf2[( iy << 8 ) + ix ] = buf3[( ( iy * ym ) << ll ) + ( ix * xm ) ];
				}
			}
		}

		re.DrawStretchRaw( x, y, w, h, 256, 256, ( byte * ) buf2, handle, true );
		cinTable[ handle ].dirty = false;
		Hunk_FreeTempMemory( buf2 );
		return;
	}

	re.DrawStretchRaw( x, y, w, h, cinTable[ handle ].drawX, cinTable[ handle ].drawY, buf, handle, cinTable[ handle ].dirty );
	cinTable[ handle ].dirty = false;
}

void CL_PlayCinematic_f()
{
	const char *arg, *s;
//	bool holdatend;
	int  bits = CIN_system;

	// don't allow this while on server
	if ( cls.state > CA_DISCONNECTED && cls.state <= CA_ACTIVE )
	{
		return;
	}

	Com_DPrintf( "CL_PlayCinematic_f\n" );

	if ( cls.state == CA_CINEMATIC )
	{
		SCR_StopCinematic();
	}

	arg = Cmd_Argv( 1 );
	s = Cmd_Argv( 2 );

//	holdatend = false;
	if ( ( s && s[ 0 ] == '1' ) || Q_stricmp( arg, "demoend.roq" ) == 0 || Q_stricmp( arg, "end.roq" ) == 0 )
	{
		bits |= CIN_hold;
	}

	if ( s && s[ 0 ] == '2' )
	{
		bits |= CIN_loop;
	}

	Audio::StopAllSounds();

	CL_handle = CIN_PlayCinematic( arg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bits );

	if ( CL_handle >= 0 )
	{
		do
		{
			SCR_RunCinematic();
		}
		while ( cinTable[ currentHandle ].buf == nullptr && cinTable[ currentHandle ].status == FMV_PLAY ); // wait for first frame (load codebook and sound)
	}
}

void SCR_DrawCinematic()
{
	if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES )
	{
		CIN_DrawCinematic( CL_handle );
	}
}

void SCR_RunCinematic()
{
	if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES )
	{
		CIN_RunCinematic( CL_handle );
	}
}

void SCR_StopCinematic()
{
	if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES )
	{
		CIN_StopCinematic( CL_handle );
		Audio::StopAllSounds();
		CL_handle = -1;
	}
}

void CIN_UploadCinematic( int handle )
{
	if ( handle >= 0 && handle < MAX_VIDEO_HANDLES )
	{
		if ( !cinTable[ handle ].buf )
		{
			return;
		}

		if ( cinTable[ handle ].playonwalls <= 0 && cinTable[ handle ].dirty )
		{
			if ( cinTable[ handle ].playonwalls == 0 )
			{
				cinTable[ handle ].playonwalls = -1;
			}
			else
			{
				if ( cinTable[ handle ].playonwalls == -1 )
				{
					cinTable[ handle ].playonwalls = -2;
				}
				else
				{
					cinTable[ handle ].dirty = false;
				}
			}
		}

		re.UploadCinematic( 256, 256, cinTable[ handle ].buf, handle, cinTable[ handle ].dirty );

		if ( cl_inGameVideo->integer == 0 && cinTable[ handle ].playonwalls == 1 )
		{
			cinTable[ handle ].playonwalls--;
		}
	}
}
