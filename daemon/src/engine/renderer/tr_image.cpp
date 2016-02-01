/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_image.c
#include <common/FileSystem.h>
#include "tr_local.h"

int                  gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int                  gl_filter_max = GL_LINEAR;

image_t              *r_imageHashTable[ IMAGE_FILE_HASH_SIZE ];

#define Tex_ByteToFloat(v) ( ( (int)(v) - 128 ) / 127.0f )
#define Tex_FloatToByte(v) ( 128 + (int) ( (v) * 127.0f + 0.5 ) )

struct textureMode_t
{
	const char *name;
	int  minimize, maximize;
};

static const textureMode_t modes[] =
{
	{ "GL_NEAREST",                GL_NEAREST,                GL_NEAREST },
	{ "GL_LINEAR",                 GL_LINEAR,                 GL_LINEAR  },
	{ "GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_NEAREST",  GL_LINEAR_MIPMAP_NEAREST,  GL_LINEAR  },
	{ "GL_NEAREST_MIPMAP_LINEAR",  GL_NEAREST_MIPMAP_LINEAR,  GL_NEAREST },
	{ "GL_LINEAR_MIPMAP_LINEAR",   GL_LINEAR_MIPMAP_LINEAR,   GL_LINEAR  }
};

/*
================
return a hash value for the filename
================
*/
long GenerateImageHashValue( const char *fname )
{
	int  i;
	long hash;
	char letter;

//  Log::Notice("tr_image::GenerateImageHashValue: '%s'", fname);

	hash = 0;
	i = 0;

	while ( fname[ i ] != '\0' )
	{
		letter = Str::ctolower( fname[ i ] );

		if ( letter == '\\' )
		{
			letter = '/'; // damn path names
		}

		hash += ( long )( letter ) * ( i + 119 );
		i++;
	}

	hash &= ( IMAGE_FILE_HASH_SIZE - 1 );
	return hash;
}

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( const char *string )
{
	int     i;
	image_t *image;

	for ( i = 0; i < 6; i++ )
	{
		if ( !Q_stricmp( modes[ i ].name, string ) )
		{
			break;
		}
	}

	if ( i == 6 )
	{
		Log::Warn("bad filter name" );
		return;
	}

	gl_filter_min = modes[ i ].minimize;
	gl_filter_max = modes[ i ].maximize;

	// bound texture anisotropy
	if ( glConfig2.textureAnisotropyAvailable )
	{
		if ( r_ext_texture_filter_anisotropic->value > glConfig2.maxTextureAnisotropy )
		{
			ri.Cvar_Set( "r_ext_texture_filter_anisotropic", va( "%f", glConfig2.maxTextureAnisotropy ) );
		}
		else if ( r_ext_texture_filter_anisotropic->value < 1.0 )
		{
			ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "1.0" );
		}
	}

	// change all the existing mipmap texture objects
	for ( i = 0; i < tr.images.currentElements; i++ )
	{
		image = (image_t*) Com_GrowListElement( &tr.images, i );

		if ( image->filterType == filterType_t::FT_DEFAULT )
		{
			GL_Bind( image );

			// set texture filter
			glTexParameterf( image->type, GL_TEXTURE_MIN_FILTER, gl_filter_min );
			glTexParameterf( image->type, GL_TEXTURE_MAG_FILTER, gl_filter_max );

			// set texture anisotropy
			if ( glConfig2.textureAnisotropyAvailable )
			{
				glTexParameterf( image->type, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_ext_texture_filter_anisotropic->value );
			}
		}
	}
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f()
{
	int        i;
	image_t    *image;
	int        texels;
	int        dataSize;
	int        imageDataSize;
	const char *yesno[] =
	{
		"no ", "yes"
	};
	const char *filter = ri.Cmd_Argc() > 1 ? ri.Cmd_Argv(1) : nullptr;

	Log::Notice("\n      -w-- -h-- -mm- -type-   -if-- wrap --name-------" );

	texels = 0;
	dataSize = 0;

	for ( i = 0; i < tr.images.currentElements; i++ )
	{
		image = (image_t*) Com_GrowListElement( &tr.images, i );
		char buffer[ MAX_TOKEN_CHARS ];
		std::string out;

		if ( filter && !Com_Filter( filter, image->name, true ) )
		{
			continue;
		}

		Com_sprintf( buffer, sizeof( buffer ), "%4i: %4i %4i  %s   ",
		           i, image->uploadWidth, image->uploadHeight, yesno[ image->filterType == filterType_t::FT_DEFAULT ] );
		out += buffer;
		switch ( image->type )
		{
			case GL_TEXTURE_2D:
				texels += image->uploadWidth * image->uploadHeight;
				imageDataSize = image->uploadWidth * image->uploadHeight;

				Com_sprintf( buffer, sizeof( buffer ),  "2D   " );
				out += buffer;
				break;

			case GL_TEXTURE_CUBE_MAP:
				texels += image->uploadWidth * image->uploadHeight * 6;
				imageDataSize = image->uploadWidth * image->uploadHeight * 6;

				Com_sprintf( buffer, sizeof( buffer ),  "CUBE " );
				out += buffer;
				break;

			default:
				Com_sprintf( buffer, sizeof( buffer ),  "???? " );
				out += buffer;
				imageDataSize = image->uploadWidth * image->uploadHeight;
				break;
		}

		switch ( image->internalFormat )
		{
			case GL_RGB8:
				Com_sprintf( buffer, sizeof( buffer ),  "RGB8     " );
				out += buffer;
				imageDataSize *= 3;
				break;

			case GL_RGBA8:
				Com_sprintf( buffer, sizeof( buffer ),  "RGBA8    " );
				out += buffer;
				imageDataSize *= 4;
				break;

			case GL_RGB16:
				Com_sprintf( buffer, sizeof( buffer ),  "RGB      " );
				out += buffer;
				imageDataSize *= 6;
				break;

			case GL_RGB16F:
				Com_sprintf( buffer, sizeof( buffer ),  "RGB16F   " );
				out += buffer;
				imageDataSize *= 6;
				break;

			case GL_RGB32F:
				Com_sprintf( buffer, sizeof( buffer ),  "RGB32F   " );
				out += buffer;
				imageDataSize *= 12;
				break;

			case GL_RGBA16F:
				Com_sprintf( buffer, sizeof( buffer ),  "RGBA16F  " );
				out += buffer;
				imageDataSize *= 8;
				break;

			case GL_RGBA32F:
				Com_sprintf( buffer, sizeof( buffer ),  "RGBA32F  " );
				out += buffer;
				imageDataSize *= 16;
				break;

			case GL_ALPHA16F_ARB:
				Com_sprintf( buffer, sizeof( buffer ),  "A16F     " );
				out += buffer;
				imageDataSize *= 2;
				break;

			case GL_ALPHA32F_ARB:
				Com_sprintf( buffer, sizeof( buffer ),  "A32F     " );
				out += buffer;
				imageDataSize *= 4;
				break;

			case GL_R16F:
				Com_sprintf( buffer, sizeof( buffer ),  "R16F     " );
				out += buffer;
				imageDataSize *= 2;
				break;

			case GL_R32F:
				Com_sprintf( buffer, sizeof( buffer ),  "R32F     " );
				out += buffer;
				imageDataSize *= 4;
				break;

			case GL_LUMINANCE_ALPHA16F_ARB:
				Com_sprintf( buffer, sizeof( buffer ),  "LA16F    " );
				out += buffer;
				imageDataSize *= 4;
				break;

			case GL_LUMINANCE_ALPHA32F_ARB:
				Com_sprintf( buffer, sizeof( buffer ),  "LA32F    " );
				out += buffer;
				imageDataSize *= 8;
				break;

			case GL_RG16F:
				Com_sprintf( buffer, sizeof( buffer ),  "RG16F    " );
				out += buffer;
				out += buffer;
				imageDataSize *= 4;
				break;

			case GL_RG32F:
				Com_sprintf( buffer, sizeof( buffer ),  "RG32F    " );
				out += buffer;
				imageDataSize *= 8;
				break;

			case GL_COMPRESSED_RGBA:
				Com_sprintf( buffer, sizeof( buffer ),  "      " );
				out += buffer;
				imageDataSize *= 4; // FIXME
				break;

			case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
				Com_sprintf( buffer, sizeof( buffer ),  "DXT1     " );
				out += buffer;
				imageDataSize *= 4 / 8;
				break;

			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
				Com_sprintf( buffer, sizeof( buffer ),  "DXT1a    " );
				out += buffer;
				imageDataSize *= 4 / 8;
				break;

			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
				Com_sprintf( buffer, sizeof( buffer ),  "DXT3     " );
				out += buffer;
				imageDataSize *= 4 / 4;
				break;

			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				Com_sprintf( buffer, sizeof( buffer ),  "DXT5     " );
				out += buffer;
				imageDataSize *= 4 / 4;
				break;

			case GL_DEPTH_COMPONENT16:
				Com_sprintf( buffer, sizeof( buffer ),  "D16      " );
				out += buffer;
				imageDataSize *= 2;
				break;

			case GL_DEPTH_COMPONENT24:
				Com_sprintf( buffer, sizeof( buffer ),  "D24      " );
				out += buffer;
				imageDataSize *= 3;
				break;

			case GL_DEPTH_COMPONENT32:
				Com_sprintf( buffer, sizeof( buffer ),  "D32      " );
				out += buffer;
				imageDataSize *= 4;
				break;

			default:
				Com_sprintf( buffer, sizeof( buffer ),  "????     " );
				out += buffer;
				imageDataSize *= 4;
				break;
		}

		switch ( image->wrapType.s )
		{
			case wrapTypeEnum_t::WT_REPEAT:
				Com_sprintf( buffer, sizeof( buffer ),  "s.rept  " );
				out += buffer;
				break;

			case wrapTypeEnum_t::WT_CLAMP:
				Com_sprintf( buffer, sizeof( buffer ),  "s.clmp  " );
				out += buffer;
				break;

			case wrapTypeEnum_t::WT_EDGE_CLAMP:
				Com_sprintf( buffer, sizeof( buffer ),  "s.eclmp " );
				out += buffer;
				break;

			case wrapTypeEnum_t::WT_ZERO_CLAMP:
				Com_sprintf( buffer, sizeof( buffer ),  "s.zclmp " );
				out += buffer;
				break;

			case wrapTypeEnum_t::WT_ALPHA_ZERO_CLAMP:
				Com_sprintf( buffer, sizeof( buffer ),  "s.azclmp" );
				out += buffer;
				break;

			default:
				Com_sprintf( buffer, sizeof( buffer ),  "s.%4i  ", Util::ordinal(image->wrapType.s) );
				out += buffer;
				break;
		}

		switch ( image->wrapType.t )
		{
			case wrapTypeEnum_t::WT_REPEAT:
				Com_sprintf( buffer, sizeof( buffer ),  "t.rept  " );
				out += buffer;
				break;

			case wrapTypeEnum_t::WT_CLAMP:
				Com_sprintf( buffer, sizeof( buffer ),  "t.clmp  " );
				out += buffer;
				break;

			case wrapTypeEnum_t::WT_EDGE_CLAMP:
				Com_sprintf( buffer, sizeof( buffer ),  "t.eclmp " );
				out += buffer;
				break;

			case wrapTypeEnum_t::WT_ZERO_CLAMP:
				Com_sprintf( buffer, sizeof( buffer ),  "t.zclmp " );
				out += buffer;
				break;

			case wrapTypeEnum_t::WT_ALPHA_ZERO_CLAMP:
				Com_sprintf( buffer, sizeof( buffer ),  "t.azclmp" );
				out += buffer;
				break;

			default:
				Com_sprintf( buffer, sizeof( buffer ),  "t.%4i  ", Util::ordinal(image->wrapType.t));
				out += buffer;
				break;
		}

		dataSize += imageDataSize;

		Log::Notice("%s %s", out.c_str(), image->name );
	}

	Log::Notice(" ---------" );
	Log::Notice(" %i total texels (not including mipmaps)", texels );
	Log::Notice(" %d.%02d MB total image memory", dataSize / ( 1024 * 1024 ),
	           ( dataSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
	Log::Notice(" %i total images\n", tr.images.currentElements );
}

//=======================================================================

/*
================
ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function
before or after.
================
*/
static void ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight,
                             bool normalMap )
{
	int      x, y;
	unsigned *inrow, *inrow2;
	unsigned frac, fracstep;
	unsigned p1[ 2048 ], p2[ 2048 ];
	byte     *pix1, *pix2, *pix3, *pix4;
	vec3_t   n, n2, n3, n4;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;

	for ( x = 0; x < outwidth; x++ )
	{
		p1[ x ] = 4 * ( frac >> 16 );
		frac += fracstep;
	}

	frac = 3 * ( fracstep >> 2 );

	for ( x = 0; x < outwidth; x++ )
	{
		p2[ x ] = 4 * ( frac >> 16 );
		frac += fracstep;
	}

	if ( normalMap )
	{
		for ( y = 0; y < outheight; y++ )
		{
			inrow = in + inwidth * ( int )( ( y + 0.25 ) * inheight / outheight );
			inrow2 = in + inwidth * ( int )( ( y + 0.75 ) * inheight / outheight );

			for ( x = 0; x < outwidth; x++ )
			{
				pix1 = ( byte * ) inrow + p1[ x ];
				pix2 = ( byte * ) inrow + p2[ x ];
				pix3 = ( byte * ) inrow2 + p1[ x ];
				pix4 = ( byte * ) inrow2 + p2[ x ];

				n[ 0 ] = Tex_ByteToFloat( pix1[ 0 ] );
				n[ 1 ] = Tex_ByteToFloat( pix1[ 1 ] );
				n[ 2 ] = Tex_ByteToFloat( pix1[ 2 ] );

				n2[ 0 ] = Tex_ByteToFloat( pix2[ 0 ] );
				n2[ 1 ] = Tex_ByteToFloat( pix2[ 1 ] );
				n2[ 2 ] = Tex_ByteToFloat( pix2[ 2 ] );

				n3[ 0 ] = Tex_ByteToFloat( pix3[ 0 ] );
				n3[ 1 ] = Tex_ByteToFloat( pix3[ 1 ] );
				n3[ 2 ] = Tex_ByteToFloat( pix3[ 2 ] );

				n4[ 0 ] = Tex_ByteToFloat( pix4[ 0 ] );
				n4[ 1 ] = Tex_ByteToFloat( pix4[ 1 ] );
				n4[ 2 ] = Tex_ByteToFloat( pix4[ 2 ] );

				VectorAdd( n, n2, n );
				VectorAdd( n, n3, n );
				VectorAdd( n, n4, n );

				if ( !VectorNormalize( n ) )
				{
					VectorSet( n, 0, 0, 1 );
				}

				( ( byte * )( out ) ) [ 0 ] = Tex_FloatToByte( n[ 0 ] );
				( ( byte * )( out ) ) [ 1 ] = Tex_FloatToByte( n[ 1 ] );
				( ( byte * )( out ) ) [ 2 ] = Tex_FloatToByte( n[ 2 ] );
				( ( byte * )( out ) ) [ 3 ] = 255;

				++out;
			}
		}
	}
	else
	{
		for ( y = 0; y < outheight; y++ )
		{
			inrow = in + inwidth * ( int )( ( y + 0.25 ) * inheight / outheight );
			inrow2 = in + inwidth * ( int )( ( y + 0.75 ) * inheight / outheight );

			for ( x = 0; x < outwidth; x++ )
			{
				pix1 = ( byte * ) inrow + p1[ x ];
				pix2 = ( byte * ) inrow + p2[ x ];
				pix3 = ( byte * ) inrow2 + p1[ x ];
				pix4 = ( byte * ) inrow2 + p2[ x ];

				( ( byte * )( out ) ) [ 0 ] = ( pix1[ 0 ] + pix2[ 0 ] + pix3[ 0 ] + pix4[ 0 ] ) >> 2;
				( ( byte * )( out ) ) [ 1 ] = ( pix1[ 1 ] + pix2[ 1 ] + pix3[ 1 ] + pix4[ 1 ] ) >> 2;
				( ( byte * )( out ) ) [ 2 ] = ( pix1[ 2 ] + pix2[ 2 ] + pix3[ 2 ] + pix4[ 2 ] ) >> 2;
				( ( byte * )( out ) ) [ 3 ] = ( pix1[ 3 ] + pix2[ 3 ] + pix3[ 3 ] + pix4[ 3 ] ) >> 2;

				++out;
			}
		}
	}
}

/*
================
R_MipMap2

Operates in place, quartering the size of the texture
Proper linear filter
================
*/
static void R_MipMap2( unsigned *in, int inWidth, int inHeight )
{
	int      i, j, k;
	byte     *outpix;
	int      inWidthMask, inHeightMask;
	int      outWidth, outHeight;
	unsigned *temp;
	byte     *row[ 4 ];

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = (unsigned int*) ri.Hunk_AllocateTempMemory( outWidth * outHeight * 4 );
	outpix = (byte *) temp;

	inWidthMask = ( inWidth << 2 ) - 1; // applied to row indices
	inHeightMask = inHeight - 1; // applied to in indices

	row[ 1 ] = (byte *) &in[( -1 & inHeightMask ) * inWidth ];
	row[ 2 ] = (byte *) &in[   0                            ];
	row[ 3 ] = (byte *) &in[(  1 & inHeightMask ) * inWidth ];

	for ( i = 0; i < inHeight; i += 2 ) // count source, row pairs
	{
		row[ 0 ] = row[ 1 ];
		row[ 1 ] = row[ 2 ];
		row[ 2 ] = row[ 3 ];
		row[ 3 ] = (byte *) &in[ ( ( i + 2 ) & inHeightMask ) * inWidth ];

		for ( j = 0; j < inWidth * 4; j += 8 ) // count source, bytes comprising texel pairs
		{
			for ( k = j; k < j + 4; k++ )
			{
				const int km1 = ( k - 4 ) & inWidthMask;
				const int kp1 = ( k + 4 ) & inWidthMask;
				const int kp2 = ( k + 8 ) & inWidthMask;

				*outpix++ = ( 1 * row[ 0 ][ km1 ] + 2 * row[ 0 ][ k   ] + 2 * row[ 0 ][ kp1 ] + 1 * row[ 0 ][ kp2 ] +
						2 * row[ 1 ][ km1 ] + 4 * row[ 1 ][ k   ] + 4 * row[ 1 ][ kp1 ] + 2 * row[ 1 ][ kp2 ] +
						2 * row[ 2 ][ km1 ] + 4 * row[ 2 ][ k   ] + 4 * row[ 2 ][ kp1 ] + 2 * row[ 2 ][ kp2 ] +
						1 * row[ 3 ][ km1 ] + 2 * row[ 3 ][ k   ] + 2 * row[ 3 ][ kp1 ] + 1 * row[ 3 ][ kp2 ] ) / 36;
			}
		}
	}

	Com_Memcpy( in, temp, outWidth * outHeight * 4 );
	ri.Hunk_FreeTempMemory( temp );
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
static void R_MipMap( byte *in, int width, int height )
{
	int  i, j;
	byte *out;
	int  row;

	if ( !r_simpleMipMaps->integer )
	{
		R_MipMap2( ( unsigned * ) in, width, height );
		return;
	}

	if ( width == 1 && height == 1 )
	{
		return;
	}

	row = width * 4;
	out = in;
	width >>= 1;
	height >>= 1;

	if ( width == 0 || height == 0 )
	{
		width += height; // get largest

		for ( i = 0; i < width; i++, out += 4, in += 8 )
		{
			out[ 0 ] = ( in[ 0 ] + in[ 4 ] ) >> 1;
			out[ 1 ] = ( in[ 1 ] + in[ 5 ] ) >> 1;
			out[ 2 ] = ( in[ 2 ] + in[ 6 ] ) >> 1;
			out[ 3 ] = ( in[ 3 ] + in[ 7 ] ) >> 1;
		}

		return;
	}

	for ( i = 0; i < height; i++, in += row )
	{
		for ( j = 0; j < width; j++, out += 4, in += 8 )
		{
			out[ 0 ] = ( in[ 0 ] + in[ 4 ] + in[ row + 0 ] + in[ row + 4 ] ) >> 2;
			out[ 1 ] = ( in[ 1 ] + in[ 5 ] + in[ row + 1 ] + in[ row + 5 ] ) >> 2;
			out[ 2 ] = ( in[ 2 ] + in[ 6 ] + in[ row + 2 ] + in[ row + 6 ] ) >> 2;
			out[ 3 ] = ( in[ 3 ] + in[ 7 ] + in[ row + 3 ] + in[ row + 7 ] ) >> 2;
		}
	}
}

/*
================
R_MipNormalMap

Operates in place, quartering the size of the texture
================
*/
// *INDENT-OFF*
static void R_MipNormalMap( byte *in, int width, int height )
{
	int    i, j;
	byte   *out;
	vec4_t n;
	vec_t  length;

	if ( width == 1 && height == 1 )
	{
		return;
	}

	out = in;
	width <<= 2;
	height >>= 1;

	for ( i = 0; i < height; i++, in += width )
	{
		for ( j = 0; j < width; j += 8, out += 4, in += 8 )
		{
			// these calculations were centred on 127.5: (p / 255.0 - 0.5) * 2.0
			n[ 0 ] = Tex_ByteToFloat( in[ 0 ] ) +
			         Tex_ByteToFloat( in[ 4 ] ) +
			         Tex_ByteToFloat( in[ width + 0 ] ) +
			         Tex_ByteToFloat( in[ width + 4 ] );

			n[ 1 ] = Tex_ByteToFloat( in[ 1 ] ) +
			         Tex_ByteToFloat( in[ 5 ] ) +
			         Tex_ByteToFloat( in[ width + 1 ] ) +
			         Tex_ByteToFloat( in[ width + 5 ] );

			n[ 2 ] = Tex_ByteToFloat( in[ 2 ] ) +
			         Tex_ByteToFloat( in[ 6 ] ) +
			         Tex_ByteToFloat( in[ width + 2 ] ) +
			         Tex_ByteToFloat( in[ width + 6 ] );

			n[ 3 ] = ( in[ 3 ] + in[ 7 ] + in[ width + 3 ] +  in[ width + 7 ] ) / 255.0f;

			length = VectorLength( n );

			if ( length )
			{
				n[ 0 ] /= length;
				n[ 1 ] /= length;
				n[ 2 ] /= length;
			}
			else
			{
				VectorSet( n, 0.0, 0.0, 1.0 );
			}

			out[ 0 ] = Tex_FloatToByte( n[ 0 ] );
			out[ 1 ] = Tex_FloatToByte( n[ 1 ] );
			out[ 2 ] = Tex_FloatToByte( n[ 2 ] );
			out[ 3 ] = ( byte )( n[ 3 ] * 255.0 / 4.0 );
		}
	}
}

/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture( byte *data, int pixelCount, const byte blend[ 4 ] )
{
	int i;
	int inverseAlpha;
	int premult[ 3 ];

	inverseAlpha = 255 - blend[ 3 ];
	premult[ 0 ] = blend[ 0 ] * blend[ 3 ];
	premult[ 1 ] = blend[ 1 ] * blend[ 3 ];
	premult[ 2 ] = blend[ 2 ] * blend[ 3 ];

	for ( i = 0; i < pixelCount; i++, data += 4 )
	{
		data[ 0 ] = ( data[ 0 ] * inverseAlpha + premult[ 0 ] ) >> 9;
		data[ 1 ] = ( data[ 1 ] * inverseAlpha + premult[ 1 ] ) >> 9;
		data[ 2 ] = ( data[ 2 ] * inverseAlpha + premult[ 2 ] ) >> 9;
	}
}

static const byte mipBlendColors[ 16 ][ 4 ] =
{
	{ 0,   0,   0,   0   }
	,
	{ 255, 0,   0,   128 }
	,
	{ 0,   255, 0,   128 }
	,
	{ 0,   0,   255, 128 }
	,
	{ 255, 0,   0,   128 }
	,
	{ 0,   255, 0,   128 }
	,
	{ 0,   0,   255, 128 }
	,
	{ 255, 0,   0,   128 }
	,
	{ 0,   255, 0,   128 }
	,
	{ 0,   0,   255, 128 }
	,
	{ 255, 0,   0,   128 }
	,
	{ 0,   255, 0,   128 }
	,
	{ 0,   0,   255, 128 }
	,
	{ 255, 0,   0,   128 }
	,
	{ 0,   255, 0,   128 }
	,
	{ 0,   0,   255, 128 }
	,
};

/*
==================
R_UnpackDXT5A
==================
*/
static void
R_UnpackDXT5A( const byte *in, byte *out )
{
	unsigned int bits0, bits1, val;
	int i;

	bits0 = in[2] + (in[3] << 8) + (in[4] << 16);
	bits1 = in[5] + (in[6] << 8) + (in[7] << 16);

	if( in[0] > in[1] ) {
		for( i = 0; i < 8; i++ ) {
			val = bits0 & 7; bits0 >>= 3;
			switch( val ) {
			case 0: val = in[0]; break;
			case 1: val = in[1]; break;
			case 2: val = (6 * in[0] + 1 * in[1]) / 7; break;
			case 3: val = (5 * in[0] + 2 * in[1]) / 7; break;
			case 4: val = (4 * in[0] + 3 * in[1]) / 7; break;
			case 5: val = (3 * in[0] + 4 * in[1]) / 7; break;
			case 6: val = (2 * in[0] + 5 * in[1]) / 7; break;
			case 7: val = (1 * in[0] + 6 * in[1]) / 7; break;
			}
			out[ i ] = val;

			val = bits1 & 7; bits1 >>= 3;
			switch( val ) {
			case 0: val = in[0]; break;
			case 1: val = in[1]; break;
			case 2: val = (6 * in[0] + 1 * in[1]) / 7; break;
			case 3: val = (5 * in[0] + 2 * in[1]) / 7; break;
			case 4: val = (4 * in[0] + 3 * in[1]) / 7; break;
			case 5: val = (3 * in[0] + 4 * in[1]) / 7; break;
			case 6: val = (2 * in[0] + 5 * in[1]) / 7; break;
			case 7: val = (1 * in[0] + 6 * in[1]) / 7; break;
			}
			out[ i + 8 ] = val;
		}
	} else {
		for( i = 0; i < 8; i++ ) {
			val = bits0 & 7; bits0 >>= 3;
			switch( val ) {
			case 0: val = in[0]; break;
			case 1: val = in[1]; break;
			case 2: val = (4 * in[0] + 1 * in[1]) / 5; break;
			case 3: val = (3 * in[0] + 2 * in[1]) / 5; break;
			case 4: val = (2 * in[0] + 3 * in[1]) / 5; break;
			case 5: val = (1 * in[0] + 4 * in[1]) / 5; break;
			case 6: val = 0;                           break;
			case 7: val = 0xff;                        break;
			}
			out[ i ] = val;

			val = bits1 & 7; bits1 >>= 3;
			switch( val ) {
			case 0: val = in[0]; break;
			case 1: val = in[1]; break;
			case 2: val = (4 * in[0] + 1 * in[1]) / 5; break;
			case 3: val = (3 * in[0] + 2 * in[1]) / 5; break;
			case 4: val = (2 * in[0] + 3 * in[1]) / 5; break;
			case 5: val = (1 * in[0] + 4 * in[1]) / 5; break;
			case 6: val = 0;                           break;
			case 7: val = 0xff;                        break;
			}
			out[ i + 8 ] = val;
		}
	}
}

static void
R_PackDXT1_Green( const byte *in, byte *out )
{
	int i;
	byte min, max;
	byte lim1, lim2, lim3;
	unsigned int bits0, bits1;

	min = max = in[ 0 ];
	for( i = 1; i < 16; i++ ) {
		if( in[ i ] < min )
			min = in[ i ];
		if( in[ i ] > max )
			max = in[ i ];
	}

	// truncate min and max to 6 bits
	i = (max - min) >> 3;
	min = (min + i) & 0xfc;
	max = (max - i) & 0xfc;
	if( min == max ) {
		if( max == 0xfc ) {
			min -= 4;
		} else {
			max += 4;
		}
	}
	min |= min >> 6;
	max |= max >> 6;

	// find best match for every pixel
	lim1 = (max + 5 * min) / 6;
	lim2 = (max + min) / 2;
	lim3 = (5 * max + min) / 6;
	bits0 = bits1 = 0;
	for( i = 7; i >= 0; i-- ) {
		bits0 <<= 2;
		bits1 <<= 2;
		if( in[ i ] > lim2 ) {
			if( in[ i ] > lim3 ) {
				bits0 |= 0;
			} else {
				bits0 |= 2;
			}
		} else {
			if( in[ i ] > lim1 ) {
				bits0 |= 3;
			} else {
				bits0 |= 1;
			}
		}
		if( in[ i + 8 ] > lim2 ) {
			if( in[ i + 8 ] > lim3 ) {
				bits1 |= 0;
			} else {
				bits1 |= 2;
			}
		} else {
			if( in[ i + 8 ] > lim1 ) {
				bits1 |= 3;
			} else {
				bits1 |= 1;
			}
		}
	}

	// write back DXT1 format data into out[]
	out[0] = (max & 0x1c) << 3;
	out[1] = 0xf8 | (max >> 5);
	out[2] = (min & 0x1c) << 3;
	out[3] = 0xf8 | (min >> 5);
	out[4] = bits0 & 0xff;
	out[5] = (bits0 >> 8) & 0xff;
	out[6] = bits1 & 0xff;
	out[7] = (bits1 >> 8) & 0xff;
}

/*
==================
R_ConvertBC5Image

If the OpenGL doesn't support BC5 (= OpenGL rgtc) textures, convert
them to BC3 (= dxt5). Sampling the original texture yields RGBA = XY01,
the converted texture yields RGBA = 1Y0X, so the shader can reconstruct
XY as vec2(tex.x * tex.a, tex.y).
==================
*/
static void
R_ConvertBC5Image(const byte **in, byte **out, int numMips, int numLayers,
		  int width, int height, bool is3D )
{
	int blocks = 0;
	int mipWidth, mipHeight, mipLayers, mipSize;
	byte data[16], *to;
	const byte *from;
	int i, j, k;

	// Allocate buffer for converted images
	mipWidth = width;
	mipHeight = height;
	mipLayers = numLayers;
	for ( i = 0; i < numMips; i++ ) {
		mipSize = ((mipWidth + 3) >> 2) * ((mipHeight + 3) >> 2);

		blocks += mipSize * mipLayers;

		if( mipWidth > 1 )
			mipWidth >>= 1;
		if( mipHeight > 1 )
			mipHeight >>= 1;
		if( is3D && mipLayers > 1 )
			mipLayers >>= 1;
	}

	*out = to = (byte *)ri.Hunk_AllocateTempMemory( blocks * 16 );

	// Convert each mipmap
	mipWidth = width;
	mipHeight = height;
	mipLayers = numLayers;
	for ( i = 0; i < numMips; i++ ) {
		mipSize = ((mipWidth + 3) >> 2) * ((mipHeight + 3) >> 2);

		for( j = 0; j < mipLayers; j++ ) {
			from = in[ i * numLayers + j ];
			in [ i * numLayers + j ] = to;

			for( k = 0; k < mipSize; k++ ) {
				// red channel is unchanged
				Com_Memcpy( to, from, 8 );

				// green channel is converted to DXT1
				R_UnpackDXT5A( from + 8, data );
				R_PackDXT1_Green( data, to + 8 );

				from += 16; to += 16;
			}
		}

		if( mipWidth > 1 )
			mipWidth >>= 1;
		if( mipHeight > 1 )
			mipHeight >>= 1;
		if( is3D && mipLayers > 1 )
			mipLayers >>= 1;
	}
}

/*
===============
R_UploadImage

dataArray points to an array of numLayers * numMips pointers, first all layers
of mip level 0, then all layers of mip level 1, etc.

Note that for 3d textures the higher mip levels have fewer layers, e.g. mip
level 1 has only numLayers/2 layers. There are still numLayers pointers in
the dataArray for every mip level, the unneeded elements at the end aren't used.
===============
*/
void R_UploadImage( const byte **dataArray, int numLayers, int numMips,
		    image_t *image )
{
	const byte *data;
	byte       *scaledBuffer = nullptr;
	int        scaledWidth, scaledHeight;
	int        mipWidth, mipHeight, mipLayers, mipSize, blockSize;
	int        i, j, c;
	const byte *scan;
	GLenum     target;
	GLenum     format = GL_RGBA;
	GLenum     internalFormat = GL_RGB;

	static const vec4_t oneClampBorder = { 1, 1, 1, 1 };
	static const vec4_t zeroClampBorder = { 0, 0, 0, 1 };
	static const vec4_t alphaZeroClampBorder = { 0, 0, 0, 0 };

	if ( numMips <= 0 )
		numMips = 1;

	GL_Bind( image );

	if ( glConfig2.textureNPOTAvailable )
	{
		scaledWidth = image->width;
		scaledHeight = image->height;
	}
	else
	{
		// convert to exact power of 2 sizes
		for ( scaledWidth = 1; scaledWidth < image->width; scaledWidth <<= 1 )
		{
			;
		}

		for ( scaledHeight = 1; scaledHeight < image->height; scaledHeight <<= 1 )
		{
			;
		}
	}

	if ( r_roundImagesDown->integer && scaledWidth > image->width )
	{
		scaledWidth >>= 1;
	}

	if ( r_roundImagesDown->integer && scaledHeight > image->height )
	{
		scaledHeight >>= 1;
	}

	// perform optional picmip operation
	if ( !( image->bits & IF_NOPICMIP ) )
	{
		int picmip = r_picmip->integer;
		if( picmip < 0 )
			picmip = 0;

		scaledWidth >>= picmip;
		scaledHeight >>= picmip;

		if( dataArray && numMips > picmip ) {
			dataArray += numLayers * picmip;
			numMips -= picmip;
		}
	}

	// clamp to minimum size
	if ( scaledWidth < 1 )
	{
		scaledWidth = 1;
	}

	if ( scaledHeight < 1 )
	{
		scaledHeight = 1;
	}

	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	if ( image->type == GL_TEXTURE_CUBE_MAP )
	{
		while ( scaledWidth > glConfig2.maxCubeMapTextureSize || scaledHeight > glConfig2.maxCubeMapTextureSize )
		{
			scaledWidth >>= 1;
			scaledHeight >>= 1;
		}
	}
	else
	{
		while ( scaledWidth > glConfig.maxTextureSize || scaledHeight > glConfig.maxTextureSize )
		{
			scaledWidth >>= 1;
			scaledHeight >>= 1;
		}
	}

	// set target
	switch ( image->type )
	{
		case GL_TEXTURE_3D:
			target = GL_TEXTURE_3D;
			break;

		case GL_TEXTURE_CUBE_MAP:
			target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
			break;

		default:
			target = GL_TEXTURE_2D;
			break;
	}

	if ( image->bits & ( IF_DEPTH16 | IF_DEPTH24 | IF_DEPTH32 ) )
	{
		format = GL_DEPTH_COMPONENT;

		if ( image->bits & IF_DEPTH16 )
		{
			internalFormat = GL_DEPTH_COMPONENT16;
		}
		else if ( image->bits & IF_DEPTH24 )
		{
			internalFormat = GL_DEPTH_COMPONENT24;
		}
		else if ( image->bits & IF_DEPTH32 )
		{
			internalFormat = GL_DEPTH_COMPONENT32;
		}
	}
	else if ( image->bits & ( IF_PACKED_DEPTH24_STENCIL8 ) )
	{
		format = GL_DEPTH_STENCIL_EXT;
		internalFormat = GL_DEPTH24_STENCIL8_EXT;
	}
	else if ( image->bits & ( IF_RGBA16F | IF_RGBA32F | IF_RGBA16 | IF_TWOCOMP16F | IF_TWOCOMP32F | IF_ONECOMP16F | IF_ONECOMP32F ) )
	{
		if( !glConfig2.textureFloatAvailable ) {
			Log::Warn("floating point image '%s' cannot be loaded", image->name );
			internalFormat = GL_RGBA8;
		}
		else if ( image->bits & IF_RGBA16F )
		{
			internalFormat = GL_RGBA16F;
		}
		else if ( image->bits & IF_RGBA32F )
		{
			internalFormat = GL_RGBA32F;
		}
		else if ( image->bits & IF_TWOCOMP16F )
		{
			internalFormat = glConfig2.textureRGAvailable ?
			  GL_RG16F : GL_LUMINANCE_ALPHA16F_ARB;
		}
		else if ( image->bits & IF_TWOCOMP32F )
		{
			internalFormat = glConfig2.textureRGAvailable ?
			  GL_RG32F : GL_LUMINANCE_ALPHA32F_ARB;
		}
		else if ( image->bits & IF_RGBA16 )
		{
			internalFormat = GL_RGBA16;
		}
		else if ( image->bits & IF_ONECOMP16F )
		{
			internalFormat = glConfig2.textureRGAvailable ?
			  GL_R16F : GL_ALPHA16F_ARB;
		}
		else if ( image->bits & IF_ONECOMP32F )
		{
			internalFormat = glConfig2.textureRGAvailable ?
			  GL_R32F : GL_ALPHA32F_ARB;
		}
	}
	else if ( image->bits & ( IF_RGBA32UI ) )
	{
		if( !glConfig2.textureIntegerAvailable ) {
			Log::Warn( "integer image '%s' cannot be loaded\n", image->name );
		}
		internalFormat = GL_RGBA32UI;
		format = GL_RGBA_INTEGER;
	}
	else if ( IsImageCompressed( image->bits ) )
	{
		if( !GLEW_EXT_texture_compression_dxt1 &&
		    !GLEW_EXT_texture_compression_s3tc ) {
			Log::Warn("compressed image '%s' cannot be loaded", image->name );
			internalFormat = GL_RGBA8;
		}
		else if( image->bits & IF_BC1 ) {
			format = GL_NONE;
			internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			blockSize = 8;
		}
		else if ( image->bits & IF_BC3 ) {
			format = GL_NONE;
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			blockSize = 16;
		}
		else if ( image->bits & IF_BC4 ) {
			if( !GLEW_ARB_texture_compression_rgtc ) {
				format = GL_NONE;
				internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				blockSize = 8;

				if( dataArray ) {
					// convert to BC1/dxt1
				}
			}
			else {
				format = GL_NONE;
				internalFormat = GL_COMPRESSED_RED_RGTC1;
				blockSize = 8;
			}
		}
		else if ( image->bits & IF_BC5 ) {
			if( !GLEW_ARB_texture_compression_rgtc ) {
				format = GL_NONE;
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				blockSize = 16;

				R_ConvertBC5Image( dataArray, &scaledBuffer,
						   numMips, numLayers,
						   scaledWidth, scaledHeight,
						   image->type == GL_TEXTURE_3D );
			}
			else {
				format = GL_NONE;
				internalFormat = GL_COMPRESSED_RG_RGTC2;
				blockSize = 16;
			}
		}
	}
	else if ( image->bits & IF_RGBE )
	{
		internalFormat = GL_RGBA8;
	}
	else if ( !dataArray ) {
		internalFormat = GL_RGBA8;
	}
	else
	{
		int samples;

		// scan the texture for each channel's max values
		// and verify if the alpha channel is being used or not
		c = image->width * image->height;
		scan = dataArray[0];

		samples = 3;

		// Tr3B: normalmaps have the displacement maps in the alpha channel
		// samples 3 would cause an opaque alpha channel and odd displacements!
		if ( image->bits & IF_NORMALMAP )
		{
			if ( image->bits & ( IF_DISPLACEMAP | IF_ALPHATEST ) )
			{
				samples = 4;
			}
			else
			{
				samples = 3;
			}
		}
		else if ( image->bits & IF_LIGHTMAP )
		{
			samples = 3;
		}
		else
		{
			for ( i = 0; i < c; i++ )
			{
				if ( scan[ i * 4 + 3 ] != 255 )
				{
					samples = 4;
					break;
				}
			}
		}

		// select proper internal format
		if ( samples == 3 )
		{
			if ( glConfig.textureCompression == textureCompression_t::TC_S3TC && !(image->bits & IF_NOCOMPRESSION ) )
			{
				internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			}
			else
			{
				internalFormat = GL_RGB8;
			}
		}
		else if ( samples == 4 )
		{
			if ( image->bits & IF_ALPHA )
			{
				internalFormat = GL_ALPHA8;
			}
			else
			{
				if ( glConfig.textureCompression == textureCompression_t::TC_S3TC && !( image->bits & IF_NOCOMPRESSION ) )
				{
					if ( image->bits & IF_DISPLACEMAP )
					{
						internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
					}
					else if ( image->bits & IF_ALPHATEST )
					{
						internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
					}
					else
					{
						internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
					}
				}
				else
				{
					internalFormat = GL_RGBA8;
				}
			}
		}
	}

	// 3D textures are uploaded in slices via glTexSubImage3D,
	// so the storage has to be allocated before the loop
	if( image->type == GL_TEXTURE_3D ) {
		mipWidth = scaledWidth;
		mipHeight = scaledHeight;
		mipLayers = numLayers;

		for( i = 0; i < numMips; i++ ) {
			glTexImage3D( GL_TEXTURE_3D, i, internalFormat,
				      scaledWidth, scaledHeight, mipLayers,
				      0, format, GL_UNSIGNED_BYTE, nullptr );

			if( mipWidth  > 1 ) mipWidth  >>= 1;
			if( mipHeight > 1 ) mipHeight >>= 1;
			if( mipLayers > 1 ) mipLayers >>= 1;
		}
	}

	if( format != GL_NONE ) {
		if( dataArray )
			scaledBuffer = (byte*) ri.Hunk_AllocateTempMemory( sizeof( byte ) * scaledWidth * scaledHeight * 4 );

		for ( i = 0; i < numLayers; i++ )
		{
			if( dataArray )
				data = dataArray[ i ];
			else
				data = nullptr;

			if( scaledBuffer )
			{
				// copy or resample data as appropriate for first MIP level
				if ( ( scaledWidth == image->width ) && ( scaledHeight == image->height ) )
				{
					Com_Memcpy( scaledBuffer, data, scaledWidth * scaledHeight * 4 );
				}
				else
				{
					ResampleTexture( ( unsigned * ) data, image->width, image->height, ( unsigned * ) scaledBuffer, scaledWidth, scaledHeight,
							 ( image->bits & IF_NORMALMAP ) );
				}

				if( image->bits & IF_NORMALMAP ) {
					c = scaledWidth * scaledHeight;
					for ( i = 0; i < c; i++ )
					{
						vec3_t n;

						n[ 0 ] = Tex_ByteToFloat( scaledBuffer[ i * 4 + 0 ] );
						n[ 1 ] = Tex_ByteToFloat( scaledBuffer[ i * 4 + 1 ] );
						n[ 2 ] = Tex_ByteToFloat( scaledBuffer[ i * 4 + 2 ] );

						VectorNormalize( n );

						scaledBuffer[ i * 4 + 0 ] = Tex_FloatToByte( n[ 0 ] );
						scaledBuffer[ i * 4 + 1 ] = Tex_FloatToByte( n[ 1 ] );
						scaledBuffer[ i * 4 + 2 ] = Tex_FloatToByte( n[ 2 ] );
					}
				}
			}

			image->uploadWidth = scaledWidth;
			image->uploadHeight = scaledHeight;
			image->internalFormat = internalFormat;

			switch ( image->type )
			{
			case GL_TEXTURE_3D:
				if( scaledBuffer ) {
					glTexSubImage3D( GL_TEXTURE_3D, 0, 0, 0, i,
							 scaledWidth, scaledHeight, 1,
							 format, GL_UNSIGNED_BYTE,
							 scaledBuffer );
				}
				break;
			case GL_TEXTURE_CUBE_MAP:
				glTexImage2D( target + i, 0, internalFormat, scaledWidth, scaledHeight, 0, format, GL_UNSIGNED_BYTE,
				              scaledBuffer );
				break;

			default:
				if ( image->bits & IF_PACKED_DEPTH24_STENCIL8 )
				{
					glTexImage2D( target, 0, internalFormat, scaledWidth, scaledHeight, 0, format, GL_UNSIGNED_INT_24_8_EXT, nullptr );
				}
				else
				{
					glTexImage2D( target, 0, internalFormat, scaledWidth, scaledHeight, 0, format, GL_UNSIGNED_BYTE, scaledBuffer );
				}

				break;
			}

			if ( image->filterType == filterType_t::FT_DEFAULT )
			{
				if( image->type != GL_TEXTURE_CUBE_MAP || i == 5 ) {
					glGenerateMipmap( image->type );
					glTexParameteri( image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );  // default to trilinear
				}
			}
		}
	} else {
		// already compressed texture data, precomputed mipmaps must be
		// in the data array
		image->uploadWidth = scaledWidth;
		image->uploadHeight = scaledHeight;
		image->internalFormat = internalFormat;

		mipWidth = scaledWidth;
		mipHeight = scaledHeight;
		mipLayers = numLayers;

		for ( i = 0; i < numMips; i++ )
		{
			mipSize = ((mipWidth + 3) >> 2)
			  * ((mipHeight + 3) >> 2) * blockSize;

			for ( j = 0; j < mipLayers; j++ ) {
				if( dataArray )
					data = dataArray[ j * numMips + i ];
				else
					data = nullptr;

				switch ( image->type )
				{
				case GL_TEXTURE_3D:
					glCompressedTexSubImage3D( GL_TEXTURE_3D, i, 0, 0, j,
								   scaledWidth, scaledHeight, 1,
								   internalFormat, mipSize, data );
					break;
				case GL_TEXTURE_CUBE_MAP:
					glCompressedTexImage2D( target + j, i, internalFormat, mipWidth, mipHeight, 0, mipSize, data );
					break;

				default:
					glCompressedTexImage2D( target, i, internalFormat, mipWidth, mipHeight, 0, mipSize, data );
					break;
				}

				if( mipWidth > 1 )
					mipWidth >>= 1;
				if( mipHeight > 1 )
					mipHeight >>= 1;
				if( image->type == GL_TEXTURE_3D && mipLayers > 1 )
					mipLayers >>= 1;
			}
		}
	}

	GL_CheckErrors();

	// set filter type
	switch ( image->filterType )
	{
		case filterType_t::FT_DEFAULT:

			// set texture anisotropy
			if ( glConfig2.textureAnisotropyAvailable )
			{
				glTexParameterf( image->type, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_ext_texture_filter_anisotropic->value );
			}

			glTexParameterf( image->type, GL_TEXTURE_MIN_FILTER, gl_filter_min );
			glTexParameterf( image->type, GL_TEXTURE_MAG_FILTER, gl_filter_max );
			break;

		case filterType_t::FT_LINEAR:
			glTexParameterf( image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( image->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			break;

		case filterType_t::FT_NEAREST:
			glTexParameterf( image->type, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameterf( image->type, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			break;

		default:
			Log::Warn("unknown filter type for image '%s'", image->name );
			glTexParameterf( image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameterf( image->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			break;
	}

	GL_CheckErrors();

	// set wrap type
	if ( image->wrapType.s == image->wrapType.t )
        {
                switch ( image->wrapType.s )
                {
                        case wrapTypeEnum_t::WT_REPEAT:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_REPEAT );
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_REPEAT );
                                break;

                        case wrapTypeEnum_t::WT_CLAMP:
                        case wrapTypeEnum_t::WT_EDGE_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
                                break;
                        case wrapTypeEnum_t::WT_ONE_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
                                glTexParameterfv( image->type, GL_TEXTURE_BORDER_COLOR, oneClampBorder );
                                break;
                        case wrapTypeEnum_t::WT_ZERO_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
                                glTexParameterfv( image->type, GL_TEXTURE_BORDER_COLOR, zeroClampBorder );
                                break;

                        case wrapTypeEnum_t::WT_ALPHA_ZERO_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
                                glTexParameterfv( image->type, GL_TEXTURE_BORDER_COLOR, alphaZeroClampBorder );
                                break;

                        default:
								Log::Warn("unknown wrap type for image '%s'", image->name );
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_REPEAT );
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_REPEAT );
                                break;
                }
        } else {
        	// warn about mismatched clamp types if both require a border colour to be set
                if ( ( image->wrapType.s == wrapTypeEnum_t::WT_ZERO_CLAMP || image->wrapType.s == wrapTypeEnum_t::WT_ONE_CLAMP || image->wrapType.s == wrapTypeEnum_t::WT_ALPHA_ZERO_CLAMP ) &&
	             ( image->wrapType.t == wrapTypeEnum_t::WT_ZERO_CLAMP || image->wrapType.t == wrapTypeEnum_t::WT_ONE_CLAMP || image->wrapType.t == wrapTypeEnum_t::WT_ALPHA_ZERO_CLAMP ) )
        	{
					Log::Warn("mismatched wrap types for image '%s'", image->name );
                }

                switch ( image->wrapType.s )
                {
                        case wrapTypeEnum_t::WT_REPEAT:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_REPEAT );
                                break;

                        case wrapTypeEnum_t::WT_CLAMP:
                        case wrapTypeEnum_t::WT_EDGE_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
                                break;

                        case wrapTypeEnum_t::WT_ONE_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
                                glTexParameterfv( image->type, GL_TEXTURE_BORDER_COLOR, oneClampBorder );
                                break;

                        case wrapTypeEnum_t::WT_ZERO_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
                                glTexParameterfv( image->type, GL_TEXTURE_BORDER_COLOR, zeroClampBorder );
                                break;

                        case wrapTypeEnum_t::WT_ALPHA_ZERO_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
                                glTexParameterfv( image->type, GL_TEXTURE_BORDER_COLOR, alphaZeroClampBorder );
                                break;

                        default:
								Log::Warn("unknown wrap type for image '%s' axis S", image->name );
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_S, GL_REPEAT );
                                break;
                }

                switch ( image->wrapType.t )
                {
                        case wrapTypeEnum_t::WT_REPEAT:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_REPEAT );
                                break;

                        case wrapTypeEnum_t::WT_CLAMP:
                        case wrapTypeEnum_t::WT_EDGE_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
                                break;

                        case wrapTypeEnum_t::WT_ONE_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
                                glTexParameterfv( image->type, GL_TEXTURE_BORDER_COLOR, oneClampBorder );
                                break;

                        case wrapTypeEnum_t::WT_ZERO_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
                                glTexParameterfv( image->type, GL_TEXTURE_BORDER_COLOR, zeroClampBorder );
                                break;

                        case wrapTypeEnum_t::WT_ALPHA_ZERO_CLAMP:
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
                                glTexParameterfv( image->type, GL_TEXTURE_BORDER_COLOR, alphaZeroClampBorder );
                                break;

                        default:
								Log::Warn("unknown wrap type for image '%s' axis T", image->name );
                                glTexParameterf( image->type, GL_TEXTURE_WRAP_T, GL_REPEAT );
                                break;
                }
        }

	GL_CheckErrors();

	if ( scaledBuffer != 0 )
	{
		ri.Hunk_FreeTempMemory( scaledBuffer );
	}

	GL_Unbind( image );
}

/*
================
R_AllocImage
================
*/
image_t        *R_AllocImage( const char *name, bool linkIntoHashTable )
{
	image_t *image;
	long    hash;
	char    buffer[ 1024 ];

	if ( strlen( name ) >= 1024 )
	{
		ri.Error(errorParm_t::ERR_DROP, "R_AllocImage: \"%s\" image name is too long", name );
		return nullptr;
	}

	image = (image_t*) ri.Hunk_Alloc( sizeof( image_t ), ha_pref::h_low );
	Com_Memset( image, 0, sizeof( image_t ) );

	glGenTextures( 1, &image->texnum );

	Com_AddToGrowList( &tr.images, image );

	Q_strncpyz( image->name, name, sizeof( image->name ) );

	if ( linkIntoHashTable )
	{
		Q_strncpyz( buffer, name, sizeof( buffer ) );
		hash = GenerateImageHashValue( buffer );
		image->next = r_imageHashTable[ hash ];
		r_imageHashTable[ hash ] = image;
	}

	return image;
}

/*
================
================
*/
static void R_ExportTexture( image_t *image )
{
	char path[ 1024 ];
	int i;


	Com_sprintf( path, sizeof( path ), "texexp/%s.ktx",
		     image->name );

	// quick and dirty sanitize path name
	for( i = strlen( path ) - 1; i >= 7; i-- ) {
		if( !Str::cisalnum( path[ i ] ) && path[ i ] != '.' && path[ i ] != '-' ) {
			path[ i ] = 'z';
		}
	}
	SaveImageKTX( path, image );
}

/*
================
R_CreateImage
================
*/
image_t        *R_CreateImage( const char *name, const byte **pic, int width, int height,
			       int numMips, int bits, filterType_t filterType, wrapType_t wrapType )
{
	image_t *image;

	image = R_AllocImage( name, true );

	if ( !image )
	{
		return nullptr;
	}

	image->type = GL_TEXTURE_2D;

	image->width = width;
	image->height = height;

	image->bits = bits;
	image->filterType = filterType;
	image->wrapType = wrapType;

	R_UploadImage( pic, 1, numMips, image );

	if( r_exportTextures->integer ) {
		R_ExportTexture( image );
	}

	return image;
}

/*
================
R_CreateGlyph
================
*/
image_t *R_CreateGlyph( const char *name, const byte *pic, int width, int height )
{
	image_t *image = R_AllocImage( name, true );

	if ( !image )
	{
		return nullptr;
	}

	image->type = GL_TEXTURE_2D;
	image->width = width;
	image->height = height;
	image->bits = IF_NOPICMIP;
	image->filterType = filterType_t::FT_LINEAR;
	image->wrapType = wrapTypeEnum_t::WT_CLAMP;

	GL_Bind( image );

	image->uploadWidth = width;
	image->uploadHeight = height;
	image->internalFormat = GL_RGBA;

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pic );

	GL_CheckErrors();

	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	GL_CheckErrors();

	GL_Unbind( image );

	return image;
}

