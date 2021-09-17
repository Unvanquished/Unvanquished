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
src/sgame/sg_bot_local.h

This file contains the headers of the internal functions used by bot only.
======================
*/

#ifndef BOT_LOCAL_H_
#define BOT_LOCAL_H_

#include "sg_local.h"

struct botEntityAndDistance_t
{
	gentity_t const *ent;
	float distance;
};

class botTarget_t
{
public:
	botTarget_t& operator=(const gentity_t *ent);
	botTarget_t& operator=(const vec3_t pos);
	void clear();
	entityType_t getTargetType() const;
	bool isValid() const;
	// checks if the target is a position
	bool targetsCoordinates() const;
	// checks if target is an entity, isn't recycled yet
	// and is still alive
	bool targetsValidEntity() const;
	// note you should use "targetsValidEntity" along with this
	const gentity_t *getTargetedEntity() const;
	// note if you don't check with "isValid" first, you may
	// have garbage as a result
	void getPos(vec3_t out) const;
private:
	GentityConstRef ent;
	vec3_t coord;
	enum class targetType { EMPTY, COORDS, ENTITY } type;
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

#define MAX_NODE_DEPTH 20
struct AIBehaviorTree_t;
struct AIGenericNode_t;
struct botMemory_t
{
	enemyQueue_t enemyQueue;
	int enemyLastSeen;

	//team the bot is on when added
	team_t botTeam;

	botTarget_t goal;
	void willSprint( bool enable );
	void doSprint( int jumpCost, int stamina, usercmd_t& cmd );
	usercmd_t   cmdBuffer;

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
	botNavCmd_t nav;

	int lastThink;
	int stuckTime;
	vec3_t stuckPosition;

	int spawnTime;
	//avoid relying on buttons to remember what AI was doing
	bool wantSprinting = false;
	bool exhausted = false;
};

bool G_BotSetupNav( const botClass_t *botClass, qhandle_t *navHandle );
void G_BotShutdownNav();
void G_BotSetNavMesh( int botClientNum, qhandle_t navHandle );
bool G_BotFindRoute( int botClientNum, const botRouteTarget_t *target, bool allowPartial );
void G_BotUpdatePath( int botClientNum, const botRouteTarget_t *target, botNavCmd_t *cmd );
bool G_BotNavTrace( int botClientNum, botTrace_t *botTrace, const vec3_t start, const vec3_t end );

#endif
