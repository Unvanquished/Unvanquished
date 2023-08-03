// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

/*
 * This file contains:
 *   - Implementation of the base entity.
 *   - Implementations of the specific entities and related helpers.
 */

#include "CBSEEntities.h"
#include <tuple>

#define myoffsetof(st, m) static_cast<int>((size_t)(&((st *)1)->m))-1

// /////////// //
// Base entity //
// /////////// //

// Base entity constructor.
Entity::Entity(const MessageHandler *messageHandlers, const int* componentOffsets, gentity_t* oldEnt)
	: messageHandlers(messageHandlers), componentOffsets(componentOffsets), oldEnt(oldEnt)
{}

// Base entity's message dispatcher.
bool Entity::SendMessage(EntityMessage msg, const void* data) {
	MessageHandler handler = messageHandlers[static_cast<int>(msg)];
	if (handler) {
		handler(this, data);
		return true;
	}
	return false;
}

// /////////////// //
// Message helpers //
// /////////////// //

bool Entity::PrepareNetCode() {
	return SendMessage(EntityMessage::PrepareNetCode, nullptr);
}

bool Entity::FreeAt(int freeTime) {
	std::tuple<int> data(freeTime);
	return SendMessage(EntityMessage::FreeAt, &data);
}

bool Entity::Ignite( gentity_t const* fireStarter )
{
	std::tuple<gentity_t const*> data( fireStarter );
	return SendMessage(EntityMessage::Ignite, &data);
}

bool Entity::Extinguish(int immunityTime) {
	std::tuple<int> data(immunityTime);
	return SendMessage(EntityMessage::Extinguish, &data);
}

bool Entity::Heal(float amount, gentity_t* source) {
	std::tuple<float, gentity_t*> data(amount, source);
	return SendMessage(EntityMessage::Heal, &data);
}

bool Entity::Damage(float amount, gentity_t* source, Util::optional<glm::vec3> location, Util::optional<glm::vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t> data(amount, source, location, direction, flags, meansOfDeath);
	return SendMessage(EntityMessage::Damage, &data);
}

bool Entity::Die(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	std::tuple<gentity_t*, meansOfDeath_t> data(killer, meansOfDeath);
	return SendMessage(EntityMessage::Die, &data);
}

bool Entity::FinishConstruction() {
	return SendMessage(EntityMessage::FinishConstruction, nullptr);
}

bool Entity::PowerUp() {
	return SendMessage(EntityMessage::PowerUp, nullptr);
}

bool Entity::PowerDown() {
	return SendMessage(EntityMessage::PowerDown, nullptr);
}

bool Entity::ApplyDamageModifier(float& damage, Util::optional<glm::vec3> location, Util::optional<glm::vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t> data(damage, location, direction, flags, meansOfDeath);
	return SendMessage(EntityMessage::ApplyDamageModifier, &data);
}

bool Entity::CheckSpawnPoint(Entity*& blocker, glm::vec3& spawnPoint) {
	std::tuple<Entity*&, glm::vec3&> data(blocker, spawnPoint);
	return SendMessage(EntityMessage::CheckSpawnPoint, &data);
}

// ///////////////////////// //
// Component implementations //
// ///////////////////////// //


std::set<DeferredFreeingComponent*> DeferredFreeingComponentBase::allSet;

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& ThinkingComponentBase::GetDeferredFreeingComponent() {
    return r_DeferredFreeingComponent;
}
const DeferredFreeingComponent& ThinkingComponentBase::GetDeferredFreeingComponent() const {
    return r_DeferredFreeingComponent;
}



std::set<ThinkingComponent*> ThinkingComponentBase::allSet;



std::set<TeamComponent*> TeamComponentBase::allSet;

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& IgnitableComponentBase::GetThinkingComponent() {
    return r_ThinkingComponent;
}
const ThinkingComponent& IgnitableComponentBase::GetThinkingComponent() const {
    return r_ThinkingComponent;
}


