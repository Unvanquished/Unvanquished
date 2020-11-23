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

#include "sg_local.h"
#include "engine/qcommon/q_unicode.h"
#include <common/FileSystem.h>
#include "Entities.h"
#include "CBSE.h"

#define CMD_CHEAT        0x0001
#define CMD_CHEAT_TEAM   0x0002 // is a cheat when used on a team
#define CMD_MESSAGE      0x0004 // sends message to others (skip when muted)
#define CMD_TEAM         0x0008 // must be on a team
#define CMD_SPEC         0x0010 // must be a spectator
#define CMD_ALIEN        0x0020
#define CMD_HUMAN        0x0040
#define CMD_ALIVE        0x0080
#define CMD_INTERMISSION 0x0100 // valid during intermission

/*
==================
G_SanitiseString

Remove color codes and non-alphanumeric characters from a string
==================
*/
void G_SanitiseString( const char *in, char *out, int len )
{
	--len;
	for ( const auto& token : Color::Parser( in ) )
	{
		if ( token.Type() == Color::Token::TokenType::CHARACTER )
		{
			int cp = Q_UTF8_CodePoint( token.Begin() );
			if ( Q_Unicode_IsAlphaOrIdeoOrDigit( cp ) )
			{
				int sz = token.Size();
				if ( Q_Unicode_IsUpper( cp ) )
				{
					cp = Q_Unicode_ToLower( cp );
					sz = Q_UTF8_WidthCP( cp );
				}

				if ( len < sz )
				{
					break;
				}

				memcpy( out, Q_UTF8_Encode( cp ), sz );
				out += sz;
				len -= sz;
			}
		}
	}
	*out = '\0';
}

/*
==================
G_MatchOnePlayer

This is a companion function to G_ClientNumbersFromString()

returns true if the int array plist only has one client id, false otherwise
In the case of false, err will be populated with an error message.
==================
*/
int G_MatchOnePlayer( const int *plist, int found, char *err, int len )
{
	err[ 0 ] = '\0';

	if ( found <= 0 )
	{
		Q_strcat( err, len, N_("no connected player by that name or slot #") );
		return -1;
	}

	if ( found > 1 )
	{
		Q_strcat( err, len, N_("more than one player name matches. "
		          "be more specific or use the slot #: ") );

		const int count = strlen( err );
		char      line[ MAX_NAME_LENGTH + 10 ] = { "" };

		for ( int p = 0; p < found; p++ )
		{
			const gclient_t *cl = &level.clients[ plist[p] ];

			if ( cl->pers.connected == CON_CONNECTED )
			{
				Com_sprintf( line, sizeof( line ), "%2i — %s^*",
				             plist[p], cl->pers.netname );

				if ( strlen( err ) + strlen( line ) > (size_t) len )
				{
					break;
				}

				Q_strcat( err, len, line );
			}
		}

		return count;
	}

	return 0;
}

/*
==================
G_ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 and optionally sets err if invalid or not exactly 1 match
err will have a trailing \n if set
==================
*/
int G_ClientNumberFromString( const char *s, char *err, int len )
{
	gclient_t *cl;
	int       i, found = 0, m = -1;
	char      s2[ MAX_NAME_LENGTH ];
	char      n2[ MAX_NAME_LENGTH ];
	char      *p = err;
	int       l, l2 = len;

	if ( !s[ 0 ] )
	{
		if ( p )
		{
			Q_strncpyz( p, N_("no player name or slot # provided"), len );
		}

		return -1;
	}

	// numeric values are just slot numbers
	for ( i = 0; s[ i ] && Str::cisdigit( s[ i ] ); i++ ) {; }

	if ( !s[ i ] )
	{
		i = atoi( s );

		if ( i < 0 || i >= level.maxclients )
		{
			if ( p )
			{
				Q_strncpyz( p, N_("no player connected in that slot #"), len );
			}
			return -1;
		}

		cl = &level.clients[ i ];

		if ( cl->pers.connected == CON_DISCONNECTED )
		{
			if ( p )
			{
				Q_strncpyz( p, N_("no player connected in that slot #"), len );
			}

			return -1;
		}

		return i;
	}

	G_SanitiseString( s, s2, sizeof( s2 ) );

	if ( !s2[ 0 ] )
	{
		if ( p )
		{
			Q_strncpyz( p, N_("no player name provided"), len );
		}

		return -1;
	}

	if ( p )
	{
		Q_strncpyz( p, N_("more than one player name matches. "
		            "be more specific or use the slot #:"), l2 );
		l = strlen( p );
		p += l;
		l2 -= l;
	}

	// check for a name match
	for ( i = 0, cl = level.clients; i < level.maxclients; i++, cl++ )
	{
		if ( cl->pers.connected == CON_DISCONNECTED )
		{
			continue;
		}

		G_SanitiseString( cl->pers.netname, n2, sizeof( n2 ) );

		if ( !strcmp( n2, s2 ) )
		{
			return i;
		}

		if ( strstr( n2, s2 ) )
		{
			if ( p )
			{
				l = Q_snprintf( p, l2, "%-2d — %s^*", i, cl->pers.netname );
				p += l;
				l2 -= l;
			}

			found++;
			m = i;
		}
	}

	if ( found == 1 )
	{
		return m;
	}

	if ( found == 0 && err )
	{
		Q_strncpyz( err, N_("no connected player by that name or slot #"), len );
	}

	return -1;
}

/*
==================
G_ClientNumbersFromString

Sets plist to an array of integers that represent client numbers that have
names that are a partial match for s.

Returns number of matching clientids up to max.
==================
*/
int G_ClientNumbersFromString( const char *s, int *plist, int max )
{
	gclient_t *p;
	int       i, found = 0;
	char      *endptr;
	char      n2[ MAX_NAME_LENGTH ] = { "" };
	char      s2[ MAX_NAME_LENGTH ] = { "" };

	if ( max == 0 )
	{
		return 0;
	}

	if ( !s[ 0 ] )
	{
		return 0;
	}

	// if a number is provided, it is a clientnum
	i = strtol( s, &endptr, 10 );

	if ( *endptr == '\0' )
	{
		if ( i >= 0 && i < level.maxclients )
		{
			p = &level.clients[ i ];

			if ( p->pers.connected != CON_DISCONNECTED )
			{
				*plist = i;
				return 1;
			}
		}

		// we must assume that if only a number is provided, it is a clientNum
		return 0;
	}

	// now look for name matches
	G_SanitiseString( s, s2, sizeof( s2 ) );

	if ( !s2[ 0 ] )
	{
		return 0;
	}

	for ( i = 0; i < level.maxclients && found < max; i++ )
	{
		p = &level.clients[ i ];

		if ( p->pers.connected == CON_DISCONNECTED )
		{
			continue;
		}

		G_SanitiseString( p->pers.netname, n2, sizeof( n2 ) );

		if ( strstr( n2, s2 ) )
		{
			*plist++ = i;
			found++;
		}
	}

	return found;
}

