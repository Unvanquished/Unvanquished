// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

/*
 * This file contains:
 *   - Declaration of the base entity.
 *   - Implementations of the base components.
 *   - Helpers to access entities and components.
 */

#ifndef CBSE_BACKEND_H_
#define CBSE_BACKEND_H_

#include <set>

#define CBSE_INCLUDE_TYPES_ONLY
#include "../CBSE.h"
#undef CBSE_INCLUDE_TYPES_ONLY

// /////////// //
// Message IDs //
// /////////// //

enum class EntityMessage {
	PrepareNetCode,
	FreeAt,
	Ignite,
	Extinguish,
	Heal,
	Damage,
	Die,
	FinishConstruction,
	PowerUp,
	PowerDown,
	ApplyDamageModifier,
	CheckSpawnPoint,
};

// //////////////////// //
// Forward declarations //
// //////////////////// //

class Entity;

class DeferredFreeingComponent;
class ThinkingComponent;
class TeamComponent;
class IgnitableComponent;
class HealthComponent;
class ClientComponent;
class ArmorComponent;
class KnockbackComponent;
class SpectatorComponent;
class AlienClassComponent;
class HumanClassComponent;
class BuildableComponent;
class AlienBuildableComponent;
class HumanBuildableComponent;
class MainBuildableComponent;
class SpawnerComponent;
class TurretComponent;
class MiningComponent;
class AcidTubeComponent;
class BarricadeComponent;
class BoosterComponent;
class EggComponent;
class HiveComponent;
class LeechComponent;
class OvermindComponent;
class SpikerComponent;
class TrapperComponent;
class ArmouryComponent;
class DrillComponent;
class MedipadComponent;
class MGTurretComponent;
class ReactorComponent;
class RocketpodComponent;
class TelenodeComponent;

/** Message handler declaration. */
using MessageHandler = void (*)(Entity*, const void* /*_data*/);

// //////////////////// //
// Component priorities //
// //////////////////// //

namespace detail {
	template<typename T> struct ComponentPriority;

	template<> struct ComponentPriority<DeferredFreeingComponent> {
		static const int value = 0;
	};

	template<> struct ComponentPriority<ThinkingComponent> {
		static const int value = 1;
	};

	template<> struct ComponentPriority<TeamComponent> {
		static const int value = 2;
	};

	template<> struct ComponentPriority<IgnitableComponent> {
		static const int value = 3;
	};

	template<> struct ComponentPriority<HealthComponent> {
		static const int value = 4;
	};

	template<> struct ComponentPriority<ClientComponent> {
		static const int value = 5;
	};

	template<> struct ComponentPriority<ArmorComponent> {
		static const int value = 6;
	};

	template<> struct ComponentPriority<KnockbackComponent> {
		static const int value = 7;
	};

	template<> struct ComponentPriority<SpectatorComponent> {
		static const int value = 8;
	};

	template<> struct ComponentPriority<AlienClassComponent> {
		static const int value = 9;
	};

	template<> struct ComponentPriority<HumanClassComponent> {
		static const int value = 10;
	};

	template<> struct ComponentPriority<BuildableComponent> {
		static const int value = 11;
	};

	template<> struct ComponentPriority<AlienBuildableComponent> {
		static const int value = 12;
	};

	template<> struct ComponentPriority<HumanBuildableComponent> {
		static const int value = 13;
	};

	template<> struct ComponentPriority<MainBuildableComponent> {
		static const int value = 14;
	};

	template<> struct ComponentPriority<SpawnerComponent> {
		static const int value = 15;
	};

	template<> struct ComponentPriority<TurretComponent> {
		static const int value = 16;
	};

	template<> struct ComponentPriority<MiningComponent> {
		static const int value = 17;
	};

	template<> struct ComponentPriority<AcidTubeComponent> {
		static const int value = 18;
	};

	template<> struct ComponentPriority<BarricadeComponent> {
		static const int value = 19;
	};

	template<> struct ComponentPriority<BoosterComponent> {
		static const int value = 20;
	};

	template<> struct ComponentPriority<EggComponent> {
		static const int value = 21;
	};

	template<> struct ComponentPriority<HiveComponent> {
		static const int value = 22;
	};

	template<> struct ComponentPriority<LeechComponent> {
		static const int value = 23;
	};

	template<> struct ComponentPriority<OvermindComponent> {
		static const int value = 24;
	};

	template<> struct ComponentPriority<SpikerComponent> {
		static const int value = 25;
	};

	template<> struct ComponentPriority<TrapperComponent> {
		static const int value = 26;
	};

	template<> struct ComponentPriority<ArmouryComponent> {
		static const int value = 27;
	};

	template<> struct ComponentPriority<DrillComponent> {
		static const int value = 28;
	};

	template<> struct ComponentPriority<MedipadComponent> {
		static const int value = 29;
	};

	template<> struct ComponentPriority<MGTurretComponent> {
		static const int value = 30;
	};

	template<> struct ComponentPriority<ReactorComponent> {
		static const int value = 31;
	};

	template<> struct ComponentPriority<RocketpodComponent> {
		static const int value = 32;
	};

	template<> struct ComponentPriority<TelenodeComponent> {
		static const int value = 33;
	};
};

// ////////////////////////////// //
// Declaration of the base Entity //
// ////////////////////////////// //

/** Base entity class. */
struct gentity_t;
class Entity {
	public:
		/**
		 * @brief Base entity constructor.
		 * @param messageHandlers Message handler vtable.
		 * @param componentOffsets Component offset vtable.
		 */
		Entity(const MessageHandler* messageHandlers, const int* componentOffsets, gentity_t* oldEnt);

