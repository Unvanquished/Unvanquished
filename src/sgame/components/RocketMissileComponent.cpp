/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2014-2023 Unvanquished Developers

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
#include "RocketMissileComponent.h"

RocketMissileComponent::RocketMissileComponent(Entity& entity, MissileComponent& r_MissileComponent)
	: RocketMissileComponentBase(entity, r_MissileComponent)
{}

void RocketMissileComponent::HandleMissileSteer() {
	vec3_t currentDir, targetDir, newDir, rotAxis;
	float  rotAngle;
	gentity_t* self = entity.oldEnt;

	// Don't turn anymore if the target is dead or gone
	if (!self->target)
	{
		return;
	}

	// Calculate current and target direction.
	VectorNormalize2(self->s.pos.trDelta, currentDir);
	VectorSubtract(self->target->r.currentOrigin, self->r.currentOrigin, targetDir);
	VectorNormalize(targetDir);

	// Don't turn anymore after the target was passed.
	if (DotProduct(currentDir, targetDir) < 0)
	{
		return;
	}

	// Calculate new direction. Use a fixed turning angle.
	CrossProduct(currentDir, targetDir, rotAxis);
	rotAngle = RAD2DEG(acosf(DotProduct(currentDir, targetDir)));
	RotatePointAroundVector(newDir, rotAxis, currentDir,
		Math::Clamp(rotAngle, -ROCKET_TURN_ANGLE, ROCKET_TURN_ANGLE));

	// Check if new direction is safe. Turn anyway if old direction is unsafe, too.
	if (!RocketpodComponent::SafeShot(
		ENTITYNUM_NONE, VEC2GLM(self->r.currentOrigin), VEC2GLM(newDir)
	) && RocketpodComponent::SafeShot(
		ENTITYNUM_NONE, VEC2GLM(self->r.currentOrigin), VEC2GLM(currentDir)
	)) {
		return;
	}

	// Update trajectory.
	VectorScale(newDir, GetMissileComponent().Attributes().speed, self->s.pos.trDelta);
	SnapVector(self->s.pos.trDelta);
	VectorCopy(self->r.currentOrigin, self->s.pos.trBase); // TODO: Snap this, too?
	self->s.pos.trTime = level.time;
}
