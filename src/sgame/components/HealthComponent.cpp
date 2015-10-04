#include "HealthComponent.h"
#include "math.h"

static Log::Logger healthLogger("sgame.health");

HealthComponent::HealthComponent(Entity& entity, float maxHealth)
	: HealthComponentBase(entity, maxHealth), health(maxHealth)
{}

// TODO: Handle rewards array.
HealthComponent& HealthComponent::operator=(const HealthComponent& other) {
	health = (other.health / other.maxHealth) * maxHealth;
	return *this;
}

void HealthComponent::HandlePrepareNetCode() {
	int transmittedHealth = Math::Clamp((int)std::ceil(health), -999, 999);
	gclient_t *client = entity.oldEnt->client;

	if (client) {
		if (client->ps.stats[STAT_HEALTH] != transmittedHealth) {
			client->pers.infoChangeTime = level.time;
		}
		client->ps.stats[STAT_HEALTH] = transmittedHealth;
		client->ps.stats[STAT_MAX_HEALTH] = (int)std::ceil(maxHealth);
	} else if (entity.oldEnt->s.eType == ET_BUILDABLE) {
		entity.oldEnt->s.generic1 = std::max(transmittedHealth, 0);
	}
}

void HealthComponent::HandleHeal(float amount, gentity_t* source) {
	if (health <= 0.0f) return;
	if (health >= maxHealth) return;

	// Only heal up to maximum health.
	amount = std::min(amount, maxHealth - health);

	if (amount <= 0.0f) return;

	healthLogger.Debug("Healing: %3.1f (%3.1f → %3.1f)", amount, health, health + amount);

	health += amount;
	ScaleDamageAccounts(amount);
}

void HealthComponent::HandleDamage(float amount, gentity_t* source, Util::optional<Vec3> location,
Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	if (health <= 0.0f) return;
	if (amount <= 0.0f) return;

	gclient_t *client = entity.oldEnt->client;

	// Check for immunity.
	if (entity.oldEnt->flags & FL_GODMODE) return;
	if (client) {
		if (client->noclip) return;
		if (client->sess.spectatorState != SPECTATOR_NOT) return;
	}

	// Set source to world if missing.
	if (!source) source = &g_entities[ENTITYNUM_WORLD];

	// Don't handle ET_MOVER w/o die or pain function.
	// TODO: Handle mover special casing in a dedicated component.
	if (entity.oldEnt->s.eType == ET_MOVER && !(entity.oldEnt->die || entity.oldEnt->pain)) {
		// Special case for ET_MOVER with act function in initial position.
		if ((entity.oldEnt->moverState == MOVER_POS1 || entity.oldEnt->moverState == ROTATOR_POS1)
		    && entity.oldEnt->act) {
			entity.oldEnt->act(entity.oldEnt, source, source);
		}

		return;
	}

	// Check for protection.
	if (!(flags & DAMAGE_NO_PROTECTION)) {
		// Check for protection from friendly damage.
		if (entity.oldEnt != source && G_OnSameTeam(entity.oldEnt, source)) {
			// Check if friendly fire has been disabled.
			if (!g_friendlyFire.integer) return;

			// Never do friendly damage on movement attacks.
			switch (meansOfDeath) {
				case MOD_LEVEL3_POUNCE:
				case MOD_LEVEL4_TRAMPLE:
					return;

				default:
					break;
			}

			// If dretchpunt is enabled and this is a dretch, do dretchpunt instead of damage.
			// TODO: Add a message for pushing.
			if (g_dretchPunt.integer && client && client->ps.stats[STAT_CLASS] == PCL_ALIEN_LEVEL0)
			{
				vec3_t dir, push;

				VectorSubtract(entity.oldEnt->r.currentOrigin, source->r.currentOrigin, dir);
				VectorNormalizeFast(dir);
				VectorScale(dir, (amount * 10.0f), push);
				push[ 2 ] = 64.0f;

				VectorAdd( client->ps.velocity, push, client->ps.velocity );

				return;
			}
		}

		// Check for protection from friendly buildable damage. Never protect from building actions.
		// TODO: Use DAMAGE_NO_PROTECTION flag instead of listing means of death here.
		if (entity.oldEnt->s.eType == ET_BUILDABLE && source->client &&
		    meansOfDeath != MOD_DECONSTRUCT && meansOfDeath != MOD_SUICIDE &&
		    meansOfDeath != MOD_REPLACE     && meansOfDeath != MOD_NOCREEP) {
			if (G_OnSameTeam(entity.oldEnt, source) && !g_friendlyBuildableFire.integer) {
				return;
			}
		}
	}

	float take = amount;

	// Apply damage modifiers.
	if (!(flags & DAMAGE_PURE)) {
		entity.ApplyDamageModifier(take, location, direction, flags, meansOfDeath);
	}

	// Update combat timers.
	// TODO: Add a message to update combat timers.
	if (client && source->client && entity.oldEnt != source) {
		client->lastCombatTime = entity.oldEnt->client->lastCombatTime = level.time;
	}

	if (client) {
		// Save damage w/o armor modifier.
		client->damage_received += (int)(amount + 0.5f);

		// Save damage direction.
		if (direction) {
			VectorCopy(direction.value().Data(), client->damage_from);
			client->damage_fromWorld = false;
		} else {
			VectorCopy(entity.oldEnt->r.currentOrigin, client->damage_from);
			client->damage_fromWorld = true;
		}

		// Drain jetpack fuel.
		// TODO: Have another component handle jetpack fuel drain.
		client->ps.stats[STAT_FUEL] = std::max(0, client->ps.stats[STAT_FUEL] -
		                                       (int)(amount + 0.5f) * JETPACK_FUEL_PER_DMG);

		// If boosted poison every attack.
		// TODO: Add a poison message and a PoisonableComponent.
		if (source->client && (source->client->ps.stats[STAT_STATE] & SS_BOOSTED) &&
		    client->pers.team == TEAM_HUMANS && client->poisonImmunityTime < level.time) {
			switch (meansOfDeath) {
				case MOD_POISON:
				case MOD_LEVEL2_ZAP:
					break;

				default:
					client->ps.stats[STAT_STATE] |= SS_POISONED;
					client->lastPoisonTime   = level.time;
					client->lastPoisonClient = source;
					break;
			}
		}
	}

	healthLogger.Notice("Taking damage: %3.1f (%3.1f → %3.1f)", take, health, health - take);

	// Do the damage.
	health -= take;

	// Update team overlay info.
	if (client) client->pers.infoChangeTime = level.time;

	// TODO: Move lastDamageTime to HealthComponent.
	entity.oldEnt->lastDamageTime = level.time;

	// HACK: gentity_t.nextRegenTime only affects alien clients.
	// TODO: Catch damage message in a new RegenerationComponent.
	entity.oldEnt->nextRegenTime = level.time + ALIEN_CLIENT_REGEN_WAIT;

	// Handle non-self damage.
	if (entity.oldEnt != source) {
		float loss = take;

		if (health < 0.0f) loss += health;

		// TODO: Use ClientComponent.
		if (source->client) {
			// Add to the attacker's account on the target.
			// TODO: Move damage account array to HealthComponent.
			entity.oldEnt->credits[source->client->ps.clientNum].value += loss;
			entity.oldEnt->credits[source->client->ps.clientNum].time = level.time;
			entity.oldEnt->credits[source->client->ps.clientNum].team = (team_t)source->client->pers.team;

			// Notify the attacker of a hit.
			SpawnHitNotification(source);
		}
	}

	// Handle death.
	// TODO: Send a Die/Pain message and handle details where appropriate.
	if (health <= 0) {
		healthLogger.Notice("Dying with %.1f health.", health);

		// Disable knockback.
		if (client) entity.oldEnt->flags |= FL_NO_KNOCKBACK;

		// Call legacy die function.
		if (entity.oldEnt->die) entity.oldEnt->die(entity.oldEnt, source, source, meansOfDeath);

		// Send die message.
		entity.Die(source, meansOfDeath);

		// Trigger ON_DIE event.
		if(!client) G_EventFireEntity(entity.oldEnt, source, ON_DIE);
	} else if (entity.oldEnt->pain) {
		entity.oldEnt->pain(entity.oldEnt, source, (int)std::ceil(take));
	}
}

