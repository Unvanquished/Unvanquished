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

//===========================================================================
//
// Name:            g_script.c
// Function:        Wolfenstein Entity Scripting
// Programmer:      Ridah
// Tab Size:        4 (real tabs)
//===========================================================================

#include "g_local.h"
#include "../../../../engine/qcommon/q_shared.h"
#ifdef OMNIBOT
#include "../../omnibot/et/g_etbot_interface.h"
#endif

/*
Scripting that allows the designers to control the behaviour of entities
according to each different scenario.
*/

vmCvar_t g_scriptDebug;

//
//====================================================================
//
// action functions need to be declared here so they can be accessed in the scriptAction table
qboolean                G_ScriptAction_GotoMarker ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_Wait ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_Trigger ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_PlaySound ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_PlayAnim ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AlertEntity ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ToggleSpeaker ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_DisableSpeaker ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_EnableSpeaker ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_Accum ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_GlobalAccum ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_Print ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_FaceAngles ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ResetScript ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_TagConnect ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_Halt ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_StopSound ( gentity_t *ent, char *params );

//qboolean G_ScriptAction_StartCam( gentity_t *ent, char *params );
qboolean                G_ScriptAction_EntityScriptName ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AIScriptName ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AxisRespawntime ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AlliedRespawntime ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_NumberofObjectives ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetWinner ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetDefendingTeam ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_Announce ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_Announce_Icon ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AddTeamVoiceAnnounce ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_RemoveTeamVoiceAnnounce ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_TeamVoiceAnnounce ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_EndRound ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetRoundTimelimit ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_RemoveEntity ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetState ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_VoiceAnnounce ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_FollowSpline ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_FollowPath ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AbortMove ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetSpeed ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetRotation ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_StopRotation ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_StartAnimation ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AttatchToTrain ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_FreezeAnimation ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_UnFreezeAnimation ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ShaderRemap ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ShaderRemapFlush ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ChangeModel ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetChargeTimeFactor ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetDamagable ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_StartCam ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetInitialCamera ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_StopCam ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_RepairMG42 ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetHQStatus ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_PrintAccum ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_PrintGlobalAccum ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ObjectiveStatus ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetModelFromBrushmodel ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetPosition ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetAutoSpawn ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetMainObjective ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SpawnRubble ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AllowTankExit ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AllowTankEnter ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetTankAmmo ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AddTankAmmo ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_Kill ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_DisableMessage ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetGlobalFog ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_Cvar ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AbortIfWarmup ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_AbortIfNotSinglePlayer ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_MusicStart ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_MusicPlay ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_MusicStop ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_MusicQueue ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_MusicFade ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetDebugLevel ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_FadeAllSounds ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_SetAASState ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_Construct ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ConstructibleClass ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ConstructibleChargeBarReq ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ConstructibleConstructXPBonus ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ConstructibleDestructXPBonus ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ConstructibleHealth ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ConstructibleWeaponclass ( gentity_t *ent, char *params );
qboolean                G_ScriptAction_ConstructibleDuration ( gentity_t *ent, char *params );

//bani
qboolean                etpro_ScriptAction_SetValues ( gentity_t *ent, char *params );

