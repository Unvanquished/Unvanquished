#include "ClientComponent.h"

ClientComponent::ClientComponent(Entity& entity, gclient_t* clientData, TeamComponent& r_TeamComponent)
	: ClientComponentBase(entity, clientData, r_TeamComponent)
{}
