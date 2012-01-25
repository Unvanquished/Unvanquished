/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the OpenWolf GPL Source Code (OpenWolf Source Code).  

OpenWolf Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenWolf Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the OpenWolf Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the OpenWolf 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

// G_referee.c: Referee handling
// -------------------------------
// 04 Apr 02
// rhea@OrangeSmoothie.org
//
#include "g_local.h"
#include "../../ui/menudef.h"


//
// UGH!  Clean me!!!!
//

// Parses for a referee command.
//  --> ref arg allows for the server console to utilize all referee commands (ent == NULL)
//
qboolean G_refCommandCheck(gentity_t * ent, char *cmd)
{
	if(!Q_stricmp(cmd, "allready"))
	{
		G_refAllReady_cmd(ent);
	}
	else if(!Q_stricmp(cmd, "lock"))
	{
		G_refLockTeams_cmd(ent, qtrue);
	}
	else if(!Q_stricmp(cmd, "help"))
	{
		G_refHelp_cmd(ent);
	}
	else if(!Q_stricmp(cmd, "pause"))
	{
		G_refPause_cmd(ent, qtrue);
	}
	else if(!Q_stricmp(cmd, "putallies"))
	{
		G_refPlayerPut_cmd(ent, TEAM_ALLIES);
	}
	else if(!Q_stricmp(cmd, "putaxis"))
	{
		G_refPlayerPut_cmd(ent, TEAM_AXIS);
	}
	else if(!Q_stricmp(cmd, "remove"))
	{
		G_refRemove_cmd(ent);
	}
	else if(!Q_stricmp(cmd, "speclock"))
	{
		G_refSpeclockTeams_cmd(ent, qtrue);
	}
	else if(!Q_stricmp(cmd, "specunlock"))
	{
		G_refSpeclockTeams_cmd(ent, qfalse);
	}
	else if(!Q_stricmp(cmd, "unlock"))
	{
		G_refLockTeams_cmd(ent, qfalse);
	}
	else if(!Q_stricmp(cmd, "unpause"))
	{
		G_refPause_cmd(ent, qfalse);
	}
	else if(!Q_stricmp(cmd, "warmup"))
	{
		G_refWarmup_cmd(ent);
	}
	else if(!Q_stricmp(cmd, "warn"))
	{
		G_refWarning_cmd(ent);
	}
	else if(!Q_stricmp(cmd, "mute"))
	{
		G_refMute_cmd(ent, qtrue);
	}
	else if(!Q_stricmp(cmd, "unmute"))
	{
		G_refMute_cmd(ent, qfalse);
	}
	else
	{
		return (qfalse);
	}

	return (qtrue);
}


// Lists ref commands.
void G_refHelp_cmd(gentity_t * ent)
{
	// List commands only for enabled refs.
	if(ent)
	{
		if(!ent->client->sess.referee)
		{
			CP("cpm \"Sorry, you are not a referee!\n");
			return;
		}

		CP("print \"\n^3Referee commands:^7\n\"");
		CP("print \"------------------------------------------\n\"");

		G_voteHelp(ent, qfalse);

		// CHRUKER: b038 - Removed non-existing restart command
		// CHRUKER: b039 - Added <pid> parameter to remove command
		CP("print \"\n^5allready         putallies^7 <pid>  ^5specunlock       warn ^7<pid>\n\"");
		CP(  "print \"^5help             putaxis^7 <pid>    ^5unlock           mute ^7<pid>\n\"");
		CP(  "print \"^5lock             remove^7 <pid>     ^5unpause          unmute ^7<pid>\n\"");
		CP(  "print \"^5pause            speclock         warmup ^7[value]\n\"");

		CP("print \"Usage: ^3\\ref <cmd> [params]\n\n\"");

		// Help for the console
	}
	else
	{
		G_Printf("\nAdditional console commands:\n");
		G_Printf("----------------------------------------------\n");
		// CHRUKER: b038 - Removed non-existing restart command
		// CHRUKER: b039 - Added <pid> parameter to remove command
		G_Printf(  "allready        putallies <pid>    unpause\n");
		G_Printf(  "help            putaxis <pid>      warmup [value]\n");
		G_Printf(  "lock            speclock           warn <pid>\n");
		G_Printf(  "pause           specunlock\n");
		G_Printf(  "remove <pid>    unlock\n\n");

		G_Printf("Usage: <cmd> [params]\n\n");
	}
}


