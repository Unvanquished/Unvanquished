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

// g_multiview.c: Multiview handling
// ---------------------------------
//
#include "g_local.h"

qboolean G_smvCommands( gentity_t *ent, char *cmd )
{
#ifdef MV_SUPPORT

	if( !Q_stricmp( cmd, "mvadd" ) )
	{
		G_smvAdd_cmd( ent );
	}
	else if( !Q_stricmp( cmd, "mvdel" ) )
	{
		G_smvDel_cmd( ent );
	}
	else if( !Q_stricmp( cmd, "mvallies" ) )
	{
		G_smvAddTeam_cmd( ent, TEAM_ALLIES );
	}
	else if( !Q_stricmp( cmd, "mvaxis" ) )
	{
		G_smvAddTeam_cmd( ent, TEAM_AXIS );
	}
	else if( !Q_stricmp( cmd, "mvall" ) )
	{
		G_smvAddTeam_cmd( ent, TEAM_ALLIES );
		G_smvAddTeam_cmd( ent, TEAM_AXIS );
	}
	else if( !Q_stricmp( cmd, "mvnone" ) )
	{
		if( ent->client->pers.mvCount > 0 )
		{
			G_smvRemoveInvalidClients( ent, TEAM_AXIS );
			G_smvRemoveInvalidClients( ent, TEAM_ALLIES );
		}
	}
	else
	{
		return ( qfalse );
	}

	return ( qtrue );

#else

	return ( qfalse );

#endif
}

void G_smvAdd_cmd( gentity_t *ent )
{
	int  pID;
	char str[ MAX_TOKEN_CHARS ];

	// Clients will always send pIDs
	trap_Argv( 1, str, sizeof( str ) );
	pID = atoi( str );

	if( pID < 0 || pID > level.maxclients || g_entities[ pID ].client->pers.connected != CON_CONNECTED )
	{
		CP( va( "print \"[lof]** [lon]Client[lof] %d [lon]is not connected[lof]!\n\"", pID ) );
		return;
	}

	if( g_entities[ pID ].client->sess.sessionTeam == TEAM_SPECTATOR )
	{
		CP( va( "print \"[lof]** [lon]Client[lof] %s^7 [lon]is not in the game[lof]!\n\"", level.clients[ pID ].pers.netname ) );
		return;
	}

	if( !G_allowFollow( ent, G_teamID( &g_entities[ pID ] ) ) )
	{
		CP( va( "print \"[lof]** [lon]The %s team is locked from spectators[lof]!\n\"", aTeams[ G_teamID( &g_entities[ pID ] ) ] ) );
		return;
	}

	G_smvAddView( ent, pID );
}

void G_smvAddTeam_cmd( gentity_t *ent, int nTeam )
{
	int i, pID;

	if( !G_allowFollow( ent, nTeam ) )
	{
		CP( va( "print \"[lof]** [lon]The %s team is locked from spectators[lof]!\n\"", aTeams[ nTeam ] ) );
		return;
	}

	// For limbo'd MV action
	if( ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
	    ( !( ent->client->ps.pm_flags & PMF_LIMBO ) || ent->client->sess.sessionTeam != nTeam ) )
	{
		return;
	}

	for( i = 0; i < level.numPlayingClients; i++ )
	{
		pID = level.sortedClients[ i ];

		if( g_entities[ pID ].client->sess.sessionTeam == nTeam && ent != &g_entities[ pID ] )
		{
			G_smvAddView( ent, pID );
		}
	}
}

void G_smvDel_cmd( gentity_t *ent )
{
	int  pID;
	char str[ MAX_TOKEN_CHARS ];

	// Clients will always send pIDs
	trap_Argv( 1, str, sizeof( str ) );
	pID = atoi( str );

	if( !G_smvLocateEntityInMVList( ent, pID, qtrue ) )
	{
		CP( va( "print \"[lof]** [lon]Client[lof] %s^7 [lon]is not currently viewed[lof]!\n\"", level.clients[ pID ].pers.netname ) );
	}
}

