/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Daemon GPL Source Code (Daemon Source Code).  

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the Daemon 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

// g_vote.c: All callvote handling
// -------------------------------
//
#include "g_local.h"
#include "../../ui/menudef.h"	// For vote options



#define T_FFA   0x01
#define T_1V1   0x02
#define T_SP    0x04
#define T_TDM   0x08
#define T_CTF   0x10
#define T_WLF   0x20
#define T_WSW   0x40
#define T_WCP   0x80
#define T_WCH   0x100


static const char *ACTIVATED = "ACTIVATED";
static const char *DEACTIVATED = "DEACTIVATED";
static const char *ENABLED = "ENABLED";
static const char *DISABLED = "DISABLED";

static const char *gameNames[] = {
	"Single Player",
	"Cooperative",
	"Objective",
	"Stopwatch",
	"Campaign",
	"Last Man Standing"
};


//
// Update info:
//  1. Add line to aVoteInfo w/appropriate info
//  2. Add implementation for attempt and success (see an existing command for an example)
//
typedef struct
{
	unsigned int    dwGameTypes;
	const char     *pszVoteName;
	int             (*pVoteCommand) (gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
	const char     *pszVoteMessage;
	const char     *pszVoteHelp;
} vote_reference_t;

// VC optimizes for dup strings :)
static const vote_reference_t aVoteInfo[] = {
	{0x1ff, "comp", G_Comp_v, "Load Competition Settings", "^7\n  Loads standard competition settings for the current mode"},
	{0x1ff, "gametype", G_Gametype_v, "Set Gametype to", " <value>^7\n  Changes the current gametype"},
	{0x1ff, "kick", G_Kick_v, "KICK", " <player_id>^7\n  Attempts to kick player from server"},
	{0x1ff, "mute", G_Mute_v, "MUTE", " <player_id>^7\n  Removes the chat capabilities of a player"},
	{0x1ff, "unmute", G_UnMute_v, "UN-MUTE", " <player_id>^7\n  Restores the chat capabilities of a player"},
	{0x1ff, "map", G_Map_v, "Change map to", " <mapname>^7\n  Votes for a new map to be loaded"},
	{0x1ff, "campaign", G_Campaign_v, "Change campaign to", " <campaign>^7\n  Votes for a new map to be loaded"},
	{0x1ff, "maprestart", G_MapRestart_v, "Map Restart", "^7\n  Restarts the current map in progress"},
	{0x1ff, "matchreset", G_MatchReset_v, "Match Reset", "^7\n  Resets the entire match"},
	{0x1ff, "mutespecs", G_Mutespecs_v, "Mute Spectators", " <0|1>^7\n  Mutes in-game spectator chat"},
	{0x1ff, "nextmap", G_Nextmap_v, "Load Next Map", "^7\n  Loads the next map or campaign in the map queue"},
	{0x1ff, "pub", G_Pub_v, "Load Public Settings", "^7\n  Loads standard public settings for the current mode"},
	{0x1ff, "referee", G_Referee_v, "Referee", " <player_id>^7\n  Elects a player to have admin abilities"},
	{0x1ff, "shuffleteamsxp", G_ShuffleTeams_v, "Shuffle Teams by XP", " ^7\n  Randomly place players on each team, based on XP"},
	{0x1ff, "startmatch", G_StartMatch_v, "Start Match", " ^7\n  Sets all players to \"ready\" status to start the match"},
	{0x1ff, "swapteams", G_SwapTeams_v, "Swap Teams", " ^7\n  Switch the players on each team"},
	{0x1ff, "friendlyfire", G_FriendlyFire_v, "Friendly Fire", " <0|1>^7\n  Toggles ability to hurt teammates"},
	{0x1ff, "timelimit", G_Timelimit_v, "Timelimit", " <value>^7\n  Changes the current timelimit"},
	{0x1ff, "unreferee", G_Unreferee_v, "UNReferee", " <player_id>^7\n  Elects a player to have admin abilities removed"},
	{0x1ff, "warmupdamage", G_Warmupfire_v, "Warmup Damage",
	 " <0|1|2>^7\n  Specifies if players can inflict damage during warmup"},
	{0x1ff, "antilag", G_AntiLag_v, "Anti-Lag", " <0|1>^7\n  Toggles Anit-Lag on the server"},
	{0x1ff, "balancedteams", G_BalancedTeams_v, "Balanced Teams", " <0|1>^7\n  Toggles team balance forcing"},
	{0, 0, NULL, 0}
};


// Checks for valid custom callvote requests from the client.
int G_voteCmdCheck(gentity_t * ent, char *arg, char *arg2, qboolean fRefereeCmd)
{
	unsigned int    i, cVoteCommands = sizeof(aVoteInfo) / sizeof(aVoteInfo[0]);

	for(i = 0; i < cVoteCommands; i++)
	{
		if(!Q_stricmp(arg, aVoteInfo[i].pszVoteName))
		{
			int             hResult = aVoteInfo[i].pVoteCommand(ent, i, arg, arg2, fRefereeCmd);

			if(hResult == G_OK)
			{
				Q_strncpyz(arg, aVoteInfo[i].pszVoteMessage, VOTE_MAXSTRING);
				level.voteInfo.vote_fn = aVoteInfo[i].pVoteCommand;
			}
			else
			{
				level.voteInfo.vote_fn = NULL;
			}

			return (hResult);
		}
	}

	return (G_NOTFOUND);
}


// Voting help summary.
void G_voteHelp(gentity_t * ent, qboolean fShowVote)
{
	int             i, rows = 0, num_cmds = sizeof(aVoteInfo) / sizeof(aVoteInfo[0]) - 1;	// Remove terminator;
	int             vi[100];	// Just make it large static.


	if(fShowVote)
	{
		CP("print \"\nValid ^3callvote^7 commands are:\n^3----------------------------\n\"");
	}

	for(i = 0; i < num_cmds; i++)
	{
		if(aVoteInfo[i].dwGameTypes & (1 << g_gametype.integer))
		{
			vi[rows++] = i;
		}
	}

	num_cmds = rows;
	rows = num_cmds / HELP_COLUMNS;

	if(num_cmds % HELP_COLUMNS)
	{
		rows++;
	}
	if(rows < 0)
	{
		return;
	}

	for(i = 0; i < rows; i++)
	{
		if(i + rows * 3 + 1 <= num_cmds)
		{
			G_refPrintf(ent, "^5%-17s%-17s%-17s%-17s", aVoteInfo[vi[i]].pszVoteName,
						aVoteInfo[vi[i + rows]].pszVoteName,
						aVoteInfo[vi[i + rows * 2]].pszVoteName, aVoteInfo[vi[i + rows * 3]].pszVoteName);
		}
		else if(i + rows * 2 + 1 <= num_cmds)
		{
			G_refPrintf(ent, "^5%-17s%-17s%-17s", aVoteInfo[vi[i]].pszVoteName,
						aVoteInfo[vi[i + rows]].pszVoteName, aVoteInfo[vi[i + rows * 2]].pszVoteName);
		}
		else if(i + rows + 1 <= num_cmds)
		{
			G_refPrintf(ent, "^5%-17s%-17s", aVoteInfo[vi[i]].pszVoteName, aVoteInfo[vi[i + rows]].pszVoteName);
		}
		else
		{
			G_refPrintf(ent, "^5%-17s", aVoteInfo[vi[i]].pszVoteName);
		}
	}

	if(fShowVote)
	{
		CP("print \"\nUsage: ^3\\callvote <command> <params>\n^7For current settings/help, use: ^3\\callvote <command> ?\n\n\"");
	}

	return;
}

// Set disabled votes for client UI
void G_voteFlags(void)
{
	int             i, flags = 0;

	for(i = 0; i < numVotesAvailable; i++)
	{
		if(trap_Cvar_VariableIntegerValue(voteToggles[i].pszCvar) == 0)
		{
			flags |= voteToggles[i].flag;
		}
	}

	if(flags != voteFlags.integer)
	{
		trap_Cvar_Set("voteFlags", va("%d", flags));
	}
}

// Prints specific callvote command help description.
qboolean G_voteDescription(gentity_t * ent, qboolean fRefereeCmd, int cmd)
{
	char            arg[MAX_TOKEN_CHARS];
	char           *ref_cmd = (fRefereeCmd) ? "\\ref" : "\\callvote";

	if(!ent)
	{
		return (qfalse);
	}

	trap_Argv(2, arg, sizeof(arg));
	if(!Q_stricmp(arg, "?") || trap_Argc() == 2)
	{
		trap_Argv(1, arg, sizeof(arg));
		G_refPrintf(ent, "\nUsage: ^3%s %s%s\n", ref_cmd, arg, aVoteInfo[cmd].pszVoteHelp);
		return (qtrue);
	}

	return (qfalse);
}


// Localize disable message info.
void G_voteDisableMessage(gentity_t * ent, const char *cmd)
{
	G_refPrintf(ent, "Sorry, [lof]^3%s^7 [lon]voting has been disabled", cmd);
}


// Player ID message stub.
void G_playersMessage(gentity_t * ent)
{
	G_refPrintf(ent, "Use the ^3players^7 command to find a valid player ID.");
}


// Localize current parameter setting.
void G_voteCurrentSetting(gentity_t * ent, const char *cmd, const char *setting)
{
	G_refPrintf(ent, "^2%s^7 is currently ^3%s\n", cmd, setting);
}


// Vote toggling
int G_voteProcessOnOff(gentity_t * ent, char *arg, char *arg2, qboolean fRefereeCmd, int curr_setting, int vote_allow,
					   int vote_type)
{
	if(!vote_allow && ent && !ent->client->sess.referee)
	{
		G_voteDisableMessage(ent, aVoteInfo[vote_type].pszVoteName);
		G_voteCurrentSetting(ent, aVoteInfo[vote_type].pszVoteName, ((curr_setting) ? ENABLED : DISABLED));
		return (G_INVALID);
	}
	if(G_voteDescription(ent, fRefereeCmd, vote_type))
	{
		G_voteCurrentSetting(ent, aVoteInfo[vote_type].pszVoteName, ((curr_setting) ? ENABLED : DISABLED));
		return (G_INVALID);
	}

	if((atoi(arg2) && curr_setting) || (!atoi(arg2) && !curr_setting))
	{
		G_refPrintf(ent, "^3%s^5 is already %s!", aVoteInfo[vote_type].pszVoteName, ((curr_setting) ? ENABLED : DISABLED));
		return (G_INVALID);
	}

	Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);
	Com_sprintf(arg2, VOTE_MAXSTRING, "%s", (atoi(arg2)) ? ACTIVATED : DEACTIVATED);

	return (G_OK);
}