// Request for ref status or lists ref commands.
void G_ref_cmd(gentity_t * ent, unsigned int dwCommand, qboolean fValue)
{
	char            arg[MAX_TOKEN_CHARS];


	// Roll through ref commands if already a ref
	if(ent == NULL || ent->client->sess.referee)
	{
		voteInfo_t      votedata;

		trap_Argv(1, arg, sizeof(arg));

		memcpy(&votedata, &level.voteInfo, sizeof(voteInfo_t));

		if(Cmd_CallVote_f(ent, 0, qtrue))
		{
			memcpy(&level.voteInfo, &votedata, sizeof(voteInfo_t));
			return;
		}
		else
		{
			memcpy(&level.voteInfo, &votedata, sizeof(voteInfo_t));

			if(G_refCommandCheck(ent, arg))
			{
				return;
			}
			else
			{
				G_refHelp_cmd(ent);
			}
		}
		return;
	}

	if(ent)
	{
		if(!Q_stricmp(refereePassword.string, "none") || !refereePassword.string[0])
		{
			// CHRUKER: b046 - Was using the cpm command, but this is really just for the console.
			CP("print \"Sorry, referee status disabled on this server.\n\"");
			return;
		}

		if(trap_Argc() < 2)
		{
			// CHRUKER: b046 - Was using the cpm command, but this is really just for the console.
			CP("print \"Usage: ref [password]\n\"");
			return;
		}

		trap_Argv(1, arg, sizeof(arg));

		if(Q_stricmp(arg, refereePassword.string))
		{
			// CHRUKER: b046 - Was using the cpm command, but this is really just for the console.
			CP("print \"Invalid referee password!\n\"");
			return;
		}

		ent->client->sess.referee = 1;
		ent->client->sess.spec_invite = TEAM_AXIS | TEAM_ALLIES;
		AP(va("cp \"%s\n^3has become a referee\n\"", ent->client->pers.netname));
		ClientUserinfoChanged(ent - g_entities);
	}
}


// Readies all players in the game.
void G_refAllReady_cmd(gentity_t * ent)
{
	int             i;
	gclient_t      *cl;

	if(g_gamestate.integer == GS_PLAYING)
	{
// rain - #105 - allow allready in intermission
//  || g_gamestate.integer == GS_INTERMISSION) {
		G_refPrintf(ent, "Match already in progress!");
		return;
	}

	// Ready them all and lock the teams
	for(i = 0; i < level.numConnectedClients; i++)
	{
		cl = level.clients + level.sortedClients[i];
		if(cl->sess.sessionTeam != TEAM_SPECTATOR)
		{
			cl->pers.ready = qtrue;
		}
	}

	// Can we start?
	level.ref_allready = qtrue;
	G_readyMatchState();
}


// Changes team lock status
void G_refLockTeams_cmd(gentity_t * ent, qboolean fLock)
{
	char           *status;

	teamInfo[TEAM_AXIS].team_lock = (TeamCount(-1, TEAM_AXIS)) ? fLock : qfalse;
	teamInfo[TEAM_ALLIES].team_lock = (TeamCount(-1, TEAM_ALLIES)) ? fLock : qfalse;

	status = va("Referee has ^3%sLOCKED^7 teams", ((fLock) ? "" : "UN"));

	// CHRUKER: b041 - Was only sending this to ent, but it should be broadcasted
	G_printFull(status, NULL);
	G_refPrintf(ent, "You have %sLOCKED teams", ((fLock) ? "" : "UN")); // CHRUKER: b047 - Removed unneeded linebreak

	if(fLock)
	{
		level.server_settings |= CV_SVS_LOCKTEAMS;
	}
	else
	{
		level.server_settings &= ~CV_SVS_LOCKTEAMS;
	}
	trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
}


