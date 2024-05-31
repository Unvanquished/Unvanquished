/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

#include "common/Common.h"
#include "bg_public.h"

void BG_Free( void *ptr )
{
	free( ptr );
}

// Allocations uncategorized as to whether they really need zeroing
void *BG_Alloc( size_t size )
{
  void* p = calloc( size, 1 );
  if ( !p && size ) Sys::Error( "BG_Alloc: Out of memory" );
  return p;
}
// Allocates unitialized memory like malloc
void *BG_Malloc( size_t size )
{
    void* p = malloc( size );
    if ( !p && size ) Sys::Error( "BG_Malloc: Out of memory" );
    return p;
}
// Allocates zeroed memory like calloc
void *BG_Calloc( size_t size )
{
    void* p = calloc( size, 1 );
    if ( !p && size ) Sys::Error( "BG_Calloc: Out of memory" );
    return p;
}

char *BG_strdup( const char *string )
{
	size_t length = strlen(string) + 1;
	char *copy = static_cast<char*>( BG_Malloc(length) );
	memcpy( copy, string, length );
	return copy;
}
