#include "sg_local.h"
#include "shared/bg_tutorial.h"
#include "CBSE.h"
#include "Entities.h"

#include <functional>

using setter = std::function<void(playerState_t &ps)>;

static void BroadcastChange(setter f)
{
	for (int i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected == CON_DISCONNECTED)
			continue;
		if (g_entities[i].r.svFlags & SVF_BOT)
			continue;
		f(g_clients[i].ps);
	}
}

// this is basically LookUpMainBuildable but with bool instead of gentity_t*,
// and it checks the buildable is powered
template<typename Component>
static bool LookUpBuildable(bool requireActive = true)
{
        bool answer = false;
        ForEntities<Component>([&](Entity& entity, Component&) {
                if (!requireActive ||
                    (entity.Get<BuildableComponent>()->GetState() == BuildableComponent::CONSTRUCTED && entity.Get<BuildableComponent>()->Powered()))
                {
                        answer = true;
                }
        });
        return answer;
}

// note this includes bots as well
static bool LookUpPlayer(class_t class_)
{
	for ( int i = 0; i < level.maxclients; i++ ) {
		if (g_clients[i].ps.stats[ STAT_CLASS ] == class_)
			return true;
	}
	return false;
}

static bool FindOneOfTeam(team_t team)
{
	for ( int i = 0; i < level.maxclients; i++ ) {
		if (G_Team(&g_entities[i]) == team
				&& Entities::IsAlive(&g_entities[i])) {
			return true;
		}
	}
	return false;
}

/*
 * "Check Progress" functions.
 * Those return true when the action is done
 */

static bool OvermindBuilt()
{
	return LookUpBuildable<OvermindComponent>(true);
}

static bool EggBuilt()
{
	return LookUpBuildable<EggComponent>(true);
}

static bool EvolvedToDretch()
{
	return LookUpPlayer(PCL_ALIEN_LEVEL0);
}

static bool KilledAllHumans()
{
	return !FindOneOfTeam(TEAM_HUMANS);
}

struct step {
	enum tutorialMsg msg;
	bool (*checkStepPassed)();
};
using steps = std::vector<struct step>;


static const std::map<std::string, steps> tutorial_maps_steps = {
	{ "tutorial0", {
		{ tutorialMsg::A_BUILD_OVERMIND, OvermindBuilt },
		{ tutorialMsg::A_BUILD_EGG, EggBuilt },
		{ tutorialMsg::A_EVOLVE_DRETCH, EvolvedToDretch },
		{ tutorialMsg::A_KILL_HUMANS, KilledAllHumans },
		       },
	},
};

static size_t DetermineStep(const std::vector<struct step>& steps)
{
	size_t i;
	for (i=0; i<steps.size(); i++) {
		if (!steps[i].checkStepPassed())
			break;
	}
	return i;
}

void G_Tutorial_CheckProgress()
{
	// static as we rely on the map not changing after the game has started
	static auto steps_iter = tutorial_maps_steps.find(Cvar::GetValue("mapname"));
	if (steps_iter == tutorial_maps_steps.end())
		return; // this is not a (known) tutorial map
	const steps &steps = steps_iter->second;

	size_t max = steps.size();
	size_t num = DetermineStep(steps);

	if (num < max) {
		BroadcastChange([steps,num,max](playerState_t &ps){
				ps.tutorialStepMsg = steps[num].msg;
				ps.tutorialStepNum = num;
				ps.tutorialStepMax = max;
			});
	} else {
		BroadcastChange([max](playerState_t &ps){
				ps.tutorialStepMsg = tutorialMsg::CONGRATS;
				ps.tutorialStepNum = max;
				ps.tutorialStepMax = max;
			});
	}
}