// Pause/unpause a match.
void G_refPause_cmd(gentity_t * ent, qboolean fPause)
{
	char           *status[2] = { "^5UN", "^1" };
	char           *referee = (ent) ? "Referee" : "ref";

	if((PAUSE_UNPAUSING >= level.match_pause && !fPause) || (PAUSE_NONE != level.match_pause && fPause))
	{
		G_refPrintf(ent, "The match is already %sPAUSED!", status[fPause]); 	// CHRUKER: b047 - Removed unneeded \" and linebreak
		return;
	}

	if(ent && !G_cmdDebounce(ent, ((fPause) ? "pause" : "unpause")))
	{
		return;
	}

	// Trigger the auto-handling of pauses
	if(fPause)
	{
		level.match_pause = 100 + ((ent) ? (1 + ent - g_entities) : 0);
		G_globalSound("sound/misc/referee.wav");
		G_spawnPrintf(DP_PAUSEINFO, level.time + 15000, NULL);
		AP(va("print \"^3%s ^1PAUSED^3 the match^3!\n", referee));
		AP(va("cp \"^3Match is ^1PAUSED^3! (^7%s^3)\n\"", referee));	// CHRUKER: b040 - Was only sending this to the client sending the command
		level.server_settings |= CV_SVS_PAUSE;
		trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
	}
	else
	{
		AP(va("print \"^3%s ^5UNPAUSES^3 the match ... resuming in 10 seconds!\n\"", referee)); // CHRUKER: b047 - Had extra linebreaks, before and after
		level.match_pause = PAUSE_UNPAUSING;
		G_globalSound("sound/osp/prepare.wav");
		G_spawnPrintf(DP_UNPAUSING, level.time + 10, NULL);
		return;
	}
}


// Puts a player on a team.
void G_refPlayerPut_cmd(gentity_t * ent, int team_id)
{
	int             pid;
	char            arg[MAX_TOKEN_CHARS];
	gentity_t      *player;

	// Works for teamplayish matches
	if(g_gametype.integer < GT_WOLF)
	{
		G_refPrintf(ent, "\"put[allies|axis]\" only for team-based games!");
		return;
	}

	// Find the player to place.
	trap_Argv(2, arg, sizeof(arg));
	if((pid = ClientNumberFromString(ent, arg)) == -1)
	{
		return;
	}

	player = g_entities + pid;

	// Can only move to other teams.
	if(player->client->sess.sessionTeam == team_id)
	{
		G_refPrintf(ent, "\"%s\" is already on team %s!", player->client->pers.netname, aTeams[team_id]); // CHRUKER: b047 - Removed unneeded linebreak
		return;
	}

	if(team_maxplayers.integer && TeamCount(-1, team_id) >= team_maxplayers.integer)
	{
		G_refPrintf(ent, "Sorry, the %s team is already full!", aTeams[team_id]); // CHRUKER: b047 - Removed unneeded linebreak
		return;
	}

	player->client->pers.invite = team_id;
	player->client->pers.ready = qfalse;

	if(team_id == TEAM_AXIS)
	{
		SetTeam(player, "red", qtrue, -1, -1, qfalse);
	}
	else
	{
		SetTeam(player, "blue", qtrue, -1, -1, qfalse);
	}

	if(g_gamestate.integer == GS_WARMUP || g_gamestate.integer == GS_WARMUP_COUNTDOWN)
	{
		G_readyMatchState();
	}
}


// Removes a player from a team.
void G_refRemove_cmd(gentity_t * ent)
{
	int             pid;
	char            arg[MAX_TOKEN_CHARS];
	gentity_t      *player;

	// Works for teamplayish matches
	if(g_gametype.integer < GT_WOLF)
	{
		G_refPrintf(ent, "\"remove\" only for team-based games!");
		return;
	}

	// Find the player to remove.
	trap_Argv(2, arg, sizeof(arg));
	if((pid = ClientNumberFromString(ent, arg)) == -1)
	{
		return;
	}

	player = g_entities + pid;

	// Can only remove active players.
	if(player->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		G_refPrintf(ent, "You can only remove people in the game!");
		return;
	}

	// Announce the removal
	AP(va("cp \"%s\n^7removed from team %s\n\"", player->client->pers.netname, aTeams[player->client->sess.sessionTeam]));
	CPx(pid, va("print \"^5You've been removed from the %s team\n\"", aTeams[player->client->sess.sessionTeam]));

	SetTeam(player, "s", qtrue, -1, -1, qfalse);

	if(g_gamestate.integer == GS_WARMUP || g_gamestate.integer == GS_WARMUP_COUNTDOWN)
	{
		G_readyMatchState();
	}
}


// Changes team spectator lock status
void G_refSpeclockTeams_cmd(gentity_t * ent, qboolean fLock)
{
	char           *status;

	// Ensure proper locking
	G_updateSpecLock(TEAM_AXIS, (TeamCount(-1, TEAM_AXIS)) ? fLock : qfalse);
	G_updateSpecLock(TEAM_ALLIES, (TeamCount(-1, TEAM_ALLIES)) ? fLock : qfalse);

	status = va("Referee has ^3SPECTATOR %sLOCKED^7 teams", ((fLock) ? "" : "UN"));

	// CHRUKER: b041 - Was only sending this to ent, but it should be broadcasted
	G_printFull(status, NULL);

	// Update viewers as necessary
//  G_pollMultiPlayers();

	if(fLock)
	{
		level.server_settings |= CV_SVS_LOCKSPECS;
	}
	else
	{
		level.server_settings &= ~CV_SVS_LOCKSPECS;
	}
	trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
}

