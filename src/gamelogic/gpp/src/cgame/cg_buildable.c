/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of OpenWolf.

OpenWolf is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenWolf is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


#include "cg_local.h"

char *cg_buildableSoundNames[ MAX_BUILDABLE_ANIMATIONS ] =
{
  "construct1.wav",
  "construct2.wav",
  "idle1.wav",
  "idle2.wav",
  "idle3.wav",
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

static sfxHandle_t defaultAlienSounds[ MAX_BUILDABLE_ANIMATIONS ];
static sfxHandle_t defaultHumanSounds[ MAX_BUILDABLE_ANIMATIONS ];

/*
===================
CG_AlienBuildableExplosion

Generated a bunch of gibs launching out from a location
===================
*/
void CG_AlienBuildableExplosion( vec3_t origin, vec3_t dir )
{
  particleSystem_t  *ps;

  trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.alienBuildableExplosion );

  //particle system
  ps = CG_SpawnNewParticleSystem( cgs.media.alienBuildableDestroyedPS );

  if( CG_IsParticleSystemValid( &ps ) )
  {
    CG_SetAttachmentPoint( &ps->attachment, origin );
    CG_SetParticleSystemNormal( ps, dir );
    CG_AttachToPoint( &ps->attachment );
  }
}

/*
=================
CG_HumanBuildableExplosion

Called for human buildables as they are destroyed
=================
*/
void CG_HumanBuildableExplosion( vec3_t origin, vec3_t dir )
{
  particleSystem_t  *ps;

  trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.humanBuildableExplosion );

  //particle system
  ps = CG_SpawnNewParticleSystem( cgs.media.humanBuildableDestroyedPS );

  if( CG_IsParticleSystemValid( &ps ) )
  {
    CG_SetAttachmentPoint( &ps->attachment, origin );
    CG_SetParticleSystemNormal( ps, dir );
    CG_AttachToPoint( &ps->attachment );
  }
}


#define CREEP_SIZE            64.0f
#define CREEP_DISTANCE        64.0f

/*
==================
CG_Creep
==================
*/
static void CG_Creep( centity_t *cent )
{
  int           msec;
  float         size, frac;
  trace_t       tr;
  vec3_t        temp, origin;
  int           scaleUpTime = BG_Buildable( cent->currentState.modelindex )->buildTime;
  int           time;

  time = cent->currentState.time;

  //should the creep be growing or receding?
  if( time >= 0 )
  {
    msec = cg.time - time;
    if( msec >= 0 && msec < scaleUpTime )
      frac = (float)msec / scaleUpTime;
    else
      frac = 1.0f;
  }
  else if( time < 0 )
  {
    msec = cg.time + time;
    if( msec >= 0 && msec < CREEP_SCALEDOWN_TIME )
      frac = 1.0f - ( (float)msec / CREEP_SCALEDOWN_TIME );
    else
      frac = 0.0f;
  }

  VectorCopy( cent->currentState.origin2, temp );
  VectorScale( temp, -CREEP_DISTANCE, temp );
  VectorAdd( temp, cent->lerpOrigin, temp );

  CG_Trace( &tr, cent->lerpOrigin, NULL, NULL, temp, cent->currentState.number, MASK_PLAYERSOLID );

  VectorCopy( tr.endpos, origin );

  size = CREEP_SIZE * frac;

  if( size > 0.0f && tr.fraction < 1.0f )
    CG_ImpactMark( cgs.media.creepShader, origin, cent->currentState.origin2,
                   0.0f, 1.0f, 1.0f, 1.0f, 1.0f, qfalse, size, qtrue );
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
  char          *text_p;
  int           len;
  int           i;
  char          *token;
  float         fps;
  char          text[ 20000 ];
  fileHandle_t  f;
  animation_t   *animations;

  animations = cg_buildables[ buildable ].animations;

  // load the file
  len = trap_FS_FOpenFile( filename, &f, FS_READ );
  if( len < 0 )
    return qfalse;

  if( len == 0 || len >= sizeof( text ) - 1 )
  {
    trap_FS_FCloseFile( f );
    CG_Printf( "File %s is %s\n", filename, len == 0 ? "empty" : "too long" );
    return qfalse;
  }

  trap_FS_Read( text, len, f );
  text[ len ] = 0;
  trap_FS_FCloseFile( f );

  // parse the text
  text_p = text;

  // read information for each frame
  for( i = BANIM_NONE + 1; i < MAX_BUILDABLE_ANIMATIONS; i++ )
  {

    token = COM_Parse( &text_p );
    if( !*token )
      break;

    animations[ i ].firstFrame = atoi( token );

    token = COM_Parse( &text_p );
    if( !*token )
      break;

    animations[ i ].numFrames = atoi( token );
    animations[ i ].reversed = qfalse;
    animations[ i ].flipflop = qfalse;

    // if numFrames is negative the animation is reversed
    if( animations[ i ].numFrames < 0 )
    {
      animations[ i ].numFrames = -animations[ i ].numFrames;
      animations[ i ].reversed = qtrue;
    }

    token = COM_Parse( &text_p );
    if ( !*token )
      break;

    animations[i].loopFrames = atoi( token );

    token = COM_Parse( &text_p );
    if( !*token )
      break;

    fps = atof( token );
    if( fps == 0 )
      fps = 1;

    animations[ i ].frameLerp = 1000 / fps;
    animations[ i ].initialLerp = 1000 / fps;
  }

  if( i != MAX_BUILDABLE_ANIMATIONS )
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
  char          *text_p;
  int           len;
  int           i;
  char          *token;
  char          text[ 20000 ];
  fileHandle_t  f;
  sound_t       *sounds;

  sounds = cg_buildables[ buildable ].sounds;

  // load the file
  len = trap_FS_FOpenFile( filename, &f, FS_READ );
  if ( len < 0 )
    return qfalse;

  if ( len == 0 || len >= sizeof( text ) - 1 )
  {
    trap_FS_FCloseFile( f );
    CG_Printf( "File %s is %s\n", filename, len == 0 ? "empty" : "too long" );
    return qfalse;
  }

  trap_FS_Read( text, len, f );
  text[len] = 0;
  trap_FS_FCloseFile( f );

  // parse the text
  text_p = text;

  // read information for each frame
  for( i = BANIM_NONE + 1; i < MAX_BUILDABLE_ANIMATIONS; i++ )
  {

    token = COM_Parse( &text_p );
    if ( !*token )
      break;

    sounds[ i ].enabled = atoi( token );

    token = COM_Parse( &text_p );
    if ( !*token )
      break;

    sounds[ i ].looped = atoi( token );

  }

  if( i != MAX_BUILDABLE_ANIMATIONS )
  {
    CG_Printf( "Error parsing sound file: %s\n", filename );
    return qfalse;
  }

  return qtrue;
}

