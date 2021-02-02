#include "MainBuildableComponent.h"

MainBuildableComponent::MainBuildableComponent(Entity& entity, BuildableComponent& r_BuildableComponent)
	: MainBuildableComponentBase(entity, r_BuildableComponent)
{}

void MainBuildableComponent::HandleDamage(float /*amount*/, gentity_t* /*source*/, Util::optional<Vec3> /*location*/,
                                          Util::optional<Vec3> /*direction*/, int /*flags*/, meansOfDeath_t meansOfDeath) {
	// Warn the team about an attack.
	if (G_IsWarnableMOD(meansOfDeath)) {
		float healthFraction = GetBuildableComponent().GetHealthComponent().HealthFraction();

		int warnLevel = 0;
		if (healthFraction < 0.75f) warnLevel++;
		if (healthFraction < 0.50f) warnLevel++;
		if (healthFraction < 0.25f) warnLevel++;

		if (warnLevel && (warnLevel > lastAttackWarnLevel || level.time > nextAttackWarning)) {
			nextAttackWarning   = level.time + ATTACKWARN_PRIMARY_PERIOD;
			lastAttackWarnLevel = warnLevel;

			// TODO: Use TeamComponent/LocationComponent.
			G_BroadcastEvent(EV_MAIN_UNDER_ATTACK, warnLevel, entity.oldEnt->buildableTeam);
			Beacon::NewArea(BCT_DEFEND, entity.oldEnt->s.origin, entity.oldEnt->buildableTeam);
		}
	}
}

void MainBuildableComponent::HandleDie(gentity_t* /*killer*/, meansOfDeath_t meansOfDeath) {
	G_UpdateBuildPointBudgets();

	if (G_IsWarnableMOD(meansOfDeath)) {
		// TODO: Use TeamComponent.
		G_BroadcastEvent(EV_MAIN_DYING, 0, entity.oldEnt->buildableTeam);
	}
}

void MainBuildableComponent::HandleFinishConstruction() {
	G_UpdateBuildPointBudgets();

	// TODO: Generate event that informs team here.
}
