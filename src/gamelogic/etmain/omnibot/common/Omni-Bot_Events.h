////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy: jswigart $
// $LastChangedDate: 2010-08-28 07:12:05 +0200 (Sa, 28 Aug 2010) $
// $LastChangedRevision: 32 $
//
// about: Generic Bot Events
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __OMNIBOT_EVENTS_H__
#define __OMNIBOT_EVENTS_H__

// typedef: EventId
//		Readable identifier for various events that can be sent to the bot
//		and considered for state changes or behavioral modifications.
typedef enum 
{
	EVENT_ID_UNDEFINED = 0,

	SYSTEM_ID_FIRST,
		SYSTEM_SCRIPT_CHANGED,
	SYSTEM_ID_LAST,

	GAME_ID_FIRST,
		GAME_STARTGAME,
		GAME_ENDGAME,
		GAME_NEWROUND,
		GAME_ENDROUND,
		GAME_CLEARGOALS,
		GAME_CLIENTCONNECTED,
		GAME_CLIENTDISCONNECTED,
		GAME_ENTITYCREATED,
		GAME_ENTITYDELETED,
		GAME_START_TRAINING,
		GAME_GRAVITY,
		GAME_CHEATS,
		GAME_SCRIPTSIGNAL,
		GAME_SOUND,
		GAME_ADD_ENTITY_CONNECTION,
	GAME_ID_LAST,

	EVENT_ID_FIRST,
		// Actions
		ACTION_ID_FIRST,
			ACTION_WEAPON_FIRE,
			ACTION_WEAPON_CHANGE,
		ACTION_ID_LAST,

		GOAL_ID_FIRST,
			GOAL_SUCCESS,
			GOAL_FAILED,
			GOAL_ABORTED,

			PATH_SUCCESS,
			PATH_FAILED,
			AIM_SUCCESS,
		GOAL_ID_LAST,

		// Messages that are passed around between any objects
		MESSAGE_ID_FIRST,
			MESSAGE_SPAWN,
			MESSAGE_CHANGETEAM,
			MESSAGE_INVALIDTEAM,
			MESSAGE_INVALIDCLASS,
			MESSAGE_CHANGECLASS,
			MESSAGE_DEATH,
			MESSAGE_HEALED,
			MESSAGE_REVIVED,
			MESSAGE_KILLEDSOMEONE,
			MESSAGE_ADDWEAPON,		// gives a weapon to the bot, should add to list to be evaluated for use
			MESSAGE_REMOVEWEAPON,	// remove a weapon from the bots inventory
			MESSAGE_RESETWEAPONS,	// tells the bot to clear out all the weapons
			MESSAGE_REFRESHWEAPON,
			MESSAGE_REFRESHALLWEAPONS,
			MESSAGE_SPECTATED,
			MESSAGE_AIMCOMPLETED,
			MESSAGE_SCRIPTMSG,
			MESSAGE_PROXIMITY_TRIGGER,
			MESSAGE_DYNAMIC_PATHS_CHANGED,
			MESSAGE_ENT_ENTER_RADIUS,
			MESSAGE_ENT_LEAVE_RADIUS,
			MESSAGE_MG_ENTER_RADIUS,
			MESSAGE_MG_LEAVE_RADIUS,
		MESSAGE_ID_LAST,

		// Percepts  (senses: feel, see, hear, smell, )
		PERCEPT_ID_FIRST,
			PERCEPT_FEEL_PLAYER_USE,
			PERCEPT_FEEL_PAIN,
			PERCEPT_HEAR_VOICEMACRO,
			PERCEPT_HEAR_GLOBALVOICEMACRO,
			PERCEPT_HEAR_TEAMVOICEMACRO,
			PERCEPT_HEAR_PRIVATEVOICEMACRO,
			PERCEPT_HEAR_GLOBALCHATMSG,
			PERCEPT_HEAR_TEAMCHATMSG,
			PERCEPT_HEAR_PRIVCHATMSG,
			PERCEPT_HEAR_GROUPCHATMSG,
			PERCEPT_HEAR_SOUND,
			PERCEPT_SENSE_ENTITY,
		PERCEPT_ID_LAST,
	EVENT_ID_LAST,
	EVENT_NUM_EVENTS
} EventId;

