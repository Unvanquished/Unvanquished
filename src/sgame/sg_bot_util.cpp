/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.

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

#include "sg_bot_ai.h"
#include "sg_bot_util.h"
#include "botlib/bot_api.h"
#include "CBSE.h"
#include "shared/bg_local.h" // MIN_WALK_NORMAL
#include "Entities.h"

#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

static void ListTeamEquipment( gentity_t *self, unsigned int (&numUpgrades)[UP_NUM_UPGRADES], unsigned int (&numWeapons)[WP_NUM_WEAPONS] );
static const int MIN_SKILL = 1;
static const int MAX_SKILL = 9;
static const int RANGE_SKILL = MAX_SKILL - MIN_SKILL;

// computes a percent modifier out of skill level which is easier to work with
// between 0 and 1
static float SkillModifier( int botSkill )
{
       return static_cast<float>( botSkill - MIN_SKILL ) / RANGE_SKILL;
}

static float GetMaximalSpeed( gentity_t const *self );
static float GetMaximalSpeed( class_t cl );

// helper function to make code more readable
static float GetMaximalSpeed( gentity_t const *self )
{
	return GetMaximalSpeed( static_cast<class_t>( self->client->ps.stats[ STAT_CLASS ] ) );
}

// returns maximum "ground speed" of a class
// It should likely be replaced by the real formula, but I don't know
// it, and the results are good enough (tm).
static float GetMaximalSpeed( class_t cl )
{
	return BG_Class( cl )->speed * BG_Class( cl )->sprintMod * g_speed.Get();
}

template <typename T>
struct equipment_t
{
	Cvar::Cvar<bool>& authorized;
	T item;
	int price( void ) const;
	bool unlocked( void ) const;
	bool allowed( void ) const;
	bool canBuyNow( void ) const
	{
		return allowed() && ( g_bot_infiniteMomentum.Get() || unlocked() );
	}
	int slots( void ) const;
	bool operator==( T other ) const
	{
		return item == other;
	}
	bool operator!=( T other ) const
	{
		return item != other;
	}
};

/// let's write some duplicated code because of nice API
/// start sillyness {

// price
template <>
int equipment_t<class_t>::price( void ) const
{
	return BG_Class( item )->price;
}

template <>
int equipment_t<upgrade_t>::price( void ) const
{
	return BG_Upgrade( item )->price;
}

template <>
int equipment_t<weapon_t>::price( void ) const
{
	return BG_Weapon( item )->price;
}

// unlocked
template <>
bool equipment_t<class_t>::unlocked( void ) const
{
	return BG_ClassUnlocked( item );
}

template <>
bool equipment_t<upgrade_t>::unlocked( void ) const
{
	return BG_UpgradeUnlocked( item );
}

template <>
bool equipment_t<weapon_t>::unlocked( void ) const
{
	return BG_WeaponUnlocked( item );
}

// allowed
template <>
bool equipment_t<class_t>::allowed( void ) const
{
// HACK: only aliens can "buy" classes, aka evolve, so this should work, for now.
	return authorized.Get() && g_bot_evolve.Get() && !BG_ClassDisabled( item );
}

template <>
bool equipment_t<upgrade_t>::allowed( void ) const
{
// HACK: only humans can buy equipment, so this should be fine for now.
	return authorized.Get() && g_bot_buy.Get() && !BG_UpgradeDisabled( item );
}

template <>
bool equipment_t<weapon_t>::allowed( void ) const
{
// HACK: only humans can buy equipment, so this should be fine for now.
	return authorized.Get() && g_bot_buy.Get() && !BG_WeaponDisabled( item );
}

// slots
template <>
int equipment_t<class_t>::slots( void ) const
{
	return 0; //classes do not really take slots
}

template <>
int equipment_t<upgrade_t>::slots( void ) const
{
	return BG_Upgrade( item )->slots;
}

template <>
int equipment_t<weapon_t>::slots( void ) const
{
	return BG_Weapon( item )->slots;
}

/// } end of sillyness

// manually sorted by preference, hopefully a future patch will have a much smarter way
equipment_t<class_t> classes[] =
{
	{ g_bot_level4    , PCL_ALIEN_LEVEL4       },
	{ g_bot_level3upg , PCL_ALIEN_LEVEL3_UPG   },
	{ g_bot_level3    , PCL_ALIEN_LEVEL3       },
	{ g_bot_level2upg , PCL_ALIEN_LEVEL2_UPG   },
	{ g_bot_level2    , PCL_ALIEN_LEVEL2       },
	{ g_bot_level1    , PCL_ALIEN_LEVEL1       },
	{ g_bot_level0    , PCL_ALIEN_LEVEL0       },
	{ g_bot_builderupg, PCL_ALIEN_BUILDER0_UPG },
	{ g_bot_builder   , PCL_ALIEN_BUILDER0     },
};

// manually sorted by preference, hopefully a future patch will have a much smarter way to select weapon
equipment_t<upgrade_t> armors[] =
{
	{ g_bot_battlesuit  , UP_BATTLESUIT },
	{ g_bot_mediumarmour, UP_MEDIUMARMOUR },
	{ g_bot_lightarmour , UP_LIGHTARMOUR },
};
// not merged because they are not armors
equipment_t<upgrade_t> others[] =
{
	{ g_bot_radar   , UP_RADAR },
	{ g_bot_grenade , UP_GRENADE },
	{ g_bot_firebomb, UP_FIREBOMB },
};

// manually sorted by preference, hopefully a future patch will have a much smarter way to select weapon
equipment_t<weapon_t> weapons[] =
{
	{ g_bot_lcannon , WP_LUCIFER_CANNON },
	{ g_bot_flamer  , WP_FLAMER },
	// pulse rifle has lower priority to keep previous "correct" behavior
	{ g_bot_prifle  , WP_PULSE_RIFLE },
	{ g_bot_chaingun, WP_CHAINGUN },
	{ g_bot_mdriver , WP_MASS_DRIVER },
	{ g_bot_lasgun  , WP_LAS_GUN },
	{ g_bot_shotgun , WP_SHOTGUN },
	{ g_bot_painsaw , WP_PAIN_SAW },
	{ g_bot_rifle   , WP_MACHINEGUN },
	{ g_bot_ckit    , WP_HBUILD },
};

/*
=======================
Scoring functions for logic
=======================
*/

// computes the maximum credits this bot could spend in
// equipment (or classes) with infinite credits.
static int GetMaxEquipmentCost( gentity_t const* self )
{
	int max_value = 0;
	switch( G_Team( self ) )
	{
		case TEAM_ALIENS:
			for ( auto const& pcl : classes )
			{
				if( pcl.canBuyNow() )
				{
					max_value = std::max( max_value, pcl.price() );
				}
			}
			break;
		case TEAM_HUMANS:
			// all this code, just to know what the higher equipment cost
			// authorized at current stage.
			{
				bool canUseBackpack = true;
				int max_item_val;

				max_item_val = 0;
				for ( auto const &armor : armors )
				{
					// this is very hackish and should be improved, like, a lot.
					// in short: add the value of possible backpacks if armor is
					// not battlesuit.
					// This will break when more equipments will use multiple slots,
					// but the game does not (yet) have easy way to handle those.
					if ( armor.canBuyNow() && max_item_val < armor.price() )
					{
						max_item_val = armor.price();
						canUseBackpack = armor.item != UP_BATTLESUIT;
					}
				}

				if( canUseBackpack && g_bot_radar.Get() && BG_UpgradeUnlocked( UP_RADAR ) )
				{
					max_item_val += BG_Upgrade( UP_RADAR )->price;
				}
				max_value += max_item_val;

				max_item_val = 0;
				for ( auto const& wp : weapons )
				{
					if( wp.canBuyNow() )
					{
						max_item_val = std::max( max_item_val, wp.price() );
					}
				}
				max_value += max_item_val;
				break;
			}
		default:
			ASSERT_UNREACHABLE();
	}
	return max_value;
}

// Gives a value between 0 and 1 representing how much a bot should want to rush.
// A rush is basically: target enemy's base.
// The idea is to have bots rushing depending on the value of their equipment,
// their skill level and what they are currently authorized to buy.
// Basically, higher skilled bots should save money before rushing, so that they
// would not be naked at their death.
// In current state of code, human bots no longer wait for battlesuit to attack,
// but alien bots are still rushing too much, probably because of their tracking
// ability and "speed".
// Those problems can probably *not* be fixed in this place, though.
// TODO: have a way to increase rush score depedning on much of mates are rushing
//       I suppose I'll have to need a team_t struct, which would contain some
//       modifier, itself reconstructed each "frame", increased depending on
//       team's average credits / player?
// TODO: compare both team's momentums to know if rushing is wise?
// TODO: check number of spawns, if less than 2, apply big score reduction?
float BotGetBaseRushScore( gentity_t *ent )
{
	ASSERT( ent && ent->botMind && ent->client );
	const float skill_modifier = 1 - 2 * SkillModifier( ent->botMind->botSkill.level );
	// if nothing allowed from current stage have a cost,
	// return average value on which other parameters can weight
	float rush_score = 0.5;

	float self_value = static_cast<float>(
		ent->client->ps.persistant[PERS_CREDIT]
			+ BG_GetPlayerPrice( ent->client->ps ) );

	int max_value = GetMaxEquipmentCost( ent );
	if ( max_value != 0 )
	{
		rush_score = ( self_value + skill_modifier * max_value )/ max_value;
		rush_score = Math::Clamp( rush_score, 0.f, 1.f );
	}
	return rush_score;
}

