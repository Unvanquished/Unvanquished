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
#include "client/client.h"
#include "detour/DetourDebugDraw.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include "detour/DebugDraw.h"
#pragma GCC diagnostic pop
#include "bot_navdraw.h"
#include "nav.h"

static const int DEFAULT_CONNECTION_SIZE = 50;
static int connectionSize = DEFAULT_CONNECTION_SIZE;

bool GetPointPointedTo( NavData_t *nav, rVec &p )
{
	qVec forward;
	qVec end;
	rVec extents;
	rVec pos;
	trace_t trace;
	dtPolyRef nearRef;

	VectorSet( extents, 640, 96, 640 );

	AngleVectors( cl.snap.ps.viewangles, forward, nullptr, nullptr );
	VectorMA( cl.snap.ps.origin, 8096, forward, end );

	CM_BoxTrace( &trace, cl.snap.ps.origin, end, nullptr, nullptr, 0,
	             CONTENTS_SOLID | CONTENTS_PLAYERCLIP, 0, traceType_t::TT_AABB );

	pos = qVec( trace.endpos );
	if ( dtStatusFailed( nav->query->findNearestPoly( pos, extents, &nav->filter, &nearRef, p ) ) )
	{
		return false;
	}

	return true;
}

static struct
{
	bool enabled;
	bool offBegin;
	bool shownodes;
	bool showportals;
	OffMeshConnection pc;
	rVec start;
	bool validPathStart;
	NavData_t *nav;
} cmd;

void BotDrawNavEdit( DebugDrawQuake *dd )
{
	rVec p;

	if ( cmd.enabled && GetPointPointedTo( cmd.nav, p ) )
	{
		unsigned int col = duRGBA( 255, 255, 255, 220 );
		dd->begin( DU_DRAW_LINES, 2.0f );
		duAppendCircle( dd, p[ 0 ], p[ 1 ], p[ 2 ], connectionSize, col );

		if ( cmd.offBegin )
		{
			duAppendArc( dd, cmd.pc.start[ 0 ], cmd.pc.start[ 1 ], cmd.pc.start[ 2 ], p[ 0 ], p[ 1 ], p[ 2 ], 0.25f,
						0.6f, 0.6f, col );
			duAppendCircle( dd, cmd.pc.start[ 0 ], cmd.pc.start[ 1 ], cmd.pc.start[ 2 ], connectionSize, col );
		}
		dd->end();
	}
}

void DrawPath( Bot_t *bot, DebugDrawQuake &dd )
{
	dd.depthMask(false);
	const unsigned int spathCol = duRGBA(128,128,128,255);
	dd.begin(DU_DRAW_LINES, 3.0f);
	dd.vertex( bot->corridor.getPos(), spathCol );
	for (int i = 0; i < bot->numCorners; ++i)
		dd.vertex(bot->cornerVerts[i*3], bot->cornerVerts[i*3+1]+5.0f, bot->cornerVerts[i*3+2], spathCol);
	if ( bot->numCorners < MAX_CORNERS - 1 )
	{
		if ( bot->numCorners % 2 != 0 )
		{
			dd.vertex( &bot->cornerVerts[ ( bot->numCorners - 1 ) * 3 ], spathCol );
		}

		dd.vertex( bot->corridor.getTarget(), spathCol );
	}
	dd.end();
	dd.depthMask(true);
}

void BotDebugDrawMesh( BotDebugInterface_t *in )
{
	static DebugDrawQuake dd;

	dd.init( in );

	if ( !cmd.enabled )
	{
		return;
	}

	if ( !cmd.nav )
	{
		return;
	}

	if ( !cmd.nav->mesh || !cmd.nav->query )
	{
		return;
	}

	if ( cmd.shownodes )
	{
		duDebugDrawNavMeshNodes( &dd, *cmd.nav->query );
	}

	if ( cmd.showportals )
	{
		duDebugDrawNavMeshPortals( &dd, *cmd.nav->mesh );
	}

	duDebugDrawNavMeshWithClosedList(&dd, *cmd.nav->mesh, *cmd.nav->query, DU_DRAWNAVMESH_OFFMESHCONS | DU_DRAWNAVMESH_CLOSEDLIST);
	BotDrawNavEdit( &dd );

	for ( int i = 0; i < MAX_CLIENTS; i++ )
	{
		Bot_t *bot = &agents[ i ];

		if ( bot->nav == cmd.nav )
		{
			DrawPath( bot, dd );
		}
	}
}

