////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy: jswigart $
// $LastChangedDate: 2010-08-28 07:12:05 +0200 (Sa, 28 Aug 2010) $
// $LastChangedRevision: 32 $
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __OMNIBOT_TYPES_H__
#define __OMNIBOT_TYPES_H__

#include "Omni-Bot_BasicTypes.h"

// constants: Omni-bot Errors
//		BOT_ERROR_NONE - No error
//		BOT_ERROR_CANTLOADDLL - Can't load DLL
//		BOT_ERROR_CANTGETBOTFUNCTIONS - Unable to get functions from bot
//		BOT_ERROR_CANTINITBOT - Unable to init bot
//		BOT_ERROR_BAD_INTERFACE - Bad interface passed to bot
//		BOT_ERROR_WRONGVERSION - Version mismatch between interface and bot
typedef enum eomnibot_error
{
	BOT_ERROR_NONE,
	BOT_ERROR_CANTLOADDLL,
	BOT_ERROR_CANTGETBOTFUNCTIONS,
	BOT_ERROR_CANTINITBOT,
	BOT_ERROR_BAD_INTERFACE,
	BOT_ERROR_WRONGVERSION,
	BOT_ERROR_FILESYSTEM,

	// THIS MUST STAY LAST
	BOT_NUM_ERRORS
} omnibot_error;

typedef enum eMessageType
{
	kNormal,
	kInfo,
	kWarning,
	kError,
	kDebug,
	kScript
} MessageType;

// typedef: GameEntity
//		Represents an entity to the bot for every game.
class GameEntity
{
public:
	obint16 GetIndex() const { return udata.m_Short[0]; }
	obint16 GetSerial() const { return udata.m_Short[1]; }

	obint32 AsInt() const { return udata.m_Int; }
	void FromInt(obint32 _n) { udata.m_Int = _n; }

	void Reset()
	{
		*this = GameEntity();
	}

	bool IsValid() const
	{
		return udata.m_Short[0] >= 0;
	}

	bool operator!=(const GameEntity& _other) const
	{
		return udata.m_Int != _other.udata.m_Int;
	}
	bool operator==(const GameEntity& _other) const
	{
		return udata.m_Int == _other.udata.m_Int;
	}

	explicit GameEntity(obint16 _index, obint16 _serial)
	{
		udata.m_Short[0] = _index;
		udata.m_Short[1] = _serial;
	}
	GameEntity()
	{
		udata.m_Short[0] = -1;
		udata.m_Short[1] = 0;
	}
private:
	union udatatype
	{
		obint32			m_Int;
		obint16			m_Short[2];
	} udata;
};

// typedef: GameId
//		A numeric value for an entities game id. Usually an array index of some sort.
typedef int GameId;

// typedef: NavFlags
//		This is the type waypoint flags, and should be a 64 bit type
typedef obuint64 NavFlags;

// enum: obBool
//		Since theres no promise that we're running in C or C++, and therefor no
//		guarantee that bools are supported, lets use an enumerated value instead.
typedef enum eobBool
{
	Invalid = -1,
	False,
	True
} obBool;

// enum: obFunctionStatus
//		Represents the status of some function.
typedef enum eobFunctionStatus
{
	Function_Finished,
	Function_InProgress,	
} obFunctionStatus;

// enumerations: obResult
//		Success - Successful.
//		OutOfPVS - Out of PVS(Potential Visibility Set).
//		UnableToAddBot - Unable to add bot for some reason.
//		InvalidEntity - Invalid entity parameter.
//		InvalidParameter - Invalid parameter.
typedef enum eobResult
{
	Success = 0,
	OutOfPVS,
	UnableToAddBot,
	InvalidEntity,
	InvalidParameter,
	UnknownMessageType,
} obResult;

#ifdef __cplusplus
inline bool SUCCESS(obResult _res)
{
	return (_res == Success) ? true : false;
}
inline int MAKE_KEY(char _v1, char _v2, char _v3, char _v4)
{
	return (((_v1)<<24) | ((_v2)<<16) | ((_v3)<<8) | (_v4));
}
#else
#define SUCCESS(res) ((res) == Success ? 1 : 0)
#define MAKE_KEY(_v1, _v2, _v3, _v4) (((_v1)<<24) | ((_v2)<<16) | ((_v3)<<8) | (_v4))
#endif

typedef enum eFireMode
{
	Primary,
	Secondary,

	// THIS MUST STAY LAST
	Num_FireModes,

	// Cept for this
	InvalidFireMode
} FireMode;

typedef enum eConnDir
{
	CON_SOURCE,
	CON_DEST,
	CON_TWO_WAY,
} ConnDir;

typedef enum eStance
{
	StanceStand,
	StanceCrouch,
	StanceProne,

	// last
	StanceNum,
} Stance;

// enumerations: WeaponType
//		INVALID_WEAPON - Used for invalid weapon id.
typedef enum eWeaponType
{
	INVALID_WEAPON = 0,
} WeaponType;

// enumerations: BotDebugFlag
//		BOT_DEBUG_LOG - Debug log for this bot.
//		BOT_DEBUG_MOVEVEC - Draw the move vector.
//		BOT_DEBUG_SCRIPT - Output info about bot script events/signals.
//		BOT_DEBUG_FPINFO - Output first person info.
//		BOT_DEBUG_EVENTS - Print out events bot recieves.
typedef enum eBotDebugFlag
{
	BOT_DEBUG_LOG = 0,
	BOT_DEBUG_MOVEVEC,
	BOT_DEBUG_SCRIPT,
	BOT_DEBUG_FPINFO,
	BOT_DEBUG_PLANNER,
	BOT_DEBUG_EVENTS,
	BOT_DEBUG_LOG_FAILED_PATHS,

	// THIS MUST STAY LAST
	NUM_BOT_DEBUG_FLAGS = 16,
} BotDebugFlag;

