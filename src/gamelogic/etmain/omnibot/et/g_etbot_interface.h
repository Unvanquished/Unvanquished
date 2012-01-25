/*
 * ET <-> Omni-Bot interface header file.
 * 
 */

#ifndef __G_ETBOT_INTERFACE_H__
#define __G_ETBOT_INTERFACE_H__

//#include "q_shared.h"
#include "../../game/g_local.h"

//#define NO_BOT_SUPPORT

// IMPORTANT: when changed this has to be copied manually to GAMEVERSION (g_local.h)
#define OMNIBOT_NAME "etmain"

#define OMNIBOT_MODNAME "etmain"
#define OMNIBOT_MODVERSION "2.60d"

//////////////////////////////////////////////////////////////////////////
// g_OmniBotFlags bits
enum BotFlagOptions
{
	OBF_DONT_XPSAVE = (1 << 0),	// Disables XPSave for bots 
	OBF_DONT_MOUNT_TANKS = (1 << 1),	// Bots cannot mount tanks 
	OBF_DONT_MOUNT_GUNS = (1 << 2),	// Bots cannot mount emplaced guns
	OBF_DONT_SHOW_BOTCOUNT = (1 << 3),	// Don't count bots
	OBF_GIBBING = (1 << 4),		// Bots will target ungibbed enemies
	OBF_TRIGGER_MINES = (1 << 5),	// Bots will trigger team and spotted mines
	OBF_SHOVING = (1 << 6),		// Bots can use g_shove
	OBF_NEXT_FLAG = (1 << 16),	// mod specific flags start from here
};

//////////////////////////////////////////////////////////////////////////

int             Bot_Interface_Init();
void            Bot_Interface_InitHandles();
int             Bot_Interface_Shutdown();

void            Bot_Interface_Update();

void            Bot_Interface_ConsoleCommand();

qboolean        Bot_Util_AllowPush(int weaponId);
qboolean        Bot_Util_CheckForSuicide(gentity_t * ent);
const char     *_GetEntityName(gentity_t * _ent);

//void Bot_Util_AddGoal(gentity_t *_ent, int _goaltype, int _team, const char *_tag, obUserData *_bud);
void            Bot_Util_SendTrigger(gentity_t * _ent, gentity_t * _activator, const char *_tagname, const char *_action);

int             Bot_WeaponGameToBot(int weapon);
int             Bot_TeamGameToBot(int team);
int             Bot_PlayerClassGameToBot(int playerClass);

void            Bot_Queue_EntityCreated(gentity_t * pEnt);
void            Bot_Event_EntityDeleted(gentity_t * pEnt);

//////////////////////////////////////////////////////////////////////////

void            Bot_Event_ClientConnected(int _client, qboolean _isbot);
void            Bot_Event_ClientDisConnected(int _client);

void            Bot_Event_ResetWeapons(int _client);
void            Bot_Event_AddWeapon(int _client, int _weaponId);
void            Bot_Event_RemoveWeapon(int _client, int _weaponId);

void            Bot_Event_TakeDamage(int _client, gentity_t * _ent);
void            Bot_Event_Death(int _client, gentity_t * _killer, const char *_meansofdeath);
void            Bot_Event_KilledSomeone(int _client, gentity_t * _victim, const char *_meansofdeath);
void            Bot_Event_Revived(int _client, gentity_t * _whodoneit);
void            Bot_Event_Healed(int _client, gentity_t * _whodoneit);
void            Bot_Event_RecievedAmmo(int _client, gentity_t * _whodoneit);

void            Bot_Event_FireWeapon(int _client, int _weaponId, gentity_t * _projectile);
void            Bot_Event_PreTriggerMine(int _client, gentity_t * _mine);
void            Bot_Event_PostTriggerMine(int _client, gentity_t * _mine);
void            Bot_Event_MortarImpact(int _client, vec3_t _pos);

void            Bot_Event_Spectated(int _client, int _who);

void            Bot_Event_ChatMessage(int _client, gentity_t * _source, int _type, const char *_message);
void            Bot_Event_VoiceMacro(int _client, gentity_t * _source, int _type, const char *_message);

void            Bot_Event_Sound(gentity_t * _source, int _sndtype, const char *_name);

void            Bot_Event_FireTeamCreated(int _client, int _fireteamnum);
void            Bot_Event_FireTeamDestroyed(int _client);
void            Bot_Event_JoinedFireTeam(int _client, gentity_t * leader);
void            Bot_Event_LeftFireTeam(int _client);
void            Bot_Event_InviteFireTeam(int _inviter, int _invitee);
void            Bot_Event_FireTeam_Proposal(int _client, int _proposed);
void            Bot_Event_FireTeam_Warn(int _client, int _warned);

// goal helpers
void            Bot_AddDynamiteGoal(gentity_t * _ent, int _team, const char *_tag);
void            Bot_AddFallenTeammateGoals(gentity_t * _teammate, int _team);
void            AddDeferredGoal(gentity_t * ent);
void            UpdateGoalEntity(gentity_t * oldent, gentity_t * newent);
void            GetEntityCenter(gentity_t * ent, vec3_t pos);

#endif
