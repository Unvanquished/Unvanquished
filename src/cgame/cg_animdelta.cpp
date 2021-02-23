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

#include "cg_animdelta.h"

namespace
{

int LoadDeltaAnimation( weapon_t weapon, const char* modelName, bool iqm )
{
	int handle = 0;
	if ( iqm )
	{
		handle = trap_R_RegisterAnimation( va( "models/players/%s/%s.iqm:%s_delta", modelName, modelName, BG_Weapon( weapon )->name ) );
	}
	else
	{
		handle = trap_R_RegisterAnimation( va( "models/players/%s/%s_delta.md5anim", modelName, BG_Weapon( weapon )->name ) );
	}
	return handle;
}

}  // namespace

bool AnimDelta::ParseConfiguration(clientInfo_t* ci, const char* token2, const char** data_p)
{
	if ( Q_stricmp( token2, "handBones" ) ) return false;
	char* token = COM_Parse2( data_p );
	if ( !token || token[0] != '{' )
	{
		Log::Warn( "Expected '{' but found '%s' in %s's AnimDelta", token, ci->modelName );
		return true;
	}
	while ( 1 )
	{
		token = COM_Parse( data_p );
		if ( !token || token[0] == '}' ) break;
		int index = trap_R_BoneIndex( ci->bodyModel, token );
		if ( index < 0 )
		{
			Log::Warn("AnimDelta: Error finding bone '%s' in %s", token, ci->modelName );
		}
		boneIndicies_.push_back( index );
	}
	return true;
}

bool AnimDelta::LoadData(clientInfo_t* ci)
{
	char newModelName[ MAX_QPATH ];
	// special handling for human_(naked|light|medium)
	if ( !Q_stricmp( ci->modelName, "human_naked"   ) ||
		!Q_stricmp( ci->modelName, "human_light"   ) ||
		!Q_stricmp( ci->modelName, "human_medium" ) )
	{
		Q_strncpyz( newModelName, "human_nobsuit_common", sizeof( newModelName ) );
	}
	else
	{
		Q_strncpyz( newModelName, ci->modelName, sizeof( newModelName ) );
	}

	refSkeleton_t base;
	refSkeleton_t delta;
	for ( int i = WP_NONE + 1; i < WP_NUM_WEAPONS; ++i )
	{
		int handle = LoadDeltaAnimation( static_cast<weapon_t>( i ), newModelName, ci->iqm );
		if ( !handle ) continue;
		Log::Debug("Loaded delta for %s %s", newModelName, BG_Weapon( i )->humanName);
		trap_R_BuildSkeleton( &delta, handle, 1, 1, 0, false );
		// Derive the delta from the base stand animation.
		trap_R_BuildSkeleton( &base, ci->animations[ TORSO_STAND ].handle, 1, 1, 0, false );
		auto ret = deltas_.insert( std::make_pair( i, std::vector<delta_t>( boneIndicies_.size() ) ) );
		auto& weaponDeltas = ret.first->second;
		for ( size_t j = 0; j < boneIndicies_.size(); ++j )
		{
			VectorSubtract( delta.bones[ boneIndicies_[ j ] ].t.trans, base.bones[ boneIndicies_[ j ] ].t.trans, weaponDeltas[ j ].delta );
			QuatInverse( base.bones[ boneIndicies_[ j ] ].t.rot );
			QuatMultiply( base.bones[ boneIndicies_[ j ] ].t.rot, delta.bones[ boneIndicies_[ j ] ].t.rot, weaponDeltas[ j ].rot );
		}
	}
	return true;
}

void AnimDelta::Apply(const SkeletonModifierContext& ctx, refSkeleton_t* skeleton)
{
	if ( ( CG_AnimNumber( ctx.es->torsoAnim ) ) < TORSO_ATTACK ) return;
	auto it = deltas_.find( ctx.es->weapon );
	if ( it == deltas_.end() ) return;
	for ( size_t i = 0; i < it->second.size(); ++i )
	{
		VectorAdd( it->second[ i ].delta, skeleton->bones[ boneIndicies_[ i ] ].t.trans, skeleton->bones[ boneIndicies_[ i ] ].t.trans );
		QuatMultiply2( skeleton->bones[ boneIndicies_[ i ] ].t.rot, it->second[ i ].rot );
	}
}




