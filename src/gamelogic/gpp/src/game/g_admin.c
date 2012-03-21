/*
===========================================================================
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

This shrubbot implementation is the original work of Tony J. White.

Contains contributions from Wesley van Beelen, Chris Bajumpaa, Josh Menke,
and Travis Maurer.

The functionality of this code mimics the behaviour of the currently
inactive project shrubet (http://www.etstats.com/shrubet/index.php?ver=2)
by Ryan Mannion.   However, shrubet was a closed-source project and
none of it's code has been copied, only it's functionality.

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

#include "g_local.h"

// big ugly global buffer for use with buffered printing of long outputs
static char       g_bfb[ 32000 ];

// note: list ordered alphabetically
g_admin_cmd_t     g_admin_cmds[] =
{
	{
		"adjustban",    G_admin_adjustban,   qfalse, "ban",
		"change the duration or reason of a ban.  duration is specified as "
		"numbers followed by units 'w' (weeks), 'd' (days), 'h' (hours) or "
		"'m' (minutes), or seconds if no units are specified.  if the duration is"
		" preceded by a + or -, the ban duration will be extended or shortened by"
		" the specified amount",
		"[^3ban#^7] (^5/mask^7) (^5duration^7) (^5reason^7)"
	},

	{
		"adminhelp",    G_admin_adminhelp,   qtrue,  "adminhelp",
		"display admin commands available to you or help on a specific command",
		"(^5command^7)"
	},

	{
		"admintest",    G_admin_admintest,   qfalse, "admintest",
		"display your current admin level",
		""
	},

	{
		"allowbuild",   G_admin_denybuild,   qfalse, "denybuild",
		"restore a player's ability to build",
		"[^3name|slot#^7]"
	},

	{
		"allready",     G_admin_allready,    qfalse, "allready",
		"makes everyone ready in intermission",
		""
	},

	{
		"ban",          G_admin_ban,         qfalse, "ban",
		"ban a player by IP and GUID with an optional expiration time and reason."
		" duration is specified as numbers followed by units 'w' (weeks), 'd' "
		"(days), 'h' (hours) or 'm' (minutes), or seconds if no units are "
		"specified",
		"[^3name|slot#|IP(/mask)^7] (^5duration^7) (^5reason^7)"
	},
	{
		"bot",          G_admin_bot,         qfalse, "bot",
		"Add/Delete bots",
		"[^5add|del^7] [^5name^7] [^5aliens/humans^7] (^5skill^7)"
	},
	{
		"builder",      G_admin_builder,     qtrue,  "builder",
		"show who built a structure",
		""
	},

	{
		"buildlog",     G_admin_buildlog,    qfalse, "buildlog",
		"show buildable log",
		"(^5name|slot#^7) (^5id^7)"
	},

	{
		"cancelvote",   G_admin_endvote,     qfalse, "cancelvote",
		"cancel a vote taking place",
		"(^5a|h^7)"
	},

	{
		"changemap",    G_admin_changemap,   qfalse, "changemap",
		"load a map (and optionally force layout)",
		"[^3mapname^7] (^5layout^7)"
	},

	{
		"denybuild",    G_admin_denybuild,   qfalse, "denybuild",
		"take away a player's ability to build",
		"[^3name|slot#^7]"
	},

	{
		"kick",         G_admin_kick,        qfalse, "kick",
		"kick a player with an optional reason",
		"[^3name|slot#^7] (^5reason^7)"
	},

	{
		"l0",           G_admin_l0,          qfalse, "l0",
		"remove name protection from a player by setting them to admin level 0",
		"[^3name|slot#|admin#^7]"
	},

	{
		"l1",           G_admin_l1,          qfalse, "l1",
		"give a player name protection by setting them to admin level 1",
		"[^3name|slot#^7]"
	},

	{
		"listadmins",   G_admin_listadmins,  qtrue,  "listadmins",
		"display a list of all server admins and their levels",
		"(^5name^7) (^5start admin#^7)"
	},

	{
		"listlayouts",  G_admin_listlayouts, qtrue,  "listlayouts",
		"display a list of all available layouts for a map",
		"(^5mapname^7)"
	},

	{
		"listmaps",     NULL,                qtrue,  NULL,
		"display a list of available maps on the server",
		"(^5mapname^7)"
	},

	{
		"listplayers",  G_admin_listplayers, qtrue,  "listplayers",
		"display a list of players, their client numbers and their levels",
		""
	},

	{
		"listrotation", NULL,                qtrue,  NULL,
		"display the active map rotation",
		""
	},

	{
		"lock",         G_admin_lock,        qfalse, "lock",
		"lock a team to prevent anyone from joining it",
		"[^3a|h^7]"
	},

	{
		"mute",         G_admin_mute,        qfalse, "mute",
		"mute a player",
		"[^3name|slot#^7]"
	},

	{
		"namelog",      G_admin_namelog,     qtrue,  "namelog",
		"display a list of names used by recently connected players",
		"(^5name|IP(/mask)^7) (start namelog#)"
	},

	{
		"nextmap",      G_admin_nextmap,     qfalse, "nextmap",
		"go to the next map in the cycle",
		""
	},

	{
		"passvote",     G_admin_endvote,     qfalse, "passvote",
		"pass a vote currently taking place",
		"(^5a|h^7)"
	},

	{
		"pause",        G_admin_pause,       qfalse, "pause",
		"Pause (or unpause) the game.",
		""
	},

	{
		"putteam",      G_admin_putteam,     qfalse, "putteam",
		"move a player to a specified team",
		"[^3name|slot#^7] [^3h|a|s^7]"
	},

	{
		"readconfig",   G_admin_readconfig,  qfalse, "readconfig",
		"reloads the admin config file and refreshes permission flags",
		""
	},

	{
		"register",     G_admin_register,    qfalse, "register",
		"register your name to protect it from being used by other players.",
		""
	},

	{
		"rename",       G_admin_rename,      qfalse, "rename",
		"rename a player",
		"[^3name|slot#^7] [^3new name^7]"
	},

	{
		"restart",      G_admin_restart,     qfalse, "restart",
		"restart the current map (optionally using named layout or keeping/switching teams)",
		"(^5layout^7) (^5keepteams|switchteams|keepteamslock|switchteamslock^7)"
	},

	{
		"revert",       G_admin_revert,      qfalse, "revert",
		"revert buildables to a given time",
		"[^3id^7]"
	},

	{
		"setlevel",     G_admin_setlevel,    qfalse, "setlevel",
		"sets the admin level of a player",
		"[^3name|slot#|admin#^7] [^3level^7]"
	},

	{
		"showbans",     G_admin_showbans,    qtrue,  "showbans",
		"display a (partial) list of active bans",
		"(^5name|IP(/mask)^7) (^5start at ban#^7)"
	},

	{
		"spec999",      G_admin_spec999,     qfalse, "spec999",
		"move 999 pingers to the spectator team",
		""
	},

	{
		"time",         G_admin_time,        qtrue,  "time",
		"show the current local server time",
		""
	},

	{
		"unban",        G_admin_unban,       qfalse, "ban",
		"unbans a player specified by the slot as seen in showbans",
		"[^3ban#^7]"
	},

	{
		"unlock",       G_admin_lock,        qfalse, "lock",
		"unlock a locked team",
		"[^3a|h^7]"
	},

	{
		"unmute",       G_admin_mute,        qfalse, "mute",
		"unmute a muted player",
		"[^3name|slot#^7]"
	},

	{
		"unregister",   G_admin_unregister,  qfalse, "unregister",
		"unregister your name so that it can be used by other players.",
		""
	}
};

static size_t     adminNumCmds = sizeof( g_admin_cmds ) / sizeof( g_admin_cmds[ 0 ] );

static int        admin_level_maxname = 0;
g_admin_level_t   *g_admin_levels = NULL;
g_admin_admin_t   *g_admin_admins = NULL;
g_admin_ban_t     *g_admin_bans = NULL;
g_admin_command_t *g_admin_commands = NULL;

void G_admin_register_cmds( void )
{
	int i;

	for ( i = 0; i < adminNumCmds; i++ )
	{
		trap_AddCommand( g_admin_cmds[ i ].keyword );
	}
}

void G_admin_unregister_cmds( void )
{
	int i;

	for ( i = 0; i < adminNumCmds; i++ )
	{
		trap_RemoveCommand( g_admin_cmds[ i ].keyword );
	}
}

void G_admin_cmdlist( gentity_t *ent )
{
	int  i;
	char out[ MAX_STRING_CHARS ] = "";
	int  len, outlen;

	outlen = 0;

	for ( i = 0; i < adminNumCmds; i++ )
	{
		if ( !G_admin_permission( ent, g_admin_cmds[ i ].flag ) )
		{
			continue;
		}

		len = strlen( g_admin_cmds[ i ].keyword ) + 1;

		if ( len + outlen >= sizeof( out ) - 1 )
		{
			trap_SendServerCommand( ent - g_entities, va( "cmds%s\n", out ) );
			outlen = 0;
		}

		strcpy( out + outlen, va( " %s", g_admin_cmds[ i ].keyword ) );
		outlen += len;
	}

	trap_SendServerCommand( ent - g_entities, va( "cmds%s\n", out ) );
}

// match a certain flag within these flags
static qboolean admin_permission( char *flags, const char *flag, qboolean *perm )
{
	char     *token, *token_p = flags;
	qboolean allflags = qfalse;
	qboolean p = qfalse;
	*perm = qfalse;

	while ( * ( token = COM_Parse( &token_p ) ) )
	{
		*perm = qtrue;

		if ( *token == '-' || *token == '+' )
		{
			*perm = *token++ == '+';
		}

		if ( !strcmp( token, flag ) )
		{
			return qtrue;
		}

		if ( !strcmp( token, ADMF_ALLFLAGS ) )
		{
			allflags = qtrue;
			p = *perm;
		}
	}

	if ( allflags )
	{
		*perm = p;
	}

	return allflags;
}

g_admin_cmd_t *G_admin_cmd( const char *cmd )
{
	return bsearch( cmd, g_admin_cmds, adminNumCmds, sizeof( g_admin_cmd_t ),
	                cmdcmp );
}

g_admin_level_t *G_admin_level( const int l )
{
	g_admin_level_t *level;

	for ( level = g_admin_levels; level; level = level->next )
	{
		if ( level->level == l )
		{
			return level;
		}
	}

	return NULL;
}

g_admin_admin_t *G_admin_admin( const char *guid )
{
	g_admin_admin_t *admin;

	for ( admin = g_admin_admins; admin; admin = admin->next )
	{
		if ( !Q_stricmp( admin->guid, guid ) )
		{
			return admin;
		}
	}

	return NULL;
}

g_admin_command_t *G_admin_command( const char *cmd )
{
	g_admin_command_t *c;

	for ( c = g_admin_commands; c; c = c->next )
	{
		if ( !Q_stricmp( c->command, cmd ) )
		{
			return c;
		}
	}

	return NULL;
}

qboolean G_admin_permission( gentity_t *ent, const char *flag )
{
	qboolean        perm;
	g_admin_admin_t *a;
	g_admin_level_t *l;

	// Always return true for dummy commands.
	if ( flag == NULL )
	{
		return qtrue;
	}

	// console always wins
	if ( !ent )
	{
		return qtrue;
	}

	if ( ent->client->pers.admin && ent->client->pers.pubkey_authenticated != 1 )
	{
		CP( "cp \"^1You are not pubkey authenticated\"\n" );
		return qfalse;
	}

	if ( ( a = ent->client->pers.admin ) )
	{
		if ( admin_permission( a->flags, flag, &perm ) )
		{
			return perm;
		}

		l = G_admin_level( a->level );
	}
	else
	{
		l = G_admin_level( 0 );
	}

	if ( l )
	{
		return admin_permission( l->flags, flag, &perm ) && perm;
	}

	return qfalse;
}

qboolean G_admin_name_check( gentity_t *ent, char *name, char *err, int len )
{
	int             i;
	gclient_t       *client;
	char            testName[ MAX_NAME_LENGTH ] = { "" };
	char            name2[ MAX_NAME_LENGTH ] = { "" };
	g_admin_admin_t *admin;
	int             alphaCount = 0;

	G_SanitiseString( name, name2, sizeof( name2 ) );

	if ( !strcmp( name2, "unnamedplayer" ) )
	{
		return qtrue;
	}

	if ( !strcmp( name2, "console" ) )
	{
		if ( err && len > 0 )
		{
			Q_strncpyz( err, "The name 'console' is not allowed.", len );
		}

		return qfalse;
	}

	G_DecolorString( name, testName, sizeof( testName ) );

	if ( isdigit( testName[ 0 ] ) )
	{
		if ( err && len > 0 )
		{
			Q_strncpyz( err, "Names cannot begin with numbers", len );
		}

		return qfalse;
	}

	for ( i = 0; testName[ i ]; i++ )
	{
		if ( isalpha( testName[ i ] ) )
		{
			alphaCount++;
		}
	}

	if ( alphaCount == 0 )
	{
		if ( err && len > 0 )
		{
			Q_strncpyz( err, "Names must contain letters", len );
		}

		return qfalse;
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

		if ( !strcmp( name2, testName ) )
		{
			if ( err && len > 0 )
			{
				Com_sprintf( err, len, "The name '%s^7' is already in use", name );
			}

			return qfalse;
		}
	}

	for ( admin = g_admin_admins; admin; admin = admin->next )
	{
		if ( admin->level < 1 )
		{
			continue;
		}

		G_SanitiseString( admin->name, testName, sizeof( testName ) );

		if ( !strcmp( name2, testName ) && ent->client->pers.admin != admin )
		{
			if ( err && len > 0 )
			{
				Com_sprintf( err, len, "The name '%s^7' belongs to an admin, "
				             "please use another name", name );
			}

			return qfalse;
		}
	}

	return qtrue;
}

static qboolean admin_higher_admin( g_admin_admin_t *a, g_admin_admin_t *b )
{
	qboolean perm;

	if ( !b )
	{
		return qtrue;
	}

	if ( admin_permission( b->flags, ADMF_IMMUTABLE, &perm ) )
	{
		return !perm;
	}

	return b->level <= ( a ? a->level : 0 );
}

static qboolean admin_higher_guid( char *admin_guid, char *victim_guid )
{
	return admin_higher_admin( G_admin_admin( admin_guid ),
	                           G_admin_admin( victim_guid ) );
}

static qboolean admin_higher( gentity_t *admin, gentity_t *victim )
{
	// console always wins
	if ( !admin )
	{
		return qtrue;
	}

	return admin_higher_admin( admin->client->pers.admin,
	                           victim->client->pers.admin );
}

static void admin_writeconfig_string( char *s, fileHandle_t f )
{
	if ( s[ 0 ] )
	{
		trap_FS_Write( s, strlen( s ), f );
	}

	trap_FS_Write( "\n", 1, f );
}

static void admin_writeconfig_int( int v, fileHandle_t f )
{
	char buf[ 32 ];

	Com_sprintf( buf, sizeof( buf ), "%d\n", v );
	trap_FS_Write( buf, strlen( buf ), f );
}

void G_admin_writeconfig( void )
{
	fileHandle_t      f;
	int               t;
	g_admin_admin_t   *a;
	g_admin_level_t   *l;
	g_admin_ban_t     *b;
	g_admin_command_t *c;

	if ( !g_admin.string[ 0 ] )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: g_admin is not set. "
		          " configuration will not be saved to a file.\n" );
		return;
	}

	t = trap_RealTime( NULL );

	if ( trap_FS_FOpenFile( g_admin.string, &f, FS_WRITE ) < 0 )
	{
		G_Printf( "admin_writeconfig: could not open g_admin file \"%s\"\n",
		          g_admin.string );
		return;
	}

	for ( l = g_admin_levels; l; l = l->next )
	{
		trap_FS_Write( "[level]\n", 8, f );
		trap_FS_Write( "level   = ", 10, f );
		admin_writeconfig_int( l->level, f );
		trap_FS_Write( "name    = ", 10, f );
		admin_writeconfig_string( l->name, f );
		trap_FS_Write( "flags   = ", 10, f );
		admin_writeconfig_string( l->flags, f );
		trap_FS_Write( "\n", 1, f );
	}

	for ( a = g_admin_admins; a; a = a->next )
	{
		// don't write level 0 users
		if ( a->level == 0 )
		{
			continue;
		}

		trap_FS_Write( "[admin]\n", 8, f );
		trap_FS_Write( "name    = ", 10, f );
		admin_writeconfig_string( a->name, f );
		trap_FS_Write( "guid    = ", 10, f );
		admin_writeconfig_string( a->guid, f );
		trap_FS_Write( "level   = ", 10, f );
		admin_writeconfig_int( a->level, f );
		trap_FS_Write( "flags   = ", 10, f );
		admin_writeconfig_string( a->flags, f );
		trap_FS_Write( "pubkey  = ", 10, f );
		admin_writeconfig_string( a->pubkey, f );
		trap_FS_Write( "msg     = ", 10, f );
		admin_writeconfig_string( a->msg, f );
		trap_FS_Write( "msg2    = ", 10, f );
		admin_writeconfig_string( a->msg2, f );
		trap_FS_Write( "counter = ", 10, f );
		admin_writeconfig_int( a->counter, f );
		trap_FS_Write( "\n", 1, f );
	}

	for ( b = g_admin_bans; b; b = b->next )
	{
		// don't write expired bans
		// if expires is 0, then it's a perm ban
		if ( b->expires != 0 && b->expires <= t )
		{
			continue;
		}

		trap_FS_Write( "[ban]\n", 6, f );
		trap_FS_Write( "name    = ", 10, f );
		admin_writeconfig_string( b->name, f );
		trap_FS_Write( "guid    = ", 10, f );
		admin_writeconfig_string( b->guid, f );
		trap_FS_Write( "ip      = ", 10, f );
		admin_writeconfig_string( b->ip.str, f );
		trap_FS_Write( "reason  = ", 10, f );
		admin_writeconfig_string( b->reason, f );
		trap_FS_Write( "made    = ", 10, f );
		admin_writeconfig_string( b->made, f );
		trap_FS_Write( "expires = ", 10, f );
		admin_writeconfig_int( b->expires, f );
		trap_FS_Write( "banner  = ", 10, f );
		admin_writeconfig_string( b->banner, f );
		trap_FS_Write( "\n", 1, f );
	}

	for ( c = g_admin_commands; c; c = c->next )
	{
		trap_FS_Write( "[command]\n", 10, f );
		trap_FS_Write( "command = ", 10, f );
		admin_writeconfig_string( c->command, f );
		trap_FS_Write( "exec    = ", 10, f );
		admin_writeconfig_string( c->exec, f );
		trap_FS_Write( "desc    = ", 10, f );
		admin_writeconfig_string( c->desc, f );
		trap_FS_Write( "flag    = ", 10, f );
		admin_writeconfig_string( c->flag, f );
		trap_FS_Write( "\n", 1, f );
	}

	trap_FS_FCloseFile( f );
}

static void admin_readconfig_string( char **cnf, char *s, int size )
{
	char *t;

	//COM_MatchToken(cnf, "=");
	s[ 0 ] = '\0';
	t = COM_ParseExt( cnf, qfalse );

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
	int             level = 0;

	l = g_admin_levels = BG_Alloc( sizeof( g_admin_level_t ) );
	l->level = level++;
	Q_strncpyz( l->name, "^4Unknown Player", sizeof( l->name ) );
	Q_strncpyz( l->flags,
	            "listplayers admintest adminhelp time register",
	            sizeof( l->flags ) );

	l = l->next = BG_Alloc( sizeof( g_admin_level_t ) );
	l->level = level++;
	Q_strncpyz( l->name, "^5Server Regular", sizeof( l->name ) );
	Q_strncpyz( l->flags,
	            "listplayers admintest adminhelp time register unregister",
	            sizeof( l->flags ) );

	l = l->next = BG_Alloc( sizeof( g_admin_level_t ) );
	l->level = level++;
	Q_strncpyz( l->name, "^6Team Manager", sizeof( l->name ) );
	Q_strncpyz( l->flags,
	            "listplayers admintest adminhelp time putteam spec999 register unregister",
	            sizeof( l->flags ) );

	l = l->next = BG_Alloc( sizeof( g_admin_level_t ) );
	l->level = level++;
	Q_strncpyz( l->name, "^2Junior Admin", sizeof( l->name ) );
	Q_strncpyz( l->flags,
	            "listplayers admintest adminhelp time putteam spec999 kick mute ADMINCHAT "
	            "register unregister l0 l1",
	            sizeof( l->flags ) );

	l = l->next = BG_Alloc( sizeof( g_admin_level_t ) );
	l->level = level++;
	Q_strncpyz( l->name, "^3Senior Admin", sizeof( l->name ) );
	Q_strncpyz( l->flags,
	            "listplayers admintest adminhelp time putteam spec999 kick mute showbans ban "
	            "namelog ADMINCHAT register unregister l0 l1",
	            sizeof( l->flags ) );

	l = l->next = BG_Alloc( sizeof( g_admin_level_t ) );
	l->level = level++;
	Q_strncpyz( l->name, "^1Server Operator", sizeof( l->name ) );
	Q_strncpyz( l->flags,
	            "ALLFLAGS -IMMUTABLE -INCOGNITO",
	            sizeof( l->flags ) );
	admin_level_maxname = 15;
}

void G_admin_authlog( gentity_t *ent )
{
	char            aflags[ MAX_ADMIN_FLAGS * 2 ];
	g_admin_level_t *level;
	int             levelNum = 0;

	if ( !ent )
	{
		return;
	}

	if ( ent->client->pers.admin )
	{
		levelNum = ent->client->pers.admin->level;
	}

	level = G_admin_level( levelNum );

	Com_sprintf( aflags, sizeof( aflags ), "%s %s",
	             ent->client->pers.admin->flags,
	             ( level ) ? level->flags : "" );

	G_LogPrintf( "AdminAuth: %i \"%s" S_COLOR_WHITE "\" \"%s" S_COLOR_WHITE
	             "\" [%d] (%s): %s\n",
	             ( int )( ent - g_entities ), ent->client->pers.netname,
	             ent->client->pers.admin->name, ent->client->pers.admin->level,
	             ent->client->pers.guid, aflags );
}

static char adminLog[ MAX_STRING_CHARS ];
static int  adminLogLen;
static void admin_log_start( gentity_t *admin, const char *cmd )
{
	const char *name = admin ? admin->client->pers.netname : "console";

	adminLogLen = Q_snprintf( adminLog, sizeof( adminLog ),
	                          "%d \"%s" S_COLOR_WHITE "\" \"%s" S_COLOR_WHITE "\" [%d] (%s): %s",
	                          admin ? admin->s.clientNum : -1,
	                          name,
	                          admin && admin->client->pers.admin ? admin->client->pers.admin->name : name,
	                          admin && admin->client->pers.admin ? admin->client->pers.admin->level : 0,
	                          admin ? admin->client->pers.guid : "",
	                          cmd );
}

static void admin_log( const char *str )
{
	if ( adminLog[ 0 ] )
	{
		adminLogLen += Q_snprintf( adminLog + adminLogLen,
		                           sizeof( adminLog ) - adminLogLen, ": %s", str );
	}
}

static void admin_log_abort( void )
{
	adminLog[ 0 ] = '\0';
	adminLogLen = 0;
}

static void admin_log_end( const qboolean ok )
{
	if ( adminLog[ 0 ] )
	{
		G_LogPrintf( "AdminExec: %s: %s\n", ok ? "ok" : "fail", adminLog );
	}

	admin_log_abort();
}

struct llist
{
	struct llist *next;
};

static int admin_search( gentity_t *ent,
                         const char *cmd,
                         const char *noun,
                         qboolean( *match )( void *, const void * ),
                         void ( *out )( void *, char * ),
                         const void *list,
                         const void *arg, /* this will be used as char* later */
                         int start,
                         const int offset,
                         const int limit )
{
	int          i;
	int          count = 0;
	int          found = 0;
	int          total;
	int          next = 0, end = 0;
	char         str[ MAX_STRING_CHARS ];
	struct llist *l = ( struct llist * ) list;

	for ( total = 0; l; total++, l = l->next ) {; }

	if ( start < 0 )
	{
		start += total;
	}
	else
	{
		start -= offset;
	}

	if ( start < 0 || start > total )
	{
		start = 0;
	}

	ADMBP_begin();

	for ( i = 0, l = ( struct llist * ) list; l; i++, l = l->next )
	{
		if ( match( l, arg ) )
		{
			if ( i >= start && ( limit < 1 || count < limit ) )
			{
				out( l, str );
				ADMBP( va( "%-3d %s\n", i + offset, str ) );
				count++;
				end = i;
			}
			else if ( count == limit )
			{
				if ( next == 0 )
				{
					next = i;
				}
			}

			found++;
		}
	}

	if ( limit > 0 )
	{
		ADMBP( va( "^3%s: ^7showing %d of %d %s %d-%d%s%s.",
		           cmd, count, found, noun, start + offset, end + offset,
		           * ( char * ) arg ? " matching " : "", ( char * ) arg ) );

		if ( next )
		{
			ADMBP( va( "  use '%s%s%s %d' to see more", cmd,
			           * ( char * ) arg ? " " : "",
			           ( char * ) arg,
			           next + offset ) );
		}
	}

	ADMBP( "\n" );
	ADMBP_end();
	return next + offset;
}

