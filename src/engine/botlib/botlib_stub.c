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

/*
 * $Id: botlib_stub.c,v 1.3 2004/01/31 04:03:49 ikkyo Exp $
 *
 * rain - this is a stub botlib so that we can compile without changes to
 * the rest of the engine.  This way, we can drop in the real botlib later
 * if we get access to it.
 *
 * Notes:
 * + The l_* files are pilfered from extractfuncs.  They work, but
 *   I believe they're a bit out-of-date versus the real versions..
 * + I don't yet return real handles for the PC functions--instead, I
 *   pass around the real pointer to the source_t struct.  This is
 *   probably a bad thing, and it should be fixed down the road.
 */

#include "../game/q_shared.h"
#include "../game/botlib.h"
#include "l_script.h"
#include "l_precomp.h"

void            botlib_stub(void);
int             PC_LoadSourceHandle(const char *);
int             PC_FreeSourceHandle(int);
int             PC_ReadTokenHandle(int, pc_token_t *);
int             PC_SourceFileAndLine(int, char *, int *);
void            PC_UnreadLastTokenHandle(int);

botlib_import_t botimport;

botlib_export_t *GetBotLibAPI(int version, botlib_import_t * imports)
{
	static botlib_export_t botlib_export;

	botimport = *imports;

	botlib_export.aas.AAS_EntityInfo = (void *)botlib_stub;
	botlib_export.aas.AAS_Initialized = (void *)botlib_stub;
	botlib_export.aas.AAS_PresenceTypeBoundingBox = (void *)botlib_stub;
	botlib_export.aas.AAS_Time = (void *)botlib_stub;
	botlib_export.aas.AAS_PointAreaNum = (void *)botlib_stub;
	botlib_export.aas.AAS_TraceAreas = (void *)botlib_stub;
	botlib_export.aas.AAS_BBoxAreas = (void *)botlib_stub;
	botlib_export.aas.AAS_AreaCenter = (void *)botlib_stub;
	botlib_export.aas.AAS_AreaWaypoint = (void *)botlib_stub;
	botlib_export.aas.AAS_PointContents = (void *)botlib_stub;
	botlib_export.aas.AAS_NextBSPEntity = (void *)botlib_stub;
	botlib_export.aas.AAS_ValueForBSPEpairKey = (void *)botlib_stub;
	botlib_export.aas.AAS_VectorForBSPEpairKey = (void *)botlib_stub;
	botlib_export.aas.AAS_FloatForBSPEpairKey = (void *)botlib_stub;
	botlib_export.aas.AAS_IntForBSPEpairKey = (void *)botlib_stub;
	botlib_export.aas.AAS_AreaReachability = (void *)botlib_stub;
	botlib_export.aas.AAS_AreaLadder = (void *)botlib_stub;
	botlib_export.aas.AAS_AreaTravelTimeToGoalArea = (void *)botlib_stub;
	botlib_export.aas.AAS_Swimming = (void *)botlib_stub;
	botlib_export.aas.AAS_PredictClientMovement = (void *)botlib_stub;
	botlib_export.aas.AAS_RT_ShowRoute = (void *)botlib_stub;
	botlib_export.aas.AAS_RT_GetHidePos = (void *)botlib_stub;
	botlib_export.aas.AAS_FindAttackSpotWithinRange = (void *)botlib_stub;
	botlib_export.aas.AAS_ListAreasInRange = (void *)botlib_stub;
	botlib_export.aas.AAS_AvoidDangerArea = (void *)botlib_stub;
	botlib_export.aas.AAS_Retreat = (void *)botlib_stub;
	botlib_export.aas.AAS_AlternativeRouteGoals = (void *)botlib_stub;
	botlib_export.aas.AAS_SetAASBlockingEntity = (void *)botlib_stub;
	botlib_export.aas.AAS_NearestHideArea = (void *)botlib_stub;
	botlib_export.aas.AAS_RecordTeamDeathArea = (void *)botlib_stub;
	botlib_export.aas.AAS_SetCurrentWorld = (void *)botlib_stub;

	botlib_export.ea.EA_Say = (void *)botlib_stub;
	botlib_export.ea.EA_SayTeam = (void *)botlib_stub;
	botlib_export.ea.EA_UseItem = (void *)botlib_stub;
	botlib_export.ea.EA_DropItem = (void *)botlib_stub;
	botlib_export.ea.EA_UseInv = (void *)botlib_stub;
	botlib_export.ea.EA_DropInv = (void *)botlib_stub;
	botlib_export.ea.EA_Gesture = (void *)botlib_stub;
	botlib_export.ea.EA_Command = (void *)botlib_stub;
	botlib_export.ea.EA_SelectWeapon = (void *)botlib_stub;
	botlib_export.ea.EA_Talk = (void *)botlib_stub;
	botlib_export.ea.EA_Attack = (void *)botlib_stub;
	botlib_export.ea.EA_Reload = (void *)botlib_stub;
	botlib_export.ea.EA_Use = (void *)botlib_stub;
	botlib_export.ea.EA_Respawn = (void *)botlib_stub;
	botlib_export.ea.EA_Jump = (void *)botlib_stub;
	botlib_export.ea.EA_DelayedJump = (void *)botlib_stub;
	botlib_export.ea.EA_Crouch = (void *)botlib_stub;
	botlib_export.ea.EA_Walk = (void *)botlib_stub;
	botlib_export.ea.EA_MoveUp = (void *)botlib_stub;
	botlib_export.ea.EA_MoveDown = (void *)botlib_stub;
	botlib_export.ea.EA_MoveForward = (void *)botlib_stub;
	botlib_export.ea.EA_MoveBack = (void *)botlib_stub;
	botlib_export.ea.EA_MoveLeft = (void *)botlib_stub;
	botlib_export.ea.EA_MoveRight = (void *)botlib_stub;
	botlib_export.ea.EA_Move = (void *)botlib_stub;
	botlib_export.ea.EA_View = (void *)botlib_stub;
	botlib_export.ea.EA_Prone = (void *)botlib_stub;
	botlib_export.ea.EA_EndRegular = (void *)botlib_stub;
	botlib_export.ea.EA_GetInput = (void *)botlib_stub;
	botlib_export.ea.EA_ResetInput = (void *)botlib_stub;

	botlib_export.ai.BotLoadCharacter = (void *)botlib_stub;
	botlib_export.ai.BotFreeCharacter = (void *)botlib_stub;
	botlib_export.ai.Characteristic_Float = (void *)botlib_stub;
	botlib_export.ai.Characteristic_BFloat = (void *)botlib_stub;
	botlib_export.ai.Characteristic_Integer = (void *)botlib_stub;
	botlib_export.ai.Characteristic_BInteger = (void *)botlib_stub;
	botlib_export.ai.Characteristic_String = (void *)botlib_stub;
	botlib_export.ai.BotAllocChatState = (void *)botlib_stub;
	botlib_export.ai.BotFreeChatState = (void *)botlib_stub;
	botlib_export.ai.BotQueueConsoleMessage = (void *)botlib_stub;
	botlib_export.ai.BotRemoveConsoleMessage = (void *)botlib_stub;
	botlib_export.ai.BotNextConsoleMessage = (void *)botlib_stub;
	botlib_export.ai.BotNumConsoleMessages = (void *)botlib_stub;
	botlib_export.ai.BotInitialChat = (void *)botlib_stub;
	botlib_export.ai.BotNumInitialChats = (void *)botlib_stub;
	botlib_export.ai.BotReplyChat = (void *)botlib_stub;
	botlib_export.ai.BotChatLength = (void *)botlib_stub;
	botlib_export.ai.BotEnterChat = (void *)botlib_stub;
	botlib_export.ai.BotGetChatMessage = (void *)botlib_stub;
	botlib_export.ai.StringContains = (void *)botlib_stub;
	botlib_export.ai.BotFindMatch = (void *)botlib_stub;
	botlib_export.ai.BotMatchVariable = (void *)botlib_stub;
	botlib_export.ai.UnifyWhiteSpaces = (void *)botlib_stub;
	botlib_export.ai.BotReplaceSynonyms = (void *)botlib_stub;
	botlib_export.ai.BotLoadChatFile = (void *)botlib_stub;
	botlib_export.ai.BotSetChatGender = (void *)botlib_stub;
	botlib_export.ai.BotSetChatName = (void *)botlib_stub;
	botlib_export.ai.BotResetGoalState = (void *)botlib_stub;
	botlib_export.ai.BotResetAvoidGoals = (void *)botlib_stub;
	botlib_export.ai.BotRemoveFromAvoidGoals = (void *)botlib_stub;
	botlib_export.ai.BotPushGoal = (void *)botlib_stub;
	botlib_export.ai.BotPopGoal = (void *)botlib_stub;
	botlib_export.ai.BotEmptyGoalStack = (void *)botlib_stub;
	botlib_export.ai.BotDumpAvoidGoals = (void *)botlib_stub;
	botlib_export.ai.BotDumpGoalStack = (void *)botlib_stub;
	botlib_export.ai.BotGoalName = (void *)botlib_stub;
	botlib_export.ai.BotGetTopGoal = (void *)botlib_stub;
	botlib_export.ai.BotGetSecondGoal = (void *)botlib_stub;
	botlib_export.ai.BotChooseLTGItem = (void *)botlib_stub;
	botlib_export.ai.BotChooseNBGItem = (void *)botlib_stub;
	botlib_export.ai.BotTouchingGoal = (void *)botlib_stub;
	botlib_export.ai.BotItemGoalInVisButNotVisible = (void *)botlib_stub;
	botlib_export.ai.BotGetLevelItemGoal = (void *)botlib_stub;
	botlib_export.ai.BotGetNextCampSpotGoal = (void *)botlib_stub;
	botlib_export.ai.BotGetMapLocationGoal = (void *)botlib_stub;
	botlib_export.ai.BotAvoidGoalTime = (void *)botlib_stub;
	botlib_export.ai.BotInitLevelItems = (void *)botlib_stub;
	botlib_export.ai.BotUpdateEntityItems = (void *)botlib_stub;
	botlib_export.ai.BotLoadItemWeights = (void *)botlib_stub;
	botlib_export.ai.BotFreeItemWeights = (void *)botlib_stub;
	botlib_export.ai.BotInterbreedGoalFuzzyLogic = (void *)botlib_stub;
	botlib_export.ai.BotSaveGoalFuzzyLogic = (void *)botlib_stub;
	botlib_export.ai.BotMutateGoalFuzzyLogic = (void *)botlib_stub;
	botlib_export.ai.BotAllocGoalState = (void *)botlib_stub;
	botlib_export.ai.BotFreeGoalState = (void *)botlib_stub;
	botlib_export.ai.BotResetMoveState = (void *)botlib_stub;
	botlib_export.ai.BotMoveToGoal = (void *)botlib_stub;
	botlib_export.ai.BotMoveInDirection = (void *)botlib_stub;
	botlib_export.ai.BotResetAvoidReach = (void *)botlib_stub;
	botlib_export.ai.BotResetLastAvoidReach = (void *)botlib_stub;
	botlib_export.ai.BotReachabilityArea = (void *)botlib_stub;
	botlib_export.ai.BotMovementViewTarget = (void *)botlib_stub;
	botlib_export.ai.BotPredictVisiblePosition = (void *)botlib_stub;
	botlib_export.ai.BotAllocMoveState = (void *)botlib_stub;
	botlib_export.ai.BotFreeMoveState = (void *)botlib_stub;
	botlib_export.ai.BotInitMoveState = (void *)botlib_stub;
	botlib_export.ai.BotInitAvoidReach = (void *)botlib_stub;
	botlib_export.ai.BotChooseBestFightWeapon = (void *)botlib_stub;
	botlib_export.ai.BotGetWeaponInfo = (void *)botlib_stub;
	botlib_export.ai.BotLoadWeaponWeights = (void *)botlib_stub;
	botlib_export.ai.BotAllocWeaponState = (void *)botlib_stub;
	botlib_export.ai.BotFreeWeaponState = (void *)botlib_stub;
	botlib_export.ai.BotResetWeaponState = (void *)botlib_stub;
	botlib_export.ai.GeneticParentsAndChildSelection = (void *)botlib_stub;

	botlib_export.BotLibSetup = (void *)botlib_stub;
	botlib_export.BotLibShutdown = (void *)botlib_stub;
	botlib_export.BotLibVarSet = (void *)botlib_stub;
	botlib_export.BotLibVarGet = (void *)botlib_stub;
	botlib_export.BotLibStartFrame = (void *)botlib_stub;
	botlib_export.BotLibLoadMap = (void *)botlib_stub;
	botlib_export.BotLibUpdateEntity = (void *)botlib_stub;
	botlib_export.Test = (void *)botlib_stub;

	botlib_export.PC_AddGlobalDefine = PC_AddGlobalDefine;
	botlib_export.PC_RemoveAllGlobalDefines = PC_RemoveAllGlobalDefines;
	botlib_export.PC_LoadSourceHandle = PC_LoadSourceHandle;
	botlib_export.PC_FreeSourceHandle = PC_FreeSourceHandle;
	botlib_export.PC_ReadTokenHandle = PC_ReadTokenHandle;
	botlib_export.PC_SourceFileAndLine = PC_SourceFileAndLine;
	botlib_export.PC_UnreadLastTokenHandle = PC_UnreadLastTokenHandle;


	return &botlib_export;
}

