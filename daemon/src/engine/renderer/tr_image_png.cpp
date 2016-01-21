/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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
#include "tr_local.h"

// Work around bug in old versions of libpng
#define PNG_SKIP_SETJMP_CHECK
#include <png.h>

/*
=========================================================

PNG LOADING

=========================================================
*/
static void png_read_data( png_structp png, png_bytep data, png_size_t length )
{
	byte *io_ptr = (byte*) png_get_io_ptr( png );
	Com_Memcpy( data, io_ptr, length );
	png_init_io( png, ( png_FILE_p )( io_ptr + length ) );
}

static void png_user_warning_fn( png_structp, png_const_charp warning_message )
{
	Log::Warn("libpng warning: %s", warning_message );
}

static void NORETURN png_user_error_fn( png_structp png_ptr, png_const_charp error_message )
{
	Log::Warn("libpng error: %s", error_message );
	longjmp( png_jmpbuf( png_ptr ), 0 );
}

void LoadPNG( const char *name, byte **pic, int *width, int *height,
	      int*, int*, int*, byte alphaByte )
{
	int          bit_depth;
	int          color_type;
	png_uint_32  w;
	png_uint_32  h;
	unsigned int row;
	png_infop    info;
	png_structp  png;
	png_bytep    *row_pointers;
	byte         *data;
	byte         *out;

	// load png
	ri.FS_ReadFile( name, ( void ** ) &data );

	if ( !data )
	{
		return;
	}

	png = png_create_read_struct( PNG_LIBPNG_VER_STRING, ( png_voidp ) nullptr, png_user_error_fn, png_user_warning_fn );

	if ( !png )
	{
		Log::Warn("LoadPNG: png_create_write_struct() failed for (%s)", name );
		ri.FS_FreeFile( data );
		return;
	}

	// allocate/initialize the memory for image information.  REQUIRED
	info = png_create_info_struct( png );

	if ( !info )
	{
		Log::Warn("LoadPNG: png_create_info_struct() failed for (%s)", name );
		ri.FS_FreeFile( data );
		png_destroy_read_struct( &png, ( png_infopp ) nullptr, ( png_infopp ) nullptr );
		return;
	}

	/*
	 * Set error handling if you are using the setjmp/longjmp method (this is
	 * the common method of doing things with libpng).  REQUIRED unless you
	 * set up your own error handlers in the png_create_read_struct() earlier.
	 */
	if ( setjmp( png_jmpbuf( png ) ) )
	{
		// if we get here, we had a problem reading the file
		Log::Warn("LoadPNG: first exception handler called for (%s)", name );
		ri.FS_FreeFile( data );
		png_destroy_read_struct( &png, ( png_infopp ) & info, ( png_infopp ) nullptr );
		return;
	}

	png_set_read_fn( png, data, png_read_data );

	png_set_sig_bytes( png, 0 );

	// The call to png_read_info() gives us all of the information from the
	// PNG file before the first IDAT (image data chunk).  REQUIRED
	png_read_info( png, info );

	// get picture info
	png_get_IHDR( png, info, ( png_uint_32 * ) &w, ( png_uint_32 * ) &h, &bit_depth, &color_type, nullptr, nullptr, nullptr );

	// tell libpng to strip 16 bit/color files down to 8 bits/color
	png_set_strip_16( png );

	// expand paletted images to RGB triplets
	if ( color_type & PNG_COLOR_MASK_PALETTE )
	{
		png_set_expand( png );
	}

	// expand gray-scaled images to RGB triplets
	if ( !( color_type & PNG_COLOR_MASK_COLOR ) )
	{
		png_set_gray_to_rgb( png );
	}

	// expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel

	// expand paletted or RGB images with transparency to full alpha channels
	// so the data will be available as RGBA quartets
	if ( png_get_valid( png, info, PNG_INFO_tRNS ) )
	{
		png_set_tRNS_to_alpha( png );
	}

	// if there is no alpha information, fill with alphaByte
	if ( !( color_type & PNG_COLOR_MASK_ALPHA ) )
	{
		png_set_filler( png, alphaByte, PNG_FILLER_AFTER );
	}

	// expand pictures with less than 8bpp to 8bpp
	if ( bit_depth < 8 )
	{
		png_set_packing( png );
	}

	png_set_interlace_handling( png );

	// update structure with the above settings
	png_read_update_info( png, info );

	// allocate the memory to hold the image
	*width = w;
	*height = h;
	*pic = out = ( byte * ) ri.Z_Malloc( w * h * 4 );

	row_pointers = ( png_bytep * ) ri.Hunk_AllocateTempMemory( sizeof( png_bytep ) * h );

	// set a new exception handler
	if ( setjmp( png_jmpbuf( png ) ) )
	{
		Log::Warn("LoadPNG: second exception handler called for (%s)", name );
		ri.Hunk_FreeTempMemory( row_pointers );
		ri.FS_FreeFile( data );
		png_destroy_read_struct( &png, ( png_infopp ) & info, ( png_infopp ) nullptr );
		return;
	}

	for ( row = 0; row < h; row++ )
	{
		row_pointers[ row ] = ( png_bytep )( out + ( row * 4 * w ) );
	}

	// read image data
	png_read_image( png, row_pointers );

	// read rest of file, and get additional chunks in info
	png_read_end( png, info );

	// clean up after the read, and free any memory allocated
	png_destroy_read_struct( &png, &info, ( png_infopp ) nullptr );

	ri.Hunk_FreeTempMemory( row_pointers );
	ri.FS_FreeFile( data );
}

