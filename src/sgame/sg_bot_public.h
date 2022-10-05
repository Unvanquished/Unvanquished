/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

/*
======================
src/sgame/sg_bot_public.h

This file contains the public interface used by sgame to interact with bots.
======================
*/

#ifndef BOT_PUBLIC_H_
#define BOT_PUBLIC_H_

#include "sg_local.h"
#include "shared/bg_public.h"

struct botMemory_t;

#define UNNAMED_BOT "[bot] Bot"

bool G_BotAdd( const char *name, team_t team, int skill, const char *behavior, bool filler = false );
int  G_BotGetSkill( int clientNum );
const char *G_BotGetBehavior( int clientNum );
void G_BotChangeBehavior( int clientNum, Str::StringRef behavior );
bool G_BotSetBehavior( botMemory_t *botMind, Str::StringRef behavior );
bool G_BotSetDefaults( int clientNum, team_t team, int skill, Str::StringRef behavior );
void G_BotDel( int clientNum );
void G_BotDelAllBots();
void G_BotThink( gentity_t *self );
void G_BotSpectatorThink( gentity_t *self );
void G_BotIntermissionThink( gclient_t *client );
void G_BotListNames( gentity_t *ent );
bool G_BotClearNames();
int  G_BotAddNames(team_t team, int arg, int last);
//TODO Area enabling API is very likely untested, plus, it only enables WALK or disable all
//     so my advice here is: DO NOT USE THAT until a better one is provided.
//     Since I'm the only one touching that kind of things anyway, I don't think this is a
//     useful warning, but who knows...
void G_BotDisableArea( const glm::vec3 &origin, const glm::vec3 &mins, const glm::vec3 &maxs );
void G_BotEnableArea( const glm::vec3 &origin, const glm::vec3 &mins, const glm::vec3 &maxs );
void G_BotAddObstacle( const vec3_t mins, const vec3_t maxs, qhandle_t *handle );
void G_BotRemoveObstacle( qhandle_t handle );
void G_BotUpdateObstacles();
bool G_BotInit();
void G_BotCleanup();
void G_BotFill( bool immediately );
void G_BotRemoveObstacle( qhandle_t handle );
void G_BotUpdateObstacles();

const char BOT_DEFAULT_BEHAVIOR[] = "default";
const char BOT_NAME_FROM_LIST[] = "*";

#endif
