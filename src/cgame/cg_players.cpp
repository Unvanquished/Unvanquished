/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

// cg_players.c -- handle the media and animation for player entities

#include "common/FileSystem.h"
#include "cg_local.h"
#include "cg_animdelta.h"
#include "cg_segmented_skeleton.h"

// debugging
int   debug_anim_current;
int   debug_anim_old;
float debug_anim_blend;

static refSkeleton_t legsSkeleton;
static refSkeleton_t torsoSkeleton;
static refSkeleton_t oldSkeleton;
static refSkeleton_t jetpackSkeleton;

static const char *const cg_customSoundNames[ MAX_CUSTOM_SOUNDS ] =
{
	"*death1",
	"*death2",
	"*death3",
	"*jump1",
	"*pain25_1",
	"*pain50_1",
	"*pain75_1",
	"*pain100_1",
	"*falling1",
	"*gasp",
	"*drown",
	"*fall1",
	"*taunt"
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
		return trap_S_RegisterSound( soundName, false );
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

	Sys::Drop( "Unknown custom sound: %s", soundName );
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

static bool CG_ParseCharacterFile( const char *filename, clientInfo_t *ci )
{
	const char         *text_p;

	std::error_code err;
	std::string text = FS::PakPath::ReadFile( filename, err );
	if ( err )
	{
		Log::Warn( "couldn't read player model file '%s': %s", filename, err.message() );
		return false;
	}

	// parse the text
	text_p = text.c_str();

	ci->footsteps = FOOTSTEP_GENERAL;
	VectorClear( ci->headOffset );
	ci->gender = GENDER_MALE;
	ci->fixedlegs = false;
	ci->fixedtorso = false;
	ci->modelScale = 1.0f;

	// read optional parameters
	while ( 1 )
	{
		const char *token = COM_Parse2( &text_p );

		if ( !token[ 0 ] )
		{
			break;
		}
		if ( !Q_stricmp( token, "modifiers" ) )
		{
			token = COM_Parse2( &text_p );
			if ( !token || *token != '{' )
			{
				Log::Warn( "Expected '{' but found '%s' in %s's character.cfg, skipping", token, ci->modelName );
				continue;
			}
			while ( 1 )
			{
				token = COM_Parse( &text_p );
				if ( !token || *token == '}' ) break;
				if ( !Q_stricmp( token, "HandDelta" ) )
				{
					ci->modifiers.emplace_back(std::make_shared<AnimDelta>());
				}
				else if ( !Q_stricmp( token, "HumanRotations" ) )
				{
					ci->modifiers.emplace_back(std::make_shared<HumanSkeletonRotations>());
				}
				else if ( !Q_stricmp( token, "BattlesuitRotations" ) )
				{
					ci->modifiers.emplace_back(std::make_shared<BsuitSkeletonRotations>());
				}
				else if ( !Q_stricmp( token, "Segmented" ) )
				{
					ci->modifiers.emplace_back(std::make_shared<SegmentedSkeletonCombiner>());
				}
				else
				{
					Log::Notice("Unknown modifier '%s' in %s's character.cfg", token, ci->modelName);
				}
			}
			continue;
		} else if (!Q_stricmp(token, "footsteps"))
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
			else if ( !Q_stricmp( token, "custom" ) )
			{
				ci->footsteps = FOOTSTEP_CUSTOM;
			}
			else if( !Q_stricmp(token, "none") )
			{
				ci->footsteps = FOOTSTEP_NONE;
			}
			else
			{
				Log::Warn( "Bad footsteps parm in %s: %s", filename, token);
			}
			continue;
		}
		else if ( !Q_stricmp( token, "headoffset" ) )
		{
			for ( int i = 0; i < 3; i++ )
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
			ci->fixedlegs = true;
			continue;
		}
		else if ( !Q_stricmp( token, "fixedtorso" ) )
		{
			ci->fixedtorso = true;
			continue;
		}
		else if ( !Q_stricmp( token, "modelScale" ) )
		{
			token = COM_ParseExt2( &text_p, false );

			if ( token )
			{
				ci->modelScale = atof( token );
			}

			continue;
		}
		else
		{
			bool parsed = false;
			for ( size_t i = 0; i < ci->modifiers.size(); ++i )
			{
				if ( ( parsed = ci->modifiers[i]->ParseConfiguration( ci, token, &text_p ) ) ) break;
			}
			if ( parsed ) continue;
		}

		Log::Notice( "unknown token '%s' in %s\n", token, filename );
	}

	return true;
}