		/**
		 * @brief Base entity deconstructor.
		 */
		virtual ~Entity() = default;

		// /////////////// //
		// Message helpers //
		// /////////////// //

		bool PrepareNetCode(); /**< Sends the PrepareNetCode message to all interested components. */
		bool FreeAt(int freeTime); /**< Sends the FreeAt message to all interested components. */
		bool Ignite(gentity_t* fireStarter); /**< Sends the Ignite message to all interested components. */
		bool Extinguish(int immunityTime); /**< Sends the Extinguish message to all interested components. */
		bool Heal(float amount, gentity_t* source); /**< Sends the Heal message to all interested components. */
		bool Damage(float amount, gentity_t* source, Util::optional<glm::vec3> location, Util::optional<glm::vec3> direction, int flags, meansOfDeath_t meansOfDeath); /**< Sends the Damage message to all interested components. */
		bool Die(gentity_t* killer, meansOfDeath_t meansOfDeath); /**< Sends the Die message to all interested components. */
		bool FinishConstruction(); /**< Sends the FinishConstruction message to all interested components. */
		bool PowerUp(); /**< Sends the PowerUp message to all interested components. */
		bool PowerDown(); /**< Sends the PowerDown message to all interested components. */
		bool ApplyDamageModifier(float& damage, Util::optional<glm::vec3> location, Util::optional<glm::vec3> direction, int flags, meansOfDeath_t meansOfDeath); /**< Sends the ApplyDamageModifier message to all interested components. */
		bool CheckSpawnPoint(Entity*& blocker, glm::vec3& spawnPoint); /**< Sends the CheckSpawnPoint message to all interested components. */

		/**
		 * @brief Returns a component of this entity, if available.
		 * @tparam T Type of component to ask for.
		 * @return Pointer to component of type T or nullptr.
		 */
		template<typename T> const T* Get() const {
			int index = detail::ComponentPriority<T>::value;
			int offset = componentOffsets[index];
			if (offset) {
				return (const T*) (((char*) this) + offset);
			} else {
				return nullptr;
			}
		}

		/**
		 * @brief Returns a component of this entity, if available.
		 * @tparam T Type of component to ask for.
		 * @return Pointer to component of type T or nullptr.
		 */
		template<typename T> T* Get() {
			return const_cast<T*>(static_cast<const Entity*>(this)->Get<T>());
		}

	private:
		/** Message handler vtable. */
		const MessageHandler* messageHandlers;

		/** Component offset vtable. */
		const int* componentOffsets;

		/**
		 * @brief Generic message dispatcher.
		 * @note Should not be called directly, use message helpers instead.
		 */
		bool SendMessage(EntityMessage msg, const void* data);

    public:
		// ///////////////////// //
		// Shared public members //
		// ///////////////////// //

		gentity_t* oldEnt;
};

// ////////////////////////// //
// Base component definitions //
// ////////////////////////// //

template<typename C>
class AllComponents {
	public:
		AllComponents(std::set<C*>& all): all(all) {}

		typename std::set<C*>::iterator begin() {
			return all.begin();
		}

		typename std::set<C*>::iterator end() {
			return all.end();
		}

	private:
		std::set<C*>& all;
};

/** Base class of DeferredFreeingComponent. */
class DeferredFreeingComponentBase {
	public:
		/**
		 * @brief DeferredFreeingComponentBase constructor.
		 * @param entity The entity that owns this component.
		 */
		DeferredFreeingComponentBase(Entity& entity)
			: entity(entity){
			allSet.insert(reinterpret_cast<DeferredFreeingComponent*>(this));
		}

		~DeferredFreeingComponentBase() {
			allSet.erase(reinterpret_cast<DeferredFreeingComponent*>(this));
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<DeferredFreeingComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:

		static std::set<DeferredFreeingComponent*> allSet;
};

/** Base class of ThinkingComponent. */
class ThinkingComponentBase {
	public:
		/**
		 * @brief ThinkingComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_DeferredFreeingComponent A DeferredFreeingComponent instance that this component depends on.
		 */
		ThinkingComponentBase(Entity& entity, DeferredFreeingComponent& r_DeferredFreeingComponent)
			: entity(entity), r_DeferredFreeingComponent(r_DeferredFreeingComponent){
			allSet.insert(reinterpret_cast<ThinkingComponent*>(this));
		}

		~ThinkingComponentBase() {
			allSet.erase(reinterpret_cast<ThinkingComponent*>(this));
		}

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<ThinkingComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		DeferredFreeingComponent& r_DeferredFreeingComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<ThinkingComponent*> allSet;
};

/** Base class of TeamComponent. */
class TeamComponentBase {
	public:
		/**
		 * @brief TeamComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param team An initialization parameter.
		 */
		TeamComponentBase(Entity& entity, team_t team)
			: entity(entity), team(team){
			allSet.insert(reinterpret_cast<TeamComponent*>(this));
		}

		~TeamComponentBase() {
			allSet.erase(reinterpret_cast<TeamComponent*>(this));
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<TeamComponent> GetAll() {
			return {allSet};
		}

	protected:
		team_t team; /**< An initialization parameter. */

	private:

		static std::set<TeamComponent*> allSet;
};

/** Base class of IgnitableComponent. */
class IgnitableComponentBase {
	public:
		/**
		 * @brief IgnitableComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param alwaysOnFire An initialization parameter.
		 * @param r_ThinkingComponent A ThinkingComponent instance that this component depends on.
		 */
		IgnitableComponentBase(Entity& entity, bool alwaysOnFire, ThinkingComponent& r_ThinkingComponent)
			: entity(entity), alwaysOnFire(alwaysOnFire), r_ThinkingComponent(r_ThinkingComponent){
			allSet.insert(reinterpret_cast<IgnitableComponent*>(this));
		}

