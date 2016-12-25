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

#include "cg_segmented_skeleton.h"

static int BoneLookup(const clientInfo_t* ci, const char* name)
{
	if (!name) {
		name = "";
	}
	int idx = trap_R_BoneIndex(ci->bodyModel, name);
	if (idx < 0) {
		Log::Warn("Unknown bone name '%s' for model %s", name, ci->modelName);
	}
	return idx;
}


bool HumanSkeletonRotations::ParseConfiguration(clientInfo_t* ci, const char* token, const char** data_p)
{
	if (!Q_stricmp(token, "torsoControlBone")) {
		torsoControlBone = BoneLookup(ci, COM_Parse2(data_p));
		return true;
	}
	if (!Q_stricmp(token, "leftShoulder")) {
		leftShoulderBone = BoneLookup(ci, COM_Parse2(data_p));
		return true;
	}
	if (!Q_stricmp(token, "rightShoulder")) {
		rightShoulderBone = BoneLookup(ci, COM_Parse2(data_p));
		return true;
	}
	return false;
}


void HumanSkeletonRotations::Apply(const SkeletonModifierContext& ctx, refSkeleton_t* skeleton)
{
	// HACK: Stop taunt from clipping through the body.
	int anim = ctx.es->torsoAnim & ~ANIM_TOGGLEBIT;
	if ( anim >= TORSO_GESTURE_BLASTER && anim <= TORSO_GESTURE_CKIT )
	{
		quat_t rot;
		QuatFromAngles( rot, 15, 0, 0 );
		QuatMultiply2( skeleton->bones[ 33 ].t.rot, rot );
	}

	// rotate torso
	if ( torsoControlBone >= 0 && torsoControlBone < skeleton->numBones )
	{
		// HACK: convert angles to bone system
		quat_t rotation;
		QuatFromAngles( rotation, ctx.torsoYawAngle, 0, 0 );
		QuatMultiply2( skeleton->bones[ torsoControlBone ].t.rot, rotation );
	}

	// HACK: limit angle (avoids worst of the gun clipping through the body)
	// Needs some proper animation fixes...
	auto pitch = Math::Clamp<vec_t>(ctx.pitchAngle, -60, 60);
	quat_t rotation;
	QuatFromAngles( rotation, -pitch, 0, 0 );
	QuatMultiply2( skeleton->bones[ rightShoulderBone ].t.rot, rotation );

	// Relationships are emphirically derived. They will probably need to be changed upon changes to the human model
	QuatFromAngles( rotation, pitch, pitch < 0 ? -pitch / 9 : -pitch / ( 8 - ( 5 * ( pitch / 90 ) ) ), 0 );
	QuatMultiply2( skeleton->bones[ leftShoulderBone ].t.rot, rotation );
}