static bool CG_RegisterPlayerAnimation( clientInfo_t *ci, const char *modelName, int anim,
		const char *animName, bool loop, bool reversed, bool clearOrigin )
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

	if ( ci->iqm )
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
		if ( cg_debugAnim.Get() )
		{
			Log::Warn( "Failed to load animation file %s", filename );
		}

		return false;
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

	return true;
}

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation counts and rates
(models/players/visor/animation.cfg, etc.)
======================
*/
static bool CG_ParseAnimationFile( const char *filename, clientInfo_t *ci )
{
	const char         *text_p, *prev;
	int          i;
	float        fps;
	int          skip;
	animation_t  *animations;

	animations = ci->animations;

	std::error_code err;
	std::string text = FS::PakPath::ReadFile( filename, err );
	if ( err )
	{
		Log::Warn( "couldn't read player animation file '%s': %s", filename, err.message() );
		return false;
	}

	// parse the text
	text_p = text.c_str();
	skip = 0; // quite the compiler warning

	ci->footsteps = FOOTSTEP_GENERAL;
	VectorClear( ci->headOffset );
	ci->gender = GENDER_MALE;
	ci->fixedlegs = false;
	ci->fixedtorso = false;
	ci->nonsegmented = false;

	// read optional parameters
	while ( 1 )
	{
		prev = text_p; // so we can unget
		const char *token = COM_Parse2( &text_p );

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
				Log::Warn( "Bad footsteps parm in %s: %s", filename, token );
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
			ci->fixedlegs = true;
			continue;
		}
		else if ( !Q_stricmp( token, "fixedtorso" ) )
		{
			ci->fixedtorso = true;
			continue;
		}
		else if ( !Q_stricmp( token, "nonsegmented" ) )
		{
			ci->nonsegmented = true;
			continue;
		}

		// if it is a number, start parsing animations
		if ( token[ 0 ] >= '0' && token[ 0 ] <= '9' )
		{
			text_p = prev; // unget the token
			break;
		}

		Log::Notice( "unknown token '%s' is %s\n", token, filename );
	}

	if ( !ci->nonsegmented )
	{
		// read information for each frame
		for ( i = 0; i < MAX_PLAYER_ANIMATIONS; i++ )
		{
			const char *token = COM_Parse2( &text_p );

			if ( !*token )
			{
				if ( i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE )
				{
					animations[ i ].firstFrame = animations[ TORSO_GESTURE ].firstFrame;
					animations[ i ].frameLerp = animations[ TORSO_GESTURE ].frameLerp;
					animations[ i ].initialLerp = animations[ TORSO_GESTURE ].initialLerp;
					animations[ i ].loopFrames = animations[ TORSO_GESTURE ].loopFrames;
					animations[ i ].numFrames = animations[ TORSO_GESTURE ].numFrames;
					animations[ i ].reversed = false;
					animations[ i ].flipflop = false;
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
			animations[ i ].reversed = false;
			animations[ i ].flipflop = false;

			// if numFrames is negative the animation is reversed
			if ( animations[ i ].numFrames < 0 )
			{
				animations[ i ].numFrames = -animations[ i ].numFrames;
				animations[ i ].reversed = true;
			}

			token = COM_Parse2( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].loopFrames = atoi( token );
			if ( animations[ i ].loopFrames && animations[ i ].loopFrames != animations[ i ].numFrames )
			{
				Log::Warn("CG_ParseAnimationFile: loopFrames != numFrames");
				animations[ i ].loopFrames = animations[ i ].numFrames;
			}

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
			Log::Warn( "Error parsing animation file: %s", filename );
			return false;
		}

		// crouch backward animation
		animations[ LEGS_BACKCR ] = animations[ LEGS_WALKCR ];
		animations[ LEGS_BACKCR ].reversed = true;
		// walk backward animation
		animations[ LEGS_BACKWALK ] = animations[ LEGS_WALK ];
		animations[ LEGS_BACKWALK ].reversed = true;
		// flag moving fast
		animations[ FLAG_RUN ].firstFrame = 0;
		animations[ FLAG_RUN ].numFrames = 16;
		animations[ FLAG_RUN ].loopFrames = 16;
		animations[ FLAG_RUN ].frameLerp = 1000 / 15;
		animations[ FLAG_RUN ].initialLerp = 1000 / 15;
		animations[ FLAG_RUN ].reversed = false;
		// flag not moving or moving slowly
		animations[ FLAG_STAND ].firstFrame = 16;
		animations[ FLAG_STAND ].numFrames = 5;
		animations[ FLAG_STAND ].loopFrames = 0;
		animations[ FLAG_STAND ].frameLerp = 1000 / 20;
		animations[ FLAG_STAND ].initialLerp = 1000 / 20;
		animations[ FLAG_STAND ].reversed = false;
		// flag speeding up
		animations[ FLAG_STAND2RUN ].firstFrame = 16;
		animations[ FLAG_STAND2RUN ].numFrames = 5;
		animations[ FLAG_STAND2RUN ].loopFrames = 1;
		animations[ FLAG_STAND2RUN ].frameLerp = 1000 / 15;
		animations[ FLAG_STAND2RUN ].initialLerp = 1000 / 15;
		animations[ FLAG_STAND2RUN ].reversed = true;
	}
	else
	{
		// read information for each frame
		for ( i = 0; i < MAX_NONSEG_PLAYER_ANIMATIONS; i++ )
		{
			const char *token = COM_Parse2( &text_p );

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
			animations[ i ].reversed = false;
			animations[ i ].flipflop = false;

			// if numFrames is negative the animation is reversed
			if ( animations[ i ].numFrames < 0 )
			{
				animations[ i ].numFrames = -animations[ i ].numFrames;
				animations[ i ].reversed = true;
			}

			token = COM_Parse2( &text_p );

			if ( !*token )
			{
				break;
			}

			animations[ i ].loopFrames = atoi( token );
			if ( animations[ i ].loopFrames && animations[ i ].loopFrames != animations[ i ].numFrames )
			{
				Log::Warn("CG_ParseAnimationFile: loopFrames != numFrames");
				animations[ i ].loopFrames = animations[ i ].numFrames;
			}

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
			Log::Warn( "Error parsing animation file: %s", filename );
			return false;
		}

		// walk backward animation
		animations[ NSPA_WALKBACK ] = animations[ NSPA_WALK ];
		animations[ NSPA_WALKBACK ].reversed = true;
	}

	return true;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static bool CG_RegisterClientSkin( clientInfo_t *ci, const char *modelName, const char *skinName )
{
	char filename[ MAX_QPATH ];

	if ( ci->skeletal )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/body_%s.skin", modelName, skinName );
		ci->bodySkin = trap_R_RegisterSkin( filename );

		if ( !ci->bodySkin )
		{
			Log::Notice( "Body skin load failure: %s\n", filename );
			return false;
		}
	}
	else if ( !ci->nonsegmented )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower_%s.skin", modelName, skinName );
		ci->legsSkin = trap_R_RegisterSkin( filename );

		if ( !ci->legsSkin )
		{
			Log::Notice( "Leg skin load failure: %s\n", filename );
		}

		Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper_%s.skin", modelName, skinName );
		ci->torsoSkin = trap_R_RegisterSkin( filename );

		if ( !ci->torsoSkin )
		{
			Log::Notice( "Torso skin load failure: %s\n", filename );
		}

		Com_sprintf( filename, sizeof( filename ), "models/players/%s/head_%s.skin", modelName, skinName );
		ci->headSkin = trap_R_RegisterSkin( filename );

		if ( !ci->headSkin )
		{
			Log::Notice( "Head skin load failure: %s\n", filename );
		}

		if ( !ci->legsSkin || !ci->torsoSkin || !ci->headSkin )
		{
			return false;
		}
	}
	else
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/nonseg_%s.skin", modelName, skinName );
		ci->nonSegSkin = trap_R_RegisterSkin( filename );

		if ( !ci->nonSegSkin )
		{
			Log::Notice( "Non-segmented skin load failure: %s\n", filename );
			return false;
		}
	}

	return true;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static bool CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName )
{
	char filename[ MAX_QPATH ];

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/%s.iqm", modelName, modelName );
	if ( FS::PakPath::FileExists( filename ) )
	{
		ci->bodyModel = trap_R_RegisterModel( filename );
	}

	if ( ! ci->bodyModel ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/body.md5mesh", modelName );
		if ( FS::PakPath::FileExists(filename) )
		{
			ci->bodyModel = trap_R_RegisterModel( filename );
		}
	}
	else
	{
		ci->iqm = true;
	}

	if ( ci->bodyModel )
	{
		ci->skeletal = true;
	}
	else
	{
		ci->skeletal = false;
	}

	if ( ci->skeletal )
	{
		// load the animations
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/character.cfg", modelName );

		if ( !CG_ParseCharacterFile( filename, ci ) )
		{
			Log::Notice( "Failed to load character file %s\n", filename );
			return false;
		}

		// If model is not an alien, load human animations
		if ( ci->gender != GENDER_NEUTER )
		{
			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_IDLE, "idle", true, false, false ) )
			{
				Log::Notice( "Failed to load idle animation." );
				return false;
			}

			// make LEGS_IDLE the default animation
			for ( int i = 0; i < MAX_PLAYER_ANIMATIONS; i++ )
			{
				if ( i == LEGS_IDLE )
				{
					continue;
				}

				ci->animations[ i ] = ci->animations[ LEGS_IDLE ];
			}

			// FIXME fail register of the entire model if one of these animations is missing

			// FIXME add death animations

			if ( !CG_RegisterPlayerAnimation( ci, modelName, BOTH_DEATH1, "die", false, false, false ) )
			{
				ci->animations[ BOTH_DEATH1 ] = ci->animations[ LEGS_IDLE ];
			}

			if( !CG_RegisterPlayerAnimation(ci, modelName, BOTH_DEATH2, "death2", false, false, false ) )
			{
				ci->animations[ BOTH_DEATH2 ] = ci->animations[ BOTH_DEATH1 ];
			}

			if( !CG_RegisterPlayerAnimation(ci, modelName, BOTH_DEATH3, "death3", false, false, false ) )
			{
				ci->animations[ BOTH_DEATH3 ] = ci->animations[ BOTH_DEATH1 ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_GESTURE, "gesture", false, false, false ) )
			{
				ci->animations[ TORSO_GESTURE ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_RALLY, "rally", false, false, false ) )
			{
				ci->animations[ TORSO_RALLY ] = ci->animations[ LEGS_IDLE ];
			}


			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_ATTACK, "attack", false, false, false ) )
			{
				ci->animations[ TORSO_ATTACK ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_ATTACK_BLASTER, "blaster_attack", false, false, false ) )
			{
				ci->animations[ TORSO_ATTACK_BLASTER ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_ATTACK_PSAW, "psaw_attack", false, false, false ) )
			{
				ci->animations[ TORSO_ATTACK_PSAW ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_STAND, "stand", true, false, false ) )
			{
				ci->animations[ TORSO_STAND ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_IDLECR, "crouch", false, false, false ) )
			{
				ci->animations[ LEGS_IDLECR ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_WALKCR, "crouch_forward", true, false, false ) )
			{
				ci->animations[ LEGS_WALKCR ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_BACKCR, "crouch_back", true, false, false ) )
			{
				ci->animations[ LEGS_BACKCR ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_WALK, "walk", true, false, false ) )
			{
				ci->animations[ LEGS_WALK ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_BACKWALK, "walk_back", true, false, false ) )
			{
				ci->animations[ LEGS_BACKWALK ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_RUN, "run", true, false, false ) )
			{
				ci->animations[ LEGS_RUN ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_BACK, "run_back", true, false, false ) )
			{
				ci->animations[ LEGS_BACK ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_SWIM, "swim", true, false, false ) )
			{
				ci->animations[ LEGS_SWIM ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_JUMP, "jump", false, false, false ) )
			{
				ci->animations[ LEGS_JUMP ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_LAND, "land", false, false, false ) )
			{
				ci->animations[ LEGS_LAND ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_JUMPB, "jump_back", false, false, false ) )
			{
				ci->animations[ LEGS_JUMPB ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_LANDB, "land_back", false, false, false ) )
			{
				ci->animations[ LEGS_LANDB ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, LEGS_TURN, "step", true, false, false ) )
			{
				ci->animations[ LEGS_TURN ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_DROP, "drop", false, false, false ) )
			{
				ci->animations[ TORSO_DROP ] = ci->animations[ LEGS_IDLE ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, TORSO_RAISE, "raise", false, false, false ) )
			{
				ci->animations[ TORSO_RAISE ] = ci->animations[ LEGS_IDLE ];
			}

			// TODO: Don't assume WP_BLASTER is first human weapon
			for ( int i = TORSO_GESTURE_BLASTER, j = WP_BLASTER; i <= TORSO_GESTURE_CKIT; i++, j++ )
			{
				if ( i == TORSO_GESTURE ) { continue; }
				if ( i == TORSO_GESTURE_CKIT ) { j = WP_HBUILD; }

				if ( !CG_RegisterPlayerAnimation( ci, modelName, i, va( "%s_taunt", BG_Weapon( j )->name ), false, false, false ) )
				{
					ci->animations[ i ] = ci->animations[ TORSO_GESTURE ];
				}
			}
		}
		else
		{
			// Load Alien animations
			if ( !CG_RegisterPlayerAnimation( ci, modelName,
NSPA_STAND, "stand", true, false, false ) &&
			     !CG_RegisterPlayerAnimation( ci, modelName,
NSPA_STAND, "idle", true, false, false )
			)
			{
				Log::Notice( "Failed to load stand animation." );
				return false;
			}

			// make LEGS_IDLE the default animation
			for ( int i = 0; i < MAX_NONSEG_PLAYER_ANIMATIONS; i++ )
			{
				if ( i == NSPA_STAND )
				{
					continue;
				}

				ci->animations[ i ] = ci->animations[ NSPA_STAND ];
			}

			// FIXME fail register of the entire model if one of these animations is missing

			// FIXME add death animations

			CG_RegisterPlayerAnimation( ci, modelName, NSPA_DEATH1, "die", false, false, false );

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_DEATH2, "die2", false, false, false ) )
			{
				ci->animations[ NSPA_DEATH2 ] = ci->animations[ NSPA_DEATH1 ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_DEATH3, "die3", false, false, false ) )
			{
				ci->animations[ NSPA_DEATH3 ] = ci->animations[ NSPA_DEATH1 ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_GESTURE, "gesture", false, false, false ) )
			{
				ci->animations[ NSPA_GESTURE ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_ATTACK1, "attack", false, false, false ) )
			{
				ci->animations[ NSPA_ATTACK1 ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_ATTACK2, "attack2", false, false, false ) )
			{
				ci->animations[ NSPA_ATTACK2 ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_ATTACK3, "attack3", false, false, false ) )
			{
				ci->animations[ NSPA_ATTACK3 ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_RUN, "run", true, false, false ) )
			{
				ci->animations[ NSPA_RUN ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_RUNBACK, "run_backwards", true, false, false ) )
			{
				ci->animations[ NSPA_RUNBACK ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_RUNLEFT, "run_left", true, false, false ) )
			{
				ci->animations[ NSPA_RUNLEFT ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_RUNRIGHT, "run_right", true, false, false ) )
			{
				ci->animations[ NSPA_RUNRIGHT ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_WALK, "walk", true, false, false ) )
			{
				ci->animations[ NSPA_WALK ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_WALKBACK, "walk", true, true, false ) )
			{
				ci->animations[ NSPA_WALKBACK ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_WALKLEFT, "walk_left", true, false, false ) )
			{
				ci->animations[ NSPA_WALKLEFT ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_WALKRIGHT, "walk_right", true, false, false ) )
			{
				ci->animations[ NSPA_WALKRIGHT ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_SWIM, "swim", true, false, false ) )
			{
				ci->animations[ NSPA_SWIM ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_JUMP, "jump", false, false, false ) )
			{
				ci->animations[ NSPA_JUMP ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_JUMPBACK, "jump_back", false, false, false ) )
			{
				ci->animations[ NSPA_JUMPBACK ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_LAND, "land", false, false, false ) )
			{
				ci->animations[ NSPA_LAND ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_LANDBACK, "land_back", false, false, false ) )
			{
				ci->animations[ NSPA_LANDBACK ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_TURN, "turn", true, false, false ) )
			{
				ci->animations[ NSPA_TURN ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_PAIN1, "pain1", false, false, false ) )
			{
				ci->animations[ NSPA_PAIN1 ] = ci->animations[ NSPA_STAND ];
			}

			if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_PAIN2, "pain2", false, false, false ) )
			{
				ci->animations[ NSPA_PAIN2 ] = ci->animations[ NSPA_STAND ];
			}

			if ( !Q_stricmp( modelName, "level4" ) )
			{
				if ( !CG_RegisterPlayerAnimation( ci, modelName, NSPA_CHARGE, "charge", true, false, false ) )
				{
					ci->animations[ NSPA_CHARGE ] = ci->animations[ NSPA_STAND ];
				}
			}
		}

		// if any skins failed to load, return failure
		if ( !CG_RegisterClientSkin( ci, modelName, skinName ) )
		{
			Log::Notice( "Failed to load skin file: %s : %s\n", modelName, skinName );
			return false;
		}

		for ( size_t i = 0; i < ci->modifiers.size(); ++i )
		{
			if ( !ci->modifiers[i]->LoadData( ci ) )
			{
				Log::Warn( "^3WARNING: Error loading data for modifier for %s", ci->modelName );
			}
		}

		return true;
	}

	// do this first so the nonsegmented property is set
	// load the animations
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", modelName );

	if ( !CG_ParseAnimationFile( filename, ci ) )
	{
		Log::Warn( "Failed to load animation file %s", filename );
		return false;
	}

	// load cmodels before models so filecache works
	if ( !ci->nonsegmented )
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", modelName );
		ci->legsModel = trap_R_RegisterModel( filename );

		if ( !ci->legsModel )
		{
			Log::Notice( "Failed to load model file %s\n", filename );
			return false;
		}

		Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", modelName );
		ci->torsoModel = trap_R_RegisterModel( filename );

		if ( !ci->torsoModel )
		{
			Log::Notice( "Failed to load model file %s\n", filename );
			return false;
		}

		Com_sprintf( filename, sizeof( filename ), "models/players/%s/head.md3", modelName );
		ci->headModel = trap_R_RegisterModel( filename );

		if ( !ci->headModel )
		{
			Log::Notice( "Failed to load model file %s\n", filename );
			return false;
		}
	}
	else
	{
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/nonseg.md3", modelName );
		ci->nonSegModel = trap_R_RegisterModel( filename );

		if ( !ci->nonSegModel )
		{
			Log::Notice( "Failed to load model file %s\n", filename );
			return false;
		}
	}

	// if any skins failed to load, return failure
	if ( !CG_RegisterClientSkin( ci, modelName, skinName ) )
	{
		Log::Notice( "Failed to load skin file: %s : %s\n", modelName, skinName );
		return false;
	}

	return true;
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
		Sys::Drop( "CG_RegisterClientModelname( %s, %s ) failed", ci->modelName, ci->skinName );
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

		ci->sounds[ i ] = trap_S_RegisterSound( va( "sound/player/%s/%s", dir, s + 1 ), false );
		// fanny about a bit with sounds that are missing
		if ( ci->sounds[ i ] == -1 )
		{
			//file doesn't exist
			if ( i == 11 || i == 8 ) // fall or falling
			{
				ci->sounds[ i ] = trap_S_RegisterSound( "sound/null", false );
			}
			else
			{
				if ( i == 9 ) // gasp
				{
					s = cg_customSoundNames[ 7 ]; // pain100_1
				}
				else if ( i == 10 ) //drown
				{
					s = cg_customSoundNames[ 0 ]; // death1
				}

				ci->sounds[ i ] = trap_S_RegisterSound( va( "sound/player/%s/%s", dir, s + 1 ), false );
			}
		}
	}

	if ( ci->footsteps == FOOTSTEP_CUSTOM )
	{
		for ( i = 0; i < 4; i++ )
		{
			ci->customFootsteps[ i ] = trap_S_RegisterSound( va( "sound/player/%s/step%d", dir, i + 1 ), false );

			if ( !ci->customFootsteps[ i ] )
			{
				ci->customFootsteps[ i ] = trap_S_RegisterSound( va( "sound/player/footsteps/step%d", i + 1 ), false );
			}

			ci->customMetalFootsteps[ i ] = trap_S_RegisterSound( va( "sound/player/%s/clank%d", dir, i + 1 ), false );

			if ( !ci->customMetalFootsteps[ i ] )
			{
				ci->customMetalFootsteps[ i ] = trap_S_RegisterSound( va( "sound/player/footsteps/clank%d", i + 1 ), false );
			}
		}
	}

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	clientNum = ci - cgs.clientinfo;

	for ( i = 0; i < MAX_GENTITIES; i++ )
	{
		if ( cg_entities[ i ].currentState.clientNum == clientNum &&
		     cg_entities[ i ].currentState.eType == entityType_t::ET_PLAYER )
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
	to->skeletal = from->skeletal;
	to->iqm = from->iqm;
	to->modifiers = from->modifiers;

	memcpy( to->animations, from->animations, sizeof( to->animations ) );
	memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );
	memcpy( to->customFootsteps, from->customFootsteps, sizeof( to->customFootsteps ) );
	memcpy( to->customMetalFootsteps, from->customMetalFootsteps, sizeof( to->customMetalFootsteps ) );
}