//
// Several commands to help with cvar setting
//
void G_voteSetOnOff(const char *desc, const char *cvar)
{
	AP(va("cpm \"^3%s is: ^5%s\n\"", desc, (atoi(level.voteInfo.vote_value)) ? ENABLED : DISABLED));
	//trap_SendConsoleCommand(EXEC_APPEND, va("%s %s\n", cvar, level.voteInfo.vote_value));
	trap_Cvar_Set(cvar, level.voteInfo.vote_value);
}

void G_voteSetValue(const char *desc, const char *cvar)
{
	AP(va("cpm \"^3%s set to: ^5%s\n\"", desc, level.voteInfo.vote_value));
	//trap_SendConsoleCommand(EXEC_APPEND, va("%s %s\n", cvar, level.voteInfo.vote_value));
	trap_Cvar_Set(cvar, level.voteInfo.vote_value);
}

void G_voteSetVoteString(const char *desc)
{
	AP(va("print \"^3%s set to: ^5%s\n\"", desc, level.voteInfo.vote_value));
	trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.voteInfo.voteString));
}






////////////////////////////////////////////////////////
//
// Actual vote command implementation
//
////////////////////////////////////////////////////////




// *** Load competition settings for current mode ***
int G_Comp_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		if(trap_Argc() > 2)
		{
			G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg,
						aVoteInfo[dwVoteIndex].pszVoteHelp);
			return (G_INVALID);
		}
		else if(vote_allow_comp.integer <= 0 && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return (G_INVALID);
		}

		// Vote action (vote has passed)
	}
	else
	{
		// Load in comp settings for current gametype
		G_configSet(g_gametype.integer, qtrue);
		// CHRUKER: b042 - Was using cp, but subsequent cp calls quickly removed it, so now its using cpm
		AP("cpm \"Competition Settings Loaded!\n\"");
	}

	return (G_OK);
}


