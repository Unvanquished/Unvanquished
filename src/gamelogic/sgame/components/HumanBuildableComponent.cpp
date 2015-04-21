#include "HumanBuildableComponent.h"

HumanBuildableComponent::HumanBuildableComponent(Entity& entity, BuildableComponent& r_BuildableComponent)
	: HumanBuildableComponentBase(entity, r_BuildableComponent)
{}

void HumanBuildableComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	// Cancel construction or prepare blast.
	if (entity.oldEnt->spawned) {
		entity.oldEnt->think = HGeneric_Blast;

		// Play a warning sound before ractor and repeater explosion.
		// Don't randomize blast delay for them so the sound stays synced.
		// TODO: Move to Repeater/ReactorComponent.
		switch (entity.oldEnt->s.modelindex) {
			case BA_H_REPEATER:
			case BA_H_REACTOR:
				G_AddEvent(entity.oldEnt, EV_HUMAN_BUILDABLE_DYING, 0);
				entity.oldEnt->nextthink = level.time + HUMAN_DETONATION_DELAY;
				break;

			default:
				entity.oldEnt->nextthink = level.time + HUMAN_DETONATION_RAND_DELAY;
		}
	} else {
		entity.oldEnt->think = HGeneric_Cancel;
		entity.oldEnt->nextthink = level.time;
	}

	// Warn if this building was powered and there's a watcher nearby.
	gentity_t *rc = G_ActiveReactor();
	if (entity.oldEnt != rc && entity.oldEnt->powered && G_IsWarnableMOD(meansOfDeath)) {
		bool inBase = G_InsideBase(entity.oldEnt, true);
		gentity_t *watcher  = nullptr, *location = nullptr;

		// Note that being inside the main base doesn't mean there always is an active reactor.
		if (inBase) watcher = G_ActiveReactor();

		// Note that a repeater will find itself as nearest power source.
		if (!watcher) watcher = G_NearestPowerSourceInRange(entity.oldEnt);

		// Get a location entity close to watcher.
		if (watcher) {
			location = Team_GetLocation(watcher);

			// Fall back to fake location entity if necessary.
			if (!location) location = level.fakeLocation;
		}

		// Warn if there was no warning for this location recently.
		if (location && level.time > location->warnTimer) {
			location->warnTimer = level.time + ATTACKWARN_NEARBY_PERIOD;
			G_BroadcastEvent(EV_WARN_ATTACK, inBase ? 0 : watcher->s.number, TEAM_HUMANS);
			Beacon::NewArea(BCT_DEFEND, entity.oldEnt->s.origin, entity.oldEnt->buildableTeam);
		}
	}
}
