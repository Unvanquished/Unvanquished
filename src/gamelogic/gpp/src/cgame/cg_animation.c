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
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
void CG_RunLerpFrame( lerpFrame_t *lf, float scale )
{
	int         f, numFrames;
	animation_t *anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 )
	{
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime )
	{
		lf->oldFrame     = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim             = lf->animation;

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

		f         = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f        *= scale;
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
				f             = numFrames - 1;
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
				CG_Printf( "Clamp lf->frameTime\n" );
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
		lf->backlerp = 1.0 - ( float )( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}
