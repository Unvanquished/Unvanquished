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

/*
**  Character loading
*/

#include "cg_local.h"

char            bigTextBuffer[100000];

// TTimo - defined but not used
#if 0
/*
======================
CG_ParseGibModels

Read a configuration file containing gib models for use with this character
======================
*/
static qboolean CG_ParseGibModels(bg_playerclass_t * classInfo)
{
	char           *text_p;
	int             len;
	int             i;
	char           *token;
	fileHandle_t    f;

	memset(classInfo->gibModels, 0, sizeof(classInfo->gibModels));

	// load the file
	len = trap_FS_FOpenFile(va("models/players/%s/gibs.cfg", classInfo->modelPath), &f, FS_READ);
	if(len <= 0)
	{
		return qfalse;
	}
	if(len >= sizeof(bigTextBuffer) - 1)
	{
		CG_Printf("File %s too long\n", va("models/players/%s/gibs.cfg", classInfo->modelPath));
		return qfalse;
	}
	trap_FS_Read(bigTextBuffer, len, f);
	bigTextBuffer[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = bigTextBuffer;

	for(i = 0; i < MAX_GIB_MODELS; i++)
	{
		token = COM_Parse(&text_p);
		if(!token)
		{
			break;
		}
		// cache this model
		classInfo->gibModels[i] = trap_R_RegisterModel(token);
	}

	return qtrue;
}
#endif

/*
======================
CG_ParseHudHeadConfig
======================
*/
static qboolean CG_ParseHudHeadConfig(const char *filename, animation_t * hha)
{
	char           *text_p;
	int             len;
	int             i;
	float           fps;
	char           *token;
	fileHandle_t    f;

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(len <= 0)
	{
		return qfalse;
	}

	if(len >= sizeof(bigTextBuffer) - 1)
	{
		CG_Printf("File %s too long\n", filename);
		return qfalse;
	}

	trap_FS_Read(bigTextBuffer, len, f);
	bigTextBuffer[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = bigTextBuffer;

	for(i = 0; i < MAX_HD_ANIMATIONS; i++)
	{
		token = COM_Parse(&text_p);	// first frame
		if(!token)
		{
			break;
		}
		hha[i].firstFrame = atoi(token);

		token = COM_Parse(&text_p);	// length
		if(!token)
		{
			break;
		}
		hha[i].numFrames = atoi(token);

		token = COM_Parse(&text_p);	// fps
		if(!token)
		{
			break;
		}
		fps = atof(token);
		if(fps == 0)
		{
			fps = 1;
		}

		hha[i].frameLerp = 1000 / fps;
		hha[i].initialLerp = 1000 / fps;

		token = COM_Parse(&text_p);	// looping frames
		if(!token)
		{
			break;
		}
		hha[i].loopFrames = atoi(token);

		if(hha[i].loopFrames > hha[i].numFrames)
		{
			hha[i].loopFrames = hha[i].numFrames;
		}
		else if(hha[i].loopFrames < 0)
		{
			hha[i].loopFrames = 0;
		}
	}

	if(i != MAX_HD_ANIMATIONS)
	{
		CG_Printf("Error parsing hud head animation file: %s", filename);
		return qfalse;
	}

	return qtrue;
}

/*
==================
CG_CalcMoveSpeeds
==================
*/
static void CG_CalcMoveSpeeds(bg_character_t * character)
{
	char           *tags[2] = { "tag_footleft", "tag_footright" };
	vec3_t          oldPos[2];
	refEntity_t     refent;
	animation_t    *anim;
	int             i, j, k;
	float           totalSpeed;
	int             numSpeed;
	int             lastLow, low;
	orientation_t   o[2];

	memset(&refent, 0, sizeof(refent));

	refent.hModel = character->mesh;

	for(i = 0; i < character->animModelInfo->numAnimations; i++)
	{
		anim = character->animModelInfo->animations[i];

		if(anim->moveSpeed >= 0)
		{
			continue;
		}

		totalSpeed = 0;
		lastLow = -1;
		numSpeed = 0;

		// for each frame
		for(j = 0; j < anim->numFrames; j++)
		{

			refent.frame = anim->firstFrame + j;
			refent.oldframe = refent.frame;
			refent.torsoFrameModel = refent.oldTorsoFrameModel = refent.frameModel = refent.oldframeModel = anim->mdxFile;

			// for each foot
			for(k = 0; k < 2; k++)
			{
				if(trap_R_LerpTag(&o[k], &refent, tags[k], 0) < 0)
				{
					CG_Error("CG_CalcMoveSpeeds: unable to find tag %s, cannot calculate movespeed", tags[k]);
				}
			}

			// find the contact foot
			if(anim->flags & ANIMFL_LADDERANIM)
			{
				if(o[0].origin[0] > o[1].origin[0])
				{
					low = 0;
				}
				else
				{
					low = 1;
				}
				totalSpeed += fabs(oldPos[low][2] - o[low].origin[2]);
			}
			else
			{
				if(o[0].origin[2] < o[1].origin[2])
				{
					low = 0;
				}
				else
				{
					low = 1;
				}
				totalSpeed += fabs(oldPos[low][0] - o[low].origin[0]);
			}

			numSpeed++;

			// save the positions
			for(k = 0; k < 2; k++)
			{
				VectorCopy(o[k].origin, oldPos[k]);
			}
			lastLow = low;
		}

		// record the speed
		anim->moveSpeed = (int)((totalSpeed / numSpeed) * 1000.0 / anim->frameLerp);
	}
}

/*
======================
CG_ParseAnimationFiles

  Read in all the configuration and script files for this model.
======================
*/
static qboolean CG_ParseAnimationFiles(bg_character_t * character, const char *animationGroup, const char *animationScript)
{
	char            filename[MAX_QPATH];
	fileHandle_t    f;
	int             len;

	// set the name of the animationGroup and animationScript in the animModelInfo structure
	Q_strncpyz(character->animModelInfo->animationGroup, animationGroup, sizeof(character->animModelInfo->animationGroup));
	Q_strncpyz(character->animModelInfo->animationScript, animationScript, sizeof(character->animModelInfo->animationScript));

	BG_R_RegisterAnimationGroup(animationGroup, character->animModelInfo);

	// calc movespeed values if required
	CG_CalcMoveSpeeds(character);

	// load the script file
	len = trap_FS_FOpenFile(animationScript, &f, FS_READ);
	if(len <= 0)
	{
		return qfalse;
	}
	if(len >= sizeof(bigTextBuffer) - 1)
	{
		CG_Printf("File %s is too long\n", filename);
		return qfalse;
	}
	trap_FS_Read(bigTextBuffer, len, f);
	bigTextBuffer[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	BG_AnimParseAnimScript(character->animModelInfo, &cgs.animScriptData, animationScript, bigTextBuffer);

	return qtrue;
}

/*
==================
CG_CheckForExistingAnimModelInfo

  If this player model has already been parsed, then use the existing information.
  Otherwise, set the modelInfo pointer to the first free slot.

  returns qtrue if existing model found, qfalse otherwise
==================
*/
static qboolean CG_CheckForExistingAnimModelInfo(const char *animationGroup, const char *animationScript,
												 animModelInfo_t ** animModelInfo)
{
	int             i;
	animModelInfo_t *trav, *firstFree = NULL;

	for(i = 0, trav = cgs.animScriptData.modelInfo; i < MAX_ANIMSCRIPT_MODELS; i++, trav++)
	{
		if(*trav->animationGroup && *trav->animationScript)
		{
			if(!Q_stricmp(trav->animationGroup, animationGroup) && !Q_stricmp(trav->animationScript, animationScript))
			{
				// found a match, use this animModelInfo
				*animModelInfo = trav;
				return qtrue;
			}
		}
		else if(!firstFree)
		{
			firstFree = trav;
		}
	}

	if(!firstFree)
	{
		CG_Error("unable to find a free modelinfo slot, cannot continue\n");
	}
	else
	{
		*animModelInfo = firstFree;
		// clear the structure out ready for use
		memset(*animModelInfo, 0, sizeof(**animModelInfo));
	}

	// qfalse signifies that we need to parse the information from the script files
	return qfalse;
}

/*
==============
CG_RegisterAcc
==============
*/
static qboolean CG_RegisterAcc(const char *modelName, int *model, const char *skinname, qhandle_t * skin)
{
	char            filename[MAX_QPATH];

	*model = trap_R_RegisterModel(modelName);

	if(!*model)
	{
		return qfalse;
	}

	COM_StripExtension(modelName, filename);
	Q_strcat(filename, sizeof(filename), va("_%s.skin", skinname));
	*skin = trap_R_RegisterSkin(filename);

	return qtrue;
}

typedef struct
{
	char           *type;
	accType_t       index;
} acc_t;

static acc_t    cg_accessories[] = {
	{"md3_beltr", ACC_BELT_LEFT},
	{"md3_beltl", ACC_BELT_RIGHT},
	{"md3_belt", ACC_BELT},
	{"md3_back", ACC_BACK},
	{"md3_weapon", ACC_WEAPON},
	{"md3_weapon2", ACC_WEAPON2},
};

static int      cg_numAccessories = sizeof(cg_accessories) / sizeof(cg_accessories[0]);

static acc_t    cg_headAccessories[] = {
	{"md3_hat", ACC_HAT},
	{"md3_rank", ACC_RANK},
	{"md3_hat2", ACC_MOUTH2},
	{"md3_hat3", ACC_MOUTH3},
};

static int      cg_numHeadAccessories = sizeof(cg_headAccessories) / sizeof(cg_headAccessories[0]);

/*
====================
CG_RegisterCharacter
====================
*/
qboolean CG_RegisterCharacter(const char *characterFile, bg_character_t * character)
{
	bg_characterDef_t characterDef;
	char           *filename;
	char            buf[MAX_QPATH];
	char            accessoryname[MAX_QPATH];
	int             i;

	memset(&characterDef, 0, sizeof(characterDef));

	if(!BG_ParseCharacterFile(characterFile, &characterDef))
	{
		return qfalse;			// the parser will provide the error message
	}

	// Register Mesh
	if(!(character->mesh = trap_R_RegisterModel(characterDef.mesh)))
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: failed to register mesh '%s' referenced from '%s'\n", characterDef.mesh,
				  characterFile);
	}

	// Register Skin
	COM_StripExtension(characterDef.mesh, buf);
	filename = va("%s_%s.skin", buf, characterDef.skin);
	if(!(character->skin = trap_R_RegisterSkin(filename)))
	{
		CG_Printf(S_COLOR_YELLOW "WARNING: failed to register skin '%s' referenced from '%s'\n", filename, characterFile);
	}
	else
	{
		for(i = 0; i < cg_numAccessories; i++)
		{
			if(trap_R_GetSkinModel(character->skin, cg_accessories[i].type, accessoryname))
			{
				if(!CG_RegisterAcc
				   (accessoryname, &character->accModels[cg_accessories[i].index], characterDef.skin,
					&character->accSkins[cg_accessories[i].index]))
				{
					CG_Printf(S_COLOR_YELLOW "WARNING: failed to register accessory '%s' referenced from '%s'->'%s'\n",
							  accessoryname, characterFile, filename);
				}
			}
		}

		for(i = 0; i < cg_numHeadAccessories; i++)
		{
			if(trap_R_GetSkinModel(character->skin, cg_headAccessories[i].type, accessoryname))
			{
				if(!CG_RegisterAcc
				   (accessoryname, &character->accModels[cg_headAccessories[i].index], characterDef.skin,
					&character->accSkins[cg_headAccessories[i].index]))
				{
					CG_Printf(S_COLOR_YELLOW "WARNING: failed to register accessory '%s' referenced from '%s'->'%s'\n",
							  accessoryname, characterFile, filename);
				}
			}
		}
	}

	// Register Undressed Corpse Media
	if(*characterDef.undressedCorpseModel)
	{
		// Register Undressed Corpse Model
		if(!(character->undressedCorpseModel = trap_R_RegisterModel(characterDef.undressedCorpseModel)))
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: failed to register undressed corpse model '%s' referenced from '%s'\n",
					  characterDef.undressedCorpseModel, characterFile);
		}

		// Register Undressed Corpse Skin
		COM_StripExtension(characterDef.undressedCorpseModel, buf);
		filename = va("%s_%s.skin", buf, characterDef.undressedCorpseSkin);
		if(!(character->undressedCorpseSkin = trap_R_RegisterSkin(filename)))
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: failed to register undressed corpse skin '%s' referenced from '%s'\n", filename,
					  characterFile);
		}
	}

	// Register the head for the hud
	if(*characterDef.hudhead)
	{
		// Register Hud Head Model
		if(!(character->hudhead = trap_R_RegisterModel(characterDef.hudhead)))
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: failed to register hud head model '%s' referenced from '%s'\n",
					  characterDef.hudhead, characterFile);
		}

		if(*characterDef.hudheadskin && !(character->hudheadskin = trap_R_RegisterSkin(characterDef.hudheadskin)))
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: failed to register hud head skin '%s' referenced from '%s'\n",
					  characterDef.hudheadskin, characterFile);
		}

		if(*characterDef.hudheadanims)
		{
			if(!CG_ParseHudHeadConfig(characterDef.hudheadanims, character->hudheadanimations))
			{
				CG_Printf(S_COLOR_YELLOW "WARNING: failed to register hud head animations '%s' referenced from '%s'\n",
						  characterDef.hudheadanims, characterFile);
			}
		}
		else
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: no hud head animations supplied in '%s'\n", characterFile);
		}
	}

	// Parse Animation Files
	if(!CG_CheckForExistingAnimModelInfo(characterDef.animationGroup, characterDef.animationScript, &character->animModelInfo))
	{
		if(!CG_ParseAnimationFiles(character, characterDef.animationGroup, characterDef.animationScript))
		{
			CG_Printf(S_COLOR_YELLOW "WARNING: failed to load animation files referenced from '%s'\n", characterFile);
			return qfalse;
		}
	}

	// STILL MISSING: GIB MODELS (OPTIONAL?)

	return qtrue;
}

