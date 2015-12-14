#include "SpectatorClassComponent.h"

SpectatorClassComponent::SpectatorClassComponent(Entity& entity, ClientComponent& r_ClientComponent)
	: SpectatorClassComponentBase(entity, r_ClientComponent)
{}

void SpectatorClassComponent::HandlePrepareNetCode() {
	gclient_t* cl = entity.oldEnt->client;

	if (cl && cl->sess.spectatorState == SPECTATOR_FOLLOW) {
		int clientNum = cl->sess.spectatorClient;

		if ( clientNum >= 0 && clientNum < level.maxclients )
		{
			gclient_t* other = &level.clients[ clientNum ];

			if ( cl->pers.connected == CON_CONNECTED )
			{
				// Save
				int score = cl->ps.persistant[ PERS_SCORE ];
				int ping = cl->ps.ping;

				// Copy
				cl->ps = other->ps;

				// Restore
				cl->ps.persistant[ PERS_SCORE ] = score;
				cl->ps.ping = ping;

				cl->ps.pm_flags |= PMF_FOLLOW;
				cl->ps.pm_flags &= ~PMF_QUEUED;
			}
		}
	}
}
