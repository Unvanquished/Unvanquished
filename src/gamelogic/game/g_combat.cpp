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

#include "g_local.h"

#define MAX_DAMAGE_REGION_TEXT 8192
#define MAX_DAMAGE_REGIONS     16

// damage region data
damageRegion_t g_damageRegions[ PCL_NUM_CLASSES ][ MAX_DAMAGE_REGIONS ];
int            g_numDamageRegions[ PCL_NUM_CLASSES ];

// these are just for logging, the client prints its own messages
// TODO: Centralize to keep in sync (e.g. bg_mod.c)
static const char *const modNames[] =
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
	"MOD_BURN",
	"MOD_GRENADE",
	"MOD_FIREBOMB",
	"MOD_WEIGHT_H",
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
	"MOD_LEVEL3_CLAW",
	"MOD_LEVEL3_POUNCE",
	"MOD_LEVEL3_BOUNCEBALL",
	"MOD_LEVEL2_CLAW",
	"MOD_LEVEL2_ZAP",
	"MOD_LEVEL4_CLAW",
	"MOD_LEVEL4_TRAMPLE",
	"MOD_WEIGHT_A",

	"MOD_SLOWBLOB",
	"MOD_POISON",
	"MOD_SWARM",

	"MOD_HSPAWN",
	"MOD_TESLAGEN",
	"MOD_MGTURRET",
	"MOD_REACTOR",

	"MOD_ASPAWN",
	"MOD_ATUBE",
	"MOD_OVERMIND",
	"MOD_DECONSTRUCT",
	"MOD_REPLACE",
	"MOD_NOCREEP"
};

/**
 * @brief Helper function for G_AddCreditsToScore and G_AddMomentumToScore.
 * @param self
 * @param score
 */
static void AddScoreHelper( gentity_t *self, float score )
{
	if ( !self->client || self->client->pers.connected != CON_CONNECTED )
	{
		return;
	}

	self->client->ps.persistant[ PERS_SCORE ] += ( int )( score + 0.5f );

	CalculateRanks();
}

/**
 * @brief Adds score to the client, input represents a credit value.
 * @param self
 * @param credits
 */
void G_AddCreditsToScore( gentity_t *self, int credits )
{
	AddScoreHelper( self, credits * SCORE_PER_CREDIT );
}

/**
 * @brief Adds score to the client, input represents a momentum value.
 * @param self
 * @param momentum
 */
void G_AddMomentumToScore( gentity_t *self, float momentum )
{
	AddScoreHelper( self, momentum * SCORE_PER_MOMENTUM );
}

void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker )
{
	if ( attacker && attacker != self )
	{
		self->client->ps.stats[ STAT_VIEWLOCK ] = attacker - g_entities;
	}
	else if ( inflictor && inflictor != self )
	{
		self->client->ps.stats[ STAT_VIEWLOCK ] = inflictor - g_entities;
	}
	else
	{
		self->client->ps.stats[ STAT_VIEWLOCK ] = self - g_entities;
	}
}

/**
 * @brief Function to find who assisted most (and, in case of a tie, most recently) with a kill
 * @param self
 * @param killer
 */
static const gentity_t *G_FindKillAssist( const gentity_t *self, const gentity_t *killer, team_t *team )
{
	const gentity_t *assistant = NULL;
	float           damage;
	int             when = 0;
	int             playerNum;

	// Suicide? No assistance needed with that
	if ( killer == self)
	{
		return NULL;
	}

	// Require that the assist was for, at least, 25% of the damage or
	// as much damage as the killer did, whichever is lower
	damage = self->client->ps.stats[ STAT_MAX_HEALTH ] / 4.0f;
	if ( killer )
	{
		damage = MIN( damage, self->credits[ killer - g_entities ].value );
	}

	for ( playerNum = 0; playerNum < level.maxclients; ++playerNum )
	{
		const gentity_t *player = &g_entities[ playerNum ];

		if ( player == killer || player == self || self->credits[ playerNum ].team <= TEAM_NONE )
		{
			continue;
		}

		if ( self->credits[ playerNum ].value > damage ||
		     (self->credits[ playerNum ].value == damage && self->credits[ playerNum ].time > when ) )
		{
			assistant = player;
			damage = self->credits[ playerNum ].value;
			when = self->credits[ playerNum ].time;
			*team = self->credits[ playerNum ].team;
		}
	}

	return assistant;
}

/**
 * @brief Function to distribute rewards to entities that killed this one.
 * @param self
 */
