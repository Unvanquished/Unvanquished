/*
===========================================================================

Unvanquished BSD Source Code
Copyright (c) 2025 Unvanquished Developers
All rights reserved.

This file is part of the Unvanquished BSD Source Code (Unvanquished Source Code).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.
	* Neither the name of the Unvanquished developers nor the
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
// EntityCache.h

#ifndef ENTITY_CACHE_H
#define ENTITY_CACHE_H

#include <cstdint>
#include <initializer_list>

struct centity_t;
struct refEntity_t;
struct attachment_t;
struct orientation_t;

extern orientation_t entityOrientations[MAX_REF_ENTITIES];

void AddRefEntities( centity_t* cent, std::initializer_list<refEntity_t> ents );

void AddRefEntity( centity_t* cent, refEntity_t& ent );
void AddRefEntities( centity_t* cent, std::vector<refEntity_t>& ents );

void ClearRefEntityCache();

void AddAttachment( attachment_t* attachment );

void SyncEntityCacheToEngine();
void SyncEntityCacheFromEngine();

class EntityCache {
	public:
	uint32_t Alloc( const uint32_t count );
	void Free( const uint32_t offset, const uint32_t count, const bool update );

	void Clear();

	private:
	using block1_t = uint64_t;
	using block2_t = uint64_t;
	static constexpr uint32_t blockSize1 = std::numeric_limits<block1_t>::digits; // "digits" = num bits
	static constexpr uint32_t blockSize2 = std::numeric_limits<block2_t>::digits;

	static constexpr uint32_t blockCount   = MAX_REF_ENTITIES / blockSize1;
	static constexpr uint32_t blockCountL2 = blockCount / blockSize2;

	block2_t blocksL2[blockCountL2];
	block1_t blocks[blockCount];
};

extern EntityCache entityCache;

#endif // ENTITY_CACHE_H
