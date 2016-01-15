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

// tr_font.c
//
//
// The font system uses FreeType 2.x to render TrueType fonts for use within the game.
// As of this writing ( Nov, 2000 ) Team Arena uses these fonts for all of the ui and
// about 90% of the cgame presentation. A few areas of the CGAME were left uses the old
// fonts since the code is shared with standard Q3A.
//
// If you include this font rendering code in a commercial product you MUST include the
// following somewhere with your product, see www.freetype.org for specifics or changes.
// The Freetype code also uses some hinting techniques that MIGHT infringe on patents
// held by apple so be aware of that also.
//
// As of Q3A 1.25+ and Team Arena, we are shipping the game with the font rendering code
// disabled. This removes any potential patent issues and it keeps us from having to
// distribute an actual TrueTrype font which is 1. expensive to do and 2. seems to require
// an act of god to accomplish.
//
// What we did was pre-render the fonts using FreeType ( which is why we leave the FreeType
// credit in the credits ) and then saved off the glyph data and then hand touched up the
// font bitmaps so they scale a bit better in GL.
//
// There are limitations in the way fonts are saved and reloaded in that it is based on
// point size and not name. So if you pre-render Helvetica in 18 point and Impact in 18 point
// you will end up with a single 18 point data file and image set. Typically you will want to
// choose 3 sizes to best approximate the scaling you will be doing in the ui scripting system
//
// In the UI Scripting code, a scale of 1.0 is equal to a 48 point font. In Team Arena, we
// use three or four scales, most of them exactly equaling the specific rendered size. We
// rendered three sizes in Team Arena, 12, 16, and 20.
//
// To generate new font data you need to go through the following steps.
// 1. delete the fontImage_x_xx.png files and fontImage_xx.dat files from the fonts path.
// 2. in a ui script, specificy a font, smallFont, and bigFont keyword with font name and
//    point size. the original TrueType fonts must exist in fonts at this point.
// 3. run the game. you should see things.
// 4. Exit the game and there will be three dat files and at least three PNG files. The
//    PNGs are in 256x256 pages so if it takes three images to render a 24 point font you
//    will end up with fontImage_0_24.tga through fontImage_2_24.tga
// 5. In future runs of the game, the system looks for these images and data files when a
//    specific point sized font is rendered and loads them for use.
// 6. Because of the original beta nature of the FreeType code you will probably want to hand
//    touch the font bitmaps.


#include "tr_local.h"

#include "qcommon/qcommon.h"
#include "qcommon/q_unicode.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ERRORS_H
#include FT_SYSTEM_H
#include FT_IMAGE_H
#include FT_OUTLINE_H

#define _FLOOR( x ) ( ( x ) & - 64 )
#define _CEIL( x )  ( ( ( x ) + 63 ) & - 64 )
#define _TRUNC( x ) ( ( x ) >> 6 )

FT_Library ftLibrary = nullptr;

#define FONT_SIZE 512

#define MAX_FONTS 16
#define MAX_FILES ( MAX_FONTS )
static fontInfo_t registeredFont[ MAX_FONTS ];
static unsigned int fontUsage[ MAX_FONTS ];
static unsigned int fontUsageVM[ MAX_FONTS ];

static struct {
	void *data;
	int   length;
	int   count;
	char  name[ MAX_QPATH ];
} fontData[ MAX_FILES ];

void RE_RenderChunk( fontInfo_t *font, const int chunk );


void R_GetGlyphInfo( FT_GlyphSlot glyph, int *left, int *right, int *width, int *top, int *bottom, int *height, int *pitch )
{
	// ±1 adjustments for border-related reasons - really want clamp to transparent border (FIXME)

	*left = _FLOOR( glyph->metrics.horiBearingX - 1);
	*right = _CEIL( glyph->metrics.horiBearingX + glyph->metrics.width + 1);
	*width = _TRUNC( *right - *left );

	*top = _CEIL( glyph->metrics.horiBearingY + 1);
	*bottom = _FLOOR( glyph->metrics.horiBearingY - glyph->metrics.height - 1);
	*height = _TRUNC( *top - *bottom );
	*pitch = ( *width + 3 ) & - 4;
}