void G_RewardAttackers( gentity_t *self )
{
	float     value, share, reward, enemyDamage, damageShare;
	int       playerNum, maxHealth;
	gentity_t *player;
	team_t    ownTeam, playerTeam;

	// Only reward killing players and buildables
	if ( self->client )
	{
		ownTeam   = (team_t) self->client->pers.team;
		maxHealth = self->client->ps.stats[ STAT_MAX_HEALTH ];
		value     = BG_GetValueOfPlayer( &self->client->ps );
	}
	else if ( self->s.eType == ET_BUILDABLE )
	{
		ownTeam   = (team_t) self->buildableTeam;
		maxHealth = BG_Buildable( self->s.modelindex )->health;
		value     = BG_Buildable( self->s.modelindex )->value;

		// Give partial credits for buildables in construction
		if ( !self->spawned )
		{
			value *= ( level.time - self->creationTime ) /
			         ( float )BG_Buildable( self->s.modelindex )->buildTime;
		}
	}
	else
	{
		return;
	}

	// Sum up damage dealt by enemies
	enemyDamage = 0.0f;

	for ( playerNum = 0; playerNum < level.maxclients; playerNum++ )
	{
		player     = &g_entities[ playerNum ];
		playerTeam = (team_t) player->client->pers.team;

		// Player must be on the other team
		if ( playerTeam == ownTeam || playerTeam <= TEAM_NONE || playerTeam >= NUM_TEAMS )
		{
			continue;
		}

		enemyDamage += self->credits[ playerNum ].value;
	}

	if ( enemyDamage <= 0 )
	{
		return;
	}

	// Give individual rewards
	for ( playerNum = 0; playerNum < level.maxclients; playerNum++ )
	{
		player      = &g_entities[ playerNum ];
		playerTeam  = (team_t) player->client->pers.team;
		damageShare = self->credits[ playerNum ].value;

		// Clear reward array
		self->credits[ playerNum ].value = 0.0f;

		// Player must be on the other team
		if ( playerTeam == ownTeam || playerTeam <= TEAM_NONE || playerTeam >= NUM_TEAMS )
		{
			continue;
		}

		// Player must have dealt damage
		if ( damageShare <= 0.0f )
		{
			continue;
		}

		share  = damageShare / ( float )maxHealth;
		reward = value * share;

		if ( self->s.eType == ET_BUILDABLE )
		{
			// Add score
			G_AddMomentumToScore( player, reward );

			// Add momentum
			G_AddMomentumForDestroyingStep( self, player, share );
		}
		else
		{
			// Add score
			G_AddCreditsToScore( player, ( int )reward );

			// Add credits
			G_AddCreditToClient( player->client, ( short )reward, qtrue );

			// Add momentum
			G_AddMomentumForKillingStep( self, player, share );
		}
	}

	// Complete momentum modification
	G_AddMomentumEnd();
}

