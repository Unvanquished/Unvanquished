/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// this file holds commands that can be executed by the server console, but not remote clients

#include "sg_local.h"

#define IS_NON_NULL_VEC3(vec3tor) (vec3tor[0] || vec3tor[1] || vec3tor[2])

void Svcmd_EntityFire_f()
{
	char argument[ MAX_STRING_CHARS ];
	int  entityNum;
	gentity_t *selection;
	gentityCall_t call;
	gentityCallDefinition_t callDefinition = { nullptr, ON_DEFAULT, nullptr, nullptr, ECA_DEFAULT };

	if ( trap_Argc() < 2 || trap_Argc() > 3 )
	{
		Log::Notice( "usage: entityFire <entityNum> [<action>]" );
		return;
	}

	trap_Argv( 1, argument, sizeof( argument ) );
	entityNum = atoi( argument );

	if ( entityNum >= level.num_entities || entityNum < MAX_CLIENTS )
	{
		Log::Notice( "invalid entityId %d", entityNum );
		return;
	}

	selection = &g_entities[entityNum];

	if (!selection->inuse)
	{
		Log::Notice("entity slot %d is not in use", entityNum);
		return;
	}

	if( trap_Argc() >= 3 )
	{
		trap_Argv( 2, argument, sizeof( argument ) );
		callDefinition.action = argument;
		callDefinition.actionType = G_GetCallActionTypeFor( callDefinition.action );
	}

	Log::Notice( "firing %s:%s", etos( selection ), callDefinition.action ? callDefinition.action : "default" );

	if(selection->names[0])
		callDefinition.name = selection->names[0];

	call.definition = &callDefinition;
	call.caller = &g_entities[ ENTITYNUM_NONE ];
	call.activator = &g_entities[ ENTITYNUM_NONE ] ;

	G_CallEntity(selection, &call);
}


static inline void PrintEntityOverviewLine( gentity_t *entity )
{
	Log::Notice( "%3i: %15s/^5%-24s^*%s%s",
			entity->s.number, Com_EntityTypeName( entity->s.eType ), entity->classname,
			entity->names[0] ? entity->names[0] : "", entity->names[1] ? " …" : "");
}

/*
===================
Svcmd_EntityShow_f
===================
*/
void Svcmd_EntityShow_f()
{
	int       entityNum;
	int       lastTargetIndex, targetIndex;
	gentity_t *selection;
	gentity_t *possibleTarget = nullptr;
	char argument[ 6 ];


	if (trap_Argc() != 2)
	{
		Log::Notice("usage: entityShow <entityId>");
		return;
	}

	trap_Argv( 1, argument, sizeof( argument ) );
	entityNum = atoi( argument );

	if (entityNum >= level.num_entities || entityNum < MAX_CLIENTS)
	{
		Log::Notice("entityId %d is out of range", entityNum);
		return;
	}

	selection = &g_entities[entityNum];

	if (!selection->inuse)
	{
		Log::Notice("entity slot %d is unused/free", entityNum);
		return;
	}

	Log::Notice( "⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼" );
	Log::Notice( "^5#%3i^*: %16s", entityNum, Com_EntityTypeName( selection->s.eType ) );
	if (IS_NON_NULL_VEC3(selection->s.origin))
	{
		Log::Notice("%26s", vtos( selection->s.origin ) );
	}
	Log::Notice( "\n⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼⎼" );
	Log::Notice( "Classname: ^5%s^*", selection->classname );
	Log::Notice( "Capabilities:%s%s%s%s%s%s%s\n",
			selection->act ? " acts" : "",
			selection->think ? " thinks" : "",
			selection->pain ? " pains" : "",
			selection->die ? " dies" : "",
			selection->reset ? " resets" : "",
			selection->touch ? " touchable" : "",
			selection->use ? " usable" : "");
	if (selection->names[0])
	{
		Log::Notice( "Names: ");
		G_PrintEntityNameList( selection );
	}

	Log::Notice("State: %s", selection->enabled ? "enabled" : "disabled");

	if (selection->groupName)
	{
		Log::Notice("Member of Group: %s%s", selection->groupName, !selection->groupMaster ? " [master]" : "");
	}

	Log::Notice( "");

	if(selection->targetCount)
	{
		Log::Notice( "Aims at");

		while ((possibleTarget = G_IterateTargets(possibleTarget, &targetIndex, selection)) != nullptr )
		{
			Log::Notice(" • %s %s", etos( possibleTarget ), vtos( possibleTarget->s.origin));
		}
		Log::Notice( "");
	}

	if(selection->callTargetCount)
	{
		lastTargetIndex = -1;
		while ((possibleTarget = G_IterateCallEndpoints(possibleTarget, &targetIndex, selection)) != nullptr )
		{

			if(lastTargetIndex != targetIndex)
			{
				Log::Notice("Calls %s \"%s:%s\"",
						selection->calltargets[ targetIndex ].event ? selection->calltargets[ targetIndex ].event : "onUnknown",
						selection->calltargets[ targetIndex ].name,
						selection->calltargets[ targetIndex ].action ? selection->calltargets[ targetIndex ].action : "default");
				lastTargetIndex = targetIndex;
			}

			Log::Notice(" • %s", etos(possibleTarget));
			if(possibleTarget->names[1])
			{
				Log::Notice(" using \"%s\" ∈ ", selection->calltargets[ targetIndex ].name);
				G_PrintEntityNameList( possibleTarget );
			}
			Log::Notice("");
		}
	}
	Log::Notice( "" );
}

