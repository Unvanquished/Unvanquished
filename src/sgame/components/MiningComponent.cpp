#include "MiningComponent.h"
#include "../Entities.h"

MiningComponent::MiningComponent(Entity& entity, ThinkingComponent& r_ThinkingComponent)
	: MiningComponentBase(entity, r_ThinkingComponent)
	, active(false) {

	// Already calculate the predicted efficiency.
	CalculateEfficiency();

	// Inform neighbouring miners so they can adjust their own predictions.
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

float MiningComponent::InterferenceMod(float distance) {
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

MiningComponent::Efficiencies MiningComponent::FindEfficiencies(
	team_t team, const glm::vec3& location, MiningComponent* skip)
{
	MiningComponent::Efficiencies efficiencies{ 1.0f, 1.0f };

	for (MiningComponent& miningComponent : Entities::Each<MiningComponent>()) {
		if (&miningComponent == skip) continue;

		// Do not consider dead neighbours, even when predicting, as they can never become active.
		if (!Entities::IsAlive(miningComponent.entity)) continue;

		float interferenceMod = InterferenceMod(glm::distance(location, VEC2GLM(miningComponent.entity.oldEnt->s.origin)));

		// Enemy miners under construction are a secret
		if (!(G_Team(miningComponent.entity.oldEnt) != team && !miningComponent.active)) {
			efficiencies.predicted *= interferenceMod;
		}

		if (miningComponent.active) {
			efficiencies.actual *= interferenceMod;
		}
	}
	return efficiencies;
}

void MiningComponent::CalculateEfficiency() {
	Efficiencies efficiencies = FindEfficiencies(
		G_Team(entity.oldEnt), VEC2GLM(entity.oldEnt->s.origin), this);
	currentEfficiency = active ? efficiencies.actual : 0.0f;
	predictedEfficiency = efficiencies.predicted;
}

void MiningComponent::InformNeighbors() {
	for (Entity& other : Entities::Having<MiningComponent>()) {
		if (&other == &entity) continue;
		if (G_Distance(entity.oldEnt, other.oldEnt) > RGS_RANGE * 2.0f) continue;

		other.Get<MiningComponent>()->CalculateEfficiency();
	}
}

float MiningComponent::Efficiency(bool predict) {
	return predict ? predictedEfficiency : currentEfficiency;
}
