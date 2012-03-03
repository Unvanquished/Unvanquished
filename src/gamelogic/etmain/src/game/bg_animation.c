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


//===========================================================================
// bg_animation.c
//
//  Incorporates several elements related to the new flexible animation system.
//
//  This includes scripting elements, support routines for new animation set
//  reference system and other bits and pieces.
//
//===========================================================================

#include "../../../../engine/qcommon/q_shared.h"
#include "bg_public.h"

// JPW NERVE -- added because I need to check single/multiplayer instances and branch accordingly
#ifdef CGAMEDLL
extern vmCvar_t cg_gameType;
#endif
#ifdef GAMEDLL
extern vmCvar_t g_gametype;
#endif

// debug defines, to prevent doing costly string cvar lookups
//#define   DBGANIMS
//#define   DBGANIMEVENTS

// this is used globally within this file to reduce redundant params
static animScriptData_t *globalScriptData = NULL;

#define MAX_ANIM_DEFINES    16

static char    *globalFilename;	// to prevent redundant params

// these are used globally during script parsing
static int      numDefines[NUM_ANIM_CONDITIONS];
static char     defineStrings[10000];	// stores the actual strings
static unsigned defineStringsOffset;
static animStringItem_t defineStr[NUM_ANIM_CONDITIONS][MAX_ANIM_DEFINES];
static int      defineBits[NUM_ANIM_CONDITIONS][MAX_ANIM_DEFINES][2];

static scriptAnimMoveTypes_t parseMovetype;
static int      parseEvent;

animStringItem_t weaponStrings[WP_NUM_WEAPONS];

animStringItem_t animStateStr[] = {
	{"RELAXED", -1},
	{"QUERY", -1},
	{"ALERT", -1},
	{"COMBAT", -1},

	{NULL, -1},
};

static animStringItem_t animMoveTypesStr[] = {
	{"** UNUSED **", -1},
	{"IDLE", -1},
	{"IDLECR", -1},
	{"WALK", -1},
	{"WALKBK", -1},
	{"WALKCR", -1},
	{"WALKCRBK", -1},
	{"RUN", -1},
	{"RUNBK", -1},
	{"SWIM", -1},
	{"SWIMBK", -1},
	{"STRAFERIGHT", -1},
	{"STRAFELEFT", -1},
	{"TURNRIGHT", -1},
	{"TURNLEFT", -1},
	{"CLIMBUP", -1},
	{"CLIMBDOWN", -1},
	{"FALLEN", -1},				// DHM - Nerve :: dead, before limbo
	{"PRONE", -1},
	{"PRONEBK", -1},
	{"IDLEPRONE", -1},
	{"FLAILING", -1},

	{NULL, -1},
};

static animStringItem_t animEventTypesStr[] = {
	{"PAIN", -1},
	{"DEATH", -1},
	{"FIREWEAPON", -1},
	{"FIREWEAPON2", -1},
	{"JUMP", -1},
	{"JUMPBK", -1},
	{"LAND", -1},
	{"DROPWEAPON", -1},
	{"RAISEWEAPON", -1},
	{"CLIMBMOUNT", -1},
	{"CLIMBDISMOUNT", -1},
	{"RELOAD", -1},
	{"PICKUPGRENADE", -1},
	{"KICKGRENADE", -1},
	{"QUERY", -1},
	{"INFORM_FRIENDLY_OF_ENEMY", -1},
	{"KICK", -1},
	{"REVIVE", -1},
	{"FIRSTSIGHT", -1},
	{"ROLL", -1},
	{"FLIP", -1},
	{"DIVE", -1},
	{"PRONE_TO_CROUCH", -1},
	{"BULLETIMPACT", -1},
	{"INSPECTSOUND", -1},
	{"SECONDLIFE", -1},
	{"DO_ALT_WEAPON_MODE", -1},
	{"UNDO_ALT_WEAPON_MODE", -1},
	{"DO_ALT_WEAPON_MODE_PRONE", -1},
	{"UNDO_ALT_WEAPON_MODE_PRONE", -1},
	{"FIREWEAPONPRONE", -1},
	{"FIREWEAPON2PRONE", -1},
	{"RAISEWEAPONPRONE", -1},
	{"RELOADPRONE", -1},
	{"TALK", -1},
	{"NOPOWER", -1},

	{NULL, -1},
};

animStringItem_t animBodyPartsStr[] = {
	{"** UNUSED **", -1},
	{"LEGS", -1},
	{"TORSO", -1},
	{"BOTH", -1},

	{NULL, -1},
};

//------------------------------------------------------------
// conditions

static animStringItem_t animConditionPositionsStr[] = {
	{"** UNUSED **", -1},
	{"BEHIND", -1},
	{"INFRONT", -1},
	{"RIGHT", -1},
	{"LEFT", -1},

	{NULL, -1},
};

static animStringItem_t animConditionMountedStr[] = {
	{"** UNUSED **", -1},
	{"MG42", -1},

	{NULL, -1},
};

static animStringItem_t animConditionLeaningStr[] = {
	{"** UNUSED **", -1},
	{"RIGHT", -1},
	{"LEFT", -1},

	{NULL, -1},
};

// !!! NOTE: this must be kept in sync with the tag names in ai_cast_characters.c
static animStringItem_t animConditionImpactPointsStr[] = {
	{"** UNUSED **", -1},
	{"HEAD", -1},
	{"CHEST", -1},
	{"GUT", -1},
	{"GROIN", -1},
	{"SHOULDER_RIGHT", -1},
	{"SHOULDER_LEFT", -1},
	{"KNEE_RIGHT", -1},
	{"KNEE_LEFT", -1},

	{NULL, -1},
};

// !!! NOTE: this must be kept in sync with the teams in ai_cast.h
static animStringItem_t animEnemyTeamsStr[] = {
	{"NAZI", -1},
	{"ALLIES", -1},
	{"MONSTER", -1},
	{"SPARE1", -1},
	{"SPARE2", -1},
	{"SPARE3", -1},
	{"SPARE4", -1},
	{"NEUTRAL", -1}
};

static animStringItem_t animHealthLevelStr[] = {
	{"1", -1},
	{"2", -1},
	{"3", -1},
};

static animStringItem_t animFlailTypeStr[] = {
	{"** UNUSED **", -1},
	{"INAIR", -1},
	{"VCRASH", -1},
	{"HCRASH", -1},

	{NULL, -1},
};

static animStringItem_t animGenBitFlagStr[] = {
/*	{"SNEAKING", -1},		// in a cover spot
	{"AFTERBATTLE", -1},	// just finished battle
	{"AGENT2", -1},			// agent2 only
	{"RELAXED", -1},		// relaxed aiState*/
	{"ZOOMING", -1},			// zooming with binoculars
};

// xkan, 1/17/2003 - sorts of duplicate aistateEnum_t.
static animStringItem_t animAIStateStr[] = {
	{"RELAXED", -1},
	{"QUERY", -1},
	{"ALERT", -1},
	{"COMBAT", -1}
};

typedef enum
{
	ANIM_CONDTYPE_BITFLAGS,
	ANIM_CONDTYPE_VALUE,

	NUM_ANIM_CONDTYPES
} animScriptConditionTypes_t;

typedef struct
{
	animScriptConditionTypes_t type;
	animStringItem_t *values;
} animConditionTable_t;

static animStringItem_t animConditionsStr[] = {
	{"WEAPONS", -1},
	{"ENEMY_POSITION", -1},
	{"ENEMY_WEAPON", -1},
	{"UNDERWATER", -1},
	{"MOUNTED", -1},
	{"MOVETYPE", -1},
	{"UNDERHAND", -1},
	{"LEANING", -1},
	{"IMPACT_POINT", -1},
	{"CROUCHING", -1},
	{"STUNNED", -1},
	{"FIRING", -1},
	{"SHORT_REACTION", -1},
	{"ENEMY_TEAM", -1},
	{"PARACHUTE", -1},
	{"CHARGING", -1},
	{"SECONDLIFE", -1},
	{"HEALTH_LEVEL", -1},
	{"FLAILING_TYPE", -1},
	{"GEN_BITFLAG", -1},
	{"AISTATE", -1},

	{NULL, -1},
};

