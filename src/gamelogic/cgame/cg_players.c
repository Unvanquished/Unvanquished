/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

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

// debugging
int   debug_anim_current;
int   debug_anim_old;
float debug_anim_blend;

static refSkeleton_t legsSkeleton;
static refSkeleton_t torsoSkeleton;
static refSkeleton_t oldSkeleton;
static refSkeleton_t jetpackSkeleton;

typedef struct {
	vec3_t delta;
	quat_t rot;
}

delta_t;

delta_t deltas[ WP_NUM_WEAPONS ][ MAX_BONES ];

static const char *const cg_customSoundNames[ MAX_CUSTOM_SOUNDS ] =
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
sfxHandle_t CG_CustomSound( int clientNum, const char *soundName )
{
	clientInfo_t *ci;
	int          i;

	if ( soundName[ 0 ] != '*' )
	{
		return trap_S_RegisterSound( soundName, qfalse );
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS )
	{
		clientNum = 0;
	}

	ci = &cgs.clientinfo[ clientNum ];

	for ( i = 0; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[ i ]; i++ )
	{
		if ( !strcmp( soundName, cg_customSoundNames[ i ] ) )
		{
			return ci->sounds[ i ];
		}
	}

	CG_Error( "Unknown custom sound: %s", soundName );
	return 0;
}

/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_ParseCharacterFile

Read a configuration file containing body.md5mesh custom
models/players/visor/character.cfg, etc
======================
*/

static qboolean CG_ParseCharacterFile( const char *filename, clientInfo_t *ci )
{
	char         *text_p;
	int          len;
	int          i;
	char         *token;
	char         text[ 20000 ];
	fileHandle_t f;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len <= 0 )
	{
		return qfalse;
	}

	if ( len + 1 >= (int) sizeof( text ) )
	{
		CG_Printf( "File %s is too long\n", filename );
		trap_FS_FCloseFile( f );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	ci->footsteps = FOOTSTEP_GENERAL;
	VectorClear( ci->headOffset );
	ci->gender = GENDER_MALE;
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;
	ci->numLegBones = 0;
	ci->modelScale = 1.0f;
	ci->leftShoulderBone = 0;
	ci->rightShoulderBone = 0;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse2( &text_p );

		if ( !token[ 0 ] )
		{
			break;
		}

		if(!Q_stricmp(token, "footsteps"))
		{
			token = COM_Parse(&text_p);
			if(!token)
			{
				break;
			}
			if( !Q_stricmp( token, "default" ) )
			{
				ci->footsteps = FOOTSTEP_GENERAL;
			}
			else if( !Q_stricmp( token, "flesh") )
			{
				ci->footsteps = FOOTSTEP_FLESH;
			}
			else if( !Q_stricmp( token, "metal") )
			{
				ci->footsteps = FOOTSTEP_METAL;
			}
			else if( !Q_stricmp(token, "splash") )
			{
				ci->footsteps = FOOTSTEP_SPLASH;
			}
			else if( !Q_stricmp(token, "none") )
			{
				ci->footsteps = FOOTSTEP_NONE;
			}
			else
			{
				CG_Printf( "Bad footsteps parm in %s: %s\n", filename, token);
			}
			continue;
		}
		else if ( !Q_stricmp( token, "headoffset" ) )
		{
			for ( i = 0; i < 3; i++ )
			{
				token = COM_Parse2( &text_p );

				if ( !token )
				{
					break;
				}

				ci->headOffset[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "sex" ) )
		{
			token = COM_Parse2( &text_p );

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
		else if ( !Q_stricmp( token, "fixedlegs" ) )
		{
			ci->fixedlegs = qtrue;
			continue;
		}
		else if ( !Q_stricmp( token, "fixedtorso" ) )
		{
			ci->fixedtorso = qtrue;
			continue;
		}
		else if ( !Q_stricmp( token, "modelScale" ) )
		{
			token = COM_ParseExt2( &text_p, qfalse );

			if ( token )
			{
				ci->modelScale = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "torsoControlBone" ) )
		{
			token = COM_Parse2( &text_p );

			ci->torsoControlBone = trap_R_BoneIndex( ci->bodyModel, token );
			continue;
		}
		else if ( !Q_stricmp( token, "leftShoulder" ) )
		{
			token = COM_Parse2( &text_p );

			ci->leftShoulderBone = trap_R_BoneIndex( ci->bodyModel, token );
			continue;
		}
		else if ( !Q_stricmp( token, "rightShoulder" ) )
		{
			token = COM_Parse2( &text_p );

			ci->rightShoulderBone = trap_R_BoneIndex( ci->bodyModel, token );
			continue;
		}
		else if ( !Q_stricmp( token, "handBones" ) )
		{
			token = COM_Parse2( &text_p );

			if ( token[0] != '{' )
			{
				Com_Printf( "^1ERROR^7: Expected '{' but found '%s' in character.cfg", token );
			}

			i = 0;

			while( 1 )
			{
				token = COM_Parse2( &text_p );

				if ( !token || token[ 0 ] == '}' )
				{
					ci->numHandBones = i;
					break;
				}

				ci->handBones[ i++ ] = trap_R_BoneIndex( ci->bodyModel, token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "legBones" ) )
		{
			token = COM_Parse2( &text_p );

			if ( token[0] != '{' )
			{
				Com_Printf( "^1ERROR^7: Expected '{' but found '%s' in character.cfg\n", token );
			}

			i = 0;

			while( 1 )
			{
				token = COM_Parse2( &text_p );

				if ( !token || token[ 0 ] == '}' )
				{
					ci->numLegBones = i;
					break;
				}

				ci->legBones[ i++ ] = trap_R_BoneIndex( ci->bodyModel, token );
			}

			continue;
		}

		Com_Printf( "unknown token '%s' is %s\n", token, filename );
	}

	return qtrue;
}

static qboolean CG_RegisterPlayerAnimation( clientInfo_t *ci, const char *modelName, int anim, const char *animName,
                                            qboolean loop, qboolean reversed, qboolean clearOrigin, qboolean iqm )
{
	char filename[ MAX_QPATH ], newModelName[ MAX_QPATH ];
	int  frameRate;

	// special handling for human_(naked|light|medium)
	if ( !Q_stricmp( modelName, "human_naked"   ) ||
	     !Q_stricmp( modelName, "human_light"   ) ||
	     !Q_stricmp( modelName, "human_medium" ) )
	{
		Q_strncpyz( newModelName, "human_nobsuit_common", sizeof( newModelName ) );
	}
	else
	{
		Q_strncpyz( newModelName, modelName, sizeof( newModelName ) );
	}

	if ( iqm )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/%s.iqm:%s",
		     newModelName, newModelName, animName );
		ci->animations[ anim ].handle = trap_R_RegisterAnimation( filename );
	}
	else
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/%s.md5anim", newModelName, animName );
		ci->animations[ anim ].handle = trap_R_RegisterAnimation( filename );
	}

	if ( !ci->animations[ anim ].handle )
	{
		Com_Printf( "Failed to load animation file %s\n", filename );
		return qfalse;
	}

	ci->animations[ anim ].firstFrame = 0;
	ci->animations[ anim ].numFrames = trap_R_AnimNumFrames( ci->animations[ anim ].handle );
	frameRate = trap_R_AnimFrameRate( ci->animations[ anim ].handle );

	if ( frameRate == 0 )
	{
		frameRate = 1;
	}

	ci->animations[ anim ].frameLerp = 1000 / frameRate;
	ci->animations[ anim ].initialLerp = 1000 / frameRate;

	if ( loop )
	{
		ci->animations[ anim ].loopFrames = ci->animations[ anim ].numFrames;
	}
	else
	{
		ci->animations[ anim ].loopFrames = 0;
	}

	ci->animations[ anim ].reversed = reversed;
	ci->animations[ anim ].clearOrigin = clearOrigin;

	return qtrue;
}

