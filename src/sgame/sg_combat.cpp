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
#include "CBSE.h"

Cvar::Cvar<float> g_rewardDestruction( "g_rewardDestruction", "Reward players when they destroy a building by momentum * g_rewardDestruction", Cvar::NONE, 0.f );
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
	"MOD_SLAP",
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
	"MOD_ROCKETPOD",
	"MOD_MGTURRET",
	"MOD_REACTOR",

	"MOD_ASPAWN",
	"MOD_ATUBE",
	"MOD_SPIKER",
	"MOD_OVERMIND",
	"MOD_DECONSTRUCT",
	"MOD_REPLACE"
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

static void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker )
{
	if ( attacker && attacker != self )
	{
		self->client->ps.stats[ STAT_VIEWLOCK ] = attacker->num();
	}
	else if ( inflictor && inflictor != self )
	{
		self->client->ps.stats[ STAT_VIEWLOCK ] = inflictor->num();
	}
	else
	{
		self->client->ps.stats[ STAT_VIEWLOCK ] = self->num();
	}
}

/**
 * @brief Function to find who assisted most (and, in case of a tie, most recently) with a kill
 * @param self
 * @param killer
 */
static const gentity_t *G_FindKillAssist( const gentity_t *self, const gentity_t *killer, team_t *team )
{
	const gentity_t *assistant = nullptr;
	float           damage;
	int             when = 0;
	int             playerNum;

	// Suicide? No assistance needed with that
	if ( killer == self )
	{
		return nullptr;
	}

	// Require that the assist was for, at least, 25% of the health or
	// as much damage as the killer did, whichever is lower
	damage = self->entity->Get<HealthComponent>()->MaxHealth() / 4.0f;
	if ( killer && killer->client )
	{
		damage = std::min( damage, self->credits[ killer->num() ].value );
	}

	// Find the best assistant
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
		maxHealth = self->entity->Get<HealthComponent>()->MaxHealth();
		value     = BG_GetPlayerValue( self->client->ps );
	}
	else if ( self->s.eType == entityType_t::ET_BUILDABLE )
	{
		ownTeam   = (team_t) self->buildableTeam;
		maxHealth = self->entity->Get<HealthComponent>()->MaxHealth();

		if ( self->entity->Get<MainBuildableComponent>() )
		{
			value = MAIN_STRUCTURE_MOMENTUM_VALUE;
		}
		else if ( self->entity->Get<MiningComponent>() )
		{
			value = MINER_MOMENTUM_VALUE;
		}
		else
		{
			value = BG_Buildable( self->s.modelindex )->buildPoints;
		}

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

		if ( self->s.eType == entityType_t::ET_BUILDABLE )
		{
			// Add score
			G_AddMomentumToScore( player, reward );

			// Add credits
			G_AddCreditToClient( player->client, static_cast<short>( reward * g_rewardDestruction.Get() ), true );

			// Add momentum
			G_AddMomentumForDestroyingStep( self, player, reward );
		}
		else
		{
			// Add score
			G_AddCreditsToScore( player, ( int )reward );

			// Add credits
			G_AddCreditToClient( player->client, ( short )reward, true );

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
	const char *killerName, *obit;

	const gentity_t *assistantEnt;
	int             assistant = ENTITYNUM_NONE;
	const char      *assistantName = nullptr;
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
		killer = attacker->num();

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
		assistant = assistantEnt->num();

		if ( assistantEnt->client )
		{
			assistantName = assistantEnt->client->pers.netname;
		}
	}

	if ( meansOfDeath < 0 || meansOfDeath >= (int) ARRAY_LEN( modNames ) )
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
		G_LogPrintf( "Die: %d %d %s %d %d: %s^* killed %s^*; %s^* assisted",
		             killer,
		             self->num(),
		             obit,
		             assistant,
		             assistantTeam,
			     killerName,
			     self->client->pers.netname,
			     assistantName );
	}
	else
	{
		G_LogPrintf( "Die: %d %d %s: %s^* killed %s",
		             killer,
		             self->num(),
		             obit,
		             killerName,
		             self->client->pers.netname );
	}

	// deactivate all upgrades
	for ( int i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		BG_DeactivateUpgrade( i, self->client->ps.stats );
	}

	// broadcast the death event to everyone
	ent = G_NewTempEntity( VEC2GLM( self->r.currentOrigin ), EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->num();
	ent->s.otherEntityNum2 = killer;
	ent->s.groundEntityNum = assistant; // we hijack the field for this
	ent->s.generic1 = assistantTeam;
	ent->r.svFlags = SVF_BROADCAST; // send to everyone

	if ( attacker && attacker->client )
	{
		if ( ( attacker == self || G_OnSameTeam( self, attacker ) ) )
		{
			//punish team kills and suicides
			if ( attacker->client->pers.team == TEAM_ALIENS )
			{
				G_AddCreditToClient( attacker->client, -ALIEN_TK_SUICIDE_PENALTY, true );
				G_AddCreditsToScore( attacker, -ALIEN_TK_SUICIDE_PENALTY );
			}
			else if ( attacker->client->pers.team == TEAM_HUMANS )
			{
				G_AddCreditToClient( attacker->client, -HUMAN_TK_SUICIDE_PENALTY, true );
				G_AddCreditsToScore( attacker, -HUMAN_TK_SUICIDE_PENALTY );
			}
		}
		else if ( g_showKillerHP.Get() )
		{
			trap_SendServerCommand( self->num(), va( "print_tr %s %s %3i", QQ( N_("Your killer, $1$^*, had $2$ HP.") ),
			                        Quote( killerName ),
			                        (int)std::ceil(attacker->entity->Get<HealthComponent>()->Health()) ) );
		}
	}
	else if ( attacker->s.eType != entityType_t::ET_BUILDABLE )
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
	for ( int i = 0; i < level.maxclients; i++ )
	{
		gclient_t *client = &level.clients[ i ];

		if ( client->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if ( client->sess.spectatorState == SPECTATOR_NOT )
		{
			continue;
		}

		if ( client->sess.spectatorClient == self->num() )
		{
			ScoreboardMessage( g_entities + i );
		}
	}

	VectorCopy( self->s.origin, self->client->pers.lastDeathLocation );

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
		static int i=0;

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
					anim = BOTH_DEATH3;
					break;
				default:
					ASSERT_UNREACHABLE();
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
					anim = NSPA_DEATH3;
					break;
				default:
					ASSERT_UNREACHABLE();
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
		            ( killer < MAX_CLIENTS ) ? killer : self->num() );

		// globally cycle through the different death animations
		i = ( i + 1 ) % 3;
	}

	Beacon::DetachTags( self );

	trap_LinkEntity( self );

	self->client->pers.infoChangeTime = level.time;
}

