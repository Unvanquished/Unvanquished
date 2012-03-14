/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "g_local.h"

//
// Waypoints
//

/*#define WAYPOINTSET_POSTDELAY_TIME  3000  // can only set a waypoint once every 3 seconds

void waypoint_think( gentity_t *ent ) {

        if( level.time - ent->lastHintCheckTime < WAYPOINTSET_POSTDELAY_TIME ) {
                ent->nextthink = level.time + FRAMETIME;
                return;
        }

        ent->nextthink = level.time + FRAMETIME;

}

void G_SetWayPoint( gentity_t *ent, wayPointType_t wayPointType, vec3_t loc ) {
        gclient_t *client;
        gentity_t *wp;

        if( !ent || !ent->client ) {
                return; // something went horribly wrong here
        }

        if( !wayPointType || wayPointType >= NUM_WAYPOINTTYPES ) {
                G_Printf( "^3WARNING: G_SetWayPoint, bad waypoint type %i\n", wayPointType );
                return;
        }

        client = ent->client;

        if( !client->pers.wayPoint ) {
                wp = G_Spawn();

                wp->r.svFlags = SVF_BROADCAST;
                wp->classname = "waypoint";
                wp->s.eType = ET_WAYPOINT;
                wp->s.pos.trType = TR_STATIONARY;

                wp->r.ownerNum = ent->s.number;
                wp->s.clientNum = ent->s.number;

                wp->think = waypoint_think;
                wp->nextthink = level.time + FRAMETIME;

                // Set location
                VectorCopy( loc, wp->s.pos.trBase );

                // Set type
                wp->s.frame = wayPointType;

                // Can't set for a while - hijack this to save some memory
                wp->lastHintCheckTime = level.time;

                trap_LinkEntity( wp );

                client->pers.wayPoint = wp;
        } else {

                wp = client->pers.wayPoint;

                if( level.time - wp->lastHintCheckTime < WAYPOINTSET_POSTDELAY_TIME ) {
                        // Latching, more hijacking
                        wp->botDelayBegin = qtrue;  // to indicate we got latched values

                        // Latch location
                        VectorCopy( loc, wp->dl_color );

                        // Latch type
                        wp->key = wayPointType;

                        return;
                }

                // Set location
                VectorCopy( loc, wp->s.pos.trBase );

                // Set type
                wp->s.frame = wayPointType;

                // Can't set for a while
                wp->lastHintCheckTime = level.time;

                trap_LinkEntity( wp );
        }
}*/

/*void G_RemoveWayPoint( gclient_t *client ) {
        if( client->pers.wayPoint ) {
                G_FreeEntity( client->pers.wayPoint );
                client->pers.wayPoint = NULL;
        }
}*/

void G_RemoveFromAllIgnoreLists( int clientNum )
{
	int i;

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		COM_BitClear( level.clients[ i ].sess.ignoreClients, clientNum );
	}
}
