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

// g_weapon.c
// perform the server side effects of a weapon firing

#include "g_local.h"

static  vec3_t forward, right, up;
static  vec3_t muzzle;

void G_ForceWeaponChange( gentity_t *ent, weapon_t weapon )
{
	playerState_t *ps = &ent->client->ps;

	// stop a reload in progress
	if ( ps->weaponstate == WEAPON_RELOADING )
	{
		ps->torsoAnim = ( ( ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | TORSO_RAISE;
		ps->weaponTime = 250;
		ps->weaponstate = WEAPON_READY;
	}

	if ( weapon == WP_NONE ||
	     !BG_InventoryContainsWeapon( weapon, ps->stats ) )
	{
		// switch to the first non blaster weapon
		ps->persistant[ PERS_NEWWEAPON ] =
		  BG_PrimaryWeapon( ent->client->ps.stats );
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

void G_GiveClientMaxAmmo( gentity_t *ent, qboolean buyingEnergyAmmo )
{
	int      i, maxAmmo, maxClips;
	qboolean restoredAmmo = qfalse, restoredEnergy = qfalse;

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		qboolean energyWeapon;

		energyWeapon = BG_Weapon( i )->usesEnergy;

		if ( !BG_InventoryContainsWeapon( i, ent->client->ps.stats ) ||
		     BG_Weapon( i )->infiniteAmmo ||
		     BG_WeaponIsFull( i, ent->client->ps.stats,
		                      ent->client->ps.ammo, ent->client->ps.clips ) ||
		     ( buyingEnergyAmmo && !energyWeapon ) )
		{
			continue;
		}

		maxAmmo = BG_Weapon( i )->maxAmmo;
		maxClips = BG_Weapon( i )->maxClips;

		// Apply battery pack modifier
		if ( energyWeapon &&
		     BG_InventoryContainsUpgrade( UP_BATTPACK, ent->client->ps.stats ) )
		{
			maxAmmo *= BATTPACK_MODIFIER;
			restoredEnergy = qtrue;
		}

		ent->client->ps.ammo = maxAmmo;
		ent->client->ps.clips = maxClips;

		restoredAmmo = qtrue;
	}

	if ( restoredAmmo )
	{
		G_ForceWeaponChange( ent, ent->client->ps.weapon );
	}

	if ( restoredEnergy )
	{
		G_AddEvent( ent, EV_RPTUSE_SOUND, 0 );
	}
}

void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout )
{
	vec3_t v, newv;
	float  dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2 * dot, dir, newv );

	VectorNormalize( newv );
	VectorMA( impact, 8192, newv, endout );
}

/*
================
Trace a bounding box against entities, but not the world
Also check there is a line of sight between the start and end point
================
*/
static void G_WideTrace( trace_t *tr, gentity_t *ent, float range,
                         float width, float height, gentity_t **target )
{
	vec3_t mins, maxs;
	vec3_t end;

	VectorSet( mins, -width, -width, -height );
	VectorSet( maxs, width, width, height );

	*target = NULL;

	if ( !ent->client )
	{
		return;
	}

	G_UnlaggedOn( ent, muzzle, range + width );

	VectorMA( muzzle, range, forward, end );

	// Trace against entities
	trap_Trace( tr, muzzle, mins, maxs, end, ent->s.number, CONTENTS_BODY );

	if ( tr->entityNum != ENTITYNUM_NONE )
	{
		*target = &g_entities[ tr->entityNum ];
	}

	// Set range to the trace length plus the width, so that the end of the
	// LOS trace is close to the exterior of the target's bounding box
	range = Distance( muzzle, tr->endpos ) + width;
	VectorMA( muzzle, range, forward, end );

	// Trace for line of sight against the world
	trap_Trace( tr, muzzle, NULL, NULL, end, ent->s.number, CONTENTS_SOLID );

	if ( tr->entityNum != ENTITYNUM_NONE )
	{
		*target = &g_entities[ tr->entityNum ];
	}

	G_UnlaggedOff();
}

/*
======================
Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
void G_SnapVectorTowards( vec3_t v, vec3_t to )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		if ( v[ i ] >= 0 )
		{
			v[ i ] = ( int )( v[ i ] + ( to[ i ] <= v[ i ] ? 0 : 1 ) );
		}
		else
		{
			v[ i ] = ( int )( v[ i ] + ( to[ i ] <= v[ i ] ? -1 : 0 ) );
		}
	}
}

/*
======================
Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
static void SnapVectorNormal( vec3_t v, vec3_t normal )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		if ( v[ i ] >= 0 )
		{
			v[ i ] = ( int )( v[ i ] + ( normal[ i ] <= 0 ? 0 : 1 ) );
		}
		else
		{
			v[ i ] = ( int )( v[ i ] + ( normal[ i ] <= 0 ? -1 : 0 ) );
		}
	}
}

/*
===============
Calculates the position of a blood spurt for wide traces and generates an event.
Used by melee attacks.
===============
*/
static void WideBloodSpurt( gentity_t *attacker, gentity_t *victim, trace_t *tr )
{
	gentity_t *tent;
	vec3_t    normal, origin;
	float     mag, radius;

	if ( !attacker->client )
	{
		return;
	}

	if ( victim->health <= 0 )
	{
		return;
	}

	if ( tr )
	{
		VectorSubtract( tr->endpos, victim->s.origin, normal );
	}
	else
	{
		VectorSubtract( attacker->client->ps.origin,
		                victim->s.origin, normal );
	}

	// Normalize the horizontal components of the vector difference to the
	// "radius" of the bounding box
	mag = sqrt( normal[ 0 ] * normal[ 0 ] + normal[ 1 ] * normal[ 1 ] );
	radius = victim->r.maxs[ 0 ] * 1.21f;

	if ( mag > radius )
	{
		normal[ 0 ] = normal[ 0 ] / mag * radius;
		normal[ 1 ] = normal[ 1 ] / mag * radius;
	}

	// Clamp origin to be within bounding box vertically
	if ( normal[ 2 ] > victim->r.maxs[ 2 ] )
	{
		normal[ 2 ] = victim->r.maxs[ 2 ];
	}

	if ( normal[ 2 ] < victim->r.mins[ 2 ] )
	{
		normal[ 2 ] = victim->r.mins[ 2 ];
	}

	VectorAdd( victim->s.origin, normal, origin );
	VectorNegate( normal, normal );
	VectorNormalize( normal );

	// send weapon hit event for actual blood effect
	tent = G_NewTempEntity( origin, EV_WEAPON_HIT_ENTITY );
	tent->s.eventParm = DirToByte( normal );
	tent->s.otherEntityNum = victim->s.number;
	tent->s.otherEntityNum2 = attacker->s.number;
	tent->s.weapon = attacker->s.weapon;
	tent->s.generic1 = attacker->s.generic1; // weaponMode
}

static void FireMelee( gentity_t *ent, float range, float width, float height,
                         int damage, meansOfDeath_t mod )
{
	trace_t   tr;
	gentity_t *traceEnt;

	G_WideTrace( &tr, ent, range, width, height, &traceEnt );

	if ( traceEnt == NULL || !traceEnt->takedamage )
	{
		return;
	}

	WideBloodSpurt( ent, traceEnt, &tr );

	G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, mod );
}