// these are the actions that each event can call
g_script_stack_action_t gScriptActions[] =
{
	{ "gotomarker",                     G_ScriptAction_GotoMarker                    },
	{ "playsound",                      G_ScriptAction_PlaySound                     },
	{ "playanim",                       G_ScriptAction_PlayAnim                      },
	{ "wait",                           G_ScriptAction_Wait                          },
	{ "trigger",                        G_ScriptAction_Trigger                       },
	{ "alertentity",                    G_ScriptAction_AlertEntity                   },
	{ "togglespeaker",                  G_ScriptAction_ToggleSpeaker                 },
	{ "disablespeaker",                 G_ScriptAction_DisableSpeaker                },
	{ "enablespeaker",                  G_ScriptAction_EnableSpeaker                 },
	{ "accum",                          G_ScriptAction_Accum                         },
	{ "globalaccum",                    G_ScriptAction_GlobalAccum                   },
	{ "print",                          G_ScriptAction_Print                         },
	{ "faceangles",                     G_ScriptAction_FaceAngles                    },
	{ "resetscript",                    G_ScriptAction_ResetScript                   },
	{ "attachtotag",                    G_ScriptAction_TagConnect                    },
	{ "halt",                           G_ScriptAction_Halt                          },
	{ "stopsound",                      G_ScriptAction_StopSound                     },
//  {"startcam",                        G_ScriptAction_StartCam},
	{ "entityscriptname",               G_ScriptAction_EntityScriptName              },
	{ "aiscriptname",                   G_ScriptAction_AIScriptName                  },
	{ "wm_axis_respawntime",            G_ScriptAction_AxisRespawntime               },
	{ "wm_allied_respawntime",          G_ScriptAction_AlliedRespawntime             },
	{ "wm_number_of_objectives",        G_ScriptAction_NumberofObjectives            },
	{ "wm_setwinner",                   G_ScriptAction_SetWinner                     },
	{ "wm_set_defending_team",          G_ScriptAction_SetDefendingTeam              },
	{ "wm_announce",                    G_ScriptAction_Announce                      },
	{ "wm_teamvoiceannounce",           G_ScriptAction_TeamVoiceAnnounce             },
	{ "wm_addteamvoiceannounce",        G_ScriptAction_AddTeamVoiceAnnounce          },
	{ "wm_removeteamvoiceannounce",     G_ScriptAction_RemoveTeamVoiceAnnounce       },
	{ "wm_announce_icon",               G_ScriptAction_Announce_Icon                 },
	{ "wm_endround",                    G_ScriptAction_EndRound                      },
	{ "wm_set_round_timelimit",         G_ScriptAction_SetRoundTimelimit             },
	{ "wm_voiceannounce",               G_ScriptAction_VoiceAnnounce                 },
	{ "wm_objective_status",            G_ScriptAction_ObjectiveStatus               },
	{ "wm_set_main_objective",          G_ScriptAction_SetMainObjective              },
	{ "remove",                         G_ScriptAction_RemoveEntity                  },
	{ "setstate",                       G_ScriptAction_SetState                      },
	{ "followspline",                   G_ScriptAction_FollowSpline                  },
	{ "followpath",                     G_ScriptAction_FollowPath                    },
	{ "abortmove",                      G_ScriptAction_AbortMove                     },
	{ "setspeed",                       G_ScriptAction_SetSpeed                      },
	{ "setrotation",                    G_ScriptAction_SetRotation                   },
	{ "stoprotation",                   G_ScriptAction_StopRotation                  },
	{ "startanimation",                 G_ScriptAction_StartAnimation                },
	{ "attatchtotrain",                 G_ScriptAction_AttatchToTrain                },
	{ "freezeanimation",                G_ScriptAction_FreezeAnimation               },
	{ "unfreezeanimation",              G_ScriptAction_UnFreezeAnimation             },
	{ "remapshader",                    G_ScriptAction_ShaderRemap                   },
	{ "remapshaderflush",               G_ScriptAction_ShaderRemapFlush              },
	{ "changemodel",                    G_ScriptAction_ChangeModel                   },
	{ "setchargetimefactor",            G_ScriptAction_SetChargeTimeFactor           },
	{ "setdamagable",                   G_ScriptAction_SetDamagable                  },
	{ "startcam",                       G_ScriptAction_StartCam                      },
	{ "SetInitialCamera",               G_ScriptAction_SetInitialCamera              },
	{ "stopcam",                        G_ScriptAction_StopCam                       },
	{ "repairmg42",                     G_ScriptAction_RepairMG42                    },
	{ "sethqstatus",                    G_ScriptAction_SetHQStatus                   },

	{ "printaccum",                     G_ScriptAction_PrintAccum                    },
	{ "printglobalaccum",               G_ScriptAction_PrintGlobalAccum              },
	{ "cvar",                           G_ScriptAction_Cvar                          },
	{ "abortifwarmup",                  G_ScriptAction_AbortIfWarmup                 },
	{ "abortifnotsingleplayer",         G_ScriptAction_AbortIfNotSinglePlayer        },

	{ "mu_start",                       G_ScriptAction_MusicStart                    },
	{ "mu_play",                        G_ScriptAction_MusicPlay                     },
	{ "mu_stop",                        G_ScriptAction_MusicStop                     },
	{ "mu_queue",                       G_ScriptAction_MusicQueue                    },
	{ "mu_fade",                        G_ScriptAction_MusicFade                     },
	{ "setdebuglevel",                  G_ScriptAction_SetDebugLevel                 },
	{ "setposition",                    G_ScriptAction_SetPosition                   },
	{ "setautospawn",                   G_ScriptAction_SetAutoSpawn                  },

	// Gordon: going for longest silly script command ever here :) (sets a model for a brush to one stolen from a func_brushmodel
	{ "setmodelfrombrushmodel",         G_ScriptAction_SetModelFromBrushmodel        },

	// fade all sounds up or down
	{ "fadeallsounds",                  G_ScriptAction_FadeAllSounds                 },

	{ "setaasstate",                    G_ScriptAction_SetAASState                   },
	{ "construct",                      G_ScriptAction_Construct                     },
	{ "spawnrubble",                    G_ScriptAction_SpawnRubble                   },
	{ "setglobalfog",                   G_ScriptAction_SetGlobalFog                  },
	{ "allowtankexit",                  G_ScriptAction_AllowTankExit                 },
	{ "allowtankenter",                 G_ScriptAction_AllowTankEnter                },
	{ "settankammo",                    G_ScriptAction_SetTankAmmo                   },
	{ "addtankammo",                    G_ScriptAction_AddTankAmmo                   },
	{ "kill",                           G_ScriptAction_Kill                          },
	{ "disablemessage",                 G_ScriptAction_DisableMessage                },

//bani
	{ "set",                            etpro_ScriptAction_SetValues                 },

	{ "constructible_class",            G_ScriptAction_ConstructibleClass            },
	{ "constructible_chargebarreq",     G_ScriptAction_ConstructibleChargeBarReq     },
	{ "constructible_constructxpbonus", G_ScriptAction_ConstructibleConstructXPBonus },
	{ "constructible_destructxpbonus",  G_ScriptAction_ConstructibleDestructXPBonus  },
	{ "constructible_health",           G_ScriptAction_ConstructibleHealth           },
	{ "constructible_weaponclass",      G_ScriptAction_ConstructibleWeaponclass      },
	{ "constructible_duration",         G_ScriptAction_ConstructibleDuration         },
	{ NULL,                             NULL                                         }
};

qboolean                G_Script_EventMatch_StringEqual ( g_script_event_t *event, char *eventParm );
qboolean                G_Script_EventMatch_IntInRange ( g_script_event_t *event, char *eventParm );

// the list of events that can start an action sequence
g_script_event_define_t gScriptEvents[] =
{
	{ "spawn",       NULL                            }, // called as each character is spawned into the game
	{ "trigger",     G_Script_EventMatch_StringEqual }, // something has triggered us (always followed by an identifier)
	{ "pain",        G_Script_EventMatch_IntInRange  }, // we've been hurt
	{ "death",       NULL                            }, // RIP
	{ "activate",    G_Script_EventMatch_StringEqual }, // something has triggered us [activator team]
	{ "stopcam",     NULL                            },
	{ "playerstart", NULL                            },
	{ "built",       G_Script_EventMatch_StringEqual },
	{ "buildstart",  G_Script_EventMatch_StringEqual },
	{ "decayed",     G_Script_EventMatch_StringEqual },
	{ "destroyed",   G_Script_EventMatch_StringEqual },
	{ "rebirth",     NULL                            },
	{ "failed",      NULL                            },
	{ "dynamited",   NULL                            },
	{ "defused",     NULL                            },
	{ "mg42",        G_Script_EventMatch_StringEqual },
	{ "message",     G_Script_EventMatch_StringEqual }, // contains a sequence of VO in a message

	{ NULL,          NULL                            }
};

