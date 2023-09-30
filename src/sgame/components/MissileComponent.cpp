/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2023 Unvanquished Developers

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

MissileComponent::MissileComponent(Entity& entity, const missileAttributes_t* attributes, ThinkingComponent& r_ThinkingComponent)
	: MissileComponentBase(entity, attributes, r_ThinkingComponent),
	ma_(attributes),
	dead_(false)
{
	REGISTER_THINKER(Move, ThinkingComponent::SCHEDULER_BEFORE, 0); // every frame
	REGISTER_THINKER(Expire, ThinkingComponent::SCHEDULER_AFTER, ma_->lifetime);
}

void MissileComponent::Expire(int)
{
	if (dead_) return;

	dead_ = true;

	if (ma_->lifeEndExplode) {
		// turns the entity into an event and frees it later
		// FIXME: things that aren't grenades probably shouldn't have this enabled (though it rarely happens anyway)
		G_ExplodeMissile(entity.oldEnt);
	} else {
		entity.FreeAt(DeferredFreeingComponent::FREE_AFTER_THINKING);
	}
}

void MissileComponent::Move(int)
{
	if (dead_) return;

	dead_ = G_MoveMissile(entity.oldEnt);
}