////////////////////////////////////////////////////////////////////////

// enumerations: GameMessage
//		GEN_MSG_NONE - Invalid message reserved as 0.
//		GEN_MSG_ADDBOT - Bot adding info.
//		GEN_MSG_ISALIVE - Is the entity alive?
//		GEN_MSG_ISRELOADING - Is the entity reloading?
//		GEN_MSG_ISREADYTOFIRE - Is the entity ready to fire?
//		GEN_MSG_ISALLIED - Is the entity allied with another?
//		GEN_MSG_ISOUTSIDE - Is this position outdoors?
//		GEN_MSG_GETEQUIPPEDWEAPON - Get the currently equipped weapon id for an entity.
//		GEN_MSG_GETMOUNTEDWEAPON - Gets the weapon id for any weapon the bot is mounted and controlling.
//		GEN_MSG_GETHEALTHARMOR - Get health and armor for an entity.
//		GEN_MSG_GETMAXSPEED - Get the max speed of the entity.
//		GEN_MSG_GETFLAGSTATE - Get the current state of the flag.
//		GEN_MSG_GAMESTATE - Get the current state of the game.
//		GEN_MSG_ENTITYSCORE - Get current frags/kills/score of an entity.
//		GEN_MSG_TEAMSCORE - Get current team score of a team.
//		GEN_MSG_WPCHARGED - Is the weapon charged?
//		GEN_MSG_WPHEATLEVEL - Get the weapon heat level.
//		GEN_MSG_ENTITYKILL - Kill a passed in entity, cheat protected.
//		GEN_MSG_SERVERCOMMAND - Execute a server command.
typedef enum
{
	GEN_MSG_NONE = 0,
	GEN_MSG_ADDBOT,
	GEN_MSG_KICKBOT,
	GEN_MSG_ISALIVE,
	GEN_MSG_ISRELOADING,
	GEN_MSG_ISREADYTOFIRE,
	GEN_MSG_ISALLIED,
	GEN_MSG_ISOUTSIDE,
	GEN_MSG_CHANGENAME,

	GEN_MSG_GETEQUIPPEDWEAPON,
	GEN_MSG_GETMOUNTEDWEAPON,
	GEN_MSG_GETWEAPONLIMITS,
	GEN_MSG_GETHEALTHARMOR,
	GEN_MSG_GETMAXSPEED,
	GEN_MSG_GETFLAGSTATE,
	GEN_MSG_GETCONTROLLINGTEAM,
	GEN_MSG_GAMESTATE,

	GEN_MSG_ENTITYSTAT,
	GEN_MSG_TEAMSTAT,

	GEN_MSG_WPCHARGED,
	GEN_MSG_WPHEATLEVEL,

	GEN_MSG_ENTITYKILL,
	GEN_MSG_SERVERCOMMAND,
	GEN_MSG_PLAYSOUND,
	GEN_MSG_STOPSOUND,
	GEN_MSG_SCRIPTEVENT,

	GEN_MSG_GOTOWAYPOINT,
	GEN_MSG_VEHICLEINFO,
	GEN_MSG_MOVERAT,

	GEN_MSG_SETLOADOUT,

	// This must stay last.
	GEN_MSG_END
} GEN_GameMessage;

// enumerations: BlackBoard_Key
//		bbk_All - Special identifier for ALL keys.
//		bbk_DelayGoal - Goal delayed for the duration of this blackboard entry.
typedef enum
{
	bbk_All = 0,
	bbk_DelayGoal,
	bbk_IsTaken,
	bbk_RunAway,
	
	// This must stay last.
	bbk_LastKey,
	bbk_FirstScript,
} BlackBoard_Key;

#endif
