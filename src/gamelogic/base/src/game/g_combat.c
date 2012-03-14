/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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

#include "g_local.h"

damageRegion_t g_damageRegions[ PCL_NUM_CLASSES ][ MAX_LOCDAMAGE_REGIONS ];
int            g_numDamageRegions[ PCL_NUM_CLASSES ];

armourRegion_t g_armourRegions[ UP_NUM_UPGRADES ][ MAX_ARMOUR_REGIONS ];
int            g_numArmourRegions[ UP_NUM_UPGRADES ];

/*
============
AddScore

Adds score to both the client and his team
============
*/
void AddScore( gentity_t *ent, int score )
{
	if ( !ent->client )
	{
		return;
	}

	// no scoring during pre-match warmup
	if ( level.warmupTime )
	{
		return;
	}

	ent->client->ps.persistant[ PERS_SCORE ] += score;
	CalculateRanks();
}

/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker )
{
	vec3_t dir;

	if ( attacker && attacker != self )
	{
		VectorSubtract( attacker->s.pos.trBase, self->s.pos.trBase, dir );
	}
	else if ( inflictor && inflictor != self )
	{
		VectorSubtract( inflictor->s.pos.trBase, self->s.pos.trBase, dir );
	}
	else
	{
		self->client->ps.stats[ STAT_VIEWLOCK ] = self->s.angles[ YAW ];
		return;
	}

	self->client->ps.stats[ STAT_VIEWLOCK ] = vectoyaw( dir );
}

