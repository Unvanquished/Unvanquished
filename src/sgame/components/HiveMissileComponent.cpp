/*
===========================================================================

Unvanquished GPL Source Code
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
#include "HiveMissileComponent.h"
#include "sgame/Entities.h"

HiveMissileComponent::HiveMissileComponent(Entity& entity, MissileComponent& r_MissileComponent)
	: HiveMissileComponentBase(entity, r_MissileComponent)
{}

void HiveMissileComponent::HandleMissileSteer() {
	vec3_t    dir;
	trace_t   tr;
	gentity_t* ent;
	int       i;
	float     d, nearest;
	gentity_t* self = entity.oldEnt;

	nearest = DistanceSquared(self->r.currentOrigin, self->target->r.currentOrigin);

	//find the closest human
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		ent = &g_entities[i];

		if (!ent->inuse) continue;
		if (ent->flags & FL_NOTARGET) continue;

		if (ent->client && Entities::IsAlive(ent) && G_Team(ent) == TEAM_HUMANS &&
			nearest > (d = DistanceSquared(ent->r.currentOrigin, self->r.currentOrigin)))
		{
			trap_Trace(&tr, self->r.currentOrigin, self->r.mins, self->r.maxs,
				ent->r.currentOrigin, self->r.ownerNum, self->clipmask, 0);

			if (tr.entityNum != ENTITYNUM_WORLD)
			{
				nearest = d;
				self->target = ent;
			}
		}
	}

	VectorSubtract(self->target->r.currentOrigin, self->r.currentOrigin, dir);
	VectorNormalize(dir);

	//change direction towards the player
	VectorScale(dir, HIVE_SPEED, self->s.pos.trDelta);
	SnapVector(self->s.pos.trDelta);  // save net bandwidth
	VectorCopy(self->r.currentOrigin, self->s.pos.trBase);
	self->s.pos.trTime = level.time;
}
