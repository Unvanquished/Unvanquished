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
Entity::Entity(const MessageHandler *messageHandlers, const int* componentOffsets, struct gentity_s* oldEnt)
	: messageHandlers(messageHandlers), componentOffsets(componentOffsets), oldEnt(oldEnt)
{}

// Base entity deconstructor.
Entity::~Entity()
{}

// Base entity's message dispatcher.
bool Entity::SendMessage(int msg, const void* data) {
	MessageHandler handler = messageHandlers[msg];
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
	return SendMessage(MSG_PREPARENETCODE, nullptr);
}

bool Entity::FreeAt(int freeTime) {
	std::tuple<int> data(freeTime);
	return SendMessage(MSG_FREEAT, &data);
}

bool Entity::Ignite(gentity_t* fireStarter) {
	std::tuple<gentity_t*> data(fireStarter);
	return SendMessage(MSG_IGNITE, &data);
}

bool Entity::Extinguish(int immunityTime) {
	std::tuple<int> data(immunityTime);
	return SendMessage(MSG_EXTINGUISH, &data);
}

bool Entity::Heal(float amount, gentity_t* source) {
	std::tuple<float, gentity_t*> data(amount, source);
	return SendMessage(MSG_HEAL, &data);
}

bool Entity::Damage(float amount, gentity_t* source, Util::optional<Vec3> location, Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t> data(amount, source, location, direction, flags, meansOfDeath);
	return SendMessage(MSG_DAMAGE, &data);
}

bool Entity::Die(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	std::tuple<gentity_t*, meansOfDeath_t> data(killer, meansOfDeath);
	return SendMessage(MSG_DIE, &data);
}

bool Entity::ApplyDamageModifier(float& damage, Util::optional<Vec3> location, Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t> data(damage, location, direction, flags, meansOfDeath);
	return SendMessage(MSG_APPLYDAMAGEMODIFIER, &data);
}

std::set<DeferredFreeingComponent*> DeferredFreeingComponentBase::allSet;
std::set<ThinkingComponent*> ThinkingComponentBase::allSet;
std::set<IgnitableComponent*> IgnitableComponentBase::allSet;
std::set<HealthComponent*> HealthComponentBase::allSet;
std::set<ClientComponent*> ClientComponentBase::allSet;
std::set<ArmorComponent*> ArmorComponentBase::allSet;
std::set<KnockbackComponent*> KnockbackComponentBase::allSet;
std::set<AlienClassComponent*> AlienClassComponentBase::allSet;
std::set<HumanClassComponent*> HumanClassComponentBase::allSet;
std::set<BuildableComponent*> BuildableComponentBase::allSet;
std::set<AlienBuildableComponent*> AlienBuildableComponentBase::allSet;
std::set<HumanBuildableComponent*> HumanBuildableComponentBase::allSet;
std::set<ResourceStorageComponent*> ResourceStorageComponentBase::allSet;
std::set<MainBuildableComponent*> MainBuildableComponentBase::allSet;
std::set<TurretComponent*> TurretComponentBase::allSet;
std::set<MiningComponent*> MiningComponentBase::allSet;
std::set<AcidTubeComponent*> AcidTubeComponentBase::allSet;
std::set<BarricadeComponent*> BarricadeComponentBase::allSet;
std::set<BoosterComponent*> BoosterComponentBase::allSet;
std::set<EggComponent*> EggComponentBase::allSet;
std::set<HiveComponent*> HiveComponentBase::allSet;
std::set<LeechComponent*> LeechComponentBase::allSet;
std::set<OvermindComponent*> OvermindComponentBase::allSet;
std::set<SpikerComponent*> SpikerComponentBase::allSet;
std::set<TrapperComponent*> TrapperComponentBase::allSet;
std::set<ArmouryComponent*> ArmouryComponentBase::allSet;
std::set<DrillComponent*> DrillComponentBase::allSet;
std::set<MedipadComponent*> MedipadComponentBase::allSet;
std::set<MGTurretComponent*> MGTurretComponentBase::allSet;
std::set<ReactorComponent*> ReactorComponentBase::allSet;
std::set<RepeaterComponent*> RepeaterComponentBase::allSet;
std::set<RocketpodComponent*> RocketpodComponentBase::allSet;
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
};

// EmptyEntity's constructor.
EmptyEntity::EmptyEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
{}

// EmptyEntity's deconstructor.
EmptyEntity::~EmptyEntity()
{}

// ClientEntity
// ---------

// ClientEntity's component offset vtable.
const int ClientEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(ClientEntity, c_HealthComponent),
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