// these are just for logging, the client prints its own messages
char *modNames[] =
{
	"MOD_UNKNOWN",
	"MOD_SHOTGUN",
	"MOD_BLASTER",
	"MOD_PAINSAW",
	"MOD_MACHINEGUN",
	"MOD_CHAINGUN",
	"MOD_PRIFLE",
	"MOD_MDRIVER",
	"MOD_LASGUN",
	"MOD_LCANNON",
	"MOD_LCANNON_SPLASH",
	"MOD_FLAMER",
	"MOD_FLAMER_SPLASH",
	"MOD_GRENADE",
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",

	"MOD_ABUILDER_CLAW",
	"MOD_LEVEL0_BITE",
	"MOD_LEVEL1_CLAW",
	"MOD_LEVEL1_PCLOUD",
	"MOD_LEVEL3_CLAW",
	"MOD_LEVEL3_POUNCE",
	"MOD_LEVEL3_BOUNCEBALL",
	"MOD_LEVEL2_CLAW",
	"MOD_LEVEL2_ZAP",
	"MOD_LEVEL4_CLAW",
	"MOD_LEVEL4_CHARGE",

	"MOD_SLOWBLOB",
	"MOD_POISON",
	"MOD_SWARM",

	"MOD_HSPAWN",
	"MOD_TESLAGEN",
	"MOD_MGTURRET",
	"MOD_REACTOR",

	"MOD_ASPAWN",
	"MOD_ATUBE",
	"MOD_OVERMIND"
};

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath )
{
	gentity_t *ent;
	int       anim;
	int       killer;
	int       i, j;
	char      *killerName, *obit;
	float     totalDamage = 0.0f;
	gentity_t *player;

	if ( self->client->ps.pm_type == PM_DEAD )
	{
		return;
	}

	if ( level.intermissiontime )
	{
		return;
	}

	// stop any following clients
	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR &&
		     level.clients[ i ].sess.spectatorState == SPECTATOR_FOLLOW &&
		     level.clients[ i ].sess.spectatorClient == self->client->ps.clientNum )
		{
			if ( !G_FollowNewClient( &g_entities[ i ], 1 ) )
			{
				G_StopFollowing( &g_entities[ i ] );
			}
		}
	}

	self->client->ps.pm_type = PM_DEAD;
	self->suicideTime = 0;

	if ( attacker )
	{
		killer = attacker->s.number;

		if ( attacker->client )
		{
			killerName = attacker->client->pers.netname;
		}
		else
		{
			killerName = "<non-client>";
		}
	}
	else
	{
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( killer < 0 || killer >= MAX_CLIENTS )
	{
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( meansOfDeath < 0 || meansOfDeath >= sizeof( modNames ) / sizeof( modNames[ 0 ] ) )
	{
		obit = "<bad obituary>";
	}
	else
	{
		obit = modNames[ meansOfDeath ];
	}

	G_LogPrintf( "Kill: %i %i %i: %s killed %s by %s\n",
	             killer, self->s.number, meansOfDeath, killerName,
	             self->client->pers.netname, obit );

	//TA: close any menus the client has open
	G_CloseMenus( self->client->ps.clientNum );

	//TA: deactivate all upgrades
	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		BG_DeactivateUpgrade( i, self->client->ps.stats );
	}

	// broadcast the death event to everyone
	ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
	ent->r.svFlags = SVF_BROADCAST; // send to everyone

	self->enemy = attacker;

	self->client->ps.persistant[ PERS_KILLED ]++;

	if ( attacker && attacker->client )
	{
		attacker->client->lastkilled_client = self->s.number;

		if ( attacker == self || OnSameTeam( self, attacker ) )
		{
			AddScore( attacker, -1 );

			//punish team kills and suicides
			if ( attacker->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS )
			{
				G_AddCreditToClient( attacker->client, -1, qtrue );
			}
			else if ( attacker->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS )
			{
				G_AddCreditToClient( attacker->client, -ASPAWN_VALUE, qtrue );
			}
		}
		else
		{
			AddScore( attacker, 1 );

			attacker->client->lastKillTime = level.time;
		}
	}
	else if ( attacker->s.eType != ET_BUILDABLE )
	{
		AddScore( self, -1 );
	}

	//total up all the damage done by every client
	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		totalDamage += ( float ) self->credits[ i ];
	}

	// if players did more than DAMAGE_FRACTION_FOR_KILL increment the stage counters
	if ( totalDamage >= ( self->client->ps.stats[ STAT_MAX_HEALTH ] * DAMAGE_FRACTION_FOR_KILL ) )
	{
		if ( self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS )
		{
			trap_Cvar_Set( "g_alienKills", va( "%d", g_alienKills.integer + 1 ) );
		}
		else if ( self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS )
		{
			trap_Cvar_Set( "g_humanKills", va( "%d", g_humanKills.integer + 1 ) );
		}
	}

	if ( totalDamage > 0.0f )
	{
		if ( self->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS )
		{
			//nice simple happy bouncy human land
			float classValue = BG_FindValueOfClass( self->client->ps.stats[ STAT_PCLASS ] );

			for ( i = 0; i < MAX_CLIENTS; i++ )
			{
				player = g_entities + i;

				if ( !player->client )
				{
					continue;
				}

				if ( player->client->ps.stats[ STAT_PTEAM ] != PTE_HUMANS )
				{
					continue;
				}

				if ( !self->credits[ i ] )
				{
					continue;
				}

				//add credit
				G_AddCreditToClient( player->client,
				                     ( int )( classValue * ( ( float ) self->credits[ i ] / totalDamage ) ), qtrue );
			}
		}
		else if ( self->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS )
		{
			//horribly complex nasty alien land
			float humanValue = BG_GetValueOfHuman( &self->client->ps );
			int   frags;
			int   unclaimedFrags = ( int ) humanValue;

			for ( i = 0; i < MAX_CLIENTS; i++ )
			{
				player = g_entities + i;

				if ( !player->client )
				{
					continue;
				}

				if ( player->client->ps.stats[ STAT_PTEAM ] != PTE_ALIENS )
				{
					continue;
				}

				//this client did no damage
				if ( !self->credits[ i ] )
				{
					continue;
				}

				//nothing left to claim
				if ( !unclaimedFrags )
				{
					break;
				}

				frags = ( int ) floor( humanValue * ( ( float ) self->credits[ i ] / totalDamage ) );

				if ( frags > 0 )
				{
					//add kills
					G_AddCreditToClient( player->client, frags, qtrue );

					//can't revist this account later
					self->credits[ i ] = 0;

					//reduce frags left to be claimed
					unclaimedFrags -= frags;
				}
			}

			//there are frags still to be claimed
			if ( unclaimedFrags )
			{
				//the clients remaining at this point do not
				//have enough credit to claim even one frag
				//so simply give the top <unclaimedFrags> clients
				//a frag each

				for ( i = 0; i < unclaimedFrags; i++ )
				{
					int maximum = 0;
					int topClient = 0;

					for ( j = 0; j < MAX_CLIENTS; j++ )
					{
						//this client did no damage
						if ( !self->credits[ j ] )
						{
							continue;
						}

						if ( self->credits[ j ] > maximum )
						{
							maximum = self->credits[ j ];
							topClient = j;
						}
					}

					if ( maximum > 0 )
					{
						player = g_entities + topClient;

						//add kills
						G_AddCreditToClient( player->client, 1, qtrue );

						//can't revist this account again
						self->credits[ topClient ] = 0;
					}
				}
			}
		}
	}

	Cmd_Score_f( self );  // show scores

	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0; i < level.maxclients; i++ )
	{
		gclient_t *client;

		client = &level.clients[ i ];

		if ( client->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if ( client->sess.sessionTeam != TEAM_SPECTATOR )
		{
			continue;
		}

		if ( client->sess.spectatorClient == self->s.number )
		{
			Cmd_Score_f( g_entities + i );
		}
	}

	self->client->pers.classSelection = PCL_NONE; //TA: reset the classtype
	VectorCopy( self->s.origin, self->client->pers.lastDeathLocation );

	self->takedamage = qfalse; // can still be gibbed

	self->s.weapon = WP_NONE;
	self->r.contents = CONTENTS_CORPSE;

	self->s.angles[ PITCH ] = 0;
	self->s.angles[ ROLL ] = 0;
	self->s.angles[ YAW ] = self->s.apos.trBase[ YAW ];
	LookAtKiller( self, inflictor, attacker );

	VectorCopy( self->s.angles, self->client->ps.viewangles );

	self->s.loopSound = 0;

	self->r.maxs[ 2 ] = -8;

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnTime = level.time + 1700;

	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof( self->client->ps.powerups ) );

	{
		// normal death
		static int i;

		if ( !( self->client->ps.persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			switch ( i )
			{
				case 0:
					anim = BOTH_DEATH1;
					break;

				case 1:
					anim = BOTH_DEATH2;
					break;

				case 2:
				default:
					anim = BOTH_DEATH3;
					break;
			}
		}
		else
		{
			switch ( i )
			{
				case 0:
					anim = NSPA_DEATH1;
					break;

				case 1:
					anim = NSPA_DEATH2;
					break;

				case 2:
				default:
					anim = NSPA_DEATH3;
					break;
			}
		}

		self->client->ps.legsAnim =
		  ( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

		if ( !( self->client->ps.persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			self->client->ps.torsoAnim =
			  ( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
		}

		G_AddEvent( self, EV_DEATH1 + i, killer );

		// globally cycle through the different death animations
		i = ( i + 1 ) % 3;
	}

	trap_LinkEntity( self );
}

////////TA: locdamage

/*
===============
G_ParseArmourScript
===============
*/
void G_ParseArmourScript( char *buf, int upgrade )
{
	char *token;
	int  count;

	count = 0;

	while ( 1 )
	{
		token = COM_Parse( &buf );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( strcmp( token, "{" ) )
		{
			G_Printf( "Missing { in armour file\n" );
			break;
		}

		if ( count == MAX_ARMOUR_REGIONS )
		{
			G_Printf( "Max armour regions exceeded in locdamage file\n" );
			break;
		}

		//default
		g_armourRegions[ upgrade ][ count ].minHeight = 0.0;
		g_armourRegions[ upgrade ][ count ].maxHeight = 1.0;
		g_armourRegions[ upgrade ][ count ].minAngle = 0;
		g_armourRegions[ upgrade ][ count ].maxAngle = 360;
		g_armourRegions[ upgrade ][ count ].modifier = 1.0;
		g_armourRegions[ upgrade ][ count ].crouch = qfalse;

		while ( 1 )
		{
			token = COM_ParseExt( &buf, qtrue );

			if ( !token[ 0 ] )
			{
				G_Printf( "Unexpected end of armour file\n" );
				break;
			}

			if ( !Q_stricmp( token, "}" ) )
			{
				break;
			}
			else if ( !strcmp( token, "minHeight" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "0" );
				}

				g_armourRegions[ upgrade ][ count ].minHeight = atof( token );
			}
			else if ( !strcmp( token, "maxHeight" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "100" );
				}

				g_armourRegions[ upgrade ][ count ].maxHeight = atof( token );
			}
			else if ( !strcmp( token, "minAngle" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "0" );
				}

				g_armourRegions[ upgrade ][ count ].minAngle = atoi( token );
			}
			else if ( !strcmp( token, "maxAngle" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "360" );
				}

				g_armourRegions[ upgrade ][ count ].maxAngle = atoi( token );
			}
			else if ( !strcmp( token, "modifier" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "1.0" );
				}

				g_armourRegions[ upgrade ][ count ].modifier = atof( token );
			}
			else if ( !strcmp( token, "crouch" ) )
			{
				g_armourRegions[ upgrade ][ count ].crouch = qtrue;
			}
		}

		g_numArmourRegions[ upgrade ]++;
		count++;
	}
}

