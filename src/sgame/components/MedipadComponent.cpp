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
	if (entity.oldEnt->target && entity.oldEnt->target->client) {
		entity.oldEnt->target->client->ps.stats[STAT_STATE] &= ~SS_HEALING_2X;
	}
}

void MedipadComponent::Think(int)
{
	int       entityList[MAX_GENTITIES];
	vec3_t    mins, maxs;
	int       playerNum, numPlayers;
	gentity_t* player;
	gclient_t* client;
	bool  occupied;
	gentity_t* self = entity.oldEnt;

	if (!self->spawned)
	{
		return;
	}

	// clear target's healing flag for now
	if (self->target && self->target->client)
	{
		self->target->client->ps.stats[STAT_STATE] &= ~SS_HEALING_2X;
	}
	else
	{
		self->target = nullptr;
	}

	// clear target on power loss
	if (!self->powered || Entities::IsDead(self))
	{
		self->medistationIsHealing = false;
		self->target = nullptr;
		return;
	}

	// get entities standing on top
	VectorAdd(self->s.origin, self->r.maxs, maxs);
	VectorAdd(self->s.origin, self->r.mins, mins);

	mins[2] += (self->r.mins[2] + self->r.maxs[2]);
	maxs[2] += 32; // continue to heal jumping players but don't heal jetpack campers

	numPlayers = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);
	occupied = false;

	// mark occupied if still healing a player
	for (playerNum = 0; playerNum < numPlayers; playerNum++)
	{
		player = &g_entities[entityList[playerNum]];
		client = player->client;

		if (self->target.entity == player && PM_Live(client->ps.pm_type) &&
			(!player->entity->Get<HealthComponent>()->FullHealth() ||
				client->ps.stats[STAT_STAMINA] < STAMINA_MAX))
		{
			occupied = true;
		}
	}

	if (!occupied)
	{
		self->target = nullptr;
	}

	// clear poison, distribute medikits, find a new target if necessary
	for (playerNum = 0; playerNum < numPlayers; playerNum++)
	{
		player = &g_entities[entityList[playerNum]];
		client = player->client;

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

		HealthComponent* healthComponent = player->entity->Get<HealthComponent>();

		// give medikit to players with full health
		if (healthComponent->FullHealth())
		{
			if (!BG_InventoryContainsUpgrade(UP_MEDKIT, player->client->ps.stats))
			{
				BG_AddUpgradeToInventory(UP_MEDKIT, player->client->ps.stats);
			}
		}

		// if not already occupied, check if someone needs healing
		if (!occupied)
		{
			if (PM_Live(client->ps.pm_type) &&
				(!healthComponent->FullHealth() ||
					client->ps.stats[STAT_STAMINA] < STAMINA_MAX))
			{
				self->target = player;
				occupied = true;
			}
		}
	}

	// if we have a target, heal it
	if (self->target && self->target->client)
	{
		player = self->target.entity;
		client = player->client;
		client->ps.stats[STAT_STATE] |= SS_HEALING_2X;

		// start healing animation
		if (!self->medistationIsHealing)
		{
			self->medistationIsHealing = true;

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
		player->entity->Heal(1.0f, nullptr);

		// check if fully healed
		if (player->entity->Get<HealthComponent>()->FullHealth())
		{
			// give medikit
			if (!BG_InventoryContainsUpgrade(UP_MEDKIT, client->ps.stats))
			{
				BG_AddUpgradeToInventory(UP_MEDKIT, client->ps.stats);
			}
		}
	}
	// we lost our target
	else if (self->medistationIsHealing)
	{
		self->medistationIsHealing = false;

		// stop healing animation
		G_SetBuildableAnim(self, BANIM_ATTACK2, true);
		G_SetIdleBuildableAnim(self, BANIM_IDLE1);
	}
}