FT_Bitmap      *R_RenderGlyph( FT_GlyphSlot glyph, glyphInfo_t *glyphOut )
{
	FT_Bitmap *bit2;
	int       left, right, width, top, bottom, height, pitch, size;

	R_GetGlyphInfo( glyph, &left, &right, &width, &top, &bottom, &height, &pitch );

	if ( glyph->format == ft_glyph_format_outline )
	{
		size = pitch * height;

		bit2 = (FT_Bitmap*) ri.Z_Malloc( sizeof( FT_Bitmap ) );

		bit2->width = width;
		bit2->rows = height;
		bit2->pitch = pitch;
		bit2->pixel_mode = ft_pixel_mode_grays;
		bit2->buffer = (unsigned char*) ri.Z_Malloc( pitch * height );
		bit2->num_grays = 256;

		Com_Memset( bit2->buffer, 0, size );

		FT_Outline_Translate( &glyph->outline, -left, -bottom );

		FT_Outline_Get_Bitmap( ftLibrary, &glyph->outline, bit2 );

		glyphOut->height = height;
		glyphOut->pitch = pitch;
		glyphOut->top = ( glyph->metrics.horiBearingY >> 6 ) + 1;
		glyphOut->bottom = bottom;

		return bit2;
	}
	else
	{
		ri.Printf( PRINT_WARNING, "Non-outline fonts are not supported\n" );
	}

	return nullptr;
}

static glyphInfo_t *RE_ConstructGlyphInfo( unsigned char *imageOut, int *xOut, int *yOut,
    int *maxHeight, FT_Face face, FT_Face fallback, const int c, bool calcHeight )
{
	int                i;
	static glyphInfo_t glyph;
	unsigned char      *src, *dst;
	float              scaledWidth, scaledHeight;
	FT_Bitmap          *bitmap = nullptr;

	Com_Memset( &glyph, 0, sizeof( glyphInfo_t ) );

	// make sure everything is here
	if ( face != nullptr )
	{
		FT_UInt index = FT_Get_Char_Index( face, c );

		if ( index == 0 && fallback )
		{
			index = FT_Get_Char_Index( face = fallback, c );
		}

		if ( index == 0 )
		{
			return nullptr; // nothing to render
		}

		FT_Load_Glyph( face, index, FT_LOAD_DEFAULT );
		bitmap = R_RenderGlyph( face->glyph, &glyph );

		if ( bitmap )
		{
			glyph.xSkip = ( face->glyph->metrics.horiAdvance >> 6 ) + 1;
		}
		else
		{
			return nullptr;
		}

		if ( glyph.height > *maxHeight )
		{
			*maxHeight = glyph.height;
		}

		if ( calcHeight )
		{
			ri.Free( bitmap->buffer );
			ri.Free( bitmap );
			return &glyph;
		}

		scaledWidth = glyph.pitch;
		scaledHeight = glyph.height;

		// we need to make sure we fit
		if ( *xOut + scaledWidth + 1 >= ( FONT_SIZE - 1 ) )
		{
			*xOut = 0;
			*yOut += *maxHeight + 1;
		}

		if ( *yOut + *maxHeight + 1 >= ( FONT_SIZE - 1 ) )
		{
			*xOut = -1;
			ri.Free( bitmap->buffer );
			ri.Free( bitmap );
			return nullptr;
		}

		src = bitmap->buffer;
		dst = imageOut + ( *yOut * FONT_SIZE ) + *xOut;

		if ( bitmap->pixel_mode == ft_pixel_mode_mono )
		{
			for ( i = 0; i < glyph.height; i++ )
			{
				int           j;
				unsigned char *_src = src;
				unsigned char *_dst = dst;
				unsigned char mask = 0x80;
				unsigned char val = *_src;

				for ( j = 0; j < glyph.pitch; j++ )
				{
					if ( mask == 0x80 )
					{
						val = *_src++;
					}

					if ( val & mask )
					{
						*_dst = 0xff;
					}

					mask >>= 1;

					if ( mask == 0 )
					{
						mask = 0x80;
					}

					_dst++;
				}

				src += glyph.pitch;
				dst += FONT_SIZE;
			}
		}
		else
		{
			for ( i = 0; i < glyph.height; i++ )
			{
				Com_Memcpy( dst, src, glyph.pitch );
				src += glyph.pitch;
				dst += FONT_SIZE;
			}
		}

		// we now have an 8 bit per pixel grey scale bitmap
		// that is width wide and pf->ftSize->metrics.y_ppem tall

		glyph.imageHeight = scaledHeight;
		glyph.imageWidth = scaledWidth;
		glyph.s = ( float ) * xOut / FONT_SIZE;
		glyph.t = ( float ) * yOut / FONT_SIZE;
		glyph.s2 = glyph.s + ( float ) scaledWidth / FONT_SIZE;
		glyph.t2 = glyph.t + ( float ) scaledHeight / FONT_SIZE;
		glyph.shaderName[0] = 1; // flag that we have a glyph here

		*xOut += scaledWidth + 1;

		ri.Free( bitmap->buffer );
		ri.Free( bitmap );

		return &glyph;
	}

	return nullptr;
}


