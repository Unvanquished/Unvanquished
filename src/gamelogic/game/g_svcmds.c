/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"


static void PrintEntityType( gentity_t *entity )
{
	if(!entity)
		return;

	switch ( entity->s.eType )
	{
		case ET_GENERAL:
			G_Printf( "ET_GENERAL          " );
			break;

		case ET_PLAYER:
			G_Printf( "ET_PLAYER           " );
			break;

		case ET_ITEM:
			G_Printf( "ET_ITEM             " );
			break;

		case ET_BUILDABLE:
			G_Printf( "ET_BUILDABLE        " );
			break;

		case ET_LOCATION:
			G_Printf( "ET_LOCATION         " );
			break;

		case ET_MISSILE:
			G_Printf( "ET_MISSILE          " );
			break;

		case ET_MOVER:
			G_Printf( "ET_MOVER            " );
			break;

		case ET_BEAM:
			G_Printf( "ET_BEAM             " );
			break;

		case ET_PORTAL:
			G_Printf( "ET_PORTAL           " );
			break;

		case ET_SPEAKER:
			G_Printf( "ET_SPEAKER          " );
			break;

		case ET_PUSH_TRIGGER:
			G_Printf( "ET_PUSH_TRIGGER     " );
			break;

		case ET_TELEPORT_TRIGGER:
			G_Printf( "ET_TELEPORT_TRIGGER " );
			break;

		case ET_INVISIBLE:
			G_Printf( "ET_INVISIBLE        " );
			break;

		case ET_GRAPPLE:
			G_Printf( "ET_GRAPPLE          " );
			break;

		case ET_CORPSE:
			G_Printf( "ET_CORPSE           " );
			break;

		case ET_PARTICLE_SYSTEM:
			G_Printf( "ET_PARTICLE_SYSTEM  " );
			break;

		case ET_ANIMMAPOBJ:
			G_Printf( "ET_ANIMMAPOBJ       " );
			break;

		case ET_MODELDOOR:
			G_Printf( "ET_MODELDOOR        " );
			break;

		case ET_LIGHTFLARE:
			G_Printf( "ET_LIGHTFLARE       " );
			break;

		case ET_LEV2_ZAP_CHAIN:
			G_Printf( "ET_LEV2_ZAP_CHAIN   " );
			break;

		default:
			G_Printf( "%-3i                 ", entity->s.eType );
			break;
	}
}

void Svcmd_EntityFire_f( void )
{
	char argument[ 4 ];
	int  entityId;
	int  event;
	gentity_t *selection;

	if ( trap_Argc() != 3 )
	{
		G_Printf( "usage: entityFire <entityNum> <event>\n" );
		return;
	}

	trap_Argv( 1, argument, sizeof( argument ) );
	entityId = atoi( argument );

	if ( entityId >= level.num_entities || entityId < MAX_CLIENTS )
	{
		G_Printf( "invalid entityId %d\n", entityId );
		return;
	}

	selection = &g_entities[entityId];

	if (!selection || !selection->inuse)
	{
		G_Printf("entity slot %d is unused/free\n", entityId);
		return;
	}

	trap_Argv( 2, argument, sizeof( argument ) );
	event = atoi( argument );

	G_Printf( "firing %s/%-3i with %i event\n", selection->classname, entityId, event);
	G_FireAllTargetsOf( selection, &g_entities[ ENTITYNUM_NONE ]);

}


/*
===================
Svcmd_EntityShow_f
===================
*/
void Svcmd_EntityShow_f( void )
{
	int       i, entityId;
	gentity_t *selection;
	char argument[ 4 ];


	if (trap_Argc() != 2)
	{
		G_Printf("usage: entityShow <entityId>\n");
		return;
	}

	trap_Argv( 1, argument, sizeof( argument ) );
	entityId = atoi( argument );

	if (entityId >= level.num_entities || entityId < MAX_CLIENTS)
	{
		G_Printf("entityId %d is out of range\n", entityId);
		return;
	}

	selection = &g_entities[entityId];

	if (!selection || !selection->inuse)
	{
		G_Printf("entity slot %d is unused/free\n", entityId);
		return;
	}

	G_Printf( "------------------------\n" );
	G_Printf( "%3i:", entityId );
	PrintEntityType( selection );
	G_Printf( "\n------------------------\n" );
	G_Printf( "Classname: %s\n", selection->classname );
	G_Printf( "Capabilities: %s%s%s\n", selection->think ? "thinks " : "", selection->touch ? "touchable " : "", selection->use ? "useable " : "");

	if (selection->names[0])
	{
		G_Printf( "Name:");
		for( i = 0; selection->names[ i ]; ++i )
			G_Printf("%s %s", ( i == 0 ? "" : "," ), selection->names[i]);
		G_Printf( "\n" );
	}

	if (selection->groupName)
	{
		G_Printf("Member of Group: %s %s\n", selection->groupName, !selection->groupMaster ? "[master]" : "");
	}

	if(selection->targets[0].name)
	{
		G_Printf( "Targets:\n");
		for( i = 0; selection->targets[ i ].name; ++i )
			G_Printf( " ⇨ %s :%s\n", selection->targets[ i ].name, selection->targets[ i ].action ? selection->targets[ i ].action : "default" );
	}
}