typedef enum eTeamBase
{
	OB_TEAM_ALL = -2,
	OB_TEAM_SPECTATOR = -1,
	OB_TEAM_NONE,
	OB_TEAM_1,
	OB_TEAM_2,
	OB_TEAM_3,
	OB_TEAM_4,
} TeamBase;

// enumerations: Helpers
//		RANDOM_CLASS - Pick a random class.
//		RANDOM_CLASS_IF_NO_CLASS - Pick a random class if we don't already have a class.
//		RANDOM_TEAM - Pick a random team.
//		RANDOM_TEAM_IF_NO_TEAM -  - Pick a random team if we don't already have a team.
typedef enum eHelpers
{
	RANDOM_CLASS				= -1,
	RANDOM_CLASS_IF_NO_CLASS	= -2,
	RANDOM_TEAM					= -1,
	RANDOM_TEAM_IF_NO_TEAM		= -2,
} Helpers;

// typedef: AABB
//		Represents the axis aligned bounding box of an object
typedef struct AABB_t
{
	float	m_Mins[3];
	float	m_Maxs[3];
#ifdef __cplusplus

	enum Direction
	{
		DIR_NORTH,
		DIR_EAST,
		DIR_SOUTH,
		DIR_WEST,
		DIR_TOP,
		DIR_BOTTOM,

		DIR_ALL,
	};

	bool IsZero() const
	{
		for(int i = 0; i < 3; ++i)
		{
			if(m_Mins[i] != 0.f || 
				m_Maxs[i] != 0.f)
				return false;
		}
		return true;
	}

	void Set(const float _pt[3])
	{
		for(int i = 0; i < 3; ++i)
		{
			m_Mins[i] = _pt[i];
			m_Maxs[i] = _pt[i];
		}
	}
	void Set(const float _min[3], const float _max[3])
	{
		for(int i = 0; i < 3; ++i)
		{
			m_Mins[i] = _min[i] < _max[i] ? _min[i] : _max[i];
			m_Maxs[i] = _min[i] > _max[i] ? _min[i] : _max[i];
		}
	}
	void CenterPoint(float _out[3]) const
	{
		_out[0] = (m_Mins[0] + m_Maxs[0]) * 0.5f;
		_out[1] = (m_Mins[1] + m_Maxs[1]) * 0.5f;
		_out[2] = (m_Mins[2] + m_Maxs[2]) * 0.5f;
	}
	void CenterTop(float _out[3]) const
	{
		_out[0] = (m_Mins[0] + m_Maxs[0]) * 0.5f;
		_out[1] = (m_Mins[1] + m_Maxs[1]) * 0.5f;
		_out[2] = m_Maxs[2];
	}
	void CenterBottom(float _out[3]) const
	{
		_out[0] = (m_Mins[0] + m_Maxs[0]) * 0.5f;
		_out[1] = (m_Mins[1] + m_Maxs[1]) * 0.5f;
		_out[2] = m_Mins[2];
	}
	void SetCenter(const float _out[3])
	{
		for(int i = 0; i < 3; ++i)
		{
			m_Mins[i] += _out[i];
			m_Maxs[i] += _out[i];
		}
	}
	void Expand(const float _pt[3])
	{
		for(int i = 0; i < 3; ++i)
		{
			if(_pt[i] < m_Mins[i])
				m_Mins[i] = _pt[i];

			if(_pt[i] > m_Maxs[i])
				m_Maxs[i] = _pt[i];
		}
	}
	void Expand(const AABB_t &_bbox)
	{
		Expand(_bbox.m_Mins);
		Expand(_bbox.m_Maxs);
	}
	bool Intersects(const AABB_t &_bbox) const
	{
		for (int i = 0; i < 3; i++)
		{
			if (m_Maxs[i] < _bbox.m_Mins[i] || m_Mins[i] > _bbox.m_Maxs[i])
				return false;
		}
		return true;
	}
	bool Contains(const float _pt[3]) const
	{
		for (int i = 0; i < 3; i++)
		{
			if (m_Maxs[i] < _pt[i] || m_Mins[i] > _pt[i])
				return false;
		}
		return true;
	}
	bool FindIntersection(const AABB_t &_bbox, AABB_t& _overlap) const
	{
		if(Intersects(_bbox))
		{
			for(int i = 0; i < 3; i++)
			{
				if(m_Maxs[i] <= _bbox.m_Maxs[i])
					_overlap.m_Maxs[i] = m_Maxs[i];
				else
					_overlap.m_Maxs[i] = _bbox.m_Maxs[i];

				if(m_Mins[i] <= _bbox.m_Mins[i])
					_overlap.m_Mins[i] = _bbox.m_Mins[i];
				else
					_overlap.m_Mins[i] = m_Mins[i];
			}
			return true;
		}
		return false;
	}
	float GetAxisLength(int _axis) const
	{
		return m_Maxs[_axis] - m_Mins[_axis];
	}
	float GetArea() const
	{
		return GetAxisLength(0) * GetAxisLength(1) * GetAxisLength(2);
	}
	float DistanceFromBottom(const float _pt[3]) const
	{
		return -(m_Mins[2] - _pt[2]);
	}
	float DistanceFromTop(const float _pt[3]) const
	{
		return (m_Maxs[2] - _pt[2]);
	}
	void Scale(float _scale)
	{
		for(int i = 0; i < 3; ++i)
		{
			m_Mins[i] *= _scale;
			m_Maxs[i] *= _scale;
		}
	}
	AABB_t ScaleCopy(float _scale)
	{
		AABB_t out = *this; // cs: was AABB, but gcc said NO
		for(int i = 0; i < 3; ++i)
		{
			out.m_Mins[i] *= _scale;
			out.m_Maxs[i] *= _scale;
		}
		return out;
	}
	void Expand(float _expand)
	{
		for(int i = 0; i < 3; ++i)
		{
			m_Mins[i] -= _expand;
			m_Maxs[i] += _expand;
		}
	}
	void ExpandAxis(int _axis, float _expand)
	{
		m_Mins[_axis] -= _expand;
		m_Maxs[_axis] += _expand;
	}
	void FlipHorizontalAxis()
	{
		for(int i = 0; i < 2; ++i)
		{
			float tmp = m_Mins[i];
			m_Mins[i] = m_Maxs[i];
			m_Maxs[i] = tmp;
		}
	}
	void GetBottomCorners(float _bl[3], float _tl[3], float _tr[3], float _br[3])
	{
		_bl[0] = m_Mins[0];
		_bl[1] = m_Mins[1];
		_bl[2] = m_Mins[2];

		_tl[0] = m_Mins[0];
		_tl[1] = m_Maxs[1];
		_tl[2] = m_Mins[2];

		_tr[0] = m_Maxs[0];
		_tr[1] = m_Maxs[1];
		_tr[2] = m_Mins[2];

		_br[0] = m_Maxs[0];
		_br[1] = m_Mins[1];
		_br[2] = m_Mins[2];
	}
	void GetTopCorners(float _bl[3], float _tl[3], float _tr[3], float _br[3])
	{
		GetBottomCorners(_bl, _tl, _tr, _br);
		_bl[2] = m_Maxs[2];
		_tl[2] = m_Maxs[2];
		_tr[2] = m_Maxs[2];
		_br[2] = m_Maxs[2];
	}
	void Translate(const float _pos[3])
	{
		for(int i = 0; i < 3; ++i)
		{
			m_Mins[i] += _pos[i];
			m_Maxs[i] += _pos[i];
		}
	}
	void UnTranslate(const float _pos[3])
	{
		for(int i = 0; i < 3; ++i)
		{
			m_Mins[i] -= _pos[i];
			m_Maxs[i] -= _pos[i];
		}
	}
	AABB_t TranslateCopy(const float _pos[3]) const
	{
		AABB_t aabb = *this;
		for(int i = 0; i < 3; ++i)
		{
			aabb.m_Mins[i] += _pos[i];
			aabb.m_Maxs[i] += _pos[i];
		}
		return aabb;
	}
	AABB_t(const float _mins[3], const float _maxs[3])
	{
		Set(_mins, _maxs);
	}
	AABB_t(const float _center[3])
	{
		Set(_center);
	}
	AABB_t()
	{
		for(int i = 0; i < 3; ++i)
		{
			m_Mins[i] = 0.f;
			m_Maxs[i] = 0.f;
		}
	}
#endif
} AABB;