static qboolean admin_match( void *admin, const void *match )
{
	char n1[ MAX_NAME_LENGTH ], n2[ MAX_NAME_LENGTH ];
	G_SanitiseString( ( char * ) match, n2, sizeof( n2 ) );

	if ( !n2[ 0 ] )
	{
		return qtrue;
	}

	G_SanitiseString( ( ( g_admin_admin_t * ) admin )->name, n1, sizeof( n1 ) );
	return strstr( n1, n2 ) ? qtrue : qfalse;
}

static void admin_out( void *admin, char *str )
{
	g_admin_admin_t *a = ( g_admin_admin_t * ) admin;
	g_admin_level_t *l = G_admin_level( a->level );
	int             lncol = 0, i;

	for ( i = 0; l && l->name[ i ]; i++ )
	{
		if ( Q_IsColorString( l->name + i ) )
		{
			lncol += 2;
		}
	}

	Com_sprintf( str, MAX_STRING_CHARS, "%-6d %*s^7 %s",
	             a->level, admin_level_maxname + lncol - 1, l ? l->name : "(null)",
	             a->name );
}

static int admin_listadmins( gentity_t *ent, int start, char *search )
{
	return admin_search( ent, "listadmins", "admins", admin_match, admin_out,
	                     g_admin_admins, search, start, MAX_CLIENTS, MAX_ADMIN_LISTITEMS );
}

static int admin_find_admin( gentity_t *ent, char *name, const char *command,
                             gentity_t **client, g_admin_admin_t **admin )
{
	char            cleanName[ MAX_NAME_LENGTH ];
	char            testName[ MAX_NAME_LENGTH ];
	char            *p;
	int             id = -1;
	int             matches = 0;
	int             i;
	g_admin_admin_t *a = NULL;
	gentity_t       *vic = NULL;
	qboolean        numeric = qtrue;

	if ( client )
	{
		*client = NULL;
	}

	if ( admin )
	{
		*admin = NULL;
	}

	if ( !name || !name[ 0 ] )
	{
		return -1;
	}

	if ( !command )
	{
		command = "match";
	}

	for ( p = name; ( *p ); p++ )
	{
		if ( !isdigit( *p ) )
		{
			numeric = qfalse;
			break;
		}
	}

	if ( numeric )
	{
		id = atoi( name );

		if ( id >= 0 && id < level.maxclients )
		{
			vic = &g_entities[ id ];

			if ( !vic || !vic->client || vic->client->pers.connected == CON_DISCONNECTED )
			{
				ADMP( va( "^3%s: ^7no player connected in slot %d\n", command, id ) );
				return -1;
			}

			a = vic->client->pers.admin;
		}
		else if ( id >= MAX_CLIENTS )
		{
			for ( i = MAX_CLIENTS, a = g_admin_admins; a; i++, a = a->next )
			{
				if ( i == id )
				{
					break;
				}
			}

			for ( i = 0; i < level.maxclients; i++ )
			{
				if ( level.clients[ i ].pers.connected == CON_DISCONNECTED )
				{
					continue;
				}

				if ( level.clients[ i ].pers.admin == a )
				{
					vic = &g_entities[ i ];
					break;
				}
			}
		}

		if ( !vic && !a )
		{
			for ( i = 0, a = g_admin_admins; a; i++, a = a->next ) {; }

			ADMP( va( "^3%s: ^7%s not in range 0-%d or %d-%d\n",
			          command, name,
			          level.maxclients - 1,
			          MAX_CLIENTS, MAX_CLIENTS + i - 1 ) );
			return -1;
		}
	}
	else
	{
		g_admin_admin_t *wa;

		G_SanitiseString( name, cleanName, sizeof( cleanName ) );

		for ( wa = g_admin_admins, i = MAX_CLIENTS; wa && matches < 2; wa = wa->next, i++ )
		{
			G_SanitiseString( wa->name, testName, sizeof( testName ) );

			if ( strstr( testName, cleanName ) )
			{
				id = i;
				a = wa;
				matches++;
			}
		}

		for ( i = 0; i < level.maxclients && matches < 2; i++ )
		{
			if ( level.clients[ i ].pers.connected == CON_DISCONNECTED )
			{
				continue;
			}

			if ( matches && a && level.clients[ i ].pers.admin == a )
			{
				vic = &g_entities[ i ];
				continue;
			}

			G_SanitiseString( level.clients[ i ].pers.netname,
			                  testName, sizeof( testName ) );

			if ( strstr( testName, cleanName ) )
			{
				id = i;
				vic = &g_entities[ i ];
				a = vic->client->pers.admin;
				matches++;
			}
		}

		if ( matches == 0 )
		{
			ADMP( va( "^3%s:^7 no match, use listplayers or listadmins to "
			          "find an appropriate number to use instead of name.\n",
			          command ) );
			return -1;
		}

		if ( matches > 1 )
		{
			ADMP( va( "^3%s:^7 more than one match, use the admin number instead:\n",
			          command ) );
			admin_listadmins( ent, 0, cleanName );
			return -1;
		}
	}

	if ( client )
	{
		*client = vic;
	}

	if ( admin )
	{
		*admin = a;
	}

	return id;
}

