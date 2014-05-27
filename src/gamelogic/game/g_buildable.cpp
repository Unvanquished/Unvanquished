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

#define PRIMARY_ATTACK_PERIOD 7500
#define NEARBY_ATTACK_PERIOD  15000

/*
================
G_WarnPrimaryUnderAttack

Warns when the team's primary building is under attack.
Called from overmind and reactor thinkers.
================
*/

static void G_WarnPrimaryUnderAttack( gentity_t *self )
{
	const buildableAttributes_t *attr = BG_Buildable( self->s.modelindex );
	float fracHealth, fracLastHealth;
	int event = 0;

	fracHealth = (float) self->health / (float) attr->health;
	fracLastHealth = (float) self->lastHealth / (float) attr->health;

	if ( fracHealth < 0.25f && fracLastHealth >= 0.25f )
	{
		event = attr->team == TEAM_ALIENS ? EV_OVERMIND_ATTACK_2 : EV_REACTOR_ATTACK_2;
	}
	else if ( fracHealth < 0.75f && fracLastHealth >= 0.75f )
	{
		event = attr->team == TEAM_ALIENS ? EV_OVERMIND_ATTACK_1 : EV_REACTOR_ATTACK_1;
	}

	if ( event && ( event != self->attackLastEvent || level.time > self->attackTimer ) )
	{
		self->attackTimer = level.time + PRIMARY_ATTACK_PERIOD; // don't spam
		self->attackLastEvent = event;
		G_BroadcastEvent( event, 0, attr->team );
	}

	self->lastHealth = self->health;
}

/*
================
IsWarnableMOD

True if the means of death allows an under-attack warning.
================
*/

static qboolean IsWarnableMOD( int mod )
{
	switch ( mod )
	{
		//case MOD_UNKNOWN:
		case MOD_TRIGGER_HURT:
		case MOD_DECONSTRUCT:
		case MOD_REPLACE:
		case MOD_NOCREEP:
			return qfalse;

		default:
			return qtrue;
	}
}

/*
================
G_SetBuildableAnim

Triggers an animation client side
================
*/
void G_SetBuildableAnim( gentity_t *ent, buildableAnimNumber_t anim, qboolean force )
{
	int localAnim = anim | ( ent->s.legsAnim & ANIM_TOGGLEBIT );

	if ( force )
	{
		localAnim |= ANIM_FORCEBIT;
	}

	// don't flip the togglebit more than once per frame
	if ( ent->animTime != level.time )
	{
		ent->animTime = level.time;
		localAnim ^= ANIM_TOGGLEBIT;
	}

	ent->s.legsAnim = localAnim;
}

/*
================
G_SetIdleBuildableAnim

Set the animation to use whilst no other animations are running
================
*/
void G_SetIdleBuildableAnim( gentity_t *ent, buildableAnimNumber_t anim )
{
	ent->s.torsoAnim = anim;
}

/*
===============
G_CheckSpawnPoint

Check if a spawn at a specified point is valid
===============
*/
gentity_t *G_CheckSpawnPoint( int spawnNum, const vec3_t origin,
                              const vec3_t normal, buildable_t spawn, vec3_t spawnOrigin )
{
	float   minDistance, maxDistance, frac, displacement;
	vec3_t  mins, maxs;
	vec3_t  cmins, cmaxs;
	vec3_t  localOrigin;
	trace_t tr;

	BG_BuildableBoundingBox( spawn, mins, maxs );

	if ( spawn == BA_A_SPAWN )
	{
		// HACK: Assume advanced granger is biggest class that can spawn
		BG_ClassBoundingBox( PCL_ALIEN_BUILDER0_UPG, cmins, cmaxs, NULL, NULL, NULL );

		// Calculate the necessary displacement for a given normal so that the bounding boxes
		// of buildable and client don't intersect.

		// Method 1
		// This describes the worst case for any angle.
		/*
		displacement = MAX( VectorLength( mins ), VectorLength( maxs ) ) +
		               MAX( VectorLength( cmins ), VectorLength( cmaxs ) ) + 1.0f;
		*/

		// Method 2
		// For normals with equal X and Y component these results are optimal. Otherwise they are
		// still better than naive worst-case calculations.
		// HACK: This only works for normals w/ angle < 45° towards either {0,0,1} or {0,0,-1}
		if ( normal[ 2 ] < 0.0f )
		{
			// best case: client spawning right below egg
			minDistance = -mins[ 2 ] + cmaxs[ 2 ];

			// worst case: bounding boxes meet at their corners
			maxDistance = VectorLength( mins ) + VectorLength( cmaxs );

			// the fraction of the angle seperating best (0°) and worst case (45°)
			frac = acos( -normal[ 2 ] ) / ( M_PI / 4.0f );
		}
		else
		{
			// best case: client spawning right above egg
			minDistance = maxs[ 2 ] - cmins[ 2 ];

			// worst case: bounding boxes meet at their corners
			maxDistance = VectorLength( maxs ) + VectorLength( cmins );

			// the fraction of the angle seperating best (0°) and worst case (45°)
			frac = acos( normal[ 2 ] ) / ( M_PI / 4.0f );
		}

		// the linear interpolation of min & max distance should be an upper boundary for the
		// optimal (closest possible) distance.
		displacement = ( 1.0f - frac ) * minDistance + frac * maxDistance + 1.0f;

		VectorMA( origin, displacement, normal, localOrigin );
	}
	else if ( spawn == BA_H_SPAWN )
	{
		// HACK: Assume naked human is biggest class that can spawn
		BG_ClassBoundingBox( PCL_HUMAN_NAKED, cmins, cmaxs, NULL, NULL, NULL );

		// Since humans always spawn right above the telenode center, this is the optimal
		// (closest possible) client origin.
		VectorCopy( origin, localOrigin );
		localOrigin[ 2 ] += maxs[ 2 ] - cmins[ 2 ] + 1.0f;
	}
	else
	{
		return NULL;
	}

	trap_Trace( &tr, origin, NULL, NULL, localOrigin, spawnNum, MASK_SHOT );

	if ( tr.entityNum != ENTITYNUM_NONE )
	{
		return &g_entities[ tr.entityNum ];
	}

	trap_Trace( &tr, localOrigin, cmins, cmaxs, localOrigin, -1, MASK_PLAYERSOLID );

	if ( tr.entityNum != ENTITYNUM_NONE )
	{
		return &g_entities[ tr.entityNum ];
	}

	if ( spawnOrigin != NULL )
	{
		VectorCopy( localOrigin, spawnOrigin );
	}

	return NULL;
}

/*
===============
G_PuntBlocker

Move spawn blockers
===============
*/
static void PuntBlocker( gentity_t *self, gentity_t *blocker )
{
	vec3_t nudge;
	if( self )
	{
		if( !self->spawnBlockTime )
		{
			// begin timer
			self->spawnBlockTime = level.time;
			return;
		}
		else if( level.time - self->spawnBlockTime > 10000 )
		{
			// still blocked, get rid of them
			G_Damage( blocker, NULL, NULL, NULL, NULL, 10000, 0, MOD_TRIGGER_HURT );
			self->spawnBlockTime = 0;
			return;
		}
		else if( level.time - self->spawnBlockTime < 5000 )
		{
			// within grace period
			return;
		}
	}

	nudge[ 0 ] = crandom() * 100.0f;
	nudge[ 1 ] = crandom() * 100.0f;
	nudge[ 2 ] = 75.0f;

	if ( blocker->r.svFlags & SVF_BOT )
	{
	        // nudge the bot (okay, we lose the fractional part)
		blocker->client->pers.cmd.forwardmove = nudge[0];
		blocker->client->pers.cmd.rightmove = nudge[1];
		blocker->client->pers.cmd.upmove = nudge[2];
		// bots don't double-tap, so use as a nudge flag
		blocker->client->pers.cmd.doubleTap = 1;
	}
	else
	{
		VectorAdd( blocker->client->ps.velocity, nudge, blocker->client->ps.velocity );
		trap_SendServerCommand( blocker - g_entities, "cp \"Don't spawn block!\"" );
        }
}

/*
==================
G_GetBuildPointsInt

Get the number of build points for a team
==================
*/
int G_GetBuildPointsInt( team_t team )
{
	if ( team > TEAM_NONE && team < NUM_TEAMS )
	{
		return ( int )level.team[ team ].buildPoints;
	}
	else
	{
		return 0;
	}
}

/*
==================
G_GetMarkedBuildPointsInt

Get the number of marked build points for a team
==================
*/
int G_GetMarkedBuildPointsInt( team_t team )
{
	gentity_t *ent;
	int       i;
	int       sum = 0;
	const buildableAttributes_t *attr;

	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( ent->s.eType != ET_BUILDABLE || !ent->inuse || ent->health <= 0 || ent->buildableTeam != team || !ent->deconstruct )
		{
			continue;
		}

		attr = BG_Buildable( ent->s.modelindex );
		sum += attr->buildPoints * ( ent->health / (float)attr->health );
	}

	return sum;
}

/*
================
G_Reactor
G_Overmind

Since there's only one of these and we quite often want to find them, cache the
results, but check them for validity each time

The code here will break if more than one reactor or overmind is allowed, even
if one of them is dead/unspawned
================
*/
static gentity_t *FindBuildable( buildable_t buildable );

gentity_t *G_Reactor( void )
{
	static gentity_t *rc;

	// If cache becomes invalid renew it
	if ( !rc || rc->s.eType != ET_BUILDABLE || rc->s.modelindex != BA_H_REACTOR )
	{
		rc = FindBuildable( BA_H_REACTOR );
	}

	return rc;
}

gentity_t *G_AliveReactor( void )
{
	gentity_t *rc = G_Reactor();

	if ( !rc || rc->health <= 0 )
	{
		return NULL;
	}

	return rc;
}

gentity_t *G_ActiveReactor( void )
{
	gentity_t *rc = G_Reactor();

	if ( !rc || rc->health <= 0 || !rc->spawned )
	{
		return NULL;
	}

	return rc;
}

gentity_t *G_Overmind( void )
{
	static gentity_t *om;

	// If cache becomes invalid renew it
	if ( !om || om->s.eType != ET_BUILDABLE || om->s.modelindex != BA_A_OVERMIND )
	{
		om = FindBuildable( BA_A_OVERMIND );
	}

	return om;
}

gentity_t *G_AliveOvermind( void )
{
	gentity_t *om = G_Overmind();

	if ( !om || om->health <= 0 )
	{
		return NULL;
	}

	return om;
}

gentity_t *G_ActiveOvermind( void )
{
	gentity_t *om = G_Overmind();

	if ( !om || om->health <= 0 || !om->spawned )
	{
		return NULL;
	}

	return om;
}

static gentity_t* GetMainBuilding( gentity_t *self, qboolean ownBase )
{
	team_t    team;
	gentity_t *mainBuilding = NULL;

	if ( self->client )
	{
		team = (team_t) self->client->pers.team;
	}
	else if ( self->s.eType == ET_BUILDABLE )
	{
		team = BG_Buildable( self->s.modelindex )->team;
	}
	else
	{
		return NULL;
	}

	if ( ownBase )
	{
		if ( team == TEAM_ALIENS )
		{
			mainBuilding = G_Overmind();
		}
		else if ( team == TEAM_HUMANS )
		{
			mainBuilding = G_Reactor();
		}
	}
	else
	{
		if ( team == TEAM_ALIENS )
		{
			mainBuilding = G_Reactor();
		}
		else if ( team == TEAM_HUMANS )
		{
			mainBuilding = G_Overmind();
		}
	}

	return mainBuilding;
}

/*
================
G_DistanceToBase

Calculates the distance of an entity to its own or the enemy base.
Returns a huge value if the base is not found.
================
*/
float G_DistanceToBase( gentity_t *self, qboolean ownBase )
{
	gentity_t *mainBuilding = GetMainBuilding( self, ownBase );

	if ( mainBuilding )
	{
		return Distance( self->s.origin, mainBuilding->s.origin );
	}
	else
	{
		return 1E+37;
	}
}

/*
================
G_InsideBase
================
*/
#define INSIDE_BASE_MAX_DISTANCE 1000.0f

qboolean G_InsideBase( gentity_t *self, qboolean ownBase )
{
	gentity_t *mainBuilding = GetMainBuilding( self, ownBase );

	if ( !mainBuilding )
	{
		return qfalse;
	}

	if ( Distance( self->s.origin, mainBuilding->s.origin ) >= INSIDE_BASE_MAX_DISTANCE )
	{
		return qfalse;
	}

	return trap_InPVSIgnorePortals( self->s.origin, mainBuilding->s.origin );
}

