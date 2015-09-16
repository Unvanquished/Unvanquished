#include "ArmorComponent.h"

static Log::Logger armorLogger("sgame.armor");

ArmorComponent::ArmorComponent(Entity& entity)
	: ArmorComponentBase(entity)
{}

float ArmorComponent::GetNonLocationalDamageMod() {
	class_t pcl = (class_t)entity.oldEnt->client->ps.stats[STAT_CLASS];

	for (int regionNum = 0; regionNum < g_numDamageRegions[pcl]; regionNum++) {
		damageRegion_t *region = &g_damageRegions[pcl][regionNum];

		if (!region->nonlocational) continue;

		armorLogger.Debug("Found non-locational damage modifier of %.2f.", region->modifier);

		return region->modifier;
	}

	armorLogger.Debug("No non-locational damage modifier found.");

	return 1.0f;
}

float ArmorComponent::GetLocationalDamageMod(float angle, float height) {
	class_t pcl = (class_t)entity.oldEnt->client->ps.stats[STAT_CLASS];

	bool crouching = (entity.oldEnt->client->ps.pm_flags & PMF_DUCKED);

	for (int regionNum = 0; regionNum < g_numDamageRegions[pcl]; regionNum++) {
		damageRegion_t *region = &g_damageRegions[pcl][regionNum];

		// Ignore the non-locational pseudo region.
		if (region->nonlocational) continue;

		// Crouch state must match.
		if (region->crouch != crouching) continue;

		// Height must be within given range.
		if (height < region->minHeight || height > region->maxHeight) continue;

		// Angle must be within given range.
		if ((region->minAngle <= region->maxAngle && (angle < region->minAngle || angle > region->maxAngle)) ||
		    (region->minAngle >  region->maxAngle && (angle > region->maxAngle && angle < region->minAngle))) {
			continue;
		}

		armorLogger.Debug("Locational damage modifier of %.2f found for angle %.2f and height %.2f (%s).",
		                  region->modifier, angle, height, region->name);

		return region->modifier;
	}

	armorLogger.Debug("Locational damage modifier for angle %.2f and height %.2f not found.",
	                  angle, height);

	return 1.0f;
}


// TODO: Use new vector math library for all calculations.
void ArmorComponent::HandleApplyDamageModifier(float& damage, Util::optional<Vec3> location,
Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	vec3_t origin, bulletPath, bulletAngle, locationRelativeToFloor, floor, normal;

	// TODO: Remove dependency on client.
	assert(entity.oldEnt->client);

	// Use non-regional damage where appropriate.
	if (flags & DAMAGE_NO_LOCDAMAGE || !location) {
		// TODO: Move G_GetNonLocDamageMod to ArmorComponent.
		damage *= GetNonLocationalDamageMod();
		return;
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
	damage *= GetLocationalDamageMod(hitRotation, hitRatio);
}