// enumerations: ButtonFlags
//		BOT_BUTTON_ATTACK1 - If the bot is pressing primary attack.
//		BOT_BUTTON_ATTACK2 - If the bot is pressing secondary attack.
//		BOT_BUTTON_JUMP - If the bot is pressing jump.
//		BOT_BUTTON_CROUCH - If the bot is pressing crouch.
//		BOT_BUTTON_PRONE - If the bot is pressing prone.
//		BOT_BUTTON_WALK - If the bot is pressing walk.
//		BOT_BUTTON_USE - If the bot is pressing use.
//		BOT_BUTTON_FWD - If the bot is pressing the forward key.
//		BOT_BUTTON_BACK - If the bot is pressing the backward key.
//		BOT_BUTTON_RSTRAFE - If the bot is pressing right strafe.
//		BOT_BUTTON_LSTRAFE - If the bot is pressing left strafe.
//		BOT_BUTTON_MOVEUP - Move up, typically a ladder
//		BOT_BUTTON_MOVEDN - Move down, typically a ladder
//		BOT_BUTTON_RELOAD - If the bot is pressing reload.
//		BOT_BUTTON_SPRINT - If the bot wants to sprint.
//		BOT_BUTTON_DROP - If the bot wants to drop current item.
//		BOT_BUTTON_LEANLEFT - If the bot wants to lean left.
//		BOT_BUTTON_LEANRIGHT - If the bot wants to lean right.
//		BOT_BUTTON_AIM - If the bot wants to drop current item.
//		BOT_BUTTON_RESPAWN - Bot wants to respawn.
typedef enum eButtonFlags
{	
	BOT_BUTTON_ATTACK1 = 0,
	BOT_BUTTON_ATTACK2,
	BOT_BUTTON_JUMP,
	BOT_BUTTON_CROUCH,
	BOT_BUTTON_PRONE,
	BOT_BUTTON_WALK,
	BOT_BUTTON_USE,
	BOT_BUTTON_FWD,
	BOT_BUTTON_BACK,
	BOT_BUTTON_RSTRAFE,
	BOT_BUTTON_LSTRAFE,
	BOT_BUTTON_MOVEUP,
	BOT_BUTTON_MOVEDN,
	BOT_BUTTON_RELOAD,
	BOT_BUTTON_SPRINT,
	BOT_BUTTON_DROP,
	BOT_BUTTON_LEANLEFT,
	BOT_BUTTON_LEANRIGHT,
	BOT_BUTTON_AIM,
	BOT_BUTTON_RESPAWN,
	BOT_BUTTON_TAUNT,

	// THIS MUST BE LAST
	BOT_BUTTON_FIRSTUSER
} ButtonFlags;

