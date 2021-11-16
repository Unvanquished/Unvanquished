/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

This shrubbot implementation is the original work of Tony J. White.

Contains contributions from Wesley van Beelen, Chris Bajumpaa, Josh Menke,
and Travis Maurer.

The functionality of this code mimics the behaviour of the currently
inactive project shrubet (http://www.etstats.com/shrubet/index.php?ver=2)
by Ryan Mannion.   However, shrubet was a closed-source project and
none of its code has been copied, only its functionality.

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

#include "sg_local.h"
#include "engine/qcommon/q_unicode.h"

struct g_admin_cmd_t
{
	const char *keyword;
	bool  ( *handler )( gentity_t *ent );
	bool   silent;
	const char *flag;
	const char *function; // used for /help
	const char *syntax; // used for /help
};

struct g_admin_level_t
{
	g_admin_level_t      *next;

	int                  level;
	char                 name[ MAX_NAME_LENGTH ];
	char                 flags[ MAX_ADMIN_FLAGS ];
};

struct g_admin_ban_t
{
	g_admin_ban_t      *next;
	int                id;

	char               name[ MAX_NAME_LENGTH ];
	char               guid[ 33 ];
	addr_t             ip;
	char               reason[ MAX_ADMIN_BAN_REASON ];
	char               made[ 20 ]; // "YYYY-MM-DD hh:mm:ss"
	int                expires;
	char               banner[ MAX_NAME_LENGTH ];
	int                warnCount;
};

struct g_admin_command_t
{
	g_admin_command_t      *next;

	char                   command[ MAX_ADMIN_CMD_LEN ];
	char                   exec[ MAX_QPATH ];
	char                   desc[ 50 ];
	char                   flag[ MAX_ADMIN_FLAG_LEN ];
};

struct g_admin_spec_t
{
	g_admin_spec_t     *next;
	char               guid[ 33 ];
	// -1: can not join until next game
	// otherwise: expires - t : can not join until delta
	int                expires;
};

struct g_admin_admin_t
{
	g_admin_admin_t      *next;

	int                  level;
	char                 guid[ 33 ];
	char                 name[ MAX_NAME_LENGTH ];
	char                 flags[ MAX_ADMIN_FLAGS ];
	char                 pubkey[ RSA_STRING_LENGTH ];
	char                 msg[ RSA_STRING_LENGTH ];
	char                 msg2[ RSA_STRING_LENGTH ];
	qtime_t              lastSeen;
	int                  counter;
};

int G_admin_joindelay( g_admin_spec_t const* ptr )
{
	return ptr->expires;
}

static void G_admin_notIntermission( gentity_t *ent )
{
	char command[ MAX_ADMIN_CMD_LEN ];
	*command = 0;
	trap_Argv( 0, command, sizeof( command ) );
	ADMP( va( "%s %s", QQ( N_("^3$1$:^* this command is not valid during intermission") ), command ) );
}

#define RETURN_IF_INTERMISSION \
	do { \
		if ( level.intermissiontime ) \
		{ \
			G_admin_notIntermission( ent ); \
			return false; \
		} \
	} while ( 0 )

// big ugly global buffer for use with buffered printing of long outputs
static std::string       g_bfb;
#define MAX_MESSAGE_SIZE static_cast<size_t>(1022)

static bool G_admin_maprestarted( gentity_t * );

static Cvar::Cvar<std::string> g_mapRestarted("g_mapRestarted", "informs whether the map was restarted (y), whether teams are kept together (k) and whether sides are swapped (s)", Cvar::NONE, "0");

// note: list ordered alphabetically
static const g_admin_cmd_t     g_admin_cmds[] =
{
	// first few are command flags...
	// each one listed affects a command's output or limits its functionality
	// none of these prevent all use of the command if denied
	// (in this list, *function is re-used for the command name)
	{ nullptr, 0, false, "buildlog",       "builder",       nullptr },
	{ nullptr, 0, false, "buildlog_admin", "buildlog",      nullptr },
	{ nullptr, 0, false, "gametimelimit",  "time",          nullptr },
	{ nullptr, 0, false, "setlevel",       "listplayers",   nullptr },

	// now the actual commands
	{
		"adjustban",    G_admin_adjustban,   false, "ban",
		N_("change the IP address mask, duration or reason of a ban.  mask is "
		"prefixed with '/'.  duration is specified as "
		"numbers followed by units 'w' (weeks), 'd' (days), 'h' (hours) or "
		"'m' (minutes), or seconds if no units are specified.  if the duration is"
		" preceded by a + or -, the ban duration will be extended or shortened by"
		" the specified amount"),
		N_("[^3ban#^7] (^5/mask^7) (^5duration^7) (^5reason^7)")
	},

	{
		"adminhelp",    G_admin_adminhelp,   true,  "adminhelp",
		N_("display admin commands available to you or help on a specific command"),
		N_("(^5command^7)")
	},

	{
		"admintest",    G_admin_admintest,   false, "admintest",
		N_("display your current admin level"),
		""
	},

	{
		"allowbuild",   G_admin_denybuild,   false, "denybuild",
		N_("restore a player's ability to build"),
		N_("[^3name|slot#^7]")
	},

	{
		"allready",     G_admin_allready,    false, "allready",
		N_("makes everyone ready in intermission"),
		""
	},

	{
		"ban",          G_admin_ban,         false, "ban",
		N_("ban a player by IP address and GUID with an optional expiration time and reason."
		" duration is specified as numbers followed by units 'w' (weeks), 'd' "
		"(days), 'h' (hours) or 'm' (minutes), or seconds if no units are "
		"specified"),
		N_("[^3name|slot#|IP(/mask)^7] (^5duration^7) (^5reason^7)")
	},
	{
		"bot",          G_admin_bot,         false, "bot",
		N_("Add/Del/Spec bots"),
		N_("[^5add|del|spec|unspec^7] [^5name|all^7] [^5aliens/humans^7] (^5skill^7)")
	},
	{
		"builder",      G_admin_builder,     true,  "builder",
		N_("show who built a structure"),
		""
	},

	{
		"buildlog",     G_admin_buildlog,    false, "buildlog",
		N_("show buildable log"),
		N_("(^5name|slot#^7) (^5id^7)")
	},

	{
		"cancelvote",   G_admin_endvote,     false, "cancelvote",
		N_("cancel a vote taking place"),
		"(^5a|h^7)"
	},

	{
		"changemap",    G_admin_changemap,   false, "changemap",
		N_("load a map (and optionally force layout)"),
		N_("[^3mapname^7] (^5layout^7)")
	},

	{
		"denybuild",    G_admin_denybuild,   false, "denybuild",
		N_("take away a player's ability to build"),
		N_("[^3name|slot#^7]")
	},

	{
		"flag",          G_admin_flag,       false, "flag",
		N_("add an admin flag to a player, prefix flag with '-' to disallow the flag. "
		   "console can use this command on admin levels by prefacing a '*' to the admin level value."),
		N_("[^3name|slot#|admin#|*level#^7] (^5+^7|^5-^7)[^3flag^7]")
	},

	{
		"flaglist",      G_admin_flaglist,   false, "flag",
		N_("list the flags understood by this server"),
		""
	},

	{
		"kick",         G_admin_kick,        false, "kick",
		N_("kick a player with an optional reason"),
		N_("[^3name|slot#^7] (^5reason^7)")
	},

	{
		"l0",           G_admin_l0,          false, "l0",
		N_("remove name protection from a player by setting them to admin level 0"),
		N_("[^3name|slot#|admin#^7]")
	},

	{
		"l1",           G_admin_l1,          false, "l1",
		N_("give a player name protection by setting them to admin level 1"),
		N_("[^3name|slot#^7]")
	},

	{
		"listadmins",   G_admin_listadmins,  true,  "listadmins",
		N_("display a list of all server admins and their levels"),
		N_("(^5name^7) (^5start admin#^7)")
	},

	{
		"listgeoip",  G_admin_listgeoip, true,  "listgeoip",
		N_("display a list of players and which countries they're from (using GeoIP data)"),
		N_("(^5name^7)")
	},

	{
		"listinactive", G_admin_listinactive, true, "listadmins",
		N_("display a list of inactive server admins and their levels"),
		N_("[^5months^7] (^5start admin#^7)")
	},

	{
		"listlayouts",  G_admin_listlayouts, true,  "listlayouts",
		N_("display a list of all available layouts for a map"),
		N_("(^5mapname^7)")
	},

	{
		"listplayers",  G_admin_listplayers, true,  "listplayers",
		N_("display a list of players, their client numbers and their levels"),
		""
	},

	{
		"lock",         G_admin_lock,        false, "lock",
		N_("lock a team to prevent anyone from joining it"),
		"[^3a|h^7]"
	},

	{
		"maprestarted", G_admin_maprestarted, false, "",
		nullptr,
		nullptr
	},

	{
		"mute",         G_admin_mute,        false, "mute",
		N_("mute a player"),
		"[^3name|slot#^7]"
	},

	{
		"namelog",      G_admin_namelog,     true,  "namelog",
		N_("display a list of names used by recently connected players"),
		N_("(^5name|IP(/mask)^7) (start namelog#)")
	},

	{
		"nextmap",      G_admin_nextmap,     false, "nextmap",
		N_("go to the next map in the cycle"),
		""
	},

	{
		"passvote",     G_admin_endvote,     false, "passvote",
		N_("pass a vote currently taking place"),
		"(^5a|h^7)"
	},

	{
		"pause",        G_admin_pause,       false, "pause",
		N_("Pause (or unpause) the game."),
		""
	},

	{
		"putteam",      G_admin_putteam,     false, "putteam",
		N_("move a player to a specified team"),
		N_("[^3name|slot#^7] [^3h|a|s^7]")
	},

	{
		"readconfig",   G_admin_readconfig,  false, "readconfig",
		N_("reloads the admin config file and refreshes permission flags"),
		""
	},

	{
		"register",     G_admin_register,    false, "register",
		N_("register your name to protect it from being used by other players."),
		""
	},

	{
		"rename",       G_admin_rename,      false, "rename",
		N_("rename a player"),
		N_("[^3name|slot#^7] [^3new name^7]")
	},

	{
		"restart",      G_admin_restart,     false, "restart",
		N_("restart the current map (optionally using named layout or keeping/switching teams)"),
		N_("(^5layout^7) (^5keepteams|switchteams|keepteamslock|switchteamslock^7)")
	},

	{
		"revert",       G_admin_revert,      false, "revert",
		N_("revert buildables to a given time"),
		N_("[^3id^7]")
	},

	{
		"setlevel",     G_admin_setlevel,    false, "setlevel",
		N_("sets the admin level of a player"),
		N_("[^3name|slot#|admin#^7] [^3level^7]")
	},

	{
		"showbans",     G_admin_showbans,    true,  "showbans",
		N_("display a (partial) list of active bans"),
		N_("(^5name|IP(/mask)^7) (^5start at ban#^7)")
	},

	{
		"spec999",      G_admin_spec999,     false, "spec999",
		N_("move 999 pingers to the spectator team"),
		""
	},

	{
		"speclock",      G_admin_speclock,   false, "kick",
		N_("move a player to spectators and prevent from joining a team"
		" duration is specified as numbers followed by units 'h' (hours) or 'm' (minutes), "
		"or seconds if no units are specified. End-of-game automatically revokes this"),
		N_("[^3name|slot#^7] [^3duration^7]")
	},

	{
		"specunlock",    G_admin_specunlock, false, "ban",
		N_("allow a player to join any team again"),
		N_("[^3name|slot#^7]")
	},

	{
		"time",         G_admin_time,        true,  "time", // setting requires "gametimelimit"
		N_("show the current local server time or set this game's time limit"),
		N_("[^3minutes^7]")
	},

	{
		"unban",        G_admin_unban,       false, "ban",
		N_("unbans a player specified by the slot as seen in showbans"),
		N_("[^3ban#^7]")
	},

	{
		"unflag",       G_admin_flag,        false, "flag",
		N_("clears an admin flag from a player. "
		   "console can use this command on admin levels by prefacing a '*' to the admin level value."),
		N_("[^3name|slot#|admin#|*level#^7] (^5+^7|^5-^7)[^3flag^7]")
	},

	{
		"unlock",       G_admin_lock,        false, "lock",
		N_("unlock a locked team"),
		N_("[^3a|h^7]")
	},

	{
		"unmute",       G_admin_mute,        false, "mute",
		N_("unmute a muted player"),
		N_("[^3name|slot#^7]")
	},

	{
		"unregister",   G_admin_unregister,  false, "unregister",
		N_("unregister your name so that it can be used by other players."),
		""
	},

	{
		"warn",         G_admin_warn,        false, "warn",
		N_("warn a player about his behaviour"),
		N_("[^3name|slot#^7] [^3reason^7]")
	}
};
#define adminNumCmds ARRAY_LEN( g_admin_cmds )

struct g_admin_flag_t
{
	const char *flag;
	const char *description;
};

static const g_admin_flag_t g_admin_flags[] = {
	{ ADMF_ACTIVITY,        "inactivity rules do not apply" },
	{ ADMF_ADMINCHAT,       "can see and use admin chat" },
	{ ADMF_ALLFLAGS,        "can use any command" },
	{ ADMF_CAN_PERM_BAN,    "can permanently ban players" },
	{ ADMF_FORCETEAMCHANGE, "team balance rules do not apply" },
	{ ADMF_INCOGNITO,       "does not show as admin in !listplayers" },
	{ ADMF_IMMUNITY,        "cannot be vote kicked, vote muted, or banned" },
	{ ADMF_IMMUTABLE,       "admin commands cannot be used on them" },
	{ ADMF_NOCENSORFLOOD,   "no flood protection" },
	{ ADMF_NO_VOTE_LIMIT,   "vote limitations do not apply" },
	{ ADMF_SPEC_ALLCHAT,    "can see team chat as spectator" },
	{ ADMF_UNACCOUNTABLE,   "does not need to specify reason for kick/ban" }
};
#define adminNumFlags ARRAY_LEN( g_admin_flags )

static int        admin_level_maxname = 0;
g_admin_level_t   *g_admin_levels = nullptr;
g_admin_admin_t   *g_admin_admins = nullptr;
g_admin_ban_t     *g_admin_bans = nullptr;
g_admin_spec_t    *g_admin_specs = nullptr;
g_admin_command_t *g_admin_commands = nullptr;

/* ent must be non-nullptr */
#define G_ADMIN_NAME( ent ) ( ent->client->pers.admin ? ent->client->pers.admin->name : ent->client->pers.netname )

const char *G_admin_name( gentity_t *ent )
{
	return ( ent ) ? G_ADMIN_NAME( ent ) : "console";
}

const char *G_quoted_admin_name( gentity_t *ent )
{
	return ( ent ) ? Quote( G_ADMIN_NAME( ent ) ) : "console";
}

static const char *G_user_name( gentity_t *ent, const char *fallback )
{
	return ( ent ) ? ent->client->pers.netname : fallback;
}

static const char *G_quoted_user_name( gentity_t *ent, const char *fallback )
{
	return Quote( G_user_name( ent, fallback ) );
}

void G_admin_register_cmds()
{
	for ( unsigned i = 0; i < adminNumCmds; i++ )
	{
		if ( g_admin_cmds[ i ].keyword )
		{
			trap_AddCommand( g_admin_cmds[ i ].keyword );
		}
	}
}

void G_admin_unregister_cmds()
{
	for ( unsigned i = 0; i < adminNumCmds; i++ )
	{
		if ( g_admin_cmds[ i ].keyword )
		{
			trap_RemoveCommand( g_admin_cmds[ i ].keyword );
		}
	}
}

void G_admin_cmdlist( gentity_t *ent )
{
	char out[ MAX_STRING_CHARS ] = "";
	unsigned len, outlen;

	outlen = 0;

	for ( unsigned i = 0; i < adminNumCmds; i++ )
	{
		if ( !g_admin_cmds[ i ].keyword || !G_admin_permission( ent, g_admin_cmds[ i ].flag ) )
		{
			continue;
		}

		len = strlen( g_admin_cmds[ i ].keyword ) + 1;

		if ( len + outlen >= sizeof( out ) - 1 )
		{
			trap_SendServerCommand( ent - g_entities, va( "cmds%s", out ) );
			outlen = 0;
		}

		strcpy( out + outlen, va( " %s", g_admin_cmds[ i ].keyword ) );
		outlen += len;
	}

	trap_SendServerCommand( ent - g_entities, va( "cmds%s", out ) );
}

// match a certain flag within these flags
static bool admin_permission( char *flags, const char *flag, bool *perm )
{
	const char     *token, *token_p = flags;
	bool allflags = false;
	bool p = false;
	*perm = false;

	while ( * ( token = COM_Parse( &token_p ) ) )
	{
		*perm = true;

		if ( *token == '-' || *token == '+' )
		{
			*perm = *token++ == '+';
		}

		if ( !strcmp( token, flag ) )
		{
			return true;
		}

		if ( !strcmp( token, ADMF_ALLFLAGS ) )
		{
			allflags = true;
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
	const g_admin_cmd_t *cmds = g_admin_cmds;
	int count = adminNumCmds;

	while ( count && !cmds->keyword )
	{
		++cmds;
		--count;
	}

	if ( !count )
	{
		return nullptr;
	}

	return (g_admin_cmd_t*) bsearch( cmd, cmds, count, sizeof( g_admin_cmd_t ), cmdcmp );
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

	return nullptr;
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

	return nullptr;
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

	return nullptr;
}

bool G_admin_permission( gentity_t *ent, const char *flag )
{
	bool        perm;
	g_admin_admin_t *a;
	g_admin_level_t *l;

	// Always return true for dummy commands.
	if ( flag == nullptr )
	{
		return true;
	}

	// console always wins
	if ( !ent )
	{
		return true;
	}

	if ( ent->client->pers.admin && ent->client->pers.pubkey_authenticated != 1 )
	{
		CP( "cp_tr " QQ(N_("^1You are not pubkey authenticated")) "" );
		return false;
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

	return false;
}

bool G_admin_name_check( gentity_t *ent, const char *name, char *err, int len )
{
	int             i;
	gclient_t       *client;
	char            testName[ MAX_NAME_LENGTH ] = { "" };
	char            name2[ MAX_NAME_LENGTH ] = { "" };
	g_admin_admin_t *admin;
	int             alphaCount = 0;

	G_SanitiseString( name, name2, sizeof( name2 ) );

	if ( !Q_stricmp( name2, UNNAMED_PLAYER ) )
	{
		return true;
	}

	if ( !strcmp( name2, "console" ) )
	{
		if ( err && len > 0 )
		{
			Q_strncpyz( err, N_("This name is not allowed:"), len );
		}

		return false;
	}

	Color::StripColors( name, testName, sizeof( testName ) );

	if ( Str::cisdigit( testName[ 0 ] ) )
	{
		if ( err && len > 0 )
		{
			Q_strncpyz( err, N_("Names cannot begin with numbers:"), len );
		}

		return false;
	}

	for ( i = 0; testName[ i ]; )
	{
		int cp = Q_UTF8_CodePoint( testName + i );

		if ( Q_Unicode_IsAlphaOrIdeo( cp ) )
		{
			alphaCount++;
		}

		i += Q_UTF8_WidthCP( cp );
	}

	if ( alphaCount == 0 )
	{
		if ( err && len > 0 )
		{
			Q_strncpyz( err, N_("Names must contain letters:"), len );
		}

		return false;
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
				Q_strncpyz( err, N_("This name is already in use:"), len );
			}

			return false;
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
				Q_strncpyz( err, N_("Please use another name. This name "
								       "belongs to an admin:"), len );
			}

			return false;
		}
	}

	return true;
}

static bool admin_higher_admin( g_admin_admin_t *a, g_admin_admin_t *b )
{
	bool perm;

	if ( !b )
	{
		return true;
	}

	if ( admin_permission( b->flags, ADMF_IMMUTABLE, &perm ) )
	{
		return !perm;
	}

	return b->level <= ( a ? a->level : 0 );
}

static bool admin_higher_guid( char *admin_guid, char *victim_guid )
{
	return admin_higher_admin( G_admin_admin( admin_guid ),
	                           G_admin_admin( victim_guid ) );
}

static bool admin_higher( gentity_t *admin, gentity_t *victim )
{
	// console always wins
	if ( !admin )
	{
		return true;
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

void G_admin_writeconfig()
{
	fileHandle_t      f;
	int               t;
	g_admin_admin_t   *a;
	g_admin_level_t   *l;
	g_admin_ban_t     *b;
	g_admin_command_t *c;

	if ( g_admin.Get().empty() )
	{
		Log::Warn("g_admin is not set. "
		          " configuration will not be saved to a file." );
		return;
	}

	t = Com_GMTime( nullptr );

	if ( trap_FS_FOpenFile( g_admin.Get().c_str(), &f, fsMode_t::FS_WRITE_VIA_TEMPORARY ) < 0 )
	{
		Log::Warn( "admin_writeconfig: could not open g_admin file \"%s\"",
		          g_admin.Get() );
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
		trap_FS_Write( "lastseen = ", 11, f );
		admin_writeconfig_int( a->lastSeen.tm_year * 10000 + a->lastSeen.tm_mon * 100 + a->lastSeen.tm_mday, f );
		trap_FS_Write( "\n", 1, f );
	}

	for ( b = g_admin_bans; b; b = b->next )
	{
		// don't write stale bans
		if ( G_ADMIN_BAN_STALE( b, t ) )
		{
			continue;
		}

		if ( G_ADMIN_BAN_IS_WARNING( b ) )
		{
			trap_FS_Write( "[warning]\n", 10, f );
		}
		else
		{
			trap_FS_Write( "[ban]\n", 6, f );
		}

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

static void admin_readconfig_string( const char **cnf, char *s, unsigned size )
{
	char *t;

	//COM_MatchToken(cnf, "=");
	s[ 0 ] = '\0';
	t = COM_ParseExt( cnf, false );

	if ( strcmp( t, "=" ) )
	{
		COM_ParseWarning( "expected '=' before \"%s\"", t );
		Q_strncpyz( s, t, size );
	}

	while ( 1 )
	{
		t = COM_ParseExt( cnf, false );

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

static void admin_readconfig_int( const char **cnf, int *v )
{
	char *t;

	//COM_MatchToken(cnf, "=");
	t = COM_ParseExt( cnf, false );

	if ( !strcmp( t, "=" ) )
	{
		t = COM_ParseExt( cnf, false );
	}
	else
	{
		COM_ParseWarning( "expected '=' before \"%s\"", t );
	}

	*v = atoi( t );
}

// if we can't parse any levels from readconfig, set up default
// ones to make new installs easier for admins
static void admin_default_levels()
{
	g_admin_level_t *l;
	int             level = 0;

	l = g_admin_levels = (g_admin_level_t*) BG_Alloc( sizeof( g_admin_level_t ) );
	l->level = level++;
	Q_strncpyz( l->name, "^4Unknown Player", sizeof( l->name ) );
	Q_strncpyz( l->flags,
	            "listplayers admintest adminhelp time register",
	            sizeof( l->flags ) );

	l = l->next = (g_admin_level_t*) BG_Alloc( sizeof( g_admin_level_t ) );
	l->level = level++;
	Q_strncpyz( l->name, "^5Server Regular", sizeof( l->name ) );
	Q_strncpyz( l->flags,
	            "listplayers admintest adminhelp time register unregister",
	            sizeof( l->flags ) );

	l = l->next = (g_admin_level_t*) BG_Alloc( sizeof( g_admin_level_t ) );
	l->level = level++;
	Q_strncpyz( l->name, "^6Team Manager", sizeof( l->name ) );
	Q_strncpyz( l->flags,
	            "listplayers admintest adminhelp time putteam spec999 register unregister",
	            sizeof( l->flags ) );

	l = l->next = (g_admin_level_t*) BG_Alloc( sizeof( g_admin_level_t ) );
	l->level = level++;
	Q_strncpyz( l->name, "^2Junior Admin", sizeof( l->name ) );
	Q_strncpyz( l->flags,
	            "listplayers admintest adminhelp time putteam spec999 warn kick mute ADMINCHAT "
	            "buildlog register unregister l0 l1",
	            sizeof( l->flags ) );

	l = l->next = (g_admin_level_t*) BG_Alloc( sizeof( g_admin_level_t ) );
	l->level = level++;
	Q_strncpyz( l->name, "^3Senior Admin", sizeof( l->name ) );
	Q_strncpyz( l->flags,
	            "listplayers admintest adminhelp time putteam spec999 warn kick mute showbans ban "
	            "namelog buildlog ADMINCHAT register unregister l0 l1",
	            sizeof( l->flags ) );

	l = l->next = (g_admin_level_t*) BG_Alloc( sizeof( g_admin_level_t ) );
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

	G_LogPrintf( "AdminAuth: %i \"%s^*\" \"%s^*\" [%d] (%s): %s",
	             ( int )( ent - g_entities ), ent->client->pers.netname,
	             ent->client->pers.admin->name, ent->client->pers.admin->level,
	             ent->client->pers.guid, aflags );
}

static char adminLog[ MAX_STRING_CHARS ];
static int  adminLogLen;
static void admin_log_start( gentity_t *admin, const char *cmd )
{
	const char *name = G_admin_name( admin );

	adminLogLen = Q_snprintf( adminLog, sizeof( adminLog ),
	                          "%d \"%s^*\" \"%s^*\" [%d] (%s): %s",
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

static void admin_log_abort()
{
	adminLog[ 0 ] = '\0';
	adminLogLen = 0;
}

static void admin_log_end( const bool ok )
{
	if ( adminLog[ 0 ] )
	{
		G_LogPrintf( "AdminExec: %s: %s", ok ? "ok" : "fail", adminLog );
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
                         bool( *match )( void *, const void * ),
                         int ( *out )( void *, char * ),
                         const void *list,
                         const void *arg,
                         const char *arglist,
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

	for ( total = 0; l; l = l->next )
	{
		int id = out( l, nullptr );
		// assume that returned ids are in ascending order
		total = id ? id : ( total + 1 );
	}

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
			int id = out( l, nullptr );

			if ( id )
			{
				i = id - offset;
			}

			if ( i >= start && ( limit < 1 || count < limit ) )
			{
				out( l, str );

				ADMBP( va( "^7%-3d %s", i + offset, str ) );

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
		ADMBP( va( "^3%s:^* showing %d of %d %s %d-%d%s%s.",
		           cmd, count, found, noun, start + offset, end + offset,
		           ( arglist && *arglist ) ? " matching " : "", arglist ) );

		if ( next )
		{
			ADMBP( va( "  use '%s%s%s %d' to see more", cmd,
			           arglist ? " " : "", arglist,
			           next + offset ) );
		}
	}

	ADMBP( "" );
	ADMBP_end();
	return next + offset;
}

static bool admin_match( void *admin, const void *match )
{
	char n1[ MAX_NAME_LENGTH ], n2[ MAX_NAME_LENGTH ];
	G_SanitiseString( ( char * ) match, n2, sizeof( n2 ) );

	if ( !n2[ 0 ] )
	{
		return true;
	}

	G_SanitiseString( ( ( g_admin_admin_t * ) admin )->name, n1, sizeof( n1 ) );
	return strstr( n1, n2 ) != nullptr;
}

static int admin_out( void *admin, char *str )
{
	g_admin_admin_t *a = ( g_admin_admin_t * ) admin;
	g_admin_level_t *l;
	char            lastSeen[64] = "          ";

	if ( !str )
	{
		return 0;
	}

	l = G_admin_level( a->level );

	int lncol = Color::StrlenNocolor( l->name );

	if ( a->lastSeen.tm_mday )
	{
		trap_GetTimeString( lastSeen, sizeof( lastSeen ), "%Y-%m-%d", &a->lastSeen );
	}

	Com_sprintf( str, MAX_STRING_CHARS, "%-6d %*s %s %s",
	             a->level, admin_level_maxname + lncol, l ? l->name : "(null)",
	             lastSeen, a->name );

	return 0;
}

static int admin_listadmins( gentity_t *ent, int start, char *search )
{
	return admin_search( ent, "listadmins", "admins", admin_match, admin_out,
	                     g_admin_admins, search, search, start, MAX_CLIENTS, MAX_ADMIN_LISTITEMS );
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
	g_admin_admin_t *a = nullptr;
	gentity_t       *vic = nullptr;
	bool        numeric = true;

	if ( client )
	{
		*client = nullptr;
	}

	if ( admin )
	{
		*admin = nullptr;
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
		if ( !Str::cisdigit( *p ) )
		{
			numeric = false;
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
				ADMP( va( "%s %s %d", QQ( N_("^3$1$:^* no player connected in slot $2$") ), command, id ) );
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

			ADMP( va( "%s %s %s %d %d %d", QQ( N_("^3$1$:^* $2$ not in range 0–$3$ or $4$–$5$") ),
			          Quote(command), Quote(name),
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
			ADMP( va( "%s %s", QQ( N_("^3$1$: no match, use listplayers or listadmins to "
			          "find an appropriate number to use instead of name.") ),
			          command ) );
			return -1;
		}

		if ( matches > 1 )
		{
			ADMP( va( "%s %s", QQ( N_("^3$1$: more than one match, use the admin number instead:") ),
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
void G_admin_duration( int secs, char *time, int timesize, char *duration, int dursize )
{
	// sizeof("12.5 minutes") == 13
	if ( secs > ( 60 * 60 * 24 * 365 * 50 ) || secs < 0 )
	{
		Q_strncpyz( duration, "PERMANENT", dursize );
		*time = '\0';
	}
	else if ( secs >= ( 60 * 60 * 24 * 365 ) )
	{
		Q_strncpyz( duration, "years", dursize );
		Com_sprintf( time, timesize, "%1.1f ", secs / ( 60 * 60 * 24 * 365.0f ) );
	}
	else if ( secs >= ( 60 * 60 * 24 * 90 ) )
	{
		Q_strncpyz( duration, "weeks", dursize );
		Com_sprintf( time, timesize, "%1.1f ", secs / ( 60 * 60 * 24 * 7.0f ) );
	}
	else if ( secs >= ( 60 * 60 * 24 ) )
	{
		Q_strncpyz( duration, "days", dursize );
		Com_sprintf( time, timesize, "%1.1f ", secs / ( 60 * 60 * 24.0f ) );
	}
	else if ( secs >= ( 60 * 60 ) )
	{
		Q_strncpyz( duration, "hours", dursize );
		Com_sprintf( time, timesize, "%1.1f ", secs / ( 60 * 60.0f ) );
	}
	else if ( secs >= 60 )
	{
		Q_strncpyz( duration, "minutes", dursize );
		Com_sprintf( time, timesize, "%1.1f ", secs / 60.0f );
	}
	else
	{
		Q_strncpyz( duration, "seconds ", dursize );
		Com_sprintf( time, timesize, "%i ", secs );
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
		char  duration[ MAX_DURATION_LENGTH ];
		char  time[ MAX_DURATION_LENGTH ];
		G_admin_duration( ban->expires - Com_GMTime( nullptr ), time, sizeof( time ), duration,
		                  sizeof( duration ) );
		// part of this might get cut off on the connect screen
		Com_sprintf( creason, clen,
					"You have been banned by %s^* duration: %s%s reason: %s",
					ban->banner,
					time, duration,
					ban->reason );
	}

	if ( areason && ent )
	{
		Com_sprintf( areason, alen,
		             "^3Banned player %s^3 tried to connect from %s (ban #%d)",
		             G_user_name( ent, ban->name ),
		             ent->client->pers.ip.str,
		             ban->id );
	}
}

static bool G_admin_ban_matches( g_admin_ban_t *ban, gentity_t *ent )
{
	return !Q_stricmp( ban->guid, ent->client->pers.guid ) ||
	       ( !G_admin_permission( ent, ADMF_IMMUNITY ) &&
	         G_AddressCompare( &ban->ip, &ent->client->pers.ip ) );
}

static g_admin_ban_t *G_admin_match_ban( gentity_t *ent, const g_admin_ban_t *start )
{
	int           t;
	g_admin_ban_t *ban;

	t = Com_GMTime( nullptr );

	if ( ent->client->pers.localClient )
	{
		return nullptr;
	}

	for ( ban = start ? start->next : g_admin_bans; ban; ban = ban->next )
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

	return nullptr;
}

bool G_admin_ban_check( gentity_t *ent, char *reason, int rlen )
{
	g_admin_ban_t *ban = nullptr;
	char          warningMessage[ MAX_STRING_CHARS ];

	if ( ent->client->pers.localClient )
	{
		return false;
	}

	while ( ( ban = G_admin_match_ban( ent, ban ) ) )
	{
		// warn count -ve ⇒ is a warning, so don't deny connection
		if ( G_ADMIN_BAN_IS_WARNING( ban ) )
		{
			ent->client->pers.hasWarnings = true;
			continue;
		}

		// otherwise, it's a ban
		G_admin_ban_message( ent, ban, reason, rlen,
		                     warningMessage, sizeof( warningMessage ) );

		// don't spam admins
		if ( ban->warnCount++ < 5 )
		{
			G_AdminMessage( nullptr, warningMessage );
		}
		// and don't fill the console
		else if ( ban->warnCount < 10 )
		{
			Log::Warn( "%s%s", warningMessage,
			                ban->warnCount + 1 == 10 ?
			                "^7 — future messages for this ban will be suppressed" :
			                "" );
		}

		return true;
	}

	return false;
}

g_admin_spec_t *G_admin_match_spec( gentity_t *ent )
{
	g_admin_spec_t *spec;

	if ( ent->client->pers.localClient )
	{
		return nullptr;
	}

	for ( spec = g_admin_specs; spec; spec = spec->next )
	{
		if ( !Q_stricmp( spec->guid, ent->client->pers.guid ) )
		{
			return spec;
		}
	}

	return nullptr;
}

bool G_admin_cmd_check( gentity_t *ent )
{
	char              command[ MAX_ADMIN_CMD_LEN ];
	g_admin_cmd_t     *admincmd;
	g_admin_command_t *c;
	bool          success;

	command[ 0 ] = '\0';
	trap_Argv( 0, command, sizeof( command ) );

	if ( !command[ 0 ] )
	{
		return false;
	}

	Q_strlwr( command );
	admin_log_start( ent, command );

	if ( ( c = G_admin_command( command ) ) )
	{
		int j;
		trap_Cvar_Set( "arg_all", ConcatArgs( 1 ) );
		trap_Cvar_Set( "arg_count", va( "%i", trap_Argc() - ( 1 ) ) );
		trap_Cvar_Set( "arg_client", G_admin_name( ent ) );

		for ( j = trap_Argc() - ( 1 ); j; j-- )
		{
			char this_arg[ MAX_CVAR_VALUE_STRING ];
			trap_Argv( j, this_arg, sizeof( this_arg ) );
			trap_Cvar_Set( va( "arg_%i", j ), this_arg );
		}

		admin_log( ConcatArgsPrintable( 1 ) );

		if ( ( success = G_admin_permission( ent, c->flag ) ) )
		{
			if ( G_FloodLimited( ent ) )
			{
				return true;
			}

			trap_SendConsoleCommand( c->exec );
		}
		else
		{
			ADMP( va( "%s %s", QQ( N_("^3$1$:^* permission denied") ), c->command ) );
		}

		admin_log_end( success );
		return true;
	}

	if ( ( admincmd = G_admin_cmd( command ) ) )
	{
		if ( ( success = G_admin_permission( ent, admincmd->flag ) ) )
		{
			if ( G_FloodLimited( ent ) )
			{
				return true;
			}

			if ( admincmd->silent )
			{
				admin_log_abort();
			}

			if ( !admincmd->handler )
			{
				return true;
			}

			if ( !( success = admincmd->handler( ent ) ) )
			{
				admin_log( ConcatArgsPrintable( 1 ) );
			}
		}
		else
		{
			ADMP( va( "%s %s", QQ( N_("^3$1$:^* permission denied") ),admincmd->keyword ) );
			admin_log( ConcatArgsPrintable( 1 ) );
		}

		admin_log_end( success );
		return true;
	}

	return false;
}

static void llsort( struct llist **head, int compar( const void *, const void * ) )
{
	struct llist *a, *b, *t, *l;

	int          i, c = 1, ns, as, bs;

	if ( !*head )
	{
		return;
	}

	b = t = nullptr;

	do
	{
		a = *head; l = *head = nullptr;

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
					t = a; a = a->next; as--;
				}
				else
				{
					t = b; b = b->next; bs--;
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

		l->next = nullptr;
		c *= 2;
	}
	while ( ns > 1 );
}

static int cmplevel( const void *a, const void *b )
{
	return ( ( g_admin_level_t * ) b )->level - ( ( g_admin_level_t * ) a )->level;
}

void G_admin_pubkey()
{
	g_admin_admin_t *highest = nullptr, *a = nullptr;

	// Uncomment this if your server lags (shouldn't happen unless you are on a *very* old computer)
	// Will only regenerate messages when there are no active client (When they are all loading the map)
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
		else if (!a->pubkey[ 0 ] )
		{
			continue;
		}
		else if ( a->counter == 0 )
		{
			if ( !a->msg2[ 0 ] )
			{
				a->counter = -1;
			}
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

bool G_admin_readconfig( gentity_t *ent )
{
	g_admin_level_t   *l = nullptr;
	g_admin_admin_t   *a = nullptr;
	g_admin_ban_t     *b = nullptr;
	g_admin_command_t *c = nullptr;
	int               lc = 0, ac = 0, bc = 0, cc = 0;
	fileHandle_t      f;
	int               len;
	char              *cnf1, *cnf2;
	char              *t;
	bool              level_open, admin_open, ban_open, command_open;
	int               i;
	char              ip[ 44 ];

	G_admin_cleanup();

	if ( g_admin.Get().empty() )
	{
		ADMP( QQ( N_("^3readconfig: g_admin is not set, not loading configuration "
		      "from a file" ) ) );
		return false;
	}

	len = trap_FS_FOpenFile( g_admin.Get().c_str(), &f, fsMode_t::FS_READ );

	if ( len < 0 )
	{
		Log::Warn( "^3readconfig:^* could not open admin config file %s",
		          g_admin.Get() );
		admin_default_levels();
		return false;
	}

	cnf1 = (char*) BG_Alloc( len + 1 );
	cnf2 = cnf1;
	trap_FS_Read(cnf1, len, f );
	* ( cnf1 + len ) = '\0';
	const char *cnf = cnf1;
	trap_FS_FCloseFile( f );

	admin_level_maxname = 0;

	level_open = admin_open = ban_open = command_open = false;
	COM_BeginParseSession( g_admin.Get().c_str() );

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
				l = l->next = (g_admin_level_t*) BG_Alloc( sizeof( g_admin_level_t ) );
			}
			else
			{
				l = g_admin_levels = (g_admin_level_t*) BG_Alloc( sizeof( g_admin_level_t ) );
			}

			memset( l, 0, sizeof( *l ) );
			level_open = true;
			admin_open = ban_open = command_open = false;
			lc++;
		}
		else if ( !Q_stricmp( t, "[admin]" ) )
		{
			if ( a )
			{
				a = a->next = (g_admin_admin_t*) BG_Alloc( sizeof( g_admin_admin_t ) );
			}
			else
			{
				a = g_admin_admins = (g_admin_admin_t*) BG_Alloc( sizeof( g_admin_admin_t ) );
			}

			memset( a, 0, sizeof( *a ) );
			admin_open = true;
			level_open = ban_open = command_open = false;
			ac++;
		}
		else if ( !Q_stricmp( t, "[ban]" ) || !Q_stricmp( t, "[warning]" ) )
		{
			if ( b )
			{
				int id = b->id + 1;
				b = b->next = (g_admin_ban_t*) BG_Alloc( sizeof( g_admin_ban_t ) );
				b->id = id;
			}
			else
			{
				b = g_admin_bans = (g_admin_ban_t*) BG_Alloc( sizeof( g_admin_ban_t ) );
				b->id = 1;
			}

			b->warnCount = ( t[ 1 ] == 'w' ) ? -1 : 0;

			ban_open = true;
			level_open = admin_open = command_open = false;
			bc++;
		}
		else if ( !Q_stricmp( t, "[command]" ) )
		{
			if ( c )
			{
				c = c->next = (g_admin_command_t*) BG_Alloc( sizeof( g_admin_command_t ) );
			}
			else
			{
				c = g_admin_commands = (g_admin_command_t*) BG_Alloc( sizeof( g_admin_command_t ) );
			}

			command_open = true;
			level_open = admin_open = ban_open = false;
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
				len = Color::StrlenNocolor( l->name );

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
			else if ( !Q_stricmp( t, "lastseen" ) )
			{
				unsigned int tm;
				admin_readconfig_int( &cnf, (int *) &tm );
				// trust the admin here...
				a->lastSeen.tm_year = tm / 10000;
				a->lastSeen.tm_mon = ( tm / 100 ) % 100;
				a->lastSeen.tm_mday = tm % 100;
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
	ADMP( va( "%s %d %d %d %d", QQ( N_("^3readconfig:^* loaded $1$ levels, $2$ admins, $3$ bans, $4$ commands") ),
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
			                                G_admin_admin( level.clients[ i ].pers.guid ) : nullptr;

			if ( level.clients[ i ].pers.admin )
			{
				G_admin_authlog( &g_entities[ i ] );
			}

			G_admin_cmdlist( &g_entities[ i ] );
		}
	}

	// register user-defined commands
	for ( c = g_admin_commands; c; c = c->next )
	{
		trap_AddCommand(c->command);
	}

	return true;
}

bool G_admin_time( gentity_t *ent )
{
	qtime_t qt;
	int timelimit, gameMinutes, gameSeconds, remainingMinutes, remainingSeconds;
	char tstr[ 12 ];

	switch ( trap_Argc() )
	{
	case 1:
		Com_GMTime( &qt );

		gameMinutes = level.matchTime/1000 / 60;
		gameSeconds = level.matchTime/1000 % 60;

		timelimit = level.timelimit * 60000; // timelimit is in minutes

		if(level.matchTime < timelimit)
		{
			remainingMinutes = (timelimit - level.matchTime)/1000 / 60;
			remainingSeconds = (timelimit - level.matchTime)/1000 % 60 + 1;

			ADMP( va( "%s %02i %02i %02i %02i %02i %i %02i", QQ( N_("^3time: ^*time is ^d$1$:$2$:$3$^* UTC; game time is ^d$4$:$5$^*, reaching time limit in ^d$6$:$7$^*") ),
			  qt.tm_hour, qt.tm_min, qt.tm_sec, gameMinutes, gameSeconds, remainingMinutes, remainingSeconds ) );
		}
		else // requesting time in intermission after the timelimit hit, or timelimit wasn't set
		{
			ADMP( va( "%s %02i %02i %02i %02i %02i", QQ( N_("^3time: ^*time is ^d$1$:$2$:$3$^* UTC; game time is ^d$4$:$5$^*") ),
					  qt.tm_hour, qt.tm_min, qt.tm_sec, gameMinutes, gameSeconds) );
		}
		return true;

	case 2:
		RETURN_IF_INTERMISSION;

		if ( !G_admin_permission( ent, "gametimelimit" ) )
		{
			ADMP( va( "%s %s", QQ( N_("^3$1$:^* permission denied") ), "time" ) );
			return false;
		}

		trap_Argv( 1, tstr, sizeof( tstr ) );

		if ( Str::cisdigit( tstr[0] ) )
		{
			timelimit = atoi( tstr );

			// clip to 0 .. 6 hours
			timelimit = std::min( 6 * 60, std::max( 0, timelimit ) );
			if ( timelimit != level.timelimit )
			{
				AP( va( "print_tr %s %d %d %s", QQ( N_("^3time:^* time limit set to $1$m from $2$m by $3$") ),
				        timelimit, level.timelimit, G_quoted_admin_name( ent ) ) );
				level.timelimit = timelimit;
				// reset 'time remaining' warnings
				level.timelimitWarning = ( level.matchTime < ( level.timelimit - 5 ) * 60000 )
				                       ? TW_NOT
				                       : ( level.matchTime < ( level.timelimit - 1 ) * 60000 )
				                       ? TW_IMMINENT
				                       : TW_PASSED;
			}
			else
			{
				ADMP( QQ( N_("^3time:^* time limit is unchanged") ) );
			}
			return true;
		}
		// fallthrough

	default:
		ADMP( QQ( N_("^3time:^* usage: time [minutes]") ) );
		return false;
	}
}


// this should be in one of the headers, but it is only used here for now
namelog_t *G_NamelogFromString( gentity_t *ent, char *s );

/*
for consistency, we should be able to target a disconnected player with setlevel
but we can't use namelog and remain consistent, so the solution would be to make
everyone a real level 0 admin so they can be targeted until the next level
but that seems kind of stupid
*/
bool G_admin_setlevel( gentity_t *ent )
{
	char            name[ MAX_NAME_LENGTH ];
	char            lstr[ 12 ]; // 11 is max strlen() for 32-bit (signed) int
	gentity_t       *vic = nullptr;
	g_admin_admin_t *a = nullptr;
	g_admin_level_t *l;

	if ( trap_Argc() < 3 )
	{
		ADMP( QQ( N_("^3setlevel:^* usage: setlevel [name|slot#] [level]") ) );
		return false;
	}

	trap_Argv( 1, name, sizeof( name ) );
	trap_Argv( 2, lstr, sizeof( lstr ) );

	if ( !( l = G_admin_level( atoi( lstr ) ) ) )
	{
		ADMP( QQ( N_("^3setlevel:^* level is not defined") ) );
		return false;
	}

	if ( ent && l->level >
	     ( ent->client->pers.admin ? ent->client->pers.admin->level : 0 ) )
	{
		ADMP( QQ( N_("^3setlevel:^* you may not use setlevel to set a level higher "
		      "than your current level") ) );
		return false;
	}

	if ( admin_find_admin( ent, name, "setlevel", &vic, &a ) < 0 )
	{
		return false;
	}

	if ( l->level && vic && G_IsUnnamed( vic->client->pers.netname ) )
	{
		ADMP( QQ( N_("^3setlevel:^* your intended victim has the default name") ) );
		return false;
	}

	if ( ent && !admin_higher_admin( ent->client->pers.admin, a ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* sorry, but your intended victim has a higher admin"
		          " level than you") ), "setlevel" ) );
		return false;
	}

	if ( !a && vic )
	{
		for ( a = g_admin_admins; a && a->next; a = a->next ) {; }

		if ( a )
		{
			a = a->next = (g_admin_admin_t*) BG_Alloc( sizeof( g_admin_admin_t ) );
		}
		else
		{
			a = g_admin_admins = (g_admin_admin_t*) BG_Alloc( sizeof( g_admin_admin_t ) );
		}

		vic->client->pers.admin = a;
		Q_strncpyz( a->guid, vic->client->pers.guid, sizeof( a->guid ) );
		Com_GMTime( &a->lastSeen ); // player is connected...
	}

	if ( !a )
	{
		return false; // Can't Happen
	}

	a->level = l->level;

	if ( vic )
	{
		Q_strncpyz( a->name, vic->client->pers.netname, sizeof( a->name ) );

		if ( l && !a->pubkey[ 0 ] )
		{
			trap_GetPlayerPubkey( vic - g_entities, a->pubkey, sizeof( a->pubkey ) );
		}

		vic->client->pers.pubkey_authenticated = 1;
	}

	admin_log( va( "%d (%s) \"%s^*\"", a->level, a->guid,
	               a->name ) );

	AP( va(
	      "print_tr %s %s %d %s", QQ( N_("^3setlevel:^* $1$^* was given level $2$ admin rights by $3$") ),
	      Quote( a->name ), a->level, G_quoted_admin_name( ent ) ) );

	G_admin_writeconfig();

	if ( vic )
	{
		G_admin_authlog( vic );
		G_admin_cmdlist( vic );
	}

	return true;
}

static g_admin_ban_t *admin_create_ban_entry( gentity_t *ent, char *netname, char *guid,
                                              addr_t *ip, int seconds, const char *reason )
{
	g_admin_ban_t *b = nullptr;
	g_admin_ban_t *prev = nullptr;
	qtime_t       qt;
	int           t;
	int           id = 1;
	int           expired = 0;

	t = Com_GMTime( &qt );

	for ( b = g_admin_bans; b; b = b->next )
	{
		id = b->id + 1; // eventually, next free id

		if ( G_ADMIN_BAN_EXPIRED( b, t ) && !G_ADMIN_BAN_STALE( b, t ) )
		{
			++expired;
		}
	}

	// free stale bans
	b = g_admin_bans;
	while ( b )
	{
		if ( G_ADMIN_BAN_EXPIRED( b, t ) &&
		     ( expired >= MAX_ADMIN_EXPIRED_BANS || G_ADMIN_BAN_STALE( b, t ) ) )
		{
			g_admin_ban_t *u = b;

			if ( prev )
			{
				prev->next = b->next;
				b = prev;
			}
			else
			{
				g_admin_bans = b->next;
				b = g_admin_bans;
			}

			if ( !G_ADMIN_BAN_STALE( u, t ) )
			{
				expired--;
			}

			BG_Free( u );
		}
		else
		{
			prev = b;
		}

		if ( b )
		{
			b = b->next;
		}
	}

	for ( b = g_admin_bans; b && b->next; b = b->next ) {}

	if ( b )
	{
		b = b->next = (g_admin_ban_t*) BG_Alloc( sizeof( g_admin_ban_t ) );
	}
	else
	{
		b = g_admin_bans = (g_admin_ban_t*) BG_Alloc( sizeof( g_admin_ban_t ) );
	}

	b->id = id;
	Q_strncpyz( b->name, netname, sizeof( b->name ) );
	Q_strncpyz( b->guid, guid, sizeof( b->guid ) );
	memcpy( &b->ip, ip, sizeof( b->ip ) );

	Com_sprintf( b->made, sizeof( b->made ), "%04i-%02i-%02i %02i:%02i:%02i",
	             1900 + qt.tm_year, qt.tm_mon + 1, qt.tm_mday,
	             qt.tm_hour, qt.tm_min, qt.tm_sec );

	Q_strncpyz( b->reason, reason, sizeof( b->reason ) );
	Q_strncpyz( b->banner, G_admin_name( ent ), sizeof( b->banner ) );

	if ( !seconds )
	{
		b->expires = 0;
	}
	else
	{
		b->expires = t + seconds;
	}

	return b;
}

static void admin_create_ban( gentity_t *ent,
                              char *netname,
                              char *guid,
                              addr_t *ip,
                              int seconds,
                              const char *reason )
{
	int           i;
	char          disconnect[ MAX_STRING_CHARS ];
	g_admin_ban_t *b = admin_create_ban_entry( ent, netname, guid, ip, seconds, ( reason && *reason ) ? reason : "banned by admin" );

	G_admin_ban_message( nullptr, b, disconnect, sizeof( disconnect ), nullptr, 0 );

	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].pers.connected == CON_DISCONNECTED )
		{
			continue;
		}

		if ( G_admin_ban_matches( b, &g_entities[ i ] ) )
		{
			trap_SendServerCommand( i, va( "disconnect %s", Quote( disconnect ) ) );

			trap_DropClient( i, va( "has been kicked by %s^7. reason: %s",
			                        b->banner, b->reason ) );
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
		if ( !Str::cisdigit( *time ) )
		{
			return -1;
		}

		while ( Str::cisdigit( *time ) )
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
				DAEMON_FALLTHROUGH;

			case 'd':
				num *= 24;
				DAEMON_FALLTHROUGH;

			case 'h':
				num *= 60;
				DAEMON_FALLTHROUGH;

			case 'm':
				num *= 60;
				DAEMON_FALLTHROUGH;

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

static void G_admin_reflag_warnings_ent( int i )
{
	const g_admin_ban_t *ban = nullptr;

	level.clients[ i ].pers.hasWarnings = false;

	while ( ( ban = G_admin_match_ban( level.gentities + i, ban ) ) )
	{
		if ( G_ADMIN_BAN_IS_WARNING( ban ) )
		{
			level.clients[ i ].pers.hasWarnings = true;
			break;
		}
	}
}

static void G_admin_reflag_warnings()
{
	int                 i;

	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].pers.connected != CON_DISCONNECTED )
		{
			G_admin_reflag_warnings_ent( i );
		}
	}
}

bool G_admin_kick( gentity_t *ent )
{
	int       pid;
	char      name[ MAX_NAME_LENGTH ], *reason, err[ MAX_STRING_CHARS ];
	int       minargc;
	gentity_t *vic;
	int       time;

	minargc = 3;

	if ( G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
	{
		minargc = 2;
	}

	if ( trap_Argc() < minargc )
	{
		ADMP( QQ( N_("^3kick:^* usage: kick [name] [reason]") ) );
		return false;
	}

	trap_Argv( 1, name, sizeof( name ) );
	reason = ConcatArgs( 2 );

	if ( ( pid = G_ClientNumberFromString( name, err, sizeof( err ) ) ) == -1 )
	{
		ADMP( va( "%s %s %s", QQ( "^3$1$:^* $2t$" ), "kick", Quote( err) ) );
		return false;
	}

	vic = &g_entities[ pid ];

	if ( !admin_higher( ent, vic ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* sorry, but your intended victim has a higher admin"
		          " level than you") ), "kick" ) );
		return false;
	}

	if ( vic->client->pers.localClient )
	{
		ADMP( QQ( N_("^3kick:^* disconnecting the host would end the game" ) ) );
		return false;
	}

	admin_log( va( "%d (%s) \"%s^*\": \"%s^*\"",
	               pid,
	               vic->client->pers.guid,
	               vic->client->pers.netname,
	               reason ) );
	time = G_admin_parse_time( g_adminTempBan.Get().c_str() );
	admin_create_ban( ent,
	                  vic->client->pers.netname,
	                  vic->client->pers.guid,
	                  &vic->client->pers.ip,
	                  std::max( 1, time ),
	                  ( *reason ) ? reason : "kicked by admin" );
	G_admin_writeconfig();

	return true;
}

bool G_admin_ban( gentity_t *ent )
{
	int       seconds;
	char      search[ MAX_NAME_LENGTH ];
	char      secs[ MAX_TOKEN_CHARS ];
	char      *reason;
	char      duration[ MAX_DURATION_LENGTH ];
	char      time[ MAX_DURATION_LENGTH ];
	int       i;
	addr_t    ip;
	bool  ipmatch = false;
	namelog_t *match = nullptr;

	if ( trap_Argc() < 2 )
	{
		ADMP( QQ( N_("^3ban:^* usage: ban [name|slot|address(/mask)] [duration] [reason]") ) );
		return false;
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
		ADMP( QQ( N_("^3ban:^* you must specify a reason" ) ) );
		return false;
	}

	if ( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
	{
		int maximum = G_admin_parse_time( g_adminMaxBan.Get().c_str() );

		if ( seconds == 0 || seconds > std::max( 1, maximum ) )
		{
			ADMP( QQ( N_("^3ban:^* you may not issue permanent bans") ) );
			seconds = maximum;
		}
	}

	if ( G_AddressParse( search, &ip ) )
	{
		int max = ip.type == IPv4 ? 32 : 64;
		int min = ent ? max / 2 : 1;

		if ( ip.mask < min || ip.mask > max )
		{
			ADMP( va( "%s %d %d %d", QQ( N_("^3ban:^* invalid netmask ($1$ is not one of $2$–$3$)") ),
			          ip.mask, min, max ) );
			return false;
		}

		ipmatch = true;

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
			ADMP( QQ( N_("^3ban:^* no player found by that IP address" ) ) );
			return false;
		}
	}
	else if ( !( match = G_NamelogFromString( ent, search ) ) || match->banned )
	{
		ADMP( QQ( N_("^3ban:^* no match") ) );
		return false;
	}

	if ( ent && !admin_higher_guid( ent->client->pers.guid, match->guid ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* sorry, but your intended victim has a higher admin"
		          " level than you") ), "ban" ) );
		return false;
	}

	if ( match->slot > -1 && level.clients[ match->slot ].pers.localClient )
	{
		ADMP( QQ( N_("^3ban:^* disconnecting the host would end the game") ) );
		return false;
	}

	G_admin_duration( ( seconds ) ? seconds : -1, time, sizeof( time ),
	                  duration, sizeof( duration ) );

	AP( va( "print_tr %s %s %s %s %s %s", QQ( N_("^3ban:^* $1$^* has been banned by $2$^*; "
	        "duration: $3$$4t$, reason: $5t$") ),
	        Quote( match->name[ match->nameOffset ] ),
	        G_quoted_admin_name( ent ),
	        Quote( time ), duration,
	        ( *reason ) ? Quote( reason ) : QQ( N_( "banned by admin" ) ) ) );

	admin_log( va( "%d (%s) \"%s^*\": \"%s^*\"",
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

	match->banned = true;

	if ( g_admin.Get().empty() )
	{
		ADMP( QQ( N_("^3ban:^* WARNING g_admin not set, not saving ban to a file" ) ) );
	}
	else
	{
		G_admin_writeconfig();
	}

	return true;
}

bool G_admin_unban( gentity_t *ent )
{
	int           bnum;
	int           time = Com_GMTime( nullptr );
	char          bs[ 5 ];
	g_admin_ban_t *ban, *p;
	bool      expireOnly;
	bool      wasWarning;

	if ( trap_Argc() < 2 )
	{
		ADMP( QQ( N_("^3unban:^* usage: unban [ban#]") ) );
		return false;
	}

	trap_Argv( 1, bs, sizeof( bs ) );
	bnum = atoi( bs );

	expireOnly = ( bnum > 0 ) && g_adminRetainExpiredBans.Get();
	bnum = abs( bnum );

	for ( ban = p = g_admin_bans; ban && ban->id != bnum; p = ban, ban = ban->next ) {}

	if ( !ban )
	{
		ADMP( QQ( N_("^3unban:^* invalid ban#" ) ) );
		return false;
	}

	if ( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
	{
		int maximum;
		if ( ban->expires == 0 ||
		     ( maximum = G_admin_parse_time( g_adminMaxBan.Get().c_str() ), ban->expires - time > std::max( 1, maximum ) ) )
		{
			ADMP( QQ( N_("^3unban:^* you cannot remove permanent bans") ) );
			return false;
		}
	}

	if ( expireOnly && G_ADMIN_BAN_EXPIRED( ban, time ) )
	{
		ADMP( va( "%s %d", QQ( N_("^3unban:^* ban #$1$ has already expired") ), bnum ) );
		return false;
	}

	wasWarning = G_ADMIN_BAN_IS_WARNING( ban );

	admin_log( va( "%d (%s) \"%s^*\": \"%s^*\": [%s]",
	               ban->expires ? ban->expires - time : 0, ban->guid, ban->name, ban->reason,
	               ban->ip.str ) );

	if ( expireOnly )
	{
		AP( va( "print_tr %s %d %s %s", QQ( N_("^3unban:^* ban #$1$ for $2$^* has been expired by $3$") ),
		        bnum, Quote( ban->name ), G_quoted_admin_name( ent ) ) );

		ban->expires = time;
	}
	else
	{
		AP( va( "print_tr %s %d %s %s", QQ( N_("^3unban:^* ban #$1$ for $2$^* has been removed by $3$") ),
		        bnum, Quote( ban->name ), G_quoted_admin_name( ent ) ) );

		if ( p == ban )
		{
			g_admin_bans = ban->next;
		}
		else
		{
			p->next = ban->next;
		}

		BG_Free( ban );
	}

	if ( wasWarning )
	{
		G_admin_reflag_warnings();
	}

	G_admin_writeconfig();
	return true;
}

bool G_admin_adjustban( gentity_t *ent )
{
	int           bnum;
	int           length, maximum;
	int           expires;
	int           time = Com_GMTime( nullptr );
	char          duration[ MAX_DURATION_LENGTH ] = { "" };
	char          seconds[ MAX_DURATION_LENGTH ];
	char          *reason;
	char          bs[ 5 ];
	char          secs[ MAX_TOKEN_CHARS ];
	char          mode = '\0';
	g_admin_ban_t *ban;
	int           mask = 0;
	int           skiparg = 0;

	if ( trap_Argc() < 3 )
	{
		ADMP( QQ( N_("^3adjustban:^* usage: adjustban [ban#] [/mask] [duration] [reason]"
		      "") ) );
		return false;
	}

	trap_Argv( 1, bs, sizeof( bs ) );
	bnum = atoi( bs );

	for ( ban = g_admin_bans; ban && ban->id != bnum; ban = ban->next ) {}

	if ( !ban )
	{
		ADMP( QQ( N_("^3adjustban:^* invalid ban#") ) );
		return false;
	}

	maximum = G_admin_parse_time( g_adminMaxBan.Get().c_str() );
	maximum = std::max( 1, maximum );

	if ( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
	     ( ban->expires == 0 || ban->expires - time > maximum ) )
	{
		ADMP( QQ( N_("^3adjustban:^* you cannot modify permanent bans") ) );
		return false;
	}

	trap_Argv( 2, secs, sizeof( secs ) );

	if ( secs[ 0 ] == '/' )
	{
		int max = ban->ip.type == IPv6 ? 64 : 32;
		int min = ent ? max / 2 : 1;
		mask = atoi( secs + 1 );

		if ( mask < min || mask > max )
		{
			ADMP( va( "%s %d %d %d", QQ( N_("^3adjustban:^* invalid netmask ($1$ is not one of $2$–$3$)") ),
			          mask, min, max ) );
			return false;
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
				ADMP( QQ( N_("^3adjustban:^* new duration must be explicit" ) ) );
				return false;
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
				ADMP( QQ( N_("^3adjustban:^* ban duration must be positive" ) ) );
				return false;
			}
		}
		else
		{
			length = expires = 0;
		}

		if ( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
		     ( length == 0 || length > maximum ) )
		{
			ADMP( QQ( N_("^3adjustban:^* you may not issue permanent bans") ) );
			expires = time + maximum;
		}

		ban->expires = expires;
		G_admin_duration( ( expires ) ? expires - time : -1, seconds, sizeof( seconds ), duration,
		                  sizeof( duration ) );
	}

	if ( mask )
	{
		char *p = strchr( ban->ip.str, '/' );

		if ( !p )
		{
			p = ban->ip.str + strlen( ban->ip.str );
		}

		if ( mask == ( ban->ip.type == IPv6 ? 64 : 32 ) )
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

	admin_log( va( "%d (%s) \"%s^*\": \"%s^*\": [%s]",
	               ban->expires ? ban->expires - time : 0, ban->guid, ban->name, ban->reason,
	               ban->ip.str ) );
	AP( va( "print_tr %s %d %s %s %s %s %s %s %s %s %s", QQ( N_("^3adjustban:^* ban #$1$ for $2$^* has been updated by $3$^* "
	        "$4$$5t$$6$$7t$$8$$9t$$10$") ),
	        bnum,
	        Quote( ban->name ),
	        G_quoted_admin_name( ent ),
	        ( mask ) ? Quote( va( "netmask: /%d%s", mask, ( length >= 0 || *reason ) ? ", " : "" ) ) : "",
		( length >= 0 ) ? QQ( N_( "duration: " ) ) : "",
		Quote( seconds ), duration,
		( length >= 0 && *reason ) ? ", " : "",
		( *reason ) ? QQ( N_( "reason: ") ) : "",
		Quote( reason ) ) );

	if ( ent )
	{
		Q_strncpyz( ban->banner, ent->client->pers.netname, sizeof( ban->banner ) );
	}

	if ( G_ADMIN_BAN_IS_WARNING( ban ) )
	{
		G_admin_reflag_warnings();
	}

	G_admin_writeconfig();
	return true;
}

bool G_admin_putteam( gentity_t *ent )
{
	int       pid;
	char      name[ MAX_NAME_LENGTH ], team[ sizeof( "spectators" ) ],
	          err[ MAX_STRING_CHARS ];
	gentity_t *vic;
	team_t    teamnum = TEAM_NONE;

	RETURN_IF_INTERMISSION;

	trap_Argv( 1, name, sizeof( name ) );
	trap_Argv( 2, team, sizeof( team ) );

	if ( trap_Argc() < 3 )
	{
		ADMP( QQ( N_("^3putteam:^* usage: putteam [name] [h|a|s]") ) );
		return false;
	}

	if ( ( pid = G_ClientNumberFromString( name, err, sizeof( err ) ) ) == -1 )
	{
		ADMP( va( "%s %s %s", QQ( "^3$1$:^* $2t$" ), "rename", Quote( err ) ) );
		return false;
	}

	vic = &g_entities[ pid ];

	if ( !admin_higher( ent, vic ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* sorry, but your intended victim has a higher admin"
		          " level than you") ), "putteam" ) );
		return false;
	}

	teamnum = G_TeamFromString( team );

	if ( teamnum == NUM_TEAMS )
	{
		ADMP( va( "%s %s", QQ( N_("^3putteam:^* unknown team $1$") ), team ) );
		return false;
	}

	if ( vic->client->pers.team == teamnum )
	{
		return false;
	}

	admin_log( va( "%d (%s) \"%s^*\"", pid, vic->client->pers.guid,
	               vic->client->pers.netname ) );
	G_ChangeTeam( vic, teamnum );

	AP( va( "print_tr %s %s %s %s", QQ( N_("^3putteam:^* $1$^* put $2$^* on to the $3$ team") ),
	        G_quoted_admin_name( ent ),
	        Quote( vic->client->pers.netname ), BG_TeamName( teamnum ) ) );
	return true;
}

bool G_admin_speclock( gentity_t *ent )
{
	int            pid, lockTime = 0;
	char           name[ MAX_NAME_LENGTH ], lock[ MAX_STRING_CHARS ],
	               err[ MAX_STRING_CHARS ], duration[ MAX_DURATION_LENGTH ],
				   time[ MAX_DURATION_LENGTH ];

	gentity_t      *vic;
	g_admin_spec_t *spec;

	RETURN_IF_INTERMISSION;

	if ( trap_Argc() < 3 )
	{
		ADMP( QQ( N_("^3speclock:^* usage: speclock [name] [duration]") ) );
		return false;
	}

	trap_Argv( 1, name, sizeof( name ) );
	trap_Argv( 2, lock, sizeof( lock ) );

	if ( ( pid = G_ClientNumberFromString( name, err, sizeof( err ) ) ) == -1 )
	{
		ADMP( va( "%s %s %s", QQ( "^3$1$:^* $2t$" ), "speclock", Quote( err ) ) );
		return false;
	}

	vic = &g_entities[ pid ];

	if ( !admin_higher( ent, vic ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* sorry, but your intended victim has a higher admin"
		          " level than you") ), "speclock" ) );
		return false;
	}

	spec = G_admin_match_spec( vic );
	lockTime = G_admin_parse_time( lock );

	if ( lockTime == -1 )
	{
		lockTime = G_admin_parse_time( g_adminTempBan.Get().c_str() );

		if ( lockTime == -1 )
		{
			lockTime = 120;
		}
	}

	if ( !spec )
	{
		spec = (g_admin_spec_t*) BG_Alloc( sizeof( g_admin_spec_t ) );
		spec->next = g_admin_specs;
		g_admin_specs = spec;
	}

	Q_strncpyz( spec->guid, vic->client->pers.guid, sizeof( spec->guid ) );

	lockTime = std::min( 86400, lockTime );
	if ( lockTime )
	{
		G_admin_duration( lockTime, time, sizeof( time ), duration, sizeof( duration ) );
		spec->expires = Com_GMTime( nullptr ) + lockTime;
	}
	else
	{
		strcpy( time, "this game" );
		spec->expires = -1;
	}

	admin_log( va( "%d (%s) \"%s^*\" SPECTATE %s%s", pid, vic->client->pers.guid,
	               vic->client->pers.netname, time, duration ) );

	if ( vic->client->pers.team != TEAM_NONE )
	{
		G_ChangeTeam( vic, TEAM_NONE );
		AP( va( "print_tr %s %s %s %s %s", QQ( N_("^3speclock:^* $1$^* put $2$^* on to the spectators team and blocked team-change for $3$$4t$") ),
		        G_quoted_admin_name( ent ),
		        Quote( vic->client->pers.netname ), Quote( time ), duration ) );
	}
	else
	{
		AP( va( "print_tr %s %s %s %s %s", QQ( N_("^3speclock:^* $1$^* blocked team-change for $2$^* for $3$$4t$") ),
		        G_quoted_admin_name( ent ),
		        Quote( vic->client->pers.netname ), Quote( time ), duration ) );
	}

	return true;
}

bool G_admin_specunlock( gentity_t *ent )
{
	int            pid;
	char           name[ MAX_NAME_LENGTH ], err[ MAX_STRING_CHARS ];
	gentity_t      *vic;
	g_admin_spec_t *spec;

	RETURN_IF_INTERMISSION;

	if ( trap_Argc() < 2 )
	{
		ADMP( QQ( N_("^3specunlock:^* usage: specunlock [name]") ) );
		return false;
	}

	trap_Argv( 1, name, sizeof( name ) );

	if ( ( pid = G_ClientNumberFromString( name, err, sizeof( err ) ) ) == -1 )
	{
		ADMP( va( "%s %s %s", QQ( "^3$1$:^* $2t$" ), "specunlock", Quote( err ) ) );
		return false;
	}

	vic = &g_entities[ pid ];

	if ( !admin_higher( ent, vic ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* sorry, but your intended victim has a higher admin"
		          " level than you") ), "specunlock" ) );
		return false;
	}

	spec = G_admin_match_spec( vic );

	if ( spec )
	{
		spec->expires = 0;
		admin_log( va( "%d (%s) \"%s^*\" SPECTATE -", pid, vic->client->pers.guid,
			               vic->client->pers.netname ) );
			AP( va( "print_tr %s %s %s", QQ( N_("^3specunlock:^* $1$^* unblocked team-change for $2$") ),
			        G_quoted_admin_name( ent ),
			        Quote( vic->client->pers.netname ) ) );
	}

	return true;
}

bool G_admin_changemap( gentity_t *ent )
{
	char map[ MAX_QPATH ];
	char layout[ MAX_QPATH ] = { "" };

	if ( trap_Argc() < 2 )
	{
		ADMP( QQ( N_("^3changemap:^* usage: changemap [map] (layout)") ) );
		return false;
	}

	trap_Argv( 1, map, sizeof( map ) );

	if ( !G_MapExists( map ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3changemap:^* invalid map name '$1$'") ), map ) );
		return false;
	}

	if ( trap_Argc() > 2 )
	{
		trap_Argv( 2, layout, sizeof( layout ) );

		if ( !Q_stricmp( layout, S_BUILTIN_LAYOUT ) ||
		     trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, layout ),
		                        nullptr, fsMode_t::FS_READ ) > 0 )
		{
			// nothing to do
		}
		else
		{
			ADMP( va( "%s %s", QQ( N_("^3changemap:^* invalid layout name '$1$'") ), layout ) );
			return false;
		}
	}

	admin_log( map );
	admin_log( layout );

	trap_SendConsoleCommand( va( "map %s %s", Quote( map ), Quote( layout ) ) );

	level.restarted = true;
	G_MapLog_Result( 'M' );
	AP( va( "print_tr %s %s %s %s %s %s", QQ( N_("^3changemap:^* map '$1$' started by $2$^* $3t$$4$$5$") ),
		Quote( map ),
	        G_quoted_admin_name( ent ),
	        ( layout[ 0 ] ) ? QQ( N_( "(forcing layout '") ) : "" ,
			( layout[ 0 ] ) ? Quote( layout ) : "",
			( layout[ 0 ] ) ? QQ( "')" ) : "" ) );
	return true;
}

bool G_admin_warn( gentity_t *ent )
{
	char      reason[ 64 ];
	int       pids[ MAX_CLIENTS ], found, count;
	char      name[ MAX_NAME_LENGTH ], err[ MAX_STRING_CHARS ];
	gentity_t *vic;

	if( trap_Argc() < 3 )
	{
		ADMP( QQ( N_("^3warn:^* usage: warn [name|slot#] [reason]") ) );
		return false;
	}

	trap_Argv( 1, name, sizeof( name ) );

	if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
	{
		count = G_MatchOnePlayer( pids, found, err, sizeof( err ) );
		ADMP( va( "%s %s %s",
				  QQ( "^3$1$:^* $2t$" ),
				  "warn",
				  ( count > 0 ) ? Quote( Substring( err, 0, count ) ) : Quote( err ) ) );
		return false;
	}

	if( !admin_higher( ent, &g_entities[ pids[ 0 ] ] ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* sorry, but your intended victim has a higher admin"
		          " level than you") ), "warn" ) );
		return false;
	}

	vic = &g_entities[ pids[ 0 ] ];

	Color::StripColors( ConcatArgs( 2 ), reason, sizeof( reason ) );

	// create a ban list entry, set warnCount to -1 to indicate that this should NOT result in denying connection
	if ( ent && !ent->client->pers.localClient )
	{
		int time = G_admin_parse_time( g_adminWarn.Get().c_str() );
		admin_create_ban_entry( ent, vic->client->pers.netname, vic->client->pers.guid, &vic->client->pers.ip, std::max(1, time), ( *reason ) ? reason : "warned by admin" )->warnCount = -1;
		vic->client->pers.hasWarnings = true;
	}

	CPx( pids[ 0 ], va( "cp_tr " QQ(N_("^1You have been warned by an administrator:\n^3$1$")) " %s",
	                    Quote( reason ) ) );
	AP( va( "print_tr %s %s %s %s", QQ( N_("^3warn:^* $1$^* has been warned: '$2$' by $3$") ),
	        Quote( vic->client->pers.netname ),
	        Quote( reason ),
	        G_quoted_admin_name( ent ) ) );

	return true;
}