/*
================
R_CreateCubeImage
================
*/
image_t        *R_CreateCubeImage( const char *name,
                                   const byte *pic[ 6 ],
                                   int width, int height, int bits, filterType_t filterType, wrapType_t wrapType )
{
	image_t *image;

	image = R_AllocImage( name, true );

	if ( !image )
	{
		return nullptr;
	}

	image->type = GL_TEXTURE_CUBE_MAP;

	image->width = width;
	image->height = height;

	image->bits = bits;
	image->filterType = filterType;
	image->wrapType = wrapType;

	R_UploadImage( pic, 6, 1, image );

	if( r_exportTextures->integer ) {
		R_ExportTexture( image );
	}

	return image;
}

/*
================
R_Create3DImage
================
*/
image_t        *R_Create3DImage( const char *name,
				 const byte *pic,
				 int width, int height, int depth,
				 int bits, filterType_t filterType,
				 wrapType_t wrapType )
{
	image_t *image;
	const byte **pics;
	int i;

	image = R_AllocImage( name, true );

	if ( !image )
	{
		return nullptr;
	}

	image->type = GL_TEXTURE_3D;

	image->width = width;
	image->height = height;

	if( pic ) {
		pics = (const byte**) ri.Hunk_AllocateTempMemory( depth * sizeof(const byte *) );
		for( i = 0; i < depth; i++ ) {
			pics[i] = pic + i * width * height * sizeof(u8vec4_t);
		}
	} else {
		pics = nullptr;
	}

	image->bits = bits;
	image->filterType = filterType;
	image->wrapType = wrapType;

	R_UploadImage( pics, depth, 1, image );

	if( pics ) {
		ri.Hunk_FreeTempMemory( pics );
	}

	if( r_exportTextures->integer ) {
		R_ExportTexture( image );
	}

	return image;
}

