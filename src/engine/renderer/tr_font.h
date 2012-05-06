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
// 3. run the game, you should see things normally.
// 4. Exit the game and there will be three dat files and at least three PNG files. The
//    PNG's are in 256x256 pages so if it takes three images to render a 24 point font you
//    will end up with fontImage_0_24.tga through fontImage_2_24.tga
// 5. In future runs of the game, the system looks for these images and data files when a
//    specific point sized font is rendered and loads them for use.
// 6. Because of the original beta nature of the FreeType code you will probably want to hand
//    touch the font bitmaps.

#include "../qcommon/qcommon.h"

#include <png.h>

#ifdef BUILD_FREETYPE
#include <freetype/ftsystem.h>
#include <freetype/ftimage.h>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>

#define _FLOOR( x ) ( ( x ) & - 64 )
#define _CEIL( x )  ( ( ( x ) + 63 ) & - 64 )
#define _TRUNC( x ) ( ( x ) >> 6 )

FT_Library ftLibrary = NULL;
#endif

#define FONT_SIZE 512

#define MAX_FONTS 16
static int        registeredFontCount = 0;
static fontInfo_t registeredFont[ MAX_FONTS ];

#ifdef BUILD_FREETYPE
void R_GetGlyphInfo( FT_GlyphSlot glyph, int *left, int *right, int *width, int *top, int *bottom, int *height, int *pitch )
{
	*left = _FLOOR( glyph->metrics.horiBearingX );
	*right = _CEIL( glyph->metrics.horiBearingX + glyph->metrics.width );
	*width = _TRUNC( *right - *left );

	*top = _CEIL( glyph->metrics.horiBearingY );
	*bottom = _FLOOR( glyph->metrics.horiBearingY - glyph->metrics.height );
	*height = _TRUNC( *top - *bottom );
	*pitch = ( qtrue ? ( *width + 3 ) & - 4 : ( *width + 7 ) >> 3 );
}

FT_Bitmap      *R_RenderGlyph( FT_GlyphSlot glyph, glyphInfo_t *glyphOut )
{
	FT_Bitmap *bit2;
	int       left, right, width, top, bottom, height, pitch, size;

	R_GetGlyphInfo( glyph, &left, &right, &width, &top, &bottom, &height, &pitch );

	if ( glyph->format == ft_glyph_format_outline )
	{
		size = pitch * height;

		bit2 = ri.Z_Malloc( sizeof( FT_Bitmap ) );

		bit2->width = width;
		bit2->rows = height;
		bit2->pitch = pitch;
		bit2->pixel_mode = ft_pixel_mode_grays;
		//bit2->pixel_mode = ft_pixel_mode_mono;
		bit2->buffer = ri.Z_Malloc( pitch * height );
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
		ri.Printf( PRINT_ALL, "Non-outline fonts are not supported\n" );
	}

	return NULL;
}