/*
==================
ScoreboardMessage

==================
*/
void ScoreboardMessage( gentity_t *ent )
{
	char      entry[ 1024 ];
	char      string[ 1400 ];
	int       stringlength;
	int       i, j;
	gclient_t *cl;
	int       numSorted;
	weapon_t  weapon = WP_NONE;
	upgrade_t upgrade = UP_NONE;

	// send the latest information on all clients
	string[ 0 ] = 0;
	stringlength = 0;

	numSorted = level.numConnectedClients;

	for ( i = 0; i < numSorted; i++ )
	{
		int ping;

		cl = &level.clients[ level.sortedClients[ i ] ];

		if ( cl->pers.connected == CON_CONNECTING )
		{
			ping = -1;
		}
		else
		{
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		if ( cl->sess.spectatorState == SPECTATOR_NOT &&
		     ( ent->client->pers.team == TEAM_NONE ||
		       cl->pers.team == ent->client->pers.team ) )
		{
			weapon = (weapon_t) cl->ps.weapon;

			if ( BG_InventoryContainsUpgrade( UP_BATTLESUIT, cl->ps.stats ) )
			{
				upgrade = UP_BATTLESUIT;
			}
			else if ( BG_InventoryContainsUpgrade( UP_JETPACK, cl->ps.stats ) )
			{
				upgrade = UP_JETPACK;
			}
			else if ( BG_InventoryContainsUpgrade( UP_RADAR, cl->ps.stats ) )
			{
				upgrade = UP_RADAR;
			}
			else if ( BG_InventoryContainsUpgrade( UP_MEDIUMARMOUR, cl->ps.stats ) )
			{
				upgrade = UP_MEDIUMARMOUR;
			}
			else if ( BG_InventoryContainsUpgrade( UP_LIGHTARMOUR, cl->ps.stats ) )
			{
				upgrade = UP_LIGHTARMOUR;
			}
			else
			{
				upgrade = UP_NONE;
			}
		}
		else
		{
			weapon = WP_NONE;
			upgrade = UP_NONE;
		}

		Com_sprintf( entry, sizeof( entry ),
		             " %d %d %d %d %d %d", level.sortedClients[ i ], cl->ps.persistant[ PERS_SCORE ],
		             ping, ( level.time - cl->pers.enterTime ) / 60000, weapon, upgrade );

		j = strlen( entry );

		if ( stringlength + j >= (int) sizeof( string ) )
		{
			break;
		}

		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	trap_SendServerCommand( ent - g_entities, va( "scores %i %i%s",
	                        level.team[ TEAM_ALIENS ].kills, level.team[ TEAM_HUMANS ].kills, string ) );
}

/*
==================
ConcatArgs
==================
*/
char *ConcatArgs( int start )
{
	int         i, c, tlen;
	static char line[ MAX_STRING_CHARS ];
	int         len;
	char        arg[ MAX_STRING_CHARS ];

	len = 0;
	c = trap_Argc();

	for ( i = start; i < c; i++ )
	{
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );

		if ( len + tlen >= MAX_STRING_CHARS - 1 )
		{
			break;
		}

		memcpy( line + len, arg, tlen );
		len += tlen;

		if ( len == MAX_STRING_CHARS - 1 )
		{
			break;
		}

		if ( i != c - 1 )
		{
			line[ len ] = ' ';
			len++;
		}
	}

	line[ len ] = 0;

	return line;
}

/*
==================
ConcatArgsPrintable
Duplicate of concatargs but enquotes things that need to be
Used to log command arguments in a way that preserves user intended tokenizing
==================
*/
char *ConcatArgsPrintable( int start )
{
	int         i, c, tlen;
	static char line[ MAX_STRING_CHARS ];
	int         len;
	char        arg[ MAX_STRING_CHARS + 2 ];
	char        *printArg;

	len = 0;
	c = trap_Argc();

	for ( i = start; i < c; i++ )
	{
		trap_Argv( i, arg, sizeof( arg ) );
		printArg = Quote( arg );

		tlen = strlen( printArg );

		if ( len + tlen >= MAX_STRING_CHARS - 1 )
		{
			break;
		}

		memcpy( line + len, printArg, tlen );
		len += tlen;

		if ( len == MAX_STRING_CHARS - 1 )
		{
			break;
		}

		if ( i != c - 1 )
		{
			line[ len ] = ' ';
			len++;
		}
	}

	line[ len ] = 0;

	return line;
}

/*
==================
Cmd_Devteam_f

Change team and spawn as builder at the current position
==================
*/
void Cmd_Devteam_f( gentity_t *ent )
{
	gentity_t *spawn;

	if ( trap_Argc() < 2 )
	{
		ADMP( "\"" N_("usage: devteam [a|h]") "\"" );
		return;
	}

	spawn = G_NewEntity();
	VectorCopy( ent->s.pos.trBase, spawn->s.pos.trBase );
	VectorCopy( ent->s.angles, spawn->s.angles );
	VectorCopy( ent->s.origin, spawn->s.origin );
	VectorCopy( ent->s.angles2, spawn->s.angles2 );

	switch ( G_TeamFromString( ConcatArgs( 1 ) ) )
	{
		case TEAM_ALIENS:
			G_LeaveTeam( ent );
			ent->client->pers.team = TEAM_ALIENS;
			ent->client->pers.classSelection = PCL_ALIEN_BUILDER0_UPG;
			ent->client->ps.stats[ STAT_CLASS ] = PCL_ALIEN_BUILDER0_UPG;
			break;
		case TEAM_HUMANS:
			G_LeaveTeam( ent );
			ent->client->pers.team = TEAM_HUMANS;
			ent->client->pers.humanItemSelection = WP_HBUILD;
			ent->client->pers.classSelection = PCL_HUMAN_NAKED;
			ent->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN_NAKED;
			break;
		default:
			ADMP( "\"" N_("usage: devteam [a|h]") "\"" );
			return;
	}

	ent->client->pers.teamChangeTime = level.time;
	ent->client->pers.teamInfo = level.startTime - 1;
	ent->client->sess.spectatorState = SPECTATOR_NOT;

	G_UpdateTeamConfigStrings();
	ClientUserinfoChanged( ent->client->ps.clientNum, false );
	ClientSpawn( ent, spawn, ent->client->ps.origin, nullptr );
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f( gentity_t *ent )
{
	char     *name;
	bool give_all = false;
	float    amount;

	if ( trap_Argc() < 2 )
	{
		ADMP( QQ( N_( "usage: give [what]" ) ) );
		ADMP( QQ( N_( "usage: valid choices are: all, health [amount], funds [amount], "
		              "bp [amount], momentum [amount], stamina, poison, fuel, ammo" ) ) );
		return;
	}

	name = ConcatArgs( 1 );

	if ( Q_stricmp( name, "all" ) == 0 )
	{
		give_all = true;
	}

	if ( give_all || Q_strnicmp( name, "funds", strlen("funds") ) == 0 )
	{
		if ( give_all || trap_Argc() < 3 )
		{
			amount = 30000.0f;
		}
		else
		{
			amount = atof( name + strlen("funds") ) *
			          ( ent->client->pers.team == TEAM_ALIENS ? CREDITS_PER_EVO : 1.0f );
			// clamp credits as G_AddCreditToClient() expects a short int
			amount = Math::Clamp(amount, -30000.0f, 30000.0f);
		}

		G_AddCreditToClient( ent->client, ( short ) amount, true );
	}

	// give momentum
	if ( Q_strnicmp( name, "momentum", strlen("momentum") ) == 0 )
	{
		if ( trap_Argc() < 3 )
		{
			amount = 300.0f;
		}
		else
		{
			amount = atof( name + strlen("momentum") );
		}

		G_AddMomentumGeneric( (team_t) ent->client->pers.team, amount );
	}

	if ( Q_strnicmp( name, "bp", strlen("bp") ) == 0 )
	{
		float bp = trap_Argc() < 3 ? 300.0f : atof( name + strlen("bp") );
		level.team[ent->client->pers.team].totalBudget += bp;
	}

	if ( Entities::IsDead( ent ) || ent->client->sess.spectatorState != SPECTATOR_NOT )
	{
		return;
	}

	if ( give_all || Q_strnicmp( name, "health", strlen("health") ) == 0 )
	{
		if ( give_all || trap_Argc() < 3 )
		{
			ent->entity->Heal(1000.0f, nullptr);
			BG_AddUpgradeToInventory( UP_MEDKIT, ent->client->ps.stats );
		}
		else
		{
			float amount = atof( name + strlen("health") );
			ent->entity->Heal(amount, nullptr);
		}
	}

	if ( give_all || Q_stricmp( name, "stamina" ) == 0 )
	{
		ent->client->ps.stats[ STAT_STAMINA ] = STAMINA_MAX;
	}

	if ( give_all || Q_stricmp( name, "fuel" ) == 0 )
	{
		G_RefillFuel(ent, false);
	}

	if ( Q_stricmp( name, "poison" ) == 0 )
	{
		if ( ent->client->pers.team == TEAM_HUMANS )
		{
			ent->client->ps.stats[ STAT_STATE ] |= SS_POISONED;
			ent->client->lastPoisonTime = level.time;
			ent->client->lastPoisonClient = ent;
		}
		else
		{
			ent->client->ps.stats[ STAT_STATE ] |= SS_BOOSTED;
			ent->client->boostedTime = level.time;
		}
	}

	if ( give_all || Q_stricmp( name, "ammo" ) == 0 )
	{
		G_RefillAmmo( ent, false );
	}
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( gentity_t *ent )
{
	const char *msg;

	ent->flags ^= FL_GODMODE;

	if ( !( ent->flags & FL_GODMODE ) )
	{
		msg = QQ( N_("godmode OFF") );
	}
	else
	{
		msg = QQ( N_("godmode ON") );
	}

	trap_SendServerCommand( ent - g_entities, va( "print_tr %s", msg ) );
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent )
{
	const char *msg;

	ent->flags ^= FL_NOTARGET;

	if ( !( ent->flags & FL_NOTARGET ) )
	{
		msg = QQ( N_("notarget OFF") );
	}
	else
	{
		msg = QQ( N_("notarget ON") );
	}

	trap_SendServerCommand( ent - g_entities, va( "print_tr %s", msg ) );
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent )
{
	const char *msg;

	if ( ent->client->noclip )
	{
		msg = QQ( N_("noclip OFF") );
		ent->r.contents = ent->client->cliprcontents;
	}
	else
	{
		msg = QQ( N_("noclip ON") );
		ent->client->cliprcontents = ent->r.contents;
		ent->r.contents = 0;
	}

	ent->client->noclip = !ent->client->noclip;

	if ( ent->r.linked )
	{
		trap_LinkEntity( ent );
	}

	trap_SendServerCommand( ent - g_entities, va( "print_tr %s", msg ) );
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent )
{
	if ( g_cheats.integer )
	{
		Entities::Kill(ent, MOD_SUICIDE);
	}
	else
	{
		if ( ent->suicideTime == 0 )
		{
			trap_SendServerCommand( ent - g_entities, "print_tr \"" N_("You will suicide in 20 seconds") "\"" );
			ent->suicideTime = level.time + 20000;
		}
		else if ( ent->suicideTime > level.time )
		{
			trap_SendServerCommand( ent - g_entities, "print_tr \"" N_("Suicide cancelled") "\"" );
			ent->suicideTime = 0;
		}
	}
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent )
{
	team_t   team;
	team_t   oldteam = (team_t) ent->client->pers.team;
	char     s[ MAX_TOKEN_CHARS ];
	bool force = G_admin_permission( ent, ADMF_FORCETEAMCHANGE );
	int      players[ NUM_TEAMS ];
	int      t;
	const g_admin_spec_t *specOnly;

	players[ TEAM_ALIENS ] = level.team[ TEAM_ALIENS ].numClients;
	players[ TEAM_HUMANS ] = level.team[ TEAM_HUMANS ].numClients;

	if ( TEAM_ALIENS == oldteam || TEAM_HUMANS == oldteam )
	{
		players[ oldteam ]--;
	}

	// stop team join spam
	if ( ent->client->pers.teamChangeTime &&
	     level.time - ent->client->pers.teamChangeTime < 1000 )
	{
		return;
	}

	// Cannot leave a team while in combat.
	if ( !g_cheats.integer &&
	     g_combatCooldown.integer &&
	     ent->client->lastCombatTime &&
	     ent->client->sess.spectatorState == SPECTATOR_NOT &&
	     Entities::IsAlive( ent ) &&
	     ent->client->lastCombatTime + g_combatCooldown.integer * 1000 > level.time )
	{
		float remaining = ( ( ent->client->lastCombatTime + g_combatCooldown.integer * 1000 ) - level.time ) / 1000;

		trap_SendServerCommand( ent - g_entities,
		    va( "print_tr %s %i %.0f", QQ( N_("You cannot leave your team until $1$ after combat. Try again in $2$s.") ),
		        g_combatCooldown.integer, remaining ) );

		return;
	}

	// disallow joining teams during warmup
	if ( g_doWarmup.integer && ( ( level.warmupTime - level.time ) / 1000 ) > 0 )
	{
		G_TriggerMenu( ent - g_entities, MN_WARMUP );
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	if ( !s[ 0 ] )
	{
		trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s", QQ( N_("team: $1$") ),
		                        Quote( BG_TeamName( oldteam ) ) ) );
		return;
	}

	if ( !Q_stricmp( s, "auto" ) )
	{
		if ( level.team[ TEAM_HUMANS ].locked && level.team[ TEAM_ALIENS ].locked )
		{
			team = TEAM_NONE;
		}
		else if ( level.team[ TEAM_HUMANS ].locked || players[ TEAM_HUMANS ] > players[ TEAM_ALIENS ] )
		{
			team = TEAM_ALIENS;
		}

		else if ( level.team[ TEAM_ALIENS ].locked || players[ TEAM_ALIENS ] > players[ TEAM_HUMANS ] )
		{
			team = TEAM_HUMANS;
		}
		else
		{
			team = (team_t) ( TEAM_ALIENS + rand() / ( RAND_MAX / 2 + 1 ) );
		}
	}
	else
	{
		switch ( G_TeamFromString( s ) )
		{
			case TEAM_NONE:
				team = TEAM_NONE;
				break;

			case TEAM_ALIENS:
				//TODO move code in a function common with next case
				if ( level.team[ TEAM_ALIENS ].locked )
				{
					G_TriggerMenu( ent - g_entities, MN_A_TEAMLOCKED );
					return;
				}
				else if ( level.team[ TEAM_HUMANS ].locked )
				{
					force = true;
				}

				if ( !force && g_teamForceBalance.integer && players[ TEAM_ALIENS ] > players[ TEAM_HUMANS ])
				{
					G_TriggerMenu( ent - g_entities, MN_A_TEAMFULL );
					return;
				}

				team = TEAM_ALIENS;
				break;

			case TEAM_HUMANS:
				//TODO move code in a function common with previous case
				if ( level.team[ TEAM_HUMANS ].locked )
				{
					G_TriggerMenu( ent - g_entities, MN_H_TEAMLOCKED );
					return;
				}
				else if ( level.team[ TEAM_ALIENS ].locked )
				{
					force = true;
				}

				if ( !force && g_teamForceBalance.integer && players[ TEAM_HUMANS ] > players[ TEAM_ALIENS ] )
				{
					G_TriggerMenu( ent - g_entities, MN_H_TEAMFULL );
					return;
				}

				team = TEAM_HUMANS;
				break;

			default:
				trap_SendServerCommand( ent - g_entities,
				                        va( "print_tr %s %s", QQ( N_("Unknown team: $1$") ), Quote( s ) ) );
				return;
		}
	}

	// Cannot join a team for a while after a locking putteam.
	t = Com_GMTime( nullptr );

	if ( team != TEAM_NONE && ( specOnly = G_admin_match_spec( ent ) ) )
	{
		if ( specOnly->expires == -1 )
		{
			trap_SendServerCommand( ent - g_entities,
			                        "print_tr \"" N_("You cannot join a team until the next game.") "\"" );
			return;
		}

		if ( specOnly->expires - t > 0 )
		{
			int remaining = specOnly->expires - t;

			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %d", QQ( N_("You cannot join a team for another $1$s.") ), remaining ) );
			return;
		}
	}

	// stop team join spam
	if ( oldteam == team )
	{
		return;
	}

	if ( team != TEAM_NONE && g_maxGameClients.integer &&
	     level.numPlayingClients >= g_maxGameClients.integer )
	{
		G_TriggerMenu( ent - g_entities, MN_PLAYERLIMIT );
		return;
	}

	// Apply the change
	G_ChangeTeam( ent, team );
}

/*
==================
G_Say
==================
*/
static bool G_SayTo( gentity_t *ent, gentity_t *other, saymode_t mode, const char *message )
{
	if ( !other )
	{
		return false;
	}

	if ( !other->inuse )
	{
		return false;
	}

	if ( !other->client )
	{
		return false;
	}

	if ( other->client->pers.connected != CON_CONNECTED )
	{
		return false;
	}

	if ( ( ent && !G_OnSameTeam( ent, other ) ) &&
	     ( mode == SAY_TEAM || mode == SAY_AREA || mode == SAY_TPRIVMSG ) )
	{
		if ( other->client->pers.team != TEAM_NONE )
		{
			return false;
		}

		// specs with ADMF_SPEC_ALLCHAT flag can see team chat
		if ( !G_admin_permission( other, ADMF_SPEC_ALLCHAT ) && mode != SAY_TPRIVMSG )
		{
			return false;
		}
	}

	if ( mode == SAY_ALL_ADMIN )
	{
		trap_SendServerCommand( other - g_entities, va( "achat %s %d %s",
		                        G_quoted_admin_name( ent ),
		                        mode, Quote( message ) ) );
	}
	else
	{
		trap_SendServerCommand( other - g_entities, va( "chat %ld %d %s",
		                        ent ? ( long )( ent - g_entities ) : -1,
		                        mode, Quote( message ) ) );
	}

	return true;
}

void G_Say( gentity_t *ent, saymode_t mode, const char *chatText )
{
	int       j;
	gentity_t *other;

	// check if blocked by g_specChat 0
	if ( ( !g_specChat.integer ) && ( mode != SAY_TEAM ) &&
	     ( ent ) && ( ent->client->pers.team == TEAM_NONE ) &&
	     ( !G_admin_permission( ent, ADMF_NOCENSORFLOOD ) ) )
	{
		trap_SendServerCommand( ent - g_entities, "print_tr \"" N_("say: Global chatting for "
		                        "spectators has been disabled. You may only use team chat.") "\"" );
		mode = SAY_TEAM;
	}

	switch ( mode )
	{
		case SAY_ALL:
			G_LogPrintf( "Say: %d \"%s^*\": ^2%s",
			             ( ent ) ? ( int )( ent - g_entities ) : -1,
			             ( ent ) ? ent->client->pers.netname : "console", chatText );
			break;

		case SAY_ALL_ADMIN:
			G_LogPrintf( "Say: %d \"%s^*\": ^6%s",
			             ( ent ) ? ( int )( ent - g_entities ) : -1,
			             ( ent ) ? ent->client->pers.netname : "console", chatText );
			break;

		case SAY_TEAM:

			// console say_team is handled in g_svscmds, not here
			if ( !ent || !ent->client )
			{
				Sys::Error( "SAY_TEAM by non-client entity" );
			}

			G_LogPrintf( "SayTeam: %d \"%s^*\": ^5%s",
			             ( int )( ent - g_entities ), ent->client->pers.netname, chatText );
			break;

		case SAY_RAW:
			if ( ent )
			{
				Sys::Error( "SAY_RAW by client entity" );
			}

			G_LogPrintf( "Chat: -1 \"console\": %s", chatText );

		default:
			break;
	}

	// send it to all the appropriate clients
	for ( j = 0; j < level.maxclients; j++ )
	{
		other = &g_entities[ j ];
		G_SayTo( ent, other, mode, chatText );
	}
}

/*
==================
Cmd_SayArea_f
==================
*/
static void Cmd_SayArea_f( gentity_t *ent )
{
	int    entityList[ MAX_GENTITIES ];
	int    num, i;
	vec3_t range = { 1000.0f, 1000.0f, 1000.0f };
	vec3_t mins, maxs;
	char   *msg;

	if ( trap_Argc() < 2 )
	{
		ADMP( "\"" N_("usage: say_area [message]") "\"" );
		return;
	}

	msg = ConcatArgs( 1 );

	for ( i = 0; i < 3; i++ )
	{
		range[ i ] = g_sayAreaRange.value;
	}

	G_LogPrintf( "SayArea: %d \"%s^*\": ^4%s",
	             ( int )( ent - g_entities ), ent->client->pers.netname, msg );

	VectorAdd( ent->s.origin, range, maxs );
	VectorSubtract( ent->s.origin, range, mins );

	num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0; i < num; i++ )
	{
		G_SayTo( ent, &g_entities[ entityList[ i ] ], SAY_AREA, msg );
	}

	//Send to ADMF_SPEC_ALLCHAT candidates
	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( g_entities[ i ].client->pers.team == TEAM_NONE &&
		     G_admin_permission( &g_entities[ i ], ADMF_SPEC_ALLCHAT ) )
		{
			G_SayTo( ent, &g_entities[ i ], SAY_AREA, msg );
		}
	}
}

static void Cmd_SayAreaTeam_f( gentity_t *ent )
{
	int    entityList[ MAX_GENTITIES ];
	int    num, i;
	vec3_t range = { 1000.0f, 1000.0f, 1000.0f };
	vec3_t mins, maxs;
	char   *msg;

	if ( trap_Argc() < 2 )
	{
		ADMP( "\"" N_("usage: say_area_team [message]") "\"" );
		return;
	}

	msg = ConcatArgs( 1 );

	for ( i = 0; i < 3; i++ )
	{
		range[ i ] = g_sayAreaRange.value;
	}

	G_LogPrintf( "SayAreaTeam: %d \"%s^*\": ^4%s",
	             ( int )( ent - g_entities ), ent->client->pers.netname, msg );

	VectorAdd( ent->s.origin, range, maxs );
	VectorSubtract( ent->s.origin, range, mins );

	num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0; i < num; i++ )
	{
		if ( g_entities[ entityList[ i ] ].client &&
			ent->client->pers.team == g_entities[ entityList[ i ] ].client->pers.team )
		{
			G_SayTo( ent, &g_entities[ entityList[ i ] ], SAY_AREA_TEAM, msg );
		}
	}

	//Send to ADMF_SPEC_ALLCHAT candidates
	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( g_entities[ i ].client->pers.team == TEAM_NONE &&
		     G_admin_permission( &g_entities[ i ], ADMF_SPEC_ALLCHAT ) )
		{
			G_SayTo( ent, &g_entities[ i ], SAY_AREA_TEAM, msg );
		}
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent )
{
	char      *p;
	char      cmd[ MAX_TOKEN_CHARS ];
	saymode_t mode = SAY_ALL;

	if ( trap_Argc() < 2 )
	{
		return;
	}

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( Q_stricmp( cmd, "say_team" ) == 0 )
	{
		mode = SAY_TEAM;
	}
	else if ( Q_stricmp( cmd, "asay" ) == 0 )
	{
		if ( !G_admin_permission( ent, ADMF_ADMINCHAT ) )
		{
			ADMP( va( "%s %s", QQ( N_("^3$1$: ^*permission denied") ), "asay" ) );
			return;
		}

		mode = SAY_ALL_ADMIN;
	}

	p = ConcatArgs( 1 );

	G_Say( ent, mode, p );
}

/*
==================
Cmd_Me_f
==================
*/
static void Cmd_Me_f( gentity_t *ent )
{
	char      *p;
	char      cmd[ MAX_TOKEN_CHARS ];
	saymode_t mode = SAY_ALL_ME;

	if ( trap_Argc() < 2 )
	{
		return;
	}

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( Q_stricmp( cmd, "me_team" ) == 0 )
	{
		mode = SAY_TEAM_ME;
	}

	p = ConcatArgs( 1 );

	G_Say( ent, mode, p );
}

/*
==================
Cmd_VSay_f
==================
*/
void Cmd_VSay_f( gentity_t *ent )
{
	char           arg[ MAX_TOKEN_CHARS ];
	voiceChannel_t vchan;
	voice_t        *voice;
	voiceCmd_t     *cmd;
	voiceTrack_t   *track;
	int            cmdNum = 0;
	int            trackNum = 0;
	char           voiceName[ MAX_VOICE_NAME_LEN ] = { "default" };
	char           voiceCmd[ MAX_VOICE_CMD_LEN ] = { "" };
	char           vsay[ 12 ] = { "" };
	weapon_t       weapon;

	if ( !ent || !ent->client )
	{
		Sys::Error( "Cmd_VSay_f() called by non-client entity" );
	}

	trap_Argv( 0, arg, sizeof( arg ) );

	if ( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent - g_entities, va(
		                          "print_tr %s %s", QQ( N_("usage: $1$ command [text]") ),  arg ) );
		return;
	}

	if ( !level.voices )
	{
		trap_SendServerCommand( ent - g_entities, va(
		                          "print_tr %s %s", QQ( N_("$1$: voice system is not installed on this server") ), arg ) );
		return;
	}

	if ( !g_enableVsays.integer )
	{
		trap_SendServerCommand( ent - g_entities, va(
		                          "print_tr %s %s", QQ( N_("$1$: voice system administratively disabled on this server") ),
		                          arg ) );
		return;
	}

	if ( !Q_stricmp( arg, "vsay" ) )
	{
		vchan = VOICE_CHAN_LOCAL;
	}
	else if ( !Q_stricmp( arg, "vsay_team" ) )
	{
		vchan = VOICE_CHAN_TEAM;
	}
	else if ( !Q_stricmp( arg, "vsay_local" ) )
	{
		vchan = VOICE_CHAN_LOCAL;
	}
	else
	{
		return;
	}

	Q_strncpyz( vsay, arg, sizeof( vsay ) );

	if ( ent->client->pers.voice[ 0 ] )
	{
		Q_strncpyz( voiceName, ent->client->pers.voice, sizeof( voiceName ) );
	}

	voice = BG_VoiceByName( level.voices, voiceName );

	if ( !voice )
	{
		trap_SendServerCommand( ent - g_entities, va(
		                          "print_tr %s %s %s", QQ( N_("$1$: voice '$2$' not found") ), vsay, Quote( voiceName ) ) );
		return;
	}

	trap_Argv( 1, voiceCmd, sizeof( voiceCmd ) );
	cmd = BG_VoiceCmdFind( voice->cmds, voiceCmd, &cmdNum );

	if ( !cmd )
	{
		trap_SendServerCommand( ent - g_entities, va(
		                          "print_tr %s %s %s %s", QQ( N_("$1$: command '$2$' not found in voice '$3$'") ),
		                          vsay, Quote( voiceCmd ), Quote( voiceName ) ) );
		return;
	}

	// filter non-spec humans by their primary weapon as well
	weapon = WP_NONE;

	if ( ent->client->sess.spectatorState == SPECTATOR_NOT )
	{
		weapon = BG_PrimaryWeapon( ent->client->ps.stats );
	}

	track = BG_VoiceTrackFind( cmd->tracks, ent->client->pers.team,
	                           ent->client->pers.classSelection, weapon, ( int ) ent->client->voiceEnthusiasm,
	                           &trackNum );

	if ( !track )
	{
		trap_SendServerCommand( ent - g_entities, va("print_tr %s %s %s %d %d %d %d %s",
		                          QQ( N_("$1$: no available track for command '$2$', team $3$, "
		                          "class $4$, weapon $5$, and enthusiasm $6$ in voice '$7$'") ),
		                          vsay, Quote( voiceCmd ), ent->client->pers.team,
		                          ent->client->pers.classSelection, weapon,
		                          ( int ) ent->client->voiceEnthusiasm, Quote( voiceName ) ) );
		return;
	}

	if ( !Q_stricmp( ent->client->lastVoiceCmd, cmd->cmd ) )
	{
		ent->client->voiceEnthusiasm++;
	}

	Q_strncpyz( ent->client->lastVoiceCmd, cmd->cmd,
	            sizeof( ent->client->lastVoiceCmd ) );

	// optional user supplied text
	trap_Argv( 2, arg, sizeof( arg ) );

	switch ( vchan )
	{
		case VOICE_CHAN_ALL:
			trap_SendServerCommand( -1, va(
			                          "voice %ld %d %d %d %s",
			                          ( long )( ent - g_entities ), vchan, cmdNum, trackNum, Quote( arg ) ) );
			break;

		case VOICE_CHAN_TEAM:
			G_TeamCommand( (team_t) ent->client->pers.team, va(
			                 "voice %ld %d %d %d %s",
			                 ( long )( ent - g_entities ), vchan, cmdNum, trackNum, Quote( arg ) ) );
			break;

		case VOICE_CHAN_LOCAL:
			G_AreaTeamCommand( ent, va(
			                 "voice %ld %d %d %d %s",
			                 ( long )( ent - g_entities ), vchan, cmdNum, trackNum, Quote( arg ) ) );
			break;

		default:
			break;
	}
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent )
{
	if ( !ent->client )
	{
		return;
	}

	trap_SendServerCommand( ent - g_entities,
	                        va( "print_tr %s %f %f %f", QQ( N_("origin: $1$ $2$ $3$") ),
	                            ent->s.origin[ 0 ], ent->s.origin[ 1 ],
	                            ent->s.origin[ 2 ] ) );
}


