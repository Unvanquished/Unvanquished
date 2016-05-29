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
#include "framework/CvarSystem.h"

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
                PrintUsage(args, "<mapname> (layoutname)", "loads a new map");
                return;
            }

            const std::string& mapName = args.Argv(1);

            //Make sure the map exists to avoid typos that would kill the game
            FS::GetAvailableMaps();
            const auto loadedPakInfo = FS::PakPath::LocateFile(Str::Format("maps/%s.bsp", mapName));
            if (!loadedPakInfo) {
                Print("Can't find map %s", mapName);
                return;
            }

            //Gets the layout list from the command but do not override if there is nothing
            std::string layouts = args.ConcatArgs(2);
            if (not layouts.empty()) {
                Cvar::SetValue("g_layouts", layouts);
            }

            Cvar::SetValueForce("sv_cheats", cheat ? "1" : "0");
            SV_SpawnServer(loadedPakInfo->name, mapName);
        }

        Cmd::CompletionResult Complete(int argNum, const Cmd::Args& args, Str::StringRef prefix) const OVERRIDE {
            if (argNum == 1) {
                Cmd::CompletionResult out;
                for (auto& map: FS::GetAvailableMaps()) {
                    if (Str::IsPrefix(prefix, map))
                        out.push_back({map, ""});
                }
                return out;
            } else if (argNum > 1) {
                return FS::HomePath::CompleteFilename(prefix, "game/layouts/" + args.Argv(1), ".dat", false, true);
            }

            return {};
        }

    private:
        bool cheat;
};
static MapCmd MapCmdRegistration("map", "starts a new map", false);
static MapCmd DevmapCmdRegistration("devmap", "starts a new map with cheats enabled", true);

void MSG_PrioritiseEntitystateFields();
void MSG_PrioritisePlayerStateFields();

