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

// sg_weapon.c
// perform the server side effects of a weapon firing

#include "sg_local.h"
#include "Entities.h"
#include "CBSE.h"
#include "sg_cm_world.h"

static glm::vec3 forward, right, up;
static glm::vec3 muzzle;

static void SendHitEvent( gentity_t *attacker, gentity_t *target, glm::vec3 const& origin, glm::vec3 const&  normal, entity_event_t evType );

static bool TakesDamages( gentity_t const* ent )
{
	return false
		|| Entities::IsAlive( ent )
		|| ( ent && ent->s.eType == entityType_t::ET_MOVER && ent->health > 0 )
		;
}

void G_ForceWeaponChange( gentity_t *ent, weapon_t weapon )
{
	playerState_t *ps = &ent->client->ps;

	if ( weapon == WP_NONE || !BG_InventoryContainsWeapon( weapon, ps->stats ) )
	{
		// switch to the first non blaster weapon
		ps->persistant[ PERS_NEWWEAPON ] = BG_PrimaryWeapon( ps->stats );
	}
	else
	{
		ps->persistant[ PERS_NEWWEAPON ] = weapon;
	}

	// force this here to prevent flamer effect from continuing
	ps->generic1 = WPM_NOTFIRING;

	// The PMove will do an animated drop, raise, and set the new weapon
	ps->pm_flags |= PMF_WEAPON_SWITCH;
}

/**
 * @brief Refills spare ammo clips.
 */
static void GiveMaxClips( gentity_t *self )
{
	playerState_t *ps;
	const weaponAttributes_t *wa;

	if ( !self || !self->client )
	{
		return;
	}

	self->client->lastAmmoRefillTime = level.time;

	ps = &self->client->ps;
	wa = BG_Weapon( ps->stats[ STAT_WEAPON ] );

	ps->clips = wa->maxClips;
}

/**
 * @brief Refills current ammo clip/charge.
 */
static void GiveFullClip( gentity_t *self )
{
	playerState_t *ps;
	const weaponAttributes_t *wa;

	if ( !self || !self->client )
	{
		return;
	}

	self->client->lastAmmoRefillTime = level.time;

	ps = &self->client->ps;
	wa = BG_Weapon( ps->stats[ STAT_WEAPON ] );

	ps->ammo = wa->maxAmmo;
}

/**
 * @brief Refills both spare clips and current clip/charge.
 */
void G_GiveMaxAmmo( gentity_t *self )
{
	GiveMaxClips( self );
	GiveFullClip( self );
}

/**
 * @brief Checks the condition for G_RefillAmmo.
 */
static bool CanUseAmmoRefill( gentity_t *self )
{
	const weaponAttributes_t *wa;
	playerState_t *ps;

	if ( !self || !self->client )
	{
		return false;
	}

	ps = &self->client->ps;
	wa = BG_Weapon( ps->stats[ STAT_WEAPON ] );

	if ( wa->infiniteAmmo )
	{
		return false;
	}

	if ( wa->maxClips == 0 )
	{
		// clipless weapons can be refilled whenever they lack ammo
		return ( ps->ammo != wa->maxAmmo );
	}
	else if ( ps->clips != wa->maxClips )
	{
		// clip weapons have to miss a clip to be refillable
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @brief Refills clips on clip based weapons, refills charge on other weapons.
 * @param self
 * @param triggerEvent Trigger an event when relevant resource was modified.
 * @return Whether relevant resource was modified.
 */
bool G_RefillAmmo( gentity_t *self, bool triggerEvent )
{
	if ( !CanUseAmmoRefill( self ) )
	{
		return false;
	}

	if ( BG_Weapon( self->client->ps.stats[ STAT_WEAPON ] )->maxClips > 0 )
	{
		GiveMaxClips( self );

		if ( triggerEvent )
		{
			G_AddEvent( self, EV_CLIPS_REFILL, 0 );
		}
	}
	else
	{
		GiveFullClip( self );

		if ( triggerEvent )
		{
			G_AddEvent( self, EV_AMMO_REFILL, 0 );
		}
	}

	return true;
}

/**
 * @brief Refills jetpack fuel.
 * @param self
 * @param triggerEvent Trigger an event when fuel was modified.
 * @return Whether fuel was modified.
 */
bool G_RefillFuel( gentity_t *self, bool triggerEvent )
{
	if ( !self || !self->client )
	{
		return false;
	}

	// needs a human with jetpack
	if ( G_Team( self) != TEAM_HUMANS ||
	     !BG_InventoryContainsUpgrade( UP_JETPACK, self->client->ps.stats ) )
	{
		return false;
	}

	if ( self->client->ps.stats[ STAT_FUEL ] != JETPACK_FUEL_MAX )
	{
		self->client->ps.stats[ STAT_FUEL ] = JETPACK_FUEL_MAX;

		self->client->lastFuelRefillTime = level.time;

		if ( triggerEvent )
		{
			G_AddEvent( self, EV_FUEL_REFILL, 0 );
		}

		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @brief Attempts to refill ammo from a close source.
 * @return Whether ammo was refilled.
 */
bool G_FindAmmo( gentity_t *self )
{
	gentity_t *neighbor = nullptr;
	bool  foundSource = false;

	// don't search for a source if refilling isn't possible
	if ( !CanUseAmmoRefill( self ) )
	{
		return false;
	}

	// search for ammo source
	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, VEC2GLM( self->s.origin ), ENTITY_USE_RANGE ) ) )
	{
		// only friendly, living and powered buildables provide ammo
		if ( neighbor->s.eType != entityType_t::ET_BUILDABLE || !G_OnSameTeam( self, neighbor ) ||
		     !neighbor->spawned || !neighbor->powered || Entities::IsDead( neighbor ) )
		{
			continue;
		}

		switch ( neighbor->s.modelindex )
		{
			case BA_H_ARMOURY:
				foundSource = true;
				break;

			case BA_H_DRILL:
			case BA_H_REACTOR:
				if ( BG_Weapon( self->client->ps.stats[ STAT_WEAPON ] )->usesEnergy )
				{
					foundSource = true;
				}
				break;
		}
	}

	if ( foundSource )
	{
		return G_RefillAmmo( self, true );
	}

	return false;
}

/**
 * @brief Attempts to refill jetpack fuel from a close source.
 * @return true if fuel was refilled.
 */
bool G_FindFuel( gentity_t *self )
{
	gentity_t *neighbor = nullptr;
	bool  foundSource = false;

	if ( !self || !self->client )
	{
		return false;
	}

	// search for fuel source
	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, VEC2GLM( self->s.origin ), ENTITY_USE_RANGE ) ) )
	{
		// only friendly, living and powered buildables provide fuel
		if ( neighbor->s.eType != entityType_t::ET_BUILDABLE || !G_OnSameTeam( self, neighbor ) ||
		     !neighbor->spawned || !neighbor->powered || Entities::IsDead( neighbor ) )
		{
			continue;
		}

		switch ( neighbor->s.modelindex )
		{
			case BA_H_ARMOURY:
				foundSource = true;
				break;
		}
	}

	if ( foundSource )
	{
		return G_RefillFuel( self, true );
	}

	return false;
}