void G_PlayerDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int meansOfDeath )
{
	gentity_t *ent;
	int       anim;
	int       killer;
	int       i;
	const char *killerName, *obit;

	const gentity_t *assistantEnt;
	int             assistant = ENTITYNUM_NONE;
	const char      *assistantName = NULL;
	team_t          assistantTeam = TEAM_NONE;

	if ( self->client->ps.pm_type == PM_DEAD )
	{
		return;
	}

	if ( level.intermissiontime )
	{
		return;
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
			killerName = "<world>";
		}
	}
	else
	{
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	assistantEnt = G_FindKillAssist( self, attacker, &assistantTeam );

	if ( assistantEnt )
	{
		assistant = assistantEnt->s.number;

		if ( assistantEnt->client )
		{
			assistantName = assistantEnt->client->pers.netname;
		}
	}

	if ( meansOfDeath < 0 || meansOfDeath >= ARRAY_LEN( modNames ) )
	{
		// fall back on the number
		obit = va( "%d", meansOfDeath );
	}
	else
	{
		obit = modNames[ meansOfDeath ];
	}

	if ( assistant != ENTITYNUM_NONE )
	{
		G_LogPrintf( "Die: %d %d %s %d %d: %s" S_COLOR_WHITE " killed %s" S_COLOR_WHITE "; %s" S_COLOR_WHITE " assisted\n",
		             killer,
		             ( int )( self - g_entities ),
		             obit,
		             assistant,
		             assistantTeam,
			     killerName,
			     self->client->pers.netname,
			     assistantName );
	}
	else
	{
		G_LogPrintf( "Die: %d %d %s: %s" S_COLOR_WHITE " killed %s\n",
		             killer,
		             ( int )( self - g_entities ),
		             obit,
		             killerName,
		             self->client->pers.netname );
	}

	// deactivate all upgrades
	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		BG_DeactivateUpgrade( i, self->client->ps.stats );
	}

	// broadcast the death event to everyone
	ent = G_NewTempEntity( self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
	ent->s.otherEntityNum3 = assistant;
	ent->s.generic1 = assistantTeam;
	ent->r.svFlags = SVF_BROADCAST; // send to everyone

	if ( attacker && attacker->client )
	{
		if ( ( attacker == self || G_OnSameTeam( self, attacker ) ) )
		{
			//punish team kills and suicides
			if ( attacker->client->pers.team == TEAM_ALIENS )
			{
				G_AddCreditToClient( attacker->client, -ALIEN_TK_SUICIDE_PENALTY, qtrue );
				G_AddCreditsToScore( attacker, -ALIEN_TK_SUICIDE_PENALTY );
			}
			else if ( attacker->client->pers.team == TEAM_HUMANS )
			{
				G_AddCreditToClient( attacker->client, -HUMAN_TK_SUICIDE_PENALTY, qtrue );
				G_AddCreditsToScore( attacker, -HUMAN_TK_SUICIDE_PENALTY );
			}
		}
		else if ( g_showKillerHP.integer )
		{
			trap_SendServerCommand( self - g_entities, va( "print_tr %s %s %3i", QQ( N_("Your killer, $1$^7, had $2$ HP.\n") ),
			                        Quote( killerName ),
			                        attacker->health ) );
		}
	}
	else if ( attacker->s.eType != ET_BUILDABLE )
	{
		if ( self->client->pers.team == TEAM_ALIENS )
		{
			G_AddCreditsToScore( self, -ALIEN_TK_SUICIDE_PENALTY );
		}
		else if ( self->client->pers.team == TEAM_HUMANS )
		{
			G_AddCreditsToScore( self, -HUMAN_TK_SUICIDE_PENALTY );
		}
	}

	// give credits for killing this player
	G_RewardAttackers( self );

	ScoreboardMessage( self );  // show scores

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

		if ( client->sess.spectatorState == SPECTATOR_NOT )
		{
			continue;
		}

		if ( client->sess.spectatorClient == self->s.number )
		{
			ScoreboardMessage( g_entities + i );
		}
	}

	VectorCopy( self->s.origin, self->client->pers.lastDeathLocation );

	self->takedamage = qfalse; // can still be gibbed

	self->s.weapon = WP_NONE;
	if ( self->client->noclip )
	{
		self->client->cliprcontents = CONTENTS_CORPSE;
	}
	else
	{
		self->r.contents = CONTENTS_CORPSE;
	}

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

	// clear misc
	memset( self->client->ps.misc, 0, sizeof( self->client->ps.misc ) );

	{
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

		// use own entityid if killed by non-client to prevent uint8_t overflow
		G_AddEvent( self, EV_DEATH1 + i,
		            ( killer < MAX_CLIENTS ) ? killer : self - g_entities );

		// globally cycle through the different death animations
		i = ( i + 1 ) % 3;
	}

	trap_LinkEntity( self );

	self->client->pers.infoChangeTime = level.time;
}

