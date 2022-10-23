#include "AlienBuildableComponent.h"
#include "../Entities.h"
#include <random>

static Log::Logger alienBuildableLogger("sgame.alienbuildings");

const int AlienBuildableComponent::MIN_BLAST_DELAY = 2000;
const int AlienBuildableComponent::MAX_BLAST_DELAY = 5000; // Should match death animation length.

AlienBuildableComponent::AlienBuildableComponent(Entity& entity, BuildableComponent& r_BuildableComponent,
	TeamComponent& r_TeamComponent, IgnitableComponent& r_IgnitableComponent)
	: AlienBuildableComponentBase(entity, r_BuildableComponent, r_TeamComponent, r_IgnitableComponent) {
	GetBuildableComponent().REGISTER_THINKER(Think, ThinkingComponent::SCHEDULER_AVERAGE, 500);
}

void AlienBuildableComponent::HandleDamage(float /*amount*/, gentity_t* /*source*/, Util::optional<glm::vec3> /*location*/,
                                           Util::optional<glm::vec3> /*direction*/, int /*flags*/, meansOfDeath_t /*meansOfDeath*/) {
	if (GetBuildableComponent().GetState() != BuildableComponent::CONSTRUCTED) return;

	// TODO: Move animation code to BuildableComponent.
	if (!GetBuildableComponent().AnimationProtected()) {
		G_SetBuildableAnim(entity.oldEnt, BANIM_PAIN1, false);
	}
}

void AlienBuildableComponent::Think(int /*timeDelta*/) {
	if ( g_gameMode.Get() == "juggernaut" )
	{
		return; // FIXME: juggernaut hack
	}

	// TODO: Find an elegant way to access per-buildable configuration.
	float creepSize = (float)BG_Buildable((buildable_t)entity.oldEnt->s.modelindex)->creepSize;

	// Slow close humans.
	ForEntities<HumanClassComponent>([&] (Entity& other, HumanClassComponent&) {
		// TODO: Add LocationComponent.
		if (G_Distance(entity.oldEnt, other.oldEnt) > creepSize) return;

		// TODO: Send (Creep)Slow message instead.
		if (other.oldEnt->flags & FL_NOTARGET) return;
		if (other.oldEnt->client->ps.groundEntityNum == ENTITYNUM_NONE) return;
		other.oldEnt->client->ps.stats[STAT_STATE] |= SS_CREEPSLOWED;
		other.oldEnt->client->lastCreepSlowTime = level.time;
	});
}

void AlienBuildableComponent::HandleDie(gentity_t* /*killer*/, meansOfDeath_t /*meansOfDeath*/) {
	// Set blast timer.
	int blastDelay = 0;
	if (entity.oldEnt->spawned && GetBuildableComponent().GetHealthComponent().Health() /
			GetBuildableComponent().GetHealthComponent().MaxHealth() > -1.0f) {
		blastDelay += GetBlastDelay();
	}

	alienBuildableLogger.Debug("Alien buildable dies, will blast in %i ms.", blastDelay);

	GetBuildableComponent().SetState(BuildableComponent::PRE_BLAST);

	GetBuildableComponent().REGISTER_THINKER(Blast, ThinkingComponent::SCHEDULER_BEFORE, blastDelay);
}

void AlienBuildableComponent::Blast(int /*timeDelta*/) {
	float          splashDamage = (float)entity.oldEnt->splashDamage;
	float          splashRadius = (float)entity.oldEnt->splashRadius;
	meansOfDeath_t splashMOD    = (meansOfDeath_t)entity.oldEnt->splashMethodOfDeath;

	// Damage close humans.
	Entities::AntiHumanRadiusDamage(entity, splashDamage, splashRadius, splashMOD);

	// Reward attackers.
	G_RewardAttackers(entity.oldEnt);

	// Stop collisions, add blast event and update buildable state.
	entity.oldEnt->r.contents = 0; trap_LinkEntity(entity.oldEnt);
	G_AddEvent(entity.oldEnt, EV_ALIEN_BUILDABLE_EXPLOSION, DirToByte(entity.oldEnt->s.origin2));
	GetBuildableComponent().SetState(BuildableComponent::POST_BLAST); // Makes entity invisible.

	// Start creep recede with a brief delay.
	GetBuildableComponent().REGISTER_THINKER(CreepRecede, ThinkingComponent::SCHEDULER_AVERAGE, 500);
	GetBuildableComponent().GetThinkingComponent().UnregisterActiveThinker();
}

// TODO: Move this to the client side.
void AlienBuildableComponent::CreepRecede(int /*timeDelta*/) {
	alienBuildableLogger.Debug("Starting creep recede.");

	G_AddEvent(entity.oldEnt, EV_BUILD_DESTROY, 0);

	if (entity.oldEnt->spawned) {
		entity.oldEnt->s.time = -level.time;
	} else {
		entity.oldEnt->s.time = -(level.time - (int)(
			(float)CREEP_SCALEDOWN_TIME *
			(1.0f - ((float)(level.time - entity.oldEnt->creationTime) /
					 (float)BG_Buildable(entity.oldEnt->s.modelindex)->buildTime)))
		);
	}

	// Remove buildable when done.
	GetBuildableComponent().REGISTER_THINKER(Remove, ThinkingComponent::SCHEDULER_AFTER, CREEP_SCALEDOWN_TIME);
	GetBuildableComponent().GetThinkingComponent().UnregisterActiveThinker();
}

void AlienBuildableComponent::Remove(int /*timeDelta*/) {
	alienBuildableLogger.Debug("Removing alien buildable.");

	entity.FreeAt(DeferredFreeingComponent::FREE_AFTER_THINKING);
}

int AlienBuildableComponent::GetBlastDelay() {
	// TODO: Have one PRNG for all of sgame.
	std::default_random_engine generator(level.time);
	std::uniform_int_distribution<int> distribution(MIN_BLAST_DELAY, MAX_BLAST_DELAY);

	return distribution(generator);
}
