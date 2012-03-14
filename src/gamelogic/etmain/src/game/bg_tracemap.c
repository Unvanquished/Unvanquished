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

#ifdef CGAMEDLL
#include "../cgame/cg_local.h"
#else
#include "g_local.h"
#endif

/*
**  Map tracemap view generation
*/

#define MAX_WORLD_HEIGHT MAX_MAP_SIZE // maximum world height
#define MIN_WORLD_HEIGHT -MAX_MAP_SIZE // minimum world height

//#define TRACEMAP_SIZE             1024
#define TRACEMAP_SIZE    256

typedef struct tracemap_s
{
	qboolean loaded;
	float    sky[ TRACEMAP_SIZE ][ TRACEMAP_SIZE ];
	float    skyground[ TRACEMAP_SIZE ][ TRACEMAP_SIZE ];
	float    ground[ TRACEMAP_SIZE ][ TRACEMAP_SIZE ];
	vec2_t   world_mins, world_maxs;
	int      groundfloor, groundceil;
} tracemap_t;

static tracemap_t tracemap;

static vec2_t     one_over_mapgrid_factor;

void              etpro_FinalizeTracemapClamp ( int *x, int *y );

#ifdef CGAMEDLL
void CG_GenerateTracemap ( void )
{
	trace_t      tr;
	vec3_t       start, end;
	int          i, j;
	float        x_step, y_step;
	int          topdownmin, topdownmax;
	int          skygroundmin, skygroundmax;
	int          min, max;
	float        scalefactor;
	fileHandle_t f;
	byte         data;
	static int   lastDraw = 0;
	static int   tracecount = 0;
	int          ms;

	if ( !developer.integer )
	{
		CG_Printf ( "Can only generate a tracemap in developer mode.\n" );
		return;
	}

	if ( !cg.mapcoordsValid )
	{
		CG_Printf ( "Need valid mapcoords in the worldspawn to be able to generate a tracemap.\n" );
		return;
	}

	if ( ( cg.mapcoordsMaxs[ 0 ] - cg.mapcoordsMins[ 0 ] ) != ( cg.mapcoordsMins[ 1 ] - cg.mapcoordsMaxs[ 1 ] ) )
	{
		CG_Printf ( "Mapcoords need to be square.\n" );
		return;
	}

	// Topdown tracing
	CG_Printf ( "Generating level heightmap and level mask...\n" );

	memset ( &tracemap, 0, sizeof ( tracemap ) );

	topdownmax = MIN_WORLD_HEIGHT;
	topdownmin = MAX_WORLD_HEIGHT;

	// calculate the size of the level
	// ok, i'm lazy. Hijack commandmap extends for now and default to a TRACEMAP_SIZE by TRACEMAP_SIZE datablock
	x_step = ( cg.mapcoordsMaxs[ 0 ] - cg.mapcoordsMins[ 0 ] ) / ( float ) TRACEMAP_SIZE;
	y_step = ( cg.mapcoordsMaxs[ 1 ] - cg.mapcoordsMins[ 1 ] ) / ( float ) TRACEMAP_SIZE;

	for ( i = 0; i < TRACEMAP_SIZE; i++ )
	{
		start[ 0 ] = end[ 0 ] = cg.mapcoordsMins[ 0 ] + i * x_step;

		for ( j = 0; j < TRACEMAP_SIZE; j++ )
		{
			start[ 1 ] = end[ 1 ] = cg.mapcoordsMins[ 1 ] + j * y_step;
			start[ 2 ] = MAX_WORLD_HEIGHT;
			end[ 2 ] = MIN_WORLD_HEIGHT;

			// Find the ceiling
			CG_Trace ( &tr, start, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID | MASK_WATER );
			start[ 2 ] = tr.endpos[ 2 ] - 1;
			tracecount++;

			// Find ground
			while ( 1 )
			{
				// Perform traces up to the sky, repeating at a higher start height if we start
				// inside a solid.

				if ( start[ 2 ] <= MIN_WORLD_HEIGHT )
				{
					tracemap.ground[ j ][ i ] = MIN_WORLD_HEIGHT;
					break;
				}

				if ( end[ 2 ] <= MIN_WORLD_HEIGHT )
				{
					end[ 2 ] = MIN_WORLD_HEIGHT + 1;
				}

				CG_Trace ( &tr, start, NULL, NULL, end, ENTITYNUM_NONE, ( MASK_SOLID | MASK_WATER ) );
				tracecount++;

				if ( tr.startsolid )
				{
					// Stuck in something, skip over it.
					start[ 2 ] -= 64;
				}
				else if ( tr.fraction == 1 )
				{
					// Didn't hit anything, we're (probably) outside the world
					tracemap.ground[ j ][ i ] = MIN_WORLD_HEIGHT;
					break;
				}
				else
				{
					tracemap.ground[ j ][ i ] = tr.endpos[ 2 ];

					if ( ! ( tr.surfaceFlags & SURF_NODRAW ) )
					{
						if ( tracemap.ground[ j ][ i ] > topdownmax )
						{
							topdownmax = tracemap.ground[ j ][ i ];
						}

						if ( tracemap.ground[ j ][ i ] < topdownmin )
						{
							topdownmin = tracemap.ground[ j ][ i ];
						}
					}

					break;
				}
			}

			// bit hacky, get some console output going
			ms = trap_Milliseconds();

			if ( ! ( ( lastDraw <= ms ) && ( lastDraw > ms - 500 ) ) )
			{
				lastDraw = ms;

				CG_Printf ( "%i of %i gridpoints calculated (%.2f%%), %i total traces\n", i * TRACEMAP_SIZE + j,
				            TRACEMAP_SIZE * TRACEMAP_SIZE,
				            ( ( i * TRACEMAP_SIZE + j ) / ( float ) ( TRACEMAP_SIZE * TRACEMAP_SIZE ) ) * 100.f, tracecount );
				trap_UpdateScreen();
			}
		}
	}

	CG_Printf ( "%i of %i gridpoints calculated (%.2f%%), %i total traces\n", i * TRACEMAP_SIZE, TRACEMAP_SIZE * TRACEMAP_SIZE,
	            ( ( i * TRACEMAP_SIZE ) / ( float ) ( TRACEMAP_SIZE * TRACEMAP_SIZE ) ) * 100.f, tracecount );
	trap_UpdateScreen();

	// Sky tracing
	CG_Printf ( "Generating sky heightmap and sky mask...\n" );

	max = MIN_WORLD_HEIGHT;
	min = MAX_WORLD_HEIGHT;

	for ( i = 0; i < TRACEMAP_SIZE; i++ )
	{
		start[ 0 ] = end[ 0 ] = cg.mapcoordsMins[ 0 ] + i * x_step;

		for ( j = 0; j < TRACEMAP_SIZE; j++ )
		{
			start[ 1 ] = end[ 1 ] = cg.mapcoordsMins[ 1 ] + j * y_step;
			//start[2] = MIN_WORLD_HEIGHT;
			start[ 2 ] = tracemap.ground[ j ][ i ];
			end[ 2 ] = MAX_WORLD_HEIGHT;

			if ( start[ 2 ] == MIN_WORLD_HEIGHT )
			{
				// we got a hole here, no need to trace
				tracemap.sky[ j ][ i ] = MAX_WORLD_HEIGHT;
			}
			else
			{
				// Find sky
				while ( 1 )
				{
					// Perform traces up to the sky, repeating at a higher start height if we start
					// inside a solid.

					if ( start[ 2 ] >= MAX_WORLD_HEIGHT )
					{
						tracemap.sky[ j ][ i ] = MAX_WORLD_HEIGHT;
						break;
					}

					if ( end[ 2 ] >= MAX_WORLD_HEIGHT )
					{
						end[ 2 ] = MAX_WORLD_HEIGHT - 1;
					}

					CG_Trace ( &tr, start, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID );
					tracecount++;

					if ( tr.startsolid )
					{
						// Stuck in something, skip over it.
						// can happen, tr.endpos still is valid even if we're starting in a solid but trace out of it hitting the next surface
						if ( tr.surfaceFlags & SURF_SKY )
						{
							// are we in a solid?
							if ( ! ( CG_PointContents ( tr.endpos, ENTITYNUM_NONE ) & ( MASK_SOLID | MASK_WATER ) ) )
							{
								tracemap.sky[ j ][ i ] = tr.endpos[ 2 ];

								if ( tracemap.sky[ j ][ i ] > max )
								{
									max = tracemap.sky[ j ][ i ];
								}

								if ( tracemap.sky[ j ][ i ] < min )
								{
									min = tracemap.sky[ j ][ i ];
								}

								break;
							}
							else
							{
								// skip over it
								start[ 2 ] = tr.endpos[ 2 ] + 1;
							}
						}
						else
						{
							start[ 2 ] = tr.endpos[ 2 ] + 1;
						}
					}
					else if ( tr.fraction == 1 )
					{
						// Didn't hit anything, we're (probably) outside the world
						tracemap.sky[ j ][ i ] = MAX_WORLD_HEIGHT;
						break;
					}
					else if ( tr.surfaceFlags & SURF_SKY )
					{
						// Hit sky, this is where we start.
						tracemap.sky[ j ][ i ] = tr.endpos[ 2 ];

						if ( tracemap.sky[ j ][ i ] > max )
						{
							max = tracemap.sky[ j ][ i ];
						}

						if ( tracemap.sky[ j ][ i ] < min )
						{
							min = tracemap.sky[ j ][ i ];
						}

						break;
					}
					else
					{
						// hit something else, skip over it
						start[ 2 ] = tr.endpos[ 2 ] + 64;
					}
				}
			}

			// bit hacky, get some console output going
			ms = trap_Milliseconds();

			if ( ! ( ( lastDraw <= ms ) && ( lastDraw > ms - 500 ) ) )
			{
				lastDraw = ms;

				CG_Printf ( "%i of %i gridpoints calculated (%.2f%%), %i total traces\n", i * TRACEMAP_SIZE + j,
				            TRACEMAP_SIZE * TRACEMAP_SIZE,
				            ( ( i * TRACEMAP_SIZE + j ) / ( float ) ( TRACEMAP_SIZE * TRACEMAP_SIZE ) ) * 100.f, tracecount );
				trap_UpdateScreen();
			}
		}
	}

	CG_Printf ( "%i of %i gridpoints calculated (%.2f%%), %i total traces\n", i * TRACEMAP_SIZE, TRACEMAP_SIZE * TRACEMAP_SIZE,
	            ( ( i * TRACEMAP_SIZE ) / ( float ) ( TRACEMAP_SIZE * TRACEMAP_SIZE ) ) * 100.f, tracecount );
	trap_UpdateScreen();

	// More groundtrace, find ceilings for areas where we don't have ground
	CG_Printf ( "Generating sky groundmap...\n" );

	skygroundmin = MAX_WORLD_HEIGHT;
	skygroundmax = MIN_WORLD_HEIGHT;

	for ( i = 0; i < TRACEMAP_SIZE; i++ )
	{
		start[ 0 ] = end[ 0 ] = cg.mapcoordsMins[ 0 ] + i * x_step;

		for ( j = 0; j < TRACEMAP_SIZE; j++ )
		{
			start[ 1 ] = end[ 1 ] = cg.mapcoordsMins[ 1 ] + j * y_step;
			start[ 2 ] = MAX_WORLD_HEIGHT;
			end[ 2 ] = MIN_WORLD_HEIGHT;

			if ( tracemap.sky[ j ][ i ] == MAX_WORLD_HEIGHT && tracemap.ground[ j ][ i ] != MIN_WORLD_HEIGHT )
			{
				// Find the ceiling
				CG_Trace ( &tr, start, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID | MASK_WATER );
				tracecount++;

				if ( tr.fraction == 1 )
				{
					// Didn't hit anything, we're (probably) outside the world
					tracemap.skyground[ j ][ i ] = MIN_WORLD_HEIGHT;
				}
				else
				{
					tracemap.skyground[ j ][ i ] = tr.endpos[ 2 ];
				}
			}
			else
			{
				tracemap.skyground[ j ][ i ] = tracemap.ground[ j ][ i ];
			}

			if ( tracemap.skyground[ j ][ i ] != MIN_WORLD_HEIGHT )
			{
				if ( tracemap.skyground[ j ][ i ] > skygroundmax )
				{
					skygroundmax = tracemap.skyground[ j ][ i ];
				}

				if ( tracemap.skyground[ j ][ i ] < skygroundmin )
				{
					skygroundmin = tracemap.skyground[ j ][ i ];
				}
			}

			// bit hacky, get some console output going
			ms = trap_Milliseconds();

			if ( ! ( ( lastDraw <= ms ) && ( lastDraw > ms - 500 ) ) )
			{
				lastDraw = ms;

				CG_Printf ( "%i of %i gridpoints calculated (%.2f%%), %i total traces\n", i * TRACEMAP_SIZE + j,
				            TRACEMAP_SIZE * TRACEMAP_SIZE,
				            ( ( i * TRACEMAP_SIZE + j ) / ( float ) ( TRACEMAP_SIZE * TRACEMAP_SIZE ) ) * 100.f, tracecount );
				trap_UpdateScreen();
			}
		}
	}

	CG_Printf ( "%i of %i gridpoints calculated (%.2f%%), %i total traces\n", i * TRACEMAP_SIZE, TRACEMAP_SIZE * TRACEMAP_SIZE,
	            ( ( i * TRACEMAP_SIZE ) / ( float ) ( TRACEMAP_SIZE * TRACEMAP_SIZE ) ) * 100.f, tracecount );
	trap_UpdateScreen();

	// R: topdown mask
	// G: there is sky here yes/no mask
	// B: sky mask
	// A: there is map here yes/no mask

	// scale everything to a colour 256 range

	// min is 0
	// max is 255
	// rain - etmain REALLY expects 1 to 255, so I'm changing this to
	// generate that instead, so that etpro tracemaps can be used with
	// etmain
	scalefactor = 254.f / ( topdownmax - topdownmin );

	if ( scalefactor == 0.f )
	{
		scalefactor = 1.f;
	}

	for ( i = 0; i < TRACEMAP_SIZE; i++ )
	{
		for ( j = 0; j < TRACEMAP_SIZE; j++ )
		{
			if ( tracemap.ground[ i ][ j ] >= topdownmin )
			{
				tracemap.ground[ i ][ j ] = 1.0 + ( tracemap.ground[ i ][ j ] - topdownmin ) * scalefactor;
			}

			// rain - hard clamp because *min and *max are rounded :(
			if ( tracemap.ground[ i ][ j ] < 1.0 )
			{
				tracemap.ground[ i ][ j ] = 1.0;
			}
			else if ( tracemap.ground[ i ][ j ] > 255.0 )
			{
				tracemap.ground[ i ][ j ] = 255.0;
			}
		}
	}

	// min is 0
	// max is 255
	// rain - this is d&l, min=1 max=255
	scalefactor = 254.f / ( skygroundmax - skygroundmin );

	if ( scalefactor == 0.f )
	{
		scalefactor = 1.f;
	}

	for ( i = 0; i < TRACEMAP_SIZE; i++ )
	{
		for ( j = 0; j < TRACEMAP_SIZE; j++ )
		{
			if ( tracemap.skyground[ i ][ j ] >= skygroundmin )
			{
				tracemap.skyground[ i ][ j ] = 1.0 + ( tracemap.skyground[ i ][ j ] - skygroundmin ) * scalefactor;
			}

			// rain - hard clamp because *min and *max are rounded :(
			if ( tracemap.skyground[ i ][ j ] < 1.0 )
			{
				tracemap.skyground[ i ][ j ] = 1.0;
			}
			else if ( tracemap.skyground[ i ][ j ] > 255.0 )
			{
				tracemap.skyground[ i ][ j ] = 255.0;
			}
		}
	}

	// no sky is 0
	// min is 1
	// max is 255
	if ( max - min == 0 )
	{
		scalefactor = 1.f;
	}
	else
	{
		scalefactor = 254.f / ( max - min );
	}

	for ( i = 0; i < TRACEMAP_SIZE; i++ )
	{
		for ( j = 0; j < TRACEMAP_SIZE; j++ )
		{
			if ( tracemap.sky[ i ][ j ] == MAX_WORLD_HEIGHT )
			{
				tracemap.sky[ i ][ j ] = 0.f;
			}
			else
			{
				tracemap.sky[ i ][ j ] = 1.f + ( tracemap.sky[ i ][ j ] - min ) * scalefactor;
			}

			// rain - hard clamp because *min and *max are rounded :(
			if ( tracemap.sky[ i ][ j ] < 0.0 )
			{
				tracemap.sky[ i ][ j ] = 0.0;
			}
			else if ( tracemap.sky[ i ][ j ] > 255.0 )
			{
				tracemap.sky[ i ][ j ] = 255.0;
			}
		}
	}

	// write tga
	trap_FS_FOpenFile ( va ( "maps/%s_tracemap.tga", Q_strlwr ( cgs.rawmapname ) ), &f, FS_WRITE );

	// header
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 0
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 1
	data = 2;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 2 : uncompressed type
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 3
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 4
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 5
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 6
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 7
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 8
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 9
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 10
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 11
	data = TRACEMAP_SIZE & 255;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 12 : width
	data = TRACEMAP_SIZE >> 8;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 13 : width
	data = TRACEMAP_SIZE & 255;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 14 : height
	data = TRACEMAP_SIZE >> 8;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 15 : height
	data = 32;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 16 : pixel size
	data = 0;
	trap_FS_Write ( &data, sizeof ( data ), f ); // 17

	// R: topdown mask
	// G: there is sky here yes/no mask
	// B: sky mask
	// A: there is map here yes/no mask
	for ( i = 0; i < TRACEMAP_SIZE; i++ )
	{
		for ( j = 0; j < TRACEMAP_SIZE; j++ )
		{
			if ( i == 0 && j < 6 )
			{
				// abuse first six pixels for our extended data
				switch ( j )
				{
					case 0:
						trap_FS_Write ( &topdownmin, sizeof ( topdownmin ), f );
						break;

					case 1:
						trap_FS_Write ( &topdownmax, sizeof ( topdownmax ), f );
						break;

					case 2:
						trap_FS_Write ( &skygroundmin, sizeof ( skygroundmin ), f );
						break;

					case 3:
						trap_FS_Write ( &skygroundmax, sizeof ( skygroundmax ), f );
						break;

					case 4:
						trap_FS_Write ( &min, sizeof ( min ), f );
						break;

					case 5:
						trap_FS_Write ( &max, sizeof ( max ), f );
						break;
				}

				continue;
			}

			data = tracemap.sky[ TRACEMAP_SIZE - 1 - i ][ j ];
			trap_FS_Write ( &data, sizeof ( data ), f ); // b

			if ( tracemap.skyground[ TRACEMAP_SIZE - 1 - i ][ j ] == MIN_WORLD_HEIGHT )
			{
				data = 0;
				trap_FS_Write ( &data, sizeof ( data ), f ); // g
			}
			else
			{
				data = tracemap.skyground[ TRACEMAP_SIZE - 1 - i ][ j ];
				trap_FS_Write ( &data, sizeof ( data ), f ); // g
			}

			if ( tracemap.ground[ TRACEMAP_SIZE - 1 - i ][ j ] == MIN_WORLD_HEIGHT )
			{
				data = 0;
				trap_FS_Write ( &data, sizeof ( data ), f ); // r
				data = 0;
				trap_FS_Write ( &data, sizeof ( data ), f ); // a
			}
			else
			{
				data = tracemap.ground[ TRACEMAP_SIZE - 1 - i ][ j ];
				trap_FS_Write ( &data, sizeof ( data ), f ); // r
				data = 255;
				trap_FS_Write ( &data, sizeof ( data ), f ); // a
			}
		}
	}

	// footer
	i = 0;
	trap_FS_Write ( &i, sizeof ( i ), f ); // extension area offset, 4 bytes
	i = 0;
	trap_FS_Write ( &i, sizeof ( i ), f ); // developer directory offset, 4 bytes
	trap_FS_Write ( "TRUEVISION-XFILE.\0", 18, f );

	trap_FS_FCloseFile ( f );
}