#define MAX_DURATION_LENGTH 13
void G_admin_duration( int secs, char *duration, int dursize )
{
	// sizeof("12.5 minutes") == 13
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

static void G_admin_ban_message(
  gentity_t     *ent,
  g_admin_ban_t *ban,
  char          *creason,
  int           clen,
  char          *areason,
  int           alen )
{
	if ( creason )
	{
		char duration[ MAX_DURATION_LENGTH ];
		G_admin_duration( ban->expires - trap_RealTime( NULL ), duration,
		                  sizeof( duration ) );
		// part of this might get cut off on the connect screen
		Com_sprintf( creason, clen,
		             "You have been banned by %s" S_COLOR_WHITE " duration: %s"
		             " reason: %s",
		             ban->banner,
		             duration,
		             ban->reason );
	}

	if ( areason && ent )
	{
		// we just want the ban number
		int           n = 1;
		g_admin_ban_t *b = g_admin_bans;

		for ( ; b && b != ban; b = b->next, n++ )
		{
			;
		}

		Com_sprintf( areason, alen,
		             S_COLOR_YELLOW "Banned player %s" S_COLOR_YELLOW
		             " tried to connect from %s (ban #%d)",
		             ent->client->pers.netname[ 0 ] ? ent->client->pers.netname : ban->name,
		             ent->client->pers.ip.str,
		             n );
	}
}

static qboolean G_admin_ban_matches( g_admin_ban_t *ban, gentity_t *ent )
{
	return !Q_stricmp( ban->guid, ent->client->pers.guid ) ||
	       ( !G_admin_permission( ent, ADMF_IMMUNITY ) &&
	         G_AddressCompare( &ban->ip, &ent->client->pers.ip ) );
}

static g_admin_ban_t *G_admin_match_ban( gentity_t *ent )
{
	int           t;
	g_admin_ban_t *ban;

	t = trap_RealTime( NULL );

	if ( ent->client->pers.localClient )
	{
		return NULL;
	}

	for ( ban = g_admin_bans; ban; ban = ban->next )
	{
		// 0 is for perm ban
		if ( ban->expires != 0 && ban->expires <= t )
		{
			continue;
		}

		if ( G_admin_ban_matches( ban, ent ) )
		{
			return ban;
		}
	}

	return NULL;
}

qboolean G_admin_ban_check( gentity_t *ent, char *reason, int rlen )
{
	g_admin_ban_t *ban;
	char          warningMessage[ MAX_STRING_CHARS ];

	if ( ent->client->pers.localClient )
	{
		return qfalse;
	}

	if ( ( ban = G_admin_match_ban( ent ) ) )
	{
		G_admin_ban_message( ent, ban, reason, rlen,
		                     warningMessage, sizeof( warningMessage ) );

		// don't spam admins
		if ( ban->warnCount++ < 5 )
		{
			G_AdminMessage( NULL, warningMessage );
		}
		// and don't fill the console
		else if ( ban->warnCount < 10 )
		{
			trap_Print( va( "%s%s\n", warningMessage,
			                ban->warnCount + 1 == 10 ?
			                S_COLOR_WHITE " - future messages for this ban will be suppressed" :
			                "" ) );
		}

		return qtrue;
	}

	return qfalse;
}

qboolean G_admin_cmd_check( gentity_t *ent )
{
	char              command[ MAX_ADMIN_CMD_LEN ];
	g_admin_cmd_t     *admincmd;
	g_admin_command_t *c;
	qboolean          success;

	command[ 0 ] = '\0';
	trap_Argv( 0, command, sizeof( command ) );

	if ( !command[ 0 ] )
	{
		return qfalse;
	}

	Q_strlwr( command );
	admin_log_start( ent, command );

	if ( ( c = G_admin_command( command ) ) )
	{
		int j;
		trap_Cvar_Register( NULL, "arg_all", "", CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED );
		trap_Cvar_Set( "arg_all", ConcatArgs( 1 ) );
		trap_Cvar_Register( NULL, "arg_count", "", CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED );
		trap_Cvar_Set( "arg_count", va( "%i", trap_Argc() - ( 1 ) ) );
		trap_Cvar_Register( NULL, "arg_client", "", CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED );
		trap_Cvar_Set( "arg_client", ( ent ) ? ent->client->pers.netname : "console" );

		for ( j = trap_Argc() - ( 1 ); j; j-- )
		{
			char this_arg[ MAX_CVAR_VALUE_STRING ];
			trap_Cvar_Register( NULL, va( "arg_%i", j ), "", CVAR_TEMP | CVAR_ROM | CVAR_USER_CREATED );
			trap_Argv( j, this_arg, sizeof( this_arg ) );
			trap_Cvar_Set( va( "arg_%i", j ), this_arg );
		}

		admin_log( ConcatArgsPrintable( 1 ) );

		if ( ( success = G_admin_permission( ent, c->flag ) ) )
		{
			if ( G_FloodLimited( ent ) )
			{
				return qtrue;
			}

			trap_SendConsoleCommand( EXEC_APPEND, c->exec );
		}
		else
		{
			ADMP( va( "^3%s: ^7permission denied\n", c->command ) );
		}

		admin_log_end( success );
		return qtrue;
	}

	if ( ( admincmd = G_admin_cmd( command ) ) )
	{
		if ( ( success = G_admin_permission( ent, admincmd->flag ) ) )
		{
			if ( G_FloodLimited( ent ) )
			{
				return qtrue;
			}

			if ( admincmd->silent )
			{
				admin_log_abort();
			}

			if ( !admincmd->handler )
			{
				return qtrue;
			}

			if ( !( success = admincmd->handler( ent ) ) )
			{
				admin_log( ConcatArgsPrintable( 1 ) );
			}
		}
		else
		{
			ADMP( va( "^3%s: ^7permission denied\n", admincmd->keyword ) );
			admin_log( ConcatArgsPrintable( 1 ) );
		}

		admin_log_end( success );
		return qtrue;
	}

	return qfalse;
}

static void llsort( struct llist **head, int compar( const void *, const void * ) )
{
	struct llist *a, *b, *t, *l;

	int          i, c = 1, ns, as, bs;

	if ( !*head )
	{
		return;
	}

	do
	{
		a = *head, l = *head = NULL;

		for ( ns = 0; a; ns++, a = b )
		{
			b = a;

			for ( i = as = 0; i < c; i++ )
			{
				as++;

				if ( !( b = b->next ) )
				{
					break;
				}
			}

			for ( bs = c; ( b && bs ) || as; l = t )
			{
				if ( as && ( !bs || !b || compar( a, b ) <= 0 ) )
				{
					t = a, a = a->next, as--;
				}
				else
				{
					t = b, b = b->next, bs--;
				}

				if ( l )
				{
					l->next = t;
				}
				else
				{
					*head = t;
				}
			}
		}

		l->next = NULL;
		c *= 2;
	}
	while ( ns > 1 );
}

static int cmplevel( const void *a, const void *b )
{
	return ( ( g_admin_level_t * ) b )->level - ( ( g_admin_level_t * ) a )->level;
}

void G_admin_pubkey( void )
{
	g_admin_admin_t *highest = NULL, *a = NULL;

	// Uncomment this if your server lags (shouldn't happen unless you are on a *very* old computer)
	// Will only regenrate messages when there are no active client (When they are all loading the map)
#if 0
	int             i;

	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( g_entities[ i ].client && g_entities[ i ].client->pers.connected == CON_CONNECTED )
		{
			return;
		}
	}

#endif

	// Only do 1 encryption per frame to avoid lag
	for ( a = g_admin_admins; a; a = a->next )
	{
		if ( a->counter == -1 && a->pubkey[ 0 ] )
		{
			highest = a;
			break;
		}
		else if ( a->counter == 0 || !a->pubkey[ 0 ] )
		{
			continue;
		}
		else if ( !highest )
		{
			highest = a;
			continue;
		}
		else if ( highest->counter < a->counter )
		{
			highest = a;
		}
	}

	if ( highest )
	{
		if ( trap_RSA_GenerateMessage( highest->pubkey, highest->msg, highest->msg2 ) )
		{
			highest->counter = 0;
		}
		else
		{
			// If the key generation failed it can only be because of a bad pubkey
			// so we clear the pubkey and ask the client for a new one when he reconnects
			highest->pubkey[ 0 ] = '\0';
			highest->msg[ 0 ] = '\0';
			highest->msg2[ 0 ] = '\0';
			highest->counter = -1;
		}

		G_admin_writeconfig();
	}
}

qboolean G_admin_readconfig( gentity_t *ent )
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
	char              ip[ 44 ];

	G_admin_cleanup();

	if ( !g_admin.string[ 0 ] )
	{
		ADMP( "^3readconfig: g_admin is not set, not loading configuration "
		      "from a file\n" );
		return qfalse;
	}

	len = trap_FS_FOpenFile( g_admin.string, &f, FS_READ );

	if ( len < 0 )
	{
		G_Printf( "^3readconfig: ^7could not open admin config file %s\n",
		          g_admin.string );
		admin_default_levels();
		return qfalse;
	}

	cnf = BG_Alloc( len + 1 );
	cnf2 = cnf;
	trap_FS_Read( cnf, len, f );
	* ( cnf + len ) = '\0';
	trap_FS_FCloseFile( f );

	admin_level_maxname = 0;

	level_open = admin_open = ban_open = command_open = qfalse;
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
			if ( l )
			{
				l = l->next = BG_Alloc( sizeof( g_admin_level_t ) );
			}
			else
			{
				l = g_admin_levels = BG_Alloc( sizeof( g_admin_level_t ) );
			}

			level_open = qtrue;
			admin_open = ban_open = command_open = qfalse;
			lc++;
		}
		else if ( !Q_stricmp( t, "[admin]" ) )
		{
			if ( a )
			{
				a = a->next = BG_Alloc( sizeof( g_admin_admin_t ) );
			}
			else
			{
				a = g_admin_admins = BG_Alloc( sizeof( g_admin_admin_t ) );
			}

			admin_open = qtrue;
			level_open = ban_open = command_open = qfalse;
			ac++;
		}
		else if ( !Q_stricmp( t, "[ban]" ) )
		{
			if ( b )
			{
				b = b->next = BG_Alloc( sizeof( g_admin_ban_t ) );
			}
			else
			{
				b = g_admin_bans = BG_Alloc( sizeof( g_admin_ban_t ) );
			}

			ban_open = qtrue;
			level_open = admin_open = command_open = qfalse;
			bc++;
		}
		else if ( !Q_stricmp( t, "[command]" ) )
		{
			if ( c )
			{
				c = c->next = BG_Alloc( sizeof( g_admin_command_t ) );
			}
			else
			{
				c = g_admin_commands = BG_Alloc( sizeof( g_admin_command_t ) );
			}

			command_open = qtrue;
			level_open = admin_open = ban_open = qfalse;
			cc++;
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
				// max printable name length for formatting
				len = Q_PrintStrlen( l->name );

				if ( len > admin_level_maxname )
				{
					admin_level_maxname = len;
				}
			}
			else if ( !Q_stricmp( t, "flags" ) )
			{
				admin_readconfig_string( &cnf, l->flags, sizeof( l->flags ) );
			}
			else
			{
				COM_ParseError( "[level] unrecognized token \"%s\"", t );
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
			else if ( !Q_stricmp( t, "pubkey" ) )
			{
				admin_readconfig_string( &cnf, a->pubkey, sizeof( a->pubkey ) );
			}
			else if ( !Q_stricmp( t, "msg" ) )
			{
				admin_readconfig_string( &cnf, a->msg, sizeof( a->msg ) );
			}
			else if ( !Q_stricmp( t, "msg2" ) )
			{
				admin_readconfig_string( &cnf, a->msg2, sizeof( a->msg2 ) );
			}
			else if ( !Q_stricmp( t, "counter" ) )
			{
				admin_readconfig_int( &cnf, &a->counter );
			}
			else
			{
				COM_ParseError( "[admin] unrecognized token \"%s\"", t );
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
				admin_readconfig_string( &cnf, ip, sizeof( ip ) );
				G_AddressParse( ip, &b->ip );
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
				COM_ParseError( "[ban] unrecognized token \"%s\"", t );
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
			else if ( !Q_stricmp( t, "flag" ) )
			{
				admin_readconfig_string( &cnf, c->flag, sizeof( c->flag ) );
			}
			else
			{
				COM_ParseError( "[command] unrecognized token \"%s\"", t );
			}
		}
		else
		{
			COM_ParseError( "unexpected token \"%s\"", t );
		}
	}

	BG_Free( cnf2 );
	ADMP( va( "^3readconfig: ^7loaded %d levels, %d admins, %d bans, %d commands\n",
	          lc, ac, bc, cc ) );

	if ( lc == 0 )
	{
		admin_default_levels();
	}
	else
	{
		llsort( ( struct llist ** ) &g_admin_levels, cmplevel );
		llsort( ( struct llist ** ) &g_admin_admins, cmplevel );
	}

	// restore admin mapping
	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].pers.connected != CON_DISCONNECTED )
		{
			level.clients[ i ].pers.admin = level.clients[ i ].pers.pubkey_authenticated ?
			                                G_admin_admin( level.clients[ i ].pers.guid ) : NULL;

			if ( level.clients[ i ].pers.admin )
			{
				G_admin_authlog( &g_entities[ i ] );
			}

			G_admin_cmdlist( &g_entities[ i ] );
		}
	}

	return qtrue;
}

qboolean G_admin_time( gentity_t *ent )
{
	qtime_t qt;

	trap_RealTime( &qt );
	ADMP( va( "^3time: ^7local time is %02i:%02i:%02i\n",
	          qt.tm_hour, qt.tm_min, qt.tm_sec ) );
	return qtrue;
}

// this should be in one of the headers, but it is only used here for now
namelog_t *G_NamelogFromString( gentity_t *ent, char *s );