bg_character_t *CG_CharacterForClientinfo(clientInfo_t * ci, centity_t * cent)
{
	int             team, cls;

	if(cent && cent->currentState.eType == ET_CORPSE)
	{
		if(cent->currentState.onFireStart >= 0)
		{
			return cgs.gameCharacters[cent->currentState.onFireStart];
		}
		else
		{
			if(cent->currentState.modelindex < 4)
			{
				return BG_GetCharacter(cent->currentState.modelindex, cent->currentState.modelindex2);
			}
			else
			{
				return BG_GetCharacter(cent->currentState.modelindex - 4, cent->currentState.modelindex2);
			}
		}
	}

	if(cent && cent->currentState.powerups & (1 << PW_OPS_DISGUISED))
	{
		team = ci->team == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;

		cls = (cent->currentState.powerups >> PW_OPS_CLASS_1) & 7;

		return BG_GetCharacter(team, cls);
	}

	if(ci->character)
	{
		return ci->character;
	}

	return BG_GetCharacter(ci->team, ci->cls);
}

bg_character_t *CG_CharacterForPlayerstate(playerState_t * ps)
{
	int             team, cls;

	if(ps->powerups[PW_OPS_DISGUISED])
	{
		team = cgs.clientinfo[ps->clientNum].team == TEAM_AXIS ? TEAM_ALLIES : TEAM_AXIS;

		cls = 0;
		if(ps->powerups[PW_OPS_CLASS_1])
		{
			cls |= 1;
		}
		if(ps->powerups[PW_OPS_CLASS_2])
		{
			cls |= 2;
		}
		if(ps->powerups[PW_OPS_CLASS_3])
		{
			cls |= 4;
		}

		return BG_GetCharacter(team, cls);
	}

	return BG_GetCharacter(cgs.clientinfo[ps->clientNum].team, cgs.clientinfo[ps->clientNum].cls);
}

