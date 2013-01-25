/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//g_bot.h - declarations of bot specific functions
//#include "g_local.h"
#ifndef __BOT_HEADER
#define __BOT_HEADER
#ifdef __cplusplus
#include "../../libs/detour/DetourNavMeshQuery.h"
#include "../../libs/detour/DetourPathCorridor.h"

//g_bot.cpp
void BotDPrintf(const char* fmt, ... ) __attribute__(( format( printf, 1, 2 ) ));
gentity_t* BotFindClosestEnemy( gentity_t *self );
gentity_t* BotFindBestEnemy( gentity_t *self );
void BotFindClosestBuildings(gentity_t *self, botEntityAndDistance_t *closest);
gentity_t* BotFindBuilding(gentity_t *self, int buildingType, int range);
gentity_t* BotFindBuilding(gentity_t *self, int buildingType);

void BotGetIdealAimLocation(gentity_t *self, botTarget_t target, vec3_t aimLocation);
void BotSlowAim( gentity_t *self, vec3_t target, float slow);
void BotShakeAim( gentity_t *self, vec3_t rVec );
void BotAimAtLocation( gentity_t *self, vec3_t target );
float BotAimNegligence(gentity_t *self, botTarget_t target);

void BotSetTarget(botTarget_t *target, gentity_t *ent, vec3_t *pos);
void BotSetGoal(gentity_t *self, gentity_t *ent, vec3_t *pos);

qboolean BotTargetIsVisible( gentity_t *self, botTarget_t target, int mask );

qboolean BotTargetInAttackRange(gentity_t *self, botTarget_t target);
int FindBots(int *botEntityNumbers, int maxBots, team_t team);

//g_humanbot.cpp
qboolean WeaponIsEmpty( weapon_t weapon, playerState_t ps);
float PercentAmmoRemaining( weapon_t weapon, playerState_t *ps );
gentity_t* BotFindDamagedFriendlyStructure( gentity_t *self );
qboolean BotGetBuildingToBuild(gentity_t *self, vec3_t origin, vec3_t normal, buildable_t *building);

//g_alienbot.cpp
float CalcPounceAimPitch(gentity_t *self, botTarget_t target);
float CalcBarbAimPitch(gentity_t *self, botTarget_t target);
bool G_RoomForClassChange( gentity_t *ent, class_t classt, vec3_t newOrigin );

//g_nav.cpp
qboolean BotPathIsWalkable(gentity_t *self, botTarget_t target);
void UpdatePathCorridor(gentity_t *self);
qboolean BotMoveToGoal( gentity_t *self );
void BotDodge(gentity_t *self, usercmd_t *botCmdBuffer);
int	FindRouteToTarget( gentity_t *self, botTarget_t target );
int DistanceToGoal(gentity_t *self);
int DistanceToGoalSquared(gentity_t *self);
int BotGetStrafeDirection(void);
void PlantEntityOnGround(gentity_t *ent, vec3_t groundPos);
void G_BotNavInit( void );
void G_BotNavCleanup( void );

extern dtNavMeshQuery* navQuerys[PCL_NUM_CLASSES];
extern dtQueryFilter navFilters[PCL_NUM_CLASSES];
extern dtPathCorridor pathCorridor[MAX_CLIENTS];
extern qboolean navMeshLoaded;

//coordinate conversion
static inline void quake2recast(vec3_t vec) {
	vec_t temp = vec[1];
	vec[0] = -vec[0];
	vec[1] = vec[2];
	vec[2] = -temp;
}

static inline void recast2quake(vec3_t vec) {
	vec_t temp = vec[1];
	vec[0] = -vec[0];
	vec[1] = -vec[2];
	vec[2] = temp;
}

//botTarget_t helpers
static inline bool BotTargetIsEntity(botTarget_t target) {
	return (target.ent && target.ent->inuse);
}

static inline bool BotTargetIsPlayer(botTarget_t target) {
	return (target.ent && target.ent->inuse && target.ent->client);
}

static inline int BotGetTargetEntityNumber(botTarget_t target) {
	if(BotTargetIsEntity(target)) 
		return target.ent->s.number;
	else
		return ENTITYNUM_NONE;
}

static inline void BotGetTargetPos(botTarget_t target, vec3_t rVec) {
	if(BotTargetIsEntity(target))
		VectorCopy( target.ent->s.origin, rVec);
	else
		VectorCopy(target.coord, rVec);
}

static inline team_t BotGetTeam(gentity_t *ent) {
	if(!ent)
		return TEAM_NONE;
	if(ent->client) {
		return (team_t)ent->client->ps.stats[STAT_TEAM];
	} else if(ent->s.eType == ET_BUILDABLE) {
		return ent->buildableTeam;
	} else {
		return TEAM_NONE;
	}
}

static inline team_t BotGetTeam( botTarget_t target) {
	if(BotTargetIsEntity(target)) {
		return BotGetTeam(target.ent);
	} else
		return TEAM_NONE;
}

static inline int BotGetTargetType( botTarget_t target) {
	if(BotTargetIsEntity(target))
		return target.ent->s.eType;
	else
		return -1;
}

static inline bool BotChangeTarget(gentity_t *self, gentity_t *target, vec3_t *pos) {
	BotSetGoal(self, target, pos);
	if( !self->botMind->goal.inuse )
		return false;

	if( FindRouteToTarget(self, self->botMind->goal) & (ROUTE_PARTIAL|ROUTE_FAILURE) ) {
		return false;
	}
	return true;
}

