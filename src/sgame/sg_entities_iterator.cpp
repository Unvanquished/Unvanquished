#include "sg_entities_iterator.h"

// generic

static bool entity_in_use(const gentity_t *ent) {
	return ent->inuse;
}
const entity_iterator iterate_entities(entity_in_use);
