/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2025 Daemon Developers
All rights reserved.

This file is part of the Daemon BSD Source Code (Daemon Source Code).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the Daemon developers nor the
	  names of its contributors may be used to endorse or promote products
	  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===========================================================================
*/
// EntityCache.cpp

#include "cg_local.h"

#include "EntityCache.h"

static refEntity_t entities[MAX_REF_ENTITIES];
orientation_t entityOrientations[MAX_REF_ENTITIES];

static std::vector<EntityUpdate> entityUpdates;
static std::vector<attachment_t*> attachmentUpdates;

EntityCache entityCache;

static Cvar::Cvar<bool> cg_showEntityCacheUpdates( "cg_showEntityCacheUpdates",
	"Show cgame-side updates in render entity cache", Cvar::NONE, false );

bool operator==( const BoneMod& lhs, const BoneMod& rhs ) {
	if ( lhs.type != rhs.type ) {
		return false;
	}

	switch ( lhs.type ) {
		case BONE_ROTATE:
			return lhs.index == rhs.index && VectorCompareEpsilon( lhs.rotation, rhs.rotation, 0.01f )
				&& fabsf( lhs.rotation[3] - rhs.rotation[3] ) <= 0.01f;
		case BUILD_EXTRA_SKELETON:
			return lhs.animationHandle == rhs.animationHandle
				&& lhs.startFrame == rhs.startFrame && lhs.endFrame == rhs.endFrame && lhs.lerp == rhs.lerp;
		case BONE_FROM_EXTRA_SKELETON:
			return lhs.index == rhs.index;
		default:
			ASSERT_UNREACHABLE();
	}
}

bool operator!=( const BoneMod& lhs, const BoneMod& rhs ) {
	return !( lhs == rhs );
}

void UpdateEntity( const refEntity_t& ent, const uint16_t id ) {
	refEntity_t* current = &entities[id];

	bool update = memcmp( &ent, current, offsetof( refEntity_t, lightingOrigin ) )
		|| ent.tag != current->tag || !current->active
		|| !VectorCompareEpsilon( ent.origin, current->origin, 0.01f )
		|| !VectorCompareEpsilon( ent.boundsRotation, current->boundsRotation, 0.01f )
		|| !VectorCompareEpsilon( ent.axis[0], current->axis[0], 0.01f )
		|| !VectorCompareEpsilon( ent.axis[1], current->axis[1], 0.01f )
		|| !VectorCompareEpsilon( ent.axis[2], current->axis[2], 0.01f )
		|| ent.nonNormalizedAxes  != current->nonNormalizedAxes
		|| ent.attachmentEntity   != current->attachmentEntity
		|| ent.shaderRGBA.Red()   != current->shaderRGBA.Red()
		|| ent.shaderRGBA.Green() != current->shaderRGBA.Green()
		|| ent.shaderRGBA.Blue()  != current->shaderRGBA.Blue()
		|| ent.shaderRGBA.Alpha() != current->shaderRGBA.Alpha();

	if ( !update && ent.boneMods.size() == current->boneMods.size() ) {
		for ( uint32_t i = 0; i < ent.boneMods.size(); i++ ) {
			if ( ent.boneMods[i] != current->boneMods[i] ) {
				update = true;
				break;
			}
		}
	}

	if ( update ) {
		entities[id] = ent;
		entities[id].active = true;
		entityUpdates.push_back( { entities[id], id } );
	}
}

