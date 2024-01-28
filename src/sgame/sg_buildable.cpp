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

#include "common/FileSystem.h"
#include "shared/bg_gameplay.h"
#include "sg_local.h"
#include "CustomSurfaceFlags.h"
#include "Entities.h"
#include "CBSE.h"
#include "sg_cm_world.h"

static Cvar::Cvar<bool> g_indestructibleBuildables(
		"g_indestructibleBuildables",
		"make buildables impossible to destroy (Note: this only applies only to buildings built after the variable is set, This also means it must be set before map load for the default buildables to be protected)", Cvar::NONE, false);

/**
 * @return Whether the means of death allow for an under-attack warning.
 */
bool G_IsWarnableMOD(meansOfDeath_t mod) {
	switch (mod) {
		case MOD_TRIGGER_HURT:
		case MOD_DECONSTRUCT:
		case MOD_REPLACE:
		case MOD_BUILDLOG_REVERT:
		case MOD_SUICIDE:
			return false;

		default:
			return true;
	}
}

static gentity_t *FindBuildable(buildable_t buildable) {
	gentity_t* found = nullptr;

	ForEntities<BuildableComponent>([&](Entity& entity, BuildableComponent&) {
		if (entity.oldEnt->s.modelindex == buildable) {
			found = entity.oldEnt;
		}
	});

	return found;
}

template<typename Component>
static gentity_t *LookUpMainBuildable(bool requireActive)
{
	gentity_t *answer = nullptr;
	ForEntities<Component>([&](Entity& entity, Component&) {
		if (!requireActive ||
		    entity.Get<BuildableComponent>()->GetState() == BuildableComponent::CONSTRUCTED)
		{
			answer = entity.oldEnt;
		}
	});
	return answer;
}

gentity_t *G_Overmind() {
	return LookUpMainBuildable<OvermindComponent>(false);
}

gentity_t *G_ActiveOvermind() {
	return LookUpMainBuildable<OvermindComponent>(true);
}

gentity_t *G_Reactor() {
	return LookUpMainBuildable<ReactorComponent>(false);
}

gentity_t *G_ActiveReactor() {
	return LookUpMainBuildable<ReactorComponent>(true);
}

gentity_t *G_MainBuildable(team_t team) {
	switch (team) {
		case TEAM_ALIENS: return G_Overmind();
		case TEAM_HUMANS: return G_Reactor();
		default:          return nullptr;
	}
}

gentity_t *G_ActiveMainBuildable(team_t team) {
	switch (team) {
		case TEAM_ALIENS: return G_ActiveOvermind();
		case TEAM_HUMANS: return G_ActiveReactor();
		default:          return nullptr;
	}
}

/**
 * @return The distance of an entity to its own base or a huge value if the base is not found.
 *
 * Please keep it in sync with CG_DistanceToBase
 */
float G_DistanceToBase(gentity_t *self) {
	gentity_t *mainBuilding = G_MainBuildable(G_Team(self));

	if (mainBuilding) {
		return G_Distance(self, mainBuilding);
	} else {
		return 1E+37;
	}
}

#define INSIDE_BASE_MAX_DISTANCE 1000.0f

/**
 * @return Whether an entity is inside its own main base.
 * @todo Use base clustering.
 */
bool G_InsideBase(gentity_t *self) {
	gentity_t *mainBuilding = G_MainBuildable(G_Team(self));

	if (G_DistanceToBase(self) >= INSIDE_BASE_MAX_DISTANCE) {
		return false;
	}

	return trap_InPVSIgnorePortals(self->s.origin, mainBuilding->s.origin);
}

bool G_DretchCanDamageEntity( const gentity_t *self, const gentity_t *ent )
{
	switch (ent->s.eType)
	{
		case entityType_t::ET_PLAYER:
		case entityType_t::ET_MOVER:
			return true;
		case entityType_t::ET_BUILDABLE:
			// dretches can only bite buildables in construction or turrets
			return !ent->spawned || BG_Buildable( ent->s.modelindex )->dretchAttackable;
		default:
			ASSERT_UNREACHABLE();
	}
}

/**
 * @brief Triggers client side buildable animation.
 */
void G_SetBuildableAnim(gentity_t *ent, buildableAnimNumber_t animation, bool force) {
	int newAnimation = (int)animation;

	newAnimation |= (ent->s.legsAnim & ANIM_TOGGLEBIT);

	if (force) newAnimation |= ANIM_FORCEBIT;

	// Don't flip the toggle bit more than once per frame.
	if (ent->animTime != level.time) {
		ent->animTime = level.time;
		newAnimation ^= ANIM_TOGGLEBIT;
	}

	ent->s.legsAnim = newAnimation;
}

/**
 * @brief Sets the client side buildable idle animation.
 */
void G_SetIdleBuildableAnim(gentity_t *ent, buildableAnimNumber_t animation) {
	ent->s.torsoAnim = animation;
}

void ABarricade_Shrink( gentity_t *self, bool shrink )
{
	if ( !self->spawned || Entities::IsDead( self ) )
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

		if ( self->spawned && Entities::IsAlive( self ) && anim != BANIM_IDLE_UNPOWERED )
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
		if ( self->spawned && Entities::IsAlive( self ) )
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
		            self->s.origin, self->num(), MASK_PLAYERSOLID, 0 );

		if ( tr.startsolid || tr.fraction < 1.0f )
		{
			self->r.maxs[ 2 ] = ( int )( self->r.maxs[ 2 ] * BARRICADE_SHRINKPROP );
			return;
		}

		self->shrunkTime = 0;

		// unshrink animation
		anim = self->s.legsAnim & ~( ANIM_FORCEBIT | ANIM_TOGGLEBIT );

		if ( self->spawned && Entities::IsAlive( self ) && anim != BANIM_CONSTRUCT && anim != BANIM_POWERUP )
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