void botlib_stub(void)
{
	botimport.Print(PRT_WARNING, "WARNING: botlib stub!\n");
}

int PC_LoadSourceHandle(const char *filename)
{
	// rain - FIXME - LoadSourceFile should take a const filename
	return (int)LoadSourceFile(filename);
}

int PC_FreeSourceHandle(int handle)
{
	FreeSource((source_t *) handle);
	return 0;
}

int PC_ReadTokenHandle(int handle, pc_token_t * token)
{
	token_t         t;
	int             ret;

	ret = PC_ReadToken((source_t *) handle, &t);

	token->type = t.type;
	token->subtype = t.subtype;
	token->intvalue = t.intvalue;
	token->floatvalue = t.floatvalue;
	Q_strncpyz(token->string, t.string, MAX_TOKENLENGTH);
	token->line = t.line;
	token->linescrossed = t.linescrossed;

	// gamecode doesn't want the quotes on the string
	if(token->type == TT_STRING)
	{
		StripDoubleQuotes(token->string);
	}

	return ret;
}

int PC_SourceFileAndLine(int handle, char *filename, int *line)
{
	source_t       *source = (source_t *) handle;

	Q_strncpyz(filename, source->filename, 128);
	// ikkyo - i'm pretty sure token.line is the line of the last token
	//         parsed, not the line of the token currently being parsed...
	*line = source->token.line;

	return 0;
}

void PC_UnreadLastTokenHandle(int handle)
{
	PC_UnreadLastToken((source_t *) handle);
}