static animConditionTable_t animConditionsTable[NUM_ANIM_CONDITIONS] = {
	{ANIM_CONDTYPE_BITFLAGS, weaponStrings},
	{ANIM_CONDTYPE_BITFLAGS, animConditionPositionsStr},
	{ANIM_CONDTYPE_BITFLAGS, weaponStrings},
	{ANIM_CONDTYPE_VALUE, NULL},
	{ANIM_CONDTYPE_VALUE, animConditionMountedStr},
	{ANIM_CONDTYPE_BITFLAGS, animMoveTypesStr},
	{ANIM_CONDTYPE_VALUE, NULL},
	{ANIM_CONDTYPE_VALUE, animConditionLeaningStr},
	{ANIM_CONDTYPE_VALUE, animConditionImpactPointsStr},
	{ANIM_CONDTYPE_VALUE, NULL},
	{ANIM_CONDTYPE_VALUE, NULL},
	{ANIM_CONDTYPE_VALUE, NULL},
	{ANIM_CONDTYPE_VALUE, NULL},
	{ANIM_CONDTYPE_VALUE, animEnemyTeamsStr},
	{ANIM_CONDTYPE_VALUE, NULL},
	{ANIM_CONDTYPE_VALUE, NULL},
	{ANIM_CONDTYPE_VALUE, NULL},
	{ANIM_CONDTYPE_VALUE, animHealthLevelStr},
	{ANIM_CONDTYPE_VALUE, animFlailTypeStr},
	{ANIM_CONDTYPE_BITFLAGS, animGenBitFlagStr},
	{ANIM_CONDTYPE_VALUE, animAIStateStr},
};

//------------------------------------------------------------

/*
================
return a hash value for the given string
================
*/
long BG_StringHashValue(const char *fname)
{
	int             i;
	long            hash;

	if(!fname)
	{
		return -1;
	}

	hash = 0;
	i = 0;
	while(fname[i] != '\0')
	{
		if(Q_isupper(fname[i]))
		{
			hash += (long)(fname[i] + ('a' - 'A')) * (i + 119);
		}
		else
		{
			hash += (long)(fname[i]) * (i + 119);
		}

		i++;
	}
	if(hash == -1)
	{
		hash = 0;				// never return -1
	}
	return hash;
}

/*
================
return a hash value for the given string (make sure the strings and lowered first)
================
*/
long BG_StringHashValue_Lwr(const char *fname)
{
	int             i;
	long            hash;

	hash = 0;
	i = 0;
	while(fname[i] != '\0')
	{
		hash += (long)(fname[i]) * (i + 119);
		i++;
	}
	if(hash == -1)
	{
		hash = 0;				// never return -1
	}
	return hash;
}

/*
=================
BG_AnimParseError
=================
*/
void QDECL __attribute__((format(printf, 1, 2))) BG_AnimParseError(const char *msg, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	if(globalFilename)
	{
		Com_Error(ERR_DROP, "%s: (%s, line %i)", text, globalFilename, COM_GetCurrentParseLine() + 1);
	}
	else
	{
		Com_Error(ERR_DROP, "%s", text);
	}
}

/*
=================
BG_ModelInfoForClient
=================
*/
/*animModelInfo_t *BG_ModelInfoForClient( int client ) {
	if (!globalScriptData)
		BG_AnimParseError( "BG_ModelInfoForClient: NULL globalScriptData" );
	//
	if (!globalScriptData->clientModels[client])
		BG_AnimParseError( "BG_ModelInfoForClient: client %i has no modelinfo", client );
	//
	return &globalScriptData->modelInfo[globalScriptData->clientModels[client] - 1];
}*/

/*
=================
BG_ModelInfoForModelname
=================
*/
/*animModelInfo_t *BG_ModelInfoForModelname( char *modelname ) {
	int i;
	animModelInfo_t *modelInfo;
	//
	if (!globalScriptData)
		BG_AnimParseError( "BG_ModelInfoForModelname: NULL globalScriptData" );
	//
	for (i=0, modelInfo=globalScriptData->modelInfo; i<MAX_ANIMSCRIPT_MODELS; i++, modelInfo++) {
		if (!modelInfo->modelname[0])
			continue;
		if (!Q_stricmp( modelname, modelInfo->modelname )) {
			return modelInfo;
		}
	}
	//
	return NULL;
}*/

/*
=================
BG_AnimationIndexForString
=================
*/
static int BG_AnimationIndexForString(char *string, animModelInfo_t * animModelInfo)
{
	int             i, hash;
	animation_t    *anim;

	hash = BG_StringHashValue(string);

	for(i = 0; i < animModelInfo->numAnimations; i++)
	{
		anim = animModelInfo->animations[i];
		if((hash == anim->nameHash) && !Q_stricmp(string, anim->name))
		{
			// found a match
			return i;
		}
	}
	// no match found
	BG_AnimParseError("BG_AnimationIndexForString: unknown index '%s' for animation group '%s'", string,
					  animModelInfo->animationGroup);
	return -1;					// shutup compiler
}

/*
=================
BG_AnimationForString
=================
*/
animation_t    *BG_AnimationForString(char *string, animModelInfo_t * animModelInfo)
{
	int             i, hash;
	animation_t    *anim;

	hash = BG_StringHashValue(string);

	for(i = 0; i < animModelInfo->numAnimations; i++)
	{
		anim = animModelInfo->animations[i];
		if((hash == anim->nameHash) && !Q_stricmp(string, anim->name))
		{
			// found a match
			return anim;
		}
	}
	// no match found
	Com_Error(ERR_DROP, "BG_AnimationForString: unknown animation '%s' for animation group '%s'", string,
			  animModelInfo->animationGroup);
	return NULL;				// shutup compiler
}

/*
=================
BG_IndexForString

  errors out if no match found
=================
*/
int BG_IndexForString(char *token, animStringItem_t * strings, qboolean allowFail)
{
	int             i, hash;
	animStringItem_t *strav;

	hash = BG_StringHashValue(token);

	for(i = 0, strav = strings; strav->string; strav++, i++)
	{
		if(strav->hash == -1)
		{
			strav->hash = BG_StringHashValue(strav->string);
		}
		if((hash == strav->hash) && !Q_stricmp(token, strav->string))
		{
			// found a match
			return i;
		}
	}
	// no match found
	if(!allowFail)
	{
		BG_AnimParseError("BG_IndexForString: unknown token '%s'", token);
	}
	//
	return -1;
}

/*
===============
BG_CopyStringIntoBuffer
===============
*/
// CHRUKER: b069 - Cleaned up a few compiler warnings
char           *BG_CopyStringIntoBuffer(char *string, char *buffer, unsigned int bufSize, unsigned int *offset)
{
	char           *pch;

	// check for overloaded buffer
	if(*offset + strlen(string) + 1 >= bufSize)
	{
		BG_AnimParseError("BG_CopyStringIntoBuffer: out of buffer space");
	}

	pch = &buffer[*offset];

	// safe to do a strcpy since we've already checked for overrun
	strcpy(pch, string);

	// move the offset along
	*offset += strlen(string) + 1;

	return pch;
}

