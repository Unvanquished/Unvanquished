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

#ifndef SG_MAP_ENTITY_H_
#define SG_MAP_ENTITY_H_

/**
 * The maximal available names or aliases per entity.
 *
 * If you increase these, then you also have to
 * change g_spawn.c to spawn additional targets and targetnames
 *
 * @see fields[] (where you should spawn additional ones)
 * @see G_SpawnGEntityFromSpawnVars()
 */

#define MAX_ENTITY_ALIASES 	3

/**
 * The maximal available targets to aim at per entity.
 */
#define MAX_ENTITY_TARGETS           4

/**
 * The maximal available calltargets per entity
 */
#define MAX_ENTITY_CALLTARGETS       16


enum gentityCallActionType_t
{
	ECA_NOP = 0,
	ECA_DEFAULT,
	ECA_CUSTOM,

	ECA_FREE,
	ECA_PROPAGATE,

	ECA_ACT,
	ECA_USE,
	ECA_RESET,

	ECA_ENABLE,
	ECA_DISABLE,
	ECA_TOGGLE

};


// movers are things like doors, plats, buttons, etc
enum moverState_t
{
  MOVER_POS1,
  MOVER_POS2,
  MOVER_1TO2,
  MOVER_2TO1,

  ROTATOR_POS1,
  ROTATOR_POS2,
  ROTATOR_1TO2,
  ROTATOR_2TO1,

  MODEL_POS1,
  MODEL_POS2,
  MODEL_1TO2,
  MODEL_2TO1
};

struct variatingTime_t
{
	float time;
	float variance;
};

/**
 * in the context of a target, this describes the conditions to create or to act within
 * while as part of trigger or most other types, it will be used as filtering condition that needs to be fulfilled to trigger, or to act directly
 */
struct gentityConditions_t
{
	team_t   team;
	int      stage;

	BoundedVector<class_t,     PCL_NUM_CLASSES>   classes;
	BoundedVector<weapon_t,    WP_NUM_WEAPONS>    weapons;
	BoundedVector<upgrade_t,   UP_NUM_UPGRADES>   upgrades;
	BoundedVector<buildable_t, BA_NUM_BUILDABLES> buildables;

	bool negated;
	bool isClassSensor = false;
	bool isEquipSensor = false;
};

/**
 * struct containing the configuration data of a gentity opposed to its state data
 */
struct gentityConfig_t
{
	/* amount of a context depended size for this entity */
	int amount;

	int health;
	float speed;
	int damage;

	/**
	 * how long to wait before fullfilling the maintask act()
	 * (e.g. how long to delay sensing as sensor or relaying as relay)
	 */
	variatingTime_t delay;
	/**
	 * the duration of one cycle in a repeating event
	 */
	variatingTime_t period;
	/**
	 * how long to wait in a state after a statechange
	 */
	variatingTime_t wait;

	// trigger "range"
	int triggerRange;
};

struct entityClass_t
{
	int instanceCounter;
	/**
	 * default config
	 * entities might fallback to their classwide config if their individual is not set
	 */
	gentityConfig_t config;
};

enum gentityCallEvent_t
{
	ON_DEFAULT = 0,
	ON_CUSTOM,

	ON_FREE,

	ON_CALL,

	ON_ACT,
	ON_USE,
	ON_DIE,
	ON_REACH,
	ON_RESET,
	ON_TOUCH,

	ON_ENABLE,
	ON_DISABLE,

};

struct gentityCallDefinition_t
{
	const char *event;
	gentityCallEvent_t eventType;

	char  *name;

	char  *action;
	gentityCallActionType_t actionType;
};

struct mapEntity_t
{
	/*
	 * the class of the entity
	 * this is shared among all instances of this type
	 */
	entityClass_t *eclass;

	int spawnflags;
	char *names[ MAX_ENTITY_ALIASES + 1 ];

	/*
	 * configuration, as supplied by the spawn string, external spawn scripts etc.
	 * as opposed to state data as placed everywhere else
	 */
	gentityConfig_t config;

	//conditions as trigger-filter or target-goal
	gentityConditions_t conditions;

	// entity groups
	char         *groupName;
	gentity_t    *groupChain; // next entity in group
	gentity_t    *groupMaster; // master of the group

	// movers {
	gentity_t *clipBrush; // clipping brush for model doors
	// describes if the mover is in original state, ending state,
	// or one of the 2 transitions between them, plus the type of
	// mover: mover, rotator, model. Don't know more.
	moverState_t moverState;
	// "ent->s.pos.trBase = $NAME" when "moverState == moverState_t::.*_$NAME"
	// In case $NAME is 1TO2 or 2TO1:
	// ent->s.pos.trDelta = scale( ( last - next ), ( 1000 / ent->s.pos.trDuration ) )
	vec3_t restingPosition, activatedPosition;

	// sounds played when "moverState == moverState_t::.*_$NAME"
	int soundPos1, soundPos2;
	int sound1to2, sound2to1;

	float rotatorAngle;
	// }

	/*
	 * targets to aim at
	 */
	int          targetCount;
	char         *targets[ MAX_ENTITY_TARGETS + 1 ];

	/*
	 * gentities to call on certain events
	 */
	int          callTargetCount;
	gentityCallDefinition_t calltargets[ MAX_ENTITY_CALLTARGETS + 1 ];

	//sound index, used by movers as well as target_speaker e.g. for looping sounds
	int          soundIndex;

	char         *message;

	char     *model;
	// This seems to be used by rotators, movers and doors. Quoting:
	// > if the "model2" key is set, use a separate model
	// > for drawing, but clip against the brushes [of the first model]
	char     *model2;

	vec4_t animation; // animated map objects

	/*
	 * for toggleable shaders
	 */
	char         *shaderKey;
	char         *shaderReplacement;

	float        speed;

	/* state of the amount of a context depended size for this entity
	 * example: current set gravity for a gravity afx-entity
	 */
	int          amount;

	/*
	 * do not abuse this variable (again) for anything but actual representing a count
	 *
	 * add your own number with correct semantic information to gentity_t or
	 * if you really have to use customNumber
	 */
	int count;

	vec3_t       movedir;

	void ( *reached )( gentity_t *self );       // movers call this when hitting endpoint
	void ( *blocked )( gentity_t *self, gentity_t *other );
	int         last_move_time;
	bool         locked;

	// for a location entity, timer set when nearby buildings attacked (EV_WARN_ATTACK)
	int warnTimer[ NUM_TEAMS ];

// functions
	inline bool triggerTeam( team_t other ) const
	{
		return conditions.team == TEAM_NONE || conditions.team == other;
	}

	inline bool initialPosition() const
	{
		return moverState == MOVER_POS1 || moverState == ROTATOR_POS1;
	}

	inline void deinstantiate()
	{
		if ( eclass && eclass->instanceCounter > 0 )
		{
			eclass->instanceCounter--;
		}
	}

	char const* nameList() const
	{
		if ( names[0] == nullptr )
		{
			return "";
		}

		static std::string buffer;
		buffer = names[0];
		for ( size_t i = 1; i < MAX_ENTITY_ALIASES; ++i )
		{
			if ( names[i] )
			{
				buffer += ", ";
				buffer += names[i];
			}
		}
		return buffer.c_str();
	}

};

void     G_ResetIntField( int* target, bool fallbackIfNegativ, int instanceField, int classField, int fallback );

#endif // SG_MAP_ENTITY_H