static int  fdOffset;
static byte *fdFile;

int readInt()
{
	int i =
	  fdFile[ fdOffset ] + ( fdFile[ fdOffset + 1 ] << 8 ) + ( fdFile[ fdOffset + 2 ] << 16 ) + ( fdFile[ fdOffset + 3 ] << 24 );
	fdOffset += 4;
	return i;
}

union poor
{
	byte  fred[ 4 ];
	float ffred;
};

float readFloat()
{
	poor me;

#ifdef Q3_BIG_ENDIAN
	me.fred[ 0 ] = fdFile[ fdOffset + 3 ];
	me.fred[ 1 ] = fdFile[ fdOffset + 2 ];
	me.fred[ 2 ] = fdFile[ fdOffset + 1 ];
	me.fred[ 3 ] = fdFile[ fdOffset + 0 ];
#else
	me.fred[ 0 ] = fdFile[ fdOffset + 0 ];
	me.fred[ 1 ] = fdFile[ fdOffset + 1 ];
	me.fred[ 2 ] = fdFile[ fdOffset + 2 ];
	me.fred[ 3 ] = fdFile[ fdOffset + 3 ];
#endif
	fdOffset += 4;
	return me.ffred;
}

void RE_GlyphChar( fontInfo_t *font, int ch, glyphInfo_t *glyph )
{
	// default if out of range
	if ( ch < 0 || ( ch >= 0xD800 && ch < 0xE000 ) || ch >= 0x110000 || ch == 0xFFFD )
	{
		ch = 0;
	}

	// render if needed
	if ( !font->glyphBlock[ ch / 256 ] )
	{
		RE_RenderChunk( font, ch / 256 );
	}

	// default if no glyph
	if ( !font->glyphBlock[ ch / 256 ][ ch % 256 ].glyph )
	{
		ch = 0;
	}

	// we have a glyph
	memcpy( glyph, &font->glyphBlock[ ch / 256][ ch % 256 ], sizeof( *glyph ) );
}

void RE_GlyphCharVM( fontHandle_t handle, int ch, glyphInfo_t *glyph )
{
	if ( handle < 0 || handle >= MAX_FONTS || !fontUsageVM[ handle ] )
	{
		memset( glyph, 0, sizeof( *glyph ) );
		return;
	}

	RE_GlyphChar( &registeredFont[ handle ], ch, glyph );
}

void RE_Glyph( fontInfo_t *font, const char *str, glyphInfo_t *glyph )
{
	RE_GlyphChar( font, Q_UTF8_CodePoint( str ), glyph );
}

void RE_GlyphVM( fontHandle_t handle, const char *str, glyphInfo_t *glyph )
{
	RE_GlyphCharVM( handle, str ? Q_UTF8_CodePoint( str ) : 0, glyph );
}

