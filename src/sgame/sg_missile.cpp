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

#include "common/Common.h"
#include "sg_local.h"
#include "sg_cm_world.h"
#include "Entities.h"
#include "CBSE.h"

// -----------
// definitions
// -----------

static Cvar::Cvar<int> g_abuild_blobFireImmunityDuration(
		"g_abuild_blobFireImmunityDuration",
		"duration of the immunity to fire from a granger spit (in milliseconds)",
		Cvar::NONE, 3000);
static Cvar::Cvar<int> g_abuild_blobFireExtinguishRange(
		"g_abuild_blobFireExtinguishRange",
		"range around which a granger spit extinguish fires on the ground",
		Cvar::NONE, 64);
static Cvar::Cvar<float> g_firefightReward(
		"g_firefightReward",
		"how much alien evos removing fire gives a granger",
		Cvar::NONE, 0.2f);

#define MISSILE_PRESTEP_TIME 50

enum missileTimePowerMod_t {
	MTPR_LINEAR_DECREASE,
	MTPR_LINEAR_INCREASE,
	MTPR_EXPONENTIAL_DECREASE // endTime is half time period, endMod is ignored
};

// -------------
// local methods
// -------------

static void BounceMissile( gentity_t *ent, const trace2_t *trace )
{
	vec3_t velocity;
	float  dot;
	int    hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta );

	if ( ent->s.eFlags & EF_BOUNCE_HALF )
	{
		VectorScale( ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta );

		// check for stop
		if ( trace->plane.normal[ 2 ] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 )
		{
			G_SetOrigin( ent, VEC2GLM( trace->endpos ) );
			return;
		}
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin );
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}

static float MissileTimePowerMod( gentity_t *self, missileTimePowerMod_t type,
                                  float startMod, float endMod, int startTime, int endTime )
{
	int   lifeTime, affectedTime;
	float fract;

	lifeTime = level.time - self->creationTime;

	if ( lifeTime <= startTime )
	{
		return startMod;
	}

	affectedTime = lifeTime - startTime; // > 0

	// sanity check endTime
	if ( endTime < 1 )
	{
		return startMod;
	}

	switch ( type )
	{
		case MTPR_LINEAR_DECREASE:
			// time is zero time
			fract = std::min( affectedTime, endTime ) / ( float )endTime;
			return startMod - ( startMod - endMod ) * fract;

		case MTPR_LINEAR_INCREASE:
			// time is full time
			fract = std::min( affectedTime, endTime ) / ( float )endTime;
			return startMod + ( endMod - startMod ) * fract;

		case MTPR_EXPONENTIAL_DECREASE:
			// arg is half life time, ln(2) ~= 0.6931472
			return startMod * exp( ( -0.6931472f * affectedTime ) / ( float )endTime );

		default:
			return startMod;
	}
}

static float MissileTimeDmgMod( gentity_t *self )
{
	switch ( self->s.weapon )
	{
		case MIS_FLAMER:
			return MissileTimePowerMod( self, MTPR_LINEAR_DECREASE, 1.0f, FLAMER_DAMAGE_MAXDST_MOD,
			                            0, BG_Missile( MIS_FLAMER )->lifetime );

		case MIS_LCANNON:
			return MissileTimePowerMod( self, MTPR_EXPONENTIAL_DECREASE, 1.0f, 0.0f,
			                            LCANNON_DAMAGE_FULL_TIME, LCANNON_DAMAGE_HALF_LIFE );

		case MIS_PRIFLE:
			return MissileTimePowerMod( self, MTPR_EXPONENTIAL_DECREASE, 1.0f, 0.0f,
			                            PRIFLE_DAMAGE_FULL_TIME, PRIFLE_DAMAGE_HALF_LIFE );
	}

	return 1.0f;
}

float G_MissileTimeSplashDmgMod( gentity_t *self )
{
	switch ( self->s.weapon )
	{
		case MIS_FLAMER:
			return MissileTimePowerMod( self, MTPR_LINEAR_INCREASE, FLAMER_SPLASH_MINDST_MOD, 1.0f,
										0, BG_Missile( MIS_FLAMER )->lifetime );
	}

	return 1.0f;
}

// Missile impact flags.
#define MIF_NO_DAMAGE   0x1 // Don't damage the entity we hit.
#define MIF_NO_EFFECT   0x2 // Don't turn into a hit effect.
#define MIF_NO_FREE     0x4 // Don't remove self (if not turning into an effect).