static qboolean CG_DeriveAnimationDelta( const char *modelName, weapon_t weapon, clientInfo_t *ci, qboolean iqm )
{
	int handle, i;
	static refSkeleton_t base, delta;

	if ( iqm )
	{
		handle = trap_R_RegisterAnimation( va( "models/players/%s/%s.iqm:%s_delta", modelName, modelName, BG_Weapon( weapon )->name ) );

	}
	else
	{
		handle = trap_R_RegisterAnimation( va( "models/players/%s/%s_delta.md5anim", modelName, BG_Weapon( weapon )->name ) );
	}

	if ( !handle )
	{
		return qfalse;
	}

	ci->weaponAdjusted |= 1 << weapon;

	trap_R_BuildSkeleton( &delta, handle, 1, 1, 0, qfalse );
	trap_R_BuildSkeleton( &base, ci->animations[ TORSO_STAND ].handle, 1, 1, 0, qfalse );

	for ( i = 0; i < ci->numHandBones; i++ )
	{
		VectorSubtract( delta.bones[ ci->handBones[ i ] ].t.trans, base.bones[ ci->handBones[ i ] ].t.trans, deltas[ weapon ][ ci->handBones[ i ] ].delta );

		QuatInverse( base.bones[ ci->handBones[ i ] ].t.rot );

		QuatMultiply1( base.bones[ ci->handBones[ i ] ].t.rot, delta.bones[ ci->handBones[ i ] ].t.rot, deltas[ weapon ][ ci->handBones[ i ] ].rot );
	}

	return qtrue;
}

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation counts and rates
(models/players/visor/animation.cfg, etc.)
======================
*/
static qboolean CG_ParseAnimationFile( const char *filename, clientInfo_t *ci )
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
	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len < 0 )
	{
		return qfalse;
	}

	if ( len == 0 || len + 1 >= (int) sizeof( text ) )
	{
		CG_Printf( len == 0 ? "File %s is empty\n" : "File %s is too long\n", filename );
		trap_FS_FCloseFile( f );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;
	skip = 0; // quite the compiler warning

	ci->footsteps = FOOTSTEP_GENERAL;
	VectorClear( ci->headOffset );
	ci->gender = GENDER_MALE;
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;
	ci->nonsegmented = qfalse;

	// read optional parameters
	while ( 1 )
	{
		prev = text_p; // so we can unget
		token = COM_Parse2( &text_p );

		if ( !token )
		{
			break;
		}

		if ( !Q_stricmp( token, "footsteps" ) )
		{
			token = COM_Parse2( &text_p );

			if ( !token )
			{
				break;
			}

			if ( !Q_stricmp( token, "default" ) )
			{
				ci->footsteps = FOOTSTEP_GENERAL;
			}
			else if ( !Q_stricmp( token, "flesh" ) )
			{
				ci->footsteps = FOOTSTEP_FLESH;
			}
			else if ( !Q_stricmp( token, "none" ) )
			{
				ci->footsteps = FOOTSTEP_NONE;
			}
			else if ( !Q_stricmp( token, "custom" ) )
			{
				ci->footsteps = FOOTSTEP_CUSTOM;
			}
			else
			{
				CG_Printf( "Bad footsteps parm in %s: %s\n", filename, token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "headoffset" ) )
		{
			for ( i = 0; i < 3; i++ )
			{
				token = COM_Parse2( &text_p );

				if ( !token )
				{
					break;
				}

				ci->headOffset[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "sex" ) )
		{
			token = COM_Parse2( &text_p );

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
		else if ( !Q_stricmp( token, "fixedlegs" ) )
		{
			ci->fixedlegs = qtrue;
			continue;
		}
		else if ( !Q_stricmp( token, "fixedtorso" ) )
		{
			ci->fixedtorso = qtrue;
			continue;
		}
		else if ( !Q_stricmp( token, "nonsegmented" ) )
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

		Com_Printf( "unknown token '%s' is %s\n", token, filename );
	}

	if ( !ci->nonsegmented )
	{
		// read information for each frame
		for ( i = 0; i < MAX_PLAYER_ANIMATIONS; i++ )
		{
			token = COM_Parse2( &text_p );

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

			animations[ i ].firstFrame = atoi( token );

			// leg only frames are adjusted to not count the upper body only frames
			if ( i == LEGS_WALKCR )
			{
				skip = animations[ LEGS_WALKCR ].firstFrame - animations[ TORSO_GESTURE ].firstFrame;
			}

			if ( i >= LEGS_WALKCR && i < TORSO_GETFLAG )
			{
				animations[ i ].firstFrame -= skip;
			}

			token = COM_Parse2( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].numFrames = atoi( token );
			animations[ i ].reversed = qfalse;
			animations[ i ].flipflop = qfalse;

			// if numFrames is negative the animation is reversed
			if ( animations[ i ].numFrames < 0 )
			{
				animations[ i ].numFrames = -animations[ i ].numFrames;
				animations[ i ].reversed = qtrue;
			}

			token = COM_Parse2( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].loopFrames = atoi( token );

			token = COM_Parse2( &text_p );

			if ( !*token )
			{
				break;
			}

			fps = atof( token );

			if ( fps == 0 )
			{
				fps = 1;
			}

			animations[ i ].frameLerp = 1000 / fps;
			animations[ i ].initialLerp = 1000 / fps;
		}

		if ( i != MAX_PLAYER_ANIMATIONS )
		{
			CG_Printf( "Error parsing animation file: %s\n", filename );
			return qfalse;
		}

		// crouch backward animation
		animations[ LEGS_BACKCR ] = animations[ LEGS_WALKCR ];
		animations[ LEGS_BACKCR ].reversed = qtrue;
		// walk backward animation
		animations[ LEGS_BACKWALK ] = animations[ LEGS_WALK ];
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
			token = COM_Parse2( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].firstFrame = atoi( token );

			token = COM_Parse2( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].numFrames = atoi( token );
			animations[ i ].reversed = qfalse;
			animations[ i ].flipflop = qfalse;

			// if numFrames is negative the animation is reversed
			if ( animations[ i ].numFrames < 0 )
			{
				animations[ i ].numFrames = -animations[ i ].numFrames;
				animations[ i ].reversed = qtrue;
			}

			token = COM_Parse2( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].loopFrames = atoi( token );

			token = COM_Parse2( &text_p );

			if ( !*token )
			{
				break;
			}

			fps = atof( token );

			if ( fps == 0 )
			{
				fps = 1;
			}

			animations[ i ].frameLerp = 1000 / fps;
			animations[ i ].initialLerp = 1000 / fps;
		}

		if ( i != MAX_NONSEG_PLAYER_ANIMATIONS )
		{
			CG_Printf( "Error parsing animation file: %s\n", filename );
			return qfalse;
		}

		// walk backward animation
		animations[ NSPA_WALKBACK ] = animations[ NSPA_WALK ];
		animations[ NSPA_WALKBACK ].reversed = qtrue;
	}

	return qtrue;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static qboolean CG_RegisterClientSkin( clientInfo_t *ci, const char *modelName, const char *skinName )
{
	char filename[ MAX_QPATH ];

	if ( ci->md5 )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/body_%s.skin", modelName, skinName );
		ci->bodySkin = trap_R_RegisterSkin( filename );

		if ( !ci->bodySkin )
		{
			Com_Printf( "MD5 skin load failure: %s\n", filename );
		}

		if ( !ci->bodySkin )
		{
			return qfalse;
		}
	}
	else if ( !ci->nonsegmented )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower_%s.skin", modelName, skinName );
		ci->legsSkin = trap_R_RegisterSkin( filename );

		if ( !ci->legsSkin )
		{
			Com_Printf( "Leg skin load failure: %s\n", filename );
		}

		Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper_%s.skin", modelName, skinName );
		ci->torsoSkin = trap_R_RegisterSkin( filename );

		if ( !ci->torsoSkin )
		{
			Com_Printf( "Torso skin load failure: %s\n", filename );
		}

		Com_sprintf( filename, sizeof( filename ), "models/players/%s/head_%s.skin", modelName, skinName );
		ci->headSkin = trap_R_RegisterSkin( filename );

		if ( !ci->headSkin )
		{
			Com_Printf( "Head skin load failure: %s\n", filename );
		}

		if ( !ci->legsSkin || !ci->torsoSkin || !ci->headSkin )
		{
			return qfalse;
		}
	}
	else
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/nonseg_%s.skin", modelName, skinName );
		ci->nonSegSkin = trap_R_RegisterSkin( filename );

		if ( !ci->nonSegSkin )
		{
			Com_Printf( "Non-segmented skin load failure: %s\n", filename );
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
static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName )
{
	char filename[ MAX_QPATH ];
	qboolean iqm = qfalse;

	if ( cg_highPolyPlayerModels.integer )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/%s.iqm",
			     modelName, modelName );
		ci->bodyModel = trap_R_RegisterModel( filename );

		if ( ! ci->bodyModel ) {
			Com_sprintf( filename, sizeof( filename ), "models/players/%s/body.md5mesh", modelName );
			ci->bodyModel = trap_R_RegisterModel( filename );
		}
		else
		{
			iqm = qtrue;
		}

		if ( ci->bodyModel )
		{
			ci->md5 = qtrue;
		}
		else
		{
			ci->md5 = qfalse;
		}
	}

	if ( ci->md5 )
	{
		int i, j;
		// load the animations
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/character.cfg", modelName );

		if ( !CG_ParseCharacterFile( filename, ci ) )
		{
			Com_Printf( "Failed to load character file %s\n", filename );
			return qfalse;
		}

		ci->weaponAdjusted = 0;

		// If model is not an alien, load human animations
		if ( ci->gender != GENDER_NEUTER )
		{
			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_IDLE, "idle", qtrue, qfalse, qfalse, iqm ) )
			{
				Com_Printf( "Failed to load idle animation file %s\n", filename );
				return qfalse;
			}

			// make LEGS_IDLE the default animation
			for ( i = 0; i < MAX_PLAYER_ANIMATIONS; i++ )
			{
				if ( i == LEGS_IDLE )
				{
					continue;
				}

				ci->animations[ i ] = ci->animations[ LEGS_IDLE ];
			}

			// FIXME fail register of the entire model if one of these animations is missing

			// FIXME add death animations

			if ( !CG_RegisterPlayerAnimation( ci, modelName, BOTH_DEATH1, "die", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ BOTH_DEATH1 ] = ci->animations[ LEGS_IDLE ];
			}

			if( !CG_RegisterPlayerAnimation(ci, modelName, BOTH_DEATH2, "death2", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ BOTH_DEATH2 ] = ci->animations[ BOTH_DEATH1 ];
			}

			if( !CG_RegisterPlayerAnimation(ci, modelName, BOTH_DEATH3, "death3", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ BOTH_DEATH3 ] = ci->animations[ BOTH_DEATH1 ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_GESTURE, "gesture", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ TORSO_GESTURE ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_RALLY, "rally", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ TORSO_RALLY ] = ci->animations[ LEGS_IDLE ];
			}


			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_ATTACK, "attack", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ TORSO_ATTACK ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_ATTACK_BLASTER, "blaster_attack", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ TORSO_ATTACK_BLASTER ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_ATTACK_PSAW, "psaw_attack", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ TORSO_ATTACK_BLASTER ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_STAND, "stand", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ TORSO_STAND ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_STAND_BLASTER, "stand_blaster", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ TORSO_STAND_BLASTER ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_IDLECR, "crouch", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_IDLECR ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_WALKCR, "crouch_forward", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_WALKCR ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_BACKCR, "crouch_back", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_BACKCR ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_WALK, "walk", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_WALK ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_BACKWALK, "walk_back", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_BACKWALK ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_RUN, "run", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_RUN ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_BACK, "run_back", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_BACK ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_SWIM, "swim", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_SWIM ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_JUMP, "jump", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_JUMP ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_LAND, "land", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_LAND ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_JUMPB, "jump_back", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_JUMPB ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_LANDB, "land_back", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_LANDB ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_TURN, "step", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ LEGS_TURN ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_DROP, "drop", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ TORSO_DROP ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_RAISE, "raise", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ TORSO_RAISE ] = ci->animations[ LEGS_IDLE ];
			}

			// TODO: Don't assume WP_BLASTER is first human weapon
			for ( i = TORSO_GESTURE_BLASTER, j = WP_BLASTER; i <= TORSO_GESTURE_CKIT; i++, j++ )
			{
				if ( i == TORSO_GESTURE ) { continue; }
				if ( i == TORSO_GESTURE_CKIT ) { j = WP_HBUILD; }

				if ( !CG_RegisterPlayerAnimation( ci, modelName, i, va( "%s_taunt", BG_Weapon( j )->name ), qfalse, qfalse, qfalse, iqm ) )
				{
					ci->animations[ i ] = ci->animations[ TORSO_GESTURE ];
				}
			}

			// TODO: Don't assume WP_BLASTER is first human weapon
			for ( i = WP_BLASTER; i < WP_NUM_WEAPONS; i++ )
			{
				if ( BG_Weapon( i )->team != TEAM_HUMANS || !BG_Weapon( i )->purchasable ) { continue; }
				CG_DeriveAnimationDelta( modelName, (weapon_t)i, ci, iqm );
			}

		}
		else
		{
			// Load Alien animations
			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_STAND, "stand", qtrue, qfalse, qfalse, iqm ) )
			{
				Com_Printf( "Failed to load standing animation file %s\n", filename );
				return qfalse;
			}

			// make LEGS_IDLE the default animation
			for ( i = 0; i < MAX_NONSEG_PLAYER_ANIMATIONS; i++ )
			{
				if ( i == NSPA_STAND )
				{
					continue;
				}

				ci->animations[ i ] = ci->animations[ NSPA_STAND ];
			}

			// FIXME fail register of the entire model if one of these animations is missing

			// FIXME add death animations

			CG_RegisterPlayerAnimation( ci, modelName, NSPA_DEATH1, "die", qfalse, qfalse, qfalse, iqm );

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_DEATH2, "die2", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_DEATH2 ] = ci->animations[ NSPA_DEATH1 ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_DEATH3, "die3", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_DEATH3 ] = ci->animations[ NSPA_DEATH1 ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_GESTURE, "gesture", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_GESTURE ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_ATTACK1, "attack", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_ATTACK1 ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_ATTACK2, "attack2", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_ATTACK2 ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_ATTACK3, "attack3", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_ATTACK3 ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_RUN, "run", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_RUN ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_RUNBACK, "run_backwards", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_RUNBACK ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_RUNLEFT, "run_left", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_RUNLEFT ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_RUNRIGHT, "run_right", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_RUNRIGHT ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_WALK, "walk", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_WALK ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_WALKBACK, "walk", qtrue, qtrue, qfalse, iqm ) )
			{
				ci->animations[ NSPA_WALKBACK ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_WALKLEFT, "walk_left", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_WALKLEFT ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_WALKRIGHT, "walk_right", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_WALKRIGHT ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_SWIM, "swim", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_SWIM ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_JUMP, "jump", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_JUMP ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_JUMPBACK, "jump_back", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_JUMPBACK ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_LAND, "land", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_LAND ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_LANDBACK, "land_back", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_LANDBACK ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_TURN, "turn", qtrue, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_TURN ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_PAIN1, "pain1", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_PAIN1 ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_PAIN2, "pain2", qfalse, qfalse, qfalse, iqm ) )
			{
				ci->animations[ NSPA_PAIN2 ] = ci->animations[ NSPA_STAND ];
			}

			if ( !Q_stricmp( modelName, "level4" ) )
			{
				if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_CHARGE, "charge", qtrue, qfalse, qfalse, iqm ) )
				{
					ci->animations[ NSPA_CHARGE ] = ci->animations[ NSPA_STAND ];
				}
			}
		}

		// if any skins failed to load, return failure
		if ( !CG_RegisterClientSkin( ci, modelName, skinName ) )
		{
			Com_Printf( "Failed to load skin file: %s : %s\n", modelName, skinName );
			return qfalse;
		}

		return qtrue;
	}

	// do this first so the nonsegmented property is set
	// load the animations
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", modelName );

	if ( !CG_ParseAnimationFile( filename, ci ) )
	{
		Com_Printf( "Failed to load animation file %s\n", filename );
		return qfalse;
	}

	// load cmodels before models so filecache works
	if ( !ci->nonsegmented )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", modelName );
		ci->legsModel = trap_R_RegisterModel( filename );

		if ( !ci->legsModel )
		{
			Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}

		Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", modelName );
		ci->torsoModel = trap_R_RegisterModel( filename );

		if ( !ci->torsoModel )
		{
			Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}

		Com_sprintf( filename, sizeof( filename ), "models/players/%s/head.md3", modelName );
		ci->headModel = trap_R_RegisterModel( filename );

		if ( !ci->headModel )
		{
			Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}
	}
	else
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/nonseg.md3", modelName );
		ci->nonSegModel = trap_R_RegisterModel( filename );

		if ( !ci->nonSegModel )
		{
			Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}
	}

	// if any skins failed to load, return failure
	if ( !CG_RegisterClientSkin( ci, modelName, skinName ) )
	{
		Com_Printf( "Failed to load skin file: %s : %s\n", modelName, skinName );
		return qfalse;
	}

	return qtrue;
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits
===================
*/
static void CG_LoadClientInfo( clientInfo_t *ci )
{
	const char *dir;
	int        i;
	const char *s;
	int        clientNum;

	if ( !CG_RegisterClientModelname( ci, ci->modelName, ci->skinName ) )
	{
		CG_Error( "CG_RegisterClientModelname( %s, %s ) failed", ci->modelName, ci->skinName );
	}

	// sounds
	dir = ci->modelName;

	for ( i = 0; i < MAX_CUSTOM_SOUNDS; i++ )
	{
		s = cg_customSoundNames[ i ];

		if ( !s )
		{
			break;
		}

		// fanny about a bit with sounds that are missing
		if ( !CG_FileExists( va( "sound/player/%s/%s", dir, s + 1 ) ) )
		{
			//file doesn't exist

			if ( i == 11 || i == 8 ) //fall or falling
			{
				ci->sounds[ i ] = trap_S_RegisterSound( "sound/null.wav", qfalse );
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

				ci->sounds[ i ] = trap_S_RegisterSound( va( "sound/player/%s/%s", dir, s + 1 ), qfalse );
			}
		}
		else
		{
			ci->sounds[ i ] = trap_S_RegisterSound( va( "sound/player/%s/%s", dir, s + 1 ), qfalse );
		}
	}

	if ( ci->footsteps == FOOTSTEP_CUSTOM )
	{
		for ( i = 0; i < 4; i++ )
		{
			ci->customFootsteps[ i ] = trap_S_RegisterSound( va( "sound/player/%s/step%d.wav", dir, i + 1 ), qfalse );

			if ( !ci->customFootsteps[ i ] )
			{
				ci->customFootsteps[ i ] = trap_S_RegisterSound( va( "sound/player/footsteps/step%d.wav", i + 1 ), qfalse );
			}

			ci->customMetalFootsteps[ i ] = trap_S_RegisterSound( va( "sound/player/%s/clank%d.wav", dir, i + 1 ), qfalse );

			if ( !ci->customMetalFootsteps[ i ] )
			{
				ci->customMetalFootsteps[ i ] = trap_S_RegisterSound( va( "sound/player/footsteps/clank%d.wav", i + 1 ), qfalse );
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
			CG_ResetPlayerEntity( &cg_entities[ i ] );
		}
	}
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to )
{
	VectorCopy( from->headOffset, to->headOffset );
	to->modelScale = from->modelScale;
	to->footsteps = from->footsteps;
	to->gender = from->gender;

	to->numLegBones = from->numLegBones;
	to->numHandBones = from->numHandBones;
	to->torsoControlBone = from->torsoControlBone;
	to->weaponAdjusted = from->weaponAdjusted;
	to->rightShoulderBone = from->rightShoulderBone;
	to->leftShoulderBone = from->leftShoulderBone;

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
	to->bodyModel = from->bodyModel;
	to->bodySkin = from->bodySkin;
	to->md5 = from->md5;

	memcpy( to->animations, from->animations, sizeof( to->animations ) );
	memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );
	memcpy( to->customFootsteps, from->customFootsteps, sizeof( to->customFootsteps ) );
	memcpy( to->customMetalFootsteps, from->customMetalFootsteps, sizeof( to->customMetalFootsteps ) );
	memcpy( to->legBones, from->legBones, sizeof( to->legBones ) );
	memcpy( to->handBones, from->handBones, sizeof( to->legBones ) );
}