// Basic vote information
// Entries must be in the same order as for voteType_t
enum {
	V_TEAM, V_PUBLIC, V_ANY
};
enum {
	T_NONE, T_PLAYER, T_OTHER
};
enum {
	VOTE_ALWAYS, // default
	VOTE_BEFORE, // within the first N minutes
	VOTE_AFTER,  // not within the first N minutes
	VOTE_REMAIN, // within N/2 minutes before SD
	VOTE_NO_AUTO,// don't automatically vote 'yes'
	VOTE_ENABLE, // for special-purpose enable flags
};
static const struct {
	const char     *name;
	bool        stopOnIntermission;
	int             type;
	int             target;
	bool        adminImmune; // from needing a reason and from being the target
	bool        quorum;
	qtrinary        reasonNeeded;
	const vmCvar_t *percentage;
	int             special;
	const vmCvar_t *specialCvar;
	const vmCvar_t *reasonFlag; // where a reason requirement is configurable (reasonNeeded must be true)
} voteInfo[] = {
	// Name           Stop?  Type      Target     Immune  Quorum    Reason            Vote percentage var  Extra
	{ "kick",         false, V_ANY,    T_PLAYER,  true,   true,     qtrinary::qyes,   &g_kickVotesPercent, VOTE_ALWAYS, nullptr, nullptr },
	{ "spectate",     false, V_ANY,    T_PLAYER,  true,   true,     qtrinary::qyes,   &g_kickVotesPercent, VOTE_ALWAYS, nullptr, nullptr },
	{ "mute",         true,  V_PUBLIC, T_PLAYER,  true,   true,     qtrinary::qyes,   &g_denyVotesPercent, VOTE_ALWAYS, nullptr, nullptr },
	{ "unmute",       true,  V_PUBLIC, T_PLAYER,  false,  true,     qtrinary::qno,    &g_denyVotesPercent, VOTE_ALWAYS, nullptr, nullptr },
	{ "denybuild",    true,  V_TEAM,   T_PLAYER,  true,   true,     qtrinary::qyes,   &g_denyVotesPercent, VOTE_ALWAYS, nullptr, nullptr },
	{ "allowbuild",   true,  V_TEAM,   T_PLAYER,  false,  true,     qtrinary::qno,    &g_denyVotesPercent, VOTE_ALWAYS, nullptr, nullptr },
	{ "extend",       true,  V_PUBLIC, T_OTHER,   false,  false,    qtrinary::qno,    &g_extendVotesPercent,      VOTE_REMAIN, &g_extendVotesTime, nullptr },
	{ "admitdefeat",  true,  V_TEAM,   T_NONE,    false,  true,     qtrinary::qno,    &g_admitDefeatVotesPercent, VOTE_ALWAYS, nullptr, nullptr },
	{ "draw",         true,  V_PUBLIC, T_NONE,    true,   true,     qtrinary::qyes,   &g_drawVotesPercent,        VOTE_AFTER,  &g_drawVotesAfter,  &g_drawVoteReasonRequired },
	{ "map_restart",  true,  V_PUBLIC, T_NONE,    false,  true,     qtrinary::qno,    &g_mapVotesPercent, VOTE_ALWAYS, nullptr, nullptr },
	{ "map",          true,  V_PUBLIC, T_OTHER,   false,  true,     qtrinary::qmaybe, &g_mapVotesPercent,         VOTE_BEFORE, &g_mapVotesBefore, nullptr },
	{ "layout",       true,  V_PUBLIC, T_OTHER,   false,  true,     qtrinary::qno,    &g_mapVotesPercent,         VOTE_BEFORE, &g_mapVotesBefore, nullptr },
	{ "nextmap",      false, V_PUBLIC, T_OTHER,   false,  false,    qtrinary::qmaybe, &g_nextMapVotesPercent, VOTE_ALWAYS, nullptr, nullptr },
	{ "poll",         false, V_ANY,    T_NONE,    false,  false,    qtrinary::qyes,   &g_pollVotesPercent,        VOTE_NO_AUTO, nullptr, nullptr },
	{ "kickbots",     true,  V_PUBLIC, T_NONE,    false,  false,    qtrinary::qno,    &g_kickVotesPercent,        VOTE_ENABLE, &g_botKickVotesAllowedThisMap, nullptr },
	{ }
	// note: map votes use the reason, if given, as the layout name
};

/*
==================
G_CheckStopVote
==================
*/
bool G_CheckStopVote( team_t team )
{
	return level.team[ team ].voteTime && voteInfo[ level.team[ team ].voteType ].stopOnIntermission;
}

/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent )
{
	char   cmd[ MAX_TOKEN_CHARS ],
	       vote[ MAX_TOKEN_CHARS ],
	       arg[ MAX_TOKEN_CHARS ];
	char   name[ MAX_NAME_LENGTH ] = "";
	char   caller[ MAX_NAME_LENGTH ] = "";
	char   reason[ MAX_TOKEN_CHARS ];
	int    clientNum = -1;
	int    id = -1;
	int    voteId;
	team_t team;
	int    i;

	trap_Argv( 0, cmd, sizeof( cmd ) );
	team = (team_t) ( ( !Q_stricmp( cmd, "callteamvote" ) ) ? ent->client->pers.team : TEAM_NONE );

	if ( !g_allowVote.integer )
	{
		trap_SendServerCommand( ent - g_entities,
		                        va( "print_tr %s %s", QQ( N_("$1$: voting not allowed here") ), cmd ) );
		return;
	}

	if ( level.team[ team ].voteTime )
	{
		trap_SendServerCommand( ent - g_entities,
		                        va( "print_tr %s %s", QQ( N_("$1$: a vote is already in progress") ), cmd ) );
		return;
	}

	if ( level.team[ team ].voteExecuteTime )
	{
		G_ExecuteVote( team );
	}

	G_ResetVote( team );

	trap_Argv( 1, vote, sizeof( vote ) );

	// look up the vote detail
	for ( voteId = 0; voteInfo[voteId].name; ++voteId )
	{
		if ( ( team == TEAM_NONE && voteInfo[voteId].type == V_TEAM   ) ||
		     ( team != TEAM_NONE && voteInfo[voteId].type == V_PUBLIC ) )
		{
			continue;
		}
		if ( !Q_stricmp( voteInfo[voteId].name, vote ) )
		{
			break;
		}
	}

	// not found? report & return
	if ( !voteInfo[voteId].name )
	{
		bool added = false;

		trap_SendServerCommand( ent - g_entities, "print_tr \"" N_("Invalid vote string") "\"" );
		trap_SendServerCommand( ent - g_entities, va( "print_tr %s", team == TEAM_NONE ? QQ( N_("Valid vote commands are: ") ) :
			QQ( N_("Valid team-vote commands are: ") ) ) );
		cmd[0] = '\0';

		Q_strcat( cmd, sizeof( cmd ), "print \"" );

		for( voteId = 0; voteInfo[voteId].name; ++voteId )
		{
			if ( ( team == TEAM_NONE && voteInfo[voteId].type != V_TEAM   ) ||
			     ( team != TEAM_NONE && voteInfo[voteId].type != V_PUBLIC ) )
			{
				if ( !voteInfo[voteId].percentage || voteInfo[voteId].percentage->integer > 0 )
				{
					Q_strcat( cmd, sizeof( cmd ), va( "%s%s", added ? ", " : "", voteInfo[voteId].name ) );
					added = true;
				}
			}
		}

		Q_strcat( cmd, sizeof( cmd ), "\"" );
		trap_SendServerCommand( ent - g_entities, cmd );

		return;
	}

	// Check for disabled vote types
	// Does not distinguish between public and team votes
	{
		int        voteNameLength = strlen( vote );
		const char *dv = g_disabledVoteCalls.string;

		while ( *dv )
		{
			const char *delim;

			// skip spaces (and commas)
			while ( *dv && ( *dv == ' ' || *dv == ',' ) )
			{
				++dv;
			}

			if ( !*dv )
			{
				break;
			}

			delim = dv;

			// find the end of this token
			while ( *delim && *delim != ' ' && *delim != ',' )
			{
				++delim;
			}

			// match? if so, complain
			if ( delim - dv == voteNameLength && !Q_strnicmp( dv, vote, voteNameLength ) )
			{
				goto vote_is_disabled; // yes, goto
			}

			dv = delim; // point past the current token
		}
	}

	if ( g_voteLimit.integer > 0 &&
	     ent->client->pers.namelog->voteCount >= g_voteLimit.integer &&
	     !G_admin_permission( ent, ADMF_NO_VOTE_LIMIT ) )
	{
		trap_SendServerCommand( ent - g_entities, va(
		                          "print_tr %s %s %d", QQ( N_("$1$: you have already called the maximum number of votes ($2$)") ),
		                          cmd, g_voteLimit.integer ) );
		return;
	}

	level.team[ team ].voteType = (voteType_t) voteId;

	// Vote time, percentage for pass, quorum
	level.team[ team ].voteDelay = 0;
	level.team[ team ].voteThreshold = voteInfo[voteId].percentage ? voteInfo[voteId].percentage->integer : 50;
	level.team[ team ].quorum = voteInfo[voteId].quorum;

	if ( level.team[ team ].voteThreshold <= 0)
	{
vote_is_disabled:
		trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s", QQ( N_("'$1$' votes have been disabled") ), voteInfo[voteId].name ) );
		return;
	}

	if ( level.team[ team ].voteThreshold > 100)
	{
		level.team[ team ].voteThreshold = 100;
	}

	switch ( voteInfo[voteId].special )
	{
	case VOTE_BEFORE:
		if ( level.matchTime >= ( voteInfo[voteId].specialCvar->integer * 60000 ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %d", QQ( N_("'$1$' votes are not allowed once $2$ minutes have passed") ), voteInfo[voteId].name, voteInfo[voteId].specialCvar->integer ) );
			return;
		}

		break;

	case VOTE_AFTER:
		if ( level.matchTime < ( voteInfo[voteId].specialCvar->integer * 60000 ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %d", QQ( N_("'$1$' votes are not allowed until $2$ minutes have passed") ), voteInfo[voteId].name, voteInfo[voteId].specialCvar->integer ) );
			return;
		}

		break;

	case VOTE_REMAIN:
		if ( !level.timelimit || level.matchTime < ( level.timelimit - voteInfo[voteId].specialCvar->integer / 2 ) * 60000 )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %d", QQ( N_("'$1$' votes are only allowed with less than $2$ minutes remaining") ),
			                            voteInfo[voteId].name, voteInfo[voteId].specialCvar->integer / 2 ) );
			return;
		}

		break;

	case VOTE_ENABLE:
		if ( !voteInfo[voteId].specialCvar->integer )
		{
			trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s", QQ( N_("'$1$' votes have been disabled") ), voteInfo[voteId].name ) );
			return;
		}

		break;

	default:;
	}

	// Get argument and reason, if needed
	arg[0] = '\0';
	reason[0] = '\0';

	if( voteInfo[voteId].target != T_NONE )
	{
		trap_Argv( 2, arg, sizeof( arg ) );
	}

	if( voteInfo[voteId].reasonNeeded != qtrinary::qno )
	{
		char *creason = ConcatArgs( voteInfo[voteId].target != T_NONE ? 3 : 2 );
		Color::StripColors( creason, reason, sizeof( reason ) );
	}

	if ( voteInfo[voteId].target == T_PLAYER )
	{
		char err[ MAX_STRING_CHARS ];

		if ( !arg[ 0 ] )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: no target") ), cmd ) );
			return;
		}

		// with a little extra work only players from the right team are considered
		clientNum = G_ClientNumberFromString( arg, err, sizeof( err ) );

		if ( clientNum == -1 )
		{
			ADMP( va( "%s %s %s", QQ( "$1$: $2t$" ), cmd, Quote( err ) ) );
			return;
		}

		Color::StripColors( level.clients[ clientNum ].pers.netname, name, sizeof( name ) );
		id = level.clients[ clientNum ].pers.namelog->id;

		if ( g_entities[clientNum].r.svFlags & SVF_BOT )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player is a bot") ), cmd ) );
			return;
		}

		if ( voteInfo[voteId].adminImmune && G_admin_permission( g_entities + clientNum, ADMF_IMMUNITY ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: admin is immune") ), cmd ) );
			G_AdminMessage( nullptr,
			                va( "^7%s^3 attempted %s %s"
			                    " on immune admin ^7%s"
			                    " ^3for: %s",
			                    ent->client->pers.netname, cmd, vote,
			                    g_entities[ clientNum ].client->pers.netname,
			                    reason[ 0 ] ? reason : "no reason" ) );
			return;
		}

		if ( level.clients[ clientNum ].pers.localClient )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: admin is immune") ), cmd ) );
			return;
		}

		if ( team != TEAM_NONE &&
			 ent->client->pers.team != level.clients[ clientNum ].pers.team )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player is not on your team") ), cmd ) );
			return;
		}
	}

	if ( voteInfo[voteId].reasonNeeded == qtrinary::qyes && !reason[ 0 ] &&
	     !( voteInfo[voteId].adminImmune && G_admin_permission( ent, ADMF_UNACCOUNTABLE ) ) &&
	     !( voteInfo[voteId].reasonFlag && voteInfo[voteId].reasonFlag->integer ) )
	{
		trap_SendServerCommand( ent - g_entities,
		                        va( "print_tr %s %s", QQ( N_("$1$: You must provide a reason") ), cmd ) );
		return;
	}

	switch( voteId )
	{
	case VOTE_KICK:
		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "ban %s 1s%s %s^* called vote kick (%s^*)", level.clients[ clientNum ].pers.ip.str,
		             Quote( g_adminTempBan.string ), Quote( ent->client->pers.netname ), Quote( reason ) );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ), N_("Kick player '%s'"), name );
		break;

	case VOTE_SPECTATE:
		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "speclock %d 1s%s", clientNum, Quote( g_adminTempBan.string ) );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             N_("Move player '%s' to spectators"), name );
		break;

	case VOTE_BOT_KICK:
		for ( i = 0; i < MAX_CLIENTS; ++i )
		{
			if ( g_entities[i].r.svFlags & SVF_BOT &&
			     g_entities[i].client->pers.team != TEAM_NONE )
			{
				break;
			}
		}

		if ( i == MAX_CLIENTS )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: there are no active bots") ), cmd ) );
			return;
		}

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ), "bot del all" );
		Com_sprintf( level.team[ team ].voteDisplayString, sizeof( level.team[ team ].voteDisplayString ), N_("Remove all bots") );

		break;

	case VOTE_MUTE:
		if ( level.clients[ clientNum ].pers.namelog->muted )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player is already muted") ), cmd ) );
			return;
		}

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "mute %d", id );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             N_("Mute player '%s'"), name );
		break;

	case VOTE_UNMUTE:
		if ( !level.clients[ clientNum ].pers.namelog->muted )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player is not currently muted") ), cmd ) );
			return;
		}

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "unmute %d", id );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             N_("Unmute player '%s'"), name );
		break;

	case VOTE_DENYBUILD:
		if ( level.clients[ clientNum ].pers.namelog->denyBuild )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player already lost building rights") ), cmd ) );
			return;
		}

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "denybuild %d", id );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             "Take away building rights from '%s'", name );
		break;

	case VOTE_ALLOWBUILD:
		if ( !level.clients[ clientNum ].pers.namelog->denyBuild )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player already has building rights") ), cmd ) );
			return;
		}

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "allowbuild %d", id );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             "Allow '%s' to build", name );
		break;

	case VOTE_EXTEND:
		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "time %i", level.timelimit + g_extendVotesTime.integer );
		Com_sprintf( level.team[ team ].voteDisplayString, sizeof( level.team[ team ].voteDisplayString ),
		             "Extend the timelimit by %d minutes", g_extendVotesTime.integer );
		break;

	case VOTE_ADMIT_DEFEAT:
		level.team[ team ].voteDelay = 3000;

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "admitdefeat %d", team );
		strcpy( level.team[ team ].voteDisplayString, "Admit Defeat" );
		break;

	case VOTE_DRAW:
		level.team[ team ].voteDelay = 3000;
		strcpy( level.team[ team ].voteString, "evacuation" );
		strcpy( level.team[ team ].voteDisplayString, "End match in a draw" );
		break;

	case VOTE_MAP_RESTART:
		strcpy( level.team[ team ].voteString, vote );
		strcpy( level.team[ team ].voteDisplayString, "Restart current map" );
		// map_restart comes with a default delay
		break;

	case VOTE_MAP:
		if ( !G_MapExists( arg ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %s", QQ( N_("$1$: 'maps/$2$.bsp' could not be found on the server") ),
			                            cmd, Quote( arg ) ) );
			return;
		}

		level.team[ team ].voteDelay = 3000;

		if ( *reason ) // layout?
		{
			Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
			             "map %s %s", Quote( arg ), Quote( reason ) );
			Com_sprintf( level.team[ team ].voteDisplayString,
			             sizeof( level.team[ team ].voteDisplayString ),
			             "Change to map '%s' layout '%s'", arg, reason );
		}
		else
		{
			Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
			             "map %s", Quote( arg ) );
			Com_sprintf( level.team[ team ].voteDisplayString,
			             sizeof( level.team[ team ].voteDisplayString ),
			             "Change to map '%s'", arg );
		}

		reason[0] = '\0'; // nullify since we've used it here...
		break;

	case VOTE_LAYOUT:
		{
			char map[ 64 ];

			trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );

			if ( Q_stricmp( arg, S_BUILTIN_LAYOUT ) &&
			     !trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, arg ), nullptr, fsMode_t::FS_READ ) )
			{
				trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s", QQ( N_("callvote: "
				                        "layout '$1$' could not be found on the server") ), Quote( arg ) ) );
				return;
			}

			Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ), "restart %s", Quote( arg ) );
			Com_sprintf( level.team[ team ].voteDisplayString,
			             sizeof( level.team[ team ].voteDisplayString ), "Change to map layout '%s'", arg );
		}
		break;

	case VOTE_NEXT_MAP:
		if ( G_MapExists( g_nextMap.string ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %s", QQ( N_("$1$: the next map is already set to '$2$'") ),
			                            cmd, Quote( g_nextMap.string ) ) );
			return;
		}

		if ( !G_MapExists( arg ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %s", QQ( N_("$1$: 'maps/$2$.bsp' could not be found on the server") ),
			                            cmd, Quote( arg ) ) );
			return;
		}

		if ( *reason ) // layout?
		{
			Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
			             "set g_nextMap %s; set g_nextMapLayouts %s", Quote( arg ), Quote( reason ) );
			Com_sprintf( level.team[ team ].voteDisplayString,
			             sizeof( level.team[ team ].voteDisplayString ),
			             "Set the next map to '%s' layout '%s'", arg, reason );
		}
		else
		{
			Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
			             "set g_nextMap %s; set g_nextMapLayouts \"\"", Quote( arg ) );
			Com_sprintf( level.team[ team ].voteDisplayString,
			             sizeof( level.team[ team ].voteDisplayString ),
			             "Set the next map to '%s'", arg );
		}

		reason[0] = '\0'; // nullify since we've used it here...
		break;

	case VOTE_POLL:
		level.team[ team ].voteString[0] = '\0';
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             "(poll) %s", reason );
		reason[0] = '\0'; // nullify since we've used it here...
		break;

	//default:;
	}

	// Append the vote reason (if applicable)
	if ( reason[ 0 ] )
	{
		Q_strcat( level.team[ team ].voteDisplayString,
		          sizeof( level.team[ team ].voteDisplayString ), va( " for '%s'", reason ) );
	}

	G_LogPrintf( "%s: %d \"%s^*\": %s",
	             team == TEAM_NONE ? "CallVote" : "CallTeamVote",
	             ( int )( ent - g_entities ), ent->client->pers.netname, level.team[ team ].voteString );

	if ( team == TEAM_NONE )
	{
		trap_SendServerCommand( -1, va( "print_tr %s %s %s", QQ( N_("$1$^* called a vote: $2$") ),
		                                Quote( ent->client->pers.netname ), Quote( level.team[ team ].voteDisplayString ) ) );
	}
	else
	{
		int i;

		for ( i = 0; i < level.maxclients; i++ )
		{
			if ( level.clients[ i ].pers.connected == CON_CONNECTED )
			{
				if ( level.clients[ i ].pers.team == team ||
				     ( level.clients[ i ].pers.team == TEAM_NONE &&
				       G_admin_permission( &g_entities[ i ], ADMF_SPEC_ALLCHAT ) ) )
				{
					trap_SendServerCommand( i, va( "print_tr %s %s %s", QQ( N_("$1$^* called a team vote: $2t$") ),
					                               Quote( ent->client->pers.netname ), Quote( level.team[ team ].voteDisplayString ) ) );
				}
				else if ( G_admin_permission( &g_entities[ i ], ADMF_ADMINCHAT ) )
				{
					trap_SendServerCommand( i, va( "chat -1 %d ^3%s\"^3 called a team vote (%ss): \"%s",
					                               SAY_ADMINS, Quote( ent->client->pers.netname ), BG_TeamName( team ),
					                               Quote( level.team[ team ].voteDisplayString ) ) );
				}
			}
		}
	}

	Color::StripColors( ent->client->pers.netname, caller, sizeof( caller ) );

	level.team[ team ].voteTime = level.time;
	trap_SetConfigstring( CS_VOTE_TIME + team,
	                      va( "%d", level.team[ team ].voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING + team,
	                      level.team[ team ].voteDisplayString );
	trap_SetConfigstring( CS_VOTE_CALLER + team,
	                      caller );

	if ( voteInfo[voteId].special != VOTE_NO_AUTO )
	{
		ent->client->pers.namelog->voteCount++;
		ent->client->pers.voteYes |= 1 << team;
		G_Vote( ent, team, true );
	}
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent )
{
	char   cmd[ MAX_TOKEN_CHARS ], vote[ MAX_TOKEN_CHARS ];
	team_t team = (team_t) ent->client->pers.team;

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( Q_stricmp( cmd, "teamvote" ) )
	{
		team = TEAM_NONE;
	}

	if ( !level.team[ team ].voteTime )
	{
		trap_SendServerCommand( ent - g_entities,
		                        va( "print_tr %s %s", QQ( N_("$1$: no vote in progress") ), cmd ) );
		return;
	}

	if ( ent->client->pers.voted & ( 1 << team ) )
	{
		trap_SendServerCommand( ent - g_entities,
		                        va( "print_tr %s %s", QQ( N_("$1$: vote already cast") ),  cmd ) );
		return;
	}

	trap_SendServerCommand( ent - g_entities,
	                        va( "print_tr %s %s", QQ( N_("$1$: vote cast") ), cmd ) );

	trap_Argv( 1, vote, sizeof( vote ) );

	switch ( vote[ 0 ] )
	{
	case 'y': case 'Y':
		ent->client->pers.voteYes |= 1 << team;
		break;

	case 'n': case 'N':
		ent->client->pers.voteNo |= 1 << team;
		break;
	}

	G_Vote( ent, team, true );
}