/*
================
Trace a bounding box against entities, but not the world
Also check there is a line of sight between the start and end point
================
*/
static trace_t G_WideTrace( gentity_t *ent, float range,
                         float width, float height, gentity_t **target )
{
	trace_t tr;
	*target = nullptr;

	if ( !ent->client )
	{
		return tr;
	}

	// Calculate box to use for trace
	glm::vec3 maxs = { width, width, height };
	glm::vec3 mins = -maxs;
	float halfDiagonal = glm::length( maxs );

	G_UnlaggedOn( ent, muzzle, range + halfDiagonal );

	// Trace box against entities
	glm::vec3 end = muzzle + range * forward;
	G_CM_Trace( &tr, muzzle, mins, maxs, end, ent->s.number, CONTENTS_BODY, 0, traceType_t::TT_AABB );

	if ( tr.entityNum != ENTITYNUM_NONE )
	{
		*target = &g_entities[ tr.entityNum ];
	}

	// Line trace against the world, so we never hit through obstacles.
	// The range is reduced according to the former trace so we don't hit something behind the
	// current target.
	end = muzzle + ( glm::distance( muzzle, VEC2GLM( tr.endpos ) ) + halfDiagonal ) * forward;
	G_CM_Trace( &tr, muzzle, glm::vec3(), glm::vec3(), end, ent->s.number, CONTENTS_SOLID, 0, traceType_t::TT_AABB );

	// In case we hit a different target, which can happen if two potential targets are close,
	// switch to it, so we will end up with the target we were looking at.
	if ( tr.entityNum != ENTITYNUM_NONE )
	{
		*target = &g_entities[ tr.entityNum ];
	}

	G_UnlaggedOff();
	return tr;
}

/*
======================
Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
void G_SnapVectorTowards( glm::vec3& v, glm::vec3 const& to )
{
	for ( int i = 0; i < 3; i++ )
	{
		if ( v[ i ] >= 0 )
		{
			v[ i ] = static_cast<int>( v[ i ] + ( to[ i ] <= v[ i ] ? 0 : 1 ) );
		}
		else
		{
			v[ i ] = static_cast<int>( v[ i ] + ( to[ i ] <= v[ i ] ? -1 : 0 ) );
		}
	}
}

static void SendRangedHitEvent( gentity_t *attacker, gentity_t *target, trace_t *tr )
{
	// snap the endpos to integers, but nudged towards the line
	glm::vec3 tmp = VEC2GLM( tr->endpos );
	G_SnapVectorTowards( tmp, muzzle );

	entity_event_t evType = HasComponents<HealthComponent>(*target->entity) ? EV_WEAPON_HIT_ENTITY : EV_WEAPON_HIT_ENVIRONMENT;
	SendHitEvent( attacker, target, tmp, VEC2GLM( tr->plane.normal ), evType );
	VectorCopy( tmp, tr->endpos );
}

static void SendHitEvent( gentity_t *attacker, gentity_t *target, glm::vec3 const& origin, glm::vec3 const&  normal, entity_event_t evType )
{
	gentity_t *event = G_NewTempEntity( origin, evType );

	// normal
	event->s.eventParm = DirToByte( &normal[0] );

	// victim
	event->s.otherEntityNum = target->s.number;

	// attacker
	event->s.otherEntityNum2 = attacker->s.number;

	// weapon
	event->s.weapon = attacker->s.weapon;

	// weapon mode
	event->s.generic1 = attacker->s.generic1;
}

static void SendMeleeHitEvent( gentity_t *attacker, gentity_t *target, trace_t *tr )
{
	if ( !attacker->client )
	{
		return;
	}

	//tyrant charge attack do not have traces... there must be a better way for that...
	glm::vec3 normal = VEC2GLM( tr ? tr->endpos : attacker->client->ps.origin ) - VEC2GLM( target->s.origin );

	// Normalize the horizontal components of the vector difference to the "radius" of the bounding box
	float mag = sqrtf( normal[ 0 ] * normal[ 0 ] + normal[ 1 ] * normal[ 1 ] );
	float radius = target->r.maxs[ 0 ] * 1.21f;
	if ( mag > radius )
	{
		normal[ 0 ] = normal[ 0 ] / mag * radius;
		normal[ 1 ] = normal[ 1 ] / mag * radius;
	}
	normal[ 2 ] = Math::Clamp( normal[ 2 ], target->r.mins[ 2 ], target->r.maxs[ 2 ] );

	glm::vec3 origin = VEC2GLM( target->s.origin ) + normal;
	normal = glm::normalize( -normal );

	SendHitEvent( attacker, target, origin, normal, EV_WEAPON_HIT_ENTITY );
}

static gentity_t *FireMelee( gentity_t *self )
{
	weapon_t wp = BG_PrimaryWeapon( self->client->ps.stats );
	weaponAttributes_t const* wpa = BG_Weapon( wp );
	ASSERT( wpa );
	float range = wpa->range;
	float width = wpa->width;
	float height = wpa->height;
	int damage = wpa->damage;
	gentity_t *traceEnt;

	trace_t tr = G_WideTrace( self, range, width, height, &traceEnt );

	if ( not TakesDamages( traceEnt ) )
	{
		return nullptr;
	}

	traceEnt->Damage( damage, self, VEC2GLM( tr.endpos ), forward, 0, wpa->meansOfDeath );

	// for painsaw. This makes really little sense to me, but this is refactoring, not bugsquashing.
	if ( wp == WP_PAIN_SAW )
	{
		SendRangedHitEvent( self, traceEnt, &tr );
	}
	else
	{
		SendMeleeHitEvent( self, traceEnt, &tr );
	}

	if ( wp == WP_ALEVEL1 )
	{
		if ( traceEnt && traceEnt->client )
		{
			traceEnt->client->ps.stats[ STAT_STATE2 ] |= SS2_LEVEL1SLOW;
			traceEnt->client->lastLevel1SlowTime = level.time;
		}
	}
	return traceEnt;
}

/*
======================================================================

MACHINEGUN

======================================================================
*/

