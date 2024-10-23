/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2022 Unvanquished Developers

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

#include "common/Common.h"
#include "sg_bot_util.h"
#include "../shared/parse.h"

static std::array<skillSet_t, 9> baseSkillset;
static skillSet_t disabledSkillset;

static std::string SkillSetToString(skillSet_t skillSet, const std::string& separator);

static void G_UpdateSkillsets()
{
	for ( int i = 0; i < level.maxclients; i++ )
	{
		gentity_t *bot = &g_entities[ i ];
		if ( bot->inuse && bot->client->pers.isBot )
		{
			BotSetSkillLevel( bot, bot->botMind->skillLevel );
		}
	}
}

static bool ParseSkill( Str::StringRef name, bot_skill &skill );

static skillSet_t G_ParseSkillsetList( Str::StringRef skillsCsv )
{
	skillSet_t skills;

	for (Parse_WordListSplitter i(skillsCsv); *i; ++i)
	{
		bot_skill skill;
		if ( ParseSkill ( *i, skill ) )
		{
			skills.set( skill );
		}
		else
		{
			Log::Warn( "unknown skill '%s'", *i );
		}
	}

	return skills;
}

static std::vector<std::pair<int, bot_skill>> G_ParseSkillsetListWithLevels( Str::StringRef skillsCsv )
{
	std::vector<std::pair<int, bot_skill>> skills;

	for (Parse_WordListSplitter i(skillsCsv); *i; ++i)
	{
		// Try to read a "n:skillname" pair where n represents a single digit
		if ( strnlen(*i, 3) < 3 || !Str::cisdigit( (*i)[0] ) || (*i)[1] != ':' )
		{
			Log::Warn("Invalid \"int:skill\" pair: " QQ("%s"), *i);
			continue;
		}
		int number = (*i)[0] - '0';
		bot_skill skill;
		if ( !ParseSkill( *i + 2, skill ) )
		{
			Log::Warn( "unknown skill '%s' in g_bot_baseSkillset", *i + 2 );
			continue;
		}
		skills.emplace_back( number, skill );
	}

	return skills;
}

static void G_SetDisabledSkillset( Str::StringRef skillsCsv )
{
	disabledSkillset = G_ParseSkillsetList( skillsCsv );
	G_UpdateSkillsets();
}

static void CheckBaseSkillset();
static void G_SetBaseSkillset( Str::StringRef skillsCsv )
{
	std::vector<std::pair<int, bot_skill>> skills = G_ParseSkillsetListWithLevels( skillsCsv );
	for ( int skillLevel = 1; skillLevel <= 9; skillLevel++ )
	{
		skillSet_t level{};
		for ( auto &skill : skills )
		{
			if (skill.first <= skillLevel)
			{
				level.set( skill.second );
			}
		}
		baseSkillset[ skillLevel - 1 ] = level;
	}

	CheckBaseSkillset();
	G_UpdateSkillsets();
}

static bool G_IsBaseSkillAtLevel( int skillLevel, bot_skill skill )
{
	ASSERT( skillLevel >= 1 && skillLevel <= 9 );
	return baseSkillset[ skillLevel - 1 ][ skill ];
}

static int skillsetBudgetAliens;
static int skillsetBudgetHumans;

static void G_SetSkillsetBudgetHumans( int val )
{
	skillsetBudgetHumans = val;
	G_UpdateSkillsets();
}

static void G_SetSkillsetBudgetAliens( int val )
{
	skillsetBudgetAliens = val;
	G_UpdateSkillsets();
}

