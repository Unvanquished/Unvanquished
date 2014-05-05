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

// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"

/*
=============
CG_Obituary
=============
*/
static const char teamTag[][8] = { "^2●^7", "^1●^7", "^4●^7" };

#define LONGFORM ">"
static const struct {
	char     icon[16];
	qboolean envKill;
	qboolean showAssist;
	team_t   team;
} meansOfDeath[] = {
	// Icon            Envkill Assist? (Team)
	{ "☠",             qfalse, qfalse },
	{ "[shotgun]",     qfalse, qtrue  },
	{ "[blaster]",     qfalse, qtrue  },
	{ "[painsaw]",     qfalse, qtrue  },
	{ "[rifle]",       qfalse, qtrue  },
	{ "[chaingun]",    qfalse, qtrue  },
	{ "[prifle]",      qfalse, qtrue  },
	{ "[mdriver]",     qfalse, qtrue  },
	{ "[lasgun]",      qfalse, qtrue  },
	{ "[lcannon]",     qfalse, qtrue  },
	{ "[lcannon]",     qfalse, qtrue  }, // splash
	{ "[flamer]",      qfalse, qtrue  },
	{ "[flamer]",      qfalse, qtrue  }, // splash
	{ "[flamer]",      qfalse, qtrue  }, // burn
	{ "[grenade]",     qfalse, qtrue  },
	{ "[firebomb]",    qfalse, qtrue  },
	{ "crushed",       qtrue,  qfalse }, // weight (H) // FIXME
	{ LONGFORM,        qtrue,  qfalse }, // water
	{ LONGFORM,        qtrue,  qfalse }, // slime
	{ LONGFORM,        qtrue,  qfalse }, // lava
	{ LONGFORM,        qtrue,  qfalse }, // crush
	{ "[telenode]",    qfalse, qfalse }, // telefrag
	{ LONGFORM,        qtrue,  qfalse }, // falling
	{ "☠",             qfalse, qfalse }, // suicide
	{ LONGFORM,        qtrue,  qfalse }, // target laser - shouldn't happen
	{ LONGFORM,        qtrue,  qfalse }, // trigger hurt

	{ "[granger]",     qfalse, qtrue  },
	{ "[dretch]",      qfalse, qtrue  },
	{ "[basilisk]",    qfalse, qtrue  },
	{ "[dragoon]",     qfalse, qtrue  },
	{ "[dragoon]",     qfalse, qtrue  }, // pounce
	{ "[advdragoon]",  qfalse, qtrue  },
	{ "[marauder]",    qfalse, qtrue  },
	{ "[advmarauder]", qfalse, qtrue  },
	{ "[tyrant]",      qfalse, qtrue  },
	{ "[tyrant]",      qfalse, qtrue  }, // trample
	{ "crushed",       qfalse, qtrue  }, // weight (A) // FIXME

	{ "[granger]",     qfalse, qtrue  }, // granger spit (slowblob)
	{ "[booster]",     qfalse, qtrue  }, // poison
	{ "[hive]",        qtrue,  qtrue,  TEAM_ALIENS },

	{ LONGFORM,        qtrue,  qfalse, TEAM_HUMANS }, // H spawn
	{ "[tesla]",       qtrue,  qtrue,  TEAM_HUMANS },
	{ "[turret]",      qtrue,  qtrue,  TEAM_HUMANS },
	{ "[reactor]",     qtrue,  qtrue,  TEAM_HUMANS },

	{ LONGFORM,        qtrue,  qfalse, TEAM_ALIENS }, // A spawn
	{ "[acidtube]",    qtrue,  qtrue,  TEAM_ALIENS },
	{ "[overmind]",    qtrue,  qtrue,  TEAM_ALIENS },
	{ "",              qtrue,  qfalse },
	{ "",              qtrue,  qfalse },
	{ "",              qtrue,  qfalse },
};