// enumerations: EntityFlags
//		ENT_FLAG_TEAM1 - This entity is only available/visible for team 1
//		ENT_FLAG_TEAM2 - This entity is only available/visible for team 2
//		ENT_FLAG_TEAM3 - This entity is only available/visible for team 3
//		ENT_FLAG_TEAM4 - This entity is only available/visible for team 4
//		ENT_FLAG_VISTEST - The entity should be vis tested. Otherwise uses disabled flag.
//		ENT_FLAG_DISABLED - Entity is disabled
//		ENT_FLAG_PRONED - This entity is prone
//		ENT_FLAG_CROUCHED - This entity is crouched
//		ENT_FLAG_CARRYABLE - This entity is carryable(flag, powerup,...)
//		ENT_FLAG_DEAD - This entity is dead
//		ENT_FLAG_INWATER - This entity is in water
//		ENT_FLAG_UNDERWATER - This entity is under water
//		ENT_FLAG_ZOOMING - This entity is zooming through scope or binoculars.
//		ENT_FLAG_ONLADDER - This entity is on a ladder.
//		ENT_FLAG_ONGROUND - Entity is standing on the ground
//		ENT_FLAG_RELOADING - Entity is currently reloading
//		ENT_FLAG_ON_ICE - Entity on slippery surface.
//		ENT_FLAG_HUMANCONTROLLED - Human player controls this entity.
//		ENT_FLAG_IRONSIGHT - Entity is aiming down their weapon.
//		ENT_FLAG_INVEHICLE - Entity is inside a vehicle
typedef enum eEntityFlag
{
	ENT_FLAG_TEAM1,
	ENT_FLAG_TEAM2,
	ENT_FLAG_TEAM3,
	ENT_FLAG_TEAM4,	
	ENT_FLAG_VISTEST,
	ENT_FLAG_DISABLED,
	ENT_FLAG_PRONED,	
	ENT_FLAG_CROUCHED,	
    ENT_FLAG_CARRYABLE,	
	ENT_FLAG_DEAD,	
	ENT_FLAG_INWATER,	
	ENT_FLAG_UNDERWATER,	
	ENT_FLAG_ZOOMING,	
	ENT_FLAG_ONLADDER,
	ENT_FLAG_ONGROUND,
	ENT_FLAG_RELOADING,
	ENT_FLAG_ON_ICE,
	ENT_FLAG_HUMANCONTROLLED,
	ENT_FLAG_IRONSIGHT,
	ENT_FLAG_INVEHICLE,
	ENT_FLAG_FROZEN,
	ENT_FLAG_TAUNTING,
	ENT_FLAG_AIMING,

	// THIS MUST BE LAST
	ENT_FLAG_FIRST_USER	= 32
} EntityFlags;

// enumerations: Powerups
//		PW_INVINCIBLE - The entity is invincible.
typedef enum ePowerups
{
	PWR_NONE = 0,
	PWR_INVINCIBLE,

	// THIS MUST BE LAST	
	PWR_FIRST_USER		= 8,
} Powerups;

// enumerations: EntityCategory
//		ENT_CAT_PLAYER - This entity is a player of some sort.
//		ENT_CAT_VEHICLE - Vehicle entity.
//		ENT_CAT_PROJECTILE - This entity is a projectile of some sort.
//		ENT_CAT_SHOOTABLE - This entity is shootable.
//		ENT_CAT_PICKUP - This entity is a pickup/powerup of some sort.
//		ENT_CAT_TRIGGER - This entity is a trigger of some sort.
//		ENT_CAT_MOVER - This entity is a mover of some sort(lift, door,...).
//		ENT_CAT_AVOID - This entity is something bots should avoid.
//		ENT_CAT_MOUNTEDWEAPON - This entity is something bots can mount and use.
//		ENT_CAT_MISC - Miscellaneous entity category.
//		ENT_CAT_STATIC - Static entities don't need to be seen. This allows the bot to skip LOS checks.
typedef enum eEntityCategory
{
	ENT_CAT_PLAYER,
	ENT_CAT_VEHICLE,
	ENT_CAT_PROJECTILE,
	ENT_CAT_SHOOTABLE,
	ENT_CAT_PICKUP,
	ENT_CAT_PICKUP_AMMO,
	ENT_CAT_PICKUP_WEAPON,
	ENT_CAT_PICKUP_HEALTH,
	ENT_CAT_PICKUP_ENERGY,
	ENT_CAT_PICKUP_ARMOR,
	ENT_CAT_TRIGGER,
	ENT_CAT_MOVER,
	ENT_CAT_AVOID,
	ENT_CAT_MOUNTEDWEAPON,
	ENT_CAT_MISC,
	ENT_CAT_STATIC,
	ENT_CAT_PROP,
	ENT_CAT_AUTODEFENSE,
	ENT_CAT_OBSTACLE,
	ENT_CAT_INTERNAL,

	// THIS MUST BE LAST
	ENT_CAT_MAX,
} EntityCategory;

// package: EntityClassGeneric
//		Class values for generic entities.
typedef enum eEntityClassGeneric
{
	ENT_CLASS_GENERIC_START = 10000,
	ENT_CLASS_GENERIC_SPECTATOR,
	ENT_CLASS_GENERIC_PLAYERSTART,
	ENT_CLASS_GENERIC_PLAYERSTART_TEAM1,
	ENT_CLASS_GENERIC_PLAYERSTART_TEAM2,
	ENT_CLASS_GENERIC_PLAYERSTART_TEAM3,
	ENT_CLASS_GENERIC_PLAYERSTART_TEAM4,
	ENT_CLASS_GENERIC_BUTTON,
	ENT_CLASS_GENERIC_HEALTH,
	ENT_CLASS_GENERIC_AMMO,
	ENT_CLASS_GENERIC_ARMOR,
	ENT_CLASS_GENERIC_LADDER,
	ENT_CLASS_GENERIC_FLAG,
	ENT_CLASS_GENERIC_FLAGCAPPOINT,
	ENT_CLASS_GENERIC_TELEPORTER,
	ENT_CLASS_GENERIC_LIFT,
	ENT_CLASS_GENERIC_MOVER,
	ENT_CLASS_GENERIC_JUMPPAD,
	ENT_CLASS_GENERIC_JUMPPAD_TARGET,
	ENT_CLASS_GENERIC_GOAL,
	ENT_CLASS_EXPLODING_BARREL,
	ENT_CLASS_GENERIC_WEAPON,
	ENT_CLASS_GENERIC_PROP,
	ENT_CLASS_GENERIC_PROP_EXPLODE,
} EntityClassGeneric;

