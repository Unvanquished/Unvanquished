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

// -----------
// definitions
// -----------

#define MISSILE_PRESTEP_TIME 50

typedef enum missileTimePowerMod_e {
	MTPR_LINEAR_DECREASE,
	MTPR_LINEAR_INCREASE,
	MTPR_EXPONENTIAL_DECREASE // endTime is half time period, endMod is ignored
} missileTimePowerMod_t;

// -------------
// local methods
// -------------

static void BounceMissile( gentity_t *ent, trace_t *trace )
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
			G_SetOrigin( ent, trace->endpos );
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
			fract = MIN( affectedTime, endTime ) / ( float )endTime;
			return startMod - ( startMod - endMod ) * fract;

		case MTPR_LINEAR_INCREASE:
			// time is full time
			fract = MIN( affectedTime, endTime ) / ( float )endTime;
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
			                            0, FLAMER_LIFETIME );

		case MIS_LCANNON:
			return MissileTimePowerMod( self, MTPR_EXPONENTIAL_DECREASE, 1.0f, 0.0f,
			                            LCANNON_DAMAGE_FULL_TIME, LCANNON_DAMAGE_HALF_LIFE );

		case MIS_PRIFLE:
			return MissileTimePowerMod( self, MTPR_EXPONENTIAL_DECREASE, 1.0f, 0.0f,
			                            PRIFLE_DAMAGE_FULL_TIME, PRIFLE_DAMAGE_HALF_LIFE );
	}

	return 1.0f;
}

static float MissileTimeSplashDmgMod( gentity_t *self )
{
	switch ( self->s.weapon )
	{
		case MIS_FLAMER:
			return MissileTimePowerMod( self, MTPR_LINEAR_INCREASE, FLAMER_SPLASH_MINDST_MOD, 1.0f,
										0, FLAMER_LIFETIME );
	}

	return 1.0f;
}

