#include "AlienBuildableComponent.h"
#include <random>

AlienBuildableComponent::AlienBuildableComponent(Entity& entity, BuildableComponent& r_BuildableComponent,
                                                 IgnitableComponent& r_IgnitableComponent)
	: AlienBuildableComponentBase(entity, r_BuildableComponent, r_IgnitableComponent)
{}

void AlienBuildableComponent::HandleDamage(float amount, gentity_t* source, Util::optional<Vec3> location,
                                           Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	if (!GetBuildableComponent().GetHealthComponent().Alive()) {
		return;
	}

	// TODO: Move animation code to BuildableComponent.
	G_SetBuildableAnim(entity.oldEnt, BANIM_PAIN1, false);
}

void AlienBuildableComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	entity.oldEnt->think = AGeneric_Blast;
	entity.oldEnt->powered = false;

	// Warn if in main base and there's an overmind.
	gentity_t *om;
	if ((om = G_ActiveOvermind()) && om != entity.oldEnt && level.time > om->warnTimer
			&& G_InsideBase(entity.oldEnt, true) && G_IsWarnableMOD(meansOfDeath)) {
		om->warnTimer = level.time + ATTACKWARN_NEARBY_PERIOD;
		G_BroadcastEvent(EV_WARN_ATTACK, 0, TEAM_ALIENS);
		Beacon::NewArea(BCT_DEFEND, entity.oldEnt->s.origin, entity.oldEnt->buildableTeam);
	}

	// Set blast timer.
	if (entity.oldEnt->spawned && GetBuildableComponent().GetHealthComponent().Health() /
			GetBuildableComponent().GetHealthComponent().MaxHealth() > -1.0f) {
		entity.oldEnt->nextthink = level.time + ALIEN_DETONATION_RAND_DELAY;
	} else {
		entity.oldEnt->nextthink = level.time;
	}
}