/*
================
G_FindCreep

attempt to find creep for self, return qtrue if successful
================
*/
qboolean G_FindCreep( gentity_t *self )
{
	int       i;
	gentity_t *ent;
	gentity_t *closestSpawn = NULL;
	int       distance = 0;
	int       minDistance = 10000;
	vec3_t    temp_v;

	//don't check for creep if flying through the air
	if ( !self->client && self->s.groundEntityNum == ENTITYNUM_NONE )
	{
		return qtrue;
	}

	//if self does not have a parentNode or its parentNode is invalid find a new one
	if ( self->client || self->powerSource == NULL || !self->powerSource->inuse ||
	     self->powerSource->health <= 0 )
	{
		for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
		{
			if ( ent->s.eType != ET_BUILDABLE )
			{
				continue;
			}

			if ( ( ent->s.modelindex == BA_A_SPAWN ||
			       ent->s.modelindex == BA_A_OVERMIND ) &&
			       ent->health > 0 )
			{
				VectorSubtract( self->s.origin, ent->s.origin, temp_v );
				distance = VectorLength( temp_v );

				if ( distance < minDistance )
				{
					closestSpawn = ent;
					minDistance = distance;
				}
			}
		}

		if ( minDistance <= CREEP_BASESIZE )
		{
			if ( !self->client )
			{
				self->powerSource = closestSpawn;
			}

			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}

	if ( self->client )
	{
		return qfalse;
	}

	// if we haven't returned by now then we must already have a valid parent
	return qtrue;
}

/**
 * @brief Simple wrapper to G_FindCreep to check if a location has creep.
 */
static qboolean IsCreepHere( vec3_t origin )
{
	gentity_t dummy;

	memset( &dummy, 0, sizeof( gentity_t ) );

	dummy.powerSource = NULL;
	dummy.s.modelindex = BA_NONE;
	VectorCopy( origin, dummy.s.origin );

	return G_FindCreep( &dummy );
}

/**
 * @brief Set any nearby humans' SS_CREEPSLOWED flag.
 */
static void AGeneric_CreepSlow( gentity_t *self )
{
	buildable_t buildable = ( buildable_t )self->s.modelindex;
	float       creepSize = ( float )BG_Buildable( buildable )->creepSize;
	gentity_t   *ent;

	for ( ent = NULL; ( ent = G_IterateEntitiesWithinRadius( ent, self->s.origin, creepSize ) ); )
	{
		if (    !ent->client
		     || ent->client->pers.team != TEAM_HUMANS
		     || ent->client->ps.groundEntityNum == ENTITYNUM_NONE
		     || ent->flags & FL_NOTARGET )
		{
			continue;
		}

		ent->client->ps.stats[ STAT_STATE ] |= SS_CREEPSLOWED;
		ent->client->lastCreepSlowTime = level.time;
	}
}

/**
 * @brief Calculates modifier for the efficiency of one RGS when another one interfers at given distance.
 */
static float RGSInterferenceMod( float distance )
{
	float dr, q;

	if ( RGS_RANGE <= 0.0f )
	{
		return 1.0f;
	}

	// q is the ratio of the part of a sphere with radius RGS_RANGE that intersects
	// with another sphere of equal size and given distance
	dr = distance / RGS_RANGE;
	q = ((dr * dr * dr) - 12.0f * dr + 16.0f) / 16.0f;

	// Two RGS together should mine at a rate proportional to the volume of the
	// union of their areas of effect. If more RGS intersect, this is just an
	// approximation that tends to punish cluttering of RGS.
	return ( (1.0f - q) + 0.5f * q );
}

/**
 * @brief Adjust the rate of a RGS.
 */
void G_RGSCalculateRate( gentity_t *self )
{
	gentity_t       *rgs;
	float           rate;

	if ( self->s.modelindex != BA_A_LEECH && self->s.modelindex != BA_H_DRILL )
	{
		return;
	}

	if ( !self->spawned || !self->powered )
	{
		self->mineRate       = 0.0f;
		self->mineEfficiency = 0.0f;

		// HACK: Save rate and efficiency entityState.weapon and entityState.weaponAnim
		self->s.weapon     = 0;
		self->s.weaponAnim = 0;
	}
	else
	{
		rate = level.mineRate;

		for ( rgs = NULL; ( rgs = G_IterateEntitiesWithinRadius( rgs, self->s.origin, RGS_RANGE * 2.0f ) ); )
		{
			if ( rgs->s.eType == ET_BUILDABLE && ( rgs->s.modelindex == BA_H_DRILL || rgs->s.modelindex == BA_A_LEECH )
				 && rgs != self && rgs->spawned && rgs->powered && rgs->health > 0 )
			{
				rate *= RGSInterferenceMod( Distance( self->s.origin, rgs->s.origin ) );
			}
		}

		self->mineRate       = rate;
		self->mineEfficiency = rate / level.mineRate;

		// HACK: Save rate and efficiency in entityState.weapon and entityState.weaponAnim
		self->s.weapon     = ( int )( self->mineRate * 1000.0f );
		self->s.weaponAnim = ( int )( self->mineEfficiency * 100.0f );

		// The transmitted rate must be positive to indicate that the RGS is active
		if ( self->s.weapon < 1 )
		{
			self->s.weapon = 1;
		}
	}
}

/**
 * @brief Adjust the rate of all RGS in range.
 */
void G_RGSInformNeighbors( gentity_t *self )
{
	gentity_t       *rgs;

	for ( rgs = NULL; ( rgs = G_IterateEntitiesWithinRadius( rgs, self->s.origin, RGS_RANGE * 2.0f ) ); )
	{
		if ( rgs->s.eType == ET_BUILDABLE && ( rgs->s.modelindex == BA_H_DRILL || rgs->s.modelindex == BA_A_LEECH )
			 && rgs != self && rgs->spawned && rgs->powered && rgs->health > 0 )
		{
			G_RGSCalculateRate( rgs );
		}
	}
}

/**
 * @brief Predict the efficiecy loss of a RGS if another one is constructed at given origin.
 * @return Efficiency loss as negative value
 */
static float RGSPredictInterferenceLoss( gentity_t *self, vec3_t origin )
{
	float currentRate, predictedRate, rateLoss;

	currentRate   = self->mineRate;
	predictedRate = currentRate * RGSInterferenceMod( Distance( self->s.origin, origin ) );
	rateLoss      = predictedRate - currentRate;

	return ( rateLoss / level.mineRate );
}

/**
 * @brief Predict the efficiency of a RGS constructed at the given point.
 * @return Predicted efficiency in percent points
 */
float G_RGSPredictEfficiency( vec3_t origin )
{
	gentity_t dummy;

	memset( &dummy, 0, sizeof( gentity_t ) );
	VectorCopy( origin, dummy.s.origin );
	dummy.s.eType = ET_BUILDABLE;
	dummy.s.modelindex = BA_A_LEECH;
	dummy.spawned = qtrue;
	dummy.powered = qtrue;

	G_RGSCalculateRate( &dummy );

	return dummy.mineEfficiency;
}

/**
 * @brief Predict the total efficiency gain for a team when a RGS is constructed at the given point.
 * @return Predicted efficiency delta in percent points
 * @todo Consider RGS set for deconstruction
 */
float G_RGSPredictEfficiencyDelta( vec3_t origin, team_t team )
{
	gentity_t *rgs;
	float     delta;

	delta = G_RGSPredictEfficiency( origin );

	for ( rgs = NULL; ( rgs = G_IterateEntitiesWithinRadius( rgs, origin, RGS_RANGE * 2.0f ) ); )
	{
		if ( rgs->s.eType == ET_BUILDABLE && ( rgs->s.modelindex == BA_H_DRILL || rgs->s.modelindex == BA_A_LEECH )
		     && rgs->spawned && rgs->powered && rgs->health > 0 && rgs->buildableTeam == team )
		{
			delta += RGSPredictInterferenceLoss( rgs, origin );
		}
	}

	return delta;
}

/*
================
nullDieFunction

hack to prevent compilers complaining about function pointer -> NULL conversion
================
*/
static void NullDieFunction( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod ) {}

/*
================
CompareEntityDistance

Sorts entities by distance, lowest first.
Input are two indices for g_entities.
================
*/
static vec3_t compareEntityDistanceOrigin;
static int    CompareEntityDistance( const void *a, const void *b )
{
	gentity_t *aEnt, *bEnt;
	vec3_t    origin;
	vec_t     aDistance, bDistance;

	aEnt = &g_entities[ *( int* )a ];
	bEnt = &g_entities[ *( int* )b ];

	VectorCopy( compareEntityDistanceOrigin, origin );

	aDistance = Distance( origin, aEnt->s.origin );
	bDistance = Distance( origin, bEnt->s.origin );

	if ( aDistance < bDistance )
	{
		return -1;
	}
	else if ( aDistance > bDistance )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/*
================
AGeneric_CreepRecede

Called when an alien buildable dies
================
*/
void AGeneric_CreepRecede( gentity_t *self )
{
	//if the creep just died begin the recession
	if ( !( self->s.eFlags & EF_DEAD ) )
	{
		self->s.eFlags |= EF_DEAD;

		G_RewardAttackers( self );

		G_AddEvent( self, EV_BUILD_DESTROY, 0 );

		if ( self->spawned )
		{
			self->s.time = -level.time;
		}
		else
		{
			self->s.time = - ( level.time -
			                   ( int )( ( float ) CREEP_SCALEDOWN_TIME *
			                            ( 1.0f - ( ( float )( level.time - self->creationTime ) /
			                                       ( float ) BG_Buildable( self->s.modelindex )->buildTime ) ) ) );
		}
	}

	//creep is still receeding
	if ( ( self->timestamp + 10000 ) > level.time )
	{
		self->nextthink = level.time + 500;
	}
	else //creep has died
	{
		G_FreeEntity( self );
	}
}

/*
================
AGeneric_Blast

Called when an Alien buildable explodes after dead state
================
*/
void AGeneric_Blast( gentity_t *self )
{
	vec3_t dir;

	VectorCopy( self->s.origin2, dir );

	//do a bit of radius damage
	G_SelectiveRadiusDamage( self->s.pos.trBase, g_entities + self->killedBy, self->splashDamage,
	                         self->splashRadius, self, self->splashMethodOfDeath, TEAM_ALIENS );

	//pretty events and item cleanup
	self->s.eFlags |= EF_NODRAW; //don't draw the model once it's destroyed
	G_AddEvent( self, EV_ALIEN_BUILDABLE_EXPLOSION, DirToByte( dir ) );
	self->timestamp = level.time;
	self->think = AGeneric_CreepRecede;
	self->nextthink = level.time + 500;

	self->r.contents = 0; //stop collisions...
	trap_LinkEntity( self );  //...requires a relink
}

/*
================
AGeneric_Die

Called when an Alien buildable is killed and enters a brief dead state prior to
exploding.
================
*/
void AGeneric_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	gentity_t *om;

	G_SetBuildableAnim( self, self->powered ? BANIM_DESTROY1 : BANIM_DESTROY_UNPOWERED, qtrue );
	G_SetIdleBuildableAnim( self, BANIM_DESTROYED );

	self->die = NullDieFunction;
	self->killedBy = attacker - g_entities;
	self->think = AGeneric_Blast;
	self->s.eFlags &= ~EF_FIRING; //prevent any firing effects
	self->powered = qfalse;

	// warn if in main base and there's an overmind
	if ( ( om = G_ActiveOvermind() ) && om != self && level.time > om->warnTimer
	     && G_InsideBase( self, qtrue ) && IsWarnableMOD( mod ) )
	{
		om->warnTimer = level.time + NEARBY_ATTACK_PERIOD; // don't spam
		G_BroadcastEvent( EV_WARN_ATTACK, 0, TEAM_ALIENS );
	}

	// fully grown and not blasted to smithereens
	if ( self->spawned && -self->health < BG_Buildable( self->s.modelindex )->health )
	{
		// blast after brief period
		self->nextthink = level.time + ALIEN_DETONATION_DELAY
		                  + ( ( rand() - ( RAND_MAX / 2 ) ) / ( float )( RAND_MAX / 2 ) )
		                  * DETONATION_DELAY_RAND_RANGE * ALIEN_DETONATION_DELAY;
	}
	else
	{
		// blast immediately
		self->nextthink = level.time;
	}

	G_LogDestruction( self, attacker, mod );
}

/*
================
AGeneric_CreepCheck

Tests for creep and kills the buildable if there is none
================
*/
void AGeneric_CreepCheck( gentity_t *self )
{
	gentity_t *spawn;

	if ( !BG_Buildable( self->s.modelindex )->creepTest )
	{
		return;
	}

	if ( !G_FindCreep( self ) )
	{
		spawn = self->powerSource;

		if ( spawn )
		{
			G_Damage( self, NULL, g_entities + spawn->killedBy, NULL, NULL, self->health, 0, MOD_NOCREEP );
		}
		else
		{
			G_Damage( self, NULL, NULL, NULL, NULL, self->health, 0, MOD_NOCREEP );
		}
	}
}

/*
================
G_IgniteBuildable

Sets an alien buildable on fire.
================
*/
void G_IgniteBuildable( gentity_t *self, gentity_t *fireStarter )
{
	if ( self->s.eType != ET_BUILDABLE || self->buildableTeam != TEAM_ALIENS )
	{
		return;
	}

	if ( !self->onFire && level.time > self->fireImmunityUntil )
	{
		if ( g_debugFire.integer )
		{
			char selfDescr[ 64 ], fireStarterDescr[ 64 ];
			BG_BuildEntityDescription( selfDescr, sizeof( selfDescr ), &self->s );
			BG_BuildEntityDescription( fireStarterDescr, sizeof( fireStarterDescr ), &fireStarter->s );
			Com_Printf("%s ^2ignited^7 %s.", fireStarterDescr, selfDescr);
		}

		self->onFire               = qtrue;
		self->fireStarter          = fireStarter;
		self->nextBurnDamage       = level.time + BURN_SELFDAMAGE_PERIOD * BURN_PERIODS_RAND_MOD;
		self->nextBurnSplashDamage = level.time + BURN_SPLDAMAGE_PERIOD  * BURN_PERIODS_RAND_MOD;
		self->nextBurnAction       = level.time + BURN_ACTION_PERIOD     * BURN_PERIODS_RAND_MOD;
	}
	else if ( self->onFire )
	{
		if ( g_debugFire.integer )
		{
			char selfDescr[ 64 ], fireStarterDescr[ 64 ];
			BG_BuildEntityDescription( selfDescr, sizeof( selfDescr ), &self->s );
			BG_BuildEntityDescription( fireStarterDescr, sizeof( fireStarterDescr ), &fireStarter->s );
			Com_Printf("%s ^2reset burning action timer of^7 %s.", fireStarterDescr, selfDescr);
		}

		// always reset the action timer
		// this leads to prolonged burning but slow spread in burning groups
		self->nextBurnAction = level.time + BURN_ACTION_PERIOD * BURN_PERIODS_RAND_MOD;
	}
}

/*
================
AGeneric_Think

A generic think function for Alien buildables
================
*/
void AGeneric_Think( gentity_t *self )
{
	// set power state
	self->powered = ( G_ActiveOvermind() != NULL );

	// check if still on creep
	AGeneric_CreepCheck( self );

	// slow down close humans
	AGeneric_CreepSlow( self );

	// burn if on fire
	if ( self->onFire )
	{
		G_FireThink( self );
	}
}

/*
================
AGeneric_Pain

A generic pain function for Alien buildables
================
*/
void AGeneric_Pain( gentity_t *self, gentity_t *attacker, int damage )
{
	if ( self->health <= 0 )
	{
		return;
	}

	// Alien buildables only have the first pain animation defined
	G_SetBuildableAnim( self, BANIM_PAIN1, qfalse );
}

//==================================================================================

/*
================
ASpawn_Think

think function for Alien Spawn
================
*/
void ASpawn_Think( gentity_t *self )
{
	gentity_t *ent;

	self->nextthink = level.time + 100;

	AGeneric_Think( self );

	if ( !self->spawned )
	{
		return;
	}

	if ( self->s.groundEntityNum != ENTITYNUM_NONE )
	{
		if ( ( ent = G_CheckSpawnPoint( self->s.number, self->s.origin,
										self->s.origin2, BA_A_SPAWN, NULL ) ) != NULL )
		{
			// If the thing blocking the spawn is a buildable, kill it.
			// If it's part of the map, kill self.
			if ( ent->s.eType == ET_BUILDABLE )
			{
				if ( ent->builtBy && ent->builtBy->slot >= 0 ) // don't queue the bp from this
				{
					G_Damage( ent, NULL, g_entities + ent->builtBy->slot, NULL, NULL, 10000, 0, MOD_SUICIDE );
				}
				else
				{
					G_Damage( ent, NULL, NULL, NULL, NULL, 10000, 0, MOD_SUICIDE );
				}

				G_SetBuildableAnim( self, BANIM_SPAWN1, qtrue );
			}
			else if ( ent->s.number == ENTITYNUM_WORLD || ent->s.eType == ET_MOVER )
			{
				G_Damage( self, NULL, NULL, NULL, NULL, 10000, 0, MOD_SUICIDE );
				return;
			}
			else if( g_antiSpawnBlock.integer &&
					 ent->client && ent->client->pers.team == TEAM_ALIENS )
			{
				PuntBlocker( self, ent );
			}

			if ( ent->s.eType == ET_CORPSE )
			{
				G_FreeEntity( ent );  //quietly remove
			}
		}
		else
		{
			self->spawnBlockTime = 0;
		}
	}
}

#define OVERMIND_DYING_PERIOD  5000
#define OVERMIND_SPAWNS_PERIOD 30000

/*
================
AOvermind_Think

Think function for Alien Overmind
================
*/
void AOvermind_Think( gentity_t *self )
{
	int clientNum;

	self->nextthink = level.time + 1000;

	AGeneric_Think( self );

	if ( self->spawned && ( self->health > 0 ) )
	{
		//do some damage
		if ( G_SelectiveRadiusDamage( self->s.pos.trBase, self, self->splashDamage,
		                              self->splashRadius, self, MOD_OVERMIND, TEAM_ALIENS ) )
		{
			self->timestamp = level.time;
			G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
		}

		// just in case an egg finishes building after we tell overmind to stfu
		if ( level.team[ TEAM_ALIENS ].numSpawns > 0 )
		{
			level.overmindMuted = qfalse;
		}

		// shut up during intermission
		if ( level.intermissiontime )
		{
			level.overmindMuted = qtrue;
		}

		//low on spawns
		if ( !level.overmindMuted && level.team[ TEAM_ALIENS ].numSpawns <= 0 &&
		     level.time > self->overmindSpawnsTimer )
		{
			qboolean  haveBuilder = qfalse;
			gentity_t *builder;

			self->overmindSpawnsTimer = level.time + OVERMIND_SPAWNS_PERIOD;
			G_BroadcastEvent( EV_OVERMIND_SPAWNS, 0, TEAM_ALIENS );

			for ( clientNum = 0; clientNum < level.numConnectedClients; clientNum++ )
			{
				builder = &g_entities[ level.sortedClients[ clientNum ] ];

				if ( builder->health > 0 &&
				     ( builder->client->pers.classSelection == PCL_ALIEN_BUILDER0 ||
				       builder->client->pers.classSelection == PCL_ALIEN_BUILDER0_UPG ) )
				{
					haveBuilder = qtrue;
					break;
				}
			}

			// aliens now know they have no eggs, but they're screwed, so stfu
			if ( !haveBuilder )
			{
				level.overmindMuted = qtrue;
			}
		}

		G_WarnPrimaryUnderAttack( self );
	}
	else
	{
		self->overmindSpawnsTimer = level.time + OVERMIND_SPAWNS_PERIOD;
	}
}

/*
================
AOvermind_Die

Called when the overmind dies
================
*/
void AOvermind_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	AGeneric_Die( self, inflictor, attacker, mod );
	if ( IsWarnableMOD( mod ) )
	{
		G_BroadcastEvent( EV_OVERMIND_DYING, 0, TEAM_ALIENS );
	}
}

/*
================
ABarricade_Pain

Barricade pain animation depends on shrunk state
================
*/
void ABarricade_Pain( gentity_t *self, gentity_t *attacker, int damage )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if ( !self->shrunkTime )
	{
		G_SetBuildableAnim( self, BANIM_PAIN1, qfalse );
	}
	else
	{
		G_SetBuildableAnim( self, BANIM_PAIN2, qfalse );
	}
}

/*
================
ABarricade_Shrink

Set shrink state for a barricade. When unshrinking, checks to make sure there
is enough room.
================
*/
void ABarricade_Shrink( gentity_t *self, qboolean shrink )
{
	if ( !self->spawned || self->health <= 0 )
	{
		shrink = qtrue;
	}

	if ( shrink && self->shrunkTime )
	{
		int anim;

		// We need to make sure that the animation has been set to shrunk mode
		// because we start out shrunk but with the construct animation when built
		self->shrunkTime = level.time;
		anim = self->s.torsoAnim & ~( ANIM_FORCEBIT | ANIM_TOGGLEBIT );

		if ( self->spawned && self->health > 0 && anim != BANIM_DESTROYED )
		{
			G_SetIdleBuildableAnim( self, BANIM_DESTROYED );
			G_SetBuildableAnim( self, BANIM_ATTACK1, qtrue );
		}

		return;
	}

	if ( !shrink && ( !self->shrunkTime ||
	                  level.time < self->shrunkTime + BARRICADE_SHRINKTIMEOUT ) )
	{
		return;
	}

	BG_BuildableBoundingBox( BA_A_BARRICADE, self->r.mins, self->r.maxs );

	if ( shrink )
	{
		self->r.maxs[ 2 ] = ( int )( self->r.maxs[ 2 ] * BARRICADE_SHRINKPROP );
		self->shrunkTime = level.time;

		// shrink animation, the destroy animation is used
		if ( self->spawned && self->health > 0 )
		{
			G_SetBuildableAnim( self, BANIM_ATTACK1, qtrue );
			G_SetIdleBuildableAnim( self, BANIM_DESTROYED );
		}
	}
	else
	{
		trace_t tr;
		int     anim;

		trap_Trace( &tr, self->s.origin, self->r.mins, self->r.maxs,
		            self->s.origin, self->s.number, MASK_PLAYERSOLID );

		if ( tr.startsolid || tr.fraction < 1.0f )
		{
			self->r.maxs[ 2 ] = ( int )( self->r.maxs[ 2 ] * BARRICADE_SHRINKPROP );
			return;
		}

		self->shrunkTime = 0;

		// unshrink animation, IDLE2 has been hijacked for this
		anim = self->s.legsAnim & ~( ANIM_FORCEBIT | ANIM_TOGGLEBIT );

		if ( self->spawned && self->health > 0 &&
		     anim != BANIM_CONSTRUCT1 && anim != BANIM_CONSTRUCT2 )
		{
			G_SetIdleBuildableAnim( self, (buildableAnimNumber_t) BG_Buildable( BA_A_BARRICADE )->idleAnim );
			G_SetBuildableAnim( self, BANIM_ATTACK2, qtrue );
		}
	}

	// a change in size requires a relink
	if ( self->spawned )
	{
		trap_LinkEntity( self );
	}
}

/*
================
ABarricade_Die

Called when an alien barricade dies
================
*/
void ABarricade_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	AGeneric_Die( self, inflictor, attacker, mod );
	ABarricade_Shrink( self, qtrue );
}

/*
================
ABarricade_Think

Think function for Alien Barricade
================
*/
void ABarricade_Think( gentity_t *self )
{
	self->nextthink = level.time + 1000;

	AGeneric_Think( self );

	// Shrink if unpowered
	ABarricade_Shrink( self, !self->powered );
}

/*
================
ABarricade_Touch

Barricades shrink when they are come into contact with an Alien that can
pass through
================
*/

void ABarricade_Touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	gclient_t *client = other->client;
	int       client_z, min_z;

	if ( !client || client->pers.team != TEAM_ALIENS )
	{
		return;
	}

	// Client must be high enough to pass over. Note that STEPSIZE (18) is
	// hardcoded here because we don't include bg_local.h!
	client_z = other->s.origin[ 2 ] + other->r.mins[ 2 ];
	min_z = self->s.origin[ 2 ] - 18 +
	        ( int )( self->r.maxs[ 2 ] * BARRICADE_SHRINKPROP );

	if ( client_z < min_z )
	{
		return;
	}

	ABarricade_Shrink( self, qtrue );
}

//==================================================================================