/*
===============
G_Script_EventMatch_StringEqual
===============
*/
qboolean G_Script_EventMatch_StringEqual ( g_script_event_t *event, char *eventParm )
{
	if ( eventParm && !Q_stricmp ( event->params, eventParm ) )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
===============
G_Script_EventMatch_IntInRange
===============
*/
qboolean G_Script_EventMatch_IntInRange ( g_script_event_t *event, char *eventParm )
{
	char *pString, *token;
	int  int1, int2, eInt;

	// get the cast name
	pString = eventParm;
	token = COM_ParseExt ( &pString, qfalse );
	int1 = atoi ( token );
	token = COM_ParseExt ( &pString, qfalse );
	int2 = atoi ( token );

	eInt = atoi ( event->params );

	if ( eventParm && eInt > int1 && eInt <= int2 )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
===============
G_Script_EventForString
===============
*/
int G_Script_EventForString ( const char *string )
{
	int i, hash;

	hash = BG_StringHashValue_Lwr ( string );

	for ( i = 0; gScriptEvents[ i ].eventStr; i++ )
	{
		if ( gScriptEvents[ i ].hash == hash && !Q_stricmp ( string, gScriptEvents[ i ].eventStr ) )
		{
			return i;
		}
	}

	return -1;
}

/*
===============
G_Script_ActionForString
===============
*/
g_script_stack_action_t *G_Script_ActionForString ( char *string )
{
	int i, hash;

	hash = BG_StringHashValue_Lwr ( string );

	for ( i = 0; gScriptActions[ i ].actionString; i++ )
	{
		if ( gScriptActions[ i ].hash == hash && !Q_stricmp ( string, gScriptActions[ i ].actionString ) )
		{
			return &gScriptActions[ i ];
		}
	}

	return NULL;
}

/*
=============
G_Script_ScriptLoad

  Loads the script for the current level into the buffer
=============
*/
void G_Script_ScriptLoad ( void )
{
	char         filename[ MAX_QPATH ];
	vmCvar_t     mapname;
	fileHandle_t f;
	int          len;

	trap_Cvar_Register ( &g_scriptDebug, "g_scriptDebug", "0", 0 );

	level.scriptEntity = NULL;

	trap_Cvar_VariableStringBuffer ( "g_scriptName", filename, sizeof ( filename ) );

	if ( strlen ( filename ) > 0 )
	{
		trap_Cvar_Register ( &mapname, "g_scriptName", "", CVAR_CHEAT );
	}
	else
	{
		trap_Cvar_Register ( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	}

	Q_strncpyz ( filename, "maps/", sizeof ( filename ) );
	Q_strcat ( filename, sizeof ( filename ), mapname.string );

	if ( g_gametype.integer == GT_WOLF_LMS )
	{
		Q_strcat ( filename, sizeof ( filename ), "_lms" );
	}

	Q_strcat ( filename, sizeof ( filename ), ".script" );

	len = trap_FS_FOpenFile ( filename, &f, FS_READ );

	// make sure we clear out the temporary scriptname
	trap_Cvar_Set ( "g_scriptName", "" );

	if ( len < 0 )
	{
		return;
	}

	// END Mad Doc - TDF
	// Arnout: make sure we terminate the script with a '\0' to prevent parser from choking
	//level.scriptEntity = G_Alloc( len );
	//trap_FS_Read( level.scriptEntity, len, f );
	level.scriptEntity = G_Alloc ( len + 1 );
	trap_FS_Read ( level.scriptEntity, len, f );
	* ( level.scriptEntity + len ) = '\0';

	// Gordon: and make sure ppl haven't put stuff with uppercase in the string table..
	G_Script_EventStringInit();

	// Gordon: discard all the comments NOW, so we dont deal with them inside scripts
	// Gordon: disabling for a sec, wanna check if i can get proper line numbers from error output
//  COM_Compress( level.scriptEntity );

	trap_FS_FCloseFile ( f );
}

/*
==============
G_Script_ParseSpawnbot

  Parses "Spawnbot" command, precaching a custom character if specified
==============
*/
void G_Script_ParseSpawnbot ( char **ppScript, char params[], int paramsize )
{
	qboolean parsingCharacter = qfalse;
	char     *token;

	token = COM_ParseExt ( ppScript, qfalse );

	while ( token[ 0 ] )
	{
		// if we are currently parsing a spawnbot command, check the parms for
		// a custom character, which we will need to precache on the client

		// did we just see a '/character' parm?
		if ( parsingCharacter )
		{
			parsingCharacter = qfalse;

			G_CharacterIndex ( token );

			if ( !BG_FindCharacter ( token ) )
			{
				bg_character_t *character = BG_FindFreeCharacter ( token );

				Q_strncpyz ( character->characterFile, token, sizeof ( character->characterFile ) );

				if ( !G_RegisterCharacter ( token, character ) )
				{
					G_Error ( "ERROR: G_Script_ParseSpawnbot: failed to load character file '%s'\n", token );
				}
			}

#ifdef DEBUG
			G_DPrintf ( "precached character %s\n", token );
#endif
		}
		else if ( !Q_stricmp ( token, "/character" ) )
		{
			parsingCharacter = qtrue;
		}

		if ( strlen ( params ) )
		{
			// add a space between each param
			Q_strcat ( params, paramsize, " " );
		}

		if ( strrchr ( token, ' ' ) )
		{
			// need to wrap this param in quotes since it has more than one word
			Q_strcat ( params, paramsize, "\"" );
		}

		Q_strcat ( params, paramsize, token );

		if ( strrchr ( token, ' ' ) )
		{
			// need to wrap this param in quotes since it has more than one word
			Q_strcat ( params, paramsize, "\"" );
		}

		token = COM_ParseExt ( ppScript, qfalse );
	}
}

/*
==============
G_Script_ScriptParse

  Parses the script for the given entity
==============
*/
void G_Script_ScriptParse ( gentity_t *ent )
{
	char             *pScript;
	char             *token;
	qboolean         wantName;
	qboolean         inScript;
	int              eventNum;
	g_script_event_t events[ G_MAX_SCRIPT_STACK_ITEMS ];
	int              numEventItems;
	g_script_event_t *curEvent;

	// DHM - Nerve :: Some of our multiplayer script commands have longer parameters
	//char      params[MAX_QPATH];
	char                    params[ MAX_INFO_STRING ];

	// dhm - end
	g_script_stack_action_t *action;
	int                     i;
	int                     bracketLevel;
	qboolean                buildScript;

	if ( !ent->scriptName )
	{
		return;
	}

	if ( !level.scriptEntity )
	{
		return;
	}

	buildScript = trap_Cvar_VariableIntegerValue ( "com_buildScript" );

	pScript = level.scriptEntity;
	wantName = qtrue;
	inScript = qfalse;
	COM_BeginParseSession ( "G_Script_ScriptParse" );
	bracketLevel = 0;
	numEventItems = 0;

	memset ( events, 0, sizeof ( events ) );

	while ( 1 )
	{
		token = COM_Parse ( &pScript );

		if ( !token[ 0 ] )
		{
			if ( !wantName )
			{
				G_Error ( "G_Script_ScriptParse(), Error (line %d): '}' expected, end of script found.\n",
				          COM_GetCurrentParseLine() );
			}

			break;
		}

		// end of script
		if ( token[ 0 ] == '}' )
		{
			if ( inScript )
			{
				break;
			}

			if ( wantName )
			{
				G_Error ( "G_Script_ScriptParse(), Error (line %d): '}' found, but not expected.\n", COM_GetCurrentParseLine() );
			}

			wantName = qtrue;
		}
		else if ( token[ 0 ] == '{' )
		{
			if ( wantName )
			{
				G_Error ( "G_Script_ScriptParse(), Error (line %d): '{' found, NAME expected.\n", COM_GetCurrentParseLine() );
			}
		}
		else if ( wantName )
		{
			if ( !Q_stricmp ( token, "bot" ) )
			{
				// a bot, skip this whole entry
				SkipRestOfLine ( &pScript );
				// skip this section
				SkipBracedSection ( &pScript );
				//
				continue;
			}

			if ( !Q_stricmp ( token, "entity" ) )
			{
				// this is an entity, so go back to look for a name
				continue;
			}

			if ( !Q_stricmp ( ent->scriptName, token ) )
			{
				inScript = qtrue;
				numEventItems = 0;
			}

			wantName = qfalse;
		}
		else if ( inScript )
		{
			Q_strlwr ( token );
			eventNum = G_Script_EventForString ( token );

			if ( eventNum < 0 )
			{
				G_Error ( "G_Script_ScriptParse(), Error (line %d): unknown event: %s.\n", COM_GetCurrentParseLine(), token );
			}

			if ( numEventItems >= G_MAX_SCRIPT_STACK_ITEMS )
			{
				G_Error ( "G_Script_ScriptParse(), Error (line %d): G_MAX_SCRIPT_STACK_ITEMS reached (%d)\n",
				          COM_GetCurrentParseLine(), G_MAX_SCRIPT_STACK_ITEMS );
			}

			curEvent = &events[ numEventItems ];
			curEvent->eventNum = eventNum;
			memset ( params, 0, sizeof ( params ) );

			// parse any event params before the start of this event's actions
			while ( ( token = COM_Parse ( &pScript ) ) != NULL && ( token[ 0 ] != '{' ) )
			{
				if ( !token[ 0 ] )
				{
					G_Error ( "G_Script_ScriptParse(), Error (line %d): '}' expected, end of script found.\n",
					          COM_GetCurrentParseLine() );
				}

				if ( strlen ( params ) )
				{
					// add a space between each param
					Q_strcat ( params, sizeof ( params ), " " );
				}

				Q_strcat ( params, sizeof ( params ), token );
			}

			if ( strlen ( params ) )
			{
				// copy the params into the event
				curEvent->params = G_Alloc ( strlen ( params ) + 1 );
				Q_strncpyz ( curEvent->params, params, strlen ( params ) + 1 );
			}

			// parse the actions for this event
			while ( ( token = COM_Parse ( &pScript ) ) != NULL && ( token[ 0 ] != '}' ) )
			{
				if ( !token[ 0 ] )
				{
					G_Error ( "G_Script_ScriptParse(), Error (line %d): '}' expected, end of script found.\n",
					          COM_GetCurrentParseLine() );
				}

				action = G_Script_ActionForString ( token );

				if ( !action )
				{
					G_Error ( "G_Script_ScriptParse(), Error (line %d): unknown action: %s.\n", COM_GetCurrentParseLine(), token );
				}

				curEvent->stack.items[ curEvent->stack.numItems ].action = action;

				memset ( params, 0, sizeof ( params ) );

				// Ikkyo - Parse for {}'s if this is a set command
				if ( !Q_stricmp ( action->actionString, "set" ) )
				{
					token = COM_Parse ( &pScript );

					if ( token[ 0 ] != '{' )
					{
						COM_ParseError ( "'{' expected, found: %s.\n", token );
					}

					while ( ( token = COM_Parse ( &pScript ) ) && ( token[ 0 ] != '}' ) )
					{
						if ( strlen ( params ) )
						{
							// add a space between each param
							Q_strcat ( params, sizeof ( params ), " " );
						}

						if ( strrchr ( token, ' ' ) )
						{
							// need to wrap this param in quotes since it has more than one word
							Q_strcat ( params, sizeof ( params ), "\"" );
						}

						Q_strcat ( params, sizeof ( params ), token );

						if ( strrchr ( token, ' ' ) )
						{
							// need to wrap this param in quotes since it has mor
							Q_strcat ( params, sizeof ( params ), "\"" );
						}
					}
				}
				else

					// hackly precaching of custom characters
					if ( !Q_stricmp ( token, "spawnbot" ) )
					{
						// this is fairly indepth, so I'll move it to a separate function for readability
						G_Script_ParseSpawnbot ( &pScript, params, MAX_INFO_STRING );
					}
					else
					{
						token = COM_ParseExt ( &pScript, qfalse );

						for ( i = 0; token[ 0 ]; i++ )
						{
							if ( strlen ( params ) )
							{
								// add a space between each param
								Q_strcat ( params, sizeof ( params ), " " );
							}

							if ( i == 0 )
							{
								// Special case: playsound's need to be cached on startup to prevent in-game pauses
								if ( !Q_stricmp ( action->actionString, "playsound" ) )
								{
									G_SoundIndex ( token );
								}
								else if ( !Q_stricmp ( action->actionString, "changemodel" ) )
								{
									G_ModelIndex ( token );
								}
								else if ( buildScript && ( !Q_stricmp ( action->actionString, "mu_start" ) ||
								                           !Q_stricmp ( action->actionString, "mu_play" ) ||
								                           !Q_stricmp ( action->actionString, "mu_queue" ) ||
								                           !Q_stricmp ( action->actionString, "startcam" ) ) )
								{
									if ( strlen ( token ) )
									{
										// we know there's a [0], but don't know if it's '0'
										trap_SendServerCommand ( -1, va ( "addToBuild %s\n", token ) );
									}
								}
							}

							if ( i == 0 || i == 1 )
							{
								if ( !Q_stricmp ( action->actionString, "remapshader" ) )
								{
									G_ShaderIndex ( token );
								}
							}

							if ( strrchr ( token, ' ' ) )
							{
								// need to wrap this param in quotes since it has more than one word
								Q_strcat ( params, sizeof ( params ), "\"" );
							}

							Q_strcat ( params, sizeof ( params ), token );

							if ( strrchr ( token, ' ' ) )
							{
								// need to wrap this param in quotes since it has more than one word
								Q_strcat ( params, sizeof ( params ), "\"" );
							}

							token = COM_ParseExt ( &pScript, qfalse );
						}
					}

				if ( strlen ( params ) )
				{
					// copy the params into the event
					curEvent->stack.items[ curEvent->stack.numItems ].params = G_Alloc ( strlen ( params ) + 1 );
					Q_strncpyz ( curEvent->stack.items[ curEvent->stack.numItems ].params, params, strlen ( params ) + 1 );
				}

				curEvent->stack.numItems++;

				if ( curEvent->stack.numItems >= G_MAX_SCRIPT_STACK_ITEMS )
				{
					G_Error ( "G_Script_ScriptParse(): script exceeded G_MAX_SCRIPT_STACK_ITEMS (%d), line %d\n",
					          G_MAX_SCRIPT_STACK_ITEMS, COM_GetCurrentParseLine() );
				}
			}

			numEventItems++;
		}
		else
		{
			// skip this character completely
			// TTimo gcc: suggest parentheses around assignment used as truth value
			while ( ( token = COM_Parse ( &pScript ) ) != NULL )
			{
				if ( !token[ 0 ] )
				{
					G_Error ( "G_Script_ScriptParse(), Error (line %d): '}' expected, end of script found.\n",
					          COM_GetCurrentParseLine() );
				}
				else if ( token[ 0 ] == '{' )
				{
					bracketLevel++;
				}
				else if ( token[ 0 ] == '}' )
				{
					if ( !--bracketLevel )
					{
						break;
					}
				}
			}
		}
	}

	// alloc and copy the events into the gentity_t for this cast
	if ( numEventItems > 0 )
	{
		ent->scriptEvents = G_Alloc ( sizeof ( g_script_event_t ) * numEventItems );
		memcpy ( ent->scriptEvents, events, sizeof ( g_script_event_t ) * numEventItems );
		ent->numScriptEvents = numEventItems;
	}
}

/*
================
G_Script_ScriptChange
================
*/
qboolean G_Script_ScriptRun ( gentity_t *ent );

void G_Script_ScriptChange ( gentity_t *ent, int newScriptNum )
{
	g_script_status_t scriptStatusBackup;

	// backup the current scripting
	memcpy ( &scriptStatusBackup, &ent->scriptStatus, sizeof ( g_script_status_t ) );

	// set the new script to this cast, and reset script status
	ent->scriptStatus.scriptEventIndex = newScriptNum;
	ent->scriptStatus.scriptStackHead = 0;
	ent->scriptStatus.scriptStackChangeTime = level.time;
	ent->scriptStatus.scriptId = scriptStatusBackup.scriptId + 1;
	ent->scriptStatus.scriptFlags |= SCFL_FIRST_CALL;

	// try and run the script, if it doesn't finish, then abort the current script (discard backup)
	if ( G_Script_ScriptRun ( ent ) && ( ent->scriptStatus.scriptId == scriptStatusBackup.scriptId + 1 ) )
	{
		// make sure we didnt change our script via a third party
		// completed successfully
		memcpy ( &ent->scriptStatus, &scriptStatusBackup, sizeof ( g_script_status_t ) );
		ent->scriptStatus.scriptFlags &= ~SCFL_FIRST_CALL;
	}
}

// Gordon: lower the strings and hash them
void G_Script_EventStringInit ( void )
{
	int i;

	for ( i = 0; gScriptEvents[ i ].eventStr; i++ )
	{
		gScriptEvents[ i ].hash = BG_StringHashValue_Lwr ( gScriptEvents[ i ].eventStr );
	}

	for ( i = 0; gScriptActions[ i ].actionString; i++ )
	{
		gScriptActions[ i ].hash = BG_StringHashValue_Lwr ( gScriptActions[ i ].actionString );
	}
}

/*
================
G_Script_GetEventIndex

  returns the event index within the entity for the specified event string
  xkan, 10/28/2002 - extracted from G_Script_ScriptEvent.
================
*/
int G_Script_GetEventIndex ( gentity_t *ent, char *eventStr, char *params )
{
	int i, eventNum = -1;

	int hash = BG_StringHashValue_Lwr ( eventStr );

	// find out which event this is
	for ( i = 0; gScriptEvents[ i ].eventStr; i++ )
	{
		if ( gScriptEvents[ i ].hash == hash && !Q_stricmp ( eventStr, gScriptEvents[ i ].eventStr ) )
		{
			// match found
			eventNum = i;
			break;
		}
	}

	if ( eventNum < 0 )
	{
		if ( g_cheats.integer )
		{
			// dev mode
			G_Printf ( "devmode-> G_Script_GetEventIndex(), unknown event: %s\n", eventStr );
		}

		return -1;
	}

	// show debugging info
	if ( g_scriptDebug.integer )
	{
		G_Printf ( "%i : (%s) GScript event: %s %s\n", level.time, ent->scriptName ? ent->scriptName : "n/a", eventStr,
		           params ? params : "" );
	}

	// see if this entity has this event
	for ( i = 0; i < ent->numScriptEvents; i++ )
	{
		if ( ent->scriptEvents[ i ].eventNum == eventNum )
		{
			if ( ( !ent->scriptEvents[ i ].params ) ||
			     ( !gScriptEvents[ eventNum ].eventMatch || gScriptEvents[ eventNum ].eventMatch ( &ent->scriptEvents[ i ], params ) ) )
			{
				return i;
			}
		}
	}

	return -1; // event not found/matched in this ent
}

/*
================
G_Script_ScriptEvent

  An event has occured, for which a script may exist
================
*/
void G_Script_ScriptEvent ( gentity_t *ent, char *eventStr, char *params )
{
	int i = G_Script_GetEventIndex ( ent, eventStr, params );

	if ( i >= 0 )
	{
		G_Script_ScriptChange ( ent, i );
	}

#ifdef OMNIBOT

	// skip these
	if ( !Q_stricmp ( eventStr, "trigger" ) ||
	     !Q_stricmp ( eventStr, "activate" ) ||
	     !Q_stricmp ( eventStr, "spawn" ) ||
	     !Q_stricmp ( eventStr, "death" ) ||
	     !Q_stricmp ( eventStr, "pain" ) ||
	     !Q_stricmp ( eventStr, "playerstart" ) )
	{
		return;
	}

	if ( !Q_stricmp ( eventStr, "defused" ) )
	{
		Bot_Util_SendTrigger ( ent, NULL,
		                       va ( "Defused at %s.", ent->parent ? ent->parent->track : ent->track ),
		                       eventStr );
	}
	else if ( !Q_stricmp ( eventStr, "dynamited" ) )
	{
		Bot_Util_SendTrigger ( ent, NULL,
		                       va ( "Planted at %s.", ent->parent ? ent->parent->track : ent->track ),
		                       eventStr );
	}
	else if ( !Q_stricmp ( eventStr, "destroyed" ) )
	{
		Bot_Util_SendTrigger ( ent, NULL,
		                       va ( "%s Destroyed.", ent->parent ? ent->parent->track : ent->track ),
		                       eventStr );
	}
	else if ( !Q_stricmp ( eventStr, "exploded" ) )
	{
		Bot_Util_SendTrigger ( ent, NULL,
		                       va ( "Explode_%s Exploded.", _GetEntityName ( ent ) ), eventStr );
	}

#endif
}

/*
=============
G_Script_ScriptRun

  returns qtrue if the script completed
=============
*/
qboolean G_Script_ScriptRun ( gentity_t *ent )
{
	g_script_stack_t *stack;
	int              oldScriptId;

	if ( !ent->scriptEvents )
	{
		ent->scriptStatus.scriptEventIndex = -1;
		return qtrue;
	}

	// if we are still doing a gotomarker, process the movement
	if ( ent->scriptStatus.scriptFlags & SCFL_GOING_TO_MARKER )
	{
		G_ScriptAction_GotoMarker ( ent, NULL );
	}

	// if we are animating, do the animation
	if ( ent->scriptStatus.scriptFlags & SCFL_ANIMATING )
	{
		G_ScriptAction_PlayAnim ( ent, ent->scriptStatus.animatingParams );
	}

	if ( ent->scriptStatus.scriptEventIndex < 0 )
	{
		return qtrue;
	}

	stack = &ent->scriptEvents[ ent->scriptStatus.scriptEventIndex ].stack;

	if ( !stack->numItems )
	{
		ent->scriptStatus.scriptEventIndex = -1;
		return qtrue;
	}

	//
	// show debugging info
	if ( g_scriptDebug.integer && ent->scriptStatus.scriptStackChangeTime == level.time )
	{
		if ( ent->scriptStatus.scriptStackHead < stack->numItems )
		{
			G_Printf ( "%i : (%s) GScript command: %s %s\n", level.time, ent->scriptName,
			           stack->items[ ent->scriptStatus.scriptStackHead ].action->actionString,
			           ( stack->items[ ent->scriptStatus.scriptStackHead ].params ? stack->items[ ent->scriptStatus.scriptStackHead ].
			             params : "" ) );
		}
	}

	//
	while ( ent->scriptStatus.scriptStackHead < stack->numItems )
	{
		oldScriptId = ent->scriptStatus.scriptId;

		if ( !stack->items[ ent->scriptStatus.scriptStackHead ].action->
		     actionFunc ( ent, stack->items[ ent->scriptStatus.scriptStackHead ].params ) )
		{
			ent->scriptStatus.scriptFlags &= ~SCFL_FIRST_CALL;
			return qfalse;
		}

		//if the scriptId changed, a new event was triggered in our scripts which did not finish
		if ( oldScriptId != ent->scriptStatus.scriptId )
		{
			return qfalse;
		}

		// move to the next action in the script
		ent->scriptStatus.scriptStackHead++;
		// record the time that this new item became active
		ent->scriptStatus.scriptStackChangeTime = level.time;
		//
		ent->scriptStatus.scriptFlags |= SCFL_FIRST_CALL;

		// show debugging info
		if ( g_scriptDebug.integer )
		{
			if ( ent->scriptStatus.scriptStackHead < stack->numItems )
			{
				G_Printf ( "%i : (%s) GScript command: %s %s\n", level.time, ent->scriptName,
				           stack->items[ ent->scriptStatus.scriptStackHead ].action->actionString,
				           ( stack->items[ ent->scriptStatus.scriptStackHead ].params ? stack->
				             items[ ent->scriptStatus.scriptStackHead ].params : "" ) );
			}
		}
	}

	ent->scriptStatus.scriptEventIndex = -1;

	return qtrue;
}

//================================================================================
// Script Entities

void mountedmg42_fire ( gentity_t *other )
{
	vec3_t    forward, right, up;
	vec3_t    muzzle;
	gentity_t *self;

	if ( ! ( self = other->tankLink ) )
	{
		return;
	}

	AngleVectors ( other->client->ps.viewangles, forward, right, up );
	VectorCopy ( other->s.pos.trBase, muzzle );
//  VectorMA( muzzle, 42, up, muzzle );
	muzzle[ 2 ] += other->client->ps.viewheight;
	VectorMA ( muzzle, 58, forward, muzzle );

	SnapVector ( muzzle );

	if ( self->s.density & 8 )
	{
		Fire_Lead_Ext ( other, other, MG42_SPREAD_MP, MG42_DAMAGE_MP, muzzle, forward, right, up, MOD_BROWNING );
	}
	else
	{
		Fire_Lead_Ext ( other, other, MG42_SPREAD_MP, MG42_DAMAGE_MP, muzzle, forward, right, up, MOD_MG42 );
	}
}

void script_linkentity ( gentity_t *ent )
{
	// this is required since non-solid brushes need to be linked but not solid
	trap_LinkEntity ( ent );

//  if ((ent->s.eType == ET_MOVER) && !(ent->spawnflags & 2)) {
//      ent->s.solid = 0;
//  }
}

void script_mover_die ( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod )
{
	G_Script_ScriptEvent ( self, "death", "" );

	if ( ! ( self->spawnflags & 8 ) )
	{
		G_FreeEntity ( self );
	}

	if ( self->tankLink )
	{
		G_LeaveTank ( self->tankLink, qtrue );
	}

	self->die = NULL;
}

void script_mover_think ( gentity_t *ent )
{
	if ( ent->spawnflags & 128 )
	{
		if ( !ent->tankLink )
		{
			if ( ent->mg42weapHeat )
			{
				ent->mg42weapHeat -= ( 300.f * FRAMETIME * 0.001 );

				if ( ent->mg42weapHeat < 0 )
				{
					ent->mg42weapHeat = 0;
				}
			}

			if ( ent->backupWeaponTime )
			{
				ent->backupWeaponTime -= FRAMETIME;

				if ( ent->backupWeaponTime < 0 )
				{
					ent->backupWeaponTime = 0;
				}
			}
		}
	}

	ent->nextthink = level.time + FRAMETIME;
}

void script_mover_spawn ( gentity_t *ent )
{
	if ( ent->spawnflags & 128 )
	{
		if ( !ent->tagBuffer )
		{
			ent->nextTrain = ent;
		}
		else
		{
			gentity_t *tent = G_FindByTargetname ( NULL, ent->tagBuffer );

			if ( !tent )
			{
				ent->nextTrain = ent;
			}
			else
			{
				ent->nextTrain = tent;
			}
		}

		ent->s.effect3Time = ent->nextTrain - g_entities;
	}

	if ( ent->spawnflags & 2 )
	{
		ent->clipmask = CONTENTS_SOLID;
		ent->r.contents = CONTENTS_SOLID;
	}
	else
	{
		ent->s.eFlags |= EF_NONSOLID_BMODEL;
		ent->clipmask = 0;
		ent->r.contents = 0;
	}

	script_linkentity ( ent );

	// now start thinking process which controls AAS interaction
	ent->think = script_mover_think;
	ent->nextthink = level.time + 200;
}

void script_mover_use ( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if ( ent->spawnflags & 8 )
	{
		if ( ent->count )
		{
			ent->health = ent->count;
			ent->s.dl_intensity = ent->health;

			G_Script_ScriptEvent ( ent, "rebirth", "" );

			ent->die = script_mover_die;
		}
	}
	else
	{
		script_mover_spawn ( ent );
	}
}

void script_mover_blocked ( gentity_t *ent, gentity_t *other )
{
	// remove it, we must not stop for anything or it will screw up script timing
	if ( !other->client && other->s.eType != ET_CORPSE )
	{
		// /me slaps nerve
		// except CTF flags!!!!
		if ( other->s.eType == ET_ITEM && other->item->giType == IT_TEAM )
		{
			Team_DroppedFlagThink ( other );
			return;
		}

		G_TempEntity ( other->s.origin, EV_ITEM_POP );
		G_FreeEntity ( other );
		return;
	}

	// FIXME: we could have certain entities stop us, thereby "pausing" movement
	// until they move out the way. then we can just call the GotoMarker() again,
	// telling it that we are just now calling it for the first time, so it should
	// start us on our way again (theoretically speaking).

	// kill them
	G_Damage ( other, ent, ent, NULL, NULL, 9999, 0, MOD_CRUSH );
}

/*QUAKED script_mover (0.5 0.25 1.0) ? TRIGGERSPAWN SOLID EXPLOSIVEDAMAGEONLY RESURECTABLE COMPASS ALLIED AXIS MOUNTED_GUN
Scripted brush entity. A simplified means of moving brushes around based on events.

"modelscale" - Scale multiplier (defaults to 1, and scales uniformly)
"modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both the "modelscale" is ignored
"model2" optional md3 to draw over the solid clip brush
"scriptname" name used for scripting purposes (like aiName in AI scripting)
"health" optionally make this entity damagable
"description" used with health, if the entity is damagable, it draws a healthbar with this description above it.
*/
void SP_script_mover ( gentity_t *ent )
{
	float  scale[ 3 ] = { 1, 1, 1 };
	vec3_t scalevec;
	char   tagname[ MAX_QPATH ];
	char   *modelname;
	char   *tagent;
	char   cs[ MAX_INFO_STRING ];
	char   *s;

	if ( !ent->model )
	{
		G_Error ( "script_mover must have a \"model\"\n" );
	}

	if ( !ent->scriptName )
	{
		G_Error ( "script_mover must have a \"scriptname\"\n" );
	}

	ent->blocked = script_mover_blocked;

	// first position at start
	VectorCopy ( ent->s.origin, ent->pos1 );

//  VectorCopy( ent->r.currentOrigin, ent->pos1 );
	VectorCopy ( ent->pos1, ent->pos2 ); // don't go anywhere just yet

	trap_SetBrushModel ( ent, ent->model );

	InitMover ( ent );
	ent->reached = NULL;
	ent->s.animMovetype = 0;

	ent->s.density = 0;

	if ( ent->spawnflags & 256 )
	{
		ent->s.density |= 2;
	}

	if ( ent->spawnflags & 8 )
	{
		ent->use = script_mover_use;
	}

	if ( ent->spawnflags & 16 )
	{
		ent->s.time2 = 1;
	}
	else
	{
		ent->s.time2 = 0;
	}

	if ( ent->spawnflags & 32 )
	{
		ent->s.teamNum = TEAM_ALLIES;
	}
	else if ( ent->spawnflags & 64 )
	{
		ent->s.teamNum = TEAM_AXIS;
	}
	else
	{
		ent->s.teamNum = TEAM_FREE;
	}

	if ( ent->spawnflags & 1 )
	{
		ent->use = script_mover_use;
		trap_UnlinkEntity ( ent ); // make sure it's not visible
		return;
	}

	G_SetAngle ( ent, ent->s.angles );

	G_SpawnInt ( "health", "0", &ent->health );

	if ( ent->health )
	{
		ent->takedamage = qtrue;
		ent->count = ent->health;

		// client needs to know about it as well
		ent->s.effect1Time = ent->count;
		ent->s.dl_intensity = 255;

		if ( G_SpawnString ( "description", "", &s ) )
		{
			trap_GetConfigstring ( CS_SCRIPT_MOVER_NAMES, cs, sizeof ( cs ) );
			Info_SetValueForKey ( cs, va ( "%li", ( long ) ( ent - g_entities ) ), s );
			trap_SetConfigstring ( CS_SCRIPT_MOVER_NAMES, cs );
		}
	}
	else
	{
		ent->count = 0;
	}

	ent->die = script_mover_die;

	// look for general scaling
	if ( G_SpawnFloat ( "modelscale", "1", &scale[ 0 ] ) )
	{
		scale[ 2 ] = scale[ 1 ] = scale[ 0 ];
	}

	if ( G_SpawnString ( "model2", "", &modelname ) )
	{
		COM_StripExtension ( modelname, tagname );
		Q_strcat ( tagname, MAX_QPATH, ".tag" );

		ent->tagNumber = trap_LoadTag ( tagname );

		/*    if( !(ent->tagNumber = trap_LoadTag( tagname )) ) {
		                        Com_Error( ERR_DROP, "Failed to load Tag File (%s)\n", tagname );
		                }*/
	}

	// look for axis specific scaling
	if ( G_SpawnVector ( "modelscale_vec", "1 1 1", &scalevec[ 0 ] ) )
	{
		VectorCopy ( scalevec, scale );
	}

	if ( scale[ 0 ] != 1 || scale[ 1 ] != 1 || scale[ 2 ] != 1 )
	{
		ent->s.density |= 1;
		// scale is stored in 'angles2'
		VectorCopy ( scale, ent->s.angles2 );
	}

	if ( ent->spawnflags & 128 )
	{
		ent->s.density |= 4;
		ent->waterlevel = 0;

		if ( G_SpawnString ( "gun", "", &modelname ) )
		{
			if ( !Q_stricmp ( modelname, "browning" ) )
			{
				ent->s.density |= 8;
			}
		}

		G_SpawnString ( "tagent", "", &tagent );
		Q_strncpyz ( ent->tagBuffer, tagent, 16 );
		ent->s.powerups = -1;
	}

	ent->think = script_mover_spawn;
	ent->nextthink = level.time + FRAMETIME;
}

//..............................................................................

void script_model_med_spawn ( gentity_t *ent )
{
	if ( ent->spawnflags & 2 )
	{
		ent->clipmask = CONTENTS_SOLID;
		ent->r.contents = CONTENTS_SOLID;
	}

	ent->s.eType = ET_GENERAL;

	ent->s.modelindex = G_ModelIndex ( ent->model );
	ent->s.frame = 0;

	VectorCopy ( ent->s.origin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;

	trap_LinkEntity ( ent );
}

void script_model_med_use ( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	script_model_med_spawn ( ent );
}

/*QUAKED script_model_med (0.5 0.25 1.0) (-16 -16 -24) (16 16 64) TriggerSpawn Solid
MEDIUM SIZED scripted entity, used for animating a model, moving it around, etc
SOLID spawnflag means this entity will clip the player and AI, otherwise they can walk
straight through it
"model" the full path of the model to use
"scriptname" name used for scripting purposes (like aiName in AI scripting)
*/
void SP_script_model_med ( gentity_t *ent )
{
	if ( !ent->model )
	{
		G_Error ( "script_model_med %s must have a \"model\"\n", ent->scriptName );
	}

	if ( !ent->scriptName )
	{
		G_Error ( "script_model_med must have a \"scriptname\"\n" );
	}

	ent->s.eType = ET_GENERAL;
	ent->s.apos.trType = TR_STATIONARY;
	ent->s.apos.trTime = 0;
	ent->s.apos.trDuration = 0;
	VectorCopy ( ent->s.angles, ent->s.apos.trBase );
	VectorClear ( ent->s.apos.trDelta );

	if ( ent->spawnflags & 1 )
	{
		ent->use = script_model_med_use;
		trap_UnlinkEntity ( ent ); // make sure it's not visible
		return;
	}

	script_model_med_spawn ( ent );
}

//..............................................................................

/*QUAKED script_camera (1.0 0.25 1.0) (-8 -8 -8) (8 8 8) TriggerSpawn

  This is a camera entity. Used by the scripting to show cinematics, via special
  camera commands. See scripting documentation.

"scriptname" name used for scripting purposes (like aiName in AI scripting)
*/
void SP_script_camera ( gentity_t *ent )
{
	if ( !ent->scriptName )
	{
		G_Error ( "%s must have a \"scriptname\"\n", ent->classname );
	}

	ent->s.eType = ET_CAMERA;
	ent->s.apos.trType = TR_STATIONARY;
	ent->s.apos.trTime = 0;
	ent->s.apos.trDuration = 0;
	VectorCopy ( ent->s.angles, ent->s.apos.trBase );
	VectorClear ( ent->s.apos.trDelta );

	ent->s.frame = 0;

	ent->r.svFlags |= SVF_NOCLIENT; // only broadcast when in use
}

/*QUAKED script_multiplayer (1.0 0.25 1.0) (-8 -8 -8) (8 8 8)

  This is used to script multiplayer maps.  Entity not displayed in game.

// Gordon: also storing some stuff that will change often but needs to be broadcast, so we dont want to use a configstring

"scriptname" name used for scripting purposes (REQUIRED)
*/
void SP_script_multiplayer ( gentity_t *ent )
{
	ent->scriptName = "game_manager";

	// Gordon: broadcasting this to clients now, should be cheaper in bandwidth for sending landmine info
	ent->s.eType = ET_GAMEMANAGER;
	ent->r.svFlags = SVF_BROADCAST;

	if ( level.gameManager )
	{
		// Gordon: ok, making this an error now
		G_Error ( "^1ERROR: multiple script_multiplayers found^7\n" );
	}

	level.gameManager = ent;
	level.gameManager->s.otherEntityNum = MAX_TEAM_LANDMINES; // axis landmine count
	level.gameManager->s.otherEntityNum2 = MAX_TEAM_LANDMINES; // allies landmine count
	level.gameManager->s.modelindex = qfalse; // axis HQ doesn't exist
	level.gameManager->s.modelindex2 = qfalse; // allied HQ doesn't exist

	trap_LinkEntity ( ent );
}