// enumerations: SoundType
//		SND_JUMP - Sound of jump from another entity.
//		SND_FOOTSTEP - Sound of footstep from another entity.
//		SND_TAKEDAMAGE - Sound of another entity taking damage.
//		SND_POWERUP_SPAWN - Sound of a powerup respawning.
//		SND_POWERUP_PICKUP - Sound of a powerup being picked up.
//		SND_WEAPON_FIRE - Sound of a weapon firing.
//		SND_WEAPON_RELOAD - Sound of a weapon reloading.
//		SND_WEAPON_EMPTY - Sound of a weapon empty.
//		SND_WEAPON_STARTFIRE - Sound of a weapon starting to fire.
//		SND_VOICE_TAUNT - Sound of a voice taunt.
//		SND_VOICE_TEAM - Sound of a voice team message.
//		SND_VOICE_ENEMY - Sound of a voice enemy message.
typedef enum eSoundType
{
	SND_NONE,
	SND_JUMP,
	SND_FOOTSTEP,
	SND_TAKEDAMAGE,
	SND_POWERUP_SPAWN,
	SND_POWERUP_PICKUP,
	SND_WEAPON_FIRE_OMNIBOT,
	SND_WEAPON_RELOAD_OMNIBOT,
	SND_WEAPON_EMPTY,
	SND_WEAPON_STARTFIRE,
	SND_VOICE_TAUNT,
	SND_VOICE_TEAM,
	SND_VOICE_ENEMY,

	// THIS MUST BE LAST
	SND_MAX_SOUNDS
} SoundType;

// enumerations: Contents
//		CONT_SOLID - Solid object.
//		CONT_WATER - In water.
//		CONT_SLIME - In slime.
//		CONT_FOG - In fog.
//		CONT_TELEPORTER - In teleporter.
//		CONT_MOVER - In mover.
//		CONT_TRIGGER - In trigger.
//		CONT_LAVA - In lava.
typedef enum eContents
{
	CONT_SOLID		= (1<<0),
	CONT_WATER		= (1<<1),
	CONT_SLIME		= (1<<2),
	CONT_FOG		= (1<<3),
	CONT_MOVER		= (1<<4),
	CONT_TRIGGER	= (1<<5),
	CONT_LAVA		= (1<<6),
	CONT_LADDER		= (1<<7),
	CONT_TELEPORTER = (1<<8),
	CONT_MOVABLE	= (1<<9),
	CONT_PLYRCLIP	= (1<<10),

	// THIS MUST BE LAST
	CONT_START_USER = (1<<24)
} Contents;

// enumerations: SurfaceFlags
//		SURFACE_SLICK - Low friction surface.
typedef enum eSurfaceFlags
{
	SURFACE_SLICK	= (1<<0),
	SURFACE_LADDER	= (1<<1),

	// THIS MUST BE LAST
	SURFACE_START_USER = (1<<24)
} SurfaceFlags;
// enumerations: SkeletonBone
//		BONE_TORSO - Torso bone
//		BONE_PELVIS - Pelvis bone
//		BONE_HEAD - Head bone
//		BONE_RIGHTARM - Right arm bone
//		BONE_LEFTARM - Left arm bone
//		BONE_RIGHTHAND - Right hand bone
//		BONE_LEFTHAND - Left hand bone
//		BONE_RIGHTLEG - Right leg bone
//		BONE_LEFTLEG - Left leg bone
//		BONE_RIGHTFOOT - Right foot bone
//		BONE_LEFTFOOT - Left foot bone
typedef enum eSkeletonBone
{
	BONE_TORSO,
	BONE_PELVIS,
	BONE_HEAD,
	BONE_RIGHTARM,
	BONE_LEFTARM,
	BONE_RIGHTHAND,
	BONE_LEFTHAND,
	BONE_RIGHTLEG,
	BONE_LEFTLEG,
	BONE_RIGHTFOOT,
	BONE_LEFTFOOT,

	// THIS MUST BE LAST
	BONE_LAST_BONE = 1000
} SkeletonBone;

// enumerations: NavigationID
//		NAVID_WP - Waypoint-based path planning implementation.
//		NAVID_NAVMESH - Navigation mesh path planning implementation.
typedef enum eNavigatorID
{
	NAVID_NONE,	
	NAVID_WP,	
	NAVID_NAVMESH,
	NAVID_FLOODFILL,
	NAVID_RECAST,

	// THIS MUST BE LAST
	NAVID_MAX
} NavigatorID;

// enumerations: TraceMasks
typedef enum eTraceMasks
{
	TR_MASK_ALL			= (1<<0), // hit everything
	TR_MASK_SOLID		= (1<<1), // blocked by solids/world
	TR_MASK_PLAYER		= (1<<2), // blocked by players
	TR_MASK_SHOT		= (1<<3), // blocked by same mask that game uses for projectile/shot traces
	TR_MASK_OPAQUE		= (1<<4), // blocked by opaque surfaces(even if they can be shot through)
	TR_MASK_WATER		= (1<<5), // blocked by see-through water
	TR_MASK_SLIME		= (1<<6), // blocked by opaque water
	TR_MASK_GRATE		= (1<<7), // blocked by grates, normally projectiles go through grates, solids dont
	TR_MASK_PLAYERCLIP	= (1<<8), // blocked by player clips
	TR_MASK_SMOKEBOMB	= (1<<9), // blocked by smoke bombs
	TR_MASK_FLOODFILL	= (1<<10), // flood fill filter
	TR_MASK_FLOODFILLENT= (1<<11), // flood fill filter with entities

	// THIS MUST BE LAST
	TR_MASK_START_USER	= (1<<24),

	// combo masks can be defined separately
	TR_MASK_VISIBLE		= TR_MASK_SOLID|TR_MASK_OPAQUE|TR_MASK_SLIME, // can see but not necessarily shoot
} TraceMasks;