static void CG_Obituary( entityState_t *ent )
{
	int          mod;
	int          target, attacker, assistant;
	int          attackerClass = -1;
	const char   *message;
	const char   *messageAssisted = NULL;
	const char   *messageSuicide = NULL;
	const char   *targetInfo;
	const char   *attackerInfo;
	const char   *assistantInfo;
	char         targetName[ MAX_NAME_LENGTH ];
	char         attackerName[ MAX_NAME_LENGTH ];
	char         assistantName[ MAX_NAME_LENGTH ];
	gender_t     gender;
	clientInfo_t *ci;
	team_t       attackerTeam, assistantTeam = TEAM_NONE;

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	assistant = ent->otherEntityNum3;
	assistantTeam = (team_t) ( ent->generic1 & 0xFF ); // ugly hack allowing for future expansion(!)
	mod = ent->eventParm;

	if ( target < 0 || target >= MAX_CLIENTS )
	{
		CG_Error( "CG_Obituary: target out of range" );
	}

	ci = &cgs.clientinfo[ target ];
	gender = ci->gender;

	if ( attacker < 0 || attacker >= MAX_CLIENTS )
	{
		attacker = ENTITYNUM_WORLD;
		attackerInfo = NULL;
		attackerTeam = TEAM_NONE;
		strcpy( attackerName, "noname" );
	}
	else
	{
		attackerInfo = CG_ConfigString( CS_PLAYERS + attacker );
		attackerTeam = cgs.clientinfo[ attacker ].team;
		Q_strncpyz( attackerName, Info_ValueForKey( attackerInfo, "n" ), sizeof( attackerName ) );

		// check for kill messages about the current clientNum
		if ( target == cg.snap->ps.clientNum )
		{
			Q_strncpyz( cg.killerName, attackerName, sizeof( cg.killerName ) );
		}
	}

	if ( assistant < 0 || assistant >= MAX_CLIENTS )
	{
		assistantInfo = NULL;
	}
	else
	{
		assistantInfo = CG_ConfigString( CS_PLAYERS + assistant );
	}

	if ( assistantTeam < TEAM_NONE || assistantTeam >= NUM_TEAMS )
	{
		assistantTeam = TEAM_NONE;
	}

	if ( !assistantInfo )
	{
		strcpy( assistantName, "noname" );
	}
	else
	{
		Q_strncpyz( assistantName, Info_ValueForKey( assistantInfo, "n" ), sizeof( assistantName ) );
	}

	targetInfo = CG_ConfigString( CS_PLAYERS + target );

	if ( !targetInfo )
	{
		return;
	}

	Q_strncpyz( targetName, Info_ValueForKey( targetInfo, "n" ), sizeof( targetName ) );

	// check for single client messages

	if ( cg_emoticonsInMessages.integer )
	{
		if ( mod < MOD_UNKNOWN || mod >= ARRAY_LEN( meansOfDeath ) )
		{
			mod = MOD_UNKNOWN;
		}

		if ( meansOfDeath[ mod ].team )
		{
			attackerTeam = meansOfDeath[ mod ].team;
		}

		// if the long form is needed, show it; but we need a kill icon for this kill type
		if ( *meansOfDeath[ mod ].icon == '>' )
		{
				goto is_long_kill_message;
		}

		// if there's text for the kill type, print a kill message
		if ( *meansOfDeath[ mod ].icon )
		{
			if ( meansOfDeath[ mod ].envKill )
			{
				if ( meansOfDeath[ mod ].showAssist && assistantInfo )
				{
					CG_Printf( "%s (+ %s%s^7) %s %s%s\n", teamTag[ attackerTeam ], teamTag[ assistantTeam ], assistantName, meansOfDeath[ mod ].icon, teamTag[ ci->team ], targetName );
				}
				else
				{
					CG_Printf( "%s %s %s%s\n", teamTag[ attackerTeam ], meansOfDeath[ mod ].icon, teamTag[ ci->team ], targetName );
				}
			}
			else if ( attacker == target )
			{
				CG_Printf( "%s %s%s\n", meansOfDeath[ mod ].icon, teamTag[ ci->team ], targetName );
			}
			else
			{
				if ( meansOfDeath[ mod ].showAssist && assistantInfo )
				{
					CG_Printf( "%s%s^7 (+ %s%s^7) %s %s%s\n", teamTag[ attackerTeam ], attackerName, teamTag[ assistantTeam ], assistantName, meansOfDeath[ mod ].icon, teamTag[ ci->team ], targetName );
				}
				else
				{
					CG_Printf( "%s%s^7 %s %s%s\n", teamTag[ attackerTeam ], attackerName, meansOfDeath[ mod ].icon, teamTag[ ci->team ], targetName );
				}

				// nice big message for teamkills
				if ( attackerTeam == ci->team && attacker == cg.clientNum )
				{
					CG_CenterPrint( va( _("You killed ^1TEAMMATE^7 %s"), targetName ),
							SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
				}
			}
		}
	}
	else // Long form
	{
		is_long_kill_message: // <- use of this label == needs a kill icon

		// Messages which contain no killer name
		switch ( mod )
		{
			// Environmental kills

			case MOD_FALLING:
				message = G_( "%s%s ^7fell foul to gravity\n" );
				break;

			case MOD_CRUSH:
				message = G_( "%s%s ^7was squished\n" );
				break;

			case MOD_WATER:
				message = G_( "%s%s ^7forgot to pack a snorkel\n" );
				break;

			case MOD_SLIME:
				message = G_( "%s%s ^7melted\n" );
				break;

			case MOD_LAVA:
				message = G_( "%s%s ^7did a back flip into the lava\n" );
				break;

			case MOD_TRIGGER_HURT:
				message = G_( "%s%s ^7was in the wrong place\n" );
				break;

			// Building explosions

			case MOD_HSPAWN:
				message = G_( "%s%s ^7should have run further\n" );
				break;

			case MOD_ASPAWN:
				message = G_( "%s%s ^7shouldn't have trod in the acid\n" );
				break;

			// Attacked by a building

			case MOD_MGTURRET:
				message = G_( "%s%s ^7was gunned down by a turret\n" );
				messageAssisted = G_( "%s%s ^7was gunned down by a turret; %s%s^7 assisted\n" );
				break;

			case MOD_TESLAGEN:
				message = G_( "%s%s ^7was zapped by a tesla generator\n" );
				messageAssisted = G_( "%s%s ^7was zapped by a tesla generator; %s%s^7 assisted\n" );
				break;

			case MOD_ATUBE:
				message = G_( "%s%s ^7was melted by an acid tube\n" );
				messageAssisted = G_( "%s%s ^7was melted by an acid tube; %s%s^7 assisted\n" );
				break;

			case MOD_OVERMIND:
				message = G_( "%s%s ^7got too close to the overmind\n" );
				messageAssisted = G_( "%s%s ^7got too close to the overmind; %s%s^7 assisted\n" );
				break;

			case MOD_REACTOR:
				message = G_( "%s%s ^7got too close to the reactor\n" );
				messageAssisted = G_( "%s%s ^7got too close to the reactor; %s%s^7 assisted\n" );
				break;

			case MOD_SLOWBLOB:
				message = G_( "%s%s ^7should have visited a medical station\n" );
				messageAssisted = G_( "%s%s ^7should have visited a medical station; %s%s^7 assisted\n" );
				break;

			case MOD_SWARM:
				message = G_( "%s%s ^7was hunted down by the swarm\n" );
				messageAssisted = G_( "%s%s ^7was hunted down by the swarm; %s%s^7 assisted\n" );
				break;

			// Shouldn't happen

			case MOD_TARGET_LASER:
				message = G_( "%s%s ^7saw the light\n" );
				messageAssisted = G_( "%s%s ^7saw the light; %s%s^7 assisted\n" );
				break;

			default:
				message = NULL;
				break;
		}

		if ( message )
		{
			if ( messageAssisted && assistantInfo )
			{
				CG_Printf( messageAssisted, teamTag[ ci->team ], targetName , teamTag[ assistantTeam ], assistantName);
			}
			else
			{
				CG_Printf( message, teamTag[ ci->team ], targetName );
			}

			return;
		}

		// Messages which contain the killer's name
		switch ( mod )
		{
			case MOD_PAINSAW:
				message = G_( "%s%s ^7was sawn by %s%s\n" );
				messageAssisted = G_( "%s%s ^7was sawn by %s%s^7; %s%s^7 assisted\n" );
				break;

			case MOD_BLASTER:
				message = G_( "%s%s ^7was blasted by %s%s\n" );
				messageAssisted = G_( "%s%s ^7was blasted by %s%s^7; %s%s^7 assisted\n" );
				break;

			case MOD_MACHINEGUN:
				message = G_( "%s%s ^7was machinegunned by %s%s\n" );
				messageAssisted = G_( "%s%s ^7was machinegunned by %s%s^7; %s%s^7 assisted\n" );
				break;

			case MOD_CHAINGUN:
				message = G_( "%s%s ^7was chaingunned by %s%s\n" );
				messageAssisted = G_( "%s%s ^7was chaingunned by %s%s^7; %s%s^7 assisted\n" );
				break;

			case MOD_SHOTGUN:
				message = G_( "%s%s ^7was gunned down by %s%s\n" );
				messageAssisted = G_( "%s%s ^7was gunned down by %s%s^7; %s%s^7 assisted\n" );
				break;

			case MOD_PRIFLE:
				message = G_( "%s%s ^7was pulse rifled by %s%s\n" );
				messageAssisted = G_( "%s%s ^7was pulse rifled by %s%s^7; %s%s^7 assisted\n" );
				break;

			case MOD_MDRIVER:
				message = G_( "%s%s ^7was mass driven by %s%s\n" );
				messageAssisted = G_( "%s%s ^7was mass driven by %s%s^7; %s%s^7 assisted\n" );
				break;

			case MOD_LASGUN:
				message = G_( "%s%s ^7was lasgunned by %s%s\n" );
				messageAssisted = G_( "%s%s ^7was lasgunned by %s%s^7; %s%s^7 assisted\n" );
				break;

			case MOD_FLAMER:
			case MOD_FLAMER_SPLASH:
				message = G_( "%s%s ^7was grilled by %s%s^7's flamer\n" );
				messageAssisted = G_( "%s%s ^7was grilled by %s%s^7's flamer; %s%s^7 assisted\n" );
				messageSuicide = G_( "%s%s ^7grilled self" );
				break;

			case MOD_BURN:
				message = G_( "%s%s ^7was burned by %s%s^7's fire\n" );
				messageAssisted = G_( "%s%s ^7was burned by %s%s^7's fire; %s%s^7 assisted\n" );
				messageSuicide = G_( "%s%s ^7burned self" );
				break;

			case MOD_LCANNON:
				message = G_( "%s%s ^7felt the full force of %s%s^7's lucifer cannon\n" );
				messageAssisted = G_( "%s%s ^7felt the full force of %s%s^7's lucifer cannon; %s%s^7 assisted\n" );
				break;

			case MOD_LCANNON_SPLASH:
				message = G_( "%s%s ^7was caught in the fallout of %s%s^7's lucifer cannon\n" );
				messageAssisted = G_( "%s%s ^7was caught in the fallout of %s%s^7's lucifer cannon; %s%s^7 assisted\n" );
				messageSuicide = G_( "%s%s ^7was caught in the fallout of own lucifer cannon" );
				break;

			case MOD_GRENADE:
				message = G_( "%s%s ^7couldn't escape %s%s^7's grenade\n" );
				messageAssisted = G_( "%s%s ^7couldn't escape %s%s^7's grenade; %s%s^7 assisted\n" );
				messageSuicide = G_( "%s%s ^7blew self up" );
				break;

			case MOD_FIREBOMB:
				message = G_( "%s%s ^7couldn't escape %s%s^7's firebomb\n" );
				messageAssisted = G_( "%s%s ^7couldn't escape %s%s^7's firebomb; %s%s^7 assisted\n" );
				messageSuicide = G_( "%s%s ^7incinerated self" );
				break;

			case MOD_ABUILDER_CLAW:
				message = G_( "%s%s ^7should leave %s%s^7's buildings alone\n" );
				messageAssisted = G_( "%s%s ^7should leave %s%s^7's buildings alone; %s%s^7 assisted\n" );
				break;

			case MOD_LEVEL0_BITE:
				message = G_( "%s%s ^7was bitten by %s%s\n" );
				messageAssisted = G_( "%s%s ^7was bitten by %s%s^7; %s%s^7 assisted\n" );
				break;

			case MOD_LEVEL1_CLAW:
				message = G_( "%s%s ^7was swiped by %s%s^7's %s\n" );
				messageAssisted = G_( "%s%s ^7was swiped by %s%s^7's %s^7; %s%s^7 assisted\n" );
				attackerClass = PCL_ALIEN_LEVEL1;
				break;

			case MOD_LEVEL2_CLAW:
				message = G_( "%s%s ^7was clawed by %s%s^7's %s\n" );
				messageAssisted = G_( "%s%s ^7was clawed by %s%s^7's %s^7; %s%s^7 assisted\n" );
				attackerClass = PCL_ALIEN_LEVEL2;
				break;

			case MOD_LEVEL2_ZAP:
				message = G_( "%s%s ^7was zapped by %s%s^7's %s\n" );
				messageAssisted = G_( "%s%s ^7was zapped by %s%s^7's %s^7; %s%s^7 assisted\n" );
				attackerClass = PCL_ALIEN_LEVEL2;
				break;

			case MOD_LEVEL3_CLAW:
				message = G_( "%s%s ^7was chomped by %s%s^7's %s\n" );
				messageAssisted = G_( "%s%s ^7was chomped by %s%s^7's %s^7; %s%s^7 assisted\n" );
				attackerClass = PCL_ALIEN_LEVEL3;
				break;

			case MOD_LEVEL3_POUNCE:
				message = G_( "%s%s ^7was pounced upon by %s%s^7's %s\n" );
				messageAssisted = G_( "%s%s ^7was pounced upon by %s%s^7's %s^7; %s%s^7 assisted\n" );
				attackerClass = PCL_ALIEN_LEVEL3;
				break;

			case MOD_LEVEL3_BOUNCEBALL:
				message = G_( "%s%s ^7was sniped by %s%s^7's %s\n" );
				messageAssisted = G_( "%s%s ^7was sniped by %s%s^7's %s^7; %s%s^7 assisted\n" );
				messageSuicide = G_( "%s%s ^7sniped self" );
				attackerClass = PCL_ALIEN_LEVEL3;
				break;

			case MOD_LEVEL4_CLAW:
				message = G_( "%s%s ^7was mauled by %s%s^7's %s\n" );
				messageAssisted = G_( "%s%s ^7was mauled by %s%s^7's %s^7; %s%s^7 assisted\n" );
				attackerClass = PCL_ALIEN_LEVEL4;
				break;

			case MOD_LEVEL4_TRAMPLE:
				message = G_( "%s%s ^7should have gotten out of the way of %s%s^7's %s\n" );
				messageAssisted = G_( "%s%s ^7should have gotten out of the way of %s%s^7's %s^7; %s%s^7 assisted\n" );
				attackerClass = PCL_ALIEN_LEVEL4;
				break;

			case MOD_WEIGHT_H:
			case MOD_WEIGHT_A:
				message = G_( "%s%s ^7was crushed under %s%s^7's weight\n" );
				messageAssisted = G_( "%s%s ^7was crushed under %s%s^7's weight; %s%s^7 assisted\n" );
				break;

			case MOD_POISON:
				message = G_( "%s%s ^7should have used a medkit against %s%s^7's poison\n" );
				messageAssisted = G_( "%s%s ^7should have used a medkit against %s%s^7's poison; %s%s^7 assisted\n" );
				break;

			case MOD_TELEFRAG:
				message = G_( "%s%s ^7tried to invade %s%s^7's personal space\n" );
				break;

			default:
				message = G_( "%s%s ^7was killed by %s%s\n" );
				messageAssisted = G_( "%s%s ^7was killed by %s%s^7 and %s%s\n" );
				messageSuicide = G_( "%s%s ^7killed self\n" );
				break;
		}

		if ( message )
		{
			// Argument order: victim, attacker, [class,] [assistant]. Each has team tag first.
			if ( messageSuicide && attacker == target )
			{
				CG_Printf( messageSuicide, teamTag[ ci->team ], targetName );
			}
			else if ( messageAssisted && assistantInfo )
			{
				if ( attackerClass != -1 )
				{
					CG_Printf( messageAssisted, teamTag[ ci->team ], targetName, teamTag[ attackerTeam ], attackerName, _( BG_ClassModelConfig( attackerClass )->humanName ), teamTag[ assistantTeam ], assistantName );
				}
				else
				{
					CG_Printf( messageAssisted, teamTag[ ci->team ], targetName, teamTag[ attackerTeam ], attackerName, teamTag[ assistantTeam ], assistantName );
				}
			}
			else
			{
				CG_Printf( message, teamTag[ ci->team ], targetName, teamTag[ attackerTeam ], attackerName, ( attackerClass != -1 ) ? _( BG_ClassModelConfig( attackerClass )->humanName ) : NULL );
			}

			if ( attackerTeam == ci->team && attacker == cg.clientNum && attacker != target )
			{
				CG_CenterPrint( va( _("You killed ^1TEAMMATE^7 %s"), targetName ),
						SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
			}

			return;
		}

		// we don't know what it was
		CG_Printf( G_( "%s%s^7 died\n" ), teamTag[ ci->team ], targetName );
	}
}

//==========================================================================

/*
================
CG_PainEvent

Also called by playerstate transition
================
*/
void CG_PainEvent( centity_t *cent, int health )
{
	char *snd;

	// don't do more than two pain sounds a second
	if ( cg.time - cent->pe.painTime < 500 )
	{
		return;
	}

	if ( health < 25 )
	{
		snd = "*pain25_1.wav";
	}
	else if ( health < 50 )
	{
		snd = "*pain50_1.wav";
	}
	else if ( health < 75 )
	{
		snd = "*pain75_1.wav";
	}
	else
	{
		snd = "*pain100_1.wav";
	}

	trap_S_StartSound( NULL, cent->currentState.number, CHAN_VOICE,
	                   CG_CustomSound( cent->currentState.number, snd ) );

	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}

/*
=========================
CG_Level2Zap
=========================
*/
static void CG_Level2Zap( entityState_t *es )
{
	int       i;
	centity_t *source = NULL, *target = NULL;

	if ( es->misc < 0 || es->misc >= MAX_CLIENTS )
	{
		return;
	}

	source = &cg_entities[ es->misc ];

	for ( i = 0; i <= 2; i++ )
	{
		switch ( i )
		{
			case 0:
				if ( es->time <= 0 )
				{
					continue;
				}

				target = &cg_entities[ es->time ];
				break;

			case 1:
				if ( es->time2 <= 0 )
				{
					continue;
				}

				target = &cg_entities[ es->time2 ];
				break;

			case 2:
				if ( es->constantLight <= 0 )
				{
					continue;
				}

				target = &cg_entities[ es->constantLight ];
				break;
		}

		if ( !CG_IsTrailSystemValid( &source->level2ZapTS[ i ] ) )
		{
			source->level2ZapTS[ i ] = CG_SpawnNewTrailSystem( cgs.media.level2ZapTS );
		}

		if ( CG_IsTrailSystemValid( &source->level2ZapTS[ i ] ) )
		{
			CG_SetAttachmentCent( &source->level2ZapTS[ i ]->frontAttachment, source );
			CG_SetAttachmentCent( &source->level2ZapTS[ i ]->backAttachment, target );
			CG_AttachToCent( &source->level2ZapTS[ i ]->frontAttachment );
			CG_AttachToCent( &source->level2ZapTS[ i ]->backAttachment );
		}
	}

	source->level2ZapTime = cg.time;
}

/*
==============
CG_Momentum

Notify player of generated momentum
==============
*/
void CG_Momentum( entityState_t *es )
{
	float                  momentum;
	qboolean               negative;

	negative   = es->groundEntityNum;
	momentum = ( negative ? -es->otherEntityNum2 : es->otherEntityNum2 ) / 10.0f;

	cg.momentumGained     = momentum;
	cg.momentumGainedTime = cg.time;
}

/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
void CG_EntityEvent( centity_t *cent, vec3_t position )
{
	entityState_t *es;
	int           event;
	vec3_t        dir;
	const char    *s;
	int           clientNum;
	clientInfo_t  *ci;
	int           steptime;

	if ( cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
	{
		steptime = 200;
	}
	else
	{
		steptime = BG_Class( cg.snap->ps.stats[ STAT_CLASS ] )->steptime;
	}

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if ( cg_debugEvents.integer )
	{
		CG_Printf( "ent:%3i  event:%3i %s\n", es->number, event,
		           BG_EventName( event ) );
	}

	if ( !event )
	{
		return;
	}

	clientNum = es->clientNum;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS )
	{
		clientNum = 0;
	}

	ci = &cgs.clientinfo[ clientNum ];

	switch ( event )
	{
		case EV_FOOTSTEP:
			if ( cg_footsteps.integer && ci->footsteps != FOOTSTEP_NONE )
			{
				if ( ci->footsteps == FOOTSTEP_CUSTOM )
				{
					trap_S_StartSound( NULL, es->number, CHAN_BODY,
					                   ci->customFootsteps[ rand() & 3 ] );
				}
				else
				{
					trap_S_StartSound( NULL, es->number, CHAN_BODY,
					                   cgs.media.footsteps[ ci->footsteps ][ rand() & 3 ] );
				}
			}

			break;

		case EV_FOOTSTEP_METAL:
			if ( cg_footsteps.integer && ci->footsteps != FOOTSTEP_NONE )
			{
				if ( ci->footsteps == FOOTSTEP_CUSTOM )
				{
					trap_S_StartSound( NULL, es->number, CHAN_BODY,
					                   ci->customMetalFootsteps[ rand() & 3 ] );
				}
				else
				{
					trap_S_StartSound( NULL, es->number, CHAN_BODY,
					                   cgs.media.footsteps[ FOOTSTEP_METAL ][ rand() & 3 ] );
				}
			}

			break;

		case EV_FOOTSTEP_SQUELCH:
			if ( cg_footsteps.integer && ci->footsteps != FOOTSTEP_NONE )
			{
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
				                   cgs.media.footsteps[ FOOTSTEP_FLESH ][ rand() & 3 ] );
			}

			break;

		case EV_FOOTSPLASH:
			if ( cg_footsteps.integer && ci->footsteps != FOOTSTEP_NONE )
			{
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
				                   cgs.media.footsteps[ FOOTSTEP_SPLASH ][ rand() & 3 ] );
			}

			break;

		case EV_FOOTWADE:
			if ( cg_footsteps.integer && ci->footsteps != FOOTSTEP_NONE )
			{
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
				                   cgs.media.footsteps[ FOOTSTEP_SPLASH ][ rand() & 3 ] );
			}

			break;

		case EV_SWIM:
			if ( cg_footsteps.integer && ci->footsteps != FOOTSTEP_NONE )
			{
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
				                   cgs.media.footsteps[ FOOTSTEP_SPLASH ][ rand() & 3 ] );
			}

			break;

		case EV_FALL_SHORT:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound );

			if ( clientNum == cg.predictedPlayerState.clientNum )
			{
				// smooth landing z changes
				cg.landChange = -8;
				cg.landTime = cg.time;
			}

			break;

		case EV_FALL_MEDIUM:
			// use a general pain sound
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*pain100_1.wav" ) );

			if ( clientNum == cg.predictedPlayerState.clientNum )
			{
				// smooth landing z changes
				cg.landChange = -16;
				cg.landTime = cg.time;
			}

			break;

		case EV_FALL_FAR:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall1.wav" ) );
			cent->pe.painTime = cg.time; // don't play a pain sound right after this

			if ( clientNum == cg.predictedPlayerState.clientNum )
			{
				// smooth landing z changes
				cg.landChange = -24;
				cg.landTime = cg.time;
			}

			break;

		case EV_FALLING:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*falling1.wav" ) );
			break;

		case EV_STEP_4:
		case EV_STEP_8:
		case EV_STEP_12:
		case EV_STEP_16: // smooth out step up transitions
		case EV_STEPDN_4:
		case EV_STEPDN_8:
		case EV_STEPDN_12:
		case EV_STEPDN_16: // smooth out step down transitions
			{
				float oldStep;
				int   delta;
				int   step;

				if ( clientNum != cg.predictedPlayerState.clientNum )
				{
					break;
				}

				// if we are interpolating, we don't need to smooth steps
				if ( cg.demoPlayback || ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ||
				     cg_nopredict.integer || cg_synchronousClients.integer )
				{
					break;
				}

				// check for stepping up before a previous step is completed
				delta = cg.time - cg.stepTime;

				if ( delta < steptime )
				{
					oldStep = cg.stepChange * ( steptime - delta ) / steptime;
				}
				else
				{
					oldStep = 0;
				}

				// add this amount
				if ( event >= EV_STEPDN_4 )
				{
					step = 4 * ( event - EV_STEPDN_4 + 1 );
					cg.stepChange = oldStep - step;
				}
				else
				{
					step = 4 * ( event - EV_STEP_4 + 1 );
					cg.stepChange = oldStep + step;
				}

				if ( cg.stepChange > MAX_STEP_CHANGE )
				{
					cg.stepChange = MAX_STEP_CHANGE;
				}
				else if ( cg.stepChange < -MAX_STEP_CHANGE )
				{
					cg.stepChange = -MAX_STEP_CHANGE;
				}

				cg.stepTime = cg.time;
				break;
			}

		case EV_JUMP:
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );

			if ( BG_ClassHasAbility( cg.predictedPlayerState.stats[ STAT_CLASS ], SCA_WALLJUMPER ) )
			{
				vec3_t surfNormal, refNormal = { 0.0f, 0.0f, 1.0f };
				vec3_t rotAxis;

				if ( clientNum != cg.predictedPlayerState.clientNum )
				{
					break;
				}

				//set surfNormal
				VectorCopy( cg.predictedPlayerState.grapplePoint, surfNormal );

				//if we are moving from one surface to another smooth the transition
				if ( !VectorCompare( surfNormal, cg.lastNormal ) && surfNormal[ 2 ] != 1.0f )
				{
					CrossProduct( refNormal, surfNormal, rotAxis );
					VectorNormalize( rotAxis );

					//add the op
					CG_addSmoothOp( rotAxis, 15.0f, 1.0f );
				}

				//copy the current normal to the lastNormal
				VectorCopy( surfNormal, cg.lastNormal );
			}

			break;

		case EV_LEV4_TRAMPLE_PREPARE:
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, cgs.media.alienL4ChargePrepare );
			break;

		case EV_LEV4_TRAMPLE_START:
			//FIXME: stop cgs.media.alienL4ChargePrepare playing here
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, cgs.media.alienL4ChargeStart );
			break;

		case EV_TAUNT:
			if ( !cg_noTaunt.integer )
			{
				trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*taunt.wav" ) );
			}

			break;

		case EV_WATER_TOUCH:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.watrInSound );
			break;

		case EV_WATER_LEAVE:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );
			break;

		case EV_WATER_UNDER:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound );
			break;

		case EV_WATER_CLEAR:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*gasp.wav" ) );
			break;

		case EV_JETPACK_ENABLE:
      cent->jetpackAnim = JANIM_SLIDEOUT;
			break;

		case EV_JETPACK_DISABLE:
      cent->jetpackAnim = JANIM_SLIDEIN;
			break;

		case EV_JETPACK_IGNITE:
			// TODO: Play jetpack ignite gfx/sfx
			break;

		case EV_JETPACK_START:
			// TODO: Start jetpack thrust gfx/sfx
			break;

		case EV_JETPACK_STOP:
			// TODO: Stop jetpack thrust gfx/sfx
			break;

		case EV_NOAMMO:
			trap_S_StartSound( NULL, es->number, CHAN_WEAPON, cgs.media.weaponEmptyClick );
			break;

		case EV_CHANGE_WEAPON:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.selectSound );
			break;

		case EV_FIRE_WEAPON:
			CG_HandleFireWeapon( cent, WPM_PRIMARY );
			break;

		case EV_FIRE_WEAPON2:
			CG_HandleFireWeapon( cent, WPM_SECONDARY );
			break;

		case EV_FIRE_WEAPON3:
			CG_HandleFireWeapon( cent, WPM_TERTIARY );
			break;

		case EV_WEAPON_RELOAD:
			if ( cg_weapons[ es->eventParm ].wim[ WPM_PRIMARY ].reloadSound )
			{
				trap_S_StartSound( NULL, es->number, CHAN_WEAPON, cg_weapons[ es->eventParm ].wim[ WPM_PRIMARY ].reloadSound );
			}
			break;

		case EV_PLAYER_TELEPORT_IN:
			//deprecated
			break;

		case EV_PLAYER_TELEPORT_OUT:
			CG_PlayerDisconnect( position );
			break;

		case EV_BUILD_CONSTRUCT:
			break;

		case EV_BUILD_DESTROY:
			break;

		case EV_AMMO_REFILL:
		case EV_CLIPS_REFILL:
		case EV_FUEL_REFILL:
			// TODO: Add different sounds for EV_AMMO_REFILL, EV_CLIPS_REFILL, EV_FUEL_REFILL
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.repeaterUseSound );
			break;

		case EV_GRENADE_BOUNCE:
			if ( rand() & 1 )
			{
				trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.hardBounceSound1 );
			}
			else
			{
				trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.hardBounceSound2 );
			}
			break;

		case EV_WEAPON_HIT_ENTITY:
			CG_HandleWeaponHitEntity( es, position );
			break;

		case EV_WEAPON_HIT_ENVIRONMENT:
			CG_HandleWeaponHitWall( es, position );
			break;

		case EV_MISSILE_HIT_ENTITY:
			CG_HandleMissileHitEntity( es, position );
			break;

		// currently there is no support for metal sounds
		case EV_MISSILE_HIT_ENVIRONMENT:
		case EV_MISSILE_HIT_METAL:
			CG_HandleMissileHitWall( es, position );
			break;

		case EV_SHOTGUN:
			CG_HandleFireShotgun( es );
			break;

		case EV_HUMAN_BUILDABLE_DYING:
			CG_HumanBuildableDying( (buildable_t) es->modelindex, position );
			break;

		case EV_HUMAN_BUILDABLE_EXPLOSION:
			ByteToDir( es->eventParm, dir );
			CG_HumanBuildableExplosion( (buildable_t) es->modelindex, position, dir );
			break;

		case EV_ALIEN_BUILDABLE_EXPLOSION:
			ByteToDir( es->eventParm, dir );
			CG_AlienBuildableExplosion( position, dir );
			break;

		case EV_TESLATRAIL:
			cent->currentState.weapon = WP_TESLAGEN;
			{
				centity_t *source = &cg_entities[ es->generic1 ];
				centity_t *target = &cg_entities[ es->clientNum ];
				vec3_t    sourceOffset = { 0.0f, 0.0f, 28.0f };

				if ( !CG_IsTrailSystemValid( &source->muzzleTS ) )
				{
					source->muzzleTS = CG_SpawnNewTrailSystem( cgs.media.teslaZapTS );

					if ( CG_IsTrailSystemValid( &source->muzzleTS ) )
					{
						CG_SetAttachmentCent( &source->muzzleTS->frontAttachment, source );
						CG_SetAttachmentCent( &source->muzzleTS->backAttachment, target );
						CG_AttachToCent( &source->muzzleTS->frontAttachment );
						CG_AttachToCent( &source->muzzleTS->backAttachment );
						CG_SetAttachmentOffset( &source->muzzleTS->frontAttachment, sourceOffset );

						source->muzzleTSDeathTime = cg.time + cg_teslaTrailTime.integer;
					}
				}
			}
			break;

		case EV_GENERAL_SOUND:
			if ( cgs.gameSounds[ es->eventParm ] )
			{
				trap_S_StartSound( NULL, es->number, CHAN_VOICE, cgs.gameSounds[ es->eventParm ] );
			}
			else
			{
				s = CG_ConfigString( CS_SOUNDS + es->eventParm );
				trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, s ) );
			}

			break;

		case EV_GLOBAL_SOUND: // play from the player's head so it never diminishes
			if ( cgs.gameSounds[ es->eventParm ] )
			{
				trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[ es->eventParm ] );
			}
			else
			{
				s = CG_ConfigString( CS_SOUNDS + es->eventParm );
				trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, CG_CustomSound( es->number, s ) );
			}

			break;

		case EV_PAIN:
			// local player sounds are triggered in CG_CheckLocalSounds,
			// so ignore events on the player
			if ( cent->currentState.number != cg.snap->ps.clientNum )
			{
				CG_PainEvent( cent, es->eventParm );
			}

			break;

		case EV_DEATH1:
		case EV_DEATH2:
		case EV_DEATH3:
			trap_S_StartSound( NULL, es->number, CHAN_VOICE,
			                   CG_CustomSound( es->number, va( "*death%i.wav", event - EV_DEATH1 + 1 ) ) );
			break;

		case EV_OBITUARY:
			CG_Obituary( es );
			break;

		case EV_GIB_PLAYER:
			// no gibbing
			break;

		case EV_STOPLOOPINGSOUND:
			trap_S_StopLoopingSound( es->number );
			es->loopSound = 0;
			break;

		case EV_BUILD_DELAY:
			if ( clientNum == cg.predictedPlayerState.clientNum )
			{
				trap_S_StartLocalSound( cgs.media.buildableRepairedSound, CHAN_LOCAL_SOUND );
				cg.lastBuildAttempt = cg.time;
			}

			break;

		case EV_BUILD_REPAIR:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.buildableRepairSound );
			break;

		case EV_BUILD_REPAIRED:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.buildableRepairedSound );
			break;

		case EV_OVERMIND_ATTACK_1:
		case EV_OVERMIND_ATTACK_2:
			if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_ALIENS )
			{
				trap_S_StartLocalSound( cgs.media.alienOvermindAttack, CHAN_ANNOUNCER );
				CG_CenterPrint( va( "^%c%s", "31"[ event - EV_OVERMIND_ATTACK_1 ], _( "The Overmind is under attack!" ) ), 200, GIANTCHAR_WIDTH * 4 );
			}

			break;

		case EV_OVERMIND_DYING:
			if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_ALIENS )
			{
				trap_S_StartLocalSound( cgs.media.alienOvermindDying, CHAN_ANNOUNCER );
				CG_CenterPrint( _( "^1The Overmind is dying!" ), 200, GIANTCHAR_WIDTH * 4 );
			}

			break;

		case EV_REACTOR_ATTACK_1:
		case EV_REACTOR_ATTACK_2:
			if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_HUMANS )
			{
				CG_CenterPrint( va( "^%c%s", "31"[ event - EV_REACTOR_ATTACK_1 ], _( "The reactor is under attack!" ) ), 200, GIANTCHAR_WIDTH * 4 );
			}

			break;

		case EV_REACTOR_DYING:
			if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_HUMANS )
			{
				CG_CenterPrint( _( "^1The reactor is about to explode!" ), 200, GIANTCHAR_WIDTH * 4 );
			}

			break;

		case EV_WARN_ATTACK:
			// if eventParm is non-zero, this is for humans and there's a nearby reactor or repeater, otherwise it's for aliens
			if ( es->eventParm >= MAX_CLIENTS && es->eventParm < MAX_GENTITIES )
			{
				const char *location;
				qboolean    base = cg_entities[ es->eventParm ].currentState.modelindex == BA_H_REACTOR;
				centity_t  *locent = CG_GetLocation( cg_entities[ es->eventParm ].currentState.origin );

				CG_CenterPrint( base ? _( "Our base is under attack!" ) : _( "A forward base is under attack!" ), 200, GIANTCHAR_WIDTH * 4 );

				if ( locent )
				{
					location = CG_ConfigString( CS_LOCATIONS + locent->currentState.generic1 );
				}
				else
				{
					location = CG_ConfigString( CS_LOCATIONS );
				}

				if ( location && *location )
				{
					Com_Printf( _( "%s Under attack – %s\n" ), base ? "[reactor]" : "[repeater]", location );
				}
				else
				{
					Com_Printf( _( "%s Under attack\n" ), base ? "[reactor]" : "[repeater]" );
				}
			}
			else // this is for aliens, and the overmind is in range
			{
				CG_CenterPrint( _( "Our base is under attack!" ), 200, GIANTCHAR_WIDTH * 4 );
			}

			break;

		case EV_MGTURRET_SPINUP:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.turretSpinupSound );
			break;

		case EV_OVERMIND_SPAWNS:
			if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_ALIENS )
			{
				trap_S_StartLocalSound( cgs.media.alienOvermindSpawns, CHAN_ANNOUNCER );
				CG_CenterPrint( "The Overmind needs spawns!", 200, GIANTCHAR_WIDTH * 4 );
			}

			break;

		case EV_ALIEN_EVOLVE:
			trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.alienEvolveSound );
			{
				particleSystem_t *ps = CG_SpawnNewParticleSystem( cgs.media.alienEvolvePS );

				if ( CG_IsParticleSystemValid( &ps ) )
				{
					CG_SetAttachmentCent( &ps->attachment, cent );
					CG_AttachToCent( &ps->attachment );
				}
			}

			if ( es->number == cg.clientNum )
			{
				CG_ResetPainBlend();
				cg.spawnTime = cg.time;
			}

			break;

		case EV_ALIEN_EVOLVE_FAILED:
			if ( clientNum == cg.predictedPlayerState.clientNum )
			{
				//FIXME: change to "negative" sound
				trap_S_StartLocalSound( cgs.media.buildableRepairedSound, CHAN_LOCAL_SOUND );
				cg.lastEvolveAttempt = cg.time;
			}

			break;

		case EV_ALIEN_ACIDTUBE:
			{
				particleSystem_t *ps = CG_SpawnNewParticleSystem( cgs.media.alienAcidTubePS );

				if ( CG_IsParticleSystemValid( &ps ) )
				{
					CG_SetAttachmentCent( &ps->attachment, cent );
					ByteToDir( es->eventParm, dir );
					CG_SetParticleSystemNormal( ps, dir );
					CG_AttachToCent( &ps->attachment );
				}
			}
			break;

		case EV_ALIEN_BOOSTER:
			{
				particleSystem_t *ps = CG_SpawnNewParticleSystem( cgs.media.alienBoosterPS );

				if ( CG_IsParticleSystemValid( &ps ) )
				{
					CG_SetAttachmentCent( &ps->attachment, cent );
					ByteToDir( es->eventParm, dir );
					CG_SetParticleSystemNormal( ps, dir );
					CG_AttachToCent( &ps->attachment );
				}
			}
			break;

		case EV_MEDKIT_USED:
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.medkitUseSound );
			break;

		case EV_PLAYER_RESPAWN:
			if ( es->number == cg.clientNum )
			{
				cg.spawnTime = cg.time;
			}

			break;

		case EV_LEV2_ZAP:
			CG_Level2Zap( es );
			break;

		case EV_HIT:
			cg.hitTime = cg.time;
			break;

		case EV_MOMENTUM:
			CG_Momentum( es );
			break;

		default:
			CG_Error( "Unknown event: %i", event );
	}
}

