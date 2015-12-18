#include "SpectatorClassComponent.h"

SpectatorClassComponent::SpectatorClassComponent(Entity& entity, ClientComponent& r_ClientComponent)
	: SpectatorClassComponentBase(entity, r_ClientComponent)
{}

void SpectatorClassComponent::HandlePrepareNetCode() {
	gclient_t* cl = entity.oldEnt->client;

	if (!cl
		|| cl->sess.spectatorState != SPECTATOR_FOLLOW
		|| cl->sess.spectatorClient < 0
		|| cl->sess.spectatorClient >= level.maxclients
		|| level.clients[cl->sess.spectatorClient].pers.connected != CON_CONNECTED) {
		return;
	}

	// Save
	int score = cl->ps.persistant[PERS_SCORE];
	int ping = cl->ps.ping;

	// Copy
	cl->ps = level.clients[cl->sess.spectatorClient].ps;

	// Restore
	cl->ps.persistant[PERS_SCORE] = score;
	cl->ps.ping = ping;

	cl->ps.pm_flags |= PMF_FOLLOW;
	cl->ps.pm_flags &= ~PMF_QUEUED;
}
