/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef SG_TRAPCALLS_H_
#define SG_TRAPCALLS_H_

#include <glm/vec3.hpp>

#include "shared/CommonProxies.h"
#include "shared/server/sg_api.h"

struct gentity_t;

// Actually involves a trap call. (But does stuff within the sgame VM too; hence it being
// implemented in the sgame rather than daemon/src/shared/).
void             trap_AdjustAreaPortalState( gentity_t *ent, bool open );

/*
 * All the rest of these are fake trap calls: none of them talk to the engine.
 */

void             trap_LinkEntity( gentity_t *ent );
void             trap_UnlinkEntity( gentity_t *ent );
int              trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *list, int maxcount );
bool         trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
void             trap_Trace( trace_t *results, const glm::vec3& start, const glm::vec3& mins, const glm::vec3& maxs, const glm::vec3& end, int passEntityNum, int contentmask , int skipmask);
void             trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask , int skipmask);
void             trap_SetBrushModel( gentity_t *ent, const char *name );
bool         trap_InPVS( const vec3_t p1, const vec3_t p2 );
bool         trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );

#endif // SG_TRAPCALLS_H_
