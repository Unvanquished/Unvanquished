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
#include "../../../../libs/detour/DetourNavMeshQuery.h"
#include "../../../../libs/detour/DetourPathCorridor.h"

typedef enum {
	TASK_STOPPED = 0,
	TASK_RUNNING
} botTaskStatus_t;

typedef enum {
	MODUS_STOPPED = 0,
	MODUS_RUNNING
} botModusStatus_t;

//g_bot.cpp
void BotDPrintf(const char* fmt, ... );
gentity_t* BotFindClosestEnemy( gentity_t *self );
gentity_t* BotFindBestEnemy( gentity_t *self );
void BotFindClosestBuildings(gentity_t *self, botClosestBuildings_t *closest);
gentity_t* BotFindBuilding(gentity_t *self, int buildingType, int range);
gentity_t* BotFindBuilding(gentity_t *self, int buildingType);

void BotGetIdealAimLocation(gentity_t *self, botTarget_t target, vec3_t aimLocation);
void BotSlowAim( gentity_t *self, vec3_t target, float slow);
void BotShakeAim( gentity_t *self, vec3_t rVec );
void BotAimAtLocation( gentity_t *self, vec3_t target , usercmd_t *rAngles);
float BotAimNegligence(gentity_t *self, botTarget_t target);

void BotSetTarget(botTarget_t *target, gentity_t *ent, vec3_t *pos);
void BotSetGoal(gentity_t *self, gentity_t *ent, vec3_t *pos);

qboolean BotTargetIsVisible( gentity_t *self, botTarget_t target, int mask );

qboolean BotTargetInAttackRange(gentity_t *self, botTarget_t target);
botModusStatus_t BotAttackModus(gentity_t *self, usercmd_t *botCmdBuffer);
botModusStatus_t BotBuildModus(gentity_t *self, usercmd_t *botCmdBuffer);
botTaskStatus_t BotTaskBuild(gentity_t *self, usercmd_t *botCmdBuffer);
botTaskStatus_t BotTaskFight(gentity_t *self, usercmd_t *botCmdBuffer);
botTaskStatus_t BotTaskHeal(gentity_t *self, usercmd_t *botCmdBuffer);
botTaskStatus_t BotTaskRetreat(gentity_t *self, usercmd_t *botCmdBuffer);
botTaskStatus_t BotTaskRush(gentity_t *self, usercmd_t *botCmdBuffer);
botTaskStatus_t BotTaskRoam(gentity_t *self, usercmd_t *botCmdBuffer);
qboolean BotTeamateIsInModus(gentity_t *self, botModus_t modus);
int FindBots(int *botEntityNumbers, int maxBots, team_t team);

//g_humanbot.cpp
qboolean WeaponIsEmpty( weapon_t weapon, playerState_t ps);
float PercentAmmoRemaining( weapon_t weapon, int stats[], playerState_t ps );
gentity_t* BotFindDamagedFriendlyStructure( gentity_t *self );
qboolean BotNeedsItem(gentity_t *self);
qboolean BotCanShop(gentity_t *self);
qboolean BotStructureIsDamaged(team_t team);
qboolean buildableIsDamaged(gentity_t *building);
qboolean BotGetBuildingToBuild(gentity_t *self, vec3_t origin, buildable_t *building);
botTaskStatus_t BotTaskBuildH(gentity_t *self, usercmd_t *botCmdBuffer);
botTaskStatus_t BotTaskBuy(gentity_t *self, usercmd_t *botCmdBuffer);
botTaskStatus_t BotTaskBuy(gentity_t *self, weapon_t weapon, upgrade_t *upgrades,int numUpgrades, usercmd_t *botCmdBuffer);
botTaskStatus_t BotTaskHealH(gentity_t *self, usercmd_t *botCmdBuffer);
botTaskStatus_t BotTaskRepair(gentity_t *self, usercmd_t *botCmdBuffer);

//g_alienbot.cpp
float CalcPounceAimPitch(gentity_t *self, botTarget_t target);
float CalcBarbAimPitch(gentity_t *self, botTarget_t target);
botTaskStatus_t BotTaskEvolve( gentity_t *self , usercmd_t *botCmdBuffer);
bool G_RoomForClassChange( gentity_t *ent, class_t classt, vec3_t newOrigin );
botTaskStatus_t BotTaskBuildA(gentity_t *self, usercmd_t *botCmdBuffer);
botTaskStatus_t BotTaskHealA(gentity_t *self, usercmd_t *botCmdBuffer);