/*
============
BG_InitWeaponStrings

  Builds the list of weapon names from the item list. This is done here rather
  than hardcoded to ease the process of modifying the weapons.
============
*/
void BG_InitWeaponStrings(void)
{
	int             i;
	gitem_t        *item;

	memset(weaponStrings, 0, sizeof(weaponStrings));

	for(i = 0; i < WP_NUM_WEAPONS; i++)
	{
		// find this weapon in the itemslist, and extract the name
		for(item = bg_itemlist + 1; item->classname; item++)
		{
			if(item->giType == IT_WEAPON && item->giTag == i)
			{
				// found a match
				weaponStrings[i].string = item->pickup_name;
				weaponStrings[i].hash = BG_StringHashValue(weaponStrings[i].string);
				break;
			}
		}

		if(!item->classname)
		{
			weaponStrings[i].string = "(unknown)";
			weaponStrings[i].hash = BG_StringHashValue(weaponStrings[i].string);
		}
	}
}

/*
=================
BG_ParseConditionBits

  convert the string into a single int containing bit flags, stopping at a ',' or end of line
=================
*/
void BG_ParseConditionBits(char **text_pp, animStringItem_t * stringTable, int condIndex, int result[2])
{
	qboolean        endFlag = qfalse;
	int             indexFound;
	int /*indexBits, */ tempBits[2];
	char            currentString[MAX_QPATH];
	qboolean        minus = qfalse;
	char           *token;

	//indexBits = 0;
	currentString[0] = '\0';
	memset(result, 0, 2 * sizeof(result[0]));
	memset(tempBits, 0, sizeof(tempBits));

	while(!endFlag)
	{

		token = COM_ParseExt(text_pp, qfalse);
		if(!token || !token[0])
		{
			COM_RestoreParseSession(text_pp);	// go back to the previous token
			endFlag = qtrue;	// done parsing indexes
			if(!strlen(currentString))
			{
				break;
			}
		}

		if(!Q_stricmp(token, ","))
		{
			endFlag = qtrue;	// end of indexes
		}

		if(!Q_stricmp(token, "none"))
		{						// first bit is always the "unused" bit
			COM_BitSet(result, 0);
			continue;
		}

		if(!Q_stricmp(token, "none,"))
		{						// first bit is always the "unused" bit
			COM_BitSet(result, 0);
			endFlag = qtrue;	// end of indexes
			continue;
		}

		if(!Q_stricmp(token, "NOT"))
		{
			token = "MINUS";	// NOT is equivalent to MINUS
		}

		if(!endFlag && Q_stricmp(token, "AND") && Q_stricmp(token, "MINUS"))
		{						// must be a index
			// check for a comma (end of indexes)
			if(token[strlen(token) - 1] == ',')
			{
				endFlag = qtrue;
				token[strlen(token) - 1] = '\0';
			}
			// append this to the currentString
			if(strlen(currentString))
			{
				Q_strcat(currentString, sizeof(currentString), " ");
			}
			Q_strcat(currentString, sizeof(currentString), token);
		}

		if(!Q_stricmp(token, "AND") || !Q_stricmp(token, "MINUS") || endFlag)
		{
			// process the currentString
			if(!strlen(currentString))
			{
				if(endFlag)
				{
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected end of condition");
				}
				else
				{
					// check for minus indexes to follow
					if(!Q_stricmp(token, "MINUS"))
					{
						minus = qtrue;
						continue;
					}
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
				}
			}
			if(!Q_stricmp(currentString, "all"))
			{
				tempBits[0] = ~0x0;
				tempBits[1] = ~0x0;
			}
			else
			{
				// first check this string with our defines
				indexFound = BG_IndexForString(currentString, defineStr[condIndex], qtrue);
				if(indexFound >= 0)
				{
					// we have precalculated the bitflags for the defines
					tempBits[0] = defineBits[condIndex][indexFound][0];
					tempBits[1] = defineBits[condIndex][indexFound][1];
				}
				else
				{
					// convert the string into an index
					indexFound = BG_IndexForString(currentString, stringTable, qfalse);
					// convert the index into a bitflag
					COM_BitSet(tempBits, indexFound);
				}
			}
			// perform operation
			if(minus)
			{					// subtract
				result[0] &= ~tempBits[0];
				result[1] &= ~tempBits[1];
			}
			else
			{					// add
				result[0] |= tempBits[0];
				result[1] |= tempBits[1];
			}
			// clear the currentString
			currentString[0] = '\0';
			// check for minus indexes to follow
			if(!Q_stricmp(token, "MINUS"))
			{
				minus = qtrue;
			}
		}

	}
}

/*
=================
BG_ParseConditions

  returns qtrue if everything went ok, error drops otherwise
=================
*/
qboolean BG_ParseConditions(char **text_pp, animScriptItem_t * scriptItem)
{
	int             conditionIndex, conditionValue[2];
	char           *token;

	conditionValue[0] = 0;
	conditionValue[1] = 0;

	while(1)
	{

		token = COM_ParseExt(text_pp, qfalse);
		if(!token || !token[0])
		{
			break;
		}

		// special case, "default" has no conditions
		if(!Q_stricmp(token, "default"))
		{
			return qtrue;
		}

		conditionIndex = BG_IndexForString(token, animConditionsStr, qfalse);

		switch (animConditionsTable[conditionIndex].type)
		{
			case ANIM_CONDTYPE_BITFLAGS:
				BG_ParseConditionBits(text_pp, animConditionsTable[conditionIndex].values, conditionIndex, conditionValue);
				break;
			case ANIM_CONDTYPE_VALUE:
				if(animConditionsTable[conditionIndex].values)
				{
					token = COM_ParseExt(text_pp, qfalse);
					if(!token || !token[0])
					{
						BG_AnimParseError("BG_AnimParseAnimScript: expected condition value, found end of line");	// RF modification
					}
					// check for a comma (condition divider)
					if(token[strlen(token) - 1] == ',')
					{
						token[strlen(token) - 1] = '\0';
					}
					conditionValue[0] = BG_IndexForString(token, animConditionsTable[conditionIndex].values, qfalse);
				}
				else
				{
					conditionValue[0] = 1;	// not used, just check for a positive condition
				}
				break;
			default:			// TTimo gcc: NUM_ANIM_CONDTYPES not handled in switch
				break;
		}

		// now append this condition to the item
		scriptItem->conditions[scriptItem->numConditions].index = conditionIndex;
		scriptItem->conditions[scriptItem->numConditions].value[0] = conditionValue[0];
		scriptItem->conditions[scriptItem->numConditions].value[1] = conditionValue[1];
		scriptItem->numConditions++;
	}

	if(scriptItem->numConditions == 0)
	{
		BG_AnimParseError("BG_ParseConditions: no conditions found");	// RF mod
	}

	return qtrue;
}