static inline bool BotChangeTarget(gentity_t *self, botTarget_t target) {
	if( !target.inuse )
		return false;

	if( FindRouteToTarget(self, target) & (ROUTE_PARTIAL | ROUTE_FAILURE)) {
		return false;
	}
	self->botMind->goal = target;
	return true;
}

//configureable constants
//For a reference of how far a number represents, take a look at tremulous.h

//how long our traces are for obstacle avoidence
#define BOT_OBSTACLE_AVOID_RANGE 50.0f

//how far off can our aim can be from true in order to try to hit the enemy
#define BOT_AIM_NEGLIGENCE 30.0f

//How long in milliseconds the bots will chase an enemy if he goes out of their sight (humans) or radar (aliens)
#define BOT_ENEMY_CHASETIME 5000

//How close does the enemy have to be for the bot to engage him/her when doing a task other than fight/roam
#define BOT_ENGAGE_DIST 200.0f

//How long in milliseconds it takes the bot to react upon seeing an enemy
#define BOT_REACTION_TIME 500

//at what hp do we use medkit?
#define BOT_USEMEDKIT_HP 50

//when human bots reach this ammo percentage left or less(and no enemy), they will head back to the base to refuel ammo
#define BOT_LOW_AMMO 0.50f

//used for clamping distance to heal structure when deciding whether to go heal
#define MAX_HEAL_DIST 2000.0f

//how far away we can be before we stop going forward when fighting an alien
#define MAX_HUMAN_DANCE_DIST 300.0f

//how far away we can be before we try to go around an alien when fighting an alien
#define MIN_HUMAN_DANCE_DIST 100.0f

//how may polys ahead we will look at for invalid path polygons
#define MAX_PATH_LOOK_AHEAD 5

typedef enum
{
	STATUS_FAILURE = 0,
	STATUS_SUCCESS,
	STATUS_RUNNING
} AINodeStatus_t;

typedef enum
{
	ACTION_BUY,
	ACTION_EVOLVE,
	ACTION_FIGHT,
	ACTION_RUSH,
	ACTION_ROAM,
	ACTION_FLEE,
	ACTION_HEAL,
	ACTION_REPAIR,
	ACTION_BUILD,
	ACTION_MOVETO
} AIAction_t;

typedef enum
{
	SELECTOR_PRIORITY,
	SELECTOR_SEQUENCE,
	SELECTOR_SELECTOR,
	SELECTOR_CONCURRENT
} AISelector_t;

typedef enum
{
	E_NONE,
	E_A_SPAWN,
	E_A_OVERMIND,
	E_A_BARRICADE,
	E_A_ACIDTUBE,
	E_A_TRAPPER,
	E_A_BOOSTER,
	E_A_HIVE,
	E_H_SPAWN,
	E_H_MGTURRET,
	E_H_TESLAGEN,
	E_H_ARMOURY,
	E_H_DCC,
	E_H_MEDISTAT,
	E_H_REACTOR,
	E_H_REPEATER,
	E_GOAL,
	E_ENEMY,
	E_DAMAGEDBUILDING
} AIEntity_t;

#define MAX_NODE_LIST 20

typedef struct
{
	AINode_t type;
	AIAction_t action;
} AIActionNode_t;

typedef struct
{
	AINode_t type;
	AISelector_t selector;
	AINode_t *list[ MAX_NODE_LIST ];
	int numNodes;
	int maxFail;
} AINodeList_t;

typedef struct
{
	AINode_t type;
	AIAction_t action;
	weapon_t weapon;
	upgrade_t upgrades[ 3 ];
	int numUpgrades;
} AIBuy_t;

typedef struct
{
	AINode_t type;
	AIAction_t action;
	AIEntity_t ent;
	float range;
} AIMoveTo_t;

typedef enum
{
	CON_BUILDING,
	CON_DAMAGEDBUILDING,
	CON_CVAR,
	CON_WEAPON,
	CON_UPGRADE,
	CON_CURWEAPON,
	CON_TEAM,
	CON_ENEMY,
	CON_HEALTH,
	CON_AMMO,
	CON_TEAM_WEAPON,
	CON_DISTANCE,
	CON_BASERUSH,
	CON_HEAL
} AIConditionInfo_t;

typedef enum
{
	VALUE_FLOAT,
	VALUE_INT,
	VALUE_PTR
} AIConditionValue_t;

typedef struct
{
	AIConditionValue_t type;
	union
	{
		float f;
		int   i;
		void *ptr;
	} value;
} AIConditionParameter_t;

typedef enum
{
	OP_NONE,
	OP_LESSTHAN,
	OP_LESSTHANEQUAL,
	OP_GREATERTHAN,
	OP_GREATERTHANEQUAL,
	OP_NOT,
	OP_EQUAL,
	OP_NEQUAL,
	OP_BOOL
} AIConditionOperator_t;

typedef struct
{
	AINode_t type;
	AINode_t *child;
	AIConditionOperator_t op;
	AIConditionInfo_t info;
	AIConditionParameter_t param1;
	AIConditionParameter_t param2;
} AICondition_t;

typedef struct
{
	AIBehaviorTree_t **trees;
	int numTrees;
	int maxTrees;
} AITreeList_t;

AINodeStatus_t BotActionBuy( gentity_t *self, AIBuy_t *node );
AINodeStatus_t BotActionHealH( gentity_t *self, AIActionNode_t *node );
AINodeStatus_t BotActionRepair( gentity_t *self, AIActionNode_t *node );
AINodeStatus_t BotActionEvolve ( gentity_t *self, AIActionNode_t *node );
AINodeStatus_t BotActionHealA( gentity_t *self, AIActionNode_t *node );
#endif

#endif

