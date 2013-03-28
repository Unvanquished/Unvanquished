/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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

#include "g_bot.h"

qboolean WeaponIsEmpty( weapon_t weapon, playerState_t ps )
{
	if ( ps.ammo <= 0 && ps.clips <= 0 && !BG_Weapon( weapon )->infiniteAmmo )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

float PercentAmmoRemaining( weapon_t weapon, playerState_t *ps )
{
	int maxAmmo, maxClips;
	float totalMaxAmmo, totalAmmo;

	maxAmmo = BG_Weapon( weapon )->maxAmmo;
	maxClips = BG_Weapon( weapon )->maxClips;
	if ( !BG_Weapon( weapon )->infiniteAmmo )
	{
		if ( BG_InventoryContainsUpgrade( UP_BATTPACK, ps->stats ) )
		{
			maxAmmo = ( int )( ( float )maxAmmo * BATTPACK_MODIFIER );
		}

		totalMaxAmmo = ( float ) maxAmmo + maxClips * maxAmmo;
		totalAmmo = ( float ) ps->ammo + ps->clips * maxAmmo;

		return ( float ) totalAmmo / totalMaxAmmo;
	}
	else
	{
		return 1.0f;
	}
}
//Cmd_Buy_f ripoff, weapon version
void BotBuyWeapon( gentity_t *self, weapon_t weapon )
{
	if ( weapon != WP_NONE )
	{
		//already got this?
		if ( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
		{
			return;
		}

		// Only humans can buy stuff
		if ( BG_Weapon( weapon )->team != TEAM_HUMANS )
		{
			return;
		}

		//are we /allowed/ to buy this?
		if ( !BG_Weapon( weapon )->purchasable )
		{
			return;
		}

		//are we /allowed/ to buy this?
		if ( !BG_WeaponAllowedInStage( weapon, ( stage_t )g_humanStage.integer ) || !BG_WeaponIsAllowed( weapon ) )
		{
			return;
		}

		//can afford this?
		if ( BG_Weapon( weapon )->price > ( short )self->client->pers.credit )
		{
			return;
		}

		//have space to carry this?
		if ( BG_Weapon( weapon )->slots & BG_SlotsForInventory( self->client->ps.stats ) )
		{
			return;
		}

		// In some instances, weapons can't be changed
		if ( !BG_PlayerCanChangeWeapon( &self->client->ps ) )
		{
			return;
		}

		self->client->ps.stats[ STAT_WEAPON ] = weapon;
		self->client->ps.ammo = BG_Weapon( weapon )->maxAmmo;
		self->client->ps.clips = BG_Weapon( weapon )->maxClips;

		if ( BG_Weapon( weapon )->usesEnergy &&
		        BG_InventoryContainsUpgrade( UP_BATTPACK, self->client->ps.stats ) )
		{
			self->client->ps.ammo *= BATTPACK_MODIFIER;
		}

		G_ForceWeaponChange( self, weapon );

		//set build delay/pounce etc to 0
		self->client->ps.stats[ STAT_MISC ] = 0;

		//subtract from funds
		G_AddCreditToClient( self->client, -( short )BG_Weapon( weapon )->price, qfalse );
	}
	else
	{
		return;
	}
	//update ClientInfo
	ClientUserinfoChanged( self->client->ps.clientNum, qfalse );
}
void BotBuyUpgrade( gentity_t *self, upgrade_t upgrade )
{
	qboolean  energyOnly = qfalse;

	// Only give energy from reactors or repeaters
	if ( upgrade == UP_AMMO &&
	        BG_Weapon( ( weapon_t )self->client->ps.stats[ STAT_WEAPON ] )->usesEnergy &&
	        ( G_BuildableRange( self->client->ps.origin, 100, BA_H_REACTOR ) ||
	          G_BuildableRange( self->client->ps.origin, 100, BA_H_REPEATER ) ) )
	{
		energyOnly = qtrue;
	}
	else if ( G_BuildableRange( self->client->ps.origin, 100, BA_H_ARMOURY ) )
	{
		energyOnly = qfalse;
	}
	else if ( upgrade == UP_AMMO && BG_Weapon( ( weapon_t )self->client->ps.weapon )->usesEnergy )
	{
		return;
	}

	if ( upgrade != UP_NONE )
	{
		//already got this?
		if ( BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ) )
		{
			return;
		}

		//can afford this?
		if ( BG_Upgrade( upgrade )->price > ( short )self->client->pers.credit )
		{
			return;
		}

		//have space to carry this?
		if ( BG_Upgrade( upgrade )->slots & BG_SlotsForInventory( self->client->ps.stats ) )
		{
			return;
		}

		// Only humans can buy stuff
		if ( BG_Upgrade( upgrade )->team != TEAM_HUMANS )
		{
			return;
		}

		//are we /allowed/ to buy this?
		if ( !BG_Upgrade( upgrade )->purchasable )
		{
			return;
		}

		//are we /allowed/ to buy this?
		if ( !BG_UpgradeAllowedInStage( upgrade, ( stage_t )g_humanStage.integer ) || !BG_UpgradeIsAllowed( upgrade ) )
		{
			return;
		}

		if ( upgrade == UP_AMMO )
		{
			G_GiveClientMaxAmmo( self, energyOnly );
		}
		else
		{
			if ( upgrade == UP_BATTLESUIT )
			{
				vec3_t newOrigin;

				if ( !G_RoomForClassChange( self, PCL_HUMAN_BSUIT, newOrigin ) )
				{
					return;
				}
				VectorCopy( newOrigin, self->client->ps.origin );
				self->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN_BSUIT;
				self->client->pers.classSelection = PCL_HUMAN_BSUIT;
				BotSetNavmesh( self, PCL_HUMAN_BSUIT );
				self->client->ps.eFlags ^= EF_TELEPORT_BIT;
			}

			//add to inventory
			BG_AddUpgradeToInventory( upgrade, self->client->ps.stats );
		}

		if ( upgrade == UP_BATTPACK )
		{
			G_GiveClientMaxAmmo( self, qtrue );
		}

		//subtract from funds
		G_AddCreditToClient( self->client, -( short )BG_Upgrade( upgrade )->price, qfalse );
	}
	else
	{
		return;
	}

	//update ClientInfo
	ClientUserinfoChanged( self->client->ps.clientNum, qfalse );
}
void BotSellWeapons( gentity_t *self )
{
	weapon_t selected = BG_GetPlayerWeapon( &self->client->ps );
	int i;

	//no armoury nearby
	if ( !G_BuildableRange( self->client->ps.origin, 100, BA_H_ARMOURY ) )
	{
		return;
	}

	if ( !BG_PlayerCanChangeWeapon( &self->client->ps ) )
	{
		return;
	}

	//sell weapons
	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		//guard against selling the HBUILD weapons exploit
		if ( i == WP_HBUILD && self->client->ps.stats[ STAT_MISC ] > 0 )
		{
			continue;
		}

		if ( BG_InventoryContainsWeapon( i, self->client->ps.stats ) &&
		        BG_Weapon( ( weapon_t )i )->purchasable )
		{
			self->client->ps.stats[ STAT_WEAPON ] = WP_NONE;

			//add to funds
			G_AddCreditToClient( self->client, ( short )BG_Weapon( ( weapon_t ) i )->price, qfalse );
		}

		//if we have this weapon selected, force a new selection
		if ( i == selected )
		{
			G_ForceWeaponChange( self, WP_NONE );
		}
	}
}
void BotSellAll( gentity_t *self )
{
	int i;

	//no armoury nearby
	if ( !G_BuildableRange( self->client->ps.origin, 100, BA_H_ARMOURY ) )
	{
		return;
	}
	BotSellWeapons( self );

	//sell upgrades
	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		//remove upgrade if carried
		if ( BG_InventoryContainsUpgrade( i, self->client->ps.stats ) &&
		        BG_Upgrade( ( upgrade_t )i )->purchasable )
		{

			// shouldn't really need to test for this, but just to be safe
			if ( i == UP_BATTLESUIT )
			{
				vec3_t newOrigin;

				if ( !G_RoomForClassChange( self, PCL_HUMAN, newOrigin ) )
				{
					continue;
				}
				VectorCopy( newOrigin, self->client->ps.origin );
				self->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN;
				self->client->pers.classSelection = PCL_HUMAN;
				self->client->ps.eFlags ^= EF_TELEPORT_BIT;
				BotSetNavmesh( self, PCL_HUMAN );
			}

			BG_RemoveUpgradeFromInventory( i, self->client->ps.stats );

			if ( i == UP_BATTPACK )
			{
				G_GiveClientMaxAmmo( self, qtrue );
			}

			//add to funds
			G_AddCreditToClient( self->client, ( short )BG_Upgrade( ( upgrade_t )i )->price, qfalse );
		}
	}
	//update ClientInfo
	ClientUserinfoChanged( self->client->ps.clientNum, qfalse );
}
int BotValueOfWeapons( gentity_t *self )
{
	int worth = 0;
	int i;

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if ( BG_InventoryContainsWeapon( i, self->client->ps.stats ) )
		{
			worth += BG_Weapon( ( weapon_t )i )->price;
		}
	}
	return worth;
}
int BotValueOfUpgrades( gentity_t *self )
{
	int worth = 0;
	int i;

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( BG_InventoryContainsUpgrade( i, self->client->ps.stats ) )
		{
			worth += BG_Upgrade( ( upgrade_t ) i )->price;
		}
	}
	return worth;
}
void BotGetDesiredBuy( gentity_t *self, weapon_t *weapon, upgrade_t *upgrades, int *numUpgrades )
{
	int i;
	int equipmentPrice = BotValueOfWeapons( self ) + BotValueOfUpgrades( self );
	int credits = self->client->ps.persistant[PERS_CREDIT];
	int usableCapital = credits + equipmentPrice;

	//decide what upgrade(s) to buy
	if ( g_humanStage.integer >= 2 && usableCapital >= ( BG_Weapon( WP_PAIN_SAW )->price + BG_Upgrade( UP_BATTLESUIT )->price ) )
	{
		upgrades[0] = UP_BATTLESUIT;
		*numUpgrades = 1;
	}
	else if ( g_humanStage.integer >= 1 && usableCapital >= ( BG_Weapon( WP_SHOTGUN )->price + BG_Upgrade( UP_LIGHTARMOUR )->price + BG_Upgrade( UP_HELMET )->price ) )
	{
		upgrades[0] = UP_LIGHTARMOUR;
		upgrades[1] = UP_HELMET;
		*numUpgrades = 2;
	}
	else if ( g_humanStage.integer >= 0 && usableCapital >= ( BG_Weapon( WP_PAIN_SAW )->price + BG_Upgrade( UP_LIGHTARMOUR )->price ) )
	{
		upgrades[0] = UP_LIGHTARMOUR;
		*numUpgrades = 1;
	}
	else
	{
		*numUpgrades = 0;
	}

	for (i = 0; i < *numUpgrades; i++)
	{
		usableCapital -= BG_Upgrade( upgrades[i] )->price;
	}

	//now decide what weapon to buy
	if ( g_humanStage.integer >= 2  && usableCapital >= BG_Weapon( WP_LUCIFER_CANNON )->price && g_bot_lcannon.integer )
	{
		*weapon = WP_LUCIFER_CANNON;;
	}
	else if ( g_humanStage.integer >= 2 && usableCapital >= BG_Weapon( WP_CHAINGUN )->price && upgrades[0] == UP_BATTLESUIT && g_bot_chaingun.integer )
	{
		*weapon = WP_CHAINGUN;
	}
	else if ( g_humanStage.integer >= 1 && g_alienStage.integer < 2 && usableCapital >= BG_Weapon( WP_FLAMER )->price && g_bot_flamer.integer )
	{
		*weapon = WP_FLAMER;
	}
	else if ( g_humanStage.integer >= 1 && usableCapital >= BG_Weapon( WP_PULSE_RIFLE )->price && g_bot_prifle.integer )
	{
		*weapon = WP_PULSE_RIFLE;
	}
	else if ( g_humanStage.integer >= 0 && usableCapital >= BG_Weapon( WP_CHAINGUN )->price && g_bot_chaingun.integer )
	{
		*weapon = WP_CHAINGUN;;
	}
	else if ( g_humanStage.integer >= 0 && usableCapital >= BG_Weapon( WP_MASS_DRIVER )->price && g_bot_mdriver.integer )
	{
		*weapon = WP_MASS_DRIVER;
	}
	else if ( g_humanStage.integer >= 0 && usableCapital >= BG_Weapon( WP_LAS_GUN )->price && g_bot_lasgun.integer )
	{
		*weapon = WP_LAS_GUN;
	}
	else if ( g_humanStage.integer >= 0 && usableCapital >= BG_Weapon( WP_SHOTGUN )->price && g_bot_shotgun.integer )
	{
		*weapon = WP_SHOTGUN;
	}
	else if ( g_humanStage.integer >= 0 && usableCapital >= BG_Weapon( WP_PAIN_SAW )->price && g_bot_painsaw.integer )
	{
		*weapon = WP_PAIN_SAW;
	}
	else
	{
		*weapon = WP_MACHINEGUN;
	}

	usableCapital -= BG_Weapon( *weapon )->price;

	//finally, see if we can buy a battpack
	if ( BG_Weapon( *weapon )->usesEnergy && usableCapital >= BATTPACK_PRICE && g_humanStage.integer >= 1 && upgrades[0] != UP_BATTLESUIT )
	{
		upgrades[( *numUpgrades )++] = UP_BATTPACK;
		usableCapital -= BATTPACK_PRICE;
	}

	//now test to see if we already have all of these items
	//check if we already have everything
	if ( BG_InventoryContainsWeapon( ( int )*weapon, self->client->ps.stats ) )
	{
		int numContain = 0;
		int i;

		for ( i = 0; i < *numUpgrades; i++ )
		{
			if ( BG_InventoryContainsUpgrade( ( int )upgrades[i], self->client->ps.stats ) )
			{
				numContain++;
			}
		}
		//we have every upgrade we want to buy, and the weapon we want to buy, so test if we need ammo
		if ( numContain == *numUpgrades )
		{
			*numUpgrades = 0;
			for ( i = 0; i < 3; i++ )
			{
				upgrades[i] = UP_NONE;
			}
			if ( PercentAmmoRemaining( BG_PrimaryWeapon( self->client->ps.stats ), &self->client->ps ) < BOT_LOW_AMMO )
			{
				upgrades[0] = UP_AMMO;
				*numUpgrades = 1;
			}
			*weapon = WP_NONE;
		}
	}
}
int BotFindBuildingNotBuiltByBot( gentity_t *self, int buildingType )
{

	float minDistance = -1;
	int closestBuilding = ENTITYNUM_NONE;
	float newDistance;
	gentity_t *target;
	int i;

	for ( i = MAX_CLIENTS; i < level.num_entities; ++i )
	{
		target = &g_entities[i];
		if ( target->s.eType == ET_BUILDABLE )
		{
			if ( target->builtBy && target->builtBy->slot >= 0 &&
			        g_entities[target->builtBy->slot].botMind )
			{
				continue;
			}
			if ( target->s.modelindex == buildingType && ( target->buildableTeam == TEAM_ALIENS || ( target->powered && target->spawned ) ) && target->health > 0 )
			{
				newDistance = DistanceSquared( self->s.origin, target->s.origin );
				if ( newDistance < minDistance || minDistance == -1 )
				{
					minDistance = newDistance;
					closestBuilding = i;
				}
			}
		}
	}
	return closestBuilding;
}
//finds buildings powered by a specified building in the list
//also filters out any buildings built by bots
int BotFindBuildingsPoweredBy( gentity_t *powerEntity, gentity_t **buildings, int maxBuildings )
{
	int numBuildings = 0;
	int i;

	memset( buildings, 0, sizeof( gentity_t* )*maxBuildings );
	for ( i = MAX_CLIENTS; i < level.num_entities; i++ )
	{
		gentity_t *building = &g_entities[i];
		if ( building->s.eType != ET_BUILDABLE )
		{
			continue;
		}
		if ( building->buildableTeam != powerEntity->buildableTeam )
		{
			continue;
		}
		if ( building == powerEntity )
		{
			continue;
		}
		if ( building->builtBy && building->builtBy->slot >= 0 &&
		        g_entities[building->builtBy->slot].botMind )
		{
			continue;
		}
		if ( numBuildings < maxBuildings )
		{
			buildings[numBuildings++] = building;
		}
	}
	return numBuildings;
}
int BotFindBuildingInList( gentity_t **buildings, int numBuildings, int buildingType )
{
	int i;

	for ( i = 0; i < numBuildings; i++ )
	{
		gentity_t *building = buildings[i];

		if ( building->s.modelindex == buildingType )

		{
			return building->s.number;
		}
	}
	return ENTITYNUM_NONE;
}

