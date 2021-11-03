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

#include "cg_local.h"

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
void CG_RunLerpFrame( lerpFrame_t *lf, float scale )
{
	// debugging tool to get no animations
	if ( !cg_animSpeed.Get() )
	{
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}
	lf->animationEnded = false;

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
			float frameLength = anim->frameLerp / scale;

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

void CG_RunMD5LerpFrame( lerpFrame_t *lf, float scale, bool animChanged )
{
	if (animChanged)
	{
		lf->frame = lf->oldFrame = 0;
		lf->frameTime = lf->oldFrameTime = cg.time;
	}
	CG_RunLerpFrame(lf, scale);
}

/*
===============
CG_BlendLerpFrame

Sets lf->blendlerp and lf->blendtime
===============
*/
void CG_BlendLerpFrame( lerpFrame_t *lf )
{
	if ( cg_animBlend.Get() <= 0.0f )
	{
		lf->blendlerp = 0.0f;
		return;
	}

	if ( ( lf->blendlerp > 0.0f ) && ( cg.time > lf->blendtime ) )
	{
		//exp blending
		lf->blendlerp -= lf->blendlerp / cg_animBlend.Get();

		if ( lf->blendlerp <= 0.0f )
		{
			lf->blendlerp = 0.0f;
		}

		if ( lf->blendlerp >= 1.0f )
		{
			lf->blendlerp = 1.0f;
		}

		lf->blendtime = cg.time + 10;

		debug_anim_blend = lf->blendlerp;
	}
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
	if ( lf->blendlerp >= 0.0f )
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