void G_GametypeList(gentity_t * ent)
{
	int             i;

	G_refPrintf(ent, "\nAvailable gametypes:\n--------------------");

	for(i = GT_WOLF; i < GT_MAX_GAME_TYPE; i++)
	{
		if(i != GT_WOLF_CAMPAIGN)
		{
			G_refPrintf(ent, "  %d ^3(%s)", i, gameNames[i]);
		}
	}

	G_refPrintf(ent, "\n");
}

// *** GameType ***
int G_Gametype_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		int             i = atoi(arg2);

		if(!vote_allow_gametype.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			G_GametypeList(ent);
			G_voteCurrentSetting(ent, arg, va("%d (%s)", g_gametype.integer, gameNames[g_gametype.integer]));
			return (G_INVALID);
		}
		else if(G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			G_GametypeList(ent);
			G_voteCurrentSetting(ent, arg, va("%d (%s)", g_gametype.integer, gameNames[g_gametype.integer]));
			return (G_INVALID);
		}

		if(i < GT_WOLF || i >= GT_MAX_GAME_TYPE || i == GT_WOLF_CAMPAIGN)
		{
			G_refPrintf(ent, "\n^3Invalid gametype: ^7%d", i);
			G_GametypeList(ent);
			return (G_INVALID);
		}

		if(i == g_gametype.integer)
		{
			G_refPrintf(ent, "\n^3Gametype^5 is already set to %s!", gameNames[i]);
			return (G_INVALID);
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", gameNames[i]);

		// Vote action (vote has passed)
	}
	else
	{
		// Set gametype
		G_voteSetValue("Gametype", "g_gametype");
		Svcmd_ResetMatch_f(qtrue, qtrue);
	}

	return (G_OK);
}