void G_InitSkilltreeCvars()
{
	static Cvar::Callback<Cvar::Cvar<std::string>> g_disabledSkillset(
		"g_bot_skillset_disabledSkills",
		"Skills that will not be selected randomly for bots, example: " QQ("mantis-attack-jump, prefer-armor"),
		Cvar::NONE,
		"",
		G_SetDisabledSkillset
		);

	static Cvar::Callback<Cvar::Cvar<std::string>> g_skillset_baseSkills(
		"g_bot_skillset_baseSkills",
		"Preferred skills for bots depending on levels, this is a level:skillName key:value list, example: " QQ("6:mantis-attack-jump, 3:prefer-armor"),
		Cvar::NONE,
		"1:movement 1:fighting 1:feels-pain 1:buy-modern-armor 1:medkit "
		"3:strafe-attack "
		"5:aim-barbs 5:mantis-attack-jump 5:small-attack-jump 5:mara-attack-jump 5:goon-attack-jump 5:tyrant-attack-run 5:a-fast-flee 5:h-fast-flee 5:prefer-armor 5:lcannon-tricks "
		"7:aim-head 7:predict-aim 7:safe-barbs "
		, // unused: fast-aim
		G_SetBaseSkillset
		);
	G_SetBaseSkillset( g_skillset_baseSkills.Get() ); // there's no cvar modification event if the cvar is unset on gamelogic load

	static Cvar::Callback<Cvar::Cvar<int>> g_skillsetBudgetAliens(
		"g_bot_skillset_budgetAliens",
		"How many skillpoint for bot aliens' random skills. Base skill costs are also removed from this sum",
		Cvar::NONE,
		0,
		G_SetSkillsetBudgetAliens
		);
	static Cvar::Callback<Cvar::Cvar<int>> g_skillsetBudgetHumans(
		"g_bot_skillset_budgetHumans",
		"How many skillpoint for bot humans' random skills. Base skill costs are also removed from this sum",
		Cvar::NONE,
		0,
		G_SetSkillsetBudgetHumans
		);
}

struct botSkillTreeElement_t {
	Str::StringRef name;
	bot_skill skill;
	int cost;
	team_t allowed_teams; // TEAM_NONE means that the skill is available for every team
	skillSet_t prerequisite; // This skill needs ONE OF those prerequisite to be selectable
};

// used as a convenience function to build the "prerequisite" bitset
static skillSet_t needs_one_of(std::initializer_list<bot_skill> skills)
{
	skillSet_t sum(0);
	for (bot_skill skill : skills)
	{
		sum[skill] = true;
	}
	return sum;
}


static const std::vector<botSkillTreeElement_t> skillTree =
{
	////
	// Basic "categories". They bring no or almost no benefit. They
	// are here to encourage the bot to specialise in a "field"
	////

	// movement skills
	{ "movement",           BOT_B_BASIC_MOVEMENT,          2,  TEAM_NONE, 0 },
	// fighting skills
	{ "fighting",           BOT_B_BASIC_FIGHT,             3,  TEAM_NONE, 0 },
	// situation awareness and survival
	{ "feels-pain",         BOT_B_PAIN,                    2,  TEAM_NONE, 0 },


	////
	// Movement skills
	////

	// aliens
	{ "strafe-attack",      BOT_A_STRAFE_ON_ATTACK,        6,  TEAM_ALIENS, needs_one_of({BOT_B_BASIC_MOVEMENT}) },
	{ "small-attack-jump",  BOT_A_SMALL_JUMP_ON_ATTACK,    5,  TEAM_ALIENS, needs_one_of({BOT_B_BASIC_MOVEMENT}) },
	{ "mara-attack-jump",   BOT_A_MARA_JUMP_ON_ATTACK,     5,  TEAM_ALIENS, needs_one_of({BOT_B_BASIC_MOVEMENT}) },
	{ "mantis-attack-jump", BOT_A_LEAP_ON_ATTACK,          3,  TEAM_ALIENS, needs_one_of({BOT_B_BASIC_MOVEMENT}) },
	{ "goon-attack-jump",   BOT_A_POUNCE_ON_ATTACK,        5,  TEAM_ALIENS, needs_one_of({BOT_B_BASIC_MOVEMENT}) },
	{ "tyrant-attack-run",  BOT_A_TYRANT_CHARGE_ON_ATTACK, 5,  TEAM_ALIENS, needs_one_of({BOT_B_BASIC_MOVEMENT}) },


	////
	// Survival skills
	////

	// aliens
	{ "safe-barbs",         BOT_A_SAFE_BARBS,              3,  TEAM_ALIENS, needs_one_of({BOT_B_PAIN}) },
	{ "a-fast-flee",        BOT_A_FAST_FLEE,               7,  TEAM_ALIENS, needs_one_of({BOT_B_PAIN}) },

	// humans
	{ "buy-modern-armor",   BOT_H_BUY_MODERN_ARMOR,        10, TEAM_HUMANS, needs_one_of({BOT_B_PAIN}) },
	{ "prefer-armor",       BOT_H_PREFER_ARMOR,            5,  TEAM_HUMANS, needs_one_of({BOT_H_BUY_MODERN_ARMOR}) },
	{ "h-fast-flee",        BOT_H_FAST_FLEE,               5,  TEAM_HUMANS, needs_one_of({BOT_B_PAIN}) },
	{ "medkit",             BOT_H_MEDKIT,                  8,  TEAM_HUMANS, needs_one_of({BOT_B_PAIN}) },


	////
	// Fighting skills
	////

	{ "fast-aim",           BOT_B_FAST_AIM,                8,  TEAM_NONE,   needs_one_of({BOT_B_BASIC_FIGHT}) },
	{ "lcannon-tricks",     BOT_H_LCANNON_TRICKS,          3,  TEAM_HUMANS, needs_one_of({BOT_B_BASIC_FIGHT}) },

	// aliens
	{ "aim-head",           BOT_A_AIM_HEAD,                10, TEAM_ALIENS, needs_one_of({BOT_B_BASIC_FIGHT}) },
	{ "aim-barbs",          BOT_A_AIM_BARBS,               7,  TEAM_ALIENS, needs_one_of({BOT_B_BASIC_FIGHT}) },

	// humans
	{ "predict-aim",        BOT_H_PREDICTIVE_AIM,          5,  TEAM_HUMANS, needs_one_of({BOT_B_BASIC_FIGHT}) },
};

