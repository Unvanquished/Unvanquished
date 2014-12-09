#ifndef	__BG_DEMOS_H
#define	__BG_DEMOS_H

#include "../../../engine/qcommon/q_shared.h"
#include "../bg_public.h"

void demoEvaluateTrajectory( const trajectory_t *tr, int atTime, float atTimeFraction, vec3_t result );
void demoCommandValue( const char *cmd, float * oldVal );

typedef struct {
	fileHandle_t fileHandle;
	int line;
	int fileSize, filePos;
	int depth;
} BG_XMLParse_t;

typedef struct BG_XMLParseBlock_s {
	char *tagName;
	qboolean (*openHandler)(BG_XMLParse_t *,const struct BG_XMLParseBlock_s *, void *);
	qboolean (*textHandler)(BG_XMLParse_t *,const char *, void *);
} BG_XMLParseBlock_t;

qboolean BG_XMLError(BG_XMLParse_t *parse, const char *msg, ... );
void BG_XMLWarning(BG_XMLParse_t *parse, const char *msg, ... );
qboolean BG_XMLOpen( BG_XMLParse_t *parse, const char *fileName );
qboolean BG_XMLParse( BG_XMLParse_t *parse, const BG_XMLParseBlock_t *fromBlock, const BG_XMLParseBlock_t *parseBlock, void *data );

#endif