/*
================
AAcidTube_Think

Think function for Alien Acid Tube
================
*/
void AAcidTube_Think( gentity_t *self )
{
	gentity_t *ent;

	// TODO: Make damage independent of think timer
	self->nextthink = level.time + ACIDTUBE_REPEAT;

	AGeneric_Think( self );

	if ( !self->spawned || !self->powered || self->health <= 0 )
	{
		return;
	}

	// check for close humans
	for ( ent = NULL; ( ent = G_IterateEntitiesWithinRadius( ent, self->s.origin, ACIDTUBE_RANGE ) ); )
	{
		if (    ( ent->flags & FL_NOTARGET )
		     || !ent->client
		     || ent->client->pers.team != TEAM_HUMANS )
		{
			continue;
		}

		// this is potentially expensive, so check this last
		if ( !G_IsVisible( self, ent, CONTENTS_SOLID ) )
		{
			continue;
		}

		if ( level.time >= self->timestamp + ACIDTUBE_REPEAT_ANIM )
		{
			self->timestamp = level.time;
			G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
			G_AddEvent( self, EV_ALIEN_ACIDTUBE, DirToByte( self->s.origin2 ) );
		}

		G_SelectiveRadiusDamage( self->s.pos.trBase, self, ACIDTUBE_DAMAGE,
								 ACIDTUBE_RANGE, self, MOD_ATUBE, TEAM_ALIENS );

		return;
	}
}

void ALeech_Think( gentity_t *self )
{
	qboolean active, lastThinkActive;
	float    resources;

	self->nextthink = level.time + 1000;

	AGeneric_Think( self );

	active = self->spawned & self->powered;
	lastThinkActive = self->s.weapon > 0;

	if ( active ^ lastThinkActive )
	{
		// If the state of this RGS has changed, adjust own rate and inform
		// active closeby RGS so they can adjust their rate immediately.
		G_RGSCalculateRate( self );
		G_RGSInformNeighbors( self );
	}

	if ( active )
	{
		resources = self->s.weapon / 60000.0f;
		self->buildableStatsTotalF += resources;
	}
}

void ALeech_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	AGeneric_Die( self, inflictor, attacker, mod );

	self->s.weapon = 0;
	self->s.weaponAnim = 0;

	// Assume our state has changed and inform closeby RGS
	G_RGSInformNeighbors( self );
}

static gentity_t *cmpHive = NULL;

static int AHive_CompareTargets( const void *first, const void *second )
{
	gentity_t *a, *b;

	if ( !cmpHive )
	{
		return 0;
	}

	a = ( gentity_t * )first;
	b = ( gentity_t * )second;

	// Always prefer target that isn't yet targeted.
	{
		if ( a->numTrackedBy == 0 && b->numTrackedBy > 0 )
		{
			return -1;
		}
		else if ( a->numTrackedBy > 0 && b->numTrackedBy == 0 )
		{
			return 1;
		}
	}

	// Prefer smaller distance.
	{
		int ad = Distance( cmpHive->s.origin, a->s.origin );
		int bd = Distance( cmpHive->s.origin, b->s.origin );

		if ( ad < bd )
		{
			return -1;
		}
		else if ( bd < ad )
		{
			return 1;
		}
	}

	// Tie breaker is random decision, so clients on lower slots don't get shot at more often.
	{
		if ( rand() < ( RAND_MAX / 2 ) )
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
}

static qboolean AHive_TargetValid( gentity_t *self, gentity_t *target, qboolean ignoreDistance )
{
	trace_t trace;
	vec3_t  tipOrigin;

	if (    !target
	     || !target->client
	     || target->client->pers.team != TEAM_HUMANS
	     || target->health <= 0
	     || ( target->flags & FL_NOTARGET ) )
	{
		return qfalse;
	}

	// use tip instead of origin for distance and line of sight checks
	// TODO: Evaluate
	VectorMA( self->s.pos.trBase, self->r.maxs[ 2 ], self->s.origin2, tipOrigin );

	// check if enemy in sense range
	if ( !ignoreDistance && Distance( tipOrigin, target->s.origin ) > HIVE_SENSE_RANGE )
	{
		return qfalse;
	}

	// check for clear line of sight
	trap_Trace( &trace, tipOrigin, NULL, NULL, target->s.pos.trBase, self->s.number, MASK_SHOT );

	if ( trace.fraction == 1.0f || trace.entityNum != target->s.number )
	{
		return qfalse;
	}

	return qtrue;
}

/**
 * @brief Forget about old target and attempt to find a new valid one.
 * @return Whether a valid target was found.
 */
static qboolean AHive_FindTarget( gentity_t *self )
{
	gentity_t *ent = NULL;
	gentity_t *validTargets[ MAX_GENTITIES ];
	int       validTargetNum = 0;

	// delete old target
	if ( self->target )
	{
		self->target->numTrackedBy--;
	}

	self->target = NULL;

	// find all potential targets
	for ( ent = NULL; ( ent = G_IterateEntitiesWithinRadius( ent, self->s.origin, HIVE_SENSE_RANGE )); )
	{
		if ( AHive_TargetValid( self, ent, qfalse ) )
		{
		     validTargets[ validTargetNum++ ] = ent;
		}
	}

	if ( validTargetNum > 0 )
	{
		// search best target
		cmpHive = self;
		qsort( validTargets, validTargetNum, sizeof( gentity_t* ), AHive_CompareTargets );

		self->target = validTargets[ 0 ];
		self->target->numTrackedBy++;

		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/**
 * @brief Fires a hive missile on the target, which is assumed to be valid.
 */
static void AHive_Fire( gentity_t *self )
{
	vec3_t dirToTarget;

	if ( !self->target )
	{
		return;
	}

	// set self active, this will be reset after the missile dies or a timeout
	self->active = qtrue;
	self->timestamp = level.time + HIVE_REPEAT;

	// set aim angles
	VectorSubtract( self->target->s.pos.trBase, self->s.pos.trBase, dirToTarget );
	VectorNormalize( dirToTarget );
	vectoangles( dirToTarget, self->buildableAim );

	// fire
	G_FireWeapon( self, WP_HIVE, WPM_PRIMARY );
	G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
}

void AHive_Think( gentity_t *self )
{
	self->nextthink = level.time + 500;

	AGeneric_Think( self );

	if ( !self->spawned || !self->powered || self->health <= 0 )
	{
		return;
	}

	// last missile hasn't returned in time, forget about it
	if ( self->timestamp < level.time )
	{
		self->active = qfalse;
	}

	if ( self->active )
	{
		return;
	}

	// attempt to find a new target and fire once at it if successful
	if ( AHive_FindTarget( self ) )
	{
		AHive_Fire( self );
	}
}

void AHive_Pain( gentity_t *self, gentity_t *attacker, int damage )
{
	AGeneric_Pain( self, attacker, damage );

	// if inactive, fire on attacker even if it's out of sense range
	if ( self->spawned && self->health > 0 && !self->active )
	{
		if ( AHive_TargetValid( self, attacker, qtrue ) )
		{
			self->target = attacker;
			attacker->numTrackedBy++;

			AHive_Fire( self );
		}
	}
}

void ABooster_Think( gentity_t *self )
{
	gentity_t *ent;
	qboolean  playHealingEffect = qfalse;

	self->nextthink = level.time + BOOST_REPEAT_ANIM / 4;

	AGeneric_Think( self );

	// check if there is a closeby alien that used this booster for healing recently
	for ( ent = NULL; ( ent = G_IterateEntitiesWithinRadius( ent, self->s.origin, REGEN_BOOST_RANGE ) ); )
	{
		if ( ent->boosterUsed == self && ent->boosterTime == level.previousTime )
		{
			playHealingEffect = qtrue;
			break;
		}
	}

	if ( playHealingEffect )
	{
		if ( level.time > self->timestamp + BOOST_REPEAT_ANIM )
		{
			self->timestamp = level.time;
			G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
			G_AddEvent( self, EV_ALIEN_BOOSTER, DirToByte( self->s.origin2 ) );
		}
	}
}

void ABooster_Touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	gclient_t *client = other->client;

	if ( !self->spawned || !self->powered || self->health <= 0 )
	{
		return;
	}

	if ( !client )
	{
		return;
	}

	if ( client->pers.team == TEAM_HUMANS )
	{
		return;
	}

	if ( other->flags & FL_NOTARGET )
	{
		return; // notarget cancels even beneficial effects?
	}

	if ( level.time - client->boostedTime > BOOST_TIME )
	{
		self->buildableStatsCount++;
	}

	client->ps.stats[ STAT_STATE ] |= SS_BOOSTED;
	client->ps.stats[ STAT_STATE ] |= SS_BOOSTEDNEW;
	client->boostedTime = level.time;
}

#define MISSILE_PRESTEP_TIME 50 // from g_missile.h
#define TRAPPER_ACCURACY 10 // lower is better

/*
================
ATrapper_FireOnEnemy

Used by ATrapper_Think to fire at enemy
================
*/
void ATrapper_FireOnEnemy( gentity_t *self, int firespeed, float range )
{
	gentity_t *target = self->target;
	vec3_t    dirToTarget;
	vec3_t    halfAcceleration, thirdJerk;
	float     distanceToTarget = LOCKBLOB_RANGE;
	int       lowMsec = 0;
	int       highMsec = ( int )( (
	                                ( ( distanceToTarget * LOCKBLOB_SPEED ) +
	                                  ( distanceToTarget * BG_Class( target->client->ps.stats[ STAT_CLASS ] )->speed ) ) /
	                                ( LOCKBLOB_SPEED * LOCKBLOB_SPEED ) ) * 1000.0f );

	VectorScale( target->acceleration, 1.0f / 2.0f, halfAcceleration );
	VectorScale( target->jerk, 1.0f / 3.0f, thirdJerk );

	// highMsec and lowMsec can only move toward
	// one another, so the loop must terminate
	while ( highMsec - lowMsec > TRAPPER_ACCURACY )
	{
		int   partitionMsec = ( highMsec + lowMsec ) / 2;
		float time = ( float ) partitionMsec / 1000.0f;
		float projectileDistance = LOCKBLOB_SPEED * ( time + MISSILE_PRESTEP_TIME / 1000.0f );

		VectorMA( target->s.pos.trBase, time, target->s.pos.trDelta, dirToTarget );
		VectorMA( dirToTarget, time * time, halfAcceleration, dirToTarget );
		VectorMA( dirToTarget, time * time * time, thirdJerk, dirToTarget );
		VectorSubtract( dirToTarget, self->s.pos.trBase, dirToTarget );
		distanceToTarget = VectorLength( dirToTarget );

		if ( projectileDistance < distanceToTarget )
		{
			lowMsec = partitionMsec;
		}
		else if ( projectileDistance > distanceToTarget )
		{
			highMsec = partitionMsec;
		}
		else if ( projectileDistance == distanceToTarget )
		{
			break; // unlikely to happen
		}
	}

	VectorNormalize( dirToTarget );
	vectoangles( dirToTarget, self->buildableAim );

	//fire at target
	G_FireWeapon( self, WP_LOCKBLOB_LAUNCHER, WPM_PRIMARY );
	G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
	self->customNumber = level.time + firespeed;
}

/*
================
ATrapper_CheckTarget

Used by ATrapper_Think to check enemies for validity
================
*/
qboolean ATrapper_CheckTarget( gentity_t *self, gentity_t *target, int range )
{
	vec3_t  distance;
	trace_t trace;

	if ( !target ) // Do we have a target?
	{
		return qfalse;
	}

	if ( !target->inuse ) // Does the target still exist?
	{
		return qfalse;
	}

	if ( target == self ) // is the target us?
	{
		return qfalse;
	}

	if ( !target->client ) // is the target a bot or player?
	{
		return qfalse;
	}

	if ( target->flags & FL_NOTARGET ) // is the target cheating?
	{
		return qfalse;
	}

	if ( target->client->pers.team == TEAM_ALIENS ) // one of us?
	{
		return qfalse;
	}

	if ( target->client->sess.spectatorState != SPECTATOR_NOT ) // is the target alive?
	{
		return qfalse;
	}

	if ( target->health <= 0 ) // is the target still alive?
	{
		return qfalse;
	}

	if ( target->client->ps.stats[ STAT_STATE ] & SS_BLOBLOCKED ) // locked?
	{
		return qfalse;
	}

	VectorSubtract( target->r.currentOrigin, self->r.currentOrigin, distance );

	if ( VectorLength( distance ) > range )  // is the target within range?
	{
		return qfalse;
	}

	//only allow a narrow field of "vision"
	VectorNormalize( distance );  //is now direction of target

	if ( DotProduct( distance, self->s.origin2 ) < LOCKBLOB_DOT )
	{
		return qfalse;
	}

	trap_Trace( &trace, self->s.pos.trBase, NULL, NULL, target->s.pos.trBase, self->s.number, MASK_SHOT );

	if ( trace.contents & CONTENTS_SOLID ) // can we see the target?
	{
		return qfalse;
	}

	return qtrue;
}

/*
================
ATrapper_FindEnemy

Used by ATrapper_Think to locate enemy gentities
================
*/
void ATrapper_FindEnemy( gentity_t *ent, int range )
{
	gentity_t *target;
	int       i;
	int       start;

	// iterate through entities
	start = rand() / ( RAND_MAX / MAX_CLIENTS + 1 );

	for ( i = start; i < MAX_CLIENTS + start; i++ )
	{
		target = g_entities + ( i % MAX_CLIENTS );

		//if target is not valid keep searching
		if ( !ATrapper_CheckTarget( ent, target, range ) )
		{
			continue;
		}

		//we found a target
		ent->target = target;
		return;
	}

	//couldn't find a target
	ent->target = NULL;
}

/*
================
ATrapper_Think

think function for Alien Defense
================
*/
void ATrapper_Think( gentity_t *self )
{
	self->nextthink = level.time + 100;

	AGeneric_Think( self );

	if ( !self->spawned || !self->powered || self->health <= 0 )
	{
		return;
	}

	//if the current target is not valid find a new one
	if ( !ATrapper_CheckTarget( self, self->target, LOCKBLOB_RANGE ) )
	{
		ATrapper_FindEnemy( self, LOCKBLOB_RANGE );
	}

	//if a new target cannot be found don't do anything
	if ( !self->target )
	{
		return;
	}

	//if we are pointing at our target and we can fire shoot it
	if ( self->customNumber < level.time )
	{
		ATrapper_FireOnEnemy( self, LOCKBLOB_REPEAT, LOCKBLOB_RANGE );
	}
}

/*
================
IdlePowerState

Set buildable idle animation to match power state
================
*/
static void IdlePowerState( gentity_t *self )
{
	if ( self->powered )
	{
		if ( self->s.torsoAnim == BANIM_IDLE_UNPOWERED )
		{
			// TODO: Play power up anim here?
			G_SetIdleBuildableAnim( self, (buildableAnimNumber_t) BG_Buildable( self->s.modelindex )->idleAnim );
		}
	}
	else
	{
		if ( self->s.torsoAnim != BANIM_IDLE_UNPOWERED )
		{
			G_SetBuildableAnim( self, BANIM_POWERDOWN, qfalse );
			G_SetIdleBuildableAnim( self, BANIM_IDLE_UNPOWERED );
		}
	}
}

/*
================
PowerRelevantRange

Determines relvant range for power related entities.
================
*/
static int PowerRelevantRange()
{
	static int relevantRange = 1;
	static int lastCalculation = 0;

	// only calculate once per server frame
	if ( lastCalculation == level.time )
	{
		return relevantRange;
	}

	lastCalculation = level.time;

	relevantRange = 1;
	relevantRange = MAX( relevantRange, g_powerReactorRange.integer );
	relevantRange = MAX( relevantRange, g_powerRepeaterRange.integer );
	relevantRange = MAX( relevantRange, g_powerCompetitionRange.integer );

	return relevantRange;
}

/*
================
NearestPowerSourceInRange

Check if origin is affected by reactor or repeater power.
If it is, return whichever is nearest (preferring reactor).
================
*/
static gentity_t *NearestPowerSourceInRange( gentity_t *self )
{
	gentity_t *neighbor = G_ActiveReactor();
	gentity_t *best = NULL;
	float bestDistance = HUGE_QFLT;

	if ( neighbor && Distance( self->s.origin, neighbor->s.origin ) <= g_powerReactorRange.integer )
	{
		return neighbor;
	}

	neighbor = NULL;

	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, self->s.origin, PowerRelevantRange() ) ) )
	{
		if ( neighbor->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( neighbor->s.modelindex == BA_H_REPEATER )
		{
			float distance = Distance( self->s.origin, neighbor->s.origin );
			if ( distance < bestDistance )
			{
				bestDistance = distance;
				best = neighbor;
			}
		}
	}

	return ( bestDistance <= g_powerRepeaterRange.integer ) ? best : NULL;
}

/*
=================
IncomingInterference

Calculates the amount of power a buildable recieves from or loses to a neighbor at given distance.
=================
*/
static float IncomingInterference( buildable_t buildable, gentity_t *neighbor,
                                   float distance, qboolean prediction )
{
	float range, power;

	// buildables that need no power don't receive interference
	if ( !BG_Buildable( buildable )->powerConsumption )
	{
		return 0.0f;
	}

	if ( !neighbor )
	{
		return 0.0f;
	}

	// interference from human buildables
	if ( neighbor->s.eType == ET_BUILDABLE && neighbor->buildableTeam == TEAM_HUMANS )
	{
		// Only take unpowered and constructing buildables in consideration when predicting
		if ( !prediction && ( !neighbor->spawned || !neighbor->powered ) )
		{
			return 0.0f;
		}

		switch ( neighbor->s.modelindex )
		{
			case BA_H_REPEATER:
				if ( neighbor->health > 0 )
				{
					power = g_powerRepeaterSupply.integer;
					range = g_powerRepeaterRange.integer;
					break;
				}
				else
				{
					return 0.0f;
				}

			case BA_H_REACTOR:
				if ( neighbor->health > 0 )
				{
					power = g_powerReactorSupply.integer;
					range = g_powerReactorRange.integer;
					break;
				}
				else
				{
					return 0.0f;
				}

			default:
				power = -BG_Buildable( neighbor->s.modelindex )->powerConsumption;
				range = g_powerCompetitionRange.integer;
		}
	}
	else
	{
		return 0.0f;
	}

	// sanity check range
	if ( range <= 0.0f )
	{
		return 0.0f;
	}

	return power * MAX( 0.0f, 1.0f - ( distance / range ) );
}

