// THIS FILE IS AUTO GENERATED, EDIT AT YOUR OWN RISK

/*
 * This file contains:
 *   - Declarations of the specific entities.
 */

#ifndef CBSE_ENTITIES_H_
#define CBSE_ENTITIES_H_

#include "CBSEComponents.h"

// ///////////////// //
// Specific Entities //
// ///////////////// //

/** A specific entity. */
class EmptyEntity: public Entity {
	public:
		/** Initialization parameters for EmptyEntity. */
		struct Params {
			gentity_t* oldEnt;
		};

		/** Default constructor of EmptyEntity. */
		EmptyEntity(Params params);

		/** Default destructor of EmptyEntity. */
		virtual ~EmptyEntity() = default;


	private:
		/** EmptyEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** EmptyEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class ClientEntity: public Entity {
	public:
		/** Initialization parameters for ClientEntity. */
		struct Params {
			gentity_t* oldEnt;
			gclient_t* Client_clientData;
		};

		/** Default constructor of ClientEntity. */
		ClientEntity(Params params);

		/** Default destructor of ClientEntity. */
		virtual ~ClientEntity() = default;

		TeamComponent c_TeamComponent; /**< ClientEntity's TeamComponent instance. */
		ClientComponent c_ClientComponent; /**< ClientEntity's ClientComponent instance. */

	private:
		/** ClientEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** ClientEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class SpectatorEntity: public Entity {
	public:
		/** Initialization parameters for SpectatorEntity. */
		struct Params {
			gentity_t* oldEnt;
			team_t Team_team;
			gclient_t* Client_clientData;
		};

		/** Default constructor of SpectatorEntity. */
		SpectatorEntity(Params params);

		/** Default destructor of SpectatorEntity. */
		virtual ~SpectatorEntity() = default;

		TeamComponent c_TeamComponent; /**< SpectatorEntity's TeamComponent instance. */
		ClientComponent c_ClientComponent; /**< SpectatorEntity's ClientComponent instance. */
		SpectatorComponent c_SpectatorComponent; /**< SpectatorEntity's SpectatorComponent instance. */

	private:
		/** SpectatorEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** SpectatorEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class DretchEntity: public Entity {
	public:
		/** Initialization parameters for DretchEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of DretchEntity. */
		DretchEntity(Params params);

		/** Default destructor of DretchEntity. */
		virtual ~DretchEntity() = default;

		TeamComponent c_TeamComponent; /**< DretchEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< DretchEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< DretchEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< DretchEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< DretchEntity's KnockbackComponent instance. */
		AlienClassComponent c_AlienClassComponent; /**< DretchEntity's AlienClassComponent instance. */

	private:
		/** DretchEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** DretchEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class MantisEntity: public Entity {
	public:
		/** Initialization parameters for MantisEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of MantisEntity. */
		MantisEntity(Params params);

		/** Default destructor of MantisEntity. */
		virtual ~MantisEntity() = default;

		TeamComponent c_TeamComponent; /**< MantisEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< MantisEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< MantisEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< MantisEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< MantisEntity's KnockbackComponent instance. */
		AlienClassComponent c_AlienClassComponent; /**< MantisEntity's AlienClassComponent instance. */

	private:
		/** MantisEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** MantisEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class MarauderEntity: public Entity {
	public:
		/** Initialization parameters for MarauderEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of MarauderEntity. */
		MarauderEntity(Params params);

		/** Default destructor of MarauderEntity. */
		virtual ~MarauderEntity() = default;

		TeamComponent c_TeamComponent; /**< MarauderEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< MarauderEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< MarauderEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< MarauderEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< MarauderEntity's KnockbackComponent instance. */
		AlienClassComponent c_AlienClassComponent; /**< MarauderEntity's AlienClassComponent instance. */

	private:
		/** MarauderEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** MarauderEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class AdvMarauderEntity: public Entity {
	public:
		/** Initialization parameters for AdvMarauderEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of AdvMarauderEntity. */
		AdvMarauderEntity(Params params);

		/** Default destructor of AdvMarauderEntity. */
		virtual ~AdvMarauderEntity() = default;

