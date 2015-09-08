/*
===========================================================================
Copyright (C) 2011 Matthias Bentrup <matthias.bentrup@googlemail.com>

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
#include <webp/decode.h>

/*
=========================================================

LoadWEBP

=========================================================
*/

void LoadWEBP( const char *filename, unsigned char **pic, int *width, int *height,
	       int*, int*, int*, byte )
{
	byte *out;
	int  len;
	int  stride;
	int  size;
	union
	{
		byte *b;
		void *v;
	} fbuffer;

	/* read compressed data */
	len = ri.FS_ReadFile( ( char * ) filename, &fbuffer.v );

	if ( !fbuffer.b || len < 0 )
	{
		return;
	}

	/* validate data and query image size */
	if ( !WebPGetInfo( fbuffer.b, len, width, height ) )
	{
		ri.FS_FreeFile( fbuffer.v );
		return;
	}

	stride = *width * sizeof( u8vec4_t );
	size = *height * stride;

	out = (byte*) ri.Z_Malloc( size );

	if ( !WebPDecodeRGBAInto( fbuffer.b, len, out, size, stride ) )
	{
		ri.Free( out );
		return;
	}

	ri.FS_FreeFile( fbuffer.v );
	*pic = out;
}
