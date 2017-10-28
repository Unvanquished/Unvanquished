#include "ZapComponent.h"

namespace
{
	void FindZapChainTargets(zap_t *zap)
	{
		gentity_t *ent = zap->targets[0]; // the source
		int entityList[MAX_GENTITIES];
		vec3_t range;
		vec3_t mins, maxs;
		int i, num;
		gentity_t *enemy;
		trace_t tr;
		float distance;

		VectorSet(range, LEVEL2_AREAZAP_CHAIN_RANGE, LEVEL2_AREAZAP_CHAIN_RANGE, LEVEL2_AREAZAP_CHAIN_RANGE);

		VectorAdd(ent->s.origin, range, maxs);
		VectorSubtract(ent->s.origin, range, mins);

		// Always reset number of targets to 1 so we can rediscover new chains.
		zap->numTargets = 1;

		num = trap_EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

		for (i = 0; i < num; i++)
		{
			enemy = &g_entities[entityList[i]];

			// don't chain to self; noclippers can be listed, don't chain to them either
			if (enemy == ent || (enemy->client && enemy->client->noclip))
			{
				continue;
			}

			distance = Distance(ent->s.origin, enemy->s.origin);

			if (((enemy->client &&
				  enemy->client->pers.team == TEAM_HUMANS) ||
				 (enemy->s.eType == entityType_t::ET_BUILDABLE &&
				  BG_Buildable(enemy->s.modelindex)->team == TEAM_HUMANS)) &&
				enemy->entity->Get<HealthComponent>()->Alive() &&
				distance <= LEVEL2_AREAZAP_CHAIN_RANGE)
			{
				// world-LOS check: trace against the world, ignoring other BODY entities
				trap_Trace(&tr, ent->s.origin, nullptr, nullptr,
						   enemy->s.origin, ent->s.number, CONTENTS_SOLID, 0);

				if (tr.entityNum == ENTITYNUM_NONE)
				{
					zap->targets[zap->numTargets] = enemy;
					zap->distances[zap->numTargets] = distance;

					if (++zap->numTargets >= LEVEL2_AREAZAP_MAX_TARGETS)
					{
						return;
					}
				}
			}
		}
	}

	void UpdateZapEffect(const glm::vec3& muzzle, zap_t *zap)
	{
		int i;
		int entityNums[LEVEL2_AREAZAP_MAX_TARGETS + 1];

		entityNums[0] = zap->creator->s.number;

		ASSERT_LE(zap->numTargets, LEVEL2_AREAZAP_MAX_TARGETS);

		for (i = 0; i < zap->numTargets; i++)
		{
			entityNums[i + 1] = zap->targets[i]->s.number;
		}

		BG_PackEntityNumbers(&zap->effectChannel->s,
							 entityNums, zap->numTargets + 1);

		G_SetOrigin(zap->effectChannel, muzzle);
		trap_LinkEntity(zap->effectChannel);
	}

} // namespace

ZapComponent::ZapComponent(Entity &entity, ThinkingComponent &r_ThinkingComponent)
	: ZapComponentBase(entity, r_ThinkingComponent)
{
	REGISTER_THINKER(UpdateZap, ThinkingComponent::SCHEDULER_AVERAGE, 100);
	zap = zap_t{};
	zap.creator = entity.oldEnt;
}

void ZapComponent::HandleZapTarget(Entity &target)
{
	if (zap.used)
		return;
	zap.timeAlive = 0;
	zap.timeWrongTarget = 0;
	zap.used = true;
	glm::vec3 forward, muzzle;
	muzzle = G_CalcMuzzlePoint(entity.oldEnt, forward);
	zap.targets[0] = target.oldEnt;
	zap.numTargets = 1;
	if (target.Damage(static_cast<float>(LEVEL2_AREAZAP_DMG), zap.creator, glm::make_vec3(target.oldEnt->s.origin),
					  glm::make_vec3(forward), DAMAGE_NO_LOCDAMAGE, MOD_LEVEL2_ZAP))
	{
		FindZapChainTargets(&zap);
		for (int i = 1; i < zap.numTargets; ++i)
		{
			float damage = LEVEL2_AREAZAP_DMG * (1 - powf((zap.distances[i] /
														   LEVEL2_AREAZAP_CHAIN_RANGE),
														  LEVEL2_AREAZAP_CHAIN_FALLOFF)) +
						   1;
			target.Damage(damage, zap.creator, glm::make_vec3(target.oldEnt->s.origin),
						  glm::make_vec3(forward), DAMAGE_NO_LOCDAMAGE, MOD_LEVEL2_ZAP);
		}
	}
	zap.effectChannel = G_NewEntity();
	zap.effectChannel->s.eType = entityType_t::ET_LEV2_ZAP_CHAIN;
	zap.effectChannel->classname = "lev2zapchain";
	UpdateZapEffect(muzzle, &zap);
}

