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
//static int      c_redundantVertexes;
static int        c_vboWorldSurfaces;
static int        c_vboLightSurfaces;
static int        c_vboShadowSurfaces;

//===============================================================================

void HSVtoRGB( float h, float s, float v, float rgb[ 3 ] )
{
	int   i;
	float f;
	float p, q, t;

	h *= 5;

	i = floor( h );
	f = h - i;

	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch ( i )
	{
		case 0:
			rgb[ 0 ] = v;
			rgb[ 1 ] = t;
			rgb[ 2 ] = p;
			break;

		case 1:
			rgb[ 0 ] = q;
			rgb[ 1 ] = v;
			rgb[ 2 ] = p;
			break;

		case 2:
			rgb[ 0 ] = p;
			rgb[ 1 ] = v;
			rgb[ 2 ] = t;
			break;

		case 3:
			rgb[ 0 ] = p;
			rgb[ 1 ] = q;
			rgb[ 2 ] = v;
			break;

		case 4:
			rgb[ 0 ] = t;
			rgb[ 1 ] = p;
			rgb[ 2 ] = v;
			break;

		case 5:
			rgb[ 0 ] = v;
			rgb[ 1 ] = p;
			rgb[ 2 ] = q;
			break;
	}
}

/*
===============
R_ColorShiftLightingBytes
===============
*/
#if defined( COMPAT_Q3A ) || defined( COMPAT_ET )
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

#endif

/*
===============
R_ColorShiftLightingFloats
===============
*/
#if 1 //defined(COMPAT_Q3A)
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

#endif

/*
===============
R_HDRTonemapLightingColors
===============
*/
#if 0
static void R_HDRTonemapLightingColors( const vec4_t in, vec4_t out, qboolean applyGamma )
{
#if 0 //!defined(USE_HDR_LIGHTMAPS)
	R_ColorShiftLightingFloats( in, out );
#else
	int i;
	//float           scaledLuminance;
	//float           finalLuminance;
	//const vec3_t    LUMINANCE_VECTOR = { 0.2125f, 0.7154f, 0.0721f };
	vec4_t sample;

	if ( !tr.worldHDR_RGBE )
	{
		R_ColorShiftLightingFloats( in, out );
		return;
	}

#if 0
	scaledLuminance = r_hdrLightmapExposure->value * DotProduct( in, LUMINANCE_VECTOR );

#if 0
	finalLuminance = scaledLuminance / ( scaledLuminance + 1.0 );
#else
	// exponential tone mapping
	finalLuminance = 1.0 - exp( -scaledLuminance );
#endif

	VectorScale( sample, finalLuminance, sample );
	sample[ 3 ] = Q_min( 1.0f, sample[ 3 ] );

	if ( !r_hdrRendering->integer || !r_hdrLightmap->integer || !glConfig2.framebufferObjectAvailable ||
	     !glConfig2.textureFloatAvailable || !glConfig2.framebufferBlitAvailable )
	{
		float max;

		// clamp with color normalization
		NormalizeColor( sample, out );
		out[ 3 ] = Q_min( 1.0f, sample[ 3 ] );
	}
	else
	{
		Vector4Copy( sample, out );
	}

#else

	if ( !r_hdrRendering->integer || !r_hdrLightmap->integer || !glConfig2.framebufferObjectAvailable ||
	     !glConfig2.textureFloatAvailable || !glConfig2.framebufferBlitAvailable )
	{
		float max;

		Vector4Copy( in, sample );

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
			VectorScale( sample, ( 255.0f / max ), out );
		}

		VectorScale( out, ( 1.0f / 255.0f ), out );

		out[ 3 ] = Q_min( 1.0f, sample[ 3 ] );
	}
	else
	{
		Vector4Scale( in, 1.0f / 255.0f, sample );

		if ( applyGamma )
		{
			for ( i = 0; i < 3; i++ )
			{
				sample[ i ] = pow( sample[ i ], 1.0f / r_hdrLightmapGamma->value );
			}
		}

#if 0
		scaledLuminance = r_hdrLightmapExposure->value * DotProduct( in, LUMINANCE_VECTOR );

#if 0
		finalLuminance = scaledLuminance / ( scaledLuminance + 1.0 );
#else
		// exponential tone mapping
		finalLuminance = 1.0 - exp( -scaledLuminance );
#endif

		VectorScale( sample, finalLuminance, out );
#else
		VectorCopy( sample, out );
#endif

		out[ 3 ] = Q_min( 1.0f, sample[ 3 ] );
	}

#endif
#endif
}

#endif

/*
===============
R_ProcessLightmap

        returns maxIntensity
===============
*/
#if defined( COMPAT_Q3A ) || defined( COMPAT_ET )
float R_ProcessLightmap( byte **pic, int in_padding, int width, int height, byte **pic_out )
{
	int   j;
	float maxIntensity = 0;
//	double          sumIntensity = 0;

	/*
	if(r_lightmap->integer > 1)
	{
	        // color code by intensity as development tool (FIXME: check range)
	        for(j = 0; j < width * height; j++)
	        {
	                float           r = (*pic)[j * in_padding + 0];
	                float           g = (*pic)[j * in_padding + 1];
	                float           b = (*pic)[j * in_padding + 2];
	                float           intensity;
	                float           out[3];

	                intensity = 0.33f * r + 0.685f * g + 0.063f * b;

	                if(intensity > 255)
	                {
	                        intensity = 1.0f;
	                }
	                else
	                {
	                        intensity /= 255.0f;
	                }

	                if(intensity > maxIntensity)
	                {
	                        maxIntensity = intensity;
	                }

	                HSVtoRGB(intensity, 1.00, 0.50, out);

	                if(r_lightmap->integer == 3)
	                {
	                        // Arnout: artists wanted the colours to be inversed
	                        (*pic_out)[j * 4 + 0] = out[2] * 255;
	                        (*pic_out)[j * 4 + 1] = out[1] * 255;
	                        (*pic_out)[j * 4 + 2] = out[0] * 255;
	                }
	                else
	                {
	                        (*pic_out)[j * 4 + 0] = out[0] * 255;
	                        (*pic_out)[j * 4 + 1] = out[1] * 255;
	                        (*pic_out)[j * 4 + 2] = out[2] * 255;
	                }
	                (*pic_out)[j * 4 + 3] = 255;

	                sumIntensity += intensity;
	        }
	}
	else
	*/
	{
		for ( j = 0; j < width * height; j++ )
		{
			R_ColorShiftLightingBytes( & ( *pic ) [ j * in_padding ], & ( *pic_out ) [ j * 4 ] );
			( *pic_out ) [ j * 4 + 3 ] = 255;
		}
	}

	return maxIntensity;
}

#endif

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
		//f = ldexp(1.0, rgbe[3] - 128) / 10.0;
		e = ( rgbe[ 3 ] - 128 ) / 4.0f;

		// RB: exp2 not defined by MSVC
		//f = exp2(e);
		f = pow( 2, e );

		//decoded = rgbe.rgb * exp2(fExp);
		*red = ( rgbe[ 0 ] / 255.0f ) * f;
		*green = ( rgbe[ 1 ] / 255.0f ) * f;
		*blue = ( rgbe[ 2 ] / 255.0f ) * f;
	}
	else
	{
		*red = *green = *blue = 0.0;
	}
}

void LoadRGBEToFloats( const char *name, float **pic, int *width, int *height, qboolean doGamma, qboolean toneMap,
                       qboolean compensate )
{
	int      i, j;
	byte     *buf_p;
	byte     *buffer;
	float    *floatbuf;
//	int             len;
	char     *token;
	int      w, h, c;
	qboolean formatFound;
	//unsigned char   rgbe[4];
	//float           red;
	//float           green;
	//float           blue;
	//float           max;
	//float           inv, dif;
	float        exposure = 1.6;
	//float           exposureGain = 1.0;
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
			//ri.Printf(PRINT_ALL, "LoadRGBE: FORMAT found\n");

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
			//buf_p++;
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

	*pic = Com_Allocate( w * h * 3 * sizeof( float ) );
	floatbuf = *pic;

	for ( i = 0; i < ( w * h ); i++ )
	{
#if 0
		rgbe[ 0 ] = *buf_p++;
		rgbe[ 1 ] = *buf_p++;
		rgbe[ 2 ] = *buf_p++;
		rgbe[ 3 ] = *buf_p++;

		rgbe2float( &red, &green, &blue, rgbe );

		*floatbuf++ = red;
		*floatbuf++ = green;
		*floatbuf++ = blue;
#else

		for ( j = 0; j < 3; j++ )
		{
			sample.b[ 0 ] = *buf_p++;
			sample.b[ 1 ] = *buf_p++;
			sample.b[ 2 ] = *buf_p++;
			sample.b[ 3 ] = *buf_p++;

			*floatbuf++ = sample.f / 255.0f; // FIXME XMap2's output is 255 times too high
		}

#endif
	}

	// LOADING DONE
	if ( doGamma )
	{
		floatbuf = *pic;
		gamma = 1.0f / r_hdrLightmapGamma->value;

		for ( i = 0; i < ( w * h ); i++ )
		{
			for ( j = 0; j < 3; j++ )
			{
				//*floatbuf = pow(*floatbuf / 255.0f, gamma) * 255.0f;
				*floatbuf = pow( *floatbuf, gamma );
				floatbuf++;
			}
		}
	}

	if ( toneMap )
	{
		// calculate the average and maximum luminance
		sum = 0.0f;
		maxLuminance = 0.0f;
		floatbuf = *pic;

		for ( i = 0; i < ( w * h ); i++ )
		{
			for ( j = 0; j < 3; j++ )
			{
				sampleVector[ j ] = *floatbuf++;
			}

			luminance = DotProduct( sampleVector, LUMINANCE_VECTOR ) + 0.0001f;

			if ( luminance > maxLuminance )
			{
				maxLuminance = luminance;
			}

			sum += log( luminance );
		}

		sum /= ( float )( w * h );
		avgLuminance = exp( sum );

		// post process buffer with tone mapping
		floatbuf = *pic;

		for ( i = 0; i < ( w * h ); i++ )
		{
			for ( j = 0; j < 3; j++ )
			{
				sampleVector[ j ] = *floatbuf++;
			}

			if ( r_hdrLightmapExposure->value <= 0 )
			{
				exposure = ( r_hdrKey->value / avgLuminance );
			}
			else
			{
				exposure = r_hdrLightmapExposure->value;
			}

			//

			scaledLuminance = exposure * DotProduct( sampleVector, LUMINANCE_VECTOR );
#if 0
			finalLuminance = scaledLuminance / ( scaledLuminance + 1.0 );
#elif 0
			finalLuminance = ( scaledLuminance * ( scaledLuminance / maxLuminance + 1.0 ) ) / ( scaledLuminance + 1.0 );
#elif 0
			finalLuminance =
			  ( scaledLuminance * ( ( scaledLuminance / ( maxLuminance * maxLuminance ) ) + 1.0 ) ) / ( scaledLuminance + 1.0 );
#else
			// exponential tone mapping
			finalLuminance = 1.0 - exp( -scaledLuminance );
#endif

			//VectorScale(sampleVector, scaledLuminance * (scaledLuminance / maxLuminance + 1.0) / (scaledLuminance + 1.0), sampleVector);
			//VectorScale(sampleVector, scaledLuminance / (scaledLuminance + 1.0), sampleVector);

			VectorScale( sampleVector, finalLuminance, sampleVector );

			floatbuf -= 3;

			for ( j = 0; j < 3; j++ )
			{
				*floatbuf++ = sampleVector[ j ];
			}
		}
	}

	if ( compensate )
	{
		floatbuf = *pic;

		for ( i = 0; i < ( w * h ); i++ )
		{
			for ( j = 0; j < 3; j++ )
			{
				*floatbuf = *floatbuf / r_hdrLightmapCompensate->value;
				floatbuf++;
			}
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

#if 0
	w = h = 0;
	LoadRGBEToFloats( name, &hdrImage, &w, &h, qtrue, qtrue, qtrue );

	*width = w;
	*height = h;

	*ldrImage = ri.Malloc( w * h * 4 );
	pixbuf = *ldrImage;

	floatbuf = hdrImage;

	for ( i = 0; i < ( w * h ); i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			sample[ j ] = *floatbuf++;
		}

		NormalizeColor( sample, sample );

		*pixbuf++ = ( byte )( sample[ 0 ] * 255 );
		*pixbuf++ = ( byte )( sample[ 1 ] * 255 );
		*pixbuf++ = ( byte )( sample[ 2 ] * 255 );
		*pixbuf++ = ( byte ) 255;
	}

#else
	w = h = 0;
	LoadRGBEToFloats( name, &hdrImage, &w, &h, qfalse, qfalse, qfalse );

	*width = w;
	*height = h;

	*ldrImage = ri.Z_Malloc( w * h * 4 );
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

#endif

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

			ri.Printf( PRINT_ALL, "...loading %i HDR lightmaps\n", numLightmaps );

			if ( r_hdrRendering->integer && r_hdrLightmap->integer && glConfig2.framebufferObjectAvailable &&
			     glConfig2.framebufferBlitAvailable && glConfig2.textureFloatAvailable && glConfig2.textureHalfFloatAvailable )
			{
				int            width, height;
				unsigned short *hdrImage;

				for ( i = 0; i < numLightmaps; i++ )
				{
					ri.Printf( PRINT_ALL, "...loading external lightmap as RGB 16 bit half HDR '%s/%s'\n", mapName, lightmapFiles[ i ] );

					width = height = 0;
					//LoadRGBEToFloats(va("%s/%s", mapName, lightmapFiles[i]), &hdrImage, &width, &height, qtrue, qfalse, qtrue);
					LoadRGBEToHalfs( va( "%s/%s", mapName, lightmapFiles[ i ] ), &hdrImage, &width, &height );

					//ri.Printf(PRINT_ALL, "...converted '%s/%s' to HALF format\n", mapName, lightmapFiles[i]);

					image = R_AllocImage( va( "%s/%s", mapName, lightmapFiles[ i ] ), qtrue );

					if ( !image )
					{
						Com_Dealloc( hdrImage );
						break;
					}

					//Q_strncpyz(image->name, );
					image->type = GL_TEXTURE_2D;

					image->width = width;
					image->height = height;

					image->bits = IF_NOPICMIP | IF_RGBA16F;
					image->filterType = FT_NEAREST;
					image->wrapType = WT_CLAMP;

					GL_Bind( image );

					image->internalFormat = GL_RGBA16F_ARB;
					image->uploadWidth = width;
					image->uploadHeight = height;

					glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB16F_ARB, width, height, 0, GL_RGB, GL_HALF_FLOAT_ARB, hdrImage );

					if ( glConfig2.generateMipmapAvailable )
					{
						//glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);    // make sure it's nice
						glTexParameteri( image->type, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
						glTexParameteri( image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );  // default to trilinear
					}

#if 0

					if ( glConfig.hardwareType == GLHW_NV_DX10 || glConfig.hardwareType == GLHW_ATI_DX10 )
					{
						glTexParameterf( image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
						glTexParameterf( image->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
					}
					else
#endif
					{
						glTexParameterf( image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
						glTexParameterf( image->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
					}

					glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
					glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

					glBindTexture( image->type, 0 );

					GL_CheckErrors();

					Com_Dealloc( hdrImage );

					Com_AddToGrowList( &tr.lightmaps, image );
				}
			}
			else
			{
				int  width, height;
				byte *ldrImage;

				for ( i = 0; i < numLightmaps; i++ )
				{
					ri.Printf( PRINT_ALL, "...loading external lightmap as RGB8 LDR '%s/%s'\n", mapName, lightmapFiles[ i ] );

					width = height = 0;
					LoadRGBEToBytes( va( "%s/%s", mapName, lightmapFiles[ i ] ), &ldrImage, &width, &height );

					image = R_CreateImage( va( "%s/%s", mapName, lightmapFiles[ i ] ), ( byte * ) ldrImage, width, height,
					                       IF_NOPICMIP | IF_LIGHTMAP | IF_NOCOMPRESSION, FT_DEFAULT, WT_CLAMP );

					Com_AddToGrowList( &tr.lightmaps, image );

					ri.Free( ldrImage );
				}
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
						ri.Printf( PRINT_WARNING, "WARNING: no lightmap files found\n" );
						return;
					}
				}

				qsort( lightmapFiles, numLightmaps, sizeof( char * ), LightmapNameCompare );

				ri.Printf( PRINT_ALL, "...loading %i deluxemaps\n", numLightmaps );

				for ( i = 0; i < numLightmaps; i++ )
				{
					ri.Printf( PRINT_ALL, "...loading external lightmap '%s/%s'\n", mapName, lightmapFiles[ i ] );

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
					ri.Printf( PRINT_WARNING, "WARNING: no lightmap files found\n" );
					return;
				}
			}

			qsort( lightmapFiles, numLightmaps, sizeof( char * ), LightmapNameCompare );

			ri.Printf( PRINT_ALL, "...loading %i lightmaps\n", numLightmaps );

			// we are about to upload textures
			R_SyncRenderThread();

			for ( i = 0; i < numLightmaps; i++ )
			{
				ri.Printf( PRINT_ALL, "...loading external lightmap '%s/%s'\n", mapName, lightmapFiles[ i ] );

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

#if 0 //defined(COMPAT_ET)
	else
	{
		static byte data[ LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4 ];

		buf = fileBase + l->fileofs;

		// we are about to upload textures
		R_SyncRenderThread();

		// create all the lightmaps
		tr.numLightmaps = len / ( LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3 );

		ri.Printf( PRINT_ALL, "...loading %i lightmaps\n", tr.numLightmaps );

		for ( i = 0; i < tr.numLightmaps; i++ )
		{
			// expand the 24 bit on-disk to 32 bit
			buf_p = buf + i * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;

			if ( tr.worldDeluxeMapping )
			{
				if ( i % 2 == 0 )
				{
					for ( j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ )
					{
						R_ColorShiftLightingBytes( &buf_p[ j * 3 ], &data[ j * 4 ] );
						data[ j * 4 + 3 ] = 255;
					}

					image = R_CreateImage( va( "_lightmap%d", i ), data, LIGHTMAP_SIZE, LIGHTMAP_SIZE, IF_LIGHTMAP | IF_NOCOMPRESSION, FT_DEFAULT, WT_CLAMP );
					Com_AddToGrowList( &tr.lightmaps, image );
				}
				else
				{
					for ( j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ )
					{
						data[ j * 4 + 0 ] = buf_p[ j * 3 + 0 ];
						data[ j * 4 + 1 ] = buf_p[ j * 3 + 1 ];
						data[ j * 4 + 2 ] = buf_p[ j * 3 + 2 ];
						data[ j * 4 + 3 ] = 255;
					}

					image = R_CreateImage( va( "_lightmap%d", i ), data, LIGHTMAP_SIZE, LIGHTMAP_SIZE, IF_NORMALMAP, FT_DEFAULT, WT_CLAMP );
					Com_AddToGrowList( &tr.deluxemaps, image );
				}
			}
			else
			{
				for ( j = 0; j < LIGHTMAP_SIZE * LIGHTMAP_SIZE; j++ )
				{
					R_ColorShiftLightingBytes( &buf_p[ j * 3 ], &data[ j * 4 ] );
					data[ j * 4 + 3 ] = 255;
				}

				image = R_CreateImage( va( "_lightmap%d", i ), data, LIGHTMAP_SIZE, LIGHTMAP_SIZE, IF_LIGHTMAP | IF_NOCOMPRESSION, FT_DEFAULT, WT_CLAMP );
				Com_AddToGrowList( &tr.lightmaps, image );
			}
		}
	}

#elif defined( COMPAT_Q3A )
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
			//this avoids this, but isn't the correct solution.
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

		fatbuffer = ri.Hunk_AllocateTempMemory( sizeof( byte ) * tr.fatLightmapSize * tr.fatLightmapSize * 4 );

		Com_Memset( fatbuffer, 128, tr.fatLightmapSize * tr.fatLightmapSize * 4 );

		for ( i = 0; i < numLightmaps; i++ )
		{
			// expand the 24 bit on-disk to 32 bit
			buf_p = buf + i * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;

			xoff = i % tr.fatLightmapStep;
			yoff = i / tr.fatLightmapStep;

			//if (tr.radbumping==qfalse)
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

			/*else
			   {
			   //We need to darken the lightmaps a little bit when mixing radbump and fallback path rendering
			   //because radbump will be darker due to the error introduced by using 3 basis vector probes for lighting instead of surf normal.
			   for ( y = 0 ; y < LIGHTMAP_SIZE ; y++ )
			   {
			   for ( x = 0 ; x < LIGHTMAP_SIZE ; x++ )
			   {
			   int index = (x+(y*tr.fatLightmapSize))+((xoff*LIGHTMAP_SIZE)+(yoff*tr.fatLightmapSize*LIGHTMAP_SIZE));
			   fatbuffer[(index*4)+0 ]=(byte)(((float)buf_p[((x+(y*LIGHTMAP_SIZE))*3)+0])*scale);
			   fatbuffer[(index*4)+1 ]=(byte)(((float)buf_p[((x+(y*LIGHTMAP_SIZE))*3)+1])*scale);
			   fatbuffer[(index*4)+2 ]=(byte)(((float)buf_p[((x+(y*LIGHTMAP_SIZE))*3)+2])*scale);
			   fatbuffer[(index*4)+3 ]=255;
			   }
			   }

			   } */
		}

		//memset(fatbuffer,128,tr.fatLightmapSize*tr.fatLightmapSize*4);

		tr.fatLightmap = R_CreateImage( va( "_fatlightmap%d", 0 ), fatbuffer, tr.fatLightmapSize, tr.fatLightmapSize, IF_LIGHTMAP | IF_NOCOMPRESSION, FT_DEFAULT, WT_CLAMP );
		Com_AddToGrowList( &tr.lightmaps, tr.fatLightmap );

		ri.Hunk_FreeTempMemory( fatbuffer );
	}

#endif
}

#if defined( COMPAT_Q3A )
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

#endif

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
	int  len;
	byte *buf;

	ri.Printf( PRINT_ALL, "...loading visibility\n" );

	len = ( s_worldData.numClusters + 63 ) & ~63;
	s_worldData.novis = ri.Hunk_Alloc( len, h_low );
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

		dest = ri.Hunk_Alloc( len - 8, h_low );
		Com_Memcpy( dest, buf + 8, len - 8 );
		s_worldData.vis = dest;
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

//  ri.Printf(PRINT_ALL, "ShaderForShaderNum: '%s'\n", dsh->shader);

	shader = R_FindShader( dsh->shader, SHADER_3D_STATIC, qtrue );

	// if the shader had errors, just use default shader
	if ( shader->defaultShader )
	{
//      ri.Printf(PRINT_ALL, "failed\n");
		return tr.defaultShader;
	}

//  ri.Printf(PRINT_ALL, "success\n");
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

	// take the plane normal from the lightmap vector and classify it
	gen->plane.normal[ 0 ] = LittleFloat( ds->lightmapVecs[ 2 ][ 0 ] );
	gen->plane.normal[ 1 ] = LittleFloat( ds->lightmapVecs[ 2 ][ 1 ] );
	gen->plane.normal[ 2 ] = LittleFloat( ds->lightmapVecs[ 2 ][ 2 ] );
	gen->plane.dist = DotProduct( pt, gen->plane.normal );
	SetPlaneSignbits( &gen->plane );
	gen->plane.type = PlaneTypeForNormal( gen->plane.normal );
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

	/*
	if(surf->lightmapNum >= tr.lightmaps.currentElements)
	{
	        ri.Error(ERR_DROP, "Bad lightmap number %i in face surface", surf->lightmapNum);
	}
	*/

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum );

	if ( r_singleShader->integer && !surf->shader->isSky )
	{
		surf->shader = tr.defaultShader;
	}

	numVerts = LittleLong( ds->numVerts );

	/*
	   if(numVerts > MAX_FACE_POINTS)
	   {
	   ri.Printf(PRINT_WARNING, "WARNING: MAX_FACE_POINTS exceeded: %i\n", numVerts);
	   numVerts = MAX_FACE_POINTS;
	   surf->shader = tr.defaultShader;
	   }
	 */
	numTriangles = LittleLong( ds->numIndexes ) / 3;

	cv = ri.Hunk_Alloc( sizeof( *cv ), h_low );
	cv->surfaceType = SF_FACE;

	cv->numTriangles = numTriangles;
	cv->triangles = ri.Hunk_Alloc( numTriangles * sizeof( cv->triangles[ 0 ] ), h_low );

	cv->numVerts = numVerts;
	cv->verts = ri.Hunk_Alloc( numVerts * sizeof( cv->verts[ 0 ] ), h_low );

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

#if defined( COMPAT_Q3A )
		cv->verts[ i ].lightmap[ 0 ] = FatPackU( LittleFloat( verts[ i ].lightmap[ 0 ] ), realLightmapNum );
		cv->verts[ i ].lightmap[ 1 ] = FatPackV( LittleFloat( verts[ i ].lightmap[ 1 ] ), realLightmapNum );

		for ( j = 0; j < 4; j++ )
		{
			cv->verts[ i ].lightColor[ j ] = verts[ i ].color[ j ] * ( 1.0f / 255.0f );
		}

		R_ColorShiftLightingFloats( cv->verts[ i ].lightColor, cv->verts[ i ].lightColor );

#elif defined( COMPAT_Q3A ) || defined( COMPAT_ET )

		for ( j = 0; j < 4; j++ )
		{
			cv->verts[ i ].lightColor[ j ] = verts[ i ].color[ j ] * ( 1.0f / 255.0f );
		}

		R_ColorShiftLightingFloats( cv->verts[ i ].lightColor, cv->verts[ i ].lightColor );
#else

		for ( j = 0; j < 4; j++ )
		{
			cv->verts[ i ].paintColor[ j ] = Q_bound( 0, LittleFloat( verts[ i ].paintColor[ j ] ), 1 );
			cv->verts[ i ].lightColor[ j ] = LittleFloat( verts[ i ].lightColor[ j ] );
		}

		for ( j = 0; j < 3; j++ )
		{
			cv->verts[ i ].lightDirection[ j ] = LittleFloat( verts[ i ].lightDirection[ j ] );
		}

		//VectorNormalize(cv->verts[i].lightDirection);

		R_HDRTonemapLightingColors( cv->verts[ i ].lightColor, cv->verts[ i ].lightColor, qtrue );
#endif
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

	R_CalcSurfaceTriangleNeighbors( numTriangles, cv->triangles );
	R_CalcSurfaceTrianglePlanes( numTriangles, cv->triangles, cv->verts );

	// take the plane information from the lightmap vector
	for ( i = 0; i < 3; i++ )
	{
		cv->plane.normal[ i ] = LittleFloat( ds->lightmapVecs[ 2 ][ i ] );
	}

	cv->plane.dist = DotProduct( cv->verts[ 0 ].xyz, cv->plane.normal );
	SetPlaneSignbits( &cv->plane );
	cv->plane.type = PlaneTypeForNormal( cv->plane.normal );

	surf->data = ( surfaceType_t * ) cv;

	// Tr3B - calc tangent spaces
#if 0
	{
		float       *v;
		const float *v0, *v1, *v2;
		const float *t0, *t1, *t2;
		vec3_t      tangent;
		vec3_t      binormal;
		vec3_t      normal;

		for ( i = 0; i < numVerts; i++ )
		{
			VectorClear( cv->verts[ i ].tangent );
			VectorClear( cv->verts[ i ].binormal );
			VectorClear( cv->verts[ i ].normal );
		}

		for ( i = 0, tri = cv->triangles; i < numTriangles; i++, tri++ )
		{
			v0 = cv->verts[ tri->indexes[ 0 ] ].xyz;
			v1 = cv->verts[ tri->indexes[ 1 ] ].xyz;
			v2 = cv->verts[ tri->indexes[ 2 ] ].xyz;

			t0 = cv->verts[ tri->indexes[ 0 ] ].st;
			t1 = cv->verts[ tri->indexes[ 1 ] ].st;
			t2 = cv->verts[ tri->indexes[ 2 ] ].st;

			R_CalcTangentSpace( tangent, binormal, normal, v0, v1, v2, t0, t1, t2 );

			for ( j = 0; j < 3; j++ )
			{
				v = cv->verts[ tri->indexes[ j ] ].tangent;
				VectorAdd( v, tangent, v );
				v = cv->verts[ tri->indexes[ j ] ].binormal;
				VectorAdd( v, binormal, v );
				v = cv->verts[ tri->indexes[ j ] ].normal;
				VectorAdd( v, normal, v );
			}
		}

		for ( i = 0; i < numVerts; i++ )
		{
			VectorNormalize( cv->verts[ i ].tangent );
			VectorNormalize( cv->verts[ i ].binormal );
			VectorNormalize( cv->verts[ i ].normal );
		}
	}
#else
	{
		srfVert_t *dv[ 3 ];

		for ( i = 0, tri = cv->triangles; i < numTriangles; i++, tri++ )
		{
			dv[ 0 ] = &cv->verts[ tri->indexes[ 0 ] ];
			dv[ 1 ] = &cv->verts[ tri->indexes[ 1 ] ];
			dv[ 2 ] = &cv->verts[ tri->indexes[ 2 ] ];

			R_CalcTangentVectors( dv );
		}
	}
#endif

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
#if defined( COMPAT_ET )

	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW )
#else
	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & ( SURF_NODRAW | SURF_COLLISION ) )
