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
#include "g_local.h"
#include "../../engine/botlib/bot_types.h"

//g_bot.cpp
void BotDPrintf( const char* fmt, ... ) __attribute__( ( format( printf, 1, 2 ) ) );
gentity_t* BotFindClosestEnemy( gentity_t *self );
gentity_t* BotFindBestEnemy( gentity_t *self );
void BotFindClosestBuildings( gentity_t *self, botEntityAndDistance_t *closest );
gentity_t* BotFindBuilding( gentity_t *self, int buildingType, int range );
void BotPain( gentity_t *self, gentity_t *attacker, int damage );
void BotGetIdealAimLocation( gentity_t *self, botTarget_t target, vec3_t aimLocation );
void BotSlowAim( gentity_t *self, vec3_t target, float slow );
void BotShakeAim( gentity_t *self, vec3_t rVec );
void BotAimAtLocation( gentity_t *self, vec3_t target );
float BotAimNegligence( gentity_t *self, botTarget_t target );

void BotSetTarget( botTarget_t *target, gentity_t *ent, vec3_t *pos );
qboolean BotChangeGoalEntity( gentity_t *self, gentity_t *goal );
qboolean BotTargetIsEntity( botTarget_t target );
qboolean BotTargetIsPlayer( botTarget_t target );
int BotGetTargetEntityNumber( botTarget_t target );
void BotGetTargetPos( botTarget_t target, vec3_t rVec );
team_t BotGetEntityTeam( gentity_t *ent );
team_t BotGetTargetTeam( botTarget_t target );
int BotGetTargetType( botTarget_t target );
qboolean BotChangeGoal( gentity_t *self, botTarget_t target );
qboolean BotChangeGoalEntity( gentity_t *self, gentity_t *goal );
qboolean BotChangeGoalPos( gentity_t *self, vec3_t goal );

qboolean BotTargetIsVisible( gentity_t *self, botTarget_t target, int mask );

qboolean BotTargetInAttackRange( gentity_t *self, botTarget_t target );
int FindBots( int *botEntityNumbers, int maxBots, team_t team );

//g_humanbot.cpp
qboolean WeaponIsEmpty( weapon_t weapon, playerState_t ps );
float PercentAmmoRemaining( weapon_t weapon, playerState_t *ps );
gentity_t* BotFindDamagedFriendlyStructure( gentity_t *self );
qboolean BotGetBuildingToBuild( gentity_t *self, vec3_t origin, vec3_t normal, buildable_t *building );

//g_alienbot.cpp
float CalcPounceAimPitch( gentity_t *self, botTarget_t target );
float CalcBarbAimPitch( gentity_t *self, botTarget_t target );

//g_nav.cpp
void BotFindRandomPointOnMesh( gentity_t *self, vec3_t point );
qboolean BotPathIsWalkable( gentity_t *self, botTarget_t target );
qboolean BotMoveToGoal( gentity_t *self );
void BotSetNavmesh( gentity_t  *ent, class_t newClass );

typedef enum
{
	MOVE_FORWARD = BIT( 0 ),
	MOVE_BACKWARD = BIT( 1 ),
	MOVE_LEFT = BIT( 2 ),
	MOVE_RIGHT = BIT( 3 )
} botMoveDir_t;

qboolean BotDodge( gentity_t *self );
qboolean BotSprint( gentity_t *self, qboolean enable );
qboolean BotJump( gentity_t *self );
void BotStrafeDodge( gentity_t *self );
void BotAlternateStrafe( gentity_t *self );
void BotMoveInDir( gentity_t *self, uint32_t moveDir );
void BotStandStill( gentity_t *self );

unsigned int FindRouteToTarget( gentity_t *self, botTarget_t target );
int DistanceToGoal( gentity_t *self );
int DistanceToGoalSquared( gentity_t *self );
int BotGetStrafeDirection( void );
void PlantEntityOnGround( gentity_t *ent, vec3_t groundPos );
void G_BotNavInit( void );
void G_BotNavCleanup( void );

extern qboolean navMeshLoaded;

//configureable constants
//For a reference of how far a number represents, take a look at tremulous.h

//how long our traces are for obstacle avoidence
#define BOT_OBSTACLE_AVOID_RANGE 5.0f

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
	E_NONE,
	E_A_SPAWN,
	E_A_OVERMIND,
	E_A_BARRICADE,
	E_A_ACIDTUBE,
	E_A_TRAPPER,
	E_A_BOOSTER,
	E_A_HIVE,
	E_A_LEECH,
	E_H_SPAWN,
	E_H_MGTURRET,
	E_H_TESLAGEN,
	E_H_ARMOURY,
	E_H_DCC,
	E_H_MEDISTAT,
	E_H_REACTOR,
	E_H_REPEATER,
	E_H_DRILL,
	E_GOAL,
	E_ENEMY,
	E_DAMAGEDBUILDING
} AIEntity_t;

typedef enum
{
	STATUS_FAILURE = 0,
	STATUS_SUCCESS,
	STATUS_RUNNING
} AINodeStatus_t;

struct AIGenericNode_s;

typedef AINodeStatus_t ( *AINodeRunner )( gentity_t *, struct AIGenericNode_s * );

// all nodes must conform to this interface
typedef struct AIGenericNode_s
{
	AINode_t type;
	AINodeRunner run;
} AIGenericNode_t;

#define MAX_NODE_LIST 20
typedef struct
{
	AINode_t type;
	AINodeRunner run;
	AIGenericNode_t *list[ MAX_NODE_LIST ];
	int numNodes;
	int maxFail;
} AINodeList_t;

typedef struct
{
	AINode_t type;
	AINodeRunner run;
	weapon_t weapon;
	upgrade_t upgrades[ 3 ];
	int numUpgrades;
} AIBuyNode_t;

typedef struct
{
	AINode_t type;
	AINodeRunner run;
	AIEntity_t ent;
	float range;
} AIMoveToNode_t;

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
	OP_EQUAL,
	OP_NEQUAL,
	OP_NOT,
	OP_BOOL
} AIConditionOperator_t;

typedef struct
{
	AINode_t type;
	AINodeRunner run;
	AIGenericNode_t *child;
	AIConditionOperator_t op;
	AIConditionInfo_t info;
	AIConditionParameter_t param1;
	AIConditionParameter_t param2;
} AIConditionNode_t;

typedef struct
{
	AIBehaviorTree_t **trees;
	int numTrees;
	int maxTrees;
} AITreeList_t;

AINodeStatus_t BotConditionNode( gentity_t *self, AIGenericNode_t *node );

AINodeStatus_t BotSelectorNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotSequenceNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotPriorityNode( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotParallelNode( gentity_t *self, AIGenericNode_t *node );

AINodeStatus_t BotActionFight( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionBuy( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionRepair( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionEvolve ( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionHealH( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionHealA( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionHeal( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionFlee( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionRoam( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionMoveTo( gentity_t *self, AIGenericNode_t *node );
AINodeStatus_t BotActionRush( gentity_t *self, AIGenericNode_t *node );
#endif