		TeamComponent c_TeamComponent; /**< AdvMarauderEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< AdvMarauderEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< AdvMarauderEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< AdvMarauderEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< AdvMarauderEntity's KnockbackComponent instance. */
		AlienClassComponent c_AlienClassComponent; /**< AdvMarauderEntity's AlienClassComponent instance. */

	private:
		/** AdvMarauderEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** AdvMarauderEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class DragoonEntity: public Entity {
	public:
		/** Initialization parameters for DragoonEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of DragoonEntity. */
		DragoonEntity(Params params);

		/** Default destructor of DragoonEntity. */
		virtual ~DragoonEntity() = default;

		TeamComponent c_TeamComponent; /**< DragoonEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< DragoonEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< DragoonEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< DragoonEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< DragoonEntity's KnockbackComponent instance. */
		AlienClassComponent c_AlienClassComponent; /**< DragoonEntity's AlienClassComponent instance. */

	private:
		/** DragoonEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** DragoonEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class AdvDragoonEntity: public Entity {
	public:
		/** Initialization parameters for AdvDragoonEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of AdvDragoonEntity. */
		AdvDragoonEntity(Params params);

		/** Default destructor of AdvDragoonEntity. */
		virtual ~AdvDragoonEntity() = default;

		TeamComponent c_TeamComponent; /**< AdvDragoonEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< AdvDragoonEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< AdvDragoonEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< AdvDragoonEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< AdvDragoonEntity's KnockbackComponent instance. */
		AlienClassComponent c_AlienClassComponent; /**< AdvDragoonEntity's AlienClassComponent instance. */

	private:
		/** AdvDragoonEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** AdvDragoonEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class TyrantEntity: public Entity {
	public:
		/** Initialization parameters for TyrantEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of TyrantEntity. */
		TyrantEntity(Params params);

		/** Default destructor of TyrantEntity. */
		virtual ~TyrantEntity() = default;

		TeamComponent c_TeamComponent; /**< TyrantEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< TyrantEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< TyrantEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< TyrantEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< TyrantEntity's KnockbackComponent instance. */
		AlienClassComponent c_AlienClassComponent; /**< TyrantEntity's AlienClassComponent instance. */

	private:
		/** TyrantEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** TyrantEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class GrangerEntity: public Entity {
	public:
		/** Initialization parameters for GrangerEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of GrangerEntity. */
		GrangerEntity(Params params);

		/** Default destructor of GrangerEntity. */
		virtual ~GrangerEntity() = default;

		TeamComponent c_TeamComponent; /**< GrangerEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< GrangerEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< GrangerEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< GrangerEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< GrangerEntity's KnockbackComponent instance. */
		AlienClassComponent c_AlienClassComponent; /**< GrangerEntity's AlienClassComponent instance. */

	private:
		/** GrangerEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** GrangerEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class AdvGrangerEntity: public Entity {
	public:
		/** Initialization parameters for AdvGrangerEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of AdvGrangerEntity. */
		AdvGrangerEntity(Params params);

		/** Default destructor of AdvGrangerEntity. */
		virtual ~AdvGrangerEntity() = default;

		TeamComponent c_TeamComponent; /**< AdvGrangerEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< AdvGrangerEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< AdvGrangerEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< AdvGrangerEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< AdvGrangerEntity's KnockbackComponent instance. */
		AlienClassComponent c_AlienClassComponent; /**< AdvGrangerEntity's AlienClassComponent instance. */

	private:
		/** AdvGrangerEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** AdvGrangerEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class NakedHumanEntity: public Entity {
	public:
		/** Initialization parameters for NakedHumanEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of NakedHumanEntity. */
		NakedHumanEntity(Params params);

		/** Default destructor of NakedHumanEntity. */
		virtual ~NakedHumanEntity() = default;

		TeamComponent c_TeamComponent; /**< NakedHumanEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< NakedHumanEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< NakedHumanEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< NakedHumanEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< NakedHumanEntity's KnockbackComponent instance. */
		HumanClassComponent c_HumanClassComponent; /**< NakedHumanEntity's HumanClassComponent instance. */

	private:
		/** NakedHumanEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** NakedHumanEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class LightHumanEntity: public Entity {
	public:
		/** Initialization parameters for LightHumanEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of LightHumanEntity. */
		LightHumanEntity(Params params);

