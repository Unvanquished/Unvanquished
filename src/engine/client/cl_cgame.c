/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

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

// cl_cgame.c  -- client system interaction with client game

#ifdef _MSC_VER
#include "../../libs/msinttypes/inttypes.h"
#else
#include <inttypes.h>
#endif

#include "client.h"

#include "../botlib/botlib.h"

#include "libmumblelink.h"

extern botlib_export_t *botlib_export;

extern qboolean loadCamera(int camNum, const char *name);
extern void     startCamera(int camNum, int time);
extern qboolean getCameraInfo(int camNum, int time, vec3_t * origin, vec3_t * angles, float *fov);

static void(*completer)(const char *s) = NULL;

// NERVE - SMF
void            Key_GetBindingBuf(int keynum, char *buf, int buflen);
void            Key_KeynumToStringBuf(int keynum, char *buf, int buflen);

// -NERVE - SMF

// ydnar: can we put this in a header, pls?
void            Key_GetBindingByString(const char *binding, int *key1, int *key2);


/*
====================
CL_GetGameState
====================
*/
void CL_GetGameState(gameState_t * gs)
{
	*gs = cl.gameState;
}

/*
====================
CL_GetGlconfig
====================
*/
void CL_GetGlconfig(glconfig_t * glconfig)
{
	*glconfig = cls.glconfig;
}


