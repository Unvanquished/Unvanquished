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

#ifndef __BOT_HEADER
#define __BOT_HEADER

#define UNNAMED_BOT "[bot] Bot"

struct botEntityAndDistance_t
{
	gentity_t *ent;
	float distance;
};

struct botTarget_t
{
	gentity_t *ent;
	vec3_t coord;
	bool inuse;
};

#define MAX_ENEMY_QUEUE 32
struct enemyQueueElement_t
{
	gentity_t *ent;
	int        timeFound;
};

struct enemyQueue_t
{
	enemyQueueElement_t enemys[ MAX_ENEMY_QUEUE ];
	int front;
	int back;
};

struct botSkill_t
{
	int level;
	float aimSlowness;
	float aimShake;
};

#include "sg_bot_ai.h"
#define MAX_NODE_DEPTH 20

struct botMemory_t
{
	enemyQueue_t enemyQueue;
	int enemyLastSeen;

	//team the bot is on when added
	team_t botTeam;

	botTarget_t goal;

	botSkill_t botSkill;
	botEntityAndDistance_t bestEnemy;
	botEntityAndDistance_t closestDamagedBuilding;
	botEntityAndDistance_t closestBuildings[ BA_NUM_BUILDABLES ];

	AIBehaviorTree_t *behaviorTree;
	AIGenericNode_t  *currentNode;
	AIGenericNode_t  *runningNodes[ MAX_NODE_DEPTH ];
	int              numRunningNodes;

	int         futureAimTime;
	int         futureAimTimeInterval;
	vec3_t      futureAim;
	usercmd_t   cmdBuffer;
	botNavCmd_t nav;

	int lastThink;
	int stuckTime;
	vec3_t stuckPosition;
};

constexpr int BOT_DEFAULT_SKILL = 5;
const char BOT_DEFAULT_BEHAVIOR[] = "default";
const char BOT_NAME_FROM_LIST[] = "*";

bool G_BotAdd( const char *name, team_t team, int skill, const char *behavior, bool filler = false );
bool G_BotSetDefaults( int clientNum, team_t team, int skill, const char* behavior );
void     G_BotDel( int clientNum );
void     G_BotDelAllBots();
void     G_BotThink( gentity_t *self );
void     G_BotSpectatorThink( gentity_t *self );
void     G_BotIntermissionThink( gclient_t *client );
void     G_BotListNames( gentity_t *ent );
bool G_BotClearNames();
int      G_BotAddNames(team_t team, int arg, int last);
void     G_BotDisableArea( vec3_t origin, vec3_t mins, vec3_t maxs );
void     G_BotEnableArea( vec3_t origin, vec3_t mins, vec3_t maxs );
void     G_BotInit();
void     G_BotCleanup();
void G_BotFill( bool immediately );
#endif