		/** Default destructor of LightHumanEntity. */
		virtual ~LightHumanEntity() = default;

		TeamComponent c_TeamComponent; /**< LightHumanEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< LightHumanEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< LightHumanEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< LightHumanEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< LightHumanEntity's KnockbackComponent instance. */
		HumanClassComponent c_HumanClassComponent; /**< LightHumanEntity's HumanClassComponent instance. */

	private:
		/** LightHumanEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** LightHumanEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class MediumHumanEntity: public Entity {
	public:
		/** Initialization parameters for MediumHumanEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of MediumHumanEntity. */
		MediumHumanEntity(Params params);

		/** Default destructor of MediumHumanEntity. */
		virtual ~MediumHumanEntity() = default;

		TeamComponent c_TeamComponent; /**< MediumHumanEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< MediumHumanEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< MediumHumanEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< MediumHumanEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< MediumHumanEntity's KnockbackComponent instance. */
		HumanClassComponent c_HumanClassComponent; /**< MediumHumanEntity's HumanClassComponent instance. */

	private:
		/** MediumHumanEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** MediumHumanEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class HeavyHumanEntity: public Entity {
	public:
		/** Initialization parameters for HeavyHumanEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of HeavyHumanEntity. */
		HeavyHumanEntity(Params params);

		/** Default destructor of HeavyHumanEntity. */
		virtual ~HeavyHumanEntity() = default;

		TeamComponent c_TeamComponent; /**< HeavyHumanEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< HeavyHumanEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< HeavyHumanEntity's ClientComponent instance. */
		ArmorComponent c_ArmorComponent; /**< HeavyHumanEntity's ArmorComponent instance. */
		KnockbackComponent c_KnockbackComponent; /**< HeavyHumanEntity's KnockbackComponent instance. */
		HumanClassComponent c_HumanClassComponent; /**< HeavyHumanEntity's HumanClassComponent instance. */

	private:
		/** HeavyHumanEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** HeavyHumanEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class AcidTubeEntity: public Entity {
	public:
		/** Initialization parameters for AcidTubeEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of AcidTubeEntity. */
		AcidTubeEntity(Params params);

		/** Default destructor of AcidTubeEntity. */
		virtual ~AcidTubeEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< AcidTubeEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< AcidTubeEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< AcidTubeEntity's TeamComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< AcidTubeEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< AcidTubeEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< AcidTubeEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< AcidTubeEntity's AlienBuildableComponent instance. */
		AcidTubeComponent c_AcidTubeComponent; /**< AcidTubeEntity's AcidTubeComponent instance. */

	private:
		/** AcidTubeEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** AcidTubeEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class BarricadeEntity: public Entity {
	public:
		/** Initialization parameters for BarricadeEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of BarricadeEntity. */
		BarricadeEntity(Params params);

		/** Default destructor of BarricadeEntity. */
		virtual ~BarricadeEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< BarricadeEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< BarricadeEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< BarricadeEntity's TeamComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< BarricadeEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< BarricadeEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< BarricadeEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< BarricadeEntity's AlienBuildableComponent instance. */
		BarricadeComponent c_BarricadeComponent; /**< BarricadeEntity's BarricadeComponent instance. */

	private:
		/** BarricadeEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** BarricadeEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class BoosterEntity: public Entity {
	public:
		/** Initialization parameters for BoosterEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of BoosterEntity. */
		BoosterEntity(Params params);

		/** Default destructor of BoosterEntity. */
		virtual ~BoosterEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< BoosterEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< BoosterEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< BoosterEntity's TeamComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< BoosterEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< BoosterEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< BoosterEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< BoosterEntity's AlienBuildableComponent instance. */
		BoosterComponent c_BoosterComponent; /**< BoosterEntity's BoosterComponent instance. */

	private:
		/** BoosterEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** BoosterEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class EggEntity: public Entity {
	public:
		/** Initialization parameters for EggEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of EggEntity. */
		EggEntity(Params params);