struct imageExtToLoaderMap_t
{
	const char *ext;
	void ( *ImageLoader )( const char *, unsigned char **, int *, int *, int *, int *, int *, byte );
};

// Note that the ordering indicates the order of preference used
// when there are multiple images of different formats available
static const imageExtToLoaderMap_t imageLoaders[] =
{
	{ "webp", LoadWEBP },
	{ "png",  LoadPNG  },
	{ "tga",  LoadTGA  },
	{ "jpg",  LoadJPG  },
	{ "jpeg", LoadJPG  },
	{ "dds",  LoadDDS  },
	{ "crn",  LoadCRN  },
	{ "ktx",  LoadKTX  },
};

static int                   numImageLoaders = ARRAY_LEN( imageLoaders );

/*
=================
R_LoadImage

Loads any of the supported image types into a canonical
32 bit format.
=================
*/
static void R_LoadImage( const char **buffer, byte **pic, int *width, int *height,
			 int *numLayers, int *numMips,
			 int *bits )
{
	char *token;

	*pic = nullptr;
	*width = 0;
	*height = 0;

	token = COM_ParseExt2( buffer, false );

	if ( !token[ 0 ] )
	{
		Log::Warn("NULL parameter for R_LoadImage" );
		return;
	}

	int        i;
	const char *ext;
	char       filename[ MAX_QPATH ];
	byte       alphaByte;

	// Tr3B: clear alpha of normalmaps for displacement mapping
	if ( *bits & IF_NORMALMAP )
	{
		alphaByte = 0x00;
	}
	else
	{
		alphaByte = 0xFF;
	}

	Q_strncpyz( filename, token, sizeof( filename ) );

	ext = COM_GetExtension( filename );

	if ( *ext )
	{
		// look for the correct loader and use it
		for ( i = 0; i < numImageLoaders; i++ )
		{
			if ( !Q_stricmp( ext, imageLoaders[ i ].ext ) )
			{
				// load
				imageLoaders[ i ].ImageLoader( filename, pic, width, height, numLayers, numMips, bits, alphaByte );
				break;
			}
		}

		// a loader was found
		if ( i < numImageLoaders )
		{
			if ( *pic == nullptr )
			{
				// loader failed, most likely because the file isn't there;
				// try again without the extension
				COM_StripExtension3( token, filename, MAX_QPATH );
			}
			else
			{
				// something loaded
				return;
			}
		}
	}

	int bestLoader = -1;
	const FS::PakInfo* bestPak = nullptr;

	// try and find a suitable match using all the image formats supported
	// prioritize with the pak priority
	for ( i = 0; i < numImageLoaders; i++ )
	{
		std::string altName = Str::Format("%s.%s", filename, imageLoaders[i].ext);
		const FS::PakInfo* pak = FS::PakPath::LocateFile(altName);

		// We found a file and its pak is better than the best pak we have
		// this relies on how the filesystem works internally and should be moved
		// to a more explicit interface once there is one. (FIXME)
		if ( pak != nullptr && (bestPak == nullptr || pak < bestPak ) )
		{
			bestPak = pak;
			bestLoader = i;
		}
	}

	if ( bestLoader >= 0 )
	{
		char *altName = va( "%s.%s", filename, imageLoaders[ bestLoader ].ext );
		imageLoaders[ bestLoader ].ImageLoader( altName, pic, width, height, numLayers, numMips, bits, alphaByte );
	}
}

