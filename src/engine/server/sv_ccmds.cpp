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

#include "server.h"

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

class MapCmd: public Cmd::StaticCmd {
    public:
        MapCmd(Str::StringRef name, Str::StringRef description, bool cheat):
            Cmd::StaticCmd(name, Cmd::SYSTEM, description), cheat(cheat) {
        }

        void Run(const Cmd::Args& args) const OVERRIDE {
            if (args.Argc() < 2) {
                PrintUsage(args, _("<mapname> (layoutname)"), _("loads a new map"));
                return;
            }

            const std::string& mapName = args.Argv(1);

            //Make sure the map exists to avoid typos that would kill the game
            if (!FS::FindPak(va("map-%s", mapName.c_str()))) {
                Print(_("Can't find map %s"), mapName.c_str());
                return;
            }

            //Gets the layout list from the command but do not override if there is nothing
            std::string layouts = args.ConcatArgs(2);
            if (not layouts.empty()) {
                Cvar_Set("g_layouts", layouts.c_str());
            }

            SV_SpawnServer(mapName.c_str());
            Cvar_Set("sv_cheats", cheat ? "1" : "0");
        }

        Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, Str::StringRef prefix) const OVERRIDE {
            if (argNum == 1) {
                Cmd::CompletionResult out;
                for (auto& x: FS::GetAvailablePaks()) {
                    if (Str::IsPrefix("map-" + prefix, x.name))
                        out.push_back({x.name.substr(4), ""});
                }
                return out;
            } else if (argNum > 1) {
                return FS::HomePath::CompleteFilename(prefix, "layouts/" + args.Argv(1), ".dat", false, true);
            }

            return {};
        }

    private:
        bool cheat;
};
static MapCmd MapCmdRegistration("map", N_("starts a new map"), false);
static MapCmd DevmapCmdRegistration("devmap", N_("starts a new map with cheats enabled"), true);

void MSG_PrioritiseEntitystateFields( void );
void MSG_PrioritisePlayerStateFields( void );

static void SV_FieldInfo_f( void )
{
	MSG_PrioritiseEntitystateFields();
	MSG_PrioritisePlayerStateFields();
}

/*
================
SV_MapRestart_f

Completely restarts a level, but doesn't send a new gamestate to the clients.
This allows fair starts with variable load times.
================
*/
static void SV_MapRestart_f( void )
{
	int         i;
	client_t    *client;
	qboolean    denied;
	char        reason[ MAX_STRING_CHARS ];
	qboolean    isBot;
	int         delay = 0;

	// make sure we aren't restarting twice in the same frame
	if ( com_frameTime == sv.serverId )
	{
		return;
	}

	// make sure server is running
	if ( !com_sv_running->integer )
	{
		Com_Printf(_( "Server is not running.\n" ));
		return;
	}

	// ydnar: allow multiple delayed server restarts [atvi bug 3813]
	//% if ( sv.restartTime ) {
	//%     return;
	//% }

	if ( Cmd_Argc() > 1 )
	{
		delay = atoi( Cmd_Argv( 1 ) );
	}

	// check for changes in variables that can't just be restarted
	// check for maxclients change
	if ( sv_maxclients->modified )
	{
		char mapname[ MAX_QPATH ];

		Com_Printf(_( "sv_maxclients variable change — restarting.\n" ));
		// restart the map the slow way
		Q_strncpyz( mapname, Cvar_VariableString( "mapname" ), sizeof( mapname ) );

		SV_SpawnServer( mapname );
		return;
	}

	// toggle the server bit so clients can detect that a
	// map_restart has happened
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	// generate a new serverid
	// TTimo - don't update restartedserverId there, otherwise we won't deal correctly with multiple map_restart
	sv.serverId = com_frameTime;
	Cvar_Set( "sv_serverid", va( "%i", sv.serverId ) );

	// reset all the VM data in place without changing memory allocation
	// note that we do NOT set sv.state = SS_LOADING, so configstrings that
	// had been changed from their default values will generate broadcast updates
	sv.state = SS_LOADING;
	sv.restarting = qtrue;

	Cvar_Set( "sv_serverRestarting", "1" );

	SV_RestartGameProgs();

	// run a few frames to allow everything to settle
	for ( i = 0; i < GAME_INIT_FRAMES; i++ )
	{
		gvm->GameRunFrame( svs.time );
		svs.time += FRAMETIME;
	}

	// create a baseline for more efficient communications
	// Gordon: meh, this won't work here as the client doesn't know it has happened
//  SV_CreateBaseline ();

	sv.state = SS_GAME;
	sv.restarting = qfalse;

	// connect and begin all the clients
	for ( i = 0; i < sv_maxclients->integer; i++ )
	{
		client = &svs.clients[ i ];

		// send the new gamestate to all connected clients
		if ( client->state < CS_CONNECTED )
		{
			continue;
		}

		if ( client->netchan.remoteAddress.type == NA_BOT )
		{
			isBot = qtrue;
		}
		else
		{
			isBot = qfalse;
		}

		// add the map_restart command
		SV_AddServerCommand( client, "map_restart\n" );

		// connect the client again, without the firstTime flag
		denied = gvm->GameClientConnect( reason, sizeof( reason ), i, qfalse, isBot );

		if ( denied )
		{
			// this generally shouldn't happen, because the client
			// was connected before the level change
			SV_DropClient( client, reason );

			if ( !isBot )
			{
				Com_Printf( "SV_MapRestart_f(%d): dropped client %i: denied!\n", delay, i );  // bk010125
			}

			continue;
		}

		client->state = CS_ACTIVE;

		SV_ClientEnterWorld( client, &client->lastUsercmd );
	}

	// run another frame to allow things to look at all the players
	gvm->GameRunFrame( svs.time );
	svs.time += FRAMETIME;

	Cvar_Set( "sv_serverRestarting", "0" );
}

