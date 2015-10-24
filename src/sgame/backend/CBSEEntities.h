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
			struct gentity_s* oldEnt;
		};

		/** Default constructor of EmptyEntity. */
		EmptyEntity(Params params);

		/** Default destructor of EmptyEntity. */
		virtual ~EmptyEntity();


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
			struct gentity_s* oldEnt;
			gclient_t* Client_clientData;
		};

		/** Default constructor of ClientEntity. */
		ClientEntity(Params params);

		/** Default destructor of ClientEntity. */
		virtual ~ClientEntity();

		HealthComponent c_HealthComponent; /**< ClientEntity's HealthComponent instance. */
		ClientComponent c_ClientComponent; /**< ClientEntity's ClientComponent instance. */

	private:
		/** ClientEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** ClientEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class DretchEntity: public Entity {
	public:
		/** Initialization parameters for DretchEntity. */
		struct Params {
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of DretchEntity. */
		DretchEntity(Params params);

		/** Default destructor of DretchEntity. */
		virtual ~DretchEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of MantisEntity. */
		MantisEntity(Params params);

		/** Default destructor of MantisEntity. */
		virtual ~MantisEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of MarauderEntity. */
		MarauderEntity(Params params);

		/** Default destructor of MarauderEntity. */
		virtual ~MarauderEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of AdvMarauderEntity. */
		AdvMarauderEntity(Params params);

		/** Default destructor of AdvMarauderEntity. */
		virtual ~AdvMarauderEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of DragoonEntity. */
		DragoonEntity(Params params);

		/** Default destructor of DragoonEntity. */
		virtual ~DragoonEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of AdvDragoonEntity. */
		AdvDragoonEntity(Params params);

		/** Default destructor of AdvDragoonEntity. */
		virtual ~AdvDragoonEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of TyrantEntity. */
		TyrantEntity(Params params);

		/** Default destructor of TyrantEntity. */
		virtual ~TyrantEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of GrangerEntity. */
		GrangerEntity(Params params);

		/** Default destructor of GrangerEntity. */
		virtual ~GrangerEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of AdvGrangerEntity. */
		AdvGrangerEntity(Params params);

		/** Default destructor of AdvGrangerEntity. */
		virtual ~AdvGrangerEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of NakedHumanEntity. */
		NakedHumanEntity(Params params);

		/** Default destructor of NakedHumanEntity. */
		virtual ~NakedHumanEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of LightHumanEntity. */
		LightHumanEntity(Params params);

		/** Default destructor of LightHumanEntity. */
		virtual ~LightHumanEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of MediumHumanEntity. */
		MediumHumanEntity(Params params);

		/** Default destructor of MediumHumanEntity. */
		virtual ~MediumHumanEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
			gclient_t* Client_clientData;
		};

		/** Default constructor of HeavyHumanEntity. */
		HeavyHumanEntity(Params params);

		/** Default destructor of HeavyHumanEntity. */
		virtual ~HeavyHumanEntity();

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of AcidTubeEntity. */
		AcidTubeEntity(Params params);

		/** Default destructor of AcidTubeEntity. */
		virtual ~AcidTubeEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< AcidTubeEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< AcidTubeEntity's ThinkingComponent instance. */
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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of BarricadeEntity. */
		BarricadeEntity(Params params);

		/** Default destructor of BarricadeEntity. */
		virtual ~BarricadeEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< BarricadeEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< BarricadeEntity's ThinkingComponent instance. */
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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of BoosterEntity. */
		BoosterEntity(Params params);

		/** Default destructor of BoosterEntity. */
		virtual ~BoosterEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< BoosterEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< BoosterEntity's ThinkingComponent instance. */
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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of EggEntity. */
		EggEntity(Params params);

		/** Default destructor of EggEntity. */
		virtual ~EggEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< EggEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< EggEntity's ThinkingComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< EggEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< EggEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< EggEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< EggEntity's AlienBuildableComponent instance. */
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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of HiveEntity. */
		HiveEntity(Params params);

		/** Default destructor of HiveEntity. */
		virtual ~HiveEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< HiveEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< HiveEntity's ThinkingComponent instance. */
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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of LeechEntity. */
		LeechEntity(Params params);

		/** Default destructor of LeechEntity. */
		virtual ~LeechEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< LeechEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< LeechEntity's ThinkingComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< LeechEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< LeechEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< LeechEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< LeechEntity's AlienBuildableComponent instance. */
		ResourceStorageComponent c_ResourceStorageComponent; /**< LeechEntity's ResourceStorageComponent instance. */
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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of OvermindEntity. */
		OvermindEntity(Params params);

		/** Default destructor of OvermindEntity. */
		virtual ~OvermindEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< OvermindEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< OvermindEntity's ThinkingComponent instance. */
		IgnitableComponent c_IgnitableComponent; /**< OvermindEntity's IgnitableComponent instance. */
		HealthComponent c_HealthComponent; /**< OvermindEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< OvermindEntity's BuildableComponent instance. */
		AlienBuildableComponent c_AlienBuildableComponent; /**< OvermindEntity's AlienBuildableComponent instance. */
		ResourceStorageComponent c_ResourceStorageComponent; /**< OvermindEntity's ResourceStorageComponent instance. */
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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of SpikerEntity. */
		SpikerEntity(Params params);

		/** Default destructor of SpikerEntity. */
		virtual ~SpikerEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< SpikerEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< SpikerEntity's ThinkingComponent instance. */
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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of TrapperEntity. */
		TrapperEntity(Params params);

		/** Default destructor of TrapperEntity. */
		virtual ~TrapperEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< TrapperEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< TrapperEntity's ThinkingComponent instance. */
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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of ArmouryEntity. */
		ArmouryEntity(Params params);

		/** Default destructor of ArmouryEntity. */
		virtual ~ArmouryEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< ArmouryEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< ArmouryEntity's ThinkingComponent instance. */
		HealthComponent c_HealthComponent; /**< ArmouryEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< ArmouryEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< ArmouryEntity's HumanBuildableComponent instance. */

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of DrillEntity. */
		DrillEntity(Params params);

		/** Default destructor of DrillEntity. */
		virtual ~DrillEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< DrillEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< DrillEntity's ThinkingComponent instance. */
		HealthComponent c_HealthComponent; /**< DrillEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< DrillEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< DrillEntity's HumanBuildableComponent instance. */
		ResourceStorageComponent c_ResourceStorageComponent; /**< DrillEntity's ResourceStorageComponent instance. */
		MiningComponent c_MiningComponent; /**< DrillEntity's MiningComponent instance. */

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of MedipadEntity. */
		MedipadEntity(Params params);

		/** Default destructor of MedipadEntity. */
		virtual ~MedipadEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< MedipadEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< MedipadEntity's ThinkingComponent instance. */
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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of MGTurretEntity. */
		MGTurretEntity(Params params);

		/** Default destructor of MGTurretEntity. */
		virtual ~MGTurretEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< MGTurretEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< MGTurretEntity's ThinkingComponent instance. */
		HealthComponent c_HealthComponent; /**< MGTurretEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< MGTurretEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< MGTurretEntity's HumanBuildableComponent instance. */
		TurretComponent c_TurretComponent; /**< MGTurretEntity's TurretComponent instance. */

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of ReactorEntity. */
		ReactorEntity(Params params);

		/** Default destructor of ReactorEntity. */
		virtual ~ReactorEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< ReactorEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< ReactorEntity's ThinkingComponent instance. */
		HealthComponent c_HealthComponent; /**< ReactorEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< ReactorEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< ReactorEntity's HumanBuildableComponent instance. */
		ResourceStorageComponent c_ResourceStorageComponent; /**< ReactorEntity's ResourceStorageComponent instance. */
		MainBuildableComponent c_MainBuildableComponent; /**< ReactorEntity's MainBuildableComponent instance. */
		ReactorComponent c_ReactorComponent; /**< ReactorEntity's ReactorComponent instance. */

	private:
		/** ReactorEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** ReactorEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class RepeaterEntity: public Entity {
	public:
		/** Initialization parameters for RepeaterEntity. */
		struct Params {
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of RepeaterEntity. */
		RepeaterEntity(Params params);

		/** Default destructor of RepeaterEntity. */
		virtual ~RepeaterEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< RepeaterEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< RepeaterEntity's ThinkingComponent instance. */
		HealthComponent c_HealthComponent; /**< RepeaterEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< RepeaterEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< RepeaterEntity's HumanBuildableComponent instance. */

	private:
		/** RepeaterEntity's message handler vtable. */
		static const MessageHandler messageHandlers[];

		/** RepeaterEntity's component offset table. */
		static const int componentOffsets[];
};

