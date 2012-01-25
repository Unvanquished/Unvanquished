////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy: jswigart $
// $LastChangedDate: 2010-08-28 07:12:05 +0200 (Sa, 28 Aug 2010) $
// $LastChangedRevision: 32 $
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __BASE_MESSAGES_H__
#define __BASE_MESSAGES_H__

#include "Omni-Bot_Types.h"

#pragma pack(push)
#pragma pack(4)

//////////////////////////////////////////////////////////////////////////

struct Msg_Addbot
{
	int			m_Team;
	int			m_Class;
	char		m_Name[64];
	char		m_Model[64];
	char		m_Skin[64];
	char		m_SpawnPointName[64];
	char		m_Profile[64];

	Msg_Addbot()
		: m_Team(RANDOM_TEAM_IF_NO_TEAM)
		, m_Class(RANDOM_CLASS_IF_NO_CLASS)
	{
		m_Name[0] = m_Model[0] = m_Skin[0] = m_SpawnPointName[0] = m_Profile[0] = 0;
	}
};

struct Msg_Kickbot
{
	enum { BufferSize = 64, InvalidGameId = -1 };
	char		m_Name[BufferSize];
	int			m_GameId;

	Msg_Kickbot()
		: m_GameId(InvalidGameId)
	{
		m_Name[0] =  0;
	}
};

struct Msg_ChangeName
{
	char	m_NewName[64];
};

struct Msg_PlayerChooseEquipment
{
	enum { NumItems = 16 };
	int			m_WeaponChoice[NumItems];
	int			m_ItemChoice[NumItems];

	Msg_PlayerChooseEquipment()
	{
		for(int i = 0; i < NumItems; ++i)
		{
			m_WeaponChoice[i] = 0;
			m_ItemChoice[i] = 0;
		}
	}
};

struct Msg_HealthArmor
{
	int			m_CurrentHealth;
	int			m_MaxHealth;
	int			m_CurrentArmor;
	int			m_MaxArmor;

	Msg_HealthArmor()
		: m_CurrentHealth(0)
		, m_MaxHealth(0)
		, m_CurrentArmor(0)
		, m_MaxArmor(0)
	{
	}
};

struct Msg_PlayerMaxSpeed
{
	float		m_MaxSpeed;

	Msg_PlayerMaxSpeed() : m_MaxSpeed(0.f) {}
};

struct Msg_IsAlive
{	
	obBool		m_IsAlive;

	Msg_IsAlive() : m_IsAlive(False) {}
};

struct Msg_IsAllied
{
	GameEntity	m_TargetEntity;
	obBool		m_IsAllied;

	Msg_IsAllied(GameEntity e) : m_TargetEntity(e), m_IsAllied(True) {}
};

struct Msg_IsOutside
{
	float		m_Position[3];
	obBool		m_IsOutside;

	Msg_IsOutside()
		: m_IsOutside(False)
	{
		m_Position[0] = m_Position[1] = m_Position[2] = 0.f;
	}
};

struct Msg_PointContents
{
	int			m_Contents;
	float		x,y,z;

	Msg_PointContents()
		: m_Contents(0)
		, x(0.f)
		, y(0.f)
		, z(0.f)
	{
	}
};

struct Msg_ReadyToFire
{
	obBool		m_Ready;
	
	Msg_ReadyToFire() : m_Ready(False) {}
};

struct Msg_Reloading
{
	obBool		m_Reloading;

	Msg_Reloading() : m_Reloading(False) {}
};

struct Msg_FlagState
{
	FlagState	m_FlagState;
	GameEntity	m_Owner;

	Msg_FlagState()
		: m_FlagState(S_FLAG_NOT_A_FLAG)
	{
	}
};

struct Msg_GameState
{
	GameState	m_GameState;
	float		m_TimeLeft;

	Msg_GameState()
		: m_GameState(GAME_STATE_INVALID)
		, m_TimeLeft(0.f)
	{
	}
};

struct Msg_EntityStat
{
	char		m_StatName[64];
	obUserData	m_Result;

	Msg_EntityStat()
	{
		m_StatName[0] = 0;
	}
};