#endif // CGAMEDLL

qboolean BG_LoadTraceMap ( char *rawmapname, vec2_t world_mins, vec2_t world_maxs )
{
	int          i, j;
	fileHandle_t f;
	byte         data, datablock[ TRACEMAP_SIZE ][ 4 ];
	int          sky_min, sky_max;
	int          ground_min, ground_max;
	int          skyground_min, skyground_max;
	float        scalefactor;

	//int startTime = trap_Milliseconds();

	ground_min = ground_max = MIN_WORLD_HEIGHT;
	skyground_min = skyground_max = MAX_WORLD_HEIGHT;
	sky_min = sky_max = MAX_WORLD_HEIGHT;

	if ( trap_FS_FOpenFile ( va ( "maps/%s_tracemap.tga", Q_strlwr ( rawmapname ) ), &f, FS_READ ) >= 0 )
	{
		// skip over header
		for ( i = 0; i < 18; i++ )
		{
			trap_FS_Read ( &data, 1, f );
		}

		for ( i = 0; i < TRACEMAP_SIZE; i++ )
		{
			trap_FS_Read ( &datablock, sizeof ( datablock ), f ); // TRACEMAP_SIZE * { b g r a }

			for ( j = 0; j < TRACEMAP_SIZE; j++ )
			{
				if ( i == 0 && j < 6 )
				{
					// abuse first six pixels for our extended data
					switch ( j )
					{
						case 0:
							ground_min =
							  datablock[ j ][ 0 ] | ( datablock[ j ][ 1 ] << 8 ) | ( datablock[ j ][ 2 ] << 16 ) | ( datablock[ j ][ 3 ] << 24 );
							break;

						case 1:
							ground_max =
							  datablock[ j ][ 0 ] | ( datablock[ j ][ 1 ] << 8 ) | ( datablock[ j ][ 2 ] << 16 ) | ( datablock[ j ][ 3 ] << 24 );
							break;

						case 2:
							skyground_min =
							  datablock[ j ][ 0 ] | ( datablock[ j ][ 1 ] << 8 ) | ( datablock[ j ][ 2 ] << 16 ) | ( datablock[ j ][ 3 ] << 24 );
							break;

						case 3:
							skyground_max =
							  datablock[ j ][ 0 ] | ( datablock[ j ][ 1 ] << 8 ) | ( datablock[ j ][ 2 ] << 16 ) | ( datablock[ j ][ 3 ] << 24 );
							break;

						case 4:
							sky_min =
							  datablock[ j ][ 0 ] | ( datablock[ j ][ 1 ] << 8 ) | ( datablock[ j ][ 2 ] << 16 ) | ( datablock[ j ][ 3 ] << 24 );
							break;

						case 5:
							sky_max =
							  datablock[ j ][ 0 ] | ( datablock[ j ][ 1 ] << 8 ) | ( datablock[ j ][ 2 ] << 16 ) | ( datablock[ j ][ 3 ] << 24 );
							break;
					}

					tracemap.sky[ TRACEMAP_SIZE - 1 - i ][ j ] = MAX_WORLD_HEIGHT;
					tracemap.skyground[ TRACEMAP_SIZE - 1 - i ][ j ] = MAX_WORLD_HEIGHT;
					tracemap.ground[ TRACEMAP_SIZE - 1 - i ][ j ] = MIN_WORLD_HEIGHT;
					continue;
				}

				tracemap.sky[ TRACEMAP_SIZE - 1 - i ][ j ] = ( float ) datablock[ j ][ 0 ]; // FIXME: swap

				if ( tracemap.sky[ TRACEMAP_SIZE - 1 - i ][ j ] == 0 )
				{
					tracemap.sky[ TRACEMAP_SIZE - 1 - i ][ j ] = MAX_WORLD_HEIGHT;
				}

				tracemap.skyground[ TRACEMAP_SIZE - 1 - i ][ j ] = ( float ) datablock[ j ][ 1 ]; // FIXME: swap

				if ( tracemap.skyground[ TRACEMAP_SIZE - 1 - i ][ j ] == 0 )
				{
					tracemap.skyground[ TRACEMAP_SIZE - 1 - i ][ j ] = MAX_WORLD_HEIGHT;
				}

				tracemap.ground[ TRACEMAP_SIZE - 1 - i ][ j ] = ( float ) datablock[ j ][ 2 ]; // FIXME: swap

				if ( tracemap.ground[ TRACEMAP_SIZE - 1 - i ][ j ] == 0 )
				{
					tracemap.ground[ TRACEMAP_SIZE - 1 - i ][ j ] = MIN_WORLD_HEIGHT;
				}

				if ( datablock[ j ][ 3 ] == 0 )
				{
					// just in case
					tracemap.skyground[ TRACEMAP_SIZE - 1 - i ][ j ] = MAX_WORLD_HEIGHT;
					tracemap.ground[ TRACEMAP_SIZE - 1 - i ][ j ] = MIN_WORLD_HEIGHT;
				}
			}

			/*for( j = 0; j < TRACEMAP_SIZE; j++ ) {
			   if( i == 0 && j < 6 ) {
			   // abuse first six pixels for our extended data
			   switch( j ) {
			   case 0:  trap_FS_Read( &ground_min, sizeof(ground_min), f ); break;
			   case 1: trap_FS_Read( &ground_max, sizeof(ground_max), f ); break;
			   case 2:  trap_FS_Read( &skyground_min, sizeof(skyground_min), f ); break;
			   case 3: trap_FS_Read( &skyground_max, sizeof(skyground_max), f ); break;
			   case 4: trap_FS_Read( &sky_min, sizeof(sky_min), f ); break;
			   case 5: trap_FS_Read( &sky_max, sizeof(sky_max), f ); break;
			   }
			   tracemap.sky[TRACEMAP_SIZE - 1 - i][j] = MAX_WORLD_HEIGHT;
			   tracemap.skyground[TRACEMAP_SIZE - 1 - i][j] = MAX_WORLD_HEIGHT;
			   tracemap.ground[TRACEMAP_SIZE - 1 - i][j] = MIN_WORLD_HEIGHT;
			   continue;
			   }

			   trap_FS_Read( &datablock, sizeof(datablock), f );    // b g r a
			   tracemap.sky[TRACEMAP_SIZE - 1 - i][j] = (float)datablock[0];    // FIXME: swap
			   if( tracemap.sky[TRACEMAP_SIZE - 1 - i][j] == 0 )
			   tracemap.sky[TRACEMAP_SIZE - 1 - i][j] = MAX_WORLD_HEIGHT;

			   //trap_FS_Read( &data, 1, f ); // g
			   tracemap.skyground[TRACEMAP_SIZE - 1 - i][j] = (float)datablock[1];  // FIXME: swap
			   if( tracemap.skyground[TRACEMAP_SIZE - 1 - i][j] == 0 )
			   tracemap.skyground[TRACEMAP_SIZE - 1 - i][j] = MAX_WORLD_HEIGHT;

			   //trap_FS_Read( &data, sizeof(data), f );    // r
			   tracemap.ground[TRACEMAP_SIZE - 1 - i][j] = (float)datablock[2]; // FIXME: swap
			   if( tracemap.ground[TRACEMAP_SIZE - 1 - i][j] == 0 )
			   tracemap.ground[TRACEMAP_SIZE - 1 - i][j] = MIN_WORLD_HEIGHT;

			   //trap_FS_Read( &data, sizeof(data), f ); // a
			   if( datablock[3] == 0 ) {
			   // just in case
			   tracemap.skyground[TRACEMAP_SIZE - 1 - i][j] = MAX_WORLD_HEIGHT;
			   tracemap.ground[TRACEMAP_SIZE - 1 - i][j] = MIN_WORLD_HEIGHT;
			   }
			   } */
		}

		trap_FS_FCloseFile ( f );

		// Ground
		// calculate scalefactor
		if ( ground_max - ground_min == 0 )
		{
			scalefactor = 1.f;
		}
		else
		{
			// rain - scalefactor 254 to compensate for broken etmain behavior
			scalefactor = 254.f / ( ground_max - ground_min );
		}

		// scale properly
		for ( i = 0; i < TRACEMAP_SIZE; i++ )
		{
			for ( j = 0; j < TRACEMAP_SIZE; j++ )
			{
				if ( tracemap.ground[ i ][ j ] != MIN_WORLD_HEIGHT )
				{
					tracemap.ground[ i ][ j ] = ground_min + ( tracemap.ground[ i ][ j ] / scalefactor );
				}
			}
		}

		// SkyGround
		// calculate scalefactor
		if ( skyground_max - skyground_min == 0 )
		{
			scalefactor = 1.f;
		}
		else
		{
			// rain - scalefactor 254 to compensate for broken etmain behavior
			scalefactor = 254.f / ( skyground_max - skyground_min );
		}

		// scale properly
		for ( i = 0; i < TRACEMAP_SIZE; i++ )
		{
			for ( j = 0; j < TRACEMAP_SIZE; j++ )
			{
				if ( tracemap.skyground[ i ][ j ] != MAX_WORLD_HEIGHT )
				{
					tracemap.skyground[ i ][ j ] = skyground_min + ( tracemap.skyground[ i ][ j ] / scalefactor );
				}
			}
		}

		// Sky
		// calculate scalefactor
		if ( sky_max - sky_min == 0 )
		{
			scalefactor = 1.f;
		}
		else
		{
			// rain - scalefactor 254 to compensate for broken etmain behavior
			scalefactor = 254.f / ( sky_max - sky_min );
		}

		// scale properly
		for ( i = 0; i < TRACEMAP_SIZE; i++ )
		{
			for ( j = 0; j < TRACEMAP_SIZE; j++ )
			{
				if ( tracemap.sky[ i ][ j ] != MAX_WORLD_HEIGHT )
				{
					tracemap.sky[ i ][ j ] = sky_min + ( tracemap.sky[ i ][ j ] / scalefactor );
				}
			}
		}
	}
	else
	{
		return ( tracemap.loaded = qfalse );
	}

	tracemap.world_mins[ 0 ] = world_mins[ 0 ];
	tracemap.world_mins[ 1 ] = world_mins[ 1 ];
	tracemap.world_maxs[ 0 ] = world_maxs[ 0 ];
	tracemap.world_maxs[ 1 ] = world_maxs[ 1 ];

	one_over_mapgrid_factor[ 0 ] = 1.f / ( ( tracemap.world_maxs[ 0 ] - tracemap.world_mins[ 0 ] ) / ( float ) TRACEMAP_SIZE );
	one_over_mapgrid_factor[ 1 ] = 1.f / ( ( tracemap.world_maxs[ 1 ] - tracemap.world_mins[ 1 ] ) / ( float ) TRACEMAP_SIZE );

	tracemap.groundfloor = ground_min;
	tracemap.groundceil = ground_max;

	//Com_Printf( "^8Loaded tracemap in %i msec\n", trap_Milliseconds() - startTime );

	return ( tracemap.loaded = qtrue );
}