		~IgnitableComponentBase() {
			allSet.erase(reinterpret_cast<IgnitableComponent*>(this));
		}

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<IgnitableComponent> GetAll() {
			return {allSet};
		}

	protected:
		bool alwaysOnFire; /**< An initialization parameter. */

	private:
		ThinkingComponent& r_ThinkingComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<IgnitableComponent*> allSet;
};

/** Base class of HealthComponent. */
class HealthComponentBase {
	public:
		/**
		 * @brief HealthComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param maxHealth An initialization parameter.
		 */
		HealthComponentBase(Entity& entity, float maxHealth)
			: entity(entity), maxHealth(maxHealth){
			allSet.insert(reinterpret_cast<HealthComponent*>(this));
		}

		~HealthComponentBase() {
			allSet.erase(reinterpret_cast<HealthComponent*>(this));
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<HealthComponent> GetAll() {
			return {allSet};
		}

	protected:
		float maxHealth; /**< An initialization parameter. */

	private:

		static std::set<HealthComponent*> allSet;
};

/** Base class of ClientComponent. */
class ClientComponentBase {
	public:
		/**
		 * @brief ClientComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param clientData An initialization parameter.
		 * @param r_TeamComponent A TeamComponent instance that this component depends on.
		 */
		ClientComponentBase(Entity& entity, gclient_t* clientData, TeamComponent& r_TeamComponent)
			: entity(entity), clientData(clientData), r_TeamComponent(r_TeamComponent){
			allSet.insert(reinterpret_cast<ClientComponent*>(this));
		}

		~ClientComponentBase() {
			allSet.erase(reinterpret_cast<ClientComponent*>(this));
		}

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<ClientComponent> GetAll() {
			return {allSet};
		}

	protected:
		gclient_t* clientData; /**< An initialization parameter. */

	private:
		TeamComponent& r_TeamComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<ClientComponent*> allSet;
};

/** Base class of ArmorComponent. */
class ArmorComponentBase {
	public:
		/**
		 * @brief ArmorComponentBase constructor.
		 * @param entity The entity that owns this component.
		 */
		ArmorComponentBase(Entity& entity)
			: entity(entity){
			allSet.insert(reinterpret_cast<ArmorComponent*>(this));
		}

		~ArmorComponentBase() {
			allSet.erase(reinterpret_cast<ArmorComponent*>(this));
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<ArmorComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:

		static std::set<ArmorComponent*> allSet;
};

/** Base class of KnockbackComponent. */
class KnockbackComponentBase {
	public:
		/**
		 * @brief KnockbackComponentBase constructor.
		 * @param entity The entity that owns this component.
		 */
		KnockbackComponentBase(Entity& entity)
			: entity(entity){
			allSet.insert(reinterpret_cast<KnockbackComponent*>(this));
		}

		~KnockbackComponentBase() {
			allSet.erase(reinterpret_cast<KnockbackComponent*>(this));
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<KnockbackComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:

		static std::set<KnockbackComponent*> allSet;
};

/** Base class of SpectatorComponent. */
class SpectatorComponentBase {
	public:
		/**
		 * @brief SpectatorComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_ClientComponent A ClientComponent instance that this component depends on.
		 */
		SpectatorComponentBase(Entity& entity, ClientComponent& r_ClientComponent)
			: entity(entity), r_ClientComponent(r_ClientComponent){
			allSet.insert(reinterpret_cast<SpectatorComponent*>(this));
		}

		~SpectatorComponentBase() {
			allSet.erase(reinterpret_cast<SpectatorComponent*>(this));
		}

		/**
		 * @return A reference to the ClientComponent of the owning entity.
		 */
		ClientComponent& GetClientComponent();
		const ClientComponent& GetClientComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<SpectatorComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		ClientComponent& r_ClientComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<SpectatorComponent*> allSet;
};

/** Base class of AlienClassComponent. */
class AlienClassComponentBase {
	public:
		/**
		 * @brief AlienClassComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_ClientComponent A ClientComponent instance that this component depends on.
		 * @param r_TeamComponent A TeamComponent instance that this component depends on.
		 * @param r_ArmorComponent A ArmorComponent instance that this component depends on.
		 * @param r_KnockbackComponent A KnockbackComponent instance that this component depends on.
		 * @param r_HealthComponent A HealthComponent instance that this component depends on.
		 */
		AlienClassComponentBase(Entity& entity, ClientComponent& r_ClientComponent, TeamComponent& r_TeamComponent, ArmorComponent& r_ArmorComponent, KnockbackComponent& r_KnockbackComponent, HealthComponent& r_HealthComponent)
			: entity(entity), r_ClientComponent(r_ClientComponent), r_TeamComponent(r_TeamComponent), r_ArmorComponent(r_ArmorComponent), r_KnockbackComponent(r_KnockbackComponent), r_HealthComponent(r_HealthComponent){
			allSet.insert(reinterpret_cast<AlienClassComponent*>(this));
		}

		~AlienClassComponentBase() {
			allSet.erase(reinterpret_cast<AlienClassComponent*>(this));
		}