/*
===============
R_FindImageFile

Finds or loads the given image.
Returns nullptr if it fails, not a default image.
==============
*/
image_t        *R_FindImageFile( const char *imageName, int bits, filterType_t filterType, wrapType_t wrapType )
{
	image_t       *image = nullptr;
	int           width = 0, height = 0, numLayers = 0, numMips = 0;
	byte          *pic[ MAX_TEXTURE_MIPS * MAX_TEXTURE_LAYERS ];
	byte          *mallocPtr = nullptr;
	long          hash;
	char          buffer[ 1024 ];
	const char          *buffer_p;
	unsigned long diff;

	if ( !imageName )
	{
		return nullptr;
	}

	Q_strncpyz( buffer, imageName, sizeof( buffer ) );
	hash = GenerateImageHashValue( buffer );

	// see if the image is already loaded
	for ( image = r_imageHashTable[ hash ]; image; image = image->next )
	{
		if ( !Q_strnicmp( buffer, image->name, sizeof( image->name ) ) )
		{
			// the white image can be used with any set of parms, but other mismatches are errors
			if ( Q_stricmp( buffer, "_white" ) )
			{
				diff = bits ^ image->bits;

				if ( diff & IF_NOPICMIP )
				{
					Log::Warn("reused image '%s' with mixed allowPicmip parm for shader", imageName );
				}

				if ( image->wrapType != wrapType )
				{
					Log::Warn("reused image '%s' with mixed glWrapType parm for shader", imageName);
				}
			}

			return image;
		}
	}

	// load the pic from disk
	pic[ 0 ] = nullptr;
	buffer_p = &buffer[ 0 ];
	R_LoadImage( &buffer_p, pic, &width, &height, &numLayers, &numMips, &bits );

	if ( (mallocPtr = pic[ 0 ]) == nullptr || numLayers > 0 )
	{
		if ( mallocPtr )
		{
			ri.Free( mallocPtr );
		}
		return nullptr;
	}

	if ( bits & IF_LIGHTMAP )
	{
		R_ProcessLightmap( pic[ 0 ], 4, width, height, bits, pic[ 0 ] );

		bits |= IF_NOCOMPRESSION;
	}

	image = R_CreateImage( ( char * ) buffer, (const byte **)pic,
			       width, height, numMips, bits,
			       filterType, wrapType );

	ri.Free( mallocPtr );
	return image;
}