/*
========================
CG_RegisterPlayerClasses
========================
*/
void CG_RegisterPlayerClasses(void)
{
	bg_playerclass_t *classInfo;
	bg_character_t *character;
	int             team, cls;

	for(team = TEAM_AXIS; team <= TEAM_ALLIES; team++)
	{
		for(cls = PC_SOLDIER; cls < NUM_PLAYER_CLASSES; cls++)
		{
			classInfo = BG_GetPlayerClassInfo(team, cls);
			character = BG_GetCharacter(team, cls);

			Q_strncpyz(character->characterFile, classInfo->characterFile, sizeof(character->characterFile));

			if(!CG_RegisterCharacter(character->characterFile, character))
			{
				CG_Error("ERROR: CG_RegisterPlayerClasses: failed to load character file '%s' for the %s %s\n",
						 character->characterFile, (team == TEAM_AXIS ? "Axis" : "Allied"),
						 BG_ClassnameForNumber(classInfo->classNum));
			}

			if(!(classInfo->icon = trap_R_RegisterShaderNoMip(classInfo->iconName)))
			{
				CG_Printf(S_COLOR_YELLOW "WARNING: failed to load class icon '%s' for the %s %s\n", classInfo->iconName,
						  (team == TEAM_AXIS ? "Axis" : "Allied"), BG_ClassnameForNumber(classInfo->classNum));
			}

			if(!(classInfo->arrow = trap_R_RegisterShaderNoMip(classInfo->iconArrow)))
			{
				CG_Printf(S_COLOR_YELLOW "WARNING: failed to load icon arrow '%s' for the %s %s\n", classInfo->iconArrow,
						  (team == TEAM_AXIS ? "Axis" : "Allied"), BG_ClassnameForNumber(classInfo->classNum));
			}
		}
	}
}
