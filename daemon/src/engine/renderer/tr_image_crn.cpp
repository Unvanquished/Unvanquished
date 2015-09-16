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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-pedantic"
#include "crunch/crn_decomp.h"
#pragma GCC diagnostic pop

void LoadCRN( const char *name, byte **data, int *width, int *height,
	      int *numLayers, int *numMips, int *bits, byte )
{
	byte    *buff;
	size_t  buffLen, size, imageSize;
	unsigned i, j;
	crnd::crn_texture_info ti;
	crnd::crn_level_info li;
	crnd::crnd_unpack_context ctx;

	*numLayers = 0;

	buffLen = ri.FS_ReadFile( name, ( void ** ) &buff );

	if ( !buff )
	{
		return;
	}

	if( !crnd::crnd_get_texture_info( buff, buffLen, &ti ) ||
	    ( ti.m_faces != 1 && ti.m_faces != 6 ) )
	{
		ri.FS_FreeFile( buff );
		return;
	}

	switch( ti.m_format ) {
	case cCRNFmtDXT1:
		*bits |= IF_BC1;
		break;
	case cCRNFmtDXT5:
		*bits |= IF_BC3;
		break;
	case cCRNFmtDXT5A:
		*bits |= IF_BC4;
		break;
	case cCRNFmtDXN_XY:
		*bits |= IF_BC5;
		break;
	default:
		ri.FS_FreeFile( buff );
		return;
	}

	*width = ti.m_width;
	*height = ti.m_height;
	*numMips = ti.m_levels;
	*numLayers = ti.m_faces == 6 ? 6 : 0;

	size = 0;
	for( i = 0; i < ti.m_levels; i++ ) {
		crnd::crnd_get_level_info( buff, buffLen, i, &li );
		imageSize = li.m_blocks_x * li.m_blocks_y * li.m_bytes_per_block;
		for( j = 0; j < ti.m_faces; j++ ) {
			size += imageSize;
			data[ i * ti.m_faces + j + 1 ] = size + (byte *)0;
		}
	}

	data[ 0 ] = (byte *)ri.Z_Malloc( size );
	for(i = 1; i <= ti.m_levels * ti.m_faces; i++ ) {
		data[ i ] = (data[ i ] - (byte *)0) + data[ 0 ];
	}

	ctx = crnd::crnd_unpack_begin( buff, buffLen );
	for( i = 0; i < ti.m_levels; i++ ) {
		crnd::crnd_unpack_level( ctx, (void **)&data[ i * ti.m_faces ],
					 data[ i * ti.m_faces + 1 ] - data[ i * ti.m_faces ], 0, i );
	}
	crnd::crnd_unpack_end( ctx );

	ri.FS_FreeFile( buff );
}
