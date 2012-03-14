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

// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"

extern void CG_StartShakeCamera( float param );
extern void CG_Tracer( vec3_t source, vec3_t dest, int sparks );

//==========================================================================

/*
=============
CG_Obituary
=============
*/
static void CG_Obituary( entityState_t *ent )
{
	int          mod;
	int          target, attacker;
	char         *message;
	char         *message2;
	char         targetName[ 32 ];
	char         attackerName[ 32 ];
	clientInfo_t *ci, *ca; // JPW NERVE ca = attacker
	qhandle_t    deathShader = cgs.media.pmImages[ PM_DEATH ];

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	mod = ent->eventParm;

	if( target < 0 || target >= MAX_CLIENTS )
	{
		CG_Error( "CG_Obituary: target out of range" );
	}

	ci = &cgs.clientinfo[ target ];

	if( attacker < 0 || attacker >= MAX_CLIENTS )
	{
		attacker = ENTITYNUM_WORLD;
		ca = NULL;
	}
	else
	{
		ca = &cgs.clientinfo[ attacker ];
	}

	Q_strncpyz( targetName, ci->name, sizeof( targetName ) - 2 );
	strcat( targetName, S_COLOR_WHITE );

	message2 = "";

	// check for single client messages
	switch( mod )
	{
		case MOD_SUICIDE:
			message = "committed suicide";
			break;

		case MOD_FALLING:
			message = "fell to his death";
			break;

		case MOD_CRUSH:
			message = "was crushed";
			break;

		case MOD_WATER:
			message = "drowned";
			break;

		case MOD_SLIME:
			message = "died by toxic materials";
			break;

		case MOD_TRIGGER_HURT:
		case MOD_TELEFRAG: // rain - added TELEFRAG and TARGET_LASER, just in case
		case MOD_TARGET_LASER:
			message = "was killed";
			break;

		case MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER:
			message = "got buried under a pile of rubble";
			break;

		case MOD_LAVA: // rain
			message = "was incinerated";
			break;

		default:
			message = NULL;
			break;
	}

	if( attacker == target )
	{
		switch( mod )
		{
			case MOD_DYNAMITE:
				message = "dynamited himself to pieces";
				break;

			case MOD_GRENADE_LAUNCHER:
			case MOD_GRENADE_PINEAPPLE: // rain - added PINEAPPLE
				message = "dove on his own grenade";
				break;

			case MOD_PANZERFAUST:
				message = "vaporized himself";
				break;

			case MOD_FLAMETHROWER: // rain
				message = "played with fire";
				break;

			case MOD_AIRSTRIKE:
				message = "obliterated himself";
				break;

			case MOD_ARTY:
				message = "fired-for-effect on himself";
				break;

			case MOD_EXPLOSIVE:
				message = "died in his own explosion";
				break;

				// rain - everything from this point on is sorted by MOD, didn't
				// resort existing messages to avoid differences between pre
				// and post-patch code (for source patching)
			case MOD_GPG40:
			case MOD_M7: // rain
				//bani - more amusing, less wordy
				message = "ate his own rifle grenade";
				break;

			case MOD_LANDMINE: // rain
				//bani - slightly more amusing
				message = "failed to spot his own landmine";
				break;

			case MOD_SATCHEL: // rain
				message = "embraced his own satchel explosion";
				break;

			case MOD_TRIPMINE: // rain - dormant code
				message = "forgot where his tripmine was";
				break;

			case MOD_CRUSH_CONSTRUCTION: // rain
				message = "engineered himself into oblivion";
				break;

			case MOD_CRUSH_CONSTRUCTIONDEATH: // rain
				message = "buried himself alive";
				break;

			case MOD_MORTAR: // rain
				message = "never saw his own mortar round coming";
				break;

			case MOD_SMOKEGRENADE: // rain
				// bani - more amusing
				message = "danced on his airstrike marker";
				break;

				// no obituary message if changing teams
			case MOD_SWITCHTEAM:
				return;

			default:
				message = "killed himself";
				break;
		}
	}

	if( message )
	{
		message = CG_TranslateString( message );
		CG_AddPMItem( PM_DEATH, va( "%s %s.", targetName, message ), deathShader );
		return;
	}

	// check for kill messages from the current clientNum
	if( attacker == cg.snap->ps.clientNum )
	{
		char *s;

		if( ci->team == ca->team )
		{
			if( mod == MOD_SWAP_PLACES )
			{
				s = va( "%s %s", CG_TranslateString( "You swapped places with" ), targetName );
			}
			else
			{
				s = va( "%s %s", CG_TranslateString( "You killed ^1TEAMMATE^7" ), targetName );
			}
		}
		else
		{
			s = va( "%s %s", CG_TranslateString( "You killed" ), targetName );
		}

		CG_PriorityCenterPrint( s, SCREEN_HEIGHT * 0.75, BIGCHAR_WIDTH * 0.6, 1 );
		// print the text message as well
	}

	// check for double client messages
	if( !ca )
	{
		strcpy( attackerName, "noname" );
	}
	else
	{
		Q_strncpyz( attackerName, ca->name, sizeof( attackerName ) - 2 );
		strcat( attackerName, S_COLOR_WHITE );

		// check for kill messages about the current clientNum
		if( target == cg.snap->ps.clientNum )
		{
			Q_strncpyz( cg.killerName, attackerName, sizeof( cg.killerName ) );
		}
	}

	if( ca )
	{
		switch( mod )
		{
			case MOD_KNIFE:
				message = "was stabbed by";
				message2 = "'s knife";
				// OSP - goat luvin
//          if( attacker == cg.snap->ps.clientNum || target == cg.snap->ps.clientNum ) {
//              trap_S_StartSound( cg.snap->ps.origin, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.goatAxis );
//          }
				break;

			case MOD_AKIMBO_COLT:
			case MOD_AKIMBO_SILENCEDCOLT:
				message = "was killed by";
				message2 = "'s Akimbo .45ACP 1911s";
				break;

			case MOD_AKIMBO_LUGER:
			case MOD_AKIMBO_SILENCEDLUGER:
				message = "was killed by";
				message2 = "'s Akimbo Luger 9mms";
				break;

			case MOD_SILENCER:
			case MOD_LUGER:
				message = "was killed by";
				message2 = "'s Luger 9mm";
				break;

			case MOD_SILENCED_COLT:
			case MOD_COLT:
				message = "was killed by";
				message2 = "'s .45ACP 1911";
				break;

			case MOD_MP40:
				message = "was killed by";
				message2 = "'s MP40";
				break;

			case MOD_THOMPSON:
				message = "was killed by";
				message2 = "'s Thompson";
				break;

			case MOD_STEN:
				message = "was killed by";
				message2 = "'s Sten";
				break;

			case MOD_DYNAMITE:
				message = "was blasted by";
				message2 = "'s dynamite";
				break;

			case MOD_PANZERFAUST:
				message = "was blasted by";
				message2 = "'s Panzerfaust";
				break;

			case MOD_GRENADE_LAUNCHER:
			case MOD_GRENADE_PINEAPPLE:
				message = "was exploded by";
				message2 = "'s grenade";
				break;

			case MOD_FLAMETHROWER:
				message = "was cooked by";
				message2 = "'s flamethrower";
				break;

			case MOD_MORTAR:
				message = "never saw";
				message2 = "'s mortar round coming";
				break;

			case MOD_MACHINEGUN:
				message = "was perforated by";
				message2 = "'s crew-served MG";
				break;

			case MOD_BROWNING:
				message = "was perforated by";
				message2 = "'s tank-mounted browning 30cal";
				break;

			case MOD_MG42:
				message = "was perforated by";
				message2 = "'s tank-mounted MG42";
				break;

			case MOD_AIRSTRIKE:
				message = "was blasted by";
				message2 = "'s support fire";
				break;

			case MOD_ARTY:
				message = "was shelled by";
				message2 = "'s artillery support";
				break;

			case MOD_SWAP_PLACES:
				message = "^2swapped places with^7";
				message2 = "";
				break;

			case MOD_KAR98: // same weapon really
			case MOD_K43:
				message = "was killed by";
				message2 = "'s K43";
				break;

			case MOD_CARBINE: // same weapon really
			case MOD_GARAND:
				message = "was killed by";
				message2 = "'s Garand";
				break;

			case MOD_GPG40:
			case MOD_M7:
				message = "was killed by";
				message2 = "'s rifle grenade";
				break;

			case MOD_LANDMINE:
				message = "failed to spot";
				message2 = "'s Landmine";
				break;

			case MOD_CRUSH_CONSTRUCTION:
				message = "got caught in";
				message2 = "'s construction madness";
				break;

			case MOD_CRUSH_CONSTRUCTIONDEATH:
				message = "got burried under";
				message2 = "'s rubble";
				break;

			case MOD_MOBILE_MG42:
				message = "was mown down by";
				message2 = "'s Mobile MG42";
				break;

			case MOD_GARAND_SCOPE:
				message = "was silenced by";
				message2 = "'s Garand";
				break;

			case MOD_K43_SCOPE:
				message = "was silenced by";
				message2 = "'s K43";
				break;

			case MOD_FG42:
				message = "was killed by";
				message2 = "'s FG42";
				break;

			case MOD_FG42SCOPE:
				message = "was sniped by";
				message2 = "'s FG42";
				break;

			case MOD_SATCHEL:
				message = "was blasted by";
				message2 = "'s Satchel Charge";
				break;

			case MOD_TRIPMINE: // rain - dormant code
				message = "was detonated by";
				message2 = "'s trip mine";
				break;

			case MOD_SMOKEGRENADE: // rain
				message = "stood on";
				message2 = "'s airstrike marker";
				break;

			default:
				message = "was killed by";
				break;
		}

		if( ci->team == ca->team )
		{
			message = "^1WAS KILLED BY TEAMMATE^7";
			message2 = "";
		}

		if( message )
		{
			message = CG_TranslateString( message );

			if( message2 )
			{
				message2 = CG_TranslateString( message2 );
				CG_AddPMItem( PM_DEATH, va( "%s %s %s%s", targetName, message, attackerName, message2 ), deathShader );
//              CG_Printf( "[cgnotify]%s %s %s%s\n", targetName, message, attackerName, message2 );
			}

			return;
		}
	}

	// we don't know what it was
	switch( mod )
	{
		default:
			CG_AddPMItem( PM_DEATH, va( "%s died.", targetName ), deathShader );
			break;
	}
}

//==========================================================================

// from cg_weapons.c
extern int CG_WeaponIndex( int weapnum, int *bank, int *cycle );

/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
static void CG_ItemPickup( int itemNum )
{
	int itemid;
	int wpbank_cur, wpbank_pickup;

	itemid = bg_itemlist[ itemNum ].giTag;

	CG_AddPMItem( PM_MESSAGE, va( "Picked up %s", CG_PickupItemText( itemNum ) ), cgs.media.pmImages[ PM_MESSAGE ] );

//  cg.itemPickup           = itemNum;
//  cg.itemPickupTime       = cg.time;
//  cg.itemPickupBlendTime  = cg.time;

	// see if it should be the grabbed weapon
	if( bg_itemlist[ itemNum ].giType == IT_WEAPON )
	{
		if( cg_autoswitch.integer && cg.predictedPlayerState.weaponstate != WEAPON_RELOADING )
		{
			//  0 - "Off"
			//  1 - "Always Switch"
			//  2 - "If New"
			//  3 - "If Better"
			//  4 - "New or Better"

			// don't ever autoswitch to secondary fire weapons
			// Gordon: Leave autoswitch to secondary kar/carbine as they use alt ammo and arent zoomed: Note, not that it would do this anyway as it isnt in a bank....
			if( itemid != WP_FG42SCOPE && itemid != WP_GARAND_SCOPE && itemid != WP_K43_SCOPE && itemid != WP_AMMO )
			{
				//----(SA)    modified
				// no weap currently selected, always just select the new one
				if( !cg.weaponSelect )
				{
					cg.weaponSelectTime = cg.time;
					cg.weaponSelect = itemid;
				}
				// 1 - always switch to new weap (Q3A default)
				else if( cg_autoswitch.integer == 1 )
				{
					cg.weaponSelectTime = cg.time;
					cg.weaponSelect = itemid;
				}
				else
				{
					// 2 - switch to weap if it's not already in the player's inventory (Wolf default)
					// 4 - both 2 and 3

					// FIXME:   this works fine for predicted pickups (when you walk over the weapon), but not for
					//          manual pickups (activate item)
					if( cg_autoswitch.integer == 2 || cg_autoswitch.integer == 4 )
					{
						if( !COM_BitCheck( cg.snap->ps.weapons, itemid ) )
						{
							cg.weaponSelectTime = cg.time;
							cg.weaponSelect = itemid;
						}
					} // end 2

					// 3 - switch to weap if it's in a bank greater than the current weap
					// 4 - both 2 and 3
					if( cg_autoswitch.integer == 3 || cg_autoswitch.integer == 4 )
					{
						// switch away only if a primary weapon is selected (read: don't switch away if current weap is a secondary mode)
						if( CG_WeaponIndex( cg.weaponSelect, &wpbank_cur, NULL ) )
						{
							if( CG_WeaponIndex( itemid, &wpbank_pickup, NULL ) )
							{
								if( wpbank_pickup > wpbank_cur )
								{
									cg.weaponSelectTime = cg.time;
									cg.weaponSelect = itemid;
								}
							}
						}
					} // end 3
				} // end cg_autoswitch.integer != 1
			}
		} // end cg_autoswitch.integer
	} // end bg_itemlist[itemNum].giType == IT_WEAPON
}