/*
for consistency, we should be able to target a disconnected player with setlevel
but we can't use namelog and remain consistent, so the solution would be to make
everyone a real level 0 admin so they can be targeted until the next level
but that seems kind of stupid
*/
qboolean G_admin_setlevel( gentity_t *ent )
{
	char            name[ MAX_NAME_LENGTH ] = { "" };
	char            lstr[ 12 ]; // 11 is max strlen() for 32-bit (signed) int
	char            testname[ MAX_NAME_LENGTH ] = { "" };
	int             i;
	gentity_t       *vic = NULL;
	g_admin_admin_t *a = NULL;
	g_admin_level_t *l = NULL;
	int             na;

	if ( trap_Argc() < 3 )
	{
		ADMP( "^3setlevel: ^7usage: setlevel [name|slot#] [level]\n" );
		return qfalse;
	}

	trap_Argv( 1, testname, sizeof( testname ) );
	trap_Argv( 2, lstr, sizeof( lstr ) );

	if ( !( l = G_admin_level( atoi( lstr ) ) ) )
	{
		ADMP( "^3setlevel: ^7level is not defined\n" );
		return qfalse;
	}

	if ( ent && l->level >
	     ( ent->client->pers.admin ? ent->client->pers.admin->level : 0 ) )
	{
		ADMP( "^3setlevel: ^7you may not use setlevel to set a level higher "
		      "than your current level\n" );
		return qfalse;
	}

	for ( na = 0, a = g_admin_admins; a; na++, a = a->next ) {; }

	for ( i = 0; testname[ i ] && isdigit( testname[ i ] ); i++ ) {; }

	if ( !testname[ i ] )
	{
		int id = atoi( testname );

		if ( id < MAX_CLIENTS )
		{
			vic = &g_entities[ id ];

			if ( !vic || !vic->client || vic->client->pers.connected == CON_DISCONNECTED )
			{
				ADMP( va( "^3setlevel: ^7no player connected in slot %d\n", id ) );
				return qfalse;
			}
		}
		else if ( id < na + MAX_CLIENTS )
		{
			for ( i = 0, a = g_admin_admins; i < id - MAX_CLIENTS; i++, a = a->next ) {; }
		}
		else
		{
			ADMP( va( "^3setlevel: ^7%s not in range 1-%d\n",
			          testname, na + MAX_CLIENTS - 1 ) );
			return qfalse;
		}
	}
	else
	{
		G_SanitiseString( testname, name, sizeof( name ) );
	}

	if ( vic )
	{
		a = vic->client->pers.admin;
	}
	else if ( !a )
	{
		g_admin_admin_t *wa;
		int             matches = 0;

		for ( wa = g_admin_admins; wa && matches < 2; wa = wa->next )
		{
			G_SanitiseString( wa->name, testname, sizeof( testname ) );

			if ( strstr( testname, name ) )
			{
				a = wa;
				matches++;
			}
		}

		for ( i = 0; i < level.maxclients && matches < 2; i++ )
		{
			if ( level.clients[ i ].pers.connected == CON_DISCONNECTED )
			{
				continue;
			}

			if ( matches && level.clients[ i ].pers.admin &&
			     level.clients[ i ].pers.admin == a )
			{
				vic = &g_entities[ i ];
				continue;
			}

			G_SanitiseString( level.clients[ i ].pers.netname, testname,
			                  sizeof( testname ) );

			if ( strstr( testname, name ) )
			{
				vic = &g_entities[ i ];
				a = vic->client->pers.admin;
				matches++;
			}
		}

		if ( matches == 0 )
		{
			ADMP( "^3setlevel:^7 no match.  use listplayers or listadmins to "
			      "find an appropriate number to use instead of name.\n" );
			return qfalse;
		}

		if ( matches > 1 )
		{
			ADMP( "^3setlevel:^7 more than one match.  Use the admin number "
			      "instead:\n" );
			admin_listadmins( ent, 0, name );
			return qfalse;
		}
	}

	if ( ent && !admin_higher_admin( ent->client->pers.admin, a ) )
	{
		ADMP( "^3setlevel: ^7sorry, but your intended victim has a higher"
		      " admin level than you\n" );
		return qfalse;
	}

	if ( !a && vic )
	{
		for ( a = g_admin_admins; a && a->next; a = a->next ) {; }

		if ( a )
		{
			a = a->next = BG_Alloc( sizeof( g_admin_admin_t ) );
		}
		else
		{
			a = g_admin_admins = BG_Alloc( sizeof( g_admin_admin_t ) );
		}

		vic->client->pers.admin = a;
		Q_strncpyz( a->guid, vic->client->pers.guid, sizeof( a->guid ) );
	}

	a->level = l->level;

	if ( vic )
	{
		Q_strncpyz( a->name, vic->client->pers.netname, sizeof( a->name ) );

		if ( l && l->level >= g_adminPubkeyID.integer && !a->pubkey[ 0 ] && vic->client->pers.cl_pubkeyID )
		{
			trap_SendServerCommand( vic - g_entities, "pubkey_request" );
		}

		vic->client->pers.pubkey_authenticated = 1;
	}

	admin_log( va( "%d (%s) \"%s" S_COLOR_WHITE "\"", a->level, a->guid,
	               a->name ) );

	AP( va(
	      "print \"^3setlevel: ^7%s^7 was given level %d admin rights by %s\n\"",
	      a->name, a->level, ( ent ) ? ent->client->pers.netname : "console" ) );

	G_admin_writeconfig();

	if ( vic )
	{
		G_admin_authlog( vic );
		G_admin_cmdlist( vic );
	}

	return qtrue;
}

static void admin_create_ban( gentity_t *ent,
                              char *netname,
                              char *guid,
                              addr_t *ip,
                              int seconds,
                              char *reason )
{
	g_admin_ban_t *b = NULL;
	qtime_t       qt;
	int           t;
	int           i;
	char          *name;
	char          disconnect[ MAX_STRING_CHARS ];

	t = trap_RealTime( &qt );

	for ( b = g_admin_bans; b; b = b->next )
	{
		if ( !b->next )
		{
			break;
		}
	}

	if ( b )
	{
		if ( !b->next )
		{
			b = b->next = BG_Alloc( sizeof( g_admin_ban_t ) );
		}
	}
	else
	{
		b = g_admin_bans = BG_Alloc( sizeof( g_admin_ban_t ) );
	}

	Q_strncpyz( b->name, netname, sizeof( b->name ) );
	Q_strncpyz( b->guid, guid, sizeof( b->guid ) );
	memcpy( &b->ip, ip, sizeof( b->ip ) );

	Com_sprintf( b->made, sizeof( b->made ), "%04i-%02i-%02i %02i:%02i:%02i",
	             1900 + qt.tm_year, qt.tm_mon + 1, qt.tm_mday,
	             qt.tm_hour, qt.tm_min, qt.tm_sec );

	if ( ent && ent->client->pers.admin )
	{
		name = ent->client->pers.admin->name;
	}
	else if ( ent )
	{
		name = ent->client->pers.netname;
	}
	else
	{
		name = "console";
	}

	Q_strncpyz( b->banner, name, sizeof( b->banner ) );

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

	G_admin_ban_message( NULL, b, disconnect, sizeof( disconnect ), NULL, 0 );

	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].pers.connected == CON_DISCONNECTED )
		{
			continue;
		}

		if ( G_admin_ban_matches( b, &g_entities[ i ] ) )
		{
			trap_SendServerCommand( i, va( "disconnect \"%s\"", disconnect ) );

			trap_DropClient( i, va( "has been kicked by %s^7. reason: %s",
			                        b->banner, b->reason ), 0 );
		}
	}
}

int G_admin_parse_time( const char *time )
{
	int seconds = 0, num = 0;

	if ( !*time )
	{
		return -1;
	}

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
		num = 0;
	}

	if ( num )
	{
		seconds += num;
	}

	return seconds;
}

qboolean G_admin_kick( gentity_t *ent )
{
	int       pid;
	char      name[ MAX_NAME_LENGTH ], *reason, err[ MAX_STRING_CHARS ];
	int       minargc;
	gentity_t *vic;

	minargc = 3;

	if ( G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
	{
		minargc = 2;
	}

	if ( trap_Argc() < minargc )
	{
		ADMP( "^3kick: ^7usage: kick [name] [reason]\n" );
		return qfalse;
	}

	trap_Argv( 1, name, sizeof( name ) );
	reason = ConcatArgs( 2 );

	if ( ( pid = G_ClientNumberFromString( name, err, sizeof( err ) ) ) == -1 )
	{
		ADMP( va( "^3kick: ^7%s", err ) );
		return qfalse;
	}

	vic = &g_entities[ pid ];

	if ( !admin_higher( ent, vic ) )
	{
		ADMP( "^3kick: ^7sorry, but your intended victim has a higher admin"
		      " level than you\n" );
		return qfalse;
	}

	if ( vic->client->pers.localClient )
	{
		ADMP( "^3kick: ^7disconnecting the host would end the game\n" );
		return qfalse;
	}

	admin_log( va( "%d (%s) \"%s" S_COLOR_WHITE "\": \"%s" S_COLOR_WHITE "\"",
	               pid,
	               vic->client->pers.guid,
	               vic->client->pers.netname,
	               reason ) );
	admin_create_ban( ent,
	                  vic->client->pers.netname,
	                  vic->client->pers.guid,
	                  &vic->client->pers.ip,
	                  MAX( 1, G_admin_parse_time( g_adminTempBan.string ) ),
	                  ( *reason ) ? reason : "kicked by admin" );
	G_admin_writeconfig();

	return qtrue;
}

qboolean G_admin_ban( gentity_t *ent )
{
	int       seconds;
	char      search[ MAX_NAME_LENGTH ];
	char      secs[ MAX_TOKEN_CHARS ];
	char      *reason;
	char      duration[ MAX_DURATION_LENGTH ];
	int       i;
	addr_t    ip;
	qboolean  ipmatch = qfalse;
	namelog_t *match = NULL;

	if ( trap_Argc() < 2 )
	{
		ADMP( "^3ban: ^7usage: ban [name|slot|IP(/mask)] [duration] [reason]\n" );
		return qfalse;
	}

	trap_Argv( 1, search, sizeof( search ) );
	trap_Argv( 2, secs, sizeof( secs ) );

	seconds = G_admin_parse_time( secs );

	if ( seconds <= 0 )
	{
		seconds = 0;
		reason = ConcatArgs( 2 );
	}
	else
	{
		reason = ConcatArgs( 3 );
	}

	if ( !*reason && !G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
	{
		ADMP( "^3ban: ^7you must specify a reason\n" );
		return qfalse;
	}

	if ( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
	{
		int maximum = MAX( 1, G_admin_parse_time( g_adminMaxBan.string ) );

		if ( seconds == 0 || seconds > maximum )
		{
			ADMP( "^3ban: ^7you may not issue permanent bans\n" );
			seconds = maximum;
		}
	}

	if ( G_AddressParse( search, &ip ) )
	{
		int max = ip.type == IPv4 ? 32 : 128;
		int min = ent ? max / 2 : 1;

		if ( ip.mask < min || ip.mask > max )
		{
			ADMP( va( "^3ban: ^7invalid netmask (%d is not one of %d-%d)\n",
			          ip.mask, min, max ) );
			return qfalse;
		}

		ipmatch = qtrue;

		for ( match = level.namelogs; match; match = match->next )
		{
			// skip players in the namelog who have already been banned
			if ( match->banned )
			{
				continue;
			}

			for ( i = 0; i < MAX_NAMELOG_ADDRS && match->ip[ i ].str[ 0 ]; i++ )
			{
				if ( G_AddressCompare( &ip, &match->ip[ i ] ) )
				{
					break;
				}
			}

			if ( i < MAX_NAMELOG_ADDRS && match->ip[ i ].str[ 0 ] )
			{
				break;
			}
		}

		if ( !match )
		{
			ADMP( "^3ban: ^7no player found by that IP address\n" );
			return qfalse;
		}
	}
	else if ( !( match = G_NamelogFromString( ent, search ) ) || match->banned )
	{
		ADMP( "^3ban: ^7no match\n" );
		return qfalse;
	}

	if ( ent && !admin_higher_guid( ent->client->pers.guid, match->guid ) )
	{
		ADMP( "^3ban: ^7sorry, but your intended victim has a higher admin"
		      " level than you\n" );
		return qfalse;
	}

	if ( match->slot > -1 && level.clients[ match->slot ].pers.localClient )
	{
		ADMP( "^3ban: ^7disconnecting the host would end the game\n" );
		return qfalse;
	}

	G_admin_duration( ( seconds ) ? seconds : -1,
	                  duration, sizeof( duration ) );

	AP( va( "print \"^3ban:^7 %s^7 has been banned by %s^7 "
	        "duration: %s, reason: %s\n\"",
	        match->name[ match->nameOffset ],
	        ( ent ) ? ent->client->pers.netname : "console",
	        duration,
	        ( *reason ) ? reason : "banned by admin" ) );

	admin_log( va( "%d (%s) \"%s" S_COLOR_WHITE "\": \"%s" S_COLOR_WHITE "\"",
	               seconds, match->guid, match->name[ match->nameOffset ], reason ) );

	if ( ipmatch )
	{
		admin_create_ban( ent,
		                  match->slot == -1 ?
		                  match->name[ match->nameOffset ] :
		                  level.clients[ match->slot ].pers.netname,
		                  match->guid,
		                  &ip,
		                  seconds, reason );
		admin_log( va( "[%s]", ip.str ) );
	}
	else
	{
		// ban all IP addresses used by this player
		for ( i = 0; i < MAX_NAMELOG_ADDRS && match->ip[ i ].str[ 0 ]; i++ )
		{
			admin_create_ban( ent,
			                  match->slot == -1 ?
			                  match->name[ match->nameOffset ] :
			                  level.clients[ match->slot ].pers.netname,
			                  match->guid,
			                  &match->ip[ i ],
			                  seconds, reason );
			admin_log( va( "[%s]", match->ip[ i ].str ) );
		}
	}

	match->banned = qtrue;

	if ( !g_admin.string[ 0 ] )
	{
		ADMP( "^3ban: ^7WARNING g_admin not set, not saving ban to a file\n" );
	}
	else
	{
		G_admin_writeconfig();
	}

	return qtrue;
}

qboolean G_admin_unban( gentity_t *ent )
{
	int           bnum;
	int           time = trap_RealTime( NULL );
	char          bs[ 5 ];
	int           i;
	g_admin_ban_t *ban, *p;

	if ( trap_Argc() < 2 )
	{
		ADMP( "^3unban: ^7usage: unban [ban#]\n" );
		return qfalse;
	}

	trap_Argv( 1, bs, sizeof( bs ) );
	bnum = atoi( bs );

	for ( ban = p = g_admin_bans, i = 1; ban && i < bnum;
	      p = ban, ban = ban->next, i++ ) {; }

	if ( i != bnum || !ban )
	{
		ADMP( "^3unban: ^7invalid ban#\n" );
		return qfalse;
	}

	if ( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
	     ( ban->expires == 0 || ( ban->expires - time > MAX( 1,
	                              G_admin_parse_time( g_adminMaxBan.string ) ) ) ) )
	{
		ADMP( "^3unban: ^7you cannot remove permanent bans\n" );
		return qfalse;
	}

	admin_log( va( "%d (%s) \"%s" S_COLOR_WHITE "\": \"%s" S_COLOR_WHITE "\": [%s]",
	               ban->expires ? ban->expires - time : 0, ban->guid, ban->name, ban->reason,
	               ban->ip.str ) );
	AP( va( "print \"^3unban: ^7ban #%d for %s^7 has been removed by %s\n\"",
	        bnum,
	        ban->name,
	        ( ent ) ? ent->client->pers.netname : "console" ) );

	if ( p == ban )
	{
		g_admin_bans = ban->next;
	}
	else
	{
		p->next = ban->next;
	}

	BG_Free( ban );
	G_admin_writeconfig();
	return qtrue;
}

qboolean G_admin_adjustban( gentity_t *ent )
{
	int           bnum;
	int           length, maximum;
	int           expires;
	int           time = trap_RealTime( NULL );
	char          duration[ MAX_DURATION_LENGTH ] = { "" };
	char          *reason;
	char          bs[ 5 ];
	char          secs[ MAX_TOKEN_CHARS ];
	char          mode = '\0';
	g_admin_ban_t *ban;
	int           mask = 0;
	int           i;
	int           skiparg = 0;

	if ( trap_Argc() < 3 )
	{
		ADMP( "^3adjustban: ^7usage: adjustban [ban#] [/mask] [duration] [reason]"
		      "\n" );
		return qfalse;
	}

	trap_Argv( 1, bs, sizeof( bs ) );
	bnum = atoi( bs );

	for ( ban = g_admin_bans, i = 1; ban && i < bnum; ban = ban->next, i++ ) {; }

	if ( i != bnum || !ban )
	{
		ADMP( "^3adjustban: ^7invalid ban#\n" );
		return qfalse;
	}

	maximum = MAX( 1, G_admin_parse_time( g_adminMaxBan.string ) );

	if ( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
	     ( ban->expires == 0 || ban->expires - time > maximum ) )
	{
		ADMP( "^3adjustban: ^7you cannot modify permanent bans\n" );
		return qfalse;
	}

	trap_Argv( 2, secs, sizeof( secs ) );

	if ( secs[ 0 ] == '/' )
	{
		int max = ban->ip.type == IPv6 ? 128 : 32;
		int min = ent ? max / 2 : 1;
		mask = atoi( secs + 1 );

		if ( mask < min || mask > max )
		{
			ADMP( va( "^3adjustban: ^7invalid netmask (%d is not one of %d-%d)\n",
			          mask, min, max ) );
			return qfalse;
		}

		trap_Argv( 3 + skiparg++, secs, sizeof( secs ) );
	}

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
				ADMP( "^3adjustban: ^7new duration must be explicit\n" );
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
				ADMP( "^3adjustban: ^7ban duration must be positive\n" );
				return qfalse;
			}
		}
		else
		{
			length = expires = 0;
		}

		if ( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
		     ( length == 0 || length > maximum ) )
		{
			ADMP( "^3adjustban: ^7you may not issue permanent bans\n" );
			expires = time + maximum;
		}

		ban->expires = expires;
		G_admin_duration( ( expires ) ? expires - time : -1, duration,
		                  sizeof( duration ) );
	}

	if ( mask )
	{
		char *p = strchr( ban->ip.str, '/' );

		if ( !p )
		{
			p = ban->ip.str + strlen( ban->ip.str );
		}

		if ( mask == ( ban->ip.type == IPv6 ? 128 : 32 ) )
		{
			*p = '\0';
		}
		else
		{
			Com_sprintf( p, sizeof( ban->ip.str ) - ( p - ban->ip.str ), "/%d", mask );
		}

		ban->ip.mask = mask;
	}

	reason = ConcatArgs( 3 + skiparg );

	if ( *reason )
	{
		Q_strncpyz( ban->reason, reason, sizeof( ban->reason ) );
	}

	admin_log( va( "%d (%s) \"%s" S_COLOR_WHITE "\": \"%s" S_COLOR_WHITE "\": [%s]",
	               ban->expires ? ban->expires - time : 0, ban->guid, ban->name, ban->reason,
	               ban->ip.str ) );
	AP( va( "print \"^3adjustban: ^7ban #%d for %s^7 has been updated by %s^7 "
	        "%s%s%s%s%s%s\n\"",
	        bnum,
	        ban->name,
	        ( ent ) ? ent->client->pers.netname : "console",
	        ( mask ) ?
	        va( "netmask: /%d%s", mask,
	            ( length >= 0 || *reason ) ? ", " : "" ) : "",
		        ( length >= 0 ) ? "duration: " : "",
		        duration,
		        ( length >= 0 && *reason ) ? ", " : "",
		        ( *reason ) ? "reason: " : "",
		        reason ) );

	if ( ent )
{
		Q_strncpyz( ban->banner, ent->client->pers.netname, sizeof( ban->banner ) );
	}

	G_admin_writeconfig();
	return qtrue;
}