// struct: BotUserData
//		Generic data structure that uses a union to hold various types
//		of information.
typedef struct obUserData_t
{
	// enum: DataType
	//		This allows a small level of type safety with the messages
	//		that send BotUserData parameters. It is a good idea to use the
	//		m_DataType parameter so signal which element of the union
	//		is currently being used.
	enum { dtNone = 0, dtVector, dtString, dtInt, /*dtInt64,*/ dtFloat, dtEntity,
		dt3_4byteFlags, dt3_Strings, dt6_2byteFlags, dt12_1byteFlags } DataType;
	union udatatype
	{
		float			m_Vector[3];
		void *			m_VoidPtrs[3];
		char *			m_CharPtrs[3];
		const char *	m_String;
		int				m_Int;
		//obint64			m_Int64;
		float			m_Float;
		int				m_Entity;
		int				m_4ByteFlags[3];
		short			m_2ByteFlags[6];
		char			m_1ByteFlags[12];
	} udata;
	// Easy Constructors for C++
#ifdef __cplusplus
	obUserData_t() : DataType(dtNone) {};
	obUserData_t(const char * _str) : DataType(dtString) { udata.m_String = _str; };
	obUserData_t(int _int) : DataType(dtInt) { udata.m_Int = _int; };
	//obUserData_t(obint64 _int) : DataType(dtInt64) { udata.m_Int64 = _int; };
	obUserData_t(float _float) : DataType(dtFloat) { udata.m_Float = _float; };
	obUserData_t(const GameEntity &_ent) : DataType(dtEntity) { udata.m_Entity = _ent.AsInt(); };
	obUserData_t(float _x, float _y, float _z) : 
		DataType(dtVector) 
	{
		udata.m_Vector[0] = _x; 
		udata.m_Vector[1] = _y; 
		udata.m_Vector[2] = _z;
	};
	obUserData_t(float *_v) : 
		DataType(dtVector) 
	{
		udata.m_Vector[0] = _v[0]; 
		udata.m_Vector[1] = _v[1]; 
		udata.m_Vector[2] = _v[2]; 
	};
	obUserData_t(int _v0, int _v1, int _v2) : DataType(dt3_4byteFlags)
	{
		udata.m_4ByteFlags[0] = _v0; 
		udata.m_4ByteFlags[1] = _v1; 
		udata.m_4ByteFlags[2] = _v2;
	};
	obUserData_t(char *_v0, char *_v1, char *_v2) : DataType(dt3_Strings)
	{
		udata.m_CharPtrs[0] = _v0; 
		udata.m_CharPtrs[1] = _v1; 
		udata.m_CharPtrs[2] = _v2;
	};
	obUserData_t(short _v0, short _v1, short _v2, short _v3, short _v4, short _v5) : 
		DataType(dt6_2byteFlags)
	{
		udata.m_2ByteFlags[0] = _v0; 
		udata.m_2ByteFlags[1] = _v1; 
		udata.m_2ByteFlags[2] = _v2;
		udata.m_2ByteFlags[3] = _v3; 
		udata.m_2ByteFlags[4] = _v4; 
		udata.m_2ByteFlags[5] = _v5;
	};
	obUserData_t(char _v0, char _v1, char _v2, char _v3, char _v4, char _v5, char _v6, char _v7, char _v8, char _v9, char _v10, char _v11) : 
		DataType(dt12_1byteFlags)
	{
		udata.m_1ByteFlags[0] = _v0; 
		udata.m_1ByteFlags[1] = _v1; 
		udata.m_1ByteFlags[2] = _v2;
		udata.m_1ByteFlags[3] = _v3; 
		udata.m_1ByteFlags[4] = _v4; 
		udata.m_1ByteFlags[5] = _v5;
		udata.m_1ByteFlags[6] = _v6; 
		udata.m_1ByteFlags[7] = _v7; 
		udata.m_1ByteFlags[8] = _v8;
		udata.m_1ByteFlags[9] = _v9; 
		udata.m_1ByteFlags[10] = _v10; 
		udata.m_1ByteFlags[11] = _v11;
	};
	// Function: IsNone
	// This <BotUserData> has no type specified
	inline bool IsNone() const { return (DataType == dtNone); };
	// Function: IsString
	// This <BotUserData> is a char * type
	inline bool IsString() const { return (DataType == dtString); };
	// Function: Is3String
	// This <BotUserData> is a array of 3 strings
	inline bool Is3String() const { return (DataType == dt3_Strings); };
	// Function: IsInt
	// This <BotUserData> is an int type
	inline bool IsInt() const { return (DataType == dtInt); };
	// Function: IsInt64
	// This <BotUserData> is an int type
	//inline bool IsInt64() const { return (DataType == dtInt64); };
	// Function: IsFloat
	// This <BotUserData> is an float type
	inline bool IsFloat() const { return (DataType == dtFloat); };
	// Function: IsFloatOrInt
	// This <BotUserData> is an float type or an int type
	inline bool IsFloatOrInt() const { return (DataType == dtFloat) || (DataType == dtInt); };
	// Function: IsEntity
	// This <BotUserData> is an <GameEntity> type
	inline bool IsEntity() const { return (DataType == dtEntity); };
	// Function: IsVector
	// This <BotUserData> is a 3d Vector type
	inline bool IsVector() const { return (DataType == dtVector); };
	// Function: Is3_4ByteFlags
	// This <BotUserData> is an array of 3 4-byte values
	inline bool Is3_4ByteFlags() const { return (DataType == dt3_4byteFlags); };
	// Function: Is6_otherByteFlags
	// This <BotUserData> is an array of 6 2-byte values
	inline bool Is6_otherByteFlags() const { return (DataType == dt6_2byteFlags); };
	// Function: Is12_1ByteFlags
	// This <BotUserData> is an array of 12 1-byte values
	inline bool Is12_1ByteFlags() const { return (DataType == dt12_1byteFlags); };

	inline const char *GetString() const { return udata.m_String; };
	inline int GetInt() const { return udata.m_Int; };
	//inline obint64 GetInt64() const { return udata.m_Int64; };
	inline float GetFloat() const { return udata.m_Float; };
	inline GameEntity GetEntity() const { GameEntity e; e.FromInt(udata.m_Entity); return e; };
	inline void SetEntity(GameEntity e) { udata.m_Entity = e.AsInt(); };
	inline const char *GetStrings(int _index) const { return udata.m_CharPtrs[_index]; };
	inline const float *GetVector() const { return udata.m_Vector; };
	inline const int *Get4ByteFlags() const { return udata.m_4ByteFlags; };
	inline const short *Get2ByteFlags() const { return udata.m_2ByteFlags; };
	inline const char *Get1ByteFlags() const { return udata.m_1ByteFlags; };
	inline float GetNumAsFloat() const 
	{ 
		if(IsFloat()) 
			return GetFloat(); 
		if(IsInt()) 
			return (float)GetInt();
		return 0.0f;
	};
	inline int GetNumAsInt() const 
	{ 
		if(IsFloat()) 
			return (int)GetFloat(); 
		if(IsInt()) 
			return GetInt(); 
		return 0;
	};

	//////////////////////////////////////////////////////////////////////////
	bool Get(float &_val)
	{
		if(IsFloat())
		{
			_val = GetFloat();
			return true;
		}
		return false;
	}
	bool Get(int &_val)
	{
		if(IsInt())
		{
			_val = GetInt();
			return true;
		}
		return false;
	}
	bool Get(float *_val)
	{
		if(IsVector())
		{
			_val[0] = GetVector()[0];
			_val[1] = GetVector()[1];
			_val[2] = GetVector()[2];
			return true;
		}
		return false;
	}
	bool Get(bool &_val)
	{
		if(IsInt())
		{
			_val = GetInt()!=0;
			return true;
		}
		return false;
	}
	//////////////////////////////////////////////////////////////////////////
#endif
} obUserData;