void Cmd_NavEdit()
{
	int argc = Cmd_Argc();
	const char *arg = nullptr;
	const char usage[] = "Usage: navedit enable/disable/save <navmesh>\n";

	if ( !Cvar_VariableIntegerValue( "sv_cheats" ) )
	{
		Com_Printf( "navedit only available in local devmap\n" );
		return;
	}

	if ( argc < 2 )
	{
		Com_Printf( "%s", usage );
		return;
	}

	arg = Cmd_Argv( 1 );

	if ( !Q_stricmp( arg, "enable" ) )
	{
		int i;
		if ( argc < 3 )
		{
			Com_Printf( "%s", usage );
			return;
		}

		arg = Cmd_Argv( 2 );
		for ( i = 0; i < numNavData; i++ )
		{
			if ( !Q_stricmp( BotNavData[ i ].name, arg ) )
			{
				cmd.nav = &BotNavData[ i ];
				break;
			}
		}

		if ( i == numNavData )
		{
			Com_Printf( "\'%s\' is not a valid navmesh name\n", arg );
			return;
		}

		if ( cmd.nav && cmd.nav->mesh && cmd.nav->cache && cmd.nav->query )
		{
			cmd.enabled = true;
			Cvar_Set( "r_debugSurface", "1" );
		}
	}
	else if ( !Q_stricmp( arg, "disable" ) )
	{
		cmd.enabled = false;
		Cvar_Set( "r_debugSurface", "0" );
	}
	else if ( !Q_stricmp( arg, "save" ) )
	{
		if ( !cmd.enabled )
		{
			return;
		}

		BotSaveOffMeshConnections( cmd.nav );
	}
	else
	{
		Com_Printf( "%s", usage );
	}
}

void Cmd_AddConnection()
{
	const char usage[] = "Usage: addcon start <dir> (radius)\n"
	                     " addcon end\n";
	const char *arg = nullptr;
	int argc = Cmd_Argc();

	if ( argc < 2 )
	{
		Com_Printf( "%s", usage );
		return;
	}

	arg = Cmd_Argv( 1 );

	if ( !Q_stricmp( arg, "start" ) )
	{
		if ( !cmd.enabled )
		{
			return;
		}

		if ( argc < 3 )
		{
			Com_Printf( "%s", usage );
			return;
		}

		arg = Cmd_Argv( 2 );

		if ( !Q_stricmp( arg, "oneway" ) )
		{
			cmd.pc.dir = 0;
		}
		else if ( !Q_stricmp( arg, "twoway" ) )
		{
			cmd.pc.dir = 1;
		}
		else
		{
			Com_Printf( "Invalid argument for direction, specify oneway or twoway\n" );
			return;
		}

		if ( GetPointPointedTo( cmd.nav, cmd.pc.start ) )
		{
			cmd.pc.area = DT_TILECACHE_WALKABLE_AREA;
			cmd.pc.flag = POLYFLAGS_WALK;
			cmd.pc.userid = 0;

			if ( argc == 4 )
			{
				cmd.pc.radius = std::max( atoi( Cmd_Argv( 3 ) ), 10 );
			}
			else
			{
				cmd.pc.radius = connectionSize;
			}
			cmd.offBegin = true;
		}
	}
	else if ( !Q_stricmp( arg, "end" ) )
	{
		if ( !cmd.enabled )
		{
			return;
		}

		if ( !cmd.offBegin )
		{
			return;
		}

		if ( GetPointPointedTo( cmd.nav, cmd.pc.end ) )
		{
			cmd.nav->process.con.addConnection( cmd.pc );

			rBounds box;
			box.addPoint( cmd.pc.start );
			box.addPoint( cmd.pc.end );

			box.mins[ 1 ] -= 10;
			box.maxs[ 1 ] += 10;

			// rebuild affected tiles
			dtCompressedTileRef refs[ 32 ];
			int tc = 0;
			cmd.nav->cache->queryTiles( box.mins, box.maxs, refs, &tc, 32 );

			for ( int k = 0; k < tc; k++ )
			{
				cmd.nav->cache->buildNavMeshTile( refs[ k ], cmd.nav->mesh );
			}

			cmd.offBegin = false;
		}
	}
	else
	{
		Com_Printf( "%s", usage );
	}
}