static qboolean CG_RegisterBuildableAnimation(buildableInfo_t * ci, const char *modelName, int anim, const char *animName,
										  qboolean loop, qboolean reversed, qboolean clearOrigin)
{
	char            filename[MAX_QPATH];
	int             frameRate;

	Com_sprintf(filename, sizeof(filename), "models/buildables/%s/%s.md5anim", modelName, animName);
	ci->animations[anim].handle = trap_R_RegisterAnimation(filename);
	if(!ci->animations[anim].handle)
	{
		Com_Printf("Failed to load animation file %s\n", filename);
		return qfalse;
	}

	ci->animations[anim].firstFrame = 0;
	ci->animations[anim].numFrames = trap_R_AnimNumFrames(ci->animations[anim].handle);
	frameRate = trap_R_AnimFrameRate(ci->animations[anim].handle);

	if(frameRate == 0)
	{
		frameRate = 1;
	}
	ci->animations[anim].frameLerp = 1000 / frameRate;
	ci->animations[anim].initialLerp = 1000 / frameRate;

	if(loop)
	{
		ci->animations[anim].loopFrames = ci->animations[anim].numFrames;
	}
	else
	{
		ci->animations[anim].loopFrames = 0;
	}

	ci->animations[anim].reversed = reversed;
	ci->animations[anim].clearOrigin = clearOrigin;

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
  char          filename[ MAX_QPATH ];
  char          soundfile[ MAX_QPATH ];
  char          *buildableName;
  char          *modelFile;
  int           i;
  int           j;
  int		n;
  fileHandle_t  f;

  memset( cg_buildables, 0, sizeof( cg_buildables ) );

  //default sounds
  for( j = BANIM_NONE + 1; j < MAX_BUILDABLE_ANIMATIONS; j++ )
  {
    strcpy( soundfile, cg_buildableSoundNames[ j - 1 ] );

    Com_sprintf( filename, sizeof( filename ), "sound/buildables/alien/%s", soundfile );
    defaultAlienSounds[ j ] = trap_S_RegisterSound( filename, qfalse );

    Com_sprintf( filename, sizeof( filename ), "sound/buildables/human/%s", soundfile );
    defaultHumanSounds[ j ] = trap_S_RegisterSound( filename, qfalse );
  }

  cg.buildablesFraction = 0.0f;

  for( i = BA_NONE + 1; i < BA_NUM_BUILDABLES; i++ )
  {
    buildableInfo_t *bi = &cg_buildables[ i ];
    buildableName = BG_Buildable( i )->name;
    //Load models
    //Prefer md5 models over md3
    
    if( ( bi->models[0] = trap_R_RegisterModel( va( "models/buildables/%s/%s.md5mesh", buildableName, buildableName ) ) ) )
      bi->md5 = qtrue;
    else
    {
      bi->md5 = qfalse;
      for( j = 0; j <= 3; j++ )
      {
	modelFile = BG_BuildableConfig( i )->models[ j ];
	if( strlen( modelFile ) > 0 )
	  cg_buildables[ i ].models[ j ] = trap_R_RegisterModel( modelFile );
      }
    }
    
    // If an md5mesh is found, register md5anims
    if( bi->md5 )
    {
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_IDLE1, 
	"idle", qtrue, qfalse, qfalse ) )
      {
          //CG_Error( "could not find '%s'", path );
      }
      for( n = BANIM_NONE; n < MAX_BUILDABLE_ANIMATIONS; n++ )
      {
	if( n == BANIM_IDLE1 )
	  continue;
	bi->animations[ n ] = bi->animations[ BANIM_IDLE1 ];
      }
      //Register the rest of the buildable animations
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_CONSTRUCT1, 
	"construct", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_CONSTRUCT1] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_CONSTRUCT2, 
	"construct2", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_CONSTRUCT2] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_IDLE2, 
	"idle2", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_IDLE2] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_IDLE3, 
	"idle3", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_IDLE3] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_ATTACK1, 
	"attack", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_ATTACK1] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_ATTACK2, 
	"attack2", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_ATTACK2] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_SPAWN1, 
	"spawn", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_SPAWN1] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_SPAWN2, 
	"spawn2", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_SPAWN2] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_PAIN1, 
	"pain", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_PAIN1] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_PAIN2, 
	"pain2", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_PAIN2] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_DESTROY1, 
	"destroy", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_DESTROY1] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_DESTROY2, 
	"destroy2", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_DESTROY2] = bi->animations[BANIM_IDLE1];
      if( !CG_RegisterBuildableAnimation( bi, buildableName, BANIM_DESTROYED,
	"destroyed", qtrue, qfalse, qfalse ) )
	bi->animations[BANIM_DESTROYED] = bi->animations[BANIM_IDLE1];
    }
    else // Not using md5s, fall back to md3s
    {
     
    //animation.cfg
    Com_sprintf( filename, sizeof( filename ), "models/buildables/%s/animation.cfg", buildableName );
    if ( !CG_ParseBuildableAnimationFile( filename, i ) )
      Com_Printf( S_COLOR_YELLOW "WARNING: failed to load animation file %s\n", filename );
    }
    
    //sound.cfg
    Com_sprintf( filename, sizeof( filename ), "sound/buildables/%s/sound.cfg", buildableName );
    if ( !CG_ParseBuildableSoundFile( filename, i ) )
      Com_Printf( S_COLOR_YELLOW "WARNING: failed to load sound file %s\n", filename );


    //sounds
    for( j = BANIM_NONE + 1; j < MAX_BUILDABLE_ANIMATIONS; j++ )
    {
      strcpy( soundfile, cg_buildableSoundNames[ j - 1 ] );
      Com_sprintf( filename, sizeof( filename ), "sound/buildables/%s/%s", buildableName, soundfile );

      if( cg_buildables[ i ].sounds[ j ].enabled )
      {
        if( trap_FS_FOpenFile( filename, &f, FS_READ ) > 0 )
        {
          //file exists so close it
          trap_FS_FCloseFile( f );

          cg_buildables[ i ].sounds[ j ].sound = trap_S_RegisterSound( filename, qfalse );
        }
        else
        {
          //file doesn't exist - use default
          if( BG_Buildable( i )->team == TEAM_ALIENS )
            cg_buildables[ i ].sounds[ j ].sound = defaultAlienSounds[ j ];
          else
            cg_buildables[ i ].sounds[ j ].sound = defaultHumanSounds[ j ];
        }
      }
    }

    cg.buildablesFraction = (float)i / (float)( BA_NUM_BUILDABLES - 1 );
    trap_UpdateScreen( );
  }

  cgs.media.teslaZapTS = CG_RegisterTrailSystem( "models/buildables/tesla/zap" );
}

