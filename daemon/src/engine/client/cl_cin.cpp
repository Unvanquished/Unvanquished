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

#include "client.h"

#include "framework/CommandSystem.h"

static const int DEFAULT_CIN_WIDTH  = 512;
static const int DEFAULT_CIN_HEIGHT = 512;

static const int MAX_VIDEO_HANDLES  = 16;

static long ROQ_YY_tab[ 256 ];
static long ROQ_UB_tab[ 256 ];
static long ROQ_UG_tab[ 256 ];
static long ROQ_VG_tab[ 256 ];
static long ROQ_VR_tab[ 256 ];

enum class filetype_t
{
  FT_OGM // ogm (ogg wrapper, vorbis audio, xvid/theora video) for WoP
};

struct cin_cache
{
	char         fileName[ MAX_OSPATH ];
	int          CIN_WIDTH, CIN_HEIGHT;
	int          xpos, ypos, width, height;
	bool     looping, holdAtEnd, dirty, alterGameState, silent, shader;
	e_status     status;
	int          startTime;
	int          lastTime;

	int          playonwalls;
	byte         *buf;
	long         drawX, drawY;
	filetype_t   fileType;
};

struct cinematics_t
{
	int   currentHandle;
};

static cinematics_t cin;
static cin_cache    cinTable[ MAX_VIDEO_HANDLES ];
static int          currentHandle = -1;
static int          CL_handle = -1;

/*
==================
CIN_StopCinematic
==================
*/
e_status CIN_StopCinematic( int handle )
{
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[ handle ].status == e_status::FMV_EOF ) { return e_status::FMV_EOF; }

	currentHandle = handle;

	Log::Debug( "trFMV::stop(), closing %s", cinTable[ currentHandle ].fileName );

	if ( !cinTable[ currentHandle ].buf )
	{
		return e_status::FMV_EOF;
	}

	if ( cinTable[ currentHandle ].alterGameState )
	{
		if ( cls.state != connstate_t::CA_CINEMATIC )
		{
			return cinTable[ currentHandle ].status;
		}
	}

	cinTable[ currentHandle ].status = e_status::FMV_EOF;

	return e_status::FMV_EOF;
}

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

	Com_Error( errorParm_t::ERR_DROP, "CIN_HandleForVideo: none free" );
}

extern int CL_ScaledMilliseconds();

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

/*
==================
CIN_RunCinematic

Fetch and decompress the pending frame
==================
*/
e_status CIN_RunCinematic( int handle )
{
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[ handle ].status == e_status::FMV_EOF ) { return e_status::FMV_EOF; }

	if ( cin.currentHandle != handle )
	{
		currentHandle = handle;
		cin.currentHandle = currentHandle;
		cinTable[ currentHandle ].status = e_status::FMV_EOF;
	}

	if ( cinTable[ handle ].playonwalls < -1 )
	{
		return cinTable[ handle ].status;
	}

	currentHandle = handle;

	if ( cinTable[ currentHandle ].alterGameState )
	{
		if ( cls.state != connstate_t::CA_CINEMATIC )
		{
			return cinTable[ currentHandle ].status;
		}
	}

	if ( cinTable[ currentHandle ].status == e_status::FMV_IDLE )
	{
		return cinTable[ currentHandle ].status;
	}

	if ( cinTable[ currentHandle ].fileType == filetype_t::FT_OGM )
	{
		if ( Cin_OGM_Run( cinTable[ currentHandle ].startTime == 0 ? 0 : CL_ScaledMilliseconds() - cinTable[ currentHandle ].startTime ) )
		{
			cinTable[ currentHandle ].status = e_status::FMV_EOF;
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

			cinTable[ currentHandle ].status = e_status::FMV_PLAY;
			cinTable[ currentHandle ].dirty = true;
		}

		if ( !cinTable[ currentHandle ].startTime )
		{
			cinTable[ currentHandle ].startTime = CL_ScaledMilliseconds();
		}

		if ( cinTable[ currentHandle ].status == e_status::FMV_EOF )
		{
			if ( cinTable[ currentHandle ].holdAtEnd )
			{
				cinTable[ currentHandle ].status = e_status::FMV_IDLE;
			}
			else if ( cinTable[ currentHandle ].looping )
			{
				Cin_OGM_Shutdown();
				Cin_OGM_Init( cinTable[ currentHandle ].fileName );
				cinTable[ currentHandle ].buf = nullptr;
				cinTable[ currentHandle ].startTime = 0;
				cinTable[ currentHandle ].status = e_status::FMV_PLAY;
			}
			else
			{
//              Cin_OGM_Shutdown();
			}
		}

		return cinTable[ currentHandle ].status;
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

	Log::Debug( "SCR_PlayCinematic( %s )", arg );

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

		cinTable[ currentHandle ].fileType = filetype_t::FT_OGM;

        cinTable[ currentHandle ].xpos = x;
        cinTable[ currentHandle ].ypos = y;
        cinTable[ currentHandle ].width = w;
        cinTable[ currentHandle ].height = h;
        cinTable[ currentHandle ].dirty = true;
		cinTable[ currentHandle ] .looping = (systemBits & CIN_loop) != 0;

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
			cls.state = connstate_t::CA_CINEMATIC;
		}

		cinTable[ currentHandle ].status = e_status::FMV_PLAY;

		return currentHandle;
	}

	return -1;
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

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[ handle ].status == e_status::FMV_EOF ) { return; }

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
	if ( cls.state > connstate_t::CA_DISCONNECTED && cls.state <= connstate_t::CA_ACTIVE )
	{
		return;
	}

	Log::Debug( "CL_PlayCinematic_f" );

	if ( cls.state == connstate_t::CA_CINEMATIC )
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
		while ( cinTable[ currentHandle ].buf == nullptr && cinTable[ currentHandle ].status == e_status::FMV_PLAY ); // wait for first frame (load codebook and sound)
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
