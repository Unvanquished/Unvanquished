/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

extern "C" {
#include "../client/client.h"
}
#include "../../libs/detour/DetourDebugDraw.h"
#include "../../libs/detour/DebugDraw.h"
#include "bot_navdraw.h"
#include "nav.h"

static qboolean navEdit = qfalse;

qboolean GetPointPointedTo( NavData_t *nav, vec3_t p )
{
	vec3_t forward;
	vec3_t end;
	trace_t trace;
	vec3_t extents = { 640, 96, 640 };
	dtPolyRef nearRef;

	AngleVectors( cl.snap.ps.viewangles, forward, NULL, NULL );
	VectorMA( cl.snap.ps.origin, 8096, forward, end );

	CM_BoxTrace( &trace, cl.snap.ps.origin, end, NULL, NULL, 0, CONTENTS_SOLID | CONTENTS_PLAYERCLIP, TT_AABB );

	VectorCopy( trace.endpos, p );
	quake2recast( p );

	if ( dtStatusFailed( nav->query->findNearestPoly( p, extents, &nav->filter, &nearRef, end ) ) )
	{
		return qfalse;
	}

	VectorCopy( end, p );
	return qtrue;
}

static OffMeshConnection c;
static qboolean offBegin = qfalse;

void BotDrawNavEdit( NavData_t *nav, DebugDrawQuake *dd )
{
	vec3_t p;
	if ( GetPointPointedTo( nav, p ) )
	{
		unsigned int col = duRGBA( 255, 255, 255, 220 );
		dd->begin( DU_DRAW_LINES, 2.0f );
		duAppendCircle( dd, p[ 0 ], p[ 1 ], p[ 2 ], 50, col );

		if ( offBegin )
		{
			duAppendArc(dd, c.start[0],c.start[1],c.start[2], p[0], p[1], p[2], 0.25f,
						0.6f, 0.6f, col);
			duAppendCircle( dd, c.start[ 0 ], c.start[ 1 ], c.start[ 2 ], 50, col );
		}
		dd->end();
	}
}

void Cmd_NavEdit( void )
{
	int argc = Cmd_Argc();
	char *arg = NULL;
	int meshnum = cl.snap.ps.stats[ STAT_CLASS ] - 1;
	NavData_t *nav;

	if ( meshnum < 0 || meshnum >= numNavData )
	{
		Com_Printf( "No navmesh loaded for this class\n" );
		return;
	}

	nav = &BotNavData[ meshnum ];

	if ( !nav->mesh )
	{
		Com_Printf( "No navmesh loaded for this class\n" );
		return;
	}

	if ( !Cvar_VariableIntegerValue( "sv_cheats" ) )
	{
		Com_Printf( "Nav edit only available in local devmap\n" );
		return;
	}

	if ( argc < 2 )
	{
		Com_Printf( "usage: navedit <start/stop>/<begin/end> [type] [oneway/twoway]\n" );
		return;
	}

	arg = Cmd_Argv( 1 );

	if ( !Q_stricmp( arg, "start" ) )
	{
		offBegin = qfalse;
		navEdit = qtrue;
		Cvar_Set( "r_debugSurface", "1" );
	}
	else if ( !Q_stricmp( arg, "stop" ) )
	{
		offBegin = qfalse;
		navEdit = qfalse;
		Cvar_Set( "r_debugSurface", "0" );
	}
	else if ( !Q_stricmp( arg, "begin" ) )
	{
		if ( argc < 2 )
		{
			Com_Printf( "usage: navedit <start/stop>/<begin/end>\n" );
			return;
		}

		if ( GetPointPointedTo( nav, c.start ) )
		{
			c.area = DT_TILECACHE_WALKABLE_AREA;
			c.flag = POLYFLAGS_WALK;
			c.userid = 0;
			c.radius = 50;
			c.dir = 0;
			offBegin = qtrue;
		}
	}
	else if ( !Q_stricmp( arg, "end" ) )
	{
		if ( argc < 2 )
		{
			Com_Printf( "usage: navedit <start/stop>/<begin/end>\n" );
			return;
		}

		if ( !offBegin )
		{
			return;
		}
		
		if ( GetPointPointedTo( nav, c.end ) )
		{
			nav->process.con.addConnection( c );

			vec3_t omin, omax;
			ClearBounds( omin, omax );
			AddPointToBounds( c.start, omin, omax );
			AddPointToBounds( c.end, omin, omax );

			omin[ 1 ] -= 10;
			omax[ 1 ] += 10;

			// rebuild affected tiles
			for ( int i = 0; i < numNavData; i++ )
			{
				NavData_t *nav = &BotNavData[ i ];
				dtCompressedTileRef refs[ 32 ];
				int tc = 0;
				nav->cache->queryTiles( omin, omax, refs, &tc, 32 );

				for ( int k = 0; k < tc; k++ )
				{
					nav->cache->buildNavMeshTile( refs[ k ], nav->mesh );
				}
			}

			offBegin = qfalse;
		}
	}
}