static void adjustConnectionSize( int dir )
{
	int argc = Cmd_Argc();
	int adjust = 5;
	int newConnectionSize;

	if ( argc > 1 )
	{
		adjust = Math::Clamp( atoi( Cmd_Argv( 1 ) ), 1, 20 );
	}

	newConnectionSize = Math::Clamp( connectionSize + dir * adjust, 20, 100 );

	if ( newConnectionSize != connectionSize )
	{
		connectionSize = newConnectionSize;
		Com_Printf( "Default connection size = %d\n", connectionSize );
	}
}

void Cmd_ConnectionSizeUp()
{
	return adjustConnectionSize( 1 );
}

void Cmd_ConnectionSizeDown()
{
	return adjustConnectionSize( -1 );
}

void Cmd_NavTest()
{
	const char usage[] = "Usage: navtest shownodes/hidenodes/showportals/hideportals/startpath/endpath\n";
	const char *arg = nullptr;
	int argc = Cmd_Argc();

	if ( !cmd.enabled )
	{
		Com_Printf( "%s", "Can't test navmesh without enabling navedit" );
		return;
	}

	if ( argc < 2 )
	{
		Com_Printf( "%s", usage );
		return;
	}

	arg = Cmd_Argv( 1 );

	if ( !Q_stricmp( arg, "shownodes" ) )
	{
		cmd.shownodes = true;
	}
	else if ( !Q_stricmp( arg, "hidenodes" ) )
	{
		cmd.shownodes = false;
	}
	else if ( !Q_stricmp( arg, "showportals" ) )
	{
		cmd.showportals = true;
	}
	else if ( !Q_stricmp( arg, "hideportals" ) )
	{
		cmd.showportals = false;
	}
	else if ( !Q_stricmp( arg, "startpath" ) )
	{
		if ( GetPointPointedTo( cmd.nav, cmd.start ) )
		{
			cmd.validPathStart = true;
		}
		else
		{
			cmd.validPathStart = false;
		}
	}
	else if ( !Q_stricmp( arg, "endpath" ) )
	{
		rVec end;
		if ( GetPointPointedTo( cmd.nav, end ) && cmd.validPathStart )
		{
			dtPolyRef startRef;
			dtPolyRef endRef;
			const int maxPath = 512;
			int npath;
			dtPolyRef path[ maxPath ];
			rVec nearPoint;
			rVec extents( 300, 300, 300 );

			cmd.nav->query->findNearestPoly( cmd.start, extents, &cmd.nav->filter, &startRef, nearPoint );
			cmd.nav->query->findNearestPoly( end, extents, &cmd.nav->filter, &endRef, nearPoint );

			cmd.nav->query->findPath( startRef, endRef, cmd.start, end, &cmd.nav->filter, path, &npath, maxPath );
		}
	}
	else
	{
		Com_Printf( "%s", usage );
	}
}

void NavEditInit()
{
#ifndef BUILD_SERVER
	memset( &cmd, 0, sizeof( cmd ) );
	Cvar_Set( "r_debugSurface", "0" );
	Cmd_AddCommand( "navedit", Cmd_NavEdit );
	Cmd_AddCommand( "addcon", Cmd_AddConnection );
	Cmd_AddCommand( "conSizeUp", Cmd_ConnectionSizeUp );
	Cmd_AddCommand( "conSizeDown", Cmd_ConnectionSizeDown );
	Cmd_AddCommand( "navtest", Cmd_NavTest );
#endif
}

void NavEditShutdown()
{
#ifndef BUILD_SERVER
	Cmd_RemoveCommand( "navedit" );
	Cmd_RemoveCommand( "addcon" );
	Cmd_RemoveCommand( "conSizeUp" );
	Cmd_RemoveCommand( "conSizeDown" );
	Cmd_RemoveCommand( "navtest" );
#endif
}
