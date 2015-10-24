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

enum {
	MSG_PREPARENETCODE,
	MSG_FREEAT,
	MSG_IGNITE,
	MSG_EXTINGUISH,
	MSG_HEAL,
	MSG_DAMAGE,
	MSG_DIE,
	MSG_APPLYDAMAGEMODIFIER,
};

// //////////////////// //
// Forward declarations //
// //////////////////// //

class Entity;

class DeferredFreeingComponent;
class ThinkingComponent;
class IgnitableComponent;
class HealthComponent;
class ClientComponent;
class ArmorComponent;
class KnockbackComponent;
class AlienClassComponent;
class HumanClassComponent;
class BuildableComponent;
class AlienBuildableComponent;
class HumanBuildableComponent;
class ResourceStorageComponent;
class MainBuildableComponent;
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
class RepeaterComponent;
class RocketpodComponent;
class TelenodeComponent;

/** Message handler declaration. */
typedef void (*MessageHandler)(Entity*, const void*);

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

	template<> struct ComponentPriority<IgnitableComponent> {
		static const int value = 2;
	};

	template<> struct ComponentPriority<HealthComponent> {
		static const int value = 3;
	};

	template<> struct ComponentPriority<ClientComponent> {
		static const int value = 4;
	};

	template<> struct ComponentPriority<ArmorComponent> {
		static const int value = 5;
	};

	template<> struct ComponentPriority<KnockbackComponent> {
		static const int value = 6;
	};

	template<> struct ComponentPriority<AlienClassComponent> {
		static const int value = 7;
	};

	template<> struct ComponentPriority<HumanClassComponent> {
		static const int value = 8;
	};

	template<> struct ComponentPriority<BuildableComponent> {
		static const int value = 9;
	};

	template<> struct ComponentPriority<AlienBuildableComponent> {
		static const int value = 10;
	};

	template<> struct ComponentPriority<HumanBuildableComponent> {
		static const int value = 11;
	};

	template<> struct ComponentPriority<ResourceStorageComponent> {
		static const int value = 12;
	};

	template<> struct ComponentPriority<MainBuildableComponent> {
		static const int value = 13;
	};

	template<> struct ComponentPriority<TurretComponent> {
		static const int value = 14;
	};

	template<> struct ComponentPriority<MiningComponent> {
		static const int value = 15;
	};

	template<> struct ComponentPriority<AcidTubeComponent> {
		static const int value = 16;
	};

	template<> struct ComponentPriority<BarricadeComponent> {
		static const int value = 17;
	};

	template<> struct ComponentPriority<BoosterComponent> {
		static const int value = 18;
	};

	template<> struct ComponentPriority<EggComponent> {
		static const int value = 19;
	};

	template<> struct ComponentPriority<HiveComponent> {
		static const int value = 20;
	};

	template<> struct ComponentPriority<LeechComponent> {
		static const int value = 21;
	};

	template<> struct ComponentPriority<OvermindComponent> {
		static const int value = 22;
	};

	template<> struct ComponentPriority<SpikerComponent> {
		static const int value = 23;
	};

	template<> struct ComponentPriority<TrapperComponent> {
		static const int value = 24;
	};

	template<> struct ComponentPriority<ArmouryComponent> {
		static const int value = 25;
	};

	template<> struct ComponentPriority<DrillComponent> {
		static const int value = 26;
	};

	template<> struct ComponentPriority<MedipadComponent> {
		static const int value = 27;
	};

	template<> struct ComponentPriority<MGTurretComponent> {
		static const int value = 28;
	};

	template<> struct ComponentPriority<ReactorComponent> {
		static const int value = 29;
	};

	template<> struct ComponentPriority<RepeaterComponent> {
		static const int value = 30;
	};

	template<> struct ComponentPriority<RocketpodComponent> {
		static const int value = 31;
	};

	template<> struct ComponentPriority<TelenodeComponent> {
		static const int value = 32;
	};
};

// ////////////////////////////// //
// Declaration of the base Entity //
// ////////////////////////////// //

/** Base entity class. */
class Entity {
	public:
		/**
		 * @brief Base entity constructor.
		 * @param messageHandlers Message handler vtable.
		 * @param componentOffsets Component offset vtable.
		 */
		Entity(const MessageHandler* messageHandlers, const int* componentOffsets, struct gentity_s* oldEnt);

