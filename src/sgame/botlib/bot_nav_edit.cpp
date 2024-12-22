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

#include "common/Common.h"
#include "common/cm/cm_public.h"
#include "sgame/sg_local.h"
#include "sgame/sg_bot_util.h"
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
#include "shared/bot_nav_shared.h"

static const int DEFAULT_CONNECTION_SIZE = 50;

static bool GetPointPointedTo( NavData_t *nav, dtQueryFilter &filter, rVec &p )
{
	rVec extents;
	trace_t trace;
	dtPolyRef nearRef;

	VectorSet( extents, 640, 96, 640 );

	// Nav edit commands are only allowed in a local game, where the host is guaranteed to be in slot 0
	const playerState_t *ps = &g_clients[ 0 ].ps;

	glm::vec3 forward;
	AngleVectors( VEC2GLM( ps->viewangles ), &forward, nullptr, nullptr );

	glm::vec3 end = VEC2GLM( ps->origin ) + 8096.0f * forward;

	CM_BoxTrace( &trace, ps->origin, GLM4READ( end ), nullptr, nullptr, 0,
	             CONTENTS_SOLID | CONTENTS_PLAYERCLIP, 0, traceType_t::TT_AABB );

	rVec pos(VEC2GLM( trace.endpos ));
	if ( dtStatusFailed( nav->query->findNearestPoly( pos, extents, &filter, &nearRef, p ) ) )
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
	dtQueryFilter filter; // default include-all filter
} cmd;