static void R_Flip( byte *in, int width, int height )
{
	int32_t *data = (int32_t *) in;
	int x, y;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width / 2; x++ )
		{
			int32_t texel = data[ x ];
			data[ x ] = data[ width - 1 - x ];
			data[ width - 1 - x ] = texel;
		}
		data += width;
	}
}

static void R_Flop( byte *in, int width, int height )
{
	int32_t *upper = (int32_t *) in;
	int32_t *lower = (int32_t *) in + ( height - 1 ) * width;
	int     x, y;

	for ( y = 0; y < height / 2; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			int32_t texel = upper[ x ];
			upper[ x ] = lower[ x ];
			lower[ x ] = texel;
		}

		upper += width;
		lower -= width;
	}
}

static void R_Rotate( byte *in, int width, int height, int degrees )
{
	byte color[ 4 ];
	int  x, y, x2, y2;
	byte *out, *tmp;

	tmp = (byte*) ri.Hunk_AllocateTempMemory( width * height * 4 );

	// rotate into tmp buffer
	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			color[ 0 ] = in[ 4 * ( y * width + x ) + 0 ];
			color[ 1 ] = in[ 4 * ( y * width + x ) + 1 ];
			color[ 2 ] = in[ 4 * ( y * width + x ) + 2 ];
			color[ 3 ] = in[ 4 * ( y * width + x ) + 3 ];

			if ( degrees == 90 )
			{
				x2 = y;
				y2 = ( height - ( 1 + x ) );

				tmp[ 4 * ( y2 * width + x2 ) + 0 ] = color[ 0 ];
				tmp[ 4 * ( y2 * width + x2 ) + 1 ] = color[ 1 ];
				tmp[ 4 * ( y2 * width + x2 ) + 2 ] = color[ 2 ];
				tmp[ 4 * ( y2 * width + x2 ) + 3 ] = color[ 3 ];
			}
			else if ( degrees == -90 )
			{
				x2 = ( width - ( 1 + y ) );
				y2 = x;

				tmp[ 4 * ( y2 * width + x2 ) + 0 ] = color[ 0 ];
				tmp[ 4 * ( y2 * width + x2 ) + 1 ] = color[ 1 ];
				tmp[ 4 * ( y2 * width + x2 ) + 2 ] = color[ 2 ];
				tmp[ 4 * ( y2 * width + x2 ) + 3 ] = color[ 3 ];
			}
			else
			{
				tmp[ 4 * ( y * width + x ) + 0 ] = color[ 0 ];
				tmp[ 4 * ( y * width + x ) + 1 ] = color[ 1 ];
				tmp[ 4 * ( y * width + x ) + 2 ] = color[ 2 ];
				tmp[ 4 * ( y * width + x ) + 3 ] = color[ 3 ];
			}
		}
	}

	// copy back to input
	out = in;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			out[ 4 * ( y * width + x ) + 0 ] = tmp[ 4 * ( y * width + x ) + 0 ];
			out[ 4 * ( y * width + x ) + 1 ] = tmp[ 4 * ( y * width + x ) + 1 ];
			out[ 4 * ( y * width + x ) + 2 ] = tmp[ 4 * ( y * width + x ) + 2 ];
			out[ 4 * ( y * width + x ) + 3 ] = tmp[ 4 * ( y * width + x ) + 3 ];
		}
	}

	ri.Hunk_FreeTempMemory( tmp );
}