void ZapComponent::HandleClearZap(Entity &player)
{
	if (!zap.used)
		return;
	glm::vec3 muzzle, forward;
	muzzle = G_CalcMuzzlePoint(entity.oldEnt, forward);
	// the disappearance of the creator or the first target destroys the whole zap effect
	if (zap.creator == player.oldEnt || zap.targets[0] == player.oldEnt)
	{
		G_FreeEntity(zap.effectChannel);
		zap.used = false;
	}

	// the disappearance of chained players destroy the appropriate beams
	for (int j = 1; j < zap.numTargets; j++)
	{
		if (zap.targets[j] == player.oldEnt)
		{
			zap.targets[j--] = zap.targets[--zap.numTargets];
		}
	}
}

void ZapComponent::UpdateZap(int timeDelta)
{
	if (!zap.used || !zap.targets[0])
		return;
	glm::vec3 muzzle, forward;
	muzzle = G_CalcMuzzlePoint(entity.oldEnt, forward);

	trace_t tr;
	gentity_t *traceEnt;

	G_WideTrace(&tr, this->entity.oldEnt, LEVEL2_AREAZAP_RANGE, LEVEL2_AREAZAP_WIDTH, LEVEL2_AREAZAP_WIDTH, &muzzle[0], &forward[0], &traceEnt);
	HealthComponent *targetHealth = zap.targets[0]->entity->Get<HealthComponent>();
	if (traceEnt == nullptr || traceEnt != zap.targets[0] || !targetHealth || !targetHealth->Alive())
	{

		// If you're not aiming at them at this moment, you have some time to aim at them again, however
		// if you are out of range, then you stop zapping.
		if (zap.timeWrongTarget < LEVEL2_AREAZAP_TIME && G_Distance(zap.targets[0], entity.oldEnt) < LEVEL2_AREAZAP_RANGE)
		{
			// Fall though and execute the rest of the attack code.
			zap.timeWrongTarget += timeDelta;
		}
		else
		{
			G_FreeEntity(zap.effectChannel);
			zap.used = false;
			for (int j = 1; j < zap.numTargets; j++)
			{
				if (!zap.targets[j]->inuse)
				{
					zap.targets[j--] = zap.targets[--zap.numTargets];
				}
			}
			return;
		}
	}
	else
	{
		zap.timeWrongTarget = 0;
	}

	float baseDamage = LEVEL2_AREAZAP_DMG * (static_cast<float>(zap.timeAlive) / 1000);
	if (zap.targets[0]->entity->Damage(baseDamage, zap.creator, glm::make_vec3(zap.targets[0]->s.origin),
									   glm::make_vec3(forward), DAMAGE_NO_LOCDAMAGE, MOD_LEVEL2_ZAP))
	{
		FindZapChainTargets(&zap);
		for (int i = 1; i < zap.numTargets; ++i)
		{
			gentity_t *target = zap.targets[i];
			float damage = baseDamage * (1 - powf((zap.distances[i] /
												   LEVEL2_AREAZAP_CHAIN_RANGE),
												  LEVEL2_AREAZAP_CHAIN_FALLOFF)) +
						   1;
			target->entity->Damage(damage, zap.creator, glm::make_vec3(target->s.origin),
								   glm::make_vec3(forward), DAMAGE_NO_LOCDAMAGE, MOD_LEVEL2_ZAP);
		}
	}
	zap.timeAlive += timeDelta;
	UpdateZapEffect(muzzle, &zap);
}
