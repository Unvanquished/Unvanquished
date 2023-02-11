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

// aliens have 61 points to spend max
// humans have 40 points to spend max
Cvar::Cvar<int> g_skillsetBudgetAliens( "g_skillsetBudgetAliens", "the skillset budget for aliens.", Cvar::NONE, 61 );
Cvar::Cvar<int> g_skillsetBudgetHumans( "g_skillsetBudgetHumans", "the skillset budget for humans.", Cvar::NONE, 40 );

static std::set<std::string>& G_GetAllowedSkillset()
{
	static std::set<std::string> allowedSkillset;
	return allowedSkillset;
}

static std::set<std::string> G_ParseAllowedSkillsetList( const std::string &skillsCsv )
{
	std::set<std::string> skills;

	for (Parse_WordListSplitter i(skillsCsv); *i; ++i)
	{
		skills.insert( *i );
	}

	return skills;
}

static void G_SetAllowedSkillset( Str::StringRef skillsCsv )
{
	G_GetAllowedSkillset() = G_ParseAllowedSkillsetList( skillsCsv );
}

static bool G_SkillAllowed( Str::StringRef behavior )
{
	return G_GetAllowedSkillset().find( behavior ) != G_GetAllowedSkillset().end();
}

void G_InitSkilltreeCvars()
{
	std::string defaultSkillset = "mara-attack-jump, mantis-attack-jump, goon-attack-jump, tyrant-attack-run, mara-flee-jump, mantis-flee-jump, goon-flee-jump, tyrant-flee-run, safe-barbs, buy-modern-armor, prefer-armor, flee-run, medkit, aim-head, aim-barbs, predict-aim, movement, fighting, feels-pain";
	static Cvar::Callback<Cvar::Cvar<std::string>> g_allowedSkillset(
        "g_allowedSkillset",
		"Allowed skills for bots, example: " QQ("mantis-attack-jump, prefer-armor"),
		Cvar::NONE,
		defaultSkillset,
		G_SetAllowedSkillset
        );
}

struct botSkillTreeElement_t {
	Str::StringRef name;
	bot_skill skill;
	int cost;
	// function that determines if this skill is available
	std::function<bool(const gentity_t *self, skillSet_t existing_skills)> predicate;
	// skills that are unlocked once you unlock this one
	std::vector<botSkillTreeElement_t> unlocked_skills;
};

static bool pred_always(const gentity_t *self, skillSet_t existing_skills)
{
	Q_UNUSED(self);
	Q_UNUSED(existing_skills);
	return true;
}

static bool pred_alien(const gentity_t *self, skillSet_t existing_skills)
{
	Q_UNUSED(existing_skills);
	return G_Team(self) == TEAM_ALIENS;
}

static bool pred_human(const gentity_t *self, skillSet_t existing_skills)
{
	Q_UNUSED(existing_skills);
	return G_Team(self) == TEAM_HUMANS;
}

static const std::vector<botSkillTreeElement_t> movement_skills = {
	// aliens
	{ "mara-attack-jump",   BOT_A_MARA_JUMP_ON_ATTACK,   5, pred_alien, {} },
	{ "mantis-attack-jump", BOT_A_LEAP_ON_ATTACK,        4, pred_alien, {} },
	{ "goon-attack-jump",   BOT_A_POUNCE_ON_ATTACK,      5, pred_alien, {} },
	{ "tyrant-attack-run",  BOT_A_TYRANT_CHARGE_ON_ATTACK, 5, pred_alien, {} },
};