		/**
		 * @brief Base entity deconstructor.
		 */
		virtual ~Entity();

		// /////////////// //
		// Message helpers //
		// /////////////// //

		bool PrepareNetCode(); /**< Sends the PrepareNetCode message to all interested components. */
		bool FreeAt(int freeTime); /**< Sends the FreeAt message to all interested components. */
		bool Ignite(gentity_t* fireStarter); /**< Sends the Ignite message to all interested components. */
		bool Extinguish(int immunityTime); /**< Sends the Extinguish message to all interested components. */
		bool Heal(float amount, gentity_t* source); /**< Sends the Heal message to all interested components. */
		bool Damage(float amount, gentity_t* source, Util::optional<Vec3> location, Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath); /**< Sends the Damage message to all interested components. */
		bool Die(gentity_t* killer, meansOfDeath_t meansOfDeath); /**< Sends the Die message to all interested components. */
		bool ApplyDamageModifier(float& damage, Util::optional<Vec3> location, Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath); /**< Sends the ApplyDamageModifier message to all interested components. */

		/**
		 * @brief Returns a component of this entity, if available.
		 * @tparam T Type of component to ask for.
		 * @return Pointer to component of type T or nullptr.
		 */
		template<typename T> T* Get() {
			int index = detail::ComponentPriority<T>::value;
			int offset = componentOffsets[index];
			if (offset) {
				return (T*) (((char*) this) + offset);
			} else {
				return nullptr;
			}
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
		bool SendMessage(int msg, const void* data);

    public:
		// ///////////////////// //
		// Shared public members //
		// ///////////////////// //

		struct gentity_s* oldEnt;
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
			allSet.insert((DeferredFreeingComponent*)((char*) this - (char*) (DeferredFreeingComponentBase*) (DeferredFreeingComponent*) nullptr));
		}

		~DeferredFreeingComponentBase() {
			allSet.erase((DeferredFreeingComponent*)((char*) this - (char*) (DeferredFreeingComponentBase*) (DeferredFreeingComponent*) nullptr));
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
			allSet.insert((ThinkingComponent*)((char*) this - (char*) (ThinkingComponentBase*) (ThinkingComponent*) nullptr));
		}

		~ThinkingComponentBase() {
			allSet.erase((ThinkingComponent*)((char*) this - (char*) (ThinkingComponentBase*) (ThinkingComponent*) nullptr));
		}