static void FireBullet( gentity_t *self, float spread, float damage, meansOfDeath_t mod )
{
	int other = 0;
	if ( self->client )
	{
		weapon_t wp = BG_PrimaryWeapon( self->client->ps.stats );
		weaponAttributes_t const* wpa = BG_Weapon( wp );

		if ( spread < 0 )
		{
			spread = wpa->spread;
		}
		if ( damage < 0 )
		{
			damage = wpa->damage;
		}
		mod = wpa->meansOfDeath;
		other |= wpa->doKnockback ? DAMAGE_KNOCKBACK : 0;
	}
	ASSERT( damage > 0 && mod != MOD_UNKNOWN );

	trace_t   tr;
	glm::vec3 end = muzzle + 8192.f * 16.f * forward;
	if ( spread > 0.f )
	{
		float r = random() * M_PI * 2.0f;
		float u = sinf( r ) * crandom() * spread * 16;
		r = cosf( r ) * crandom() * spread * 16;
		end += r * right;
		end += u * up;
	}

	// don't use unlagged if this is not a client (e.g. turret)
	if ( self->client )
	{
		G_UnlaggedOn( self, muzzle, 8192 * 16 );
		G_CM_Trace( &tr, muzzle, glm::vec3(), glm::vec3(), end, self->s.number, MASK_SHOT, 0, traceType_t::TT_AABB );
		G_UnlaggedOff();
	}
	else
	{
		G_CM_Trace( &tr, muzzle, glm::vec3(), glm::vec3(), end, self->s.number, MASK_SHOT, 0, traceType_t::TT_AABB );
	}

	if ( tr.surfaceFlags & SURF_NOIMPACT )
	{
		return;
	}

	gentity_t *target = &g_entities[ tr.entityNum ];
	SendRangedHitEvent( self, target, &tr );
	target->Damage(damage, self, VEC2GLM( tr.endpos ), forward, other, mod);
}

// spawns a missile at parent's muzzle going in forward dir
// missile: missile type
// target: if not nullptr, missile will automatically aim at target
// think: callback giving missile's behavior
// thinkDelta: number of ms between each call to think()
// lifetime: how long missile will be active
// fromTip: fire from the tip, not the center (whatever the point might be, used by hive)
//
// This function uses those additional variables:
// * muzzle
// * forward
// * level.time
static void FireMissile( gentity_t* self, missile_t missile, gentity_t* target, void (*think)( gentity_t* ), int thinkDelta, int lifetime, bool fromTip )
{
	glm::vec3 origin = muzzle;
	if ( fromTip )
	{
		origin += self->r.maxs[2] * VEC2GLM( self->s.origin2 );
	}

	gentity_t *m = G_SpawnMissile( missile, self, origin, forward, target , think, level.time + thinkDelta );
	if ( lifetime )
	{
		m->timestamp = level.time + lifetime;
	}
}

/*
======================================================================

SHOTGUN

======================================================================
*/

/*
================
Keep this in sync with ShotgunPattern in CGAME!
================
*/
static void ShotgunPattern( glm::vec3 const& origin, glm::vec3 const& origin2, int seed, gentity_t *self )
{
	weapon_t wp = BG_PrimaryWeapon( self->client->ps.stats );
	weaponAttributes_t const* wpa = BG_Weapon( wp );

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	glm::vec3 forward = glm::normalize( origin2 );
	glm::vec3 right;
	PerpendicularVector( &right[0], &forward[0] );
	glm::vec3 up = glm::cross( forward, right );

	// generate the "random" spread pattern
	for ( int i = 0; i < SHOTGUN_PELLETS; i++ )
	{
		float r = Q_crandom( &seed ) * M_PI;
		float a = Q_random( &seed ) * wpa->spread * 16;

		float u = sinf( r ) * a;
		r = cosf( r ) * a;

		glm::vec3 end = origin + wpa->range * forward;
		end += r * right;
		end += u * up;

		trace_t tr;
		G_CM_Trace( &tr, origin, glm::vec3(), glm::vec3(), end, self->s.number, MASK_SHOT, 0, traceType_t::TT_AABB );
		g_entities[ tr.entityNum ].entity->Damage( wpa->damage, self, VEC2GLM( tr.endpos ), forward, 0, wpa->meansOfDeath );
	}
}

static void FireShotgun( gentity_t *self ) //TODO merge with FireBullet
{
	weapon_t wp = BG_PrimaryWeapon( self->client->ps.stats );
	weaponAttributes_t const* wpa = BG_Weapon( wp );
	gentity_t *tent;

	// instead of an EV_WEAPON_HIT_* event, send this so client can generate the same spread pattern
	tent = G_NewTempEntity( muzzle, EV_SHOTGUN );
	VectorScale( forward, 4096, tent->s.origin2 );
	SnapVector( tent->s.origin2 );
	tent->s.eventParm = rand() / ( RAND_MAX / 0x100 + 1 ); // seed for spread pattern
	tent->s.otherEntityNum = self->s.number;

	// calculate the pattern and do the damage
	G_UnlaggedOn( self, muzzle, wpa->range );
	ShotgunPattern( VEC2GLM( tent->s.pos.trBase ), VEC2GLM( tent->s.origin2 ), tent->s.eventParm, self );
	G_UnlaggedOff();
}

/*
======================================================================

HIVE

======================================================================
*/

/*
================
Target tracking for the hive missile.
================
*/
static void HiveMissileThink( gentity_t *self )
{
	trace_t   tr;
	gentity_t *ent;
	int       i;
	float     d, nearest;

	if ( level.time > self->timestamp ) // swarm lifetime exceeded
	{
		VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
		self->s.pos.trType = trType_t::TR_STATIONARY;
		self->s.pos.trTime = level.time;

		self->think = G_ExplodeMissile;
		self->nextthink = level.time + 50;
		self->parent->hiveInsectsActive = false; //allow the parent to start again
		return;
	}

	nearest = DistanceSquared( self->r.currentOrigin, self->target->r.currentOrigin );

	//find the closest human
	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		ent = &g_entities[ i ];

		if ( !ent->inuse ) continue;
		if ( ent->flags & FL_NOTARGET ) continue;

		if ( ent->client && Entities::IsAlive( ent ) && G_Team( ent ) == TEAM_HUMANS &&
		     nearest > ( d = DistanceSquared( ent->r.currentOrigin, self->r.currentOrigin ) ) )
		{
			G_CM_Trace( &tr, VEC2GLM( self->r.currentOrigin ), VEC2GLM( self->r.mins ), VEC2GLM( self->r.maxs ), VEC2GLM( ent->r.currentOrigin ), self->r.ownerNum, self->clipmask, 0, traceType_t::TT_AABB );

			if ( tr.entityNum != ENTITYNUM_WORLD )
			{
				nearest = d;
				self->target = ent;
			}
		}
	}

	glm::vec3 dir = VEC2GLM( self->target->r.currentOrigin ) - VEC2GLM( self->r.currentOrigin );
	dir = glm::normalize( dir ) * HIVE_SPEED;

	//change direction towards the player
	VectorCopy( dir, self->s.pos.trDelta );
	SnapVector( self->s.pos.trDelta );  // save net bandwidth
	VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
	self->s.pos.trTime = level.time;

	self->nextthink = level.time + HIVE_DIR_CHANGE_PERIOD;
}