/*
=================
OutgoingInterference

Calculates the amount of power a buildable gives to or takes from a neighbor at given distance.
=================
*/
static float OutgoingInterference( buildable_t buildable, gentity_t *neighbor, float distance )
{
	float range, power;

	if ( !neighbor )
	{
		return 0.0f;
	}

	// can only influence human buildables
	if ( neighbor->s.eType != ET_BUILDABLE || neighbor->buildableTeam != TEAM_HUMANS )
	{
		return 0.0f;
	}

	// cannot influence buildables that need no power
	if ( !BG_Buildable(neighbor->s.modelindex )->powerConsumption )
	{
		return 0.0f;
	}

	switch ( buildable )
	{
		case BA_H_REPEATER:
			power = g_powerReactorSupply.integer;
			range = g_powerReactorRange.integer;
			break;

		case BA_H_REACTOR:
			power = g_powerReactorSupply.integer;
			range = g_powerReactorRange.integer;
			break;

		default:
			power = -BG_Buildable( buildable )->powerConsumption;
			range = g_powerCompetitionRange.integer;
	}

	// sanity check range
	if ( range <= 0.0f )
	{
		return 0.0f;
	}

	return power * MAX( 0.0f, 1.0f - ( distance / range ) );
}

/*
=================
CalculateSparePower

Calculates the current and expected amount of spare power of a buildable.
=================
*/
static void CalculateSparePower( gentity_t *self )
{
	gentity_t *neighbor;
	float     distance;
	int       powerConsumption, currentBaseSupply, expectedBaseSupply;

	if ( self->s.eType != ET_BUILDABLE || self->buildableTeam != TEAM_HUMANS )
	{
		return;
	}

	powerConsumption = BG_Buildable( self->s.modelindex )->powerConsumption;

	if ( !powerConsumption )
	{
		return;
	}

	// reactor enables base supply everywhere on the map
	if ( G_ActiveReactor() != NULL )
	{
		currentBaseSupply = expectedBaseSupply = g_powerBaseSupply.integer;
	}
	else if ( G_AliveReactor() != NULL )
	{
		currentBaseSupply = 0;
		expectedBaseSupply = g_powerBaseSupply.integer;
	}
	else
	{
		currentBaseSupply = expectedBaseSupply = 0;
	}

	self->expectedSparePower = expectedBaseSupply - powerConsumption;

	if ( self->spawned )
	{
		self->currentSparePower = currentBaseSupply - powerConsumption;
	}
	else
	{
		self->currentSparePower = 0;
	}

	neighbor = NULL;
	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, self->s.origin, PowerRelevantRange() ) ) )
	{
		if ( self == neighbor )
		{
			continue;
		}

		distance = Distance( self->s.origin, neighbor->s.origin );

		self->expectedSparePower += IncomingInterference( (buildable_t) self->s.modelindex, neighbor, distance, qtrue );

		if ( self->spawned )
		{
			self->currentSparePower += IncomingInterference( (buildable_t) self->s.modelindex, neighbor, distance, qfalse );
		}
	}

	// HACK: Store total power (used + spare) in entityState_t.clientNum for display
	if ( self->spawned )
	{
		int totalPower = (int)self->currentSparePower + BG_Buildable( self->s.modelindex )->powerConsumption;

		self->s.clientNum = Q_clamp( totalPower, 0, POWER_DISPLAY_MAX );
	}
	else
	{
		self->s.clientNum = 0;
	}
}


/*
=================
G_SetHumanBuildablePowerState

Powers human buildables up and down based on available power and reactor status.
Updates expected spare power for all human buildables.

TODO: Assemble a list of relevant entities first.
=================
*/
void G_SetHumanBuildablePowerState()
{
	qboolean  done;
	float     lowestSparePower;
	gentity_t *ent = NULL, *lowestSparePowerEnt;

	static int nextCalculation = 0;

	if ( level.time < nextCalculation )
	{
		return;
	}

	// first pass: predict spare power for all buildables,
	//             power up buildables that have enough power
	while ( ( ent = G_IterateEntities( ent, NULL, qfalse, 0, NULL ) ) )
	{
		// discard irrelevant entities
		if ( ent->s.eType != ET_BUILDABLE || ent->buildableTeam != TEAM_HUMANS )
		{
			continue;
		}

		CalculateSparePower( ent );

		if ( ent->currentSparePower >= 0.0f )
		{
			ent->powered = qtrue;
		}
	}

	// power down buildables that lack power, highest deficit first
	do
	{
		lowestSparePowerEnt = NULL;
		lowestSparePower = MAX_QINT;

		// find buildable with highest power deficit
		while ( ( ent = G_IterateEntities( ent, NULL, qfalse, 0, NULL ) ) )
		{
			// discard irrelevant entities
			if ( ent->s.eType != ET_BUILDABLE || ent->buildableTeam != TEAM_HUMANS )
			{
				continue;
			}

			// ignore buildables that haven't yet spawned or are already powered down
			if ( !ent->spawned || !ent->powered )
			{
				continue;
			}

			// ignore buildables that need no power
			if ( !BG_Buildable( ent->s.modelindex )->powerConsumption )
			{
				continue;
			}

			CalculateSparePower( ent );

			// never shut down the telenode, even if it was set to consume power and operates below
			// its threshold
			if ( ent->s.modelindex == BA_H_SPAWN )
			{
				continue;
			}

			if ( ent->currentSparePower < lowestSparePower )
			{
				lowestSparePower = ent->currentSparePower;
				lowestSparePowerEnt = ent;
			}
		}

		if ( lowestSparePower < 0.0f )
		{
			lowestSparePowerEnt->powered = qfalse;
			done = qfalse;
		}
		else
		{
			done = qtrue;
		}
	}
	while ( !done );

	nextCalculation = level.time + 500;
}

/*
================
HGeneric_Blast

Called when a human buildable explodes.
================
*/
void HGeneric_Blast( gentity_t *self )
{
	vec3_t dir;

	// we don't have a valid direction, so just point straight up
	dir[ 0 ] = dir[ 1 ] = 0;
	dir[ 2 ] = 1;

	self->timestamp = level.time;

	//do some radius damage
	G_RadiusDamage( self->s.pos.trBase, g_entities + self->killedBy, self->splashDamage,
	                self->splashRadius, self, self->splashMethodOfDeath );

	// begin freeing build points
	G_RewardAttackers( self );

	// turn into an explosion
	self->s.eType = (entityType_t) ( ET_EVENTS + EV_HUMAN_BUILDABLE_EXPLOSION );
	self->freeAfterEvent = qtrue;
	G_AddEvent( self, EV_HUMAN_BUILDABLE_EXPLOSION, DirToByte( dir ) );
}

/*
================
HGeneric_Disappear

Called when a human buildable is destroyed before it is spawned.
================
*/
void HGeneric_Disappear( gentity_t *self )
{
	self->timestamp = level.time;
	G_RewardAttackers( self );

	G_FreeEntity( self );
}

void HGeneric_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	G_SetBuildableAnim( self, self->powered ? BANIM_DESTROY1 : BANIM_DESTROY_UNPOWERED, qtrue );
	G_SetIdleBuildableAnim( self, BANIM_DESTROYED );

	self->die = NullDieFunction;
	self->killedBy = attacker - g_entities;
	//self->powered = qfalse;
	self->s.eFlags &= ~EF_FIRING; // prevent any firing effects

	if ( self->spawned )
	{
		// blast after a brief period
		self->think = HGeneric_Blast;
		self->nextthink = level.time + HUMAN_DETONATION_DELAY;

		// make a warning sound before ractor and repeater explosion
		// don't randomize blast delay for them so the sound stays synced
		switch ( self->s.modelindex )
		{
			case BA_H_REPEATER:
			case BA_H_REACTOR:
				G_AddEvent( self, EV_HUMAN_BUILDABLE_DYING, 0 );
				break;

			default:
				self->nextthink += ( ( rand() - ( RAND_MAX / 2 ) ) / ( float )( RAND_MAX / 2 ) )
				                   * DETONATION_DELAY_RAND_RANGE * HUMAN_DETONATION_DELAY;
		}
	}
	else
	{
		// disappear immediately
		self->think = HGeneric_Disappear;
		self->nextthink = level.time;
	}

	// warn if this building was powered and there's a watcher nearby
	if ( self->s.modelindex != BA_H_REACTOR && self->powered && IsWarnableMOD( mod ) )
	{
		qboolean  inBase    = G_InsideBase( self, qtrue );
		gentity_t *watcher  = NULL;
		gentity_t *location = NULL;

		if ( inBase )
		{
			// note that being inside the main base doesn't mean there always is an active reactor
			watcher = G_ActiveReactor();
		}

		if ( !watcher )
		{
			// note that a repeater will find itself as nearest power source
			watcher = NearestPowerSourceInRange( self );
		}

		// found reactor or repeater? get location entity
		if ( watcher )
		{
			location = Team_GetLocation( watcher );
			if ( !location )
			{
				location = level.fakeLocation; // fall back on the fake location
			}
		}

		// check that the location/repeater/reactor hasn't warned recently
		if ( location && level.time > location->warnTimer )
		{
			location->warnTimer = level.time + NEARBY_ATTACK_PERIOD; // don't spam
			G_BroadcastEvent( EV_WARN_ATTACK, inBase ? 0 : ( watcher - level.gentities ), TEAM_HUMANS );
		}
	}

	G_LogDestruction( self, attacker, mod );
}

void HSpawn_Think( gentity_t *self )
{
	gentity_t *ent;

	self->nextthink = level.time + 100;

	if ( self->spawned )
	{
		//only suicide if at rest
		if ( self->s.groundEntityNum != ENTITYNUM_NONE )
		{
			if ( ( ent = G_CheckSpawnPoint( self->s.number, self->s.origin,
			                                self->s.origin2, BA_H_SPAWN, NULL ) ) != NULL )
			{
				// If the thing blocking the spawn is a buildable, kill it.
				// If it's part of the map, kill self.
				if ( ent->s.eType == ET_BUILDABLE )
				{
					G_Damage( ent, NULL, NULL, NULL, NULL, self->health, 0, MOD_SUICIDE );
					G_SetBuildableAnim( self, BANIM_SPAWN1, qtrue );
				}
				else if ( ent->s.number == ENTITYNUM_WORLD || ent->s.eType == ET_MOVER )
				{
					G_Damage( self, NULL, NULL, NULL, NULL, self->health, 0, MOD_SUICIDE );
					return;
				}
				else if( g_antiSpawnBlock.integer &&
				         ent->client && ent->client->pers.team == TEAM_HUMANS )
				{
					PuntBlocker( self, ent );
				}

				if ( ent->s.eType == ET_CORPSE )
				{
					G_FreeEntity( ent );  //quietly remove
				}
			}
			else
			{
				self->spawnBlockTime = 0;
			}
		}
	}
}

void HRepeater_Think( gentity_t *self )
{
	self->nextthink = level.time + 1000;

	IdlePowerState( self );
}

void HReactor_Think( gentity_t *self )
{
	gentity_t *ent, *trail;
	qboolean  fire = qfalse;

	self->nextthink = level.time + REACTOR_ATTACK_REPEAT;

	if ( !self->spawned || self->health <= 0 )
	{
		return;
	}

	// create a tesla trail for every target
	for ( ent = NULL; ( ent = G_IterateEntitiesWithinRadius( ent, self->s.pos.trBase, REACTOR_ATTACK_RANGE ) ); )
	{
		if ( !ent->client ||
			 ent->client->pers.team != TEAM_ALIENS ||
			 ent->flags & FL_NOTARGET )
		{
			continue;
		}

		trail = G_NewTempEntity( ent->s.pos.trBase, EV_TESLATRAIL );
		trail->s.generic1 = self->s.number; // source
		trail->s.clientNum = ent->s.number; // destination
		VectorCopy( self->s.pos.trBase, trail->s.origin2 );

		fire = qtrue;
	}

	// actual damage is done by radius
	if ( fire )
	{
		self->timestamp = level.time;

		G_SelectiveRadiusDamage( self->s.pos.trBase, self, REACTOR_ATTACK_DAMAGE,
		                         REACTOR_ATTACK_RANGE, self, MOD_REACTOR, TEAM_HUMANS );
	}

	G_WarnPrimaryUnderAttack( self );
}

void HReactor_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	HGeneric_Die( self, inflictor, attacker, mod );
	if ( IsWarnableMOD( mod ) )
	{
		G_BroadcastEvent( EV_REACTOR_DYING, 0, TEAM_HUMANS );
	}
}

void HArmoury_Use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !self->spawned )
	{
		return;
	}

	if ( activator->client->pers.team != TEAM_HUMANS )
	{
		return;
	}

	if ( !self->powered )
	{
		G_TriggerMenu( activator->client->ps.clientNum, MN_H_NOTPOWERED );
		return;
	}

	G_TriggerMenu( activator->client->ps.clientNum, MN_H_ARMOURY );
}

void HArmoury_Think( gentity_t *self )
{
	self->nextthink = level.time + 1000;
}

void HMedistat_Die( gentity_t *self, gentity_t *inflictor,
                    gentity_t *attacker, int mod )
{
	// clear target's healing flag
	if ( self->target && self->target->client )
	{
		self->target->client->ps.stats[ STAT_STATE ] &= ~SS_HEALING_2X;
	}

	HGeneric_Die( self, inflictor, attacker, mod );
}

void HMedistat_Think( gentity_t *self )
{
	int       entityList[ MAX_GENTITIES ];
	vec3_t    mins, maxs;
	int       playerNum, numPlayers;
	gentity_t *player;
	gclient_t *client;
	qboolean  occupied;

	self->nextthink = level.time + 100;

	if ( !self->spawned )
	{
		return;
	}

	// set animation
	if ( self->powered )
	{
		if ( !self->active && self->s.torsoAnim != BANIM_IDLE1 )
		{
			G_SetBuildableAnim( self, BANIM_CONSTRUCT2, qtrue );
			G_SetIdleBuildableAnim( self, BANIM_IDLE1 );
		}
	}
	else if ( self->s.torsoAnim != BANIM_IDLE_UNPOWERED )
	{
			G_SetBuildableAnim( self, BANIM_POWERDOWN, qfalse );
			G_SetIdleBuildableAnim( self, BANIM_IDLE_UNPOWERED );
	}

	// clear target's healing flag for now
	if ( self->target && self->target->client )
	{
		self->target->client->ps.stats[ STAT_STATE ] &= ~SS_HEALING_2X;
	}

	// clear target on power loss
	if ( !self->powered )
	{
		self->active = qfalse;
		self->target = NULL;

		self->nextthink = level.time + POWER_REFRESH_TIME;

		return;
	}

	// get entities standing on top
	VectorAdd( self->s.origin, self->r.maxs, maxs );
	VectorAdd( self->s.origin, self->r.mins, mins );

	mins[ 2 ] += ( self->r.mins[ 2 ] + self->r.maxs[ 2 ] );
	maxs[ 2 ] += 32; // continue to heal jumping players but don't heal jetpack campers

	numPlayers = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
	occupied = qfalse;

	// mark occupied if still healing a player
	for ( playerNum = 0; playerNum < numPlayers; playerNum++ )
	{
		player = &g_entities[ entityList[ playerNum ] ];
		client = player->client;

		if ( self->target == player && PM_Live( client->ps.pm_type ) &&
			 ( player->health < client->ps.stats[ STAT_MAX_HEALTH ] ||
			   client->ps.stats[ STAT_STAMINA ] < STAMINA_MAX ) )
		{
			occupied = qtrue;
		}
	}

	if ( !occupied )
	{
		self->target = NULL;
	}

	// clear poison, distribute medikits, find a new target if necessary
	for ( playerNum = 0; playerNum < numPlayers; playerNum++ )
	{
		player = &g_entities[ entityList[ playerNum ] ];
		client = player->client;

		// only react to humans
		if ( !client || client->pers.team != TEAM_HUMANS )
		{
			continue;
		}

		// ignore 'notarget' users
		if ( player->flags & FL_NOTARGET )
		{
			continue;
		}

		// remove poison
		if ( client->ps.stats[ STAT_STATE ] & SS_POISONED )
		{
			client->ps.stats[ STAT_STATE ] &= ~SS_POISONED;
		}

		// give medikit to players with full health
		if ( player->health >= client->ps.stats[ STAT_MAX_HEALTH ] )
		{
			player->health = client->ps.stats[ STAT_MAX_HEALTH ];

			if ( !BG_InventoryContainsUpgrade( UP_MEDKIT, player->client->ps.stats ) )
			{
				BG_AddUpgradeToInventory( UP_MEDKIT, player->client->ps.stats );
			}
		}

		// if not already occupied, check if someone needs healing
		if ( !occupied )
		{
			if ( PM_Live( client->ps.pm_type ) &&
				 ( player->health < client->ps.stats[ STAT_MAX_HEALTH ] ||
				   client->ps.stats[ STAT_STAMINA ] < STAMINA_MAX ) )
			{
				self->target = player;
				occupied = qtrue;
			}
		}
	}

	// if we have a target, heal it
	if ( self->target && self->target->client )
	{
		player = self->target;
		client = player->client;
		client->ps.stats[ STAT_STATE ] |= SS_HEALING_2X;

		// start healing animation
		if ( !self->active )
		{
			self->active = qtrue;

			G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
			G_SetIdleBuildableAnim( self, BANIM_IDLE2 );
		}

		// restore stamina
		if ( client->ps.stats[ STAT_STAMINA ] + STAMINA_MEDISTAT_RESTORE >= STAMINA_MAX )
		{
			client->ps.stats[ STAT_STAMINA ] = STAMINA_MAX;
		}
		else
		{
			client->ps.stats[ STAT_STAMINA ] += STAMINA_MEDISTAT_RESTORE;
		}

		// restore health
		if ( player->health < client->ps.stats[ STAT_MAX_HEALTH ] )
		{
			self->buildableStatsTotal += G_Heal( player, 1 );

			// check if fully healed
			if ( player->health == client->ps.stats[ STAT_MAX_HEALTH ] )
			{
				// give medikit
				if ( !BG_InventoryContainsUpgrade( UP_MEDKIT, client->ps.stats ) )
				{
					BG_AddUpgradeToInventory( UP_MEDKIT, client->ps.stats );
				}

				self->buildableStatsCount++;
			}
		}
	}
	// if we lost our target, replay construction animation
	else if ( self->active )
	{
		self->active = qfalse;

		G_SetBuildableAnim( self, BANIM_CONSTRUCT2, qtrue );
		G_SetIdleBuildableAnim( self, BANIM_IDLE1 );
	}
}

static int HTurret_DistanceToZone( float distance )
{
	const float zoneWidth = ( float )TURRET_RANGE / ( float )TURRET_ZONES;

	int zone = ( int )( distance / zoneWidth );

	if ( zone >= TURRET_ZONES )
	{
		return TURRET_ZONES - 1;
	}
	else
	{
		return zone;
	}
}