// Calculates how important a bot should consider finding a heal source
// returns a number between 0 and 1, 1 being "I need to heal"
// The idea is to make bots more likely to do "opportunistic healing",
// that is, if they have taken only low damages but a heal source is
// at a distance which can be reached very quickly, replenish health.
// But if heal source is too far, don't even try, even if almost dead,
// because it would require more time to go to healer, wait for heal,
// go back into action.
// Also, the more damaged you are, the more vulnerable you are when
// going back to heal, thus just finish what you were up to.
//
// Note that, as for anything involving distances to take decisions in
// game, it's broken because distances are from point A to point B:
// they are "ghost fly distances" (even faster than birds).
//
// The "amplification factor" of 5 have no special meaning, and was
// only choosen because it "looks good" on the curve.
float BotGetHealScore( gentity_t *self )
{
	float percentHealth = Entities::HealthFraction(self);
	if ( percentHealth == 1.0f )
	{
		return 0.0f;
	}

	float distToHealer = 0;

	if ( self->client->pers.team == TEAM_ALIENS )
	{
		//Alien code is notoriously bugged if the heal source can not be reached,
		//which includes sources on walls and ceilings.
		//This way of doing code more or less works, because boosters have rather
		//big area (but aliens won't take poison) and players usually put them in
		//places they can reach without too much hassle.
		//The bug can be noted if overmind is removed and closest egg is high enough.
		if ( self->botMind->closestBuildings[ BA_A_BOOSTER ].ent )
		{
			distToHealer = self->botMind->closestBuildings[ BA_A_BOOSTER ].distance;
		}
		else if ( self->botMind->closestBuildings[ BA_A_OVERMIND ].ent )
		{
			distToHealer = self->botMind->closestBuildings[ BA_A_OVERMIND ].distance;
		}
		else if ( self->botMind->closestBuildings[ BA_A_SPAWN ].ent )
		{
			distToHealer = self->botMind->closestBuildings[ BA_A_SPAWN ].distance;
		}
	}
	else
	{
		distToHealer = self->botMind->closestBuildings[ BA_H_MEDISTAT ].distance;
	}

	float timeDist = distToHealer / GetMaximalSpeed( self );
	return ( 1 + 5 * SkillModifier( self->botMind->botSkill.level ) ) * ( 1 - percentHealth ) / sqrt( timeDist );
}

float BotGetEnemyPriority( gentity_t *self, gentity_t *ent )
{
	float enemyScore;
	float distanceScore;
	distanceScore = Distance( self->s.origin, ent->s.origin );

	if ( ent->client )
	{
		switch ( ent->s.weapon )
		{
			case WP_ALEVEL0:
				enemyScore = 0.1;
				break;
			case WP_ALEVEL1:
				enemyScore = 0.3;
				break;
			case WP_ALEVEL2:
				enemyScore = 0.4;
				break;
			case WP_ALEVEL2_UPG:
				enemyScore = 0.7;
				break;
			case WP_ALEVEL3:
				enemyScore = 0.7;
				break;
			case WP_ALEVEL3_UPG:
				enemyScore = 0.8;
				break;
			case WP_ALEVEL4:
				enemyScore = 1.0;
				break;
			case WP_BLASTER:
				enemyScore = 0.2;
				break;
			case WP_MACHINEGUN:
				enemyScore = 0.4;
				break;
			case WP_PAIN_SAW:
				enemyScore = 0.4;
				break;
			case WP_LAS_GUN:
				enemyScore = 0.4;
				break;
			case WP_MASS_DRIVER:
				enemyScore = 0.4;
				break;
			case WP_CHAINGUN:
				enemyScore = 0.6;
				break;
			case WP_FLAMER:
				enemyScore = 0.6;
				break;
			case WP_PULSE_RIFLE:
				enemyScore = 0.5;
				break;
			case WP_LUCIFER_CANNON:
				enemyScore = 1.0;
				break;
			default:
				enemyScore = 0.5;
				break;
		}
	}
	else
	{
		switch ( ent->s.modelindex )
		{
			case BA_H_MGTURRET:
				enemyScore = 0.6;
				break;
			case BA_H_REACTOR:
				enemyScore = 0.5;
				break;
			case BA_H_ROCKETPOD:
				enemyScore = 0.7;
				break;
			case BA_H_SPAWN:
				enemyScore = 0.9;
				break;
			case BA_H_ARMOURY:
				enemyScore = 0.8;
				break;
			case BA_H_MEDISTAT:
				enemyScore = 0.6;
				break;
			case BA_A_ACIDTUBE:
				enemyScore = 0.7;
				break;
			case BA_A_SPAWN:
				enemyScore = 0.9;
				break;
			case BA_A_OVERMIND:
				enemyScore = 0.5;
				break;
			case BA_A_HIVE:
				enemyScore = 0.7;
				break;
			case BA_A_BOOSTER:
				enemyScore = 0.4;
				break;
			case BA_A_TRAPPER:
				enemyScore = 0.8;
				break;
			case BA_A_LEECH:
				enemyScore = 0.6;
				break;
			case BA_H_DRILL:
				enemyScore = 0.6;
				break;
			default:
				enemyScore = 0.5;
				break;

		}
	}
	return enemyScore * 1000 / distanceScore;
}


bool BotCanEvolveToClass( const gentity_t *self, class_t newClass )
{
	equipment_t<class_t>* cl = std::find( std::begin( classes ), std::end( classes ), newClass );
	int fromClass = self->client->ps.stats[STAT_CLASS];
	evolveInfo_t info = BG_ClassEvolveInfoFromTo( fromClass, newClass );

	if ( cl == std::end( classes ) )
	{
		Log::Warn( "invalid class requested" );
		return false;
	}

	if ( !g_bot_evolve.Get() || Entities::IsDead( self ) )
	{
		return false;
	}

	return cl->canBuyNow()
		&& self->client->ps.persistant[PERS_CREDIT] >= info.evolveCost;
}

bool WeaponIsEmpty( weapon_t weapon, playerState_t *ps )
{
	return ps->ammo <= 0 && ps->clips <= 0 && !BG_Weapon( weapon )->infiniteAmmo;
}

float PercentAmmoRemaining( weapon_t weapon, playerState_t const* ps )
{
	int maxAmmo, maxClips;
	float totalMaxAmmo, totalAmmo;

	maxAmmo = BG_Weapon( weapon )->maxAmmo;
	maxClips = BG_Weapon( weapon )->maxClips;

	if ( !BG_Weapon( weapon )->infiniteAmmo )
	{
		totalMaxAmmo = ( float ) maxAmmo + maxClips * maxAmmo;
		totalAmmo = ( float ) ps->ammo + ps->clips * maxAmmo;

		return totalAmmo / totalMaxAmmo;
	}
	else
	{
		return 1.0f;
	}
}

AINodeStatus_t BotActionEvolve( gentity_t *self, AIGenericNode_t* )
{
	class_t currentClass = static_cast<class_t>( self->client->ps.stats[ STAT_CLASS ] );

	for ( auto const& cl : classes )
	{
		evolveInfo_t info = BG_ClassEvolveInfoFromTo( currentClass, cl.item );

		if ( info.evolveCost > 0 // no devolving or evolving to the same
				&& BotEvolveToClass( self, cl.item ) )
		{
			return STATUS_SUCCESS;
		}
	}

	return STATUS_FAILURE;
}

// Allow human bots to decide what to buy
// pre-condition:
// * weapon is a valid pointer
// * "upgrades" is an array of at least upgradesSize elements
// post-conditions:
// * "upgrades" contains a list of upgrades to use
// * "weapon" contains weapon to use
// * Returns number of upgrades to buy (does not includes weapons)
//
// will favor better armor above everything else. If it is possible
// to buy an armor, then evaluate other team-utilies and finally choose
// the most expensive gun possible.
// TODO: allow bots to buy jetpack, despite the fact they can't use them
//  (yet): since default cVar prevent bots to buy those, that will be one
//  less thing to change later and would have no impact.
int BotGetDesiredBuy( gentity_t *self, weapon_t &weapon, upgrade_t upgrades[], size_t upgradesSize )
{
	ASSERT( self && upgrades );
	ASSERT( self->client->pers.team == TEAM_HUMANS ); // only humans can buy
	ASSERT( upgradesSize >= 2 ); // we access to 2 elements maximum, and don't really check boundaries (would result in a nerf)
	int equipmentPrice = BG_GetPlayerPrice( self->client->ps );
	int credits = self->client->ps.persistant[PERS_CREDIT];
	int usableCapital = credits + equipmentPrice;
	size_t numUpgrades = 0;
	int usedSlots = 0;

	unsigned int numTeamUpgrades[UP_NUM_UPGRADES] = {};
	unsigned int numTeamWeapons[WP_NUM_WEAPONS] = {};

	ListTeamEquipment( self, numTeamUpgrades, numTeamWeapons );
	weapon = WP_NONE;
	for ( size_t i = 0; i < upgradesSize; ++i )
	{
		upgrades[i] = UP_NONE;
	}

	for ( auto const &armor : armors )
	{
		// buy armor if one of following is true:
		// * already worn (assumed that armors are sorted by preference)
		// * better (more expensive), authorized, and have the slots
		if ( BG_InventoryContainsUpgrade( armor.item, self->client->ps.stats ) ||
				( armor.canBuyNow() && usableCapital >= armor.price()
				&& ( usedSlots & armor.slots() ) == 0 ) )
		{
			upgrades[numUpgrades] = armor.item;
			usableCapital -= armor.price();
			usedSlots |= armor.slots();
			numUpgrades++;
			break;
		}
	}

	//TODO this really needs more generic code, but that would require
	//deeper refactoring (probably move equipments and classes into structs)
	//and code to make bots _actually_ use other equipments.
	int nbTeam = level.team[ G_Team( self ) ].numClients;
	int nbRadars = numTeamUpgrades[UP_RADAR];
	bool teamNeedsRadar = 100 *  nbRadars / nbTeam < g_bot_radarRatio.Get();

	// others[0] is radar, buying this utility makes sense even if one can't buy
	// a better weapon, because it helps the whole team.
	if ( numUpgrades > 0 && teamNeedsRadar
			&& others[0].canBuyNow() && usableCapital >= others[0].price()
			&& ( usedSlots & others[0].slots() ) == 0
			&& numUpgrades < upgradesSize )
	{
		upgrades[numUpgrades] = others[0].item;
		usableCapital -= others[0].price();
		usedSlots |= others[0].slots();
		numUpgrades ++;
	}

	for ( auto const &wp : weapons )
	{
		if ( wp.canBuyNow() && usableCapital >= wp.price()
				&& ( usedSlots & wp.slots() ) == 0 )
		{
			if ( wp.item == WP_FLAMER && numTeamWeapons[WP_FLAMER] > numTeamWeapons[WP_PULSE_RIFLE] )
			{
				continue;
			}
			weapon = wp.item;
			usableCapital -= wp.price();
			break;
		}
	}

	for ( auto const &tool : others )
	{
		// skip radar checks, since already done
		if ( tool.item == others[0].item )
		{
			continue;
		}
		if ( tool.canBuyNow() && usableCapital >= tool.price()
				&& ( usedSlots & tool.slots() ) == 0
				&& numUpgrades < upgradesSize )
		{
			upgrades[numUpgrades] = tool.item;
			usableCapital -= tool.price();
			usedSlots |= tool.slots();
			numUpgrades ++;
		}
	}
	return numUpgrades;
}

