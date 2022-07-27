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
#include "Entities.h"
#include "sgame/lua/Hooks.h"

/*
================
G_TeamFromString

Return the team referenced by a string
================
*/
team_t G_TeamFromString( const char *str )
{
	switch ( Str::ctolower( *str ) )
	{
		case '0':
		case 's':
			return TEAM_NONE;

		case '1':
		case 'a':
			return TEAM_ALIENS;

		case '2':
		case 'h':
			return TEAM_HUMANS;

		default:
			return NUM_TEAMS;
	}
}

/*
================
G_TeamCommand

Broadcasts a command to only a specific team
================
*/
void G_TeamCommand( team_t team, const char *cmd )
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
				trap_SendServerCommand( i, cmd );
			}
		}
	}
}

/*
================
G_AreaTeamCommand

Broadcasts a command to only a specific team within a specific range
================
*/
void G_AreaTeamCommand( const gentity_t *ent, const char *cmd )
{
	int    entityList[ MAX_GENTITIES ];
	int    num, i;
	vec3_t range = { 1000.0f, 1000.0f, 1000.0f };
	vec3_t mins, maxs;
	team_t team = (team_t) ent->client->pers.team;

	for ( i = 0; i < 3; i++ )
	{
		range[ i ] = g_sayAreaRange.Get();
	}

	VectorAdd( ent->s.origin, range, maxs );
	VectorSubtract( ent->s.origin, range, mins );

	num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0; i < num; i++ )
	{
		if ( g_entities[ entityList[ i ] ].client && g_entities[ entityList[ i ] ].client->pers.connected == CON_CONNECTED )
		{
			if ( g_entities[ entityList[ i ] ].client->pers.team == team )
			{
				trap_SendServerCommand( entityList[ i ], cmd );
			}
		}
	}
}

// TODO: Add a TeamComponent.
team_t G_Team( const gentity_t *ent )
{
	if ( !ent )
	{
		return TEAM_NONE;
	}
	if ( ent->client )
	{
		return (team_t)ent->client->pers.team;
	}
	if ( ent->s.eType == entityType_t::ET_BUILDABLE )
	{
		return ent->buildableTeam;
	}
	return TEAM_NONE;
}

bool G_OnSameTeam( const gentity_t *ent1, const gentity_t *ent2 )
{
	team_t team1 = G_Team( ent1 );
	return ( team1 != TEAM_NONE && team1 == G_Team( ent2 ) );
}