		/**
		 * @return A reference to the ClientComponent of the owning entity.
		 */
		ClientComponent& GetClientComponent();
		const ClientComponent& GetClientComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the ArmorComponent of the owning entity.
		 */
		ArmorComponent& GetArmorComponent();
		const ArmorComponent& GetArmorComponent() const;

		/**
		 * @return A reference to the KnockbackComponent of the owning entity.
		 */
		KnockbackComponent& GetKnockbackComponent();
		const KnockbackComponent& GetKnockbackComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<AlienClassComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		ClientComponent& r_ClientComponent; /**< A component of the owning entity that this component depends on. */
		TeamComponent& r_TeamComponent; /**< A component of the owning entity that this component depends on. */
		ArmorComponent& r_ArmorComponent; /**< A component of the owning entity that this component depends on. */
		KnockbackComponent& r_KnockbackComponent; /**< A component of the owning entity that this component depends on. */
		HealthComponent& r_HealthComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<AlienClassComponent*> allSet;
};

/** Base class of HumanClassComponent. */
class HumanClassComponentBase {
	public:
		/**
		 * @brief HumanClassComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_ClientComponent A ClientComponent instance that this component depends on.
		 * @param r_TeamComponent A TeamComponent instance that this component depends on.
		 * @param r_ArmorComponent A ArmorComponent instance that this component depends on.
		 * @param r_KnockbackComponent A KnockbackComponent instance that this component depends on.
		 * @param r_HealthComponent A HealthComponent instance that this component depends on.
		 */
		HumanClassComponentBase(Entity& entity, ClientComponent& r_ClientComponent, TeamComponent& r_TeamComponent, ArmorComponent& r_ArmorComponent, KnockbackComponent& r_KnockbackComponent, HealthComponent& r_HealthComponent)
			: entity(entity), r_ClientComponent(r_ClientComponent), r_TeamComponent(r_TeamComponent), r_ArmorComponent(r_ArmorComponent), r_KnockbackComponent(r_KnockbackComponent), r_HealthComponent(r_HealthComponent){
			allSet.insert(reinterpret_cast<HumanClassComponent*>(this));
		}

		~HumanClassComponentBase() {
			allSet.erase(reinterpret_cast<HumanClassComponent*>(this));
		}

		/**
		 * @return A reference to the ClientComponent of the owning entity.
		 */
		ClientComponent& GetClientComponent();
		const ClientComponent& GetClientComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the ArmorComponent of the owning entity.
		 */
		ArmorComponent& GetArmorComponent();
		const ArmorComponent& GetArmorComponent() const;

		/**
		 * @return A reference to the KnockbackComponent of the owning entity.
		 */
		KnockbackComponent& GetKnockbackComponent();
		const KnockbackComponent& GetKnockbackComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<HumanClassComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		ClientComponent& r_ClientComponent; /**< A component of the owning entity that this component depends on. */
		TeamComponent& r_TeamComponent; /**< A component of the owning entity that this component depends on. */
		ArmorComponent& r_ArmorComponent; /**< A component of the owning entity that this component depends on. */
		KnockbackComponent& r_KnockbackComponent; /**< A component of the owning entity that this component depends on. */
		HealthComponent& r_HealthComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<HumanClassComponent*> allSet;
};

/** Base class of BuildableComponent. */
class BuildableComponentBase {
	public:
		/**
		 * @brief BuildableComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_HealthComponent A HealthComponent instance that this component depends on.
		 * @param r_ThinkingComponent A ThinkingComponent instance that this component depends on.
		 * @param r_TeamComponent A TeamComponent instance that this component depends on.
		 */
		BuildableComponentBase(Entity& entity, HealthComponent& r_HealthComponent, ThinkingComponent& r_ThinkingComponent, TeamComponent& r_TeamComponent)
			: entity(entity), r_HealthComponent(r_HealthComponent), r_ThinkingComponent(r_ThinkingComponent), r_TeamComponent(r_TeamComponent){
			allSet.insert(reinterpret_cast<BuildableComponent*>(this));
		}

		~BuildableComponentBase() {
			allSet.erase(reinterpret_cast<BuildableComponent*>(this));
		}

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<BuildableComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HealthComponent& r_HealthComponent; /**< A component of the owning entity that this component depends on. */
		ThinkingComponent& r_ThinkingComponent; /**< A component of the owning entity that this component depends on. */
		TeamComponent& r_TeamComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<BuildableComponent*> allSet;
};

/** Base class of AlienBuildableComponent. */
class AlienBuildableComponentBase {
	public:
		/**
		 * @brief AlienBuildableComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_BuildableComponent A BuildableComponent instance that this component depends on.
		 * @param r_TeamComponent A TeamComponent instance that this component depends on.
		 * @param r_IgnitableComponent A IgnitableComponent instance that this component depends on.
		 */
		AlienBuildableComponentBase(Entity& entity, BuildableComponent& r_BuildableComponent, TeamComponent& r_TeamComponent, IgnitableComponent& r_IgnitableComponent)
			: entity(entity), r_BuildableComponent(r_BuildableComponent), r_TeamComponent(r_TeamComponent), r_IgnitableComponent(r_IgnitableComponent){
			allSet.insert(reinterpret_cast<AlienBuildableComponent*>(this));
		}