/*
================
CG_PainEvent

Also called by playerstate transition
================
*/
typedef struct
{
	char *tag;
	int  refEntOfs;
	int  anim;
} painAnimForTag_t;

#define PEFOFS( x ) ( (int)&( ( (playerEntity_t *)0 )->x ) )

void CG_PainEvent( centity_t *cent, int health, qboolean crouching )
{
	char *snd;

	// don't do more than two pain sounds a second
	if( cg.time - cent->pe.painTime < 500 )
	{
		return;
	}

	if( health < 25 )
	{
		snd = "*pain25_1.wav";
	}
	else if( health < 50 )
	{
		snd = "*pain50_1.wav";
	}
	else if( health < 75 )
	{
		snd = "*pain75_1.wav";
	}
	else
	{
		snd = "*pain100_1.wav";
	}

	trap_S_StartSound( NULL, cent->currentState.number, CHAN_VOICE, CG_CustomSound( cent->currentState.number, snd ) );

	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}

/*
==============
CG_Explode


        if (cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2])

==============
*/

#define POSSIBLE_PIECES 6

typedef struct fxSound_s
{
	int        max;
	qhandle_t  sound[ 3 ];
	const char *soundfile[ 3 ];
} fxSound_t;

static fxSound_t fxSounds[ POSSIBLE_PIECES ] =
{
	// wood
	{ 1, { -1, -1, -1 }, { "sound/world/boardbreak.wav",  NULL,                          NULL                          } },
	// glass
	{ 3, { -1, -1, -1 }, { "sound/world/glassbreak1.wav", "sound/world/glassbreak2.wav", "sound/world/glassbreak3.wav" } },
	// metal
	{ 1, { -1, -1, -1 }, { "sound/world/metalbreak.wav",  NULL,                          NULL                          } },
	// gibs
	{ 1, { -1, -1, -1 }, { "sound/world/gibsplit1.wav",   NULL,                          NULL                          } },
	// brick
	{ 1, { -1, -1, -1 }, { "sound/world/debris1.wav",     NULL,                          NULL                          } },
	// stone
	{ 1, { -1, -1, -1 }, { "sound/world/stonefall.wav",   NULL,                          NULL                          } }
};

void CG_PrecacheFXSounds( void )
{
	int i, j;

	for( i = 0; i < POSSIBLE_PIECES; i++ )
	{
		for( j = 0; j < fxSounds[ i ].max; j++ )
		{
			fxSounds[ i ].sound[ j ] = trap_S_RegisterSound( fxSounds[ i ].soundfile[ j ], qfalse );
		}
	}
}

void CG_Explodef( vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader );
void CG_RubbleFx( vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader,
                  float speedscale, float sizescale );

/*
==============
CG_Explode
        the old cent-based explode calls will still work with this pass-through
==============
*/
void CG_Explode( centity_t *cent, vec3_t origin, vec3_t dir, qhandle_t shader )
{
	qhandle_t inheritmodel = 0;

	// inherit shader
	// (SA) FIXME: do this at spawn time rather than explode time so any new necessary shaders are created earlier
	if( cent->currentState.eFlags & EF_INHERITSHADER )
	{
		if( !shader )
		{
//          inheritmodel = cent->currentState.modelindex;
			inheritmodel = cgs.inlineDrawModel[ cent->currentState.modelindex ]; // okay, this should be better.

			if( inheritmodel )
			{
				shader = trap_R_GetShaderFromModel( inheritmodel, 0, 0 );
			}
		}
	}

	if( !cent->currentState.dl_intensity )
	{
		sfxHandle_t sound;

		sound = random() * fxSounds[ cent->currentState.frame ].max;

		if( fxSounds[ cent->currentState.frame ].sound[ sound ] == -1 )
		{
			fxSounds[ cent->currentState.frame ].sound[ sound ] =
			  trap_S_RegisterSound( fxSounds[ cent->currentState.frame ].soundfile[ sound ], qfalse );
		}

		sound = fxSounds[ cent->currentState.frame ].sound[ sound ];

		CG_Explodef( origin, dir, cent->currentState.density,  // mass
		             cent->currentState.frame, // type
		             sound, // sound
		             cent->currentState.weapon, // forceLowGrav
		             shader );
	}
	else
	{
		sfxHandle_t sound;

		if( cent->currentState.dl_intensity == -1 )
		{
			sound = 0;
		}
		else
		{
			sound = cgs.gameSounds[ cent->currentState.dl_intensity ];
		}

		CG_Explodef( origin, dir, cent->currentState.density,  // mass
		             cent->currentState.frame, // type
		             sound, // sound
		             cent->currentState.weapon, // forceLowGrav
		             shader );
	}
}

/*
==============
CG_Explode
        the old cent-based explode calls will still work with this pass-through
==============
*/
void CG_Rubble( centity_t *cent, vec3_t origin, vec3_t dir, qhandle_t shader )
{
	qhandle_t inheritmodel = 0;

	// inherit shader
	// (SA) FIXME: do this at spawn time rather than explode time so any new necessary shaders are created earlier
	if( cent->currentState.eFlags & EF_INHERITSHADER )
	{
		if( !shader )
		{
//          inheritmodel = cent->currentState.modelindex;
			inheritmodel = cgs.inlineDrawModel[ cent->currentState.modelindex ]; // okay, this should be better.

			if( inheritmodel )
			{
				shader = trap_R_GetShaderFromModel( inheritmodel, 0, 0 );
			}
		}
	}

	if( !cent->currentState.dl_intensity )
	{
		sfxHandle_t sound;

		sound = random() * fxSounds[ cent->currentState.frame ].max;

		if( fxSounds[ cent->currentState.frame ].sound[ sound ] == -1 )
		{
			fxSounds[ cent->currentState.frame ].sound[ sound ] =
			  trap_S_RegisterSound( fxSounds[ cent->currentState.frame ].soundfile[ sound ], qfalse );
		}

		sound = fxSounds[ cent->currentState.frame ].sound[ sound ];

		CG_RubbleFx( origin, dir, cent->currentState.density,  // mass
		             cent->currentState.frame, // type
		             sound, // sound
		             cent->currentState.weapon, // forceLowGrav
		             shader, cent->currentState.angles2[ 0 ], cent->currentState.angles2[ 1 ] );
	}
	else
	{
		sfxHandle_t sound;

		if( cent->currentState.dl_intensity == -1 )
		{
			sound = 0;
		}
		else
		{
			sound = cgs.gameSounds[ cent->currentState.dl_intensity ];
		}

		CG_RubbleFx( origin, dir, cent->currentState.density,  // mass
		             cent->currentState.frame, // type
		             sound, // sound
		             cent->currentState.weapon, // forceLowGrav
		             shader, cent->currentState.angles2[ 0 ], cent->currentState.angles2[ 1 ] );
	}
}

