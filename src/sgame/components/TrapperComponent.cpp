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
#include "TrapperComponent.h"
#include "sgame/Entities.h"

#define TRAPPER_ACCURACY 10 // lower is better

TrapperComponent::TrapperComponent(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
	: TrapperComponentBase(entity, r_AlienBuildableComponent)
{
	REGISTER_THINKER(Think, ThinkingComponent::SCHEDULER_AVERAGE, 100);
}

void TrapperComponent::FireOnEnemy()
{
	vec3_t    dirToTarget;
	vec3_t    halfAcceleration, thirdJerk;
	float     distanceToTarget = LOCKBLOB_RANGE;
	gentity_t* self = entity.oldEnt;
	gentity_t* target = target_.get();

	int       lowMsec = 0;
	int       highMsec = (int)((
		((distanceToTarget * LOCKBLOB_SPEED) +
			(distanceToTarget * BG_Class(target->client->ps.stats[STAT_CLASS])->speed)) /
		(LOCKBLOB_SPEED * LOCKBLOB_SPEED)) * 1000.0f);

	VectorScale(target->acceleration, 1.0f / 2.0f, halfAcceleration);
	VectorScale(target->jerk, 1.0f / 3.0f, thirdJerk);

	// highMsec and lowMsec can only move toward
	// one another, so the loop must terminate
	while (highMsec - lowMsec > TRAPPER_ACCURACY)
	{
		int   partitionMsec = (highMsec + lowMsec) / 2;
		float time = (float)partitionMsec / 1000.0f;
		float projectileDistance = LOCKBLOB_SPEED * (time + MISSILE_PRESTEP_TIME / 1000.0f);

		VectorMA(target->s.pos.trBase, time, target->s.pos.trDelta, dirToTarget);
		VectorMA(dirToTarget, time * time, halfAcceleration, dirToTarget);
		VectorMA(dirToTarget, time * time * time, thirdJerk, dirToTarget);
		VectorSubtract(dirToTarget, self->s.pos.trBase, dirToTarget);
		distanceToTarget = VectorLength(dirToTarget);

		if (projectileDistance < distanceToTarget)
		{
			lowMsec = partitionMsec;
		}
		else if (projectileDistance > distanceToTarget)
		{
			highMsec = partitionMsec;
		}
		else if (projectileDistance == distanceToTarget)
		{
			break; // unlikely to happen
		}
	}

	VectorNormalize(dirToTarget);

	//fire at target
	G_SpawnDumbMissile( MIS_LOCKBLOB, self, VEC2GLM( self->s.pos.trBase ), VEC2GLM( dirToTarget ) );
	G_SetBuildableAnim(self, BANIM_ATTACK1, false);
	nextFireTime_ = level.time + LOCKBLOB_REPEAT;
}

static bool ATrapper_CheckTarget(gentity_t* self, gentity_t* target)
{
	vec3_t  distance;
	trace_t trace;

	ASSERT(target->client);

	if (target->flags & FL_NOTARGET) // is the target cheating?
	{
		return false;
	}

	if (target->client->pers.team == TEAM_ALIENS) // one of us?
	{
		return false;
	}

	if (target->client->sess.spectatorState != SPECTATOR_NOT) // is the target alive?
	{
		return false;
	}

	if (Entities::IsDead(target)) // is the target still alive?
	{
		return false;
	}

	if (target->client->ps.stats[STAT_STATE] & SS_BLOBLOCKED) // locked?
	{
		return false;
	}

	VectorSubtract(target->r.currentOrigin, self->r.currentOrigin, distance);

	if (VectorLength(distance) > LOCKBLOB_RANGE)  // is the target within range?
	{
		return false;
	}

	//only allow a narrow field of "vision"
	VectorNormalize(distance);  //is now direction of target

	if (DotProduct(distance, self->s.origin2) < LOCKBLOB_DOT)
	{
		return false;
	}

	trap_Trace(&trace, self->s.pos.trBase, nullptr, nullptr, target->s.pos.trBase, self->num(),
		MASK_SHOT, 0);

	if (trace.contents & CONTENTS_SOLID) // can we see the target?
	{
		return false;
	}

	return true;
}

static gentity_t* ATrapper_FindEnemy(gentity_t* ent)
{
	int       i;
	int       start;

	// iterate through entities
	start = rand() / (RAND_MAX / MAX_CLIENTS + 1);

	for (i = start; i < MAX_CLIENTS + start; i++)
	{
		gentity_t* target = g_entities + (i % MAX_CLIENTS);

		//if target is not valid keep searching
		if (!target->inuse || !ATrapper_CheckTarget(ent, target))
		{
			continue;
		}

		//we found a target
		return target;
	}

	//couldn't find a target
	return nullptr;
}

void TrapperComponent::Think(int)
{
	gentity_t* self = entity.oldEnt;

	if (!self->spawned || !self->powered || Entities::IsDead(self))
	{
		return;
	}

	//if the current target is not valid find a new one
	if (!target_ || !ATrapper_CheckTarget(self, target_.get()))
	{
		target_ = ATrapper_FindEnemy(self);
	}

	//if a new target cannot be found don't do anything
	if (!target_)
	{
		return;
	}

	//if we are pointing at our target and we can fire shoot it
	if (nextFireTime_ < level.time)
	{
		FireOnEnemy();
	}
}