/*
====================
CL_GetUserCmd
====================
*/
qboolean CL_GetUserCmd(int cmdNumber, usercmd_t * ucmd)
{
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if(cmdNumber > cl.cmdNumber)
	{
		Com_Error(ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber);
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if(cmdNumber <= cl.cmdNumber - CMD_BACKUP)
	{
		return qfalse;
	}

	*ucmd = cl.cmds[cmdNumber & CMD_MASK];

	return qtrue;
}

int CL_GetCurrentCmdNumber(void)
{
	return cl.cmdNumber;
}


/*
====================
CL_GetParseEntityState
====================
*/
qboolean CL_GetParseEntityState(int parseEntityNumber, entityState_t * state)
{
	// can't return anything that hasn't been parsed yet
	if(parseEntityNumber >= cl.parseEntitiesNum)
	{
		Com_Error(ERR_DROP, "CL_GetParseEntityState: %i >= %i", parseEntityNumber, cl.parseEntitiesNum);
	}

	// can't return anything that has been overwritten in the circular buffer
	if(parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES)
	{
		return qfalse;
	}

	*state = cl.parseEntities[parseEntityNumber & (MAX_PARSE_ENTITIES - 1)];
	return qtrue;
}

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void CL_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime)
{
	*snapshotNumber = cl.snap.messageNum;
	*serverTime = cl.snap.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
qboolean CL_GetSnapshot(int snapshotNumber, snapshot_t * snapshot)
{
	clSnapshot_t   *clSnap;
	int             i, count;

	if(snapshotNumber > cl.snap.messageNum)
	{
		Com_Error(ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum");
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if(cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP)
	{
		return qfalse;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
	if(!clSnap->valid)
	{
		return qfalse;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if(cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES)
	{
		return qfalse;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	memcpy(snapshot->areamask, clSnap->areamask, sizeof(snapshot->areamask));
	snapshot->ps = clSnap->ps;
	count = clSnap->numEntities;
	if(count > MAX_ENTITIES_IN_SNAPSHOT)
	{
		Com_DPrintf("CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT);
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;
	for(i = 0; i < count; i++)
	{
		snapshot->entities[i] = cl.parseEntities[(clSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES - 1)];
	}

	// FIXME: configstring changes and server commands!!!

	return qtrue;
}

/*
==============
CL_SetUserCmdValue
==============
*/
void CL_SetUserCmdValue(int userCmdValue, int flags, float sensitivityScale, int mpIdentClient)
{
	cl.cgameUserCmdValue = userCmdValue;
	cl.cgameFlags = flags;
	cl.cgameSensitivity = sensitivityScale;
	cl.cgameMpIdentClient = mpIdentClient;	// NERVE - SMF
}

/*
==================
CL_SetClientLerpOrigin
==================
*/
void CL_SetClientLerpOrigin(float x, float y, float z)
{
	cl.cgameClientLerpOrigin[0] = x;
	cl.cgameClientLerpOrigin[1] = y;
	cl.cgameClientLerpOrigin[2] = z;
}

/*
=====================
CL_CompleteCgameCommand
=====================
*/
void CL_CompleteCgameCommand( char *args, int argNum ) {
	Field_CompleteCgame( argNum );
}

/*
=====================
CL_CgameCompletion
=====================
*/
void CL_CgameCompletion( void(*callback)(const char *s), int argNum ) {
	completer = callback;
	VM_Call( cgvm, CG_COMPLETE_COMMAND, argNum );
	completer = NULL;
}

/*
==============
CL_AddCgameCommand
==============
*/
void CL_AddCgameCommand(const char *cmdName)
{
	Cmd_AddCommand(cmdName, NULL);
	Cmd_SetCommandCompletionFunc( cmdName, CL_CompleteCgameCommand );
}

/*
==============
CL_CgameError
==============
*/
void CL_CgameError(const char *string)
{
	Com_Error(ERR_DROP, "%s", string);
}

qboolean CL_CGameCheckKeyExec(int key)
{
	if(cgvm)
	{
		return VM_Call(cgvm, CG_CHECKEXECKEY, key);
	}
	else
	{
		return qfalse;
	}
}


/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified(void)
{
	char           *old, *s;
	int             i, index;
	char           *dup;
	gameState_t     oldGs;
	int             len;

	index = atoi(Cmd_Argv(1));
	if(index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		Com_Error(ERR_DROP, "configstring > MAX_CONFIGSTRINGS");
	}
//  s = Cmd_Argv(2);
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom(2);

	old = cl.gameState.stringData + cl.gameState.stringOffsets[index];
	if(!strcmp(old, s))
	{
		return;					// unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	memset(&cl.gameState, 0, sizeof(cl.gameState));

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;

	for(i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if(i == index)
		{
			dup = s;
		}
		else
		{
			dup = oldGs.stringData + oldGs.stringOffsets[i];
		}
		if(!dup[0])
		{
			continue;			// leave with the default empty string
		}

		len = strlen(dup);

		if(len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS)
		{
			Com_Error(ERR_DROP, "MAX_GAMESTATE_CHARS exceeded");
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[i] = cl.gameState.dataCount;
		memcpy(cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1);
		cl.gameState.dataCount += len + 1;
	}

	if(index == CS_SYSTEMINFO)
	{
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}

}


/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand(int serverCommandNumber)
{
	char           *s;
	char           *cmd;
	static char     bigConfigString[BIG_INFO_STRING];
	int             argc;

	// if we have irretrievably lost a reliable command, drop the connection
	if(serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS)
	{
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if(clc.demoplaying)
		{
			return qfalse;
		}
		Com_Error(ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out");
		return qfalse;
	}

	if(serverCommandNumber > clc.serverCommandSequence)
	{
		Com_Error(ERR_DROP, "CL_GetServerCommand: requested a command not received");
		return qfalse;
	}

	s = clc.serverCommands[serverCommandNumber & (MAX_RELIABLE_COMMANDS - 1)];
	clc.lastExecutedServerCommand = serverCommandNumber;

	if(cl_showServerCommands->integer)
	{							// NERVE - SMF
		Com_DPrintf("serverCommand: %i : %s\n", serverCommandNumber, s);
	}

  rescan:
	Cmd_TokenizeString(s);
	cmd = Cmd_Argv(0);
	argc = Cmd_Argc();

	if(!strcmp(cmd, "disconnect"))
	{
		// NERVE - SMF - allow server to indicate why they were disconnected
		if(argc >= 2)
		{
			Com_Error(ERR_SERVERDISCONNECT, "Server Disconnected - %s", Cmd_Argv(1));
		}
		else
		{
			Com_Error(ERR_SERVERDISCONNECT, "Server disconnected\n");
		}
	}

	if(!strcmp(cmd, "bcs0"))
	{
		Com_sprintf(bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2));
		return qfalse;
	}

	if(!strcmp(cmd, "bcs1"))
	{
		s = Cmd_Argv(2);
		if(strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING)
		{
			Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
		}
		strcat(bigConfigString, s);
		return qfalse;
	}

	if(!strcmp(cmd, "bcs2"))
	{
		s = Cmd_Argv(2);
		if(strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING)
		{
			Com_Error(ERR_DROP, "bcs exceeded BIG_INFO_STRING");
		}
		strcat(bigConfigString, s);
		strcat(bigConfigString, "\"");
		s = bigConfigString;
		goto rescan;
	}

	if(!strcmp(cmd, "cs"))
	{
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString(s);
		return qtrue;
	}

	if(!strcmp(cmd, "map_restart"))
	{
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		memset(cl.cmds, 0, sizeof(cl.cmds));
		return qtrue;
	}

	if(!strcmp(cmd, "popup"))
	{							// direct server to client popup request, bypassing cgame
//      trap_UI_Popup(Cmd_Argv(1));
//      if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
//          VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_CLIPBOARD);
//          Menus_OpenByName(Cmd_Argv(1));
//      }
		return qfalse;
	}

#ifdef USE_CRYPTO
	if ( cl_pubkeyID->integer && !strcmp( cmd, "pubkey_request" ) ) {
		char buffer[ MAX_STRING_CHARS ] = "pubkey ";
		mpz_get_str( buffer + 7, 16, public_key.n );
		CL_AddReliableCommand( buffer );
		return qfalse;
	}

	if ( cl_pubkeyID->integer && !strcmp( cmd, "pubkey_decrypt" ) && argc > 1 ) {
		char buffer[ MAX_STRING_CHARS ] = "pubkey_identify ";
		unsigned int msg_len = MAX_STRING_CHARS - 16;
		mpz_t message;
		mpz_init_set_str( message, Cmd_Argv( 1 ), 16 );
		if ( rsa_decrypt( &private_key, &msg_len, (unsigned char *) buffer + 16, message ) )
		{
			nettle_mpz_set_str_256_u( message, msg_len, (unsigned char *) buffer + 16 );
			mpz_get_str( buffer + 16, 16, message );
			CL_AddReliableCommand( buffer );
		}
		mpz_clear( message );
		return qfalse;
	}
#endif

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return qtrue;
}

// DHM - Nerve :: Copied from server to here
/*
====================
CL_SetExpectedHunkUsage

  Sets com_expectedhunkusage, so the client knows how to draw the percentage bar
====================
*/
void CL_SetExpectedHunkUsage(const char *mapname)
{
	int             handle;
	char           *memlistfile = "hunkusage.dat";
	char           *buf;
	char           *buftrav;
	char           *token;
	int             len;

	len = FS_FOpenFileByMode(memlistfile, &handle, FS_READ);
	if(len >= 0)
	{							// the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value

		buf = (char *)Z_Malloc(len + 1);
		memset(buf, 0, len + 1);

		FS_Read((void *)buf, len, handle);
		FS_FCloseFile(handle);

		// now parse the file, filtering out the current map
		buftrav = buf;
		while((token = COM_Parse(&buftrav)) != NULL && token[0])
		{
			if(!Q_stricmp(token, (char *)mapname))
			{
				// found a match
				token = COM_Parse(&buftrav);	// read the size
				if(token && *token)
				{
					// this is the usage
					com_expectedhunkusage = atoi(token);
					Z_Free(buf);
					return;
				}
			}
		}

		Z_Free(buf);
	}
	// just set it to a negative number,so the cgame knows not to draw the percent bar
	com_expectedhunkusage = -1;
}

// dhm - nerve

/*
====================
CL_SendBinaryMessage
====================
*/
static void CL_SendBinaryMessage(const char *buf, int buflen)
{
	if(buflen < 0 || buflen > MAX_BINARY_MESSAGE)
	{
		Com_Error(ERR_DROP, "CL_SendBinaryMessage: bad length %i", buflen);
		clc.binaryMessageLength = 0;
		return;
	}

	clc.binaryMessageLength = buflen;
	memcpy(clc.binaryMessage, buf, buflen);
}

/*
====================
CL_BinaryMessageStatus
====================
*/
static int CL_BinaryMessageStatus(void)
{
	if(clc.binaryMessageLength == 0)
	{
		return MESSAGE_EMPTY;
	}

	if(clc.binaryMessageOverflowed)
	{
		return MESSAGE_WAITING_OVERFLOW;
	}

	return MESSAGE_WAITING;
}

/*
====================
CL_CGameBinaryMessageReceived
====================
*/
void CL_CGameBinaryMessageReceived(const char *buf, int buflen, int serverTime)
{
	VM_Call(cgvm, CG_MESSAGERECEIVED, buf, buflen, serverTime);
}

/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void CL_CM_LoadMap(const char *mapname)
{
	int             checksum;

	// DHM - Nerve :: If we are not running the server, then set expected usage here
	if(!com_sv_running->integer)
	{
		CL_SetExpectedHunkUsage(mapname);
	}
	else
	{
		// TTimo
		// catch here when a local server is started to avoid outdated com_errorDiagnoseIP
		Cvar_Set("com_errorDiagnoseIP", "");
	}

	CM_LoadMap(mapname, qtrue, &checksum);
}

/*
====================
CL_ShutdonwCGame

====================
*/
void CL_ShutdownCGame(void)
{
	cls.keyCatchers &= ~KEYCATCH_CGAME;
	cls.cgameStarted = qfalse;
	if(!cgvm)
	{
		return;
	}
	VM_Call(cgvm, CG_SHUTDOWN);
	VM_Free(cgvm);
	cgvm = NULL;
}

static int FloatAsInt(float f)
{
	floatint_t      fi;

fi.f = f;
	return fi.i;
}

//static int numtraces = 0;

/*
====================
CL_CgameSystemCalls

The cgame module is making a system call
====================
*/
intptr_t CL_CgameSystemCalls(intptr_t * args)
{
	switch (args[0])
	{
		case CG_PRINT:
			Com_Printf("%s", (char *)VMA(1));
			return 0;
		case CG_ERROR:
			Com_Error(ERR_DROP, "%s", (char *)VMA(1));
			return 0;
		case CG_MILLISECONDS:
			return Sys_Milliseconds();
		case CG_CVAR_REGISTER:
			Cvar_Register(VMA(1), VMA(2), VMA(3), args[4]);
			return 0;
		case CG_CVAR_UPDATE:
			Cvar_Update(VMA(1));
			return 0;
		case CG_CVAR_SET:
			Cvar_Set(VMA(1), VMA(2));
			return 0;
		case CG_CVAR_VARIABLESTRINGBUFFER:
			Cvar_VariableStringBuffer(VMA(1), VMA(2), args[3]);
			return 0;
		case CG_CVAR_LATCHEDVARIABLESTRINGBUFFER:
			Cvar_LatchedVariableStringBuffer(VMA(1), VMA(2), args[3]);
			return 0;
		case CG_ARGC:
			return Cmd_Argc();
		case CG_ARGV:
			Cmd_ArgvBuffer(args[1], VMA(2), args[3]);
			return 0;
		case CG_ARGS:
			Cmd_ArgsBuffer(VMA(1), args[2]);
			return 0;
		case CG_LITERAL_ARGS:
			// Dushan : FIX ME
			Cmd_LiteralArgsBuffer( VMA(1), args[2] );
// 			Cmd_ArgsBuffer(VMA(1), args[2]);
			return 0;
		case CG_GETDEMOSTATE:
			//return CL_DemoState( );
			// Dushan : FIX ME
			return 0;
		case CG_GETDEMOPOS:
			//return CL_DemoPos( );
			return 0;
		case CG_FS_FOPENFILE:
			return FS_FOpenFileByMode(VMA(1), VMA(2), args[3]);
		case CG_FS_READ:
			FS_Read2(VMA(1), args[2], args[3]);
			return 0;
		case CG_FS_WRITE:
			return FS_Write(VMA(1), args[2], args[3]);
		case CG_FS_FCLOSEFILE:
			FS_FCloseFile(args[1]);
			return 0;
		case CG_FS_GETFILELIST:
			return FS_GetFileList(VMA(1), VMA(2), VMA(3), args[4]);
		case CG_FS_DELETEFILE:
			return FS_Delete(VMA(1));
		case CG_SENDCONSOLECOMMAND:
			Cbuf_AddText(VMA(1));
			return 0;
		case CG_ADDCOMMAND:
			CL_AddCgameCommand(VMA(1));
			return 0;
		case CG_REMOVECOMMAND:
			Cmd_RemoveCommand(VMA(1));
			return 0;
		case CG_COMPLETE_CALLBACK:
			if( completer )
				completer( VMA(1) );
			return 0;
		case CG_SENDCLIENTCOMMAND:
			CL_AddReliableCommand(VMA(1));
			return 0;
		case CG_UPDATESCREEN:
			SCR_UpdateScreen();
			return 0;
		case CG_CM_LOADMAP:
			CL_CM_LoadMap(VMA(1));
			return 0;
		case CG_CM_NUMINLINEMODELS:
			return CM_NumInlineModels();
		case CG_CM_INLINEMODEL:
			return CM_InlineModel(args[1]);
		case CG_CM_TEMPBOXMODEL:
			return CM_TempBoxModel(VMA(1), VMA(2), qfalse);
		case CG_CM_TEMPCAPSULEMODEL:
			return CM_TempBoxModel(VMA(1), VMA(2), qtrue);
		case CG_CM_POINTCONTENTS:
			return CM_PointContents(VMA(1), args[2]);
		case CG_CM_TRANSFORMEDPOINTCONTENTS:
			return CM_TransformedPointContents(VMA(1), args[2], VMA(3), VMA(4));
		case CG_CM_BOXTRACE:
//      numtraces++;
			//CM_BoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /*int capsule */ qfalse);
			CM_BoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], TT_AABB);
			return 0;
		case CG_CM_TRANSFORMEDBOXTRACE:
//      numtraces++;
			//CM_TransformedBoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], VMA(8), VMA(9), /*int capsule */ qfalse);
			CM_TransformedBoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], VMA(8), VMA(9), TT_AABB);
			return 0;
		case CG_CM_CAPSULETRACE:
//      numtraces++;
			//CM_BoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /*int capsule */ qtrue);
			CM_BoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], TT_CAPSULE);
			return 0;
		case CG_CM_TRANSFORMEDCAPSULETRACE:
//      numtraces++;
			//CM_TransformedBoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], VMA(8), VMA(9),	/*int capsule */ qtrue);
			CM_TransformedBoxTrace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], VMA(8), VMA(9), TT_CAPSULE);
			return 0;
		case CG_CM_BISPHERETRACE:
			CM_BiSphereTrace( VMA(1), VMA(2), VMA(3), VMF(4), VMF(5), args[6], args[7] );
		return 0;
		case CG_CM_TRANSFORMEDBISPHERETRACE:
			CM_TransformedBiSphereTrace( VMA(1), VMA(2), VMA(3), VMF(4), VMF(5), args[6], args[7], VMA(8) );
			return 0;
		case CG_CM_MARKFRAGMENTS:
			return re.MarkFragments(args[1], VMA(2), VMA(3), args[4], VMA(5), args[6], VMA(7));

		case CG_R_PROJECTDECAL:
			re.ProjectDecal(args[1], args[2], VMA(3), VMA(4), VMA(5), args[6], args[7]);
			return 0;
		case CG_R_CLEARDECALS:
			re.ClearDecals();
			return 0;

		case CG_S_STARTSOUND:
			S_StartSound( VMA(1), args[2], args[3], args[4] );
			//S_StartSound(VMA(1), args[2], args[3], args[4], args[5]);
			return 0;
//----(SA)  added
		case CG_S_STARTSOUNDEX:
			// Dushan - FIX ME
			//S_StartSoundEx(VMA(1), args[2], args[3], args[4], args[5], args[6]);
			return 0;
//----(SA)  end
		case CG_S_STARTLOCALSOUND:
			//S_StartLocalSound(args[1], args[2], args[3]);
			S_StartLocalSound( args[1], args[2] );
			return 0;
		case CG_S_CLEARLOOPINGSOUNDS:
			//S_ClearLoopingSounds();
			S_ClearLoopingSounds(args[1]);
			return 0;
		case CG_S_CLEARSOUNDS:
			// Dushan - FIX ME
			/*if(args[1] == 0)
			{
				S_ClearSounds(qtrue, qfalse);
			}
			else if(args[1] == 1)
			{
				S_ClearSounds(qtrue, qtrue);
			}*/
			return 0;
		case CG_S_ADDLOOPINGSOUND:
			// FIXME MrE: handling of looping sounds changed
			//S_AddLoopingSound(VMA(1), VMA(2), args[3], args[4], args[5], args[6]);
			S_AddLoopingSound( args[1], VMA(2), VMA(3), args[4] );
			return 0;
		case CG_S_ADDREALLOOPINGSOUND:
			//S_AddRealLoopingSound(VMA(1), VMA(2), args[3], args[4], args[5], args[6]);
			S_AddRealLoopingSound( args[1], VMA(2), VMA(3), args[4] );
			return 0;
		case CG_S_STOPLOOPINGSOUND:
			S_StopLoopingSound( args[1] );
			return 0;
		case CG_S_STOPSTREAMINGSOUND:
			// Dushan - FIX ME
			//S_StopEntStreamingSound(args[1]);
			return 0;
		case CG_S_UPDATEENTITYPOSITION:
			S_UpdateEntityPosition(args[1], VMA(2));
			return 0;
// Ridah, talking animations
		case CG_S_GETVOICEAMPLITUDE:
			// Dushan - FIX ME
			//return S_GetVoiceAmplitude(args[1]);
			return 0;
// done.
		case CG_S_GETSOUNDLENGTH:
			// Dushan - FIX ME
			//return S_GetSoundLength(args[1]);
			return 0;

			// ydnar: for looped sound starts
		case CG_S_GETCURRENTSOUNDTIME:
			// Dushan - FIX ME
			//return S_GetCurrentSoundTime();
			return 0;

		case CG_S_RESPATIALIZE:
			S_Respatialize(args[1], VMA(2), VMA(3), args[4]);
			return 0;
		case CG_S_REGISTERSOUND:
#ifdef DOOMSOUND				///// (SA) DOOMSOUND
			return S_RegisterSound(VMA(1));
#else
			return S_RegisterSound(VMA(1), args[2]);
#endif							///// (SA) DOOMSOUND
		case CG_S_STARTBACKGROUNDTRACK:
			//S_StartBackgroundTrack(VMA(1), VMA(2), args[3]);	//----(SA)  added fadeup time
			S_StartBackgroundTrack(VMA(1), VMA(2));
			return 0;
		case CG_S_FADESTREAMINGSOUND:
			// Dushan - FIX ME
			//S_FadeStreamingSound(VMF(1), args[2], args[3]);	//----(SA)  added music/all-streaming options
			return 0;
		case CG_S_STARTSTREAMINGSOUND:
			// Dushan - FIX ME
			//return S_StartStreamingSound(VMA(1), VMA(2), args[3], args[4], args[5]);
			return 0;
		case CG_R_LOADWORLDMAP:
			re.LoadWorld(VMA(1));
			return 0;
		case CG_R_REGISTERMODEL:
#ifdef IPHONE
			GLimp_AcquireGL();
			return re.RegisterModel( VMA(1) );
			GLimp_ReleaseGL();
#else
			return re.RegisterModel(VMA(1));
#endif // IPHONE
		case CG_R_REGISTERSKIN:
			return re.RegisterSkin(VMA(1));

			//----(SA)  added
		case CG_R_GETSKINMODEL:
			return re.GetSkinModel(args[1], VMA(2), VMA(3));
		case CG_R_GETMODELSHADER:
			return re.GetShaderFromModel(args[1], args[2], args[3]);
			//----(SA)  end

		case CG_R_REGISTERSHADER:
#ifdef IPHONE_NOTYET
			GLimp_AcquireGL();
			return re.RegisterShader( VMA(1) );
			GLimp_ReleaseGL();
#else
			return re.RegisterShader(VMA(1));
#endif // IPHONE
		case CG_R_REGISTERFONT:
			re.RegisterFont(VMA(1), args[2], VMA(3));
			return 0;
		case CG_R_REGISTERSHADERNOMIP:
#ifdef IPHONE_NOTYET
			GLimp_AcquireGL();
			return re.RegisterShaderNoMip( VMA(1) );
			GLimp_ReleaseGL();
#else
			return re.RegisterShaderNoMip(VMA(1));
#endif // IPHONE
#if defined(USE_REFLIGHT)
		case CG_R_REGISTERSHADERLIGHTATTENUATION:
			return re.RegisterShaderLightAttenuation(VMA(1));
#endif
		case CG_R_CLEARSCENE:
			re.ClearScene();
			return 0;
		case CG_R_ADDREFENTITYTOSCENE:
			re.AddRefEntityToScene(VMA(1));
			return 0;
#if defined(USE_REFLIGHT)
		case CG_R_ADDREFLIGHTSTOSCENE:
			re.AddRefLightToScene(VMA(1));
			return 0;
#endif
		case CG_R_ADDPOLYTOSCENE:
			re.AddPolyToScene(args[1], args[2], VMA(3));
			return 0;
			// Ridah
		case CG_R_ADDPOLYSTOSCENE:
			re.AddPolysToScene(args[1], args[2], VMA(3), args[4]);
			return 0;
		case CG_R_ADDPOLYBUFFERTOSCENE:
			re.AddPolyBufferToScene(VMA(1));
			break;
			// done.
//  case CG_R_LIGHTFORPOINT:
//      return re.LightForPoint( VMA(1), VMA(2), VMA(3), VMA(4) );
		case CG_R_ADDLIGHTTOSCENE:
			// ydnar: new dlight code
			//% re.AddLightToScene( VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6] );
			re.AddLightToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), args[7], args[8]);
			return 0;
//  case CG_R_ADDADDITIVELIGHTTOSCENE:
//      re.AddAdditiveLightToScene( VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
//      return 0;
		case CG_R_ADDADDITIVELIGHTTOSCENE:
			re.AddAdditiveLightToScene( VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
			return 0;
		case CG_FS_SEEK:
			return FS_Seek( args[1], args[2], args[3] );
		case CG_R_ADDCORONATOSCENE:
			re.AddCoronaToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
			return 0;
		case CG_R_SETFOG:
			re.SetFog(args[1], args[2], args[3], VMF(4), VMF(5), VMF(6), VMF(7));
			return 0;
		case CG_R_SETGLOBALFOG:
			re.SetGlobalFog(args[1], args[2], VMF(3), VMF(4), VMF(5), VMF(6));
			return 0;
		case CG_R_RENDERSCENE:
			re.RenderScene(VMA(1));
			return 0;
		case CG_R_SAVEVIEWPARMS:
			re.SaveViewParms();
			return 0;
		case CG_R_RESTOREVIEWPARMS:
			re.RestoreViewParms();
			return 0;
		case CG_R_SETCOLOR:
			re.SetColor(VMA(1));
			return 0;
		// Tremulous
		case CG_R_SETCLIPREGION:
			re.SetClipRegion( VMA(1) );
			return 0;
		case CG_R_DRAWSTRETCHPIC:
			re.DrawStretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
			return 0;
		case CG_R_DRAWROTATEDPIC:
			re.DrawRotatedPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], VMF(10));
			return 0;
		case CG_R_DRAWSTRETCHPIC_GRADIENT:
			re.DrawStretchPicGradient(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], VMA(10), args[11]);
			return 0;
		case CG_R_DRAW2DPOLYS:
			re.Add2dPolys(VMA(1), args[2], args[3]);
			return 0;
		case CG_R_MODELBOUNDS:
			re.ModelBounds(args[1], VMA(2), VMA(3));
			return 0;
		case CG_R_LERPTAG:
			return re.LerpTag(VMA(1), VMA(2), VMA(3), args[4]);

		case CG_GETGLCONFIG:
			CL_GetGlconfig(VMA(1));
			return 0;
		case CG_GETGAMESTATE:
			CL_GetGameState(VMA(1));
			return 0;
		case CG_GETCURRENTSNAPSHOTNUMBER:
			CL_GetCurrentSnapshotNumber(VMA(1), VMA(2));
			return 0;
		case CG_GETSNAPSHOT:
			return CL_GetSnapshot(args[1], VMA(2));
		case CG_GETSERVERCOMMAND:
			return CL_GetServerCommand(args[1]);
		case CG_GETCURRENTCMDNUMBER:
			return CL_GetCurrentCmdNumber();
		case CG_GETUSERCMD:
			return CL_GetUserCmd(args[1], VMA(2));
		case CG_SETUSERCMDVALUE:
			CL_SetUserCmdValue(args[1], args[2], VMF(3), args[4]);
			return 0;
		case CG_SETCLIENTLERPORIGIN:
			CL_SetClientLerpOrigin(VMF(1), VMF(2), VMF(3));
			return 0;
		case CG_MEMORY_REMAINING:
			return Hunk_MemoryRemaining();
		case CG_KEY_ISDOWN:
			return Key_IsDown(args[1]);
		case CG_KEY_GETCATCHER:
			return Key_GetCatcher();
		case CG_KEY_SETCATCHER:
			Key_SetCatcher(args[1]);
			return 0;
		case CG_KEY_GETKEY:
			return Key_GetKey(VMA(1));

		case CG_KEY_GETOVERSTRIKEMODE:
			return Key_GetOverstrikeMode();
		case CG_KEY_SETOVERSTRIKEMODE:
			Key_SetOverstrikeMode(args[1]);
			return 0;




		case CG_MEMSET:
			// we cannot return server-address to QVM !
			memset(VMA(1), args[2], args[3]);
			return args[1];
		case CG_MEMCPY:
			// we cannot return server-address to QVM !
			memcpy(VMA(1), VMA(2), args[3]);
			return args[1];
		case CG_STRNCPY:
			// we cannot return server-address to QVM !
			strncpy(VMA(1), VMA(2), args[3]);
			return args[1];
		case CG_SIN:
			return FloatAsInt(sin(VMF(1)));
		case CG_COS:
			return FloatAsInt(cos(VMF(1)));
		case CG_ATAN2:
			return FloatAsInt(atan2(VMF(1), VMF(2)));
		case CG_SQRT:
			return FloatAsInt(sqrt(VMF(1)));
		case CG_FLOOR:
			return FloatAsInt(floor(VMF(1)));
		case CG_CEIL:
			return FloatAsInt(ceil(VMF(1)));
		case CG_ACOS:
			return FloatAsInt(Q_acos(VMF(1)));

		case CG_PC_ADD_GLOBAL_DEFINE:
			return botlib_export->PC_AddGlobalDefine(VMA(1));
		case CG_PC_LOAD_SOURCE:
			return botlib_export->PC_LoadSourceHandle(VMA(1));
		case CG_PC_FREE_SOURCE:
			return botlib_export->PC_FreeSourceHandle(args[1]);
		case CG_PC_READ_TOKEN:
			return botlib_export->PC_ReadTokenHandle(args[1], VMA(2));
		case CG_PC_SOURCE_FILE_AND_LINE:
			return botlib_export->PC_SourceFileAndLine(args[1], VMA(2), VMA(3));
		case CG_PC_UNREAD_TOKEN:
			botlib_export->PC_UnreadLastTokenHandle(args[1]);
			return 0;

		case CG_S_STOPBACKGROUNDTRACK:
			S_StopBackgroundTrack();
			return 0;

		case CG_REAL_TIME:
			return Com_RealTime(VMA(1));
		case CG_SNAPVECTOR:
			Q_SnapVector(VMA(1));
			return 0;

		case CG_CIN_PLAYCINEMATIC:
			return CIN_PlayCinematic(VMA(1), args[2], args[3], args[4], args[5], args[6]);

		case CG_CIN_STOPCINEMATIC:
			return CIN_StopCinematic(args[1]);

		case CG_CIN_RUNCINEMATIC:
			return CIN_RunCinematic(args[1]);

		case CG_CIN_DRAWCINEMATIC:
			CIN_DrawCinematic(args[1]);
			return 0;

		case CG_CIN_SETEXTENTS:
			CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
			return 0;

		case CG_R_REMAP_SHADER:
			re.RemapShader(VMA(1), VMA(2), VMA(3));
			return 0;

		case CG_TESTPRINTINT:
			Com_Printf("%s%"PRIiPTR"\n", (char *)VMA(1), args[2]);
			return 0;
		case CG_TESTPRINTFLOAT:
			Com_Printf("%s%f\n", (char *)VMA(1), VMF(2));
			return 0;

		case CG_LOADCAMERA:
			//return loadCamera(args[1], VMA(2));
			return 0;

		case CG_STARTCAMERA:
			//if(args[1] == 0)
			//{					// CAM_PRIMARY
			//	cl.cameraMode = qtrue;
			//}
			//startCamera(args[1], args[2]);
			return 0;

		case CG_STOPCAMERA:
				//if(args[1] == 0)
				//{					// CAM_PRIMARY
				//	cl.cameraMode = qfalse;
				//}
			return 0;

		case CG_GETCAMERAINFO:
			//return getCameraInfo(args[1], args[2], VMA(3), VMA(4), VMA(5));
			return 0;

		case CG_GET_ENTITY_TOKEN:
			return re.GetEntityToken(VMA(1), args[2]);

		case CG_INGAME_POPUP:
			if(cls.state == CA_ACTIVE && !clc.demoplaying)
			{
				if(uivm)
				{				// Gordon: can be called as the system is shutting down
					VM_Call(uivm, UI_SET_ACTIVE_MENU, args[1]);
				}
			}
			return 0;

		case CG_INGAME_CLOSEPOPUP:
			return 0;

		case CG_KEY_GETBINDINGBUF:
			Key_GetBindingBuf(args[1], VMA(2), args[3]);
			return 0;

		case CG_KEY_SETBINDING:
			Key_SetBinding(args[1], VMA(2));
			return 0;

		// Dushan - Tremulous
		case CG_PARSE_ADD_GLOBAL_DEFINE:
			return Parse_AddGlobalDefine( VMA(1) );
		case CG_PARSE_LOAD_SOURCE:
			return Parse_LoadSourceHandle( VMA(1) );
		case CG_PARSE_FREE_SOURCE:
			return Parse_FreeSourceHandle( args[1] );
		case CG_PARSE_READ_TOKEN:
			return Parse_ReadTokenHandle( args[1], VMA(2) );
		case CG_PARSE_SOURCE_FILE_AND_LINE:
			return Parse_SourceFileAndLine( args[1], VMA(2), VMA(3) );

		case CG_KEY_KEYNUMTOSTRINGBUF:
			Key_KeynumToStringBuf(args[1], VMA(2), args[3]);
			return 0;

		case CG_KEY_BINDINGTOKEYS:
			Key_GetBindingByString(VMA(1), VMA(2), VMA(3));
			return 0;

		case CG_TRANSLATE_STRING:
			CL_TranslateString(VMA(1), VMA(2));
			return 0;

		case CG_S_FADEALLSOUNDS:
			// Dushan - FIX ME
			//S_FadeAllSounds(VMF(1), args[2], args[3]);
			return 0;

		case CG_R_INPVS:
			return re.inPVS(VMA(1), VMA(2));

		case CG_GETHUNKDATA:
			Com_GetHunkInfo(VMA(1), VMA(2));
			return 0;

		case CG_PUMPEVENTLOOP:
//      Com_EventLoop();
//      CL_WritePacket();
			return 0;

			//zinx - binary channel
		case CG_SENDMESSAGE:
			CL_SendBinaryMessage(VMA(1), args[2]);
			return 0;
		case CG_MESSAGESTATUS:
			return CL_BinaryMessageStatus();
			//bani - dynamic shaders
		case CG_R_LOADDYNAMICSHADER:
			return re.LoadDynamicShader(VMA(1), VMA(2));
			// fretn - render to texture
		case CG_R_RENDERTOTEXTURE:
			re.RenderToTexture(args[1], args[2], args[3], args[4], args[5]);
			return 0;
			//bani
		case CG_R_GETTEXTUREID:
			return re.GetTextureId(VMA(1));
			//bani - flush gl rendering buffers
		case CG_R_FINISH:
			re.Finish();
			return 0;
		case CG_GETDEMONAME:
			// Dushan: FIX ME
			//CL_DemoName( VMA(1), args[2] );
			return 0;
		case CG_R_LIGHTFORPOINT:
			return re.LightForPoint( VMA(1), VMA(2), VMA(3), VMA(4) );
		case CG_S_SOUNDDURATION:
			return S_SoundDuration( args[1] );
#if defined(USE_REFENTITY_ANIMATIONSYSTEM)
		case CG_R_REGISTERANIMATION:
			return re.RegisterAnimation( VMA(1) );
		case CG_R_CHECKSKELETON:
			return re.CheckSkeleton(VMA(1), args[2], args[3]);
		case CG_R_BUILDSKELETON:
			return re.BuildSkeleton(VMA(1), args[2], args[3], args[4], VMF(5), args[6]);
		case CG_R_BLENDSKELETON:
			return re.BlendSkeleton(VMA(1), VMA(2), VMF(3));
		case CG_R_BONEINDEX:
			return re.BoneIndex(args[1], VMA(2));
		case CG_R_ANIMNUMFRAMES:
			return re.AnimNumFrames(args[1]);
		case CG_R_ANIMFRAMERATE:
			return re.AnimFrameRate(args[1]);