bool G_admin_mute( gentity_t *ent )
{
	char      name[ MAX_NAME_LENGTH ];
	char      command[ MAX_ADMIN_CMD_LEN ];
	namelog_t *vic;

	trap_Argv( 0, command, sizeof( command ) );

	if ( trap_Argc() < 2 )
	{
		ADMP( va( "%s %s %s", QQ( N_("^3$1$:^* usage: $2$ [name|slot#]") ),command, command ) );
		return false;
	}

	trap_Argv( 1, name, sizeof( name ) );

	if ( !( vic = G_NamelogFromString( ent, name ) ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* no match") ), command ) );
		return false;
	}

	if ( ent && !admin_higher_admin( ent->client->pers.admin,
	                                 G_admin_admin( vic->guid ) ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* sorry, but your intended victim has a higher admin"
		          " level than you") ), command ) );
		return false;
	}

	if ( vic->muted )
	{
		if ( !Q_stricmp( command, "mute" ) )
		{
			ADMP( QQ( N_("^3mute:^* player is already muted") ) );
			return false;
		}

		vic->muted = false;

		if ( vic->slot > -1 )
		{
			CPx( vic->slot, "cp_tr " QQ(N_("^1You have been unmuted")) );
		}

		AP( va( "print_tr %s %s %s", QQ( N_("^3unmute:^* $1$^* has been unmuted by $2$") ),
		        Quote( vic->name[ vic->nameOffset ] ),
		        G_quoted_admin_name( ent ) ) );
	}
	else
	{
		if ( !Q_stricmp( command, "unmute" ) )
		{
			ADMP( QQ( N_("^3unmute:^* player is not currently muted") ) );
			return false;
		}

		vic->muted = true;

		if ( vic->slot > -1 )
		{
			CPx( vic->slot, "cp_tr " QQ(N_("^1You've been muted")) );
		}

		AP( va( "print_tr %s %s %s", QQ( N_("^3mute:^* $1$^* has been muted by $2$") ),
		        Quote( vic->name[ vic->nameOffset ] ),
		        G_quoted_admin_name( ent ) ) );
	}

	admin_log( va( "%d (%s) \"%s^*\"", vic->slot, vic->guid,
	               vic->name[ vic->nameOffset ] ) );
	return true;
}