static void BotDrawNavEdit( DebugDrawQuake *dd )
{
	rVec p;

	if ( cmd.enabled && GetPointPointedTo( cmd.nav, cmd.filter, p ) )
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

static void DrawPath( Bot_t *bot, DebugDrawQuake &dd )
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

// This draws a red box around every obstacle. Each face are crossed.
// The high number of line doesn't look too ugly, (IMO) convey well the idea
// that it is impossible to go there, are actually helpful because more lines
// means it will be more likely that one line will be displayed right
// (see https://github.com/Unvanquished/Unvanquished/issues/2337 for the bug)
static void BotDebugDrawObstacle( DebugDrawQuake &dd, const glm::vec3 qmins, const glm::vec3 qmaxs )
{
	rVec rmins(qmins);
	rVec rmaxs(qmaxs);

	dd.depthMask(false);
	const unsigned int color = duRGBA(255,128,128,255);
	dd.begin(DU_DRAW_LINES, 4.0f);

	// The 8 edges that the cube has
	rVec corners[] = {
		{ rmins[0], rmins[1], rmins[2] },
		{ rmaxs[0], rmins[1], rmins[2] },
		{ rmins[0], rmaxs[1], rmins[2] },
		{ rmaxs[0], rmaxs[1], rmins[2] },
		{ rmins[0], rmins[1], rmaxs[2] },
		{ rmaxs[0], rmins[1], rmaxs[2] },
		{ rmins[0], rmaxs[1], rmaxs[2] },
		{ rmaxs[0], rmaxs[1], rmaxs[2] },
	};

	// The indices of the edges that makes up the cube's 6 faces.
	// Each face has 4 edges.
	int squares[][4] = {
		{0, 1, 2, 3},
		{0, 1, 4, 5},
		{0, 1, 6, 7},
		{2, 3, 4, 5},
		{2, 3, 6, 7},
		{4, 5, 6, 7},
	};
	for ( int *sq : squares )
	{
		dd.vertex(corners[sq[0]][0], corners[sq[0]][1], corners[sq[0]][2], color);
		dd.vertex(corners[sq[1]][0], corners[sq[1]][1], corners[sq[1]][2], color);
		dd.vertex(corners[sq[2]][0], corners[sq[2]][1], corners[sq[2]][2], color);
		dd.vertex(corners[sq[3]][0], corners[sq[3]][1], corners[sq[3]][2], color);

		dd.vertex(corners[sq[0]][0], corners[sq[0]][1], corners[sq[0]][2], color);
		dd.vertex(corners[sq[2]][0], corners[sq[2]][1], corners[sq[2]][2], color);
		dd.vertex(corners[sq[1]][0], corners[sq[1]][1], corners[sq[1]][2], color);
		dd.vertex(corners[sq[3]][0], corners[sq[3]][1], corners[sq[3]][2], color);

		dd.vertex(corners[sq[0]][0], corners[sq[0]][1], corners[sq[0]][2], color);
		dd.vertex(corners[sq[3]][0], corners[sq[3]][1], corners[sq[3]][2], color);
		dd.vertex(corners[sq[1]][0], corners[sq[1]][1], corners[sq[1]][2], color);
		dd.vertex(corners[sq[2]][0], corners[sq[2]][1], corners[sq[2]][2], color);
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

	for ( auto &obstacle : savedObstacles )
	{
		BotDebugDrawObstacle( dd, obstacle.second.bbox.mins, obstacle.second.bbox.maxs );
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

class NaveditCmd : public Cmd::StaticCmd
{
public:
	NaveditCmd() : StaticCmd( "navedit", "view and edit bot navmeshes" ) {}

	void Run( const Cmd::Args &args ) const override
	{
		const char *arg = nullptr;
		const char usage[] = "Usage: navedit enable/disable/save <navmesh>";

		if ( args.Argc() < 2 )
		{
			Print( usage );
			return;
		}

		arg = args.Argv( 1 ).c_str();

		if ( !Q_stricmp( arg, "enable" ) )
		{
			int i;
			if ( args.Argc() < 3 )
			{
				Print( usage );
				return;
			}

			G_BotNavInit( 0 );
			if ( navMeshLoaded != navMeshStatus_t::LOADED )
			{
				Log::Warn( "Couldn't load navmeshes" );
				return;
			}

			arg = args.Argv( 2 ).c_str();
			for ( i = 0; i < numNavData; i++ )
			{
				if ( !Q_stricmp( BG_Class( BotNavData[ i ].species )->name, arg ) )
				{
					cmd.nav = &BotNavData[ i ];
					break;
				}
			}

			if ( i == numNavData )
			{
				Print( "\'%s\' is not a valid navmesh name", arg );
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
			Print( usage );
		}
	}
};

class AddconCmd: public Cmd::StaticCmd
{
public:
	AddconCmd() : StaticCmd( "addcon", "add navcon connection (during navedit)" ) {}

	void Run( const Cmd::Args &args ) const override
	{
		const char usage[] = "Usage: addcon start <dir> (<radius> (jetpack))\n"
		                     " addcon end";
		const char *arg = nullptr;

		if ( args.Argc() < 2 )
		{
			Print( usage );
			return;
		}

		arg = args.Argv( 1 ).c_str();

		if ( !Q_stricmp( arg, "start" ) )
		{
			if ( !cmd.enabled )
			{
				return;
			}

			if ( args.Argc() < 3 )
			{
				Print( usage );
				return;
			}

			arg = args.Argv( 2 ).c_str();

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
				Print( "Invalid argument for direction, specify oneway or twoway" );
				return;
			}

			if ( GetPointPointedTo( cmd.nav, cmd.filter, cmd.pc.start ) )
			{
				cmd.pc.area = DT_TILECACHE_WALKABLE_AREA;
				cmd.pc.flag = POLYFLAGS_WALK;
				cmd.pc.userid = 0;
				cmd.pc.radius = DEFAULT_CONNECTION_SIZE;

				if ( args.Argc() >= 4 )
				{
					float val = 0.f;
					if ( !Str::ToFloat( args.Argv( 3 ), val ) || val < 10.f )
					{
						Print( "Invalid argument for radius, must be 10 or more" );
						return;
					}
					cmd.pc.radius = val;
				}
				if ( args.Argc() == 5 )
				{
					// TODO: only allow this for the human_naked class
					const std::string &subArg = args.Argv( 4 );
					if ( Q_stricmp( subArg.c_str(), "jetpack" ) == 0 )
					{
						cmd.pc.flag = POLYFLAGS_JETPACK;
					}
					else
					{
						Print( "Invalid argument for flags, must be jetpack if specified" );
						return;
					}
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

			if ( GetPointPointedTo( cmd.nav, cmd.filter, cmd.pc.end ) )
			{
				cmd.nav->process.con.addConnection( cmd.pc );

				rVec boxMins, boxMaxs;
				for ( int i = 0; i < 3; i++ )
				{
					std::tie( boxMins[ i ], boxMaxs[ i ] ) = std::minmax( cmd.pc.start[ i ], cmd.pc.end[ i ] );
				}

				boxMins[ 1 ] -= 10;
				boxMaxs[ 1 ] += 10;

				// rebuild affected tiles
				dtCompressedTileRef refs[ 32 ];
				int tc = 0;
				cmd.nav->cache->queryTiles( boxMins, boxMaxs, refs, &tc, 32 );

				for ( int k = 0; k < tc; k++ )
				{
					cmd.nav->cache->buildNavMeshTile( refs[ k ], cmd.nav->mesh );
				}

				cmd.offBegin = false;
			}
		}
		else
		{
			Print( usage );
		}
	}
};

// return the navcon number and true if the targetPoint is at the navcon start,
// false if it is at the end
// return -1 and an unspecified boolean value if the targetPoint is at neither of them
static std::tuple< int, bool > findClosestNavcon( OffMeshConnections &cons, rVec &targetPoint )
{
	int resultIndex = -1;
	float resultDistanceSquare = std::numeric_limits<float>::max();
	bool resultIsStart = true;
	for ( int i = 0; i < cons.offMeshConCount; i++ )
	{
		int n = i * 6;
		// look at start and end points
		for ( int count = 0; count < 2; count++ )
		{
			rVec navconVertex = rVec::Load( &cons.verts[ n + count * 3 ] );
			float distanceSquare = DistanceSquared( targetPoint, navconVertex );
			if ( distanceSquare <= Square( cons.rad[ i ] ) && distanceSquare < resultDistanceSquare )
			{
				resultIndex = i;
				resultDistanceSquare = distanceSquare;
				resultIsStart = count == 0;
			}
		}
	}
	return std::tie( resultIndex, resultIsStart );
}

class ViewconCmd: public Cmd::StaticCmd
{
public:
	ViewconCmd() : StaticCmd( "viewcon", "display information about a navcon connection (during navedit)" ) {}

	void Run( const Cmd::Args &args ) const override
	{
		if ( args.Argc() != 1 )
		{
			PrintUsage( args, "" );
			return;
		}

		if ( !cmd.enabled )
		{
			return;
		}

		OffMeshConnections &cons = cmd.nav->process.con;
		rVec targetPoint;
		if ( GetPointPointedTo( cmd.nav, cmd.filter, targetPoint ) )
		{
			int i;
			bool isStart;
			std::tie( i, isStart ) = findClosestNavcon( cons, targetPoint );
			if ( i < 0 )
			{
				Print( "no navcon here" );
				return;
			}
			int n = i * 6;
			Print( "navcon %s^*, from ( %.0f, %.0f, %.0f ) to ( %.0f, %.0f, %.0f ), %s, radius %.0f%s",
			       isStart ? "^2start" : "^3end",
			       cons.verts[ n ], cons.verts[ n + 2 ], cons.verts[ n + 1 ],
			       cons.verts[ n + 3 ], cons.verts[ n + 5 ], cons.verts[ n + 4 ],
			       cons.dirs[ i ] == 0 ? "oneway" : "twoway", cons.rad[ i ],
			       cons.flags[ i ] == POLYFLAGS_JETPACK ? ", jetpack" : "");
		}
	}
};

class DelconCmd: public Cmd::StaticCmd
{
public:
	DelconCmd() : StaticCmd( "delcon", "delete a navcon connection (during navedit)" ) {}

	void Run( const Cmd::Args &args ) const override
	{
		if ( args.Argc() != 1 )
		{
			PrintUsage( args, "" );
			return;
		}

		if ( !cmd.enabled )
		{
			return;
		}

		OffMeshConnections &cons = cmd.nav->process.con;
		rVec targetPoint;
		if ( GetPointPointedTo( cmd.nav, cmd.filter, targetPoint ) )
		{
			int i;
			bool unused;
			std::tie( i, unused ) = findClosestNavcon( cons, targetPoint );
			
			if ( i < 0 )
			{
				Print( "no navcon here" );
				return;
			}

			int n = i * 6;
			rVec start = rVec::Load( &cons.verts[ n ] );
			rVec end = rVec::Load( &cons.verts[ n + 3 ] );

			cons.delConnection( i );

			rVec boxMins, boxMaxs;
			for ( int k = 0; k < 3; k++ )
			{
				std::tie( boxMins[ k ], boxMaxs[ k ] ) = std::minmax( start[ k ], end[ k ] );
			}

			boxMins[ 1 ] -= 10;
			boxMaxs[ 1 ] += 10;

			// rebuild affected tiles
			dtCompressedTileRef refs[ 32 ];
			int tc = 0;
			cmd.nav->cache->queryTiles( boxMins, boxMaxs, refs, &tc, 32 );

			for ( int k = 0; k < tc; k++ )
			{
				cmd.nav->cache->buildNavMeshTile( refs[ k ], cmd.nav->mesh );
			}
		}
	}
};

class NavtestCmd : public Cmd::StaticCmd
{
public:
	NavtestCmd() : StaticCmd( "navtest", "does something during navedit" ) {}

	void Run( const Cmd::Args &args ) const override
	{
		const char usage[] = "Usage: navtest shownodes/hidenodes/showportals/hideportals/startpath/endpath";

		if ( !cmd.enabled )
		{
			Print( "Can't test navmesh without enabling navedit" );
			return;
		}

		if ( args.Argc() < 2 )
		{
			Print( usage );
			return;
		}

		const char* arg = args.Argv( 1 ).c_str();

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
			if ( GetPointPointedTo( cmd.nav, cmd.filter, cmd.start ) )
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
			if ( GetPointPointedTo( cmd.nav, cmd.filter, end ) && cmd.validPathStart )
			{
				dtPolyRef startRef;
				dtPolyRef endRef;
				const int maxPath = 512;
				int npath;
				dtPolyRef path[ maxPath ];
				rVec nearPoint;
				rVec extents( 300, 300, 300 );

				cmd.nav->query->findNearestPoly( cmd.start, extents, &cmd.filter, &startRef, nearPoint );
				cmd.nav->query->findNearestPoly( end, extents, &cmd.filter, &endRef, nearPoint );

				cmd.nav->query->findPath( startRef, endRef, cmd.start, end, &cmd.filter, path, &npath, maxPath );
			}
		}
		else
		{
			Print( usage );
		}
	}
};

void BotRegisterNavEdit()
{
	if ( level.inClient )
	{
		static struct {
			NaveditCmd navedit;
			AddconCmd addcon;
			ViewconCmd viewcon;
			DelconCmd delcon;
			NavtestCmd navtest;
		} commandRegistration;
	}
}

void NavEditShutdown()
{
	if ( cmd.enabled )
	{
		Cvar::SetValue( "r_debugSurface", "0" );
	}

	cmd = {};
}