/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& IgnitableComponentBase::GetDeferredFreeingComponent() {
    return r_ThinkingComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& IgnitableComponentBase::GetDeferredFreeingComponent() const {
    return r_ThinkingComponent.GetDeferredFreeingComponent();
}


std::set<IgnitableComponent*> IgnitableComponentBase::allSet;



std::set<HealthComponent*> HealthComponentBase::allSet;

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& ClientComponentBase::GetTeamComponent() {
    return r_TeamComponent;
}
const TeamComponent& ClientComponentBase::GetTeamComponent() const {
    return r_TeamComponent;
}



std::set<ClientComponent*> ClientComponentBase::allSet;



std::set<ArmorComponent*> ArmorComponentBase::allSet;



std::set<KnockbackComponent*> KnockbackComponentBase::allSet;

/**
 * @return A reference to the ClientComponent of the owning entity.
 */
ClientComponent& SpectatorComponentBase::GetClientComponent() {
    return r_ClientComponent;
}
const ClientComponent& SpectatorComponentBase::GetClientComponent() const {
    return r_ClientComponent;
}


/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& SpectatorComponentBase::GetTeamComponent() {
    return r_ClientComponent.GetTeamComponent();
}
const TeamComponent& SpectatorComponentBase::GetTeamComponent() const {
    return r_ClientComponent.GetTeamComponent();
}


std::set<SpectatorComponent*> SpectatorComponentBase::allSet;

/**
 * @return A reference to the ClientComponent of the owning entity.
 */
ClientComponent& AlienClassComponentBase::GetClientComponent() {
    return r_ClientComponent;
}
const ClientComponent& AlienClassComponentBase::GetClientComponent() const {
    return r_ClientComponent;
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& AlienClassComponentBase::GetTeamComponent() {
    return r_TeamComponent;
}
const TeamComponent& AlienClassComponentBase::GetTeamComponent() const {
    return r_TeamComponent;
}

/**
 * @return A reference to the ArmorComponent of the owning entity.
 */
ArmorComponent& AlienClassComponentBase::GetArmorComponent() {
    return r_ArmorComponent;
}
const ArmorComponent& AlienClassComponentBase::GetArmorComponent() const {
    return r_ArmorComponent;
}

/**
 * @return A reference to the KnockbackComponent of the owning entity.
 */
KnockbackComponent& AlienClassComponentBase::GetKnockbackComponent() {
    return r_KnockbackComponent;
}
const KnockbackComponent& AlienClassComponentBase::GetKnockbackComponent() const {
    return r_KnockbackComponent;
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& AlienClassComponentBase::GetHealthComponent() {
    return r_HealthComponent;
}
const HealthComponent& AlienClassComponentBase::GetHealthComponent() const {
    return r_HealthComponent;
}



std::set<AlienClassComponent*> AlienClassComponentBase::allSet;

/**
 * @return A reference to the ClientComponent of the owning entity.
 */
ClientComponent& HumanClassComponentBase::GetClientComponent() {
    return r_ClientComponent;
}
const ClientComponent& HumanClassComponentBase::GetClientComponent() const {
    return r_ClientComponent;
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& HumanClassComponentBase::GetTeamComponent() {
    return r_TeamComponent;
}
const TeamComponent& HumanClassComponentBase::GetTeamComponent() const {
    return r_TeamComponent;
}

/**
 * @return A reference to the ArmorComponent of the owning entity.
 */
ArmorComponent& HumanClassComponentBase::GetArmorComponent() {
    return r_ArmorComponent;
}
const ArmorComponent& HumanClassComponentBase::GetArmorComponent() const {
    return r_ArmorComponent;
}

/**
 * @return A reference to the KnockbackComponent of the owning entity.
 */
KnockbackComponent& HumanClassComponentBase::GetKnockbackComponent() {
    return r_KnockbackComponent;
}
const KnockbackComponent& HumanClassComponentBase::GetKnockbackComponent() const {
    return r_KnockbackComponent;
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& HumanClassComponentBase::GetHealthComponent() {
    return r_HealthComponent;
}
const HealthComponent& HumanClassComponentBase::GetHealthComponent() const {
    return r_HealthComponent;
}



std::set<HumanClassComponent*> HumanClassComponentBase::allSet;

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& BuildableComponentBase::GetHealthComponent() {
    return r_HealthComponent;
}
const HealthComponent& BuildableComponentBase::GetHealthComponent() const {
    return r_HealthComponent;
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& BuildableComponentBase::GetThinkingComponent() {
    return r_ThinkingComponent;
}
const ThinkingComponent& BuildableComponentBase::GetThinkingComponent() const {
    return r_ThinkingComponent;
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& BuildableComponentBase::GetTeamComponent() {
    return r_TeamComponent;
}
const TeamComponent& BuildableComponentBase::GetTeamComponent() const {
    return r_TeamComponent;
}


/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& BuildableComponentBase::GetDeferredFreeingComponent() {
    return r_ThinkingComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& BuildableComponentBase::GetDeferredFreeingComponent() const {
    return r_ThinkingComponent.GetDeferredFreeingComponent();
}


std::set<BuildableComponent*> BuildableComponentBase::allSet;

/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& AlienBuildableComponentBase::GetBuildableComponent() {
    return r_BuildableComponent;
}
const BuildableComponent& AlienBuildableComponentBase::GetBuildableComponent() const {
    return r_BuildableComponent;
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& AlienBuildableComponentBase::GetTeamComponent() {
    return r_TeamComponent;
}
const TeamComponent& AlienBuildableComponentBase::GetTeamComponent() const {
    return r_TeamComponent;
}

/**
 * @return A reference to the IgnitableComponent of the owning entity.
 */
IgnitableComponent& AlienBuildableComponentBase::GetIgnitableComponent() {
    return r_IgnitableComponent;
}
const IgnitableComponent& AlienBuildableComponentBase::GetIgnitableComponent() const {
    return r_IgnitableComponent;
}


/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& AlienBuildableComponentBase::GetHealthComponent() {
    return r_BuildableComponent.GetHealthComponent();
}
const HealthComponent& AlienBuildableComponentBase::GetHealthComponent() const {
    return r_BuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& AlienBuildableComponentBase::GetThinkingComponent() {
    return r_BuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& AlienBuildableComponentBase::GetThinkingComponent() const {
    return r_BuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& AlienBuildableComponentBase::GetDeferredFreeingComponent() {
    return r_BuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& AlienBuildableComponentBase::GetDeferredFreeingComponent() const {
    return r_BuildableComponent.GetDeferredFreeingComponent();
}


std::set<AlienBuildableComponent*> AlienBuildableComponentBase::allSet;

/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& HumanBuildableComponentBase::GetBuildableComponent() {
    return r_BuildableComponent;
}
const BuildableComponent& HumanBuildableComponentBase::GetBuildableComponent() const {
    return r_BuildableComponent;
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& HumanBuildableComponentBase::GetTeamComponent() {
    return r_TeamComponent;
}
const TeamComponent& HumanBuildableComponentBase::GetTeamComponent() const {
    return r_TeamComponent;
}


/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& HumanBuildableComponentBase::GetHealthComponent() {
    return r_BuildableComponent.GetHealthComponent();
}
const HealthComponent& HumanBuildableComponentBase::GetHealthComponent() const {
    return r_BuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& HumanBuildableComponentBase::GetThinkingComponent() {
    return r_BuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& HumanBuildableComponentBase::GetThinkingComponent() const {
    return r_BuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& HumanBuildableComponentBase::GetDeferredFreeingComponent() {
    return r_BuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& HumanBuildableComponentBase::GetDeferredFreeingComponent() const {
    return r_BuildableComponent.GetDeferredFreeingComponent();
}


std::set<HumanBuildableComponent*> HumanBuildableComponentBase::allSet;

/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& MainBuildableComponentBase::GetBuildableComponent() {
    return r_BuildableComponent;
}
const BuildableComponent& MainBuildableComponentBase::GetBuildableComponent() const {
    return r_BuildableComponent;
}


/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& MainBuildableComponentBase::GetHealthComponent() {
    return r_BuildableComponent.GetHealthComponent();
}
const HealthComponent& MainBuildableComponentBase::GetHealthComponent() const {
    return r_BuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& MainBuildableComponentBase::GetThinkingComponent() {
    return r_BuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& MainBuildableComponentBase::GetThinkingComponent() const {
    return r_BuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& MainBuildableComponentBase::GetTeamComponent() {
    return r_BuildableComponent.GetTeamComponent();
}
const TeamComponent& MainBuildableComponentBase::GetTeamComponent() const {
    return r_BuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& MainBuildableComponentBase::GetDeferredFreeingComponent() {
    return r_BuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& MainBuildableComponentBase::GetDeferredFreeingComponent() const {
    return r_BuildableComponent.GetDeferredFreeingComponent();
}


std::set<MainBuildableComponent*> MainBuildableComponentBase::allSet;

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& SpawnerComponentBase::GetTeamComponent() {
    return r_TeamComponent;
}
const TeamComponent& SpawnerComponentBase::GetTeamComponent() const {
    return r_TeamComponent;
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& SpawnerComponentBase::GetThinkingComponent() {
    return r_ThinkingComponent;
}
const ThinkingComponent& SpawnerComponentBase::GetThinkingComponent() const {
    return r_ThinkingComponent;
}


/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& SpawnerComponentBase::GetDeferredFreeingComponent() {
    return r_ThinkingComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& SpawnerComponentBase::GetDeferredFreeingComponent() const {
    return r_ThinkingComponent.GetDeferredFreeingComponent();
}


std::set<SpawnerComponent*> SpawnerComponentBase::allSet;



std::set<TurretComponent*> TurretComponentBase::allSet;

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& MiningComponentBase::GetThinkingComponent() {
    return r_ThinkingComponent;
}
const ThinkingComponent& MiningComponentBase::GetThinkingComponent() const {
    return r_ThinkingComponent;
}


/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& MiningComponentBase::GetDeferredFreeingComponent() {
    return r_ThinkingComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& MiningComponentBase::GetDeferredFreeingComponent() const {
    return r_ThinkingComponent.GetDeferredFreeingComponent();
}


std::set<MiningComponent*> MiningComponentBase::allSet;

/**
 * @return A reference to the AlienBuildableComponent of the owning entity.
 */
AlienBuildableComponent& AcidTubeComponentBase::GetAlienBuildableComponent() {
    return r_AlienBuildableComponent;
}
const AlienBuildableComponent& AcidTubeComponentBase::GetAlienBuildableComponent() const {
    return r_AlienBuildableComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& AcidTubeComponentBase::GetBuildableComponent() {
    return r_AlienBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& AcidTubeComponentBase::GetBuildableComponent() const {
    return r_AlienBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& AcidTubeComponentBase::GetTeamComponent() {
    return r_AlienBuildableComponent.GetTeamComponent();
}
const TeamComponent& AcidTubeComponentBase::GetTeamComponent() const {
    return r_AlienBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the IgnitableComponent of the owning entity.
 */
IgnitableComponent& AcidTubeComponentBase::GetIgnitableComponent() {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}
const IgnitableComponent& AcidTubeComponentBase::GetIgnitableComponent() const {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& AcidTubeComponentBase::GetHealthComponent() {
    return r_AlienBuildableComponent.GetHealthComponent();
}
const HealthComponent& AcidTubeComponentBase::GetHealthComponent() const {
    return r_AlienBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& AcidTubeComponentBase::GetThinkingComponent() {
    return r_AlienBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& AcidTubeComponentBase::GetThinkingComponent() const {
    return r_AlienBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& AcidTubeComponentBase::GetDeferredFreeingComponent() {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& AcidTubeComponentBase::GetDeferredFreeingComponent() const {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}


std::set<AcidTubeComponent*> AcidTubeComponentBase::allSet;

/**
 * @return A reference to the AlienBuildableComponent of the owning entity.
 */
AlienBuildableComponent& BarricadeComponentBase::GetAlienBuildableComponent() {
    return r_AlienBuildableComponent;
}
const AlienBuildableComponent& BarricadeComponentBase::GetAlienBuildableComponent() const {
    return r_AlienBuildableComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& BarricadeComponentBase::GetBuildableComponent() {
    return r_AlienBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& BarricadeComponentBase::GetBuildableComponent() const {
    return r_AlienBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& BarricadeComponentBase::GetTeamComponent() {
    return r_AlienBuildableComponent.GetTeamComponent();
}
const TeamComponent& BarricadeComponentBase::GetTeamComponent() const {
    return r_AlienBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the IgnitableComponent of the owning entity.
 */
IgnitableComponent& BarricadeComponentBase::GetIgnitableComponent() {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}
const IgnitableComponent& BarricadeComponentBase::GetIgnitableComponent() const {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& BarricadeComponentBase::GetHealthComponent() {
    return r_AlienBuildableComponent.GetHealthComponent();
}
const HealthComponent& BarricadeComponentBase::GetHealthComponent() const {
    return r_AlienBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& BarricadeComponentBase::GetThinkingComponent() {
    return r_AlienBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& BarricadeComponentBase::GetThinkingComponent() const {
    return r_AlienBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& BarricadeComponentBase::GetDeferredFreeingComponent() {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& BarricadeComponentBase::GetDeferredFreeingComponent() const {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}


std::set<BarricadeComponent*> BarricadeComponentBase::allSet;

/**
 * @return A reference to the AlienBuildableComponent of the owning entity.
 */
AlienBuildableComponent& BoosterComponentBase::GetAlienBuildableComponent() {
    return r_AlienBuildableComponent;
}
const AlienBuildableComponent& BoosterComponentBase::GetAlienBuildableComponent() const {
    return r_AlienBuildableComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& BoosterComponentBase::GetBuildableComponent() {
    return r_AlienBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& BoosterComponentBase::GetBuildableComponent() const {
    return r_AlienBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& BoosterComponentBase::GetTeamComponent() {
    return r_AlienBuildableComponent.GetTeamComponent();
}
const TeamComponent& BoosterComponentBase::GetTeamComponent() const {
    return r_AlienBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the IgnitableComponent of the owning entity.
 */
IgnitableComponent& BoosterComponentBase::GetIgnitableComponent() {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}
const IgnitableComponent& BoosterComponentBase::GetIgnitableComponent() const {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& BoosterComponentBase::GetHealthComponent() {
    return r_AlienBuildableComponent.GetHealthComponent();
}
const HealthComponent& BoosterComponentBase::GetHealthComponent() const {
    return r_AlienBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& BoosterComponentBase::GetThinkingComponent() {
    return r_AlienBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& BoosterComponentBase::GetThinkingComponent() const {
    return r_AlienBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& BoosterComponentBase::GetDeferredFreeingComponent() {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& BoosterComponentBase::GetDeferredFreeingComponent() const {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}


std::set<BoosterComponent*> BoosterComponentBase::allSet;

/**
 * @return A reference to the AlienBuildableComponent of the owning entity.
 */
AlienBuildableComponent& EggComponentBase::GetAlienBuildableComponent() {
    return r_AlienBuildableComponent;
}
const AlienBuildableComponent& EggComponentBase::GetAlienBuildableComponent() const {
    return r_AlienBuildableComponent;
}

/**
 * @return A reference to the SpawnerComponent of the owning entity.
 */
SpawnerComponent& EggComponentBase::GetSpawnerComponent() {
    return r_SpawnerComponent;
}
const SpawnerComponent& EggComponentBase::GetSpawnerComponent() const {
    return r_SpawnerComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& EggComponentBase::GetBuildableComponent() {
    return r_AlienBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& EggComponentBase::GetBuildableComponent() const {
    return r_AlienBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& EggComponentBase::GetTeamComponent() {
    return r_AlienBuildableComponent.GetTeamComponent();
}
const TeamComponent& EggComponentBase::GetTeamComponent() const {
    return r_AlienBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the IgnitableComponent of the owning entity.
 */
IgnitableComponent& EggComponentBase::GetIgnitableComponent() {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}
const IgnitableComponent& EggComponentBase::GetIgnitableComponent() const {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& EggComponentBase::GetHealthComponent() {
    return r_AlienBuildableComponent.GetHealthComponent();
}
const HealthComponent& EggComponentBase::GetHealthComponent() const {
    return r_AlienBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& EggComponentBase::GetThinkingComponent() {
    return r_AlienBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& EggComponentBase::GetThinkingComponent() const {
    return r_AlienBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& EggComponentBase::GetDeferredFreeingComponent() {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& EggComponentBase::GetDeferredFreeingComponent() const {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}


std::set<EggComponent*> EggComponentBase::allSet;

/**
 * @return A reference to the AlienBuildableComponent of the owning entity.
 */
AlienBuildableComponent& HiveComponentBase::GetAlienBuildableComponent() {
    return r_AlienBuildableComponent;
}
const AlienBuildableComponent& HiveComponentBase::GetAlienBuildableComponent() const {
    return r_AlienBuildableComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& HiveComponentBase::GetBuildableComponent() {
    return r_AlienBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& HiveComponentBase::GetBuildableComponent() const {
    return r_AlienBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& HiveComponentBase::GetTeamComponent() {
    return r_AlienBuildableComponent.GetTeamComponent();
}
const TeamComponent& HiveComponentBase::GetTeamComponent() const {
    return r_AlienBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the IgnitableComponent of the owning entity.
 */
IgnitableComponent& HiveComponentBase::GetIgnitableComponent() {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}
const IgnitableComponent& HiveComponentBase::GetIgnitableComponent() const {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& HiveComponentBase::GetHealthComponent() {
    return r_AlienBuildableComponent.GetHealthComponent();
}
const HealthComponent& HiveComponentBase::GetHealthComponent() const {
    return r_AlienBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& HiveComponentBase::GetThinkingComponent() {
    return r_AlienBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& HiveComponentBase::GetThinkingComponent() const {
    return r_AlienBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& HiveComponentBase::GetDeferredFreeingComponent() {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& HiveComponentBase::GetDeferredFreeingComponent() const {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}


std::set<HiveComponent*> HiveComponentBase::allSet;

/**
 * @return A reference to the AlienBuildableComponent of the owning entity.
 */
AlienBuildableComponent& LeechComponentBase::GetAlienBuildableComponent() {
    return r_AlienBuildableComponent;
}
const AlienBuildableComponent& LeechComponentBase::GetAlienBuildableComponent() const {
    return r_AlienBuildableComponent;
}

/**
 * @return A reference to the MiningComponent of the owning entity.
 */
MiningComponent& LeechComponentBase::GetMiningComponent() {
    return r_MiningComponent;
}
const MiningComponent& LeechComponentBase::GetMiningComponent() const {
    return r_MiningComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& LeechComponentBase::GetBuildableComponent() {
    return r_AlienBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& LeechComponentBase::GetBuildableComponent() const {
    return r_AlienBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& LeechComponentBase::GetTeamComponent() {
    return r_AlienBuildableComponent.GetTeamComponent();
}
const TeamComponent& LeechComponentBase::GetTeamComponent() const {
    return r_AlienBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the IgnitableComponent of the owning entity.
 */
IgnitableComponent& LeechComponentBase::GetIgnitableComponent() {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}
const IgnitableComponent& LeechComponentBase::GetIgnitableComponent() const {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& LeechComponentBase::GetHealthComponent() {
    return r_AlienBuildableComponent.GetHealthComponent();
}
const HealthComponent& LeechComponentBase::GetHealthComponent() const {
    return r_AlienBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& LeechComponentBase::GetThinkingComponent() {
    return r_AlienBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& LeechComponentBase::GetThinkingComponent() const {
    return r_AlienBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& LeechComponentBase::GetDeferredFreeingComponent() {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& LeechComponentBase::GetDeferredFreeingComponent() const {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}


std::set<LeechComponent*> LeechComponentBase::allSet;

/**
 * @return A reference to the AlienBuildableComponent of the owning entity.
 */
AlienBuildableComponent& OvermindComponentBase::GetAlienBuildableComponent() {
    return r_AlienBuildableComponent;
}
const AlienBuildableComponent& OvermindComponentBase::GetAlienBuildableComponent() const {
    return r_AlienBuildableComponent;
}

/**
 * @return A reference to the MainBuildableComponent of the owning entity.
 */
MainBuildableComponent& OvermindComponentBase::GetMainBuildableComponent() {
    return r_MainBuildableComponent;
}
const MainBuildableComponent& OvermindComponentBase::GetMainBuildableComponent() const {
    return r_MainBuildableComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& OvermindComponentBase::GetBuildableComponent() {
    return r_AlienBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& OvermindComponentBase::GetBuildableComponent() const {
    return r_AlienBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& OvermindComponentBase::GetTeamComponent() {
    return r_AlienBuildableComponent.GetTeamComponent();
}
const TeamComponent& OvermindComponentBase::GetTeamComponent() const {
    return r_AlienBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the IgnitableComponent of the owning entity.
 */
IgnitableComponent& OvermindComponentBase::GetIgnitableComponent() {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}
const IgnitableComponent& OvermindComponentBase::GetIgnitableComponent() const {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& OvermindComponentBase::GetHealthComponent() {
    return r_AlienBuildableComponent.GetHealthComponent();
}
const HealthComponent& OvermindComponentBase::GetHealthComponent() const {
    return r_AlienBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& OvermindComponentBase::GetThinkingComponent() {
    return r_AlienBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& OvermindComponentBase::GetThinkingComponent() const {
    return r_AlienBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& OvermindComponentBase::GetDeferredFreeingComponent() {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& OvermindComponentBase::GetDeferredFreeingComponent() const {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}


std::set<OvermindComponent*> OvermindComponentBase::allSet;

/**
 * @return A reference to the AlienBuildableComponent of the owning entity.
 */
AlienBuildableComponent& SpikerComponentBase::GetAlienBuildableComponent() {
    return r_AlienBuildableComponent;
}
const AlienBuildableComponent& SpikerComponentBase::GetAlienBuildableComponent() const {
    return r_AlienBuildableComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& SpikerComponentBase::GetBuildableComponent() {
    return r_AlienBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& SpikerComponentBase::GetBuildableComponent() const {
    return r_AlienBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& SpikerComponentBase::GetTeamComponent() {
    return r_AlienBuildableComponent.GetTeamComponent();
}
const TeamComponent& SpikerComponentBase::GetTeamComponent() const {
    return r_AlienBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the IgnitableComponent of the owning entity.
 */
IgnitableComponent& SpikerComponentBase::GetIgnitableComponent() {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}
const IgnitableComponent& SpikerComponentBase::GetIgnitableComponent() const {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& SpikerComponentBase::GetHealthComponent() {
    return r_AlienBuildableComponent.GetHealthComponent();
}
const HealthComponent& SpikerComponentBase::GetHealthComponent() const {
    return r_AlienBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& SpikerComponentBase::GetThinkingComponent() {
    return r_AlienBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& SpikerComponentBase::GetThinkingComponent() const {
    return r_AlienBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& SpikerComponentBase::GetDeferredFreeingComponent() {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& SpikerComponentBase::GetDeferredFreeingComponent() const {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}


std::set<SpikerComponent*> SpikerComponentBase::allSet;

/**
 * @return A reference to the AlienBuildableComponent of the owning entity.
 */
AlienBuildableComponent& TrapperComponentBase::GetAlienBuildableComponent() {
    return r_AlienBuildableComponent;
}
const AlienBuildableComponent& TrapperComponentBase::GetAlienBuildableComponent() const {
    return r_AlienBuildableComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& TrapperComponentBase::GetBuildableComponent() {
    return r_AlienBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& TrapperComponentBase::GetBuildableComponent() const {
    return r_AlienBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& TrapperComponentBase::GetTeamComponent() {
    return r_AlienBuildableComponent.GetTeamComponent();
}
const TeamComponent& TrapperComponentBase::GetTeamComponent() const {
    return r_AlienBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the IgnitableComponent of the owning entity.
 */
IgnitableComponent& TrapperComponentBase::GetIgnitableComponent() {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}
const IgnitableComponent& TrapperComponentBase::GetIgnitableComponent() const {
    return r_AlienBuildableComponent.GetIgnitableComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& TrapperComponentBase::GetHealthComponent() {
    return r_AlienBuildableComponent.GetHealthComponent();
}
const HealthComponent& TrapperComponentBase::GetHealthComponent() const {
    return r_AlienBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& TrapperComponentBase::GetThinkingComponent() {
    return r_AlienBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& TrapperComponentBase::GetThinkingComponent() const {
    return r_AlienBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& TrapperComponentBase::GetDeferredFreeingComponent() {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& TrapperComponentBase::GetDeferredFreeingComponent() const {
    return r_AlienBuildableComponent.GetDeferredFreeingComponent();
}


std::set<TrapperComponent*> TrapperComponentBase::allSet;

/**
 * @return A reference to the HumanBuildableComponent of the owning entity.
 */
HumanBuildableComponent& ArmouryComponentBase::GetHumanBuildableComponent() {
    return r_HumanBuildableComponent;
}
const HumanBuildableComponent& ArmouryComponentBase::GetHumanBuildableComponent() const {
    return r_HumanBuildableComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& ArmouryComponentBase::GetBuildableComponent() {
    return r_HumanBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& ArmouryComponentBase::GetBuildableComponent() const {
    return r_HumanBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& ArmouryComponentBase::GetTeamComponent() {
    return r_HumanBuildableComponent.GetTeamComponent();
}
const TeamComponent& ArmouryComponentBase::GetTeamComponent() const {
    return r_HumanBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& ArmouryComponentBase::GetHealthComponent() {
    return r_HumanBuildableComponent.GetHealthComponent();
}
const HealthComponent& ArmouryComponentBase::GetHealthComponent() const {
    return r_HumanBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& ArmouryComponentBase::GetThinkingComponent() {
    return r_HumanBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& ArmouryComponentBase::GetThinkingComponent() const {
    return r_HumanBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& ArmouryComponentBase::GetDeferredFreeingComponent() {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& ArmouryComponentBase::GetDeferredFreeingComponent() const {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}


std::set<ArmouryComponent*> ArmouryComponentBase::allSet;

/**
 * @return A reference to the HumanBuildableComponent of the owning entity.
 */
HumanBuildableComponent& DrillComponentBase::GetHumanBuildableComponent() {
    return r_HumanBuildableComponent;
}
const HumanBuildableComponent& DrillComponentBase::GetHumanBuildableComponent() const {
    return r_HumanBuildableComponent;
}

/**
 * @return A reference to the MiningComponent of the owning entity.
 */
MiningComponent& DrillComponentBase::GetMiningComponent() {
    return r_MiningComponent;
}
const MiningComponent& DrillComponentBase::GetMiningComponent() const {
    return r_MiningComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& DrillComponentBase::GetBuildableComponent() {
    return r_HumanBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& DrillComponentBase::GetBuildableComponent() const {
    return r_HumanBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& DrillComponentBase::GetTeamComponent() {
    return r_HumanBuildableComponent.GetTeamComponent();
}
const TeamComponent& DrillComponentBase::GetTeamComponent() const {
    return r_HumanBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& DrillComponentBase::GetHealthComponent() {
    return r_HumanBuildableComponent.GetHealthComponent();
}
const HealthComponent& DrillComponentBase::GetHealthComponent() const {
    return r_HumanBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& DrillComponentBase::GetThinkingComponent() {
    return r_HumanBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& DrillComponentBase::GetThinkingComponent() const {
    return r_HumanBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& DrillComponentBase::GetDeferredFreeingComponent() {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& DrillComponentBase::GetDeferredFreeingComponent() const {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}


std::set<DrillComponent*> DrillComponentBase::allSet;

/**
 * @return A reference to the HumanBuildableComponent of the owning entity.
 */
HumanBuildableComponent& MedipadComponentBase::GetHumanBuildableComponent() {
    return r_HumanBuildableComponent;
}
const HumanBuildableComponent& MedipadComponentBase::GetHumanBuildableComponent() const {
    return r_HumanBuildableComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& MedipadComponentBase::GetBuildableComponent() {
    return r_HumanBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& MedipadComponentBase::GetBuildableComponent() const {
    return r_HumanBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& MedipadComponentBase::GetTeamComponent() {
    return r_HumanBuildableComponent.GetTeamComponent();
}
const TeamComponent& MedipadComponentBase::GetTeamComponent() const {
    return r_HumanBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& MedipadComponentBase::GetHealthComponent() {
    return r_HumanBuildableComponent.GetHealthComponent();
}
const HealthComponent& MedipadComponentBase::GetHealthComponent() const {
    return r_HumanBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& MedipadComponentBase::GetThinkingComponent() {
    return r_HumanBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& MedipadComponentBase::GetThinkingComponent() const {
    return r_HumanBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& MedipadComponentBase::GetDeferredFreeingComponent() {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& MedipadComponentBase::GetDeferredFreeingComponent() const {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}


std::set<MedipadComponent*> MedipadComponentBase::allSet;

/**
 * @return A reference to the HumanBuildableComponent of the owning entity.
 */
HumanBuildableComponent& MGTurretComponentBase::GetHumanBuildableComponent() {
    return r_HumanBuildableComponent;
}
const HumanBuildableComponent& MGTurretComponentBase::GetHumanBuildableComponent() const {
    return r_HumanBuildableComponent;
}

/**
 * @return A reference to the TurretComponent of the owning entity.
 */
TurretComponent& MGTurretComponentBase::GetTurretComponent() {
    return r_TurretComponent;
}
const TurretComponent& MGTurretComponentBase::GetTurretComponent() const {
    return r_TurretComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& MGTurretComponentBase::GetBuildableComponent() {
    return r_HumanBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& MGTurretComponentBase::GetBuildableComponent() const {
    return r_HumanBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& MGTurretComponentBase::GetTeamComponent() {
    return r_HumanBuildableComponent.GetTeamComponent();
}
const TeamComponent& MGTurretComponentBase::GetTeamComponent() const {
    return r_HumanBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& MGTurretComponentBase::GetHealthComponent() {
    return r_HumanBuildableComponent.GetHealthComponent();
}
const HealthComponent& MGTurretComponentBase::GetHealthComponent() const {
    return r_HumanBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& MGTurretComponentBase::GetThinkingComponent() {
    return r_HumanBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& MGTurretComponentBase::GetThinkingComponent() const {
    return r_HumanBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& MGTurretComponentBase::GetDeferredFreeingComponent() {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& MGTurretComponentBase::GetDeferredFreeingComponent() const {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}


std::set<MGTurretComponent*> MGTurretComponentBase::allSet;

/**
 * @return A reference to the HumanBuildableComponent of the owning entity.
 */
HumanBuildableComponent& ReactorComponentBase::GetHumanBuildableComponent() {
    return r_HumanBuildableComponent;
}
const HumanBuildableComponent& ReactorComponentBase::GetHumanBuildableComponent() const {
    return r_HumanBuildableComponent;
}

/**
 * @return A reference to the MainBuildableComponent of the owning entity.
 */
MainBuildableComponent& ReactorComponentBase::GetMainBuildableComponent() {
    return r_MainBuildableComponent;
}
const MainBuildableComponent& ReactorComponentBase::GetMainBuildableComponent() const {
    return r_MainBuildableComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& ReactorComponentBase::GetBuildableComponent() {
    return r_HumanBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& ReactorComponentBase::GetBuildableComponent() const {
    return r_HumanBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& ReactorComponentBase::GetTeamComponent() {
    return r_HumanBuildableComponent.GetTeamComponent();
}
const TeamComponent& ReactorComponentBase::GetTeamComponent() const {
    return r_HumanBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& ReactorComponentBase::GetHealthComponent() {
    return r_HumanBuildableComponent.GetHealthComponent();
}
const HealthComponent& ReactorComponentBase::GetHealthComponent() const {
    return r_HumanBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& ReactorComponentBase::GetThinkingComponent() {
    return r_HumanBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& ReactorComponentBase::GetThinkingComponent() const {
    return r_HumanBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& ReactorComponentBase::GetDeferredFreeingComponent() {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& ReactorComponentBase::GetDeferredFreeingComponent() const {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}


std::set<ReactorComponent*> ReactorComponentBase::allSet;

/**
 * @return A reference to the HumanBuildableComponent of the owning entity.
 */
HumanBuildableComponent& RocketpodComponentBase::GetHumanBuildableComponent() {
    return r_HumanBuildableComponent;
}
const HumanBuildableComponent& RocketpodComponentBase::GetHumanBuildableComponent() const {
    return r_HumanBuildableComponent;
}

/**
 * @return A reference to the TurretComponent of the owning entity.
 */
TurretComponent& RocketpodComponentBase::GetTurretComponent() {
    return r_TurretComponent;
}
const TurretComponent& RocketpodComponentBase::GetTurretComponent() const {
    return r_TurretComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& RocketpodComponentBase::GetBuildableComponent() {
    return r_HumanBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& RocketpodComponentBase::GetBuildableComponent() const {
    return r_HumanBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& RocketpodComponentBase::GetTeamComponent() {
    return r_HumanBuildableComponent.GetTeamComponent();
}
const TeamComponent& RocketpodComponentBase::GetTeamComponent() const {
    return r_HumanBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& RocketpodComponentBase::GetHealthComponent() {
    return r_HumanBuildableComponent.GetHealthComponent();
}
const HealthComponent& RocketpodComponentBase::GetHealthComponent() const {
    return r_HumanBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& RocketpodComponentBase::GetThinkingComponent() {
    return r_HumanBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& RocketpodComponentBase::GetThinkingComponent() const {
    return r_HumanBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& RocketpodComponentBase::GetDeferredFreeingComponent() {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& RocketpodComponentBase::GetDeferredFreeingComponent() const {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}


std::set<RocketpodComponent*> RocketpodComponentBase::allSet;

/**
 * @return A reference to the HumanBuildableComponent of the owning entity.
 */
HumanBuildableComponent& TelenodeComponentBase::GetHumanBuildableComponent() {
    return r_HumanBuildableComponent;
}
const HumanBuildableComponent& TelenodeComponentBase::GetHumanBuildableComponent() const {
    return r_HumanBuildableComponent;
}

/**
 * @return A reference to the SpawnerComponent of the owning entity.
 */
SpawnerComponent& TelenodeComponentBase::GetSpawnerComponent() {
    return r_SpawnerComponent;
}
const SpawnerComponent& TelenodeComponentBase::GetSpawnerComponent() const {
    return r_SpawnerComponent;
}


/**
 * @return A reference to the BuildableComponent of the owning entity.
 */
BuildableComponent& TelenodeComponentBase::GetBuildableComponent() {
    return r_HumanBuildableComponent.GetBuildableComponent();
}
const BuildableComponent& TelenodeComponentBase::GetBuildableComponent() const {
    return r_HumanBuildableComponent.GetBuildableComponent();
}

/**
 * @return A reference to the TeamComponent of the owning entity.
 */
TeamComponent& TelenodeComponentBase::GetTeamComponent() {
    return r_HumanBuildableComponent.GetTeamComponent();
}
const TeamComponent& TelenodeComponentBase::GetTeamComponent() const {
    return r_HumanBuildableComponent.GetTeamComponent();
}

/**
 * @return A reference to the HealthComponent of the owning entity.
 */
HealthComponent& TelenodeComponentBase::GetHealthComponent() {
    return r_HumanBuildableComponent.GetHealthComponent();
}
const HealthComponent& TelenodeComponentBase::GetHealthComponent() const {
    return r_HumanBuildableComponent.GetHealthComponent();
}

/**
 * @return A reference to the ThinkingComponent of the owning entity.
 */
ThinkingComponent& TelenodeComponentBase::GetThinkingComponent() {
    return r_HumanBuildableComponent.GetThinkingComponent();
}
const ThinkingComponent& TelenodeComponentBase::GetThinkingComponent() const {
    return r_HumanBuildableComponent.GetThinkingComponent();
}

/**
 * @return A reference to the DeferredFreeingComponent of the owning entity.
 */
DeferredFreeingComponent& TelenodeComponentBase::GetDeferredFreeingComponent() {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}
const DeferredFreeingComponent& TelenodeComponentBase::GetDeferredFreeingComponent() const {
    return r_HumanBuildableComponent.GetDeferredFreeingComponent();
}


std::set<TelenodeComponent*> TelenodeComponentBase::allSet;


// ////////////////////// //
// Entity implementations //
// ////////////////////// //

// EmptyEntity
// ---------

// EmptyEntity's component offset vtable.
const int EmptyEntity::componentOffsets[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// EmptyEntity's message dispatcher vtable.
const MessageHandler EmptyEntity::messageHandlers[] = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// EmptyEntity's constructor.
EmptyEntity::EmptyEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
{}

// ClientEntity
// ---------

// ClientEntity's component offset vtable.
const int ClientEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(ClientEntity, c_TeamComponent),
	0,
	0,
	myoffsetof(ClientEntity, c_ClientComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// ClientEntity's message dispatcher vtable.
const MessageHandler ClientEntity::messageHandlers[] = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// ClientEntity's constructor.
ClientEntity::ClientEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_NONE)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
{}

// SpectatorEntity
// ---------

// SpectatorEntity's component offset vtable.
const int SpectatorEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(SpectatorEntity, c_TeamComponent),
	0,
	0,
	myoffsetof(SpectatorEntity, c_ClientComponent),
	0,
	0,
	myoffsetof(SpectatorEntity, c_SpectatorComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// SpectatorEntity's PrepareNetCode message dispatcher.
void h_Spectator_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<SpectatorEntity*>(_entity);
	entity->c_SpectatorComponent.HandlePrepareNetCode();
}

// SpectatorEntity's message dispatcher vtable.
const MessageHandler SpectatorEntity::messageHandlers[] = {
	h_Spectator_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// SpectatorEntity's constructor.
SpectatorEntity::SpectatorEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, params.Team_team)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_SpectatorComponent(*this, c_ClientComponent)
{}

// DretchEntity
// ---------

// DretchEntity's component offset vtable.
const int DretchEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(DretchEntity, c_TeamComponent),
	0,
	myoffsetof(DretchEntity, c_HealthComponent),
	myoffsetof(DretchEntity, c_ClientComponent),
	myoffsetof(DretchEntity, c_ArmorComponent),
	myoffsetof(DretchEntity, c_KnockbackComponent),
	0,
	myoffsetof(DretchEntity, c_AlienClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// DretchEntity's Heal message dispatcher.
void h_Dretch_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<DretchEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// DretchEntity's ApplyDamageModifier message dispatcher.
void h_Dretch_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<DretchEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// DretchEntity's PrepareNetCode message dispatcher.
void h_Dretch_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<DretchEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// DretchEntity's Damage message dispatcher.
void h_Dretch_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<DretchEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// DretchEntity's message dispatcher vtable.
const MessageHandler DretchEntity::messageHandlers[] = {
	h_Dretch_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_Dretch_Heal,
	h_Dretch_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_Dretch_ApplyDamageModifier,
	nullptr,
};

// DretchEntity's constructor.
DretchEntity::DretchEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// MantisEntity
// ---------

// MantisEntity's component offset vtable.
const int MantisEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(MantisEntity, c_TeamComponent),
	0,
	myoffsetof(MantisEntity, c_HealthComponent),
	myoffsetof(MantisEntity, c_ClientComponent),
	myoffsetof(MantisEntity, c_ArmorComponent),
	myoffsetof(MantisEntity, c_KnockbackComponent),
	0,
	myoffsetof(MantisEntity, c_AlienClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// MantisEntity's Heal message dispatcher.
void h_Mantis_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MantisEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// MantisEntity's ApplyDamageModifier message dispatcher.
void h_Mantis_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MantisEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// MantisEntity's PrepareNetCode message dispatcher.
void h_Mantis_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<MantisEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// MantisEntity's Damage message dispatcher.
void h_Mantis_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MantisEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// MantisEntity's message dispatcher vtable.
const MessageHandler MantisEntity::messageHandlers[] = {
	h_Mantis_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_Mantis_Heal,
	h_Mantis_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_Mantis_ApplyDamageModifier,
	nullptr,
};

// MantisEntity's constructor.
MantisEntity::MantisEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// MarauderEntity
// ---------

// MarauderEntity's component offset vtable.
const int MarauderEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(MarauderEntity, c_TeamComponent),
	0,
	myoffsetof(MarauderEntity, c_HealthComponent),
	myoffsetof(MarauderEntity, c_ClientComponent),
	myoffsetof(MarauderEntity, c_ArmorComponent),
	myoffsetof(MarauderEntity, c_KnockbackComponent),
	0,
	myoffsetof(MarauderEntity, c_AlienClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// MarauderEntity's Heal message dispatcher.
void h_Marauder_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MarauderEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// MarauderEntity's ApplyDamageModifier message dispatcher.
void h_Marauder_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MarauderEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// MarauderEntity's PrepareNetCode message dispatcher.
void h_Marauder_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<MarauderEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// MarauderEntity's Damage message dispatcher.
void h_Marauder_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MarauderEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// MarauderEntity's message dispatcher vtable.
const MessageHandler MarauderEntity::messageHandlers[] = {
	h_Marauder_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_Marauder_Heal,
	h_Marauder_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_Marauder_ApplyDamageModifier,
	nullptr,
};

// MarauderEntity's constructor.
MarauderEntity::MarauderEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// AdvMarauderEntity
// ---------

// AdvMarauderEntity's component offset vtable.
const int AdvMarauderEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(AdvMarauderEntity, c_TeamComponent),
	0,
	myoffsetof(AdvMarauderEntity, c_HealthComponent),
	myoffsetof(AdvMarauderEntity, c_ClientComponent),
	myoffsetof(AdvMarauderEntity, c_ArmorComponent),
	myoffsetof(AdvMarauderEntity, c_KnockbackComponent),
	0,
	myoffsetof(AdvMarauderEntity, c_AlienClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// AdvMarauderEntity's Heal message dispatcher.
void h_AdvMarauder_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AdvMarauderEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// AdvMarauderEntity's ApplyDamageModifier message dispatcher.
void h_AdvMarauder_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AdvMarauderEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// AdvMarauderEntity's PrepareNetCode message dispatcher.
void h_AdvMarauder_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<AdvMarauderEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// AdvMarauderEntity's Damage message dispatcher.
void h_AdvMarauder_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AdvMarauderEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// AdvMarauderEntity's message dispatcher vtable.
const MessageHandler AdvMarauderEntity::messageHandlers[] = {
	h_AdvMarauder_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_AdvMarauder_Heal,
	h_AdvMarauder_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_AdvMarauder_ApplyDamageModifier,
	nullptr,
};

// AdvMarauderEntity's constructor.
AdvMarauderEntity::AdvMarauderEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// DragoonEntity
// ---------

// DragoonEntity's component offset vtable.
const int DragoonEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(DragoonEntity, c_TeamComponent),
	0,
	myoffsetof(DragoonEntity, c_HealthComponent),
	myoffsetof(DragoonEntity, c_ClientComponent),
	myoffsetof(DragoonEntity, c_ArmorComponent),
	myoffsetof(DragoonEntity, c_KnockbackComponent),
	0,
	myoffsetof(DragoonEntity, c_AlienClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// DragoonEntity's Heal message dispatcher.
void h_Dragoon_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<DragoonEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// DragoonEntity's ApplyDamageModifier message dispatcher.
void h_Dragoon_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<DragoonEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// DragoonEntity's PrepareNetCode message dispatcher.
void h_Dragoon_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<DragoonEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// DragoonEntity's Damage message dispatcher.
void h_Dragoon_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<DragoonEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// DragoonEntity's message dispatcher vtable.
const MessageHandler DragoonEntity::messageHandlers[] = {
	h_Dragoon_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_Dragoon_Heal,
	h_Dragoon_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_Dragoon_ApplyDamageModifier,
	nullptr,
};

// DragoonEntity's constructor.
DragoonEntity::DragoonEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// AdvDragoonEntity
// ---------

// AdvDragoonEntity's component offset vtable.
const int AdvDragoonEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(AdvDragoonEntity, c_TeamComponent),
	0,
	myoffsetof(AdvDragoonEntity, c_HealthComponent),
	myoffsetof(AdvDragoonEntity, c_ClientComponent),
	myoffsetof(AdvDragoonEntity, c_ArmorComponent),
	myoffsetof(AdvDragoonEntity, c_KnockbackComponent),
	0,
	myoffsetof(AdvDragoonEntity, c_AlienClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// AdvDragoonEntity's Heal message dispatcher.
void h_AdvDragoon_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AdvDragoonEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// AdvDragoonEntity's ApplyDamageModifier message dispatcher.
void h_AdvDragoon_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AdvDragoonEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// AdvDragoonEntity's PrepareNetCode message dispatcher.
void h_AdvDragoon_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<AdvDragoonEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// AdvDragoonEntity's Damage message dispatcher.
void h_AdvDragoon_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AdvDragoonEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// AdvDragoonEntity's message dispatcher vtable.
const MessageHandler AdvDragoonEntity::messageHandlers[] = {
	h_AdvDragoon_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_AdvDragoon_Heal,
	h_AdvDragoon_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_AdvDragoon_ApplyDamageModifier,
	nullptr,
};

// AdvDragoonEntity's constructor.
AdvDragoonEntity::AdvDragoonEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// TyrantEntity
// ---------

// TyrantEntity's component offset vtable.
const int TyrantEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(TyrantEntity, c_TeamComponent),
	0,
	myoffsetof(TyrantEntity, c_HealthComponent),
	myoffsetof(TyrantEntity, c_ClientComponent),
	myoffsetof(TyrantEntity, c_ArmorComponent),
	myoffsetof(TyrantEntity, c_KnockbackComponent),
	0,
	myoffsetof(TyrantEntity, c_AlienClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// TyrantEntity's Heal message dispatcher.
void h_Tyrant_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TyrantEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// TyrantEntity's ApplyDamageModifier message dispatcher.
void h_Tyrant_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TyrantEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// TyrantEntity's PrepareNetCode message dispatcher.
void h_Tyrant_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<TyrantEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// TyrantEntity's Damage message dispatcher.
void h_Tyrant_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TyrantEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// TyrantEntity's message dispatcher vtable.
const MessageHandler TyrantEntity::messageHandlers[] = {
	h_Tyrant_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_Tyrant_Heal,
	h_Tyrant_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_Tyrant_ApplyDamageModifier,
	nullptr,
};

// TyrantEntity's constructor.
TyrantEntity::TyrantEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// GrangerEntity
// ---------

// GrangerEntity's component offset vtable.
const int GrangerEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(GrangerEntity, c_TeamComponent),
	0,
	myoffsetof(GrangerEntity, c_HealthComponent),
	myoffsetof(GrangerEntity, c_ClientComponent),
	myoffsetof(GrangerEntity, c_ArmorComponent),
	myoffsetof(GrangerEntity, c_KnockbackComponent),
	0,
	myoffsetof(GrangerEntity, c_AlienClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// GrangerEntity's Heal message dispatcher.
void h_Granger_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<GrangerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// GrangerEntity's ApplyDamageModifier message dispatcher.
void h_Granger_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<GrangerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// GrangerEntity's PrepareNetCode message dispatcher.
void h_Granger_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<GrangerEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// GrangerEntity's Damage message dispatcher.
void h_Granger_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<GrangerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// GrangerEntity's message dispatcher vtable.
const MessageHandler GrangerEntity::messageHandlers[] = {
	h_Granger_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_Granger_Heal,
	h_Granger_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_Granger_ApplyDamageModifier,
	nullptr,
};

// GrangerEntity's constructor.
GrangerEntity::GrangerEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// AdvGrangerEntity
// ---------

// AdvGrangerEntity's component offset vtable.
const int AdvGrangerEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(AdvGrangerEntity, c_TeamComponent),
	0,
	myoffsetof(AdvGrangerEntity, c_HealthComponent),
	myoffsetof(AdvGrangerEntity, c_ClientComponent),
	myoffsetof(AdvGrangerEntity, c_ArmorComponent),
	myoffsetof(AdvGrangerEntity, c_KnockbackComponent),
	0,
	myoffsetof(AdvGrangerEntity, c_AlienClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// AdvGrangerEntity's Heal message dispatcher.
void h_AdvGranger_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AdvGrangerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// AdvGrangerEntity's ApplyDamageModifier message dispatcher.
void h_AdvGranger_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AdvGrangerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// AdvGrangerEntity's PrepareNetCode message dispatcher.
void h_AdvGranger_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<AdvGrangerEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// AdvGrangerEntity's Damage message dispatcher.
void h_AdvGranger_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AdvGrangerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// AdvGrangerEntity's message dispatcher vtable.
const MessageHandler AdvGrangerEntity::messageHandlers[] = {
	h_AdvGranger_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_AdvGranger_Heal,
	h_AdvGranger_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_AdvGranger_ApplyDamageModifier,
	nullptr,
};

// AdvGrangerEntity's constructor.
AdvGrangerEntity::AdvGrangerEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// NakedHumanEntity
// ---------

// NakedHumanEntity's component offset vtable.
const int NakedHumanEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(NakedHumanEntity, c_TeamComponent),
	0,
	myoffsetof(NakedHumanEntity, c_HealthComponent),
	myoffsetof(NakedHumanEntity, c_ClientComponent),
	myoffsetof(NakedHumanEntity, c_ArmorComponent),
	myoffsetof(NakedHumanEntity, c_KnockbackComponent),
	0,
	0,
	myoffsetof(NakedHumanEntity, c_HumanClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// NakedHumanEntity's Heal message dispatcher.
void h_NakedHuman_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<NakedHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// NakedHumanEntity's ApplyDamageModifier message dispatcher.
void h_NakedHuman_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<NakedHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// NakedHumanEntity's PrepareNetCode message dispatcher.
void h_NakedHuman_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<NakedHumanEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// NakedHumanEntity's Damage message dispatcher.
void h_NakedHuman_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<NakedHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// NakedHumanEntity's message dispatcher vtable.
const MessageHandler NakedHumanEntity::messageHandlers[] = {
	h_NakedHuman_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_NakedHuman_Heal,
	h_NakedHuman_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_NakedHuman_ApplyDamageModifier,
	nullptr,
};

// NakedHumanEntity's constructor.
NakedHumanEntity::NakedHumanEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_HUMANS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_HumanClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// LightHumanEntity
// ---------

// LightHumanEntity's component offset vtable.
const int LightHumanEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(LightHumanEntity, c_TeamComponent),
	0,
	myoffsetof(LightHumanEntity, c_HealthComponent),
	myoffsetof(LightHumanEntity, c_ClientComponent),
	myoffsetof(LightHumanEntity, c_ArmorComponent),
	myoffsetof(LightHumanEntity, c_KnockbackComponent),
	0,
	0,
	myoffsetof(LightHumanEntity, c_HumanClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// LightHumanEntity's Heal message dispatcher.
void h_LightHuman_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<LightHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// LightHumanEntity's ApplyDamageModifier message dispatcher.
void h_LightHuman_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<LightHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// LightHumanEntity's PrepareNetCode message dispatcher.
void h_LightHuman_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<LightHumanEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// LightHumanEntity's Damage message dispatcher.
void h_LightHuman_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<LightHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// LightHumanEntity's message dispatcher vtable.
const MessageHandler LightHumanEntity::messageHandlers[] = {
	h_LightHuman_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_LightHuman_Heal,
	h_LightHuman_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_LightHuman_ApplyDamageModifier,
	nullptr,
};

// LightHumanEntity's constructor.
LightHumanEntity::LightHumanEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_HUMANS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_HumanClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// MediumHumanEntity
// ---------

// MediumHumanEntity's component offset vtable.
const int MediumHumanEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(MediumHumanEntity, c_TeamComponent),
	0,
	myoffsetof(MediumHumanEntity, c_HealthComponent),
	myoffsetof(MediumHumanEntity, c_ClientComponent),
	myoffsetof(MediumHumanEntity, c_ArmorComponent),
	myoffsetof(MediumHumanEntity, c_KnockbackComponent),
	0,
	0,
	myoffsetof(MediumHumanEntity, c_HumanClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// MediumHumanEntity's Heal message dispatcher.
void h_MediumHuman_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MediumHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// MediumHumanEntity's ApplyDamageModifier message dispatcher.
void h_MediumHuman_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MediumHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// MediumHumanEntity's PrepareNetCode message dispatcher.
void h_MediumHuman_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<MediumHumanEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// MediumHumanEntity's Damage message dispatcher.
void h_MediumHuman_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MediumHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// MediumHumanEntity's message dispatcher vtable.
const MessageHandler MediumHumanEntity::messageHandlers[] = {
	h_MediumHuman_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_MediumHuman_Heal,
	h_MediumHuman_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_MediumHuman_ApplyDamageModifier,
	nullptr,
};

// MediumHumanEntity's constructor.
MediumHumanEntity::MediumHumanEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_HUMANS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_HumanClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// HeavyHumanEntity
// ---------

// HeavyHumanEntity's component offset vtable.
const int HeavyHumanEntity::componentOffsets[] = {
	0,
	0,
	myoffsetof(HeavyHumanEntity, c_TeamComponent),
	0,
	myoffsetof(HeavyHumanEntity, c_HealthComponent),
	myoffsetof(HeavyHumanEntity, c_ClientComponent),
	myoffsetof(HeavyHumanEntity, c_ArmorComponent),
	myoffsetof(HeavyHumanEntity, c_KnockbackComponent),
	0,
	0,
	myoffsetof(HeavyHumanEntity, c_HumanClassComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// HeavyHumanEntity's Heal message dispatcher.
void h_HeavyHuman_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<HeavyHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// HeavyHumanEntity's ApplyDamageModifier message dispatcher.
void h_HeavyHuman_ApplyDamageModifier(Entity* _entity, const void* _data) {
	auto* entity = static_cast<HeavyHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float&, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
}

// HeavyHumanEntity's PrepareNetCode message dispatcher.
void h_HeavyHuman_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<HeavyHumanEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// HeavyHumanEntity's Damage message dispatcher.
void h_HeavyHuman_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<HeavyHumanEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// HeavyHumanEntity's message dispatcher vtable.
const MessageHandler HeavyHumanEntity::messageHandlers[] = {
	h_HeavyHuman_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_HeavyHuman_Heal,
	h_HeavyHuman_Damage,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_HeavyHuman_ApplyDamageModifier,
	nullptr,
};

// HeavyHumanEntity's constructor.
HeavyHumanEntity::HeavyHumanEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_TeamComponent(*this, TEAM_HUMANS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData, c_TeamComponent)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_HumanClassComponent(*this, c_ClientComponent, c_TeamComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// AcidTubeEntity
// ---------

// AcidTubeEntity's component offset vtable.
const int AcidTubeEntity::componentOffsets[] = {
	myoffsetof(AcidTubeEntity, c_DeferredFreeingComponent),
	myoffsetof(AcidTubeEntity, c_ThinkingComponent),
	myoffsetof(AcidTubeEntity, c_TeamComponent),
	myoffsetof(AcidTubeEntity, c_IgnitableComponent),
	myoffsetof(AcidTubeEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(AcidTubeEntity, c_BuildableComponent),
	myoffsetof(AcidTubeEntity, c_AlienBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	myoffsetof(AcidTubeEntity, c_AcidTubeComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// AcidTubeEntity's Extinguish message dispatcher.
void h_AcidTube_Extinguish(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AcidTubeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
}

// AcidTubeEntity's Heal message dispatcher.
void h_AcidTube_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AcidTubeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// AcidTubeEntity's Ignite message dispatcher.
void h_AcidTube_Ignite(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AcidTubeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*>*>(_data);
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// AcidTubeEntity's PrepareNetCode message dispatcher.
void h_AcidTube_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<AcidTubeEntity*>(_entity);
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// AcidTubeEntity's FreeAt message dispatcher.
void h_AcidTube_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AcidTubeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// AcidTubeEntity's Damage message dispatcher.
void h_AcidTube_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AcidTubeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// AcidTubeEntity's Die message dispatcher.
void h_AcidTube_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<AcidTubeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// AcidTubeEntity's message dispatcher vtable.
const MessageHandler AcidTubeEntity::messageHandlers[] = {
	h_AcidTube_PrepareNetCode,
	h_AcidTube_FreeAt,
	h_AcidTube_Ignite,
	h_AcidTube_Extinguish,
	h_AcidTube_Heal,
	h_AcidTube_Damage,
	h_AcidTube_Die,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// AcidTubeEntity's constructor.
AcidTubeEntity::AcidTubeEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_TeamComponent, c_IgnitableComponent)
	, c_AcidTubeComponent(*this, c_AlienBuildableComponent)
{}

// BarricadeEntity
// ---------

// BarricadeEntity's component offset vtable.
const int BarricadeEntity::componentOffsets[] = {
	myoffsetof(BarricadeEntity, c_DeferredFreeingComponent),
	myoffsetof(BarricadeEntity, c_ThinkingComponent),
	myoffsetof(BarricadeEntity, c_TeamComponent),
	myoffsetof(BarricadeEntity, c_IgnitableComponent),
	myoffsetof(BarricadeEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(BarricadeEntity, c_BuildableComponent),
	myoffsetof(BarricadeEntity, c_AlienBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(BarricadeEntity, c_BarricadeComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// BarricadeEntity's Extinguish message dispatcher.
void h_Barricade_Extinguish(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BarricadeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
}

// BarricadeEntity's Heal message dispatcher.
void h_Barricade_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BarricadeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// BarricadeEntity's Ignite message dispatcher.
void h_Barricade_Ignite(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BarricadeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*>*>(_data);
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// BarricadeEntity's PrepareNetCode message dispatcher.
void h_Barricade_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<BarricadeEntity*>(_entity);
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// BarricadeEntity's FreeAt message dispatcher.
void h_Barricade_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BarricadeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// BarricadeEntity's Damage message dispatcher.
void h_Barricade_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BarricadeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_BarricadeComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// BarricadeEntity's Die message dispatcher.
void h_Barricade_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BarricadeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_BarricadeComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// BarricadeEntity's message dispatcher vtable.
const MessageHandler BarricadeEntity::messageHandlers[] = {
	h_Barricade_PrepareNetCode,
	h_Barricade_FreeAt,
	h_Barricade_Ignite,
	h_Barricade_Extinguish,
	h_Barricade_Heal,
	h_Barricade_Damage,
	h_Barricade_Die,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// BarricadeEntity's constructor.
BarricadeEntity::BarricadeEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_TeamComponent, c_IgnitableComponent)
	, c_BarricadeComponent(*this, c_AlienBuildableComponent)
{}

// BoosterEntity
// ---------

// BoosterEntity's component offset vtable.
const int BoosterEntity::componentOffsets[] = {
	myoffsetof(BoosterEntity, c_DeferredFreeingComponent),
	myoffsetof(BoosterEntity, c_ThinkingComponent),
	myoffsetof(BoosterEntity, c_TeamComponent),
	myoffsetof(BoosterEntity, c_IgnitableComponent),
	myoffsetof(BoosterEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(BoosterEntity, c_BuildableComponent),
	myoffsetof(BoosterEntity, c_AlienBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(BoosterEntity, c_BoosterComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// BoosterEntity's Extinguish message dispatcher.
void h_Booster_Extinguish(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BoosterEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
}

// BoosterEntity's Heal message dispatcher.
void h_Booster_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BoosterEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// BoosterEntity's Ignite message dispatcher.
void h_Booster_Ignite(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BoosterEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*>*>(_data);
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// BoosterEntity's PrepareNetCode message dispatcher.
void h_Booster_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<BoosterEntity*>(_entity);
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// BoosterEntity's FreeAt message dispatcher.
void h_Booster_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BoosterEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// BoosterEntity's Damage message dispatcher.
void h_Booster_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BoosterEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// BoosterEntity's Die message dispatcher.
void h_Booster_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<BoosterEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// BoosterEntity's message dispatcher vtable.
const MessageHandler BoosterEntity::messageHandlers[] = {
	h_Booster_PrepareNetCode,
	h_Booster_FreeAt,
	h_Booster_Ignite,
	h_Booster_Extinguish,
	h_Booster_Heal,
	h_Booster_Damage,
	h_Booster_Die,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// BoosterEntity's constructor.
BoosterEntity::BoosterEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_TeamComponent, c_IgnitableComponent)
	, c_BoosterComponent(*this, c_AlienBuildableComponent)
{}

// EggEntity
// ---------

// EggEntity's component offset vtable.
const int EggEntity::componentOffsets[] = {
	myoffsetof(EggEntity, c_DeferredFreeingComponent),
	myoffsetof(EggEntity, c_ThinkingComponent),
	myoffsetof(EggEntity, c_TeamComponent),
	myoffsetof(EggEntity, c_IgnitableComponent),
	myoffsetof(EggEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(EggEntity, c_BuildableComponent),
	myoffsetof(EggEntity, c_AlienBuildableComponent),
	0,
	0,
	myoffsetof(EggEntity, c_SpawnerComponent),
	0,
	0,
	0,
	0,
	0,
	myoffsetof(EggEntity, c_EggComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// EggEntity's Extinguish message dispatcher.
void h_Egg_Extinguish(Entity* _entity, const void* _data) {
	auto* entity = static_cast<EggEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
}

// EggEntity's Heal message dispatcher.
void h_Egg_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<EggEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// EggEntity's CheckSpawnPoint message dispatcher.
void h_Egg_CheckSpawnPoint(Entity* _entity, const void* _data) {
	auto* entity = static_cast<EggEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<Entity*&, glm::vec3&>*>(_data);
	entity->c_EggComponent.HandleCheckSpawnPoint(std::get<0>(*data), std::get<1>(*data));
}

// EggEntity's Ignite message dispatcher.
void h_Egg_Ignite(Entity* _entity, const void* _data) {
	auto* entity = static_cast<EggEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*>*>(_data);
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// EggEntity's PrepareNetCode message dispatcher.
void h_Egg_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<EggEntity*>(_entity);
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// EggEntity's FreeAt message dispatcher.
void h_Egg_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<EggEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// EggEntity's Damage message dispatcher.
void h_Egg_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<EggEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// EggEntity's Die message dispatcher.
void h_Egg_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<EggEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_SpawnerComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// EggEntity's message dispatcher vtable.
const MessageHandler EggEntity::messageHandlers[] = {
	h_Egg_PrepareNetCode,
	h_Egg_FreeAt,
	h_Egg_Ignite,
	h_Egg_Extinguish,
	h_Egg_Heal,
	h_Egg_Damage,
	h_Egg_Die,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_Egg_CheckSpawnPoint,
};

// EggEntity's constructor.
EggEntity::EggEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_TeamComponent, c_IgnitableComponent)
	, c_SpawnerComponent(*this, c_TeamComponent, c_ThinkingComponent)
	, c_EggComponent(*this, c_AlienBuildableComponent, c_SpawnerComponent)
{}

// HiveEntity
// ---------

// HiveEntity's component offset vtable.
const int HiveEntity::componentOffsets[] = {
	myoffsetof(HiveEntity, c_DeferredFreeingComponent),
	myoffsetof(HiveEntity, c_ThinkingComponent),
	myoffsetof(HiveEntity, c_TeamComponent),
	myoffsetof(HiveEntity, c_IgnitableComponent),
	myoffsetof(HiveEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(HiveEntity, c_BuildableComponent),
	myoffsetof(HiveEntity, c_AlienBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(HiveEntity, c_HiveComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// HiveEntity's Extinguish message dispatcher.
void h_Hive_Extinguish(Entity* _entity, const void* _data) {
	auto* entity = static_cast<HiveEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
}

// HiveEntity's Heal message dispatcher.
void h_Hive_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<HiveEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// HiveEntity's Ignite message dispatcher.
void h_Hive_Ignite(Entity* _entity, const void* _data) {
	auto* entity = static_cast<HiveEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*>*>(_data);
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// HiveEntity's PrepareNetCode message dispatcher.
void h_Hive_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<HiveEntity*>(_entity);
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// HiveEntity's FreeAt message dispatcher.
void h_Hive_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<HiveEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// HiveEntity's Damage message dispatcher.
void h_Hive_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<HiveEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_HiveComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// HiveEntity's Die message dispatcher.
void h_Hive_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<HiveEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// HiveEntity's message dispatcher vtable.
const MessageHandler HiveEntity::messageHandlers[] = {
	h_Hive_PrepareNetCode,
	h_Hive_FreeAt,
	h_Hive_Ignite,
	h_Hive_Extinguish,
	h_Hive_Heal,
	h_Hive_Damage,
	h_Hive_Die,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// HiveEntity's constructor.
HiveEntity::HiveEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_TeamComponent, c_IgnitableComponent)
	, c_HiveComponent(*this, c_AlienBuildableComponent)
{}

// LeechEntity
// ---------

// LeechEntity's component offset vtable.
const int LeechEntity::componentOffsets[] = {
	myoffsetof(LeechEntity, c_DeferredFreeingComponent),
	myoffsetof(LeechEntity, c_ThinkingComponent),
	myoffsetof(LeechEntity, c_TeamComponent),
	myoffsetof(LeechEntity, c_IgnitableComponent),
	myoffsetof(LeechEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(LeechEntity, c_BuildableComponent),
	myoffsetof(LeechEntity, c_AlienBuildableComponent),
	0,
	0,
	0,
	0,
	myoffsetof(LeechEntity, c_MiningComponent),
	0,
	0,
	0,
	0,
	0,
	myoffsetof(LeechEntity, c_LeechComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// LeechEntity's Extinguish message dispatcher.
void h_Leech_Extinguish(Entity* _entity, const void* _data) {
	auto* entity = static_cast<LeechEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
}

// LeechEntity's Heal message dispatcher.
void h_Leech_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<LeechEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// LeechEntity's Ignite message dispatcher.
void h_Leech_Ignite(Entity* _entity, const void* _data) {
	auto* entity = static_cast<LeechEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*>*>(_data);
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// LeechEntity's FinishConstruction message dispatcher.
void h_Leech_FinishConstruction(Entity* _entity, const void*) {
	auto* entity = static_cast<LeechEntity*>(_entity);
	entity->c_MiningComponent.HandleFinishConstruction();
}

// LeechEntity's PrepareNetCode message dispatcher.
void h_Leech_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<LeechEntity*>(_entity);
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
	entity->c_MiningComponent.HandlePrepareNetCode();
}

// LeechEntity's FreeAt message dispatcher.
void h_Leech_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<LeechEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// LeechEntity's Damage message dispatcher.
void h_Leech_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<LeechEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// LeechEntity's Die message dispatcher.
void h_Leech_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<LeechEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_MiningComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// LeechEntity's message dispatcher vtable.
const MessageHandler LeechEntity::messageHandlers[] = {
	h_Leech_PrepareNetCode,
	h_Leech_FreeAt,
	h_Leech_Ignite,
	h_Leech_Extinguish,
	h_Leech_Heal,
	h_Leech_Damage,
	h_Leech_Die,
	h_Leech_FinishConstruction,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// LeechEntity's constructor.
LeechEntity::LeechEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_TeamComponent, c_IgnitableComponent)
	, c_MiningComponent(*this, c_ThinkingComponent)
	, c_LeechComponent(*this, c_AlienBuildableComponent, c_MiningComponent)
{}

// OvermindEntity
// ---------

// OvermindEntity's component offset vtable.
const int OvermindEntity::componentOffsets[] = {
	myoffsetof(OvermindEntity, c_DeferredFreeingComponent),
	myoffsetof(OvermindEntity, c_ThinkingComponent),
	myoffsetof(OvermindEntity, c_TeamComponent),
	myoffsetof(OvermindEntity, c_IgnitableComponent),
	myoffsetof(OvermindEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(OvermindEntity, c_BuildableComponent),
	myoffsetof(OvermindEntity, c_AlienBuildableComponent),
	0,
	myoffsetof(OvermindEntity, c_MainBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(OvermindEntity, c_OvermindComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// OvermindEntity's Extinguish message dispatcher.
void h_Overmind_Extinguish(Entity* _entity, const void* _data) {
	auto* entity = static_cast<OvermindEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
}

// OvermindEntity's Heal message dispatcher.
void h_Overmind_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<OvermindEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// OvermindEntity's Ignite message dispatcher.
void h_Overmind_Ignite(Entity* _entity, const void* _data) {
	auto* entity = static_cast<OvermindEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*>*>(_data);
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// OvermindEntity's FinishConstruction message dispatcher.
void h_Overmind_FinishConstruction(Entity* _entity, const void*) {
	auto* entity = static_cast<OvermindEntity*>(_entity);
	entity->c_MainBuildableComponent.HandleFinishConstruction();
	entity->c_OvermindComponent.HandleFinishConstruction();
}

// OvermindEntity's PrepareNetCode message dispatcher.
void h_Overmind_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<OvermindEntity*>(_entity);
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
	entity->c_OvermindComponent.HandlePrepareNetCode();
}

// OvermindEntity's FreeAt message dispatcher.
void h_Overmind_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<OvermindEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// OvermindEntity's Damage message dispatcher.
void h_Overmind_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<OvermindEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_MainBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// OvermindEntity's Die message dispatcher.
void h_Overmind_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<OvermindEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_MainBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// OvermindEntity's message dispatcher vtable.
const MessageHandler OvermindEntity::messageHandlers[] = {
	h_Overmind_PrepareNetCode,
	h_Overmind_FreeAt,
	h_Overmind_Ignite,
	h_Overmind_Extinguish,
	h_Overmind_Heal,
	h_Overmind_Damage,
	h_Overmind_Die,
	h_Overmind_FinishConstruction,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// OvermindEntity's constructor.
OvermindEntity::OvermindEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_TeamComponent, c_IgnitableComponent)
	, c_MainBuildableComponent(*this, c_BuildableComponent)
	, c_OvermindComponent(*this, c_AlienBuildableComponent, c_MainBuildableComponent)
{}

// SpikerEntity
// ---------

// SpikerEntity's component offset vtable.
const int SpikerEntity::componentOffsets[] = {
	myoffsetof(SpikerEntity, c_DeferredFreeingComponent),
	myoffsetof(SpikerEntity, c_ThinkingComponent),
	myoffsetof(SpikerEntity, c_TeamComponent),
	myoffsetof(SpikerEntity, c_IgnitableComponent),
	myoffsetof(SpikerEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(SpikerEntity, c_BuildableComponent),
	myoffsetof(SpikerEntity, c_AlienBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(SpikerEntity, c_SpikerComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// SpikerEntity's Extinguish message dispatcher.
void h_Spiker_Extinguish(Entity* _entity, const void* _data) {
	auto* entity = static_cast<SpikerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
}

// SpikerEntity's Heal message dispatcher.
void h_Spiker_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<SpikerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// SpikerEntity's Ignite message dispatcher.
void h_Spiker_Ignite(Entity* _entity, const void* _data) {
	auto* entity = static_cast<SpikerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*>*>(_data);
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// SpikerEntity's PrepareNetCode message dispatcher.
void h_Spiker_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<SpikerEntity*>(_entity);
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// SpikerEntity's FreeAt message dispatcher.
void h_Spiker_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<SpikerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// SpikerEntity's Damage message dispatcher.
void h_Spiker_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<SpikerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_SpikerComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// SpikerEntity's Die message dispatcher.
void h_Spiker_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<SpikerEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// SpikerEntity's message dispatcher vtable.
const MessageHandler SpikerEntity::messageHandlers[] = {
	h_Spiker_PrepareNetCode,
	h_Spiker_FreeAt,
	h_Spiker_Ignite,
	h_Spiker_Extinguish,
	h_Spiker_Heal,
	h_Spiker_Damage,
	h_Spiker_Die,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// SpikerEntity's constructor.
SpikerEntity::SpikerEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_TeamComponent, c_IgnitableComponent)
	, c_SpikerComponent(*this, c_AlienBuildableComponent)
{}

// TrapperEntity
// ---------

// TrapperEntity's component offset vtable.
const int TrapperEntity::componentOffsets[] = {
	myoffsetof(TrapperEntity, c_DeferredFreeingComponent),
	myoffsetof(TrapperEntity, c_ThinkingComponent),
	myoffsetof(TrapperEntity, c_TeamComponent),
	myoffsetof(TrapperEntity, c_IgnitableComponent),
	myoffsetof(TrapperEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(TrapperEntity, c_BuildableComponent),
	myoffsetof(TrapperEntity, c_AlienBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(TrapperEntity, c_TrapperComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// TrapperEntity's Extinguish message dispatcher.
void h_Trapper_Extinguish(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TrapperEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
}

// TrapperEntity's Heal message dispatcher.
void h_Trapper_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TrapperEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// TrapperEntity's Ignite message dispatcher.
void h_Trapper_Ignite(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TrapperEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*>*>(_data);
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// TrapperEntity's PrepareNetCode message dispatcher.
void h_Trapper_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<TrapperEntity*>(_entity);
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// TrapperEntity's FreeAt message dispatcher.
void h_Trapper_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TrapperEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// TrapperEntity's Damage message dispatcher.
void h_Trapper_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TrapperEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// TrapperEntity's Die message dispatcher.
void h_Trapper_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TrapperEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// TrapperEntity's message dispatcher vtable.
const MessageHandler TrapperEntity::messageHandlers[] = {
	h_Trapper_PrepareNetCode,
	h_Trapper_FreeAt,
	h_Trapper_Ignite,
	h_Trapper_Extinguish,
	h_Trapper_Heal,
	h_Trapper_Damage,
	h_Trapper_Die,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// TrapperEntity's constructor.
TrapperEntity::TrapperEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_ALIENS)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_TeamComponent, c_IgnitableComponent)
	, c_TrapperComponent(*this, c_AlienBuildableComponent)
{}

// ArmouryEntity
// ---------

// ArmouryEntity's component offset vtable.
const int ArmouryEntity::componentOffsets[] = {
	myoffsetof(ArmouryEntity, c_DeferredFreeingComponent),
	myoffsetof(ArmouryEntity, c_ThinkingComponent),
	myoffsetof(ArmouryEntity, c_TeamComponent),
	0,
	myoffsetof(ArmouryEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(ArmouryEntity, c_BuildableComponent),
	0,
	myoffsetof(ArmouryEntity, c_HumanBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(ArmouryEntity, c_ArmouryComponent),
	0,
	0,
	0,
	0,
	0,
	0,
};

// ArmouryEntity's Heal message dispatcher.
void h_Armoury_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<ArmouryEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// ArmouryEntity's Die message dispatcher.
void h_Armoury_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<ArmouryEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// ArmouryEntity's PrepareNetCode message dispatcher.
void h_Armoury_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<ArmouryEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// ArmouryEntity's FreeAt message dispatcher.
void h_Armoury_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<ArmouryEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// ArmouryEntity's Damage message dispatcher.
void h_Armoury_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<ArmouryEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// ArmouryEntity's message dispatcher vtable.
const MessageHandler ArmouryEntity::messageHandlers[] = {
	h_Armoury_PrepareNetCode,
	h_Armoury_FreeAt,
	nullptr,
	nullptr,
	h_Armoury_Heal,
	h_Armoury_Damage,
	h_Armoury_Die,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// ArmouryEntity's constructor.
ArmouryEntity::ArmouryEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_HUMANS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent, c_TeamComponent)
	, c_ArmouryComponent(*this, c_HumanBuildableComponent)
{}

// DrillEntity
// ---------

// DrillEntity's component offset vtable.
const int DrillEntity::componentOffsets[] = {
	myoffsetof(DrillEntity, c_DeferredFreeingComponent),
	myoffsetof(DrillEntity, c_ThinkingComponent),
	myoffsetof(DrillEntity, c_TeamComponent),
	0,
	myoffsetof(DrillEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(DrillEntity, c_BuildableComponent),
	0,
	myoffsetof(DrillEntity, c_HumanBuildableComponent),
	0,
	0,
	0,
	myoffsetof(DrillEntity, c_MiningComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(DrillEntity, c_DrillComponent),
	0,
	0,
	0,
	0,
	0,
};

// DrillEntity's Heal message dispatcher.
void h_Drill_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<DrillEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// DrillEntity's Die message dispatcher.
void h_Drill_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<DrillEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_MiningComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// DrillEntity's FinishConstruction message dispatcher.
void h_Drill_FinishConstruction(Entity* _entity, const void*) {
	auto* entity = static_cast<DrillEntity*>(_entity);
	entity->c_MiningComponent.HandleFinishConstruction();
}

// DrillEntity's PrepareNetCode message dispatcher.
void h_Drill_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<DrillEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
	entity->c_MiningComponent.HandlePrepareNetCode();
}

// DrillEntity's FreeAt message dispatcher.
void h_Drill_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<DrillEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// DrillEntity's Damage message dispatcher.
void h_Drill_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<DrillEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// DrillEntity's message dispatcher vtable.
const MessageHandler DrillEntity::messageHandlers[] = {
	h_Drill_PrepareNetCode,
	h_Drill_FreeAt,
	nullptr,
	nullptr,
	h_Drill_Heal,
	h_Drill_Damage,
	h_Drill_Die,
	h_Drill_FinishConstruction,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// DrillEntity's constructor.
DrillEntity::DrillEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_HUMANS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent, c_TeamComponent)
	, c_MiningComponent(*this, c_ThinkingComponent)
	, c_DrillComponent(*this, c_HumanBuildableComponent, c_MiningComponent)
{}

// MedipadEntity
// ---------

// MedipadEntity's component offset vtable.
const int MedipadEntity::componentOffsets[] = {
	myoffsetof(MedipadEntity, c_DeferredFreeingComponent),
	myoffsetof(MedipadEntity, c_ThinkingComponent),
	myoffsetof(MedipadEntity, c_TeamComponent),
	0,
	myoffsetof(MedipadEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(MedipadEntity, c_BuildableComponent),
	0,
	myoffsetof(MedipadEntity, c_HumanBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(MedipadEntity, c_MedipadComponent),
	0,
	0,
	0,
	0,
};

// MedipadEntity's Heal message dispatcher.
void h_Medipad_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MedipadEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// MedipadEntity's Die message dispatcher.
void h_Medipad_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MedipadEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_MedipadComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// MedipadEntity's PrepareNetCode message dispatcher.
void h_Medipad_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<MedipadEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// MedipadEntity's FreeAt message dispatcher.
void h_Medipad_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MedipadEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// MedipadEntity's Damage message dispatcher.
void h_Medipad_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MedipadEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// MedipadEntity's message dispatcher vtable.
const MessageHandler MedipadEntity::messageHandlers[] = {
	h_Medipad_PrepareNetCode,
	h_Medipad_FreeAt,
	nullptr,
	nullptr,
	h_Medipad_Heal,
	h_Medipad_Damage,
	h_Medipad_Die,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// MedipadEntity's constructor.
MedipadEntity::MedipadEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_HUMANS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent, c_TeamComponent)
	, c_MedipadComponent(*this, c_HumanBuildableComponent)
{}

// MGTurretEntity
// ---------

// MGTurretEntity's component offset vtable.
const int MGTurretEntity::componentOffsets[] = {
	myoffsetof(MGTurretEntity, c_DeferredFreeingComponent),
	myoffsetof(MGTurretEntity, c_ThinkingComponent),
	myoffsetof(MGTurretEntity, c_TeamComponent),
	0,
	myoffsetof(MGTurretEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(MGTurretEntity, c_BuildableComponent),
	0,
	myoffsetof(MGTurretEntity, c_HumanBuildableComponent),
	0,
	0,
	myoffsetof(MGTurretEntity, c_TurretComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(MGTurretEntity, c_MGTurretComponent),
	0,
	0,
	0,
};

// MGTurretEntity's Heal message dispatcher.
void h_MGTurret_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MGTurretEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// MGTurretEntity's PowerUp message dispatcher.
void h_MGTurret_PowerUp(Entity* _entity, const void*) {
	auto* entity = static_cast<MGTurretEntity*>(_entity);
	entity->c_MGTurretComponent.HandlePowerUp();
}

// MGTurretEntity's PowerDown message dispatcher.
void h_MGTurret_PowerDown(Entity* _entity, const void*) {
	auto* entity = static_cast<MGTurretEntity*>(_entity);
	entity->c_MGTurretComponent.HandlePowerDown();
}

// MGTurretEntity's PrepareNetCode message dispatcher.
void h_MGTurret_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<MGTurretEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
	entity->c_TurretComponent.HandlePrepareNetCode();
	entity->c_MGTurretComponent.HandlePrepareNetCode();
}

// MGTurretEntity's FreeAt message dispatcher.
void h_MGTurret_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MGTurretEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// MGTurretEntity's Damage message dispatcher.
void h_MGTurret_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MGTurretEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// MGTurretEntity's Die message dispatcher.
void h_MGTurret_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<MGTurretEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_MGTurretComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// MGTurretEntity's message dispatcher vtable.
const MessageHandler MGTurretEntity::messageHandlers[] = {
	h_MGTurret_PrepareNetCode,
	h_MGTurret_FreeAt,
	nullptr,
	nullptr,
	h_MGTurret_Heal,
	h_MGTurret_Damage,
	h_MGTurret_Die,
	nullptr,
	h_MGTurret_PowerUp,
	h_MGTurret_PowerDown,
	nullptr,
	nullptr,
};

// MGTurretEntity's constructor.
MGTurretEntity::MGTurretEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_HUMANS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent, c_TeamComponent)
	, c_TurretComponent(*this)
	, c_MGTurretComponent(*this, c_HumanBuildableComponent, c_TurretComponent)
{}

// ReactorEntity
// ---------

// ReactorEntity's component offset vtable.
const int ReactorEntity::componentOffsets[] = {
	myoffsetof(ReactorEntity, c_DeferredFreeingComponent),
	myoffsetof(ReactorEntity, c_ThinkingComponent),
	myoffsetof(ReactorEntity, c_TeamComponent),
	0,
	myoffsetof(ReactorEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(ReactorEntity, c_BuildableComponent),
	0,
	myoffsetof(ReactorEntity, c_HumanBuildableComponent),
	myoffsetof(ReactorEntity, c_MainBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(ReactorEntity, c_ReactorComponent),
	0,
	0,
};

// ReactorEntity's Heal message dispatcher.
void h_Reactor_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<ReactorEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// ReactorEntity's Die message dispatcher.
void h_Reactor_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<ReactorEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_MainBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// ReactorEntity's FinishConstruction message dispatcher.
void h_Reactor_FinishConstruction(Entity* _entity, const void*) {
	auto* entity = static_cast<ReactorEntity*>(_entity);
	entity->c_MainBuildableComponent.HandleFinishConstruction();
}

// ReactorEntity's PrepareNetCode message dispatcher.
void h_Reactor_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<ReactorEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// ReactorEntity's FreeAt message dispatcher.
void h_Reactor_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<ReactorEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// ReactorEntity's Damage message dispatcher.
void h_Reactor_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<ReactorEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_MainBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// ReactorEntity's message dispatcher vtable.
const MessageHandler ReactorEntity::messageHandlers[] = {
	h_Reactor_PrepareNetCode,
	h_Reactor_FreeAt,
	nullptr,
	nullptr,
	h_Reactor_Heal,
	h_Reactor_Damage,
	h_Reactor_Die,
	h_Reactor_FinishConstruction,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// ReactorEntity's constructor.
ReactorEntity::ReactorEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_HUMANS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent, c_TeamComponent)
	, c_MainBuildableComponent(*this, c_BuildableComponent)
	, c_ReactorComponent(*this, c_HumanBuildableComponent, c_MainBuildableComponent)
{}

// RocketpodEntity
// ---------

// RocketpodEntity's component offset vtable.
const int RocketpodEntity::componentOffsets[] = {
	myoffsetof(RocketpodEntity, c_DeferredFreeingComponent),
	myoffsetof(RocketpodEntity, c_ThinkingComponent),
	myoffsetof(RocketpodEntity, c_TeamComponent),
	0,
	myoffsetof(RocketpodEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(RocketpodEntity, c_BuildableComponent),
	0,
	myoffsetof(RocketpodEntity, c_HumanBuildableComponent),
	0,
	0,
	myoffsetof(RocketpodEntity, c_TurretComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(RocketpodEntity, c_RocketpodComponent),
	0,
};

// RocketpodEntity's Heal message dispatcher.
void h_Rocketpod_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<RocketpodEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// RocketpodEntity's PowerUp message dispatcher.
void h_Rocketpod_PowerUp(Entity* _entity, const void*) {
	auto* entity = static_cast<RocketpodEntity*>(_entity);
	entity->c_RocketpodComponent.HandlePowerUp();
}

// RocketpodEntity's PowerDown message dispatcher.
void h_Rocketpod_PowerDown(Entity* _entity, const void*) {
	auto* entity = static_cast<RocketpodEntity*>(_entity);
	entity->c_RocketpodComponent.HandlePowerDown();
}

// RocketpodEntity's PrepareNetCode message dispatcher.
void h_Rocketpod_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<RocketpodEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
	entity->c_TurretComponent.HandlePrepareNetCode();
	entity->c_RocketpodComponent.HandlePrepareNetCode();
}

// RocketpodEntity's FreeAt message dispatcher.
void h_Rocketpod_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<RocketpodEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// RocketpodEntity's Damage message dispatcher.
void h_Rocketpod_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<RocketpodEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// RocketpodEntity's Die message dispatcher.
void h_Rocketpod_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<RocketpodEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_RocketpodComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// RocketpodEntity's message dispatcher vtable.
const MessageHandler RocketpodEntity::messageHandlers[] = {
	h_Rocketpod_PrepareNetCode,
	h_Rocketpod_FreeAt,
	nullptr,
	nullptr,
	h_Rocketpod_Heal,
	h_Rocketpod_Damage,
	h_Rocketpod_Die,
	nullptr,
	h_Rocketpod_PowerUp,
	h_Rocketpod_PowerDown,
	nullptr,
	nullptr,
};

// RocketpodEntity's constructor.
RocketpodEntity::RocketpodEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_HUMANS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent, c_TeamComponent)
	, c_TurretComponent(*this)
	, c_RocketpodComponent(*this, c_HumanBuildableComponent, c_TurretComponent)
{}

// TelenodeEntity
// ---------

// TelenodeEntity's component offset vtable.
const int TelenodeEntity::componentOffsets[] = {
	myoffsetof(TelenodeEntity, c_DeferredFreeingComponent),
	myoffsetof(TelenodeEntity, c_ThinkingComponent),
	myoffsetof(TelenodeEntity, c_TeamComponent),
	0,
	myoffsetof(TelenodeEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(TelenodeEntity, c_BuildableComponent),
	0,
	myoffsetof(TelenodeEntity, c_HumanBuildableComponent),
	0,
	myoffsetof(TelenodeEntity, c_SpawnerComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	myoffsetof(TelenodeEntity, c_TelenodeComponent),
};

// TelenodeEntity's Heal message dispatcher.
void h_Telenode_Heal(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TelenodeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*>*>(_data);
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// TelenodeEntity's Die message dispatcher.
void h_Telenode_Die(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TelenodeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*, meansOfDeath_t>*>(_data);
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_SpawnerComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// TelenodeEntity's PrepareNetCode message dispatcher.
void h_Telenode_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<TelenodeEntity*>(_entity);
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// TelenodeEntity's FreeAt message dispatcher.
void h_Telenode_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TelenodeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// TelenodeEntity's CheckSpawnPoint message dispatcher.
void h_Telenode_CheckSpawnPoint(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TelenodeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<Entity*&, glm::vec3&>*>(_data);
	entity->c_TelenodeComponent.HandleCheckSpawnPoint(std::get<0>(*data), std::get<1>(*data));
}

// TelenodeEntity's Damage message dispatcher.
void h_Telenode_Damage(Entity* _entity, const void* _data) {
	auto* entity = static_cast<TelenodeEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<float, gentity_t*, Util::optional<glm::vec3>, Util::optional<glm::vec3>, int, meansOfDeath_t>*>(_data);
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// TelenodeEntity's message dispatcher vtable.
const MessageHandler TelenodeEntity::messageHandlers[] = {
	h_Telenode_PrepareNetCode,
	h_Telenode_FreeAt,
	nullptr,
	nullptr,
	h_Telenode_Heal,
	h_Telenode_Damage,
	h_Telenode_Die,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	h_Telenode_CheckSpawnPoint,
};

// TelenodeEntity's constructor.
TelenodeEntity::TelenodeEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_TeamComponent(*this, TEAM_HUMANS)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent, c_TeamComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent, c_TeamComponent)
	, c_SpawnerComponent(*this, c_TeamComponent, c_ThinkingComponent)
	, c_TelenodeComponent(*this, c_HumanBuildableComponent, c_SpawnerComponent)
{}

// FireEntity
// ---------

// FireEntity's component offset vtable.
const int FireEntity::componentOffsets[] = {
	myoffsetof(FireEntity, c_DeferredFreeingComponent),
	myoffsetof(FireEntity, c_ThinkingComponent),
	0,
	myoffsetof(FireEntity, c_IgnitableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// FireEntity's Ignite message dispatcher.
void h_Fire_Ignite(Entity* _entity, const void* _data) {
	auto* entity = static_cast<FireEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<gentity_t*>*>(_data);
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// FireEntity's PrepareNetCode message dispatcher.
void h_Fire_PrepareNetCode(Entity* _entity, const void*) {
	auto* entity = static_cast<FireEntity*>(_entity);
	entity->c_IgnitableComponent.HandlePrepareNetCode();
}

// FireEntity's FreeAt message dispatcher.
void h_Fire_FreeAt(Entity* _entity, const void* _data) {
	auto* entity = static_cast<FireEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// FireEntity's Extinguish message dispatcher.
void h_Fire_Extinguish(Entity* _entity, const void* _data) {
	auto* entity = static_cast<FireEntity*>(_entity);
	const auto* data = static_cast<const std::tuple<int>*>(_data);
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
}

// FireEntity's message dispatcher vtable.
const MessageHandler FireEntity::messageHandlers[] = {
	h_Fire_PrepareNetCode,
	h_Fire_FreeAt,
	h_Fire_Ignite,
	h_Fire_Extinguish,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

// FireEntity's constructor.
FireEntity::FireEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_IgnitableComponent(*this, true, c_ThinkingComponent)
{}

#undef myoffsetof