static void RE_StoreImage( fontInfo_t *font, int chunk, int page, int from, int to, const unsigned char *bitmap, int yEnd )
{
	int           scaledSize = FONT_SIZE * FONT_SIZE;
	int           i, j, y;
	float         max;

	glyphInfo_t   *glyphs = font->glyphBlock[chunk];

	unsigned char *buffer;
	image_t       *image;
	qhandle_t     h;

	char          fileName[ MAX_QPATH ];

	// about to render an image
	R_SyncRenderThread();

	// maybe crop image while retaining power-of-2 height
	i = 1;
	y = FONT_SIZE;

	// How much to reduce it?
	while ( yEnd < y / 2 - 1 ) { i += i; y /= 2; }

	// Fix up the glyphs' Y co-ordinates
	for ( j = from; j < to; j++ ) { glyphs[j].t *= i; glyphs[j].t2 *= i; }

	scaledSize /= i;

	max = 0;

	for ( i = 0; i < scaledSize; i++ )
	{
		if ( max < bitmap[ i ] )
		{
			max = bitmap[ i ];
		}
	}

	if ( max > 0 )
	{
		max = 255 / max;
	}

	buffer = ( unsigned char * ) ri.Z_Malloc( scaledSize * 4 );
	for ( i = j = 0; i < scaledSize; i++ )
	{
		buffer[ j++ ] = 255;
		buffer[ j++ ] = 255;
		buffer[ j++ ] = 255;
		buffer[ j++ ] = ( ( float ) bitmap[ i ] * max );
	}


	Com_sprintf( fileName, sizeof( fileName ), "%s_%i_%i_%i.png", font->name, chunk, page, font->pointSize );

	image = R_CreateGlyph( fileName, buffer, FONT_SIZE, y );

	ri.Free( buffer );

	h = RE_RegisterShaderFromImage( fileName, image );

	for ( j = from; j < to; j++ )
	{
		if ( font->glyphBlock[ chunk ][ j ].shaderName[0] ) // non-0 if we have a glyph here
		{
			font->glyphBlock[ chunk ][ j ].glyph = h;
			Q_strncpyz( font->glyphBlock[ chunk ][ j ].shaderName, fileName, sizeof( font->glyphBlock[ chunk ][ j ].shaderName ) );
		}
	}
}

static glyphBlock_t nullGlyphs;

void RE_RenderChunk( fontInfo_t *font, const int chunk )
{
	int           xOut, yOut, maxHeight;
	int           i, lastStart, page;
	unsigned char *out;
	glyphInfo_t   *glyphs;
	bool      rendered;

	const int     startGlyph = chunk * 256;

	// sanity check
	if ( chunk < 0 || chunk >= 0x1100 || font->glyphBlock[ chunk ] )
	{
		return;
	}

	out = (unsigned char*) ri.Z_Malloc( FONT_SIZE * FONT_SIZE );

	if ( out == nullptr )
	{
		ri.Printf( PRINT_WARNING, "RE_RenderChunk: ri.Malloc failure during output image creation.\n" );
		return;
	}

	Com_Memset( out, 0, FONT_SIZE * FONT_SIZE );

	// calculate max height
	maxHeight = 0;
	rendered = false;

	for ( i = 0; i < 256; i++ )
	{
		rendered |= !!RE_ConstructGlyphInfo( out, &xOut, &yOut, &maxHeight, (FT_Face) font->face, (FT_Face) font->fallback, ( i + startGlyph ) ? ( i + startGlyph ) : 0xFFFD, true );
	}

	// no glyphs? just return
	if ( !rendered )
	{
		ri.Free( out );
		font->glyphBlock[ chunk ] = nullGlyphs;
		return;
	}

	glyphs = font->glyphBlock[ chunk ] = (glyphInfo_t*) ri.Z_Malloc( sizeof( glyphBlock_t ) );
	memset( glyphs, 0, sizeof( glyphBlock_t ) );

	xOut = yOut = 0;
	rendered = false;
	i = lastStart = page = 0;

	while ( i < 256 )
	{
		glyphInfo_t *glyph = RE_ConstructGlyphInfo( out, &xOut, &yOut, &maxHeight, (FT_Face) font->face, (FT_Face) font->fallback, ( i + startGlyph ) ? ( i + startGlyph ) : 0xFFFD, false );

		if ( glyph )
		{
			rendered = true;
			Com_Memcpy( glyphs + i, glyph, sizeof( glyphInfo_t ) );
		}

		if ( xOut == -1 )
		{
			RE_StoreImage( font, chunk, page++, lastStart, i, out, yOut + maxHeight + 1 );
			Com_Memset( out, 0, FONT_SIZE * FONT_SIZE );
			xOut = yOut = 0;
			rendered = false;
			lastStart = i;
		}
		else
		{
			i++;
		}
	}

	if ( rendered )
	{
		RE_StoreImage( font, chunk, page, lastStart, 256, out, yOut + maxHeight + 1 );
	}
	ri.Free( out );
}