/*
=========================================================

PNG SAVING

=========================================================
*/
static int png_compressed_size;

static void png_write_data( png_structp png, png_bytep data, png_size_t length )
{
	byte *io_ptr = (byte*) png_get_io_ptr( png );
	Com_Memcpy( io_ptr, data, length );
	png_init_io( png, ( png_FILE_p )( io_ptr + length ) );
	png_compressed_size += length;
}

static void png_flush_data( png_structp )
{
}

void SavePNG( const char *name, const byte *pic, int width, int height, int numBytes, bool flip )
{
	png_structp png;
	png_infop   info;
	int         i;
	int         row_stride;
	byte        *buffer;
	byte        *row;
	png_bytep   *row_pointers;

	png = png_create_write_struct( PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr );

	if ( !png )
	{
		return;
	}

	// Allocate/initialize the image information data
	info = png_create_info_struct( png );

	if ( !info )
	{
		png_destroy_write_struct( &png, ( png_infopp ) nullptr );
		return;
	}

	png_compressed_size = 0;
	buffer = (byte*) ri.Hunk_AllocateTempMemory( width * height * numBytes );

	// set error handling
	if ( setjmp( png_jmpbuf( png ) ) )
	{
		ri.Hunk_FreeTempMemory( buffer );
		png_destroy_write_struct( &png, &info );
		return;
	}

	png_set_write_fn( png, buffer, png_write_data, png_flush_data );

	switch ( numBytes )
	{
	default:
		png_set_IHDR( png, info, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		              PNG_FILTER_TYPE_DEFAULT );
		break;
	case 3:
		png_set_IHDR( png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		              PNG_FILTER_TYPE_DEFAULT );
		break;
	case 2:
		png_set_IHDR( png, info, width, height, 8, PNG_COLOR_TYPE_GA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		              PNG_FILTER_TYPE_DEFAULT );
		break;
	case 1:
		png_set_IHDR( png, info, width, height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		              PNG_FILTER_TYPE_DEFAULT );
		break;
	}

	// write the file header information
	png_write_info( png, info );

	row_pointers = (png_bytep*) ri.Hunk_AllocateTempMemory( height * sizeof( png_bytep ) );

	if ( setjmp( png_jmpbuf( png ) ) )
	{
		ri.Hunk_FreeTempMemory( row_pointers );
		ri.Hunk_FreeTempMemory( buffer );
		png_destroy_write_struct( &png, &info );
		return;
	}

	row_stride = width * numBytes;
	row = ( byte * ) pic + ( height - 1 ) * row_stride;

	if ( flip )
	{
		for ( i = height - 1; i >= 0; i-- )
		{
			row_pointers[ i ] = row;
			row -= row_stride;
		}
	}
	else
	{
		for ( i = 0; i < height; i++ )
		{
			row_pointers[ i ] = row;
			row -= row_stride;
		}
	}

	png_write_image( png, row_pointers );
	png_write_end( png, info );

	// clean up after the write, and free any memory allocated
	png_destroy_write_struct( &png, &info );

	ri.Hunk_FreeTempMemory( row_pointers );

	ri.FS_WriteFile( name, buffer, png_compressed_size );

	ri.Hunk_FreeTempMemory( buffer );
}