bool G_admin_denybuild( gentity_t *ent )
{
	char      name[ MAX_NAME_LENGTH ];
	char      command[ MAX_ADMIN_CMD_LEN ];
	namelog_t *vic;

	RETURN_IF_INTERMISSION;

	trap_Argv( 0, command, sizeof( command ) );

	if ( trap_Argc() < 2 )
	{
		ADMP( va( "%s %s %s", QQ( N_("^3$1$:^* usage: $2$ [name|slot#]") ), command, command ) );
		return false;
	}

	trap_Argv( 1, name, sizeof( name ) );

	if ( !( vic = G_NamelogFromString( ent, name ) ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* no match") ), command ) );
		return false;
	}

	if ( ent && !admin_higher_admin( ent->client->pers.admin,
	                                 G_admin_admin( vic->guid ) ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* sorry, but your intended victim has a higher admin"
		          " level than you") ), command ) );
		return false;
	}

	if ( vic->denyBuild )
	{
		if ( !Q_stricmp( command, "denybuild" ) )
		{
			ADMP( QQ( N_("^3denybuild:^* player already has no building rights" ) ) );
			return false;
		}

		vic->denyBuild = false;

		if ( vic->slot > -1 )
		{
			CPx( vic->slot, "cp_tr " QQ(N_("^1You've regained your building rights")) );
		}

		AP( va( "print_tr %s %s %s", QQ( N_("^3allowbuild:^* building rights for ^7$1$^* restored by $2$") ),
		        Quote( vic->name[ vic->nameOffset ] ),
		        G_quoted_admin_name( ent ) ) );
	}
	else
	{
		if ( !Q_stricmp( command, "allowbuild" ) )
		{
			ADMP( QQ( N_("^3allowbuild:^* player already has building rights" ) ) );
			return false;
		}

		vic->denyBuild = true;

		if ( vic->slot > -1 )
		{
			level.clients[ vic->slot ].ps.stats[ STAT_BUILDABLE ] = BA_NONE;
			CPx( vic->slot, "cp_tr " QQ(N_("^1You've lost your building rights")) );
		}

		AP( va( "print_tr %s %s %s", QQ( N_("^3denybuild:^* building rights for ^7$1$^* revoked by $2$") ),
		        Quote( vic->name[ vic->nameOffset ] ),
		        G_quoted_admin_name( ent ) ) );
	}

	admin_log( va( "%d (%s) \"%s^*\"", vic->slot, vic->guid,
	               vic->name[ vic->nameOffset ] ) );
	return true;
}