/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent )
{
	vec3_t origin, angles;
	char   buffer[ MAX_TOKEN_CHARS ];
	int    i, entityId;
	gentity_t* selection;

	if ( trap_Argc() < 4 && trap_Argc() != 2 )
	{
		trap_SendServerCommand( ent - g_entities, "print_tr \"" N_("usage: setviewpos (<x> <y> <z> [<yaw> [<pitch>]] | <entitynum>)") "\"" );
		return;
	}

	if(trap_Argc() == 2)
	{
		trap_Argv( 1, buffer, sizeof( buffer ) );
		entityId = atoi( buffer );

		if (entityId >= level.num_entities || entityId < MAX_CLIENTS)
		{
			Log::Warn("entityId %d is out of range", entityId);
			return;
		}
		selection = &g_entities[entityId];
		if (!selection->inuse)
		{
			Log::Warn("entity slot %d is not in use", entityId);
			return;
		}

		VectorCopy( selection->s.origin, origin );
		VectorCopy( selection->s.angles, angles );
	}
	else
	{
		for ( i = 0; i < 3; i++ )
		{
			trap_Argv( i + 1, buffer, sizeof( buffer ) );
			origin[ i ] = atof( buffer );
		}
		origin[ 2 ] -= ent->client->ps.viewheight;
		VectorCopy( ent->client->ps.viewangles, angles );
		angles[ ROLL ] = 0;

		if ( trap_Argc() >= 5 )
		{
			trap_Argv( 4, buffer, sizeof( buffer ) );
			angles[ YAW ] = atof( buffer );
			if ( trap_Argc() >= 6 )
			{
				trap_Argv( 5, buffer, sizeof( buffer ) );
				angles[ PITCH ] = atof( buffer );
			}
		}
	}

	G_TeleportPlayer( ent, origin, angles, 0.0f );
}

#define AS_OVER_RT3 (( ALIENSENSE_RANGE * 0.5f ) / M_ROOT3 )

bool G_RoomForClassChange( gentity_t *ent, class_t pcl, vec3_t newOrigin )
{
	vec3_t  fromMins, fromMaxs;
	vec3_t  toMins, toMaxs;
	vec3_t  temp;
	trace_t tr;
	float   nudgeHeight;
	float   maxHorizGrowth;
	class_t oldClass = (class_t) ent->client->ps.stats[ STAT_CLASS ];

	BG_ClassBoundingBox( oldClass, fromMins, fromMaxs, nullptr, nullptr, nullptr );
	BG_ClassBoundingBox( pcl, toMins, toMaxs, nullptr, nullptr, nullptr );

	VectorCopy( ent->client->ps.origin, newOrigin );

	// find max x/y diff
	maxHorizGrowth = toMaxs[ 0 ] - fromMaxs[ 0 ];

	if ( toMaxs[ 1 ] - fromMaxs[ 1 ] > maxHorizGrowth )
	{
		maxHorizGrowth = toMaxs[ 1 ] - fromMaxs[ 1 ];
	}

	if ( toMins[ 0 ] - fromMins[ 0 ] > -maxHorizGrowth )
	{
		maxHorizGrowth = - ( toMins[ 0 ] - fromMins[ 0 ] );
	}

	if ( toMins[ 1 ] - fromMins[ 1 ] > -maxHorizGrowth )
	{
		maxHorizGrowth = - ( toMins[ 1 ] - fromMins[ 1 ] );
	}

	if ( maxHorizGrowth > 0.0f )
	{
		// test by moving the player up the max required on a 60 degree slope
		nudgeHeight = maxHorizGrowth * 2.0f;
	}
	else
	{
		// player is shrinking, so there's no need to nudge them upwards
		nudgeHeight = 0.0f;
	}

	// find what the new origin would be on a level surface
	newOrigin[ 2 ] -= toMins[ 2 ] - fromMins[ 2 ];

	if ( ent->client->noclip )
	{
		return true;
	}

	//compute a place up in the air to start the real trace
	VectorCopy( newOrigin, temp );
	temp[ 2 ] += nudgeHeight;
	trap_Trace( &tr, newOrigin, toMins, toMaxs, temp, ent->s.number, MASK_PLAYERSOLID, 0 );

	//trace down to the ground so that we can evolve on slopes
	VectorCopy( newOrigin, temp );
	temp[ 2 ] += ( nudgeHeight * tr.fraction );
	trap_Trace( &tr, temp, toMins, toMaxs, newOrigin, ent->s.number, MASK_PLAYERSOLID, 0 );
	VectorCopy( tr.endpos, newOrigin );

	//make REALLY sure
	trap_Trace( &tr, newOrigin, toMins, toMaxs, newOrigin, ent->s.number, MASK_PLAYERSOLID, 0 );

	//check there is room to evolve
	return ( !tr.startsolid && tr.fraction == 1.0f );
}