/*
==============
CG_RubbleFx
==============
*/
void CG_RubbleFx( vec3_t origin, vec3_t dir, int mass, int type, sfxHandle_t sound, int forceLowGrav, qhandle_t shader,
                  float speedscale, float sizescale )
{
	int           i;
	localEntity_t *le;
	refEntity_t   *re;
	int           howmany, total, totalsounds;
	int           pieces[ 6 ]; // how many of each piece
	qhandle_t     modelshader = 0;
	float         materialmul = 1; // multiplier for different types

	memset( &pieces, 0, sizeof( pieces ) );

	pieces[ 5 ] = ( int )( mass / 250.0f );
	pieces[ 4 ] = ( int )( mass / 76.0f );
	pieces[ 3 ] = ( int )( mass / 37.0f );  // so 2 per 75
	pieces[ 2 ] = ( int )( mass / 15.0f );
	pieces[ 1 ] = ( int )( mass / 10.0f );
	pieces[ 0 ] = ( int )( mass / 5.0f );

	if( pieces[ 0 ] > 20 )
	{
		pieces[ 0 ] = 20; // cap some of the smaller bits so they don't get out of control
	}

	if( pieces[ 1 ] > 15 )
	{
		pieces[ 1 ] = 15;
	}

	if( pieces[ 2 ] > 10 )
	{
		pieces[ 2 ] = 10;
	}

	if( type == 0 )
	{
		// cap wood even more since it's often grouped, and the small splinters can add up
		if( pieces[ 0 ] > 10 )
		{
			pieces[ 0 ] = 10;
		}

		if( pieces[ 1 ] > 10 )
		{
			pieces[ 1 ] = 10;
		}

		if( pieces[ 2 ] > 10 )
		{
			pieces[ 2 ] = 10;
		}
	}

	totalsounds = 0;
	total = pieces[ 5 ] + pieces[ 4 ] + pieces[ 3 ] + pieces[ 2 ] + pieces[ 1 ] + pieces[ 0 ];

	if( sound )
	{
		trap_S_StartSound( origin, -1, CHAN_AUTO, sound );
	}

	if( shader )
	{
		// shader passed in to use
		modelshader = shader;
	}

	for( i = 0; i < POSSIBLE_PIECES; i++ )
	{
		leBounceSoundType_t snd = LEBS_NONE;
		int                 hmodel = 0;
		float               scale;
		int                 endtime;

		for( howmany = 0; howmany < pieces[ i ]; howmany++ )
		{
			scale = 1.0f;
			endtime = 0; // set endtime offset for faster/slower fadeouts

			switch( type )
			{
				case 0: // "wood"
					snd = LEBS_WOOD;
					hmodel = cgs.media.debWood[ i ];

					if( i == 0 )
					{
						scale = 0.5f;
					}
					else if( i == 1 )
					{
						scale = 0.6f;
					}
					else if( i == 2 )
					{
						scale = 0.7f;
					}
					else if( i == 3 )
					{
						scale = 0.5f;
					}

					//                  else goto pass;

					if( i < 3 )
					{
						endtime = -3000; // small bits live 3 sec shorter than normal
					}

					break;

				case 1: // "glass"
					snd = LEBS_NONE;

					if( i == 5 )
					{
						hmodel = cgs.media.shardGlass1;
					}
					else if( i == 4 )
					{
						hmodel = cgs.media.shardGlass2;
					}
					else if( i == 2 )
					{
						hmodel = cgs.media.shardGlass2;
					}
					else if( i == 1 )
					{
						hmodel = cgs.media.shardGlass2;
						scale = 0.5f;
					}
					else
					{
						goto pass;
					}

					break;

				case 2: // "metal"
					snd = LEBS_METAL;

					if( i == 5 )
					{
						hmodel = cgs.media.shardMetal1;
					}
					else if( i == 4 )
					{
						hmodel = cgs.media.shardMetal2;
					}
					else if( i == 2 )
					{
						hmodel = cgs.media.shardMetal2;
					}
					else if( i == 1 )
					{
						hmodel = cgs.media.shardMetal2;
						scale = 0.5f;
					}
					else
					{
						goto pass;
					}

					break;

				case 3: // "gibs"
					snd = LEBS_BLOOD;

					if( i == 5 )
					{
						hmodel = cgs.media.gibIntestine;
					}
					else if( i == 4 )
					{
						hmodel = cgs.media.gibLeg;
					}
					else if( i == 2 )
					{
						hmodel = cgs.media.gibChest;
					}
					else
					{
						goto pass;
					}

					break;

				case 4: // "brick"
					snd = LEBS_ROCK;
					hmodel = cgs.media.debBlock[ i ];
					break;

				case 5: // "rock"
					snd = LEBS_ROCK;

					if( i == 5 )
					{
						hmodel = cgs.media.debRock[ 2 ]; // temporarily use the next smallest rock piece
					}
					else if( i == 4 )
					{
						hmodel = cgs.media.debRock[ 2 ];
					}
					else if( i == 3 )
					{
						hmodel = cgs.media.debRock[ 1 ];
					}
					else if( i == 2 )
					{
						hmodel = cgs.media.debRock[ 0 ];
					}
					else if( i == 1 )
					{
						hmodel = cgs.media.debBlock[ 1 ]; // temporarily use the small block pieces
					}
					else
					{
						hmodel = cgs.media.debBlock[ 0 ]; // temporarily use the small block pieces
					}

					if( i <= 2 )
					{
						endtime = -2000; // small bits live 2 sec shorter than normal
					}

					break;

				case 6: // "fabric"
					if( i == 5 )
					{
						hmodel = cgs.media.debFabric[ 0 ];
					}
					else if( i == 4 )
					{
						hmodel = cgs.media.debFabric[ 1 ];
					}
					else if( i == 2 )
					{
						hmodel = cgs.media.debFabric[ 2 ];
					}
					else if( i == 1 )
					{
						hmodel = cgs.media.debFabric[ 2 ];
						scale = 0.5;
					}
					else
					{
						goto pass; // (only do 5, 4, 2 and 1)
					}

					break;
			}

			le = CG_AllocLocalEntity();
			re = &le->refEntity;

			le->leType = LE_FRAGMENT;
			le->startTime = cg.time;

			le->endTime = ( le->startTime + 5000 + random() * 5000 ) + endtime;

			// as it turns out, i'm not sure if setting the re->axis here will actually do anything
			//          AxisClear(re->axis);
			//          re->axis[0][0] =
			//          re->axis[1][1] =
			//          re->axis[2][2] = scale;
			//
			//          if(scale != 1.0)
			//              re->nonNormalizedAxes = qtrue;

			le->sizeScale = scale * sizescale;

			if( type == 1 )
			{
				// glass
				// Rafael added this because glass looks funky when it fades out
				// TBD: need to look into this so that they fade out correctly
				re->fadeStartTime = le->endTime;
				re->fadeEndTime = le->endTime;
			}
			else
			{
				re->fadeStartTime = le->endTime - 4000;
				re->fadeEndTime = le->endTime;
			}

			if( total > 5 )
			{
				if( totalsounds > 5 || ( howmany % 8 ) != 0 )
				{
					snd = LEBS_NONE;
				}
				else
				{
					totalsounds++;
				}
			}

			le->lifeRate = 1.0 / ( le->endTime - le->startTime );
			le->leFlags = LEF_TUMBLE;
			le->leMarkType = 0;

			VectorCopy( origin, re->origin );
			AxisCopy( axisDefault, re->axis );

			le->leBounceSoundType = snd;
			re->hModel = hmodel;

			// inherit shader
			if( modelshader )
			{
				re->customShader = modelshader;
			}

			re->radius = 1000;

			// trying to make this a little more interesting
			if( type == 6 )
			{
				// "fabric"
				le->pos.trType = TR_GRAVITY_FLOAT; // the fabric stuff will change to use something that looks better
			}
			else
			{
				if( !forceLowGrav && rand() & 1 )
				{
					// if low gravity is not forced and die roll goes our way use regular grav
					le->pos.trType = TR_GRAVITY;
				}
				else
				{
					le->pos.trType = TR_GRAVITY_LOW;
				}
			}

			switch( type )
			{
				case 6: // fabric
					le->bounceFactor = 0.0;
					materialmul = 0.3; // rotation speed
					break;

				default:
					le->bounceFactor = 0.4;
					break;
			}

			// rotation
			le->angles.trType = TR_LINEAR;
			le->angles.trTime = cg.time;
			le->angles.trBase[ 0 ] = rand() & 31;
			le->angles.trBase[ 1 ] = rand() & 31;
			le->angles.trBase[ 2 ] = rand() & 31;
			le->angles.trDelta[ 0 ] = ( ( 100 + ( rand() & 500 ) ) - 300 ) * materialmul;
			le->angles.trDelta[ 1 ] = ( ( 100 + ( rand() & 500 ) ) - 300 ) * materialmul;
			le->angles.trDelta[ 2 ] = ( ( 100 + ( rand() & 500 ) ) - 300 ) * materialmul;

			//          if(type == 6)   // fabric
			//              materialmul = 1;        // translation speed

			VectorCopy( origin, le->pos.trBase );
			VectorNormalize( dir );
			le->pos.trTime = cg.time;

			// (SA) hoping that was just intended to represent randomness
			//          if (cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2])
			if( le->angles.trBase[ 0 ] == 1 || le->angles.trBase[ 1 ] == 1 || le->angles.trBase[ 2 ] == 1 )
			{
				le->pos.trType = TR_GRAVITY;
				VectorScale( dir, 10 * 8, le->pos.trDelta );
				le->pos.trDelta[ 0 ] += ( ( random() * 400 ) - 200 ) * speedscale;
				le->pos.trDelta[ 1 ] += ( ( random() * 400 ) - 200 ) * speedscale;
				le->pos.trDelta[ 2 ] = ( ( random() * 400 ) + 400 ) * speedscale;
			}
			else
			{
				// location
				VectorScale( dir, 200 + mass, le->pos.trDelta );
				le->pos.trDelta[ 0 ] += ( ( random() * 200 ) - 100 );
				le->pos.trDelta[ 1 ] += ( ( random() * 200 ) - 100 );

				if( dir[ 2 ] )
				{
					le->pos.trDelta[ 2 ] = random() * 200 * materialmul; // randomize sort of a lot so they don't all land together
				}
				else
				{
					le->pos.trDelta[ 2 ] = random() * 20;
				}
			}
		}

pass:
		continue;
	}
}

/*
==============
CG_Explodef
        made this more generic for spawning hits and breaks without needing a *cent
==============
*/
void CG_Explodef( vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader )
{
	int           i;
	localEntity_t *le;
	refEntity_t   *re;
	int           howmany, total, totalsounds;
	int           pieces[ 6 ]; // how many of each piece
	qhandle_t     modelshader = 0;
	float         materialmul = 1; // multiplier for different types

	memset( &pieces, 0, sizeof( pieces ) );

	pieces[ 5 ] = ( int )( mass / 250.0f );
	pieces[ 4 ] = ( int )( mass / 76.0f );
	pieces[ 3 ] = ( int )( mass / 37.0f );  // so 2 per 75
	pieces[ 2 ] = ( int )( mass / 15.0f );
	pieces[ 1 ] = ( int )( mass / 10.0f );
	pieces[ 0 ] = ( int )( mass / 5.0f );

	if( pieces[ 0 ] > 20 )
	{
		pieces[ 0 ] = 20; // cap some of the smaller bits so they don't get out of control
	}

	if( pieces[ 1 ] > 15 )
	{
		pieces[ 1 ] = 15;
	}

	if( pieces[ 2 ] > 10 )
	{
		pieces[ 2 ] = 10;
	}

	if( type == 0 )
	{
		// cap wood even more since it's often grouped, and the small splinters can add up
		if( pieces[ 0 ] > 10 )
		{
			pieces[ 0 ] = 10;
		}

		if( pieces[ 1 ] > 10 )
		{
			pieces[ 1 ] = 10;
		}

		if( pieces[ 2 ] > 10 )
		{
			pieces[ 2 ] = 10;
		}
	}

	total = pieces[ 5 ] + pieces[ 4 ] + pieces[ 3 ] + pieces[ 2 ] + pieces[ 1 ] + pieces[ 0 ];
	totalsounds = 0;

	if( sound )
	{
		trap_S_StartSound( origin, -1, CHAN_AUTO, sound );
	}

	if( shader )
	{
		// shader passed in to use
		modelshader = shader;
	}

	for( i = 0; i < POSSIBLE_PIECES; i++ )
	{
		leBounceSoundType_t snd = LEBS_NONE;
		int                 hmodel = 0;
		float               scale;
		int                 endtime;

		for( howmany = 0; howmany < pieces[ i ]; howmany++ )
		{
			scale = 1.0f;
			endtime = 0; // set endtime offset for faster/slower fadeouts

			switch( type )
			{
				case 0: // "wood"
					snd = LEBS_WOOD;
					hmodel = cgs.media.debWood[ i ];

					if( i == 0 )
					{
						scale = 0.5f;
					}
					else if( i == 1 )
					{
						scale = 0.6f;
					}
					else if( i == 2 )
					{
						scale = 0.7f;
					}
					else if( i == 3 )
					{
						scale = 0.5f;
					}

					//                  else goto pass;

					if( i < 3 )
					{
						endtime = -3000; // small bits live 3 sec shorter than normal
					}

					break;

				case 1: // "glass"
					snd = LEBS_NONE;

					if( i == 5 )
					{
						hmodel = cgs.media.shardGlass1;
					}
					else if( i == 4 )
					{
						hmodel = cgs.media.shardGlass2;
					}
					else if( i == 2 )
					{
						hmodel = cgs.media.shardGlass2;
					}
					else if( i == 1 )
					{
						hmodel = cgs.media.shardGlass2;
						scale = 0.5f;
					}
					else
					{
						goto pass;
					}

					break;

				case 2: // "metal"
					snd = LEBS_BRASS;

					if( i == 5 )
					{
						hmodel = cgs.media.shardMetal1;
					}
					else if( i == 4 )
					{
						hmodel = cgs.media.shardMetal2;
					}
					else if( i == 2 )
					{
						hmodel = cgs.media.shardMetal2;
					}
					else if( i == 1 )
					{
						hmodel = cgs.media.shardMetal2;
						scale = 0.5f;
					}
					else
					{
						goto pass;
					}

					break;

				case 3: // "gibs"
					snd = LEBS_BLOOD;

					if( i == 5 )
					{
						hmodel = cgs.media.gibIntestine;
					}
					else if( i == 4 )
					{
						hmodel = cgs.media.gibLeg;
					}
					else if( i == 2 )
					{
						hmodel = cgs.media.gibChest;
					}
					else
					{
						goto pass;
					}

					break;

				case 4: // "brick"
					snd = LEBS_ROCK;
					hmodel = cgs.media.debBlock[ i ];
					break;

				case 5: // "rock"
					snd = LEBS_ROCK;

					if( i == 5 )
					{
						hmodel = cgs.media.debRock[ 2 ]; // temporarily use the next smallest rock piece
					}
					else if( i == 4 )
					{
						hmodel = cgs.media.debRock[ 2 ];
					}
					else if( i == 3 )
					{
						hmodel = cgs.media.debRock[ 1 ];
					}
					else if( i == 2 )
					{
						hmodel = cgs.media.debRock[ 0 ];
					}
					else if( i == 1 )
					{
						hmodel = cgs.media.debBlock[ 1 ]; // temporarily use the small block pieces
					}
					else
					{
						hmodel = cgs.media.debBlock[ 0 ]; // temporarily use the small block pieces
					}

					if( i <= 2 )
					{
						endtime = -2000; // small bits live 2 sec shorter than normal
					}

					break;

				case 6: // "fabric"
					if( i == 5 )
					{
						hmodel = cgs.media.debFabric[ 0 ];
					}
					else if( i == 4 )
					{
						hmodel = cgs.media.debFabric[ 1 ];
					}
					else if( i == 2 )
					{
						hmodel = cgs.media.debFabric[ 2 ];
					}
					else if( i == 1 )
					{
						hmodel = cgs.media.debFabric[ 2 ];
						scale = 0.5;
					}
					else
					{
						goto pass; // (only do 5, 4, 2 and 1)
					}

					break;
			}

			le = CG_AllocLocalEntity();
			re = &le->refEntity;

			le->leType = LE_FRAGMENT;
			le->startTime = cg.time;

			le->endTime = ( le->startTime + 5000 + random() * 5000 ) + endtime;

			// as it turns out, i'm not sure if setting the re->axis here will actually do anything
			//          AxisClear(re->axis);
			//          re->axis[0][0] =
			//          re->axis[1][1] =
			//          re->axis[2][2] = scale;
			//
			//          if(scale != 1.0)
			//              re->nonNormalizedAxes = qtrue;

			le->sizeScale = scale;

			if( type == 1 )
			{
				// glass
				// Rafael added this because glass looks funky when it fades out
				// TBD: need to look into this so that they fade out correctly
				re->fadeStartTime = le->endTime;
				re->fadeEndTime = le->endTime;
			}
			else
			{
				re->fadeStartTime = le->endTime - 4000;
				re->fadeEndTime = le->endTime;
			}

			if( total > 5 )
			{
				if( totalsounds > 5 || ( howmany % 8 ) != 0 )
				{
					snd = LEBS_NONE;
				}
				else
				{
					totalsounds++;
				}
			}

			le->lifeRate = 1.0 / ( le->endTime - le->startTime );
			le->leFlags = LEF_TUMBLE;
			le->leMarkType = 0;

			VectorCopy( origin, re->origin );
			AxisCopy( axisDefault, re->axis );

			le->leBounceSoundType = snd;
			re->hModel = hmodel;

			// inherit shader
			if( modelshader )
			{
				re->customShader = modelshader;
			}

			re->radius = 1000;

			// trying to make this a little more interesting
			if( type == 6 )
			{
				// "fabric"
				le->pos.trType = TR_GRAVITY_FLOAT; // the fabric stuff will change to use something that looks better
			}
			else
			{
				if( !forceLowGrav && rand() & 1 )
				{
					// if low gravity is not forced and die roll goes our way use regular grav
					le->pos.trType = TR_GRAVITY;
				}
				else
				{
					le->pos.trType = TR_GRAVITY_LOW;
				}
			}

			switch( type )
			{
				case 6: // fabric
					le->bounceFactor = 0.0;
					materialmul = 0.3; // rotation speed
					break;

				default:
					le->bounceFactor = 0.4;
					break;
			}

			// rotation
			le->angles.trType = TR_LINEAR;
			le->angles.trTime = cg.time;
			le->angles.trBase[ 0 ] = rand() & 31;
			le->angles.trBase[ 1 ] = rand() & 31;
			le->angles.trBase[ 2 ] = rand() & 31;
			le->angles.trDelta[ 0 ] = ( ( 100 + ( rand() & 500 ) ) - 300 ) * materialmul;
			le->angles.trDelta[ 1 ] = ( ( 100 + ( rand() & 500 ) ) - 300 ) * materialmul;
			le->angles.trDelta[ 2 ] = ( ( 100 + ( rand() & 500 ) ) - 300 ) * materialmul;

			//          if(type == 6)   // fabric
			//              materialmul = 1;        // translation speed

			VectorCopy( origin, le->pos.trBase );
			VectorNormalize( dir );
			le->pos.trTime = cg.time;

			// (SA) hoping that was just intended to represent randomness
			//          if (cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2])
			if( le->angles.trBase[ 0 ] == 1 || le->angles.trBase[ 1 ] == 1 || le->angles.trBase[ 2 ] == 1 )
			{
				le->pos.trType = TR_GRAVITY;
				VectorScale( dir, 10 * 8, le->pos.trDelta );
				le->pos.trDelta[ 0 ] += ( ( random() * 100 ) - 50 );
				le->pos.trDelta[ 1 ] += ( ( random() * 100 ) - 50 );
				le->pos.trDelta[ 2 ] = ( random() * 200 ) + 200;
			}
			else
			{
				// location
				VectorScale( dir, 200 + mass, le->pos.trDelta );
				le->pos.trDelta[ 0 ] += ( ( random() * 100 ) - 50 );
				le->pos.trDelta[ 1 ] += ( ( random() * 100 ) - 50 );

				if( dir[ 2 ] )
				{
					le->pos.trDelta[ 2 ] = random() * 200 * materialmul; // randomize sort of a lot so they don't all land together
				}
				else
				{
					le->pos.trDelta[ 2 ] = random() * 20;
				}
			}
		}

pass:
		continue;
	}
}