// Missile impact behaviours.
#define MIB_IMPACT      0 // Damage target and turn into hit effect.
#define MIB_FREE        ( MIF_NO_DAMAGE | MIF_NO_EFFECT ) // Quietly remove the missile.
#define MIB_BOUNCE      ( MIF_NO_DAMAGE | MIF_NO_EFFECT | MIF_NO_FREE ) // Continue flight.

static int ImpactGrenade( gentity_t *ent, const trace2_t *trace, gentity_t * )
{
	BounceMissile( ent, trace );

	if ( !( ent->s.eFlags & EF_NO_BOUNCE_SOUND ) )
	{
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
	}

	return MIB_BOUNCE;
}

static int ImpactFlamer( gentity_t *ent, const trace2_t *trace, gentity_t *hitEnt )
{
	gentity_t *neighbor = nullptr;

	// ignite on direct hit
	if ( random() < FLAMER_IGNITE_CHANCE )
	{
		hitEnt->entity->Ignite( ent->parent );
	}

	// ignite in radius
	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, VEC2GLM( trace->endpos ), FLAMER_IGNITE_RADIUS ) ) )
	{
		// we already handled other, since it might not always be in FLAMER_IGNITE_RADIUS due to BBOX sizes
		if ( neighbor == hitEnt )
		{
			continue;
		}

		if ( random() < FLAMER_IGNITE_SPLCHANCE )
		{
			neighbor->entity->Ignite( ent->parent );
		}
	}

	// set the environment on fire
	if ( hitEnt->num() == ENTITYNUM_WORLD )
	{
		if ( random() < FLAMER_LEAVE_FIRE_CHANCE )
		{
			G_SpawnFire( trace->endpos, trace->plane.normal, ent->parent );
		}
	}

	return MIB_IMPACT;
}

static int ImpactFirebombSub( gentity_t *ent, const trace2_t *trace, gentity_t *hitEnt )
{
	// ignite on direct hit
	hitEnt->entity->Ignite( ent->parent );

	// set the environment on fire
	if ( hitEnt->num() == ENTITYNUM_WORLD )
	{
		G_SpawnFire( trace->endpos, trace->plane.normal, ent->parent );
	}

	return MIB_IMPACT;
}

static int ImpactLockblock( gentity_t*, const trace2_t*, gentity_t *hitEnt )
{
	vec3_t dir;

	if ( hitEnt->client && hitEnt->client->pers.team == TEAM_HUMANS )
	{
		hitEnt->client->ps.stats[ STAT_STATE ] |= SS_BLOBLOCKED;
		hitEnt->client->lastLockTime = level.time;
		AngleVectors( hitEnt->client->ps.viewangles, dir, nullptr, nullptr );
		hitEnt->client->ps.stats[ STAT_VIEWLOCK ] = DirToByte( dir );
	}

	return MIB_IMPACT;
}

static int ImpactSlowblob( gentity_t *ent, const trace2_t *trace, gentity_t *hitEnt )
{
	int       impactFlags = MIB_IMPACT;
	gentity_t *attacker = &g_entities[ ent->r.ownerNum ];
	int reward = 0;

	// put out fires on direct hit
	if ( G_IsOnFire( hitEnt ) )
	{
		++reward;
		hitEnt->entity->Extinguish( g_abuild_blobFireImmunityDuration.Get() );
	}

	// put out fires in range
	gentity_t *neighbor = nullptr;
	// TODO: Iterate over all ignitable entities only
	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, VEC2GLM( trace->endpos ),
							    g_abuild_blobFireExtinguishRange.Get() ) ) )
	{
		// extinguish other entity on fire nearby,
		// and fires on ground
		if ( neighbor != hitEnt && G_IsOnFire( neighbor ) )
		{
			++reward;
			neighbor->entity->Extinguish( g_abuild_blobFireImmunityDuration.Get() );
		}
	}

	if ( reward )
	{
		// we extinguished some fire
		float credits = g_firefightReward.Get() * CREDITS_PER_EVO * reward;
		G_AddCreditToClient( attacker->client, credits, true );
		CombatFeedback::HitNotify( attacker, &g_entities[ENTITYNUM_NONE], {}, 0, MOD_SLOWBLOB, true );
	}

	if ( hitEnt->client && hitEnt->client->pers.team == TEAM_HUMANS )
	{
		hitEnt->client->ps.stats[ STAT_STATE ] |= SS_SLOWLOCKED;
		hitEnt->client->lastSlowTime = level.time;
	}
	else if ( hitEnt->s.eType == entityType_t::ET_BUILDABLE && hitEnt->buildableTeam == TEAM_ALIENS )
	{
		impactFlags |= MIF_NO_DAMAGE;
	}

	return impactFlags;
}

