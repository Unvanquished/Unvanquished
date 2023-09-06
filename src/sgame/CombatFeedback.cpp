/*
===========================================================================

Copyright 2016 Unvanquished Developers

This file is part of Unvanquished.

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "sg_local.h"
#include "sg_cm_world.h"

namespace CombatFeedback {

/**
 * @brief Notify a client of a hit
 */
void HitNotify(gentity_t *attacker, gentity_t *victim,
               Util::optional<glm::vec3> point_opt, float damage,
               meansOfDeath_t mod, bool lethal)
{
	ASSERT(attacker->client);
	bool indirect = false;
	int flags = 0;
	gentity_t *event;

	switch (mod) {
	case MOD_DECONSTRUCT:
		return;

	case MOD_FLAMER_SPLASH:
	case MOD_LCANNON_SPLASH:
	case MOD_GRENADE:
		// 'point' is incorrect for splash damage
		indirect = true;
		break;

	default:
		break;
	}

	if (victim->s.eType == entityType_t::ET_BUILDABLE)
		flags |= HIT_BUILDING;

	if (G_OnSameTeam(attacker, victim))
		flags |= HIT_FRIENDLY;

	if (lethal)
		flags |= HIT_LETHAL;

	event = G_NewEntity( NO_CBSE );
	event->s.eType = Util::enum_cast<entityType_t>(Util::ordinal(
	                 entityType_t::ET_EVENTS) + EV_HIT);

	event->nextthink = level.time + INDICATOR_LIFETIME;
	event->think = G_FreeEntity;

	event->r.svFlags = SVF_SINGLECLIENT | SVF_BROADCAST;
	event->r.singleClient = attacker->num();

	// warning: otherEntityNum2 is only 10 bits wide
	event->s.otherEntityNum2 = flags;
	event->s.angles2[0] = damage;
	event->s.otherEntityNum = victim->num();

	glm::vec3 point;
	if (!point_opt || indirect)
	{
		point = VEC2GLM( victim->r.currentOrigin ) + 0.5f * ( VEC2GLM( victim->r.mins ) + VEC2GLM( victim->r.maxs ) );
	}
	else
	{
		point = point_opt.value();
	}

	G_SetOrigin( event, point );

	// FIXME: is this necessary with SVF_BROADCAST?
	G_CM_LinkEntity(event);
}

} // namespace CombatFeedback
