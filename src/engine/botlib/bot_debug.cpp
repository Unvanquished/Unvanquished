extern "C" {
#include "../client/client.h"
}
#include "../../libs/detour/DetourDebugDraw.h"
#include "../../libs/detour/DebugDraw.h"
#include "bot_local.h"
#include "nav.h"
#include "bot_debug.h"

class DebugDrawQuake : public duDebugDraw
{
	duDebugDrawPrimitives mode;
	float size;
	int numFilled;
	BotDebugInterface_t *re;
public:
	void init(BotDebugInterface_t *in);
	void depthMask(bool state);
	void texture(bool state);
	void begin(duDebugDrawPrimitives prim, float size = 1.0f);
	void vertex(const float* pos, unsigned int color);
	void vertex(const float x, const float y, const float z, unsigned int color);
	void vertex(const float* pos, unsigned int color, const float* uv);
	void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v);
	void end();
};

void DebugDrawQuake::init(BotDebugInterface_t *ref) {
	re = ref;
	numFilled = 0;
}

void DebugDrawQuake::depthMask(bool state) { re->DebugDrawDepthMask((qboolean)(int)state);}

void DebugDrawQuake::texture(bool state) {};

void DebugDrawQuake::begin(duDebugDrawPrimitives prim, float s)
{
	re->DebugDrawBegin((debugDrawMode_t)prim, s);
}

void DebugDrawQuake::vertex(const float* pos, unsigned int c)
{
	//Draw();
	vertex(pos, c, NULL);
}

void DebugDrawQuake::vertex(const float x, const float y, const float z, unsigned int color)
{
	vec3_t vert;
	VectorSet(vert, x, y, z);
	vertex(vert,color, NULL);
}

void DebugDrawQuake::vertex(const float *pos, unsigned int color, const float* uv)
{

	vec3_t vert;
	VectorCopy(pos, vert);
	recast2quake(vert);
	re->DebugDrawVertex(vert, color, uv);
}

void DebugDrawQuake::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
	vec3_t vert;
	vec2_t uv;
	uv[0] = u;
	uv[1] = v;
	VectorSet(vert, x, y, z);
	vertex(vert, color, uv);
}

void DebugDrawQuake::end()
{
	re->DebugDrawEnd();
}

void BotDebugDrawMesh( BotDebugInterface_t *in )
{
	int navMeshNum = cl.snap.ps.stats[ 5 ] - 1;

	if ( navMeshNum < 0 || navMeshNum > numNavData )
	{
		return;
	}

	NavData_t *nav = &BotNavData[ navMeshNum ];

	if ( !nav->mesh ) 
	{
		return;
	}

	DebugDrawQuake dd;
	dd.init( in );
	duDebugDrawNavMeshWithClosedList(&dd, *nav->mesh, *nav->query, DU_DRAWNAVMESH_OFFMESHCONS | DU_DRAWNAVMESH_CLOSEDLIST);
}