/*
=======================
Entity Querys
=======================
*/

gentity_t* BotFindBuilding( gentity_t *self, int buildingType, int range )
{
	float minDistance = -1;
	gentity_t* closestBuilding = nullptr;
	float newDistance;
	float rangeSquared = Square( range );
	gentity_t *target = &g_entities[MAX_CLIENTS];
	int i;

	for ( i = MAX_CLIENTS; i < level.num_entities; i++, target++ )
	{
		if ( !target->inuse )
		{
			continue;
		}
		if ( target->s.eType == entityType_t::ET_BUILDABLE &&
		     target->s.modelindex == buildingType &&
		     target->powered && target->spawned &&
		     Entities::IsAlive( target ) )
		{
			newDistance = DistanceSquared( self->s.origin, target->s.origin );
			if ( range && newDistance > rangeSquared )
			{
				continue;
			}
			if ( newDistance < minDistance || minDistance == -1 )
			{
				minDistance = newDistance;
				closestBuilding = target;
			}
		}
	}
	return closestBuilding;
}

void BotFindClosestBuildings( gentity_t *self )
{
	gentity_t *testEnt;
	botEntityAndDistance_t *ent;

	// clear out building list
	for ( unsigned i = 0; i < ARRAY_LEN( self->botMind->closestBuildings ); i++ )
	{
		self->botMind->closestBuildings[ i ].ent = nullptr;
		self->botMind->closestBuildings[ i ].distance = std::numeric_limits<float>::max();
	}

	for ( testEnt = &g_entities[MAX_CLIENTS]; testEnt < &g_entities[level.num_entities - 1]; testEnt++ )
	{
		float newDist;
		// ignore entities that aren't in use
		if ( !testEnt->inuse )
		{
			continue;
		}

		// skip non buildings
		if ( testEnt->s.eType != entityType_t::ET_BUILDABLE )
		{
			continue;
		}

		// ignore dead targets
		if ( Entities::IsDead( testEnt ) )
		{
			continue;
		}

		// skip buildings that are currently building or aren't powered
		if ( !testEnt->powered || !testEnt->spawned )
		{
			continue;
		}

		newDist = Distance( self->s.origin, testEnt->s.origin );

		ent = &self->botMind->closestBuildings[ testEnt->s.modelindex ];

		if ( newDist < ent->distance )
		{
			ent->ent = testEnt;
			ent->distance = newDist;
		}
	}
}

void BotFindDamagedFriendlyStructure( gentity_t *self )
{
	float minDistSqr;

	gentity_t *target;
	self->botMind->closestDamagedBuilding.ent = nullptr;
	self->botMind->closestDamagedBuilding.distance = std::numeric_limits<float>::max();

	minDistSqr = Square( self->botMind->closestDamagedBuilding.distance );

	for ( target = &g_entities[MAX_CLIENTS]; target < &g_entities[level.num_entities]; target++ )
	{
		float distSqr;

		if ( !target->inuse )
		{
			continue;
		}

		if ( target->s.eType != entityType_t::ET_BUILDABLE )
		{
			continue;
		}

		if ( target->buildableTeam != self->client->pers.team )
		{
			continue;
		}

		if ( Entities::HasFullHealth(target) )
		{
			continue;
		}

		if ( Entities::IsDead( target ) )
		{
			continue;
		}

		if ( !target->spawned || !target->powered )
		{
			continue;
		}

		distSqr = DistanceSquared( self->s.origin, target->s.origin );
		if ( distSqr < minDistSqr )
		{
			self->botMind->closestDamagedBuilding.ent = target;
			self->botMind->closestDamagedBuilding.distance = sqrtf( distSqr );
			minDistSqr = distSqr;
		}
	}
}

static bool BotEntityIsVisible( const gentity_t *self, gentity_t *target, int mask )
{
	botTarget_t bt;
	bt = target;
	return BotTargetIsVisible( self, bt, mask );
}

static float BotAimAngle( gentity_t *self, const glm::vec3 &pos )
{
	glm::vec3 forward;

	AngleVectors( self->client->ps.viewangles, &forward[0], nullptr, nullptr );
	glm::vec3 viewPos = BG_GetClientViewOrigin( &self->client->ps );
	glm::vec3 ideal = pos - viewPos;

	return glm::degrees( glm::angle( glm::normalize( forward ), glm::normalize( ideal ) ) );
}

gentity_t* BotFindBestEnemy( gentity_t *self )
{
	float bestVisibleEnemyScore = 0.0f;
	float bestInvisibleEnemyScore = 0.0f;
	gentity_t *bestVisibleEnemy = nullptr;
	gentity_t *bestInvisibleEnemy = nullptr;
	gentity_t *target;
	team_t    team = G_Team( self );
	bool  hasRadar = ( team == TEAM_ALIENS ) ||
	                     ( team == TEAM_HUMANS && BG_InventoryContainsUpgrade( UP_RADAR, self->client->ps.stats ) );

	for ( target = g_entities; target < &g_entities[level.num_entities - 1]; target++ )
	{
		float newScore;

		if ( !BotEntityIsValidEnemyTarget( self, target ) )
		{
			continue;
		}

		if ( DistanceSquared( self->s.origin, target->s.origin ) > Square( ALIENSENSE_RANGE ) )
		{
			continue;
		}

		glm::vec3 vorigin = VEC2GLM( target->s.origin );
		if ( target->s.eType == entityType_t::ET_PLAYER && self->client->pers.team == TEAM_HUMANS
		    && BotAimAngle( self, vorigin ) > g_bot_fov.Get() / 2 )
		{
			continue;
		}

		if ( target == self->botMind->goal.getTargetedEntity() )
		{
			continue;
		}

		newScore = BotGetEnemyPriority( self, target );

		if ( newScore > bestVisibleEnemyScore && BotEntityIsVisible( self, target, MASK_OPAQUE ) )
		{
			//store the new score and the index of the entity
			bestVisibleEnemyScore = newScore;
			bestVisibleEnemy = target;
		}
		else if ( newScore > bestInvisibleEnemyScore && hasRadar )
		{
			bestInvisibleEnemyScore = newScore;
			bestInvisibleEnemy = target;
		}
	}
	if ( bestVisibleEnemy || !hasRadar )
	{
		return bestVisibleEnemy;
	}
	else
	{
		return bestInvisibleEnemy;
	}
}

gentity_t* BotFindClosestEnemy( gentity_t *self )
{
	gentity_t* closestEnemy = nullptr;
	float minDistance = Square( ALIENSENSE_RANGE );
	gentity_t *target;

	for ( target = g_entities; target < &g_entities[level.num_entities - 1]; target++ )
	{
		float newDistance;
		//ignore entities that arnt in use
		if ( !target->inuse )
		{
			continue;
		}

		if ( !BotEntityIsValidEnemyTarget( self, target ) )
		{
			continue;
		}

		newDistance = DistanceSquared( self->s.origin, target->s.origin );
		if ( newDistance <= minDistance )
		{
			minDistance = newDistance;
			closestEnemy = target;
		}
	}
	return closestEnemy;
}

botTarget_t BotGetRushTarget( const gentity_t *self )
{
	botTarget_t target;
	gentity_t const* rushTarget = nullptr;
	if ( G_Team( self ) == TEAM_HUMANS )
	{
		if ( self->botMind->closestBuildings[BA_A_SPAWN].ent )
		{
			rushTarget = self->botMind->closestBuildings[BA_A_SPAWN].ent;
		}
		else if ( self->botMind->closestBuildings[BA_A_OVERMIND].ent )
		{
			rushTarget = self->botMind->closestBuildings[BA_A_OVERMIND].ent;
		}
	}
	else    //team aliens
	{
		if ( self->botMind->closestBuildings[BA_H_SPAWN].ent )
		{
			rushTarget = self->botMind->closestBuildings[BA_H_SPAWN].ent;
		}
		else if ( self->botMind->closestBuildings[BA_H_REACTOR].ent )
		{
			rushTarget = self->botMind->closestBuildings[BA_H_REACTOR].ent;
		}
	}
	target = rushTarget;
	return target;
}