static void BG_ClampPointToTracemapExtends ( vec3_t point, vec2_t out )
{
	if ( point[ 0 ] < tracemap.world_mins[ 0 ] )
	{
		out[ 0 ] = tracemap.world_mins[ 0 ];
	}
	else if ( point[ 0 ] > tracemap.world_maxs[ 0 ] )
	{
		out[ 0 ] = tracemap.world_maxs[ 0 ];
	}
	else
	{
		out[ 0 ] = point[ 0 ];
	}

	if ( point[ 1 ] < tracemap.world_maxs[ 1 ] )
	{
		out[ 1 ] = tracemap.world_maxs[ 1 ];
	}
	else if ( point[ 1 ] > tracemap.world_mins[ 1 ] )
	{
		out[ 1 ] = tracemap.world_mins[ 1 ];
	}
	else
	{
		out[ 1 ] = point[ 1 ];
	}
}

float BG_GetSkyHeightAtPoint ( vec3_t pos )
{
	int    i, j;
	vec2_t point;

//  int msec = trap_Milliseconds();

//  n_getskytime++;

	if ( !tracemap.loaded )
	{
//      getskytime += trap_Milliseconds() - msec;
		return MAX_WORLD_HEIGHT;
	}

	BG_ClampPointToTracemapExtends ( pos, point );

	i = myftol ( ( point[ 0 ] - tracemap.world_mins[ 0 ] ) * one_over_mapgrid_factor[ 0 ] );
	j = myftol ( ( point[ 1 ] - tracemap.world_mins[ 1 ] ) * one_over_mapgrid_factor[ 1 ] );

	// rain - re-clamp the points, because a rounding error can cause
	// them to go outside the array
	etpro_FinalizeTracemapClamp ( &i, &j );

//  getskytime += trap_Milliseconds() - msec;
	return ( tracemap.sky[ j ][ i ] );
}