class KeyVals
{
public:
	enum { MaxArgs = 32, MaxArgLength = 32, MaxStringLength = 64 };

	bool SetInt(const char *_key, int _val);
	bool SetFloat(const char *_key, float _val);
	bool SetEntity(const char *_key, GameEntity _val);
	bool SetVector(const char *_key, float _x,float _y,float _z);
	bool SetVector(const char *_key, const float *_v);
	bool SetString(const char *_key, const char *_value);
	bool Set(const char *_key, const obUserData &_value);

	bool GetInt(const char *_key, int &_val) const;
	bool GetFloat(const char *_key, float &_val) const;
	bool GetEntity(const char *_key, GameEntity &_val) const;
	bool GetVector(const char *_key, float &_x,float &_y,float &_z) const;
	bool GetVector(const char *_key, float *_v) const;
	bool GetString(const char *_key, const char *&_value) const;

	void Reset();

	void GetKV(int _index, const char *&_key, obUserData &ud) const;

	KeyVals();
private:
	char		m_Key[MaxArgs][MaxArgLength];
	char		m_String[MaxArgs][MaxStringLength];
	obUserData	m_Value[MaxArgs];

	bool SetKeyVal(const char *_key, const obUserData &_ud);
	bool GetKeyVal(const char *_key, obUserData &_ud) const;
};

class Seconds
{
public:
	int GetMs() const { return m_Ms; } 
	Seconds(float _seconds = 0.f) : m_Ms((int)(_seconds*1000.f)) {}
private:
	int		m_Ms;
};

// struct: TriggerInfo
enum { TriggerBufferSize = 72 };
typedef struct TriggerInfo_t
{
	// ptr: m_TagName
	//		The tagname of this trigger, usually a name given by the mapper.
	char m_TagName[TriggerBufferSize];
	// ptr: m_Action
	//		The name of the action this trigger is performing, mod specific
	char m_Action[TriggerBufferSize];
	// ptr: m_Entity
	//		The entity of this trigger, if available
	GameEntity m_Entity;
	// ptr: m_Activator
	//		The entity that activated this trigger
	GameEntity m_Activator;
#ifdef __cplusplus
	TriggerInfo_t()
	{
		for(int i = 0; i < TriggerBufferSize; ++i)
			m_TagName[i] = m_Action[i] = 0;
	}
	TriggerInfo_t(const TriggerInfo_t &_ti)
	{
		m_Entity = _ti.m_Entity;
		m_Activator = _ti.m_Activator;
		for(int i = 0; i < TriggerBufferSize; ++i)
		{
			m_TagName[i] = _ti.m_TagName[i];
			m_Action[i] = _ti.m_Action[i];
		}
	}
	TriggerInfo_t(GameEntity _ent, GameEntity _activator) : 
		m_Entity(_ent), m_Activator(_activator) 
	{
		m_TagName[0] = m_Action[0] = 0;
	}
#endif
} TriggerInfo;