/*
======================================================================

MACHINEGUN

======================================================================
*/

static void FireBullet( gentity_t *ent, float spread, int damage, int mod )
{
	// TODO: Merge this with other *Fire functions

	trace_t   tr;
	vec3_t    end;
	float     r;
	float     u;
	gentity_t *tent;
	gentity_t *traceEnt;

	r = random() * M_PI * 2.0f;
	u = sin( r ) * crandom() * spread * 16;
	r = cos( r ) * crandom() * spread * 16;
	VectorMA( muzzle, 8192 * 16, forward, end );
	VectorMA( end, r, right, end );
	VectorMA( end, u, up, end );

	// don't use unlagged if this is not a client (e.g. turret)
	if ( ent->client )
	{
		G_UnlaggedOn( ent, muzzle, 8192 * 16 );
		trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
		G_UnlaggedOff();
	}
	else
	{
		trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
	}

	if ( tr.surfaceFlags & SURF_NOIMPACT )
	{
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// snap the endpos to integers, but nudged towards the line
	G_SnapVectorTowards( tr.endpos, muzzle );

	// send impact
	if ( traceEnt->takedamage &&
	     ( traceEnt->s.eType == ET_BUILDABLE ||
	       traceEnt->s.eType == ET_PLAYER ) )
	{
		tent = G_NewTempEntity( tr.endpos, EV_WEAPON_HIT_ENTITY );
		tent->s.eventParm = traceEnt->s.number;
		tent->s.weapon = ent->s.weapon;
		tent->s.generic1 = ent->s.generic1; //weaponMode

		// send victim
		tent->s.otherEntityNum = traceEnt->s.number;
	}
	else
	{
		tent = G_NewTempEntity( tr.endpos, EV_WEAPON_HIT_ENVIRONMENT );
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
		tent->s.generic1 = ent->s.generic1; //weaponMode
	}

	// send attacker
	tent->s.otherEntityNum2 = traceEnt->s.number;

	if ( traceEnt->takedamage )
	{
		G_Damage( traceEnt, ent, ent, forward, tr.endpos,
		          damage, 0, mod );
	}
}

/*
======================================================================

SHOTGUN

======================================================================
*/

/*
================
Keep this in sync with ShotgunPattern in g_weapon.c!
================
*/
static void ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, gentity_t *ent )
{
	int       i;
	float     r, u, a;
	vec3_t    end;
	vec3_t    forward, right, up;
	trace_t   tr;
	gentity_t *traceEnt;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// generate the "random" spread pattern
	for ( i = 0; i < SHOTGUN_PELLETS; i++ )
	{
		r = Q_crandom( &seed ) * M_PI;
		a = Q_random( &seed ) * SHOTGUN_SPREAD * 16;

		u = sin( r ) * a;
		r = cos( r ) * a;

		VectorMA( origin, SHOTGUN_RANGE, forward, end );
		VectorMA( end, r, right, end );
		VectorMA( end, u, up, end );

		trap_Trace( &tr, origin, NULL, NULL, end, ent->s.number, MASK_SHOT );
		traceEnt = &g_entities[ tr.entityNum ];

		// send bullet impact
		if ( !( tr.surfaceFlags & SURF_NOIMPACT ) )
		{
			if ( traceEnt->takedamage )
			{
				G_Damage( traceEnt, ent, ent, forward, tr.endpos,  SHOTGUN_DMG, 0, MOD_SHOTGUN );
			}
		}
	}
}

static void FireShotgun( gentity_t *ent )
{
	gentity_t *tent;

	// send shotgun blast
	tent = G_NewTempEntity( muzzle, EV_SHOTGUN );
	VectorScale( forward, 4096, tent->s.origin2 );
	SnapVector( tent->s.origin2 );
	tent->s.eventParm = rand() / ( RAND_MAX / 0x100 + 1 ); // seed for spread pattern
	tent->s.otherEntityNum = ent->s.number;
	G_UnlaggedOn( ent, muzzle, SHOTGUN_RANGE );
	ShotgunPattern( tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent );
	G_UnlaggedOff();
}

/*
======================================================================

MASS DRIVER

======================================================================
*/