#endif
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

#if defined( COMPAT_Q3A )
		points[ i ].lightmap[ 0 ] = FatPackU( LittleFloat( verts[ i ].lightmap[ 0 ] ), realLightmapNum );
		points[ i ].lightmap[ 1 ] = FatPackV( LittleFloat( verts[ i ].lightmap[ 1 ] ), realLightmapNum );

		for ( j = 0; j < 4; j++ )
		{
			points[ i ].lightColor[ j ] = verts[ i ].color[ j ] * ( 1.0f / 255.0f );
		}

		R_ColorShiftLightingFloats( points[ i ].lightColor, points[ i ].lightColor );

#elif defined( COMPAT_Q3A ) || defined( COMPAT_ET )

		for ( j = 0; j < 4; j++ )
		{
			points[ i ].lightColor[ j ] = verts[ i ].color[ j ] * ( 1.0f / 255.0f );
		}

		R_ColorShiftLightingFloats( points[ i ].lightColor, points[ i ].lightColor );
#else

		for ( j = 0; j < 4; j++ )
		{
			points[ i ].paintColor[ j ] = Q_bound( 0, LittleFloat( verts[ i ].paintColor[ j ] ), 1 );
			points[ i ].lightColor[ j ] = LittleFloat( verts[ i ].lightColor[ j ] );
		}

		for ( j = 0; j < 3; j++ )
		{
			points[ i ].lightDirection[ j ] = LittleFloat( verts[ i ].lightDirection[ j ] );
		}

		//VectorNormalize(points[i].lightDirection);

		R_HDRTonemapLightingColors( points[ i ].lightColor, points[ i ].lightColor, qtrue );
#endif
	}

	// pre-tesseleate
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
//	int             realLightmapNum;
	static surfaceType_t skipData = SF_SKIP;

	// get lightmap
#if defined( COMPAT_Q3A )
	surf->lightmapNum = -1; // FIXME LittleLong(ds->lightmapNum);
#else
	realLightmapNum = LittleLong( ds->lightmapNum );

	if ( r_vertexLighting->integer || !r_precomputedLighting->integer )
	{
		surf->lightmapNum = -1;
	}
	else
	{
		surf->lightmapNum = realLightmapNum;
	}

#endif

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
#if defined( COMPAT_ET )

	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & SURF_NODRAW )
#else
	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & ( SURF_NODRAW | SURF_COLLISION ) )
#endif
	{
		surf->data = &skipData;
		return;
	}

	numVerts = LittleLong( ds->numVerts );
	numTriangles = LittleLong( ds->numIndexes ) / 3;

	cv = ri.Hunk_Alloc( sizeof( *cv ), h_low );
	cv->surfaceType = SF_TRIANGLES;

	cv->numTriangles = numTriangles;
	cv->triangles = ri.Hunk_Alloc( numTriangles * sizeof( cv->triangles[ 0 ] ), h_low );

	cv->numVerts = numVerts;
	cv->verts = ri.Hunk_Alloc( numVerts * sizeof( cv->verts[ 0 ] ), h_low );

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

#if defined( COMPAT_Q3A ) || defined( COMPAT_ET )

		for ( j = 0; j < 4; j++ )
		{
			cv->verts[ i ].lightColor[ j ] = verts[ i ].color[ j ] * ( 1.0f / 255.0f );
		}

		R_ColorShiftLightingFloats( cv->verts[ i ].lightColor, cv->verts[ i ].lightColor );
#else

		for ( j = 0; j < 4; j++ )
		{
			cv->verts[ i ].paintColor[ j ] = Q_bound( 0, LittleFloat( verts[ i ].paintColor[ j ] ), 1 );
			cv->verts[ i ].lightColor[ j ] = LittleFloat( verts[ i ].lightColor[ j ] );
		}

		for ( j = 0; j < 3; j++ )
		{
			cv->verts[ i ].lightDirection[ j ] = LittleFloat( verts[ i ].lightDirection[ j ] );
		}

		//VectorNormalize(cv->verts[i].lightDirection);

		R_HDRTonemapLightingColors( cv->verts[ i ].lightColor, cv->verts[ i ].lightColor, qtrue );
#endif
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

	R_CalcSurfaceTriangleNeighbors( numTriangles, cv->triangles );
	R_CalcSurfaceTrianglePlanes( numTriangles, cv->triangles, cv->verts );

	// Tr3B - calc tangent spaces
#if 0
	{
		float       *v;
		const float *v0, *v1, *v2;
		const float *t0, *t1, *t2;
		vec3_t      tangent;
		vec3_t      binormal;
		vec3_t      normal;

		for ( i = 0; i < numVerts; i++ )
		{
			VectorClear( cv->verts[ i ].tangent );
			VectorClear( cv->verts[ i ].binormal );
			VectorClear( cv->verts[ i ].normal );
		}

		for ( i = 0, tri = cv->triangles; i < numTriangles; i++, tri++ )
		{
			v0 = cv->verts[ tri->indexes[ 0 ] ].xyz;
			v1 = cv->verts[ tri->indexes[ 1 ] ].xyz;
			v2 = cv->verts[ tri->indexes[ 2 ] ].xyz;

			t0 = cv->verts[ tri->indexes[ 0 ] ].st;
			t1 = cv->verts[ tri->indexes[ 1 ] ].st;
			t2 = cv->verts[ tri->indexes[ 2 ] ].st;

#if 1
			R_CalcTangentSpace( tangent, binormal, normal, v0, v1, v2, t0, t1, t2 );
#else
			R_CalcNormalForTriangle( normal, v0, v1, v2 );
			R_CalcTangentsForTriangle2( tangent, binormal, v0, v1, v2, t0, t1, t2 );
#endif

			for ( j = 0; j < 3; j++ )
			{
				v = cv->verts[ tri->indexes[ j ] ].tangent;
				VectorAdd( v, tangent, v );
				v = cv->verts[ tri->indexes[ j ] ].binormal;
				VectorAdd( v, binormal, v );
				v = cv->verts[ tri->indexes[ j ] ].normal;
				VectorAdd( v, normal, v );
			}
		}

		for ( i = 0; i < numVerts; i++ )
		{
			float dot;

			//VectorNormalize(cv->verts[i].tangent);
			VectorNormalize( cv->verts[ i ].binormal );
			VectorNormalize( cv->verts[ i ].normal );

			// Gram-Schmidt orthogonalize
			dot = DotProduct( cv->verts[ i ].normal, cv->verts[ i ].tangent );
			VectorMA( cv->verts[ i ].tangent, -dot, cv->verts[ i ].normal, cv->verts[ i ].tangent );
			VectorNormalize( cv->verts[ i ].tangent );

			//dot = DotProduct(cv->verts[i].normal, cv->verts[i].tangent);
			//VectorMA(cv->verts[i].tangent, -dot, cv->verts[i].normal, cv->verts[i].tangent);
			//VectorNormalize(cv->verts[i].tangent);
		}
	}
#else
	{
		srfVert_t *dv[ 3 ];

		for ( i = 0, tri = cv->triangles; i < numTriangles; i++, tri++ )
		{
			dv[ 0 ] = &cv->verts[ tri->indexes[ 0 ] ];
			dv[ 1 ] = &cv->verts[ tri->indexes[ 1 ] ];
			dv[ 2 ] = &cv->verts[ tri->indexes[ 2 ] ];

			R_CalcTangentVectors( dv );
		}
	}
#endif

#if 0

	// do another extra smoothing for normals to avoid flat shading
	for ( i = 0; i < numVerts; i++ )
	{
		for ( j = 0; j < numVerts; j++ )
		{
			if ( i == j )
			{
				continue;
			}

			if ( R_CompareVert( &cv->verts[ i ], &cv->verts[ j ], qfalse ) )
			{
				VectorAdd( cv->verts[ i ].normal, cv->verts[ j ].normal, cv->verts[ i ].normal );
			}
		}

		VectorNormalize( cv->verts[ i ].normal );
	}

#endif

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

	flare = ri.Hunk_Alloc( sizeof( *flare ), h_low );
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
			//grid2->lodFixed = 1;
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
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
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
					s_worldData.surfaces[ grid2num ].data = ( void * ) grid2;
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
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
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
					s_worldData.surfaces[ grid2num ].data = ( void * ) grid2;
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
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
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
					s_worldData.surfaces[ grid2num ].data = ( void * ) grid2;
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
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
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
					s_worldData.surfaces[ grid2num ].data = ( void * ) grid2;
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
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					if ( m )
					{
						row = grid2->height - 1;
					}
					else
					{
						row = 0;
					}

					grid2 = R_GridInsertColumn( grid2, l + 1, row, grid1->verts[ k - 1 + offset1 ].xyz, grid1->widthLodError[ k + 1 ] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( void * ) grid2;
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
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					if ( m )
					{
						column = grid2->width - 1;
					}
					else
					{
						column = 0;
					}

					grid2 = R_GridInsertRow( grid2, l + 1, column, grid1->verts[ k - 1 + offset1 ].xyz, grid1->widthLodError[ k + 1 ] );

					if ( !grid2 )
					{
						break;
					}

					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( void * ) grid2;
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
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
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
					                            grid1->verts[ grid1->width * ( k - 1 ) + offset1 ].xyz, grid1->heightLodError[ k + 1 ] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( void * ) grid2;
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
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
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
					                         grid1->verts[ grid1->width * ( k - 1 ) + offset1 ].xyz, grid1->heightLodError[ k + 1 ] );
					grid2->lodStitched = qfalse;
					s_worldData.surfaces[ grid2num ].data = ( void * ) grid2;
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

	ri.Printf( PRINT_ALL, "...stitching LoD cracks\n" );

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

	ri.Printf( PRINT_ALL, "stitched %d LoD cracks\n", numstitches );
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
		hunkgrid = ri.Hunk_Alloc( size, h_low );
		Com_Memcpy( hunkgrid, grid, size );

		hunkgrid->widthLodError = ri.Hunk_Alloc( grid->width * 4, h_low );
		Com_Memcpy( hunkgrid->widthLodError, grid->widthLodError, grid->width * 4 );

		hunkgrid->heightLodError = ri.Hunk_Alloc( grid->height * 4, h_low );
		Com_Memcpy( hunkgrid->heightLodError, grid->heightLodError, grid->height * 4 );

		hunkgrid->numTriangles = grid->numTriangles;
		hunkgrid->triangles = ri.Hunk_Alloc( grid->numTriangles * sizeof( srfTriangle_t ), h_low );
		Com_Memcpy( hunkgrid->triangles, grid->triangles, grid->numTriangles * sizeof( srfTriangle_t ) );

		hunkgrid->numVerts = grid->numVerts;
		hunkgrid->verts = ri.Hunk_Alloc( grid->numVerts * sizeof( srfVert_t ), h_low );
		Com_Memcpy( hunkgrid->verts, grid->verts, grid->numVerts * sizeof( srfVert_t ) );

		R_FreeSurfaceGridMesh( grid );

		s_worldData.surfaces[ i ].data = ( void * ) hunkgrid;
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
		out->tangent[ j ] = in->tangent[ j ];
		out->binormal[ j ] = in->binormal[ j ];
		out->normal[ j ] = in->normal[ j ];
		out->lightDirection[ j ] = in->lightDirection[ j ];
	}

	for ( j = 0; j < 2; j++ )
	{
		out->st[ j ] = in->st[ j ];
		out->lightmap[ j ] = in->lightmap[ j ];
	}

	for ( j = 0; j < 4; j++ )
	{
		out->paintColor[ j ] = in->paintColor[ j ];
		out->lightColor[ j ] = in->lightColor[ j ];
	}

#if DEBUG_OPTIMIZEVERTICES
	out->id = in->id;
#endif
}

#define EQUAL_EPSILON 0.001

/*static qboolean CompareWorldVert(const srfVert_t * v1, const srfVert_t * v2)
{
        int             i;

        for(i = 0; i < 3; i++)
        {
                //if(fabs(v1->xyz[i] - v2->xyz[i]) > EQUAL_EPSILON)
                if(v1->xyz[i] != v2->xyz[i])
                        return qfalse;
        }

        for(i = 0; i < 2; i++)
        {
                if(v1->st[i] != v2->st[i])
                        return qfalse;
        }

        if(r_precomputedLighting->integer)
        {
                for(i = 0; i < 2; i++)
                {
                        if(v1->lightmap[i] != v2->lightmap[i])
                                return qfalse;
                }
        }

        for(i = 0; i < 4; i++)
        {
                if(v1->lightColor[i] != v2->lightColor[i])
                        return qfalse;
        }

        return qtrue;
}

static qboolean CompareLightVert(const srfVert_t * v1, const srfVert_t * v2)
{
        int             i;

        for(i = 0; i < 3; i++)
        {
                //if(fabs(v1->xyz[i] - v2->xyz[i]) > EQUAL_EPSILON)
                if(v1->xyz[i] != v2->xyz[i])
                        return qfalse;
        }

        for(i = 0; i < 2; i++)
        {
                if(v1->st[i] != v2->st[i])
                        return qfalse;
        }

        for(i = 0; i < 4; i++)
        {
                if(v1->paintColor[i] != v2->paintColor[i])
                        return qfalse;
        }

        return qtrue;
}

static qboolean CompareShadowVertAlphaTest(const srfVert_t * v1, const srfVert_t * v2)
{
        int             i;

        for(i = 0; i < 3; i++)
        {
                //if(fabs(v1->xyz[i] - v2->xyz[i]) > EQUAL_EPSILON)
                if(v1->xyz[i] != v2->xyz[i])
                        return qfalse;
        }

        for(i = 0; i < 2; i++)
        {
                if(v1->st[i] != v2->st[i])
                        return qfalse;
        }

        return qtrue;
}

static qboolean CompareShadowVert(const srfVert_t * v1, const srfVert_t * v2)
{
        int             i;

        for(i = 0; i < 3; i++)
        {
                //if(fabs(v1->xyz[i] - v2->xyz[i]) > EQUAL_EPSILON)
                if(v1->xyz[i] != v2->xyz[i])
                        return qfalse;
        }

        return qtrue;
}*/

#if 0
static qboolean CompareShadowVolumeVert( const srfVert_t *v1, const srfVert_t *v2 )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		// don't consider epsilon to avoid shadow volume z-fighting
		if ( v1->xyz[ i ] != v2->xyz[ i ] )
		{
			return qfalse;
		}
	}

	return qtrue;
}

#endif

/*static qboolean CompareWorldVertSmoothNormal(const srfVert_t * v1, const srfVert_t * v2)
{
        int             i;

        for(i = 0; i < 3; i++)
        {
                if(fabs(v1->xyz[i] - v2->xyz[i]) > EQUAL_EPSILON)
                        return qfalse;
        }

        return qtrue;
}*/

