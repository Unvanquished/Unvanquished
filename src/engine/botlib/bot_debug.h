#ifndef BOT_DEBUG_H
#define BOT_DEBUG_H
typedef enum
{
	D_DRAW_POINTS,
	D_DRAW_LINES,
	D_DRAW_TRIS,
	D_DRAW_QUADS
} debugDrawMode_t;

typedef struct
{
	void ( *DebugDrawBegin ) ( debugDrawMode_t mode, float size );
	void ( *DebugDrawDepthMask )( qboolean state );
	void ( *DebugDrawVertex ) ( const vec3_t pos, unsigned int color,const vec2_t uv );
	void ( *DebugDrawEnd ) ();
} BotDebugInterface_t;

void     BotDebugDrawMesh(BotDebugInterface_t *in);
#endif