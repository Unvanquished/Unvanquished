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

#include "sg_bot_util.h"
#include "../shared/parse.h"

static std::array<std::set<std::string>, 9> baseSkillset;
static std::set<std::string> disabledSkillset;

static void G_UpdateSkillsets()
{
	for ( int i = 0; i < level.maxclients; i++ )
	{
		gentity_t *bot = &g_entities[ i ];
		if ( bot->r.svFlags & SVF_BOT )
		{
			BotSetSkillLevel( bot, bot->botMind->skillLevel );
		}
	}
}

static std::set<std::string> G_ParseSkillsetList( Str::StringRef skillsCsv )
{
	std::set<std::string> skills;

	for (Parse_WordListSplitter i(skillsCsv); *i; ++i)
	{
		skills.insert( *i );
	}

	return skills;
}

static std::set<std::pair<int, std::string>> G_ParseSkillsetListWithLevels( Str::StringRef skillsCsv )
{
	std::set<std::pair<int, std::string>> skills;

	for (Parse_WordListSplitter i(skillsCsv); *i; ++i)
	{
		// Try to read a "n:skillname" pair where n represents a single digit
		if ( strnlen(*i, 3) < 3 || !Str::cisdigit( (*i)[0] ) || (*i)[1] != ':' )
		{
			Log::Warn("Invalid \"int:skill\" pair: " QQ("%s"), *i);
			continue;
		}
		int number = (*i)[0] - '0';
		std::string skillName = *i + 2; // skip the first two character
		skills.insert( { number, std::move(skillName) } );
	}

	return skills;
}

static void G_SetDisabledSkillset( Str::StringRef skillsCsv )
{
	disabledSkillset = G_ParseSkillsetList( skillsCsv );
	G_UpdateSkillsets();
}

static bool G_SkillDisabled( Str::StringRef behavior )
{
	return disabledSkillset.find( behavior ) != disabledSkillset.end();
}

static void G_SetBaseSkillset( Str::StringRef skillsCsv )
{
	std::set<std::pair<int, std::string>> skills = G_ParseSkillsetListWithLevels( skillsCsv );
	for ( int skillLevel = 1; skillLevel <= 9; skillLevel++ )
	{
		std::set<std::string> level;
		for ( auto &skill : skills )
		{
			if (skill.first <= skillLevel)
			{
				level.insert(skill.second);
			}
		}
		baseSkillset[skillLevel - 1] = std::move(level);
	}
	G_UpdateSkillsets();
}

static bool G_IsBaseSkillAtLevel( int skillLevel, Str::StringRef behavior )
{
	ASSERT( skillLevel >= 1 && skillLevel <= 9 );
	return baseSkillset[skillLevel - 1].find( behavior ) != baseSkillset[skillLevel - 1].end();
}

// aliens have 71 points to spend max, but we give them a bit less for balancing
static int skillsetBudgetAliens = 63;
// humans have 48 points to spend max
static int skillsetBudgetHumans = 48;

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
	static Cvar::Callback<Cvar::Cvar<std::string>> g_bot_skillset_disabledSkills(
		"g_bot_skillset_disabledSkills",
		"Skills that will not be selected randomly for bots, example: " QQ("mantis-attack-jump, prefer-armor"),
		Cvar::NONE,
		"",
		G_SetDisabledSkillset
		);

	static Cvar::Callback<Cvar::Cvar<std::string>> g_bot_skillset_baseSkills(
		"g_bot_skillset_baseSkills",
		"Preferred skills for bots depending on levels, this is a level:skillName key:value list, example: " QQ("6:mantis-attack-jump, 3:prefer-armor"),
		Cvar::NONE,
		"",
		G_SetBaseSkillset
		);

	static Cvar::Callback<Cvar::Cvar<int>> g_bot_skillset_budgetAliens(
		"g_bot_skillset_budgetAliens",
		"How many skillpoint for bot aliens' random skills. Base skill costs are also removed from this sum",
		Cvar::NONE,
		skillsetBudgetAliens,
		G_SetSkillsetBudgetAliens
		);
	static Cvar::Callback<Cvar::Cvar<int>> g_bot_skillset_budgetHumans(
		"g_bot_skillset_budgetHumans",
		"How many skillpoint for bot humans' random skills. Base skill costs are also removed from this sum",
		Cvar::NONE,
		skillsetBudgetHumans,
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

	// aliens
	{ "aim-head",           BOT_A_AIM_HEAD,                10, TEAM_ALIENS, needs_one_of({BOT_B_BASIC_FIGHT}) },
	{ "aim-barbs",          BOT_A_AIM_BARBS,               7,  TEAM_ALIENS, needs_one_of({BOT_B_BASIC_FIGHT}) },

	// humans
	{ "predict-aim",        BOT_H_PREDICTIVE_AIM,          5,  TEAM_HUMANS, needs_one_of({BOT_B_BASIC_FIGHT}) },
};

static bool SkillIsAvailable(const botSkillTreeElement_t &skill, team_t team, skillSet_t activated_skills)
{
	return !G_SkillDisabled(skill.name)
		&& (skill.allowed_teams == TEAM_NONE || skill.allowed_teams == team) // and correspond to our team
		&& (skill.prerequisite == 0 || (activated_skills & skill.prerequisite) != 0); // and has one of the prerequisites matching, if any
}