/*
===============
R_FindCubeImage

Finds or loads the given image.
Returns nullptr if it fails, not a default image.

Tr3B: fear the use of goto
==============
*/
static void R_FreeCubePics( byte **pic, int count )
{
	while (--count >= 0)
	{
		if ( pic[ count ] )
		{
			ri.Free( pic[ count ] );
		}
	}
}

image_t        *R_FindCubeImage( const char *imageName, int bits, filterType_t filterType, wrapType_t wrapType )
{
	int         i;
	image_t     *image = nullptr;
	int         width = 0, height = 0, numLayers = 0, numMips = 0;
	byte        *pic[ MAX_TEXTURE_MIPS * MAX_TEXTURE_LAYERS ];
	long        hash;
	int         numPicsToFree = 0;

	static const char *openglSuffices[ 6 ] = { "px", "nx", "py", "ny", "pz", "nz" };

	static const char *doom3Suffices[ 6 ] = { "forward", "back", "left", "right", "up", "down" };
	static bool doom3FlipX[ 6 ] = { true,  true,  false, true,  true,  false };
	static bool doom3FlipY[ 6 ] = { false, false, true,  false, false, true };
	static int      doom3Rot[ 6 ] = { 90,           -90,    0,              0,              90,             -90 };

	static const char *quakeSuffices[ 6 ] = { "rt", "lf", "bk", "ft", "up", "dn" };
	static bool quakeFlipX[ 6 ] = { true,  true,  false, true,  true,  false };
	static bool quakeFlipY[ 6 ] = { false, false, true,  false, false, true };
	static int      quakeRot[ 6 ] = { 90,           -90,    0,              0,              90,             -90 };

	char            buffer[ 1024 ], filename[ 1024 ];
	const  char     *filename_p;

	if ( !imageName )
	{
		return nullptr;
	}

	Q_strncpyz( buffer, imageName, sizeof( buffer ) );
	hash = GenerateImageHashValue( buffer );

	// see if the image is already loaded
	for ( image = r_imageHashTable[ hash ]; image; image = image->next )
	{
		if ( !Q_stricmp( buffer, image->name ) )
		{
			return image;
		}
	}

	// try to load .CRN cubemap
	LoadCRN( buffer, pic, &width, &height, &numLayers, &numMips, &bits, 0 );
	if( numLayers == 6 && pic[0] ) {
		numPicsToFree = 1;
		goto createCubeImage;
	} else {
		R_FreeCubePics( pic, numLayers );
	}

	// try to load .KTX cubemap
	LoadKTX( buffer, pic, &width, &height, &numLayers, &numMips, &bits, 0 );
	if( numLayers == 6 && pic[0] ) {
		numPicsToFree = 1;
		goto createCubeImage;
	} else {
		R_FreeCubePics( pic, numLayers );
	}

	for ( i = 0; i < 6; i++ )
	{
		pic[ i ] = nullptr;
	}

	for ( i = 0; i < 6; i++ )
	{
		Com_sprintf( filename, sizeof( filename ), "%s_%s", buffer, openglSuffices[ i ] );

		filename_p = &filename[ 0 ];
		R_LoadImage( &filename_p, &pic[ i ], &width, &height, &numLayers, &numMips, &bits );

		if ( IsImageCompressed( bits ) )
		{
				Log::Warn("DXTn compression found in multi-file cube map; ignoring '%s'", imageName );
		        goto skipCubeImage;
		}

		if ( !pic[ i ] || width != height || numLayers > 0 )
		{
			image = nullptr;
			goto tryDoom3Suffices;
		}
		numPicsToFree = i;
	}

	goto createCubeImage;

tryDoom3Suffices:

	for ( i = 0; i < numPicsToFree; i++ ) {
		ri.Free( pic[i] );
		pic[i] = nullptr;
	}
	numPicsToFree = 0;

	for ( i = 0; i < 6; i++ )
	{
		Com_sprintf( filename, sizeof( filename ), "%s_%s", buffer, doom3Suffices[ i ] );

		filename_p = &filename[ 0 ];
		R_LoadImage( &filename_p, &pic[ i ], &width, &height, &numLayers, &numMips, &bits );

		if ( IsImageCompressed( bits ) )
		{
				Log::Warn("DXTn compression found in multi-file cube map; ignoring '%s'", imageName );
		        goto skipCubeImage;
		}

		if ( !pic[ i ] || width != height || numLayers > 0 )
		{
			image = nullptr;
			goto tryQuakeSuffices;
		}

		if ( doom3FlipX[ i ] )
		{
			R_Flip( pic[ i ], width, height );
		}

		if ( doom3FlipY[ i ] )
		{
			R_Flop( pic[ i ], width, height );
		}

		R_Rotate( pic[ i ], width, height, doom3Rot[ i ] );

		numPicsToFree = i;
	}

	goto createCubeImage;

tryQuakeSuffices:

	for ( i = 0; i < numPicsToFree; i++ ) {
		ri.Free( pic[i] );
		pic[i] = nullptr;
	}
	numPicsToFree = 0;

	for ( i = 0; i < 6; i++ )
	{
		Com_sprintf( filename, sizeof( filename ), "%s_%s", buffer, quakeSuffices[ i ] );

		filename_p = &filename[ 0 ];
		R_LoadImage( &filename_p, &pic[ i ], &width, &height, &numLayers, &numMips, &bits );

		if ( IsImageCompressed( bits ) )
		{
				Log::Warn("DXTn compression found in multi-file cube map; ignoring '%s'", imageName );
		        goto skipCubeImage;
		}

		if ( !pic[ i ] || width != height || numLayers > 0 )
		{
			image = nullptr;
			goto skipCubeImage;
		}

		if ( quakeFlipX[ i ] )
		{
			R_Flip( pic[ i ], width, height );
		}

		if ( quakeFlipY[ i ] )
		{
			R_Flop( pic[ i ], width, height );
		}

		R_Rotate( pic[ i ], width, height, quakeRot[ i ] );

		numPicsToFree = i;
	}

createCubeImage:
	image = R_CreateCubeImage( ( char * ) buffer, ( const byte ** ) pic, width, height, bits, filterType, wrapType );

skipCubeImage:
	R_FreeCubePics( pic, numPicsToFree );

	return image;
}