/*
=================
Cmd_Class_f
=================
*/
static bool Cmd_Class_internal( gentity_t *ent, const char *s, bool report )
{
	int       clientNum;
	int       i;
	vec3_t    infestOrigin;
	class_t   currentClass = ent->client->pers.classSelection;
	class_t   newClass;
	int       entityList[ MAX_GENTITIES ];
	vec3_t    range = { AS_OVER_RT3, AS_OVER_RT3, AS_OVER_RT3 };
	vec3_t    mins, maxs;
	int       num;
	gentity_t *other;
	int       oldBoostTime = -1;
	vec3_t    oldVel;

	clientNum = ent->client - level.clients;
	newClass = BG_ClassByName( s )->number;

	if ( ent->client->sess.spectatorState != SPECTATOR_NOT )
	{
		team_t team;
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
		{
			G_StopFollowing( ent );
		}

		team = (team_t) ent->client->pers.team;
		if ( team == TEAM_ALIENS )
		{
			if ( newClass != PCL_ALIEN_BUILDER0 &&
			     newClass != PCL_ALIEN_BUILDER0_UPG &&
			     newClass != PCL_ALIEN_LEVEL0 )
			{
				if ( report )
				{
					G_TriggerMenuArgs( ent->client->ps.clientNum, MN_A_CLASSNOTSPAWN, newClass );
				}
				return false;
			}

			if ( BG_ClassDisabled( newClass ) )
			{
				if ( report )
				{
					G_TriggerMenuArgs( ent->client->ps.clientNum, MN_A_CLASSNOTALLOWED, newClass );
				}
				return false;
			}

			if ( !BG_ClassUnlocked( newClass ) )
			{
				if ( report )
				{
					G_TriggerMenuArgs( ent->client->ps.clientNum, MN_A_CLASSLOCKED, newClass );
				}
				return false;
			}

			// spawn from an egg
			//TODO merge with human's code
			if ( G_PushSpawnQueue( &level.team[ team ].spawnQueue, clientNum ) )
			{
				ent->client->pers.classSelection = newClass;
				ent->client->ps.stats[ STAT_CLASS ] = newClass;

				return true;
			}
		}
		else if ( team == TEAM_HUMANS )
		{
			//set the item to spawn with
			if ( !Q_stricmp( s, BG_Weapon( WP_MACHINEGUN )->name ) &&
			     !BG_WeaponDisabled( WP_MACHINEGUN ) )
			{
				ent->client->pers.humanItemSelection = WP_MACHINEGUN;
			}
			else if ( !Q_stricmp( s, BG_Weapon( WP_HBUILD )->name ) &&
			          !BG_WeaponDisabled( WP_HBUILD ) )
			{
				ent->client->pers.humanItemSelection = WP_HBUILD;
			}
			else
			{
				if ( report )
				{
					G_TriggerMenu( ent->client->ps.clientNum, MN_H_UNKNOWNSPAWNITEM );
				}
				return false;
			}

			// spawn from a telenode
			//TODO merge with alien's code
			newClass = PCL_HUMAN_NAKED;
			if ( G_PushSpawnQueue( &level.team[ team ].spawnQueue, clientNum ) )
			{
				ent->client->pers.classSelection = newClass;
				ent->client->ps.stats[ STAT_CLASS ] = newClass;

				return true;
			}
		}

		return false;
	}

	if ( Entities::IsDead( ent ) )
	{
		return true; // dead, can't evolve; no point in trying other classes (if any listed)
	}

	if ( ent->client->pers.team == TEAM_ALIENS )
	{
		if ( newClass == PCL_NONE )
		{
			if ( report )
			{
				G_TriggerMenu( ent->client->ps.clientNum, MN_A_UNKNOWNCLASS );
			}
			return false;
		}

		//if we are not currently spectating, we are attempting evolution
		if ( ent->client->pers.classSelection != PCL_NONE )
		{
			int cost;

			//check that we have an overmind
			if ( !G_ActiveOvermind() )
			{
				if ( report )
				{
					G_TriggerMenu( clientNum, MN_A_NOOVMND_EVOLVE );
				}
				return false;
			}

			//check there are no humans nearby
			VectorAdd( ent->client->ps.origin, range, maxs );
			VectorSubtract( ent->client->ps.origin, range, mins );

			num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

			int alienBuildingsInRange = 0;
			int humansInRange = 0;
			bool visibleToHumans = false;

			for ( i = 0; i < num; i++ )
			{
				other = &g_entities[ entityList[ i ] ];

				if ( ( other->client && other->client->pers.team == TEAM_HUMANS ) ||
				     ( other->s.eType == entityType_t::ET_BUILDABLE && other->buildableTeam == TEAM_HUMANS &&
				       other->powered ) )
				{
					humansInRange++;
					if( !visibleToHumans && G_LineOfSight( ent, other ) )
					{
						visibleToHumans = true;
					}

				}
				else if ( ( other->s.eType == entityType_t::ET_BUILDABLE && other->buildableTeam == TEAM_ALIENS ) )
				{
					alienBuildingsInRange++;
				}
			}
			if ( visibleToHumans && alienBuildingsInRange < ( humansInRange * g_evolveAroundHumans.Get() ) ) {
				if ( report )
				{
					G_TriggerMenu( clientNum, MN_A_TOOCLOSE );
				}
				return false;
			}


			//check that we are not wallwalking
			if ( ent->client->ps.eFlags & EF_WALLCLIMB )
			{
				if ( report )
				{
					G_TriggerMenu( clientNum, MN_A_EVOLVEWALLWALK );
				}
				return false;
			}

			if ( ent->client->sess.spectatorState == SPECTATOR_NOT &&
			     ( currentClass == PCL_ALIEN_BUILDER0 ||
			       currentClass == PCL_ALIEN_BUILDER0_UPG ) &&
			     ent->client->ps.stats[ STAT_MISC ] > 0 )
			{
				if ( report )
				{
					G_TriggerMenu( ent->client->ps.clientNum, MN_A_EVOLVEBUILDTIMER );
				}
				return false;
			}

			cost = BG_ClassCanEvolveFromTo( currentClass, newClass, ent->client->pers.credit );

			if ( G_RoomForClassChange( ent, newClass, infestOrigin ) )
			{
				if ( cost != CANT_EVOLVE )
				{

					if ( ( cost < 0 || ( cost == 0 && currentClass == PCL_ALIEN_LEVEL0 ) )	&& ( G_DistanceToBase( ent ) >= g_devolveMaxBaseDistance.Get() ) ) {
						if ( report )
						{
							G_TriggerMenu( clientNum, MN_A_NOTINBASE );
						}
						return false;
					}

					ent->client->pers.evolveHealthFraction = ent->entity->Get<HealthComponent>()->HealthFraction();

					if ( ent->client->pers.evolveHealthFraction < 0.0f )
					{
						ent->client->pers.evolveHealthFraction = 0.0f;
					}
					else if ( ent->client->pers.evolveHealthFraction > 1.0f )
					{
						ent->client->pers.evolveHealthFraction = 1.0f;
					}

					if ( cost < 0 ){
						 cost = cost * ent->client->pers.evolveHealthFraction * DEVOLVE_RETURN_RATE;
					}
					//remove credit
					G_AddCreditToClient( ent->client, -cost, true );
					ent->client->pers.classSelection = newClass;
					ClientUserinfoChanged( clientNum, false );
					VectorCopy( infestOrigin, ent->s.pos.trBase );
					VectorCopy( ent->client->ps.velocity, oldVel );

					if ( ent->client->ps.stats[ STAT_STATE ] & SS_BOOSTED )
					{
						oldBoostTime = ent->client->boostedTime;
					}

					ClientSpawn( ent, ent, ent->s.pos.trBase, ent->s.apos.trBase );

					VectorCopy( oldVel, ent->client->ps.velocity );

					if ( oldBoostTime > 0 )
					{
						ent->client->boostedTime = oldBoostTime;
						ent->client->ps.stats[ STAT_STATE ] |= SS_BOOSTED;
					}
				}
				else
				{
					if ( report )
					{
						G_TriggerMenuArgs( clientNum, MN_A_CANTEVOLVE, newClass );
					}
					return false;
				}
			}
			else
			{
				if ( report )
				{
					G_TriggerMenu( clientNum, MN_A_NOEROOM );
				}
				return false;
			}
		}
	}
	else if ( ent->client->pers.team == TEAM_HUMANS )
	{
		if ( report )
		{
			G_TriggerMenu( clientNum, MN_H_DEADTOCLASS );
		}
		return false;
	}

	// if we reach this, found a valid class and changed to it
	return true;
}

void Cmd_Class_f( gentity_t *ent )
{
	char s[ MAX_TOKEN_CHARS ];
	int  i;
	int  args = trap_Argc() - 1;

	for ( i = 1; i <= args; ++i )
	{
		trap_Argv( i, s, sizeof( s ) );

		if ( Cmd_Class_internal( ent, s, i == args ) ) break;
	}
}

void Cmd_Deconstruct_f( gentity_t *ent )
{
	char      arg[ 32 ];
	vec3_t    viewOrigin, forward, end;
	trace_t   trace;
	gentity_t *buildable;
	bool  instant;

	// check for revoked building rights
	if ( ent->client->pers.namelog->denyBuild )
	{
		G_TriggerMenu( ent->client->ps.clientNum, MN_B_REVOKED );
		return;
	}

	// trace for target
	BG_GetClientViewOrigin( &ent->client->ps, viewOrigin );
	AngleVectors( ent->client->ps.viewangles, forward, nullptr, nullptr );
	VectorMA( viewOrigin, 100, forward, end );
	trap_Trace( &trace, viewOrigin, nullptr, nullptr, end, ent->s.number, MASK_PLAYERSOLID, 0 );
	buildable = &g_entities[ trace.entityNum ];

	// check if target is valid
	if ( trace.fraction >= 1.0f ||
	     buildable->s.eType != entityType_t::ET_BUILDABLE ||
	     !G_OnSameTeam( ent, buildable ) )
	{
		return;
	}

	// check for valid build weapon
	switch ( ent->client->ps.weapon )
	{
		case WP_HBUILD:
		case WP_ABUILD:
		case WP_ABUILD2:
			break;

		default:
			return;
	}

	// always let the builder prevent the explosion of a buildable
	if ( Entities::IsDead( buildable ) )
	{
		G_RewardAttackers( buildable );
		G_FreeEntity( buildable );
		return;
	}

	// check for instant mode
	if ( trap_Argc() == 2 )
	{
		trap_Argv( 1, arg, sizeof( arg ) );
		instant = !Q_stricmp( arg, "marked" );
	}
	else
	{
		instant = false;
	}

	if ( instant && buildable->entity->Get<BuildableComponent>()->MarkedForDeconstruction() )
	{
		if ( !g_cheats.integer )
		{
			// check if the buildable is protected from instant deconstruction
			switch ( buildable->s.modelindex )
			{
				case BA_A_OVERMIND:
				case BA_H_REACTOR:
					G_TriggerMenu( ent->client->ps.clientNum, MN_B_MAINSTRUCTURE );
					return;

				case BA_A_SPAWN:
				case BA_H_SPAWN:
					if ( level.team[ ent->client->ps.persistant[ PERS_TEAM ] ].numSpawns <= 1 )
					{
						G_TriggerMenu( ent->client->ps.clientNum, MN_B_LASTSPAWN );
						return;
					}
					break;
			}

			// deny if build timer active
			if ( ent->client->ps.stats[ STAT_MISC ] > 0 )
			{
				G_AddEvent( ent, EV_BUILD_DELAY, ent->client->ps.clientNum );
				return;
			}

			// add to build timer
			ent->client->ps.stats[ STAT_MISC ] += BG_Buildable( buildable->s.modelindex )->buildTime / 4;
		}

		G_Deconstruct( buildable, ent, MOD_DECONSTRUCT );
	}
	else
	{
		// toggle mark
		buildable->entity->Get<BuildableComponent>()->ToggleDeconstructionMark();
	}
}

/*
=================
Cmd_Ignite_f
=================
*/
void Cmd_Ignite_f( gentity_t *player )
{
	vec3_t    viewOrigin, forward, end;
	trace_t   trace;

	BG_GetClientViewOrigin( &player->client->ps, viewOrigin );
	AngleVectors( player->client->ps.viewangles, forward, nullptr, nullptr );
	VectorMA( viewOrigin, 1000, forward, end );
	trap_Trace( &trace, viewOrigin, nullptr, nullptr, end, player->s.number, MASK_PLAYERSOLID, 0 );

	if ( trace.entityNum == ENTITYNUM_WORLD ) {
		G_SpawnFire( trace.endpos, trace.plane.normal, player );
	} else {
		g_entities[ trace.entityNum ].entity->Ignite( player );
	}

}

/*
=================
Cmd_ActivateItem_f

Activate an item
=================
*/
void Cmd_ActivateItem_f( gentity_t *ent )
{
	char s[ MAX_TOKEN_CHARS ];
	int  upgrade, weapon;

	trap_Argv( 1, s, sizeof( s ) );

	// "weapon" aliased to whatever weapon you have
	if ( !Q_stricmp( "weapon", s ) )
	{
		if ( ent->client->ps.weapon == WP_BLASTER &&
		     BG_PlayerCanChangeWeapon( &ent->client->ps ) )
		{
			G_ForceWeaponChange( ent, WP_NONE );
		}

		return;
	}

	// "grenade" aliased to whatever's in the grenade slot
	if ( !Q_stricmp( "grenade", s ) )
	{
		for ( upgrade = UP_NUM_UPGRADES - 1; upgrade > UP_NONE; --upgrade )
		{
			const upgradeAttributes_t *upg = BG_Upgrade( upgrade );

			if ( ( upg->slots & SLOT_GRENADE ) && BG_InventoryContainsUpgrade( upgrade, ent->client->ps.stats ) )
			{
				break;
			}
		}
	}
	else
	{
		upgrade = BG_UpgradeByName( s )->number;
	}

	weapon = BG_WeaponNumberByName( s );

	if ( upgrade != UP_NONE && BG_InventoryContainsUpgrade( upgrade, ent->client->ps.stats ) )
	{
		BG_ActivateUpgrade( upgrade, ent->client->ps.stats );
	}
	else if ( weapon != WP_NONE &&
	          BG_InventoryContainsWeapon( weapon, ent->client->ps.stats ) )
	{
		if ( ent->client->ps.weapon != weapon &&
		     BG_PlayerCanChangeWeapon( &ent->client->ps ) )
		{
			G_ForceWeaponChange( ent, (weapon_t) weapon );
		}
	}
	else
	{
		trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s", QQ( N_("You don't have the $1$") ), Quote( s ) ) );
	}
}

/*
=================
Cmd_DeActivateItem_f

Deactivate an item
=================
*/
void Cmd_DeActivateItem_f( gentity_t *ent )
{
	char      s[ MAX_TOKEN_CHARS ];
	upgrade_t upgrade;

	trap_Argv( 1, s, sizeof( s ) );
	upgrade = BG_UpgradeByName( s )->number;

	if ( BG_InventoryContainsUpgrade( upgrade, ent->client->ps.stats ) )
	{
		BG_DeactivateUpgrade( upgrade, ent->client->ps.stats );
	}
	else
	{
		trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s", QQ( N_("You don't have the $1$") ), Quote( s ) ) );
	}
}

/*
=================
Cmd_ToggleItem_f
=================
*/
void Cmd_ToggleItem_f( gentity_t *ent )
{
	char      s[ MAX_TOKEN_CHARS ];
	weapon_t  weapon;
	upgrade_t upgrade;

	trap_Argv( 1, s, sizeof( s ) );

	upgrade = BG_UpgradeByName( s )->number;
	weapon  = BG_WeaponNumberByName( s );

	if ( weapon != WP_NONE )
	{
		if ( !BG_PlayerCanChangeWeapon( &ent->client->ps ) )
		{
			return;
		}

		//special case to allow switching between
		//the blaster and the primary weapon
		if ( ent->client->ps.weapon != WP_BLASTER )
		{
			weapon = WP_BLASTER;
		}
		else
		{
			weapon = WP_NONE;
		}

		G_ForceWeaponChange( ent, weapon );
	}
	else if ( BG_InventoryContainsUpgrade( upgrade, ent->client->ps.stats ) )
	{
		if ( BG_UpgradeIsActive( upgrade, ent->client->ps.stats ) )
		{
			BG_DeactivateUpgrade( upgrade, ent->client->ps.stats );
		}
		else
		{
			BG_ActivateUpgrade( upgrade, ent->client->ps.stats );
		}
	}
	else
	{
		trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s", QQ( N_("You don't have the $1$") ), Quote( s ) ) );
	}
}

/*
=================
Cmd_Sell_f
=================
*/
static bool Cmd_Sell_weapons( gentity_t *ent )
{
	int      i;
	weapon_t selected = BG_GetPlayerWeapon( &ent->client->ps );
	bool sold = false;

	if ( !BG_PlayerCanChangeWeapon( &ent->client->ps ) )
	{
		return false;
	}

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		// guard against selling the HBUILD weapons exploit
		if ( i == WP_HBUILD && ent->client->ps.stats[ STAT_MISC ] > 0 )
		{
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_ARMOURYBUILDTIMER );
			continue;
		}

		if ( BG_InventoryContainsWeapon( i, ent->client->ps.stats ) &&
		     BG_Weapon( i )->purchasable )
		{
			ent->client->ps.stats[ STAT_WEAPON ] = WP_NONE;

			// add to funds
			G_AddCreditToClient( ent->client, ( short ) BG_Weapon( i )->price, false );

			sold = true;
		}

		// if we have this weapon selected, force a new selection
		if ( i == selected )
		{
			G_ForceWeaponChange( ent, WP_NONE );
		}
	}

	return sold;
}

static bool Cmd_Sell_upgradeItem( gentity_t *ent, upgrade_t item )
{
	// check if carried and sellable
	if ( !BG_InventoryContainsUpgrade( item, ent->client->ps.stats ) ||
	     !BG_Upgrade( item )->purchasable )
	{
		return false;
	}

	// shouldn't really need to test for this, but just to be safe
	if ( item == UP_LIGHTARMOUR || item == UP_MEDIUMARMOUR || item == UP_BATTLESUIT )
	{
		vec3_t newOrigin;

		if ( !G_RoomForClassChange( ent, PCL_HUMAN_NAKED, newOrigin ) )
		{
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOROOMARMOURCHANGE );
			return false;
		}

		VectorCopy( newOrigin, ent->client->ps.origin );
		ent->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN_NAKED;
		ent->client->pers.classSelection = PCL_HUMAN_NAKED;
		ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
	}

	// remove from inventory
	BG_RemoveUpgradeFromInventory( item, ent->client->ps.stats );

	// add to funds
	G_AddCreditToClient( ent->client, ( short ) BG_Upgrade( item )->price, false );

	return true;
}

static bool Cmd_Sell_upgrades( gentity_t *ent )
{
	int      i;
	bool sold = false;

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		sold |= Cmd_Sell_upgradeItem( ent, (upgrade_t) i );
	}

	return sold;
}

static bool Cmd_Sell_armour( gentity_t *ent )
{
	return Cmd_Sell_upgradeItem( ent, UP_LIGHTARMOUR ) |
	       Cmd_Sell_upgradeItem( ent, UP_MEDIUMARMOUR ) |
	       Cmd_Sell_upgradeItem( ent, UP_BATTLESUIT ) |
	       Cmd_Sell_upgradeItem( ent, UP_RADAR );
}