static int ParseDmgScript( damageRegion_t *regions, char *buf )
{
	char  *token;
	float angleSpan, heightSpan;
	int   count;

	for ( count = 0;; count++ )
	{
		token = COM_Parse( &buf );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( strcmp( token, "{" ) )
		{
			COM_ParseError( "Missing {" );
			break;
		}

		if ( count >= MAX_DAMAGE_REGIONS )
		{
			COM_ParseError( "Max damage regions exceeded" );
			break;
		}

		// defaults
		regions[ count ].name[ 0 ]     = '\0';
		regions[ count ].minHeight     = 0.0f;
		regions[ count ].maxHeight     = 1.0f;
		regions[ count ].minAngle      = 0.0f;
		regions[ count ].maxAngle      = 360.0f;
		regions[ count ].modifier      = 1.0f;
		regions[ count ].crouch        = qfalse;
		regions[ count ].nonlocational = qfalse;

		while ( 1 )
		{
			token = COM_ParseExt( &buf, qtrue );

			if ( !token[ 0 ] )
			{
				COM_ParseError( "Unexpected end of file" );
				break;
			}

			if ( !Q_stricmp( token, "}" ) )
			{
				break;
			}
			else if ( !strcmp( token, "name" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( token[ 0 ] )
				{
					Q_strncpyz( regions[ count ].name, token,
					            sizeof( regions[ count ].name ) );
				}
			}
			else if ( !strcmp( token, "minHeight" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "0" );
				}

				regions[ count ].minHeight = atof( token );
			}
			else if ( !strcmp( token, "maxHeight" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "100" );
				}

				regions[ count ].maxHeight = atof( token );
			}
			else if ( !strcmp( token, "minAngle" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "0" );
				}

				regions[ count ].minAngle = atoi( token );
			}
			else if ( !strcmp( token, "maxAngle" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "360" );
				}

				regions[ count ].maxAngle = atoi( token );
			}
			else if ( !strcmp( token, "modifier" ) )
			{
				token = COM_ParseExt( &buf, qfalse );

				if ( !token[ 0 ] )
				{
					strcpy( token, "1.0" );
				}

				regions[ count ].modifier = atof( token );
			}
			else if ( !strcmp( token, "crouch" ) )
			{
				regions[ count ].crouch = qtrue;
			}
			else if ( !strcmp( token, "nonlocational" ) )
			{
				regions[ count ].nonlocational = qtrue;
			}
			else
			{
				COM_ParseWarning( "Unknown token \"%s\"", token );
			}
		}

		// Angle portion covered
		angleSpan = regions[ count ].maxAngle - regions[ count ].minAngle;

		if ( angleSpan < 0.0f )
		{
			angleSpan += 360.0f;
		}

		angleSpan /= 360.0f;

		// Height portion covered
		heightSpan = regions[ count ].maxHeight - regions[ count ].minHeight;

		if ( heightSpan < 0.0f )
		{
			heightSpan = -heightSpan;
		}

		if ( heightSpan > 1.0f )
		{
			heightSpan = 1.0f;
		}

		regions[ count ].area = angleSpan * heightSpan;

		if ( !regions[ count ].area )
		{
			regions[ count ].area = 0.00001f;
		}
	}

	return count;
}

float G_GetNonLocDamageMod( class_t pcl )
{
	int            regionNum;
	damageRegion_t *region;

	for ( regionNum = 0; regionNum < g_numDamageRegions[ pcl ]; regionNum++ )
	{
		region = &g_damageRegions[ pcl ][ regionNum ];

		if ( !region->nonlocational )
		{
			continue;
		}

		if ( g_debugDamage.integer > 1 )
		{
			Com_Printf( "GetNonLocDamageModifier( pcl = %s ): "
			            S_COLOR_GREEN "FOUND:" S_COLOR_WHITE " %.2f\n",
			            BG_Class( pcl )->name, region->modifier );
		}

		return region->modifier;
	}

	if ( g_debugDamage.integer > 1 )
	{
		Com_Printf( "GetNonLocDamageModifier( pcl = %s ): "
		            S_COLOR_YELLOW "NOT FOUND:" S_COLOR_WHITE " %.2f.\n",
		            BG_Class( pcl )->name, 1.0f );
	}

	return 1.0f;
}

float G_GetPointDamageMod( gentity_t *target, class_t pcl, float angle, float height )
{
	int            regionNum;
	damageRegion_t *region;
	qboolean       crouching;

	if ( !target || !target->client )
	{
		return 1.0f;
	}

	crouching = ( target->client->ps.pm_flags & PMF_DUCKED );

	for ( regionNum = 0; regionNum < g_numDamageRegions[ pcl ]; regionNum++ )
	{
		region = &g_damageRegions[ pcl ][ regionNum ];

		// ignore nonlocational
		if ( region->nonlocational )
		{
			continue;
		}

		// crouch state must match
		if ( region->crouch != crouching )
		{
			continue;
		}

		// height must be within range
		if ( height < region->minHeight || height > region->maxHeight )
		{
			continue;
		}

		// angle must be within range
		if ( ( region->minAngle <= region->maxAngle && ( angle < region->minAngle || angle > region->maxAngle ) ) ||
		     ( region->minAngle >  region->maxAngle && ( angle > region->maxAngle && angle < region->minAngle ) ) )
		{
			continue;
		}

		if ( g_debugDamage.integer > 1 )
		{
			G_Printf( "GetPointDamageModifier( pcl = %s, angle = %.2f, height = %.2f ): "
			          S_COLOR_GREEN "FOUND:" S_COLOR_WHITE " %.2f (%s)\n",
			          BG_Class( pcl )->name, angle, height, region->modifier, region->name );
		}

		return region->modifier;
	}

	if ( g_debugDamage.integer > 1 )
	{
		G_Printf( "GetPointDamageModifier( pcl = %s, angle = %.2f, height = %.2f ): "
		          S_COLOR_YELLOW "NOT FOUND:" S_COLOR_WHITE " %.2f\n",
		          BG_Class( pcl )->name, angle, height, 1.0f );
	}

	return 1.0f;
}