static int ParseDmgScript( damageRegion_t *regions, const char *buf )
{
	const char *token;
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
		regions[ count ].crouch        = false;
		regions[ count ].nonlocational = false;

		while ( 1 )
		{
			token = COM_ParseExt( &buf, true );

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
				token = COM_ParseExt( &buf, false );

				if ( token[ 0 ] )
				{
					Q_strncpyz( regions[ count ].name, token,
					            sizeof( regions[ count ].name ) );
				}
			}
			else if ( !strcmp( token, "minHeight" ) )
			{
				token = COM_ParseExt( &buf, false );

				if ( !token[ 0 ] )
				{
					token = "0";
				}

				regions[ count ].minHeight = atof( token );
			}
			else if ( !strcmp( token, "maxHeight" ) )
			{
				token = COM_ParseExt( &buf, false );

				if ( !token[ 0 ] )
				{
					token = "100";
				}

				regions[ count ].maxHeight = atof( token );
			}
			else if ( !strcmp( token, "minAngle" ) )
			{
				token = COM_ParseExt( &buf, false );

				if ( !token[ 0 ] )
				{
					token = "0";
				}

				regions[ count ].minAngle = atoi( token );
			}
			else if ( !strcmp( token, "maxAngle" ) )
			{
				token = COM_ParseExt( &buf, false );

				if ( !token[ 0 ] )
				{
					token = "360";
				}

				regions[ count ].maxAngle = atoi( token );
			}
			else if ( !strcmp( token, "modifier" ) )
			{
				token = COM_ParseExt( &buf, false );

				if ( !token[ 0 ] )
				{
					token = "1.0";
				}

				regions[ count ].modifier = atof( token );
			}
			else if ( !strcmp( token, "crouch" ) )
			{
				regions[ count ].crouch = true;
			}
			else if ( !strcmp( token, "nonlocational" ) )
			{
				regions[ count ].nonlocational = true;
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

void G_InitDamageLocations()
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

		len = G_FOpenGameOrPakPath( filename, fileHandle );

		if ( !fileHandle )
		{
			Log::Warn( "^1file not found: %s", filename );
			continue;
		}

		if ( len >= MAX_DAMAGE_REGION_TEXT )
		{
			Log::Warn( "^1file too large: %s is %i, max allowed is %i",
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

// TODO: Move to HealthComponent.
void G_SelectiveDamage( gentity_t *targ, gentity_t * /*inflictor*/, gentity_t *attacker,
                        vec3_t dir, vec3_t point, int damage, int dflags, int mod, int team )
{
	if ( targ->client && ( team != targ->client->pers.team ) )
	{
		targ->entity->Damage((float)damage, attacker, VEC2GLM( point ), VEC2GLM( dir ), dflags,
		                     (meansOfDeath_t)mod);
	}
}

/**
 * @brief Used for explosions and melee attacks.
 * @param targ
 * @param origin
 * @return true if the inflictor can directly damage the target.
 */
bool G_CanDamage( gentity_t *targ, vec3_t origin )
{
	vec3_t  dest;
	trace_t tr;
	vec3_t  midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd( targ->r.absmin, targ->r.absmax, midpoint );
	VectorScale( midpoint, 0.5, midpoint );

	VectorCopy( midpoint, dest );
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID, 0 );

	if ( tr.fraction == 1.0  || tr.entityNum == targ->num() )
	{
		return true;
	}

	// this should probably check in the plane of projection,
	// rather than in world coordinate, and also include Z
	VectorCopy( midpoint, dest );
	dest[ 0 ] += 15.0;
	dest[ 1 ] += 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID, 0 );

	if ( tr.fraction == 1.0 )
	{
		return true;
	}

	VectorCopy( midpoint, dest );
	dest[ 0 ] += 15.0;
	dest[ 1 ] -= 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID, 0 );

	if ( tr.fraction == 1.0 )
	{
		return true;
	}

	VectorCopy( midpoint, dest );
	dest[ 0 ] -= 15.0;
	dest[ 1 ] += 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID, 0 );

	if ( tr.fraction == 1.0 )
	{
		return true;
	}

	VectorCopy( midpoint, dest );
	dest[ 0 ] -= 15.0;
	dest[ 1 ] -= 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID, 0 );

	if ( tr.fraction == 1.0 )
	{
		return true;
	}

	return false;
}