static bool Cmd_Sell_internal( gentity_t *ent, const char *s )
{
	weapon_t  weapon;
	upgrade_t upgrade;

	//no armoury nearby
	if ( !G_BuildableInRange( ent->client->ps.origin, ENTITY_BUY_RANGE, BA_H_ARMOURY ) )
	{
		G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOARMOURYHERE );
		return false;
	}

	if ( !Q_strnicmp( s, "weapon", 6 ) )
	{
		weapon = (weapon_t) ent->client->ps.stats[ STAT_WEAPON ];
	}
	else
	{
		weapon = BG_WeaponNumberByName( s );
	}

	upgrade = BG_UpgradeByName( s )->number;

	if ( weapon != WP_NONE )
	{
		weapon_t selected = BG_GetPlayerWeapon( &ent->client->ps );

		if ( !BG_PlayerCanChangeWeapon( &ent->client->ps ) )
		{
			return false;
		}

		//are we /allowed/ to sell this?
		if ( !BG_Weapon( weapon )->purchasable )
		{
			trap_SendServerCommand( ent - g_entities, "print_tr \"" N_("You can't sell this weapon") "\"" );
			return false;
		}

		//remove weapon if carried
		if ( BG_InventoryContainsWeapon( weapon, ent->client->ps.stats ) )
		{
			//guard against selling the HBUILD weapons exploit
			if ( weapon == WP_HBUILD && ent->client->ps.stats[ STAT_MISC ] > 0 )
			{
				G_TriggerMenu( ent->client->ps.clientNum, MN_H_ARMOURYBUILDTIMER );
				return false;
			}

			ent->client->ps.stats[ STAT_WEAPON ] = WP_NONE;
			// Cancel ghost buildables
			ent->client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;

			//add to funds
			G_AddCreditToClient( ent->client, ( short ) BG_Weapon( weapon )->price, false );
		}

		//if we have this weapon selected, force a new selection
		if ( weapon == selected )
		{
			G_ForceWeaponChange( ent, WP_NONE );
		}
	}
	else if ( upgrade != UP_NONE )
	{
		//are we /allowed/ to sell this?
		if ( !BG_Upgrade( upgrade )->purchasable )
		{
			trap_SendServerCommand( ent - g_entities, "print_tr \"" N_("You can't sell this item") "\"" );
			return false;
		}

		return Cmd_Sell_upgradeItem( ent, upgrade );
	}
	else if ( !Q_stricmp( s, "weapons" ) )
	{
		return Cmd_Sell_weapons( ent );
	}
	else if ( !Q_stricmp( s, "upgrades" ) )
	{
		return Cmd_Sell_upgrades( ent );
	}
	else if ( !Q_stricmp( s, "armour" ) || !Q_stricmp( s, "armor" ) )
	{
		return Cmd_Sell_armour( ent );
	}
	else if ( !Q_stricmp( s, "all" ) )
	{
		return Cmd_Sell_weapons( ent ) | Cmd_Sell_upgrades( ent );
	}
	else
	{
		G_TriggerMenu( ent->client->ps.clientNum, MN_H_UNKNOWNITEM );
	}

	return false;
}

void Cmd_Sell_f( gentity_t *ent )
{
	char     s[ MAX_TOKEN_CHARS ];
	int      c, args;
	bool updated = false;

	args = trap_Argc();

	for ( c = 1; c < args; ++c )
	{
		trap_Argv( c, s, sizeof( s ) );
		updated |= Cmd_Sell_internal( ent, s );
	}

	//update ClientInfo
	if ( updated )
	{
		ClientUserinfoChanged( ent->client->ps.clientNum, false );
		ent->client->pers.infoChangeTime = level.time;
	}
}

/*
=================
Cmd_Buy_f
=================
*/
static bool Cmd_Sell_conflictingUpgrades( gentity_t *ent, upgrade_t upgrade )
{
	const int  slots = BG_Upgrade( upgrade )->slots;
	int        i;
	bool   sold = false;
	int *const stats = ent->client->ps.stats;

	for ( i = UP_NONE; i < UP_NUM_UPGRADES; i++ )
	{
		if ( i != upgrade && BG_InventoryContainsUpgrade( i, stats ) )
		{
			int slot = BG_Upgrade( i )->slots;

			if ( slots & slot )
			{
				sold |= Cmd_Sell_upgradeItem( ent, (upgrade_t) i );
			}
		}
	}

	return sold;
}


static bool Cmd_Buy_internal( gentity_t *ent, const char *s, bool sellConflicting, bool quiet )
{
#define Maybe_TriggerMenu(num, reason) do { if ( !quiet ) G_TriggerMenu( (num), (reason) ); } while ( 0 )
	weapon_t  weapon;
	upgrade_t upgrade;
	vec3_t    newOrigin;

	weapon = BG_WeaponNumberByName( s );
	upgrade = BG_UpgradeByName( s )->number;

	// check if armoury is in reach
	if ( !G_BuildableInRange( ent->client->ps.origin, ENTITY_BUY_RANGE, BA_H_ARMOURY ) )
	{
		G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOARMOURYHERE );

		return false;
	}

	if ( weapon != WP_NONE )
	{
		//already got this?
		if ( BG_InventoryContainsWeapon( weapon, ent->client->ps.stats ) )
		{
			Maybe_TriggerMenu( ent->client->ps.clientNum, MN_H_ITEMHELD );
			return false;
		}

		// Only humans can buy stuff
		if ( BG_Weapon( weapon )->team != TEAM_HUMANS )
		{
			goto not_alien;
		}

		//are we /allowed/ to buy this?
		if ( !BG_Weapon( weapon )->purchasable )
		{
			goto cant_buy;
		}

		//are we /allowed/ to buy this?
		if ( !BG_WeaponUnlocked( weapon ) || BG_WeaponDisabled( weapon ) )
		{
			goto cant_buy;
		}

		// In some instances, weapons can't be changed
		if ( !BG_PlayerCanChangeWeapon( &ent->client->ps ) )
		{
			return false;
		}

		for (;;)
		{
			//can afford this?
			if ( BG_Weapon( weapon )->price > ( short ) ent->client->pers.credit )
			{
				// if requested, try to sell then recheck
				if ( sellConflicting && Cmd_Sell_weapons( ent ) )
				{
					continue;
				}

				Maybe_TriggerMenu( ent->client->ps.clientNum, MN_H_NOFUNDS );
				return false;
			}

			//have space to carry this?
			if ( BG_Weapon( weapon )->slots & BG_SlotsForInventory( ent->client->ps.stats ) )
			{
				// if requested, try to sell then recheck
				if ( sellConflicting && Cmd_Sell_weapons( ent ) )
				{
					continue;
				}

				Maybe_TriggerMenu( ent->client->ps.clientNum, MN_H_NOSLOTS );
				return false;
			}

			break; // okay, can buy this
		}

		ent->client->ps.stats[ STAT_WEAPON ] = weapon;
		G_GiveMaxAmmo( ent );
		G_ForceWeaponChange( ent, weapon );

		//set build delay/pounce etc to 0
		ent->client->ps.stats[ STAT_MISC ] = 0;

		//subtract from funds
		G_AddCreditToClient( ent->client, - ( short ) BG_Weapon( weapon )->price, false );

		return true;
	}
	else if ( upgrade != UP_NONE )
	{
		//already got this?
		if ( BG_InventoryContainsUpgrade( upgrade, ent->client->ps.stats ) )
		{
			Maybe_TriggerMenu( ent->client->ps.clientNum, MN_H_ITEMHELD );
			return false;
		}

		// Only humans can buy stuff
		if ( BG_Upgrade( upgrade )->team != TEAM_HUMANS )
		{
			goto not_alien;
		}

		//are we /allowed/ to buy this?
		if ( !BG_Upgrade( upgrade )->purchasable )
		{
			goto cant_buy;
		}

		//are we /allowed/ to buy this?
		if ( !BG_UpgradeUnlocked( upgrade ) || BG_UpgradeDisabled( upgrade ) )
		{
			goto cant_buy;
		}

		for (;;)
		{
			//can afford this?
			if ( BG_Upgrade( upgrade )->price > ( short ) ent->client->pers.credit )
			{
				// if requested, try to sell then recheck
				if ( sellConflicting && Cmd_Sell_conflictingUpgrades( ent, upgrade ) )
				{
					continue;
				}

				Maybe_TriggerMenu( ent->client->ps.clientNum, MN_H_NOFUNDS );
				return false;
			}

			//have space to carry this?
			if ( BG_Upgrade( upgrade )->slots & BG_SlotsForInventory( ent->client->ps.stats ) )
			{
				// if requested, try to sell then recheck
				if ( sellConflicting && Cmd_Sell_conflictingUpgrades( ent, upgrade ) )
				{
					continue;
				}

				Maybe_TriggerMenu( ent->client->ps.clientNum, MN_H_NOSLOTS );
				return false;
			}

			break; // okay, can buy this
		}

		if ( upgrade == UP_LIGHTARMOUR )
		{
			if ( !G_RoomForClassChange( ent, PCL_HUMAN_LIGHT, newOrigin ) )
			{
				G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOROOMARMOURCHANGE );
				return false;
			}

			VectorCopy( newOrigin, ent->client->ps.origin );
			ent->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN_LIGHT;
			ent->client->pers.classSelection = PCL_HUMAN_LIGHT;
			ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
		}
		else if ( upgrade == UP_MEDIUMARMOUR )
		{
			if ( !G_RoomForClassChange( ent, PCL_HUMAN_MEDIUM, newOrigin ) )
			{
				G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOROOMARMOURCHANGE );
				return false;
			}

			VectorCopy( newOrigin, ent->client->ps.origin );
			ent->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN_MEDIUM;
			ent->client->pers.classSelection = PCL_HUMAN_MEDIUM;
			ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
		}
		else if ( upgrade == UP_BATTLESUIT )
		{
			if ( !G_RoomForClassChange( ent, PCL_HUMAN_BSUIT, newOrigin ) )
			{
				G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOROOMARMOURCHANGE );
				return false;
			}

			VectorCopy( newOrigin, ent->client->ps.origin );
			ent->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN_BSUIT;
			ent->client->pers.classSelection = PCL_HUMAN_BSUIT;
			ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
		}

		//add to inventory
		BG_AddUpgradeToInventory( upgrade, ent->client->ps.stats );

		//subtract from funds
		G_AddCreditToClient( ent->client, - ( short ) BG_Upgrade( upgrade )->price, false );

		return true;
	}
	else
	{
		G_TriggerMenu( ent->client->ps.clientNum, MN_H_UNKNOWNITEM );
	}

	return false;

cant_buy:
	trap_SendServerCommand( ent - g_entities, va( "print_tr \"" N_("You can't buy this item ($1$)") "\" %s", Quote( s ) ) );
	return false;

not_alien:
	trap_SendServerCommand( ent - g_entities, "print_tr \"" N_("You can't buy alien items") "\"" );
	return false;
}

void Cmd_Buy_f( gentity_t *ent )
{
	char     s[ MAX_TOKEN_CHARS ];
	int      c, args;
	bool updated = false;

	args = trap_Argc();

	c = 1;
	trap_Argv( c, s, sizeof( s ) );

	for ( ; c < args; ++c )
	{
		trap_Argv( c, s, sizeof( s ) );

		switch ( s[0] )
		{
		case '-':
			updated |= Cmd_Sell_internal( ent, s + 1 );
			break;
		case '+':
			updated |= Cmd_Buy_internal( ent, s + 1, true, false ); // auto-sell if needed
			break;
		case '?':
			updated |= Cmd_Buy_internal( ent, s + 1, false, true ); // quiet mode
			break;
		default:
			updated |= Cmd_Buy_internal( ent, s, false, false );
			break;
		}
	}

	//update ClientInfo
	if ( updated )
	{
		ClientUserinfoChanged( ent->client->ps.clientNum, false );
		ent->client->pers.infoChangeTime = level.time;
	}
}

/*
=================
Cmd_Build_f
=================
*/
void Cmd_Build_f( gentity_t *ent )
{
	char        s[ MAX_TOKEN_CHARS ];
	buildable_t buildable;
	float       dist;
	vec3_t      origin, normal;
	int         groundEntNum;

	if ( ent->client->pers.namelog->denyBuild )
	{
		G_TriggerMenu( ent->client->ps.clientNum, MN_B_REVOKED );
		return;
	}

	if ( ent->client->pers.team == level.surrenderTeam )
	{
		G_TriggerMenu( ent->client->ps.clientNum, MN_B_SURRENDER );
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	buildable = BG_BuildableByName( s )->number;

	if ( buildable != BA_NONE &&
	     ( ( 1 << ent->client->ps.weapon ) & BG_Buildable( buildable )->buildWeapon ) &&
	     !BG_BuildableDisabled( buildable ) && BG_BuildableUnlocked( buildable ) )
	{
		dynMenu_t err;
		vec3_t forward, aimDir;
		itemBuildError_t reason;

		BG_GetClientNormal( &ent->client->ps, normal );
		AngleVectors( ent->client->ps.viewangles, aimDir, nullptr, nullptr );
		ProjectPointOnPlane( forward, aimDir, normal);
		VectorNormalize( forward );

		dist = BG_Class( ent->client->ps.stats[ STAT_CLASS ] )->buildDist * DotProduct( forward, aimDir );

		reason = G_CanBuild( ent, buildable, dist, origin, normal, &groundEntNum );
		ent->client->ps.stats[ STAT_BUILDABLE ] = BA_NONE | SB_BUILDABLE_FROM_IBE( reason );

		// these are the errors displayed when the builder first selects something to use
		switch ( reason )
		{
			// non-temporary issues won't spawn a blueprint buildable
			case IBE_DISABLED:
				err = MN_B_DISABLED;
				break;

			case IBE_ONEOVERMIND:
				err = MN_A_ONEOVERMIND;
				break;

			case IBE_ONEREACTOR:
				err = MN_H_ONEREACTOR;
				break;

			// otherwise we don't error for now
			default:
				err = MN_NONE;
				break;
		}

		if ( err == MN_NONE || ent->client->pers.disableBlueprintErrors )
		{
			ent->client->ps.stats[ STAT_BUILDABLE ] |= buildable;
		}
		else
		{
			G_TriggerMenu( ent->client->ps.clientNum, err );
		}
	}
	else
	{
		G_TriggerMenu( ent->client->ps.clientNum, MN_B_CANNOT );
	}
}

void Cmd_Reload_f( gentity_t *ent )
{
	playerState_t *ps;
	const weaponAttributes_t *wa;

	if ( !ent->client )
	{
		return;
	}

	ps = &ent->client->ps;
	wa = BG_Weapon( ps->weapon );

	// don't reload if the currently held weapon doesn't use ammo
	if ( BG_Weapon( ps->weapon )->infiniteAmmo )
	{
		return;
	}

	// can't reload if there is no clip
	if ( ps->clips <= 0 )
	{
		return;
	}

	// don't reload when full
	if ( ps->ammo >= wa->maxAmmo )
	{
		return;
	}

	// the actual reload process is handled synchronously in PM
	if ( ent->client->ps.weaponstate != WEAPON_RELOADING )
	{
		ent->client->ps.pm_flags |= PMF_WEAPON_RELOAD;
	}
}

/*
=================
G_StopFromFollowing

stops any other clients from following this one
called when a player leaves a team or dies
=================
*/
void G_StopFromFollowing( gentity_t *ent )
{
	int i;

	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].sess.spectatorState == SPECTATOR_FOLLOW &&
		     level.clients[ i ].sess.spectatorClient == ent->client->ps.clientNum )
		{
			if ( !G_FollowNewClient( &g_entities[ i ], 1 ) )
			{
				G_StopFollowing( &g_entities[ i ] );
			}
		}
	}
}

/*
=================
G_StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void G_StopFollowing( gentity_t *ent )
{
	ent->client->ps.persistant[ PERS_TEAM ] = ent->client->pers.team;

	if ( ent->client->pers.team == TEAM_NONE )
	{
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->ps.persistant[ PERS_SPECSTATE ] = SPECTATOR_FREE;
	}
	else
	{
		vec3_t spawn_origin, spawn_angles;

		ent->client->sess.spectatorState = SPECTATOR_LOCKED;
		ent->client->ps.persistant[ PERS_SPECSTATE ] = SPECTATOR_LOCKED;

		if ( ent->client->pers.team == TEAM_ALIENS )
		{
			G_SelectAlienLockSpawnPoint( spawn_origin, spawn_angles );
		}
		else if ( ent->client->pers.team == TEAM_HUMANS )
		{
			G_SelectHumanLockSpawnPoint( spawn_origin, spawn_angles );
		}
		else
		{
			G_SelectSpectatorSpawnPoint( spawn_origin, spawn_angles );
		}

		G_SetOrigin( ent, spawn_origin );
		VectorCopy( spawn_origin, ent->client->ps.origin );
		G_SetClientViewAngle( ent, spawn_angles );
	}

	ent->client->sess.spectatorClient = -1;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->client->ps.groundEntityNum = ENTITYNUM_NONE;
	ent->client->ps.stats[ STAT_STATE ] = 0;
	ent->client->ps.stats[ STAT_VIEWLOCK ] = 0;
	ent->client->ps.eFlags &= ~( EF_WALLCLIMB | EF_WALLCLIMBCEILING );
	ent->client->ps.clientNum = ent - g_entities;
	ent->client->ps.persistant[ PERS_CREDIT ] = ent->client->pers.credit;

	if ( ent->client->pers.team == TEAM_NONE )
	{
		vec3_t viewOrigin, angles;

		BG_GetClientViewOrigin( &ent->client->ps, viewOrigin );
		VectorCopy( ent->client->ps.viewangles, angles );
		angles[ ROLL ] = 0;
		G_TeleportPlayer( ent, viewOrigin, angles, false );
	}

	CalculateRanks();
}

/*
=================
G_FollowLockView

Client is still following a player, but that player has gone to spectator
mode and cannot be followed for the moment
=================
*/
void G_FollowLockView( gentity_t *ent )
{
	vec3_t spawn_origin, spawn_angles;
	int    clientNum;

	clientNum = ent->client->sess.spectatorClient;
	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->ps.persistant[ PERS_SPECSTATE ] = SPECTATOR_FOLLOW;
	ent->client->ps.clientNum = clientNum;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->client->ps.persistant[ PERS_TEAM ] = ent->client->pers.team;
	ent->client->ps.stats[ STAT_STATE ] &= ~SS_WALLCLIMBING;
	ent->client->ps.stats[ STAT_VIEWLOCK ] = 0;
	ent->client->ps.eFlags &= ~( EF_WALLCLIMB | EF_WALLCLIMBCEILING );
	ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
	ent->client->ps.viewangles[ PITCH ] = 0.0f;

	// Put the view at the team spectator lock position
	if ( level.clients[ clientNum ].pers.team == TEAM_ALIENS )
	{
		G_SelectAlienLockSpawnPoint( spawn_origin, spawn_angles );
	}
	else if ( level.clients[ clientNum ].pers.team == TEAM_HUMANS )
	{
		G_SelectHumanLockSpawnPoint( spawn_origin, spawn_angles );
	}
	else
	{
		G_SelectSpectatorSpawnPoint( spawn_origin, spawn_angles );
	}

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, ent->client->ps.origin );
	G_SetClientViewAngle( ent, spawn_angles );
}