static int ImpactHive( gentity_t *, const trace2_t*, gentity_t *hitEnt )
{
	// Damage only humans and do so quietly.
	if ( hitEnt->client && hitEnt->client->pers.team == TEAM_HUMANS )
	{
		return MIF_NO_EFFECT;
	}
	else
	{
		return MIB_FREE;
	}
}

static int DefaultImpactFunc( gentity_t*, const trace2_t*, gentity_t* )
{
	return MIB_IMPACT;
}

// Returns true if the missile has ceased to exist
bool G_MissileImpact( gentity_t *ent, const trace2_t *trace )
{
	int       dirAsByte, impactFlags;
	const missileAttributes_t *ma = &ent->entity->Get<MissileComponent>()->Attributes();
	gentity_t *hitEnt   = &g_entities[ trace->entityNum ];
	gentity_t *attacker = &g_entities[ ent->r.ownerNum ];

	// Returns whether damage and hit effects should be done and played.
	int (*impactFunc)( gentity_t *, const trace2_t *, gentity_t * );

	// Check for bounce.
	if ( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) &&
	     !HasComponents<HealthComponent>(*hitEnt->entity) )
	{
		BounceMissile( ent, trace );

		if ( !( ent->s.eFlags & EF_NO_BOUNCE_SOUND ) )
		{
			G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		}

		return false;
	}

	// Call missile specific impact functions.
	switch( ent->s.modelindex )
	{
		case MIS_GRENADE:      impactFunc = ImpactGrenade;     break;
		case MIS_FIREBOMB:     impactFunc = ImpactGrenade;     break;
		case MIS_FLAMER:       impactFunc = ImpactFlamer;      break;
		case MIS_FIREBOMB_SUB: impactFunc = ImpactFirebombSub; break;
		case MIS_LOCKBLOB:     impactFunc = ImpactLockblock;   break;
		case MIS_SLOWBLOB:     impactFunc = ImpactSlowblob;    break;
		case MIS_HIVE:         impactFunc = ImpactHive;        break;
		default:               impactFunc = DefaultImpactFunc; break;
	}

	impactFlags = impactFunc( ent, trace, hitEnt );

	// Deal impact damage.
	if ( !( impactFlags & MIF_NO_DAMAGE ) )
	{
		if ( ma->damage && ( Entities::IsAlive( hitEnt ) || ( hitEnt && hitEnt->s.eType == entityType_t::ET_MOVER ) ) )
		{
			vec3_t dir;

			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, dir );

			if ( VectorNormalize( dir ) == 0 )
			{
				dir[ 2 ] = 1; // stepped on a grenade
			}

			int dflags = 0;
			if ( !ma->doLocationalDamage ) dflags |= DAMAGE_NO_LOCDAMAGE;
			if ( ma->doKnockback )         dflags |= DAMAGE_KNOCKBACK;

			hitEnt->Damage(ma->damage * MissileTimeDmgMod(ent), attacker,
			                       VEC2GLM( trace->endpos ), VEC2GLM( dir ), dflags,
			                       ma->meansOfDeath);
		}

		// splash damage (doesn't apply to person directly hit)
		if ( ma->splashDamage )
		{
			G_RadiusDamage( trace->endpos, ent->parent,
			                ma->splashDamage * G_MissileTimeSplashDmgMod( ent ),
			                ma->splashRadius, hitEnt, ( ma->doKnockback ? DAMAGE_KNOCKBACK : 0 ),
			                ma->splashMeansOfDeath );
		}
	}

	// Play hit effects and remove the missile.
	if ( !( impactFlags & MIF_NO_EFFECT ) )
	{
		// Use the surface normal for the hit event.
		dirAsByte = DirToByte( trace->plane.normal );

		// Add hit event.
		if ( HasComponents<HealthComponent>(*hitEnt->entity) )
		{
			G_AddEvent( ent, EV_MISSILE_HIT_ENTITY, dirAsByte );

			ent->s.otherEntityNum = hitEnt->num();
		}
		else if ( trace->surfaceFlags & SURF_METAL )
		{
			G_AddEvent( ent, EV_MISSILE_HIT_METAL, dirAsByte );
		}
		else
		{
			G_AddEvent( ent, EV_MISSILE_HIT_ENVIRONMENT, dirAsByte );
		}

		ent->freeAfterEvent = true;

		// HACK: Change over to a general entity at the point of impact.
		ent->s.eType = entityType_t::ET_GENERAL;

		// Prevent map models from appearing at impact point.
		ent->s.modelindex = 0;

		// Save net bandwidth.
		glm::vec3 snappedPosition = VEC2GLM( trace->endpos );
		G_SnapVectorTowards( &snappedPosition[ 0 ], ent->s.pos.trBase );
		G_SetOrigin( ent, snappedPosition );

		trap_LinkEntity( ent );
		return true;
	}
	// If no impact happened, check if we should continue or free ourselves.
	else if ( !( impactFlags & MIF_NO_FREE ) )
	{
		ent->entity->FreeAt( DeferredFreeingComponent::FREE_AFTER_THINKING );
		return true;
	}
	else
	{
		return false;
	}
}