/*
=================
BG_ParseCommands
=================
*/
static void BG_ParseCommands(char **input, animScriptItem_t * scriptItem, animModelInfo_t * animModelInfo,
							 animScriptData_t * scriptData)
{
	char           *token;

	// TTimo gcc: might be used uninitialized
	animScriptCommand_t *command = NULL;
	int             partIndex = 0;

	while(1)
	{
		// parse the body part
		token = COM_ParseExt(input, (partIndex < 1));
		if(!token || !*token)
		{
			break;
		}
		if(!Q_stricmp(token, "}"))
		{
			// unget the bracket and get out of here
			*input -= strlen(token);
			break;
		}

		// new command?
		if(partIndex == 0)
		{
			// have we exceeded the maximum number of commands?
			if(scriptItem->numCommands >= MAX_ANIMSCRIPT_ANIMCOMMANDS)
			{
				BG_AnimParseError("BG_ParseCommands: exceeded maximum number of animations (%i)", MAX_ANIMSCRIPT_ANIMCOMMANDS);
			}
			command = &scriptItem->commands[scriptItem->numCommands++];
			memset(command, 0, sizeof(*command));
		}

		command->bodyPart[partIndex] = BG_IndexForString(token, animBodyPartsStr, qtrue);
		if(command->bodyPart[partIndex] > 0)
		{
			// parse the animation
			token = COM_ParseExt(input, qfalse);
			if(!token || !token[0])
			{
				BG_AnimParseError("BG_ParseCommands: expected animation");
			}
			command->animIndex[partIndex] = BG_AnimationIndexForString(token, animModelInfo);
			command->animDuration[partIndex] = animModelInfo->animations[command->animIndex[partIndex]]->duration;
			// if this is a locomotion, set the movetype of the animation so we can reverse engineer the movetype from the animation, on the client
			if(parseMovetype != ANIM_MT_UNUSED && command->bodyPart[partIndex] != ANIM_BP_TORSO)
			{
				animModelInfo->animations[command->animIndex[partIndex]]->movetype |= (1 << parseMovetype);
			}
			// if this is a fireweapon event, then this is a firing animation
			if(parseEvent == ANIM_ET_FIREWEAPON || parseEvent == ANIM_ET_FIREWEAPONPRONE)
			{
				animModelInfo->animations[command->animIndex[partIndex]]->flags |= ANIMFL_FIRINGANIM;
				animModelInfo->animations[command->animIndex[partIndex]]->initialLerp = 40;
			}
			// check for a duration for this animation instance
			token = COM_ParseExt(input, qfalse);
			if(token && token[0])
			{
				if(!Q_stricmp(token, "duration"))
				{
					// read the duration
					token = COM_ParseExt(input, qfalse);
					if(!token || !token[0])
					{
						BG_AnimParseError("BG_ParseCommands: expected duration value");
					}
					command->animDuration[partIndex] = atoi(token);
				}
				else
				{				// unget the token
					COM_RestoreParseSession(input);
				}
			}
			else
			{
				COM_RestoreParseSession(input);
			}

			if(command->bodyPart[partIndex] != ANIM_BP_BOTH && partIndex++ < 1)
			{
				continue;		// allow parsing of another bodypart
			}
		}
		else
		{
			// unget the token
			*input -= strlen(token);
		}

		// parse optional parameters (sounds, etc)
		while(1)
		{
			token = COM_ParseExt(input, qfalse);
			if(!token || !token[0])
			{
				break;
			}

			if(!Q_stricmp(token, "sound"))
			{

				token = COM_ParseExt(input, qfalse);
				if(!token || !token[0])
				{
					BG_AnimParseError("BG_ParseCommands: expected sound");
				}
				// NOTE: only sound script are supported at this stage
				if(strstr(token, ".wav"))
				{
					BG_AnimParseError("BG_ParseCommands: wav files not supported, only sound scripts");	// RF mod
				}
				// ydnar: this was crashing because soundIndex wasn't initialized
				// FIXME: find the reason
				// Gordon: hmmm, soundindex is setup on both client and server :/
				//  cgs.animScriptData.soundIndex = CG_SoundScriptPrecache;
				//  level.animScriptData.soundIndex = G_SoundIndex;
				command->soundIndex = globalScriptData->soundIndex != NULL ? globalScriptData->soundIndex(token) : 0;

			}
			else
			{
				// unknown??
				BG_AnimParseError("BG_ParseCommands: unknown parameter '%s'", token);
			}
		}

		partIndex = 0;
	}
}

/*
=================
BG_AnimParseAnimScript

  Parse the animation script for this model, converting it into run-time structures
=================
*/

typedef enum
{
	PARSEMODE_DEFINES,
	PARSEMODE_ANIMATION,
	PARSEMODE_CANNED_ANIMATIONS,
	PARSEMODE_STATECHANGES,
	PARSEMODE_EVENTS
} animScriptParseMode_t;

static animStringItem_t animParseModesStr[] = {
	{"defines", -1},
	{"animations", -1},
	{"canned_animations", -1},
	{"statechanges", -1},
	{"events", -1},

	{NULL, -1},
};

