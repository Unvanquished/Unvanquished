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

#ifndef CG_ANIMDELTA_H
#define CG_ANIMDELTA_H

#include <vector>
#include <unordered_map>

#include "cg_local.h"
#include "cg_skeleton_modifier.h"

class AnimDelta : public SkeletonModifier
{
public:
	virtual bool ParseConfiguration( clientInfo_t* ci, const char* token, const char** data_p ) override;
	virtual bool LoadData( clientInfo_t* ci ) override;
	virtual void Apply( const SkeletonModifierContext& ctx, refSkeleton_t* skeleton ) override;

private:
	struct delta_t {
		vec3_t delta;
		quat_t rot;
	};

	std::unordered_map<int, std::vector<delta_t>> deltas_;
	std::vector<int> boneIndicies_;
};
#endif  // CG_ANIMDELTA_H