/*
===================
Svcmd_EntityList_f
===================
*/

void  Svcmd_EntityList_f()
{
	int       entityNum;
	int i;
	int currentEntityCount;
	gentity_t *displayedEntity;
	char* filter;

	displayedEntity = g_entities;

	if(trap_Argc() > 1)
	{
		filter = ConcatArgs( 1 );
	}
	else
	{
		filter = nullptr;
	}

	for ( entityNum = 0, currentEntityCount = 0; entityNum < level.num_entities; entityNum++, displayedEntity++ )
	{
		if ( !displayedEntity->inuse )
		{
			continue;
		}
		currentEntityCount++;

		if(filter && !Com_Filter(filter, displayedEntity->classname, false) )
		{
			for (i = 0; i < MAX_ENTITY_ALIASES && displayedEntity->names[i]; ++i)
			{
				if( Com_Filter(filter, displayedEntity->names[i], false) )
				{
					PrintEntityOverviewLine( displayedEntity );
					break;
				}
			}
			continue;
		}
		PrintEntityOverviewLine( displayedEntity );
	}

	Log::Notice( "A total of %i entities are currently in use.", currentEntityCount);
}

static gclient_t *ClientForString( char *s )
{
	int  idnum;
	char err[ MAX_STRING_CHARS ];

	idnum = G_ClientNumberFromString( s, err, sizeof( err ) );

	if ( idnum == -1 )
	{
		Log::Notice( err );
		return nullptr;
	}

	return &level.clients[ idnum ];
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
static void Svcmd_ForceTeam_f()
{
	gclient_t *cl;
	char      str[ MAX_TOKEN_CHARS ];
	team_t    team;

	if ( trap_Argc() != 3 )
	{
		Log::Notice( "usage: forceteam <player> <team>" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );

	if ( !cl )
	{
		return;
	}

	trap_Argv( 2, str, sizeof( str ) );
	team = G_TeamFromString( str );

	if ( team == NUM_TEAMS )
	{
		Log::Notice( "forceteam: invalid team \"%s\"", str );
		return;
	}

	G_ChangeTeam( &g_entities[ cl - level.clients ], team );
}

/*
===================
Svcmd_LayoutSave_f

layoutsave <name>
===================
*/
static void Svcmd_LayoutSave_f()
{
	char str[ MAX_QPATH ];
	char str2[ MAX_QPATH - 4 ];
	char *s;
	int  i = 0;

	if ( trap_Argc() != 2 )
	{
		Log::Notice( "usage: layoutsave <name>" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	// sanitize name
	s = &str[ 0 ];
	str2[ 0 ] = 0;

	while ( *s && i < (int) sizeof( str2 ) - 1 )
	{
		if ( Str::cisalnum( *s ) || *s == '-' || *s == '_' )
		{
			str2[ i++ ] = *s;
			str2[ i ] = '\0';
		}

		s++;
	}

	if ( !str2[ 0 ] )
	{
		Log::Notice( "layoutsave: invalid name \"%s\"", str );
		return;
	}

	G_LayoutSave( str2 );
}

char *ConcatArgs( int start );

/*
===================
Svcmd_LayoutLoad_f

layoutload [<name> [<name2> [<name3 [...]]]]

This is just a silly alias for doing:
 set g_layouts "name name2 name3"
 map_restart
===================
*/
static void Svcmd_LayoutLoad_f()
{
	char layouts[ MAX_CVAR_VALUE_STRING ];
	char *s;

	if ( trap_Argc() < 2 )
	{
		Log::Notice( "usage: layoutload <name> …" );
		return;
	}

	s = ConcatArgs( 1 );
	Q_strncpyz( layouts, s, sizeof( layouts ) );
	g_layouts.Set(layouts);
	trap_SendConsoleCommand( "map_restart\n" );
	level.restarted = true;
}

static void Svcmd_AdmitDefeat_f()
{
	int  team;
	char teamNum[ 2 ];

	if ( trap_Argc() != 2 )
	{
		Log::Notice( "admitdefeat: must provide a team" );
		return;
	}

	trap_Argv( 1, teamNum, sizeof( teamNum ) );
	team = G_TeamFromString( teamNum );

	if ( team == TEAM_ALIENS )
	{
		G_TeamCommand( TEAM_ALIENS, "cp \"Hivemind Link Broken\" 1" );
		trap_SendServerCommand( -1, "print_tr \"" N_("Alien team has admitted defeat\n") "\"" );
	}
	else if ( team == TEAM_HUMANS )
	{
		G_TeamCommand( TEAM_HUMANS, "cp \"Life Support Terminated\" 1" );
		trap_SendServerCommand( -1, "print_tr \"" N_("Human team has admitted defeat\n") "\"" );
	}
	else
	{
		Log::Notice( "admitdefeat: invalid team" );
		return;
	}

	level.surrenderTeam = (team_t) team;
	G_BaseSelfDestruct( (team_t) team );
}

static void Svcmd_TeamWin_f()
{
	// this is largely made redundant by admitdefeat <team>
	char cmd[ 6 ];
	team_t team;
	trap_Argv( 0, cmd, sizeof( cmd ) );

	team = G_TeamFromString( cmd );

	if ( TEAM_ALIENS == team )
	{
		G_BaseSelfDestruct( TEAM_HUMANS );
	}
	if ( TEAM_HUMANS == team )
	{
		G_BaseSelfDestruct( TEAM_ALIENS );
	}
}

static void Svcmd_Evacuation_f()
{
	trap_SendServerCommand( -1, "print_tr \"" N_("Evacuation ordered\n") "\"" );
	level.lastWin = TEAM_NONE;
	trap_SetConfigstring( CS_WINNER, "Evacuation" );
	G_notify_sensor_end( TEAM_NONE );
	LogExit( "Evacuation." );
	G_MapLog_Result( 'd' );
}

static void Svcmd_MapRotation_f()
{
	char rotationName[ MAX_QPATH ];

	if ( trap_Argc() != 2 )
	{
		Log::Notice( "usage: maprotation <name>" );
		return;
	}

	G_ClearRotationStack();

	trap_Argv( 1, rotationName, sizeof( rotationName ) );

	if ( !G_StartMapRotation( rotationName, false, true, false, 0 ) )
	{
		Log::Notice( "maprotation: invalid map rotation \"%s\"", rotationName );
	}
}

static void Svcmd_TeamMessage_f()
{
	char   teamNum[ 2 ];
	team_t team;
	char   *arg;

	if ( trap_Argc() < 3 )
	{
		Log::Notice( "usage: say_team <team> <message>" );
		return;
	}

	trap_Argv( 1, teamNum, sizeof( teamNum ) );
	team = G_TeamFromString( teamNum );

	if ( team == NUM_TEAMS )
	{
		Log::Notice( "say_team: invalid team \"%s\"", teamNum );
		return;
	}

	arg = ConcatArgs( 2 );
	G_TeamCommand( team, va( "chat -1 %d %s", SAY_TEAM, Quote( arg ) ) );
	G_LogPrintf( "SayTeam: -1 \"console\": %s\n", arg );
}

static void Svcmd_CenterPrint_f()
{
	if ( trap_Argc() < 2 )
	{
		Log::Notice( "usage: cp <message>" );
		return;
	}

	trap_SendServerCommand( -1, va( "cp %s", Quote( ConcatArgs( 1 ) ) ) );
}

static void Svcmd_EjectClient_f()
{
	char *reason, name[ MAX_STRING_CHARS ];

	if ( trap_Argc() < 2 )
	{
		Log::Notice( "usage: eject <player|-1> <reason>" );
		return;
	}

	trap_Argv( 1, name, sizeof( name ) );
	reason = ConcatArgs( 2 );

	if ( atoi( name ) == -1 )
	{
		int i;

		for ( i = 0; i < level.maxclients; i++ )
		{
			if ( level.clients[ i ].pers.connected == CON_DISCONNECTED )
			{
				continue;
			}

			if ( level.clients[ i ].pers.localClient )
			{
				continue;
			}

			trap_DropClient( i, reason );
		}
	}
	else
	{
		gclient_t *cl = ClientForString( name );

		if ( !cl )
		{
			return;
		}

		if ( cl->pers.localClient )
		{
			Log::Notice( "eject: cannot eject local clients" );
			return;
		}

		trap_DropClient( cl - level.clients, reason );
	}
}

static void Svcmd_DumpUser_f()
{
	char       name[ MAX_STRING_CHARS ], userinfo[ MAX_INFO_STRING ];
	char       key[ BIG_INFO_KEY ], value[ BIG_INFO_VALUE ];
	const char *info;
	gclient_t  *cl;

	if ( trap_Argc() != 2 )
	{
		Log::Notice( "usage: dumpuser <player>" );
		return;
	}

	trap_Argv( 1, name, sizeof( name ) );
	cl = ClientForString( name );

	if ( !cl )
	{
		return;
	}

	trap_GetUserinfo( cl - level.clients, userinfo, sizeof( userinfo ) );
	info = &userinfo[ 0 ];
	Log::Notice( "userinfo\n--------" );

	//Info_Print( userinfo );
	while ( 1 )
	{
		Info_NextPair( &info, key, value );

		if ( !*info )
		{
			return;
		}

		Log::Notice( "%-20s%s", key, value );
	}
}

static void Svcmd_Pr_f()
{
	char targ[ 4 ];
	int  cl;

	if ( trap_Argc() < 3 )
	{
		Log::Notice( "usage: <clientnum|-1> <message>" );
		return;
	}

	trap_Argv( 1, targ, sizeof( targ ) );
	cl = atoi( targ );

	if ( cl >= MAX_CLIENTS || cl < -1 )
	{
		Log::Notice( "invalid clientnum %d", cl );
		return;
	}

	trap_SendServerCommand( cl, va( "print %s\\\n", Quote( ConcatArgs( 2 ) ) ) );
}

static void Svcmd_PrintQueue_f()
{
	team_t team;
	char teamName[ MAX_STRING_CHARS ];

	if ( trap_Argc() != 2 )
	{
		Log::Notice( "usage: printqueue <team>" );
		return;
	}

	trap_Argv( 1, teamName, sizeof( teamName ) );

	team = G_TeamFromString(teamName);
	if ( G_IsPlayableTeam( team ) )
	{
		G_PrintSpawnQueue( &level.team[ team ].spawnQueue );
	}
	else
	{
		Log::Notice( "unknown team" );
	}
}

// dumb wrapper for "a", "m", "chat", and "say"
static void Svcmd_MessageWrapper()
{
	char cmd[ 5 ];
	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( !Q_stricmp( cmd, "a" ) )
	{
		Cmd_AdminMessage_f( nullptr );
	}
	else if ( !Q_stricmp( cmd, "asay" ) )
	{
		G_Say( nullptr, SAY_ALL_ADMIN, ConcatArgs( 1 ) );
	}
	else if ( !Q_stricmp( cmd, "m" ) )
	{
		Cmd_PrivateMessage_f( nullptr );
	}
	else if ( !Q_stricmp( cmd, "say" ) )
	{
		G_Say( nullptr, SAY_ALL, ConcatArgs( 1 ) );
	}
	else if ( !Q_stricmp( cmd, "chat" ) )
	{
		G_Say( nullptr, SAY_RAW, ConcatArgs( 1 ) );
	}
}

static void Svcmd_MapLogWrapper()
{
	Cmd_MapLog_f( nullptr );
}

static void Svcmd_G_AdvanceMapRotation_f()
{
	G_AdvanceMapRotation( 0 );
}

static const struct svcmd
{
	const char *cmd;
	bool conflicts; //With a command registered by cgame
	void ( *function )();
} svcmds[] =
{
	{ "a",                  true,  Svcmd_MessageWrapper         },
	{ "admitDefeat",        false, Svcmd_AdmitDefeat_f          },
	{ "advanceMapRotation", false, Svcmd_G_AdvanceMapRotation_f },
	{ "alienWin",           false, Svcmd_TeamWin_f              },
	{ "asay",               true,  Svcmd_MessageWrapper         },
	{ "chat",               true,  Svcmd_MessageWrapper         },
	{ "cp",                 true,  Svcmd_CenterPrint_f          },
	{ "dumpuser",           false, Svcmd_DumpUser_f             },
	{ "eject",              false, Svcmd_EjectClient_f          },
	{ "entityFire",         false, Svcmd_EntityFire_f           },
	{ "entityList",         false, Svcmd_EntityList_f           },
	{ "entityShow",         false, Svcmd_EntityShow_f           },
	{ "evacuation",         false, Svcmd_Evacuation_f           },
	{ "forceTeam",          false, Svcmd_ForceTeam_f            },
	{ "humanWin",           false, Svcmd_TeamWin_f              },
	{ "layoutLoad",         false, Svcmd_LayoutLoad_f           },
	{ "layoutSave",         false, Svcmd_LayoutSave_f           },
	{ "m",                  true,  Svcmd_MessageWrapper         },
	{ "maplog",             true,  Svcmd_MapLogWrapper          },
	{ "mapRotation",        false, Svcmd_MapRotation_f          },
	{ "pr",                 false, Svcmd_Pr_f                   },
	{ "printqueue",         false, Svcmd_PrintQueue_f           },
	{ "say",                true,  Svcmd_MessageWrapper         },
	{ "say_team",           true,  Svcmd_TeamMessage_f          },
	{ "stopMapRotation",    false, G_StopMapRotation            },
};

/*
=================
ConsoleCommand

=================
*/
bool  ConsoleCommand()
{
	char         cmd[ MAX_TOKEN_CHARS ];
	struct svcmd *command;

	trap_Argv( 0, cmd, sizeof( cmd ) );

	command = (struct svcmd*) bsearch( cmd, svcmds, ARRAY_LEN( svcmds ),
	                   sizeof( struct svcmd ), cmdcmp );

	if ( !command )
	{
		// see if this is an admin command
		if ( G_admin_cmd_check( nullptr ) )
		{
			return true;
		}

		if ( level.inClient )
		{
			Log::Warn( "unknown command server console command: %s", cmd );
		}

		return false;
	}

	if ( command->conflicts && level.inClient )
	{
		return false;
	}

	command->function();
	return true;
}

void CompleteCommand(int)
{
}

void G_RegisterCommands()
{
	for ( unsigned i = 0; i < ARRAY_LEN( svcmds ); i++ )
	{
		if ( svcmds[ i ].conflicts && level.inClient )
		{
			continue;
		}

		trap_AddCommand( svcmds[ i ].cmd );
	}

	G_admin_register_cmds();
}

void G_UnregisterCommands()
{
	for ( unsigned i = 0; i < ARRAY_LEN( svcmds ); i++ )
	{
		if ( svcmds[ i ].conflicts && level.inClient )
		{
			continue;
		}

		trap_RemoveCommand( svcmds[ i ].cmd );
	}

	G_admin_unregister_cmds();
}