bool G_admin_listadmins( gentity_t *ent )
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

			for ( ; Str::cisdigit( s[ i ] ); i++ ) { }
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
	return true;
}

static bool admin_match_inactive( void *admin, const void *match )
{
	g_admin_admin_t *a = ( g_admin_admin_t * ) admin;
	unsigned int    date = a->lastSeen.tm_year * 10000 + a->lastSeen.tm_mon * 100 + a->lastSeen.tm_mday;
	unsigned int	since = *(unsigned int *) match;

	return date < since;
}

bool G_admin_listinactive( gentity_t *ent )
{
	int          i;
	int          months, date;
	int          start = MAX_CLIENTS;
	char         s[ MAX_NAME_LENGTH ] = { "" };
	qtime_t      tm;

	i = trap_Argc();
	if ( i > 3 )
	{
		ADMP( QQ( N_("^3listinactive:^* usage: listinactive [^5months^7] (^5start admin#^7)") ) );
		return false;
	}

	trap_Argv( 1, s, sizeof( s ) );
	Com_GMTime( &tm );

	months = atoi( s );
	months = months < 1 ? 1 : months; // minimum of 1 month

	// move the date back by the requested no. of months
	tm.tm_mon -= months;

	// correct for -ve month no.
	while ( tm.tm_mon < 0 )
	{
		--tm.tm_year;
		tm.tm_mon += 12;
	}

	// ... and clip to Jan 1900 (which is more than far enough in the past)
	if ( tm.tm_year < 0 )
	{
		tm.tm_mon = tm.tm_year = 0;
	}

	date = tm.tm_year * 10000 + tm.tm_mon * 100 + tm.tm_mday;

	if ( i == 3 ) // (i still contains the argument count)
	{
		// just assume that this is a number
		trap_Argv( 2, s, sizeof( s ) );
		start = atoi( s );
	}

	Com_sprintf( s, sizeof( s ), "%d", months );
	admin_search( ent, "listinactive", "admins", admin_match_inactive, admin_out,
	              g_admin_admins, &date, s, start, MAX_CLIENTS, MAX_ADMIN_LISTITEMS );

	return true;
}

