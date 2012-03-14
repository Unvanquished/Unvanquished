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

/*
 * name:    g_items.c
 *
 * desc:    Items are any object that a player can touch to gain some effect.
 *        Pickup will return the number of seconds until they should respawn.
 *        all items should pop when dropped in lava or slime.
 *        Respawnable items don't actually go away when picked up, they are
 *        just made invisible and untouchable.  This allows them to ride
 *        movers and respawn apropriately.
 *
*/

#include "g_local.h"
#ifdef OMNIBOT
#include "../../omnibot/et/g_etbot_interface.h"
#endif

#define RESPAWN_SP           -1
#define RESPAWN_KEY          4
#define RESPAWN_ARMOR        25
#define RESPAWN_TEAM_WEAPON  30
#define RESPAWN_HEALTH       35
#define RESPAWN_AMMO         40
#define RESPAWN_HOLDABLE     60
#define RESPAWN_MEGAHEALTH   120
#define RESPAWN_POWERUP      120
#define RESPAWN_PARTIAL      998 // for multi-stage ammo/health
#define RESPAWN_PARTIAL_DONE 999 // for multi-stage ammo/health

//======================================================================

int Pickup_Powerup( gentity_t *ent, gentity_t *other )
{
	int       quantity;
	int       i;
	gclient_t *client;

	if ( !other->client->ps.powerups[ ent->item->giTag ] )
	{
		// some powerups are time based on how long the powerup is /used/
		// rather than timed from when the player picks it up.
		if ( ent->item->giTag == PW_NOFATIGUE )
		{
		}
		else
		{
			// round timing to seconds to make multiple powerup timers
			// count in sync
			other->client->ps.powerups[ ent->item->giTag ] = level.time - ( level.time % 1000 );
		}
	}

	// if an amount was specified in the ent, use it
	if ( ent->count )
	{
		quantity = ent->count;
	}
	else
	{
		quantity = ent->item->quantity;
	}

	other->client->ps.powerups[ ent->item->giTag ] += quantity * 1000;

	// brandy also gives a little health (10)
	if ( ent->item->giTag == PW_NOFATIGUE )
	{
		if ( Q_stricmp( ent->item->classname, "item_stamina_brandy" ) == 0 )
		{
			other->health += 10;

			if ( other->health > other->client->ps.stats[ STAT_MAX_HEALTH ] )
			{
				other->health = other->client->ps.stats[ STAT_MAX_HEALTH ];
			}

			other->client->ps.stats[ STAT_HEALTH ] = other->health;
		}
	}

	// give any nearby players a "denied" anti-reward
	for ( i = 0; i < level.maxclients; i++ )
	{
		vec3_t  delta;
		float   len;
		vec3_t  forward;
		trace_t tr;

		client = &level.clients[ i ];

		if ( client == other->client )
		{
			continue;
		}

		if ( client->pers.connected == CON_DISCONNECTED )
		{
			continue;
		}

		if ( client->ps.stats[ STAT_HEALTH ] <= 0 )
		{
			continue;
		}

		// if too far away, no sound
		VectorSubtract( ent->s.pos.trBase, client->ps.origin, delta );
		len = VectorNormalize( delta );

		if ( len > 192 )
		{
			continue;
		}

		// if not facing, no sound
		AngleVectors( client->ps.viewangles, forward, NULL, NULL );

		if ( DotProduct( delta, forward ) < 0.4 )
		{
			continue;
		}

		// if not line of sight, no sound
		trap_Trace( &tr, client->ps.origin, NULL, NULL, ent->s.pos.trBase, ENTITYNUM_NONE, CONTENTS_SOLID );

		if ( tr.fraction != 1.0 )
		{
			continue;
		}
	}

	if ( ent->s.density == 2 )
	{
		// multi-stage health first stage
		return RESPAWN_PARTIAL;
	}
	else if ( ent->s.density == 1 )
	{
		// last stage, leave the plate
		return RESPAWN_PARTIAL_DONE;
	}

	return RESPAWN_POWERUP;
}

//----(SA) Wolf keys
//======================================================================
int Pickup_Key( gentity_t *ent, gentity_t *other )
{
	other->client->ps.stats[ STAT_KEYS ] |= ( 1 << ent->item->giTag );
	return RESPAWN_KEY;
}

#ifdef KITS

// START Mad Doc - TDF
// in single player, the player can pick up the 'kits' of dead allies. Kits contain ammo for the special weapons
// that bot was carrying
int Pickup_Kit( gentity_t *ent, gentity_t *other )
{
	switch ( ent->item->giTag )
	{
		case KIT_ENGINEER:
			drop->item->giAmmoIndex = ps->ammoclip[ BG_FindClipForWeapon( WP_LANDMINE ) ];
			drop->item->giClipIndex = ps->ammoclip[ BG_FindClipForWeapon( WP_DYNAMITE ) ];
			break;
	}

case PC_FIELDOPS:
	{
		drop->item->giAmmoIndex = ps->ammoclip[ BG_FindClipForWeapon( WP_AMMO ) ];
		drop->item->giClipIndex = ps->ammoclip[ BG_FindClipForWeapon( WP_SMOKE_MARKER ) ];
		break;
	}

case PC_COVERTOPS:
	{
		drop->item->giAmmoIndex = ps->ammoclip[ BG_FindClipForWeapon( WP_SMOKE_BOMB ) ];
		// no second special for covert ops
		break;
	}
}

// END Mad Doc - TDF

#endif

