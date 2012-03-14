/*
===========================================================================

Daemon GPL Source Code

Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2004-2006 Tony J. White
Copyright (C) 2011 Dusan Jocic

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

#ifdef _MSC_VER
#include "../../../../libs/msinttypes/inttypes.h"
#else
#include <inttypes.h>
#endif

#include "g_local.h"

// big ugly global buffer for use with buffered printing of long outputs
static char       g_bfb[ 32000 ];

// note: list ordered alphabetically
g_admin_cmd_t     g_admin_cmds[] =
{
	{
		"adjustban",   G_admin_adjustban,   "b",
		"change the duration or reason of a ban.  duration is specified as "
		"numbers followed by units 'w' (weeks), 'd' (days), 'h' (hours) or "
		"'m' (minutes), or seconds if no units are specified.  if the duration is"
		" preceded by a + or -, the ban duration will be extended or shortened by"
		" the specified amount",
		"[^3ban#^7] (^5duration^7) (^5reason^7)"
	},

	{
		"admintest",   G_admin_admintest,   "a",
		"display your current admin level",
		""
	},

	{
		"ban",         G_admin_ban,         "b",
		"ban a player by IP and GUID with an optional expiration time and reason."
		" duration is specified as numbers followed by units 'w' (weeks), 'd' "
		"(days), 'h' (hours) or 'm' (minutes), or seconds if no units are "
		"specified",
		"[^3name|slot#|IP^7] (^5duration^7) (^5reason^7)"
	},

	{
		"cancelvote",  G_admin_cancelvote,  "c",
		"cancel a vote taking place",
		""
	},

	{
		"help",        G_admin_help,        "h",
		"display commands available to you or help on a specific command",
		"(^5command^7)"
	},

	{
		"kick",        G_admin_kick,        "k",
		"kick a player with an optional reason",
		"(^5reason^7)"
	},

	{
		"listadmins",  G_admin_listadmins,  "D",
		"display a list of all server admins and their levels",
		"(^5name|start admin#^7)"
	},

	{
		"listplayers", G_admin_listplayers, "i",
		"display a list of players, their client numbers and their levels",
		""
	},

	{
		"mute",        G_admin_mute,        "m",
		"mute a player",
		"[^3name|slot#^7]"
	},

	{
		"namelog",     G_admin_namelog,     "e",
		"display a list of names used by recently connected players",
		"(^5name^7)"
	},

	{
		"nextmap",     G_admin_nextmap,     "n",
		"go to the next map in the cycle",
		""
	},

	{
		"passvote",    G_admin_passvote,    "V",
		"pass a vote currently taking place",
		""
	},

	{
		"putteam",     G_admin_putteam,     "p",
		"move a player to a specified team",
		"[^3name|slot#^7] [^3h|a|s^7]"
	},

	{
		"readconfig",  G_admin_readconfig,  "G",
		"reloads the admin config file and refreshes permission flags",
		""
	},

	{
		"register",    G_admin_register,    "R",
		"Registers your name to protect it from being used by others or updates your admin name to your current name.",
		""
	},

	{
		"rename",      G_admin_rename,      "N",
		"rename a player",
		"[^3name|slot#^7] [^3new name^7]"
	},

	{
		"restart",     G_admin_restart,     "r",
		"restart the current map",
		""
	},

	{
		"setlevel",    G_admin_setlevel,    "s",
		"sets the admin level of a player",
		"[^3name|slot#|admin#^7] [^3level^7]"
	},

	{
		"showbans",    G_admin_showbans,    "B",
		"display a (partial) list of active bans",
		"(^5start at ban#^7) (^5name|IP^7)"
	},

	{
		"spec999",     G_admin_spec999,     "P",
		"move 999 pingers to the spectator team",
		""
	},

	{
		"time",        G_admin_time,        "C",
		"show the current local server time",
		""
	},

	{
		"unban",       G_admin_unban,       "b",
		"unbans a player specified by the slot as seen in showbans",
		"[^3ban#^7]"
	},

	{
		"unmute",      G_admin_mute,        "m",
		"unmute a muted player",
		"[^3name|slot#^7]"
	}
};

static int        adminNumCmds        = sizeof( g_admin_cmds ) / sizeof( g_admin_cmds[ 0 ] );

static int        admin_level_maxname = 0;
g_admin_level_t   *g_admin_levels[ MAX_ADMIN_LEVELS ];
g_admin_admin_t   *g_admin_admins[ MAX_ADMIN_ADMINS ];
g_admin_ban_t     *g_admin_bans[ MAX_ADMIN_BANS ];
g_admin_command_t *g_admin_commands[ MAX_ADMIN_COMMANDS ];
g_admin_namelog_t *g_admin_namelog[ MAX_ADMIN_NAMELOGS ];

qboolean G_admin_permission( gentity_t *ent, char flag )
{
	int  i;
	int  l = 0;
	char *flags;

	// console always wins
	if ( !ent )
	{
		return qtrue;
	}

	for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
	{
		if ( !Q_stricmp( ent->client->pers.guid, g_admin_admins[ i ]->guid ) )
		{
			flags = g_admin_admins[ i ]->flags;

			while ( *flags )
			{
				if ( *flags == flag )
				{
					return qtrue;
				}
				else if ( *flags == '-' )
				{
					while ( *flags++ )
					{
						if ( *flags == flag )
						{
							return qfalse;
						}
						else if ( *flags == '+' )
						{
							break;
						}
					}
				}
				else if ( *flags == '*' )
				{
					while ( *flags++ )
					{
						if ( *flags == flag )
						{
							return qfalse;
						}
					}

					// flags with significance only for individuals (
					// like ADMF_INCOGNITO and ADMF_IMMUTABLE are NOT covered
					// by the '*' wildcard.  They must be specified manually.
					switch ( flag )
					{
						case ADMF_INCOGNITO:
						case ADMF_IMMUTABLE:
							return qfalse;

						default:
							return qtrue;
					}
				}

				flags++;
			}

			l = g_admin_admins[ i ]->level;
		}
	}

	for ( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
	{
		if ( g_admin_levels[ i ]->level == l )
		{
			flags = g_admin_levels[ i ]->flags;

			while ( *flags )
			{
				if ( *flags == flag )
				{
					return qtrue;
				}

				if ( *flags == '*' )
				{
					while ( *flags++ )
					{
						if ( *flags == flag )
						{
							return qfalse;
						}
					}

					// flags with significance only for individuals (
					// like ADMF_INCOGNITO and ADMF_IMMUTABLE are NOT covered
					// by the '*' wildcard.  They must be specified manually.
					switch ( flag )
					{
						case ADMF_INCOGNITO:
						case ADMF_IMMUTABLE:
							return qfalse;

						default:
							return qtrue;
					}
				}

				flags++;
			}
		}
	}

	return qfalse;
}

qboolean G_admin_name_check( gentity_t *ent, char *name, char *err, int len )
{
	int       i;
	gclient_t *client;
	char      testName[ MAX_NAME_LENGTH ] = { "" };
	char      name2[ MAX_NAME_LENGTH ]    = { "" };

	G_SanitiseString( name, name2, sizeof( name2 ) );

	if ( !Q_stricmp( name2, "UnnamedPlayer" ) )
	{
		return qtrue;
	}

	for ( i = 0; i < level.maxclients; i++ )
	{
		client = &level.clients[ i ];

		if ( client->pers.connected == CON_DISCONNECTED )
		{
			continue;
		}

		// can rename ones self to the same name using different colors
		if ( i == ( ent - g_entities ) )
		{
			continue;
		}

		G_SanitiseString( client->pers.netname, testName, sizeof( testName ) );

		if ( !Q_stricmp( name2, testName ) )
		{
			Com_sprintf( err, len, "The name '%s^7' is already in use", name );
			return qfalse;
		}
	}

	if ( !g_adminNameProtect.integer )
	{
		return qtrue;
	}

	for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
	{
		if ( g_admin_admins[ i ]->level < 1 )
		{
			continue;
		}

		G_SanitiseString( g_admin_admins[ i ]->name, testName, sizeof( testName ) );

		if ( !Q_stricmp( name2, testName ) &&
		     Q_stricmp( ent->client->pers.guid, g_admin_admins[ i ]->guid ) )
		{
			Com_sprintf( err, len, "The name '%s^7' belongs to an admin, "
			             "please use another name", name );
			return qfalse;
		}
	}

	return qtrue;
}

static qboolean admin_higher_guid( char *admin_guid, char *victim_guid )
{
	int i;
	int alevel = 0;

	for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
	{
		if ( !Q_stricmp( admin_guid, g_admin_admins[ i ]->guid ) )
		{
			alevel = g_admin_admins[ i ]->level;
			break;
		}
	}

	for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
	{
		if ( !Q_stricmp( victim_guid, g_admin_admins[ i ]->guid ) )
		{
			if ( alevel < g_admin_admins[ i ]->level )
			{
				return qfalse;
			}

			return !strstr( g_admin_admins[ i ]->flags, va( "%c", ADMF_IMMUTABLE ) );
		}
	}

	return qtrue;
}

static qboolean admin_higher( gentity_t *admin, gentity_t *victim )
{
	// console always wins
	if ( !admin )
	{
		return qtrue;
	}

	// just in case
	if ( !victim )
	{
		return qtrue;
	}

	return admin_higher_guid( admin->client->pers.guid,
	                          victim->client->pers.guid );
}

static void admin_writeconfig_string( char *s, fileHandle_t f )
{
	char buf[ MAX_STRING_CHARS ];

	buf[ 0 ] = '\0';

	if ( s[ 0 ] )
	{
		//Q_strcat(buf, sizeof(buf), s);
		Q_strncpyz( buf, s, sizeof( buf ) );
		trap_FS_Write( buf, strlen( buf ), f );
	}

	trap_FS_Write( "\n", 1, f );
}

static void admin_writeconfig_int( int v, fileHandle_t f )
{
	char buf[ 32 ];

	Com_sprintf( buf, sizeof( buf ), "%d", v );
	trap_FS_Write( buf, strlen( buf ), f );
	trap_FS_Write( "\n", 1, f );
}

static void admin_writeconfig( void )
{
	fileHandle_t f;
	int          len, i, j;
	qtime_t      qt;
	int          t;
	char         levels[ MAX_STRING_CHARS ] = { "" };
	char         *filename                  = g_admin.string;

	if ( !g_admin.string[ 0 ] )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: g_admin is not set. "
		          " configuration will not be saved to a file.\n" );
		return;
	}

writeshared:
	t   = trap_RealTime( &qt );
	len = trap_FS_FOpenFile( filename, &f, FS_WRITE );

	if ( len < 0 )
	{
		G_Printf( "admin_writeconfig: could not open %s\n",
		          filename );
	}

	if ( ( strchr( g_adminSharedConfig.string, 'l' ) != 0 ) ^ ( filename == g_admin.string ) )
	{
		for ( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
		{
			trap_FS_Write( "[level]\n", 8, f );
			trap_FS_Write( "level   = ", 10, f );
			admin_writeconfig_int( g_admin_levels[ i ]->level, f );
			trap_FS_Write( "name    = ", 10, f );
			admin_writeconfig_string( g_admin_levels[ i ]->name, f );
			trap_FS_Write( "flags   = ", 10, f );
			admin_writeconfig_string( g_admin_levels[ i ]->flags, f );
			trap_FS_Write( "\n", 1, f );
		}
	}

	if ( ( strchr( g_adminSharedConfig.string, 'a' ) != 0 ) ^ ( filename == g_admin.string ) )
	{
		for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
		{
			// don't write level 0 users
			if ( g_admin_admins[ i ]->level < 1 )
			{
				continue;
			}

			trap_FS_Write( "[admin]\n", 8, f );
			trap_FS_Write( "name    = ", 10, f );
			admin_writeconfig_string( g_admin_admins[ i ]->name, f );
			trap_FS_Write( "guid    = ", 10, f );
			admin_writeconfig_string( g_admin_admins[ i ]->guid, f );
			trap_FS_Write( "level   = ", 10, f );
			admin_writeconfig_int( g_admin_admins[ i ]->level, f );
			trap_FS_Write( "flags   = ", 10, f );
			admin_writeconfig_string( g_admin_admins[ i ]->flags, f );
			trap_FS_Write( "\n", 1, f );
		}
	}

	if ( ( strchr( g_adminSharedConfig.string, 'b' ) != 0 ) ^ ( filename == g_admin.string ) )
	{
		for ( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
		{
			// don't write expired bans
			// if expires is 0, then it's a perm ban
			if ( g_admin_bans[ i ]->expires != 0 &&
			     ( g_admin_bans[ i ]->expires - t ) < 1 )
			{
				continue;
			}

			trap_FS_Write( "[ban]\n", 6, f );
			trap_FS_Write( "name    = ", 10, f );
			admin_writeconfig_string( g_admin_bans[ i ]->name, f );
			trap_FS_Write( "guid    = ", 10, f );
			admin_writeconfig_string( g_admin_bans[ i ]->guid, f );
			trap_FS_Write( "ip      = ", 10, f );
			admin_writeconfig_string( g_admin_bans[ i ]->ip, f );
			trap_FS_Write( "reason  = ", 10, f );
			admin_writeconfig_string( g_admin_bans[ i ]->reason, f );
			trap_FS_Write( "made    = ", 10, f );
			admin_writeconfig_string( g_admin_bans[ i ]->made, f );
			trap_FS_Write( "expires = ", 10, f );
			admin_writeconfig_int( g_admin_bans[ i ]->expires, f );
			trap_FS_Write( "banner  = ", 10, f );
			admin_writeconfig_string( g_admin_bans[ i ]->banner, f );
			trap_FS_Write( "\n", 1, f );
		}
	}

	if ( ( strchr( g_adminSharedConfig.string, 'c' ) != 0 ) ^ ( filename == g_admin.string ) )
	{
		for ( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
		{
			levels[ 0 ] = '\0';
			trap_FS_Write( "[command]\n", 10, f );
			trap_FS_Write( "command = ", 10, f );
			admin_writeconfig_string( g_admin_commands[ i ]->command, f );
			trap_FS_Write( "exec    = ", 10, f );
			admin_writeconfig_string( g_admin_commands[ i ]->exec, f );
			trap_FS_Write( "desc    = ", 10, f );
			admin_writeconfig_string( g_admin_commands[ i ]->desc, f );
			trap_FS_Write( "levels  = ", 10, f );

			for ( j = 0; g_admin_commands[ i ]->levels[ j ] != -1; j++ )
			{
				Q_strcat( levels, sizeof( levels ),
				          va( "%i ", g_admin_commands[ i ]->levels[ j ] ) );
			}

			admin_writeconfig_string( levels, f );
			trap_FS_Write( "\n", 1, f );
		}
	}

	trap_FS_FCloseFile( f );

	if ( filename == g_admin.string && g_adminShared.string[ 0 ] && g_adminSharedConfig.string[ 0 ] )
	{
		filename = g_adminShared.string;
		goto writeshared;
	}

	else if ( filename == g_adminShared.string && g_adminSharedServers.string[ 0 ] )
	{
		const char *s = g_adminSharedServers.string;
		char       port[ BIG_INFO_KEY ], rconpass[ BIG_INFO_VALUE ];

		// send readconfig to all servers
		while ( *s )
		{
			Info_NextPair( &s, port, rconpass );

			if ( !port[ 0 ] || !rconpass[ 0 ] )
			{
				break;
			}

			if ( trap_Cvar_VariableIntegerValue( "net_port" ) == atoi( port ) )
			{
				continue;
			}

			trap_UpdateSharedConfig( atoi( port ), rconpass );
		}
	}
}

static void admin_readconfig_string( char **cnf, char *s, int size )
{
	char *t;

	//COM_MatchToken(cnf, "=");
	s[ 0 ] = '\0';
	t      = COM_ParseExt( cnf, qfalse );

	if ( strcmp( t, "=" ) )
	{
		COM_ParseWarning( "expected '=' before \"%s\"", t );
		Q_strncpyz( s, t, size );
	}

	while ( 1 )
	{
		t = COM_ParseExt( cnf, qfalse );

		if ( !*t )
		{
			break;
		}

		if ( strlen( t ) + strlen( s ) >= size )
		{
			break;
		}

		if ( *s )
		{
			Q_strcat( s, size, " " );
		}

		Q_strcat( s, size, t );
	}
}

static void admin_readconfig_int( char **cnf, int *v )
{
	char *t;

	//COM_MatchToken(cnf, "=");
	t = COM_ParseExt( cnf, qfalse );

	if ( !strcmp( t, "=" ) )
	{
		t = COM_ParseExt( cnf, qfalse );
	}
	else
	{
		COM_ParseWarning( "expected '=' before \"%s\"", t );
	}

	*v = atoi( t );
}

// if we can't parse any levels from readconfig, set up default
// ones to make new installs easier for admins
static void admin_default_levels( void )
{
	g_admin_level_t *l;
	int             i;

	for ( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
	{
		G_Free( g_admin_levels[ i ] );
		g_admin_levels[ i ] = NULL;
	}

	for ( i = 0; i <= 5; i++ )
	{
		l                   = G_Alloc( sizeof( g_admin_level_t ) );
		l->level            = i;
		*l->name            = '\0';
		*l->flags           = '\0';
		g_admin_levels[ i ] = l;
	}

	Q_strncpyz( g_admin_levels[ 0 ]->name, "^4Unknown Player",
	            sizeof( l->name ) );
	Q_strncpyz( g_admin_levels[ 0 ]->flags, "iahC", sizeof( l->flags ) );

	Q_strncpyz( g_admin_levels[ 1 ]->name, "^5Server Regular",
	            sizeof( l->name ) );
	Q_strncpyz( g_admin_levels[ 1 ]->flags, "iahC", sizeof( l->flags ) );

	Q_strncpyz( g_admin_levels[ 2 ]->name, "^6Team Manager",
	            sizeof( l->name ) );
	Q_strncpyz( g_admin_levels[ 2 ]->flags, "iahCpP", sizeof( l->flags ) );

	Q_strncpyz( g_admin_levels[ 3 ]->name, "^2Junior Admin",
	            sizeof( l->name ) );
	Q_strncpyz( g_admin_levels[ 3 ]->flags, "iahCpPkm?", sizeof( l->flags ) );

	Q_strncpyz( g_admin_levels[ 4 ]->name, "^3Senior Admin",
	            sizeof( l->name ) );
	Q_strncpyz( g_admin_levels[ 4 ]->flags, "iahCpPkmBbe?", sizeof( l->flags ) );

	Q_strncpyz( g_admin_levels[ 5 ]->name, "^1Server Operator",
	            sizeof( l->name ) );
	Q_strncpyz( g_admin_levels[ 5 ]->flags, "*", sizeof( l->flags ) );
	admin_level_maxname = 15;
}

// return a level for a player entity.
int G_admin_level( gentity_t *ent )
{
	int i;

	if ( !ent )
	{
		return MAX_ADMIN_LEVELS;
	}

	for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
	{
		if ( !Q_stricmp( g_admin_admins[ i ]->guid, ent->client->pers.guid ) )
		{
			return g_admin_admins[ i ]->level;
		}
	}

	return 0;
}

static qboolean admin_command_permission( gentity_t *ent, char *command )
{
	int i, j;
	int level;

	if ( !ent )
	{
		return qtrue;
	}

	level = ent->client->pers.adminLevel;

	for ( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
	{
		if ( !Q_stricmp( command, g_admin_commands[ i ]->command ) )
		{
			for ( j = 0; g_admin_commands[ i ]->levels[ j ] != -1; j++ )
			{
				if ( g_admin_commands[ i ]->levels[ j ] == level )
				{
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

static void admin_log( gentity_t *admin, char *cmd, int skiparg )
{
	fileHandle_t    f;
	int             len, i, j;
	char            string[ MAX_STRING_CHARS ];
	int             min, tens, sec;
	g_admin_admin_t *a;
	g_admin_level_t *l;
	char            flags[ MAX_ADMIN_FLAGS * 2 ];
	gentity_t       *victim = NULL;
	int             pids[ MAX_CLIENTS ];
	char            name[ MAX_NAME_LENGTH ];

	if ( !g_adminLog.string[ 0 ] )
	{
		return;
	}

	len = trap_FS_FOpenFile( g_adminLog.string, &f, FS_APPEND );

	if ( len < 0 )
	{
		G_Printf( "admin_log: error could not open %s\n", g_adminLog.string );
		return;
	}

	sec    = level.time / 1000;
	min    = sec / 60;
	sec   -= min * 60;
	tens   = sec / 10;
	sec   -= tens * 10;

	*flags = '\0';

	if ( admin )
	{
		for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
		{
			if ( !Q_stricmp( g_admin_admins[ i ]->guid, admin->client->pers.guid ) )
			{
				a = g_admin_admins[ i ];
				Q_strncpyz( flags, a->flags, sizeof( flags ) );

				for ( j = 0; j < MAX_ADMIN_LEVELS && g_admin_levels[ j ]; j++ )
				{
					if ( g_admin_levels[ j ]->level == a->level )
					{
						l = g_admin_levels[ j ];
						Q_strcat( flags, sizeof( flags ), l->flags );
						break;
					}
				}

				break;
			}
		}
	}

	if ( G_SayArgc() > 1 + skiparg )
	{
		G_SayArgv( 1 + skiparg, name, sizeof( name ) );

		if ( G_ClientNumbersFromString( name, pids ) == 1 )
		{
			victim = &g_entities[ pids[ 0 ] ];
		}
	}

	if ( victim && Q_stricmp( cmd, "attempted" ) )
	{
		Com_sprintf( string, sizeof( string ),
		             "%3i:%i%i: %i: %s: %s: %s: %s: %s: %s: \"%s\"\n",
		             min,
		             tens,
		             sec,
		             ( admin ) ? admin->s.clientNum : -1,
		             ( admin ) ? admin->client->pers.guid
		             : "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
		             ( admin ) ? admin->client->pers.netname : "console",
		             flags,
		             cmd,
		             victim->client->pers.guid,
		             victim->client->pers.netname,
		             G_SayConcatArgs( 2 + skiparg ) );
	}
	else
	{
		Com_sprintf( string, sizeof( string ),
		             "%3i:%i%i: %i: %s: %s: %s: %s: \"%s\"\n",
		             min,
		             tens,
		             sec,
		             ( admin ) ? admin->s.clientNum : -1,
		             ( admin ) ? admin->client->pers.guid
		             : "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
		             ( admin ) ? admin->client->pers.netname : "console",
		             flags,
		             cmd,
		             G_SayConcatArgs( 1 + skiparg ) );
	}

	trap_FS_Write( string, strlen( string ), f );
	trap_FS_FCloseFile( f );
}

static int admin_listadmins( gentity_t *ent, int start, char *search )
{
	int       drawn                    = 0;
	char      guid_stub[ 9 ];
	char      name[ MAX_NAME_LENGTH ]  = { "" };
	char      name2[ MAX_NAME_LENGTH ] = { "" };
	char      lname[ MAX_NAME_LENGTH ] = { "" };
	int       i, j;
	gentity_t *vic;
	int       l                        = 0;
	qboolean  dup                      = qfalse;

	ADMBP_begin();

	// print out all connected players regardless of level if name searching
	for ( i = 0; i < level.maxclients && search[ 0 ]; i++ )
	{
		vic = &g_entities[ i ];

		if ( vic->client && vic->client->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		l = vic->client->pers.adminLevel;

		G_SanitiseString( vic->client->pers.netname, name, sizeof( name ) );

		if ( !strstr( name, search ) )
		{
			continue;
		}

		for ( j = 0; j < 8; j++ )
		{
			guid_stub[ j ] = vic->client->pers.guid[ j + 24 ];
		}

		guid_stub[ j ] = '\0';

		lname[ 0 ]     = '\0';

		for ( j = 0; j < MAX_ADMIN_LEVELS && g_admin_levels[ j ]; j++ )
		{
			if ( g_admin_levels[ j ]->level == l )
			{
				int k, colorlen;

				for ( colorlen = k = 0; g_admin_levels[ j ]->name[ k ]; k++ )
				{
					if ( Q_IsColorString( &g_admin_levels[ j ]->name[ k ] ) )
					{
						colorlen += 2;
					}
				}

				Com_sprintf( lname, sizeof( lname ), "%*s",
				             admin_level_maxname + colorlen,
				             g_admin_levels[ j ]->name );
				break;
			}
		}

		ADMBP( va( "%4i %4i %s^7 (*%s) %s^7\n",
		           i,
		           l,
		           lname,
		           guid_stub,
		           vic->client->pers.netname ) );
		drawn++;
	}

	for ( i = start; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ] &&
	      drawn < MAX_ADMIN_LISTITEMS; i++ )
	{
		if ( search[ 0 ] )
		{
			G_SanitiseString( g_admin_admins[ i ]->name, name, sizeof( name ) );

			if ( !strstr( name, search ) )
			{
				continue;
			}

			// verify we don't have the same guid/name pair in connected players
			// since we don't want to draw the same player twice
			dup = qfalse;

			for ( j = 0; j < level.maxclients; j++ )
			{
				vic = &g_entities[ j ];

				if ( !vic->client || vic->client->pers.connected != CON_CONNECTED )
				{
					continue;
				}

				G_SanitiseString( vic->client->pers.netname, name2, sizeof( name2 ) );

				if ( !Q_stricmp( vic->client->pers.guid, g_admin_admins[ i ]->guid ) &&
				     strstr( name2, search ) )
				{
					dup = qtrue;
					break;
				}
			}

			if ( dup )
			{
				continue;
			}
		}

		for ( j = 0; j < 8; j++ )
		{
			guid_stub[ j ] = g_admin_admins[ i ]->guid[ j + 24 ];
		}

		guid_stub[ j ] = '\0';

		lname[ 0 ]     = '\0';

		for ( j = 0; j < MAX_ADMIN_LEVELS && g_admin_levels[ j ]; j++ )
		{
			if ( g_admin_levels[ j ]->level == g_admin_admins[ i ]->level )
			{
				int k, colorlen;

				for ( colorlen = k = 0; g_admin_levels[ j ]->name[ k ]; k++ )
				{
					if ( Q_IsColorString( &g_admin_levels[ j ]->name[ k ] ) )
					{
						colorlen += 2;
					}
				}

				Com_sprintf( lname, sizeof( lname ), "%*s",
				             admin_level_maxname + colorlen,
				             g_admin_levels[ j ]->name );
				break;
			}
		}

		ADMBP( va( "%4i %4i %s^7 (*%s) %s^7\n",
		           ( i + MAX_CLIENTS ),
		           g_admin_admins[ i ]->level,
		           lname,
		           guid_stub,
		           g_admin_admins[ i ]->name ) );
		drawn++;
	}

	ADMBP_end();
	return drawn;
}

void G_admin_duration( int secs, char *duration, int dursize )
{
	if ( secs > ( 60 * 60 * 24 * 365 * 50 ) || secs < 0 )
	{
		Q_strncpyz( duration, "PERMANENT", dursize );
	}
	else if ( secs >= ( 60 * 60 * 24 * 365 ) )
	{
		Com_sprintf( duration, dursize, "%1.1f years",
		             ( secs / ( 60 * 60 * 24 * 365.0f ) ) );
	}
	else if ( secs >= ( 60 * 60 * 24 * 90 ) )
	{
		Com_sprintf( duration, dursize, "%1.1f weeks",
		             ( secs / ( 60 * 60 * 24 * 7.0f ) ) );
	}
	else if ( secs >= ( 60 * 60 * 24 ) )
	{
		Com_sprintf( duration, dursize, "%1.1f days",
		             ( secs / ( 60 * 60 * 24.0f ) ) );
	}
	else if ( secs >= ( 60 * 60 ) )
	{
		Com_sprintf( duration, dursize, "%1.1f hours",
		             ( secs / ( 60 * 60.0f ) ) );
	}
	else if ( secs >= 60 )
	{
		Com_sprintf( duration, dursize, "%1.1f minutes",
		             ( secs / 60.0f ) );
	}
	else
	{
		Com_sprintf( duration, dursize, "%i seconds", secs );
	}
}

qboolean G_admin_ban_check( char *userinfo, char *reason, int rlen )
{
	char    *guid, *ip;
	int     i;
	qtime_t qt;
	int     t;

	*reason = '\0';
	t       = trap_RealTime( &qt );

	if ( !*userinfo )
	{
		return qfalse;
	}

	ip = Info_ValueForKey( userinfo, "ip" );

	if ( !*ip )
	{
		return qfalse;
	}

	guid = Info_ValueForKey( userinfo, "cl_guid" );

	for ( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
	{
		// 0 is for perm ban
		if ( g_admin_bans[ i ]->expires != 0 &&
		     ( g_admin_bans[ i ]->expires - t ) < 1 )
		{
			continue;
		}

		if ( strstr( ip, g_admin_bans[ i ]->ip ) )
		{
			char duration[ 32 ];
			G_admin_duration( ( g_admin_bans[ i ]->expires - t ),
			                  duration, sizeof( duration ) );
			Com_sprintf(
			  reason,
			  rlen,
			  "You have been banned by %s^7 reason: %s^7 expires: %s",
			  g_admin_bans[ i ]->banner,
			  g_admin_bans[ i ]->reason,
			  duration
			);
			G_Printf( "Banned player tried to connect from IP %s\n", ip );
			return qtrue;
		}

		if ( *guid && !Q_stricmp( g_admin_bans[ i ]->guid, guid ) )
		{
			char duration[ 32 ];
			G_admin_duration( ( g_admin_bans[ i ]->expires - t ),
			                  duration, sizeof( duration ) );
			Com_sprintf(
			  reason,
			  rlen,
			  "You have been banned by %s^7 reason: %s^7 expires: %s",
			  g_admin_bans[ i ]->banner,
			  g_admin_bans[ i ]->reason,
			  duration
			);
			G_Printf( "Banned player tried to connect with GUID %s\n", guid );
			return qtrue;
		}
	}

	return qfalse;
}

qboolean G_admin_cmd_check( gentity_t *ent, qboolean say )
{
	int  i;
	char command[ MAX_ADMIN_CMD_LEN ];
	char *cmd;
	int  skip = 0;

	if ( g_admin.string[ 0 ] == '\0' )
	{
		return qfalse;
	}

	command[ 0 ] = '\0';
	G_SayArgv( 0, command, sizeof( command ) );

	if ( !Q_stricmp( command, "say" ) ||
	     ( G_admin_permission( ent, ADMF_TEAMFTCMD ) &&
	       ( !Q_stricmp( command, "say_team" ) ||
	         !Q_stricmp( command, "say_buddy" ) ) ) )
	{
		skip = 1;
		G_SayArgv( 1, command, sizeof( command ) );
	}

	if ( !command[ 0 ] )
	{
		return qfalse;
	}

	if ( command[ 0 ] == '!' )
	{
		cmd = &command[ 1 ];
	}
	else
	{
		return qfalse;
	}

	for ( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
	{
		if ( Q_stricmp( cmd, g_admin_commands[ i ]->command ) )
		{
			continue;
		}

		if ( admin_command_permission( ent, cmd ) )
		{
			trap_SendConsoleCommand( EXEC_APPEND, g_admin_commands[ i ]->exec );
			admin_log( ent, cmd, skip );
			return qtrue;
		}
		else
		{
			ADMP( va( "^3!%s: ^7permission denied\n", g_admin_commands[ i ]->command ) );
			admin_log( ent, "attempted", skip - 1 );
			return qfalse;
		}
	}

	for ( i = 0; i < adminNumCmds; i++ )
	{
		if ( Q_stricmp( cmd, g_admin_cmds[ i ].keyword ) )
		{
			continue;
		}

		if ( G_admin_permission( ent, g_admin_cmds[ i ].flag[ 0 ] ) )
		{
			g_admin_cmds[ i ].handler( ent, skip );
			admin_log( ent, cmd, skip );
			return qtrue;
		}
		else
		{
			ADMP( va( "^3!%s: ^7permission denied\n", g_admin_cmds[ i ].keyword ) );
			admin_log( ent, "attempted", skip - 1 );
		}
	}

	return qfalse;
}

void G_admin_namelog_cleanup()
{
	int i;

	for ( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
	{
		G_Free( g_admin_namelog[ i ] );
		g_admin_namelog[ i ] = NULL;
	}
}

void G_admin_namelog_update( gclient_t *client, qboolean disconnect )
{
	int               i, j;
	g_admin_namelog_t *namelog;
	char              n1[ MAX_NAME_LENGTH ];
	char              n2[ MAX_NAME_LENGTH ];
	int               clientNum = ( client - level.clients );

	if ( !g_admin.string[ 0 ] )
	{
		return;
	}

	G_SanitiseName( client->pers.netname, n1 );

	for ( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
	{
		if ( disconnect && g_admin_namelog[ i ]->slot != clientNum )
		{
			continue;
		}

		if ( !disconnect && !( g_admin_namelog[ i ]->slot == clientNum ||
		                       g_admin_namelog[ i ]->slot == -1 ) )
		{
			continue;
		}

		if ( !Q_stricmp( client->pers.ip, g_admin_namelog[ i ]->ip )
		     && !Q_stricmp( client->pers.guid, g_admin_namelog[ i ]->guid ) )
		{
			for ( j = 0; j < MAX_ADMIN_NAMELOG_NAMES
			      && g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
			{
				G_SanitiseName( g_admin_namelog[ i ]->name[ j ], n2 );

				if ( !Q_stricmp( n1, n2 ) )
				{
					break;
				}
			}

			if ( j == MAX_ADMIN_NAMELOG_NAMES )
			{
				j = MAX_ADMIN_NAMELOG_NAMES - 1;
			}

			Q_strncpyz( g_admin_namelog[ i ]->name[ j ], client->pers.netname,
			            sizeof( g_admin_namelog[ i ]->name[ j ] ) );
			g_admin_namelog[ i ]->slot = ( disconnect ) ? -1 : clientNum;
			return;
		}
	}

	if ( i >= MAX_ADMIN_NAMELOGS )
	{
		G_Printf( "G_admin_namelog_update: warning, g_admin_namelogs overflow\n" );
		return;
	}

	namelog         = G_Alloc( sizeof( g_admin_namelog_t ) );
	namelog->banned = qfalse;

	for ( j = 1; j < MAX_ADMIN_NAMELOG_NAMES; j++ )
	{
		namelog->name[ j ][ 0 ] = '\0';
	}

	Q_strncpyz( namelog->ip, client->pers.ip, sizeof( namelog->ip ) );
	Q_strncpyz( namelog->guid, client->pers.guid, sizeof( namelog->guid ) );
	Q_strncpyz( namelog->name[ 0 ], client->pers.netname,
	            sizeof( namelog->name[ 0 ] ) );
	namelog->slot        = ( disconnect ) ? -1 : clientNum;
	g_admin_namelog[ i ] = namelog;
}

qboolean G_admin_readconfig( gentity_t *ent, int skiparg )
{
	g_admin_level_t   *l = NULL;
	g_admin_admin_t   *a = NULL;
	g_admin_ban_t     *b = NULL;
	g_admin_command_t *c = NULL;
	int               lc = 0, ac = 0, bc = 0, cc = 0;
	fileHandle_t      f;
	int               len;
	char              *cnf, *cnf2;
	char              *t;
	qboolean          level_open, admin_open, ban_open, command_open;
	int               i;
	char              *filename = g_admin.string;

	G_admin_cleanup();

	if ( !g_admin.string[ 0 ] )
	{
		ADMP( "^3!readconfig: g_admin is not set, not loading configuration "
		      "from a file\n" );
		admin_default_levels();
		return qfalse;
	}

reparse:
	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len < 0 )
	{
		G_Printf( "^3!readconfig: ^7could not open admin config file %s\n",
		          filename );
		admin_default_levels();
		return qfalse;
	}

	cnf                 = G_Alloc( len + 1 );
	cnf2                = cnf;
	trap_FS_Read( cnf, len, f );
	*( cnf + len )      = '\0';
	trap_FS_FCloseFile( f );

	admin_level_maxname = 0;

	level_open          = admin_open = ban_open = command_open = qfalse;
	COM_BeginParseSession( g_admin.string );

	while ( 1 )
	{
		t = COM_Parse( &cnf );

		if ( !*t )
		{
			break;
		}

		if ( !Q_stricmp( t, "[level]" ) )
		{
			if ( lc >= MAX_ADMIN_LEVELS )
			{
				return qfalse;
			}

			l                      = G_Alloc( sizeof( g_admin_level_t ) );
			g_admin_levels[ lc++ ] = l;
			level_open             = qtrue;
			admin_open             = ban_open = command_open = qfalse;
		}
		else if ( !Q_stricmp( t, "[admin]" ) )
		{
			if ( ac >= MAX_ADMIN_ADMINS )
			{
				return qfalse;
			}

			a                      = G_Alloc( sizeof( g_admin_admin_t ) );
			g_admin_admins[ ac++ ] = a;
			admin_open             = qtrue;
			level_open             = ban_open = command_open = qfalse;
		}
		else if ( !Q_stricmp( t, "[ban]" ) )
		{
			if ( bc >= MAX_ADMIN_BANS )
			{
				return qfalse;
			}

			b                    = G_Alloc( sizeof( g_admin_ban_t ) );
			g_admin_bans[ bc++ ] = b;
			ban_open             = qtrue;
			level_open           = admin_open = command_open = qfalse;
		}
		else if ( !Q_stricmp( t, "[command]" ) )
		{
			if ( cc >= MAX_ADMIN_COMMANDS )
			{
				return qfalse;
			}

			c                        = G_Alloc( sizeof( g_admin_command_t ) );
			g_admin_commands[ cc++ ] = c;
			c->levels[ 0 ]           = -1;
			command_open             = qtrue;
			level_open               = admin_open = ban_open = qfalse;
		}
		else if ( level_open )
		{
			if ( !Q_stricmp( t, "level" ) )
			{
				admin_readconfig_int( &cnf, &l->level );
			}
			else if ( !Q_stricmp( t, "name" ) )
			{
				admin_readconfig_string( &cnf, l->name, sizeof( l->name ) );
			}
			else if ( !Q_stricmp( t, "flags" ) )
			{
				admin_readconfig_string( &cnf, l->flags, sizeof( l->flags ) );
			}
			else
			{
				ADMP( va( "^3!readconfig: ^7[level] parse error near %s on line %d in file %s\n",
				          t,
				          COM_GetCurrentParseLine(),
				          filename ) );
			}
		}
		else if ( admin_open )
		{
			if ( !Q_stricmp( t, "name" ) )
			{
				admin_readconfig_string( &cnf, a->name, sizeof( a->name ) );
			}
			else if ( !Q_stricmp( t, "guid" ) )
			{
				admin_readconfig_string( &cnf, a->guid, sizeof( a->guid ) );
			}
			else if ( !Q_stricmp( t, "level" ) )
			{
				admin_readconfig_int( &cnf, &a->level );
			}
			else if ( !Q_stricmp( t, "flags" ) )
			{
				admin_readconfig_string( &cnf, a->flags, sizeof( a->flags ) );
			}
			else
			{
				ADMP( va( "^3!readconfig: ^7[admin] parse error near %s on line %d in file %s\n",
				          t,
				          COM_GetCurrentParseLine(),
				          filename ) );
			}
		}
		else if ( ban_open )
		{
			if ( !Q_stricmp( t, "name" ) )
			{
				admin_readconfig_string( &cnf, b->name, sizeof( b->name ) );
			}
			else if ( !Q_stricmp( t, "guid" ) )
			{
				admin_readconfig_string( &cnf, b->guid, sizeof( b->guid ) );
			}
			else if ( !Q_stricmp( t, "ip" ) )
			{
				admin_readconfig_string( &cnf, b->ip, sizeof( b->ip ) );
			}
			else if ( !Q_stricmp( t, "reason" ) )
			{
				admin_readconfig_string( &cnf, b->reason, sizeof( b->reason ) );
			}
			else if ( !Q_stricmp( t, "made" ) )
			{
				admin_readconfig_string( &cnf, b->made, sizeof( b->made ) );
			}
			else if ( !Q_stricmp( t, "expires" ) )
			{
				admin_readconfig_int( &cnf, &b->expires );
			}
			else if ( !Q_stricmp( t, "banner" ) )
			{
				admin_readconfig_string( &cnf, b->banner, sizeof( b->banner ) );
			}
			else
			{
				ADMP( va( "^3!readconfig: ^7[ban] parse error near %s on line %d in file %s\n",
				          t,
				          COM_GetCurrentParseLine(),
				          filename ) );
			}
		}
		else if ( command_open )
		{
			if ( !Q_stricmp( t, "command" ) )
			{
				admin_readconfig_string( &cnf, c->command, sizeof( c->command ) );
			}
			else if ( !Q_stricmp( t, "exec" ) )
			{
				admin_readconfig_string( &cnf, c->exec, sizeof( c->exec ) );
			}
			else if ( !Q_stricmp( t, "desc" ) )
			{
				admin_readconfig_string( &cnf, c->desc, sizeof( c->desc ) );
			}
			else if ( !Q_stricmp( t, "levels" ) )
			{
				char levels[ MAX_STRING_CHARS ] = { "" };
				char *level                     = levels;
				char *lp;
				int  cmdlevel                   = 0;

				admin_readconfig_string( &cnf, levels, sizeof( levels ) );

				while ( cmdlevel < MAX_ADMIN_LEVELS )
				{
					lp                      = COM_Parse( &level );

					if ( !*lp )
					{
						break;
					}

					c->levels[ cmdlevel++ ] = atoi( lp );
				}

				// ensure the list is -1 terminated
				c->levels[ cmdlevel ] = -1;
			}
			else
			{
				ADMP( va( "^3!readconfig: ^7[command] parse error near %s on line %d in file %s\n",
				          t,
				          COM_GetCurrentParseLine(),
				          filename ) );
			}
		}
		else
		{
			COM_ParseError( "unexpected token \"%s\"", t );
		}
	}

	G_Free( cnf2 );

	if ( filename == g_admin.string && g_adminShared.string[ 0 ] )
	{
		filename = g_adminShared.string;
		goto reparse;
	}

	ADMP( va( "^3!readconfig: ^7loaded %d levels, %d admins, %d bans, %d commands\n",
	          lc, ac, bc, cc ) );

	if ( lc == 0 )
	{
		admin_default_levels();
	}
	else
	{
		// max printable name length for formatting
		for ( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
		{
			len = Q_PrintStrlen( l->name );

			if ( len > admin_level_maxname )
			{
				admin_level_maxname = len;
			}
		}
	}

	// reset adminLevel
	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].pers.connected != CON_DISCONNECTED )
		{
			level.clients[ i ].pers.adminLevel = G_admin_level( &g_entities[ i ] );
		}
	}

	return qtrue;
}

qboolean G_admin_time( gentity_t *ent, int skiparg )
{
	qtime_t qt;
	int     t;

	t = trap_RealTime( &qt );
	AP( va( "print \"^3!time: ^7local time is %02i:%02i:%02i\n\"",
	        qt.tm_hour, qt.tm_min, qt.tm_sec ) );
	return qtrue;
}

qboolean G_admin_setlevel( gentity_t *ent, int skiparg )
{
	char            name[ MAX_NAME_LENGTH ]      = { "" };
	char            lstr[ 11 ]; // 10 is max strlen() for 32-bit int
	char            adminname[ MAX_NAME_LENGTH ] = { "" };
	char            testname[ MAX_NAME_LENGTH ]  = { "" };
	char            guid[ 33 ];
	int             l, i;
	gentity_t       *vic                         = NULL;
	qboolean        updated                      = qfalse;
	g_admin_admin_t *a;
	qboolean        found                        = qfalse;
	qboolean        numeric                      = qtrue;
	int             matches                      = 0;
	int             id                           = -1;

	if ( G_SayArgc() < 3 + skiparg )
	{
		ADMP( "^3!setlevel: ^7usage: !setlevel [name|slot#] [level]\n" );
		return qfalse;
	}

	G_SayArgv( 1 + skiparg, testname, sizeof( testname ) );
	G_SayArgv( 2 + skiparg, lstr, sizeof( lstr ) );
	l = atoi( lstr );
	G_SanitiseString( testname, name, sizeof( name ) );

	for ( i = 0; i < sizeof( name ) && name[ i ]; i++ )
	{
		if ( !isdigit( name[ i ] ) )
		{
			numeric = qfalse;
			break;
		}
	}

	if ( numeric )
	{
		id = atoi( name );
	}

	if ( ent && l > ent->client->pers.adminLevel )
	{
		ADMP( "^3!setlevel: ^7you may not use !setlevel to set a level higher "
		      "than your current level\n" );
		return qfalse;
	}

	// if admin is activated for the first time on a running server, we need
	// to ensure at least the default levels get created
	if ( !ent && !g_admin_levels[ 0 ] )
	{
		G_admin_readconfig( NULL, 0 );
	}

	for ( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
	{
		if ( g_admin_levels[ i ]->level == l )
		{
			found = qtrue;
			break;
		}
	}

	if ( !found )
	{
		ADMP( "^3!setlevel: ^7level is not defined\n" );
		return qfalse;
	}

	if ( numeric && id >= 0 && id < level.maxclients )
	{
		vic = &g_entities[ id ];
	}

	if ( vic && vic->client && vic->client->pers.connected != CON_DISCONNECTED )
	{
		Q_strncpyz( adminname, vic->client->pers.netname, sizeof( adminname ) );
		Q_strncpyz( guid, vic->client->pers.guid, sizeof( guid ) );
		matches = 1;
	}
	else if ( numeric && id >= MAX_CLIENTS && id < MAX_CLIENTS + MAX_ADMIN_ADMINS
	          && g_admin_admins[ id - MAX_CLIENTS ] )
	{
		Q_strncpyz( adminname, g_admin_admins[ id - MAX_CLIENTS ]->name,
		            sizeof( adminname ) );
		Q_strncpyz( guid, g_admin_admins[ id - MAX_CLIENTS ]->guid,
		            sizeof( guid ) );
		matches = 1;
	}
	else
	{
		for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ] && matches < 2; i++ )
		{
			G_SanitiseString( g_admin_admins[ i ]->name, testname, sizeof( testname ) );

			if ( strstr( testname, name ) )
			{
				Q_strncpyz( adminname, g_admin_admins[ i ]->name, sizeof( adminname ) );
				Q_strncpyz( guid, g_admin_admins[ i ]->guid, sizeof( guid ) );
				matches++;
			}
		}

		for ( i = 0; i < level.maxclients && matches < 2; i++ )
		{
			if ( level.clients[ i ].pers.connected == CON_DISCONNECTED )
			{
				continue;
			}

			if ( matches && !Q_stricmp( level.clients[ i ].pers.guid, guid ) )
			{
				vic = &g_entities[ i ];
				continue;
			}

			G_SanitiseString( level.clients[ i ].pers.netname, testname,
			                  sizeof( testname ) );

			if ( strstr( testname, name ) )
			{
				vic = &g_entities[ i ];
				matches++;
				Q_strncpyz( guid, vic->client->pers.guid, sizeof( guid ) );
			}
		}

		if ( vic )
		{
			Q_strncpyz( adminname, vic->client->pers.netname, sizeof( adminname ) );
		}
	}

	if ( matches == 0 )
	{
		ADMP( "^3!setlevel:^7 no match.  use !listplayers or !listadmins to "
		      "find an appropriate number to use instead of name.\n" );
		return qfalse;
	}

	if ( matches > 1 )
	{
		ADMP( "^3!setlevel:^7 more than one match.  Use the admin number "
		      "instead:\n" );
		admin_listadmins( ent, 0, name );
		return qfalse;
	}

	if ( !Q_stricmp( guid, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) )
	{
		ADMP( va( "^3!setlevel: ^7%s does not have a valid GUID\n", adminname ) );
		return qfalse;
	}

	if ( ent && !admin_higher_guid( ent->client->pers.guid, guid ) )
	{
		ADMP( "^3!setlevel: ^7sorry, but your intended victim has a higher"
		      " admin level than you\n" );
		return qfalse;
	}

	for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
	{
		if ( !Q_stricmp( g_admin_admins[ i ]->guid, guid ) )
		{
			g_admin_admins[ i ]->level = l;
			Q_strncpyz( g_admin_admins[ i ]->name, adminname,
			            sizeof( g_admin_admins[ i ]->name ) );
			updated                    = qtrue;
		}
	}

	if ( !updated )
	{
		if ( i == MAX_ADMIN_ADMINS )
		{
			ADMP( "^3!setlevel: ^7too many admins\n" );
			return qfalse;
		}

		a                   = G_Alloc( sizeof( g_admin_admin_t ) );
		a->level            = l;
		Q_strncpyz( a->name, adminname, sizeof( a->name ) );
		Q_strncpyz( a->guid, guid, sizeof( a->guid ) );
		*a->flags           = '\0';
		g_admin_admins[ i ] = a;
	}

	AP( va(
	      "print \"^3!setlevel: ^7%s^7 was given level %d admin rights by %s\n\"",
	      adminname, l, ( ent ) ? ent->client->pers.netname : "console" ) );

	if ( vic )
	{
		vic->client->pers.adminLevel = l;
	}

	if ( !g_admin.string[ 0 ] )
	{
		ADMP( "^3!setlevel: ^7WARNING g_admin not set, not saving admin record "
		      "to a file\n" );
	}
	else
	{
		admin_writeconfig();
	}

	return qtrue;
}

static qboolean admin_create_ban( gentity_t *ent,
                                  char *netname,
                                  char *guid,
                                  char *ip,
                                  int seconds,
                                  char *reason )
{
	g_admin_ban_t *b = NULL;
	qtime_t       qt;
	int           t;
	int           i;

	t = trap_RealTime( &qt );
	b = G_Alloc( sizeof( g_admin_ban_t ) );

	if ( !b )
	{
		return qfalse;
	}

	Q_strncpyz( b->name, netname, sizeof( b->name ) );
	Q_strncpyz( b->guid, guid, sizeof( b->guid ) );
	Q_strncpyz( b->ip, ip, sizeof( b->ip ) );

	Com_sprintf( b->made, sizeof( b->made ), "%04i-%02i-%02i %02i:%02i:%02i",
	             1900 + qt.tm_year, qt.tm_mon + 1, qt.tm_mday,
	             qt.tm_hour, qt.tm_min, qt.tm_sec );

	if ( ent )
	{
		Q_strncpyz( b->banner, ent->client->pers.netname, sizeof( b->banner ) );
	}
	else
	{
		Q_strncpyz( b->banner, "console", sizeof( b->banner ) );
	}

	if ( !seconds )
	{
		b->expires = 0;
	}
	else
	{
		b->expires = t + seconds;
	}

	if ( !*reason )
	{
		Q_strncpyz( b->reason, "banned by admin", sizeof( b->reason ) );
	}
	else
	{
		Q_strncpyz( b->reason, reason, sizeof( b->reason ) );
	}

	for ( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
	{
		;
	}

	if ( i == MAX_ADMIN_BANS )
	{
		ADMP( "^3!ban: ^7too many bans\n" );
		G_Free( b );
		return qfalse;
	}

	g_admin_bans[ i ] = b;
	return qtrue;
}

int G_admin_parse_time( const char *time )
{
	int seconds = 0, num = 0;

	while ( *time )
	{
		if ( !isdigit( *time ) )
		{
			return -1;
		}

		while ( isdigit( *time ) )
		{
			num = num * 10 + *time++ - '0';
		}

		if ( !*time )
		{
			break;
		}

		switch ( *time++ )
		{
			case 'w':
				num *= 7;

			case 'd':
				num *= 24;

			case 'h':
				num *= 60;

			case 'm':
				num *= 60;

			case 's':
				break;

			default:
				return -1;
		}

		seconds += num;
		num      = 0;
	}

	if ( num )
	{
		seconds += num;
	}

	return seconds;
}

qboolean G_admin_kick( gentity_t *ent, int skiparg )
{
	int       pids[ MAX_CLIENTS ];
	char      name[ MAX_NAME_LENGTH ], *reason, err[ MAX_STRING_CHARS ];
	int       minargc;
	gentity_t *vic;

	minargc = 3 + skiparg;

	if ( G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
	{
		minargc = 2 + skiparg;
	}

	if ( G_SayArgc() < minargc )
	{
		ADMP( "^3!kick: ^7usage: kick [name] [reason]\n" );
		return qfalse;
	}

	G_SayArgv( 1 + skiparg, name, sizeof( name ) );
	reason = G_SayConcatArgs( 2 + skiparg );

	if ( G_ClientNumbersFromString( name, pids ) != 1 )
	{
		G_MatchOnePlayer( pids, err, sizeof( err ) );
		ADMP( va( "^3!kick: ^7%s\n", err ) );
		return qfalse;
	}

	if ( !admin_higher( ent, &g_entities[ pids[ 0 ] ] ) )
	{
		ADMP( "^3!kick: ^7sorry, but your intended victim has a higher admin"
		      " level than you\n" );
		return qfalse;
	}

	vic = &g_entities[ pids[ 0 ] ];

	if ( g_adminTempBan.integer > 0 )
	{
		admin_create_ban( ent,
		                  vic->client->pers.netname,
		                  vic->client->pers.guid,
		                  vic->client->pers.ip, g_adminTempBan.integer,
		                  "automatic temp ban created by kick" );
	}

	trap_SendServerCommand( pids[ 0 ],
	                        va( "disconnect \"You have been kicked.\n%s^7\nreason:\n%s\"",
	                            ( ent ) ? va( "admin:\n%s", ent->client->pers.netname ) : "",
	                            ( *reason ) ? reason : "kicked by admin" ) );

	trap_DropClient( pids[ 0 ], va( "has been kicked%s^7. reason: %s",
	                                ( ent ) ? va( " by %s", ent->client->pers.netname ) : "",
	                                ( *reason ) ? reason : "kicked by admin" ), 120 );

	return qtrue;
}

qboolean G_admin_ban( gentity_t *ent, int skiparg )
{
	int      seconds;
	char     search[ MAX_NAME_LENGTH ];
	char     secs[ MAX_TOKEN_CHARS ];
	char     *reason;
	int      minargc;
	char     duration[ 32 ];
	int      logmatch   = -1, logmatches = 0;
	int      i, j;
	qboolean exactmatch = qfalse;
	char     n2[ MAX_NAME_LENGTH ];
	char     s2[ MAX_NAME_LENGTH ];
	char     guid_stub[ 9 ];

	if ( G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
	     G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
	{
		minargc = 2 + skiparg;
	}
	else if ( G_admin_permission( ent, ADMF_CAN_PERM_BAN ) ||
	          G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
	{
		minargc = 3 + skiparg;
	}
	else
	{
		minargc = 4 + skiparg;
	}

	if ( G_SayArgc() < minargc )
	{
		ADMP( "^3!ban: ^7usage: !ban [name|slot|ip] [duration] [reason]\n" );
		return qfalse;
	}

	G_SayArgv( 1 + skiparg, search, sizeof( search ) );
	G_SanitiseString( search, s2, sizeof( s2 ) );
	G_SayArgv( 2 + skiparg, secs, sizeof( secs ) );

	seconds = G_admin_parse_time( secs );

	if ( seconds <= 0 )
	{
		if ( G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
		{
			seconds = 0;
		}
		else
		{
			ADMP( "^3!ban: ^7you may not issue permanent bans\n" );
			return qfalse;
		}

		reason = G_SayConcatArgs( 2 + skiparg );
	}
	else
	{
		reason = G_SayConcatArgs( 3 + skiparg );
	}

	for ( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
	{
		// skip players in the namelog who have already been banned
		if ( g_admin_namelog[ i ]->banned )
		{
			continue;
		}

		// skip disconnected players when banning on slot number
		if ( g_admin_namelog[ i ]->slot == -1 )
		{
			continue;
		}

		if ( !Q_stricmp( va( "%d", g_admin_namelog[ i ]->slot ), search ) )
		{
			logmatches = 1;
			logmatch   = i;
			exactmatch = qtrue;
			break;
		}
	}

	for ( i = 0;
	      !exactmatch && i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ];
	      i++ )
	{
		// skip players in the namelog who have already been banned
		if ( g_admin_namelog[ i ]->banned )
		{
			continue;
		}

		if ( !Q_stricmp( g_admin_namelog[ i ]->ip, search ) )
		{
			logmatches = 1;
			logmatch   = i;
			exactmatch = qtrue;
			break;
		}

		for ( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
		      g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
		{
			G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );

			if ( strstr( n2, s2 ) )
			{
				if ( logmatch != i )
				{
					logmatches++;
				}

				logmatch = i;
			}
		}
	}

	if ( !logmatches )
	{
		ADMP( "^3!ban: ^7no player found by that name, IP, or slot number\n" );
		return qfalse;
	}

	if ( logmatches > 1 )
	{
		ADMBP_begin();
		ADMBP( "^3!ban: ^7multiple recent clients match name, use IP or slot#:\n" );

		for ( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
		{
			for ( j = 0; j < 8; j++ )
			{
				guid_stub[ j ] = g_admin_namelog[ i ]->guid[ j + 24 ];
			}

			guid_stub[ j ] = '\0';

			for ( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
			      g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
			{
				G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );

				if ( strstr( n2, s2 ) )
				{
					if ( g_admin_namelog[ i ]->slot > -1 )
					{
						ADMBP( "^3" );
					}

					ADMBP( va( "%-2s (*%s) %15s ^7'%s^7'\n",
					           ( g_admin_namelog[ i ]->slot > -1 ) ?
					           va( "%d", g_admin_namelog[ i ]->slot ) : "-",
					           guid_stub,
					           g_admin_namelog[ i ]->ip,
					           g_admin_namelog[ i ]->name[ j ] ) );
				}
			}
		}

		ADMBP_end();
		return qfalse;
	}

	if ( ent && !admin_higher_guid( ent->client->pers.guid,
	                                g_admin_namelog[ logmatch ]->guid ) )
	{
		ADMP( "^3!ban: ^7sorry, but your intended victim has a higher admin"
		      " level than you\n" );
		return qfalse;
	}

	if ( !strcmp( g_admin_namelog[ logmatch ]->ip, "localhost" ) )
	{
		ADMP( "^3!ban: ^7disconnecting the host would end the game\n" );
		return qfalse;
	}

	G_admin_duration( ( seconds ) ? seconds : -1,
	                  duration, sizeof( duration ) );

	admin_create_ban( ent,
	                  g_admin_namelog[ logmatch ]->name[ 0 ],
	                  g_admin_namelog[ logmatch ]->guid,
	                  g_admin_namelog[ logmatch ]->ip,
	                  seconds, reason );

	g_admin_namelog[ logmatch ]->banned = qtrue;

	if ( !g_admin.string[ 0 ] )
	{
		ADMP( "^3!ban: ^7WARNING g_admin not set, not saving ban to a file\n" );
	}
	else
	{
		admin_writeconfig();
	}

	if ( g_admin_namelog[ logmatch ]->slot == -1 )
	{
		// client is already disconnected so stop here
		AP( va( "print \"^3!ban:^7 %s^7 has been banned by %s^7, "
		        "duration: %s, reason: %s\n\"",
		        g_admin_namelog[ logmatch ]->name[ 0 ],
		        ( ent ) ? ent->client->pers.netname : "console",
		        duration,
		        ( *reason ) ? reason : "banned by admin" ) );
		return qtrue;
	}

	trap_SendServerCommand( g_admin_namelog[ logmatch ]->slot,
	                        va( "disconnect \"You have been banned.\n"
	                            "admin:\n%s^7\nduration:\n%s\nreason:\n%s\"",
	                            ( ent ) ? ent->client->pers.netname : "console",
	                            duration,
	                            ( *reason ) ? reason : "banned by admin" ) );

	trap_DropClient(  g_admin_namelog[ logmatch ]->slot,
	                  va( "has been banned by %s^7, duration: %s, reason: %s",
	                      ( ent ) ? ent->client->pers.netname : "console",
	                      duration,
	                      ( *reason ) ? reason : "banned by admin" ), 0 );
	return qtrue;
}

qboolean G_admin_unban( gentity_t *ent, int skiparg )
{
	int  bnum;
	char bs[ 5 ];

	if ( G_SayArgc() < 2 + skiparg )
	{
		ADMP( "^3!unban: ^7usage: !unban [ban#]\n" );
		return qfalse;
	}

	G_SayArgv( 1 + skiparg, bs, sizeof( bs ) );
	bnum = atoi( bs );

	if ( bnum < 1 || bnum > MAX_ADMIN_BANS || !g_admin_bans[ bnum - 1 ] )
	{
		ADMP( "^3!unban: ^7invalid ban#\n" );
		return qfalse;
	}

	if ( g_admin_bans[ bnum - 1 ]->expires == 0 &&
	     !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
	{
		ADMP( "^3!unban: ^7you cannot remove permanent bans\n" );
		return qfalse;
	}

	g_admin_bans[ bnum - 1 ]->expires = trap_RealTime( NULL );
	AP( va( "print \"^3!unban: ^7ban #%d for %s^7 has been removed by %s\n\"",
	        bnum,
	        g_admin_bans[ bnum - 1 ]->name,
	        ( ent ) ? ent->client->pers.netname : "console" ) );

	if ( g_admin.string[ 0 ] )
	{
		admin_writeconfig();
	}

	return qtrue;
}

qboolean G_admin_adjustban( gentity_t *ent, int skiparg )
{
	int           bnum;
	int           length;
	int           expires;
	int           time           = trap_RealTime( NULL );
	char          duration[ 32 ] = { "" };
	char          *reason;
	char          bs[ 5 ];
	char          secs[ MAX_TOKEN_CHARS ];
	char          mode = '\0';
	g_admin_ban_t *ban;

	if ( G_SayArgc() < 3 + skiparg )
	{
		ADMP( "^3!adjustban: ^7usage: !adjustban [ban#] [duration] [reason]\n" );
		return qfalse;
	}

	G_SayArgv( 1 + skiparg, bs, sizeof( bs ) );
	bnum = atoi( bs );

	if ( bnum < 1 || bnum > MAX_ADMIN_BANS || !g_admin_bans[ bnum - 1 ] )
	{
		ADMP( "^3!adjustban: ^7invalid ban#\n" );
		return qfalse;
	}

	ban = g_admin_bans[ bnum - 1 ];

	if ( ban->expires == 0 && !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
	{
		ADMP( "^3!adjustban: ^7you cannot modify permanent bans\n" );
		return qfalse;
	}

	G_SayArgv( 2 + skiparg, secs, sizeof( secs ) );

	if ( secs[ 0 ] == '+' || secs[ 0 ] == '-' )
	{
		mode = secs[ 0 ];
	}

	length = G_admin_parse_time( &secs[ mode ? 1 : 0 ] );

	if ( length < 0 )
	{
		skiparg--;
	}
	else
	{
		if ( length )
		{
			if ( ban->expires == 0 && mode )
			{
				ADMP( "^3!adjustban: ^7new duration must be explicit\n" );
				return qfalse;
			}

			if ( mode == '+' )
			{
				expires = ban->expires + length;
			}
			else if ( mode == '-' )
			{
				expires = ban->expires - length;
			}
			else
			{
				expires = time + length;
			}

			if ( expires <= time )
			{
				ADMP( "^3!adjustban: ^7ban duration must be positive\n" );
				return qfalse;
			}
		}
		else if ( G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
		{
			expires = 0;
		}
		else
		{
			ADMP( "^3!adjustban: ^7ban duration must be positive\n" );
			return qfalse;
		}

		ban->expires = expires;
		G_admin_duration( ( expires ) ? expires - time : -1, duration,
		                  sizeof( duration ) );
	}

	reason = G_SayConcatArgs( 3 + skiparg );

	if ( *reason )
	{
		Q_strncpyz( ban->reason, reason, sizeof( ban->reason ) );
	}

	AP( va( "print \"^3!adjustban: ^7ban #%d for %s^7 has been updated by %s^7 "
	        "%s%s%s%s%s\n\"",
	        bnum,
	        ban->name,
	        ( ent ) ? ent->client->pers.netname : "console",
	        ( length >= 0 ) ? "duration: " : "",
	        duration,
	        ( length >= 0 && *reason ) ? ", " : "",
	        ( *reason ) ? "reason: " : "",
	        reason ) );

	if ( ent )
	{
		Q_strncpyz( ban->banner, ent->client->pers.netname, sizeof( ban->banner ) );
	}

	if ( g_admin.string[ 0 ] )
	{
		admin_writeconfig();
	}

	return qtrue;
}

qboolean G_admin_putteam( gentity_t *ent, int skiparg )
{
	int       pids[ MAX_CLIENTS ];
	char      name[ MAX_NAME_LENGTH ], team[ 7 ], err[ MAX_STRING_CHARS ];
	gentity_t *vic;

	G_SayArgv( 1 + skiparg, name, sizeof( name ) );
	G_SayArgv( 2 + skiparg, team, sizeof( team ) );

	if ( G_SayArgc() < 3 + skiparg )
	{
		ADMP( "^/putteam usage: ^7!putteam [name] [r|b|s]" );
		return qfalse;
	}

	G_SayArgv( 1 + skiparg, name, sizeof( name ) );
	G_SayArgv( 2 + skiparg, team, sizeof( team ) );

	if ( G_ClientNumbersFromString( name, pids ) != 1 )
	{
		G_MatchOnePlayer( pids, err, sizeof( err ) );
		ADMP( va( "^/putteam: ^7%s", err ) );
		return qfalse;
	}

	vic = &g_entities[ pids[ 0 ] ];

	if ( !admin_higher( ent, vic ) )
	{
		ADMP( "^/putteam: ^7sorry, but your intended victim has a higher "
		      " admin level than you do" );
		return qfalse;
	}

	if ( !SetTeam( vic, team, qtrue, -1, -1, qfalse ) )
	{
		ADMP( "^/putteam: ^7SetTeam failed" );
		return qfalse;
	}

	if ( !Q_stricmp( team, "red" )
	     || !Q_stricmp( team, "r" )
	     || !Q_stricmp( team, "axis" )
	     || !Q_stricmp( team, "blue" )
	     || !Q_stricmp( team, "b" )
	     || !Q_stricmp( team, "allies" ) )
	{
	}

	return qtrue;
}

qboolean G_admin_mute( gentity_t *ent, int skiparg )
{
	int       pids[ MAX_CLIENTS ];
	char      name[ MAX_NAME_LENGTH ], err[ MAX_STRING_CHARS ];
	char      command[ MAX_ADMIN_CMD_LEN ], *cmd;
	gentity_t *vic;

	if ( G_SayArgc() < 2 + skiparg )
	{
		ADMP( "^3!mute: ^7usage: mute [name|slot#]\n" );
		return qfalse;
	}

	G_SayArgv( skiparg, command, sizeof( command ) );
	cmd = command;

	if ( cmd && *cmd == '!' )
	{
		cmd++;
	}

	G_SayArgv( 1 + skiparg, name, sizeof( name ) );

	if ( G_ClientNumbersFromString( name, pids ) != 1 )
	{
		G_MatchOnePlayer( pids, err, sizeof( err ) );
		ADMP( va( "^3!mute: ^7%s\n", err ) );
		return qfalse;
	}

	if ( !admin_higher( ent, &g_entities[ pids[ 0 ] ] ) )
	{
		ADMP( "^3!mute: ^7sorry, but your intended victim has a higher admin"
		      " level than you\n" );
		return qfalse;
	}

	vic = &g_entities[ pids[ 0 ] ];

	if ( vic->client->pers.muted == qtrue )
	{
		if ( !Q_stricmp( cmd, "mute" ) )
		{
			ADMP( "^3!mute: ^7player is already muted\n" );
			return qtrue;
		}

		vic->client->pers.muted = qfalse;
		CPx( pids[ 0 ], "cp \"^1You have been unmuted\"" );
		AP( va( "print \"^3!unmute: ^7%s^7 has been unmuted by %s\n\"",
		        vic->client->pers.netname,
		        ( ent ) ? ent->client->pers.netname : "console" ) );
	}
	else
	{
		if ( !Q_stricmp( cmd, "unmute" ) )
		{
			ADMP( "^3!unmute: ^7player is not currently muted\n" );
			return qtrue;
		}

		vic->client->pers.muted = qtrue;
		CPx( pids[ 0 ], "cp \"^1You've been muted\"" );
		AP( va( "print \"^3!mute: ^7%s^7 has been muted by ^7%s\n\"",
		        vic->client->pers.netname,
		        ( ent ) ? ent->client->pers.netname : "console" ) );
	}

	ClientUserinfoChanged( pids[ 0 ] );
	return qtrue;
}

qboolean G_admin_listadmins( gentity_t *ent, int skiparg )
{
	int      i, found = 0;
	char     search[ MAX_NAME_LENGTH ] = { "" };
	char     s[ MAX_NAME_LENGTH ]      = { "" };
	int      start                     = 0;
	qboolean numeric                   = qtrue;
	int      drawn                     = 0;

	for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
	{
		if ( g_admin_admins[ i ]->level == 0 )
		{
			continue;
		}

		found++;
	}

	if ( !found )
	{
		ADMP( "^3!listadmins: ^7no admins defined\n" );
		return qfalse;
	}

	if ( G_SayArgc() == 2 + skiparg )
	{
		G_SayArgv( 1 + skiparg, s, sizeof( s ) );

		for ( i = 0; i < sizeof( s ) && s[ i ]; i++ )
		{
			if ( isdigit( s[ i ] ) )
			{
				continue;
			}

			numeric = qfalse;
		}

		if ( numeric )
		{
			start = atoi( s );

			if ( start > 0 )
			{
				start -= 1;
			}
			else if ( start < 0 )
			{
				start = found + start;
			}
		}
		else
		{
			G_SanitiseString( s, search, sizeof( search ) );
		}
	}

	if ( start >= found || start < 0 )
	{
		start = 0;
	}

	if ( start >= found )
	{
		ADMP( va( "^3!listadmins: ^7listing %d admins\n", found ) );
		return qfalse;
	}

	drawn = admin_listadmins( ent, start, search );

	if ( search[ 0 ] )
	{
		ADMP( va( "^3!listadmins:^7 found %d admins matching '%s^7'\n",
		          drawn, search ) );
	}
	else
	{
		ADMBP_begin();
		ADMBP( va( "^3!listadmins:^7 showing admin %d - %d of %d.  ",
		           ( found ) ? ( start + 1 ) : 0,
		           ( ( start + MAX_ADMIN_LISTITEMS ) > found ) ?
		           found : ( start + MAX_ADMIN_LISTITEMS ),
		           found ) );

		if ( ( start + MAX_ADMIN_LISTITEMS ) < found )
		{
			ADMBP( va( "run '!listadmins %d' to see more",
			           ( start + MAX_ADMIN_LISTITEMS + 1 ) ) );
		}

		ADMBP( "\n" );
		ADMBP_end();
	}

	return qtrue;
}

qboolean G_admin_listplayers( gentity_t *ent, int skiparg )
{
	int       i, j;
	gclient_t *p;
	char      c[ 3 ], t[ 2 ]; // color and team letter
	char      n[ MAX_NAME_LENGTH ]  = { "" };
	char      n2[ MAX_NAME_LENGTH ] = { "" };
	char      n3[ MAX_NAME_LENGTH ] = { "" };
	char      lname[ MAX_NAME_LENGTH ];
	char      lname2[ MAX_NAME_LENGTH ];
	char      guid_stub[ 9 ];
	char      muted[ 2 ];
	int       l;
	int       lname_max = 1;
	char      lname_fmt[ 5 ];

// cs: todo: commented now for compiler warnings. seems to be from etpub extended function, but not implemented
//	char cleanLevelName[MAX_NAME_LENGTH];
//	int spaces;

	// detect the longest level name length
	for ( i = 0; g_admin_levels[ i ]; i++ )
	{
		G_DecolorString( g_admin_levels[ i ]->name, n );

		if ( strlen( n ) > lname_max )
		{
			lname_max = strlen( n );
		}

		Com_sprintf( n, sizeof( n ), "%d", g_admin_levels[ i ]->level );

		if ( strlen( n ) > lname_max )
		{
			lname_max = strlen( n );
		}
	}

	ADMBP_begin();
	ADMBP( va( "^/listplayers: ^7%d players connected:\n",
	           level.numConnectedClients ) );

	for ( i = 0; i < level.maxclients; i++ )
	{
		p = &level.clients[ i ];
		Q_strncpyz( t, "S", sizeof( t ) );
		Q_strncpyz( c, S_COLOR_YELLOW, sizeof( c ) );

		if ( p->sess.sessionTeam == TEAM_ALLIES )
		{
			Q_strncpyz( t, "B", sizeof( t ) );
			Q_strncpyz( c, S_COLOR_BLUE, sizeof( c ) );
		}
		else if ( p->sess.sessionTeam == TEAM_AXIS )
		{
			Q_strncpyz( t, "R", sizeof( t ) );
			Q_strncpyz( c, S_COLOR_RED, sizeof( c ) );
		}

		if ( p->pers.connected == CON_CONNECTING )
		{
			Q_strncpyz( t, "C", sizeof( t ) );
			Q_strncpyz( c, S_COLOR_ORANGE, sizeof( c ) );
		}
		else if ( p->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		for ( j = 0; j <= 8; j++ )
		{
			guid_stub[ j ] = p->pers.guid[ j + 24 ];
		}

		guid_stub[ j ] = '\0';

		muted[ 0 ]     = '\0';

		if ( p->pers.muted )
		{
			Q_strncpyz( muted, "M", sizeof( muted ) );
		}

		l      = 0;
		G_SanitiseName( p->pers.netname, n2 );
		n[ 0 ] = '\0';

		for ( j = 0; j < MAX_ADMIN_ADMINS && g_admin_admins[ j ]; j++ )
		{
			if ( !Q_stricmp( g_admin_admins[ j ]->guid, p->pers.guid ) )
			{
				// don't gather aka or level info if the admin is incognito
				if ( G_admin_permission( &g_entities[ i ], ADMF_INCOGNITO ) )
				{
					break;
				}

				l = g_admin_admins[ j ]->level;
				G_SanitiseName( g_admin_admins[ j ]->name, n3 );

				if ( Q_stricmp( n2, n3 ) )
				{
					Q_strncpyz( n, g_admin_admins[ j ]->name, sizeof( n ) );
				}

				break;
			}
		}

		lname[ 0 ] = '\0';
		Q_strncpyz( lname_fmt, "%s", sizeof( lname_fmt ) );

		for ( j = 0; j < MAX_ADMIN_LEVELS && g_admin_levels[ j ]; j++ )
		{
			if ( g_admin_levels[ j ]->level == l )
			{
				Q_strncpyz( lname, g_admin_levels[ j ]->name, sizeof( lname ) );

				if ( *lname )
				{
					G_DecolorString( lname, lname2 );
					Com_sprintf( lname_fmt, sizeof( lname_fmt ), "%%%" PRIuPTR "s",
					             ( admin_level_maxname + strlen( lname ) - strlen( lname2 ) ) );
					Com_sprintf( lname2, sizeof( lname2 ), lname_fmt, lname );
				}

				break;
			}
		}

		ADMBP( va( "%2i %s%s^7 %-2i %s^7 (*%s) ^1%1s^7 %s^7 %s%s^7%s\n",
		           i,
		           c,
		           t,
		           l,
		           ( *lname ) ? lname2 : "",
		           guid_stub,
		           muted,
		           p->pers.netname,
		           ( *n ) ? "(a.k.a. " : "",
		           n,
		           ( *n ) ? ")" : ""
		         ) );
	}

	ADMBP_end();
	return qtrue;
}

qboolean G_admin_showbans( gentity_t *ent, int skiparg )
{
	int      i, found = 0;
	int      max                       = -1, count;
	int      t;
	char     duration[ 32 ];
	int      max_name                  = 1, max_banner = 1, colorlen;
	int      len;
	int      secs;
	int      start                     = 0;
	char     filter[ MAX_NAME_LENGTH ] = { "" };
	char     date[ 11 ];
	char     *made;
	int      j, k;
	char     n1[ MAX_NAME_LENGTH * 2 ]     = { "" };
	char     n2[ MAX_NAME_LENGTH * 2 ]     = { "" };
	qboolean numeric                       = qtrue;
	char     *ip_match                     = NULL;
	int      ip_match_len                  = 0;
	char     name_match[ MAX_NAME_LENGTH ] = { "" };

	t = trap_RealTime( NULL );

	for ( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
	{
		if ( g_admin_bans[ i ]->expires != 0 &&
		     ( g_admin_bans[ i ]->expires - t ) < 1 )
		{
			continue;
		}

		found++;
		max = i;
	}

	if ( max < 0 )
	{
		ADMP( "^3!showbans: ^7no bans to display\n" );
		return qfalse;
	}

	if ( G_SayArgc() >= 2 + skiparg )
	{
		G_SayArgv( 1 + skiparg, filter, sizeof( filter ) );

		if ( G_SayArgc() >= 3 + skiparg )
		{
			start = atoi( filter );
			G_SayArgv( 2 + skiparg, filter, sizeof( filter ) );
		}

		for ( i = 0; i < sizeof( filter ) && filter[ i ]; i++ )
		{
			if ( !isdigit( filter[ i ] ) &&
			     filter[ i ] != '.' && filter[ i ] != '-' )
			{
				numeric = qfalse;
				break;
			}
		}

		if ( !numeric )
		{
			G_SanitiseString( filter, name_match, sizeof( name_match ) );
		}
		else if ( strchr( filter, '.' ) )
		{
			ip_match     = filter;
			ip_match_len = strlen( ip_match );
		}
		else
		{
			start       = atoi( filter );
			filter[ 0 ] = '\0';
		}

		// showbans 1 means start with ban 0
		if ( start > 0 )
		{
			start--;
		}
		else if ( start < 0 )
		{
			for ( i = max, count = 0; i >= 0 && count < -start; i-- )
			{
				if ( g_admin_bans[ i ]->expires == 0 ||
				     ( g_admin_bans[ i ]->expires - t ) > 0 )
				{
					count++;
				}
			}

			start = i + 1;
		}
	}

	if ( start < 0 )
	{
		start = 0;
	}

	if ( start > max )
	{
		ADMP( va( "^3!showbans: ^7%d is the last valid ban\n", max + 1 ) );
		return qfalse;
	}

	for ( i = start, count = 0; i <= max && count < MAX_ADMIN_SHOWBANS; i++ )
	{
		if ( g_admin_bans[ i ]->expires != 0 &&
		     ( g_admin_bans[ i ]->expires - t ) < 1 )
		{
			continue;
		}

		if ( name_match[ 0 ] )
		{
			G_SanitiseString( g_admin_bans[ i ]->name, n1, sizeof( n1 ) );

			if ( !strstr( n1, name_match ) )
			{
				continue;
			}
		}

		if ( ip_match &&
		     Q_strncmp( ip_match, g_admin_bans[ i ]->ip, ip_match_len ) )
		{
			continue;
		}

		count++;

		len = Q_PrintStrlen( g_admin_bans[ i ]->name );

		if ( len > max_name )
		{
			max_name = len;
		}

		len = Q_PrintStrlen( g_admin_bans[ i ]->banner );

		if ( len > max_banner )
		{
			max_banner = len;
		}
	}

	ADMBP_begin();

	for ( i = start, count = 0; i <= max && count < MAX_ADMIN_SHOWBANS; i++ )
	{
		if ( g_admin_bans[ i ]->expires != 0 &&
		     ( g_admin_bans[ i ]->expires - t ) < 1 )
		{
			continue;
		}

		if ( name_match[ 0 ] )
		{
			G_SanitiseString( g_admin_bans[ i ]->name, n1, sizeof( n1 ) );

			if ( !strstr( n1, name_match ) )
			{
				continue;
			}
		}

		if ( ip_match &&
		     Q_strncmp( ip_match, g_admin_bans[ i ]->ip, ip_match_len ) )
		{
			continue;
		}

		count++;

		// only print out the the date part of made
		date[ 0 ] = '\0';
		made      = g_admin_bans[ i ]->made;

		for ( j = 0; made && *made; j++ )
		{
			if ( ( j + 1 ) >= sizeof( date ) )
			{
				break;
			}

			if ( *made == ' ' )
			{
				break;
			}

			date[ j ]     = *made;
			date[ j + 1 ] = '\0';
			made++;
		}

		secs = ( g_admin_bans[ i ]->expires - t );
		G_admin_duration( secs, duration, sizeof( duration ) );

		for ( colorlen = k = 0; g_admin_bans[ i ]->name[ k ]; k++ )
		{
			if ( Q_IsColorString( &g_admin_bans[ i ]->name[ k ] ) )
			{
				colorlen += 2;
			}
		}

		Com_sprintf( n1, sizeof( n1 ), "%*s", max_name + colorlen,
		             g_admin_bans[ i ]->name );

		for ( colorlen = k = 0; g_admin_bans[ i ]->banner[ k ]; k++ )
		{
			if ( Q_IsColorString( &g_admin_bans[ i ]->banner[ k ] ) )
			{
				colorlen += 2;
			}
		}

		Com_sprintf( n2, sizeof( n2 ), "%*s", max_banner + colorlen,
		             g_admin_bans[ i ]->banner );

		ADMBP( va( "%4i %s^7 %-15s %-8s %s^7 %-10s\n     \\__ %s\n",
		           ( i + 1 ),
		           n1,
		           g_admin_bans[ i ]->ip,
		           date,
		           n2,
		           duration,
		           g_admin_bans[ i ]->reason ) );
	}

	if ( name_match[ 0 ] || ip_match )
	{
		ADMBP( va( "^3!showbans:^7 found %d matching bans by %s.  ",
		           count,
		           ( ip_match ) ? "IP" : "name" ) );
	}
	else
	{
		ADMBP( va( "^3!showbans:^7 showing bans %d - %d of %d (%d total).",
		           ( found ) ? ( start + 1 ) : 0,
		           i,
		           max + 1,
		           found ) );
	}

	if ( i <= max )
	{
		ADMBP( va( "  run !showbans %d%s%s to see more",
		           i + 1,
		           ( filter[ 0 ] ) ? " " : "",
		           ( filter[ 0 ] ) ? filter : "" ) );
	}

	ADMBP( "\n" );
	ADMBP_end();
	return qtrue;
}

qboolean G_admin_help( gentity_t *ent, int skiparg )
{
	int i;

	if ( G_SayArgc() < 2 + skiparg )
	{
		int j     = 0;
		int count = 0;

		ADMBP_begin();

		for ( i = 0; i < adminNumCmds; i++ )
		{
			if ( G_admin_permission( ent, g_admin_cmds[ i ].flag[ 0 ] ) )
			{
				ADMBP( va( "^3!%-12s", g_admin_cmds[ i ].keyword ) );
				j++;
				count++;
			}

			// show 6 commands per line
			if ( j == 6 )
			{
				ADMBP( "\n" );
				j = 0;
			}
		}

		for ( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
		{
			if ( !admin_command_permission( ent, g_admin_commands[ i ]->command ) )
			{
				continue;
			}

			ADMBP( va( "^3!%-12s", g_admin_commands[ i ]->command ) );
			j++;
			count++;

			// show 6 commands per line
			if ( j == 6 )
			{
				ADMBP( "\n" );
				j = 0;
			}
		}

		if ( count )
		{
			ADMBP( "\n" );
		}

		ADMBP( va( "^3!help: ^7%i available commands\n", count ) );
		ADMBP( "run !help [^3command^7] for help with a specific command.\n" );
		ADMBP_end();

		return qtrue;
	}
	else
	{
		//!help param
		char param[ MAX_ADMIN_CMD_LEN ];
		char *cmd;

		G_SayArgv( 1 + skiparg, param, sizeof( param ) );
		cmd = ( param[ 0 ] == '!' ) ? &param[ 1 ] : &param[ 0 ];
		ADMBP_begin();

		for ( i = 0; i < adminNumCmds; i++ )
		{
			if ( !Q_stricmp( cmd, g_admin_cmds[ i ].keyword ) )
			{
				if ( !G_admin_permission( ent, g_admin_cmds[ i ].flag[ 0 ] ) )
				{
					ADMBP( va( "^3!help: ^7you have no permission to use '%s'\n",
					           g_admin_cmds[ i ].keyword ) );
					return qfalse;
				}

				ADMBP( va( "^3!help: ^7help for '!%s':\n",
				           g_admin_cmds[ i ].keyword ) );
				ADMBP( va( " ^3Funtion: ^7%s\n", g_admin_cmds[ i ].function ) );
				ADMBP( va( " ^3Syntax: ^7%s %s\n", g_admin_cmds[ i ].keyword,
				           g_admin_cmds[ i ].syntax ) );
				ADMBP( va( " ^3Flag: ^7'%c'\n", g_admin_cmds[ i ].flag[ 0 ] ) );
				ADMBP_end();
				return qtrue;
			}
		}

		for ( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
		{
			if ( !Q_stricmp( cmd, g_admin_commands[ i ]->command ) )
			{
				if ( !admin_command_permission( ent, g_admin_commands[ i ]->command ) )
				{
					ADMBP( va( "^3!help: ^7you have no permission to use '%s'\n",
					           g_admin_commands[ i ]->command ) );
					ADMBP_end();
					return qfalse;
				}

				ADMBP( va( "^3!help: ^7help for '%s':\n",
				           g_admin_commands[ i ]->command ) );
				ADMBP( va( " ^3Description: ^7%s\n", g_admin_commands[ i ]->desc ) );
				ADMBP( va( " ^3Syntax: ^7%s\n", g_admin_commands[ i ]->command ) );
				ADMBP_end();
				return qtrue;
			}
		}

		ADMBP( va( "^3!help: ^7no help found for '%s'\n", cmd ) );
		ADMBP_end();
		return qfalse;
	}
}

qboolean G_admin_admintest( gentity_t *ent, int skiparg )
{
	int      i, l = 0;
	qboolean found = qfalse;
	qboolean lname = qfalse;

	if ( !ent )
	{
		ADMP( "^3!admintest: ^7you are on the console.\n" );
		return qtrue;
	}

	for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
	{
		if ( !Q_stricmp( g_admin_admins[ i ]->guid, ent->client->pers.guid ) )
		{
			found = qtrue;
			break;
		}
	}

	if ( found )
	{
		l = g_admin_admins[ i ]->level;

		for ( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
		{
			if ( g_admin_levels[ i ]->level != l )
			{
				continue;
			}

			if ( *g_admin_levels[ i ]->name )
			{
				lname = qtrue;
				break;
			}
		}
	}

	AP( va( "print \"^3!admintest: ^7%s^7 is a level %d admin %s%s^7%s\n\"",
	        ent->client->pers.netname,
	        l,
	        ( lname ) ? "(" : "",
	        ( lname ) ? g_admin_levels[ i ]->name : "",
	        ( lname ) ? ")" : "" ) );
	return qtrue;
}

qboolean G_admin_cancelvote( gentity_t *ent, int skiparg )
{
	if ( level.voteInfo.voteTime )
	{
		level.voteInfo.voteYes = 0;
		level.voteInfo.voteNo  = level.numConnectedClients;
		CheckVote( qtrue );
		AP( "^/cancelvote: ^7turns out everyone voted No" );
	}
	else
	{
		AP( "^/cancelvote: ^7No vote in progress" );
	}

	return qtrue;
}

qboolean G_admin_passvote( gentity_t *ent, int skiparg )
{
	if ( level.voteInfo.voteTime )
	{
		level.voteInfo.voteYes = level.numConnectedClients;
		level.voteInfo.voteNo  = 0;
		CheckVote( qtrue );
		AP( "^/passvote: ^7turns out everyone voted Yes" );
	}
	else
	{
		AP( "^/passvote: ^7No vote in progress" );
	}

	return qtrue;
}

qboolean G_admin_spec999( gentity_t *ent, int skiparg )
{
	int       i, movedtospec = 0;
	gentity_t *vic;

	for ( i = 0; i < level.maxclients; i++ )
	{
		vic = &g_entities[ i ];

		if ( !vic->client ) { continue; }

		if ( vic->client->pers.connected != CON_CONNECTED ) { continue; }

		if ( vic->client->ps.ping == 999 )
		{
			SetTeam( vic, "s", qtrue, -1, -1, qfalse );
			AP( va( "chat \"^/spec999: ^7%s^7 moved to spectators\" -1",
			        vic->client->pers.netname ) );
			movedtospec++;
		}
	}

	if ( !movedtospec )
	{
		AP( "chat \"^/spec999: ^7No clients moved to spectators\" -1" );
	}

	return qtrue;
}

qboolean G_admin_register( gentity_t *ent, int skiparg )
{
	int level = 0;

	level = G_admin_level( ent );

	if ( level == 0 )
	{
		level = 1;
	}

	if ( !Q_stricmp( ent->client->pers.guid, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) )
	{
		ADMP( va( "^3!register: ^7 You cannot register for name protection until you get GUIDs.\n" ) );
		return qfalse;
	}

	trap_SendConsoleCommand( EXEC_APPEND, va( "!setlevel %ld %d;", ( long )( ent - g_entities ), level ) );
	ClientUserinfoChanged( ent - g_entities );

	AP( va( "print \"^3!register: ^7%s^7 is now a protected nickname.\n\"", ent->client->pers.netname ) );

	return qtrue;
}

qboolean G_admin_autoregister( gentity_t *ent )
{
	int level;

	if ( !g_autoRegister.integer )
	{
		return qtrue;
	}

	level = 0;

	level = G_admin_level( ent );

	if ( level == 0 )
	{
		level = 1;
	}

	if ( !Q_stricmp( ent->client->pers.guid, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) )
	{
		return qfalse;
	}

	trap_SendConsoleCommand( EXEC_APPEND, va( "!setlevel %ld %d;", ( long )( ent - g_entities ), level ) );
	ClientUserinfoChanged( ent - g_entities );

	AP( va( "print \"^3!register: ^7%s^7 is now a protected nickname.\n\"", ent->client->pers.netname ) );

	return qtrue;
}

qboolean G_admin_rename( gentity_t *ent, int skiparg )
{
	int       pids[ MAX_CLIENTS ];
	char      name[ MAX_NAME_LENGTH ];
	char      newname[ MAX_NAME_LENGTH ];
	char      oldname[ MAX_NAME_LENGTH ];
	char      err[ MAX_STRING_CHARS ];
	char      userinfo[ MAX_INFO_STRING ];
	char      *s;
	gentity_t *victim = NULL;

	if ( G_SayArgc() < 3 + skiparg )
	{
		ADMP( "^3!rename: ^7usage: rename [name] [newname]\n" );
		return qfalse;
	}

	G_SayArgv( 1 + skiparg, name, sizeof( name ) );
	s = G_SayConcatArgs( 2 + skiparg );
	Q_strncpyz( newname, s, sizeof( newname ) );

	if ( G_ClientNumbersFromString( name, pids ) != 1 )
	{
		G_MatchOnePlayer( pids, err, sizeof( err ) );
		ADMP( va( "^3!rename: ^7%s\n", err ) );
		return qfalse;
	}

	victim = &g_entities[ pids[ 0 ] ];

	if ( !admin_higher( ent, victim ) )
	{
		ADMP( "^3!rename: ^7sorry, but your intended victim has a higher admin"
		      " level than you\n" );
		return qfalse;
	}

	if ( !G_admin_name_check( victim, newname, err, sizeof( err ) ) )
	{
		ADMP( va( "^3!rename: ^7%s\n", err ) );
		return qfalse;
	}

	level.clients[ pids[ 0 ] ].pers.nameChanges--;
	level.clients[ pids[ 0 ] ].pers.nameChangeTime = 0;
	trap_GetUserinfo( pids[ 0 ], userinfo, sizeof( userinfo ) );
	s                                              = Info_ValueForKey( userinfo, "name" );
	Q_strncpyz( oldname, s, sizeof( oldname ) );
	Info_SetValueForKey( userinfo, "name", newname );
	trap_SetUserinfo( pids[ 0 ], userinfo );
	ClientUserinfoChanged( pids[ 0 ] );
	AP( va( "print \"^3!rename: ^7%s^7 has been renamed to %s^7 by %s\n\"",
	        oldname,
	        newname,
	        ( ent ) ? ent->client->pers.netname : "console" ) );
	return qtrue;
}

qboolean G_admin_restart( gentity_t *ent, int skiparg )
{
	char command[ MAX_ADMIN_CMD_LEN ];

	G_SayArgv( skiparg, command, sizeof( command ) );
	trap_SendConsoleCommand( EXEC_APPEND, "map_restart" );
	AP( va( "print \"^3!restart: ^7map restarted by %s\n\"",
	        ( ent ) ? ent->client->pers.netname : "console" ) );
	return qtrue;
}

qboolean G_admin_nextmap( gentity_t *ent, int skiparg )
{
	// copied from G_Nextmap_v() in g_vote.c

	g_campaignInfo_t *campaign;

	if ( g_gametype.integer == GT_WOLF_CAMPAIGN )
	{
		campaign = &g_campaigns[ level.currentCampaign ];

		if ( campaign->current + 1 < campaign->mapCount )
		{
			trap_Cvar_Set( "g_currentCampaignMap",
			               va( "%i", campaign->current + 1 ) );
			trap_SendConsoleCommand( EXEC_APPEND,
			                         va( "map %s\n",
			                             campaign->mapnames[ campaign->current + 1 ] ) );
			AP( "cp \"^3*** Loading next map in campaign! ***\n\"" );
		}
		else
		{
			// Load in the nextcampaign
			trap_SendConsoleCommand( EXEC_APPEND,
			                         "vstr nextcampaign\n" );
			AP( "cp \"^3*** Loading nextcampaign! ***\n\"" );
		}
	}
	else
	{
		// Load in the nextmap
		trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
		AP( "cp \"^3*** Loading nextmap! ***\n\"" );
	}

	AP( "chat \"^/nextmap: ^7Next map was loaded\" -1" );
	return qtrue;
}

qboolean G_admin_namelog( gentity_t *ent, int skiparg )
{
	int      i, j;
	char     search[ MAX_NAME_LENGTH ] = { "" };
	char     s2[ MAX_NAME_LENGTH ]     = { "" };
	char     n2[ MAX_NAME_LENGTH ]     = { "" };
	char     guid_stub[ 9 ];
	qboolean found                     = qfalse;
	int      printed                   = 0;

	if ( G_SayArgc() > 1 + skiparg )
	{
		G_SayArgv( 1 + skiparg, search, sizeof( search ) );
		G_SanitiseString( search, s2, sizeof( s2 ) );
	}

	ADMBP_begin();

	for ( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
	{
		if ( search[ 0 ] )
		{
			found = qfalse;

			for ( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
			      g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
			{
				G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );

				if ( strstr( n2, s2 ) )
				{
					found = qtrue;
					break;
				}
			}

			if ( !found )
			{
				continue;
			}
		}

		printed++;

		for ( j = 0; j < 8; j++ )
		{
			guid_stub[ j ] = g_admin_namelog[ i ]->guid[ j + 24 ];
		}

		guid_stub[ j ] = '\0';

		if ( g_admin_namelog[ i ]->slot > -1 )
		{
			ADMBP( "^3" );
		}

		ADMBP( va( "%-2s (*%s) %15s^7",
		           ( g_admin_namelog[ i ]->slot > -1 ) ?
		           va( "%d", g_admin_namelog[ i ]->slot ) : "-",
		           guid_stub, g_admin_namelog[ i ]->ip ) );

		for ( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
		      g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
		{
			ADMBP( va( " '%s^7'", g_admin_namelog[ i ]->name[ j ] ) );
		}

		ADMBP( "\n" );
	}

	ADMBP( va( "^3!namelog:^7 %d recent clients found\n", printed ) );
	ADMBP_end();
	return qtrue;
}

/*
 * This function facilitates the TP define.  ADMP() is similar to CP except that
 * it prints the message to the server console if ent is not defined.
 */