/*
remove duplicated / redundant vertices from a batch of vertices
return the new number of vertices
*/
#if 0
static int OptimizeVertices( int numVerts, srfVert_t *verts, int numTriangles, srfTriangle_t *triangles, srfVert_t *outVerts,
                             qboolean( *CompareVert )( const srfVert_t *v1, const srfVert_t *v2 ) )
{
	srfTriangle_t *tri;
	int           i, j, k, l;

	static int    redundantIndex[ MAX_MAP_DRAW_VERTS ];
	int           numOutVerts;

	if ( r_vboOptimizeVertices->integer )
	{
		if ( numVerts >= MAX_MAP_DRAW_VERTS )
		{
			ri.Printf( PRINT_WARNING, "OptimizeVertices: MAX_MAP_DRAW_VERTS reached\n" );

			for ( j = 0; j < numVerts; j++ )
			{
				CopyVert( &verts[ j ], &outVerts[ j ] );
			}

			return numVerts;
		}

		memset( redundantIndex, -1, sizeof( redundantIndex ) );

		c_redundantVertexes = 0;
		numOutVerts = 0;

		for ( i = 0; i < numVerts; i++ )
		{
#if DEBUG_OPTIMIZEVERTICES
			verts[ i ].id = i;
#endif

			if ( redundantIndex[ i ] == -1 )
			{
				for ( j = i + 1; j < numVerts; j++ )
				{
					if ( redundantIndex[ i ] != -1 )
					{
						continue;
					}

					if ( CompareVert( &verts[ i ], &verts[ j ] ) )
					{
						// mark vertex as redundant
						redundantIndex[ j ] = i; //numOutVerts;
					}
				}
			}
		}

#if DEBUG_OPTIMIZEVERTICES
		ri.Printf( PRINT_ALL, "input triangles: " );

		for ( k = 0, tri = triangles; k < numTriangles; k++, tri++ )
		{
			ri.Printf( PRINT_ALL, "(%i,%i,%i),", verts[ tri->indexes[ 0 ] ].id, verts[ tri->indexes[ 1 ] ].id, verts[ tri->indexes[ 2 ] ].id );
		}

		ri.Printf( PRINT_ALL, "\n" );
#endif

#if DEBUG_OPTIMIZEVERTICES
		ri.Printf( PRINT_ALL, "input vertices: " );

		for ( i = 0; i < numVerts; i++ )
		{
			if ( redundantIndex[ i ] != -1 )
			{
				ri.Printf( PRINT_ALL, "(%i,%i),", i, redundantIndex[ i ] );
			}
			else
			{
				ri.Printf( PRINT_ALL, "(%i,-),", i );
			}
		}

		ri.Printf( PRINT_ALL, "\n" );
#endif

		for ( i = 0; i < numVerts; i++ )
		{
			if ( redundantIndex[ i ] != -1 )
			{
				c_redundantVertexes++;
			}
			else
			{
				CopyVert( &verts[ i ], &outVerts[ numOutVerts ] );
				numOutVerts++;
			}
		}

#if DEBUG_OPTIMIZEVERTICES
		ri.Printf( PRINT_ALL, "output vertices: " );

		for ( i = 0; i < numOutVerts; i++ )
		{
			ri.Printf( PRINT_ALL, "(%i),", outVerts[ i ].id );
		}

		ri.Printf( PRINT_ALL, "\n" );
#endif

		for ( i = 0; i < numVerts; )
		{
			qboolean noIncrement = qfalse;

			if ( redundantIndex[ i ] != -1 )
			{
#if DEBUG_OPTIMIZEVERTICES
				ri.Printf( PRINT_ALL, "-------------------------------------------------\n" );
				ri.Printf( PRINT_ALL, "changing triangles for redundant vertex (%i->%i):\n", i, redundantIndex[ i ] );
#endif

				// kill redundant vert
				for ( k = 0, tri = triangles; k < numTriangles; k++, tri++ )
				{
					for ( l = 0; l < 3; l++ )
					{
						if ( tri->indexes[ l ] == i ) //redundantIndex[i])
						{
							// replace duplicated index j with the original vertex index i
							tri->indexes[ l ] = redundantIndex[ i ]; //numOutVerts;

#if DEBUG_OPTIMIZEVERTICES
							ri.Printf( PRINT_ALL, "mapTriangleIndex<%i,%i>(%i->%i)\n", k, l, i, redundantIndex[ i ] );
#endif
						}

#if 1
						else if ( tri->indexes[ l ] > i ) // && redundantIndex[tri->indexes[l]] == -1)
						{
							tri->indexes[ l ]--;
#if DEBUG_OPTIMIZEVERTICES
							ri.Printf( PRINT_ALL, "decTriangleIndex<%i,%i>(%i->%i)\n", k, l, tri->indexes[ l ] + 1, tri->indexes[ l ] );
#endif

							if ( tri->indexes[ l ] < 0 )
							{
								ri.Printf( PRINT_WARNING, "OptimizeVertices: triangle index < 0\n" );

								for ( j = 0; j < numVerts; j++ )
								{
									CopyVert( &verts[ j ], &outVerts[ j ] );
								}

								return numVerts;
							}
						}

#endif
					}
				}

#if DEBUG_OPTIMIZEVERTICES
				ri.Printf( PRINT_ALL, "pending redundant vertices: " );

				for ( j = i + 1; j < numVerts; j++ )
				{
					if ( redundantIndex[ j ] != -1 )
					{
						ri.Printf( PRINT_ALL, "(%i,%i),", j, redundantIndex[ j ] );
					}
					else
					{
						//ri.Printf(PRINT_ALL, "(%i,-),", j);
					}
				}

				ri.Printf( PRINT_ALL, "\n" );
#endif

				for ( j = i + 1; j < numVerts; j++ )
				{
					if ( redundantIndex[ j ] != -1 ) //> i)//== tri->indexes[l])
					{
#if DEBUG_OPTIMIZEVERTICES
						ri.Printf( PRINT_ALL, "updateRedundantIndex(%i->%i) to (%i->%i)\n", j, redundantIndex[ j ], j - 1,
						           redundantIndex[ j ] );
#endif

						if ( redundantIndex[ j ] <= i )
						{
							redundantIndex[ j - 1 ] = redundantIndex[ j ];
							redundantIndex[ j ] = -1;
						}
						else
						{
							redundantIndex[ j - 1 ] = redundantIndex[ j ] - 1;
							redundantIndex[ j ] = -1;
						}

						if ( ( j - 1 ) == i )
						{
							noIncrement = qtrue;
						}
					}
				}

#if DEBUG_OPTIMIZEVERTICES
				ri.Printf( PRINT_ALL, "current triangles: " );

				for ( k = 0, tri = triangles; k < numTriangles; k++, tri++ )
				{
					ri.Printf( PRINT_ALL, "(%i,%i,%i),", verts[ tri->indexes[ 0 ] ].id, verts[ tri->indexes[ 1 ] ].id,
					           verts[ tri->indexes[ 2 ] ].id );
				}

				ri.Printf( PRINT_ALL, "\n" );
#endif
			}

			if ( !noIncrement )
			{
				i++;
			}
		}

#if DEBUG_OPTIMIZEVERTICES
		ri.Printf( PRINT_ALL, "output triangles: " );

		for ( k = 0, tri = triangles; k < numTriangles; k++, tri++ )
		{
			ri.Printf( PRINT_ALL, "(%i,%i,%i),", verts[ tri->indexes[ 0 ] ].id, verts[ tri->indexes[ 1 ] ].id, verts[ tri->indexes[ 2 ] ].id );
		}

		ri.Printf( PRINT_ALL, "\n" );
#endif

		if ( c_redundantVertexes )
		{
			//*numVerts -= c_redundantVertexes;

			//ri.Printf(PRINT_ALL, "removed %i redundant vertices\n", c_redundantVertexes);
		}

		return numOutVerts;
	}
	else
	{
		for ( j = 0; j < numVerts; j++ )
		{
			CopyVert( &verts[ j ], &outVerts[ j ] );
		}

		return numVerts;
	}
}

#endif

/*static void OptimizeTriangles(int numVerts, srfVert_t * verts, int numTriangles, srfTriangle_t * triangles,
                                                          qboolean(*compareVert) (const srfVert_t * v1, const srfVert_t * v2))
{
#if 1
        srfTriangle_t  *tri;
        int             i, j, k, l;
        static int      redundantIndex[MAX_MAP_DRAW_VERTS];
        static          qboolean(*compareFunction) (const srfVert_t * v1, const srfVert_t * v2) = NULL;
        //static int      minVertOld = 9999999, maxVertOld = 0;
        int             minVert, maxVert;

        if(numVerts >= MAX_MAP_DRAW_VERTS)
        {
                ri.Printf(PRINT_WARNING, "OptimizeTriangles: MAX_MAP_DRAW_VERTS reached\n");
                return;
        }

        // find vertex bounds
        maxVert = 0;
        minVert = numVerts - 1;
        for(k = 0, tri = triangles; k < numTriangles; k++, tri++)
        {
                for(l = 0; l < 3; l++)
                {
                        if(tri->indexes[l] > maxVert)
                                maxVert = tri->indexes[l];

                        if(tri->indexes[l] < minVert)
                                minVert = tri->indexes[l];
                }
        }

        ri.Printf(PRINT_ALL, "OptimizeTriangles: minVert %i maxVert %i\n", minVert, maxVert);

//  if(compareFunction != compareVert || minVert != minVertOld || maxVert != maxVertOld)
        {
                compareFunction = compareVert;

                memset(redundantIndex, -1, sizeof(redundantIndex));

                for(i = minVert; i <= maxVert; i++)
                {
                        if(redundantIndex[i] == -1)
                        {
                                for(j = i + 1; j <= maxVert; j++)
                                {
                                        if(redundantIndex[i] != -1)
                                                continue;

                                        if(compareVert(&verts[i], &verts[j]))
                                        {
                                                // mark vertex as redundant
                                                redundantIndex[j] = i;
                                        }
                                }
                        }
                }
        }

        c_redundantVertexes = 0;
        for(i = minVert; i <= maxVert; i++)
        {
                if(redundantIndex[i] != -1)
                {
                        //ri.Printf(PRINT_ALL, "changing triangles for redundant vertex (%i->%i):\n", i, redundantIndex[i]);

                        // kill redundant vert
                        for(k = 0, tri = triangles; k < numTriangles; k++, tri++)
                        {
                                for(l = 0; l < 3; l++)
                                {
                                        if(tri->indexes[l] == i)  //redundantIndex[i])
                                        {
                                                // replace duplicated index j with the original vertex index i
                                                tri->indexes[l] = redundantIndex[i];

                                                //ri.Printf(PRINT_ALL, "mapTriangleIndex<%i,%i>(%i->%i)\n", k, l, i, redundantIndex[i]);
                                        }
                                }
                        }

                        c_redundantVertexes++;
                }
        }

#if 0
        ri.Printf(PRINT_ALL, "vertices: ");
        for(i = 0; i < numVerts; i++)
        {
                if(redundantIndex[i] != -1)
                {
                        ri.Printf(PRINT_ALL, "(%i,%i),", i, redundantIndex[i]);
                }
                else
                {
                        ri.Printf(PRINT_ALL, "(%i,-),", i);
                }
        }
        ri.Printf(PRINT_ALL, "\n");

        ri.Printf(PRINT_ALL, "triangles: ");
        for(k = 0, tri = triangles; k < numTriangles; k++, tri++)
        {
                ri.Printf(PRINT_ALL, "(%i,%i,%i),", tri->indexes[0], tri->indexes[1], tri->indexes[2]);
        }
        ri.Printf(PRINT_ALL, "\n");
#endif

        if(c_redundantVertexes)
        {
                // *numVerts -= c_redundantVertexes;

                //ri.Printf(PRINT_ALL, "removed %i redundant vertices\n", c_redundantVertexes);
        }
#endif
}

static void OptimizeTrianglesLite(const int *redundantIndex, int numTriangles, srfTriangle_t * triangles)
{
        srfTriangle_t  *tri;
        int             k, l;

        c_redundantVertexes = 0;
        for(k = 0, tri = triangles; k < numTriangles; k++, tri++)
        {
                for(l = 0; l < 3; l++)
                {
                        if(redundantIndex[tri->indexes[l]] != -1)
                        {
                                // replace duplicated index j with the original vertex index i
                                tri->indexes[l] = redundantIndex[tri->indexes[l]];

                                //ri.Printf(PRINT_ALL, "mapTriangleIndex<%i,%i>(%i->%i)\n", k, l, tri->indexes[l], redundantIndex[tri->indexes[l]]);

                                c_redundantVertexes++;
                        }
                }
        }
}

static void BuildRedundantIndices(int numVerts, const srfVert_t * verts, int *redundantIndex,
                                                                  qboolean(*CompareVert) (const srfVert_t * v1, const srfVert_t * v2))
{
        int             i, j;

        memset(redundantIndex, -1, numVerts * sizeof(int));

        for(i = 0; i < numVerts; i++)
        {
                if(redundantIndex[i] == -1)
                {
                        for(j = i + 1; j < numVerts; j++)
                        {
                                if(redundantIndex[j] != -1)
                                        continue;

                                if(CompareVert(&verts[i], &verts[j]))
                                {
                                        // mark vertex as redundant
                                        redundantIndex[j] = i;
                                }
                        }
                }
        }
}*/

/*
static void R_LoadAreaPortals(const char *bspName)
{
        int             i, j, k;
        char            fileName[MAX_QPATH];
        char           *token;
        char           *buf_p;
        byte           *buffer;
        int             bufferLen;
        const char     *version = "AREAPRT1";
        int             numAreas;
        int             numAreaPortals;
        int             numPoints;

        bspAreaPortal_t *ap;

        Q_strncpyz(fileName, bspName, sizeof(fileName));
        COM_StripExtension3(fileName, fileName, sizeof(fileName));
        Q_strcat(fileName, sizeof(fileName), ".areaprt");

        bufferLen = ri.FS_ReadFile(fileName, (void **)&buffer);
        if(!buffer)
        {
                ri.Printf(PRINT_WARNING, "WARNING: could not load area portals file '%s'\n", fileName);
                return;
        }

        buf_p = (char *)buffer;

        // check version
        token = COM_ParseExt2(&buf_p, qfalse);
        if(strcmp(token, version))
        {
                ri.Printf(PRINT_WARNING, "R_LoadAreaPortals: %s has wrong version (%i should be %i)\n", fileName, token, version);
                return;
        }

        // load areas num
        token = COM_ParseExt2(&buf_p, qtrue);
        numAreas = atoi(token);
        if(numAreas != s_worldData.numAreas)
        {
                ri.Printf(PRINT_WARNING, "R_LoadAreaPortals: %s has wrong number of areas (%i should be %i)\n", fileName, numAreas,
                                  s_worldData.numAreas);
                return;
        }

        // load areas portals
        token = COM_ParseExt2(&buf_p, qtrue);
        numAreaPortals = atoi(token);

        ri.Printf(PRINT_ALL, "...loading %i area portals\n", numAreaPortals);

        s_worldData.numAreaPortals = numAreaPortals;
        s_worldData.areaPortals = ri.Hunk_Alloc(numAreaPortals * sizeof(*s_worldData.areaPortals), h_low);

        for(i = 0, ap = s_worldData.areaPortals; i < numAreaPortals; i++, ap++)
        {
                token = COM_ParseExt2(&buf_p, qtrue);
                numPoints = atoi(token);

                if(numPoints != 4)
                {
                        ri.Printf(PRINT_WARNING, "R_LoadAreaPortals: expected 4 found '%s' in file '%s'\n", token, fileName);
                        return;
                }

                COM_ParseExt2(&buf_p, qfalse);
                ap->areas[0] = atoi(token);

                COM_ParseExt2(&buf_p, qfalse);
                ap->areas[1] = atoi(token);

                for(j = 0; j < numPoints; j++)
                {
                        // skip (
                        token = COM_ParseExt2(&buf_p, qfalse);
                        if(Q_stricmp(token, "("))
                        {
                                ri.Printf(PRINT_WARNING, "R_LoadAreaPortals: expected '(' found '%s' in file '%s'\n", token, fileName);
                                return;
                        }

                        for(k = 0; k < 3; k++)
                        {
                                token = COM_ParseExt2(&buf_p, qfalse);
                                ap->points[j][k] = atof(token);
                        }

                        // skip )
                        token = COM_ParseExt2(&buf_p, qfalse);
                        if(Q_stricmp(token, ")"))
                        {
                                ri.Printf(PRINT_WARNING, "R_LoadAreaPortals: expected ')' found '%s' in file '%s'\n", token, fileName);
                                return;
                        }
                }
        }
}
*/

/*
=================
R_CreateAreas
=================
*/

/*
static void R_CreateAreas()
{
        int             i, j;
        int             numAreas, maxArea;
        bspNode_t      *node;
        bspArea_t      *area;
        growList_t      areaSurfaces;
        int             c;
        bspSurface_t   *surface, **mark;
        int             surfaceNum;


        ri.Printf(PRINT_ALL, "...creating BSP areas\n");

        // go through the leaves and count areas
        maxArea = 0;

        for(i = 0, node = s_worldData.nodes; i < s_worldData.numnodes; i++, node++)
        {
                if(node->contents == CONTENTS_NODE)
                        continue;

                if(node->area == -1)
                        continue;

                if(node->area > maxArea)
                        maxArea = node->area;
        }

        numAreas = maxArea + 1;

        s_worldData.numAreas = numAreas;
        s_worldData.areas = ri.Hunk_Alloc(numAreas * sizeof(*s_worldData.areas), h_low);

        // reset surfaces' viewCount
        for(i = 0, surface = s_worldData.surfaces; i < s_worldData.numSurfaces; i++, surface++)
        {
                surface->viewCount = -1;
        }

        // add area surfaces
        for(i = 0; i < numAreas; i++)
        {
                area = &s_worldData.areas[i];

                Com_InitGrowList(&areaSurfaces, 100);

                for(j = 0, node = s_worldData.nodes; j < s_worldData.numnodes; j++, node++)
                {
                        if(node->contents == CONTENTS_NODE)
                                continue;

                        if(node->area != i)
                                continue;

                        if(node->cluster < 0)
                                continue;

                        // add the individual surfaces
                        mark = node->markSurfaces;
                        c = node->numMarkSurfaces;
                        while(c--)
                        {
                                // the surface may have already been added if it
                                // spans multiple leafs
                                surface = *mark;

                                surfaceNum = surface - s_worldData.surfaces;

                                if((surface->viewCount != (i + 1)) && (surfaceNum < s_worldData.numWorldSurfaces))
                                {
                                        surface->viewCount = i + 1;
                                        Com_AddToGrowList(&areaSurfaces, surface);
                                }

                                mark++;
                        }
                }

                // move area surfaces list to hunk
                area->numMarkSurfaces = areaSurfaces.currentElements;
                area->markSurfaces = ri.Hunk_Alloc(area->numMarkSurfaces * sizeof(*area->markSurfaces), h_low);

                for(j = 0; j < area->numMarkSurfaces; j++)
                {
                        area->markSurfaces[j] = (bspSurface_t *) Com_GrowListElement(&areaSurfaces, j);
                }

                Com_DestroyGrowList(&areaSurfaces);

                ri.Printf(PRINT_ALL, "area %i contains %i bsp surfaces\n", i, area->numMarkSurfaces);
        }

        ri.Printf(PRINT_ALL, "%i world areas created\n", numAreas);
}
*/

/*
===============
R_CreateVBOWorldSurfaces
===============
*/