/*
======================
CG_GetCorpseNum
======================
*/
static int CG_GetCorpseNum( class_t class_ )
{
	int          i;
	clientInfo_t *match;
	char         *modelName;
	char         *skinName;

	modelName = BG_ClassModelConfig( class_ )->modelName;
	skinName = BG_ClassModelConfig( class_ )->skinName;

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		match = &cgs.corpseinfo[ i ];

		if ( !match->infoValid )
		{
			continue;
		}

		if ( !Q_stricmp( modelName, match->modelName ) &&
		     !Q_stricmp( skinName, match->skinName ) )
		{
			// this clientinfo is identical, so use its handles
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
static qboolean CG_ScanForExistingClientInfo( clientInfo_t *ci )
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

		if ( !Q_stricmp( ci->modelName, match->modelName ) &&
		     !Q_stricmp( ci->skinName, match->skinName ) )
		{
			// this clientinfo is identical, so use its handles
			CG_CopyClientInfoModel( match, ci );

			return qtrue;
		}
	}

	// shouldn't happen
	return qfalse;
}

/*
======================
CG_PrecacheClientInfo
======================
*/
void CG_PrecacheClientInfo( class_t class_, const char *model, const char *skin )
{
	clientInfo_t *ci;
	clientInfo_t newInfo;

	ci = &cgs.corpseinfo[ class_ ];

	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );

	// model
	Q_strncpyz( newInfo.modelName, model, sizeof( newInfo.modelName ) );

	// modelName did not include a skin name
	if ( !skin )
	{
		Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
	}
	else
	{
		Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
	}

	newInfo.infoValid = qtrue;

	// actually register the models
	*ci = newInfo;
	CG_LoadClientInfo( ci );
}

/*
=============
CG_StatusMessages

Print messages for player status changes
=============
*/
static void CG_StatusMessages( clientInfo_t *new_, clientInfo_t *old )
{
	if ( !old->infoValid )
	{
		return;
	}

	if ( strcmp( new_->name, old->name ) )
	{
		CG_Printf(_( "%s^7 renamed to %s\n"), old->name, new_->name );
	}

	if ( old->team != new_->team )
	{
		if ( new_->team == TEAM_NONE )
		{
			CG_Printf(_( "%s^7 left the %s\n"), new_->name,
			           BG_TeamNamePlural( old->team ) );
		}
		else if ( old->team == TEAM_NONE )
		{
			CG_Printf(_( "%s^7 joined the %s\n"), new_->name,
			           BG_TeamNamePlural( new_->team ) );
		}
		else
		{
			CG_Printf(_( "%s^7 left the %s and joined the %s\n"),
			           new_->name, BG_TeamNamePlural( old->team ), BG_TeamNamePlural( new_->team ) );
		}
	}
}

/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo( int clientNum )
{
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char   *configstring;
	const char   *v;
	char         *slash;

	ci = &cgs.clientinfo[ clientNum ];

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );

	if ( !configstring[ 0 ] )
	{
		memset( ci, 0, sizeof( *ci ) );
		return; // player just left
	}

	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );

	// grab our own ignoreList
	if ( clientNum == cg.predictedPlayerState.clientNum )
	{
		v = Info_ValueForKey( configstring, "ig" );
		Com_ClientListParse( &cgs.ignoreList, v );
	}

	// isolate the player's name
	v = Info_ValueForKey( configstring, "n" );
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );

	// team
	v = Info_ValueForKey( configstring, "t" );
	newInfo.team = (team_t) atoi( v );

	// model
	v = Info_ValueForKey( configstring, "model" );
	Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );

	slash = strchr( newInfo.modelName, '/' );

	if ( !slash )
	{
		// modelName didn not include a skin name
		Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
	}
	else
	{
		Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
		// truncate modelName
		*slash = 0;
	}

	// voice
	v = Info_ValueForKey( configstring, "v" );
	Q_strncpyz( newInfo.voice, v, sizeof( newInfo.voice ) );

	CG_StatusMessages( &newInfo, ci );

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci = newInfo;

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo( ci ) )
	{
		CG_LoadClientInfo( ci );
	}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/

static void CG_CombineLegSkeleton( refSkeleton_t *dest, refSkeleton_t *legs, int *legBones, int numBones )
{
	int i;

	for ( i = 0; i < numBones; i++ )
	{
		dest->bones[ legBones[ i ] ] = legs->bones[ legBones[ i ] ];
	}
}



/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, refSkeleton_t *skel )
{
	animation_t *anim;
	lf->old_animationNumber = lf->animationNumber;
	lf->old_animation = lf->animation;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_PLAYER_TOTALANIMATIONS )
	{
		CG_Error( "Bad animation number: %i", newAnimation );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;

	if ( ci->md5 )
	{
		debug_anim_current = lf->animationNumber;
		debug_anim_old = lf->old_animationNumber;

		if ( lf->old_animationNumber <= 0 )
		{
			// skip initial / invalid blending
			lf->blendlerp = 0.0f;
			return;
		}

		if ( lf->old_animationNumber <= 0 )
		{
			// skip initial / invalid blending
			lf->blendlerp = 0.0f;
			return;
		}

		// TODO: blend through two blendings!

		if ( ( lf->blendlerp <= 0.0f ) )
		{
			lf->blendlerp = 1.0f;
		}
		else
		{
			lf->blendlerp = 1.0f - lf->blendlerp; // use old blending for smooth blending between two blended animations
		}

		oldSkeleton = *skel;

		//Com_Printf(_("new: %i old %i\n"), newAnimation,lf->old_animationNumber);

		if ( lf->old_animation->handle && oldSkeleton.numBones == skel->numBones )
		{
			if ( !trap_R_BuildSkeleton( &oldSkeleton, lf->old_animation->handle, lf->oldFrame, lf->frame, lf->blendlerp, lf->old_animation->clearOrigin ) )
			{
				CG_Printf( "Can't blend skeleton\n" );
				return;
			}
		}

		lf->animationTime = cg.time + anim->initialLerp;

		lf->oldFrame = lf->frame = 0;
		lf->oldFrameTime = lf->frameTime = 0;
	}
	else
	{
		lf->animationTime = lf->frameTime + anim->initialLerp;
	}

	if ( cg_debugAnim.integer )
	{
		CG_Printf( "Anim: %i\n", newAnimation );
	}

}