void G_SetUpMissile( gentity_t *m, gentity_t *parent, const vec3_t start, const vec3_t dir )
{
	vec3_t                    velocity;
	const missileAttributes_t *ma = &m->entity->Get<MissileComponent>()->Attributes();

	// generic
	m->s.eType             = entityType_t::ET_MISSILE;
	m->s.modelindex        = ma->number;
	m->r.ownerNum          = parent->num();
	m->parent              = parent;

	// from attribute config file
	m->s.weapon            = ma->number;
	m->classname           = ma->name;
	m->clipmask            = ma->clipmask;
	BG_MissileBounds( ma, m->r.mins, m->r.maxs );
	m->s.eFlags            = ma->flags;

	// trajectory
	{
		// set trajectory type
		m->s.pos.trType = ma->trajectoryType;

		// move a bit on the first frame
		m->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;

		// set starting point
		VectorCopy( start, m->s.pos.trBase );
		VectorCopy( start, m->r.currentOrigin );

		// set speed
		VectorScale( dir, ma->speed, velocity );

		// add lag
		if ( ma->lag && parent->client )
		{
			VectorMA( velocity, ma->lag, parent->client->ps.velocity, velocity );
		}

		// copy velocity
		VectorCopy( velocity, m->s.pos.trDelta );

		// save net bandwidth
		SnapVector( m->s.pos.trDelta );
	}
}

// a non-guided missile
gentity_t *G_SpawnDumbMissile( missile_t missile, gentity_t *parent, const glm::vec3 &start, const glm::vec3 &dir )
{
	gentity_t *m = G_NewEntity( HAS_CBSE );
	DumbMissileEntity::Params params;
	params.oldEnt = m;
	params.Missile_attributes = BG_Missile( missile );
	m->entity = new DumbMissileEntity{ params };
	G_SetUpMissile( m, parent, GLM4READ( start ), GLM4READ( dir ) );
	return m;
}

/*
===============
G_SpawnFire
===============
*/
gentity_t *G_SpawnFire( const vec3_t origin, const vec3_t normal, gentity_t *fireStarter )
{
	gentity_t *fire;
	vec3_t    snapHelper, floorNormal;

	VectorSet( floorNormal, 0.0f, 0.0f, 1.0f );

	// don't spawn fire on walls and ceiling since we can't display it properly yet
	// TODO: Add fire effects for floor and ceiling
	if ( DotProduct( normal, floorNormal ) < 0.71f ) // 0.71 ~= cos(45°)
	{
		return nullptr;
	}

	// don't spawn a fire inside another fire
	fire = nullptr;
	while ( ( fire = G_IterateEntitiesWithinRadius( fire, VEC2GLM( origin ), FIRE_MIN_DISTANCE ) ) )
	{
		if ( fire->s.eType == entityType_t::ET_FIRE )
		{
			return nullptr;
		}
	}

	fire = G_NewEntity( HAS_CBSE );

	// create a fire entity
	fire->classname = "fire";
	fire->s.eType   = entityType_t::ET_FIRE;
	fire->clipmask  = 0;

	fire->entity = new FireEntity(FireEntity::Params{fire});
	fire->entity->Ignite(fireStarter);

	// attacker
	fire->r.ownerNum = fireStarter->num();

	// normal
	ASSERT( VectorLength( normal ) > 0.999f && VectorLength( normal ) < 1.001f );
	VectorCopy( normal, fire->s.origin2 );

	// origin
	VectorCopy( origin, fire->s.origin );
	VectorAdd( origin, normal, snapHelper );
	G_SnapVectorTowards( fire->s.origin, snapHelper ); // save net bandwidth
	VectorCopy( fire->s.origin, fire->r.currentOrigin );

	// send to client
	trap_LinkEntity( fire );

	if ( g_debugFire.Get() )
	{
		char descr[ 64 ];
		BG_BuildEntityDescription( descr, sizeof( descr ), &fire->s );
		Log::Notice("%s spawned.", descr);
	}

	return fire;
}
