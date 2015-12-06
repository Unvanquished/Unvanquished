/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "engine/qcommon/q_shared.h"
#include "engine/botlib/bot_api.h"

#define GAME_API_VERSION          2

#define SVF_NOCLIENT              0x00000001
#define SVF_CLIENTMASK            0x00000002
#define SVF_VISDUMMY              0x00000004
#define SVF_BOT                   0x00000008
#define SVF_POW                   0x00000010 // ignored by the engine
#define SVF_BROADCAST             0x00000020
#define SVF_PORTAL                0x00000040
#define SVF_BLANK                 0x00000080 // ignored by the engine
#define SVF_NOFOOTSTEPS           0x00000100 // ignored by the engine
#define SVF_CAPSULE               0x00000200
#define SVF_VISDUMMY_MULTIPLE     0x00000400
#define SVF_SINGLECLIENT          0x00000800
#define SVF_NOSERVERINFO          0x00001000 // only meaningful for entities numbered in [0..MAX_CLIENTS)
#define SVF_NOTSINGLECLIENT       0x00002000
#define SVF_IGNOREBMODELEXTENTS   0x00004000
#define SVF_SELF_PORTAL           0x00008000
#define SVF_SELF_PORTAL_EXCLUSIVE 0x00010000
#define SVF_RIGID_BODY            0x00020000 // ignored by the engine
#define SVF_CLIENTS_IN_RANGE      0x00040000 // clients within range

#define MAX_ENT_CLUSTERS  16

typedef struct
{
	bool linked; // false if not in any good cluster
	int      linkcount;

	int      svFlags; // SVF_NOCLIENT, SVF_BROADCAST, etc.
	int      singleClient; // only send to this client when SVF_SINGLECLIENT is set
	int      hiMask, loMask; // if SVF_CLIENTMASK is set, then only send to the
	//  clients specified by the following 64-bit bitmask:
	//  hiMask: high-order bits (32..63)
	//  loMask: low-order bits (0..31)
	float    clientRadius;    // if SVF_CLIENTS_IN_RANGE, send to all clients within this range

	bool bmodel; // if false, assume an explicit mins/maxs bounding box
	// only set by trap_SetBrushModel
	vec3_t   mins, maxs;
	int      contents; // CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc.
	// a non-solid entity should have this set to 0

	vec3_t absmin, absmax; // derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultaneous collision issues
	vec3_t currentOrigin;
	vec3_t currentAngles;

	// when a trace call is made and the specified pass entity isn't none,
	//  then a given entity will be excluded from testing if:
	// - the given entity is the pass entity (use case: don't interact with self),
	// - the owner of the given entity is the pass entity (use case: don't interact with your own missiles), or
	// - the given entity and the pass entity have the same owner entity (that is not none)
	//    (use case: don't interact with other missiles from owner).
	// that is, ent will be excluded if
	// ( passEntityNum != ENTITYNUM_NONE &&
	//   ( ent->s.number == passEntityNum || ent->r.ownerNum == passEntityNum ||
	//     ( ent->r.ownerNum != ENTITYNUM_NONE && ent->r.ownerNum == entities[passEntityNum].r.ownerNum ) ) )
	int      ownerNum;

	bool snapshotCallback;

	int numClusters; // if -1, use headnode instead
	int clusternums[ MAX_ENT_CLUSTERS ];
	int lastCluster; // if all the clusters don't fit in clusternums
	int originCluster; // Gordon: calced upon linking, for origin only bmodel vis checks
	int areanum, areanum2;
} entityShared_t;

// the server looks at a sharedEntity_t structure, which must be at the start of a gentity_t structure
typedef struct
{
	entityState_t  s; // communicated by the server to clients
	entityShared_t r; // shared by both the server and game module
} sharedEntity_t;

