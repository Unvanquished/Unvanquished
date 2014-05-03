/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>
Copyright (C) 2009 Peter McNeill <n27@bigpond.net.au>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_bsp.c
#include "tr_local.h"
#include "../../common/Maths.h"

/*
========================================================
Loads and prepares a map file for scene rendering.

A single entry point:
RE_LoadWorldMap(const char *name);
========================================================
*/

static world_t    s_worldData;
static int        s_lightCount;
static growList_t s_interactions;
static byte       *fileBase;

static int        c_redundantInteractions;
static int        c_vboWorldSurfaces;
static int        c_vboLightSurfaces;
static int        c_vboShadowSurfaces;

//===============================================================================

/*
===============
R_ColorShiftLightingBytes
===============
*/
static void R_ColorShiftLightingBytes( byte in[ 4 ], byte out[ 4 ] )
{
	int shift, r, g, b;

	// shift the color data based on overbright range
	shift = tr.mapOverBrightBits - tr.overbrightBits;

	// shift the data based on overbright range
	r = in[ 0 ] << shift;
	g = in[ 1 ] << shift;
	b = in[ 2 ] << shift;

	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 )
	{
		int max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[ 0 ] = r;
	out[ 1 ] = g;
	out[ 2 ] = b;
	out[ 3 ] = in[ 3 ];
}

static void R_ColorShiftLightingBytesCompressed( byte in[ 8 ], byte out[ 8 ] )
{
	unsigned short rgb565;
	byte rgba[4];

	// color shift the endpoint colors in the dxt block
	rgb565 = in[1] << 8 | in[0];
	rgba[0] = (rgb565 >> 8) & 0xf8;
	rgba[1] = (rgb565 >> 3) & 0xfc;
	rgba[2] = (rgb565 << 3) & 0xf8;
	rgba[3] = 0xff;
	R_ColorShiftLightingBytes( rgba, rgba );
	rgb565 = ((rgba[0] >> 3) << 11) |
		((rgba[1] >> 2) << 5) |
		((rgba[2] >> 3) << 0);
	out[0] = rgb565 & 0xff;
	out[1] = rgb565 >> 8;

	rgb565 = in[3] << 8 | in[2];
	rgba[0] = (rgb565 >> 8) & 0xf8;
	rgba[1] = (rgb565 >> 3) & 0xfc;
	rgba[2] = (rgb565 << 3) & 0xf8;
	rgba[3] = 0xff;
	R_ColorShiftLightingBytes( rgba, rgba );
	rgb565 = ((rgba[0] >> 3) << 11) |
		((rgba[1] >> 2) << 5) |
		((rgba[2] >> 3) << 0);
	out[2] = rgb565 & 0xff;
	out[3] = rgb565 >> 8;
}

/*
===============
R_ColorShiftLightingFloats
===============
*/
static void R_ColorShiftLightingFloats( const vec4_t in, vec4_t out )
{
	int shift, r, g, b;

	// shift the color data based on overbright range
	shift = tr.mapOverBrightBits - tr.overbrightBits;

	// shift the data based on overbright range
	r = ( ( byte )( in[ 0 ] * 255 ) ) << shift;
	g = ( ( byte )( in[ 1 ] * 255 ) ) << shift;
	b = ( ( byte )( in[ 2 ] * 255 ) ) << shift;

	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 )
	{
		int max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[ 0 ] = r * ( 1.0f / 255.0f );
	out[ 1 ] = g * ( 1.0f / 255.0f );
	out[ 2 ] = b * ( 1.0f / 255.0f );
	out[ 3 ] = in[ 3 ];
}

/*
===============
R_ProcessLightmap

        returns maxIntensity
===============
*/
float R_ProcessLightmap( byte *pic, int in_padding, int width, int height, int bits, byte *pic_out )
{
	int   j;
	float maxIntensity = 0;

	if( bits & IF_BC1 ) {
		for ( j = 0; j < ((width + 3) >> 2) * ((height + 3) >> 2); j++ )
		{
			R_ColorShiftLightingBytesCompressed( &pic[ j * 8 ], &pic_out[ j * 8 ] );
		}
	} else if( bits & IF_BC3 ) {
		for ( j = 0; j < ((width + 3) >> 2) * ((height + 3) >> 2); j++ )
		{
			R_ColorShiftLightingBytesCompressed( &pic[ j * 16 ], &pic_out[ j * 16 ] );
		}
	} else {
		for ( j = 0; j < width * height; j++ )
		{
			R_ColorShiftLightingBytes( &pic[ j * in_padding ], &pic_out[ j * 4 ] );
			pic_out[ j * 4 + 3 ] = 255;
		}
	}

	return maxIntensity;
}

static int QDECL LightmapNameCompare( const void *a, const void *b )
{
	char *s1, *s2;
	int  c1, c2;

	s1 = * ( char ** ) a;
	s2 = * ( char ** ) b;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if ( c1 >= 'a' && c1 <= 'z' )
		{
			c1 -= ( 'a' - 'A' );
		}

		if ( c2 >= 'a' && c2 <= 'z' )
		{
			c2 -= ( 'a' - 'A' );
		}

		if ( c1 == '\\' || c1 == ':' )
		{
			c1 = '/';
		}

		if ( c2 == '\\' || c2 == ':' )
		{
			c2 = '/';
		}

		if ( c1 < c2 )
		{
			// strings not equal
			return -1;
		}

		if ( c1 > c2 )
		{
			return 1;
		}
	}
	while ( c1 );

	// strings are equal
	return 0;
}

/* standard conversion from rgbe to float pixels */
/* note: Ward uses ldexp(col+0.5,exp-(128+8)).  However we wanted pixels */
/*       in the range [0,1] to map back into the range [0,1].            */
static INLINE void rgbe2float( float *red, float *green, float *blue, unsigned char rgbe[ 4 ] )
{
	float e;
	float f;

	if ( rgbe[ 3 ] )
	{
		/*nonzero pixel */
		f = ldexp( 1.0, rgbe[ 3 ] - ( int )( 128 + 8 ) );
		e = ( rgbe[ 3 ] - 128 ) / 4.0f;

		// RB: exp2 not defined by MSVC
		//f = exp2(e);
		f = pow( 2, e );

		*red = ( rgbe[ 0 ] / 255.0f ) * f;
		*green = ( rgbe[ 1 ] / 255.0f ) * f;
		*blue = ( rgbe[ 2 ] / 255.0f ) * f;
	}
	else
	{
		*red = *green = *blue = 0.0;
	}
}

void LoadRGBEToFloats( const char *name, float **pic, int *width, int *height )
{
	int      i, j;
	byte     *buf_p;
	byte     *buffer;
	float    *floatbuf;
	char     *token;
	int      w, h, c;
	qboolean formatFound;
	float        exposure = 1.6;
	const vec3_t LUMINANCE_VECTOR = { 0.2125f, 0.7154f, 0.0721f };
	float        luminance;
	float        avgLuminance;
	float        maxLuminance;
	float        scaledLuminance;
	float        finalLuminance;
	double       sum;
	float        gamma;

	union
	{
		byte  b[ 4 ];
		float f;
	} sample;

	vec4_t sampleVector;

	*pic = NULL;

	// load the file
	ri.FS_ReadFile( ( char * ) name, ( void ** ) &buffer );

	if ( !buffer )
	{
		ri.Error( ERR_DROP, "LoadRGBE: '%s' not found", name );
	}

	buf_p = buffer;

	formatFound = qfalse;
	w = h = 0;

	while ( qtrue )
	{
		token = COM_ParseExt2( ( char ** ) &buf_p, qtrue );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( !Q_stricmp( token, "FORMAT" ) )
		{
			token = COM_ParseExt2( ( char ** ) &buf_p, qfalse );

			if ( !Q_stricmp( token, "=" ) )
			{
				token = COM_ParseExt2( ( char ** ) &buf_p, qfalse );

				if ( !Q_stricmp( token, "32" ) )
				{
					token = COM_ParseExt2( ( char ** ) &buf_p, qfalse );

					if ( !Q_stricmp( token, "-" ) )
					{
						token = COM_ParseExt2( ( char ** ) &buf_p, qfalse );

						if ( !Q_stricmp( token, "bit_rle_rgbe" ) )
						{
							formatFound = qtrue;
						}
						else
						{
							ri.Printf( PRINT_ALL, "LoadRGBE: Expected 'bit_rle_rgbe' found instead '%s'\n", token );
						}
					}
					else
					{
						ri.Printf( PRINT_ALL, "LoadRGBE: Expected '-' found instead '%s'\n", token );
					}
				}
				else
				{
					ri.Printf( PRINT_ALL, "LoadRGBE: Expected '32' found instead '%s'\n", token );
				}
			}
			else
			{
				ri.Printf( PRINT_ALL, "LoadRGBE: Expected '=' found instead '%s'\n", token );
			}
		}

		if ( !Q_stricmp( token, "-" ) )
		{
			token = COM_ParseExt2( ( char ** ) &buf_p, qfalse );

			if ( !Q_stricmp( token, "Y" ) )
			{
				token = COM_ParseExt2( ( char ** ) &buf_p, qfalse );
				w = atoi( token );

				token = COM_ParseExt2( ( char ** ) &buf_p, qfalse );

				if ( !Q_stricmp( token, "+" ) )
				{
					token = COM_ParseExt2( ( char ** ) &buf_p, qfalse );

					if ( !Q_stricmp( token, "X" ) )
					{
						token = COM_ParseExt2( ( char ** ) &buf_p, qfalse );
						h = atoi( token );
						break;
					}
					else
					{
						ri.Printf( PRINT_ALL, "LoadRGBE: Expected 'X' found instead '%s'\n", token );
					}
				}
				else
				{
					ri.Printf( PRINT_ALL, "LoadRGBE: Expected '+' found instead '%s'\n", token );
				}
			}
			else
			{
				ri.Printf( PRINT_ALL, "LoadRGBE: Expected 'Y' found instead '%s'\n", token );
			}
		}
	}

	// go to the first byte
	while ( ( c = *buf_p++ ) != 0 )
	{
		if ( c == '\n' )
		{
			break;
		}
	}

	if ( width )
	{
		*width = w;
	}

	if ( height )
	{
		*height = h;
	}

	if ( !formatFound )
	{
		ri.FS_FreeFile( buffer );
		ri.Error( ERR_DROP, "LoadRGBE: %s has no format", name );
	}

	if ( !w || !h )
	{
		ri.FS_FreeFile( buffer );
		ri.Error( ERR_DROP, "LoadRGBE: %s has an invalid image size", name );
	}

	*pic = (float*) Com_Allocate( w * h * 3 * sizeof( float ) );
	floatbuf = *pic;

	for ( i = 0; i < ( w * h ); i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			sample.b[ 0 ] = *buf_p++;
			sample.b[ 1 ] = *buf_p++;
			sample.b[ 2 ] = *buf_p++;
			sample.b[ 3 ] = *buf_p++;

			*floatbuf++ = sample.f / 255.0f; // FIXME XMap2's output is 255 times too high
		}
	}

	ri.FS_FreeFile( buffer );
}

static void LoadRGBEToBytes( const char *name, byte **ldrImage, int *width, int *height )
{
	int    i, j;
	int    w, h;
	float  *hdrImage;
	float  *floatbuf;
	byte   *pixbuf;
	vec3_t sample;
	float  max;

	w = h = 0;
	LoadRGBEToFloats( name, &hdrImage, &w, &h );

	*width = w;
	*height = h;

	*ldrImage = (byte*) ri.Z_Malloc( w * h * 4 );
	pixbuf = *ldrImage;

	floatbuf = hdrImage;

	for ( i = 0; i < ( w * h ); i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			sample[ j ] = *floatbuf++ * 255.0f;
		}

		// clamp with color normalization
		max = sample[ 0 ];

		if ( sample[ 1 ] > max )
		{
			max = sample[ 1 ];
		}

		if ( sample[ 2 ] > max )
		{
			max = sample[ 2 ];
		}

		if ( max > 255.0f )
		{
			VectorScale( sample, ( 255.0f / max ), sample );
		}

		*pixbuf++ = ( byte ) sample[ 0 ];
		*pixbuf++ = ( byte ) sample[ 1 ];
		*pixbuf++ = ( byte ) sample[ 2 ];
		*pixbuf++ = ( byte ) 255;
	}

	Com_Dealloc( hdrImage );
}

void LoadRGBEToHalfs( const char *name, unsigned short **halfImage, int *width, int *height );

/*
===============
R_LoadLightmaps
===============
*/
#define LIGHTMAP_SIZE 128
static void R_LoadLightmaps( lump_t *l, const char *bspName )
{
	int     len;
	image_t *image;
	int     i;
	int     numLightmaps;

	tr.fatLightmapSize = 0;

	len = l->filelen;

	if ( !len )
	{
		char mapName[ MAX_QPATH ];
		char **lightmapFiles;

		Q_strncpyz( mapName, bspName, sizeof( mapName ) );
		COM_StripExtension3( mapName, mapName, sizeof( mapName ) );

		if ( tr.worldHDR_RGBE )
		{
			// we are about to upload textures
			R_SyncRenderThread();

			// load HDR lightmaps
			lightmapFiles = ri.FS_ListFiles( mapName, ".hdr", &numLightmaps );

			qsort( lightmapFiles, numLightmaps, sizeof( char * ), LightmapNameCompare );

			if ( !lightmapFiles || !numLightmaps )
			{
				ri.Printf( PRINT_WARNING, "WARNING: no lightmap files found\n" );
				return;
			}

			int  width, height;
			byte *ldrImage;

			for ( i = 0; i < numLightmaps; i++ )
			{
				ri.Printf( PRINT_DEVELOPER, "...loading external lightmap as RGB8 LDR '%s/%s'\n", mapName, lightmapFiles[ i ] );

				width = height = 0;
				LoadRGBEToBytes( va( "%s/%s", mapName, lightmapFiles[ i ] ), &ldrImage, &width, &height );

				image = R_CreateImage( va( "%s/%s", mapName, lightmapFiles[ i ] ), (const byte **)&ldrImage, width, height,
									   1, IF_NOPICMIP | IF_LIGHTMAP | IF_NOCOMPRESSION, FT_DEFAULT, WT_CLAMP );

				Com_AddToGrowList( &tr.lightmaps, image );

				ri.Free( ldrImage );
			}

			if ( tr.worldDeluxeMapping )
			{
				// load deluxemaps
				lightmapFiles = ri.FS_ListFiles( mapName, ".png", &numLightmaps );

				if ( !lightmapFiles || !numLightmaps )
				{
					lightmapFiles = ri.FS_ListFiles( mapName, ".tga", &numLightmaps );

					if ( !lightmapFiles || !numLightmaps )
					{
						lightmapFiles = ri.FS_ListFiles( mapName, ".webp", &numLightmaps );

						if ( !lightmapFiles || !numLightmaps )
						{
							lightmapFiles = ri.FS_ListFiles( mapName, ".crn", &numLightmaps );

							if ( !lightmapFiles || !numLightmaps )
							{
								ri.Printf( PRINT_WARNING, "WARNING: no lightmap files found\n" );
								return;
							}
						}
					}
				}

				qsort( lightmapFiles, numLightmaps, sizeof( char * ), LightmapNameCompare );

				ri.Printf( PRINT_DEVELOPER, "...loading %i deluxemaps\n", numLightmaps );

				for ( i = 0; i < numLightmaps; i++ )
				{
					ri.Printf( PRINT_DEVELOPER, "...loading external lightmap '%s/%s'\n", mapName, lightmapFiles[ i ] );

					image = R_FindImageFile( va( "%s/%s", mapName, lightmapFiles[ i ] ), IF_NORMALMAP | IF_NOCOMPRESSION, FT_DEFAULT, WT_CLAMP, NULL );
					Com_AddToGrowList( &tr.deluxemaps, image );
				}
			}
		}
		else
		{
			lightmapFiles = ri.FS_ListFiles( mapName, ".png", &numLightmaps );

			if ( !lightmapFiles || !numLightmaps )
			{
				lightmapFiles = ri.FS_ListFiles( mapName, ".tga", &numLightmaps );

				if ( !lightmapFiles || !numLightmaps )
				{
					lightmapFiles = ri.FS_ListFiles( mapName, ".webp", &numLightmaps );

					if ( !lightmapFiles || !numLightmaps )
					{
						lightmapFiles = ri.FS_ListFiles( mapName, ".crn", &numLightmaps );

						if ( !lightmapFiles || !numLightmaps )
						{
							ri.Printf( PRINT_WARNING, "WARNING: no lightmap files found\n" );
							return;
						}
					}
				}
			}

			qsort( lightmapFiles, numLightmaps, sizeof( char * ), LightmapNameCompare );

			ri.Printf( PRINT_DEVELOPER, "...loading %i lightmaps\n", numLightmaps );

			// we are about to upload textures
			R_SyncRenderThread();

			for ( i = 0; i < numLightmaps; i++ )
			{
				ri.Printf( PRINT_DEVELOPER, "...loading external lightmap '%s/%s'\n", mapName, lightmapFiles[ i ] );

				if ( tr.worldDeluxeMapping )
				{
					if ( i % 2 == 0 )
					{
						image = R_FindImageFile( va( "%s/%s", mapName, lightmapFiles[ i ] ), IF_LIGHTMAP | IF_NOCOMPRESSION, FT_LINEAR, WT_CLAMP, NULL );
						Com_AddToGrowList( &tr.lightmaps, image );
					}
					else
					{
						image = R_FindImageFile( va( "%s/%s", mapName, lightmapFiles[ i ] ), IF_NORMALMAP | IF_NOCOMPRESSION, FT_LINEAR, WT_CLAMP, NULL );
						Com_AddToGrowList( &tr.deluxemaps, image );
					}
				}
				else
				{
					image = R_FindImageFile( va( "%s/%s", mapName, lightmapFiles[ i ] ), IF_LIGHTMAP | IF_NOCOMPRESSION, FT_LINEAR, WT_CLAMP, NULL );
					Com_AddToGrowList( &tr.lightmaps, image );
				}
			}
		}
	}

	else
	{
		int  i;
		byte *buf, *buf_p;

		//int       BIGSIZE=2048;
		//int       BIGNUM=16;

		byte *fatbuffer;
		int  xoff, yoff, x, y;
		//float           scale = 0.9f;

		tr.fatLightmapSize = 2048;
		tr.fatLightmapStep = 16;

		len = l->filelen;

		if ( !len )
		{
			return;
		}

		buf = fileBase + l->fileofs;

		// we are about to upload textures
		R_SyncRenderThread();

		// create all the lightmaps
		numLightmaps = len / ( LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3 );

		if ( numLightmaps == 1 )
		{
			//FIXME: HACK: maps with only one lightmap turn up fullbright for some reason.
			//this hack avoids that scenario, but isn't the correct solution.
			numLightmaps++;
		}
		else if ( numLightmaps >= MAX_LIGHTMAPS )
		{
			// 20051020 misantropia
			ri.Printf( PRINT_WARNING, "WARNING: number of lightmaps > MAX_LIGHTMAPS\n" );
			numLightmaps = MAX_LIGHTMAPS;
		}

		if ( numLightmaps < 65 )
		{
			// optimize: use a 1024 if we can get away with it
			tr.fatLightmapSize = 1024;
			tr.fatLightmapStep = 8;
		}

		fatbuffer = (byte*) ri.Hunk_AllocateTempMemory( sizeof( byte ) * tr.fatLightmapSize * tr.fatLightmapSize * 4 );

		Com_Memset( fatbuffer, 128, tr.fatLightmapSize * tr.fatLightmapSize * 4 );

		for ( i = 0; i < numLightmaps; i++ )
		{
			// expand the 24 bit on-disk to 32 bit
			buf_p = buf + i * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;

			xoff = i % tr.fatLightmapStep;
			yoff = i / tr.fatLightmapStep;

			if ( 1 )
			{
				for ( y = 0; y < LIGHTMAP_SIZE; y++ )
				{
					for ( x = 0; x < LIGHTMAP_SIZE; x++ )
					{
						int index =
						  ( x + ( y * tr.fatLightmapSize ) ) + ( ( xoff * LIGHTMAP_SIZE ) + ( yoff * tr.fatLightmapSize * LIGHTMAP_SIZE ) );
						fatbuffer[( index * 4 ) + 0 ] = buf_p[( ( x + ( y * LIGHTMAP_SIZE ) ) * 3 ) + 0 ];
						fatbuffer[( index * 4 ) + 1 ] = buf_p[( ( x + ( y * LIGHTMAP_SIZE ) ) * 3 ) + 1 ];
						fatbuffer[( index * 4 ) + 2 ] = buf_p[( ( x + ( y * LIGHTMAP_SIZE ) ) * 3 ) + 2 ];
						fatbuffer[( index * 4 ) + 3 ] = 255;

						R_ColorShiftLightingBytes( &fatbuffer[( index * 4 ) + 0 ], &fatbuffer[( index * 4 ) + 0 ] );
					}
				}
			}
		}

		tr.fatLightmap = R_CreateImage( va( "_fatlightmap%d", 0 ), (const byte **)&fatbuffer,
						tr.fatLightmapSize, tr.fatLightmapSize, 1,
						IF_LIGHTMAP | IF_NOCOMPRESSION, FT_DEFAULT, WT_CLAMP );
		Com_AddToGrowList( &tr.lightmaps, tr.fatLightmap );

		ri.Hunk_FreeTempMemory( fatbuffer );
	}
}

static float FatPackU( float input, int lightmapnum )
{
	if ( tr.fatLightmapSize > 0 )
	{
		int x = lightmapnum % tr.fatLightmapStep;

		return ( input / ( ( float ) tr.fatLightmapStep ) ) + ( ( 1.0 / ( ( float ) tr.fatLightmapStep ) ) * ( float ) x );
	}

	return input;
}

static float FatPackV( float input, int lightmapnum )
{
	if ( tr.fatLightmapSize > 0 )
	{
		int y = lightmapnum / ( ( float ) tr.fatLightmapStep );

		return ( input / ( ( float ) tr.fatLightmapStep ) ) + ( ( 1.0 / ( ( float ) tr.fatLightmapStep ) ) * ( float ) y );
	}

	return input;
}

/*
=================
RE_SetWorldVisData

This is called by the clipmodel subsystem so we can share the 1.8 megs of
space in big maps...
=================
*/
void RE_SetWorldVisData( const byte *vis )
{
	tr.externalVisData = vis;
}