		~AlienBuildableComponentBase() {
			allSet.erase(reinterpret_cast<AlienBuildableComponent*>(this));
		}

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the IgnitableComponent of the owning entity.
		 */
		IgnitableComponent& GetIgnitableComponent();
		const IgnitableComponent& GetIgnitableComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<AlienBuildableComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		BuildableComponent& r_BuildableComponent; /**< A component of the owning entity that this component depends on. */
		TeamComponent& r_TeamComponent; /**< A component of the owning entity that this component depends on. */
		IgnitableComponent& r_IgnitableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<AlienBuildableComponent*> allSet;
};

/** Base class of HumanBuildableComponent. */
class HumanBuildableComponentBase {
	public:
		/**
		 * @brief HumanBuildableComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_BuildableComponent A BuildableComponent instance that this component depends on.
		 * @param r_TeamComponent A TeamComponent instance that this component depends on.
		 */
		HumanBuildableComponentBase(Entity& entity, BuildableComponent& r_BuildableComponent, TeamComponent& r_TeamComponent)
			: entity(entity), r_BuildableComponent(r_BuildableComponent), r_TeamComponent(r_TeamComponent){
			allSet.insert(reinterpret_cast<HumanBuildableComponent*>(this));
		}

		~HumanBuildableComponentBase() {
			allSet.erase(reinterpret_cast<HumanBuildableComponent*>(this));
		}

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<HumanBuildableComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		BuildableComponent& r_BuildableComponent; /**< A component of the owning entity that this component depends on. */
		TeamComponent& r_TeamComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<HumanBuildableComponent*> allSet;
};

/** Base class of MainBuildableComponent. */
class MainBuildableComponentBase {
	public:
		/**
		 * @brief MainBuildableComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_BuildableComponent A BuildableComponent instance that this component depends on.
		 */
		MainBuildableComponentBase(Entity& entity, BuildableComponent& r_BuildableComponent)
			: entity(entity), r_BuildableComponent(r_BuildableComponent){
			allSet.insert(reinterpret_cast<MainBuildableComponent*>(this));
		}

		~MainBuildableComponentBase() {
			allSet.erase(reinterpret_cast<MainBuildableComponent*>(this));
		}

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<MainBuildableComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		BuildableComponent& r_BuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<MainBuildableComponent*> allSet;
};

/** Base class of SpawnerComponent. */
class SpawnerComponentBase {
	public:
		/**
		 * @brief SpawnerComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_TeamComponent A TeamComponent instance that this component depends on.
		 * @param r_ThinkingComponent A ThinkingComponent instance that this component depends on.
		 */
		SpawnerComponentBase(Entity& entity, TeamComponent& r_TeamComponent, ThinkingComponent& r_ThinkingComponent)
			: entity(entity), r_TeamComponent(r_TeamComponent), r_ThinkingComponent(r_ThinkingComponent){
			allSet.insert(reinterpret_cast<SpawnerComponent*>(this));
		}

		~SpawnerComponentBase() {
			allSet.erase(reinterpret_cast<SpawnerComponent*>(this));
		}

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<SpawnerComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		TeamComponent& r_TeamComponent; /**< A component of the owning entity that this component depends on. */
		ThinkingComponent& r_ThinkingComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<SpawnerComponent*> allSet;
};

/** Base class of TurretComponent. */
class TurretComponentBase {
	public:
		/**
		 * @brief TurretComponentBase constructor.
		 * @param entity The entity that owns this component.
		 */
		TurretComponentBase(Entity& entity)
			: entity(entity){
			allSet.insert(reinterpret_cast<TurretComponent*>(this));
		}

		~TurretComponentBase() {
			allSet.erase(reinterpret_cast<TurretComponent*>(this));
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<TurretComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:

		static std::set<TurretComponent*> allSet;
};

/** Base class of MiningComponent. */
class MiningComponentBase {
	public:
		/**
		 * @brief MiningComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_ThinkingComponent A ThinkingComponent instance that this component depends on.
		 */
		MiningComponentBase(Entity& entity, ThinkingComponent& r_ThinkingComponent)
			: entity(entity), r_ThinkingComponent(r_ThinkingComponent){
			allSet.insert(reinterpret_cast<MiningComponent*>(this));
		}