// Note: this function modifies possible_choices to remove the one we just chose.
static Util::optional<botSkillTreeElement_t> ChooseOneSkill(team_t team, skillSet_t skillSet, std::vector<botSkillTreeElement_t> &possible_choices, std::mt19937_64& rng, int skillLevel)
{
	int total_skill_points = 0;
	for (auto skill = possible_choices.begin(); skill != possible_choices.end(); skill++)
	{
		// force base skills to be selected first (if the the skill is for the correct team)
		if (G_IsBaseSkillAtLevel(skillLevel, skill->name) && (skill->allowed_teams == TEAM_NONE || skill->allowed_teams == team))
		{
			// force this to be selected first
			botSkillTreeElement_t choice = *skill;
			possible_choices.erase(skill);
			return choice;
		}
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
std::vector<botSkillTreeElement_t> BotPickSkillset(std::string seed, int skillLevel, team_t team)
{
	std::vector<botSkillTreeElement_t> possible_choices = skillTree;
	skillSet_t skillSet(0);

	float max = team == TEAM_ALIENS ? static_cast<float>( skillsetBudgetAliens ) : static_cast<float>( skillsetBudgetHumans );

	// unlock every skill at skill 7 (almost every for aliens)
	int skill_points = static_cast<float>(skillLevel + 2) / 9.0f * max;

	// rng preparation
	std::seed_seq seed_seq(seed.begin(), seed.end());
	std::mt19937_64 rng(seed_seq);

	std::vector<botSkillTreeElement_t> results;
	while (true)
	{
		Util::optional<botSkillTreeElement_t> new_skill =
			ChooseOneSkill(team, skillSet, possible_choices, rng, skillLevel);
		if ( !new_skill )
		{
			// no skill left to unlock
			break;
		}

		skill_points -= new_skill->cost;

		// force base skills to always be selected, no matter the cost
		bool isBaseSkill = G_IsBaseSkillAtLevel( skillLevel, new_skill->name );

		if ( skill_points < 0 && !isBaseSkill )
		{
			// we don't have money to spend anymore, we are done
			continue;
		}

		skillSet[new_skill->skill] = true;
		results.push_back(*new_skill);
	}
	return results;
}

std::pair<std::string, skillSet_t> BotDetermineSkills(gentity_t *bot, int skillLevel)
{
	std::string seed = bot->client->pers.netname;

	skillSet_t skillSet(0);

	std::vector<std::string> skillNames;

	for ( const auto &skill : BotPickSkillset(seed, skillLevel, G_Team(bot)) )
	{
		skillSet[skill.skill] = true;
		skillNames.push_back(skill.name);
	}

	// pretty list string
	std::sort( skillNames.begin(), skillNames.end() );
	std::string skill_list;
	for ( auto const& skill_ : skillNames )
	{
		if (!skill_list.empty())
		{
			skill_list.append(" ");
		}
		skill_list.append( skill_ );
	}
	return { skill_list, skillSet };
}

// This function is used for debug only
std::string G_BotPrintSkillGraph(team_t team, int skillLevel)
{
	std::string output;

	output += "digraph skilltree {\n";

	if (skillLevel)
	{
		output += "graph [labelloc=\"b\" labeljust=\"r\" label=\"\n";
		output += Str::Format("percentages are skill selection chance for skillLevel %i\\l\n", skillLevel);
		output += "green is for force-enabled skills (g_skillset_baseSkills)\\l";
		output += "red is for force-disabled skills (g_skillset_disabledSkills)\\l\"];\n";
	}

	std::array<int, BOT_NUM_SKILLS> counts = {};
	constexpr int N = 10000; // number of tries to smooth the values
	if (skillLevel)
	{
		for (int i = 0; i<N; i++)
		{
			std::string seed = std::to_string(i);

			for ( const auto &skill : BotPickSkillset(seed, skillLevel, team) )
			{
				counts[skill.skill]++;
			}
		}
	}

	for (const auto &skill : skillTree)
	{
		if (skill.allowed_teams != TEAM_NONE && skill.allowed_teams != team)
		{
			continue;
		}
		output += "\ta";
		output += std::to_string(skill.skill);
		output += "[label=\"";
		output += skill.name;
		if (skillLevel)
		{
			output += "\n";
			float percent = 100.0f * counts[skill.skill] / N;
			output += Str::Format("%.1f%%", percent);
		}
		output += "\"";

		if (skillLevel)
		{
			if (G_IsBaseSkillAtLevel(skillLevel, skill.name))
			{
				output += " style=filled fillcolor=\"#aaeeaa\"";
			}
			else if (G_SkillDisabled(skill.name))
			{
				output += " style=filled fillcolor=\"#eeaaaa\"";
			}
		}
		output += "];\n";
	}
	output += "\n";

	for (const auto &skill : skillTree)
	{
		if (skill.allowed_teams != TEAM_NONE && skill.allowed_teams != team)
		{
			continue;
		}

		for (size_t i=0; i < skill.prerequisite.size(); i++)
		{
			if (skill.prerequisite[i]) {
				output += "\ta";
				output += std::to_string(i);
				output += " -> a";
				output += std::to_string(skill.skill);
				output += ";\n";
			}
		}
	}

	output += "}\n";

	return output;
}

// This function is used for debug only
int G_BotCountSkillPoints(team_t team)
{
	// rng preparation
	std::mt19937_64 rng;

	std::vector<botSkillTreeElement_t> possible_choices = skillTree;
	skillSet_t skillSet(0);

	int total = 0;
	Util::optional<botSkillTreeElement_t> new_skill;
	while (!possible_choices.empty()
			&& (new_skill = ChooseOneSkill(team, skillSet, possible_choices, rng, /*skillLevel=*/5)))
	{
		total += new_skill->cost;
		skillSet[new_skill->skill] = true;
	}

	return total;
}
