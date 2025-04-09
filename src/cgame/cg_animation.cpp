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
#include "cg_local.h"

// This shouldn't be much longer than 100 because then short animations (e.g. barricade raise/lower)
// still won't be fully blended in before they finish.
// FIXME: detect short animations and accelerate convergence of blend fraction to make
// sure it's close to 0 by the time the anim finishes?
static Cvar::Cvar<float> cg_animBlendHalfLife(
	"cg_animBlendHalfLife", "inter-animation transition time (higher:slower, 0:instant)", Cvar::NONE, 100);

/*
===============
CG_AnimNumber

Gives the raw animation number removing the togglebit and forcebit if they are present.
===============
*/
int CG_AnimNumber( int anim )
{
	return anim & ~( ANIM_TOGGLEBIT | ANIM_FORCEBIT );
}

/*
===============
CG_RunLerpFrame

cg.time should be between oldFrameTime and frameTime after exit
===============
*/
void CG_RunLerpFrame( lerpFrame_t *lf )
{
	// debugging tool to get no animations
	if ( !cg_animSpeed.Get() )
	{
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}
	lf->animationEnded = false;

	/* Don't process animations with frameLerp 0
	or some computation will divide by zero.
	This happens with md3 first-person weapon models
	that doesn't have any animation. */
	if ( !lf->animation->frameLerp )
	{
		return;
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time > lf->frameTime)
	{
		if (cg.time < lf->animationTime)
		{
			// initial lerp
			lf->frameTime = lf->oldFrameTime = lf->animationTime;
			lf->oldFrame = lf->frame;
		}
		else
		{
			lf->oldFrame = lf->frame;
			lf->oldFrameTime = lf->frameTime;

			animation_t *anim = lf->animation;
			int numFrames = anim->numFrames;

			/* Previously the code was dividing frameLength by scale
			But scale was always eqal to 1.0f so we did not it */
			float frameLength = anim->frameLerp;

			int relativeFrame = ceil((cg.time - lf->animationTime) / frameLength);
			if (relativeFrame >= numFrames)
			{
				bool looping = !!anim->loopFrames;
				if (looping)
				{
					ASSERT_EQ(anim->loopFrames, numFrames);
					lf->animationTime += relativeFrame / numFrames * (numFrames * frameLength);
					relativeFrame %= numFrames;
					lf->frameTime = lf->animationTime + relativeFrame * frameLength;
				}
				else
				{
					relativeFrame = numFrames - 1;
					// the animation is stuck at the end, so it
					// can immediately transition to another sequence
					lf->frameTime = cg.time;
					lf->animationEnded = true;
				}
			}
			else
			{
				lf->frameTime = lf->animationTime + relativeFrame * frameLength;
			}
			if (anim->reversed)
			{
				lf->frame = anim->firstFrame + numFrames - 1 - relativeFrame;
			}
			else
			{
				lf->frame = anim->firstFrame + relativeFrame;
			}
		}
	}

	if (lf->frameTime > lf->oldFrameTime)
	{
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
	else
	{
		lf->backlerp = 0;
	}
}

void CG_RunMD5LerpFrame( lerpFrame_t *lf, bool animChanged )
{
	if (animChanged)
	{
		lf->frame = lf->oldFrame = 0;
		lf->frameTime = lf->oldFrameTime = cg.time;
	}
	CG_RunLerpFrame( lf );
}

/*
===============
CG_BlendLerpFrame

Sets lf->blendlerp
===============
*/
void CG_BlendLerpFrame( lerpFrame_t *lf )
{
	if ( cg_animBlendHalfLife.Get() <= 0.0f )
	{
		lf->blendlerp = 0.0f;
		return;
	}

	if ( lf->blendlerp < 1.0e-6f )
	{
		// Truncate to 0 to avoid wasting CPU/IPC on blending operations for
		// invisibly small differences.
		// 1e-6 is intended as a conservative value that would definitely not be visibly different;
		// it would probably be fine to use 1e-4 and be slightly more efficient.
		lf->blendlerp = 0.0f;
		return;
	}

	//exp blending
	lf->blendlerp *= exp2f( -cg.frametime / cg_animBlendHalfLife.Get() );
	lf->blendlerp = Math::Clamp( lf->blendlerp, 0.0f, 1.0f );
}

/*
===============
CG_BuildAnimSkeleton

Builds the skeleton for the current animation
Also blends between the old and new skeletons if necessary
===============
*/
void CG_BuildAnimSkeleton( const lerpFrame_t *lf, refSkeleton_t *newSkeleton, const refSkeleton_t *oldSkeleton )
{
	if( !lf->animation || !lf->animation->handle )
	{
		// initialize skeleton if animation handle is invalid
		int i;

		newSkeleton->type = refSkeletonType_t::SK_ABSOLUTE;
		newSkeleton->numBones = MAX_BONES;
		for( i = 0; i < MAX_BONES; i++ ) {
			newSkeleton->bones[i].parentIndex = -1;
			TransInit(&newSkeleton->bones[i].t);
		}

		return;
	}

	if ( !trap_R_BuildSkeleton( newSkeleton, lf->animation->handle, lf->oldFrame, lf->frame, 1 - lf->backlerp, lf->animation->clearOrigin ) )
	{
		Log::Warn( "CG_BuildAnimSkeleton: Can't build skeleton" );
	}

	// lerp between old and new animation if possible
	if ( lf->blendlerp > 0.0f )
	{
		if ( newSkeleton->type != refSkeletonType_t::SK_INVALID && oldSkeleton->type != refSkeletonType_t::SK_INVALID && newSkeleton->numBones == oldSkeleton->numBones )
		{
			if ( !trap_R_BlendSkeleton( newSkeleton, oldSkeleton, lf->blendlerp ) )
			{
				Log::Warn( "CG_BuildAnimSkeleton: Can't blend skeletons" );
				return;
			}
		}
	}
}