// *** Player Kick ***
int G_Kick_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		int             pid;

		if(!vote_allow_kick.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return G_INVALID;
		}
		else if(G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return G_INVALID;
		}
		else if((pid = ClientNumberFromString(ent, arg2)) == -1)
		{
			return G_INVALID;
		}

		if(level.clients[pid].sess.referee)
		{
			G_refPrintf(ent, "Can't vote to kick referees!");
			return G_INVALID;
		}

		if(!fRefereeCmd && ent)
		{
			if(level.clients[pid].sess.sessionTeam != TEAM_SPECTATOR &&
			   level.clients[pid].sess.sessionTeam != ent->client->sess.sessionTeam)
			{
				G_refPrintf(ent, "Can't vote to kick players on opposing team!");
				return G_INVALID;
			}
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname);

		// Vote action (vote has passed)
	}
	else
	{
		// Kick a player
		trap_SendConsoleCommand(EXEC_APPEND, va("clientkick %d\n", atoi(level.voteInfo.vote_value)));
		AP(va("cp \"%s\n^3has been kicked!\n\"", level.clients[atoi(level.voteInfo.vote_value)].pers.netname));
	}

	return G_OK;
}

// *** Player Mute ***
int G_Mute_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	if(fRefereeCmd)
	{
		// handled elsewhere
		return (G_NOTFOUND);
	}

	// Vote request (vote is being initiated)
	if(arg)
	{
		int             pid;

		if(!vote_allow_muting.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return (G_INVALID);
		}
		else if(G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return (G_INVALID);
		}
		else if((pid = ClientNumberFromString(ent, arg2)) == -1)
		{
			return (G_INVALID);
		}

		if(level.clients[pid].sess.referee)
		{
			G_refPrintf(ent, "Can't vote to mute referees!");
			return (G_INVALID);
		}

		if(level.clients[pid].sess.muted)
		{
			G_refPrintf(ent, "Player is already muted!");
			return (G_INVALID);
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname);

		// Vote action (vote has passed)
	}
	else
	{
		int             pid = atoi(level.voteInfo.vote_value);

		// Mute a player
		if(level.clients[pid].sess.referee != RL_RCON)
		{
			trap_SendServerCommand(pid, va("cpm \"^3You have been muted\""));
			level.clients[pid].sess.muted = qtrue;
			AP(va("cp \"%s\n^3has been muted!\n\"", level.clients[pid].pers.netname));
			ClientUserinfoChanged(pid);
		}
		else
		{
			G_Printf("Cannot mute a referee.\n");
		}
	}

	return (G_OK);
}

