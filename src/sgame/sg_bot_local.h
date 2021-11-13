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
	glm::vec3 getPos( void ) const;
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

class botSkill_t
{
	int level;
	float aimSlowness;
	float aimShake;
	friend constexpr size_t botSkill_max_length( void );
	static const size_t max_length = 2;
	unsigned m_level; //initial skill value

	enum skills { MOVE, AIM, AGGRO, NUM_SKILLS };
	int m_skills[NUM_SKILLS]; //individual skills (kept for future (de)serialisation)

	unsigned m_aim; //resulting average aiming
	float m_aggro;  //resulting average aggessiveness
public:

	// allows to serialize the skill repartitions
	// TODO: implement and use properly
	char const* serialize( void ) const;

	// resets a bot's skills according to an old skill repartition
	// TODO: implement and use properly
	bool deserialize( char const* ) const;

	// how much move skills the bots knows
	// between 1 and 9, included.
	uint8_t move( void ) const;

	// bot aggressiveness between 0 and 1 included
	float aggro( void ) const;

	// bot's aiming, between 1 and 1000 included
	int aim( void ) const;
	// bot's aiming, modified with a random factor
	// between 1 and 1000 included
	int rndAim( void ) const;

	// generates a random skill repartition from a base level.
	// on error, use an base level and return true.
	// number of points dispatched are base * number of trees.
	bool set( char const* name, int ent_ID /*hackish*/, int level );
};

constexpr size_t botSkill_max_length( void )
{
	return botSkill_t::max_length;
}

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
	BoundedVector<AIGenericNode_t*, MAX_NODE_DEPTH> runningNodes;
	int              numRunningNodes;

	int         futureAimTime;
	int         futureAimTimeInterval;
	glm::vec3   futureAim;
private:
	botNavCmd_t m_nav;
public:
	botNavCmd_t const& nav( void ) const { return m_nav; };
	void clearNav( void ) { memset( &m_nav, 0, sizeof( m_nav ) ); }
	void setGoal( botTarget_t target, bool direct ) { goal = target; m_nav.directPathToGoal = direct; }
	friend void G_BotThink( gentity_t * );
	friend bool BotChangeGoal( gentity_t *, botTarget_t );

	int lastThink;
	int stuckTime;
	glm::vec3 stuckPosition;

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
bool G_BotNavTrace( int botClientNum, botTrace_t *botTrace, const glm::vec3& start, const glm::vec3& end );
glm::vec3 ProjectPointOntoVector( const glm::vec3 &point, const glm::vec3 &vStart, const glm::vec3 &vEnd );

#endif
