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

#include "common/Common.h"
#include "common/FileSystem.h"
#include "shared/parse.h"
#include "cg_local.h"

static const char *const cg_buildableSoundNames[ MAX_BUILDABLE_ANIMATIONS ] =
{
	"idle1",
	"idle2",
	"powerdown",
	"idle_unpowered",
	"construct1",
	"construct2",
	"attack1",
	"attack2",
	"spawn1",
	"spawn2",
	"pain1",
	"pain2",
	"destroy1",
	"destroy2",
	"destroyed",
	"destroyed2"
};

// Shorthand definitions for the buildable animation names below.
enum shorthand_t {
	XX,

	// classic names
	I1, // idle
	I2, // idle2
	PD, // powerdown
	IU, // idle_unpowered
	C1, // construct
	C2, // construct2
	A1, // attack
	A2, // attack2
	S1, // spawn
	S2, // spawn2
	P1, // pain
	P2, // pain2
	DE, // destroy
	DU, // destroy_unpowered
	DD, // destroyed

	// custom names
	OP, // open
	CD, // closed

	// barricade
	BS, // shrink
	BU, // unshrink
	IS, // idle_shrunk
	PS, // pain_shrunk
	dS, // destroy_shrunk
	DS, // destroyed_shrunk

	NUM_SHORTHANDS
};

// Buildable animation names.
static const char* shorthandToName[ NUM_SHORTHANDS ] = {
	nullptr,

	"idle",
	"idle2",
	"powerdown",
	"idle_unpowered",
	"construct",
	"construct2",
	"attack",
	"attack2",
	"spawn",
	"spawn2",
	"pain",
	"pain2",
	"destroy",
	"destroy_unpowered",
	"destroyed",

	"open",
	"closed",

	"shrink",
	"unshrink",
	"idle_shrunk",
	"pain_shrunk",
	"destroy_shrunk",
	"destroyed_shrunk"
};

// Buildable animation flags.
#define BAF_LOOP    0x1
#define BAF_REVERSE 0x2