bool G_admin_listlayouts( gentity_t *ent )
{
	char list[ MAX_CVAR_VALUE_STRING ];
	char map[ MAX_QPATH ];
	int  count = 0;
	char *s;
	char layout[ MAX_QPATH ] = { "" };
	unsigned i = 0;

	if ( trap_Argc() == 2 )
	{
		trap_Argv( 1, map, sizeof( map ) );
	}
	else
	{
		trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
	}

	count = G_LayoutList( map, list, sizeof( list ) );
	ADMP( va( "%s %d %s", QQ( N_("^3listlayouts:^* $1$ layouts found for '$2$':") ), count, map ) );
	ADMBP_begin();
	s = &list[ 0 ];

	while ( *s )
	{
		if ( *s == ' ' )
		{
			ADMBP( va( " %s", layout ) );
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
		ADMBP( va( " %s", layout ) );
	}

	ADMBP_end();
	return true;
}

bool G_admin_listgeoip( gentity_t *ent )
{
	int i;
	int clients[ MAX_CLIENTS ];
	int clientCount;
	int countryLength = 1;

	if ( trap_Argc() > 1 )
	{
		char name[ MAX_NAME_LENGTH ];

		trap_Argv( 1, name, sizeof( name ) );
		clientCount = G_ClientNumbersFromString( name, clients, MAX_CLIENTS );

		if ( !clientCount )
		{
			return true;
		}
	}
	else
	{
		for ( clientCount = 0; clientCount < MAX_CLIENTS; ++clientCount )
		{
			clients[ clientCount ] = clientCount;
		}
	}

	for ( i = 0; i < clientCount; i++ )
	{
		int length;
		const gclient_t *p = &level.clients[ clients[ i ] ];

		if ( p->pers.connected == CON_DISCONNECTED )
		{
			continue;
		}

		length = strlen( p->pers.country );
		countryLength = std::max( length, countryLength );
	}

	ADMBP_begin();

	for ( i = 0; i < clientCount; i++ )
	{
		const gclient_t *p = &level.clients[ clients[ i ] ];

		if ( p->pers.connected == CON_DISCONNECTED )
		{
			continue;
		}

		ADMBP( va( "%2i %*s %s^*", clients[ i ], countryLength, p->pers.country[0] ? p->pers.country : "?", p->pers.netname ) );
	}

	ADMBP_end();
	return true;
}

bool G_admin_listplayers( gentity_t *ent )
{
	int             i;
	gclient_t       *p;
	Color::Color    color;
	char            t; // color and team letter
	char            *registeredname;
	char            lname[ MAX_NAME_LENGTH ];
	char            bot, muted, denied;
	int             authed = 1;
	char            namecleaned[ MAX_NAME_LENGTH ];
	char            name2cleaned[ MAX_NAME_LENGTH ];
	g_admin_level_t *l;
	g_admin_level_t *d = G_admin_level( 0 );
	bool        hint;
	bool        canset = G_admin_permission( ent, "setlevel" );
	bool	canseeWarn = G_admin_permission( ent, "warn" ) || G_admin_permission( ent, "ban" );

	ADMP( va( "%s %d", QQ( N_("^3listplayers:^* $1$ players connected:") ),
	           level.numConnectedClients ) );
	ADMBP_begin();

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
			color = Color::Yellow;
		}
		else
		{
			t = Str::ctoupper( * ( BG_TeamName( p->pers.team ) ) );

			if ( p->pers.team == TEAM_HUMANS )
			{
				color = Color::Cyan;
			}
			else if ( p->pers.team == TEAM_ALIENS )
			{
				color = Color::Red;
			}
			else
			{
				color = Color::White;
			}
		}

		bot = ( level.gentities[ i ].r.svFlags & SVF_BOT ) ? 'R' : ' ';
		muted = p->pers.namelog->muted ? 'M' : ' ';
		denied = p->pers.namelog->denyBuild ? 'B' : ' ';

		l = d;
		registeredname = nullptr;
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
		else
		{
			lname[ 0 ] = 0;
		}

		int colorlen = Color::StrlenNocolor( lname );

		ADMBP( va( "%2i %s%c^7 %-2i^2%c^7 %*s^* ^5%c^1%c%c%s^7 %s^* %s%s%s %s",
		           i,
		           Color::ToString( color ).c_str(),
		           t,
		           l ? l->level : 0,
		           hint ? '*' : ' ',
		           admin_level_maxname + colorlen,
		           lname,
		           bot,
		           muted,
		           denied,
		           canseeWarn ? ( p->pers.hasWarnings ? "^3W" : " " ) : "",
		           p->pers.netname,
		           ( registeredname ) ? "(a.k.a. " : "",
		           ( registeredname ) ? registeredname : "",
		           ( registeredname ) ? "^*)" : "",
		           ( !authed ) ? "^1NOT AUTHED" : "" ) );
	}

	ADMBP_end();
	return true;
}

static bool ban_matchip( void *ban, const void *ip )
{
	return G_AddressCompare( & ( ( g_admin_ban_t * ) ban )->ip, ( addr_t * ) ip ) ||
	       G_AddressCompare( ( addr_t * ) ip, & ( ( g_admin_ban_t * ) ban )->ip );
}

static bool ban_matchname( void *ban, const void *name )
{
	char match[ MAX_NAME_LENGTH ];

	G_SanitiseString( ( ( g_admin_ban_t * ) ban )->name, match, sizeof( match ) );
	return strstr( match, ( const char * ) name ) != nullptr;
}

static int ban_out( void *ban, char *str )
{
	unsigned i;
    int t;
	char          duration[ MAX_DURATION_LENGTH ];
	char          time[ MAX_DURATION_LENGTH ];
	Color::Color  d_color = Color::White;
	char          date[ 11 ];
	g_admin_ban_t *b = ( g_admin_ban_t * ) ban;
	char          *made = b->made;

	if ( !str )
	{
		return b->id;
	}

	t = Com_GMTime( nullptr );

	// only print out the date part of made
	date[ 0 ] = '\0';

	for ( i = 0; *made && *made != ' ' && i < sizeof( date ) - 1; i++ )
	{
		date[ i ] = *made++;
	}

	date[ i ] = 0;

	if ( !b->expires || b->expires - t > 0 )
	{
		G_admin_duration( b->expires ? b->expires - t : -1,
						  time, sizeof( time ),
						  duration, sizeof( duration ) );
	}
	else
	{
		*time = 0;
		Q_strncpyz( duration, "expired", sizeof( duration ) );
		d_color = Color::Cyan;
	}

	Com_sprintf( str, MAX_STRING_CHARS, "%s"
	             "         %s\\__ %s%s%-*s %s%-15s ^7%-8s %s"
	             "          %s\\__ %s:^* %s",
	             b->name,
	             Color::ToString( G_ADMIN_BAN_IS_WARNING( b ) ? Color::Yellow : Color::Red ).c_str(),
	             Color::ToString( d_color ).c_str(),
	             time,
	             MAX_DURATION_LENGTH - 1,
	             duration,
	             Color::ToString( ( strchr( b->ip.str, '/' ) ) ? Color::Red : Color::White ).c_str(),
	             b->ip.str,
	             date,
	             b->banner,
	             Color::ToString( G_ADMIN_BAN_IS_WARNING( b ) ? Color::Yellow : Color::Red ).c_str(),
	             G_ADMIN_BAN_IS_WARNING( b ) ? "WARNING" : "BAN",
	             b->reason );

	return b->id;
}

bool G_admin_showbans( gentity_t *ent )
{
	int      i;
	int      start = 1;
	char     filter[ MAX_NAME_LENGTH ] = { "" };
	char     name_match[ MAX_NAME_LENGTH ] = { "" };
	bool ipmatch = false;
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
			for ( i = filter[ 0 ] == '-'; Str::cisdigit( filter[ i ] ); i++ ) {; }
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
	              ipmatch ? ( const char * ) &ip : ( const char * ) name_match,
	              ipmatch ? ( const char * ) &ip : ( const char * ) name_match,
	              start, 1, MAX_ADMIN_SHOWBANS );
	return true;
}