static gentity_t *cmpTurret = NULL;

static int HTurret_CompareTargets( const void *first, const void *second )
{
	gentity_t *a, *b;

	if ( !cmpTurret )
	{
		return 0;
	}

	a = ( gentity_t * )first;
	b = ( gentity_t * )second;

	// Always prefer target that isn't yet targeted.
	// This makes group attacks more and dretch spam less efficient.
	{
		if ( a->numTrackedBy == 0 && b->numTrackedBy > 0 )
		{
			return -1;
		}
		else if ( a->numTrackedBy > 0 && b->numTrackedBy == 0 )
		{
			return 1;
		}
	}

	// Prefer target that can be aimed at more quickly.
	// This makes the turret spend less time tracking enemies.
	{
		vec3_t aimDir, aDir, bDir;

		AngleVectors( cmpTurret->buildableAim, aimDir, NULL, NULL );

		VectorSubtract( a->s.origin, cmpTurret->s.origin, aDir );
		VectorSubtract( b->s.origin, cmpTurret->s.origin, bDir );

		VectorNormalize( aDir );
		VectorNormalize( bDir );

		if ( DotProduct( aDir, aimDir ) > DotProduct( bDir, aimDir ) )
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}

	/*
	// Prefer smaller distance. Use zones so close targets with different hit ranges are treated equally.
	// Note that one target can't hide the other, since the other wouldn't be valid.
	{
		int ad = HTurret_DistanceToZone( Distance( cmpTurret->s.origin, a->s.origin ) );
		int bd = HTurret_DistanceToZone( Distance( cmpTurret->s.origin, b->s.origin ) );

		if ( ad < bd )
		{
			return -1;
		}
		else if ( bd < ad )
		{
			return 1;
		}
	}

	// Tie breaker is random decision, so clients on lower slots don't get shot at more often.
	{
		if ( rand() < ( RAND_MAX / 2 ) )
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	*/
}

static qboolean HTurret_TargetValid( gentity_t *self, gentity_t *target, qboolean newTarget )
{
	trace_t tr;
	vec3_t  dir, end;

	if (    !target
	     || !target->client
	     || target->health <= 0
	     || G_OnSameTeam( self, target )
	     || target->flags & FL_NOTARGET
	     || Distance( self->s.origin, target->s.origin ) > TURRET_RANGE )
	{
		if ( g_debugTurrets.integer > 0 && self->target )
		{
			Com_Printf( "Turret %d: Target %d eliminated or out of sight.\n",
			            self->s.number, self->target->s.number );
		}

		return qfalse;
	}

	if ( newTarget )
	{
		// check if target could be hit with a precise shot
		VectorSubtract( target->s.pos.trBase, self->s.pos.trBase, dir );
		VectorNormalize( dir );
		VectorMA( self->s.pos.trBase, TURRET_RANGE, dir, end );
		trap_Trace( &tr, self->s.pos.trBase, NULL, NULL, end, self->s.number, MASK_SHOT );

		if ( tr.entityNum != ( target - g_entities ) )
		{
			return qfalse;
		}
	}
	else
	{
		// give up on target if we couldn't shoot at it for a while
		if ( self->turretLastShotAtTarget && self->turretLastShotAtTarget + TURRET_GIVEUP_TARGET < level.time )
		{
			if ( g_debugTurrets.integer > 0 && self->target )
			{
				Com_Printf( "Turret %d: No line of sight to target %d for %d ms, giving up.\n",
				            self->s.number, self->target->s.number, TURRET_GIVEUP_TARGET );
			}

			return qfalse;
		}
	}

	self->turretLastSeenATarget = level.time;

	return qtrue;
}

static qboolean HTurret_FindTarget( gentity_t *self )
{
	gentity_t *neighbour = NULL;
	gentity_t *validTargets[ MAX_GENTITIES ];
	int       validTargetNum = 0;

	// delete old target
	if ( self->target )
	{
		self->target->numTrackedBy--;
	}

	self->target = NULL;
	self->turretLastShotAtTarget = 0;

	// find all potential targets
	for ( neighbour = NULL; ( neighbour = G_IterateEntitiesWithinRadius( neighbour, self->s.origin, TURRET_RANGE )); )
	{
		if ( HTurret_TargetValid( self, neighbour, qtrue ) )
		{
		     validTargets[ validTargetNum++ ] = neighbour;
		}
	}

	if ( validTargetNum > 0 )
	{
		// search best target
		cmpTurret = self;
		qsort( validTargets, validTargetNum, sizeof( gentity_t* ), HTurret_CompareTargets );

		self->target = validTargets[ 0 ];
		self->target->numTrackedBy++;

		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

static void HTurret_MoveHeadToTarget( gentity_t *self )
{
	const static vec3_t upwards = { 0.0f, 0.0f, 1.0f };

	vec3_t rotAxis, angleToTarget, relativeDirToTarget, deltaAngles, relativeNewDir, newDir, maxAngles;
	float  rotAngle, relativePitch, timeMod;
	int    elapsed, currentAngle;

	// calculate maximum angular movement for this execution
	if ( !self->turretLastHeadMove )
	{
		// no information yet, start moving next time
		self->turretLastHeadMove = level.time;
		return;
	}

	elapsed = level.time - self->turretLastHeadMove;

	if ( elapsed <= 0 )
	{
		return;
	}

	timeMod = ( float )elapsed / 1000.0f;

	maxAngles[ YAW ]   = timeMod * ( float )TURRET_YAW_SPEED;
	maxAngles[ PITCH ] = timeMod * ( float )TURRET_PITCH_SPEED;
	maxAngles[ ROLL ]  = 0.0f;

	// make sure direction to target is normalized
	VectorNormalize( self->turretDirToTarget );

	// get angle from current orientation towards target direction
	CrossProduct( self->s.origin2, upwards, rotAxis );
	VectorNormalize( rotAxis );
	rotAngle = RAD2DEG( acos( DotProduct( self->s.origin2, upwards ) ) );
	RotatePointAroundVector( relativeDirToTarget, rotAxis, self->turretDirToTarget, rotAngle );
	vectoangles( relativeDirToTarget, angleToTarget );

	// adjust pitch and yaw
	for ( currentAngle = 0; currentAngle < 3; currentAngle++ )
	{
		if ( currentAngle == ROLL )
		{
			continue;
		}

		// get angle delta from current orientation towards trarget angle
		deltaAngles[ currentAngle ] = AngleSubtract( self->s.angles2[ currentAngle ], angleToTarget[ currentAngle ] );

		// adjust enttiyState_t.angles2
		if      ( deltaAngles[ currentAngle ] < 0.0f && deltaAngles[ currentAngle ] < -maxAngles[ currentAngle ] )
		{
			self->s.angles2[ currentAngle ] += maxAngles[ currentAngle ];
		}
		else if ( deltaAngles[ currentAngle ] > 0.0f && deltaAngles[ currentAngle ] >  maxAngles[ currentAngle ] )
		{
			self->s.angles2[ currentAngle ] -= maxAngles[ currentAngle ];
		}
		else
		{
			self->s.angles2[ currentAngle ] = angleToTarget[ currentAngle ];
		}
	}

	// limit pitch
	relativePitch = fabs( self->s.angles2[ PITCH ] );

	if ( relativePitch > 180 )
	{
		relativePitch -= 360;
	}

	if ( relativePitch < -TURRET_PITCH_CAP )
	{
		self->s.angles2[ PITCH ] = TURRET_PITCH_CAP - 360.0f;
	}

	// update muzzle angles
	AngleVectors( self->s.angles2, relativeNewDir, NULL, NULL ); //
	RotatePointAroundVector( newDir, rotAxis, relativeNewDir, -rotAngle );
	vectoangles( newDir, self->buildableAim );

	self->turretLastHeadMove = level.time;
}

static void HTurret_TrackTarget( gentity_t *self )
{
	if ( !self->target )
	{
		return;
	}

	// set target direction
	VectorSubtract( self->target->s.pos.trBase, self->s.pos.trBase, self->turretDirToTarget );
	VectorNormalize( self->turretDirToTarget );
}

static void HTurret_SetBaseDir( gentity_t *self )
{
	trace_t tr;
	vec3_t  dir, end;

	// check if we already have a base direction
	if ( VectorLength( self->turretBaseDir ) > 0.5f )
	{
		return;
	}

	// prefer buildable orientation
	AngleVectors( self->s.angles, dir, NULL, NULL );
	VectorNormalize( dir );

	// invert base direction if it reaches into a wall within the first damage zone
	VectorMA( self->s.origin, ( float )TURRET_RANGE / ( float )TURRET_ZONES, dir, end );
	trap_Trace( &tr, self->s.pos.trBase, NULL, NULL, end, self->s.number, MASK_SHOT );

	if ( tr.entityNum == ENTITYNUM_WORLD || g_entities[ tr.entityNum ].s.eType == ET_BUILDABLE )
	{
		VectorNegate( dir, self->turretBaseDir );
	}
	else
	{
		VectorCopy( dir, self->turretBaseDir );
	}
}

static void HTurret_ResetDirection( gentity_t *self )
{
	VectorCopy( self->turretBaseDir, self->turretDirToTarget );
}

static void HTurret_ResetPitch( gentity_t *self )
{
	vec3_t angles;

	VectorCopy( self->s.angles2, angles );

	angles[ PITCH ] = 0.0f;

	AngleVectors( angles, self->turretDirToTarget, NULL, NULL );

	VectorNormalize( self->turretDirToTarget );
}

static void HTurret_LowerPitch( gentity_t *self )
{
	vec3_t angles;

	VectorCopy( self->s.angles2, angles );

	angles[ PITCH ] = TURRET_PITCH_CAP;

	AngleVectors( angles, self->turretDirToTarget, NULL, NULL );

	VectorNormalize( self->turretDirToTarget );
}

static qboolean HTurret_TargetInReach( gentity_t *self )
{
	trace_t tr;
	vec3_t  forward, end;

	if ( !self->target )
	{
		return qfalse;
	}

	// check if a precise shot would hit the target
	AngleVectors( self->buildableAim, forward, NULL, NULL );
	VectorMA( self->s.pos.trBase, TURRET_RANGE, forward, end );
	trap_Trace( &tr, self->s.pos.trBase, NULL, NULL, end, self->s.number, MASK_SHOT );

	return ( tr.entityNum == ( self->target - g_entities ) );
}

static void HTurret_Shoot( gentity_t *self )
{
	const int zoneDamage[] = TURRET_ZONE_DAMAGE;
	int       zone;

	// if turret doesn't have the fast loader upgrade, pause after three shots
	if ( !self->turretHasFastLoader && self->turretSuccessiveShots >= 3 )
	{
		self->turretSuccessiveShots = 0;

		self->turretNextShot = level.time + TURRET_ATTACK_PERIOD;
		self->s.eFlags &= ~EF_FIRING;

		return;
	}

	self->s.eFlags |= EF_FIRING;

	zone = HTurret_DistanceToZone( Distance( self->s.pos.trBase, self->target->s.pos.trBase ) );
	self->turretCurrentDamage = zoneDamage[ zone ];

	if ( g_debugTurrets.integer > 1 )
	{
		const static char color[] = {'1', '8', '3', '2'};
		Com_Printf( "Turret %d: Shooting at %d: ^%cZone %d/%d → %d damage\n",
		            self->s.number, self->target->s.number, color[zone], zone + 1,
		            TURRET_ZONES, self->turretCurrentDamage );
	}

	G_AddEvent( self, EV_FIRE_WEAPON, 0 );
	G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );
	G_FireWeapon( self, WP_MGTURRET, WPM_PRIMARY );

	self->turretSuccessiveShots++;
	self->turretLastShotAtTarget = level.time;
	self->turretNextShot = level.time + TURRET_ATTACK_PERIOD;
}

void HTurret_Think( gentity_t *self )
{
	qboolean gotValidTarget;

	//HGeneric_Think( self );
	self->nextthink = level.time + TURRET_THINK_PERIOD;

	IdlePowerState( self );

	// disable muzzle flash for now
	self->s.eFlags &= ~EF_FIRING;

	if ( !self->spawned )
	{
		return;
	}

	// adjust yaw according to power state
	if ( !self->powered )
	{
		self->turretDisabled = qtrue;
		self->turretSuccessiveShots = 0;

		HTurret_LowerPitch( self );
		HTurret_MoveHeadToTarget( self );

		return;
	}
	else
	{
		if ( self->turretDisabled )
		{
			HTurret_ResetPitch( self );
		}

		self->turretDisabled = qfalse;
	}

	// set turret base direction
	HTurret_SetBaseDir( self );

	// check for valid target
	if ( HTurret_TargetValid( self, self->target, qfalse ) )
	{
		gotValidTarget = qtrue;
	}
	else
	{
		gotValidTarget = HTurret_FindTarget( self );

		if ( gotValidTarget && g_debugTurrets.integer > 0 )
		{
			Com_Printf( "Turret %d: New target %d.\n", self->s.number, self->target->s.number );
		}
	}

	// adjust target direction
	if ( gotValidTarget )
	{
		HTurret_TrackTarget( self );
	}
	else if ( !self->turretLastSeenATarget )
	{
		HTurret_ResetDirection( self );
	}

	// move head according to target direction
	HTurret_MoveHeadToTarget( self );

	// shoot if possible
	if ( HTurret_TargetInReach( self ) )
	{
		if ( self->turretNextShot < level.time )
		{
			HTurret_Shoot( self );
		}
	}
	else
	{
		self->turretSuccessiveShots = 0;
	}
}

void HTurret_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	if ( self->target )
	{
		self->target->numTrackedBy--;
	}

	HGeneric_Die( self, inflictor, attacker, mod );
}

void HTeslaGen_Think( gentity_t *self )
{
	gentity_t *ent;

	self->nextthink = level.time + 150;

	IdlePowerState( self );

	if ( !self->spawned )
	{
		return;
	}

	if ( !self->powered )
	{
		self->s.eFlags &= ~EF_FIRING;
		self->nextthink = level.time + POWER_REFRESH_TIME;

		return;
	}

	if ( self->timestamp < level.time )
	{
		vec3_t muzzle;

		// Stop firing effect for now
		self->s.eFlags &= ~EF_FIRING;

		// Move the muzzle from the entity origin up a bit to fire over turrets
		VectorMA( self->s.origin, self->r.maxs[ 2 ], self->s.origin2, muzzle );

		// Attack nearby Aliens
		for ( ent = NULL; ( ent = G_IterateEntitiesWithinRadius( ent, muzzle, TESLAGEN_RANGE ) ); )
		{
			// TODO: Replace this with IsAliveEnemy helper
			if ( !ent->client ||
			     ent->client->pers.team != TEAM_ALIENS ||
			     ent->health <= 0 ||
			     ent->flags & FL_NOTARGET )
			{
				continue;
			}

			self->target = ent;
			G_FireWeapon( self, WP_TESLAGEN, WPM_PRIMARY );
			self->target = NULL;
		}

		if ( self->s.eFlags & EF_FIRING )
		{
			G_AddEvent( self, EV_FIRE_WEAPON, 0 );
			//G_SetBuildableAnim( self, BANIM_ATTACK1, qfalse );

			self->timestamp = level.time + TESLAGEN_REPEAT;
		}
	}
}

void HDrill_Think( gentity_t *self )
{
	qboolean active, lastThinkActive;
	float    resources;

	self->nextthink = level.time + 1000;

	active = self->spawned & self->powered;
	lastThinkActive = self->s.weapon > 0;

	if ( active ^ lastThinkActive )
	{
		// If the state of this RGS has changed, adjust own rate and inform
		// active closeby RGS so they can adjust their rate immediately.
		G_RGSCalculateRate( self );
		G_RGSInformNeighbors( self );
	}

	if ( active )
	{
		resources = self->s.weapon / 60000.0f;
		self->buildableStatsTotalF += resources;
	}

	IdlePowerState( self );
}

void HDrill_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int mod )
{
	HGeneric_Die( self, inflictor, attacker, mod );

	self->s.weapon = 0;
	self->s.weaponAnim = 0;

	// Assume our state has changed and inform closeby RGS
	G_RGSInformNeighbors( self );
}