/*
=================
R_LoadVisibility
=================
*/
static void R_LoadVisibility( lump_t *l )
{
	int  len, i, j, k, m;
	byte *buf;

	ri.Printf( PRINT_DEVELOPER, "...loading visibility\n" );

	len = ( s_worldData.numClusters + 63 ) & ~63;
	s_worldData.novis = (byte*) ri.Hunk_Alloc( len, h_low );
	Com_Memset( s_worldData.novis, 0xff, len );

	len = l->filelen;

	if ( !len )
	{
		return;
	}

	buf = fileBase + l->fileofs;

	s_worldData.numClusters = LittleLong( ( ( int * ) buf ) [ 0 ] );
	s_worldData.clusterBytes = LittleLong( ( ( int * ) buf ) [ 1 ] );

	// CM_Load should have given us the vis data to share, so
	// we don't need to allocate another copy
	if ( tr.externalVisData )
	{
		s_worldData.vis = tr.externalVisData;
	}
	else
	{
		byte *dest;

		dest = (byte*) ri.Hunk_Alloc( len - 8, h_low );
		Com_Memcpy( dest, buf + 8, len - 8 );
		s_worldData.vis = dest;
	}

	// initialize visvis := vis
	len = s_worldData.numClusters * s_worldData.clusterBytes;
	s_worldData.visvis = (byte*) ri.Hunk_Alloc( len, h_low );
	memcpy( s_worldData.visvis, s_worldData.vis, len );

	for ( i = 0; i < s_worldData.numClusters; i++ )
	{
		const byte *src;
		const long *src2;
		byte *dest;

		src  = s_worldData.vis + i * s_worldData.clusterBytes;
		dest = s_worldData.visvis + i * s_worldData.clusterBytes;

		// for each byte in the current cluster's vis data
		for ( j = 0; j < s_worldData.clusterBytes; j++ )
		{
			byte bitbyte = src[ j ];

			if ( !bitbyte )
			{
				continue;
			}

			for ( k = 0; k < 8; k++ )
			{
				int index;

				// check if this cluster ( k = ( cluster & 7 ) ) is visible from the current cluster
				if ( ! ( bitbyte & ( 1 << k ) ) )
				{
					continue;
				}

				// retrieve vis data for the cluster
				index = ( ( j << 3 ) | k );
				src2 = ( long * ) ( s_worldData.vis + index * s_worldData.clusterBytes );
				
				// OR this vis data with the current cluster's
				for ( m = 0; m < ( s_worldData.clusterBytes / sizeof( long ) ); m++ )
				{
					( ( long * ) dest )[ m ] |= src2[ m ];
				}
			}
		}
	}
}

//===============================================================================

/*
===============
ShaderForShaderNum
===============
*/
static shader_t *ShaderForShaderNum( int shaderNum )
{
	shader_t  *shader;
	dshader_t *dsh;

	shaderNum = LittleLong( shaderNum ) + 0;  // silence the warning

	if ( shaderNum < 0 || shaderNum >= s_worldData.numShaders )
	{
		ri.Error( ERR_DROP, "ShaderForShaderNum: bad num %i", shaderNum );
	}

	dsh = &s_worldData.shaders[ shaderNum ];

	shader = R_FindShader( dsh->shader, SHADER_3D_STATIC, RSF_DEFAULT );

	// if the shader had errors, just use default shader
	if ( shader->defaultShader )
	{
		return tr.defaultShader;
	}

	return shader;
}

/*
SphereFromBounds() - ydnar
creates a bounding sphere from a bounding box
*/

static void SphereFromBounds( vec3_t mins, vec3_t maxs, vec3_t origin, float *radius )
{
	vec3_t temp;

	VectorAdd( mins, maxs, origin );
	VectorScale( origin, 0.5, origin );
	VectorSubtract( maxs, origin, temp );
	*radius = VectorLength( temp );
}

/*
FinishGenericSurface() - ydnar
handles final surface classification
*/

static void FinishGenericSurface( dsurface_t *ds, srfGeneric_t *gen, vec3_t pt )
{
	// set bounding sphere
	SphereFromBounds( gen->bounds[ 0 ], gen->bounds[ 1 ], gen->origin, &gen->radius );

	if ( gen->surfaceType == SF_FACE )
	{
		srfSurfaceFace_t *srf = ( srfSurfaceFace_t * )gen;
		// take the plane normal from the lightmap vector and classify it
		srf->plane.normal[ 0 ] = LittleFloat( ds->lightmapVecs[ 2 ][ 0 ] );
		srf->plane.normal[ 1 ] = LittleFloat( ds->lightmapVecs[ 2 ][ 1 ] );
		srf->plane.normal[ 2 ] = LittleFloat( ds->lightmapVecs[ 2 ][ 2 ] );
		srf->plane.dist = DotProduct( pt, srf->plane.normal );
		SetPlaneSignbits( &srf->plane );
		srf->plane.type = PlaneTypeForNormal( srf->plane.normal );
	}
}

/*
===============
ParseFace
===============
*/
static void ParseFace( dsurface_t *ds, drawVert_t *verts, bspSurface_t *surf, int *indexes )
{
	int              i, j;
	srfSurfaceFace_t *cv;
	srfTriangle_t    *tri;
	int              numVerts, numTriangles;
	int              realLightmapNum;

	// get lightmap
	realLightmapNum = LittleLong( ds->lightmapNum );

	if ( r_vertexLighting->integer || !r_precomputedLighting->integer )
	{
		surf->lightmapNum = -1;
	}
	else
	{
		surf->lightmapNum = tr.fatLightmapSize ? 0 : realLightmapNum;
	}

	if ( tr.worldDeluxeMapping && surf->lightmapNum >= 2 )
	{
		surf->lightmapNum /= 2;
	}

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum );

	if ( r_singleShader->integer && !surf->shader->isSky )
	{
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong( ds->numVerts );

	numTriangles = LittleLong( ds->numIndexes ) / 3;

	cv = (srfSurfaceFace_t*) ri.Hunk_Alloc( sizeof( *cv ), h_low );
	cv->surfaceType = SF_FACE;

	cv->numTriangles = numTriangles;
	cv->triangles = (srfTriangle_t*) ri.Hunk_Alloc( numTriangles * sizeof( cv->triangles[ 0 ] ), h_low );

	cv->numVerts = numVerts;
	cv->verts = (srfVert_t*) ri.Hunk_Alloc( numVerts * sizeof( cv->verts[ 0 ] ), h_low );

	surf->data = ( surfaceType_t * ) cv;

	// copy vertexes
	ClearBounds( cv->bounds[ 0 ], cv->bounds[ 1 ] );
	verts += LittleLong( ds->firstVert );

	for ( i = 0; i < numVerts; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			cv->verts[ i ].xyz[ j ] = LittleFloat( verts[ i ].xyz[ j ] );
			cv->verts[ i ].normal[ j ] = LittleFloat( verts[ i ].normal[ j ] );
		}

		AddPointToBounds( cv->verts[ i ].xyz, cv->bounds[ 0 ], cv->bounds[ 1 ] );

		for ( j = 0; j < 2; j++ )
		{
			cv->verts[ i ].st[ j ] = LittleFloat( verts[ i ].st[ j ] );
			cv->verts[ i ].lightmap[ j ] = LittleFloat( verts[ i ].lightmap[ j ] );
		}

		cv->verts[ i ].lightmap[ 0 ] = FatPackU( LittleFloat( verts[ i ].lightmap[ 0 ] ), realLightmapNum );
		cv->verts[ i ].lightmap[ 1 ] = FatPackV( LittleFloat( verts[ i ].lightmap[ 1 ] ), realLightmapNum );

		for ( j = 0; j < 4; j++ )
		{
			cv->verts[ i ].lightColor[ j ] = verts[ i ].color[ j ];
		}

		R_ColorShiftLightingBytes( cv->verts[ i ].lightColor, cv->verts[ i ].lightColor );
	}

	// copy triangles
	indexes += LittleLong( ds->firstIndex );

	for ( i = 0, tri = cv->triangles; i < numTriangles; i++, tri++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			tri->indexes[ j ] = LittleLong( indexes[ i * 3 + j ] );

			if ( tri->indexes[ j ] < 0 || tri->indexes[ j ] >= numVerts )
			{
				ri.Error( ERR_DROP, "Bad index in face surface" );
			}
		}
	}

	// take the plane information from the lightmap vector
	for ( i = 0; i < 3; i++ )
	{
		cv->plane.normal[ i ] = LittleFloat( ds->lightmapVecs[ 2 ][ i ] );
	}

	cv->plane.dist = DotProduct( cv->verts[ 0 ].xyz, cv->plane.normal );
	SetPlaneSignbits( &cv->plane );
	cv->plane.type = PlaneTypeForNormal( cv->plane.normal );

	surf->data = ( surfaceType_t * ) cv;

	{
		srfVert_t *dv0, *dv1, *dv2;
		vec3_t    tangent, binormal;

		for ( i = 0, tri = cv->triangles; i < numTriangles; i++, tri++ )
		{
			dv0 = &cv->verts[ tri->indexes[ 0 ] ];
			dv1 = &cv->verts[ tri->indexes[ 1 ] ];
			dv2 = &cv->verts[ tri->indexes[ 2 ] ];

			R_CalcTangents( tangent, binormal,
					dv0->xyz, dv1->xyz, dv2->xyz,
					dv0->st, dv1->st, dv2->st );
			R_TBNtoQtangents( tangent, binormal, dv0->normal,
					  dv0->qtangent );
			R_TBNtoQtangents( tangent, binormal, dv1->normal,
					  dv1->qtangent );
			R_TBNtoQtangents( tangent, binormal, dv2->normal,
					  dv2->qtangent );
		}
	}

	// finish surface
	FinishGenericSurface( ds, ( srfGeneric_t * ) cv, cv->verts[ 0 ].xyz );
}

/*
===============
ParseMesh
===============
*/
static void ParseMesh( dsurface_t *ds, drawVert_t *verts, bspSurface_t *surf )
{
	srfGridMesh_t        *grid;
	int                  i, j;
	int                  width, height, numPoints;
	static srfVert_t     points[ MAX_PATCH_SIZE * MAX_PATCH_SIZE ];
	vec3_t               bounds[ 2 ];
	vec3_t               tmpVec;
	static surfaceType_t skipData = SF_SKIP;
	int                  realLightmapNum;

	// get lightmap
	realLightmapNum = LittleLong( ds->lightmapNum );

	if ( r_vertexLighting->integer || !r_precomputedLighting->integer )
	{
		surf->lightmapNum = -1;
	}
	else
	{
		surf->lightmapNum = tr.fatLightmapSize ? 0 : realLightmapNum;
	}

	if ( tr.worldDeluxeMapping && surf->lightmapNum >= 2 )
	{
		surf->lightmapNum /= 2;
	}

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum );

	if ( r_singleShader->integer && !surf->shader->isSky )
	{
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW )
	{
		surf->data = &skipData;
		return;
	}

	width = LittleLong( ds->patchWidth );
	height = LittleLong( ds->patchHeight );

	if ( width < 0 || width > MAX_PATCH_SIZE || height < 0 || height > MAX_PATCH_SIZE )
	{
		ri.Error( ERR_DROP, "ParseMesh: bad size" );
	}

	verts += LittleLong( ds->firstVert );
	numPoints = width * height;

	for ( i = 0; i < numPoints; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			points[ i ].xyz[ j ] = LittleFloat( verts[ i ].xyz[ j ] );
			points[ i ].normal[ j ] = LittleFloat( verts[ i ].normal[ j ] );
		}

		for ( j = 0; j < 2; j++ )
		{
			points[ i ].st[ j ] = LittleFloat( verts[ i ].st[ j ] );
			points[ i ].lightmap[ j ] = LittleFloat( verts[ i ].lightmap[ j ] );
		}

		points[ i ].lightmap[ 0 ] = FatPackU( LittleFloat( verts[ i ].lightmap[ 0 ] ), realLightmapNum );
		points[ i ].lightmap[ 1 ] = FatPackV( LittleFloat( verts[ i ].lightmap[ 1 ] ), realLightmapNum );

		for ( j = 0; j < 4; j++ )
		{
			points[ i ].lightColor[ j ] = verts[ i ].color[ j ];
		}

		R_ColorShiftLightingBytes( points[ i ].lightColor, points[ i ].lightColor );
	}

	// pre-tesselate
	grid = R_SubdividePatchToGrid( width, height, points );
	surf->data = ( surfaceType_t * ) grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for ( i = 0; i < 3; i++ )
	{
		bounds[ 0 ][ i ] = LittleFloat( ds->lightmapVecs[ 0 ][ i ] );
		bounds[ 1 ][ i ] = LittleFloat( ds->lightmapVecs[ 1 ][ i ] );
	}

	VectorAdd( bounds[ 0 ], bounds[ 1 ], bounds[ 1 ] );
	VectorScale( bounds[ 1 ], 0.5f, grid->lodOrigin );
	VectorSubtract( bounds[ 0 ], grid->lodOrigin, tmpVec );
	grid->lodRadius = VectorLength( tmpVec );

	// finish surface
	FinishGenericSurface( ds, ( srfGeneric_t * ) grid, grid->verts[ 0 ].xyz );
}

/*
===============
ParseTriSurf
===============
*/
static void ParseTriSurf( dsurface_t *ds, drawVert_t *verts, bspSurface_t *surf, int *indexes )
{
	srfTriangles_t       *cv;
	srfTriangle_t        *tri;
	int                  i, j;
	int                  numVerts, numTriangles;
	static surfaceType_t skipData = SF_SKIP;

	// get lightmap
	surf->lightmapNum = -1; // FIXME LittleLong(ds->lightmapNum);

	if ( tr.worldDeluxeMapping && surf->lightmapNum >= 2 )
	{
		surf->lightmapNum /= 2;
	}

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum );

	if ( r_singleShader->integer && !surf->shader->isSky )
	{
		surf->shader = tr.defaultShader;
	}

	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW )
	{
		surf->data = &skipData;
		return;
	}

	numVerts = LittleLong( ds->numVerts );
	numTriangles = LittleLong( ds->numIndexes ) / 3;

	cv = (srfTriangles_t*) ri.Hunk_Alloc( sizeof( *cv ), h_low );
	cv->surfaceType = SF_TRIANGLES;

	cv->numTriangles = numTriangles;
	cv->triangles = (srfTriangle_t*) ri.Hunk_Alloc( numTriangles * sizeof( cv->triangles[ 0 ] ), h_low );

	cv->numVerts = numVerts;
	cv->verts = (srfVert_t*) ri.Hunk_Alloc( numVerts * sizeof( cv->verts[ 0 ] ), h_low );

	surf->data = ( surfaceType_t * ) cv;

	// copy vertexes
	verts += LittleLong( ds->firstVert );

	for ( i = 0; i < numVerts; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			cv->verts[ i ].xyz[ j ] = LittleFloat( verts[ i ].xyz[ j ] );
			cv->verts[ i ].normal[ j ] = LittleFloat( verts[ i ].normal[ j ] );
		}

		for ( j = 0; j < 2; j++ )
		{
			cv->verts[ i ].st[ j ] = LittleFloat( verts[ i ].st[ j ] );
			cv->verts[ i ].lightmap[ j ] = LittleFloat( verts[ i ].lightmap[ j ] );
		}

		for ( j = 0; j < 4; j++ )
		{
			cv->verts[ i ].lightColor[ j ] = verts[ i ].color[ j ];
		}

		R_ColorShiftLightingBytes( cv->verts[ i ].lightColor, cv->verts[ i ].lightColor );
	}

	// copy triangles
	indexes += LittleLong( ds->firstIndex );

	for ( i = 0, tri = cv->triangles; i < numTriangles; i++, tri++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			tri->indexes[ j ] = LittleLong( indexes[ i * 3 + j ] );

			if ( tri->indexes[ j ] < 0 || tri->indexes[ j ] >= numVerts )
			{
				ri.Error( ERR_DROP, "Bad index in face surface" );
			}
		}
	}

	// calc bounding box
	// HACK: don't loop only through the vertices because they can contain bad data with .lwo models ...
	ClearBounds( cv->bounds[ 0 ], cv->bounds[ 1 ] );

	for ( i = 0, tri = cv->triangles; i < numTriangles; i++, tri++ )
	{
		AddPointToBounds( cv->verts[ tri->indexes[ 0 ] ].xyz, cv->bounds[ 0 ], cv->bounds[ 1 ] );
		AddPointToBounds( cv->verts[ tri->indexes[ 1 ] ].xyz, cv->bounds[ 0 ], cv->bounds[ 1 ] );
		AddPointToBounds( cv->verts[ tri->indexes[ 2 ] ].xyz, cv->bounds[ 0 ], cv->bounds[ 1 ] );
	}

	// Tr3B - calc tangent spaces
	{
		srfVert_t *dv0, *dv1, *dv2;
		vec3_t    tangent, binormal;

		for ( i = 0, tri = cv->triangles; i < numTriangles; i++, tri++ )
		{
			dv0 = &cv->verts[ tri->indexes[ 0 ] ];
			dv1 = &cv->verts[ tri->indexes[ 1 ] ];
			dv2 = &cv->verts[ tri->indexes[ 2 ] ];

			R_CalcTangents( tangent, binormal,
					dv0->xyz, dv1->xyz, dv2->xyz,
					dv0->st, dv1->st, dv2->st );
			R_TBNtoQtangents( tangent, binormal, dv0->normal,
					  dv0->qtangent );
			R_TBNtoQtangents( tangent, binormal, dv1->normal,
					  dv1->qtangent );
			R_TBNtoQtangents( tangent, binormal, dv2->normal,
					  dv2->qtangent );
		}
	}

	// finish surface
	FinishGenericSurface( ds, ( srfGeneric_t * ) cv, cv->verts[ 0 ].xyz );
}

/*
===============
ParseFlare
===============
*/
static void ParseFlare( dsurface_t *ds, drawVert_t *verts, bspSurface_t *surf, int *indexes )
{
	srfFlare_t *flare;
	int        i;

	// set lightmap
	surf->lightmapNum = -1;

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum );

	if ( r_singleShader->integer && !surf->shader->isSky )
	{
		surf->shader = tr.defaultShader;
	}

	flare = (srfFlare_t*) ri.Hunk_Alloc( sizeof( *flare ), h_low );
	flare->surfaceType = SF_FLARE;

	surf->data = ( surfaceType_t * ) flare;

	for ( i = 0; i < 3; i++ )
	{
		flare->origin[ i ] = LittleFloat( ds->lightmapOrigin[ i ] );
		flare->color[ i ] = LittleFloat( ds->lightmapVecs[ 0 ][ i ] );
		flare->normal[ i ] = LittleFloat( ds->lightmapVecs[ 2 ][ i ] );
	}
}

/*
=================
R_MergedWidthPoints

returns true if there are grid points merged on a width edge
=================
*/
int R_MergedWidthPoints( srfGridMesh_t *grid, int offset )
{
	int i, j;

	for ( i = 1; i < grid->width - 1; i++ )
	{
		for ( j = i + 1; j < grid->width - 1; j++ )
		{
			if ( fabs( grid->verts[ i + offset ].xyz[ 0 ] - grid->verts[ j + offset ].xyz[ 0 ] ) > .1 )
			{
				continue;
			}

			if ( fabs( grid->verts[ i + offset ].xyz[ 1 ] - grid->verts[ j + offset ].xyz[ 1 ] ) > .1 )
			{
				continue;
			}

			if ( fabs( grid->verts[ i + offset ].xyz[ 2 ] - grid->verts[ j + offset ].xyz[ 2 ] ) > .1 )
			{
				continue;
			}

			return qtrue;
		}
	}

	return qfalse;
}

/*
=================
R_MergedHeightPoints

returns true if there are grid points merged on a height edge
=================
*/
int R_MergedHeightPoints( srfGridMesh_t *grid, int offset )
{
	int i, j;

	for ( i = 1; i < grid->height - 1; i++ )
	{
		for ( j = i + 1; j < grid->height - 1; j++ )
		{
			if ( fabs( grid->verts[ grid->width * i + offset ].xyz[ 0 ] - grid->verts[ grid->width * j + offset ].xyz[ 0 ] ) > .1 )
			{
				continue;
			}

			if ( fabs( grid->verts[ grid->width * i + offset ].xyz[ 1 ] - grid->verts[ grid->width * j + offset ].xyz[ 1 ] ) > .1 )
			{
				continue;
			}

			if ( fabs( grid->verts[ grid->width * i + offset ].xyz[ 2 ] - grid->verts[ grid->width * j + offset ].xyz[ 2 ] ) > .1 )
			{
				continue;
			}

			return qtrue;
		}
	}

	return qfalse;
}

/*
=================
R_FixSharedVertexLodError_r

NOTE: never sync LoD through grid edges with merged points!

FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
=================
*/
void R_FixSharedVertexLodError_r( int start, srfGridMesh_t *grid1 )
{
	int           j, k, l, m, n, offset1, offset2, touch;
	srfGridMesh_t *grid2;

	for ( j = start; j < s_worldData.numSurfaces; j++ )
	{
		//
		grid2 = ( srfGridMesh_t * ) s_worldData.surfaces[ j ].data;

		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID )
		{
			continue;
		}

		// if the LOD errors are already fixed for this patch
		if ( grid2->lodFixed == 2 )
		{
			continue;
		}

		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius )
		{
			continue;
		}

		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[ 0 ] != grid2->lodOrigin[ 0 ] )
		{
			continue;
		}

		if ( grid1->lodOrigin[ 1 ] != grid2->lodOrigin[ 1 ] )
		{
			continue;
		}

		if ( grid1->lodOrigin[ 2 ] != grid2->lodOrigin[ 2 ] )
		{
			continue;
		}

		//
		touch = qfalse;

		for ( n = 0; n < 2; n++ )
		{
			//
			if ( n )
			{
				offset1 = ( grid1->height - 1 ) * grid1->width;
			}
			else
			{
				offset1 = 0;
			}

			if ( R_MergedWidthPoints( grid1, offset1 ) )
			{
				continue;
			}

			for ( k = 1; k < grid1->width - 1; k++ )
			{
				for ( m = 0; m < 2; m++ )
				{
					if ( m )
					{
						offset2 = ( grid2->height - 1 ) * grid2->width;
					}
					else
					{
						offset2 = 0;
					}

					if ( R_MergedWidthPoints( grid2, offset2 ) )
					{
						continue;
					}

					for ( l = 1; l < grid2->width - 1; l++ )
					{
						//
						if ( fabs( grid1->verts[ k + offset1 ].xyz[ 0 ] - grid2->verts[ l + offset2 ].xyz[ 0 ] ) > .1 )
						{
							continue;
						}

						if ( fabs( grid1->verts[ k + offset1 ].xyz[ 1 ] - grid2->verts[ l + offset2 ].xyz[ 1 ] ) > .1 )
						{
							continue;
						}

						if ( fabs( grid1->verts[ k + offset1 ].xyz[ 2 ] - grid2->verts[ l + offset2 ].xyz[ 2 ] ) > .1 )
						{
							continue;
						}

						// ok the points are equal and should have the same lod error
						grid2->widthLodError[ l ] = grid1->widthLodError[ k ];
						touch = qtrue;
					}
				}

				for ( m = 0; m < 2; m++ )
				{
					if ( m )
					{
						offset2 = grid2->width - 1;
					}
					else
					{
						offset2 = 0;
					}

					if ( R_MergedHeightPoints( grid2, offset2 ) )
					{
						continue;
					}

					for ( l = 1; l < grid2->height - 1; l++ )
					{
						//
						if ( fabs( grid1->verts[ k + offset1 ].xyz[ 0 ] - grid2->verts[ grid2->width * l + offset2 ].xyz[ 0 ] ) > .1 )
						{
							continue;
						}

						if ( fabs( grid1->verts[ k + offset1 ].xyz[ 1 ] - grid2->verts[ grid2->width * l + offset2 ].xyz[ 1 ] ) > .1 )
						{
							continue;
						}

						if ( fabs( grid1->verts[ k + offset1 ].xyz[ 2 ] - grid2->verts[ grid2->width * l + offset2 ].xyz[ 2 ] ) > .1 )
						{
							continue;
						}

						// ok the points are equal and should have the same lod error
						grid2->heightLodError[ l ] = grid1->widthLodError[ k ];
						touch = qtrue;
					}
				}
			}
		}

		for ( n = 0; n < 2; n++ )
		{
			//
			if ( n )
			{
				offset1 = grid1->width - 1;
			}
			else
			{
				offset1 = 0;
			}

			if ( R_MergedHeightPoints( grid1, offset1 ) )
			{
				continue;
			}

			for ( k = 1; k < grid1->height - 1; k++ )
			{
				for ( m = 0; m < 2; m++ )
				{
					if ( m )
					{
						offset2 = ( grid2->height - 1 ) * grid2->width;
					}
					else
					{
						offset2 = 0;
					}

					if ( R_MergedWidthPoints( grid2, offset2 ) )
					{
						continue;
					}

					for ( l = 1; l < grid2->width - 1; l++ )
					{
						//
						if ( fabs( grid1->verts[ grid1->width * k + offset1 ].xyz[ 0 ] - grid2->verts[ l + offset2 ].xyz[ 0 ] ) > .1 )
						{
							continue;
						}

						if ( fabs( grid1->verts[ grid1->width * k + offset1 ].xyz[ 1 ] - grid2->verts[ l + offset2 ].xyz[ 1 ] ) > .1 )
						{
							continue;
						}

						if ( fabs( grid1->verts[ grid1->width * k + offset1 ].xyz[ 2 ] - grid2->verts[ l + offset2 ].xyz[ 2 ] ) > .1 )
						{
							continue;
						}

						// ok the points are equal and should have the same lod error
						grid2->widthLodError[ l ] = grid1->heightLodError[ k ];
						touch = qtrue;
					}
				}

				for ( m = 0; m < 2; m++ )
				{
					if ( m )
					{
						offset2 = grid2->width - 1;
					}
					else
					{
						offset2 = 0;
					}

					if ( R_MergedHeightPoints( grid2, offset2 ) )
					{
						continue;
					}

					for ( l = 1; l < grid2->height - 1; l++ )
					{
						//
						if ( fabs
						     ( grid1->verts[ grid1->width * k + offset1 ].xyz[ 0 ] -
						       grid2->verts[ grid2->width * l + offset2 ].xyz[ 0 ] ) > .1 )
						{
							continue;
						}

						if ( fabs
						     ( grid1->verts[ grid1->width * k + offset1 ].xyz[ 1 ] -
						       grid2->verts[ grid2->width * l + offset2 ].xyz[ 1 ] ) > .1 )
						{
							continue;
						}

						if ( fabs
						     ( grid1->verts[ grid1->width * k + offset1 ].xyz[ 2 ] -
						       grid2->verts[ grid2->width * l + offset2 ].xyz[ 2 ] ) > .1 )
						{
							continue;
						}

						// ok the points are equal and should have the same lod error
						grid2->heightLodError[ l ] = grid1->heightLodError[ k ];
						touch = qtrue;
					}
				}
			}
		}

		if ( touch )
		{
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r( start, grid2 );
			//NOTE: this would be correct but makes things really slow
		}
	}
}

