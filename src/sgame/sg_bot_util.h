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

#ifndef BOT_UTIL_H_
#define BOT_UTIL_H_
#include <bitset>
#include "sg_local.h"
#include "botlib/bot_types.h"
#include "sg_bot_local.h"

#include <glm/vec3.hpp>

bool PlayersBehindBotInSpawnQueue( gentity_t *self );
int      BotGetDefaultSkill();
void     BotSetSkillLevel( gentity_t *self, int skill );
std::pair<std::string, skillSet_t> BotDetermineSkills( gentity_t *bot, int skill);

// unsticking
void BotCalculateStuckTime( gentity_t *self );
void BotResetStuckTime( gentity_t *self );

// entity queries
int        FindBots( int *botEntityNumbers, int maxBots, team_t team );
gentity_t* BotFindClosestEnemy( gentity_t *self );
gentity_t* BotFindBestEnemy( gentity_t *self );
void       BotFindClosestBuildings( gentity_t *self );
bool   BotTeamateHasWeapon( gentity_t *self, int weapon );
void       BotSearchForEnemy( gentity_t *self );
void       BotPain( gentity_t *self, gentity_t *attacker, int damage );
botEntityAndDistance_t BotGetHealTarget( const gentity_t *self );

// aiming
glm::vec3 BotGetIdealAimLocation( gentity_t *self, const botTarget_t &target, int lagPredictTime );
void  BotAimAtEnemy( gentity_t *self );
void  BotSlowAim( gentity_t *self, glm::vec3& target, float slow );
void  BotAimAtLocation( gentity_t *self, const glm::vec3 &target );

// targets
bool BotEntityIsValidTarget( const gentity_t *ent );
bool BotEntityIsValidEnemyTarget( const gentity_t *self, const gentity_t *enemy );
bool BotTargetIsVisible( const gentity_t *self, botTarget_t target, int mask );
bool BotTargetInAttackRange( const gentity_t *self, botTarget_t target );
void BotTargetToRouteTarget( const gentity_t *self, botTarget_t target, botRouteTarget_t *routeTarget );
botTarget_t BotGetRoamTarget( const gentity_t *self );
botTarget_t BotGetRetreatTarget( const gentity_t *self );
botTarget_t BotGetRushTarget( const gentity_t *self );

// logic functions
float    BotGetHealScore( gentity_t *self );
float    BotGetResupplyScore( gentity_t *ent );
float    BotGetBaseRushScore( gentity_t *ent );
float    BotGetEnemyPriority( gentity_t *self, gentity_t *ent );

// goal changing
bool BotChangeGoal( gentity_t *self, botTarget_t target );
bool BotChangeGoalEntity( gentity_t *self, gentity_t const *goal );
bool BotChangeGoalPos( gentity_t *self, const glm::vec3 &goal );

// fighting
void     BotResetEnemyQueue( enemyQueue_t *queue );
void     BotFireWeapon( weaponMode_t mode, usercmd_t *botCmdBuffer );
void     BotFireWeaponAI( gentity_t *self );
void     BotClassMovement( gentity_t *self, bool inAttackRange );

// human bots
bool   WeaponIsEmpty( weapon_t weapon, playerState_t const *ps );
float      PercentAmmoRemaining( weapon_t weapon, playerState_t const* ps );
void       BotFindDamagedFriendlyStructure( gentity_t *self );
bool       BotBuyWeapon( gentity_t *self, weapon_t weapon );
bool       BotBuyUpgrade( gentity_t *self, upgrade_t upgrade );
void       BotSellWeapons( gentity_t *self );
void       BotSellUpgrades( gentity_t *self );
int        BotGetDesiredBuy( gentity_t *self, weapon_t &weapon, upgrade_t upgrades[], size_t upgradesSize );

// alien bots
float CalcAimPitch( gentity_t *self, glm::vec3& pos, float launchSpeed );
float CalcPounceAimPitch( gentity_t *self, glm::vec3 &pos );
float CalcBarbAimPitch( gentity_t *self, glm::vec3 & pos );
bool BotCanEvolveToClass( const gentity_t *self, class_t newClass );
bool BotEvolveToClass( gentity_t *ent, class_t newClass );

//g_bot_nav.c
enum botMoveDir_t
{
	MOVE_FORWARD = BIT( 0 ),
	MOVE_BACKWARD = BIT( 1 ),
	MOVE_LEFT = BIT( 2 ),
	MOVE_RIGHT = BIT( 3 )
};

// navmesh generation
void G_BlockingGenerateNavmesh( std::bitset<PCL_NUM_CLASSES> classes );

// global navigation
void         G_BotNavInit( int generateNeeded );
void         G_BotNavCleanup();
bool     FindRouteToTarget( gentity_t *self, botTarget_t target, bool allowPartial );
bool         BotMoveToGoal( gentity_t *self );
void         BotSetNavmesh( gentity_t  *ent, class_t newClass );
void         BotMoveUpward( gentity_t *self, glm::vec3 target );

// local navigation
void     BotWalk( gentity_t *self, bool enable );
bool BotSprint( gentity_t *self, bool enable );
bool BotJump( gentity_t *self );
void     BotStrafeDodge( gentity_t *self );
void     BotAlternateStrafe( gentity_t *self );
void     BotMoveInDir( gentity_t *self, uint32_t moveDir );
void     BotStandStill( gentity_t *self );
bool BotWalkIfStaminaLow( gentity_t *self );

// navigation queries
bool  GoalInRange( const gentity_t *self, float r );
float DistanceToGoal( const gentity_t *self );
float DistanceToGoalSquared( const gentity_t *self );
float DistanceToGoal2DSquared( const gentity_t *self );
float BotGetGoalRadius( const gentity_t *self );
bool  BotFindRandomPointInRadius( int botClientNum, const glm::vec3 &origin, glm::vec3 &point, float radius );

bool  BotPathIsWalkable( const gentity_t *self, botTarget_t target );

#endif