/*
===============
G_ParseDmgScript
===============
*/
void G_ParseDmgScript( char *buf, int class )
{
	char *token;
	int  count;

	count = 0;

	while ( 1 )
	{
		token = COM_Parse( &buf );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( strcmp( token, "{" ) )
		{
			G_Printf( "Missing { in locdamage file\n" );
			break;
		}

		if ( count == MAX_LOCDAMAGE_REGIONS )
		{
			G_Printf( "Max damage regions exceeded in locdamage file\n" );
			break;
		}

		//default
		g_damageRegions[ class ][ count ].minHeight = 0.0;
		g_damageRegions[ class ][ count ].maxHeight = 1.0;
		g_damageRegions[ class ][ count ].minAngle = 0;
		g_damageRegions[ class ][ count ].maxAngle = 360;
		g_damageRegions[ class ][ count ].modifier = 1.0;
		g_damageRegions[ class ][ count ].crouch = qfalse;

		while ( 1 )
		{
			token = COM_ParseExt( &buf, qtrue );

			if ( !token[ 0 ] )
			{
				G_Printf( "Unexpected end of locdamage file\n" );
				break;
			}

			if ( !Q_stricmp( token, "}" ) )
			{
				break;
			}
			else if ( !strcmp( token, "minHeight" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "0" );
				}

				g_damageRegions[ class ][ count ].minHeight = atof( token );
			}
			else if ( !strcmp( token, "maxHeight" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "100" );
				}

				g_damageRegions[ class ][ count ].maxHeight = atof( token );
			}
			else if ( !strcmp( token, "minAngle" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "0" );
				}

				g_damageRegions[ class ][ count ].minAngle = atoi( token );
			}
			else if ( !strcmp( token, "maxAngle" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "360" );
				}

				g_damageRegions[ class ][ count ].maxAngle = atoi( token );
			}
			else if ( !strcmp( token, "modifier" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "1.0" );
				}

				g_damageRegions[ class ][ count ].modifier = atof( token );
			}
			else if ( !strcmp( token, "crouch" ) )
			{
				g_damageRegions[ class ][ count ].crouch = qtrue;
			}
		}

		g_numDamageRegions[ class ]++;
		count++;
	}
}