void BG_AnimParseAnimScript(animModelInfo_t * animModelInfo, animScriptData_t * scriptData, const char *filename, char *input)
{
#define MAX_INDENT_LEVELS   3

	// FIXME: change this to use the botlib parser

	char           *text_p, *token;
	animScriptParseMode_t parseMode;
	animScript_t   *currentScript;
	animScriptItem_t tempScriptItem;

	// TTimo gcc: might be used unitialized
	animScriptItem_t *currentScriptItem = NULL;
	int             indexes[MAX_INDENT_LEVELS], indentLevel, /*oldState, */ newParseMode;
	int             i, defineType;

	// the scriptData passed into here must be the one this binary is using
	globalScriptData = scriptData;

	// start at the defines
	parseMode = PARSEMODE_DEFINES;

	// init the global defines
	globalFilename = (char *)filename;
	memset(defineStr, 0, sizeof(defineStr));
	memset(defineStrings, 0, sizeof(defineStrings));
	memset(numDefines, 0, sizeof(numDefines));
	defineStringsOffset = 0;

	for(i = 0; i < MAX_INDENT_LEVELS; i++)
		indexes[i] = -1;
	indentLevel = 0;
	currentScript = NULL;

	text_p = input;
	COM_BeginParseSession("BG_AnimParseAnimScript");

	// read in the weapon defines
	while(1)
	{
		token = COM_Parse(&text_p);
		if(!token || !*token)
		{
			if(indentLevel)
			{
				BG_AnimParseError("BG_AnimParseAnimScript: unexpected end of file");
			}
			break;
		}

		// check for a new section
		newParseMode = BG_IndexForString(token, animParseModesStr, qtrue);
		if(newParseMode >= 0)
		{
			if(indentLevel)
			{
				BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);	// RF mod
			}

			parseMode = newParseMode;
			parseMovetype = ANIM_MT_UNUSED;
			parseEvent = -1;
			continue;
		}

		switch (parseMode)
		{

			case PARSEMODE_DEFINES:

				if(!Q_stricmp(token, "set"))
				{

					// read in the define type
					token = COM_ParseExt(&text_p, qfalse);
					if(!token || !*token)
					{
						BG_AnimParseError("BG_AnimParseAnimScript: expected condition type string");	// RF mod
					}
					defineType = BG_IndexForString(token, animConditionsStr, qfalse);

					// read in the define
					token = COM_ParseExt(&text_p, qfalse);
					if(!token || !*token)
					{
						BG_AnimParseError("BG_AnimParseAnimScript: expected condition define string");	// RF mod
					}

					// copy the define to the strings list
					defineStr[defineType][numDefines[defineType]].string =
						BG_CopyStringIntoBuffer(token, defineStrings, sizeof(defineStrings), &defineStringsOffset);
					defineStr[defineType][numDefines[defineType]].hash =
						BG_StringHashValue(defineStr[defineType][numDefines[defineType]].string);
					// expecting an =
					token = COM_ParseExt(&text_p, qfalse);
					if(!token)
					{
						BG_AnimParseError("BG_AnimParseAnimScript: expected '=', found end of line");	// RF mod
					}
					if(Q_stricmp(token, "="))
					{
						BG_AnimParseError("BG_AnimParseAnimScript: expected '=', found '%s'", token);	// RF mod
					}

					// parse the bits
					BG_ParseConditionBits(&text_p, animConditionsTable[defineType].values, defineType,
										  defineBits[defineType][numDefines[defineType]]);
					numDefines[defineType]++;

					// copy the weapon defines over to the enemy_weapon defines
					memcpy(&defineStr[ANIM_COND_ENEMY_WEAPON][0], &defineStr[ANIM_COND_WEAPON][0],
						   sizeof(animStringItem_t) * MAX_ANIM_DEFINES);
					memcpy(&defineBits[ANIM_COND_ENEMY_WEAPON][0], &defineBits[ANIM_COND_WEAPON][0],
						   sizeof(defineBits[ANIM_COND_ENEMY_WEAPON][0]) * MAX_ANIM_DEFINES);
					numDefines[ANIM_COND_ENEMY_WEAPON] = numDefines[ANIM_COND_WEAPON];

				}

				break;

			case PARSEMODE_ANIMATION:
			case PARSEMODE_CANNED_ANIMATIONS:

				if(!Q_stricmp(token, "{"))
				{

					// about to increment indent level, check that we have enough information to do this
					if(indentLevel >= MAX_INDENT_LEVELS)
					{			// too many indentations
						BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);	// RF mod
					}
					if(indexes[indentLevel] < 0)
					{			// we havent found out what this new group is yet
						BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);	// RF mod
					}
					//
					indentLevel++;

				}
				else if(!Q_stricmp(token, "}"))
				{

					// reduce the indentLevel
					indentLevel--;
					if(indentLevel < 0)
					{
						BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);	// RF mod
					}
					if(indentLevel == 1)
					{
						currentScript = NULL;
					}
					// make sure we read a new index before next indent
					indexes[indentLevel] = -1;

				}
				else if(indentLevel == 0 && (indexes[indentLevel] < 0))
				{

					if(Q_stricmp(token, "state"))
					{
						BG_AnimParseError("BG_AnimParseAnimScript: expected 'state'");	// RF mod
					}

					// read in the state type
					token = COM_ParseExt(&text_p, qfalse);
					if(!token)
					{
						BG_AnimParseError("BG_AnimParseAnimScript: expected state type");	// RF mod
					}
					indexes[indentLevel] = BG_IndexForString(token, animStateStr, qfalse);

//----(SA) // RF mod
					// check for the open bracket
					token = COM_ParseExt(&text_p, qtrue);
					if(!token || Q_stricmp(token, "{"))
					{
						BG_AnimParseError("BG_AnimParseAnimScript: expected '{'");
					}
					indentLevel++;
//----(SA) // RF mod
				}
				else if((indentLevel == 1) && (indexes[indentLevel] < 0))
				{

					// we are expecting a movement type
					indexes[indentLevel] = BG_IndexForString(token, animMoveTypesStr, qfalse);
					if(parseMode == PARSEMODE_ANIMATION)
					{
						currentScript = &animModelInfo->scriptAnims[indexes[0]][indexes[1]];
						parseMovetype = indexes[1];
					}
					else if(parseMode == PARSEMODE_CANNED_ANIMATIONS)
					{
						currentScript = &animModelInfo->scriptCannedAnims[indexes[1]];
					}
					memset(currentScript, 0, sizeof(*currentScript));

				}
				else if((indentLevel == 2) && (indexes[indentLevel] < 0))
				{

					// we are expecting a condition specifier
					// move the text_p backwards so we can read in the last token again
					text_p -= strlen(token);
					// sanity check that
					if(Q_strncmp(text_p, token, strlen(token)))
					{
						// this should never happen, just here to check that this operation is correct before code goes live
						BG_AnimParseError("BG_AnimParseAnimScript: internal error");
					}
					//
					memset(&tempScriptItem, 0, sizeof(tempScriptItem));
					indexes[indentLevel] = BG_ParseConditions(&text_p, &tempScriptItem);
					// do we have enough room in this script for another item?
					if(currentScript->numItems >= MAX_ANIMSCRIPT_ITEMS)
					{
						BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum items per script (%i)", MAX_ANIMSCRIPT_ITEMS);	// RF mod
					}
					// are there enough items left in the global list?
					if(animModelInfo->numScriptItems >= MAX_ANIMSCRIPT_ITEMS_PER_MODEL)
					{
						BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum global items (%i)", MAX_ANIMSCRIPT_ITEMS_PER_MODEL);	// RF mod
					}
					// it was parsed ok, so grab an item from the global list to use
					currentScript->items[currentScript->numItems] = &animModelInfo->scriptItems[animModelInfo->numScriptItems++];
					currentScriptItem = currentScript->items[currentScript->numItems];
					currentScript->numItems++;
					// copy the data across from the temp script item
					*currentScriptItem = tempScriptItem;

				}
				else if(indentLevel == 3)
				{

					// we are reading the commands, so parse this line as if it were a command
					// move the text_p backwards so we can read in the last token again
					text_p -= strlen(token);
					// sanity check that
					if(Q_strncmp(text_p, token, strlen(token)))
					{
						// this should never happen, just here to check that this operation is correct before code goes live
						BG_AnimParseError("BG_AnimParseAnimScript: internal error");
					}
					//
					BG_ParseCommands(&text_p, currentScriptItem, animModelInfo, scriptData);

				}
				else
				{

					// huh ??
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);	// RF mod

				}

				break;

//      case PARSEMODE_STATECHANGES:
			case PARSEMODE_EVENTS:

				if(!Q_stricmp(token, "{"))
				{

					// about to increment indent level, check that we have enough information to do this
					if(indentLevel >= MAX_INDENT_LEVELS)
					{			// too many indentations
						BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);	// RF mod
					}
					if(indexes[indentLevel] < 0)
					{			// we havent found out what this new group is yet
						BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);	// RF mod
					}
					//
					indentLevel++;

				}
				else if(!Q_stricmp(token, "}"))
				{

					// reduce the indentLevel
					indentLevel--;
					if(indentLevel < 0)
					{
						BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);	// RF mod
					}
					if(indentLevel == 0)
					{
						currentScript = NULL;
					}
					// make sure we read a new index before next indent
					indexes[indentLevel] = -1;

				}
				else if(indentLevel == 0 && (indexes[indentLevel] < 0))
				{

/*				if ( parseMode == PARSEMODE_STATECHANGES ) {

					if ( Q_stricmp( token, "statechange" ) ) {
						BG_AnimParseError( "BG_AnimParseAnimScript: expected 'statechange', got '%s'", token );	// RF mod
					}

					// read in the old state type
					token = COM_ParseExt( &text_p, qfalse );
					if ( !token ) {
						BG_AnimParseError( "BG_AnimParseAnimScript: expected <state type>" );	// RF mod
					}
					oldState = BG_IndexForString( token, animStateStr, qfalse );

					// read in the new state type
					token = COM_ParseExt( &text_p, qfalse );
					if ( !token ) {
						BG_AnimParseError( "BG_AnimParseAnimScript: expected <state type>" );	// RF mod
					}
					indexes[indentLevel] = BG_IndexForString( token, animStateStr, qfalse );

					currentScript = &animModelInfo->scriptStateChange[oldState][indexes[indentLevel]];

//----(SA)		// RF mod
					// check for the open bracket
					token = COM_ParseExt( &text_p, qtrue );
					if ( !token || Q_stricmp( token, "{" ) ) {
						BG_AnimParseError( "BG_AnimParseAnimScript: expected '{'" );
					}
					indentLevel++;
//----(SA)		// RF mod
				} else {*/

					// read in the event type
					indexes[indentLevel] = BG_IndexForString(token, animEventTypesStr, qfalse);
					currentScript = &animModelInfo->scriptEvents[indexes[0]];

					parseEvent = indexes[indentLevel];

//              }

					memset(currentScript, 0, sizeof(*currentScript));

				}
				else if((indentLevel == 1) && (indexes[indentLevel] < 0))
				{

					// we are expecting a condition specifier
					// move the text_p backwards so we can read in the last token again
					text_p -= strlen(token);
					// sanity check that
					if(Q_strncmp(text_p, token, strlen(token)))
					{
						// this should never happen, just here to check that this operation is correct before code goes live
						BG_AnimParseError("BG_AnimParseAnimScript: internal error");
					}
					//
					memset(&tempScriptItem, 0, sizeof(tempScriptItem));
					indexes[indentLevel] = BG_ParseConditions(&text_p, &tempScriptItem);
					// do we have enough room in this script for another item?
					if(currentScript->numItems >= MAX_ANIMSCRIPT_ITEMS)
					{
						BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum items per script (%i)", MAX_ANIMSCRIPT_ITEMS);	// RF mod
					}
					// are there enough items left in the global list?
					if(animModelInfo->numScriptItems >= MAX_ANIMSCRIPT_ITEMS_PER_MODEL)
					{
						BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum global items (%i)", MAX_ANIMSCRIPT_ITEMS_PER_MODEL);	// RF mod
					}
					// it was parsed ok, so grab an item from the global list to use
					currentScript->items[currentScript->numItems] = &animModelInfo->scriptItems[animModelInfo->numScriptItems++];
					currentScriptItem = currentScript->items[currentScript->numItems];
					currentScript->numItems++;
					// copy the data across from the temp script item
					*currentScriptItem = tempScriptItem;

				}
				else if(indentLevel == 2)
				{

					// we are reading the commands, so parse this line as if it were a command
					// move the text_p backwards so we can read in the last token again
					text_p -= strlen(token);
					// sanity check that
					if(Q_strncmp(text_p, token, strlen(token)))
					{
						// this should never happen, just here to check that this operation is correct before code goes live
						BG_AnimParseError("BG_AnimParseAnimScript: internal error");
					}
					//
					BG_ParseCommands(&text_p, currentScriptItem, animModelInfo, scriptData);

				}
				else
				{

					// huh ??
					BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);	// RF mod

				}

			default:
				break;
		}
	}

	globalFilename = NULL;

}

