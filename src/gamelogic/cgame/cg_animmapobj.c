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

/*
===============
CG_DoorAnimation
===============
*/
static void CG_DoorAnimation( centity_t *cent, int *old, int *now, float *backLerp )
{
	CG_RunLerpFrame( &cent->lerpFrame, 1.0f );

	*old = cent->lerpFrame.oldFrame;
	*now = cent->lerpFrame.frame;
	*backLerp = cent->lerpFrame.backlerp;
}

/*
===============
CG_ModelDoor
===============
*/
void CG_ModelDoor( centity_t *cent )
{
	refEntity_t   ent;
	entityState_t *es;
	animation_t   anim;
	lerpFrame_t   *lf = &cent->lerpFrame;

	es = &cent->currentState;

	if ( !es->modelindex )
	{
		return;
	}

	//create the render entity
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );
	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	//add the door model
	ent.skinNum = 0;
	ent.hModel = cgs.gameModels[ es->modelindex ];

	//scale the door
	VectorScale( ent.axis[ 0 ], es->origin2[ 0 ], ent.axis[ 0 ] );
	VectorScale( ent.axis[ 1 ], es->origin2[ 1 ], ent.axis[ 1 ] );
	VectorScale( ent.axis[ 2 ], es->origin2[ 2 ], ent.axis[ 2 ] );
	ent.nonNormalizedAxes = qtrue;

	//setup animation
	anim.firstFrame = es->misc;
	anim.numFrames = es->weapon;
	anim.reversed = !es->legsAnim;
	anim.flipflop = qfalse;
	anim.loopFrames = 0;
	anim.frameLerp = 1000 / es->torsoAnim;
	anim.initialLerp = 1000 / es->torsoAnim;

	//door changed state
	if ( es->legsAnim != cent->doorState )
	{
		lf->animationTime = lf->frameTime + anim.initialLerp;
		cent->doorState = es->legsAnim;
	}

	lf->animation = &anim;

	//run animation
	CG_DoorAnimation( cent, &ent.oldframe, &ent.frame, &ent.backlerp );

	trap_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_AMOAnimation
===============
*/
static void CG_AMOAnimation( centity_t *cent, int *old, int *now, float *backLerp )
{
	if ( !( cent->currentState.eFlags & EF_MOVER_STOP ) || cent->animPlaying )
	{
		int delta = cg.time - cent->miscTime;

		//hack to prevent "pausing" mucking up the lerping
		if ( delta > 900 )
		{
			cent->lerpFrame.oldFrameTime += delta;
			cent->lerpFrame.frameTime += delta;
		}

		CG_RunLerpFrame( &cent->lerpFrame, 1.0f );
		cent->miscTime = cg.time;
	}

	*old = cent->lerpFrame.oldFrame;
	*now = cent->lerpFrame.frame;
	*backLerp = cent->lerpFrame.backlerp;
}

/*
==================
CG_animMapObj
==================
*/
void CG_AnimMapObj( centity_t *cent )
{
	refEntity_t   ent;
	entityState_t *es;
	float         scale;
	animation_t   anim;

	es = &cent->currentState;

	// if set to invisible, skip
	if ( !es->modelindex || ( es->eFlags & EF_NODRAW ) )
	{
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( es->angles, cent->lerpAngles );
	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.hModel = cgs.gameModels[ es->modelindex ];

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	ent.nonNormalizedAxes = qfalse;

	//scale the model
	if ( es->angles2[ 0 ] )
	{
		scale = es->angles2[ 0 ];
		VectorScale( ent.axis[ 0 ], scale, ent.axis[ 0 ] );
		VectorScale( ent.axis[ 1 ], scale, ent.axis[ 1 ] );
		VectorScale( ent.axis[ 2 ], scale, ent.axis[ 2 ] );
		ent.nonNormalizedAxes = qtrue;
	}

	//setup animation
	anim.firstFrame = es->misc;
	anim.numFrames = es->weapon;
	anim.reversed = qfalse;
	anim.flipflop = qfalse;

	// if numFrames is negative the animation is reversed
	if ( anim.numFrames < 0 )
	{
		anim.numFrames = -anim.numFrames;
		anim.reversed = qtrue;
	}

	anim.loopFrames = es->torsoAnim;

	if ( !es->legsAnim )
	{
		anim.frameLerp = 1000;
		anim.initialLerp = 1000;
	}
	else
	{
		anim.frameLerp = 1000 / es->legsAnim;
		anim.initialLerp = 1000 / es->legsAnim;
	}

	cent->lerpFrame.animation = &anim;

	if ( !anim.loopFrames )
	{
		// add one frame to allow the animation to play the last frame
		// add another to get it to stop playing at the first frame
		anim.numFrames += 2;

		if ( !cent->animInit )
		{
			cent->animInit = qtrue;
			cent->animPlaying = !( cent->currentState.eFlags & EF_MOVER_STOP );
		}
		else
		{
			if ( cent->animLastState !=
			     !( cent->currentState.eFlags & EF_MOVER_STOP ) )
			{
				cent->animPlaying = qtrue;
				cent->lerpFrame.animationTime = cg.time;
				cent->lerpFrame.frameTime = cg.time;
			}
		}

		cent->animLastState = !( cent->currentState.eFlags & EF_MOVER_STOP );
	}

	//run animation
	CG_AMOAnimation( cent, &ent.oldframe, &ent.frame, &ent.backlerp );

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}