static float CalcDamageModifier( vec3_t point, gentity_t *target, class_t pcl, int damageFlags )
{
	vec3_t targOrigin, bulletPath, bulletAngle, pMINUSfloor, floor, normal;
	float  clientHeight, hitRelative, hitRatio, modifier;
	int    hitRotation;

	// handle nonlocational damage
	if ( damageFlags & DAMAGE_NO_LOCDAMAGE )
	{
		return G_GetNonLocDamageMod( pcl );
	}

	// need a valid point and target client
	if ( !point || !target || !target->client )
	{
		return 1.0f;
	}

	// Get the point location relative to the floor under the target
	if ( g_unlagged.integer && target->client->unlaggedCalc.used )
	{
		VectorCopy( target->client->unlaggedCalc.origin, targOrigin );
	}
	else
	{
		VectorCopy( target->r.currentOrigin, targOrigin );
	}

	BG_GetClientNormal( &target->client->ps, normal );
	VectorMA( targOrigin, target->r.mins[ 2 ], normal, floor );
	VectorSubtract( point, floor, pMINUSfloor );

	// Get the proportion of the target height where the hit landed
	clientHeight = target->r.maxs[ 2 ] - target->r.mins[ 2 ];

	if ( !clientHeight )
	{
		clientHeight = 1.0f;
	}

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

	// Get the yaw of the attack relative to the target's view yaw
	VectorSubtract( point, targOrigin, bulletPath );
	vectoangles( bulletPath, bulletAngle );
	hitRotation = AngleNormalize360( target->client->ps.viewangles[ YAW ] - bulletAngle[ YAW ] );

	// Get damage region modifier
	modifier = G_GetPointDamageMod( target, pcl, hitRotation, hitRatio );

	return modifier;
}

void G_InitDamageLocations( void )
{
	const char   *modelName;
	char         filename[ MAX_QPATH ];
	int          i;
	int          len;
	fileHandle_t fileHandle;
	char         buffer[ MAX_DAMAGE_REGION_TEXT ];

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		modelName = BG_ClassModelConfig( i )->modelName;
		Com_sprintf( filename, sizeof( filename ), "configs/classes/%s.locdamage.cfg", modelName );

		len = trap_FS_FOpenFile( filename, &fileHandle, FS_READ );

		if ( !fileHandle )
		{
			G_Printf( S_COLOR_RED "file not found: %s\n", filename );
			continue;
		}

		if ( len >= MAX_DAMAGE_REGION_TEXT )
		{
			G_Printf( S_COLOR_RED "file too large: %s is %i, max allowed is %i\n",
			          filename, len, MAX_DAMAGE_REGION_TEXT );
			trap_FS_FCloseFile( fileHandle );
			continue;
		}

		COM_BeginParseSession( filename );

		trap_FS_Read( buffer, len, fileHandle );
		buffer[ len ] = 0;
		trap_FS_FCloseFile( fileHandle );

		g_numDamageRegions[ i ] = ParseDmgScript( g_damageRegions[ i ], buffer );
	}
}

/**
 * @brief G_SelectiveDamage
 * @param targ
 * @param inflictor
 * @param attacker
 * @param dir
 * @param point
 * @param damage
 * @param dflags
 * @param mod
 * @param team team that is immune to this damage
 */
void G_SelectiveDamage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
                        vec3_t dir, vec3_t point, int damage, int dflags, int mod, int team )
{
	if ( targ->client && ( team != targ->client->pers.team ) )
	{
		G_Damage( targ, inflictor, attacker, dir, point, damage, dflags, mod );
	}
}

static void NotifyClientOfHit( gentity_t *attacker )
{
	gentity_t *event;

	if ( !attacker->client )
	{
		return;
	}

	event = G_NewTempEntity( attacker->s.origin, EV_HIT );
	event->r.svFlags = SVF_SINGLECLIENT;
	event->r.singleClient = attacker->client->ps.clientNum;
}

#define KNOCKBACK_NORMAL_MASS 100
#define KNOCKBACK_PMOVE_TIME  50