int BotFindBestHDecon( gentity_t *self, buildable_t building, vec3_t origin )
{

	gentity_t* buildings[100];
	gentity_t *powerEntity;
	int numBuildings;
	int bestBuild;

	if ( building == BA_H_REACTOR )
	{
		return G_Reactor()->s.number;
	}

	powerEntity = G_PowerEntityForPoint( origin );
	numBuildings = BotFindBuildingsPoweredBy( powerEntity, buildings, 100 );

	//there is nothing providing power here, nothing to decon
	if ( powerEntity == NULL )
	{
		return ENTITYNUM_NONE;
	}

	bestBuild = BotFindBuildingInList( buildings, numBuildings, BA_H_MGTURRET );
	if ( bestBuild == ENTITYNUM_NONE )
	{
		bestBuild = BotFindBuildingInList( buildings, numBuildings, BA_H_TESLAGEN );
		if ( bestBuild == ENTITYNUM_NONE )
		{
			bestBuild = BotFindBuildingInList( buildings, numBuildings, BA_H_DCC );
			if ( bestBuild == ENTITYNUM_NONE )
			{
				bestBuild = BotFindBuildingInList( buildings, numBuildings, BA_H_MEDISTAT );
				if ( bestBuild == ENTITYNUM_NONE )
				{
					bestBuild = BotFindBuildingInList( buildings, numBuildings, BA_H_ARMOURY );
					if ( bestBuild == ENTITYNUM_NONE )
					{
						bestBuild = BotFindBuildingInList( buildings, numBuildings, BA_H_SPAWN );
					}
				}
			}
		}
	}

	return bestBuild;
}
qboolean BotGetBuildingToBuild( gentity_t *self, vec3_t origin, vec3_t normal, buildable_t *building )
{
	int i;

	if ( !level.botBuildLayout.numBuildings )
	{
		return qfalse;
	}
	//NOTE: Until alien building is (re)implemented, the only buildings in level.botLayout are ones on the human team

	//check all buildings in the current bot layout and see if we need to build them
	for ( i = 0; i < level.botBuildLayout.numBuildings; i++ )
	{
		int block = 0;
		int num;
		VectorCopy( level.botBuildLayout.buildings[i].origin, origin );
		VectorCopy( level.botBuildLayout.buildings[i].normal, normal );
		*building = level.botBuildLayout.buildings[i].type;

		//make sure we have a reactor
		if ( !G_Reactor() && *building != BA_H_REACTOR )
		{
			continue;
		}

		//cant build some stuff if we arnt in the right stage
		if ( !BG_BuildableAllowedInStage( *building, ( stage_t ) g_humanStage.integer ) )
		{
			continue;
		}

		//check if we have enough buildpoints in a location to build something
		if ( G_GetBuildPointsInt( BotGetEntityTeam( self ) ) < BG_Buildable( *building )->buildPoints )
		{
			//not enough build points, check if there is something we can decon to make room
			if ( BotFindBestHDecon( self, *building, origin ) == ENTITYNUM_NONE )
			{
				continue;
			}
		}

		num = trap_EntitiesInBox( origin, origin, &block, 1 );
		if ( num == 0 || g_entities[block].s.modelindex != *building )
		{
			return qtrue;
		}
	}
	return qfalse;
}