botTarget_t BotGetRetreatTarget( const gentity_t *self )
{
	botTarget_t target;
	gentity_t const* retreatTarget = nullptr;
	//FIXME, this seems like it could be done better...
	if ( self->client->pers.team == TEAM_HUMANS )
	{
		if ( self->botMind->closestBuildings[BA_H_REACTOR].ent )
		{
			retreatTarget = self->botMind->closestBuildings[BA_H_REACTOR].ent;
		}
	}
	else
	{
		if ( self->botMind->closestBuildings[BA_A_OVERMIND].ent )
		{
			retreatTarget = self->botMind->closestBuildings[BA_A_OVERMIND].ent;
		}
	}
	target = retreatTarget;
	return target;
}

botTarget_t BotGetRoamTarget( const gentity_t *self )
{
	botTarget_t target;
	glm::vec3 point;

	if ( !BotFindRandomPointInRadius( self->s.number, VEC2GLM( self->s.origin ), point, 2000 ) )
	{
		target = VEC2GLM( self->s.origin );
	}
	else
	{
		target = point;
	}

	return target;
}

/*
========================
BotTarget Helpers
========================
*/

static void BotTargetGetBoundingBox( botTarget_t target, glm::vec3 &mins, glm::vec3 &maxs, botRouteTarget_t *routeTarget )
{
	ASSERT( target.isValid() );

	if ( target.targetsCoordinates() )
	{
		// point target
		maxs = glm::vec3(  96,  96,  96 );
		mins = glm::vec3( -96, -96, -96 );
		routeTarget->type = botRouteTargetType_t::BOT_TARGET_STATIC;
		return;
	}

	// entity
	const gentity_t *ent = target.getTargetedEntity();
	bool isPlayer = ent->client != nullptr;
	if ( isPlayer )
	{
		class_t class_ = static_cast<class_t>( ent->client->ps.stats[ STAT_CLASS ] );
		BG_BoundingBox( class_, &mins, &maxs, nullptr, nullptr, nullptr );
	}
	else if ( target.getTargetType() == entityType_t::ET_BUILDABLE )
	{
		BG_BoundingBox( static_cast<buildable_t>( ent->s.modelindex ), &mins, &maxs );
	}
	else
	{
		mins = VEC2GLM( ent->r.mins );
		maxs = VEC2GLM( ent->r.maxs );
	}

	if ( isPlayer )
	{
		routeTarget->type = botRouteTargetType_t::BOT_TARGET_DYNAMIC;
	}
	else
	{
		routeTarget->type = botRouteTargetType_t::BOT_TARGET_STATIC;
	}
}

void BotTargetToRouteTarget( const gentity_t *self, botTarget_t target, botRouteTarget_t *routeTarget )
{
	ASSERT( target.isValid() );

	glm::vec3 mins, maxs;

	routeTarget->setPos( target.getPos() );
	BotTargetGetBoundingBox( target, mins, maxs, routeTarget );

	routeTarget->polyExtents[ 0 ] = std::max( Q_fabs( mins[ 0 ] ), maxs[ 0 ] );
	routeTarget->polyExtents[ 1 ] = std::max( Q_fabs( mins[ 1 ] ), maxs[ 1 ] );
	routeTarget->polyExtents[ 2 ] = std::max( Q_fabs( mins[ 2 ] ), maxs[ 2 ] );

	// move center a bit lower so we don't get polys above the object
	// and get polys below the object on a slope
	routeTarget->pos[ 2 ] -= routeTarget->polyExtents[ 2 ] / 2;

	entityType_t entType = target.getTargetType();

	// account for building on wall or ceiling
	if ( entType == entityType_t::ET_PLAYER ||
			( entType == entityType_t::ET_BUILDABLE
			  && target.getTargetedEntity()->s.origin2[ 2 ] < MIN_WALK_NORMAL ) )
	{
		trace_t trace;

		routeTarget->polyExtents[ 0 ] += 25;
		routeTarget->polyExtents[ 1 ] += 25;
		routeTarget->polyExtents[ 2 ] += 300;

		// try to find a position closer to the ground
		glm::vec3 invNormal = { 0, 0, -1 };
		glm::vec3 targetPos = target.getPos();
		glm::vec3 end = targetPos + 600.f * invNormal;
		trap_Trace( &trace, targetPos, mins, maxs, end, target.getTargetedEntity()->s.number,
		            CONTENTS_SOLID, MASK_ENTITY );
		routeTarget->setPos( VEC2GLM( trace.endpos ) );
	}
	

	// increase extents a little to account for obstacles cutting into the navmesh
	// also accounts for navmesh erosion at mesh boundrys
	routeTarget->polyExtents[ 0 ] += self->r.maxs[ 0 ] + 10;
	routeTarget->polyExtents[ 1 ] += self->r.maxs[ 1 ] + 10;
}

bool BotChangeGoal( gentity_t *self, botTarget_t target )
{
	if ( !target.isValid() )
	{
		return false;
	}

	if ( !FindRouteToTarget( self, target, false ) )
	{
		return false;
	}

	self->botMind->goal = target;
	self->botMind->m_nav.directPathToGoal = false;
	return true;
}

bool BotChangeGoalEntity( gentity_t *self, gentity_t const *goal )
{
	botTarget_t target;
	target = goal;
	return BotChangeGoal( self, target );
}

bool BotChangeGoalPos( gentity_t *self, const glm::vec3 &goal )
{
	botTarget_t target;
	target = goal;
	return BotChangeGoal( self, target );
}

bool BotTargetInAttackRange( const gentity_t *self, botTarget_t target )
{
	ASSERT( target.targetsValidEntity() );

	float range, secondaryRange;
	glm::vec3 forward, right, up;
	glm::vec3 muzzle;
	glm::vec3 maxs, mins;
	glm::vec3 targetPos;
	trace_t trace;
	float width = 0, height = 0;

	AngleVectors( VEC2GLM( self->client->ps.viewangles ), &forward, &right, &up );
	muzzle = G_CalcMuzzlePoint( self, forward );
	targetPos = target.getPos();
	switch ( self->client->ps.weapon )
	{
		case WP_ABUILD:
			range = ABUILDER_CLAW_RANGE;
			secondaryRange = 0;
			width = height = ABUILDER_CLAW_WIDTH;
			break;
		case WP_ABUILD2:
			range = ABUILDER_CLAW_RANGE;
			secondaryRange = 300; //An arbitrary value for the blob launcher, has nothing to do with actual range
			width = height = ABUILDER_CLAW_WIDTH;
			break;
		case WP_ALEVEL0:
			range = LEVEL0_BITE_RANGE;
			secondaryRange = 0;
			break;
		case WP_ALEVEL1:
			range = LEVEL1_CLAW_RANGE;
			secondaryRange = LEVEL1_POUNCE_DISTANCE;
			width = height = LEVEL1_CLAW_WIDTH;
			break;
		case WP_ALEVEL2:
			range = LEVEL2_CLAW_RANGE;
			secondaryRange = 0;
			width = height = LEVEL2_CLAW_WIDTH;
			break;
		case WP_ALEVEL2_UPG:
			range = LEVEL2_CLAW_U_RANGE;
			secondaryRange = LEVEL2_AREAZAP_RANGE;
			width = height = LEVEL2_CLAW_WIDTH;
			break;
		case WP_ALEVEL3:
			range = LEVEL3_CLAW_RANGE;
			//need to check if we can pounce to the target
			secondaryRange = LEVEL3_POUNCE_JUMP_MAG; //An arbitrary value for pounce, has nothing to do with actual range
			width = height = LEVEL3_CLAW_WIDTH;
			break;
		case WP_ALEVEL3_UPG:
			range = LEVEL3_CLAW_RANGE;
			//we can pounce, or we have barbs
			secondaryRange = LEVEL3_POUNCE_JUMP_MAG_UPG; //An arbitrary value for pounce and barbs, has nothing to do with actual range
			if ( self->client->ps.ammo > 0 )
			{
				secondaryRange = 900;
			}
			width = height = LEVEL3_CLAW_WIDTH;
			break;
		case WP_ALEVEL4:
			range = LEVEL4_CLAW_RANGE;
			secondaryRange = 0; //Using 0 since tyrant rush is basically just movement, not a ranged attack
			width = height = LEVEL4_CLAW_WIDTH;
			break;
		case WP_HBUILD:
			range = 100;
			secondaryRange = 0;
			break;
		case WP_PAIN_SAW:
			range = PAINSAW_RANGE;
			secondaryRange = 0;
			break;
		case WP_FLAMER:
			{
				trajectory_t t;
			
				// Correct muzzle so that the missile does not start in the ceiling
				muzzle += -7.0f * up;

				// Correct muzzle so that the missile fires from the player's hand
				muzzle += 4.5f * right;

				// flamer projectiles add the player's velocity scaled by FLAMER_LAG to the fire direction with length FLAMER_SPEED
				glm::vec3 dir = targetPos - muzzle;
				dir = glm::normalize( dir );
				glm::vec3 nvel = VEC2GLM( self->client->ps.velocity ) * FLAMER_LAG;
				VectorMA( nvel, FLAMER_SPEED, dir, t.trDelta );
				SnapVector( t.trDelta );
				VectorCopy( muzzle, t.trBase );
				t.trType = trType_t::TR_LINEAR;
				t.trTime = level.time - 50;
			
				// find projectile's final position
				glm::vec3 npos;
				BG_EvaluateTrajectory( &t, level.time + FLAMER_LIFETIME, &npos[0] );

				// find distance traveled by projectile along fire line
				glm::vec3 proj = ProjectPointOntoVector( npos, muzzle, targetPos );
				range = glm::distance( muzzle, proj );

				// make sure the sign of the range is correct
				glm::vec3 rdir = npos - muzzle;
				if ( glm::dot( rdir, dir ) < 0 )
				{
					range = -range;
				}

				// the flamer uses a cosine based power falloff by default
				// so decrease the range to give us a usable minimum damage
				// FIXME: depend on the value of the flamer damage falloff cvar
				// FIXME: have to stand further away from acid or will be
				//        pushed back and will stop attacking (too far away)
				if ( target.getTargetType() == entityType_t::ET_BUILDABLE &&
				     target.getTargetedEntity()->s.modelindex != BA_A_ACIDTUBE )
				{
					range -= 300;
				}
				else
				{
					range -= 150;
				}
				range = std::max( range, 100.0f );
				secondaryRange = 0;
				width = height = FLAMER_SIZE;
			}
			break;
		case WP_SHOTGUN:
			range = ( 50 * 8192 ) / SHOTGUN_SPREAD; //50 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		case WP_MACHINEGUN:
			range = ( 100 * 8192 ) / RIFLE_SPREAD; //100 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		case WP_CHAINGUN:
			range = ( 60 * 8192 ) / CHAINGUN_SPREAD; //60 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		default:
			range = 4098 * 4; //large range for guns because guns have large ranges :)
			secondaryRange = 0; //no secondary attack
	}
	maxs = {  width,  width,  width };
	mins = { -width, -width, -height };

	trap_Trace( &trace, muzzle, mins, maxs, targetPos, self->s.number, MASK_SHOT, 0 );

	return !G_OnSameTeam( self, &g_entities[trace.entityNum] )
		&& G_Team( &g_entities[ trace.entityNum ] ) != TEAM_NONE
		&& glm::distance( muzzle, VEC2GLM( trace.endpos ) ) <= std::max( range, secondaryRange );
}

