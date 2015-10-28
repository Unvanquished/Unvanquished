/*
===========================================================================
Copyright (C) 2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_image_exr.c

//#include "tr_local.h"
#include "qcommon/q_shared.h"

//#include <OpenEXR/half.h>
#include "openexr/half.h"

	void LoadRGBEToFloats( const char *name, float **pic, int *width, int *height );
	void LoadRGBEToHalfs( const char *name, unsigned short **halfImage, int *width, int *height )
	{
		int            i, j;
		int            w, h;
		float          *hdrImage;
		float          *floatbuf;
		unsigned short *halfbuf;

		w = h = 0;
		LoadRGBEToFloats( name, &hdrImage, &w, &h );

		*width = w;
		*height = h;

		*halfImage = ( unsigned short * ) Com_Allocate( w * h * 3 * 6 );

		halfbuf = *halfImage;
		floatbuf = hdrImage;

		for ( i = 0; i < ( w * h ); i++ )
		{
			for ( j = 0; j < 3; j++ )
			{
				half sample( *floatbuf++ );
				*halfbuf++ = sample.bits();
			}
		}

		Com_Dealloc( hdrImage );
	}