void G_KnockbackByDir( gentity_t *target, const vec3_t direction, float strength,
                              qboolean ignoreMass )
{
	vec3_t dir, vel;
	int    mass;
	float  massMod;
	const classAttributes_t *ca;

	// sanity check parameters
	if ( !target || !target->client || VectorLength( direction ) == 0.0f || strength == 0 )
	{
		return;
	}

	// check target flags
	if ( target->flags & FL_NO_KNOCKBACK )
	{
		return;
	}

	ca = BG_Class( target->client->ps.stats[ STAT_CLASS ] );

	// normalize direction
	VectorCopy( direction, dir );
	VectorNormalize( dir );

	// adjust strength according to client mass
	if ( !ignoreMass )
	{
		if ( ca->mass <= 0 )
		{
			mass = KNOCKBACK_NORMAL_MASS;
		}
		else
		{
			mass = ca->mass;
		}

		massMod = ( float )KNOCKBACK_NORMAL_MASS / ( float )mass;

		if ( massMod < 0.5f )
		{
			massMod = 0.5f;
		}
		else if ( massMod > 2.0f )
		{
			massMod = 2.0f;
		}
	}
	else
	{
		// for debug print
		massMod = 1.0f;
	}

	strength *= massMod;

	// adjust client velocity
	VectorScale( dir, strength, vel );
	VectorAdd( target->client->ps.velocity, vel, target->client->ps.velocity );

	// set pmove timer so that the client can't cancel out the movement immediately
	if ( !target->client->ps.pm_time )
	{
		target->client->ps.pm_time = KNOCKBACK_PMOVE_TIME;
		target->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}

	// print debug info
	if ( g_debugKnockback.integer )
	{
		G_Printf( "%i: Knockback: client: %i, strength: %.1f (massMod: %.1f)\n",
		          level.time, target->s.number, strength, massMod );
	}
}

void G_KnockbackBySource( gentity_t *target, gentity_t *source, float strength, qboolean ignoreMass )
{
	vec3_t dir;

	if ( !target || !source )
	{
		return;
	}

	VectorSubtract( target->s.origin, source->s.origin, dir );
	VectorNormalize( dir );

	G_KnockbackByDir( target, dir, strength, ignoreMass );
}

#define DAMAGE_TO_KNOCKBACK   5
#define MAX_FALLDMG_KNOCKBACK 50