		/**
		 * @return A reference to the DeferredFreeingComponent of the owning entity.
		 */
		DeferredFreeingComponent& GetDeferredFreeingComponent() {
			return r_DeferredFreeingComponent;
		}

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
			allSet.insert((IgnitableComponent*)((char*) this - (char*) (IgnitableComponentBase*) (IgnitableComponent*) nullptr));
		}

		~IgnitableComponentBase() {
			allSet.erase((IgnitableComponent*)((char*) this - (char*) (IgnitableComponentBase*) (IgnitableComponent*) nullptr));
		}

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent() {
			return r_ThinkingComponent;
		}

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
			allSet.insert((HealthComponent*)((char*) this - (char*) (HealthComponentBase*) (HealthComponent*) nullptr));
		}

		~HealthComponentBase() {
			allSet.erase((HealthComponent*)((char*) this - (char*) (HealthComponentBase*) (HealthComponent*) nullptr));
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
		 */
		ClientComponentBase(Entity& entity, gclient_t* clientData)
			: entity(entity), clientData(clientData){
			allSet.insert((ClientComponent*)((char*) this - (char*) (ClientComponentBase*) (ClientComponent*) nullptr));
		}

		~ClientComponentBase() {
			allSet.erase((ClientComponent*)((char*) this - (char*) (ClientComponentBase*) (ClientComponent*) nullptr));
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<ClientComponent> GetAll() {
			return {allSet};
		}

	protected:
		gclient_t* clientData; /**< An initialization parameter. */

	private:

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
			allSet.insert((ArmorComponent*)((char*) this - (char*) (ArmorComponentBase*) (ArmorComponent*) nullptr));
		}

		~ArmorComponentBase() {
			allSet.erase((ArmorComponent*)((char*) this - (char*) (ArmorComponentBase*) (ArmorComponent*) nullptr));
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
			allSet.insert((KnockbackComponent*)((char*) this - (char*) (KnockbackComponentBase*) (KnockbackComponent*) nullptr));
		}

		~KnockbackComponentBase() {
			allSet.erase((KnockbackComponent*)((char*) this - (char*) (KnockbackComponentBase*) (KnockbackComponent*) nullptr));
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

/** Base class of AlienClassComponent. */
class AlienClassComponentBase {
	public:
		/**
		 * @brief AlienClassComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_ClientComponent A ClientComponent instance that this component depends on.
		 * @param r_ArmorComponent A ArmorComponent instance that this component depends on.
		 * @param r_KnockbackComponent A KnockbackComponent instance that this component depends on.
		 * @param r_HealthComponent A HealthComponent instance that this component depends on.
		 */
		AlienClassComponentBase(Entity& entity, ClientComponent& r_ClientComponent, ArmorComponent& r_ArmorComponent, KnockbackComponent& r_KnockbackComponent, HealthComponent& r_HealthComponent)
			: entity(entity), r_ClientComponent(r_ClientComponent), r_ArmorComponent(r_ArmorComponent), r_KnockbackComponent(r_KnockbackComponent), r_HealthComponent(r_HealthComponent){
			allSet.insert((AlienClassComponent*)((char*) this - (char*) (AlienClassComponentBase*) (AlienClassComponent*) nullptr));
		}

		~AlienClassComponentBase() {
			allSet.erase((AlienClassComponent*)((char*) this - (char*) (AlienClassComponentBase*) (AlienClassComponent*) nullptr));
		}

		/**
		 * @return A reference to the ClientComponent of the owning entity.
		 */
		ClientComponent& GetClientComponent() {
			return r_ClientComponent;
		}

		/**
		 * @return A reference to the ArmorComponent of the owning entity.
		 */
		ArmorComponent& GetArmorComponent() {
			return r_ArmorComponent;
		}

		/**
		 * @return A reference to the KnockbackComponent of the owning entity.
		 */
		KnockbackComponent& GetKnockbackComponent() {
			return r_KnockbackComponent;
		}

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent() {
			return r_HealthComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<AlienClassComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		ClientComponent& r_ClientComponent; /**< A component of the owning entity that this component depends on. */
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
		 * @param r_ArmorComponent A ArmorComponent instance that this component depends on.
		 * @param r_KnockbackComponent A KnockbackComponent instance that this component depends on.
		 * @param r_HealthComponent A HealthComponent instance that this component depends on.
		 */
		HumanClassComponentBase(Entity& entity, ClientComponent& r_ClientComponent, ArmorComponent& r_ArmorComponent, KnockbackComponent& r_KnockbackComponent, HealthComponent& r_HealthComponent)
			: entity(entity), r_ClientComponent(r_ClientComponent), r_ArmorComponent(r_ArmorComponent), r_KnockbackComponent(r_KnockbackComponent), r_HealthComponent(r_HealthComponent){
			allSet.insert((HumanClassComponent*)((char*) this - (char*) (HumanClassComponentBase*) (HumanClassComponent*) nullptr));
		}

		~HumanClassComponentBase() {
			allSet.erase((HumanClassComponent*)((char*) this - (char*) (HumanClassComponentBase*) (HumanClassComponent*) nullptr));
		}

		/**
		 * @return A reference to the ClientComponent of the owning entity.
		 */
		ClientComponent& GetClientComponent() {
			return r_ClientComponent;
		}

		/**
		 * @return A reference to the ArmorComponent of the owning entity.
		 */
		ArmorComponent& GetArmorComponent() {
			return r_ArmorComponent;
		}

		/**
		 * @return A reference to the KnockbackComponent of the owning entity.
		 */
		KnockbackComponent& GetKnockbackComponent() {
			return r_KnockbackComponent;
		}

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent() {
			return r_HealthComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<HumanClassComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		ClientComponent& r_ClientComponent; /**< A component of the owning entity that this component depends on. */
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
		 */
		BuildableComponentBase(Entity& entity, HealthComponent& r_HealthComponent, ThinkingComponent& r_ThinkingComponent)
			: entity(entity), r_HealthComponent(r_HealthComponent), r_ThinkingComponent(r_ThinkingComponent){
			allSet.insert((BuildableComponent*)((char*) this - (char*) (BuildableComponentBase*) (BuildableComponent*) nullptr));
		}

		~BuildableComponentBase() {
			allSet.erase((BuildableComponent*)((char*) this - (char*) (BuildableComponentBase*) (BuildableComponent*) nullptr));
		}

		/**
		 * @return A reference to the HealthComponent of the owning entity.
		 */
		HealthComponent& GetHealthComponent() {
			return r_HealthComponent;
		}

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent() {
			return r_ThinkingComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<BuildableComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HealthComponent& r_HealthComponent; /**< A component of the owning entity that this component depends on. */
		ThinkingComponent& r_ThinkingComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<BuildableComponent*> allSet;
};

/** Base class of AlienBuildableComponent. */
class AlienBuildableComponentBase {
	public:
		/**
		 * @brief AlienBuildableComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_BuildableComponent A BuildableComponent instance that this component depends on.
		 * @param r_IgnitableComponent A IgnitableComponent instance that this component depends on.
		 */
		AlienBuildableComponentBase(Entity& entity, BuildableComponent& r_BuildableComponent, IgnitableComponent& r_IgnitableComponent)
			: entity(entity), r_BuildableComponent(r_BuildableComponent), r_IgnitableComponent(r_IgnitableComponent){
			allSet.insert((AlienBuildableComponent*)((char*) this - (char*) (AlienBuildableComponentBase*) (AlienBuildableComponent*) nullptr));
		}

		~AlienBuildableComponentBase() {
			allSet.erase((AlienBuildableComponent*)((char*) this - (char*) (AlienBuildableComponentBase*) (AlienBuildableComponent*) nullptr));
		}

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent() {
			return r_BuildableComponent;
		}

		/**
		 * @return A reference to the IgnitableComponent of the owning entity.
		 */
		IgnitableComponent& GetIgnitableComponent() {
			return r_IgnitableComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<AlienBuildableComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		BuildableComponent& r_BuildableComponent; /**< A component of the owning entity that this component depends on. */
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
		 */
		HumanBuildableComponentBase(Entity& entity, BuildableComponent& r_BuildableComponent)
			: entity(entity), r_BuildableComponent(r_BuildableComponent){
			allSet.insert((HumanBuildableComponent*)((char*) this - (char*) (HumanBuildableComponentBase*) (HumanBuildableComponent*) nullptr));
		}

		~HumanBuildableComponentBase() {
			allSet.erase((HumanBuildableComponent*)((char*) this - (char*) (HumanBuildableComponentBase*) (HumanBuildableComponent*) nullptr));
		}

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent() {
			return r_BuildableComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<HumanBuildableComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		BuildableComponent& r_BuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<HumanBuildableComponent*> allSet;
};

/** Base class of ResourceStorageComponent. */
class ResourceStorageComponentBase {
	public:
		/**
		 * @brief ResourceStorageComponentBase constructor.
		 * @param entity The entity that owns this component.
		 */
		ResourceStorageComponentBase(Entity& entity)
			: entity(entity){
			allSet.insert((ResourceStorageComponent*)((char*) this - (char*) (ResourceStorageComponentBase*) (ResourceStorageComponent*) nullptr));
		}

		~ResourceStorageComponentBase() {
			allSet.erase((ResourceStorageComponent*)((char*) this - (char*) (ResourceStorageComponentBase*) (ResourceStorageComponent*) nullptr));
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<ResourceStorageComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:

		static std::set<ResourceStorageComponent*> allSet;
};

/** Base class of MainBuildableComponent. */
class MainBuildableComponentBase {
	public:
		/**
		 * @brief MainBuildableComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_BuildableComponent A BuildableComponent instance that this component depends on.
		 * @param r_ResourceStorageComponent A ResourceStorageComponent instance that this component depends on.
		 */
		MainBuildableComponentBase(Entity& entity, BuildableComponent& r_BuildableComponent, ResourceStorageComponent& r_ResourceStorageComponent)
			: entity(entity), r_BuildableComponent(r_BuildableComponent), r_ResourceStorageComponent(r_ResourceStorageComponent){
			allSet.insert((MainBuildableComponent*)((char*) this - (char*) (MainBuildableComponentBase*) (MainBuildableComponent*) nullptr));
		}

		~MainBuildableComponentBase() {
			allSet.erase((MainBuildableComponent*)((char*) this - (char*) (MainBuildableComponentBase*) (MainBuildableComponent*) nullptr));
		}

		/**
		 * @return A reference to the BuildableComponent of the owning entity.
		 */
		BuildableComponent& GetBuildableComponent() {
			return r_BuildableComponent;
		}

		/**
		 * @return A reference to the ResourceStorageComponent of the owning entity.
		 */
		ResourceStorageComponent& GetResourceStorageComponent() {
			return r_ResourceStorageComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<MainBuildableComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		BuildableComponent& r_BuildableComponent; /**< A component of the owning entity that this component depends on. */
		ResourceStorageComponent& r_ResourceStorageComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<MainBuildableComponent*> allSet;
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
			allSet.insert((TurretComponent*)((char*) this - (char*) (TurretComponentBase*) (TurretComponent*) nullptr));
		}

		~TurretComponentBase() {
			allSet.erase((TurretComponent*)((char*) this - (char*) (TurretComponentBase*) (TurretComponent*) nullptr));
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
		 * @param r_ResourceStorageComponent A ResourceStorageComponent instance that this component depends on.
		 * @param r_ThinkingComponent A ThinkingComponent instance that this component depends on.
		 */
		MiningComponentBase(Entity& entity, ResourceStorageComponent& r_ResourceStorageComponent, ThinkingComponent& r_ThinkingComponent)
			: entity(entity), r_ResourceStorageComponent(r_ResourceStorageComponent), r_ThinkingComponent(r_ThinkingComponent){
			allSet.insert((MiningComponent*)((char*) this - (char*) (MiningComponentBase*) (MiningComponent*) nullptr));
		}

		~MiningComponentBase() {
			allSet.erase((MiningComponent*)((char*) this - (char*) (MiningComponentBase*) (MiningComponent*) nullptr));
		}

		/**
		 * @return A reference to the ResourceStorageComponent of the owning entity.
		 */
		ResourceStorageComponent& GetResourceStorageComponent() {
			return r_ResourceStorageComponent;
		}

		/**
		 * @return A reference to the ThinkingComponent of the owning entity.
		 */
		ThinkingComponent& GetThinkingComponent() {
			return r_ThinkingComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<MiningComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		ResourceStorageComponent& r_ResourceStorageComponent; /**< A component of the owning entity that this component depends on. */
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
			allSet.insert((AcidTubeComponent*)((char*) this - (char*) (AcidTubeComponentBase*) (AcidTubeComponent*) nullptr));
		}

		~AcidTubeComponentBase() {
			allSet.erase((AcidTubeComponent*)((char*) this - (char*) (AcidTubeComponentBase*) (AcidTubeComponent*) nullptr));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent() {
			return r_AlienBuildableComponent;
		}

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
			allSet.insert((BarricadeComponent*)((char*) this - (char*) (BarricadeComponentBase*) (BarricadeComponent*) nullptr));
		}

		~BarricadeComponentBase() {
			allSet.erase((BarricadeComponent*)((char*) this - (char*) (BarricadeComponentBase*) (BarricadeComponent*) nullptr));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent() {
			return r_AlienBuildableComponent;
		}

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
			allSet.insert((BoosterComponent*)((char*) this - (char*) (BoosterComponentBase*) (BoosterComponent*) nullptr));
		}

		~BoosterComponentBase() {
			allSet.erase((BoosterComponent*)((char*) this - (char*) (BoosterComponentBase*) (BoosterComponent*) nullptr));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent() {
			return r_AlienBuildableComponent;
		}

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
		 */
		EggComponentBase(Entity& entity, AlienBuildableComponent& r_AlienBuildableComponent)
			: entity(entity), r_AlienBuildableComponent(r_AlienBuildableComponent){
			allSet.insert((EggComponent*)((char*) this - (char*) (EggComponentBase*) (EggComponent*) nullptr));
		}

		~EggComponentBase() {
			allSet.erase((EggComponent*)((char*) this - (char*) (EggComponentBase*) (EggComponent*) nullptr));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent() {
			return r_AlienBuildableComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<EggComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		AlienBuildableComponent& r_AlienBuildableComponent; /**< A component of the owning entity that this component depends on. */

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
			allSet.insert((HiveComponent*)((char*) this - (char*) (HiveComponentBase*) (HiveComponent*) nullptr));
		}

		~HiveComponentBase() {
			allSet.erase((HiveComponent*)((char*) this - (char*) (HiveComponentBase*) (HiveComponent*) nullptr));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent() {
			return r_AlienBuildableComponent;
		}

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
			allSet.insert((LeechComponent*)((char*) this - (char*) (LeechComponentBase*) (LeechComponent*) nullptr));
		}

		~LeechComponentBase() {
			allSet.erase((LeechComponent*)((char*) this - (char*) (LeechComponentBase*) (LeechComponent*) nullptr));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent() {
			return r_AlienBuildableComponent;
		}

		/**
		 * @return A reference to the MiningComponent of the owning entity.
		 */
		MiningComponent& GetMiningComponent() {
			return r_MiningComponent;
		}

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
			allSet.insert((OvermindComponent*)((char*) this - (char*) (OvermindComponentBase*) (OvermindComponent*) nullptr));
		}

		~OvermindComponentBase() {
			allSet.erase((OvermindComponent*)((char*) this - (char*) (OvermindComponentBase*) (OvermindComponent*) nullptr));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent() {
			return r_AlienBuildableComponent;
		}

		/**
		 * @return A reference to the MainBuildableComponent of the owning entity.
		 */
		MainBuildableComponent& GetMainBuildableComponent() {
			return r_MainBuildableComponent;
		}

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
			allSet.insert((SpikerComponent*)((char*) this - (char*) (SpikerComponentBase*) (SpikerComponent*) nullptr));
		}

		~SpikerComponentBase() {
			allSet.erase((SpikerComponent*)((char*) this - (char*) (SpikerComponentBase*) (SpikerComponent*) nullptr));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent() {
			return r_AlienBuildableComponent;
		}

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
			allSet.insert((TrapperComponent*)((char*) this - (char*) (TrapperComponentBase*) (TrapperComponent*) nullptr));
		}

		~TrapperComponentBase() {
			allSet.erase((TrapperComponent*)((char*) this - (char*) (TrapperComponentBase*) (TrapperComponent*) nullptr));
		}

		/**
		 * @return A reference to the AlienBuildableComponent of the owning entity.
		 */
		AlienBuildableComponent& GetAlienBuildableComponent() {
			return r_AlienBuildableComponent;
		}

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
			allSet.insert((ArmouryComponent*)((char*) this - (char*) (ArmouryComponentBase*) (ArmouryComponent*) nullptr));
		}

		~ArmouryComponentBase() {
			allSet.erase((ArmouryComponent*)((char*) this - (char*) (ArmouryComponentBase*) (ArmouryComponent*) nullptr));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent() {
			return r_HumanBuildableComponent;
		}

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
			allSet.insert((DrillComponent*)((char*) this - (char*) (DrillComponentBase*) (DrillComponent*) nullptr));
		}

		~DrillComponentBase() {
			allSet.erase((DrillComponent*)((char*) this - (char*) (DrillComponentBase*) (DrillComponent*) nullptr));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent() {
			return r_HumanBuildableComponent;
		}

		/**
		 * @return A reference to the MiningComponent of the owning entity.
		 */
		MiningComponent& GetMiningComponent() {
			return r_MiningComponent;
		}

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
			allSet.insert((MedipadComponent*)((char*) this - (char*) (MedipadComponentBase*) (MedipadComponent*) nullptr));
		}

		~MedipadComponentBase() {
			allSet.erase((MedipadComponent*)((char*) this - (char*) (MedipadComponentBase*) (MedipadComponent*) nullptr));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent() {
			return r_HumanBuildableComponent;
		}

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
		 */
		MGTurretComponentBase(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent)
			: entity(entity), r_HumanBuildableComponent(r_HumanBuildableComponent){
			allSet.insert((MGTurretComponent*)((char*) this - (char*) (MGTurretComponentBase*) (MGTurretComponent*) nullptr));
		}

		~MGTurretComponentBase() {
			allSet.erase((MGTurretComponent*)((char*) this - (char*) (MGTurretComponentBase*) (MGTurretComponent*) nullptr));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent() {
			return r_HumanBuildableComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<MGTurretComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HumanBuildableComponent& r_HumanBuildableComponent; /**< A component of the owning entity that this component depends on. */

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
			allSet.insert((ReactorComponent*)((char*) this - (char*) (ReactorComponentBase*) (ReactorComponent*) nullptr));
		}

		~ReactorComponentBase() {
			allSet.erase((ReactorComponent*)((char*) this - (char*) (ReactorComponentBase*) (ReactorComponent*) nullptr));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent() {
			return r_HumanBuildableComponent;
		}

		/**
		 * @return A reference to the MainBuildableComponent of the owning entity.
		 */
		MainBuildableComponent& GetMainBuildableComponent() {
			return r_MainBuildableComponent;
		}

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

/** Base class of RepeaterComponent. */
class RepeaterComponentBase {
	public:
		/**
		 * @brief RepeaterComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_HumanBuildableComponent A HumanBuildableComponent instance that this component depends on.
		 */
		RepeaterComponentBase(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent)
			: entity(entity), r_HumanBuildableComponent(r_HumanBuildableComponent){
			allSet.insert((RepeaterComponent*)((char*) this - (char*) (RepeaterComponentBase*) (RepeaterComponent*) nullptr));
		}

		~RepeaterComponentBase() {
			allSet.erase((RepeaterComponent*)((char*) this - (char*) (RepeaterComponentBase*) (RepeaterComponent*) nullptr));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent() {
			return r_HumanBuildableComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<RepeaterComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HumanBuildableComponent& r_HumanBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<RepeaterComponent*> allSet;
};

/** Base class of RocketpodComponent. */
class RocketpodComponentBase {
	public:
		/**
		 * @brief RocketpodComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_HumanBuildableComponent A HumanBuildableComponent instance that this component depends on.
		 */
		RocketpodComponentBase(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent)
			: entity(entity), r_HumanBuildableComponent(r_HumanBuildableComponent){
			allSet.insert((RocketpodComponent*)((char*) this - (char*) (RocketpodComponentBase*) (RocketpodComponent*) nullptr));
		}

		~RocketpodComponentBase() {
			allSet.erase((RocketpodComponent*)((char*) this - (char*) (RocketpodComponentBase*) (RocketpodComponent*) nullptr));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent() {
			return r_HumanBuildableComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<RocketpodComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HumanBuildableComponent& r_HumanBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<RocketpodComponent*> allSet;
};

/** Base class of TelenodeComponent. */
class TelenodeComponentBase {
	public:
		/**
		 * @brief TelenodeComponentBase constructor.
		 * @param entity The entity that owns this component.
		 * @param r_HumanBuildableComponent A HumanBuildableComponent instance that this component depends on.
		 */
		TelenodeComponentBase(Entity& entity, HumanBuildableComponent& r_HumanBuildableComponent)
			: entity(entity), r_HumanBuildableComponent(r_HumanBuildableComponent){
			allSet.insert((TelenodeComponent*)((char*) this - (char*) (TelenodeComponentBase*) (TelenodeComponent*) nullptr));
		}

		~TelenodeComponentBase() {
			allSet.erase((TelenodeComponent*)((char*) this - (char*) (TelenodeComponentBase*) (TelenodeComponent*) nullptr));
		}

		/**
		 * @return A reference to the HumanBuildableComponent of the owning entity.
		 */
		HumanBuildableComponent& GetHumanBuildableComponent() {
			return r_HumanBuildableComponent;
		}

		/** A reference to the entity that owns the component instance. Allows sending back messages. */
		Entity& entity;

		static AllComponents<TelenodeComponent> GetAll() {
			return {allSet};
		}

	protected:

	private:
		HumanBuildableComponent& r_HumanBuildableComponent; /**< A component of the owning entity that this component depends on. */

		static std::set<TelenodeComponent*> allSet;
};


// ////////////////////////// //
// Definitions of ForEntities //
// ////////////////////////// //

template <typename Component> bool HasComponents(Entity& ent) {
    return ent.Get<Component>() != nullptr;
}

template <typename Component1, typename Component2, typename ... Components> bool HasComponents(Entity& ent) {
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