void G_refWarmup_cmd(gentity_t * ent)
{
	char            cmd[MAX_TOKEN_CHARS];

	trap_Argv(2, cmd, sizeof(cmd));

	if(!*cmd || atoi(cmd) < 0)
	{
		trap_Cvar_VariableStringBuffer("g_warmup", cmd, sizeof(cmd));
		G_refPrintf(ent, "Warmup Time: %d", atoi(cmd)); // CHRUKER: b047 - Removed unneeded linebreak
		return;
	}

	trap_Cvar_Set("g_warmup", va("%d", atoi(cmd)));
}

void G_refWarning_cmd(gentity_t * ent)
{
	char            cmd[MAX_TOKEN_CHARS];
	char            reason[MAX_TOKEN_CHARS];
	int             kicknum;

	trap_Argv(2, cmd, sizeof(cmd));

	if(!*cmd)
	{
		G_refPrintf(ent, "usage: ref warn <clientname> [reason].");
		return;
	}

	trap_Argv(3, reason, sizeof(reason));

	kicknum = G_refClientnumForName(ent, cmd);

	if(kicknum != MAX_CLIENTS)
	{
		if(level.clients[kicknum].sess.referee == RL_NONE ||
		   ((!ent || ent->client->sess.referee == RL_RCON) && level.clients[kicknum].sess.referee <= RL_REFEREE))
		{
			trap_SendServerCommand(-1,
								   va("cpm \"%s^7 was issued a ^1Warning^7 (%s)\n\"\n", level.clients[kicknum].pers.netname,
									  *reason ? reason : "No Reason Supplied"));
		}
		else
		{
			G_refPrintf(ent, "Insufficient rights to issue client a warning.");
		}
	}
}

// (Un)Mutes a player
void G_refMute_cmd(gentity_t * ent, qboolean mute)
{
	int             pid;
	char            arg[MAX_TOKEN_CHARS];
	gentity_t      *player;

	// Find the player to mute.
	trap_Argv(2, arg, sizeof(arg));
	if((pid = ClientNumberFromString(ent, arg)) == -1)
	{
		return;
	}

	player = g_entities + pid;

	// CHRUKER: b060 - Added mute check so that players that are muted before granted referee status, can be unmuted
	if(player->client->sess.referee != RL_NONE && mute)
	{
		G_refPrintf(ent, "Cannot mute a referee."); // CHRUKER: b047 - Removed unneeded linebreak
		return;
	}

	if(player->client->sess.muted == mute)
	{
		G_refPrintf(ent, "\"%s^*\" %s", player->client->pers.netname, mute ? "is already muted!" : "is not muted!"); // CHRUKER: b047 - Removed unneeded linebreak
		return;
	}

	if(mute)
	{
		CPx(pid, "print \"^5You've been muted\n\"");
		player->client->sess.muted = qtrue;
		G_Printf("\"%s^*\" has been muted\n", player->client->pers.netname);
		ClientUserinfoChanged(pid);
	}
	else
	{
		CPx(pid, "print \"^5You've been unmuted\n\"");
		player->client->sess.muted = qfalse;
		G_Printf("\"%s^*\" has been unmuted\n", player->client->pers.netname);
		ClientUserinfoChanged(pid);
	}
}