/*
======================
GetCorpseInfo
======================
*/
static clientInfo_t *GetCorpseInfo( class_t class_ )
{
	clientInfo_t *match;
	char         *modelName;
	char         *skinName;

	modelName = BG_ClassModelConfig( class_ )->modelName;
	skinName = BG_ClassModelConfig( class_ )->skinName;

	match = &cgs.corpseinfo[ class_ ];

	if ( match->infoValid &&
	     !Q_stricmp( modelName, match->modelName ) &&
	     !Q_stricmp( skinName, match->skinName ) )
	{
		// this clientinfo is identical, so use its handles
		return match;
	}

	// happens when cg_lazyLoadModels is on
	Log::Verbose( "lazily loading class %d corpseinfo", class_ );
	CG_PrecacheClientInfo( class_, modelName, skinName );

	if ( match->infoValid &&
	     !Q_stricmp( modelName, match->modelName ) &&
	     !Q_stricmp( skinName, match->skinName ) )
	{
		return match;
	}

	//something has gone horribly wrong
	return nullptr;
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static bool CG_ScanForExistingClientInfo( clientInfo_t *ci )
{
	int          i;
	clientInfo_t *match;

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		match = &cgs.corpseinfo[ i ];

		if ( match->infoValid &&
		     !Q_stricmp( ci->modelName, match->modelName ) &&
		     !Q_stricmp( ci->skinName, match->skinName ) )
		{
			// this clientinfo is identical, so use its handles
			CG_CopyClientInfoModel( match, ci );

			return true;
		}
	}

	// shouldn't happen
	return false;
}

