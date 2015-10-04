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

#include "sg_local.h"
#include "CBSE.h"

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
	float healthFrac;
	int event = 0;

	healthFrac = self->entity->Get<HealthComponent>()->HealthFraction();

	if ( healthFrac < 0.25f && self->lastHealthFrac >= 0.25f )
	{
		event = attr->team == TEAM_ALIENS ? EV_OVERMIND_ATTACK_2 : EV_REACTOR_ATTACK_2;
	}
	else if ( healthFrac < 0.75f && self->lastHealthFrac >= 0.75f )
	{
		event = attr->team == TEAM_ALIENS ? EV_OVERMIND_ATTACK_1 : EV_REACTOR_ATTACK_1;
	}

	if ( event && ( event != self->attackLastEvent || level.time > self->attackTimer ) )
	{
		self->attackTimer = level.time + ATTACKWARN_PRIMARY_PERIOD; // don't spam
		self->attackLastEvent = event;
		G_BroadcastEvent( event, 0, attr->team );
		Beacon::NewArea( BCT_DEFEND, self->s.origin, self->buildableTeam );
	}

	self->lastHealthFrac = healthFrac;
}

/*
================
IsWarnableMOD

True if the means of death allows an under-attack warning.
================
*/

bool G_IsWarnableMOD( int mod )
{
	switch ( mod )
	{
		//case MOD_UNKNOWN:
		case MOD_TRIGGER_HURT:
		case MOD_DECONSTRUCT:
		case MOD_REPLACE:
		case MOD_NOCREEP:
			return false;

		default:
			return true;
	}
}

/*
================
G_SetBuildableAnim

Triggers an animation client side
================
*/
void G_SetBuildableAnim( gentity_t *ent, buildableAnimNumber_t anim, bool force )
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
		BG_ClassBoundingBox( PCL_ALIEN_BUILDER0_UPG, cmins, cmaxs, nullptr, nullptr, nullptr );

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
			frac = acos( -normal[ 2 ] ) / M_PI_4;
		}
		else
		{
			// best case: client spawning right above egg
			minDistance = maxs[ 2 ] - cmins[ 2 ];

			// worst case: bounding boxes meet at their corners
			maxDistance = VectorLength( maxs ) + VectorLength( cmins );

			// the fraction of the angle seperating best (0°) and worst case (45°)
			frac = acos( normal[ 2 ] ) / M_PI_4;
		}

		// the linear interpolation of min & max distance should be an upper boundary for the
		// optimal (closest possible) distance.
		displacement = ( 1.0f - frac ) * minDistance + frac * maxDistance + 1.0f;

		VectorMA( origin, displacement, normal, localOrigin );
	}
	else if ( spawn == BA_H_SPAWN )
	{
		// HACK: Assume naked human is biggest class that can spawn
		BG_ClassBoundingBox( PCL_HUMAN_NAKED, cmins, cmaxs, nullptr, nullptr, nullptr );

		// Since humans always spawn right above the telenode center, this is the optimal
		// (closest possible) client origin.
		VectorCopy( origin, localOrigin );
		localOrigin[ 2 ] += maxs[ 2 ] - cmins[ 2 ] + 1.0f;
	}
	else
	{
		return nullptr;
	}

	trap_Trace( &tr, origin, nullptr, nullptr, localOrigin, spawnNum, MASK_SHOT, 0 );

	if ( tr.entityNum != ENTITYNUM_NONE )
	{
		return &g_entities[ tr.entityNum ];
	}

	trap_Trace( &tr, localOrigin, cmins, cmaxs, localOrigin, ENTITYNUM_NONE, MASK_PLAYERSOLID, 0 );

	if ( tr.entityNum != ENTITYNUM_NONE )
	{
		return &g_entities[ tr.entityNum ];
	}

	if ( spawnOrigin != nullptr )
	{
		VectorCopy( localOrigin, spawnOrigin );
	}

	return nullptr;
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
			G_Kill(blocker, MOD_TRIGGER_HURT);
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

gentity_t *G_Reactor()
{
	static gentity_t *rc;

	// If cache becomes invalid renew it
	if ( !rc || rc->s.eType != ET_BUILDABLE || rc->s.modelindex != BA_H_REACTOR )
	{
		rc = FindBuildable( BA_H_REACTOR );
	}

	return rc;
}

gentity_t *G_AliveReactor()
{
	gentity_t *rc = G_Reactor();

	if ( !rc || G_Dead( rc ) )
	{
		return nullptr;
	}

	return rc;
}

gentity_t *G_ActiveReactor()
{
	gentity_t *rc = G_AliveReactor();

	if ( !rc || !rc->spawned )
	{
		return nullptr;
	}

	return rc;
}

gentity_t *G_Overmind()
{
	static gentity_t *om;

	// If cache becomes invalid renew it
	if ( !om || om->s.eType != ET_BUILDABLE || om->s.modelindex != BA_A_OVERMIND )
	{
		om = FindBuildable( BA_A_OVERMIND );
	}

	return om;
}

gentity_t *G_AliveOvermind()
{
	gentity_t *om = G_Overmind();

	if ( !om || G_Dead( om ) )
	{
		return nullptr;
	}

	return om;
}

gentity_t *G_ActiveOvermind()
{
	gentity_t *om = G_AliveOvermind();

	if ( !om || !om->spawned )
	{
		return nullptr;
	}

	return om;
}

static gentity_t* GetMainBuilding( gentity_t *self, bool ownBase )
{
	team_t    team;
	gentity_t *mainBuilding = nullptr;

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
		return nullptr;
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
float G_DistanceToBase( gentity_t *self, bool ownBase )
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

// TODO: Use base clustering.
bool G_InsideBase( gentity_t *self, bool ownBase )
{
	gentity_t *mainBuilding = GetMainBuilding( self, ownBase );

	if ( !mainBuilding )
	{
		return false;
	}

	if ( Distance( self->s.origin, mainBuilding->s.origin ) >= INSIDE_BASE_MAX_DISTANCE )
	{
		return false;
	}

	return trap_InPVSIgnorePortals( self->s.origin, mainBuilding->s.origin );
}

/*
================
G_FindCreep

attempt to find creep for self, return true if successful
================
*/
bool G_FindCreep( gentity_t *self )
{
	int       i;
	gentity_t *ent;
	gentity_t *closestSpawn = nullptr;
	int       distance = 0;
	int       minDistance = 10000;
	vec3_t    temp_v;

	//don't check for creep if flying through the air
	if ( !self->client && self->s.groundEntityNum == ENTITYNUM_NONE )
	{
		return true;
	}

	//if self does not have a parentNode or its parentNode is invalid find a new one
	if ( self->client || !self->powerSource || !self->powerSource->inuse || G_Dead( self->powerSource ) )
	{
		for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
		{
			if ( ent->s.eType != ET_BUILDABLE )
			{
				continue;
			}

			if ( ( ent->s.modelindex == BA_A_SPAWN || ent->s.modelindex == BA_A_OVERMIND ) &&
			     G_Alive( ent ) )
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

			return true;
		}
		else
		{
			return false;
		}
	}

	if ( self->client )
	{
		return false;
	}

	// if we haven't returned by now then we must already have a valid parent
	return true;
}

/**
 * @brief Simple wrapper to G_FindCreep to check if a location has creep.
 */
static bool IsCreepHere( vec3_t origin )
{
	gentity_t dummy;

	memset( &dummy, 0, sizeof( gentity_t ) );

	dummy.powerSource = nullptr;
	dummy.s.modelindex = BA_NONE;
	VectorCopy( origin, dummy.s.origin );

	return G_FindCreep( &dummy );
}

/*
================
nullDieFunction

hack to prevent compilers complaining about function pointer -> nullptr conversion
================
*/
static void NullDieFunction( gentity_t*, gentity_t*, gentity_t*, int ) {}

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

	if ( !self->spawned )
	{
		return;
	}

	if ( self->s.groundEntityNum != ENTITYNUM_NONE )
	{
		if ( ( ent = G_CheckSpawnPoint( self->s.number, self->s.origin,
										self->s.origin2, BA_A_SPAWN, nullptr ) ) != nullptr )
		{
			// If the thing blocking the spawn is a buildable, kill it.
			// If it's part of the map, kill self.
			if ( ent->s.eType == ET_BUILDABLE )
			{
				G_Kill(ent, (ent->builtBy && ent->builtBy->slot >= 0) ?
				       &g_entities[ent->builtBy->slot] : nullptr, MOD_SUICIDE);

				G_SetBuildableAnim( self, BANIM_SPAWN1, true );
			}
			else if ( ent->s.number == ENTITYNUM_WORLD || ent->s.eType == ET_MOVER )
			{
				G_Kill(ent, MOD_SUICIDE);
				return;
			}
			else if( g_antiSpawnBlock.integer &&
					 ent->client && ent->client->pers.team == TEAM_ALIENS )
			{
				PuntBlocker( self, ent );
			}

			if ( ent->s.eType == ET_CORPSE )
			{
				G_FreeEntity( ent );
			}
		}
		else
		{
			self->spawnBlockTime = 0;
		}
	}
}

static bool AOvermind_TargetValid( gentity_t *self, gentity_t *target )
{
	if (    !target
	     || !target->client
	     || target->client->sess.spectatorState != SPECTATOR_NOT
	     || G_Dead( target )
	     || target->flags & FL_NOTARGET
	     || !trap_InPVS( self->s.origin, target->s.origin )
	     || !G_LineOfSight( self, target, MASK_SOLID, false ) )
	{
		return false;
	}

	return true;
}

static bool AOvermind_IsBetterTarget( gentity_t *self, gentity_t *candidate )
{
	team_t tTeam, cTeam;

	// Any target is better than none.
	if ( !self->target )
	{
		return true;
	}

	// Prefer humans.
	tTeam = G_Team( self->target );
	cTeam = G_Team( candidate );

	if ( tTeam != TEAM_HUMANS && cTeam == TEAM_HUMANS )
	{
		return true;
	}
	else if ( tTeam == TEAM_HUMANS && cTeam != TEAM_HUMANS )
	{
		return false;
	}

	// Prefer closer target.
	if ( Distance( self->s.origin, candidate->s.origin ) <
	     Distance( self->s.origin, self->target->s.origin ) )
	{
		return true;
	}

	return false;
}