/*
==============
CG_Effect
        Quake ed -> target_effect (0 .5 .8) (-6 -6 -6) (6 6 6) fire explode smoke debris gore lowgrav
==============
*/
void CG_Effect( centity_t *cent, vec3_t origin, vec3_t dir )
{
	localEntity_t *le;
	refEntity_t   *re;

//  int             howmany;
	int           mass;

//  int             large, small;
	vec4_t        projection, color;

	VectorSet( dir, 0, 0, 1 );  // straight up.

	mass = cent->currentState.density;

//      1 large per 100, 1 small per 24
//  large   = (int)(mass / 100);
//  small   = (int)(mass / 24) + 1;

	if( cent->currentState.eventParm & 1 )
	{
		// fire
		CG_MissileHitWall( WP_DYNAMITE, 0, origin, dir, 0 );
		return;
	}

	// (SA) right now force smoke on any explosions
//  if(cent->currentState.eventParm & 4)    // smoke
	if( cent->currentState.eventParm & 7 )
	{
		int    i, j;
		vec3_t sprVel, sprOrg;

		// explosion sprite animation
		VectorScale( dir, 16, sprVel );

		for( i = 0; i < 5; i++ )
		{
			for( j = 0; j < 3; j++ )
			{
				sprOrg[ j ] = origin[ j ] + 64 * dir[ j ] + 24 * crandom();
			}

			sprVel[ 2 ] += rand() % 50;
//          CG_ParticleExplosion( 2, sprOrg, sprVel, 1000+rand()%250, 20, 40+rand()%60 );
			CG_ParticleExplosion( "blacksmokeanim", sprOrg, sprVel, 3500 + rand() % 250, 10, 250 + rand() % 60, qfalse );  // JPW NERVE was smokeanimb
		}
	}

	if( cent->currentState.eventParm & 2 )
	{
		// explode
		vec3_t sprVel, sprOrg;

		trap_S_StartSound( origin, -1, CHAN_AUTO, cgs.media.sfx_rockexp );

		// new explode  (from rl)
		VectorMA( origin, 16, dir, sprOrg );
		VectorScale( dir, 100, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 500, 20, 160, qtrue );
		//CG_ParticleExplosion( "blueexp", sprOrg, sprVel, 1200, 9, 300 );

		// (SA) this is done only if the level designer has it marked in the entity.
		//      (see "cent->currentState.eventParm & 64" below)

		// RF, throw some debris
//      CG_AddDebris( origin, dir,
//                      280,    // speed
//                      1400,   // duration
//                      // 15 + rand()%5 ); // count
//                      7 + rand()%2 ); // count

		//% CG_ImpactMark( cgs.media.burnMarkShader, origin, dir, random()*360, 1,1,1,1, qfalse, 64, qfalse, 0xffffffff );
		VectorSet( projection, 0, 0, -1 );
		projection[ 3 ] = 64.0f;
		Vector4Set( color, 1.0f, 1.0f, 1.0f, 1.0f );
		trap_R_ProjectDecal( cgs.media.burnMarkShader, 1, ( vec3_t * ) origin, projection, color, cg_markTime.integer,
		                     ( cg_markTime.integer >> 4 ) );
	}

	if( cent->currentState.eventParm & 8 )
	{
		// rubble
		// share the cg_explode code with func_explosives
		const char *s;
		qhandle_t  sh = 0; // shader handle

		vec3_t     newdir = { 0, 0, 0 };

		if( cent->currentState.angles2[ 0 ] || cent->currentState.angles2[ 1 ] || cent->currentState.angles2[ 2 ] )
		{
			VectorCopy( cent->currentState.angles2, newdir );
		}

		s = CG_ConfigString( CS_TARGETEFFECT );  // see if ent has a shader specified

		if( s && strlen( s ) > 0 )
		{
			sh = trap_R_RegisterShader( va( "textures/%s", s ) );   // FIXME: don't do this here.  only for testing
		}

		cent->currentState.eFlags &= ~EF_INHERITSHADER; // don't try to inherit shader
		cent->currentState.dl_intensity = 0; // no sound
		CG_Explode( cent, origin, newdir, sh );
	}

	if( cent->currentState.eventParm & 16 )
	{
		// gore
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + 5000 + random() * 3000;
//----(SA)  fading out
		re->fadeStartTime = le->endTime - 4000;
		re->fadeEndTime = le->endTime;
//----(SA)  end

		VectorCopy( origin, re->origin );
		AxisCopy( axisDefault, re->axis );
		//  re->hModel = hModel;
		re->hModel = cgs.media.gibIntestine;
		le->pos.trType = TR_GRAVITY;
		VectorCopy( origin, le->pos.trBase );

		//  VectorCopy( velocity, le->pos.trDelta );
		VectorNormalize( dir );
		VectorMA( dir, 200, dir, le->pos.trDelta );

		le->pos.trTime = cg.time;

		le->bounceFactor = 0.3;

		le->leBounceSoundType = LEBS_BLOOD;
		le->leMarkType = LEMT_BLOOD;
	}

	if( cent->currentState.eventParm & 64 )
	{
		// debris trails (the black strip that Ryan did)
		CG_AddDebris( origin, dir, 280,  // speed
		              1400, // duration
		              // 15 + rand()%5 );   // count
		              7 + rand() % 2 ); // count
	}
}

/*
CG_Shard

        We should keep this separate since there will be considerable differences
        in the physical properties of shard vrs debris. not to mention the fact
        there is no way we can quantify what type of effects the designers will
        potentially desire. If it is still possible to merge the functionality of
        cg_shard into cg_explode at a latter time I would have no problem with that
        but for now I want to keep it separate
*/
void CG_Shard( centity_t *cent, vec3_t origin, vec3_t dir )
{
	localEntity_t *le;
	refEntity_t   *re;
	int           type;
	int           howmany;
	int           i;
	int           rval;

	qboolean      isflyingdebris = qfalse;

	type = cent->currentState.density;
	howmany = cent->currentState.frame;

	for( i = 0; i < howmany; i++ )
	{
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + 5000 + random() * 5000;

//----(SA)  fading out
		re->fadeStartTime = le->endTime - 1000;
		re->fadeEndTime = le->endTime;
//----(SA)  end

		if( type == 999 )
		{
			le->startTime = cg.time;
			le->endTime = le->startTime + 100;
			re->fadeStartTime = le->endTime - 100;
			re->fadeEndTime = le->endTime;
			type = 1;

			isflyingdebris = qtrue;
		}

		le->lifeRate = 1.0 / ( le->endTime - le->startTime );
		le->leFlags = LEF_TUMBLE;
		le->bounceFactor = 0.4;
		// le->leBounceSoundType    = LEBS_WOOD;
		le->leMarkType = 0;

		VectorCopy( origin, re->origin );
		AxisCopy( axisDefault, re->axis );

		if( type == FXTYPE_GLASS )
		{
			// glass
			rval = rand() % 2;

			if( rval )
			{
				re->hModel = cgs.media.shardGlass1;
			}
			else
			{
				re->hModel = cgs.media.shardGlass2;
			}
		}
		else if( type == FXTYPE_WOOD )
		{
			// wood
			rval = rand() % 2;

			if( rval )
			{
				re->hModel = cgs.media.shardWood1;
			}
			else
			{
				re->hModel = cgs.media.shardWood2;
			}
		}
		else if( type == FXTYPE_METAL )
		{
			// metal
			rval = rand() % 2;

			if( rval )
			{
				re->hModel = cgs.media.shardMetal1;
			}
			else
			{
				re->hModel = cgs.media.shardMetal2;
			}
		}

		/*else if (type == 3) // ceramic
		   {
		   rval = rand()%2;

		   if (rval)
		   re->hModel = cgs.media.shardCeramic1;
		   else
		   re->hModel = cgs.media.shardCeramic2;
		   } */
		else if( type == FXTYPE_BRICK || type == FXTYPE_STONE )
		{
			// rubble
			rval = rand() % 3;

			if( rval == 1 )
			{
				re->hModel = cgs.media.shardRubble1;
			}
			else if( rval == 2 )
			{
				re->hModel = cgs.media.shardRubble2;
			}
			else
			{
				re->hModel = cgs.media.shardRubble3;
			}
		}
		else
		{
			CG_Printf( "CG_Debris has an unknown type\n" );
		}

		// location
		if( isflyingdebris )
		{
			le->pos.trType = TR_GRAVITY_LOW;
		}
		else
		{
			le->pos.trType = TR_GRAVITY;
		}

		VectorCopy( origin, le->pos.trBase );
		VectorNormalize( dir );
		VectorScale( dir, 10 * howmany, le->pos.trDelta );
		le->pos.trTime = cg.time;
		le->pos.trDelta[ 0 ] += ( ( random() * 100 ) - 50 );
		le->pos.trDelta[ 1 ] += ( ( random() * 100 ) - 50 );

		if( type )
		{
			le->pos.trDelta[ 2 ] = ( random() * 200 ) + 100; // randomize sort of a lot so they don't all land together
		}
		else
		{
			// glass
			le->pos.trDelta[ 2 ] = ( random() * 100 ) + 50; // randomize sort of a lot so they don't all land together
		}

		// rotation
		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[ 0 ] = rand() & 31;
		le->angles.trBase[ 1 ] = rand() & 31;
		le->angles.trBase[ 2 ] = rand() & 31;
		le->angles.trDelta[ 0 ] = ( 100 + ( rand() & 500 ) ) - 300;
		le->angles.trDelta[ 1 ] = ( 100 + ( rand() & 500 ) ) - 300;
		le->angles.trDelta[ 2 ] = ( 100 + ( rand() & 500 ) ) - 300;
	}
}

