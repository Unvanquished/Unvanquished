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

#ifndef BOTLIB_TYPE_H_
#define BOTLIB_TYPE_H_

#include <glm/vec3.hpp>

struct gentity_t;
struct gclient_t;

struct botClass_t
{
	char name[ 64 ];
	unsigned short polyFlagsInclude;
	unsigned short polyFlagsExclude;
};

struct botTrace_t
{
	float frac;
	float normal[ 3 ];
};

// parameters outputted by navigation
// if they are followed exactly, the bot will not go off the nav mesh
struct botNavCmd_t
{
	float    pos[ 3 ]; // self position (with a height diff though)
	float    tpos[ 3 ]; // position of the target (as for pos, with a height diff I guess)
	float    dir[ 3 ]; // normalized direction of the target
	int      directPathToGoal;
	int      havePath;
	glm::vec3 glm_dir()  const { return glm::vec3( dir[0] , dir[1] , dir[2] ); };
	glm::vec3 glm_pos()  const { return glm::vec3( pos[0] , pos[1] , pos[2] ); };
	glm::vec3 glm_tpos() const { return glm::vec3( tpos[0], tpos[1], tpos[2] ); };
};

enum class botRouteTargetType_t
{
	BOT_TARGET_STATIC, // target stays in one place always
	BOT_TARGET_DYNAMIC // target can move
};

// type: determines if the object can move or not
// pos: the object's position
// polyExtents: how far away from pos to search for a nearby navmesh polygon for finding a route
struct botRouteTarget_t
{
	botRouteTargetType_t type;
	float pos[ 3 ];
	float polyExtents[ 3 ];
	void setPos( glm::vec3 const& p ){ pos[0] = p[0]; pos[1] = p[1]; pos[2] = p[2]; }
};

//route status flags
#define ROUTE_FAILED  ( 1u << 31 )
#define	ROUTE_SUCCEED ( 1u << 30 )
#define	ROUTE_PARTIAL ( 1 << 6 )

#endif