/*
===============
CG_RunPlayerLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunPlayerLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, refSkeleton_t *skel, float speedScale )
{
	qboolean animChanged = qfalse;

    // see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation )
	{
		CG_SetLerpFrameAnimation( ci, lf, newAnimation, skel );
		animChanged = qtrue;
	}

	CG_RunLerpFrame( lf, speedScale );

	if ( ci->md5 )
	{
		CG_RunMD5LerpFrame( lf, speedScale, animChanged );

		// blend old and current animation
		CG_BlendLerpFrame( lf );

		if( ci->team != TEAM_NONE )
			CG_BuildAnimSkeleton( lf, skel, &oldSkeleton );
	}
}

static void CG_RunCorpseLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation )
{
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
		CG_SetLerpFrameAnimation( ci, lf, newAnimation, NULL );

		if ( !lf->animation )
		{
			oldSkeleton = legsSkeleton;
		}
	}

	anim = lf->animation;

	if ( !anim || !anim->frameLerp )
	{
		return; // shouldn't happen
	}

	// blend old and current animation
	CG_BlendLerpFrame( lf );

	if ( lf->animation )
	{
		if ( !trap_R_BuildSkeleton( &legsSkeleton, lf->animation->handle, anim->numFrames, anim->numFrames, 0, lf->animation->clearOrigin ) )
		{
			CG_Printf( "Can't build lf->skeleton\n" );
		}
	}
}



/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber, refSkeleton_t *skel )
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation( ci, lf, animationNumber, skel );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}

/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
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
		CG_RunPlayerLerpFrame( ci, &cent->pe.legs, LEGS_TURN, NULL, speedScale );
	}
	else
	{
		CG_RunPlayerLerpFrame( ci, &cent->pe.legs, cent->currentState.legsAnim, NULL, speedScale );
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunPlayerLerpFrame( ci, &cent->pe.torso, cent->currentState.torsoAnim, NULL, speedScale );

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

/*
===============
CG_PlayerNonSegAnimation
===============
*/
static void CG_PlayerNonSegAnimation( centity_t *cent, int *nonSegOld,
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
		CG_RunPlayerLerpFrame( ci, &cent->pe.nonseg, NSPA_TURN, NULL, speedScale );
	}
	else
	{
		CG_RunPlayerLerpFrame( ci, &cent->pe.nonseg, cent->currentState.legsAnim, NULL, speedScale );
	}

	*nonSegOld = cent->pe.nonseg.oldFrame;
	*nonSeg = cent->pe.nonseg.frame;
	*nonSegBackLerp = cent->pe.nonseg.backlerp;
}

/*
===============
CG_PlayerMD5Animation
===============
*/

static void CG_PlayerMD5Animation( centity_t *cent )
{
	clientInfo_t *ci;
	int          clientNum;
	float        speedScale;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer )
	{
		return;
	}

	speedScale = 1;

	ci = &cgs.clientinfo[ clientNum ];

	// do the shuffle turn frames locally
	if ( cent->pe.legs.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE )
	{
		CG_RunPlayerLerpFrame( ci, &cent->pe.legs, LEGS_TURN, &legsSkeleton, speedScale );
	}
	else
	{
		CG_RunPlayerLerpFrame( ci, &cent->pe.legs, cent->currentState.legsAnim, &legsSkeleton, speedScale );
	}

	CG_RunPlayerLerpFrame( ci, &cent->pe.torso, cent->currentState.torsoAnim, &torsoSkeleton, speedScale );
}

/*
===============
CG_PlayerMD5AlienAnimation
===============
*/

static void CG_PlayerMD5AlienAnimation( centity_t *cent )
{
	clientInfo_t  *ci;
	int           clientNum;
	float         speedScale;
	static refSkeleton_t blend;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer )
	{
		return;
	}

	speedScale = 1;

	ci = &cgs.clientinfo[ clientNum ];

	// If we are attacking/taunting, and in motion, allow blending the two skeletons
	if ( ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) >= NSPA_ATTACK1 &&
	       ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) <= NSPA_ATTACK3 ) ||
	     ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == NSPA_GESTURE )
	{
		blend.type = SK_RELATIVE; // Tell game to blend

		if( ( cent->pe.nonseg.animationNumber & ~ANIM_TOGGLEBIT ) <= NSPA_TURN &&
			( cent->pe.nonseg.animationNumber & ~ANIM_TOGGLEBIT ) != NSPA_GESTURE )
		{
			cent->pe.legs = cent->pe.nonseg;
		}
	}
	else
	{
		blend.type = SK_INVALID;
	}

	// do the shuffle turn frames locally
	if ( cent->pe.nonseg.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == NSPA_STAND )
	{
		CG_RunPlayerLerpFrame( ci, &cent->pe.nonseg, NSPA_TURN, &legsSkeleton, speedScale );
	}
	else
	{
		CG_RunPlayerLerpFrame( ci, &cent->pe.nonseg, cent->currentState.legsAnim, &legsSkeleton, speedScale );
	}

	if ( blend.type == SK_RELATIVE )
	{
		CG_RunPlayerLerpFrame( ci, &cent->pe.legs, cent->pe.legs.animationNumber, &blend, speedScale );
		trap_R_BlendSkeleton( &legsSkeleton, &blend, 0.5 );
	}
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
static void CG_SwingAngles( float destination, float swingTolerance, float clampTolerance,
                            float speed, float *angle, qboolean *swinging )
{
	float swing;
	float move;
	float scale;

	if ( !*swinging )
	{
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );

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
	swing = AngleSubtract( destination, *angle );
	scale = fabs( swing );

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

		*angle = AngleMod( *angle + move );
	}
	else if ( swing < 0 )
	{
		move = cg.frametime * scale * -speed;

		if ( move <= swing )
		{
			move = swing;
			*swinging = qfalse;
		}

		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );

	if ( swing > clampTolerance )
	{
		*angle = AngleMod( destination - ( clampTolerance - 1 ) );
	}
	else if ( swing < -clampTolerance )
	{
		*angle = AngleMod( destination + ( clampTolerance - 1 ) );
	}
}

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch( centity_t *cent, vec3_t torsoAngles )
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

