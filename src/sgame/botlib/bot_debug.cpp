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

#include "engine/server/sg_msgdef.h"
#include "shared/VMMain.h"
#include "DetourDebugDraw.h"
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "DebugDraw.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include "bot_local.h"
#include "engine/botlib/bot_debug.h"
#include "bot_navdraw.h"

void DebugDrawQuake::init()
{
	commands = {};
}

void DebugDrawQuake::sendCommands()
{
	commands.Write<debugDrawCommand_t>(debugDrawCommand_t::EOC);
	VM::SendMsg<BotDebugDrawMsg>(commands.GetData());
}

void DebugDrawQuake::depthMask(bool state)
{
	commands.Write<debugDrawCommand_t>(debugDrawCommand_t::DEPTHMASK);
	commands.Write<bool>(state);
}

void DebugDrawQuake::begin(duDebugDrawPrimitives prim, float s)
{
	commands.Write<debugDrawCommand_t>(debugDrawCommand_t::BEGIN);
	commands.Write<debugDrawMode_t>(static_cast<debugDrawMode_t>(prim));
	commands.Write<float>(s);
}

void DebugDrawQuake::vertex(const float* pos, unsigned int c)
{
	vertex( pos[0], pos[1], pos[2], c );
}

void DebugDrawQuake::vertex(const float x, const float y, const float z, unsigned int color)
{
	Vec3 vert{ x, y, z };
	recast2quake( vert.Data() );
	commands.Write<debugDrawCommand_t>(debugDrawCommand_t::VERTEX);
	commands.Write<Vec3>(vert);
	commands.Write<unsigned int>(color);
}

void DebugDrawQuake::vertex(const float *pos, unsigned int color, const float* uv)
{
	vertex(pos[0], pos[1], pos[2], color, uv[0], uv[1]);
}

void DebugDrawQuake::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
	Vec3 vert{ x, y, z };
	Vec2 uv{ u, v };
	recast2quake( vert.Data() );
	commands.Write<debugDrawCommand_t>(debugDrawCommand_t::VERTEX_UV);
	commands.Write<Vec3>(vert);
	commands.Write<unsigned int>(color);
	commands.Write<Vec2>(uv);
}

void DebugDrawQuake::end()
{
	commands.Write<debugDrawCommand_t>(debugDrawCommand_t::END);
}