bool G_SelectiveRadiusDamage( vec3_t origin, gentity_t *attacker, float damage,
                                  float radius, gentity_t *ignore, int mod, int ignoreTeam )
{
	float     points, dist;
	gentity_t *ent;
	int       entityList[ MAX_GENTITIES ];
	int       numListedEntities;
	vec3_t    mins, maxs;
	int       i, e;
	bool  hitClient = false;

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

		if ( ent->flags & FL_NOTARGET )
		{
			continue;
		}

		// find the distance from the edge of the bounding box
		dist = G_DistanceToBBox( origin, ent );

		if ( dist >= radius )
		{
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if ( G_CanDamage( ent, origin ) && ent->client &&
		     ent->client->pers.team != ignoreTeam )
		{
			hitClient = ent->entity->Damage(points, attacker, VEC2GLM( origin ), Util::nullopt,
			                                DAMAGE_NO_LOCDAMAGE, (meansOfDeath_t)mod);
		}
	}

	return hitClient;
}

bool G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage,
                         float radius, gentity_t *ignore, int dflags, int mod, team_t testHit )
{
	float     points, dist;
	gentity_t *ent;
	int       entityList[ MAX_GENTITIES ];
	int       numListedEntities;
	vec3_t    mins, maxs;
	vec3_t    dir;
	int       i, e;
	bool  hitSomething = false;

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

		// find the distance from the edge of the bounding box
		dist = G_DistanceToBBox( origin, ent );

		if ( dist >= radius )
		{
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if ( G_CanDamage( ent, origin ) )
		{
			if ( testHit == TEAM_NONE )
			{
				VectorSubtract( ent->r.currentOrigin, origin, dir );
				// push the center of mass higher than the origin so players
				// get knocked into the air more
				dir[ 2 ] += 24;
				VectorNormalize( dir );

				hitSomething = ent->entity->Damage(points, attacker, VEC2GLM( origin ), VEC2GLM( dir ),
				                                   (DAMAGE_NO_LOCDAMAGE | dflags), (meansOfDeath_t)mod);
			}
			else if ( G_Team( ent ) == testHit && Entities::IsAlive( ent ) )
			{
				return true;
			}
		}
	}

	return hitSomething;
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

		default:
			if ( actor->client )
			{
				fate = G_OnSameTeam(self, actor) ?
					BF_TEAMKILL : BF_DESTROY;
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

	G_LogPrintf( "^3Deconstruct: %d %d %s %s: %s %s by %s",
	             actor->num(),
	             self->num(),
	             BG_Buildable( self->s.modelindex )->name,
	             modNames[ mod ],
	             BG_Buildable( self->s.modelindex )->humanName,
	             mod == MOD_DECONSTRUCT ? "deconstructed" : "destroyed",
	             actor->client ? actor->client->pers.netname : "<world>" );

	if ( actor->client && G_OnSameTeam( self, actor ) )
	{
		G_TeamCommand( G_Team( actor ),
		               va( "print_tr %s %s %s", mod == MOD_DECONSTRUCT ? QQ( N_("$1$ ^3DECONSTRUCTED^* by $2$") ) :
						   QQ( N_("$1$ ^3DESTROYED^* by $2$") ),
		                   Quote( BG_Buildable( self->s.modelindex )->humanName ),
		                   Quote( actor->client->pers.netname ) ) );
	}
}