bool BotEntityIsValidTarget( const gentity_t *ent )
{
	// spectators are not considered alive
	return ent != nullptr && ent->inuse && Entities::IsAlive(ent);
}

bool BotEntityIsValidEnemyTarget( const gentity_t *self, const gentity_t *enemy )
{
	if ( !BotEntityIsValidTarget( enemy ) )
	{
		return false;
	}

	// ignore neutrals
	if ( G_Team( enemy ) == TEAM_NONE )
	{
		return false;
	}

	// ignore teamates
	if ( G_OnSameTeam( self, enemy ) )
	{
		return false;
	}

	// ignore buildings if we can't attack them
	if ( enemy->s.eType == entityType_t::ET_BUILDABLE && !g_bot_attackStruct.Get() )
	{
		return false;
	}

	// dretch limitations
	if ( self->client->ps.stats[STAT_CLASS] == PCL_ALIEN_LEVEL0 && !G_DretchCanDamageEntity( self, enemy ) )
	{
		return false;
	}

	return true;
}

bool BotTargetIsVisible( const gentity_t *self, botTarget_t target, int mask )
{
	ASSERT( target.targetsValidEntity() );

	trace_t trace;
	glm::vec3  muzzle, targetPos;
	glm::vec3  forward, right, up;

	AngleVectors( VEC2GLM( self->client->ps.viewangles ), &forward, &right, &up );
	muzzle = G_CalcMuzzlePoint( self, forward );
	targetPos = target.getPos();

	if ( !trap_InPVS( &muzzle[0], &targetPos[0] ) )
	{
		return false;
	}

	trap_Trace( &trace, &muzzle[0], nullptr, nullptr, &targetPos[0], self->s.number, mask, 0 );

	if ( trace.surfaceFlags & SURF_NOIMPACT )
	{
		return false;
	}

	//target is in range
	if ( ( trace.entityNum == target.getTargetedEntity()->s.number
				|| trace.fraction == 1.0f )
			&& !trace.startsolid )
	{
		return true;
	}
	return false;
}

/*
========================
Bot Aiming
========================
*/

glm::vec3 BotGetIdealAimLocation( gentity_t *self, const botTarget_t &target )
{
	ASSERT( target.targetsValidEntity() );
	glm::vec3 aimLocation = target.getPos();
	const gentity_t *targetEnt = target.getTargetedEntity();
	team_t targetTeam = G_Team( targetEnt );
	bool isTargetBuildable = target.getTargetType() == entityType_t::ET_BUILDABLE;

	//this retrieves the target's species, to aim at weak point:
	// * for humans, it's the head (but code only applies an offset here, with the hope it's the head)
	// * for aliens, there is no weak point, and human bots try to take missile's speed into consideration (for luci)
	if ( !isTargetBuildable && targetTeam == TEAM_HUMANS )
	{
		//aim at head
		//FIXME: do not rely on hard-coded offset but evaluate which point have lower armor
		aimLocation[2] += targetEnt->r.maxs[2] * 0.85;

	}
	else
	{
		//make lucifer cannons (& other slow human weapons, maybe aliens would need it, too?) aim ahead based on the target's velocity
		if ( self->botMind->botSkill.level >= 5 )
		{
			//would be better if it was possible to do self.weapon->speed directly
			int weapon_speed = 0;
			switch ( self->client->ps.weapon ) {
			case WP_BLASTER:
				weapon_speed = BLASTER_SPEED;
				break;
			case WP_FLAMER:
				weapon_speed = FLAMER_SPEED;
				break;
			case WP_PULSE_RIFLE:
				weapon_speed = PRIFLE_SPEED;
				break;
			case WP_LUCIFER_CANNON:
				weapon_speed = LCANNON_SPEED;
				break;
			}
			if( weapon_speed )
			{
				aimLocation += glm::distance( VEC2GLM( self->s.origin ), aimLocation ) / weapon_speed * VEC2GLM( targetEnt->s.pos.trDelta );
			}
		}
	}
	return aimLocation;
}

static int BotGetAimPredictionTime( gentity_t *self )
{
	auto time = ( 10 - self->botMind->botSkill.level ) * 100 * std::max( random(), 0.5f );
	return std::max( 1, int(time) );
}

static glm::vec3 BotPredictPosition( gentity_t *self, gentity_t const *predict, int time )
{
	botTarget_t target;
	target = predict;
	glm::vec3 aimLoc = BotGetIdealAimLocation( self, target );
	return aimLoc + time / 1000.0f * VEC2GLM( predict->s.apos.trDelta );
}

void BotAimAtEnemy( gentity_t *self )
{
	ASSERT( self->botMind->goal.targetsValidEntity() );

	int i;
	float frac;
	const gentity_t *enemy = self->botMind->goal.getTargetedEntity();

	if ( self->botMind->futureAimTime < level.time )
	{
		int predictTime = self->botMind->futureAimTimeInterval = BotGetAimPredictionTime( self );
		self->botMind->futureAim = BotPredictPosition( self, enemy, predictTime );
		self->botMind->futureAimTime = level.time + predictTime;
	}

	glm::vec3 viewOrigin = BG_GetClientViewOrigin( &self->client->ps );
	glm::vec3 desired = VEC2GLM( self->botMind->futureAim ) - viewOrigin;
	desired = glm::normalize( desired );
	glm::vec3 current;
	AngleVectors( VEC2GLM( self->client->ps.viewangles ), &current, nullptr, nullptr );

	frac = ( 1.0f - ( ( float ) ( self->botMind->futureAimTime - level.time ) ) / self->botMind->futureAimTimeInterval );
	glm::vec3 newAim = glm::mix( current, desired, frac );

	VectorSet( self->client->ps.delta_angles, 0, 0, 0 );
	glm::vec3 angles;
	vectoangles( &newAim[0], &angles[0] );

	for ( i = 0; i < 3; i++ )
	{
		self->botMind->cmdBuffer.angles[ i ] = ANGLE2SHORT( angles[ i ] );
	}
}

void BotAimAtLocation( gentity_t *self, const glm::vec3 &target )
{
	glm::vec3 aimVec, aimAngles, viewBase;
	int i;
	usercmd_t *rAngles = &self->botMind->cmdBuffer;

	if ( !self->client )
	{
		return;
	}

	BG_GetClientViewOrigin( &self->client->ps, &viewBase[0] );
	aimVec = target - viewBase;

	vectoangles( &aimVec[0], &aimAngles[0] );

	VectorSet( self->client->ps.delta_angles, 0.0f, 0.0f, 0.0f );

	for ( i = 0; i < 3; i++ )
	{
		aimAngles[i] = ANGLE2SHORT( aimAngles[i] );
	}

	//save bandwidth
	//Meh. I doubt it saves much. Casting to short ints might have, though. (copypaste)
	aimAngles = glm::floor( aimAngles + 0.5f );
	rAngles->angles[0] = aimAngles[0];
	rAngles->angles[1] = aimAngles[1];
	rAngles->angles[2] = aimAngles[2];
}