/*
==================
G_ClientListForTeam
==================
*/
static clientList_t G_ClientListForTeam( team_t team )
{
	int          i;
	clientList_t clientList;

	memset( &clientList, 0, sizeof( clientList_t ) );

	for ( i = 0; i < level.maxclients; i++ )
	{
		gentity_t *ent = g_entities + i;

		if ( ent->client->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if ( ent->inuse && ( ent->client->pers.team == team ) )
		{
			Com_ClientListAdd( &clientList, ent->client->ps.clientNum );
		}
	}

	return clientList;
}

/*
==================
G_UpdateTeamConfigStrings
==================
*/
void G_UpdateTeamConfigStrings()
{
	clientList_t alienTeam = G_ClientListForTeam( TEAM_ALIENS );
	clientList_t humanTeam = G_ClientListForTeam( TEAM_HUMANS );

	if ( level.intermissiontime )
	{
		// No restrictions once the game has ended
		memset( &alienTeam, 0, sizeof( clientList_t ) );
		memset( &humanTeam, 0, sizeof( clientList_t ) );
	}

	trap_SetConfigstringRestrictions( CS_VOTE_TIME + TEAM_ALIENS,   &humanTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_STRING + TEAM_ALIENS, &humanTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_YES + TEAM_ALIENS,    &humanTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_NO + TEAM_ALIENS,     &humanTeam );

	trap_SetConfigstringRestrictions( CS_VOTE_TIME + TEAM_HUMANS,   &alienTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_STRING + TEAM_HUMANS, &alienTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_YES + TEAM_HUMANS,    &alienTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_NO + TEAM_HUMANS,     &alienTeam );
}

/*
==================
G_LeaveTeam
==================
*/

void G_LeaveTeam( gentity_t *self )
{
	team_t    team = (team_t) self->client->pers.team;
	gentity_t *ent;
	int       i;

	if ( G_IsPlayableTeam( team ) )
	{
		G_RemoveFromSpawnQueue( &level.team[ team ].spawnQueue, self->client->ps.clientNum );
	}
	else
	{
		if ( self->client->sess.spectatorState == SPECTATOR_FOLLOW )
		{
			G_StopFollowing( self );
		}

		return;
	}

	// stop any following clients
	G_StopFromFollowing( self );

	G_Vote( self, team, false );
	self->suicideTime = 0;

	for ( i = 0; i < level.num_entities; i++ )
	{
		ent = &g_entities[ i ];

		if ( !ent->inuse )
		{
			continue;
		}

		if ( ent->client && ent->client->pers.connected == CON_CONNECTED )
		{
			// cure poison
			if ( ( ent->client->ps.stats[ STAT_STATE ] & SS_POISONED ) &&
			     ent->client->lastPoisonClient == self )
			{
				ent->client->ps.stats[ STAT_STATE ] &= ~SS_POISONED;
			}

			// reset damage dealt to the other clients
			ent->credits[ self->client->ps.clientNum ] = { 0.0f, 0, TEAM_NONE };
		}
		else if ( ent->s.eType == entityType_t::ET_MISSILE && ent->r.ownerNum == self->s.number )
		{
			G_FreeEntity( ent );
		}
	}

	trap_UnlinkEntity( self );

	// cut all relevant zap beams
	G_ClearPlayerZapEffects( self );

	Beacon::DeleteTags( self );
	Beacon::RemoveOrphaned( self->s.number );

	G_namelog_update_score( self->client );
}

/*
=================
G_ChangeTeam
=================
*/

void G_ChangeTeam( gentity_t *ent, team_t newTeam )
{
	team_t oldTeam = (team_t) ent->client->pers.team;

	if ( oldTeam == newTeam )
	{
		return;
	}

	G_LeaveTeam( ent );
	ent->client->pers.teamChangeTime = level.time;
	ent->client->pers.team = newTeam;
	ent->client->pers.teamInfo = level.startTime - 1;
	ent->client->pers.classSelection = PCL_NONE;
	ClientSpawn( ent, nullptr, nullptr, nullptr );

	if ( oldTeam == TEAM_HUMANS && newTeam == TEAM_ALIENS )
	{
		// Convert from human to alien credits
		ent->client->pers.credit =
		  ( int )( ent->client->pers.credit *
		           ALIEN_MAX_CREDITS / HUMAN_MAX_CREDITS + 0.5f );
	}
	else if ( oldTeam == TEAM_ALIENS && newTeam == TEAM_HUMANS )
	{
		// Convert from alien to human credits
		ent->client->pers.credit =
		  ( int )( ent->client->pers.credit *
		           HUMAN_MAX_CREDITS / ALIEN_MAX_CREDITS + 0.5f );
	}

	if ( !g_cheats )
	{
		if ( ent->client->noclip )
		{
			ent->client->noclip = false;
			ent->r.contents = ent->client->cliprcontents;
		}
		ent->flags &= ~( FL_GODMODE | FL_NOTARGET );
	}

	// Copy credits to ps for the client
	ent->client->ps.persistant[ PERS_CREDIT ] = ent->client->pers.credit;

	// Update PERS_UNLOCKABLES in the same frame as PERS_TEAM to prevent bad status change notifications
	ent->client->ps.persistant[ PERS_UNLOCKABLES ] = BG_UnlockablesMask( newTeam );

	ClientUserinfoChanged( ent->client->ps.clientNum, false );

	G_UpdateTeamConfigStrings();

	Beacon::PropagateAll( );

	G_LogPrintf( "ChangeTeam: %d %s: %s^* switched teams",
	             ( int )( ent - g_entities ), BG_TeamName( newTeam ), ent->client->pers.netname );

	G_namelog_update_score( ent->client );
	TeamplayInfoMessage( ent );
	Unv::SGame::Lua::ExecTeamChangeHooks(ent, newTeam);
}

/**
 * @todo Move out of sg_team.c as it is not team-specific.
 */
gentity_t *GetCloseLocationEntity( gentity_t *ent )
{
	gentity_t *eloc, *best;
	float     bestlen, len;

	best = nullptr;
	bestlen = 3.0f * 8192.0f * 8192.0f;

	for ( eloc = level.locationHead; eloc; eloc = eloc->nextPathSegment )
	{
		len = DistanceSquared( ent->r.currentOrigin, eloc->r.currentOrigin );

		if ( len > bestlen )
		{
			continue;
		}

		if ( !trap_InPVS( ent->r.currentOrigin, eloc->r.currentOrigin ) )
		{
			continue;
		}

		bestlen = len;
		best = eloc;
	}

	return best;
}

/*---------------------------------------------------------------------------*/

/*
==================
TeamplayInfoMessage

Format:
  clientNum location health weapon upgrade

==================
*/
void TeamplayInfoMessage( gentity_t *ent )
{
	char      entry[ 24 ];
	char      string[ ( MAX_CLIENTS - 1 ) * ( sizeof( entry ) - 1 ) + 1 ];
	int       i, j;
	int       team, stringlength;
	gentity_t *player;
	gclient_t *cl;
	upgrade_t upgrade = UP_NONE;
	int       curWeaponClass = WP_NONE; // sends weapon for humans, class for aliens
	int       health = 0;

	if ( !g_allowTeamOverlay.Get() )
	{
		return;
	}

	if ( !ent->client->pers.teamInfo )
	{
		return;
	}

	if ( ent->client->pers.team == TEAM_NONE )
	{
		if ( ent->client->sess.spectatorState == SPECTATOR_FREE ||
		     ent->client->sess.spectatorClient < 0 )
		{
			return;
		}

		team = g_entities[ ent->client->sess.spectatorClient ].client->
		       pers.team;
	}
	else
	{
		team = ent->client->pers.team;
	}

	string[ 0 ] = '\0';
	stringlength = 0;

	for ( i = 0; i < level.maxclients; i++ )
	{
		player = g_entities + i;
		cl = player->client;

		if ( ent == player || !cl || team != cl->pers.team ||
		     !player->inuse )
		{
			continue;
		}

		// only update if changed since last time
		if ( cl->pers.infoChangeTime <= ent->client->pers.teamInfo )
		{
			continue;
		}

		if ( cl->sess.spectatorState != SPECTATOR_NOT )
		{
			curWeaponClass = WP_NONE;
			upgrade = UP_NONE;
		}
		else if ( cl->pers.team == TEAM_HUMANS )
		{
			curWeaponClass = cl->ps.weapon;

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
			else if ( BG_InventoryContainsUpgrade( UP_LIGHTARMOUR, cl->ps.stats ) )
			{
				upgrade = UP_LIGHTARMOUR;
			}
			else
			{
				upgrade = UP_NONE;
			}
			health = static_cast<int>( std::ceil( Entities::HealthOf(player) ) );
		}
		else if ( cl->pers.team == TEAM_ALIENS )
		{
			curWeaponClass = cl->ps.stats[ STAT_CLASS ];
			upgrade = UP_NONE;
			health = static_cast<int>( std::ceil( Entities::HealthOf(player) ) );
		}

		if( team == TEAM_ALIENS ) // aliens don't have upgrades
		{
			Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i", i,
						 cl->pers.location,
			             health,
						 curWeaponClass,
						 cl->pers.credit );
		}
		else
		{
			Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i", i,
			             cl->pers.location,
			             health,
			             curWeaponClass,
			             cl->pers.credit,
			             upgrade );
		}


		j = strlen( entry );

		// this should not happen if entry and string sizes are correct
		if ( stringlength + j >= (int) sizeof( string ) )
		{
			break;
		}

		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	if( string[ 0 ] )
	{
		trap_SendServerCommand( ent - g_entities, va( "tinfo%s", string ) );
		ent->client->pers.teamInfo = level.time;
	}
}