static void SV_FieldInfo_f()
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
static void SV_MapRestart_f()
{
	int         i;
	client_t    *client;
	bool    denied;
	char        reason[ MAX_STRING_CHARS ];
	bool    isBot;

	// make sure we aren't restarting twice in the same frame
	if ( com_frameTime == sv.serverId )
	{
		return;
	}

	// make sure server is running
	if ( !com_sv_running->integer )
	{
		Log::Notice( "Server is not running.\n" );
		return;
	}

	// check for changes in variables that can't just be restarted
	// check for maxclients change
	if ( sv_maxclients->modified )
	{
		char mapname[ MAX_QPATH ];
		char pakname[ MAX_QPATH ];

		Log::Notice( "sv_maxclients variable change — restarting.\n" );
		// restart the map the slow way
		Q_strncpyz( mapname, Cvar_VariableString( "mapname" ), sizeof( mapname ) );
		Q_strncpyz( pakname, Cvar_VariableString( "pakname" ), sizeof( pakname ) );

		SV_SpawnServer(pakname, mapname);
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
	sv.state = serverState_t::SS_LOADING;
	sv.restarting = true;

	SV_RestartGameProgs();

	// run a few frames to allow everything to settle
	for ( i = 0; i < GAME_INIT_FRAMES; i++ )
	{
		gvm.GameRunFrame( sv.time );
		svs.time += FRAMETIME;
		sv.time += FRAMETIME;
	}

	// create a baseline for more efficient communications
	// Gordon: meh, this won't work here as the client doesn't know it has happened
//  SV_CreateBaseline ();

	sv.state = serverState_t::SS_GAME;
	sv.restarting = false;

	// connect and begin all the clients
	for ( i = 0; i < sv_maxclients->integer; i++ )
	{
		client = &svs.clients[ i ];

		// send the new gamestate to all connected clients
		if ( client->state < clientState_t::CS_CONNECTED )
		{
			continue;
		}

		isBot = SV_IsBot(client);

		// add the map_restart command
		SV_AddServerCommand( client, "map_restart\n" );

		// connect the client again, without the firstTime flag
		denied = gvm.GameClientConnect( reason, sizeof( reason ), i, false, isBot );

		if ( denied )
		{
			// this generally shouldn't happen, because the client
			// was connected before the level change
			SV_DropClient( client, reason );

			if ( !isBot )
			{
				Log::Notice( "SV_MapRestart_f: dropped client %i: denied!\n", i );
			}

			continue;
		}

		client->state = clientState_t::CS_ACTIVE;

		SV_ClientEnterWorld( client, &client->lastUsercmd );
	}

	// run another frame to allow things to look at all the players
	gvm.GameRunFrame( sv.time );
	svs.time += FRAMETIME;
	sv.time += FRAMETIME;
}

class StatusCmd: public Cmd::StaticCmd
{
public:
	StatusCmd():
		StaticCmd("status", Cmd::SYSTEM, "Shows a table with server and player information")
	{}

	void Run(const Cmd::Args&) const OVERRIDE
	{
		// make sure server is running
		if ( !com_sv_running->integer )
		{
			Log::Notice( "Server is not running.\n" );
			return;
		}

		float cpu = ( svs.stats.latched_active + svs.stats.latched_idle );

		if ( cpu )
		{
			cpu = 100 * svs.stats.latched_active / cpu;
		}

		auto players = std::count_if(
			svs.clients,
			svs.clients+sv_maxclients->integer,
			[](const client_t& cl) {
				return cl.state != clientState_t::CS_FREE;
			}
		);

		std::string time_string;
		auto seconds = svs.time / 1000 % 60;
		auto minutes = svs.time / 1000 / 60 % 60;
		if ( auto hours = svs.time / 1000 / 60 / 60 / 60 )
		{
			time_string = Str::Format("%02d:", hours);
		}
		time_string += Str::Format("%02d:%02d", minutes, seconds);

		Print(
			"(begin server status)\n"
			"hostname: %s\n"
			"version:  %s\n"
			"protocol: %d\n"
			"cpu:      %.0f%%\n"
			"time:     %s\n"
			"map:      %s\n"
			"players:  %d / %d\n"
			"num score connection address                port   name\n"
			"--- ----- ---------- ---------------------- ------ ----",
			sv_hostname->string,
			Q3_VERSION " on " Q3_ENGINE,
			PROTOCOL_VERSION,
			cpu,
			time_string,
			sv_mapname->string,
			players,
			sv_maxclients->integer
		);

		for ( int i = 0; i < sv_maxclients->integer; i++ )
		{
			const client_t& cl = svs.clients[i];
			if ( cl.state == clientState_t::CS_FREE )
			{
				continue;
			}

			std::string connection;

			if ( cl.state == clientState_t::CS_CONNECTED )
			{
				connection = "CONNECTED";
			}
			else if ( cl.state == clientState_t::CS_ZOMBIE )
			{
				connection = "ZOMBIE";
			}
			else
			{
				connection = std::to_string(cl.ping);
				if ( connection.size() > 10 )
				{
					connection = "ERROR";
				}
			}
			playerState_t* ps = SV_GameClientNum( i );

			const char *address = NET_AdrToString( cl.netchan.remoteAddress );

			Print(
				"%3i %5i %10s %-22s %-6i %s",
				i,
				ps->persistant[ PERS_SCORE ],
				connection,
				address,
				cl.netchan.qport,
				cl.name
			);
		}

		Print( "(end server status)" );
	}
};
static StatusCmd StatusCmdRegistration;


/*
==================
SV_Heartbeat_f

Also called by SV_DropClient, SV_DirectConnect, and SV_SpawnServer
==================
*/
void SV_Heartbeat_f()
{
	svs.nextHeartbeatTime = -9999999;
}

/*
===========
SV_Serverinfo_f

Examine the serverinfo string
===========
*/
static void SV_Serverinfo_f()
{
	// make sure server is running
	if ( !com_sv_running->integer )
	{
		Log::Notice( "Server is not running.\n" );
		return;
	}

	Log::Notice( "Server info settings:\n" );
	Info_Print( Cvar_InfoString( CVAR_SERVERINFO, false ) );
}

/*
===========
SV_Systeminfo_f

Examine the systeminfo string
===========
*/
static void SV_Systeminfo_f()
{
	// make sure server is running
	if ( !com_sv_running->integer )
	{
		Log::Notice( "Server is not running.\n" );
		return;
	}

	Log::Notice( "System info settings:\n" );
	Info_Print( Cvar_InfoString( CVAR_SYSTEMINFO, false ) );
}

class ListMapsCmd: public Cmd::StaticCmd
{
public:
	ListMapsCmd():
		StaticCmd("listmaps", Cmd::SYSTEM, "Lists all maps available to the server")
	{}

	void Run(const Cmd::Args&) const OVERRIDE
	{
		auto maps = FS::GetAvailableMaps();

		Print("Listing %d maps:", maps.size());

		for ( const auto& map: maps )
		{
			Print("%s", map.c_str());
		}
	}
};

#if BUILD_SERVER
static ListMapsCmd ListMapsCmdRegistration;
#endif

/*
==================
SV_AddOperatorCommands
==================
*/
void SV_AddOperatorCommands()
{
	if ( com_sv_running->integer )
	{
		// These commands should only be available while the server is running.
		Cmd_AddCommand( "fieldinfo",   SV_FieldInfo_f );
		Cmd_AddCommand( "heartbeat",   SV_Heartbeat_f );
		Cmd_AddCommand( "map_restart", SV_MapRestart_f );
		Cmd_AddCommand( "serverinfo",  SV_Serverinfo_f );
		Cmd_AddCommand( "systeminfo",  SV_Systeminfo_f );
	}
}

/*
==================
SV_RemoveOperatorCommands
==================
*/
void SV_RemoveOperatorCommands()
{
	Cmd_RemoveCommand( "fieldinfo" );
	Cmd_RemoveCommand( "heartbeat" );
	Cmd_RemoveCommand( "map_restart" );
	Cmd_RemoveCommand( "serverinfo" );
	Cmd_RemoveCommand( "status" );
	Cmd_RemoveCommand( "systeminfo" );
}
