#include "common/Common.h"
#include "sg_local.h"

namespace Shape {

static gentity_t *SpawnBase(const glm::vec3 &origin, int lifetime)
{
	gentity_t *ent;

	ent = G_NewEntity(NO_CBSE);
	ent->s.eType = entityType_t::ET_SHAPE;
	ent->classname = "shape";

	ent->think = G_FreeEntity;
	ent->nextthink = level.time + lifetime;

	VectorCopy(origin, ent->r.currentOrigin);
	VectorCopy(origin, ent->s.origin);

	trap_LinkEntity(ent);

	return ent;
}

void SpawnLine(const glm::vec3 &start, const glm::vec3 &end, const glm::vec4 &color, int lifetime)
{
	gentity_t *ent = SpawnBase(start, lifetime);

	VectorCopy(end, ent->s.origin2);

	ent->s.angles[0] = color.x;
	ent->s.angles[1] = color.y;
	ent->s.angles[2] = color.z;
	ent->s.angles2[0] = color.w;
}

}