/*
============
G_BuildableTouchTriggers

Find all trigger entities that a buildable touches.
============
*/
void G_BuildableTouchTriggers( gentity_t *ent )
{
	int              i, num;
	int              touch[ MAX_GENTITIES ];
	gentity_t        *hit;
	trace_t          trace;
	vec3_t           mins, maxs;
	vec3_t           bmins, bmaxs;
	static    vec3_t range = { 10, 10, 10 };

	// dead buildables don't activate triggers!
	if ( ent->health <= 0 )
	{
		return;
	}

	BG_BuildableBoundingBox( ent->s.modelindex, bmins, bmaxs );

	VectorAdd( ent->s.origin, bmins, mins );
	VectorAdd( ent->s.origin, bmaxs, maxs );

	VectorSubtract( mins, range, mins );
	VectorAdd( maxs, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	VectorAdd( ent->s.origin, bmins, mins );
	VectorAdd( ent->s.origin, bmaxs, maxs );

	for ( i = 0; i < num; i++ )
	{
		hit = &g_entities[ touch[ i ] ];

		if ( !hit->touch )
		{
			continue;
		}

		if ( !( hit->r.contents & CONTENTS_SENSOR ) )
		{
			continue;
		}

		if ( !hit->enabled )
		{
			continue;
		}

		//ignore buildables not yet spawned
		if ( !ent->spawned )
		{
			continue;
		}

		if ( !trap_EntityContact( mins, maxs, hit ) )
		{
			continue;
		}

		memset( &trace, 0, sizeof( trace ) );

		if ( hit->touch )
		{
			hit->touch( hit, ent, &trace );
		}
	}
}

/*
===============
G_BuildableThink

General think function for buildables
===============
*/
void G_BuildableThink( gentity_t *ent, int msec )
{
	int   maxHealth = BG_Buildable( ent->s.modelindex )->health;
	int   regenRate = BG_Buildable( ent->s.modelindex )->regenRate;
	int   buildTime = BG_Buildable( ent->s.modelindex )->buildTime;

	//toggle spawned flag for buildables
	if ( !ent->spawned && ent->health > 0 && !level.pausedTime )
	{
		if ( ent->creationTime + buildTime < level.time )
		{
			ent->spawned = qtrue;
			ent->enabled = qtrue;

			if ( ent->s.modelindex == BA_A_OVERMIND )
			{
				G_TeamCommand( TEAM_ALIENS, "cp \"The Overmind has awakened!\"" );
			}

			// Award momentum
			G_AddMomentumForBuilding( ent );
		}
	}

	// Timer actions
	ent->time1000 += msec;

	if ( ent->time1000 >= 1000 )
	{
		ent->time1000 -= 1000;

		if ( !ent->spawned && ent->health > 0 )
		{
			int gain = ( int )ceil( maxHealth * ( 1.0f - BUILDABLE_START_HEALTH_FRAC ) * 1000.0f / buildTime );

			G_Heal( ent, gain );
		}
		else if ( ent->health > 0 && ent->health < maxHealth )
		{
			int regenWait;

			switch ( ent->buildableTeam )
			{
				case TEAM_ALIENS: regenWait = ALIEN_BUILDABLE_REGEN_WAIT; break;
				case TEAM_HUMANS: regenWait = HUMAN_BUILDABLE_REGEN_WAIT; break;
				default:          regenWait = 0;                          break;
			}

			if ( regenRate && ( ent->lastDamageTime + regenWait ) < level.time )
			{
				G_Heal( ent, regenRate );
			}
		}
	}

	if ( ent->clientSpawnTime > 0 )
	{
		ent->clientSpawnTime -= msec;
	}

	if ( ent->clientSpawnTime < 0 )
	{
		ent->clientSpawnTime = 0;
	}

	// Set health
	ent->s.generic1 = MAX( ent->health, 0 );

	// Set flags
	ent->s.eFlags &= ~( EF_B_SPAWNED |  EF_B_POWERED | EF_B_MARKED | EF_B_ONFIRE );

	if ( ent->spawned )
	{
		ent->s.eFlags |= EF_B_SPAWNED;
	}

	if ( ent->powered )
	{
		ent->s.eFlags |= EF_B_POWERED;
	}

	if ( ent->deconstruct )
	{
		ent->s.eFlags |= EF_B_MARKED;
	}

	if ( ent->onFire )
	{
		ent->s.eFlags |= EF_B_ONFIRE;
	}

	// Check if this buildable is touching any triggers
	G_BuildableTouchTriggers( ent );

	// Fall back on generic physics routines
	G_Physics( ent, msec );
}

/*
===============
G_BuildableRange

Check whether a point is within some range of a type of buildable
===============
*/
qboolean G_BuildableInRange( vec3_t origin, float radius, buildable_t buildable )
{
	gentity_t *neighbor = NULL;

	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, origin, radius ) ) )
	{
		if ( neighbor->s.eType != ET_BUILDABLE || !neighbor->spawned || neighbor->health <= 0 ||
		     ( neighbor->buildableTeam == TEAM_HUMANS && !neighbor->powered ) )
		{
			continue;
		}

		if ( neighbor->s.modelindex == buildable )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
G_FindBuildable

Finds a buildable of the specified type
================
*/
static gentity_t *FindBuildable( buildable_t buildable )
{
	int       i;
	gentity_t *ent;

	for ( i = MAX_CLIENTS, ent = g_entities + i;
	      i < level.num_entities; i++, ent++ )
	{
		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( ent->s.modelindex == buildable && !( ent->s.eFlags & EF_DEAD ) )
		{
			return ent;
		}
	}

	return NULL;
}

/*
===============
G_BuildablesIntersect

Test if two buildables intersect each other
===============
*/
static qboolean BuildablesIntersect( buildable_t a, vec3_t originA,
                                     buildable_t b, vec3_t originB )
{
	vec3_t minsA, maxsA;
	vec3_t minsB, maxsB;

	BG_BuildableBoundingBox( a, minsA, maxsA );
	VectorAdd( minsA, originA, minsA );
	VectorAdd( maxsA, originA, maxsA );

	BG_BuildableBoundingBox( b, minsB, maxsB );
	VectorAdd( minsB, originB, minsB );
	VectorAdd( maxsB, originB, maxsB );

	return BoundsIntersect( minsA, maxsA, minsB, maxsB );
}

/*
===============
G_CompareBuildablesForRemoval

qsort comparison function for a buildable removal list
===============
*/
static buildable_t cmpBuildable;
static vec3_t      cmpOrigin;
static int CompareBuildablesForRemoval( const void *a, const void *b )
{
	int precedence[] =
	{
		BA_NONE,

		BA_A_BARRICADE,
		BA_A_ACIDTUBE,
		BA_A_TRAPPER,
		BA_A_HIVE,
		BA_A_BOOSTER,
		BA_A_SPAWN,
		BA_A_OVERMIND,

		BA_H_MGTURRET,
		BA_H_TESLAGEN,
		BA_H_MEDISTAT,
		BA_H_ARMOURY,
		BA_H_SPAWN,
		BA_H_REPEATER,
		BA_H_REACTOR
	};

	gentity_t *buildableA, *buildableB;
	int       i;
	int       aPrecedence = 0, bPrecedence = 0;
	qboolean  aMatches = qfalse, bMatches = qfalse;

	buildableA = * ( gentity_t ** ) a;
	buildableB = * ( gentity_t ** ) b;

	// Prefer the one that collides with the thing we're building
	aMatches = BuildablesIntersect( cmpBuildable, cmpOrigin,
	                                (buildable_t) buildableA->s.modelindex, buildableA->s.origin );
	bMatches = BuildablesIntersect( cmpBuildable, cmpOrigin,
	                                (buildable_t) buildableB->s.modelindex, buildableB->s.origin );

	if ( aMatches && !bMatches )
	{
		return -1;
	}
	else if ( !aMatches && bMatches )
	{
		return 1;
	}

	// If the only spawn is marked, prefer it last
	if ( cmpBuildable == BA_A_SPAWN || cmpBuildable == BA_H_SPAWN )
	{
		if ( ( buildableA->s.modelindex == BA_A_SPAWN && level.team[ TEAM_ALIENS ].numSpawns == 1 ) ||
		     ( buildableA->s.modelindex == BA_H_SPAWN && level.team[ TEAM_HUMANS ].numSpawns == 1 ) )
		{
			return 1;
		}

		if ( ( buildableB->s.modelindex == BA_A_SPAWN && level.team[ TEAM_ALIENS ].numSpawns == 1 ) ||
		     ( buildableB->s.modelindex == BA_H_SPAWN && level.team[ TEAM_HUMANS ].numSpawns == 1 ) )
		{
			return -1;
		}
	}

	// Prefer the buildable without power
	if ( !buildableA->powered && buildableB->powered )
	{
		return -1;
	}
	else if ( buildableA->powered && !buildableB->powered )
	{
		return 1;
	}

	// If both are unpowered, prefer the closer buildable
	if ( !buildableA->powered && !buildableB->powered )
	{
		if ( Distance( cmpOrigin, buildableA->s.origin ) < Distance( cmpOrigin, buildableB->s.origin ) )
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}

	// If one matches the thing we're building, prefer it
	aMatches = ( buildableA->s.modelindex == cmpBuildable );
	bMatches = ( buildableB->s.modelindex == cmpBuildable );

	if ( aMatches && !bMatches )
	{
		return -1;
	}
	else if ( !aMatches && bMatches )
	{
		return 1;
	}

	// They're the same type
	if ( buildableA->s.modelindex == buildableB->s.modelindex )
	{
		// Pick the one marked earliest
		return buildableA->deconstructTime - buildableB->deconstructTime;
	}

	// Resort to preference list
	for ( i = 0; i < ARRAY_LEN( precedence ); i++ )
	{
		if ( buildableA->s.modelindex == precedence[ i ] )
		{
			aPrecedence = i;
		}

		if ( buildableB->s.modelindex == precedence[ i ] )
		{
			bPrecedence = i;
		}
	}

	return aPrecedence - bPrecedence;
}

void G_Deconstruct( gentity_t *self, gentity_t *deconner, meansOfDeath_t deconType )
{
	int   refund;
	const buildableAttributes_t *attr;

	if ( !self || self->s.eType != ET_BUILDABLE )
	{
		return;
	}

	attr = BG_Buildable( self->s.modelindex );

	// return BP
	refund = attr->buildPoints * ( self->health / ( float )attr->health );
	G_ModifyBuildPoints( self->buildableTeam, refund );

	// remove momentum
	G_RemoveMomentumForDecon( self, deconner );

	// deconstruct
	G_Damage( self, NULL, deconner, NULL, NULL, self->health, 0, deconType );
	G_FreeEntity( self );
}

/*
===============
G_FreeMarkedBuildables

Returns the number of buildables removed.
===============
*/
int G_FreeMarkedBuildables( gentity_t *deconner, char *readable, int rsize,
                             char *nums, int nsize )
{
	int       i;
	int       bNum;
	int       listItems = 0;
	int       totalListItems = 0;
	gentity_t *ent;
	int       removalCounts[ BA_NUM_BUILDABLES ] = { 0 };
	const buildableAttributes_t *attr;
	int       numRemoved = 0;

	if ( readable && rsize )
	{
		readable[ 0 ] = '\0';
	}

	if ( nums && nsize )
	{
		nums[ 0 ] = '\0';
	}

	for ( i = 0; i < level.numBuildablesForRemoval; i++ )
	{
		ent = level.markedBuildables[ i ];
		attr = BG_Buildable( ent->s.modelindex );
		bNum = attr->number;

		if ( removalCounts[ bNum ] == 0 )
		{
			totalListItems++;
		}

		G_Deconstruct( ent, deconner, MOD_REPLACE );

		removalCounts[ bNum ]++;

		if ( nums )
		{
			Q_strcat( nums, nsize, va( " %ld", ( long )( ent - g_entities ) ) );
		}

		numRemoved++;
	}

	if ( !readable )
	{
		return numRemoved;
	}

	for ( i = 0; i < BA_NUM_BUILDABLES; i++ )
	{
		if ( removalCounts[ i ] )
		{
			if ( listItems )
			{
				if ( listItems == ( totalListItems - 1 ) )
				{
					Q_strcat( readable, rsize,  va( "%s and ",
					                                ( totalListItems > 2 ) ? "," : "" ) );
				}
				else
				{
					Q_strcat( readable, rsize, ", " );
				}
			}

			Q_strcat( readable, rsize, BG_Buildable( i )->humanName );

			if ( removalCounts[ i ] > 1 )
			{
				Q_strcat( readable, rsize, va( " (%dx)", removalCounts[ i ] ) );
			}

			listItems++;
		}
	}

	return numRemoved;
}

static qboolean IsSetForDeconstruction( gentity_t *ent )
{
	int markedNum;

	for ( markedNum = 0; markedNum < level.numBuildablesForRemoval; markedNum++ )
	{
		if ( level.markedBuildables[ markedNum ] == ent )
		{
			return qtrue;
		}
	}

	return qfalse;
}

static itemBuildError_t BuildableReplacementChecks( buildable_t oldBuildable, buildable_t newBuildable )
{
	// don't replace the main buildable with any other buildable
	if (    ( oldBuildable == BA_H_REACTOR  && newBuildable != BA_H_REACTOR  )
	     || ( oldBuildable == BA_A_OVERMIND && newBuildable != BA_A_OVERMIND ) )
	{
		return IBE_MAINSTRUCTURE;
	}

	// don't replace last spawn with a non-spawn
	if (    ( oldBuildable == BA_H_SPAWN && newBuildable != BA_H_SPAWN &&
	          level.team[ TEAM_HUMANS ].numSpawns == 1 )
	     || ( oldBuildable == BA_A_SPAWN && newBuildable != BA_A_SPAWN &&
	          level.team[ TEAM_ALIENS ].numSpawns == 1 ) )
	{
		return IBE_LASTSPAWN;
	}

	// TODO: don't replace an egg that is the single creep provider for a buildable

	return IBE_NONE;
}

/*
=================
G_PredictBuildablePower

Predicts whether a buildable can be built without causing interference.
Takes buildables prepared for deconstruction into account.
=================
*/
static qboolean PredictBuildablePower( buildable_t buildable, vec3_t origin )
{
	gentity_t       *neighbor, *buddy;
	float           distance, ownPrediction, neighborPrediction;
	int             powerConsumption, baseSupply;

	powerConsumption = BG_Buildable( buildable )->powerConsumption;

	if ( !powerConsumption )
	{
		return qtrue;
	}

	// reactor enables base supply everywhere on the map
	if ( G_AliveReactor() )
	{
		baseSupply = g_powerBaseSupply.integer;
	}
	else
	{
		baseSupply = 0;
	}

	ownPrediction = baseSupply - powerConsumption;

	neighbor = NULL;
	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, origin, PowerRelevantRange() ) ) )
	{
		// only predict interference with friendly buildables
		if ( neighbor->s.eType != ET_BUILDABLE || neighbor->buildableTeam != TEAM_HUMANS )
		{
			continue;
		}

		// discard neighbors that are set for deconstruction
		if ( IsSetForDeconstruction( neighbor ) )
		{
			continue;
		}

		distance = Distance( origin, neighbor->s.origin );

		ownPrediction += IncomingInterference( buildable, neighbor, distance, qtrue );
		neighborPrediction = neighbor->expectedSparePower + OutgoingInterference( buildable, neighbor, distance );

		// check power of neighbor, with regards to pending deconstruction
		if ( neighborPrediction < 0.0f && distance < g_powerCompetitionRange.integer )
		{
			buddy = NULL;
			while ( ( buddy = G_IterateEntitiesWithinRadius( buddy, neighbor->s.origin, PowerRelevantRange() ) ) )
			{
				if ( IsSetForDeconstruction( buddy ) )
				{
					distance = Distance( neighbor->s.origin, buddy->s.origin );
					neighborPrediction -= IncomingInterference( (buildable_t) neighbor->s.modelindex, buddy, distance, qtrue );
				}
			}

			if ( neighborPrediction < 0.0f )
			{
				return qfalse;
			}
		}
	}

	return ( ownPrediction >= 0.0f );
}

/*
===============
G_PrepareBuildableReplacement

Attempts to build a set of buildables that have to be deconstructed for a new buildable.
Takes both power consumption and build points into account.
Sets level.markedBuildables and level.numBuildablesForRemoval.
===============
*/
static itemBuildError_t PrepareBuildableReplacement( buildable_t buildable, vec3_t origin )
{
	int              entNum, listLen;
	gentity_t        *ent, *list[ MAX_GENTITIES ];
	itemBuildError_t reason;
	const buildableAttributes_t *attr, *entAttr;

	// power consumption related
	int       numNeighbors, neighborNum, neighbors[ MAX_GENTITIES ];
	gentity_t *neighbor;

	// resource related
	float     cost;

	level.numBuildablesForRemoval = 0;

	attr = BG_Buildable( buildable );

	// ---------------
	// main buildables
	// ---------------

	if ( buildable == BA_H_REACTOR )
	{
		ent = G_Reactor();

		if ( ent )
		{
			if ( ent->deconstruct )
			{
				level.markedBuildables[ level.numBuildablesForRemoval++ ] = ent;
			}
			else
			{
				return IBE_ONEREACTOR;
			}
		}
	}
	else if ( buildable == BA_A_OVERMIND )
	{
		ent = G_Overmind();

		if ( ent )
		{
			if ( ent->deconstruct )
			{
				level.markedBuildables[ level.numBuildablesForRemoval++ ] = ent;
			}
			else
			{
				return IBE_ONEOVERMIND;
			}
		}
	}

	// -------------------
	// check for collision
	// -------------------

	for ( entNum = MAX_CLIENTS; entNum < level.num_entities; entNum++ )
	{
		ent = &g_entities[ entNum ];

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( BuildablesIntersect( buildable, origin, (buildable_t) ent->s.modelindex, ent->s.origin ) )
		{
			if ( ent->buildableTeam == attr->team && ent->deconstruct )
			{
				// ignore main buildable since it will already be on the list
				if (    !( buildable == BA_H_REACTOR  && ent->s.modelindex == BA_H_REACTOR )
				     && !( buildable == BA_A_OVERMIND && ent->s.modelindex == BA_A_OVERMIND ) )
				{
					// apply general replacement rules
					if ( ( reason = BuildableReplacementChecks( (buildable_t) ent->s.modelindex, buildable ) ) != IBE_NONE )
					{
						return reason;
					}

					level.markedBuildables[ level.numBuildablesForRemoval++ ] = ent;
				}
			}
			else
			{
				return IBE_NOROOM;
			}
		}
	}

	// ---------------
	// check for power
	// ---------------

	if ( attr->team == TEAM_HUMANS )
	{
		numNeighbors = 0;
		neighbor = NULL;

		// assemble a list of closeby human buildable IDs
		while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, origin, PowerRelevantRange() ) ) )
		{
			if ( neighbor->s.eType != ET_BUILDABLE || neighbor->buildableTeam != TEAM_HUMANS )
			{
				continue;
			}

			neighbors[ numNeighbors++ ] = neighbor - g_entities;
		}

		// sort by distance
		if ( numNeighbors > 0 )
		{
			VectorCopy( origin, compareEntityDistanceOrigin );
			qsort( neighbors, numNeighbors, sizeof( int ), CompareEntityDistance );
		}

		neighborNum = 0;

		// check for power
		while ( !PredictBuildablePower( buildable, origin ) )
		{
			if ( neighborNum == numNeighbors )
			{
				// can't build due to unsolvable lack of power
				return IBE_NOPOWERHERE;
			}

			// find an interfering buildable to remove
			for ( ; neighborNum < numNeighbors; neighborNum++ )
			{
				neighbor = &g_entities[ neighbors[ neighborNum ] ];

				// when predicting, only take human buildables into account
				if ( neighbor->s.eType != ET_BUILDABLE || neighbor->buildableTeam != TEAM_HUMANS )
				{
					continue;
				}

				// check if marked for deconstruction
				if ( !neighbor->deconstruct )
				{
					continue;
				}

				// discard non-interfering buildable types
				switch ( neighbor->s.modelindex )
				{
					case BA_H_REPEATER:
					case BA_H_REACTOR:
						continue;
				}

				// check if already set for deconstruction
				if ( IsSetForDeconstruction( neighbor ) )
				{
					continue;
				}

				// apply general replacement rules
				if ( BuildableReplacementChecks( (buildable_t) neighbor->s.modelindex, buildable ) != IBE_NONE )
				{
					continue;
				}

				// set for deconstruction
				level.markedBuildables[ level.numBuildablesForRemoval++ ] = neighbor;

				// recheck
				neighborNum++;
				break;
			}
		}
	}

	// -------------------
	// check for resources
	// -------------------

	cost = attr->buildPoints;

	// if we already have set buildables for construction, decrease cost
	for ( entNum = 0; entNum < level.numBuildablesForRemoval; entNum++ )
	{
		ent = level.markedBuildables[ entNum ];
		entAttr = BG_Buildable( ent->s.modelindex );

		cost -= entAttr->buildPoints * ( ent->health / ( float )entAttr->health );
	}
	cost = MAX( 0.0f, cost );

	// check if we can already afford the new buildable
	if ( G_CanAffordBuildPoints( attr->team, cost ) )
	{
		return IBE_NONE;
	}

	// build a list of additional buildables that can be deconstructed
	listLen = 0;

	for ( entNum = MAX_CLIENTS; entNum < level.num_entities; entNum++ )
	{
		ent = &g_entities[ entNum ];

		// check if buildable of own team
		if ( ent->s.eType != ET_BUILDABLE || ent->buildableTeam != attr->team )
		{
			continue;
		}

		// check if available for deconstruction
		if ( !ent->deconstruct )
		{
			continue;
		}

		// check if already set for deconstruction
		if ( IsSetForDeconstruction( ent ) )
		{
			continue;
		}

		// apply general replacement rules
		if ( BuildableReplacementChecks( (buildable_t) ent->s.modelindex, buildable ) != IBE_NONE )
		{
			continue;
		}

		// add to list
		list[ listLen++ ] = ent;
	}

	// sort the list
	qsort( list, listLen, sizeof( gentity_t* ), CompareBuildablesForRemoval );

	// set buildables for deconstruction until we can pay for the new buildable
	for ( entNum = 0; entNum < listLen; entNum++ )
	{
		ent = list[ entNum ];
		entAttr = BG_Buildable( ent->s.modelindex );

		level.markedBuildables[ level.numBuildablesForRemoval++ ] = ent;

		cost -= entAttr->buildPoints * ( ent->health / ( float )entAttr->health );
		cost = MAX( 0.0f, cost );

		// check if we have enough resources now
		if ( G_CanAffordBuildPoints( attr->team, cost ) )
		{
			return IBE_NONE;
		}
	}

	// we don't have enough resources
	if ( attr->team == TEAM_ALIENS )
	{
		return IBE_NOALIENBP;
	}
	else if ( attr->team == TEAM_HUMANS )
	{
		return IBE_NOHUMANBP;
	}
	else
	{
		// shouldn't really happen
		return IBE_NOHUMANBP;
	}
}