void CG_ShardJunk( centity_t *cent, vec3_t origin, vec3_t dir )
{
	localEntity_t *le;
	refEntity_t   *re;
	int           type;

	type = cent->currentState.density;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + random() * 5000;

	re->fadeStartTime = le->endTime - 1000;
	re->fadeEndTime = le->endTime;

	le->lifeRate = 1.0 / ( le->endTime - le->startTime );
	le->leFlags = LEF_TUMBLE;
	le->bounceFactor = 0.4;
	le->leMarkType = 0;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );

	re->hModel = cgs.media.shardJunk[ rand() % MAX_LOCKER_DEBRIS ];

	le->pos.trType = TR_GRAVITY;

	VectorCopy( origin, le->pos.trBase );
	VectorNormalize( dir );
	VectorScale( dir, 10 * 8, le->pos.trDelta );
	le->pos.trTime = cg.time;
	le->pos.trDelta[ 0 ] += ( ( random() * 100 ) - 50 );
	le->pos.trDelta[ 1 ] += ( ( random() * 100 ) - 50 );

	le->pos.trDelta[ 2 ] = ( random() * 100 ) + 50; // randomize sort of a lot so they don't all land together

	// rotation
	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	//le->angles.trBase[0] = rand()&31;
	//le->angles.trBase[1] = rand()&31;
	le->angles.trBase[ 2 ] = rand() & 31;

	//le->angles.trDelta[0] = (100 + (rand()&500)) - 300;
	//le->angles.trDelta[1] = (100 + (rand()&500)) - 300;
	le->angles.trDelta[ 2 ] = ( 100 + ( rand() & 500 ) ) - 300;
}

// Gordon: debris test
void CG_Debris( centity_t *cent, vec3_t origin, vec3_t dir )
{
	localEntity_t *le;
	refEntity_t   *re;
	int           type;

	type = cent->currentState.density;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + random() * 5000;

	re->fadeStartTime = le->endTime - 1000;
	re->fadeEndTime = le->endTime;

	le->lifeRate = 1.0 / ( le->endTime - le->startTime );
	le->leFlags = LEF_TUMBLE | LEF_TUMBLE_SLOW;
	le->bounceFactor = 0.4;
	le->leMarkType = 0;
	le->breakCount = 1;
	le->sizeScale = 0.5;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );

	re->hModel = cgs.inlineDrawModel[ cent->currentState.modelindex ];

	le->pos.trType = TR_GRAVITY;

	VectorCopy( origin, le->pos.trBase );
	VectorCopy( dir, le->pos.trDelta );
	le->pos.trTime = cg.time;

	// rotation
	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[ 2 ] = rand() & 31;

	le->angles.trDelta[ 2 ] = ( 100 + ( rand() & 500 ) ) - 300;
	le->angles.trDelta[ 2 ] = ( 50 + ( rand() & 400 ) ) - 100;
	le->angles.trDelta[ 2 ] = ( 50 + ( rand() & 400 ) ) - 100;
}

// ===================

//void CG_BatDeath( centity_t *cent )
//{
//  CG_ParticleExplosion( "blood", cent->lerpOrigin, vec3_origin, 400, 20, 30, qfalse );
//}

void CG_MortarImpact( centity_t *cent, vec3_t origin, int sfx, qboolean dist )
{
	if( sfx >= 0 )
	{
		trap_S_StartSound( origin, -1, CHAN_AUTO, cgs.media.sfx_mortarexp[ sfx ] );
	}

	if( dist )
	{
		vec3_t gorg, norm;
		float  gdist;

		VectorSubtract( origin, cg.refdef_current->vieworg, norm );
		gdist = VectorNormalize( norm );

		if( gdist > 1200 && gdist < 8000 )
		{
			// 1200 is max cam shakey dist (2*600) use gorg as the new sound origin
			VectorMA( cg.refdef_current->vieworg, 800, norm, gorg );  // non-distance falloff makes more sense; sfx2range was gdist*0.2
			// sfx2range is variable to give us minimum volume control different explosion sizes (see mortar, panzerfaust, and grenade)
			trap_S_StartSoundEx( gorg, -1, CHAN_WEAPON, cgs.media.sfx_mortarexpDist, SND_NOCUT );
		}

		if( cent->currentState.clientNum == cg.snap->ps.clientNum && cg.mortarImpactTime != -2 )
		{
			VectorCopy( origin, cg.mortarImpactPos );
			cg.mortarImpactTime = cg.time;
			cg.mortarImpactOutOfMap = qfalse;
		}
	}
}

void CG_MortarMiss( centity_t *cent, vec3_t origin )
{
	if( cent->currentState.clientNum == cg.snap->ps.clientNum && cg.mortarImpactTime != -2 )
	{
		VectorCopy( origin, cg.mortarImpactPos );
		cg.mortarImpactTime = cg.time;

		if( cent->currentState.density )
		{
			cg.mortarImpactOutOfMap = qtrue;
		}
		else
		{
			cg.mortarImpactOutOfMap = qfalse;
		}
	}
}

// a convenience function for all footstep sound playing
static void CG_StartFootStepSound( bg_playerclass_t *classInfo, entityState_t *es, sfxHandle_t sfx )
{
	if( cg_footsteps.integer )
	{
		trap_S_StartSound( NULL, es->number, CHAN_BODY, sfx );
	}
}

/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
extern void CG_AddBulletParticles( vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale );

// JPW NERVE
void        CG_MachineGunEjectBrass( centity_t *cent );
void        CG_MachineGunEjectBrassNew( centity_t *cent );

// jpw
#define DEBUGNAME( x ) if ( cg_debugEvents.integer ) { CG_Printf( x "\n" ); }
void CG_EntityEvent( centity_t *cent, vec3_t position )
{
	entityState_t    *es;
	int              event;
	vec3_t           dir;
	const char       *s;
	int              clientNum;
	clientInfo_t     *ci;
	char             tempStr[ MAX_QPATH ];
	bg_playerclass_t *classInfo;
	bg_character_t   *character;

// JPW NERVE copied here for mg42 SFX event
	vec3_t           porg, gorg, norm; // player/gun origin
	float            gdist;

// jpw

	static int footstepcnt = 0;
	static int splashfootstepcnt = 0;

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if( cg_debugEvents.integer )
	{
		CG_Printf( "time:%i ent:%3i  event:%3i ", cg.time, es->number, event );
	}

	if( !event )
	{
		DEBUGNAME( "ZEROEVENT" );
		return;
	}

	clientNum = es->clientNum;

	if( clientNum < 0 || clientNum >= MAX_CLIENTS )
	{
		clientNum = 0;
	}

	ci = &cgs.clientinfo[ clientNum ];
	classInfo = CG_PlayerClassForClientinfo( ci, cent );
	character = CG_CharacterForClientinfo( ci, cent );

	switch( event )
	{
			//
			// movement generated events
			//
		case EV_FOOTSTEP:
			DEBUGNAME( "EV_FOOTSTEP" );

			if( es->eventParm != FOOTSTEP_TOTAL )
			{
				if( es->eventParm )
				{
					CG_StartFootStepSound( classInfo, es, cgs.media.footsteps[ es->eventParm ][ footstepcnt ] );
				}
				else
				{
					CG_StartFootStepSound( classInfo, es, cgs.media.footsteps[ character->animModelInfo->footsteps ][ footstepcnt ] );
				}
			}

			break;

		case EV_FOOTSPLASH:
			DEBUGNAME( "EV_FOOTSPLASH" );
			CG_StartFootStepSound( classInfo, es, cgs.media.footsteps[ FOOTSTEP_SPLASH ][ splashfootstepcnt ] );
			break;

		case EV_FOOTWADE:
			DEBUGNAME( "EV_FOOTWADE" );
			CG_StartFootStepSound( classInfo, es, cgs.media.footsteps[ FOOTSTEP_SPLASH ][ splashfootstepcnt ] );
			break;

		case EV_SWIM:
			DEBUGNAME( "EV_SWIM" );
			CG_StartFootStepSound( classInfo, es, cgs.media.footsteps[ FOOTSTEP_SPLASH ][ footstepcnt ] );
			break;

		case EV_FALL_SHORT:
			DEBUGNAME( "EV_FALL_SHORT" );

			if( es->eventParm != FOOTSTEP_TOTAL )
			{
				if( es->eventParm )
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ es->eventParm ] );
				}
				else
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ character->animModelInfo->footsteps ] );
				}
			}

			if( clientNum == cg.predictedPlayerState.clientNum )
			{
				// smooth landing z changes
				cg.landChange = -8;
				cg.landTime = cg.time;
			}

			break;

		case EV_FALL_DMG_10:
			DEBUGNAME( "EV_FALL_DMG_10" );

			if( es->eventParm != FOOTSTEP_TOTAL )
			{
				if( es->eventParm )
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ es->eventParm ] );
				}
				else
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ character->animModelInfo->footsteps ] );
				}
			}

			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landHurt );
			cent->pe.painTime = cg.time; // don't play a pain sound right after this

			if( clientNum == cg.predictedPlayerState.clientNum )
			{
				// smooth landing z changes
				cg.landChange = -16;
				cg.landTime = cg.time;
			}

			break;

		case EV_FALL_DMG_15:
			DEBUGNAME( "EV_FALL_DMG_15" );

			if( es->eventParm != FOOTSTEP_TOTAL )
			{
				if( es->eventParm )
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ es->eventParm ] );
				}
				else
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ character->animModelInfo->footsteps ] );
				}
			}

			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landHurt );
			cent->pe.painTime = cg.time; // don't play a pain sound right after this

			if( clientNum == cg.predictedPlayerState.clientNum )
			{
				// smooth landing z changes
				cg.landChange = -16;
				cg.landTime = cg.time;
			}

			break;

		case EV_FALL_DMG_25:
			DEBUGNAME( "EV_FALL_DMG_25" );

			if( es->eventParm != FOOTSTEP_TOTAL )
			{
				if( es->eventParm )
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ es->eventParm ] );
				}
				else
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ character->animModelInfo->footsteps ] );
				}
			}

			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landHurt );
			cent->pe.painTime = cg.time; // don't play a pain sound right after this

			if( clientNum == cg.predictedPlayerState.clientNum )
			{
				// smooth landing z changes
				cg.landChange = -24;
				cg.landTime = cg.time;
			}

			break;

		case EV_FALL_DMG_50:
			DEBUGNAME( "EV_FALL_DMG_50" );

			if( es->eventParm != FOOTSTEP_TOTAL )
			{
				if( es->eventParm )
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ es->eventParm ] );
				}
				else
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ character->animModelInfo->footsteps ] );
				}
			}

			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landHurt );
			cent->pe.painTime = cg.time; // don't play a pain sound right after this

			if( clientNum == cg.predictedPlayerState.clientNum )
			{
				// smooth landing z changes
				cg.landChange = -24;
				cg.landTime = cg.time;
			}

			break;

		case EV_FALL_NDIE:
			DEBUGNAME( "EV_FALL_NDIE" );

			if( es->eventParm != FOOTSTEP_TOTAL )
			{
				if( es->eventParm )
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ es->eventParm ] );
				}
				else
				{
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound[ character->animModelInfo->footsteps ] );
				}
			}

			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landHurt );
			cent->pe.painTime = cg.time; // don't play a pain sound right after this
			// splat
			break;

		case EV_EXERT1:
			DEBUGNAME( "EV_EXERT1" );
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*exert1.wav" ) );
			break;

		case EV_EXERT2:
			DEBUGNAME( "EV_EXERT2" );
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*exert2.wav" ) );
			break;

		case EV_EXERT3:
			DEBUGNAME( "EV_EXERT3" );
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*exert3.wav" ) );
			break;

		case EV_STEP_4:
		case EV_STEP_8:
		case EV_STEP_12:
		case EV_STEP_16: // smooth out step up transitions
			DEBUGNAME( "EV_STEP" );
			{
				float oldStep;
				int   delta;
				int   step;

				if( clientNum != cg.predictedPlayerState.clientNum )
				{
					break;
				}

				// if we are interpolating, we don't need to smooth steps
				if( cg.demoPlayback || ( cg.snap->ps.pm_flags & PMF_FOLLOW ) || cg_nopredict.integer
#ifdef ALLOW_GSYNC
				    || cg_synchronousClients.integer
#endif // ALLOW_GSYNC
				  )
				{
					break;
				}

				// check for stepping up before a previous step is completed
				delta = cg.time - cg.stepTime;

				if( delta < STEP_TIME )
				{
					oldStep = cg.stepChange * ( STEP_TIME - delta ) / STEP_TIME;
				}
				else
				{
					oldStep = 0;
				}

				// add this amount
				step = 4 * ( event - EV_STEP_4 + 1 );
				cg.stepChange = oldStep + step;

				if( cg.stepChange > MAX_STEP_CHANGE )
				{
					cg.stepChange = MAX_STEP_CHANGE;
				}

				cg.stepTime = cg.time;
				break;
			}

		case EV_JUMP:
			DEBUGNAME( "EV_JUMP" );
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );
			break;

		case EV_TAUNT:
			DEBUGNAME( "EV_TAUNT" );
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*taunt.wav" ) );
			break;

		case EV_WATER_TOUCH:
			DEBUGNAME( "EV_WATER_TOUCH" );
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.watrInSound );
			break;

		case EV_WATER_LEAVE:
			DEBUGNAME( "EV_WATER_LEAVE" );
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );
			break;

		case EV_WATER_UNDER:
			DEBUGNAME( "EV_WATER_UNDER" );
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound );

			if( cg.clientNum == es->number )
			{
				cg.waterundertime = cg.time + HOLDBREATHTIME;
			}