/*
======================================================================

ROCKET POD

======================================================================
*/

static void RocketThink( gentity_t *self )
{
	if ( level.time > self->timestamp )
	{
		self->think     = G_ExplodeMissile;
		self->nextthink = level.time;

		return;
	}

	self->nextthink = level.time + ROCKET_TURN_PERIOD;

	// Don't turn anymore if the target is dead or gone
	if ( !self->target )
	{
		return;
	}

	// Calculate current and target direction.
	glm::vec3 currentOrigin = VEC2GLM( self->r.currentOrigin );
	glm::vec3 currentDir = glm::normalize( VEC2GLM( self->s.pos.trDelta ) );
	glm::vec3 targetDir = VEC2GLM( self->target->r.currentOrigin ) - currentOrigin;
	targetDir = glm::normalize( targetDir );

	// Don't turn anymore after the target was passed.
	if ( glm::dot( currentDir, targetDir ) < 0 )
	{
		return;
	}

	// Calculate new direction. Use a fixed turning angle.
	glm::vec3 rotAxis = glm::cross( currentDir, targetDir );
	float  rotAngle = RAD2DEG( acosf( glm::dot( currentDir, targetDir ) ) );
	glm::vec3 newDir;
	RotatePointAroundVector( &newDir[0], &rotAxis[0], &currentDir[0],
	                         Math::Clamp( rotAngle, -ROCKET_TURN_ANGLE, ROCKET_TURN_ANGLE ) );

	// Check if new direction is safe. Turn anyway if old direction is unsafe, too.
	if ( !RocketpodComponent::SafeShot( ENTITYNUM_NONE,  currentOrigin, newDir )
			&& RocketpodComponent::SafeShot( ENTITYNUM_NONE, currentOrigin, currentDir )
	)
	{
		return;
	}

	// Update trajectory.
	VectorScale( newDir, BG_Missile( self->s.modelindex )->speed, self->s.pos.trDelta );
	SnapVector( self->s.pos.trDelta );
	VectorCopy( self->r.currentOrigin, self->s.pos.trBase ); // TODO: Snap this, too?
	self->s.pos.trTime = level.time;
}

/*
======================================================================

FIREBOMB

======================================================================
*/

#define FIREBOMB_SUBMISSILE_COUNT 15
#define FIREBOMB_IGNITE_RANGE     192
#define FIREBOMB_TIMER            4000

static void FirebombMissileThink( gentity_t *self )
{
	// ignite alien buildables in range
	gentity_t* neighbor = nullptr;
	while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, VEC2GLM( self->s.origin ), FIREBOMB_IGNITE_RANGE ) ) )
	{
		if ( neighbor->s.eType == entityType_t::ET_BUILDABLE && G_Team( neighbor ) == TEAM_ALIENS &&
		     G_LineOfSight( self, neighbor ) )
		{
			neighbor->entity->Ignite( self->parent );
		}
	}

	// set floor below on fire (assumes the firebomb lays on the floor!)
	G_SpawnFire( VEC2GLM( self->s.origin ), glm::vec3( 0.f, 0.f, 1.f ), self->parent );

	// spam fire
	for ( int subMissileNum = 0; subMissileNum < FIREBOMB_SUBMISSILE_COUNT; subMissileNum++ )
	{
		glm::vec3 dir =
		{
			( rand() / static_cast<float>( RAND_MAX ) ) - 0.5f,
			( rand() / static_cast<float>( RAND_MAX ) ) - 0.5f,
			( rand() / static_cast<float>( RAND_MAX ) ) * 0.5f,
		};

		dir = glm::normalize( dir );

		// the submissile's parent is the attacker
		gentity_t* m = G_SpawnMissile( MIS_FIREBOMB_SUB, self->parent, VEC2GLM( self->s.origin ), dir, nullptr, G_FreeEntity, level.time + 10000 );

		// randomize missile speed
		VectorScale( m->s.pos.trDelta, ( rand() / ( float )RAND_MAX ) + 0.5f, m->s.pos.trDelta );
	}

	// explode
	G_ExplodeMissile( self );
}

/*
======================================================================

LUCIFER CANNON

======================================================================
*/

static gentity_t *FireLcannonHelper( gentity_t *self, glm::vec3 const start, glm::vec3 const& dir,
                                     int damage, int radius, int speed )
{
	// TODO: Tidy up this and lcannonFire
	gentity_t *m;
	int nextthink = level.time;

	// explode in front of player when overcharged
	if ( damage != LCANNON_DAMAGE )
	{
		nextthink += 10000;
	}

	if ( self->s.generic1 == WPM_PRIMARY )
	{
		m = G_SpawnMissile( MIS_LCANNON, self, start, dir, nullptr, G_ExplodeMissile, nextthink );

		// some values are set in the code
		m->damage       = damage;
		m->splashDamage = damage / 2;
		m->splashRadius = radius;
		VectorScale( dir, speed, m->s.pos.trDelta );
		SnapVector( m->s.pos.trDelta ); // save net bandwidth

		// pass the missile charge through
		float charge = static_cast<float>( damage - LCANNON_SECONDARY_DAMAGE ) / LCANNON_DAMAGE;

		m->s.torsoAnim = charge * 255;

		if ( m->s.torsoAnim < 0 )
		{
			m->s.torsoAnim = 0;
		}
	}
	else
	{
		m = G_SpawnMissile( MIS_LCANNON2, self, start, dir, nullptr, G_ExplodeMissile, nextthink );
	}

	return m;
}

static void FireLcannon( gentity_t *self, bool secondary )
{
	if ( secondary && self->client->ps.weaponCharge <= 0 )
	{
		FireLcannonHelper( self, muzzle, forward, LCANNON_SECONDARY_DAMAGE,
		                   LCANNON_SECONDARY_RADIUS, LCANNON_SECONDARY_SPEED );
	}
	else
	{
		FireLcannonHelper( self, muzzle, forward,
		                   self->client->ps.weaponCharge * LCANNON_DAMAGE / LCANNON_CHARGE_TIME_MAX,
		                   LCANNON_RADIUS, LCANNON_SPEED );
	}

	self->client->ps.weaponCharge = 0;
}

/*
======================================================================

BUILD GUN

======================================================================
*/