// Mapping of buildableAnimNumber_t numbers to animation names (through their shorthand) and flags.
// Every row is a buildable, every column is a buildableAnimNumber_t number.
// The pd suffix means "_POWERDOWN".
// TODO: Move this into buildable model config files (ideally after killing MD3).
// TODO: Make XX print an error and keep current animation, right now the model would become distorted.
static const struct { shorthand_t shorthand; int flags; } anims[ BA_NUM_BUILDABLES ][ MAX_BUILDABLE_ANIMATIONS ] = {
// NONE IDLE1  IDLE2  PWRDWN IDLEpd CNSTR  PWRUP  ATTCK1 ATTCK2 SPAWN1 SPAWN2 PAIN1  PAIN2  DESTR  DSTRpd DESTRD DSTRDpd  // BA_*
{{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0}}, // NONE
{{XX,0},{I1,1},{XX,0},{DD,0},{DD,1},{C1,0},{C1,0},{XX,0},{XX,0},{S1,0},{XX,0},{P1,0},{XX,0},{DE,0},{XX,0},{DD,0},{XX,0}}, // A_SPAWN
{{XX,0},{I1,1},{XX,0},{DD,0},{DD,1},{C1,0},{C1,0},{A1,0},{XX,0},{XX,0},{XX,0},{P1,0},{P2,0},{DE,0},{XX,0},{DD,0},{XX,0}}, // A_OVERMIND
{{XX,0},{I1,1},{XX,0},{BS,0},{IS,1},{C1,0},{BU,0},{XX,0},{XX,0},{XX,0},{XX,0},{P1,0},{PS,0},{DE,0},{dS,0},{DD,0},{DS,0}}, // A_BARRICADE
{{XX,0},{I1,1},{XX,0},{DD,0},{DD,1},{C1,0},{C1,0},{A1,0},{XX,0},{XX,0},{XX,0},{P1,0},{XX,0},{DE,0},{DE,0},{DD,0},{XX,0}}, // A_ACIDTUBE
{{XX,0},{I1,1},{XX,0},{DD,0},{DD,1},{C1,0},{C1,0},{A1,0},{XX,0},{XX,0},{XX,0},{P1,0},{XX,0},{DE,0},{DE,0},{DD,0},{XX,0}}, // A_TRAPPER
{{XX,0},{I1,1},{XX,0},{DD,0},{DD,1},{C1,0},{C1,0},{A1,0},{XX,0},{XX,0},{XX,0},{P1,0},{XX,0},{DE,0},{DE,0},{DD,0},{XX,0}}, // A_BOOSTER
{{XX,0},{I1,1},{XX,0},{DD,0},{DD,1},{C1,0},{C1,0},{A1,0},{XX,0},{XX,0},{XX,0},{P1,0},{XX,0},{DE,0},{DE,0},{DD,0},{XX,0}}, // A_HIVE
{{XX,0},{I1,1},{XX,0},{DD,0},{IU,1},{C1,0},{C1,0},{XX,0},{XX,0},{XX,0},{XX,0},{P1,0},{XX,0},{DE,0},{XX,0},{DD,0},{XX,0}}, // A_LEECH
{{XX,0},{I1,1},{XX,0},{DD,0},{DD,1},{C1,0},{C1,0},{A1,0},{XX,0},{XX,0},{XX,0},{P1,0},{XX,0},{DE,0},{DE,0},{DD,0},{XX,0}}, // A_SPIKER
{{XX,0},{I1,0},{XX,0},{XX,0},{I1,1},{I1,0},{XX,0},{XX,0},{XX,0},{S1,0},{XX,0},{XX,0},{XX,0},{I1,0},{XX,0},{I1,0},{XX,0}}, // H_SPAWN
{{XX,0},{I1,0},{XX,0},{I1,0},{I1,1},{I1,0},{I1,0},{I1,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{I1,0},{I1,0},{I1,0},{XX,0}}, // H_MGTURRET
{{XX,0},{I1,0},{XX,0},{OP,2},{CD,1},{OP,0},{OP,0},{I1,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{OP,2},{CD,0},{CD,0},{XX,0}}, // H_ROCKETPOD
{{XX,0},{I1,0},{XX,0},{I1,0},{I1,1},{I1,0},{I1,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{I1,0},{I1,0},{I1,0},{XX,0}}, // H_ARMOURY
{{XX,0},{I1,1},{I2,1},{PD,0},{I1,1},{C1,0},{I1,0},{A1,0},{C2,0},{XX,0},{XX,0},{XX,0},{XX,0},{DE,0},{DU,0},{DD,0},{XX,0}}, // H_MEDISTAT
{{XX,0},{I1,1},{XX,0},{I1,1},{I1,1},{I1,1},{I1,1},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{I1,0},{I1,0},{I1,0},{XX,0}}, // H_DRILL
{{XX,0},{I1,1},{XX,0},{XX,0},{I1,1},{C1,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{XX,0},{DE,0},{XX,0},{DD,0},{XX,0}}, // H_REACTOR
};

static const char *GetAnimationName( buildable_t buildable, buildableAnimNumber_t animNumber )
{
	return shorthandToName[ anims[ buildable ][ animNumber ].shorthand ];
}

static int GetAnimationFlags( buildable_t buildable, buildableAnimNumber_t animNumber )
{
	return anims[ buildable ][ animNumber ].flags;
}

static bool IsLooped( buildable_t buildable, buildableAnimNumber_t animNumber )
{
	return ( GetAnimationFlags( buildable, animNumber ) & BAF_LOOP );
}

static bool IsReversed( buildable_t buildable, buildableAnimNumber_t animNumber )
{
	return ( GetAnimationFlags( buildable, animNumber ) & BAF_REVERSE );
}


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

	trap_S_StartSound( origin, ENTITYNUM_WORLD, soundChannel_t::CHAN_AUTO, cgs.media.alienBuildableExplosion );

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
CG_HumanBuildableDying

Called for human buildables that are about to blow up
=================
*/
void CG_HumanBuildableDying( buildable_t buildable, vec3_t origin )
{
	switch ( buildable )
	{
		case BA_H_REACTOR:
			trap_S_StartSound( origin, ENTITYNUM_WORLD, soundChannel_t::CHAN_AUTO, cgs.media.humanBuildableDying );
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
	particleSystem_t *explosion = nullptr;
	particleSystem_t *nova = nullptr;

	if ( buildable == BA_H_REACTOR )
	{
		nova = CG_SpawnNewParticleSystem( cgs.media.humanBuildableNovaPS );
	}

	trap_S_StartSound( origin, ENTITYNUM_WORLD, soundChannel_t::CHAN_AUTO, cgs.media.humanBuildableExplosion );
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
			frac = sinf( 0.5f * msec / scaleUpTime * M_PI );
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
			frac = cosf( 0.5f * msec / CREEP_SCALEDOWN_TIME * M_PI );
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

	CG_Trace( &tr, cent->lerpOrigin, nullptr, nullptr, temp, cent->currentState.number,
	          MASK_PLAYERSOLID, 0 );

	if ( size > 0.0f && tr.fraction < 1.0f )
	{
		CG_ImpactMark( cgs.media.creepShader, tr.endpos, cent->currentState.origin2,
		               0.0f, 1.0f, 1.0f, 1.0f, 1.0f, false, size, true );
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

	if ( es->eType != entityType_t::ET_BUILDABLE )
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

	switch ( CG_Team(*es) )
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
static bool CG_ParseBuildableAnimationFile( const char *filename, buildable_t buildable )
{
	const char         *text_p;
	int          i;
	char         *token;
	float        fps;
	animation_t  *animations;

	animations = cg_buildables[ buildable ].animations;

	std::error_code err;
	std::string text = FS::PakPath::ReadFile( filename, err );
	if ( err )
	{
		Log::Warn( "couldn't read buildable animation file '%s': %s", filename, err.message() );
		return false;
	}

	// parse the text
	text_p = text.c_str();

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
		animations[ i ].reversed = false;
		animations[ i ].flipflop = false;

		// if numFrames is negative the animation is reversed
		if ( animations[ i ].numFrames < 0 )
		{
			animations[ i ].numFrames = -animations[ i ].numFrames;
			animations[ i ].reversed = true;
		}

		token = COM_Parse( &text_p );

		if ( !*token )
		{
			break;
		}

		animations[ i ].loopFrames = atoi( token );
		if ( animations[ i ].loopFrames && animations[ i ].loopFrames != animations[ i ].numFrames )
		{
			Log::Warn("CG_ParseBuildableAnimationFile: loopFrames != numFrames");
			animations[ i ].loopFrames = animations[ i ].numFrames;
		}

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
		Log::Warn( "Error parsing animation file: %s", filename );
		return false;
	}

	return true;
}

/*
======================
CG_ParseBuildableSoundFile

Read a configuration file containing sound properties
sound/buildables/hivemind/sound.cfg, etc
======================
*/
static bool CG_ParseBuildableSoundFile( const char *filename, buildable_t buildable )
{
	const char         *text_p;
	int          i;
	char         *token;
	sound_t      *sounds;

	sounds = cg_buildables[ buildable ].sounds;

	std::error_code err;
	std::string text = FS::PakPath::ReadFile( filename, err );
	if ( err )
	{
		Log::Warn( "couldn't read buildable sound file '%s': %s", filename, err.message() );
		return false;
	}

	// parse the text
	text_p = text.c_str();

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
		Log::Warn( "Error parsing sound file: %s", filename );
		return false;
	}

	return true;
}

static bool CG_RegisterBuildableAnimation( buildableInfo_t *ci, const char *modelName, int anim, const char *animName,
		bool loop, bool reversed, bool clearOrigin, bool iqm )
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
===============
CG_InitBuildables

Initialises the animation db
===============
*/
void CG_InitBuildables()
{
	char         filename[ MAX_QPATH ];
	char         soundfile[ MAX_QPATH ];
	const char   *buildableName;
	const char   *buildableIcon;
	const char   *modelFile;
	buildable_t  buildable;
	int          j;
	buildableAnimNumber_t anim;

	memset( cg_buildables, 0, sizeof( cg_buildables ) );

	//default sounds
	for ( j = BANIM_NONE + 1; j < MAX_BUILDABLE_ANIMATIONS; j++ )
	{
		Q_strncpyz( soundfile, cg_buildableSoundNames[ j - 1 ], sizeof soundfile );

		Com_sprintf( filename, sizeof( filename ), "sound/buildables/alien/%s", soundfile );
		defaultAlienSounds[ j ] = trap_S_RegisterSound( filename, false );

		Com_sprintf( filename, sizeof( filename ), "sound/buildables/human/%s", soundfile );
		defaultHumanSounds[ j ] = trap_S_RegisterSound( filename, false );
	}

	for ( buildable = (buildable_t)( BA_NONE + 1 ); buildable < BA_NUM_BUILDABLES;
	      buildable = (buildable_t)( buildable + 1 ) )
	{
		buildableInfo_t *bi = &cg_buildables[ buildable ];
		bool         iqm = false;

		buildableName = BG_Buildable( buildable )->name;
		//Load models
		//Prefer md5 models over md3

		if ( FS::PakPath::FileExists( va( "models/buildables/%s/%s.iqm", buildableName, buildableName ) ) &&
		     ( bi->models[ 0 ] = trap_R_RegisterModel( va( "models/buildables/%s/%s.iqm",
		                                                   buildableName, buildableName ) ) ) )
		{
			bi->md5 = true;
			iqm     = true;
		}
		else if ( ( bi->models[ 0 ] = trap_R_RegisterModel( va( "models/buildables/%s/%s.md5mesh",
		                                                        buildableName, buildableName ) ) ) )
		{
			bi->md5 = true;
		}
		else
		{
			bi->md5 = false;

			for ( j = 0; j < MAX_BUILDABLE_MODELS; j++ )
			{
				modelFile = BG_BuildableModelConfig( buildable )->models[ j ];

				if ( strlen( modelFile ) > 0 )
				{
					cg_buildables[ buildable ].models[ j ] = trap_R_RegisterModel( modelFile );
				}
			}
		}

		// If an md5mesh is found, register md5anims
		if ( bi->md5 )
		{
			for ( anim = (buildableAnimNumber_t)( BANIM_NONE + 1 ); anim < MAX_BUILDABLE_ANIMATIONS;
			      anim = (buildableAnimNumber_t)( anim + 1) )
			{
				const char *animName = GetAnimationName( buildable, anim );

				if ( !animName )
				{
					// No animation shall be loaded into this slot.
					continue;
				}

				if ( !CG_RegisterBuildableAnimation( bi, buildableName, anim, animName,
				                                     IsLooped( buildable, anim ),
				                                     IsReversed( buildable, anim ), false, iqm ) )
				{
					Log::Warn( "Failed to load animation '%s' for buildable '%s' and "
					            "animation slot #%d.", animName, buildableName, anim );
				}
			}
		}
		else // Not using md5s, fall back to md3s
		{
			//animation.cfg
			Com_sprintf( filename, sizeof( filename ), "models/buildables/%s/animation.cfg", buildableName );

			if ( !CG_ParseBuildableAnimationFile( filename, (buildable_t) buildable ) )
			{
				Log::Warn( "failed to load animation file %s", filename );
			}
		}

		//sound.cfg
		Com_sprintf( filename, sizeof( filename ), "sound/buildables/%s/sound.cfg", buildableName );

		if ( !CG_ParseBuildableSoundFile( filename, (buildable_t) buildable ) )
		{
			Log::Warn( "failed to load sound file %s", filename );
		}

		//sounds
		for ( j = BANIM_NONE + 1; j < MAX_BUILDABLE_ANIMATIONS; j++ )
		{
			Q_strncpyz( soundfile, cg_buildableSoundNames[ j - 1 ], sizeof soundfile );
			Com_sprintf( filename, sizeof( filename ), "sound/buildables/%s/%s", buildableName, soundfile );

			if ( cg_buildables[ buildable ].sounds[ j ].enabled )
			{
				cg_buildables[ buildable ].sounds[ j ].sound = trap_S_RegisterSound( filename, false );
				if ( cg_buildables[ buildable ].sounds[ j ].sound < 0 )
				{
					// file doesn't exist - use default
					if ( BG_Buildable( buildable )->team == TEAM_ALIENS )
					{
						cg_buildables[ buildable ].sounds[ j ].sound = defaultAlienSounds[ j ];
					}
					else
					{
						cg_buildables[ buildable ].sounds[ j ].sound = defaultHumanSounds[ j ];
					}
				}
			}
		}

		//icon
		if ( ( buildableIcon = BG_Buildable( buildable )->icon ) )
		{
			cg_buildables[ buildable ].buildableIcon = trap_R_RegisterShader( buildableIcon, (RegisterShaderFlags_t) ( RSF_NOMIP ) );
		}

		cg.buildableLoadingFraction = ( float ) buildable / ( float )( BA_NUM_BUILDABLES - 1 );
		trap_UpdateScreen();
	}

	cgs.media.reactorZapTS = CG_RegisterTrailSystem( "trails/weapons/reactor/lightning" );
}

/*
================
CG_GetBuildableRangeMarkerProperties
================
*/
static bool CG_GetBuildableRangeMarkerProperties( buildable_t bType, rangeMarker_t *rmType, float *range, Color::Color& rgba )
{
	shaderColorEnum_t shc;

	switch ( bType )
	{
		case BA_A_ACIDTUBE:
			*range = ACIDTUBE_RANGE;
			shc = SHC_ORANGE;
			break;

		case BA_A_SPIKER:
			*range = SPIKER_SENSE_RANGE;
			shc = SHC_PINK;
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
			*range = REGEN_BOOSTER_RANGE;
			shc = SHC_YELLOW;
			break;

		case BA_H_MGTURRET:
			*range = MGTURRET_RANGE;
			shc = SHC_ORANGE;
			break;

		case BA_H_ROCKETPOD:
			*range = ROCKETPOD_RANGE;
			shc = SHC_RED;
			break;

		case BA_H_DRILL:
			*range = RGS_RANGE;
			shc = SHC_GREY;
			break;

		default:
			return false;
	}

	if ( bType == BA_A_TRAPPER )
	{
		// HACK: Assumes certain trapper attributes
		*rmType = RM_SPHERICAL_CONE_64;
	}
	else if ( bType == BA_H_MGTURRET || bType == BA_H_ROCKETPOD )
	{
		// HACK: Assumes TURRET_PITCH_CAP == 30
		*rmType = RM_SPHERICAL_CONE_240;
	}
	else
	{
		*rmType = RM_SPHERE;
	}

	rgba = Color::Color(
		cg_shaderColors[shc][0],
		cg_shaderColors[shc][1],
		cg_shaderColors[shc][2],
		1.0f
	);

	return true;
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
		Sys::Drop( "Bad animation number: %i", newAnimation );
	}

	if ( cg_buildables[ buildable ].md5 )
	{
		if ( bSkeleton.type != refSkeletonType_t::SK_INVALID )
		{
			if ( lf->old_animation != nullptr && lf->old_animation->handle )
			{
				if ( !trap_R_BuildSkeleton( &oldbSkeleton, lf->old_animation->handle, lf->oldFrame, lf->frame, lf->blendlerp, lf->old_animation->clearOrigin ) )
				{
					Log::Warn( "Can't build old buildable bSkeleton" );
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

	if ( cg_debugAnim.Get() )
	{
		Log::Debug( "Anim: %i", newAnimation );
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
	buildableAnimNumber_t newAnimation = static_cast<buildableAnimNumber_t>( CG_AnimNumber( cent->buildableAnim  ) );

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation )
	{
		CG_SetBuildableLerpFrameAnimation( buildable, lf, newAnimation );

		if ( !cg_buildables[ buildable ].sounds[ newAnimation ].looped &&
		     cg_buildables[ buildable ].sounds[ newAnimation ].enabled )
		{
			trap_S_StartSound( cent->lerpOrigin, cent->currentState.number, soundChannel_t::CHAN_AUTO,
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

	if ( lf->animationEnded )
	{
		cent->buildableAnim = (buildableAnimNumber_t) cent->currentState.torsoAnim;
		cent->buildableIdleAnim = true;
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
		cent->buildableIdleAnim = true;
	}

	// display the first frame of the construction anim if not yet spawned
	if ( !( es->eFlags & EF_B_SPAWNED ) )
	{
		animation_t *anim = &cg_buildables[ es->modelindex ].animations[ BANIM_CONSTRUCT ];

		// Change the animation in the lerpFrame so that md5s will use it too.
		cent->lerpFrame.animation = &cg_buildables[ es->modelindex ].animations[ BANIM_CONSTRUCT ];

		// so that when animation starts for real it has sensible numbers
		cent->lerpFrame.oldFrameTime =
		  cent->lerpFrame.frameTime =
		    cent->lerpFrame.animationTime =
		      cg.time;

		*old = cent->lerpFrame.oldFrame = anim->firstFrame;
		*now = cent->lerpFrame.frame = anim->firstFrame;
		*backLerp = cent->lerpFrame.backlerp = 0.0f;

		// ensure that an animation is triggered once the buildable has spawned
		cent->oldBuildableAnim = BANIM_NONE;
	}
	else
	{
		if ( ( cent->oldBuildableAnim ^ es->legsAnim ) & ANIM_TOGGLEBIT )
		{
			if ( cg_debugAnim.Get() )
			{
				Log::Debug( "%d->%d l:%d t:%d %s(%d)",
				           cent->oldBuildableAnim & ~( ANIM_FORCEBIT | ANIM_TOGGLEBIT ), cent->buildableAnim & ~( ANIM_FORCEBIT | ANIM_TOGGLEBIT ),
				           es->legsAnim, es->torsoAnim,
				           BG_Buildable( es->modelindex )->humanName, es->number );
			}

			if ( cent->buildableAnim == es->torsoAnim || es->legsAnim & ANIM_FORCEBIT )
			{
				cent->buildableAnim = cent->oldBuildableAnim = (buildableAnimNumber_t) es->legsAnim;
				cent->buildableIdleAnim = false;
			}
			else
			{
				cent->buildableAnim = cent->oldBuildableAnim = (buildableAnimNumber_t) es->torsoAnim;
				cent->buildableIdleAnim = true;
			}
		}
		else if ( cent->buildableIdleAnim &&
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

#define TRACE_DEPTH 10.0f

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

	AngleVectors( angles, forward, nullptr, nullptr );
	VectorCopy( normal, outAxis[ 2 ] );
	ProjectPointOnPlane( outAxis[ 0 ], forward, outAxis[ 2 ] );

	if ( !VectorNormalize( outAxis[ 0 ] ) )
	{
		AngleVectors( angles, nullptr, nullptr, forward );
		ProjectPointOnPlane( outAxis[ 0 ], forward, outAxis[ 2 ] );
		VectorNormalize( outAxis[ 0 ] );
	}

	CrossProduct( outAxis[ 0 ], outAxis[ 2 ], outAxis[ 1 ] );
	outAxis[ 1 ][ 0 ] = -outAxis[ 1 ][ 0 ];
	outAxis[ 1 ][ 1 ] = -outAxis[ 1 ][ 1 ];
	outAxis[ 1 ][ 2 ] = -outAxis[ 1 ][ 2 ];

	VectorMA( inOrigin, -TRACE_DEPTH, normal, end );

	CG_CapTrace( &tr, inOrigin, mins, maxs, end, skipNumber, MASK_DEADSOLID, 0 );

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
	Color::Color rgba;

	if ( CG_GetBuildableRangeMarkerProperties( buildable, &rmType, &range, rgba ) )
	{
		vec3_t localOrigin;

		rgba.SetAlpha( opacity );

		if ( buildable == BA_A_HIVE )
		{
			VectorMA( origin, BG_BuildableModelConfig( buildable )->maxs[ 2 ], normal, localOrigin );
		}
		else
		{
			VectorCopy( origin, localOrigin );
		}

		if ( rmType == RM_SPHERE )
		{
			CG_DrawRangeMarker( rmType, localOrigin, range, nullptr, rgba );
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
// draw "ghost" buildings, that is, (probably) the blueprints showed to allow
// player to visualize where the building will go, and not (human) buildings
// which already entered the construction phase (and appear ghostly, too).
//
// buildableInfo is a 16 bits integer, with a valid buildable_t on the 8 lower
// bits, and a mask on the 8 higher bits (see SB_BUILDABLE_STATE_MASK).
void CG_GhostBuildable( int buildableInfo )
{
	playerState_t *ps;
	vec3_t        angles, entity_origin;
	vec3_t        mins, maxs;
	trace_t       tr;
	float         scale;
	buildable_t   buildable = (buildable_t)( buildableInfo & SB_BUILDABLE_MASK );
	ASSERT( BA_NONE < buildable && buildable < BA_NUM_BUILDABLES );
	const buildableModelConfig_t *bmc = BG_BuildableModelConfig( buildable );

	ps = &cg.predictedPlayerState;

	refEntity_t ent{};

	BG_BuildableBoundingBox( buildable, mins, maxs );

	BG_PositionBuildableRelativeToPlayer( ps, mins, maxs, CG_Trace, entity_origin, angles, &tr );

	if ( cg_rangeMarkerForBlueprint.Get() && tr.entityNum != ENTITYNUM_NONE )
	{
		CG_DrawBuildableRangeMarker( buildable, entity_origin, tr.plane.normal, 1.0f );
	}

	CG_PositionAndOrientateBuildable( angles, entity_origin, tr.plane.normal, ps->clientNum,
	                                  mins, maxs, ent.axis, ent.origin );

	//offset on the Z axis if required
	VectorMA( ent.origin, bmc->zOffset, tr.plane.normal, ent.origin );

	VectorCopy( ent.origin, ent.lightingOrigin );
	VectorCopy( ent.origin, ent.oldorigin );  // don't positionally lerp at all

	ent.hModel = cg_buildables[ buildable ].models[ 0 ];

	int reason = SB_BUILDABLE_TO_IBE( buildableInfo );
	if ( reason == IBE_NONE ) // can build
	{
		ent.customShader = cgs.media.greenBuildShader;
	}
	else if ( reason == IBE_NORMAL || reason == IBE_NOROOM || reason == IBE_SURFACE )
	{
		ent.customShader = cgs.media.redBuildShader;
	}
	else
	{
		ent.customShader = cgs.media.yellowBuildShader;
	}

	scale = bmc->modelScale;
	if ( !scale ) scale = 1.0f;

	if ( cg_buildables[ buildable ].md5 )
	{
		trap_R_BuildSkeleton( &ent.skeleton, cg_buildables[ buildable ].animations[ BANIM_IDLE1 ].handle, 0, 0, 0, false );
		CG_TransformSkeleton( &ent.skeleton, scale );
	}

	// Apply rotation from config.
	{
		matrix_t axisMat;
		quat_t   axisQuat, rotQuat;

		QuatFromAngles( rotQuat, bmc->modelRotation[ PITCH ], bmc->modelRotation[ YAW ],
		                bmc->modelRotation[ ROLL ] );

		MatrixFromVectorsFLU( axisMat, ent.axis[ 0 ], ent.axis[ 1 ], ent.axis[ 2 ] );
		QuatFromMatrix( axisQuat, axisMat );
		QuatMultiply2( axisQuat, rotQuat );
		MatrixFromQuat( axisMat, axisQuat );
		MatrixToVectorsFLU( axisMat, ent.axis[ 0 ], ent.axis[ 1 ], ent.axis[ 2 ] );
	}

	if ( scale != 1.0f )
	{
		VectorScale( ent.axis[ 0 ], scale, ent.axis[ 0 ] );
		VectorScale( ent.axis[ 1 ], scale, ent.axis[ 1 ] );
		VectorScale( ent.axis[ 2 ], scale, ent.axis[ 2 ] );

		ent.nonNormalizedAxes = true;
	}
	else
	{
		ent.nonNormalizedAxes = false;
	}

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

/*
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
	entity_origin[ 2 ] += ( std::abs( mins[ 2 ] ) + std::abs( maxs[ 2 ] ) ) / 2;

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


		const char *text = nullptr;
		qhandle_t  shader = 0;

		picM = picH * scale;
		picH = picM * ( 1.0f - bs->verticalMargin );

		Color::Color  backColor = bs->backColor;
		backColor.SetAlpha( bs->backColor.Alpha() / 3 );

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
			trap_R_SetColor( backColor );
			CG_DrawPic( picX - picM / 2, picY - picM / 2, picM, picM, cgs.media.whiteShader );
			trap_R_SetColor( bs->foreColor );
			CG_DrawPic( picX - picH / 2, picY - picH / 2, picH, picH, bs->noPowerShader );
			trap_R_ClearColor();
		}

		if ( text )
		{
			float     tx = 0, ty = 0;
			trap_R_SetColor( backColor );

			CG_DrawPic( tx - ( picM - picH ) / 2, ty - ( picM - picH ) / 4 - ( ty - picY ) * 2,
			            ( picX - tx ) * 2 + ( picM - picH ), ( ty - picY ) * 2 + ( picM - picH ),
			            cgs.media.whiteShader );

			trap_R_ClearColor();
		}
	}
}
*/

/*
==================
CG_BuildableParticleEffects
==================
*/
static void CG_BuildableParticleEffects( centity_t *cent )
{
	entityState_t *es = &cent->currentState;
	team_t        team = BG_Buildable( es->modelindex )->team;
	int           health = CG_Health(*es);
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
	Color::Color c;

	handle = Parse_LoadSourceHandle( filename, trap_FS_OpenPakFile );

	if ( !handle )
	{
		return;
	}

	while ( 1 )
	{
		if ( !Parse_ReadTokenHandle( handle, &token ) )
		{
			break;
		}

		if ( !Q_stricmp( token.string, "frameShader" ) )
		{
			if ( PC_String_Parse( handle, &s ) )
			{
				bs->frameShader = trap_R_RegisterShader( s, (RegisterShaderFlags_t) ( RSF_NOMIP ) );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "overlayShader" ) )
		{
			if ( PC_String_Parse( handle, &s ) )
			{
				bs->overlayShader = trap_R_RegisterShader( s, (RegisterShaderFlags_t) ( RSF_NOMIP ) );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "noPowerShader" ) )
		{
			if ( PC_String_Parse( handle, &s ) )
			{
				bs->noPowerShader = trap_R_RegisterShader( s, (RegisterShaderFlags_t) ( RSF_NOMIP ) );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "markedShader" ) )
		{
			if ( PC_String_Parse( handle, &s ) )
			{
				bs->markedShader = trap_R_RegisterShader( s, (RegisterShaderFlags_t) ( RSF_NOMIP ) );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "healthSevereColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				bs->healthSevereColor = c;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "healthHighColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				bs->healthHighColor = c;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "healthElevatedColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				bs->healthElevatedColor = c;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "healthGuardedColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				bs->healthGuardedColor = c;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "healthLowColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				bs->healthLowColor = c;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "foreColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				bs->foreColor = c;
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "backColor" ) )
		{
			if ( PC_Color_Parse( handle, &c ) )
			{
				bs->backColor = c;
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
			Log::Notice( "CG_BuildableStatusParse: unknown token %s in %s\n",
			            token.string, filename );
			bs->loaded = false;
			Parse_FreeSourceHandle( handle );
			return;
		}
	}

	bs->loaded = true;
	Parse_FreeSourceHandle( handle );
}

#define STATUS_FADE_TIME     200
#define STATUS_MAX_VIEW_DIST 900.0f
#define STATUS_PEEK_DIST     20

static Color::Color HealthColorFade( float healthFrac, buildStat_t *bs )
{
	healthFrac = Math::Clamp( healthFrac, 0.0f, 1.0f );

	if ( healthFrac == 1.0f )
	{
		return bs->healthLowColor;
	}

	Color::Color out;
	if ( healthFrac > 0.666f )
	{
		float frac = 1.0f - ( healthFrac - 0.666f ) * 3.0f;
		out = Color::Blend( bs->healthGuardedColor, bs->healthElevatedColor, frac );
	}
	else if ( healthFrac > 0.333f )
	{
		float frac = 1.0f - ( healthFrac - 0.333f ) * 3.0f;
		out = Color::Blend( bs->healthElevatedColor, bs->healthHighColor, frac );
	}
	else
	{
		float frac = 1.0f - healthFrac * 3.0f;
		out = Color::Blend( bs->healthHighColor, bs->healthSevereColor, frac );
	}

	return out;
}

static Color::Color DepletionColorFade( float frac, buildStat_t *bs )
{
	frac = Math::Clamp( frac, 0.0f, 1.0f );

	frac = frac * 0.6f + 0.4f;

	Color::Color out = bs->healthLowColor;
	out *= frac;
	return out;
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
	float         healthFrac, mineEfficiencyFrac = 0.0f;
	int           health = CG_Health(*es);
	float         x, y;
	bool          powered, marked, showMineEfficiency;
	trace_t       tr;
	float         d;
	buildStat_t   *bs;
	int           i, j;
	int           entNum;
	vec3_t        trOrigin;
	vec3_t        right;
	bool          visible = false;
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

	Color::Color color = bs->foreColor;

	// trace for center point
	BG_BuildableBoundingBox( es->modelindex, mins, maxs );

	// cull buildings outside the view frustum
	VectorAdd(cent->lerpOrigin, mins, cullMins);
	VectorAdd(cent->lerpOrigin, maxs, cullMaxs);

	if(CG_CullBox(cullMins, cullMaxs))
		return;

	// hack for shrunken barricades
	anim = CG_AnimNumber( es->torsoAnim );

	if ( es->modelindex == BA_A_BARRICADE &&
	     ( anim == BANIM_DESTROYED || !( es->eFlags & EF_B_SPAWNED ) ) )
	{
		maxs[ 2 ] = ( int )( maxs[ 2 ] * BARRICADE_SHRINKPROP );
	}

	VectorCopy( cent->lerpOrigin, origin );

	// center point
	origin[ 2 ] += mins[ 2 ];
	origin[ 2 ] += ( std::abs( mins[ 2 ] ) + std::abs( maxs[ 2 ] ) ) / 2;

	entNum = cg.predictedPlayerState.clientNum;

	// if first try fails, step left, step right
	for ( j = 0; j < 3 && !visible; j++ )
	{
		VectorCopy( cg.refdef.vieworg, trOrigin );

		switch ( j )
		{
			case 1:
				// step right
				AngleVectors( cg.refdefViewAngles, nullptr, right, nullptr );
				VectorMA( trOrigin, STATUS_PEEK_DIST, right, trOrigin );
				break;

			case 2:
				// step left
				AngleVectors( cg.refdefViewAngles, nullptr, right, nullptr );
				VectorMA( trOrigin, -STATUS_PEEK_DIST, right, trOrigin );
				break;

			default:
				break;
		}

		// look through up to 3 players and/or transparent buildables
		for ( i = 0; i < 3; i++ )
		{
			CG_Trace( &tr, trOrigin, nullptr, nullptr, origin, entNum, MASK_SHOT, 0 );

			if ( tr.entityNum == cent->currentState.number )
			{
				visible = true;
				break;
			}

			if ( tr.entityNum == ENTITYNUM_WORLD )
			{
				break;
			}

			hit = &cg_entities[ tr.entityNum ].currentState;

			if ( tr.entityNum < MAX_CLIENTS || ( hit->eType == entityType_t::ET_BUILDABLE &&
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
		cent->buildableStatus.visible = false;
		cent->buildableStatus.lastTime = cg.time;
	}
	else if ( visible && !cent->buildableStatus.visible )
	{
		cent->buildableStatus.visible = true;
		cent->buildableStatus.lastTime = cg.time;
	}

	// Fade up
	if ( cent->buildableStatus.visible )
	{
		if ( cent->buildableStatus.lastTime + STATUS_FADE_TIME > cg.time )
		{
			color.SetAlpha( static_cast<float>( cg.time - cent->buildableStatus.lastTime ) / STATUS_FADE_TIME );
		}
	}

	// Fade down
	if ( !cent->buildableStatus.visible )
	{
		if ( cent->buildableStatus.lastTime + STATUS_FADE_TIME > cg.time )
		{
			color.SetAlpha( 1.0f - static_cast<float>( cg.time - cent->buildableStatus.lastTime ) / STATUS_FADE_TIME );
		}
		else
		{
			return;
		}
	}

	// find out what data we want to display
	showMineEfficiency = ( attr->number == BA_A_LEECH || attr->number == BA_H_DRILL );

	// calculate mine efficiency bar size
	if ( showMineEfficiency )
	{
		mineEfficiencyFrac = static_cast<float>(es->weaponAnim) / 255.0f;

		mineEfficiencyFrac = Math::Clamp( mineEfficiencyFrac, 0.0f, 1.0f );
	}

	// calculate health bar size
	healthFrac = static_cast<float>(health)
		/ static_cast<float>(attr->health);
	healthFrac = Math::Clamp( healthFrac, 0.0f, 1.0f );

	if ( health > 0.0f && healthFrac < 0.01f )
	{
		healthFrac = 0.01f;
	}

	// draw elements
	if ( CG_WorldToScreen( origin, &x, &y ) )
	{
		float  picH = bs->frameHeight;
		float  picW = bs->frameWidth;
		float  picX = x;
		float  picY = y;
		float  scale, pad;
		float  subH, subY;
		float  clipX, clipY, clipW, clipH;
		Color::Color frameColor;

		// this is fudged to get the width/height in the cfg to be more realistic
		scale = ( picH / d ) * 3;
		pad   = bs->healthPadding * scale;

		powered = es->eFlags & EF_B_POWERED;
		marked = es->eFlags & EF_B_MARKED;

		picH *= scale;
		picW *= scale;
		picX -= ( picW * 0.5f );
		picY -= ( picH * 0.5f );

		// sub-elements such as icons and number
		subH = picH - ( picH * bs->verticalMargin );
		subY = picY + ( picH * 0.5f ) - ( subH * 0.5f );

		clipW = ( 640.0f * cg_viewsize.Get() ) / 100.0f;
		clipH = ( 480.0f * cg_viewsize.Get() ) / 100.0f;
		clipX = 320.0f - ( clipW * 0.5f );
		clipY = 240.0f - ( clipH * 0.5f );
		CG_SetClipRegion( clipX, clipY, clipW, clipH );

		// draw bar frames
		if ( bs->frameShader )
		{
			frameColor = bs->backColor;
			frameColor.SetAlpha( color.Alpha() );
			trap_R_SetColor( frameColor );

			if ( showMineEfficiency )
			{
				CG_SetClipRegion( picX, picY - 0.5f * picH, picW, 0.5f * picH );
				CG_DrawPic( picX, picY - 0.5f * picH, picW, picH, bs->frameShader );
				CG_ClearClipRegion();
			}

			CG_DrawPic( picX, picY, picW, picH, bs->frameShader );

			trap_R_ClearColor();
		}

		// draw mine rate bar
		if ( showMineEfficiency )
		{
			float  barX, barY, barW, barH;
			//float levelRate  = cg.predictedPlayerState.persistant[ PERS_MINERATE ] / 10.0f;

			barX = picX + pad;
			barY = picY - ( 0.5f * picH ) + pad;
			barW = ( picW - ( 2.0f * pad ) ) * mineEfficiencyFrac;
			barH = ( 0.5f * picH ) - pad;

			Color::Color barColor = DepletionColorFade( mineEfficiencyFrac, bs );
			barColor.SetAlpha( color.Alpha() );

			trap_R_SetColor( barColor );
			CG_DrawPic( barX, barY, barW, barH, cgs.media.whiteShader );
			trap_R_ClearColor();

			// TODO: Draw text using libRocket
			//UI_Text_Paint( barX + pad, barY + barH - pad, 0.3f * scale, colorBlack,
			//               va("Mines %.1f BP/min", mineEfficiencyFrac * levelRate), 0,
			//               ITEM_TEXTSTYLE_PLAIN );
		}

		// draw health bar
		if ( health > 0 )
		{
			float  barX, barY, barW, barH;

			barX = picX + ( bs->healthPadding * scale );
			barY = picY + ( bs->healthPadding * scale );
			barH = picH - ( bs->healthPadding * 2.0f * scale );
			barW = picW * healthFrac - ( bs->healthPadding * 2.0f * scale );

			Color::Color barColor = HealthColorFade( healthFrac, bs );
			barColor.SetAlpha( color.Alpha() );

			trap_R_SetColor( barColor );
			CG_DrawPic( barX, barY, barW, barH, cgs.media.whiteShader );

			trap_R_ClearColor();
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

			trap_R_ClearColor();
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

		trap_R_ClearColor();
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
static bool CG_PlayerIsBuilder( buildable_t buildable )
{
	switch ( cg.predictedPlayerState.weapon )
	{
		case WP_ABUILD:
		case WP_ABUILD2:
		case WP_HBUILD:
			return BG_Buildable( buildable )->team ==
			       BG_Weapon( cg.predictedPlayerState.weapon )->team;

		default:
			return false;
	}
}

/*
==================
CG_BuildableRemovalPending
==================
*/
static bool CG_BuildableRemovalPending( int entityNum )
{
	int           i;
	playerState_t *ps = &cg.snap->ps;

	if ( SB_BUILDABLE_TO_IBE( ps->stats[ STAT_BUILDABLE ] ) != IBE_NONE )
	{
		return false;
	}

	for ( i = 0; i < MAX_MISC; i++ )
	{
		if ( ps->misc[ i ] == entityNum )
		{
			return true;
		}
	}

	return false;
}

/*
==================
CG_DrawBuildableStatus
==================
*/
void CG_DrawBuildableStatus()
{
	centity_t     *cent;
	entityState_t *es;
	int           buildableList[ MAX_ENTITIES_IN_SNAPSHOT ];
	unsigned      buildables = 0;

	if ( !cg_drawBuildableHealth.Get() )
	{
		return;
	}

	for ( unsigned i = 0; i < cg.snap->entities.size(); i++ )
	{
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		es = &cent->currentState;

		if ( es->eType == entityType_t::ET_BUILDABLE && CG_PlayerIsBuilder( (buildable_t) es->modelindex ) )
		{
			buildableList[ buildables++ ] = cg.snap->entities[ i ].number;
		}
	}

	qsort( buildableList, buildables, sizeof( int ), CG_SortDistance );

	for ( unsigned i = 0; i < buildables; i++ )
	{
		CG_BuildableStatusDisplay( &cg_entities[ buildableList[ i ] ] );
	}

	if ( cg.predictedPlayerState.stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK )
	{
// 		CG_GhostBuildableStatus( cg.predictedPlayerState.stats[ STAT_BUILDABLE ] );
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
	entityState_t *es = &cent->currentState;
	vec3_t        angles;
	vec3_t        surfNormal, xNormal, mins, maxs;
	vec3_t        refNormal = { 0.0f, 0.0f, 1.0f };
	float         scale;
	int           health = CG_Health(cent);
	buildable_t   buildable = (buildable_t)es->modelindex;
	const buildableAttributes_t  *ba  = BG_Buildable( buildable );
	const buildableModelConfig_t *bmc = BG_BuildableModelConfig( buildable );
	team_t        team = ba->team;

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

	refEntity_t ent{};

	VectorCopy( es->origin2, surfNormal );

	VectorCopy( es->angles, angles );
	BG_BuildableBoundingBox( es->modelindex, mins, maxs );

	if ( es->pos.trType == trType_t::TR_STATIONARY )
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

	if( cg_drawBBOX.Get() > 0 )
	{
		CG_DrawBoundingBox( cg_drawBBOX.Get(), cent->lerpOrigin, mins, maxs );
	}

	//offset on the Z axis if required
	VectorMA( ent.origin, bmc->zOffset, surfNormal, ent.origin );

	VectorCopy( ent.origin, ent.oldorigin );  // don't positionally lerp at all
	VectorCopy( ent.origin, ent.lightingOrigin );

	ent.hModel = cg_buildables[ es->modelindex ].models[ 0 ];
	ent.shaderTime = float(double(cent->currentState.time) * 0.001);

	if ( !( es->eFlags & EF_B_SPAWNED ) )
	{
		sfxHandle_t prebuildSound = cgs.media.humanBuildablePrebuild;

		switch ( team )
		{
		case TEAM_HUMANS:
			ent.customShader = cgs.media.humanSpawningShader;
			prebuildSound = cgs.media.humanBuildablePrebuild;
			break;
		case TEAM_ALIENS:
			prebuildSound = cgs.media.alienBuildablePrebuild;
			break;
		default:
			ASSERT_UNREACHABLE();
		}

		trap_S_AddLoopingSound( es->number, cent->lerpOrigin, vec3_origin, prebuildSound );
	}

	CG_BuildableAnimation( cent, &ent.oldframe, &ent.frame, &ent.backlerp );

	// TODO: Merge the scaling and rotation of (non-)ghost buildables.

	// Apply rotation from config.
	{
		matrix_t axisMat;
		quat_t   axisQuat, rotQuat;

		QuatFromAngles( rotQuat, bmc->modelRotation[ PITCH ], bmc->modelRotation[ YAW ],
		                bmc->modelRotation[ ROLL ] );

		MatrixFromVectorsFLU( axisMat, ent.axis[ 0 ], ent.axis[ 1 ], ent.axis[ 2 ] );
		QuatFromMatrix( axisQuat, axisMat );
		QuatMultiply2( axisQuat, rotQuat );
		MatrixFromQuat( axisMat, axisQuat );
		MatrixToVectorsFLU( axisMat, ent.axis[ 0 ], ent.axis[ 1 ], ent.axis[ 2 ] );
	}

	// Apply scale from config.
	scale = bmc->modelScale;

	if ( scale != 1.0f )
	{
		VectorScale( ent.axis[ 0 ], scale, ent.axis[ 0 ] );
		VectorScale( ent.axis[ 1 ], scale, ent.axis[ 1 ] );
		VectorScale( ent.axis[ 2 ], scale, ent.axis[ 2 ] );

		ent.nonNormalizedAxes = true;
	}
	else
	{
		ent.nonNormalizedAxes = false;
	}

	// add inverse shadow map
	if ( cg_shadows > shadowingMode_t::SHADOWING_BLOB && cg_buildableShadows.Get() )
	{
		CG_StartShadowCaster( ent.lightingOrigin, mins, maxs );
	}

	if ( CG_PlayerIsBuilder( (buildable_t) es->modelindex ) && CG_BuildableRemovalPending( es->number ) )
	{
		ent.customShader = cgs.media.redBuildShader;
	}

	// TODO: Rename condition as this is true for iqm models, too.
	if ( cg_buildables[ es->modelindex ].md5 )
	{
		// If buildable has spawned or is a human buildable, don't alter the size
		bool  spawned = ( es->eFlags & EF_B_SPAWNED ) || ( team == TEAM_HUMANS );

		float adjustScale = spawned ? 1.0f :
			sinf( static_cast<float>(cg.time - es->time) / ba->buildTime * M_PI/2.0f );
		ent.skeleton = bSkeleton;

		if( es->modelindex == BA_H_MGTURRET || es->modelindex == BA_H_ROCKETPOD )
		{
			quat_t   rotation;
			matrix_t mat;
			vec3_t   nBounds[ 2 ];
			float    yaw, pitch, roll;

			yaw   = es->angles2[ YAW ];
			pitch = es->angles2[ PITCH ];

			// TODO: Access bones by name instead of by number.

			// The roll of Bone_platform is the turrets' yaw.
			QuatFromAngles( rotation, 0, 0, yaw );
			QuatMultiply2( ent.skeleton.bones[ 1 ].t.rot, rotation );

			// The roll of Bone_gatlin is the turrets' pitch.
			QuatFromAngles( rotation, 0, 0, pitch );
			QuatMultiply2( ent.skeleton.bones[ 2 ].t.rot, rotation );

			// The roll of Bone_barrel is the mgturret's barrel roll.
			if ( es->modelindex == BA_H_MGTURRET )
			{
				roll = Math::Clamp( ( 1.0f / MGTURRET_ATTACK_PERIOD ) *
				        120.0f * ( cg.time - cent->muzzleFlashTime ), 0.0f, 120.0f );

				QuatFromAngles( rotation, 0, 0, roll );
				QuatMultiply2( ent.skeleton.bones[ 3 ].t.rot, rotation );
			}

			// transform bounds so they more accurately reflect the turrets' new transformation
			// TODO: Evaluate
			MatrixFromAngles( mat, pitch, yaw, 0 );

			MatrixTransformBounds(mat, ent.skeleton.bounds[0], ent.skeleton.bounds[1], nBounds[0], nBounds[1]);

			BoundsAdd( ent.skeleton.bounds[ 0 ], ent.skeleton.bounds[ 1 ], nBounds[ 0 ], nBounds[ 1 ] );
		}

#define OVERMIND_EYE_CLAMP    43.0f // 45° shows seams due to imperfections of the low poly version.
#define OVERMIND_EYE_LAMBDA   10.0f
#define OVERMIND_IDLE_ANGLE   ( 0.3f * OVERMIND_EYE_CLAMP )
#define OVERMIND_EYE_Z_OFFSET 64.0f

		// Handle overmind eye movement.
		if( es->modelindex == BA_A_OVERMIND )
		{
			vec3_t   dirToTarget, angles, eyeOrigin;
			quat_t   rotation;

			// Calculate relative angles to target.
			// TODO: Also check if target entity is known.
			if ( es->otherEntityNum == ENTITYNUM_NONE /*||
				 !cg_entities[ es->otherEntityNum ].valid*/ )
			{
				int randSeed = cg.time / 3000;

				// HACK: Mess with the seed once because the first output of Q_crandom is so bad.
				Q_crandom( &randSeed );

				angles[ PITCH ] = Q_crandom( &randSeed ) * OVERMIND_IDLE_ANGLE;
				angles[ YAW ]   = Q_crandom( &randSeed ) * OVERMIND_IDLE_ANGLE;
				angles[ ROLL ]  = 0.0f;
			}
			else
			{
				// HACK: Fixed offset for eye height.
				// TODO: Retrieve eye origin from skeleton.
				VectorCopy( es->origin, eyeOrigin );
				eyeOrigin[ 2 ] += OVERMIND_EYE_Z_OFFSET;

				// Get absolute angles to target.
				VectorSubtract( cg_entities[ es->otherEntityNum ].lerpOrigin, eyeOrigin, dirToTarget );
				VectorNormalize( dirToTarget );
				vectoangles( dirToTarget, angles );

				// Transform into relative angles.
				angles[ PITCH ] -= es->angles[ PITCH ];
				angles[ YAW ]   -= ( es->angles[ YAW ] - 180.0f );
				angles[ ROLL ]  = 0;

				// Limit angles.
				if ( angles[ PITCH ] < -180.0f ) angles[ PITCH ] += 360.0f;
				if ( angles[ PITCH ] >  180.0f ) angles[ PITCH ] -= 360.0f;
				if ( angles[ YAW ]   < -180.0f ) angles[ YAW ]   += 360.0f;
				if ( angles[ YAW ]   >  180.0f ) angles[ YAW ]   -= 360.0f;
				angles[ PITCH ] = Math::Clamp( angles[ PITCH ], -OVERMIND_EYE_CLAMP, OVERMIND_EYE_CLAMP );
				angles[ YAW ]   = Math::Clamp( angles[ YAW ],   -OVERMIND_EYE_CLAMP, OVERMIND_EYE_CLAMP );
			}

			// Smooth out movement.
			ExponentialFade( &cent->overmindEyeAngle[ PITCH ], angles[ PITCH ], OVERMIND_EYE_LAMBDA,
			                 0.001f * cg.frametime );
			ExponentialFade( &cent->overmindEyeAngle[ YAW ],   angles[ YAW ],   OVERMIND_EYE_LAMBDA,
			                 0.001f * cg.frametime );

			// TODO: Access bone by name instead of by number.
			// Note that rotation's pitch is the eye's roll and vice versa.
			// Also the yaw needs to be inverted.
			QuatFromAngles( rotation, 0, -cent->overmindEyeAngle[ YAW ], cent->overmindEyeAngle[ PITCH ] );
			QuatMultiply2( ent.skeleton.bones[ 38 ].t.rot, rotation );
		}

		CG_TransformSkeleton( &ent.skeleton, adjustScale );
	}

	if ( health <= 0 )
	{
		ent.altShaderIndex = CG_ALTSHADER_DEAD;
	}
	else if ( !(es->eFlags & EF_B_POWERED) )
	{
		ent.altShaderIndex = CG_ALTSHADER_UNPOWERED;
	}
	else if ( cent->buildableAnim == BANIM_IDLE1 )
	{
		ent.altShaderIndex = CG_ALTSHADER_IDLE;
	}
	else if ( cent->buildableAnim == BANIM_IDLE2 )
	{
		ent.altShaderIndex = CG_ALTSHADER_IDLE2;
	}


	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );

	CrossProduct( surfNormal, refNormal, xNormal );
	VectorNormalize( xNormal );

	// weapon effects
	if ( es->eFlags & EF_FIRING )
	{
		weaponInfo_t *wi = &cg_weapons[ es->weapon ];

		// muzzle flash
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
				if ( cg_buildables[ es->modelindex ].md5 )
				{
					switch ( es->modelindex )
					{
						case BA_H_ROCKETPOD:
						{
							int num = ( cent->muzzleFlashTime / ROCKETPOD_ATTACK_PERIOD ) % 6;
							const char *side = ( num % 2 == 0 ) ? "left" : "right";
							num = ( num % 3 ) + 1;
							CG_SetAttachmentTag( &cent->muzzlePS->attachment, &ent, ent.hModel,
							                     va( "Bone_fire_%s_%d", side, num ) );
							break;
						}

						default:
							CG_SetAttachmentTag( &cent->muzzlePS->attachment, &ent, ent.hModel, "Bone_fire" );
							break;
					}
				}
				else
				{
					CG_SetAttachmentTag( &cent->muzzlePS->attachment, &ent, ent.hModel, "tag_flash" );
				}

				CG_AttachToTag( &cent->muzzlePS->attachment );
			}
		}

		// dynamic light
		if ( cg.time - cent->muzzleFlashTime < MUZZLE_FLASH_TIME )
		{
			if ( wi->wim[ WPM_PRIMARY ].flashDlight )
			{
				trap_R_AddLightToScene( cent->lerpOrigin, wi->wim[ WPM_PRIMARY ].flashDlight,
				                        wi->wim[ WPM_PRIMARY ].flashDlightIntensity,
				                        wi->wim[ WPM_PRIMARY ].flashDlightColor[ 0 ],
				                        wi->wim[ WPM_PRIMARY ].flashDlightColor[ 1 ],
				                        wi->wim[ WPM_PRIMARY ].flashDlightColor[ 2 ], 0, 0 );
			}
		}

		// Play firing sound.
		if ( wi->wim[ WPM_PRIMARY ].firingSound )
		{
			trap_S_AddLoopingSound( es->number, cent->lerpOrigin, vec3_origin,
			                        wi->wim[ WPM_PRIMARY ].firingSound );
		}
	}
	else // Not firing.
	{
		// Destroy active muzzle PS.
		if ( CG_IsParticleSystemValid( &cent->muzzlePS ) )
		{
			CG_DestroyParticleSystem( &cent->muzzlePS );
		}

		// Play lockon sound if applicable.
		if ( health > 0 && ( es->eFlags & EF_B_LOCKON ) )
		{
			trap_S_AddLoopingSound( es->number, cent->lerpOrigin, vec3_origin,
			                        cgs.media.rocketpodLockonSound );
		}
	}

	if ( health < cent->lastBuildableHealth && ( es->eFlags & EF_B_SPAWNED ) )
	{
		if ( cent->lastBuildableDamageSoundTime + BUILDABLE_SOUND_PERIOD < cg.time )
		{
			if ( team == TEAM_HUMANS )
			{
				int i = rand() % 4;
				trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_BODY, cgs.media.humanBuildableDamage[ i ] );
			}

			cent->lastBuildableDamageSoundTime = cg.time;
		}
	}

	cent->lastBuildableHealth = health;

	// set particle effect to fire if buildable is burning
	CG_OnFire( cent );

	// smoke etc for damaged buildables
	CG_BuildableParticleEffects( cent );

	// draw range marker if enabled
	if( team == CG_MyTeam() ) {
		bool drawRange;
		float dist, maxDist = std::max( RADAR_RANGE, ALIENSENSE_RANGE );

		if ( team == TEAM_HUMANS ) {
			drawRange = BG_InventoryContainsWeapon( WP_HBUILD, cg.predictedPlayerState.stats );
		} else if ( team == TEAM_ALIENS ) {
			drawRange = cg.predictedPlayerState.weapon == WP_ABUILD ||
			            cg.predictedPlayerState.weapon == WP_ABUILD2;
		} else {
			drawRange = cg_rangeMarkerWhenSpectating.Get();
		}

		drawRange &= (cg_buildableRangeMarkerMask & (1 << ba->number)) != 0;

		dist = Distance( cent->lerpOrigin, cg.refdef.vieworg );

		if( drawRange && dist <= maxDist ) {
			float opacity = 1.0f;

			if( dist >= 0.9f * maxDist ) {
				opacity = 10.0f - 10.0f * dist / maxDist;
			}

			CG_DrawBuildableRangeMarker( ba->number, cent->lerpOrigin, cent->currentState.origin2, opacity );
		}
	}

	if ( cg_shadows > shadowingMode_t::SHADOWING_BLOB && cg_buildableShadows.Get() )
	{
		CG_EndShadowCaster( );
	}
}

// maybe move this to shared/ and add a prototype?
static bool IsMainBuildable(buildable_t buildable)
{
	return buildable == BA_A_OVERMIND || buildable == BA_H_REACTOR;
}

const centity_t *CG_LookupMainBuildable()
{
	for( int beaconNum = 0; beaconNum < cg.beaconCount; beaconNum++ ) {
		const auto b = cg.beacons[ beaconNum ];
		if ( b->type == BCT_TAG && !(b->flags & (EF_BC_TAG_PLAYER|EF_BC_ENEMY))
				&& IsMainBuildable( static_cast<buildable_t>(b->data) ) )
		{
			return &cg_entities[ b->target ];
		}
	}

	return nullptr;
}

// keep in sync with G_DistanceToBase
float CG_DistanceToBase()
{
	const centity_t *ent = CG_LookupMainBuildable();
	if (!ent)
		return 1e+37f; // in accordance to sgame
	return Distance(cg.predictedPlayerEntity.lerpOrigin, ent->lerpOrigin);
}