void BotSlowAim( gentity_t *self, glm::vec3 &target, float slowAmount )
{
	if ( !( self && self->client ) )
	{
		return;
	}

	//clamp from 0.1 to 1
	float slow = Math::Clamp( slowAmount, 0.1f, 1.0f );

	//get the point that the bot is aiming from
	glm::vec3 viewBase = BG_GetClientViewOrigin( &self->client->ps );

	//get the Vector from the bot to the enemy (ideal aim Vector)
	glm::vec3 aimVec = target - viewBase;
	float length = glm::length( aimVec );
	aimVec = glm::normalize( aimVec );

	//take the current aim Vector
	glm::vec3 forward;
	AngleVectors( VEC2GLM( self->client->ps.viewangles ), &forward, nullptr, nullptr );

	float cosAngle = glm::dot( forward, aimVec );
	cosAngle = ( cosAngle + 1.0f ) / 2.0f;
	cosAngle = 1.0f - cosAngle;
	cosAngle = Math::Clamp( cosAngle, 0.1f, 0.5f );
	glm::vec3 skilledVec = glm::mix( forward, aimVec, slow * cosAngle );

	//now find a point to return, this point will be aimed at
	target = viewBase + length * skilledVec;
}

float BotAimAngle( gentity_t *self, vec3_t pos )
{
	vec3_t viewPos;
	vec3_t forward;
	vec3_t ideal;

	AngleVectors( self->client->ps.viewangles, forward, nullptr, nullptr );
	BG_GetClientViewOrigin( &self->client->ps, viewPos );
	VectorSubtract( pos, viewPos, ideal );

	return AngleBetweenVectors( forward, ideal );
}

/*
========================
Bot Team Querys
========================
*/

int FindBots( int *botEntityNumbers, int maxBots, team_t team )
{
	gentity_t *testEntity;
	int numBots = 0;
	int i;
	memset( botEntityNumbers, 0, sizeof( int )*maxBots );
	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		testEntity = &g_entities[i];
		if ( testEntity->r.svFlags & SVF_BOT )
		{
			if ( testEntity->client->pers.team == team && numBots < maxBots )
			{
				botEntityNumbers[numBots++] = i;
			}
		}
	}
	return numBots;
}

bool PlayersBehindBotInSpawnQueue( gentity_t *self )
{
	//this function only checks if there are Humans in the SpawnQueue
	//which are behind the bot
	int i;
	int botPos = 0, lastPlayerPos = 0;
	spawnQueue_t *sq;

	if ( self->client->pers.team > TEAM_NONE &&
	     self->client->pers.team < NUM_TEAMS )
	{
		sq = &level.team[ self->client->pers.team ].spawnQueue;
	}
	else
	{
		return false;
	}

	i = sq->front;

	if ( G_GetSpawnQueueLength( sq ) )
	{
		do
		{
			if ( !( g_entities[sq->clients[ i ]].r.svFlags & SVF_BOT ) )
			{
				if ( i < sq->front )
				{
					lastPlayerPos = i + MAX_CLIENTS - sq->front;
				}
				else
				{
					lastPlayerPos = i - sq->front;
				}
			}

			if ( sq->clients[ i ] == self->s.number )
			{
				if ( i < sq->front )
				{
					botPos = i + MAX_CLIENTS - sq->front;
				}
				else
				{
					botPos = i - sq->front;
				}
			}

			i = QUEUE_PLUS1( i );
		}
		while ( i != QUEUE_PLUS1( sq->back ) );
	}

	if ( botPos < lastPlayerPos )
	{
		return true;
	}
	else
	{
		return false;
	}
}

// Initializes numUpgrades and numWeapons arrays with team's current equipment
// pre-condition:
// * self points to a player which is inside a playable team
// * numUpgrades memory is initialized
// * numWeapons memory is initialized
static void ListTeamEquipment( gentity_t *self, unsigned int (&numUpgrades)[UP_NUM_UPGRADES], unsigned int (&numWeapons)[WP_NUM_WEAPONS] )
{
	ASSERT( self );
	const team_t team = static_cast<team_t>( self->client->pers.team );

	ForEntities<HumanClassComponent>([&](Entity& ent, HumanClassComponent&) {
		gentity_t* ally = ent.oldEnt;
		if ( ally == self || ally->client->pers.team != team )
		{
			return;
		}
		++numWeapons[ally->client->ps.stats[STAT_WEAPON]];

		// for all possible upgrades, check if current player already have it.
		// this is very fragile, at best, since it will break if TEAM_ALIENS becomes
		// able to have upgrades. Class is not considered, neither.
		// last but not least, it's playing with bits.
		int stat = ally->client->ps.stats[STAT_ITEMS];
		for ( int up = UP_NONE + 1; up < UP_NUM_UPGRADES; ++up )
		{
			if ( stat & ( 1 << up ) )
			{
				++numUpgrades[up];
			}
		}
	});
}

bool BotTeamateHasWeapon( gentity_t *self, int weapon )
{
	int botNumbers[MAX_CLIENTS];
	int i;
	int numBots = FindBots( botNumbers, MAX_CLIENTS, ( team_t ) self->client->pers.team );

	for ( i = 0; i < numBots; i++ )
	{
		gentity_t *bot = &g_entities[botNumbers[i]];
		if ( bot == self )
		{
			continue;
		}
		if ( BG_InventoryContainsWeapon( weapon, bot->client->ps.stats ) )
		{
			return true;
		}
	}
	return false;
}

/*
========================
Misc Bot Stuff
========================
*/

void BotFireWeapon( weaponMode_t mode, usercmd_t *botCmdBuffer )
{
	if ( mode == WPM_PRIMARY )
	{
		usercmdPressButton( botCmdBuffer->buttons, BTN_ATTACK );
	}
	else if ( mode == WPM_SECONDARY )
	{
		usercmdPressButton( botCmdBuffer->buttons, BTN_ATTACK2 );
	}
	else if ( mode == WPM_TERTIARY )
	{
		usercmdPressButton( botCmdBuffer->buttons, BTN_ATTACK3 );
	}
}
void BotClassMovement( gentity_t *self, bool inAttackRange )
{
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	switch ( self->client->ps.stats[STAT_CLASS] )
	{
		case PCL_ALIEN_LEVEL0:
			BotStrafeDodge( self );
			break;
		case PCL_ALIEN_LEVEL1:
			break;
		case PCL_ALIEN_LEVEL2:
		case PCL_ALIEN_LEVEL2_UPG:
			if ( self->botMind->nav().directPathToGoal )
			{
				if ( self->client->time1000 % 300 == 0 )
				{
					BotJump( self );
				}
				BotStrafeDodge( self );
			}
			break;
		case PCL_ALIEN_LEVEL3:
			break;
		case PCL_ALIEN_LEVEL3_UPG:
			if ( self->botMind->goal.getTargetType() == entityType_t::ET_BUILDABLE && self->client->ps.ammo > 0
				&& inAttackRange )
			{
				//dont move when sniping buildings
				BotStandStill( self );
			}
			break;
		case PCL_ALIEN_LEVEL4:
			//use rush to approach faster
			if ( !inAttackRange )
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );
			}
			break;
		default:
			break;
	}
}

float CalcAimPitch( gentity_t *self, glm::vec3 &targetPos, float launchSpeed )
{
	float g = self->client->ps.gravity;
	float v = launchSpeed;

	glm::vec3 forward;
	AngleVectors( VEC2GLM( self->client->ps.viewangles ), &forward, nullptr, nullptr );
	const glm::vec3 muzzle = G_CalcMuzzlePoint( self, forward );
	const glm::vec3 &startPos = muzzle;

	// Project everything onto a 2D plane with initial position at (0,0)
	// dz (Δz) is the difference in height and dr (Δr) the distance forward
	float dz = targetPos[2] - startPos[2];
	float dr = glm::length(glm::vec2(targetPos) - glm::vec2(startPos));

	// As long as we would get NaN (sqrt of a negative number), increase
	// velocity to compensate.
	// This is better than returning some failure value because it gives us
	// the best launch angle possible, even if it can't hit in the end.
	float check = Square( Square( v ) ) - g * ( g * Square( dr ) + 2 * dz * Square( v ) );
	while ( check < 0 )
	{
		v += 5;
		check = Square( Square( v ) ) - g * ( g * Square( dr ) + 2 * dz * Square( v ) );
	}

	// the same equation with a + sign would also give a correct result,
	// but with a longer path. Let's use this one instead
	float angle = atanf( ( Square( v ) - sqrtf( check ) ) / ( g * dr ) );

	// give a result in degrees (ps.viewangles units)
	return RAD2DEG( angle );
}

float CalcPounceAimPitch( gentity_t *self, glm::vec3 &targetPos )
{
	vec_t speed = ( self->client->ps.stats[STAT_CLASS] == PCL_ALIEN_LEVEL3 ) ? LEVEL3_POUNCE_JUMP_MAG : LEVEL3_POUNCE_JUMP_MAG_UPG;

	// Aim a bit higher to account for
	// 1. enemies moving (often backwards when fighting)
	// 2. bbox "mins" differences
	// 3. and last but not least, to make it more likely that you pounce
	//    *on them*, actually dealing damage
	// This does still hit the enemy if it moves towards you
	targetPos[2] += 32.0f; // aim 1m higher

	return CalcAimPitch( self, targetPos, speed );

	//in usrcmd angles, a positive angle is down, so multiply angle by -1
	// botCmdBuffer->angles[PITCH] = ANGLE2SHORT(-angle);
}