void G_CheckCkitRepair( gentity_t *self )
{
	if ( self->client->ps.weaponTime > 0 ||
	     self->client->ps.stats[ STAT_MISC ] > 0 )
	{
		return;
	}

	glm::vec3 viewOrigin = BG_GetClientViewOrigin( &self->client->ps );
	glm::vec3 forward;
	AngleVectors( VEC2GLM( self->client->ps.viewangles ), &forward, nullptr, nullptr );
	glm::vec3 end = viewOrigin + 100.f * forward;

	trace_t tr;
	G_CM_Trace( &tr, viewOrigin, glm::vec3(), glm::vec3(), end, self->s.number, MASK_PLAYERSOLID, 0, traceType_t::TT_AABB );
	gentity_t *traceEnt = &g_entities[ tr.entityNum ];

	if ( tr.fraction < 1.0f && traceEnt->spawned && traceEnt->s.eType == entityType_t::ET_BUILDABLE &&
	     G_Team( traceEnt ) == TEAM_HUMANS )
	{
		HealthComponent const *healthComponent = traceEnt->entity->Get<HealthComponent>();

		if ( healthComponent && healthComponent->Alive() && !healthComponent->FullHealth() )
		{
			traceEnt->entity->Heal( HBUILD_HEALRATE, nullptr );

			G_AddEvent( self, healthComponent->FullHealth() ? EV_BUILD_REPAIRED : EV_BUILD_REPAIR, 0 );
			self->client->ps.weaponTime += BG_Weapon( self->client->ps.weapon )->repeatRate1;
		}
	}
}

static void CancelBuild( gentity_t *self )
{
	// Cancel ghost buildable
	if ( self->client->ps.stats[ STAT_BUILDABLE ] != BA_NONE )
	{
		self->client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;
		self->client->ps.stats[ STAT_PREDICTION ] = 0;
		return;
	}

	if ( self->client->ps.weapon == WP_ABUILD ||
	     self->client->ps.weapon == WP_ABUILD2 )
	{
		FireMelee( self );
	}
}

// do the same thing as /deconstruct
static void FireMarkDeconstruct( gentity_t *self )
{
	gentity_t* buildable = G_GetDeconstructibleBuildable( self );
	if ( buildable == nullptr )
	{
		return;
	}
	if ( G_DeconstructDead( buildable ) )
	{
		return;
	}
	buildable->entity->Get<BuildableComponent>()->ToggleDeconstructionMark();
}

static void DeconstructSelectTarget( gentity_t *self )
{
	gentity_t* target = G_GetDeconstructibleBuildable( self );
	if ( target == nullptr // No target found
		|| G_DeconstructDead( target ) // Successfully deconned dead target
		|| G_CheckDeconProtectionAndWarn( target, self ) ) // Not allowed to decon target
	{
		self->target = nullptr;
		// Stop the force-deconstruction charge bar
		self->client->pmext.cancelDeconstructCharge = true;
	}
	else
	{
		// Set the target which will be used upon reaching full charge
		self->target = target;
	}
}

static void FireForceDeconstruct( gentity_t *self )
{
	if ( !self->target )
	{
		return;
	}
	gentity_t* target = self->target.get();
	glm::vec3 viewOrigin = BG_GetClientViewOrigin( &self->client->ps );
	// The builder must still be in a range such that G_GetDeconstructibleBuildable could return
	// the buildable (but with 10% extra distance allowed).
	// However it is not necessary to be aiming at the target.
	if ( G_DistanceToBBox( viewOrigin, target ) > BUILDER_DECONSTRUCT_RANGE * 1.1f )
	{
		// Target is too far away.
		// TODO: Continuously check that target is valid (in range and still exists), rather
		// than only at the beginning and end of the charge.
		return;
	}

	if ( G_DeconstructDead( self->target.get() ) )
	{
		return;
	}
	G_DeconstructUnprotected( self->target.get(), self );
}

static void FireBuild( gentity_t *self, dynMenu_t menu )
{
	buildable_t buildable;

	if ( !self->client )
	{
		return;
	}

	buildable = (buildable_t) ( self->client->ps.stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK );

	// open build menu
	if ( buildable <= BA_NONE )
	{
		G_TriggerMenu( self->num(), menu );
		return;
	}

	// can't build just yet
	if ( self->client->ps.stats[ STAT_MISC ] > 0 )
	{
		G_AddEvent( self, EV_BUILD_DELAY, self->num() );
		return;
	}

	// build
	if ( G_BuildIfValid( self, buildable ) )
	{
		if ( !g_instantBuilding.Get() )
		{
			int buildTime = BG_Buildable( buildable )->buildTime;

			switch ( G_Team( self ) )
			{
				case TEAM_ALIENS:
					buildTime *= ALIEN_BUILDDELAY_MOD;
					break;

				case TEAM_HUMANS:
					buildTime *= HUMAN_BUILDDELAY_MOD;
					break;

				default:
					break;
			}

			self->client->ps.stats[ STAT_MISC ] += buildTime;
		}

		self->client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;
	}
}

/*
======================================================================

LEVEL0

======================================================================
*/

bool G_CheckDretchAttack( gentity_t *self )
{
	gentity_t *traceEnt;

	if ( self->client->ps.weaponTime )
	{
		return false;
	}

	// Calculate muzzle point
	AngleVectors( VEC2GLM( self->client->ps.viewangles ), &forward, nullptr, nullptr );
	muzzle = G_CalcMuzzlePoint( self, forward );

	weaponAttributes_t const* wpa = BG_Weapon( BG_PrimaryWeapon( self->client->ps.stats ) );
	trace_t tr = G_WideTrace( self, wpa->range, wpa->width, wpa->height, &traceEnt );

	//this is ugly, but so is all that mess in any case. I'm just trying to fix shit here, so go complain to whoever broke the game, not to me.

	if ( not TakesDamages( traceEnt )
				|| G_OnSameTeam( self, traceEnt )
				|| !G_DretchCanDamageEntity( self, traceEnt ) )
	{
		return false;
	}

	traceEnt->entity->Damage( wpa->damage, self, VEC2GLM( tr.endpos ), forward, 0, wpa->meansOfDeath );

	SendMeleeHitEvent( self, traceEnt, &tr );

	self->client->ps.weaponTime += wpa->repeatRate1;

	return true;
}

/*
======================================================================

LEVEL2

======================================================================
*/
#define MAX_ZAPS MAX_CLIENTS

static zap_t zaps[ MAX_ZAPS ];