/*
==============
Pickup_Clipboard
==============
*/
int Pickup_Clipboard( gentity_t *ent, gentity_t *other )
{
	if ( ent->spawnflags & 4 )
	{
		return 0; // leave in world
	}

	return -1;
}

/*
==============
Pickup_Treasure
==============
*/
int Pickup_Treasure( gentity_t *ent, gentity_t *other )
{
	return -1;
}

/*
==============
UseHoldableItem
        server side handling of holdable item use
==============
*/
void UseHoldableItem( gentity_t *ent, int item )
{
	switch ( item )
	{
		case HI_MEDKIT:
			ent->health = ent->client->ps.stats[ STAT_MAX_HEALTH ];
			break;

		case HI_WINE: // 1921 Chateu Lafite - gives 25 pts health up to max health
			ent->health += 25;

			if ( ent->health > ent->client->ps.stats[ STAT_MAX_HEALTH ] )
			{
				ent->health = ent->client->ps.stats[ STAT_MAX_HEALTH ];
			}

			break;

		case HI_SKULL: // skull of invulnerable - 30 sec invincible
			ent->client->ps.powerups[ PW_INVULNERABLE ] = level.time + 30000;
			break;

		case HI_WATER: // protection from drowning - 30 sec underwater breathing time
			ent->client->ps.powerups[ PW_BREATHER ] = 30000;
			break;

		case HI_ELECTRIC: // protection from electric attacks - absorbs 500 points of electric damage
			ent->client->ps.powerups[ PW_ELECTRIC ] = 500;
			break;

		case HI_FIRE: // protection from fire attacks - absorbs 500 points of fire damage
			ent->client->ps.powerups[ PW_FIRE ] = 500;
			break;

		case HI_STAMINA: // restores fatigue bar and sets "nofatigue" for a time period (currently forced to 60 sec)
			//----(SA)  NOTE:   currently only gives free nofatigue time, doesn't reset fatigue bar.
			//                  (this is because I'd like the restore to be visually gradual (on the HUD item representing
			//                  current status of your fatigue) rather than snapping back to 'full')
			ent->client->ps.powerups[ PW_NOFATIGUE ] = 60000;
			break;

		case HI_BOOK1:
		case HI_BOOK2:
		case HI_BOOK3:
			//G_AddEvent(   ent, EV_POPUPBOOK, (item - HI_BOOK1)+1 );
			break;
	}
}

//======================================================================

int Pickup_Holdable( gentity_t *ent, gentity_t *other )
{
	return RESPAWN_HOLDABLE;
}

// xkan, 10/26/2002
// extracted from Fill_Clip: add the specified ammount of ammo into the clip
// returns whether ammo was added to the clip
int AddToClip( playerState_t *ps,  // which player
               int weapon, // weapon to add ammo for
               int ammomove, // ammount to add. 0 means fill the clip if possible
               int outOfReserve )
{
	// is the amount to be added out of reserve
	int inclip, maxclip;
	int ammoweap = BG_FindAmmoForWeapon( weapon );

	if ( weapon < WP_LUGER || weapon >= WP_NUM_WEAPONS )
	{
		return qfalse;
	}

	inclip = ps->ammoclip[ BG_FindClipForWeapon( weapon ) ];
	maxclip = GetAmmoTableData( weapon )->maxclip;

	if ( !ammomove )
	{
		// amount to add to the clip not specified
		ammomove = maxclip - inclip; // max amount that can be moved into the clip
	}
	else if ( ammomove > maxclip - inclip )
	{
		ammomove = maxclip - inclip;
	}

	if ( outOfReserve )
	{
		// cap move amount if it's more than you've got in reserve
		if ( ammomove > ps->ammo[ ammoweap ] )
		{
			ammomove = ps->ammo[ ammoweap ];
		}
	}

	if ( ammomove )
	{
		if ( outOfReserve )
		{
			ps->ammo[ ammoweap ] -= ammomove;
		}

		ps->ammoclip[ BG_FindClipForWeapon( weapon ) ] += ammomove;
		return qtrue;
	}

	return qfalse;
}

//======================================================================

/*
==============
Fill_Clip
        push reserve ammo into available space in the clip
==============
*/
void Fill_Clip( playerState_t *ps, int weapon )
{
	AddToClip( ps, weapon, 0, qtrue );
}