/*
============
G_CalcDamageModifier
============
*/
static float G_CalcDamageModifier( vec3_t point, gentity_t *targ, gentity_t *attacker, int class, int dflags )
{
	vec3_t bulletPath;
	vec3_t bulletAngle;
	vec3_t pMINUSfloor, floor, normal;

	float  clientHeight, hitRelative, hitRatio;
	int    bulletRotation, clientRotation, hitRotation;
	float  modifier = 1.0f;
	int    i, j;

	if ( point == NULL )
	{
		return 1.0f;
	}

	clientHeight = targ->r.maxs[ 2 ] - targ->r.mins[ 2 ];

	if ( targ->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING )
	{
		VectorCopy( targ->client->ps.grapplePoint, normal );
	}
	else
	{
		VectorSet( normal, 0, 0, 1 );
	}

	VectorMA( targ->r.currentOrigin, targ->r.mins[ 2 ], normal, floor );
	VectorSubtract( point, floor, pMINUSfloor );

	hitRelative = DotProduct( normal, pMINUSfloor ) / VectorLength( normal );

	if ( hitRelative < 0.0f )
	{
		hitRelative = 0.0f;
	}

	if ( hitRelative > clientHeight )
	{
		hitRelative = clientHeight;
	}

	hitRatio = hitRelative / clientHeight;

	VectorSubtract( targ->r.currentOrigin, point, bulletPath );
	vectoangles( bulletPath, bulletAngle );

	clientRotation = targ->client->ps.viewangles[ YAW ];
	bulletRotation = bulletAngle[ YAW ];

	hitRotation = abs( clientRotation - bulletRotation );

	hitRotation = hitRotation % 360; // Keep it in the 0-359 range

	if ( dflags & DAMAGE_NO_LOCDAMAGE )
	{
		for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
		{
			float totalModifier = 0.0f;
			float averageModifier = 1.0f;

			//average all of this upgrade's armour regions together
			if ( BG_InventoryContainsUpgrade( i, targ->client->ps.stats ) )
			{
				for ( j = 0; j < g_numArmourRegions[ i ]; j++ )
				{
					totalModifier += g_armourRegions[ i ][ j ].modifier;
				}

				if ( g_numArmourRegions[ i ] )
				{
					averageModifier = totalModifier / g_numArmourRegions[ i ];
				}
				else
				{
					averageModifier = 1.0f;
				}
			}

			modifier *= averageModifier;
		}
	}
	else
	{
		for ( i = 0; i < g_numDamageRegions[ class ]; i++ )
		{
			qboolean rotationBound;

			if ( g_damageRegions[ class ][ i ].minAngle >
			     g_damageRegions[ class ][ i ].maxAngle )
			{
				rotationBound = ( hitRotation >= g_damageRegions[ class ][ i ].minAngle &&
				                  hitRotation <= 360 ) || ( hitRotation >= 0 &&
				                      hitRotation <= g_damageRegions[ class ][ i ].maxAngle );
			}
			else
			{
				rotationBound = ( hitRotation >= g_damageRegions[ class ][ i ].minAngle &&
				                  hitRotation <= g_damageRegions[ class ][ i ].maxAngle );
			}

			if ( rotationBound &&
			     hitRatio >= g_damageRegions[ class ][ i ].minHeight &&
			     hitRatio <= g_damageRegions[ class ][ i ].maxHeight &&
			     ( g_damageRegions[ class ][ i ].crouch ==
			       ( targ->client->ps.pm_flags & PMF_DUCKED ) ) )
			{
				modifier *= g_damageRegions[ class ][ i ].modifier;
			}
		}

		for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
		{
			if ( BG_InventoryContainsUpgrade( i, targ->client->ps.stats ) )
			{
				for ( j = 0; j < g_numArmourRegions[ i ]; j++ )
				{
					qboolean rotationBound;

					if ( g_armourRegions[ i ][ j ].minAngle >
					     g_armourRegions[ i ][ j ].maxAngle )
					{
						rotationBound = ( hitRotation >= g_armourRegions[ i ][ j ].minAngle &&
						                  hitRotation <= 360 ) || ( hitRotation >= 0 &&
						                      hitRotation <= g_armourRegions[ i ][ j ].maxAngle );
					}
					else
					{
						rotationBound = ( hitRotation >= g_armourRegions[ i ][ j ].minAngle &&
						                  hitRotation <= g_armourRegions[ i ][ j ].maxAngle );
					}

					if ( rotationBound &&
					     hitRatio >= g_armourRegions[ i ][ j ].minHeight &&
					     hitRatio <= g_armourRegions[ i ][ j ].maxHeight &&
					     ( g_armourRegions[ i ][ j ].crouch ==
					       ( targ->client->ps.pm_flags & PMF_DUCKED ) ) )
					{
						modifier *= g_armourRegions[ i ][ j ].modifier;
					}
				}
			}
		}
	}

	return modifier;
}