static void FireMassdriver( gentity_t *ent )
{
	// TODO: Merge this with other *Fire functions

	trace_t   tr;
	vec3_t    end;
	gentity_t *tent;
	gentity_t *traceEnt;

	VectorMA( muzzle, 8192.0f * 16.0f, forward, end );

	G_UnlaggedOn( ent, muzzle, 8192.0f * 16.0f );
	trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
	G_UnlaggedOff();

	if ( tr.surfaceFlags & SURF_NOIMPACT )
	{
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// snap the endpos to integers, but nudged towards the line
	G_SnapVectorTowards( tr.endpos, muzzle );

	// send impact
	if ( traceEnt->takedamage &&
	     ( traceEnt->s.eType == ET_BUILDABLE ||
	       traceEnt->s.eType == ET_PLAYER ) )
	{
		//BloodSpurt( ent, traceEnt, &tr );
		tent = G_NewTempEntity( tr.endpos, EV_WEAPON_HIT_ENTITY );
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
		tent->s.generic1 = ent->s.generic1; //weaponMode

		// send victim
		tent->s.otherEntityNum = traceEnt->s.number;
	}
	else
	{
		tent = G_NewTempEntity( tr.endpos, EV_WEAPON_HIT_ENVIRONMENT );
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
		tent->s.generic1 = ent->s.generic1; //weaponMode
	}

	if ( traceEnt->takedamage )
	{
		G_Damage( traceEnt, ent, ent, forward, tr.endpos,
		          MDRIVER_DMG, 0, MOD_MDRIVER );
	}
}

/*
======================================================================

LOCKBLOB

======================================================================
*/

static void FireLockblob( gentity_t *self )
{
	G_SpawnMissile( MIS_LOCKBLOB, self, muzzle, forward, NULL, G_ExplodeMissile, level.time + 15000 );
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
	vec3_t    dir;
	trace_t   tr;
	gentity_t *ent;
	int       i;
	float     d, nearest;

	if ( level.time > self->timestamp )
	{
		VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
		self->s.pos.trType = TR_STATIONARY;
		self->s.pos.trTime = level.time;

		self->think = G_ExplodeMissile;
		self->nextthink = level.time + 50;
		self->parent->active = qfalse; //allow the parent to start again
		return;
	}

	nearest = DistanceSquared( self->r.currentOrigin, self->target->r.currentOrigin );

	//find the closest human
	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		ent = &g_entities[ i ];

		if ( ent->flags & FL_NOTARGET )
		{
			continue;
		}

		if ( ent->client &&
		     ent->health > 0 &&
		     ent->client->pers.team == TEAM_HUMANS &&
		     nearest > ( d = DistanceSquared( ent->r.currentOrigin, self->r.currentOrigin ) ) )
		{
			trap_Trace( &tr, self->r.currentOrigin, self->r.mins, self->r.maxs,
			            ent->r.currentOrigin, self->r.ownerNum, self->clipmask );

			if ( tr.entityNum != ENTITYNUM_WORLD )
			{
				nearest = d;
				self->target = ent;
			}
		}
	}

	VectorSubtract( self->target->r.currentOrigin, self->r.currentOrigin, dir );
	VectorNormalize( dir );

	//change direction towards the player
	VectorScale( dir, HIVE_SPEED, self->s.pos.trDelta );
	SnapVector( self->s.pos.trDelta );  // save net bandwidth
	VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
	self->s.pos.trTime = level.time;

	self->nextthink = level.time + HIVE_DIR_CHANGE_PERIOD;
}

static void FireHive( gentity_t *self )
{
	vec3_t    origin;
	gentity_t *m;

	// fire from the hive tip, not the center
	VectorMA( muzzle, self->r.maxs[ 2 ], self->s.origin2, origin );

	m = G_SpawnMissile( MIS_HIVE, self, origin, forward, self->target,
	                    HiveMissileThink, level.time + HIVE_DIR_CHANGE_PERIOD );

	m->timestamp = level.time + HIVE_LIFETIME;
}

/*
======================================================================

BLASTER PISTOL

======================================================================
*/

static void FireBlaster( gentity_t *self )
{
	G_SpawnMissile( MIS_BLASTER, self, muzzle, forward, NULL, G_ExplodeMissile, level.time + 10000 );
}

/*
======================================================================

PULSE RIFLE

======================================================================
*/

static void FirePrifle( gentity_t *self )
{
	G_SpawnMissile( MIS_PRIFLE, self, muzzle, forward, NULL, G_ExplodeMissile, level.time + 10000 );
}

/*
======================================================================

FLAME THROWER

======================================================================
*/

static void FireFlamer( gentity_t *self )
{
	G_SpawnMissile( MIS_FLAMER, self, muzzle, forward, NULL, G_FreeEntity, level.time + FLAMER_LIFETIME );
}

/*
======================================================================

GRENADE

======================================================================
*/

static void FireGrenade( gentity_t *self )
{
	G_SpawnMissile( MIS_GRENADE, self, muzzle, forward, NULL, G_ExplodeMissile, level.time + 5000 );
}

/*
======================================================================

LAS GUN

======================================================================
*/

