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

#ifndef BOTLIB_NAVDRAW_H_
#define BOTLIB_NAVDRAW_H_

#include "DetourDebugDraw.h"
#include "DebugDraw.h"
#include "bot_local.h"

class DebugDrawQuake final : public duDebugDraw
{
	Util::Writer commands;

	// Machinery for converting quads to triangles
	int quadCounter;
	glm::vec3 quadFirstVertex;
	unsigned int quadFirstColor;

public:
	void init();
	void sendCommands();

	void depthMask(bool state) override;
	void texture(bool) override;
	void begin(duDebugDrawPrimitives prim, float size = 1.0f) override;
	void vertex(const float* pos, unsigned int color) override;
	void vertex(const float x, const float y, const float z, unsigned int color) override;
	void vertex(const float* pos, unsigned int color, const float* uv) override;
	void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v) override;
	void end() override;
};

#endif
