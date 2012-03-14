/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

// g_config.c: Config/Mode settings
// --------------------------------
//
#include "g_local.h"

#define M_FFA  0x01
#define M_1V1  0x02
#define M_SP   0x04
#define M_TEAM 0x08
#define M_CTF  0x10
#define M_WOLF 0x20
#define M_WSW  0x40
#define M_WCP  0x80
#define M_WCPH 0x100

#define M_ALL  M_FFA | M_1V1 | M_SP | M_SP | M_TEAM | M_CTF | M_WOLF | M_WSW | M_WCP | M_WCPH

typedef struct
{
	int  modes;
	char *cvar_name;
	char *cvar_value;
} modeCvarTable_t;

static const modeCvarTable_t aCompSettings[] =
{
	{ M_ALL, "g_altStopwatchMode",     "0"  },
	{ M_ALL, "g_complaintlimit",       "0"  },
	{ M_ALL, "g_ipcomplaintlimit",     "0"  },
	{ M_ALL, "g_doWarmup",             "1"  },
	{ M_ALL, "g_inactivity",           "0"  },
	{ M_ALL, "g_maxlives",             "0"  },
	{ M_ALL, "g_teamforcebalance",     "0"  },
	{ M_ALL, "g_lms_teamForceBalance", "0"  },
	{ M_ALL, "g_voicechatsallowed",    "50" },
	{ M_ALL, "g_warmup",               "10" },
	{ M_ALL, "match_latejoin",         "0"  },
	{ M_ALL, "match_mutespecs",        "0"  },
	{ M_ALL, "match_timeoutcount",     "1"  },
	{ M_ALL, "match_warmupDamage",     "1"  },
	{ M_ALL, "team_maxplayers",        "10" },
	{ M_ALL, "team_nocontrols",        "0"  },
	{ M_ALL, "sv_allowDownload",       "1"  },
	{ M_ALL, "sv_floodProtect",        "0"  },
	{ M_ALL, "sv_fps",                 "20" },
	{ M_ALL, "vote_limit",             "10" },
	{ 0,     NULL,                     NULL } //end of list
};

static const modeCvarTable_t aPubSettings[] =
{
	{ M_ALL, "g_altStopwatchMode",     "0"  },
	{ M_ALL, "g_complaintlimit",       "6"  },
	{ M_ALL, "g_ipcomplaintlimit",     "3"  },
	{ M_ALL, "g_doWarmup",             "0"  },
	{ M_ALL, "g_maxlives",             "0"  },
	{ M_ALL, "g_teamforcebalance",     "1"  },
	{ M_ALL, "g_lms_teamForceBalance", "1"  },
	{ M_ALL, "g_voicechatsallowed",    "50" },
	{ M_ALL, "g_warmup",               "60" },
	{ M_ALL, "match_latejoin",         "1"  },
	{ M_ALL, "match_mutespecs",        "1"  },
	{ M_ALL, "match_timeoutcount",     "0"  },
	{ M_ALL, "match_warmupDamage",     "0"  },
	{ M_ALL, "team_maxplayers",        "0"  },
	{ M_ALL, "team_nocontrols",        "1"  },
	{ M_ALL, "sv_allowDownload",       "1"  },
	{ M_ALL, "sv_floodProtect",        "0"  },
	{ M_ALL, "sv_fps",                 "20" },
	{ M_ALL, "vote_limit",             "5"  },
	{ 0,     NULL,                     NULL } //end of list
};

// Force settings to predefined state.
void G_configSet ( int dwMode, qboolean doComp )
{
	unsigned int          dwGameType;
	const modeCvarTable_t *pModeCvars;

	if ( dwMode < 0 || dwMode >= GT_MAX_GAME_TYPE )
	{
		return;
	}

	dwGameType = 1 << dwMode;

	G_wipeCvars();

	for ( pModeCvars = ( ( doComp ) ? aCompSettings : aPubSettings ); pModeCvars->cvar_name; pModeCvars++ )
	{
		if ( pModeCvars->modes & dwGameType )
		{
			trap_Cvar_Set ( pModeCvars->cvar_name, pModeCvars->cvar_value );
			G_Printf ( "set %s %s\n", pModeCvars->cvar_name, pModeCvars->cvar_value );
		}
	}

	G_UpdateCvars();
	G_Printf ( ">> %s settings loaded!\n", ( doComp ) ? "Competition" : "Public" );

	if ( doComp && g_gamestate.integer == GS_WARMUP_COUNTDOWN )
	{
		level.lastRestartTime = level.time;
		trap_SendConsoleCommand ( EXEC_APPEND, va ( "map_restart 0 %i\n", GS_WARMUP ) );
	}
	else
	{
		trap_SendConsoleCommand ( EXEC_APPEND, va ( "map_restart 0 %i\n", GS_WARMUP ) );
	}
}