//------------------------------------------------------------------------
//
// run-time gameplay functions, these are called during gameplay, so they must be
// cpu efficient.
//

/*
===============
BG_EvaluateConditions

  returns qfalse if the set of conditions fails, qtrue otherwise
===============
*/
qboolean BG_EvaluateConditions(int client, animScriptItem_t * scriptItem)
{
	int             i;
	animScriptCondition_t *cond;

	for(i = 0, cond = scriptItem->conditions; i < scriptItem->numConditions; i++, cond++)
	{
		switch (animConditionsTable[cond->index].type)
		{
			case ANIM_CONDTYPE_BITFLAGS:
				if(!(globalScriptData->clientConditions[client][cond->index][0] & cond->value[0]) &&
				   !(globalScriptData->clientConditions[client][cond->index][1] & cond->value[1]))
				{
					return qfalse;
				}
				break;
			case ANIM_CONDTYPE_VALUE:
				if(!(globalScriptData->clientConditions[client][cond->index][0] == cond->value[0]))
				{
					return qfalse;
				}
				break;
			default:			// TTimo NUM_ANIM_CONDTYPES not handled
				break;
		}
	}
	//
	// all conditions must have passed
	return qtrue;
}

/*
===============
BG_FirstValidItem

  scroll through the script items, returning the first script found to pass all conditions

  returns NULL if no match found
===============
*/
animScriptItem_t *BG_FirstValidItem(int client, animScript_t * script)
{
	animScriptItem_t **ppScriptItem;

	int             i;

	for(i = 0, ppScriptItem = script->items; i < script->numItems; i++, ppScriptItem++)
	{
		if(BG_EvaluateConditions(client, *ppScriptItem))
		{
			return *ppScriptItem;
		}
	}
	//
	return NULL;
}

/*
=====================
BG_ClearAnimTimer: clears the animation timer.
	this is useful when we want to break a animscriptevent
=====================
*/
void BG_ClearAnimTimer(playerState_t * ps, animBodyPart_t bodyPart)
{
	switch (bodyPart)
	{
		case ANIM_BP_LEGS:

			ps->legsTimer = 0;
			break;

		case ANIM_BP_TORSO:
			ps->torsoTimer = 0;
			break;

		case ANIM_BP_BOTH:
		default:
			ps->legsTimer = 0;
			ps->torsoTimer = 0;
			break;
	}
}

