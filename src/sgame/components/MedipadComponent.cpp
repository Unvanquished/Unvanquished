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
#include "MedipadComponent.h"
#include "sgame/Entities.h"

MedipadComponent::MedipadComponent(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent)
	: MedipadComponentBase(entity, r_HumanBuildableComponent)
{
	REGISTER_THINKER(Think, ThinkingComponent::SCHEDULER_AVERAGE, 100);
}

void MedipadComponent::HandleDie(gentity_t* /*killer*/, meansOfDeath_t /*meansOfDeath*/) {
	// Clear target's healing flag.
	// TODO: This will fail if multiple sources try to control the flag.
	if (target_) {
		target_->client->ps.stats[STAT_STATE] &= ~SS_HEALING_2X;
	}
}

gentity_t* MedipadComponent::GetTarget() const
{
	return target_.get();
}

void MedipadComponent::Think(int timeDelta)
{
	int       entityList[MAX_GENTITIES];
	vec3_t    mins, maxs;
	gentity_t* self = entity.oldEnt;

	if (!self->spawned)
	{
		return;
	}

	// clear target's healing flag for now
	if (target_)
	{
		target_->client->ps.stats[STAT_STATE] &= ~SS_HEALING_2X;
	}

	// clear target on power loss
	if (!self->powered || Entities::IsDead(self))
	{
		target_ = nullptr;
		return;
	}

	// The target is assigned on every think, so we can see if it was healing during
	// the last think by checking whether it was assigned something non-null
	// (ignoring reference invalidation).
	bool medistationWasHealing = target_.entity != nullptr;

	// get entities standing on top
	VectorAdd(self->s.origin, self->r.maxs, maxs);
	VectorAdd(self->s.origin, self->r.mins, mins);

	mins[2] += (self->r.mins[2] + self->r.maxs[2]);
	maxs[2] += 32; // continue to heal jumping players but don't heal jetpack campers

	int numPlayers = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);
	gentity_t* oldTarget = target_.get();
	gentity_t* newTarget = nullptr;

	// clear poison, distribute medikits, find a new target if necessary
	for (int playerNum = 0; playerNum < numPlayers; playerNum++)
	{
		gentity_t* player = &g_entities[entityList[playerNum]];
		gclient_t* client = player->client;

		// only react to humans
		if (!client || client->pers.team != TEAM_HUMANS)
		{
			continue;
		}

		// ignore 'notarget' users
		if (player->flags & FL_NOTARGET)
		{
			continue;
		}

		// remove poison
		if (client->ps.stats[STAT_STATE] & SS_POISONED)
		{
			client->ps.stats[STAT_STATE] &= ~SS_POISONED;
		}

		bool fullHealth = player->entity->Get<HealthComponent>()->FullHealth();

		// give medikit to players with full health
		if (fullHealth)
		{
			if (!BG_InventoryContainsUpgrade(UP_MEDKIT, player->client->ps.stats))
			{
				BG_AddUpgradeToInventory(UP_MEDKIT, player->client->ps.stats);
			}
		}

		// if not already occupied, check if someone needs healing
		if (!newTarget || player == oldTarget)
		{
			if (PM_Live(client->ps.pm_type) &&
				(!fullHealth || client->ps.stats[STAT_STAMINA] < STAMINA_MAX))
			{
				newTarget = player;
			}
		}
	}

	target_ = newTarget;

	// if we have a target, heal it
	if (newTarget)
	{
		gclient_t* client = newTarget->client;
		client->ps.stats[STAT_STATE] |= SS_HEALING_2X;

		// start healing animation
		if (!medistationWasHealing)
		{
			G_SetBuildableAnim(self, BANIM_ATTACK1, false);
			G_SetIdleBuildableAnim(self, BANIM_IDLE2);
		}

		// restore stamina
		if (client->ps.stats[STAT_STAMINA] + STAMINA_MEDISTAT_RESTORE >= STAMINA_MAX)
		{
			client->ps.stats[STAT_STAMINA] = STAMINA_MAX;
		}
		else
		{
			client->ps.stats[STAT_STAMINA] += STAMINA_MEDISTAT_RESTORE;
		}

		// restore health
		newTarget->entity->Heal(timeDelta * 0.01f, nullptr);

		// check if fully healed
		if (newTarget->entity->Get<HealthComponent>()->FullHealth())
		{
			// give medikit
			if (!BG_InventoryContainsUpgrade(UP_MEDKIT, client->ps.stats))
			{
				BG_AddUpgradeToInventory(UP_MEDKIT, client->ps.stats);
			}
		}
	}
	// we lost our target
	else if (medistationWasHealing)
	{
		// stop healing animation
		G_SetBuildableAnim(self, BANIM_ATTACK2, true);
		G_SetIdleBuildableAnim(self, BANIM_IDLE1);
	}
}