// *** Player Un-Mute ***
int G_UnMute_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	if(fRefereeCmd)
	{
		// handled elsewhere
		return (G_NOTFOUND);
	}

	// Vote request (vote is being initiated)
	if(arg)
	{
		int             pid;

		if(!vote_allow_muting.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return (G_INVALID);
		}
		else if(G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return (G_INVALID);
		}
		else if((pid = ClientNumberFromString(ent, arg2)) == -1)
		{
			return (G_INVALID);
		}

		if(!level.clients[pid].sess.muted)
		{
			G_refPrintf(ent, "Player is not muted!");
			return (G_INVALID);
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname);

		// Vote action (vote has passed)
	}
	else
	{
		int             pid = atoi(level.voteInfo.vote_value);

		// Mute a player
		if(level.clients[pid].sess.referee != RL_RCON)
		{
			trap_SendServerCommand(pid, va("cpm \"^3You have been un-muted\""));
			level.clients[pid].sess.muted = qfalse;
			AP(va("cp \"%s\n^3has been un-muted!\n\"", level.clients[pid].pers.netname));
			ClientUserinfoChanged(pid);
		}
		else
		{
			G_Printf("Cannot un-mute a referee.\n");
		}
	}

	return (G_OK);
}

// *** Map - simpleton: we dont verify map is allowed/exists ***
int G_Map_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		char            serverinfo[MAX_INFO_STRING];

		trap_GetServerinfo(serverinfo, sizeof(serverinfo));

		if(!vote_allow_map.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			G_voteCurrentSetting(ent, arg, Info_ValueForKey(serverinfo, "mapname"));
			return (G_INVALID);
		}
		else if(G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			G_voteCurrentSetting(ent, arg, Info_ValueForKey(serverinfo, "mapname"));
			return (G_INVALID);
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);

		// Vote action (vote has passed)
	}
	else
	{
		char            s[MAX_STRING_CHARS];

		if(g_gametype.integer == GT_WOLF_CAMPAIGN)
		{
			trap_Cvar_VariableStringBuffer("nextcampaign", s, sizeof(s));
			trap_SendConsoleCommand(EXEC_APPEND,
									va("campaign %s%s\n", level.voteInfo.vote_value,
									   ((*s) ? va("; set nextcampaign \"%s\"", s) : "")));
		}
		else
		{
			Svcmd_ResetMatch_f(qtrue, qfalse);
			trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
			trap_SendConsoleCommand(EXEC_APPEND,
									va("map %s%s\n", level.voteInfo.vote_value, ((*s) ? va("; set nextmap \"%s\"", s) : "")));
		}
	}

	return (G_OK);
}

// *** Campaign - simpleton: we dont verify map is allowed/exists ***
int G_Campaign_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		char            serverinfo[MAX_INFO_STRING];

		trap_GetServerinfo(serverinfo, sizeof(serverinfo));

		if(!vote_allow_map.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			if(g_gametype.integer == GT_WOLF_CAMPAIGN)
			{
				G_voteCurrentSetting(ent, arg, g_campaigns[level.currentCampaign].shortname);
			}
			return (G_INVALID);
		}
		else if(G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			if(g_gametype.integer == GT_WOLF_CAMPAIGN)
			{
				G_voteCurrentSetting(ent, arg, g_campaigns[level.currentCampaign].shortname);
			}
			return (G_INVALID);
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);

		// Vote action (vote has passed)
	}
	else
	{
		char            s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer("nextcampaign", s, sizeof(s));
		trap_SendConsoleCommand(EXEC_APPEND,
								va("campaign %s%s\n", level.voteInfo.vote_value,
								   ((*s) ? va("; set nextcampaign \"%s\"", s) : "")));
	}

	return (G_OK);
}

// *** Map Restart ***
int G_MapRestart_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		if(trap_Argc() > 2)
		{
			if(!Q_stricmp(arg2, "?"))
			{
				G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg,
							aVoteInfo[dwVoteIndex].pszVoteHelp);
				return (G_INVALID);
			}
		}

		// Vote action (vote has passed)
	}
	else
	{
		// Restart the map back to warmup
		Svcmd_ResetMatch_f(qfalse, qtrue);
		AP("cp \"^1*** Level Restarted! ***\n\"");
	}

	return (G_OK);
}