static Cvar::Cvar<bool> countBots("g_teamBalanceCountBots", "include bots when checking team size", Cvar::NONE, false);
int G_PlayerCountForBalance( team_t team )
{
	return countBots.Get() ? level.team[ team ].numClients : level.team[ team ].numPlayers;
}

void CheckTeamStatus()
{
	int       i;
	gentity_t *loc, *ent;

	if ( level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME )
	{
		level.lastTeamLocationTime = level.time;

		for ( i = 0; i < level.maxclients; i++ )
		{
			ent = g_entities + i;

			if ( ent->client->pers.connected != CON_CONNECTED )
			{
				continue;
			}

			if ( ent->inuse && G_IsPlayableTeam( ent->client->pers.team ) )
			{
				loc = GetCloseLocationEntity( ent );

				if ( loc )
				{
					if( ent->client->pers.location != loc->s.generic1 )
					{
						ent->client->pers.infoChangeTime = level.time;
						ent->client->pers.location = loc->s.generic1;
					}
				}
				else if ( ent->client->pers.location != 0 )
				{
					ent->client->pers.infoChangeTime = level.time;
					ent->client->pers.location = 0;
				}
			}
		}

		for ( i = 0; i < level.maxclients; i++ )
		{
			ent = g_entities + i;

			if ( ent->client->pers.connected != CON_CONNECTED )
			{
				continue;
			}

			if ( ent->inuse )
			{
				TeamplayInfoMessage( ent );
			}
		}
	}

	// Warn on imbalanced teams
	if ( g_teamImbalanceWarnings.Get() && !level.intermissiontime &&
	     ( level.time - level.lastTeamImbalancedTime >
	       ( g_teamImbalanceWarnings.Get() * 1000 ) ) &&
	     level.numTeamImbalanceWarnings < 3 && !level.restarted )
	{
		level.lastTeamImbalancedTime = level.time;

		if ( level.team[ TEAM_ALIENS ].numSpawns > 0 &&
		     G_PlayerCountForBalance( TEAM_HUMANS ) - G_PlayerCountForBalance( TEAM_ALIENS ) > 2 )
		{
			trap_SendServerCommand( -1, "print_tr \"" N_("Teams are imbalanced. "
			                        "Humans have more players.") "\"" );
			level.numTeamImbalanceWarnings++;
		}
		else if ( level.team[ TEAM_HUMANS ].numSpawns > 0 &&
		          G_PlayerCountForBalance( TEAM_ALIENS ) - G_PlayerCountForBalance( TEAM_HUMANS ) > 2 )
		{
			trap_SendServerCommand( -1, "print_tr \"" N_("Teams are imbalanced. "
			                        "Aliens have more players.") "\"" );
			level.numTeamImbalanceWarnings++;
		}
		else
		{
			level.numTeamImbalanceWarnings = 0;
		}
	}
}