/*
static void R_CreateVBOWorldSurfaces()
{
        int             i, j, k, l, a;

        int             numVerts;
        srfVert_t      *verts;

        srfVert_t      *optimizedVerts;

        int             numTriangles;
        srfTriangle_t  *triangles;

        shader_t       *shader, *oldShader;
        int             lightmapNum, oldLightmapNum;

        int             numSurfaces;
        bspSurface_t   *surface, *surface2;
        bspSurface_t  **surfacesSorted;

        bspArea_t      *area;

        growList_t      vboSurfaces;
        srfVBOMesh_t   *vboSurf;

        if(!glConfig.vertexBufferObjectAvailable)
                return;

        for(a = 0, area = s_worldData.areas; a < s_worldData.numAreas; a++, area++)
        {
                // count number of static area surfaces
                numSurfaces = 0;
                for(k = 0; k < area->numMarkSurfaces; k++)
                {
                        surface = area->markSurfaces[k];
                        shader = surface->shader;

                        if(shader->isSky)
                                continue;

                        if(shader->isPortal || shader->isMirror)
                                continue;

                        if(shader->numDeforms)
                                continue;

                        numSurfaces++;
                }

                if(!numSurfaces)
                        continue;

                // build interaction caches list
                surfacesSorted = ri.Hunk_AllocateTempMemory(numSurfaces * sizeof(surfacesSorted[0]));

                numSurfaces = 0;
                for(k = 0; k < area->numMarkSurfaces; k++)
                {
                        surface = area->markSurfaces[k];
                        shader = surface->shader;

                        if(shader->isSky)
                                continue;

                        if(shader->isPortal || shader->isMirror)
                                continue;

                        if(shader->numDeforms)
                                continue;

                        surfacesSorted[numSurfaces] = surface;
                        numSurfaces++;
                }

                Com_InitGrowList(&vboSurfaces, 100);

                // sort surfaces by shader
                qsort(surfacesSorted, numSurfaces, sizeof(surfacesSorted), BSPSurfaceCompare);

                // create a VBO for each shader
                shader = oldShader = NULL;
                lightmapNum = oldLightmapNum = -1;

                for(k = 0; k < numSurfaces; k++)
                {
                        surface = surfacesSorted[k];
                        shader = surface->shader;
                        lightmapNum = surface->lightmapNum;

                        if(shader != oldShader || (r_precomputedLighting->integer ? lightmapNum != oldLightmapNum : 0))
                        {
                                oldShader = shader;
                                oldLightmapNum = lightmapNum;

                                // count vertices and indices
                                numVerts = 0;
                                numTriangles = 0;

                                for(l = k; l < numSurfaces; l++)
                                {
                                        surface2 = surfacesSorted[l];

                                        if(surface2->shader != shader)
                                                continue;

                                        if(*surface2->data == SF_FACE)
                                        {
                                                srfSurfaceFace_t *face = (srfSurfaceFace_t *) surface2->data;

                                                if(face->numVerts)
                                                        numVerts += face->numVerts;

                                                if(face->numTriangles)
                                                        numTriangles += face->numTriangles;
                                        }
                                        else if(*surface2->data == SF_GRID)
                                        {
                                                srfGridMesh_t  *grid = (srfGridMesh_t *) surface2->data;

                                                if(grid->numVerts)
                                                        numVerts += grid->numVerts;

                                                if(grid->numTriangles)
                                                        numTriangles += grid->numTriangles;
                                        }
                                        else if(*surface2->data == SF_TRIANGLES)
                                        {
                                                srfTriangles_t *tri = (srfTriangles_t *) surface2->data;

                                                if(tri->numVerts)
                                                        numVerts += tri->numVerts;

                                                if(tri->numTriangles)
                                                        numTriangles += tri->numTriangles;
                                        }
                                }

                                if(!numVerts || !numTriangles)
                                        continue;

                                ri.Printf(PRINT_DEVELOPER, "...calculating world mesh VBOs ( %s, %i verts %i tris )\n", shader->name, numVerts,
                                                  numTriangles);

                                // create surface
                                vboSurf = ri.Hunk_Alloc(sizeof(*vboSurf), h_low);
                                Com_AddToGrowList(&vboSurfaces, vboSurf);

                                vboSurf->surfaceType = SF_VBO_MESH;
                                vboSurf->numIndexes = numTriangles * 3;
                                vboSurf->numVerts = numVerts;

                                vboSurf->shader = shader;
                                vboSurf->lightmapNum = lightmapNum;

                                // create arrays
                                verts = ri.Hunk_AllocateTempMemory(numVerts * sizeof(srfVert_t));
                                optimizedVerts = ri.Hunk_AllocateTempMemory(numVerts * sizeof(srfVert_t));
                                numVerts = 0;

                                triangles = ri.Hunk_AllocateTempMemory(numTriangles * sizeof(srfTriangle_t));
                                numTriangles = 0;

                                ClearBounds(vboSurf->bounds[0], vboSurf->bounds[1]);

                                // build triangle indices
                                for(l = k; l < numSurfaces; l++)
                                {
                                        surface2 = surfacesSorted[l];

                                        if(surface2->shader != shader)
                                                continue;

                                        // set up triangle indices
                                        if(*surface2->data == SF_FACE)
                                        {
                                                srfSurfaceFace_t *srf = (srfSurfaceFace_t *) surface2->data;

                                                if(srf->numTriangles)
                                                {
                                                        srfTriangle_t  *tri;

                                                        for(i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
                                                        {
                                                                for(j = 0; j < 3; j++)
                                                                {
                                                                        triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
                                                                }
                                                        }

                                                        numTriangles += srf->numTriangles;
                                                }

                                                if(srf->numVerts)
                                                        numVerts += srf->numVerts;
                                        }
                                        else if(*surface2->data == SF_GRID)
                                        {
                                                srfGridMesh_t  *srf = (srfGridMesh_t *) surface2->data;

                                                if(srf->numTriangles)
                                                {
                                                        srfTriangle_t  *tri;

                                                        for(i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
                                                        {
                                                                for(j = 0; j < 3; j++)
                                                                {
                                                                        triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
                                                                }
                                                        }

                                                        numTriangles += srf->numTriangles;
                                                }

                                                if(srf->numVerts)
                                                        numVerts += srf->numVerts;
                                        }
                                        else if(*surface2->data == SF_TRIANGLES)
                                        {
                                                srfTriangles_t *srf = (srfTriangles_t *) surface2->data;

                                                if(srf->numTriangles)
                                                {
                                                        srfTriangle_t  *tri;

                                                        for(i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++)
                                                        {
                                                                for(j = 0; j < 3; j++)
                                                                {
                                                                        triangles[numTriangles + i].indexes[j] = numVerts + tri->indexes[j];
                                                                }
                                                        }

                                                        numTriangles += srf->numTriangles;
                                                }

                                                if(srf->numVerts)
                                                        numVerts += srf->numVerts;
                                        }
                                }

                                // build vertices
                                numVerts = 0;
                                for(l = k; l < numSurfaces; l++)
                                {
                                        surface2 = surfacesSorted[l];

                                        if(surface2->shader != shader)
                                                continue;

                                        if(*surface2->data == SF_FACE)
                                        {
                                                srfSurfaceFace_t *cv = (srfSurfaceFace_t *) surface2->data;

                                                if(cv->numVerts)
                                                {
                                                        for(i = 0; i < cv->numVerts; i++)
                                                        {
                                                                CopyVert(&cv->verts[i], &verts[numVerts + i]);

                                                                AddPointToBounds(cv->verts[i].xyz, vboSurf->bounds[0], vboSurf->bounds[1]);
                                                        }

                                                        numVerts += cv->numVerts;
                                                }
                                        }
                                        else if(*surface2->data == SF_GRID)
                                        {
                                                srfGridMesh_t  *cv = (srfGridMesh_t *) surface2->data;

                                                if(cv->numVerts)
                                                {
                                                        for(i = 0; i < cv->numVerts; i++)
                                                        {
                                                                CopyVert(&cv->verts[i], &verts[numVerts + i]);

                                                                AddPointToBounds(cv->verts[i].xyz, vboSurf->bounds[0], vboSurf->bounds[1]);
                                                        }

                                                        numVerts += cv->numVerts;
                                                }
                                        }
                                        else if(*surface2->data == SF_TRIANGLES)
                                        {
                                                srfTriangles_t *cv = (srfTriangles_t *) surface2->data;

                                                if(cv->numVerts)
                                                {
                                                        for(i = 0; i < cv->numVerts; i++)
                                                        {
                                                                CopyVert(&cv->verts[i], &verts[numVerts + i]);

                                                                AddPointToBounds(cv->verts[i].xyz, vboSurf->bounds[0], vboSurf->bounds[1]);
                                                        }

                                                        numVerts += cv->numVerts;
                                                }
                                        }
                                }

                                numVerts = OptimizeVertices(numVerts, verts, numTriangles, triangles, optimizedVerts, CompareWorldVert);
                                if(c_redundantVertexes)
                                {
                                        ri.Printf(PRINT_DEVELOPER,
                                                          "...removed %i redundant vertices from staticWorldMesh %i ( %s, %i verts %i tris )\n",
                                                          c_redundantVertexes, vboSurfaces.currentElements, shader->name, numVerts, numTriangles);
                                }

                                vboSurf->vbo = R_CreateVBO2(va("staticWorldMesh_vertices %i", vboSurfaces.currentElements), numVerts, optimizedVerts,
                                                                           ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_TANGENT | ATTR_BINORMAL | ATTR_NORMAL
                                                                           | ATTR_COLOR);

                                vboSurf->ibo = R_CreateIBO2(va("staticWorldMesh_indices %i", vboSurfaces.currentElements), numTriangles, triangles);

                                ri.Hunk_FreeTempMemory(triangles);
                                ri.Hunk_FreeTempMemory(optimizedVerts);
                                ri.Hunk_FreeTempMemory(verts);
                        }
                }

                ri.Hunk_FreeTempMemory(surfacesSorted);

                // move VBO surfaces list to hunk
                area->numVBOSurfaces = vboSurfaces.currentElements;
                area->vboSurfaces = ri.Hunk_Alloc(area->numVBOSurfaces * sizeof(*area->vboSurfaces), h_low);

                for(i = 0; i < area->numVBOSurfaces; i++)
                {
                        area->vboSurfaces[i] = (srfVBOMesh_t *) Com_GrowListElement(&vboSurfaces, i);
                }

                Com_DestroyGrowList(&vboSurfaces);

                ri.Printf(PRINT_ALL, "%i VBO surfaces created for area %i\n", area->numVBOSurfaces, a);
        }
}
*/

/*
=================
R_CreateClusters
=================
*/
static void R_CreateClusters()
{
	int          i, j;
	bspSurface_t *surface;
	bspNode_t    *node;
#if defined( USE_BSP_CLUSTERSURFACE_MERGING )
	int          numClusters;
	bspCluster_t *cluster;
	growList_t   clusterSurfaces;
	const byte   *vis;
	int          c;
	int          surfaceNum;
	vec3_t       mins, maxs;

	ri.Printf( PRINT_ALL, "...creating BSP clusters\n" );

	if ( s_worldData.vis )
	{
		// go through the leaves and count clusters
		numClusters = 0;

		for ( i = 0, node = s_worldData.nodes; i < s_worldData.numnodes; i++, node++ )
		{
			if ( node->cluster >= numClusters )
			{
				numClusters = node->cluster;
			}
		}

		numClusters++;

		s_worldData.numClusters = numClusters;
		s_worldData.clusters = ri.Hunk_Alloc( ( numClusters + 1 ) * sizeof( *s_worldData.clusters ), h_low );   // + supercluster

		// reset surfaces' viewCount
		for ( i = 0, surface = s_worldData.surfaces; i < s_worldData.numSurfaces; i++, surface++ )
		{
			surface->viewCount = -1;
		}

		for ( j = 0, node = s_worldData.nodes; j < s_worldData.numnodes; j++, node++ )
		{
			node->visCounts[ 0 ] = -1;
		}

		for ( i = 0; i < numClusters; i++ )
		{
			cluster = &s_worldData.clusters[ i ];

			// mark leaves in cluster
			vis = s_worldData.vis + i * s_worldData.clusterBytes;

			for ( j = 0, node = s_worldData.nodes; j < s_worldData.numnodes; j++, node++ )
			{
				if ( node->cluster < 0 || node->cluster >= numClusters )
				{
					continue;
				}

				// check general pvs
				if ( !( vis[ node->cluster >> 3 ] & ( 1 << ( node->cluster & 7 ) ) ) )
				{
					continue;
				}

				parent = node;

				do
				{
					if ( parent->visCounts[ 0 ] == i )
					{
						break;
					}

					parent->visCounts[ 0 ] = i;
					parent = parent->parent;
				}
				while ( parent );
			}

			// add cluster surfaces
			Com_InitGrowList( &clusterSurfaces, 10000 );

			ClearBounds( mins, maxs );

			for ( j = 0, node = s_worldData.nodes; j < s_worldData.numnodes; j++, node++ )
			{
				if ( node->contents == CONTENTS_NODE )
				{
					continue;
				}

				if ( node->visCounts[ 0 ] != i )
				{
					continue;
				}

				BoundsAdd( mins, maxs, node->mins, node->maxs );

				mark = node->markSurfaces;
				c = node->numMarkSurfaces;

				while ( c-- )
				{
					// the surface may have already been added if it
					// spans multiple leafs
					surface = *mark;

					surfaceNum = surface - s_worldData.surfaces;

					if ( ( surface->viewCount != i ) && ( surfaceNum < s_worldData.numWorldSurfaces ) )
					{
						surface->viewCount = i;
						Com_AddToGrowList( &clusterSurfaces, surface );
					}

					mark++;
				}
			}

			cluster->origin[ 0 ] = ( mins[ 0 ] + maxs[ 0 ] ) / 2;
			cluster->origin[ 1 ] = ( mins[ 1 ] + maxs[ 1 ] ) / 2;
			cluster->origin[ 2 ] = ( mins[ 2 ] + maxs[ 2 ] ) / 2;

			//ri.Printf(PRINT_ALL, "cluster %i origin at (%i %i %i)\n", i, (int)cluster->origin[0], (int)cluster->origin[1], (int)cluster->origin[2]);

			// move cluster surfaces list to hunk
			cluster->numMarkSurfaces = clusterSurfaces.currentElements;
			cluster->markSurfaces = ri.Hunk_Alloc( cluster->numMarkSurfaces * sizeof( *cluster->markSurfaces ), h_low );

			for ( j = 0; j < cluster->numMarkSurfaces; j++ )
			{
				cluster->markSurfaces[ j ] = ( bspSurface_t * ) Com_GrowListElement( &clusterSurfaces, j );
			}

			Com_DestroyGrowList( &clusterSurfaces );

			//ri.Printf(PRINT_ALL, "cluster %i contains %i bsp surfaces\n", i, cluster->numMarkSurfaces);
		}
	}
	else
	{
		numClusters = 0;

		s_worldData.numClusters = numClusters;
		s_worldData.clusters = ri.Hunk_Alloc( ( numClusters + 1 ) * sizeof( *s_worldData.clusters ), h_low );   // + supercluster
	}

	// create a super cluster that will be always used when no view cluster can be found
	Com_InitGrowList( &clusterSurfaces, 10000 );

	for ( i = 0, surface = s_worldData.surfaces; i < s_worldData.numWorldSurfaces; i++, surface++ )
	{
		Com_AddToGrowList( &clusterSurfaces, surface );
	}

	cluster = &s_worldData.clusters[ numClusters ];
	cluster->numMarkSurfaces = clusterSurfaces.currentElements;
	cluster->markSurfaces = ri.Hunk_Alloc( cluster->numMarkSurfaces * sizeof( *cluster->markSurfaces ), h_low );

	for ( j = 0; j < cluster->numMarkSurfaces; j++ )
	{
		cluster->markSurfaces[ j ] = ( bspSurface_t * ) Com_GrowListElement( &clusterSurfaces, j );
	}

	Com_DestroyGrowList( &clusterSurfaces );

	for ( i = 0; i < MAX_VISCOUNTS; i++ )
	{
		Com_InitGrowList( &s_worldData.clusterVBOSurfaces[ i ], 100 );
	}

	//ri.Printf(PRINT_ALL, "noVis cluster contains %i bsp surfaces\n", cluster->numMarkSurfaces);

	ri.Printf( PRINT_ALL, "%i world clusters created\n", numClusters + 1 );
#endif // #if defined(USE_BSP_CLUSTERSURFACE_MERGING)

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
#if 0
#define MAX_SAMPLES          256
#define THETA_EPSILON        0.000001
#define EQUAL_NORMAL_EPSILON 0.01

void SmoothNormals( const char *name, srfVert_t *verts, int numTotalVerts )
{
	int       i, j, k, f, cs, numVerts, numVotes, fOld, startTime, endTime;
	float     shadeAngle, defaultShadeAngle, maxShadeAngle, dot, testAngle;

//  shaderInfo_t   *si;
	float     *shadeAngles;
	byte      *smoothed;
	vec3_t    average, diff;
	int       indexes[ MAX_SAMPLES ];
	vec3_t    votes[ MAX_SAMPLES ];

	srfVert_t *yDrawVerts;

	ri.Printf( PRINT_ALL, "smoothing normals for mesh '%s'\n", name );

	yDrawVerts = Com_Allocate( numTotalVerts * sizeof( srfVert_t ) );
	memcpy( yDrawVerts, verts, numTotalVerts * sizeof( srfVert_t ) );

	// allocate shade angle table
	shadeAngles = Com_Allocate( numTotalVerts * sizeof( float ) );
	memset( shadeAngles, 0, numTotalVerts * sizeof( float ) );

	// allocate smoothed table
	cs = ( numTotalVerts / 8 ) + 1;
	smoothed = Com_Allocate( cs );
	memset( smoothed, 0, cs );

	// set default shade angle
	defaultShadeAngle = DEG2RAD( 179 );
	maxShadeAngle = defaultShadeAngle;

	// run through every surface and flag verts belonging to non-lightmapped surfaces
	//   and set per-vertex smoothing angle

	/*
	   for(i = 0; i < numBSPDrawSurfaces; i++)
	   {
	   // get drawsurf
	   ds = &bspDrawSurfaces[i];

	   // get shader for shade angle
	   si = surfaceInfos[i].si;
	   if(si->shadeAngleDegrees)
	   shadeAngle = DEG2RAD(si->shadeAngleDegrees);
	   else
	   shadeAngle = defaultShadeAngle;
	   if(shadeAngle > maxShadeAngle)
	   maxShadeAngle = shadeAngle;

	   // flag its verts
	   for(j = 0; j < ds->numVerts; j++)
	   {
	   f = ds->firstVert + j;
	   shadeAngles[f] = shadeAngle;
	   if(ds->surfaceType == MST_TRIANGLE_SOUP)
	   smoothed[f >> 3] |= (1 << (f & 7));
	   }
	   }

	   // bail if no surfaces have a shade angle
	   if(maxShadeAngle == 0)
	   {
	   Com_Dealloc(shadeAngles);
	   Com_Dealloc(smoothed);
	   return;
	   }
	 */

	// init pacifier
	fOld = -1;
	startTime = ri.Milliseconds();

	// go through the list of vertexes
	for ( i = 0; i < numTotalVerts; i++ )
	{
		// print pacifier
		f = 10 * i / numTotalVerts;

		if ( f != fOld )
		{
			fOld = f;
			ri.Printf( PRINT_ALL, "%i...", f );
		}

		// already smoothed?
		if ( smoothed[ i >> 3 ] & ( 1 << ( i & 7 ) ) )
		{
			continue;
		}

		// clear
		VectorClear( average );
		numVerts = 0;
		numVotes = 0;

		// build a table of coincident vertexes
		for ( j = i; j < numTotalVerts && numVerts < MAX_SAMPLES; j++ )
		{
			// already smoothed?
			if ( smoothed[ j >> 3 ] & ( 1 << ( j & 7 ) ) )
			{
				continue;
			}

			// test vertexes
			if ( CompareWorldVertSmoothNormal( &yDrawVerts[ i ], &yDrawVerts[ j ] ) == qfalse )
			{
				continue;
			}

			// use smallest shade angle
			//shadeAngle = (shadeAngles[i] < shadeAngles[j] ? shadeAngles[i] : shadeAngles[j]);
			shadeAngle = maxShadeAngle;

			// check shade angle
			dot = DotProduct( verts[ i ].normal, verts[ j ].normal );

			if ( dot > 1.0 )
			{
				dot = 1.0;
			}
			else if ( dot < -1.0 )
			{
				dot = -1.0;
			}

			testAngle = acos( dot ) + THETA_EPSILON;

			if ( testAngle >= shadeAngle )
			{
				//Sys_Printf( "F(%3.3f >= %3.3f) ", RAD2DEG( testAngle ), RAD2DEG( shadeAngle ) );
				continue;
			}

			//Sys_Printf( "P(%3.3f < %3.3f) ", RAD2DEG( testAngle ), RAD2DEG( shadeAngle ) );

			// add to the list
			indexes[ numVerts++ ] = j;

			// flag vertex
			smoothed[ j >> 3 ] |= ( 1 << ( j & 7 ) );

			// see if this normal has already been voted
			for ( k = 0; k < numVotes; k++ )
			{
				VectorSubtract( verts[ j ].normal, votes[ k ], diff );

				if ( fabs( diff[ 0 ] ) < EQUAL_NORMAL_EPSILON &&
				     fabs( diff[ 1 ] ) < EQUAL_NORMAL_EPSILON && fabs( diff[ 2 ] ) < EQUAL_NORMAL_EPSILON )
				{
					break;
				}
			}

			// add a new vote?
			if ( k == numVotes && numVotes < MAX_SAMPLES )
			{
				VectorAdd( average, verts[ j ].normal, average );
				VectorCopy( verts[ j ].normal, votes[ numVotes ] );
				numVotes++;
			}
		}

		// don't average for less than 2 verts
		if ( numVerts < 2 )
		{
			continue;
		}

		// average normal
		if ( VectorNormalize( average ) > 0 )
		{
			// smooth
			for ( j = 0; j < numVerts; j++ )
			{
				VectorCopy( average, yDrawVerts[ indexes[ j ] ].normal );
			}
		}
	}

	// copy yDrawVerts normals back
	for ( i = 0; i < numTotalVerts; i++ )
	{
		VectorCopy( yDrawVerts[ i ].normal, verts[ i ].normal );
	}

	// free the tables
	Com_Dealloc( yDrawVerts );
	Com_Dealloc( shadeAngles );
	Com_Dealloc( smoothed );

	endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, " (%5.2f seconds)\n", ( endTime - startTime ) / 1000.0 );
}

#endif