static void AOvermind_FindTarget( gentity_t *self )
{
	// Clear old target if invalid.
	if ( !AOvermind_TargetValid( self, self->target ) )
	{
		self->target = nullptr;
	}

	// Search best target.
	gentity_t *ent = nullptr;
	while ( ( ent = G_IterateEntities( ent ) ) )
	{
		if ( AOvermind_TargetValid( self, ent ) && AOvermind_IsBetterTarget( self, ent ) )
		{
			self->target = ent;
		}
	}

	// Copy to entity state so target can be tracked in cgame.
	self->s.otherEntityNum = self->target ? self->target->s.number : ENTITYNUM_NONE;
}

#define OVERMIND_SPAWNS_PERIOD 30000

void AOvermind_Think( gentity_t *self )
{
	int clientNum;

	self->nextthink = level.time + 1000;

	if ( self->spawned && G_Alive( self ) )
	{
		AOvermind_FindTarget( self );

		//do some damage
		if ( G_SelectiveRadiusDamage( self->s.pos.trBase, self, self->splashDamage,
		                              self->splashRadius, self, MOD_OVERMIND, TEAM_ALIENS ) )
		{
			self->timestamp = level.time;
			G_SetBuildableAnim( self, BANIM_ATTACK1, false );
		}

		// just in case an egg finishes building after we tell overmind to stfu
		if ( level.team[ TEAM_ALIENS ].numSpawns > 0 )
		{
			level.overmindMuted = false;
		}

		// shut up during intermission
		if ( level.intermissiontime )
		{
			level.overmindMuted = true;
		}

		//low on spawns
		if ( !level.overmindMuted && level.team[ TEAM_ALIENS ].numSpawns <= 0 &&
		     level.time > self->overmindSpawnsTimer )
		{
			bool  haveBuilder = false;
			gentity_t *builder;

			self->overmindSpawnsTimer = level.time + OVERMIND_SPAWNS_PERIOD;
			G_BroadcastEvent( EV_OVERMIND_SPAWNS, 0, TEAM_ALIENS );

			for ( clientNum = 0; clientNum < level.numConnectedClients; clientNum++ )
			{
				builder = &g_entities[ level.sortedClients[ clientNum ] ];

				if ( G_Alive( builder ) &&
				     ( builder->client->pers.classSelection == PCL_ALIEN_BUILDER0 ||
				       builder->client->pers.classSelection == PCL_ALIEN_BUILDER0_UPG ) )
				{
					haveBuilder = true;
					break;
				}
			}

			// aliens now know they have no eggs, but they're screwed, so stfu
			if ( !haveBuilder )
			{
				level.overmindMuted = true;
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
ABarricade_Shrink

Set shrink state for a barricade. When unshrinking, checks to make sure there
is enough room.
================
*/
void ABarricade_Shrink( gentity_t *self, bool shrink )
{
	if ( !self->spawned || G_Dead( self ) )
	{
		shrink = true;
	}

	if ( shrink && self->shrunkTime )
	{
		int anim;

		// We need to make sure that the animation has been set to shrunk mode
		// because we start out shrunk but with the construct animation when built
		self->shrunkTime = level.time;
		anim = self->s.torsoAnim & ~( ANIM_FORCEBIT | ANIM_TOGGLEBIT );

		if ( self->spawned && G_Alive( self ) && anim != BANIM_IDLE_UNPOWERED )
		{
			G_SetIdleBuildableAnim( self, BANIM_IDLE_UNPOWERED );
			G_SetBuildableAnim( self, BANIM_POWERDOWN, true );
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

		// shrink animation
		if ( self->spawned && G_Alive( self ) )
		{
			G_SetBuildableAnim( self, BANIM_POWERDOWN, true );
			G_SetIdleBuildableAnim( self, BANIM_IDLE_UNPOWERED );
		}
	}
	else
	{
		trace_t tr;
		int     anim;

		trap_Trace( &tr, self->s.origin, self->r.mins, self->r.maxs,
		            self->s.origin, self->s.number, MASK_PLAYERSOLID, 0 );

		if ( tr.startsolid || tr.fraction < 1.0f )
		{
			self->r.maxs[ 2 ] = ( int )( self->r.maxs[ 2 ] * BARRICADE_SHRINKPROP );
			return;
		}

		self->shrunkTime = 0;

		// unshrink animation
		anim = self->s.legsAnim & ~( ANIM_FORCEBIT | ANIM_TOGGLEBIT );

		if ( self->spawned && G_Alive( self ) && anim != BANIM_CONSTRUCT && anim != BANIM_POWERUP )
		{
			G_SetBuildableAnim( self, BANIM_POWERUP, true );
			G_SetIdleBuildableAnim( self, BANIM_IDLE1 );
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
ABarricade_Think

Think function for Alien Barricade
================
*/
void ABarricade_Think( gentity_t *self )
{
	self->nextthink = level.time + 1000;

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

void ABarricade_Touch( gentity_t *self, gentity_t *other, trace_t* )
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

	ABarricade_Shrink( self, true );
}

bool ASpiker_Fire( gentity_t *self )
{
	// check if still resting
	if ( self->spikerRestUntil > level.time )
	{
		return false;
	}

	// play shooting animation
	G_SetBuildableAnim( self, BANIM_ATTACK1, false );
	//G_AddEvent( self, EV_ALIEN_SPIKER, DirToByte( self->s.origin2 ) );

	// calculate total perimeter of all spike rows to allow for a more even spike distribution
	float totalPerimeter = 0.0f;

	for ( int row = 0; row < SPIKER_MISSILEROWS; row++ )
	{
		float altitude = ( ( (float)row + SPIKER_ROWOFFSET ) * M_PI_2 ) / (float)SPIKER_MISSILEROWS;
		float perimeter = 2.0f * M_PI * cos( altitude );

		totalPerimeter += perimeter;
	}

	// distribute and launch missiles
	vec3_t dir, rowBase, zenith, rotAxis;

	VectorCopy( self->s.origin2, zenith );
	PerpendicularVector( rotAxis, zenith );

	for ( int row = 0; row < SPIKER_MISSILEROWS; row++ )
	{
		float altitude = ( ( (float)row + SPIKER_ROWOFFSET ) * M_PI_2 ) / (float)SPIKER_MISSILEROWS;
		float perimeter = 2.0f * M_PI * cos( altitude );

		RotatePointAroundVector( rowBase, rotAxis, zenith, RAD2DEG( M_PI_2 - altitude ) );

		// attempt to distribute spikes with equal distance on all rows
		int spikes = (int)round( ( (float)SPIKER_MISSILES * perimeter ) / totalPerimeter );

		for ( int spike = 0; spike < spikes; spike++ )
		{
			float azimuth = 2.0f * M_PI * ( ( (float)spike + 0.5f * crandom() ) / (float)spikes );
			float altitudeVariance = 0.5f * crandom() * M_PI_2 / (float)SPIKER_MISSILEROWS;

			RotatePointAroundVector( dir, zenith, rowBase, RAD2DEG( azimuth ) );
			RotatePointAroundVector( dir, rotAxis, dir, RAD2DEG( altitudeVariance ) );

			if ( g_debugTurrets.integer )
			{
				Com_Printf( "Spiker #%d fires: Row %d/%d: Spike %2d/%2d: "
				            "( Alt %2.0f°, Az %3.0f° → %.2f, %.2f, %.2f )\n",
				            self->s.number, row + 1, SPIKER_MISSILEROWS, spike + 1, spikes,
				            RAD2DEG( altitude + altitudeVariance ), RAD2DEG( azimuth ),
				            dir[0], dir[1], dir[2] );
			}

			G_SpawnMissile( MIS_SPIKER, self, self->s.origin, dir, nullptr, G_FreeEntity,
			                level.time + (int)( 1000.0f * SPIKER_SPIKE_RANGE /
			                                    (float)BG_Missile( MIS_SPIKER )->speed ) );
		}
	}

	// do radius damage in addition to spike missiles
	//G_SelectiveRadiusDamage( self->s.origin, source, SPIKER_RADIUS_DAMAGE, SPIKER_RANGE, self,
	//                         MOD_SPIKER, TEAM_ALIENS );

	self->spikerRestUntil = level.time + SPIKER_COOLDOWN;
	return true;
}

void ASpiker_Think( gentity_t *self )
{
	gentity_t *ent;
	float     scoring, enemyDamage, friendlyDamage;
	bool  sensing;

	self->nextthink = level.time + 500;

	if ( !self->spawned || !self->powered || G_Dead( self ) )
	{
		return;
	}

	// stop here if recovering from shot
	if ( level.time < self->spikerRestUntil )
	{
		self->spikerLastScoring = 0.0f;
		self->spikerLastSensing = false;

		return;
	}

	// calculate a "scoring" of the situation to decide on the best moment to shoot
	enemyDamage = friendlyDamage = 0.0f; sensing = false;
	for ( ent = nullptr; ( ent = G_IterateEntitiesWithinRadius( ent, self->s.origin,
	                                                         SPIKER_SPIKE_RANGE ) ); )
	{
		float health, durability;

		if ( self == ent || ( ent->flags & FL_NOTARGET ) ) continue;

		HealthComponent *healthComponent = ent->entity->Get<HealthComponent>();

		if (!healthComponent) continue;

		if ( G_OnSameTeam( self, ent ) ) {
			health = healthComponent->Health();
		} else if ( ent->s.eType == ET_BUILDABLE ) {
			// Enemy buildables don't count in the scoring.
			continue;
		} else {
			health = healthComponent->MaxHealth();
		}

		ArmorComponent *armorComponent  = ent->entity->Get<ArmorComponent>();

		if (armorComponent) {
			durability = health / armorComponent->GetNonLocationalDamageMod();
		} else {
			durability = health;
		}

		if ( durability <= 0.0f || !G_LineOfSight( self, ent ) ) continue;

		vec3_t vecToTarget;
		VectorSubtract( ent->s.origin, self->s.origin, vecToTarget );

		// only entities in the spiker's upper hemisphere can be hit
		if ( DotProduct( self->s.origin2, vecToTarget ) < 0 )
		{
			continue;
		}

		// approximate average damage the entity would receive from spikes
		float diameter = VectorLength( ent->r.mins ) + VectorLength( ent->r.maxs );
		float distance = VectorLength( vecToTarget );
		float effectArea = 2.0f * M_PI * distance * distance; // half sphere
		float targetArea = 0.5f * diameter * diameter; // approx. proj. of target on effect area
		float expectedDamage = ( targetArea / effectArea ) * (float)SPIKER_MISSILES *
		                       (float)BG_Missile( MIS_SPIKER )->damage;
		float relativeDamage = expectedDamage / durability;

		if ( G_OnSameTeam( self, ent ) )
		{
			friendlyDamage += relativeDamage;
		}
		else
		{
			if ( distance < SPIKER_SENSE_RANGE )
			{
				sensing = true;
			}
			enemyDamage += relativeDamage;
		}
	}

	// friendly entities that can receive damage substract from the scoring, enemies add to it
	scoring = enemyDamage - friendlyDamage;

	// Shoot if a viable target leaves sense range even if scoring is bad.
	// Guarantees that the spiker always shoots eventually when an enemy gets close enough to it.
	bool senseLost = self->spikerLastScoring > 0.0f && self->spikerLastSensing && !sensing;

	if ( scoring > 0.0f || senseLost )
	{
		if ( g_debugTurrets.integer )
		{
			Com_Printf( "Spiker #%i scoring %.1f - %.1f = %.1f%s%s\n",
			            self->s.number, enemyDamage, friendlyDamage, scoring,
			            sensing ? " (sensing)" : "", senseLost ? " (lost target)" : "" );
		}

		if ( ( scoring <= self->spikerLastScoring && sensing ) || senseLost )
		{
			ASpiker_Fire( self );
		}
		else if ( sensing )
		{
			self->nextthink = level.time; // Maximize sampling rate.
		}
	}

	self->spikerLastScoring = scoring;
	self->spikerLastSensing = sensing;
}

static bool AHive_isBetterTarget(const gentity_t *const self, const gentity_t *candidate)
{

	// Any valid target is better than none
	if ( self->target == nullptr )
	{
		return true;
	}

	// Always prefer target that isn't yet targeted.
	if ( self->target->numTrackedBy == 0 && candidate->numTrackedBy > 0 )
	{
		return false;
	}
	else if ( self->target->numTrackedBy > 0 && candidate->numTrackedBy == 0 )
	{
		return true;
	}

	// Prefer smaller distance.
	int dt = Distance( self->s.origin, self->target->s.origin );
	int dc = Distance( self->s.origin, candidate->s.origin );

	if ( dc < dt )
	{
		return true;
	}
	else if ( dt < dc )
	{
		return false;
	}

	// Tie breaker is random decision, so clients on lower slots don't get shot at more often.
	return rand()%2 ? true : false;
}

bool AHive_TargetValid( gentity_t *self, gentity_t *target, bool ignoreDistance )
{
	trace_t trace;
	vec3_t  tipOrigin;

	if (    !target
	     || !target->client
	     || target->client->pers.team != TEAM_HUMANS
	     || G_Dead( target )
	     || ( target->flags & FL_NOTARGET ) )
	{
		return false;
	}

	// use tip instead of origin for distance and line of sight checks
	// TODO: Evaluate
	VectorMA( self->s.pos.trBase, self->r.maxs[ 2 ], self->s.origin2, tipOrigin );

	// check if enemy in sense range
	if ( !ignoreDistance && Distance( tipOrigin, target->s.origin ) > HIVE_SENSE_RANGE )
	{
		return false;
	}

	// check for clear line of sight
	trap_Trace( &trace, tipOrigin, nullptr, nullptr, target->s.pos.trBase, self->s.number, MASK_SHOT, 0 );

	if ( trace.fraction == 1.0f || trace.entityNum != target->s.number )
	{
		return false;
	}

	return true;
}

/**
 * @brief Forget about old target and attempt to find a new valid one.
 * @return Whether a valid target was found.
 */
static bool AHive_FindTarget( gentity_t *self )
{

	// delete old target
	if ( self->target )
	{
		self->target->numTrackedBy--;
		self->target = nullptr;
	}

	// search best target
	gentity_t *ent = nullptr;
	while ( ( ent = G_IterateEntitiesWithinRadius( ent, self->s.origin, HIVE_SENSE_RANGE ) ) )
	{
		if ( AHive_TargetValid( self, ent, false ) && AHive_isBetterTarget( self, ent ) )
		{
			// change target if I find a valid target that's better than the old one
			self->target = ent;
		}
	}

	// track target
	if ( self->target )
	{
		self->target->numTrackedBy++;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @brief Fires a hive missile on the target, which is assumed to be valid.
 */
void AHive_Fire( gentity_t *self )
{
	vec3_t dirToTarget;

	if ( !self->target )
	{
		return;
	}

	// set self active, this will be reset after the missile dies or a timeout
	self->active = true;
	self->timestamp = level.time + HIVE_REPEAT;

	// set aim angles
	VectorSubtract( self->target->s.pos.trBase, self->s.pos.trBase, dirToTarget );
	VectorNormalize( dirToTarget );
	vectoangles( dirToTarget, self->buildableAim );

	// fire
	G_FireWeapon( self, WP_HIVE, WPM_PRIMARY );
	G_SetBuildableAnim( self, BANIM_ATTACK1, false );
}

void AHive_Think( gentity_t *self )
{
	self->nextthink = level.time + 500;

	// last missile hasn't returned in time, forget about it
	if ( self->timestamp < level.time )
	{
		self->active = false;
	}

	if ( !self->spawned || !self->powered || G_Dead( self ) )
	{
		return;
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

void ABooster_Think( gentity_t *self )
{
	gentity_t *ent;
	bool  playHealingEffect = false;

	self->nextthink = level.time + BOOST_REPEAT_ANIM / 4;

	// check if there is a closeby alien that used this booster for healing recently
	for ( ent = nullptr; ( ent = G_IterateEntitiesWithinRadius( ent, self->s.origin, REGEN_BOOSTER_RANGE ) ); )
	{
		if ( ent->boosterUsed == self && ent->boosterTime == level.previousTime )
		{
			playHealingEffect = true;
			break;
		}
	}

	if ( playHealingEffect )
	{
		if ( level.time > self->timestamp + BOOST_REPEAT_ANIM )
		{
			self->timestamp = level.time;
			G_SetBuildableAnim( self, BANIM_ATTACK1, false );
			G_AddEvent( self, EV_ALIEN_BOOSTER, DirToByte( self->s.origin2 ) );
		}
	}
}

void ABooster_Touch( gentity_t *self, gentity_t *other, trace_t* )
{
	gclient_t *client = other->client;

	if ( !self->spawned || !self->powered || G_Dead( self ) )
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
void ATrapper_FireOnEnemy( gentity_t *self, int firespeed )
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
	G_SetBuildableAnim( self, BANIM_ATTACK1, false );
	self->customNumber = level.time + firespeed;
}

/*
================
ATrapper_CheckTarget

Used by ATrapper_Think to check enemies for validity
================
*/
bool ATrapper_CheckTarget( gentity_t *self, gentity_t *target, int range )
{
	vec3_t  distance;
	trace_t trace;

	if ( !target ) // Do we have a target?
	{
		return false;
	}

	if ( !target->inuse ) // Does the target still exist?
	{
		return false;
	}

	if ( target == self ) // is the target us?
	{
		return false;
	}

	if ( !target->client ) // is the target a bot or player?
	{
		return false;
	}

	if ( target->flags & FL_NOTARGET ) // is the target cheating?
	{
		return false;
	}

	if ( target->client->pers.team == TEAM_ALIENS ) // one of us?
	{
		return false;
	}

	if ( target->client->sess.spectatorState != SPECTATOR_NOT ) // is the target alive?
	{
		return false;
	}

	if ( G_Dead( target ) ) // is the target still alive?
	{
		return false;
	}

	if ( target->client->ps.stats[ STAT_STATE ] & SS_BLOBLOCKED ) // locked?
	{
		return false;
	}

	VectorSubtract( target->r.currentOrigin, self->r.currentOrigin, distance );

	if ( VectorLength( distance ) > range )  // is the target within range?
	{
		return false;
	}

	//only allow a narrow field of "vision"
	VectorNormalize( distance );  //is now direction of target

	if ( DotProduct( distance, self->s.origin2 ) < LOCKBLOB_DOT )
	{
		return false;
	}

	trap_Trace( &trace, self->s.pos.trBase, nullptr, nullptr, target->s.pos.trBase, self->s.number,
	            MASK_SHOT, 0 );

	if ( trace.contents & CONTENTS_SOLID ) // can we see the target?
	{
		return false;
	}

	return true;
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
	ent->target = nullptr;
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

	if ( !self->spawned || !self->powered || G_Dead( self ) )
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
		ATrapper_FireOnEnemy( self, LOCKBLOB_REPEAT );
	}
}

/*
================
IdlePowerState

Set buildable idle animation to match power state
================
*/
static void PlayPowerStateAnims( gentity_t *self )
{
	if ( self->powered && self->s.torsoAnim == BANIM_IDLE_UNPOWERED )
	{
		G_SetBuildableAnim( self, BANIM_POWERUP, false );
		G_SetIdleBuildableAnim( self, BANIM_IDLE1 );
	}
	else if ( !self->powered && self->s.torsoAnim != BANIM_IDLE_UNPOWERED )
	{
		G_SetBuildableAnim( self, BANIM_POWERDOWN, false );
		G_SetIdleBuildableAnim( self, BANIM_IDLE_UNPOWERED );
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
gentity_t *G_NearestPowerSourceInRange( gentity_t *self )
{
	gentity_t *neighbor = G_ActiveReactor();
	gentity_t *best = nullptr;
	float bestDistance = HUGE_QFLT;

	if ( neighbor && Distance( self->s.origin, neighbor->s.origin ) <= g_powerReactorRange.integer )
	{
		return neighbor;
	}

	neighbor = nullptr;

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

	return ( bestDistance <= g_powerRepeaterRange.integer ) ? best : nullptr;
}

/*
=================
IncomingInterference

Calculates the amount of power a buildable recieves from or loses to a neighbor at given distance.
=================
*/
static float IncomingInterference( buildable_t buildable, gentity_t *neighbor,
                                   float distance, bool prediction )
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
				if ( G_Alive( neighbor ) )
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
				if ( G_Alive( neighbor ) )
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
	if ( G_ActiveReactor() != nullptr )
	{
		currentBaseSupply = expectedBaseSupply = g_powerBaseSupply.integer;
	}
	else if ( G_AliveReactor() != nullptr )
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

	neighbor = nullptr;
	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, self->s.origin, PowerRelevantRange() ) ) )
	{
		if ( self == neighbor )
		{
			continue;
		}

		distance = Distance( self->s.origin, neighbor->s.origin );

		self->expectedSparePower += IncomingInterference( (buildable_t) self->s.modelindex, neighbor, distance, true );

		if ( self->spawned )
		{
			self->currentSparePower += IncomingInterference( (buildable_t) self->s.modelindex, neighbor, distance, false );
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
	bool  done;
	float     lowestSparePower;
	gentity_t *ent = nullptr, *lowestSparePowerEnt;

	static int nextCalculation = 0;

	if ( level.time < nextCalculation )
	{
		return;
	}

	// first pass: predict spare power for all buildables,
	//             power up buildables that have enough power
	while ( ( ent = G_IterateEntities( ent, nullptr, false, 0, nullptr ) ) )
	{
		// discard irrelevant entities
		if ( ent->s.eType != ET_BUILDABLE || ent->buildableTeam != TEAM_HUMANS )
		{
			continue;
		}

		CalculateSparePower( ent );

		if ( ent->currentSparePower >= 0.0f )
		{
			ent->powered = true;
		}
	}

	// power down buildables that lack power, highest deficit first
	do
	{
		lowestSparePowerEnt = nullptr;
		lowestSparePower = MAX_QINT;

		// find buildable with highest power deficit
		while ( ( ent = G_IterateEntities( ent, nullptr, false, 0, nullptr ) ) )
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
			lowestSparePowerEnt->powered = false;
			done = false;
		}
		else
		{
			done = true;
		}
	}
	while ( !done );

	nextCalculation = level.time + 500;
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
			                                self->s.origin2, BA_H_SPAWN, nullptr ) ) != nullptr )
			{
				// If the thing blocking the spawn is a buildable, kill it.
				// If it's part of the map, kill self.
				if ( ent->s.eType == ET_BUILDABLE )
				{
					G_Kill(ent, MOD_SUICIDE);
					G_SetBuildableAnim( self, BANIM_SPAWN1, true );
				}
				else if ( ent->s.number == ENTITYNUM_WORLD || ent->s.eType == ET_MOVER )
				{
					G_Kill(self, MOD_SUICIDE);
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

	PlayPowerStateAnims( self );
}

void HReactor_Think( gentity_t *self )
{
	gentity_t *ent, *trail;
	bool  fire = false;

	self->nextthink = level.time + REACTOR_ATTACK_REPEAT;

	if ( !self->spawned || G_Dead( self ) )
	{
		return;
	}

	// create a tesla trail for every target
	for ( ent = nullptr; ( ent = G_IterateEntitiesWithinRadius( ent, self->s.pos.trBase, REACTOR_ATTACK_RANGE ) ); )
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

		fire = true;
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

void HArmoury_Use( gentity_t *self, gentity_t*, gentity_t *activator )
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

void HMedistat_Think( gentity_t *self )
{
	int       entityList[ MAX_GENTITIES ];
	vec3_t    mins, maxs;
	int       playerNum, numPlayers;
	gentity_t *player;
	gclient_t *client;
	bool  occupied;

	self->nextthink = level.time + 100;

	if ( !self->spawned )
	{
		return;
	}

	PlayPowerStateAnims( self );

	// clear target's healing flag for now
	if ( self->target && self->target->client )
	{
		self->target->client->ps.stats[ STAT_STATE ] &= ~SS_HEALING_2X;
	}

	// clear target on power loss
	if ( !self->powered )
	{
		self->active = false;
		self->target = nullptr;

		self->nextthink = level.time + POWER_REFRESH_TIME;

		return;
	}

	// get entities standing on top
	VectorAdd( self->s.origin, self->r.maxs, maxs );
	VectorAdd( self->s.origin, self->r.mins, mins );

	mins[ 2 ] += ( self->r.mins[ 2 ] + self->r.maxs[ 2 ] );
	maxs[ 2 ] += 32; // continue to heal jumping players but don't heal jetpack campers

	numPlayers = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
	occupied = false;

	// mark occupied if still healing a player
	for ( playerNum = 0; playerNum < numPlayers; playerNum++ )
	{
		player = &g_entities[ entityList[ playerNum ] ];
		client = player->client;

		if ( self->target == player && PM_Live( client->ps.pm_type ) &&
			 ( !player->entity->Get<HealthComponent>()->FullHealth() ||
		       client->ps.stats[ STAT_STAMINA ] < STAMINA_MAX ) )
		{
			occupied = true;
		}
	}

	if ( !occupied )
	{
		self->target = nullptr;
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

		HealthComponent *healthComponent = player->entity->Get<HealthComponent>();

		// give medikit to players with full health
		if ( healthComponent->FullHealth() )
		{
			if ( !BG_InventoryContainsUpgrade( UP_MEDKIT, player->client->ps.stats ) )
			{
				BG_AddUpgradeToInventory( UP_MEDKIT, player->client->ps.stats );
			}
		}

		// if not already occupied, check if someone needs healing
		if ( !occupied )
		{
			if ( PM_Live( client->ps.pm_type ) &&
				 ( !healthComponent->FullHealth() ||
				   client->ps.stats[ STAT_STAMINA ] < STAMINA_MAX ) )
			{
				self->target = player;
				occupied = true;
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
			self->active = true;

			G_SetBuildableAnim( self, BANIM_ATTACK1, false );
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
		player->entity->Heal(1.0f, nullptr);

		// check if fully healed
		if ( player->entity->Get<HealthComponent>()->FullHealth() )
		{
			// give medikit
			if ( !BG_InventoryContainsUpgrade( UP_MEDKIT, client->ps.stats ) )
			{
				BG_AddUpgradeToInventory( UP_MEDKIT, client->ps.stats );
			}
		}
	}
	// we lost our target
	else if ( self->active )
	{
		self->active = false;

		// stop healing animation
		G_SetBuildableAnim( self, BANIM_ATTACK2, true );
		G_SetIdleBuildableAnim( self, BANIM_IDLE1 );
	}
}

static int HMGTurret_DistanceToZone( float distance )
{
	const float zoneWidth = ( float )MGTURRET_RANGE / ( float )MGTURRET_ZONES;

	int zone = ( int )( distance / zoneWidth );

	if ( zone >= MGTURRET_ZONES )
	{
		return MGTURRET_ZONES - 1;
	}
	else
	{
		return zone;
	}
}

static bool HMGTurret_IsBetterTarget( gentity_t *self, gentity_t *candidate )
{
	// Any target is better than none
	if ( self->target == nullptr)
	{
		return true;
	}

	// Prefer target that isn't yet targeted.
	// This makes group attacks more and dretch spam less efficient.
	if ( self->target->numTrackedBy == 0 && candidate->numTrackedBy > 0 )
	{
		return false;
	}
	else if ( self->target->numTrackedBy > 0 && candidate->numTrackedBy == 0 )
	{
		return true;
	}

	// Prefer the target that is in a line of sight.
	// This prevents the turret from keeping a lock on a target behind cover.
	// (For the MG turret, do so after the already-targeted check so that such a lock is kept if all
	// other valid targets are being dealt with. This results in suppressive fire against targets
	// behind cover as long as it's safe for the turret to do so.)
	bool los2t = G_LineOfFire( self, self->target );
	bool los2c = G_LineOfFire( self, candidate );

	if ( los2t && !los2c )
	{
		return false;
	}
	else if ( los2c && !los2t )
	{
		return true;
	}

	// Prefer target that can be aimed at more quickly.
	// This makes the turret spend less time tracking enemies.
	vec3_t aimDir, dir_t, dir_c;

	AngleVectors( self->buildableAim, aimDir, nullptr, nullptr );

	VectorSubtract( self->target->s.origin, self->s.pos.trBase, dir_t ); VectorNormalize( dir_t );
	VectorSubtract( candidate->s.origin, self->s.pos.trBase, dir_c ); VectorNormalize( dir_c );

	if ( DotProduct( dir_t, aimDir ) > DotProduct( dir_c, aimDir ) )
	{
		return false;
	}
	else
	{
		return true;
	}
}

static bool HRocketpod_IsBetterTarget( gentity_t *self, gentity_t *candidate )
{
	// Any target is better than none
	if ( self->target == nullptr)
	{
		return true;
	}

	// Prefer the target that is in a line of sight.
	// This prevents the turret from keeping a lock on a target behind cover.
	bool los2t = G_LineOfFire( self, self->target );
	bool los2c = G_LineOfFire( self, candidate );

	if ( los2t && !los2c )
	{
		return false;
	}
	else if ( los2c && !los2t )
	{
		return true;
	}

	// First, prefer target that is currently safe to shoot at.
	// Then, prefer target that can be aimed at more quickly.
	vec3_t aimDir, dir_t, dir_c;
	bool   safe_t, safe_c;

	VectorSubtract( self->target->s.origin, self->s.pos.trBase, dir_t ); VectorNormalize( dir_t );
	VectorSubtract( candidate->s.origin, self->s.pos.trBase, dir_c ); VectorNormalize( dir_c );

	safe_t = G_RocketpodSafeShot( self->s.number, self->s.pos.trBase, dir_t );
	safe_c = G_RocketpodSafeShot( self->s.number, self->s.pos.trBase, dir_c );

	if ( safe_t && !safe_c )
	{
		return false;
	}
	else if ( safe_c && !safe_t )
	{
		return true;
	}

	AngleVectors( self->buildableAim, aimDir, nullptr, nullptr );

	if ( DotProduct( dir_t, aimDir ) > DotProduct( dir_c, aimDir ) )
	{
		return false;
	}
	else
	{
		return true;
	}
}

static void HTurret_RemoveTarget( gentity_t *self )
{
	if ( self->target )
	{
		self->target->numTrackedBy--;
		self->target = nullptr;
	}

	self->turretLastLOSToTarget = 0;
}

static bool HTurret_TargetValid( gentity_t *self, gentity_t *target, bool newTarget,
                                     float range )
{
	if (    !target
	     || !target->client
	     || target->client->sess.spectatorState != SPECTATOR_NOT
	     || G_Dead( target )
	     || target->flags & FL_NOTARGET
	     || G_OnSameTeam( self, target )
	     || Distance( self->s.origin, target->s.origin ) > range
	     || !trap_InPVS( self->s.origin, target->s.origin ) )
	{
		if ( g_debugTurrets.integer > 0 && self->target )
		{
			Com_Printf( "Turret %d: Target %d lost: Eliminated or out of range.\n",
			            self->s.number, self->target->s.number );
		}

		return false;
	}

	// check for LOS, don't accept new targets if there is none
	if ( G_LineOfFire( self, target ) )
	{
		self->turretLastLOSToTarget = level.time;
	}
	else if ( newTarget )
	{
		return false;
	}

	// give up on existing target if there hasn't been a clear LOS for a while
	if ( self->turretLastLOSToTarget + TURRET_GIVEUP_TARGET < level.time )
	{
		if ( g_debugTurrets.integer > 0 && self->target )
		{
			Com_Printf( "Turret %d: Target %d lost: No clear LOS for %d ms.\n",
						self->s.number, self->target->s.number, TURRET_GIVEUP_TARGET );
		}

		return false;
	}

	self->turretLastValidTargetTime = level.time;

	return true;
}

static bool HTurret_FindTarget( gentity_t *self, float range,
                                    bool (*isBetterTarget)(gentity_t* , gentity_t* ) )
{

	self->turretLastTargetSearch = level.time;

	// delete old target
	HTurret_RemoveTarget( self );

	// search best target
	gentity_t *ent = nullptr;
	while ( ( ent = G_IterateEntitiesWithinRadius( ent, self->s.origin, range ) ) )
	{
		if ( HTurret_TargetValid( self, ent, true, range ) && isBetterTarget( self, ent ) )
		{
			self->target = ent;
		}
	}

	if ( self->target )
	{
		self->target->numTrackedBy++;
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @brief If the turret's head should rest this needs to be called so the move timer stays valid.
 */
static void HTurret_PauseHead( gentity_t *self )
{
	self->turretLastHeadMove = level.time;
}

/**
 * @return Whether another movement action will be needed afterwards.
 */
static bool HTurret_MoveHeadToTarget( gentity_t *self )
{
	const static vec3_t upwards = { 0.0f, 0.0f, 1.0f };

	vec3_t   rotAxis, angleToTarget, relativeDirToTarget, deltaAngles, relativeNewDir, newDir,
	         maxAngles;
	float    rotAngle, relativePitch, timeMod;
	int      elapsed, currentAngle;
	bool anotherMoveNeeded;

	// calculate maximum angular movement for this execution
	if ( !self->turretLastHeadMove )
	{
		// no information yet, start moving next time
		self->turretLastHeadMove = level.time;
		return true;
	}

	elapsed = level.time - self->turretLastHeadMove;

	if ( elapsed <= 0 )
	{
		return true;
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

	anotherMoveNeeded = false;

	// adjust pitch and yaw
	for ( currentAngle = 0; currentAngle < 3; currentAngle++ )
	{
		if ( currentAngle == ROLL )
		{
			continue;
		}

		// get angle delta from current orientation towards trarget angle
		deltaAngles[ currentAngle ] = AngleSubtract( self->s.angles2[ currentAngle ],
		                                             angleToTarget[ currentAngle ] );

		// adjust enttiyState_t.angles2
		if      ( deltaAngles[ currentAngle ] < 0.0f &&
		          deltaAngles[ currentAngle ] < -maxAngles[ currentAngle ] )
		{
			self->s.angles2[ currentAngle ] += maxAngles[ currentAngle ];
			anotherMoveNeeded = true;
		}
		else if ( deltaAngles[ currentAngle ] > 0.0f &&
		          deltaAngles[ currentAngle ] >  maxAngles[ currentAngle ] )
		{
			self->s.angles2[ currentAngle ] -= maxAngles[ currentAngle ];
			anotherMoveNeeded = true;
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
	AngleVectors( self->s.angles2, relativeNewDir, nullptr, nullptr ); //
	RotatePointAroundVector( newDir, rotAxis, relativeNewDir, -rotAngle );
	vectoangles( newDir, self->buildableAim );

	self->turretLastHeadMove = level.time;

	return anotherMoveNeeded;
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
	AngleVectors( self->s.angles, dir, nullptr, nullptr );
	VectorNormalize( dir );

	// invert base direction if it reaches into a wall within the first damage zone
	VectorMA( self->s.origin, ( float )MGTURRET_RANGE / ( float )MGTURRET_ZONES, dir, end );
	trap_Trace( &tr, self->s.pos.trBase, nullptr, nullptr, end, self->s.number, MASK_SHOT, 0 );

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
	HTurret_SetBaseDir( self );

	VectorCopy( self->turretBaseDir, self->turretDirToTarget );
}

static void HTurret_ResetPitch( gentity_t *self )
{
	vec3_t angles;

	VectorCopy( self->s.angles2, angles );

	angles[ PITCH ] = 0.0f;

	AngleVectors( angles, self->turretDirToTarget, nullptr, nullptr );

	VectorNormalize( self->turretDirToTarget );
}

static void HTurret_LowerPitch( gentity_t *self )
{
	vec3_t angles;

	VectorCopy( self->s.angles2, angles );

	angles[ PITCH ] = TURRET_PITCH_CAP;

	AngleVectors( angles, self->turretDirToTarget, nullptr, nullptr );

	VectorNormalize( self->turretDirToTarget );
}

void HTurret_PreBlast( gentity_t *self )
{
	HTurret_LowerPitch( self );

	// Enter regular blast state as soon as the barrel is lowered.
	if ( !HTurret_MoveHeadToTarget( self ) )
	{
		//self->think = HGeneric_Blast;
		self->nextthink = level.time + HUMAN_DETONATION_RAND_DELAY;
	}
	else
	{
		self->nextthink = level.time + TURRET_THINK_PERIOD;
	}
}

static bool HTurret_TargetInReach( gentity_t *self, float range )
{
	trace_t tr;
	vec3_t  forward, end;

	if ( !self->target )
	{
		return false;
	}

	// check if a shot would hit the target
	AngleVectors( self->buildableAim, forward, nullptr, nullptr );
	VectorMA( self->s.pos.trBase, range, forward, end );
	trap_Trace( &tr, self->s.pos.trBase, nullptr, nullptr, end, self->s.number, MASK_SHOT, 0 );

	return ( tr.entityNum == ( self->target - g_entities ) );
}

static void HMGTurret_Shoot( gentity_t *self )
{
	const int zoneDamage[] = MGTURRET_ZONE_DAMAGE;
	int       zone;

	self->s.eFlags |= EF_FIRING;

	zone = HMGTurret_DistanceToZone( Distance( self->s.pos.trBase, self->target->s.pos.trBase ) );
	self->turretCurrentDamage = zoneDamage[ zone ];

	if ( g_debugTurrets.integer > 1 )
	{
		const static char color[] = {'1', '8', '3', '2'};
		Com_Printf( "MGTurret %d: Shooting at %d: ^%cZone %d/%d → %d damage\n",
		            self->s.number, self->target->s.number, color[zone], zone + 1,
		            MGTURRET_ZONES, self->turretCurrentDamage );
	}

	G_AddEvent( self, EV_FIRE_WEAPON, 0 );
	G_FireWeapon( self, WP_MGTURRET, WPM_PRIMARY );

	self->turretNextShot = level.time + MGTURRET_ATTACK_PERIOD;
}

/**
 * @brief Adjusts the pitch in addition to regular power animations.
 * @return Whether the structure is powered.
 */
static bool HMGTurret_PowerHelper( gentity_t *self )
{
	if ( !self->powered )
	{
		if ( !self->turretDisabled )
		{
			self->turretDisabled = true;

			// Forget about a target.
			HTurret_RemoveTarget( self );

			// Lower head.
			HTurret_LowerPitch( self );
		}
	}
	else if ( self->turretDisabled )
	{
		self->turretDisabled = false;

		// Raise head.
		HTurret_ResetPitch( self );
	}

	return self->powered;
}

void HMGTurret_Think( gentity_t *self )
{
	self->nextthink = level.time + TURRET_THINK_PERIOD;

	// Disable muzzle flash for now.
	self->s.eFlags &= ~EF_FIRING;

	if ( !self->spawned )
	{
		return;
	}

	// If powered down, move head to allow for pitch adjustments.
	if ( !HMGTurret_PowerHelper( self ) )
	{
		HTurret_MoveHeadToTarget( self );
		return;
	}

	// Remove existing target if it became invalid.
	if ( !HTurret_TargetValid( self, self->target, false, MGTURRET_RANGE ) )
	{
		HTurret_RemoveTarget( self );
	}

	// Try to find a new target if there is none.
	if ( !self->target && level.time - self->turretLastTargetSearch > TURRET_SEARCH_PERIOD )
	{
		if ( HTurret_FindTarget( self, MGTURRET_RANGE, HMGTurret_IsBetterTarget ) &&
		     g_debugTurrets.integer > 0 )
		{
			Com_Printf( "MGTurret %d: New target %d.\n", self->s.number, self->target->s.number );
		}
	}

	if ( self->target )
	{
		// Track target.
		HTurret_TrackTarget( self );
	}
	else if ( !self->turretLastValidTargetTime )
	{
		// Reset direction once after spawning.
		HTurret_ResetDirection( self );
	}

	// Shoot if target in reach.
	if ( HTurret_TargetInReach( self, MGTURRET_RANGE ) )
	{
		// If the target's origin is visible, aim for it first, otherwise pause head.
		if ( G_LineOfFire( self, self->target ) )
		{
			HTurret_MoveHeadToTarget( self );
		}
		else
		{
			HTurret_PauseHead( self );
		}

		if ( self->turretNextShot < level.time )
		{
			HMGTurret_Shoot( self );
		}
		else
		{
			// Keep the flag enabled in between shots.
			self->s.eFlags |= EF_FIRING;
		}
	}
	else
	{
		// Target not in reach, move head towards it.
		HTurret_MoveHeadToTarget( self );
	}
}

static void HRocketpod_Shoot( gentity_t *self )
{
	self->s.eFlags |= EF_FIRING;

	if ( g_debugTurrets.integer > 1 )
	{
		Com_Printf( "Rocketpod %d: Shooting at %d\n", self->s.number, self->target->s.number );
	}

	G_AddEvent( self, EV_FIRE_WEAPON, 0 );
	G_FireWeapon( self, WP_ROCKETPOD, WPM_PRIMARY );

	self->turretNextShot = level.time + ROCKETPOD_ATTACK_PERIOD;
}

/**
 * @return Whether the mode was changed.
 */
static bool HRocketpod_SafeMode( gentity_t *self, bool enable )
{
	if ( enable && !self->turretSafeMode )
	{
		self->turretSafeMode = true;

		HTurret_RemoveTarget( self );
		HTurret_ResetPitch( self );

		G_SetBuildableAnim( self, BANIM_POWERDOWN, true );
		G_SetIdleBuildableAnim( self, BANIM_IDLE_UNPOWERED );

		return true;
	}
	else if ( !enable && self->turretSafeMode )
	{
		self->turretSafeMode = false;

		G_SetBuildableAnim( self, BANIM_POWERUP, true );
		G_SetIdleBuildableAnim( self, BANIM_IDLE1 );

		// Can't shoot for another second as the shutters open.
		self->turretPrepareTime = level.time + 1000;

		return true;
	}

	return false;
}

/**
 * @brief Adjusts the pitch in addition to regular power animations.
 * @return Whether the structure is powered.
 */
static bool HRocketpod_PowerHelper( gentity_t *self )
{
	if ( !self->powered )
	{
		if ( !self->turretDisabled )
		{
			self->turretDisabled = true;

			// Forget about a target.
			HTurret_RemoveTarget( self );

			// Lower head.
			HTurret_LowerPitch( self );

			// Close shutters.
			PlayPowerStateAnims( self );
		}
	}
	else if ( self->turretDisabled )
	{
		self->turretDisabled = false;

		// Raise head.
		HTurret_ResetPitch( self );

		// Open shutters.
		PlayPowerStateAnims( self );

		// Can't shoot for another second as the shutters open.
		self->turretPrepareTime = level.time + 1000;
	}

	return self->powered;
}

static bool HRocketPod_EnemyClose( gentity_t *self )
{
	gentity_t *neighbour;

	// Let the safe mode cache the return value of this function.
	if ( level.time - self->turretSafeModeCheckTime < TURRET_SEARCH_PERIOD )
	{
		return self->turretSafeMode;
	}

	self->turretSafeModeCheckTime = level.time;

	for ( neighbour = nullptr; ( neighbour = G_IterateEntitiesWithinRadius( neighbour, self->s.origin,
	                                                                     ROCKETPOD_RANGE ) ); )
	{
		if ( neighbour->client && G_Alive( neighbour ) && !G_OnSameTeam( self, neighbour ) &&
		     neighbour->client->sess.spectatorState == SPECTATOR_NOT )
		{
			classModelConfig_t *cmc = BG_ClassModelConfig( neighbour->client->pers.classSelection );
			float classRadius = 0.5f * ( VectorLength( cmc->mins ) + VectorLength( cmc->maxs ) );

			if ( Distance( self->s.origin, neighbour->s.origin ) - classRadius
			     < BG_Missile( MIS_ROCKET )->splashRadius )
			{
				return true;
			}
		}
	}

	return false;
}

void HRocketpod_Think( gentity_t *self )
{
	self->nextthink = level.time + TURRET_THINK_PERIOD;

	// Disable muzzle flash and lockon sound for now.
	self->s.eFlags &= ~EF_FIRING;
	self->s.eFlags &= ~EF_B_LOCKON;

	if ( !self->spawned )
	{
		return;
	}

	// If powered down, move head to allow for pitch adjustments.
	if ( !HRocketpod_PowerHelper( self ) )
	{
		HTurret_MoveHeadToTarget( self );
		return;
	}

	// If there's an enemy really close, enter safe mode.
	HRocketpod_SafeMode( self, HRocketPod_EnemyClose( self ) );

	// If in safe mode, move head to allow for pitch adjustments.
	if ( self->turretSafeMode )
	{
		HTurret_MoveHeadToTarget( self );
		return;
	}

	// Don't do anything while opening shutters.
	if ( self->turretPrepareTime > level.time )
	{
		HTurret_PauseHead( self );
		return;
	}

	// Remove existing target if it became invalid.
	if ( !HTurret_TargetValid( self, self->target, false, ROCKETPOD_RANGE ) )
	{
		HTurret_RemoveTarget( self );

		// Delay the first shot on a new target.
		self->turretLockonTime = level.time + ROCKETPOD_LOCKON_TIME;
	}

	// Try to find a new target if there is none.
	if ( !self->target && level.time - self->turretLastTargetSearch > TURRET_SEARCH_PERIOD )
	{
		if ( HTurret_FindTarget( self, ROCKETPOD_RANGE, HRocketpod_IsBetterTarget ) &&
		     g_debugTurrets.integer > 0 )
		{
			Com_Printf( "Rocketpod %d: New target %d.\n", self->s.number, self->target->s.number );
		}
	}

	if ( self->target )
	{
		// Track target.
		HTurret_TrackTarget( self );
	}
	else if ( !self->turretLastValidTargetTime )
	{
		// Reset direction once after spawning.
		HTurret_ResetDirection( self );
	}

	// Shoot if target in reach and safe.
	if ( HTurret_TargetInReach( self, ROCKETPOD_RANGE ) )
	{
		// If the target's origin is visible, aim for it first, otherwise pause head.
		if ( G_LineOfFire( self, self->target ) )
		{
			HTurret_MoveHeadToTarget( self );
		}
		else
		{
			HTurret_PauseHead( self );
		}

		if ( level.time < self->turretLockonTime )
		{
			// Play lockon sound.
			self->s.eFlags |= EF_B_LOCKON;
		}
		else
		{
			vec3_t aimDir;

			AngleVectors( self->buildableAim, aimDir, nullptr, nullptr );

			// While it's safe to shoot, do so.
			if ( G_RocketpodSafeShot( self->s.number, self->s.pos.trBase, aimDir ) )
			{
				if ( self->turretNextShot < level.time )
				{
					HRocketpod_Shoot( self );
				}
				else
				{
					// Keep the flag enabled in between shots.
					self->s.eFlags |= EF_FIRING;
				}
			}
			else
			{
				// Delay the first shot after the situation became safe to shoot.
				self->turretLockonTime = level.time + ROCKETPOD_LOCKON_TIME;
			}
		}
	}
	else
	{
		// Target not in reach, move head towards it.
		if ( HTurret_MoveHeadToTarget( self ) )
		{
			// Delay the first shot after significant tracking.
			self->turretLockonTime = level.time + ROCKETPOD_LOCKON_TIME;
		}

		// Delay the first shot after the target left its cover.
		/*self->turretLockonTime = std::max( self->turretLockonTime,
			                               level.time + ROCKETPOD_LOCKON_TIME );*/
	}
}

void HDrill_Think( gentity_t *self )
{
	self->nextthink = level.time + 1000;

	PlayPowerStateAnims( self );
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

	// dead buildables don't activate triggers
	if ( G_Dead( ent ) )
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
G_BuildableRange

Check whether a point is within some range of a type of buildable
===============
*/
bool G_BuildableInRange( vec3_t origin, float radius, buildable_t buildable )
{
	gentity_t *neighbor = nullptr;

	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, origin, radius ) ) )
	{
		if ( neighbor->s.eType != ET_BUILDABLE || !neighbor->spawned || G_Dead( neighbor ) ||
		     ( neighbor->buildableTeam == TEAM_HUMANS && !neighbor->powered ) )
		{
			continue;
		}

		if ( neighbor->s.modelindex == buildable )
		{
			return true;
		}
	}

	return false;
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

	return nullptr;
}

/*
===============
G_BuildablesIntersect

Test if two buildables intersect each other
===============
*/
static bool BuildablesIntersect( buildable_t a, vec3_t originA,
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
		BA_H_ROCKETPOD,
		BA_H_MEDISTAT,
		BA_H_ARMOURY,
		BA_H_SPAWN,
		BA_H_REPEATER,
		BA_H_REACTOR
	};

	gentity_t *buildableA, *buildableB;
	int       aPrecedence = 0, bPrecedence = 0;
	bool  aMatches = false, bMatches = false;

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
		return Math::Clamp(buildableA->entity->Get<BuildableComponent>()->GetMarkTime() -
		                   buildableB->entity->Get<BuildableComponent>()->GetMarkTime(), -1, 1);
	}

	// Resort to preference list
	for ( unsigned i = 0; i < ARRAY_LEN( precedence ); i++ )
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
	if ( !self || self->s.eType != ET_BUILDABLE )
	{
		return;
	}

	const buildableAttributes_t *attr = BG_Buildable( self->s.modelindex );

	// save remaining health fraction
	self->deconHealthFrac = self->entity->Get<HealthComponent>()->HealthFraction();

	// return BP
	int refund = attr->buildPoints * self->deconHealthFrac;
	G_ModifyBuildPoints( self->buildableTeam, refund );

	// remove momentum
	G_RemoveMomentumForDecon( self, deconner );

	// reward attackers if the structure was hurt before deconstruction
	if ( self->deconHealthFrac < 1.0f ) G_RewardAttackers( self );

	// deconstruct
	G_Kill(self, deconner, deconType);

	// TODO: Check if freeing needs to be deferred.
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

static bool IsSetForDeconstruction( gentity_t *ent )
{
	int markedNum;

	for ( markedNum = 0; markedNum < level.numBuildablesForRemoval; markedNum++ )
	{
		if ( level.markedBuildables[ markedNum ] == ent )
		{
			return true;
		}
	}

	return false;
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
static bool PredictBuildablePower( buildable_t buildable, vec3_t origin )
{
	gentity_t       *neighbor, *buddy;
	float           distance, ownPrediction, neighborPrediction;
	int             powerConsumption, baseSupply;

	powerConsumption = BG_Buildable( buildable )->powerConsumption;

	if ( !powerConsumption )
	{
		return true;
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

	neighbor = nullptr;
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

		ownPrediction += IncomingInterference( buildable, neighbor, distance, true );
		neighborPrediction = neighbor->expectedSparePower + OutgoingInterference( buildable, neighbor, distance );

		// check power of neighbor, with regards to pending deconstruction
		if ( neighborPrediction < 0.0f && distance < g_powerCompetitionRange.integer )
		{
			buddy = nullptr;
			while ( ( buddy = G_IterateEntitiesWithinRadius( buddy, neighbor->s.origin, PowerRelevantRange() ) ) )
			{
				if ( IsSetForDeconstruction( buddy ) )
				{
					distance = Distance( neighbor->s.origin, buddy->s.origin );
					neighborPrediction -= IncomingInterference( (buildable_t) neighbor->s.modelindex, buddy, distance, true );
				}
			}

			if ( neighborPrediction < 0.0f )
			{
				return false;
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
			if ( ent->entity->Get<BuildableComponent>()->MarkedForDeconstruction() )
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
			if ( ent->entity->Get<BuildableComponent>()->MarkedForDeconstruction() )
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

	// TODO: Once ForEntities allows break semantics, rewrite.
	itemBuildError_t collisionError = IBE_NONE;
	ForEntities<BuildableComponent>([&] (Entity& entity, BuildableComponent& buildableComponent) {
		// HACK: Fake a break.
		if (collisionError != IBE_NONE) return;

		buildable_t otherBuildable = (buildable_t)entity.oldEnt->s.modelindex;
		team_t      otherTeam      = entity.oldEnt->buildableTeam;

		if (BuildablesIntersect(buildable, origin, otherBuildable, entity.oldEnt->s.origin)) {
			if (otherTeam != attr->team) {
				collisionError = IBE_NOROOM;
				return;
			}

			if (!buildableComponent.MarkedForDeconstruction()) {
				collisionError = IBE_NOROOM;
				return;
			}

			// Ignore main buildable replacement since it will already be on the list.
			if (!(BG_IsMainStructure(buildable) && BG_IsMainStructure(otherBuildable))) {
				// Apply general replacement rules.
				if ((collisionError = BuildableReplacementChecks((buildable_t)entity.oldEnt->s.modelindex, buildable)) != IBE_NONE) {
					return;
				}

				level.markedBuildables[level.numBuildablesForRemoval++] = entity.oldEnt;
			}
		}
	});
	if (collisionError != IBE_NONE) return collisionError;

	// ---------------
	// check for power
	// ---------------

	if ( attr->team == TEAM_HUMANS )
	{
		numNeighbors = 0;
		neighbor = nullptr;

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
				if ( !neighbor->entity->Get<BuildableComponent>()->MarkedForDeconstruction() )
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

		cost -= entAttr->buildPoints * ent->entity->Get<HealthComponent>()->HealthFraction();
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
		if ( !ent->entity->Get<BuildableComponent>()->MarkedForDeconstruction() )
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

		cost -= entAttr->buildPoints * ent->entity->Get<HealthComponent>()->HealthFraction();
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
static void SetBuildableLinkState( bool link )
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

static void SetBuildableMarkedLinkState( bool link )
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
itemBuildError_t G_CanBuild( gentity_t *ent, buildable_t buildable, int /*distance*/, //TODO
                             vec3_t origin, vec3_t normal, int *groundEntNum )
{
	vec3_t           angles;
	vec3_t           entity_origin;
	vec3_t           mins, maxs;
	trace_t          tr1, tr2, tr3;
	itemBuildError_t reason = IBE_NONE, tempReason;
	gentity_t        *tempent;
	float            minNormal;
	bool         invert;
	int              contents;
	playerState_t    *ps = &ent->client->ps;

	// Stop all buildables from interacting with traces
	SetBuildableLinkState( false );

	BG_BuildableBoundingBox( buildable, mins, maxs );

	BG_PositionBuildableRelativeToPlayer( ps, mins, maxs, trap_Trace, entity_origin, angles, &tr1 );
	trap_Trace( &tr2, entity_origin, mins, maxs, entity_origin, ENTITYNUM_NONE, MASK_PLAYERSOLID, 0 );
	trap_Trace( &tr3, ps->origin, nullptr, nullptr, entity_origin, ent->s.number, MASK_PLAYERSOLID, 0 );

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

		if ( tempent && !tempent->entity->Get<BuildableComponent>()->MarkedForDeconstruction() )
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
	SetBuildableLinkState( true );

	//check there is enough room to spawn from (presuming this is a spawn)
	if ( reason == IBE_NONE )
	{
		SetBuildableMarkedLinkState( false );

		if ( G_CheckSpawnPoint( ENTITYNUM_NONE, origin, normal, buildable, nullptr ) != nullptr )
		{
			reason = IBE_NORMAL;
		}

		SetBuildableMarkedLinkState( true );
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

/** Sets shared buildable entity parameters. */
#define BUILDABLE_ENTITY_SET_PARAMS(params)\
	params.oldEnt = ent;\
	params.Health_maxHealth = BG_Buildable(buildable)->health;

/** Creates basic buildable entity of specific type. */
#define BUILDABLE_ENTITY_CREATE(entityType)\
	entityType::Params params;\
	BUILDABLE_ENTITY_SET_PARAMS(params);\
	ent->entity = new entityType(params);\

/**
 * @brief Handles creation of buildable entities.
 */
static void BuildableSpawnCBSE(gentity_t *ent, buildable_t buildable) {
	switch (buildable) {
		case BA_A_ACIDTUBE: {
			BUILDABLE_ENTITY_CREATE(AcidTubeEntity);
			break;
		}

		case BA_A_BARRICADE: {
			BUILDABLE_ENTITY_CREATE(BarricadeEntity);
			break;
		}

		case BA_A_BOOSTER: {
			BUILDABLE_ENTITY_CREATE(BoosterEntity);
			break;
		}

		case BA_A_HIVE: {
			BUILDABLE_ENTITY_CREATE(HiveEntity);
			break;
		}

		case BA_A_LEECH: {
			BUILDABLE_ENTITY_CREATE(LeechEntity);
			break;
		}

		case BA_A_OVERMIND: {
			BUILDABLE_ENTITY_CREATE(OvermindEntity);
			break;
		}

		case BA_A_SPAWN: {
			BUILDABLE_ENTITY_CREATE(EggEntity);
			break;
		}

		case BA_A_SPIKER: {
			BUILDABLE_ENTITY_CREATE(SpikerEntity);
			break;
		}

		case BA_A_TRAPPER: {
			BUILDABLE_ENTITY_CREATE(TrapperEntity);
			break;
		}

		case BA_H_ARMOURY: {
			BUILDABLE_ENTITY_CREATE(ArmouryEntity);
			break;
		}

		case BA_H_DRILL: {
			BUILDABLE_ENTITY_CREATE(DrillEntity);
			break;
		}

		case BA_H_MEDISTAT: {
			BUILDABLE_ENTITY_CREATE(MedipadEntity);
			break;
		}

		case BA_H_MGTURRET: {
			BUILDABLE_ENTITY_CREATE(MGTurretEntity);
			break;
		}

		case BA_H_REACTOR: {
			BUILDABLE_ENTITY_CREATE(ReactorEntity);
			break;
		}

		case BA_H_REPEATER: {
			BUILDABLE_ENTITY_CREATE(RepeaterEntity);
			break;
		}

		case BA_H_ROCKETPOD: {
			BUILDABLE_ENTITY_CREATE(RocketpodEntity);
			break;
		}

		case BA_H_SPAWN: {
			BUILDABLE_ENTITY_CREATE(TelenodeEntity);
			break;
		}

		default:
			assert(false);
	}
}

/*
================
G_Build

Spawns a buildable
================
*/
static gentity_t *Build( gentity_t *builder, buildable_t buildable, const vec3_t origin,
                         const vec3_t normal, const vec3_t angles, int groundEntNum )
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
		log = nullptr;
	}

	built = G_NewEntity();

	BuildableSpawnCBSE(built, buildable);

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

	if (builder->client) {
		// Build instantly in cheat mode.
		if (g_instantBuilding.integer) {
			// HACK: This causes animation issues and can result in built->creationTime < 0.
			built->creationTime -= attr->buildTime;
		} else {
			HealthComponent *healthComponent = built->entity->Get<HealthComponent>();
			healthComponent->SetHealth(healthComponent->MaxHealth() * BUILDABLE_START_HEALTH_FRAC);
		}
	}

	built->splashDamage = attr->splashDamage;
	built->splashRadius = attr->splashRadius;
	built->splashMethodOfDeath = attr->meansOfDeath;

	built->nextthink = level.time;
	built->enabled = false;
	built->spawned = false;

	built->s.time = built->creationTime;

	//things that vary for each buildable that aren't in the dbase
	switch ( buildable )
	{
		case BA_A_SPAWN:
			built->think = ASpawn_Think;
			break;

		case BA_A_BARRICADE:
			built->think = ABarricade_Think;
			built->touch = ABarricade_Touch;
			built->shrunkTime = 0;
			ABarricade_Shrink( built, true );
			break;

		case BA_A_BOOSTER:
			built->think = ABooster_Think;
			built->touch = ABooster_Touch;
			break;

		case BA_A_ACIDTUBE:
			break;

		case BA_A_HIVE:
			built->think = AHive_Think;
			break;

		case BA_A_TRAPPER:
			built->think = ATrapper_Think;
			break;

		case BA_A_LEECH:
			break;

		case BA_A_SPIKER:
			built->think = ASpiker_Think;
			break;

		case BA_A_OVERMIND:
			built->think = AOvermind_Think;
			break;

		case BA_H_SPAWN:
			built->think = HSpawn_Think;
			break;

		case BA_H_MGTURRET:
			built->think = HMGTurret_Think;
			break;

		case BA_H_ROCKETPOD:
			built->think = HRocketpod_Think;
			break;

		case BA_H_ARMOURY:
			built->think = HArmoury_Think;
			built->use = HArmoury_Use;
			break;

		case BA_H_MEDISTAT:
			built->think = HMedistat_Think;
			break;

		case BA_H_DRILL:
			built->think = HDrill_Think;
			break;

		case BA_H_REACTOR:
			built->think = HReactor_Think;
			built->powered = built->active = true;
			break;

		case BA_H_REPEATER:
			built->think = HRepeater_Think;
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
	built->target     = nullptr;
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
		built->builtBy = nullptr;
	}

	G_SetOrigin( built, origin );

	// set turret angles
	VectorCopy( builder->s.angles2, built->s.angles2 );

	VectorCopy( angles, built->s.angles );
	built->s.angles[ PITCH ] = 0.0f;
	built->s.angles2[ YAW ] = angles[ YAW ];
	built->s.angles2[ PITCH ] = 0.0f; // Neutral pitch since this is how the ghost buildable looks.
	built->physicsBounce = attr->bounce;

	built->s.groundEntityNum = groundEntNum;
	if ( groundEntNum == ENTITYNUM_NONE )
	{
		built->s.pos.trType = attr->traj;
		built->s.pos.trTime = level.time;
		// gently nudge the buildable onto the surface :)
		VectorScale( normal, -50.0f, built->s.pos.trDelta );
	}

	/*built->powered = true;

	built->s.eFlags |= EF_B_POWERED;
	built->s.eFlags &= ~EF_B_SPAWNED;*/

	VectorCopy( normal, built->s.origin2 );

	G_AddEvent( built, EV_BUILD_CONSTRUCT, 0 );

	G_SetIdleBuildableAnim( built, BANIM_IDLE1 );
	G_SetBuildableAnim( built, BANIM_CONSTRUCT, true );

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

	if( builder->client )
		Beacon::Tag( built, (team_t)builder->client->pers.team, true );

	return built;
}

bool G_BuildIfValid( gentity_t *ent, buildable_t buildable )
{
	float  dist;
	vec3_t origin, normal;
	int    groundEntNum;
	vec3_t forward, aimDir;

	BG_GetClientNormal( &ent->client->ps, normal);
	AngleVectors( ent->client->ps.viewangles, aimDir, nullptr, nullptr );
	ProjectPointOnPlane( forward, aimDir, normal );
	VectorNormalize( forward );
	dist = BG_Class( ent->client->ps.stats[ STAT_CLASS ] )->buildDist * DotProduct( forward, aimDir );

	switch ( G_CanBuild( ent, buildable, dist, origin, normal, &groundEntNum ) )
	{
		case IBE_NONE:
			Build( ent, buildable, origin, normal, ent->s.apos.trBase, groundEntNum );
			G_ModifyBuildPoints( BG_Buildable( buildable )->team, -BG_Buildable( buildable )->buildPoints );
			return true;

		case IBE_NOALIENBP:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOBP );
			return false;

		case IBE_NOOVERMIND:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOOVMND );
			return false;

		case IBE_NOCREEP:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOCREEP );
			return false;

		case IBE_ONEOVERMIND:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_ONEOVERMIND );
			return false;

		case IBE_NORMAL:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_NORMAL );
			return false;

		case IBE_SURFACE:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_NORMAL );
			return false;

		case IBE_DISABLED:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_DISABLED );
			return false;

		case IBE_ONEREACTOR:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_ONEREACTOR );
			return false;

		case IBE_NOPOWERHERE:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOPOWERHERE );
			return false;

		case IBE_NOREACTOR:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOREACTOR );
			return false;

		case IBE_NOROOM:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_NOROOM );
			return false;

		case IBE_NOHUMANBP:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOBP );
			return false;

		case IBE_LASTSPAWN:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_LASTSPAWN );
			return false;

		case IBE_MAINSTRUCTURE:
			G_TriggerMenu( ent->client->ps.clientNum, MN_B_MAINSTRUCTURE );
			return false;

		default:
			break;
	}

	return false;
}

/*
================
G_FinishSpawningBuildable

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
static gentity_t *FinishSpawningBuildable( gentity_t *ent, bool force )
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

	built = Build( ent, buildable, ent->s.pos.trBase, normal, ent->s.angles, ENTITYNUM_NONE );

	// This particular function is used by buildables that skip construction.
	built->entity->Get<BuildableComponent>()->SetState(BuildableComponent::CONSTRUCTED);

	/*built->spawned = true; //map entities are already spawned
	built->enabled = true;
	built->s.eFlags |= EF_B_SPAWNED;*/

	// drop towards normal surface
	VectorScale( built->s.origin2, -4096.0f, dest );
	VectorAdd( dest, built->s.origin, dest );

	trap_Trace( &tr, built->s.origin, built->r.mins, built->r.maxs, dest, built->s.number,
	            built->clipmask, 0 );

	if ( tr.startsolid && !force )
	{
		G_Printf( S_COLOR_YELLOW "G_FinishSpawningBuildable: %s startsolid at %s\n",
		          built->classname, vtos( built->s.origin ) );
		G_FreeEntity( built );
		return nullptr;
	}

	//point items in the correct direction
	VectorCopy( tr.plane.normal, built->s.origin2 );

	// allow to ride movers
	built->s.groundEntityNum = tr.entityNum;

	G_SetOrigin( built, tr.endpos );

	trap_LinkEntity( built );

	Beacon::Tag( built, (team_t)BG_Buildable( buildable )->team, true );

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
	FinishSpawningBuildable( ent, false );
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
		if ( ( listLen + fileLen ) >= (int) sizeof( layouts ) )
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
void G_LayoutSelect()
{
	char fileName[ MAX_OSPATH ];
	char layouts[ MAX_CVAR_VALUE_STRING ];
	char layouts2[ MAX_CVAR_VALUE_STRING ];
	const char *layoutPtr;
	char map[ MAX_QPATH ];
	char *layout;
	int  cnt = 0;
	int  layoutNum;

	Q_strncpyz( layouts, g_layouts.string, sizeof( layouts ) );
	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );

	// one time use cvar
	trap_Cvar_Set( "g_layouts", "" );

	// pick from list of default layouts if provided
	if ( !layouts[ 0 ] )
	{
		Q_strncpyz( layouts, g_defaultLayouts.string, sizeof( layouts ) );
	}

	// pick an included layout at random if no list has been provided
	if ( !layouts[ 0 ] && g_layoutAuto.integer )
	{
		G_LayoutList( map, layouts, sizeof( layouts ) );
	}

	// no layout specified, use the map's builtin layout
	if ( !layouts[ 0 ] )
	{
		return;
	}

	Q_strncpyz( layouts2, layouts, sizeof( layouts2 ) );
	layoutPtr = &layouts2[ 0 ];
	layouts[ 0 ] = '\0';

	while ( *(layout = COM_ParseExt( &layoutPtr, false )) )
	{
		if ( !Q_stricmp( layout, S_BUILTIN_LAYOUT ) )
		{
			Q_strcat( layouts, sizeof( layouts ), layout );
			Q_strcat( layouts, sizeof( layouts ), " " );
			cnt++;
			continue;
		}

		Com_sprintf( fileName, sizeof( fileName ), "layouts/%s/%s.dat", map, layout );

		if ( trap_FS_FOpenFile( fileName, nullptr, FS_READ ) > 0 )
		{
			Q_strcat( layouts, sizeof( layouts ), layout );
			Q_strcat( layouts, sizeof( layouts ), " " );
			cnt++;
		}
		else
		{
			G_Printf( S_WARNING "Layout \"%s\" does not exist.\n", layout );
		}
	}

	if ( !cnt )
	{
		G_Printf( S_ERROR "None of the specified layouts could be found, using map default.\n" );
		return;
	}

	layoutNum = ( rand() % cnt ) + 1;
	Q_strncpyz( layouts2, layouts, sizeof( layouts2 ) );
	layoutPtr = &layouts2[ 0 ];

	for ( cnt = 0; *(layout = COM_ParseExt( &layoutPtr, false )) && cnt < layoutNum; cnt++ )
	{
		Q_strncpyz( level.layout, layout, sizeof( level.layout ) );
	}

	G_Printf( "Using layout \"%s\" from list (%s).\n", level.layout, layouts );
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
void G_LayoutLoad()
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
		if ( i >= (int) sizeof( line ) - 1 )
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

		if ( ent->s.eType != ET_BUILDABLE )
		{
			continue;
		}

		if ( ent->buildableTeam != team )
		{
			continue;
		}

		G_Kill(ent, MOD_SUICIDE);
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
	log->actor = actor && actor->client ? actor->client->pers.namelog : nullptr;
	return log;
}

void G_BuildLogSet( buildLog_t *log, gentity_t *ent )
{
	log->buildableTeam = ent->buildableTeam;
	log->modelindex = (buildable_t) ent->s.modelindex;
	log->markedForDeconstruction = ent->entity->Get<BuildableComponent>()->MarkedForDeconstruction();
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

	built = FinishSpawningBuildable( ent, true );

	// TODO: CBSE-ify. Currently ent is a pseudo entity that doesn't have a buildable component, so
	//       we can't get rid of gentity_t.deconstruct yet.
	if (ent->deconMarkHack) {
		// Note that this doesn't preserve original mark time.
		built->entity->Get<BuildableComponent>()->SetDeconstructionMark();
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

				if ( ( ( ent->s.eType == ET_BUILDABLE && G_Alive( ent ) ) ||
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
			// HACK: Uses legacy pseudo entity. TODO: CBSE-ify.
			buildable = G_NewEntity();
			VectorCopy( log->origin, buildable->s.pos.trBase );
			VectorCopy( log->angles, buildable->s.angles );
			VectorCopy( log->origin2, buildable->s.origin2 );
			VectorCopy( log->angles2, buildable->s.angles2 );
			buildable->s.modelindex = log->modelindex;
			buildable->deconMarkHack = log->markedForDeconstruction;
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