// struct: MapGoalDef
typedef struct MapGoalDef_t
{
	KeyVals			Props;
#ifdef __cplusplus
	
	void Reset()
	{
		Props.Reset();
	}
	MapGoalDef_t() { Reset(); }
#endif
} MapGoalDef;

// struct: AutoNavFeature
typedef struct AutoNavFeature_t
{
	int			m_Type;
	int			m_Team;
	float		m_Position[3];
	float		m_Facing[3];
	float		m_TargetPosition[3];
	AABB		m_TargetBounds;
	float		m_TravelTime;
	AABB		m_Bounds;
	bool		m_ObstacleEntity;
	bool		m_BiDirectional;
} AutoNavFeature;

// Generic Enumerations

// enumerations: PlayerState
//		S_PLAYER_INVALID - Player doesn't exist
//		S_PLAYER_SPECTATOR - Player is in spectator mode.
//		S_PLAYED_WAITING_TEAM - Player waiting on team selection.
//		S_PLAYED_WAITING_CLASS - Player waiting on class selection.
//		S_PLAYED_WAITING_NEXTROUND - Player waiting on on the next round. Died or something.
//		S_PLAYER_PLAYING - Player is good to go, and fully joined.
typedef enum ePlayerState
{
	S_PLAYER_INVALID = 0,			// Player doesn't exist
	S_PLAYER_SPECTATOR,			// Player is in spectator mode.
	S_PLAYED_WAITING_TEAM,		// Player waiting on team selection.
	S_PLAYED_WAITING_CLASS,		// Player waiting on class selection.
	S_PLAYED_WAITING_NEXTROUND,	// Player waiting on on the next round. Died or something.
	S_PLAYER_PLAYING			// Player is good to go, and fully joined.
} PlayerState;

// enumerations: FlagState
//		S_FLAG_NOT_A_FLAG - The entity isn't a flag. Typically an error condition.
//		S_FLAG_AT_BASE - The flag is at its base position.
//		S_FLAG_DROPPED - The flag has been dropped in the field somewhere.
//		S_FLAG_CARRIED - The flag is being carried by someone/something. Should have valid owner entity.
//		S_FLAG_UNAVAILABLE - Flag is not available for some reason.
typedef enum eFlagState
{
	S_FLAG_NOT_A_FLAG = 0,
	S_FLAG_AT_BASE,
	S_FLAG_DROPPED,
	S_FLAG_CARRIED,
	S_FLAG_UNAVAILABLE,
	S_FLAG_UNKNOWN
} FlagState;

// enumerations: GameState
//		GAME_STATE_INVALID - Invalid game state.
//		GAME_STATE_PLAYING - Game is currently being played.
//		GAME_STATE_WARMUP - Game is currently in warmup period.
//		GAME_STATE_WARMUP_COUNTDOWN - Game is currently in warmup countdown, about to start.
//		GAME_STATE_INTERMISSION - Game is currently in intermission, between rounds.
//		GAME_STATE_WAITINGFORPLAYERS - Game is awaiting more players before starting.
//		GAME_STATE_PAUSED - Game is currently paused.
//		GAME_STATE_SUDDENDEATH - Sudden Death Overtime.
//		GAME_STATE_SCOREBOARD - Game finished, looking at scoreboard.
typedef enum eGameState
{
	GAME_STATE_INVALID = 0,
	GAME_STATE_INTERMISSION,
	GAME_STATE_WAITINGFORPLAYERS,
	GAME_STATE_WARMUP,
	GAME_STATE_WARMUP_COUNTDOWN,
	GAME_STATE_PLAYING,
	GAME_STATE_SUDDENDEATH,
	GAME_STATE_SCOREBOARD,
	GAME_STATE_PAUSED,
} GameState;

//////////////////////////////////////////////////////////////////////////

class Arguments
{
public:
	enum { MaxArgs = 64, MaxArgLength = 128, };
	
	Arguments() : m_NumArgs(0) 
	{
		for(int i = 0; i < MaxArgs; ++i)
			m_Args[i][0] = 0;
	}

	char	m_Args[MaxArgs][MaxArgLength];
	int		m_NumArgs;
};

//////////////////////////////////////////////////////////////////////////

struct obVec3 { float x,y,z; };

typedef enum
{
	DRAW_LINE,
	DRAW_RADIUS,
	DRAW_BOUNDS,
	DRAW_POLYGON,
	DRAW_TEXT,
} DebugMsgType;

typedef float vector_t;
typedef vector_t vector3_t[3];

typedef struct
{
	obVec3			m_Start, m_End;	
	int				m_Color;
} IPC_DebugLineMessage;

typedef struct
{
	obVec3			m_Pos;
	float			m_Radius;
	int				m_Color;
} IPC_DebugRadiusMessage;

typedef struct
{
	obVec3			m_Mins, m_Maxs;	
	int				m_Color;
	int				m_Sides;
} IPC_DebugAABBMessage;

typedef struct
{
	enum { MaxPolyVerts=32 };
	obVec3			m_Verts[MaxPolyVerts];	
	int				m_NumVerts;
	int				m_Color;
} IPC_DebugPolygonMessage;

typedef struct  
{
	enum { BufferSize=256 };
	obVec3			m_Pos;
	char			m_Buffer[BufferSize];
	int				m_Color;
} IPC_DebugTextMessage;

typedef struct
{
	union
	{
		IPC_DebugLineMessage	m_Line;
		IPC_DebugRadiusMessage	m_Radius;
		IPC_DebugAABBMessage	m_AABB;
		IPC_DebugPolygonMessage m_Polygon;
		IPC_DebugTextMessage	m_Text;
	} data;

	int				m_Duration;
	DebugMsgType	m_Debugtype;
	
} IPC_DebugDrawMsg;
//////////////////////////////////////////////////////////////////////////

#endif