Handles separate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles( centity_t *cent, const vec3_t srcAngles,
                             vec3_t legsAngles, vec3_t torsoAngles, vec3_t headAngles )
{
	float        dest;
	static int   movementOffsets[ 8 ] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	vec3_t       velocity;
	float        speed;
	int          dir, clientNum;
	clientInfo_t *ci;

	VectorCopy( srcAngles, headAngles );
	headAngles[ YAW ] = AngleMod( headAngles[ YAW ] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_IDLE ||
	     ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) != TORSO_STAND )
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
		// did use angles2.. now uses time2.. looks a bit funny but time2 isn't used othwise
		dir = cent->currentState.time2;

		if ( dir < 0 || dir > 7 )
		{
			CG_Error( "Bad player movement angle" );
		}
	}

	legsAngles[ YAW ] = headAngles[ YAW ] + movementOffsets[ dir ];
	torsoAngles[ YAW ] = headAngles[ YAW ] + 0.25 * movementOffsets[ dir ];

	// torso
	if ( cent->currentState.eFlags & EF_DEAD )
	{
		CG_SwingAngles( torsoAngles[ YAW ], 0, 0, cg_swingSpeed.value,
		                &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
		CG_SwingAngles( legsAngles[ YAW ], 0, 0, cg_swingSpeed.value,
		                &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
	}
	else
	{
		CG_SwingAngles( torsoAngles[ YAW ], 25, 90, cg_swingSpeed.value,
		                &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
		CG_SwingAngles( legsAngles[ YAW ], 40, 90, cg_swingSpeed.value,
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

	CG_SwingAngles( dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
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
	VectorCopy( cent->currentState.pos.trDelta, velocity );
	speed = VectorNormalize( velocity );

	if ( speed )
	{
		vec3_t axis[ 3 ];
		float  side;

		speed *= 0.03f;

		AnglesToAxis( legsAngles, axis );
		side = speed * DotProduct( velocity, axis[ 1 ] );
		legsAngles[ ROLL ] -= side;

		side = speed * DotProduct( velocity, axis[ 0 ] );
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
	CG_AddPainTwitch( cent, torsoAngles );

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
}

static void CG_PlayerAxis( centity_t *cent, const vec3_t srcAngles,
                           vec3_t legs[ 3 ], vec3_t torso[ 3 ], vec3_t head[ 3 ] )
{
	vec3_t legsAngles, torsoAngles, headAngles;
	CG_PlayerAngles( cent, srcAngles, legsAngles, torsoAngles, headAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}

#define MODEL_WWSMOOTHTIME 200

/*
===============
CG_PlayerWWSmoothing

Smooth the angles of transitioning wall walkers
===============
*/
static void CG_PlayerWWSmoothing( centity_t *cent, vec3_t in[ 3 ], vec3_t out[ 3 ] )
{
	entityState_t *es = &cent->currentState;
	int           i;
	vec3_t        surfNormal, rotAxis, temp;
	vec3_t        refNormal = { 0.0f, 0.0f,  1.0f };
	vec3_t        ceilingNormal = { 0.0f, 0.0f, -1.0f };
	float         stLocal, sFraction, rotAngle;
	vec3_t        inAxis[ 3 ], lastAxis[ 3 ], outAxis[ 3 ];

	//set surfNormal
	if ( !( es->eFlags & EF_WALLCLIMB ) )
	{
		VectorCopy( refNormal, surfNormal );
	}
	else if ( !( es->eFlags & EF_WALLCLIMBCEILING ) )
	{
		VectorCopy( es->angles2, surfNormal );
	}
	else
	{
		VectorCopy( ceilingNormal, surfNormal );
	}

	AxisCopy( in, inAxis );

	if ( !VectorCompare( surfNormal, cent->pe.lastNormal ) )
	{
		// special case: moving from the ceiling to the floor
		if ( VectorCompare( ceilingNormal, cent->pe.lastNormal ) &&
		     VectorCompare( refNormal,     surfNormal ) )
		{
			VectorCopy( in[ 1 ], rotAxis );
			rotAngle = 180.0f;
		}
		else
		{
			AxisCopy( cent->pe.lastAxis, lastAxis );
			rotAngle = DotProduct( inAxis[ 0 ], lastAxis[ 0 ] ) +
			           DotProduct( inAxis[ 1 ], lastAxis[ 1 ] ) +
			           DotProduct( inAxis[ 2 ], lastAxis[ 2 ] );

			rotAngle = RAD2DEG( acos( ( rotAngle - 1.0f ) / 2.0f ) );

			CrossProduct( lastAxis[ 0 ], inAxis[ 0 ], temp );
			VectorCopy( temp, rotAxis );
			CrossProduct( lastAxis[ 1 ], inAxis[ 1 ], temp );
			VectorAdd( rotAxis, temp, rotAxis );
			CrossProduct( lastAxis[ 2 ], inAxis[ 2 ], temp );
			VectorAdd( rotAxis, temp, rotAxis );

			VectorNormalize( rotAxis );
		}

		//iterate through smooth array
		for ( i = 0; i < MAXSMOOTHS; i++ )
		{
			//found an unused index in the smooth array
			if ( cent->pe.sList[ i ].time + MODEL_WWSMOOTHTIME < cg.time )
			{
				//copy to array and stop
				VectorCopy( rotAxis, cent->pe.sList[ i ].rotAxis );
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
			sFraction = - ( cos( stLocal * M_PI ) + 1.0f ) / 2.0f;

			RotatePointAroundVector( outAxis[ 0 ], cent->pe.sList[ i ].rotAxis,
			                         inAxis[ 0 ], sFraction * cent->pe.sList[ i ].rotAngle );
			RotatePointAroundVector( outAxis[ 1 ], cent->pe.sList[ i ].rotAxis,
			                         inAxis[ 1 ], sFraction * cent->pe.sList[ i ].rotAngle );
			RotatePointAroundVector( outAxis[ 2 ], cent->pe.sList[ i ].rotAxis,
			                         inAxis[ 2 ], sFraction * cent->pe.sList[ i ].rotAngle );

			AxisCopy( outAxis, inAxis );
		}
	}

	//outAxis has been copied to inAxis
	AxisCopy( inAxis, out );
}

/*
===============
CG_PlayerNonSegAxis

Resolve Axis for non-segmented models
===============
*/
static void CG_PlayerNonSegAxis( centity_t *cent, vec3_t srcAngles, vec3_t nonSegAxis[ 3 ] )
{
	vec3_t        localAngles;
	vec3_t        velocity;
	float         speed;
	int           dir;
	entityState_t *es = &cent->currentState;
	vec3_t        surfNormal;
	vec3_t        ceilingNormal = { 0.0f, 0.0f, -1.0f };

	VectorCopy( srcAngles, localAngles );
	localAngles[ YAW ] = AngleMod( localAngles[ YAW ] );
	localAngles[ PITCH ] = 0.0f;
	localAngles[ ROLL ] = 0.0f;

	//set surfNormal
	if ( !( es->eFlags & EF_WALLCLIMBCEILING ) )
	{
		VectorCopy( es->angles2, surfNormal );
	}
	else
	{
		VectorCopy( ceilingNormal, surfNormal );
	}

	//make sure that WW transitions don't cause the swing stuff to go nuts
	if ( !VectorCompare( surfNormal, cent->pe.lastNormal ) )
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
		// did use angles2.. now uses time2.. looks a bit funny but time2 isn't used othwise
		dir = cent->currentState.time2;

		if ( dir < 0 || dir > 7 )
		{
			CG_Error( "Bad player movement angle" );
		}
	}

	// torso
	if ( cent->currentState.eFlags & EF_DEAD )
	{
		CG_SwingAngles( localAngles[ YAW ], 0, 0, cg_swingSpeed.value,
		                &cent->pe.nonseg.yawAngle, &cent->pe.nonseg.yawing );
	}
	else
	{
		CG_SwingAngles( localAngles[ YAW ], 40, 90, cg_swingSpeed.value,
		                &cent->pe.nonseg.yawAngle, &cent->pe.nonseg.yawing );
	}

	localAngles[ YAW ] = cent->pe.nonseg.yawAngle;

	// --------- pitch -------------

	//NO PITCH!

	// --------- roll -------------

	// lean towards the direction of travel
	VectorCopy( cent->currentState.pos.trDelta, velocity );
	speed = VectorNormalize( velocity );

	if ( speed )
	{
		vec3_t axis[ 3 ];
		float  side;

		//much less than with the regular model system
		speed *= 0.01f;

		AnglesToAxis( localAngles, axis );
		side = speed * DotProduct( velocity, axis[ 1 ] );
		localAngles[ ROLL ] -= side;

		side = speed * DotProduct( velocity, axis[ 0 ] );
		localAngles[ PITCH ] += side;
	}

	//FIXME: PAIN[123] animations?
	// pain twitch
	//CG_AddPainTwitch( cent, torsoAngles );

	AnglesToAxis( localAngles, nonSegAxis );
}

/*
===============
CG_JetpackAnimation
===============
*/
#define JETPACK_USES_SKELETAL_ANIMATION 1

static void CG_JetpackAnimation( centity_t *cent, int *old, int *now, float *backLerp )
{
	lerpFrame_t	*lf = &cent->jetpackLerpFrame;
	animation_t	*anim;

	if ( cent->jetpackAnim != lf->animationNumber || !lf->animation )
	{
		lf->old_animationNumber = lf->animationNumber;
		lf->old_animation = lf->animation;

		lf->animationNumber = cent->jetpackAnim;

		if( cent->jetpackAnim < 0 || cent->jetpackAnim >= MAX_JETPACK_ANIMATIONS )
			CG_Error( "Bad animation number: %i", cent->jetpackAnim );


		if( JETPACK_USES_SKELETAL_ANIMATION )
		{
			if ( jetpackSkeleton.type != SK_INVALID )
			{
				oldSkeleton = jetpackSkeleton;

				if ( lf->old_animation != NULL && lf->old_animation->handle )
				{
					if ( !trap_R_BuildSkeleton( &oldSkeleton, lf->old_animation->handle, lf->oldFrame, lf->frame, lf->blendlerp, lf->old_animation->clearOrigin ) )
					{
						CG_Printf( "Can't build old jetpack skeleton\n" );
						return;
					}
				}
			}
		}

		anim = &cgs.media.jetpackAnims[ cent->jetpackAnim ];

		if ( !lf->animation )
			lf->frameTime = cg.time;

		lf->animation = anim;

		lf->animationTime = cg.time + anim->initialLerp;

		if ( JETPACK_USES_SKELETAL_ANIMATION )
		{
			lf->oldFrame = lf->frame = 0;
			lf->oldFrameTime = lf->frameTime = 0;
		}

		if ( lf->old_animationNumber <= 0 )
			lf->blendlerp = 0.0f;
		else
		{
			if ( lf->blendlerp <= 0.0f )
				lf->blendlerp = 1.0f;
			else
				lf->blendlerp = 1.0f - lf->blendlerp;
		}
	}

	CG_RunLerpFrame( lf, 1.0f );

	*old = lf->oldFrame;
	*now = lf->frame;
	*backLerp = lf->backlerp;

	if ( JETPACK_USES_SKELETAL_ANIMATION )
	{
		CG_BlendLerpFrame( lf );
		CG_BuildAnimSkeleton( lf, &jetpackSkeleton, &oldSkeleton );
	}
}

static void CG_PlayerUpgrades( centity_t *cent, refEntity_t *torso )
{
	// These are static because otherwise we have >32K of locals, and lcc doesn't like that.
	// Also, jetpack and battpack are never both in use together, so just #define.
	refEntity_t jetpack;
	refEntity_t flash;

#	define radar jetpack

	int           held, publicFlags;
	entityState_t *es = &cent->currentState;

	held        = es->modelindex;
	publicFlags = es->modelindex2;

	// jetpack model and effects
	if ( held & ( 1 << UP_JETPACK ) )
	{
		memset( &jetpack, 0, sizeof( jetpack ) );
		VectorCopy( torso->lightingOrigin, jetpack.lightingOrigin );
		jetpack.shadowPlane = torso->shadowPlane;
		jetpack.renderfx = torso->renderfx;

		jetpack.hModel = cgs.media.jetpackModel;

		// identity matrix
		AxisCopy( axisDefault, jetpack.axis );

		// FIXME: change to tag_back when it exists
		CG_PositionRotatedEntityOnTag( &jetpack, torso, torso->hModel, "tag_gear" );

		CG_JetpackAnimation( cent, &jetpack.oldframe, &jetpack.frame, &jetpack.backlerp );
		jetpack.skeleton = jetpackSkeleton;
		CG_TransformSkeleton( &jetpack.skeleton, 1.0f );

		trap_R_AddRefEntityToScene( &jetpack );

		if ( publicFlags & PF_JETPACK_ACTIVE )
		{
			// spawn ps if necessary
			if ( cent->jetPackState != JPS_ACTIVE )
			{
				if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 0 ] ) )
				{
					CG_DestroyParticleSystem( &cent->jetPackPS[ 0 ] );
				}

				if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 1 ] ) )
				{
					CG_DestroyParticleSystem( &cent->jetPackPS[ 1 ] );
				}


				cent->jetPackPS[ 0 ] = CG_SpawnNewParticleSystem( cgs.media.jetPackThrustPS );
				cent->jetPackPS[ 1 ] = CG_SpawnNewParticleSystem( cgs.media.jetPackThrustPS );

				cent->jetPackState = JPS_ACTIVE;
			}

			// play thrust sound
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin,
			                        vec3_origin, cgs.media.jetpackThrustLoopSound );

			// Add flash tag (?)
			{
				memset( &flash, 0, sizeof( flash ) );
				VectorCopy( torso->lightingOrigin, flash.lightingOrigin );
				flash.shadowPlane = torso->shadowPlane;
				flash.renderfx = torso->renderfx;

				flash.hModel = cgs.media.jetpackFlashModel;

				if ( !flash.hModel )
				{
					return;
				}

				AxisCopy( axisDefault, flash.axis );

				CG_PositionRotatedEntityOnTag( &flash, &jetpack, jetpack.hModel, "tag_flash" );
				trap_R_AddRefEntityToScene( &flash );
			}

			// attach ps
			if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 0 ] ) )
			{
				CG_SetAttachmentTag( &cent->jetPackPS[ 0 ]->attachment, &jetpack, jetpack.hModel, "nozzle.R" );
				CG_SetAttachmentCent( &cent->jetPackPS[ 0 ]->attachment, cent );
				CG_AttachToTag( &cent->jetPackPS[ 0 ]->attachment );
			}

			if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 1 ] ) )
			{
				CG_SetAttachmentTag( &cent->jetPackPS[ 1 ]->attachment, &jetpack, jetpack.hModel, "nozzle.L" );
				CG_SetAttachmentCent( &cent->jetPackPS[ 1 ]->attachment, cent );
				CG_AttachToTag( &cent->jetPackPS[ 1 ]->attachment );
			}
		}
		else
		{
			if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 0 ] ) )
			{
				// disable jetpack ps when not thrusting anymore
				CG_DestroyParticleSystem( &cent->jetPackPS[ 0 ] );
				cent->jetPackState = JPS_INACTIVE;
			}

			if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 1 ] ) )
			{
				// disable jetpack ps when not thrusting anymore
				CG_DestroyParticleSystem( &cent->jetPackPS[ 1 ] );
				cent->jetPackState = JPS_INACTIVE;
			}
		}
	}
	else
	{
		// disable jetpack ps when not carrying it anymore
		if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 0 ] ) )
		{
			// disable jetpack ps when not thrusting anymore
			CG_DestroyParticleSystem( &cent->jetPackPS[ 0 ] );
			cent->jetPackState = JPS_INACTIVE;
		}

		if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 1 ] ) )
		{
			// disable jetpack ps when not thrusting anymore
			CG_DestroyParticleSystem( &cent->jetPackPS[ 1 ] );
			cent->jetPackState = JPS_INACTIVE;
		}
	}

	// battery pack
	if ( held & ( 1 << UP_RADAR ) )
	{
		memset( &radar, 0, sizeof( radar ) );
		VectorCopy( torso->lightingOrigin, radar.lightingOrigin );
		radar.shadowPlane = torso->shadowPlane;
		radar.renderfx = torso->renderfx;

		radar.hModel = cgs.media.radarModel;

		//identity matrix
		AxisCopy( axisDefault, radar.axis );

		//FIXME: change to tag_back when it exists
		CG_PositionRotatedEntityOnTag( &radar, torso, torso->hModel, "tag_head" );

		trap_R_AddRefEntityToScene( &radar );
	}

	// creep below bloblocked players
	if ( es->eFlags & EF_BLOBLOCKED )
	{
		vec3_t  temp, origin, up = { 0.0f, 0.0f, 1.0f };
		trace_t tr;
		float   size;

		VectorCopy( es->pos.trBase, temp );
		temp[ 2 ] -= 4096.0f;

		CG_Trace( &tr, es->pos.trBase, NULL, NULL, temp, es->number, MASK_SOLID );
		VectorCopy( tr.endpos, origin );

		size = 32.0f;

		if ( size > 0.0f )
		{
			CG_ImpactMark( cgs.media.creepShader, origin, up,
			               0.0f, 1.0f, 1.0f, 1.0f, 1.0f, qfalse, size, qtrue );
		}
	}

