#include "HumanClassComponent.h"

HumanClassComponent::HumanClassComponent(Entity& entity, ClientComponent& r_ClientComponent, ArmorComponent& r_ArmorComponent, KnockbackComponent& r_KnockbackComponent, HealthComponent& r_HealthComponent)
	: HumanClassComponentBase(entity, r_ClientComponent, r_ArmorComponent, r_KnockbackComponent, r_HealthComponent)
{}