/*
==============
Add_Ammo
        Try to always add ammo here unless you have specific needs
        (like the AI "infinite ammo" where they get below 900 and force back up to 999)

        fillClip will push the ammo straight through into the clip and leave the rest in reserve
==============
*/
//----(SA)  modified
// xkan, 10/25/2002 - modified to return whether any ammo was added.
int Add_Ammo( gentity_t *ent, int weapon, int count, qboolean fillClip )
{
	int ammoweap = BG_FindAmmoForWeapon( weapon );
	int originalCount;
	int maxammo = BG_MaxAmmoForWeapon( ammoweap, ent->client->sess.skill );

	originalCount = ent->client->ps.ammo[ ammoweap ];

	if ( ammoweap == WP_GRENADE_LAUNCHER )
	{
		// make sure if he picks up a grenade that he get's the "launcher" too
		COM_BitSet( ent->client->ps.weapons, WP_GRENADE_LAUNCHER );
		fillClip = qtrue; // grenades always filter into the "clip"
	}
	else if ( ammoweap == WP_GRENADE_PINEAPPLE )
	{
		COM_BitSet( ent->client->ps.weapons, WP_GRENADE_PINEAPPLE );
		fillClip = qtrue; // grenades always filter into the "clip"
	}
	else if ( ammoweap == WP_DYNAMITE )
	{
		COM_BitSet( ent->client->ps.weapons, WP_DYNAMITE );
		fillClip = qtrue;
	}
	else if ( ammoweap == WP_SATCHEL_DET )
	{
		COM_BitSet( ent->client->ps.weapons, WP_SATCHEL_DET );
		fillClip = qtrue;
	}

	if ( fillClip )
	{
		Fill_Clip( &ent->client->ps, weapon );
	}

	if ( ammoweap == WP_PANZERFAUST || ammoweap == WP_FLAMETHROWER )
	{
		ent->client->ps.ammoclip[ ammoweap ] += count;

		if ( ent->client->ps.ammoclip[ ammoweap ] > maxammo )
		{
			ent->client->ps.ammoclip[ ammoweap ] = maxammo; // - ent->client->ps.ammoclip[BG_FindClipForWeapon(weapon)];
		}
	}
	else
	{
		ent->client->ps.ammo[ ammoweap ] += count;

		if ( ent->client->ps.ammo[ ammoweap ] > maxammo )
		{
			ent->client->ps.ammo[ ammoweap ] = maxammo; // - ent->client->ps.ammoclip[BG_FindClipForWeapon(weapon)];
		}
	}

	if ( count >= 999 )
	{
		// 'really, give /all/'
		ent->client->ps.ammo[ ammoweap ] = count;
	}

	return ( ent->client->ps.ammo[ ammoweap ] > originalCount );
}

/*
==============
Pickup_Ammo
==============
*/
int Pickup_Ammo( gentity_t *ent, gentity_t *other )
{
	// added some ammo pickups, so I'll use ent->item->quantity if no ent->count
	if ( ent->count )
	{
		Add_Ammo( other, ent->item->giTag, ent->count, qfalse );
	}
	else
	{
		Add_Ammo( other, ent->item->giTag, ent->item->quantity, qfalse );
	}

	return RESPAWN_AMMO;
}

// xkan, 9/18/2002 - Extracted AddMagicAmmo from Pickup_Weapon()

/*
=================================================================
AddMagicAmmo - added the specified number of clips of magic ammo
for any two-handed weapon

- returns whether any ammo was actually added
=================================================================
*/
qboolean AddMagicAmmo( gentity_t *receiver, int numOfClips )
{
	return BG_AddMagicAmmo( &receiver->client->ps, receiver->client->sess.skill, receiver->client->sess.sessionTeam, numOfClips );
}

//======================================================================

weapon_t G_GetPrimaryWeaponForClient( gclient_t *client )
{
	int              i;
	bg_playerclass_t *classInfo;

	if ( client->sess.sessionTeam != TEAM_ALLIES && client->sess.sessionTeam != TEAM_AXIS )
	{
		return WP_NONE;
	}

	if ( client->sess.skill[ SK_HEAVY_WEAPONS ] < 4 )
	{
		if ( COM_BitCheck( client->ps.weapons, WP_THOMPSON ) )
		{
			return WP_THOMPSON;
		}

		if ( COM_BitCheck( client->ps.weapons, WP_MP40 ) )
		{
			return WP_MP40;
		}
	}

	classInfo = &bg_allies_playerclasses[ client->sess.playerType ];

	for ( i = 0; i < MAX_WEAPS_PER_CLASS; i++ )
	{
		if ( classInfo->classWeapons[ i ] == WP_MP40 || classInfo->classWeapons[ i ] == WP_THOMPSON )
		{
			continue;
		}

		if ( COM_BitCheck( client->ps.weapons, classInfo->classWeapons[ i ] ) )
		{
			return classInfo->classWeapons[ i ];
		}
	}

	classInfo = &bg_axis_playerclasses[ client->sess.playerType ];

	for ( i = 0; i < MAX_WEAPS_PER_CLASS; i++ )
	{
		if ( classInfo->classWeapons[ i ] == WP_MP40 || classInfo->classWeapons[ i ] == WP_THOMPSON )
		{
			continue;
		}

		if ( COM_BitCheck( client->ps.weapons, classInfo->classWeapons[ i ] ) )
		{
			return classInfo->classWeapons[ i ];
		}
	}

	return WP_NONE;
}

