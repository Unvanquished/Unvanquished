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

#include <bitset>

enum class navMeshStatus_t
{
	UNINITIALIZED,
	GENERATING,
	LOAD_FAILED,
	LOADED,
};
extern navMeshStatus_t navMeshLoaded;

struct botEntityAndDistance_t
{
	gentity_t const *ent;
	float distance;
};

class botTarget_t
{
public:
	botTarget_t& operator=(const gentity_t *ent);
	botTarget_t& operator=( glm::vec3 pos );
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
	glm::vec3 getPos() const;
private:
	GentityConstRef ent;
	glm::vec3 coord;
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

// boolean flags that tells which skill (compétence) the bot has.
//
// When you add a skill, add it to the skill tree in sg_bot_skilltree.cpp and
// change the number of skillpoint a bot has in BotDetermineSkills
enum bot_skill
{
	// movement skills
	BOT_B_BASIC_MOVEMENT, // doesn't do anything as of now
	BOT_A_SMALL_JUMP_ON_ATTACK, // small aliens (dretch and mantis) do jump from time to time when attacking, this helps dodging
	BOT_A_MARA_JUMP_ON_ATTACK, // marauder jumps when in a fight
	BOT_A_STRAFE_ON_ATTACK, // strafe when attacking to dodge bullets better
	BOT_A_LEAP_ON_ATTACK, // mantis
	BOT_A_POUNCE_ON_ATTACK, // dragoon and adv dragoon
	BOT_A_TYRANT_CHARGE_ON_ATTACK,

	// situation awareness and survival
	BOT_B_PAIN, // basic awareness: notice an enemy if it bites you, or shoots at you
	BOT_A_FAST_FLEE,
	BOT_H_FAST_FLEE,
	BOT_A_SAFE_BARBS, // don't barb yourself as adv goon
	BOT_H_BUY_MODERN_ARMOR, // if this is disabled, bot will buy the second-to-last unlocked armor, and will never buy battlesuit
	BOT_H_PREFER_ARMOR, // prefer to buy armor rather than guns
	BOT_H_MEDKIT, // knows the medkit even exist

	// fighting skills
	BOT_B_BASIC_FIGHT, // doesn't do anything as of now
	BOT_B_FAST_AIM,
	BOT_A_AIM_HEAD,
	BOT_A_AIM_BARBS, // precisely target barbs
	BOT_H_PREDICTIVE_AIM, // predict where to aim depending on weapon and enemy speed
	BOT_H_LCANNON_TRICKS,

	BOT_NUM_SKILLS
};

using skillSet_t = std::bitset<BOT_NUM_SKILLS>;

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

	int         skillLevel;
	skillSet_t  skillSet; // boolean flags
	std::string skillSetExplaination;

	botEntityAndDistance_t bestEnemy;
	botEntityAndDistance_t closestDamagedBuilding;

	// For allied buildable types: closest alive and active buildable
	// For enemy buildable types: closest alive buildable with a tag beacon
	botEntityAndDistance_t closestBuildings[ BA_NUM_BUILDABLES ];

	AIBehaviorTree_t *behaviorTree;
	AIGenericNode_t  *currentNode;
	BoundedVector<AIGenericNode_t*, MAX_NODE_DEPTH> runningNodes;
	int              numRunningNodes;

	int         futureAimTime;
	int         futureAimTimeInterval;
	glm::vec3   futureAim;
private:
	botNavCmd_t m_nav;
public:
	botNavCmd_t const& nav() const { return m_nav; };
	void clearNav() { memset( &m_nav, 0, sizeof( m_nav ) ); }
	void setGoal( botTarget_t target, bool direct ) { goal = target; m_nav.directPathToGoal = direct; }
	friend void G_BotThink( gentity_t * );
	friend bool BotChangeGoal( gentity_t *, botTarget_t );

	int lastThink;
	int stuckTime;
	glm::vec3 stuckPosition;

	int spawnTime;
	int lastNavconTime;
	int lastNavconDistance;

	void setHasOffmeshGoal( bool val ) { hasOffmeshGoal = val; }
	bool hasOffmeshGoal;

	int myTimer;
	int blackboardTransient;

	Util::optional< glm::vec3 > userSpecifiedPosition;
	Util::optional< int > userSpecifiedClientNum;
private:
	//avoid relying on buttons to remember what AI was doing
	bool wantSprinting = false;
	bool exhausted = false;
};

struct NavgenConfig;
navMeshStatus_t G_BotSetupNav( const NavgenConfig &config, const botClass_t *botClass, qhandle_t *navHandle );
void G_BotShutdownNav();
void G_BotSetNavMesh( int botClientNum, qhandle_t navHandle );
bool G_BotFindRoute( int botClientNum, const botRouteTarget_t *target, bool allowPartial );
bool G_BotPathNextCorner( int botClientNum, glm::vec3 &result );
void G_BotUpdatePath( int botClientNum, const botRouteTarget_t *target, botNavCmd_t *cmd );
bool G_IsBotOverNavcon( int botClientNum );
bool G_BotNavTrace( int botClientNum, botTrace_t *botTrace, const glm::vec3& start, const glm::vec3& end );
glm::vec3 ProjectPointOntoVector( const glm::vec3 &point, const glm::vec3 &linePoint1, const glm::vec3 &linePoint2 );

#endif
