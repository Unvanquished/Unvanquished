/*
This file is part of Unvanquished.

Unvanquished is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rocketGameLogElement.h"

using Rocket::Core::Element;
using Rocket::Core::ElementList;

struct Event
{
	int time;
	gameLogEventType_t type;
	std::string args[4];
};

static std::deque<Event> eventQueue;

// FIXME: refactor this array into something less rigid
static const struct templateProperties {
	const char *name;
	const char *fields[4];
} templateTable[gameLogEventType_t::COUNT] =
{
	{
		"obituary",
		{"obituary-killer", "obituary-means", "obituary-victim", NULL}
	},
	{
		"beacon",
		{"beacon-icon", "beacon-text", NULL}
	}
};

// FIXME: move all of this to the constructor?
void RocketGameLogElement::Initialize(void)
{
	memset(templates, 0, sizeof(templates));
	
	for (size_t i = 0; i < gameLogEventType_t::COUNT; i++) {
		const templateProperties *temp = templateTable + i;
		std::string id;
		Element *element;

		id = Str::Format("%s-template", temp->name);

		element = GetElementById(id.c_str());
		if (!element) {
			Log::Warn("<gamelog> is missing '%s'", id);
		next_element:
			continue;
		}

		// make sure all needed nodes exist and are unique
		for (auto field = temp->fields; *field; field++) {
			ElementList list;

			element->GetElementsByClassName(list, *field);

			if (list.size() == 1)
				continue;

			Log::Warn("<gamelog> is missing a unique '%s' in "
				  "template %s", *field, temp->name);

			element->GetParentNode()->RemoveChild(element);
			goto next_element; // continue the *outer* loop
		}
		
		element->AddReference();
		element->GetParentNode()->RemoveChild(element);

		templates[i] = element;
	}

	initialized = true;
}

RocketGameLogElement::~RocketGameLogElement(void)
{
	for (size_t i = 0; i < gameLogEventType_t::COUNT; i++)
		if (templates[i])
			templates[i]->RemoveReference();
}

void RocketGameLogElement::OnUpdate(void)
{
	// TODO: use a constructor instead of this garbage
	if (!initialized) {
		Initialize();

		if (!initialized)
			return;
	}

	// remove old entries first
	for (std::list<gameLogEntry_t>::iterator i = entries.begin(), next;
	     i != entries.end(); i = next) {
		next = i;
		next++;

		// FIXME: don't hardcode the time
		if (cg.time > i->time + 3000) {
			RemoveChild(i->element);
			entries.erase(i, i);
		}
	}

	// turn all queued events into Rocket elements
	while (!eventQueue.empty()) {
                Event event;
		const templateProperties *temp;
		gameLogEntry_t entry;

		event = eventQueue.back();
		temp = templateTable + event.type;

		// paste the template...
		entry.time = cg.time;
		entry.element = templates[event.type]->Clone();

		// ...and fill in the information
		for (size_t i = 0; temp->fields[i]; i++ ) {
			ElementList list;

			entry.element->GetElementsByClassName(list,
				                              temp->fields[i]);

			for(auto j : list) {
				Rocket::Core::String RML;
			
				RML = Rocket_QuakeToRML(event.args[i].c_str(),
				                        RP_EMOTICONS);
				j->SetInnerRML(RML);
			}
		}

		AppendChild(entry.element);
		entries.push_front(entry);
		eventQueue.pop_back();
	}

	Element::OnUpdate();
}

static std::string TeamIcon(team_t team)
{
	switch (team) {
	case TEAM_ALIENS:
		return "^1●^7";
	case TEAM_HUMANS:
		return "^4●^7";
	default:
		return "^2●^7";
	}
}

/* FIXME:
 * - move this table to bg and keep it in sync with the mod enum
 * - do we need all the columns?
 * - draw the missing icons (mostly env kills)
 */