#	undef battpack
}

/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader )
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

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[ 2 ] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[ 0 ] = 255;
	ent.shaderRGBA[ 1 ] = 255;
	ent.shaderRGBA[ 2 ] = 255;
	ent.shaderRGBA[ 3 ] = 255;
	trap_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent )
{
	if ( cent->currentState.eFlags & EF_CONNECTION )
	{
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader );
		return;
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
static qboolean CG_PlayerShadow( centity_t *cent, float *shadowPlane, class_t class_ )
{
	vec3_t        end, mins, maxs;
	trace_t       trace;
	float         alpha;
	entityState_t *es = &cent->currentState;
	vec3_t        surfNormal = { 0.0f, 0.0f, 1.0f };

	BG_ClassBoundingBox( class_, mins, maxs, NULL, NULL, NULL );
	mins[ 2 ] = 0.0f;
	maxs[ 2 ] = 2.0f;

	if ( es->eFlags & EF_WALLCLIMB )
	{
		if ( es->eFlags & EF_WALLCLIMBCEILING )
		{
			VectorSet( surfNormal, 0.0f, 0.0f, -1.0f );
		}
		else
		{
			VectorCopy( es->angles2, surfNormal );
		}
	}

	*shadowPlane = 0;

	if ( cg_shadows.integer == SHADOWING_NONE )
	{
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	VectorMA( cent->lerpOrigin, -SHADOW_DISTANCE, surfNormal, end );

	trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid )
	{
		return qfalse;
	}

	// FIXME: stencil shadows will be broken for walls.
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

	if ( cg_shadows.integer > SHADOWING_BLOB &&
	     cg_playerShadows.integer ) {
		// add inverse shadow map
		{
		  CG_StartShadowCaster( cent->lerpOrigin, mins, maxs );
		}
	}

	if ( cg_shadows.integer != SHADOWING_BLOB ) // no mark for stencil or projection shadows
	{
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal,
	               cent->pe.legs.yawAngle, 0.0f, 0.0f, 0.0f, alpha, qfalse,
	               24.0f * BG_ClassModelConfig( class_ )->shadowScale, qtrue );

	return qtrue;
}

static void CG_PlayerShadowEnd( void )
{
	if ( cg_shadows.integer == SHADOWING_NONE )
	{
		return;
	}

	if ( cg_shadows.integer > SHADOWING_BLOB &&
	     cg_playerShadows.integer ) {
		CG_EndShadowCaster( );
	}
}

/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash( centity_t *cent, class_t class_ )
{
	vec3_t  start, end;
	vec3_t  mins, maxs;
	trace_t trace;
	int     contents;

	if ( cg_shadows.integer == SHADOWING_NONE )
	{
		return;
	}

	BG_ClassBoundingBox( class_, mins, maxs, NULL, NULL, NULL );

	VectorCopy( cent->lerpOrigin, end );
	end[ 2 ] += mins[ 2 ];

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = trap_CM_PointContents( end, 0 );

	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) )
	{
		return;
	}

	VectorCopy( cent->lerpOrigin, start );
	start[ 2 ] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents( start, 0 );

	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
	{
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0,
	                  ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	if ( trace.fraction == 1.0f )
	{
		return;
	}

	CG_ImpactMark( cgs.media.wakeMarkShader, trace.endpos, trace.plane.normal,
	               cent->pe.legs.yawAngle, 1.0f, 1.0f, 1.0f, 1.0f, qfalse,
	               32.0f * BG_ClassModelConfig( class_ )->shadowScale, qtrue );
}