/*
================
SV_Status_f
================
*/
static void SV_Status_f( void )
{
	int           i, j, l;
	client_t      *cl;
	playerState_t *ps;
	const char    *s;
	int           ping;
	float         cpu, avg;

	// make sure server is running
	if ( !com_sv_running->integer )
	{
		Com_Printf(_( "Server is not running.\n" ));
		return;
	}

	cpu = ( svs.stats.latched_active + svs.stats.latched_idle );

	if ( cpu )
	{
		cpu = 100 * svs.stats.latched_active / cpu;
	}

	avg = 1000 * svs.stats.latched_active / STATFRAMES;

	Com_Printf( "cpu utilization  : %3i%%\n"
	            "avg response time: %i ms\n"
	            "map: %s\n"
	            "num score ping name            lastmsg address               qport rate\n"
	            "--- ----- ---- --------------- ------- --------------------- ----- -----\n",
	           ( int ) cpu, ( int ) avg, sv_mapname->string );

	for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if ( !cl->state )
		{
			continue;
		}

		Com_Printf( "%3i ", i );
		ps = SV_GameClientNum( i );
		Com_Printf( "%5i ", ps->persistant[ PERS_SCORE ] );

		if ( cl->state == CS_CONNECTED )
		{
			Com_Printf( "CNCT " );
		}
		else if ( cl->state == CS_ZOMBIE )
		{
			Com_Printf( "ZMBI " );
		}
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_Printf( "%4i ", ping );
		}

		Com_Printf( "%s", cl->name );
		l = 16 - strlen( cl->name );

		for ( j = 0; j < l; j++ )
		{
			Com_Printf( " " );
		}

		Com_Printf( "%7i ", svs.time - cl->lastPacketTime );

		s = NET_AdrToString( cl->netchan.remoteAddress );
		Com_Printf( "%s", s );
		l = 22 - strlen( s );

		for ( j = 0; j < l; j++ )
		{
			Com_Printf( " " );
		}

		Com_Printf( "%5i", cl->netchan.qport );

		Com_Printf( " %5i", cl->rate );

		Com_Printf( "\n" );
	}

	Com_Printf( "\n" );
}

/*
==================
SV_Heartbeat_f

Also called by SV_DropClient, SV_DirectConnect, and SV_SpawnServer
==================
*/
void SV_Heartbeat_f( void )
{
	svs.nextHeartbeatTime = -9999999;
}

/*
===========
SV_Serverinfo_f

Examine the serverinfo string
===========
*/
static void SV_Serverinfo_f( void )
{
	// make sure server is running
	if ( !com_sv_running->integer )
	{
		Com_Printf(_( "Server is not running.\n" ));
		return;
	}

	Com_Printf(_( "Server info settings:\n" ));
	Info_Print( Cvar_InfoString( CVAR_SERVERINFO, qfalse ) );
}

/*
===========
SV_Systeminfo_f

Examine the systeminfo string
===========
*/
static void SV_Systeminfo_f( void )
{
	// make sure server is running
	if ( !com_sv_running->integer )
	{
		Com_Printf(_( "Server is not running.\n" ));
		return;
	}

	Com_Printf(_( "System info settings:\n" ));
	Info_Print( Cvar_InfoString( CVAR_SYSTEMINFO, qfalse ) );
}

/*
=================
SV_KillServer
=================
*/
static void SV_KillServer_f( void )
{
	SV_Shutdown( "killserver" );
}

/*
==================
SV_AddOperatorCommands
==================
*/
void SV_AddOperatorCommands( void )
{
	if ( com_sv_running->integer )
	{
		// These commands should only be available while the server is running.
		Cmd_AddCommand( "fieldinfo",   SV_FieldInfo_f );
		Cmd_AddCommand( "heartbeat",   SV_Heartbeat_f );
		Cmd_AddCommand( "killserver",  SV_KillServer_f );
		Cmd_AddCommand( "map_restart", SV_MapRestart_f );
		Cmd_AddCommand( "sectorlist",  SV_SectorList_f );
		Cmd_AddCommand( "serverinfo",  SV_Serverinfo_f );
		Cmd_AddCommand( "status",      SV_Status_f );
		Cmd_AddCommand( "systeminfo",  SV_Systeminfo_f );
	}
}

/*
==================
SV_RemoveOperatorCommands
==================
*/
void SV_RemoveOperatorCommands( void )
{
	Cmd_RemoveCommand( "dumpuser" );
	Cmd_RemoveCommand( "fieldinfo" );
	Cmd_RemoveCommand( "heartbeat" );
	Cmd_RemoveCommand( "killserver" );
	Cmd_RemoveCommand( "map_restart" );
	Cmd_RemoveCommand( "say" );
	Cmd_RemoveCommand( "sectorlist" );
	Cmd_RemoveCommand( "serverinfo" );
	Cmd_RemoveCommand( "status" );
	Cmd_RemoveCommand( "systeminfo" );
}