static void FireLasgun( gentity_t *ent )
{
	// TODO: Merge this with other *Fire functions

	trace_t   tr;
	vec3_t    end;
	gentity_t *tent;
	gentity_t *traceEnt;

	VectorMA( muzzle, 8192 * 16, forward, end );

	G_UnlaggedOn( ent, muzzle, 8192 * 16 );
	trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
	G_UnlaggedOff();

	if ( tr.surfaceFlags & SURF_NOIMPACT )
	{
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// snap the endpos to integers, but nudged towards the line
	G_SnapVectorTowards( tr.endpos, muzzle );

	// send impact
	if ( traceEnt->takedamage &&
	     ( traceEnt->s.eType == ET_BUILDABLE ||
	       traceEnt->s.eType == ET_PLAYER ) )
	{
		//BloodSpurt( ent, traceEnt, &tr );
		tent = G_NewTempEntity( tr.endpos, EV_WEAPON_HIT_ENTITY );
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
		tent->s.generic1 = ent->s.generic1; //weaponMode

		// send victim
		tent->s.otherEntityNum = traceEnt->s.number;
	}
	else
	{
		tent = G_NewTempEntity( tr.endpos, EV_WEAPON_HIT_ENVIRONMENT );
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
		tent->s.generic1 = ent->s.generic1; //weaponMode
	}

	if ( traceEnt->takedamage )
	{
		G_Damage( traceEnt, ent, ent, forward, tr.endpos, LASGUN_DAMAGE, 0, MOD_LASGUN );
	}
}

/*
======================================================================

PAIN SAW

======================================================================
*/

static void FirePainsaw( gentity_t *ent )
{
	trace_t   tr;
	gentity_t *tent, *traceEnt;

	G_WideTrace( &tr, ent, PAINSAW_RANGE, PAINSAW_WIDTH, PAINSAW_HEIGHT,
	             &traceEnt );

	if ( !traceEnt || !traceEnt->takedamage )
	{
		return;
	}

	// send blood impact
	if ( traceEnt->s.eType == ET_PLAYER || traceEnt->s.eType == ET_BUILDABLE )
	{
		tent = G_NewTempEntity( tr.endpos, EV_WEAPON_HIT_ENTITY );
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
		tent->s.generic1 = ent->s.generic1; //weaponMode

		// send victim
		tent->s.otherEntityNum = traceEnt->s.number;
	}
	else
	{
		tent = G_NewTempEntity( tr.endpos, EV_WEAPON_HIT_ENVIRONMENT );
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
		tent->s.generic1 = ent->s.generic1; //weaponMode
	}

	G_Damage( traceEnt, ent, ent, forward, tr.endpos, PAINSAW_DAMAGE, DAMAGE_NO_KNOCKBACK, MOD_PAINSAW );
}

/*
======================================================================

LUCIFER CANNON

======================================================================
*/

static gentity_t *FireLcannonHelper( gentity_t *self, vec3_t start, vec3_t dir,
                                            int damage, int radius, int speed )
{
	// TODO: Tidy up this and lcannonFire

	gentity_t *m;
	int       nextthink;
	float     charge;

	// explode in front of player when overcharged
	if ( damage == LCANNON_DAMAGE )
	{
		nextthink = level.time;
	}
	else
	{
		nextthink = level.time + 10000;
	}

	if ( self->s.generic1 == WPM_PRIMARY )
	{
		m = G_SpawnMissile( MIS_LCANNON, self, start, dir, NULL, G_ExplodeMissile, nextthink );

		// some values are set in the code
		m->damage       = damage;
		m->splashDamage = damage / 2;
		m->splashRadius = radius;
		VectorScale( dir, speed, m->s.pos.trDelta );
		SnapVector( m->s.pos.trDelta ); // save net bandwidth

		// pass the missile charge through
		charge = ( float )( damage - LCANNON_SECONDARY_DAMAGE ) / LCANNON_DAMAGE;

		m->s.torsoAnim = charge * 255;

		if ( m->s.torsoAnim < 0 )
		{
			m->s.torsoAnim = 0;
		}
	}
	else
	{
		m = G_SpawnMissile( MIS_LCANNON2, self, start, dir, NULL, G_ExplodeMissile, nextthink );
	}

	return m;
}

static void FireLcannon( gentity_t *self, qboolean secondary )
{
	if ( secondary && self->client->ps.stats[ STAT_MISC ] <= 0 )
	{
		FireLcannonHelper( self, muzzle, forward, LCANNON_SECONDARY_DAMAGE,
		                   LCANNON_SECONDARY_RADIUS, LCANNON_SECONDARY_SPEED );
	}
	else
	{
		FireLcannonHelper( self, muzzle, forward,
		                   self->client->ps.stats[ STAT_MISC ] * LCANNON_DAMAGE / LCANNON_CHARGE_TIME_MAX,
		                   LCANNON_RADIUS, LCANNON_SPEED );
	}

	self->client->ps.stats[ STAT_MISC ] = 0;
}

/*
======================================================================

TESLA GENERATOR

======================================================================
*/

static void FireTesla( gentity_t *self )
{
	trace_t   tr;
	vec3_t    origin, target;
	gentity_t *tent;

	if ( !self->target )
	{
		return;
	}

	// Move the muzzle from the entity origin up a bit to fire over turrets
	VectorMA( muzzle, self->r.maxs[ 2 ], self->s.origin2, origin );

	// Don't aim for the center, aim at the top of the bounding box
	VectorCopy( self->target->s.origin, target );
	target[ 2 ] += self->target->r.maxs[ 2 ];

	// Trace to the target entity
	trap_Trace( &tr, origin, NULL, NULL, target, self->s.number, MASK_SHOT );

	if ( tr.entityNum != self->target->s.number )
	{
		return;
	}

	// Client side firing effect
	self->s.eFlags |= EF_FIRING;

	// Deal damage
	if ( self->target->takedamage )
	{
		vec3_t dir;

		VectorSubtract( target, origin, dir );
		VectorNormalize( dir );
		G_Damage( self->target, self, self, dir, tr.endpos,
		          TESLAGEN_DMG, 0, MOD_TESLAGEN );
	}

	// Send tesla zap trail
	tent = G_NewTempEntity( tr.endpos, EV_TESLATRAIL );
	tent->s.generic1 = self->s.number; // src
	tent->s.clientNum = self->target->s.number; // dest
}

/*
======================================================================

BUILD GUN

======================================================================
*/

void G_CheckCkitRepair( gentity_t *ent )
{
	vec3_t    viewOrigin, forward, end;
	trace_t   tr;
	gentity_t *traceEnt;

	if ( ent->client->ps.weaponTime > 0 ||
	     ent->client->ps.stats[ STAT_MISC ] > 0 )
	{
		return;
	}

	BG_GetClientViewOrigin( &ent->client->ps, viewOrigin );
	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );
	VectorMA( viewOrigin, 100, forward, end );

	trap_Trace( &tr, viewOrigin, NULL, NULL, end, ent->s.number, MASK_PLAYERSOLID );
	traceEnt = &g_entities[ tr.entityNum ];

	if ( tr.fraction < 1.0f && traceEnt->spawned && traceEnt->health > 0 &&
	     traceEnt->s.eType == ET_BUILDABLE && traceEnt->buildableTeam == TEAM_HUMANS )
	{
		const buildableAttributes_t *buildable;

		buildable = BG_Buildable( traceEnt->s.modelindex );

		if ( traceEnt->health < buildable->health )
		{
			traceEnt->health += HBUILD_HEALRATE;

			if ( traceEnt->health >= buildable->health )
			{
				traceEnt->health = buildable->health;
				G_AddEvent( ent, EV_BUILD_REPAIRED, 0 );
			}
			else
			{
				G_AddEvent( ent, EV_BUILD_REPAIR, 0 );
			}

			ent->client->ps.weaponTime += BG_Weapon( ent->client->ps.weapon )->repeatRate1;
		}
	}
}