float CalcBarbAimPitch( gentity_t *self, glm::vec3 &targetPos )
{
	vec_t speed = LEVEL3_BOUNCEBALL_SPEED;
	return CalcAimPitch( self, targetPos, speed );

	//in usrcmd angles, a positive angle is down, so multiply angle by -1
	//botCmdBuffer->angles[PITCH] = ANGLE2SHORT(-angle);
}

void BotFireWeaponAI( gentity_t *self )
{
	float distance;
	glm::vec3 forward, right, up;
	glm::vec3 muzzle;
	trace_t trace;
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	AngleVectors( VEC2GLM( self->client->ps.viewangles ), &forward, &right, &up );
	muzzle = G_CalcMuzzlePoint( self, forward );
	glm::vec3 targetPos = BotGetIdealAimLocation( self, self->botMind->goal );

	trap_Trace( &trace, &muzzle[0], nullptr, nullptr, &targetPos[0], ENTITYNUM_NONE, MASK_SHOT, 0 );
	distance = glm::distance( muzzle, VEC2GLM( trace.endpos ) );
	bool readyFire = self->client->ps.IsWeaponReady();
	glm::vec3 target = self->botMind->goal.getPos();
	switch ( self->s.weapon )
	{
		case WP_ABUILD:
			if ( distance <= ABUILDER_CLAW_RANGE )
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );
			}
			else
			{
				usercmdPressButton( botCmdBuffer->buttons, BTN_GESTURE );    //make cute granger sounds to ward off the would be attackers
			}
			break;
		case WP_ABUILD2:
			if ( distance <= ABUILDER_CLAW_RANGE )
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );    //swipe
			}
			else
			{
				BotFireWeapon( WPM_TERTIARY, botCmdBuffer );    //blob launcher
			}
			break;
		case WP_ALEVEL0:
			break; //auto hit
		case WP_ALEVEL1:
			if ( distance < LEVEL1_CLAW_RANGE && readyFire )
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer ); //mantis swipe
			}
			else if ( self->client->ps.weaponCharge == 0 )
			{
				BotMoveInDir( self, MOVE_FORWARD );
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer ); //mantis forward pounce
			}
			break;
		case WP_ALEVEL2:
			BotFireWeapon( WPM_PRIMARY, botCmdBuffer ); //mara swipe
			break;
		case WP_ALEVEL2_UPG:
			if ( distance <= LEVEL2_CLAW_U_RANGE )
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //mara swipe
			}
			else
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );    //mara lightning
			}
			break;
		case WP_ALEVEL3:
			if ( distance > LEVEL3_CLAW_RANGE && self->client->ps.weaponCharge < LEVEL3_POUNCE_TIME )
			{
				botCmdBuffer->angles[PITCH] = ANGLE2SHORT( -CalcPounceAimPitch( self, target ) ); //compute and apply correct aim pitch to hit target
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer ); //goon pounce
			}
			else
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //goon chomp
			}
			break;
		case WP_ALEVEL3_UPG:
		{
			// We add some protection for barbs so that bots don't
			// barb themselves too easily. The safety factor
			// hopefully accounts for the movement of the bot and
			// its target
			constexpr float barbSafetyFactor = 4.0f/3.0f;
			if ( self->client->ps.ammo > 0 && distance > LEVEL3_CLAW_UPG_RANGE && distance > (barbSafetyFactor * BG_Missile(MIS_BOUNCEBALL)->splashRadius) )
			{
				botCmdBuffer->angles[PITCH] = ANGLE2SHORT( -CalcBarbAimPitch( self, target ) ); //compute and apply correct aim pitch to hit target
				BotFireWeapon( WPM_TERTIARY, botCmdBuffer ); //goon barb
			}
			else if ( distance > LEVEL3_CLAW_UPG_RANGE && self->client->ps.weaponCharge < LEVEL3_POUNCE_TIME_UPG )
			{
				botCmdBuffer->angles[PITCH] = ANGLE2SHORT( -CalcPounceAimPitch( self, target ) ); //compute and apply correct aim pitch to hit target
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer ); //goon pounce
			}
			else
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //goon chomp
			}
			break;
		}
		case WP_ALEVEL4:
			if ( distance > LEVEL4_CLAW_RANGE && self->client->ps.weaponCharge < LEVEL4_TRAMPLE_CHARGE_MAX )
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );    //rant charge
			}
			else
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //rant swipe
			}
			break;
		case WP_LUCIFER_CANNON:
			if ( self->client->ps.weaponCharge < LCANNON_CHARGE_TIME_MAX * Math::Clamp( random(), 0.5f, 1.0f ) )
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );
			}
			break;
		default:
			BotFireWeapon( WPM_PRIMARY, botCmdBuffer );
	}
}

static bool BotChangeClass( gentity_t *self, class_t newClass, glm::vec3 newOrigin )
{
	if ( !G_RoomForClassChange( self, newClass, &newOrigin[0] ) )
	{
		return false;
	}
	VectorCopy( newOrigin, self->client->ps.origin );
	self->client->ps.stats[ STAT_CLASS ] = newClass;
	self->client->pers.classSelection = newClass;
	BotSetNavmesh( self, newClass );
	self->client->ps.eFlags ^= EF_TELEPORT_BIT;
	return true;
}

bool BotEvolveToClass( gentity_t *ent, class_t newClass )
{
	int clientNum;
	int i;
	glm::vec3 infestOrigin;
	class_t currentClass = static_cast<class_t>( ent->client->ps.stats[ STAT_CLASS ] );
	evolveInfo_t evolveInfo;
	int entityList[ MAX_GENTITIES ];
	glm::vec3 range = { AS_OVER_RT3, AS_OVER_RT3, AS_OVER_RT3 };
	glm::vec3 mins, maxs;
	int num;
	gentity_t *other;

	if ( currentClass == newClass )
	{
		return true;
	}

	clientNum = ent->client - level.clients;

	if ( !BotCanEvolveToClass( ent, newClass ) )
	{
		return false;
	}

	//if we are not currently spectating, we are attempting evolution
	if ( ent->client->pers.classSelection != PCL_NONE )
	{
		if ( ( ent->client->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
		{
			ent->client->pers.cmd.upmove = 0;
		}

		//check there are no humans nearby
		maxs = VEC2GLM( ent->client->ps.origin ) + range;
		mins = VEC2GLM( ent->client->ps.origin ) - range;

		num = trap_EntitiesInBox( &mins[0], &maxs[0], entityList, MAX_GENTITIES );
		for ( i = 0; i < num; i++ )
		{
			other = &g_entities[ entityList[ i ] ];

			if ( ( other->client && other->client->pers.team == TEAM_HUMANS ) ||
				( other->s.eType == entityType_t::ET_BUILDABLE && other->buildableTeam == TEAM_HUMANS ) )
			{
				return false;
			}
		}

		if ( !G_ActiveOvermind() )
		{
			return false;
		}

		evolveInfo = BG_ClassEvolveInfoFromTo( currentClass, newClass );

		if ( G_RoomForClassChange( ent, newClass, &infestOrigin[0] ) )
		{
			ent->client->pers.evolveHealthFraction =
				Math::Clamp( Entities::HealthFraction(ent), 0.0f, 1.0f );

			//remove credit
			G_AddCreditToClient( ent->client, -( short )evolveInfo.evolveCost, true );
			ent->client->pers.classSelection = newClass;
			BotSetNavmesh( ent, newClass );
			ClientUserinfoChanged( clientNum, false );
			VectorCopy( infestOrigin, ent->s.pos.trBase );
			ClientSpawn( ent, ent, ent->s.pos.trBase, ent->s.apos.trBase );

			//trap_SendServerCommand( -1, va( "print \"evolved to %s\n\"", classname) );

			return true;
		}
	}
	return false;
}

//Cmd_Buy_f ripoff, weapon version
bool BotBuyWeapon( gentity_t *self, weapon_t weapon )
{
	if ( weapon == WP_NONE )
	{
		return false;
	}

	//already got this?
	if ( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
	{
		return true;
	}

	if ( BG_Weapon( weapon )->team != G_Team( self ) )
	{
		return false;
	}

	//are we /allowed/ to buy this?
	if ( !BG_Weapon( weapon )->purchasable )
	{
		return false;
	}

	equipment_t<weapon_t>* wp = std::find( std::begin( weapons ), std::end( weapons ), weapon );
	if ( wp == std::end( weapons ) )
	{
		Log::Warn( "invalid weapon requested" );
		return false;
	}

	if ( !wp->canBuyNow() )
	{
		return false;
	}

	//can afford this?
	if ( BG_Weapon( weapon )->price > ( short )self->client->pers.credit )
	{
		return false;
	}

	//have space to carry this?
	if ( BG_Weapon( weapon )->slots & BG_SlotsForInventory( self->client->ps.stats ) )
	{
		return false;
	}

	// In some instances, weapons can't be changed
	if ( !BG_PlayerCanChangeWeapon( &self->client->ps ) )
	{
		return false;
	}

	self->client->ps.stats[ STAT_WEAPON ] = weapon;
	G_GiveMaxAmmo( self );
	G_ForceWeaponChange( self, weapon );

	//set build delay/pounce etc to 0
	self->client->ps.stats[ STAT_MISC ] = 0;
	self->client->ps.weaponCharge = 0;

	//subtract from funds
	G_AddCreditToClient( self->client, -( short )BG_Weapon( weapon )->price, false );

	//update ClientInfo
	ClientUserinfoChanged( self->client->ps.clientNum, false );
	return true;
}

bool BotBuyUpgrade( gentity_t *self, upgrade_t upgrade )
{
	if ( upgrade == UP_NONE )
	{
		return false;
	}

	//already got this?
	if ( BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ) )
	{
		return true;
	}

	//can afford this?
	if ( BG_Upgrade( upgrade )->price > ( short )self->client->pers.credit )
	{
		return false;
	}

	//have space to carry this?
	if ( BG_Upgrade( upgrade )->slots & BG_SlotsForInventory( self->client->ps.stats ) )
	{
		return false;
	}

	if ( BG_Upgrade( upgrade )->team != G_Team( self ) )
	{
		return false;
	}

	//are we /allowed/ to buy this?
	if ( !BG_Upgrade( upgrade )->purchasable )
	{
		return false;
	}

	equipment_t<upgrade_t>* up;
	up = std::find( std::begin( armors ), std::end( armors ), upgrade );
	if ( up == std::end( armors ) )
	{
		up = std::find( std::begin( others ), std::end( others ), upgrade );
		if ( up == std::end( others ) )
		{
			Log::Warn( "invalid upgrade requested" );
			return false;
		}
	}

	if ( !up->canBuyNow() )
	{
		return false;
	}

	glm::vec3 newOrigin = {};
	struct
	{
		upgrade_t upg;
		class_t cls;
	} armorToClass[] =
	{
		{ UP_LIGHTARMOUR, PCL_HUMAN_LIGHT },
		{ UP_MEDIUMARMOUR, PCL_HUMAN_MEDIUM },
		{ UP_BATTLESUIT, PCL_HUMAN_BSUIT },
	};

	for ( auto const& armor : armorToClass )
	{
		//fail if there's not enough space
		if ( upgrade == armor.upg && !BotChangeClass( self, armor.cls, newOrigin ) )
		{
			return false;
		}
	}

	//add to inventory
	BG_AddUpgradeToInventory( upgrade, self->client->ps.stats );

	//subtract from funds
	G_AddCreditToClient( self->client, -( short )BG_Upgrade( upgrade )->price, false );

	//update ClientInfo
	ClientUserinfoChanged( self->client->ps.clientNum, false );
	return true;
}

void BotSellWeapons( gentity_t *self )
{
	weapon_t selected = BG_GetPlayerWeapon( &self->client->ps );
	int i;

	//no armoury nearby
	if ( !G_BuildableInRange( self->client->ps.origin, ENTITY_USE_RANGE, BA_H_ARMOURY ) )
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
			G_AddCreditToClient( self->client, ( short )BG_Weapon( ( weapon_t ) i )->price, false );
		}

		//if we have this weapon selected, force a new selection
		if ( i == selected )
		{
			G_ForceWeaponChange( self, WP_NONE );
		}
	}
}
void BotSellUpgrades( gentity_t *self )
{
	int i;

	//no armoury nearby
	if ( !G_BuildableInRange( self->client->ps.origin, ENTITY_USE_RANGE, BA_H_ARMOURY ) )
	{
		return;
	}

	//sell upgrades
	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		//remove upgrade if carried
		if ( BG_InventoryContainsUpgrade( i, self->client->ps.stats ) &&
			BG_Upgrade( ( upgrade_t )i )->purchasable )
		{

			// shouldn't really need to test for this, but just to be safe
			if ( i == UP_LIGHTARMOUR || i == UP_MEDIUMARMOUR || i == UP_BATTLESUIT )
			{
				glm::vec3 newOrigin = {};

				if ( !G_RoomForClassChange( self, PCL_HUMAN_NAKED, &newOrigin[0] ) )
				{
					continue;
				}
				VectorCopy( newOrigin, self->client->ps.origin );
				self->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN_NAKED;
				self->client->pers.classSelection = PCL_HUMAN_NAKED;
				self->client->ps.eFlags ^= EF_TELEPORT_BIT;
				BotSetNavmesh( self, PCL_HUMAN_NAKED );
			}

			//remove from inventory
			BG_RemoveUpgradeFromInventory( i, self->client->ps.stats );

			//add to funds
			G_AddCreditToClient( self->client, ( short )BG_Upgrade( ( upgrade_t )i )->price, false );
		}
	}
	//update ClientInfo
	ClientUserinfoChanged( self->client->ps.clientNum, false );
}

