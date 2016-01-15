/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2007 HermitWorks Entertainment Corporation
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

#include "tr_local.h"

struct DDCOLORKEY_t
{
	unsigned int dwColorSpaceLowValue; // low boundary of color space that is to
	// be treated as Color Key, inclusive

	unsigned int dwColorSpaceHighValue; // high boundary of color space that is
	// to be treated as Color Key, inclusive
};

struct DDSCAPS2_t
{
	unsigned int dwCaps; // capabilities of surface wanted
	unsigned int dwCaps2;
	unsigned int dwCaps3;
	unsigned int dwCaps4;
};

struct DDS_PIXELFORMAT_t
{
	unsigned int dwSize; // size of structure
	unsigned int dwFlags; // pixel format flags
	unsigned int dwFourCC; // (FOURCC code)

	unsigned int dwRGBBitCount; // how many bits per pixel
	unsigned int dwRBitMask; // mask for red bit
	unsigned int dwGBitMask; // mask for green bits
	unsigned int dwBBitMask; // mask for blue bits
	unsigned int dwABitMask; // mask for alpha channel
};

struct DDSHEADER_t
{
	unsigned int dwSize; // size of the DDSURFACEDESC structure
	unsigned int dwFlags; // determines what fields are valid
	unsigned int dwHeight; // height of surface to be created
	unsigned int dwWidth; // width of input surface

	unsigned int dwPitchOrLinearSize; // Formless late-allocated optimized surface size

	unsigned int dwDepth; // the depth if this is a volume texture

	unsigned int dwMipMapCount; // number of mip-map levels requestde

	unsigned int dwReserved[11]; // reserved

	DDS_PIXELFORMAT_t ddpfPixelFormat; // pixel format description of the surface

	DDSCAPS2_t   ddsCaps; // direct draw surface capabilities
	unsigned int dwReserved2;
};

//
// DDSURFACEDESC2 flags that mark the validity of the struct data
//
#define DDSD_CAPS        0x00000001l // default
#define DDSD_HEIGHT      0x00000002l // default
#define DDSD_WIDTH       0x00000004l // default
#define DDSD_PIXELFORMAT 0x00001000l // default
#define DDSD_PITCH       0x00000008l // For uncompressed formats
#define DDSD_MIPMAPCOUNT 0x00020000l
#define DDSD_LINEARSIZE  0x00080000l // For compressed formats
#define DDSD_DEPTH       0x00800000l // Volume Textures

//
// DDPIXELFORMAT flags
//
#define DDPF_ALPHAPIXELS       0x00000001l
#define DDPF_FOURCC            0x00000004l // Compressed formats
#define DDPF_RGB               0x00000040l // Uncompressed formats
#define DDPF_ALPHA             0x00000002l
#define DDPF_COMPRESSED        0x00000080l
#define DDPF_LUMINANCE         0x00020000l
#define DDPF_PALETTEINDEXED4   0x00000008l
#define DDPF_PALETTEINDEXEDTO8 0x00000010l
#define DDPF_PALETTEINDEXED8   0x00000020l

//
// DDSCAPS flags
//
#define DDSCAPS_COMPLEX            0x00000008l
#define DDSCAPS_TEXTURE            0x00001000l // default
#define DDSCAPS_MIPMAP             0x00400000l

#define DDSCAPS2_VOLUME            0x00200000l
#define DDSCAPS2_CUBEMAP           0x00000200L
#define DDSCAPS2_CUBEMAP_POSITIVEX 0x00000400L
#define DDSCAPS2_CUBEMAP_NEGATIVEX 0x00000800L
#define DDSCAPS2_CUBEMAP_POSITIVEY 0x00001000L
#define DDSCAPS2_CUBEMAP_NEGATIVEY 0x00002000L
#define DDSCAPS2_CUBEMAP_POSITIVEZ 0x00004000L
#define DDSCAPS2_CUBEMAP_NEGATIVEZ 0x00008000L

