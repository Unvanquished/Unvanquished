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

#ifndef CG_SEGMENTED_SKELETON_H
#define CG_SEGMENTED_SKELETON_H

#include "cg_local.h"
#include "cg_skeleton_modifier.h"

class HumanSkeletonRotations : public SkeletonModifier
{
public:
	virtual bool ParseConfiguration( clientInfo_t* ci, const char* token, const char** data_p ) override;
	virtual void Apply( const SkeletonModifierContext& ctx, refSkeleton_t* skeleton ) override;

private:
	int torsoControlBone = -1;
	int leftShoulderBone = -1;
	int rightShoulderBone = -1;
};

class BsuitSkeletonRotations : public SkeletonModifier
{
public:
	virtual bool ParseConfiguration( clientInfo_t* ci, const char* token, const char** data_p ) override;
	virtual void Apply( const SkeletonModifierContext& ctx, refSkeleton_t* skeleton ) override;

private:
	int torsoControlBone = -1;
	int leftShoulderBone = -1;
	int rightShoulderBone = -1;
};


class SegmentedSkeletonCombiner : public SkeletonModifier
{
public:
	virtual bool ParseConfiguration(clientInfo_t* ci, const char* token, const char** data_p) override;
	virtual void Apply(const SkeletonModifierContext& ctx, refSkeleton_t* skeleton) override;

private:
	std::vector<int> legBoneIndices;
};

#endif  // CG_SEGMENTED_SKELETON_H