static void ABarricade_Think( gentity_t *self )
{
	self->nextthink = level.time + 1000;

	// Shrink if unpowered
	ABarricade_Shrink( self, !self->powered );
}

static void ABarricade_Touch( gentity_t *self, gentity_t *other, trace_t* )
{
	gclient_t *client = other->client;
	int       client_z, min_z;

	if ( !client || client->pers.team != TEAM_ALIENS )
	{
		return;
	}

	// Client must be high enough to pass over.
	client_z = other->s.origin[ 2 ] + other->r.mins[ 2 ];
	min_z = self->s.origin[ 2 ] - STEPSIZE +
	        ( int )( self->r.maxs[ 2 ] * BARRICADE_SHRINKPROP );

	if ( client_z < min_z )
	{
		return;
	}

	ABarricade_Shrink( self, true );
}

static void ABooster_Think( gentity_t *self )
{
	gentity_t *ent;
	bool  playHealingEffect = false;

	self->nextthink = level.time + BOOST_REPEAT_ANIM / 4;

	// check if there is a closeby alien that used this booster for healing recently
	for ( ent = nullptr; ( ent = G_IterateEntitiesWithinRadius( ent, VEC2GLM( self->s.origin ), REGEN_BOOSTER_RANGE ) ); )
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

static void ABooster_Touch( gentity_t *self, gentity_t *other, trace_t* )
{
	gclient_t *client = other->client;

	if ( !self->spawned || !self->powered || Entities::IsDead( self ) )
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

static void ATrapper_FireOnEnemy( gentity_t *self, int firespeed )
{
	gentity_t *target = self->target.entity;
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

	//fire at target
	G_SpawnMissile( MIS_LOCKBLOB, self, self->s.pos.trBase, dirToTarget, nullptr,
	                G_ExplodeMissile, level.time + BG_Missile( MIS_LOCKBLOB )->lifetime );
	G_SetBuildableAnim( self, BANIM_ATTACK1, false );
	self->customNumber = level.time + firespeed;
}

static bool ATrapper_CheckTarget( gentity_t *self, GentityRef target, int range )
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

	if ( target.entity == self ) // is the target us?
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

	if ( Entities::IsDead( target.entity ) ) // is the target still alive?
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

	trap_Trace( &trace, self->s.pos.trBase, nullptr, nullptr, target->s.pos.trBase, self->num(),
	            MASK_SHOT, 0 );

	if ( trace.contents & CONTENTS_SOLID ) // can we see the target?
	{
		return false;
	}

	return true;
}

static void ATrapper_FindEnemy( gentity_t *ent, int range )
{
	GentityRef target;
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

static void ATrapper_Think( gentity_t *self )
{
	self->nextthink = level.time + 100;

	if ( !self->spawned || !self->powered || Entities::IsDead( self ) )
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

static void HArmoury_Use( gentity_t *self, gentity_t*, gentity_t *activator )
{
	if ( !self->spawned )
	{
		return;
	}

	if ( activator->client->pers.team != TEAM_HUMANS )
	{
		return;
	}

	if ( !self->powered || Entities::IsDead(self) )
	{
		G_TriggerMenu( activator->num(), MN_H_NOTPOWERED );
		return;
	}

	G_TriggerMenu( activator->client->ps.clientNum, MN_H_ARMOURY );
}

static void HMedistat_Think( gentity_t *self )
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

	// clear target's healing flag for now
	if ( self->target && self->target->client )
	{
		self->target->client->ps.stats[ STAT_STATE ] &= ~SS_HEALING_2X;
	}
	else
	{
		self->target = nullptr;
	}

	// clear target on power loss
	if ( !self->powered || Entities::IsDead(self) )
	{
		self->medistationIsHealing = false;
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

		if ( self->target.entity == player && PM_Live( client->ps.pm_type ) &&
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
		player = self->target.entity;
		client = player->client;
		client->ps.stats[ STAT_STATE ] |= SS_HEALING_2X;

		// start healing animation
		if ( !self->medistationIsHealing )
		{
			self->medistationIsHealing = true;

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
	else if ( self->medistationIsHealing )
	{
		self->medistationIsHealing = false;

		// stop healing animation
		G_SetBuildableAnim( self, BANIM_ATTACK2, true );
		G_SetIdleBuildableAnim( self, BANIM_IDLE1 );
	}
}

/**
 * @brief Orders buildables that were pre-selected for power down to make good a budget deficit.
 * @todo Add const to parameters once there is a const variant of Entity::Get.
 */
static bool CompareBuildablesForPowerSaving(Entity* a, Entity* b)
{
	if (!a) return false;
	if (!b) return true;

	const BuildableComponent* aC = a->Get<BuildableComponent>();
	const BuildableComponent* bC = b->Get<BuildableComponent>();

	// Prefer the marked buildable.
	if ( aC->MarkedForDeconstruction() && !bC->MarkedForDeconstruction()) return true;
	if (!aC->MarkedForDeconstruction() &&  bC->MarkedForDeconstruction()) return false;

	// If both are marked, prefer the one marked last.
	if (aC->MarkedForDeconstruction() && bC->MarkedForDeconstruction()) {
		return (aC->GetMarkTime() > bC->GetMarkTime());
	}

	// Prefer the buildable further away from the base.
	// Note that this function is supposed to be used only when there is a base, since otherwise
	// every structure that can shut down did so already.
	return (G_DistanceToBase(a->oldEnt) > G_DistanceToBase(b->oldEnt));
}

/**
 * @brief Set the power state of both team's buildables based on budget deficits.
 */
void G_UpdateBuildablePowerStates()
{
	gentity_t* activeMainBuildable;

	for (team_t team = TEAM_NONE; (team = G_IterateTeams(team)); ) {
		std::vector<Entity*> poweredBuildables;
		std::vector<Entity*> unpoweredBuildables;
		int unpoweredBuildableTotal = 0;
		activeMainBuildable = G_ActiveMainBuildable(team);

		ForEntities<BuildableComponent>([&](Entity& entity, BuildableComponent& buildableComponent) {
			if (G_Team(entity.oldEnt) != team) return;

			// Never shut down the main buildable or miners.
			if (entity.Get<MainBuildableComponent>()) return;
			if (entity.Get<MiningComponent>()) return;

			// Never shut down spawns.
			// TODO: Refer to a SpawnerComponent here.
			if (entity.Get<TelenodeComponent>() || entity.Get<EggComponent>()) return;

			// Power off all buildables if there is no main buildable.
			if (!activeMainBuildable) {
				buildableComponent.SetPowerState(false);
				return;
			}

			// In order to make good a deficit, don't shut down buildables that have no cost.
			if (BG_Buildable(entity.oldEnt->s.modelindex)->buildPoints <= 0) return;
			if (!entity.oldEnt->powered) {
				unpoweredBuildables.push_back(&entity);
				unpoweredBuildableTotal += BG_Buildable(entity.oldEnt->s.modelindex)->buildPoints;
			} else {
				poweredBuildables.push_back(&entity);
			}
		});

		// If there is no active main buildable, all buildables that can shut down already did so.
		if (!activeMainBuildable) continue;

		// Positive deficit means that we are over, and negative means we have a surplus.
		int deficit = level.team[team].spentBudget - (int)level.team[team].totalBudget - unpoweredBuildableTotal;

		// Exactly at our limit. Nothing else to do.
		if (deficit == 0) continue;

		// We have surplus bp, but nothing else to power on, so we're done here.
		if (deficit < 0 && unpoweredBuildables.empty()) continue;

		// Uh oh...start powering stuff down.
		if (deficit > 0) {
			std::sort(poweredBuildables.begin(), poweredBuildables.end(), CompareBuildablesForPowerSaving);
			for(Entity* entity : poweredBuildables) {
				entity->Get<BuildableComponent>()->SetPowerState(false);

				// Dying buildables have already substracted their share from the spent budget pool.
				if (entity->Get<HealthComponent>()->Alive()) {
					deficit -= BG_Buildable(entity->oldEnt->s.modelindex)->buildPoints;
				}

				if (deficit <= 0) break;
			}
		} else if (deficit < 0) {
			// Make our deficit positive for ease of calculation.
			int surplus = -deficit;
			std::sort(unpoweredBuildables.begin(), unpoweredBuildables.end(), CompareBuildablesForPowerSaving);
			for (auto it = unpoweredBuildables.rbegin(); it != unpoweredBuildables.rend(); ++it) {
				int buildableCost = BG_Buildable((*it)->oldEnt->s.modelindex)->buildPoints;

				// not cheap enough
				if (surplus < buildableCost) continue;
				// don't switch on unpowered buildables on destruction
				if (!(*it)->Get<HealthComponent>()->Alive()) continue;

				(*it)->Get<BuildableComponent>()->SetPowerState(true);
				surplus -= buildableCost;
			}
		}
	}
}

/**
 * @brief Find all trigger entities that a buildable touches.
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
	if ( Entities::IsDead( ent ) )
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

		if ( !( hit->r.contents & CONTENTS_TRIGGER ) )
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

/**
 * @return Whether origin is within a distance of radius of a buildable of the given type.
 */
bool G_BuildableInRange( vec3_t origin, float radius, buildable_t buildable )
{
	gentity_t *neighbor = nullptr;

	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, VEC2GLM( origin ), radius ) ) )
	{
		if ( neighbor->s.eType != entityType_t::ET_BUILDABLE || !neighbor->spawned || Entities::IsDead( neighbor ) ||
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

/**
 * @return Whether two buildables built at the given locations would intersect.
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

gentity_t *G_GetDeconstructibleBuildable( gentity_t *ent )
{
	vec3_t viewOrigin, forward, end;
	trace_t trace;
	gentity_t *buildable;

	// Check for revoked building rights.
	if ( ent->client->pers.namelog->denyBuild || G_admin_permission( ent, ADMF_NO_BUILD ) )
	{
		G_TriggerMenu( ent->client->ps.clientNum, MN_B_REVOKED );
		return nullptr;
	}

	// Trace for target.
	BG_GetClientViewOrigin( &ent->client->ps, viewOrigin );
	AngleVectors( ent->client->ps.viewangles, forward, nullptr, nullptr );
	VectorMA( viewOrigin, BUILDER_DECONSTRUCT_RANGE, forward, end );
	trap_Trace( &trace, viewOrigin, nullptr, nullptr, end, ent->num(), MASK_PLAYERSOLID, 0 );
	buildable = &g_entities[ trace.entityNum ];

	// Check if target is valid.
	if ( trace.fraction >= 1.0f ||
	     buildable->s.eType != entityType_t::ET_BUILDABLE ||
	     !G_OnSameTeam( ent, buildable ) )
	{
		return nullptr;
	}

	return buildable;
}

bool G_DeconstructDead( gentity_t *buildable )
{
	// Always let the builder prevent the explosion of a buildable.
	if ( Entities::IsDead( buildable ) )
	{
		G_RewardAttackers( buildable );
		G_FreeEntity( buildable );
		return true;
	}

	return false;
}

static void G_Deconstruct( gentity_t *self, gentity_t *deconner, meansOfDeath_t deconType )
{
	if ( !self || self->s.eType != entityType_t::ET_BUILDABLE )
	{
		return;
	}

	const buildableAttributes_t *attr = BG_Buildable( self->s.modelindex );

	// return some build points immediately
	int immediateRefund = G_BuildableDeconValue( self );
	int queuedRefund    = attr->buildPoints - immediateRefund;
	G_FreeBudget( self->buildableTeam, immediateRefund, queuedRefund );

	// remove momentum
	G_RemoveMomentumForDecon( self, deconner );

	// reward attackers if the structure was hurt before deconstruction
	float healthFraction = self->entity->Get<HealthComponent>()->HealthFraction();
	if ( healthFraction < 1.0f ) G_RewardAttackers( self );

	// deconstruct
	Entities::Kill(self, deconner, deconType);

	// TODO: Check if freeing needs to be deferred.
	G_FreeEntity( self );
}

// Returns true and warns the player if they are not allowed to decon it
bool G_CheckDeconProtectionAndWarn( gentity_t *buildable, gentity_t *player )
{
	if ( g_instantBuilding.Get() )
	{
		return false;
	}
	switch ( buildable->s.modelindex )
	{
		case BA_A_OVERMIND:
		case BA_H_REACTOR:
			G_TriggerMenu( player->client->ps.clientNum, MN_B_MAINSTRUCTURE );
			return true;

		case BA_A_SPAWN:
		case BA_H_SPAWN:
			if ( level.team[ player->client->ps.persistant[ PERS_TEAM ] ].numSpawns <= 1 )
			{
				G_TriggerMenu( player->client->ps.clientNum, MN_B_LASTSPAWN );
				return true;
			}
			break;
	}
	return false;
}

void G_DeconstructUnprotected( gentity_t *buildable, gentity_t *ent )
{
	if ( !g_instantBuilding.Get() )
	{
		// Check if the buildable is protected from instant deconstruction.
		G_CheckDeconProtectionAndWarn( buildable, ent );

		// Deny if build timer active.
		if ( ent->client->ps.stats[ STAT_MISC ] > 0 )
		{
			G_AddEvent( ent, EV_BUILD_DELAY, ent->client->ps.clientNum );
			return;
		}

		// Add to build timer.
		ent->client->ps.stats[ STAT_MISC ] += BG_Buildable( buildable->s.modelindex )->buildTime / 4;
	}

	G_Deconstruct( buildable, ent, MOD_DECONSTRUCT );
}

/**
 * @return The number of buildables removed.
 */
static int G_FreeMarkedBuildables( gentity_t *deconner, char *readable,
		int rsize, char *nums, int nsize )
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
			Q_strcat( nums, nsize, va( " %ld", ( long )( ent->num() ) ) );
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

	return IBE_NONE;
}

/**
 * @brief Attempts to build a set of buildables that have to be deconstructed for a new buildable.
 *
 * Sets level.markedBuildables and level.numBuildablesForRemoval.
 */
static itemBuildError_t PrepareBuildableReplacement( buildable_t buildable, vec3_t origin )
{
	int              entNum, listLen;
	gentity_t        *ent, *list[ MAX_GENTITIES ];
	const buildableAttributes_t *attr;

	// resource related
	int cost;

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

	// -------------------
	// check for resources
	// -------------------

	cost = attr->buildPoints;

	// Buildables with no cost can be built even if the free budget is negative.
	if (cost <= 0)
	{
		return IBE_NONE;
	}

	// if we already have set buildables for removal, decrease cost
	for ( entNum = 0; entNum < level.numBuildablesForRemoval; entNum++ )
	{
		cost -= G_BuildableDeconValue( level.markedBuildables[ entNum ] );
	}

	// check if we can already afford the new buildable
	if ( G_GetFreeBudget( attr->team ) >= cost )
	{
		return IBE_NONE;
	}

	// build a list of additional buildables that can be deconstructed
	listLen = 0;

	for ( entNum = MAX_CLIENTS; entNum < level.num_entities; entNum++ )
	{
		ent = &g_entities[ entNum ];

		// check if buildable of own team
		if ( ent->s.eType != entityType_t::ET_BUILDABLE || ent->buildableTeam != attr->team )
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

		// fix https://github.com/Unvanquished/Unvanquished/issues/2907
		// the case of a collision requiring removal of the leech/drill is handled elsewhere
		buildable_t otherBuildable = static_cast< buildable_t >( ent->s.modelindex );
		if ( otherBuildable == BA_A_LEECH || otherBuildable == BA_H_DRILL )
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

		level.markedBuildables[ level.numBuildablesForRemoval++ ] = ent;

		cost -= G_BuildableDeconValue( ent );

		// check if we have enough resources now
		if ( G_GetFreeBudget( attr->team ) >= cost )
		{
			return IBE_NONE;
		}
	}

	// we don't have enough resources
	// TODO: Remove alien/human distinction.
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

static void SetBuildableLinkState( bool link )
{
	int       i;
	gentity_t *ent;

	for ( i = MAX_CLIENTS, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( ent->s.eType != entityType_t::ET_BUILDABLE )
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

itemBuildError_t G_CanBuild( gentity_t *ent, buildable_t buildable, int /*distance*/, //TODO
                             vec3_t origin, vec3_t normal, int *groundEntNum )
{
	vec3_t           angles;
	vec3_t           entity_origin;
	vec3_t           mins, maxs;
	trace_t          tr1, tr2, tr3;
	itemBuildError_t reason = IBE_NONE;
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
	trap_Trace( &tr3, ps->origin, nullptr, nullptr, entity_origin, ent->num(), MASK_PLAYERSOLID, 0 );

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

	contents = G_CM_PointContents( entity_origin, -1 );

	// Prepare replacement of other buildables.
	itemBuildError_t replacementError;
	if ( ( replacementError = PrepareBuildableReplacement( buildable, origin ) ) != IBE_NONE )
	{
		reason = replacementError;
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

		// Check surface permissions
		bool invalid = (tr1.contents & (CUSTOM_CONTENTS_NOALIENBUILD | CUSTOM_CONTENTS_NOBUILD)) || (contents & (CUSTOM_CONTENTS_NOALIENBUILD | CUSTOM_CONTENTS_NOBUILD));
		if ( invalid && !g_ignoreNobuild.Get() )
		{
			reason = IBE_SURFACE;
		}

		// Check level permissions
		if ( !g_alienAllowBuilding.Get() )
		{
			reason = IBE_DISABLED;
		}
	}
	else if ( ent->client->pers.team == TEAM_HUMANS )
	{
		// Check for Reactor
		if ( buildable != BA_H_REACTOR )
		{
			if ( !G_ActiveReactor() )
			{
				reason = IBE_NOREACTOR;
			}
		}

		// Check permissions
		bool invalid = (tr1.contents & (CUSTOM_CONTENTS_NOHUMANBUILD | CUSTOM_CONTENTS_NOBUILD)) || (contents & (CUSTOM_CONTENTS_NOHUMANBUILD | CUSTOM_CONTENTS_NOBUILD));
		if ( invalid && !g_ignoreNobuild.Get() )
		{
			reason = IBE_SURFACE;
		}

		// Check level permissions
		if ( !g_humanAllowBuilding.Get() )
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
					Sys::Error( "No reason for denying build of %d", buildable );
					break;
			}
		}
	}

	// Relink buildables
	SetBuildableLinkState( true );

	// Check there is enough room to spawn from, if trying to build a spawner.
	if ( reason == IBE_NONE )
	{
		glm::vec3 originV = VEC2GLM( origin );
		glm::vec3 normalV = VEC2GLM( normal );
		Entity* blocker;
		glm::vec3 spawnPoint;

		switch (buildable) {
			case BA_A_SPAWN:
				SetBuildableMarkedLinkState(false);
				if (!EggComponent::CheckSpawnPoint(0, originV, normalV, blocker, spawnPoint)) {
					reason = IBE_NORMAL;
				}
				SetBuildableMarkedLinkState(true);
				break;

			case BA_H_SPAWN:
				SetBuildableMarkedLinkState(false);
				if (!TelenodeComponent::CheckSpawnPoint(0, originV, normalV, blocker, spawnPoint)) {
					reason = IBE_NORMAL;
				}
				SetBuildableMarkedLinkState(true);
				break;

			default:
				break;
		}
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

	int max_miners = g_maxMiners.Get();
	if ( max_miners >= 0 && ( buildable == BA_H_DRILL || buildable == BA_A_LEECH ) )
	{
		int miners = 0;
		ForEntities<MiningComponent> ( [&](Entity& entity, MiningComponent& )
		{
			if ( Entities::IsAlive(entity) && G_OnSameTeam( entity.oldEnt, ent ) )
			{
				miners++;
			}
		});
		if ( miners >= max_miners )
		{
			return ent->client->pers.team == TEAM_HUMANS ? IBE_NOMOREDRILLS : IBE_NOMORELEECHES;
		}
	}

	return reason;
}

/** Sets shared buildable entity parameters. */
#define BUILDABLE_ENTITY_SET_PARAMS(params)\
	params.oldEnt = ent;\
	params.Health_maxHealth = BG_Buildable(buildable)->health;

#define BUILDABLE_ENTITY_START(entityType)\
	entityType::Params params;\
	BUILDABLE_ENTITY_SET_PARAMS(params);

#define BUILDABLE_ENTITY_END(entityType)\
	ent->entity = new entityType(params);

/** Creates basic buildable entity of specific type. */
#define BUILDABLE_ENTITY_CREATE(entityType)\
	BUILDABLE_ENTITY_START(entityType);\
	BUILDABLE_ENTITY_END(entityType);

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

		case BA_H_ROCKETPOD: {
			BUILDABLE_ENTITY_CREATE(RocketpodEntity);
			break;
		}

		case BA_H_SPAWN: {
			BUILDABLE_ENTITY_CREATE(TelenodeEntity);
			break;
		}

		default:
			ASSERT_UNREACHABLE();
	}
}

static gentity_t *SpawnBuildable( gentity_t *builder, buildable_t buildable, const glm::vec3 &origin,
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

	built = G_NewEntity( HAS_CBSE );

	// ----------------------------
	// Set legacy fields below here
	// ----------------------------

	built->s.eType = entityType_t::ET_BUILDABLE;
	built->killedBy = ENTITYNUM_NONE;
	built->classname = attr->entityName;
	built->s.modelindex = buildable;
	built->s.modelindex2 = attr->team;
	built->buildableTeam = (team_t) built->s.modelindex2;
	BG_BuildableBoundingBox( buildable, built->r.mins, built->r.maxs );

	built->nextthink = level.time;
	built->enabled = false;
	built->spawned = false;

	built->flags = g_indestructibleBuildables.Get() ?
		FL_GODMODE | FL_NOTARGET : 0;

	built->s.time = built->creationTime;

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

	// HACK: These are preliminary angles. The real angles of the model are calculated client side.
	// TODO: Use proper angles with respect to the world?
	built->s.angles[ PITCH ] = 0.0f;
	built->s.angles[ YAW ]   = angles[ YAW ];
	built->s.angles[ ROLL ]  = angles[ ROLL ];

	// Turret angles.
	VectorClear(built->s.angles2);

	built->physicsBounce = attr->bounce;

	built->s.groundEntityNum = groundEntNum;
	if ( groundEntNum == ENTITYNUM_NONE )
	{
		built->s.pos.trType = attr->traj;
		built->s.pos.trTime = level.time;
		// gently nudge the buildable onto the surface :)
		VectorScale( normal, -50.0f, built->s.pos.trDelta );
	}

	VectorCopy( normal, built->s.origin2 );

	switch ( buildable )
	{
		case BA_A_SPAWN:
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
			break;

		case BA_A_TRAPPER:
			built->think = ATrapper_Think;
			break;

		case BA_A_LEECH:
			break;

		case BA_A_SPIKER:
			break;

		case BA_A_OVERMIND:
			break;

		case BA_H_SPAWN:
			break;

		case BA_H_MGTURRET:
			break;

		case BA_H_ROCKETPOD:
			break;

		case BA_H_ARMOURY:
			built->use = HArmoury_Use;
			break;

		case BA_H_MEDISTAT:
			built->think = HMedistat_Think;
			break;

		case BA_H_DRILL:
			break;

		case BA_H_REACTOR:
			break;

		default:
			break;
	}

	// ---------------------------------
	// Create the component based entity
	// ---------------------------------

	// Do this as late as possible so the component constructors can access legacy fields set above.
	BuildableSpawnCBSE(built, buildable);

	// -------------------------------------------------
	// Function calls that may use components below here
	// -------------------------------------------------

	// Start construction.
	if (builder->client) {
		if (g_instantBuilding.Get()) {
			// Build instantly in cheat mode.
			// HACK: This causes animation issues and can result in built->creationTime < 0.
			built->creationTime -= attr->buildTime;
		} else {
			HealthComponent *healthComponent = built->entity->Get<HealthComponent>();
			healthComponent->SetHealth(healthComponent->MaxHealth() * BUILDABLE_START_HEALTH_FRAC);
		}
	}

	// Free existing buildables
	G_FreeMarkedBuildables( builder, readable, sizeof( readable ), buildnums, sizeof( buildnums ) );

	// Add bot obstacles
	if ( built->r.maxs[2] - built->r.mins[2] > 47.0f ) // HACK: Fixed jump height
	{
		// HACK: work around bots unable to reach armory due to
		// navmesh margins in *some* situations.
		// fixVal is choosen to work on all known cases:
		// * 0.9 was not enough on urbanp2 but was on chasm
		// * 0.8 was enough on both
		// Note that said bug is not reported, but notably happens
		// on chasm with default layout as of 0.52.1
		glm::vec3 mins = VEC2GLM( built->r.mins );
		glm::vec3 maxs = VEC2GLM( built->r.maxs );
		constexpr float fixVal = 0.8;
		mins[0] *= fixVal; mins[1] *= fixVal;
		maxs[0] *= fixVal; maxs[1] *= fixVal;
		mins += origin;
		maxs += origin;
		G_BotAddObstacle( mins, maxs, built->num() );
	}

	G_AddEvent( built, EV_BUILD_CONSTRUCT, 0 );

	G_SetIdleBuildableAnim( built, BANIM_IDLE1 );
	G_SetBuildableAnim( built, BANIM_CONSTRUCT, true );

	trap_LinkEntity( built );

	if ( builder->client )
	{
	        // readable and the model name shouldn't need quoting
		G_TeamCommand( (team_t) builder->client->pers.team,
		               va( "print_tr %s %s %s %s", ( readable[ 0 ] ) ?
						QQ( N_("$1$ ^2built^* by $2$^*, ^3replacing^* $3$") ) :
						QQ( N_("$1$ ^2built^* by $2$$3$") ),
		                   Quote( BG_Buildable( built->s.modelindex )->humanName ),
		                   Quote( builder->client->pers.netname ),
		                   Quote( readable ) ) );
		G_LogPrintf( "Construct: %d %d %s%s: %s^* is building "
		             "%s%s%s",
		             builder->num(),
		             built->num(),
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
			SpawnBuildable( ent, buildable, VEC2GLM(origin), normal, ent->s.apos.trBase, groundEntNum );
			G_SpendBudget( BG_Buildable( buildable )->team, BG_Buildable( buildable )->buildPoints );
			return true;

		case IBE_NOALIENBP:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOBP );
			return false;

		case IBE_NOOVERMIND:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOOVMND );
			return false;

		case IBE_ONEOVERMIND:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_ONEOVERMIND );
			return false;

		case IBE_NOMORELEECHES:
			G_TriggerMenu( ent->client->ps.clientNum, MN_A_NOMORELEECHES );
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

		case IBE_NOREACTOR:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOREACTOR );
			return false;

		case IBE_NOMOREDRILLS:
			G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOMOREDRILLS );
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
	else if ( BG_Buildable( buildable )->traj == trType_t::TR_BUOYANCY )
	{
		VectorSet( normal, 0.0f, 0.0f, -1.0f );
	}
	else
	{
		VectorSet( normal, 0.0f, 0.0f, 1.0f );
	}

	built = SpawnBuildable( ent, buildable, VEC2GLM( ent->s.pos.trBase ), normal, ent->s.angles, ENTITYNUM_NONE );

	// This particular function is used by buildables that skip construction.
	built->entity->Get<BuildableComponent>()->SetState(BuildableComponent::CONSTRUCTED);

	/*built->spawned = true; //map entities are already spawned
	built->enabled = true;
	built->s.eFlags |= EF_B_SPAWNED;*/

	// drop towards normal surface
	VectorScale( built->s.origin2, -4096.0f, dest );
	VectorAdd( dest, built->s.origin, dest );

	trap_Trace( &tr, built->s.origin, built->r.mins, built->r.maxs, dest, built->num(),
	            built->clipmask, 0 );

	if ( tr.startsolid && !force )
	{
		Log::Debug( "^3G_FinishSpawningBuildable: %s startsolid at %s",
		          built->classname, vtos( built->s.origin ) );
		G_FreeEntity( built );
		return nullptr;
	}

	//point items in the correct direction
	VectorCopy( tr.plane.normal, built->s.origin2 );

	// allow to ride movers
	built->s.groundEntityNum = tr.entityNum;

	G_SetOrigin( built, VEC2GLM( tr.endpos ) );

	trap_LinkEntity( built );

	Beacon::Tag( built, (team_t)BG_Buildable( buildable )->team, true );

	return built;
}

static void SpawnBuildableThink( gentity_t *ent )
{
	FinishSpawningBuildable( ent, false );
	G_FreeEntity( ent );
}

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
		Log::Warn( "layoutsave: mapname is null" );
		return;
	}

	Com_sprintf( fileName, sizeof( fileName ), "layouts/%s/%s.dat", map, name );

	len = trap_FS_FOpenFile( fileName, &f, fsMode_t::FS_WRITE );

	if ( len < 0 )
	{
		Log::Warn( "layoutsave: could not open %s", fileName );
		return;
	}

	Log::Notice( "layoutsave: saving layout to %s", fileName );

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ )
	{
		ent = &level.gentities[ i ];

		if ( ent->s.eType != entityType_t::ET_BUILDABLE )
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

		ASSERT_GE(listLen, 4);
		// strip extension and add space delimiter
		layouts[ listLen - 4 ] = ' ';
		layouts[ listLen - 3 ] = '\0';
		count++;
	}

	if ( count != numFiles )
	{
		Log::Warn( "layout list was truncated to %d "
		          "layouts, but %d layout files exist in layouts/%s/.",
		          count, numFiles, map );
	}

	Q_strncpyz( list, layouts, len );
	return count + 1;
}