/*
=================
G_FollowNewClient

This was a really nice, elegant function. Then I fucked it up.
=================
*/
bool G_FollowNewClient( gentity_t *ent, int dir )
{
	int      clientnum = ent->client->sess.spectatorClient;
	int      original = clientnum;
	bool selectAny = false;

	if ( dir > 1 )
	{
		dir = 1;
	}
	else if ( dir < -1 )
	{
		dir = -1;
	}
	else if ( dir == 0 )
	{
		return true;
	}

	if ( ent->client->sess.spectatorState == SPECTATOR_NOT )
	{
		return false;
	}

	// select any if no target exists
	if ( clientnum < 0 || clientnum >= level.maxclients )
	{
		clientnum = original = 0;
		selectAny = true;
	}

	do
	{
		clientnum += dir;

		if ( clientnum >= level.maxclients )
		{
			clientnum = 0;
		}

		if ( clientnum < 0 )
		{
			clientnum = level.maxclients - 1;
		}

		// can't follow self
		if ( &g_entities[ clientnum ] == ent )
		{
			continue;
		}

		// avoid selecting existing follow target
		if ( clientnum == original && !selectAny )
		{
			continue; //effectively break;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED )
		{
			continue;
		}

		// can't follow a spectator
		if ( level.clients[ clientnum ].pers.team == TEAM_NONE )
		{
			continue;
		}

		// if stickyspec is disabled, can't follow someone in queue either
		if ( !ent->client->pers.stickySpec &&
		     level.clients[ clientnum ].sess.spectatorState != SPECTATOR_NOT )
		{
			continue;
		}

		// can only follow teammates when dead and on a team
		if ( ent->client->pers.team != TEAM_NONE &&
		     ( level.clients[ clientnum ].pers.team !=
		       ent->client->pers.team ) )
		{
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;

		// if this client is in the spawn queue, we need to do something special
		if ( level.clients[ clientnum ].sess.spectatorState != SPECTATOR_NOT )
		{
			G_FollowLockView( ent );
		}

		return true;
	}
	while ( clientnum != original );

	return false;
}

/*
=================
G_ToggleFollow
=================
*/
void G_ToggleFollow( gentity_t *ent )
{
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
	{
		G_StopFollowing( ent );
	}
	else
	{
		G_FollowNewClient( ent, 1 );
	}
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent )
{
	int  i;
	char arg[ MAX_NAME_LENGTH ];

	// won't work unless spectating
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT )
	{
		return;
	}

	if ( trap_Argc() != 2 )
	{
		G_ToggleFollow( ent );
	}
	else
	{
		char err[ MAX_STRING_CHARS ];
		trap_Argv( 1, arg, sizeof( arg ) );

		i = G_ClientNumberFromString( arg, err, sizeof( err ) );

		if ( i == -1 )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %s", QQ( "$1$: $2t$" ), "follow", Quote( err ) ) );
			return;
		}

		// can't follow self
		if ( &level.clients[ i ] == ent->client )
		{
			return;
		}

		// can't follow another spectator if sticky spec is off
		if ( !ent->client->pers.stickySpec &&
		     level.clients[ i ].sess.spectatorState != SPECTATOR_NOT )
		{
			return;
		}

		// if not on team spectator, you can only follow teammates
		if ( ent->client->pers.team != TEAM_NONE &&
		     ( level.clients[ i ].pers.team !=
		       ent->client->pers.team ) )
		{
			return;
		}

		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		ent->client->sess.spectatorClient = i;
	}
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent )
{
	char args[ 11 ];
	int  dir = 1;

	trap_Argv( 0, args, sizeof( args ) );

	if ( Q_stricmp( args, "followprev" ) == 0 )
	{
		dir = -1;
	}

	// won't work unless spectating
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT )
	{
		return;
	}

	G_FollowNewClient( ent, dir );
}

static void Cmd_Ignore_f( gentity_t *ent )
{
	int      pids[ MAX_CLIENTS ];
	char     name[ MAX_NAME_LENGTH ];
	char     cmd[ 9 ];
	int      matches = 0;
	int      i;
	bool ignore = false;

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( Q_stricmp( cmd, "ignore" ) == 0 )
	{
		ignore = true;
	}

	if ( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent - g_entities, va( "print_tr \"" S_SKIPNOTIFY
		                        "%s\" %s", N_("usage: $1$ [clientNum | partial name match]"), cmd ) );
		return;
	}

	Q_strncpyz( name, ConcatArgs( 1 ), sizeof( name ) );
	matches = G_ClientNumbersFromString( name, pids, MAX_CLIENTS );

	if ( matches < 1 )
	{
		trap_SendServerCommand( ent - g_entities, va( "print_tr \"" S_SKIPNOTIFY
		                        "%s\" %s %s", N_("$1$: no clients match the name '$2$'"), cmd, Quote( name ) ) );
		return;
	}

	for ( i = 0; i < matches; i++ )
	{
		if ( ignore )
		{
			if ( !Com_ClientListContains( &ent->client->sess.ignoreList, pids[ i ] ) )
			{
				Com_ClientListAdd( &ent->client->sess.ignoreList, pids[ i ] );
				ClientUserinfoChanged( ent->client->ps.clientNum, false );
				trap_SendServerCommand( ent - g_entities, va( "print_tr \"" S_SKIPNOTIFY
				                        "%s\" %s", N_("ignore: added $1$^* to your ignore list"),
				                        Quote( level.clients[ pids[ i ] ].pers.netname ) ) );
			}
			else
			{
				trap_SendServerCommand( ent - g_entities, va( "print_tr \"" S_SKIPNOTIFY
				                        "%s\" %s", N_("ignore: $1$^* is already on your ignore list"),
				                        Quote( level.clients[ pids[ i ] ].pers.netname ) ) );
			}
		}
		else
		{
			if ( Com_ClientListContains( &ent->client->sess.ignoreList, pids[ i ] ) )
			{
				Com_ClientListRemove( &ent->client->sess.ignoreList, pids[ i ] );
				ClientUserinfoChanged( ent->client->ps.clientNum, false );
				trap_SendServerCommand( ent - g_entities, va( "print_tr \"" S_SKIPNOTIFY
				                        "%s\" %s", N_("unignore: removed $1$^* from your ignore list"),
				                        Quote( level.clients[ pids[ i ] ].pers.netname ) ) );
			}
			else
			{
				trap_SendServerCommand( ent - g_entities, va( "print_tr \"" S_SKIPNOTIFY
				                        "%s\" %s", N_("unignore: $1$^* is not on your ignore list"),
				                        Quote( level.clients[ pids[ i ] ].pers.netname ) )  );
			}
		}
	}
}

/*
=================
Cmd_ListMaps_f

List all maps on the server
=================
*/

#define MAX_MAPLIST_MAPS 256
#define MAX_MAPLIST_ROWS 9
#define MAX_MAPLIST_COLS 3

void Cmd_ListMaps_f( gentity_t *ent )
{
	char search[ 16 ] = { "" };
	int  page = 0;

	if ( trap_Argc() > 1 )
	{
		trap_Argv( 1, search, sizeof( search ) );

		char *p;
		for ( p = search; ( *p ) && Str::cisdigit( *p ); p++ ) {; }

		if ( !( *p ) )
		{
			page = atoi( search );
			search[ 0 ] = '\0';
		}
		else if ( trap_Argc() > 2 )
		{
			char lp[ 8 ];
			trap_Argv( 2, lp, sizeof( lp ) );
			page = atoi( lp );
		}

		if ( page > 0 )
		{
			page--;
		}
		else if ( page < 0 )
		{
			page = 0;
		}
	}

	const char *mapNames[ MAX_MAPLIST_MAPS ] = { 0 };
	int         mapNamesCount = 0;
	const auto paks = FS::GetAvailablePaks();

	for ( size_t i = 0; i < paks.size(); ++i )
	{
		const char *pakName = paks[ i ].name.c_str();

		// Filter out duplicates
		if ( i && !strcmp( pakName, paks[ i - 1 ].name.c_str() ) )
		{
			continue;
		}

		if ( Q_strncmp( "map-", pakName, 4 ) ||
			 ( search[ 0 ] && !strstr( pakName + 4, search ) ) )
		{
			continue;
		}

		mapNames[ mapNamesCount++ ] = pakName + 4;
	}

	std::sort( mapNames,
			   mapNames + mapNamesCount,
			   [] ( const char *mapNameFirst, const char *mapNameSecond )
			   {
			   		return strcmp( mapNameFirst, mapNameSecond ) < 0;
			   }
	);

	/* About the MAX_MAPLIST_COLS - 1 trick:
	 *
	 * This is to fill an extra row when the number of map is not a
	 * multiple of MAX_MAPLIST_COLS, 1 being the obvious minimal number
	 * of map to make the filling effective and add an extra row.
	 *
	 * Because:
	 *     1 extra map name + MAX_MAPLIST_COLS - 1 = MAX_MAPLIST_COLS
	 * The condition:
	 *     N extra map name + MAX_MAPLIST_COLS - 1 > MAX_MAPLIST_COLS
	 * is true when a an extra row is needed.
	 *
	 * The integer result of the division by MAX_MAPLIST_COLS does
	 * that > condition implicitely since:
	 *     N extra maps + MAX_MAPLIST_COLS - 1 / MAX_MAPLIST_COLS
	 * returns 0 if it divides a multiple of MAX_MAPLIST_COLS
	 * or truncates to 1 if not, so:
	 *
	 * With I a multiple of MAX_MAPLIST_COLS
	 * And N a number of extra maps not being a multiple of MAX_MAPLIST_COLS
	 * The way:
	 *    mapNamesCount = I × MAX_MAPLIST_COLS + N
	 *
	 * We can compute:
	 *    rows = (I + N + MAX_MAPLIST_COLS - 1)                     / MAX_MAPLIST_COLS
	 *    rows = (I / MAX_MAPLIST_COLS) + (N + MAX_MAPLIST_COLS - 1 / MAX_MAPLIST_COLS)
	 *    rows =  I                     + 1
	 *
	 * This adds an extra row every time a row is not enough to contains
	 * remaining map names.
	 *
	 * The rows + MAX_MAPLIST_ROWS - 1 ) / MAX_MAPLIST_ROWS trick does the same
	 * to add an extra page everytime a column is not enough to contains the
	 * remaining map names */

	int rows = ( mapNamesCount + MAX_MAPLIST_COLS - 1 ) / MAX_MAPLIST_COLS;
	int pages = std::max( 1, ( rows + MAX_MAPLIST_ROWS - 1 ) / MAX_MAPLIST_ROWS );

	if ( page >= pages )
	{
		page = pages - 1;
	}

	int start = page * MAX_MAPLIST_ROWS * MAX_MAPLIST_COLS;

	if ( mapNamesCount < start + ( MAX_MAPLIST_ROWS * MAX_MAPLIST_COLS) )
	{
		rows = ( mapNamesCount - start + MAX_MAPLIST_COLS - 1 ) / MAX_MAPLIST_COLS;
	}
	else
	{
		rows = MAX_MAPLIST_ROWS;
	}

	char mapName[ MAX_QPATH ];

	trap_Cvar_VariableStringBuffer( "mapname", mapName, sizeof( mapName ) );

	ADMBP_begin();

	int shownMapNamesCount = 0;

	for ( int row = 0; row < rows; row++ )
	{
		for ( int i = start + row, j = 0; i < mapNamesCount && j < MAX_MAPLIST_COLS; i += rows, j++ )
		{
			const char *printedMapName = mapNames[ i ];

			if ( !strcmp( printedMapName, mapName ) )
			{
				ADMBP_raw( va( "^3 %-20s", printedMapName ) );
			}
			else
			{
				ADMBP_raw( va( "^7 %-20s", printedMapName ) );
			}

			shownMapNamesCount++;
		}

		ADMBP( "" );
	}

	ADMBP_end();

	if ( search[ 0 ] )
	{
		ADMP_P( va( "%s %d %s",
					Quote( P_("^3listmaps:^* found $1$ map matching '$2$^*'",
							  "^3listmaps:^* found $1$ maps matching '$2$^*'",
							  mapNamesCount)
					),
					mapNamesCount,
					Quote( search )
			      ),
				mapNamesCount );
	}
	else
	{
		ADMP_P( va( "%s %d %d",
					Quote( P_("^3listmaps:^* listing $1$ of $2$ map",
							  "^3listmaps:^* listing $1$ of $2$ maps",
							  mapNamesCount)
					),
					shownMapNamesCount,
					mapNamesCount
				  ),
				mapNamesCount );
	}

	if ( pages > 1 && page + 1 < pages )
	{
		ADMP( va( "%s %d %d %s %s %d",
				  QQ( N_("^3listmaps:^* page $1$ of $2$; use 'listmaps $3$$4$$5$' to see more") ),
		          page + 1, pages, Quote( search ), ( search[ 0 ] ) ? " " : "", page + 2 ) );
	}
	else if ( pages > 1 )
	{
		ADMP( va( "%s %d %d", QQ( N_("^3listmaps:^* page $1$ of $2$") ),  page + 1, pages ) );
	}
}

#define MAX_MAPLOGS 5

struct mapLogResult_t {
	char       flag;
	const char description[64];
};

static const mapLogResult_t maplog_table[] = {
	{ 't', "^7tie"                                  },
	{ 'a', "^1Alien win"                            },
	{ 'A', "^1Alien win^* / Humans admitted defeat" },
	{ 'h', "^5Human win"                            },
	{ 'H', "^5Human win^* / Aliens admitted defeat" },
	{ 'd', "^2draw vote"                            },
	{ 'm', "^2map vote"                             },
	{ 'r', "^2restart vote"                         },
	{ 'M', "^6admin changed map"                    },
	{ 'N', "^6admin loaded next map"                },
	{ 'R', "^6admin restarted map"                  },
	{ '\0', "" }
};

void G_MapLog_NewMap()
{
	char maplog[ MAX_CVAR_VALUE_STRING ];
	char map[ MAX_QPATH ];
	char *ptr;
	int  count = 0;

	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
	Q_strncpyz( maplog, g_mapLog.string, sizeof( maplog ) );
	ptr = maplog;

	while ( *ptr && count < MAX_MAPLOGS )
	{
		while ( *ptr && *ptr != ' ' )
		{
			ptr++;
		}
		count++;

		if ( count == MAX_MAPLOGS )
		{
			*ptr = '\0';
		}

		if ( *ptr == ' ' )
		{
			ptr++;
		}
	}

	trap_Cvar_Set( "g_mapLog",
	               va( "%s%s%s", map, maplog[ 0 ] ? " " : "", maplog ) );
}

void G_MapLog_Result( char result )
{
	static int lastTime = 0;
	char   maplog[ MAX_CVAR_VALUE_STRING ];
	int    t;

	// there is a chance is called more than once per frame
	if ( level.time == lastTime )
	{
		return;
	}

	lastTime = level.time;

	// check for earlier result
	if ( g_mapLog.string[ 0 ] && g_mapLog.string[ 1 ] == ';' )
	{
		return;
	}

	if ( level.surrenderTeam != TEAM_NONE )
	{
		if ( result == 'a' && level.surrenderTeam == TEAM_HUMANS )
		{
			result = 'A';
		}
		else if ( result == 'h' && level.surrenderTeam == TEAM_ALIENS )
		{
			result = 'H';
		}
	}

	t = level.matchTime / 1000;
	Q_strncpyz( maplog, g_mapLog.string, sizeof( maplog ) );
	trap_Cvar_Set( "g_mapLog",
	               va( "%c;%d:%02d;%s", result, t / 60, t % 60, maplog ) );
}