float BG_GetSkyGroundHeightAtPoint ( vec3_t pos )
{
	int    i, j;
	vec2_t point;

//  int msec = trap_Milliseconds();

//  n_getgroundtime++;

	if ( !tracemap.loaded )
	{
//      getgroundtime += trap_Milliseconds() - msec;
		return MAX_WORLD_HEIGHT;
	}

	BG_ClampPointToTracemapExtends ( pos, point );

	i = myftol ( ( point[ 0 ] - tracemap.world_mins[ 0 ] ) * one_over_mapgrid_factor[ 0 ] );
	j = myftol ( ( point[ 1 ] - tracemap.world_mins[ 1 ] ) * one_over_mapgrid_factor[ 1 ] );

	// rain - re-clamp the points, because a rounding error can cause
	// them to go outside the array
	etpro_FinalizeTracemapClamp ( &i, &j );

//  getgroundtime += trap_Milliseconds() - msec;
	return ( tracemap.skyground[ j ][ i ] );
}

float BG_GetGroundHeightAtPoint ( vec3_t pos )
{
	int    i, j;
	vec2_t point;

//  int msec = trap_Milliseconds();

//  n_getgroundtime++;

	if ( !tracemap.loaded )
	{
//      getgroundtime += trap_Milliseconds() - msec;
		return MIN_WORLD_HEIGHT;
	}

	BG_ClampPointToTracemapExtends ( pos, point );

	i = myftol ( ( point[ 0 ] - tracemap.world_mins[ 0 ] ) * one_over_mapgrid_factor[ 0 ] );
	j = myftol ( ( point[ 1 ] - tracemap.world_mins[ 1 ] ) * one_over_mapgrid_factor[ 1 ] );

	// rain - re-clamp the points, because a rounding error can cause
	// them to go outside the array
	etpro_FinalizeTracemapClamp ( &i, &j );

//  getgroundtime += trap_Milliseconds() - msec;
	return ( tracemap.ground[ j ][ i ] );
}

int BG_GetTracemapGroundFloor ( void )
{
	if ( !tracemap.loaded )
	{
		return MIN_WORLD_HEIGHT;
	}

	return tracemap.groundfloor;
}

int BG_GetTracemapGroundCeil ( void )
{
	if ( !tracemap.loaded )
	{
		return MAX_WORLD_HEIGHT;
	}

	return tracemap.groundceil;
}

// rain - re-clamp the points, because a rounding error can cause
// them to go outside the array
void etpro_FinalizeTracemapClamp ( int *x, int *y )
{
	if ( *x < 0 )
	{
		*x = 0;
	}
	else if ( *x > TRACEMAP_SIZE - 1 )
	{
		*x = TRACEMAP_SIZE - 1;
	}

	if ( *y < 0 )
	{
		*y = 0;
	}
	else if ( *y > TRACEMAP_SIZE - 1 )
	{
		*y = TRACEMAP_SIZE - 1;
	}
}
