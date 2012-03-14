/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

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

#ifdef USE_LOCAL_HEADERS
#       include "SDL.h"
#else
#       include <SDL.h>
#endif

#include "../renderer/tr_local.h"
#include "../qcommon/qcommon.h"

#ifdef _WIN32
#include <windows.h>
#endif

static Uint16 OldGammaRamp[ 3 ][ 256 ];

/*
===========
GLimp_InitGamma

Saves old gamma ramp and checks if gamma is supportet by the hardware.
===========
*/
void GLimp_InitGamma( void )
{
	// Get original gamma ramp
	if( SDL_GetGammaRamp( OldGammaRamp[ 0 ], OldGammaRamp[ 1 ], OldGammaRamp[ 2 ] ) == -1 )
	{
		glConfig.deviceSupportsGamma = qfalse;
		return;
	}

	glConfig.deviceSupportsGamma = qtrue;
#ifdef WIN32

	if( !r_ignorehwgamma->integer )
	{
		// do a sanity check on the gamma values
		if( ( HIBYTE( OldGammaRamp[ 0 ][ 255 ] ) <= HIBYTE( OldGammaRamp[ 0 ][ 0 ] ) ) ||
		    ( HIBYTE( OldGammaRamp[ 1 ][ 255 ] ) <= HIBYTE( OldGammaRamp[ 1 ][ 0 ] ) ) ||
		    ( HIBYTE( OldGammaRamp[ 2 ][ 255 ] ) <= HIBYTE( OldGammaRamp[ 2 ][ 0 ] ) ) )
		{
			glConfig.deviceSupportsGamma = qfalse;
			ri.Printf( PRINT_WARNING, "WARNING: device has broken gamma support, generated gamma.dat\n" );
		}

		// make sure that we didn't have a prior crash in the game
		// and if so we need to restore the gamma values to at least a linear value
		if( ( HIBYTE( OldGammaRamp[ 0 ][ 181 ] ) == 255 ) )
		{
			int g;

			ri.Printf( PRINT_WARNING, "WARNING: suspicious gamma tables, using linear ramp for restoration\n" );

			for( g = 0; g < 255; g++ )
			{
				OldGammaRamp[ 0 ][ g ] = g << 8;
				OldGammaRamp[ 1 ][ g ] = g << 8;
				OldGammaRamp[ 2 ][ g ] = g << 8;
			}
		}
	}

#endif
}

/*
=================
GLimp_SetGamma
=================
*/
void GLimp_SetGamma( unsigned char red[ 256 ], unsigned char green[ 256 ], unsigned char blue[ 256 ] )
{
#if defined( IPHONE )
	UNIMPL();
#else
#if 1
	Uint16 table[ 256 ];
	int    i, value, lastvalue = 0;

	for( i = 0; i < 256; i++ )
	{
		value = ( ( ( Uint16 ) red[ i ] ) << 8 ) | red[ i ];

		if( i < 128 && ( value > ( ( 128 + i ) << 8 ) ) )
		{
			value = ( 128 + i ) << 8;
		}

		if( i && ( value < lastvalue ) )
		{
			value = lastvalue;
		}

		lastvalue = table[ i ] = value;
	}

	if( SDL_SetGammaRamp( table, table, table ) == -1 )
	{
		Com_Printf( "SDL_SetGammaRamp failed.\n" );
	}

#else
	float g = Cvar_Get( "r_gamma", "1.0", 0 )->value;

	if( SDL_SetGamma( g, g, g ) == -1 )
	{
		Com_Printf( "SDL_SetGamma failed.\n" );
	}

#endif
}

/*
===========
GLimp_RestoreGamma

Restores original gamma ramp
===========
*/
void GLimp_RestoreGamma( void )
{
	if( glConfig.deviceSupportsGamma )
	{
		// Restore original gamma
		if( SDL_SetGammaRamp( OldGammaRamp[ 0 ], OldGammaRamp[ 1 ], OldGammaRamp[ 2 ] ) == -1 )
		{
			Com_Printf( "SDL_SetGammaRamp failed.\n" );
		}
	}
}

#endif //IPHONE