/*
================
G_SetBuildableLinkState

Links or unlinks all the buildable entities
================
*/
static void SetBuildableLinkState( qboolean link )
{
	int       i;
	gentity_t *ent;

	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( link )
		{
			trap_LinkEntity( ent );
		}
		else
		{
			trap_UnlinkEntity( ent );
		}
	}
}

static void SetBuildableMarkedLinkState( qboolean link )
{
	int       i;
	gentity_t *ent;

	for ( i = 0; i < level.numBuildablesForRemoval; i++ )
	{
		ent = level.markedBuildables[ i ];

		if ( link )
		{
			trap_LinkEntity( ent );
		}
		else
		{
			trap_UnlinkEntity( ent );
		}
	}
}

/*
================
G_CanBuild

Checks to see if a buildable can be built
================
*/
itemBuildError_t G_CanBuild( gentity_t *ent, buildable_t buildable, int distance,
                             vec3_t origin, vec3_t normal, int *groundEntNum )
{
	vec3_t           angles;
	vec3_t           entity_origin;
	vec3_t           mins, maxs;
	trace_t          tr1, tr2, tr3;
	itemBuildError_t reason = IBE_NONE, tempReason;
	gentity_t        *tempent;
	float            minNormal;
	qboolean         invert;
	int              contents;
	playerState_t    *ps = &ent->client->ps;

	// Stop all buildables from interacting with traces
	SetBuildableLinkState( qfalse );

	BG_BuildableBoundingBox( buildable, mins, maxs );

	BG_PositionBuildableRelativeToPlayer( ps, mins, maxs, trap_Trace, entity_origin, angles, &tr1 );
	trap_Trace( &tr2, entity_origin, mins, maxs, entity_origin, -1, MASK_PLAYERSOLID ); // Setting entnum to -1 means that the player isn't ignored
	trap_Trace( &tr3, ps->origin, NULL, NULL, entity_origin, ent->s.number, MASK_PLAYERSOLID );

	VectorCopy( entity_origin, origin );
	*groundEntNum = tr1.entityNum;
	VectorCopy( tr1.plane.normal, normal );
	minNormal = BG_Buildable( buildable )->minNormal;
	invert = BG_Buildable( buildable )->invertNormal;

	// Can we build at this angle?
	if ( !( normal[ 2 ] >= minNormal || ( invert && normal[ 2 ] <= -minNormal ) ) )
	{
		reason = IBE_NORMAL;
	}

	if ( tr1.entityNum != ENTITYNUM_WORLD )
	{
		reason = IBE_NORMAL;
	}

	contents = trap_PointContents( entity_origin, -1 );

	// Prepare replacment of other buildables
	if ( ( tempReason = PrepareBuildableReplacement( buildable, origin ) ) != IBE_NONE )
	{
		reason = tempReason;

		if ( reason == IBE_NOPOWERHERE && !G_AliveReactor() )
		{
			reason = IBE_NOREACTOR;
		}
		else if ( reason == IBE_NOCREEP && !G_ActiveOvermind() )
		{
			reason = IBE_NOOVERMIND;
		}
	}
	else if ( ent->client->pers.team == TEAM_ALIENS )
	{
		// Check for Overmind
		if ( buildable != BA_A_OVERMIND )
		{
			if ( !G_ActiveOvermind() )
			{
				reason = IBE_NOOVERMIND;
			}
		}

		// Check for creep
		if ( BG_Buildable( buildable )->creepTest )
		{
			if ( !IsCreepHere( entity_origin ) )
			{
				reason = IBE_NOCREEP;
			}
		}

		// Check surface permissions
		if ( (tr1.surfaceFlags & (SURF_NOALIENBUILD | SURF_NOBUILD)) || (contents & (CONTENTS_NOALIENBUILD | CONTENTS_NOBUILD)) )
		{
			reason = IBE_SURFACE;
		}

		// Check level permissions
		if ( !g_alienAllowBuilding.integer )
		{
			reason = IBE_DISABLED;
		}
	}
	else if ( ent->client->pers.team == TEAM_HUMANS )
	{
		// Check permissions
		if ( (tr1.surfaceFlags & (SURF_NOHUMANBUILD | SURF_NOBUILD)) || (contents & (CONTENTS_NOHUMANBUILD | CONTENTS_NOBUILD)) )
		{
			reason = IBE_SURFACE;
		}

		// Check level permissions
		if ( !g_humanAllowBuilding.integer )
		{
			reason = IBE_DISABLED;
		}
	}

	// Can we only have one of these?
	if ( BG_Buildable( buildable )->uniqueTest )
	{
		tempent = FindBuildable( buildable );

		if ( tempent && !tempent->deconstruct )
		{
			switch ( buildable )
			{
				case BA_A_OVERMIND:
					reason = IBE_ONEOVERMIND;
					break;

				case BA_H_REACTOR:
					reason = IBE_ONEREACTOR;
					break;

				default:
					Com_Error( ERR_FATAL, "No reason for denying build of %d", buildable );
					break;
			}
		}
	}

	// Relink buildables
	SetBuildableLinkState( qtrue );

	//check there is enough room to spawn from (presuming this is a spawn)
	if ( reason == IBE_NONE )
	{
		SetBuildableMarkedLinkState( qfalse );

		if ( G_CheckSpawnPoint( ENTITYNUM_NONE, origin, normal, buildable, NULL ) != NULL )
		{
			reason = IBE_NORMAL;
		}

		SetBuildableMarkedLinkState( qtrue );
	}

	//this item does not fit here
	if ( tr2.fraction < 1.0f || tr3.fraction < 1.0f )
	{
		reason = IBE_NOROOM;
	}

	if ( reason != IBE_NONE )
	{
		level.numBuildablesForRemoval = 0;
	}

	return reason;
}

/*
================
G_Build

Spawns a buildable
================
*/
static gentity_t *Build( gentity_t *builder, buildable_t buildable,
                         const vec3_t origin, const vec3_t normal, const vec3_t angles, int groundEntNum )
{
	gentity_t  *built;
	char       readable[ MAX_STRING_CHARS ];
	char       buildnums[ MAX_STRING_CHARS ];
	buildLog_t *log;
	const buildableAttributes_t *attr = BG_Buildable( buildable );

	if ( builder->client )
	{
		log = G_BuildLogNew( builder, BF_CONSTRUCT );
	}
	else
	{
		log = NULL;
	}

	built = G_NewEntity();

	// Free existing buildables
	G_FreeMarkedBuildables( builder, readable, sizeof( readable ), buildnums, sizeof( buildnums ) );

	// Spawn the buildable
	built->s.eType = ET_BUILDABLE;
	built->killedBy = ENTITYNUM_NONE;
	built->classname = attr->entityName;
	built->s.modelindex = buildable;
	built->s.modelindex2 = attr->team;
	built->buildableTeam = (team_t) built->s.modelindex2;
	BG_BuildableBoundingBox( buildable, built->r.mins, built->r.maxs );

	built->health = ( int )ceil( attr->health * BUILDABLE_START_HEALTH_FRAC );

	built->splashDamage = attr->splashDamage;
	built->splashRadius = attr->splashRadius;
	built->splashMethodOfDeath = attr->meansOfDeath;

	built->nextthink = level.time;
	built->takedamage = qtrue;
	built->enabled = qfalse;
	built->spawned = qfalse;

	// build instantly in cheat mode
	if ( builder->client && g_cheats.integer )
	{
		built->health = attr->health;
		built->creationTime -= attr->buildTime;
	}
	built->s.time = built->creationTime;

	//things that vary for each buildable that aren't in the dbase
	switch ( buildable )
	{
		case BA_A_SPAWN:
			built->die = AGeneric_Die;
			built->think = ASpawn_Think;
			built->pain = AGeneric_Pain;
			break;

		case BA_A_BARRICADE:
			built->die = ABarricade_Die;
			built->think = ABarricade_Think;
			built->pain = ABarricade_Pain;
			built->touch = ABarricade_Touch;
			built->shrunkTime = 0;
			ABarricade_Shrink( built, qtrue );
			break;

		case BA_A_BOOSTER:
			built->die = AGeneric_Die;
			built->think = ABooster_Think;
			built->pain = AGeneric_Pain;
			built->touch = ABooster_Touch;
			break;

		case BA_A_ACIDTUBE:
			built->die = AGeneric_Die;
			built->think = AAcidTube_Think;
			built->pain = AGeneric_Pain;
			break;

		case BA_A_HIVE:
			built->die = AGeneric_Die;
			built->think = AHive_Think;
			built->pain = AHive_Pain;
			break;

		case BA_A_TRAPPER:
			built->die = AGeneric_Die;
			built->think = ATrapper_Think;
			built->pain = AGeneric_Pain;
			break;

		case BA_A_LEECH:
			built->die = ALeech_Die;
			built->think = ALeech_Think;
			built->pain = AGeneric_Pain;
			break;

		case BA_A_OVERMIND:
			built->die = AOvermind_Die;
			built->think = AOvermind_Think;
			built->pain = AGeneric_Pain;
			break;

		case BA_H_SPAWN:
			built->die = HGeneric_Die;
			built->think = HSpawn_Think;
			break;

		case BA_H_MGTURRET:
			built->die = HTurret_Die;
			built->think = HTurret_Think;
			break;

		case BA_H_TESLAGEN:
			built->die = HGeneric_Die;
			built->think = HTeslaGen_Think;
			break;

		case BA_H_ARMOURY:
			built->think = HArmoury_Think;
			built->die = HGeneric_Die;
			built->use = HArmoury_Use;
			break;

		case BA_H_MEDISTAT:
			built->think = HMedistat_Think;
			built->die = HMedistat_Die;
			break;

		case BA_H_DRILL:
			built->think = HDrill_Think;
			built->die = HDrill_Die;
			break;

		case BA_H_REACTOR:
			built->think = HReactor_Think;
			built->die = HReactor_Die;
			built->powered = built->active = qtrue;
			break;

		case BA_H_REPEATER:
			built->think = HRepeater_Think;
			built->die = HGeneric_Die;
			built->customNumber = -1;
			break;

		default:
			break;
	}

	// Add bot obstacles
	if ( built->r.maxs[2] - built->r.mins[2] > 47.0f ) // HACK: Fixed jump height
	{
		vec3_t mins;
		vec3_t maxs;
		VectorCopy( built->r.mins, mins );
		VectorCopy( built->r.maxs, maxs );
		VectorAdd( mins, origin, mins );
		VectorAdd( maxs, origin, maxs );
		trap_BotAddObstacle( mins, maxs, &built->obstacleHandle );
	}

	built->r.contents = CONTENTS_BODY;
	built->clipmask   = MASK_PLAYERSOLID;
	built->target     = NULL;
	built->s.weapon   = attr->weapon;

	if ( builder->client )
	{
		built->builtBy = builder->client->pers.namelog;
	}
	else if ( builder->builtBy )
	{
		built->builtBy = builder->builtBy;
	}
	else
	{
		built->builtBy = NULL;
	}

	G_SetOrigin( built, origin );

	// set turret angles
	VectorCopy( builder->s.angles2, built->s.angles2 );

	VectorCopy( angles, built->s.angles );
	built->s.angles[ PITCH ] = 0.0f;
	built->s.angles2[ YAW ] = angles[ YAW ];
	built->s.angles2[ PITCH ] = TURRET_PITCH_CAP;
	built->physicsBounce = attr->bounce;

	built->s.groundEntityNum = groundEntNum;
	if ( groundEntNum == ENTITYNUM_NONE )
	{
		built->s.pos.trType = attr->traj;
		built->s.pos.trTime = level.time;
		// gently nudge the buildable onto the surface :)
		VectorScale( normal, -50.0f, built->s.pos.trDelta );
	}

	built->s.generic1 = MAX( built->health, 0 );

	built->powered = qtrue;
	built->s.eFlags |= EF_B_POWERED;

	built->s.eFlags &= ~EF_B_SPAWNED;

	VectorCopy( normal, built->s.origin2 );

	G_AddEvent( built, EV_BUILD_CONSTRUCT, 0 );

	G_SetIdleBuildableAnim( built, ( buildableAnimNumber_t )attr->idleAnim );

	if ( built->builtBy )
	{
		G_SetBuildableAnim( built, BANIM_CONSTRUCT1, qtrue );
	}

	trap_LinkEntity( built );

	if ( builder->client )
	{
	        // readable and the model name shouldn't need quoting
		G_TeamCommand( (team_t) builder->client->pers.team,
		               va( "print_tr %s %s %s %s", ( readable[ 0 ] ) ?
						QQ( N_("$1$ ^2built^7 by $2$^7, ^3replacing^7 $3$\n") ) :
						QQ( N_("$1$ ^2built^7 by $2$$3$\n") ),
		                   Quote( BG_Buildable( built->s.modelindex )->humanName ),
		                   Quote( builder->client->pers.netname ),
		                   Quote( readable ) ) );
		G_LogPrintf( "Construct: %d %d %s%s: %s" S_COLOR_WHITE " is building "
		             "%s%s%s\n",
		             ( int )( builder - g_entities ),
		             ( int )( built - g_entities ),
		             BG_Buildable( built->s.modelindex )->name,
		             buildnums,
		             builder->client->pers.netname,
		             BG_Buildable( built->s.modelindex )->humanName,
		             readable[ 0 ] ? ", replacing " : "",
		             readable );
	}

	if ( log )
	{
		// HACK: Assume the buildable got build in full
		built->momentumEarned = G_PredictMomentumForBuilding( built );
		G_BuildLogSet( log, built );
	}

	return built;
}

qboolean G_BuildIfValid( gentity_t *ent, buildable_t buildable )
{
	float  dist;
	vec3_t origin, normal;
	int    groundEntNum;
	vec3_t forward, aimDir;

	BG_GetClientNormal( &ent->client->ps, normal);
	AngleVectors( ent->client->ps.viewangles, aimDir, NULL, NULL );
	ProjectPointOnPlane( forward, aimDir, normal );
	VectorNormalize( forward );
	dist = BG_Class( ent->client->ps.stats[ STAT_CLASS ] )->buildDist * DotProduct( forward, aimDir );

	switch ( G_CanBuild( ent, buildable, dist, origin, normal, &groundEntNum ) )
	{
		case IBE_NONE:
			Build( ent, buildable, origin, normal, ent->s.apos.trBase, groundEntNum );
			G_ModifyBuildPoints( BG_Buildable( buildable )->team, -BG_Buildable( buildable )->buildPoints );
			return qtrue;

		case IBE_NOALIENBP:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOBP );
			return qfalse;

		case IBE_NOOVERMIND:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOOVMND );
			return qfalse;

		case IBE_NOCREEP:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOCREEP );
			return qfalse;

		case IBE_ONEOVERMIND:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_ONEOVERMIND );
			return qfalse;

		case IBE_NORMAL:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_NORMAL );
			return qfalse;

		case IBE_SURFACE:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_NORMAL );
			return qfalse;

		case IBE_DISABLED:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_DISABLED );
			return qfalse;

		case IBE_ONEREACTOR:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_ONEREACTOR );
			return qfalse;

		case IBE_NOPOWERHERE:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOPOWERHERE );
			return qfalse;

		case IBE_NOREACTOR:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOREACTOR );
			return qfalse;

		case IBE_NOROOM:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_NOROOM );
			return qfalse;

		case IBE_NOHUMANBP:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOBP );
			return qfalse;

		case IBE_LASTSPAWN:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_LASTSPAWN );
			return qfalse;

		case IBE_MAINSTRUCTURE:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_MAINSTRUCTURE );
			return qfalse;

		default:
			break;
	}

	return qfalse;
}