/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents( centity_t *cent )
{
	entity_event_t event;
	entity_event_t oldEvent = EV_NONE;

	// check for event-only entities
	if ( cent->currentState.eType > ET_EVENTS )
	{
		event = (entity_event_t) ( cent->currentState.eType - ET_EVENTS );

		if ( cent->previousEvent )
		{
			return; // already fired
		}

		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;

		// Move the pointer to the entity that the
		// event was originally attached to
		if ( cent->currentState.eFlags & EF_PLAYER_EVENT )
		{
			cent = &cg_entities[ cent->currentState.otherEntityNum ];
			oldEvent = (entity_event_t) cent->currentState.event;
			cent->currentState.event = event;
		}
	}
	else
	{
		// check for events riding with another entity
		if ( cent->currentState.event == cent->previousEvent )
		{
			return;
		}

		cent->previousEvent = cent->currentState.event;

		if ( ( cent->currentState.event & ~EV_EVENT_BITS ) == 0 )
		{
			return;
		}
	}

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin );
	CG_SetEntitySoundPosition( cent );

	CG_EntityEvent( cent, cent->lerpOrigin );

	// If this was a reattached spilled event, restore the original event
	if ( oldEvent != EV_NONE )
	{
		cent->currentState.event = oldEvent;
	}
}