/*
===============
BG_PlayAnim
===============
*/
int BG_PlayAnim(playerState_t * ps, animModelInfo_t * animModelInfo, int animNum, animBodyPart_t bodyPart, int forceDuration,
				qboolean setTimer, qboolean isContinue, qboolean force)
{
	int             duration;
	qboolean        wasSet = qfalse;

	if(forceDuration)
	{
		duration = forceDuration;
	}
	else
	{
		duration = animModelInfo->animations[animNum]->duration + 50;	// account for lerping between anims
	}

	switch (bodyPart)
	{
		case ANIM_BP_BOTH:
		case ANIM_BP_LEGS:

			if((ps->legsTimer < 50) || force)
			{
				if(!isContinue || !((ps->legsAnim & ~ANIM_TOGGLEBIT) == animNum))
				{
					wasSet = qtrue;
					ps->legsAnim = ((ps->legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | animNum;
					if(setTimer)
					{
						ps->legsTimer = duration;
					}
				}
				else if(setTimer && animModelInfo->animations[animNum]->loopFrames)
				{
					ps->legsTimer = duration;
				}
			}

			if(bodyPart == ANIM_BP_LEGS)
			{
				break;
			}

		case ANIM_BP_TORSO:

			if((ps->torsoTimer < 50) || force)
			{
				if(!isContinue || !((ps->torsoAnim & ~ANIM_TOGGLEBIT) == animNum))
				{
					ps->torsoAnim = ((ps->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | animNum;
					if(setTimer)
					{
						ps->torsoTimer = duration;
					}
				}
				else if(setTimer && animModelInfo->animations[animNum]->loopFrames)
				{
					ps->torsoTimer = duration;
				}
			}

			break;
		default:				// TTimo default ANIM_BP_UNUSED NUM_ANIM_BODYPARTS not handled
			break;
	}

	if(!wasSet)
	{
		return -1;
	}

	return duration;
}

/*
===============
BG_PlayAnimName
===============
*/
int BG_PlayAnimName(playerState_t * ps, animModelInfo_t * animModelInfo, char *animName, animBodyPart_t bodyPart,
					qboolean setTimer, qboolean isContinue, qboolean force)
{
	//return BG_PlayAnim( ps, BG_AnimationIndexForString( animName, BG_GetCharacterForPlayerstate( ps )->animModelInfo ), bodyPart, 0, setTimer, isContinue, force );
	return BG_PlayAnim(ps, animModelInfo, BG_AnimationIndexForString(animName, animModelInfo), bodyPart, 0, setTimer, isContinue,
					   force);
}

/*
===============
BG_ExecuteCommand

  returns the duration of the animation, -1 if no anim was set
===============
*/
int BG_ExecuteCommand(playerState_t * ps, animModelInfo_t * animModelInfo, animScriptCommand_t * scriptCommand, qboolean setTimer,
					  qboolean isContinue, qboolean force)
{
	int             duration = -1;
	qboolean        playedLegsAnim = qfalse;

	if(scriptCommand->bodyPart[0])
	{
		duration = scriptCommand->animDuration[0] + 50;
		// FIXME: how to sync torso/legs anims accounting for transition blends, etc
		if(scriptCommand->bodyPart[0] == ANIM_BP_BOTH || scriptCommand->bodyPart[0] == ANIM_BP_LEGS)
		{
			playedLegsAnim =
				(BG_PlayAnim
				 (ps, animModelInfo, scriptCommand->animIndex[0], scriptCommand->bodyPart[0], duration, setTimer, isContinue,
				  force) > -1);
		}
		else
		{
			BG_PlayAnim(ps, animModelInfo, scriptCommand->animIndex[0], scriptCommand->bodyPart[0], duration, setTimer,
						isContinue, force);
		}
	}
	if(scriptCommand->bodyPart[1])
	{
		duration = scriptCommand->animDuration[0] + 50;
		// FIXME: how to sync torso/legs anims accounting for transition blends, etc
		// just play the animation for the torso
		if(scriptCommand->bodyPart[1] == ANIM_BP_BOTH || scriptCommand->bodyPart[1] == ANIM_BP_LEGS)
		{
			playedLegsAnim =
				(BG_PlayAnim
				 (ps, animModelInfo, scriptCommand->animIndex[1], scriptCommand->bodyPart[1], duration, setTimer, isContinue,
				  force) > -1);
		}
		else
		{
			BG_PlayAnim(ps, animModelInfo, scriptCommand->animIndex[1], scriptCommand->bodyPart[1], duration, setTimer,
						isContinue, force);
		}
	}

	if(scriptCommand->soundIndex)
	{
		globalScriptData->playSound(scriptCommand->soundIndex, ps->origin, ps->clientNum);
	}

	if(!playedLegsAnim)
	{
		return -1;
	}

	return duration;
}

/*
================
BG_AnimScriptAnimation

  runs the normal locomotive animations

  returns 1 if an animation was set, -1 if no animation was found, 0 otherwise
================
*/
int BG_AnimScriptAnimation(playerState_t * ps, animModelInfo_t * animModelInfo, scriptAnimMoveTypes_t movetype,
						   qboolean isContinue)
{
	animScript_t   *script = NULL;
	animScriptItem_t *scriptItem = NULL;
	animScriptCommand_t *scriptCommand = NULL;
	int             state = ps->aiState;

	// Allow fallen movetype while dead
	if(ps->eFlags & EF_DEAD && movetype != ANIM_MT_FALLEN && movetype != ANIM_MT_FLAILING)
	{
		return -1;
	}

#ifdef DBGANIMS
	Com_Printf("script anim: cl %i, mt %s, ", ps->clientNum, animMoveTypesStr[movetype]);
#endif

	// xkan, 1/10/2003 - adapted from original SP source
	// try finding a match in all states ABOVE the given state
	while(!scriptItem && state < MAX_AISTATES)
	{
		script = &animModelInfo->scriptAnims[state][movetype];
		if(!script->numItems)
		{
			state++;
			continue;
		}
		// find the first script item, that passes all the conditions for this event
		scriptItem = BG_FirstValidItem(ps->clientNum, script);
		if(!scriptItem)
		{
			state++;
			continue;
		}
	}

	//
	if(!scriptItem)
	{
#ifdef DBGANIMS
		Com_Printf("no valid conditions\n");
#endif
		return -1;
	}
	// save this as our current movetype
	BG_UpdateConditionValue(ps->clientNum, ANIM_COND_MOVETYPE, movetype, qtrue);
	// pick the correct animation for this character (animations must be constant for each character, otherwise they'll constantly change)
	scriptCommand = &scriptItem->commands[ps->clientNum % scriptItem->numCommands];

#ifdef DBGANIMS
	if(scriptCommand->bodyPart[0])
	{
		Com_Printf("anim0 (%s): %s", animBodyPartsStr[scriptCommand->bodyPart[0]].string,
				   animModelInfo->animations[scriptCommand->animIndex[0]]->name);
	}
	if(scriptCommand->bodyPart[1])
	{
		Com_Printf("anim1 (%s): %s", animBodyPartsStr[scriptCommand->bodyPart[1]].string,
				   animModelInfo->animations[scriptCommand->animIndex[1]]->name);
	}
	Com_Printf("\n");
#endif

	// run it
	return (BG_ExecuteCommand(ps, animModelInfo, scriptCommand, qfalse, isContinue, qfalse) != -1);
}

/*
================
BG_AnimScriptCannedAnimation

  uses the current movetype for this client to play a canned animation

  returns the duration in milliseconds that this model should be paused. -1 if no anim found
================
*/
int BG_AnimScriptCannedAnimation(playerState_t * ps, animModelInfo_t * animModelInfo)
{
	animScript_t   *script;
	animScriptItem_t *scriptItem;
	animScriptCommand_t *scriptCommand;
	scriptAnimMoveTypes_t movetype;

	if(ps->eFlags & EF_DEAD)
	{
		return -1;
	}

	movetype = globalScriptData->clientConditions[ps->clientNum][ANIM_COND_MOVETYPE][0];
	if(!movetype)
	{							// no valid movetype yet for this client
		return -1;
	}
	//
	script = &animModelInfo->scriptCannedAnims[movetype];
	if(!script->numItems)
	{
		return -1;
	}
	// find the first script item, that passes all the conditions for this event
	scriptItem = BG_FirstValidItem(ps->clientNum, script);
	if(!scriptItem)
	{
		return -1;
	}
	// pick a random command
	scriptCommand = &scriptItem->commands[rand() % scriptItem->numCommands];
	// run it
	return BG_ExecuteCommand(ps, animModelInfo, scriptCommand, qtrue, qfalse, qfalse);
}

/*
================
BG_AnimScriptEvent

  returns the duration in milliseconds that this model should be paused. -1 if no event found
================
*/
int BG_AnimScriptEvent(playerState_t * ps, animModelInfo_t * animModelInfo, scriptAnimEventTypes_t event, qboolean isContinue,
					   qboolean force)
{
	animScript_t   *script;
	animScriptItem_t *scriptItem;
	animScriptCommand_t *scriptCommand;

	if(event != ANIM_ET_DEATH && ps->eFlags & EF_DEAD)
	{
		return -1;
	}

#ifdef DBGANIMEVENTS
	Com_Printf("script event: cl %i, ev %s, ", ps->clientNum, animEventTypesStr[event]);
#endif

	script = &animModelInfo->scriptEvents[event];
	if(!script->numItems)
	{
#ifdef DBGANIMEVENTS
		Com_Printf("no entry\n");
#endif
		return -1;
	}
	// find the first script item, that passes all the conditions for this event
	scriptItem = BG_FirstValidItem(ps->clientNum, script);
	if(!scriptItem)
	{
#ifdef DBGANIMEVENTS
		Com_Printf("no valid conditions\n");
#endif
		return -1;
	}
	// pick a random command
	scriptCommand = &scriptItem->commands[rand() % scriptItem->numCommands];

#ifdef DBGANIMEVENTS
	if(scriptCommand->bodyPart[0])
	{
		Com_Printf("anim0 (%s): %s", animBodyPartsStr[scriptCommand->bodyPart[0]].string,
				   animModelInfo->animations[scriptCommand->animIndex[0]]->name);
	}
	if(scriptCommand->bodyPart[1])
	{
		Com_Printf("anim1 (%s): %s", animBodyPartsStr[scriptCommand->bodyPart[1]].string,
				   animModelInfo->animations[scriptCommand->animIndex[1]]->name);
	}
	Com_Printf("\n");
#endif

	// run it
	return BG_ExecuteCommand(ps, animModelInfo, scriptCommand, qtrue, isContinue, force);
}

/*
===============
BG_ValidAnimScript

  returns qtrue if the given client has animation scripts
===============
*/
/*qboolean BG_ValidAnimScript( int clientNum )
{
	if (!globalScriptData->clientModels[clientNum])
		return qfalse;
	//
	if (!globalScriptData->modelInfo[ globalScriptData->clientModels[clientNum] ].numScriptItems)
		return qfalse;
	//
	return qtrue;
}*/

/*
===============
BG_GetAnimString
===============
*/
char           *BG_GetAnimString(animModelInfo_t * animModelInfo, int anim)
{
	if(anim >= animModelInfo->numAnimations)
	{
		BG_AnimParseError("BG_GetAnimString: anim index is out of range");
	}
	return animModelInfo->animations[anim]->name;
}

/*
==============
BG_UpdateConditionValue
==============
*/
void BG_UpdateConditionValue(int client, int condition, int value, qboolean checkConversion)
{
	// rain - fixed checkConversion brained-damagedness, which would try
	// to BitSet an insane value if checkConversion was false but this
	// anim was ANIM_CONDTYPE_BITFLAGS
	if(checkConversion == qtrue)
	{
		if(animConditionsTable[condition].type == ANIM_CONDTYPE_BITFLAGS)
		{

			// we may need to convert to bitflags
			// DHM - Nerve :: We want to set the ScriptData to the explicit value passed in.
			//              COM_BitSet will OR values on top of each other, so clear it first.
			globalScriptData->clientConditions[client][condition][0] = 0;
			globalScriptData->clientConditions[client][condition][1] = 0;
			// dhm - end

			COM_BitSet(globalScriptData->clientConditions[client][condition], value);
			return;
		}
		// rain - we must fall through here because a bunch of non-bitflag
		// conditions are set with checkConversion == qtrue
	}
	globalScriptData->clientConditions[client][condition][0] = value;
}

/*
==============
BG_GetConditionValue
==============
*/
int BG_GetConditionValue(int client, int condition, qboolean checkConversion)
{
	int             i;

	if(animConditionsTable[condition].type == ANIM_CONDTYPE_BITFLAGS)
	{
		if(checkConversion)
		{
			// we may need to convert to a value
			//if (!value)
			//  return 0;
			for(i = 0; i < 8 * sizeof(globalScriptData->clientConditions[0][0]); i++)
			{
				if(COM_BitCheck(globalScriptData->clientConditions[client][condition], i))
				{
					return i;
				}
			}
			// nothing found
			return 0;
		}
		else {
			Com_Error( ERR_DROP, "FIXME");
			// xkan, 1/14/2003 - must use COM_BitCheck on the result.
			return (int)globalScriptData->clientConditions[client][condition][0];
		}
		//BG_AnimParseError( "BG_GetConditionValue: internal error" );
	}
	else
	{
		return globalScriptData->clientConditions[client][condition][0];
	}
}

/*====================
BG_GetConditionBitFlag: returns whether the specified bit flag is set
====================*/
qboolean BG_GetConditionBitFlag(int client, int condition, int bitNumber)
{
	if(animConditionsTable[condition].type == ANIM_CONDTYPE_BITFLAGS)
	{
		return (COM_BitCheck(globalScriptData->clientConditions[client][condition], bitNumber));
	}
	else
	{
		Com_Error(ERR_DROP, "BG_GetConditionBitFlag: animation condition %i is not a bitflag condition",
				  animConditionsTable[condition].type);
	}
	return qfalse;
}

void BG_SetConditionBitFlag(int client, int condition, int bitNumber)
{
	COM_BitSet(globalScriptData->clientConditions[client][condition], bitNumber);
}

void BG_ClearConditionBitFlag(int client, int condition, int bitNumber)
{
	COM_BitClear(globalScriptData->clientConditions[client][condition], bitNumber);
}

/*
================
BG_GetAnimScriptAnimation

  returns the locomotion animation index, -1 if no animation was found, 0 otherwise
================
*/
int BG_GetAnimScriptAnimation(int client, animModelInfo_t * animModelInfo, aistateEnum_t aistate, scriptAnimMoveTypes_t movetype)
{
	animScript_t   *script;
	animScriptItem_t *scriptItem = NULL;
	animScriptCommand_t *scriptCommand;
	int             state = aistate;

	// xkan, 1/10/2003 - adapted from the original SP source code
	// try finding a match in all states ABOVE the given state
	while(!scriptItem && state < MAX_AISTATES)
	{
		script = &animModelInfo->scriptAnims[state][movetype];
		if(!script->numItems)
		{
			state++;
			continue;
		}
		// find the first script item, that passes all the conditions for this event
		scriptItem = BG_FirstValidItem(client, script);
		if(!scriptItem)
		{
			state++;
			continue;
		}
	}
	//
	if(!scriptItem)
	{
		return -1;
	}
	// pick the correct animation for this character (animations must be constant for each character, otherwise they'll constantly change)
	scriptCommand = &scriptItem->commands[client % scriptItem->numCommands];
	if(!scriptCommand->bodyPart[0])
	{
		return -1;
	}
	// return the animation
	return scriptCommand->animIndex[0];
}

/*
================
BG_GetAnimScriptEvent

  returns the animation index for this event
================
*/
int BG_GetAnimScriptEvent(playerState_t * ps, scriptAnimEventTypes_t event)
{
	animModelInfo_t *animModelInfo;
	animScript_t   *script;
	animScriptItem_t *scriptItem;
	animScriptCommand_t *scriptCommand;

	if(event != ANIM_ET_DEATH && ps->eFlags & EF_DEAD)
	{
		return -1;
	}

	animModelInfo = BG_GetCharacterForPlayerstate(ps)->animModelInfo;
	script = &animModelInfo->scriptEvents[event];
	if(!script->numItems)
	{
		return -1;
	}
	// find the first script item, that passes all the conditions for this event
	scriptItem = BG_FirstValidItem(ps->clientNum, script);
	if(!scriptItem)
	{
		return -1;
	}
	// pick a random command
	scriptCommand = &scriptItem->commands[rand() % scriptItem->numCommands];

	// return the animation
	return scriptCommand->animIndex[0];
}

/*
===============
BG_GetAnimationForIndex

  returns the animation_t for the given index
===============
*/
animation_t    *BG_GetAnimationForIndex(animModelInfo_t * animModelInfo, int index)
{
	if(index < 0 || index >= animModelInfo->numAnimations)
	{
		Com_Error(ERR_DROP, "BG_GetAnimationForIndex: index out of bounds");
	}

	return animModelInfo->animations[index];
}

/*
=================
BG_AnimUpdatePlayerStateConditions
=================
*/
void BG_AnimUpdatePlayerStateConditions(pmove_t * pmove)
{
	playerState_t  *ps = pmove->ps;

	// WEAPON
	if(ps->eFlags & EF_ZOOMING)
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_WEAPON, WP_BINOCULARS, qtrue);
		BG_SetConditionBitFlag(ps->clientNum, ANIM_COND_GEN_BITFLAG, ANIM_BITFLAG_ZOOMING);
	}
	else
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_WEAPON, ps->weapon, qtrue);
		BG_ClearConditionBitFlag(ps->clientNum, ANIM_COND_GEN_BITFLAG, ANIM_BITFLAG_ZOOMING);
	}

	// MOUNTED
	if(ps->eFlags & EF_MG42_ACTIVE || ps->eFlags & EF_MOUNTEDTANK)
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_MOUNTED, MOUNTED_MG42, qtrue);
	}
	else if(ps->eFlags & EF_AAGUN_ACTIVE)
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_MOUNTED, MOUNTED_AAGUN, qtrue);
	}
	else
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_MOUNTED, MOUNTED_UNUSED, qtrue);
	}

	// UNDERHAND
	BG_UpdateConditionValue(ps->clientNum, ANIM_COND_UNDERHAND, ps->viewangles[0] > 0, qtrue);

	if(ps->viewheight == ps->crouchViewHeight)
	{
		ps->eFlags |= EF_CROUCHING;
	}
	else
	{
		ps->eFlags &= ~EF_CROUCHING;
	}

	if(pmove->cmd.buttons & BUTTON_ATTACK)
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_FIRING, qtrue, qtrue);
	}
	else
	{
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_FIRING, qfalse, qtrue);
	}

	if(ps->pm_flags & PMF_FLAILING)
	{
		if(ps->groundEntityNum == ENTITYNUM_NONE)
		{
			BG_UpdateConditionValue(ps->clientNum, ANIM_COND_FLAILING_TYPE, FLAILING_INAIR, qtrue);
			ps->pm_time = 750;
		}
		else
		{
			if(globalScriptData->clientConditions[ps->clientNum][ANIM_COND_FLAILING_TYPE][0] != FLAILING_VCRASH)
			{
				BG_UpdateConditionValue(ps->clientNum, ANIM_COND_FLAILING_TYPE, FLAILING_VCRASH, qtrue);
				ps->pm_time = 750;
			}
		}
	}
}