static void FindZapChainTargets( zap_t *zap )
{
	gentity_t *ent = zap->targets[ 0 ]; // the source
	int       entityList[ MAX_GENTITIES ];

	glm::vec3 range = { LEVEL2_AREAZAP_CHAIN_RANGE, LEVEL2_AREAZAP_CHAIN_RANGE, LEVEL2_AREAZAP_CHAIN_RANGE };
	glm::vec3 origin = VEC2GLM( ent->s.origin );
	glm::vec3 maxs = origin + range;
	glm::vec3 mins = origin - range;

	int num = G_CM_AreaEntities( mins, maxs, entityList, MAX_GENTITIES );

	for ( int i = 0; i < num; i++ )
	{
		gentity_t *enemy = &g_entities[ entityList[ i ] ];

		// don't chain to self; noclippers can be listed, don't chain to them either
		if ( enemy == ent || ( enemy->client && enemy->client->noclip ) )
		{
			continue;
		}

		float distance = glm::distance( origin, VEC2GLM( enemy->s.origin ) );

		//TODO: implement support for map-entities
		if ( G_Team( enemy ) == TEAM_HUMANS
				&& ( enemy->client || enemy->s.eType == entityType_t::ET_BUILDABLE )
				&& Entities::IsAlive( enemy )
				&& distance <= LEVEL2_AREAZAP_CHAIN_RANGE )
		{
			// world-LOS check: trace against the world, ignoring other BODY entities
			trace_t tr;
			G_CM_Trace( &tr, origin, glm::vec3(), glm::vec3(), VEC2GLM( enemy->s.origin ), ent->s.number, CONTENTS_SOLID, 0, traceType_t::TT_AABB );

			if ( tr.entityNum == ENTITYNUM_NONE )
			{
				zap->targets[ zap->numTargets ] = enemy;
				zap->distances[ zap->numTargets ] = distance;

				if ( ++zap->numTargets >= LEVEL2_AREAZAP_MAX_TARGETS )
				{
					return;
				}
			}
		}
	}
}

static void UpdateZapEffect( zap_t *zap )
{
	int i;
	int entityNums[ LEVEL2_AREAZAP_MAX_TARGETS + 1 ];

	entityNums[ 0 ] = zap->creator->s.number;

	ASSERT_LE(zap->numTargets, LEVEL2_AREAZAP_MAX_TARGETS);

	for ( i = 0; i < zap->numTargets; i++ )
	{
		entityNums[ i + 1 ] = zap->targets[ i ]->s.number;
	}

	BG_PackEntityNumbers( &zap->effectChannel->s,
	                      entityNums, zap->numTargets + 1 );


	G_SetOrigin( zap->effectChannel, muzzle );
	G_CM_LinkEntity( zap->effectChannel );
}

static void CreateNewZap( gentity_t *creator, gentity_t *target )
{
	int   i;
	zap_t *zap;

	for ( i = 0; i < MAX_ZAPS; i++ )
	{
		zap = &zaps[ i ];

		if ( zap->used )
		{
			continue;
		}

		zap->used = true;
		zap->timeToLive = LEVEL2_AREAZAP_TIME;

		zap->creator = creator;
		zap->targets[ 0 ] = target;
		zap->numTargets = 1;

		// Zap chains only originate from alive entities.
		if (target->Damage((float)LEVEL2_AREAZAP_DMG, creator, VEC2GLM( target->s.origin ),
		                           forward, DAMAGE_NO_LOCDAMAGE, MOD_LEVEL2_ZAP)) {
			FindZapChainTargets( zap );

			for ( i = 1; i < zap->numTargets; i++ )
			{
				float damage = LEVEL2_AREAZAP_DMG * ( 1 - powf( ( zap->distances[ i ] /
				               LEVEL2_AREAZAP_CHAIN_RANGE ), LEVEL2_AREAZAP_CHAIN_FALLOFF ) ) + 1;

				zap->targets[i]->Damage(damage, zap->creator, VEC2GLM( zap->targets[i]->s.origin ),
				                       forward, DAMAGE_NO_LOCDAMAGE, MOD_LEVEL2_ZAP);
			}
		}

		zap->effectChannel = G_NewEntity( NO_CBSE );
		zap->effectChannel->s.eType = entityType_t::ET_LEV2_ZAP_CHAIN;
		zap->effectChannel->classname = "lev2zapchain";
		UpdateZapEffect( zap );

		return;
	}
}

void G_UpdateZaps( int msec )
{
	int   i, j;
	zap_t *zap;

	for ( i = 0; i < MAX_ZAPS; i++ )
	{
		zap = &zaps[ i ];

		if ( !zap->used )
		{
			continue;
		}

		zap->timeToLive -= msec;

		// first, the disappearance of players is handled immediately in G_ClearPlayerZapEffects()

		// the deconstruction or gibbing of a directly targeted buildable destroys the whole zap effect
		if ( zap->timeToLive <= 0 || !zap->targets[ 0 ]->inuse )
		{
			G_FreeEntity( zap->effectChannel );
			zap->used = false;
			continue;
		}

		// the deconstruction or gibbing of chained buildables destroy the appropriate beams
		for ( j = 1; j < zap->numTargets; j++ )
		{
			if ( !zap->targets[ j ]->inuse )
			{
				zap->targets[ j-- ] = zap->targets[ --zap->numTargets ];
			}
		}

		UpdateZapEffect( zap );
	}
}

/*
===============
Called from G_LeaveTeam() and TeleportPlayer().
===============
*/
void G_ClearPlayerZapEffects( gentity_t *player )
{
	int   i, j;
	zap_t *zap;

	for ( i = 0; i < MAX_ZAPS; i++ )
	{
		zap = &zaps[ i ];

		if ( !zap->used )
		{
			continue;
		}

		// the disappearance of the creator or the first target destroys the whole zap effect
		if ( zap->creator == player || zap->targets[ 0 ] == player )
		{
			G_FreeEntity( zap->effectChannel );
			zap->used = false;
			continue;
		}

		// the disappearance of chained players destroy the appropriate beams
		for ( j = 1; j < zap->numTargets; j++ )
		{
			if ( zap->targets[ j ] == player )
			{
				zap->targets[ j-- ] = zap->targets[ --zap->numTargets ];
			}
		}
	}
}

static void FireAreaZap( gentity_t *ent )
{
	gentity_t *traceEnt;

	G_WideTrace( ent, LEVEL2_AREAZAP_RANGE, LEVEL2_AREAZAP_WIDTH, LEVEL2_AREAZAP_WIDTH, &traceEnt );

	if ( G_Team( traceEnt ) == TEAM_HUMANS &&
			( traceEnt->client || traceEnt->s.eType == entityType_t::ET_BUILDABLE ) )
	{
		CreateNewZap( ent, traceEnt );
	}
}

/*
======================================================================

LEVEL3

======================================================================
*/