static bool ParseSkill( Str::StringRef name, bot_skill &skill )
{
	for ( const botSkillTreeElement_t &s : skillTree )
	{
		if ( name == s.name )
		{
			skill = s.skill;
			return true;
		}
	}
	return false;
}

static bool SkillIsAvailable(const botSkillTreeElement_t &skill, team_t team, skillSet_t activated_skills)
{
	return !disabledSkillset[ skill.skill ]
		&& (skill.allowed_teams == TEAM_NONE || skill.allowed_teams == team) // and correspond to our team
		&& (skill.prerequisite == 0 || (activated_skills & skill.prerequisite) != 0); // and has one of the prerequisites matching, if any
}

// Note: this function modifies possible_choices to remove the one we just chose.
static Util::optional<botSkillTreeElement_t> ChooseOneSkill(team_t team, skillSet_t skillSet, std::vector<botSkillTreeElement_t> &possible_choices, std::mt19937_64& rng)
{
	int total_skill_points = 0;
	for (auto skill = possible_choices.begin(); skill != possible_choices.end(); skill++)
	{
		if (SkillIsAvailable(*skill, team, skillSet))
		{
			total_skill_points += skill->cost;
		}
	}

	if (total_skill_points <= 0)
		return Util::nullopt;

	std::uniform_int_distribution<int> dist(0, total_skill_points - 1);
	int random = dist(rng);

	// find which skill this corresponds to (higher point skills are more
	// likely to be found)
	int cursor = 0;
	for (auto skill = possible_choices.begin(); skill != possible_choices.end(); skill++)
	{
		if (!SkillIsAvailable(*skill, team, skillSet))
		{
			continue;
		}

		cursor += skill->cost;
		if (random < cursor)
		{
			botSkillTreeElement_t choice = *skill;
			possible_choices.erase(skill);
			return choice;
		}
	}
	return Util::nullopt;
}

// Returns the base skill set according to g_bot_baseSkills and the given level and team.
// Removes selected skills from `choices` and subtracts points used from `points`.
static skillSet_t ChooseBaseSkills( int skillLevel, team_t team, std::vector<botSkillTreeElement_t> &choices, int &points)
{
	skillSet_t skills{};
	for ( size_t i = 0; i < choices.size(); )
	{
		if ( G_IsBaseSkillAtLevel( skillLevel, choices[ i ].skill ) &&
		     ( choices[ i ].allowed_teams == TEAM_NONE || choices[ i ].allowed_teams == team ) )
		{
			skills.set( choices[ i ].skill );
			points -= choices[ i ].cost;
			choices[ i ] = std::move( choices.back() );
			choices.pop_back();
		}
		else
		{
			++i;
		}
	}
	return skills;
}

