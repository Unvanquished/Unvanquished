/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../../engine/qcommon/q_shared.h"
#include "bg_public.h"

void *BG_Alloc( int size )
{
	void *ptr = malloc( size );

	if ( ptr )
	{
		memset( ptr, 0, size );
	}

	return ptr;
}

void BG_Free( void *ptr )
{
	free( ptr );
}

void BG_InitMemory( void )
{
}

void BG_DefragmentMemory( void )
{
}

void BG_MemoryInfo( void )
{
}