void G_DropWeapon( gentity_t *ent, weapon_t weapon )
{
	vec3_t    angles, velocity, org, offset, mins, maxs;
	gclient_t *client = ent->client;
	gentity_t *ent2;
	gitem_t   *item;
	trace_t   tr;

	item = BG_FindItemForWeapon( weapon );
	VectorCopy( client->ps.viewangles, angles );

	// clamp pitch
	if ( angles[ PITCH ] < -30 )
	{
		angles[ PITCH ] = -30;
	}
	else if ( angles[ PITCH ] > 30 )
	{
		angles[ PITCH ] = 30;
	}

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 64, offset );
	offset[ 2 ] += client->ps.viewheight / 2.f;
	VectorScale( velocity, 75, velocity );
	velocity[ 2 ] += 50 + random() * 35;

	VectorAdd( client->ps.origin, offset, org );

	VectorSet( mins, -ITEM_RADIUS, -ITEM_RADIUS, 0 );
	VectorSet( maxs, ITEM_RADIUS, ITEM_RADIUS, 2 * ITEM_RADIUS );

	trap_Trace( &tr, client->ps.origin, mins, maxs, org, ent->s.number, MASK_SOLID );
	VectorCopy( tr.endpos, org );

	ent2 = LaunchItem( item, org, velocity, client->ps.clientNum );
	COM_BitClear( client->ps.weapons, weapon );

	if ( weapon == WP_KAR98 )
	{
		COM_BitClear( client->ps.weapons, WP_GPG40 );
	}
	else if ( weapon == WP_CARBINE )
	{
		COM_BitClear( client->ps.weapons, WP_M7 );
	}
	else if ( weapon == WP_FG42 )
	{
		COM_BitClear( client->ps.weapons, WP_FG42SCOPE );
	}
	else if ( weapon == WP_K43 )
	{
		COM_BitClear( client->ps.weapons, WP_K43_SCOPE );
	}
	else if ( weapon == WP_GARAND )
	{
		COM_BitClear( client->ps.weapons, WP_GARAND_SCOPE );
	}
	else if ( weapon == WP_MORTAR )
	{
		COM_BitClear( client->ps.weapons, WP_MORTAR_SET );
	}
	else if ( weapon == WP_MOBILE_MG42 )
	{
		COM_BitClear( client->ps.weapons, WP_MOBILE_MG42_SET );
	}

	// Clear out empty weapon, change to next best weapon
	G_AddEvent( ent, EV_WEAPONSWITCHED, 0 );

	if ( weapon == client->ps.weapon )
	{
		client->ps.weapon = 0;
	}

	if ( weapon == WP_MORTAR )
	{
		ent2->count = client->ps.ammo[ BG_FindAmmoForWeapon( weapon ) ] + client->ps.ammoclip[ BG_FindClipForWeapon( weapon ) ];
	}
	else
	{
		ent2->count = client->ps.ammoclip[ BG_FindClipForWeapon( weapon ) ];
	}

	if ( weapon == WP_KAR98 || weapon == WP_CARBINE )
	{
		ent2->delay = client->ps.ammo[ BG_FindAmmoForWeapon( weapAlts[ weapon ] ) ];
	}
	else
	{
		ent2->delay = 0;
	}

//  ent2->item->quantity = client->ps.ammoclip[BG_FindClipForWeapon(weapon)]; // Gordon: um, modifying an item is not a good idea
	client->ps.ammoclip[ BG_FindClipForWeapon( weapon ) ] = 0;

#ifdef OMNIBOT
	Bot_Event_RemoveWeapon( client->ps.clientNum, Bot_WeaponGameToBot( weapon ) );
#endif
}

// TAT 1/6/2003 - Bot picks up a new weapon
void BotPickupWeapon( int client, int weaponnum, qboolean alreadyHave );

qboolean G_CanPickupWeapon( weapon_t weapon, gentity_t *ent )
{
	if ( ent->client->sess.sessionTeam == TEAM_AXIS )
	{
		if ( weapon == WP_THOMPSON )
		{
			weapon = WP_MP40;
		}
		else if ( weapon == WP_CARBINE )
		{
			weapon = WP_KAR98;
		}
		else if ( weapon == WP_GARAND )
		{
			weapon = WP_K43;
		}
	}
	else if ( ent->client->sess.sessionTeam == TEAM_ALLIES )
	{
		if ( weapon == WP_MP40 )
		{
			weapon = WP_THOMPSON;
		}
		else if ( weapon == WP_KAR98 )
		{
			weapon = WP_CARBINE;
		}
		else if ( weapon == WP_K43 )
		{
			weapon = WP_GARAND;
		}
	}

	if ( ent->client->sess.skill[ SK_HEAVY_WEAPONS ] >= 4 && ( weapon == WP_THOMPSON || weapon == WP_MP40 ) )
	{
		return qfalse;
	}

	return BG_WeaponIsPrimaryForClassAndTeam( ent->client->sess.playerType, ent->client->sess.sessionTeam, weapon );
}