/*
============
G_InitDamageLocations
============
*/
void G_InitDamageLocations( void )
{
	char         *modelName;
	char         filename[ MAX_QPATH ];
	int          i;
	int          len;
	fileHandle_t fileHandle;
	char         buffer[ MAX_LOCDAMAGE_TEXT ];

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		modelName = BG_FindModelNameForClass( i );
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/locdamage.cfg", modelName );

		len = trap_FS_FOpenFile( filename, &fileHandle, FS_READ );

		if ( !fileHandle )
		{
			G_Printf( S_COLOR_RED "file not found: %s\n", filename );
			continue;
		}

		if ( len >= MAX_LOCDAMAGE_TEXT )
		{
			G_Printf( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_LOCDAMAGE_TEXT );
			trap_FS_FCloseFile( fileHandle );
			continue;
		}

		trap_FS_Read( buffer, len, fileHandle );
		buffer[ len ] = 0;
		trap_FS_FCloseFile( fileHandle );

		G_ParseDmgScript( buffer, i );
	}

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		modelName = BG_FindNameForUpgrade( i );
		Com_sprintf( filename, sizeof( filename ), "armour/%s.armour", modelName );

		len = trap_FS_FOpenFile( filename, &fileHandle, FS_READ );

		//no file - no parsage
		if ( !fileHandle )
		{
			continue;
		}

		if ( len >= MAX_LOCDAMAGE_TEXT )
		{
			G_Printf( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_LOCDAMAGE_TEXT );
			trap_FS_FCloseFile( fileHandle );
			continue;
		}

		trap_FS_Read( buffer, len, fileHandle );
		buffer[ len ] = 0;
		trap_FS_FCloseFile( fileHandle );

		G_ParseArmourScript( buffer, i );
	}
}