//g_nav.cpp
qboolean BotFindNearestPoly( gentity_t *self, gentity_t *ent, dtPolyRef *nearestPoly, vec3_t nearPoint);
qboolean BotNav_Trace(dtNavMeshQuery *navQuery, dtQueryFilter* navFilter, vec3_t start, vec3_t end, float *hit, vec3_t normal, dtPolyRef *pathPolys, int *numHit, int maxPolies);
qboolean BotPathIsWalkable(gentity_t *self, botTarget_t target);
qboolean BotMoveToGoal( gentity_t *self, usercmd_t *botCmdBuffer );
void BotDodge(gentity_t *self, usercmd_t *botCmdBuffer);
void BotGoto(gentity_t *self, botTarget_t target, usercmd_t *botCmdBuffer);
int	FindRouteToTarget( gentity_t *self, botTarget_t target );
void BotSearchForGoal(gentity_t *self, usercmd_t *botCmdBuffer);
int DistanceToGoal(gentity_t *self);
int DistanceToGoalSquared(gentity_t *self);
int BotGetStrafeDirection(void);
void PlantEntityOnGround(gentity_t *ent, vec3_t groundPos);

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

static inline bool BotRoutePermission(gentity_t *self, botTask_t task) {
	return (!self->botMind->goal.inuse || self->botMind->needNewGoal || self->botMind->task != task);
}

static inline bool BotChangeTarget(gentity_t *self, gentity_t *target, vec3_t *pos) {
	BotSetGoal(self, target, pos);
	if( !self->botMind->goal.inuse )
		return false;
	if( FindRouteToTarget(self, self->botMind->goal) & (STATUS_PARTIAL|STATUS_FAILED) ) {
		return false;
	}
	self->botMind->needNewGoal = qfalse;
	return true;
}

static inline bool BotChangeTarget(gentity_t *self, botTarget_t target) {
	if( !target.inuse )
		return false;
	if( FindRouteToTarget(self, target) & (STATUS_PARTIAL | STATUS_FAILED)) {
		return false;
	}
	self->botMind->needNewGoal = qfalse;
	self->botMind->goal = target;
	return true;
}

static inline void BotChangeTask(gentity_t *self, botTask_t task) {
	self->botMind->task = task;
}
//configureable constants
//For a reference of how far a number represents, take a look at tremulous.h

//how long our traces are for obstacle avoidence
#define BOT_OBSTACLE_AVOID_RANGE 50.0f

//when closer to the enemy than this range, the human bots will backup
#define BOT_BACKUP_RANGE 300.0f

//how far off can our aim can be from true in order to try to hit the enemy
#define BOT_AIM_NEGLIGENCE 30.0f

//How long in milliseconds the bots will chase an enemy if he goes out of their sight (humans) or radar (aliens)
#define BOT_ENEMY_CHASETIME 5000

//How close does the enemy have to be for the bot to engage him/her when doing a task other than fight/roam
#define BOT_ENGAGE_DIST 500

//How long in milliseconds it takes the bot to react upon seeing an enemy
#define BOT_REACTION_TIME 500

//at what hp do we use medkit?
#define BOT_USEMEDKIT_HP 50

//when human bots reach this ammo percentage left or less(and no enemy), they will head back to the base to refuel ammo
#define BOT_LOW_AMMO 0.50f

//used for clamping distance to heal structure when deciding whether to go heal
#define MAX_HEAL_DIST 2000

//compared to heal logic value to decide if the bot should return to base to heal
//heal logic value <= BOT_FUZZY_HEAL_VALUE means bot will go heal
#define BOT_FUZZY_HEAL_VALUE MAX_HEAL_DIST / 4

//how long the bot will continue retreating after loseing sight of the enemy
#define BOT_RETREAT_TIME 5000

//how close the enemy has to be to make the bot engage the enemy when the bot is retreating
#define BOT_RETREAT_ENGAGE_DIST 300

#define BOT_LEADER_WAIT_RANGE 250 //should be > BOT_FOLLOW_RANGE + BOT_FOLLOW_RANGE_NEGLIGENCE
#define BOT_FOLLOW_RANGE 100
#define BOT_FOLLOW_RANGE_NEGLIGENCE 30
#endif

#endif

