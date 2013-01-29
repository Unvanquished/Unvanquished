#include "bot_types.h"
#ifdef __cplusplus
extern "C"
{
#endif
qboolean     BotSetupNav( const botClass_t *botClass, qhandle_t *navHandle );
void         BotShutdownNav( void );

void         BotDisableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
void         BotEnableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
void         BotSetNavMesh( int botClientNum, qhandle_t nav );
unsigned int BotFindRouteExt( int botClientNum, const vec3_t target );
void         BotUpdateCorridor( int botClientNum, vec3_t *corners, int *numCorners, int maxCorners, const vec3_t target );
void         BotFindRandomPoint( int botClientNum, vec3_t point );
qboolean     BotNavTrace( int botClientNum, botTrace_t *trace, const vec3_t start, const vec3_t end );
#ifdef __cplusplus
}
#endif