//----(SA)  this fog stuff for underwater is really just a test for feasibility of creating the under-water effect that way.
//----(SA)  the related issues of load/savegames, death underwater, etc. are not handled at all.
//----(SA)  the actual problem, of course, is doing underwater stuff when the water is very turbulant and you can't simply
//----(SA)  do things based on the players head being above/below the water brushes top surface. (since the waves can potentially be /way/ above/below that)

			// DHM - Nerve :: causes problems in multiplayer...
			break;

		case EV_WATER_CLEAR:
			DEBUGNAME( "EV_WATER_CLEAR" );
			//trap_S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*gasp.wav" ) );
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );

			if( es->eventParm )
			{
				trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.watrGaspSound );
			}

			break;

		case EV_ITEM_PICKUP:
		case EV_ITEM_PICKUP_QUIET:
			DEBUGNAME( "EV_ITEM_PICKUP" );
			{
				gitem_t *item;
				int     index;

				index = es->eventParm; // player predicted

				if( index < 1 || index >= bg_numItems )
				{
					break;
				}

				item = &bg_itemlist[ index ];

				if( event == EV_ITEM_PICKUP )
				{
					// not quiet
					// powerups and team items will have a separate global sound, this one
					// will be played at prediction time
					if( item->giType == IT_TEAM )
					{
						trap_S_StartSound( NULL, es->number, CHAN_AUTO, trap_S_RegisterSound( "sound/misc/w_pkup.wav", qfalse ) );
					}
					else
					{
						trap_S_StartSound( NULL, es->number, CHAN_AUTO, trap_S_RegisterSound( item->pickup_sound, qfalse ) );
					}
				}

				// show icon and name on status bar
				if( es->number == cg.snap->ps.clientNum )
				{
					CG_ItemPickup( index );
				}

//----(SA)  draw the HUD items for a sec since this is a special item

				/*      if ( item->giType == IT_KEY)
				                                cg.itemFadeTime = cg.time + 1000;*/
			}
			break;

		case EV_GLOBAL_ITEM_PICKUP:
			DEBUGNAME( "EV_GLOBAL_ITEM_PICKUP" );
			{
				gitem_t *item;
				int     index;

				index = es->eventParm; // player predicted

				if( index < 1 || index >= bg_numItems )
				{
					break;
				}

				item = &bg_itemlist[ index ];

				if( *item->pickup_sound )
				{
					// powerup pickups are global
					trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, trap_S_RegisterSound( item->pickup_sound, qfalse ) );   // FIXME: precache
				}

				// show icon and name on status bar
				if( es->number == cg.snap->ps.clientNum )
				{
					CG_ItemPickup( index );
				}
			}
			break;

			//
			// weapon events
			//
		case EV_VENOM:
			DEBUGNAME( "EV_VENOM" );
