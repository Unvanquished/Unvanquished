#include "MiningComponent.h"

MiningComponent::MiningComponent(Entity& entity, bool blueprint,
                                 ThinkingComponent& r_ThinkingComponent,
								 TeamComponent& r_TeamComponent)
	: MiningComponentBase(entity, blueprint, r_ThinkingComponent, r_TeamComponent)
	, active(false) {

	// Already calculate the predicted efficiency.
	CalculateEfficiency();

	// Inform neighbouring miners so they can adjust their own predictions.
	// Note that blueprint miners skip this.
	InformNeighbors();
}

void MiningComponent::HandlePrepareNetCode() {
	// Mining efficiency.
	entity.oldEnt->s.weaponAnim = (int)std::round(Efficiency() * (float)0xff);

	// TODO: Transmit budget grant.
}

void MiningComponent::HandleFinishConstruction() {
	active = true;

	// Now that we are active, calculate the current efficiency.
	CalculateEfficiency();

	// Inform neighbouring miners so they can react immediately.
	InformNeighbors();

	// Update both team's budgets.
	G_UpdateBuildPointBudgets();

	level.team[GetTeamComponent().Team()].numMiners++;
}

void MiningComponent::HandleDie(gentity_t* /*killer*/, meansOfDeath_t /*meansOfDeath*/) {
	active = false;

	// Efficiency will be zero from now on.
	currentEfficiency   = 0.0f;
	predictedEfficiency = 0.0f;

	// Inform neighbouring miners so they can react immediately.
	InformNeighbors();

	// Update both team's budgets.
	G_UpdateBuildPointBudgets();

	level.team[GetTeamComponent().Team()].numMiners--;
}

float MiningComponent::InterferenceMod(float distance) {
	if (g_minerRange.Get() <= 0.0f) return 1.0f;
	if (distance > 2.0f * g_minerRange.Get()) return 1.0f;

	// q is the ratio of the part of a sphere with radius g_minerRange.Get() that intersects
	// with another sphere of equal size and given distance
	float dr = distance / g_minerRange.Get();
	float q  = ((dr * dr * dr) - 12.0f * dr + 16.0f) / 16.0f;

	// Two miners together should mine at a rate proportional to the volume of the
	// union of their areas of effect. If more miners intersect, this is just an
	// approximation that tends to punish cluttering of miners.
	return ((1.0f - q) + 0.5f * q);
}

void MiningComponent::CalculateEfficiency() {
	currentEfficiency   = active ? 1.0f : 0.0f;
	predictedEfficiency = 1.0f;

	ForEntities<MiningComponent>([&] (Entity& other, MiningComponent& miningComponent) {
		if (&other == &entity) return;

		// Never consider blueprint miners.
		if (miningComponent.Blueprint()) return;

		// Do not consider dead neighbours, even when predicting, as they can never become active.
		HealthComponent *healthComponent = other.Get<HealthComponent>();
		if (healthComponent && !healthComponent->Alive()) return;

		float interferenceMod = InterferenceMod(G_Distance(entity.oldEnt, other.oldEnt));

		// TODO: Exclude enemy miners in construction from the prediction.

		predictedEfficiency *= interferenceMod;

		// Current efficiency is zero when not active.
		if (!active) return;

		// Only consider active neighbours for the current efficiency.
		if (!miningComponent.Active()) return;

		currentEfficiency *= interferenceMod;
	});
}

void MiningComponent::InformNeighbors() {
	// Blueprint miners cause neither real nor predicted interference, so no need to tell neighbours
	// about them.
	if (blueprint) return;

	ForEntities<MiningComponent>([&] (Entity& other, MiningComponent& miningComponent) {
		if (&other == &entity) return;
		if (G_Distance(entity.oldEnt, other.oldEnt) > g_minerRange.Get() * 2.0f) return;

		miningComponent.CalculateEfficiency();
	});
}

float MiningComponent::Efficiency(bool predict) {
	return predict ? predictedEfficiency : currentEfficiency;
}
