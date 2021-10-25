#include "MiningComponent.h"

MiningComponent::MiningComponent(Entity& entity, bool blueprint,
                                 ThinkingComponent& r_ThinkingComponent)
	: MiningComponentBase(entity, blueprint, r_ThinkingComponent)
	, active(false) {

	// Already calculate the predicted efficiency.
	CalculateEfficiency();

	// Inform neighbouring miners so they can adjust their own predictions.
	// Note that blueprint miners skip this.
	InformNeighbors();

	REGISTER_THINKER(Think, ThinkingComponent::SCHEDULER_AVERAGE, 250);
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
}

float MiningComponent::AliveTimePercentage() const {
	constexpr float fullyActiveTime =  3.0f * 60.0f * 1000.0f; // 3 minutes

	Log::Warn("creation time: %i", entity.oldEnt->creationTime);
	if (entity.oldEnt->creationTime < 1000)
		return 1.0f; // spawned with world, can be considered alive since long

	// TODO: use time after activating, and not after spawning
	float activeTime = static_cast<float>(
			level.time - entity.oldEnt->creationTime);

	return Math::Clamp(activeTime / fullyActiveTime, 0.0f, 1.0f);
}

float MiningComponent::AliveTimeMod() const {
	return AliveTimePercentage();
}

float MiningComponent::PredictedInterferenceMod(float distance) {
	if (RGS_RANGE <= 0.0f) return 1.0f;
	if (distance > 2.0f * RGS_RANGE) return 1.0f;

	// q is the ratio of the part of a sphere with radius RGS_RANGE that intersects
	// with another sphere of equal size and given distance
	float dr = distance / RGS_RANGE;
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

		float interferenceMod = PredictedInterferenceMod(G_Distance(entity.oldEnt, other.oldEnt));

		// TODO: Exclude enemy miners in construction from the prediction.

		predictedEfficiency *= interferenceMod;

		// Current efficiency is zero when not active.
		if (!active) return;

		// Only consider active neighbours for the current efficiency.
		if (!miningComponent.Active()) return;

		currentEfficiency *= interferenceMod;
		currentEfficiency *= AliveTimeMod();
	});
}

void MiningComponent::InformNeighbors() {
	// Blueprint miners cause neither real nor predicted interference, so no need to tell neighbours
	// about them.
	if (blueprint) return;

	ForEntities<MiningComponent>([&] (Entity& other, MiningComponent& miningComponent) {
		if (&other == &entity) return;
		if (G_Distance(entity.oldEnt, other.oldEnt) > RGS_RANGE * 2.0f) return;

		miningComponent.CalculateEfficiency();
	});
}

float MiningComponent::Efficiency(bool predict) {
	return predict ? predictedEfficiency : currentEfficiency;
}

void MiningComponent::Think(int)
{
	// Already calculate the predicted efficiency.
	CalculateEfficiency();

	// Inform neighbouring miners so they can adjust their own predictions.
	// Note that blueprint miners skip this.
	InformNeighbors();

	// Update both team's budgets.
	G_UpdateBuildPointBudgets();

	// TODO: stop thinking after some time
	//GetThinkingComponent().UnregisterActiveThinker();
}