void AddRefEntities( centity_t* cent, std::vector<refEntity_t>& ents, const bool realloc ) {
	uint16_t frameOffset    = 0;
	uint8_t& frameCount     = cent->refEntitiesFrameCount[cent->refEntitiesFrame];
	uint8_t  lastFrameCount = cent->refEntitiesFrameCount[cent->refEntitiesFrame ^ 1];

	if ( cg_showEntityCacheUpdates.Get() ) {
		Log::defaultLogger.WithoutSuppression().Notice(
			Str::Format( "Cache add entities: centity: %u frame: %u "
				"current entities: %u last frame entities: %u offset: %u count: %u new entities: %u realloc: %s",
				cent - cg_entities, cent->refEntitiesFrame, frameCount, lastFrameCount,
				cent->refEntitiesOffset, cent->refEntitiesCount,
				ents.size(), realloc )
		);
	}

	// if ( realloc ) {
		if ( ents.size() + frameCount > cent->refEntitiesCount ) {
			entityCache.Free( cent->refEntitiesOffset, cent->refEntitiesCount, false );

			const uint16_t oldOffset = cent->refEntitiesOffset;
			cent->refEntitiesOffset = entityCache.Alloc( ents.size() + frameCount );

			if ( cent->refEntitiesCount && cent->refEntitiesOffset != oldOffset ) {
				entityCache.Free( oldOffset, cent->refEntitiesCount, true );

				for ( refEntity_t* ent = entities + oldOffset, *ent2 = entities + cent->refEntitiesOffset;
					ent < entities + oldOffset + cent->refEntitiesCount;
					ent++, ent2++ ) {
					*ent2 = *ent;
				}
			}

			cent->refEntitiesCount = ents.size() + frameCount;
		}

		frameOffset = frameCount;
		frameCount += ents.size();

		if ( cg_showEntityCacheUpdates.Get() ) {
			Log::defaultLogger.WithoutSuppression().Notice(
				Str::Format( "Cache add entities: reallocated: frameOffset: %u frameCount: %u",
					frameOffset, frameCount )
			);
	//	}
	/* } else if ( ents.size() > cent->refEntitiesCount ) {
		entityCache.Free( cent->refEntitiesOffset, cent->refEntitiesCount, false );

		cent->refEntitiesOffset = entityCache.Alloc( ents.size() );
		cent->refEntitiesCount = ents.size();
		frameCount = ents.size();

		if ( cg_showEntityCacheUpdates.Get() ) {
			Log::defaultLogger.WithoutSuppression().Notice(
				Str::Format( "Cache add entities: allocated: offset: %u frameCount: %u",
					cent->refEntitiesOffset, frameCount )
			);
		} */
	} else if ( false && ents.size() < cent->refEntitiesCount ) {
		/* if ( ents.size() > lastFrameCount ) {
			entityCache.Free( cent->refEntitiesOffset + ents.size(), cent->refEntitiesCount - ents.size(), true );
			cent->refEntitiesCount = ents.size();

			if ( cg_showEntityCacheUpdates.Get() ) {
				Log::defaultLogger.WithoutSuppression().Notice(
					Str::Format( "Cache add entities: free: %u", ents.size() )
				);
			}
		} else */ if ( cent->refEntitiesCount > lastFrameCount ) {
			entityCache.Free( cent->refEntitiesOffset + lastFrameCount, cent->refEntitiesCount - lastFrameCount, true );
			cent->refEntitiesCount = lastFrameCount;

			if ( cg_showEntityCacheUpdates.Get() ) {
				Log::defaultLogger.WithoutSuppression().Notice(
					Str::Format( "Cache add entities: free: %u", lastFrameCount )
				);
			}
		}

		frameCount = ents.size();
	}

	uint16_t i = 0;
	for ( refEntity_t& ent : ents ) {
		ent.attachmentEntity += cent->refEntitiesOffset + frameOffset;
		UpdateEntity( ent, i + cent->refEntitiesOffset + frameOffset );
		i++;
	}
}

void AddRefEntity( centity_t* cent, refEntity_t& ent, const bool realloc ) {
	std::vector<refEntity_t> ents{ ent };

	AddRefEntities( cent, ents, realloc );
}

void AddRefEntities( centity_t* cent, std::initializer_list<refEntity_t> ents, const bool realloc ) {
	std::vector<refEntity_t> ents2;
	ents2.reserve( ents.size() );

	for ( const refEntity_t& ent : ents ) {
		ents2.push_back( ent );
	}

	AddRefEntities( cent, ents2, realloc );
}

void ClearRefEntityCache() {
	entityUpdates.clear();
	entityUpdates.reserve( 512 );

	attachmentUpdates.clear();
	attachmentUpdates.reserve( 512 );

	entityCache.Clear();
}

void AddAttachment( attachment_t* attachment ) {
	attachmentUpdates.push_back( attachment );
}

void SyncEntityCacheToEngine() {
	if ( cg_showEntityCacheUpdates.Get() ) {
		Log::defaultLogger.WithoutSuppression().Notice(
			Str::Format( "Cache sync cgame->engine: %u entity updates", entityUpdates.size() )
		);
	}

	trap_R_SyncRefEntities( entityUpdates );

	entityUpdates.clear();
}

