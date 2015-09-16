#include "KnockbackComponent.h"

static Log::Logger knockbackLogger("sgame.knockback");

KnockbackComponent::KnockbackComponent(Entity& entity)
	: KnockbackComponentBase(entity)
{}

// TODO: Consider location as well as direction when both given.
void KnockbackComponent::HandleDamage(float amount, gentity_t* source, Util::optional<Vec3> location,
                                      Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	if (!(flags & DAMAGE_KNOCKBACK)) return;
	if (amount <= 0.0f) return;

	if (!direction) {
		knockbackLogger.Warn("Received damage message with knockback flag set but no direction.");
		return;
	}

	if (Math::Length(direction.value()) == 0.0f) {
		knockbackLogger.Warn("Attempt to do knockback with null vector direction.");
		return;
	}

	// TODO: Remove dependency on client.
	gclient_t *client = entity.oldEnt->client;
	assert(client);

	// Check for immunity.
	if (client->noclip) return;
	if (client->sess.spectatorState != SPECTATOR_NOT) return;

	float mass = (float)BG_Class(client->ps.stats[ STAT_CLASS ])->mass;

	if (mass <= 0.0f) {
		knockbackLogger.Warn("Attempt to do knockback against target with no mass, assuming normal mass.");
		mass = KNOCKBACK_NORMAL_MASS;
	}

	float massMod  = Math::Clamp(KNOCKBACK_NORMAL_MASS / mass, KNOCKBACK_MIN_MASSMOD, KNOCKBACK_MAX_MASSMOD);
	float strength = amount * DAMAGE_TO_KNOCKBACK * massMod;

	// Change client velocity.
	Vec3 clientVelocity = Vec3::Load(client->ps.velocity);
	clientVelocity += Math::Normalize(direction.value()) * strength;
	clientVelocity.Store(client->ps.velocity);

	// Set pmove timer so that the client can't cancel out the movement immediately.
	if (!client->ps.pm_time) {
		client->ps.pm_time = KNOCKBACK_PMOVE_TIME;
		client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}

	knockbackLogger.Debug("Knockback: client: %i, strength: %.1f (massMod: %.1f).",
	                      entity.oldEnt->s.number, strength, massMod);
}