int Pickup_Weapon( gentity_t *ent, gentity_t *other )
{
	int      quantity;
	qboolean alreadyHave = qfalse;

	// JPW NERVE -- magic ammo for any two-handed weapon
	if ( ent->item->giTag == WP_AMMO )
	{
		AddMagicAmmo( other, ent->count );

		// if LT isn't giving ammo to self or another LT or the enemy, give him some props
		if ( other->client->ps.stats[ STAT_PLAYER_CLASS ] != PC_FIELDOPS )
		{
			if ( ent->parent && ent->parent->client && other->client->sess.sessionTeam == ent->parent->client->sess.sessionTeam )
			{
				if ( !( ent->parent->client->PCSpecialPickedUpCount % LT_SPECIAL_PICKUP_MOD ) )
				{
					AddScore( ent->parent, WOLF_AMMO_UP );

					if ( ent->parent && ent->parent->client )
					{
						G_LogPrintf( "Ammo_Pack: %ld %ld\n", ( long )( ent->parent - g_entities ), ( long )( other - g_entities ) );    // OSP
					}
				}

				ent->parent->client->PCSpecialPickedUpCount++;
				G_AddSkillPoints( ent->parent, SK_SIGNALS, 1.f );
				G_DebugAddSkillPoints( ent->parent, SK_SIGNALS, 1.f, "ammo pack picked up" );
#ifdef OMNIBOT

				if ( ent->parent )
				{
					Bot_Event_RecievedAmmo( other - g_entities, ent->parent );
				}

#endif

				// extracted code originally here into AddMagicAmmo -xkan, 9/18/2002
				// add 1 clip of magic ammo for any two-handed weapon
			}

			return RESPAWN_SP;
		}
	}

	quantity = ent->count;

	// check if player already had the weapon
	alreadyHave = COM_BitCheck( other->client->ps.weapons, ent->item->giTag );

	// JPW NERVE  prevents drop/pickup weapon "quick reload" exploit
	if ( alreadyHave )
	{
		Add_Ammo( other, ent->item->giTag, quantity, qfalse );

		// Gordon: secondary weapon ammo
		if ( ent->delay )
		{
			Add_Ammo( other, weapAlts[ ent->item->giTag ], ent->delay, qfalse );
		}
	}
	else
	{
		if ( level.time - other->client->dropWeaponTime < 1000 )
		{
			return 0;
		}

		if ( other->client->ps.weapon == WP_MORTAR_SET || other->client->ps.weapon == WP_MOBILE_MG42_SET )
		{
			return 0;
		}

		// See if we can pick it up
		if ( G_CanPickupWeapon( ent->item->giTag, other ) )
		{
			weapon_t primaryWeapon = G_GetPrimaryWeaponForClient( other->client );

			// rain - added parens around ambiguous &&
			if ( primaryWeapon ||
			     ( other->client->sess.playerType == PC_SOLDIER && other->client->sess.skill[ SK_HEAVY_WEAPONS ] >= 4 ) )
			{
				if ( primaryWeapon )
				{
					// drop our primary weapon
					G_DropWeapon( other, primaryWeapon );
				}

				// now pickup the other one
				other->client->dropWeaponTime = level.time;

				// add the weapon
				COM_BitSet( other->client->ps.weapons, ent->item->giTag );

				// DHM - Fixup mauser/sniper issues
				if ( ent->item->giTag == WP_FG42 )
				{
					COM_BitSet( other->client->ps.weapons, WP_FG42SCOPE );
				}
				else if ( ent->item->giTag == WP_GARAND )
				{
					COM_BitSet( other->client->ps.weapons, WP_GARAND_SCOPE );
				}
				else if ( ent->item->giTag == WP_K43 )
				{
					COM_BitSet( other->client->ps.weapons, WP_K43_SCOPE );
				}
				else if ( ent->item->giTag == WP_MORTAR )
				{
					COM_BitSet( other->client->ps.weapons, WP_MORTAR_SET );
				}
				else if ( ent->item->giTag == WP_MOBILE_MG42 )
				{
					COM_BitSet( other->client->ps.weapons, WP_MOBILE_MG42_SET );
				}
				else if ( ent->item->giTag == WP_CARBINE )
				{
					COM_BitSet( other->client->ps.weapons, WP_M7 );
				}
				else if ( ent->item->giTag == WP_KAR98 )
				{
					COM_BitSet( other->client->ps.weapons, WP_GPG40 );
				}

				other->client->ps.ammoclip[ BG_FindClipForWeapon( ent->item->giTag ) ] = 0;
				other->client->ps.ammo[ BG_FindAmmoForWeapon( ent->item->giTag ) ] = 0;

				if ( ent->item->giTag == WP_MORTAR )
				{
					other->client->ps.ammo[ BG_FindClipForWeapon( ent->item->giTag ) ] = quantity;

					// Gordon: secondary weapon ammo
					if ( ent->delay )
					{
						Add_Ammo( other, weapAlts[ ent->item->giTag ], ent->delay, qfalse );
					}
				}
				else
				{
					other->client->ps.ammoclip[ BG_FindClipForWeapon( ent->item->giTag ) ] = quantity;

					// Gordon: secondary weapon ammo
					if ( ent->delay )
					{
						other->client->ps.ammo[ weapAlts[ ent->item->giTag ] ] = ent->delay;
					}
				}
			}
		}
		else
		{
			return 0;
		}
	}

#ifdef OMNIBOT
	Bot_Event_AddWeapon( other->client->ps.clientNum, Bot_WeaponGameToBot( ent->item->giTag ) );
#endif

	return -1;
}

//======================================================================

int Pickup_Health( gentity_t *ent, gentity_t *other )
{
	int max;

//  int         quantity = 0;

	// if medic isn't giving ammo to self or another medic or the enemy, give him some props
	if ( other->client->ps.stats[ STAT_PLAYER_CLASS ] != PC_MEDIC )
	{
		if ( ent->parent && ent->parent->client && other->client->sess.sessionTeam == ent->parent->client->sess.sessionTeam )
		{
			if ( !( ent->parent->client->PCSpecialPickedUpCount % MEDIC_SPECIAL_PICKUP_MOD ) )
			{
				AddScore( ent->parent, WOLF_HEALTH_UP );
				G_LogPrintf( "Health_Pack: %ld %ld\n", ( long )( ent->parent - g_entities ), ( long )( other - g_entities ) );    // OSP
			}

			G_AddSkillPoints( ent->parent, SK_FIRST_AID, 1.f );
			G_DebugAddSkillPoints( ent->parent, SK_FIRST_AID, 1.f, "health pack picked up" );
			ent->parent->client->PCSpecialPickedUpCount++;
		}
	}

	max = other->client->ps.stats[ STAT_MAX_HEALTH ];

	if ( other->client->sess.playerType == PC_MEDIC )
	{
		max *= 1.12f;
	}

	other->health += ent->item->quantity;

	if ( other->health > max )
	{
		other->health = max;
	}

	other->client->ps.stats[ STAT_HEALTH ] = other->health;

#ifdef OMNIBOT

	if ( ent->parent )
	{
		Bot_Event_Healed( other - g_entities, ent->parent );
	}

#endif

	return -1;
}

//======================================================================