// TODO: Clean this mess further (split into helper functions)
void G_Damage( gentity_t *target, gentity_t *inflictor, gentity_t *attacker,
               vec3_t dir, vec3_t point, int damage, int damageFlags, int mod )
{
	gclient_t *client;
	int       take, loss;
	int       knockback;
	float     modifier;

	if ( !target || !target->takedamage || target->health <= 0 || level.intermissionQueued )
	{
		return;
	}

	client = target->client;

	// don't handle noclip clients
	if ( client && client->noclip )
	{
		return;
	}

	// set inflictor to world if missing
	if ( !inflictor )
	{
		inflictor = &g_entities[ ENTITYNUM_WORLD ];
	}

	// set attacker to world if missing
	if ( !attacker )
	{
		attacker = &g_entities[ ENTITYNUM_WORLD ];
	}

	// don't handle ET_MOVER w/o die or pain function
	if ( target->s.eType == ET_MOVER && !( target->die || target->pain ) )
	{
		// special case for ET_MOVER with act function in initial position
		if ( ( target->moverState == MOVER_POS1 || target->moverState == ROTATOR_POS1 ) &&
		     target->act )
		{
			target->act( target, inflictor, attacker );
		}

		return;
	}

	// do knockback against clients
	if ( client && !( damageFlags & DAMAGE_NO_KNOCKBACK ) && dir )
	{
		// scale knockback by weapon
		if ( inflictor->s.weapon != WP_NONE )
		{
			knockback = ( int )( ( float )damage * BG_Weapon( inflictor->s.weapon )->knockbackScale );
		}
		else
		{
			knockback = damage;
		}

		// apply generic damage to knockback modifier
		knockback *= DAMAGE_TO_KNOCKBACK;

		// HACK: Too much knockback from falling makes you bounce and looks silly
		if ( mod == MOD_FALLING )
		{
			knockback = MIN( knockback, MAX_FALLDMG_KNOCKBACK );
		}

		G_KnockbackByDir( target, dir, knockback, qfalse );
	}
	else
	{
		// damage knockback gets saved, so initialize it here
		knockback = 0;
	}

	// godmode prevents damage
	if ( target->flags & FL_GODMODE )
	{
		return;
	}

	// check for protection
	if ( !( damageFlags & DAMAGE_NO_PROTECTION ) )
	{
		// check for protection from friendly damage
		if ( target != attacker && G_OnSameTeam( target, attacker ) )
		{
			// check if friendly fire has been disabled
			if ( !g_friendlyFire.integer )
			{
				return;
			}

			// don't do friendly damage on movement attacks
			switch ( mod )
			{
				case MOD_LEVEL3_POUNCE:
				case MOD_LEVEL4_TRAMPLE:
					return;

				default:
					break;
			}

			// if dretchpunt is enabled and this is a dretch, do dretchpunt instead of damage
			if ( g_dretchPunt.integer && target->client && target->client->ps.stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL0 )
			{
				vec3_t dir, push;

				VectorSubtract( target->r.currentOrigin, attacker->r.currentOrigin, dir );
				VectorNormalizeFast( dir );
				VectorScale( dir, ( damage * 10.0f ), push );
				push[ 2 ] = 64.0f;

				VectorAdd( target->client->ps.velocity, push, target->client->ps.velocity );

				return;
			}
		}

		// for buildables, never protect from damage dealt by building actions
		if ( target->s.eType == ET_BUILDABLE && attacker->client &&
		     mod != MOD_DECONSTRUCT && mod != MOD_SUICIDE &&
		     mod != MOD_REPLACE     && mod != MOD_NOCREEP )
		{
			// check for protection from friendly buildable damage
			if ( G_OnSameTeam( target, attacker ) && !g_friendlyBuildableFire.integer )
			{
				return;
			}
		}
	}

	// update combat timers
	if ( target->client && attacker->client && target != attacker )
	{
		target->client->lastCombatTime   = level.time;
		attacker->client->lastCombatTime = level.time;
	}

	if ( client )
	{
		// save damage (w/o armor modifier), knockback
		client->damage_received  += damage;
		client->damage_knockback += knockback;

		// save damage direction
		if ( dir )
		{
			VectorCopy( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		}
		else
		{
			VectorCopy( target->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}

		// drain jetpack fuel
		client->ps.stats[ STAT_FUEL ] -= damage * JETPACK_FUEL_PER_DMG;
		if ( client->ps.stats[ STAT_FUEL ] < 0 )
		{
			client->ps.stats[ STAT_FUEL ] = 0;
		}

		// apply damage modifier
		modifier = CalcDamageModifier( point, target, (class_t) client->ps.stats[ STAT_CLASS ], damageFlags );
		take = ( int )( ( float )damage * modifier + 0.5f );

		// if boosted poison every attack
		if ( attacker->client &&
		     ( attacker->client->ps.stats[ STAT_STATE ] & SS_BOOSTED ) &&
		     target->client->pers.team == TEAM_HUMANS &&
		     target->client->poisonImmunityTime < level.time )
		{
			switch ( mod )
			{
				case MOD_POISON:
				case MOD_LEVEL2_ZAP:
					break;

				default:
					target->client->ps.stats[ STAT_STATE ] |= SS_POISONED;
					target->client->lastPoisonTime   = level.time;
					target->client->lastPoisonClient = attacker;
			}
		}
	}
	else
	{
		take = damage;
	}

	// make sure damage is done
	if ( take < 1 )
	{
		take = 1;
	}

	if ( g_debugDamage.integer > 0 )
	{
		G_Printf( "G_Damage: %3i (%3i → %3i)\n",
		          take, target->health, target->health - take );
	}

	// do the damage
	target->health = target->health - take;

	if ( target->client )
	{
		target->client->ps.stats[ STAT_HEALTH ] = target->health;
		target->client->pers.infoChangeTime = level.time; // ?
	}

	target->lastDamageTime = level.time;

	// TODO: gentity_t->nextRegenTime only affects alien clients, remove it and use lastDamageTime
	// Optionally (if needed for some reason), move into client struct and add "Alien" to name
	target->nextRegenTime = level.time + ALIEN_CLIENT_REGEN_WAIT;

	// handle non-self damage
	if ( attacker != target )
	{
		if ( target->health < 0 )
		{
			loss = ( take + target->health );
		}
		else
		{
			loss = take;
		}

		if ( attacker->client )
		{
			// add to the attacker's account on the target
			target->credits[ attacker->client->ps.clientNum ].value += ( float )loss;
			target->credits[ attacker->client->ps.clientNum ].time = level.time;
			target->credits[ attacker->client->ps.clientNum ].team = (team_t)attacker->client->pers.team;

			// notify the attacker of a hit
			NotifyClientOfHit( attacker );
		}

		// update buildable stats
		if ( attacker->s.eType == ET_BUILDABLE && attacker->health > 0 )
		{
			attacker->buildableStatsTotal += loss;
		}
	}

	// handle dying target
	if ( target->health <= 0 )
	{
		// set no knockback flag for clients
		if ( client )
		{
			target->flags |= FL_NO_KNOCKBACK;
		}

		// cap negative health
		if ( target->health < -999 )
		{
			target->health = -999;
		}

		// call die function
		if ( target->die )
		{
			target->die( target, inflictor, attacker, mod );
		}

		// update buildable stats
		if ( attacker->s.eType == ET_BUILDABLE && attacker->health > 0 )
		{
			attacker->buildableStatsCount++;
		}

		// for non-client victims, fire ON_DIE event
		if( !target->client )
		{
			G_EventFireEntity( target, attacker, ON_DIE );
		}

		return;
	}
	else if ( target->pain )
	{
		target->pain( target, attacker, take );
	}
}

/**
 * @brief Used for explosions and melee attacks.
 * @param targ
 * @param origin
 * @return qtrue if the inflictor can directly damage the target.
 */
qboolean G_CanDamage( gentity_t *targ, vec3_t origin )
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

qboolean G_SelectiveRadiusDamage( vec3_t origin, gentity_t *attacker, float damage,
                                  float radius, gentity_t *ignore, int mod, int ignoreTeam )
{
	float     points, dist;
	gentity_t *ent;
	int       entityList[ MAX_GENTITIES ];
	int       numListedEntities;
	vec3_t    mins, maxs;
	vec3_t    v;
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

		if ( ent->flags & FL_NOTARGET )
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

		if ( G_CanDamage( ent, origin ) && ent->client &&
		     ent->client->pers.team != ignoreTeam )
		{
			hitClient = qtrue;

			// don't do knockback, since an attack that spares one team is most likely
			// not based on kinetic energy
			G_Damage( ent, NULL, attacker, NULL, origin, ( int ) points,
			          DAMAGE_RADIUS | DAMAGE_NO_LOCDAMAGE | DAMAGE_NO_KNOCKBACK, mod );
		}
	}

	return hitClient;
}

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

		if ( G_CanDamage( ent, origin ) )
		{
			VectorSubtract( ent->r.currentOrigin, origin, dir );
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[ 2 ] += 24;
			VectorNormalize( dir );
			hitClient = qtrue;
			G_Damage( ent, NULL, attacker, dir, origin,
			          ( int ) points, DAMAGE_RADIUS | DAMAGE_NO_LOCDAMAGE, mod );
		}
	}

	return hitClient;
}