#endif
		default:
			Com_Error( ERR_DROP, "Bad cgame system trap: %ld", (long int) args[0] );
	}
	return 0;
}

/*
====================
CL_UpdateLevelHunkUsage

  This updates the "hunkusage.dat" file with the current map and it's hunk usage count

  This is used for level loading, so we can show a percentage bar dependant on the amount
  of hunk memory allocated so far

  This will be slightly inaccurate if some settings like sound quality are changed, but these
  things should only account for a small variation (hopefully)
====================
*/
void CL_UpdateLevelHunkUsage(void)
{
	int             handle;
	char           *memlistfile = "hunkusage.dat";
	char           *buf, *outbuf;
	char           *buftrav, *outbuftrav;
	char           *token;
	char            outstr[256];
	int             len, memusage;

	memusage = Cvar_VariableIntegerValue("com_hunkused") + Cvar_VariableIntegerValue("hunk_soundadjust");

	len = FS_FOpenFileByMode(memlistfile, &handle, FS_READ);
	if(len >= 0)
	{							// the file exists, so read it in, strip out the current entry for this map, and save it out, so we can append the new value

		buf = (char *)Z_Malloc(len + 1);
		memset(buf, 0, len + 1);
		outbuf = (char *)Z_Malloc(len + 1);
		memset(outbuf, 0, len + 1);

		FS_Read((void *)buf, len, handle);
		FS_FCloseFile(handle);

		// now parse the file, filtering out the current map
		buftrav = buf;
		outbuftrav = outbuf;
		outbuftrav[0] = '\0';
		while((token = COM_Parse(&buftrav)) != NULL && token[0])
		{
			if(!Q_stricmp(token, cl.mapname))
			{
				// found a match
				token = COM_Parse(&buftrav);	// read the size
				if(token && token[0])
				{
					if(atoi(token) == memusage)
					{			// if it is the same, abort this process
						Z_Free(buf);
						Z_Free(outbuf);
						return;
					}
				}
			}
			else
			{					// send it to the outbuf
				Q_strcat(outbuftrav, len + 1, token);
				Q_strcat(outbuftrav, len + 1, " ");
				token = COM_Parse(&buftrav);	// read the size
				if(token && token[0])
				{
					Q_strcat(outbuftrav, len + 1, token);
					Q_strcat(outbuftrav, len + 1, "\n");
				}
				else
				{
					Com_Error(ERR_DROP, "hunkusage.dat file is corrupt\n");
				}
			}
		}

		handle = FS_FOpenFileWrite(memlistfile);
		if(handle < 0)
		{
			Com_Error(ERR_DROP, "cannot create %s\n", memlistfile);
		}
		// input file is parsed, now output to the new file
		len = strlen(outbuf);
		if(FS_Write((void *)outbuf, len, handle) != len)
		{
			Com_Error(ERR_DROP, "cannot write to %s\n", memlistfile);
		}
		FS_FCloseFile(handle);

		Z_Free(buf);
		Z_Free(outbuf);
	}
	// now append the current map to the current file
	FS_FOpenFileByMode(memlistfile, &handle, FS_APPEND);
	if(handle < 0)
	{
		Com_Error(ERR_DROP, "cannot write to hunkusage.dat, check disk full\n");
	}
	Com_sprintf(outstr, sizeof(outstr), "%s %i\n", cl.mapname, memusage);
	FS_Write(outstr, strlen(outstr), handle);
	FS_FCloseFile(handle);

	// now just open it and close it, so it gets copied to the pak dir
	len = FS_FOpenFileByMode(memlistfile, &handle, FS_READ);
	if(len >= 0)
	{
		FS_FCloseFile(handle);
	}
}

