/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "cg_local.h"

typedef struct progressBarCmd_s
{
	const char *command;
	float ( *get ) ( void );
} progressBarCmd_t;

static float CG_Rocket_GetBuildableLoadProgress( void )
{
	return cg.buildablesFraction;
}

static float CG_Rocket_GetCharLoadProgress( void )
{
	return cg.charModelFraction;
}

static float CG_Rocket_GetMediaLoadProgress( void )
{
	return cg.mediaFraction;
}

static float CG_Rocket_GetOverallLoadProgress( void )
{
	return ( cg.mediaFraction + cg.charModelFraction + cg.buildablesFraction ) / 3.0f;
}

static float CG_Rocket_GetBuildTimerProgress( void )
{
	static int misc = 0;
	static int max;
	playerState_t *ps = &cg.snap->ps;
	weapon_t weapon = BG_GetPlayerWeapon( ps );

	// Not building anything
	if ( weapon != WP_HBUILD && weapon != WP_ABUILD && weapon != WP_ABUILD2 )
	{
		return 0;
	}

	// Building something new. Note max value.
	if ( ps->stats[ STAT_MISC ] > 0 && misc <= 0 )
	{
		max = ps->stats[ STAT_MISC ];
	}

	misc = ps->stats[ STAT_MISC ];

	return ( float ) misc / ( float ) max;
}

static const progressBarCmd_t progressBarCmdList[] =
{
	{ "buildables", &CG_Rocket_GetBuildableLoadProgress },
	{ "buildTimer", &CG_Rocket_GetBuildTimerProgress },
	{ "characters", &CG_Rocket_GetCharLoadProgress },
	{ "media", &CG_Rocket_GetMediaLoadProgress },
	{ "overall", &CG_Rocket_GetOverallLoadProgress },
};

static const size_t progressBarCmdListCount = ARRAY_LEN( progressBarCmdList );

static int progressBarCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( progressBarCmd_t * ) b )->command );
}

float CG_Rocket_ProgressBarValue( void )
{
	progressBarCmd_t *cmd;

	// Get the progressbar command
	cmd = bsearch( CG_Argv( 0 ), progressBarCmdList, progressBarCmdListCount, sizeof( progressBarCmd_t ), progressBarCmdCmp );

	if ( cmd )
	{
		return cmd->get();
	}

	return 0;
}
