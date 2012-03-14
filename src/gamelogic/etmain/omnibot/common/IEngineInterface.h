////////////////////////////////////////////////////////////////////////////////
//
// $LastChangedBy: jswigart $
// $LastChangedDate: 2010-08-28 07:12:05 +0200 (Sa, 28 Aug 2010) $
// $LastChangedRevision: 32 $
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __ENGINE_INTERFACE_H__
#define __ENGINE_INTERFACE_H__

#include "Omni-Bot_Types.h"
#include "Omni-Bot_BitFlags.h"
#include "Omni-Bot_Color.h"
#include "MessageHelper.h"

// Title: Engine Interface

// struct: EntityInfo
//		Used to store information about an entity
typedef struct
{
	// BitFlag32: m_Team
	//    If this != 0, this entity should only be visible to certain teams.
	BitFlag32 m_Team;
	// int: m_EntityClass
	//    The specific classification of this entity
	int m_EntityClass;
	// BitFlag64: m_EntityCategoty
	//    Current category of this entity, see <EntityCategory>
	BitFlag64 m_EntityCategory;
	// BitFlag64: m_EntityFlags
	//    Current flags of this entity, see <EntityFlags>
	BitFlag64 m_EntityFlags;
	// BitFlag64: m_EntityPowerups
	//    Current power-ups of this entity, see <Powerups>
	BitFlag64 m_EntityPowerups;
} EntityInfo;

// struct: ClientInput
//		Generic data structure representing the bots input and movement states
//		Game is responsible for translating this into a format suitable for use
//		by the game.
typedef struct
{
	// float: m_Facing
	//    The direction the bot is facing/aiming
	float m_Facing[ 3 ];
	// float: m_MoveDir
	//    The direction the bot is moving
	float m_MoveDir[ 3 ];
	// int: m_ButtonFlags
	//    64 bit int of bits representing bot keypresses, see <ButtonFlags>
	BitFlag64 m_ButtonFlags;
	// int: m_CurrentWeapon
	//    The current weapon Id this bot wants to use.
	int m_CurrentWeapon;
} ClientInput;

// class: obTraceResult
//		This file defines all the common structures used by the game and bot alike.
class obTraceResult
{
public:
// float: m_Fraction
//		0.0 - 1.0 how far the trace went
	float      m_Fraction;
// float: m_Normal
//		The plane normal that was struck
	float      m_Normal[ 3 ];
// float: m_Endpos
//		The end point the trace ended at
	float      m_Endpos[ 3 ];
// var: m_HitEntity
//		The entity that was hit by the trace
	GameEntity m_HitEntity;
// int: m_StartSolid
//		Did the trace start inside a solid?
	int        m_StartSolid;
// int: m_Contents
//		Content flags.
	int        m_Contents;
// int: m_Surface;
//		Flags representing the surface struct by the trace.
	int        m_Surface;

	obTraceResult() :
		m_Fraction( 0.f ),
		m_StartSolid( 0 ),
		m_Contents( 0 ),
		m_Surface( 0 )
	{
	}
};

class obPlayerInfo
{
public:
	enum { MaxPlayers = 64, MaxTeams = 6 };

	enum Controller { Bot = 0, Human, Both };

	struct PInfo
	{
		int        m_Team;
		int        m_Class;
		Controller m_Controller;

		PInfo()
		{
			m_Team = OB_TEAM_NONE;
			m_Class = 0;
			m_Controller = Bot;
		}
	};

	PInfo m_Players[ MaxPlayers ];
	int   m_MaxPlayers;
	int   m_AvailableTeams;

	int GetNumPlayers( int t = OB_TEAM_ALL, Controller c = Both ) const
	{
		int n = 0;

		for( int i = 0; i < MaxPlayers; ++i )
		{
			if( m_Players[ i ].m_Team == 0 )
			{
				continue;
			}

			if( t != OB_TEAM_ALL && m_Players[ i ].m_Team != t )
			{
				continue;
			}

			if( c != Both && m_Players[ i ].m_Controller != c )
			{
				continue;
			}

			++n;
		}

		return n;
	}