/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable()
{
	int   i;
	float d;
	float exp;

	exp = 0.5;

	for ( i = 0; i < FOG_TABLE_SIZE; i++ )
	{
		d = pow( ( float ) i / ( FOG_TABLE_SIZE - 1 ), exp );

		tr.fogTable[ i ] = d;
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float R_FogFactor( float s, float t )
{
	float d;

	s -= 1.0 / 512;

	if ( s < 0 )
	{
		return 0;
	}

	if ( t < 1.0 / 32 )
	{
		return 0;
	}

	if ( t < 31.0 / 32 )
	{
		s *= ( t - 1.0f / 32.0f ) / ( 30.0f / 32.0f );
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 )
	{
		s = 1.0;
	}

	d = tr.fogTable[( int )( s * ( FOG_TABLE_SIZE - 1 ) ) ];

	return d;
}

/*
================
R_CreateFogImage
================
*/
static const int FOG_S = 256;
static const int FOG_T = 32;
static void R_CreateFogImage()
{
	int   x, y;
	byte  *data, *ptr;
	float d;
	float borderColor[ 4 ];

	ptr = data = (byte*) ri.Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

	// S is distance, T is depth
	for ( y = 0; y < FOG_T; y++ )
	{
		for ( x = 0; x < FOG_S; x++ )
		{
			d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			ptr[ 0 ] = ptr[ 1 ] = ptr[ 2 ] = 255;
			ptr[ 3 ] = 255 * d;
			ptr += 4;
		}
	}

	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage( "_fog", ( const byte ** ) &data,
				     FOG_S, FOG_T, 1, IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_CLAMP );
	ri.Hunk_FreeTempMemory( data );

	borderColor[ 0 ] = 1.0;
	borderColor[ 1 ] = 1.0;
	borderColor[ 2 ] = 1.0;
	borderColor[ 3 ] = 1;

	glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

/*
==================
R_CreateDefaultImage
==================
*/
static const int DEFAULT_SIZE = 128;
static void R_CreateDefaultImage()
{
	int  x;
	byte data[ DEFAULT_SIZE ][ DEFAULT_SIZE ][ 4 ];
	byte *dataPtr = &data[0][0][0];

	// the default image will be a box, to allow you to see the mapping coordinates
	Com_Memset( data, 32, sizeof( data ) );

	for ( x = 0; x < DEFAULT_SIZE; x++ )
	{
		data[ 0 ][ x ][ 0 ] = data[ 0 ][ x ][ 1 ] = data[ 0 ][ x ][ 2 ] = data[ 0 ][ x ][ 3 ] = 255;
		data[ x ][ 0 ][ 0 ] = data[ x ][ 0 ][ 1 ] = data[ x ][ 0 ][ 2 ] = data[ x ][ 0 ][ 3 ] = 255;

		data[ DEFAULT_SIZE - 1 ][ x ][ 0 ] =
		  data[ DEFAULT_SIZE - 1 ][ x ][ 1 ] = data[ DEFAULT_SIZE - 1 ][ x ][ 2 ] = data[ DEFAULT_SIZE - 1 ][ x ][ 3 ] = 255;

		data[ x ][ DEFAULT_SIZE - 1 ][ 0 ] =
		  data[ x ][ DEFAULT_SIZE - 1 ][ 1 ] = data[ x ][ DEFAULT_SIZE - 1 ][ 2 ] = data[ x ][ DEFAULT_SIZE - 1 ][ 3 ] = 255;
	}

	tr.defaultImage = R_CreateImage( "_default", ( const byte ** ) &dataPtr,
					 DEFAULT_SIZE, DEFAULT_SIZE, 1, IF_NOPICMIP, filterType_t::FT_DEFAULT, wrapTypeEnum_t::WT_REPEAT );
}

static void R_CreateRandomNormalsImage()
{
	int  x, y;
	byte data[ DEFAULT_SIZE ][ DEFAULT_SIZE ][ 4 ];
	byte *ptr = &data[0][0][0];
	byte *dataPtr = &data[0][0][0];

	// the default image will be a box, to allow you to see the mapping coordinates
	Com_Memset( data, 32, sizeof( data ) );

	for ( y = 0; y < DEFAULT_SIZE; y++ )
	{
		for ( x = 0; x < DEFAULT_SIZE; x++ )
		{
			vec3_t n;
			float  r, angle;

			r = random();
			angle = 2.0 * M_PI * r; // / 360.0;

			VectorSet( n, cos( angle ), sin( angle ), r );
			VectorNormalize( n );

			ptr[ 0 ] = ( byte )( 128 + 127 * n[ 0 ] );
			ptr[ 1 ] = ( byte )( 128 + 127 * n[ 1 ] );
			ptr[ 2 ] = ( byte )( 128 + 127 * n[ 2 ] );
			ptr[ 3 ] = 255;
			ptr += 4;
		}
	}

	tr.randomNormalsImage = R_CreateImage( "_randomNormals", ( const byte ** ) &dataPtr,
					       DEFAULT_SIZE, DEFAULT_SIZE, 1, IF_NOPICMIP, filterType_t::FT_DEFAULT, wrapTypeEnum_t::WT_REPEAT );
}

static void R_CreateNoFalloffImage()
{
	byte data[ DEFAULT_SIZE ][ DEFAULT_SIZE ][ 4 ];
	byte *dataPtr = &data[0][0][0];

	// we use a solid white image instead of disabling texturing
	Com_Memset( data, 255, sizeof( data ) );
	tr.noFalloffImage = R_CreateImage( "_noFalloff", ( const byte ** ) &dataPtr,
					   8, 8, 1, IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_EDGE_CLAMP );
}

static const int ATTENUATION_XY_SIZE = 128;
static void R_CreateAttenuationXYImage()
{
	int  x, y;
	byte data[ ATTENUATION_XY_SIZE ][ ATTENUATION_XY_SIZE ][ 4 ];
	byte *ptr = &data[0][0][0];
	byte *dataPtr = &data[0][0][0];
	int  b;

	// make a centered inverse-square falloff blob for dynamic lighting
	for ( y = 0; y < ATTENUATION_XY_SIZE; y++ )
	{
		for ( x = 0; x < ATTENUATION_XY_SIZE; x++ )
		{
			float d;

			d = ( ATTENUATION_XY_SIZE / 2 - 0.5f - x ) * ( ATTENUATION_XY_SIZE / 2 - 0.5f - x ) +
			    ( ATTENUATION_XY_SIZE / 2 - 0.5f - y ) * ( ATTENUATION_XY_SIZE / 2 - 0.5f - y );
			b = 4000 / d;

			if ( b > 255 )
			{
				b = 255;
			}
			else if ( b < 75 )
			{
				b = 0;
			}

			ptr[ 0 ] = ptr[ 1 ] = ptr[ 2 ] = b;
			ptr[ 3 ] = 255;
			ptr += 4;
		}
	}

	tr.attenuationXYImage =
	  R_CreateImage( "_attenuationXY", ( const byte ** ) &dataPtr,
			 ATTENUATION_XY_SIZE, ATTENUATION_XY_SIZE, 1, IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_CLAMP);
}

static void R_CreateContrastRenderFBOImage()
{
	int  width, height;

	if ( glConfig2.textureNPOTAvailable )
	{
		width = glConfig.vidWidth * 0.25f;
		height = glConfig.vidHeight * 0.25f;
	}
	else
	{
		width = NearestPowerOfTwo( glConfig.vidWidth ) * 0.25f;
		height = NearestPowerOfTwo( glConfig.vidHeight ) * 0.25f;
	}

	tr.contrastRenderFBOImage = R_CreateImage( "_contrastRenderFBO", nullptr, width, height, 1, IF_NOPICMIP | IF_NOCOMPRESSION, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_CLAMP );
}

static void R_CreateBloomRenderFBOImage()
{
	int  i;
	int  width, height;

	if ( glConfig2.textureNPOTAvailable )
	{
		width = glConfig.vidWidth * 0.25f;
		height = glConfig.vidHeight * 0.25f;
	}
	else
	{
		width = NearestPowerOfTwo( glConfig.vidWidth ) * 0.25f;
		height = NearestPowerOfTwo( glConfig.vidHeight ) * 0.25f;
	}

	for ( i = 0; i < 2; i++ )
	{
		tr.bloomRenderFBOImage[ i ] = R_CreateImage( va( "_bloomRenderFBO%d", i ), nullptr, width, height, 1, IF_NOPICMIP | IF_NOCOMPRESSION, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_CLAMP );
	}
}

static void R_CreateCurrentRenderImage()
{
	int  width, height;

	if ( glConfig2.textureNPOTAvailable )
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}
	else
	{
		width = NearestPowerOfTwo( glConfig.vidWidth );
		height = NearestPowerOfTwo( glConfig.vidHeight );
	}

	tr.currentRenderImage[0] = R_CreateImage( "_currentRender[0]", nullptr, width, height, 1, IF_NOPICMIP | IF_NOCOMPRESSION, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );
	tr.currentRenderImage[1] = R_CreateImage( "_currentRender[1]", nullptr, width, height, 1, IF_NOPICMIP | IF_NOCOMPRESSION, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );
	tr.currentDepthImage = R_CreateImage( "_currentDepth", nullptr, width, height, 1, IF_NOPICMIP | IF_NOCOMPRESSION | IF_DEPTH24, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );
}

static void R_CreateDepthRenderImage()
{
	int  width, height, w, h;

	if ( glConfig2.textureNPOTAvailable )
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}
	else
	{
		width = NearestPowerOfTwo( glConfig.vidWidth );
		height = NearestPowerOfTwo( glConfig.vidHeight );
	}

	tr.depthRenderImage = R_CreateImage( "_depthRender", nullptr, width, height, 1, IF_NOPICMIP | IF_DEPTH24, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );

	w = (width + TILE_SIZE_STEP1 - 1) >> TILE_SHIFT_STEP1;
	h = (height + TILE_SIZE_STEP1 - 1) >> TILE_SHIFT_STEP1;
	tr.depthtile1RenderImage = R_CreateImage( "_depthtile1Render", nullptr, w, h, 1, IF_NOPICMIP | IF_RGBA32F, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );

	w = (width + TILE_SIZE - 1) >> TILE_SHIFT;
	h = (height + TILE_SIZE - 1) >> TILE_SHIFT;
	tr.depthtile2RenderImage = R_CreateImage( "_depthtile2Render", nullptr, w, h, 1, IF_NOPICMIP | IF_RGBA32F, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );

	if ( glConfig2.textureIntegerAvailable ) {
		tr.lighttileRenderImage = R_Create3DImage( "_lighttileRender", nullptr, w, h, 4, IF_NOPICMIP | IF_RGBA32UI, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );
	} else {
		tr.lighttileRenderImage = R_Create3DImage( "_lighttileRender", nullptr, w, h, 4, IF_NOPICMIP, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );
	}
}

static void R_CreatePortalRenderImage()
{
	int  width, height;

	if ( glConfig2.textureNPOTAvailable )
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}
	else
	{
		width = NearestPowerOfTwo( glConfig.vidWidth );
		height = NearestPowerOfTwo( glConfig.vidHeight );
	}

	tr.portalRenderImage = R_CreateImage( "_portalRender", nullptr, width, height, 1, IF_NOPICMIP | IF_NOCOMPRESSION, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );
}

static void R_CreateOcclusionRenderFBOImage()
{
	int  width, height;

	if ( glConfig2.textureNPOTAvailable )
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}
	else
	{
		width = NearestPowerOfTwo( glConfig.vidWidth );
		height = NearestPowerOfTwo( glConfig.vidHeight );
	}

	{
		tr.occlusionRenderFBOImage = R_CreateImage( "_occlusionFBORender", nullptr, width, height, 1, IF_NOPICMIP | IF_NOCOMPRESSION, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );
	}
}

static void R_CreateDepthToColorFBOImages()
{
	int  width, height;

	if ( glConfig2.textureNPOTAvailable )
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}
	else
	{
		width = NearestPowerOfTwo( glConfig.vidWidth );
		height = NearestPowerOfTwo( glConfig.vidHeight );
	}

	{
		tr.depthToColorBackFacesFBOImage = R_CreateImage( "_depthToColorBackFacesFBORender", nullptr, width, height, 1, IF_NOPICMIP | IF_NOCOMPRESSION, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );
		tr.depthToColorFrontFacesFBOImage = R_CreateImage( "_depthToColorFrontFacesFBORender", nullptr, width, height, 1, IF_NOPICMIP | IF_NOCOMPRESSION, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );
	}
}

// Tr3B: clean up this mess some day ...
static void R_CreateDownScaleFBOImages()
{
	int  width, height;

	if ( glConfig2.textureNPOTAvailable )
	{
		width = glConfig.vidWidth * 0.25f;
		height = glConfig.vidHeight * 0.25f;
	}
	else
	{
		width = NearestPowerOfTwo( glConfig.vidWidth * 0.25f );
		height = NearestPowerOfTwo( glConfig.vidHeight * 0.25f );
	}

	tr.downScaleFBOImage_quarter = R_CreateImage( "_downScaleFBOImage_quarter", nullptr, width, height, 1, IF_NOPICMIP | IF_NOCOMPRESSION, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );

	width = height = 64;

	tr.downScaleFBOImage_64x64 = R_CreateImage( "_downScaleFBOImage_64x64", nullptr, width, height, 1, IF_NOPICMIP | IF_NOCOMPRESSION, filterType_t::FT_NEAREST, wrapTypeEnum_t::WT_CLAMP );
}

// *INDENT-OFF*
static void R_CreateShadowMapFBOImage()
{
	int  i;
	int  width, height;
	int numShadowMaps = ( r_softShadowsPP->integer && r_shadows->integer >= Util::ordinal(shadowingMode_t::SHADOWING_VSM16)) ? MAX_SHADOWMAPS * 2 : MAX_SHADOWMAPS;
	int format;
	filterType_t filter;

	if ( !glConfig2.textureFloatAvailable || r_shadows->integer < Util::ordinal(shadowingMode_t::SHADOWING_ESM16))
	{
		return;
	}

	if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_ESM32))
	{
		format = IF_NOPICMIP | IF_ONECOMP32F;
	}
	else if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_VSM32))
	{
		format = IF_NOPICMIP | IF_TWOCOMP32F;
	}
	else if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_EVSM32))
	{
		if ( r_evsmPostProcess->integer )
		{
			format = IF_NOPICMIP | IF_ONECOMP32F;
		}
		else
		{
			format = IF_NOPICMIP | IF_RGBA32F;
		}
	}
	else if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_ESM16))
	{
		format = IF_NOPICMIP | IF_ONECOMP16F;
	}
	else if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_VSM16))
	{
		format = IF_NOPICMIP | IF_TWOCOMP16F;
	}
	else
	{
		format = IF_NOPICMIP | IF_RGBA16F;
	}
	if( r_shadowMapLinearFilter->integer )
	{
		filter = filterType_t::FT_LINEAR;
	}
	else
	{
		filter = filterType_t::FT_NEAREST;
	}

	for ( i = 0; i < numShadowMaps; i++ )
	{
		width = height = shadowMapResolutions[ i % MAX_SHADOWMAPS ];

		tr.shadowMapFBOImage[ i ] = R_CreateImage( va( "_shadowMapFBO%d", i ), nullptr, width, height, 1, format, filter, wrapTypeEnum_t::WT_ONE_CLAMP );
		tr.shadowClipMapFBOImage[ i ] = R_CreateImage( va( "_shadowClipMapFBO%d", i ), nullptr, width, height, 1, format, filter, wrapTypeEnum_t::WT_ONE_CLAMP );
	}

	// sun shadow maps
	for ( i = 0; i < numShadowMaps; i++ )
	{
		width = height = sunShadowMapResolutions[ i % MAX_SHADOWMAPS ];

		tr.sunShadowMapFBOImage[ i ] = R_CreateImage( va( "_sunShadowMapFBO%d", i ), nullptr, width, height, 1, format, filter, wrapTypeEnum_t::WT_ONE_CLAMP );
		tr.sunShadowClipMapFBOImage[ i ] = R_CreateImage( va( "_sunShadowClipMapFBO%d", i ), nullptr, width, height, 1, format, filter, wrapTypeEnum_t::WT_ONE_CLAMP );
	}
}

// *INDENT-ON*