// ClientEntity's Damage message dispatcher.
void h_Client_Damage(Entity* _entity, const void*  _data) {
	ClientEntity* entity = (ClientEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// ClientEntity's Heal message dispatcher.
void h_Client_Heal(Entity* _entity, const void*  _data) {
	ClientEntity* entity = (ClientEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// ClientEntity's PrepareNetCode message dispatcher.
void h_Client_PrepareNetCode(Entity* _entity, const void* ) {
	ClientEntity* entity = (ClientEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// ClientEntity's message dispatcher vtable.
const MessageHandler ClientEntity::messageHandlers[] = {
	h_Client_PrepareNetCode,
	nullptr,
	nullptr,
	nullptr,
	h_Client_Heal,
	h_Client_Damage,
	nullptr,
	nullptr,
};

// ClientEntity's constructor.
ClientEntity::ClientEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, 1)
	, c_ClientComponent(*this, params.Client_clientData)
{}

// ClientEntity's deconstructor.
ClientEntity::~ClientEntity()
{}

// DretchEntity
// ---------

// DretchEntity's component offset vtable.
const int DretchEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(DretchEntity, c_HealthComponent),
	myoffsetof(DretchEntity, c_ClientComponent),
	myoffsetof(DretchEntity, c_ArmorComponent),
	myoffsetof(DretchEntity, c_KnockbackComponent),
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
	0,
};

// DretchEntity's Damage message dispatcher.
void h_Dretch_Damage(Entity* _entity, const void*  _data) {
	DretchEntity* entity = (DretchEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// DretchEntity's Heal message dispatcher.
void h_Dretch_Heal(Entity* _entity, const void*  _data) {
	DretchEntity* entity = (DretchEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// DretchEntity's PrepareNetCode message dispatcher.
void h_Dretch_PrepareNetCode(Entity* _entity, const void* ) {
	DretchEntity* entity = (DretchEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// DretchEntity's ApplyDamageModifier message dispatcher.
void h_Dretch_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	DretchEntity* entity = (DretchEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_Dretch_ApplyDamageModifier,
};

// DretchEntity's constructor.
DretchEntity::DretchEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// DretchEntity's deconstructor.
DretchEntity::~DretchEntity()
{}

// MantisEntity
// ---------

// MantisEntity's component offset vtable.
const int MantisEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(MantisEntity, c_HealthComponent),
	myoffsetof(MantisEntity, c_ClientComponent),
	myoffsetof(MantisEntity, c_ArmorComponent),
	myoffsetof(MantisEntity, c_KnockbackComponent),
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
	0,
};

// MantisEntity's Damage message dispatcher.
void h_Mantis_Damage(Entity* _entity, const void*  _data) {
	MantisEntity* entity = (MantisEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// MantisEntity's Heal message dispatcher.
void h_Mantis_Heal(Entity* _entity, const void*  _data) {
	MantisEntity* entity = (MantisEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// MantisEntity's PrepareNetCode message dispatcher.
void h_Mantis_PrepareNetCode(Entity* _entity, const void* ) {
	MantisEntity* entity = (MantisEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// MantisEntity's ApplyDamageModifier message dispatcher.
void h_Mantis_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	MantisEntity* entity = (MantisEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_Mantis_ApplyDamageModifier,
};

// MantisEntity's constructor.
MantisEntity::MantisEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// MantisEntity's deconstructor.
MantisEntity::~MantisEntity()
{}

// MarauderEntity
// ---------

// MarauderEntity's component offset vtable.
const int MarauderEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(MarauderEntity, c_HealthComponent),
	myoffsetof(MarauderEntity, c_ClientComponent),
	myoffsetof(MarauderEntity, c_ArmorComponent),
	myoffsetof(MarauderEntity, c_KnockbackComponent),
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
	0,
};

// MarauderEntity's Damage message dispatcher.
void h_Marauder_Damage(Entity* _entity, const void*  _data) {
	MarauderEntity* entity = (MarauderEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// MarauderEntity's Heal message dispatcher.
void h_Marauder_Heal(Entity* _entity, const void*  _data) {
	MarauderEntity* entity = (MarauderEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// MarauderEntity's PrepareNetCode message dispatcher.
void h_Marauder_PrepareNetCode(Entity* _entity, const void* ) {
	MarauderEntity* entity = (MarauderEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// MarauderEntity's ApplyDamageModifier message dispatcher.
void h_Marauder_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	MarauderEntity* entity = (MarauderEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_Marauder_ApplyDamageModifier,
};

// MarauderEntity's constructor.
MarauderEntity::MarauderEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// MarauderEntity's deconstructor.
MarauderEntity::~MarauderEntity()
{}

// AdvMarauderEntity
// ---------

// AdvMarauderEntity's component offset vtable.
const int AdvMarauderEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(AdvMarauderEntity, c_HealthComponent),
	myoffsetof(AdvMarauderEntity, c_ClientComponent),
	myoffsetof(AdvMarauderEntity, c_ArmorComponent),
	myoffsetof(AdvMarauderEntity, c_KnockbackComponent),
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
	0,
};

// AdvMarauderEntity's Damage message dispatcher.
void h_AdvMarauder_Damage(Entity* _entity, const void*  _data) {
	AdvMarauderEntity* entity = (AdvMarauderEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// AdvMarauderEntity's Heal message dispatcher.
void h_AdvMarauder_Heal(Entity* _entity, const void*  _data) {
	AdvMarauderEntity* entity = (AdvMarauderEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// AdvMarauderEntity's PrepareNetCode message dispatcher.
void h_AdvMarauder_PrepareNetCode(Entity* _entity, const void* ) {
	AdvMarauderEntity* entity = (AdvMarauderEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// AdvMarauderEntity's ApplyDamageModifier message dispatcher.
void h_AdvMarauder_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	AdvMarauderEntity* entity = (AdvMarauderEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_AdvMarauder_ApplyDamageModifier,
};

// AdvMarauderEntity's constructor.
AdvMarauderEntity::AdvMarauderEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// AdvMarauderEntity's deconstructor.
AdvMarauderEntity::~AdvMarauderEntity()
{}

// DragoonEntity
// ---------

// DragoonEntity's component offset vtable.
const int DragoonEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(DragoonEntity, c_HealthComponent),
	myoffsetof(DragoonEntity, c_ClientComponent),
	myoffsetof(DragoonEntity, c_ArmorComponent),
	myoffsetof(DragoonEntity, c_KnockbackComponent),
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
	0,
};

// DragoonEntity's Damage message dispatcher.
void h_Dragoon_Damage(Entity* _entity, const void*  _data) {
	DragoonEntity* entity = (DragoonEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// DragoonEntity's Heal message dispatcher.
void h_Dragoon_Heal(Entity* _entity, const void*  _data) {
	DragoonEntity* entity = (DragoonEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// DragoonEntity's PrepareNetCode message dispatcher.
void h_Dragoon_PrepareNetCode(Entity* _entity, const void* ) {
	DragoonEntity* entity = (DragoonEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// DragoonEntity's ApplyDamageModifier message dispatcher.
void h_Dragoon_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	DragoonEntity* entity = (DragoonEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_Dragoon_ApplyDamageModifier,
};

// DragoonEntity's constructor.
DragoonEntity::DragoonEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// DragoonEntity's deconstructor.
DragoonEntity::~DragoonEntity()
{}

// AdvDragoonEntity
// ---------

// AdvDragoonEntity's component offset vtable.
const int AdvDragoonEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(AdvDragoonEntity, c_HealthComponent),
	myoffsetof(AdvDragoonEntity, c_ClientComponent),
	myoffsetof(AdvDragoonEntity, c_ArmorComponent),
	myoffsetof(AdvDragoonEntity, c_KnockbackComponent),
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
	0,
};

// AdvDragoonEntity's Damage message dispatcher.
void h_AdvDragoon_Damage(Entity* _entity, const void*  _data) {
	AdvDragoonEntity* entity = (AdvDragoonEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// AdvDragoonEntity's Heal message dispatcher.
void h_AdvDragoon_Heal(Entity* _entity, const void*  _data) {
	AdvDragoonEntity* entity = (AdvDragoonEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// AdvDragoonEntity's PrepareNetCode message dispatcher.
void h_AdvDragoon_PrepareNetCode(Entity* _entity, const void* ) {
	AdvDragoonEntity* entity = (AdvDragoonEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// AdvDragoonEntity's ApplyDamageModifier message dispatcher.
void h_AdvDragoon_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	AdvDragoonEntity* entity = (AdvDragoonEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_AdvDragoon_ApplyDamageModifier,
};

// AdvDragoonEntity's constructor.
AdvDragoonEntity::AdvDragoonEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// AdvDragoonEntity's deconstructor.
AdvDragoonEntity::~AdvDragoonEntity()
{}

// TyrantEntity
// ---------

// TyrantEntity's component offset vtable.
const int TyrantEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(TyrantEntity, c_HealthComponent),
	myoffsetof(TyrantEntity, c_ClientComponent),
	myoffsetof(TyrantEntity, c_ArmorComponent),
	myoffsetof(TyrantEntity, c_KnockbackComponent),
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
	0,
};

// TyrantEntity's Damage message dispatcher.
void h_Tyrant_Damage(Entity* _entity, const void*  _data) {
	TyrantEntity* entity = (TyrantEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// TyrantEntity's Heal message dispatcher.
void h_Tyrant_Heal(Entity* _entity, const void*  _data) {
	TyrantEntity* entity = (TyrantEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// TyrantEntity's PrepareNetCode message dispatcher.
void h_Tyrant_PrepareNetCode(Entity* _entity, const void* ) {
	TyrantEntity* entity = (TyrantEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// TyrantEntity's ApplyDamageModifier message dispatcher.
void h_Tyrant_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	TyrantEntity* entity = (TyrantEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_Tyrant_ApplyDamageModifier,
};

// TyrantEntity's constructor.
TyrantEntity::TyrantEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// TyrantEntity's deconstructor.
TyrantEntity::~TyrantEntity()
{}

// GrangerEntity
// ---------

// GrangerEntity's component offset vtable.
const int GrangerEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(GrangerEntity, c_HealthComponent),
	myoffsetof(GrangerEntity, c_ClientComponent),
	myoffsetof(GrangerEntity, c_ArmorComponent),
	myoffsetof(GrangerEntity, c_KnockbackComponent),
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
	0,
};

// GrangerEntity's Damage message dispatcher.
void h_Granger_Damage(Entity* _entity, const void*  _data) {
	GrangerEntity* entity = (GrangerEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// GrangerEntity's Heal message dispatcher.
void h_Granger_Heal(Entity* _entity, const void*  _data) {
	GrangerEntity* entity = (GrangerEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// GrangerEntity's PrepareNetCode message dispatcher.
void h_Granger_PrepareNetCode(Entity* _entity, const void* ) {
	GrangerEntity* entity = (GrangerEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// GrangerEntity's ApplyDamageModifier message dispatcher.
void h_Granger_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	GrangerEntity* entity = (GrangerEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_Granger_ApplyDamageModifier,
};

// GrangerEntity's constructor.
GrangerEntity::GrangerEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// GrangerEntity's deconstructor.
GrangerEntity::~GrangerEntity()
{}

// AdvGrangerEntity
// ---------

// AdvGrangerEntity's component offset vtable.
const int AdvGrangerEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(AdvGrangerEntity, c_HealthComponent),
	myoffsetof(AdvGrangerEntity, c_ClientComponent),
	myoffsetof(AdvGrangerEntity, c_ArmorComponent),
	myoffsetof(AdvGrangerEntity, c_KnockbackComponent),
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
	0,
};

// AdvGrangerEntity's Damage message dispatcher.
void h_AdvGranger_Damage(Entity* _entity, const void*  _data) {
	AdvGrangerEntity* entity = (AdvGrangerEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// AdvGrangerEntity's Heal message dispatcher.
void h_AdvGranger_Heal(Entity* _entity, const void*  _data) {
	AdvGrangerEntity* entity = (AdvGrangerEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// AdvGrangerEntity's PrepareNetCode message dispatcher.
void h_AdvGranger_PrepareNetCode(Entity* _entity, const void* ) {
	AdvGrangerEntity* entity = (AdvGrangerEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// AdvGrangerEntity's ApplyDamageModifier message dispatcher.
void h_AdvGranger_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	AdvGrangerEntity* entity = (AdvGrangerEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_AdvGranger_ApplyDamageModifier,
};

// AdvGrangerEntity's constructor.
AdvGrangerEntity::AdvGrangerEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_AlienClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// AdvGrangerEntity's deconstructor.
AdvGrangerEntity::~AdvGrangerEntity()
{}

// NakedHumanEntity
// ---------

// NakedHumanEntity's component offset vtable.
const int NakedHumanEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(NakedHumanEntity, c_HealthComponent),
	myoffsetof(NakedHumanEntity, c_ClientComponent),
	myoffsetof(NakedHumanEntity, c_ArmorComponent),
	myoffsetof(NakedHumanEntity, c_KnockbackComponent),
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
	0,
};

// NakedHumanEntity's Damage message dispatcher.
void h_NakedHuman_Damage(Entity* _entity, const void*  _data) {
	NakedHumanEntity* entity = (NakedHumanEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// NakedHumanEntity's Heal message dispatcher.
void h_NakedHuman_Heal(Entity* _entity, const void*  _data) {
	NakedHumanEntity* entity = (NakedHumanEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// NakedHumanEntity's PrepareNetCode message dispatcher.
void h_NakedHuman_PrepareNetCode(Entity* _entity, const void* ) {
	NakedHumanEntity* entity = (NakedHumanEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// NakedHumanEntity's ApplyDamageModifier message dispatcher.
void h_NakedHuman_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	NakedHumanEntity* entity = (NakedHumanEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_NakedHuman_ApplyDamageModifier,
};

// NakedHumanEntity's constructor.
NakedHumanEntity::NakedHumanEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_HumanClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// NakedHumanEntity's deconstructor.
NakedHumanEntity::~NakedHumanEntity()
{}

// LightHumanEntity
// ---------

// LightHumanEntity's component offset vtable.
const int LightHumanEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(LightHumanEntity, c_HealthComponent),
	myoffsetof(LightHumanEntity, c_ClientComponent),
	myoffsetof(LightHumanEntity, c_ArmorComponent),
	myoffsetof(LightHumanEntity, c_KnockbackComponent),
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
	0,
};

// LightHumanEntity's Damage message dispatcher.
void h_LightHuman_Damage(Entity* _entity, const void*  _data) {
	LightHumanEntity* entity = (LightHumanEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// LightHumanEntity's Heal message dispatcher.
void h_LightHuman_Heal(Entity* _entity, const void*  _data) {
	LightHumanEntity* entity = (LightHumanEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// LightHumanEntity's PrepareNetCode message dispatcher.
void h_LightHuman_PrepareNetCode(Entity* _entity, const void* ) {
	LightHumanEntity* entity = (LightHumanEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// LightHumanEntity's ApplyDamageModifier message dispatcher.
void h_LightHuman_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	LightHumanEntity* entity = (LightHumanEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_LightHuman_ApplyDamageModifier,
};

// LightHumanEntity's constructor.
LightHumanEntity::LightHumanEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_HumanClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// LightHumanEntity's deconstructor.
LightHumanEntity::~LightHumanEntity()
{}

// MediumHumanEntity
// ---------

// MediumHumanEntity's component offset vtable.
const int MediumHumanEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(MediumHumanEntity, c_HealthComponent),
	myoffsetof(MediumHumanEntity, c_ClientComponent),
	myoffsetof(MediumHumanEntity, c_ArmorComponent),
	myoffsetof(MediumHumanEntity, c_KnockbackComponent),
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
	0,
};

// MediumHumanEntity's Damage message dispatcher.
void h_MediumHuman_Damage(Entity* _entity, const void*  _data) {
	MediumHumanEntity* entity = (MediumHumanEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// MediumHumanEntity's Heal message dispatcher.
void h_MediumHuman_Heal(Entity* _entity, const void*  _data) {
	MediumHumanEntity* entity = (MediumHumanEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// MediumHumanEntity's PrepareNetCode message dispatcher.
void h_MediumHuman_PrepareNetCode(Entity* _entity, const void* ) {
	MediumHumanEntity* entity = (MediumHumanEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// MediumHumanEntity's ApplyDamageModifier message dispatcher.
void h_MediumHuman_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	MediumHumanEntity* entity = (MediumHumanEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_MediumHuman_ApplyDamageModifier,
};

// MediumHumanEntity's constructor.
MediumHumanEntity::MediumHumanEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_HumanClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// MediumHumanEntity's deconstructor.
MediumHumanEntity::~MediumHumanEntity()
{}

// HeavyHumanEntity
// ---------

// HeavyHumanEntity's component offset vtable.
const int HeavyHumanEntity::componentOffsets[] = {
	0,
	0,
	0,
	myoffsetof(HeavyHumanEntity, c_HealthComponent),
	myoffsetof(HeavyHumanEntity, c_ClientComponent),
	myoffsetof(HeavyHumanEntity, c_ArmorComponent),
	myoffsetof(HeavyHumanEntity, c_KnockbackComponent),
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
	0,
};

// HeavyHumanEntity's Damage message dispatcher.
void h_HeavyHuman_Damage(Entity* _entity, const void*  _data) {
	HeavyHumanEntity* entity = (HeavyHumanEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_KnockbackComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// HeavyHumanEntity's Heal message dispatcher.
void h_HeavyHuman_Heal(Entity* _entity, const void*  _data) {
	HeavyHumanEntity* entity = (HeavyHumanEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// HeavyHumanEntity's PrepareNetCode message dispatcher.
void h_HeavyHuman_PrepareNetCode(Entity* _entity, const void* ) {
	HeavyHumanEntity* entity = (HeavyHumanEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
}

// HeavyHumanEntity's ApplyDamageModifier message dispatcher.
void h_HeavyHuman_ApplyDamageModifier(Entity* _entity, const void*  _data) {
	HeavyHumanEntity* entity = (HeavyHumanEntity*) _entity;
	const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float&, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_ArmorComponent.HandleApplyDamageModifier(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data));
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
	h_HeavyHuman_ApplyDamageModifier,
};

// HeavyHumanEntity's constructor.
HeavyHumanEntity::HeavyHumanEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_ClientComponent(*this, params.Client_clientData)
	, c_ArmorComponent(*this)
	, c_KnockbackComponent(*this)
	, c_HumanClassComponent(*this, c_ClientComponent, c_ArmorComponent, c_KnockbackComponent, c_HealthComponent)
{}

// HeavyHumanEntity's deconstructor.
HeavyHumanEntity::~HeavyHumanEntity()
{}

// AcidTubeEntity
// ---------

// AcidTubeEntity's component offset vtable.
const int AcidTubeEntity::componentOffsets[] = {
	myoffsetof(AcidTubeEntity, c_DeferredFreeingComponent),
	myoffsetof(AcidTubeEntity, c_ThinkingComponent),
	myoffsetof(AcidTubeEntity, c_IgnitableComponent),
	myoffsetof(AcidTubeEntity, c_HealthComponent),
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
	0,
};

// AcidTubeEntity's Damage message dispatcher.
void h_AcidTube_Damage(Entity* _entity, const void*  _data) {
	AcidTubeEntity* entity = (AcidTubeEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// AcidTubeEntity's Heal message dispatcher.
void h_AcidTube_Heal(Entity* _entity, const void*  _data) {
	AcidTubeEntity* entity = (AcidTubeEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// AcidTubeEntity's Ignite message dispatcher.
void h_AcidTube_Ignite(Entity* _entity, const void*  _data) {
	AcidTubeEntity* entity = (AcidTubeEntity*) _entity;
	const std::tuple<gentity_t*>* data = (const std::tuple<gentity_t*>*) _data;
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// AcidTubeEntity's Die message dispatcher.
void h_AcidTube_Die(Entity* _entity, const void*  _data) {
	AcidTubeEntity* entity = (AcidTubeEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// AcidTubeEntity's FreeAt message dispatcher.
void h_AcidTube_FreeAt(Entity* _entity, const void*  _data) {
	AcidTubeEntity* entity = (AcidTubeEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// AcidTubeEntity's PrepareNetCode message dispatcher.
void h_AcidTube_PrepareNetCode(Entity* _entity, const void* ) {
	AcidTubeEntity* entity = (AcidTubeEntity*) _entity;
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// AcidTubeEntity's Extinguish message dispatcher.
void h_AcidTube_Extinguish(Entity* _entity, const void*  _data) {
	AcidTubeEntity* entity = (AcidTubeEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
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
};

// AcidTubeEntity's constructor.
AcidTubeEntity::AcidTubeEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_IgnitableComponent)
	, c_AcidTubeComponent(*this, c_AlienBuildableComponent)
{}

// AcidTubeEntity's deconstructor.
AcidTubeEntity::~AcidTubeEntity()
{}

// BarricadeEntity
// ---------

// BarricadeEntity's component offset vtable.
const int BarricadeEntity::componentOffsets[] = {
	myoffsetof(BarricadeEntity, c_DeferredFreeingComponent),
	myoffsetof(BarricadeEntity, c_ThinkingComponent),
	myoffsetof(BarricadeEntity, c_IgnitableComponent),
	myoffsetof(BarricadeEntity, c_HealthComponent),
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
	0,
};

// BarricadeEntity's Damage message dispatcher.
void h_Barricade_Damage(Entity* _entity, const void*  _data) {
	BarricadeEntity* entity = (BarricadeEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_BarricadeComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// BarricadeEntity's Heal message dispatcher.
void h_Barricade_Heal(Entity* _entity, const void*  _data) {
	BarricadeEntity* entity = (BarricadeEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// BarricadeEntity's Ignite message dispatcher.
void h_Barricade_Ignite(Entity* _entity, const void*  _data) {
	BarricadeEntity* entity = (BarricadeEntity*) _entity;
	const std::tuple<gentity_t*>* data = (const std::tuple<gentity_t*>*) _data;
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// BarricadeEntity's Die message dispatcher.
void h_Barricade_Die(Entity* _entity, const void*  _data) {
	BarricadeEntity* entity = (BarricadeEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_BarricadeComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// BarricadeEntity's FreeAt message dispatcher.
void h_Barricade_FreeAt(Entity* _entity, const void*  _data) {
	BarricadeEntity* entity = (BarricadeEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// BarricadeEntity's PrepareNetCode message dispatcher.
void h_Barricade_PrepareNetCode(Entity* _entity, const void* ) {
	BarricadeEntity* entity = (BarricadeEntity*) _entity;
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// BarricadeEntity's Extinguish message dispatcher.
void h_Barricade_Extinguish(Entity* _entity, const void*  _data) {
	BarricadeEntity* entity = (BarricadeEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
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
};

// BarricadeEntity's constructor.
BarricadeEntity::BarricadeEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_IgnitableComponent)
	, c_BarricadeComponent(*this, c_AlienBuildableComponent)
{}

// BarricadeEntity's deconstructor.
BarricadeEntity::~BarricadeEntity()
{}

// BoosterEntity
// ---------

// BoosterEntity's component offset vtable.
const int BoosterEntity::componentOffsets[] = {
	myoffsetof(BoosterEntity, c_DeferredFreeingComponent),
	myoffsetof(BoosterEntity, c_ThinkingComponent),
	myoffsetof(BoosterEntity, c_IgnitableComponent),
	myoffsetof(BoosterEntity, c_HealthComponent),
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
	0,
};

// BoosterEntity's Damage message dispatcher.
void h_Booster_Damage(Entity* _entity, const void*  _data) {
	BoosterEntity* entity = (BoosterEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// BoosterEntity's Heal message dispatcher.
void h_Booster_Heal(Entity* _entity, const void*  _data) {
	BoosterEntity* entity = (BoosterEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// BoosterEntity's Ignite message dispatcher.
void h_Booster_Ignite(Entity* _entity, const void*  _data) {
	BoosterEntity* entity = (BoosterEntity*) _entity;
	const std::tuple<gentity_t*>* data = (const std::tuple<gentity_t*>*) _data;
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// BoosterEntity's Die message dispatcher.
void h_Booster_Die(Entity* _entity, const void*  _data) {
	BoosterEntity* entity = (BoosterEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// BoosterEntity's FreeAt message dispatcher.
void h_Booster_FreeAt(Entity* _entity, const void*  _data) {
	BoosterEntity* entity = (BoosterEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// BoosterEntity's PrepareNetCode message dispatcher.
void h_Booster_PrepareNetCode(Entity* _entity, const void* ) {
	BoosterEntity* entity = (BoosterEntity*) _entity;
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// BoosterEntity's Extinguish message dispatcher.
void h_Booster_Extinguish(Entity* _entity, const void*  _data) {
	BoosterEntity* entity = (BoosterEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
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
};

// BoosterEntity's constructor.
BoosterEntity::BoosterEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_IgnitableComponent)
	, c_BoosterComponent(*this, c_AlienBuildableComponent)
{}

// BoosterEntity's deconstructor.
BoosterEntity::~BoosterEntity()
{}

// EggEntity
// ---------

// EggEntity's component offset vtable.
const int EggEntity::componentOffsets[] = {
	myoffsetof(EggEntity, c_DeferredFreeingComponent),
	myoffsetof(EggEntity, c_ThinkingComponent),
	myoffsetof(EggEntity, c_IgnitableComponent),
	myoffsetof(EggEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	myoffsetof(EggEntity, c_BuildableComponent),
	myoffsetof(EggEntity, c_AlienBuildableComponent),
	0,
	0,
	0,
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
	0,
};

// EggEntity's Damage message dispatcher.
void h_Egg_Damage(Entity* _entity, const void*  _data) {
	EggEntity* entity = (EggEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// EggEntity's Heal message dispatcher.
void h_Egg_Heal(Entity* _entity, const void*  _data) {
	EggEntity* entity = (EggEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// EggEntity's Ignite message dispatcher.
void h_Egg_Ignite(Entity* _entity, const void*  _data) {
	EggEntity* entity = (EggEntity*) _entity;
	const std::tuple<gentity_t*>* data = (const std::tuple<gentity_t*>*) _data;
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// EggEntity's Die message dispatcher.
void h_Egg_Die(Entity* _entity, const void*  _data) {
	EggEntity* entity = (EggEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// EggEntity's FreeAt message dispatcher.
void h_Egg_FreeAt(Entity* _entity, const void*  _data) {
	EggEntity* entity = (EggEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// EggEntity's PrepareNetCode message dispatcher.
void h_Egg_PrepareNetCode(Entity* _entity, const void* ) {
	EggEntity* entity = (EggEntity*) _entity;
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// EggEntity's Extinguish message dispatcher.
void h_Egg_Extinguish(Entity* _entity, const void*  _data) {
	EggEntity* entity = (EggEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
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
};

// EggEntity's constructor.
EggEntity::EggEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_IgnitableComponent)
	, c_EggComponent(*this, c_AlienBuildableComponent)
{}

// EggEntity's deconstructor.
EggEntity::~EggEntity()
{}

// HiveEntity
// ---------

// HiveEntity's component offset vtable.
const int HiveEntity::componentOffsets[] = {
	myoffsetof(HiveEntity, c_DeferredFreeingComponent),
	myoffsetof(HiveEntity, c_ThinkingComponent),
	myoffsetof(HiveEntity, c_IgnitableComponent),
	myoffsetof(HiveEntity, c_HealthComponent),
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
	0,
};

// HiveEntity's Damage message dispatcher.
void h_Hive_Damage(Entity* _entity, const void*  _data) {
	HiveEntity* entity = (HiveEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_HiveComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// HiveEntity's Heal message dispatcher.
void h_Hive_Heal(Entity* _entity, const void*  _data) {
	HiveEntity* entity = (HiveEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// HiveEntity's Ignite message dispatcher.
void h_Hive_Ignite(Entity* _entity, const void*  _data) {
	HiveEntity* entity = (HiveEntity*) _entity;
	const std::tuple<gentity_t*>* data = (const std::tuple<gentity_t*>*) _data;
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// HiveEntity's Die message dispatcher.
void h_Hive_Die(Entity* _entity, const void*  _data) {
	HiveEntity* entity = (HiveEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// HiveEntity's FreeAt message dispatcher.
void h_Hive_FreeAt(Entity* _entity, const void*  _data) {
	HiveEntity* entity = (HiveEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// HiveEntity's PrepareNetCode message dispatcher.
void h_Hive_PrepareNetCode(Entity* _entity, const void* ) {
	HiveEntity* entity = (HiveEntity*) _entity;
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// HiveEntity's Extinguish message dispatcher.
void h_Hive_Extinguish(Entity* _entity, const void*  _data) {
	HiveEntity* entity = (HiveEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
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
};

// HiveEntity's constructor.
HiveEntity::HiveEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_IgnitableComponent)
	, c_HiveComponent(*this, c_AlienBuildableComponent)
{}

// HiveEntity's deconstructor.
HiveEntity::~HiveEntity()
{}

// LeechEntity
// ---------

// LeechEntity's component offset vtable.
const int LeechEntity::componentOffsets[] = {
	myoffsetof(LeechEntity, c_DeferredFreeingComponent),
	myoffsetof(LeechEntity, c_ThinkingComponent),
	myoffsetof(LeechEntity, c_IgnitableComponent),
	myoffsetof(LeechEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	myoffsetof(LeechEntity, c_BuildableComponent),
	myoffsetof(LeechEntity, c_AlienBuildableComponent),
	0,
	myoffsetof(LeechEntity, c_ResourceStorageComponent),
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
	0,
};

// LeechEntity's Damage message dispatcher.
void h_Leech_Damage(Entity* _entity, const void*  _data) {
	LeechEntity* entity = (LeechEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// LeechEntity's Heal message dispatcher.
void h_Leech_Heal(Entity* _entity, const void*  _data) {
	LeechEntity* entity = (LeechEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// LeechEntity's Ignite message dispatcher.
void h_Leech_Ignite(Entity* _entity, const void*  _data) {
	LeechEntity* entity = (LeechEntity*) _entity;
	const std::tuple<gentity_t*>* data = (const std::tuple<gentity_t*>*) _data;
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// LeechEntity's Die message dispatcher.
void h_Leech_Die(Entity* _entity, const void*  _data) {
	LeechEntity* entity = (LeechEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_ResourceStorageComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_MiningComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// LeechEntity's FreeAt message dispatcher.
void h_Leech_FreeAt(Entity* _entity, const void*  _data) {
	LeechEntity* entity = (LeechEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// LeechEntity's PrepareNetCode message dispatcher.
void h_Leech_PrepareNetCode(Entity* _entity, const void* ) {
	LeechEntity* entity = (LeechEntity*) _entity;
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
	entity->c_ResourceStorageComponent.HandlePrepareNetCode();
	entity->c_MiningComponent.HandlePrepareNetCode();
}

// LeechEntity's Extinguish message dispatcher.
void h_Leech_Extinguish(Entity* _entity, const void*  _data) {
	LeechEntity* entity = (LeechEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
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
	nullptr,
};

// LeechEntity's constructor.
LeechEntity::LeechEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_IgnitableComponent)
	, c_ResourceStorageComponent(*this)
	, c_MiningComponent(*this, c_ResourceStorageComponent, c_ThinkingComponent)
	, c_LeechComponent(*this, c_AlienBuildableComponent, c_MiningComponent)
{}

// LeechEntity's deconstructor.
LeechEntity::~LeechEntity()
{}

// OvermindEntity
// ---------

// OvermindEntity's component offset vtable.
const int OvermindEntity::componentOffsets[] = {
	myoffsetof(OvermindEntity, c_DeferredFreeingComponent),
	myoffsetof(OvermindEntity, c_ThinkingComponent),
	myoffsetof(OvermindEntity, c_IgnitableComponent),
	myoffsetof(OvermindEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	myoffsetof(OvermindEntity, c_BuildableComponent),
	myoffsetof(OvermindEntity, c_AlienBuildableComponent),
	0,
	myoffsetof(OvermindEntity, c_ResourceStorageComponent),
	myoffsetof(OvermindEntity, c_MainBuildableComponent),
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
	0,
};

// OvermindEntity's Damage message dispatcher.
void h_Overmind_Damage(Entity* _entity, const void*  _data) {
	OvermindEntity* entity = (OvermindEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// OvermindEntity's Heal message dispatcher.
void h_Overmind_Heal(Entity* _entity, const void*  _data) {
	OvermindEntity* entity = (OvermindEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// OvermindEntity's Ignite message dispatcher.
void h_Overmind_Ignite(Entity* _entity, const void*  _data) {
	OvermindEntity* entity = (OvermindEntity*) _entity;
	const std::tuple<gentity_t*>* data = (const std::tuple<gentity_t*>*) _data;
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// OvermindEntity's Die message dispatcher.
void h_Overmind_Die(Entity* _entity, const void*  _data) {
	OvermindEntity* entity = (OvermindEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_ResourceStorageComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_OvermindComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// OvermindEntity's FreeAt message dispatcher.
void h_Overmind_FreeAt(Entity* _entity, const void*  _data) {
	OvermindEntity* entity = (OvermindEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// OvermindEntity's PrepareNetCode message dispatcher.
void h_Overmind_PrepareNetCode(Entity* _entity, const void* ) {
	OvermindEntity* entity = (OvermindEntity*) _entity;
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
	entity->c_ResourceStorageComponent.HandlePrepareNetCode();
}

// OvermindEntity's Extinguish message dispatcher.
void h_Overmind_Extinguish(Entity* _entity, const void*  _data) {
	OvermindEntity* entity = (OvermindEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
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
	nullptr,
};

// OvermindEntity's constructor.
OvermindEntity::OvermindEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_IgnitableComponent)
	, c_ResourceStorageComponent(*this)
	, c_MainBuildableComponent(*this, c_BuildableComponent, c_ResourceStorageComponent)
	, c_OvermindComponent(*this, c_AlienBuildableComponent, c_MainBuildableComponent)
{}

// OvermindEntity's deconstructor.
OvermindEntity::~OvermindEntity()
{}

// SpikerEntity
// ---------

// SpikerEntity's component offset vtable.
const int SpikerEntity::componentOffsets[] = {
	myoffsetof(SpikerEntity, c_DeferredFreeingComponent),
	myoffsetof(SpikerEntity, c_ThinkingComponent),
	myoffsetof(SpikerEntity, c_IgnitableComponent),
	myoffsetof(SpikerEntity, c_HealthComponent),
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
	0,
};

// SpikerEntity's Damage message dispatcher.
void h_Spiker_Damage(Entity* _entity, const void*  _data) {
	SpikerEntity* entity = (SpikerEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_SpikerComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// SpikerEntity's Heal message dispatcher.
void h_Spiker_Heal(Entity* _entity, const void*  _data) {
	SpikerEntity* entity = (SpikerEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// SpikerEntity's Ignite message dispatcher.
void h_Spiker_Ignite(Entity* _entity, const void*  _data) {
	SpikerEntity* entity = (SpikerEntity*) _entity;
	const std::tuple<gentity_t*>* data = (const std::tuple<gentity_t*>*) _data;
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// SpikerEntity's Die message dispatcher.
void h_Spiker_Die(Entity* _entity, const void*  _data) {
	SpikerEntity* entity = (SpikerEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// SpikerEntity's FreeAt message dispatcher.
void h_Spiker_FreeAt(Entity* _entity, const void*  _data) {
	SpikerEntity* entity = (SpikerEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// SpikerEntity's PrepareNetCode message dispatcher.
void h_Spiker_PrepareNetCode(Entity* _entity, const void* ) {
	SpikerEntity* entity = (SpikerEntity*) _entity;
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// SpikerEntity's Extinguish message dispatcher.
void h_Spiker_Extinguish(Entity* _entity, const void*  _data) {
	SpikerEntity* entity = (SpikerEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
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
};

// SpikerEntity's constructor.
SpikerEntity::SpikerEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_IgnitableComponent)
	, c_SpikerComponent(*this, c_AlienBuildableComponent)
{}

// SpikerEntity's deconstructor.
SpikerEntity::~SpikerEntity()
{}

// TrapperEntity
// ---------

// TrapperEntity's component offset vtable.
const int TrapperEntity::componentOffsets[] = {
	myoffsetof(TrapperEntity, c_DeferredFreeingComponent),
	myoffsetof(TrapperEntity, c_ThinkingComponent),
	myoffsetof(TrapperEntity, c_IgnitableComponent),
	myoffsetof(TrapperEntity, c_HealthComponent),
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
	0,
};

// TrapperEntity's Damage message dispatcher.
void h_Trapper_Damage(Entity* _entity, const void*  _data) {
	TrapperEntity* entity = (TrapperEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
	entity->c_AlienBuildableComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// TrapperEntity's Heal message dispatcher.
void h_Trapper_Heal(Entity* _entity, const void*  _data) {
	TrapperEntity* entity = (TrapperEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// TrapperEntity's Ignite message dispatcher.
void h_Trapper_Ignite(Entity* _entity, const void*  _data) {
	TrapperEntity* entity = (TrapperEntity*) _entity;
	const std::tuple<gentity_t*>* data = (const std::tuple<gentity_t*>*) _data;
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// TrapperEntity's Die message dispatcher.
void h_Trapper_Die(Entity* _entity, const void*  _data) {
	TrapperEntity* entity = (TrapperEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_AlienBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// TrapperEntity's FreeAt message dispatcher.
void h_Trapper_FreeAt(Entity* _entity, const void*  _data) {
	TrapperEntity* entity = (TrapperEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// TrapperEntity's PrepareNetCode message dispatcher.
void h_Trapper_PrepareNetCode(Entity* _entity, const void* ) {
	TrapperEntity* entity = (TrapperEntity*) _entity;
	entity->c_IgnitableComponent.HandlePrepareNetCode();
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// TrapperEntity's Extinguish message dispatcher.
void h_Trapper_Extinguish(Entity* _entity, const void*  _data) {
	TrapperEntity* entity = (TrapperEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_IgnitableComponent.HandleExtinguish(std::get<0>(*data));
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
};

// TrapperEntity's constructor.
TrapperEntity::TrapperEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_IgnitableComponent(*this, false, c_ThinkingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_AlienBuildableComponent(*this, c_BuildableComponent, c_IgnitableComponent)
	, c_TrapperComponent(*this, c_AlienBuildableComponent)
{}

// TrapperEntity's deconstructor.
TrapperEntity::~TrapperEntity()
{}

// ArmouryEntity
// ---------

// ArmouryEntity's component offset vtable.
const int ArmouryEntity::componentOffsets[] = {
	myoffsetof(ArmouryEntity, c_DeferredFreeingComponent),
	myoffsetof(ArmouryEntity, c_ThinkingComponent),
	0,
	myoffsetof(ArmouryEntity, c_HealthComponent),
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
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// ArmouryEntity's Damage message dispatcher.
void h_Armoury_Damage(Entity* _entity, const void*  _data) {
	ArmouryEntity* entity = (ArmouryEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// ArmouryEntity's Heal message dispatcher.
void h_Armoury_Heal(Entity* _entity, const void*  _data) {
	ArmouryEntity* entity = (ArmouryEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// ArmouryEntity's Die message dispatcher.
void h_Armoury_Die(Entity* _entity, const void*  _data) {
	ArmouryEntity* entity = (ArmouryEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// ArmouryEntity's FreeAt message dispatcher.
void h_Armoury_FreeAt(Entity* _entity, const void*  _data) {
	ArmouryEntity* entity = (ArmouryEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// ArmouryEntity's PrepareNetCode message dispatcher.
void h_Armoury_PrepareNetCode(Entity* _entity, const void* ) {
	ArmouryEntity* entity = (ArmouryEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
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
};

// ArmouryEntity's constructor.
ArmouryEntity::ArmouryEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent)
{}

// ArmouryEntity's deconstructor.
ArmouryEntity::~ArmouryEntity()
{}

// DrillEntity
// ---------

// DrillEntity's component offset vtable.
const int DrillEntity::componentOffsets[] = {
	myoffsetof(DrillEntity, c_DeferredFreeingComponent),
	myoffsetof(DrillEntity, c_ThinkingComponent),
	0,
	myoffsetof(DrillEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	myoffsetof(DrillEntity, c_BuildableComponent),
	0,
	myoffsetof(DrillEntity, c_HumanBuildableComponent),
	myoffsetof(DrillEntity, c_ResourceStorageComponent),
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
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

// DrillEntity's Damage message dispatcher.
void h_Drill_Damage(Entity* _entity, const void*  _data) {
	DrillEntity* entity = (DrillEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// DrillEntity's Heal message dispatcher.
void h_Drill_Heal(Entity* _entity, const void*  _data) {
	DrillEntity* entity = (DrillEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// DrillEntity's Die message dispatcher.
void h_Drill_Die(Entity* _entity, const void*  _data) {
	DrillEntity* entity = (DrillEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_ResourceStorageComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_MiningComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// DrillEntity's FreeAt message dispatcher.
void h_Drill_FreeAt(Entity* _entity, const void*  _data) {
	DrillEntity* entity = (DrillEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// DrillEntity's PrepareNetCode message dispatcher.
void h_Drill_PrepareNetCode(Entity* _entity, const void* ) {
	DrillEntity* entity = (DrillEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
	entity->c_ResourceStorageComponent.HandlePrepareNetCode();
	entity->c_MiningComponent.HandlePrepareNetCode();
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
	nullptr,
};

// DrillEntity's constructor.
DrillEntity::DrillEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent)
	, c_ResourceStorageComponent(*this)
	, c_MiningComponent(*this, c_ResourceStorageComponent, c_ThinkingComponent)
{}

// DrillEntity's deconstructor.
DrillEntity::~DrillEntity()
{}

// MedipadEntity
// ---------

// MedipadEntity's component offset vtable.
const int MedipadEntity::componentOffsets[] = {
	myoffsetof(MedipadEntity, c_DeferredFreeingComponent),
	myoffsetof(MedipadEntity, c_ThinkingComponent),
	0,
	myoffsetof(MedipadEntity, c_HealthComponent),
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
	0,
};

// MedipadEntity's Damage message dispatcher.
void h_Medipad_Damage(Entity* _entity, const void*  _data) {
	MedipadEntity* entity = (MedipadEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// MedipadEntity's Heal message dispatcher.
void h_Medipad_Heal(Entity* _entity, const void*  _data) {
	MedipadEntity* entity = (MedipadEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// MedipadEntity's Die message dispatcher.
void h_Medipad_Die(Entity* _entity, const void*  _data) {
	MedipadEntity* entity = (MedipadEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_MedipadComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// MedipadEntity's FreeAt message dispatcher.
void h_Medipad_FreeAt(Entity* _entity, const void*  _data) {
	MedipadEntity* entity = (MedipadEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// MedipadEntity's PrepareNetCode message dispatcher.
void h_Medipad_PrepareNetCode(Entity* _entity, const void* ) {
	MedipadEntity* entity = (MedipadEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
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
};

// MedipadEntity's constructor.
MedipadEntity::MedipadEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent)
	, c_MedipadComponent(*this, c_HumanBuildableComponent)
{}

// MedipadEntity's deconstructor.
MedipadEntity::~MedipadEntity()
{}

// MGTurretEntity
// ---------

// MGTurretEntity's component offset vtable.
const int MGTurretEntity::componentOffsets[] = {
	myoffsetof(MGTurretEntity, c_DeferredFreeingComponent),
	myoffsetof(MGTurretEntity, c_ThinkingComponent),
	0,
	myoffsetof(MGTurretEntity, c_HealthComponent),
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
	0,
	0,
	0,
	0,
	0,
};

// MGTurretEntity's Damage message dispatcher.
void h_MGTurret_Damage(Entity* _entity, const void*  _data) {
	MGTurretEntity* entity = (MGTurretEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// MGTurretEntity's Heal message dispatcher.
void h_MGTurret_Heal(Entity* _entity, const void*  _data) {
	MGTurretEntity* entity = (MGTurretEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// MGTurretEntity's Die message dispatcher.
void h_MGTurret_Die(Entity* _entity, const void*  _data) {
	MGTurretEntity* entity = (MGTurretEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_TurretComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// MGTurretEntity's FreeAt message dispatcher.
void h_MGTurret_FreeAt(Entity* _entity, const void*  _data) {
	MGTurretEntity* entity = (MGTurretEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// MGTurretEntity's PrepareNetCode message dispatcher.
void h_MGTurret_PrepareNetCode(Entity* _entity, const void* ) {
	MGTurretEntity* entity = (MGTurretEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
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
};

// MGTurretEntity's constructor.
MGTurretEntity::MGTurretEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent)
	, c_TurretComponent(*this)
{}

// MGTurretEntity's deconstructor.
MGTurretEntity::~MGTurretEntity()
{}

// ReactorEntity
// ---------

// ReactorEntity's component offset vtable.
const int ReactorEntity::componentOffsets[] = {
	myoffsetof(ReactorEntity, c_DeferredFreeingComponent),
	myoffsetof(ReactorEntity, c_ThinkingComponent),
	0,
	myoffsetof(ReactorEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	myoffsetof(ReactorEntity, c_BuildableComponent),
	0,
	myoffsetof(ReactorEntity, c_HumanBuildableComponent),
	myoffsetof(ReactorEntity, c_ResourceStorageComponent),
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
	myoffsetof(ReactorEntity, c_ReactorComponent),
	0,
	0,
	0,
};

// ReactorEntity's Damage message dispatcher.
void h_Reactor_Damage(Entity* _entity, const void*  _data) {
	ReactorEntity* entity = (ReactorEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// ReactorEntity's Heal message dispatcher.
void h_Reactor_Heal(Entity* _entity, const void*  _data) {
	ReactorEntity* entity = (ReactorEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// ReactorEntity's Die message dispatcher.
void h_Reactor_Die(Entity* _entity, const void*  _data) {
	ReactorEntity* entity = (ReactorEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_ResourceStorageComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_ReactorComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// ReactorEntity's FreeAt message dispatcher.
void h_Reactor_FreeAt(Entity* _entity, const void*  _data) {
	ReactorEntity* entity = (ReactorEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// ReactorEntity's PrepareNetCode message dispatcher.
void h_Reactor_PrepareNetCode(Entity* _entity, const void* ) {
	ReactorEntity* entity = (ReactorEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
	entity->c_ResourceStorageComponent.HandlePrepareNetCode();
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
	nullptr,
};

// ReactorEntity's constructor.
ReactorEntity::ReactorEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent)
	, c_ResourceStorageComponent(*this)
	, c_MainBuildableComponent(*this, c_BuildableComponent, c_ResourceStorageComponent)
	, c_ReactorComponent(*this, c_HumanBuildableComponent, c_MainBuildableComponent)
{}

// ReactorEntity's deconstructor.
ReactorEntity::~ReactorEntity()
{}

// RepeaterEntity
// ---------

// RepeaterEntity's component offset vtable.
const int RepeaterEntity::componentOffsets[] = {
	myoffsetof(RepeaterEntity, c_DeferredFreeingComponent),
	myoffsetof(RepeaterEntity, c_ThinkingComponent),
	0,
	myoffsetof(RepeaterEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	myoffsetof(RepeaterEntity, c_BuildableComponent),
	0,
	myoffsetof(RepeaterEntity, c_HumanBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
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

// RepeaterEntity's Damage message dispatcher.
void h_Repeater_Damage(Entity* _entity, const void*  _data) {
	RepeaterEntity* entity = (RepeaterEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// RepeaterEntity's Heal message dispatcher.
void h_Repeater_Heal(Entity* _entity, const void*  _data) {
	RepeaterEntity* entity = (RepeaterEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// RepeaterEntity's Die message dispatcher.
void h_Repeater_Die(Entity* _entity, const void*  _data) {
	RepeaterEntity* entity = (RepeaterEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// RepeaterEntity's FreeAt message dispatcher.
void h_Repeater_FreeAt(Entity* _entity, const void*  _data) {
	RepeaterEntity* entity = (RepeaterEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// RepeaterEntity's PrepareNetCode message dispatcher.
void h_Repeater_PrepareNetCode(Entity* _entity, const void* ) {
	RepeaterEntity* entity = (RepeaterEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
}

// RepeaterEntity's message dispatcher vtable.
const MessageHandler RepeaterEntity::messageHandlers[] = {
	h_Repeater_PrepareNetCode,
	h_Repeater_FreeAt,
	nullptr,
	nullptr,
	h_Repeater_Heal,
	h_Repeater_Damage,
	h_Repeater_Die,
	nullptr,
};

// RepeaterEntity's constructor.
RepeaterEntity::RepeaterEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent)
{}

// RepeaterEntity's deconstructor.
RepeaterEntity::~RepeaterEntity()
{}

// RocketpodEntity
// ---------

// RocketpodEntity's component offset vtable.
const int RocketpodEntity::componentOffsets[] = {
	myoffsetof(RocketpodEntity, c_DeferredFreeingComponent),
	myoffsetof(RocketpodEntity, c_ThinkingComponent),
	0,
	myoffsetof(RocketpodEntity, c_HealthComponent),
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
	0,
	0,
	0,
};

// RocketpodEntity's Damage message dispatcher.
void h_Rocketpod_Damage(Entity* _entity, const void*  _data) {
	RocketpodEntity* entity = (RocketpodEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// RocketpodEntity's Heal message dispatcher.
void h_Rocketpod_Heal(Entity* _entity, const void*  _data) {
	RocketpodEntity* entity = (RocketpodEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// RocketpodEntity's Die message dispatcher.
void h_Rocketpod_Die(Entity* _entity, const void*  _data) {
	RocketpodEntity* entity = (RocketpodEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_TurretComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// RocketpodEntity's FreeAt message dispatcher.
void h_Rocketpod_FreeAt(Entity* _entity, const void*  _data) {
	RocketpodEntity* entity = (RocketpodEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// RocketpodEntity's PrepareNetCode message dispatcher.
void h_Rocketpod_PrepareNetCode(Entity* _entity, const void* ) {
	RocketpodEntity* entity = (RocketpodEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
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
};

// RocketpodEntity's constructor.
RocketpodEntity::RocketpodEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent)
	, c_TurretComponent(*this)
{}

// RocketpodEntity's deconstructor.
RocketpodEntity::~RocketpodEntity()
{}

// TelenodeEntity
// ---------

// TelenodeEntity's component offset vtable.
const int TelenodeEntity::componentOffsets[] = {
	myoffsetof(TelenodeEntity, c_DeferredFreeingComponent),
	myoffsetof(TelenodeEntity, c_ThinkingComponent),
	0,
	myoffsetof(TelenodeEntity, c_HealthComponent),
	0,
	0,
	0,
	0,
	0,
	myoffsetof(TelenodeEntity, c_BuildableComponent),
	0,
	myoffsetof(TelenodeEntity, c_HumanBuildableComponent),
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
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

// TelenodeEntity's Damage message dispatcher.
void h_Telenode_Damage(Entity* _entity, const void*  _data) {
	TelenodeEntity* entity = (TelenodeEntity*) _entity;
	const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>* data = (const std::tuple<float, gentity_t*, Util::optional<Vec3>, Util::optional<Vec3>, int, meansOfDeath_t>*) _data;
	entity->c_HealthComponent.HandleDamage(std::get<0>(*data), std::get<1>(*data), std::get<2>(*data), std::get<3>(*data), std::get<4>(*data), std::get<5>(*data));
}

// TelenodeEntity's Heal message dispatcher.
void h_Telenode_Heal(Entity* _entity, const void*  _data) {
	TelenodeEntity* entity = (TelenodeEntity*) _entity;
	const std::tuple<float, gentity_t*>* data = (const std::tuple<float, gentity_t*>*) _data;
	entity->c_HealthComponent.HandleHeal(std::get<0>(*data), std::get<1>(*data));
}

// TelenodeEntity's Die message dispatcher.
void h_Telenode_Die(Entity* _entity, const void*  _data) {
	TelenodeEntity* entity = (TelenodeEntity*) _entity;
	const std::tuple<gentity_t*, meansOfDeath_t>* data = (const std::tuple<gentity_t*, meansOfDeath_t>*) _data;
	entity->c_BuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
	entity->c_HumanBuildableComponent.HandleDie(std::get<0>(*data), std::get<1>(*data));
}

// TelenodeEntity's FreeAt message dispatcher.
void h_Telenode_FreeAt(Entity* _entity, const void*  _data) {
	TelenodeEntity* entity = (TelenodeEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// TelenodeEntity's PrepareNetCode message dispatcher.
void h_Telenode_PrepareNetCode(Entity* _entity, const void* ) {
	TelenodeEntity* entity = (TelenodeEntity*) _entity;
	entity->c_HealthComponent.HandlePrepareNetCode();
	entity->c_BuildableComponent.HandlePrepareNetCode();
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
};

// TelenodeEntity's constructor.
TelenodeEntity::TelenodeEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_HealthComponent(*this, params.Health_maxHealth)
	, c_BuildableComponent(*this, c_HealthComponent, c_ThinkingComponent)
	, c_HumanBuildableComponent(*this, c_BuildableComponent)
{}

// TelenodeEntity's deconstructor.
TelenodeEntity::~TelenodeEntity()
{}

// FireEntity
// ---------

// FireEntity's component offset vtable.
const int FireEntity::componentOffsets[] = {
	myoffsetof(FireEntity, c_DeferredFreeingComponent),
	myoffsetof(FireEntity, c_ThinkingComponent),
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

// FireEntity's FreeAt message dispatcher.
void h_Fire_FreeAt(Entity* _entity, const void*  _data) {
	FireEntity* entity = (FireEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
	entity->c_DeferredFreeingComponent.HandleFreeAt(std::get<0>(*data));
}

// FireEntity's Ignite message dispatcher.
void h_Fire_Ignite(Entity* _entity, const void*  _data) {
	FireEntity* entity = (FireEntity*) _entity;
	const std::tuple<gentity_t*>* data = (const std::tuple<gentity_t*>*) _data;
	entity->c_IgnitableComponent.HandleIgnite(std::get<0>(*data));
}

// FireEntity's PrepareNetCode message dispatcher.
void h_Fire_PrepareNetCode(Entity* _entity, const void* ) {
	FireEntity* entity = (FireEntity*) _entity;
	entity->c_IgnitableComponent.HandlePrepareNetCode();
}

// FireEntity's Extinguish message dispatcher.
void h_Fire_Extinguish(Entity* _entity, const void*  _data) {
	FireEntity* entity = (FireEntity*) _entity;
	const std::tuple<int>* data = (const std::tuple<int>*) _data;
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
};

// FireEntity's constructor.
FireEntity::FireEntity(Params params)
	: Entity(messageHandlers, componentOffsets, params.oldEnt)
	, c_DeferredFreeingComponent(*this)
	, c_ThinkingComponent(*this, c_DeferredFreeingComponent)
	, c_IgnitableComponent(*this, true, c_ThinkingComponent)
{}

// FireEntity's deconstructor.
FireEntity::~FireEntity()
{}

#undef myoffsetof
