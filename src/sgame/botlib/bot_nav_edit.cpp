/*
===========================================================================

Daemon BSD Source Code
Copyright (c) 2013 Daemon Developers
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

===========================================================================
*/

#include "common/cm/cm_public.h"
#include "sgame/sg_local.h"
#include "DetourDebugDraw.h"
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "DebugDraw.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include "bot_navdraw.h"
#include "nav.h"

static const int DEFAULT_CONNECTION_SIZE = 50;

bool GetPointPointedTo( NavData_t *nav, rVec &p )
{
	qVec forward;
	qVec end;
	rVec extents;
	rVec pos;
	trace_t trace;
	dtPolyRef nearRef;

	VectorSet( extents, 640, 96, 640 );

	// Nav edit commands are only allowed in a local game, where the host is guaranteed to be in slot 0
	const playerState_t *ps = &g_clients[ 0 ].ps;

	AngleVectors( ps->viewangles, forward, nullptr, nullptr );
	VectorMA( ps->origin, 8096, forward, end );

	CM_BoxTrace( &trace, ps->origin, end, nullptr, nullptr, 0,
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
		duAppendCircle( dd, p[ 0 ], p[ 1 ], p[ 2 ], DEFAULT_CONNECTION_SIZE, col );

		if ( cmd.offBegin )
		{
			duAppendArc( dd, cmd.pc.start[ 0 ], cmd.pc.start[ 1 ], cmd.pc.start[ 2 ], p[ 0 ], p[ 1 ], p[ 2 ], 0.25f,
						0.6f, 0.6f, col );
			duAppendCircle( dd, cmd.pc.start[ 0 ], cmd.pc.start[ 1 ], cmd.pc.start[ 2 ], DEFAULT_CONNECTION_SIZE, col );
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

void BotDebugDrawMesh()
{
	static DebugDrawQuake dd;

	dd.init();

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

	dd.sendCommands();
}

static bool CheckHost( gentity_t *ent )
{
	if ( level.inClient && ent->client->ps.clientNum == 0 )
	{
		// It's ok for further messages from commands to use Log::Notice since we have established
		// that the client console is the server console
		return true;
	}

	trap_SendServerCommand(
		ent - g_entities,
		"print_tr " QQ( "you must be the host of a local devmap to use this command" ) );
	return false;
}

#define Cmd_Argc trap_Argc
#define Cmd_Argv(i) (trap_Args().Argv(i).c_str())

void Cmd_NavEdit( gentity_t *ent )
{
	if ( !CheckHost( ent ) ) return;
	int argc = Cmd_Argc();
	const char *arg = nullptr;
	const char usage[] = "Usage: navedit enable/disable/save <navmesh>";

	if ( argc < 2 )
	{
		Log::Notice( usage );
		return;
	}

	arg = Cmd_Argv( 1 );

	if ( !Q_stricmp( arg, "enable" ) )
	{
		int i;
		if ( argc < 3 )
		{
			Log::Notice( usage );
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
			Log::Notice( "\'%s\' is not a valid navmesh name", arg );
			return;
		}

		if ( cmd.nav && cmd.nav->mesh && cmd.nav->cache && cmd.nav->query )
		{
			cmd.enabled = true;
			Cvar::SetValue( "r_debugSurface", "1" );
		}
	}
	else if ( !Q_stricmp( arg, "disable" ) )
	{
		cmd.enabled = false;
		Cvar::SetValue( "r_debugSurface", "0" );
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
		Log::Notice( usage );
	}
}

void Cmd_AddConnection( gentity_t *ent )
{
	if ( !CheckHost( ent ) ) return;
	const char usage[] = "Usage: addcon start <dir> (radius)\n"
	                     " addcon end";
	const char *arg = nullptr;
	int argc = Cmd_Argc();

	if ( argc < 2 )
	{
		Log::Notice( usage );
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
			Log::Notice( usage );
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
			Log::Notice( "Invalid argument for direction, specify oneway or twoway" );
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
				cmd.pc.radius = DEFAULT_CONNECTION_SIZE;
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
		Log::Notice( usage );
	}
}

void Cmd_NavTest( gentity_t *ent )
{
	if ( !CheckHost( ent ) ) return;
	const char usage[] = "Usage: navtest shownodes/hidenodes/showportals/hideportals/startpath/endpath";
	const char *arg = nullptr;
	int argc = Cmd_Argc();

	if ( !cmd.enabled )
	{
		Log::Notice( "Can't test navmesh without enabling navedit" );
		return;
	}

	if ( argc < 2 )
	{
		Log::Notice( usage );
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
		Log::Notice( usage );
	}
}

void NavEditInit()
{
	memset( &cmd, 0, sizeof( cmd ) );
}

void NavEditShutdown()
{
	if ( cmd.enabled )
	{
		Cvar::SetValue( "r_debugSurface", "0" );
	}
}