bool G_CheckPounceAttack( gentity_t *self )
{
	gentity_t *traceEnt;
	int       damage, timeMax, pounceRange, payload;

	if ( self->client->pmext.pouncePayload <= 0 )
	{
		return false;
	}

	// In case the goon lands on his target, he gets one shot after landing
	payload = self->client->pmext.pouncePayload;

	if ( !( self->client->ps.pm_flags & PMF_CHARGE ) )
	{
		self->client->pmext.pouncePayload = 0;
	}

	// Calculate muzzle point
	AngleVectors( VEC2GLM( self->client->ps.viewangles ), &forward, nullptr, nullptr );
	muzzle = G_CalcMuzzlePoint( self, forward );

	// Trace from muzzle to see what we hit
	pounceRange = self->client->ps.weapon == WP_ALEVEL3 ? LEVEL3_POUNCE_RANGE :
	              LEVEL3_POUNCE_UPG_RANGE;
	trace_t tr = G_WideTrace( self, pounceRange, LEVEL3_POUNCE_WIDTH,
	             LEVEL3_POUNCE_WIDTH, &traceEnt );

	if ( not TakesDamages( traceEnt ) )
	{
		return false;
	}

	timeMax = self->client->ps.weapon == WP_ALEVEL3 ? LEVEL3_POUNCE_TIME : LEVEL3_POUNCE_TIME_UPG;
	damage  = payload * LEVEL3_POUNCE_DMG / timeMax;

	self->client->pmext.pouncePayload = 0;

	traceEnt->Damage((float)damage, self, VEC2GLM( tr.endpos ), forward,
	                         DAMAGE_NO_LOCDAMAGE, MOD_LEVEL3_POUNCE);

	SendMeleeHitEvent( self, traceEnt, &tr );

	return true;
}

/*
======================================================================

LEVEL4

======================================================================
*/

void G_ChargeAttack( gentity_t *self, gentity_t *victim )
{
	if ( !self->client || self->client->ps.weaponCharge <= 0 ||
	     !( self->client->ps.stats[ STAT_STATE ] & SS_CHARGING ) ||
	     self->client->ps.weaponTime )
	{
		return;
	}

	if ( not TakesDamages( victim ) )
	{
		return;
	}

	glm::vec3 forward = VEC2GLM( victim->s.origin ) - VEC2GLM( self->s.origin );
	forward = glm::normalize( forward );

	// For buildables, track the last MAX_TRAMPLE_BUILDABLES_TRACKED buildables
	//  hit, and do not do damage if the current buildable is in that list
	//  in order to prevent dancing over stuff to kill it very quickly
	if ( !victim->client )
	{
		for ( int i = 0; i < MAX_TRAMPLE_BUILDABLES_TRACKED; i++ )
		{
			if ( self->client->trampleBuildablesHit[ i ] == victim->num() )
			{
				return;
			}
		}

		self->client->trampleBuildablesHit[
		  self->client->trampleBuildablesHitPos++ % MAX_TRAMPLE_BUILDABLES_TRACKED ] =
		    victim->num();
	}

	int damage = LEVEL4_TRAMPLE_DMG * self->client->ps.weaponCharge / LEVEL4_TRAMPLE_DURATION;

	victim->Damage( static_cast<float>( damage ), self, VEC2GLM( victim->s.origin ), forward,
	                       DAMAGE_NO_LOCDAMAGE, MOD_LEVEL4_TRAMPLE);

	SendMeleeHitEvent( self, victim, nullptr );

	self->client->ps.weaponTime += LEVEL4_TRAMPLE_REPEAT;
}

/*
======================================================================

GENERIC

======================================================================
*/

static meansOfDeath_t ModWeight( const gentity_t *self )
{
	return G_Team( self ) == TEAM_HUMANS ? MOD_WEIGHT_H : MOD_WEIGHT_A;
}

void G_ImpactAttack( gentity_t *self, gentity_t *victim )
{
	// self must be a client
	if ( !self->client )
	{
		return;
	}

	// don't do friendly fire
	if ( G_OnSameTeam( self, victim ) )
	{
		return;
	}

	// attacker must be above victim
	if ( self->client->ps.origin[ 2 ] + self->r.mins[ 2 ] <
	     victim->s.origin[ 2 ] + victim->r.maxs[ 2 ] )
	{
		return;
	}

	// allow the granger airlifting ritual
	if ( victim->client && victim->client->ps.stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE &&
	     ( self->client->pers.classSelection == PCL_ALIEN_BUILDER0 ||
	       self->client->pers.classSelection == PCL_ALIEN_BUILDER0_UPG ) )
	{
		return;
	}

	// calculate impact damage
	float impactVelocity = fabs( self->client->pmext.fallImpactVelocity[ 2 ] ) * QU_TO_METER; // in m/s

	if ( !impactVelocity )
	{
		return;
	}

	int attackerMass = BG_Class( self->client->pers.classSelection )->mass;
	float impactEnergy = attackerMass * impactVelocity * impactVelocity; // in J
	float impactDamage = impactEnergy * IMPACTDMG_JOULE_TO_DAMAGE;

	// calculate knockback direction
	glm::vec3 knockbackDir = VEC2GLM( victim->s.origin ) - VEC2GLM( self->client->ps.origin );
	knockbackDir = glm::normalize( knockbackDir );

	victim->Damage( impactDamage, self, VEC2GLM( victim->s.origin ),
						   knockbackDir, DAMAGE_NO_LOCDAMAGE, ModWeight(self) );
}