static int RE_LoadFontFile( const char *name, void **buffer )
{
	int i;

	// if we already have this file, return it
	for ( i = 0; i < MAX_FILES; ++i )
	{
		if ( !fontData[ i ].count || Q_stricmp( name, fontData[ i ].name ) )
		{
			continue;
		}

		++fontData[ i ].count;

		*buffer = fontData[ i ].data;
		return fontData[ i ].length;
	}

	// otherwise, find a free entry and load the file
	for ( i = 0; i < MAX_FILES; ++i )
	{
		if ( !fontData [ i ].count )
		{
			void *tmp;
			int  length = ri.FS_ReadFile( name, &tmp );

			if ( length <= 0 )
			{
				return 0;
			}

			fontData[ i ].data = malloc( length );

			if ( !fontData[ i ].data )
			{
				ri.FS_FreeFile( tmp );
				return 0;
			}

			fontData[ i ].length = length;
			fontData[ i ].count = 1;
			*buffer = fontData[ i ].data;

			memcpy( fontData[ i ].data, tmp, length );
			ri.FS_FreeFile( tmp );

			Q_strncpyz( fontData[ i ].name, name, sizeof( fontData[ i ].name ) );

			return length;
		}
	}

	return 0;
}

static void RE_FreeFontFile( void *data )
{
	int i;

	if ( !data )
	{
		return;
	}

	for ( i = 0; i < MAX_FILES; ++i )
	{
		if ( fontData[ i ].data == data )
		{
			if ( !--fontData[ i ].count )
			{
				free( fontData[ i ].data );
			}
			break;
		}
	}
}