/*
================
G_FinishSpawningBuildable

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
static gentity_t *FinishSpawningBuildable( gentity_t *ent, qboolean force )
{
	trace_t     tr;
	vec3_t      normal, dest;
	gentity_t   *built;
	buildable_t buildable = (buildable_t) ent->s.modelindex;

	if ( ent->s.origin2[ 0 ] || ent->s.origin2[ 1 ] || ent->s.origin2[ 2 ] )
	{
		VectorCopy( ent->s.origin2, normal );
	}
	else if ( BG_Buildable( buildable )->traj == TR_BUOYANCY )
	{
		VectorSet( normal, 0.0f, 0.0f, -1.0f );
	}
	else
	{
		VectorSet( normal, 0.0f, 0.0f, 1.0f );
	}

	built = Build( ent, buildable, ent->s.pos.trBase,
	                 normal, ent->s.angles, ENTITYNUM_NONE );

	built->takedamage = qtrue;
	built->spawned = qtrue; //map entities are already spawned
	built->enabled = qtrue;
	built->health = BG_Buildable( buildable )->health;
	built->s.eFlags |= EF_B_SPAWNED;

	// drop towards normal surface
	VectorScale( built->s.origin2, -4096.0f, dest );
	VectorAdd( dest, built->s.origin, dest );

	trap_Trace( &tr, built->s.origin, built->r.mins, built->r.maxs, dest, built->s.number, built->clipmask );

	if ( tr.startsolid && !force )
	{
		G_Printf( S_COLOR_YELLOW "G_FinishSpawningBuildable: %s startsolid at %s\n",
		          built->classname, vtos( built->s.origin ) );
		G_FreeEntity( built );
		return NULL;
	}

	//point items in the correct direction
	VectorCopy( tr.plane.normal, built->s.origin2 );

	// allow to ride movers
	built->s.groundEntityNum = tr.entityNum;

	G_SetOrigin( built, tr.endpos );

	trap_LinkEntity( built );
	return built;
}

/*
============
G_SpawnBuildableThink

Complete spawning a buildable using its placeholder
============
*/
static void SpawnBuildableThink( gentity_t *ent )
{
	FinishSpawningBuildable( ent, qfalse );
	G_FreeEntity( ent );
}

/*
============
G_SpawnBuildable

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnBuildable( gentity_t *ent, buildable_t buildable )
{
	ent->s.modelindex = buildable;

	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = SpawnBuildableThink;
}

void G_LayoutSave( const char *name )
{
	char         map[ MAX_QPATH ];
	char         fileName[ MAX_OSPATH ];
	fileHandle_t f;
	int          len;
	int          i;
	gentity_t    *ent;
	char         *s;

	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );

	if ( !map[ 0 ] )
	{
		G_Printf( "layoutsave: mapname is null\n" );
		return;
	}

	Com_sprintf( fileName, sizeof( fileName ), "layouts/%s/%s.dat", map, name );

	len = trap_FS_FOpenFile( fileName, &f, FS_WRITE );

	if ( len < 0 )
	{
		G_Printf( "layoutsave: could not open %s\n", fileName );
		return;
	}

	G_Printf( "layoutsave: saving layout to %s\n", fileName );

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ )
	{
		ent = &level.gentities[ i ];

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		s = va( "%s %f %f %f %f %f %f %f %f %f %f %f %f\n",
		        BG_Buildable( ent->s.modelindex )->name,
		        ent->s.pos.trBase[ 0 ],
		        ent->s.pos.trBase[ 1 ],
		        ent->s.pos.trBase[ 2 ],
		        ent->s.angles[ 0 ],
		        ent->s.angles[ 1 ],
		        ent->s.angles[ 2 ],
		        ent->s.origin2[ 0 ],
		        ent->s.origin2[ 1 ],
		        ent->s.origin2[ 2 ],
		        ent->s.angles2[ 0 ],
		        ent->s.angles2[ 1 ],
		        ent->s.angles2[ 2 ] );
		trap_FS_Write( s, strlen( s ), f );
	}

	trap_FS_FCloseFile( f );
}

int G_LayoutList( const char *map, char *list, int len )
{
	// up to 128 single character layout names could fit in layouts
	char fileList[( MAX_CVAR_VALUE_STRING / 2 ) * 5 ] = { "" };
	char layouts[ MAX_CVAR_VALUE_STRING ] = { "" };
	int  numFiles, i, fileLen = 0, listLen;
	int  count = 0;
	char *filePtr;

	Q_strcat( layouts, sizeof( layouts ), S_BUILTIN_LAYOUT " " );
	numFiles = trap_FS_GetFileList( va( "layouts/%s", map ), ".dat",
	                                fileList, sizeof( fileList ) );
	filePtr = fileList;

	for ( i = 0; i < numFiles; i++, filePtr += fileLen + 1 )
	{
		fileLen = strlen( filePtr );
		listLen = strlen( layouts );

		if ( fileLen < 5 )
		{
			continue;
		}

		// list is full, stop trying to add to it
		if ( ( listLen + fileLen ) >= sizeof( layouts ) )
		{
			break;
		}

		Q_strcat( layouts,  sizeof( layouts ), filePtr );
		listLen = strlen( layouts );

		// strip extension and add space delimiter
		layouts[ listLen - 4 ] = ' ';
		layouts[ listLen - 3 ] = '\0';
		count++;
	}

	if ( count != numFiles )
	{
		G_Printf( S_WARNING "layout list was truncated to %d "
		          "layouts, but %d layout files exist in layouts/%s/.\n",
		          count, numFiles, map );
	}

	Q_strncpyz( list, layouts, len );
	return count + 1;
}

/*
============
G_LayoutSelect

set level.layout based on g_layouts or g_layoutAuto
============
*/
void G_LayoutSelect( void )
{
	char fileName[ MAX_OSPATH ];
	char layouts[ MAX_CVAR_VALUE_STRING ];
	char layouts2[ MAX_CVAR_VALUE_STRING ];
	char *l;
	char map[ MAX_QPATH ];
	char *s;
	int  cnt = 0;
	int  layoutNum;

	Q_strncpyz( layouts, g_layouts.string, sizeof( layouts ) );
	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );

	// one time use cvar
	trap_Cvar_Set( "g_layouts", "" );

	if ( !layouts[ 0 ] )
	{
		Q_strncpyz( layouts, g_defaultLayouts.string, sizeof( layouts ) );
	}

	// pick an included layout at random if no list has been provided
	if ( !layouts[ 0 ] && g_layoutAuto.integer )
	{
		G_LayoutList( map, layouts, sizeof( layouts ) );
	}

	if ( !layouts[ 0 ] )
	{
		return;
	}

	Q_strncpyz( layouts2, layouts, sizeof( layouts2 ) );
	l = &layouts2[ 0 ];
	layouts[ 0 ] = '\0';

	while ( 1 )
	{
		s = COM_ParseExt( &l, qfalse );

		if ( !*s )
		{
			break;
		}

		if ( !Q_stricmp( s, S_BUILTIN_LAYOUT ) )
		{
			Q_strcat( layouts, sizeof( layouts ), s );
			Q_strcat( layouts, sizeof( layouts ), " " );
			cnt++;
			continue;
		}

		Com_sprintf( fileName, sizeof( fileName ), "layouts/%s/%s.dat", map, s );

		if ( trap_FS_FOpenFile( fileName, NULL, FS_READ ) > 0 )
		{
			Q_strcat( layouts, sizeof( layouts ), s );
			Q_strcat( layouts, sizeof( layouts ), " " );
			cnt++;
		}
		else
		{
			G_Printf( S_WARNING "layout \"%s\" does not exist\n", s );
		}
	}

	if ( !cnt )
	{
		G_Printf( S_ERROR "none of the specified layouts could be "
		          "found, using map default\n" );
		return;
	}

	layoutNum = rand() / ( RAND_MAX / cnt + 1 ) + 1;
	cnt = 0;

	Q_strncpyz( layouts2, layouts, sizeof( layouts2 ) );
	l = &layouts2[ 0 ];

	while ( 1 )
	{
		s = COM_ParseExt( &l, qfalse );

		if ( !*s )
		{
			break;
		}

		Q_strncpyz( level.layout, s, sizeof( level.layout ) );
		cnt++;

		if ( cnt >= layoutNum )
		{
			break;
		}
	}

	G_Printf( "using layout \"%s\" from list (%s)\n", level.layout, layouts );
	trap_Cvar_Set( "layout", level.layout );
}

static void LayoutBuildItem( buildable_t buildable, vec3_t origin,
                             vec3_t angles, vec3_t origin2, vec3_t angles2 )
{
	gentity_t *builder;

	builder = G_NewEntity();
	VectorCopy( origin, builder->s.pos.trBase );
	VectorCopy( angles, builder->s.angles );
	VectorCopy( origin2, builder->s.origin2 );
	VectorCopy( angles2, builder->s.angles2 );
	G_SpawnBuildable( builder, buildable );
}

/*
============
G_LayoutLoad

load the layout .dat file indicated by level.layout and spawn buildables
as if a builder was creating them
============
*/
void G_LayoutLoad( void )
{
	fileHandle_t f;
	int          len;
	char         *layout, *layoutHead;
	char         map[ MAX_QPATH ];
	char         buildName[ MAX_TOKEN_CHARS ];
	int          buildable;
	vec3_t       origin = { 0.0f, 0.0f, 0.0f };
	vec3_t       angles = { 0.0f, 0.0f, 0.0f };
	vec3_t       origin2 = { 0.0f, 0.0f, 0.0f };
	vec3_t       angles2 = { 0.0f, 0.0f, 0.0f };
	char         line[ MAX_STRING_CHARS ];
	int          i = 0;
	const buildableAttributes_t *attr;

	if ( !level.layout[ 0 ] || !Q_stricmp( level.layout, S_BUILTIN_LAYOUT ) )
	{
		return;
	}

	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
	len = trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, level.layout ), &f, FS_READ );

	if ( len < 0 )
	{
		G_Printf( "ERROR: layout %s could not be opened\n", level.layout );
		return;
	}

	layoutHead = layout = (char*) BG_Alloc( len + 1 );
	trap_FS_Read( layout, len, f );
	layout[ len ] = '\0';
	trap_FS_FCloseFile( f );

	while ( *layout )
	{
		if ( i >= sizeof( line ) - 1 )
		{
			G_Printf( S_ERROR "line overflow in %s before \"%s\"\n",
			          va( "layouts/%s/%s.dat", map, level.layout ), line );
			break;
		}

		line[ i++ ] = *layout;
		line[ i ] = '\0';

		if ( *layout == '\n' )
		{
			i = 0;
			sscanf( line, "%s %f %f %f %f %f %f %f %f %f %f %f %f\n",
			        buildName,
			        &origin[ 0 ], &origin[ 1 ], &origin[ 2 ],
			        &angles[ 0 ], &angles[ 1 ], &angles[ 2 ],
			        &origin2[ 0 ], &origin2[ 1 ], &origin2[ 2 ],
			        &angles2[ 0 ], &angles2[ 1 ], &angles2[ 2 ] );

			attr = BG_BuildableByName( buildName );
			buildable = attr->number;

			if ( buildable <= BA_NONE || buildable >= BA_NUM_BUILDABLES )
			{
				G_Printf( S_WARNING "bad buildable name (%s) in layout."
				          " skipping\n", buildName );
			}
			else
			{
				LayoutBuildItem( (buildable_t) buildable, origin, angles, origin2, angles2 );
				level.team[ attr->team ].layoutBuildPoints += attr->buildPoints;
			}
		}

		layout++;
	}

	BG_Free( layoutHead );
}

void G_BaseSelfDestruct( team_t team )
{
	int       i;
	gentity_t *ent;

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ )
	{
		ent = &level.gentities[ i ];

		if ( ent->health <= 0 )
		{
			continue;
		}

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( ent->buildableTeam != team )
		{
			continue;
		}

		G_Damage( ent, NULL, NULL, NULL, NULL, 10000, 0, MOD_SUICIDE );
	}
}

buildLog_t *G_BuildLogNew( gentity_t *actor, buildFate_t fate )
{
	buildLog_t *log = &level.buildLog[ level.buildId++ % MAX_BUILDLOG ];

	if ( level.numBuildLogs < MAX_BUILDLOG )
	{
		level.numBuildLogs++;
	}

	log->time = level.time;
	log->fate = fate;
	log->actor = actor && actor->client ? actor->client->pers.namelog : NULL;
	return log;
}

void G_BuildLogSet( buildLog_t *log, gentity_t *ent )
{
	log->buildableTeam = ent->buildableTeam;
	log->modelindex = (buildable_t) ent->s.modelindex;
	log->deconstruct = ent->deconstruct;
	log->deconstructTime = ent->deconstructTime;
	log->builtBy = ent->builtBy;
	log->momentumEarned = ent->momentumEarned;
	VectorCopy( ent->s.pos.trBase, log->origin );
	VectorCopy( ent->s.angles, log->angles );
	VectorCopy( ent->s.origin2, log->origin2 );
	VectorCopy( ent->s.angles2, log->angles2 );
}

void G_BuildLogAuto( gentity_t *actor, gentity_t *buildable, buildFate_t fate )
{
	G_BuildLogSet( G_BuildLogNew( actor, fate ), buildable );
}

void G_BuildLogRevertThink( gentity_t *ent )
{
	gentity_t *built;
	vec3_t    mins, maxs;
	int       blockers[ MAX_GENTITIES ];
	int       num;
	int       victims = 0;
	int       i;

	if ( ent->suicideTime > 0 )
	{
		BG_BuildableBoundingBox( ent->s.modelindex, mins, maxs );
		VectorAdd( ent->s.pos.trBase, mins, mins );
		VectorAdd( ent->s.pos.trBase, maxs, maxs );
		num = trap_EntitiesInBox( mins, maxs, blockers, MAX_GENTITIES );

		for ( i = 0; i < num; i++ )
		{
			gentity_t *targ;
			vec3_t    push;

			targ = g_entities + blockers[ i ];

			if ( targ->client )
			{
				float val = ( targ->client->ps.eFlags & EF_WALLCLIMB ) ? 300.0 : 150.0;

				VectorSet( push, crandom() * val, crandom() * val, random() * val );
				VectorAdd( targ->client->ps.velocity, push, targ->client->ps.velocity );
				victims++;
			}
		}

		ent->suicideTime--;

		if ( victims )
		{
			// still a blocker
			ent->nextthink = level.time + FRAMETIME;
			return;
		}
	}

	built = FinishSpawningBuildable( ent, qtrue );

	if ( ( built->deconstruct = ent->deconstruct ) )
	{
		built->deconstructTime = ent->deconstructTime;
	}

	built->creationTime = built->s.time = 0;
	built->momentumEarned = ent->momentumEarned;
	G_KillBox( built );

	G_LogPrintf( "revert: restore %d %s\n",
	             ( int )( built - g_entities ), BG_Buildable( built->s.modelindex )->name );

	G_FreeEntity( ent );
}

void G_BuildLogRevert( int id )
{
	buildLog_t *log;
	gentity_t  *ent;
	int        team;
	vec3_t     dist;
	gentity_t  *buildable;
	float      momentumChange[ NUM_TEAMS ] = { 0 };

	level.numBuildablesForRemoval = 0;

	level.numBuildLogs -= level.buildId - id;

	while ( level.buildId > id )
	{
		log = &level.buildLog[ --level.buildId % MAX_BUILDLOG ];

		switch ( log->fate )
		{
		case BF_CONSTRUCT:
			for ( team = MAX_CLIENTS; team < level.num_entities; team++ )
			{
				ent = &g_entities[ team ];

				if ( ( ( ent->s.eType == ET_BUILDABLE && ent->health > 0 ) ||
					   ( ent->s.eType == ET_GENERAL && ent->think == G_BuildLogRevertThink ) ) &&
					 ent->s.modelindex == log->modelindex )
				{
					VectorSubtract( ent->s.pos.trBase, log->origin, dist );

					if ( VectorLengthSquared( dist ) <= 2.0f )
					{
						if ( ent->s.eType == ET_BUILDABLE )
						{
							G_LogPrintf( "revert: remove %d %s\n",
										 ( int )( ent - g_entities ), BG_Buildable( ent->s.modelindex )->name );
						}

						// Revert resources
						G_ModifyBuildPoints( ent->buildableTeam, BG_Buildable( ent->s.modelindex )->buildPoints );
						momentumChange[ log->buildableTeam ] -= log->momentumEarned;

						// Free buildable
						G_FreeEntity( ent );

						break;
					}
				}
			}
			break;

		case BF_DECONSTRUCT:
		case BF_REPLACE:
				// Revert resources
				G_ModifyBuildPoints( log->buildableTeam, -BG_Buildable( log->modelindex )->buildPoints );
				momentumChange[ log->buildableTeam ] += log->momentumEarned;

				// Fall through to default

		default:
			// Spawn buildable
			buildable = G_NewEntity();
			VectorCopy( log->origin, buildable->s.pos.trBase );
			VectorCopy( log->angles, buildable->s.angles );
			VectorCopy( log->origin2, buildable->s.origin2 );
			VectorCopy( log->angles2, buildable->s.angles2 );
			buildable->s.modelindex = log->modelindex;
			buildable->deconstruct = log->deconstruct;
			buildable->deconstructTime = log->deconstructTime;
			buildable->builtBy = log->builtBy;
			buildable->momentumEarned = log->momentumEarned;
			buildable->think = G_BuildLogRevertThink;
			buildable->nextthink = level.time + FRAMETIME;
			buildable->suicideTime = 30; // number of thinks before killing players in the way
		}
	}

	for ( team = TEAM_NONE + 1; team < NUM_TEAMS; ++team )
	{
		G_AddMomentumGenericStep( (team_t) team, momentumChange[ team ] );
	}

	G_AddMomentumEnd();
}

/*
================
G_CanAffordBuildPoints

Tests wether a team can afford an amont of build points.
The sign of amount is discarded.
================
*/
qboolean G_CanAffordBuildPoints( team_t team, float amount )
{
	float *bp;

	//TODO write a function to check if a team is a playable one
	if ( TEAM_ALIENS == team || TEAM_HUMANS == team )
	{
		bp = &level.team[ team ].buildPoints;
	}
	else
	{
		return qfalse;
	}

	if ( fabs( amount ) > *bp )
	{
		return qfalse;
	}
	else
	{
		return qtrue;
	}
}

/*
================
G_ModifyBuildPoints

Adds or removes build points from a team.
================
*/
void G_ModifyBuildPoints( team_t team, float amount )
{
	float *bp, newbp;

	if ( team > TEAM_NONE && team < NUM_TEAMS )
	{
		bp = &level.team[ team ].buildPoints;
	}
	else
	{
		return;
	}

	newbp = *bp + amount;

	if ( newbp < 0.0f )
	{
		*bp = 0.0f;
	}
	else
	{
		*bp = newbp;
	}
}

/*
=================
G_GetBuildableValueBP

Calculates the value of buildables (in build points) for both teams.
=================
*/
void G_GetBuildableResourceValue( int *teamValue )
{
	int       entityNum;
	gentity_t *ent;
	int       team;
	const buildableAttributes_t *attr;

	for ( team = TEAM_NONE + 1; team < NUM_TEAMS; team++ )
	{
		teamValue[ team ] = 0;
	}

	for ( entityNum = MAX_CLIENTS; entityNum < level.num_entities; entityNum++ )
	{
		ent = &g_entities[ entityNum ];

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		team = ent->buildableTeam ;
		attr = BG_Buildable( ent->s.modelindex );

		teamValue[ team ] += ( attr->buildPoints * MAX( 0, ent->health ) ) / attr->health;
	}
}