void G_WeightAttack( gentity_t *self, gentity_t *victim )
{
	float  weightDPS, weightDamage;
	int    attackerMass, victimMass;

	// weight damage is only dealt between clients
	if ( !self->client || !victim->client )
	{
		return;
	}

	// don't do friendly fire
	if ( G_OnSameTeam( self, victim ) )
	{
		return;
	}

	// attacker must be above victim
	if ( self->client->ps.origin[ 2 ] + self->r.mins[ 2 ] <
	     victim->s.origin[ 2 ] + victim->r.maxs[ 2 ] )
	{
		return;
	}

	// victim must be on the ground
	if ( victim->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{
		return;
	}

	// check timer
	if ( victim->client->nextCrushTime > level.time )
	{
		return;
	}

	attackerMass = BG_Class( self->client->pers.classSelection )->mass;
	victimMass = BG_Class( victim->client->pers.classSelection )->mass;
	weightDPS = WEIGHTDMG_DMG_MODIFIER * std::max( attackerMass - victimMass, 0 );

	if ( weightDPS > WEIGHTDMG_DPS_THRESHOLD )
	{
		weightDamage = weightDPS * ( WEIGHTDMG_REPEAT / 1000.0f );

		victim->Damage(weightDamage, self, VEC2GLM( victim->s.origin ), Util::nullopt,
		                       DAMAGE_NO_LOCDAMAGE, ModWeight(self));
	}

	victim->client->nextCrushTime = level.time + WEIGHTDMG_REPEAT;
}

//======================================================================

/*
===============
Set muzzle location relative to pivoting eye.
===============
*/
glm::vec3 G_CalcMuzzlePoint( const gentity_t *self, glm::vec3 const& forward )
{
	glm::vec3 muzzlePoint = VEC2GLM( self->client->ps.origin );
	glm::vec3 normal = BG_GetClientNormal( &self->client->ps );

	muzzlePoint += static_cast<float>( self->client->ps.viewheight ) * normal;
	muzzlePoint += forward;

	// snap to integer coordinates for more efficient network bandwidth usage
	return glm::floor( muzzlePoint + 0.5f );
}

void G_FireWeapon( gentity_t *self, weapon_t weapon, weaponMode_t weaponMode )
{
	// calculate muzzle
	if ( self->client )
	{
		AngleVectors( VEC2GLM( self->client->ps.viewangles ), &forward, nullptr, nullptr );
		muzzle = G_CalcMuzzlePoint( self, forward );
	}
	else
	{
		AngleVectors( self->buildableAim, &forward, nullptr, nullptr );
		muzzle = VEC2GLM( self->s.pos.trBase );
	}

	switch ( weaponMode )
	{
		case WPM_PRIMARY:
		{
			switch ( weapon )
			{
				case WP_ALEVEL1:
				case WP_ALEVEL2:
				case WP_ALEVEL2_UPG:
				case WP_ALEVEL3:
				case WP_ALEVEL3_UPG:
				case WP_ALEVEL4:
				case WP_PAIN_SAW:
					FireMelee( self );
					break;

				case WP_BLASTER:
					FireMissile( self, MIS_BLASTER, nullptr, G_ExplodeMissile, 10000, 0, false );
					break;

				case WP_MACHINEGUN:
					FireBullet( self, -1.f, -1.f, MOD_UNKNOWN );
					break;

				case WP_SHOTGUN:
					FireShotgun( self );
					break;

				case WP_CHAINGUN:
					FireBullet( self, -1.f, -1.f, MOD_UNKNOWN );
					break;

				case WP_FLAMER:
					FireMissile( self, MIS_FLAMER, nullptr, G_FreeEntity, FLAMER_LIFETIME, 0, false );
					break;

				case WP_PULSE_RIFLE:
					FireMissile( self, MIS_PRIFLE, nullptr, G_ExplodeMissile, 10000, 0, false );
					break;

				case WP_MASS_DRIVER:
					FireBullet( self, 0.f, -1, MOD_UNKNOWN );
					break;

				case WP_LUCIFER_CANNON:
					FireLcannon( self, false );
					break;

				case WP_LAS_GUN:
					FireBullet( self, 0.f, -1, MOD_UNKNOWN );
					break;

				case WP_LOCKBLOB_LAUNCHER:
					FireMissile( self, MIS_LOCKBLOB, nullptr, G_ExplodeMissile, 15000, 0, false );
					break;

				case WP_HIVE:
					FireMissile( self, MIS_HIVE, self->target.get(), HiveMissileThink, HIVE_DIR_CHANGE_PERIOD, HIVE_LIFETIME, true );
					break;

				case WP_ROCKETPOD:
					FireMissile( self, MIS_ROCKET, self->target.get(), RocketThink, ROCKET_TURN_PERIOD, ROCKET_LIFETIME, false );
					break;

				case WP_MGTURRET:
					FireBullet( self, MGTURRET_SPREAD, self->turretCurrentDamage, MOD_MGTURRET );
					break;

				case WP_ABUILD:
				case WP_ABUILD2:
					FireBuild( self, MN_A_BUILD );
					break;

				case WP_HBUILD:
					FireBuild( self, MN_H_BUILD );
					break;

				default:
					break;
			}
			break;
		}
		case WPM_SECONDARY:
		{
			switch ( weapon )
			{
				case WP_LUCIFER_CANNON:
					FireLcannon( self, true );
					break;

				case WP_ALEVEL2_UPG:
					FireAreaZap( self );
					break;

				case WP_ABUILD:
				case WP_ABUILD2:
				case WP_HBUILD:
					CancelBuild( self );
					break;

				default:
					break;
			}
			break;
		}
		case WPM_TERTIARY:
		{
			switch ( weapon )
			{
				case WP_ALEVEL3_UPG:
					FireMissile( self, MIS_BOUNCEBALL, nullptr, G_ExplodeMissile, 3000, 0, false );
					break;

				case WP_ABUILD2:
					FireMissile( self, MIS_SLOWBLOB, nullptr, G_ExplodeMissile, 15000, 0, false );
					break;

				default:
					break;
			}
			break;
		}
		case WPM_DECONSTRUCT:
		{
			switch ( weapon )
			{
				case WP_ABUILD:
				case WP_ABUILD2:
				case WP_HBUILD:
					FireMarkDeconstruct( self );
					break;

				default:
					break;
			}
			break;
		}
		case WPM_DECONSTRUCT_SELECT_TARGET:
		{
			switch ( weapon )
			{
				case WP_ABUILD:
				case WP_ABUILD2:
				case WP_HBUILD:
					DeconstructSelectTarget( self );
					break;

				default:
					break;
			}
			break;
		}
		case WPM_DECONSTRUCT_LONG:
		{
			switch ( weapon )
			{
				case WP_ABUILD:
				case WP_ABUILD2:
				case WP_HBUILD:
					FireForceDeconstruct( self );
					break;

				default:
					break;
			}
			break;
		}
		default:
		{
			break;
		}
	}

}

void G_FireUpgrade( gentity_t *self, upgrade_t upgrade )
{
	if ( !self || !self->client )
	{
		Log::Warn( "G_FireUpgrade: Called with non-player parameter." );
		return;
	}

	if ( upgrade <= UP_NONE || upgrade >= UP_NUM_UPGRADES )
	{
		Log::Warn( "G_FireUpgrade: Called with unknown upgrade." );
		return;
	}

	AngleVectors( VEC2GLM( self->client->ps.viewangles ), &forward, nullptr, nullptr );
	muzzle = G_CalcMuzzlePoint( self, forward );

	switch ( upgrade )
	{
		case UP_GRENADE:
			FireMissile( self, MIS_GRENADE, nullptr, G_ExplodeMissile, 5000, 0, false );
			break;
		case UP_FIREBOMB:
			FireMissile( self, MIS_FIREBOMB, nullptr, FirebombMissileThink, FIREBOMB_TIMER, 0, false );
			break;
		default:
			break;
	}

	switch ( upgrade )
	{
		case UP_GRENADE:
		case UP_FIREBOMB:
			trap_SendServerCommand( self->num(), "vcommand grenade" );
			break;

		default:
			break;
	}
}