struct Msg_TeamStat
{
	int			m_Team;
	char		m_StatName[64];
	obUserData	m_Result;

	Msg_TeamStat()
		: m_Team(0)
	{
		m_StatName[0] = 0;
	}
};

struct Msg_ServerCommand
{
	char		m_Command[256];

	Msg_ServerCommand()
	{
		m_Command[0] = 0;
	}
};

struct WeaponCharged
{
	int			m_Weapon;
	FireMode	m_FireMode;
	obBool		m_IsCharged;
	obBool		m_IsCharging;

	WeaponCharged(int w = 0, FireMode m = Primary)
		: m_Weapon(w)
		, m_FireMode(m)
		, m_IsCharged(False)
		, m_IsCharging(False)
	{
	}
};

struct WeaponHeatLevel
{
	FireMode	m_FireMode;
	float		m_CurrentHeat;
	float		m_MaxHeat;

	WeaponHeatLevel(FireMode m = Primary)
		: m_FireMode(m)
		, m_CurrentHeat(0.f)
		, m_MaxHeat(0.f)
	{
	}
};

struct VehicleInfo
{
	int			m_Type;
	GameEntity	m_Entity;
	GameEntity	m_Weapon;
	GameEntity	m_Driver;
	int			m_VehicleHealth;
	int			m_VehicleMaxHealth;
	float		m_Armor;

	VehicleInfo()
	{
		m_Type = 0;
		m_Entity = GameEntity();
		m_Weapon = GameEntity();
		m_Driver = GameEntity();
		m_VehicleHealth = 0;
		m_VehicleMaxHealth = 0;
		m_Armor = 0.f;
	}
};

struct ControllingTeam
{
	int		m_ControllingTeam;

	ControllingTeam() : m_ControllingTeam(0) {}
};

struct WeaponStatus
{
	int			m_WeaponId;
	//FireMode	m_FireMode;

	WeaponStatus() : m_WeaponId(0) {}

	bool operator==(const WeaponStatus &_w2)
	{
		return m_WeaponId == _w2.m_WeaponId;
	}
	bool operator!=(const WeaponStatus &_w2)
	{
		return !(*this == _w2);
	}
};

struct WeaponLimits
{
	float		m_CenterFacing[3];
	float		m_MinYaw;
	float		m_MaxYaw;
	float		m_MinPitch;
	float		m_MaxPitch;
	int			m_WeaponId;
	obBool		m_Limited;

	WeaponLimits()
		: m_MinYaw(-45.f)
		, m_MaxYaw( 45.f)
		, m_MinPitch(-20.f)
		, m_MaxPitch( 20.f)
		, m_WeaponId(0)
		, m_Limited(False)
	{
		m_CenterFacing[0] = 0.f;
		m_CenterFacing[1] = 0.f;
		m_CenterFacing[2] = 0.f;
	}
};

struct Msg_KillEntity
{
	GameEntity	m_WhoToKill;
};

struct Event_PlaySound
{
	char		m_SoundName[128];
};

struct Event_StopSound
{
	char		m_SoundName[128];
};

struct Event_ScriptEvent
{
	char		m_FunctionName[64];
	char		m_EntityName[64];
	char		m_Param1[64];
	char		m_Param2[64];
	char		m_Param3[64];
};

struct Msg_GotoWaypoint
{
	char		m_WaypointName[64];
	float		m_Origin[3];

	Msg_GotoWaypoint()
	{
		m_Origin[0] = 0.f;
		m_Origin[1] = 0.f;
		m_Origin[2] = 0.f;
		m_WaypointName[0] = 0;
	}
};

struct Msg_MoverAt
{
	float		m_Position[3];
	float		m_Under[3];

	GameEntity	m_Entity;

	Msg_MoverAt()
	{
		m_Position[0]=m_Position[1]=m_Position[2]=0.f;
		m_Under[0]=m_Under[1]=m_Under[2]=0.f;
	}
};

//////////////////////////////////////////////////////////////////////////
// Events

struct Event_SystemScriptUpdated
{
	obint32		m_ScriptKey;
};