//////////////////////////////
//  Client authentication
//
void Cmd_AuthRcon_f(gentity_t * ent)
{
	char            buf[MAX_TOKEN_CHARS], cmd[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("rconPassword", buf, sizeof(buf));
	trap_Argv(1, cmd, sizeof(cmd));

	if(*buf && !strcmp(buf, cmd))
	{
		ent->client->sess.referee = RL_RCON;
	}
}


//////////////////////////////
//  Console admin commands
//
void G_PlayerBan()
{
	char            cmd[MAX_TOKEN_CHARS];
	int             bannum;

	trap_Argv(1, cmd, sizeof(cmd));

	if(!*cmd)
	{
		G_Printf("usage: ban <clientname>.");
		return;
	}

	bannum = G_refClientnumForName(NULL, cmd);

	if(bannum != MAX_CLIENTS)
	{
//      if( level.clients[bannum].sess.referee != RL_RCON ) {
		const char     *value;
		char            userinfo[MAX_INFO_STRING];

		trap_GetUserinfo(bannum, userinfo, sizeof(userinfo));
		value = Info_ValueForKey(userinfo, "ip");

		AddIPBan(value);
//      } else {
//          G_Printf( "^3*** Can't ban a superuser!\n" );
//      }
	}
}

void G_MakeReferee()
{
	char            cmd[MAX_TOKEN_CHARS];
	int             cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if(!*cmd)
	{
		G_Printf("usage: MakeReferee <clientname>.");
		return;
	}

	cnum = G_refClientnumForName(NULL, cmd);

	if(cnum != MAX_CLIENTS)
	{
		if(level.clients[cnum].sess.referee == RL_NONE)
		{
			level.clients[cnum].sess.referee = RL_REFEREE;
			AP(va("cp \"%s\n^3has been made a referee\n\"", cmd));
			G_Printf("%s has been made a referee.\n", cmd);
		}
		else
		{
			G_Printf("User is already authed.\n");
		}
	}
}

void G_RemoveReferee()
{
	char            cmd[MAX_TOKEN_CHARS];
	int             cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if(!*cmd)
	{
		G_Printf("usage: RemoveReferee <clientname>.");
		return;
	}

	cnum = G_refClientnumForName(NULL, cmd);

	if(cnum != MAX_CLIENTS)
	{
		if(level.clients[cnum].sess.referee == RL_REFEREE)
		{
			level.clients[cnum].sess.referee = RL_NONE;
			G_Printf("%s is no longer a referee.\n", cmd);
		}
		else
		{
			G_Printf("User is not a referee.\n");
		}
	}
}

void G_MuteClient()
{
	char            cmd[MAX_TOKEN_CHARS];
	int             cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if(!*cmd)
	{
		G_Printf("usage: Mute <clientname>.");
		return;
	}

	cnum = G_refClientnumForName(NULL, cmd);

	if(cnum != MAX_CLIENTS)
	{
		if(level.clients[cnum].sess.referee != RL_RCON)
		{
			trap_SendServerCommand(cnum, va("cpm \"^3You have been muted\""));
			level.clients[cnum].sess.muted = qtrue;
			G_Printf("%s^* has been muted\n", cmd);
			ClientUserinfoChanged(cnum);
		}
		else
		{
			G_Printf("Cannot mute a referee.\n");
		}
	}
}

void G_UnMuteClient()
{
	char            cmd[MAX_TOKEN_CHARS];
	int             cnum;

	trap_Argv(1, cmd, sizeof(cmd));

	if(!*cmd)
	{
		G_Printf("usage: Unmute <clientname>.\n");
		return;
	}

	cnum = G_refClientnumForName(NULL, cmd);

	if(cnum != MAX_CLIENTS)
	{
		if(level.clients[cnum].sess.muted)
		{
			trap_SendServerCommand(cnum, va("cpm \"^2You have been un-muted\""));
			level.clients[cnum].sess.muted = qfalse;
			G_Printf("%s has been un-muted\n", cmd);
			ClientUserinfoChanged(cnum);
		}
		else
		{
			G_Printf("User is not muted.\n");
		}
	}
}


/////////////////
//   Utility
//
int G_refClientnumForName(gentity_t * ent, const char *name)
{
	char            cleanName[MAX_TOKEN_CHARS];
	int             i;

	if(!*name)
	{
		return (MAX_CLIENTS);
	}

	for(i = 0; i < level.numConnectedClients; i++)
	{
		Q_strncpyz(cleanName, level.clients[level.sortedClients[i]].pers.netname, sizeof(cleanName));
		Q_CleanStr(cleanName);
		if(!Q_stricmp(cleanName, name))
		{
			return (level.sortedClients[i]);
		}
	}

	G_refPrintf(ent, "Client not on server.");

	return (MAX_CLIENTS);
}

void G_refPrintf(gentity_t * ent, const char *fmt, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	if(ent == NULL)
	{
		trap_Print(va("%s\n", text));	// CHRUKER: b047 - Added linebreak to the string
	}
	else
	{
		// CHRUKER: b046 - Was using the cpm command, but this is really just for the console
		CP(va("print \"%s\n\"", text));
	}
}