/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts( vec3_t normal, int numVerts, polyVert_t *verts )
{
	int    i, j;
	float  incoming;
	vec3_t ambientLight;
	vec3_t lightDir;
	vec3_t directedLight;

	trap_R_LightForPoint( verts[ 0 ].xyz, ambientLight, directedLight, lightDir );

	for ( i = 0; i < numVerts; i++ )
	{
		incoming = DotProduct( normal, lightDir );

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
int CG_LightFromDirection( vec3_t point, vec3_t direction )
{
	int    j;
	float  incoming;
	vec3_t ambientLight;
	vec3_t lightDir;
	vec3_t directedLight;
	vec3_t result;

	trap_R_LightForPoint( point, ambientLight, directedLight, lightDir );

	incoming = DotProduct( direction, lightDir );

	if ( incoming <= 0 )
	{
		result[ 0 ] = ambientLight[ 0 ];
		result[ 1 ] = ambientLight[ 1 ];
		result[ 2 ] = ambientLight[ 2 ];
		return ( int )( ( float )( result[ 0 ] + result[ 1 ] + result[ 2 ] ) / 3.0f );
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

	return ( int )( ( float )( result[ 0 ] + result[ 1 ] + result[ 2 ] ) / 3.0f );
}

/*
=================
CG_AmbientLight
=================
*/
int CG_AmbientLight( vec3_t point )
{
	vec3_t ambientLight;
	vec3_t lightDir;
	vec3_t directedLight;
	vec3_t result;

	trap_R_LightForPoint( point, ambientLight, directedLight, lightDir );

	result[ 0 ] = ambientLight[ 0 ];
	result[ 1 ] = ambientLight[ 1 ];
	result[ 2 ] = ambientLight[ 2 ];
	return ( int )( ( float )( result[ 0 ] + result[ 1 ] + result[ 2 ] ) / 3.0f );
}

#define TRACE_DEPTH    32.0f

/*
===============
Statics for CG_Player & CG_Corpse
These are QVM-only to keep locals down below 32K
===============
*/
#ifdef Q3_VM
static refEntity_t legs, torso, body, head;
#endif

/*
===============
CG_Player
===============
*/
void CG_Player( centity_t *cent )
{
	clientInfo_t *ci;

	// NOTE: legs is used for nonsegmented models
	//       this helps reduce code to be changed
#ifndef Q3_VM
	refEntity_t legs, torso, body, head;
#endif

	int           clientNum;
	int           renderfx;
	float         shadowPlane = 0.0f;
	entityState_t *es = &cent->currentState;
	class_t       class_ = (class_t) ( ( es->misc >> 8 ) & 0xFF );
	float         scale;
	vec3_t        tempAxis[ 3 ], tempAxis2[ 3 ];
	vec3_t        angles;
	int           held = es->modelindex;
	vec3_t        surfNormal = { 0.0f, 0.0f, 1.0f };

	altShader_t   altShaderIndex;

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = es->clientNum;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS )
	{
		CG_Error( "Bad clientNum on player entity" );
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

	if ( es->eFlags & EF_DEAD )
	{
		altShaderIndex = CG_ALTSHADER_DEAD;
	}
	else if ( !(es->eFlags & EF_B_POWERED) )
	{
		altShaderIndex = CG_ALTSHADER_UNPOWERED;
	}
	else
	{
		altShaderIndex = CG_ALTSHADER_DEFAULT;
	}

	// get the player model information
	renderfx = 0;

	if ( es->number == cg.snap->ps.clientNum )
	{
		if ( !cg.renderingThirdPerson )
		{
			renderfx = RF_THIRD_PERSON; // only draw in mirrors
		}
	}

	if ( cg_drawBBOX.integer && cg.renderingThirdPerson )
	{
		vec3_t mins, maxs;

		BG_ClassBoundingBox( class_, mins, maxs, NULL, NULL, NULL );
		CG_DrawBoundingBox( cg_drawBBOX.integer, cent->lerpOrigin, mins, maxs );
	}

	if ( ci->md5 )
	{
		memset( &body,    0, sizeof( body ) );
	}
	else
	{
		memset( &legs,    0, sizeof( legs ) );
		memset( &torso,   0, sizeof( torso ) );
		memset( &head,    0, sizeof( head ) );
	}

	VectorCopy( cent->lerpAngles, angles );
	AnglesToAxis( cent->lerpAngles, tempAxis );

	//rotate lerpAngles to floor
	if ( es->eFlags & EF_WALLCLIMB &&
	     BG_RotateAxis( es->angles2, tempAxis, tempAxis2, qtrue, es->eFlags & EF_WALLCLIMBCEILING ) )
	{
		AxisToAngles( tempAxis2, angles );
	}
	else
	{
		VectorCopy( cent->lerpAngles, angles );
	}

	//normalise the pitch
	if ( angles[ PITCH ] < -180.0f )
	{
		angles[ PITCH ] += 360.0f;
	}

	if ( ci->md5 )
	{
		vec3_t legsAngles, torsoAngles, headAngles;
		int    boneIndex;
		vec3_t playerOrigin, mins, maxs;
		quat_t rotation;

		if ( ci->gender != GENDER_NEUTER )
		{
			CG_PlayerAngles( cent, angles, legsAngles, torsoAngles, headAngles );
			AnglesToAxis( legsAngles, body.axis );
		}
		else
		{
			CG_PlayerNonSegAxis( cent, angles, body.axis );
		}

		AxisCopy( body.axis, tempAxis );

		// rotate the legs axis to back to the wall
		if ( es->eFlags & EF_WALLCLIMB && BG_RotateAxis( es->angles2, body.axis, tempAxis, qfalse, es->eFlags & EF_WALLCLIMBCEILING ) )
		{
			AxisCopy( tempAxis, body.axis );
		}

		// smooth out WW transitions so the model doesn't hop around
		CG_PlayerWWSmoothing( cent, body.axis, body.axis );

		AxisCopy( tempAxis, cent->pe.lastAxis );

		// get the animation state (after rotation, to allow feet shuffle)
		if ( ci->gender != GENDER_NEUTER )
		{
			CG_PlayerMD5Animation( cent );
		}
		else
		{
			CG_PlayerMD5AlienAnimation( cent );
		}

		// add the talk baloon or disconnect icon
		CG_PlayerSprites( cent );

		// add the shadow
		if ( ( es->number == cg.snap->ps.clientNum && cg.renderingThirdPerson ) ||
			 es->number != cg.snap->ps.clientNum )
		{
			CG_PlayerShadow( cent, &shadowPlane, class_ );
		}

		// add a water splash if partially in and out of water
		CG_PlayerSplash( cent, class_ );

		renderfx |= RF_LIGHTING_ORIGIN; // use the same origin for all

		// add the body
		body.hModel = ci->bodyModel;
		body.customSkin = ci->bodySkin;

		if ( !body.hModel )
		{
			CG_Printf( "No body model for player %i\n", clientNum );
			return;
		}

		body.shadowPlane = shadowPlane;
		body.renderfx = renderfx;

		BG_ClassBoundingBox( class_, mins, maxs, NULL, NULL, NULL );

		// move the origin closer into the wall with a CapTrace
		if ( es->eFlags & EF_WALLCLIMB && !( es->eFlags & EF_DEAD ) && !( cg.intermissionStarted ) )
		{
			vec3_t  start, end;
			trace_t tr;

			if ( es->eFlags & EF_WALLCLIMBCEILING )
			{
				VectorSet( surfNormal, 0.0f, 0.0f, -1.0f );
			}
			else
			{
				VectorCopy( es->angles2, surfNormal );
			}


			VectorMA( cent->lerpOrigin, -TRACE_DEPTH, surfNormal, end );
			VectorMA( cent->lerpOrigin, 1.0f, surfNormal, start );
			CG_CapTrace( &tr, start, NULL, NULL, end, es->number, MASK_PLAYERSOLID );

			// if the trace misses completely then just use body.origin
			// apparently capsule traces are "smaller" than box traces
			if ( tr.fraction != 1.0f )
			{
				VectorCopy( tr.endpos, playerOrigin );

				// MD5 player models have their model origin at (0 0 0)
				VectorMA( playerOrigin, 0, surfNormal, body.origin );
			}
			else
			{
				VectorCopy( cent->lerpOrigin, playerOrigin );

				// MD5 player models have their model origin at (0 0 0)
				VectorMA( cent->lerpOrigin, -TRACE_DEPTH, surfNormal, body.origin );
			}

			VectorCopy( body.origin, body.lightingOrigin );
			VectorCopy( body.origin, body.oldorigin );  // don't positionally lerp at all
		}
		else
		{
			VectorCopy( cent->lerpOrigin, playerOrigin );
			VectorCopy( playerOrigin, body.origin );
			body.origin[ 0 ] -= ci->headOffset[ 0 ];
			body.origin[ 1 ] -= ci->headOffset[ 1 ];
			body.origin[ 2 ] -= 22 + ci->headOffset[ 2 ];
		}

		VectorCopy( body.origin, body.lightingOrigin );
		VectorCopy( body.origin, body.oldorigin );  // don't positionally lerp at all

		if ( ci->gender != GENDER_NEUTER )
		{
			// copy legs skeleton to have a base
			body.skeleton = torsoSkeleton;
			if ( torsoSkeleton.numBones != legsSkeleton.numBones )
			{

				// seems only to happen when switching from an MD3 model to an MD5 model
				// while spectating (switching between players on the human team)
				// - don't treat as fatal, but doing so will (briefly?) cause rendering
				// glitches if chasing; also, brief spam
				CG_Printf( "[skipnotify]WARNING: cent->pe.legs.skeleton.numBones != cent->pe.torso.skeleton.numBones\n" );
			}

			// combine legs and torso skeletons
			if ( ci->numLegBones )
			{
				CG_CombineLegSkeleton( &body.skeleton, &legsSkeleton, ci->legBones, ci->numLegBones );
			}

			if ( ci->weaponAdjusted & ( 1 << es->weapon ) && ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) >= TORSO_ATTACK  )
			{
				int j;

				for ( j = 0; j < ci->numHandBones; j++ )
				{
					VectorAdd( deltas[ es->weapon ][ ci->handBones[ j ] ].delta, body.skeleton.bones[ ci->handBones[ j ] ].t.trans, body.skeleton.bones[ ci->handBones[ j ] ].t.trans );
					QuatMultiply0( body.skeleton.bones[ ci->handBones[ j ] ].t.rot, deltas[ es->weapon ][ ci->handBones[ j ] ].rot );
				}
			}

			// HACK: Stop taunt from clipping through the body.
			if ( ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) >= TORSO_GESTURE_BLASTER && ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) <= TORSO_GESTURE_CKIT  )
			{
				quat_t rot;
				QuatFromAngles( rot, 15, 0, 0 );
				QuatMultiply0( body.skeleton.bones[ 33 ].t.rot, rot );
			}

			// rotate torso
			boneIndex = ci->torsoControlBone;

			if ( boneIndex >= 0 && boneIndex < torsoSkeleton.numBones )
			{
				// HACK: convert angles to bone system
				QuatFromAngles( rotation, torsoAngles[ YAW ], 0, 0 );
				QuatMultiply0( body.skeleton.bones[ boneIndex ].t.rot, rotation );
			}

			// HACK: limit angle (avoids worst of the gun clipping through the body)
			// Needs some proper animation fixes...
			if ( cent->lerpAngles[ 0 ] > 60 )
			{
				cent->lerpAngles[ 0 ] = 60;
			}
			else if ( cent->lerpAngles[ 0 ] < -60 )
			{
				cent->lerpAngles[ 0 ] = -60;
			}

			QuatFromAngles( rotation, -cent->lerpAngles[ 0 ], 0, 0 );
			QuatMultiply0( body.skeleton.bones[ ci->rightShoulderBone ].t.rot, rotation );

			// Relationships are emphirically derived. They will probably need to be changed upon changes to the human model
			QuatFromAngles( rotation, cent->lerpAngles[ 0 ], cent->lerpAngles[ 0 ] < 0 ? -cent->lerpAngles[ 0 ] / 9 : -cent->lerpAngles[ 0 ] / ( 8 - ( 5 * ( cent->lerpAngles[ 0 ] / 90 ) ) )  , 0 );
			QuatMultiply0( body.skeleton.bones[ ci->leftShoulderBone ].t.rot, rotation );
		}
		else
		{
			body.skeleton = legsSkeleton;
		}

		// transform relative bones to absolute ones required for vertex skinning and tag attachments
		CG_TransformSkeleton( &body.skeleton, ci->modelScale );

		// add the gun / barrel / flash
		if ( es->weapon != WP_NONE )
		{
			CG_AddPlayerWeapon( &body, NULL, cent );
		}

		CG_PlayerUpgrades( cent, &body );

		// add body to renderer
		body.altShaderIndex = altShaderIndex;
		trap_R_AddRefEntityToScene( &body );

		goto finish_up;

	}

	// get the rotation information
	if ( !ci->nonsegmented )
	{
		CG_PlayerAxis( cent, angles, legs.axis, torso.axis, head.axis );
	}
	else
	{
		CG_PlayerNonSegAxis( cent, angles, legs.axis );
	}

	AxisCopy( legs.axis, tempAxis );

	//rotate the legs axis to back to the wall
	if ( es->eFlags & EF_WALLCLIMB &&
	     BG_RotateAxis( es->angles2, legs.axis, tempAxis, qfalse, es->eFlags & EF_WALLCLIMBCEILING ) )
	{
		AxisCopy( tempAxis, legs.axis );
	}

	//smooth out WW transitions so the model doesn't hop around
	CG_PlayerWWSmoothing( cent, legs.axis, legs.axis );

	AxisCopy( tempAxis, cent->pe.lastAxis );

	// get the animation state (after rotation, to allow feet shuffle)
	if ( !ci->nonsegmented )
	{
		CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
		                    &torso.oldframe, &torso.frame, &torso.backlerp );
	}
	else
	{
		CG_PlayerNonSegAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp );
	}

	// add the talk baloon or disconnect icon
	CG_PlayerSprites( cent );

	// add the shadow
	if ( ( es->number == cg.snap->ps.clientNum && cg.renderingThirdPerson ) ||
	     es->number != cg.snap->ps.clientNum )
	{
		CG_PlayerShadow( cent, &shadowPlane, class_ );
	}

	// add a water splash if partially in and out of water
	CG_PlayerSplash( cent, class_ );

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

	VectorCopy( cent->lerpOrigin, legs.origin );

	VectorCopy( cent->lerpOrigin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy( legs.origin, legs.oldorigin );  // don't positionally lerp at all

	//move the origin closer into the wall with a CapTrace
	if ( es->eFlags & EF_WALLCLIMB && !( es->eFlags & EF_DEAD ) && !( cg.intermissionStarted ) )
	{
		vec3_t  start, end, mins, maxs;
		trace_t tr;

		if ( es->eFlags & EF_WALLCLIMBCEILING )
		{
			VectorSet( surfNormal, 0.0f, 0.0f, -1.0f );
		}
		else
		{
			VectorCopy( es->angles2, surfNormal );
		}

		BG_ClassBoundingBox( class_, mins, maxs, NULL, NULL, NULL );

		VectorMA( legs.origin, -TRACE_DEPTH, surfNormal, end );
		VectorMA( legs.origin, 1.0f, surfNormal, start );
		CG_CapTrace( &tr, start, mins, maxs, end, es->number, MASK_PLAYERSOLID );

		//if the trace misses completely then just use legs.origin
		//apparently capsule traces are "smaller" than box traces
		if ( tr.fraction != 1.0f )
		{
			VectorMA( legs.origin, tr.fraction * -TRACE_DEPTH, surfNormal, legs.origin );
		}

		VectorCopy( legs.origin, legs.lightingOrigin );
		VectorCopy( legs.origin, legs.oldorigin );  // don't positionally lerp at all
	}

	//rescale the model
	scale = BG_ClassModelConfig( class_ )->modelScale;

	if ( scale != 1.0f )
	{
		VectorScale( legs.axis[ 0 ], scale, legs.axis[ 0 ] );
		VectorScale( legs.axis[ 1 ], scale, legs.axis[ 1 ] );
		VectorScale( legs.axis[ 2 ], scale, legs.axis[ 2 ] );

		legs.nonNormalizedAxes = qtrue;
	}

	//offset on the Z axis if required
	VectorMA( legs.origin, BG_ClassModelConfig( class_ )->zOffset, surfNormal, legs.origin );
	VectorCopy( legs.origin, legs.lightingOrigin );
	VectorCopy( legs.origin, legs.oldorigin );  // don't positionally lerp at all

	legs.altShaderIndex = altShaderIndex;
	trap_R_AddRefEntityToScene( &legs );

	// if the model failed, allow the default nullmodel to be displayed
	if ( !legs.hModel )
	{
		CG_PlayerShadowEnd( );
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
			CG_PlayerShadowEnd( );
			return;
		}

		VectorCopy( cent->lerpOrigin, torso.lightingOrigin );

		CG_PositionRotatedEntityOnTag( &torso, &legs, ci->legsModel, "tag_torso" );

		torso.shadowPlane = shadowPlane;
		torso.renderfx = renderfx;

		torso.altShaderIndex = altShaderIndex;
		trap_R_AddRefEntityToScene( &torso );

		//
		// add the head
		//
		head.hModel = ci->headModel;
		head.customSkin = ci->headSkin;

		if ( !head.hModel )
		{
			CG_PlayerShadowEnd( );
			return;
		}

		VectorCopy( cent->lerpOrigin, head.lightingOrigin );

		CG_PositionRotatedEntityOnTag( &head, &torso, ci->torsoModel, "tag_head" );

		head.shadowPlane = shadowPlane;
		head.renderfx = renderfx;

		head.altShaderIndex = altShaderIndex;
		trap_R_AddRefEntityToScene( &head );
	}

	//
	// add the gun / barrel / flash
	//
	if ( es->weapon != WP_NONE )
	{
		if ( !ci->nonsegmented )
		{
			CG_AddPlayerWeapon( &torso, NULL, cent );
		}
		else
		{
			CG_AddPlayerWeapon( &legs, NULL, cent );
		}
	}

	CG_PlayerUpgrades( cent, &torso );