#ifndef MAKEFOURCC

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                                                                                  \
        ((unsigned int)(char)( ch0 ) | ((unsigned int)(char)( ch1 ) << 8 ) |                     \
         ((unsigned int)(char)( ch2 ) << 16 ) | ((unsigned int)(char)( ch3 ) << 24 ))

#endif

#define FOURCC_DDS              MAKEFOURCC( 'D', 'D', 'S', ' ' )

//FOURCC codes for DXTn compressed-texture pixel formats
#define FOURCC_DXT1             MAKEFOURCC( 'D', 'X', 'T', '1' )
#define FOURCC_DXT2             MAKEFOURCC( 'D', 'X', 'T', '2' )
#define FOURCC_DXT3             MAKEFOURCC( 'D', 'X', 'T', '3' )
#define FOURCC_DXT4             MAKEFOURCC( 'D', 'X', 'T', '4' )
#define FOURCC_DXT5             MAKEFOURCC( 'D', 'X', 'T', '5' )
#define FOURCC_ATI1             MAKEFOURCC( 'A', 'T', 'I', '1' )
#define FOURCC_ATI2             MAKEFOURCC( 'A', 'T', 'I', '2' )
#define FOURCC_BC4U             MAKEFOURCC( 'B', 'C', '4', 'U' )
#define FOURCC_BC5U             MAKEFOURCC( 'B', 'C', '5', 'U' )