static void MissileImpact( gentity_t *ent, trace_t *trace )
{
	// TODO: Move special treatment for certain missiles somewhere else

	gentity_t *other, *attacker, *neighbor;
	qboolean  doDamage = qtrue, returnAfterDamage = qfalse;
	vec3_t    dir;

	other = &g_entities[ trace->entityNum ];
	attacker = &g_entities[ ent->r.ownerNum ];

	// check for bounce
	if ( !other->takedamage &&
	     ( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) )
	{
		BounceMissile( ent, trace );

		//only play a sound if requested
		if ( !( ent->s.eFlags & EF_NO_BOUNCE_SOUND ) )
		{
			G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		}

		return;
	}

	if ( !strcmp( ent->classname, "grenade" ) || !strcmp( ent->classname, "firebomb" ) )
	{
		// grenade doesn't explode on impact
		BounceMissile( ent, trace );

		// only play a sound if requested
		if ( !( ent->s.eFlags & EF_NO_BOUNCE_SOUND ) )
		{
			G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		}

		return;
	}
	else if ( !strcmp( ent->classname, "flamer" ) )
	{
		// ignite alien buildables on direct hit
		if ( other->s.eType == ET_BUILDABLE && other->buildableTeam == TEAM_ALIENS )
		{
			if ( random() < FLAMER_IGNITE_CHANCE )
			{
				G_IgniteBuildable( other, ent->parent );
			}
		}

		// ignite alien buildables in radius
		neighbor = NULL;
		while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, trace->endpos, FLAMER_IGNITE_RADIUS ) ) )
		{
			// we already handled other, since it might not always be in FLAMER_IGNITE_RADIUS due to BBOX sizes
			if ( neighbor == other )
			{
				continue;
			}

			if ( neighbor->s.eType == ET_BUILDABLE && neighbor->buildableTeam == TEAM_ALIENS )
			{
				if ( random() < FLAMER_IGNITE_SPLCHANCE )
				{
					G_IgniteBuildable( neighbor, ent->parent );
				}
			}
		}

		// set the environment on fire
		if ( other->s.number == ENTITYNUM_WORLD )
		{
			if ( random() < FLAMER_LEAVE_FIRE_CHANCE )
			{
				G_SpawnFire( trace->endpos, trace->plane.normal, ent->parent );
			}
		}
	}
	else if ( !strcmp( ent->classname, "firebomb_sub" ) )
	{
		// ignite alien buildables on direct hit
		if ( other->s.eType == ET_BUILDABLE && other->buildableTeam == TEAM_ALIENS )
		{
			G_IgniteBuildable( other, ent->parent );
		}

		// set the environment on fire
		if ( other->s.number == ENTITYNUM_WORLD )
		{
			G_SpawnFire( trace->endpos, trace->plane.normal, ent->parent );
		}
	}
	else if ( !strcmp( ent->classname, "lockblob" ) )
	{
		if ( other->client && other->client->pers.team == TEAM_HUMANS )
		{
			other->client->ps.stats[ STAT_STATE ] |= SS_BLOBLOCKED;
			other->client->lastLockTime = level.time;
			AngleVectors( other->client->ps.viewangles, dir, NULL, NULL );
			other->client->ps.stats[ STAT_VIEWLOCK ] = DirToByte( dir );
		}
	}
	else if ( !strcmp( ent->classname, "slowblob" ) )
	{
		if ( other->client && other->client->pers.team == TEAM_HUMANS )
		{
			other->client->ps.stats[ STAT_STATE ] |= SS_SLOWLOCKED;
			other->client->lastSlowTime = level.time;
		}
		else if ( other->s.eType == ET_BUILDABLE && other->buildableTeam == TEAM_ALIENS )
		{
			other->onFire = qfalse;
			other->fireImmunityUntil = level.time + ABUILDER_BLOB_FIRE_IMMUNITY;
			doDamage = qfalse;
		}
		else if ( other->s.number == ENTITYNUM_WORLD )
		{
			// put out floor fires in range
			neighbor = NULL;
			while ( ( neighbor = G_IterateEntitiesWithinRadius( neighbor, trace->endpos,
			                                                  ABUILDER_BLOB_FIRE_STOP_RANGE ) ) )
			{
				if ( neighbor->s.eType == ET_FIRE )
				{
					G_FreeEntity( neighbor );
				}
			}
		}
	}
	else if ( !strcmp( ent->classname, "hive" ) )
	{
		if ( other->s.eType == ET_BUILDABLE && other->s.modelindex == BA_A_HIVE )
		{
			if ( !ent->parent )
			{
				G_Printf( S_WARNING "hive entity has no parent in G_MissileImpact\n" );
			}
			else
			{
				ent->parent->active = qfalse;
			}

			G_FreeEntity( ent );
			return;
		}
		else
		{
			//prevent collision with the client when returning
			ent->r.ownerNum = other->s.number;

			ent->think = G_ExplodeMissile;
			ent->nextthink = level.time + FRAMETIME;

			//only damage humans
			if ( other->client && other->client->pers.team == TEAM_HUMANS )
			{
				returnAfterDamage = qtrue;
			}
			else
			{
				return;
			}
		}
	}

	// impact damage
	if ( doDamage && ent->damage && other->takedamage )
	{
		vec3_t dir;

		BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, dir );

		if ( VectorNormalize( dir ) == 0 )
		{
			dir[ 2 ] = 1; // stepped on a grenade
		}

		G_Damage( other, ent, attacker, dir, ent->s.origin,
				  roundf( ent->damage * MissileTimeDmgMod( ent ) ),
				  DAMAGE_NO_LOCDAMAGE, ent->methodOfDeath );
	}

	if ( returnAfterDamage )
	{
		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && ( other->s.eType == ET_PLAYER || other->s.eType == ET_BUILDABLE ) )
	{
		G_AddEvent( ent, EV_MISSILE_HIT_ENTITY, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	}
	else if ( trace->surfaceFlags & SURF_METAL )
	{
		G_AddEvent( ent, EV_MISSILE_HIT_METAL, DirToByte( trace->plane.normal ) );
	}
	else
	{
		G_AddEvent( ent, EV_MISSILE_HIT_ENVIRONMENT, DirToByte( trace->plane.normal ) );
	}

	ent->freeAfterEvent = qtrue;

	// change over to a general entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	G_SnapVectorTowards( trace->endpos, ent->s.pos.trBase );  // save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	// splash damage (doesn't apply to person directly hit)
	if ( doDamage && ent->splashDamage )
	{
		G_RadiusDamage( trace->endpos, ent->parent,
		                ent->splashDamage * MissileTimeSplashDmgMod( ent ),
		                ent->splashRadius, other, ent->splashMethodOfDeath );
	}

	trap_LinkEntity( ent );
}

// ------------
// GAME methods
// ------------

