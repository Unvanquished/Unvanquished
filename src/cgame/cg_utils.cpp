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

// cg_utils.c -- utility functions

#include "cg_local.h"

/*
===============
CG_ParseColor
===============
*/
bool CG_ParseColor( byte *c, const char **text_p )
{
	char *token;
	int  i;

	for ( i = 0; i <= 2; i++ )
	{
		token = COM_Parse( text_p );

		if ( !*token )
		{
			return false;
		}

		c[ i ] = ( int )( ( float ) 0xFF * atof_neg( token, false ) );
	}

	token = COM_Parse( text_p );

	if ( strcmp( token, "}" ) )
	{
		Log::Warn( "missing '}'" );
		return false;
	}

	return true;
}

/*
==========================
CG_GetShaderNameFromHandle

Gets the name of a shader from the qhandle_t
==========================
*/

const char *CG_GetShaderNameFromHandle( const qhandle_t shader )
{
	static char out[ MAX_STRING_CHARS ];

	trap_R_GetShaderNameFromHandle( shader, out, sizeof( out ) );
	return out;
}

//
// UI Helper Function
//

Color::Color UI_GetChatColour( int which, int team )
{

	switch ( which )
	{
		default:
			return Color::White;

		case SAY_ALL:
			return Color::Green;

		case SAY_TEAM:
			switch ( team )
			{
				case TEAM_NONE:
					return Color::Yellow;
				default:
					return Color::Cyan;
			}

		case SAY_PRIVMSG:
			return Color::Green;

		case SAY_TPRIVMSG:
			return Color::Cyan;

		case SAY_AREA:
		case SAY_AREA_TEAM:
			return Color::Blue;

		case SAY_ADMINS:
		case SAY_ADMINS_PUBLIC:
		case SAY_ALL_ADMIN:
			return Color::Magenta;
			#ifdef UIGPP
		case SAY_RAW:
			return Color::LtGrey;
			#endif
	}
}

void CG_ReadableSize( char *buf, int bufsize, int value )
{
	if ( value > 1024 * 1024 * 1024 )
	{
		// gigs
		Com_sprintf( buf, bufsize, "%d", value / ( 1024 * 1024 * 1024 ) );
		Com_sprintf( buf + strlen( buf ), bufsize - strlen( buf ), ".%02d GB",
			     ( value % ( 1024 * 1024 * 1024 ) ) * 100 / ( 1024 * 1024 * 1024 ) );
	}
	else if ( value > 1024 * 1024 )
	{
		// megs
		Com_sprintf( buf, bufsize, "%d", value / ( 1024 * 1024 ) );
		Com_sprintf( buf + strlen( buf ), bufsize - strlen( buf ), ".%02d MB",
			     ( value % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
	}
	else if ( value > 1024 )
	{
		// kilos
		Com_sprintf( buf, bufsize, "%d KB", value / 1024 );
	}
	else
	{
		// bytes
		Com_sprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
void CG_PrintTime( char *buf, int bufsize, int time )
{
	time /= 1000; // change to seconds

	if ( time > 3600 )
	{
		// in the hours range
		Com_sprintf( buf, bufsize, "%d hr %d min", time / 3600, ( time % 3600 ) / 60 );
	}
	else if ( time > 60 )
	{
		// mins
		Com_sprintf( buf, bufsize, "%d min %d sec", time / 60, time % 60 );
	}
	else
	{
		// secs
		Com_sprintf( buf, bufsize, "%d sec", time );
	}
}

void CG_SetKeyCatcher( int catcher )
{
	rocketInfo.keyCatcher = catcher;
	Rocket_SetActiveContext( catcher );
	trap_Key_SetCatcher( catcher );
}
