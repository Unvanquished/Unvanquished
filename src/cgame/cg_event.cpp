/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"
#include "rocket/rocket.h"

/*
=============
CG_Obituary
=============
*/
// end on WHITE instead of ESC for player names following the tags
static const char teamTag[][8] = { "^2●^7", "^1●^7", "^4●^7" };

#define LONGFORM ">"
static const struct {
	char     icon[16];
	bool envKill;
	bool showAssist;
	team_t   team;
} meansOfDeath[] = {
	// Icon            Envkill Assist? (Team)
	{ "☠",             false, false, TEAM_NONE   }, // unknown
	{ "[shotgun]",     false, true,  TEAM_HUMANS },
	{ "[blaster]",     false, true,  TEAM_HUMANS },
	{ "[painsaw]",     false, true,  TEAM_HUMANS },
	{ "[rifle]",       false, true,  TEAM_HUMANS },
	{ "[chaingun]",    false, true,  TEAM_HUMANS },
	{ "[prifle]",      false, true,  TEAM_HUMANS },
	{ "[mdriver]",     false, true,  TEAM_HUMANS },
	{ "[lasgun]",      false, true,  TEAM_HUMANS },
	{ "[lcannon]",     false, true,  TEAM_HUMANS },
	{ "[lcannon]",     false, true,  TEAM_HUMANS }, // splash
	{ "[flamer]",      false, true,  TEAM_HUMANS },
	{ "[flamer]",      false, true,  TEAM_HUMANS }, // splash
	{ "[fire]",        false, true,  TEAM_HUMANS }, // burn
	{ "[grenade]",     false, true,  TEAM_HUMANS },
	{ "[firebomb]",    false, true,  TEAM_HUMANS },
	{ "crushed",       false, true,  TEAM_HUMANS }, // crushed by human player. FIXME: emoticon?
	{ LONGFORM,        true,  false, TEAM_NONE   }, // water
	{ LONGFORM,        true,  false, TEAM_NONE   }, // slime
	{ LONGFORM,        true,  false, TEAM_NONE   }, // lava
	{ LONGFORM,        true,  false, TEAM_NONE   }, // crush
	{ "[telenode]",    false, false, TEAM_NONE   }, // telefrag
	{ "[cross]",       false, false, TEAM_NONE   }, // Admin Authority™ (/slap command)
	{ LONGFORM,        true,  false, TEAM_NONE   }, // falling
	{ "☠",             false, false, TEAM_NONE   }, // suicide
	{ LONGFORM,        true,  false, TEAM_NONE   }, // target laser - shouldn't happen
	{ LONGFORM,        true,  false, TEAM_NONE   }, // trigger hurt

	{ "[granger]",     false, true,  TEAM_ALIENS }, // granger claw
	{ "[dretch]",      false, true,  TEAM_ALIENS },
	{ "[mantis]",      false, true,  TEAM_ALIENS },
	{ "[dragoon]",     false, true,  TEAM_ALIENS },
	{ "[dragoon]",     false, true,  TEAM_ALIENS }, // pounce
	{ "[advdragoon]",  false, true,  TEAM_ALIENS }, // barbs
	{ "[marauder]",    false, true,  TEAM_ALIENS },
	{ "[advmarauder]", false, true,  TEAM_ALIENS }, // zap
	{ "[tyrant]",      false, true,  TEAM_ALIENS },
	{ "[tyrant]",      false, true,  TEAM_ALIENS }, // trample
	{ "crushed",       false, true,  TEAM_ALIENS }, // crushed by alien player. FIXME: emoticon?

	{ "[advgranger]",  false, true,  TEAM_ALIENS }, // granger spit (slowblob)
	{ "[booster]",     false, true,  TEAM_ALIENS }, // poison
	{ "[hive]",        true,  true,  TEAM_ALIENS },

	{ LONGFORM,        true,  false, TEAM_HUMANS }, // H spawn (human building explosion)
	{ "^dR^*[turret]", true,  true,  TEAM_HUMANS }, // rocket pod. FIXME: we need a rocket pod emoticon
	{ "[turret]",      true,  true,  TEAM_HUMANS },
	{ "[reactor]",     true,  true,  TEAM_HUMANS },

	{ LONGFORM,        true,  false, TEAM_ALIENS }, // A spawn (alien building explosion)
	{ "[acidtube]",    true,  true,  TEAM_ALIENS },
	{ "[hovel]",       true,  true,  TEAM_ALIENS }, // spiker. FIXME: we need a spiker emoticon
	{ "[overmind]",    true,  false, TEAM_ALIENS },
	{ "",              true,  false, TEAM_NONE },   // (MOD_DECONSTRUCT)
	{ "",              true,  false, TEAM_NONE },   // (MOD_REPLACE)
	{ "",              true,  false, TEAM_NONE },   // (MOD_BUILDLOG_REVERT)
};