static fontHandle_t RE_RegisterFont_Internal( const char *fontName, const char *fallbackName, int pointSize, fontInfo_t **font )
{
	FT_Face       face, fallback;
	void          *faceData, *fallbackData;
	int           i, len, fontNo;
	char          fileName[ MAX_QPATH ];
	char          strippedName[ MAX_QPATH ];
	char          registeredName[ MAX_QPATH ];

	if ( pointSize <= 0 )
	{
		pointSize = 12;
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	COM_StripExtension2( fontName, strippedName, sizeof( strippedName ) );

	if ( !Q_stricmp( strippedName, fontName ) )
	{
		fallbackName = nullptr;
		Com_sprintf( fileName, sizeof( fileName ), "fonts/fontImage_%i.dat", pointSize );
	}
	else
	{
		Com_sprintf( fileName, sizeof( fileName ), "fonts/%s_%i.dat", strippedName, pointSize );
	}

	if ( fallbackName )
	{
		COM_StripExtension2( fallbackName, fileName, sizeof( fileName ) );
		Q_snprintf( registeredName, sizeof( registeredName ), "%s#%s#%d", strippedName, fileName, pointSize );
	}
	else
	{
		Q_snprintf( registeredName, sizeof( registeredName ), "%s#%d", strippedName, pointSize );
	}

	fontNo = -1;

	for ( i = 0; i < MAX_FONTS; i++ )
	{
		if ( !fontUsage[ i ] )
		{
			if ( fontNo < 0 )
			{
				fontNo = i;
			}
		}
		else if ( pointSize == registeredFont[ i ].pointSize && Q_stricmp( strippedName, registeredFont[ i ].name ) == 0 )
		{
			*font =  &registeredFont[ i ];
			++fontUsage[ i ];
			return i;
		}
	}

	*font = &registeredFont[ fontNo ];
	memset( *font, 0, sizeof( fontInfo_t ) );

	if ( fontNo < 0 )
	{
		ri.Printf( PRINT_WARNING, "RE_RegisterFont: Too many fonts registered already.\n" );
		return -1;
	}

	len = ri.FS_ReadFile( fileName, nullptr );

	if ( len > 0x5004 && len <= 0x5004 + MAX_QPATH ) // 256 glyphs, scale info, and the bitmap name
	{
		glyphInfo_t *glyphs;
		int height = 0;

		ri.FS_ReadFile( fileName, &faceData );
		fdOffset = 0;
		fdFile = (byte*) faceData;

		glyphs = (*font)->glyphBlock[0] = (glyphInfo_t*) ri.Z_Malloc( sizeof( glyphBlock_t ) );
		memset( glyphs, 0, sizeof( glyphBlock_t ) );

		for ( i = 0; i < GLYPHS_PER_FONT; i++ )
		{
			glyphs[ i ].height = readInt();

			if ( glyphs[ i ].height > height )
			{
				height = glyphs[ i ].height;
			}

			glyphs[ i ].top = readInt();
			glyphs[ i ].bottom = readInt();
			glyphs[ i ].pitch = readInt();
			glyphs[ i ].xSkip = readInt();
			glyphs[ i ].imageWidth = readInt();
			glyphs[ i ].imageHeight = readInt();
			glyphs[ i ].s = readFloat();
			glyphs[ i ].t = readFloat();
			glyphs[ i ].s2 = readFloat();
			glyphs[ i ].t2 = readFloat();
			glyphs[ i ].glyph = readInt();
			Q_strncpyz( glyphs[ i ].shaderName, (const char *) &fdFile[ fdOffset ], sizeof( glyphs[ i ].shaderName ) );
			fdOffset += sizeof( glyphs[ i ].shaderName );
		}

		(*font)->pointSize = pointSize;
		(*font)->height = height;
		(*font)->glyphScale = readFloat();
		Q_strncpyz( (*font)->name, registeredName, sizeof( (*font)->name ) );

		ri.FS_FreeFile( faceData );

		for ( i = GLYPH_START; i <= GLYPH_END; i++ )
		{
			glyphs[ i ].glyph = RE_RegisterShader( glyphs[ i ].shaderName, RSF_NOMIP );
		}

		++fontUsage[ fontNo ];
		return fontNo;
	}

	if ( ftLibrary == nullptr )
	{
		ri.Printf( PRINT_WARNING, "RE_RegisterFont: FreeType not initialized.\n" );
		return -1;
	}

	// FIXME: fallback name OR index no.
	Q_strncpyz( (*font)->name, strippedName, sizeof( (*font)->name ) );

	Q_strncpyz( fileName, fontName, sizeof( fileName ) );

	len = RE_LoadFontFile( fileName, &faceData );

	if ( len <= 0 )
	{
		ri.Printf( PRINT_WARNING, "RE_RegisterFont: Unable to read font file %s\n", fileName );
		RE_FreeFontFile( faceData );
		return -1;
	}

	// allocate on the stack first in case we fail
	if ( FT_New_Memory_Face( ftLibrary, (FT_Byte*) faceData, len, 0, &face ) )
	{
		ri.Printf( PRINT_WARNING, "RE_RegisterFont: FreeType2, unable to allocate new face.\n" );
		RE_FreeFontFile( faceData );
		return -1;
	}

	if ( FT_Set_Char_Size( face, pointSize << 6, pointSize << 6, 72, 72 ) )
	{
		ri.Printf( PRINT_WARNING, "RE_RegisterFont: FreeType2, Unable to set face char size.\n" );
		FT_Done_Face( face );
		RE_FreeFontFile( faceData );
		return -1;
	}

	fallback = nullptr;
	fallbackData = nullptr;

	if ( fallbackName )
	{
		Q_strncpyz( fileName, fallbackName, sizeof( fileName ) );

		len = RE_LoadFontFile( fileName, &fallbackData );

		if ( len <= 0 )
		{
			ri.Printf( PRINT_WARNING, "RE_RegisterFont: Unable to read font file %s\n", fileName );
			RE_FreeFontFile( fallbackData );
			FT_Done_Face( face );
			RE_FreeFontFile( faceData );
			return -1;
		}

		// allocate on the stack first in case we fail
		if ( FT_New_Memory_Face( ftLibrary, (FT_Byte*) fallbackData, len, 0, &fallback ) )
		{
			ri.Printf( PRINT_WARNING, "RE_RegisterFont: FreeType2, unable to allocate new face.\n" );
			RE_FreeFontFile( fallbackData );
			FT_Done_Face( face );
			RE_FreeFontFile( faceData );
			return -1;
		}

		if ( FT_Set_Char_Size( fallback, pointSize << 6, pointSize << 6, 72, 72 ) )
		{
			ri.Printf( PRINT_WARNING, "RE_RegisterFont: FreeType2, Unable to set face char size.\n" );
			FT_Done_Face( fallback );
			RE_FreeFontFile( fallbackData );
			FT_Done_Face( face );
			RE_FreeFontFile( faceData );
			return -1;
		}
	}

	(*font)->face = face;
	(*font)->faceData = faceData;
	(*font)->fallback = fallback;
	(*font)->fallbackData = fallbackData;
	(*font)->pointSize = pointSize;
	(*font)->glyphScale = Com_Clamp( 24.0f, 64.0f, r_fontScale->value ) / pointSize;
	(*font)->height = ceil( ( face->height / 64.0 ) * ( face->size->metrics.y_scale / 65536.0 ) * (*font)->glyphScale );

	RE_RenderChunk( *font, 0 );

	++fontUsage[ fontNo ];
	return fontNo;
}

void RE_RegisterFont( const char *fontName, const char *fallbackName, int pointSize, fontInfo_t **font )
{
	RE_RegisterFont_Internal( fontName, fallbackName, pointSize, font );
}

void RE_RegisterFontVM( const char *fontName, const char *fallbackName, int pointSize, fontMetrics_t *metrics )
{
	fontInfo_t *font;
	fontHandle_t handle = RE_RegisterFont_Internal( fontName, fallbackName, pointSize, &font );

	if ( handle >= 0 )
	{
		++fontUsageVM[ handle ];
		metrics->handle = handle;
		metrics->isBitmap = !!font->face;
		metrics->pointSize = font->pointSize;
		metrics->height = font->height;
		metrics->glyphScale = font->glyphScale;
	}
	else
	{
		metrics->handle = -1;
	}
}

void R_InitFreeType()
{
	if ( FT_Init_FreeType( &ftLibrary ) )
	{
		ri.Printf( PRINT_WARNING, "R_InitFreeType: Unable to initialize FreeType.\n" );
	}
}

void RE_UnregisterFont_Internal( fontHandle_t handle )
{
	int i;

	if ( !fontUsage[ handle ] )
	{
		return;
	}

	if ( --fontUsage[ handle ] )
	{
		return;
	}


	if ( registeredFont[ handle ].face )
	{
		FT_Done_Face( (FT_Face) registeredFont[ handle ].face );
		RE_FreeFontFile( registeredFont[ handle ].faceData );
	}

	if ( registeredFont[ handle ].fallback )
	{
		FT_Done_Face( (FT_Face) registeredFont[ handle ].fallback );
		RE_FreeFontFile( registeredFont[ handle ].fallbackData );
	}

	for ( i = 0; i < 0x1100; ++i )
	{
		if ( registeredFont[ handle ].glyphBlock[ i ] && registeredFont[ handle ].glyphBlock[ i ] != nullGlyphs )
		{
			ri.Free( registeredFont[ handle ].glyphBlock[ i ] );
			registeredFont[ handle ].glyphBlock[ i ] = nullptr;
		}
	}

	memset( &registeredFont[ handle ], 0, sizeof( fontInfo_t ) );
}

void RE_UnregisterFont( fontInfo_t *font )
{
	int i;

	for ( i = 0; i < MAX_FONTS; ++i )
	{
		if ( !fontUsage[ i ] )
		{
			continue;
		}

		if ( font && font != &registeredFont[ i ] )
		{
			continue; // name & size don't match
		}

		RE_UnregisterFont_Internal( i );

		if ( font )
		{
			break;
		}
	}
}

void RE_UnregisterFontVM( fontHandle_t handle )
{
	if ( handle >= 0 && handle < MAX_FONTS && fontUsageVM[ handle ] )
	{
		--fontUsageVM[ handle ];
		RE_UnregisterFont_Internal( handle );
	}
}

void R_DoneFreeType()
{
	if ( ftLibrary )
	{
		RE_UnregisterFont( nullptr );
		FT_Done_FreeType( ftLibrary );
		ftLibrary = nullptr;
	}
}