/**
 * @brief Log deconstruct/destroy events
 * @param self
 * @param actor
 * @param mod
 */
void G_LogDestruction( gentity_t *self, gentity_t *actor, int mod )
{
	buildFate_t fate;

	switch ( mod )
	{
		case MOD_DECONSTRUCT:
			fate = BF_DECONSTRUCT;
			break;

		case MOD_REPLACE:
			fate = BF_REPLACE;
			break;

		case MOD_NOCREEP:
			fate = ( actor->client ) ? BF_UNPOWER : BF_AUTO;
			break;

		default:
			if ( actor->client )
			{
				if ( actor->client->pers.team ==
				     BG_Buildable( self->s.modelindex )->team )
				{
					fate = BF_TEAMKILL;
				}
				else
				{
					fate = BF_DESTROY;
				}
			}
			else
			{
				fate = BF_AUTO;
			}

			break;
	}

	G_BuildLogAuto( actor, self, fate );

	// don't log when marked structures are removed
	if ( mod == MOD_REPLACE )
	{
		return;
	}

	G_LogPrintf( S_COLOR_YELLOW "Deconstruct: %d %d %s %s: %s %s by %s\n",
	             ( int )( actor - g_entities ),
	             ( int )( self - g_entities ),
	             BG_Buildable( self->s.modelindex )->name,
	             modNames[ mod ],
	             BG_Buildable( self->s.modelindex )->humanName,
	             mod == MOD_DECONSTRUCT ? "deconstructed" : "destroyed",
	             actor->client ? actor->client->pers.netname : "<world>" );

	// No-power deaths for humans come after some minutes and it's confusing
	//  when the messages appear attributed to the deconner. Just don't print them.
	if ( mod == MOD_NOCREEP && actor->client &&
	     actor->client->pers.team == TEAM_HUMANS )
	{
		return;
	}

	if ( actor->client && actor->client->pers.team ==
	     BG_Buildable( self->s.modelindex )->team )
	{
		G_TeamCommand( (team_t) actor->client->pers.team,
		               va( "print_tr %s %s %s", mod == MOD_DECONSTRUCT ? QQ( N_("$1$ ^3DECONSTRUCTED^7 by $2$\n") ) :
						   QQ( N_("$1$ ^3DESTROYED^7 by $2$\n") ),
		                   Quote( BG_Buildable( self->s.modelindex )->humanName ),
		                   Quote( actor->client->pers.netname ) ) );
	}
}