//      CG_VenomFire( es, qfalse );
			break;

		case EV_WEAP_OVERHEAT:
			DEBUGNAME( "EV_WEAP_OVERHEAT" );

			// start weapon idle animation
			if( es->number == cg.snap->ps.clientNum )
			{
				cg.predictedPlayerState.weapAnim =
				  ( ( cg.predictedPlayerState.weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | PM_IdleAnimForWeapon( cg.snap->ps.
				      weapon );
				cent->overheatTime = cg.time; // used to make the barrels smoke when overheated
			}

			if( BG_PlayerMounted( es->eFlags ) )
			{
				trap_S_StartSoundVControl( NULL, es->number, CHAN_AUTO, cgs.media.hWeaponHeatSnd, 255 );
			}
			else if( es->eFlags & EF_MOUNTEDTANK )
			{
				if( cg_entities[ cg_entities[ cg_entities[ es->number ].tagParent ].tankparent ].currentState.density & 8 )
				{
					trap_S_StartSoundVControl( NULL, es->number, CHAN_AUTO, cgs.media.hWeaponHeatSnd_2, 255 );
				}
				else
				{
					trap_S_StartSoundVControl( NULL, es->number, CHAN_AUTO, cgs.media.hWeaponHeatSnd, 255 );
				}
			}
			else if( cg_weapons[ es->weapon ].overheatSound )
			{
				trap_S_StartSound( NULL, es->number, CHAN_AUTO, cg_weapons[ es->weapon ].overheatSound );
			}

			break;

// JPW NERVE
		case EV_SPINUP:
			DEBUGNAME( "EV_SPINUP" );
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cg_weapons[ es->weapon ].spinupSound );
			break;

// jpw
		case EV_EMPTYCLIP:
			DEBUGNAME( "EV_EMPTYCLIP" );
			break;

		case EV_FILL_CLIP:
			DEBUGNAME( "EV_FILL_CLIP" );

			if( cgs.clientinfo[ cg.clientNum ].skill[ SK_LIGHT_WEAPONS ] >= 2 && BG_isLightWeaponSupportingFastReload( es->weapon ) &&
			    cg_weapons[ es->weapon ].reloadFastSound )
			{
				trap_S_StartSound( NULL, es->number, CHAN_WEAPON, cg_weapons[ es->weapon ].reloadFastSound );
			}
			else if( cg_weapons[ es->weapon ].reloadSound )
			{
				trap_S_StartSound( NULL, es->number, CHAN_WEAPON, cg_weapons[ es->weapon ].reloadSound );  // JPW NERVE following sherman's SP fix, should allow killing reload sound when player dies
			}

			break;

// JPW NERVE play a sound when engineer fixes MG42
		case EV_MG42_FIXED:
			DEBUGNAME( "EV_MG42_FIXED" );
			//trap_S_StartSound(NULL,es->number,CHAN_WEAPON,cg_weapons[WP_MAUSER].reloadSound); // Arnout: needs updating
			break;
// jpw

		case EV_NOAMMO:
		case EV_WEAPONSWITCHED:
			DEBUGNAME( "EV_NOAMMO" );

			if( ( es->weapon != WP_GRENADE_LAUNCHER ) &&
			    ( es->weapon != WP_GRENADE_PINEAPPLE ) &&
			    ( es->weapon != WP_DYNAMITE ) &&
			    ( es->weapon != WP_LANDMINE ) &&
			    ( es->weapon != WP_SATCHEL ) &&
			    ( es->weapon != WP_SATCHEL_DET ) &&
			    ( es->weapon != WP_TRIPMINE ) &&
			    ( es->weapon != WP_SMOKE_BOMB ) && ( es->weapon != WP_AMMO ) && ( es->weapon != WP_MEDKIT ) )
			{
				trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
			}

			if( es->number == cg.snap->ps.clientNum && ( ( cg_noAmmoAutoSwitch.integer > 0 && !CG_WeaponSelectable( cg.weaponSelect ) )
			    || es->weapon == WP_MORTAR_SET || es->weapon == WP_MOBILE_MG42_SET ||
			    es->weapon == WP_GRENADE_LAUNCHER || es->weapon == WP_GRENADE_PINEAPPLE ||
			    es->weapon == WP_DYNAMITE || es->weapon == WP_SMOKE_MARKER ||
			    es->weapon == WP_PANZERFAUST || es->weapon == WP_ARTY ||
			    es->weapon == WP_LANDMINE || es->weapon == WP_SATCHEL ||
			    es->weapon == WP_SATCHEL_DET || es->weapon == WP_TRIPMINE ||
			    es->weapon == WP_SMOKE_BOMB || es->weapon == WP_AMMO ||
			    es->weapon == WP_MEDKIT ) )
			{
				CG_OutOfAmmoChange( event == EV_WEAPONSWITCHED ? qfalse : qtrue );
			}

			break;

		case EV_CHANGE_WEAPON:
			DEBUGNAME( "EV_CHANGE_WEAPON" );
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.selectSound );
			break;

		case EV_CHANGE_WEAPON_2:
			DEBUGNAME( "EV_CHANGE_WEAPON" );

			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.selectSound );

			if( es->number == cg.snap->ps.clientNum )
			{
				int newweap = 0;

				// client will get this message if reloading while using an alternate weapon
				// client should voluntarily switch back to primary at that point
				switch( es->weapon )
				{
					case WP_FG42SCOPE:
						newweap = WP_FG42;
						break;

					case WP_GARAND_SCOPE:
						newweap = WP_GARAND;
						break;

					case WP_K43_SCOPE:
						newweap = WP_K43;
						break;

					default:
						break;
				}

				if( newweap )
				{
					CG_FinishWeaponChange( es->weapon, newweap );
				}
			}

			break;

		case EV_FIRE_WEAPON_MOUNTEDMG42:
		case EV_FIRE_WEAPON_MG42:
			VectorCopy( cent->currentState.pos.trBase, gorg );
			VectorCopy( cg.refdef_current->vieworg, porg );
			VectorSubtract( gorg, porg, norm );
			gdist = VectorNormalize( norm );

			if( gdist > 512 && gdist < 4096 )
			{
				VectorMA( cg.refdef_current->vieworg, 64, norm, gorg );

				if( cg_entities[ cg_entities[ cg_entities[ cent->currentState.number ].tagParent ].tankparent ].currentState.density & 8 )
				{
					// should we use a browning?
					trap_S_StartSoundEx( gorg, cent->currentState.number, CHAN_WEAPON, cgs.media.hWeaponEchoSnd_2, SND_NOCUT );
				}
				else
				{
					trap_S_StartSoundEx( gorg, cent->currentState.number, CHAN_WEAPON, cgs.media.hWeaponEchoSnd, SND_NOCUT );
				}
			}

			DEBUGNAME( "EV_FIRE_WEAPON_MG42" );
			CG_FireWeapon( cent );
			break;

		case EV_FIRE_WEAPON_AAGUN:
			DEBUGNAME( "EV_FIRE_WEAPON_AAGUN" );
			CG_FireWeapon( cent );
			break;

		case EV_FIRE_WEAPON:
		case EV_FIRE_WEAPONB:
			DEBUGNAME( "EV_FIRE_WEAPON" );

			if( cent->currentState.clientNum == cg.snap->ps.clientNum && cg.snap->ps.eFlags & EF_ZOOMING )
			{
				// to stop airstrike sfx
				break;
			}

			CG_FireWeapon( cent );

			if( event == EV_FIRE_WEAPONB )
			{
				// akimbo firing
				cent->akimboFire = qtrue;
			}
			else
			{
				cent->akimboFire = qfalse;
			}

			break;

		case EV_FIRE_WEAPON_LASTSHOT:
			DEBUGNAME( "EV_FIRE_WEAPON_LASTSHOT" );
			CG_FireWeapon( cent );
			break;

		case EV_NOFIRE_UNDERWATER:
			DEBUGNAME( "EV_NOFIRE_UNDERWATER" );

			if( cgs.media.noFireUnderwater )
			{
				trap_S_StartSound( NULL, es->number, CHAN_WEAPON, cgs.media.noFireUnderwater );
			}

			break;

		case EV_PLAYER_TELEPORT_IN:
			break;

		case EV_PLAYER_TELEPORT_OUT:
			break;

		case EV_ITEM_POP:
			break;

		case EV_ITEM_RESPAWN:
			break;

		case EV_GRENADE_BOUNCE:
			DEBUGNAME( "EV_GRENADE_BOUNCE" );

			// DYNAMITE // Gordon: or LANDMINE FIXME: change this? (mebe a metallic sound)
			if( es->weapon == WP_SATCHEL )
			{
				trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.satchelbounce1 );
			}
			else if( es->weapon == WP_DYNAMITE )
			{
				trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.dynamitebounce1 );
			}
			else if( es->weapon == WP_LANDMINE )
			{
				trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landminebounce1 );
			}
			else
			{
				// GRENADES
				if( es->eventParm != FOOTSTEP_TOTAL )
				{
					if( rand() & 1 )
					{
						trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce[ es->eventParm ][ 0 ] );
					}
					else
					{
						trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce[ es->eventParm ][ 1 ] );
					}
				}
			}

			break;

			/*  case EV_FLAMEBARREL_BOUNCE:
			                DEBUGNAME("EV_FLAMEBARREL_BOUNCE");
			                if ( rand() & 1 ) {
			                        trap_S_StartSound (NULL, es->number, CHAN_AUTO, cgs.media.fbarrelexp1 );
			                } else {
			                        trap_S_StartSound (NULL, es->number, CHAN_AUTO,  cgs.media.fbarrelexp2 );
			                }
			                break;*/

		case EV_RAILTRAIL:
			CG_RailTrail( &cgs.clientinfo[ es->otherEntityNum2 ], es->origin2, es->pos.trBase, es->dmgFlags );  //----(SA) added 'type' field
			break;

			//
			// missile impacts
			//
		case EV_MISSILE_HIT:
			DEBUGNAME( "EV_MISSILE_HIT" );
			ByteToDir( es->eventParm, dir );
			CG_MissileHitPlayer( cent, es->weapon, position, dir, es->otherEntityNum );

			if( es->weapon == WP_MORTAR_SET )
			{
				if( !es->legsAnim )
				{
					CG_MortarImpact( cent, position, 3, qtrue );
				}
				else
				{
					CG_MortarImpact( cent, position, -1, qtrue );
				}
			}

			break;

		case EV_MISSILE_MISS_SMALL:
			DEBUGNAME( "EV_MISSILE_MISS" );
			ByteToDir( es->eventParm, dir );
			CG_MissileHitWallSmall( es->weapon, 0, position, dir );
			break;

		case EV_MISSILE_MISS:
			DEBUGNAME( "EV_MISSILE_MISS" );
			ByteToDir( es->eventParm, dir );
			CG_MissileHitWall( es->weapon, 0, position, dir, 0 );  // (SA) modified to send missilehitwall surface parameters

			if( es->weapon == WP_MORTAR_SET )
			{
				if( !es->legsAnim )
				{
					CG_MortarImpact( cent, position, 3, qtrue );
				}
				else
				{
					CG_MortarImpact( cent, position, -1, qtrue );
				}
			}

			break;

		case EV_MISSILE_MISS_LARGE:
			DEBUGNAME( "EV_MISSILE_MISS_LARGE" );
			ByteToDir( es->eventParm, dir );

			if( es->weapon == WP_ARTY || es->weapon == WP_SMOKE_MARKER )
			{
				CG_MissileHitWall( es->weapon, 0, position, dir, 0 );  // (SA) modified to send missilehitwall surface parameters
			}
			else
			{
				CG_MissileHitWall( VERYBIGEXPLOSION, 0, position, dir, 0 );  // (SA) modified to send missilehitwall surface parameters
			}

			break;

		case EV_MORTAR_IMPACT:
			DEBUGNAME( "EV_MORTAR_IMPACT" );
			CG_MortarImpact( cent, position, rand() % 3, qfalse );
			break;

		case EV_MORTAR_MISS:
			DEBUGNAME( "EV_MORTAR_MISS" );
			CG_MortarMiss( cent, position );
			break;

		case EV_MG42BULLET_HIT_WALL:
			DEBUGNAME( "EV_MG42BULLET_HIT_WALL" );
			ByteToDir( es->eventParm, dir );
			CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD, es->otherEntityNum2, es->origin2[ 0 ],
			           es->effect1Time );
			break;

		case EV_MG42BULLET_HIT_FLESH:
			DEBUGNAME( "EV_MG42BULLET_HIT_FLESH" );
			CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm, es->otherEntityNum2, 0, es->effect1Time );
			break;

		case EV_BULLET_HIT_WALL:
			DEBUGNAME( "EV_BULLET_HIT_WALL" );
			ByteToDir( es->eventParm, dir );
			CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD, es->otherEntityNum2, es->origin2[ 0 ], 0 );
			break;

		case EV_BULLET_HIT_FLESH:
			DEBUGNAME( "EV_BULLET_HIT_FLESH" );
			CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm, es->otherEntityNum2, 0, 0 );
			break;

		case EV_POPUPBOOK:
		case EV_POPUP:
		case EV_GIVEPAGE:
			break;

		case EV_GENERAL_SOUND:
			DEBUGNAME( "EV_GENERAL_SOUND" );
			// Ridah, check for a sound script
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );

			if( !strstr( s, ".wav" ) )
			{
				if( CG_SoundPlaySoundScript( s, NULL, es->number, qfalse ) )
				{
					break;
				}

				// try with .wav
				Q_strncpyz( tempStr, s, sizeof( tempStr ) );
				Q_strcat( tempStr, sizeof( tempStr ), ".wav" );
				s = tempStr;
			}

			// done.
			if( cgs.gameSounds[ es->eventParm ] )
			{
				// xkan, 10/31/2002 - crank up the volume
				trap_S_StartSoundVControl( NULL, es->number, CHAN_VOICE, cgs.gameSounds[ es->eventParm ], 255 );
			}
			else
			{
				s = CG_ConfigString( CS_SOUNDS + es->eventParm );
				// xkan, 10/31/2002 - crank up the volume
				trap_S_StartSoundVControl( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, s ), 255 );
			}

			break;

		case EV_FX_SOUND:
			{
				sfxHandle_t sound;

				DEBUGNAME( "EV_FX_SOUND" );

				sound = random() * fxSounds[ es->eventParm ].max;

				if( fxSounds[ es->eventParm ].sound[ sound ] == -1 )
				{
					fxSounds[ es->eventParm ].sound[ sound ] = trap_S_RegisterSound( fxSounds[ es->eventParm ].soundfile[ sound ], qfalse );
				}

				sound = fxSounds[ es->eventParm ].sound[ sound ];

				trap_S_StartSoundVControl( NULL, es->number, CHAN_VOICE, sound, 255 );
			}
			break;

		case EV_GENERAL_SOUND_VOLUME:
			{
				int sound = es->eventParm;
				int volume = es->onFireStart;

				DEBUGNAME( "EV_GENERAL_SOUND_VOLUME" );
				// Ridah, check for a sound script
				s = CG_ConfigString( CS_SOUNDS + sound );

				if( !strstr( s, ".wav" ) )
				{
					if( CG_SoundPlaySoundScript( s, NULL, es->number, qfalse ) )
					{
						break;
					}

					// try with .wav
					Q_strncpyz( tempStr, s, sizeof( tempStr ) );
					Q_strcat( tempStr, sizeof( tempStr ), ".wav" );
					s = tempStr;
				}

				// done.
				if( cgs.gameSounds[ sound ] )
				{
					trap_S_StartSoundVControl( NULL, es->number, CHAN_VOICE, cgs.gameSounds[ sound ], volume );
				}
				else
				{
					s = CG_ConfigString( CS_SOUNDS + sound );
					trap_S_StartSoundVControl( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, s ), volume );
				}
			}
			break;

		case EV_GLOBAL_TEAM_SOUND:
			DEBUGNAME( "EV_GLOBAL_TEAM_SOUND" );

			if( cgs.clientinfo[ cg.snap->ps.clientNum ].team != es->teamNum )
			{
				break;
			}

		case EV_GLOBAL_SOUND: // play from the player's head so it never diminishes
			DEBUGNAME( "EV_GLOBAL_SOUND" );
			// Ridah, check for a sound script
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );

			if( !strstr( s, ".wav" ) )
			{
				if( CG_SoundPlaySoundScript( s, NULL, -1, qtrue ) )
				{
					break;
				}

				// try with .wav
				Q_strncpyz( tempStr, s, sizeof( tempStr ) );
				Q_strcat( tempStr, sizeof( tempStr ), ".wav" );
				s = tempStr;
			}

			if( cgs.gameSounds[ es->eventParm ] )
			{
				trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[ es->eventParm ] );
			}
			else
			{
				s = CG_ConfigString( CS_SOUNDS + es->eventParm );
				trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, CG_CustomSound( es->number, s ) );
			}

			break;

			// DHM - Nerve
		case EV_GLOBAL_CLIENT_SOUND:
			DEBUGNAME( "EV_GLOBAL_CLIENT_SOUND" );

			if( cg.snap->ps.clientNum == es->teamNum )
			{
				s = CG_ConfigString( CS_SOUNDS + es->eventParm );

				if( !strstr( s, ".wav" ) )
				{
					if( CG_SoundPlaySoundScript( s, NULL, -1, ( es->effect1Time ? qfalse : qtrue ) ) )
					{
						break;
					}

					// try with .wav
					Q_strncpyz( tempStr, s, sizeof( tempStr ) );
					Q_strcat( tempStr, sizeof( tempStr ), ".wav" );
					s = tempStr;
				}

				// done.
				if( cgs.gameSounds[ es->eventParm ] )
				{
					trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[ es->eventParm ] );
				}
				else
				{
					s = CG_ConfigString( CS_SOUNDS + es->eventParm );
					trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, CG_CustomSound( es->number, s ) );
				}
			}

			break;
			// dhm - end

		case EV_PAIN:
			// local player sounds are triggered in CG_CheckLocalSounds,
			// so ignore events on the player
			DEBUGNAME( "EV_PAIN" );

			if( cent->currentState.number != cg.snap->ps.clientNum )
			{
				CG_PainEvent( cent, es->eventParm, qfalse );
			}

			break;

		case EV_CROUCH_PAIN:
			// local player sounds are triggered in CG_CheckLocalSounds,
			// so ignore events on the player
			DEBUGNAME( "EV_PAIN" );

			if( cent->currentState.number != cg.snap->ps.clientNum )
			{
				CG_PainEvent( cent, es->eventParm, qtrue );
			}

			break;

		case EV_DEATH1:
		case EV_DEATH2:
		case EV_DEATH3:
			DEBUGNAME( "EV_DEATHx" );
			trap_S_StartSound( NULL, es->number, CHAN_VOICE,
			                   CG_CustomSound( es->number, va( "*death%i.wav", event - EV_DEATH1 + 1 ) ) );
			break;

		case EV_OBITUARY:
			DEBUGNAME( "EV_OBITUARY" );
			CG_Obituary( es );
			break;

			// JPW NERVE -- swiped from SP/Sherman
		case EV_STOPSTREAMINGSOUND:
			DEBUGNAME( "EV_STOPLOOPINGSOUND" );