/*
=================
R_FixSharedVertexLodError

This function assumes that all patches in one group are nicely stitched together for the highest LoD.
If this is not the case this function will still do its job but won't fix the highest LoD cracks.
=================
*/
void R_FixSharedVertexLodError( void )
{
	int           i;
	srfGridMesh_t *grid1;

	for ( i = 0; i < s_worldData.numSurfaces; i++ )
	{
		//
		grid1 = ( srfGridMesh_t * ) s_worldData.surfaces[ i ].data;

		// if this surface is not a grid
		if ( grid1->surfaceType != SF_GRID )
		{
			continue;
		}

		//
		if ( grid1->lodFixed )
		{
			continue;
		}

		//
		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r( i + 1, grid1 );
	}
}

/*
===============
R_StitchPatches
===============
*/
int R_StitchPatches( int grid1num, int grid2num )
{
	float         *v1, *v2;
	srfGridMesh_t *grid1, *grid2;
	int           k, l, m, n, offset1, offset2, row, column;

	grid1 = ( srfGridMesh_t * ) s_worldData.surfaces[ grid1num ].data;
	grid2 = ( srfGridMesh_t * ) s_worldData.surfaces[ grid2num ].data;

	for ( n = 0; n < 2; n++ )
	{
		//
		if ( n )
		{
			offset1 = ( grid1->height - 1 ) * grid1->width;
		}
		else
		{
			offset1 = 0;
		}

		if ( R_MergedWidthPoints( grid1, offset1 ) )
		{
			continue;
		}

		for ( k = 0; k < grid1->width - 2; k += 2 )
		{
			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->width >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = ( grid2->height - 1 ) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->width - 1; l++ )
				{
					//
					v1 = grid1->verts[ k + offset1 ].xyz;
					v2 = grid2->verts[ l + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					v1 = grid1->verts[ k + 2 + offset1 ].xyz;
					v2 = grid2->verts[ l + 1 + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					//
					v1 = grid2->verts[ l + offset2 ].xyz;
					v2 = grid2->verts[ l + 1 + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) < .01 && fabs( v1[ 1 ] - v2[ 1 ] ) < .01 && fabs( v1[ 2 ] - v2[ 2 ] ) < .01 )
					{
						continue;
					}

					//
					// insert column into grid2 right after after column l
					if ( m )
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}

					grid2 = R_GridInsertColumn( grid2, l + 1, row, grid1->verts[ k + 1 + offset1 ].xyz, grid1->widthLodError[ k + 1 ] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return qtrue;
				}
			}

			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->height >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->height - 1; l++ )
				{
					//
					v1 = grid1->verts[ k + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * l + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					v1 = grid1->verts[ k + 2 + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					//
					v1 = grid2->verts[ grid2->width * l + offset2 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) < .01 && fabs( v1[ 1 ] - v2[ 1 ] ) < .01 && fabs( v1[ 2 ] - v2[ 2 ] ) < .01 )
					{
						continue;
					}

					//
					// insert row into grid2 right after after row l
					if ( m )
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}

					grid2 = R_GridInsertRow( grid2, l + 1, column, grid1->verts[ k + 1 + offset1 ].xyz, grid1->widthLodError[ k + 1 ] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return qtrue;
				}
			}
		}
	}

	for ( n = 0; n < 2; n++ )
	{
		//
		if ( n )
		{
			offset1 = grid1->width - 1;
		}
		else
		{
			offset1 = 0;
		}

		if ( R_MergedHeightPoints( grid1, offset1 ) )
		{
			continue;
		}

		for ( k = 0; k < grid1->height - 2; k += 2 )
		{
			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->width >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = ( grid2->height - 1 ) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->width - 1; l++ )
				{
					//
					v1 = grid1->verts[ grid1->width * k + offset1 ].xyz;
					v2 = grid2->verts[ l + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					v1 = grid1->verts[ grid1->width * ( k + 2 ) + offset1 ].xyz;
					v2 = grid2->verts[ l + 1 + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					//
					v1 = grid2->verts[ l + offset2 ].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) < .01 && fabs( v1[ 1 ] - v2[ 1 ] ) < .01 && fabs( v1[ 2 ] - v2[ 2 ] ) < .01 )
					{
						continue;
					}

					//
					// insert column into grid2 right after after column l
					if ( m )
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}

					grid2 = R_GridInsertColumn( grid2, l + 1, row,
					                            grid1->verts[ grid1->width * ( k + 1 ) + offset1 ].xyz, grid1->heightLodError[ k + 1 ] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return qtrue;
				}
			}

			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->height >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->height - 1; l++ )
				{
					//
					v1 = grid1->verts[ grid1->width * k + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * l + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					v1 = grid1->verts[ grid1->width * ( k + 2 ) + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					//
					v1 = grid2->verts[ grid2->width * l + offset2 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) < .01 && fabs( v1[ 1 ] - v2[ 1 ] ) < .01 && fabs( v1[ 2 ] - v2[ 2 ] ) < .01 )
					{
						continue;
					}

					//
					// insert row into grid2 right after after row l
					if ( m )
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}

					grid2 = R_GridInsertRow( grid2, l + 1, column,
					                         grid1->verts[ grid1->width * ( k + 1 ) + offset1 ].xyz, grid1->heightLodError[ k + 1 ] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return qtrue;
				}
			}
		}
	}

	for ( n = 0; n < 2; n++ )
	{
		//
		if ( n )
		{
			offset1 = ( grid1->height - 1 ) * grid1->width;
		}
		else
		{
			offset1 = 0;
		}

		if ( R_MergedWidthPoints( grid1, offset1 ) )
		{
			continue;
		}

		for ( k = grid1->width - 1; k > 1; k -= 2 )
		{
			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->width >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = ( grid2->height - 1 ) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->width - 1; l++ )
				{
					//
					v1 = grid1->verts[ k + offset1 ].xyz;
					v2 = grid2->verts[ l + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					v1 = grid1->verts[ k - 2 + offset1 ].xyz;
					v2 = grid2->verts[ l + 1 + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					//
					v1 = grid2->verts[ l + offset2 ].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) < .01 && fabs( v1[ 1 ] - v2[ 1 ] ) < .01 && fabs( v1[ 2 ] - v2[ 2 ] ) < .01 )
					{
						continue;
					}

					//
					// insert column into grid2 right after after column l
					if ( m )
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}

					grid2 = R_GridInsertColumn( grid2, l + 1, row, grid1->verts[ k - 1 + offset1 ].xyz, grid1->widthLodError[ k - 1 ] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return qtrue;
				}
			}

			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->height >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->height - 1; l++ )
				{
					//
					v1 = grid1->verts[ k + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * l + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					v1 = grid1->verts[ k - 2 + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					//
					v1 = grid2->verts[ grid2->width * l + offset2 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) < .01 && fabs( v1[ 1 ] - v2[ 1 ] ) < .01 && fabs( v1[ 2 ] - v2[ 2 ] ) < .01 )
					{
						continue;
					}

					//
					// insert row into grid2 right after after row l
					if ( m )
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}

					grid2 = R_GridInsertRow( grid2, l + 1, column, grid1->verts[ k - 1 + offset1 ].xyz, grid1->widthLodError[ k - 1 ] );

					if ( !grid2 )
					{
						break;
					}

					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return qtrue;
				}
			}
		}
	}

	for ( n = 0; n < 2; n++ )
	{
		//
		if ( n )
		{
			offset1 = grid1->width - 1;
		}
		else
		{
			offset1 = 0;
		}

		if ( R_MergedHeightPoints( grid1, offset1 ) )
		{
			continue;
		}

		for ( k = grid1->height - 1; k > 1; k -= 2 )
		{
			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->width >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = ( grid2->height - 1 ) * grid2->width;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->width - 1; l++ )
				{
					//
					v1 = grid1->verts[ grid1->width * k + offset1 ].xyz;
					v2 = grid2->verts[ l + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					v1 = grid1->verts[ grid1->width * ( k - 2 ) + offset1 ].xyz;
					v2 = grid2->verts[ l + 1 + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					//
					v1 = grid2->verts[ l + offset2 ].xyz;
					v2 = grid2->verts[( l + 1 ) + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) < .01 && fabs( v1[ 1 ] - v2[ 1 ] ) < .01 && fabs( v1[ 2 ] - v2[ 2 ] ) < .01 )
					{
						continue;
					}

					//
					// insert column into grid2 right after after column l
					if ( m )
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}

					grid2 = R_GridInsertColumn( grid2, l + 1, row,
					                            grid1->verts[ grid1->width * ( k - 1 ) + offset1 ].xyz, grid1->heightLodError[ k - 1 ] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return qtrue;
				}
			}

			for ( m = 0; m < 2; m++ )
			{
				if ( grid2->height >= MAX_GRID_SIZE )
				{
					break;
				}

				if ( m )
				{
					offset2 = grid2->width - 1;
				}
				else
				{
					offset2 = 0;
				}

				for ( l = 0; l < grid2->height - 1; l++ )
				{
					//
					v1 = grid1->verts[ grid1->width * k + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * l + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					v1 = grid1->verts[ grid1->width * ( k - 2 ) + offset1 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 1 ] - v2[ 1 ] ) > .1 )
					{
						continue;
					}

					if ( fabs( v1[ 2 ] - v2[ 2 ] ) > .1 )
					{
						continue;
					}

					//
					v1 = grid2->verts[ grid2->width * l + offset2 ].xyz;
					v2 = grid2->verts[ grid2->width * ( l + 1 ) + offset2 ].xyz;

					if ( fabs( v1[ 0 ] - v2[ 0 ] ) < .01 && fabs( v1[ 1 ] - v2[ 1 ] ) < .01 && fabs( v1[ 2 ] - v2[ 2 ] ) < .01 )
					{
						continue;
					}

					//
					// insert row into grid2 right after after row l
					if ( m )
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}

					grid2 = R_GridInsertRow( grid2, l + 1, column,
					                         grid1->verts[ grid1->width * ( k - 1 ) + offset1 ].xyz, grid1->heightLodError[ k - 1 ] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( surfaceType_t * ) grid2;
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

/*
===============
R_TryStitchPatch

This function will try to stitch patches in the same LoD group together for the highest LoD.

Only single missing vertice cracks will be fixed.

Vertices will be joined at the patch side a crack is first found, at the other side
of the patch (on the same row or column) the vertices will not be joined and cracks
might still appear at that side.
===============
*/
int R_TryStitchingPatch( int grid1num )
{
	int           j, numstitches;
	srfGridMesh_t *grid1, *grid2;

	numstitches = 0;
	grid1 = ( srfGridMesh_t * ) s_worldData.surfaces[ grid1num ].data;

	for ( j = 0; j < s_worldData.numSurfaces; j++ )
	{
		//
		grid2 = ( srfGridMesh_t * ) s_worldData.surfaces[ j ].data;

		// if this surface is not a grid
		if ( grid2->surfaceType != SF_GRID )
		{
			continue;
		}

		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius )
		{
			continue;
		}

		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin[ 0 ] != grid2->lodOrigin[ 0 ] )
		{
			continue;
		}

		if ( grid1->lodOrigin[ 1 ] != grid2->lodOrigin[ 1 ] )
		{
			continue;
		}

		if ( grid1->lodOrigin[ 2 ] != grid2->lodOrigin[ 2 ] )
		{
			continue;
		}

		//
		while ( R_StitchPatches( grid1num, j ) )
		{
			numstitches++;
		}
	}

	return numstitches;
}

/*
===============
R_StitchAllPatches
===============
*/
void R_StitchAllPatches( void )
{
	int           i, stitched, numstitches;
	srfGridMesh_t *grid1;

	ri.Printf( PRINT_DEVELOPER, "...stitching LoD cracks\n" );

	numstitches = 0;

	do
	{
		stitched = qfalse;

		for ( i = 0; i < s_worldData.numSurfaces; i++ )
		{
			//
			grid1 = ( srfGridMesh_t * ) s_worldData.surfaces[ i ].data;

			// if this surface is not a grid
			if ( grid1->surfaceType != SF_GRID )
			{
				continue;
			}

			//
			if ( grid1->lodStitched )
			{
				continue;
			}

			//
			grid1->lodStitched = qtrue;
			stitched = qtrue;
			//
			numstitches += R_TryStitchingPatch( i );
		}
	}
	while ( stitched );

	ri.Printf( PRINT_DEVELOPER, "stitched %d LoD cracks\n", numstitches );
}

/*
===============
R_MovePatchSurfacesToHunk
===============
*/
void R_MovePatchSurfacesToHunk( void )
{
	int           i, size;
	srfGridMesh_t *grid, *hunkgrid;

	for ( i = 0; i < s_worldData.numSurfaces; i++ )
	{
		//
		grid = ( srfGridMesh_t * ) s_worldData.surfaces[ i ].data;

		// if this surface is not a grid
		if ( grid->surfaceType != SF_GRID )
		{
			continue;
		}

		//
		size = sizeof( *grid );
		hunkgrid = (srfGridMesh_t*) ri.Hunk_Alloc( size, h_low );
		Com_Memcpy( hunkgrid, grid, size );

		hunkgrid->widthLodError = (float*) ri.Hunk_Alloc( grid->width * 4, h_low );
		Com_Memcpy( hunkgrid->widthLodError, grid->widthLodError, grid->width * 4 );

		hunkgrid->heightLodError = (float*) ri.Hunk_Alloc( grid->height * 4, h_low );
		Com_Memcpy( hunkgrid->heightLodError, grid->heightLodError, grid->height * 4 );

		hunkgrid->numTriangles = grid->numTriangles;
		hunkgrid->triangles = (srfTriangle_t*) ri.Hunk_Alloc( grid->numTriangles * sizeof( srfTriangle_t ), h_low );
		Com_Memcpy( hunkgrid->triangles, grid->triangles, grid->numTriangles * sizeof( srfTriangle_t ) );

		hunkgrid->numVerts = grid->numVerts;
		hunkgrid->verts = (srfVert_t*) ri.Hunk_Alloc( grid->numVerts * sizeof( srfVert_t ), h_low );
		Com_Memcpy( hunkgrid->verts, grid->verts, grid->numVerts * sizeof( srfVert_t ) );

		R_FreeSurfaceGridMesh( grid );

		s_worldData.surfaces[ i ].data = ( surfaceType_t * ) hunkgrid;
	}
}

/*
=================
BSPSurfaceCompare
compare function for qsort()
=================
*/
static int BSPSurfaceCompare( const void *a, const void *b )
{
	bspSurface_t *aa, *bb;

	aa = * ( bspSurface_t ** ) a;
	bb = * ( bspSurface_t ** ) b;

	// shader first
	if ( aa->shader < bb->shader )
	{
		return -1;
	}

	else if ( aa->shader > bb->shader )
	{
		return 1;
	}

	// by lightmap
	if ( aa->lightmapNum < bb->lightmapNum )
	{
		return -1;
	}

	else if ( aa->lightmapNum > bb->lightmapNum )
	{
		return 1;
	}

	return 0;
}

static void CopyVert( const srfVert_t *in, srfVert_t *out )
{
	int j;

	for ( j = 0; j < 3; j++ )
	{
		out->xyz[ j ] = in->xyz[ j ];
		out->qtangent[ j ] = in->qtangent[ j ];
		out->normal[ j ] = in->normal[ j ];
	}

	for ( j = 0; j < 2; j++ )
	{
		out->st[ j ] = in->st[ j ];
		out->lightmap[ j ] = in->lightmap[ j ];
	}

	for ( j = 0; j < 4; j++ )
	{
		out->lightColor[ j ] = in->lightColor[ j ];
	}
}

/*
=================
R_CreateClusters
=================
*/
static void R_CreateClusters( void )
{
	int          i, j;
	bspSurface_t *surface;
	bspNode_t    *node;

	// reset surfaces' viewCount
	for ( i = 0, surface = s_worldData.surfaces; i < s_worldData.numSurfaces; i++, surface++ )
	{
		surface->viewCount = -1;
	}

	for ( j = 0, node = s_worldData.nodes; j < s_worldData.numnodes; j++, node++ )
	{
		node->visCounts[ 0 ] = -1;
	}
}

/*
SmoothNormals()
smooths together coincident vertex normals across the bsp
*/
static int LeafSurfaceCompare( const void *a, const void *b )
{
	bspSurface_t *aa, *bb;

	aa = * ( bspSurface_t ** ) a;
	bb = * ( bspSurface_t ** ) b;

	// shader first
	if ( aa->shader < bb->shader )
	{
		return -1;
	}

	else if ( aa->shader > bb->shader )
	{
		return 1;
	}

	// by lightmap
	if ( aa->lightmapNum < bb->lightmapNum )
	{
		return -1;
	}

	else if ( aa->lightmapNum > bb->lightmapNum )
	{
		return 1;
	}

	if ( aa->fogIndex < bb->fogIndex )
	{
		return -1;
	}
	else if ( aa->fogIndex > bb->fogIndex )
	{
		return 1;
	}

	if ( aa->viewCount < bb->viewCount )
	{
		return -1;
	}
	else if ( aa->viewCount > bb->viewCount )
	{
		return 1;
	}

	return 0;
}