void BotSetSkillLevel( gentity_t *self, int skill )
{
	self->botMind->botSkill.level = skill;
	// TODO: different aim for different teams
	self->botMind->botSkill.aimSlowness = ( float ) skill / 10;
	self->botMind->botSkill.aimShake = 10 - skill;
}

void BotResetEnemyQueue( enemyQueue_t *queue )
{
	queue->front = 0;
	queue->back = 0;
	memset( queue->enemys, 0, sizeof( queue->enemys ) );
}

static void BotPushEnemy( enemyQueue_t *queue, gentity_t *enemy )
{
	if ( enemy )
	{
		if ( ( queue->back + 1 ) % MAX_ENEMY_QUEUE != queue->front )
		{
			queue->enemys[ queue->back ].ent = enemy;
			queue->enemys[ queue->back ].timeFound = level.time;
			queue->back = ( queue->back + 1 ) % MAX_ENEMY_QUEUE;
		}
	}
}

static gentity_t *BotPopEnemy( enemyQueue_t *queue )
{
	// queue empty
	if ( queue->front == queue->back )
	{
		return nullptr;
	}

	if ( level.time - queue->enemys[ queue->front ].timeFound >= g_bot_reactiontime.Get() )
	{
		gentity_t *ret = queue->enemys[ queue->front ].ent;
		queue->front = ( queue->front + 1 ) % MAX_ENEMY_QUEUE;
		return ret;
	}

	return nullptr;
}

void BotPain( gentity_t *self, gentity_t *attacker, int )
{
	if ( G_Team( attacker ) != TEAM_NONE
		&& !G_OnSameTeam( self, attacker )
		&& attacker->s.eType == entityType_t::ET_PLAYER )
	{

		BotPushEnemy( &self->botMind->enemyQueue, attacker );
	}
}

void BotSearchForEnemy( gentity_t *self )
{
	gentity_t *enemy = BotFindBestEnemy( self );
	enemyQueue_t *queue = &self->botMind->enemyQueue;
	BotPushEnemy( queue, enemy );

	do
	{
		enemy = BotPopEnemy( queue );
	} while ( enemy && !BotEntityIsValidEnemyTarget( self, enemy ) );

	self->botMind->bestEnemy.ent = enemy;

	if ( self->botMind->bestEnemy.ent )
	{
		self->botMind->bestEnemy.distance = Distance( self->s.origin, self->botMind->bestEnemy.ent->s.origin );
	}
	else
	{
		self->botMind->bestEnemy.distance = std::numeric_limits<float>::max();
	}
}

void BotResetStuckTime( gentity_t *self )
{
	self->botMind->stuckTime = level.time;
	self->botMind->stuckPosition = VEC2GLM( self->client->ps.origin );
}

void BotCalculateStuckTime( gentity_t *self )
{
	// last think time condition to avoid stuck condition after respawn or /pause
	bool dataValid = level.time - self->botMind->lastThink < 1000;
	if ( !dataValid
			|| glm::distance2( self->botMind->stuckPosition, VEC2GLM( self->client->ps.origin ) ) >= Square( BOT_STUCK_RADIUS ) )
	{
		BotResetStuckTime( self );
	}
	self->botMind->lastThink = level.time;
}

/*
========================
botTarget_t methods implementations
========================
 */

botTarget_t& botTarget_t::operator=(const gentity_t *newTarget) {
	if (newTarget == nullptr) {
		this->clear();
		return *this;
	}

	ent = newTarget;
	coord = glm::vec3();
	type = targetType::ENTITY;

	if (!targetsValidEntity())
	{
		Log::Warn("bot: selecting invalid entity as target");
	}

	return *this;
}

botTarget_t& botTarget_t::operator=( glm::vec3 newTarget )
{
	coord = newTarget;
	ent = nullptr;
	type = targetType::COORDS;
	return *this;
}

void botTarget_t::clear() {
	ent = nullptr;
	coord = glm::vec3();
	type = targetType::EMPTY;
}

entityType_t botTarget_t::getTargetType() const
{
	if ( targetsValidEntity() )
	{
		return ent->s.eType;
	}
	else
	{
		return Util::enum_cast<entityType_t>(-1);
	}
}


bool botTarget_t::isValid() const
{
	if (targetsCoordinates())
		return true;
	if (targetsValidEntity())
		return true;
	return false;
}

bool botTarget_t::targetsCoordinates() const
{
	return type == targetType::COORDS;
}

bool botTarget_t::targetsValidEntity() const
{
	if (ent) {
		return BotEntityIsValidTarget(ent.get());
	}
	return false;
}

const gentity_t *botTarget_t::getTargetedEntity() const
{
	return ent.get();
}

glm::vec3 botTarget_t::getPos() const
{
	if ( type == targetType::ENTITY && ent )
	{
		return VEC2GLM( ent->s.origin );
	}
	if ( type == targetType::COORDS )
	{
		return coord;
	}
	Log::Warn("Bot: couldn't get position of target");
	return glm::vec3();
}


// Reimplementation of Daemon's ProjectPointOntoVector.
// Projects `point` onto the line passing through linePoint1 and linePoint2.
glm::vec3 ProjectPointOntoVector( const glm::vec3 &point, const glm::vec3 &linePoint1, const glm::vec3 &linePoint2 )
{
	glm::vec3 pointRelative = point - linePoint1;
	glm::vec3 lineDir = glm::normalize( linePoint2 - linePoint1 );
	return linePoint1 + glm::dot( pointRelative, lineDir ) * lineDir;
}