////////TA: locdamage

/*
============
T_Damage

targ    entity that is being damaged
inflictor entity that is causing the damage
attacker  entity that caused the inflictor to damage targ
  example: targ=monster, inflictor=rocket, attacker=player

dir     direction of the attack for knockback
point   point at which the damage is being inflicted, used for headshots
damage    amount of damage being inflicted
knockback force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags    these flags are used to control how T_Damage works
  DAMAGE_RADIUS     damage was indirect (from a nearby explosion)
  DAMAGE_NO_ARMOR     armor does not protect from this damage
  DAMAGE_NO_KNOCKBACK   do not affect velocity, just view angles
  DAMAGE_NO_PROTECTION  kills godmode, armor, everything
============
*/

//TA: team is the team that is immune to this damage
void G_SelectiveDamage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
                        vec3_t dir, vec3_t point, int damage, int dflags, int mod, int team )
{
	if ( targ->client && ( team != targ->client->ps.stats[ STAT_PTEAM ] ) )
	{
		G_Damage( targ, inflictor, attacker, dir, point, damage, dflags, mod );
	}
}

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
               vec3_t dir, vec3_t point, int damage, int dflags, int mod )
{
	gclient_t *client;
	int       take;
	int       save;
	int       asave = 0;
	int       knockback;

	if ( !targ->takedamage )
	{
		return;
	}

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued )
	{
		return;
	}

	if ( !inflictor )
	{
		inflictor = &g_entities[ ENTITYNUM_WORLD ];
	}

	if ( !attacker )
	{
		attacker = &g_entities[ ENTITYNUM_WORLD ];
	}

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER )
	{
		if ( targ->use && ( targ->moverState == MOVER_POS1 ||
		                    targ->moverState == ROTATOR_POS1 ) )
		{
			targ->use( targ, inflictor, attacker );
		}

		return;
	}

	client = targ->client;

	if ( client )
	{
		if ( client->noclip )
		{
			return;
		}
	}

	if ( !dir )
	{
		dflags |= DAMAGE_NO_KNOCKBACK;
	}
	else
	{
		VectorNormalize( dir );
	}

	knockback = damage;

	// silly hack to give norf his knockbacking teslas
	if ( !strcmp( inflictor->classname, "team_human_tesla" ) )
	{
		knockback *= 4;
	}

	// ...and for goon pouncing
	if ( mod == MOD_LEVEL3_POUNCE )
	{
		knockback *= 3;
	}

	if ( targ->client )
	{
		knockback = ( int )( ( float ) knockback *
		                     BG_FindKnockbackScaleForClass( targ->client->ps.stats[ STAT_PCLASS ] ) );
	}

	if ( knockback > 200 )
	{
		knockback = 200;
	}

	if ( targ->flags & FL_NO_KNOCKBACK )
	{
		knockback = 0;
	}

	if ( dflags & DAMAGE_NO_KNOCKBACK )
	{
		knockback = 0;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client )
	{
		vec3_t kvel;
		float  mass;

		mass = 200;

		VectorScale( dir, g_knockback.value * ( float ) knockback / mass, kvel );
		VectorAdd( targ->client->ps.velocity, kvel, targ->client->ps.velocity );

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if ( !targ->client->ps.pm_time )
		{
			int t;

			t = knockback * 2;

			if ( t < 50 )
			{
				t = 50;
			}

			if ( t > 200 )
			{
				t = 200;
			}

			targ->client->ps.pm_time = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	// check for completely getting out of the damage
	if ( !( dflags & DAMAGE_NO_PROTECTION ) )
	{
		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if ( targ != attacker && OnSameTeam( targ, attacker ) )
		{
			if ( !g_friendlyFire.integer )
			{
				return;
			}
		}

		// check for godmode
		if ( targ->flags & FL_GODMODE )
		{
			return;
		}
	}

	// add to the attacker's hit counter
	if ( attacker->client && targ != attacker && targ->health > 0
	     && targ->s.eType != ET_MISSILE
	     && targ->s.eType != ET_GENERAL )
	{
		if ( OnSameTeam( targ, attacker ) )
		{
			attacker->client->ps.persistant[ PERS_HITS ]--;
		}
		else
		{
			attacker->client->ps.persistant[ PERS_HITS ]++;
		}
	}

	take = damage;
	save = 0;

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( client )
	{
		if ( attacker )
		{
			client->ps.persistant[ PERS_ATTACKER ] = attacker->s.number;
		}
		else
		{
			client->ps.persistant[ PERS_ATTACKER ] = ENTITYNUM_WORLD;
		}

		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;

		if ( dir )
		{
			VectorCopy( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		}
		else
		{
			VectorCopy( targ->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}

		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
		take = ( int )( ( float ) take * G_CalcDamageModifier( point, targ, attacker,
		                client->ps.stats[ STAT_PCLASS ], dflags ) );

		//if boosted poison every attack
		if ( attacker->client && attacker->client->ps.stats[ STAT_STATE ] & SS_BOOSTED )
		{
			if ( !( targ->client->ps.stats[ STAT_STATE ] & SS_POISONED ) &&
			     !BG_InventoryContainsUpgrade( UP_BATTLESUIT, targ->client->ps.stats ) &&
			     mod != MOD_LEVEL2_ZAP &&
			     targ->client->poisonImmunityTime < level.time )
			{
				targ->client->ps.stats[ STAT_STATE ] |= SS_POISONED;
				targ->client->lastPoisonTime = level.time;
				targ->client->lastPoisonClient = attacker;
			}
		}
	}

	if ( take < 1 )
	{
		take = 1;
	}

	if ( g_debugDamage.integer )
	{
		G_Printf( "%i: client:%i health:%i damage:%i armor:%i\n", level.time, targ->s.number,
		          targ->health, take, asave );
	}

	// do the damage
	if ( take )
	{
		targ->health = targ->health - take;

		if ( targ->client )
		{
			targ->client->ps.stats[ STAT_HEALTH ] = targ->health;
		}

		targ->lastDamageTime = level.time;

		//TA: add to the attackers "account" on the target
		if ( targ->client && attacker->client )
		{
			if ( attacker != targ && !OnSameTeam( targ, attacker ) )
			{
				targ->credits[ attacker->client->ps.clientNum ] += take;
			}
		}

		if ( targ->health <= 0 )
		{
			if ( client )
			{
				targ->flags |= FL_NO_KNOCKBACK;
			}

			if ( targ->health < -999 )
			{
				targ->health = -999;
			}

			targ->enemy = attacker;
			targ->die( targ, inflictor, attacker, take, mod );
			return;
		}
		else if ( targ->pain )
		{
			targ->pain( targ, attacker, take );
		}
	}
}

/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage( gentity_t *targ, vec3_t origin )
{
	vec3_t  dest;
	trace_t tr;
	vec3_t  midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd( targ->r.absmin, targ->r.absmax, midpoint );
	VectorScale( midpoint, 0.5, midpoint );

	VectorCopy( midpoint, dest );
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );

	if ( tr.fraction == 1.0  || tr.entityNum == targ->s.number )
	{
		return qtrue;
	}

	// this should probably check in the plane of projection,
	// rather than in world coordinate, and also include Z
	VectorCopy( midpoint, dest );
	dest[ 0 ] += 15.0;
	dest[ 1 ] += 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );

	if ( tr.fraction == 1.0 )
	{
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[ 0 ] += 15.0;
	dest[ 1 ] -= 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );

	if ( tr.fraction == 1.0 )
	{
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[ 0 ] -= 15.0;
	dest[ 1 ] += 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );

	if ( tr.fraction == 1.0 )
	{
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[ 0 ] -= 15.0;
	dest[ 1 ] -= 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );

	if ( tr.fraction == 1.0 )
	{
		return qtrue;
	}

	return qfalse;
}

//TA:

/*
============
G_SelectiveRadiusDamage
============
*/
qboolean G_SelectiveRadiusDamage( vec3_t origin, gentity_t *attacker, float damage,
                                  float radius, gentity_t *ignore, int mod, int team )
{
	float     points, dist;
	gentity_t *ent;
	int       entityList[ MAX_GENTITIES ];
	int       numListedEntities;
	vec3_t    mins, maxs;
	vec3_t    v;
	vec3_t    dir;
	int       i, e;
	qboolean  hitClient = qfalse;

	if ( radius < 1 )
	{
		radius = 1;
	}

	for ( i = 0; i < 3; i++ )
	{
		mins[ i ] = origin[ i ] - radius;
		maxs[ i ] = origin[ i ] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0; e < numListedEntities; e++ )
	{
		ent = &g_entities[ entityList[ e ] ];

		if ( ent == ignore )
		{
			continue;
		}

		if ( !ent->takedamage )
		{
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0; i < 3; i++ )
		{
			if ( origin[ i ] < ent->r.absmin[ i ] )
			{
				v[ i ] = ent->r.absmin[ i ] - origin[ i ];
			}
			else if ( origin[ i ] > ent->r.absmax[ i ] )
			{
				v[ i ] = origin[ i ] - ent->r.absmax[ i ];
			}
			else
			{
				v[ i ] = 0;
			}
		}

		dist = VectorLength( v );

		if ( dist >= radius )
		{
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if ( CanDamage( ent, origin ) )
		{
			VectorSubtract( ent->r.currentOrigin, origin, dir );
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[ 2 ] += 24;
			G_SelectiveDamage( ent, NULL, attacker, dir, origin,
			                   ( int ) points, DAMAGE_RADIUS | DAMAGE_NO_LOCDAMAGE, mod, team );
		}
	}

	return hitClient;
}

/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage,
                         float radius, gentity_t *ignore, int mod )
{
	float     points, dist;
	gentity_t *ent;
	int       entityList[ MAX_GENTITIES ];
	int       numListedEntities;
	vec3_t    mins, maxs;
	vec3_t    v;
	vec3_t    dir;
	int       i, e;
	qboolean  hitClient = qfalse;

	if ( radius < 1 )
	{
		radius = 1;
	}

	for ( i = 0; i < 3; i++ )
	{
		mins[ i ] = origin[ i ] - radius;
		maxs[ i ] = origin[ i ] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0; e < numListedEntities; e++ )
	{
		ent = &g_entities[ entityList[ e ] ];

		if ( ent == ignore )
		{
			continue;
		}

		if ( !ent->takedamage )
		{
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0; i < 3; i++ )
		{
			if ( origin[ i ] < ent->r.absmin[ i ] )
			{
				v[ i ] = ent->r.absmin[ i ] - origin[ i ];
			}
			else if ( origin[ i ] > ent->r.absmax[ i ] )
			{
				v[ i ] = origin[ i ] - ent->r.absmax[ i ];
			}
			else
			{
				v[ i ] = 0;
			}
		}

		dist = VectorLength( v );

		if ( dist >= radius )
		{
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if ( CanDamage( ent, origin ) )
		{
			VectorSubtract( ent->r.currentOrigin, origin, dir );
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[ 2 ] += 24;
			G_Damage( ent, NULL, attacker, dir, origin,
			          ( int ) points, DAMAGE_RADIUS | DAMAGE_NO_LOCDAMAGE, mod );
		}
	}

	return hitClient;
}