bool G_admin_adminhelp( gentity_t *ent )
{
	g_admin_command_t *c;

	if ( trap_Argc() < 2 )
	{
		int count = 0;
		int width = 13;
		int perline;
		bool perms[ adminNumCmds ];

		for ( unsigned i = 0; i < adminNumCmds; i++ )
		{
			if ( g_admin_cmds[ i ].keyword && G_admin_permission( ent, g_admin_cmds[ i ].flag ) )
			{
				int thiswidth = strlen( g_admin_cmds[ i ].keyword );

				if ( width < thiswidth )
				{
					width = thiswidth;
				}

				perms[ i ] = true;
			}
			else
			{
				perms[ i ] = false;
			}
		}

		for ( c = g_admin_commands; c; c = c->next )
		{
			if ( G_admin_permission( ent, c->flag ) )
			{
				int thiswidth = strlen( c->command );

				if ( width < thiswidth )
				{
					width = thiswidth;
				}
			}
		}

		perline = 78 / ( width + 1 ); // allow for curses console border and at least one space between each word

		std::string out;

		for ( unsigned i = 0; i < adminNumCmds; i++ )
		{
			if ( perms[ i ] )
			{
				out += va( "^3%-*s%c", width, g_admin_cmds[ i ].keyword, ( ++count % perline == 0 ) ? '\n' : ' ' );
			}
		}

		for ( c = g_admin_commands; c; c = c->next )
		{
			if ( G_admin_permission( ent, c->flag ) )
			{
				out += va( "^1%-*s%c", width, c->command, ( ++count % perline == 0 ) ? '\n' : ' ' );
			}
		}

		out += "\n";

		ADMP( Cmd::Escape( out ) );
		ADMP( va( "%s %d", QQ( N_("^3adminhelp:^* $1$ available commands"
		"run adminhelp [^3command^7] for adminhelp with a specific command.") ),count ) );

		return true;
	}
	else
	{
		// /adminhelp param
		char          param[ MAX_ADMIN_CMD_LEN ];
		g_admin_cmd_t *admincmd;
		bool      denied = false;

		trap_Argv( 1, param, sizeof( param ) );

		if ( ( c = G_admin_command( param ) ) )
		{
			if ( G_admin_permission( ent, c->flag ) )
			{
				ADMP( va( "%s %s", QQ( N_("^3adminhelp:^* help for '$1$':") ), c->command ) );

				if ( c->desc[ 0 ] )
				{
					ADMP( va( "%s %s", QQ( N_(" ^3Description:^* $1t$") ), Quote( c->desc ) ) );
				}

				ADMP( va( "%s %s", QQ( N_(" ^3Syntax:^* $1$") ), c->command ) );
				ADMP( va( "%s %s", QQ( N_(" ^3Flag:^* '$1$'") ), c->flag ) );

				return true;
			}

			denied = true;
		}

		if ( ( admincmd = G_admin_cmd( param ) ) )
		{
			if ( G_admin_permission( ent, admincmd->flag ) )
			{
				ADMP( va( "%s %s", QQ( N_("^3adminhelp:^* help for '$1$':") ), admincmd->keyword ) );

				if ( admincmd->function )
				{
					ADMP( va( "%s %s", QQ( N_(" ^3Description:^* $1t$") ), admincmd->function ) );
				}

				ADMP( va( "%s %s %s", QQ( N_(" ^3Syntax:^* $1$ $2t$") ), admincmd->keyword, admincmd->syntax ? Quote( admincmd->syntax ) : "" ) );

				if ( admincmd->flag )
				{
					ADMP( va( "%s %s", QQ( N_(" ^3Flag:^* '$1$'") ), admincmd->flag ) );
				}

				return true;
			}

			denied = true;
		}

		ADMP( va( "%s %s", denied ? QQ( N_("^3adminhelp:^* you do not have permission to use '$1$'") )
		                          : QQ( N_("^3adminhelp:^* no help found for '$1$'") ), param ) );
		return false;
	}
}

bool G_admin_admintest( gentity_t *ent )
{
	g_admin_level_t *l;

	if ( !ent )
	{
		ADMP( QQ( N_("^3admintest:^* you are on the console.") ) );
		return true;
	}

	l = G_admin_level( ent->client->pers.admin ? ent->client->pers.admin->level : 0 );

	AP( va( "print_tr %s %s %d %s %s %s", QQ( N_("^3admintest:^* $1$^* is a level $2$ admin $3$$4$^7$5$") ),
	        Quote( ent->client->pers.netname ),
	        l ? l->level : 0,
	        l ? "(" : "",
	        l ? Quote( l->name ) : "",
	        l ? ")" : "" ) );
	return true;
}

bool G_admin_allready( gentity_t *ent )
{
	int       i = 0;
	gclient_t *cl;

	if ( !level.intermissiontime )
	{
		ADMP( QQ( N_("^3allready:^* this command is only valid during intermission") ));
		return false;
	}

	for ( i = 0; i < level.maxclients; i++ )
	{
		cl = level.clients + i;

		if ( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if ( cl->pers.team == TEAM_NONE )
		{
			continue;
		}

		cl->readyToExit = true;
	}

	AP( va( "print_tr %s %s", QQ( N_("^3allready:^* $1$^* says everyone is READY now") ),
	        G_quoted_admin_name( ent ) ) );
	return true;
}

bool G_admin_endvote( gentity_t *ent )
{
	char     teamName[ sizeof( "spectators" ) ] = { "s" };
	char     command[ MAX_ADMIN_CMD_LEN ];
	team_t   team;
	bool cancel;
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
		ADMP( va( "%s %s %s", QQ( N_( "^3$1$:^* invalid team '$2$'" ) ), command, teamName ) );
		return false;
	}

	msg = va( "print_tr %s %s %s", cancel ? QQ( N_("^3$1$:^* $2$^* decided that everyone voted No") ) :
		QQ( N_("^3$1$:^* $2$^* decided that everyone voted Yes") ),
			  command, G_quoted_admin_name( ent ) );

	if ( !level.team[ team ].voteTime )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* no vote in progress") ), command ) );
		return false;
	}

	admin_log( BG_TeamName( team ) );
	level.team[ team ].voteNo = cancel ? level.team[ team ].numPlayers : 0;
	level.team[ team ].voteYes = cancel ? 0 : level.team[ team ].numPlayers;
	level.team[ team ].quorum = 0;
	G_CheckVote( team );

	if ( team == TEAM_NONE )
	{
		AP( msg );
	}
	else
	{
		G_TeamCommand( team, msg );
	}

	return true;
}

bool G_admin_spec999( gentity_t *ent )
{
	int       i;
	gentity_t *vic;

	RETURN_IF_INTERMISSION;

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

		if ( vic->client->pers.team == TEAM_NONE )
		{
			continue;
		}

		if ( vic->client->ps.ping == 999 )
		{
			G_ChangeTeam( vic, TEAM_NONE );
			AP( va( "print_tr %s %s %s", QQ( N_("^3spec999:^* $1$^* moved $2$^* to spectators") ),
			        G_quoted_admin_name( ent ),
			        Quote( vic->client->pers.netname ) ) );
		}
	}

	return true;
}

bool G_admin_rename( gentity_t *ent )
{
	int       pid;
	char      name[ MAX_NAME_LENGTH ];
	char      newname[ MAX_NAME_LENGTH ];
	char      err[ MAX_STRING_CHARS ];
	char      userinfo[ MAX_INFO_STRING ];
	gentity_t *victim = nullptr;

	if ( trap_Argc() < 3 )
	{
		ADMP( QQ( N_("^3rename:^* usage: rename [name] [newname]") ) );
		return false;
	}

	trap_Argv( 1, name, sizeof( name ) );
	Q_strncpyz( newname, ConcatArgs( 2 ), sizeof( newname ) );

	if ( ( pid = G_ClientNumberFromString( name, err, sizeof( err ) ) ) == -1 )
	{
		ADMP( va( "%s %s %s %s", QQ( "^3$1$: $2t$ $3$" ), "rename", Quote( err ), Quote( name ) ) );
		return false;
	}

	victim = &g_entities[ pid ];

	if ( !admin_higher( ent, victim ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* sorry, but your intended victim has a higher admin"
		          " level than you") ), "rename" ) );
		return false;
	}

	if ( !G_admin_name_check( victim, newname, err, sizeof( err ) ) )
	{
		ADMP( va( "%s %s %s %s", QQ( "^3$1$:^* $2t$ $3$" ), "rename", Quote( err ), Quote( name ) ) );
		return false;
	}

	if ( victim->client->pers.connected != CON_CONNECTED )
	{
		ADMP( QQ( N_("^3rename:^* sorry, but your intended victim is still connecting") ) );
		return false;
	}

	admin_log( va( "%d (%s) \"%s^7\"", pid,
	               victim->client->pers.guid, victim->client->pers.netname ) );
	admin_log( newname );
	trap_GetUserinfo( pid, userinfo, sizeof( userinfo ) );
	AP( va( "print_tr %s %s %s %s", QQ( N_("^3rename:^* $1$^* has been renamed to $2$^* by $3$") ),
	        Quote( victim->client->pers.netname ),
	        Quote( newname ),
	        G_quoted_admin_name( ent ) ) );
	Info_SetValueForKey( userinfo, "name", newname, false );
	trap_SetUserinfo( pid, userinfo );
	ClientUserinfoChanged( pid, true );
	return true;
}

bool G_admin_restart( gentity_t *ent )
{
	char      layout[ MAX_CVAR_VALUE_STRING ] = { "" };
	char      teampref[ MAX_STRING_CHARS ] = { "" };
	int       i;
	gclient_t *cl;

	char      map[ MAX_QPATH ];
	bool  builtin;

	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
	trap_Argv( 1, layout, sizeof( layout ) );

	if ( trap_Argc() > 2 )
	{
		// first one's the layout
		trap_Argv( 2, teampref, sizeof( teampref ) );
	}
	else if ( !Q_stricmp( layout, "keepteams" )       || !Q_stricmp( layout, "kt" )  ||
	          !Q_stricmp( layout, "keepteamslock" )   || !Q_stricmp( layout, "ktl" ) ||
	          !Q_stricmp( layout, "switchteams" )     || !Q_stricmp( layout, "st" )  ||
	          !Q_stricmp( layout, "switchteamslock" ) || !Q_stricmp( layout, "stl" ) )
	{
		// one argument, and it's a flag
		*layout = 0;
		trap_Argv( 1, teampref, sizeof( teampref ) );
	}
	else if ( trap_Argc() > 1 )
	{
		// one argument, and it's a layout name
		// nothing to do
	}
	else
	{
		// no arguments
		// nothing to do
	}

	// check that the layout's available
	builtin = !*layout || !Q_stricmp( layout, S_BUILTIN_LAYOUT );

	if ( !builtin && !trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, layout ), nullptr, fsMode_t::FS_READ ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3restart:^* layout '$1$' does not exist") ), layout ) );
		return false;
	}

	// report
	admin_log( layout );
	admin_log( teampref );

	// cvars
	g_layouts.Set(layout);
	g_mapRestarted.Set("y");

	// handle the flag
	if ( !Q_stricmp( teampref, "keepteams" ) || !Q_stricmp( teampref, "keepteamslock" ) || !Q_stricmp( teampref,"kt" ) || !Q_stricmp( teampref,"ktl" ) )
	{
		for ( i = 0; i < level.maxclients; i++ )
		{
			cl = level.clients + i;

			if ( cl->pers.connected != CON_CONNECTED )
			{
				continue;
			}

			if ( cl->pers.team == TEAM_NONE )
			{
				continue;
			}

			cl->sess.restartTeam = (team_t) cl->pers.team;
		}

		g_mapRestarted.Set("yk");
	}
	else if ( !Q_stricmp( teampref, "switchteams" ) || !Q_stricmp( teampref, "switchteamslock" ) || !Q_stricmp( teampref,"st" ) || !Q_stricmp( teampref,"stl" ))
	{
		for ( i = 0; i < level.maxclients; i++ )
		{
			cl = level.clients + i;

			if ( cl->pers.connected != CON_CONNECTED )
			{
				continue;
			}

			if ( cl->pers.team == TEAM_HUMANS )
			{
				cl->sess.restartTeam = TEAM_ALIENS;
			}
			else if ( cl->pers.team == TEAM_ALIENS )
			{
				cl->sess.restartTeam = TEAM_HUMANS;
			}
		}

		g_mapRestarted.Set("yks");
	}
	else if ( *teampref )
	{
		ADMP( va( "%s %s", QQ( N_( "^3restart:^* unrecognised option '$1$'") ), Quote( teampref ) ) );
		return false;
	}

	if ( !Q_stricmp( teampref, "switchteamslock" ) ||
	     !Q_stricmp( teampref, "keepteamslock" ) ||
		 !Q_stricmp( teampref,"ktl" ) || !Q_stricmp( teampref,"stl" ) )
	{
		g_lockTeamsAtStart.Set(true);
	}

	trap_SendConsoleCommand( "map_restart" );
	G_MapLog_Result( 'R' );

	AP( va( "print_tr %s %s %s %s %s %s %s %s", QQ( N_("^3restart:^* map restarted by $1$ $2t$$3$$4$$5t$$6$$7$") ),
	        G_quoted_admin_name( ent ),
	        ( layout[ 0 ] ) ? QQ( N_( "^7(forcing layout '" ) ) : "",
			( layout[ 0 ] ) ? Quote( layout ) : "",
			( layout[ 0 ] ) ? QQ( "^7') " ) : "",
	        ( teampref[ 0 ] ) ? QQ( N_( "^7(with teams option: '" ) ) : "",
	        ( teampref[ 0 ] ) ? Quote( teampref ) : "",
	        ( teampref[ 0 ] ) ? QQ( "^7')" ) : "" ) );
	return true;
}

bool G_admin_nextmap( gentity_t *ent )
{
	RETURN_IF_INTERMISSION;

	AP( va( "print_tr %s %s", QQ( N_("^3nextmap:^* $1$^* decided to load the next map") ),
	        G_quoted_admin_name( ent ) ) );
	level.lastWin = TEAM_NONE;
	trap_SetConfigstring( CS_WINNER, "Evacuation" );
	G_notify_sensor_end( TEAM_NONE );
	LogExit( va( "nextmap was run by %s", G_admin_name( ent ) ) );
	G_MapLog_Result( 'N' );
	return true;
}

static bool namelog_matchip( void *namelog, const void *ip )
{
	int       i;
	namelog_t *n = ( namelog_t * ) namelog;

	for ( i = 0; i < MAX_NAMELOG_ADDRS && n->ip[ i ].str[ 0 ]; i++ )
	{
		if ( G_AddressCompare( &n->ip[ i ], ( addr_t * ) ip ) ||
		     G_AddressCompare( ( addr_t * ) ip, &n->ip[ i ] ) )
		{
			return true;
		}
	}

	return false;
}

static bool namelog_matchname( void *namelog, const void *name )
{
	char      match[ MAX_NAME_LENGTH ];
	int       i;
	namelog_t *n = ( namelog_t * ) namelog;

	for ( i = 0; i < MAX_NAMELOG_NAMES && n->name[ i ][ 0 ]; i++ )
	{
		G_SanitiseString( n->name[ i ], match, sizeof( match ) );

		if ( strstr( match, ( const char * ) name ) )
		{
			return true;
		}
	}

	return false;
}

static int namelog_out( void *namelog, char *str )
{
	namelog_t  *n = ( namelog_t * ) namelog;
	char       *p = str;
	int        l, l2 = MAX_STRING_CHARS, i;
	Color::Color scolor;

	if ( !str )
	{
		return 0;
	}

	if ( n->slot > -1 )
	{
		scolor = Color::Yellow;
		l = Q_snprintf( p, l2, "%s%-2d", Color::ToString( scolor ).c_str(), n->slot );
		p += l;
		l2 -= l;
	}
	else
	{
		*p++ = '-';
		*p++ = ' ';
		*p = '\0';
		l2 -= 2;
		scolor = Color::White;
	}

	for ( i = 0; i < MAX_NAMELOG_ADDRS && n->ip[ i ].str[ 0 ]; i++ )
	{
		l = Q_snprintf( p, l2, " %s", n->ip[ i ].str );
		p += l;
		l2 -= l;
	}

	for ( i = 0; i < MAX_NAMELOG_NAMES && n->name[ i ][ 0 ]; i++ )
	{
		l = Q_snprintf( p, l2, " '^7%s%s'%s", n->name[ i ], Color::ToString( scolor ).c_str(),
		                i == n->nameOffset ? "*" : "" );
		p += l;
		l2 -= l;
	}

	return 0;
}

bool G_admin_namelog( gentity_t *ent )
{
	char     search[ MAX_NAME_LENGTH ] = { "" };
	char     s2[ MAX_NAME_LENGTH ] = { "" };
	addr_t   ip;
	bool ipmatch = false;
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
			for ( i = search[ 0 ] == '-'; Str::cisdigit( search[ i ] ); i++ ) {; }
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
	              ipmatch ? ( const char * ) &ip : s2, ipmatch ? ( const char * ) &ip : s2,
	              start, MAX_CLIENTS, MAX_ADMIN_LISTITEMS );
	return true;
}

/*
==================
G_NamelogFromString

This is similar to G_ClientNumberFromString but for namelog instead
Returns nullptr if not exactly 1 match
==================
*/
namelog_t *G_NamelogFromString( gentity_t *ent, char *s )
{
	namelog_t *p, *m = nullptr;
	int       i, found = 0;
	char      n2[ MAX_NAME_LENGTH ] = { "" };
	char      s2[ MAX_NAME_LENGTH ] = { "" };

	if ( !s[ 0 ] )
	{
		return nullptr;
	}

	// if a number is provided, it is a clientnum or namelog id
	for ( i = 0; s[ i ] && Str::cisdigit( s[ i ] ); i++ ) {; }

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

		return nullptr;
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
		              namelog_out, level.namelogs, s2, s2, 0, MAX_CLIENTS, -1 );
	}

	return nullptr;
}