finish_up:
	//sanity check that particle systems are stopped when dead
	if ( es->eFlags & EF_DEAD )
	{
		if ( CG_IsParticleSystemValid( &cent->muzzlePS ) )
		{
			CG_DestroyParticleSystem( &cent->muzzlePS );
		}

		if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 0 ] ) )
		{
			CG_DestroyParticleSystem( &cent->jetPackPS[ 0 ] );
		}

		if ( CG_IsParticleSystemValid( &cent->jetPackPS[ 1 ] ) )
		{
			CG_DestroyParticleSystem( &cent->jetPackPS[ 1 ] );
		}
	}

	VectorCopy( surfNormal, cent->pe.lastNormal );
	CG_PlayerShadowEnd( );
}

/*
===============
CG_Corpse
===============
*/
void CG_Corpse( centity_t *cent )
{
#ifndef Q3_VM
	refEntity_t   legs, torso, head;
#endif
	clientInfo_t  *ci;
	entityState_t *es = &cent->currentState;
	int           corpseNum;
	int           renderfx;
	float         shadowPlane;
	vec3_t        origin, liveZ, deadZ, deadMax;
	float         scale;

	corpseNum = CG_GetCorpseNum( (class_t) es->clientNum );

	if ( corpseNum < 0 || corpseNum >= MAX_CLIENTS )
	{
		CG_Error( "Bad corpseNum on corpse entity: %d", corpseNum );
	}

	ci = &cgs.corpseinfo[ corpseNum ];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid )
	{
		return;
	}

	memset( &legs, 0, sizeof( legs ) );
	memset( &torso, 0, sizeof( torso ) );
	memset( &head, 0, sizeof( head ) );

	VectorCopy( cent->lerpOrigin, origin );
	BG_ClassBoundingBox( es->clientNum, liveZ, NULL, NULL, deadZ, deadMax );
	origin[ 2 ] -= ( liveZ[ 2 ] - deadZ[ 2 ] );

	if( ci->md5 )
	{
		origin[ 0 ] -= ci->headOffset[ 0 ];
		origin[ 1 ] -= ci->headOffset[ 1 ];
		origin[ 2 ] -= 19 + ci->headOffset[ 2 ];
	}
	VectorCopy( es->angles, cent->lerpAngles );

	// get the rotation information
	if ( !ci->nonsegmented )
	{
		CG_PlayerAxis( cent, cent->lerpAngles, legs.axis, torso.axis, head.axis );
	}
	else
	{
		CG_PlayerNonSegAxis( cent, cent->lerpAngles, legs.axis );
	}

	//set the correct frame (should always be dead)
	if ( cg_noPlayerAnims.integer )
	{
		legs.oldframe = legs.frame = torso.oldframe = torso.frame = 0;
	}
	else if ( ci->md5 )
	{
		if ( ci->gender == GENDER_NEUTER )
		{
			memset( &cent->pe.nonseg, 0, sizeof( lerpFrame_t ) );
			CG_RunCorpseLerpFrame( ci, &cent->pe.nonseg, NSPA_DEATH1 );
			legs.oldframe = cent->pe.nonseg.oldFrame;
			legs.frame = cent->pe.nonseg.frame;
			legs.backlerp = cent->pe.nonseg.backlerp;
		}
		else
		{
			memset( &cent->pe.legs, 0, sizeof( lerpFrame_t ) );
			CG_RunCorpseLerpFrame( ci, &cent->pe.legs, BOTH_DEATH1 );
			legs.oldframe = cent->pe.legs.oldFrame;
			legs.frame = cent->pe.legs.frame;
			legs.backlerp = cent->pe.legs.backlerp;
		}
	}
	else if ( !ci->nonsegmented )
	{
		memset( &cent->pe.legs, 0, sizeof( lerpFrame_t ) );
		CG_RunPlayerLerpFrame( ci, &cent->pe.legs, es->legsAnim, NULL, 1 );
		legs.oldframe = cent->pe.legs.oldFrame;
		legs.frame = cent->pe.legs.frame;
		legs.backlerp = cent->pe.legs.backlerp;

		memset( &cent->pe.torso, 0, sizeof( lerpFrame_t ) );
		CG_RunPlayerLerpFrame( ci, &cent->pe.torso, es->torsoAnim, NULL, 1 );
		torso.oldframe = cent->pe.torso.oldFrame;
		torso.frame = cent->pe.torso.frame;
		torso.backlerp = cent->pe.torso.backlerp;
	}
	else
	{
		memset( &cent->pe.nonseg, 0, sizeof( lerpFrame_t ) );
		CG_RunPlayerLerpFrame( ci, &cent->pe.nonseg, es->legsAnim, NULL, 1 );
		legs.oldframe = cent->pe.nonseg.oldFrame;
		legs.frame = cent->pe.nonseg.frame;
		legs.backlerp = cent->pe.nonseg.backlerp;
	}

	// add the shadow
	CG_PlayerShadow( cent, &shadowPlane, (class_t) es->clientNum );

	// get the player model information
	renderfx = RF_LIGHTING_ORIGIN; // use the same origin for all

	//
	// add the legs
	//
	if ( ci->md5 )
	{
		legs.hModel = ci->bodyModel;
		legs.customSkin = ci->bodySkin;
		legs.skeleton = legsSkeleton;
		CG_TransformSkeleton( &legs.skeleton, ci->modelScale );
	}
	else if ( !ci->nonsegmented )
	{
		legs.hModel = ci->legsModel;
		legs.customSkin = ci->legsSkin;
	}
	else
	{
		legs.hModel = ci->nonSegModel;
		legs.customSkin = ci->nonSegSkin;
	}

	VectorCopy( origin, legs.origin );

	VectorCopy( origin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	legs.origin[ 2 ] += BG_ClassModelConfig( es->clientNum )->zOffset;
	VectorCopy( legs.origin, legs.oldorigin );  // don't positionally lerp at all

	//rescale the model
	scale = BG_ClassModelConfig( es->clientNum )->modelScale;

	if ( scale != 1.0f && !ci->md5 )
	{
		VectorScale( legs.axis[ 0 ], scale, legs.axis[ 0 ] );
		VectorScale( legs.axis[ 1 ], scale, legs.axis[ 1 ] );
		VectorScale( legs.axis[ 2 ], scale, legs.axis[ 2 ] );

		legs.nonNormalizedAxes = qtrue;
	}

	legs.altShaderIndex = CG_ALTSHADER_DEAD;
	trap_R_AddRefEntityToScene( &legs );

	// if the model failed, allow the default nullmodel to be displayed. Also, if MD5, no need to add other parts
	if ( !legs.hModel || ci->md5 )
	{
		CG_PlayerShadowEnd( );
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
			CG_PlayerShadowEnd( );
			return;
		}

		torso.customSkin = ci->torsoSkin;

		VectorCopy( origin, torso.lightingOrigin );

		CG_PositionRotatedEntityOnTag( &torso, &legs, ci->legsModel, "tag_torso" );

		torso.shadowPlane = shadowPlane;
		torso.renderfx = renderfx;

		torso.altShaderIndex = CG_ALTSHADER_DEAD;
		trap_R_AddRefEntityToScene( &torso );

		//
		// add the head
		//
		head.hModel = ci->headModel;

		if ( !head.hModel )
		{
			CG_PlayerShadowEnd( );
			return;
		}

		head.customSkin = ci->headSkin;

		VectorCopy( origin, head.lightingOrigin );

		CG_PositionRotatedEntityOnTag( &head, &torso, ci->torsoModel, "tag_head" );

		head.shadowPlane = shadowPlane;
		head.renderfx = renderfx;

		head.altShaderIndex = CG_ALTSHADER_DEAD;
		trap_R_AddRefEntityToScene( &head );
	}
}

//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity( centity_t *cent )
{
	cent->errorTime = -99999; // guarantee no error decay added
	cent->extrapolated = qfalse;

	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ],
	                   &cent->pe.legs, cent->currentState.legsAnim, &legsSkeleton );
	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ],
	                   &cent->pe.torso, cent->currentState.torsoAnim, &torsoSkeleton );
	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ],
	                   &cent->pe.nonseg, cent->currentState.legsAnim, &legsSkeleton );

	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

	VectorCopy( cent->lerpOrigin, cent->rawOrigin );
	VectorCopy( cent->lerpAngles, cent->rawAngles );

	memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->rawAngles[ YAW ];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset( &cent->pe.torso, 0, sizeof( cent->pe.torso ) );
	cent->pe.torso.yawAngle = cent->rawAngles[ YAW ];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[ PITCH ];
	cent->pe.torso.pitching = qfalse;

	memset( &cent->pe.nonseg, 0, sizeof( cent->pe.nonseg ) );
	cent->pe.nonseg.yawAngle = cent->rawAngles[ YAW ];
	cent->pe.nonseg.yawing = qfalse;
	cent->pe.nonseg.pitchAngle = cent->rawAngles[ PITCH ];
	cent->pe.nonseg.pitching = qfalse;

	if ( cg_debugPosition.integer )
	{
		CG_Printf( "%i ResetPlayerEntity yaw=%.2f\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
}

/*
==================
CG_PlayerDisconnect

Player disconnecting
==================
*/
void CG_PlayerDisconnect( vec3_t org )
{
	particleSystem_t *ps;

	trap_S_StartSound( org, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.disconnectSound );

	ps = CG_SpawnNewParticleSystem( cgs.media.disconnectPS );

	if ( CG_IsParticleSystemValid( &ps ) )
	{
		CG_SetAttachmentPoint( &ps->attachment, org );
		CG_AttachToPoint( &ps->attachment );
	}
}

centity_t *CG_GetLocation( vec3_t origin )
{
	int       i;
	centity_t *eloc, *best;
	float     bestlen, len;

	best = NULL;
	bestlen = 3.0f * 8192.0f * 8192.0f;

	for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++ )
	{
		eloc = &cg_entities[ i ];

		if ( !eloc->valid || eloc->currentState.eType != ET_LOCATION )
		{
			continue;
		}

		len = DistanceSquared( origin, eloc->lerpOrigin );

		if ( len > bestlen )
		{
			continue;
		}

		if ( !trap_R_inPVS( origin, eloc->lerpOrigin ) )
		{
			continue;
		}

		bestlen = len;
		best = eloc;
	}

	return best;
}

centity_t *CG_GetPlayerLocation( void )
{
	vec3_t    origin;

	VectorCopy( cg.predictedPlayerState.origin, origin );
	return CG_GetLocation( origin );
}

void CG_InitClasses( void )
{
	int i;

	Com_Memset( cg_classes, 0, sizeof( cg_classes ) );

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		const char *icon = BG_Class( i )->icon;

		if ( icon )
		{
			cg_classes[ i ].classIcon = trap_R_RegisterShader( icon, RSF_DEFAULT );

			if ( !cg_classes[ i ].classIcon )
			{
				Com_Printf( S_ERROR "Failed to load class icon file %s\n", icon );
			}
		}
	}
}