// This selects a set of skills for a bot.
//
// The algorithm is stable and deterministic.
// Deterministic:
//     a bot's skill set is determined only by the bot's name and the bot's
//     skill level. A different bot will have a different skillset, but this
//     means that a bot with the same name will have the same skillset from
//     a game to another.
// Stable:
//     when a bot changes skill level, the first skills unlocked are common
//     between both skill levels. This means that a bot whose skill is raised
//     by BotSetSkillLevel will keep the skills it already had, and get new
//     ones.
//     This essentially means that a bot will keep its "personality" after
//     a skill change, although keeping its base skills or unlocking new ones.
skillSet_t BotPickSkillset(std::string seed, int skillLevel, team_t team)
{
	std::vector<botSkillTreeElement_t> possible_choices = skillTree;

	float max = team == TEAM_ALIENS ? static_cast<float>( skillsetBudgetAliens ) : static_cast<float>( skillsetBudgetHumans );

	// unlock every skill at skill 7 (almost every for aliens)
	int skill_points = static_cast<float>(skillLevel + 2) / 9.0f * max;

	skillSet_t skillSet = ChooseBaseSkills( skillLevel, team, possible_choices, skill_points );

	// rng preparation
	std::seed_seq seed_seq(seed.begin(), seed.end());
	std::mt19937_64 rng(seed_seq);

	std::vector<botSkillTreeElement_t> results;
	while (true)
	{
		Util::optional<botSkillTreeElement_t> new_skill =
			ChooseOneSkill(team, skillSet, possible_choices, rng);
		if ( !new_skill )
		{
			// no skill left to unlock
			break;
		}

		skill_points -= new_skill->cost;

		if ( skill_points < 0 )
		{
			// we don't have money to spend anymore, we are done
			continue;
		}

		skillSet[new_skill->skill] = true;
		results.push_back(*new_skill);
	}
	return skillSet;
}

static std::string SkillSetToString( skillSet_t skillSet, const std::string &separator )
{
	std::vector<std::string> skillNames;

	for ( const botSkillTreeElement_t &s : skillTree )
	{
		if ( skillSet[ s.skill ] )
		{
			skillNames.push_back( s.name );
		}
	}

	// pretty list string
	std::sort( skillNames.begin(), skillNames.end() );
	std::string skill_list;
	for ( auto const& skill_ : skillNames )
	{
		if (!skill_list.empty())
		{
			skill_list.append( separator );
		}
		skill_list.append( skill_ );
	}
	return skill_list;
}

std::pair<std::string, skillSet_t> BotDetermineSkills(gentity_t *bot, int skillLevel)
{
	std::string seed = bot->client->pers.netname;
	skillSet_t skillSet = BotPickSkillset( seed, skillLevel, G_Team( bot ));
	return { SkillSetToString( skillSet, " " ), skillSet };
}

// Sometimes there is code like
// if (bot has basic skill) {
//     ...
//     if (bot has skill depending on basic skill) {
//         ...
//
// So warn the user if a skill has a missing dependency which means that the
// value of the skill may not really be seen
static void CheckBaseSkillset()
{
	for ( const botSkillTreeElement_t &skill : skillTree )
	{
		// Find the lowest level the skill is in and see if it has unset dependencies
		for ( int l = 1; l <= 9; l++ )
		{
			const skillSet_t &level = baseSkillset[ l - 1 ];
			if ( level[ skill.skill ] )
			{
				if ( skill.prerequisite.any() && ( skill.prerequisite & level ).none() )
				{
					std::string deps = SkillSetToString( skill.prerequisite, " / " );
					Log::Warn( "Base skillset for level %d includes %s, but includes none of its dependencies: %s",
						l, skill.name, deps );
				}
				break;
			}
		}
	}
}
