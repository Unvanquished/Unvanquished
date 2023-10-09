/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development
Copyright (C) 2012-2023 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/
#include "MissileComponent.h"
#include "sgame/sg_cm_world.h"

MissileComponent::MissileComponent(Entity& entity, const missileAttributes_t* attributes, ThinkingComponent& r_ThinkingComponent)
	: MissileComponentBase(entity, attributes, r_ThinkingComponent),
	ma_(attributes),
	dead_(false)
{
	REGISTER_THINKER(Move, ThinkingComponent::SCHEDULER_BEFORE, 0); // every frame
	REGISTER_THINKER(Expire, ThinkingComponent::SCHEDULER_AFTER, ma_->lifetime);
	if (ma_->steeringPeriod) {
		REGISTER_THINKER(Steer, ThinkingComponent::SCHEDULER_AVERAGE, ma_->steeringPeriod);
	}
}

void MissileComponent::Expire(int)
{
	if (dead_) return;

	dead_ = true;

	if (ma_->lifeEndExplode) {
		// turns the entity into an event and frees it later
		// FIXME: things that aren't grenades probably shouldn't have this enabled (though it rarely happens anyway)
		Explode();
	} else {
		entity.FreeAt(DeferredFreeingComponent::FREE_AFTER_THINKING);
	}
}

static trace2_t MissileTrace(const missileAttributes_t& ma, gentity_t* ent)
{
	// get current position
	vec3_t origin;
	BG_EvaluateTrajectory(&ent->s.pos, level.time, origin);

	// ignore interactions with the missile owner
	int passent = ent->r.ownerNum;

	trace2_t result;

	if (ma.pointAgainstWorld)
	{
		ASSERT(ent->clipmask & CONTENTS_BODY);
		trace2_t trWorld = G_Trace2(ent->r.currentOrigin, nullptr, nullptr, origin,
			passent, ent->clipmask & ~CONTENTS_BODY, 0);

		if (trWorld.startsolid)
		{
			return trWorld;
		}

		trace2_t trBody = G_Trace2(ent->r.currentOrigin, ent->r.mins, ent->r.maxs, trWorld.endpos,
			passent, CONTENTS_BODY, 0);

		result = trWorld.fraction < trBody.fraction ? trWorld : trBody;
	}
	else
	{
		result = G_Trace2(ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
			passent, ent->clipmask, 0);
	}

	if (result.startsolid)
	{
		// In this case we have a collision, and so we want a normal vector, but there isn't one
		// Try to use the missile's direction of travel
		glm::vec3 dir = VEC2GLM(origin) - VEC2GLM(ent->r.currentOrigin);
		if (VectorNormalize(&dir[0]))
		{
			VectorCopy(dir, result.plane.normal);
		}
		else
		{
			VectorSet(result.plane.normal, 1, 0, 0);
		}
	}
	return result;
}

// Move missile along its trajectory and detect collisions.
// Returns true if the missile has ceased to exist
static bool MoveMissile(const missileAttributes_t& ma, gentity_t* ent)
{
	trace2_t tr = MissileTrace(ma, ent);
	VectorCopy(tr.endpos, ent->r.currentOrigin);

	if (tr.fraction < 1.0f)
	{
		// Never explode or bounce when hitting the sky.
		if (tr.surfaceFlags & SURF_NOIMPACT)
		{
			ent->entity->FreeAt(DeferredFreeingComponent::FREE_AFTER_THINKING);

			return true;
		}

		// Check for impact damage and effects.
		if (G_MissileImpact(ent, &tr))
		{
			return true;
		}
	}

	ent->r.contents = CONTENTS_SOLID; //trick trap_LinkEntity into...
	trap_LinkEntity(ent);
	ent->r.contents = 0; //...encoding bbox information

	return false;
}

void MissileComponent::Move(int)
{
	if (dead_) return;

	dead_ = MoveMissile(*ma_, entity.oldEnt);
}

void MissileComponent::Steer(int)
{
	if (dead_) return;

	if (!entity.MissileSteer()) {
		Log::Warn("Missile can't steer");
	}
}

void MissileComponent::Explode()
{
	gentity_t* ent = entity.oldEnt;

	if (ent->s.modelindex == MIS_FIREBOMB)
	{
		G_FirebombMissileIgnite(ent);
	}

	vec3_t dir;
	const missileAttributes_t* ma = BG_Missile(ent->s.modelindex);

	glm::vec3 origin = VEC2GLM(ent->r.currentOrigin);
	SnapVector(origin);
	G_SetOrigin(ent, origin);

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	// turn the missile into an event carrier
	ent->s.eType = entityType_t::ET_INVISIBLE;
	ent->freeAfterEvent = true;
	G_AddEvent(ent, EV_MISSILE_HIT_ENVIRONMENT, DirToByte(dir));

	// splash damage
	if (ent->splashDamage)
	{
		G_RadiusDamage(ent->r.currentOrigin, ent->parent,
			ent->splashDamage * G_MissileTimeSplashDmgMod(ent),
			ent->splashRadius, ent, (ma->doKnockback ? DAMAGE_KNOCKBACK : 0),
			ma->splashMeansOfDeath);
	}

	trap_LinkEntity(ent);
}
