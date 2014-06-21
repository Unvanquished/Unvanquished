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

#include "cg_local.h"

static const char *const cg_buildableSoundNames[ MAX_BUILDABLE_ANIMATIONS ] =
{
	"idle1.wav",
	"idle2.wav",
	"powerdown.wav",
	"idle_unpowered.wav",
	"construct1.wav",
	"construct2.wav",
	"attack1.wav",
	"attack2.wav",
	"spawn1.wav",
	"spawn2.wav",
	"pain1.wav",
	"pain2.wav",
	"destroy1.wav",
	"destroy2.wav",
	"destroyed.wav"
};

// MD5 buildable animation info helper macro
#define CG_ANIM( b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15 ) \
	( (   1   <<  1 ) | ( !!b2  <<  2 ) | ( !!b3  <<  3 ) | ( !!b4  <<  4 ) | \
	  ( !!b5  <<  5 ) | ( !!b6  <<  6 ) | ( !!b7  <<  7 ) | ( !!b8  <<  8 ) | \
	  ( !!b9  <<  9 ) | ( !!b10 << 10 ) | ( !!b11 << 11 ) | ( !!b12 << 12 ) | \
	  ( !!b13 << 13 ) | ( !!b14 << 14 ) | ( !!b15 << 15 ) \
	)

// MD5 buildable animation names, flags and fallbacks
static const struct {
	const char            *name;
	qboolean              loop, reversed, clearOrigin;
	buildableAnimNumber_t fallback;
	qboolean              fallbackReversed; // true if the fallback's reversed flag is to be inverted
} animTypes[ MAX_BUILDABLE_ANIMATIONS ] = {
	{ "" }, // unused
	{ "idle",              qtrue,  qfalse, qfalse, BANIM_NONE,     qfalse },
	{ "idle2",     	       qtrue,  qfalse, qfalse, BANIM_IDLE1,    qfalse },
	{ "powerdown",         qfalse, qfalse, qfalse, BANIM_IDLE1,    qfalse },
	{ "idle_unpowered",    qtrue,  qfalse, qfalse, BANIM_IDLE1,    qfalse },
	{ "construct", 	       qfalse, qfalse, qfalse, BANIM_IDLE1,    qfalse },
	{ "construct2",        qfalse, qfalse, qfalse, BANIM_IDLE1,    qfalse },
	{ "attack",            qfalse, qfalse, qfalse, BANIM_IDLE1,    qfalse },
	{ "attack2",           qfalse, qfalse, qfalse, BANIM_ATTACK1,  qtrue  },
	{ "spawn",     	       qfalse, qfalse, qfalse, BANIM_IDLE1,    qfalse },
	{ "spawn2",            qfalse, qfalse, qfalse, BANIM_IDLE1,    qfalse },
	{ "pain",              qfalse, qfalse, qfalse, BANIM_IDLE1,    qfalse },
	{ "pain2",             qtrue,  qfalse, qfalse, BANIM_PAIN1,    qfalse },
	{ "destroy",  	       qfalse, qfalse, qfalse, BANIM_IDLE1,    qfalse },
	{ "destroy_unpowered", qfalse, qfalse, qfalse, BANIM_DESTROY1, qfalse },
	{ "destroyed",         qfalse, qfalse, qfalse, BANIM_IDLE1,    qfalse },
};