static const struct {
	const char *icon;
	bool envkill;
	bool assist;
	team_t team;
} obituaryModTable[ ] =
{
	// Icon            Envkill Assist? (Team)
	{ "☠",             false, false, TEAM_HUMANS },
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
	{ "[flamer]",      false, true,  TEAM_HUMANS }, // burn
	{ "[grenade]",     false, true,  TEAM_HUMANS },
	{ "[firebomb]",    false, true,  TEAM_HUMANS },
	{ "crushed",       true,  false, TEAM_NONE   }, // weight (H) // FIXME
	{ "<water>",       true,  false, TEAM_NONE   }, // water
	{ "<slime>",       true,  false, TEAM_NONE   }, // slime
	{ "<lava>",        true,  false, TEAM_NONE   }, // lava
	{ "<crush>",       true,  false, TEAM_NONE   }, // crush
	{ "[telenode]",    false, false, TEAM_NONE   }, // telefrag
	{ "<falling>",     true,  false, TEAM_NONE   }, // falling
	{ "☠",             false, false, TEAM_NONE   }, // suicide
	{ "<laser>",       true,  false, TEAM_NONE   }, // target laser
	{ "<trigger_hurt>",true,  false, TEAM_NONE   }, // trigger hurt

	{ "[granger]",     false, true,  TEAM_ALIENS },
	{ "[dretch]",      false, true,  TEAM_ALIENS },
	{ "[basilisk]",    false, true,  TEAM_ALIENS },
	{ "[dragoon]",     false, true,  TEAM_ALIENS },
	{ "[dragoon]",     false, true,  TEAM_ALIENS }, // pounce
	{ "[advdragoon]",  false, true,  TEAM_ALIENS },
	{ "[marauder]",    false, true,  TEAM_ALIENS },
	{ "[advmarauder]", false, true,  TEAM_ALIENS },
	{ "[tyrant]",      false, true,  TEAM_ALIENS },
	{ "[tyrant]",      false, true,  TEAM_ALIENS }, // trample
	{ "crushed",       false, true,  TEAM_ALIENS }, // weight (A) // FIXME

	{ "[granger]",     false, true,  TEAM_ALIENS }, // granger spit
	{ "[booster]",     false, true,  TEAM_ALIENS }, // poison
	{ "[hive]",        true,  true,  TEAM_ALIENS },

	{ "<H spawn>",     true,  false, TEAM_HUMANS }, // H spawn
	{ "[rocketpod]",   true,  true,  TEAM_HUMANS },
	{ "[turret]",      true,  true,  TEAM_HUMANS },
	{ "[reactor]",     true,  true,  TEAM_HUMANS },

	{ "<A spawn>",     true,  false, TEAM_ALIENS }, // A spawn
	{ "[acidtube]",    true,  true,  TEAM_ALIENS },
	{ "[overmind]",    true,  true,  TEAM_ALIENS },
	{ "",              true,  false, TEAM_NONE },
	{ "",              true,  false, TEAM_NONE },
	{ "",              true,  false, TEAM_NONE }
};

void Rocket_GameLog_Obituary(clientInfo_t *target, clientInfo_t *attacker,
                             clientInfo_t *assistant, int mod)
{
	Event event;
	std::string targetStr, attackerStr, assistantStr;

	targetStr = TeamIcon(target->team) + target->name;
	
	if (attacker)
		attackerStr = TeamIcon(attacker->team) + attacker->name;
	else if (assistant)
		attackerStr = "<world>";
	
	if (assistant)
		assistantStr = TeamIcon(assistant->team) + assistant->name;

	event.type = gameLogEventType_t::OBITUARY;
	if (assistant)
		event.args[0] = attackerStr + " ^7+ " + assistantStr;
	else
		event.args[0] = attackerStr;
	event.args[1] = obituaryModTable[mod].icon;
	event.args[2] = targetStr;
	eventQueue.push_front(event);

	// TODO: print obituaries to the console, so there's a backlog
}

void Rocket_GameLog_Beacon(cbeacon_t *beacon)
{
	Event event;
	const char *icon;

	switch (beacon->type) {
	case BCT_ATTACK:
		icon = "[attack]";
		break;

	case BCT_DEFEND:
		icon = "[defend]";
		break;

	case BCT_REPAIR:
		icon = "[repair]";
		break;

	case BCT_BASE:
		if (beacon->flags & EF_BC_BASE_OUTPOST)
			icon = "[outpost]";
		else
			icon = "[base]";
		break;
	
	default:
		return;
	}

	event.type = gameLogEventType_t::BEACON;
	event.args[0] = icon;
	event.args[1] = "Beacon";

	eventQueue.push_front(event);
}