		~MiningComponentBase() {
			allSet.erase(reinterpret_cast<MiningComponent*>(this));
		}

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<MiningComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		ThinkingComponent& r_ThinkingComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<MiningComponent*> allSet;
};

/** Base class of AcidTubeComponent. */
class AcidTubeComponentBase {
	public:
		/**
		 * @brief AcidTubeComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_AlienBuildableComponent A AlienBuildableComponent instance that this component depends on.
		 */
		AcidTubeComponentBase(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
			: entity(entity), r_AlienBuildableComponent(r_AlienBuildableComponent){
			allSet.insert(reinterpret_cast<AcidTubeComponent*>(this));
		}

		~AcidTubeComponentBase() {
			allSet.erase(reinterpret_cast<AcidTubeComponent*>(this));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent();
		const AlienBuildableComponent& GetAlienBuildableComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the IgnitableComponent of the owning entity.
		 */
		IgnitableComponent& GetIgnitableComponent();
		const IgnitableComponent& GetIgnitableComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<AcidTubeComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		AlienBuildableComponent& r_AlienBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<AcidTubeComponent*> allSet;
};

/** Base class of BarricadeComponent. */
class BarricadeComponentBase {
	public:
		/**
		 * @brief BarricadeComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_AlienBuildableComponent A AlienBuildableComponent instance that this component depends on.
		 */
		BarricadeComponentBase(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
			: entity(entity), r_AlienBuildableComponent(r_AlienBuildableComponent){
			allSet.insert(reinterpret_cast<BarricadeComponent*>(this));
		}

		~BarricadeComponentBase() {
			allSet.erase(reinterpret_cast<BarricadeComponent*>(this));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent();
		const AlienBuildableComponent& GetAlienBuildableComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the IgnitableComponent of the owning entity.
		 */
		IgnitableComponent& GetIgnitableComponent();
		const IgnitableComponent& GetIgnitableComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<BarricadeComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		AlienBuildableComponent& r_AlienBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<BarricadeComponent*> allSet;
};

/** Base class of BoosterComponent. */
class BoosterComponentBase {
	public:
		/**
		 * @brief BoosterComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_AlienBuildableComponent A AlienBuildableComponent instance that this component depends on.
		 */
		BoosterComponentBase(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
			: entity(entity), r_AlienBuildableComponent(r_AlienBuildableComponent){
			allSet.insert(reinterpret_cast<BoosterComponent*>(this));
		}

		~BoosterComponentBase() {
			allSet.erase(reinterpret_cast<BoosterComponent*>(this));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent();
		const AlienBuildableComponent& GetAlienBuildableComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the IgnitableComponent of the owning entity.
		 */
		IgnitableComponent& GetIgnitableComponent();
		const IgnitableComponent& GetIgnitableComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<BoosterComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		AlienBuildableComponent& r_AlienBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<BoosterComponent*> allSet;
};

/** Base class of EggComponent. */
class EggComponentBase {
	public:
		/**
		 * @brief EggComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_AlienBuildableComponent A AlienBuildableComponent instance that this component depends on.
		 * @param r_SpawnerComponent A SpawnerComponent instance that this component depends on.
		 */
		EggComponentBase(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent, SpawnerComponent& r_SpawnerComponent)
			: entity(entity), r_AlienBuildableComponent(r_AlienBuildableComponent), r_SpawnerComponent(r_SpawnerComponent){
			allSet.insert(reinterpret_cast<EggComponent*>(this));
		}

		~EggComponentBase() {
			allSet.erase(reinterpret_cast<EggComponent*>(this));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent();
		const AlienBuildableComponent& GetAlienBuildableComponent() const;

		/**
		 * @return A reference to the SpawnerComponent of the owning entity.
		 */
		SpawnerComponent& GetSpawnerComponent();
		const SpawnerComponent& GetSpawnerComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the IgnitableComponent of the owning entity.
		 */
		IgnitableComponent& GetIgnitableComponent();
		const IgnitableComponent& GetIgnitableComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<EggComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		AlienBuildableComponent& r_AlienBuildableComponent; /**< A component of the owning entity that this component depends on. */
		SpawnerComponent& r_SpawnerComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<EggComponent*> allSet;
};

/** Base class of HiveComponent. */
class HiveComponentBase {
	public:
		/**
		 * @brief HiveComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_AlienBuildableComponent A AlienBuildableComponent instance that this component depends on.
		 */
		HiveComponentBase(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
			: entity(entity), r_AlienBuildableComponent(r_AlienBuildableComponent){
			allSet.insert(reinterpret_cast<HiveComponent*>(this));
		}

		~HiveComponentBase() {
			allSet.erase(reinterpret_cast<HiveComponent*>(this));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent();
		const AlienBuildableComponent& GetAlienBuildableComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the IgnitableComponent of the owning entity.
		 */
		IgnitableComponent& GetIgnitableComponent();
		const IgnitableComponent& GetIgnitableComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<HiveComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		AlienBuildableComponent& r_AlienBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<HiveComponent*> allSet;
};

/** Base class of LeechComponent. */
class LeechComponentBase {
	public:
		/**
		 * @brief LeechComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_AlienBuildableComponent A AlienBuildableComponent instance that this component depends on.
		 * @param r_MiningComponent A MiningComponent instance that this component depends on.
		 */
		LeechComponentBase(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent, MiningComponent& r_MiningComponent)
			: entity(entity), r_AlienBuildableComponent(r_AlienBuildableComponent), r_MiningComponent(r_MiningComponent){
			allSet.insert(reinterpret_cast<LeechComponent*>(this));
		}

		~LeechComponentBase() {
			allSet.erase(reinterpret_cast<LeechComponent*>(this));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent();
		const AlienBuildableComponent& GetAlienBuildableComponent() const;

		/**
		 * @return A reference to the MiningComponent of the owning entity.
		 */
		MiningComponent& GetMiningComponent();
		const MiningComponent& GetMiningComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the IgnitableComponent of the owning entity.
		 */
		IgnitableComponent& GetIgnitableComponent();
		const IgnitableComponent& GetIgnitableComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<LeechComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		AlienBuildableComponent& r_AlienBuildableComponent; /**< A component of the owning entity that this component depends on. */
		MiningComponent& r_MiningComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<LeechComponent*> allSet;
};

/** Base class of OvermindComponent. */
class OvermindComponentBase {
	public:
		/**
		 * @brief OvermindComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_AlienBuildableComponent A AlienBuildableComponent instance that this component depends on.
		 * @param r_MainBuildableComponent A MainBuildableComponent instance that this component depends on.
		 */
		OvermindComponentBase(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent, MainBuildableComponent& r_MainBuildableComponent)
			: entity(entity), r_AlienBuildableComponent(r_AlienBuildableComponent), r_MainBuildableComponent(r_MainBuildableComponent){
			allSet.insert(reinterpret_cast<OvermindComponent*>(this));
		}

		~OvermindComponentBase() {
			allSet.erase(reinterpret_cast<OvermindComponent*>(this));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent();
		const AlienBuildableComponent& GetAlienBuildableComponent() const;

		/**
		 * @return A reference to the MainBuildableComponent of the owning entity.
		 */
		MainBuildableComponent& GetMainBuildableComponent();
		const MainBuildableComponent& GetMainBuildableComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the IgnitableComponent of the owning entity.
		 */
		IgnitableComponent& GetIgnitableComponent();
		const IgnitableComponent& GetIgnitableComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<OvermindComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		AlienBuildableComponent& r_AlienBuildableComponent; /**< A component of the owning entity that this component depends on. */
		MainBuildableComponent& r_MainBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<OvermindComponent*> allSet;
};

/** Base class of SpikerComponent. */
class SpikerComponentBase {
	public:
		/**
		 * @brief SpikerComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_AlienBuildableComponent A AlienBuildableComponent instance that this component depends on.
		 */
		SpikerComponentBase(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
			: entity(entity), r_AlienBuildableComponent(r_AlienBuildableComponent){
			allSet.insert(reinterpret_cast<SpikerComponent*>(this));
		}

		~SpikerComponentBase() {
			allSet.erase(reinterpret_cast<SpikerComponent*>(this));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent();
		const AlienBuildableComponent& GetAlienBuildableComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the IgnitableComponent of the owning entity.
		 */
		IgnitableComponent& GetIgnitableComponent();
		const IgnitableComponent& GetIgnitableComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<SpikerComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		AlienBuildableComponent& r_AlienBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<SpikerComponent*> allSet;
};

/** Base class of TrapperComponent. */
class TrapperComponentBase {
	public:
		/**
		 * @brief TrapperComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_AlienBuildableComponent A AlienBuildableComponent instance that this component depends on.
		 */
		TrapperComponentBase(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
			: entity(entity), r_AlienBuildableComponent(r_AlienBuildableComponent){
			allSet.insert(reinterpret_cast<TrapperComponent*>(this));
		}

		~TrapperComponentBase() {
			allSet.erase(reinterpret_cast<TrapperComponent*>(this));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent();
		const AlienBuildableComponent& GetAlienBuildableComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the IgnitableComponent of the owning entity.
		 */
		IgnitableComponent& GetIgnitableComponent();
		const IgnitableComponent& GetIgnitableComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<TrapperComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		AlienBuildableComponent& r_AlienBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<TrapperComponent*> allSet;
};

/** Base class of ArmouryComponent. */
class ArmouryComponentBase {
	public:
		/**
		 * @brief ArmouryComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_HumanBuildableComponent A HumanBuildableComponent instance that this component depends on.
		 */
		ArmouryComponentBase(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent)
			: entity(entity), r_HumanBuildableComponent(r_HumanBuildableComponent){
			allSet.insert(reinterpret_cast<ArmouryComponent*>(this));
		}

		~ArmouryComponentBase() {
			allSet.erase(reinterpret_cast<ArmouryComponent*>(this));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent();
		const HumanBuildableComponent& GetHumanBuildableComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<ArmouryComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HumanBuildableComponent& r_HumanBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<ArmouryComponent*> allSet;
};

/** Base class of DrillComponent. */
class DrillComponentBase {
	public:
		/**
		 * @brief DrillComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_HumanBuildableComponent A HumanBuildableComponent instance that this component depends on.
		 * @param r_MiningComponent A MiningComponent instance that this component depends on.
		 */
		DrillComponentBase(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent, MiningComponent& r_MiningComponent)
			: entity(entity), r_HumanBuildableComponent(r_HumanBuildableComponent), r_MiningComponent(r_MiningComponent){
			allSet.insert(reinterpret_cast<DrillComponent*>(this));
		}

		~DrillComponentBase() {
			allSet.erase(reinterpret_cast<DrillComponent*>(this));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent();
		const HumanBuildableComponent& GetHumanBuildableComponent() const;

		/**
		 * @return A reference to the MiningComponent of the owning entity.
		 */
		MiningComponent& GetMiningComponent();
		const MiningComponent& GetMiningComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<DrillComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HumanBuildableComponent& r_HumanBuildableComponent; /**< A component of the owning entity that this component depends on. */
		MiningComponent& r_MiningComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<DrillComponent*> allSet;
};

/** Base class of MedipadComponent. */
class MedipadComponentBase {
	public:
		/**
		 * @brief MedipadComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_HumanBuildableComponent A HumanBuildableComponent instance that this component depends on.
		 */
		MedipadComponentBase(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent)
			: entity(entity), r_HumanBuildableComponent(r_HumanBuildableComponent){
			allSet.insert(reinterpret_cast<MedipadComponent*>(this));
		}

		~MedipadComponentBase() {
			allSet.erase(reinterpret_cast<MedipadComponent*>(this));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent();
		const HumanBuildableComponent& GetHumanBuildableComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<MedipadComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HumanBuildableComponent& r_HumanBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<MedipadComponent*> allSet;
};

/** Base class of MGTurretComponent. */
class MGTurretComponentBase {
	public:
		/**
		 * @brief MGTurretComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_HumanBuildableComponent A HumanBuildableComponent instance that this component depends on.
		 * @param r_TurretComponent A TurretComponent instance that this component depends on.
		 */
		MGTurretComponentBase(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent, TurretComponent& r_TurretComponent)
			: entity(entity), r_HumanBuildableComponent(r_HumanBuildableComponent), r_TurretComponent(r_TurretComponent){
			allSet.insert(reinterpret_cast<MGTurretComponent*>(this));
		}

		~MGTurretComponentBase() {
			allSet.erase(reinterpret_cast<MGTurretComponent*>(this));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent();
		const HumanBuildableComponent& GetHumanBuildableComponent() const;

		/**
		 * @return A reference to the TurretComponent of the owning entity.
		 */
		TurretComponent& GetTurretComponent();
		const TurretComponent& GetTurretComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<MGTurretComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HumanBuildableComponent& r_HumanBuildableComponent; /**< A component of the owning entity that this component depends on. */
		TurretComponent& r_TurretComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<MGTurretComponent*> allSet;
};

/** Base class of ReactorComponent. */
class ReactorComponentBase {
	public:
		/**
		 * @brief ReactorComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_HumanBuildableComponent A HumanBuildableComponent instance that this component depends on.
		 * @param r_MainBuildableComponent A MainBuildableComponent instance that this component depends on.
		 */
		ReactorComponentBase(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent, MainBuildableComponent& r_MainBuildableComponent)
			: entity(entity), r_HumanBuildableComponent(r_HumanBuildableComponent), r_MainBuildableComponent(r_MainBuildableComponent){
			allSet.insert(reinterpret_cast<ReactorComponent*>(this));
		}

		~ReactorComponentBase() {
			allSet.erase(reinterpret_cast<ReactorComponent*>(this));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent();
		const HumanBuildableComponent& GetHumanBuildableComponent() const;

		/**
		 * @return A reference to the MainBuildableComponent of the owning entity.
		 */
		MainBuildableComponent& GetMainBuildableComponent();
		const MainBuildableComponent& GetMainBuildableComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<ReactorComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HumanBuildableComponent& r_HumanBuildableComponent; /**< A component of the owning entity that this component depends on. */
		MainBuildableComponent& r_MainBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<ReactorComponent*> allSet;
};

/** Base class of RocketpodComponent. */
class RocketpodComponentBase {
	public:
		/**
		 * @brief RocketpodComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_HumanBuildableComponent A HumanBuildableComponent instance that this component depends on.
		 * @param r_TurretComponent A TurretComponent instance that this component depends on.
		 */
		RocketpodComponentBase(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent, TurretComponent& r_TurretComponent)
			: entity(entity), r_HumanBuildableComponent(r_HumanBuildableComponent), r_TurretComponent(r_TurretComponent){
			allSet.insert(reinterpret_cast<RocketpodComponent*>(this));
		}

		~RocketpodComponentBase() {
			allSet.erase(reinterpret_cast<RocketpodComponent*>(this));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent();
		const HumanBuildableComponent& GetHumanBuildableComponent() const;

		/**
		 * @return A reference to the TurretComponent of the owning entity.
		 */
		TurretComponent& GetTurretComponent();
		const TurretComponent& GetTurretComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<RocketpodComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HumanBuildableComponent& r_HumanBuildableComponent; /**< A component of the owning entity that this component depends on. */
		TurretComponent& r_TurretComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<RocketpodComponent*> allSet;
};

/** Base class of TelenodeComponent. */
class TelenodeComponentBase {
	public:
		/**
		 * @brief TelenodeComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_HumanBuildableComponent A HumanBuildableComponent instance that this component depends on.
		 * @param r_SpawnerComponent A SpawnerComponent instance that this component depends on.
		 */
		TelenodeComponentBase(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent, SpawnerComponent& r_SpawnerComponent)
			: entity(entity), r_HumanBuildableComponent(r_HumanBuildableComponent), r_SpawnerComponent(r_SpawnerComponent){
			allSet.insert(reinterpret_cast<TelenodeComponent*>(this));
		}

		~TelenodeComponentBase() {
			allSet.erase(reinterpret_cast<TelenodeComponent*>(this));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent();
		const HumanBuildableComponent& GetHumanBuildableComponent() const;

		/**
		 * @return A reference to the SpawnerComponent of the owning entity.
		 */
		SpawnerComponent& GetSpawnerComponent();
		const SpawnerComponent& GetSpawnerComponent() const;

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent();
		const BuildableComponent& GetBuildableComponent() const;

		/**
		 * @return A reference to the TeamComponent of the owning entity.
		 */
		TeamComponent& GetTeamComponent();
		const TeamComponent& GetTeamComponent() const;

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent();
		const HealthComponent& GetHealthComponent() const;

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent();
		const ThinkingComponent& GetThinkingComponent() const;

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent();
		const DeferredFreeingComponent& GetDeferredFreeingComponent() const;

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<TelenodeComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HumanBuildableComponent& r_HumanBuildableComponent; /**< A component of the owning entity that this component depends on. */
		SpawnerComponent& r_SpawnerComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<TelenodeComponent*> allSet;
};


// ////////////////////////// //
// Definitions of ForEntities //
// ////////////////////////// //

template <typename Component> bool HasComponents(const Entity& ent) {
    return ent.Get<Component>() != nullptr;
}

template <typename Component1, typename Component2, typename ... Components> bool HasComponents(const Entity& ent) {
    return HasComponents<Component1>(ent) && HasComponents<Component2, Components...>(ent);
}

template <typename Component1, typename ... Components, typename FuncType>
void ForEntities(FuncType f) {
    for(auto* component1: Component1::GetAll()) {
        Entity& ent = component1->entity;

        if (HasComponents<Component1, Components...>(ent)) {
            f(ent, *component1, *ent.Get<Components>()...);
        }
    }
}

#endif // CBSE_BACKEND_H_