void HealthComponent::SetHealth(float health) {
	Math::Clamp(health, FLT_EPSILON, maxHealth);

	healthLogger.Debug("Changing health: %3.1f → %3.1f.", this->health, health);

	ScaleDamageAccounts(health - this->health);
	HealthComponent::health = health;
}

void HealthComponent::SetMaxHealth(float maxHealth, bool scaleHealth) {
	assert(maxHealth > 0.0f);

	healthLogger.Debug("Changing maximum health: %3.1f → %3.1f.", this->maxHealth, maxHealth);

	HealthComponent::maxHealth = maxHealth;
	if (scaleHealth) SetHealth(health * (this->maxHealth / maxHealth));
}

// TODO: Move credits array to HealthComponent.
void HealthComponent::ScaleDamageAccounts(float healthRestored) {
	if (healthRestored <= 0.0f) return;

	// Get total damage account and remember relevant clients.
	float totalAccreditedDamage = 0.0f;
	std::vector<Entity*> relevantClients;
	ForEntities<ClientComponent>([&](Entity& other, ClientComponent& client) {
		float clientDamage = entity.oldEnt->credits[other.oldEnt->s.number].value;
		if (clientDamage > 0.0f) {
			totalAccreditedDamage += clientDamage;
			relevantClients.push_back(&other);
		}
	});

	if (relevantClients.empty()) return;

	// Calculate account scale factor.
	float scale;
	if (healthRestored < totalAccreditedDamage) {
		scale = (totalAccreditedDamage - healthRestored) / totalAccreditedDamage;

		healthLogger.Debug("Scaling damage accounts of %i client(s) by %.2f.",
		                   relevantClients.size(), scale);
	} else {
		// Clear all accounts.
		scale = 0.0f;

		healthLogger.Debug("Clearing damage accounts of %i client(s).", relevantClients.size());
	}

	// Scale down or clear damage accounts.
	for (Entity* other : relevantClients) {
		entity.oldEnt->credits[other->oldEnt->s.number].value *= scale;
	}
}

// TODO: Replace this with a call to a proper event factory.
void HealthComponent::SpawnHitNotification(gentity_t *attacker)
{
	if (!attacker->client) return;

	gentity_t *event = G_NewTempEntity(attacker->s.origin, EV_HIT);
	event->r.svFlags = SVF_SINGLECLIENT;
	event->r.singleClient = attacker->client->ps.clientNum;
}
