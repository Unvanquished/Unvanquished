/*
===========================================================================
Copyright (C) 2014 

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

struct KTX_header_t {
	byte     identifier[12];
	uint32_t endianness;
	uint32_t glType;
	uint32_t glTypeSize;
	uint32_t glFormat;
	uint32_t glInternalFormat;
	uint32_t glBaseInternalFormat;
	uint32_t pixelWidth;
	uint32_t pixelHeight;
	uint32_t pixelDepth;
	uint32_t numberOfArrayElements;
	uint32_t numberOfFaces;
	uint32_t numberOfMipmapLevels;
	uint32_t bytesOfKeyValueData;
};

static const byte KTX_identifier[12] = {
	0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};
static const uint32_t KTX_endianness = 0x04030201;
static const uint32_t KTX_endianness_reverse = 0x01020304;

void LoadKTX( const char *name, byte **data, int *width, int *height,
	      int *numLayers, int *numMips, int *bits, byte )
{
	KTX_header_t *hdr;
	byte         *ptr;
	size_t        bufLen, size;
	uint32_t      imageSize;

	*numLayers = 0;

	bufLen = ri.FS_ReadFile( name, ( void ** ) &hdr );

	if( !hdr )
	{
		return;
	}

	if( bufLen < sizeof( KTX_header_t ) ||
	    memcmp( hdr->identifier, KTX_identifier, sizeof( KTX_identifier) ) ) {
		ri.FS_FreeFile( hdr );
		return;
	}

	switch( hdr->endianness ) {
	case KTX_endianness:
		break;

	case KTX_endianness_reverse:
		hdr->glType                = Swap32( hdr->glType );
		hdr->glTypeSize            = Swap32( hdr->glTypeSize );
		hdr->glFormat              = Swap32( hdr->glFormat );
		hdr->glInternalFormat      = Swap32( hdr->glInternalFormat );
		hdr->glBaseInternalFormat  = Swap32( hdr->glBaseInternalFormat );
		hdr->pixelWidth            = Swap32( hdr->pixelWidth );
		hdr->pixelHeight           = Swap32( hdr->pixelHeight );
		hdr->pixelDepth            = Swap32( hdr->pixelDepth );
		hdr->numberOfArrayElements = Swap32( hdr->numberOfArrayElements );
		hdr->numberOfFaces         = Swap32( hdr->numberOfFaces );
		hdr->numberOfMipmapLevels  = Swap32( hdr->numberOfMipmapLevels );
		hdr->bytesOfKeyValueData   = Swap32( hdr->bytesOfKeyValueData );
		break;

	default:
		ri.FS_FreeFile( hdr );
		return;
	}

	// Support only for RGBA8 and BCn
	switch( hdr->glInternalFormat ) {
	case GL_RGBA8:
		break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		*bits |= IF_BC1;
		break;
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		*bits |= IF_BC3;
		break;
	case GL_COMPRESSED_RED_RGTC1:
		*bits |= IF_BC4;
		break;
	case GL_COMPRESSED_RG_RGTC2:
		*bits |= IF_BC5;
		break;
	default:
		ri.FS_FreeFile( hdr );
		return;
	}

	if( hdr->numberOfArrayElements != 0 ||
	    (hdr->numberOfFaces != 1 && hdr->numberOfFaces != 6) ||
	    hdr->pixelWidth == 0 ||
	    hdr->pixelHeight == 0 ||
	    hdr->pixelDepth != 0 ) {
		ri.FS_FreeFile( hdr );
		return;
	}

	*width = hdr->pixelWidth;
	*height = hdr->pixelHeight;
	*numMips = hdr->numberOfMipmapLevels;
	*numLayers = hdr->numberOfFaces == 6 ? 6 : 0;

	ptr = (byte *)(hdr + 1);
	ptr += hdr->bytesOfKeyValueData;
	size = 0;
	for(unsigned i = 0; i < hdr->numberOfMipmapLevels; i++ ) {
		imageSize = *((uint32_t *)ptr);
		if( hdr->endianness == KTX_endianness_reverse )
			imageSize = Swap32( imageSize );
		imageSize = PAD( imageSize, 4 );

		size += imageSize * hdr->numberOfFaces;
		ptr += 4 + imageSize;
	}

	ptr = (byte *)(hdr + 1);
	ptr += hdr->bytesOfKeyValueData;
	data[ 0 ] = (byte *)ri.Z_Malloc( size );

	imageSize = *((uint32_t *)ptr);
	if( hdr->endianness == KTX_endianness_reverse )
		imageSize = Swap32( imageSize );
	imageSize = PAD( imageSize, 4 );
	ptr += 4;
	Com_Memcpy( data[ 0 ], ptr, imageSize );
	ptr += imageSize;
	for(unsigned j = 1; j < hdr->numberOfFaces; j++ ) {
		data[ j ] = data[ j - 1 ] + imageSize;
		Com_Memcpy( data[ j ], ptr, imageSize );
		ptr += imageSize;
	}
	for(unsigned i = 1; i <= hdr->numberOfMipmapLevels; i++ ) {
		imageSize = *((uint32_t *)ptr);
		if( hdr->endianness == KTX_endianness_reverse )
			imageSize = Swap32( imageSize );
		imageSize = PAD( imageSize, 4 );
		ptr += 4;

		for(unsigned j = 0; j < hdr->numberOfFaces; j++ ) {
			int idx = i * hdr->numberOfFaces + j;
			data[ idx ] = data[ idx - 1 ] + imageSize;
			Com_Memcpy( data[ idx ], ptr, imageSize );
			ptr += imageSize;
		}
	}

	ri.FS_FreeFile( hdr );
}

void SaveImageKTX( const char *path, image_t *img )
{
	KTX_header_t hdr;
	int          size, components;
	int          mipWidth, mipHeight, mipDepth, mipSize;
	GLenum       target;
	byte        *data, *ptr;

	Com_Memcpy( &hdr.identifier, KTX_identifier, sizeof( KTX_identifier ) );
	hdr.endianness = KTX_endianness;

	GL_Bind( img );
	if( img->type == GL_TEXTURE_CUBE_MAP ) {
		hdr.numberOfFaces = 6;
		target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
	} else {
		hdr.numberOfFaces = 1;
		target = img->type;
	}

	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_INTERNAL_FORMAT,
				  (GLint *)&hdr.glInternalFormat );
	switch( hdr.glInternalFormat ) {
	case GL_ALPHA4:
	case GL_ALPHA8:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_ALPHA;
		hdr.glBaseInternalFormat = GL_ALPHA;
		components               = 1;
		break;
	case GL_ALPHA12:
	case GL_ALPHA16:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_ALPHA;
		hdr.glBaseInternalFormat = GL_ALPHA;
		components               = 1;
		break;
	case GL_R8:
	case GL_R8_SNORM:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RED;
		hdr.glBaseInternalFormat = GL_RED;
		components               = 1;
		break;
	case GL_R16:
	case GL_R16_SNORM:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RED;
		hdr.glBaseInternalFormat = GL_RED;
		components               = 1;
		break;
	case GL_RG8:
	case GL_RG8_SNORM:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RG;
		hdr.glBaseInternalFormat = GL_RG;
		components               = 2;
		break;
	case GL_RG16:
	case GL_RG16_SNORM:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RG;
		hdr.glBaseInternalFormat = GL_RG;
		components               = 2;
		break;
	case GL_R3_G3_B2:
		hdr.glType               = GL_UNSIGNED_BYTE_3_3_2;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RGB;
		hdr.glBaseInternalFormat = GL_RGB;
		components               = 1;
		break;
	case GL_RGB4:
	case GL_RGB5:
#ifdef GL_RGB565
	case GL_RGB565:
#endif
		hdr.glType               = GL_UNSIGNED_SHORT_5_6_5;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RGB;
		hdr.glBaseInternalFormat = GL_RGB;
		components               = 1;
		break;
	case GL_RGB8:
	case GL_RGB8_SNORM:
	case GL_SRGB8:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RGBA; //GL_RGB;
		hdr.glBaseInternalFormat = GL_RGBA; //GL_RGB;
		components               = 4;       //3;
		break;
	case GL_RGB10:
	case GL_RGB12:
	case GL_RGB16:
	case GL_RGB16_SNORM:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RGB;
		hdr.glBaseInternalFormat = GL_RGB;
		components               = 3;
		break;
	case GL_RGBA2:
	case GL_RGBA4:
		hdr.glType               = GL_UNSIGNED_SHORT_4_4_4_4;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RGBA;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 4;
		break;
	case GL_RGB5_A1:
		hdr.glType               = GL_UNSIGNED_SHORT_5_5_5_1;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RGBA;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 1;
		break;
	case GL_RGBA8:
	case GL_RGBA8_SNORM:
	case GL_SRGB8_ALPHA8:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RGBA;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 4;
		break;
	case GL_RGB10_A2:
		hdr.glType               = GL_UNSIGNED_INT_10_10_10_2;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RGBA;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 1;
		break;
	case GL_RGB10_A2UI:
		hdr.glType               = GL_UNSIGNED_INT_10_10_10_2;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RGBA_INTEGER;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 1;
		break;
	case GL_RGBA12:
	case GL_RGBA16:
	case GL_RGBA16_SNORM:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RGBA;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 4;
		break;
	case GL_R16F:
	case GL_R32F:
		hdr.glType               = GL_FLOAT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RED;
		hdr.glBaseInternalFormat = GL_RED;
		components               = 1;
		break;
	case GL_RG16F:
	case GL_RG32F:
		hdr.glType               = GL_FLOAT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RG;
		hdr.glBaseInternalFormat = GL_RG;
		components               = 2;
		break;
	case GL_RGB16F:
	case GL_RGB32F:
	case GL_R11F_G11F_B10F:
	case GL_RGB9_E5:
		hdr.glType               = GL_FLOAT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RGB;
		hdr.glBaseInternalFormat = GL_RGB;
		components               = 3;
		break;
	case GL_RGBA16F:
	case GL_RGBA32F:
		hdr.glType               = GL_FLOAT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RGBA;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 4;
		break;
	case GL_R8I:
		hdr.glType               = GL_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RED_INTEGER;
		hdr.glBaseInternalFormat = GL_RED;
		components               = 1;
		break;
	case GL_R8UI:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RED_INTEGER;
		hdr.glBaseInternalFormat = GL_RED;
		components               = 1;
		break;
	case GL_R16I:
		hdr.glType               = GL_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RED_INTEGER;
		hdr.glBaseInternalFormat = GL_RED;
		components               = 1;
		break;
	case GL_R16UI:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RED_INTEGER;
		hdr.glBaseInternalFormat = GL_RED;
		components               = 1;
		break;
	case GL_R32I:
		hdr.glType               = GL_INT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RED_INTEGER;
		hdr.glBaseInternalFormat = GL_RED;
		components               = 1;
		break;
	case GL_R32UI:
		hdr.glType               = GL_UNSIGNED_INT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RED_INTEGER;
		hdr.glBaseInternalFormat = GL_RED;
		components               = 1;
		break;
	case GL_RG8I:
		hdr.glType               = GL_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RG_INTEGER;
		hdr.glBaseInternalFormat = GL_RG;
		components               = 2;
		break;
	case GL_RG8UI:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RG_INTEGER;
		hdr.glBaseInternalFormat = GL_RG;
		components               = 2;
		break;
	case GL_RG16I:
		hdr.glType               = GL_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RG_INTEGER;
		hdr.glBaseInternalFormat = GL_RG;
		components               = 2;
		break;
	case GL_RG16UI:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RG_INTEGER;
		hdr.glBaseInternalFormat = GL_RG;
		components               = 2;
		break;
	case GL_RG32I:
		hdr.glType               = GL_INT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RG_INTEGER;
		hdr.glBaseInternalFormat = GL_RG;
		components               = 2;
		break;
	case GL_RG32UI:
		hdr.glType               = GL_UNSIGNED_INT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RG_INTEGER;
		hdr.glBaseInternalFormat = GL_RG;
		components               = 2;
		break;
	case GL_RGB8I:
		hdr.glType               = GL_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RGBA_INTEGER; //GL_RGB_INTEGER;
		hdr.glBaseInternalFormat = GL_RGBA;         //GL_RGB;
		components               = 4;               //3;
		break;
	case GL_RGB8UI:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RGBA_INTEGER; //GL_RGB_INTEGER;
		hdr.glBaseInternalFormat = GL_RGBA;         //GL_RGB;
		components               = 4;               //3;
		break;
	case GL_RGB16I:
		hdr.glType               = GL_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RGB_INTEGER;
		hdr.glBaseInternalFormat = GL_RGB;
		components               = 3;
		break;
	case GL_RGB16UI:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RGB_INTEGER;
		hdr.glBaseInternalFormat = GL_RGB;
		components               = 3;
		break;
	case GL_RGB32I:
		hdr.glType               = GL_INT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RGB_INTEGER;
		hdr.glBaseInternalFormat = GL_RGB;
		components               = 3;
		break;
	case GL_RGB32UI:
		hdr.glType               = GL_UNSIGNED_INT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RGB_INTEGER;
		hdr.glBaseInternalFormat = GL_RGB;
		components               = 3;
		break;
	case GL_RGBA8I:
		hdr.glType               = GL_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RGBA_INTEGER;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 4;
		break;
	case GL_RGBA8UI:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_RGBA_INTEGER;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 4;
		break;
	case GL_RGBA16I:
		hdr.glType               = GL_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RGBA_INTEGER;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 4;
		break;
	case GL_RGBA16UI:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_RGBA_INTEGER;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 4;
		break;
	case GL_RGBA32I:
		hdr.glType               = GL_INT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RGBA_INTEGER;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 4;
		break;
	case GL_RGBA32UI:
		hdr.glType               = GL_UNSIGNED_INT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_RGBA_INTEGER;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 4;
		break;
	case GL_LUMINANCE4:
	case GL_LUMINANCE8:
	case GL_SLUMINANCE:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_LUMINANCE;
		hdr.glBaseInternalFormat = GL_LUMINANCE;
		components               = 1;
		break;
	case GL_LUMINANCE12:
	case GL_LUMINANCE16:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_LUMINANCE;
		hdr.glBaseInternalFormat = GL_LUMINANCE;
		components               = 1;
		break;
	case GL_LUMINANCE4_ALPHA4:
	case GL_LUMINANCE6_ALPHA2:
	case GL_LUMINANCE8_ALPHA8:
	case GL_SLUMINANCE8_ALPHA8:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_LUMINANCE_ALPHA;
		hdr.glBaseInternalFormat = GL_LUMINANCE_ALPHA;
		components               = 2;
		break;
	case GL_LUMINANCE12_ALPHA4:
	case GL_LUMINANCE12_ALPHA12:
	case GL_LUMINANCE16_ALPHA16:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_LUMINANCE_ALPHA;
		hdr.glBaseInternalFormat = GL_LUMINANCE_ALPHA;
		components               = 2;
		break;
	case GL_INTENSITY4:
	case GL_INTENSITY8:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_INTENSITY;
		hdr.glBaseInternalFormat = GL_INTENSITY;
		components               = 1;
		break;
	case GL_INTENSITY12:
	case GL_INTENSITY16:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_INTENSITY;
		hdr.glBaseInternalFormat = GL_INTENSITY;
		components               = 1;
		break;
	case GL_DEPTH_COMPONENT16:
		hdr.glType               = GL_UNSIGNED_SHORT;
		hdr.glTypeSize           = 2;
		hdr.glFormat             = GL_DEPTH_COMPONENT;
		hdr.glBaseInternalFormat = GL_DEPTH_COMPONENT;
		components               = 1;
		break;
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
		hdr.glType               = GL_UNSIGNED_INT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_DEPTH_COMPONENT;
		hdr.glBaseInternalFormat = GL_DEPTH_COMPONENT;
		components               = 1;
		break;
	case GL_DEPTH_COMPONENT32F:
		hdr.glType               = GL_FLOAT;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_DEPTH_COMPONENT;
		hdr.glBaseInternalFormat = GL_DEPTH_COMPONENT;
		components               = 1;
		break;
	case GL_DEPTH24_STENCIL8:
		hdr.glType               = GL_UNSIGNED_INT_24_8;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_DEPTH_STENCIL;
		hdr.glBaseInternalFormat = GL_DEPTH_STENCIL;
		components               = 1;
		break;
	case GL_DEPTH32F_STENCIL8:
		hdr.glType               = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
		hdr.glTypeSize           = 4;
		hdr.glFormat             = GL_DEPTH_STENCIL;
		hdr.glBaseInternalFormat = GL_DEPTH_STENCIL;
		components               = 1;
		break;
	case GL_STENCIL_INDEX8:
		hdr.glType               = GL_UNSIGNED_BYTE;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = GL_STENCIL_INDEX;
		hdr.glBaseInternalFormat = GL_STENCIL_INDEX;
		components               = 1;
		break;
	case GL_COMPRESSED_RED_RGTC1:
	case GL_COMPRESSED_SIGNED_RED_RGTC1:
		hdr.glType               = 0;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = 0;
		hdr.glBaseInternalFormat = GL_RED;
		components               = 0;
		break;
	case GL_COMPRESSED_RG_RGTC2:
	case GL_COMPRESSED_SIGNED_RG_RGTC2:
		hdr.glType               = 0;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = 0;
		hdr.glBaseInternalFormat = GL_RG;
		components               = 0;
		break;
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		hdr.glType               = 0;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = 0;
		hdr.glBaseInternalFormat = GL_RGB;
		components               = 0;
		break;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		hdr.glType               = 0;
		hdr.glTypeSize           = 1;
		hdr.glFormat             = 0;
		hdr.glBaseInternalFormat = GL_RGBA;
		components               = 0;
		break;
	default:
		ri.Printf(printParm_t::PRINT_WARNING, "Unknown texture format %x\n",
			  hdr.glInternalFormat );
		return;
	}

	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_WIDTH,
				  (GLint *)&hdr.pixelWidth );
	hdr.numberOfArrayElements = 0;
	if( img->type == GL_TEXTURE_CUBE_MAP ) {
		hdr.pixelHeight = hdr.pixelWidth;
		hdr.pixelDepth = 0;
	} else if( img->type == GL_TEXTURE_1D ) {
		hdr.pixelHeight = 0;
		hdr.pixelDepth = 0;
	} else {
		glGetTexLevelParameteriv( target, 0, GL_TEXTURE_HEIGHT,
					  (GLint *)&hdr.pixelHeight );
		if( img->type == GL_TEXTURE_2D ) {
			hdr.pixelDepth = 0;
		} else {
			glGetTexLevelParameteriv( target, 0, GL_TEXTURE_DEPTH,
						  (GLint *)&hdr.pixelDepth );
		}
	}

	hdr.numberOfMipmapLevels = 1;
    int i;
	glGetTexParameteriv( target, GL_TEXTURE_MIN_FILTER,
			     (GLint *)&i );
	if( i == GL_NEAREST_MIPMAP_NEAREST || i == GL_NEAREST_MIPMAP_LINEAR ||
	    i == GL_LINEAR_MIPMAP_NEAREST || i == GL_LINEAR_MIPMAP_LINEAR ) {
		
		mipWidth = std::max(hdr.pixelWidth, 1u);
		mipHeight = std::max(hdr.pixelHeight, 1u);
		mipDepth = std::max(hdr.pixelDepth, 1u);

		while( mipWidth > 1 || mipHeight > 1 || mipDepth > 1 ) {
			hdr.numberOfMipmapLevels++;

			if( mipWidth  > 1 ) mipWidth  >>= 1;
			if( mipHeight > 1 ) mipHeight >>= 1;
			if( mipDepth  > 1 ) mipDepth  >>= 1;
		}
	}

	hdr.bytesOfKeyValueData = 0;

	size = 0;
	mipWidth = std::max(hdr.pixelWidth, 1u);
	mipHeight = std::max(hdr.pixelHeight, 1u);
	mipDepth = std::max(hdr.pixelDepth, 1u);
	for(unsigned i = size = 0; i < hdr.numberOfMipmapLevels; i++ ) {
		size += 4;
		if( !hdr.glFormat ) {
			glGetTexLevelParameteriv( target, i,
						  GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
						  &mipSize );
		} else {
			mipSize = mipWidth * hdr.glTypeSize * components;
			mipSize = PAD( mipSize, 4 );
			mipSize *= mipHeight * mipDepth;
		}
		size += hdr.numberOfFaces * PAD( mipSize, 4 );

		if( mipWidth  > 1 ) mipWidth  >>= 1;
		if( mipHeight > 1 ) mipHeight >>= 1;
		if( mipDepth  > 1 ) mipDepth  >>= 1;
	}

	data = (byte *)ri.Hunk_AllocateTempMemory( size + sizeof( hdr ) );

	ptr = data;
	Com_Memcpy( ptr, &hdr, sizeof( hdr ) );
	ptr += sizeof( hdr );
	
	mipWidth = std::max(hdr.pixelWidth, 1u);
	mipHeight = std::max(hdr.pixelHeight, 1u);
	mipDepth = std::max(hdr.pixelDepth, 1u);
	for(unsigned i = 0; i < hdr.numberOfMipmapLevels; i++ ) {
		if( !hdr.glFormat ) {
			glGetTexLevelParameteriv( target, i,
						  GL_TEXTURE_COMPRESSED_IMAGE_SIZE,
						  &mipSize );
		} else {
			mipSize = mipWidth * hdr.glTypeSize * components;
			mipSize = PAD( mipSize, 4 );
			mipSize *= mipHeight * mipDepth;
		}
		*(int32_t *)ptr = PAD( mipSize, 4 );
		ptr += sizeof( int32_t );

		for(unsigned j = 0; j < hdr.numberOfFaces; j++ ) {
			if( !hdr.glFormat ) {
				glGetCompressedTexImage( target + j, i, ptr );
			} else {
				glGetTexImage( target + j, i, hdr.glFormat,
					       hdr.glType, ptr );
			}
			ptr += PAD( mipSize, 4);
		}

		if( mipWidth  > 1 ) mipWidth  >>= 1;
		if( mipHeight > 1 ) mipHeight >>= 1;
		if( mipDepth  > 1 ) mipDepth  >>= 1;
	}

	ri.FS_WriteFile( path, data, size + sizeof( hdr ) );

	ri.Hunk_FreeTempMemory( data );
}