void G_admin_print( gentity_t *ent, char *m )
{
	if ( ent )
	{
		trap_SendServerCommand( ent - level.gentities, va( "print \"%s\"", m ) );
	}
	else
	{
		char m2[ MAX_STRING_CHARS ];
		G_DecolorString( m, m2 );
		G_Printf( "%s", m2 );
	}
}

void G_admin_buffer_begin()
{
	g_bfb[ 0 ] = '\0';
}

void G_admin_buffer_end( gentity_t *ent )
{
	ADMP( g_bfb );
}

void G_admin_buffer_print( gentity_t *ent, char *m )
{
	// 1022 - strlen("print 64 \"\"") - 1
	if ( strlen( m ) + strlen( g_bfb ) >= 1009 )
	{
		ADMP( g_bfb );
		g_bfb[ 0 ] = '\0';
	}

	Q_strcat( g_bfb, sizeof( g_bfb ), m );
}

void G_admin_cleanup()
{
	int i = 0;

	for ( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
	{
		G_Free( g_admin_levels[ i ] );
		g_admin_levels[ i ] = NULL;
	}

	for ( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
	{
		G_Free( g_admin_admins[ i ] );
		g_admin_admins[ i ] = NULL;
	}

	for ( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
	{
		G_Free( g_admin_bans[ i ] );
		g_admin_bans[ i ] = NULL;
	}

	for ( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
	{
		G_Free( g_admin_commands[ i ] );
		g_admin_commands[ i ] = NULL;
	}
}