static void CancelBuild( gentity_t *ent )
{
	// Cancel ghost buildable
	if ( ent->client->ps.stats[ STAT_BUILDABLE ] != BA_NONE )
	{
		ent->client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;
		ent->client->ps.stats[ STAT_PREDICTION ] = 0;
		return;
	}

	if ( ent->client->ps.weapon == WP_ABUILD ||
	     ent->client->ps.weapon == WP_ABUILD2 )
	{
		FireMelee( ent, ABUILDER_CLAW_RANGE, ABUILDER_CLAW_WIDTH,
		             ABUILDER_CLAW_WIDTH, ABUILDER_CLAW_DMG, MOD_ABUILDER_CLAW );
	}
}

static void FireBuild( gentity_t *ent, dynMenu_t menu )
{
	buildable_t buildable = ( ent->client->ps.stats[ STAT_BUILDABLE ]
	                          & ~SB_VALID_TOGGLEBIT );

	if ( buildable > BA_NONE )
	{
		if ( ent->client->ps.stats[ STAT_MISC ] > 0 )
		{
			G_AddEvent( ent, EV_BUILD_DELAY, ent->client->ps.clientNum );
			return;
		}

		if ( G_BuildIfValid( ent, buildable ) )
		{
			if ( !g_cheats.integer )
			{
				ent->client->ps.stats[ STAT_MISC ] +=
				  BG_Buildable( buildable )->buildTime;
			}

			ent->client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;
		}

		return;
	}

	G_TriggerMenu( ent->client->ps.clientNum, menu );
}

static void FireSlowblob( gentity_t *self )
{
	G_SpawnMissile( MIS_SLOWBLOB, self, muzzle, forward, NULL, G_ExplodeMissile, level.time + 15000 );
}

/*
======================================================================

LEVEL0

======================================================================
*/

qboolean G_CheckVenomAttack( gentity_t *ent )
{
	trace_t   tr;
	gentity_t *traceEnt;
	int       damage = LEVEL0_BITE_DMG;

	if ( ent->client->ps.weaponTime )
	{
		return qfalse;
	}

	// Calculate muzzle point
	AngleVectors( ent->client->ps.viewangles, forward, right, up );
	G_CalcMuzzlePoint( ent, forward, right, up, muzzle );

	G_WideTrace( &tr, ent, LEVEL0_BITE_RANGE, LEVEL0_BITE_WIDTH,
	             LEVEL0_BITE_WIDTH, &traceEnt );

	if ( traceEnt == NULL )
	{
		return qfalse;
	}

	if ( !traceEnt->takedamage )
	{
		return qfalse;
	}

	if ( traceEnt->health <= 0 )
	{
		return qfalse;
	}

	// only allow bites to work against buildings as they are constructing
	if ( traceEnt->s.eType == ET_BUILDABLE )
	{
		if ( traceEnt->spawned )
		{
			return qfalse;
		}

		if ( traceEnt->buildableTeam == TEAM_ALIENS )
		{
			return qfalse;
		}
	}

	if ( traceEnt->client )
	{
		if ( traceEnt->client->pers.team == TEAM_ALIENS )
		{
			return qfalse;
		}

		if ( traceEnt->client->ps.stats[ STAT_HEALTH ] <= 0 )
		{
			return qfalse;
		}
	}

	// send blood impact
	WideBloodSpurt( ent, traceEnt, &tr );

	G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_LEVEL0_BITE );
	ent->client->ps.weaponTime += LEVEL0_BITE_REPEAT;
	return qtrue;
}

/*
======================================================================

LEVEL1

======================================================================
*/

void G_CheckGrabAttack( gentity_t *ent )
{
	trace_t   tr;
	vec3_t    end, dir;
	gentity_t *traceEnt;

	// set aiming directions
	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	G_CalcMuzzlePoint( ent, forward, right, up, muzzle );

	if ( ent->client->ps.weapon == WP_ALEVEL1 )
	{
		VectorMA( muzzle, LEVEL1_GRAB_RANGE, forward, end );
	}
	else if ( ent->client->ps.weapon == WP_ALEVEL1_UPG )
	{
		VectorMA( muzzle, LEVEL1_GRAB_U_RANGE, forward, end );
	}

	trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );

	if ( tr.surfaceFlags & SURF_NOIMPACT )
	{
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	if ( !traceEnt->takedamage )
	{
		return;
	}

	if ( traceEnt->client )
	{
		if ( traceEnt->client->pers.team == TEAM_ALIENS )
		{
			return;
		}

		if ( traceEnt->client->ps.stats[ STAT_HEALTH ] <= 0 )
		{
			return;
		}

		if ( !( traceEnt->client->ps.stats[ STAT_STATE ] & SS_GRABBED ) )
		{
			AngleVectors( traceEnt->client->ps.viewangles, dir, NULL, NULL );
			traceEnt->client->ps.stats[ STAT_VIEWLOCK ] = DirToByte( dir );

			//event for client side grab effect
			G_AddPredictableEvent( ent, EV_LEV1_GRAB, 0 );
		}

		traceEnt->client->ps.stats[ STAT_STATE ] |= SS_GRABBED;

		if ( ent->client->ps.weapon == WP_ALEVEL1 )
		{
			traceEnt->client->grabExpiryTime = level.time + LEVEL1_GRAB_TIME;

			// Update the last combat time.
			ent->client->lastCombatTime = level.time + LEVEL1_GRAB_TIME;
			traceEnt->client->lastCombatTime = level.time + LEVEL1_GRAB_TIME;
		}
		else if ( ent->client->ps.weapon == WP_ALEVEL1_UPG )
		{
			traceEnt->client->grabExpiryTime = level.time + LEVEL1_GRAB_U_TIME;

			// Update the last combat time.
			ent->client->lastCombatTime = level.time + LEVEL1_GRAB_TIME;
			traceEnt->client->lastCombatTime = level.time + LEVEL1_GRAB_TIME;
		}
	}
}