// *** Match Restart ***
int G_MatchReset_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		if(!vote_allow_matchreset.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return (G_INVALID);
		}
		else if(trap_Argc() != 2 && G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return (G_INVALID);
//      } else if(trap_Argc() > 2) {
//          if(!Q_stricmp(arg2, "?")) {
//              G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg, aVoteInfo[dwVoteIndex].pszVoteHelp);
//              return(G_INVALID);
//          }
		}

		// Vote action (vote has passed)
	}
	else
	{
		// Restart the map back to warmup
		Svcmd_ResetMatch_f(qtrue, qtrue);
		AP("cp \"^1*** Match Reset! ***\n\"");
	}

	return (G_OK);
}


// *** Mute Spectators ***
int G_Mutespecs_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		return (G_voteProcessOnOff(ent, arg, arg2, fRefereeCmd,
								   !!(match_mutespecs.integer), vote_allow_mutespecs.integer, dwVoteIndex));

		// Vote action (vote has passed)
	}
	else
	{
		// Mute/unmute spectators
		G_voteSetOnOff("Spectator Muting", "match_mutespecs");
	}

	return (G_OK);
}


// *** Nextmap ***
int G_Nextmap_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		if(trap_Argc() > 2)
		{
			G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg,
						aVoteInfo[dwVoteIndex].pszVoteHelp);
			return (G_INVALID);
		}
		else if(!vote_allow_nextmap.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return (G_INVALID);
		}
		else
		{
			char            s[MAX_STRING_CHARS];

			if(g_gametype.integer == GT_WOLF_CAMPAIGN)
			{
				trap_Cvar_VariableStringBuffer("nextcampaign", s, sizeof(s));
				if(!*s)
				{
					G_refPrintf(ent, "'nextcampaign' is not set.");
					return (G_INVALID);
				}
			}
			else
			{
				trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
				if(!*s)
				{
					G_refPrintf(ent, "'nextmap' is not set.");
					return (G_INVALID);
				}
			}
		}

		// Vote action (vote has passed)
	}
	else
	{
		if(g_gametype.integer == GT_WOLF_CAMPAIGN)
		{
			// Load in the nextcampaign
			trap_SendConsoleCommand(EXEC_APPEND, "vstr nextcampaign\n");
			AP("cp \"^3*** Loading nextcampaign! ***\n\"");
		}
		else
		{
			// Load in the nextmap
			trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");
			AP("cp \"^3*** Loading nextmap! ***\n\"");
		}
	}

	return (G_OK);
}


// *** Load public settings for current mode ***
int G_Pub_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		if(trap_Argc() > 2)
		{
			G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg,
						aVoteInfo[dwVoteIndex].pszVoteHelp);
			return (G_INVALID);
		}
		else if(vote_allow_pub.integer <= 0 && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return (G_INVALID);
		}

		// Vote action (vote has passed)
	}
	else
	{
		// Load in pub settings for current gametype
		G_configSet(g_gametype.integer, qfalse);
		// CHRUKER: b042 - Was using cp, but subsequent cp calls quickly removed it, so now its using cpm
		AP("cpm \"Public Settings Loaded!\n\"");
	}

	return (G_OK);
}


// *** Referee voting ***
int G_Referee_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		int             pid;

		if(!vote_allow_referee.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return (G_INVALID);
		}

		if(!ent->client->sess.referee && level.numPlayingClients < 3)
		{
			G_refPrintf(ent, "Sorry, not enough clients in the game to vote for a referee");
			return (G_INVALID);
		}

		if(ent->client->sess.referee && trap_Argc() == 2)
		{
			G_playersMessage(ent);
			return (G_INVALID);
		}
		else if(trap_Argc() == 2)
		{
			pid = ent - g_entities;
		}
		else if(G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return (G_INVALID);
		}
		else if((pid = ClientNumberFromString(ent, arg2)) == -1)
		{
			return (G_INVALID);
		}

		if(level.clients[pid].sess.referee)
		{
			G_refPrintf(ent, "[lof]%s [lon]is already a referee!", level.clients[pid].pers.netname);
			return (-1);
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname);

		// Vote action (vote has passed)
	}
	else
	{
		// Voting in a new referee
		gclient_t      *cl = &level.clients[atoi(level.voteInfo.vote_value)];

		if(cl->pers.connected == CON_DISCONNECTED)
		{
			AP("print \"Player left before becoming referee\n\"");
		}
		else
		{
			cl->sess.referee = RL_REFEREE;	// FIXME: Differentiate voted refs from passworded refs
			cl->sess.spec_invite = TEAM_AXIS | TEAM_ALLIES;
			AP(va("cp \"%s^7 is now a referee\n\"", cl->pers.netname));
			ClientUserinfoChanged(atoi(level.voteInfo.vote_value));
		}
	}
	return (G_OK);
}