AINodeStatus_t BotActionBuy( gentity_t *self, AIGenericNode_t *node )
{
	AIBuyNode_t *buy = ( AIBuyNode_t * ) node;

	weapon_t weapon = buy->weapon;
	upgrade_t *upgrades = buy->upgrades;
	int numUpgrades = buy->numUpgrades;
	int i;

	if ( weapon == WP_NONE && numUpgrades == 0 )
	{
		BotGetDesiredBuy( self, &weapon, upgrades, &numUpgrades );
	}

	if ( !g_bot_buy.integer )
	{
		return STATUS_FAILURE;
	}
	if ( BotGetEntityTeam( self ) != TEAM_HUMANS )
	{
		return STATUS_FAILURE;
	}
	if ( numUpgrades && upgrades[0] == UP_AMMO && BG_Weapon( (weapon_t)self->client->ps.stats[ STAT_WEAPON ] )->usesEnergy )
	{
		if ( !self->botMind->closestBuildings[ BA_H_ARMOURY ].ent &&
		     !self->botMind->closestBuildings[ BA_H_REPEATER ].ent &&
		     !self->botMind->closestBuildings[ BA_H_REACTOR ].ent )
		{
			return STATUS_FAILURE; //wanted ammo for energy? no armoury, repeater or reactor, so fail
		}
	}
	else if ( !self->botMind->closestBuildings[BA_H_ARMOURY].ent )
	{
		return STATUS_FAILURE;    //no armoury, so fail
	}

	//check if we already have everything
	if ( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) || weapon == WP_NONE )
	{
		int numContain = 0;

		//cant buy more than 3 upgrades
		if ( numUpgrades > 3 )
		{
			return STATUS_FAILURE;
		}

		if ( numUpgrades == 0 )
		{
			return STATUS_FAILURE;
		}

		for ( i = 0; i < numUpgrades; i++ )
		{
			if ( BG_InventoryContainsUpgrade( upgrades[i], self->client->ps.stats ) )
			{
				numContain++;
			}
		}
		//we have every upgrade we want to buy
		if ( numContain == numUpgrades )
		{
			return STATUS_FAILURE;
		}
	}

	if ( self->botMind->currentNode != node )
	{
		//default to armoury
		const botEntityAndDistance_t *ent = &self->botMind->closestBuildings[ BA_H_ARMOURY ];

		//wanting energy ammo only? look for closer repeater or reactor
		if ( numUpgrades && upgrades[0] == UP_AMMO && BG_Weapon( (weapon_t)self->client->ps.stats[ STAT_WEAPON ] )->usesEnergy)
		{
#define DISTANCE(obj) ( self->botMind->closestBuildings[ obj ].ent ? self->botMind->closestBuildings[ obj ].distance : INT_MAX )
			//repeater closest? use that
			if ( DISTANCE( BA_H_REPEATER ) < DISTANCE( BA_H_REACTOR ) )
			{
				if ( DISTANCE( BA_H_REPEATER ) < DISTANCE( BA_H_ARMOURY ) )
				{
					ent = &self->botMind->closestBuildings[ BA_H_REPEATER ];
				}
			}
			//reactor closest? use that
			else if ( DISTANCE( BA_H_REACTOR ) < DISTANCE( BA_H_ARMOURY ) )
			{
				ent = &self->botMind->closestBuildings[ BA_H_REACTOR ];
			}
		}
		if ( !BotChangeGoalEntity( self, ent->ent ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}
	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	if ( DistanceToGoalSquared( self ) > 100 * 100 )
	{
		BotMoveToGoal( self );
		return STATUS_RUNNING;
	}
	else
	{

		if ( numUpgrades && upgrades[0] != UP_AMMO )
		{
			BotSellAll( self );
		}
		else if ( weapon != WP_NONE )
		{
			BotSellWeapons( self );
		}

		BotBuyWeapon( self, weapon );

		for ( i = 0; i < numUpgrades; i++ )
		{
			BotBuyUpgrade( self, upgrades[i] );
		}

		// make sure that we're not using the blaster
		G_ForceWeaponChange( self, weapon );
		
		return STATUS_SUCCESS;
	}
}

AINodeStatus_t BotActionHealH( gentity_t *self, AIGenericNode_t *node )
{
	vec3_t targetPos;
	vec3_t myPos;

	if ( self->client->ps.stats[STAT_TEAM] != TEAM_HUMANS )
	{
		return STATUS_FAILURE;
	}

	//we are fully healed
	if ( BG_Class( ( class_t )self->client->ps.stats[STAT_CLASS] )->health <= self->health && BG_InventoryContainsUpgrade( UP_MEDKIT, self->client->ps.stats ) )
	{
		return STATUS_SUCCESS;
	}

	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeGoalEntity( self, self->botMind->closestBuildings[ BA_H_MEDISTAT ].ent ) )
		{
			return STATUS_FAILURE;
		}
	}

	//safety check
	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	//the medi has died so signal that the goal is unusable
	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	//this medi is no longer powered so signal that the goal is unusable
	if ( !self->botMind->goal.ent->powered )
	{
		return STATUS_FAILURE;
	}

	BotGetTargetPos( self->botMind->goal, targetPos );
	VectorCopy( self->s.origin, myPos );
	targetPos[2] += BG_BuildableModelConfig( BA_H_MEDISTAT )->maxs[2];
	myPos[2] += self->r.mins[2]; //mins is negative

	//keep moving to the medi until we are on top of it
	if ( DistanceSquared( myPos, targetPos ) > Square( BG_BuildableModelConfig( BA_H_MEDISTAT )->mins[1] ) )
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}
AINodeStatus_t BotActionRepair( gentity_t *self, AIGenericNode_t *node )
{
	vec3_t forward;
	vec3_t targetPos;
	vec3_t selfPos;

	const buildableAttributes_t *buildable;

	if ( node != self->botMind->currentNode )
	{
		if ( !BotChangeGoalEntity( self, self->botMind->closestDamagedBuilding.ent ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	buildable = BG_Buildable( ( buildable_t )self->botMind->goal.ent->s.modelindex );

	if ( !G_CanAffordBuildPoints( TEAM_HUMANS, G_GetRepairCostForBuilding( buildable ) ) )
	{
		return STATUS_FAILURE;
	}

	if ( self->botMind->goal.ent->health >= buildable->health )
	{
		return STATUS_SUCCESS;
	}

	if ( self->client->ps.weapon != WP_HBUILD )
	{
		G_ForceWeaponChange( self, WP_HBUILD );
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	BotGetTargetPos( self->botMind->goal, targetPos );
	VectorMA( self->s.origin, self->r.maxs[1], forward, selfPos );

	//move to the damaged building until we are in range
	if ( !BotTargetIsVisible( self, self->botMind->goal, MASK_SHOT ) || DistanceToGoalSquared( self ) > Square( 100 ) )
	{
		BotMoveToGoal( self );
	}
	else
	{
		//aim at the buildable
		BotSlowAim( self, targetPos, 0.5 );
		BotAimAtLocation( self, targetPos );
		// we automatically heal a building if close enough and aiming at it
	}
	return STATUS_RUNNING;
}

gentity_t* BotFindDamagedFriendlyStructure( gentity_t *self )
{
	//closest building
	gentity_t* closestBuilding = NULL;

	//minimum distance found
	float minDistance = Square( ALIENSENSE_RANGE );

	gentity_t *target;

	for ( target = &g_entities[MAX_CLIENTS]; target < &g_entities[level.num_entities - 1]; target++ )
	{
		float distance;
		const buildableAttributes_t *buildable;

		if ( target->s.eType != ET_BUILDABLE )
		{
			continue;
		}
		if ( target->buildableTeam != TEAM_HUMANS )
		{
			continue;
		}
		buildable = BG_Buildable( ( buildable_t )target->s.modelindex );
		if ( target->health >= buildable->health )
		{
			continue;
		}
		if ( target->health <= 0 )
		{
			continue;
		}
		if ( !target->spawned || !target->powered )
		{
			continue;
		}
		if ( !G_CanAffordBuildPoints( TEAM_HUMANS, G_GetRepairCostForBuilding( buildable ) ) )
		{
			continue;
		}
		distance = DistanceSquared( self->s.origin, target->s.origin );
		if ( distance <= minDistance )
		{
			minDistance = distance;
			closestBuilding = target;
		}
	}
	return closestBuilding;
}