/** A specific entity. */
class RocketpodEntity: public Entity {
	public:
		/** Initialization parameters for RocketpodEntity. */
		struct Params {
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of RocketpodEntity. */
		RocketpodEntity(Params params);

		/** Default destructor of RocketpodEntity. */
		virtual ~RocketpodEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< RocketpodEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< RocketpodEntity's ThinkingComponent instance. */
		HealthComponent c_HealthComponent; /**< RocketpodEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< RocketpodEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< RocketpodEntity's HumanBuildableComponent instance. */
		TurretComponent c_TurretComponent; /**< RocketpodEntity's TurretComponent instance. */

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
			struct gentity_s* oldEnt;
			float Health_maxHealth;
		};

		/** Default constructor of TelenodeEntity. */
		TelenodeEntity(Params params);

		/** Default destructor of TelenodeEntity. */
		virtual ~TelenodeEntity();

		DeferredFreeingComponent c_DeferredFreeingComponent; /**< TelenodeEntity's DeferredFreeingComponent instance. */
		ThinkingComponent c_ThinkingComponent; /**< TelenodeEntity's ThinkingComponent instance. */
		HealthComponent c_HealthComponent; /**< TelenodeEntity's HealthComponent instance. */
		BuildableComponent c_BuildableComponent; /**< TelenodeEntity's BuildableComponent instance. */
		HumanBuildableComponent c_HumanBuildableComponent; /**< TelenodeEntity's HumanBuildableComponent instance. */

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
			struct gentity_s* oldEnt;
		};

		/** Default constructor of FireEntity. */
		FireEntity(Params params);

		/** Default destructor of FireEntity. */
		virtual ~FireEntity();

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