bool G_admin_lock( gentity_t *ent )
{
	char     command[ MAX_ADMIN_CMD_LEN ];
	char     teamName[ sizeof( "aliens" ) ];
	team_t   team;
	bool lock, fail = false;

	RETURN_IF_INTERMISSION;

	trap_Argv( 0, command, sizeof( command ) );

	if ( trap_Argc() < 2 )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* usage: $1$ [a|h]") ), command ) );
		return false;
	}

	lock = !Q_stricmp( command, "lock" );
	trap_Argv( 1, teamName, sizeof( teamName ) );
	team = G_TeamFromString( teamName );

	if ( G_IsPlayableTeam( team ) )
	{
		if ( level.team[ team ].locked == lock )
		{
			fail = true;
		}
		else
		{
			level.team[ team ].locked = lock;
		}
	}
	else
	{
		ADMP( va( "%s %s %s", QQ( N_("^3$1$:^* invalid team: '$2$'") ), command, teamName ) );
		return false;
	}

	if ( fail )
	{
		ADMP( va( "%s %s %s", lock ? QQ( N_("^3$1$:^* the $2$ team is already locked") ) :
			QQ( N_("^3$1$:^* the $2$ team is not currently locked") ), command, BG_TeamName( team ) ) );
		return false;
	}

	admin_log( BG_TeamName( team ) );
	if ( lock )
	{
		AP( va( "print_tr %s %s %s %s", QQ( N_("^3$1$:^* the $2$ team has been locked by $3$") ),
		        command, BG_TeamName( team ),
		        G_quoted_admin_name( ent ) ) );
	}
	else
	{
		AP( va( "print_tr %s %s %s %s", QQ( N_("^3$1$:^* the $2$ team has been unlocked by $3$") ),
		        command, BG_TeamName( team ),
		        G_quoted_admin_name( ent ) ) );
	}

	return true;
}


static int G_admin_flag_sort( const void *pa, const void *pb )
{
	const char *a = (const char*) pa;
	const char *b = (const char*) pb;

	if ( *a == '-' || *a == '+' )
	{
		a++;
	}

	if ( *b == '-' || *b == '+' )
	{
		b++;
	}

	return strcmp( a, b );
}

const char *G_admin_flag_update( char *newflags, char *oldflags, int size,
                                 const char *flag, bool add, bool permission )
{
	const char     *token, *token_p;
	const char     *key;
	char     flags[ MAX_ADMIN_FLAG_KEYS ][ MAX_ADMIN_FLAG_LEN ];
	bool found = false;
	int      count = 0;
	int      i;

	if( !flag[ 0 ] )
	{
		return N_("invalid admin flag");
	}

	token_p = oldflags;

	while ( *( token = COM_Parse( &token_p ) ) )
	{
		key = token;

		if ( *key == '-' || *key == '+' )
		{
			key++;
		}

		if ( !strcmp( key, flag ) )
		{
			found = true;
			continue;
		}

		if ( count < MAX_ADMIN_FLAG_KEYS )
		{
			Q_strncpyz( flags[ count ], token, sizeof( flags[ count ] ) );
			count++;
		}
	}

	if ( add )
	{
		if ( count >= MAX_ADMIN_FLAG_KEYS )
		{
			return N_("too many admin flags, flag not set");
		}

		Com_sprintf( flags[ count ], sizeof( flags[ count ] ), "%c%s",
		             ( permission ) ? '+' : '-', flag );
		count++;
	}
	else if ( !found )
	{
		return N_("flag was not present");
	}

	qsort( flags, count, sizeof( flags[ 0 ] ), G_admin_flag_sort );

	// build new string
	newflags[ 0 ] = '\0';

	for ( i = 0; i < count; i++ )
	{
		Q_strcat( newflags, size,
		          va( "%s%s", ( i ) ? " " : "", flags[ i ] ) );
	}

	return nullptr;
}

bool G_admin_flaglist( gentity_t *ent )
{
	bool shown[ adminNumCmds ] = { false };
	int      count = 0;

	ADMP( QQ( N_("^3Ability flags:") ) );
	ADMBP_begin();

	for( unsigned i = 0; i < adminNumFlags; i++ )
	{
		ADMBP( va( "  ^5%-20s ^7%s",
		           g_admin_flags[ i ].flag,
		           g_admin_flags[ i ].description ) );
	}

	ADMBP_end();
	ADMP( QQ( N_("^3Command flags:") ) );
	ADMBP_begin();

	for ( unsigned i = 0; i < adminNumCmds; i++ )
	{
		if ( !g_admin_cmds[ i ].flag || !g_admin_cmds[ i ].flag[ 0 ] || shown[ i ] )
		{
			continue;
		}

		std::string line;

		line += va( "  ^5%-20s^7", g_admin_cmds[ i ].flag );

		for ( unsigned j = i; j < adminNumCmds; j++ )
		{
			if ( g_admin_cmds[ j ].keyword && g_admin_cmds[ j ].flag &&
			     !strcmp ( g_admin_cmds[ j ].flag, g_admin_cmds[ i ].flag ) )
			{
				line += " ";
				line += g_admin_cmds[ j ].keyword;
				shown[ j ] = true;
			}
		}

		line += "^2";

		for ( unsigned j = i; j < adminNumCmds; j++ )
		{
			if ( !g_admin_cmds[ j ].keyword && g_admin_cmds[ j ].flag &&
			     !strcmp ( g_admin_cmds[ j ].flag, g_admin_cmds[ i ].flag ) )
			{
				line += va( " %s", g_admin_cmds[ j ].function );
				shown[ j ] = true;
			}
		}

		ADMBP( line );
		count++;
	}

	ADMBP_end();
	ADMP( va( "%s %d %d", QQ( N_("^3flaglist:^* listed $1$ ability and $2$ command flags") ), (int) adminNumFlags, count ) );

	return true;
}

bool G_admin_flag( gentity_t *ent )
{
	g_admin_admin_t *admin = nullptr;
	g_admin_level_t *level = nullptr;
	gentity_t       *vic = nullptr;
	char            command[ MAX_ADMIN_CMD_LEN ];
	char            name[ MAX_NAME_LENGTH ];
	char            adminname[ MAX_NAME_LENGTH ];
	char            flagbuf[ MAX_ADMIN_FLAG_LEN ];
	char            *flag;
	bool        add = true;
	bool        perm = true;
	const char      *result;
	char            *flagPtr;
	int             flagSize;
	int             i, id;

	enum { ACTION_ALLOWED, ACTION_CLEARED, ACTION_DENIED } action = ACTION_ALLOWED;

	trap_Argv( 0, command, sizeof( command ) );

	if ( trap_Argc() < 2 )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* usage: $1$ [^3name|slot#|admin#|*level#^7] (^5+^7|^5-^7)[^3flag^7]") ), command ) );
		return false;
	}

	trap_Argv( 1, name, sizeof( name ) );

	if ( name[ 0 ] == '*' )
	{
		if ( ent )
		{
			ADMP( va( "%s %s", QQ( N_("^3$1$: only console can change admin level flags") ), command ) );
			return false;
		}

		id = atoi( name + 1 );
		level = G_admin_level( id );

		if ( !level )
		{
			ADMP( va( "%s %s %d", QQ( N_("^3$1$: admin level $2$ does not exist") ), command, id ) );
			return false;
		}

		Com_sprintf( adminname, sizeof( adminname ), "admin level %d", level->level );
	}
	else
	{
		if ( admin_find_admin( ent, name, command, &vic, &admin ) < 0 )
		{
			return false;
		}

		if ( !admin || admin->level == 0 )
		{
			ADMP( va( "%s %s", QQ( N_("^3$1$:^* your intended victim is not an admin") ), command ) );
			return false;
		}

		if ( ent && !admin_higher_admin( ent->client->pers.admin, admin ) )
		{
			ADMP( va( "%s %s", QQ( N_("^3$1$:^* your intended victim has a higher admin level than you") ), command ) );
			return false;
		}

		Q_strncpyz( adminname, admin->name, sizeof( adminname ));
	}

	if( trap_Argc() < 3 )
	{
		if ( !level )
		{
			level = G_admin_level( admin->level );
			ADMP( va( "%s %s %s %s", QQ( N_("^3$1$:^* flags for $2$^* are '^3$3$^*'") ),
			          command, Quote( admin->name ), Quote( admin->flags ) ) );
		}

		if ( level )
		{
			ADMP( va( "%s %s %d %s", QQ( N_("^3$1$:^* admin level $2$ flags are '$3$'") ),
			          command, level->level, Quote( level->flags ) ) );
		}

		return true;
	}

	trap_Argv( 2, flagbuf, sizeof( flagbuf ) );
	flag = flagbuf;

	if ( flag[ 0 ] == '-' || flag[ 0 ] == '+' )
	{
		perm = ( flag[ 0 ] == '+' );
		flag++;

		if ( !perm )
		{
			action = ACTION_DENIED;
		}
	}

	// flag name must be alphanumeric
	for ( i = 0; flag[ i ]; ++i )
	{
		if ( !Str::cisalnum( flag[ i ] ) )
		{
			break;
		}
	}

	if ( !i || flag[ i ] )
	{
		ADMP( va( "%s %s %s", QQ( N_("^3$1$:^* bad flag name '$2$^*'") ), command, Quote( flag ) ) );
		return false;
	}

	if ( !Q_stricmp( command, "unflag" ) )
	{
		add = false;
		action = ACTION_CLEARED;
	}

	if ( ent && ent->client->pers.admin == admin )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* you may not change your own flags (use rcon)") ), command ) );
		return false;
	}

	if ( !G_admin_permission( ent, flag ) )
	{
		ADMP( va( "%s %s", QQ( N_("^3$1$:^* you may only change flags that you also have") ), command ) );
		return false;
	}

	if ( level )
	{
		flagPtr = level->flags;
		flagSize = sizeof( level->flags );
	}
	else
	{
		flagPtr = admin->flags;
		flagSize = sizeof( admin->flags );
	}

	result = G_admin_flag_update( flagPtr, flagPtr, flagSize, flag, add, perm );

	if ( result )
	{
		const char *msg = add
		                ? QQ( N_("^3$1$:^* an error occurred when setting flag '^3$2$^*' for $3$^*, $4t$") )
		                : QQ( N_("^3$1$:^* an error occurred when clearing flag '^3$2$^*' for $3$^*, $4t$") );
		ADMP( va( "%s %s %s %s %s", msg, command, Quote( flag ), Quote( adminname ), Quote( result ) ) );
		return false;
	}

	if ( !G_admin_permission( ent, ADMF_ADMINCHAT ) )
	{
		const char *msg[] = {
			QQ( N_("^3$1$:^* flag '$2$' allowed for $3$") ),
			QQ( N_("^3$1$:^* flag '$2$' cleared for $3$") ),
			QQ( N_("^3$1$:^* flag '$2$' denied for $3$") )
		};
		ADMP( va( "%s %s %s %s", msg[ action ], command, Quote( flag ), Quote( adminname ) ) );
	}

	{
		const char *msg[] = {
			"admin flag '%s' allowed for %s",
			"admin flag '%s' cleared for %s",
			"admin flag '%s' denied for %s"
		};
		G_AdminMessage( ent, va( msg[ action ], flag, adminname ) );
	}

	G_admin_writeconfig();

	if( vic )
	{
		G_admin_authlog( vic );
	}

	return true;
}

bool G_admin_builder( gentity_t *ent )
{
	vec3_t     forward, right, up;
	vec3_t     start, end, dist;
	trace_t    tr;
	gentity_t  *traceEnt;
	buildLog_t *log;
	int        i = 0;
	bool   buildlog;

	RETURN_IF_INTERMISSION;

	if ( !ent )
	{
		ADMP( QQ( N_("^3builder:^* console can't aim.") ) );
		return false;
	}

	buildlog = G_admin_permission( ent, "buildlog" );

	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	if ( ent->client->pers.team != TEAM_NONE &&
	     ent->client->sess.spectatorState == SPECTATOR_NOT )
	{
		G_CalcMuzzlePoint( ent, forward, right, up, start );
	}
	else
	{
		VectorCopy( ent->client->ps.origin, start );
	}

	VectorMA( start, 1000, forward, end );

	trap_Trace( &tr, start, nullptr, nullptr, end, ent->s.number, MASK_PLAYERSOLID, 0 );
	traceEnt = &g_entities[ tr.entityNum ];

	if ( tr.fraction < 1.0f && ( traceEnt->s.eType == entityType_t::ET_BUILDABLE ) )
	{
		const char *builder, *buildingName;

		if ( !buildlog &&
		     ent->client->pers.team != TEAM_NONE &&
		     ent->client->pers.team != traceEnt->buildableTeam )
		{
			ADMP( QQ( N_("^3builder:^* structure not owned by your team" ) ) );
			return false;
		}

		if ( buildlog )
		{
			// i is only valid if buildlog is set (i.e. actor has that flag)
			for ( i = 0; i < level.numBuildLogs; i++ )
			{
				log = &level.buildLog[( level.buildId - i - 1 ) % MAX_BUILDLOG ];

				if ( log->fate == BF_CONSTRUCT && traceEnt->s.modelindex == log->modelindex )
				{
				        VectorSubtract( traceEnt->s.pos.trBase, log->origin, dist );

				        if ( VectorLengthSquared( dist ) < 2.0f )
				        {
						break;
					}
				}
			}
		}

		builder = traceEnt->builtBy ? traceEnt->builtBy->name[ traceEnt->builtBy->nameOffset ] : "<world>";
		buildingName = BG_Buildable( traceEnt->s.modelindex )->humanName;

		if ( !buildingName )
		{
			buildingName = "[unknown building]";
		}

		if ( buildlog && traceEnt->builtBy && i < level.numBuildLogs )
		{
			ADMP( va( "%s %s %s %d", QQ( N_("^3builder:^* $1$ built by $2$^*, buildlog #$3$") ),
				  Quote( buildingName ), Quote( builder ), MAX_CLIENTS + level.buildId - i - 1 ) );
		}
		else if ( traceEnt->builtBy )
		{
			ADMP( va( "%s %s %s", QQ( N_("^3builder:^* $1$ built by $2$^*") ),
				  Quote( buildingName ), Quote( builder ) ) );
		}
		else
		{
			ADMP( va( "%s %s", QQ( N_("^3builder:^* $1$ appears to be a layout item") ),
				  Quote( buildingName ) ) );
		}
	}
	else
	{
		ADMP( QQ( N_("^3builder:^* no structure found under crosshair") ) );
	}

	return true;
}

bool G_admin_pause( gentity_t *ent )
{
	if ( !level.pausedTime )
	{
		AP( va( "print_tr %s %s", QQ( N_("^3pause:^* $1$^* paused the game.") ),
		        G_quoted_admin_name( ent ) ) );
		level.pausedTime = 1;
		trap_SendServerCommand( -1, "cp \"The game has been paused. Please wait.\"" );
	}
	else
	{
		// Prevent accidental pause->unpause race conditions by two admins
		if ( level.pausedTime < 1000 )
		{
			ADMP( QQ( N_("^3pause:^* Unpausing so soon assumed accidental and ignored.") ) );
			return false;
		}

		AP( va( "print_tr %s %s %d", QQ( N_("^3pause:^* $1$^* unpaused the game (paused for $2$ sec)") ),
		        G_quoted_admin_name( ent ),
		        ( int )( ( float ) level.pausedTime / 1000.0f ) ) );
		trap_SendServerCommand( -1, "cp \"The game has been unpaused!\"" );

		level.pausedTime = 0;
	}

	return true;
}

static const char *const fates[] =
{
	N_("^2built^*"),
	N_("^3deconstructed^*"),
	N_("^7replaced^*"),
	N_("^5destroyed^*"),
	N_("^1TEAMKILLED^*"),
	N_("^7unpowered^*"),
	N_("removed")
};
bool G_admin_buildlog( gentity_t *ent )
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
	bool       admin = !ent || G_admin_permission( ent, "buildlog_admin" );
	int        team = admin ? TEAM_NONE : G_Team( ent );
	buildLog_t *log;


	if ( !admin && team == TEAM_NONE )
	{
		ADMP( QQ( N_("^3buildlog:^* spectators have no buildings") ) );
		return false;
	}

	if ( !level.buildId )
	{
		ADMP( QQ( N_("^3buildlog:^* log is empty") ) );
		return true;
	}

	if ( trap_Argc() == 3 )
	{
		trap_Argv( 2, search, sizeof( search ) );
		start = atoi( search );
	}

	if ( trap_Argc() > 1 )
	{
		trap_Argv( 1, search, sizeof( search ) );

		for ( i = search[ 0 ] == '-'; Str::cisdigit( search[ i ] ); i++ ) {; }

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
				ADMP( QQ( N_("^3buildlog:^* invalid client id") ) );
				return false;
			}
		}
		else
		{
			G_SanitiseString( search, s, sizeof( s ) );
		}
	}
	else
	{
		start = std::max( -MAX_ADMIN_LISTITEMS, -level.buildId );
	}

	if ( start < 0 )
	{
		start = std::max( level.buildId - level.numBuildLogs, start + level.buildId );
	}
	else
	{
		start -= MAX_CLIENTS;
	}

	if ( start < level.buildId - level.numBuildLogs || start >= level.buildId )
	{
		ADMP( QQ( N_("^3buildlog:^* invalid build ID") ) );
		return false;
	}

	if ( ent && ent->client->pers.team != TEAM_NONE )
	{
		if ( team == TEAM_NONE )
		{
			trap_SendServerCommand( -1,
			                        va( "print_tr %s %s", QQ( N_("^3buildlog:^* $1$^* requested a log of recent building activity") ),
			                            Quote( ent->client->pers.netname ) ) );
		}
		else
		{
			// FIXME? Send only to team-mates
			trap_SendServerCommand( -1,
			                        va( "print_tr %s %s %s", QQ( N_("^3buildlog:^* $1$^* requested a log of recent $2$ building activity") ),
			                            Quote( ent->client->pers.netname ), Quote( BG_TeamName( team ) ) ) );
		}
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
		else if ( !admin && BG_Buildable( log->modelindex )->team != team )
		{
			continue;
		}

		printed++;
		time = ( log->time - level.startTime ) / 1000;
		Com_sprintf( stamp, sizeof( stamp ), "%3d:%02d", time / 60, time % 60 );
		ADMBP( va( "^2%c^7%-3d %s ^7%s^7%s%s%s %s%s%s",
		           log->actor && log->fate != BF_REPLACE && log->fate != BF_UNPOWER ?
		           '*' : ' ',
		           i + MAX_CLIENTS,
		           log->actor && ( log->fate == BF_REPLACE || log->fate == BF_UNPOWER ) ?
		           "    \\_" : stamp,
		           BG_Buildable( log->modelindex )->humanName,
		           log->builtBy && log->fate != BF_CONSTRUCT ? " (built by " : "",
		           log->builtBy && log->fate != BF_CONSTRUCT ? log->builtBy->name[ log->builtBy->nameOffset ] : "",
		           log->builtBy && log->fate != BF_CONSTRUCT ? "^7)" : "",
		           fates[ log->fate ],
		           log->actor ? " by " : "",
		           log->actor ?
		           log->actor->name[ log->actor->nameOffset ] :
		           "" ) );
	}

	ADMBP_end();

	ADMP( va( "%s %d %d %d %d %d %s", QQ( N_("^3buildlog:^* showing $1$ build logs $2$–$3$ of $4$–$5$.  $6$") ),
	           printed, start + MAX_CLIENTS, i + MAX_CLIENTS - 1,
	           level.buildId + MAX_CLIENTS - level.numBuildLogs,
	           level.buildId + MAX_CLIENTS - 1,
	           i < level.buildId ? va( "run 'buildlog %s%s%d' to see more",
	                                   search, search[ 0 ] ? " " : "", i + MAX_CLIENTS ) : "" ) );
	return true;
}