qboolean G_admin_putteam( gentity_t *ent )
{
	int       pid;
	char      name[ MAX_NAME_LENGTH ], team[ sizeof( "spectators" ) ],
	          err[ MAX_STRING_CHARS ];
	gentity_t *vic;
	team_t    teamnum = TEAM_NONE;

	trap_Argv( 1, name, sizeof( name ) );
	trap_Argv( 2, team, sizeof( team ) );

	if ( trap_Argc() < 3 )
	{
		ADMP( "^3putteam: ^7usage: putteam [name] [h|a|s]\n" );
		return qfalse;
	}

	if ( ( pid = G_ClientNumberFromString( name, err, sizeof( err ) ) ) == -1 )
	{
		ADMP( va( "^3putteam: ^7%s", err ) );
		return qfalse;
	}

	vic = &g_entities[ pid ];

	if ( !admin_higher( ent, vic ) )
	{
		ADMP( "^3putteam: ^7sorry, but your intended victim has a higher "
		      " admin level than you\n" );
		return qfalse;
	}

	teamnum = G_TeamFromString( team );

	if ( teamnum == NUM_TEAMS )
	{
		ADMP( va( "^3putteam: ^7unknown team %s\n", team ) );
		return qfalse;
	}

	if ( vic->client->pers.teamSelection == teamnum )
	{
		return qfalse;
	}

	admin_log( va( "%d (%s) \"%s" S_COLOR_WHITE "\"", pid, vic->client->pers.guid,
	               vic->client->pers.netname ) );
	G_ChangeTeam( vic, teamnum );

	AP( va( "print \"^3putteam: ^7%s^7 put %s^7 on to the %s team\n\"",
	        ( ent ) ? ent->client->pers.netname : "console",
	        vic->client->pers.netname, BG_TeamName( teamnum ) ) );
	return qtrue;
}

qboolean G_admin_changemap( gentity_t *ent )
{
	char map[ MAX_QPATH ];
	char layout[ MAX_QPATH ] = { "" };

	if ( trap_Argc() < 2 )
	{
		ADMP( "^3changemap: ^7usage: changemap [map] (layout)\n" );
		return qfalse;
	}

	trap_Argv( 1, map, sizeof( map ) );

	if ( !trap_FS_FOpenFile( va( "maps/%s.bsp", map ), NULL, FS_READ ) )
	{
		ADMP( va( "^3changemap: ^7invalid map name '%s'\n", map ) );
		return qfalse;
	}

	if ( trap_Argc() > 2 )
	{
		trap_Argv( 2, layout, sizeof( layout ) );

		if ( !Q_stricmp( layout, "*BUILTIN*" ) ||
		     trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, layout ),
		                        NULL, FS_READ ) > 0 )
		{
			trap_Cvar_Set( "g_layouts", layout );
		}
		else
		{
			ADMP( va( "^3changemap: ^7invalid layout name '%s'\n", layout ) );
			return qfalse;
		}
	}

	admin_log( map );
	admin_log( layout );

	trap_SendConsoleCommand( EXEC_APPEND, va( "map %s", map ) );
	level.restarted = qtrue;
	AP( va( "print \"^3changemap: ^7map '%s' started by %s^7 %s\n\"", map,
	        ( ent ) ? ent->client->pers.netname : "console",
	        ( layout[ 0 ] ) ? va( "(forcing layout '%s')", layout ) : "" ) );
	return qtrue;
}

qboolean G_admin_mute( gentity_t *ent )
{
	char      name[ MAX_NAME_LENGTH ];
	char      command[ MAX_ADMIN_CMD_LEN ];
	namelog_t *vic;

	trap_Argv( 0, command, sizeof( command ) );

	if ( trap_Argc() < 2 )
	{
		ADMP( va( "^3%s: ^7usage: %s [name|slot#]\n", command, command ) );
		return qfalse;
	}

	trap_Argv( 1, name, sizeof( name ) );

	if ( !( vic = G_NamelogFromString( ent, name ) ) )
	{
		ADMP( va( "^3%s: ^7no match\n", command ) );
		return qfalse;
	}

	if ( ent && !admin_higher_admin( ent->client->pers.admin,
	                                 G_admin_admin( vic->guid ) ) )
	{
		ADMP( va( "^3%s: ^7sorry, but your intended victim has a higher admin"
		          " level than you\n", command ) );
		return qfalse;
	}

	if ( vic->muted )
	{
		if ( !Q_stricmp( command, "mute" ) )
		{
			ADMP( "^3mute: ^7player is already muted\n" );
			return qfalse;
		}

		vic->muted = qfalse;

		if ( vic->slot > -1 )
		{
			CPx( vic->slot, "cp \"^1You have been unmuted\"" );
		}

		AP( va( "print \"^3unmute: ^7%s^7 has been unmuted by %s\n\"",
		        vic->name[ vic->nameOffset ],
		        ( ent ) ? ent->client->pers.netname : "console" ) );
	}
	else
	{
		if ( !Q_stricmp( command, "unmute" ) )
		{
			ADMP( "^3unmute: ^7player is not currently muted\n" );
			return qfalse;
		}

		vic->muted = qtrue;

		if ( vic->slot > -1 )
		{
			CPx( vic->slot, "cp \"^1You've been muted\"" );
		}

		AP( va( "print \"^3mute: ^7%s^7 has been muted by ^7%s\n\"",
		        vic->name[ vic->nameOffset ],
		        ( ent ) ? ent->client->pers.netname : "console" ) );
	}

	admin_log( va( "%d (%s) \"%s" S_COLOR_WHITE "\"", vic->slot, vic->guid,
	               vic->name[ vic->nameOffset ] ) );
	return qtrue;
}

qboolean G_admin_denybuild( gentity_t *ent )
{
	char      name[ MAX_NAME_LENGTH ];
	char      command[ MAX_ADMIN_CMD_LEN ];
	namelog_t *vic;

	trap_Argv( 0, command, sizeof( command ) );

	if ( trap_Argc() < 2 )
	{
		ADMP( va( "^3%s: ^7usage: %s [name|slot#]\n", command, command ) );
		return qfalse;
	}

	trap_Argv( 1, name, sizeof( name ) );

	if ( !( vic = G_NamelogFromString( ent, name ) ) )
	{
		ADMP( va( "^3%s: ^7no match\n", command ) );
		return qfalse;
	}

	if ( ent && !admin_higher_admin( ent->client->pers.admin,
	                                 G_admin_admin( vic->guid ) ) )
	{
		ADMP( va( "^3%s: ^7sorry, but your intended victim has a higher admin"
		          " level than you\n", command ) );
		return qfalse;
	}

	if ( vic->denyBuild )
	{
		if ( !Q_stricmp( command, "denybuild" ) )
		{
			ADMP( "^3denybuild: ^7player already has no building rights\n" );
			return qfalse;
		}

		vic->denyBuild = qfalse;

		if ( vic->slot > -1 )
		{
			CPx( vic->slot, "cp \"^1You've regained your building rights\"" );
		}

		AP( va(
		      "print \"^3allowbuild: ^7building rights for ^7%s^7 restored by %s\n\"",
		      vic->name[ vic->nameOffset ],
		      ( ent ) ? ent->client->pers.netname : "console" ) );
	}
	else
	{
		if ( !Q_stricmp( command, "allowbuild" ) )
		{
			ADMP( "^3allowbuild: ^7player already has building rights\n" );
			return qfalse;
		}

		vic->denyBuild = qtrue;

		if ( vic->slot > -1 )
		{
			level.clients[ vic->slot ].ps.stats[ STAT_BUILDABLE ] = BA_NONE;
			CPx( vic->slot, "cp \"^1You've lost your building rights\"" );
		}

		AP( va(
		      "print \"^3denybuild: ^7building rights for ^7%s^7 revoked by ^7%s\n\"",
		      vic->name[ vic->nameOffset ],
		      ( ent ) ? ent->client->pers.netname : "console" ) );
	}

	admin_log( va( "%d (%s) \"%s" S_COLOR_WHITE "\"", vic->slot, vic->guid,
	               vic->name[ vic->nameOffset ] ) );
	return qtrue;
}

qboolean G_admin_listadmins( gentity_t *ent )
{
	int  i;
	char search[ MAX_NAME_LENGTH ] = { "" };
	char s[ MAX_NAME_LENGTH ] = { "" };
	int  start = MAX_CLIENTS;

	if ( trap_Argc() == 3 )
	{
		trap_Argv( 2, s, sizeof( s ) );
		start = atoi( s );
	}

	if ( trap_Argc() > 1 )
	{
		trap_Argv( 1, s, sizeof( s ) );
		i = 0;

		if ( trap_Argc() == 2 )
		{
			i = s[ 0 ] == '-';

			for ( ; isdigit( s[ i ] ); i++ ) {; }
		}

		if ( i && !s[ i ] )
		{
			start = atoi( s );
		}
		else
		{
			G_SanitiseString( s, search, sizeof( search ) );
		}
	}

	admin_listadmins( ent, start, search );
	return qtrue;
}

qboolean G_admin_listlayouts( gentity_t *ent )
{
	char list[ MAX_CVAR_VALUE_STRING ];
	char map[ MAX_QPATH ];
	int  count = 0;
	char *s;
	char layout[ MAX_QPATH ] = { "" };
	int  i = 0;

	if ( trap_Argc() == 2 )
	{
		trap_Argv( 1, map, sizeof( map ) );
	}
	else
	{
		trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
	}

	count = G_LayoutList( map, list, sizeof( list ) );
	ADMBP_begin();
	ADMBP( va( "^3listlayouts:^7 %d layouts found for '%s':\n", count, map ) );
	s = &list[ 0 ];

	while ( *s )
	{
		if ( *s == ' ' )
		{
			ADMBP( va( " %s\n", layout ) );
			layout[ 0 ] = '\0';
			i = 0;
		}
		else if ( i < sizeof( layout ) - 2 )
		{
			layout[ i++ ] = *s;
			layout[ i ] = '\0';
		}

		s++;
	}

	if ( layout[ 0 ] )
	{
		ADMBP( va( " %s\n", layout ) );
	}

	ADMBP_end();
	return qtrue;
}