bool G_LayoutExists( Str::StringRef map, Str::StringRef layout )
{
	std::string path = Str::Format( "layouts/%s/%s.dat", map, layout );
	return FS::PakPath::FileExists( path ) || FS::HomePath::FileExists( path );
}

void G_LayoutSelect()
{
	char layouts[ MAX_CVAR_VALUE_STRING ];
	char layouts2[ MAX_CVAR_VALUE_STRING ];
	const char *layoutPtr;
	char map[ MAX_QPATH ];
	const char *layout;
	int  cnt = 0;
	int  layoutNum;

	Q_strncpyz( layouts, g_layouts.Get().c_str(), sizeof( layouts ) );
	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );

	// one time use cvar
	g_layouts.Set("");

	// pick from list of default layouts if provided
	if ( !layouts[ 0 ] )
	{
		Q_strncpyz( layouts, g_defaultLayouts.Get().c_str(), sizeof( layouts ) );
	}

	// pick an included layout at random if no list has been provided
	if ( !layouts[ 0 ] && g_layoutAuto.Get() )
	{
		G_LayoutList( map, layouts, sizeof( layouts ) );
	}

	// no layout specified
	if ( !layouts[ 0 ] )
	{
		//use default layout if available
		if ( G_LayoutExists( map, "default" ) )
		{
			strcpy( layouts, "default" );
		}
		else if ( G_LayoutExists( map, "builtin" ) )
		{
			strcpy( layouts, "builtin" );
		}
		else
		{
			//use the map's builtin layout
			Cvar::SetValue( "layout", "" );
			return;
		}
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

		if ( G_LayoutExists( map, layout ) )
		{
			Q_strcat( layouts, sizeof( layouts ), layout );
			Q_strcat( layouts, sizeof( layouts ), " " );
			cnt++;
		}
		else
		{
			Log::Warn( "Layout \"%s\" does not exist.", layout );
		}
	}

	if ( !cnt )
	{
		Log::Warn( "None of the specified layouts could be found, using map default." );
		Cvar::SetValue( "layout", "" );
		return;
	}

	layoutNum = ( rand() % cnt ) + 1;
	Q_strncpyz( layouts2, layouts, sizeof( layouts2 ) );
	layoutPtr = &layouts2[ 0 ];

	for ( cnt = 0; *(layout = COM_ParseExt( &layoutPtr, false )) && cnt < layoutNum; cnt++ )
	{
		Q_strncpyz( level.layout, layout, sizeof( level.layout ) );
	}

	Log::Notice( "Using layout \"%s\" from list (%s).", level.layout, layouts );
	trap_Cvar_Set( "layout", level.layout );
}