/*
====================
CL_InitCGame

Should only by called by CL_StartHunkUsers
====================
*/
void CL_InitCGame(void) {
	const char     *info;
	const char     *mapname;
	int             t1, t2;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
	mapname = Info_ValueForKey(info, "mapname");
	Com_sprintf(cl.mapname, sizeof(cl.mapname), "maps/%s.bsp", mapname);

	// Dushan - load the dll or bytecode
	cgvm = VM_Create( "cgame", CL_CgameSystemCalls, Cvar_VariableValue( "vm_cgame" ) );
	if ( !cgvm ) {
		Com_Error( ERR_DROP, "VM_Create on cgame failed" );
	}
	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	//bani - added clc.demoplaying, since some mods need this at init time, and drawactiveframe is too late for them
	VM_Call(cgvm, CG_INIT, clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum, clc.demoplaying);

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_Printf("CL_InitCGame: %5.2f seconds\n", (t2 - t1) / 1000.0);

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// make sure everything is paged in
	if(!Sys_LowPhysicalMemory())
	{
		Com_TouchMemory();
	}

	// clear anything that got printed
	Con_ClearNotify();

	// Ridah, update the memory usage file
	CL_UpdateLevelHunkUsage();

//  if( cl_autorecord->integer ) {
//      Cvar_Set( "g_synchronousClients", "1" );
//  }
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameCommand(void)
{
	if(!cgvm)
	{
		return qfalse;
	}

	return VM_Call(cgvm, CG_CONSOLE_COMMAND);
}

/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameConsoleText(void)
{
	if(!cgvm)
	{
		return qfalse;
	}

	return VM_Call(cgvm, CG_CONSOLE_TEXT);
}


/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering(stereoFrame_t stereo)
{
/*	static int x = 0;
	if(!((++x) % 20)) {
		Com_Printf( "numtraces: %i\n", numtraces / 20 );
		numtraces = 0;
	} else {
	}*/

	VM_Call(cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, stereo, clc.demoplaying);
	VM_Debug(0);
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define RESET_TIME  500

void CL_AdjustTimeDelta(void)
{
//	int             resetTime;
	int             newDelta;
	int             deltaDelta;

	cl.newSnapshots = qfalse;

	// the delta never drifts when replaying a demo
	if(clc.demoplaying)
	{
		return;
	}

	// if the current time is WAY off, just correct to the current value
/*
	if(com_sv_running->integer)
	{
		resetTime = 100;
	}
	else
	{
		resetTime = RESET_TIME;
	}
*/

	newDelta = cl.snap.serverTime - cls.realtime;
	deltaDelta = abs(newDelta - cl.serverTimeDelta);

	if(deltaDelta > RESET_TIME)
	{
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.snap.serverTime;	// FIXME: is this a problem for cgame?
		cl.serverTime = cl.snap.serverTime;
		if(cl_showTimeDelta->integer)
		{
			Com_Printf("<RESET> ");
		}
	}
	else if(deltaDelta > 100)
	{
		// fast adjust, cut the difference in half
		if(cl_showTimeDelta->integer)
		{
			Com_Printf("<FAST> ");
		}
		cl.serverTimeDelta = (cl.serverTimeDelta + newDelta) >> 1;
	}
	else
	{
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if(com_timescale->value == 0 || com_timescale->value == 1)
		{
			if(cl.extrapolatedSnapshot)
			{
				cl.extrapolatedSnapshot = qfalse;
				cl.serverTimeDelta -= 2;
			}
			else
			{
				// otherwise, move our sense of time forward to minimize total latency
				cl.serverTimeDelta++;
			}
		}
	}

	if(cl_showTimeDelta->integer)
	{
		Com_Printf("%i ", cl.serverTimeDelta);
	}
}


/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot(void)
{
	// ignore snapshots that don't have entities
	if(cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE)
	{
		return;
	}
	cls.state = CA_ACTIVE;

#ifdef IPHONE
	// Force the device into right landscape mode:
	GLimp_SetMode(90);
#endif // IPHONE

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
	cl.oldServerTime = cl.snap.serverTime;

	clc.timeDemoBaseTime = cl.snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if(cl_activeAction->string[0]) {
		Cbuf_AddText(cl_activeAction->string);
		Cbuf_AddText("\n");
		Cvar_Set("activeAction", "");
	}

#ifdef USE_MUMBLE
	if ((cl_useMumble->integer) && !mumble_islinked()) {
		int ret = mumble_link(CLIENT_WINDOW_TITLE);
		Com_Printf("Mumble: Linking to Mumble application %s\n", ret==0?"ok":"failed");
	}
#endif

#ifdef USE_VOIP
	if (!clc.speexInitialized) {
		int i;
		speex_bits_init(&clc.speexEncoderBits);
		speex_bits_reset(&clc.speexEncoderBits);

		clc.speexEncoder = speex_encoder_init(&speex_nb_mode);

		speex_encoder_ctl(clc.speexEncoder, SPEEX_GET_FRAME_SIZE,
		                  &clc.speexFrameSize);
		speex_encoder_ctl(clc.speexEncoder, SPEEX_GET_SAMPLING_RATE,
		                  &clc.speexSampleRate);

		clc.speexPreprocessor = speex_preprocess_state_init(clc.speexFrameSize,
		                                                  clc.speexSampleRate);

		i = 1;
		speex_preprocess_ctl(clc.speexPreprocessor,
		                     SPEEX_PREPROCESS_SET_DENOISE, &i);

		i = 1;
		speex_preprocess_ctl(clc.speexPreprocessor,
		                     SPEEX_PREPROCESS_SET_AGC, &i);

		for (i = 0; i < MAX_CLIENTS; i++) {
			speex_bits_init(&clc.speexDecoderBits[i]);
			speex_bits_reset(&clc.speexDecoderBits[i]);
			clc.speexDecoder[i] = speex_decoder_init(&speex_nb_mode);
			clc.voipIgnore[i] = qfalse;
			clc.voipGain[i] = 1.0f;
		}
		clc.speexInitialized = qtrue;
		clc.voipMuteAll = qfalse;
		Cmd_AddCommand ("voip", CL_Voip_f);
		Cvar_Set("cl_voipSendTarget", "spatial");
		Com_Memset(clc.voipTargets, ~0, sizeof(clc.voipTargets));
	}
#endif
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime(void)
{
	// getting a valid frame message ends the connection process
	if(cls.state != CA_ACTIVE)
	{
		if(cls.state != CA_PRIMED)
		{
			return;
		}
		if(clc.demoplaying)
		{
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if(!clc.firstDemoFrameSkipped)
			{
				clc.firstDemoFrameSkipped = qtrue;
				return;
			}
			CL_ReadDemoMessage();
		}
		if(cl.newSnapshots)
		{
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}
		if(cls.state != CA_ACTIVE)
		{
			return;
		}
	}

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if(!cl.snap.valid)
	{
		Com_Error(ERR_DROP, "CL_SetCGameTime: !cl.snap.valid");
	}

	// allow pause in single player
	if(sv_paused->integer && cl_paused->integer && com_sv_running->integer)
	{
		// paused
		return;
	}

	if(cl.snap.serverTime < cl.oldFrameServerTime)
	{
		// Ridah, if this is a localhost, then we are probably loading a savegame
		if(!Q_stricmp(cls.servername, "localhost"))
		{
			// do nothing?
			CL_FirstSnapshot();
		}
		else
		{
			Com_Error(ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime");
		}
	}
	cl.oldFrameServerTime = cl.snap.serverTime;


	// get our current view of time

	if(clc.demoplaying && cl_freezeDemo->integer)
	{
		// cl_freezeDemo is used to lock a demo in place for single frame advances

	}
	else
	{
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better
		// smoothness or better responsiveness.
		int             tn;

		tn = cl_timeNudge->integer;
		if(tn < -30)
		{
			tn = -30;
		}
		else if(tn > 30)
		{
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if(cl.serverTime < cl.oldServerTime)
		{
			cl.serverTime = cl.oldServerTime;
		}
		cl.oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if(cls.realtime + cl.serverTimeDelta >= cl.snap.serverTime - 5)
		{
			cl.extrapolatedSnapshot = qtrue;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if(cl.newSnapshots)
	{
		CL_AdjustTimeDelta();
	}

	if(!clc.demoplaying)
	{
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definately
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if(cl_timedemo->integer)
	{
		if(!clc.timeDemoStart)
		{
			clc.timeDemoStart = Sys_Milliseconds();
		}
		clc.timeDemoFrames++;
		cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
	}

	while(cl.serverTime >= cl.snap.serverTime)
	{
		// feed another messag, which should change
		// the contents of cl.snap
		CL_ReadDemoMessage();
		if(cls.state != CA_ACTIVE)
		{
			Cvar_Set("timescale", "1");
			return;				// end of demo
		}
	}

}

/*
====================
CL_GetTag
====================
*/
qboolean CL_GetTag(int clientNum, char *tagname, orientation_t * or)
{
	if(!cgvm)
	{
		return qfalse;
	}

	return VM_Call(cgvm, CG_GET_TAG, clientNum, tagname, or);
}