qboolean G_admin_listplayers( gentity_t *ent )
{
	int             i, j;
	gclient_t       *p;
	char            c, t; // color and team letter
	char            *registeredname;
	char            lname[ MAX_NAME_LENGTH ];
	char            muted, denied;
	int             colorlen;
	int             authed = 1;
	char            namecleaned[ MAX_NAME_LENGTH ];
	char            name2cleaned[ MAX_NAME_LENGTH ];
	g_admin_level_t *l;
	g_admin_level_t *d = G_admin_level( 0 );
	qboolean        hint;
	qboolean        canset = G_admin_permission( ent, "setlevel" );

	ADMBP_begin();
	ADMBP( va( "^3listplayers: ^7%d players connected:\n",
	           level.numConnectedClients ) );

	for ( i = 0; i < level.maxclients; i++ )
	{
		p = &level.clients[ i ];

		if ( p->pers.connected == CON_DISCONNECTED )
		{
			continue;
		}

		if ( p->pers.connected == CON_CONNECTING )
		{
			t = 'C';
			c = COLOR_YELLOW;
		}
		else
		{
			t = toupper( * ( BG_TeamName( p->pers.teamSelection ) ) );

			if ( p->pers.teamSelection == TEAM_HUMANS )
			{
				c = COLOR_CYAN;
			}
			else if ( p->pers.teamSelection == TEAM_ALIENS )
			{
				c = COLOR_RED;
			}
			else
			{
				c = COLOR_WHITE;
			}
		}

		muted = p->pers.namelog->muted ? 'M' : ' ';
		denied = p->pers.namelog->denyBuild ? 'B' : ' ';

		l = d;
		registeredname = NULL;
		hint = canset;

		if ( p->pers.admin )
		{
			authed = p->pers.pubkey_authenticated;

			if ( hint )
			{
				hint = admin_higher( ent, &g_entities[ i ] );
			}

			if ( hint || !G_admin_permission( &g_entities[ i ], ADMF_INCOGNITO ) )
			{
				l = G_admin_level( p->pers.admin->level );
				G_SanitiseString( p->pers.netname, namecleaned,
				                  sizeof( namecleaned ) );
				G_SanitiseString( p->pers.admin->name,
				                  name2cleaned, sizeof( name2cleaned ) );

				if ( Q_stricmp( namecleaned, name2cleaned ) )
				{
					registeredname = p->pers.admin->name;
				}
			}
		}

		if ( l )
		{
			Q_strncpyz( lname, l->name, sizeof( lname ) );
		}

		for ( colorlen = j = 0; lname[ j ]; j++ )
		{
			if ( Q_IsColorString( &lname[ j ] ) )
			{
				colorlen += 2;
			}
		}

		ADMBP( va( "%2i ^%c%c^7 %-2i^2%c^7 %*s^7 ^1%c%c^7 %s^7 %s%s%s %s\n",
		           i,
		           c,
		           t,
		           l ? l->level : 0,
		           hint ? '*' : ' ',
		           admin_level_maxname + colorlen,
		           lname,
		           muted,
		           denied,
		           p->pers.netname,
		           ( registeredname ) ? "(a.k.a. " : "",
		           ( registeredname ) ? registeredname : "",
		           ( registeredname ) ? S_COLOR_WHITE ")" : "",
		           ( !authed ) ? S_COLOR_RED "NOT AUTHED" : "" ) );
	}

	ADMBP_end();
	return qtrue;
}

static qboolean ban_matchip( void *ban, const void *ip )
{
	return G_AddressCompare( & ( ( g_admin_ban_t * ) ban )->ip, ( addr_t * ) ip ) ||
	       G_AddressCompare( ( addr_t * ) ip, & ( ( g_admin_ban_t * ) ban )->ip );
}

static qboolean ban_matchname( void *ban, const void *name )
{
	char match[ MAX_NAME_LENGTH ];

	G_SanitiseString( ( ( g_admin_ban_t * ) ban )->name, match, sizeof( match ) );
	return strstr( match, ( const char * ) name ) != NULL;
}

static void ban_out( void *ban, char *str )
{
	int           i;
	int           colorlen1 = 0;
	char          duration[ MAX_DURATION_LENGTH ];
	char          *d_color = S_COLOR_WHITE;
	char          date[ 11 ];
	g_admin_ban_t *b = ( g_admin_ban_t * ) ban;
	int           t = trap_RealTime( NULL );
	char          *made = b->made;

	for ( i = 0; b->name[ i ]; i++ )
	{
		if ( Q_IsColorString( &b->name[ i ] ) )
		{
			colorlen1 += 2;
		}
	}

	// only print out the the date part of made
	date[ 0 ] = '\0';

	for ( i = 0; *made && *made != ' ' && i < sizeof( date ) - 1; i++ )
	{
		date[ i ] = *made++;
	}

	date[ i ] = 0;

	if ( !b->expires || b->expires - t > 0 )
	{
		G_admin_duration( b->expires ? b->expires - t : -1,
		                  duration, sizeof( duration ) );
	}
	else
	{
		Q_strncpyz( duration, "expired", sizeof( duration ) );
		d_color = S_COLOR_CYAN;
	}

	Com_sprintf( str, MAX_STRING_CHARS, "%-*s %s%-15s " S_COLOR_WHITE "%-8s %s"
	             "\n     \\__ %s%-*s " S_COLOR_WHITE "%s",
	             MAX_NAME_LENGTH + colorlen1 - 1, b->name,
	             ( strchr( b->ip.str, '/' ) ) ? S_COLOR_RED : S_COLOR_WHITE,
	             b->ip.str,
	             date,
	             b->banner,
	             d_color,
	             MAX_DURATION_LENGTH - 1,
	             duration,
	             b->reason );
}

qboolean G_admin_showbans( gentity_t *ent )
{
	int      i;
	int      start = 1;
	char     filter[ MAX_NAME_LENGTH ] = { "" };
	char     name_match[ MAX_NAME_LENGTH ] = { "" };
	qboolean ipmatch = qfalse;
	addr_t   ip;

	if ( trap_Argc() == 3 )
	{
		trap_Argv( 2, filter, sizeof( filter ) );
		start = atoi( filter );
	}

	if ( trap_Argc() > 1 )
	{
		trap_Argv( 1, filter, sizeof( filter ) );
		i = 0;

		if ( trap_Argc() == 2 )
		{
			for ( i = filter[ 0 ] == '-'; isdigit( filter[ i ] ); i++ ) {; }
		}

		if ( !filter[ i ] )
		{
			start = atoi( filter );
		}
		else if ( !( ipmatch = G_AddressParse( filter, &ip ) ) )
		{
			G_SanitiseString( filter, name_match, sizeof( name_match ) );
		}
	}

	admin_search( ent, "showbans", "bans",
	              ipmatch ? ban_matchip : ban_matchname,
	              ban_out, g_admin_bans,
	              ipmatch ? ( void * ) &ip : ( void * ) name_match,
	              start, 1, MAX_ADMIN_SHOWBANS );
	return qtrue;
}

qboolean G_admin_adminhelp( gentity_t *ent )
{
	g_admin_command_t *c;

	if ( trap_Argc() < 2 )
	{
		int i;
		int count = 0;

		ADMBP_begin();

		for ( i = 0; i < adminNumCmds; i++ )
		{
			if ( G_admin_permission( ent, g_admin_cmds[ i ].flag ) )
			{
				ADMBP( va( "^3%-13s", g_admin_cmds[ i ].keyword ) );
				count++;

				// show 6 commands per line
				if ( count % 6 == 0 )
				{
					ADMBP( "\n" );
				}
			}
		}

		for ( c = g_admin_commands; c; c = c->next )
		{
			if ( !G_admin_permission( ent, c->flag ) )
			{
				continue;
			}

			ADMBP( va( "^1%-13s", c->command ) );
			count++;

			// show 6 commands per line
			if ( count % 6 == 0 )
			{
				ADMBP( "\n" );
			}
		}

		if ( count % 6 )
		{
			ADMBP( "\n" );
		}

		ADMBP( va( "^3adminhelp: ^7%i available commands\n", count ) );
		ADMBP( "run adminhelp [^3command^7] for adminhelp with a specific command.\n" );
		ADMBP_end();

		return qtrue;
	}
	else
	{
		//!adminhelp param
		char          param[ MAX_ADMIN_CMD_LEN ];
		g_admin_cmd_t *admincmd;
		qboolean      denied = qfalse;

		trap_Argv( 1, param, sizeof( param ) );
		ADMBP_begin();

		if ( ( c = G_admin_command( param ) ) )
		{
			if ( G_admin_permission( ent, c->flag ) )
			{
				ADMBP( va( "^3adminhelp: ^7help for '%s':\n", c->command ) );
				ADMBP( va( " ^3Description: ^7%s\n", c->desc ) );
				ADMBP( va( " ^3Syntax: ^7%s\n", c->command ) );
				ADMBP( va( " ^3Flag: ^7'%s'\n", c->flag ) );
				ADMBP_end();
				return qtrue;
			}

			denied = qtrue;
		}

		if ( ( admincmd = G_admin_cmd( param ) ) )
		{
			if ( G_admin_permission( ent, admincmd->flag ) )
			{
				ADMBP( va( "^3adminhelp: ^7help for '%s':\n", admincmd->keyword ) );
				ADMBP( va( " ^3Description: ^7%s\n", admincmd->function ) );
				ADMBP( va( " ^3Syntax: ^7%s %s\n", admincmd->keyword,
				           admincmd->syntax ) );
				ADMBP( va( " ^3Flag: ^7'%s'\n", admincmd->flag ) );
				ADMBP_end();
				return qtrue;
			}

			denied = qtrue;
		}

		ADMBP( va( "^3adminhelp: ^7%s '%s'\n",
		           denied ? "you do not have permission to use" : "no help found for",
		           param ) );
		ADMBP_end();
		return qfalse;
	}
}

qboolean G_admin_admintest( gentity_t *ent )
{
	g_admin_level_t *l;

	if ( !ent )
	{
		ADMP( "^3admintest: ^7you are on the console.\n" );
		return qtrue;
	}

	l = G_admin_level( ent->client->pers.admin ? ent->client->pers.admin->level : 0 );

	AP( va( "print \"^3admintest: ^7%s^7 is a level %d admin %s%s^7%s\n\"",
	        ent->client->pers.netname,
	        l ? l->level : 0,
	        l ? "(" : "",
	        l ? l->name : "",
	        l ? ")" : "" ) );
	return qtrue;
}