	int GetMaxPlayers() const
	{
		return m_MaxPlayers;
	}

	int GetAvailableTeams() const
	{
		return m_AvailableTeams;
	}

	obPlayerInfo()
	{
		for( int i = 0; i < MaxPlayers; ++i )
		{
			m_Players[ i ] = PInfo();
		}

		m_AvailableTeams = 0;
		m_MaxPlayers = 0;
	}
};

// typedef: IEngineInterface
//		This struct defines all the function that the game will implement
//		and give to the bot so that the bot may perform generic
//		actions without caring about the underlying engine or game.
class IEngineInterface
{
public:
	enum DebugRenderFlags
	{
	  DR_NODEPTHTEST = ( 1 << 0 ),
	};

// Function: AddBot
//		This function should add a bot to the game with the name specified,
//		and return the bots GameID
	virtual int AddBot( const MessageHelper &_data ) = 0;

// Function: RemoveBot
//		This function should remove/kick a bot from the game by its name or id
	virtual void RemoveBot( const MessageHelper &_data ) = 0;

// Function: ChangeTeam
//		This function should force a bot to a certain team
	virtual obResult ChangeTeam( int _client, int _newteam, const MessageHelper *_data ) = 0;

// Function: ChangeClass
//		This function should force a bot to change to a certain class
	virtual obResult ChangeClass( int _client, int _newclass, const MessageHelper *_data ) = 0;

// Function: UpdateBotInput
//		This function should interpret and handle the bots input
	virtual void UpdateBotInput( int _client, const ClientInput &_input ) = 0;

// Function: BotCommand
//		This function should perform a bot 'console command'
	virtual void BotCommand( int _client, const char *_cmd ) = 0;

// Function: IsInPVS
//		This function should check if a target position is within pvs of another position.
	virtual obBool IsInPVS( const float _pos[ 3 ], const float _target[ 3 ] ) = 0;

// Function: TraceLine
//		This bot should intepret and perform a traceline, returning
//		the results into the <BotTraceResult> parameter.
	virtual obResult TraceLine( obTraceResult &_result, const float _start[ 3 ], const float _end[ 3 ], const AABB *_pBBox, int _mask, int _user, obBool _bUsePVS ) = 0;

// Function: GetPointContents
//		Gets the content bitflags for a location.
	virtual int GetPointContents( const float _pos[ 3 ] ) = 0;

// Function: GetLocalGameEntity
//		Gets the game entity of the local client. For listen servers only.
	virtual GameEntity GetLocalGameEntity() = 0;

// Function: FindEntityInSphere
//		This function should return entities matching the class id, and in a radius, and should be
//		compatible with a while loop using the _pStart and returning 0 at the end of search
	virtual GameEntity FindEntityInSphere( const float _pos[ 3 ], float _radius, GameEntity _pStart, int classId ) = 0;

// Function: GetEntityClass
//		This function should return the bot class of the entity.
	virtual int GetEntityClass( const GameEntity _ent ) = 0;

// Function: GetEntityCategory
//		This function should return the bot class of the entity.
	virtual obResult GetEntityCategory( const GameEntity _ent, BitFlag32 &_category ) = 0;

// Function: GetEntityFlags
//		This function should return the entity flags for an entity
	virtual obResult GetEntityFlags( const GameEntity _ent, BitFlag64 &_flags ) = 0;

// Function: GetEntityPowerups
//		This function should return the powerup flags for an entity.
	virtual obResult GetEntityPowerups( const GameEntity _ent, BitFlag64 &_flags ) = 0;

// Function: GetEntityEyePosition
//		This function should return the eye position of an entity
	virtual obResult GetEntityEyePosition( const GameEntity _ent, float _pos[ 3 ] ) = 0;

// Function: GetEntityEyePosition
//		This function should return the bone position of an entity
	virtual obResult GetEntityBonePosition( const GameEntity _ent, int _boneid, float _pos[ 3 ] ) = 0;

// Function: GetEntityOrientation
//		This function should return the orientation of a <GameEntity> as fwd, right, up vectors
	virtual obResult GetEntityOrientation( const GameEntity _ent, float _fwd[ 3 ], float _right[ 3 ], float _up[ 3 ] ) = 0;

// Function: GetEntityVelocity
//		This function should return the velocity of a <GameEntity> in world space
	virtual obResult GetEntityVelocity( const GameEntity _ent, float _velocity[ 3 ] ) = 0;

// Function: GetEntityPosition
//		This function should return the position of a <GameEntity> in world space
	virtual obResult GetEntityPosition( const GameEntity _ent, float _pos[ 3 ] ) = 0;

// Function: GetEntityLocalAABB
//		This function should return the axis aligned box of a <GameEntity> in local space
	virtual obResult GetEntityLocalAABB( const GameEntity _ent, AABB &_aabb ) = 0;

// Function: GetEntityWorldAABB
//		This function should return the axis aligned box of a <GameEntity> in world space
	virtual obResult GetEntityWorldAABB( const GameEntity _ent, AABB &_aabb ) = 0;

// Function: GetEntityWorldOBB
//		This function should return the oriented box of a <GameEntity> in world space
	virtual obResult GetEntityWorldOBB( const GameEntity _ent, float *_center, float *_axis0, float *_axis1, float *_axis2, float *_extents ) = 0;

// Function: GetEntityGroundEntity
//		This function should return any entity being rested/stood on.
	virtual obResult GetEntityGroundEntity( const GameEntity _ent, GameEntity &moveent ) = 0;

// Function: GetEntityOwner
//		This function should return the <GameID> of a client that owns this item
	virtual GameEntity GetEntityOwner( const GameEntity _ent ) = 0;

// Function: GetEntityTeam
//		This function should return the bot team of the entity.
	virtual int GetEntityTeam( const GameEntity _ent ) = 0;

// Function: GetClientName
//		This function should give access to the in-game name of the client
	virtual const char *GetEntityName( const GameEntity _ent ) = 0;

// Function: GetCurrentWeaponClip
//		This function should update weapon clip count for the current weapon
	virtual obResult GetCurrentWeaponClip( const GameEntity _ent, FireMode _mode, int &_curclip, int &_maxclip ) = 0;

// Function: BotGetCurrentAmmo
//		This function should update ammo stats for a client and ammotype
	virtual obResult GetCurrentAmmo( const GameEntity _ent, int _weaponId, FireMode _mode, int &_cur, int &_max ) = 0;

// Function: GetGameTime
//		This function should return the current game time in milli-seconds
	virtual int GetGameTime() = 0;

// Function: GetGoals
//		This function should tell the game to register all <MapGoal>s with the bot
	virtual void GetGoals() = 0;

// Function: GetPlayerInfo
//		Collects team,class,controller info for all players.
	virtual void GetPlayerInfo( obPlayerInfo &info ) = 0;

// Function: InterfaceSendMessage
//		This function sends a message to the game with optional <MessageHelper> in/out parameters
//		to request additional or mod specific info from the game
	virtual obResult InterfaceSendMessage( const MessageHelper &_data, const GameEntity _ent ) = 0;

// Function: DebugLine
//		Adds a line to immediately display between 2 positions, with a specific color
	virtual bool DebugLine( const float _start[ 3 ], const float _end[ 3 ], const obColor &_color, float _time )
	{
		_start;
		_end;
		_color;
		_time;
		return false;
	}

// Function: DebugBox
//		Adds a line to immediately display between 2 positions, with a specific color
	virtual bool DebugBox( const float _mins[ 3 ], const float _maxs[ 3 ], const obColor &_color, float _time )
	{
		_mins;
		_maxs;
		_color;
		_time;
		return false;
	}

// Function: DebugArrow
//		Adds a line to immediately display between 2 positions, with a specific color
	virtual bool DebugArrow( const float _start[ 3 ], const float _end[ 3 ], const obColor &_color, float _time )
	{
		return DebugLine( _start, _end, _color, _time );
	}

// Function: DebugRadius
//		Adds a radius indicator to be displayed at a certain position with radius and color
	virtual bool DebugRadius( const float _pos[ 3 ], const float _radius, const obColor &_color, float _time )
	{
		_pos;
		_radius;
		_color;
		_time;
		return false;
	}

// Function: DebugPolygon
//		Draw a shaded polygon.
	virtual bool DebugPolygon( const obVec3 *_verts, const int _numverts, const obColor &_color, float _time, int _flags )
	{
		_verts;
		_numverts;
		_color;
		_time;
		_flags;
		return false;
	}

// Function: PrintScreenMessage
//		This function should print a message the the game screen if possible
	virtual bool PrintScreenText( const float _pos[ 3 ], float _duration, const obColor &_color, const char *_msg )
	{
		_pos;
		_duration;
		_color;
		_msg;
		return false;
	}

// Function: PrintError
//		This function should print an error the the game however desired,
//		whether it be to the console, messagebox,...
	virtual void PrintError( const char *_error ) = 0;

// Function: PrintMessage
//		This function should print a message the the game however desired,
//		whether it be to the console, messagebox,...
	virtual void PrintMessage( const char *_msg ) = 0;

// Function: GetMapName
//		This function should give access to the name of the currently loaded map
	virtual const char *GetMapName() = 0;

// Function: GetMapExtents
//		This function gets the extents of the current map.
	virtual void GetMapExtents( AABB &_aabb ) = 0;

// Function: EntityFromID
//		This function should return the <GameEntity> that matches the provided Id
	virtual GameEntity EntityFromID( const int _gameId ) = 0;

// Function: EntityByName
//		This function should return the <GameEntity> that matches a name
	virtual GameEntity EntityByName( const char *_name ) = 0;

// Function: IDFromEntity
//		This function should return the Id that matches the provided <GameEntity>
	virtual int IDFromEntity( const GameEntity _ent ) = 0;

// Function: DoesEntityStillExist
//		Checks to see if the entity that corresponds to a handle still exists.
	virtual bool DoesEntityStillExist( const GameEntity &_hndl ) = 0;

// Function: GetAutoNavFeatures
//		Gets information from the game that is used to automatically place navigation.
	virtual int GetAutoNavFeatures( AutoNavFeature *_feature, int _max ) = 0;

// Function: GetGameName
//		This function should give access to the name of the currently loaded game
	virtual const char *GetGameName() = 0;

// Function: GetModName
//		This function should give access to the name of the currently loaded mod
	virtual const char *GetModName() = 0;

// Function: GetModVers
//		This function should give access to the version of the currently loaded mod
	virtual const char *GetModVers() = 0;

// Function: GetBotPath
//		This function should get the bot path to the bot dll and base path to supplemental files.
	virtual const char *GetBotPath() = 0;

// Function: GetLogPath
//		This function should get the log path to the bot dll and base path to supplemental files.
	virtual const char *GetLogPath() = 0;

