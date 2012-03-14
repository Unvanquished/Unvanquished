////////////////////////////////////////////////////////////////////////////////
//
// $LastChangedBy: jswigart $
// $LastChangedDate: 2010-08-28 07:12:05 +0200 (Sa, 28 Aug 2010) $
// $LastChangedRevision: 32 $
//
// Title: TF Message Structure Definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TF_MESSAGES_H__
#define __TF_MESSAGES_H__

#include "../common/Base_Messages.h"

#pragma pack(push)
#pragma pack(4)

//////////////////////////////////////////////////////////////////////////

struct ET_WeaponOverheated
{
	ET_Weapon m_Weapon;
	obBool    m_IsOverheated;
};

struct ET_WeaponHeatLevel
{
	GameEntity m_Entity;
	int        m_Current;
	int        m_Max;
};

struct ET_ExplosiveState
{
	GameEntity     m_Explosive;
	ExplosiveState m_State;
};

struct ET_ConstructionState
{
	GameEntity         m_Constructable;
	ConstructableState m_State;
};

struct ET_Destroyable
{
	GameEntity         m_Entity;
	ConstructableState m_State;
};

struct ET_HasFlag
{
	obBool m_HasFlag;
};

struct ET_CanBeGrabbed
{
	GameEntity m_Entity;
	obBool     m_CanBeGrabbed;
};

struct ET_TeamMines
{
	int m_Current;
	int m_Max;
};

struct ET_WaitingForMedic
{
	obBool m_WaitingForMedic;
};

struct ET_SelectWeapon
{
	ET_Weapon m_Selection;
	obBool    m_Good;
};

struct ET_ReinforceTime
{
	int m_ReinforceTime;
};

struct ET_MedicNear
{
	obBool m_MedicNear;
};

struct ET_GoLimbo
{
	obBool m_GoLimbo;
};

struct ET_MG42MountedPlayer
{
	GameEntity m_MG42Entity;
	GameEntity m_MountedEntity;
};

struct ET_MG42MountedRepairable
{
	GameEntity m_MG42Entity;
	obBool     m_Repairable;
};

struct ET_MG42Health
{
	GameEntity m_MG42Entity;
	int        m_Health;
};

struct ET_CursorHint
{
	int m_Type;
	int m_Value;
};

struct ET_SpawnPoint
{
	int m_SpawnPoint;
};

struct ET_MG42Info
{
	float m_CenterFacing[ 3 ];
	float m_MinHorizontalArc, m_MaxHorizontalArc;
	float m_MinVerticalArc, m_MaxVerticalArc;
};

struct ET_CabinetData
{
	int m_CurrentAmount;
	int m_MaxAmount;
	int m_Rate;
};

struct ET_PlayerSkills
{
	int m_Skill[ ET_SKILLS_NUM_SKILLS ];
};

struct ET_FireTeamApply
{
	int m_FireTeamNum;
};

//struct ET_FireTeamJoin
//{
//	int    m_FireTeamNum;
//};

struct ET_FireTeam
{
	GameEntity m_Target;
};

struct ET_FireTeamInfo
{
	enum { MaxMembers = 64 };

	obBool     m_InFireTeam;
	GameEntity m_Leader;
	GameEntity m_Members[ MaxMembers ];
	int        m_FireTeamNum;

	ET_FireTeamInfo()
		: m_InFireTeam( False )
		, m_FireTeamNum( 0 )
	{
	}
};

//////////////////////////////////////////////////////////////////////////

struct Event_MortarImpact_ET
{
	float m_Position[ 3 ];
};

struct Event_TriggerMine_ET
{
	GameEntity m_MineEntity;
};

struct Event_FireTeamCreated
{
	int m_FireTeamNum;
};

struct Event_FireTeamDisbanded
{
};

struct Event_FireTeamJoined
{
	GameEntity m_TeamLeader;
};

struct Event_FireTeamLeft
{
};

struct Event_FireTeamInvited
{
	GameEntity m_TeamLeader;
};

struct Event_FireTeamProposal
{
	GameEntity m_Invitee;
};

struct Event_FireTeamWarning
{
	GameEntity m_WarnedBy;
};

struct Event_Ammo
{
	GameEntity m_WhoDoneIt;
};

struct ET_GameType
{
	int m_GameType;
};

struct ET_CvarSet
{
	char *m_Cvar;
	char *m_Value;
};

struct ET_CvarGet
{
	char *m_Cvar;
	int  m_Value;
};

struct ET_DisableBotPush
{
	int m_Push;
};

#pragma pack(pop)

#endif
