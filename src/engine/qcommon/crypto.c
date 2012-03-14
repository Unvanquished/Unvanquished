/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2007-2008 Amanieu d'Antras (amanieu@gmail.com)
Copyright (C) 2012 Dusan Jocic <dusanjocic@msn.com>

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

// Public-key identification

#include "q_shared.h"
#include "qcommon.h"
#include "crypto.h"

#ifdef USE_CRYPTO

void *gmp_alloc( size_t size )
{
	return Z_TagMalloc( size, TAG_CRYPTO );
}

void *gmp_realloc( void *ptr, size_t old_size, size_t new_size )
{
	void *new_ptr = Z_TagMalloc( new_size, TAG_CRYPTO );

	if ( old_size >= new_size )
	{
		return ptr;
	}

	if ( ptr )
	{
		Com_Memcpy( new_ptr, ptr, old_size );
		Z_Free( ptr );
	}

	return new_ptr;
}

void gmp_free( void *ptr, size_t size )
{
	Z_Free( ptr );
}

void *nettle_realloc( void *ctx, void *ptr, unsigned size )
{
	void *new_ptr = gmp_realloc( ptr, *( int * )ctx, size );
	// new size will always be > old size
	*( int * )ctx = size;
	return new_ptr;
}

void qnettle_random( void *ctx, unsigned length, uint8_t *dst )
{
	Com_RandomBytes( dst, length );
}

void qnettle_buffer_init( struct nettle_buffer *buffer, int *size )
{
	nettle_buffer_init_realloc( buffer, size, nettle_realloc );
}

qboolean Crypto_Init( void )
{
	mp_set_memory_functions( gmp_alloc, gmp_realloc, gmp_free );
	return qtrue;
}

void Crypto_Shutdown( void )
{
	Z_FreeTags( TAG_CRYPTO );
}

#endif /* USE_CRYPTO */