static glyphInfo_t *RE_ConstructGlyphInfo( unsigned char *imageOut, int *xOut, int *yOut,
    int *maxHeight, FT_Face face, const unsigned char c, qboolean calcHeight )
{
	int                i;
	static glyphInfo_t glyph;
	unsigned char      *src, *dst;
	float              scaledWidth, scaledHeight;
	FT_Bitmap          *bitmap = NULL;

	Com_Memset( &glyph, 0, sizeof( glyphInfo_t ) );

	// make sure everything is here
	if ( face != NULL )
	{
		FT_Load_Glyph( face, FT_Get_Char_Index( face, c ), FT_LOAD_DEFAULT );
		bitmap = R_RenderGlyph( face->glyph, &glyph );

		if ( bitmap )
		{
			glyph.xSkip = ( face->glyph->metrics.horiAdvance >> 6 ) + 1;
		}
		else
		{
			return &glyph;
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

		/*
		    // need to convert to power of 2 sizes so we do not get
		    // any scaling from the gl upload
		        for (scaled_width = 1 ; scaled_width < glyph.pitch ; scaled_width<<=1)
		                ;
		        for (scaled_height = 1 ; scaled_height < glyph.height ; scaled_height<<=1)
		                ;
		*/

		scaledWidth = glyph.pitch;
		scaledHeight = glyph.height;

		// we need to make sure we fit
		if ( *xOut + scaledWidth + 1 >= ( FONT_SIZE - 1 ) )
		{
			if ( *yOut + ( *maxHeight + 1 ) * 2 >= ( FONT_SIZE - 1 ) )
				//if(*yOut + scaledHeight + 1 >= 255)
			{
				//ri.Printf(PRINT_WARNING, "RE_ConstructGlyphInfo: character %c does not fit width and height\n", c);

				*yOut = -1;
				*xOut = -1;
				ri.Free( bitmap->buffer );
				ri.Free( bitmap );
				return &glyph;
			}
			else
			{
				//ri.Printf(PRINT_WARNING, "RE_ConstructGlyphInfo: character %c does not fit width\n", c);

				*xOut = 0;
				*yOut += *maxHeight + 1;
			}
		}
		else if ( *yOut + *maxHeight + 1 >= ( FONT_SIZE - 1 ) )
		{
			//ri.Printf(PRINT_WARNING, "RE_ConstructGlyphInfo: character %c does not fit height\n", c);

			*yOut = -1;
			*xOut = -1;
			ri.Free( bitmap->buffer );
			ri.Free( bitmap );
			return &glyph;
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

		*xOut += scaledWidth + 1;
	}

	ri.Free( bitmap->buffer );
	ri.Free( bitmap );

	return &glyph;
}

#endif

static int  fdOffset;
static byte *fdFile;

int readInt()
{
	int i =
	  fdFile[ fdOffset ] + ( fdFile[ fdOffset + 1 ] << 8 ) + ( fdFile[ fdOffset + 2 ] << 16 ) + ( fdFile[ fdOffset + 3 ] << 24 );
	fdOffset += 4;
	return i;
}

typedef union
{
	byte  fred[ 4 ];
	float ffred;
} poor;

float readFloat()
{
	poor me;

#if idppc
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

#ifdef BUILD_FREETYPE

static unsigned long stream_read(FT_Stream stream, unsigned long offset, unsigned char *buffer, unsigned long count )
{
	if( count == 0 )
	{
		int r = ri.FS_Seek( (fileHandle_t) stream->descriptor.value, (long) offset, FS_SEEK_SET );
		if( r == offset )
			return 0;
		else
			return r;
	}

	if( !buffer )
	{
		ri.Printf(PRINT_ALL, "stream_read: buffer is NULL\n");
		return 0;
	}
	if( !stream->descriptor.value )
	{
		ri.Printf(PRINT_ALL, "stream_read: stream->descriptor.value is zero\n");
		return 0;
	}

	if( ri.FS_FTell( ( fileHandle_t ) stream->descriptor.value ) != (long) offset )
		ri.FS_Seek( (fileHandle_t) stream->descriptor.value, (long) offset,  FS_SEEK_SET );

	return ri.FS_Read( (char *) buffer, (long) count, (fileHandle_t) stream->descriptor.value );
}

static void stream_close(FT_Stream stream)
{
	ri.FS_FCloseFile( (fileHandle_t) stream->descriptor.value );
}

static void *memory_alloc(FT_Memory memory, long size)
{
	return malloc( size );
}

static void memory_free(FT_Memory memory, void *block)
{
	free( block );
}

static void *memory_realloc(FT_Memory memory, long cur_size, long new_size, void *block)
{
	return realloc( block, new_size );
}

void RE_LoadFace(const char *fileName, int pointSize, const char *name, face_t *face)
{
	fileHandle_t h;
	static struct FT_MemoryRec_ memory =
	{
		NULL, memory_alloc, memory_free, memory_realloc
	};
	FT_Stream stream;
	FT_Open_Args oa =
	{
		FT_OPEN_STREAM, NULL, 0L, NULL, NULL /* stream */, NULL /* driver */, 0, NULL
	};
	FT_Face ftFace;
	float dpi = 72.0f;
	float glyphScale =  72.0f / dpi; // change the scale to be relative to 1 based on 72 dpi ( so dpi of 144 means a scale of .5 )
	int ec;

	if( !face || !fileName || !name )
	{
		return;
	}

	if( ri.FS_ReadFile( fileName, NULL ) <= 0 )
	{
		return;
	}

	oa.stream = face->mem = stream = malloc(sizeof(*stream));

	ri.FS_FOpenFileRead(fileName, &h, qtrue);
	stream->base = NULL;
	stream->size = ri.FS_ReadFile(fileName, NULL);
	stream->pos = 0;
	stream->descriptor.value = h;
	stream->pathname.pointer = (void *) fileName;
	stream->read = (FT_Stream_IoFunc) stream_read;
	stream->close = (FT_Stream_CloseFunc) stream_close;
	stream->memory = &memory;
	stream->cursor = NULL;
	stream->limit = NULL;

	if (ftLibrary == NULL)
	{
		ri.Printf(PRINT_ALL, "RE_LoadFace: FreeType not initialized.\n");
		return;
	}

	if( ( ec = FT_Open_Face( ftLibrary, &oa, 0, (FT_Face *) &( face->opaque ) ) ) != 0 )
	{
		ri.Printf(PRINT_ALL, "RE_LoadFace: FreeType2, Unable to open face; error code: %d\n", ec);
		return;
	}

	ftFace = (FT_Face) face->opaque;

	if( !ftFace )
	{
		ri.Printf(PRINT_ALL, "RE_LoadFace: face handle is NULL");
		return;
	}

	if (pointSize <= 0)
	{
		pointSize = 12;
	}

	// we also need to adjust the scale based on point size relative to 48 points as the ui scaling is based on a 48 point font
	glyphScale *= 48.0f / pointSize;

	face->glyphScale = glyphScale;
	strncpy(face->name, name, MAX_QPATH);

	if((ec = FT_Set_Char_Size( ftFace, pointSize << 6, pointSize << 6, dpi, dpi)) != 0)
	{
		ri.Printf(PRINT_ALL, "RE_LoadFace: FreeType2, Unable to set face char size; error code: %d\n", ec);
		return;
	}
}

void RE_FreeFace(face_t *face)
{
	FT_Face ftFace;

	if( !face )
		return;

	ftFace = face->opaque;

	if( !ftFace )
	{
		if( face->mem )
		{
			free( face->mem );
			face->mem = NULL;
		}

		return;
	}

	if( ftLibrary )
	{
		FT_Done_Face( (FT_Face) ftFace );
		face->opaque = NULL;
	}

	if( face->mem )
	{
		free( face->mem );
		face->mem = NULL;
	}
}

void RE_LoadGlyphChar(face_t *face, int ch, int img, glyphInfo_t *glyphInfo)
{
	FT_Face ftFace = face ? (FT_Face) face->opaque : NULL;
	glyphInfo_t *tmp;
	int i;
	int x = 0, y = 0, maxHeight = 0;
	int left, max;
	static char name[MAX_QPATH];
	image_t *image;
	qhandle_t h;
	static unsigned char buf[8*256*256];
	static unsigned char imageBuf[4*256*256];

	if( !face || !ftFace )
		return;

	if( img > MAX_FACE_GLYPHS )
	{
		ri.Printf(PRINT_ALL, "RE_LoadGlyph: img > MAX_FACE_GLYPHS\n");

		return;
	}

	Com_Memset(buf, 0, sizeof(buf));
	tmp = RE_ConstructGlyphInfo(buf, &x, &y, &maxHeight, ftFace, ch, qfalse);
	if( x == -1 || y == -1 )
	{
		Com_Memset(buf, 0, sizeof(buf));
		Com_Memset(imageBuf, 0, sizeof(imageBuf));
		return;
	}
	Com_Memcpy(glyphInfo, tmp, sizeof(glyphInfo_t));

	left = 0;
	max = 0;
	for(i = 0; i < 32*32; i++)
	{
		if(max < buf[i])
		{
			max = buf[i];
		}
	}

	if(max > 0)
	{
		max = 255/max;
	}

	//for(i = 0; i < (scaledSize); i++)
	for(i = 0; i < (32*32); i++)
	{
		imageBuf[left++] = 255;
		imageBuf[left++] = 255;
		imageBuf[left++] = 255;

		imageBuf[left++] = ((float)buf[i] * max);
	}

	Com_sprintf( name, sizeof(name), "./../._FONT_%d", img );
	//{static int n = 0; Com_sprintf( name, sizeof(name), "./../._FONT_%d", n++ );}

	image = R_CreateImage(name, imageBuf, 32, 32, qfalse, qfalse, GL_CLAMP_TO_EDGE);
	face->images[ img ] = (void *) image;
#ifdef RENDERER_GL3
	h = RE_RegisterShaderFromImage(name, image, qfalse);
#else
	h = RE_RegisterShaderFromImage(name, LIGHTMAP_2D, image, qfalse);
#endif
	Com_Memset(buf, 0, sizeof(buf));
	Com_Memset(imageBuf, 0, sizeof(imageBuf));

	glyphInfo->glyph = h;
	strncpy( glyphInfo->shaderName, name, sizeof( glyphInfo->shaderName ) );
}

void RE_LoadGlyph(face_t *face, const char *str, int img, glyphInfo_t *glyphInfo)
{
	RE_LoadGlyphChar( face, Q_UTF8CodePoint( str ), img, glyphInfo );
}

void RE_FreeGlyph(face_t *face, int img, glyphInfo_t *glyphInfo)
{
	image_t *image;

	if( !face || !glyphInfo )
		return;

	if( img > MAX_FACE_GLYPHS )
	{
		ri.Printf(PRINT_ALL, "RE_LoadGlyph: img > MAX_FACE_GLYPHS\n");

		return;
	}

	image = (image_t *) face->images[ img ];

	if( !image )
		return;

	face->images[ img ] = NULL;
}

typedef struct
{
	qboolean    used;
	int         ch;
	glyphInfo_t glyph;
	face_t      *face;
} glyphCache_t;

static glyphCache_t glyphCache[MAX_FACE_GLYPHS] = {{0}}, *nextCache = glyphCache;

void RE_GlyphChar( fontInfo_t *font, face_t *face, int ch, glyphInfo_t *glyph )
{
	int    i;
	int    width;

	if( ch < 0x100 || !face )
	{
		memcpy( glyph, &font->glyphs[ ch ], sizeof( *glyph ) );
		return;
	}

	for( i = 0; i < MAX_FACE_GLYPHS; i++ )
	{
		glyphCache_t *c = &glyphCache[ i ];

		if( c->used && c->face == face )
		{
			if( !face->images[ c - glyphCache ] )
			{
				// was freed
				c->used = qfalse;
			}
			else if ( ch == c->ch )
			{
				memcpy( glyph, &c->glyph, sizeof( *glyph ) );
				return;
			}
		}
	}

	if( nextCache->used )
	{
		RE_FreeGlyph( nextCache->face, nextCache - glyphCache, &nextCache->glyph );
	}

	RE_LoadGlyphChar( face, ch, nextCache - glyphCache, &nextCache->glyph );
	nextCache->face = face;

	nextCache->ch = ch;
	nextCache->used = qtrue;

	memcpy( glyph, &nextCache->glyph, sizeof( *glyph ) );

	if( ++nextCache - glyphCache >= MAX_FACE_GLYPHS )
		nextCache = glyphCache;
}

void RE_Glyph( fontInfo_t *font, face_t *face, const char *str, glyphInfo_t *glyph )
{
	RE_GlyphChar( font, face, Q_UTF8CodePoint( str ), glyph );
}

void RE_FreeCachedGlyphs( face_t *face )
{
	int i;

	for( i = 0; i < MAX_FACE_GLYPHS; i++ )
	{
		glyphCache_t *c = &glyphCache[ i ];

		if( c->used )
		{
			if( c->face == face )
			{
				c->used = qfalse;

				RE_FreeGlyph( face, i, &c->glyph );
			}
		}
	}
}

static void RE_ClearGlyphCache( void )
{
	int i;

	for( i = 0; i < MAX_FACE_GLYPHS; i++ )
	{
		glyphCache_t *c = &glyphCache[ i ];

		if( c->used )
		{
			c->used = qfalse;

			RE_FreeGlyph( c->face, i, &c->glyph );
		}
	}
}
#else
void RE_LoadFace(const char *fileName, int pointSize, const char *name, face_t *face)
{
}
void RE_FreeFace(face_t *face)
{
}
void RE_LoadGlyph(face_t *face, const char *str, glyphInfo_t *glyphInfo)
{
}
void RE_FreeGlyph(face_t *face, glyphInfo_t *glyphInfo)
{
}
void RE_Glyph( fontInfo_t *font, face_t *face, const char *str, glyphInfo_t *glyph )
{
	memcpy( glyph, &font->glyphs[ (int)*str ], sizeof( *glyph ) );
}
void RE_FreeCachedGlyphs( face_t *face )
{
}
static void RE_ClearGlyphCache( void )
{
}
#endif


void RE_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font )
{
#ifdef BUILD_FREETYPE
	FT_Face       face;
	int           j, k, xOut, yOut, lastStart, imageNumber;
	int           scaledSize, newSize, maxHeight, left;
	unsigned char *out, *imageBuff;
	glyphInfo_t   *glyph;
	image_t       *image;
	qhandle_t     h;
	float         max;
#endif
	void          *faceData;
	int           i, len;
	char          fileName[ MAX_QPATH ];
	char          strippedName[ MAX_QPATH ];
	float         dpi = 72; //
	float         glyphScale = 72.0f / dpi; // change the scale to be relative to 1 based on 72 dpi ( so dpi of 144 means a scale of .5 )

	if ( pointSize <= 0 )
	{
		pointSize = 12;
	}

	// we also need to adjust the scale based on point size relative to 48 points as the ui scaling is based on a 48 point font
	glyphScale *= 48.0f / pointSize;

	// make sure the render thread is stopped
	R_SyncRenderThread();

	if ( registeredFontCount >= MAX_FONTS )
	{
		ri.Printf( PRINT_WARNING, "RE_RegisterFont: Too many fonts registered already.\n" );
		return;
	}

	COM_StripExtension2( fontName, strippedName, sizeof( strippedName ) );

	if ( !Q_stricmp( strippedName, fontName ) )
	{
		Com_sprintf( fileName, sizeof( fileName ), "fonts/fontImage_%i.dat", pointSize );
	}
	else
	{
		Com_sprintf( fileName, sizeof( fileName ), "fonts/%s_%i.dat", strippedName, pointSize );
	}

	for ( i = 0; i < registeredFontCount; i++ )
	{
		if ( Q_stricmp( fileName, registeredFont[ i ].name ) == 0 )
		{
			Com_Memcpy( font, &registeredFont[ i ], sizeof( fontInfo_t ) );
			return;
		}
	}

#if defined( COMPAT_ET ) // DON'T DO THIS WITH VANILLA XREAL
	len = ri.FS_ReadFile( fileName, NULL );

	if ( len == sizeof( fontInfo_t ) )
	{
		ri.FS_ReadFile( fileName, &faceData );
		fdOffset = 0;
		fdFile = faceData;

		for ( i = 0; i < GLYPHS_PER_FONT; i++ )
		{
			font->glyphs[ i ].height = readInt();
			font->glyphs[ i ].top = readInt();
			font->glyphs[ i ].bottom = readInt();
			font->glyphs[ i ].pitch = readInt();
			font->glyphs[ i ].xSkip = readInt();
			font->glyphs[ i ].imageWidth = readInt();
			font->glyphs[ i ].imageHeight = readInt();
			font->glyphs[ i ].s = readFloat();
			font->glyphs[ i ].t = readFloat();
			font->glyphs[ i ].s2 = readFloat();
			font->glyphs[ i ].t2 = readFloat();
			font->glyphs[ i ].glyph = readInt();
			Com_Memcpy( font->glyphs[ i ].shaderName, &fdFile[ fdOffset ], 32 );
			fdOffset += 32;
		}

		font->glyphScale = readFloat();
		Com_Memcpy( font->name, &fdFile[ fdOffset ], 64 );

//      Com_Memcpy(font, faceData, sizeof(fontInfo_t));
		Q_strncpyz( font->name, fileName, sizeof( font->name ) );

		for ( i = GLYPH_START; i < GLYPH_END; i++ )
		{
			font->glyphs[ i ].glyph = RE_RegisterShaderNoMip( font->glyphs[ i ].shaderName );
		}

		Com_Memcpy( &registeredFont[ registeredFontCount++ ], font, sizeof( fontInfo_t ) );
		return;
	}

#endif

#ifndef BUILD_FREETYPE
	ri.Printf( PRINT_ALL, "RE_RegisterFont: FreeType code not available\n" );
#else

	if ( ftLibrary == NULL )
	{
		ri.Printf( PRINT_WARNING, "RE_RegisterFont: FreeType not initialized.\n" );
		return;
	}

	Com_sprintf( fileName, sizeof( fileName ), "%s", fontName );

	len = ri.FS_ReadFile( fileName, &faceData );

	if ( len <= 0 )
	{
		ri.Printf( PRINT_ALL, "RE_RegisterFont: Unable to read font file %s\n", fileName );
		return;
	}

	// allocate on the stack first in case we fail
	if ( FT_New_Memory_Face( ftLibrary, faceData, len, 0, &face ) )
	{
		ri.Printf( PRINT_WARNING, "RE_RegisterFont: FreeType2, unable to allocate new face.\n" );
		return;
	}

	if ( FT_Set_Char_Size( face, pointSize << 6, pointSize << 6, dpi, dpi ) )
	{
		ri.Printf( PRINT_WARNING, "RE_RegisterFont: FreeType2, Unable to set face char size.\n" );
		return;
	}

	// make a 256x256 image buffer, once it is full, register it, clean it and keep going
	// until all glyphs are rendered

	out = ri.Z_Malloc( 1024 * 1024 );

	if ( out == NULL )
	{
		ri.Printf( PRINT_WARNING, "RE_RegisterFont: ri.Malloc failure during output image creation.\n" );
		return;
	}

	Com_Memset( out, 0, 1024 * 1024 );

	// calculate max height
	maxHeight = 0;

	for ( i = GLYPH_START; i < GLYPH_END; i++ )
	{
		glyph = RE_ConstructGlyphInfo( out, &xOut, &yOut, &maxHeight, face, ( unsigned char ) i, qtrue );
	}

	//ri.Printf(PRINT_WARNING, "RE_RegisterFont: max glyph height for font %s is %i\n", strippedName, maxHeight);

	xOut = 0;
	yOut = 0;
	i = GLYPH_START;
	lastStart = i;
	imageNumber = 0;

	while ( i <= GLYPH_END )
	{
		glyph = RE_ConstructGlyphInfo( out, &xOut, &yOut, &maxHeight, face, ( unsigned char ) i, qfalse );

		if ( xOut == -1 || yOut == -1 || i == GLYPH_END )
		{
			if ( xOut == -1 || yOut == -1 )
			{
				//ri.Printf(PRINT_WARNING, "RE_RegisterFont: character %c does not fit image number %i\n", (unsigned char) i, imageNumber);
			}

			// ran out of room
			// we need to create an image from the bitmap, set all the handles in the glyphs to this point
			scaledSize = FONT_SIZE * FONT_SIZE;
			newSize = scaledSize * 4;
			imageBuff = ri.Z_Malloc( newSize );
			left = 0;
			max = 0;

			for ( k = 0; k < ( scaledSize ); k++ )
			{
				if ( max < out[ k ] )
				{
					max = out[ k ];
				}
			}

			if ( max > 0 )
			{
				max = 255 / max;
			}

			for ( k = 0; k < ( scaledSize ); k++ )
			{
				imageBuff[ left++ ] = 255;
				imageBuff[ left++ ] = 255;
				imageBuff[ left++ ] = 255;

				imageBuff[ left++ ] = ( ( float ) out[ k ] * max );
			}

			Com_sprintf( fileName, sizeof( fileName ), "%s_%i_%i.png", strippedName, imageNumber++, pointSize );

			if ( !ri.FS_FileExists( fileName ) )
			{
				SavePNG( fileName, imageBuff, FONT_SIZE, FONT_SIZE, 4, qtrue );
			}

#ifdef RENDERER_GL3
			image = R_CreateImage( fileName, imageBuff, FONT_SIZE, FONT_SIZE, IF_NOPICMIP, FT_LINEAR, WT_CLAMP );
			h = RE_RegisterShaderFromImage( fileName, image, qfalse );
#else
			image = R_CreateImage( fileName, imageBuff, FONT_SIZE, FONT_SIZE, qfalse, qfalse, GL_CLAMP_TO_EDGE );
			h = RE_RegisterShaderFromImage( fileName, LIGHTMAP_2D, image, qfalse );
#endif

			Com_Memset( out, 0, 1024 * 1024 );
			xOut = 0;
			yOut = 0;
			ri.Free( imageBuff );

			if ( i == GLYPH_END )
			{
				for ( j = lastStart; j <= GLYPH_END; j++ )
				{
					font->glyphs[ j ].glyph = h;
					Q_strncpyz( font->glyphs[ j ].shaderName, fileName, sizeof( font->glyphs[ j ].shaderName ) );
				}

				break;
			}
			else
			{
				for ( j = lastStart; j < i; j++ )
				{
					font->glyphs[ j ].glyph = h;
					Q_strncpyz( font->glyphs[ j ].shaderName, fileName, sizeof( font->glyphs[ j ].shaderName ) );
				}

				lastStart = i;
			}
		}
		else
		{
			Com_Memcpy( &font->glyphs[ i ], glyph, sizeof( glyphInfo_t ) );
			i++;
		}
	}

	registeredFont[ registeredFontCount ].glyphScale = glyphScale;
	font->glyphScale = glyphScale;

	Com_sprintf( fileName, sizeof( fileName ), "%s_%i.dat", strippedName, pointSize );
	Q_strncpyz( font->name, fileName, sizeof( font->name ) );

	if ( !ri.FS_FileExists( fileName ) )
	{
		ri.FS_WriteFile( fileName, font, sizeof( fontInfo_t ) );
	}

	Com_Memcpy( &registeredFont[ registeredFontCount++ ], font, sizeof( fontInfo_t ) );

	ri.Free( out );

	ri.FS_FreeFile( faceData );
#endif
}

void R_InitFreeType()
{
#ifdef BUILD_FREETYPE

	if ( FT_Init_FreeType( &ftLibrary ) )
	{
		ri.Printf( PRINT_ALL, "R_InitFreeType: Unable to initialize FreeType.\n" );
	}

#endif
	registeredFontCount = 0;
}

void R_DoneFreeType()
{
#ifdef BUILD_FREETYPE

	if ( ftLibrary )
	{
		RE_ClearGlyphCache();
		FT_Done_FreeType( ftLibrary );
		ftLibrary = NULL;
	}

#endif
	registeredFontCount = 0;
}