static void LayoutBuildItem( buildable_t buildable, vec3_t origin,
                             vec3_t angles, vec3_t origin2, vec3_t angles2 )
{
	gentity_t *builder;

	builder = G_NewEntity( NO_CBSE ); // this is a temp entity, not the actual buildable
	VectorCopy( origin, builder->s.pos.trBase );
	VectorCopy( angles, builder->s.angles );
	VectorCopy( origin2, builder->s.origin2 );
	VectorCopy( angles2, builder->s.angles2 );
	G_SpawnBuildable( builder, buildable );
}

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
	len = BG_FOpenGameOrPakPath( va( "layouts/%s/%s.dat", map, level.layout ), f );

	if ( len < 0 )
	{
		Log::Warn( "layout %s could not be opened", level.layout );
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
			Log::Warn( "line overflow in %s before \"%s\"",
			          va( "layouts/%s/%s.dat", map, level.layout ), line );
			break;
		}

		line[ i++ ] = *layout;
		line[ i ] = '\0';

		if ( *layout == '\n' )
		{
			i = 0;
			sscanf( line, "%1023s %f %f %f %f %f %f %f %f %f %f %f %f\n",
			        buildName,
			        &origin[ 0 ], &origin[ 1 ], &origin[ 2 ],
			        &angles[ 0 ], &angles[ 1 ], &angles[ 2 ],
			        &origin2[ 0 ], &origin2[ 1 ], &origin2[ 2 ],
			        &angles2[ 0 ], &angles2[ 1 ], &angles2[ 2 ] );

			attr = BG_BuildableByName( buildName );
			buildable = attr->number;

			if ( buildable <= BA_NONE || buildable >= BA_NUM_BUILDABLES )
			{
				Log::Warn( "bad buildable name (%s) in layout."
				          " skipping", buildName );
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

		if ( ent->s.eType != entityType_t::ET_BUILDABLE )
		{
			continue;
		}

		if ( ent->buildableTeam != team )
		{
			continue;
		}

		Entities::Kill(ent, MOD_SUICIDE);
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

static void G_BuildLogRevertThink( gentity_t *ent )
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

	G_LogPrintf( "revert: restore %d %s",
	             built->num(), BG_Buildable( built->s.modelindex )->name );

	G_FreeEntity( ent );
}

void G_BuildLogRevert( int id )
{
	buildLog_t *log;
	gentity_t  *ent;
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
			for ( int entityNum = MAX_CLIENTS; entityNum < level.num_entities; entityNum++ )
			{
				ent = &g_entities[ entityNum ];

				if ( ( ( ent->s.eType == entityType_t::ET_BUILDABLE && Entities::IsAlive( ent ) ) ||
					   ( ent->s.eType == entityType_t::ET_GENERAL && ent->think == G_BuildLogRevertThink ) ) &&
					 ent->s.modelindex == log->modelindex )
				{
					VectorSubtract( ent->s.pos.trBase, log->origin, dist );

					if ( VectorLengthSquared( dist ) <= 2.0f )
					{
						if ( ent->s.eType == entityType_t::ET_BUILDABLE )
						{
							G_LogPrintf( "revert: remove %d %s",
										 ent->num(), BG_Buildable( ent->s.modelindex )->name );

							// HACK: set max health to refund all BP and avoid rewarding attackers
							auto &health = *ent->entity->Get<HealthComponent>();
							health.SetHealth( health.MaxHealth() );

							G_Deconstruct( ent, nullptr, MOD_BUILDLOG_REVERT );
						}
						else
						{
							// Free a pseudo-entity (see the destruction case below)
							G_FreeBudget( log->buildableTeam, BG_Buildable( ent->s.modelindex )->buildPoints, 0 );
							momentumChange[ log->buildableTeam ] -= log->momentumEarned;
							G_FreeEntity( ent );
						}

						break;
					}
				}
			}
			break;

		case BF_DECONSTRUCT:
		case BF_REPLACE:
				momentumChange[ log->buildableTeam ] += log->momentumEarned;
				DAEMON_FALLTHROUGH;

		// Destruction. TODO: try to unqueue BP if applicable
		case BF_DESTROY:
		case BF_TEAMKILL:
		case BF_AUTO:
			G_SpendBudget( log->buildableTeam, BG_Buildable( log->modelindex )->buildPoints );

			// Spawn buildable
			// HACK: Uses legacy pseudo entity. TODO: CBSE-ify.
			buildable = G_NewEntity( NO_CBSE );
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

	for ( int team = TEAM_NONE + 1; team < NUM_TEAMS; ++team )
	{
		G_AddMomentumGenericStep( (team_t) team, momentumChange[ team ] );
	}

	G_AddMomentumEnd();
}