	IEngineInterface()
	{
	}

	virtual ~IEngineInterface()
	{
	}
};

//class SkeletonInterface : public IEngineInterface
//{
//public:
//	virtual int AddBot(const MessageHelper &_data) = 0;
//
//	virtual int RemoveBot(const char *_name) = 0;
//
//	virtual obResult ChangeTeam(int _client, int _newteam, const MessageHelper *_data) = 0;
//
//	virtual obResult ChangeClass(int _client, int _newclass, const MessageHelper *_data) = 0;
//
//	virtual void UpdateBotInput(int _client, const ClientInput &_input) = 0;
//
//	virtual void BotCommand(int _client, const char *_cmd) = 0;
//
//	virtual obBool IsInPVS(const float _pos[3], const float _target[3]) = 0;
//
//	virtual obResult TraceLine(obTraceResult &_result, const float _start[3], const float _end[3], const AABB *_pBBox , int _mask, int _user, obBool _bUsePVS) = 0;
//
//	virtual int GetPointContents(const float _pos[3]) = 0;
//
//	virtual GameEntity FindEntityInSphere(const float _pos[3], float _radius, GameEntity _pStart, int classId) = 0;
//
//	virtual int GetEntityClass(const GameEntity _ent) = 0;
//
//	virtual obResult  GetEntityCategory(const GameEntity _ent, BitFlag32 &_category) = 0;
//
//	virtual obResult GetEntityFlags(const GameEntity _ent, BitFlag64 &_flags) = 0;
//
//	virtual obResult GetEntityPowerups(const GameEntity _ent, BitFlag64 &_flags) = 0;
//
//	virtual obResult GetEntityEyePosition(const GameEntity _ent, float _pos[3]) = 0;
//
//	virtual obResult GetEntityBonePosition(const GameEntity _ent, int _boneid, float _pos[3]) = 0;
//
//	virtual obResult GetEntityOrientation(const GameEntity _ent, float _fwd[3], float _right[3], float _up[3]) = 0;
//
//	virtual obResult GetEntityVelocity(const GameEntity _ent, float _velocity[3]) = 0;
//
//	virtual obResult GetEntityPosition(const GameEntity _ent, float _pos[3]) = 0;
//
//	virtual obResult GetEntityWorldAABB(const GameEntity _ent, AABB &_aabb) = 0;
//
//	virtual int GetEntityOwner(const GameEntity _ent) = 0;
//
//	virtual int GetEntityTeam(const GameEntity _ent) = 0;
//
//	virtual const char *GetEntityName(const GameEntity _ent) = 0;
//
//	virtual obResult GetCurrentWeaponClip(const GameEntity _ent, FireMode _mode, int &_curclip, int &_maxclip) = 0;
//
//	virtual obResult GetCurrentAmmo(const GameEntity _ent, int _weaponId, FireMode _mode, int &_cur, int &_max) = 0;
//
//	virtual int GetGameTime() = 0;
//
//	virtual void GetGoals() = 0;
//
//	virtual void GetThreats() = 0;
//
//	virtual int GetMaxNumPlayers() = 0;
//
//	virtual int GetCurNumPlayers() = 0;
//
//	virtual obResult InterfaceSendMessage(const MessageHelper &_data, const GameEntity _ent) = 0;
//
//	virtual bool DebugLine(const float _start[3], const float _end[3], const obColor &_color, float _time)
//	{
//	}
//
//	virtual bool DebugRadius(const float _pos[3], const float _radius, const obColor &_color, float _time)
//	{
//	}
//
//	virtual void PrintError(const char *_error) = 0;
//
//	virtual void PrintMessage(const char *_msg) = 0;
//
//	virtual void PrintScreenText(const float _pos[3], float _duration, const obColor &_color, const char *_msg) = 0;
//
//	virtual const char *GetMapName() = 0;
//
//	virtual void GetMapExtents(AABB &_aabb) = 0;
//
//	virtual GameEntity EntityFromID(const int _gameId) = 0;
//
//	virtual int IDFromEntity(const GameEntity _ent) = 0;
//
//	virtual const char *GetGameName() = 0;
//
//	virtual const char *GetModName() = 0;
//
//	virtual const char *GetModVers() = 0;
//
//	virtual const char *GetBotPath() = 0;
//
//	virtual const char *GetLogPath() = 0;
//};

#endif