// *** Shuffle teams
int G_ShuffleTeams_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		if(trap_Argc() > 2)
		{
			G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg, // CHRUKER: b047 - Removed unneeded linebreak
						aVoteInfo[dwVoteIndex].pszVoteHelp);
			return (G_INVALID);
		}
		else if(!vote_allow_shuffleteamsxp.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return (G_INVALID);
		}

		// Vote action (vote has passed)
	}
	else
	{
		// Swap the teams!
		Svcmd_ShuffleTeams_f();
	}

	return (G_OK);
}


// *** Start Match ***
int G_StartMatch_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		if(trap_Argc() > 2)
		{
			if(!Q_stricmp(arg2, "?"))
			{
				G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg,
							aVoteInfo[dwVoteIndex].pszVoteHelp);
				return (G_INVALID);
			}
		}

		if(g_gamestate.integer == GS_PLAYING || g_gamestate.integer == GS_INTERMISSION)
		{
			G_refPrintf(ent, "^3Match is already in progress!");
			return (G_INVALID);
		}

		if(g_gamestate.integer == GS_WARMUP_COUNTDOWN)
		{
			G_refPrintf(ent, "^3Countdown already started!");
			return (G_INVALID);
		}

		if(level.numPlayingClients < match_minplayers.integer)
		{
			G_refPrintf(ent, "^3Not enough players to start match!");
			return (G_INVALID);
		}

		// Vote action (vote has passed)
	}
	else
	{
		// Set everyone to "ready" status
		G_refAllReady_cmd(NULL);
	}

	return (G_OK);
}


// *** Swap teams
int G_SwapTeams_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		if(trap_Argc() > 2)
		{
			G_refPrintf(ent, "Usage: ^3%s %s%s\n", ((fRefereeCmd) ? "\\ref" : "\\callvote"), arg,
						aVoteInfo[dwVoteIndex].pszVoteHelp);
			return (G_INVALID);
		}
		else if(!vote_allow_swapteams.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return (G_INVALID);
		}

		// Vote action (vote has passed)
	}
	else
	{
		// Swap the teams!
		Svcmd_SwapTeams_f();
	}

	return (G_OK);
}


// *** Team Damage ***
int G_FriendlyFire_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		return (G_voteProcessOnOff(ent, arg, arg2, fRefereeCmd,
								   !!(g_friendlyFire.integer), vote_allow_friendlyfire.integer, dwVoteIndex));

		// Vote action (vote has passed)
	}
	else
	{
		// Team damage (friendlyFire)
		G_voteSetOnOff("Friendly Fire", "g_friendlyFire");
	}

	return (G_OK);
}

// Anti-Lag
int G_AntiLag_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		return (G_voteProcessOnOff(ent, arg, arg2, fRefereeCmd, !!(g_antilag.integer), vote_allow_antilag.integer, dwVoteIndex));

		// Vote action (vote has passed)
	}
	else
	{
		// Anti-Lag (g_antilag)
		G_voteSetOnOff("Anti-Lag", "g_antilag");
	}

	return (G_OK);
}

// Balanced Teams
int G_BalancedTeams_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		return (G_voteProcessOnOff(ent, arg, arg2, fRefereeCmd,
								   !!(g_balancedteams.integer), vote_allow_balancedteams.integer, dwVoteIndex));
		// Vote action (vote has passed)
	}
	else
	{
		// Balanced Teams (g_balancedteams)
		G_voteSetOnOff("Balanced Teams", "g_balancedteams");
		trap_Cvar_Set("g_teamForceBalance", level.voteInfo.vote_value);
		trap_Cvar_Set("g_lms_teamForceBalance", level.voteInfo.vote_value);
	}

	return (G_OK);
}

// *** Timelimit ***
int G_Timelimit_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		if(!vote_allow_timelimit.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			G_voteCurrentSetting(ent, arg, g_timelimit.string);
			return (G_INVALID);
		}
		else if(G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			G_voteCurrentSetting(ent, arg, g_timelimit.string);
			return (G_INVALID);
		}
		else if(atoi(arg2) < 0)
		{
			G_refPrintf(ent, "Sorry, can't specify a timelimit < 0!");
			return (G_INVALID);
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);

		// Vote action (vote has passed)
	}
	else
	{
		// Timelimit change
		G_voteSetVoteString("Timelimit");
	}

	return (G_OK);
}