		/** Default destructor of EggEntity. */
		virtual ~EggEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< EggEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< EggEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< EggEntity's TeamComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< EggEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< EggEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< EggEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< EggEntity's AlienBuildableComponent instance. */
		SpawnerComponent c_SpawnerComponent; /**< EggEntity's SpawnerComponent instance. */
		EggComponent c_EggComponent; /**< EggEntity's EggComponent instance. */

	private:
		/** EggEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** EggEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class HiveEntity: public Entity {
	public:
		/** Initialization parameters for HiveEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of HiveEntity. */
		HiveEntity(Params params);

		/** Default destructor of HiveEntity. */
		virtual ~HiveEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< HiveEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< HiveEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< HiveEntity's TeamComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< HiveEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< HiveEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< HiveEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< HiveEntity's AlienBuildableComponent instance. */
		HiveComponent c_HiveComponent; /**< HiveEntity's HiveComponent instance. */

	private:
		/** HiveEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** HiveEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class LeechEntity: public Entity {
	public:
		/** Initialization parameters for LeechEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of LeechEntity. */
		LeechEntity(Params params);

		/** Default destructor of LeechEntity. */
		virtual ~LeechEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< LeechEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< LeechEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< LeechEntity's TeamComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< LeechEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< LeechEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< LeechEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< LeechEntity's AlienBuildableComponent instance. */
		MiningComponent c_MiningComponent; /**< LeechEntity's MiningComponent instance. */
		LeechComponent c_LeechComponent; /**< LeechEntity's LeechComponent instance. */

	private:
		/** LeechEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** LeechEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class OvermindEntity: public Entity {
	public:
		/** Initialization parameters for OvermindEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of OvermindEntity. */
		OvermindEntity(Params params);

		/** Default destructor of OvermindEntity. */
		virtual ~OvermindEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< OvermindEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< OvermindEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< OvermindEntity's TeamComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< OvermindEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< OvermindEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< OvermindEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< OvermindEntity's AlienBuildableComponent instance. */
		MainBuildableComponent c_MainBuildableComponent; /**< OvermindEntity's MainBuildableComponent instance. */
		OvermindComponent c_OvermindComponent; /**< OvermindEntity's OvermindComponent instance. */

	private:
		/** OvermindEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** OvermindEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class SpikerEntity: public Entity {
	public:
		/** Initialization parameters for SpikerEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of SpikerEntity. */
		SpikerEntity(Params params);

		/** Default destructor of SpikerEntity. */
		virtual ~SpikerEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< SpikerEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< SpikerEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< SpikerEntity's TeamComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< SpikerEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< SpikerEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< SpikerEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< SpikerEntity's AlienBuildableComponent instance. */
		SpikerComponent c_SpikerComponent; /**< SpikerEntity's SpikerComponent instance. */

	private:
		/** SpikerEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** SpikerEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class TrapperEntity: public Entity {
	public:
		/** Initialization parameters for TrapperEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of TrapperEntity. */
		TrapperEntity(Params params);

		/** Default destructor of TrapperEntity. */
		virtual ~TrapperEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< TrapperEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< TrapperEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< TrapperEntity's TeamComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< TrapperEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< TrapperEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< TrapperEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< TrapperEntity's AlienBuildableComponent instance. */
		TrapperComponent c_TrapperComponent; /**< TrapperEntity's TrapperComponent instance. */

	private:
		/** TrapperEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** TrapperEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class ArmouryEntity: public Entity {
	public:
		/** Initialization parameters for ArmouryEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of ArmouryEntity. */
		ArmouryEntity(Params params);

		/** Default destructor of ArmouryEntity. */
		virtual ~ArmouryEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< ArmouryEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< ArmouryEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< ArmouryEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< ArmouryEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< ArmouryEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< ArmouryEntity's HumanBuildableComponent instance. */
		ArmouryComponent c_ArmouryComponent; /**< ArmouryEntity's ArmouryComponent instance. */

	private:
		/** ArmouryEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** ArmouryEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class DrillEntity: public Entity {
	public:
		/** Initialization parameters for DrillEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of DrillEntity. */
		DrillEntity(Params params);

		/** Default destructor of DrillEntity. */
		virtual ~DrillEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< DrillEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< DrillEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< DrillEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< DrillEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< DrillEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< DrillEntity's HumanBuildableComponent instance. */
		MiningComponent c_MiningComponent; /**< DrillEntity's MiningComponent instance. */
		DrillComponent c_DrillComponent; /**< DrillEntity's DrillComponent instance. */

