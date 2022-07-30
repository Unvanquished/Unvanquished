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

#ifndef BOTLIB_API_H_
#define BOTLIB_API_H_

#include "bot_types.h"
#include "sgame/sg_bot_local.h"

void BotInit();
void BotDebugDrawMesh();
void Cmd_NavEdit( gentity_t *ent );
void Cmd_AddConnection( gentity_t *ent );
void Cmd_NavTest( gentity_t *ent );

void         BotDisableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
void         BotEnableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
bool     BotNavTrace( int botClientNum, botTrace_t *trace, const vec3_t start, const vec3_t end );

void BotFindRandomPoint( int botClientNum, vec3_t point );
bool BotFindRandomPointInRadius( int botClientNum, const vec3_t origin, vec3_t point, float radius );

// be careful: those two function give incorrect behavior if an area is disabled more than once: a node disabled twice will be enabled back on the first try
// TODO: decide if we want to use G_BotAddObstacle instead
void G_BotDisableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
void G_BotEnableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );

#endif