// Bitmaps for each buildable type, defining which animations are to be initialised
// We expect that all have idle animations
static const int animLoading[] = {
	0,
	//       idle2   pwrdwn  idlunp  cnstr   cnstr2  attack   attack2 spawn   spawn2  pain    pain2   dstry   dstry2  dstryed
	CG_ANIM( qtrue,  qfalse, qfalse, qtrue,  qtrue,  qtrue,   qtrue,  qtrue,  qfalse, qtrue,  qtrue,  qtrue,  qtrue,  qtrue  ), // BA_A_SPAWN
	CG_ANIM( qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qfalse, qtrue  ), // BA_A_OVERMIND
	CG_ANIM( qtrue,  qtrue,  qtrue,  qtrue,  qfalse, qtrue,   qtrue,  qfalse, qfalse, qtrue,  qtrue,  qtrue,  qtrue,  qtrue  ), // BA_A_BARRICADE
	CG_ANIM( qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_A_ACIDTUBE
	CG_ANIM( qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_A_TRAPPER
	CG_ANIM( qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_A_BOOSTER
	CG_ANIM( qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_A_HIVE
	CG_ANIM( qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_A_LEECH

	CG_ANIM( qtrue,  qtrue,  qtrue,  qtrue,  qtrue,  qtrue,   qtrue,  qtrue,  qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_H_SPAWN
	CG_ANIM( qfalse, qtrue,  qtrue,  qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_H_TURRET
	CG_ANIM( qfalse, qtrue,  qtrue,  qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_H_TESLAGEN
	CG_ANIM( qfalse, qtrue,  qtrue,  qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_H_ARMOURY
	CG_ANIM( qtrue,  qtrue,  qtrue,  qtrue,  qtrue,  qtrue,   qtrue,  qfalse, qfalse, qtrue,  qtrue,  qtrue,  qtrue,  qtrue  ), // BA_H_MEDISTAT
	CG_ANIM( qfalse, qtrue,  qtrue,  qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_H_DRILL
	CG_ANIM( qfalse, qtrue,  qtrue,  qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_H_REACTOR
	CG_ANIM( qfalse, qtrue,  qtrue,  qtrue,  qfalse, qtrue,   qfalse, qfalse, qfalse, qtrue,  qfalse, qtrue,  qtrue,  qtrue  ), // BA_H_REPEATER
};

static sfxHandle_t defaultAlienSounds[ MAX_BUILDABLE_ANIMATIONS ];
static sfxHandle_t defaultHumanSounds[ MAX_BUILDABLE_ANIMATIONS ];

static refSkeleton_t bSkeleton;
static refSkeleton_t oldbSkeleton;

/*
===================
CG_AlienBuildableExplosion

Generated a bunch of gibs launching out from a location
===================
*/
void CG_AlienBuildableExplosion( vec3_t origin, vec3_t dir )
{
	particleSystem_t *ps;

	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.alienBuildableExplosion );

	//particle system
	ps = CG_SpawnNewParticleSystem( cgs.media.alienBuildableDestroyedPS );

	if ( CG_IsParticleSystemValid( &ps ) )
	{
		CG_SetAttachmentPoint( &ps->attachment, origin );
		CG_SetParticleSystemNormal( ps, dir );
		CG_AttachToPoint( &ps->attachment );
	}
}

/*
=================
CG_HumanBuildableDieing

Called for human buildables that are about to blow up
=================
*/
void CG_HumanBuildableDying( buildable_t buildable, vec3_t origin )
{
	switch ( buildable )
	{
		case BA_H_REPEATER:
		case BA_H_REACTOR:
			trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.humanBuildableDying );
		default:
			return;
	}
}

/*
=================
CG_HumanBuildableExplosion

Called for human buildables as they are destroyed
=================
*/
void CG_HumanBuildableExplosion( buildable_t buildable, vec3_t origin, vec3_t dir )
{
	particleSystem_t *explosion = NULL;
	particleSystem_t *nova = NULL;

	if ( buildable == BA_H_REPEATER || buildable == BA_H_REACTOR )
	{
		nova = CG_SpawnNewParticleSystem( cgs.media.humanBuildableNovaPS );
	}

	trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.humanBuildableExplosion );
	explosion = CG_SpawnNewParticleSystem( cgs.media.humanBuildableDestroyedPS );

	if ( CG_IsParticleSystemValid( &nova ) )
	{
		CG_SetAttachmentPoint( &nova->attachment, origin );
		CG_SetParticleSystemNormal( nova, dir );
		CG_AttachToPoint( &nova->attachment );
	}

	if ( CG_IsParticleSystemValid( &explosion ) )
	{
		CG_SetAttachmentPoint( &explosion->attachment, origin );
		CG_SetParticleSystemNormal( explosion, dir );
		CG_AttachToPoint( &explosion->attachment );
	}
}

static void CG_Creep( centity_t *cent )
{
	int     msec;
	float   size, frac;
	trace_t tr;
	vec3_t  temp;
	int     time;
	const buildableAttributes_t *attr = BG_Buildable( cent->currentState.modelindex );

	time = cent->currentState.time;

	// should the creep be growing or receding?
	if ( time >= 0 )
	{
		int scaleUpTime = attr->buildTime;

		msec = cg.time - time;

		if ( msec >= 0 && msec < scaleUpTime )
		{
			frac = ( float ) sin ( 0.5f * msec / scaleUpTime * M_PI );
		}
		else
		{
			frac = 1.0f;
		}
	}
	else
	{
		msec = cg.time + time;

		if ( msec >= 0 && msec < CREEP_SCALEDOWN_TIME )
		{
			frac = ( float ) cos ( 0.5f * msec / CREEP_SCALEDOWN_TIME * M_PI );
		}
		else
		{
			frac = 0.0f;
		}
	}

	size = attr->creepSize * frac;

	// TODO: Add comment explaining what this does
	VectorCopy( cent->currentState.origin2, temp );
	VectorScale( temp, -attr->creepSize, temp );
	VectorAdd( temp, cent->lerpOrigin, temp );

	CG_Trace( &tr, cent->lerpOrigin, NULL, NULL, temp, cent->currentState.number, MASK_PLAYERSOLID );

	if ( size > 0.0f && tr.fraction < 1.0f )
	{
		CG_ImpactMark( cgs.media.creepShader, tr.endpos, cent->currentState.origin2,
		               0.0f, 1.0f, 1.0f, 1.0f, 1.0f, qfalse, size, qtrue );
	}
}

/*
==================
CG_OnFire

Sets buildable particle system to a fire effect if buildable is burning
==================
*/
static void CG_OnFire( centity_t *cent )
{
	entityState_t *es = &cent->currentState;
	team_t        team = BG_Buildable( es->modelindex )->team;

	if ( es->eType != ET_BUILDABLE )
	{
		return;
	}

	if ( !( es->eFlags & EF_B_ONFIRE ) )
	{
		if ( CG_IsParticleSystemValid( &cent->buildableStatusPS ) )
		{
			CG_DestroyParticleSystem( &cent->buildableStatusPS );
		}

		return;
	}

	switch ( team )
	{
		case TEAM_ALIENS:
			if ( !CG_IsParticleSystemValid( &cent->buildableStatusPS ) )
			{
				cent->buildableStatusPS = CG_SpawnNewParticleSystem( cgs.media.alienBuildableBurnPS );
			}
			break;

		default:
			// human buildables cannot burn … yet
			return;
	}

	if ( CG_IsParticleSystemValid( &cent->buildableStatusPS ) )
	{
		CG_SetAttachmentCent( &cent->buildableStatusPS->attachment, cent );
		CG_AttachToCent( &cent->buildableStatusPS->attachment );
	}
}

/*
======================
CG_ParseBuildableAnimationFile

Read a configuration file containing animation counts and rates
models/buildables/hivemind/animation.cfg, etc
======================
*/
static qboolean CG_ParseBuildableAnimationFile( const char *filename, buildable_t buildable )
{
	char         *text_p;
	int          len;
	int          i;
	char         *token;
	float        fps;
	char         text[ 20000 ];
	fileHandle_t f;
	animation_t  *animations;

	animations = cg_buildables[ buildable ].animations;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len < 0 )
	{
		return qfalse;
	}

	if ( len == 0 || len + 1 >= (int) sizeof( text ) )
	{
		trap_FS_FCloseFile( f );
		CG_Printf( len == 0 ? "File %s is empty\n" : "File %s is too long\n", filename );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	// read information for each frame
	for ( i = BANIM_NONE + 1; i < MAX_BUILDABLE_ANIMATIONS; i++ )
	{
		token = COM_Parse( &text_p );

		if ( !*token )
		{
			break;
		}

		animations[ i ].firstFrame = atoi( token );

		token = COM_Parse( &text_p );

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

		token = COM_Parse( &text_p );

		if ( !*token )
		{
			break;
		}

		animations[ i ].loopFrames = atoi( token );

		token = COM_Parse( &text_p );

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

	if ( i != MAX_BUILDABLE_ANIMATIONS )
	{
		CG_Printf( "Error parsing animation file: %s\n", filename );
		return qfalse;
	}

	return qtrue;
}

/*
======================
CG_ParseBuildableSoundFile

Read a configuration file containing sound properties
sound/buildables/hivemind/sound.cfg, etc
======================
*/
static qboolean CG_ParseBuildableSoundFile( const char *filename, buildable_t buildable )
{
	char         *text_p;
	int          len;
	int          i;
	char         *token;
	char         text[ 20000 ];
	fileHandle_t f;
	sound_t      *sounds;

	sounds = cg_buildables[ buildable ].sounds;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len < 0 )
	{
		return qfalse;
	}

	if ( len == 0 || len + 1 >= (int) sizeof( text ) )
	{
		trap_FS_FCloseFile( f );
		CG_Printf( len == 0 ? "File %s is empty\n" : "File %s is too long\n", filename );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	// read information for each frame
	for ( i = BANIM_NONE + 1; i < MAX_BUILDABLE_ANIMATIONS; i++ )
	{
		token = COM_Parse( &text_p );

		if ( !*token )
		{
			break;
		}

		sounds[ i ].enabled = atoi( token );

		token = COM_Parse( &text_p );

		if ( !*token )
		{
			break;
		}

		sounds[ i ].looped = atoi( token );
	}

	if ( i != MAX_BUILDABLE_ANIMATIONS )
	{
		CG_Printf( "Error parsing sound file: %s\n", filename );
		return qfalse;
	}

	return qtrue;
}

static qboolean CG_RegisterBuildableAnimation( buildableInfo_t *ci, const char *modelName, int anim, const char *animName,
    qboolean loop, qboolean reversed, qboolean clearOrigin, qboolean iqm )
{
	char filename[ MAX_QPATH ];
	int  frameRate;

	if ( iqm )
	{
		Com_sprintf( filename, sizeof( filename ), "models/buildables/%s/%s.iqm:%s", modelName, modelName, animName );
		ci->animations[ anim ].handle = trap_R_RegisterAnimation( filename );
	}

	else
	{
		Com_sprintf( filename, sizeof( filename ), "models/buildables/%s/%s.md5anim", modelName, animName );
		ci->animations[ anim ].handle = trap_R_RegisterAnimation( filename );
	}

	if ( !ci->animations[ anim ].handle )
	{
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

/*
===============
CG_InitBuildables

Initialises the animation db
===============
*/
void CG_InitBuildables( void )
{
	char         filename[ MAX_QPATH ];
	char         soundfile[ MAX_QPATH ];
	const char   *buildableName;
	const char   *buildableIcon;
	const char   *modelFile;
	int          i;
	int          j;
	int          n;
	fileHandle_t f;

	memset( cg_buildables, 0, sizeof( cg_buildables ) );

	//default sounds
	for ( j = BANIM_NONE + 1; j < MAX_BUILDABLE_ANIMATIONS; j++ )
	{
		strcpy( soundfile, cg_buildableSoundNames[ j - 1 ] );

		Com_sprintf( filename, sizeof( filename ), "sound/buildables/alien/%s", soundfile );
		defaultAlienSounds[ j ] = trap_S_RegisterSound( filename, qfalse );

		Com_sprintf( filename, sizeof( filename ), "sound/buildables/human/%s", soundfile );
		defaultHumanSounds[ j ] = trap_S_RegisterSound( filename, qfalse );
	}

	cg.buildablesFraction = 0.0f;

	for ( i = BA_NONE + 1; i < BA_NUM_BUILDABLES; i++ )
	{
		buildableInfo_t *bi = &cg_buildables[ i ];
		qboolean         iqm = qfalse;

		buildableName = BG_Buildable( i )->name;
		//Load models
		//Prefer md5 models over md3

		if ( cg_highPolyBuildableModels.integer && ( bi->models[ 0 ] = trap_R_RegisterModel( va( "models/buildables/%s/%s.iqm", buildableName, buildableName ) ) ) )
		{
			bi->md5 = qtrue;
			iqm = qtrue;
		}
		else if ( cg_highPolyBuildableModels.integer && ( bi->models[ 0 ] = trap_R_RegisterModel( va( "models/buildables/%s/%s.md5mesh", buildableName, buildableName ) ) ) )
		{
			bi->md5 = qtrue;
		}
		else
		{
			bi->md5 = qfalse;

			for ( j = 0; j < MAX_BUILDABLE_MODELS; j++ )
			{
				modelFile = BG_BuildableModelConfig( i )->models[ j ];

				if ( strlen( modelFile ) > 0 )
				{
					cg_buildables[ i ].models[ j ] = trap_R_RegisterModel( modelFile );
				}
			}
		}

		// If an md5mesh is found, register md5anims
		if ( bi->md5 )
		{
			for ( n = BANIM_NONE + 1; n < MAX_BUILDABLE_ANIMATIONS; n++ )
			{
				if ( animLoading[ i ] & ( 1 << n ) )
				{
					if ( !CG_RegisterBuildableAnimation( bi, buildableName, n, animTypes[ n ].name, animTypes[ n ].loop, animTypes[ n ].reversed, animTypes[ n ].clearOrigin, iqm ) )
					{
						int o = (int) animTypes[ n ].fallback;

						if ( o == BANIM_NONE )
						{
							// missing animation file, no fallback defined
							Com_Printf( S_WARNING "Failed to load animation file '%s' for model '%s'\n", animTypes[ n ].name, buildableName );
						}
						else if ( !bi->animations[ o ].handle )
						{
							// no animation file, fallback wasn't loaded
							if ( cg_debugAnim.integer )
							{
								Com_Printf( S_WARNING "No fallback for animation file '%s' for model '%s'\n", animTypes[ n ].name, buildableName );
							}
						}
						else // valid fallback
						{
							// copy the animation data
							bi->animations[ n ] = bi->animations[ o ];

							// and reverse it if needed
							if ( animTypes[ n ].fallbackReversed )
							{
									bi->animations[ n ].reversed = !bi->animations[ n ].reversed;
							}
						}
					}
				}
			}
		}
		else // Not using md5s, fall back to md3s
		{
			//animation.cfg
			Com_sprintf( filename, sizeof( filename ), "models/buildables/%s/animation.cfg", buildableName );

			if ( !CG_ParseBuildableAnimationFile( filename, (buildable_t) i ) )
			{
				Com_Printf( S_WARNING "failed to load animation file %s\n", filename );
			}
		}

		//sound.cfg
		Com_sprintf( filename, sizeof( filename ), "sound/buildables/%s/sound.cfg", buildableName );

		if ( !CG_ParseBuildableSoundFile( filename, (buildable_t) i ) )
		{
			Com_Printf( S_WARNING "failed to load sound file %s\n", filename );
		}

		//sounds
		for ( j = BANIM_NONE + 1; j < MAX_BUILDABLE_ANIMATIONS; j++ )
		{
			strcpy( soundfile, cg_buildableSoundNames[ j - 1 ] );
			Com_sprintf( filename, sizeof( filename ), "sound/buildables/%s/%s", buildableName, soundfile );

			if ( cg_buildables[ i ].sounds[ j ].enabled )
			{
				if ( trap_FS_FOpenFile( filename, &f, FS_READ ) > 0 )
				{
					//file exists so close it
					trap_FS_FCloseFile( f );

					cg_buildables[ i ].sounds[ j ].sound = trap_S_RegisterSound( filename, qfalse );
				}
				else
				{
					//file doesn't exist - use default
					if ( BG_Buildable( i )->team == TEAM_ALIENS )
					{
						cg_buildables[ i ].sounds[ j ].sound = defaultAlienSounds[ j ];
					}
					else
					{
						cg_buildables[ i ].sounds[ j ].sound = defaultHumanSounds[ j ];
					}
				}
			}
		}

		//icon
		if ( ( buildableIcon = BG_Buildable( i )->icon ) )
		{
		        cg_buildables[ i ].buildableIcon = trap_R_RegisterShader( buildableIcon, RSF_DEFAULT );
                }

		cg.buildablesFraction = ( float ) i / ( float )( BA_NUM_BUILDABLES - 1 );
		trap_UpdateScreen();
	}

	cgs.media.teslaZapTS = CG_RegisterTrailSystem( "models/buildables/tesla/zap" );
}

/*
================
CG_BuildableRangeMarkerProperties
================
*/
qboolean CG_GetBuildableRangeMarkerProperties( buildable_t bType, rangeMarker_t *rmType, float *range, vec4_t rgba )
{
	shaderColorEnum_t shc;

	switch ( bType )
	{
		case BA_A_SPAWN:
			*range = CREEP_BASESIZE;
			shc = SHC_LIGHT_GREEN;
			break;

		case BA_A_OVERMIND:
			*range = CREEP_BASESIZE;
			shc = SHC_DARK_GREEN;
			break;

		case BA_A_ACIDTUBE:
			*range = ACIDTUBE_RANGE;
			shc = SHC_ORANGE;
			break;

		case BA_A_TRAPPER:
			*range = TRAPPER_RANGE;
			shc = SHC_VIOLET;
			break;

		case BA_A_HIVE:
			*range = HIVE_SENSE_RANGE;
			shc = SHC_RED;
			break;

		case BA_A_LEECH:
			*range = RGS_RANGE;
			shc = SHC_GREY;
			break;

		case BA_A_BOOSTER:
			*range = REGEN_BOOST_RANGE;
			shc = SHC_YELLOW;
			break;

		case BA_H_MGTURRET:
			*range = TURRET_RANGE;
			shc = SHC_ORANGE;
			break;

		case BA_H_TESLAGEN:
			*range = TESLAGEN_RANGE;
			shc = SHC_RED;
			break;

		case BA_H_DRILL:
			*range = RGS_RANGE;
			shc = SHC_GREY;
			break;

		case BA_H_REACTOR:
			*range = cgs.powerReactorRange;
			shc = SHC_DARK_BLUE;
			break;

		case BA_H_REPEATER:
			*range = cgs.powerRepeaterRange;
			shc = SHC_LIGHT_BLUE;
			break;

		default:
			return qfalse;
	}

	if ( bType == BA_A_TRAPPER )
	{
		// HACK: Assumes certain trapper attributes
		*rmType = RM_SPHERICAL_CONE_64;
	}
	else if ( bType == BA_H_MGTURRET )
	{
		// HACK: Assumes TURRET_PITCH_CAP == 30
		*rmType = RM_SPHERICAL_CONE_240;
	}
	else
	{
		*rmType = RM_SPHERE;
	}

	VectorCopy( cg_shaderColors[ shc ], rgba );
	rgba[3] = 1.0f;

	return qtrue;
}

/*
===============
CG_SetBuildableLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetBuildableLerpFrameAnimation( buildable_t buildable, lerpFrame_t *lf, int newAnimation )
{
	animation_t *anim;

	lf->old_animationNumber = lf->animationNumber;
	lf->old_animation = lf->animation;

	lf->animationNumber = newAnimation;

	if ( newAnimation < 0 || newAnimation >= MAX_BUILDABLE_ANIMATIONS )
	{
		CG_Error( "Bad animation number: %i", newAnimation );
	}

	if ( cg_buildables[ buildable ].md5 )
	{
		if ( bSkeleton.type != SK_INVALID )
		{
			oldbSkeleton = bSkeleton;

			if ( lf->old_animation != NULL && lf->old_animation->handle )
			{
				if ( !trap_R_BuildSkeleton( &oldbSkeleton, lf->old_animation->handle, lf->oldFrame, lf->frame, lf->blendlerp, lf->old_animation->clearOrigin ) )
				{
					CG_Printf( "Can't build old buildable bSkeleton\n" );
					return;
				}
			}
		}
	}

	anim = &cg_buildables[ buildable ].animations[ newAnimation ];

	//this item has just spawned so lf->frameTime will be zero
	if ( !lf->animation )
	{
		lf->frameTime = cg.time + 1000; //1 sec delay before starting the spawn anim
	}

	lf->animation = anim;

	if ( cg_buildables[ buildable ].md5 )
	{
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

	if ( lf->old_animationNumber <= 0 ) // Skip initial / invalid blending
	{
		lf->blendlerp = 0.0f;
		return;
	}

	if ( lf->blendlerp <= 0.0f )
	{
		lf->blendlerp = 1.0f;
	}
	else
	{
		lf->blendlerp = 1.0f - lf->blendlerp; //use old blending for smooth blending between two blended animations
	}
}

/*
===============
CG_RunBuildableLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunBuildableLerpFrame( centity_t *cent )
{
	buildable_t           buildable = (buildable_t) cent->currentState.modelindex;
	lerpFrame_t           *lf = &cent->lerpFrame;
	buildableAnimNumber_t newAnimation = (buildableAnimNumber_t) ( cent->buildableAnim & ~( ANIM_TOGGLEBIT | ANIM_FORCEBIT ) );

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation )
	{
		if ( cg_debugRandom.integer )
		{
			CG_Printf( "newAnimation: %d lf->animationNumber: %d lf->animation: %p\n",
			           newAnimation, lf->animationNumber, lf->animation );
		}

		CG_SetBuildableLerpFrameAnimation( buildable, lf, newAnimation );

		if ( !cg_buildables[ buildable ].sounds[ newAnimation ].looped &&
		     cg_buildables[ buildable ].sounds[ newAnimation ].enabled )
		{
			if ( cg_debugRandom.integer )
			{
				CG_Printf( "Sound for animation %d for a %s\n",
				           newAnimation, BG_Buildable( buildable )->humanName );
			}

			trap_S_StartSound( cent->lerpOrigin, cent->currentState.number, CHAN_AUTO,
			                   cg_buildables[ buildable ].sounds[ newAnimation ].sound );
		}
	}

	if ( cg_buildables[ buildable ].sounds[ lf->animationNumber ].looped &&
	     cg_buildables[ buildable ].sounds[ lf->animationNumber ].enabled )
	{
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
		                        cg_buildables[ buildable ].sounds[ lf->animationNumber ].sound );
	}

	CG_RunLerpFrame( lf, 1.0f );

	// animation ended
	if ( lf->frameTime == cg.time )
	{
		cent->buildableAnim = (buildableAnimNumber_t) cent->currentState.torsoAnim;
		cent->buildableIdleAnim = qtrue;
	}
}

/*
===============
CG_BuildableAnimation
===============
*/
static void CG_BuildableAnimation( centity_t *cent, int *old, int *now, float *backLerp )
{
	entityState_t *es = &cent->currentState;
	lerpFrame_t   *lf = &cent->lerpFrame;

	//if no animation is set default to idle anim
	if ( cent->buildableAnim == BANIM_NONE )
	{
		cent->buildableAnim = (buildableAnimNumber_t) es->torsoAnim;
		cent->buildableIdleAnim = qtrue;
	}

	//display the first frame of the construction anim if not yet spawned
	if ( !( es->eFlags & EF_B_SPAWNED ) )
	{
		animation_t *anim = &cg_buildables[ es->modelindex ].animations[ BANIM_CONSTRUCT1 ];

		// Change the animation in the lerpFrame so that md5s will use it too.
		cent->lerpFrame.animation = &cg_buildables[ es->modelindex ].animations[ BANIM_CONSTRUCT1 ];

		//so that when animation starts for real it has sensible numbers
		cent->lerpFrame.oldFrameTime =
		  cent->lerpFrame.frameTime =
		    cent->lerpFrame.animationTime =
		      cg.time;

		*old = cent->lerpFrame.oldFrame = anim->firstFrame;
		*now = cent->lerpFrame.frame = anim->firstFrame;
		*backLerp = cent->lerpFrame.backlerp = 0.0f;

		//ensure that an animation is triggered once the buildable has spawned
		cent->oldBuildableAnim = BANIM_NONE;
	}
	else
	{
		if ( ( cent->oldBuildableAnim ^ es->legsAnim ) & ANIM_TOGGLEBIT )
		{
			if ( cg_debugAnim.integer )
			{
				CG_Printf( "%d->%d l:%d t:%d %s(%d)\n",
				           cent->oldBuildableAnim, cent->buildableAnim,
				           es->legsAnim, es->torsoAnim,
				           BG_Buildable( es->modelindex )->humanName, es->number );
			}

			if ( cent->buildableAnim == es->torsoAnim || es->legsAnim & ANIM_FORCEBIT )
			{
				cent->buildableAnim = cent->oldBuildableAnim = (buildableAnimNumber_t) es->legsAnim;
				cent->buildableIdleAnim = qfalse;
			}
			else
			{
				cent->buildableAnim = cent->oldBuildableAnim = (buildableAnimNumber_t) es->torsoAnim;
				cent->buildableIdleAnim = qtrue;
			}
		}
		else if ( cent->buildableIdleAnim == qtrue &&
		          cent->buildableAnim != es->torsoAnim )
		{
			cent->buildableAnim = (buildableAnimNumber_t) es->torsoAnim;
		}

		CG_RunBuildableLerpFrame( cent );

		*old = cent->lerpFrame.oldFrame;
		*now = cent->lerpFrame.frame;
		*backLerp = cent->lerpFrame.backlerp;
	}

	if ( cg_buildables[ BG_Buildable( cent->currentState.modelindex )->number ].md5 )
	{
		CG_BlendLerpFrame( lf );

		CG_BuildAnimSkeleton( lf, &bSkeleton, &oldbSkeleton );
	}
}

#define TRACE_DEPTH 64.0f

/*
===============
CG_PositionAndOrientateBuildable
===============
*/
static void CG_PositionAndOrientateBuildable( const vec3_t angles, const vec3_t inOrigin,
    const vec3_t normal, const int skipNumber,
    const vec3_t mins, const vec3_t maxs,
    vec3_t outAxis[ 3 ], vec3_t outOrigin )
{
	vec3_t  forward, end;
	trace_t tr;
	float   fraction;

	AngleVectors( angles, forward, NULL, NULL );
	VectorCopy( normal, outAxis[ 2 ] );
	ProjectPointOnPlane( outAxis[ 0 ], forward, outAxis[ 2 ] );

	if ( !VectorNormalize( outAxis[ 0 ] ) )
	{
		AngleVectors( angles, NULL, NULL, forward );
		ProjectPointOnPlane( outAxis[ 0 ], forward, outAxis[ 2 ] );
		VectorNormalize( outAxis[ 0 ] );
	}

	CrossProduct( outAxis[ 0 ], outAxis[ 2 ], outAxis[ 1 ] );
	outAxis[ 1 ][ 0 ] = -outAxis[ 1 ][ 0 ];
	outAxis[ 1 ][ 1 ] = -outAxis[ 1 ][ 1 ];
	outAxis[ 1 ][ 2 ] = -outAxis[ 1 ][ 2 ];

	VectorMA( inOrigin, -TRACE_DEPTH, normal, end );

	CG_CapTrace( &tr, inOrigin, mins, maxs, end, skipNumber,
	             CONTENTS_SOLID | CONTENTS_PLAYERCLIP );

	fraction = tr.fraction;
	if ( tr.startsolid )
	{
		fraction = 0;
	}
	else if ( tr.fraction == 1.0f )
	{
		// this is either too far off of the bbox to be useful for gameplay purposes
		//  or the model is positioned in thin air anyways.
		fraction = 0;
	}

	VectorMA( inOrigin, fraction * -TRACE_DEPTH, normal, outOrigin );
}

/*
================
CG_DrawBuildableRangeMarker
================
*/
static void CG_DrawBuildableRangeMarker( buildable_t buildable, const vec3_t origin, const vec3_t normal, float opacity )
{
	rangeMarker_t rmType;
	float    range;
	vec4_t   rgba;

	if ( CG_GetBuildableRangeMarkerProperties( buildable, &rmType, &range, rgba ) )
	{
		vec3_t localOrigin;

		rgba[3] *= opacity;

		if ( buildable == BA_A_HIVE || buildable == BA_H_TESLAGEN )
		{
			VectorMA( origin, BG_BuildableModelConfig( buildable )->maxs[ 2 ], normal, localOrigin );
		}
		else
		{
			VectorCopy( origin, localOrigin );
		}

		if ( rmType == RM_SPHERE )
		{
			CG_DrawRangeMarker( rmType, localOrigin, range, NULL, rgba );
		}
		else
		{
			vec3_t angles;
			vectoangles( normal, angles );
			CG_DrawRangeMarker( rmType, localOrigin, range, angles, rgba );
		}
	}
}

/*
==================
CG_GhostBuildable
==================
*/
void CG_GhostBuildable( int buildableInfo )
{
	refEntity_t   ent;
	playerState_t *ps;
	vec3_t        angles, entity_origin;
	vec3_t        mins, maxs;
	trace_t       tr;
	float         scale;
	buildable_t   buildable = (buildable_t)( buildableInfo & SB_BUILDABLE_MASK ); // assumed not BA_NONE

	ps = &cg.predictedPlayerState;

	memset( &ent, 0, sizeof( ent ) );

	BG_BuildableBoundingBox( buildable, mins, maxs );

	BG_PositionBuildableRelativeToPlayer( ps, mins, maxs, CG_Trace, entity_origin, angles, &tr );

	if ( cg_rangeMarkerForBlueprint.integer && tr.entityNum != ENTITYNUM_NONE )
	{
		CG_DrawBuildableRangeMarker( buildable, entity_origin, tr.plane.normal, 1.0f );
	}

	CG_PositionAndOrientateBuildable( ps->viewangles, entity_origin, tr.plane.normal, ps->clientNum,
	                                  mins, maxs, ent.axis, ent.origin );

	//offset on the Z axis if required
	VectorMA( ent.origin, cg_highPolyBuildableModels.integer ? BG_BuildableModelConfig( buildable )->zOffset : BG_BuildableModelConfig( buildable )->oldOffset, tr.plane.normal, ent.origin );

	VectorCopy( ent.origin, ent.lightingOrigin );
	VectorCopy( ent.origin, ent.oldorigin );  // don't positionally lerp at all

	ent.hModel = cg_buildables[ buildable ].models[ 0 ];

	ent.customShader = ( SB_BUILDABLE_TO_IBE( buildableInfo ) == IBE_NONE )
	                     ? cgs.media.greenBuildShader
	                     : cgs.media.redBuildShader;

	// Draw predicted RGS efficiency
	// TODO: Add fancy display for predicted RGS efficiency
	if ( buildable == BA_H_DRILL || buildable == BA_A_LEECH )
	{
		char color;
		int  delta = ps->stats[ STAT_PREDICTION ];

		if ( delta < 0 )
		{
			color = COLOR_RED;
		}
		else if ( delta < 10 )
		{
			color = COLOR_ORANGE;
		}
		else if ( delta < 50 )
		{
			color = COLOR_YELLOW;
		}
		else
		{
			color = COLOR_GREEN;
		}

		CG_CenterPrint(va("^%c%+d%%", color, delta), 200, GIANTCHAR_WIDTH * 4 );
	}

	//rescale the model
	scale = BG_BuildableModelConfig( buildable )->modelScale;

	if ( cg_buildables[ buildable ].md5 )
	{
		trap_R_BuildSkeleton( &ent.skeleton, cg_buildables[ buildable ].animations[ BANIM_IDLE1 ].handle, 0, 0, 0, qfalse );
		CG_TransformSkeleton( &ent.skeleton, scale );
	}

	if ( scale != 1.0f )
	{
		VectorScale( ent.axis[ 0 ], scale, ent.axis[ 0 ] );
		VectorScale( ent.axis[ 1 ], scale, ent.axis[ 1 ] );
		VectorScale( ent.axis[ 2 ], scale, ent.axis[ 2 ] );

		ent.nonNormalizedAxes = qtrue;
	}
	else
	{
		ent.nonNormalizedAxes = qfalse;
	}

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

static void CG_GhostBuildableStatus( int buildableInfo )
{
	playerState_t *ps;
	vec3_t        angles, entity_origin;
	vec3_t        mins, maxs;
	trace_t       tr;
	float         x, y;
	buildable_t   buildable = (buildable_t)( buildableInfo & SB_BUILDABLE_MASK ); // assumed not BA_NONE

	ps = &cg.predictedPlayerState;

	BG_BuildableBoundingBox( buildable, mins, maxs );

	BG_PositionBuildableRelativeToPlayer( ps, mins, maxs, CG_Trace, entity_origin, angles, &tr );

	entity_origin[ 2 ] += mins[ 2 ];
	entity_origin[ 2 ] += ( abs( mins[ 2 ] ) + abs( maxs[ 2 ] ) ) / 2;

	if ( CG_WorldToScreen( entity_origin, &x, &y ) )
	{
		team_t team = BG_Buildable( buildable )->team;
		const buildStat_t *bs = ( team == TEAM_ALIENS )
		                      ? &cgs.alienBuildStat
		                      : &cgs.humanBuildStat;

		float  d = Distance( entity_origin, cg.refdef.vieworg );
		float  picX = x;
		float  picY = y;
		float  picH = bs->frameHeight;
		float  picM;
		float  scale = ( picH / d ) * 3.0f;

		vec4_t backColour;

		const char *text = NULL;
		qhandle_t  shader = 0;

		picM = picH * scale;
		picH = picM * ( 1.0f - bs->verticalMargin );

		backColour[0] = bs->backColor[0];
		backColour[1] = bs->backColor[1];
		backColour[2] = bs->backColor[2];
		backColour[3] = bs->backColor[3] / 3;

		switch ( SB_BUILDABLE_TO_IBE( buildableInfo ) )
		{
			case IBE_NOOVERMIND:
			case IBE_NOREACTOR:
			case IBE_NOPOWERHERE:
				shader = bs->noPowerShader;
				break;

			case IBE_NOALIENBP:
				text = "[leech]";
				break;

			case IBE_NOHUMANBP:
				text = "[drill]";
				break;

			case IBE_NOCREEP:
				text = "[egg]";
				break;

			case IBE_SURFACE:
				text = "✕";
				break;

			case IBE_DISABLED:
				text = "⨂";
				break;

			case IBE_NORMAL:
				text = "∡";
				break;

			case IBE_LASTSPAWN:
				text = ( team == TEAM_ALIENS ) ? "[egg]" : "[telenode]";
				break;

			case IBE_MAINSTRUCTURE:
				text = ( team == TEAM_ALIENS ) ? "[overmind]" : "[reactor]";
				break;

			case IBE_NOROOM:
				text = "⧉";
				break;

			default:
				break;
		}

		if ( shader )
		{
			trap_R_SetColor( backColour );
			CG_DrawPic( picX - picM / 2, picY - picM / 2, picM, picM, cgs.media.whiteShader );
			trap_R_SetColor( bs->foreColor );
			CG_DrawPic( picX - picH / 2, picY - picH / 2, picH, picH, bs->noPowerShader );
			trap_R_SetColor( NULL );
		}

		if ( text )
		{
			rectDef_t rect;
			float     tx, ty;
			vec4_t    colour;

			rect.x = picX - 128;
			rect.y = picY - picH / 2;
			rect.w = 256;
			rect.h = picH;

			trap_R_SetColor( backColour );

			CG_AlignText( &rect, text, scale, 0, 0, ALIGN_CENTER, VALIGN_CENTER, &tx, &ty );
			CG_DrawPic( tx - ( picM - picH ) / 2, ty - ( picM - picH ) / 4 - ( ty - picY ) * 2, ( picX - tx ) * 2 + ( picM - picH ), ( ty - picY ) * 2 + ( picM - picH ), cgs.media.whiteShader );

			trap_R_SetColor( NULL );

			colour[0] = bs->foreColor[0];
			colour[1] = bs->foreColor[1];
			colour[2] = bs->foreColor[2];
			colour[3] = bs->foreColor[3];

			UI_Text_Paint( tx, ty, scale, colour, text, 0, ITEM_TEXTSTYLE_PLAIN );
		}
	}
}

/*
==================
CG_BuildableParticleEffects
==================
*/
static void CG_BuildableParticleEffects( centity_t *cent )
{
	entityState_t *es = &cent->currentState;
	team_t        team = BG_Buildable( es->modelindex )->team;
	int           health = es->generic1;
	float         healthFrac = ( float ) health / BG_Buildable( es->modelindex )->health;

	if ( !( es->eFlags & EF_B_SPAWNED ) )
	{
		return;
	}

	if ( team == TEAM_HUMANS )
	{
		if ( healthFrac < 0.33f && !CG_IsParticleSystemValid( &cent->buildablePS ) )
		{
			cent->buildablePS = CG_SpawnNewParticleSystem( cgs.media.humanBuildableDamagedPS );

			if ( CG_IsParticleSystemValid( &cent->buildablePS ) )
			{
				CG_SetAttachmentCent( &cent->buildablePS->attachment, cent );
				CG_AttachToCent( &cent->buildablePS->attachment );
			}
		}
		else if ( healthFrac >= 0.33f && CG_IsParticleSystemValid( &cent->buildablePS ) )
		{
			CG_DestroyParticleSystem( &cent->buildablePS );
		}
	}
	else if ( team == TEAM_ALIENS )
	{
		if ( healthFrac < 0.33f && !CG_IsParticleSystemValid( &cent->buildablePS ) )
		{
			cent->buildablePS = CG_SpawnNewParticleSystem( cgs.media.alienBuildableDamagedPS );

			if ( CG_IsParticleSystemValid( &cent->buildablePS ) )
			{
				CG_SetAttachmentCent( &cent->buildablePS->attachment, cent );
				CG_SetParticleSystemNormal( cent->buildablePS, es->origin2 );
				CG_AttachToCent( &cent->buildablePS->attachment );
			}
		}
		else if ( healthFrac >= 0.33f && CG_IsParticleSystemValid( &cent->buildablePS ) )
		{
			CG_DestroyParticleSystem( &cent->buildablePS );
		}
	}
}

/*
==================
CG_BuildableStatusParse
==================
*/
void CG_BuildableStatusParse( const char *filename, buildStat_t *bs )
{
	pc_token_t token;
	int        handle;
	const char *s;
	int        i;
	float      f;
	vec4_t     c;

	handle = trap_Parse_LoadSource( filename );

	if ( !handle )
	{
		return;
	}

	while ( 1 )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			break;
		}

		if ( !Q_stricmp( token.string, "frameShader" ) )
		{
			if ( PC_String_Parse( handle, &s ) )
			{
				bs->frameShader = trap_R_RegisterShader(s,
									RSF_DEFAULT);
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "overlayShader" ) )
		{
			if ( PC_String_Parse( handle, &s ) )
			{
				bs->overlayShader = trap_R_RegisterShader(s,
									  RSF_DEFAULT);
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "noPowerShader" ) )
		{
			if ( PC_String_Parse( handle, &s ) )
			{
				bs->noPowerShader = trap_R_RegisterShader(s,
									  RSF_DEFAULT);
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "markedShader" ) )
		{
			if ( PC_String_Parse( handle, &s ) )
			{
				bs->markedShader = trap_R_RegisterShader(s,
									 RSF_DEFAULT);
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "healthSevereColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				Vector4Copy( c, bs->healthSevereColor );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "healthHighColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				Vector4Copy( c, bs->healthHighColor );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "healthElevatedColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				Vector4Copy( c, bs->healthElevatedColor );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "healthGuardedColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				Vector4Copy( c, bs->healthGuardedColor );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "healthLowColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				Vector4Copy( c, bs->healthLowColor );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "foreColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				Vector4Copy( c, bs->foreColor );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "backColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				Vector4Copy( c, bs->backColor );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "frameHeight" ) )
		{
			if ( PC_Int_Parse( handle, &i ) )
			{
				bs->frameHeight = i;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "frameWidth" ) )
		{
			if ( PC_Int_Parse( handle, &i ) )
			{
				bs->frameWidth = i;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "healthPadding" ) )
		{
			if ( PC_Int_Parse( handle, &i ) )
			{
				bs->healthPadding = i;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "overlayHeight" ) )
		{
			if ( PC_Int_Parse( handle, &i ) )
			{
				bs->overlayHeight = i;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "overlayWidth" ) )
		{
			if ( PC_Int_Parse( handle, &i ) )
			{
				bs->overlayWidth = i;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "verticalMargin" ) )
		{
			if ( PC_Float_Parse( handle, &f ) )
			{
				bs->verticalMargin = f;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "horizontalMargin" ) )
		{
			if ( PC_Float_Parse( handle, &f ) )
			{
				bs->horizontalMargin = f;
			}

			continue;
		}
		else
		{
			Com_Printf( "CG_BuildableStatusParse: unknown token %s in %s\n",
			            token.string, filename );
			bs->loaded = qfalse;
			trap_Parse_FreeSource( handle );
			return;
		}
	}

	bs->loaded = qtrue;
	trap_Parse_FreeSource( handle );
}

#define STATUS_FADE_TIME     200
#define STATUS_MAX_VIEW_DIST 900.0f
#define STATUS_PEEK_DIST     20

static void HealthColorFade( vec4_t out, float healthFrac, buildStat_t *bs )
{
	float frac;

	if ( healthFrac > 1.0f )
	{
		healthFrac = 1.0f;
	}
	else if ( healthFrac < 0.0f )
	{
		healthFrac = 0.0f;
	}

	if ( healthFrac == 1.0f )
	{
		Vector4Copy( bs->healthLowColor, out );
	}
	else if ( healthFrac > 0.666f )
	{
		frac = 1.0f - ( healthFrac - 0.666f ) * 3.0f;
		Vector4Lerp( frac, bs->healthGuardedColor, bs->healthElevatedColor, out );
	}
	else if ( healthFrac > 0.333f )
	{
		frac = 1.0f - ( healthFrac - 0.333f ) * 3.0f;
		Vector4Lerp( frac, bs->healthElevatedColor, bs->healthHighColor, out );
	}
	else
	{
		frac = 1.0f - healthFrac * 3.0f;
		Vector4Lerp( frac, bs->healthHighColor, bs->healthSevereColor, out );
	}
}

static void DepletionColorFade( vec4_t out, float frac, buildStat_t *bs )
{
	if ( frac > 1.0f )
	{
		frac = 1.0f;
	}
	else if ( frac < 0.0f )
	{
		frac = 0.0f;
	}

	frac = frac * 0.6f + 0.4f;

	Vector4Copy( bs->healthLowColor, out );
	Vector4Scale( out, frac, out );
}

/*
==================
CG_BuildableStatusDisplay
==================
*/
static void CG_BuildableStatusDisplay( centity_t *cent )
{
	entityState_t *es = &cent->currentState;
	vec3_t        origin;
	float         healthFrac, powerUsageFrac = 0, totalPowerFrac = 0, mineEfficiencyFrac = 0;
	int           health, powerUsage = 0, totalPower = 0, mineEfficiency = 0;
	float         x, y;
	vec4_t        color;
	qboolean      powered, marked, showMineEfficiency, showPower;
	trace_t       tr;
	float         d;
	buildStat_t   *bs;
	int           i, j;
	int           entNum;
	vec3_t        trOrigin;
	vec3_t        right;
	qboolean      visible = qfalse;
	vec3_t        mins, maxs;
	vec3_t        cullMins, cullMaxs;
	entityState_t *hit;
	int           anim;
	const buildableAttributes_t *attr = BG_Buildable( es->modelindex );

	if ( attr->team == TEAM_ALIENS )
	{
		bs = &cgs.alienBuildStat;
	}
	else
	{
		bs = &cgs.humanBuildStat;
	}

	if ( !bs->loaded )
	{
		return;
	}

	d = Distance( cent->lerpOrigin, cg.refdef.vieworg );

	if ( d > STATUS_MAX_VIEW_DIST )
	{
		return;
	}

	Vector4Copy( bs->foreColor, color );

	// trace for center point
	BG_BuildableBoundingBox( es->modelindex, mins, maxs );

	// cull buildings outside the view frustum
	VectorAdd(cent->lerpOrigin, mins, cullMins);
	VectorAdd(cent->lerpOrigin, maxs, cullMaxs);

	if(CG_CullBox(cullMins, cullMaxs))
		return;

	// hack for shrunken barricades
	anim = es->torsoAnim & ~( ANIM_FORCEBIT | ANIM_TOGGLEBIT );

	if ( es->modelindex == BA_A_BARRICADE &&
	     ( anim == BANIM_DESTROYED || !( es->eFlags & EF_B_SPAWNED ) ) )
	{
		maxs[ 2 ] = ( int )( maxs[ 2 ] * BARRICADE_SHRINKPROP );
	}

	VectorCopy( cent->lerpOrigin, origin );

	// center point
	origin[ 2 ] += mins[ 2 ];
	origin[ 2 ] += ( abs( mins[ 2 ] ) + abs( maxs[ 2 ] ) ) / 2;

	entNum = cg.predictedPlayerState.clientNum;

	// if first try fails, step left, step right
	for ( j = 0; j < 3 && !visible; j++ )
	{
		VectorCopy( cg.refdef.vieworg, trOrigin );

		switch ( j )
		{
			case 1:
				// step right
				AngleVectors( cg.refdefViewAngles, NULL, right, NULL );
				VectorMA( trOrigin, STATUS_PEEK_DIST, right, trOrigin );
				break;

			case 2:
				// step left
				AngleVectors( cg.refdefViewAngles, NULL, right, NULL );
				VectorMA( trOrigin, -STATUS_PEEK_DIST, right, trOrigin );
				break;

			default:
				break;
		}

		// look through up to 3 players and/or transparent buildables
		for ( i = 0; i < 3; i++ )
		{
			CG_Trace( &tr, trOrigin, NULL, NULL, origin, entNum, MASK_SHOT );

			if ( tr.entityNum == cent->currentState.number )
			{
				visible = qtrue;
				break;
			}

			if ( tr.entityNum == ENTITYNUM_WORLD )
			{
				break;
			}

			hit = &cg_entities[ tr.entityNum ].currentState;

			if ( tr.entityNum < MAX_CLIENTS || ( hit->eType == ET_BUILDABLE &&
			                                     ( !( es->eFlags & EF_B_SPAWNED ) ||
			                                       BG_Buildable( hit->modelindex )->transparentTest ) ) )
			{
				entNum = tr.entityNum;
				VectorCopy( tr.endpos, trOrigin );
			}
			else
			{
				break;
			}
		}
	}

	// check if visibility state changed
	if ( !visible && cent->buildableStatus.visible )
	{
		cent->buildableStatus.visible = qfalse;
		cent->buildableStatus.lastTime = cg.time;
	}
	else if ( visible && !cent->buildableStatus.visible )
	{
		cent->buildableStatus.visible = qtrue;
		cent->buildableStatus.lastTime = cg.time;
	}

	// Fade up
	if ( cent->buildableStatus.visible )
	{
		if ( cent->buildableStatus.lastTime + STATUS_FADE_TIME > cg.time )
		{
			color[ 3 ] = ( float )( cg.time - cent->buildableStatus.lastTime ) / STATUS_FADE_TIME;
		}
	}

	// Fade down
	if ( !cent->buildableStatus.visible )
	{
		if ( cent->buildableStatus.lastTime + STATUS_FADE_TIME > cg.time )
		{
			color[ 3 ] = 1.0f - ( float )( cg.time - cent->buildableStatus.lastTime ) / STATUS_FADE_TIME;
		}
		else
		{
			return;
		}
	}

	// find out what data we want to display
	showMineEfficiency = ( attr->number == BA_A_LEECH || attr->number == BA_H_DRILL );
	showPower          = ( attr->team == TEAM_HUMANS && attr->powerConsumption > 0 );

	// calculate mine efficiency bar size
	if ( showMineEfficiency )
	{
		mineEfficiency      = es->weaponAnim;
		mineEfficiencyFrac = (float)mineEfficiency / 100.0f;

		if ( mineEfficiency > 0 && mineEfficiencyFrac < 0.01f )
		{
			mineEfficiencyFrac = 0.01f;
		}
		else if ( mineEfficiencyFrac < 0.0f )
		{
			mineEfficiencyFrac = 0.0f;
		}
		else if ( mineEfficiencyFrac > 1.0f )
		{
			mineEfficiencyFrac = 1.0f;
		}
	}

	// calculate health bar size
	{
		health     = es->generic1;
		healthFrac = (float)health / (float)attr->health;

		if ( health > 0 && healthFrac < 0.01f )
		{
			healthFrac = 0.01f;
		}
		else if ( healthFrac < 0.0f )
		{
			healthFrac = 0.0f;
		}
		else if ( healthFrac > 1.0f )
		{
			healthFrac = 1.0f;
		}
	}

	// calculate power consumption bar size
	if ( showPower )
	{
		totalPower     = Q_clamp( es->clientNum, 0, POWER_DISPLAY_MAX );
		powerUsage     = BG_Buildable( es->modelindex )->powerConsumption;

		totalPowerFrac = (float)totalPower / (float)POWER_DISPLAY_MAX;
		powerUsageFrac = (float)powerUsage / (float)POWER_DISPLAY_MAX;
	}

	// draw elements
	if ( CG_WorldToScreen( origin, &x, &y ) )
	{
		float  picH = bs->frameHeight;
		float  picW = bs->frameWidth;
		float  picX = x;
		float  picY = y;
		float  scale;
		float  subH, subY;
		float  clipX, clipY, clipW, clipH;
		vec4_t frameColor;

		// this is fudged to get the width/height in the cfg to be more realistic
		scale = ( picH / d ) * 3;

		powered = es->eFlags & EF_B_POWERED;
		marked = es->eFlags & EF_B_MARKED;

		picH *= scale;
		picW *= scale;
		picX -= ( picW * 0.5f );
		picY -= ( picH * 0.5f );

		// sub-elements such as icons and number
		subH = picH - ( picH * bs->verticalMargin );
		subY = picY + ( picH * 0.5f ) - ( subH * 0.5f );

		clipW = ( 640.0f * cg_viewsize.integer ) / 100.0f;
		clipH = ( 480.0f * cg_viewsize.integer ) / 100.0f;
		clipX = 320.0f - ( clipW * 0.5f );
		clipY = 240.0f - ( clipH * 0.5f );
		CG_SetClipRegion( clipX, clipY, clipW, clipH );

		// draw bar frames
		if ( bs->frameShader )
		{
			Vector4Copy( bs->backColor, frameColor );
			frameColor[ 3 ] = color[ 3 ];
			trap_R_SetColor( frameColor );

			if ( showMineEfficiency )
			{
				CG_SetClipRegion( picX, picY - 0.5f * picH, picW, 0.5f * picH );
				CG_DrawPic( picX, picY - 0.5f * picH, picW, picH, bs->frameShader );
				CG_ClearClipRegion();
			}

			CG_DrawPic( picX, picY, picW, picH, bs->frameShader );

			if ( showPower )
			{
				CG_SetClipRegion( picX, picY + picH, picW, 0.5f * picH );
				CG_DrawPic( picX, picY + 0.5f * picH, picW, picH, bs->frameShader );
				CG_ClearClipRegion();
			}

			trap_R_SetColor( NULL );
		}

		// draw mine rate bar
		if ( showMineEfficiency && mineEfficiency > 0 )
		{
			float  barX, barY, barW, barH;
			vec4_t barColor;

			barX = picX + ( bs->healthPadding * scale );
			barY = picY - ( 0.5f * picH ) + ( bs->healthPadding * scale );
			barH = ( 0.5f * picH ) - ( bs->healthPadding * scale );
			barW = ( picW * mineEfficiencyFrac ) - ( bs->healthPadding * 2.0f * scale );

			DepletionColorFade( barColor, mineEfficiencyFrac, bs );
			barColor[ 3 ] = color[ 3 ];

			trap_R_SetColor( barColor );

			CG_DrawPic( barX, barY, barW, barH, cgs.media.whiteShader );

			trap_R_SetColor( NULL );
		}

		// draw health bar
		if ( health > 0 )
		{
			float  barX, barY, barW, barH;
			vec4_t barColor;

			barX = picX + ( bs->healthPadding * scale );
			barY = picY + ( bs->healthPadding * scale );
			barH = picH - ( bs->healthPadding * 2.0f * scale );
			barW = picW * healthFrac - ( bs->healthPadding * 2.0f * scale );

			HealthColorFade( barColor, healthFrac, bs );
			barColor[ 3 ] = color[ 3 ];

			trap_R_SetColor( barColor );
			CG_DrawPic( barX, barY, barW, barH, cgs.media.whiteShader );

			trap_R_SetColor( NULL );
		}

		// draw power consumption bar
		if ( showPower )
		{
			float  barX, barY, barW, barH, markX, markH, markW;
			vec4_t barColor, markColor;

			// draw bar
			barX = picX + ( bs->healthPadding * scale );
			barY = picY + picH;
			barH = ( 0.5f * picH ) - ( bs->healthPadding * scale );
			barW = ( picW * totalPowerFrac ) - ( bs->healthPadding * 2.0f * scale );

			if ( barW > 0.0f )
			{
				if ( powered ) { DepletionColorFade( barColor, totalPowerFrac, bs ); }
				else           { Vector4Copy( bs->healthSevereColor, barColor ); }
				barColor[ 3 ] = color[ 3 ];

				trap_R_SetColor( barColor );
				CG_DrawPic( barX, barY, barW, barH, cgs.media.whiteShader );
			}

			// draw mark
			markX = barX + ( picW * powerUsageFrac ) - ( bs->healthPadding * 2.5f * scale );
			markH = 0.5f * barH;
			markW = ( bs->healthPadding * scale );

			Vector4Copy( colorBlack, markColor );

			trap_R_SetColor( markColor );
			CG_DrawPic( markX, barY, markW, markH, cgs.media.whiteShader );

			trap_R_SetColor( NULL );
		}

		if ( bs->overlayShader )
		{
			float oW = bs->overlayWidth;
			float oH = bs->overlayHeight;
			float oX = x;
			float oY = y;

			oH *= scale;
			oW *= scale;
			oX -= ( oW * 0.5f );
			oY -= ( oH * 0.5f );

			trap_R_SetColor( frameColor );
			CG_DrawPic( oX, oY, oW, oH, bs->overlayShader );

			trap_R_SetColor( NULL );
		}

		trap_R_SetColor( color );

		// show no power icon
		if ( !powered )
		{
			float pX;

			pX = picX + ( subH * bs->horizontalMargin );
			CG_DrawPic( pX, subY, subH, subH, bs->noPowerShader );
		}

		// show marked icon
		if ( marked )
		{
			float mX;

			mX = picX + picW - ( subH * bs->horizontalMargin ) - subH;
			CG_DrawPic( mX, subY, subH, subH, bs->markedShader );
		}

		// show hp
		{
			float nX;
			int   healthMax;
			int   healthPoints;

			healthMax = BG_Buildable( es->modelindex )->health;
			healthPoints = ( int )( healthFrac * healthMax );

			if ( health > 0 && healthPoints < 1 )
			{
				healthPoints = 1;
			}

			nX = picX + ( picW * 0.5f ) - 2.0f - ( ( subH * 4 ) * 0.5f );

			if ( healthPoints > 999 )
			{
				nX -= 0.0f;
			}
			else if ( healthPoints > 99 )
			{
				nX -= subH * 0.5f;
			}
			else if ( healthPoints > 9 )
			{
				nX -= subH * 1.0f;
			}
			else
			{
				nX -= subH * 1.5f;
			}

			CG_DrawField( nX, subY, 4, subH, subH, healthPoints );
		}

		trap_R_SetColor( NULL );
		CG_ClearClipRegion();
	}
}

/*
==================
CG_SortDistance
==================
*/
static int CG_SortDistance( const void *a, const void *b )
{
	centity_t *aent, *bent;
	float     adist, bdist;

	aent = &cg_entities[ * ( int * ) a ];
	bent = &cg_entities[ * ( int * ) b ];
	adist = Distance( cg.refdef.vieworg, aent->lerpOrigin );
	bdist = Distance( cg.refdef.vieworg, bent->lerpOrigin );

	if ( adist > bdist )
	{
		return -1;
	}
	else if ( adist < bdist )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/*
==================
CG_PlayerIsBuilder
==================
*/
static qboolean CG_PlayerIsBuilder( buildable_t buildable )
{
	switch ( cg.predictedPlayerState.weapon )
	{
		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			return BG_Buildable( buildable )->team ==
			       BG_Weapon( cg.predictedPlayerState.weapon )->team;

		default:
			return qfalse;
	}
}

/*
==================
CG_BuildableRemovalPending
==================
*/
static qboolean CG_BuildableRemovalPending( int entityNum )
{
	int           i;
	playerState_t *ps = &cg.snap->ps;

	if ( SB_BUILDABLE_TO_IBE( ps->stats[ STAT_BUILDABLE ] ) != IBE_NONE )
	{
		return qfalse;
	}

	for ( i = 0; i < MAX_MISC; i++ )
	{
		if ( ps->misc[ i ] == entityNum )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
==================
CG_DrawBuildableStatus
==================
*/
void CG_DrawBuildableStatus( void )
{
	int           i;
	centity_t     *cent;
	entityState_t *es;
	int           buildableList[ MAX_ENTITIES_IN_SNAPSHOT ];
	int           buildables = 0;

	if ( !cg_drawBuildableHealth.integer )
	{
		return;
	}

	for ( i = 0; i < cg.snap->numEntities; i++ )
	{
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		es = &cent->currentState;

		if ( es->eType == ET_BUILDABLE && CG_PlayerIsBuilder( (buildable_t) es->modelindex ) )
		{
			buildableList[ buildables++ ] = cg.snap->entities[ i ].number;
		}
	}

	qsort( buildableList, buildables, sizeof( int ), CG_SortDistance );

	for ( i = 0; i < buildables; i++ )
	{
		CG_BuildableStatusDisplay( &cg_entities[ buildableList[ i ] ] );
	}

	if ( cg.predictedPlayerState.stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK )
	{
		CG_GhostBuildableStatus( cg.predictedPlayerState.stats[ STAT_BUILDABLE ] );
        }
}

#define BUILDABLE_SOUND_PERIOD 500

/*
==================
CG_Buildable
==================
*/
void CG_Buildable( centity_t *cent )
{
	refEntity_t   ent;
	entityState_t *es = &cent->currentState;
	vec3_t        angles;
	vec3_t        surfNormal, xNormal, mins, maxs;
	vec3_t        refNormal = { 0.0f, 0.0f, 1.0f };
	float         rotAngle;
	const buildableAttributes_t *buildable = BG_Buildable( es->modelindex );
	team_t        team = buildable->team;
	float         scale;
	int           health;

	//must be before EF_NODRAW check
	if ( team == TEAM_ALIENS )
	{
		CG_Creep( cent );
	}

	// if set to invisible, skip
	if ( es->eFlags & EF_NODRAW )
	{
		if ( CG_IsParticleSystemValid( &cent->buildablePS ) )
		{
			CG_DestroyParticleSystem( &cent->buildablePS );
		}

		if ( CG_IsParticleSystemValid( &cent->buildableStatusPS ) )
		{
			CG_DestroyParticleSystem( &cent->buildableStatusPS );
		}

		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( es->origin2, surfNormal );

	VectorCopy( es->angles, angles );
	BG_BuildableBoundingBox( es->modelindex, mins, maxs );

	if ( es->pos.trType == TR_STATIONARY )
	{
		// seeing as buildables rarely move, we cache the results and recalculate
		// only if the buildable moves or changes orientation
		if ( VectorCompare( cent->buildableCache.cachedOrigin, cent->lerpOrigin ) &&
		     VectorCompare( cent->buildableCache.cachedAngles, cent->lerpAngles ) &&
		     VectorCompare( cent->buildableCache.cachedNormal, surfNormal ) &&
		     cent->buildableCache.cachedType == es->modelindex )
		{
			VectorCopy( cent->buildableCache.axis[ 0 ], ent.axis[ 0 ] );
			VectorCopy( cent->buildableCache.axis[ 1 ], ent.axis[ 1 ] );
			VectorCopy( cent->buildableCache.axis[ 2 ], ent.axis[ 2 ] );
			VectorCopy( cent->buildableCache.origin, ent.origin );
		}
		else
		{
			CG_PositionAndOrientateBuildable( angles, cent->lerpOrigin, surfNormal,
			                                  es->number, mins, maxs, ent.axis,
			                                  ent.origin );
			VectorCopy( ent.axis[ 0 ], cent->buildableCache.axis[ 0 ] );
			VectorCopy( ent.axis[ 1 ], cent->buildableCache.axis[ 1 ] );
			VectorCopy( ent.axis[ 2 ], cent->buildableCache.axis[ 2 ] );
			VectorCopy( ent.origin, cent->buildableCache.origin );
			VectorCopy( cent->lerpOrigin, cent->buildableCache.cachedOrigin );
			VectorCopy( cent->lerpAngles, cent->buildableCache.cachedAngles );
			VectorCopy( surfNormal, cent->buildableCache.cachedNormal );
			cent->buildableCache.cachedType = (buildable_t) es->modelindex;
		}
	}
	else
	{
		VectorCopy( cent->lerpOrigin, ent.origin );
		AnglesToAxis( cent->lerpAngles, ent.axis );
	}

	if( cg_drawBBOX.integer )
	{
		CG_DrawBoundingBox( cg_drawBBOX.integer, cent->lerpOrigin, mins, maxs );
	}

	//offset on the Z axis if required
	VectorMA( ent.origin, cg_highPolyBuildableModels.integer ? BG_BuildableModelConfig( es->modelindex )->zOffset : BG_BuildableModelConfig( es->modelindex )->oldOffset, surfNormal, ent.origin );

	VectorCopy( ent.origin, ent.oldorigin );  // don't positionally lerp at all
	VectorCopy( ent.origin, ent.lightingOrigin );

	ent.hModel = cg_buildables[ es->modelindex ].models[ 0 ];

	if ( !( es->eFlags & EF_B_SPAWNED ) )
	{
		sfxHandle_t prebuildSound = cgs.media.humanBuildablePrebuild;

		if ( team == TEAM_HUMANS )
		{
			ent.customShader = cgs.media.humanSpawningShader;
			prebuildSound = cgs.media.humanBuildablePrebuild;
		}
		else if ( team == TEAM_ALIENS )
		{
			prebuildSound = cgs.media.alienBuildablePrebuild;
		}

		trap_S_AddLoopingSound( es->number, cent->lerpOrigin, vec3_origin, prebuildSound );
	}

	CG_BuildableAnimation( cent, &ent.oldframe, &ent.frame, &ent.backlerp );

	//rescale the model
	scale = cg_highPolyBuildableModels.integer ? BG_BuildableModelConfig( es->modelindex )->modelScale : BG_BuildableModelConfig( es->modelindex )->oldScale;

	if ( scale != 1.0f )
	{
		VectorScale( ent.axis[ 0 ], scale, ent.axis[ 0 ] );
		VectorScale( ent.axis[ 1 ], scale, ent.axis[ 1 ] );
		VectorScale( ent.axis[ 2 ], scale, ent.axis[ 2 ] );

		ent.nonNormalizedAxes = qtrue;
	}
	else
	{
		ent.nonNormalizedAxes = qfalse;
	}

	// add inverse shadow map
	if ( cg_shadows.integer > SHADOWING_BLOB && cg_buildableShadows.integer )
	{
		CG_StartShadowCaster( ent.lightingOrigin, mins, maxs );
	}

	if ( CG_PlayerIsBuilder( (buildable_t) es->modelindex ) && CG_BuildableRemovalPending( es->number ) )
	{
		ent.customShader = cgs.media.redBuildShader;
	}

	if ( cg_buildables[ es->modelindex ].md5 )
	{
		qboolean  spawned = ( es->eFlags & EF_B_SPAWNED ) || ( team == TEAM_HUMANS ); // If buildable has spawned or is a human buildable, don't alter the size

		float realScale = spawned ? scale :
			scale * (float) sin ( 0.5f * (cg.time - es->time) / buildable->buildTime * M_PI );
		ent.skeleton = bSkeleton;

		if( es->modelindex == BA_H_MGTURRET )
		{
			quat_t   rotation;
			matrix_t mat;
			vec3_t   nBounds[ 2 ];
			vec3_t   p1, p2;

			//FIXME: Don't hard code bones to specific assets. Soon, I should put bone names in
			// .cfg so we can change it should the rig change.

			QuatFromAngles( rotation, es->angles2[ YAW ] - es->angles[ YAW ], 0, 0 );
			QuatMultiply0( ent.skeleton.bones[ 1 ].t.rot, rotation );

			QuatFromAngles( rotation, es->angles2[ PITCH ], 0, 0 );
			QuatMultiply0( ent.skeleton.bones[ 6 ].t.rot, rotation );

			// transform bounds so they more accurately reflect the turret's new trasnformation
			MatrixFromAngles( mat, es->angles2[ PITCH ], es->angles2[ YAW ] - es->angles[ YAW ], 0 );

			MatrixTransformNormal( mat, ent.skeleton.bounds[ 0 ], p1 );
			MatrixTransformNormal( mat, ent.skeleton.bounds[ 1 ], p2 );

			ClearBounds( nBounds[ 0 ], nBounds[ 1 ] );
			AddPointToBounds( p1, nBounds[ 0 ], nBounds[ 1 ] );
			AddPointToBounds( p2, nBounds[ 0 ], nBounds[ 1 ] );

			BoundsAdd( ent.skeleton.bounds[ 0 ], ent.skeleton.bounds[ 1 ], nBounds[ 0 ], nBounds[ 1 ] );
		}

		CG_TransformSkeleton( &ent.skeleton, realScale );
	}

	if ( es->generic1 <= 0 )
	{
		ent.altShaderIndex = CG_ALTSHADER_DEAD;
	}
	else if ( !(es->eFlags & EF_B_POWERED) )
	{
		ent.altShaderIndex = CG_ALTSHADER_UNPOWERED;
	}

	//add to refresh list
	trap_R_AddRefEntityToScene( &ent );

	CrossProduct( surfNormal, refNormal, xNormal );
	VectorNormalize( xNormal );
	rotAngle = RAD2DEG( acos( DotProduct( surfNormal, refNormal ) ) );

	//turret barrel bit
	if ( cg_buildables[ es->modelindex ].models[ 1 ] )
	{
		refEntity_t turretBarrel;
		vec3_t      flatAxis[ 3 ];

		memset( &turretBarrel, 0, sizeof( turretBarrel ) );

		turretBarrel.hModel = cg_buildables[ es->modelindex ].models[ 1 ];

		CG_PositionEntityOnTag( &turretBarrel, &ent, ent.hModel, "tag_turret" );
		VectorCopy( cent->lerpOrigin, turretBarrel.lightingOrigin );
		AnglesToAxis( es->angles2, flatAxis );

		RotatePointAroundVector( turretBarrel.axis[ 0 ], xNormal, flatAxis[ 0 ], -rotAngle );
		RotatePointAroundVector( turretBarrel.axis[ 1 ], xNormal, flatAxis[ 1 ], -rotAngle );
		RotatePointAroundVector( turretBarrel.axis[ 2 ], xNormal, flatAxis[ 2 ], -rotAngle );

		turretBarrel.oldframe = ent.oldframe;
		turretBarrel.frame = ent.frame;
		turretBarrel.backlerp = ent.backlerp;

		turretBarrel.customShader = ent.customShader;

		if ( scale != 1.0f )
		{
			VectorScale( turretBarrel.axis[ 0 ], scale, turretBarrel.axis[ 0 ] );
			VectorScale( turretBarrel.axis[ 1 ], scale, turretBarrel.axis[ 1 ] );
			VectorScale( turretBarrel.axis[ 2 ], scale, turretBarrel.axis[ 2 ] );

			turretBarrel.nonNormalizedAxes = qtrue;
		}
		else
		{
			turretBarrel.nonNormalizedAxes = qfalse;
		}

		if ( CG_PlayerIsBuilder( (buildable_t) es->modelindex ) && CG_BuildableRemovalPending( es->number ) )
		{
			turretBarrel.customShader = cgs.media.redBuildShader;
		}

		turretBarrel.altShaderIndex = ent.altShaderIndex;
		trap_R_AddRefEntityToScene( &turretBarrel );
	}

	//turret barrel bit
	if ( cg_buildables[ es->modelindex ].models[ 2 ] )
	{
		refEntity_t turretTop;
		vec3_t      flatAxis[ 3 ];
		vec3_t      swivelAngles;

		memset( &turretTop, 0, sizeof( turretTop ) );

		VectorCopy( es->angles2, swivelAngles );
		swivelAngles[ PITCH ] = 0.0f;

		turretTop.hModel = cg_buildables[ es->modelindex ].models[ 2 ];

		CG_PositionRotatedEntityOnTag( &turretTop, &ent, ent.hModel, "tag_turret" );
		VectorCopy( cent->lerpOrigin, turretTop.lightingOrigin );
		AnglesToAxis( swivelAngles, flatAxis );

		RotatePointAroundVector( turretTop.axis[ 0 ], xNormal, flatAxis[ 0 ], -rotAngle );
		RotatePointAroundVector( turretTop.axis[ 1 ], xNormal, flatAxis[ 1 ], -rotAngle );
		RotatePointAroundVector( turretTop.axis[ 2 ], xNormal, flatAxis[ 2 ], -rotAngle );

		turretTop.oldframe = ent.oldframe;
		turretTop.frame = ent.frame;
		turretTop.backlerp = ent.backlerp;

		turretTop.customShader = ent.customShader;

		if ( scale != 1.0f )
		{
			VectorScale( turretTop.axis[ 0 ], scale, turretTop.axis[ 0 ] );
			VectorScale( turretTop.axis[ 1 ], scale, turretTop.axis[ 1 ] );
			VectorScale( turretTop.axis[ 2 ], scale, turretTop.axis[ 2 ] );

			turretTop.nonNormalizedAxes = qtrue;
		}
		else
		{
			turretTop.nonNormalizedAxes = qfalse;
		}

		if ( CG_PlayerIsBuilder( (buildable_t) es->modelindex ) && CG_BuildableRemovalPending( es->number ) )
		{
			turretTop.customShader = cgs.media.redBuildShader;
		}

		turretTop.altShaderIndex = ent.altShaderIndex;
		trap_R_AddRefEntityToScene( &turretTop );
	}

	//weapon effects for turrets
	if ( es->eFlags & EF_FIRING )
	{
		weaponInfo_t *wi = &cg_weapons[ es->weapon ];

		if ( wi->wim[ WPM_PRIMARY ].muzzleParticleSystem )
		{
			// spawn muzzle ps if necessary
			if ( !CG_IsParticleSystemValid( &cent->muzzlePS ) )
			{
				cent->muzzlePS = CG_SpawnNewParticleSystem( wi->wim[ WPM_PRIMARY ].muzzleParticleSystem );
			}

			// update muzzle ps position
			if ( CG_IsParticleSystemValid( &cent->muzzlePS ) )
			{
				CG_SetAttachmentTag( &cent->muzzlePS->attachment, &ent, ent.hModel, "tag_flash" );
				CG_AttachToTag( &cent->muzzlePS->attachment );
			}
		}

		if ( cg.time - cent->muzzleFlashTime < MUZZLE_FLASH_TIME || ( weapon_t )es->weapon == WP_TESLAGEN )
		{
			// add dynamic light
			if ( wi->wim[ WPM_PRIMARY ].flashDlight )
			{
				trap_R_AddLightToScene( cent->lerpOrigin, wi->wim[ WPM_PRIMARY ].flashDlight,
				                        wi->wim[ WPM_PRIMARY ].flashDlightIntensity,
				                        wi->wim[ WPM_PRIMARY ].flashDlightColor[ 0 ],
				                        wi->wim[ WPM_PRIMARY ].flashDlightColor[ 1 ],
				                        wi->wim[ WPM_PRIMARY ].flashDlightColor[ 2 ], 0, 0 );
			}
		}

		// spawn firing sound
		if ( wi->wim[ WPM_PRIMARY ].firingSound )
		{
			trap_S_AddLoopingSound( es->number, cent->lerpOrigin, vec3_origin, wi->wim[ WPM_PRIMARY ].firingSound );
		}
		else if ( wi->readySound )
		{
			trap_S_AddLoopingSound( es->number, cent->lerpOrigin, vec3_origin, wi->readySound );
		}
	}
	else if ( CG_IsParticleSystemValid( &cent->muzzlePS ) )
	{
		// destroy active muzzle ps
		CG_DestroyParticleSystem( &cent->muzzlePS );
	}

	health = es->generic1;

	if ( health < cent->lastBuildableHealth && ( es->eFlags & EF_B_SPAWNED ) )
	{
		if ( cent->lastBuildableDamageSoundTime + BUILDABLE_SOUND_PERIOD < cg.time )
		{
			if ( team == TEAM_HUMANS )
			{
				int i = rand() % 4;
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.humanBuildableDamage[ i ] );
			}

			cent->lastBuildableDamageSoundTime = cg.time;
		}
	}

	cent->lastBuildableHealth = health;

	// set particle effect to fire if buildable is burning
	CG_OnFire( cent );

	//smoke etc for damaged buildables
	CG_BuildableParticleEffects( cent );

	// draw range marker if enabled
	if( team == cg.predictedPlayerState.persistant[ PERS_TEAM ] ) {
		qboolean drawRange;
		float dist, maxDist = MAX( RADAR_RANGE, ALIENSENSE_RANGE );

		if ( team == TEAM_HUMANS ) {
			drawRange = BG_InventoryContainsWeapon( WP_HBUILD, cg.predictedPlayerState.stats );
		} else if ( team == TEAM_ALIENS ) {
			drawRange = cg.predictedPlayerState.weapon == WP_ABUILD ||
			            cg.predictedPlayerState.weapon == WP_ABUILD2;
		} else {
			drawRange = !!(cg_buildableRangeMarkerMask.integer & (1 << BA_NONE));
		}

		drawRange &= !!(cg_buildableRangeMarkerMask.integer & (1 << buildable->number));

		dist = Distance( cent->lerpOrigin, cg.refdef.vieworg );

		if( drawRange && dist <= maxDist ) {
			float opacity = 1.0f;

			if( dist >= 0.9f * maxDist ) {
				opacity = 10.0f - 10.0f * dist / maxDist;
			}

			CG_DrawBuildableRangeMarker( buildable->number, cent->lerpOrigin, cent->currentState.origin2, opacity );
		}
	}

	if ( cg_shadows.integer > SHADOWING_BLOB && cg_buildableShadows.integer )
	{
		CG_EndShadowCaster( );
	}
}