//      trap_S_StopStreamingSound( es->number );
			trap_S_StartSoundEx( NULL, es->number, CHAN_WEAPON, 0, SND_CUTOFF_ALL );  // kill weapon sound (could be reloading)
			break;

		case EV_LOSE_HAT:
			DEBUGNAME( "EV_LOSE_HAT" );
			ByteToDir( es->eventParm, dir );
			CG_LoseHat( cent, dir );
			break;

		case EV_GIB_PLAYER:
			DEBUGNAME( "EV_GIB_PLAYER" );
			trap_S_StartSound( es->pos.trBase, -1, CHAN_AUTO, cgs.media.gibSound );
			ByteToDir( es->eventParm, dir );
			CG_GibPlayer( cent, cent->lerpOrigin, dir );
			break;

		case EV_STOPLOOPINGSOUND:
			DEBUGNAME( "EV_STOPLOOPINGSOUND" );
			es->loopSound = 0;
			break;

		case EV_DEBUG_LINE:
			DEBUGNAME( "EV_DEBUG_LINE" );
			CG_Beam( cent );
			break;

			// Rafael particles
		case EV_SMOKE:
			DEBUGNAME( "EV_SMOKE" );

			if( cent->currentState.density == 3 )
			{
				CG_ParticleSmoke( cgs.media.smokePuffShaderdirty, cent );
			}
			else if( !( cent->currentState.density ) )
			{
				CG_ParticleSmoke( cgs.media.smokePuffShader, cent );
			}
			else
			{
				CG_ParticleSmoke( cgs.media.smokePuffShader, cent );
			}

			break;

		case EV_FLAMETHROWER_EFFECT:
			CG_FireFlameChunks( cent, cent->currentState.origin, cent->currentState.apos.trBase, 0.6, 2 );
			break;

		case EV_DUST:
			CG_ParticleDust( cent, cent->currentState.origin, cent->currentState.angles );
			break;

		case EV_RUMBLE_EFX:
			{
				float pitch, yaw;

				pitch = cent->currentState.angles[ 0 ];
				yaw = cent->currentState.angles[ 1 ];
				CG_RumbleEfx( pitch, yaw );
			}
			break;

		case EV_CONCUSSIVE:
			CG_Concussive( cent );
			break;

		case EV_EMITTER:
			{
				localEntity_t *le;

				le = CG_AllocLocalEntity();
				le->leType = LE_EMITTER;
				le->startTime = cg.time;
				le->endTime = le->startTime + 20000;
				le->pos.trType = TR_STATIONARY;
				VectorCopy( cent->currentState.origin, le->pos.trBase );
				VectorCopy( cent->currentState.origin2, le->angles.trBase );
				le->ownerNum = 0;
			}
			break;

		case EV_OILPARTICLES:
			CG_Particle_OilParticle( cgs.media.oilParticle, cent->currentState.origin, cent->currentState.origin2,
			                         cent->currentState.time, cent->currentState.density );
			break;

		case EV_OILSLICK:
			CG_Particle_OilSlick( cgs.media.oilSlick, cent );
			break;

		case EV_OILSLICKREMOVE:
			CG_OilSlickRemove( cent );
			break;

		case EV_MG42EFX:
			CG_MG42EFX( cent );
			break;

		case EV_SPARKS_ELECTRIC:
		case EV_SPARKS:
			{
				int    numsparks;
				int    i;
				int    duration;
				float  x, y;
				float  speed;
				vec3_t source, dest;

				if( !( cent->currentState.density ) )
				{
					cent->currentState.density = 1;
				}

				numsparks = rand() % cent->currentState.density;
				duration = cent->currentState.frame;
				x = cent->currentState.angles2[ 0 ];
				y = cent->currentState.angles2[ 1 ];
				speed = cent->currentState.angles2[ 2 ];

				if( !numsparks )
				{
					numsparks = 1;
				}

				for( i = 0; i < numsparks; i++ )
				{
					if( event == EV_SPARKS_ELECTRIC )
					{
						VectorCopy( cent->currentState.origin, source );

						VectorCopy( source, dest );
						dest[ 0 ] += ( ( rand() & 31 ) - 16 );
						dest[ 1 ] += ( ( rand() & 31 ) - 16 );
						dest[ 2 ] += ( ( rand() & 31 ) - 16 );

						CG_Tracer( source, dest, 1 );
					}
					else
					{
						CG_ParticleSparks( cent->currentState.origin, cent->currentState.angles, duration, x, y, speed );
					}
				}
			}
			break;

		case EV_GUNSPARKS:
			{
				int numsparks;
				int speed;

				//int   count;

				numsparks = cent->currentState.density;
				speed = cent->currentState.angles2[ 2 ];

				CG_AddBulletParticles( cent->currentState.origin, cent->currentState.angles, speed, 800, numsparks, 1.0f );
			}
			break;

			// Rafael snow pvs check
		case EV_SNOW_ON:
			CG_SnowLink( cent, qtrue );
			break;

		case EV_SNOW_OFF:
			CG_SnowLink( cent, qfalse );
			break;

		case EV_SNOWFLURRY:
			CG_ParticleSnowFlurry( cgs.media.snowShader, cent );
			break;

			// for func_exploding
		case EV_EXPLODE:
			DEBUGNAME( "EV_EXPLODE" );
			ByteToDir( es->eventParm, dir );
			CG_Explode( cent, position, dir, 0 );
			break;

		case EV_RUBBLE:
			DEBUGNAME( "EV_RUBBLE" );
			ByteToDir( es->eventParm, dir );
			CG_Rubble( cent, position, dir, 0 );
			break;

			// for target_effect
		case EV_EFFECT:
			DEBUGNAME( "EV_EFFECT" );
			ByteToDir( es->eventParm, dir );
			CG_Effect( cent, position, dir );
			break;

		case EV_MORTAREFX: // mortar firing
			DEBUGNAME( "EV_MORTAREFX" );
			CG_MortarEFX( cent );
			break;

		case EV_SHARD:
			ByteToDir( es->eventParm, dir );
			CG_Shard( cent, position, dir );
			break;

		case EV_JUNK:
			ByteToDir( es->eventParm, dir );
			{
				int i;
				int rval;

				rval = rand() % 3 + 3;

				for( i = 0; i < rval; i++ )
				{
					CG_ShardJunk( cent, position, dir );
				}
			}
			break;

		case EV_DISGUISE_SOUND:
			trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.uniformPickup );
			break;

		case EV_BUILDDECAYED_SOUND:
			trap_S_StartSound( cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, cgs.media.buildDecayedSound );
			break;

			// Gordon: debris test
		case EV_DEBRIS:
			CG_Debris( cent, position, cent->currentState.origin2 );
			break;
			// ===================

		case EV_SHAKE:
			{
				vec3_t v;
				float  len;

				DEBUGNAME( "EV_SHAKE" );

				VectorSubtract( cg.snap->ps.origin, cent->lerpOrigin, v );
				len = VectorLength( v );

				if( len > cent->currentState.onFireStart )
				{
					break;
				}

				len = 1.0f - ( len / ( float ) cent->currentState.onFireStart );
				len = min( 1.f, len );

				CG_StartShakeCamera( len );
			}

			break;

		case EV_ALERT_SPEAKER:
			DEBUGNAME( "EV_ALERT_SPEAKER" );

			switch( cent->currentState.otherEntityNum2 )
			{
				case 1:
					CG_UnsetActiveOnScriptSpeaker( cent->currentState.otherEntityNum );
					break;

				case 2:
					CG_SetActiveOnScriptSpeaker( cent->currentState.otherEntityNum );
					break;

				case 0:
				default:
					CG_ToggleActiveOnScriptSpeaker( cent->currentState.otherEntityNum );
					break;
			}

			break;

		case EV_POPUPMESSAGE:
			{
				const char *str = CG_GetPMItemText( cent );
				qhandle_t  shader = CG_GetPMItemIcon( cent );

				if( str )
				{
					CG_AddPMItem( cent->currentState.effect1Time, str, shader );
				}

				CG_PlayPMItemSound( cent );
			}
			break;

		case EV_AIRSTRIKEMESSAGE:
			{
				const char *wav = NULL;

				switch( cent->currentState.density )
				{
					case 0: // too many called
						if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS )
						{
							wav = "axis_hq_airstrike_denied";
						}
						else
						{
							wav = "allies_hq_airstrike_denied";
						}

						break;

					case 1: // aborting can't see target
						if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS )
						{
							wav = "axis_hq_airstrike_abort";
						}
						else
						{
							wav = "allies_hq_airstrike_abort";
						}

						break;

					case 2: // firing for effect
						if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS )
						{
							wav = "axis_hq_airstrike";
						}
						else
						{
							wav = "allies_hq_airstrike";
						}

						break;
				}

				if( wav )
				{
					CG_SoundPlaySoundScript( wav, NULL, -1, ( es->effect1Time ? qfalse : qtrue ) );
				}
			}
			break;

		case EV_ARTYMESSAGE:
			{
				const char *wav = NULL;

				switch( cent->currentState.density )
				{
					case 0: // too many called
						if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS )
						{
							wav = "axis_hq_ffe_denied";
						}
						else
						{
							wav = "allies_hq_ffe_denied";
						}

						break;

					case 1: // aborting can't see target
						if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS )
						{
							wav = "axis_hq_ffe_abort";
						}
						else
						{
							wav = "allies_hq_ffe_abort";
						}

						break;

					case 2: // firing for effect
						if( cgs.clientinfo[ cg.snap->ps.clientNum ].team == TEAM_AXIS )
						{
							wav = "axis_hq_ffe";
						}
						else
						{
							wav = "allies_hq_ffe";
						}

						break;
				}

				if( wav )
				{
					CG_SoundPlaySoundScript( wav, NULL, -1, ( es->effect1Time ? qfalse : qtrue ) );
				}
			}
			break;

		case EV_MEDIC_CALL:
			switch( cgs.clientinfo[ cent->currentState.number ].team )
			{
				case TEAM_AXIS:
					trap_S_StartSound( NULL, cent->currentState.number, CHAN_AUTO, cgs.media.sndMedicCall[ 0 ] );
					break;

				case TEAM_ALLIES:
					trap_S_StartSound( NULL, cent->currentState.number, CHAN_AUTO, cgs.media.sndMedicCall[ 1 ] );
					break;

				default: // shouldn't happen
					break;
			}

			break;

		default:
			DEBUGNAME( "UNKNOWN" );
			CG_Error( "Unknown event: %i", event );
			break;
	}

	{
		int rval;

		rval = rand() & 3;

		if( splashfootstepcnt != rval )
		{
			splashfootstepcnt = rval;
		}
		else
		{
			splashfootstepcnt++;
		}

		if( splashfootstepcnt > 3 )
		{
			splashfootstepcnt = 0;
		}

		if( footstepcnt != rval )
		{
			footstepcnt = rval;
		}
		else
		{
			footstepcnt++;
		}

		if( footstepcnt > 3 )
		{
			footstepcnt = 0;
		}
	}
}

/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents( centity_t *cent )
{
	int i, event;

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin, qfalse, cent->currentState.effect2Time );
	CG_SetEntitySoundPosition( cent );

	// check for event-only entities
	if( cent->currentState.eType > ET_EVENTS )
	{
		if( cent->previousEvent )
		{
			//goto skipEvent;
			return; // already fired
		}

		// if this is a player event set the entity number of the client entity number
//(SA) note: EF_PLAYER_EVENT never set
//      if ( cent->currentState.eFlags & EF_PLAYER_EVENT ) {
//          cent->currentState.number = cent->currentState.otherEntityNum;
//      }

		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	}
	else
	{
		// DHM - Nerve :: Entities that make it here are Not TempEntities.
		//      As far as we could tell, for all non-TempEntities, the
		//      circular 'events' list contains the valid events.  So we
		//      skip processing the single 'event' field and go straight
		//      to the circular list.

		goto skipEvent;

		/*
		   // check for events riding with another entity
		   if ( cent->currentState.event == cent->previousEvent ) {
		   goto skipEvent;
		   //return;
		   }
		   cent->previousEvent = cent->currentState.event;
		   if ( ( cent->currentState.event & ~EV_EVENT_BITS ) == 0 ) {
		   goto skipEvent;
		   //return;
		   }
		 */
		// dhm - end
	}

	CG_EntityEvent( cent, cent->lerpOrigin );
	// DHM - Nerve :: Temp ents return after processing
	return;

skipEvent:

	// check the sequencial list
	// if we've added more events than can fit into the list, make sure we only add them once
	if( cent->currentState.eventSequence < cent->previousEventSequence )
	{
		cent->previousEventSequence -= ( 1 << 8 ); // eventSequence is sent as an 8-bit through network stream
	}

	if( cent->currentState.eventSequence - cent->previousEventSequence > MAX_EVENTS )
	{
		cent->previousEventSequence = cent->currentState.eventSequence - MAX_EVENTS;
	}

	for( i = cent->previousEventSequence; i != cent->currentState.eventSequence; i++ )
	{
		event = cent->currentState.events[ i & ( MAX_EVENTS - 1 ) ];

		cent->currentState.event = event;
		cent->currentState.eventParm = cent->currentState.eventParms[ i & ( MAX_EVENTS - 1 ) ];
		CG_EntityEvent( cent, cent->lerpOrigin );
	}

	cent->previousEventSequence = cent->currentState.eventSequence;

	// set the event back so we don't think it's changed next frame (unless it really has)
	cent->currentState.event = cent->previousEvent;
}
