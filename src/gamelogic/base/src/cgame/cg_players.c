/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// cg_players.c -- handle the media and animation for player entities

#include "cg_local.h"

char *cg_customSoundNames[ MAX_CUSTOM_SOUNDS ] =
{
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25_1.wav",
	"*pain50_1.wav",
	"*pain75_1.wav",
	"*pain100_1.wav",
	"*falling1.wav",
	"*gasp.wav",
	"*drown.wav",
	"*fall1.wav",
	"*taunt.wav"
};

/*
================
CG_CustomSound

================
*/
sfxHandle_t CG_CustomSound ( int clientNum, const char *soundName )
{
	clientInfo_t *ci;
	int          i;

	if ( soundName[ 0 ] != '*' )
	{
		return trap_S_RegisterSound ( soundName, qfalse );
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS )
	{
		clientNum = 0;
	}

	ci = &cgs.clientinfo[ clientNum ];

	for ( i = 0; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[ i ]; i++ )
	{
		if ( !strcmp ( soundName, cg_customSoundNames[ i ] ) )
		{
			return ci->sounds[ i ];
		}
	}

	CG_Error ( "Unknown custom sound: %s", soundName );
	return 0;
}

/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation coutns and rates
models/players/visor/animation.cfg, etc
======================
*/
static qboolean CG_ParseAnimationFile ( const char *filename, clientInfo_t *ci )
{
	char         *text_p, *prev;
	int          len;
	int          i;
	char         *token;
	float        fps;
	int          skip;
	char         text[ 20000 ];
	fileHandle_t f;
	animation_t  *animations;

	animations = ci->animations;

	// load the file
	len = trap_FS_FOpenFile ( filename, &f, FS_READ );

	if ( len <= 0 )
	{
		return qfalse;
	}

	if ( len >= sizeof ( text ) - 1 )
	{
		CG_Printf ( "File %s too long\n", filename );
		trap_FS_FCloseFile ( f );
		return qfalse;
	}

	trap_FS_Read ( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile ( f );

	// parse the text
	text_p = text;
	skip = 0; // quite the compiler warning

	ci->footsteps = FOOTSTEP_NORMAL;
	VectorClear ( ci->headOffset );
	ci->gender = GENDER_MALE;
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;
	ci->nonsegmented = qfalse;

	// read optional parameters
	while ( 1 )
	{
		prev = text_p; // so we can unget
		token = COM_Parse ( &text_p );

		if ( !token )
		{
			break;
		}

		if ( !Q_stricmp ( token, "footsteps" ) )
		{
			token = COM_Parse ( &text_p );

			if ( !token )
			{
				break;
			}

			if ( !Q_stricmp ( token, "default" ) || !Q_stricmp ( token, "normal" ) )
			{
				ci->footsteps = FOOTSTEP_NORMAL;
			}
			else if ( !Q_stricmp ( token, "flesh" ) )
			{
				ci->footsteps = FOOTSTEP_FLESH;
			}
			else if ( !Q_stricmp ( token, "none" ) )
			{
				ci->footsteps = FOOTSTEP_NONE;
			}
			else if ( !Q_stricmp ( token, "custom" ) )
			{
				ci->footsteps = FOOTSTEP_CUSTOM;
			}
			else
			{
				CG_Printf ( "Bad footsteps parm in %s: %s\n", filename, token );
			}

			continue;
		}
		else if ( !Q_stricmp ( token, "headoffset" ) )
		{
			for ( i = 0; i < 3; i++ )
			{
				token = COM_Parse ( &text_p );

				if ( !token )
				{
					break;
				}

				ci->headOffset[ i ] = atof ( token );
			}

			continue;
		}
		else if ( !Q_stricmp ( token, "sex" ) )
		{
			token = COM_Parse ( &text_p );

			if ( !token )
			{
				break;
			}

			if ( token[ 0 ] == 'f' || token[ 0 ] == 'F' )
			{
				ci->gender = GENDER_FEMALE;
			}
			else if ( token[ 0 ] == 'n' || token[ 0 ] == 'N' )
			{
				ci->gender = GENDER_NEUTER;
			}
			else
			{
				ci->gender = GENDER_MALE;
			}

			continue;
		}
		else if ( !Q_stricmp ( token, "fixedlegs" ) )
		{
			ci->fixedlegs = qtrue;
			continue;
		}
		else if ( !Q_stricmp ( token, "fixedtorso" ) )
		{
			ci->fixedtorso = qtrue;
			continue;
		}
		else if ( !Q_stricmp ( token, "nonsegmented" ) )
		{
			ci->nonsegmented = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if ( token[ 0 ] >= '0' && token[ 0 ] <= '9' )
		{
			text_p = prev; // unget the token
			break;
		}

		Com_Printf ( "unknown token '%s' is %s\n", token, filename );
	}

	if ( !ci->nonsegmented )
	{
		// read information for each frame
		for ( i = 0; i < MAX_PLAYER_ANIMATIONS; i++ )
		{
			token = COM_Parse ( &text_p );

			if ( !*token )
			{
				if ( i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE )
				{
					animations[ i ].firstFrame = animations[ TORSO_GESTURE ].firstFrame;
					animations[ i ].frameLerp = animations[ TORSO_GESTURE ].frameLerp;
					animations[ i ].initialLerp = animations[ TORSO_GESTURE ].initialLerp;
					animations[ i ].loopFrames = animations[ TORSO_GESTURE ].loopFrames;
					animations[ i ].numFrames = animations[ TORSO_GESTURE ].numFrames;
					animations[ i ].reversed = qfalse;
					animations[ i ].flipflop = qfalse;
					continue;
				}

				break;
			}

			animations[ i ].firstFrame = atoi ( token );

			// leg only frames are adjusted to not count the upper body only frames
			if ( i == LEGS_WALKCR )
			{
				skip = animations[ LEGS_WALKCR ].firstFrame - animations[ TORSO_GESTURE ].firstFrame;
			}

			if ( i >= LEGS_WALKCR && i < TORSO_GETFLAG )
			{
				animations[ i ].firstFrame -= skip;
			}

			token = COM_Parse ( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].numFrames = atoi ( token );
			animations[ i ].reversed = qfalse;
			animations[ i ].flipflop = qfalse;

			// if numFrames is negative the animation is reversed
			if ( animations[ i ].numFrames < 0 )
			{
				animations[ i ].numFrames = -animations[ i ].numFrames;
				animations[ i ].reversed = qtrue;
			}

			token = COM_Parse ( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].loopFrames = atoi ( token );

			token = COM_Parse ( &text_p );

			if ( !*token )
			{
				break;
			}

			fps = atof ( token );

			if ( fps == 0 )
			{
				fps = 1;
			}

			animations[ i ].frameLerp = 1000 / fps;
			animations[ i ].initialLerp = 1000 / fps;
		}

		if ( i != MAX_PLAYER_ANIMATIONS )
		{
			CG_Printf ( "Error parsing animation file: %s", filename );
			return qfalse;
		}

		// crouch backward animation
		memcpy ( &animations[ LEGS_BACKCR ], &animations[ LEGS_WALKCR ], sizeof ( animation_t ) );
		animations[ LEGS_BACKCR ].reversed = qtrue;
		// walk backward animation
		memcpy ( &animations[ LEGS_BACKWALK ], &animations[ LEGS_WALK ], sizeof ( animation_t ) );
		animations[ LEGS_BACKWALK ].reversed = qtrue;
		// flag moving fast
		animations[ FLAG_RUN ].firstFrame = 0;
		animations[ FLAG_RUN ].numFrames = 16;
		animations[ FLAG_RUN ].loopFrames = 16;
		animations[ FLAG_RUN ].frameLerp = 1000 / 15;
		animations[ FLAG_RUN ].initialLerp = 1000 / 15;
		animations[ FLAG_RUN ].reversed = qfalse;
		// flag not moving or moving slowly
		animations[ FLAG_STAND ].firstFrame = 16;
		animations[ FLAG_STAND ].numFrames = 5;
		animations[ FLAG_STAND ].loopFrames = 0;
		animations[ FLAG_STAND ].frameLerp = 1000 / 20;
		animations[ FLAG_STAND ].initialLerp = 1000 / 20;
		animations[ FLAG_STAND ].reversed = qfalse;
		// flag speeding up
		animations[ FLAG_STAND2RUN ].firstFrame = 16;
		animations[ FLAG_STAND2RUN ].numFrames = 5;
		animations[ FLAG_STAND2RUN ].loopFrames = 1;
		animations[ FLAG_STAND2RUN ].frameLerp = 1000 / 15;
		animations[ FLAG_STAND2RUN ].initialLerp = 1000 / 15;
		animations[ FLAG_STAND2RUN ].reversed = qtrue;
	}
	else
	{
		// read information for each frame
		for ( i = 0; i < MAX_NONSEG_PLAYER_ANIMATIONS; i++ )
		{
			token = COM_Parse ( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].firstFrame = atoi ( token );

			token = COM_Parse ( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].numFrames = atoi ( token );
			animations[ i ].reversed = qfalse;
			animations[ i ].flipflop = qfalse;

			// if numFrames is negative the animation is reversed
			if ( animations[ i ].numFrames < 0 )
			{
				animations[ i ].numFrames = -animations[ i ].numFrames;
				animations[ i ].reversed = qtrue;
			}

			token = COM_Parse ( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].loopFrames = atoi ( token );

			token = COM_Parse ( &text_p );

			if ( !*token )
			{
				break;
			}

			fps = atof ( token );

			if ( fps == 0 )
			{
				fps = 1;
			}

			animations[ i ].frameLerp = 1000 / fps;
			animations[ i ].initialLerp = 1000 / fps;
		}

		if ( i != MAX_NONSEG_PLAYER_ANIMATIONS )
		{
			CG_Printf ( "Error parsing animation file: %s", filename );
			return qfalse;
		}

		// walk backward animation
		memcpy ( &animations[ NSPA_WALKBACK ], &animations[ NSPA_WALK ], sizeof ( animation_t ) );
		animations[ NSPA_WALKBACK ].reversed = qtrue;
	}

	return qtrue;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static qboolean CG_RegisterClientSkin ( clientInfo_t *ci, const char *modelName, const char *skinName )
{
	char filename[ MAX_QPATH ];

	if ( !ci->nonsegmented )
	{
		Com_sprintf ( filename, sizeof ( filename ), "models/players/%s/lower_%s.skin", modelName, skinName );
		ci->legsSkin = trap_R_RegisterSkin ( filename );

		if ( !ci->legsSkin )
		{
			Com_Printf ( "Leg skin load failure: %s\n", filename );
		}

		Com_sprintf ( filename, sizeof ( filename ), "models/players/%s/upper_%s.skin", modelName, skinName );
		ci->torsoSkin = trap_R_RegisterSkin ( filename );

		if ( !ci->torsoSkin )
		{
			Com_Printf ( "Torso skin load failure: %s\n", filename );
		}

		Com_sprintf ( filename, sizeof ( filename ), "models/players/%s/head_%s.skin", modelName, skinName );
		ci->headSkin = trap_R_RegisterSkin ( filename );

		if ( !ci->headSkin )
		{
			Com_Printf ( "Head skin load failure: %s\n", filename );
		}

		if ( !ci->legsSkin || !ci->torsoSkin || !ci->headSkin )
		{
			return qfalse;
		}
	}
	else
	{
		Com_sprintf ( filename, sizeof ( filename ), "models/players/%s/nonseg_%s.skin", modelName, skinName );
		ci->nonSegSkin = trap_R_RegisterSkin ( filename );

		if ( !ci->nonSegSkin )
		{
			Com_Printf ( "Non-segmented skin load failure: %s\n", filename );
		}

		if ( !ci->nonSegSkin )
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static qboolean CG_RegisterClientModelname ( clientInfo_t *ci, const char *modelName, const char *skinName )
{
	char filename[ MAX_QPATH * 2 ];

	//TA: do this first so the nonsegmented property is set
	// load the animations
	Com_sprintf ( filename, sizeof ( filename ), "models/players/%s/animation.cfg", modelName );

	if ( !CG_ParseAnimationFile ( filename, ci ) )
	{
		Com_Printf ( "Failed to load animation file %s\n", filename );
		return qfalse;
	}

	// load cmodels before models so filecache works

	if ( !ci->nonsegmented )
	{
		Com_sprintf ( filename, sizeof ( filename ), "models/players/%s/lower.md3", modelName );
		ci->legsModel = trap_R_RegisterModel ( filename );

		if ( !ci->legsModel )
		{
			Com_Printf ( "Failed to load model file %s\n", filename );
			return qfalse;
		}

		Com_sprintf ( filename, sizeof ( filename ), "models/players/%s/upper.md3", modelName );
		ci->torsoModel = trap_R_RegisterModel ( filename );

		if ( !ci->torsoModel )
		{
			Com_Printf ( "Failed to load model file %s\n", filename );
			return qfalse;
		}

		Com_sprintf ( filename, sizeof ( filename ), "models/players/%s/head.md3", modelName );
		ci->headModel = trap_R_RegisterModel ( filename );

		if ( !ci->headModel )
		{
			Com_Printf ( "Failed to load model file %s\n", filename );
			return qfalse;
		}
	}
	else
	{
		Com_sprintf ( filename, sizeof ( filename ), "models/players/%s/nonseg.md3", modelName );
		ci->nonSegModel = trap_R_RegisterModel ( filename );

		if ( !ci->nonSegModel )
		{
			Com_Printf ( "Failed to load model file %s\n", filename );
			return qfalse;
		}
	}

	// if any skins failed to load, return failure
	if ( !CG_RegisterClientSkin ( ci, modelName, skinName ) )
	{
		Com_Printf ( "Failed to load skin file: %s : %s\n", modelName, skinName );
		return qfalse;
	}

	//FIXME: skins do not load without icon present. do we want icons anyway?

	/*  Com_sprintf( filename, sizeof( filename ), "models/players/%s/icon_%s.tga", modelName, skinName );
	  ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	  if( !ci->modelIcon )
	  {
	    Com_Printf( "Failed to load icon file: %s\n", filename );
	    return qfalse;
	  }*/

	return qtrue;
}

/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString ( const char *v, vec3_t color )
{
	int val;

	VectorClear ( color );

	val = atoi ( v );

	if ( val < 1 || val > 7 )
	{
		VectorSet ( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 )
	{
		color[ 2 ] = 1.0f;
	}

	if ( val & 2 )
	{
		color[ 1 ] = 1.0f;
	}

	if ( val & 4 )
	{
		color[ 0 ] = 1.0f;
	}
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits
===================
*/
static void CG_LoadClientInfo ( clientInfo_t *ci )
{
	const char *dir, *fallback;
	int        i;
	const char *s;
	int        clientNum;

	if ( !CG_RegisterClientModelname ( ci, ci->modelName, ci->skinName ) )
	{
		if ( cg_buildScript.integer )
		{
			CG_Error ( "CG_RegisterClientModelname( %s, %s ) failed", ci->modelName, ci->skinName );
		}

		// fall back
		if ( !CG_RegisterClientModelname ( ci, DEFAULT_MODEL, "default" ) )
		{
			CG_Error ( "DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL );
		}
	}

	// sounds
	dir = ci->modelName;
	fallback = DEFAULT_MODEL;

	for ( i = 0; i < MAX_CUSTOM_SOUNDS; i++ )
	{
		s = cg_customSoundNames[ i ];

		if ( !s )
		{
			break;
		}

		// fanny about a bit with sounds that are missing
		if ( !CG_FileExists ( va ( "sound/player/%s/%s", dir, s + 1 ) ) )
		{
			//file doesn't exist

			if ( i == 11 || i == 8 ) //fall or falling
			{
				ci->sounds[ i ] = trap_S_RegisterSound ( "sound/null.wav", qfalse );
			}
			else
			{
				if ( i == 9 ) //gasp
				{
					s = cg_customSoundNames[ 7 ]; //pain100_1
				}
				else if ( i == 10 ) //drown
				{
					s = cg_customSoundNames[ 0 ]; //death1
				}

				ci->sounds[ i ] = trap_S_RegisterSound ( va ( "sound/player/%s/%s", dir, s + 1 ), qfalse );

				if ( !ci->sounds[ i ] )
				{
					ci->sounds[ i ] = trap_S_RegisterSound ( va ( "sound/player/%s/%s", fallback, s + 1 ), qfalse );
				}
			}
		}
		else
		{
			ci->sounds[ i ] = trap_S_RegisterSound ( va ( "sound/player/%s/%s", dir, s + 1 ), qfalse );

			if ( !ci->sounds[ i ] )
			{
				ci->sounds[ i ] = trap_S_RegisterSound ( va ( "sound/player/%s/%s", fallback, s + 1 ), qfalse );
			}
		}
	}

	if ( ci->footsteps == FOOTSTEP_CUSTOM )
	{
		for ( i = 0; i < 4; i++ )
		{
			ci->customFootsteps[ i ] = trap_S_RegisterSound ( va ( "sound/player/%s/step%d.wav", dir, i + 1 ), qfalse );

			if ( !ci->customFootsteps[ i ] )
			{
				ci->customFootsteps[ i ] = trap_S_RegisterSound ( va ( "sound/player/footsteps/step%d.wav", i + 1 ), qfalse );
			}

			ci->customMetalFootsteps[ i ] = trap_S_RegisterSound ( va ( "sound/player/%s/clank%d.wav", dir, i + 1 ), qfalse );

			if ( !ci->customMetalFootsteps[ i ] )
			{
				ci->customMetalFootsteps[ i ] = trap_S_RegisterSound ( va ( "sound/player/footsteps/clank%d.wav", i + 1 ), qfalse );
			}
		}
	}

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	clientNum = ci - cgs.clientinfo;

	for ( i = 0; i < MAX_GENTITIES; i++ )
	{
		if ( cg_entities[ i ].currentState.clientNum == clientNum &&
		     cg_entities[ i ].currentState.eType == ET_PLAYER )
		{
			CG_ResetPlayerEntity ( &cg_entities[ i ] );
		}
	}
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel ( clientInfo_t *from, clientInfo_t *to )
{
	VectorCopy ( from->headOffset, to->headOffset );
	to->footsteps = from->footsteps;
	to->gender = from->gender;

	to->legsModel = from->legsModel;
	to->legsSkin = from->legsSkin;
	to->torsoModel = from->torsoModel;
	to->torsoSkin = from->torsoSkin;
	to->headModel = from->headModel;
	to->headSkin = from->headSkin;
	to->nonSegModel = from->nonSegModel;
	to->nonSegSkin = from->nonSegSkin;
	to->nonsegmented = from->nonsegmented;
	to->modelIcon = from->modelIcon;

	memcpy ( to->animations, from->animations, sizeof ( to->animations ) );
	memcpy ( to->sounds, from->sounds, sizeof ( to->sounds ) );
	memcpy ( to->customFootsteps, from->customFootsteps, sizeof ( to->customFootsteps ) );
	memcpy ( to->customMetalFootsteps, from->customMetalFootsteps, sizeof ( to->customMetalFootsteps ) );
}

/*
======================
CG_GetCorpseNum
======================
*/
static int CG_GetCorpseNum ( pClass_t class )
{
	int          i;
	clientInfo_t *match;
	char         *modelName;
	char         *skinName;

	modelName = BG_FindModelNameForClass ( class );
	skinName = BG_FindSkinNameForClass ( class );

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		match = &cgs.corpseinfo[ i ];

		if ( !match->infoValid )
		{
			continue;
		}

		if ( !Q_stricmp ( modelName, match->modelName )
		     && !Q_stricmp ( skinName, match->skinName ) )
		{
			// this clientinfo is identical, so use it's handles
			return i;
		}
	}

	//something has gone horribly wrong
	return -1;
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo ( clientInfo_t *ci )
{
	int          i;
	clientInfo_t *match;

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		match = &cgs.corpseinfo[ i ];

		if ( !match->infoValid )
		{
			continue;
		}

		if ( !Q_stricmp ( ci->modelName, match->modelName ) &&
		     !Q_stricmp ( ci->skinName, match->skinName ) )
		{
			// this clientinfo is identical, so use it's handles
			CG_CopyClientInfoModel ( match, ci );

			return qtrue;
		}
	}

	//TA: shouldn't happen
	return qfalse;
}

/*
======================
CG_PrecacheClientInfo
======================
*/
void CG_PrecacheClientInfo ( pClass_t class, char *model, char *skin )
{
	clientInfo_t *ci;
	clientInfo_t newInfo;

	ci = &cgs.corpseinfo[ class ];

	// the old value
	memset ( &newInfo, 0, sizeof ( newInfo ) );

	// model
	Q_strncpyz ( newInfo.modelName, model, sizeof ( newInfo.modelName ) );
	Q_strncpyz ( newInfo.headModelName, model, sizeof ( newInfo.headModelName ) );

	// modelName didn not include a skin name
	if ( !skin )
	{
		Q_strncpyz ( newInfo.skinName, "default", sizeof ( newInfo.skinName ) );
		Q_strncpyz ( newInfo.headSkinName, "default", sizeof ( newInfo.headSkinName ) );
	}
	else
	{
		Q_strncpyz ( newInfo.skinName, skin, sizeof ( newInfo.skinName ) );
		Q_strncpyz ( newInfo.headSkinName, skin, sizeof ( newInfo.headSkinName ) );
	}

	newInfo.infoValid = qtrue;

	//TA: actually register the models
	*ci = newInfo;
	CG_LoadClientInfo ( ci );
}

/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo ( int clientNum )
{
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char   *configstring;
	const char   *v;
	char         *slash;

	ci = &cgs.clientinfo[ clientNum ];

	configstring = CG_ConfigString ( clientNum + CS_PLAYERS );

	if ( !configstring[ 0 ] )
	{
		memset ( ci, 0, sizeof ( *ci ) );
		return; // player just left
	}

	// the old value
	memset ( &newInfo, 0, sizeof ( newInfo ) );

	// isolate the player's name
	v = Info_ValueForKey ( configstring, "n" );
	Q_strncpyz ( newInfo.name, v, sizeof ( newInfo.name ) );

	// colors
	v = Info_ValueForKey ( configstring, "c1" );
	CG_ColorFromString ( v, newInfo.color1 );

	v = Info_ValueForKey ( configstring, "c2" );
	CG_ColorFromString ( v, newInfo.color2 );

	// bot skill
	v = Info_ValueForKey ( configstring, "skill" );
	newInfo.botSkill = atoi ( v );

	// handicap
	v = Info_ValueForKey ( configstring, "hc" );
	newInfo.handicap = atoi ( v );

	// wins
	v = Info_ValueForKey ( configstring, "w" );
	newInfo.wins = atoi ( v );

	// losses
	v = Info_ValueForKey ( configstring, "l" );
	newInfo.losses = atoi ( v );

	// team
	v = Info_ValueForKey ( configstring, "t" );
	newInfo.team = atoi ( v );

	// team task
	v = Info_ValueForKey ( configstring, "tt" );
	newInfo.teamTask = atoi ( v );

	// team leader
	v = Info_ValueForKey ( configstring, "tl" );
	newInfo.teamLeader = atoi ( v );

	v = Info_ValueForKey ( configstring, "g_redteam" );
	Q_strncpyz ( newInfo.redTeam, v, MAX_TEAMNAME );

	v = Info_ValueForKey ( configstring, "g_blueteam" );
	Q_strncpyz ( newInfo.blueTeam, v, MAX_TEAMNAME );

	// model
	v = Info_ValueForKey ( configstring, "model" );
	Q_strncpyz ( newInfo.modelName, v, sizeof ( newInfo.modelName ) );

	slash = strchr ( newInfo.modelName, '/' );

	if ( !slash )
	{
		// modelName didn not include a skin name
		Q_strncpyz ( newInfo.skinName, "default", sizeof ( newInfo.skinName ) );
	}
	else
	{
		Q_strncpyz ( newInfo.skinName, slash + 1, sizeof ( newInfo.skinName ) );
		// truncate modelName
		*slash = 0;
	}

	//CG_Printf( "NCI: %s\n", v );

	// head model
	v = Info_ValueForKey ( configstring, "hmodel" );
	Q_strncpyz ( newInfo.headModelName, v, sizeof ( newInfo.headModelName ) );

	slash = strchr ( newInfo.headModelName, '/' );

	if ( !slash )
	{
		// modelName didn not include a skin name
		Q_strncpyz ( newInfo.headSkinName, "default", sizeof ( newInfo.headSkinName ) );
	}
	else
	{
		Q_strncpyz ( newInfo.headSkinName, slash + 1, sizeof ( newInfo.headSkinName ) );
		// truncate modelName
		*slash = 0;
	}

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci = newInfo;

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo ( ci ) )
	{
		CG_LoadClientInfo ( ci );
	}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/

/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation ( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation )
{
	animation_t *anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_PLAYER_TOTALANIMATIONS )
	{
		CG_Error ( "Bad animation number: %i", newAnimation );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if ( cg_debugAnim.integer )
	{
		CG_Printf ( "Anim: %i\n", newAnimation );
	}
}

/*
===============
CG_RunPlayerLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunPlayerLerpFrame ( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale )
{
	int         f, numFrames;
	animation_t *anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 )
	{
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation )
	{
		CG_SetLerpFrameAnimation ( ci, lf, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime )
	{
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;

		if ( !anim->frameLerp )
		{
			return; // shouldn't happen
		}

		if ( cg.time < lf->animationTime )
		{
			lf->frameTime = lf->animationTime; // initial lerp
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}

		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f *= speedScale; // adjust for haste, etc
		numFrames = anim->numFrames;

		if ( anim->flipflop )
		{
			numFrames *= 2;
		}

		if ( f >= numFrames )
		{
			f -= numFrames;

			if ( anim->loopFrames )
			{
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			}
			else
			{
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}

		if ( anim->reversed )
		{
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else if ( anim->flipflop && f >= anim->numFrames )
		{
			lf->frame = anim->firstFrame + anim->numFrames - 1 - ( f % anim->numFrames );
		}
		else
		{
			lf->frame = anim->firstFrame + f;
		}

		if ( cg.time > lf->frameTime )
		{
			lf->frameTime = cg.time;

			if ( cg_debugAnim.integer )
			{
				CG_Printf ( "Clamp lf->frameTime\n" );
			}
		}
	}

	if ( lf->frameTime > cg.time + 200 )
	{
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time )
	{
		lf->oldFrameTime = cg.time;
	}

	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime )
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0 - ( float ) ( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}

/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame ( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber )
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation ( ci, lf, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}

/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation ( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
                                 int *torsoOld, int *torso, float *torsoBackLerp )
{
	clientInfo_t *ci;
	int          clientNum;
	float        speedScale = 1.0f;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer )
	{
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	ci = &cgs.clientinfo[ clientNum ];

	// do the shuffle turn frames locally
	if ( cent->pe.legs.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE )
	{
		CG_RunPlayerLerpFrame ( ci, &cent->pe.legs, LEGS_TURN, speedScale );
	}
	else
	{
		CG_RunPlayerLerpFrame ( ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale );
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunPlayerLerpFrame ( ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale );

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

/*
===============
CG_PlayerNonSegAnimation
===============
*/
static void CG_PlayerNonSegAnimation ( centity_t *cent, int *nonSegOld,
                                       int *nonSeg, float *nonSegBackLerp )
{
	clientInfo_t *ci;
	int          clientNum;
	float        speedScale = 1.0f;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer )
	{
		*nonSegOld = *nonSeg = 0;
		return;
	}

	ci = &cgs.clientinfo[ clientNum ];

	// do the shuffle turn frames locally
	if ( cent->pe.nonseg.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == NSPA_STAND )
	{
		CG_RunPlayerLerpFrame ( ci, &cent->pe.nonseg, NSPA_TURN, speedScale );
	}
	else
	{
		CG_RunPlayerLerpFrame ( ci, &cent->pe.nonseg, cent->currentState.legsAnim, speedScale );
	}

	*nonSegOld = cent->pe.nonseg.oldFrame;
	*nonSeg = cent->pe.nonseg.frame;
	*nonSegBackLerp = cent->pe.nonseg.backlerp;
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles ( float destination, float swingTolerance, float clampTolerance,
                             float speed, float *angle, qboolean *swinging )
{
	float swing;
	float move;
	float scale;

	if ( !*swinging )
	{
		// see if a swing should be started
		swing = AngleSubtract ( *angle, destination );

		if ( swing > swingTolerance || swing < -swingTolerance )
		{
			*swinging = qtrue;
		}
	}

	if ( !*swinging )
	{
		return;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract ( destination, *angle );
	scale = fabs ( swing );

	if ( scale < swingTolerance * 0.5 )
	{
		scale = 0.5;
	}
	else if ( scale < swingTolerance )
	{
		scale = 1.0;
	}
	else
	{
		scale = 2.0;
	}

	// swing towards the destination angle
	if ( swing >= 0 )
	{
		move = cg.frametime * scale * speed;

		if ( move >= swing )
		{
			move = swing;
			*swinging = qfalse;
		}

		*angle = AngleMod ( *angle + move );
	}
	else if ( swing < 0 )
	{
		move = cg.frametime * scale * -speed;

		if ( move <= swing )
		{
			move = swing;
			*swinging = qfalse;
		}

		*angle = AngleMod ( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract ( destination, *angle );

	if ( swing > clampTolerance )
	{
		*angle = AngleMod ( destination - ( clampTolerance - 1 ) );
	}
	else if ( swing < -clampTolerance )
	{
		*angle = AngleMod ( destination + ( clampTolerance - 1 ) );
	}
}

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch ( centity_t *cent, vec3_t torsoAngles )
{
	int   t;
	float f;

	t = cg.time - cent->pe.painTime;

	if ( t >= PAIN_TWITCH_TIME )
	{
		return;
	}

	f = 1.0 - ( float ) t / PAIN_TWITCH_TIME;

	if ( cent->pe.painDirection )
	{
		torsoAngles[ ROLL ] += 20 * f;
	}
	else
	{
		torsoAngles[ ROLL ] -= 20 * f;
	}
}

/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles ( centity_t *cent, vec3_t srcAngles,
                              vec3_t legs[ 3 ], vec3_t torso[ 3 ], vec3_t head[ 3 ] )
{
	vec3_t       legsAngles, torsoAngles, headAngles;
	float        dest;
	static int   movementOffsets[ 8 ] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	vec3_t       velocity;
	float        speed;
	int          dir, clientNum;
	clientInfo_t *ci;

	VectorCopy ( srcAngles, headAngles );
	headAngles[ YAW ] = AngleMod ( headAngles[ YAW ] );
	VectorClear ( legsAngles );
	VectorClear ( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_IDLE ||
	     ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) != TORSO_STAND  )
	{
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue; // always center
		cent->pe.torso.pitching = qtrue; // always center
		cent->pe.legs.yawing = qtrue; // always center
	}

	// adjust legs for movement dir
	if ( cent->currentState.eFlags & EF_DEAD )
	{
		// don't let dead bodies twitch
		dir = 0;
	}
	else
	{
		//TA: did use angles2.. now uses time2.. looks a bit funny but time2 isn't used othwise
		dir = cent->currentState.time2;

		if ( dir < 0 || dir > 7 )
		{
			CG_Error ( "Bad player movement angle" );
		}
	}

	legsAngles[ YAW ] = headAngles[ YAW ] + movementOffsets[ dir ];
	torsoAngles[ YAW ] = headAngles[ YAW ] + 0.25 * movementOffsets[ dir ];

	// torso
	if ( cent->currentState.eFlags & EF_DEAD )
	{
		CG_SwingAngles ( torsoAngles[ YAW ], 0, 0, cg_swingSpeed.value,
		                 &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
		CG_SwingAngles ( legsAngles[ YAW ], 0, 0, cg_swingSpeed.value,
		                 &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
	}
	else
	{
		CG_SwingAngles ( torsoAngles[ YAW ], 25, 90, cg_swingSpeed.value,
		                 &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
		CG_SwingAngles ( legsAngles[ YAW ], 40, 90, cg_swingSpeed.value,
		                 &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
	}

	torsoAngles[ YAW ] = cent->pe.torso.yawAngle;
	legsAngles[ YAW ] = cent->pe.legs.yawAngle;

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if ( headAngles[ PITCH ] > 180 )
	{
		dest = ( -360 + headAngles[ PITCH ] ) * 0.75f;
	}
	else
	{
		dest = headAngles[ PITCH ] * 0.75f;
	}

	CG_SwingAngles ( dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	torsoAngles[ PITCH ] = cent->pe.torso.pitchAngle;

	//
	clientNum = cent->currentState.clientNum;

	if ( clientNum >= 0 && clientNum < MAX_CLIENTS )
	{
		ci = &cgs.clientinfo[ clientNum ];

		if ( ci->fixedtorso )
		{
			torsoAngles[ PITCH ] = 0.0f;
		}
	}

	// --------- roll -------------

	// lean towards the direction of travel
	VectorCopy ( cent->currentState.pos.trDelta, velocity );
	speed = VectorNormalize ( velocity );

	if ( speed )
	{
		vec3_t axis[ 3 ];
		float  side;

		speed *= 0.05f;

		AnglesToAxis ( legsAngles, axis );
		side = speed * DotProduct ( velocity, axis[ 1 ] );
		legsAngles[ ROLL ] -= side;

		side = speed * DotProduct ( velocity, axis[ 0 ] );
		legsAngles[ PITCH ] += side;
	}

	//
	clientNum = cent->currentState.clientNum;

	if ( clientNum >= 0 && clientNum < MAX_CLIENTS )
	{
		ci = &cgs.clientinfo[ clientNum ];

		if ( ci->fixedlegs )
		{
			legsAngles[ YAW ] = torsoAngles[ YAW ];
			legsAngles[ PITCH ] = 0.0f;
			legsAngles[ ROLL ] = 0.0f;
		}
	}

	// pain twitch
	CG_AddPainTwitch ( cent, torsoAngles );

	// pull the angles back out of the hierarchial chain
	AnglesSubtract ( headAngles, torsoAngles, headAngles );
	AnglesSubtract ( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis ( legsAngles, legs );
	AnglesToAxis ( torsoAngles, torso );
	AnglesToAxis ( headAngles, head );
}

#define MODEL_WWSMOOTHTIME 200

/*
===============
CG_PlayerWWSmoothing

Smooth the angles of transitioning wall walkers
===============
*/
static void CG_PlayerWWSmoothing ( centity_t *cent, vec3_t in[ 3 ], vec3_t out[ 3 ] )
{
	entityState_t *es = &cent->currentState;
	int           i;
	vec3_t        surfNormal, rotAxis, temp;
	vec3_t        refNormal = { 0.0f, 0.0f,  1.0f };
	vec3_t        ceilingNormal = { 0.0f, 0.0f, -1.0f };
	float         stLocal, sFraction, rotAngle;
	vec3_t        inAxis[ 3 ], lastAxis[ 3 ], outAxis[ 3 ];

	//set surfNormal
	if ( ! ( es->eFlags & EF_WALLCLIMB ) )
	{
		VectorCopy ( refNormal, surfNormal );
	}
	else if ( ! ( es->eFlags & EF_WALLCLIMBCEILING ) )
	{
		VectorCopy ( es->angles2, surfNormal );
	}
	else
	{
		VectorCopy ( ceilingNormal, surfNormal );
	}

	AxisCopy ( in, inAxis );

	if ( !VectorCompare ( surfNormal, cent->pe.lastNormal ) )
	{
		//if we moving from the ceiling to the floor special case
		//( x product of colinear vectors is undefined)
		if ( VectorCompare ( ceilingNormal, cent->pe.lastNormal ) &&
		     VectorCompare ( refNormal,     surfNormal ) )
		{
			VectorCopy ( in[ 1 ], rotAxis );
			rotAngle = 180.0f;
		}
		else
		{
			AxisCopy ( cent->pe.lastAxis, lastAxis );
			rotAngle = DotProduct ( inAxis[ 0 ], lastAxis[ 0 ] ) +
			           DotProduct ( inAxis[ 1 ], lastAxis[ 1 ] ) +
			           DotProduct ( inAxis[ 2 ], lastAxis[ 2 ] );

			rotAngle = RAD2DEG ( acos ( ( rotAngle - 1.0f ) / 2.0f ) );

			CrossProduct ( lastAxis[ 0 ], inAxis[ 0 ], temp );
			VectorCopy ( temp, rotAxis );
			CrossProduct ( lastAxis[ 1 ], inAxis[ 1 ], temp );
			VectorAdd ( rotAxis, temp, rotAxis );
			CrossProduct ( lastAxis[ 2 ], inAxis[ 2 ], temp );
			VectorAdd ( rotAxis, temp, rotAxis );

			VectorNormalize ( rotAxis );
		}

		//iterate through smooth array
		for ( i = 0; i < MAXSMOOTHS; i++ )
		{
			//found an unused index in the smooth array
			if ( cent->pe.sList[ i ].time + MODEL_WWSMOOTHTIME < cg.time )
			{
				//copy to array and stop
				VectorCopy ( rotAxis, cent->pe.sList[ i ].rotAxis );
				cent->pe.sList[ i ].rotAngle = rotAngle;
				cent->pe.sList[ i ].time = cg.time;
				break;
			}
		}
	}

	//iterate through ops
	for ( i = MAXSMOOTHS - 1; i >= 0; i-- )
	{
		//if this op has time remaining, perform it
		if ( cg.time < cent->pe.sList[ i ].time + MODEL_WWSMOOTHTIME )
		{
			stLocal = 1.0f - ( ( ( cent->pe.sList[ i ].time + MODEL_WWSMOOTHTIME ) - cg.time ) / MODEL_WWSMOOTHTIME );
			sFraction = - ( cos ( stLocal * M_PI ) + 1.0f ) / 2.0f;

			RotatePointAroundVector ( outAxis[ 0 ], cent->pe.sList[ i ].rotAxis,
			                          inAxis[ 0 ], sFraction * cent->pe.sList[ i ].rotAngle );
			RotatePointAroundVector ( outAxis[ 1 ], cent->pe.sList[ i ].rotAxis,
			                          inAxis[ 1 ], sFraction * cent->pe.sList[ i ].rotAngle );
			RotatePointAroundVector ( outAxis[ 2 ], cent->pe.sList[ i ].rotAxis,
			                          inAxis[ 2 ], sFraction * cent->pe.sList[ i ].rotAngle );

			AxisCopy ( outAxis, inAxis );
		}
	}

	//outAxis has been copied to inAxis
	AxisCopy ( inAxis, out );
}

/*
===============
CG_PlayerNonSegAngles

Resolve angles for non-segmented models
===============
*/
static void CG_PlayerNonSegAngles ( centity_t *cent, vec3_t srcAngles, vec3_t nonSegAxis[ 3 ] )
{
	vec3_t        localAngles;
	vec3_t        velocity;
	float         speed;
	int           dir;
	entityState_t *es = &cent->currentState;
	vec3_t        surfNormal;
	vec3_t        ceilingNormal = { 0.0f, 0.0f, -1.0f };

	VectorCopy ( srcAngles, localAngles );
	localAngles[ YAW ] = AngleMod ( localAngles[ YAW ] );
	localAngles[ PITCH ] = 0.0f;
	localAngles[ ROLL ] = 0.0f;

	//set surfNormal
	if ( ! ( es->eFlags & EF_WALLCLIMBCEILING ) )
	{
		VectorCopy ( es->angles2, surfNormal );
	}
	else
	{
		VectorCopy ( ceilingNormal, surfNormal );
	}

	//make sure that WW transitions don't cause the swing stuff to go nuts
	if ( !VectorCompare ( surfNormal, cent->pe.lastNormal ) )
	{
		//stop CG_SwingAngles having an eppy
		cent->pe.nonseg.yawAngle = localAngles[ YAW ];
		cent->pe.nonseg.yawing = qfalse;
	}

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != NSPA_STAND )
	{
		// if not standing still, always point all in the same direction
		cent->pe.nonseg.yawing = qtrue; // always center
	}

	// adjust legs for movement dir
	if ( cent->currentState.eFlags & EF_DEAD )
	{
		// don't let dead bodies twitch
		dir = 0;
	}
	else
	{
		//TA: did use angles2.. now uses time2.. looks a bit funny but time2 isn't used othwise
		dir = cent->currentState.time2;

		if ( dir < 0 || dir > 7 )
		{
			CG_Error ( "Bad player movement angle" );
		}
	}

	// torso
	if ( cent->currentState.eFlags & EF_DEAD )
	{
		CG_SwingAngles ( localAngles[ YAW ], 0, 0, cg_swingSpeed.value,
		                 &cent->pe.nonseg.yawAngle, &cent->pe.nonseg.yawing );
	}
	else
	{
		CG_SwingAngles ( localAngles[ YAW ], 40, 90, cg_swingSpeed.value,
		                 &cent->pe.nonseg.yawAngle, &cent->pe.nonseg.yawing );
	}

	localAngles[ YAW ] = cent->pe.nonseg.yawAngle;

	// --------- pitch -------------

	//NO PITCH!

	// --------- roll -------------

	// lean towards the direction of travel
	VectorCopy ( cent->currentState.pos.trDelta, velocity );
	speed = VectorNormalize ( velocity );

	if ( speed )
	{
		vec3_t axis[ 3 ];
		float  side;

		//much less than with the regular model system
		speed *= 0.01f;

		AnglesToAxis ( localAngles, axis );
		side = speed * DotProduct ( velocity, axis[ 1 ] );
		localAngles[ ROLL ] -= side;

		side = speed * DotProduct ( velocity, axis[ 0 ] );
		localAngles[ PITCH ] += side;
	}

	//FIXME: PAIN[123] animations?
	// pain twitch
	//CG_AddPainTwitch( cent, torsoAngles );

	AnglesToAxis ( localAngles, nonSegAxis );
}

//==========================================================================

/*
===============
CG_PlayerUpgrade
===============
*/
static void CG_PlayerUpgrades ( centity_t *cent, refEntity_t *torso )
{
	int           held, active;
	refEntity_t   jetpack;
	refEntity_t   battpack;
	refEntity_t   flash;
	entityState_t *es = &cent->currentState;

	held = es->modelindex;
	active = es->modelindex2;

	if ( held & ( 1 << UP_JETPACK ) )
	{
		memset ( &jetpack, 0, sizeof ( jetpack ) );
		VectorCopy ( torso->lightingOrigin, jetpack.lightingOrigin );
		jetpack.shadowPlane = torso->shadowPlane;
		jetpack.renderfx = torso->renderfx;

		jetpack.hModel = cgs.media.jetpackModel;

		//identity matrix
		AxisCopy ( axisDefault, jetpack.axis );

		//FIXME: change to tag_back when it exists
		CG_PositionRotatedEntityOnTag ( &jetpack, torso, torso->hModel, "tag_head" );

		trap_R_AddRefEntityToScene ( &jetpack );

		if ( active & ( 1 << UP_JETPACK ) )
		{
			if ( es->pos.trDelta[ 2 ] > 10.0f )
			{
				if ( cent->jetPackState != JPS_ASCENDING )
				{
					if ( CG_IsParticleSystemValid ( &cent->jetPackPS ) )
					{
						CG_DestroyParticleSystem ( &cent->jetPackPS );
					}

					cent->jetPackPS = CG_SpawnNewParticleSystem ( cgs.media.jetPackAscendPS );
					cent->jetPackState = JPS_ASCENDING;
				}

				/*trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin,
				                        vec3_origin, cgs.media.jetpackAscendSound );*/
			}
			else if ( es->pos.trDelta[ 2 ] < -10.0f )
			{
				if ( cent->jetPackState != JPS_DESCENDING )
				{
					if ( CG_IsParticleSystemValid ( &cent->jetPackPS ) )
					{
						CG_DestroyParticleSystem ( &cent->jetPackPS );
					}

					cent->jetPackPS = CG_SpawnNewParticleSystem ( cgs.media.jetPackDescendPS );
					cent->jetPackState = JPS_DESCENDING;
				}

				/*trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin,
				                        vec3_origin, cgs.media.jetpackDescendSound );*/
			}
			else
			{
				if ( cent->jetPackState != JPS_HOVERING )
				{
					if ( CG_IsParticleSystemValid ( &cent->jetPackPS ) )
					{
						CG_DestroyParticleSystem ( &cent->jetPackPS );
					}

					cent->jetPackPS = CG_SpawnNewParticleSystem ( cgs.media.jetPackHoverPS );
					cent->jetPackState = JPS_HOVERING;
				}

				/*trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin,
				                        vec3_origin, cgs.media.jetpackIdleSound );*/
			}

			memset ( &flash, 0, sizeof ( flash ) );
			VectorCopy ( torso->lightingOrigin, flash.lightingOrigin );
			flash.shadowPlane = torso->shadowPlane;
			flash.renderfx = torso->renderfx;

			flash.hModel = cgs.media.jetpackFlashModel;

			if ( !flash.hModel )
			{
				return;
			}

			AxisCopy ( axisDefault, flash.axis );

			CG_PositionRotatedEntityOnTag ( &flash, &jetpack, jetpack.hModel, "tag_flash" );
			trap_R_AddRefEntityToScene ( &flash );

			if ( CG_IsParticleSystemValid ( &cent->jetPackPS ) )
			{
				CG_SetAttachmentTag ( &cent->jetPackPS->attachment,
				                      jetpack, jetpack.hModel, "tag_flash" );
				CG_SetAttachmentCent ( &cent->jetPackPS->attachment, cent );
				CG_AttachToTag ( &cent->jetPackPS->attachment );
			}
		}
		else if ( CG_IsParticleSystemValid ( &cent->jetPackPS ) )
		{
			CG_DestroyParticleSystem ( &cent->jetPackPS );
			cent->jetPackState = JPS_OFF;
		}
	}
	else if ( CG_IsParticleSystemValid ( &cent->jetPackPS ) )
	{
		CG_DestroyParticleSystem ( &cent->jetPackPS );
		cent->jetPackState = JPS_OFF;
	}

	if ( held & ( 1 << UP_BATTPACK ) )
	{
		memset ( &battpack, 0, sizeof ( battpack ) );
		VectorCopy ( torso->lightingOrigin, battpack.lightingOrigin );
		battpack.shadowPlane = torso->shadowPlane;
		battpack.renderfx = torso->renderfx;

		battpack.hModel = cgs.media.battpackModel;

		//identity matrix
		AxisCopy ( axisDefault, battpack.axis );

		//FIXME: change to tag_back when it exists
		CG_PositionRotatedEntityOnTag ( &battpack, torso, torso->hModel, "tag_head" );

		trap_R_AddRefEntityToScene ( &battpack );
	}

	if ( es->eFlags & EF_BLOBLOCKED )
	{
		vec3_t  temp, origin, up = { 0.0f, 0.0f, 1.0f };
		trace_t tr;
		float   size;

		VectorCopy ( es->pos.trBase, temp );
		temp[ 2 ] -= 4096.0f;

		CG_Trace ( &tr, es->pos.trBase, NULL, NULL, temp, es->number, MASK_SOLID );
		VectorCopy ( tr.endpos, origin );

		size = 32.0f;

		if ( size > 0.0f )
		{
			CG_ImpactMark ( cgs.media.creepShader, origin, up,
			                0.0f, 1.0f, 1.0f, 1.0f, 1.0f, qfalse, size, qtrue );
		}
	}
}

/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite ( centity_t *cent, qhandle_t shader )
{
	int         rf;
	refEntity_t ent;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson )
	{
		rf = RF_THIRD_PERSON; // only show in mirrors
	}
	else
	{
		rf = 0;
	}

	memset ( &ent, 0, sizeof ( ent ) );
	VectorCopy ( cent->lerpOrigin, ent.origin );
	ent.origin[ 2 ] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[ 0 ] = 255;
	ent.shaderRGBA[ 1 ] = 255;
	ent.shaderRGBA[ 2 ] = 255;
	ent.shaderRGBA[ 3 ] = 255;
	trap_R_AddRefEntityToScene ( &ent );
}

/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites ( centity_t *cent )
{
	if ( cent->currentState.eFlags & EF_CONNECTION )
	{
		CG_PlayerFloatSprite ( cent, cgs.media.connectionShader );
		return;
	}

	if ( cent->currentState.eFlags & EF_TALK )
	{
		//TA: the masses have decreed this to be wrong

		/*    CG_PlayerFloatSprite( cent, cgs.media.balloonShader );
		    return;*/
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define SHADOW_DISTANCE 128
static qboolean CG_PlayerShadow ( centity_t *cent, float *shadowPlane, pClass_t class )
{
	vec3_t        end, mins, maxs;
	trace_t       trace;
	float         alpha;
	entityState_t *es = &cent->currentState;
	vec3_t        surfNormal = { 0.0f, 0.0f, 1.0f };

	BG_FindBBoxForClass ( class, mins, maxs, NULL, NULL, NULL );
	mins[ 2 ] = 0.0f;
	maxs[ 2 ] = 2.0f;

	if ( es->eFlags & EF_WALLCLIMB )
	{
		if ( es->eFlags & EF_WALLCLIMBCEILING )
		{
			VectorSet ( surfNormal, 0.0f, 0.0f, -1.0f );
		}
		else
		{
			VectorCopy ( es->angles2, surfNormal );
		}
	}

	*shadowPlane = 0;

	if ( cg_shadows.integer == 0 )
	{
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy ( cent->lerpOrigin, end );
	VectorMA ( cent->lerpOrigin, -SHADOW_DISTANCE, surfNormal, end );

	trap_CM_BoxTrace ( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid )
	{
		return qfalse;
	}

	//TA: FIXME: stencil shadows will be broken for walls.
	//           Unfortunately there isn't much that can be
	//           done since Q3 references only the Z coord
	//           of the shadowPlane
	if ( surfNormal[ 2 ] < 0.0f )
	{
		*shadowPlane = trace.endpos[ 2 ] - 1.0f;
	}
	else
	{
		*shadowPlane = trace.endpos[ 2 ] + 1.0f;
	}

	if ( cg_shadows.integer != 1 ) // no mark for stencil or projection shadows
	{
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark ( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal,
	                cent->pe.legs.yawAngle, 0.0f, 0.0f, 0.0f, alpha, qfalse,
	                24.0f * BG_FindShadowScaleForClass ( class ), qtrue );

	return qtrue;
}

/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash ( centity_t *cent, pClass_t class )
{
	vec3_t  start, end;
	vec3_t  mins, maxs;
	trace_t trace;
	int     contents;

	if ( !cg_shadows.integer )
	{
		return;
	}

	BG_FindBBoxForClass ( class, mins, maxs, NULL, NULL, NULL );

	VectorCopy ( cent->lerpOrigin, end );
	end[ 2 ] += mins[ 2 ];

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = trap_CM_PointContents ( end, 0 );

	if ( ! ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) )
	{
		return;
	}

	VectorCopy ( cent->lerpOrigin, start );
	start[ 2 ] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents ( start, 0 );

	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
	{
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace ( &trace, start, end, NULL, NULL, 0,
	                   ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	if ( trace.fraction == 1.0f )
	{
		return;
	}

	CG_ImpactMark ( cgs.media.wakeMarkShader, trace.endpos, trace.plane.normal,
	                cent->pe.legs.yawAngle, 1.0f, 1.0f, 1.0f, 1.0f, qfalse,
	                32.0f * BG_FindShadowScaleForClass ( class ), qtrue );
}

/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts ( vec3_t normal, int numVerts, polyVert_t *verts )
{
	int    i, j;
	float  incoming;
	vec3_t ambientLight;
	vec3_t lightDir;
	vec3_t directedLight;

	trap_R_LightForPoint ( verts[ 0 ].xyz, ambientLight, directedLight, lightDir );

	for ( i = 0; i < numVerts; i++ )
	{
		incoming = DotProduct ( normal, lightDir );

		if ( incoming <= 0 )
		{
			verts[ i ].modulate[ 0 ] = ambientLight[ 0 ];
			verts[ i ].modulate[ 1 ] = ambientLight[ 1 ];
			verts[ i ].modulate[ 2 ] = ambientLight[ 2 ];
			verts[ i ].modulate[ 3 ] = 255;
			continue;
		}

		j = ( ambientLight[ 0 ] + incoming * directedLight[ 0 ] );

		if ( j > 255 )
		{
			j = 255;
		}

		verts[ i ].modulate[ 0 ] = j;

		j = ( ambientLight[ 1 ] + incoming * directedLight[ 1 ] );

		if ( j > 255 )
		{
			j = 255;
		}

		verts[ i ].modulate[ 1 ] = j;

		j = ( ambientLight[ 2 ] + incoming * directedLight[ 2 ] );

		if ( j > 255 )
		{
			j = 255;
		}

		verts[ i ].modulate[ 2 ] = j;

		verts[ i ].modulate[ 3 ] = 255;
	}

	return qtrue;
}

/*
=================
CG_LightFromDirection
=================
*/
int CG_LightFromDirection ( vec3_t point, vec3_t direction )
{
	int    j;
	float  incoming;
	vec3_t ambientLight;
	vec3_t lightDir;
	vec3_t directedLight;
	vec3_t result;

	trap_R_LightForPoint ( point, ambientLight, directedLight, lightDir );

	incoming = DotProduct ( direction, lightDir );

	if ( incoming <= 0 )
	{
		result[ 0 ] = ambientLight[ 0 ];
		result[ 1 ] = ambientLight[ 1 ];
		result[ 2 ] = ambientLight[ 2 ];
		return ( int ) ( ( float ) ( result[ 0 ] + result[ 1 ] + result[ 2 ] ) / 3.0f );
	}

	j = ( ambientLight[ 0 ] + incoming * directedLight[ 0 ] );

	if ( j > 255 )
	{
		j = 255;
	}

	result[ 0 ] = j;

	j = ( ambientLight[ 1 ] + incoming * directedLight[ 1 ] );

	if ( j > 255 )
	{
		j = 255;
	}

	result[ 1 ] = j;

	j = ( ambientLight[ 2 ] + incoming * directedLight[ 2 ] );

	if ( j > 255 )
	{
		j = 255;
	}

	result[ 2 ] = j;

	return ( int ) ( ( float ) ( result[ 0 ] + result[ 1 ] + result[ 2 ] ) / 3.0f );
}

/*
=================
CG_AmbientLight
=================
*/
int CG_AmbientLight ( vec3_t point )
{
	vec3_t ambientLight;
	vec3_t lightDir;
	vec3_t directedLight;
	vec3_t result;

	trap_R_LightForPoint ( point, ambientLight, directedLight, lightDir );

	result[ 0 ] = ambientLight[ 0 ];
	result[ 1 ] = ambientLight[ 1 ];
	result[ 2 ] = ambientLight[ 2 ];
	return ( int ) ( ( float ) ( result[ 0 ] + result[ 1 ] + result[ 2 ] ) / 3.0f );
}

#define TRACE_DEPTH 32.0f

/*
===============
CG_Player
===============
*/
void CG_Player ( centity_t *cent )
{
	clientInfo_t *ci;

	//TA: NOTE: legs is used for nonsegmented models
	//          this helps reduce code to be changed
	refEntity_t   legs;
	refEntity_t   torso;
	refEntity_t   head;
	int           clientNum;
	int           renderfx;
	qboolean      shadow = qfalse;
	float         shadowPlane;
	entityState_t *es = &cent->currentState;
	pClass_t      class = ( es->powerups >> 8 ) & 0xFF;
	float         scale;
	vec3_t        tempAxis[ 3 ], tempAxis2[ 3 ];
	vec3_t        angles;
	int           held = es->modelindex;
	vec3_t        surfNormal = { 0.0f, 0.0f, 1.0f };

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = es->clientNum;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS )
	{
		CG_Error ( "Bad clientNum on player entity" );
	}

	ci = &cgs.clientinfo[ clientNum ];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid )
	{
		return;
	}

	//don't draw
	if ( es->eFlags & EF_NODRAW )
	{
		return;
	}

	// get the player model information
	renderfx = 0;

	if ( es->number == cg.snap->ps.clientNum )
	{
		if ( !cg.renderingThirdPerson )
		{
			renderfx = RF_THIRD_PERSON; // only draw in mirrors
		}
		else if ( cg_cameraMode.integer )
		{
			return;
		}
	}

	if ( cg_drawBBOX.integer )
	{
		vec3_t mins, maxs;

		BG_FindBBoxForClass ( class, mins, maxs, NULL, NULL, NULL );
		CG_DrawBoundingBox ( cent->lerpOrigin, mins, maxs );
	}

	memset ( &legs,    0, sizeof ( legs ) );
	memset ( &torso,   0, sizeof ( torso ) );
	memset ( &head,    0, sizeof ( head ) );

	VectorCopy ( cent->lerpAngles, angles );
	AnglesToAxis ( cent->lerpAngles, tempAxis );

	//rotate lerpAngles to floor
	if ( es->eFlags & EF_WALLCLIMB &&
	     BG_RotateAxis ( es->angles2, tempAxis, tempAxis2, qtrue, es->eFlags & EF_WALLCLIMBCEILING ) )
	{
		AxisToAngles ( tempAxis2, angles );
	}
	else
	{
		VectorCopy ( cent->lerpAngles, angles );
	}

	//normalise the pitch
	if ( angles[ PITCH ] < -180.0f )
	{
		angles[ PITCH ] += 360.0f;
	}

	// get the rotation information
	if ( !ci->nonsegmented )
	{
		CG_PlayerAngles ( cent, angles, legs.axis, torso.axis, head.axis );
	}
	else
	{
		CG_PlayerNonSegAngles ( cent, angles, legs.axis );
	}

	AxisCopy ( legs.axis, tempAxis );

	//rotate the legs axis to back to the wall
	if ( es->eFlags & EF_WALLCLIMB &&
	     BG_RotateAxis ( es->angles2, legs.axis, tempAxis, qfalse, es->eFlags & EF_WALLCLIMBCEILING ) )
	{
		AxisCopy ( tempAxis, legs.axis );
	}

	//smooth out WW transitions so the model doesn't hop around
	CG_PlayerWWSmoothing ( cent, legs.axis, legs.axis );

	AxisCopy ( tempAxis, cent->pe.lastAxis );

	// get the animation state (after rotation, to allow feet shuffle)
	if ( !ci->nonsegmented )
	{
		CG_PlayerAnimation ( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
		                     &torso.oldframe, &torso.frame, &torso.backlerp );
	}
	else
	{
		CG_PlayerNonSegAnimation ( cent, &legs.oldframe, &legs.frame, &legs.backlerp );
	}

	// add the talk baloon or disconnect icon
	CG_PlayerSprites ( cent );

	// add the shadow
	if ( ( es->number == cg.snap->ps.clientNum && cg.renderingThirdPerson ) ||
	     es->number != cg.snap->ps.clientNum )
	{
		shadow = CG_PlayerShadow ( cent, &shadowPlane, class );
	}

	// add a water splash if partially in and out of water
	CG_PlayerSplash ( cent, class );

	if ( cg_shadows.integer == 3 && shadow )
	{
		renderfx |= RF_SHADOW_PLANE;
	}

	renderfx |= RF_LIGHTING_ORIGIN; // use the same origin for all

	//
	// add the legs
	//
	if ( !ci->nonsegmented )
	{
		legs.hModel = ci->legsModel;

		if ( held & ( 1 << UP_LIGHTARMOUR ) )
		{
			legs.customSkin = cgs.media.larmourLegsSkin;
		}
		else
		{
			legs.customSkin = ci->legsSkin;
		}
	}
	else
	{
		legs.hModel = ci->nonSegModel;
		legs.customSkin = ci->nonSegSkin;
	}

	VectorCopy ( cent->lerpOrigin, legs.origin );

	VectorCopy ( cent->lerpOrigin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy ( legs.origin, legs.oldorigin ); // don't positionally lerp at all

	//move the origin closer into the wall with a CapTrace
	if ( es->eFlags & EF_WALLCLIMB && ! ( es->eFlags & EF_DEAD ) && ! ( cg.intermissionStarted ) )
	{
		vec3_t  start, end, mins, maxs;
		trace_t tr;

		if ( es->eFlags & EF_WALLCLIMBCEILING )
		{
			VectorSet ( surfNormal, 0.0f, 0.0f, -1.0f );
		}
		else
		{
			VectorCopy ( es->angles2, surfNormal );
		}

		BG_FindBBoxForClass ( class, mins, maxs, NULL, NULL, NULL );

		VectorMA ( legs.origin, -TRACE_DEPTH, surfNormal, end );
		VectorMA ( legs.origin, 1.0f, surfNormal, start );
		CG_CapTrace ( &tr, start, mins, maxs, end, es->number, MASK_PLAYERSOLID );

		//if the trace misses completely then just use legs.origin
		//apparently capsule traces are "smaller" than box traces
		if ( tr.fraction != 1.0f )
		{
			VectorMA ( legs.origin, tr.fraction * -TRACE_DEPTH, surfNormal, legs.origin );
		}

		VectorCopy ( legs.origin, legs.lightingOrigin );
		VectorCopy ( legs.origin, legs.oldorigin ); // don't positionally lerp at all
	}

	//rescale the model
	scale = BG_FindModelScaleForClass ( class );

	if ( scale != 1.0f )
	{
		VectorScale ( legs.axis[ 0 ], scale, legs.axis[ 0 ] );
		VectorScale ( legs.axis[ 1 ], scale, legs.axis[ 1 ] );
		VectorScale ( legs.axis[ 2 ], scale, legs.axis[ 2 ] );

		legs.nonNormalizedAxes = qtrue;
	}

	//offset on the Z axis if required
	VectorMA ( legs.origin, BG_FindZOffsetForClass ( class ), surfNormal, legs.origin );
	VectorCopy ( legs.origin, legs.lightingOrigin );
	VectorCopy ( legs.origin, legs.oldorigin ); // don't positionally lerp at all

	trap_R_AddRefEntityToScene ( &legs );

	// if the model failed, allow the default nullmodel to be displayed
	if ( !legs.hModel )
	{
		return;
	}

	if ( !ci->nonsegmented )
	{
		//
		// add the torso
		//
		torso.hModel = ci->torsoModel;

		if ( held & ( 1 << UP_LIGHTARMOUR ) )
		{
			torso.customSkin = cgs.media.larmourTorsoSkin;
		}
		else
		{
			torso.customSkin = ci->torsoSkin;
		}

		if ( !torso.hModel )
		{
			return;
		}

		VectorCopy ( cent->lerpOrigin, torso.lightingOrigin );

		CG_PositionRotatedEntityOnTag ( &torso, &legs, ci->legsModel, "tag_torso" );

		torso.shadowPlane = shadowPlane;
		torso.renderfx = renderfx;

		trap_R_AddRefEntityToScene ( &torso );

		//
		// add the head
		//
		head.hModel = ci->headModel;

		if ( held & ( 1 << UP_HELMET ) )
		{
			head.customSkin = cgs.media.larmourHeadSkin;
		}
		else
		{
			head.customSkin = ci->headSkin;
		}

		if ( !head.hModel )
		{
			return;
		}

		VectorCopy ( cent->lerpOrigin, head.lightingOrigin );

		CG_PositionRotatedEntityOnTag ( &head, &torso, ci->torsoModel, "tag_head" );

		head.shadowPlane = shadowPlane;
		head.renderfx = renderfx;

		trap_R_AddRefEntityToScene ( &head );
	}

	//
	// add the gun / barrel / flash
	//
	if ( es->weapon != WP_NONE )
	{
		if ( !ci->nonsegmented )
		{
			CG_AddPlayerWeapon ( &torso, NULL, cent );
		}
		else
		{
			CG_AddPlayerWeapon ( &legs, NULL, cent );
		}
	}

	CG_PlayerUpgrades ( cent, &torso );

	//sanity check that particle systems are stopped when dead
	if ( es->eFlags & EF_DEAD )
	{
		if ( CG_IsParticleSystemValid ( &cent->muzzlePS ) )
		{
			CG_DestroyParticleSystem ( &cent->muzzlePS );
		}

		if ( CG_IsParticleSystemValid ( &cent->jetPackPS ) )
		{
			CG_DestroyParticleSystem ( &cent->jetPackPS );
		}
	}

	VectorCopy ( surfNormal, cent->pe.lastNormal );
}

/*
===============
CG_Corpse
===============
*/
void CG_Corpse ( centity_t *cent )
{
	clientInfo_t  *ci;
	refEntity_t   legs;
	refEntity_t   torso;
	refEntity_t   head;
	entityState_t *es = &cent->currentState;
	int           corpseNum;
	int           renderfx;
	qboolean      shadow = qfalse;
	float         shadowPlane;
	vec3_t        origin, liveZ, deadZ;
	float         scale;

	corpseNum = CG_GetCorpseNum ( es->clientNum );

	if ( corpseNum < 0 || corpseNum >= MAX_CLIENTS )
	{
		CG_Error ( "Bad corpseNum on corpse entity: %d", corpseNum );
	}

	ci = &cgs.corpseinfo[ corpseNum ];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid )
	{
		return;
	}

	memset ( &legs, 0, sizeof ( legs ) );
	memset ( &torso, 0, sizeof ( torso ) );
	memset ( &head, 0, sizeof ( head ) );

	VectorCopy ( cent->lerpOrigin, origin );
	BG_FindBBoxForClass ( es->clientNum, liveZ, NULL, NULL, deadZ, NULL );
	origin[ 2 ] -= ( liveZ[ 2 ] - deadZ[ 2 ] );

	VectorCopy ( es->angles, cent->lerpAngles );

	// get the rotation information
	if ( !ci->nonsegmented )
	{
		CG_PlayerAngles ( cent, cent->lerpAngles, legs.axis, torso.axis, head.axis );
	}
	else
	{
		CG_PlayerNonSegAngles ( cent, cent->lerpAngles, legs.axis );
	}

	//set the correct frame (should always be dead)
	if ( cg_noPlayerAnims.integer )
	{
		legs.oldframe = legs.frame = torso.oldframe = torso.frame = 0;
	}
	else if ( !ci->nonsegmented )
	{
		memset ( &cent->pe.legs, 0, sizeof ( lerpFrame_t ) );
		CG_RunPlayerLerpFrame ( ci, &cent->pe.legs, es->legsAnim, 1 );
		legs.oldframe = cent->pe.legs.oldFrame;
		legs.frame = cent->pe.legs.frame;
		legs.backlerp = cent->pe.legs.backlerp;

		memset ( &cent->pe.torso, 0, sizeof ( lerpFrame_t ) );
		CG_RunPlayerLerpFrame ( ci, &cent->pe.torso, es->torsoAnim, 1 );
		torso.oldframe = cent->pe.torso.oldFrame;
		torso.frame = cent->pe.torso.frame;
		torso.backlerp = cent->pe.torso.backlerp;
	}
	else
	{
		memset ( &cent->pe.nonseg, 0, sizeof ( lerpFrame_t ) );
		CG_RunPlayerLerpFrame ( ci, &cent->pe.nonseg, es->legsAnim, 1 );
		legs.oldframe = cent->pe.nonseg.oldFrame;
		legs.frame = cent->pe.nonseg.frame;
		legs.backlerp = cent->pe.nonseg.backlerp;
	}

	// add the shadow
	shadow = CG_PlayerShadow ( cent, &shadowPlane, es->clientNum );

	// get the player model information
	renderfx = 0;

	if ( cg_shadows.integer == 3 && shadow )
	{
		renderfx |= RF_SHADOW_PLANE;
	}

	renderfx |= RF_LIGHTING_ORIGIN; // use the same origin for all

	//
	// add the legs
	//
	if ( !ci->nonsegmented )
	{
		legs.hModel = ci->legsModel;
		legs.customSkin = ci->legsSkin;
	}
	else
	{
		legs.hModel = ci->nonSegModel;
		legs.customSkin = ci->nonSegSkin;
	}

	VectorCopy ( origin, legs.origin );

	VectorCopy ( origin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	legs.origin[ 2 ] += BG_FindZOffsetForClass ( es->clientNum );
	VectorCopy ( legs.origin, legs.oldorigin ); // don't positionally lerp at all

	//rescale the model
	scale = BG_FindModelScaleForClass ( es->clientNum );

	if ( scale != 1.0f )
	{
		VectorScale ( legs.axis[ 0 ], scale, legs.axis[ 0 ] );
		VectorScale ( legs.axis[ 1 ], scale, legs.axis[ 1 ] );
		VectorScale ( legs.axis[ 2 ], scale, legs.axis[ 2 ] );

		legs.nonNormalizedAxes = qtrue;
	}

	//CG_AddRefEntityWithPowerups( &legs, es->powerups, ci->team );
	trap_R_AddRefEntityToScene ( &legs );

	// if the model failed, allow the default nullmodel to be displayed
	if ( !legs.hModel )
	{
		return;
	}

	if ( !ci->nonsegmented )
	{
		//
		// add the torso
		//
		torso.hModel = ci->torsoModel;

		if ( !torso.hModel )
		{
			return;
		}

		torso.customSkin = ci->torsoSkin;

		VectorCopy ( origin, torso.lightingOrigin );

		CG_PositionRotatedEntityOnTag ( &torso, &legs, ci->legsModel, "tag_torso" );

		torso.shadowPlane = shadowPlane;
		torso.renderfx = renderfx;

		//CG_AddRefEntityWithPowerups( &torso, es->powerups, ci->team );
		trap_R_AddRefEntityToScene ( &torso );

		//
		// add the head
		//
		head.hModel = ci->headModel;

		if ( !head.hModel )
		{
			return;
		}

		head.customSkin = ci->headSkin;

		VectorCopy ( origin, head.lightingOrigin );

		CG_PositionRotatedEntityOnTag ( &head, &torso, ci->torsoModel, "tag_head" );

		head.shadowPlane = shadowPlane;
		head.renderfx = renderfx;

		//CG_AddRefEntityWithPowerups( &head, es->powerups, ci->team );
		trap_R_AddRefEntityToScene ( &head );
	}
}

//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity ( centity_t *cent )
{
	cent->errorTime = -99999; // guarantee no error decay added
	cent->extrapolated = qfalse;

	CG_ClearLerpFrame ( &cgs.clientinfo[ cent->currentState.clientNum ],
	                    &cent->pe.legs, cent->currentState.legsAnim );
	CG_ClearLerpFrame ( &cgs.clientinfo[ cent->currentState.clientNum ],
	                    &cent->pe.torso, cent->currentState.torsoAnim );
	CG_ClearLerpFrame ( &cgs.clientinfo[ cent->currentState.clientNum ],
	                    &cent->pe.nonseg, cent->currentState.legsAnim );

	BG_EvaluateTrajectory ( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory ( &cent->currentState.apos, cg.time, cent->lerpAngles );

	VectorCopy ( cent->lerpOrigin, cent->rawOrigin );
	VectorCopy ( cent->lerpAngles, cent->rawAngles );

	memset ( &cent->pe.legs, 0, sizeof ( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->rawAngles[ YAW ];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset ( &cent->pe.torso, 0, sizeof ( cent->pe.legs ) );
	cent->pe.torso.yawAngle = cent->rawAngles[ YAW ];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[ PITCH ];
	cent->pe.torso.pitching = qfalse;

	memset ( &cent->pe.nonseg, 0, sizeof ( cent->pe.nonseg ) );
	cent->pe.nonseg.yawAngle = cent->rawAngles[ YAW ];
	cent->pe.nonseg.yawing = qfalse;
	cent->pe.nonseg.pitchAngle = cent->rawAngles[ PITCH ];
	cent->pe.nonseg.pitching = qfalse;

	if ( cg_debugPosition.integer )
	{
		CG_Printf ( "%i ResetPlayerEntity yaw=%.2f\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
}

/*
==================
CG_PlayerDisconnect

Player disconnecting
==================
*/
void CG_PlayerDisconnect ( vec3_t org )
{
	particleSystem_t *ps;

	trap_S_StartSound ( org, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.disconnectSound );

	ps = CG_SpawnNewParticleSystem ( cgs.media.disconnectPS );

	if ( CG_IsParticleSystemValid ( &ps ) )
	{
		CG_SetAttachmentPoint ( &ps->attachment, org );
		CG_AttachToPoint ( &ps->attachment );
	}
}

/*
=================
CG_Bleed

This is the spurt of blood when a character gets hit
=================
*/
void CG_Bleed ( vec3_t origin, vec3_t normal, int entityNum )
{
	pTeam_t          team = cgs.clientinfo[ entityNum ].team;
	qhandle_t        bleedPS;
	particleSystem_t *ps;

	if ( !cg_blood.integer )
	{
		return;
	}

	if ( team == PTE_ALIENS )
	{
		bleedPS = cgs.media.alienBleedPS;
	}
	else if ( team == PTE_HUMANS )
	{
		bleedPS = cgs.media.humanBleedPS;
	}
	else
	{
		return;
	}

	ps = CG_SpawnNewParticleSystem ( bleedPS );

	if ( CG_IsParticleSystemValid ( &ps ) )
	{
		CG_SetAttachmentPoint ( &ps->attachment, origin );
		CG_SetAttachmentCent ( &ps->attachment, &cg_entities[ entityNum ] );
		CG_AttachToPoint ( &ps->attachment );

		CG_SetParticleSystemNormal ( ps, normal );
	}
}

/*
===============
CG_AtHighestClass

Is the local client at the highest class possible?
===============
*/
qboolean CG_AtHighestClass ( void )
{
	int      i;
	qboolean superiorClasses = qfalse;

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		if ( BG_ClassCanEvolveFromTo (
		       cg.predictedPlayerState.stats[ STAT_PCLASS ], i,
		       ALIEN_MAX_KILLS, 0 ) >= 0 &&
		     BG_FindStagesForClass ( i, cgs.alienStage ) &&
		     BG_ClassIsAllowed ( i ) )
		{
			superiorClasses = qtrue;
			break;
		}
	}

	return !superiorClasses;
}