static void FirePoisonCloud( gentity_t *ent )
{
	int       entityList[ MAX_GENTITIES ];
	vec3_t    range;
	vec3_t    mins, maxs;
	int       i, num;
	gentity_t *humanPlayer;
	trace_t   tr;

	VectorSet(range, LEVEL1_PCLOUD_RANGE, LEVEL1_PCLOUD_RANGE, LEVEL1_PCLOUD_RANGE);

	VectorAdd( ent->client->ps.origin, range, maxs );
	VectorSubtract( ent->client->ps.origin, range, mins );

	G_UnlaggedOn( ent, ent->client->ps.origin, LEVEL1_PCLOUD_RANGE );
	num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0; i < num; i++ )
	{
		humanPlayer = &g_entities[ entityList[ i ] ];

		if ( humanPlayer->client &&
		     humanPlayer->client->pers.team == TEAM_HUMANS )
		{
			trap_Trace( &tr, muzzle, NULL, NULL, humanPlayer->s.origin,
			            humanPlayer->s.number, CONTENTS_SOLID );

			//can't see target from here
			if ( tr.entityNum == ENTITYNUM_WORLD )
			{
				continue;
			}

			humanPlayer->client->ps.eFlags |= EF_POISONCLOUDED;
			humanPlayer->client->lastPoisonCloudedTime = level.time;

			trap_SendServerCommand( humanPlayer->client->ps.clientNum,
			                        "poisoncloud" );
		}
	}

	G_UnlaggedOff();
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
	vec3_t    range;
	vec3_t    mins, maxs;
	int       i, num;
	gentity_t *enemy;
	trace_t   tr;
	float     distance;

	VectorSet(range, LEVEL2_AREAZAP_CHAIN_RANGE, LEVEL2_AREAZAP_CHAIN_RANGE, LEVEL2_AREAZAP_CHAIN_RANGE);

	VectorAdd( ent->s.origin, range, maxs );
	VectorSubtract( ent->s.origin, range, mins );

	num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0; i < num; i++ )
	{
		enemy = &g_entities[ entityList[ i ] ];

		// don't chain to self; noclippers can be listed, don't chain to them either
		if ( enemy == ent || ( enemy->client && enemy->client->noclip ) )
		{
			continue;
		}

		distance = Distance( ent->s.origin, enemy->s.origin );

		if ( ( ( enemy->client &&
		         enemy->client->pers.team == TEAM_HUMANS ) ||
		       ( enemy->s.eType == ET_BUILDABLE &&
		         BG_Buildable( enemy->s.modelindex )->team == TEAM_HUMANS ) ) &&
		     enemy->health > 0 && // only chain to living targets
		     distance <= LEVEL2_AREAZAP_CHAIN_RANGE )
		{
			// world-LOS check: trace against the world, ignoring other BODY entities
			trap_Trace( &tr, ent->s.origin, NULL, NULL,
			            enemy->s.origin, ent->s.number, CONTENTS_SOLID );

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

	for ( i = 0; i < zap->numTargets; i++ )
	{
		entityNums[ i + 1 ] = zap->targets[ i ]->s.number;
	}

	BG_PackEntityNumbers( &zap->effectChannel->s,
	                      entityNums, zap->numTargets + 1 );


	G_SetOrigin( zap->effectChannel, muzzle );
	trap_LinkEntity( zap->effectChannel );
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

		zap->used = qtrue;
		zap->timeToLive = LEVEL2_AREAZAP_TIME;

		zap->creator = creator;
		zap->targets[ 0 ] = target;
		zap->numTargets = 1;

		// the zap chains only through living entities
		if ( target->health > 0 )
		{
			G_Damage( target, creator, creator, forward,
			          target->s.origin, LEVEL2_AREAZAP_DMG,
			          DAMAGE_NO_KNOCKBACK | DAMAGE_NO_LOCDAMAGE,
			          MOD_LEVEL2_ZAP );

			FindZapChainTargets( zap );

			for ( i = 1; i < zap->numTargets; i++ )
			{
				G_Damage( zap->targets[ i ], target, zap->creator, forward, target->s.origin,
				          LEVEL2_AREAZAP_DMG * ( 1 - powf( ( zap->distances[ i ] /
				                                 LEVEL2_AREAZAP_CHAIN_RANGE ), LEVEL2_AREAZAP_CHAIN_FALLOFF ) ) + 1,
				          DAMAGE_NO_KNOCKBACK | DAMAGE_NO_LOCDAMAGE,
				          MOD_LEVEL2_ZAP );
			}
		}

		zap->effectChannel = G_NewEntity();
		zap->effectChannel->s.eType = ET_LEV2_ZAP_CHAIN;
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
			zap->used = qfalse;
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
			zap->used = qfalse;
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
	trace_t   tr;
	gentity_t *traceEnt;

	G_WideTrace( &tr, ent, LEVEL2_AREAZAP_RANGE, LEVEL2_AREAZAP_WIDTH, LEVEL2_AREAZAP_WIDTH, &traceEnt );

	if ( traceEnt == NULL )
	{
		return;
	}

	if ( ( traceEnt->client && traceEnt->client->pers.team == TEAM_HUMANS ) ||
	     ( traceEnt->s.eType == ET_BUILDABLE &&
	       BG_Buildable( traceEnt->s.modelindex )->team == TEAM_HUMANS ) )
	{
		CreateNewZap( ent, traceEnt );
	}
}

/*
======================================================================

LEVEL3

======================================================================
*/

qboolean G_CheckPounceAttack( gentity_t *ent )
{
	trace_t   tr;
	gentity_t *traceEnt;
	int       damage, timeMax, pounceRange, payload;

	if ( ent->client->pmext.pouncePayload <= 0 )
	{
		return qfalse;
	}

	// In case the goon lands on his target, he gets one shot after landing
	payload = ent->client->pmext.pouncePayload;

	if ( !( ent->client->ps.pm_flags & PMF_CHARGE ) )
	{
		ent->client->pmext.pouncePayload = 0;
	}

	// Calculate muzzle point
	AngleVectors( ent->client->ps.viewangles, forward, right, up );
	G_CalcMuzzlePoint( ent, forward, right, up, muzzle );

	// Trace from muzzle to see what we hit
	pounceRange = ent->client->ps.weapon == WP_ALEVEL3 ? LEVEL3_POUNCE_RANGE :
	              LEVEL3_POUNCE_UPG_RANGE;
	G_WideTrace( &tr, ent, pounceRange, LEVEL3_POUNCE_WIDTH,
	             LEVEL3_POUNCE_WIDTH, &traceEnt );

	if ( traceEnt == NULL )
	{
		return qfalse;
	}

	// Send blood impact
	if ( traceEnt->takedamage )
	{
		WideBloodSpurt( ent, traceEnt, &tr );
	}

	if ( !traceEnt->takedamage )
	{
		return qfalse;
	}

	// Deal damage
	timeMax = ent->client->ps.weapon == WP_ALEVEL3 ? LEVEL3_POUNCE_TIME :
	          LEVEL3_POUNCE_TIME_UPG;
	damage = payload * LEVEL3_POUNCE_DMG / timeMax;
	ent->client->pmext.pouncePayload = 0;
	G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage,
	          DAMAGE_NO_LOCDAMAGE, MOD_LEVEL3_POUNCE );

	return qtrue;
}

