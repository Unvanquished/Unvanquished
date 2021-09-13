/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2013 Daemon Developers
All rights reserved.

This file is part of the Daemon BSD Source Code (Daemon Source Code).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS

===========================================================================
*/

#include "bot_types.h"
#include "sgame/sg_typedef.h"

void BotInit();
bool     BotSetupNav( const botClass_t *botClass, qhandle_t *navHandle );
void         BotShutdownNav();
void BotDebugDrawMesh();
void Cmd_NavEdit( gentity_t *ent );
void Cmd_AddConnection( gentity_t *ent );
// the difference with Cmd_AddConnection is that, instead of getting coords
// aimed at, the connection point is placed at feets, which can be more precise
// and easier depending on situation.
void Cmd_PutCon_f( gentity_t *ent );
void Cmd_NavTest( gentity_t *ent );

void         BotDisableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
void         BotEnableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
void         BotSetNavMesh( int botClientNum, qhandle_t nav );
bool     BotFindRouteExt( int botClientNum, const botRouteTarget_t *target, bool allowPartial );
void         BotUpdateCorridor( int botClientNum, const botRouteTarget_t *target, botNavCmd_t *cmd );
void         BotFindRandomPoint( int botClientNum, vec3_t point );
bool     BotFindRandomPointInRadius( int botClientNum, const vec3_t origin, vec3_t point, float radius );
bool     BotNavTrace( int botClientNum, botTrace_t *trace, const vec3_t start, const vec3_t end );
void         BotAddObstacle( const vec3_t mins, const vec3_t maxs, qhandle_t *obstacleHandle );
void         BotRemoveObstacle( qhandle_t obstacleHandle );
void         BotUpdateObstacles();