/*
=================
Cmd_MapLog_f

Print recent map results
=================
*/
void Cmd_MapLog_f( gentity_t *ent )
{
	char maplog[ MAX_CVAR_VALUE_STRING ];
	char *ptr;
	int  i;

	Q_strncpyz( maplog, g_mapLog.string, sizeof( maplog ) );
	ptr = maplog;

	ADMP( "\"" N_("^3maplog:^* recent map results, newest first") "\"" );
	ADMBP_begin();

	while( *ptr )
	{
		const char *clock = "  -:--";
		const char *result = "^1unknown";
		char       *end = ptr;

		while ( *end && *end != ' ' )
		{
			end++;
		}

		if ( *end == ' ' )
		{
			*end++ = '\0';
		}

		if ( ptr[ 0 ] && ptr[ 1 ] == ';' )
		{
			for ( i = 0; maplog_table[ i ].flag; i++ )
			{
				if ( maplog_table[ i ].flag == *ptr )
				{
					result = maplog_table[ i ].description;
					break;
				}
			}
			ptr += 2;
			clock = ptr;
			while( *ptr && *ptr != ';' )
			{
				ptr++;
			}

			if( *ptr == ';' )
			{
				*ptr++ = '\0';
			}
		}
		else if ( ptr == maplog )
		{
			result = "^7current map";
		}

		ADMBP( va( "  ^%s%-20s %6s %s^*",
		           ptr == maplog ? "2" : "7",
		           ptr, clock, result ) );
		ptr = end;
	}
	ADMBP_end();
}

/*
=================
Cmd_Damage_f

Deals damage to you (for testing), arguments: [damage] [dx] [dy] [dz]
The dx/dy arguments describe the damage point's offset from the entity origin
=================
*/
void Cmd_Damage_f( gentity_t *ent )
{
	vec3_t   point;
	char     arg[ 16 ];
	float    dx = 0.0f, dy = 0.0f, dz = 100.0f;
	int      damage = 100;
	bool nonloc = true;

	if ( trap_Argc() > 1 )
	{
		trap_Argv( 1, arg, sizeof( arg ) );
		damage = atoi( arg );
	}

	if ( trap_Argc() > 4 )
	{
		trap_Argv( 2, arg, sizeof( arg ) );
		dx = atof( arg );
		trap_Argv( 3, arg, sizeof( arg ) );
		dy = atof( arg );
		trap_Argv( 4, arg, sizeof( arg ) );
		dz = atof( arg );
		nonloc = false;
	}

	VectorCopy( ent->s.origin, point );
	point[ 0 ] += dx;
	point[ 1 ] += dy;
	point[ 2 ] += dz;

	ent->entity->Damage((float)damage, nullptr, Vec3::Load(point), Util::nullopt,
	                    nonloc ? DAMAGE_NO_LOCDAMAGE : 0, MOD_TARGET_LASER);
}

/*
=================
Cmd_Beacon_f
=================
*/
void Cmd_Beacon_f( gentity_t *ent )
{
	char         type_str[ 64 ];
	beaconType_t type;
	team_t       team;
	int          flags;
	vec3_t       origin, end, forward;
	trace_t      tr;
	const beaconAttributes_t *battr;

	// Check usage.
	if ( trap_Argc( ) < 2 )
	{
		trap_SendServerCommand( ent - g_entities,
		                        va( "print_tr %s", QQ( N_("Usage: beacon [type]") ) ) );
		return;
	}

	// Get arguments.
	trap_Argv( 1, type_str, sizeof( type_str ) );

	battr = BG_BeaconByName( type_str );

	// Check arguments.
	if ( !battr || battr->flags & BCF_RESERVED )
	{
		trap_SendServerCommand( ent - g_entities,
		                        va( "print_tr %s %s", QQ( N_("Unknown beacon type $1$") ),
		                            Quote( type_str ) ) );
		return;
	}

	type  = battr->number;
	flags = battr->flags;
	team  = (team_t)ent->client->pers.team;

	// Trace in view direction.
	BG_GetClientViewOrigin( &ent->client->ps, origin );
	AngleVectors( ent->client->ps.viewangles, forward, nullptr, nullptr );
	VectorMA( origin, 65536, forward, end );

	G_UnlaggedOn( ent, origin, 65536 );
	trap_Trace( &tr, origin, nullptr, nullptr, end, ent->s.number, MASK_PLAYERSOLID, 0 );
	G_UnlaggedOff( );

	// Evaluate flood limit.
	if( G_FloodLimited( ent ) )
		return;

	if ( !( flags & BCF_PRECISE ) )
		Beacon::MoveTowardsRoom( tr.endpos );

	Beacon::Propagate( Beacon::New( tr.endpos, type, 0, team, ent->s.number, BCH_REMOVE ) );
	return;
}

/*
==================
G_FloodLimited

Determine whether a user is flood limited, and adjust their flood demerits
Print them a warning message if they are over the limit
Return is time in msec until the user can speak again
==================
*/
int G_FloodLimited( gentity_t *ent )
{
	int deltatime, ms;

	if ( g_floodMinTime.integer <= 0 )
	{
		return 0;
	}

	if ( G_admin_permission( ent, ADMF_NOCENSORFLOOD ) )
	{
		return 0;
	}

	deltatime = level.time - ent->client->pers.floodTime;

	ent->client->pers.floodDemerits += g_floodMinTime.integer - deltatime;

	if ( ent->client->pers.floodDemerits < 0 )
	{
		ent->client->pers.floodDemerits = 0;
	}

	ent->client->pers.floodTime = level.time;

	ms = ent->client->pers.floodDemerits - g_floodMaxDemerits.integer;

	if ( ms <= 0 )
	{
		return 0;
	}

	trap_SendServerCommand( ent - g_entities, va( "print_tr %s %d", QQ( N_("You are flooding: "
	                        "please wait $1$s before trying again") ),
	                        ( ms + 999 ) / 1000 ) );
	return ms;
}

static void Cmd_Pubkey_Identify_f( gentity_t *ent )
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

// commands must be in alphabetical order!
// keep the list synchronized with the list in cg_consolecmds for completion.
static const commands_t cmds[] =
{
	{ "a",               CMD_MESSAGE | CMD_INTERMISSION,      Cmd_AdminMessage_f     },
	{ "asay",            CMD_MESSAGE | CMD_INTERMISSION,      Cmd_Say_f              },
	{ "beacon",          CMD_TEAM | CMD_ALIVE,                Cmd_Beacon_f           },
	{ "build",           CMD_TEAM | CMD_ALIVE,                Cmd_Build_f            },
	{ "buy",             CMD_HUMAN | CMD_ALIVE,               Cmd_Buy_f              },
	{ "callteamvote",    CMD_MESSAGE | CMD_TEAM,              Cmd_CallVote_f         },
	{ "callvote",        CMD_MESSAGE,                         Cmd_CallVote_f         },
	{ "class",           CMD_TEAM,                            Cmd_Class_f            },
	{ "damage",          CMD_CHEAT | CMD_ALIVE,               Cmd_Damage_f           },
	{ "deconstruct",     CMD_TEAM | CMD_ALIVE,                Cmd_Deconstruct_f      },
	{ "devteam",         CMD_CHEAT,                           Cmd_Devteam_f          },
	{ "follow",          CMD_SPEC,                            Cmd_Follow_f           },
	{ "follownext",      CMD_SPEC,                            Cmd_FollowCycle_f      },
	{ "followprev",      CMD_SPEC,                            Cmd_FollowCycle_f      },
	{ "give",            CMD_CHEAT | CMD_TEAM,                Cmd_Give_f             },
	{ "god",             CMD_CHEAT,                           Cmd_God_f              },
	{ "ignite",          CMD_CHEAT | CMD_TEAM | CMD_ALIVE,    Cmd_Ignite_f           },
	{ "ignore",          0,                                   Cmd_Ignore_f           },
	{ "itemact",         CMD_HUMAN | CMD_ALIVE,               Cmd_ActivateItem_f     },
	{ "itemdeact",       CMD_HUMAN | CMD_ALIVE,               Cmd_DeActivateItem_f   },
	{ "itemtoggle",      CMD_HUMAN | CMD_ALIVE,               Cmd_ToggleItem_f       },
	{ "kill",            CMD_TEAM | CMD_ALIVE,                Cmd_Kill_f             },
	{ "listmaps",        CMD_MESSAGE | CMD_INTERMISSION,      Cmd_ListMaps_f         },
	{ "listrotation",    CMD_MESSAGE | CMD_INTERMISSION,      G_PrintCurrentRotation },
	{ "m",               CMD_MESSAGE | CMD_INTERMISSION,      Cmd_PrivateMessage_f   },
	{ "maplog",          CMD_MESSAGE | CMD_INTERMISSION,      Cmd_MapLog_f           },
	{ "me",              CMD_MESSAGE | CMD_INTERMISSION,      Cmd_Me_f               },
	{ "me_team",         CMD_MESSAGE | CMD_INTERMISSION,      Cmd_Me_f               },
	{ "mt",              CMD_MESSAGE | CMD_INTERMISSION,      Cmd_PrivateMessage_f   },
	{ "noclip",          CMD_CHEAT_TEAM,                      Cmd_Noclip_f           },
	{ "notarget",        CMD_CHEAT | CMD_TEAM | CMD_ALIVE,    Cmd_Notarget_f         },
	{ "pubkey_identify", CMD_INTERMISSION,                    Cmd_Pubkey_Identify_f  },
	{ "reload",          CMD_HUMAN | CMD_ALIVE,               Cmd_Reload_f           },
	{ "say",             CMD_MESSAGE | CMD_INTERMISSION,      Cmd_Say_f              },
	{ "say_area",        CMD_MESSAGE | CMD_TEAM | CMD_ALIVE,  Cmd_SayArea_f          },
	{ "say_area_team",   CMD_MESSAGE | CMD_TEAM | CMD_ALIVE,  Cmd_SayAreaTeam_f      },
	{ "say_team",        CMD_MESSAGE | CMD_INTERMISSION,      Cmd_Say_f              },
	{ "score",           CMD_INTERMISSION,                    ScoreboardMessage      },
	{ "sell",            CMD_HUMAN | CMD_ALIVE,               Cmd_Sell_f             },
	{ "setviewpos",      CMD_CHEAT_TEAM,                      Cmd_SetViewpos_f       },
	{ "team",            0,                                   Cmd_Team_f             },
	{ "teamvote",        CMD_TEAM | CMD_INTERMISSION,         Cmd_Vote_f             },
	{ "unignore",        0,                                   Cmd_Ignore_f           },
	{ "vote",            CMD_INTERMISSION,                    Cmd_Vote_f             },
	{ "vsay",            CMD_MESSAGE | CMD_INTERMISSION,      Cmd_VSay_f             },
	{ "vsay_local",      CMD_MESSAGE | CMD_INTERMISSION,      Cmd_VSay_f             },
	{ "vsay_team",       CMD_MESSAGE | CMD_INTERMISSION,      Cmd_VSay_f             },
	{ "where",           0,                                   Cmd_Where_f            }
};
static const size_t numCmds = ARRAY_LEN( cmds );

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum )
{
	gentity_t  *ent;
	char       cmd[ MAX_TOKEN_CHARS ];
	commands_t *command;

	ent = g_entities + clientNum;

	if ( !ent->client || ent->client->pers.connected != CON_CONNECTED )
	{
		return; // not fully in game yet
	}

	trap_Argv( 0, cmd, sizeof( cmd ) );

	command = (commands_t*) bsearch( cmd, cmds, numCmds, sizeof( cmds[ 0 ] ), cmdcmp );

	if ( !command )
	{
		if ( !G_admin_cmd_check( ent ) )
		{
			trap_SendServerCommand( clientNum,
			                        va( "print_tr %s %s", QQ( N_("Unknown command $1$") ), Quote( cmd ) ) );
		}

		return;
	}

	// do tests here to reduce the amount of repeated code

	if ( !( command->cmdFlags & CMD_INTERMISSION ) &&
	     ( level.intermissiontime || level.pausedTime ) )
	{
		return;
	}

	if ( (command->cmdFlags & CMD_CHEAT) && !g_cheats.integer )
	{
		G_TriggerMenu( clientNum, MN_CMD_CHEAT );
		return;
	}

	if ( (command->cmdFlags & CMD_MESSAGE) && ( ent->client->pers.namelog->muted ||
	     G_FloodLimited( ent ) ) )
	{
		return;
	}

	if ( (command->cmdFlags & CMD_TEAM) &&
	     ent->client->pers.team == TEAM_NONE )
	{
		G_TriggerMenu( clientNum, MN_CMD_TEAM );
		return;
	}

	if ( (command->cmdFlags & CMD_CHEAT_TEAM) && !g_cheats.integer &&
	     ent->client->pers.team != TEAM_NONE )
	{
		G_TriggerMenu( clientNum, MN_CMD_CHEAT_TEAM );
		return;
	}

	if ( (command->cmdFlags & CMD_SPEC) &&
	     ent->client->sess.spectatorState == SPECTATOR_NOT )
	{
		G_TriggerMenu( clientNum, MN_CMD_SPEC );
		return;
	}

	if ( (command->cmdFlags & CMD_ALIEN) &&
	     ent->client->pers.team != TEAM_ALIENS )
	{
		G_TriggerMenu( clientNum, MN_CMD_ALIEN );
		return;
	}

	if ( (command->cmdFlags & CMD_HUMAN) &&
	     ent->client->pers.team != TEAM_HUMANS )
	{
		G_TriggerMenu( clientNum, MN_CMD_HUMAN );
		return;
	}

	if ( (command->cmdFlags & CMD_ALIVE) &&
	     ( Entities::IsDead( ent ) || ent->client->sess.spectatorState != SPECTATOR_NOT ) )
	{
		G_TriggerMenu( clientNum, MN_CMD_ALIVE );
		return;
	}

	command->cmdHandler( ent );
}

void G_UnEscapeString( const char *in, char *out, int len )
{
	len--;

	while ( *in && len > 0 )
	{
		if ( (unsigned char)*in >= ' ' || *in == '\n' )
		{
			*out++ = *in;
			len--;
		}

		in++;
	}

	*out = '\0';
}

void Cmd_PrivateMessage_f( gentity_t *ent )
{
	int      pids[ MAX_CLIENTS ];
	char     name[ MAX_NAME_LENGTH ];
	char     cmd[ 12 ];
	char     *msg;
	int      i, pcount;
	int      count = 0;
	bool teamonly = false;
	char     recipients[ MAX_STRING_CHARS ] = "";

	if ( !g_privateMessages.integer && ent )
	{
		ADMP( "\"" N_("Sorry, but private messages have been disabled") "\"" );
		return;
	}

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( trap_Argc() < 3 )
	{
		ADMP( va( "%s %s", QQ( N_("usage: $1$ [name|slot#] [message]") ), cmd ) );
		return;
	}

	if ( !Q_stricmp( cmd, "mt" ) )
	{
		teamonly = true;
	}

	trap_Argv( 1, name, sizeof( name ) );
	msg = ConcatArgs( 2 );
	pcount = G_ClientNumbersFromString( name, pids, MAX_CLIENTS );

	// send the message
	for ( i = 0; i < pcount; i++ )
	{
		if ( G_SayTo( ent, &g_entities[ pids[ i ] ],
		              teamonly ? SAY_TPRIVMSG : SAY_PRIVMSG, msg ) )
		{
			count++;
			Q_strcat( recipients, sizeof( recipients ), va( "%s^*, ",
			          level.clients[ pids[ i ] ].pers.netname ) );
		}
	}

	// report the results
	auto color = Color::ToString( teamonly ? Color::Cyan : Color::Yellow );

	if ( !count )
	{
		ADMP( va( "%s %s", QQ( N_("^3No player matching ^7 '$1$^*' ^3to send message to.") ),
		          Quote( name ) ) );
	}
	else
	{
		ADMP( va( "%s %s %s", QQ( N_("$1$Private message: ^7$2$") ), color.c_str(), Quote( msg ) ) );
		// remove trailing ", "
		recipients[ strlen( recipients ) - 2 ] = '\0';
		// FIXME PLURAL
		ADMP( va( "%s %s %i %s",
		          Quote( P_( "$1$sent to $2$ player: ^7$3$",
		                     "$1$sent to $2$ players: ^7$3$", count ) ),
		          color.c_str(), count, Quote( recipients ) ) );

		G_LogPrintf( "%s: %d \"%s^*\" \"%s\": %s%s",
		             ( teamonly ) ? "TPrivMsg" : "PrivMsg",
		             ( ent ) ? ( int )( ent - g_entities ) : -1,
		             ( ent ) ? ent->client->pers.netname : "console",
		             name, color.c_str(), msg );
	}
}

/*
=================
Cmd_AdminMessage_f

Send a message to all active admins
=================
*/
void Cmd_AdminMessage_f( gentity_t *ent )
{
	// Check permissions and add the appropriate user [prefix]
	if ( !G_admin_permission( ent, ADMF_ADMINCHAT ) )
	{
		if ( !g_publicAdminMessages.integer )
		{
			ADMP( "\"" N_("Sorry, but use of /a by non-admins has been disabled.") "\"" );
			return;
		}
		else
		{
			ADMP( "\"" N_("Your message has been sent to any available admins "
			      "and to the server logs.") "\"" );
		}
	}

	if ( trap_Argc() < 2 )
	{
		ADMP( "\"" N_("usage: a [message]") "\"" );
		return;
	}

	G_AdminMessage( ent, ConcatArgs( 1 ) );
}