char           *warmupType[] = { "None", "Enemies Only", "Everyone" };

void G_WarmupDamageTypeList(gentity_t * ent)
{
	int             i;

	G_refPrintf(ent, "\nAvailable Warmup Damage types:\n------------------------------");
	for(i = 0; i < (sizeof(warmupType) / sizeof(char *)); i++)
		G_refPrintf(ent, "  %d ^3(%s)", i, warmupType[i]);
	G_refPrintf(ent, "\n");
}

// *** Warmup Weapon Fire ***
int G_Warmupfire_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		int             i = atoi(arg2), val = (match_warmupDamage.integer < 0) ? 0 :
			(match_warmupDamage.integer > 2) ? 2 : match_warmupDamage.integer;

		if(!vote_allow_warmupdamage.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			G_WarmupDamageTypeList(ent);
			G_voteCurrentSetting(ent, arg, va("%d (%s)", val, warmupType[val]));
			return (G_INVALID);
		}
		else if(G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			G_WarmupDamageTypeList(ent);
			G_voteCurrentSetting(ent, arg, va("%d (%s)", val, warmupType[val]));
			return (G_INVALID);
		}

		if(i < 0 || i > 2)
		{
			G_refPrintf(ent, "\n^3Invalid Warmup Damage type: ^7%d", i);
			G_WarmupDamageTypeList(ent);
			return (G_INVALID);
		}

		if(i == val)
		{
			G_refPrintf(ent, "\n^3Warmup Damage^5 is already set to %s!", warmupType[i]);
			return (G_INVALID);
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", warmupType[i]);


		// Vote action (vote has passed)
	}
	else
	{
		// Warmup damage setting
		// CHRUKER: b048 - Was using print, but this really should be displayed as a popup message.
		AP(va("cpm \"^3Warmup Damage set to: ^5%s\n\"", warmupType[atoi(level.voteInfo.vote_value)]));
		trap_SendConsoleCommand(EXEC_APPEND, va("match_warmupDamage %s\n", level.voteInfo.vote_value));
	}

	return (G_OK);
}


// *** Un-Referee voting ***
int G_Unreferee_v(gentity_t * ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd)
{
	// Vote request (vote is being initiated)
	if(arg)
	{
		int             pid;

		if(!vote_allow_referee.integer && ent && !ent->client->sess.referee)
		{
			G_voteDisableMessage(ent, arg);
			return (G_INVALID);
		}

		if(ent->client->sess.referee && trap_Argc() == 2)
		{
			G_playersMessage(ent);
			return (G_INVALID);
		}
		else if(trap_Argc() == 2)
		{
			pid = ent - g_entities;
		}
		else if(G_voteDescription(ent, fRefereeCmd, dwVoteIndex))
		{
			return (G_INVALID);
		}
		else if((pid = ClientNumberFromString(ent, arg2)) == -1)
		{
			return (G_INVALID);
		}

		if(level.clients[pid].sess.referee == RL_NONE)
		{
			G_refPrintf(ent, "[lof]%s [lon]isn't a referee!", level.clients[pid].pers.netname);
			return (G_INVALID);
		}

		if(level.clients[pid].sess.referee == RL_RCON)
		{
			G_refPrintf(ent, "[lof]%s's [lon]status cannot be removed", level.clients[pid].pers.netname);
			return (G_INVALID);
		}

		if(level.clients[pid].pers.localClient)
		{
			G_refPrintf(ent, "[lof]%s's [lon]is the Server Host", level.clients[pid].pers.netname);
			return (G_INVALID);
		}

		Com_sprintf(level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid);
		Com_sprintf(arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname);

		// Vote action (vote has passed)
	}
	else
	{
		// Stripping of referee status
		gclient_t      *cl = &level.clients[atoi(level.voteInfo.vote_value)];

		cl->sess.referee = RL_NONE;
		cl->sess.spec_invite = 0;
		AP(va("cp \"%s^7\nis no longer a referee\n\"", cl->pers.netname));
		ClientUserinfoChanged(atoi(level.voteInfo.vote_value));
	}

	return (G_OK);
}
