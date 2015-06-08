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

#ifndef TR_IMAGE_H
#define TR_IMAGE_H

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */
#include <jpeglib.h>
#include <jerror.h>

#if JPEG_LIB_VERSION < 80
// jpeg-6 deals with 4 byte / pixel, in RGBA order
#define JPEG_PIXEL_SIZE 4
#define JPEG_HAS_ALPHA
// ugly, if it's not jpeg-8, we need the internal stuff too
#include <jpegint.h>
#else
// jpeg-8 uses 3 byte/pixel, in RGB order
#define JPEG_PIXEL_SIZE 3
#endif

#endif