/*
===================
Svcmd_EntityList_f
===================
*/
void  Svcmd_EntityList_f( void )
{
	int       entity;
	int currentEntityCount;
	gentity_t *check;

	check = g_entities;

	for ( entity = 0, currentEntityCount = 0; entity < level.num_entities; entity++, check++ )
	{
		if ( !check->inuse )
		{
			continue;
		}

		currentEntityCount++;

		G_Printf( "%3i:", entity );
		PrintEntityType( check );

		if ( check->classname )
		{
			G_Printf( "%-24s ", check->classname );
			if ( check->names[0] )
			{
				G_Printf( "\"%s\"", check->names[0] );
				if ( check->names[1] )
				{
					G_Printf( ", \"%s\"", check->names[1] );
				}
			}
		}

		G_Printf( "\n" );
	}

	G_Printf( "A total of %i entities are currently in use.\n", currentEntityCount);
}

static gclient_t *ClientForString( char *s )
{
	int  idnum;
	char err[ MAX_STRING_CHARS ];

	idnum = G_ClientNumberFromString( s, err, sizeof( err ) );

	if ( idnum == -1 )
	{
		G_Printf( "%s", err );
		return NULL;
	}

	return &level.clients[ idnum ];
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
static void Svcmd_ForceTeam_f( void )
{
	gclient_t *cl;
	char      str[ MAX_TOKEN_CHARS ];
	team_t    team;

	if ( trap_Argc() != 3 )
	{
		G_Printf( "usage: forceteam <player> <team>\n" );
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
		G_Printf( "forceteam: invalid team \"%s\"\n", str );
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
static void Svcmd_LayoutSave_f( void )
{
	char str[ MAX_QPATH ];
	char str2[ MAX_QPATH - 4 ];
	char *s;
	int  i = 0;

	if ( trap_Argc() != 2 )
	{
		G_Printf( "usage: layoutsave <name>\n" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	// sanitize name
	s = &str[ 0 ];

	while ( *s && i < sizeof( str2 ) - 1 )
	{
		if ( isalnum( *s ) || *s == '-' || *s == '_' )
		{
			str2[ i++ ] = *s;
			str2[ i ] = '\0';
		}

		s++;
	}

	if ( !str2[ 0 ] )
	{
		G_Printf( "layoutsave: invalid name \"%s\"\n", str );
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
static void Svcmd_LayoutLoad_f( void )
{
	char layouts[ MAX_CVAR_VALUE_STRING ];
	char *s;

	if ( trap_Argc() < 2 )
	{
		G_Printf( "usage: layoutload <name> …\n" );
		return;
	}

	s = ConcatArgs( 1 );
	Q_strncpyz( layouts, s, sizeof( layouts ) );
	trap_Cvar_Set( "g_layouts", layouts );
	trap_SendConsoleCommand( EXEC_APPEND, "map_restart\n" );
	level.restarted = qtrue;
}

static void Svcmd_AdmitDefeat_f( void )
{
	int  team;
	char teamNum[ 2 ];

	if ( trap_Argc() != 2 )
	{
		G_Printf( "admitdefeat: must provide a team\n" );
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
		G_Printf( "admitdefeat: invalid team\n" );
		return;
	}

	level.surrenderTeam = team;
	G_BaseSelfDestruct( team );
}

static void Svcmd_TeamWin_f( void )
{
	// this is largely made redundant by admitdefeat <team>
	char cmd[ 6 ];
	trap_Argv( 0, cmd, sizeof( cmd ) );

	switch ( G_TeamFromString( cmd ) )
	{
		case TEAM_ALIENS:
			G_BaseSelfDestruct( TEAM_HUMANS );
			break;

		case TEAM_HUMANS:
			G_BaseSelfDestruct( TEAM_ALIENS );
			break;

		default:
			return;
	}
}

static void Svcmd_Evacuation_f( void )
{
	trap_SendServerCommand( -1, "print_tr \"" N_("Evacuation ordered\n") "\"" );
	level.lastWin = TEAM_NONE;
	trap_SetConfigstring( CS_WINNER, "Evacuation" );
	G_notify_sensor_end( TEAM_NONE );
	LogExit( "Evacuation." );
	G_MapLog_Result( 'd' );
}

static void Svcmd_MapRotation_f( void )
{
	char rotationName[ MAX_QPATH ];

	if ( trap_Argc() != 2 )
	{
		G_Printf( "usage: maprotation <name>\n" );
		return;
	}

	G_ClearRotationStack();

	trap_Argv( 1, rotationName, sizeof( rotationName ) );

	if ( !G_StartMapRotation( rotationName, qfalse, qtrue, qfalse, 0 ) )
	{
		G_Printf( "maprotation: invalid map rotation \"%s\"\n", rotationName );
	}
}

static void Svcmd_TeamMessage_f( void )
{
	char   teamNum[ 2 ];
	team_t team;
	char   *arg;

	if ( trap_Argc() < 3 )
	{
		G_Printf( "usage: say_team <team> <message>\n" );
		return;
	}

	trap_Argv( 1, teamNum, sizeof( teamNum ) );
	team = G_TeamFromString( teamNum );

	if ( team == NUM_TEAMS )
	{
		G_Printf( "say_team: invalid team \"%s\"\n", teamNum );
		return;
	}

	arg = ConcatArgs( 2 );
	G_TeamCommand( team, va( "chat -1 %d %s", SAY_TEAM, Quote( arg ) ) );
	G_LogPrintf( "SayTeam: -1 \"console\": %s\n", arg );
}

static void Svcmd_CenterPrint_f( void )
{
	if ( trap_Argc() < 2 )
	{
		G_Printf( "usage: cp <message>\n" );
		return;
	}

	trap_SendServerCommand( -1, va( "cp %s", Quote( ConcatArgs( 1 ) ) ) );
}

static void Svcmd_EjectClient_f( void )
{
	char *reason, name[ MAX_STRING_CHARS ];

	if ( trap_Argc() < 2 )
	{
		G_Printf( "usage: eject <player|-1> <reason>\n" );
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
			G_Printf( "eject: cannot eject local clients\n" );
			return;
		}

		trap_DropClient( cl - level.clients, reason );
	}
}

static void Svcmd_DumpUser_f( void )
{
	char       name[ MAX_STRING_CHARS ], userinfo[ MAX_INFO_STRING ];
	char       key[ BIG_INFO_KEY ], value[ BIG_INFO_VALUE ];
	const char *info;
	gclient_t  *cl;

	if ( trap_Argc() != 2 )
	{
		G_Printf( "usage: dumpuser <player>\n" );
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
	G_Printf( "userinfo\n--------\n" );

	//Info_Print( userinfo );
	while ( 1 )
	{
		Info_NextPair( &info, key, value );

		if ( !*info )
		{
			return;
		}

		G_Printf( "%-20s%s\n", key, value );
	}
}

static void Svcmd_Pr_f( void )
{
	char targ[ 4 ];
	int  cl;

	if ( trap_Argc() < 3 )
	{
		G_Printf( "usage: <clientnum|-1> <message>\n" );
		return;
	}

	trap_Argv( 1, targ, sizeof( targ ) );
	cl = atoi( targ );

	if ( cl >= MAX_CLIENTS || cl < -1 )
	{
		G_Printf( "invalid clientnum %d\n", cl );
		return;
	}

	trap_SendServerCommand( cl, va( "print %s\\\n", Quote( ConcatArgs( 2 ) ) ) );
}

static void Svcmd_PrintQueue_f( void )
{
	char team[ MAX_STRING_CHARS ];

	if ( trap_Argc() != 2 )
	{
		G_Printf( "usage: printqueue <team>\n" );
		return;
	}

	trap_Argv( 1, team, sizeof( team ) );

	switch ( G_TeamFromString( team ) )
	{
		case TEAM_ALIENS:
			G_PrintSpawnQueue( &level.alienSpawnQueue );
			break;

		case TEAM_HUMANS:
			G_PrintSpawnQueue( &level.humanSpawnQueue );
			break;

		default:
			G_Printf( "unknown team\n" );
	}
}

// dumb wrapper for "a", "m", "chat", and "say"
static void Svcmd_MessageWrapper( void )
{
	char cmd[ 5 ];
	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( !Q_stricmp( cmd, "a" ) )
	{
		Cmd_AdminMessage_f( NULL );
	}
	else if ( !Q_stricmp( cmd, "asay" ) )
	{
		G_Say( NULL, SAY_ALL_ADMIN, ConcatArgs( 1 ) );
	}
	else if ( !Q_stricmp( cmd, "m" ) )
	{
		Cmd_PrivateMessage_f( NULL );
	}
	else if ( !Q_stricmp( cmd, "say" ) )
	{
		G_Say( NULL, SAY_ALL, ConcatArgs( 1 ) );
	}
	else if ( !Q_stricmp( cmd, "chat" ) )
	{
		G_Say( NULL, SAY_RAW, ConcatArgs( 1 ) );
	}
}

static void Svcmd_ListMapsWrapper( void )
{
	Cmd_ListMaps_f( NULL );
}

static void Svcmd_ListRotationWrapper( void )
{
	G_PrintCurrentRotation( NULL );
}

static void Svcmd_MapLogWrapper( void )
{
	Cmd_MapLog_f( NULL );
}

static void Svcmd_SuddenDeath_f( void )
{
	char secs[ 5 ];
	int  offset;
	trap_Argv( 1, secs, sizeof( secs ) );
	offset = atoi( secs );

	level.suddenDeathBeginTime = level.time - level.startTime + offset * 1000;
	trap_SendServerCommand( -1, va( "cp \"Sudden Death will begin in %ds\"", offset ) );
}

static void Svcmd_G_AdvanceMapRotation_f( void )
{
	G_AdvanceMapRotation( 0 );
}

static const struct svcmd
{
	const char *cmd;
	qboolean dedicated;
	void ( *function )( void );
} svcmds[] =
{
	{ "a",                  qtrue,  Svcmd_MessageWrapper         },
	{ "admitDefeat",        qfalse, Svcmd_AdmitDefeat_f          },
	{ "advanceMapRotation", qfalse, Svcmd_G_AdvanceMapRotation_f },
	{ "alienWin",           qfalse, Svcmd_TeamWin_f              },
	{ "asay",               qtrue,  Svcmd_MessageWrapper         },
	{ "chat",               qtrue,  Svcmd_MessageWrapper         },
	{ "cp",                 qtrue,  Svcmd_CenterPrint_f          },
	{ "dumpuser",           qfalse, Svcmd_DumpUser_f             },
	{ "eject",              qfalse, Svcmd_EjectClient_f          },
	{ "entityFire",         qfalse, Svcmd_EntityFire_f           },
	{ "entityList",         qfalse, Svcmd_EntityList_f           },
	{ "entityShow",         qfalse, Svcmd_EntityShow_f           },
	{ "evacuation",         qfalse, Svcmd_Evacuation_f           },
	{ "forceTeam",          qfalse, Svcmd_ForceTeam_f            },
	{ "game_memory",        qfalse, BG_MemoryInfo                },
	{ "humanWin",           qfalse, Svcmd_TeamWin_f              },
	{ "layoutLoad",         qfalse, Svcmd_LayoutLoad_f           },
	{ "layoutSave",         qfalse, Svcmd_LayoutSave_f           },
	{ "listmaps",           qtrue,  Svcmd_ListMapsWrapper        },
	{ "listrotation",       qtrue,  Svcmd_ListRotationWrapper    },
	{ "loadcensors",        qfalse, G_LoadCensors                },
	{ "m",                  qtrue,  Svcmd_MessageWrapper         },
	{ "maplog",             qtrue,  Svcmd_MapLogWrapper          },
	{ "mapRotation",        qfalse, Svcmd_MapRotation_f          },
	{ "pr",                 qfalse, Svcmd_Pr_f                   },
	{ "printqueue",         qfalse, Svcmd_PrintQueue_f           },
	{ "say",                qtrue,  Svcmd_MessageWrapper         },
	{ "say_team",           qtrue,  Svcmd_TeamMessage_f          },
	{ "stopMapRotation",    qfalse, G_StopMapRotation            },
	{ "suddendeath",        qfalse, Svcmd_SuddenDeath_f          }
};

/*
=================
ConsoleCommand

=================
*/
qboolean  ConsoleCommand( void )
{
	char         cmd[ MAX_TOKEN_CHARS ];
	struct svcmd *command;

	trap_Argv( 0, cmd, sizeof( cmd ) );

	command = bsearch( cmd, svcmds, ARRAY_LEN( svcmds ),
	                   sizeof( struct svcmd ), cmdcmp );

	if ( !command )
	{
		// see if this is an admin command
		if ( G_admin_cmd_check( NULL ) )
		{
			return qtrue;
		}

		if ( g_dedicated.integer )
		{
			G_Printf( "unknown command: %s\n", cmd );
		}

		return qfalse;
	}

	if ( command->dedicated && !g_dedicated.integer )
	{
		return qfalse;
	}

	command->function();
	return qtrue;
}

void G_RegisterCommands( void )
{
	int i;

	for ( i = 0; i < ARRAY_LEN( svcmds ); i++ )
	{
		if ( svcmds[ i ].dedicated && !g_dedicated.integer )
		{
			continue;
		}

		trap_AddCommand( svcmds[ i ].cmd );
	}

	G_admin_register_cmds();
}

void G_UnregisterCommands( void )
{
	int i;

	for ( i = 0; i < ARRAY_LEN( svcmds ); i++ )
	{
		if ( svcmds[ i ].dedicated && !g_dedicated.integer )
		{
			continue;
		}

		trap_RemoveCommand( svcmds[ i ].cmd );
	}

	G_admin_unregister_cmds();
}