void SyncEntityCacheFromEngine() {
	std::vector<LerpTagUpdate> lerpTagUpdates;
	lerpTagUpdates.reserve( attachmentUpdates.size() );

	for ( attachment_t* attachment : attachmentUpdates ) {
		lerpTagUpdates.push_back(
			{ attachment->tagName, ( uint16_t ) ( attachment->centity->refEntitiesOffset + attachment->entity ) }
		);
	}

	std::vector<LerpTagSync> orientations = trap_R_SyncLerpTags( lerpTagUpdates );

	if ( cg_showEntityCacheUpdates.Get() ) {
		Log::defaultLogger.WithoutSuppression().Notice( Str::Format( "Cache sync engine->cgame:" ) );

		for ( uint16_t i = 0; i < orientations.size(); i++ ) {
			orientation_t& entityOrientation = orientations[i].entityOrientation;
			vec3_t entityAngles;
			AxisToAngles( entityOrientation.axis, entityAngles );

			orientation_t& orientation       = orientations[i].orientation;
			vec3_t boneAngles;
			AxisToAngles( orientation.axis,       boneAngles );

			Log::defaultLogger.WithoutSuppression().Notice(
				Str::Format( " %u:%s|%f %f %f|%f %f %f|%f %f %f|%f %f %f",
				lerpTagUpdates[i].id, lerpTagUpdates[i].tag,
				entityOrientation.origin[0], entityOrientation.origin[1], entityOrientation.origin[2],
				entityAngles[0], entityAngles[1], entityAngles[2],
				orientation.origin[0], orientation.origin[1], orientation.origin[2],
				boneAngles[0], boneAngles[1], boneAngles[2] )
			);
		}
	}

	for ( uint16_t i = 0; i < orientations.size(); i++ ) {
		VectorCopy( orientations[i].entityOrientation.origin, attachmentUpdates[i]->origin );

		for ( int j = 0; j < 3; j++ ) {
			VectorMA( attachmentUpdates[i]->origin, orientations[i].orientation.origin[j],
				orientations[i].entityOrientation.axis[j], attachmentUpdates[i]->origin );
		}

		axis_t tempAxis;
		AxisMultiply( axisDefault, orientations[i].orientation.axis, tempAxis );
		AxisMultiply( tempAxis, orientations[i].entityOrientation.axis, attachmentUpdates[i]->axis );
	}

	attachmentUpdates.clear();
}

uint32_t EntityCache::Alloc( const uint32_t count ) {
	if ( !count ) {
		Log::Warn( "EntityCache: tried to allocate 0 blocks" );
		return 0;
	}

	if( count > 64 ) {
		Sys::Drop( "EntityCache: allocation failed: only up to 64 blocks supported in one allocation, got %u", count );
	}

	for ( uint64_t& block : blocks ) {
		if ( block == UINT64_MAX ) {
			continue;
		}

		if ( !block ) {
			block |= UINT64_MAX >> ( 64 - count );
			return ( &block - &blocks[0] ) * 64;
		}

		uint64_t mask = UINT64_MAX;
		uint32_t bitOffset = 0;
		while ( true ) {
			uint32_t offset = ( block & mask ) ? CountTrailingZeroes( block & mask ) : 64;

			if ( offset == 64 && offset == bitOffset ) {
				break;
			}
			
			if ( count <= offset - bitOffset ) {
				mask = UINT64_MAX >> ( 64 - count );
				block |= mask << bitOffset;
				return bitOffset + ( &block - &blocks[0] ) * 64;
			}

			mask = offset == 63 ? 0 : UINT64_MAX << ( offset + 1 );

			bitOffset = offset + 1;

			// Find the next zero bit so we don't loop over blocks that are allocated
			uint64_t flipped = ~block & mask;

			if ( flipped ) {
				bitOffset = CountTrailingZeroes( flipped );
				mask = UINT64_MAX << ( bitOffset );
			} else {
				break;
			}
		}
	}

	Sys::Drop( "EntityCache: allocation failed: no contigous memory found for %u blocks", count );
}

void EntityCache::Free( const uint32_t offset, const uint32_t count, const bool update ) {
	if ( !count ) {
		return;
	}

	uint64_t& block = blocks[offset / 64];
	const uint64_t mask = UINT64_MAX >> ( 64 - count );

	block ^= mask << offset;

	if ( update ) {
		for ( refEntity_t* ent = entities + offset; ent < entities + offset + count; ent++ ) {
			ent->active = false;
			entityUpdates.push_back( EntityUpdate{ *ent, ( uint16_t ) ( ent - entities ) } );
		}
	}
}

void EntityCache::Clear() {
	memset( blocks, 0, blockCount * sizeof( uint64_t ) );
}