/*
===============
RespawnItem
===============
*/
void RespawnItem( gentity_t *ent )
{
	// randomly select from teamed entities
	if ( ent->team )
	{
		gentity_t *master;
		int       count;
		int       choice;

		if ( !ent->teammaster )
		{
			G_Error( "RespawnItem: bad teammaster" );
		}

		master = ent->teammaster;

		for ( count = 0, ent = master; ent; ent = ent->teamchain, count++ )
		{
			;
		}

		choice = rand() % count;

		for ( count = 0, ent = master; count < choice; ent = ent->teamchain, count++ )
		{
			;
		}
	}

	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.eFlags &= ~EF_NODRAW;
	ent->flags &= ~FL_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity( ent );

	// play the normal respawn sound only to nearby clients
	G_AddEvent( ent, EV_ITEM_RESPAWN, 0 );

	ent->nextthink = 0;
}

/*
==============
Touch_Item
        if other->client->pers.autoActivate == PICKUP_ACTIVATE  (0), he will pick up items only when using +activate
        if other->client->pers.autoActivate == PICKUP_TOUCH   (1), he will pickup items when touched
        if other->client->pers.autoActivate == PICKUP_FORCE   (2), he will pickup the next item when touched (and reset to PICKUP_ACTIVATE when done)
==============
*/
void Touch_Item_Auto( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	if ( other->client->pers.autoActivate == PICKUP_ACTIVATE )
	{
		return;
	}

	if ( !ent->active && ent->item->giType == IT_WEAPON )
	{
		if ( ent->item->giTag != WP_AMMO )
		{
			if ( !COM_BitCheck( other->client->ps.weapons, ent->item->giTag ) )
			{
				return; // force activate only
			}
		}
	}

	ent->active = qtrue;
	Touch_Item( ent, other, trace );

	if ( other->client->pers.autoActivate == PICKUP_FORCE )
	{
		// autoactivate probably forced by the "Cmd_Activate_f()" function
		other->client->pers.autoActivate = PICKUP_ACTIVATE; // so reset it.
	}
}

/*
===============
Touch_Item
===============
*/
void Touch_Item( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	int respawn;
	int makenoise = EV_ITEM_PICKUP;

	// only activated items can be picked up
	if ( !ent->active )
	{
		return;
	}
	else
	{
		// need to set active to false if player is maxed out
		ent->active = qfalse;
	}

	if ( !other->client )
	{
		return;
	}

	if ( other->health <= 0 )
	{
		return; // dead people can't pickup
	}

	// the same pickup rules are used for client side and server side
	if ( !BG_CanItemBeGrabbed( &ent->s, &other->client->ps, other->client->sess.skill, other->client->sess.sessionTeam ) )
	{
		return;
	}

	if ( g_gamestate.integer == GS_PLAYING )
	{
		G_LogPrintf( "Item: %i %s\n", other->s.number, ent->item->classname );
	}
	else
	{
		// OSP - Don't let them pickup winning stuff in warmup
		if ( ent->item->giType != IT_WEAPON && ent->item->giType != IT_AMMO && ent->item->giType != IT_HEALTH )
		{
			return;
		}
	}

//  G_LogPrintf( "Calling item pickup function for %s\n", ent->item->classname );

	// call the item-specific pickup function
	switch ( ent->item->giType )
	{
		case IT_WEAPON:
			respawn = Pickup_Weapon( ent, other );
			break;

		case IT_HEALTH:
			respawn = Pickup_Health( ent, other );
			break;

		case IT_TEAM:
			respawn = Pickup_Team( ent, other );
			break;

		default:
			return;
	}

//  G_LogPrintf( "Finished pickup function\n" );

	if ( !respawn )
	{
		return;
	}

	// play sounds
	if ( ent->noise_index )
	{
		// (SA) a sound was specified in the entity, so play that sound
		// (this G_AddEvent) and send the pickup as "EV_ITEM_PICKUP_QUIET"
		// so it doesn't make the default pickup sound when the pickup event is recieved
		makenoise = EV_ITEM_PICKUP_QUIET;
		G_AddEvent( other, EV_GENERAL_SOUND, ent->noise_index );
	}

	G_AddEvent( other, makenoise, ent->s.modelindex );

	// powerup pickups are global broadcasts
	if ( ent->item->giType == IT_TEAM )
	{
		gentity_t *te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );

		te->s.eventParm = ent->s.modelindex;
		te->r.svFlags |= SVF_BROADCAST;
	}

//  G_LogPrintf( "Firing item targets\n" );

	// fire item targets
	G_UseTargets( ent, other );

	// dropped items will not respawn
	if ( ent->flags & FL_DROPPED_ITEM )
	{
		ent->freeAfterEvent = qtrue;
	}

	// picked up items still stay around, they just don't
	// draw anything.  This allows respawnable items
	// to be placed on movers.
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->flags |= FL_NODRAW;
	ent->r.contents = 0;

	// ZOID
	// A negative respawn times means to never respawn this item (but don't
	// delete it).  This is used by items that are respawned by third party
	// events such as ctf flags
	if ( respawn <= 0 )
	{
		ent->nextthink = 0;
		ent->think = 0;
	}
	else
	{
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
	}

	trap_LinkEntity( ent );
}