static void FireBounceball( gentity_t *self )
{
	G_SpawnMissile( MIS_BOUNCEBALL, self, muzzle, forward, NULL, G_ExplodeMissile, level.time + 3000 );
}

/*
======================================================================

LEVEL4

======================================================================
*/

void G_ChargeAttack( gentity_t *ent, gentity_t *victim )
{
	int    damage;
	int    i;
	vec3_t forward;

	if ( ent->client->ps.stats[ STAT_MISC ] <= 0 ||
	     !( ent->client->ps.stats[ STAT_STATE ] & SS_CHARGING ) ||
	     ent->client->ps.weaponTime )
	{
		return;
	}

	VectorSubtract( victim->s.origin, ent->s.origin, forward );
	VectorNormalize( forward );

	if ( !victim->takedamage )
	{
		return;
	}

	// For buildables, track the last MAX_TRAMPLE_BUILDABLES_TRACKED buildables
	//  hit, and do not do damage if the current buildable is in that list
	//  in order to prevent dancing over stuff to kill it very quickly
	if ( !victim->client )
	{
		for ( i = 0; i < MAX_TRAMPLE_BUILDABLES_TRACKED; i++ )
		{
			if ( ent->client->trampleBuildablesHit[ i ] == victim - g_entities )
			{
				return;
			}
		}

		ent->client->trampleBuildablesHit[
		  ent->client->trampleBuildablesHitPos++ % MAX_TRAMPLE_BUILDABLES_TRACKED ] =
		    victim - g_entities;
	}

	WideBloodSpurt( ent, victim, NULL );

	damage = LEVEL4_TRAMPLE_DMG * ent->client->ps.stats[ STAT_MISC ] /
	         LEVEL4_TRAMPLE_DURATION;

	G_Damage( victim, ent, ent, forward, victim->s.origin, damage,
	          DAMAGE_NO_LOCDAMAGE, MOD_LEVEL4_TRAMPLE );

	ent->client->ps.weaponTime += LEVEL4_TRAMPLE_REPEAT;
}

/*
======================================================================

GENERIC

======================================================================
*/

static INLINE meansOfDeath_t G_ModWeight( const gentity_t *ent )
{
	return ent->client->pers.team == TEAM_HUMANS ? MOD_WEIGHT_H : MOD_WEIGHT_A;
}

void G_ImpactAttack( gentity_t *attacker, gentity_t *victim )
{
	float  impactVelocity, impactEnergy;
	vec3_t knockbackDir;
	int    attackerMass, impactDamage;

	// self must be a client
	if ( !attacker->client )
	{
		return;
	}

	// ignore invincible targets
	if ( !victim->takedamage )
	{
		return;
	}

	// don't do friendly fire
	if ( OnSameTeam( attacker, victim ) )
	{
		return;
	}

	// attacker must be above victim
	if ( attacker->client->ps.origin[ 2 ] + attacker->r.mins[ 2 ] <
	     victim->s.origin[ 2 ] + victim->r.maxs[ 2 ] )
	{
		return;
	}

	// allow the granger airlifting ritual
	if ( victim->client && BG_UpgradeIsActive( UP_JETPACK, victim->client->ps.stats ) &&
	     ( attacker->client->pers.classSelection == PCL_ALIEN_BUILDER0 ||
	       attacker->client->pers.classSelection == PCL_ALIEN_BUILDER0_UPG ) )
	{
		return;
	}

	// calculate impact damage
	attackerMass = BG_Class( attacker->client->pers.classSelection )->mass;
	impactVelocity = fabs( attacker->client->pmext.fallImpactVelocity[ 2 ] ) * IMPACTDMG_QU_TO_METER; // in m/s
	impactEnergy = attackerMass * impactVelocity * impactVelocity; // in J
	impactDamage = ( int )( impactEnergy * IMPACTDMG_JOULE_TO_DAMAGE );

	// deal impact damage to both clients and structures, use a threshold for friendly fire
	if ( impactDamage > 0 )
	{
		// calculate knockback direction
		VectorSubtract( victim->s.origin, attacker->client->ps.origin, knockbackDir );
		VectorNormalize( knockbackDir );

		G_Damage( victim, attacker, attacker, knockbackDir, victim->s.origin, impactDamage,
		          DAMAGE_NO_LOCDAMAGE, G_ModWeight( attacker ) );
	}
}

