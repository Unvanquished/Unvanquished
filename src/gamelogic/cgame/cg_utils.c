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

// cg_utils.c -- utility functions

#include "cg_local.h"

/*
===============
CG_ParseColor
===============
*/
qboolean CG_ParseColor( byte *c, char **text_p )
{
	char *token;
	int  i;

	for ( i = 0; i <= 2; i++ )
	{
		token = COM_Parse( text_p );

		if ( !*token )
		{
			return qfalse;
		}

		c[ i ] = ( int )( ( float ) 0xFF * atof_neg( token, qfalse ) );
	}

	token = COM_Parse( text_p );

	if ( strcmp( token, "}" ) )
	{
		CG_Printf( S_ERROR "missing '}'\n" );
		return qfalse;
	}

	return qtrue;
}