//======================================================================

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t      *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity, int ownerNum )
{
	gentity_t *dropped;
	trace_t   tr;
	vec3_t    vec, temp;
	int       i;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist; // store item number in modelindex
	dropped->s.otherEntityNum2 = 1; // DHM - Nerve :: this is taking modelindex2's place for a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet( dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, 0 );  //----(SA)  so items sit on the ground
	VectorSet( dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, 2 * ITEM_RADIUS );  //----(SA)    so items sit on the ground
	dropped->r.contents = CONTENTS_TRIGGER | CONTENTS_ITEM;

	dropped->clipmask = CONTENTS_SOLID | CONTENTS_MISSILECLIP; // NERVE - SMF - fix for items falling through grates

	dropped->touch = Touch_Item_Auto;

	trap_Trace( &tr, origin, dropped->r.mins, dropped->r.maxs, origin, ownerNum, MASK_SOLID );

	if ( tr.startsolid )
	{
		VectorSubtract( g_entities[ ownerNum ].s.origin, origin, temp );
		VectorNormalize( temp );

		for ( i = 16; i <= 48; i += 16 )
		{
			VectorScale( temp, i, vec );
			VectorAdd( origin, vec, origin );

			trap_Trace( &tr, origin, dropped->r.mins, dropped->r.maxs, origin, ownerNum, MASK_SOLID );

			if ( !tr.startsolid )
			{
				break;
			}
		}
	}

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	// ydnar: set yaw to parent angles
	temp[ PITCH ] = 0;
	temp[ YAW ] = g_entities[ ownerNum ].s.apos.trBase[ YAW ];
	temp[ ROLL ] = 0;
	G_SetAngle( dropped, temp );

	dropped->s.eFlags |= EF_BOUNCE_HALF;

	if ( item->giType == IT_TEAM )
	{
		// Special case for CTF flags
		gentity_t *flag = &g_entities[ g_entities[ ownerNum ].client->flagParent ];

		dropped->s.otherEntityNum = g_entities[ ownerNum ].client->flagParent; // store the entitynum of our original flag spawner
		dropped->s.density = 1;
		dropped->think = Team_DroppedFlagThink;
		dropped->nextthink = level.time + 30000;

		if ( level.gameManager )
		{
			G_Script_ScriptEvent( level.gameManager, "trigger",
			                      flag->item->giTag == PW_REDFLAG ? "allied_object_dropped" : "axis_object_dropped" );
		}

		G_Script_ScriptEvent( flag, "trigger", "dropped" );
	}
	else
	{
		// auto-remove after 30 seconds
		dropped->think = G_FreeEntity;

		dropped->nextthink = level.time + 30000;
	}

	dropped->flags = FL_DROPPED_ITEM;

	trap_LinkEntity( dropped );

	return dropped;
}

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t      *Drop_Item( gentity_t *ent, gitem_t *item, float angle, qboolean novelocity )
{
	vec3_t velocity;
	vec3_t angles;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[ YAW ] += angle;
	angles[ PITCH ] = 0; // always forward

	if ( novelocity )
	{
		VectorClear( velocity );
	}
	else
	{
		AngleVectors( angles, velocity, NULL, NULL );
		VectorScale( velocity, 150, velocity );
		velocity[ 2 ] += 200 + crandom() * 50;
	}

	return LaunchItem( item, ent->s.pos.trBase, velocity, ent->s.number );
}

/*
================
Use_Item

Respawn the item
================
*/
void Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	RespawnItem( ent );
}

//======================================================================

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningItem( gentity_t *ent )
{
	trace_t tr;
	vec3_t  dest;
	vec3_t  maxs;

	if ( ent->spawnflags & 1 )
	{
		// suspended
		VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
		VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );
		VectorCopy( ent->r.maxs, maxs );
	}
	else
	{
		// Rafael
		// had to modify this so that items would spawn in shelves
		VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, 0 );
		VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );
		VectorCopy( ent->r.maxs, maxs );
		maxs[ 2 ] /= 2;
	}

	ent->r.contents = CONTENTS_TRIGGER | CONTENTS_ITEM;
	ent->touch = Touch_Item_Auto;
	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist; // store item number in modelindex

	ent->s.otherEntityNum2 = 0; // DHM - Nerve :: takes modelindex2's place in signaling a dropped item

//----(SA)  we don't use this (yet, anyway) so I'm taking it so you can specify a model for treasure items and clipboards
//  ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item
	if ( ent->model )
	{
		ent->s.modelindex2 = G_ModelIndex( ent->model );
	}

//----(SA)  added
	if ( ent->item->giType == IT_TREASURE )
	{
		ent->touch = Touch_Item; // no auto-pickup, only activate
	}

//----(SA)  end

	// using an item causes it to respawn
	ent->use = Use_Item;