	private:
		/** DrillEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** DrillEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class MedipadEntity: public Entity {
	public:
		/** Initialization parameters for MedipadEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of MedipadEntity. */
		MedipadEntity(Params params);

		/** Default destructor of MedipadEntity. */
		virtual ~MedipadEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< MedipadEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< MedipadEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< MedipadEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< MedipadEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< MedipadEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< MedipadEntity's HumanBuildableComponent instance. */
		MedipadComponent c_MedipadComponent; /**< MedipadEntity's MedipadComponent instance. */

	private:
		/** MedipadEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** MedipadEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class MGTurretEntity: public Entity {
	public:
		/** Initialization parameters for MGTurretEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of MGTurretEntity. */
		MGTurretEntity(Params params);

		/** Default destructor of MGTurretEntity. */
		virtual ~MGTurretEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< MGTurretEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< MGTurretEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< MGTurretEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< MGTurretEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< MGTurretEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< MGTurretEntity's HumanBuildableComponent instance. */
		TurretComponent c_TurretComponent; /**< MGTurretEntity's TurretComponent instance. */
		MGTurretComponent c_MGTurretComponent; /**< MGTurretEntity's MGTurretComponent instance. */

	private:
		/** MGTurretEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** MGTurretEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class ReactorEntity: public Entity {
	public:
		/** Initialization parameters for ReactorEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of ReactorEntity. */
		ReactorEntity(Params params);

		/** Default destructor of ReactorEntity. */
		virtual ~ReactorEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< ReactorEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< ReactorEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< ReactorEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< ReactorEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< ReactorEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< ReactorEntity's HumanBuildableComponent instance. */
		MainBuildableComponent c_MainBuildableComponent; /**< ReactorEntity's MainBuildableComponent instance. */
		ReactorComponent c_ReactorComponent; /**< ReactorEntity's ReactorComponent instance. */

	private:
		/** ReactorEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** ReactorEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class RocketpodEntity: public Entity {
	public:
		/** Initialization parameters for RocketpodEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of RocketpodEntity. */
		RocketpodEntity(Params params);

		/** Default destructor of RocketpodEntity. */
		virtual ~RocketpodEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< RocketpodEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< RocketpodEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< RocketpodEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< RocketpodEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< RocketpodEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< RocketpodEntity's HumanBuildableComponent instance. */
		TurretComponent c_TurretComponent; /**< RocketpodEntity's TurretComponent instance. */
		RocketpodComponent c_RocketpodComponent; /**< RocketpodEntity's RocketpodComponent instance. */

	private:
		/** RocketpodEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** RocketpodEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class TelenodeEntity: public Entity {
	public:
		/** Initialization parameters for TelenodeEntity. */
		struct Params {
			gentity_t* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of TelenodeEntity. */
		TelenodeEntity(Params params);

		/** Default destructor of TelenodeEntity. */
		virtual ~TelenodeEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< TelenodeEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< TelenodeEntity's ThinkingComponent instance. */
		TeamComponent c_TeamComponent; /**< TelenodeEntity's TeamComponent instance. */
		HealthComponent c_HealthComponent; /**< TelenodeEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< TelenodeEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< TelenodeEntity's HumanBuildableComponent instance. */
		SpawnerComponent c_SpawnerComponent; /**< TelenodeEntity's SpawnerComponent instance. */
		TelenodeComponent c_TelenodeComponent; /**< TelenodeEntity's TelenodeComponent instance. */

	private:
		/** TelenodeEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** TelenodeEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class FireEntity: public Entity {
	public:
		/** Initialization parameters for FireEntity. */
		struct Params {
			gentity_t* oldEnt;
		};

		/** Default constructor of FireEntity. */
		FireEntity(Params params);

		/** Default destructor of FireEntity. */
		virtual ~FireEntity() = default;

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< FireEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< FireEntity's ThinkingComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< FireEntity's IgnitableComponent instance. */

	private:
		/** FireEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** FireEntity's component offset table. */
		static const int componentOffsets[];
};


#endif // CBSE_ENTITIES_H_
