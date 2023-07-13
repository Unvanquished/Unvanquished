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

#include <vector>
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
	botTarget_t();
	botTarget_t(botTarget_t const&);
	botTarget_t(botTarget_t &&);
	botTarget_t& operator=(botTarget_t const&);
	botTarget_t& operator=(botTarget_t &&);
	~botTarget_t();

	botTarget_t(const gentity_t *ent);
	botTarget_t& operator=(const gentity_t *ent);

	botTarget_t( glm::vec3 pos );
	botTarget_t& operator=( glm::vec3 pos );

	bool operator==( botTarget_t const& o ) const;
	bool operator!=( botTarget_t const& o ) const;

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
	GentityConstRef ent = nullptr;
	glm::vec3 coord = glm::vec3();
	enum class targetType { EMPTY, COORDS, ENTITY } type = targetType::EMPTY;
};

struct botSkill_t
{
	int level;
	inline float percent( void ) const
	{
		float l = static_cast<float>( level );
		return ( l - 1.f ) / ( 9.f - 1.f );
	}
};

#define MAX_NODE_DEPTH 20
struct AIBehaviorTree_t;
struct AIGenericNode_t;
struct botMemory_t
{
	struct queue_element_t
	{
		GentityConstRef enemy;
		float score; // higher is more important
		float dist;
		int lastSeen;
		int firstSeen;
		//TODO: int lastHurt; //allows to not be angry at someone until they die
		struct detectionType_t
		{
			unsigned pain : 1;
			unsigned visible : 1;
			unsigned radar : 1; // or alien sense actually
		} detection;
		bool previousTarget = false;

		//TODO: favor targets at range
		float balancedScore( void ) const
		{
			float mod = 1.f;
			if( detection.pain )
			{
				mod += 5.0f;
			}
			if( detection.visible )
			{
				mod += 3.0f;
			}
			if( detection.radar )
			{
				mod += 0.0f;
			}
			if( previousTarget )
			{
				mod *= 1.1f;
			}
			return mod * score;
		}

		bool operator<( queue_element_t const& o ) const
		{
			return balancedScore() < o.balancedScore();
		}

		bool valid( void ) const
		{
			return false
				|| detection.pain
				|| detection.visible
				|| detection.radar
				;
		}
	};
	std::vector<queue_element_t> m_queue;
	queue_element_t bestEnemy;
public:
	void addEnemy( const gentity_t *self, const GentityConstRef &enemy, size_t oldEnemies, float dist, queue_element_t::detectionType_t detection );

	//team the bot is on when added
	team_t botTeam;

	botTarget_t goal;
	void willSprint( bool enable );
	void doSprint( int jumpCost, int stamina, usercmd_t& cmd );
	usercmd_t   cmdBuffer;

	botSkill_t botSkill;

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
	friend void BotSearchForEnemy( gentity_t * );

	int lastThink;
	int stuckTime;
	glm::vec3 stuckPosition;

	int spawnTime;
	int lastNavconTime;
	int lastNavconDistance;

	void setHasOffmeshGoal( bool val ) { hasOffmeshGoal = val; }
	bool hasOffmeshGoal;

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