/*
======================
CG_PrecacheClientInfo
======================
*/
void CG_PrecacheClientInfo( class_t class_, const char *model, const char *skin )
{
	clientInfo_t *ci;

	ci = &cgs.corpseinfo[ class_ ];

	// the old value
	clientInfo_t newInfo{};

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

	newInfo.infoValid = true;

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
		Log::Notice(_( "%s^* renamed to %s"), old->name, new_->name );
	}

	if ( old->team != new_->team )
	{
		if ( new_->team == TEAM_NONE )
		{
			Log::Notice(_( "%s^* left the %s"), new_->name,
			           BG_TeamNamePlural( old->team ) );
		}
		else if ( old->team == TEAM_NONE )
		{
			Log::Notice(_( "%s^* joined the %s"), new_->name,
			           BG_TeamNamePlural( new_->team ) );
		}
		//else
		//{
		//	Log::Notice(_( "%s^* left the %s and joined the %s"),
		//	           new_->name, BG_TeamNamePlural( old->team ), BG_TeamNamePlural( new_->team ) );
		//}
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
	const char   *configstring;
	const char   *v;
	char         *slash;

	ci = &cgs.clientinfo[ clientNum ];

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );

	if ( !configstring[ 0 ] )
	{
		*ci = {};
		return; // player just left
	}

	// the old value
	clientInfo_t newInfo{};

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
	newInfo.infoValid = true;
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
	newAnimation = CG_AnimNumber( newAnimation );
	int oldAnimation = CG_AnimNumber( lf->old_animationNumber );

	if ( newAnimation < 0 || newAnimation >= MAX_PLAYER_TOTALANIMATIONS )
	{
		Sys::Drop( "Bad animation number: %i", newAnimation );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;

	if ( ci->skeletal )
	{
		debug_anim_current = lf->animationNumber;
		debug_anim_old = lf->old_animationNumber;

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

		if ( lf->old_animation->handle && oldSkeleton.numBones == skel->numBones )
		{
			if ( !trap_R_BuildSkeleton( &oldSkeleton, lf->old_animation->handle, lf->oldFrame, lf->frame, lf->blendlerp, lf->old_animation->clearOrigin ) )
			{
				Log::Warn( "Can't blend skeleton" );
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

	// HACK: If the previous animation was crouch and we are crouching, the frame number will be 0,
	//       but we want the model to still be crouching so we should actually be at the last frame.
	if ( newAnimation == LEGS_IDLECR && ( oldAnimation == LEGS_BACKCR ||
			oldAnimation == LEGS_WALKCR ) )
	{
		lf->frame = lf->oldFrame = lf->animation->numFrames - 1;
		lf->animationTime = lf->frameTime + anim->initialLerp;
	}

	if ( cg_debugAnim.Get() )
	{
		Log::Warn( "Anim: %i", newAnimation );
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
	bool animChanged = false;

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation )
	{
		CG_SetLerpFrameAnimation( ci, lf, newAnimation, skel );
		animChanged = true;
	}

	if ( ci->skeletal )
	{
		CG_RunMD5LerpFrame( lf, speedScale, animChanged );

		// blend old and current animation
		CG_BlendLerpFrame( lf );

		if( ci->team != TEAM_NONE )
			CG_BuildAnimSkeleton( lf, skel, &oldSkeleton );
	}
	else
	{
		CG_RunLerpFrame( lf, speedScale );
	}
}

static void CG_RunCorpseLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation )
{
	animation_t *anim;

	// debugging tool to get no animations
	if ( !cg_animSpeed.Get() )
	{
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation )
	{
		CG_SetLerpFrameAnimation( ci, lf, newAnimation, nullptr );

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
		if ( !trap_R_BuildSkeleton( &legsSkeleton, lf->animation->handle, anim->numFrames - 1, anim->numFrames - 1, 0, lf->animation->clearOrigin ) )
		{
			Log::Warn( "Can't build lf->skeleton" );
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
CG_SegmentAnimation
===============
*/
static void CG_SegmentAnimation( centity_t *cent, lerpFrame_t *lf, refSkeleton_t *skel, int anim, int *oldFrame, int *frame, float *backlerp )
{
	clientInfo_t *ci;
	int          clientNum;
	float        speedScale = 1.0f;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.Get() )
	{
		*oldFrame = *frame = 0;
		return;
	}

	ci = &cgs.clientinfo[ clientNum ];
	CG_RunPlayerLerpFrame( ci, lf, anim, skel, speedScale );

	*oldFrame = lf->oldFrame;
	*frame = lf->frame;
	*backlerp = lf->backlerp;
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

	if ( cg_noPlayerAnims.Get() )
	{
		return;
	}

	speedScale = 1;

	ci = &cgs.clientinfo[ clientNum ];

	// If we are attacking/taunting, and in motion, allow blending the two skeletons
	if ( ( CG_AnimNumber( cent->currentState.legsAnim ) >= NSPA_ATTACK1 &&
				CG_AnimNumber( cent->currentState.legsAnim ) <= NSPA_ATTACK3 ) ||
				CG_AnimNumber( cent->currentState.legsAnim ) == NSPA_GESTURE )
	{
		blend.type = refSkeletonType_t::SK_RELATIVE; // Tell game to blend

		if( CG_AnimNumber( cent->pe.nonseg.animationNumber ) <= NSPA_TURN &&
			CG_AnimNumber( cent->pe.nonseg.animationNumber ) != NSPA_GESTURE )
		{
			cent->pe.legs = cent->pe.nonseg;
		}
	}
	else
	{
		blend.type = refSkeletonType_t::SK_INVALID;
	}

	// do the shuffle turn frames locally
	if ( cent->pe.nonseg.yawing && CG_AnimNumber( cent->currentState.legsAnim ) == NSPA_STAND )
	{
		CG_RunPlayerLerpFrame( ci, &cent->pe.nonseg, NSPA_TURN, &legsSkeleton, speedScale );
	}
	else
	{
		CG_RunPlayerLerpFrame( ci, &cent->pe.nonseg, cent->currentState.legsAnim, &legsSkeleton, speedScale );
	}

	if ( blend.type == refSkeletonType_t::SK_RELATIVE )
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
                            float speed, float *angle, bool *swinging )
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
			*swinging = true;
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
			*swinging = false;
		}

		*angle = AngleMod( *angle + move );
	}
	else if ( swing < 0 )
	{
		move = cg.frametime * scale * -speed;

		if ( move <= swing )
		{
			move = swing;
			*swinging = false;
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
	if ( CG_AnimNumber( cent->currentState.legsAnim ) != LEGS_IDLE ||
	     CG_AnimNumber( cent->currentState.torsoAnim ) != TORSO_STAND )
	{
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = true; // always center
		cent->pe.torso.pitching = true; // always center
		cent->pe.legs.yawing = true; // always center
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
			Sys::Drop( "Bad player movement angle" );
		}
	}

	legsAngles[ YAW ] = headAngles[ YAW ] + movementOffsets[ dir ];
	torsoAngles[ YAW ] = headAngles[ YAW ] + 0.25 * movementOffsets[ dir ];

	// torso
	if ( cent->currentState.eFlags & EF_DEAD )
	{
		CG_SwingAngles( torsoAngles[ YAW ], 0, 0, cg_swingSpeed.Get(),
		                &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
		CG_SwingAngles( legsAngles[ YAW ], 0, 0, cg_swingSpeed.Get(),
		                &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
	}
	else
	{
		CG_SwingAngles( torsoAngles[ YAW ], 25, 90, cg_swingSpeed.Get(),
		                &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
		CG_SwingAngles( legsAngles[ YAW ], 40, 90, cg_swingSpeed.Get(),
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

			rotAngle = RAD2DEG( acosf( ( rotAngle - 1.0f ) / 2.0f ) );

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
			sFraction = - ( cosf( stLocal * M_PI ) + 1.0f ) / 2.0f;

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
		vec3_t ceilingNormal = { 0.0f, 0.0f, -1.0f };
		VectorCopy( ceilingNormal, surfNormal );
	}

	//make sure that WW transitions don't cause the swing stuff to go nuts
	if ( !VectorCompare( surfNormal, cent->pe.lastNormal ) )
	{
		//stop CG_SwingAngles having an eppy
		cent->pe.nonseg.yawAngle = localAngles[ YAW ];
		cent->pe.nonseg.yawing = false;
	}

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( CG_AnimNumber( cent->currentState.legsAnim ) != NSPA_STAND )
	{
		// if not standing still, always point all in the same direction
		cent->pe.nonseg.yawing = true; // always center
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
			Sys::Drop( "Bad player movement angle" );
		}
	}

	// torso
	if ( cent->currentState.eFlags & EF_DEAD )
	{
		CG_SwingAngles( localAngles[ YAW ], 0, 0, cg_swingSpeed.Get(),
		                &cent->pe.nonseg.yawAngle, &cent->pe.nonseg.yawing );
	}
	else
	{
		CG_SwingAngles( localAngles[ YAW ], 40, 90, cg_swingSpeed.Get(),
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
			Sys::Drop( "Bad animation number: %i", cent->jetpackAnim );


		if( JETPACK_USES_SKELETAL_ANIMATION )
		{
			if ( jetpackSkeleton.type != refSkeletonType_t::SK_INVALID )
			{
				oldSkeleton = jetpackSkeleton;

				if ( lf->old_animation != nullptr && lf->old_animation->handle )
				{
					if ( !trap_R_BuildSkeleton( &oldSkeleton, lf->old_animation->handle, lf->oldFrame, lf->frame, lf->blendlerp, lf->old_animation->clearOrigin ) )
					{
						Log::Warn( "Can't build old jetpack skeleton" );
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
	entityState_t *es = &cent->currentState;

	int held        = es->modelindex;
	int publicFlags = es->modelindex2;

	// jetpack model and effects
	if ( held & ( 1 << UP_JETPACK ) )
	{
		refEntity_t jetpack{};
		VectorCopy( torso->lightingOrigin, jetpack.lightingOrigin );
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
		refEntity_t radar{};
		VectorCopy( torso->lightingOrigin, radar.lightingOrigin );
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

		CG_Trace( &tr, es->pos.trBase, nullptr, nullptr, temp, es->number, MASK_SOLID, 0 );
		VectorCopy( tr.endpos, origin );

		size = 32.0f;

		if ( size > 0.0f )
		{
			CG_ImpactMark( cgs.media.creepShader, origin, up,
			               0.0f, 1.0f, 1.0f, 1.0f, 1.0f, false, size, true );
		}
	}
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

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson )
	{
		rf = RF_THIRD_PERSON; // only show in mirrors
	}
	else
	{
		rf = 0;
	}

	refEntity_t ent{};
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[ 2 ] += 48;
	ent.reType = refEntityType_t::RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA = Color::White;
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
	}
	else if ( cent->currentState.eFlags & EF_TYPING )
	{
		CG_PlayerFloatSprite( cent, cgs.media.balloonShader );
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
static bool CG_PlayerShadow( centity_t *cent, class_t class_ )
{
	vec3_t        end, mins, maxs;
	trace_t       trace;
	float         alpha;
	entityState_t *es = &cent->currentState;
	vec3_t        surfNormal = { 0.0f, 0.0f, 1.0f };

	BG_ClassBoundingBox( class_, mins, maxs, nullptr, nullptr, nullptr );
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

	if ( cg_shadows == shadowingMode_t::SHADOWING_NONE)
	{
		return false;
	}

	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	VectorMA( cent->lerpOrigin, -SHADOW_DISTANCE, surfNormal, end );

	CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID, 0, traceType_t::TT_AABB );

	// no shadow if too high
	if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid )
	{
		return false;
	}

	if ( cg_shadows > shadowingMode_t::SHADOWING_BLOB &&
	     cg_playerShadows.Get() ) {
		// add inverse shadow map
		{
		  CG_StartShadowCaster( cent->lerpOrigin, mins, maxs );
		}
	}

	if ( cg_shadows != shadowingMode_t::SHADOWING_BLOB) // no mark for stencil or projection shadows
	{
		return true;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal,
	               cent->pe.legs.yawAngle, 0.0f, 0.0f, 0.0f, alpha, false,
	               24.0f * BG_ClassModelConfig( class_ )->shadowScale, true );

	return true;
}

static void CG_PlayerShadowEnd()
{
	if ( cg_shadows == shadowingMode_t::SHADOWING_NONE)
	{
		return;
	}

	if ( cg_shadows > shadowingMode_t::SHADOWING_BLOB &&
	     cg_playerShadows.Get() ) {
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

	if ( cg_shadows == shadowingMode_t::SHADOWING_NONE)
	{
		return;
	}

	BG_ClassBoundingBox( class_, mins, maxs, nullptr, nullptr, nullptr );

	VectorCopy( cent->lerpOrigin, end );
	end[ 2 ] += mins[ 2 ];

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = CM_PointContents( end, 0 );

	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) )
	{
		return;
	}

	VectorCopy( cent->lerpOrigin, start );
	start[ 2 ] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = CM_PointContents( start, 0 );

	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
	{
		return;
	}

	// trace down to find the surface
	CM_BoxTrace( &trace, start, end, nullptr, nullptr, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ), 0, traceType_t::TT_AABB );

	if ( trace.fraction == 1.0f )
	{
		return;
	}

	CG_ImpactMark( cgs.media.wakeMarkShader, trace.endpos, trace.plane.normal,
	               cent->pe.legs.yawAngle, 1.0f, 1.0f, 1.0f, 1.0f, false,
	               32.0f * BG_ClassModelConfig( class_ )->shadowScale, true );
}

#define TRACE_DEPTH    32.0f

/*
===============
CG_Player
===============
*/
// TODO: Break this function up and reduce code copy-paste between md3 and skeletal code paths.
void CG_Player( centity_t *cent )
{
	clientInfo_t *ci;
	int           clientNum;
	int           renderfx;
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
		Sys::Drop( "Bad clientNum on player entity" );
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

	if ( cg_drawBBOX.Get() && cg.renderingThirdPerson )
	{
		vec3_t mins, maxs;

		BG_ClassBoundingBox( class_, mins, maxs, nullptr, nullptr, nullptr );
		CG_DrawBoundingBox( cg_drawBBOX.Get(), cent->lerpOrigin, mins, maxs );
	}

	// NOTE: legs is used for nonsegmented and skeletal models
	//       this helps reduce code to be changed
	refEntity_t legs{}, torso{}, head{};

	VectorCopy( cent->lerpAngles, angles );
	AnglesToAxis( cent->lerpAngles, tempAxis );

	// rotate lerpAngles to floor
	if ( es->eFlags & EF_WALLCLIMB &&
	     BG_RotateAxis( es->angles2, tempAxis, tempAxis2, true, es->eFlags & EF_WALLCLIMBCEILING ) )
	{
		AxisToAngles( tempAxis2, angles );
	}
	else
	{
		VectorCopy( cent->lerpAngles, angles );
	}

	// normalise the pitch
	if ( angles[ PITCH ] < -180.0f )
	{
		angles[ PITCH ] += 360.0f;
	}

	if ( ci->skeletal )
	{
		vec3_t legsAngles, torsoAngles, headAngles;
		vec3_t playerOrigin, mins, maxs;

		// TODO: Don't use GENDER to differentiate between aliens and humans
		if ( ci->gender != GENDER_NEUTER )
		{
			CG_PlayerAngles( cent, angles, legsAngles, torsoAngles, headAngles );
			AnglesToAxis( legsAngles, legs.axis );
		}
		else
		{
			CG_PlayerNonSegAxis( cent, angles, legs.axis );
		}

		AxisCopy( legs.axis, tempAxis );

		// rotate the legs axis to back to the wall
		if ( es->eFlags & EF_WALLCLIMB && BG_RotateAxis( es->angles2, legs.axis, tempAxis, false, es->eFlags & EF_WALLCLIMBCEILING ) )
		{
			AxisCopy( tempAxis, legs.axis );
		}

		// smooth out WW transitions so the model doesn't hop around
		CG_PlayerWWSmoothing( cent, legs.axis, legs.axis );

		AxisCopy( tempAxis, cent->pe.lastAxis );

		// get the animation state (after rotation, to allow feet shuffle)
		if ( ci->gender != GENDER_NEUTER )
		{
					bool yawing = cent->pe.legs.yawing && CG_AnimNumber( cent->currentState.legsAnim ) == LEGS_IDLE;
		CG_SegmentAnimation( cent, &cent->pe.legs, &legsSkeleton, yawing ? LEGS_TURN : cent->currentState.legsAnim,
												 &legs.oldframe, &legs.frame, &legs.backlerp );
		CG_SegmentAnimation( cent, &cent->pe.torso, &torsoSkeleton, cent->currentState.torsoAnim,
												 &torso.oldframe, &torso.frame, &torso.backlerp );

		}
		else
		{
			// Special code for blending alien skeletons.
			CG_PlayerMD5AlienAnimation( cent );
		}

		// add the talk baloon or disconnect icon
		CG_PlayerSprites( cent );

		// add the shadow
		if ( ( es->number == cg.snap->ps.clientNum && cg.renderingThirdPerson ) ||
			 es->number != cg.snap->ps.clientNum )
		{
			CG_PlayerShadow( cent, class_ );
		}

		// add a water splash if partially in and out of water
		CG_PlayerSplash( cent, class_ );

		renderfx |= RF_LIGHTING_ORIGIN; // use the same origin for all

		// add the body
		legs.hModel = ci->bodyModel;
		legs.customSkin = ci->bodySkin;

		if ( !legs.hModel )
		{
			Log::Warn( "No body model for player %i", clientNum );
			return;
		}

		legs.renderfx = renderfx;

		BG_ClassBoundingBox( class_, mins, maxs, nullptr, nullptr, nullptr );

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
			CG_CapTrace( &tr, start, nullptr, nullptr, end, es->number, MASK_PLAYERSOLID, 0 );

			// if the trace misses completely then just use legs.origin
			// apparently capsule traces are "smaller" than box traces
			if ( tr.fraction != 1.0f )
			{
				VectorCopy( tr.endpos, playerOrigin );

				// MD5 player models have their model origin at (0 0 0)
				VectorMA( playerOrigin, 0, surfNormal, legs.origin );
			}
			else
			{
				VectorCopy( cent->lerpOrigin, playerOrigin );

				// MD5 player models have their model origin at (0 0 0)
				VectorMA( cent->lerpOrigin, -TRACE_DEPTH, surfNormal, legs.origin );
			}

			VectorCopy( legs.origin, legs.lightingOrigin );
			VectorCopy( legs.origin, legs.oldorigin );  // don't positionally lerp at all
		}
		else
		{
			VectorCopy( cent->lerpOrigin, playerOrigin );
			VectorCopy( playerOrigin, legs.origin );
			legs.origin[ 0 ] -= ci->headOffset[ 0 ];
			legs.origin[ 1 ] -= ci->headOffset[ 1 ];
			legs.origin[ 2 ] -= 22 + ci->headOffset[ 2 ];
			VectorMA( legs.origin, BG_ClassModelConfig( class_ )->zOffset, surfNormal, legs.origin );
		}

		VectorCopy( legs.origin, legs.lightingOrigin );
		VectorCopy( legs.origin, legs.oldorigin );  // don't positionally lerp at all

		if ( ci->gender != GENDER_NEUTER )
		{
			// copy legs skeleton to have a base
			legs.skeleton = torsoSkeleton;
			if ( torsoSkeleton.numBones != legsSkeleton.numBones )
			{

				// seems only to happen when switching from an MD3 model to an MD5 model
				// while spectating (switching between players on the human team)
				// - don't treat as fatal, but doing so will (briefly?) cause rendering
				// glitches if chasing; also, brief spam
				Log::Warn( S_SKIPNOTIFY "cent->pe.legs.skeleton.numBones != cent->pe.torso.skeleton.numBones" );
			}

			// apply skeleton modifiers
			SkeletonModifierContext ctx;
			ctx.es = es;
			ctx.torsoYawAngle = torsoAngles[YAW];
			ctx.pitchAngle = cent->lerpAngles[PITCH];
			ctx.legsSkeleton = &legsSkeleton;
			for ( auto& mod : ci->modifiers )
			{
				mod->Apply( ctx, &legs.skeleton );
			}

		}
		else
		{
			legs.skeleton = legsSkeleton;
		}

		// transform relative bones to absolute ones required for vertex skinning and tag attachments
		CG_TransformSkeleton( &legs.skeleton, ci->modelScale );

		// add the gun / barrel / flash
		if ( es->weapon != WP_NONE )
		{
			CG_AddPlayerWeapon( &legs, nullptr, cent );
		}

		CG_PlayerUpgrades( cent, &legs );

		// add skeletal model to renderer
		legs.altShaderIndex = altShaderIndex;
		trap_R_AddRefEntityToScene( &legs );

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
	     BG_RotateAxis( es->angles2, legs.axis, tempAxis, false, es->eFlags & EF_WALLCLIMBCEILING ) )
	{
		AxisCopy( tempAxis, legs.axis );
	}

	//smooth out WW transitions so the model doesn't hop around
	CG_PlayerWWSmoothing( cent, legs.axis, legs.axis );

	AxisCopy( tempAxis, cent->pe.lastAxis );

	// get the animation state (after rotation, to allow feet shuffle)
	if ( !ci->nonsegmented )
	{
		bool yawing = cent->pe.legs.yawing && CG_AnimNumber( cent->currentState.legsAnim ) == LEGS_IDLE;
		CG_SegmentAnimation( cent, &cent->pe.legs, nullptr, yawing ? LEGS_TURN : cent->currentState.legsAnim,
												 &legs.oldframe, &legs.frame, &legs.backlerp );
		CG_SegmentAnimation( cent, &cent->pe.torso, nullptr, cent->currentState.torsoAnim,
												 &torso.oldframe, &torso.frame, &torso.backlerp );
	}
	else
	{
		bool yawing = cent->pe.legs.yawing && CG_AnimNumber( cent->currentState.legsAnim ) == NSPA_STAND;
		CG_SegmentAnimation( cent, &cent->pe.legs, nullptr, yawing ? NSPA_TURN : cent->currentState.legsAnim,
												 &legs.oldframe, &legs.frame, &legs.backlerp );
	}

	// add the talk baloon or disconnect icon
	CG_PlayerSprites( cent );

	// add the shadow
	if ( ( es->number == cg.snap->ps.clientNum && cg.renderingThirdPerson ) ||
	     es->number != cg.snap->ps.clientNum )
	{
		CG_PlayerShadow( cent, class_ );
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

		BG_ClassBoundingBox( class_, mins, maxs, nullptr, nullptr, nullptr );

		VectorMA( legs.origin, -TRACE_DEPTH, surfNormal, end );
		VectorMA( legs.origin, 1.0f, surfNormal, start );
		CG_CapTrace( &tr, start, mins, maxs, end, es->number, MASK_PLAYERSOLID, 0 );

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

		legs.nonNormalizedAxes = true;
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
			CG_AddPlayerWeapon( &torso, nullptr, cent );
		}
		else
		{
			CG_AddPlayerWeapon( &legs, nullptr, cent );
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
	clientInfo_t  *ci;
	entityState_t *es = &cent->currentState;
	int           renderfx;
	vec3_t        origin, liveZ, deadZ, deadMax;
	float         scale;

	ci = GetCorpseInfo( (class_t) es->clientNum );

	if ( ci == nullptr )
	{
		Log::Warn( "Missing corpseinfo on corpse entity: %d", es->clientNum );
		return;
	}

	refEntity_t legs{}, torso{}, head{};

	VectorCopy( cent->lerpOrigin, origin );
	BG_ClassBoundingBox( es->clientNum, liveZ, nullptr, nullptr, deadZ, deadMax );
	origin[ 2 ] -= ( liveZ[ 2 ] - deadZ[ 2 ] );

	if( ci->skeletal )
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
	if ( cg_noPlayerAnims.Get() )
	{
		legs.oldframe = legs.frame = torso.oldframe = torso.frame = 0;
	}
	else if ( ci->skeletal )
	{
		if ( ci->gender == GENDER_NEUTER )
		{
			memset( &cent->pe.nonseg, 0, sizeof( lerpFrame_t ) );
			CG_RunCorpseLerpFrame( ci, &cent->pe.nonseg, es->legsAnim );
			legs.oldframe = cent->pe.nonseg.oldFrame;
			legs.frame = cent->pe.nonseg.frame;
			legs.backlerp = cent->pe.nonseg.backlerp;
		}
		else
		{
			memset( &cent->pe.legs, 0, sizeof( lerpFrame_t ) );
			CG_RunCorpseLerpFrame( ci, &cent->pe.legs, es->legsAnim );
			legs.oldframe = cent->pe.legs.oldFrame;
			legs.frame = cent->pe.legs.frame;
			legs.backlerp = cent->pe.legs.backlerp;
		}
	}
	else if ( !ci->nonsegmented )
	{
		memset( &cent->pe.legs, 0, sizeof( lerpFrame_t ) );
		CG_RunPlayerLerpFrame( ci, &cent->pe.legs, es->legsAnim, nullptr, 1 );
		legs.oldframe = cent->pe.legs.oldFrame;
		legs.frame = cent->pe.legs.frame;
		legs.backlerp = cent->pe.legs.backlerp;

		memset( &cent->pe.torso, 0, sizeof( lerpFrame_t ) );
		CG_RunPlayerLerpFrame( ci, &cent->pe.torso, es->torsoAnim, nullptr, 1 );
		torso.oldframe = cent->pe.torso.oldFrame;
		torso.frame = cent->pe.torso.frame;
		torso.backlerp = cent->pe.torso.backlerp;
	}
	else
	{
		memset( &cent->pe.nonseg, 0, sizeof( lerpFrame_t ) );
		CG_RunPlayerLerpFrame( ci, &cent->pe.nonseg, es->legsAnim, nullptr, 1 );
		legs.oldframe = cent->pe.nonseg.oldFrame;
		legs.frame = cent->pe.nonseg.frame;
		legs.backlerp = cent->pe.nonseg.backlerp;
	}

	// add the shadow
	CG_PlayerShadow( cent, (class_t) es->clientNum );

	// get the player model information
	renderfx = RF_LIGHTING_ORIGIN; // use the same origin for all

	//
	// add the legs
	//
	if ( ci->skeletal )
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
	legs.renderfx = renderfx;
	legs.origin[ 2 ] += BG_ClassModelConfig( es->clientNum )->zOffset;
	VectorCopy( legs.origin, legs.oldorigin );  // don't positionally lerp at all

	//rescale the model
	scale = BG_ClassModelConfig( es->clientNum )->modelScale;

	if ( scale != 1.0f && !ci->skeletal )
	{
		VectorScale( legs.axis[ 0 ], scale, legs.axis[ 0 ] );
		VectorScale( legs.axis[ 1 ], scale, legs.axis[ 1 ] );
		VectorScale( legs.axis[ 2 ], scale, legs.axis[ 2 ] );

		legs.nonNormalizedAxes = true;
	}

	legs.altShaderIndex = CG_ALTSHADER_DEAD;
	trap_R_AddRefEntityToScene( &legs );

	// if the model failed, allow the default nullmodel to be displayed. Also, if MD5, no need to add other parts
	if ( !legs.hModel || ci->skeletal )
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
	cent->extrapolated = false;

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
	cent->pe.legs.yawing = false;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = false;

	memset( &cent->pe.torso, 0, sizeof( cent->pe.torso ) );
	cent->pe.torso.yawAngle = cent->rawAngles[ YAW ];
	cent->pe.torso.yawing = false;
	cent->pe.torso.pitchAngle = cent->rawAngles[ PITCH ];
	cent->pe.torso.pitching = false;

	memset( &cent->pe.nonseg, 0, sizeof( cent->pe.nonseg ) );
	cent->pe.nonseg.yawAngle = cent->rawAngles[ YAW ];
	cent->pe.nonseg.yawing = false;
	cent->pe.nonseg.pitchAngle = cent->rawAngles[ PITCH ];
	cent->pe.nonseg.pitching = false;
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

	trap_S_StartSound( org, ENTITYNUM_WORLD, soundChannel_t::CHAN_AUTO, cgs.media.disconnectSound );

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

	best = nullptr;
	bestlen = 3.0f * 8192.0f * 8192.0f;

	for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++ )
	{
		eloc = &cg_entities[ i ];

		if ( !eloc->valid || eloc->currentState.eType != entityType_t::ET_LOCATION )
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

centity_t *CG_GetPlayerLocation()
{
	vec3_t    origin;

	VectorCopy( cg.predictedPlayerState.origin, origin );
	return CG_GetLocation( origin );
}

void CG_InitClasses()
{
	int i;

	memset( cg_classes, 0, sizeof( cg_classes ) );

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		const char *icon = BG_Class( i )->icon;

		if ( icon )
		{
			cg_classes[ i ].classIcon = trap_R_RegisterShader( icon, (RegisterShaderFlags_t) ( RSF_NOMIP ) );

			if ( !cg_classes[ i ].classIcon )
			{
				Log::Warn( "Failed to load class icon file %s", icon );
			}
		}
	}
}
