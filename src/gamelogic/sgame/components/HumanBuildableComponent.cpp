#include "HumanBuildableComponent.h"

static Log::Logger humanBuildableLogger("sgame.humanbuildings");

HumanBuildableComponent::HumanBuildableComponent(Entity& entity, BuildableComponent& r_BuildableComponent)
	: HumanBuildableComponentBase(entity, r_BuildableComponent)
{}

void HumanBuildableComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	switch (GetBuildableComponent().GetState()) {
		// Regular death, fully constructed.
		case BuildableComponent::CONSTRUCTED:
			// Play a warning sound before ractor and repeater explosion. Don't randomize blast
			// delay for them so the sound stays synced.
			// TODO: Move to Repeater/ReactorComponent if possible.
			switch (entity.oldEnt->s.modelindex) {
				case BA_H_REPEATER:
				case BA_H_REACTOR:
					G_AddEvent(entity.oldEnt, EV_HUMAN_BUILDABLE_DYING, 0);

					GetBuildableComponent().REGISTER_THINKER(
						Blast, ThinkingComponent::SCHEDULER_BEFORE, HUMAN_DETONATION_DELAY
					);
					break;

				default:
					GetBuildableComponent().REGISTER_THINKER(
						Blast, ThinkingComponent::SCHEDULER_BEFORE, HUMAN_DETONATION_RAND_DELAY
					);
			}
			GetBuildableComponent().SetState(BuildableComponent::PRE_BLAST);
			humanBuildableLogger.Notice("Human buildable is going to blow up.");
			break;

		// Construction was canceled.
		case BuildableComponent::CONSTRUCTING:
			G_RewardAttackers(entity.oldEnt);
			entity.FreeAt(DeferredFreeingComponent::FREE_AFTER_THINKING);
			humanBuildableLogger.Notice("Human buildable was canceled.");
			break;

		default:
			humanBuildableLogger.Warn("Handling human buildable death event when not alive.");
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

void HumanBuildableComponent::Blast(int timeDelta) {
	float          splashDamage = (float)entity.oldEnt->splashDamage;
	float          splashRadius = (float)entity.oldEnt->splashRadius;
	meansOfDeath_t splashMOD    = (meansOfDeath_t)entity.oldEnt->splashMethodOfDeath;

	humanBuildableLogger.Notice("Human buildable is exploding.");

	// Damage close entity.
	Utility::KnockbackRadiusDamage(entity, splashDamage, splashRadius, splashMOD);

	// Reward attackers.
	G_RewardAttackers(entity.oldEnt);

	// Stop collisions, add blast event and update buildable state.
	entity.oldEnt->r.contents = 0; trap_LinkEntity(entity.oldEnt);
	Vec3 blastDirection(0.0f, 0.0f, 1.0f);
	G_AddEvent(entity.oldEnt, EV_HUMAN_BUILDABLE_EXPLOSION, DirToByte(blastDirection.Data()));
	GetBuildableComponent().SetState(BuildableComponent::POST_BLAST); // Makes entity invisible.

	// Remove right after blast.
	entity.oldEnt->freeAfterEvent = true;
	GetBuildableComponent().GetThinkingComponent().UnregisterActiveThinker();
}