void G_WeightAttack( gentity_t *attacker, gentity_t *victim )
{
	float  weightDPS;
	int    attackerMass, victimMass, weightDamage;

	// weigth damage is only dealt between clients
	if ( !attacker->client || !victim->client )
	{
		return;
	}

	// ignore invincible targets
	if ( !victim->takedamage )
	{
		return;
	}

	// attacker must be above victim
	if ( attacker->client->ps.origin[ 2 ] + attacker->r.mins[ 2 ] <
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

	attackerMass = BG_Class( attacker->client->pers.classSelection )->mass;
	victimMass = BG_Class( victim->client->pers.classSelection )->mass;
	weightDPS = WEIGHTDMG_DMG_MODIFIER * MAX( attackerMass - victimMass, 0 );

	if ( weightDPS > WEIGHTDMG_DPS_THRESHOLD )
	{
		weightDamage = ( int )( weightDPS * ( WEIGHTDMG_REPEAT / 1000.0f ) );

		if ( weightDamage > 0 )
		{
			G_Damage( victim, attacker, attacker, NULL, victim->s.origin, weightDamage,
					  DAMAGE_NO_LOCDAMAGE, G_ModWeight( attacker ) );
		}
	}

	victim->client->nextCrushTime = level.time + WEIGHTDMG_REPEAT;
}

//======================================================================

/*
===============
Set muzzle location relative to pivoting eye.
===============
*/
void G_CalcMuzzlePoint( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint )
{
	vec3_t normal;

	VectorCopy( ent->client->ps.origin, muzzlePoint );
	BG_GetClientNormal( &ent->client->ps, normal );
	VectorMA( muzzlePoint, ent->client->ps.viewheight, normal, muzzlePoint );
	VectorMA( muzzlePoint, 1, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

void G_FireWeapon3( gentity_t *ent )
{
	if ( ent->client )
	{
		AngleVectors( ent->client->ps.viewangles, forward, right, up );
		G_CalcMuzzlePoint( ent, forward, right, up, muzzle );
	}
	else
	{
		AngleVectors( ent->s.angles2, forward, right, up );
		VectorCopy( ent->s.pos.trBase, muzzle );
	}

	switch ( ent->s.weapon )
	{
		case WP_ALEVEL3_UPG:
			FireBounceball( ent );
			break;

		case WP_ABUILD2:
			FireSlowblob( ent );
			break;

		default:
			break;
	}
}

void G_FireWeapon2( gentity_t *ent )
{
	if ( ent->client )
	{
		AngleVectors( ent->client->ps.viewangles, forward, right, up );
		G_CalcMuzzlePoint( ent, forward, right, up, muzzle );
	}
	else
	{
		AngleVectors( ent->s.angles2, forward, right, up );
		VectorCopy( ent->s.pos.trBase, muzzle );
	}

	switch ( ent->s.weapon )
	{
		case WP_ALEVEL1_UPG:
			FirePoisonCloud( ent );
			break;

		case WP_LUCIFER_CANNON:
			FireLcannon( ent, qtrue );
			break;

		case WP_ALEVEL2_UPG:
			FireAreaZap( ent );
			break;

		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			CancelBuild( ent );
			break;

		default:
			break;
	}
}

void G_FireWeapon( gentity_t *ent )
{
	if ( ent->client )
	{
		AngleVectors( ent->client->ps.viewangles, forward, right, up );
		G_CalcMuzzlePoint( ent, forward, right, up, muzzle );
	}
	else
	{
		AngleVectors( ent->turretAim, forward, right, up );
		VectorCopy( ent->s.pos.trBase, muzzle );
	}

	switch ( ent->s.weapon )
	{
		case WP_ALEVEL1:
			FireMelee( ent, LEVEL1_CLAW_RANGE, LEVEL1_CLAW_WIDTH, LEVEL1_CLAW_WIDTH,
			           LEVEL1_CLAW_DMG, MOD_LEVEL1_CLAW );
			break;

		case WP_ALEVEL1_UPG:
			FireMelee( ent, LEVEL1_CLAW_U_RANGE, LEVEL1_CLAW_WIDTH, LEVEL1_CLAW_WIDTH,
			           LEVEL1_CLAW_DMG, MOD_LEVEL1_CLAW );
			break;

		case WP_ALEVEL3:
			FireMelee( ent, LEVEL3_CLAW_RANGE, LEVEL3_CLAW_WIDTH, LEVEL3_CLAW_WIDTH,
			           LEVEL3_CLAW_DMG, MOD_LEVEL3_CLAW );
			break;

		case WP_ALEVEL3_UPG:
			FireMelee( ent, LEVEL3_CLAW_UPG_RANGE, LEVEL3_CLAW_WIDTH, LEVEL3_CLAW_WIDTH,
			           LEVEL3_CLAW_DMG, MOD_LEVEL3_CLAW );
			break;

		case WP_ALEVEL2:
			FireMelee( ent, LEVEL2_CLAW_RANGE, LEVEL2_CLAW_WIDTH, LEVEL2_CLAW_WIDTH,
			           LEVEL2_CLAW_DMG, MOD_LEVEL2_CLAW );
			break;

		case WP_ALEVEL2_UPG:
			FireMelee( ent, LEVEL2_CLAW_U_RANGE, LEVEL2_CLAW_WIDTH, LEVEL2_CLAW_WIDTH,
			           LEVEL2_CLAW_DMG, MOD_LEVEL2_CLAW );
			break;

		case WP_ALEVEL4:
			FireMelee( ent, LEVEL4_CLAW_RANGE, LEVEL4_CLAW_WIDTH, LEVEL4_CLAW_HEIGHT,
			           LEVEL4_CLAW_DMG, MOD_LEVEL4_CLAW );
			break;

		case WP_BLASTER:
			FireBlaster( ent );
			break;

		case WP_MACHINEGUN:
			FireBullet( ent, RIFLE_SPREAD, RIFLE_DMG, MOD_MACHINEGUN );
			break;

		case WP_SHOTGUN:
			FireShotgun( ent );
			break;

		case WP_CHAINGUN:
			FireBullet( ent, CHAINGUN_SPREAD, CHAINGUN_DMG, MOD_CHAINGUN );
			break;

		case WP_FLAMER:
			FireFlamer( ent );
			break;

		case WP_PULSE_RIFLE:
			FirePrifle( ent );
			break;

		case WP_MASS_DRIVER:
			FireMassdriver( ent );
			break;

		case WP_LUCIFER_CANNON:
			FireLcannon( ent, qfalse );
			break;

		case WP_LAS_GUN:
			FireLasgun( ent );
			break;

		case WP_PAIN_SAW:
			FirePainsaw( ent );
			break;

		case WP_GRENADE:
			FireGrenade( ent );
			break;

		case WP_LOCKBLOB_LAUNCHER:
			FireLockblob( ent );
			break;

		case WP_HIVE:
			FireHive( ent );
			break;

		case WP_TESLAGEN:
			FireTesla( ent );
			break;

		case WP_MGTURRET:
			FireBullet( ent, MGTURRET_SPREAD, MGTURRET_DMG, MOD_MGTURRET );
			break;

		case WP_ABUILD:
		case WP_ABUILD2:
			FireBuild( ent, MN_A_BUILD );
			break;

		case WP_HBUILD:
			FireBuild( ent, MN_H_BUILD );
			break;

		default:
			break;
	}
}
