#include "ArmorComponent.h"

ArmorComponent::ArmorComponent(Entity& entity)
	: ArmorComponentBase(entity)
{}

// TODO: Use new vector math library for all calculations.
float ArmorComponent::DamageModifier(Util::optional<Vec3> location, int damageFlags) {
	vec3_t origin, bulletPath, bulletAngle, locationRelativeToFloor, floor, normal;

	// TODO: Make ArmorComponent depend on ClientComponent or allow armor on all entities.
	if (!entity.oldEnt->client) return 1.0f;

	// Use non-regional damage where appropriate.
	if (damageFlags & DAMAGE_NO_LOCDAMAGE || !location) {
		// TODO: Move G_GetNonLocDamageMod to ArmorComponent.
		return G_GetNonLocDamageMod((class_t)entity.oldEnt->client->ps.stats[STAT_CLASS]);
	}

	// Get hit location relative to the floor beneath.
	if (g_unlagged.integer && entity.oldEnt->client->unlaggedCalc.used) {
		VectorCopy(entity.oldEnt->client->unlaggedCalc.origin, origin);
	} else {
		VectorCopy(entity.oldEnt->r.currentOrigin, origin);
	}
	BG_GetClientNormal(&entity.oldEnt->client->ps, normal);
	VectorMA(origin, entity.oldEnt->r.mins[2], normal, floor);
	VectorSubtract(location.value().Data(), floor, locationRelativeToFloor);

	// Get fraction of height where the hit landed.
	float height = entity.oldEnt->r.maxs[2] - entity.oldEnt->r.mins[2];
	if (!height) height = 1.0f;
	float hitRatio = Math::Clamp(DotProduct(normal, locationRelativeToFloor) / VectorLength(normal),
	                             0.0f, height) / height;

	// Get the yaw of the attack relative to the target's view yaw
	VectorSubtract(location.value().Data(), origin, bulletPath);
	vectoangles(bulletPath, bulletAngle);
	int hitRotation = AngleNormalize360(entity.oldEnt->client->ps.viewangles[YAW] - bulletAngle[YAW]);

	// Use regional modifier.
	// TODO: Move G_GetPointDamageMod to ArmorComponent.
	return G_GetPointDamageMod(entity.oldEnt, (class_t)entity.oldEnt->client->ps.stats[STAT_CLASS],
	                           hitRotation, hitRatio);
}