static const std::vector<botSkillTreeElement_t> survival_skills = {
	// aliens
	{ "mara-flee-jump",     BOT_A_MARA_JUMP_ON_FLEE,     4, pred_alien, {} },
	{ "mantis-flee-jump",   BOT_A_LEAP_ON_FLEE,          3, pred_alien, {} },
	{ "goon-flee-jump",     BOT_A_POUNCE_ON_FLEE,        4, pred_alien, {} },
	{ "tyrant-flee-run",    BOT_A_TYRANT_CHARGE_ON_FLEE, 4, pred_alien, {} },
	{ "safe-barbs",         BOT_A_SAFE_BARBS,            3, pred_alien, {} },

	// humans
	{ "buy-modern-armor",   BOT_H_BUY_MODERN_ARMOR, 10, pred_human, {
		{ "prefer-armor", BOT_H_PREFER_ARMOR, 5, pred_human, {} },
	}},
	{ "flee-run",           BOT_H_RUN_ON_FLEE,  5, pred_human, {} },
	{ "medkit",             BOT_H_MEDKIT,       8, pred_human, {} },
};

static const std::vector<botSkillTreeElement_t> fighting_skills = {
	// aliens
	{ "aim-head",           BOT_A_AIM_HEAD,      10, pred_alien, {} },
	{ "aim-barbs",          BOT_A_AIM_BARBS,     7,  pred_alien, {} },

	// humans
	{ "predict-aim",        BOT_H_PREDICTIVE_AIM, 5, pred_human, {} },
};

static const std::vector<botSkillTreeElement_t> initial_unlockable_skills = {
	// movement skills
	{ "movement", BOT_B_BASIC_MOVEMENT, 2, pred_always, movement_skills },
	// fighting skills
	{ "fighting", BOT_B_BASIC_FIGHT, 3, pred_always, fighting_skills },
	// situation awareness and survival
	{ "feels-pain", BOT_B_PAIN, 2, pred_always, survival_skills },
};

// Note: this function modifies possible_choices to remove the one we just
//       chose, and add the new unlocked skills to that list.
static Util::optional<botSkillTreeElement_t> ChooseOneSkill(const gentity_t *bot, skillSet_t skillSet, std::vector<botSkillTreeElement_t> &possible_choices, std::mt19937_64& rng)
{
	int total_skill_points = 0;
	for (const auto &skill : possible_choices)
	{
		if (skill.predicate(bot, skillSet))
		{
			total_skill_points += skill.cost;
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
		if (!skill->predicate(bot, skillSet))
		{
			continue;
		}

		cursor += skill->cost;
		if (random < cursor)
		{
			botSkillTreeElement_t result = *skill;

			// update the list of future choices
			possible_choices.erase(skill);
			for (const auto &child : result.unlocked_skills)
			{
				possible_choices.push_back(std::move(child));
			}
			result.unlocked_skills.clear();

			return result;
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
std::pair<std::string, skillSet_t> BotDetermineSkills(gentity_t *bot, int skill)
{
	std::vector<botSkillTreeElement_t> possible_choices = initial_unlockable_skills;

	float max = G_Team(bot) == TEAM_ALIENS ? static_cast<float>( g_skillsetBudgetAliens.Get() ) : static_cast<float>( g_skillsetBudgetHumans.Get() );

	// unlock every skill at skill 7
	int skill_points = static_cast<float>(skill) / 7.0f * max;

	// rng preparation
	std::string name = bot->client->pers.netname;
	std::seed_seq seed(name.begin(), name.end());
	std::mt19937_64 rng(seed);

	skillSet_t skillSet;
	std::string skill_list;

	while (!possible_choices.empty())
	{
		Util::optional<botSkillTreeElement_t> new_skill =
			ChooseOneSkill(bot, skillSet, possible_choices, rng);
		if ( !new_skill )
		{
			// no skill left to unlock
			break;
		}

		if ( !G_SkillAllowed( new_skill->name ) )
		{
			continue;
		}

		skill_points -= new_skill->cost;

		if ( skill_points < 0 )
		{
			// we don't have money to spend anymore, we are done
			break;
		}

		skillSet[new_skill->skill] = true;

		if (!skill_list.empty())
		{
			skill_list.append(" ");
		}
		skill_list.append(new_skill->name);
	}


	return { skill_list, skillSet };
}