void R_LoadDDSImageData( void *pImageData, const char *name, byte **data,
			 int *width, int *height, int *numLayers,
			 int *numMips, int *bits )
{
	byte         *buff;
	DDSHEADER_t  *ddsd; //used to get at the dds header in the read buffer

	//mip count and pointers to image data for each mip
	//level, idx 0 = top level last pointer does not start
	//a mip level, it's just there to mark off the end of
	//the final mip data segment (thus the odd + 1)
	//
	//for cube textures we only find the offsets into the
	//first face of the cube, subsequent faces will use the
	//same offsets, just shifted over
	int      i, size;
	bool compressed;

	*numLayers = 0;

	buff = ( byte * ) pImageData;

	data[0] = nullptr;

	if ( strncmp( ( const char * ) buff, "DDS ", 4 ) != 0 )
	{
		ri.Printf( PRINT_WARNING, "R_LoadDDSImage: invalid dds header \"%s\"\n", name );
		return;
	}

	ddsd = ( DDSHEADER_t * )( buff + 4 );

	//Byte Swapping for the DDS headers.
	//beware: we ignore some of the shorts.
#ifdef Q3_BIG_ENDIAN
	{
		int i;
		int *field;

		for ( i = 0; i < sizeof( DDSHEADER_t ) / sizeof( int ); i++ )
		{
			field = ( int * ) ddsd;
			field[ i ] = LittleLong( field[ i ] );
		}
	}
#endif

	if ( ddsd->dwSize != sizeof( DDSHEADER_t ) || ddsd->ddpfPixelFormat.dwSize != sizeof( DDS_PIXELFORMAT_t ) )
	{
		ri.Printf( PRINT_WARNING, "R_LoadDDSImage: invalid dds header \"%s\"\n", name );
		return;
	}

	*numMips = ( ( ddsd->dwFlags & DDSD_MIPMAPCOUNT ) && ( ddsd->dwMipMapCount > 1 ) ) ? ddsd->dwMipMapCount : 1;

	if ( *numMips > MAX_TEXTURE_MIPS )
	{
		ri.Printf( PRINT_WARNING, "R_LoadDDSImage: dds image has too many mip levels \"%s\"\n", name );
		return;
	}

	compressed = ( ddsd->ddpfPixelFormat.dwFlags & DDPF_FOURCC ) ? true : false;

	// either a cube or a volume
	if ( ddsd->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP )
	{
		// cube texture

		if ( ddsd->dwWidth != ddsd->dwHeight )
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: invalid dds image \"%s\"\n", name );
			return;
		}

		*width = ddsd->dwWidth;
		*height = ddsd->dwHeight;
		*numLayers = 6;

		if ( *width & ( *width - 1 ) )
		{
			//cubes must be a power of two
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: cube images must be power of two \"%s\"\n", name );
			return;
		}
	}
	else if ( ( ddsd->ddsCaps.dwCaps2 & DDSCAPS2_VOLUME ) && ( ddsd->dwFlags & DDSD_DEPTH ) )
	{
		// 3D texture

		*width = ddsd->dwWidth;
		*height = ddsd->dwHeight;
		*numLayers = ddsd->dwDepth;

		if ( *numLayers > MAX_TEXTURE_LAYERS )
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: dds image has too many layers \"%s\"\n", name );
			return;
		}

		if ( *width & ( *width - 1 ) || *height & ( *height - 1 ) || *numLayers & ( *numLayers - 1 ) )
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: volume images must be power of two \"%s\"\n", name );
			return;
		}
	}
	else
	{
		// 2D texture

		*width = ddsd->dwWidth;
		*height = ddsd->dwHeight;
		*numLayers = 0;

		//these are allowed to be non power of two, will be dealt with later on
		//except for compressed images!
		if ( compressed && ( *width & ( *width - 1 ) || *height & ( *height - 1 ) ) )
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: compressed texture images must be power of two \"%s\"\n", name );
			return;
		}
	}

	if ( compressed )
	{
		int w, h, blockSize;

		if ( *numLayers != 0 )
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: compressed volume textures are not supported \"%s\"\n", name );
			return;
		}

		//compressed FOURCC formats
		switch ( ddsd->ddpfPixelFormat.dwFourCC )
		{
		case FOURCC_DXT1:
			*bits |= IF_BC1;
			blockSize = 8;
			break;

		case FOURCC_DXT5:
			*bits |= IF_BC3;
			blockSize = 16;
			break;

		case FOURCC_ATI1:
		case FOURCC_BC4U:
			*bits |= IF_BC4;
			blockSize = 8;
			break;

		case FOURCC_ATI2:
		case FOURCC_BC5U:
			*bits |= IF_BC5;
			blockSize = 16;
			break;

		default:
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: unsupported FOURCC 0x%08x, \"%s\"\n",
				   ddsd->ddpfPixelFormat.dwFourCC, name );
			return;
		}
		w = *width;
		h = *height;
		size = 0;

		for( i = 0; i < *numMips; i++ ) {
			int mipSize = ((w + 3) >> 2) * ((h + 3) >> 2) * blockSize;

			data[ i ] = (byte *)(intptr_t)size;
			size += mipSize;
			if( w > 1 ) w >>= 1;
			if( h > 1 ) h >>= 1;
		}
	}
	else
	{
		// uncompressed format
		if ( ddsd->ddpfPixelFormat.dwFlags & DDPF_RGB )
		{
			switch ( ddsd->ddpfPixelFormat.dwRGBBitCount )
			{
				case 32:
					size = 4 * *width * *height;
					break;

				default:
					ri.Printf( PRINT_WARNING, "R_LoadDDSImage: unsupported RGB bit depth \"%s\"\n", name );
					return;
			}
		}
		else
		{
			ri.Printf( PRINT_WARNING, "R_LoadDDSImage: unsupported DDS image type \"%s\"\n", name );
			return;
		}
	}

	data[0] = (byte*) ri.Z_Malloc( size );
	Com_Memcpy( data[0], ddsd + 1, size );

	if( compressed ) {
		for( i = 1; i < *numMips; i++ ) {
			data[ i ] = (intptr_t)data[ i ] + data[ 0 ];
		}
	}
}

void LoadDDS( const char *name, byte **data, int *width, int *height,
	      int *numLayers, int *numMips, int *bits, byte )
{
	byte    *buff;

	ri.FS_ReadFile( name, ( void ** ) &buff );

	if ( !buff )
	{
		return;
	}

	R_LoadDDSImageData( buff, name, data, width, height,
			    numLayers, numMips, bits );

	ri.FS_FreeFile( buff );
}