/*
================
CG_BuildableRangeMarkerProperties
================
*/
qboolean CG_GetBuildableRangeMarkerProperties( buildable_t bType, int *rmType, float *range, vec3_t rgb )
{
  shaderColorEnum_t shc;

  switch( bType )
  {
    case BA_A_SPAWN:    *range = CREEP_BASESIZE;    shc = SHC_LIGHT_GREEN; break;
    case BA_A_OVERMIND: *range = CREEP_BASESIZE;    shc = SHC_DARK_GREEN;  break;
    case BA_A_ACIDTUBE: *range = ACIDTUBE_RANGE;    shc = SHC_RED;         break;
    case BA_A_TRAPPER:  *range = TRAPPER_RANGE;     shc = SHC_PINK;        break;
    case BA_A_HIVE:     *range = HIVE_SENSE_RANGE;  shc = SHC_YELLOW;      break;
    case BA_H_MGTURRET: *range = MGTURRET_RANGE;    shc = SHC_ORANGE;      break;
    case BA_H_TESLAGEN: *range = TESLAGEN_RANGE;    shc = SHC_VIOLET;      break;
    case BA_H_DCC:      *range = DC_RANGE;          shc = SHC_GREEN_CYAN;  break;
    case BA_H_REACTOR:  *range = REACTOR_BASESIZE;  shc = SHC_DARK_BLUE;   break;
    case BA_H_REPEATER: *range = REPEATER_BASESIZE; shc = SHC_LIGHT_BLUE;  break;
    default: return qfalse;
  }

  if( bType == BA_A_TRAPPER )
    *rmType = 1;
  else if( bType == BA_H_MGTURRET )
    *rmType = 2;
  else
    *rmType = 0;

  VectorCopy( cg_shaderColors[ shc ], rgb );

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

  if( newAnimation < 0 || newAnimation >= MAX_BUILDABLE_ANIMATIONS )
    CG_Error( "Bad animation number: %i", newAnimation );

  anim = &cg_buildables[ buildable ].animations[ newAnimation ];

  //this item has just spawned so lf->frameTime will be zero
  if( !lf->animation )
    lf->frameTime = cg.time + 1000; //1 sec delay before starting the spawn anim

  lf->animation = anim;
  lf->animationTime = lf->frameTime + anim->initialLerp;

  if( cg_debugAnim.integer )
    CG_Printf( "Anim: %i\n", newAnimation );
  
  if( lf->old_animationNumber <= 0 ) // Skip initial / invalid blending
  {
    lf->blendlerp = 0.0f;
    return;
  }
  
  if( lf->blendlerp <= 0.0f )
    lf->blendlerp = 1.0f;
  else
    lf->blendlerp = 1.0f - lf->blendlerp; //use old blending for smooth blending between two blended animations
  if( cg_buildables[buildable].md5 )
  {
    if( lf->skeleton.type != SK_INVALID ) {
      memcpy( &lf->oldSkeleton, &lf->skeleton, sizeof( refSkeleton_t ) );
      
      if( lf->old_animation != NULL ) {
	if( !trap_R_BuildSkeleton( &lf->oldSkeleton, lf->old_animation->handle, lf->oldFrame, lf->frame, lf->blendlerp, lf->old_animation->clearOrigin ) )
	{
	  CG_Printf( "Can't build old buildable skeleton\n" );
	  return;
	}
      }
    }
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
  buildable_t           buildable = cent->currentState.modelindex;
  lerpFrame_t           *lf = &cent->lerpFrame;
  buildableAnimNumber_t newAnimation = cent->buildableAnim & ~( ANIM_TOGGLEBIT|ANIM_FORCEBIT );

  // see if the animation sequence is switching
  if( newAnimation != lf->animationNumber || !lf->animation )
  {
    if( cg_debugRandom.integer )
      CG_Printf( "newAnimation: %d lf->animationNumber: %d lf->animation: %d\n",
                 newAnimation, lf->animationNumber, lf->animation );

    CG_SetBuildableLerpFrameAnimation( buildable, lf, newAnimation );

    if( !cg_buildables[ buildable ].sounds[ newAnimation ].looped &&
        cg_buildables[ buildable ].sounds[ newAnimation ].enabled )
    {
      if( cg_debugRandom.integer )
        CG_Printf( "Sound for animation %d for a %s\n",
            newAnimation, BG_Buildable( buildable )->humanName );

      trap_S_StartSound( cent->lerpOrigin, cent->currentState.number, CHAN_AUTO,
        cg_buildables[ buildable ].sounds[ newAnimation ].sound );
    }
  }

  if( cg_buildables[ buildable ].sounds[ lf->animationNumber ].looped &&
      cg_buildables[ buildable ].sounds[ lf->animationNumber ].enabled )
    trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
      cg_buildables[ buildable ].sounds[ lf->animationNumber ].sound );
    
  CG_RunLerpFrame( lf, 1.0f );

  // animation ended
  if( lf->frameTime == cg.time )
  {
    cent->buildableAnim = cent->currentState.torsoAnim;
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
  lerpFrame_t *lf = &cent->lerpFrame;

  //if no animation is set default to idle anim
  if( cent->buildableAnim == BANIM_NONE )
  {
    cent->buildableAnim = es->torsoAnim;
    cent->buildableIdleAnim = qtrue;
  }

  //display the first frame of the construction anim if not yet spawned
  if( !( es->eFlags & EF_B_SPAWNED ) )
  {
    animation_t *anim = &cg_buildables[ es->modelindex ].animations[ BANIM_CONSTRUCT1 ];

    //so that when animation starts for real it has sensible numbers
    cent->lerpFrame.oldFrameTime =
      cent->lerpFrame.frameTime =
      cent->lerpFrame.animationTime =
      cg.time;

    *old      = cent->lerpFrame.oldFrame = anim->firstFrame;
    *now      = cent->lerpFrame.frame    = anim->firstFrame;
    *backLerp = cent->lerpFrame.backlerp = 0.0f;

    //ensure that an animation is triggered once the buildable has spawned
    cent->oldBuildableAnim = BANIM_NONE;
  }
  else
  {
    if( ( cent->oldBuildableAnim ^ es->legsAnim ) & ANIM_TOGGLEBIT )
    {
      if( cg_debugAnim.integer )
        CG_Printf( "%d->%d l:%d t:%d %s(%d)\n",
                   cent->oldBuildableAnim, cent->buildableAnim,
                   es->legsAnim, es->torsoAnim,
                   BG_Buildable( es->modelindex )->humanName, es->number );

      if( cent->buildableAnim == es->torsoAnim || es->legsAnim & ANIM_FORCEBIT )
      {
        cent->buildableAnim = cent->oldBuildableAnim = es->legsAnim;
        cent->buildableIdleAnim = qfalse;
      }
      else
      {
        cent->buildableAnim = cent->oldBuildableAnim = es->torsoAnim;
        cent->buildableIdleAnim = qtrue;
      }
    }
    else if( cent->buildableIdleAnim == qtrue &&
             cent->buildableAnim != es->torsoAnim )
    {
      cent->buildableAnim = es->torsoAnim;
    }

    CG_RunBuildableLerpFrame( cent );

    *old      = cent->lerpFrame.oldFrame;
    *now      = cent->lerpFrame.frame;
    *backLerp = cent->lerpFrame.backlerp;
  }
  if( cg_buildables[BG_Buildable( cent->currentState.modelindex )->number].md5 )
  {
        // blend old and current animation
	if(cg_animBlend.value <= 0.0f)
	{
		lf->blendlerp = 0.0f;
	}

	if((lf->blendlerp > 0.0f) && (cg.time > lf->blendtime))
	{
#if 0
		// linear blending
		lf->blendlerp -= 0.025f;
#else
		// exp blending
		lf->blendlerp -= lf->blendlerp / cg_animBlend.value;
#endif
		if(lf->blendlerp <= 0.0f)
		{
			lf->blendlerp = 0.0f;
		}

		if(lf->blendlerp >= 1.0f)
		{
			lf->blendlerp = 1.0f;
		}

		lf->blendtime = cg.time + 10;
	}
	if( lf->animation && lf->animation->handle )
	{
	  if(!trap_R_BuildSkeleton(&lf->skeleton, lf->animation->handle, lf->oldFrame, lf->frame, 1.0 - lf->backlerp, lf->animation->clearOrigin))
	  {
		  CG_Printf("CG_RunBuildableLerpFrame: Can't build lf->skeleton\n");
	  }
	  if( lf->oldSkeleton.type != SK_INVALID )
	  {
	    // lerp between old and new animation if possible
	    if(lf->blendlerp > 0.0f)
	    {
		    if(!trap_R_BlendSkeleton(&lf->skeleton, &lf->oldSkeleton, lf->blendlerp))
		    {
			    CG_Printf("CG_RunBuildableLerpFrame: Can't blend lf->skeleton\n");
			    return;
		    }
	    }
	  }
	}
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
  vec3_t  forward, start, end;
  trace_t tr, box_tr;
  float mag, fraction;

  AngleVectors( angles, forward, NULL, NULL );
  VectorCopy( normal, outAxis[ 2 ] );
  ProjectPointOnPlane( outAxis[ 0 ], forward, outAxis[ 2 ] );

  if( !VectorNormalize( outAxis[ 0 ] ) )
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
  VectorMA( inOrigin, 1.0f, normal, start );

  // Take both capsule and box traces. If the capsule trace does not differ
  //  significantly from the box trace use it. This may cause buildables to be
  //  positioned *inside* the surface on which it is placed. This is intentional

  CG_CapTrace( &tr, start, mins, maxs, end, skipNumber,
               CONTENTS_SOLID | CONTENTS_PLAYERCLIP );

  CG_Trace( &box_tr, start, mins, maxs, end, skipNumber,
            CONTENTS_SOLID | CONTENTS_PLAYERCLIP );

  mag = Distance( tr.endpos, box_tr.endpos );

  fraction = tr.fraction;

  // this is either too far off of the bbox to be useful for gameplay purposes
  //  or the model is positioned in thin air anyways.
  if( mag > 15.0f || tr.fraction == 1.0f )
    fraction = box_tr.fraction; 

  VectorMA( inOrigin, fraction * -TRACE_DEPTH, normal, outOrigin );
}

/*
================
CG_GhostBuildableRangeMarker
================
*/
static void CG_GhostBuildableRangeMarker( buildable_t buildable, const vec3_t origin, const vec3_t normal )
{
  qboolean drawS, drawI, drawF;
  float so, lo, th;
  int rmType;
  float range;
  vec3_t rgb;

  if( CG_GetRangeMarkerPreferences( &drawS, &drawI, &drawF, &so, &lo, &th ) &&
      CG_GetBuildableRangeMarkerProperties( buildable, &rmType, &range, rgb ) )
  {
    vec3_t localOrigin, angles;

    if( buildable == BA_A_HIVE || buildable == BA_H_TESLAGEN )
      VectorMA( origin, BG_BuildableConfig( buildable )->maxs[ 2 ], normal, localOrigin );
    else
      VectorCopy( origin, localOrigin );

    if( rmType > 0 )
      vectoangles( normal, angles );

    CG_DrawRangeMarker( rmType, localOrigin, ( rmType > 0 ? angles : NULL ),
                        range, drawS, drawI, drawF, rgb, so, lo, th );
  }
}

/*
==================
CG_GhostBuildable
==================
*/
void CG_GhostBuildable( buildable_t buildable )
{
  refEntity_t     ent;
  playerState_t   *ps;
  vec3_t          angles, entity_origin;
  vec3_t          mins, maxs;
  trace_t         tr;
  float           scale;

  ps = &cg.predictedPlayerState;

  memset( &ent, 0, sizeof( ent ) );

  BG_BuildableBoundingBox( buildable, mins, maxs );

  BG_PositionBuildableRelativeToPlayer( ps, mins, maxs, CG_Trace, entity_origin, angles, &tr );

  if( cg_rangeMarkerForBlueprint.integer && tr.entityNum != ENTITYNUM_NONE )
    CG_GhostBuildableRangeMarker( buildable, entity_origin, tr.plane.normal );

  CG_PositionAndOrientateBuildable( ps->viewangles, entity_origin, tr.plane.normal, ps->clientNum,
                                    mins, maxs, ent.axis, ent.origin );

  //offset on the Z axis if required
  VectorMA( ent.origin, BG_BuildableConfig( buildable )->zOffset, tr.plane.normal, ent.origin );

  VectorCopy( ent.origin, ent.lightingOrigin );
  VectorCopy( ent.origin, ent.oldorigin ); // don't positionally lerp at all

  ent.hModel = cg_buildables[ buildable ].models[ 0 ];

  if( ps->stats[ STAT_BUILDABLE ] & SB_VALID_TOGGLEBIT )
    ent.customShader = cgs.media.greenBuildShader;
  else
    ent.customShader = cgs.media.redBuildShader;

  //rescale the model
  scale = BG_BuildableConfig( buildable )->modelScale;
  if( cg_buildables[ buildable ].md5 )
  {
    vec3_t Scale = { scale, scale, scale };
    trap_R_BuildSkeleton( &ent.skeleton, cg_buildables[buildable].animations[ BANIM_IDLE1 ].handle, 0, 0, 0, qfalse );
    CG_TransformSkeleton( &ent.skeleton, NULL );
  }
  if( scale != 1.0f )
  {
    VectorScale( ent.axis[ 0 ], scale, ent.axis[ 0 ] );
    VectorScale( ent.axis[ 1 ], scale, ent.axis[ 1 ] );
    VectorScale( ent.axis[ 2 ], scale, ent.axis[ 2 ] );

    ent.nonNormalizedAxes = qtrue;
  }
  else
    ent.nonNormalizedAxes = qfalse;

  // add to refresh list
  trap_R_AddRefEntityToScene( &ent );
}

/*
==================
CG_BuildableParticleEffects
==================
*/
static void CG_BuildableParticleEffects( centity_t *cent )
{
  entityState_t   *es = &cent->currentState;
  team_t          team = BG_Buildable( es->modelindex )->team;
  int             health = es->generic1;
  float           healthFrac = (float)health / BG_Buildable( es->modelindex )->health;

  if( !( es->eFlags & EF_B_SPAWNED ) )
    return;

  if( team == TEAM_HUMANS )
  {
    if( healthFrac < 0.33f && !CG_IsParticleSystemValid( &cent->buildablePS ) )
    {
      cent->buildablePS = CG_SpawnNewParticleSystem( cgs.media.humanBuildableDamagedPS );

      if( CG_IsParticleSystemValid( &cent->buildablePS ) )
      {
        CG_SetAttachmentCent( &cent->buildablePS->attachment, cent );
        CG_AttachToCent( &cent->buildablePS->attachment );
      }
    }
    else if( healthFrac >= 0.33f && CG_IsParticleSystemValid( &cent->buildablePS ) )
      CG_DestroyParticleSystem( &cent->buildablePS );
  }
  else if( team == TEAM_ALIENS )
  {
    if( healthFrac < 0.33f && !CG_IsParticleSystemValid( &cent->buildablePS ) )
    {
      cent->buildablePS = CG_SpawnNewParticleSystem( cgs.media.alienBuildableDamagedPS );

      if( CG_IsParticleSystemValid( &cent->buildablePS ) )
      {
        CG_SetAttachmentCent( &cent->buildablePS->attachment, cent );
        CG_SetParticleSystemNormal( cent->buildablePS, es->origin2 );
        CG_AttachToCent( &cent->buildablePS->attachment );
      }
    }
    else if( healthFrac >= 0.33f && CG_IsParticleSystemValid( &cent->buildablePS ) )
      CG_DestroyParticleSystem( &cent->buildablePS );
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
  if( !handle )
    return;
  while( 1 )
  {
    if( !trap_Parse_ReadToken( handle, &token ) )
      break;
    if( !Q_stricmp( token.string, "frameShader" ) )
    {
      if( PC_String_Parse( handle, &s ) )
        bs->frameShader = trap_R_RegisterShader( s );
      continue;
    }
    else if( !Q_stricmp( token.string, "overlayShader" ) )
    {
      if( PC_String_Parse( handle, &s ) )
        bs->overlayShader = trap_R_RegisterShader( s );
      continue;
    }
    else if( !Q_stricmp( token.string, "noPowerShader" ) )
    {
      if( PC_String_Parse( handle, &s ) )
        bs->noPowerShader = trap_R_RegisterShader( s );
      continue;
    }
    else if( !Q_stricmp( token.string, "markedShader" ) )
    {
      if( PC_String_Parse( handle, &s ) )
        bs->markedShader = trap_R_RegisterShader( s );
      continue;
    }
    else if( !Q_stricmp( token.string, "healthSevereColor" ) )
    {
      if( PC_Color_Parse( handle, &c ) )
        Vector4Copy( c, bs->healthSevereColor );
      continue;
    }
    else if( !Q_stricmp( token.string, "healthHighColor" ) )
    {
      if( PC_Color_Parse( handle, &c ) )
        Vector4Copy( c, bs->healthHighColor );
      continue;
    }
    else if( !Q_stricmp( token.string, "healthElevatedColor" ) )
    {
      if( PC_Color_Parse( handle, &c ) )
        Vector4Copy( c, bs->healthElevatedColor );
      continue;
    }
    else if( !Q_stricmp( token.string, "healthGuardedColor" ) )
    {
      if( PC_Color_Parse( handle, &c ) )
        Vector4Copy( c, bs->healthGuardedColor );
      continue;
    }
    else if( !Q_stricmp( token.string, "healthLowColor" ) )
    {
      if( PC_Color_Parse( handle, &c ) )
        Vector4Copy( c, bs->healthLowColor );
      continue;
    }
    else if( !Q_stricmp( token.string, "foreColor" ) )
    {
      if( PC_Color_Parse( handle, &c ) )
        Vector4Copy( c, bs->foreColor );
      continue;
    }
    else if( !Q_stricmp( token.string, "backColor" ) )
    {
      if( PC_Color_Parse( handle, &c ) )
        Vector4Copy( c, bs->backColor );
      continue;
    }
    else if( !Q_stricmp( token.string, "frameHeight" ) )
    {
      if( PC_Int_Parse( handle, &i ) )
        bs->frameHeight = i;
      continue;
    }
    else if( !Q_stricmp( token.string, "frameWidth" ) )
    {
      if( PC_Int_Parse( handle, &i ) )
        bs->frameWidth = i;
      continue;
    }
    else if( !Q_stricmp( token.string, "healthPadding" ) )
    {
      if( PC_Int_Parse( handle, &i ) )
        bs->healthPadding = i;
      continue;
    }
    else if( !Q_stricmp( token.string, "overlayHeight" ) )
    {
      if( PC_Int_Parse( handle, &i ) )
        bs->overlayHeight = i;
      continue;
    }
    else if( !Q_stricmp( token.string, "overlayWidth" ) )
    {
      if( PC_Int_Parse( handle, &i ) )
        bs->overlayWidth = i;
      continue;
    }
    else if( !Q_stricmp( token.string, "verticalMargin" ) )
    {
      if( PC_Float_Parse( handle, &f ) )
        bs->verticalMargin = f;
      continue;
    }
    else if( !Q_stricmp( token.string, "horizontalMargin" ) )
    {
      if( PC_Float_Parse( handle, &f ) )
        bs->horizontalMargin = f;
      continue;
    }
    else
    {
      Com_Printf("CG_BuildableStatusParse: unknown token %s in %s\n",
        token.string, filename );
      bs->loaded = qfalse;
      trap_Parse_FreeSource( handle );
      return;
    }
  }
  bs->loaded = qtrue;
  trap_Parse_FreeSource( handle );
}

#define STATUS_FADE_TIME      200
#define STATUS_MAX_VIEW_DIST  900.0f
#define STATUS_PEEK_DIST      20
/*
==================
CG_BuildableStatusDisplay
==================
*/
static void CG_BuildableStatusDisplay( centity_t *cent )
{
  entityState_t   *es = &cent->currentState;
  vec3_t          origin;
  float           healthScale;
  int             health;
  float           x, y;
  vec4_t          color;
  qboolean        powered, marked;
  trace_t         tr;
  float           d;
  buildStat_t     *bs;
  int             i, j;
  int             entNum;
  vec3_t          trOrigin;
  vec3_t          right;
  qboolean        visible = qfalse;
  vec3_t          mins, maxs;
  entityState_t   *hit;
  int             anim;

  if( BG_Buildable( es->modelindex )->team == TEAM_ALIENS )
    bs = &cgs.alienBuildStat;
  else
    bs = &cgs.humanBuildStat;

  if( !bs->loaded )
    return;

  d = Distance( cent->lerpOrigin, cg.refdef.vieworg );
  if( d > STATUS_MAX_VIEW_DIST )
    return;

  Vector4Copy( bs->foreColor, color );

  // trace for center point
  BG_BuildableBoundingBox( es->modelindex, mins, maxs );

  // hack for shrunken barricades
  anim = es->torsoAnim & ~( ANIM_FORCEBIT | ANIM_TOGGLEBIT );
  if( es->modelindex == BA_A_BARRICADE &&
      ( anim == BANIM_DESTROYED || !( es->eFlags & EF_B_SPAWNED ) ) )
    maxs[ 2 ] = (int)( maxs[ 2 ] * BARRICADE_SHRINKPROP );

  VectorCopy( cent->lerpOrigin, origin );

  // center point
  origin[ 2 ] += mins[ 2 ];
  origin[ 2 ] += ( abs( mins[ 2 ] ) + abs( maxs[ 2 ] ) ) / 2;

  entNum = cg.predictedPlayerState.clientNum;

  // if first try fails, step left, step right
  for( j = 0; j < 3; j++ )
  {
    VectorCopy( cg.refdef.vieworg, trOrigin );
    switch( j )
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
    for( i = 0; i < 3; i++ )
    {
      CG_Trace( &tr, trOrigin, NULL, NULL, origin, entNum, MASK_SHOT );
      if( tr.entityNum == cent->currentState.number )
      {
        visible = qtrue;
        break;
      }

      if( tr.entityNum == ENTITYNUM_WORLD )
        break;

      hit  = &cg_entities[ tr.entityNum ].currentState;

      if( tr.entityNum < MAX_CLIENTS || ( hit->eType == ET_BUILDABLE &&
          ( !( es->eFlags & EF_B_SPAWNED ) ||
            BG_Buildable( hit->modelindex )->transparentTest ) ) )
      {
        entNum = tr.entityNum;
        VectorCopy( tr.endpos, trOrigin );
      }
      else
        break;
    }
  }
  // hack to make the kit obscure view
  if( cg_drawGun.integer && visible &&
      cg.predictedPlayerState.stats[ STAT_TEAM ] == TEAM_HUMANS &&
      CG_WorldToScreen( origin, &x, &y ) )
  {
    if( x > 450 && y > 290 )
      visible = qfalse;
  }

  if( !visible && cent->buildableStatus.visible )
  {
    cent->buildableStatus.visible   = qfalse;
    cent->buildableStatus.lastTime  = cg.time;
  }
  else if( visible && !cent->buildableStatus.visible )
  {
    cent->buildableStatus.visible   = qtrue;
    cent->buildableStatus.lastTime  = cg.time;
  }

  // Fade up
  if( cent->buildableStatus.visible )
  {
    if( cent->buildableStatus.lastTime + STATUS_FADE_TIME > cg.time )
      color[ 3 ] = (float)( cg.time - cent->buildableStatus.lastTime ) / STATUS_FADE_TIME;
  }

  // Fade down
  if( !cent->buildableStatus.visible )
  {
    if( cent->buildableStatus.lastTime + STATUS_FADE_TIME > cg.time )
      color[ 3 ] = 1.0f - (float)( cg.time - cent->buildableStatus.lastTime ) / STATUS_FADE_TIME;
    else
      return;
  }

  health = es->generic1;
  healthScale = (float)health / BG_Buildable( es->modelindex )->health;

  if( health > 0 && healthScale < 0.01f )
    healthScale = 0.01f;
  else if( healthScale < 0.0f )
    healthScale = 0.0f;
  else if( healthScale > 1.0f )
    healthScale = 1.0f;

  if( CG_WorldToScreen( origin, &x, &y ) )
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

    if( bs->frameShader )
    {
      Vector4Copy( bs->backColor, frameColor );
      frameColor[ 3 ] = color[ 3 ];
      trap_R_SetColor( frameColor );
      CG_DrawPic( picX, picY, picW, picH, bs->frameShader );
      trap_R_SetColor( NULL );
    }

    if( health > 0 )
    {
      float hX, hY, hW, hH;
      vec4_t healthColor;

      hX = picX + ( bs->healthPadding * scale );
      hY = picY + ( bs->healthPadding * scale );
      hH = picH - ( bs->healthPadding * 2.0f * scale );
      hW = picW * healthScale - ( bs->healthPadding * 2.0f * scale );

      if( healthScale == 1.0f )
        Vector4Copy( bs->healthLowColor, healthColor );
      else if( healthScale >= 0.75f )
        Vector4Copy( bs->healthGuardedColor, healthColor );
      else if( healthScale >= 0.50f )
        Vector4Copy( bs->healthElevatedColor, healthColor );
      else if( healthScale >= 0.25f )
        Vector4Copy( bs->healthHighColor, healthColor );
      else
        Vector4Copy( bs->healthSevereColor, healthColor );

      healthColor[ 3 ] = color[ 3 ];
      trap_R_SetColor( healthColor );

      CG_DrawPic( hX, hY, hW, hH, cgs.media.whiteShader );
      trap_R_SetColor( NULL );
    }

    if( bs->overlayShader )
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
    if( !powered )
    {
      float pX;

      pX = picX + ( subH * bs->horizontalMargin );
      CG_DrawPic( pX, subY, subH, subH, bs->noPowerShader );
    }

    if( marked )
    {
      float mX;

      mX = picX + picW - ( subH * bs->horizontalMargin ) - subH;
      CG_DrawPic( mX, subY, subH, subH, bs->markedShader );
    }

    {
      float nX;
      int healthMax;
      int healthPoints;

      healthMax = BG_Buildable( es->modelindex )->health;
      healthPoints = (int)( healthScale * healthMax );
      if( health > 0 && healthPoints < 1 )
        healthPoints = 1;
      nX = picX + ( picW * 0.5f ) - 2.0f - ( ( subH * 4 ) * 0.5f );

      if( healthPoints > 999 )
        nX -= 0.0f;
      else if( healthPoints > 99 )
        nX -= subH * 0.5f;
      else if( healthPoints > 9 )
        nX -= subH * 1.0f;
      else
        nX -= subH * 1.5f;

      CG_DrawField( nX, subY, 4, subH, subH, healthPoints );
    }

    trap_R_SetColor( NULL );
    CG_ClearClipRegion( );
  }
}

/*
==================
CG_SortDistance
==================
*/
static int CG_SortDistance( const void *a, const void *b )
{
  centity_t    *aent, *bent;
  float        adist, bdist;

  aent = &cg_entities[ *(int *)a ];
  bent = &cg_entities[ *(int *)b ];
  adist = Distance( cg.refdef.vieworg, aent->lerpOrigin );
  bdist = Distance( cg.refdef.vieworg, bent->lerpOrigin );
  if( adist > bdist )
    return -1;
  else if( adist < bdist )
    return 1;
  else
    return 0;
}

/*
==================
CG_PlayerIsBuilder
==================
*/
static qboolean CG_PlayerIsBuilder( buildable_t buildable )
{
  switch( cg.predictedPlayerState.weapon )
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

  if( !( ps->stats[ STAT_BUILDABLE ] & SB_VALID_TOGGLEBIT ) )
    return qfalse;

  for( i = 0; i < MAX_MISC; i++ )
  {
    if( ps->misc[ i ] == entityNum )
      return qtrue;
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
  int             i;
  centity_t       *cent;
  entityState_t   *es;
  int             buildableList[ MAX_ENTITIES_IN_SNAPSHOT ];
  int             buildables = 0;

  for( i = 0; i < cg.snap->numEntities; i++ )
  {
    cent  = &cg_entities[ cg.snap->entities[ i ].number ];
    es    = &cent->currentState;

    if( es->eType == ET_BUILDABLE && CG_PlayerIsBuilder( es->modelindex ) )
      buildableList[ buildables++ ] = cg.snap->entities[ i ].number;
  }

  qsort( buildableList, buildables, sizeof( int ), CG_SortDistance );
  for( i = 0; i < buildables; i++ )
    CG_BuildableStatusDisplay( &cg_entities[ buildableList[ i ] ] );
}

#define BUILDABLE_SOUND_PERIOD  500

/*
==================
CG_Buildable
==================
*/
void CG_Buildable( centity_t *cent )
{
  refEntity_t     ent;
  entityState_t   *es = &cent->currentState;
  vec3_t          angles;
  vec3_t          surfNormal, xNormal, mins, maxs;
  vec3_t          refNormal = { 0.0f, 0.0f, 1.0f };
  float           rotAngle;
  team_t          team = BG_Buildable( es->modelindex )->team;
  float           scale;
  int             health;

  //must be before EF_NODRAW check
  if( team == TEAM_ALIENS )
    CG_Creep( cent );

  // if set to invisible, skip
  if( es->eFlags & EF_NODRAW )
  {
    if( CG_IsParticleSystemValid( &cent->buildablePS ) )
      CG_DestroyParticleSystem( &cent->buildablePS );

    return;
  }

  memset ( &ent, 0, sizeof( ent ) );

  VectorCopy( cent->lerpOrigin, ent.origin );
  VectorCopy( cent->lerpOrigin, ent.oldorigin );
  VectorCopy( cent->lerpOrigin, ent.lightingOrigin );

  VectorCopy( es->origin2, surfNormal );

  VectorCopy( es->angles, angles );
  BG_BuildableBoundingBox( es->modelindex, mins, maxs );

  if( es->pos.trType == TR_STATIONARY )
  {
    // Positioning a buildable involves potentially up to two traces, and
    // seeing as buildables rarely move, we cache the results and recalculate
    // only if the buildable moves or changes orientation
    if( VectorCompare( cent->buildableCache.cachedOrigin, cent->lerpOrigin ) &&
        VectorCompare( cent->buildableCache.cachedNormal, surfNormal ) )
    {
      VectorCopy( cent->buildableCache.axis[ 0 ], ent.axis[ 0 ] );
      VectorCopy( cent->buildableCache.axis[ 1 ], ent.axis[ 1 ] );
      VectorCopy( cent->buildableCache.axis[ 2 ], ent.axis[ 2 ] );
      VectorCopy( cent->buildableCache.origin, ent.origin );
    }
    else
    {
      CG_PositionAndOrientateBuildable( angles, ent.origin, surfNormal,
                                        es->number, mins, maxs, ent.axis,
                                        ent.origin );
      VectorCopy( ent.axis[ 0 ], cent->buildableCache.axis[ 0 ] );
      VectorCopy( ent.axis[ 1 ], cent->buildableCache.axis[ 1 ] );
      VectorCopy( ent.axis[ 2 ], cent->buildableCache.axis[ 2 ] );
      VectorCopy( ent.origin, cent->buildableCache.origin );
      VectorCopy( cent->lerpOrigin, cent->buildableCache.cachedOrigin );
      VectorCopy( surfNormal, cent->buildableCache.cachedNormal );
    }
  }

  //offset on the Z axis if required
  VectorMA( ent.origin, BG_BuildableConfig( es->modelindex )->zOffset, surfNormal, ent.origin );

  VectorCopy( ent.origin, ent.oldorigin ); // don't positionally lerp at all
  VectorCopy( ent.origin, ent.lightingOrigin );

  ent.hModel = cg_buildables[ es->modelindex ].models[ 0 ];

  if( !( es->eFlags & EF_B_SPAWNED ) )
  {
    sfxHandle_t prebuildSound = cgs.media.humanBuildablePrebuild;

    if( team == TEAM_HUMANS )
    {
      ent.customShader = cgs.media.humanSpawningShader;
      prebuildSound = cgs.media.humanBuildablePrebuild;
    }
    else if( team == TEAM_ALIENS )
      prebuildSound = cgs.media.alienBuildablePrebuild;

    trap_S_AddLoopingSound( es->number, cent->lerpOrigin, vec3_origin, prebuildSound );
  }

  CG_BuildableAnimation( cent, &ent.oldframe, &ent.frame, &ent.backlerp );

  //rescale the model
  scale = BG_BuildableConfig( es->modelindex )->modelScale;

  if( scale != 1.0f )
  {
    VectorScale( ent.axis[ 0 ], scale, ent.axis[ 0 ] );
    VectorScale( ent.axis[ 1 ], scale, ent.axis[ 1 ] );
    VectorScale( ent.axis[ 2 ], scale, ent.axis[ 2 ] );

    ent.nonNormalizedAxes = qtrue;
  }
  else
    ent.nonNormalizedAxes = qfalse;

  if( CG_PlayerIsBuilder( es->modelindex ) && CG_BuildableRemovalPending( es->number ) )
    ent.customShader = cgs.media.redBuildShader;
  
  if( cg_buildables[ es->modelindex ].md5 )
  {
    vec3_t Scale = { scale, scale, scale };
    memcpy( &ent.skeleton, &cent->lerpFrame.skeleton, sizeof( refSkeleton_t ) );
    CG_TransformSkeleton( &ent.skeleton, Scale );
  }

  //add to refresh list
  trap_R_AddRefEntityToScene( &ent );

  CrossProduct( surfNormal, refNormal, xNormal );
  VectorNormalize( xNormal );
  rotAngle = RAD2DEG( acos( DotProduct( surfNormal, refNormal ) ) );

  //turret barrel bit
  if( cg_buildables[ es->modelindex ].models[ 1 ] )
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
    turretBarrel.frame    = ent.frame;
    turretBarrel.backlerp = ent.backlerp;

    turretBarrel.customShader = ent.customShader;

    if( scale != 1.0f )
    {
      VectorScale( turretBarrel.axis[ 0 ], scale, turretBarrel.axis[ 0 ] );
      VectorScale( turretBarrel.axis[ 1 ], scale, turretBarrel.axis[ 1 ] );
      VectorScale( turretBarrel.axis[ 2 ], scale, turretBarrel.axis[ 2 ] );

      turretBarrel.nonNormalizedAxes = qtrue;
    }
    else
      turretBarrel.nonNormalizedAxes = qfalse;

    if( CG_PlayerIsBuilder( es->modelindex ) && CG_BuildableRemovalPending( es->number ) )
      turretBarrel.customShader = cgs.media.redBuildShader;

    trap_R_AddRefEntityToScene( &turretBarrel );
  }

  //turret barrel bit
  if( cg_buildables[ es->modelindex ].models[ 2 ] )
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
    turretTop.frame    = ent.frame;
    turretTop.backlerp = ent.backlerp;

    turretTop.customShader = ent.customShader;

    if( scale != 1.0f )
    {
      VectorScale( turretTop.axis[ 0 ], scale, turretTop.axis[ 0 ] );
      VectorScale( turretTop.axis[ 1 ], scale, turretTop.axis[ 1 ] );
      VectorScale( turretTop.axis[ 2 ], scale, turretTop.axis[ 2 ] );

      turretTop.nonNormalizedAxes = qtrue;
    }
    else
      turretTop.nonNormalizedAxes = qfalse;

    if( CG_PlayerIsBuilder( es->modelindex ) && CG_BuildableRemovalPending( es->number ) )
      turretTop.customShader = cgs.media.redBuildShader;

    trap_R_AddRefEntityToScene( &turretTop );
  }

  //weapon effects for turrets
  if( es->eFlags & EF_FIRING )
  {
    weaponInfo_t  *weapon = &cg_weapons[ es->weapon ];

    if( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME ||
        BG_Buildable( es->modelindex )->turretProjType == WP_TESLAGEN )
    {
      if( weapon->wim[ WPM_PRIMARY ].flashDlightColor[ 0 ] ||
          weapon->wim[ WPM_PRIMARY ].flashDlightColor[ 1 ] ||
          weapon->wim[ WPM_PRIMARY ].flashDlightColor[ 2 ] )
      {
        trap_R_AddLightToScene( cent->lerpOrigin, rand( ) & 20, 300 + ( rand( ) & 31 ),
            weapon->wim[ WPM_PRIMARY ].flashDlightColor[ 0 ],
            weapon->wim[ WPM_PRIMARY ].flashDlightColor[ 1 ],
            weapon->wim[ WPM_PRIMARY ].flashDlightColor[ 2 ], 0, 0 );
      }
    }

    if( weapon->wim[ WPM_PRIMARY ].firingSound )
    {
      trap_S_AddLoopingSound( es->number, cent->lerpOrigin, vec3_origin,
          weapon->wim[ WPM_PRIMARY ].firingSound );
    }
    else if( weapon->readySound )
      trap_S_AddLoopingSound( es->number, cent->lerpOrigin, vec3_origin, weapon->readySound );
  }

  health = es->generic1;

  if( health < cent->lastBuildableHealth &&
      ( es->eFlags & EF_B_SPAWNED ) )
  {
    if( cent->lastBuildableDamageSoundTime + BUILDABLE_SOUND_PERIOD < cg.time )
    {
      if( team == TEAM_HUMANS )
      {
        int i = rand( ) % 4;
        trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.humanBuildableDamage[ i ] );
      }
      else if( team == TEAM_ALIENS )
        trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.alienBuildableDamage );

      cent->lastBuildableDamageSoundTime = cg.time;
    }
  }

  cent->lastBuildableHealth = health;

  //smoke etc for damaged buildables
  CG_BuildableParticleEffects( cent );
}