// Add a player entity to another player's multiview list
void G_smvAddView( gentity_t *ent, int pID )
{
	int       i;
	mview_t   *mv = NULL;
	gentity_t *v;

	if( pID >= MAX_MVCLIENTS || G_smvLocateEntityInMVList( ent, pID, qfalse ) )
	{
		return;
	}

	for( i = 0; i < MULTIVIEW_MAXVIEWS; i++ )
	{
		if( !ent->client->pers.mv[ i ].fActive )
		{
			mv = &ent->client->pers.mv[ i ];
			break;
		}
	}

	if( mv == NULL )
	{
		CP( va( "print \"[lof]** [lon]Sorry, no more MV slots available (all[lof] %d [lon]in use)[lof]\n\"", MULTIVIEW_MAXVIEWS ) );
		return;
	}

	mv->camera = G_Spawn();

	if( mv->camera == NULL )
	{
		return;
	}

	if( ent->client->sess.sessionTeam == TEAM_SPECTATOR &&  /*ent->client->sess.sessionTeam != TEAM_SPECTATOR || */
	    ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
	{
		SetTeam( ent, "s", qtrue, -1, -1, qfalse );
	}
	else if( ent->client->sess.sessionTeam != TEAM_SPECTATOR && !( ent->client->ps.pm_flags & PMF_LIMBO ) )
	{
		limbo( ent, qtrue );
	}

	ent->client->ps.clientNum = ent - g_entities;
	ent->client->sess.spectatorState = SPECTATOR_FREE;

	ent->client->pers.mvCount++;
	mv->fActive = qtrue;
	mv->entID = pID;

	v = mv->camera;
	v->classname = "misc_portal_surface";
	v->r.svFlags = SVF_PORTAL | SVF_SINGLECLIENT; // Only merge snapshots for the target client
	v->r.singleClient = ent->s.number;
	v->s.eType = ET_PORTAL;

	VectorClear( v->r.mins );
	VectorClear( v->r.maxs );
	trap_LinkEntity( v );

	v->target_ent = &g_entities[ pID ];
	v->TargetFlag = pID;
	v->tagParent = ent;

	G_smvUpdateClientCSList( ent );
}

// Find, and optionally delete an entity in a player's MV list
qboolean G_smvLocateEntityInMVList( gentity_t *ent, int pID, qboolean fRemove )
{
	int i;

	if( ent->client->pers.mvCount > 0 )
	{
		for( i = 0; i < MULTIVIEW_MAXVIEWS; i++ )
		{
			if( ent->client->pers.mv[ i ].fActive && ent->client->pers.mv[ i ].entID == pID )
			{
				if( fRemove )
				{
					G_smvRemoveEntityInMVList( ent, &ent->client->pers.mv[ i ] );
				}

				return ( qtrue );
			}
		}
	}

	return ( qfalse );
}

// Explicitly shutdown a camera and update global list
void G_smvRemoveEntityInMVList( gentity_t *ent, mview_t *ref )
{
	ref->entID = -1;
	ref->fActive = qfalse;
	G_FreeEntity( ref->camera );
	ref->camera = NULL;
	ent->client->pers.mvCount--;

	G_smvUpdateClientCSList( ent );
}

// Calculates a given client's MV player list
unsigned int G_smvGenerateClientList( gentity_t *ent )
{
	unsigned int i, mClients = 0;

	for( i = 0; i < MULTIVIEW_MAXVIEWS; i++ )
	{
		if( ent->client->pers.mv[ i ].fActive )
		{
			mClients |= 1 << ent->client->pers.mv[ i ].entID;
		}
	}

	return ( mClients );
}

// Calculates a given client's MV player list
void G_smvRegenerateClients( gentity_t *ent, int clientList )
{
	int i;

	for( i = 0; i < MAX_MVCLIENTS; i++ )
	{
		if( clientList & ( 1 << i ) )
		{
			G_smvAddView( ent, i );
		}
	}
}

// Update global MV list for a given client
void G_smvUpdateClientCSList( gentity_t *ent )
{
	ent->client->ps.powerups[ PW_MVCLIENTLIST ] = G_smvGenerateClientList( ent );
}

// Remove all clients from a team we can't watch (due to updated speclock)
// or if the viewer enters the game
void G_smvRemoveInvalidClients( gentity_t *ent, int nTeam )
{
	int i, id;

	for( i = 0; i < level.numConnectedClients; i++ )
	{
		id = level.sortedClients[ i ];

		if( level.clients[ id ].sess.sessionTeam == TEAM_SPECTATOR )
		{
			continue;
		}

		if( level.clients[ id ].sess.sessionTeam != nTeam && ent->client->sess.sessionTeam == TEAM_SPECTATOR )
		{
			continue;
		}

		G_smvLocateEntityInMVList( ent, id, qtrue );
	}
}

// Scan all MV lists for a single client to remove
void G_smvAllRemoveSingleClient( int pID )
{
	int       i;
	gentity_t *ent;

	for( i = 0; i < level.numConnectedClients; i++ )
	{
		ent = g_entities + level.sortedClients[ i ];

		if( ent->client->pers.mvCount < 1 )
		{
			continue;
		}

		G_smvLocateEntityInMVList( ent, pID, qtrue );
	}
}

// Set up snapshot merge based on this portal
qboolean G_smvRunCamera( gentity_t *ent )
{
	int           id = ent->TargetFlag;
	int           chargeTime, sprintTime, hintTime, weapHeat;
	playerState_t *tps, *ps;

	// Opt out if not a real MV portal
	if( ent->tagParent == NULL || ent->tagParent->client == NULL )
	{
		return ( qfalse );
	}

	if( ( ps = &ent->tagParent->client->ps ) == NULL )
	{
		return ( qfalse );
	}

	// If viewing client is no longer connected, delete this camera
	if( ent->tagParent->client->pers.connected != CON_CONNECTED )
	{
		G_FreeEntity( ent );
		return ( qtrue );
	}

	// Also remove if the target player is no longer in the game playing
	if( ent->target_ent->client->pers.connected != CON_CONNECTED || ent->target_ent->client->sess.sessionTeam == TEAM_SPECTATOR )
	{
		G_smvLocateEntityInMVList( ent->tagParent, ent->target_ent - g_entities, qtrue );
		return ( qtrue );
	}

	// Seems that depending on player's state, we pull from either r.currentOrigin, or s.origin
	//      if(!spec) then use: r.currentOrigin
	//      if(spec) then use:  s.origin
	//
	// This is true for both the portal origin and its target (origin2)
	//
	VectorCopy( ent->tagParent->s.origin, ent->s.origin );
	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->target_ent->r.currentOrigin, ent->s.origin2 );
	trap_LinkEntity( ent );

	// Only allow client ids 0 to (MAX_MVCLIENTS-1) to be updated with extra info
	if( id >= MAX_MVCLIENTS )
	{
		return ( qtrue );
	}

	tps = &ent->target_ent->client->ps;

	if( tps->stats[ STAT_PLAYER_CLASS ] == PC_ENGINEER )
	{
		chargeTime = g_engineerChargeTime.value;
	}
	else if( tps->stats[ STAT_PLAYER_CLASS ] == PC_MEDIC )
	{
		chargeTime = g_medicChargeTime.value;
	}
	else if( tps->stats[ STAT_PLAYER_CLASS ] == PC_FIELDOPS )
	{
		chargeTime = g_LTChargeTime.value;
	}
	else if( tps->stats[ STAT_PLAYER_CLASS ] == PC_COVERTOPS )
	{
		chargeTime = g_covertopsChargeTime.value;
	}
	else
	{
		chargeTime = g_soldierChargeTime.value;
	}

	chargeTime =
	  ( level.time - tps->classWeaponTime >=
	    ( int ) chargeTime ) ? 0 : ( 1 + floor( 15.0f * ( float )( level.time - tps->classWeaponTime ) / chargeTime ) );
	sprintTime =
	  ( ent->target_ent->client->pmext.sprintTime >=
	    20000 ) ? 0.0f : ( 1 + floor( 7.0f * ( float ) ent->target_ent->client->pmext.sprintTime / 20000.0f ) );
	weapHeat = floor( ( float ) tps->curWeapHeat * 15.0f / 255.0f );
	hintTime = ( tps->serverCursorHint != HINT_BUILD && ( tps->serverCursorHintVal >= 255 || tps->serverCursorHintVal == 0 ) ) ?
	           0 : ( 1 + floor( 15.0f * ( float ) tps->serverCursorHintVal / 255.0f ) );

	// (Remaining bits)
	// ammo      : 0
	// ammo-1    : 0
	// ammiclip  : 0
	// ammoclip-1: 16
	id = MAX_WEAPONS - 1 - ( id * 2 );

	if( tps->pm_flags & PMF_LIMBO )
	{
		ps->ammo[ id ] = 0;
		ps->ammo[ id - 1 ] = 0;
		ps->ammoclip[ id - 1 ] = 0;
	}
	else
	{
		ps->ammo[ id ] = ( ( ( ent->target_ent->health > 0 ) ? ent->target_ent->health : 0 ) & 0xFF ); // Meds up to 140 :(
		ps->ammo[ id ] |= ( hintTime & 0x0F ) << 8; // 4 bits for work on current item (dynamite, weapon repair, etc.)
		ps->ammo[ id ] |= ( weapHeat & 0x0F ) << 12; // 4 bits for weapon heat info

		ps->ammo[ id - 1 ] = tps->ammo[ BG_FindAmmoForWeapon( tps->weapon ) ] & 0x3FF;  // 11 bits needed to cover 1500 Venom ammo
		ps->ammo[ id - 1 ] |= ( BG_simpleWeaponState( tps->weaponstate ) & 0x03 ) << 11;  // 2 bits for current weapon state
		ps->ammo[ id - 1 ] |= ( ( tps->persistant[ PERS_HWEAPON_USE ] ) ? 1 : 0 ) << 13; // 1 bit for mg42 use
		ps->ammo[ id - 1 ] |= ( BG_simpleHintsCollapse( tps->serverCursorHint, hintTime ) & 0x03 ) << 14;  // 2 bits for cursor hints

//  G_Printf("tps->hint: %d, dr: %d, collapse: %d\n", tps->serverCursorHint, HINT_DOOR_ROTATING, G_simpleHintsCollapse(tps->serverCursorHint, hintTime));

		ps->ammoclip[ id - 1 ] = tps->ammoclip[ BG_FindClipForWeapon( tps->weapon ) ] & 0x1FF;  // 9 bits to cover 500 Venom ammo clip
		ps->ammoclip[ id - 1 ] |= ( chargeTime & 0x0F ) << 9; // 4 bits for weapon charge time
		ps->ammoclip[ id - 1 ] |= ( sprintTime & 0x07 ) << 13; // 3 bits for fatigue
	}

	return ( qtrue );
}