void G_ExplodeMissile( gentity_t *ent )
{
	vec3_t dir;
	vec3_t origin;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[ 0 ] = dir[ 1 ] = 0;
	dir[ 2 ] = 1;

	// turn the missile into an event carrier
	ent->s.eType = ET_INVISIBLE;
	ent->freeAfterEvent = qtrue;
	G_AddEvent( ent, EV_MISSILE_HIT_ENVIRONMENT, DirToByte( dir ) );

	// splash damage
	if ( ent->splashDamage )
	{
		G_RadiusDamage( ent->r.currentOrigin, ent->parent,
		                ent->splashDamage * MissileTimeSplashDmgMod( ent ),
		                ent->splashRadius, ent, ent->splashMethodOfDeath );
	}

	trap_LinkEntity( ent );
}

void G_RunMissile( gentity_t *ent )
{
	vec3_t   origin;
	trace_t  tr;
	int      passent;
	qboolean impact = qfalse;

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// ignore interactions with the missile owner
	passent = ent->r.ownerNum;

	// general trace to see if we hit anything at all
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs,
	            origin, passent, ent->clipmask );

	if ( tr.startsolid || tr.allsolid )
	{
		tr.fraction = 0.0f;
		VectorCopy( ent->r.currentOrigin, tr.endpos );
	}

	if ( tr.fraction < 1.0f )
	{
		if ( !ent->pointAgainstWorld || (tr.contents & CONTENTS_BODY) )
		{
			// We hit an entity or we don't care
			impact = qtrue;
		}
		else
		{
			trap_Trace( &tr, ent->r.currentOrigin, NULL, NULL, origin,
			            passent, ent->clipmask );

			if ( tr.fraction < 1.0f )
			{
				// Hit the world with point trace
				impact = qtrue;
			}
			else
			{
				if ( tr.contents & CONTENTS_BODY )
				{
					// Hit an entity
					impact = qtrue;
				}
				else
				{
					trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs,
					            origin, passent, CONTENTS_BODY );

					if ( tr.fraction < 1.0f )
					{
						impact = qtrue;
					}
				}
			}
		}
	}

	VectorCopy( tr.endpos, ent->r.currentOrigin );

	if ( impact )
	{
		if ( tr.surfaceFlags & SURF_NOIMPACT )
		{
			// Never explode or bounce on sky
			G_FreeEntity( ent );
			return;
		}

		MissileImpact( ent, &tr );

		if ( ent->s.eType != ET_MISSILE )
		{
			return; // exploded
		}
	}

	ent->r.contents = CONTENTS_SOLID; //trick trap_LinkEntity into...
	trap_LinkEntity( ent );
	ent->r.contents = 0; //...encoding bbox information

	if ( ent->flightSplashDamage )
	{
		G_RadiusDamage( tr.endpos, ent->parent, ent->flightSplashDamage, ent->flightSplashRadius,
		                ent->parent, ent->splashMethodOfDeath );
	}

	// check think function after bouncing
	G_RunThink( ent );
}

gentity_t *G_SpawnMissile( missile_t missile, gentity_t *parent, vec3_t start, vec3_t dir,
                           gentity_t *target, void ( *think )( gentity_t *self ), int nextthink )
{
	gentity_t                 *m;
	const missileAttributes_t *ma;
	vec3_t                    velocity;

	if ( !parent )
	{
		return NULL;
	}

	ma = BG_Missile( missile );

	m = G_NewEntity();

	// generic
	m->s.eType             = ET_MISSILE;
	m->r.ownerNum          = parent->s.number;
	m->parent              = parent;
	m->target              = target;
	m->think               = think;
	m->nextthink           = nextthink;

	// from attribute config file
	m->s.weapon            = ma->number;
	m->classname           = ma->name;
	m->pointAgainstWorld   = ma->pointAgainstWorld;
	m->damage              = ma->damage;
	m->methodOfDeath       = ma->meansOfDeath;
	m->splashDamage        = ma->splashDamage;
	m->splashRadius        = ma->splashRadius;
	m->splashMethodOfDeath = ma->splashMeansOfDeath;
	m->clipmask            = ma->clipmask;
	m->r.mins[ 0 ]         =
	m->r.mins[ 1 ]         =
	m->r.mins[ 2 ]         = -ma->size;
	m->r.maxs[ 0 ]         =
	m->r.maxs[ 1 ]         =
	m->r.maxs[ 2 ]         = ma->size;
	m->s.eFlags            = ma->flags;

	// not yet implemented / deprecated
	m->flightSplashDamage  = 0;
	m->flightSplashRadius  = 0;

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

	return m;
}