static Cvar::Cvar<bool> cg_showObituaries( "cg_showObituaries", "show obituaries in chat", Cvar::NONE, true );
static void CG_Obituary( entityState_t *ent )
{
	if ( !cg_showObituaries.Get() )
	{
		return;
	}
	int          mod;
	int          target, attacker, assistant;
	const char   *message;
	const char   *messageAssisted = nullptr;
	const char   *messageSuicide = nullptr;
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
	assistant = ent->groundEntityNum; // we hijack the field for this
	assistantTeam = (team_t) ( ent->generic1 & 0xFF ); // ugly hack allowing for future expansion(!)
	mod = ent->eventParm;

	if ( target < 0 || target >= MAX_CLIENTS )
	{
		Sys::Drop( "CG_Obituary: target out of range" );
	}

	ci = &cgs.clientinfo[ target ];
	gender = ci->gender;

	if ( ( attacker < 0 || attacker >= MAX_CLIENTS )
	     || ( !cgs.clientinfo[ attacker ].infoValid ) )
	{
		attacker = ENTITYNUM_WORLD;
		attackerInfo = nullptr;
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
		assistantInfo = nullptr;
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

	if ( cg_emoticonsInMessages.Get() )
	{
		if ( mod < MOD_UNKNOWN || mod >= (int) ARRAY_LEN( meansOfDeath ) )
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
					Log::Notice( "%s %s (+ %s%s%s^*) killed %s%s",
					             teamTag[ attackerTeam ], meansOfDeath[ mod ].icon,
					             teamTag[ assistantTeam ], assistantName,
					             ( assistantTeam == ci->team ? " ^a!!" : "" ),
					             teamTag[ ci->team ], targetName );
				}
				else
				{
					Log::Notice( "%s %s %s%s",
					             teamTag[ attackerTeam ], meansOfDeath[ mod ].icon,
					             teamTag[ ci->team ], targetName );
				}
			}
			else if ( !cgs.clientinfo[ attacker ].infoValid || attacker == target )
			{
				Log::Notice( "%s %s%s",
				             meansOfDeath[ mod ].icon,
				             teamTag[ ci->team ], targetName );
			}
			else
			{
				if ( meansOfDeath[ mod ].showAssist && assistantInfo )
				{
					Log::Notice( "%s%s%s^* (+ %s%s%s^*) %s %s%s",
					             teamTag[ attackerTeam ], attackerName,
					             ( attackerTeam == ci->team ? " ^1!!!^*" : "" ),
					             teamTag[ assistantTeam ], assistantName,
					             ( assistantTeam == ci->team ? " ^a!!" : "" ),
					             meansOfDeath[ mod ].icon,
					             teamTag[ ci->team ], targetName );
				}
				else
				{
					Log::Notice( "%s%s%s^* %s %s%s",
					             teamTag[ attackerTeam ], attackerName,
					             ( attackerTeam == ci->team ? " ^1!!!^*" : "" ),
					             meansOfDeath[ mod ].icon,
					             teamTag[ ci->team ], targetName );
				}

				// nice big message for teamkills
				if ( attackerTeam == ci->team && attacker == cg.clientNum )
				{
					CG_CenterPrint( va( _("You killed ^1TEAMMATE^* %s"), targetName ),
							1.5f );
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
				message = G_( "%s%s ^*fell to death" );
				break;

			case MOD_CRUSH:
				message = G_( "%s%s ^*was crushed to death" );
				break;

			case MOD_WATER:
				message = G_( "%s%s ^*drowned" );
				break;

			case MOD_SLIME:
				message = G_( "%s%s ^*melted" );
				break;

			case MOD_LAVA:
				message = G_( "%s%s ^*took a dip in some lava" );
				break;

			case MOD_TRIGGER_HURT:
				message = G_( "%s%s ^*was in the wrong place" );
				break;

			// Building explosions

			case MOD_HSPAWN:
				message = G_( "%s%s ^*was caught in the explosion" );
				break;

			case MOD_ASPAWN:
				message = G_( "%s%s ^*was caught in the acid" );
				break;

			// Attacked by a building

			case MOD_MGTURRET:
				message = G_( "%s%s ^*was gunned down by a turret" );
				messageAssisted = G_( "%s%s ^*was gunned down by a turret; %s%s%s^* assisted" );
				break;

			case MOD_ROCKETPOD:
				message = G_( "%s%s ^*caught a rocket" );
				messageAssisted = G_( "%s%s ^*caught a rocket; %s%s%s^* assisted" );
				break;

			case MOD_ATUBE:
				message = G_( "%s%s ^*was melted by an acid tube" );
				messageAssisted = G_( "%s%s ^*was melted by an acid tube; %s%s%s^* assisted" );
				break;

			case MOD_SPIKER:
				message = G_( "%s%s ^*was flechetted by a spiker" );
				messageAssisted = G_( "%s%s ^*was flechetted by a spiker; %s%s%s^* assisted" );
				break;

			case MOD_OVERMIND:
				message = G_( "%s%s ^*was whipped by the overmind" );
				messageAssisted = G_( "%s%s ^*was whipped by the overmind; %s%s%s^* assisted" );
				break;

			case MOD_REACTOR:
				message = G_( "%s%s ^*was fried by the reactor" );
				messageAssisted = G_( "%s%s ^*was fried by the reactor; %s%s%s^* assisted" );
				break;

			case MOD_SLOWBLOB:
				message = G_( "%s%s ^*couldn't avoid the granger" );
				messageAssisted = G_( "%s%s ^*couldn't avoid the granger; %s%s%s^* assisted" );
				break;

			case MOD_SWARM:
				message = G_( "%s%s ^*was consumed by the swarm" );
				messageAssisted = G_( "%s%s ^*was consumed by the swarm; %s%s%s^* assisted" );
				break;

			// Shouldn't happen

			case MOD_TARGET_LASER:
				message = G_( "%s%s ^*saw the light" );
				messageAssisted = G_( "%s%s ^*saw the light; %s%s%s^* assisted" );
				break;

			default:
				message = nullptr;
				break;
		}

		if ( message )
		{
			if ( messageAssisted && assistantInfo )
			{
				Log::Notice( messageAssisted,
				             teamTag[ ci->team ], targetName,
				             ( assistantTeam == ci->team ? "^aTEAMMATE " : "" ),
				             teamTag[ assistantTeam ], assistantName);
			}
			else
			{
				Log::Notice( message, teamTag[ ci->team ], targetName );
			}

			return;
		}

		// Messages which contain the killer's name
		switch ( mod )
		{
			case MOD_PAINSAW:
				message = G_( "%s%s ^*was sawn by %s%s%s" );
				messageAssisted = G_( "%s%s ^*was sawn by %s%s%s^*; %s%s%s^* assisted" );
				break;

			case MOD_BLASTER:
				message = G_( "%s%s ^*was blasted by %s%s%s" );
				messageAssisted = G_( "%s%s ^*was blasted by %s%s%s^*; %s%s%s^* assisted" );
				break;

			case MOD_MACHINEGUN:
				message = G_( "%s%s ^*was machinegunned by %s%s%s" );
				messageAssisted = G_( "%s%s ^*was machinegunned by %s%s%s^*; %s%s%s^* assisted" );
				break;

			case MOD_CHAINGUN:
				message = G_( "%s%s ^*was mowed down by %s%s%s" );
				messageAssisted = G_( "%s%s ^*was mowed down by %s%s%s^*; %s%s%s^* assisted" );
				break;

			case MOD_SHOTGUN:
				message = G_( "%s%s ^*was gunned down by %s%s%s" );
				messageAssisted = G_( "%s%s ^*was gunned down by %s%s%s^*; %s%s%s^* assisted" );
				break;

			case MOD_PRIFLE:
				message = G_( "%s%s ^*was seared by %s%s%s^*'s pulse blast" );
				messageAssisted = G_( "%s%s ^*was seared by %s%s%s^*'s pulse blast; %s%s%s^* assisted" );
				break;

			case MOD_MDRIVER:
				message = G_( "%s%s ^*was sniped by %s%s%s" );
				messageAssisted = G_( "%s%s ^*was sniped by %s%s%s^*; %s%s%s^* assisted" );
				break;

			case MOD_LASGUN:
				message = G_( "%s%s ^*was lasered by %s%s%s" );
				messageAssisted = G_( "%s%s ^*was lasered by %s%s%s^*; %s%s%s^* assisted" );
				break;

			case MOD_FLAMER:
			case MOD_FLAMER_SPLASH:
				message = G_( "%s%s ^*was grilled by %s%s%s^*'s flame" );
				messageAssisted = G_( "%s%s ^*was grilled by %s%s%s^*'s flame; %s%s%s^* assisted" );
				messageSuicide = G_( "%s%s ^*was charred to a crisp" );
				break;

			case MOD_BURN:
				message = G_( "%s%s ^*was burned by %s%s%s^*'s fire" );
				messageAssisted = G_( "%s%s ^*was burned by %s%s%s^*'s fire; %s%s%s^* assisted" );
				messageSuicide = G_( "%s%s ^*was burned to death" );
				break;

			case MOD_LCANNON:
				message = G_( "%s%s ^*was annihilated by %s%s%s^*'s plasma blast" );
				messageAssisted = G_( "%s%s ^*was annihilated by %s%s%s^*'s plasma blast; %s%s%s^* assisted" );
				break;

			case MOD_LCANNON_SPLASH:
				message = G_( "%s%s ^*was irradiated by %s%s%s^*'s plasma blast" );
				messageAssisted = G_( "%s%s ^*was irradiated by %s%s%s^*'s plasma blast; %s%s%s^* assisted" );
				messageSuicide = G_( "%s%s ^*was irradiated" );
				break;

			case MOD_GRENADE:
				message = G_( "%s%s ^*was blown up by %s%s%s^*'s grenade" );
				messageAssisted = G_( "%s%s ^*was blown up by %s%s%s^*'s grenade; %s%s%s^* assisted" );
				messageSuicide = G_( "%s%s ^*was blown up" );
				break;

			case MOD_FIREBOMB:
				message = G_( "%s%s ^*was incinerated by %s%s%s^*'s firebomb" );
				messageAssisted = G_( "%s%s ^*was incinerated by %s%s%s^*'s firebomb; %s%s%s^* assisted" );
				messageSuicide = G_( "%s%s ^*was incinerated" );
				break;

			case MOD_ABUILDER_CLAW:
				message = G_( "%s%s ^*was gently nibbled by %s%s%s^*'s granger" );
				messageAssisted = G_( "%s%s ^*was gently nibbled by %s%s%s^*'s granger; %s%s%s^* assisted" );
				break;

			case MOD_LEVEL0_BITE:
				message = G_( "%s%s ^*was bitten by %s%s%s" );
				messageAssisted = G_( "%s%s ^*was bitten by %s%s%s^*; %s%s%s^* assisted" );
				break;

			case MOD_LEVEL1_CLAW:
				message = G_( "%s%s ^*was sliced by %s%s%s^*'s mantis" );
				messageAssisted = G_( "%s%s ^*was sliced by %s%s%s^*'s mantis; %s%s%s^* assisted" );
				break;

			case MOD_LEVEL2_CLAW:
				message = G_( "%s%s ^*was shredded by %s%s%s^*'s marauder" );
				messageAssisted = G_( "%s%s ^*was shredded by %s%s%s^*'s marauder; %s%s%s^* assisted" );
				break;

			case MOD_LEVEL2_ZAP:
				message = G_( "%s%s ^*was electrocuted by %s%s%s^*'s marauder" );
				messageAssisted = G_( "%s%s ^*was electrocuted by %s%s%s^*'s marauder; %s%s%s^* assisted" );
				break;

			case MOD_LEVEL3_CLAW:
				message = G_( "%s%s ^*was eviscerated by %s%s%s^*'s dragoon" );
				messageAssisted = G_( "%s%s ^*was eviscerated by %s%s%s^*'s dragoon; %s%s%s^* assisted" );
				break;

			case MOD_LEVEL3_POUNCE:
				message = G_( "%s%s ^*was pounced upon by %s%s%s^*'s dragoon" );
				messageAssisted = G_( "%s%s ^*was pounced upon by %s%s%s^*'s dragoon; %s%s%s^* assisted" );
				break;

			case MOD_LEVEL3_BOUNCEBALL:
				message = G_( "%s%s ^*was barbed by %s%s%s^*'s dragoon" );
				messageAssisted = G_( "%s%s ^*was barbed by %s%s%s^*'s dragoon; %s%s%s^* assisted" );
				messageSuicide = G_( "%s%s ^*was barbed" );
				break;

			case MOD_LEVEL4_CLAW:
				message = G_( "%s%s ^*was mauled by %s%s%s^*'s tyrant" );
				messageAssisted = G_( "%s%s ^*was mauled by %s%s%s^*'s tyrant; %s%s%s^* assisted" );
				break;

			case MOD_LEVEL4_TRAMPLE:
				message = G_( "%s%s ^*should have gotten out of the way of %s%s%s^*'s tyrant" );
				messageAssisted = G_( "%s%s ^*should have gotten out of the way of %s%s%s^*'s tyrant; %s%s%s^* assisted" );
				break;

			case MOD_WEIGHT_H:
			case MOD_WEIGHT_A:
				message = G_( "%s%s ^*was crushed under %s%s%s^*'s weight" );
				messageAssisted = G_( "%s%s ^*was crushed under %s%s%s^*'s weight; %s%s%s^* assisted" );
				break;

			case MOD_POISON:
				message = G_( "%s%s ^*should have used a medkit against %s%s%s^*'s poison" );
				messageAssisted = G_( "%s%s ^*should have used a medkit against %s%s%s^*'s poison; %s%s%s^* assisted" );
				break;

			case MOD_TELEFRAG:
				message = G_( "%s%s ^*tried to invade %s%s%s^*'s personal space" );
				messageSuicide = G_( "%s%s ^*ended up in the void" );
				break;

			case MOD_SLAP:
				message = G_( "%s%s ^*felt %s%s%s^*'s authority" );
				messageAssisted = G_( "%s%s ^*felt %s%s%s^*'s authority; %s%s%s^* assisted" );
				break;

			default:
				message = G_( "%s%s ^*was killed by %s%s%s" );
				messageAssisted = G_( "%s%s ^*was killed by %s%s%s^* and %s%s%s" );
				messageSuicide = G_( "%s%s ^*committed suicide" );
				break;
		}

		if ( message )
		{

			// Argument order: victim, attacker, [class,] [assistant]. Each has team tag first.
			if ( messageSuicide && ( !cgs.clientinfo[ attacker ].infoValid || attacker == target ) )
			{
				Log::Notice( messageSuicide, teamTag[ ci->team ], targetName );
			}
			else if ( messageAssisted && assistantInfo )
			{
				Log::Notice( messageAssisted,
					teamTag[ ci->team ], targetName,
					( attackerTeam == ci->team ? "^1TEAMMATE " : "" ),
					teamTag[ attackerTeam ], attackerName,
					( assistantTeam == ci->team ? "^aTEAMMATE " : "" ),
					teamTag[ assistantTeam ], assistantName );
			}
			else
			{
				Log::Notice( message,
					teamTag[ ci->team ], targetName,
					( attackerTeam == ci->team ? "^1TEAMMATE " : "" ),
					teamTag[ attackerTeam ], attackerName );
			}

			if ( attackerTeam == ci->team && attacker == cg.clientNum && attacker != target )
			{
				CG_CenterPrint( va( _("You killed ^1TEAMMATE^* %s"), targetName ),
						1.5f );
			}

			return;
		}

		// we don't know what it was
		Log::Notice( G_( "%s%s^* died" ), teamTag[ ci->team ], targetName );
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
	const char *snd;

	// don't do more than two pain sounds a second
	if ( cg.time - cent->pe.painTime < 500 )
	{
		return;
	}

	if ( health < 25 )
	{
		snd = "*pain25_1";
	}
	else if ( health < 50 )
	{
		snd = "*pain50_1";
	}
	else if ( health < 75 )
	{
		snd = "*pain75_1";
	}
	else
	{
		snd = "*pain100_1";
	}

	trap_S_StartSound( nullptr, cent->currentState.number, soundChannel_t::CHAN_VOICE,
	                   CG_CustomSound( cent->currentState.number, snd ) );

	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}

/*
=========================
CG_OnPlayerWeaponChange

Called on weapon change
=========================
*/
void CG_OnPlayerWeaponChange()
{
	// Change the HUD to match the weapon. Close the old hud first
	Rocket_ShowHud( cg.snap->ps.weapon );

	CG_Rocket_UpdateArmouryMenu();
	cg.weaponOffsetsFilter.Reset( );

	cg.predictedPlayerEntity.pe.weapon.animationNumber = -1; //force weapon lerpframe recalculation
}

/*
=========================
CG_Rocket_UpdateArmouryMenu

Called on upgrade change
=========================
*/

void CG_Rocket_UpdateArmouryMenu()
{
	// Rebuild weapon lists if UI is in focus.
	if ( rocketInfo.keyCatcher & KEYCATCH_UI && CG_MyTeam() == TEAM_HUMANS )
	{
		Rml::ElementDocument* document = menuContext->GetDocument( rocketInfo.menu[ ROCKETMENU_ARMOURYBUY ].id );
		if ( document->IsVisible() )
		{
			document->DispatchEvent( "refreshdata", {} );
		}
	}
}

/*
=========================
CG_OnMapRestart

Called whenever the map is restarted
via map_restart
=========================
*/
void CG_OnMapRestart()
{
	// if scoreboard is showing, hide it
	CG_HideScores_f();

	// hide any other menus
	Rocket_DocumentAction( "", "blurall" );
}

/*
==============
CG_Momentum

Notify player of generated momentum
==============
*/
static void CG_Momentum( entityState_t *es )
{
	float                  momentum;
	bool               negative;

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

	if ( cg_debugEvents.Get() )
	{
		Log::Debug( "ent:%3i  event:%3i %s", es->number, event,
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
			if ( cg_footsteps.Get() && ci->footsteps != FOOTSTEP_NONE )
			{
				if ( ci->footsteps == FOOTSTEP_CUSTOM )
				{
					trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_BODY,
					                   ci->customFootsteps[ rand() & 3 ] );
				}
				else
				{
					trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_BODY,
					                   cgs.media.footsteps[ ci->footsteps ][ rand() & 3 ] );
				}
			}

			break;

		case EV_FOOTSTEP_METAL:
			if ( cg_footsteps.Get() && ci->footsteps != FOOTSTEP_NONE )
			{
				if ( ci->footsteps == FOOTSTEP_CUSTOM )
				{
					trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_BODY,
					                   ci->customMetalFootsteps[ rand() & 3 ] );
				}
				else
				{
					trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_BODY,
					                   cgs.media.footsteps[ FOOTSTEP_METAL ][ rand() & 3 ] );
				}
			}

			break;

		case EV_FOOTSTEP_SQUELCH:
			if ( cg_footsteps.Get() && ci->footsteps != FOOTSTEP_NONE )
			{
				trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_BODY,
				                   cgs.media.footsteps[ FOOTSTEP_FLESH ][ rand() & 3 ] );
			}

			break;

		case EV_FOOTSPLASH:
			if ( cg_footsteps.Get() && ci->footsteps != FOOTSTEP_NONE )
			{
				trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_BODY,
				                   cgs.media.footsteps[ FOOTSTEP_SPLASH ][ rand() & 3 ] );
			}

			break;

		case EV_FOOTWADE:
			if ( cg_footsteps.Get() && ci->footsteps != FOOTSTEP_NONE )
			{
				trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_BODY,
				                   cgs.media.footsteps[ FOOTSTEP_SPLASH ][ rand() & 3 ] );
			}

			break;

		case EV_SWIM:
			if ( cg_footsteps.Get() && ci->footsteps != FOOTSTEP_NONE )
			{
				trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_BODY,
				                   cgs.media.footsteps[ FOOTSTEP_SPLASH ][ rand() & 3 ] );
			}

			break;

		case EV_FALL_SHORT:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.landSound );

			if ( clientNum == cg.predictedPlayerState.clientNum )
			{
				// smooth landing z changes
				cg.landChange = -8;
				cg.landTime = cg.time;
			}

			break;

		case EV_FALL_MEDIUM:
			// use a general pain sound
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_VOICE, CG_CustomSound( es->number, "*pain100_1" ) );

			if ( clientNum == cg.predictedPlayerState.clientNum )
			{
				// smooth landing z changes
				cg.landChange = -16;
				cg.landTime = cg.time;
			}

			break;

		case EV_FALL_FAR:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, CG_CustomSound( es->number, "*fall1" ) );
			cent->pe.painTime = cg.time; // don't play a pain sound right after this

			if ( clientNum == cg.predictedPlayerState.clientNum )
			{
				// smooth landing z changes
				cg.landChange = -24;
				cg.landTime = cg.time;
			}

			break;

		case EV_FALLING:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, CG_CustomSound( es->number, "*falling1" ) );
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
				     cg_nopredict.Get() || cg.pmoveParams.synchronous )
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

				cg.stepChange = Math::Clamp( cg.stepChange, -MAX_STEP_CHANGE, +MAX_STEP_CHANGE );

				cg.stepTime = cg.time;
				break;
			}

		case EV_JUMP:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_VOICE, CG_CustomSound( es->number, "*jump1" ) );

			if ( BG_ClassHasAbility( cg.predictedPlayerState.stats[ STAT_CLASS ], SCA_WALLJUMPER ) )
			{
				vec3_t surfNormal;
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
					vec3_t refNormal = { 0.0f, 0.0f, 1.0f };
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
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_VOICE, cgs.media.alienL4ChargePrepare );
			break;

		case EV_LEV4_TRAMPLE_START:
			//FIXME: stop cgs.media.alienL4ChargePrepare playing here
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_VOICE, cgs.media.alienL4ChargeStart );
			break;

		case EV_TAUNT:
			if ( !cg_noTaunt.Get() )
			{
				trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_VOICE, CG_CustomSound( es->number, "*taunt" ) );
			}

			break;

		case EV_WATER_TOUCH:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.watrInSound );
			break;

		case EV_WATER_LEAVE:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.watrOutSound );
			break;

		case EV_WATER_UNDER:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.watrUnSound );
			break;

		case EV_WATER_CLEAR:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, CG_CustomSound( es->number, "*gasp" ) );
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
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_WEAPON, cgs.media.weaponEmptyClick );
			break;

		case EV_CHANGE_WEAPON:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.selectSound );
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

		case EV_FIRE_DECONSTRUCT:
		case EV_FIRE_DECONSTRUCT_LONG:
		case EV_DECONSTRUCT_SELECT_TARGET:
			break;

		case EV_WEAPON_RELOAD:
			if ( cg_weapons[ es->eventParm ].wim[ WPM_PRIMARY ].reloadSound )
			{
				trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_WEAPON, cg_weapons[ es->eventParm ].wim[ WPM_PRIMARY ].reloadSound );
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
		case EV_FUEL_REFILL:
			// TODO: Add different sounds for EV_AMMO_REFILL, EV_FUEL_REFILL
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.itemFillSound );
			break;

		case EV_GRENADE_BOUNCE:
			if ( rand() & 1 )
			{
				trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.grenadeBounceSound0 );
			}
			else
			{
				trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.grenadeBounceSound1 );
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

		case EV_ALIEN_BUILDABLE_DYING:
			CG_AlienBuildableDying( (buildable_t) es->modelindex, position );
			break;

		case EV_ALIEN_BUILDABLE_EXPLOSION:
			ByteToDir( es->eventParm, dir );
			CG_AlienBuildableExplosion( position, dir );
			break;

		// TODO: Allow multiple tesla trails for any one source.
		//       This is necessary as the reactor attacks all adjacent targets.
		case EV_TESLATRAIL:
			{
				centity_t *source = &cg_entities[ es->otherEntityNum ];
				centity_t *target = &cg_entities[ es->clientNum ];

				if ( !CG_IsTrailSystemValid( &source->muzzleTS ) )
				{
					if ( ( source->muzzleTS = CG_SpawnNewTrailSystem( cgs.media.reactorZapTS ) ) != nullptr )
					{
						CG_SetAttachmentCent( &source->muzzleTS->frontAttachment, source );
						CG_SetAttachmentCent( &source->muzzleTS->backAttachment, target );
						CG_AttachToCent( &source->muzzleTS->frontAttachment );
						CG_AttachToCent( &source->muzzleTS->backAttachment );

						source->muzzleTSDeathTime = cg.time + cg_teslaTrailTime.Get();
					}
				}
			}
			break;

		case EV_GENERAL_SOUND:
			if ( cgs.gameSounds[ es->eventParm ] )
			{
				trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_VOICE, cgs.gameSounds[ es->eventParm ] );
			}
			else
			{
				s = CG_ConfigString( CS_SOUNDS + es->eventParm );
				trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_VOICE, CG_CustomSound( es->number, s ) );
			}

			break;

		case EV_GLOBAL_SOUND: // play from the player's head so it never diminishes
			if ( cgs.gameSounds[ es->eventParm ] )
			{
				trap_S_StartSound( nullptr, cg.snap->ps.clientNum, soundChannel_t::CHAN_AUTO, cgs.gameSounds[ es->eventParm ] );
			}
			else
			{
				s = CG_ConfigString( CS_SOUNDS + es->eventParm );
				trap_S_StartSound( nullptr, cg.snap->ps.clientNum, soundChannel_t::CHAN_AUTO, CG_CustomSound( es->number, s ) );
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
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_VOICE,
			                   CG_CustomSound( es->number, va( "*death%i", event - EV_DEATH1 + 1 ) ) );
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
				trap_S_StartLocalSound( cgs.media.buildableRepairedSound, soundChannel_t::CHAN_LOCAL_SOUND );
				cg.lastBuildAttempt = cg.time;
			}

			break;

		case EV_BUILD_REPAIR:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.buildableRepairSound );
			break;

		case EV_BUILD_REPAIRED:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.buildableRepairedSound );
			break;

		case EV_MAIN_UNDER_ATTACK:
			// Sanity check the warn level: Must be between 1 and 3.
			if (es->eventParm < 1 || es->eventParm > 3)
			{
				break;
			}

			switch ( CG_MyTeam() )
			{
				case TEAM_ALIENS:
					trap_S_StartLocalSound( cgs.media.alienOvermindAttack, soundChannel_t::CHAN_ANNOUNCER );
					CG_CenterPrint( va( "^%c%s", "381"[ es->eventParm - 1 ], _( "The Overmind is under attack!" ) ),
					                1.5f );
					break;

				case TEAM_HUMANS:
					// TODO: Add a "reactor is under attack" sound.
					//trap_S_StartLocalSound( cgs.media.humanReactorAttack, soundChannel_t::CHAN_ANNOUNCER );
					CG_CenterPrint( va( "^%c%s", "381"[ es->eventParm - 1 ], _( "The reactor is under attack!" ) ),
					                1.5f );
					break;

				default:
					break;
			}

			break;

		case EV_MAIN_DYING:
			switch ( CG_MyTeam() )
			{
				case TEAM_ALIENS:
					trap_S_StartLocalSound( cgs.media.alienOvermindDying, soundChannel_t::CHAN_ANNOUNCER );
					CG_CenterPrint( va( "^1%s", _( "The Overmind is dying!" ) ), 1.5f );
					break;

				case TEAM_HUMANS:
					// TODO: Add a "reactor is going down" sound.
					//trap_S_StartLocalSound( cgs.media.humanReactorDying, soundChannel_t::CHAN_ANNOUNCER );
					CG_CenterPrint( va( "^1%s", _( "The reactor is going down!" ) ), 1.5f );
					break;

				default:
					break;
			}

			break;

		case EV_WARN_ATTACK:
			// if eventParm is non-zero, there is a nearby main structure (OM/RC). Humans are only notified
			// of base attacks if RC is in range. Aliens are notified of any base attacks, but base attacks
			// without the OM in range do not give you the location of the attack.
			if ( es->eventParm >= 0 && es->eventParm < MAX_LOCATIONS )
			{
				const char *emoticon = CG_MyTeam() == TEAM_ALIENS ? "[overmind]" : "[reactor]";

				CG_CenterPrint( _( "Our base is under attack!" ), 1.25f );

				const char *location = CG_ConfigString( CS_LOCATIONS + es->eventParm );

				if ( location && *location )
				{
					Log::Notice( _( "%s under attack – %s" ), emoticon, location );
				}
				else
				{
					Log::Notice( _( "%s under attack" ), emoticon );
				}
			}
			else // this is for aliens, and the overmind is in range
			{
				ASSERT_EQ( es->eventParm, MAX_LOCATIONS );
				CG_CenterPrint( _( "Our base is under attack!" ), 1.25f );
			}

			break;

		case EV_MGTURRET_SPINUP:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.turretSpinupSound );
			break;

		case EV_NO_SPAWNS:
			switch ( CG_MyTeam() )
			{
				case TEAM_ALIENS:
					trap_S_StartLocalSound( cgs.media.alienOvermindSpawns, soundChannel_t::CHAN_ANNOUNCER );
					CG_CenterPrint( "The Overmind needs spawns!", 1.5f );
					break;

				case TEAM_HUMANS:
					// TODO: Add a sound.
					CG_CenterPrint( "There are no telenodes left!", 1.5f );
					break;

				default:
					break;
			}

			break;

		case EV_ALIEN_EVOLVE:
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_BODY, cgs.media.alienEvolveSound );
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
				trap_S_StartLocalSound( cgs.media.buildableRepairedSound, soundChannel_t::CHAN_LOCAL_SOUND );
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
			trap_S_StartSound( nullptr, es->number, soundChannel_t::CHAN_AUTO, cgs.media.medkitUseSound );
			break;

		case EV_PLAYER_RESPAWN:
			if ( es->number == cg.clientNum )
			{
				cg.spawnTime = cg.time;
			}

			break;

		case EV_MOMENTUM:
			CG_Momentum( es );
			break;

		case EV_HIT:
			CombatFeedback::Event(es);
			break;

		default:
			Sys::Drop("Unknown event: %i", event);
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
	if ( cent->currentState.eType > entityType_t::ET_EVENTS )
	{
		event = Util::enum_cast<entity_event_t>( Util::ordinal(cent->currentState.eType) - Util::ordinal(entityType_t::ET_EVENTS) );

		if ( cent->previousEvent )
		{
			return; // already fired
		}

		cent->previousEvent = 1;

		cent->currentState.event = Util::ordinal(cent->currentState.eType) - Util::ordinal(entityType_t::ET_EVENTS);

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
