#include "MiningComponent.h"

MiningComponent::MiningComponent(Entity& entity, ResourceStorageComponent& r_ResourceStorageComponent,
                                 ThinkingComponent& r_ThinkingComponent)
	: MiningComponentBase(entity, r_ResourceStorageComponent, r_ThinkingComponent)
    , efficiency(0.0f)
	, lastThinkActive(false) {
	REGISTER_THINKER(Think, ThinkingComponent::SCHEDULER_AVERAGE, 1000);
}

void MiningComponent::HandlePrepareNetCode() {
	// Mining efficiency.
	entity.oldEnt->s.weaponAnim = (int)std::round(Efficiency() * 255.0f);
}

void MiningComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	efficiency = 0.0f;

	// Inform neighbours so they can increase their rate immediately.
	InformNeighbors();
}

bool MiningComponent::Active() {
	BuildableComponent *buildableComponent = entity.Get<BuildableComponent>();
	HealthComponent    *healthComponent    = entity.Get<HealthComponent>();

	// Buildables must be active and entities with health must be alive to mine.
	return ((!buildableComponent || buildableComponent->Active()) &&
	        (!healthComponent    || healthComponent->Alive()));
}

float MiningComponent::InterferenceMod(float distance) {
	if (RGS_RANGE <= 0.0f) return 1.0f;
	if (distance > 2.0f * RGS_RANGE) return 1.0f;

	// q is the ratio of the part of a sphere with radius RGS_RANGE that intersects
	// with another sphere of equal size and given distance
	float dr = distance / RGS_RANGE;
	float q  = ((dr * dr * dr) - 12.0f * dr + 16.0f) / 16.0f;

	// Two RGS together should mine at a rate proportional to the volume of the
	// union of their areas of effect. If more RGS intersect, this is just an
	// approximation that tends to punish cluttering of RGS.
	return ((1.0f - q) + 0.5f * q);
}

void MiningComponent::CalculateEfficiency() {
	if (!Active()) {
		efficiency = 0.0f;
		return;
	}

	float newEfficiency = 1.0f;

	ForEntities<MiningComponent>([&] (Entity& other, MiningComponent& miningComponent) {
		if (&other == &entity) return;
		if (!miningComponent.Active()) return;

		newEfficiency *= InterferenceMod(G_Distance(entity.oldEnt, other.oldEnt));
	});

	efficiency = newEfficiency;
}

void MiningComponent::InformNeighbors() {
	ForEntities<MiningComponent>([&] (Entity& other, MiningComponent& miningComponent) {
		if (&other == &entity) return;
		if (G_Distance(entity.oldEnt, other.oldEnt) > RGS_RANGE * 2.0f) return;

		miningComponent.CalculateEfficiency();
	});
}

void MiningComponent::Think(int timeDelta) {
	bool active = Active();

	// Think if and only if activity state changed.
	if (active ^ lastThinkActive) {
		// If the state of this RGS has changed, adjust own rate and inform active closeby mining
		// structures so they can adjust their rate immediately.
		CalculateEfficiency();
		InformNeighbors();
	}

	lastThinkActive = active;
}
