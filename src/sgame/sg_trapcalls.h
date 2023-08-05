/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef SG_TRAPCALLS_H_
#define SG_TRAPCALLS_H_

#include <glm/vec3.hpp>

#include "shared/CommonProxies.h"

struct gentity_t;

void             trap_LocateGameData( int numGEntities, int sizeofGEntity_t, int sizeofGClient );
void             trap_DropClient( int clientNum, const char *reason );
void             trap_SendServerCommand( int clientNum, const char *text );
void             trap_SetConfigstring( int num, const char *string );
void             trap_LinkEntity( gentity_t *ent );
void             trap_UnlinkEntity( gentity_t *ent );
int              trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *list, int maxcount );
bool         trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
void             trap_Trace( trace_t *results, const glm::vec3& start, const glm::vec3& mins, const glm::vec3& maxs, const glm::vec3& end, int passEntityNum, int contentmask , int skipmask);
void             trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask , int skipmask);
void             trap_SetBrushModel( gentity_t *ent, const char *name );
bool         trap_InPVS( const vec3_t p1, const vec3_t p2 );
bool         trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );
void             trap_SetConfigstringRestrictions( int num, const clientList_t *clientList );
void             trap_GetConfigstring( int num, char *buffer, int bufferSize );
void             trap_SetUserinfo( int num, const char *buffer );
void             trap_GetUserinfo( int num, char *buffer, int bufferSize );
void             trap_GetServerinfo( char *buffer, int bufferSize );
void             trap_AdjustAreaPortalState( gentity_t *ent, bool open );
int              trap_BotAllocateClient();
void             trap_BotFreeClient( int clientNum );
void             trap_GetUsercmd( int clientNum, usercmd_t *cmd );
bool         trap_GetEntityToken( char *buffer, int bufferSize );
int              trap_BotGetServerCommand( int clientNum, char *message, int size );

int              trap_RSA_GenerateMessage( const char *public_key, char *cleartext, char *encrypted );

void             trap_GenFingerprint( const char *pubkey, int size, char *buffer, int bufsize );
void             trap_GetPlayerPubkey( int clientNum, char *pubkey, int size );

void             trap_GetTimeString( char *buffer, int size, const char *format, const qtime_t *tm );

#endif // SG_TRAPCALLS_H_