qboolean G_admin_allready( gentity_t *ent )
{
	int       i = 0;
	gclient_t *cl;

	if ( !level.intermissiontime )
	{
		ADMP( "^3allready: ^7this command is only valid during intermission\n" );
		return qfalse;
	}

	for ( i = 0; i < g_maxclients.integer; i++ )
	{
		cl = level.clients + i;

		if ( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if ( cl->pers.teamSelection == TEAM_NONE )
		{
			continue;
		}

		cl->readyToExit = qtrue;
	}

	AP( va( "print \"^3allready:^7 %s^7 says everyone is READY now\n\"",
	        ( ent ) ? ent->client->pers.netname : "console" ) );
	return qtrue;
}

qboolean G_admin_endvote( gentity_t *ent )
{
	char     teamName[ sizeof( "spectators" ) ] = { "s" };
	char     command[ MAX_ADMIN_CMD_LEN ];
	team_t   team;
	qboolean cancel;
	char     *msg;

	trap_Argv( 0, command, sizeof( command ) );
	cancel = !Q_stricmp( command, "cancelvote" );

	if ( trap_Argc() == 2 )
	{
		trap_Argv( 1, teamName, sizeof( teamName ) );
	}

	team = G_TeamFromString( teamName );

	if ( team == NUM_TEAMS )
	{
		ADMP( va( "^3%s: ^7invalid team '%s'\n", command, teamName ) );
		return qfalse;
	}

	msg = va( "print \"^3%s: ^7%s^7 decided that everyone voted %s\n\"",
	          command, ( ent ) ? ent->client->pers.netname : "console",
	          cancel ? "No" : "Yes" );

	if ( !level.voteTime[ team ] )
	{
		ADMP( va( "^3%s: ^7no vote in progress\n", command ) );
		return qfalse;
	}

	admin_log( BG_TeamName( team ) );
	level.voteNo[ team ] = cancel ? level.numVotingClients[ team ] : 0;
	level.voteYes[ team ] = cancel ? 0 : level.numVotingClients[ team ];
	G_CheckVote( team );

	if ( !Q_strncmp( level.voteDisplayString[ team ], "Extend", 6 ) &&
	     level.extend_vote_count > 0 )
	{
		level.extend_vote_count--;
	}

	if ( team == TEAM_NONE )
	{
		AP( msg );
	}
	else
	{
		G_TeamCommand( team, msg );
	}

	return qtrue;
}

qboolean G_admin_spec999( gentity_t *ent )
{
	int       i;
	gentity_t *vic;

	for ( i = 0; i < level.maxclients; i++ )
	{
		vic = &g_entities[ i ];

		if ( !vic->client )
		{
			continue;
		}

		if ( vic->client->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if ( vic->client->pers.teamSelection == TEAM_NONE )
		{
			continue;
		}

		if ( vic->client->ps.ping == 999 )
		{
			G_ChangeTeam( vic, TEAM_NONE );
			AP( va( "print \"^3spec999: ^7%s^7 moved %s^7 to spectators\n\"",
			        ( ent ) ? ent->client->pers.netname : "console",
			        vic->client->pers.netname ) );
		}
	}

	return qtrue;
}

qboolean G_admin_rename( gentity_t *ent )
{
	int       pid;
	char      name[ MAX_NAME_LENGTH ];
	char      newname[ MAX_NAME_LENGTH ];
	char      err[ MAX_STRING_CHARS ];
	char      userinfo[ MAX_INFO_STRING ];
	gentity_t *victim = NULL;

	if ( trap_Argc() < 3 )
	{
		ADMP( "^3rename: ^7usage: rename [name] [newname]\n" );
		return qfalse;
	}

	trap_Argv( 1, name, sizeof( name ) );
	Q_strncpyz( newname, ConcatArgs( 2 ), sizeof( newname ) );

	if ( ( pid = G_ClientNumberFromString( name, err, sizeof( err ) ) ) == -1 )
	{
		ADMP( va( "^3rename: ^7%s", err ) );
		return qfalse;
	}

	victim = &g_entities[ pid ];

	if ( !admin_higher( ent, victim ) )
	{
		ADMP( "^3rename: ^7sorry, but your intended victim has a higher admin"
		      " level than you\n" );
		return qfalse;
	}

	if ( !G_admin_name_check( victim, newname, err, sizeof( err ) ) )
	{
		ADMP( va( "^3rename: ^7%s\n", err ) );
		return qfalse;
	}

	if ( victim->client->pers.connected != CON_CONNECTED )
	{
		ADMP( "^3rename: ^7sorry, but your intended victim is still connecting\n" );
		return qfalse;
	}

	admin_log( va( "%d (%s) \"%s" S_COLOR_WHITE "\"", pid,
	               victim->client->pers.guid, victim->client->pers.netname ) );
	admin_log( newname );
	trap_GetUserinfo( pid, userinfo, sizeof( userinfo ) );
	AP( va( "print \"^3rename: ^7%s^7 has been renamed to %s^7 by %s\n\"",
	        victim->client->pers.netname,
	        newname,
	        ( ent ) ? ent->client->pers.netname : "console" ) );
	Info_SetValueForKey( userinfo, "name", newname );
	trap_SetUserinfo( pid, userinfo );
	ClientUserinfoChanged( pid, qtrue );
	return qtrue;
}

qboolean G_admin_restart( gentity_t *ent )
{
	char      layout[ MAX_CVAR_VALUE_STRING ] = { "" };
	char      teampref[ MAX_STRING_CHARS ] = { "" };
	int       i;
	gclient_t *cl;

	if ( trap_Argc() > 1 )
	{
		char map[ MAX_QPATH ];

		trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
		trap_Argv( 1, layout, sizeof( layout ) );

		// Figure out which argument is which
		if ( Q_stricmp( layout, "keepteams" ) &&
		     Q_stricmp( layout, "keepteamslock" ) &&
		     Q_stricmp( layout, "switchteams" ) &&
		     Q_stricmp( layout, "switchteamslock" ) )
		{
			if ( !Q_stricmp( layout, "*BUILTIN*" ) ||
			     trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, layout ),
			                        NULL, FS_READ ) > 0 )
			{
				trap_Cvar_Set( "g_layouts", layout );
			}
			else
			{
				ADMP( va( "^3restart: ^7layout '%s' does not exist\n", layout ) );
				return qfalse;
			}
		}
		else
		{
			layout[ 0 ] = '\0';
			trap_Argv( 1, teampref, sizeof( teampref ) );
		}
	}

	if ( trap_Argc() > 2 )
	{
		trap_Argv( 2, teampref, sizeof( teampref ) );
	}

	admin_log( layout );
	admin_log( teampref );

	if ( !Q_stricmpn( teampref, "keepteams", 9 ) )
	{
		for ( i = 0; i < g_maxclients.integer; i++ )
		{
			cl = level.clients + i;

			if ( cl->pers.connected != CON_CONNECTED )
			{
				continue;
			}

			if ( cl->pers.teamSelection == TEAM_NONE )
			{
				continue;
			}

			cl->sess.restartTeam = cl->pers.teamSelection;
		}
	}
	else if ( !Q_stricmpn( teampref, "switchteams", 11 ) )
	{
		for ( i = 0; i < g_maxclients.integer; i++ )
		{
			cl = level.clients + i;

			if ( cl->pers.connected != CON_CONNECTED )
			{
				continue;
			}

			if ( cl->pers.teamSelection == TEAM_HUMANS )
			{
				cl->sess.restartTeam = TEAM_ALIENS;
			}
			else if ( cl->pers.teamSelection == TEAM_ALIENS )
			{
				cl->sess.restartTeam = TEAM_HUMANS;
			}
		}
	}

	if ( !Q_stricmp( teampref, "switchteamslock" ) ||
	     !Q_stricmp( teampref, "keepteamslock" ) )
	{
		trap_Cvar_Set( "g_lockTeamsAtStart", "1" );
	}

	trap_SendConsoleCommand( EXEC_APPEND, "map_restart" );

	AP( va( "print \"^3restart: ^7map restarted by %s %s %s\n\"",
	        ( ent ) ? ent->client->pers.netname : "console",
	        ( layout[ 0 ] ) ? va( "^7(forcing layout '%s^7')", layout ) : "",
	        ( teampref[ 0 ] ) ? va( "^7(with teams option: '%s^7')", teampref ) : "" ) );
	return qtrue;
}

qboolean G_admin_nextmap( gentity_t *ent )
{
	AP( va( "print \"^3nextmap: ^7%s^7 decided to load the next map\n\"",
	        ( ent ) ? ent->client->pers.netname : "console" ) );
	level.lastWin = TEAM_NONE;
	trap_SetConfigstring( CS_WINNER, "Evacuation" );
	LogExit( va( "nextmap was run by %s",
	             ( ent ) ? ent->client->pers.netname : "console" ) );
	return qtrue;
}

