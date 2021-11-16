#include "sg_entities_iterator.h"

// generic

static bool entity_in_use(const gentity_t *ent) {
	return ent->inuse;
}
const entity_iterator iterate_entities(entity_in_use);


// clients

static bool entity_is_bot(const gentity_t *ent) {
	return ent->inuse && (ent->r.svFlags & SVF_BOT);
}

const client_entity_iterator iterate_client_entities(entity_in_use);
const client_entity_iterator iterate_bot_entities(entity_is_bot);


// non-clients

static bool entity_is_buildable(const gentity_t *ent) {
	return ent->inuse && ent->s.eType == entityType_t::ET_BUILDABLE;
}

const non_client_entity_iterator iterate_non_client_entities(entity_in_use);
const non_client_entity_iterator iterate_buildable_entities(entity_is_buildable);