//----(SA) moved this up so it happens for suspended items too (and made it a function)
	G_SetAngle( ent, ent->s.angles );

	if ( ent->spawnflags & 1 )
	{
		// suspended
		G_SetOrigin( ent, ent->s.origin );
	}
	else
	{
		VectorSet( dest, ent->s.origin[ 0 ], ent->s.origin[ 1 ], ent->s.origin[ 2 ] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->r.mins, maxs, dest, ent->s.number, MASK_SOLID );

		if ( tr.startsolid )
		{
			vec3_t temp;

			VectorCopy( ent->s.origin, temp );
			temp[ 2 ] -= ITEM_RADIUS;

			VectorSet( dest, ent->s.origin[ 0 ], ent->s.origin[ 1 ], ent->s.origin[ 2 ] - 4096 );
			trap_Trace( &tr, temp, ent->r.mins, maxs, dest, ent->s.number, MASK_SOLID );
		}

#if 0
		// drop to floor
		VectorSet( dest, ent->s.origin[ 0 ], ent->s.origin[ 1 ], ent->s.origin[ 2 ] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->r.mins, maxs, dest, ent->s.number, MASK_SOLID );
#endif

		if ( tr.startsolid )
		{
			G_Printf( "FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos( ent->s.origin ) );
			G_FreeEntity( ent );
			return;
		}

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin( ent, tr.endpos );
	}

	if ( ent->spawnflags & 2 )
	{
		// spin
		ent->s.eFlags |= EF_SPINNING;
	}

	// team slaves and targeted items aren't present at start
	if ( ( ent->flags & FL_TEAMSLAVE ) || ent->targetname )
	{
		ent->flags |= FL_NODRAW;
		//ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	// health/ammo can potentially be multi-stage (multiple use)
	if ( ent->item->giType == IT_HEALTH || ent->item->giType == IT_AMMO )
	{
		int i;

		// having alternate models defined in bg_misc.c for a health or ammo item specify it as "multi-stage"
		// TTimo left-hand operand of comma expression has no effect
		// initial line: for(i=0;i<4,ent->item->world_model[i];i++) {}
		for ( i = 0; i < 4 && ent->item->world_model[ i ]; i++ )
		{
		}

		ent->s.density = i - 1; // store number of stages in 'density' for client (most will have '1')
	}

	trap_LinkEntity( ent );
}

/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnItem( gentity_t *ent, gitem_t *item )
{
	char *noise;

	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnFloat( "wait", "0", &ent->wait );

	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	if ( G_SpawnString( "noise", 0, &noise ) )
	{
		ent->noise_index = G_SoundIndex( noise );
	}

	ent->physicsBounce = 0.50; // items are bouncy

	if ( ent->model )
	{
		ent->s.modelindex2 = G_ModelIndex( ent->model );
	}

	if ( item->giType == IT_TEAM )
	{
		G_SpawnInt( "count", "1", &ent->s.density );
		G_SpawnInt( "speedscale", "100", &ent->splashDamage );

		if ( !ent->splashDamage )
		{
			ent->splashDamage = 100;
		}
	}
}

/*
================
G_BounceItem

================
*/
void G_BounceItem( gentity_t *ent, trace_t *trace )
{
	vec3_t velocity;
	float  dot;
	int    hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity, qfalse, ent->s.effect2Time );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta );

	// cut the velocity to keep from bouncing forever
	VectorScale( ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta );

	// check for stop
	if ( trace->plane.normal[ 2 ] > 0 && ent->s.pos.trDelta[ 2 ] < 40 )
	{
		trace->endpos[ 2 ] += 1.0; // make sure it is off ground
		SnapVector( trace->endpos );
		G_SetOrigin( ent, trace->endpos );
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin );
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}

/*
=================
G_RunItemProp
=================
*/

void G_RunItemProp( gentity_t *ent, vec3_t origin )
{
	gentity_t *traceEnt;
	trace_t   trace;
	gentity_t *owner;
	vec3_t    start;
	vec3_t    end;

	owner = &g_entities[ ent->r.ownerNum ];

	VectorCopy( ent->r.currentOrigin, start );
	start[ 2 ] += 1;

	VectorCopy( origin, end );
	end[ 2 ] += 1;

	trap_Trace( &trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, end, ent->r.ownerNum, MASK_SHOT );

	traceEnt = &g_entities[ trace.entityNum ];

	if ( traceEnt && traceEnt->takedamage && traceEnt != ent )
	{
		ent->enemy = traceEnt;
	}

	if ( owner->client && trace.startsolid && traceEnt != owner && traceEnt != ent /* && !traceEnt->active */ )
	{
		ent->takedamage = qfalse;
		ent->die( ent, ent, NULL, 10, 0 );
		Prop_Break_Sound( ent );

		return;
	}
	else if ( trace.surfaceFlags & SURF_NOIMPACT )
	{
		ent->takedamage = qfalse;

		Props_Chair_Skyboxtouch( ent );

		return;
	}
}

/*
================
G_RunItem

================
*/
void G_RunItem( gentity_t *ent )
{
	vec3_t  origin;
	trace_t tr;
	int     contents;
	int     mask;

	// if groundentity has been set to -1, it may have been pushed off an edge
	if ( ent->s.groundEntityNum == -1 )
	{
		if ( ent->s.pos.trType != TR_GRAVITY )
		{
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if ( ent->s.pos.trType == TR_STATIONARY || ent->s.pos.trType == TR_GRAVITY_PAUSED )
	{
		//----(SA)
		// check think function
		G_RunThink( ent );
		return;
	}

	if ( ent->s.pos.trType == TR_LINEAR && ( !ent->clipmask && !ent->r.contents ) )
	{
		// check think function
		G_RunThink( ent );
		return;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin, qfalse, ent->s.effect2Time );

	// trace a line from the previous position to the current position
	if ( ent->clipmask )
	{
		mask = ent->clipmask;
	}
	else
	{
		mask = MASK_SOLID;
	}

	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, ent->r.ownerNum, mask );

	if ( ent->isProp && ent->takedamage )
	{
		G_RunItemProp( ent, origin );
	}

	VectorCopy( tr.endpos, ent->r.currentOrigin );

	if ( tr.startsolid )
	{
		tr.fraction = 0;
	}

	trap_LinkEntity( ent );  // FIXME: avoid this for stationary?

	// check think function
	G_RunThink( ent );

	if ( tr.fraction == 1 )
	{
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = trap_PointContents( ent->r.currentOrigin, -1 );

	if ( contents & CONTENTS_NODROP )
	{
		if ( ent->item && ent->item->giType == IT_TEAM )
		{
			Team_ReturnFlag( ent );
		}
		else
		{
			G_FreeEntity( ent );
		}

		return;
	}

	G_BounceItem( ent, &tr );
}
