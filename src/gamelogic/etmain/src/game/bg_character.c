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
**  Character definition code
*/

#include "../../../../engine/qcommon/q_shared.h"
#include "bg_public.h"

bg_character_t  alliedClassCharacters[NUM_PLAYER_CLASSES];
bg_character_t  axisClassCharacters[NUM_PLAYER_CLASSES];

static qboolean BG_PCF_ParseError(int handle, char *format, ...)
{
	int             line;
	char            filename[128];
	va_list         argptr;
	static char     string[4096];

	va_start(argptr, format);
	Q_vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	filename[0] = '\0';
	line = 0;
	trap_PC_SourceFileAndLine(handle, filename, &line);

	Com_Printf(S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string);

	trap_PC_FreeSource(handle);

	return qfalse;
}

qboolean BG_ParseCharacterFile(const char *filename, bg_characterDef_t * characterDef)
{
	pc_token_t      token;
	int             handle;

	handle = trap_PC_LoadSource(filename);

	if(!handle)
	{
		return qfalse;
	}

	if(!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "characterDef"))
	{
		return BG_PCF_ParseError(handle, "expected 'characterDef'");
	}

	if(!trap_PC_ReadToken(handle, &token) || Q_stricmp(token.string, "{"))
	{
		return BG_PCF_ParseError(handle, "expected '{'");
	}

	while(1)
	{
		if(!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if(token.string[0] == '}')
		{
			break;
		}

		if(!Q_stricmp(token.string, "mesh"))
		{
			if(!PC_String_ParseNoAlloc(handle, characterDef->mesh, sizeof(characterDef->mesh)))
			{
				return BG_PCF_ParseError(handle, "expected mesh filename");
			}
		}
		else if(!Q_stricmp(token.string, "animationGroup"))
		{
			if(!PC_String_ParseNoAlloc(handle, characterDef->animationGroup, sizeof(characterDef->animationGroup)))
			{
				return BG_PCF_ParseError(handle, "expected animationGroup filename");
			}
		}
		else if(!Q_stricmp(token.string, "animationScript"))
		{
			if(!PC_String_ParseNoAlloc(handle, characterDef->animationScript, sizeof(characterDef->animationScript)))
			{
				return BG_PCF_ParseError(handle, "expected animationScript filename");
			}
		}
		else if(!Q_stricmp(token.string, "skin"))
		{
			if(!PC_String_ParseNoAlloc(handle, characterDef->skin, sizeof(characterDef->skin)))
			{
				return BG_PCF_ParseError(handle, "expected skin filename");
			}
		}
		else if(!Q_stricmp(token.string, "undressedCorpseModel"))
		{
			if(!PC_String_ParseNoAlloc(handle, characterDef->undressedCorpseModel, sizeof(characterDef->undressedCorpseModel)))
			{
				return BG_PCF_ParseError(handle, "expected undressedCorpseModel filename");
			}
		}
		else if(!Q_stricmp(token.string, "undressedCorpseSkin"))
		{
			if(!PC_String_ParseNoAlloc(handle, characterDef->undressedCorpseSkin, sizeof(characterDef->undressedCorpseSkin)))
			{
				return BG_PCF_ParseError(handle, "expected undressedCorpseSkin filename");
			}
		}
		else if(!Q_stricmp(token.string, "hudhead"))
		{
			if(!PC_String_ParseNoAlloc(handle, characterDef->hudhead, sizeof(characterDef->hudhead)))
			{
				return BG_PCF_ParseError(handle, "expected hudhead filename");
			}
		}
		else if(!Q_stricmp(token.string, "hudheadskin"))
		{
			if(!PC_String_ParseNoAlloc(handle, characterDef->hudheadskin, sizeof(characterDef->hudheadskin)))
			{
				return BG_PCF_ParseError(handle, "expected hudhead filename");
			}
		}
		else if(!Q_stricmp(token.string, "hudheadanims"))
		{
			if(!PC_String_ParseNoAlloc(handle, characterDef->hudheadanims, sizeof(characterDef->hudheadanims)))
			{
				return BG_PCF_ParseError(handle, "expected hudheadanims filename");
			}
		}
		else
		{
			return BG_PCF_ParseError(handle, "unknown token '%s'", token.string);
		}
	}

	trap_PC_FreeSource(handle);

	return qtrue;
}

bg_character_t *BG_GetCharacter(int team, int cls)
{
	switch (team)
	{
		default:
		case TEAM_AXIS:
			return &axisClassCharacters[cls];
		case TEAM_ALLIES:
			return &alliedClassCharacters[cls];
	}
}

bg_character_t *BG_GetCharacterForPlayerstate(playerState_t * ps)
{
	// FIXME: add disguise?
	switch (ps->persistant[PERS_TEAM])
	{
		default:
		case TEAM_AXIS:
			return &axisClassCharacters[ps->stats[STAT_PLAYER_CLASS]];
		case TEAM_ALLIES:
			return &alliedClassCharacters[ps->stats[STAT_PLAYER_CLASS]];
	}
}

//
// Character Pool - used for custom characters
//

bg_character_t  bg_characterPool[MAX_CHARACTERS];
qboolean        bg_characterPoolInuse[MAX_CHARACTERS];

void BG_ClearCharacterPool(void)
{
	memset(&bg_characterPool, 0, sizeof(bg_characterPool));
	memset(&bg_characterPoolInuse, 0, sizeof(bg_characterPoolInuse));
}

bg_character_t *BG_FindFreeCharacter(const char *characterFile)
{
	int             i;

	// see if we already got it
	for(i = 0; i < MAX_CHARACTERS; i++)
	{
		if(!bg_characterPoolInuse[i])
		{
			continue;
		}

		if(!Q_stricmp(characterFile, bg_characterPool[i].characterFile))
		{
			return &bg_characterPool[i];
		}
	}

	// else get a free one
	for(i = 0; i < MAX_CHARACTERS; i++)
	{
		if(!bg_characterPoolInuse[i])
		{
			bg_characterPoolInuse[i] = qtrue;
			Q_strncpyz(bg_characterPool[i].characterFile, characterFile, sizeof(bg_characterPool[i].characterFile));
			return &bg_characterPool[i];
		}
	}

	// should never get here
	return NULL;
}

bg_character_t *BG_FindCharacter(const char *characterFile)
{
	int             i;

	// see if we already got it
	for(i = 0; i < MAX_CHARACTERS; i++)
	{
		if(!bg_characterPoolInuse[i])
		{
			continue;
		}

		if(!Q_stricmp(characterFile, bg_characterPool[i].characterFile))
		{
			return &bg_characterPool[i];
		}
	}

	// nope didn't find it
	return NULL;
}