// *INDENT-OFF*
static void R_CreateShadowCubeFBOImage()
{
	int  j;
	int  width, height;
	int format;
	filterType_t filter;

	if ( !glConfig2.textureFloatAvailable || r_shadows->integer < Util::ordinal(shadowingMode_t::SHADOWING_ESM16))
	{
		return;
	}

	if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_ESM32))
	{
		format = IF_NOPICMIP | IF_ONECOMP32F;
	}
	else if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_VSM32))
	{
		format = IF_NOPICMIP | IF_TWOCOMP32F;
	}
	else if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_EVSM32))
	{
		if ( r_evsmPostProcess->integer )
		{
			format = IF_NOPICMIP | IF_ONECOMP32F;
		}
		else
		{
			format = IF_NOPICMIP | IF_RGBA32F;
		}
	}
	else if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_ESM16))
	{
		format = IF_NOPICMIP | IF_ONECOMP16F;
	}
	else if ( r_shadows->integer == Util::ordinal(shadowingMode_t::SHADOWING_VSM16))
	{
		format = IF_NOPICMIP | IF_TWOCOMP16F;
	}
	else
	{
		format = IF_NOPICMIP | IF_RGBA16F;
	}
	if( r_shadowMapLinearFilter->integer )
	{
		filter = filterType_t::FT_LINEAR;
	}
	else
	{
		filter = filterType_t::FT_NEAREST;
	}

	for ( j = 0; j < 5; j++ )
	{
		width = height = shadowMapResolutions[ j ];

		tr.shadowCubeFBOImage[ j ] = R_CreateCubeImage( va( "_shadowCubeFBO%d", j ), nullptr, width, height, format, filter, wrapTypeEnum_t::WT_EDGE_CLAMP );
		tr.shadowClipCubeFBOImage[ j ] = R_CreateCubeImage( va( "_shadowClipCubeFBO%d", j ), nullptr, width, height, format, filter, wrapTypeEnum_t::WT_EDGE_CLAMP );
	}
}

// *INDENT-ON*

// *INDENT-OFF*
static void R_CreateBlackCubeImage()
{
	int  i;
	int  width, height;
	byte *data[ 6 ];

	width = REF_CUBEMAP_SIZE;
	height = REF_CUBEMAP_SIZE;

	for ( i = 0; i < 6; i++ )
	{
		data[ i ] = (byte*) ri.Hunk_AllocateTempMemory( width * height * 4 );
		Com_Memset( data[ i ], 0, width * height * 4 );
	}

	tr.blackCubeImage = R_CreateCubeImage( "_blackCube", ( const byte ** ) data, width, height, IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_EDGE_CLAMP );
	tr.autoCubeImage = R_CreateCubeImage( "_autoCube", ( const byte ** ) data, width, height, IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_EDGE_CLAMP );

	for ( i = 5; i >= 0; i-- )
	{
		ri.Hunk_FreeTempMemory( data[ i ] );
	}
}

// *INDENT-ON*

// *INDENT-OFF*
static void R_CreateWhiteCubeImage()
{
	int  i;
	int  width, height;
	byte *data[ 6 ];

	width = REF_CUBEMAP_SIZE;
	height = REF_CUBEMAP_SIZE;

	for ( i = 0; i < 6; i++ )
	{
		data[ i ] = (byte*) ri.Hunk_AllocateTempMemory( width * height * 4 );
		Com_Memset( data[ i ], 0xFF, width * height * 4 );
	}

	tr.whiteCubeImage = R_CreateCubeImage( "_whiteCube", ( const byte ** ) data, width, height, IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_EDGE_CLAMP );

	for ( i = 5; i >= 0; i-- )
	{
		ri.Hunk_FreeTempMemory( data[ i ] );
	}
}

// *INDENT-ON*

static void R_CreateColorGradeImage()
{
	byte *data, *ptr;
	int i, r, g, b;

	data = (byte*) ri.Hunk_AllocateTempMemory( REF_COLORGRADE_SLOTS * REF_COLORGRADEMAP_STORE_SIZE * sizeof(u8vec4_t) );

	// 255 is 15 * 17, so the colors range from 0 to 255
	for( ptr = data, i = 0; i < REF_COLORGRADE_SLOTS; i++ ) {
		glState.colorgradeSlots[ i ] = nullptr;

		for( b = 0; b < REF_COLORGRADEMAP_SIZE; b++ ) {
			for( g = 0; g < REF_COLORGRADEMAP_SIZE; g++ ) {
				for( r = 0; r < REF_COLORGRADEMAP_SIZE; r++ ) {
					*ptr++ = (byte) r * 17;
					*ptr++ = (byte) g * 17;
					*ptr++ = (byte) b * 17;
					*ptr++ = 255;
				}
			}
		}
	}

	tr.colorGradeImage = R_Create3DImage( "_colorGrade", data,
					      REF_COLORGRADEMAP_SIZE,
					      REF_COLORGRADEMAP_SIZE,
					      REF_COLORGRADE_SLOTS * REF_COLORGRADEMAP_SIZE,
					      IF_NOPICMIP | IF_NOCOMPRESSION | IF_NOLIGHTSCALE,
					      filterType_t::FT_LINEAR,
						  wrapTypeEnum_t::WT_EDGE_CLAMP );

	ri.Hunk_FreeTempMemory( data );
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages()
{
	int   x, y;
	byte  data[ DEFAULT_SIZE ][ DEFAULT_SIZE ][ 4 ];
	byte  *dataPtr = &data[0][0][0];
	byte  *out;
	float s, value;
	byte  intensity;

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	Com_Memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage( "_white", ( const byte ** ) &dataPtr,
				       8, 8, 1, IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_REPEAT );

	// we use a solid black image instead of disabling texturing
	Com_Memset( data, 0, sizeof( data ) );
	tr.blackImage = R_CreateImage( "_black", ( const byte ** ) &dataPtr,
				       8, 8, 1, IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_REPEAT );

	// red
	for ( x = DEFAULT_SIZE * DEFAULT_SIZE, out = &data[0][0][0]; x; --x, out += 4 )
	{
		out[ 1 ] = out[ 2 ] = 0;
		out[ 0 ] = out[ 3 ] = 255;
	}

	tr.redImage = R_CreateImage( "_red", ( const byte ** ) &dataPtr,
				     8, 8, 1, IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_REPEAT );

	// green
	for ( x = DEFAULT_SIZE * DEFAULT_SIZE, out = &data[0][0][0]; x; --x, out += 4 )
	{
		out[ 0 ] = out[ 2 ] = 0;
		out[ 1 ] = out[ 3 ] = 255;
	}

	tr.greenImage = R_CreateImage( "_green", ( const byte ** ) &dataPtr,
				       8, 8, 1, IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_REPEAT );

	// blue
	for ( x = DEFAULT_SIZE * DEFAULT_SIZE, out = &data[0][0][0]; x; --x, out += 4 )
	{
		out[ 0 ] = out[ 1 ] = 0;
		out[ 2 ] = out[ 3 ] = 255;
	}

	tr.blueImage = R_CreateImage( "_blue", ( const byte ** ) &dataPtr,
				      8, 8, 1, IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_REPEAT );

	// generate a default normalmap with a zero heightmap
	for ( x = DEFAULT_SIZE * DEFAULT_SIZE, out = &data[0][0][0]; x; --x, out += 4 )
	{
		out[ 0 ] = out[ 1 ] = 128;
		out[ 2 ] = 255;
		out[ 3 ] = 0;
	}

	tr.flatImage = R_CreateImage( "_flat", ( const byte ** ) &dataPtr,
				      8, 8, 1, IF_NOPICMIP | IF_NORMALMAP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_REPEAT );

	for ( x = 0; x < 32; x++ )
	{
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[ x ] = R_CreateImage( "_scratch", ( const byte ** ) &dataPtr,
						      DEFAULT_SIZE, DEFAULT_SIZE, 1, IF_NONE, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_CLAMP );
	}

	out = &data[ 0 ][ 0 ][ 0 ];

	for ( y = 0; y < DEFAULT_SIZE; y++ )
	{
		for ( x = 0; x < DEFAULT_SIZE; x++, out += 4 )
		{
			s = ( ( ( float ) x + 0.5f ) * ( 2.0f / DEFAULT_SIZE ) - 1.0f );

			s = Q_fabs( s ) - ( 1.0f / DEFAULT_SIZE );

			value = 1.0f - ( s * 2.0f ) + ( s * s );

			intensity = ClampByte( Q_ftol( value * 255.0f ) );

			out[ 0 ] = intensity;
			out[ 1 ] = intensity;
			out[ 2 ] = intensity;
			out[ 3 ] = intensity;
		}
	}

	tr.quadraticImage =
		R_CreateImage( "_quadratic", ( const byte ** ) &dataPtr,
			       DEFAULT_SIZE, DEFAULT_SIZE, 1, IF_NOPICMIP | IF_NOCOMPRESSION, filterType_t::FT_LINEAR,
					   wrapTypeEnum_t::WT_CLAMP );

	R_CreateRandomNormalsImage();
	R_CreateFogImage();
	R_CreateNoFalloffImage();
	R_CreateAttenuationXYImage();
	R_CreateContrastRenderFBOImage();
	R_CreateBloomRenderFBOImage();
	R_CreateCurrentRenderImage();
	R_CreateDepthRenderImage();
	R_CreatePortalRenderImage();
	R_CreateOcclusionRenderFBOImage();
	R_CreateDepthToColorFBOImages();
	R_CreateDownScaleFBOImages();
	R_CreateShadowMapFBOImage();
	R_CreateShadowCubeFBOImage();
	R_CreateBlackCubeImage();
	R_CreateWhiteCubeImage();
	R_CreateColorGradeImage();
}

/*
===============
R_InitImages
===============
*/
void R_InitImages()
{
	const char *charsetImage = "gfx/2d/consolechars";

	Log::Debug("------- R_InitImages -------" );

	Com_Memset( r_imageHashTable, 0, sizeof( r_imageHashTable ) );
	Com_InitGrowList( &tr.images, 4096 );
	Com_InitGrowList( &tr.lightmaps, 128 );
	Com_InitGrowList( &tr.deluxemaps, 128 );

	// These are the values expected by the rest of the renderer (esp. tr_bsp), used for "gamma correction of the map"
	// (both were set to 0 if we had neither COMPAT_ET nor COMPAT_Q3, it may be interesting to remember)
	tr.overbrightBits = 0;
	tr.mapOverBrightBits = 2;
	tr.identityLight = 1.0f / ( 1 << tr.overbrightBits );

	// create default texture and white texture
	R_CreateBuiltinImages();

	tr.charsetImage = R_FindImageFile( charsetImage, IF_NOCOMPRESSION | IF_NOPICMIP, filterType_t::FT_DEFAULT, wrapTypeEnum_t::WT_CLAMP );

	if ( !tr.charsetImage )
	{
		ri.Error( errorParm_t::ERR_FATAL, "R_InitImages: could not load '%s'", charsetImage );
	}
}

/*
===============
R_ShutdownImages
===============
*/
void R_ShutdownImages()
{
	int     i;
	image_t *image;

	Log::Debug("------- R_ShutdownImages -------" );

	for ( i = 0; i < tr.images.currentElements; i++ )
	{
		image = (image_t*) Com_GrowListElement( &tr.images, i );

		glDeleteTextures( 1, &image->texnum );
	}

	Com_Memset( glState.currenttextures, 0, sizeof( glState.currenttextures ) );

	Com_DestroyGrowList( &tr.images );
	Com_DestroyGrowList( &tr.lightmaps );
	Com_DestroyGrowList( &tr.deluxemaps );
	Com_DestroyGrowList( &tr.cubeProbes );

	FreeVertexHashTable( tr.cubeHashTable );
}

void RE_GetTextureSize( int textureID, int *width, int *height )
{
	image_t *baseImage;
	shader_t *shader;

	shader = R_GetShaderByHandle( textureID );

	ASSERT(shader != nullptr);
	if ( !shader )
	{
		return;
	}

	baseImage = shader->stages[ 0 ]->bundle->image[ 0 ];
	if ( !baseImage )
	{
		Log::Debug( "%sRE_GetTextureSize: shader %s is missing base image",
			Color::CString( Color::Yellow ),
			shader->name );
		return;
	}

	if ( width )
	{
		*width = baseImage->width;
	}
	if ( height )
	{
		*height = baseImage->height;
	}
}

int numTextures = 0;

qhandle_t RE_GenerateTexture( const byte *pic, int width, int height )
{
	const char *name = va( "rocket%d", numTextures++ );
	R_SyncRenderThread();
	return RE_RegisterShaderFromImage( name, R_CreateImage( name, &pic, width, height, 1, IF_NOCOMPRESSION | IF_NOPICMIP, filterType_t::FT_LINEAR, wrapTypeEnum_t::WT_CLAMP ) );
}