/*
===============
R_CreateWorldVBO
===============
*/
static void R_CreateWorldVBO( void )
{
	int       i, j, k;

	int       numVerts;
	srfVert_t *verts;

	int           numTriangles;
	srfTriangle_t *triangles;

	int            numSurfaces;
	bspSurface_t  *surface;
	bspSurface_t  **surfaces;
	bspSurface_t  *mergedSurf;
	int           startTime, endTime;

	startTime = ri.Milliseconds();

	numVerts = 0;
	numTriangles = 0;
	numSurfaces = 0;

	for ( k = 0, surface = &s_worldData.surfaces[ 0 ]; k < s_worldData.numSurfaces; k++, surface++ )
	{
		if ( surface->shader->isSky || surface->shader->isPortal || ShaderRequiresCPUDeforms( surface->shader ) )
		{
			continue;
		}

		if ( *surface->data == SF_FACE )
		{
			srfSurfaceFace_t *face = ( srfSurfaceFace_t * ) surface->data;

			numVerts += face->numVerts;
			numTriangles += face->numTriangles;
		}
		else if ( *surface->data == SF_GRID )
		{
			srfGridMesh_t *grid = ( srfGridMesh_t * ) surface->data;

			numVerts += grid->numVerts;
			numTriangles += grid->numTriangles;
		}
		else if ( *surface->data == SF_TRIANGLES )
		{
			srfTriangles_t *tri = ( srfTriangles_t * ) surface->data;

			numVerts += tri->numVerts;
			numTriangles += tri->numTriangles;
		}
		else
		{
			continue;
		}

		numSurfaces++;
	}

	if ( !numVerts || !numTriangles || !numSurfaces )
	{
		return;
	}

	// reset surface view counts
	for ( i = 0, surface = s_worldData.surfaces; i < s_worldData.numSurfaces; i++, surface++ )
	{
		surface->viewCount = -1;
	}

	if ( r_mergeLeafSurfaces->integer )
	{
		// mark matching surfaces
		for ( i = 0; i < s_worldData.numnodes - s_worldData.numDecisionNodes; i++ )
		{
			bspNode_t *leaf = s_worldData.nodes + s_worldData.numDecisionNodes + i;

			for ( j = 0; j < leaf->numMarkSurfaces; j++ )
			{
				bspSurface_t *surf1;
				shader_t *shader1;
				int fogIndex1;
				int lightMapNum1;
				qboolean merged = qfalse;
				surf1 = leaf->markSurfaces[ j ];

				if ( surf1->viewCount != -1 )
				{
					continue;
				}

				if ( *surf1->data != SF_GRID && *surf1->data != SF_TRIANGLES && *surf1->data != SF_FACE )
				{
					continue;
				}

				shader1 = surf1->shader;

				if ( shader1->isSky || shader1->isPortal || ShaderRequiresCPUDeforms( shader1 ) )
				{
					continue;
				}

				fogIndex1 = surf1->fogIndex;
				lightMapNum1 = surf1->lightmapNum;
				surf1->viewCount = surf1 - s_worldData.surfaces;

				for ( k = j + 1; k < leaf->numMarkSurfaces; k++ )
				{
					bspSurface_t *surf2;
					shader_t *shader2;
					int fogIndex2;
					int lightMapNum2;

					surf2 = leaf->markSurfaces[ k ];

					if ( surf2->viewCount != -1 )
					{
						continue;
					}

					if ( *surf2->data != SF_GRID && *surf2->data != SF_TRIANGLES && *surf2->data != SF_FACE )
					{
						continue;
					}

					shader2 = surf2->shader;
					fogIndex2 = surf2->fogIndex;
					lightMapNum2 = surf2->lightmapNum;
					if ( shader1 != shader2 || fogIndex1 != fogIndex2 || lightMapNum1 != lightMapNum2 )
					{
						continue;
					}

					surf2->viewCount = surf1->viewCount;
					merged = qtrue;
				}

				if ( !merged )
				{
					surf1->viewCount = -1;
				}
			}
		}
	}

	surfaces = ( bspSurface_t ** ) ri.Hunk_AllocateTempMemory( sizeof( *surfaces ) * numSurfaces );

	numSurfaces = 0;
	for ( k = 0, surface = &s_worldData.surfaces[ 0 ]; k < s_worldData.numSurfaces; k++, surface++ )
	{
		if ( surface->shader->isSky || surface->shader->isPortal || ShaderRequiresCPUDeforms( surface->shader ) )
		{
			continue;
		}

		if ( *surface->data == SF_FACE || *surface->data == SF_GRID || *surface->data == SF_TRIANGLES )
		{
			surfaces[ numSurfaces++ ] = surface;
		}
	}

	qsort( surfaces, numSurfaces, sizeof( *surfaces ), LeafSurfaceCompare );

	ri.Printf( PRINT_DEVELOPER, "...calculating world VBO ( %i verts %i tris )\n", numVerts, numTriangles );

	// create arrays
	s_worldData.numVerts = numVerts;
	s_worldData.verts = verts = (srfVert_t*) ri.Hunk_Alloc( numVerts * sizeof( srfVert_t ), h_low );

	s_worldData.numTriangles = numTriangles;
	s_worldData.triangles = triangles = (srfTriangle_t*) ri.Hunk_Alloc( numTriangles * sizeof( srfTriangle_t ), h_low );

	// set up triangle and vertex arrays
	numVerts = 0;
	numTriangles = 0;

	for ( k = 0; k < numSurfaces; k++ )
	{
		surface = surfaces[ k ];

		if ( *surface->data == SF_FACE )
		{
			srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface->data;

			srf->firstTriangle = numTriangles;
			srf->firstVert = numVerts;

			if ( srf->numTriangles )
			{
				srfTriangle_t *tri;

				for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
				{
					for ( j = 0; j < 3; j++ )
					{
						triangles[ numTriangles + i ].indexes[ j ] = srf->firstVert + tri->indexes[ j ];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if ( srf->numVerts )
			{
				for ( i = 0; i < srf->numVerts; i++ )
				{
					CopyVert( &srf->verts[ i ], &verts[ srf->firstVert + i ] );
				}

				numVerts += srf->numVerts;
			}
		}
		else if ( *surface->data == SF_GRID )
		{
			srfGridMesh_t *srf = ( srfGridMesh_t * ) surface->data;

			srf->firstTriangle = numTriangles;
			srf->firstVert = numVerts;

			if ( srf->numTriangles )
			{
				srfTriangle_t *tri;

				for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
				{
					for ( j = 0; j < 3; j++ )
					{
						triangles[ numTriangles + i ].indexes[ j ] = srf->firstVert + tri->indexes[ j ];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if ( srf->numVerts )
			{
				for ( i = 0; i < srf->numVerts; i++ )
				{
					CopyVert( &srf->verts[ i ], &verts[ srf->firstVert + i ] );
				}

				numVerts += srf->numVerts;
			}
		}
		else if ( *surface->data == SF_TRIANGLES )
		{
			srfTriangles_t *srf = ( srfTriangles_t * ) surface->data;

			srf->firstTriangle = numTriangles;
			srf->firstVert = numVerts;

			if ( srf->numTriangles )
			{
				srfTriangle_t *tri;

				for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
				{
					for ( j = 0; j < 3; j++ )
					{
						triangles[ numTriangles + i ].indexes[ j ] = srf->firstVert + tri->indexes[ j ];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if ( srf->numVerts )
			{
				for ( i = 0; i < srf->numVerts; i++ )
				{
					CopyVert( &srf->verts[ i ], &verts[ srf->firstVert + i ] );
				}

				numVerts += srf->numVerts;
			}
		}
	}

	// create vbo and ibo
	s_worldData.vbo = R_CreateStaticVBO2( va( "staticWorld_VBO %i", 0 ), numVerts, verts,
	                                ATTR_POSITION | ATTR_TEXCOORD | ATTR_QTANGENT | ATTR_COLOR
	                                 );

	s_worldData.ibo = R_CreateStaticIBO2( va( "staticWorld_IBO %i", 0 ), numTriangles, triangles );

	if ( r_mergeLeafSurfaces->integer )
	{
		// count merged/unmerged surfaces
		int numUnmergedSurfaces = 0;
		int numMergedSurfaces = 0;
		int oldViewCount = -2;

		for ( i = 0; i < numSurfaces; i++ )
		{
			surface = surfaces[ i ];

			if ( surface->viewCount == -1 )
			{
				numUnmergedSurfaces++;
			}
			else if ( surface->viewCount != oldViewCount )
			{
				oldViewCount = surface->viewCount;
				numMergedSurfaces++;
			}
		}

		// Allocate merged surfaces
		s_worldData.mergedSurfaces = ( bspSurface_t * ) ri.Hunk_Alloc( sizeof( *s_worldData.mergedSurfaces ) * numMergedSurfaces, h_low );

		// actually merge surfaces
		mergedSurf = s_worldData.mergedSurfaces;
		oldViewCount = -2;
		for ( i = 0; i < numSurfaces; i++ )
		{
			vec3_t bounds[ 2 ];
			int numVerts = 0;
			int numIndexes = 0;
			int firstIndex = numTriangles * 3;
			srfVBOMesh_t *vboSurf;
			bspSurface_t *surf1 = surfaces[ i ];

			// skip unmergable surfaces
			if ( surf1->viewCount == -1 )
			{
				continue;
			}

			// skip surfaces that have already been merged
			if ( surf1->viewCount == oldViewCount )
			{
				continue;
			}

			oldViewCount = surf1->viewCount;

			if ( *surf1->data == SF_FACE )
			{
				srfSurfaceFace_t *face = ( srfSurfaceFace_t * ) surf1->data;
				firstIndex = face->firstTriangle * 3;
			}
			else if ( *surf1->data == SF_TRIANGLES )
			{
				srfTriangles_t *tris = ( srfTriangles_t * ) surf1->data;
				firstIndex = tris->firstTriangle * 3;
			}
			else if ( *surf1->data == SF_GRID )
			{
				srfGridMesh_t *grid = ( srfGridMesh_t * ) surf1->data;
				firstIndex = grid->firstTriangle * 3;
			}

			// count verts and indexes and add bounds for the merged surface
			ClearBounds( bounds[ 0 ], bounds[ 1 ] );
			for ( j = i; j < numSurfaces; j++ )
			{
				bspSurface_t *surf2 = surfaces[ j ];

				// stop merging when we hit a surface that can't be merged
				if ( surf2->viewCount != surf1->viewCount )
				{
					break;
				}

				if ( *surf2->data == SF_FACE )
				{
					srfSurfaceFace_t *face = ( srfSurfaceFace_t * ) surf2->data;
					numIndexes += face->numTriangles * 3;
					numVerts += face->numVerts;
					BoundsAdd( bounds[ 0 ], bounds[ 1 ], face->bounds[ 0 ], face->bounds[ 1 ] );
				}
				else if ( *surf2->data == SF_TRIANGLES )
				{
					srfTriangles_t *tris = ( srfTriangles_t * ) surf2->data;
					numIndexes += tris->numTriangles * 3;
					numVerts += tris->numVerts;
					BoundsAdd( bounds[ 0 ], bounds[ 1 ], tris->bounds[ 0 ], tris->bounds[ 1 ] );
				}
				else if ( *surf2->data == SF_GRID )
				{
					srfGridMesh_t *grid = ( srfGridMesh_t * ) surf2->data;
					numIndexes += grid->numTriangles * 3;
					numVerts += grid->numVerts;
					BoundsAdd( bounds[ 0 ], bounds[ 1 ], grid->bounds[ 0 ], grid->bounds[ 1 ] );
				}
			}

			if ( !numIndexes || !numVerts )
			{
				continue;
			}

			vboSurf = ( srfVBOMesh_t * ) ri.Hunk_Alloc( sizeof( *vboSurf ), h_low );
			memset( vboSurf, 0, sizeof( *vboSurf ) );
			vboSurf->surfaceType = SF_VBO_MESH;

			vboSurf->numIndexes = numIndexes;
			vboSurf->numVerts = numVerts;
			vboSurf->firstIndex = firstIndex;

			vboSurf->shader = surf1->shader;
			vboSurf->fogIndex = surf1->fogIndex;
			vboSurf->lightmapNum = surf1->lightmapNum;
			vboSurf->vbo = s_worldData.vbo;
			vboSurf->ibo = s_worldData.ibo;

			VectorCopy( bounds[ 0 ], vboSurf->bounds[ 0 ] );
			VectorCopy( bounds[ 1 ], vboSurf->bounds[ 1 ] );
			SphereFromBounds( vboSurf->bounds[ 0 ], vboSurf->bounds[ 1 ], vboSurf->origin, &vboSurf->radius );
	
			mergedSurf->data = ( surfaceType_t * ) vboSurf;
			mergedSurf->fogIndex = surf1->fogIndex;
			mergedSurf->shader = surf1->shader;
			mergedSurf->lightmapNum = surf1->lightmapNum;
			mergedSurf->viewCount = -1;

			// redirect view surfaces to this surf
			for ( k = 0; k < s_worldData.numMarkSurfaces; k++ )
			{
				bspSurface_t **view = s_worldData.viewSurfaces + k;

				if ( ( *view )->viewCount == surf1->viewCount )
				{
					*view = mergedSurf;
				}
			}

			mergedSurf++;
		}

		ri.Printf( PRINT_DEVELOPER, "Processed %d surfaces into %d merged, %d unmerged\n", numSurfaces, numMergedSurfaces, numUnmergedSurfaces );
	}

	endTime = ri.Milliseconds();
	ri.Printf( PRINT_DEVELOPER, "world VBO calculation time = %5.2f seconds\n", ( endTime - startTime ) / 1000.0 );

	// point triangle surfaces to world VBO
	for ( k = 0; k < numSurfaces; k++ )
	{
		surface = surfaces[ k ];

		if ( *surface->data == SF_FACE )
		{
			srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface->data;
			srf->vbo = s_worldData.vbo;
			srf->ibo = s_worldData.ibo;
		}
		else if ( *surface->data == SF_GRID )
		{
			srfGridMesh_t *srf = ( srfGridMesh_t * ) surface->data;
			srf->vbo = s_worldData.vbo;
			srf->ibo = s_worldData.ibo;
		}
		else if ( *surface->data == SF_TRIANGLES )
		{
			srfTriangles_t *srf = ( srfTriangles_t * ) surface->data;
			srf->vbo = s_worldData.vbo;
			srf->ibo = s_worldData.ibo;
		}

		surface->viewCount = -1;
	}

	ri.Hunk_FreeTempMemory( surfaces );
}

/*
===============
R_LoadSurfaces
===============
*/
static void R_LoadSurfaces( lump_t *surfs, lump_t *verts, lump_t *indexLump )
{
	dsurface_t   *in;
	bspSurface_t *out;
	drawVert_t   *dv;
	int          *indexes;
	int          count;
	int          numFaces, numMeshes, numTriSurfs, numFlares, numFoliages;
	int          i;

	ri.Printf( PRINT_DEVELOPER, "...loading surfaces\n" );

	numFaces = 0;
	numMeshes = 0;
	numTriSurfs = 0;
	numFlares = 0;
	numFoliages = 0;

	in = ( dsurface_t * )( fileBase + surfs->fileofs );

	if ( surfs->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = surfs->filelen / sizeof( *in );

	dv = ( drawVert_t * )( fileBase + verts->fileofs );

	if ( verts->filelen % sizeof( *dv ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	indexes = ( int * )( fileBase + indexLump->fileofs );

	if ( indexLump->filelen % sizeof( *indexes ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	out = (bspSurface_t*) ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	s_worldData.surfaces = out;
	s_worldData.numSurfaces = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		switch ( LittleLong( in->surfaceType ) )
		{
			case MST_PATCH:
				ParseMesh( in, dv, out );
				numMeshes++;
				break;

			case MST_TRIANGLE_SOUP:
				ParseTriSurf( in, dv, out, indexes );
				numTriSurfs++;
				break;

			case MST_PLANAR:
				ParseFace( in, dv, out, indexes );
				numFaces++;
				break;

			case MST_FLARE:
				ParseFlare( in, dv, out, indexes );
				numFlares++;
				break;

			case MST_FOLIAGE:
				// Tr3B: TODO ParseFoliage
				ParseTriSurf( in, dv, out, indexes );
				numFoliages++;
				break;

			default:
				ri.Error( ERR_DROP, "Bad surfaceType" );
		}
	}

	ri.Printf( PRINT_DEVELOPER, "...loaded %d faces, %i meshes, %i trisurfs, %i flares %i foliages\n", numFaces, numMeshes, numTriSurfs,
	           numFlares, numFoliages );

	if ( r_stitchCurves->integer )
	{
		R_StitchAllPatches();
	}

	R_FixSharedVertexLodError();

	if ( r_stitchCurves->integer )
	{
		R_MovePatchSurfacesToHunk();
	}
}

/*
=================
R_LoadSubmodels
=================
*/
static void R_LoadSubmodels( lump_t *l )
{
	dmodel_t   *in;
	bspModel_t *out;
	int        i, j, count;

	ri.Printf( PRINT_DEVELOPER, "...loading submodels\n" );

	in = ( dmodel_t * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );

	s_worldData.numModels = count;
	s_worldData.models = out = (bspModel_t*) ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	for ( i = 0; i < count; i++, in++, out++ )
	{
		model_t *model;

		model = R_AllocModel();

		assert( model != NULL );  // this should never happen

		if ( model == NULL )
		{
			ri.Error( ERR_DROP, "R_LoadSubmodels: R_AllocModel() failed" );
		}

		model->type = MOD_BSP;
		model->bsp = out;
		Com_sprintf( model->name, sizeof( model->name ), "*%d", i );

		for ( j = 0; j < 3; j++ )
		{
			out->bounds[ 0 ][ j ] = LittleFloat( in->mins[ j ] );
			out->bounds[ 1 ][ j ] = LittleFloat( in->maxs[ j ] );
		}

		out->firstSurface = s_worldData.surfaces + LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );

		// ydnar: allocate decal memory
		j = ( i == 0 ? MAX_WORLD_DECALS : MAX_ENTITY_DECALS );
		out->decals = (decal_t*) ri.Hunk_Alloc( j * sizeof( *out->decals ), h_low );
		memset( out->decals, 0, j * sizeof( *out->decals ) );
	}
}

//==================================================================

/*
=================
R_SetParent
=================
*/
static void R_SetParent( bspNode_t *node, bspNode_t *parent )
{
	node->parent = parent;

	if ( node->contents != CONTENTS_NODE )
	{
		// add node surfaces to bounds
		if ( node->numMarkSurfaces > 0 )
		{
			int          c;
			bspSurface_t **mark;
			srfGeneric_t *gen;
			qboolean     mergedSurfBounds;

			// add node surfaces to bounds
			mark = node->markSurfaces;
			c = node->numMarkSurfaces;
			ClearBounds( node->surfMins, node->surfMaxs );
			mergedSurfBounds = qfalse;

			while ( c-- )
			{
				gen = ( srfGeneric_t * )( **mark ).data;

				if ( gen->surfaceType != SF_FACE &&
				     gen->surfaceType != SF_GRID && gen->surfaceType != SF_TRIANGLES )
				{
					continue;
				}

				AddPointToBounds( gen->bounds[ 0 ], node->surfMins, node->surfMaxs );
				AddPointToBounds( gen->bounds[ 1 ], node->surfMins, node->surfMaxs );
				mark++;
				mergedSurfBounds = qtrue;
			}

			if ( !mergedSurfBounds )
			{
				VectorCopy( node->mins, node->surfMins );
				VectorCopy( node->maxs, node->surfMaxs );
			}
		}

		return;
	}

	R_SetParent( node->children[ 0 ], node );
	R_SetParent( node->children[ 1 ], node );

	// ydnar: surface bounds
	AddPointToBounds( node->children[ 0 ]->surfMins, node->surfMins, node->surfMaxs );
	AddPointToBounds( node->children[ 0 ]->surfMins, node->surfMins, node->surfMaxs );
	AddPointToBounds( node->children[ 1 ]->surfMins, node->surfMins, node->surfMaxs );
	AddPointToBounds( node->children[ 1 ]->surfMaxs, node->surfMins, node->surfMaxs );
}

/*
=================
R_LoadNodesAndLeafs
=================
*/
static void R_LoadNodesAndLeafs( lump_t *nodeLump, lump_t *leafLump )
{
	int           i, j, p;
	dnode_t       *in;
	dleaf_t       *inLeaf;
	bspNode_t     *out;
	int           numNodes, numLeafs;
	vboData_t     data;
	srfTriangle_t *triangles = NULL;
	IBO_t         *volumeIBO;
	vec3_t        mins, maxs;

	ri.Printf( PRINT_DEVELOPER, "...loading nodes and leaves\n" );

	memset( &data, 0, sizeof( data ) );

	in = ( dnode_t * ) ( void * )( fileBase + nodeLump->fileofs );

	if ( nodeLump->filelen % sizeof( dnode_t ) || leafLump->filelen % sizeof( dleaf_t ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	numNodes = nodeLump->filelen / sizeof( dnode_t );
	numLeafs = leafLump->filelen / sizeof( dleaf_t );

	out = (bspNode_t*) ri.Hunk_Alloc( ( numNodes + numLeafs ) * sizeof( *out ), h_low );

	s_worldData.nodes = out;
	s_worldData.numnodes = numNodes + numLeafs;
	s_worldData.numDecisionNodes = numNodes;

	// ydnar: skybox optimization
	s_worldData.numSkyNodes = 0;
	s_worldData.skyNodes = (bspNode_t**) ri.Hunk_Alloc( WORLD_MAX_SKY_NODES * sizeof( *s_worldData.skyNodes ), h_low );

	// load nodes
	for ( i = 0; i < numNodes; i++, in++, out++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			out->mins[ j ] = LittleLong( in->mins[ j ] );
			out->maxs[ j ] = LittleLong( in->maxs[ j ] );
		}

		// ydnar: surface bounds
		VectorCopy( out->mins, out->surfMins );
		VectorCopy( out->maxs, out->surfMaxs );

		p = LittleLong( in->planeNum );
		out->plane = s_worldData.planes + p;

		out->contents = CONTENTS_NODE; // differentiate from leafs

		for ( j = 0; j < 2; j++ )
		{
			p = LittleLong( in->children[ j ] );

			if ( p >= 0 )
			{
				out->children[ j ] = s_worldData.nodes + p;
			}
			else
			{
				out->children[ j ] = s_worldData.nodes + numNodes + ( -1 - p );
			}
		}
	}

	// load leafs
	inLeaf = ( dleaf_t * )( fileBase + leafLump->fileofs );

	for ( i = 0; i < numLeafs; i++, inLeaf++, out++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			out->mins[ j ] = LittleLong( inLeaf->mins[ j ] );
			out->maxs[ j ] = LittleLong( inLeaf->maxs[ j ] );
		}

		// ydnar: surface bounds
		ClearBounds( out->surfMins, out->surfMaxs );

		out->cluster = LittleLong( inLeaf->cluster );
		out->area = LittleLong( inLeaf->area );

		if ( out->cluster >= s_worldData.numClusters )
		{
			s_worldData.numClusters = out->cluster + 1;
		}

		out->markSurfaces = s_worldData.markSurfaces + LittleLong( inLeaf->firstLeafSurface );
		out->viewSurfaces = s_worldData.viewSurfaces + LittleLong( inLeaf->firstLeafSurface );
		out->numMarkSurfaces = LittleLong( inLeaf->numLeafSurfaces );
	}

	// chain decendants and compute surface bounds
	R_SetParent( s_worldData.nodes, NULL );

	// calculate occlusion query volumes
	for ( j = 0, out = &s_worldData.nodes[ 0 ]; j < s_worldData.numnodes; j++, out++ )
	{

		Com_Memset( out->lastVisited, -1, sizeof( out->lastVisited ) );
		Com_Memset( out->visible, qfalse, sizeof( out->visible ) );

		InitLink( &out->visChain, out );
		InitLink( &out->occlusionQuery, out );
		InitLink( &out->occlusionQuery2, out );

		glGenQueries( MAX_VIEWS, out->occlusionQueryObjects );

		tess.multiDrawPrimitives = 0;
		tess.numIndexes = 0;
		tess.numVertexes = 0;

		VectorCopy( out->mins, mins );
		VectorCopy( out->maxs, maxs );

		for ( i = 0; i < 3; i++ )
		{
			out->origin[ i ] = ( mins[ i ] + maxs[ i ] ) * 0.5f;
		}

		Tess_AddCube( vec3_origin, mins, maxs, colorWhite );

		if ( j == 0 )
		{
			data.xyz = (vec3_t*) ri.Hunk_AllocateTempMemory(tess.numVertexes * sizeof(*data.xyz));
			triangles = (srfTriangle_t*) ri.Hunk_AllocateTempMemory((tess.numIndexes / 3) * sizeof(srfTriangle_t));
		}

		for ( i = 0; i < tess.numVertexes; i++ )
		{
			VectorCopy( tess.xyz[ i ], data.xyz[ i ] );
		}
		data.numVerts = tess.numVertexes;

		for ( i = 0; i < ( tess.numIndexes / 3 ); i++ )
		{
			triangles[ i ].indexes[ 0 ] = tess.indexes[ i * 3 + 0 ];
			triangles[ i ].indexes[ 1 ] = tess.indexes[ i * 3 + 1 ];
			triangles[ i ].indexes[ 2 ] = tess.indexes[ i * 3 + 2 ];
		}

		out->volumeVBO = R_CreateStaticVBO( va( "staticBspNode_VBO %i", j ), data, VBO_LAYOUT_SEPERATE );

		if ( j == 0 )
		{
			out->volumeIBO = volumeIBO = R_CreateStaticIBO( "staticBspNode_IBO", tess.indexes, tess.numIndexes );
		}
		else
		{
			out->volumeIBO = volumeIBO;
		}

		out->volumeVerts = tess.numVertexes;
		out->volumeIndexes = tess.numIndexes;
	}

//I'm unsure if Hunk_FreeTempMemory can handle NULL values.
	if ( triangles ) { ri.Hunk_FreeTempMemory( triangles ); }

	if ( data.xyz ) { ri.Hunk_FreeTempMemory( data.xyz ); }

	tess.multiDrawPrimitives = 0;
	tess.numIndexes = 0;
	tess.numVertexes = 0;
}

//=============================================================================

/*
=================
R_LoadShaders
=================
*/
static void R_LoadShaders( lump_t *l )
{
	int       i, count;
	dshader_t *in, *out;

	ri.Printf( PRINT_DEVELOPER, "...loading shaders\n" );

	in = ( dshader_t * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );
	out = (dshader_t*) ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	s_worldData.shaders = out;
	s_worldData.numShaders = count;

	Com_Memcpy( out, in, count * sizeof( *out ) );

	for ( i = 0; i < count; i++ )
	{
		ri.Printf( PRINT_DEVELOPER, "shader: '%s'\n", out[ i ].shader );

		out[ i ].surfaceFlags = LittleLong( out[ i ].surfaceFlags );
		out[ i ].contentFlags = LittleLong( out[ i ].contentFlags );
	}
}

/*
=================
R_LoadMarksurfaces
=================
*/
static void R_LoadMarksurfaces( lump_t *l )
{
	int          i, j, count;
	int          *in;
	bspSurface_t **out;

	ri.Printf( PRINT_DEVELOPER, "...loading mark surfaces\n" );

	in = ( int * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );
	out = (bspSurface_t**) ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	s_worldData.markSurfaces = out;
	s_worldData.numMarkSurfaces = count;
	s_worldData.viewSurfaces = ( bspSurface_t ** ) ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	for ( i = 0; i < count; i++ )
	{
		j = LittleLong( in[ i ] );
		out[ i ] = s_worldData.surfaces + j;
		s_worldData.viewSurfaces[ i ] = out[ i ];
	}
}

/*
=================
R_LoadPlanes
=================
*/
static void R_LoadPlanes( lump_t *l )
{
	int      i, j;
	cplane_t *out;
	dplane_t *in;
	int      count;
	int      bits;

	ri.Printf( PRINT_DEVELOPER, "...loading planes\n" );

	in = ( dplane_t * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );
	out = (cplane_t*) ri.Hunk_Alloc( count * 2 * sizeof( *out ), h_low );

	s_worldData.planes = out;
	s_worldData.numplanes = count;

	for ( i = 0; i < count; i++, in++, out++ )
	{
		bits = 0;

		for ( j = 0; j < 3; j++ )
		{
			out->normal[ j ] = LittleFloat( in->normal[ j ] );

			if ( out->normal[ j ] < 0 )
			{
				bits |= 1 << j;
			}
		}

		out->dist = LittleFloat( in->dist );
		out->type = PlaneTypeForNormal( out->normal );
		out->signbits = bits;
	}
}

/*
=================
R_LoadFogs
=================
*/
static void R_LoadFogs( lump_t *l, lump_t *brushesLump, lump_t *sidesLump )
{
	int          i;
	fog_t        *out;
	dfog_t       *fogs;
	dbrush_t     *brushes, *brush;
	dbrushside_t *sides;
	int          count, brushesCount, sidesCount;
	int          sideNum;
	int          planeNum;
	shader_t     *shader;
	float        d;
	int          firstSide = 0;

	ri.Printf( PRINT_DEVELOPER, "...loading fogs\n" );

	fogs = ( dfog_t* )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *fogs ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *fogs );

	// create fog strucutres for them
	s_worldData.numFogs = count + 1;
	s_worldData.fogs = (fog_t*) ri.Hunk_Alloc( s_worldData.numFogs * sizeof( *out ), h_low );
	out = s_worldData.fogs + 1;

	// ydnar: reset global fog
	s_worldData.globalFog = -1;

	if ( !count )
	{
		ri.Printf( PRINT_DEVELOPER, "no fog volumes loaded\n" );
		return;
	}

	brushes = ( dbrush_t * )( fileBase + brushesLump->fileofs );

	if ( brushesLump->filelen % sizeof( *brushes ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	brushesCount = brushesLump->filelen / sizeof( *brushes );

	sides = ( dbrushside_t * )( fileBase + sidesLump->fileofs );

	if ( sidesLump->filelen % sizeof( *sides ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	sidesCount = sidesLump->filelen / sizeof( *sides );

	for ( i = 0; i < count; i++, fogs++ )
	{
		out->originalBrushNumber = LittleLong( fogs->brushNum );

		// ydnar: global fog has a brush number of -1, and no visible side
		if ( out->originalBrushNumber == -1 )
		{
			VectorSet( out->bounds[ 0 ], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD );
			VectorSet( out->bounds[ 1 ], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD );
		}
		else
		{
			if ( ( unsigned ) out->originalBrushNumber >= brushesCount )
			{
				ri.Error( ERR_DROP, "fog brushNumber out of range" );
			}

			brush = brushes + out->originalBrushNumber;

			firstSide = LittleLong( brush->firstSide );

			if ( ( unsigned ) firstSide > sidesCount - 6 )
			{
				ri.Error( ERR_DROP, "fog brush sideNumber out of range" );
			}

			// brushes are always sorted with the axial sides first
			sideNum = firstSide + 0;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 0 ][ 0 ] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 1;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 1 ][ 0 ] = s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 2;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 0 ][ 1 ] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 3;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 1 ][ 1 ] = s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 4;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 0 ][ 2 ] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 5;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 1 ][ 2 ] = s_worldData.planes[ planeNum ].dist;
		}

		// get information from the shader for fog parameters
		shader = R_FindShader( fogs->shader, SHADER_3D_DYNAMIC, RSF_DEFAULT );

		out->fogParms = shader->fogParms;

		out->color[ 0 ] = shader->fogParms.color[ 0 ] * tr.identityLight;
		out->color[ 1 ] = shader->fogParms.color[ 1 ] * tr.identityLight;
		out->color[ 2 ] = shader->fogParms.color[ 2 ] * tr.identityLight;
		out->color[ 3 ] = 1;

		d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / ( d * 8 );

		// ydnar: global fog sets clearcolor/zfar
		if ( out->originalBrushNumber == -1 )
		{
			s_worldData.globalFog = i + 1;
			VectorCopy( shader->fogParms.color, s_worldData.globalOriginalFog );
			s_worldData.globalOriginalFog[ 3 ] = shader->fogParms.depthForOpaque;
		}

		// set the gradient vector
		sideNum = LittleLong( fogs->visibleSide );

		// ydnar: made this check a little more strenuous (was sideNum == -1)
		if ( sideNum < 0 || sideNum >= sidesCount )
		{
			out->hasSurface = qfalse;
		}
		else
		{
			out->hasSurface = qtrue;
			planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
			VectorSubtract( vec3_origin, s_worldData.planes[ planeNum ].normal, out->surface );
			out->surface[ 3 ] = -s_worldData.planes[ planeNum ].dist;
		}

		out++;
	}

	ri.Printf( PRINT_DEVELOPER, "%i fog volumes loaded\n", s_worldData.numFogs );
}

/*
================
R_LoadLightGrid
================
*/
void R_LoadLightGrid( lump_t *l )
{
	int            i, j, k;
	vec3_t         maxs;
	world_t        *w;
	float          *wMins, *wMaxs;
	dgridPoint_t   *in;
	bspGridPoint1_t *gridPoint1;
	bspGridPoint2_t *gridPoint2;
	float          lat, lng;
	int            from[ 3 ], to[ 3 ];
	float          weights[ 3 ] = { 0.25f, 0.5f, 0.25f };
	float          *factors[ 3 ] = { weights, weights, weights };
	vec3_t         ambientColor, directedColor, direction;
	float          scale;


	ri.Printf( PRINT_DEVELOPER, "...loading light grid\n" );

	w = &s_worldData;

	w->lightGridInverseSize[ 0 ] = 1.0f / w->lightGridSize[ 0 ];
	w->lightGridInverseSize[ 1 ] = 1.0f / w->lightGridSize[ 1 ];
	w->lightGridInverseSize[ 2 ] = 1.0f / w->lightGridSize[ 2 ];

	wMins = w->models[ 0 ].bounds[ 0 ];
	wMaxs = w->models[ 0 ].bounds[ 1 ];

	for ( i = 0; i < 3; i++ )
	{
		w->lightGridOrigin[ i ] = w->lightGridSize[ i ] * ceil( wMins[ i ] / w->lightGridSize[ i ] );
		maxs[ i ] = w->lightGridSize[ i ] * floor( wMaxs[ i ] / w->lightGridSize[ i ] );
		w->lightGridBounds[ i ] = ( maxs[ i ] - w->lightGridOrigin[ i ] ) / w->lightGridSize[ i ] + 1;
	}

	VectorMA( w->lightGridOrigin, -0.5f, w->lightGridSize,
		  w->lightGridGLOrigin );

	w->lightGridGLScale[ 0 ] = w->lightGridInverseSize[ 0 ] / w->lightGridBounds[ 0 ];
	w->lightGridGLScale[ 1 ] = w->lightGridInverseSize[ 1 ] / w->lightGridBounds[ 1 ];
	w->lightGridGLScale[ 2 ] = w->lightGridInverseSize[ 2 ] / w->lightGridBounds[ 2 ];

	w->numLightGridPoints = w->lightGridBounds[ 0 ] * w->lightGridBounds[ 1 ] * w->lightGridBounds[ 2 ];

	ri.Printf( PRINT_DEVELOPER, "grid size (%i %i %i)\n", ( int ) w->lightGridSize[ 0 ], ( int ) w->lightGridSize[ 1 ],
	           ( int ) w->lightGridSize[ 2 ] );
	ri.Printf( PRINT_DEVELOPER, "grid bounds (%i %i %i)\n", ( int ) w->lightGridBounds[ 0 ], ( int ) w->lightGridBounds[ 1 ],
	           ( int ) w->lightGridBounds[ 2 ] );

	if ( l->filelen != w->numLightGridPoints * sizeof( dgridPoint_t ) )
	{
		ri.Printf( PRINT_WARNING, "WARNING: light grid mismatch\n" );
		w->lightGridData1 = NULL;
		w->lightGridData2 = NULL;
		return;
	}

	in = ( dgridPoint_t * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	i = w->numLightGridPoints * ( sizeof( *gridPoint1 ) + sizeof( *gridPoint2 ) );
	gridPoint1 = (bspGridPoint1_t *) ri.Hunk_Alloc( i, h_low );
	gridPoint2 = (bspGridPoint2_t *) (gridPoint1 + w->numLightGridPoints);

	w->lightGridData1 = gridPoint1;
	w->lightGridData2 = gridPoint2;

	for ( i = 0; i < w->numLightGridPoints;
	      i++, in++, gridPoint1++, gridPoint2++ )
	{
		byte tmpAmbient[ 4 ];
		byte tmpDirected[ 4 ];

		tmpAmbient[ 0 ] = in->ambient[ 0 ];
		tmpAmbient[ 1 ] = in->ambient[ 1 ];
		tmpAmbient[ 2 ] = in->ambient[ 2 ];
		tmpAmbient[ 3 ] = 255;

		tmpDirected[ 0 ] = in->directed[ 0 ];
		tmpDirected[ 1 ] = in->directed[ 1 ];
		tmpDirected[ 2 ] = in->directed[ 2 ];
		tmpDirected[ 3 ] = 255;

		R_ColorShiftLightingBytes( tmpAmbient, tmpAmbient );
		R_ColorShiftLightingBytes( tmpDirected, tmpDirected );

		for ( j = 0; j < 3; j++ )
		{
			ambientColor[ j ] = tmpAmbient[ j ] * ( 1.0f / 255.0f );
			directedColor[ j ] = tmpDirected[ j ] * ( 1.0f / 255.0f );
		}

		// standard spherical coordinates to cartesian coordinates conversion

		// decode X as cos( lat ) * sin( long )
		// decode Y as sin( lat ) * sin( long )
		// decode Z as cos( long )

		// RB: having a look in NormalToLatLong used by q3map2 shows the order of latLong

		// Lat = 0 at (1,0,0) to 360 (-1,0,0), encoded in 8-bit sine table format
		// Lng = 0 at (0,0,1) to 180 (0,0,-1), encoded in 8-bit sine table format

		lat = DEG2RAD( in->latLong[ 1 ] * ( 360.0f / 255.0f ) );
		lng = DEG2RAD( in->latLong[ 0 ] * ( 360.0f / 255.0f ) );

		direction[ 0 ] = cos( lat ) * sin( lng );
		direction[ 1 ] = sin( lat ) * sin( lng );
		direction[ 2 ] = cos( lng );

		// Pack data into an bspGridPoint
		gridPoint1->ambient[ 0 ] = floatToUnorm8( ambientColor[ 0 ] );
		gridPoint1->ambient[ 1 ] = floatToUnorm8( ambientColor[ 1 ] );
		gridPoint1->ambient[ 2 ] = floatToUnorm8( ambientColor[ 2 ] );
		gridPoint2->directed[ 0 ] = floatToUnorm8( directedColor[ 0 ] );
		gridPoint2->directed[ 1 ] = floatToUnorm8( directedColor[ 1 ] );
		gridPoint2->directed[ 2 ] = floatToUnorm8( directedColor[ 2 ] );

		scale = fabsf( direction[ 0 ] ) + fabsf( direction[ 1 ] ) + fabsf( direction[ 2 ] );
		if( scale > 0.0f ) {
			VectorScale( direction, 1.0f / scale, direction );
			if( direction[ 2 ] < 0.0f ) {
				float X = direction[ 0 ];
				float Y = direction[ 1 ];
				direction[ 0 ] = copysignf( 1.0f - fabs( Y ), X );
				direction[ 1 ] = copysignf( 1.0f - fabs( X ), Y );
			}
		}
		gridPoint1->lightVecX = floatToSnorm8( direction[ 0 ] );
		gridPoint2->lightVecY = floatToSnorm8( direction[ 1 ] );
	}

	// fill in gridpoints with zero light (samples in walls) to avoid
	// darkening of objects near walls
	gridPoint1 = w->lightGridData1;
	gridPoint2 = w->lightGridData2;

	for( k = 0; k < w->lightGridBounds[ 2 ]; k++ ) {
		from[ 2 ] = k - 1;
		to[ 2 ] = k + 1;

		for( j = 0; j < w->lightGridBounds[ 1 ]; j++ ) {
			from[ 1 ] = j - 1;
			to[ 1 ] = j + 1;

			for( i = 0; i < w->lightGridBounds[ 0 ];
			     i++, gridPoint1++, gridPoint2++ ) {
				from[ 0 ] = i - 1;
				to[ 0 ] = i + 1;

				if( gridPoint1->ambient[ 0 ] ||
				    gridPoint1->ambient[ 1 ] ||
				    gridPoint1->ambient[ 2 ] )
					continue;

				scale = R_InterpolateLightGrid( w, from, to, factors,
								ambientColor, directedColor,
								direction );
				if( scale > 0.0f ) {
					scale = 1.0f / scale;

					VectorScale( ambientColor, scale, ambientColor );
					VectorScale( directedColor, scale, directedColor );
					VectorScale( direction, scale, direction );
					
					gridPoint1->ambient[ 0 ] = floatToUnorm8( ambientColor[ 0 ] );
					gridPoint1->ambient[ 1 ] = floatToUnorm8( ambientColor[ 1 ] );
					gridPoint1->ambient[ 2 ] = floatToUnorm8( ambientColor[ 2 ] );
					gridPoint1->lightVecX = floatToSnorm8( direction[ 0 ] );
					gridPoint2->directed[ 0 ] = floatToUnorm8( directedColor[ 0 ] );
					gridPoint2->directed[ 1 ] = floatToUnorm8( directedColor[ 1 ] );
					gridPoint2->directed[ 2 ] = floatToUnorm8( directedColor[ 2 ] );
					gridPoint2->lightVecY = floatToSnorm8( direction[ 1 ] );
				}
			}
		}
	}

	tr.lightGrid1Image = R_Create3DImage("<lightGrid1>", (const byte *)w->lightGridData1,
					     w->lightGridBounds[ 0 ], w->lightGridBounds[ 1 ],
					     w->lightGridBounds[ 2 ], IF_NOPICMIP | IF_NOLIGHTSCALE | IF_NOCOMPRESSION,
					     FT_LINEAR, WT_EDGE_CLAMP );
	tr.lightGrid2Image = R_Create3DImage("<lightGrid2>", (const byte *)w->lightGridData2,
					     w->lightGridBounds[ 0 ], w->lightGridBounds[ 1 ],
					     w->lightGridBounds[ 2 ], IF_NOPICMIP | IF_NOLIGHTSCALE | IF_NOCOMPRESSION,
					     FT_LINEAR, WT_EDGE_CLAMP );

	ri.Printf( PRINT_DEVELOPER, "%i light grid points created\n", w->numLightGridPoints );
}

/*
================
R_LoadEntities
================
*/
void R_LoadEntities( lump_t *l )
{
	int          i;
	char         *p, *pOld, *token, *s;
	char         keyname[ MAX_TOKEN_CHARS ];
	char         value[ MAX_TOKEN_CHARS ];
	world_t      *w;
	qboolean     isLight = qfalse;
	int          numEntities = 0;
	int          numLights = 0;
	int          numOmniLights = 0;
	int          numProjLights = 0;
	int          numParallelLights = 0;
	trRefLight_t *light;

	ri.Printf( PRINT_DEVELOPER, "...loading entities\n" );

	w = &s_worldData;
	w->lightGridSize[ 0 ] = 64;
	w->lightGridSize[ 1 ] = 64;
	w->lightGridSize[ 2 ] = 128;

	// store for reference by the cgame
	w->entityString = (char*) ri.Hunk_Alloc( l->filelen + 1, h_low );
	//strcpy(w->entityString, (char *)(fileBase + l->fileofs));
	Q_strncpyz( w->entityString, ( char * )( fileBase + l->fileofs ), l->filelen + 1 );
	w->entityParsePoint = w->entityString;

	p = w->entityString;

	// only parse the world spawn
	while ( 1 )
	{
		// parse key
		token = COM_ParseExt2( &p, qtrue );

		if ( !*token )
		{
			ri.Printf( PRINT_WARNING, "WARNING: unexpected end of entities string while parsing worldspawn\n" );
			break;
		}

		if ( *token == '{' )
		{
			continue;
		}

		if ( *token == '}' )
		{
			break;
		}

		Q_strncpyz( keyname, token, sizeof( keyname ) );

		// parse value
		token = COM_ParseExt2( &p, qfalse );

		if ( !*token )
		{
			continue;
		}

		Q_strncpyz( value, token, sizeof( value ) );

		// check for remapping of shaders for vertex lighting
		s = "vertexremapshader";

		if ( !Q_strncmp( keyname, s, strlen( s ) ) )
		{
			s = strchr( value, ';' );

			if ( !s )
			{
				ri.Printf( PRINT_WARNING, "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
				break;
			}

			*s++ = 0;
			continue;
		}

		// check for remapping of shaders
		s = "remapshader";

		if ( !Q_strncmp( keyname, s, strlen( s ) ) )
		{
			s = strchr( value, ';' );

			if ( !s )
			{
				ri.Printf( PRINT_WARNING, "WARNING: no semi colon in shaderremap '%s'\n", value );
				break;
			}

			*s++ = 0;
			R_RemapShader( value, s, "0" );
			continue;
		}

		// check for a different grid size
		if ( !Q_stricmp( keyname, "gridsize" ) )
		{
			sscanf( value, "%f %f %f", &w->lightGridSize[ 0 ], &w->lightGridSize[ 1 ], &w->lightGridSize[ 2 ] );
			continue;
		}

		// check for ambient color
		else if ( !Q_stricmp( keyname, "_color" ) || !Q_stricmp( keyname, "ambientColor" ) )
		{
			if ( r_forceAmbient->value <= 0 )
			{
				sscanf( value, "%f %f %f", &tr.worldEntity.ambientLight[ 0 ], &tr.worldEntity.ambientLight[ 1 ],
				        &tr.worldEntity.ambientLight[ 2 ] );

				VectorCopy( tr.worldEntity.ambientLight, tr.worldEntity.ambientLight );
				VectorScale( tr.worldEntity.ambientLight, r_ambientScale->value, tr.worldEntity.ambientLight );
			}
		}

		// check for fog color
		else if ( !Q_stricmp( keyname, "fogColor" ) )
		{
			sscanf( value, "%f %f %f", &tr.fogColor[ 0 ], &tr.fogColor[ 1 ], &tr.fogColor[ 2 ] );
		}

		// check for fog density
		else if ( !Q_stricmp( keyname, "fogDensity" ) )
		{
			tr.fogDensity = atof( value );
		}

		// check for deluxe mapping support
		if ( !Q_stricmp( keyname, "deluxeMapping" ) && !Q_stricmp( value, "1" ) )
		{
			ri.Printf( PRINT_DEVELOPER, "map features directional light mapping\n" );
			tr.worldDeluxeMapping = qtrue;
			continue;
		}

		// check for mapOverBrightBits override
		else if ( !Q_stricmp( keyname, "mapOverBrightBits" ) )
		{
			tr.mapOverBrightBits = Maths::clamp( atof( value ), 0.0, 3.0 );
		}

		// check for deluxe mapping provided by NetRadiant's q3map2
		if ( !Q_stricmp( keyname, "_q3map2_cmdline" ) )
		{
			s = strstr( value, "-deluxe" );

			if ( s )
			{
				ri.Printf( PRINT_DEVELOPER, "map features directional light mapping\n" );
				tr.worldDeluxeMapping = qtrue;
			}

			continue;
		}

		// check for HDR light mapping support
		if ( !Q_stricmp( keyname, "hdrRGBE" ) && !Q_stricmp( value, "1" ) )
		{
			ri.Printf( PRINT_DEVELOPER, "map features HDR light mapping\n" );
			tr.worldHDR_RGBE = qtrue;
			continue;
		}

		if ( !Q_stricmp( keyname, "classname" ) && Q_stricmp( value, "worldspawn" ) )
		{
			ri.Printf( PRINT_WARNING, "WARNING: expected worldspawn found '%s'\n", value );
			continue;
		}
	}

	pOld = p;
	numEntities = 1; // parsed worldspawn so far

	// count lights
	while ( 1 )
	{
		// parse {
		token = COM_ParseExt2( &p, qtrue );

		if ( !*token )
		{
			// end of entities string
			break;
		}

		if ( *token != '{' )
		{
			ri.Printf( PRINT_WARNING, "WARNING: expected { found '%s'\n", token );
			break;
		}

		// new entity
		isLight = qfalse;

		// parse epairs
		while ( 1 )
		{
			// parse key
			token = COM_ParseExt2( &p, qtrue );

			if ( *token == '}' )
			{
				break;
			}

			if ( !*token )
			{
				ri.Printf( PRINT_WARNING, "WARNING: EOF without closing bracket\n" );
				break;
			}

			Q_strncpyz( keyname, token, sizeof( keyname ) );

			// parse value
			token = COM_ParseExt2( &p, qfalse );

			if ( !*token )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing value for key '%s'\n", keyname );
				continue;
			}

			Q_strncpyz( value, token, sizeof( value ) );

			// check if this entity is a light
			if ( !Q_stricmp( keyname, "classname" ) && !Q_stricmp( value, "light" ) )
			{
				isLight = qtrue;
			}
		}

		if ( *token != '}' )
		{
			ri.Printf( PRINT_WARNING, "WARNING: expected } found '%s'\n", token );
			break;
		}

		if ( isLight )
		{
			numLights++;
		}

		numEntities++;
	}

	ri.Printf( PRINT_DEVELOPER, "%i total entities counted\n", numEntities );
	ri.Printf( PRINT_DEVELOPER, "%i total lights counted\n", numLights );

	s_worldData.numLights = numLights;

	// Tr3B: FIXME add 1 dummy light so we don't trash the hunk memory system ...
	s_worldData.lights = (trRefLight_t*) ri.Hunk_Alloc( ( s_worldData.numLights + 1 ) * sizeof( trRefLight_t ), h_low );

	// basic light setup
	for ( i = 0, light = s_worldData.lights; i < s_worldData.numLights; i++, light++ )
	{
		QuatClear( light->l.rotation );
		VectorClear( light->l.center );

		light->l.color[ 0 ] = 1;
		light->l.color[ 1 ] = 1;
		light->l.color[ 2 ] = 1;

		light->l.scale = r_lightScale->value;

		light->l.radius[ 0 ] = 300;
		light->l.radius[ 1 ] = 300;
		light->l.radius[ 2 ] = 300;

		VectorClear( light->l.projTarget );
		VectorClear( light->l.projRight );
		VectorClear( light->l.projUp );
		VectorClear( light->l.projStart );
		VectorClear( light->l.projEnd );

		light->l.inverseShadows = qfalse;

		light->isStatic = qtrue;
		light->noRadiosity = qfalse;
		light->additive = qtrue;

		light->shadowLOD = 0;
	}

	// parse lights
	p = pOld;
	numEntities = 1;
	light = &s_worldData.lights[ 0 ];

	while ( 1 )
	{
		// parse {
		token = COM_ParseExt2( &p, qtrue );

		if ( !*token )
		{
			// end of entities string
			break;
		}

		if ( *token != '{' )
		{
			ri.Printf( PRINT_WARNING, "WARNING: expected { found '%s'\n", token );
			break;
		}

		// new entity
		isLight = qfalse;

		// parse epairs
		while ( 1 )
		{
			// parse key
			token = COM_ParseExt2( &p, qtrue );

			if ( *token == '}' )
			{
				break;
			}

			if ( !*token )
			{
				ri.Printf( PRINT_WARNING, "WARNING: EOF without closing bracket\n" );
				break;
			}

			Q_strncpyz( keyname, token, sizeof( keyname ) );

			// parse value
			token = COM_ParseExt2( &p, qfalse );

			if ( !*token )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing value for key '%s'\n", keyname );
				continue;
			}

			Q_strncpyz( value, token, sizeof( value ) );

			// check if this entity is a light
			if ( !Q_stricmp( keyname, "classname" ) && !Q_stricmp( value, "light" ) )
			{
				isLight = qtrue;
			}
			// check for origin
			else if ( !Q_stricmp( keyname, "origin" ) || !Q_stricmp( keyname, "light_origin" ) )
			{
				sscanf( value, "%f %f %f", &light->l.origin[ 0 ], &light->l.origin[ 1 ], &light->l.origin[ 2 ] );
				s = &value[ 0 ];
			}
			// check for center
			else if ( !Q_stricmp( keyname, "light_center" ) )
			{
				sscanf( value, "%f %f %f", &light->l.center[ 0 ], &light->l.center[ 1 ], &light->l.center[ 2 ] );
				s = &value[ 0 ];
			}
			// check for color
			else if ( !Q_stricmp( keyname, "_color" ) )
			{
				sscanf( value, "%f %f %f", &light->l.color[ 0 ], &light->l.color[ 1 ], &light->l.color[ 2 ] );
				s = &value[ 0 ];
			}
			// check for radius
			else if ( !Q_stricmp( keyname, "light_radius" ) )
			{
				sscanf( value, "%f %f %f", &light->l.radius[ 0 ], &light->l.radius[ 1 ], &light->l.radius[ 2 ] );
				s = &value[ 0 ];
			}
			// check for light_target
			else if ( !Q_stricmp( keyname, "light_target" ) )
			{
				sscanf( value, "%f %f %f", &light->l.projTarget[ 0 ], &light->l.projTarget[ 1 ], &light->l.projTarget[ 2 ] );
				s = &value[ 0 ];
				light->l.rlType = RL_PROJ;
			}
			// check for light_right
			else if ( !Q_stricmp( keyname, "light_right" ) )
			{
				sscanf( value, "%f %f %f", &light->l.projRight[ 0 ], &light->l.projRight[ 1 ], &light->l.projRight[ 2 ] );
				s = &value[ 0 ];
				light->l.rlType = RL_PROJ;
			}
			// check for light_up
			else if ( !Q_stricmp( keyname, "light_up" ) )
			{
				sscanf( value, "%f %f %f", &light->l.projUp[ 0 ], &light->l.projUp[ 1 ], &light->l.projUp[ 2 ] );
				s = &value[ 0 ];
				light->l.rlType = RL_PROJ;
			}
			// check for light_start
			else if ( !Q_stricmp( keyname, "light_start" ) )
			{
				sscanf( value, "%f %f %f", &light->l.projStart[ 0 ], &light->l.projStart[ 1 ], &light->l.projStart[ 2 ] );
				s = &value[ 0 ];
				light->l.rlType = RL_PROJ;
			}
			// check for light_end
			else if ( !Q_stricmp( keyname, "light_end" ) )
			{
				sscanf( value, "%f %f %f", &light->l.projEnd[ 0 ], &light->l.projEnd[ 1 ], &light->l.projEnd[ 2 ] );
				s = &value[ 0 ];
				light->l.rlType = RL_PROJ;
			}
			// check for radius
			else if ( !Q_stricmp( keyname, "light" ) || !Q_stricmp( keyname, "_light" ) )
			{
				vec_t value2;

				value2 = atof( value );
				light->l.radius[ 0 ] = value2;
				light->l.radius[ 1 ] = value2;
				light->l.radius[ 2 ] = value2;
			}
			// check for scale
			else if ( !Q_stricmp( keyname, "light_scale" ) )
			{
				light->l.scale = atof( value );

				if ( light->l.scale >= r_lightScale->value )
				{
					light->l.scale = r_lightScale->value;
				}
			}
			// check for light shader
			else if ( !Q_stricmp( keyname, "texture" ) )
			{
				light->l.attenuationShader = RE_RegisterShader( value, RSF_LIGHT_ATTENUATION );
			}
			// check for rotation
			else if ( !Q_stricmp( keyname, "rotation" ) || !Q_stricmp( keyname, "light_rotation" ) )
			{
				matrix_t rotation;

				sscanf( value, "%f %f %f %f %f %f %f %f %f", &rotation[ 0 ], &rotation[ 1 ], &rotation[ 2 ],
				        &rotation[ 4 ], &rotation[ 5 ], &rotation[ 6 ], &rotation[ 8 ], &rotation[ 9 ], &rotation[ 10 ] );

				QuatFromMatrix( light->l.rotation, rotation );
			}
			// check if this light does not cast any shadows
			else if ( !Q_stricmp( keyname, "noshadows" ) && !Q_stricmp( value, "1" ) )
			{
				light->l.noShadows = qtrue;
			}
			// check if this light does not contribute to the global lightmapping
			else if ( !Q_stricmp( keyname, "noradiosity" ) && !Q_stricmp( value, "1" ) )
			{
				light->noRadiosity = qtrue;
			}
			// check if this light is a parallel sun light
			else if ( !Q_stricmp( keyname, "parallel" ) && !Q_stricmp( value, "1" ) )
			{
				light->l.rlType = RL_DIRECTIONAL;
			}
		}

		if ( *token != '}' )
		{
			ri.Printf( PRINT_WARNING, "WARNING: expected } found '%s'\n", token );
			break;
		}

		if ( !isLight )
		{
			// reset rotation because it may be set to the rotation of other entities
			QuatClear( light->l.rotation );
		}
		else
		{
			if ( ( numOmniLights + numProjLights + numParallelLights ) < s_worldData.numLights )
			{
				switch ( light->l.rlType )
				{
					case RL_OMNI:
						numOmniLights++;
						break;

					case RL_PROJ:
						numProjLights++;
						break;

					case RL_DIRECTIONAL:
						numParallelLights++;
						break;

					default:
						break;
				}

				light++;
			}
		}

		numEntities++;
	}

	if ( ( numOmniLights + numProjLights + numParallelLights ) != s_worldData.numLights )
	{
		ri.Error( ERR_DROP, "counted %i lights and parsed %i lights", s_worldData.numLights, ( numOmniLights + numProjLights + numParallelLights ) );
	}

	ri.Printf( PRINT_DEVELOPER, "%i total entities parsed\n", numEntities );
	ri.Printf( PRINT_DEVELOPER, "%i total lights parsed\n", numOmniLights + numProjLights );
	ri.Printf( PRINT_DEVELOPER, "%i omni-directional lights parsed\n", numOmniLights );
	ri.Printf( PRINT_DEVELOPER, "%i projective lights parsed\n", numProjLights );
	ri.Printf( PRINT_DEVELOPER, "%i directional lights parsed\n", numParallelLights );
}

/*
=================
R_GetEntityToken
=================
*/
qboolean R_GetEntityToken( char *buffer, int size )
{
	const char *s;

	s = COM_Parse2( &s_worldData.entityParsePoint );
	Q_strncpyz( buffer, s, size );

	if ( !s_worldData.entityParsePoint || !s[ 0 ] )
	{
		s_worldData.entityParsePoint = s_worldData.entityString;
		return qfalse;
	}
	else
	{
		return qtrue;
	}
}

/*
=================
R_PrecacheInteraction
=================
*/
static void R_PrecacheInteraction( trRefLight_t *light, bspSurface_t *surface )
{
	interactionCache_t *iaCache;

	iaCache = (interactionCache_t*) ri.Hunk_Alloc( sizeof( *iaCache ), h_low );
	Com_AddToGrowList( &s_interactions, iaCache );

	// connect to interaction grid
	if ( !light->firstInteractionCache )
	{
		light->firstInteractionCache = iaCache;
	}

	if ( light->lastInteractionCache )
	{
		light->lastInteractionCache->next = iaCache;
	}

	light->lastInteractionCache = iaCache;

	iaCache->next = NULL;
	iaCache->surface = surface;

	iaCache->redundant = qfalse;
}

/*
================
R_PrecacheGenericSurfInteraction
================
*/
static qboolean R_PrecacheGenericSurfInteraction( srfGeneric_t *face, trRefLight_t *light )
{
	if ( !BoundsIntersect( face->bounds[ 0 ], face->bounds[ 1 ], light->worldBounds[ 0 ], light->worldBounds[ 1 ] ) )
	{
		return qfalse;
	}

	return qtrue;
}

/*
======================
R_PrecacheInteractionSurface
======================
*/
static void R_PrecacheInteractionSurface( bspSurface_t *surf, trRefLight_t *light )
{
	qboolean intersects;

	if ( surf->lightCount == s_lightCount )
	{
		return; // already checked this surface
	}

	surf->lightCount = s_lightCount;

	// skip all surfaces that don't matter for lighting only pass
	if ( surf->shader->isSky || ( !surf->shader->interactLight && surf->shader->noShadows ) )
	{
		return;
	}

	switch ( *surf->data )
	{
		case SF_FACE:
		case SF_GRID:
		case SF_TRIANGLES:
			intersects = R_PrecacheGenericSurfInteraction( ( srfGeneric_t * ) surf->data, light );
			break;
		default:
			intersects = qfalse;
			break;
	}

	if ( intersects )
	{
		R_PrecacheInteraction( light, surf );
	}
}

/*
================
R_RecursivePrecacheInteractionNode
================
*/
static void R_RecursivePrecacheInteractionNode( bspNode_t *node, trRefLight_t *light )
{
	int r;

	do
	{
		// light already hit node
		if ( node->lightCount == s_lightCount )
		{
			return;
		}

		node->lightCount = s_lightCount;

		if ( node->contents != -1 )
		{
			break;
		}

		// node is just a decision point, so go down both sides
		// since we don't care about sort orders, just go positive to negative
		r = BoxOnPlaneSide( light->worldBounds[ 0 ], light->worldBounds[ 1 ], node->plane );

		switch ( r )
		{
			case 1:
				node = node->children[ 0 ];
				break;

			case 2:
				node = node->children[ 1 ];
				break;

			case 3:
			default:
				// recurse down the children, front side first
				R_RecursivePrecacheInteractionNode( node->children[ 0 ], light );

				// tail recurse
				node = node->children[ 1 ];
				break;
		}
	}
	while ( 1 );

	{
		// leaf node, so add mark surfaces
		int          c;
		bspSurface_t *surf, **mark;
		vec3_t       worldBounds[ 2 ];

		// add the individual surfaces
		mark = node->markSurfaces;
		c = node->numMarkSurfaces;

		while ( c-- )
		{
			// the surface may have already been added if it
			// spans multiple leafs
			surf = *mark;
			R_PrecacheInteractionSurface( surf, light );
			mark++;
		}

		VectorCopy( node->mins, worldBounds[ 0 ] );
		VectorCopy( node->maxs, worldBounds[ 1 ] );

		if ( node->numMarkSurfaces > 0 && R_CullLightWorldBounds( light, worldBounds ) != CULL_OUT )
		{
			link_t *l;

			l = ( link_t *)ri.Hunk_Alloc( sizeof( *l ), h_low );
			InitLink( l, node );

			InsertLink( l, &light->leafs );

			light->leafs.numElements++;
		}
	}
}

/*
=================
R_CreateInteractionVBO
=================
*/
static interactionVBO_t *R_CreateInteractionVBO( trRefLight_t *light )
{
	interactionVBO_t *iaVBO;

	iaVBO = (interactionVBO_t*) ri.Hunk_Alloc( sizeof( *iaVBO ), h_low );

	// connect to interaction grid
	if ( !light->firstInteractionVBO )
	{
		light->firstInteractionVBO = iaVBO;
	}

	if ( light->lastInteractionVBO )
	{
		light->lastInteractionVBO->next = iaVBO;
	}

	light->lastInteractionVBO = iaVBO;
	iaVBO->next = NULL;

	return iaVBO;
}

/*
=================
InteractionCacheCompare
compare function for qsort()
=================
*/
static int InteractionCacheCompare( const void *a, const void *b )
{
	interactionCache_t *aa, *bb;

	aa = * ( interactionCache_t ** ) a;
	bb = * ( interactionCache_t ** ) b;

	// shader first
	if ( aa->surface->shader < bb->surface->shader )
	{
		return -1;
	}

	else if ( aa->surface->shader > bb->surface->shader )
	{
		return 1;
	}

	// then alphaTest
	if ( aa->surface->shader->alphaTest < bb->surface->shader->alphaTest )
	{
		return -1;
	}

	else if ( aa->surface->shader->alphaTest > bb->surface->shader->alphaTest )
	{
		return 1;
	}

	return 0;
}

static int UpdateLightTriangles( const srfVert_t *verts, int numTriangles, srfTriangle_t *triangles, shader_t *surfaceShader,
                                 trRefLight_t *light )
{
	int           i;
	srfTriangle_t *tri;
	int           numFacing;

	numFacing = 0;

	for ( i = 0, tri = triangles; i < numTriangles; i++, tri++ )
	{
		vec3_t pos[ 3 ];
		vec4_t triPlane;
		float  d;

		VectorCopy( verts[ tri->indexes[ 0 ] ].xyz, pos[ 0 ] );
		VectorCopy( verts[ tri->indexes[ 1 ] ].xyz, pos[ 1 ] );
		VectorCopy( verts[ tri->indexes[ 2 ] ].xyz, pos[ 2 ] );

		if ( PlaneFromPoints( triPlane, pos[ 0 ], pos[ 1 ], pos[ 2 ] ) )
		{

			if ( light->l.rlType == RL_DIRECTIONAL )
			{
				vec3_t lightDirection;

				// light direction is from surface to light
				VectorCopy( tr.sunDirection, lightDirection );

				d = DotProduct( triPlane, lightDirection );

				if ( surfaceShader->cullType == CT_TWO_SIDED || ( d > 0 && surfaceShader->cullType != CT_BACK_SIDED ) )
				{
					tri->facingLight = qtrue;
				}
				else
				{
					tri->facingLight = qfalse;
				}
			}
			else
			{
				// check if light origin is behind triangle
				d = DotProduct( triPlane, light->origin ) - triPlane[ 3 ];

				if ( surfaceShader->cullType == CT_TWO_SIDED || ( d > 0 && surfaceShader->cullType != CT_BACK_SIDED ) )
				{
					tri->facingLight = qtrue;
				}
				else
				{
					tri->facingLight = qfalse;
				}
			}
		}
		else
		{
			tri->facingLight = qtrue; // FIXME ?
		}

		if ( R_CullLightTriangle( light, pos ) == CULL_OUT )
		{
			tri->facingLight = qfalse;
		}

		if ( tri->facingLight )
		{
			numFacing++;
		}
	}

	return numFacing;
}

/*
===============
R_CreateVBOLightMeshes
===============
*/
static void R_CreateVBOLightMeshes( trRefLight_t *light )
{
	int                i, j, k, l;

	int                numVerts;

	int                numTriangles, numLitTriangles;
	srfTriangle_t      *triangles;
	srfTriangle_t      *tri;

	interactionVBO_t   *iaVBO;

	interactionCache_t *iaCache, *iaCache2;
	interactionCache_t **iaCachesSorted;
	int                numCaches;

	shader_t           *shader, *oldShader;

	bspSurface_t       *surface;

	srfVBOMesh_t       *vboSurf;
	vec3_t             bounds[ 2 ];

	if ( !r_vboLighting->integer )
	{
		return;
	}

	if ( !light->firstInteractionCache )
	{
		// this light has no interactions precached
		return;
	}

	// count number of interaction caches
	numCaches = 0;

	for ( iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next )
	{
		if ( iaCache->redundant )
		{
			continue;
		}

		surface = iaCache->surface;

		if ( !surface->shader->interactLight )
		{
			continue;
		}

		if ( surface->shader->isPortal )
		{
			continue;
		}

		if ( ShaderRequiresCPUDeforms( surface->shader ) )
		{
			continue;
		}

		numCaches++;
	}

	// build interaction caches list
	iaCachesSorted = (interactionCache_t**) ri.Hunk_AllocateTempMemory( numCaches * sizeof( iaCachesSorted[ 0 ] ) );

	numCaches = 0;

	for ( iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next )
	{
		if ( iaCache->redundant )
		{
			continue;
		}

		surface = iaCache->surface;

		if ( !surface->shader->interactLight )
		{
			continue;
		}

		if ( surface->shader->isPortal )
		{
			continue;
		}

		if ( ShaderRequiresCPUDeforms( surface->shader ) )
		{
			continue;
		}

		iaCachesSorted[ numCaches ] = iaCache;
		numCaches++;
	}

	// sort interaction caches by shader
	qsort( iaCachesSorted, numCaches, sizeof( *iaCachesSorted ), InteractionCacheCompare );

	// create a VBO for each shader
	shader = oldShader = NULL;

	for ( k = 0; k < numCaches; k++ )
	{
		iaCache = iaCachesSorted[ k ];

		iaCache->mergedIntoVBO = qtrue;

		shader = iaCache->surface->shader;

		if ( shader != oldShader )
		{
			oldShader = shader;

			// count vertices and indices
			numVerts = 0;
			numTriangles = 0;

			ClearBounds( bounds[ 0 ], bounds[ 1 ] );

			for ( l = k; l < numCaches; l++ )
			{
				iaCache2 = iaCachesSorted[ l ];

				surface = iaCache2->surface;

				if ( surface->shader != shader )
				{
					continue;
				}

				if ( *surface->data == SF_FACE )
				{
					srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface->data;

					if ( srf->numTriangles )
					{
						numLitTriangles = UpdateLightTriangles( s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light );

						if ( numLitTriangles )
						{
							BoundsAdd( bounds[ 0 ], bounds[ 1 ], srf->bounds[ 0 ], srf->bounds[ 1 ] );
						}

						numTriangles += numLitTriangles;
					}

					if ( srf->numVerts )
					{
						numVerts += srf->numVerts;
					}
				}
				else if ( *surface->data == SF_GRID )
				{
					srfGridMesh_t *srf = ( srfGridMesh_t * ) surface->data;

					if ( srf->numTriangles )
					{
						numLitTriangles = UpdateLightTriangles( s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light );

						if ( numLitTriangles )
						{
							BoundsAdd( bounds[ 0 ], bounds[ 1 ], srf->bounds[ 0 ], srf->bounds[ 1 ] );
						}

						numTriangles += numLitTriangles;
					}

					if ( srf->numVerts )
					{
						numVerts += srf->numVerts;
					}
				}
				else if ( *surface->data == SF_TRIANGLES )
				{
					srfTriangles_t *srf = ( srfTriangles_t * ) surface->data;

					if ( srf->numTriangles )
					{
						numLitTriangles = UpdateLightTriangles( s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light );

						if ( numLitTriangles )
						{
							BoundsAdd( bounds[ 0 ], bounds[ 1 ], srf->bounds[ 0 ], srf->bounds[ 1 ] );
						}

						numTriangles += numLitTriangles;
					}

					if ( srf->numVerts )
					{
						numVerts += srf->numVerts;
					}
				}
			}

			if ( !numVerts || !numTriangles )
			{
				continue;
			}

			// create surface
			vboSurf = (srfVBOMesh_t*) ri.Hunk_Alloc( sizeof( *vboSurf ), h_low );
			vboSurf->surfaceType = SF_VBO_MESH;
			vboSurf->numIndexes = numTriangles * 3;
			vboSurf->numVerts = numVerts;
			vboSurf->lightmapNum = -1;

			VectorCopy( bounds[ 0 ], vboSurf->bounds[ 0 ] );
			VectorCopy( bounds[ 1 ], vboSurf->bounds[ 1 ] );

			// create arrays
			triangles = (srfTriangle_t*) ri.Hunk_AllocateTempMemory( numTriangles * sizeof( srfTriangle_t ) );
			numTriangles = 0;

			// build triangle indices
			for ( l = k; l < numCaches; l++ )
			{
				iaCache2 = iaCachesSorted[ l ];

				surface = iaCache2->surface;

				if ( surface->shader != shader )
				{
					continue;
				}

				if ( *surface->data == SF_FACE )
				{
					srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface->data;

					for ( i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++ )
					{
						if ( tri->facingLight )
						{
							for ( j = 0; j < 3; j++ )
							{
								triangles[ numTriangles ].indexes[ j ] = tri->indexes[ j ];
							}

							numTriangles++;
						}
					}
				}
				else if ( *surface->data == SF_GRID )
				{
					srfGridMesh_t *srf = ( srfGridMesh_t * ) surface->data;

					for ( i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++ )
					{
						if ( tri->facingLight )
						{
							for ( j = 0; j < 3; j++ )
							{
								triangles[ numTriangles ].indexes[ j ] = tri->indexes[ j ];
							}

							numTriangles++;
						}
					}
				}
				else if ( *surface->data == SF_TRIANGLES )
				{
					srfTriangles_t *srf = ( srfTriangles_t * ) surface->data;

					for ( i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++ )
					{
						if ( tri->facingLight )
						{
							for ( j = 0; j < 3; j++ )
							{
								triangles[ numTriangles ].indexes[ j ] = tri->indexes[ j ];
							}

							numTriangles++;
						}
					}
				}
			}

			vboSurf->vbo = s_worldData.vbo;
			vboSurf->ibo = R_CreateStaticIBO2( va( "staticLightMesh_IBO %i", c_vboLightSurfaces ), numTriangles, triangles );

			ri.Hunk_FreeTempMemory( triangles );

			// add everything needed to the light
			iaVBO = R_CreateInteractionVBO( light );
			iaVBO->shader = ( struct shader_s * ) shader;
			iaVBO->vboLightMesh = ( struct srfVBOMesh_s * ) vboSurf;

			c_vboLightSurfaces++;
		}
	}

	ri.Hunk_FreeTempMemory( iaCachesSorted );
}

/*
===============
R_CreateVBOShadowMeshes
===============
*/
static void R_CreateVBOShadowMeshes( trRefLight_t *light )
{
	int                i, j, k, l;

	int                numVerts;

	int                numTriangles, numLitTriangles;
	srfTriangle_t      *triangles;
	srfTriangle_t      *tri;

	interactionVBO_t   *iaVBO;

	interactionCache_t *iaCache, *iaCache2;
	interactionCache_t **iaCachesSorted;
	int                numCaches;

	shader_t           *shader, *oldShader;
	qboolean           alphaTest, oldAlphaTest;

	bspSurface_t       *surface;

	srfVBOMesh_t       *vboSurf;
	vec3_t             bounds[ 2 ];

	if ( !r_vboShadows->integer )
	{
		return;
	}

	if ( r_shadows->integer < SHADOWING_ESM16 )
	{
		return;
	}

	if ( !light->firstInteractionCache )
	{
		// this light has no interactions precached
		return;
	}

	if ( light->l.noShadows )
	{
		return;
	}

	switch ( light->l.rlType )
	{
		case RL_OMNI:
			return;

		case RL_DIRECTIONAL:
		case RL_PROJ:
			break;

		default:
			return;
	}

	// count number of interaction caches
	numCaches = 0;

	for ( iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next )
	{
		if ( iaCache->redundant )
		{
			continue;
		}

		surface = iaCache->surface;

		if ( !surface->shader->interactLight )
		{
			continue;
		}

		if ( surface->shader->isSky )
		{
			continue;
		}

		if ( surface->shader->noShadows )
		{
			continue;
		}

		if ( surface->shader->sort > SS_OPAQUE )
		{
			continue;
		}

		if ( surface->shader->isPortal )
		{
			continue;
		}

		if ( ShaderRequiresCPUDeforms( surface->shader ) )
		{
			continue;
		}

		numCaches++;
	}

	// build interaction caches list
	iaCachesSorted = (interactionCache_t**) ri.Hunk_AllocateTempMemory( numCaches * sizeof( iaCachesSorted[ 0 ] ) );

	numCaches = 0;

	for ( iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next )
	{
		if ( iaCache->redundant )
		{
			continue;
		}

		surface = iaCache->surface;

		if ( !surface->shader->interactLight )
		{
			continue;
		}

		if ( surface->shader->isSky )
		{
			continue;
		}

		if ( surface->shader->noShadows )
		{
			continue;
		}

		if ( surface->shader->sort > SS_OPAQUE )
		{
			continue;
		}

		if ( surface->shader->isPortal )
		{
			continue;
		}

		if ( ShaderRequiresCPUDeforms( surface->shader ) )
		{
			continue;
		}

		iaCachesSorted[ numCaches ] = iaCache;
		numCaches++;
	}

	// sort interaction caches by shader
	qsort( iaCachesSorted, numCaches, sizeof( *iaCachesSorted ), InteractionCacheCompare );

	// create a VBO for each shader
	shader = oldShader = NULL;
	oldAlphaTest = alphaTest = -1;

	for ( k = 0; k < numCaches; k++ )
	{
		iaCache = iaCachesSorted[ k ];

		iaCache->mergedIntoVBO = qtrue;

		shader = iaCache->surface->shader;
		alphaTest = shader->alphaTest;

		iaCache->mergedIntoVBO = qtrue;

		if ( alphaTest ? shader != oldShader : alphaTest != oldAlphaTest )
		{
			oldShader = shader;
			oldAlphaTest = alphaTest;

			// count vertices and indices
			numVerts = 0;
			numTriangles = 0;

			ClearBounds( bounds[ 0 ], bounds[ 1 ] );

			for ( l = k; l < numCaches; l++ )
			{
				iaCache2 = iaCachesSorted[ l ];

				surface = iaCache2->surface;

				if ( alphaTest )
				{
					if ( surface->shader != shader )
					{
						break;
					}
				}
				else
				{
					if ( surface->shader->alphaTest != alphaTest )
					{
						break;
					}
				}

				if ( *surface->data == SF_FACE )
				{
					srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface->data;

					if ( srf->numTriangles )
					{
						numLitTriangles = UpdateLightTriangles( s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light );

						if ( numLitTriangles )
						{
							BoundsAdd( bounds[ 0 ], bounds[ 1 ], srf->bounds[ 0 ], srf->bounds[ 1 ] );
						}

						numTriangles += numLitTriangles;
					}

					if ( srf->numVerts )
					{
						numVerts += srf->numVerts;
					}
				}
				else if ( *surface->data == SF_GRID )
				{
					srfGridMesh_t *srf = ( srfGridMesh_t * ) surface->data;

					if ( srf->numTriangles )
					{
						numLitTriangles = UpdateLightTriangles( s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light );

						if ( numLitTriangles )
						{
							BoundsAdd( bounds[ 0 ], bounds[ 1 ], srf->bounds[ 0 ], srf->bounds[ 1 ] );
						}

						numTriangles += numLitTriangles;
					}

					if ( srf->numVerts )
					{
						numVerts += srf->numVerts;
					}
				}
				else if ( *surface->data == SF_TRIANGLES )
				{
					srfTriangles_t *srf = ( srfTriangles_t * ) surface->data;

					if ( srf->numTriangles )
					{
						numLitTriangles = UpdateLightTriangles( s_worldData.verts, srf->numTriangles, s_worldData.triangles + srf->firstTriangle, surface->shader, light );

						if ( numLitTriangles )
						{
							BoundsAdd( bounds[ 0 ], bounds[ 1 ], srf->bounds[ 0 ], srf->bounds[ 1 ] );
						}

						numTriangles += numLitTriangles;
					}

					if ( srf->numVerts )
					{
						numVerts += srf->numVerts;
					}
				}
			}

			if ( !numVerts || !numTriangles )
			{
				continue;
			}

			// create surface
			vboSurf = (srfVBOMesh_t*) ri.Hunk_Alloc( sizeof( *vboSurf ), h_low );
			vboSurf->surfaceType = SF_VBO_MESH;
			vboSurf->numIndexes = numTriangles * 3;
			vboSurf->numVerts = numVerts;
			vboSurf->lightmapNum = -1;

			VectorCopy( bounds[ 0 ], vboSurf->bounds[ 0 ] );
			VectorCopy( bounds[ 1 ], vboSurf->bounds[ 1 ] );

			// create arrays
			triangles = (srfTriangle_t*) ri.Hunk_AllocateTempMemory( numTriangles * sizeof( srfTriangle_t ) );
			numTriangles = 0;

			// build triangle indices
			for ( l = k; l < numCaches; l++ )
			{
				iaCache2 = iaCachesSorted[ l ];

				surface = iaCache2->surface;

				if ( alphaTest )
				{
					if ( surface->shader != shader )
					{
						break;
					}
				}
				else
				{
					if ( surface->shader->alphaTest != alphaTest )
					{
						break;
					}
				}

				if ( *surface->data == SF_FACE )
				{
					srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface->data;

					for ( i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++ )
					{
						if ( tri->facingLight )
						{
							for ( j = 0; j < 3; j++ )
							{
								triangles[ numTriangles ].indexes[ j ] = tri->indexes[ j ];
							}

							numTriangles++;
						}
					}
				}
				else if ( *surface->data == SF_GRID )
				{
					srfGridMesh_t *srf = ( srfGridMesh_t * ) surface->data;

					for ( i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++ )
					{
						if ( tri->facingLight )
						{
							for ( j = 0; j < 3; j++ )
							{
								triangles[ numTriangles ].indexes[ j ] = tri->indexes[ j ];
							}

							numTriangles++;
						}
					}
				}
				else if ( *surface->data == SF_TRIANGLES )
				{
					srfTriangles_t *srf = ( srfTriangles_t * ) surface->data;

					for ( i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++ )
					{
						if ( tri->facingLight )
						{
							for ( j = 0; j < 3; j++ )
							{
								triangles[ numTriangles ].indexes[ j ] = tri->indexes[ j ];
							}

							numTriangles++;
						}
					}
				}
			}

			vboSurf->vbo = s_worldData.vbo;
			vboSurf->ibo = R_CreateStaticIBO2( va( "staticShadowMesh_IBO %i", c_vboLightSurfaces ), numTriangles, triangles );

			ri.Hunk_FreeTempMemory( triangles );

			// add everything needed to the light
			iaVBO = R_CreateInteractionVBO( light );
			iaVBO->shader = ( struct shader_s * ) shader;
			iaVBO->vboShadowMesh = ( struct srfVBOMesh_s * ) vboSurf;

			c_vboShadowSurfaces++;
		}
	}

	ri.Hunk_FreeTempMemory( iaCachesSorted );
}

/*
===============
R_CreateVBOShadowCubeMeshes
===============
*/
static void R_CreateVBOShadowCubeMeshes( trRefLight_t *light )
{
	int                i, j, k, l;

	int                numVerts;

	int                numTriangles;
	srfTriangle_t      *triangles;
	srfTriangle_t      *tri;

	interactionVBO_t   *iaVBO;

	interactionCache_t *iaCache, *iaCache2;
	interactionCache_t **iaCachesSorted;
	int                numCaches;

	shader_t           *shader, *oldShader;
	qboolean           alphaTest, oldAlphaTest;
	int                cubeSide;

	bspSurface_t       *surface;

	srfVBOMesh_t       *vboSurf;

	if ( !r_vboShadows->integer )
	{
		return;
	}

	if ( r_shadows->integer < SHADOWING_ESM16 )
	{
		return;
	}

	if ( !light->firstInteractionCache )
	{
		// this light has no interactions precached
		return;
	}

	if ( light->l.noShadows )
	{
		return;
	}

	if ( light->l.rlType != RL_OMNI )
	{
		return;
	}

	// count number of interaction caches
	numCaches = 0;

	for ( iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next )
	{
		if ( iaCache->redundant )
		{
			continue;
		}

		surface = iaCache->surface;

		if ( !surface->shader->interactLight )
		{
			continue;
		}

		if ( surface->shader->isSky )
		{
			continue;
		}

		if ( surface->shader->noShadows )
		{
			continue;
		}

		if ( surface->shader->sort > SS_OPAQUE )
		{
			continue;
		}

		if ( surface->shader->isPortal )
		{
			continue;
		}

		if ( ShaderRequiresCPUDeforms( surface->shader ) )
		{
			continue;
		}

		numCaches++;
	}

	// build interaction caches list
	iaCachesSorted = (interactionCache_t**) ri.Hunk_AllocateTempMemory( numCaches * sizeof( iaCachesSorted[ 0 ] ) );

	numCaches = 0;

	for ( iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next )
	{
		if ( iaCache->redundant )
		{
			continue;
		}

		surface = iaCache->surface;

		if ( !surface->shader->interactLight )
		{
			continue;
		}

		if ( surface->shader->isSky )
		{
			continue;
		}

		if ( surface->shader->noShadows )
		{
			continue;
		}

		if ( surface->shader->sort > SS_OPAQUE )
		{
			continue;
		}

		if ( surface->shader->isPortal )
		{
			continue;
		}

		if ( ShaderRequiresCPUDeforms( surface->shader ) )
		{
			continue;
		}

		iaCachesSorted[ numCaches ] = iaCache;
		numCaches++;
	}

	// sort interaction caches by shader
	qsort( iaCachesSorted, numCaches, sizeof( *iaCachesSorted ), InteractionCacheCompare );

	// create a VBO for each shader
	for ( cubeSide = 0; cubeSide < 6; cubeSide++ )
	{
		shader = oldShader = NULL;
		oldAlphaTest = alphaTest = -1;

		for ( k = 0; k < numCaches; k++ )
		{
			iaCache = iaCachesSorted[ k ];
			shader = iaCache->surface->shader;
			alphaTest = shader->alphaTest;

			iaCache->mergedIntoVBO = qtrue;

			if ( alphaTest ? shader != oldShader : alphaTest != oldAlphaTest )
			{
				oldShader = shader;
				oldAlphaTest = alphaTest;

				// count vertices and indices
				numVerts = 0;
				numTriangles = 0;

				for ( l = k; l < numCaches; l++ )
				{
					iaCache2 = iaCachesSorted[ l ];

					surface = iaCache2->surface;

					if ( alphaTest )
					{
						if ( surface->shader != shader )
						{
							break;
						}
					}
					else
					{
						if ( surface->shader->alphaTest != alphaTest )
						{
							break;
						}
					}

					if ( !( iaCache2->cubeSideBits & ( 1 << cubeSide ) ) )
					{
						continue;
					}

					if ( *surface->data == SF_FACE )
					{
						srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface->data;

						if ( srf->numTriangles )
						{
							numTriangles +=
							  UpdateLightTriangles( s_worldData.verts, srf->numTriangles,
							                        s_worldData.triangles + srf->firstTriangle, surface->shader, light );
						}

						if ( srf->numVerts )
						{
							numVerts += srf->numVerts;
						}
					}
					else if ( *surface->data == SF_GRID )
					{
						srfGridMesh_t *srf = ( srfGridMesh_t * ) surface->data;

						if ( srf->numTriangles )
						{
							numTriangles +=
							  UpdateLightTriangles( s_worldData.verts, srf->numTriangles,
							                        s_worldData.triangles + srf->firstTriangle, surface->shader, light );
						}

						if ( srf->numVerts )
						{
							numVerts += srf->numVerts;
						}
					}
					else if ( *surface->data == SF_TRIANGLES )
					{
						srfTriangles_t *srf = ( srfTriangles_t * ) surface->data;

						if ( srf->numTriangles )
						{
							numTriangles +=
							  UpdateLightTriangles( s_worldData.verts, srf->numTriangles,
							                        s_worldData.triangles + srf->firstTriangle, surface->shader, light );
						}

						if ( srf->numVerts )
						{
							numVerts += srf->numVerts;
						}
					}
				}

				if ( !numVerts || !numTriangles )
				{
					continue;
				}

				// create surface
				vboSurf = (srfVBOMesh_t*) ri.Hunk_Alloc( sizeof( *vboSurf ), h_low );
				vboSurf->surfaceType = SF_VBO_MESH;
				vboSurf->numIndexes = numTriangles * 3;
				vboSurf->numVerts = numVerts;
				vboSurf->lightmapNum = -1;
				ZeroBounds( vboSurf->bounds[ 0 ], vboSurf->bounds[ 1 ] );

				// create arrays
				triangles = (srfTriangle_t*) ri.Hunk_AllocateTempMemory( numTriangles * sizeof( srfTriangle_t ) );
				numTriangles = 0;

				// build triangle indices
				for ( l = k; l < numCaches; l++ )
				{
					iaCache2 = iaCachesSorted[ l ];

					surface = iaCache2->surface;

					if ( alphaTest )
					{
						if ( surface->shader != shader )
						{
							break;
						}
					}
					else
					{
						if ( surface->shader->alphaTest != alphaTest )
						{
							break;
						}
					}

					if ( !( iaCache2->cubeSideBits & ( 1 << cubeSide ) ) )
					{
						continue;
					}

					if ( *surface->data == SF_FACE )
					{
						srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface->data;

						for ( i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++ )
						{
							if ( tri->facingLight )
							{
								for ( j = 0; j < 3; j++ )
								{
									triangles[ numTriangles ].indexes[ j ] = tri->indexes[ j ];
								}

								numTriangles++;
							}
						}
					}
					else if ( *surface->data == SF_GRID )
					{
						srfGridMesh_t *srf = ( srfGridMesh_t * ) surface->data;

						for ( i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++ )
						{
							if ( tri->facingLight )
							{
								for ( j = 0; j < 3; j++ )
								{
									triangles[ numTriangles ].indexes[ j ] = tri->indexes[ j ];
								}

								numTriangles++;
							}
						}
					}
					else if ( *surface->data == SF_TRIANGLES )
					{
						srfTriangles_t *srf = ( srfTriangles_t * ) surface->data;

						for ( i = 0, tri = s_worldData.triangles + srf->firstTriangle; i < srf->numTriangles; i++, tri++ )
						{
							if ( tri->facingLight )
							{
								for ( j = 0; j < 3; j++ )
								{
									triangles[ numTriangles ].indexes[ j ] = tri->indexes[ j ];
								}

								numTriangles++;
							}
						}
					}
				}

				vboSurf->vbo = s_worldData.vbo;
				vboSurf->ibo = R_CreateStaticIBO2( va( "staticShadowPyramidMesh_IBO %i", c_vboShadowSurfaces ), numTriangles, triangles );

				ri.Hunk_FreeTempMemory( triangles );

				// add everything needed to the light
				iaVBO = R_CreateInteractionVBO( light );
				iaVBO->cubeSideBits = ( 1 << cubeSide );
				iaVBO->shader = ( struct shader_s * ) shader;
				iaVBO->vboShadowMesh = ( struct srfVBOMesh_s * ) vboSurf;

				c_vboShadowSurfaces++;
			}
		}
	}

	ri.Hunk_FreeTempMemory( iaCachesSorted );
}

static void R_CalcInteractionCubeSideBits( trRefLight_t *light )
{
	interactionCache_t *iaCache;
	bspSurface_t       *surface;
	vec3_t             localBounds[ 2 ];

	if ( r_shadows->integer <= SHADOWING_BLOB )
	{
		return;
	}

	if ( !light->firstInteractionCache )
	{
		// this light has no interactions precached
		return;
	}

	if ( light->l.noShadows )
	{
		// actually noShadows lights are quite bad concerning this optimization
		return;
	}

	if ( light->l.rlType != RL_OMNI )
	{
		return;
	}

	{
		for ( iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next )
		{
			surface = iaCache->surface;

			if ( *surface->data == SF_FACE || *surface->data == SF_GRID || *surface->data == SF_TRIANGLES )
			{
				srfGeneric_t *gen;

				gen = ( srfGeneric_t * ) surface->data;

				VectorCopy( gen->bounds[ 0 ], localBounds[ 0 ] );
				VectorCopy( gen->bounds[ 1 ], localBounds[ 1 ] );
			}
			else
			{
				iaCache->cubeSideBits = CUBESIDE_CLIPALL;
				continue;
			}

			light->shadowLOD = 0; // important for R_CalcLightCubeSideBits
			iaCache->cubeSideBits = R_CalcLightCubeSideBits( light, localBounds );
		}
	}
}

/*
=============
R_PrecacheInteractions
=============
*/
void R_PrecacheInteractions( void )
{
	int          i;
	trRefLight_t *light;
	bspSurface_t *surface;
	int          startTime, endTime;

	startTime = ri.Milliseconds();

	// reset surfaces' viewCount
	s_lightCount = 0;

	for ( i = 0, surface = s_worldData.surfaces; i < s_worldData.numSurfaces; i++, surface++ )
	{
		surface->lightCount = -1;
	}

	Com_InitGrowList( &s_interactions, 100 );

	c_redundantInteractions = 0;
	c_vboWorldSurfaces = 0;
	c_vboLightSurfaces = 0;
	c_vboShadowSurfaces = 0;

	ri.Printf( PRINT_DEVELOPER, "...precaching %i lights\n", s_worldData.numLights );

	for ( i = 0; i < s_worldData.numLights; i++ )
	{
		light = &s_worldData.lights[ i ];

		if ( ( r_precomputedLighting->integer || r_vertexLighting->integer ) && !light->noRadiosity )
		{
			continue;
		}

		// set up light transform matrix
		MatrixSetupTransformFromQuat( light->transformMatrix, light->l.rotation, light->l.origin );

		// set up light origin for lighting and shadowing
		R_SetupLightOrigin( light );

		// set up model to light view matrix
		R_SetupLightView( light );

		// set up projection
		R_SetupLightProjection( light );

		// calc local bounds for culling
		R_SetupLightLocalBounds( light );

		// setup world bounds for intersection tests
		R_SetupLightWorldBounds( light );

		// setup frustum planes for intersection tests
		R_SetupLightFrustum( light );

		// setup interactions
		light->firstInteractionCache = NULL;
		light->lastInteractionCache = NULL;

		light->firstInteractionVBO = NULL;
		light->lastInteractionVBO = NULL;

		// perform culling and add all the potentially visible surfaces
		s_lightCount++;
		QueueInit( &light->leafs );
		R_RecursivePrecacheInteractionNode( s_worldData.nodes, light );

		// create a static VBO surface for each light geometry batch
		R_CreateVBOLightMeshes( light );

		// create a static VBO surface for each shadow geometry batch
		R_CreateVBOShadowMeshes( light );

		// calculate pyramid bits for each interaction in omni-directional lights
		R_CalcInteractionCubeSideBits( light );

		// create a static VBO surface for each light geometry batch inside a cubemap pyramid
		R_CreateVBOShadowCubeMeshes( light );
	}

	// move interactions grow list to hunk
	s_worldData.numInteractions = s_interactions.currentElements;
	s_worldData.interactions = (interactionCache_t**) ri.Hunk_Alloc( s_worldData.numInteractions * sizeof( *s_worldData.interactions ), h_low );

	for ( i = 0; i < s_worldData.numInteractions; i++ )
	{
		s_worldData.interactions[ i ] = ( interactionCache_t * ) Com_GrowListElement( &s_interactions, i );
	}

	Com_DestroyGrowList( &s_interactions );

	ri.Printf( PRINT_DEVELOPER, "%i interactions precached\n", s_worldData.numInteractions );
	ri.Printf( PRINT_DEVELOPER, "%i interactions were hidden in shadows\n", c_redundantInteractions );

	if ( r_shadows->integer >= SHADOWING_ESM16 )
	{
		// only interesting for omni-directional shadow mapping
		ri.Printf( PRINT_DEVELOPER, "%i omni pyramid tests\n", tr.pc.c_pyramidTests );
		ri.Printf( PRINT_DEVELOPER, "%i omni pyramid surfaces visible\n", tr.pc.c_pyramid_cull_ent_in );
		ri.Printf( PRINT_DEVELOPER, "%i omni pyramid surfaces clipped\n", tr.pc.c_pyramid_cull_ent_clip );
		ri.Printf( PRINT_DEVELOPER, "%i omni pyramid surfaces culled\n", tr.pc.c_pyramid_cull_ent_out );
	}

	endTime = ri.Milliseconds();

	ri.Printf( PRINT_DEVELOPER, "lights precaching time = %5.2f seconds\n", ( endTime - startTime ) / 1000.0 );
}

#define HASHTABLE_SIZE 7919 // 32749 // 2039    /* prime, use % */
#define HASH_USE_EPSILON

#ifdef HASH_USE_EPSILON
#define HASH_XYZ_EPSILON                 0.01f
#define HASH_XYZ_EPSILONSPACE_MULTIPLIER 1.f / HASH_XYZ_EPSILON
#endif

unsigned int VertexCoordGenerateHash( const vec3_t xyz )
{
	unsigned int hash = 0;

#ifndef HASH_USE_EPSILON
	hash += ~( * ( ( unsigned int * ) &xyz[ 0 ] ) << 15 );
	hash ^= ( * ( ( unsigned int * ) &xyz[ 0 ] ) >> 10 );
	hash += ( * ( ( unsigned int * ) &xyz[ 1 ] ) << 3 );
	hash ^= ( * ( ( unsigned int * ) &xyz[ 1 ] ) >> 6 );
	hash += ~( * ( ( unsigned int * ) &xyz[ 2 ] ) << 11 );
	hash ^= ( * ( ( unsigned int * ) &xyz[ 2 ] ) >> 16 );
#else
	vec3_t xyz_epsilonspace;

	VectorScale( xyz, HASH_XYZ_EPSILONSPACE_MULTIPLIER, xyz_epsilonspace );
	xyz_epsilonspace[ 0 ] = ( double ) floor( xyz_epsilonspace[ 0 ] );
	xyz_epsilonspace[ 1 ] = ( double ) floor( xyz_epsilonspace[ 1 ] );
	xyz_epsilonspace[ 2 ] = ( double ) floor( xyz_epsilonspace[ 2 ] );

	hash += ~( * ( ( unsigned int * ) &xyz_epsilonspace[ 0 ] ) << 15 );
	hash ^= ( * ( ( unsigned int * ) &xyz_epsilonspace[ 0 ] ) >> 10 );
	hash += ( * ( ( unsigned int * ) &xyz_epsilonspace[ 1 ] ) << 3 );
	hash ^= ( * ( ( unsigned int * ) &xyz_epsilonspace[ 1 ] ) >> 6 );
	hash += ~( * ( ( unsigned int * ) &xyz_epsilonspace[ 2 ] ) << 11 );
	hash ^= ( * ( ( unsigned int * ) &xyz_epsilonspace[ 2 ] ) >> 16 );

#endif

	hash = hash % ( HASHTABLE_SIZE );
	return hash;
}

vertexHash_t **NewVertexHashTable( void )
{
	vertexHash_t **hashTable = (vertexHash_t**) Com_Allocate( HASHTABLE_SIZE * sizeof( vertexHash_t * ) );

	Com_Memset( hashTable, 0, HASHTABLE_SIZE * sizeof( vertexHash_t * ) );

	return hashTable;
}

void FreeVertexHashTable( vertexHash_t **hashTable )
{
	int          i;
	vertexHash_t *vertexHash;
	vertexHash_t *nextVertexHash;

	if ( hashTable == NULL )
	{
		return;
	}

	for ( i = 0; i < HASHTABLE_SIZE; i++ )
	{
		if ( hashTable[ i ] )
		{
			nextVertexHash = NULL;

			for ( vertexHash = hashTable[ i ]; vertexHash; vertexHash = nextVertexHash )
			{
				nextVertexHash = vertexHash->next;

				Com_Dealloc( vertexHash );
			}
		}
	}

	Com_Dealloc( hashTable );
}

vertexHash_t *FindVertexInHashTable( vertexHash_t **hashTable, const vec3_t xyz, float distance )
{
	unsigned int hash;
	vertexHash_t *vertexHash;

	if ( hashTable == NULL || xyz == NULL )
	{
		return NULL;
	}

	hash = VertexCoordGenerateHash( xyz );

	for ( vertexHash = hashTable[ hash ]; vertexHash; vertexHash = vertexHash->next )
	{
#ifndef HASH_USE_EPSILON

		if ( ( vertexHash->vcd.xyz[ 0 ] != xyz[ 0 ] || vertexHash->vcd.xyz[ 1 ] != xyz[ 1 ] ||
		       vertexHash->vcd.xyz[ 2 ] != xyz[ 2 ] ) )
		{
			continue;
		}

#elif 1

		if ( Distance( xyz, vertexHash->xyz ) > distance )
		{
			continue;
		}

#else

		if ( ( fabs( xyz[ 0 ] - vertexHash->vcd.xyz[ 0 ] ) ) > HASH_XYZ_EPSILON ||
		     ( fabs( xyz[ 1 ] - vertexHash->vcd.xyz[ 1 ] ) ) > HASH_XYZ_EPSILON ||
		     ( fabs( xyz[ 2 ] - vertexHash->vcd.xyz[ 2 ] ) ) > HASH_XYZ_EPSILON )
		{
			continue;
		}

#endif
		return vertexHash;
	}

	return NULL;
}

vertexHash_t *AddVertexToHashTable( vertexHash_t **hashTable, vec3_t xyz, void *data )
{
	unsigned int hash;
	vertexHash_t *vertexHash;

	if ( hashTable == NULL || xyz == NULL )
	{
		return NULL;
	}

	vertexHash = (vertexHash_t*) Com_Allocate( sizeof( vertexHash_t ) );

	if ( !vertexHash )
	{
		return NULL;
	}

	hash = VertexCoordGenerateHash( xyz );

	VectorCopy( xyz, vertexHash->xyz );
	vertexHash->data = data;

	// link into table
	vertexHash->next = hashTable[ hash ];
	hashTable[ hash ] = vertexHash;

	return vertexHash;
}

void GL_BindNearestCubeMap( const vec3_t xyz )
{
	float          distance, maxDistance;
	cubemapProbe_t *cubeProbe;
	unsigned int   hash;
	vertexHash_t   *vertexHash;

	tr.autoCubeImage = tr.whiteCubeImage;

	if ( !r_reflectionMapping->integer )
	{
		return;
	}

	if ( tr.cubeHashTable == NULL || xyz == NULL )
	{
		return;
	}

	maxDistance = 9999999.0f;

	hash = VertexCoordGenerateHash( xyz );

	for ( vertexHash = tr.cubeHashTable[ hash ]; vertexHash; vertexHash = vertexHash->next )
	{
		cubeProbe = (cubemapProbe_t*) vertexHash->data;

		distance = Distance( cubeProbe->origin, xyz );

		if ( distance < maxDistance )
		{
			tr.autoCubeImage = cubeProbe->cubemap;
			maxDistance = distance;
		}
	}

	GL_Bind( tr.autoCubeImage );
}

void R_FindTwoNearestCubeMaps( const vec3_t position, cubemapProbe_t **cubeProbeNearest, cubemapProbe_t **cubeProbeSecondNearest )
{
	int            j;
	float          distance, maxDistance, maxDistance2;
	cubemapProbe_t *cubeProbe;
	unsigned int   hash;
	vertexHash_t   *vertexHash;

	GLimp_LogComment( "--- R_FindTwoNearestCubeMaps ---\n" );

	*cubeProbeNearest = NULL;
	*cubeProbeSecondNearest = NULL;

	if ( tr.cubeHashTable == NULL || position == NULL )
	{
		return;
	}

	hash = VertexCoordGenerateHash( position );
	maxDistance = maxDistance2 = 9999999.0f;

	for ( j = 0, vertexHash = tr.cubeHashTable[ hash ]; vertexHash; vertexHash = vertexHash->next, j++ )
	{
		cubeProbe = (cubemapProbe_t*) vertexHash->data;
		distance = Distance( cubeProbe->origin, position );

		if ( distance < maxDistance )
		{
			*cubeProbeSecondNearest = *cubeProbeNearest;
			maxDistance2 = maxDistance;

			*cubeProbeNearest = cubeProbe;
			maxDistance = distance;
		}
		else if ( distance < maxDistance2 && distance > maxDistance )
		{
			*cubeProbeSecondNearest = cubeProbe;
			maxDistance2 = distance;
		}
	}
}

void R_BuildCubeMaps( void )
{
	int            i, j;
	int            ii, jj;
	refdef_t       rf;
	qboolean       flipx;
	qboolean       flipy;
	int            x, y, xy, xy2;

	cubemapProbe_t *cubeProbe;
	byte           temp[ REF_CUBEMAP_SIZE * REF_CUBEMAP_SIZE * 4 ];
	byte           *dest;

	int    startTime, endTime;
	size_t tics = 0;
	size_t nextTicCount = 0;

	startTime = ri.Milliseconds();

	memset( &rf, 0, sizeof( refdef_t ) );

	for ( i = 0; i < 6; i++ )
	{
		tr.cubeTemp[ i ] = (byte*) ri.Z_Malloc( REF_CUBEMAP_SIZE * REF_CUBEMAP_SIZE * 4 );
	}

	// calculate origins for our probes
	Com_InitGrowList( &tr.cubeProbes, 4000 );
	tr.cubeHashTable = NewVertexHashTable();

	{
		bspNode_t *node;

		for ( i = 0; i < tr.world->numnodes; i++ )
		{
			node = &tr.world->nodes[ i ];

			// check to see if this is a shit location
			if ( node->contents == CONTENTS_NODE )
			{
				continue;
			}

			if ( node->area == -1 )
			{
				// location is in the void
				continue;
			}

			if ( FindVertexInHashTable( tr.cubeHashTable, node->origin, 256 ) == NULL )
			{
				cubeProbe = (cubemapProbe_t*) ri.Hunk_Alloc( sizeof( *cubeProbe ), h_high );
				Com_AddToGrowList( &tr.cubeProbes, cubeProbe );

				VectorCopy( node->origin, cubeProbe->origin );

				AddVertexToHashTable( tr.cubeHashTable, cubeProbe->origin, cubeProbe );
			}
		}
	}

	// if we can't find one, fake one
	if ( tr.cubeProbes.currentElements == 0 )
	{
		cubeProbe = (cubemapProbe_t*) ri.Hunk_Alloc( sizeof( *cubeProbe ), h_low );
		Com_AddToGrowList( &tr.cubeProbes, cubeProbe );

		VectorClear( cubeProbe->origin );
	}

	ri.Printf( PRINT_ALL, "...pre-rendering %d cubemaps\n", tr.cubeProbes.currentElements );
	ri.Cvar_Set( "viewlog", "1" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );

	for ( j = 0; j < tr.cubeProbes.currentElements; j++ )
	{
		cubeProbe = (cubemapProbe_t*) Com_GrowListElement( &tr.cubeProbes, j );

		//ri.Printf(PRINT_ALL, "rendering cubemap at (%i %i %i)\n", (int)cubeProbe->origin[0], (int)cubeProbe->origin[1],
		//      (int)cubeProbe->origin[2]);

		if ( ( j + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( j + 1 ) / tr.cubeProbes.currentElements ) * 50.0 );

			do
			{
				ri.Printf( PRINT_ALL, "*" );
				ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
			}
			while ( ++tics < ticsNeeded );

			nextTicCount = ( size_t )( ( tics / 50.0 ) * tr.cubeProbes.currentElements );

			if ( ( j + 1 ) == tr.cubeProbes.currentElements )
			{
				if ( tics < 51 )
				{
					ri.Printf( PRINT_ALL, "*" );
				}

				ri.Printf( PRINT_ALL, "\n" );
			}
		}

		VectorCopy( cubeProbe->origin, rf.vieworg );

		AxisClear( rf.viewaxis );

		rf.fov_x = 90;
		rf.fov_y = 90;
		rf.x = 0;
		rf.y = 0;
		rf.width = REF_CUBEMAP_SIZE;
		rf.height = REF_CUBEMAP_SIZE;
		rf.time = 0;

		rf.rdflags = RDF_NOCUBEMAP | RDF_NOBLOOM;

		for ( i = 0; i < 6; i++ )
		{
			flipx = qfalse;
			flipy = qfalse;

			switch ( i )
			{
				case 0:
					{
						//X+
						rf.viewaxis[ 0 ][ 0 ] = 1;
						rf.viewaxis[ 0 ][ 1 ] = 0;
						rf.viewaxis[ 0 ][ 2 ] = 0;

						rf.viewaxis[ 1 ][ 0 ] = 0;
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = 1;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						//flipx=qtrue;
						break;
					}

				case 1:
					{
						//X-
						rf.viewaxis[ 0 ][ 0 ] = -1;
						rf.viewaxis[ 0 ][ 1 ] = 0;
						rf.viewaxis[ 0 ][ 2 ] = 0;

						rf.viewaxis[ 1 ][ 0 ] = 0;
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = -1;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						//flipx=qtrue;
						break;
					}

				case 2:
					{
						//Y+
						rf.viewaxis[ 0 ][ 0 ] = 0;
						rf.viewaxis[ 0 ][ 1 ] = 1;
						rf.viewaxis[ 0 ][ 2 ] = 0;

						rf.viewaxis[ 1 ][ 0 ] = -1;
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = 0;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						//flipx=qtrue;
						break;
					}

				case 3:
					{
						//Y-
						rf.viewaxis[ 0 ][ 0 ] = 0;
						rf.viewaxis[ 0 ][ 1 ] = -1;
						rf.viewaxis[ 0 ][ 2 ] = 0;

						rf.viewaxis[ 1 ][ 0 ] = -1; //-1
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = 0;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						//flipx=qtrue;
						break;
					}

				case 4:
					{
						//Z+
						rf.viewaxis[ 0 ][ 0 ] = 0;
						rf.viewaxis[ 0 ][ 1 ] = 0;
						rf.viewaxis[ 0 ][ 2 ] = 1;

						rf.viewaxis[ 1 ][ 0 ] = -1;
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = 0;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						//  flipx=qtrue;
						break;
					}

				case 5:
					{
						//Z-
						rf.viewaxis[ 0 ][ 0 ] = 0;
						rf.viewaxis[ 0 ][ 1 ] = 0;
						rf.viewaxis[ 0 ][ 2 ] = -1;

						rf.viewaxis[ 1 ][ 0 ] = 1;
						rf.viewaxis[ 1 ][ 1 ] = 0;
						rf.viewaxis[ 1 ][ 2 ] = 0;

						CrossProduct( rf.viewaxis[ 0 ], rf.viewaxis[ 1 ], rf.viewaxis[ 2 ] );
						//flipx=qtrue;
						break;
					}
			}

			tr.refdef.pixelTarget = tr.cubeTemp[ i ];
			memset( tr.cubeTemp[ i ], 255, REF_CUBEMAP_SIZE * REF_CUBEMAP_SIZE * 4 );
			tr.refdef.pixelTargetWidth = REF_CUBEMAP_SIZE;
			tr.refdef.pixelTargetHeight = REF_CUBEMAP_SIZE;

			RE_BeginFrame( STEREO_CENTER );
			RE_RenderScene( &rf );
			RE_EndFrame( &ii, &jj );

			if ( flipx )
			{
				dest = tr.cubeTemp[ i ];
				memcpy( temp, dest, REF_CUBEMAP_SIZE * REF_CUBEMAP_SIZE * 4 );

				for ( y = 0; y < REF_CUBEMAP_SIZE; y++ )
				{
					for ( x = 0; x < REF_CUBEMAP_SIZE; x++ )
					{
						xy = ( ( y * REF_CUBEMAP_SIZE ) + x ) * 4;
						xy2 = ( ( y * REF_CUBEMAP_SIZE ) + ( ( REF_CUBEMAP_SIZE - 1 ) - x ) ) * 4;
						dest[ xy2 + 0 ] = temp[ xy + 0 ];
						dest[ xy2 + 1 ] = temp[ xy + 1 ];
						dest[ xy2 + 2 ] = temp[ xy + 2 ];
						dest[ xy2 + 3 ] = temp[ xy + 3 ];
					}
				}
			}

			if ( flipy )
			{
				dest = tr.cubeTemp[ i ];
				memcpy( temp, dest, REF_CUBEMAP_SIZE * REF_CUBEMAP_SIZE * 4 );

				for ( y = 0; y < REF_CUBEMAP_SIZE; y++ )
				{
					for ( x = 0; x < REF_CUBEMAP_SIZE; x++ )
					{
						xy = ( ( y * REF_CUBEMAP_SIZE ) + x ) * 4;
						xy2 = ( ( ( ( REF_CUBEMAP_SIZE - 1 ) - y ) * REF_CUBEMAP_SIZE ) + x ) * 4;
						dest[ xy2 + 0 ] = temp[ xy + 0 ];
						dest[ xy2 + 1 ] = temp[ xy + 1 ];
						dest[ xy2 + 2 ] = temp[ xy + 2 ];
						dest[ xy2 + 3 ] = temp[ xy + 3 ];
					}
				}
			}

			// encode the pixel intensity into the alpha channel, saves work in the shader
			if ( qtrue )
			{
				byte r, g, b, best;

				dest = tr.cubeTemp[ i ];

				for ( y = 0; y < REF_CUBEMAP_SIZE; y++ )
				{
					for ( x = 0; x < REF_CUBEMAP_SIZE; x++ )
					{
						xy = ( ( y * REF_CUBEMAP_SIZE ) + x ) * 4;

						r = dest[ xy + 0 ];
						g = dest[ xy + 1 ];
						b = dest[ xy + 2 ];

						if ( ( r > g ) && ( r > b ) )
						{
							best = r;
						}
						else if ( ( g > r ) && ( g > b ) )
						{
							best = g;
						}
						else
						{
							best = b;
						}

						dest[ xy + 3 ] = best;
					}
				}
			}
		}

		// build the cubemap
		cubeProbe->cubemap = R_AllocImage( va( "_autoCube%d", j ), qfalse );

		if ( !cubeProbe->cubemap )
		{
			return;
		}

		cubeProbe->cubemap->type = GL_TEXTURE_CUBE_MAP;

		cubeProbe->cubemap->width = REF_CUBEMAP_SIZE;
		cubeProbe->cubemap->height = REF_CUBEMAP_SIZE;

		cubeProbe->cubemap->bits = IF_NOPICMIP;
		cubeProbe->cubemap->filterType = FT_LINEAR;
		cubeProbe->cubemap->wrapType = WT_EDGE_CLAMP;

		R_UploadImage( ( const byte ** ) tr.cubeTemp, 6, 1, cubeProbe->cubemap );
	}

	ri.Printf( PRINT_ALL, "\n" );

	// turn pixel targets off
	tr.refdef.pixelTarget = NULL;

	// assign the surfs a cubemap
	endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "cubemap probes pre-rendering time of %i cubes = %5.2f seconds\n", tr.cubeProbes.currentElements,
	           ( endTime - startTime ) / 1000.0 );
}

/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
void RE_LoadWorldMap( const char *name )
{
	int       i;
	dheader_t *header;
	byte      *buffer;
	byte      *startMarker;

	if ( tr.worldMapLoaded )
	{
		ri.Error( ERR_DROP, "ERROR: attempted to redundantly load world map" );
	}

	ri.Printf( PRINT_DEVELOPER, "----- RE_LoadWorldMap( %s ) -----\n", name );

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[ 0 ] = 0.45f;
	tr.sunDirection[ 1 ] = 0.3f;
	tr.sunDirection[ 2 ] = 0.9f;

	VectorNormalize( tr.sunDirection );

	// inalidate fogs (likely to be re-initialized to new values by the current map)
	// TODO:(SA)this is sort of silly.  I'm going to do a general cleanup on fog stuff
	//          now that I can see how it's been used.  (functionality can narrow since
	//          it's not used as much as it's designed for.)

	RE_SetFog( FOG_SKY, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_PORTALVIEW, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_HUD, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_MAP, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_CURRENT, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_TARGET, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_WATER, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_SERVER, 0, 0, 0, 0, 0, 0 );

	tr.glfogNum = (glfogType_t) 0;

	VectorCopy( colorMdGrey, tr.fogColor );
	tr.fogDensity = 0;

	// set default ambient color
	tr.worldEntity.ambientLight[ 0 ] = r_forceAmbient->value;
	tr.worldEntity.ambientLight[ 1 ] = r_forceAmbient->value;
	tr.worldEntity.ambientLight[ 2 ] = r_forceAmbient->value;

	tr.worldMapLoaded = qtrue;

	// load it
	ri.FS_ReadFile( name, ( void ** ) &buffer );

	if ( !buffer )
	{
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s not found", name );
	}

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	// tr.worldDeluxeMapping will be set by R_LoadEntities()
	tr.worldDeluxeMapping = qfalse;
	tr.worldHDR_RGBE = qfalse;

	Com_Memset( &s_worldData, 0, sizeof( s_worldData ) );
	Q_strncpyz( s_worldData.name, name, sizeof( s_worldData.name ) );

	Q_strncpyz( s_worldData.baseName, COM_SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
	COM_StripExtension3( s_worldData.baseName, s_worldData.baseName, sizeof( s_worldData.baseName ) );

	startMarker = (byte*) ri.Hunk_Alloc( 0, h_low );

	header = ( dheader_t * ) buffer;
	fileBase = ( byte * ) header;

	i = LittleLong( header->version );

	if ( i != BSP_VERSION && i != BSP_VERSION_Q3 )
	{
		ri.FS_FreeFile( buffer );
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i for ET or %i for Q3)",
		          name, i, BSP_VERSION, BSP_VERSION_Q3 );
	}

	// swap all the lumps
	for ( i = 0; i < sizeof( dheader_t ) / 4; i++ )
	{
		( ( int * ) header ) [ i ] = LittleLong( ( ( int * ) header ) [ i ] );
	}

	// load into heap
	R_LoadEntities( &header->lumps[ LUMP_ENTITIES ] );

	R_LoadShaders( &header->lumps[ LUMP_SHADERS ] );

	R_LoadLightmaps( &header->lumps[ LUMP_LIGHTMAPS ], name );

	R_LoadPlanes( &header->lumps[ LUMP_PLANES ] );

	R_LoadSurfaces( &header->lumps[ LUMP_SURFACES ], &header->lumps[ LUMP_DRAWVERTS ], &header->lumps[ LUMP_DRAWINDEXES ] );

	R_LoadMarksurfaces( &header->lumps[ LUMP_LEAFSURFACES ] );

	R_LoadNodesAndLeafs( &header->lumps[ LUMP_NODES ], &header->lumps[ LUMP_LEAFS ] );

	R_LoadSubmodels( &header->lumps[ LUMP_MODELS ] );

	// moved fog lump loading here, so fogs can be tagged with a model num
	R_LoadFogs( &header->lumps[ LUMP_FOGS ], &header->lumps[ LUMP_BRUSHES ], &header->lumps[ LUMP_BRUSHSIDES ] );

	R_LoadVisibility( &header->lumps[ LUMP_VISIBILITY ] );

	R_LoadLightGrid( &header->lumps[ LUMP_LIGHTGRID ] );

	// create a static vbo for the world
	R_CreateWorldVBO();
	R_CreateClusters();

	// we precache interactions between lights and surfaces
	// to reduce the polygon count
	R_PrecacheInteractions();

	s_worldData.dataSize = ( byte * ) ri.Hunk_Alloc( 0, h_low ) - startMarker;

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

	// reset fog to world fog (if present)
	RE_SetFog( FOG_CMD_SWITCHFOG, FOG_MAP, 20, 0, 0, 0, 0 );

	//----(SA)  set the sun shader if there is one
	if ( tr.sunShaderName )
	{
		tr.sunShader = R_FindShader( tr.sunShaderName, SHADER_3D_STATIC, RSF_DEFAULT );
	}

	//----(SA)  end

	// build cubemaps after the necessary vbo stuff is done
	//R_BuildCubeMaps();

	// never move this to RE_BeginFrame because we need it to set it here for the first frame
	// but we need the information across 2 frames
	ClearLink( &tr.traversalStack );
	ClearLink( &tr.occlusionQueryQueue );
	ClearLink( &tr.occlusionQueryList );

	ri.FS_FreeFile( buffer );
}