bool G_admin_revert( gentity_t *ent )
{
	char       arg[ MAX_TOKEN_CHARS ];
	char       duration[ MAX_DURATION_LENGTH ];
	char       time[ MAX_DURATION_LENGTH ];
	int        id;
	buildLog_t *log;

	RETURN_IF_INTERMISSION;

	if ( trap_Argc() != 2 )
	{
		ADMP( QQ( N_("^3revert:^* usage: revert [id]") ) );
		return false;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	id = atoi( arg ) - MAX_CLIENTS;

	if ( id < level.buildId - level.numBuildLogs || id >= level.buildId )
	{
		ADMP( QQ( N_("^3revert:^* invalid id") ) );
		return false;
	}

	log = &level.buildLog[ id % MAX_BUILDLOG ];

	if ( !log->actor || log->fate == BF_REPLACE || log->fate == BF_UNPOWER )
	{
		// fixme: then why list them with an id # in build log ? - rez
		ADMP( QQ( N_("^3revert:^* you can only revert direct player actions, "
		      "indicated by ^2* ^7in buildlog") ) );
		return false;
	}

	G_admin_duration( ( level.time - log->time ) / 1000, time,
	                  sizeof( time ), duration, sizeof( duration ) );
	admin_log( arg );
	AP( va( "print_tr %s %s %d %s %s", ( level.buildId - id ) > 1 ?
		QQ( N_("^3revert:^* $1$^* reverted $2$ changes over the past $3$ $4t$") ) :
		QQ( N_("^3revert:^* $1$^* reverted $2$ change over the past $3$ $4t$") ),
		G_quoted_admin_name( ent ),
	    level.buildId - id,
	    time, duration ) );
	G_BuildLogRevert( id );
	return true;
}

bool G_admin_l0( gentity_t *ent )
{
	char            name[ MAX_NAME_LENGTH ];
	gentity_t       *vic = nullptr;
	g_admin_admin_t *a = nullptr;
	int             id;

	if ( trap_Argc() < 2 )
	{
		ADMP( QQ( N_("^3l0:^* usage: l0 [name|slot#|admin#]") ) );
		return false;
	}

	trap_Argv( 1, name, sizeof( name ) );
	id = admin_find_admin( ent, name, "l0", &vic, &a );

	if ( id < 0 )
	{
		return false;
	}

	if ( !a || a->level != 1 )
	{
		ADMP( QQ( N_("^3l0:^* your intended victim is not level 1") ) );
		return false;
	}

	trap_SendConsoleCommand( va( "setlevel %d 0;", id ) );

	AP( va( "print_tr %s %s %s", QQ( N_("^3l0:^* name protection for $1$^* removed by $2$") ),
	        G_quoted_user_name( vic, a->name ),
	        G_quoted_admin_name( ent ) ) );
	return true;
}

bool G_admin_l1( gentity_t *ent )
{
	char            name[ MAX_NAME_LENGTH ];
	gentity_t       *vic = nullptr;
	g_admin_admin_t *a = nullptr;
	int             id;

	if ( trap_Argc() < 2 )
	{
		ADMP( QQ( N_("^3l1:^* usage: l1 [name|slot#|admin#]") ) );
		return false;
	}

	trap_Argv( 1, name, sizeof( name ) );

	id = admin_find_admin( ent, name, "l1", &vic, &a );

	if ( id < 0 )
	{
		return false;
	}

	if ( a && a->level != 0 )
	{
		ADMP( QQ( N_("^3l1:^* your intended victim is not level 0") ) );
		return false;
	}

	if ( vic && G_IsUnnamed( vic->client->pers.netname ) )
	{
		ADMP( QQ( N_("^3l1:^* your intended victim has the default name") ) );
		return false;
	}

	trap_SendConsoleCommand( va( "setlevel %d 1;", id ) );

	AP( va( "print_tr %s %s %s", QQ( N_("^3l1:^* name protection for $1$^* enabled by $2$") ),
	        G_quoted_user_name( vic, a->name ),
	        G_quoted_admin_name( ent ) ) );
	return true;
}

bool G_admin_register( gentity_t *ent )
{
	int level = 1;

	if ( !ent )
	{
		return false;
	}

	if ( ent->client->pers.admin && ent->client->pers.admin->level != 0 )
	{
		level = ent->client->pers.admin->level;
	}

	if ( G_IsUnnamed( ent->client->pers.netname ) )
	{
		ADMP( QQ( N_("^3register:^* you must first change your name" ) ) );
		return false;
	}

	trap_SendConsoleCommand( va( "setlevel %d %d;",
	                         ( int )( ent - g_entities ),
	                         level ) );

	AP( va( "print_tr %s %s", QQ( N_("^3register:^* $1$^* is now a protected name") ),
	        Quote( ent->client->pers.netname ) ) );

	return true;
}

bool G_admin_unregister( gentity_t *ent )
{
	if ( !ent )
	{
		return false;
	}

	if ( !ent->client->pers.admin || ent->client->pers.admin->level == 0 )
	{
		ADMP( QQ( N_("^3unregister:^* you do not have a protected name" ) ) );
		return false;
	}

	trap_SendConsoleCommand( va( "setlevel %d 0;",
	                         ( int )( ent - g_entities ) ) );

	AP( va( "print_tr %s %s", QQ( N_("^3unregister:^* $1$^* is now an unprotected name") ),
	        Quote( ent->client->pers.admin->name ) ) );

	return true;
}

/*
================
 G_admin_print

 This function facilitates the ADMP define.  ADMP() is similar to CP except
 that it prints the message to the server console if ent is not defined.

 The supplied string is assumed to be quoted as needed.
================
*/
void G_admin_print( gentity_t *ent, Str::StringRef m )
{
	if ( ent )
	{
		trap_SendServerCommand( ent->s.number, va( "print_tr %s", m.c_str() ) );
	}
	else
	{
		trap_SendServerCommand( -2, va( "print_tr %s", m.c_str() ) );
	}
}

void G_admin_print_plural( gentity_t *ent, Str::StringRef m, int number )
{
	if ( ent )
	{
		trap_SendServerCommand( ent->s.number, va( "print_tr_p %d %s", number, m.c_str() ) );
	}
	else
	{
		trap_SendServerCommand( -2, va( "print_tr_p %d %s", number, m.c_str() ) );
	}
}

/*
================
 G_admin_buffer_begin, G_admin_buffer_print, G_admin_buffer_end,

 These function facilitates the ADMBP* defines, and output is as for ADMP().

 The supplied text is raw; it will be quoted but not marked translatable.
 FIXME: it actually is marked translatable (print_tr is used) but shouldn't be
================
*/
void G_admin_buffer_begin()
{
	g_bfb.clear();
	g_bfb.reserve(MAX_MESSAGE_SIZE);
}

void G_admin_buffer_end( gentity_t *ent )
{
	G_admin_print( ent, Cmd::Escape( g_bfb ) );
}

static inline void G_admin_buffer_print_raw( gentity_t *ent, Str::StringRef m, bool appendNewLine )
{
	// Ensure we don't overflow client buffers.
	if ( g_bfb.size() + m.size() >= MAX_MESSAGE_SIZE )
	{
		G_admin_buffer_end( ent );
		G_admin_buffer_begin();
	}
	g_bfb += m;

	if ( appendNewLine )
	{
		g_bfb += '\n';
	}
}

void G_admin_buffer_print_raw( gentity_t *ent, Str::StringRef m )
{
	G_admin_buffer_print_raw( ent, m, false );
}

void G_admin_buffer_print( gentity_t *ent, Str::StringRef m )
{
	G_admin_buffer_print_raw( ent, m, true );
}

void G_admin_cleanup()
{
	g_admin_level_t   *l;
	g_admin_admin_t   *a;
	g_admin_ban_t     *b;
	g_admin_spec_t    *s;
	g_admin_command_t *c;
	void              *n;

	for ( l = g_admin_levels; l; l = (g_admin_level_t*) n )
	{
		n = l->next;
		BG_Free( l );
	}

	g_admin_levels = nullptr;

	for ( a = g_admin_admins; a; a = (g_admin_admin_t*) n )
	{
		n = a->next;
		BG_Free( a );
	}

	g_admin_admins = nullptr;

	for ( b = g_admin_bans; b; b = (g_admin_ban_t*) n )
	{
		n = b->next;
		BG_Free( b );
	}

	g_admin_bans = nullptr;

	for ( s = g_admin_specs; s; s = (g_admin_spec_t*) n )
	{
		n = s->next;
		BG_Free( s );
	}

	g_admin_specs = nullptr;

	for ( c = g_admin_commands; c; c = (g_admin_command_t*) n )
	{
		n = c->next;
		trap_RemoveCommand(c->command);
		BG_Free( c );
	}

	g_admin_commands = nullptr;
}

static void BotUsage( gentity_t *ent )
{
	static const char bot_usage[] = QQ( N_( "^3bot:^* usage: bot add (<name> | *) (aliens | humans) [<skill level> [<behavior>]]\n"
	                                        "            bot fill <count> [<team> [<skill level>]]\n"
	                                        "            bot del (<name> | all)\n"
	                                        "            bot names (aliens | humans) <names>…\n"
	                                        "            bot names (clear | list)\n"
	                                        "            bot behavior (<name> | <slot#>) <behavior>\n"
	                                        "            bot debug_reload" ) );
	ADMP( bot_usage );
}


static int BotSkillFromString( gentity_t* ent, const char* s )
{
	int skill0 = atoi( s );
	int skill = Math::Clamp( skill0, 1, 9 );
	if (skill0 != skill) {
		ADMP( QQ( N_( "Bot skill level should be from 1 to 9" ) ) );
	}
	return skill;
}

static bool BotAddCmd( gentity_t* ent, const Cmd::Args& args )
{
	RETURN_IF_INTERMISSION;

	if ( args.Argc() < 4 || args.Argc() > 6 )
	{
		BotUsage( ent );
		return false;
	}

	const char* name = args[2].data();
	if ( !Q_stricmp( name, "all" ) )
	{
		ADMP( QQ( N_( "bots can't have that name" ) ) );
		return false;
	}

	team_t team = BG_PlayableTeamFromString( args[3].data() );
	if ( team == team_t::TEAM_NONE )
	{
		ADMP( QQ( N_( "Invalid team name" ) ) );
		BotUsage( ent );
		return false;
	}

	int skill;
	if ( args.Argc() >= 5 )
	{
		skill = BotSkillFromString( ent, args[4].data() );
	}
	else
	{
		skill = BOT_DEFAULT_SKILL;
	}

	const char* behavior = args.Argc() >= 6 ? args[5].data() : BOT_DEFAULT_BEHAVIOR;

	bool result = G_BotAdd( name, team, skill, behavior );
	if ( !result )
	{
		ADMP( QQ( N_( "Can't add a bot" ) ) );
	}
	return result;
}

static bool BotFillCmd( gentity_t *ent, const Cmd::Args& args )
{
	if (args.Argc() < 3 || args.Argc() > 5)
	{
		BotUsage( ent );
		return false;
	}
	int count = atoi( args[2].data() );
	std::vector<team_t> teams;
	if ( args.Argc() >= 4 )
	{
		team_t team = BG_PlayableTeamFromString(args[3].data());
		if (team == team_t::TEAM_NONE)
		{
			BotUsage( ent );
			return false;
		}
		teams = { team };
	}
	else
	{
		teams = { team_t::TEAM_ALIENS, team_t::TEAM_HUMANS };
	}
	int skill = args.Argc() >= 5 ? BotSkillFromString(ent, args[4].data()) : BOT_DEFAULT_SKILL;

	for (team_t team : teams)
	{
		level.team[team].botFillTeamSize = count;
		level.team[team].botFillSkillLevel = skill;
	}

	G_BotFill(true);
	return true;
}

bool G_admin_bot( gentity_t *ent )
{
	auto args = trap_Args();
	if ( args.Argc() < 2)
	{
		BotUsage( ent );
		return false;
	}
	const char* arg1 = args[1].data();

	if ( !Q_stricmp( arg1, "add" ) )
	{
		return BotAddCmd( ent, args );
	}
	else if ( !Q_stricmp( arg1, "fill" ) )
	{
		return BotFillCmd( ent, args );
	}
	else if ( !Q_stricmp( arg1, "del" ) && args.Argc() == 3 )
	{
		RETURN_IF_INTERMISSION;

		char err[MAX_STRING_CHARS];
		const char *name = args[2].data();

		if ( !Q_stricmp( name, "all" ) )
		{
			G_BotDelAllBots();
		}
		else
		{
			int clientNum = G_ClientNumberFromString( name, err, sizeof( err ) );
			if ( clientNum == -1 ) //something went wrong when finding the client Number
			{
				ADMP( va( "%s %s %s", QQ( "^3$1$:^* $2t$" ), "bot", Quote( err ) ) );
				return false;
			}
			G_BotDel( clientNum ); //delete the bot
		}
	}
	else if ( !Q_stricmp( arg1, "behavior" ) && args.Argc() == 4 )
	{
		RETURN_IF_INTERMISSION;

		char err[MAX_STRING_CHARS];
		const char *name = args[2].data();
		int clientNum = G_ClientNumberFromString( name, err, sizeof( err ) );
		if ( clientNum == -1 ) //something went wrong when finding the client Number
		{
			ADMP( va( "%s %s %s", QQ( "^3$1$:^* $2t$" ), "bot", Quote( err ) ) );
			return false;
		}
		const char *behavior = args[3].data();
		G_BotChangeBehavior( clientNum, behavior );

	}
	else if ( !Q_stricmp( arg1, "names" ) && args.Argc() >= 3 )
	{
		const char *name = args[2].data();
		team_t team = BG_PlayableTeamFromString( name );

		if ( team == team_t::TEAM_HUMANS )
		{
			int i = G_BotAddNames( team_t::TEAM_HUMANS, 3, trap_Argc() );
			ADMP( va( "%s %d", Quote( P_( "added $1$ human bot name", "added $1$ human bot names", i ) ), i ) );
		}
		else if ( team == team_t::TEAM_ALIENS )
		{
			int i = G_BotAddNames( team_t::TEAM_ALIENS, 3, trap_Argc() );
			ADMP( va( "%s %d", Quote( P_( "added $1$ alien bot name", "added $1$ alien bot names", i ) ), i ) );
		}
		else if ( !Q_stricmp( name, "clear" ) || !Q_stricmp( name, "c" ) )
		{
			if ( args.Argc() > 3 )
			{
				BotUsage( ent );
				return false;
			}
			if ( !G_BotClearNames() )
			{
				ADMP( QQ( N_( "some automatic bot names are in use – not clearing lists" ) ) );
				return false;
			}
		}
		else if ( !Q_stricmp( name, "list" ) || !Q_stricmp( name, "l" ) )
		{
			if ( args.Argc() > 3 )
			{
				BotUsage( ent );
				return false;
			}
			G_BotListNames( ent );
		}
		else
		{
			BotUsage( ent );
			return false;
		}
	}
	else if ( !Q_stricmp( arg1, "debug_reload" )  )
	{
		G_BotDelAllBots();
		G_BotCleanup();
		G_BotInit();
		return true;
	}
	else
	{
		BotUsage( ent );
		return false;
	}
	return true;
}

static bool G_admin_maprestarted( gentity_t *ent )
{
	if ( !ent )
	{
		g_mapRestarted.Set("");
	}

	return true;
}

/*
============
ClientAdminChallenge
============
*/
void ClientAdminChallenge( int clientNum )
{
	gclient_t       *client = level.clients + clientNum;
	g_admin_admin_t *admin = client->pers.admin;

	if ( !client->pers.pubkey_authenticated && admin && admin->pubkey[ 0 ] && ( level.time - client->pers.pubkey_challengedAt ) >= 6000 )
	{
		trap_SendServerCommand( clientNum, va( "pubkey_decrypt %s", admin->msg2 ) );
		client->pers.pubkey_challengedAt = level.time ^ ( 5 * clientNum ); // a small amount of jitter

		// copy the decrypted message because generating a new message will overwrite it
		G_admin_writeconfig();
	}
}

void Cmd_Pubkey_Identify_f( gentity_t *ent )
{
	char            buffer[ MAX_STRING_CHARS ];
	g_admin_admin_t *admin = ent->client->pers.admin;

	if ( trap_Argc() != 2 )
	{
		return;
	}

	if ( ent->client->pers.pubkey_authenticated != 0 || !admin->pubkey[ 0 ] || admin->counter == -1 )
	{
		return;
	}

	trap_Argv( 1, buffer, sizeof( buffer ) );
	if ( Q_strncmp( buffer, admin->msg, MAX_STRING_CHARS ) )
	{
		return;
	}

	ent->client->pers.pubkey_authenticated = 1;
	G_admin_authlog( ent );
	G_admin_cmdlist( ent );
	CP( "cp_tr " QQ(N_("^2Pubkey authenticated")) "" );
}

qtime_t* G_admin_lastSeen( g_admin_admin_t* admin )
{
	return &admin->lastSeen;
}