/*
===============
R_CreateWorldVBO
===============
*/
static void R_CreateWorldVBO()
{
	int       i, j, k;

	int       numVerts;
	srfVert_t *verts;

//  srfVert_t      *optimizedVerts;

	int           numTriangles;
	srfTriangle_t *triangles;

//  int             numSurfaces;
	bspSurface_t  *surface;

//	trRefLight_t   *light;

	int startTime, endTime;

	startTime = ri.Milliseconds();

	numVerts = 0;
	numTriangles = 0;

	for ( k = 0, surface = &s_worldData.surfaces[ 0 ]; k < s_worldData.numWorldSurfaces; k++, surface++ )
	{
		if ( *surface->data == SF_FACE )
		{
			srfSurfaceFace_t *face = ( srfSurfaceFace_t * ) surface->data;

			if ( face->numVerts )
			{
				numVerts += face->numVerts;
			}

			if ( face->numTriangles )
			{
				numTriangles += face->numTriangles;
			}
		}
		else if ( *surface->data == SF_GRID )
		{
			srfGridMesh_t *grid = ( srfGridMesh_t * ) surface->data;

			if ( grid->numVerts )
			{
				numVerts += grid->numVerts;
			}

			if ( grid->numTriangles )
			{
				numTriangles += grid->numTriangles;
			}
		}
		else if ( *surface->data == SF_TRIANGLES )
		{
			srfTriangles_t *tri = ( srfTriangles_t * ) surface->data;

			if ( tri->numVerts )
			{
				numVerts += tri->numVerts;
			}

			if ( tri->numTriangles )
			{
				numTriangles += tri->numTriangles;
			}
		}
	}

	if ( !numVerts || !numTriangles )
	{
		return;
	}

	ri.Printf( PRINT_ALL, "...calculating world VBO ( %i verts %i tris )\n", numVerts, numTriangles );

	// create arrays

	s_worldData.numVerts = numVerts;
	s_worldData.verts = verts = ri.Hunk_Alloc( numVerts * sizeof( srfVert_t ), h_low );
	//optimizedVerts = ri.Hunk_AllocateTempMemory(numVerts * sizeof(srfVert_t));

	s_worldData.numTriangles = numTriangles;
	s_worldData.triangles = triangles = ri.Hunk_Alloc( numTriangles * sizeof( srfTriangle_t ), h_low );

	// set up triangle indices
	numVerts = 0;
	numTriangles = 0;

	for ( k = 0, surface = &s_worldData.surfaces[ 0 ]; k < s_worldData.numWorldSurfaces; k++, surface++ )
	{
		if ( *surface->data == SF_FACE )
		{
			srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface->data;

			srf->firstTriangle = numTriangles;

			if ( srf->numTriangles )
			{
				srfTriangle_t *tri;

				for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
				{
					for ( j = 0; j < 3; j++ )
					{
						triangles[ numTriangles + i ].indexes[ j ] = numVerts + tri->indexes[ j ];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if ( srf->numVerts )
			{
				numVerts += srf->numVerts;
			}
		}
		else if ( *surface->data == SF_GRID )
		{
			srfGridMesh_t *srf = ( srfGridMesh_t * ) surface->data;

			srf->firstTriangle = numTriangles;

			if ( srf->numTriangles )
			{
				srfTriangle_t *tri;

				for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
				{
					for ( j = 0; j < 3; j++ )
					{
						triangles[ numTriangles + i ].indexes[ j ] = numVerts + tri->indexes[ j ];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if ( srf->numVerts )
			{
				numVerts += srf->numVerts;
			}
		}
		else if ( *surface->data == SF_TRIANGLES )
		{
			srfTriangles_t *srf = ( srfTriangles_t * ) surface->data;

			srf->firstTriangle = numTriangles;

			if ( srf->numTriangles )
			{
				srfTriangle_t *tri;

				for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
				{
					for ( j = 0; j < 3; j++ )
					{
						triangles[ numTriangles + i ].indexes[ j ] = numVerts + tri->indexes[ j ];
					}
				}

				numTriangles += srf->numTriangles;
			}

			if ( srf->numVerts )
			{
				numVerts += srf->numVerts;
			}
		}
	}

	// build vertices
	numVerts = 0;

	for ( k = 0, surface = &s_worldData.surfaces[ 0 ]; k < s_worldData.numWorldSurfaces; k++, surface++ )
	{
		if ( *surface->data == SF_FACE )
		{
			srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface->data;

			srf->firstVert = numVerts;

			if ( srf->numVerts )
			{
				for ( i = 0; i < srf->numVerts; i++ )
				{
					CopyVert( &srf->verts[ i ], &verts[ numVerts + i ] );
				}

				numVerts += srf->numVerts;
			}
		}
		else if ( *surface->data == SF_GRID )
		{
			srfGridMesh_t *srf = ( srfGridMesh_t * ) surface->data;

			srf->firstVert = numVerts;

			if ( srf->numVerts )
			{
				for ( i = 0; i < srf->numVerts; i++ )
				{
					CopyVert( &srf->verts[ i ], &verts[ numVerts + i ] );
				}

				numVerts += srf->numVerts;
			}
		}
		else if ( *surface->data == SF_TRIANGLES )
		{
			srfTriangles_t *srf = ( srfTriangles_t * ) surface->data;

			srf->firstVert = numVerts;

			if ( srf->numVerts )
			{
				for ( i = 0; i < srf->numVerts; i++ )
				{
					CopyVert( &srf->verts[ i ], &verts[ numVerts + i ] );
				}

				numVerts += srf->numVerts;
			}
		}
	}

#if 0
	numVerts = OptimizeVertices( numVerts, verts, numTriangles, triangles, optimizedVerts, CompareWorldVert );

	if ( c_redundantVertexes )
	{
		ri.Printf( PRINT_DEVELOPER,
		           "...removed %i redundant vertices from staticWorldMesh %i ( %s, %i verts %i tris )\n",
		           c_redundantVertexes, vboSurfaces.currentElements, shader->name, numVerts, numTriangles );
	}

	s_worldData.vbo = R_CreateVBO2( va( "bspModelMesh_vertices %i", 0 ), numVerts, optimizedVerts,
	                                ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_TANGENT | ATTR_BINORMAL |
	                                ATTR_NORMAL | ATTR_COLOR | GLCS_LIGHTCOLOR | ATTR_LIGHTDIRECTION );
#else
	s_worldData.vbo = R_CreateVBO2( va( "staticBspModel0_VBO %i", 0 ), numVerts, verts,
	                                ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_TANGENT | ATTR_BINORMAL |
	                                ATTR_NORMAL | ATTR_COLOR
#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )
	                                | ATTR_PAINTCOLOR | ATTR_LIGHTDIRECTION
#endif
	                                , VBO_USAGE_STATIC );
#endif

	s_worldData.ibo = R_CreateIBO2( va( "staticBspModel0_IBO %i", 0 ), numTriangles, triangles, VBO_USAGE_STATIC );

	endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "world VBO calculation time = %5.2f seconds\n", ( endTime - startTime ) / 1000.0 );

	// point triangle surfaces to world VBO
	for ( k = 0, surface = &s_worldData.surfaces[ 0 ]; k < s_worldData.numWorldSurfaces; k++, surface++ )
	{
		if ( *surface->data == SF_FACE )
		{
			srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface->data;

			//if(r_vboFaces->integer && srf->numVerts && srf->numTriangles)
			{
				srf->vbo = s_worldData.vbo;
				srf->ibo = s_worldData.ibo;
				//srf->ibo = R_CreateIBO2(va("staticBspModel0_planarSurface_IBO %i", k), srf->numTriangles, triangles + srf->firstTriangle, VBO_USAGE_STATIC);
			}
		}
		else if ( *surface->data == SF_GRID )
		{
			srfGridMesh_t *srf = ( srfGridMesh_t * ) surface->data;

			//if(r_vboCurves->integer && srf->numVerts && srf->numTriangles)
			{
				srf->vbo = s_worldData.vbo;
				srf->ibo = s_worldData.ibo;
				//srf->ibo = R_CreateIBO2(va("staticBspModel0_curveSurface_IBO %i", k), srf->numTriangles, triangles + srf->firstTriangle, VBO_USAGE_STATIC);
			}
		}
		else if ( *surface->data == SF_TRIANGLES )
		{
			srfTriangles_t *srf = ( srfTriangles_t * ) surface->data;

			//if(r_vboTriangles->integer && srf->numVerts && srf->numTriangles)
			{
				srf->vbo = s_worldData.vbo;
				srf->ibo = s_worldData.ibo;
				//srf->ibo = R_CreateIBO2(va("staticBspModel0_triangleSurface_IBO %i", k), srf->numTriangles, triangles + srf->firstTriangle, VBO_USAGE_STATIC);
			}
		}
	}

	startTime = ri.Milliseconds();

	// Tr3B: FIXME move this to somewhere else?
#if CALC_REDUNDANT_SHADOWVERTS
	s_worldData.redundantVertsCalculationNeeded = 0;

	for ( i = 0; i < s_worldData.numLights; i++ )
	{
		light = &s_worldData.lights[ i ];

		if ( ( r_precomputedLighting->integer || r_vertexLighting->integer ) && !light->noRadiosity )
		{
			continue;
		}

		s_worldData.redundantVertsCalculationNeeded++;
	}

	if ( s_worldData.redundantVertsCalculationNeeded )
	{
		ri.Printf( PRINT_ALL, "...calculating redundant world vertices ( %i verts )\n", numVerts );

		s_worldData.redundantLightVerts = ri.Hunk_Alloc( numVerts * sizeof( int ), h_low );
		BuildRedundantIndices( numVerts, verts, s_worldData.redundantLightVerts, CompareLightVert );

		s_worldData.redundantShadowVerts = ri.Hunk_Alloc( numVerts * sizeof( int ), h_low );
		BuildRedundantIndices( numVerts, verts, s_worldData.redundantShadowVerts, CompareShadowVert );

		s_worldData.redundantShadowAlphaTestVerts = ri.Hunk_Alloc( numVerts * sizeof( int ), h_low );
		BuildRedundantIndices( numVerts, verts, s_worldData.redundantShadowAlphaTestVerts, CompareShadowVertAlphaTest );
	}

	endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "redundant world vertices calculation time = %5.2f seconds\n", ( endTime - startTime ) / 1000.0 );
#endif

	//ri.Hunk_FreeTempMemory(triangles);
	//ri.Hunk_FreeTempMemory(optimizedVerts);
	//ri.Hunk_FreeTempMemory(verts);
}

/*
===============
R_CreateSubModelVBOs
===============
*/
static void R_CreateSubModelVBOs()
{
	int           i, j, k, l, m;

	int           numVerts;
	srfVert_t     *verts;

	srfVert_t     *optimizedVerts;

	int           numTriangles;
	srfTriangle_t *triangles;

	shader_t      *shader, *oldShader;
	int           lightmapNum, oldLightmapNum;

	int           numSurfaces;
	bspSurface_t  *surface, *surface2;
	bspSurface_t  **surfacesSorted;

	bspModel_t    *model;

	growList_t    vboSurfaces;
	srfVBOMesh_t  *vboSurf;

	for ( m = 1, model = s_worldData.models; m < s_worldData.numModels; m++, model++ )
	{
		// count number of static area surfaces
		numSurfaces = 0;

		for ( k = 0; k < model->numSurfaces; k++ )
		{
			surface = model->firstSurface + k;
			shader = surface->shader;

			if ( shader->isSky )
			{
				continue;
			}

			if ( shader->isPortal )
			{
				continue;
			}

			if ( ShaderRequiresCPUDeforms( shader ) )
			{
				continue;
			}

			numSurfaces++;
		}

		if ( !numSurfaces )
		{
			continue;
		}

		// build interaction caches list
		surfacesSorted = ri.Hunk_AllocateTempMemory( numSurfaces * sizeof( surfacesSorted[ 0 ] ) );

		numSurfaces = 0;

		for ( k = 0; k < model->numSurfaces; k++ )
		{
			surface = model->firstSurface + k;
			shader = surface->shader;

			if ( shader->isSky )
			{
				continue;
			}

			if ( shader->isPortal )
			{
				continue;
			}

			if ( ShaderRequiresCPUDeforms( shader ) )
			{
				continue;
			}

			surfacesSorted[ numSurfaces ] = surface;
			numSurfaces++;
		}

		Com_InitGrowList( &vboSurfaces, 100 );

		// sort surfaces by shader
		qsort( surfacesSorted, numSurfaces, sizeof( surfacesSorted ), BSPSurfaceCompare );

		// create a VBO for each shader
		shader = oldShader = NULL;
		lightmapNum = oldLightmapNum = -1;

		for ( k = 0; k < numSurfaces; k++ )
		{
			surface = surfacesSorted[ k ];
			shader = surface->shader;
			lightmapNum = surface->lightmapNum;

			if ( shader != oldShader || ( r_precomputedLighting->integer ? lightmapNum != oldLightmapNum : 0 ) )
			{
				oldShader = shader;
				oldLightmapNum = lightmapNum;

				// count vertices and indices
				numVerts = 0;
				numTriangles = 0;

				for ( l = k; l < numSurfaces; l++ )
				{
					surface2 = surfacesSorted[ l ];

					if ( surface2->shader != shader )
					{
						continue;
					}

					if ( *surface2->data == SF_FACE )
					{
						srfSurfaceFace_t *face = ( srfSurfaceFace_t * ) surface2->data;

						if ( face->numVerts )
						{
							numVerts += face->numVerts;
						}

						if ( face->numTriangles )
						{
							numTriangles += face->numTriangles;
						}
					}
					else if ( *surface2->data == SF_GRID )
					{
						srfGridMesh_t *grid = ( srfGridMesh_t * ) surface2->data;

						if ( grid->numVerts )
						{
							numVerts += grid->numVerts;
						}

						if ( grid->numTriangles )
						{
							numTriangles += grid->numTriangles;
						}
					}
					else if ( *surface2->data == SF_TRIANGLES )
					{
						srfTriangles_t *tri = ( srfTriangles_t * ) surface2->data;

						if ( tri->numVerts )
						{
							numVerts += tri->numVerts;
						}

						if ( tri->numTriangles )
						{
							numTriangles += tri->numTriangles;
						}
					}
				}

				if ( !numVerts || !numTriangles )
				{
					continue;
				}

				ri.Printf( PRINT_DEVELOPER, "...calculating entity mesh VBOs ( %s, %i verts %i tris )\n", shader->name, numVerts,
				           numTriangles );

				// create surface
				vboSurf = ri.Hunk_Alloc( sizeof( *vboSurf ), h_low );
				Com_AddToGrowList( &vboSurfaces, vboSurf );

				vboSurf->surfaceType = SF_VBO_MESH;
				vboSurf->numIndexes = numTriangles * 3;
				vboSurf->numVerts = numVerts;

				vboSurf->shader = shader;
				vboSurf->lightmapNum = lightmapNum;

				// create arrays
				verts = ri.Hunk_AllocateTempMemory( numVerts * sizeof( srfVert_t ) );
				optimizedVerts = ri.Hunk_AllocateTempMemory( numVerts * sizeof( srfVert_t ) );
				numVerts = 0;

				triangles = ri.Hunk_AllocateTempMemory( numTriangles * sizeof( srfTriangle_t ) );
				numTriangles = 0;

				ClearBounds( vboSurf->bounds[ 0 ], vboSurf->bounds[ 1 ] );

				// build triangle indices
				for ( l = k; l < numSurfaces; l++ )
				{
					surface2 = surfacesSorted[ l ];

					if ( surface2->shader != shader )
					{
						continue;
					}

					// set up triangle indices
					if ( *surface2->data == SF_FACE )
					{
						srfSurfaceFace_t *srf = ( srfSurfaceFace_t * ) surface2->data;

						if ( srf->numTriangles )
						{
							srfTriangle_t *tri;

							for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
							{
								for ( j = 0; j < 3; j++ )
								{
									triangles[ numTriangles + i ].indexes[ j ] = numVerts + tri->indexes[ j ];
								}
							}

							numTriangles += srf->numTriangles;
						}

						if ( srf->numVerts )
						{
							numVerts += srf->numVerts;
						}
					}
					else if ( *surface2->data == SF_GRID )
					{
						srfGridMesh_t *srf = ( srfGridMesh_t * ) surface2->data;

						if ( srf->numTriangles )
						{
							srfTriangle_t *tri;

							for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
							{
								for ( j = 0; j < 3; j++ )
								{
									triangles[ numTriangles + i ].indexes[ j ] = numVerts + tri->indexes[ j ];
								}
							}

							numTriangles += srf->numTriangles;
						}

						if ( srf->numVerts )
						{
							numVerts += srf->numVerts;
						}
					}
					else if ( *surface2->data == SF_TRIANGLES )
					{
						srfTriangles_t *srf = ( srfTriangles_t * ) surface2->data;

						if ( srf->numTriangles )
						{
							srfTriangle_t *tri;

							for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
							{
								for ( j = 0; j < 3; j++ )
								{
									triangles[ numTriangles + i ].indexes[ j ] = numVerts + tri->indexes[ j ];
								}
							}

							numTriangles += srf->numTriangles;
						}

						if ( srf->numVerts )
						{
							numVerts += srf->numVerts;
						}
					}
				}

				// build vertices
				numVerts = 0;

				for ( l = k; l < numSurfaces; l++ )
				{
					surface2 = surfacesSorted[ l ];

					if ( surface2->shader != shader )
					{
						continue;
					}

					if ( *surface2->data == SF_FACE )
					{
						srfSurfaceFace_t *cv = ( srfSurfaceFace_t * ) surface2->data;

						if ( cv->numVerts )
						{
							for ( i = 0; i < cv->numVerts; i++ )
							{
								CopyVert( &cv->verts[ i ], &verts[ numVerts + i ] );

								AddPointToBounds( cv->verts[ i ].xyz, vboSurf->bounds[ 0 ], vboSurf->bounds[ 1 ] );
							}

							numVerts += cv->numVerts;
						}
					}
					else if ( *surface2->data == SF_GRID )
					{
						srfGridMesh_t *cv = ( srfGridMesh_t * ) surface2->data;

						if ( cv->numVerts )
						{
							for ( i = 0; i < cv->numVerts; i++ )
							{
								CopyVert( &cv->verts[ i ], &verts[ numVerts + i ] );

								AddPointToBounds( cv->verts[ i ].xyz, vboSurf->bounds[ 0 ], vboSurf->bounds[ 1 ] );
							}

							numVerts += cv->numVerts;
						}
					}
					else if ( *surface2->data == SF_TRIANGLES )
					{
						srfTriangles_t *cv = ( srfTriangles_t * ) surface2->data;

						if ( cv->numVerts )
						{
							for ( i = 0; i < cv->numVerts; i++ )
							{
								CopyVert( &cv->verts[ i ], &verts[ numVerts + i ] );

								AddPointToBounds( cv->verts[ i ].xyz, vboSurf->bounds[ 0 ], vboSurf->bounds[ 1 ] );
							}

							numVerts += cv->numVerts;
						}
					}
				}

#if 0
				numVerts = OptimizeVertices( numVerts, verts, numTriangles, triangles, optimizedVerts, CompareWorldVert );

				if ( c_redundantVertexes )
				{
					ri.Printf( PRINT_DEVELOPER,
					           "...removed %i redundant vertices from staticEntityMesh %i ( %s, %i verts %i tris )\n",
					           c_redundantVertexes, vboSurfaces.currentElements, shader->name, numVerts, numTriangles );
				}

				vboSurf->vbo =
				  R_CreateVBO2( va( "staticBspModel%i_VBO %i", m, vboSurfaces.currentElements ), numVerts, optimizedVerts,
				                ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_TANGENT | ATTR_BINORMAL | ATTR_NORMAL
				                | ATTR_COLOR | GLCS_LIGHTCOLOR | ATTR_LIGHTDIRECTION, VBO_USAGE_STATIC );
#else
				vboSurf->vbo =
				  R_CreateVBO2( va( "staticBspModel%i_VBO %i", m, vboSurfaces.currentElements ), numVerts, verts,
				                ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_TANGENT | ATTR_BINORMAL | ATTR_NORMAL
				                | ATTR_COLOR
#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )
				                | ATTR_PAINTCOLOR | ATTR_LIGHTDIRECTION
#endif
				                , VBO_USAGE_STATIC );
#endif

				vboSurf->ibo =
				  R_CreateIBO2( va( "staticBspModel%i_IBO %i", m, vboSurfaces.currentElements ), numTriangles, triangles,
				                VBO_USAGE_STATIC );

				ri.Hunk_FreeTempMemory( triangles );
				ri.Hunk_FreeTempMemory( optimizedVerts );
				ri.Hunk_FreeTempMemory( verts );
			}
		}

		ri.Hunk_FreeTempMemory( surfacesSorted );

		// move VBO surfaces list to hunk
		model->numVBOSurfaces = vboSurfaces.currentElements;
		model->vboSurfaces = ri.Hunk_Alloc( model->numVBOSurfaces * sizeof( *model->vboSurfaces ), h_low );

		for ( i = 0; i < model->numVBOSurfaces; i++ )
		{
			model->vboSurfaces[ i ] = ( srfVBOMesh_t * ) Com_GrowListElement( &vboSurfaces, i );
		}

		Com_DestroyGrowList( &vboSurfaces );

		ri.Printf( PRINT_ALL, "%i VBO surfaces created for BSP submodel %i\n", model->numVBOSurfaces, m );
	}
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

	ri.Printf( PRINT_ALL, "...loading surfaces\n" );

	numFaces = 0;
	numMeshes = 0;
	numTriSurfs = 0;
	numFlares = 0;
	numFoliages = 0;

	in = ( void * )( fileBase + surfs->fileofs );

	if ( surfs->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = surfs->filelen / sizeof( *in );

	dv = ( void * )( fileBase + verts->fileofs );

	if ( verts->filelen % sizeof( *dv ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	indexes = ( void * )( fileBase + indexLump->fileofs );

	if ( indexLump->filelen % sizeof( *indexes ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	out = ri.Hunk_Alloc( count * sizeof( *out ), h_low );

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

	ri.Printf( PRINT_ALL, "...loaded %d faces, %i meshes, %i trisurfs, %i flares %i foliages\n", numFaces, numMeshes, numTriSurfs,
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

	ri.Printf( PRINT_ALL, "...loading submodels\n" );

	in = ( void * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );

	s_worldData.numModels = count;
	s_worldData.models = out = ri.Hunk_Alloc( count * sizeof( *out ), h_low );

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

		if ( i == 0 )
		{
			// Tr3B: add this for limiting VBO surface creation
			s_worldData.numWorldSurfaces = out->numSurfaces;
		}

		// ydnar: for attaching fog brushes to models
		//out->firstBrush = LittleLong(in->firstBrush);
		//out->numBrushes = LittleLong(in->numBrushes);

		// ydnar: allocate decal memory
		j = ( i == 0 ? MAX_WORLD_DECALS : MAX_ENTITY_DECALS );
		out->decals = ri.Hunk_Alloc( j * sizeof( *out->decals ), h_low );
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
		/*
		node->sameAABBAsParent = VectorCompare(node->mins, parent->mins) && VectorCompare(node->maxs, parent->maxs);
		if(node->sameAABBAsParent)
		{
		        //ri.Printf(PRINT_ALL, "node %i has same AABB as their parent\n", node - s_worldData.nodes);
		}
		*/

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
				     gen->surfaceType != SF_GRID && gen->surfaceType != SF_TRIANGLES ) // && gen->surfaceType != SF_FOLIAGE)
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
#if 1
	AddPointToBounds( node->children[ 0 ]->surfMins, node->surfMins, node->surfMaxs );
	AddPointToBounds( node->children[ 0 ]->surfMins, node->surfMins, node->surfMaxs );
	AddPointToBounds( node->children[ 1 ]->surfMins, node->surfMins, node->surfMaxs );
	AddPointToBounds( node->children[ 1 ]->surfMaxs, node->surfMins, node->surfMaxs );
#endif
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
	srfVert_t     *verts = NULL;
	srfTriangle_t *triangles = NULL;
	IBO_t         *volumeIBO;
	vec3_t        mins, maxs;
//	vec3_t     offset = {0.01, 0.01, 0.01};

	ri.Printf( PRINT_ALL, "...loading nodes and leaves\n" );

	in = ( void * )( fileBase + nodeLump->fileofs );

	if ( nodeLump->filelen % sizeof( dnode_t ) || leafLump->filelen % sizeof( dleaf_t ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	numNodes = nodeLump->filelen / sizeof( dnode_t );
	numLeafs = leafLump->filelen / sizeof( dleaf_t );

	out = ri.Hunk_Alloc( ( numNodes + numLeafs ) * sizeof( *out ), h_low );

	s_worldData.nodes = out;
	s_worldData.numnodes = numNodes + numLeafs;
	s_worldData.numDecisionNodes = numNodes;

	// ydnar: skybox optimization
	s_worldData.numSkyNodes = 0;
	s_worldData.skyNodes = ri.Hunk_Alloc( WORLD_MAX_SKY_NODES * sizeof( *s_worldData.skyNodes ), h_low );

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
	inLeaf = ( void * )( fileBase + leafLump->fileofs );

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
		out->numMarkSurfaces = LittleLong( inLeaf->numLeafSurfaces );
	}

	// chain decendants and compute surface bounds
	R_SetParent( s_worldData.nodes, NULL );

	// calculate occlusion query volumes
	for ( j = 0, out = &s_worldData.nodes[ 0 ]; j < s_worldData.numnodes; j++, out++ )
	{
		//if(out->contents != -1 && !out->numMarkSurfaces)
		//  ri.Error(ERR_DROP, "leaf %i is empty", j);

		Com_Memset( out->lastVisited, -1, sizeof( out->lastVisited ) );
		Com_Memset( out->visible, qfalse, sizeof( out->visible ) );
		//out->occlusionQuerySamples[0] = 1;

		InitLink( &out->visChain, out );
		InitLink( &out->occlusionQuery, out );
		InitLink( &out->occlusionQuery2, out );
		//QueueInit(&node->multiQuery);

		glGenQueriesARB( MAX_VIEWS, out->occlusionQueryObjects );

		tess.multiDrawPrimitives = 0;
		tess.numIndexes = 0;
		tess.numVertexes = 0;

#if 0
		out->shrinkedAABB = qfalse;

		if ( out->contents != CONTENTS_NODE && out->numMarkSurfaces )
		{
			// BSP leaves don't have an optimal size so shrink them if possible by their surfaces
#if 1
			mins[ 0 ] = Q_min( Q_max( out->mins[ 0 ], out->surfMins[ 0 ] ), out->maxs[ 0 ] );
			mins[ 1 ] = Q_min( Q_max( out->mins[ 1 ], out->surfMins[ 1 ] ), out->maxs[ 1 ] );
			mins[ 2 ] = Q_min( Q_max( out->mins[ 2 ], out->surfMins[ 2 ] ), out->maxs[ 2 ] );

			maxs[ 0 ] = Q_max( Q_min( out->maxs[ 0 ], out->surfMaxs[ 0 ] ), out->mins[ 0 ] );
			maxs[ 1 ] = Q_max( Q_min( out->maxs[ 1 ], out->surfMaxs[ 1 ] ), out->mins[ 1 ] );
			maxs[ 2 ] = Q_max( Q_min( out->maxs[ 2 ], out->surfMaxs[ 2 ] ), out->mins[ 2 ] );
#else
			mins[ 0 ] = Q_max( out->mins[ 0 ], out->surfMins[ 0 ] );
			mins[ 1 ] = Q_max( out->mins[ 1 ], out->surfMins[ 1 ] );
			mins[ 2 ] = Q_max( out->mins[ 2 ], out->surfMins[ 2 ] );

			maxs[ 0 ] = Q_min( out->maxs[ 0 ], out->surfMaxs[ 0 ] );
			maxs[ 1 ] = Q_min( out->maxs[ 1 ], out->surfMaxs[ 1 ] );
			maxs[ 2 ] = Q_min( out->maxs[ 2 ], out->surfMaxs[ 2 ] );
#endif

			for ( i = 0; i < 3; i++ )
			{
				if ( mins[ i ] > maxs[ i ] )
				{
					float tmp = mins[ i ];
					mins[ i ] = maxs[ i ];
					maxs[ i ] = tmp;
				}
			}

			VectorCopy( out->surfMins, mins );
			VectorCopy( out->surfMaxs, maxs );

			if ( !VectorCompareEpsilon( out->mins, mins, 1.0 ) || !VectorCompareEpsilon( out->maxs, maxs, 1.0 ) )
			{
				out->shrinkedAABB = qtrue;
			}
		}
		else
#endif
		{
			VectorCopy( out->mins, mins );
			VectorCopy( out->maxs, maxs );
		}

		for ( i = 0; i < 3; i++ )
		{
			out->origin[ i ] = ( mins[ i ] + maxs[ i ] ) * 0.5f;
		}

#if 0
		// HACK: make the AABB a little bit smaller to avoid z-fighting for the occlusion queries
		VectorAdd( mins, offset, mins );
		VectorSubtract( maxs, offset, maxs );
#endif
		Tess_AddCube( vec3_origin, mins, maxs, colorWhite );

		if ( j == 0 )
		{
			verts = ri.Hunk_AllocateTempMemory( tess.numVertexes * sizeof( srfVert_t ) );
			triangles = ri.Hunk_AllocateTempMemory( ( tess.numIndexes / 3 ) * sizeof( srfTriangle_t ) );
		}

		for ( i = 0; i < tess.numVertexes; i++ )
		{
			VectorCopy( tess.xyz[ i ], verts[ i ].xyz );
		}

		for ( i = 0; i < ( tess.numIndexes / 3 ); i++ )
		{
			triangles[ i ].indexes[ 0 ] = tess.indexes[ i * 3 + 0 ];
			triangles[ i ].indexes[ 1 ] = tess.indexes[ i * 3 + 1 ];
			triangles[ i ].indexes[ 2 ] = tess.indexes[ i * 3 + 2 ];
		}

		out->volumeVBO = R_CreateVBO2( va( "staticBspNode_VBO %i", j ), tess.numVertexes, verts, ATTR_POSITION, VBO_USAGE_STATIC );

		if ( j == 0 )
		{
			out->volumeIBO = volumeIBO = R_CreateIBO2( "staticBspNode_IBO", tess.numIndexes / 3, triangles, VBO_USAGE_STATIC );
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

	if ( verts ) { ri.Hunk_FreeTempMemory( verts ); }

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

	ri.Printf( PRINT_ALL, "...loading shaders\n" );

	in = ( void * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );
	out = ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	s_worldData.shaders = out;
	s_worldData.numShaders = count;

	Com_Memcpy( out, in, count * sizeof( *out ) );

	for ( i = 0; i < count; i++ )
	{
		ri.Printf( PRINT_ALL, "shader: '%s'\n", out[ i ].shader );

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

	ri.Printf( PRINT_ALL, "...loading mark surfaces\n" );

	in = ( void * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );
	out = ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	s_worldData.markSurfaces = out;
	s_worldData.numMarkSurfaces = count;

	for ( i = 0; i < count; i++ )
	{
		j = LittleLong( in[ i ] );
		out[ i ] = s_worldData.surfaces + j;
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

	ri.Printf( PRINT_ALL, "...loading planes\n" );

	in = ( void * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *in );
	out = ri.Hunk_Alloc( count * 2 * sizeof( *out ), h_low );

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

	ri.Printf( PRINT_ALL, "...loading fogs\n" );

	fogs = ( void * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *fogs ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	count = l->filelen / sizeof( *fogs );

	// create fog strucutres for them
	s_worldData.numFogs = count + 1;
	s_worldData.fogs = ri.Hunk_Alloc( s_worldData.numFogs * sizeof( *out ), h_low );
	out = s_worldData.fogs + 1;

	// ydnar: reset global fog
	s_worldData.globalFog = -1;

	if ( !count )
	{
		ri.Printf( PRINT_ALL, "no fog volumes loaded\n" );
		return;
	}

	brushes = ( void * )( fileBase + brushesLump->fileofs );

	if ( brushesLump->filelen % sizeof( *brushes ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	brushesCount = brushesLump->filelen / sizeof( *brushes );

	sides = ( void * )( fileBase + sidesLump->fileofs );

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
		shader = R_FindShader( fogs->shader, SHADER_3D_DYNAMIC, qtrue );

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

	ri.Printf( PRINT_ALL, "%i fog volumes loaded\n", s_worldData.numFogs );
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
	bspGridPoint_t *gridPoint;
	float          lat, lng;
	int            gridStep[ 3 ];
	int            pos[ 3 ];
	float          posFloat[ 3 ];

	ri.Printf( PRINT_ALL, "...loading light grid\n" );

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

	w->numLightGridPoints = w->lightGridBounds[ 0 ] * w->lightGridBounds[ 1 ] * w->lightGridBounds[ 2 ];

	ri.Printf( PRINT_ALL, "grid size (%i %i %i)\n", ( int ) w->lightGridSize[ 0 ], ( int ) w->lightGridSize[ 1 ],
	           ( int ) w->lightGridSize[ 2 ] );
	ri.Printf( PRINT_ALL, "grid bounds (%i %i %i)\n", ( int ) w->lightGridBounds[ 0 ], ( int ) w->lightGridBounds[ 1 ],
	           ( int ) w->lightGridBounds[ 2 ] );

	if ( l->filelen != w->numLightGridPoints * sizeof( dgridPoint_t ) )
	{
		ri.Printf( PRINT_WARNING, "WARNING: light grid mismatch\n" );
		w->lightGridData = NULL;
		return;
	}

	in = ( void * )( fileBase + l->fileofs );

	if ( l->filelen % sizeof( *in ) )
	{
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s", s_worldData.name );
	}

	gridPoint = ri.Hunk_Alloc( w->numLightGridPoints * sizeof( *gridPoint ), h_low );

	w->lightGridData = gridPoint;
	//Com_Memcpy(w->lightGridData, (void *)(fileBase + l->fileofs), l->filelen);

	for ( i = 0; i < w->numLightGridPoints; i++, in++, gridPoint++ )
	{
#if defined( COMPAT_Q3A ) || defined( COMPAT_ET )
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
			gridPoint->ambientColor[ j ] = tmpAmbient[ j ] * ( 1.0f / 255.0f );
			gridPoint->directedColor[ j ] = tmpDirected[ j ] * ( 1.0f / 255.0f );
		}

#else

		for ( j = 0; j < 3; j++ )
		{
			gridPoint->ambientColor[ j ] = LittleFloat( in->ambient[ j ] );
			gridPoint->directedColor[ j ] = LittleFloat( in->directed[ j ] );
		}

#endif

		gridPoint->ambientColor[ 3 ] = 1.0f;
		gridPoint->directedColor[ 3 ] = 1.0f;

		// standard spherical coordinates to cartesian coordinates conversion

		// decode X as cos( lat ) * sin( long )
		// decode Y as sin( lat ) * sin( long )
		// decode Z as cos( long )

		// RB: having a look in NormalToLatLong used by q3map2 shows the order of latLong

		// Lat = 0 at (1,0,0) to 360 (-1,0,0), encoded in 8-bit sine table format
		// Lng = 0 at (0,0,1) to 180 (0,0,-1), encoded in 8-bit sine table format

		lat = DEG2RAD( in->latLong[ 1 ] * ( 360.0f / 255.0f ) );
		lng = DEG2RAD( in->latLong[ 0 ] * ( 360.0f / 255.0f ) );

		gridPoint->direction[ 0 ] = cos( lat ) * sin( lng );
		gridPoint->direction[ 1 ] = sin( lat ) * sin( lng );
		gridPoint->direction[ 2 ] = cos( lng );

#if 0
		// debug print to see if the XBSP format is correct
		ri.Printf( PRINT_ALL, "%9d Amb: (%03.1f %03.1f %03.1f) Dir: (%03.1f %03.1f %03.1f)\n",
		           i, gridPoint->ambient[ 0 ], gridPoint->ambient[ 1 ], gridPoint->ambient[ 2 ], gridPoint->directed[ 0 ], gridPoint->directed[ 1 ], gridPoint->directed[ 2 ] );
#endif

#if !defined( COMPAT_Q3A ) && !defined( COMPAT_ET )
		// deal with overbright bits
		R_HDRTonemapLightingColors( gridPoint->ambientColor, gridPoint->ambientColor, qtrue );
		R_HDRTonemapLightingColors( gridPoint->directedColor, gridPoint->directedColor, qtrue );
#endif
	}

	// calculate grid point positions
	gridStep[ 0 ] = 1;
	gridStep[ 1 ] = w->lightGridBounds[ 0 ];
	gridStep[ 2 ] = w->lightGridBounds[ 0 ] * w->lightGridBounds[ 1 ];

	for ( i = 0; i < w->lightGridBounds[ 0 ]; i += 1 )
	{
		for ( j = 0; j < w->lightGridBounds[ 1 ]; j += 1 )
		{
			for ( k = 0; k < w->lightGridBounds[ 2 ]; k += 1 )
			{
				pos[ 0 ] = i;
				pos[ 1 ] = j;
				pos[ 2 ] = k;

				posFloat[ 0 ] = i * w->lightGridSize[ 0 ];
				posFloat[ 1 ] = j * w->lightGridSize[ 1 ];
				posFloat[ 2 ] = k * w->lightGridSize[ 2 ];

				gridPoint = w->lightGridData + pos[ 0 ] * gridStep[ 0 ] + pos[ 1 ] * gridStep[ 1 ] + pos[ 2 ] * gridStep[ 2 ];

				VectorAdd( posFloat, w->lightGridOrigin, gridPoint->origin );
			}
		}
	}

	ri.Printf( PRINT_ALL, "%i light grid points created\n", w->numLightGridPoints );
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

	ri.Printf( PRINT_ALL, "...loading entities\n" );

	w = &s_worldData;
	w->lightGridSize[ 0 ] = 64;
	w->lightGridSize[ 1 ] = 64;
	w->lightGridSize[ 2 ] = 128;

	// store for reference by the cgame
	w->entityString = ri.Hunk_Alloc( l->filelen + 1, h_low );
	//strcpy(w->entityString, (char *)(fileBase + l->fileofs));
	Q_strncpyz( w->entityString, ( char * )( fileBase + l->fileofs ), l->filelen + 1 );
	w->entityParsePoint = w->entityString;

#if 1
	p = w->entityString;
#else
	p = ( char * )( fileBase + l->fileofs );
#endif

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
			ri.Printf( PRINT_ALL, "map features directional light mapping\n" );
			tr.worldDeluxeMapping = qtrue;
			continue;
		}

		// check for mapOverBrightBits override
		else if ( !Q_stricmp( keyname, "mapOverBrightBits" ) )
		{
			tr.mapOverBrightBits = Q_bound( 0, atof( value ), 3 );
		}

		// check for deluxe mapping provided by NetRadiant's q3map2
		if ( !Q_stricmp( keyname, "_q3map2_cmdline" ) )
		{
			s = strstr( value, "-deluxe" );

			if ( s )
			{
				ri.Printf( PRINT_ALL, "map features directional light mapping\n" );
				tr.worldDeluxeMapping = qtrue;
			}

			continue;
		}

		// check for HDR light mapping support
		if ( !Q_stricmp( keyname, "hdrRGBE" ) && !Q_stricmp( value, "1" ) )
		{
			ri.Printf( PRINT_ALL, "map features HDR light mapping\n" );
			tr.worldHDR_RGBE = qtrue;
			continue;
		}

		if ( !Q_stricmp( keyname, "classname" ) && Q_stricmp( value, "worldspawn" ) )
		{
			ri.Printf( PRINT_WARNING, "WARNING: expected worldspawn found '%s'\n", value );
			continue;
		}
	}

//  ri.Printf(PRINT_ALL, "-----------\n%s\n----------\n", p);

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

	ri.Printf( PRINT_ALL, "%i total entities counted\n", numEntities );
	ri.Printf( PRINT_ALL, "%i total lights counted\n", numLights );

	s_worldData.numLights = numLights;

	// Tr3B: FIXME add 1 dummy light so we don't trash the hunk memory system ...
	s_worldData.lights = ri.Hunk_Alloc( ( s_worldData.numLights + 1 ) * sizeof( trRefLight_t ), h_low );

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
				//COM_Parse1DMatrix(&s, 3, light->l.origin, qfalse);
			}
			// check for center
			else if ( !Q_stricmp( keyname, "light_center" ) )
			{
				sscanf( value, "%f %f %f", &light->l.center[ 0 ], &light->l.center[ 1 ], &light->l.center[ 2 ] );
				s = &value[ 0 ];
				//Com_Parse1DMatrix(&s, 3, light->l.center, qfalse);
			}
			// check for color
			else if ( !Q_stricmp( keyname, "_color" ) )
			{
				sscanf( value, "%f %f %f", &light->l.color[ 0 ], &light->l.color[ 1 ], &light->l.color[ 2 ] );
				s = &value[ 0 ];
				//Com_Parse1DMatrix(&s, 3, light->l.color, qfalse);
			}
			// check for radius
			else if ( !Q_stricmp( keyname, "light_radius" ) )
			{
				sscanf( value, "%f %f %f", &light->l.radius[ 0 ], &light->l.radius[ 1 ], &light->l.radius[ 2 ] );
				s = &value[ 0 ];
				//Com_Parse1DMatrix(&s, 3, light->l.radius, qfalse);
			}
			// check for light_target
			else if ( !Q_stricmp( keyname, "light_target" ) )
			{
				sscanf( value, "%f %f %f", &light->l.projTarget[ 0 ], &light->l.projTarget[ 1 ], &light->l.projTarget[ 2 ] );
				s = &value[ 0 ];
				//Com_Parse1DMatrix(&s, 3, light->l.projTarget, qfalse);
				light->l.rlType = RL_PROJ;
			}
			// check for light_right
			else if ( !Q_stricmp( keyname, "light_right" ) )
			{
				sscanf( value, "%f %f %f", &light->l.projRight[ 0 ], &light->l.projRight[ 1 ], &light->l.projRight[ 2 ] );
				s = &value[ 0 ];
				//Com_Parse1DMatrix(&s, 3, light->l.projRight, qfalse);
				light->l.rlType = RL_PROJ;
			}
			// check for light_up
			else if ( !Q_stricmp( keyname, "light_up" ) )
			{
				sscanf( value, "%f %f %f", &light->l.projUp[ 0 ], &light->l.projUp[ 1 ], &light->l.projUp[ 2 ] );
				s = &value[ 0 ];
				//Com_Parse1DMatrix(&s, 3, light->l.projUp, qfalse);
				light->l.rlType = RL_PROJ;
			}
			// check for light_start
			else if ( !Q_stricmp( keyname, "light_start" ) )
			{
				sscanf( value, "%f %f %f", &light->l.projStart[ 0 ], &light->l.projStart[ 1 ], &light->l.projStart[ 2 ] );
				s = &value[ 0 ];
				//Com_Parse1DMatrix(&s, 3, light->l.projStart, qfalse);
				light->l.rlType = RL_PROJ;
			}
			// check for light_end
			else if ( !Q_stricmp( keyname, "light_end" ) )
			{
				sscanf( value, "%f %f %f", &light->l.projEnd[ 0 ], &light->l.projEnd[ 1 ], &light->l.projEnd[ 2 ] );
				s = &value[ 0 ];
				//Com_Parse1DMatrix(&s, 3, light->l.projEnd, qfalse);
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

				if ( !r_hdrRendering->integer || !glConfig2.textureFloatAvailable || !glConfig2.framebufferObjectAvailable || !glConfig2.framebufferBlitAvailable )
				{
					if ( light->l.scale >= r_lightScale->value )
					{
						light->l.scale = r_lightScale->value;
					}
				}
			}
			// check for light shader
			else if ( !Q_stricmp( keyname, "texture" ) )
			{
				light->l.attenuationShader = RE_RegisterShaderLightAttenuation( value );
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

	ri.Printf( PRINT_ALL, "%i total entities parsed\n", numEntities );
	ri.Printf( PRINT_ALL, "%i total lights parsed\n", numOmniLights + numProjLights );
	ri.Printf( PRINT_ALL, "%i omni-directional lights parsed\n", numOmniLights );
	ri.Printf( PRINT_ALL, "%i projective lights parsed\n", numProjLights );
	ri.Printf( PRINT_ALL, "%i directional lights parsed\n", numParallelLights );
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

	iaCache = ri.Hunk_Alloc( sizeof( *iaCache ), h_low );
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
static int R_BuildShadowVolume(int numTriangles, const srfTriangle_t * triangles, int numVerts, int indexes[SHADER_MAX_INDEXES])
{
        int             i;
        int             numIndexes;
        const srfTriangle_t *tri;

        // calculate zfail shadow volume
        numIndexes = 0;

        // set up indices for silhouette edges
        for(i = 0, tri = triangles; i < numTriangles; i++, tri++)
        {
                if(!sh.facing[i])
                {
                        continue;
                }

                if(tri->neighbors[0] < 0 || !sh.facing[tri->neighbors[0]])
                {
                        indexes[numIndexes + 0] = tri->indexes[1];
                        indexes[numIndexes + 1] = tri->indexes[0];
                        indexes[numIndexes + 2] = tri->indexes[0] + numVerts;

                        indexes[numIndexes + 3] = tri->indexes[1];
                        indexes[numIndexes + 4] = tri->indexes[0] + numVerts;
                        indexes[numIndexes + 5] = tri->indexes[1] + numVerts;

                        numIndexes += 6;
                }

                if(tri->neighbors[1] < 0 || !sh.facing[tri->neighbors[1]])
                {
                        indexes[numIndexes + 0] = tri->indexes[2];
                        indexes[numIndexes + 1] = tri->indexes[1];
                        indexes[numIndexes + 2] = tri->indexes[1] + numVerts;

                        indexes[numIndexes + 3] = tri->indexes[2];
                        indexes[numIndexes + 4] = tri->indexes[1] + numVerts;
                        indexes[numIndexes + 5] = tri->indexes[2] + numVerts;

                        numIndexes += 6;
                }

                if(tri->neighbors[2] < 0 || !sh.facing[tri->neighbors[2]])
                {
                        indexes[numIndexes + 0] = tri->indexes[0];
                        indexes[numIndexes + 1] = tri->indexes[2];
                        indexes[numIndexes + 2] = tri->indexes[2] + numVerts;

                        indexes[numIndexes + 3] = tri->indexes[0];
                        indexes[numIndexes + 4] = tri->indexes[2] + numVerts;
                        indexes[numIndexes + 5] = tri->indexes[0] + numVerts;

                        numIndexes += 6;
                }
        }

        // set up indices for light and dark caps
        for(i = 0, tri = triangles; i < numTriangles; i++, tri++)
        {
                if(!sh.facing[i])
                {
                        continue;
                }

                // light cap
                indexes[numIndexes + 0] = tri->indexes[0];
                indexes[numIndexes + 1] = tri->indexes[1];
                indexes[numIndexes + 2] = tri->indexes[2];

                // dark cap
                indexes[numIndexes + 3] = tri->indexes[2] + numVerts;
                indexes[numIndexes + 4] = tri->indexes[1] + numVerts;
                indexes[numIndexes + 5] = tri->indexes[0] + numVerts;

                numIndexes += 6;
        }

        return numIndexes;
}
*/

/*
static int R_BuildShadowPlanes(int numTriangles, const srfTriangle_t * triangles, int numVerts, srfVert_t * verts,
                                                           cplane_t shadowPlanes[SHADER_MAX_TRIANGLES], trRefLight_t * light)
{
        int             i;
        int             numShadowPlanes;
        const srfTriangle_t *tri;
        vec3_t          pos[3];

//  vec3_t          lightDir;
        vec4_t          plane;

        if(r_noShadowFrustums->integer)
        {
                return 0;
        }

        // calculate shadow frustum
        numShadowPlanes = 0;

        // set up indices for silhouette edges
        for(i = 0, tri = triangles; i < numTriangles; i++, tri++)
        {
                if(!sh.facing[i])
                {
                        continue;
                }

                if(tri->neighbors[0] < 0 || !sh.facing[tri->neighbors[0]])
                {
                        //indexes[numIndexes + 0] = tri->indexes[1];
                        //indexes[numIndexes + 1] = tri->indexes[0];
                        //indexes[numIndexes + 2] = tri->indexes[0] + numVerts;

                        VectorCopy(verts[tri->indexes[1]].xyz, pos[0]);
                        VectorCopy(verts[tri->indexes[0]].xyz, pos[1]);
                        VectorCopy(light->origin, pos[2]);

                        // extrude the infinite one
                        //VectorSubtract(verts[tri->indexes[0]].xyz, light->origin, lightDir);
                        //VectorAdd(verts[tri->indexes[0]].xyz, lightDir, pos[2]);
                        //VectorNormalize(lightDir);
                        //VectorMA(verts[tri->indexes[0]].xyz, 9999, lightDir, pos[2]);

                        if(PlaneFromPoints(plane, pos[0], pos[1], pos[2], qtrue))
                        {
                                shadowPlanes[numShadowPlanes].normal[0] = plane[0];
                                shadowPlanes[numShadowPlanes].normal[1] = plane[1];
                                shadowPlanes[numShadowPlanes].normal[2] = plane[2];
                                shadowPlanes[numShadowPlanes].dist = plane[3];

                                numShadowPlanes++;
                        }
                        else
                        {
                                return 0;
                        }
                }

                if(tri->neighbors[1] < 0 || !sh.facing[tri->neighbors[1]])
                {
                        //indexes[numIndexes + 0] = tri->indexes[2];
                        //indexes[numIndexes + 1] = tri->indexes[1];
                        //indexes[numIndexes + 2] = tri->indexes[1] + numVerts;

                        VectorCopy(verts[tri->indexes[2]].xyz, pos[0]);
                        VectorCopy(verts[tri->indexes[1]].xyz, pos[1]);
                        VectorCopy(light->origin, pos[2]);

                        // extrude the infinite one
                        //VectorSubtract(verts[tri->indexes[1]].xyz, light->origin, lightDir);
                        //VectorNormalize(lightDir);
                        //VectorMA(verts[tri->indexes[1]].xyz, 9999, lightDir, pos[2]);

                        if(PlaneFromPoints(plane, pos[0], pos[1], pos[2], qtrue))
                        {
                                shadowPlanes[numShadowPlanes].normal[0] = plane[0];
                                shadowPlanes[numShadowPlanes].normal[1] = plane[1];
                                shadowPlanes[numShadowPlanes].normal[2] = plane[2];
                                shadowPlanes[numShadowPlanes].dist = plane[3];

                                numShadowPlanes++;
                        }
                        else
                        {
                                return 0;
                        }
                }

                if(tri->neighbors[2] < 0 || !sh.facing[tri->neighbors[2]])
                {
                        //indexes[numIndexes + 0] = tri->indexes[0];
                        //indexes[numIndexes + 1] = tri->indexes[2];
                        //indexes[numIndexes + 2] = tri->indexes[2] + numVerts;

                        VectorCopy(verts[tri->indexes[0]].xyz, pos[0]);
                        VectorCopy(verts[tri->indexes[2]].xyz, pos[1]);
                        VectorCopy(light->origin, pos[2]);

                        // extrude the infinite one
                        //VectorSubtract(verts[tri->indexes[2]].xyz, light->origin, lightDir);
                        //VectorNormalize(lightDir);
                        //VectorMA(verts[tri->indexes[2]].xyz, 9999, lightDir, pos[2]);

                        if(PlaneFromPoints(plane, pos[0], pos[1], pos[2], qtrue))
                        {
                                shadowPlanes[numShadowPlanes].normal[0] = plane[0];
                                shadowPlanes[numShadowPlanes].normal[1] = plane[1];
                                shadowPlanes[numShadowPlanes].normal[2] = plane[2];
                                shadowPlanes[numShadowPlanes].dist = plane[3];

                                numShadowPlanes++;
                        }
                        else
                        {
                                return 0;
                        }
                }
        }

        // set up indices for light and dark caps
        for(i = 0, tri = triangles; i < numTriangles; i++, tri++)
        {
                if(!sh.facing[i])
                {
                        continue;
                }

                // light cap
                //indexes[numIndexes + 0] = tri->indexes[0];
                //indexes[numIndexes + 1] = tri->indexes[1];
                //indexes[numIndexes + 2] = tri->indexes[2];

                VectorCopy(verts[tri->indexes[0]].xyz, pos[0]);
                VectorCopy(verts[tri->indexes[1]].xyz, pos[1]);
                VectorCopy(verts[tri->indexes[2]].xyz, pos[2]);

                if(PlaneFromPoints(plane, pos[0], pos[1], pos[2], qfalse))
                {
                        shadowPlanes[numShadowPlanes].normal[0] = plane[0];
                        shadowPlanes[numShadowPlanes].normal[1] = plane[1];
                        shadowPlanes[numShadowPlanes].normal[2] = plane[2];
                        shadowPlanes[numShadowPlanes].dist = plane[3];

                        numShadowPlanes++;
                }
                else
                {
                        return 0;
                }
        }


        for(i = 0; i < numShadowPlanes; i++)
        {
                //vec_t           length, ilength;

                shadowPlanes[i].type = PLANE_NON_AXIAL;


                SetPlaneSignbits(&shadowPlanes[i]);
        }

        return numShadowPlanes;
}
*/

static qboolean R_PrecacheFaceInteraction( srfSurfaceFace_t *cv, shader_t *shader, trRefLight_t *light )
{
	// check if bounds intersect
	if ( !BoundsIntersect( light->worldBounds[ 0 ], light->worldBounds[ 1 ], cv->bounds[ 0 ], cv->bounds[ 1 ] ) )
	{
		return qfalse;
	}

	return qtrue;
}

static int R_PrecacheGridInteraction( srfGridMesh_t *cv, shader_t *shader, trRefLight_t *light )
{
	// check if bounds intersect
	if ( !BoundsIntersect( light->worldBounds[ 0 ], light->worldBounds[ 1 ], cv->bounds[ 0 ], cv->bounds[ 1 ] ) )
	{
		return qfalse;
	}

	return qtrue;
}

static int R_PrecacheTrisurfInteraction( srfTriangles_t *cv, shader_t *shader, trRefLight_t *light )
{
	// check if bounds intersect
	if ( !BoundsIntersect( light->worldBounds[ 0 ], light->worldBounds[ 1 ], cv->bounds[ 0 ], cv->bounds[ 1 ] ) )
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

	if ( *surf->data == SF_FACE )
	{
		intersects = R_PrecacheFaceInteraction( ( srfSurfaceFace_t * ) surf->data, surf->shader, light );
	}
	else if ( *surf->data == SF_GRID )
	{
		intersects = R_PrecacheGridInteraction( ( srfGridMesh_t * ) surf->data, surf->shader, light );
	}
	else if ( *surf->data == SF_TRIANGLES )
	{
		intersects = R_PrecacheTrisurfInteraction( ( srfTriangles_t * ) surf->data, surf->shader, light );
	}
	else
	{
		intersects = qfalse;
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

	// light already hit node
	if ( node->lightCount == s_lightCount )
	{
		return;
	}

	node->lightCount = s_lightCount;

	if ( node->contents != -1 )
	{
		// leaf node, so add mark surfaces
		int          c;
		bspSurface_t *surf, **mark;

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

		return;
	}

	// node is just a decision point, so go down both sides
	// since we don't care about sort orders, just go positive to negative
	r = BoxOnPlaneSide( light->worldBounds[ 0 ], light->worldBounds[ 1 ], node->plane );

	switch ( r )
	{
		case 1:
			R_RecursivePrecacheInteractionNode( node->children[ 0 ], light );
			break;

		case 2:
			R_RecursivePrecacheInteractionNode( node->children[ 1 ], light );
			break;

		case 3:
		default:
			// recurse down the children, front side first
			R_RecursivePrecacheInteractionNode( node->children[ 0 ], light );
			R_RecursivePrecacheInteractionNode( node->children[ 1 ], light );
			break;
	}
}

/*
================
R_RecursiveAddInteractionNode
================
*/
static void R_RecursiveAddInteractionNode( bspNode_t *node, trRefLight_t *light )
{
	int r;

	// light already hit node
	if ( node->lightCount == s_lightCount )
	{
		return;
	}

	node->lightCount = s_lightCount;

	if ( node->contents != -1 )
	{
		vec3_t worldBounds[ 2 ];

		VectorCopy( node->mins, worldBounds[ 0 ] );
		VectorCopy( node->maxs, worldBounds[ 1 ] );

		if ( R_CullLightWorldBounds( light, worldBounds ) != CULL_OUT )
		{
			link_t *l;

			l = ri.Hunk_Alloc( sizeof( *l ), h_low );
			InitLink( l, node );

			InsertLink( l, &light->leafs );

			light->leafs.numElements++;
		}

		return;
	}

	// node is just a decision point, so go down both sides
	// since we don't care about sort orders, just go positive to negative
	r = BoxOnPlaneSide( light->worldBounds[ 0 ], light->worldBounds[ 1 ], node->plane );

	switch ( r )
	{
		case 1:
			R_RecursiveAddInteractionNode( node->children[ 0 ], light );
			break;

		case 2:
			R_RecursiveAddInteractionNode( node->children[ 1 ], light );
			break;

		case 3:
		default:
			// recurse down the children, front side first
			R_RecursiveAddInteractionNode( node->children[ 0 ], light );
			R_RecursiveAddInteractionNode( node->children[ 1 ], light );
			break;
	}
}

/*
=================
R_ShadowFrustumCullWorldBounds

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
int R_ShadowFrustumCullWorldBounds( int numShadowPlanes, cplane_t *shadowPlanes, vec3_t worldBounds[ 2 ] )
{
	int      i;
	cplane_t *plane;
	qboolean anyClip;
	int      r;

	if ( !numShadowPlanes )
	{
		return CULL_CLIP;
	}

	// check against frustum planes
	anyClip = qfalse;

	for ( i = 0; i < numShadowPlanes; i++ )
	{
		plane = &shadowPlanes[ i ];

		r = BoxOnPlaneSide( worldBounds[ 0 ], worldBounds[ 1 ], plane );

		if ( r == 2 )
		{
			// completely outside frustum
			return CULL_OUT;
		}

		if ( r == 3 )
		{
			anyClip = qtrue;
		}
	}

	if ( !anyClip )
	{
		// completely inside frustum
		return CULL_IN;
	}

	// partially clipped
	return CULL_CLIP;
}

/*
=============
R_KillRedundantInteractions
=============
*/

/*
static void R_KillRedundantInteractions(trRefLight_t * light)
{
        interactionCache_t *iaCache, *iaCache2;
        bspSurface_t   *surface;
        vec3_t          localBounds[2];

        if(r_shadows->integer <= SHADOWING_BLOB)
                return;

        if(!light->firstInteractionCache)
        {
                // this light has no interactions precached
                return;
        }

        if(light->l.noShadows)
        {
                // actually noShadows lights are quite bad concerning this optimization
                return;
        }

        for(iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
        {
                surface = iaCache->surface;

                if(surface->shader->sort > SS_OPAQUE)
                        continue;

                if(surface->shader->noShadows)
                        continue;

                // HACK: allow fancy alphatest shadows with shadow mapping
                if(r_shadows->integer >= SHADOWING_ESM16 && surface->shader->alphaTest)
                        continue;

                for(iaCache2 = light->firstInteractionCache; iaCache2; iaCache2 = iaCache2->next)
                {
                        if(iaCache == iaCache2)
                        {
                                // don't check the surface of the current interaction with its shadow frustum
                                continue;
                        }

                        surface = iaCache2->surface;

                        if(*surface->data == SF_FACE)
                        {
                                srfSurfaceFace_t *face;

                                face = (srfSurfaceFace_t *) surface->data;

                                VectorCopy(face->bounds[0], localBounds[0]);
                                VectorCopy(face->bounds[1], localBounds[1]);
                        }
                        else if(*surface->data == SF_GRID)
                        {
                                srfGridMesh_t  *grid;

                                grid = (srfGridMesh_t *) surface->data;

                                VectorCopy(grid->meshBounds[0], localBounds[0]);
                                VectorCopy(grid->meshBounds[1], localBounds[1]);
                        }
                        else if(*surface->data == SF_TRIANGLES)
                        {
                                srfTriangles_t *tri;

                                tri = (srfTriangles_t *) surface->data;

                                VectorCopy(tri->bounds[0], localBounds[0]);
                                VectorCopy(tri->bounds[1], localBounds[1]);
                        }
                        else
                        {
                                iaCache2->redundant = qfalse;
                                continue;
                        }

                        if(R_ShadowFrustumCullWorldBounds(iaCache->numShadowPlanes, iaCache->shadowPlanes, localBounds) == CULL_IN)
                        {
                                iaCache2->redundant = qtrue;
                                c_redundantInteractions++;
                        }
                }

                if(iaCache->redundant)
                {
                        c_redundantInteractions++;
                }
        }
}
*/

/*
=================
R_CreateInteractionVBO
=================
*/
static interactionVBO_t *R_CreateInteractionVBO( trRefLight_t *light )
{
	interactionVBO_t *iaVBO;

	iaVBO = ri.Hunk_Alloc( sizeof( *iaVBO ), h_low );

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
#if 1
		vec3_t pos[ 3 ];
		float  d;

		VectorCopy( verts[ tri->indexes[ 0 ] ].xyz, pos[ 0 ] );
		VectorCopy( verts[ tri->indexes[ 1 ] ].xyz, pos[ 1 ] );
		VectorCopy( verts[ tri->indexes[ 2 ] ].xyz, pos[ 2 ] );

		if ( PlaneFromPoints( tri->plane, pos[ 0 ], pos[ 1 ], pos[ 2 ] ) )
		{
			tri->degenerated = qfalse;

			if ( light->l.rlType == RL_DIRECTIONAL )
			{
				vec3_t lightDirection;

				// light direction is from surface to light
#if 1
				VectorCopy( tr.sunDirection, lightDirection );
#else
				VectorCopy( light->direction, lightDirection );
#endif

				d = DotProduct( tri->plane, lightDirection );

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
				d = DotProduct( tri->plane, light->origin ) - tri->plane[ 3 ];

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
			tri->degenerated = qtrue;
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

#else
		tri->facingLight = qtrue;
		numFacing++;
#endif
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
#if 1
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

	if ( r_deferredShading->integer && r_shadows->integer < SHADOWING_ESM16 )
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
	iaCachesSorted = ri.Hunk_AllocateTempMemory( numCaches * sizeof( iaCachesSorted[ 0 ] ) );

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
	qsort( iaCachesSorted, numCaches, sizeof( iaCachesSorted ), InteractionCacheCompare );

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

			//ri.Printf(PRINT_ALL, "...calculating light mesh VBOs ( %s, %i verts %i tris )\n", shader->name, vertexesNum, indexesNum / 3);

			// create surface
			vboSurf = ri.Hunk_Alloc( sizeof( *vboSurf ), h_low );
			vboSurf->surfaceType = SF_VBO_MESH;
			vboSurf->numIndexes = numTriangles * 3;
			vboSurf->numVerts = numVerts;
			vboSurf->lightmapNum = -1;

			VectorCopy( bounds[ 0 ], vboSurf->bounds[ 0 ] );
			VectorCopy( bounds[ 1 ], vboSurf->bounds[ 1 ] );

			// create arrays
			triangles = ri.Hunk_AllocateTempMemory( numTriangles * sizeof( srfTriangle_t ) );
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

			/*
			   numVerts = OptimizeVertices(numVerts, verts, numTriangles, triangles, optimizedVerts, CompareLightVert);
			   if(c_redundantVertexes)
			   {
			   ri.Printf(PRINT_DEVELOPER,
			   "...removed %i redundant vertices from staticLightMesh %i ( %s, %i verts %i tris )\n",
			   c_redundantVertexes, c_vboLightSurfaces, shader->name, numVerts, numTriangles);
			   }

			   vboSurf->vbo = R_CreateVBO2(va("staticLightMesh_vertices %i", c_vboLightSurfaces), numVerts, optimizedVerts,
			   ATTR_POSITION | ATTR_TEXCOORD | ATTR_TANGENT | ATTR_BINORMAL | ATTR_NORMAL |
			   ATTR_COLOR, VBO_USAGE_STATIC);
			 */

#if CALC_REDUNDANT_SHADOWVERTS
			OptimizeTrianglesLite( s_worldData.redundantLightVerts, numTriangles, triangles );

			if ( c_redundantVertexes )
			{
				ri.Printf( PRINT_DEVELOPER, "...removed %i redundant vertices from staticLightMesh %i ( %s, %i verts %i tris )\n",
				           c_redundantVertexes, c_vboLightSurfaces, shader->name, numVerts, numTriangles );
			}

#endif
			vboSurf->vbo = s_worldData.vbo;
			vboSurf->ibo =
			  R_CreateIBO2( va( "staticLightMesh_IBO %i", c_vboLightSurfaces ), numTriangles, triangles, VBO_USAGE_STATIC );

			ri.Hunk_FreeTempMemory( triangles );

			// add everything needed to the light
			iaVBO = R_CreateInteractionVBO( light );
			iaVBO->shader = ( struct shader_s * ) shader;
			iaVBO->vboLightMesh = ( struct srfVBOMesh_s * ) vboSurf;

			c_vboLightSurfaces++;
		}
	}

	ri.Hunk_FreeTempMemory( iaCachesSorted );
#endif
}

/*
===============
R_CreateVBOShadowMeshes
===============
*/
static void R_CreateVBOShadowMeshes( trRefLight_t *light )
{
#if 1
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
	iaCachesSorted = ri.Hunk_AllocateTempMemory( numCaches * sizeof( iaCachesSorted[ 0 ] ) );

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
	qsort( iaCachesSorted, numCaches, sizeof( iaCachesSorted ), InteractionCacheCompare );

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

#if 0

				if ( surface->shader != shader )
				{
					break;
				}

#else

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

#endif

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

			//ri.Printf(PRINT_ALL, "...calculating light mesh VBOs ( %s, %i verts %i tris )\n", shader->name, vertexesNum, indexesNum / 3);

			// create surface
			vboSurf = ri.Hunk_Alloc( sizeof( *vboSurf ), h_low );
			vboSurf->surfaceType = SF_VBO_MESH;
			vboSurf->numIndexes = numTriangles * 3;
			vboSurf->numVerts = numVerts;
			vboSurf->lightmapNum = -1;

			VectorCopy( bounds[ 0 ], vboSurf->bounds[ 0 ] );
			VectorCopy( bounds[ 1 ], vboSurf->bounds[ 1 ] );

			// create arrays
			triangles = ri.Hunk_AllocateTempMemory( numTriangles * sizeof( srfTriangle_t ) );
			numTriangles = 0;

			// build triangle indices
			for ( l = k; l < numCaches; l++ )
			{
				iaCache2 = iaCachesSorted[ l ];

				surface = iaCache2->surface;

#if 0

				if ( surface->shader != shader )
				{
					break;
				}

#else

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

#endif

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

			/*
			   numVerts = OptimizeVertices(numVerts, verts, numTriangles, triangles, optimizedVerts, CompareLightVert);
			   if(c_redundantVertexes)
			   {
			   ri.Printf(PRINT_DEVELOPER,
			   "...removed %i redundant vertices from staticLightMesh %i ( %s, %i verts %i tris )\n",
			   c_redundantVertexes, c_vboLightSurfaces, shader->name, numVerts, numTriangles);
			   }

			   vboSurf->vbo = R_CreateVBO2(va("staticLightMesh_vertices %i", c_vboLightSurfaces), numVerts, optimizedVerts,
			   ATTR_POSITION | ATTR_TEXCOORD | ATTR_TANGENT | ATTR_BINORMAL | ATTR_NORMAL |
			   ATTR_COLOR, VBO_USAGE_STATIC);
			 */

#if CALC_REDUNDANT_SHADOWVERTS
			OptimizeTrianglesLite( s_worldData.redundantShadowVerts, numTriangles, triangles );

			if ( c_redundantVertexes )
			{
				ri.Printf( PRINT_DEVELOPER, "...removed %i redundant vertices from staticLightMesh %i ( %s, %i verts %i tris )\n",
				           c_redundantVertexes, c_vboLightSurfaces, shader->name, numVerts, numTriangles );
			}

#endif
			vboSurf->vbo = s_worldData.vbo;
			vboSurf->ibo = R_CreateIBO2( va( "staticShadowMesh_IBO %i", c_vboLightSurfaces ), numTriangles, triangles, VBO_USAGE_STATIC );

			ri.Hunk_FreeTempMemory( triangles );

			// add everything needed to the light
			iaVBO = R_CreateInteractionVBO( light );
			iaVBO->shader = ( struct shader_s * ) shader;
			iaVBO->vboShadowMesh = ( struct srfVBOMesh_s * ) vboSurf;

			c_vboShadowSurfaces++;
		}
	}

	ri.Hunk_FreeTempMemory( iaCachesSorted );
#endif
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
	iaCachesSorted = ri.Hunk_AllocateTempMemory( numCaches * sizeof( iaCachesSorted[ 0 ] ) );

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
	qsort( iaCachesSorted, numCaches, sizeof( iaCachesSorted ), InteractionCacheCompare );

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

			//if(!(iaCache->cubeSideBits & (1 << cubeSide)))
			//  continue;

			//if(shader != oldShader)
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

#if 0

					if ( surface->shader != shader )
					{
						break;
					}

#else

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

#endif

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

				//ri.Printf(PRINT_ALL, "...calculating light mesh VBOs ( %s, %i verts %i tris )\n", shader->name, vertexesNum, indexesNum / 3);

				// create surface
				vboSurf = ri.Hunk_Alloc( sizeof( *vboSurf ), h_low );
				vboSurf->surfaceType = SF_VBO_MESH;
				vboSurf->numIndexes = numTriangles * 3;
				vboSurf->numVerts = numVerts;
				vboSurf->lightmapNum = -1;
				ZeroBounds( vboSurf->bounds[ 0 ], vboSurf->bounds[ 1 ] );

				// create arrays
				triangles = ri.Hunk_AllocateTempMemory( numTriangles * sizeof( srfTriangle_t ) );
				numTriangles = 0;

				// build triangle indices
				for ( l = k; l < numCaches; l++ )
				{
					iaCache2 = iaCachesSorted[ l ];

					surface = iaCache2->surface;

#if 0

					if ( surface->shader != shader )
					{
						break;
					}

#else

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

#endif

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

				if ( alphaTest )
				{
#if CALC_REDUNDANT_SHADOWVERTS
					//OptimizeTriangles(s_worldData.numVerts, s_worldData.verts, numTriangles, triangles, CompareShadowVertAlphaTest);
					OptimizeTrianglesLite( s_worldData.redundantShadowAlphaTestVerts, numTriangles, triangles );

					if ( c_redundantVertexes )
					{
						ri.Printf( PRINT_DEVELOPER,
						           "...removed %i redundant vertices from staticShadowPyramidMesh %i ( %s, %i verts %i tris )\n",
						           c_redundantVertexes, c_vboShadowSurfaces, shader->name, numVerts, numTriangles );
					}

#endif
					vboSurf->vbo = s_worldData.vbo;
					vboSurf->ibo =
					  R_CreateIBO2( va( "staticShadowPyramidMesh_IBO %i", c_vboShadowSurfaces ), numTriangles, triangles,
					                VBO_USAGE_STATIC );
				}
				else
				{
#if CALC_REDUNDANT_SHADOWVERTS
					//OptimizeTriangles(s_worldData.numVerts, s_worldData.verts, numTriangles, triangles, CompareShadowVert);
					OptimizeTrianglesLite( s_worldData.redundantShadowVerts, numTriangles, triangles );

					if ( c_redundantVertexes )
					{
						ri.Printf( PRINT_DEVELOPER,
						           "...removed %i redundant vertices from staticShadowPyramidMesh %i ( %s, %i verts %i tris )\n",
						           c_redundantVertexes, c_vboShadowSurfaces, shader->name, numVerts, numTriangles );
					}

#endif
					vboSurf->vbo = s_worldData.vbo;
					vboSurf->ibo =
					  R_CreateIBO2( va( "staticShadowPyramidMesh_IBO %i", c_vboShadowSurfaces ), numTriangles, triangles,
					                VBO_USAGE_STATIC );
				}

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

	/*
	   if(glConfig.vertexBufferObjectAvailable && r_vboLighting->integer)
	   {
	   srfVBOLightMesh_t *srf;

	   for(iaCache = light->firstInteractionCache; iaCache; iaCache = iaCache->next)
	   {
	   if(iaCache->redundant)
	   continue;

	   if(!iaCache->vboLightMesh)
	   continue;

	   srf = iaCache->vboLightMesh;

	   VectorCopy(srf->bounds[0], localBounds[0]);
	   VectorCopy(srf->bounds[1], localBounds[1]);

	   light->shadowLOD = 0;    // important for R_CalcLightCubeSideBits
	   iaCache->cubeSideBits = R_CalcLightCubeSideBits(light, localBounds);
	   }
	   }
	   else
	 */
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
void R_PrecacheInteractions()
{
	int          i;
	trRefLight_t *light;
	bspSurface_t *surface;
//	int             numLeafs;
	int          startTime, endTime;

	//if(r_precomputedLighting->integer)
	//  return;

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

	ri.Printf( PRINT_ALL, "...precaching %i lights\n", s_worldData.numLights );

	for ( i = 0; i < s_worldData.numLights; i++ )
	{
		light = &s_worldData.lights[ i ];

		if ( ( r_precomputedLighting->integer || r_vertexLighting->integer ) && !light->noRadiosity )
		{
			continue;
		}

#if 0
		ri.Printf( PRINT_ALL, "light %i: origin(%i %i %i) radius(%i %i %i) color(%f %f %f)\n",
		           i,
		           ( int ) light->l.origin[ 0 ], ( int ) light->l.origin[ 1 ], ( int ) light->l.origin[ 2 ],
		           ( int ) light->l.radius[ 0 ], ( int ) light->l.radius[ 1 ], ( int ) light->l.radius[ 2 ],
		           light->l.color[ 0 ], light->l.color[ 1 ], light->l.color[ 2 ] );
#endif

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
		R_RecursivePrecacheInteractionNode( s_worldData.nodes, light );

		// count number of leafs that touch this light
		s_lightCount++;
		QueueInit( &light->leafs );
		R_RecursiveAddInteractionNode( s_worldData.nodes, light );
		//ri.Printf(PRINT_ALL, "light %i touched %i leaves\n", i, QueueSize(&light->leafs));

#if 0
		// Tr3b: this can cause really bad shadow problems :/

		// check if interactions are inside shadows of other interactions
		R_KillRedundantInteractions( light );
#endif

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
	s_worldData.interactions = ri.Hunk_Alloc( s_worldData.numInteractions * sizeof( *s_worldData.interactions ), h_low );

	for ( i = 0; i < s_worldData.numInteractions; i++ )
	{
		s_worldData.interactions[ i ] = ( interactionCache_t * ) Com_GrowListElement( &s_interactions, i );
	}

	Com_DestroyGrowList( &s_interactions );

	ri.Printf( PRINT_ALL, "%i interactions precached\n", s_worldData.numInteractions );
	ri.Printf( PRINT_ALL, "%i interactions were hidden in shadows\n", c_redundantInteractions );

	if ( r_shadows->integer >= SHADOWING_ESM16 )
	{
		// only interesting for omni-directional shadow mapping
		ri.Printf( PRINT_ALL, "%i omni pyramid tests\n", tr.pc.c_pyramidTests );
		ri.Printf( PRINT_ALL, "%i omni pyramid surfaces visible\n", tr.pc.c_pyramid_cull_ent_in );
		ri.Printf( PRINT_ALL, "%i omni pyramid surfaces clipped\n", tr.pc.c_pyramid_cull_ent_clip );
		ri.Printf( PRINT_ALL, "%i omni pyramid surfaces culled\n", tr.pc.c_pyramid_cull_ent_out );
	}

	endTime = ri.Milliseconds();

	ri.Printf( PRINT_ALL, "lights precaching time = %5.2f seconds\n", ( endTime - startTime ) / 1000.0 );
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

	/*
	        // strict aliasing... better?
	        hash += ~({floatint_t __f; __f.f = xyz_epsilonspace[0]; __f.i << 15;});
	        hash ^= ({floatint_t __f; __f.f = xyz_epsilonspace[0]; __f.i >> 10;});
	        hash += ({floatint_t __f; __f.f = xyz_epsilonspace[1]; __f.i << 3;});
	        hash ^= ({floatint_t __f; __f.f = xyz_epsilonspace[1]; __f.i >> 6;});
	        hash += ~({floatint_t __f; __f.f = xyz_epsilonspace[2]; __f.i << 11;});
	        hash ^= ({floatint_t __f; __f.f = xyz_epsilonspace[2]; __f.i >> 16;});
	*/

#endif

	hash = ( int ) fabs( xyz[ 3 ] ) / 8;

	hash = hash % ( HASHTABLE_SIZE );
	return hash;
}

vertexHash_t **NewVertexHashTable( void )
{
	vertexHash_t **hashTable = Com_Allocate( HASHTABLE_SIZE * sizeof( vertexHash_t * ) );

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

				/*
				if(vertexHash->data != NULL)
				{
				        Com_Dealloc(vertexHash->data);
				}
				*/
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

	vertexHash = Com_Allocate( sizeof( vertexHash_t ) );

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
#if 0
	int            j;
	float          distance, maxDistance;
	cubemapProbe_t *cubeProbe;

	GLimp_LogComment( "--- GL_BindNearestCubeMap ---\n" );

	maxDistance = 9999999.0f;
	tr.autoCubeImage = tr.blackCubeImage;

	for ( j = 0; j < tr.cubeProbes.currentElements; j++ )
	{
		cubeProbe = Com_GrowListElement( &tr.cubeProbes, j );

		distance = Distance( cubeProbe->origin, xyz );

		if ( distance < maxDistance )
		{
			tr.autoCubeImage = cubeProbe->cubemap;
			maxDistance = distance;
		}
	}

#else
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
		cubeProbe = vertexHash->data;

		distance = Distance( cubeProbe->origin, xyz );

		if ( distance < maxDistance )
		{
			tr.autoCubeImage = cubeProbe->cubemap;
			maxDistance = distance;
		}
	}

#endif

#if defined( USE_D3D10 )
	// TODO
#else
	GL_Bind( tr.autoCubeImage );
#endif
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

#if 0

	for ( j = 0; j < tr.cubeProbes.currentElements; j++ )
	{
		cubeProbe = Com_GrowListElement( &tr.cubeProbes, j );
#else

	for ( j = 0, vertexHash = tr.cubeHashTable[ hash ]; vertexHash; vertexHash = vertexHash->next, j++ )
	{
		cubeProbe = vertexHash->data;
#endif
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

	//ri.Printf(PRINT_ALL, "iterated through %i cubeprobes\n", j);
}

void R_BuildCubeMaps( void )
{
#if 1
	int            i, j;
	int            ii, jj;
	refdef_t       rf;
	qboolean       flipx;
	qboolean       flipy;
	int            x, y, xy, xy2;

	cubemapProbe_t *cubeProbe;
	byte           temp[ REF_CUBEMAP_SIZE * REF_CUBEMAP_SIZE * 4 ];
	byte           *dest;

#if 0
	byte           *fileBuf;
	char           *fileName = NULL;
	int            fileCount = 0;
	int            fileBufX = 0;
	int            fileBufY = 0;
#endif

	//

	//int             distance = 512;
	//qboolean        bad;

//  srfSurfaceStatic_t *sv;
	int    startTime, endTime;
	size_t tics = 0;
	size_t nextTicCount = 0;

	startTime = ri.Milliseconds();

	memset( &rf, 0, sizeof( refdef_t ) );

	for ( i = 0; i < 6; i++ )
	{
		tr.cubeTemp[ i ] = ri.Z_Malloc( REF_CUBEMAP_SIZE * REF_CUBEMAP_SIZE * 4 );
	}

//	fileBuf = ri.Z_Malloc(REF_CUBEMAP_STORE_SIZE * REF_CUBEMAP_STORE_SIZE * 4);

	// calculate origins for our probes
	Com_InitGrowList( &tr.cubeProbes, 4000 );
	tr.cubeHashTable = NewVertexHashTable();

#if 0

	if ( tr.world->vis )
	{
		bspCluster_t *cluster;

		for ( i = 0; i < tr.world->numClusters; i++ )
		{
			cluster = &tr.world->clusters[ i ];

			// check to see if this is a shit location
			if ( ri.CM_PointContents( cluster->origin, 0 ) == CONTENTS_SOLID )
			{
				continue;
			}

			if ( FindVertexInHashTable( tr.cubeHashTable, cluster->origin, 256 ) == NULL )
			{
				cubeProbe = ri.Hunk_Alloc( sizeof( *cubeProbe ), h_high );
				Com_AddToGrowList( &tr.cubeProbes, cubeProbe );

				VectorCopy( cluster->origin, cubeProbe->origin );

				AddVertexToHashTable( tr.cubeHashTable, cubeProbe->origin, cubeProbe );

				//gridPoint = tr.world->lightGridData + pos[0] * gridStep[0] + pos[1] * gridStep[1] + pos[2] * gridStep[2];

				// TODO connect cubeProbe with gridPoint
			}
		}
	}

#elif 1
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
				cubeProbe = ri.Hunk_Alloc( sizeof( *cubeProbe ), h_high );
				Com_AddToGrowList( &tr.cubeProbes, cubeProbe );

				VectorCopy( node->origin, cubeProbe->origin );

				AddVertexToHashTable( tr.cubeHashTable, cubeProbe->origin, cubeProbe );
			}
		}
	}
#else
	{
		int            k;
		int            numGridPoints;
		bspGridPoint_t *gridPoint;
		int            gridStep[ 3 ];
		int            pos[ 3 ];
		float          posFloat[ 3 ];

		gridStep[ 0 ] = 1;
		gridStep[ 1 ] = tr.world->lightGridBounds[ 0 ];
		gridStep[ 2 ] = tr.world->lightGridBounds[ 0 ] * tr.world->lightGridBounds[ 1 ];

		numGridPoints = tr.world->lightGridBounds[ 0 ] * tr.world->lightGridBounds[ 1 ] * tr.world->lightGridBounds[ 2 ];

		ri.Printf( PRINT_ALL, "...trying to allocate %d cubemaps", numGridPoints );
		ri.Printf( PRINT_ALL, " with gridsize (%i %i %i)", ( int ) tr.world->lightGridSize[ 0 ], ( int ) tr.world->lightGridSize[ 1 ],
		           ( int ) tr.world->lightGridSize[ 2 ] );
		ri.Printf( PRINT_ALL, " and gridbounds (%i %i %i)\n", ( int ) tr.world->lightGridBounds[ 0 ], ( int ) tr.world->lightGridBounds[ 1 ],
		           ( int ) tr.world->lightGridBounds[ 2 ] );

		for ( i = 0; i < tr.world->lightGridBounds[ 0 ]; i += 1 )
		{
			for ( j = 0; j < tr.world->lightGridBounds[ 1 ]; j += 1 )
			{
				for ( k = 0; k < tr.world->lightGridBounds[ 2 ]; k += 1 )
				{
					pos[ 0 ] = i;
					pos[ 1 ] = j;
					pos[ 2 ] = k;

					posFloat[ 0 ] = i * tr.world->lightGridSize[ 0 ];
					posFloat[ 1 ] = j * tr.world->lightGridSize[ 1 ];
					posFloat[ 2 ] = k * tr.world->lightGridSize[ 2 ];

					VectorAdd( posFloat, tr.world->lightGridOrigin, posFloat );

					// check to see if this is a shit location
					if ( ri.CM_PointContents( posFloat, 0 ) == CONTENTS_SOLID )
					{
						continue;
					}

					if ( FindVertexInHashTable( tr.cubeHashTable, posFloat, 256 ) == NULL )
					{
						cubeProbe = ri.Hunk_Alloc( sizeof( *cubeProbe ), h_high );
						Com_AddToGrowList( &tr.cubeProbes, cubeProbe );

						VectorCopy( posFloat, cubeProbe->origin );

						AddVertexToHashTable( tr.cubeHashTable, posFloat, cubeProbe );

						gridPoint = tr.world->lightGridData + pos[ 0 ] * gridStep[ 0 ] + pos[ 1 ] * gridStep[ 1 ] + pos[ 2 ] * gridStep[ 2 ];

						// TODO connect cubeProbe with gridPoint
					}
				}
			}
		}
	}
#endif

	// if we can't find one, fake one
	if ( tr.cubeProbes.currentElements == 0 )
	{
		cubeProbe = ri.Hunk_Alloc( sizeof( *cubeProbe ), h_low );
		Com_AddToGrowList( &tr.cubeProbes, cubeProbe );

		VectorClear( cubeProbe->origin );
	}

	ri.Printf( PRINT_ALL, "...pre-rendering %d cubemaps\n", tr.cubeProbes.currentElements );
	ri.Cvar_Set( "viewlog", "1" );
	ri.Printf( PRINT_ALL, "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
	ri.Printf( PRINT_ALL, "|----|----|----|----|----|----|----|----|----|----|\n" );

	for ( j = 0; j < tr.cubeProbes.currentElements; j++ )
	{
		cubeProbe = Com_GrowListElement( &tr.cubeProbes, j );

		//ri.Printf(PRINT_ALL, "rendering cubemap at (%i %i %i)\n", (int)cubeProbe->origin[0], (int)cubeProbe->origin[1],
		//      (int)cubeProbe->origin[2]);

		if ( ( j + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( j + 1 ) / tr.cubeProbes.currentElements ) * 50.0 );

			do
			{
				ri.Printf( PRINT_ALL, "*" );

#if defined( COMPAT_ET )
				ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
#endif
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

			// collate cubemaps into one large image and write it out
#if 0

			if ( qfalse )
			{
				// Initialize output buffer
				if ( fileBufX == 0 && fileBufY == 0 )
				{
					memset( fileBuf, 255, REF_CUBEMAP_STORE_SIZE * REF_CUBEMAP_STORE_SIZE * 4 );
				}

				// Copy this cube map into buffer
				R_SubImageCpy( fileBuf,
				               fileBufX * REF_CUBEMAP_SIZE, fileBufY * REF_CUBEMAP_SIZE,
				               REF_CUBEMAP_STORE_SIZE, REF_CUBEMAP_STORE_SIZE,
				               tr.cubeTemp[ i ],
				               REF_CUBEMAP_SIZE, REF_CUBEMAP_SIZE,
				               4, qtrue );

				// Increment everything
				fileBufX++;

				if ( fileBufX >= REF_CUBEMAP_STORE_SIDE )
				{
					fileBufY++;
					fileBufX = 0;
				}

				if ( fileBufY >= REF_CUBEMAP_STORE_SIDE )
				{
					// File is full, write it
					fileName = va( "maps/%s/cm_%04d.png", s_worldData.baseName, fileCount );
					ri.Printf( PRINT_ALL, "\nwriting %s\n", fileName );
					ri.FS_WriteFile( fileName, fileBuf, 1 );  // create path
					SavePNG( fileName, fileBuf, REF_CUBEMAP_STORE_SIZE, REF_CUBEMAP_STORE_SIZE, 4, qfalse );

					fileCount++;
					fileBufY = 0;
				}
			}

#endif
		}

#if defined( USE_D3D10 )
		// TODO
		continue;
#else
		// build the cubemap
		//cubeProbe->cubemap = R_CreateCubeImage(va("_autoCube%d", j), (const byte **)tr.cubeTemp, REF_CUBEMAP_SIZE, REF_CUBEMAP_SIZE, IF_NOPICMIP, FT_LINEAR, WT_EDGE_CLAMP);
		cubeProbe->cubemap = R_AllocImage( va( "_autoCube%d", j ), qfalse );

		if ( !cubeProbe->cubemap )
		{
			return;
		}

		cubeProbe->cubemap->type = GL_TEXTURE_CUBE_MAP_ARB;

		cubeProbe->cubemap->width = REF_CUBEMAP_SIZE;
		cubeProbe->cubemap->height = REF_CUBEMAP_SIZE;

		cubeProbe->cubemap->bits = IF_NOPICMIP;
		cubeProbe->cubemap->filterType = FT_LINEAR;
		cubeProbe->cubemap->wrapType = WT_EDGE_CLAMP;

		GL_Bind( cubeProbe->cubemap );

		R_UploadImage( ( const byte ** ) tr.cubeTemp, 6, cubeProbe->cubemap );

		glBindTexture( cubeProbe->cubemap->type, 0 );
#endif
	}

	ri.Printf( PRINT_ALL, "\n" );

#if 0

	// flush the buffer if there's any still unwritten content
	if ( fileBufX != 0 || fileBufY != 0 )
	{
		fileName = va( "maps/%s/cm_%04d.png", s_worldData.baseName, fileCount );
		ri.Printf( PRINT_ALL, "writing %s\n", fileName );
		ri.FS_WriteFile( fileName, fileBuf, 1 );  // create path
		SavePNG( fileName, fileBuf, REF_CUBEMAP_STORE_SIZE, REF_CUBEMAP_STORE_SIZE, 4, qfalse );
	}

	ri.Printf( PRINT_ALL, "Wrote %d cubemaps in %d files.\n", j, fileCount + 1 );
	ri.Free( fileBuf );
#endif

	// turn pixel targets off
	tr.refdef.pixelTarget = NULL;

	// assign the surfs a cubemap
#if 0

	for ( i = 0; i < tr.world->numnodes; i++ )
	{
		msurface_t **mark;
		msurface_t *surf;

		if ( tr.world->nodes[ i ].contents != CONTENTS_SOLID )
		{
			mark = tr.world->nodes[ i ].firstmarksurface;
			j = tr.world->nodes[ i ].nummarksurfaces;

			while ( j-- )
			{
				int dist = 9999999;
				int best = 0;

				surf = *mark;
				mark++;
				sv = ( void * ) surf->data;

				if ( sv->surfaceType != SF_STATIC )
				{
					continue; //
				}

				if ( sv->numIndices == 0 || sv->numVerts == 0 )
				{
					continue;
				}

				if ( sv->cubemap != NULL )
				{
					continue;
				}

				for ( x = 0; x < tr.cubeProbesCount; x++ )
				{
					vec3_t pos;

					pos[ 0 ] = tr.cubeProbes[ x ].origin[ 0 ] - sv->origin[ 0 ];
					pos[ 1 ] = tr.cubeProbes[ x ].origin[ 1 ] - sv->origin[ 1 ];
					pos[ 2 ] = tr.cubeProbes[ x ].origin[ 2 ] - sv->origin[ 2 ];

					distance = VectorLength( pos );

					if ( distance < dist )
					{
						dist = distance;
						best = x;
					}
				}

				sv->cubemap = tr.cubeProbes[ best ].cubemap;
			}
		}
	}

#endif

	endTime = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "cubemap probes pre-rendering time of %i cubes = %5.2f seconds\n", tr.cubeProbes.currentElements,
	           ( endTime - startTime ) / 1000.0 );

#endif
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

	ri.Printf( PRINT_ALL, "----- RE_LoadWorldMap( %s ) -----\n", name );

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

#if defined( COMPAT_ET )
	RE_SetFog( FOG_SKY, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_PORTALVIEW, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_HUD, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_MAP, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_CURRENT, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_TARGET, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_WATER, 0, 0, 0, 0, 0, 0 );
	RE_SetFog( FOG_SERVER, 0, 0, 0, 0, 0, 0 );

	tr.glfogNum = 0;
#endif

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

	startMarker = ri.Hunk_Alloc( 0, h_low );

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
//	ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n");
	R_LoadEntities( &header->lumps[ LUMP_ENTITIES ] );

//	ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n");
	R_LoadShaders( &header->lumps[ LUMP_SHADERS ] );

//	ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n");
	R_LoadLightmaps( &header->lumps[ LUMP_LIGHTMAPS ], name );

//	ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n");
	R_LoadPlanes( &header->lumps[ LUMP_PLANES ] );

//	ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n");
	R_LoadSurfaces( &header->lumps[ LUMP_SURFACES ], &header->lumps[ LUMP_DRAWVERTS ], &header->lumps[ LUMP_DRAWINDEXES ] );

//	ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n");
	R_LoadMarksurfaces( &header->lumps[ LUMP_LEAFSURFACES ] );

//	ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n");
	R_LoadNodesAndLeafs( &header->lumps[ LUMP_NODES ], &header->lumps[ LUMP_LEAFS ] );

//	ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n");
	R_LoadSubmodels( &header->lumps[ LUMP_MODELS ] );

	// moved fog lump loading here, so fogs can be tagged with a model num
//	ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n");
	R_LoadFogs( &header->lumps[ LUMP_FOGS ], &header->lumps[ LUMP_BRUSHES ], &header->lumps[ LUMP_BRUSHSIDES ] );

//	ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n");
	R_LoadVisibility( &header->lumps[ LUMP_VISIBILITY ] );

//	ri.Cmd_ExecuteText(EXEC_NOW, "updatescreen\n");
	R_LoadLightGrid( &header->lumps[ LUMP_LIGHTGRID ] );

	// create static VBOS from the world
	R_CreateWorldVBO();
	R_CreateClusters();
	R_CreateSubModelVBOs();

	// we precache interactions between lights and surfaces
	// to reduce the polygon count
	R_PrecacheInteractions();

	s_worldData.dataSize = ( byte * ) ri.Hunk_Alloc( 0, h_low ) - startMarker;

	//ri.Printf(PRINT_ALL, "total world data size: %d.%02d MB\n", s_worldData.dataSize / (1024 * 1024),
	//        (s_worldData.dataSize % (1024 * 1024)) * 100 / (1024 * 1024));

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

#if defined( COMPAT_ET )
	// reset fog to world fog (if present)
	RE_SetFog( FOG_CMD_SWITCHFOG, FOG_MAP, 20, 0, 0, 0, 0 );
#endif

	// make sure the VBO glState entries are save
	R_BindNullVBO();
	R_BindNullIBO();

	//----(SA)  set the sun shader if there is one
	if ( tr.sunShaderName )
	{
		tr.sunShader = R_FindShader( tr.sunShaderName, SHADER_3D_STATIC, qtrue );
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