static qboolean namelog_matchip( void *namelog, const void *ip )
{
	int       i;
	namelog_t *n = ( namelog_t * ) namelog;

	for ( i = 0; i < MAX_NAMELOG_ADDRS && n->ip[ i ].str[ 0 ]; i++ )
	{
		if ( G_AddressCompare( &n->ip[ i ], ( addr_t * ) ip ) ||
		     G_AddressCompare( ( addr_t * ) ip, &n->ip[ i ] ) )
		{
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean namelog_matchname( void *namelog, const void *name )
{
	char      match[ MAX_NAME_LENGTH ];
	int       i;
	namelog_t *n = ( namelog_t * ) namelog;

	for ( i = 0; i < MAX_NAMELOG_NAMES && n->name[ i ][ 0 ]; i++ )
	{
		G_SanitiseString( n->name[ i ], match, sizeof( match ) );

		if ( strstr( match, ( const char * ) name ) )
		{
			return qtrue;
		}
	}

	return qfalse;
}

static void namelog_out( void *namelog, char *str )
{
	namelog_t  *n = ( namelog_t * ) namelog;
	char       *p = str;
	int        l, l2 = MAX_STRING_CHARS, i;
	const char *scolor;

	if ( n->slot > -1 )
	{
		scolor = S_COLOR_YELLOW;
		l = Q_snprintf( p, l2, "%s%-2d", scolor, n->slot );
		p += l;
		l2 -= l;
	}
	else
	{
		*p++ = '-';
		*p++ = ' ';
		*p = '\0';
		l2 -= 2;
		scolor = S_COLOR_WHITE;
	}

	for ( i = 0; i < MAX_NAMELOG_ADDRS && n->ip[ i ].str[ 0 ]; i++ )
	{
		l = Q_snprintf( p, l2, " %s", n->ip[ i ].str );
		p += l;
		l2 -= l;
	}

	for ( i = 0; i < MAX_NAMELOG_NAMES && n->name[ i ][ 0 ]; i++ )
	{
		l = Q_snprintf( p, l2, " '" S_COLOR_WHITE "%s%s'%s", n->name[ i ], scolor,
		                i == n->nameOffset ? "*" : "" );
		p += l;
		l2 -= l;
	}
}

qboolean G_admin_namelog( gentity_t *ent )
{
	char     search[ MAX_NAME_LENGTH ] = { "" };
	char     s2[ MAX_NAME_LENGTH ] = { "" };
	addr_t   ip;
	qboolean ipmatch = qfalse;
	int      start = MAX_CLIENTS, i;

	if ( trap_Argc() == 3 )
	{
		trap_Argv( 2, search, sizeof( search ) );
		start = atoi( search );
	}

	if ( trap_Argc() > 1 )
	{
		trap_Argv( 1, search, sizeof( search ) );
		i = 0;

		if ( trap_Argc() == 2 )
		{
			for ( i = search[ 0 ] == '-'; isdigit( search[ i ] ); i++ ) {; }
		}

		if ( !search[ i ] )
		{
			start = atoi( search );
		}
		else if ( !( ipmatch = G_AddressParse( search, &ip ) ) )
		{
			G_SanitiseString( search, s2, sizeof( s2 ) );
		}
	}

	admin_search( ent, "namelog", "recent players",
	              ipmatch ? namelog_matchip : namelog_matchname, namelog_out, level.namelogs,
	              ipmatch ? ( void * ) &ip : s2, start, MAX_CLIENTS, MAX_ADMIN_LISTITEMS );
	return qtrue;
}

/*
==================
G_NamelogFromString

This is similar to G_ClientNumberFromString but for namelog instead
Returns NULL if not exactly 1 match
==================
*/
namelog_t *G_NamelogFromString( gentity_t *ent, char *s )
{
	namelog_t *p, *m = NULL;
	int       i, found = 0;
	char      n2[ MAX_NAME_LENGTH ] = { "" };
	char      s2[ MAX_NAME_LENGTH ] = { "" };

	if ( !s[ 0 ] )
	{
		return NULL;
	}

	// if a number is provided, it is a clientnum or namelog id
	for ( i = 0; s[ i ] && isdigit( s[ i ] ); i++ ) {; }

	if ( !s[ i ] )
	{
		i = atoi( s );

		if ( i >= 0 && i < level.maxclients )
		{
			if ( level.clients[ i ].pers.connected != CON_DISCONNECTED )
			{
				return level.clients[ i ].pers.namelog;
			}
		}
		else if ( i >= MAX_CLIENTS )
		{
			for ( p = level.namelogs; p; p = p->next )
			{
				if ( p->id == i )
				{
					break;
				}
			}

			if ( p )
			{
				return p;
			}
		}

		return NULL;
	}

	// check for a name match
	G_SanitiseString( s, s2, sizeof( s2 ) );

	for ( p = level.namelogs; p; p = p->next )
	{
		for ( i = 0; i < MAX_NAMELOG_NAMES && p->name[ i ][ 0 ]; i++ )
		{
			G_SanitiseString( p->name[ i ], n2, sizeof( n2 ) );

			// if this is an exact match to a current player
			if ( i == p->nameOffset && p->slot > -1 && !strcmp( s2, n2 ) )
			{
				return p;
			}

			if ( strstr( n2, s2 ) )
			{
				m = p;
			}
		}

		if ( m == p )
		{
			found++;
		}
	}

	if ( found == 1 )
	{
		return m;
	}

	if ( found > 1 )
	{
		admin_search( ent, "namelog", "recent players", namelog_matchname,
		              namelog_out, level.namelogs, s2, 0, MAX_CLIENTS, -1 );
	}

	return NULL;
}

qboolean G_admin_lock( gentity_t *ent )
{
	char     command[ MAX_ADMIN_CMD_LEN ];
	char     teamName[ sizeof( "aliens" ) ];
	team_t   team;
	qboolean lock, fail = qfalse;

	trap_Argv( 0, command, sizeof( command ) );

	if ( trap_Argc() < 2 )
	{
		ADMP( va( "^3%s: ^7usage: %s [a|h]\n", command, command ) );
		return qfalse;
	}

	lock = !Q_stricmp( command, "lock" );
	trap_Argv( 1, teamName, sizeof( teamName ) );
	team = G_TeamFromString( teamName );

	if ( team == TEAM_ALIENS )
	{
		if ( level.alienTeamLocked == lock )
		{
			fail = qtrue;
		}
		else
		{
			level.alienTeamLocked = lock;
		}
	}
	else if ( team == TEAM_HUMANS )
	{
		if ( level.humanTeamLocked == lock )
		{
			fail = qtrue;
		}
		else
		{
			level.humanTeamLocked = lock;
		}
	}
	else
	{
		ADMP( va( "^3%s: ^7invalid team: '%s'\n", command, teamName ) );
		return qfalse;
	}

	if ( fail )
	{
		ADMP( va( "^3%s: ^7the %s team is %s locked\n",
		          command, BG_TeamName( team ), lock ? "already" : "not currently" ) );

		return qfalse;
	}

	admin_log( BG_TeamName( team ) );
	AP( va( "print \"^3%s: ^7the %s team has been %slocked by %s\n\"",
	        command, BG_TeamName( team ), lock ? "" : "un",
	        ent ? ent->client->pers.netname : "console" ) );

	return qtrue;
}

qboolean G_admin_builder( gentity_t *ent )
{
	vec3_t     forward, right, up;
	vec3_t     start, end, dist;
	trace_t    tr;
	gentity_t  *traceEnt;
	buildLog_t *log;
	int        i;

	if ( !ent )
	{
		ADMP( "^3builder: ^7console can't aim.\n" );
		return qfalse;
	}

	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	if ( ent->client->pers.teamSelection != TEAM_NONE &&
	     ent->client->sess.spectatorState == SPECTATOR_NOT )
	{
		CalcMuzzlePoint( ent, forward, right, up, start );
	}
	else
	{
		VectorCopy( ent->client->ps.origin, start );
	}

	VectorMA( start, 1000, forward, end );

	trap_Trace( &tr, start, NULL, NULL, end, ent->s.number, MASK_PLAYERSOLID );
	traceEnt = &g_entities[ tr.entityNum ];

	if ( tr.fraction < 1.0f && ( traceEnt->s.eType == ET_BUILDABLE ) )
	{
		if ( !G_admin_permission( ent, "buildlog" ) &&
		     ent->client->pers.teamSelection != TEAM_NONE &&
		     ent->client->pers.teamSelection != traceEnt->buildableTeam )
		{
			ADMP( "^3builder: ^7structure not owned by your team\n" );
			return qfalse;
		}

		for ( i = 0; i < level.numBuildLogs; i++ )
		{
			log = &level.buildLog[( level.buildId - i - 1 ) % MAX_BUILDLOG ];

			if ( log->fate != BF_CONSTRUCT || traceEnt->s.modelindex != log->modelindex )
			{
				continue;
			}

			VectorSubtract( traceEnt->s.pos.trBase, log->origin, dist );

			if ( VectorLengthSquared( dist ) < 2.0f )
			{
				char logid[ 20 ] = { "" };

				if ( G_admin_permission( ent, "buildlog" ) )
				{
					Com_sprintf( logid, sizeof( logid ), ", buildlog #%d",
					             MAX_CLIENTS + level.buildId - i - 1 );
				}

				ADMP( va( "^3builder: ^7%s built by %s^7%s\n",
				          BG_Buildable( log->modelindex )->humanName,
				          log->actor ?
				          log->actor->name[ log->actor->nameOffset ] :
				          "<world>",
				          logid ) );
				break;
			}
		}

		if ( i == level.numBuildLogs )
		{
			ADMP( va( "^3builder: ^7%s not in build log, possibly a layout item\n",
			          BG_Buildable( traceEnt->s.modelindex )->humanName ) );
		}
	}
	else
	{
		ADMP( "^3builder: ^7no structure found under crosshair\n" );
	}

	return qtrue;
}

qboolean G_admin_pause( gentity_t *ent )
{
	if ( !level.pausedTime )
	{
		AP( va( "print \"^3pause: ^7%s^7 paused the game.\n\"",
		        ( ent ) ? ent->client->pers.netname : "console" ) );
		level.pausedTime = 1;
		trap_SendServerCommand( -1, "cp \"The game has been paused. Please wait.\"" );
	}
	else
	{
		// Prevent accidental pause->unpause race conditions by two admins
		if ( level.pausedTime < 1000 )
		{
			ADMP( "^3pause: ^7Unpausing so soon assumed accidental and ignored.\n" );
			return qfalse;
		}

		AP( va( "print \"^3pause: ^7%s^7 unpaused the game (Paused for %d sec) \n\"",
		        ( ent ) ? ent->client->pers.netname : "console",
		        ( int )( ( float ) level.pausedTime / 1000.0f ) ) );
		trap_SendServerCommand( -1, "cp \"The game has been unpaused!\"" );

		level.pausedTime = 0;
	}

	return qtrue;
}

static char *fates[] =
{
	"^2built^7",
	"^3deconstructed^7",
	"^7replaced^7",
	"^3destroyed^7",
	"^1TEAMKILLED^7",
	"^7unpowered^7",
	"removed"
};
qboolean G_admin_buildlog( gentity_t *ent )
{
	char       search[ MAX_NAME_LENGTH ] = { "" };
	char       s[ MAX_NAME_LENGTH ] = { "" };
	char       n[ MAX_NAME_LENGTH ];
	char       stamp[ 8 ];
	int        id = -1;
	int        printed = 0;
	int        time;
	int        start = MAX_CLIENTS + level.buildId - level.numBuildLogs;
	int        i = 0, j;
	buildLog_t *log;

	if ( !level.buildId )
	{
		ADMP( "^3buildlog: ^7log is empty\n" );
		return qtrue;
	}

	if ( trap_Argc() == 3 )
	{
		trap_Argv( 2, search, sizeof( search ) );
		start = atoi( search );
	}

	if ( trap_Argc() > 1 )
	{
		trap_Argv( 1, search, sizeof( search ) );

		for ( i = search[ 0 ] == '-'; isdigit( search[ i ] ); i++ ) {; }

		if ( i && !search[ i ] )
		{
			id = atoi( search );

			if ( trap_Argc() == 2 && ( id < 0 || id >= MAX_CLIENTS ) )
			{
				start = id;
				id = -1;
			}
			else if ( id < 0 || id >= MAX_CLIENTS ||
			          level.clients[ id ].pers.connected != CON_CONNECTED )
			{
				ADMP( "^3buildlog: ^7invalid client id\n" );
				return qfalse;
			}
		}
		else
		{
			G_SanitiseString( search, s, sizeof( s ) );
		}
	}
	else
	{
		start = MAX( -MAX_ADMIN_LISTITEMS, -level.buildId );
	}

	if ( start < 0 )
	{
		start = MAX( level.buildId - level.numBuildLogs, start + level.buildId );
	}
	else
	{
		start -= MAX_CLIENTS;
	}

	if ( start < level.buildId - level.numBuildLogs || start >= level.buildId )
	{
		ADMP( "^3buildlog: ^7invalid build ID\n" );
		return qfalse;
	}

	if ( ent && ent->client->pers.teamSelection != TEAM_NONE )
	{
		trap_SendServerCommand( -1,
		                        va( "print \"^3buildlog: ^7%s^7 requested a log of recent building activity\n\"",
		                            ent->client->pers.netname ) );
	}

	ADMBP_begin();

	for ( i = start; i < level.buildId && printed < MAX_ADMIN_LISTITEMS; i++ )
	{
		log = &level.buildLog[ i % MAX_BUILDLOG ];

		if ( id >= 0 && id < MAX_CLIENTS )
		{
			if ( log->actor != level.clients[ id ].pers.namelog )
			{
				continue;
			}
		}
		else if ( s[ 0 ] )
		{
			if ( !log->actor )
			{
				continue;
			}

			for ( j = 0; j < MAX_NAMELOG_NAMES && log->actor->name[ j ][ 0 ]; j++ )
			{
				G_SanitiseString( log->actor->name[ j ], n, sizeof( n ) );

				if ( strstr( n, s ) )
				{
					break;
				}
			}

			if ( j >= MAX_NAMELOG_NAMES || !log->actor->name[ j ][ 0 ] )
			{
				continue;
			}
		}

		printed++;
		time = ( log->time - level.startTime ) / 1000;
		Com_sprintf( stamp, sizeof( stamp ), "%3d:%02d", time / 60, time % 60 );
		ADMBP( va( "^2%c^7%-3d %s ^7%s^7 %s%s%s\n",
		           log->actor && log->fate != BF_REPLACE && log->fate != BF_UNPOWER ?
		           '*' : ' ',
		           i + MAX_CLIENTS,
		           log->actor && ( log->fate == BF_REPLACE || log->fate == BF_UNPOWER ) ?
		           "    \\_" : stamp,
		           BG_Buildable( log->modelindex )->humanName,
		           fates[ log->fate ],
		           log->actor ? " by " : "",
		           log->actor ?
		           log->actor->name[ log->actor->nameOffset ] :
		           "" ) );
	}

	ADMBP( va( "^3buildlog: ^7showing %d build logs %d - %d of %d - %d.  %s\n",
	           printed, start + MAX_CLIENTS, i + MAX_CLIENTS - 1,
	           level.buildId + MAX_CLIENTS - level.numBuildLogs,
	           level.buildId + MAX_CLIENTS - 1,
	           i < level.buildId ? va( "run 'buildlog %s%s%d' to see more",
	                                   search, search[ 0 ] ? " " : "", i + MAX_CLIENTS ) : "" ) );
	ADMBP_end();
	return qtrue;
}

qboolean G_admin_revert( gentity_t *ent )
{
	char       arg[ MAX_TOKEN_CHARS ];
	char       time[ MAX_DURATION_LENGTH ];
	int        id;
	buildLog_t *log;

	if ( trap_Argc() != 2 )
	{
		ADMP( "^3revert: ^7usage: revert [id]\n" );
		return qfalse;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	id = atoi( arg ) - MAX_CLIENTS;

	if ( id < level.buildId - level.numBuildLogs || id >= level.buildId )
	{
		ADMP( "^3revert: ^7invalid id\n" );
		return qfalse;
	}

	log = &level.buildLog[ id % MAX_BUILDLOG ];

	if ( !log->actor || log->fate == BF_REPLACE || log->fate == BF_UNPOWER )
	{
		// fixme: then why list them with an id # in build log ? - rez
		ADMP( "^3revert: ^7you can only revert direct player actions, "
		      "indicated by ^2* ^7in buildlog\n" );
		return qfalse;
	}

	G_admin_duration( ( level.time - log->time ) / 1000, time,
	                  sizeof( time ) );
	admin_log( arg );
	AP( va( "print \"^3revert: ^7%s^7 reverted %d %s over the past %s\n\"",
	        ent ? ent->client->pers.netname : "console",
	        level.buildId - id,
	        ( level.buildId - id ) > 1 ? "changes" : "change",
	        time ) );
	G_BuildLogRevert( id );
	return qtrue;
}

qboolean G_admin_l0( gentity_t *ent )
{
	char            name[ MAX_NAME_LENGTH ];
	gentity_t       *vic = NULL;
	g_admin_admin_t *a = NULL;
	int             id;

	if ( trap_Argc() < 2 )
	{
		ADMP( "^3l0: ^7usage: l0 [name|slot#|admin#]\n" );
		return qfalse;
	}

	trap_Argv( 1, name, sizeof( name ) );
	id = admin_find_admin( ent, name, "l0", &vic, &a );

	if ( id < 0 )
	{
		return qfalse;
	}

	if ( !a || a->level != 1 )
	{
		ADMP( "^3l0: ^7your intended victim is not level 1\n" );
		return qfalse;
	}

	trap_SendConsoleCommand( EXEC_APPEND, va( "setlevel %d 0;", id ) );

	AP( va( "print \"^3l0: ^7name protection for %s^7 removed by %s\n\"",
	        ( vic ) ? vic->client->pers.netname : a->name,
	        ( ent ) ? ent->client->pers.netname : "console" ) );
	return qtrue;
}

qboolean G_admin_l1( gentity_t *ent )
{
	char            name[ MAX_NAME_LENGTH ];
	gentity_t       *vic = NULL;
	g_admin_admin_t *a = NULL;
	int             id;

	if ( trap_Argc() < 2 )
	{
		ADMP( "^3l1: ^7usage: l1 [name|slot#|admin#]\n" );
		return qfalse;
	}

	trap_Argv( 1, name, sizeof( name ) );
	id = admin_find_admin( ent, name, "l1", &vic, &a );

	if ( id < 0 )
	{
		return qfalse;
	}

	if ( !a || a->level != 0 )
	{
		ADMP( "^3l1: ^7your intended victim is not level 0\n" );
		return qfalse;
	}

	trap_SendConsoleCommand( EXEC_APPEND, va( "setlevel %d 1;", id ) );

	AP( va( "print \"^3l1: ^7name protection for %s^7 enabled by %s\n\"",
	        ( vic ) ? vic->client->pers.netname : a->name,
	        ( ent ) ? ent->client->pers.netname : "console" ) );
	return qtrue;
}

qboolean G_admin_register( gentity_t *ent )
{
	int level = 1;

	if ( !ent )
	{
		return qfalse;
	}

	if ( ent->client->pers.admin && ent->client->pers.admin->level != 0 )
	{
		level = ent->client->pers.admin->level;
	}

	trap_SendConsoleCommand( EXEC_APPEND, va( "setlevel %d %d;",
	                         ( int )( ent - g_entities ),
	                         level ) );

	AP( va( "print \"^3register: ^7%s^7 is now a protected name\n\"",
	        ent->client->pers.netname ) );

	return qtrue;
}

qboolean G_admin_unregister( gentity_t *ent )
{
	if ( !ent )
	{
		return qfalse;
	}

	if ( !ent->client->pers.admin || ent->client->pers.admin->level == 0 )
	{
		ADMP( "^3unregister: ^7you do not have a protected name\n" );
		return qfalse;
	}

	trap_SendConsoleCommand( EXEC_APPEND, va( "setlevel %d 0;",
	                         ( int )( ent - g_entities ) ) );

	AP( va( "print \"^3unregister: ^7%s^7 is now an unprotected name\n\"",
	        ent->client->pers.admin->name ) );

	return qtrue;
}

/*
================
 G_admin_print

 This function facilitates the ADMP define.  ADMP() is similar to CP except
 that it prints the message to the server console if ent is not defined.
================
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

		if ( !trap_Cvar_VariableIntegerValue( "com_ansiColor" ) )
		{
			G_DecolorString( m, m2, sizeof( m2 ) );
			trap_Print( m2 );
		}
		else
		{
			trap_Print( m );
		}
	}
}

void G_admin_buffer_begin( void )
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

void G_admin_cleanup( void )
{
	g_admin_level_t   *l;
	g_admin_admin_t   *a;
	g_admin_ban_t     *b;
	g_admin_command_t *c;
	void              *n;

	for ( l = g_admin_levels; l; l = n )
	{
		n = l->next;
		BG_Free( l );
	}

	g_admin_levels = NULL;

	for ( a = g_admin_admins; a; a = n )
	{
		n = a->next;
		BG_Free( a );
	}

	g_admin_admins = NULL;

	for ( b = g_admin_bans; b; b = n )
	{
		n = b->next;
		BG_Free( b );
	}

	g_admin_bans = NULL;

	for ( c = g_admin_commands; c; c = n )
	{
		n = c->next;
		BG_Free( c );
	}

	g_admin_commands = NULL;
	BG_DefragmentMemory();
}
qboolean G_admin_bot( gentity_t *ent ) {
	int min_args = 3;
	char arg1[MAX_TOKEN_CHARS];
	char name[MAX_TOKEN_CHARS];
	char team[MAX_TOKEN_CHARS];
	char skill[MAX_TOKEN_CHARS];
	char err[MAX_STRING_CHARS];
	int skill_int;
	int i;
	if(trap_Argc() < min_args) {
		ADMP( "^3bot: ^7usage: bot [^5add|del^7] [^5name^7] (5aliens/humans^7) (^5skill^7)\n" );
		return qfalse;
	}
	trap_Argv(1, arg1, sizeof(arg1));
	trap_Argv(2, name, sizeof(name));

	if(!Q_stricmp(arg1, "add")) {
		min_args++; //now we also need a team name
		if(!Q_stricmp(name, "all")) {
			ADMP( "bots can't have that name\n");
			return qfalse;
		}
		if(trap_Argc() < min_args) {
			ADMP( "^3bot: ^7usage: bot [^5add|del^7] [^5name^7] (5aliens/humans^7) (^5skill^7)\n" );
			return qfalse;
		}
		trap_Argv(3, team, sizeof(team));

		//skill level checks
		min_args++;
		if(trap_Argc() < min_args) {
			skill_int = 5; //no skill arg
		} else {
			trap_Argv(4, skill, sizeof(skill));
			skill_int = atoi(skill);
			if(skill_int < 1) {
				skill_int = 1; //skill arg too small, reset to 1
			} else if(skill_int > 10) {
				skill_int = 10; //skill arg too bit, reset to 10
			}
		}
	//choose team
		if(!Q_stricmp(team, "humans") || !Q_stricmp(team, "h")) {

			if(!G_BotAdd(name,TEAM_HUMANS,skill_int)) {
				ADMP("Can't add a bot\n");
			return qfalse;
		}

		} else if(!Q_stricmp(team, "aliens") || !Q_stricmp(team, "a")) {
			if(!G_BotAdd(name,TEAM_ALIENS,skill_int)) {
				ADMP("Can't add a bot\n");
				return qfalse;
			}
		} else {
			ADMP( "Invalid team name\n");
			ADMP( "^3bot: ^7usage: bot [^5add|del^7] [^5name^7] (5aliens/humans^7) (^5skill^7)\n" );
			return qfalse;
		}
	} else if(!Q_stricmp(arg1, "del")) {
		int clientNum = G_ClientNumberFromString(name,err, sizeof(err));
		if(!Q_stricmp(name, "all")) {
			for(i=0;i<MAX_CLIENTS;i++) {
				if(g_entities[i].r.svFlags & SVF_BOT)
					G_BotDel(i);
			}
			return qtrue;
		}

		if(clientNum == -1) { //something went wrong when finding the client Number
			ADMP(err);
			return qfalse;
		}
		G_BotDel(clientNum); //delete the bot
	} else {
		ADMP("Invalid command\n");
		ADMP( "^3bot: ^7usage: bot [^5add|del^7] [^5name^7] (5aliens/humans^7) (^5skill^7)\n" );
		return qfalse;
	}
	return qtrue;
}
