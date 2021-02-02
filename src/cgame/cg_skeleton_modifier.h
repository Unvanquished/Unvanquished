/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2016 Unvanquished Developers

Unvanquished is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Unvanquished is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef CG_SKELETON_MODIFIER_H
#define CG_SKELETON_MODIFIER_H

// Forward declare due to circular dependency.
struct clientInfo_t;

struct SkeletonModifierContext
{
	const entityState_t* es;
	vec_t torsoYawAngle;
	vec_t pitchAngle;
	const refSkeleton_t* legsSkeleton;
};

class SkeletonModifier
{
public:
	virtual ~SkeletonModifier() {}
	// ParseConfiguration: Return true if firstToken is consumed
	virtual bool ParseConfiguration( clientInfo_t*, const char* /*firstToken*/, const char**) { return false; }
	virtual bool LoadData( clientInfo_t* ) { return true; }
	virtual void Apply(const SkeletonModifierContext&, refSkeleton_t*) {}
};
#endif  // CG_SKELETON_MODIFIER_H