struct Event_SystemClientConnected
{
	int			m_GameId;
	obBool		m_IsBot;
	int			m_DesiredClass;
	int			m_DesiredTeam;

	Event_SystemClientConnected() 
		: m_GameId(-1)
		, m_IsBot(False)
		, m_DesiredClass(RANDOM_CLASS_IF_NO_CLASS)
		, m_DesiredTeam(RANDOM_TEAM_IF_NO_TEAM)
	{
	}
};

struct Event_SystemClientDisConnected
{
	int			m_GameId;
};

struct Event_SystemGravity
{
	float		m_Gravity;
};

struct Event_SystemCheats
{
	obBool		m_Enabled;
};

struct Event_EntityCreated
{
	GameEntity		m_Entity;
	BitFlag32		m_EntityCategory;
	int				m_EntityClass;	
};

struct EntityInstance
{
	GameEntity		m_Entity;
	BitFlag32		m_EntityCategory;
	int				m_EntityClass;
	int				m_TimeStamp;
};

struct Event_EntityDeleted
{
	GameEntity		m_Entity;
};

//////////////////////////////////////////////////////////////////////////

struct Event_Death
{
	GameEntity	m_WhoKilledMe;
	char		m_MeansOfDeath[32];
};

struct Event_KilledSomeone
{
	GameEntity	m_WhoIKilled;
	char		m_MeansOfDeath[32];
};

struct Event_TakeDamage
{
	GameEntity	m_Inflictor;
};

struct Event_Healed
{
	GameEntity	m_WhoHealedMe;
};

struct Event_Revived
{
	GameEntity	m_WhoRevivedMe;
};

struct Event_ChangeTeam
{
	int			m_NewTeam;
};

struct Event_WeaponChanged
{
	int			m_WeaponId;
};

struct Event_ChangeClass
{
	int			m_NewClass;
};

struct Event_Spectated
{
	int			m_WhoSpectatingMe;
};

struct Event_AddWeapon
{
	int			m_WeaponId;
};

struct Event_RemoveWeapon
{
	int			m_WeaponId;
};

struct Event_RefreshWeapon
{
	obint32		m_WeaponId;
};

struct Event_WeaponFire
{
	int			m_WeaponId;
	FireMode	m_FireMode;
	GameEntity	m_Projectile;
};

struct Event_WeaponChange
{
	int			m_WeaponId;
};

struct Event_ChatMessage
{
	GameEntity	m_WhoSaidIt;
	char		m_Message[512];
};

struct Event_VoiceMacro
{
	GameEntity	m_WhoSaidIt;
	char		m_MacroString[64];
};

struct Event_PlayerUsed
{
	GameEntity	m_WhoDidIt;
};

struct Event_Sound
{
	char		m_SoundName[64];
	float		m_Origin[3];
	GameEntity	m_Source;
	int			m_SoundType;
};

struct Event_EntitySensed
{
	int			m_EntityClass;
	GameEntity	m_Entity;
};

struct Event_DynamicPathsChanged
{
	int			m_TeamMask;
	int			m_NavId;

	Event_DynamicPathsChanged(int _team, int _navid = 0)
		: m_TeamMask(_team)
		, m_NavId(_navid)
	{
	}
};

struct Event_ScriptMessage
{
	char		m_MessageName[64];
	char		m_MessageData1[64];
	char		m_MessageData2[64];
	char		m_MessageData3[64];
};

struct Event_ScriptSignal
{
	char		m_SignalName[64];
};

struct Event_EntityConnection
{
	GameEntity	m_Entity;
	int			m_ConnectionId;
	ConnDir		m_ConnectionDir;
	BitFlag32	m_Team;
	float		m_Radius;
	bool		m_Teleport;

	Event_EntityConnection()
		: m_ConnectionId(0)
		, m_ConnectionDir(CON_TWO_WAY)
		, m_Radius(0.f)
		, m_Teleport(false)
	{
	}
};

struct Event_EntEnterRadius
{
	GameEntity	m_Entity;
};

struct Event_EntLeaveRadius
{
	GameEntity	m_Entity;
};

#pragma pack(pop)

#endif
