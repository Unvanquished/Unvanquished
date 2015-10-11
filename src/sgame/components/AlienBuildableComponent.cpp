#include "AlienBuildableComponent.h"
#include <random>

static Log::Logger alienBuildableLogger("sgame.alienbuildings");

const int AlienBuildableComponent::MIN_BLAST_DELAY = 2000;
const int AlienBuildableComponent::MAX_BLAST_DELAY = 5000; // Should match death animation length.

AlienBuildableComponent::AlienBuildableComponent(Entity& entity, BuildableComponent& r_BuildableComponent,
                                                 IgnitableComponent& r_IgnitableComponent)
	: AlienBuildableComponentBase(entity, r_BuildableComponent, r_IgnitableComponent) {
	GetBuildableComponent().REGISTER_THINKER(Think, ThinkingComponent::SCHEDULER_AVERAGE, 500);
}

void AlienBuildableComponent::HandleDamage(float amount, gentity_t* source, Util::optional<Vec3> location,
                                           Util::optional<Vec3> direction, int flags, meansOfDeath_t meansOfDeath) {
	if (GetBuildableComponent().GetState() != BuildableComponent::CONSTRUCTED) return;

	// TODO: Move animation code to BuildableComponent.
	G_SetBuildableAnim(entity.oldEnt, BANIM_PAIN1, false);
}

void AlienBuildableComponent::Think(int timeDelta) {
	// TODO: Port gentity_t.powered.
	entity.oldEnt->powered = (G_ActiveOvermind() != nullptr);

	// Suicide if creep is gone.
	if (BG_Buildable(entity.oldEnt->s.modelindex)->creepTest && !G_FindCreep(entity.oldEnt)) {
		G_Kill(entity.oldEnt, entity.oldEnt->powerSource ? &g_entities[entity.oldEnt->powerSource->killedBy] : nullptr, MOD_NOCREEP);
	}

	// TODO: Find an elegant way to access per-buildable configuration.
	float creepSize = (float)BG_Buildable((buildable_t)entity.oldEnt->s.modelindex)->creepSize;

	// Slow close humans.
	ForEntities<HumanClassComponent>([&] (Entity& other, HumanClassComponent& humanClassComponent) {
		// TODO: Add LocationComponent.
		if (G_Distance(entity.oldEnt, other.oldEnt) > creepSize) return;

		// TODO: Send (Creep)Slow message instead.
		if (other.oldEnt->flags & FL_NOTARGET) return;
		if (other.oldEnt->client->ps.groundEntityNum == ENTITYNUM_NONE) return;
		other.oldEnt->client->ps.stats[STAT_STATE] |= SS_CREEPSLOWED;
		other.oldEnt->client->lastCreepSlowTime = level.time;
	});
}

void AlienBuildableComponent::HandleDie(gentity_t* killer, meansOfDeath_t meansOfDeath) {
	entity.oldEnt->powered = false;

	// Warn if in main base and there's an overmind.
	gentity_t *om;
	if ((om = G_ActiveOvermind()) && om != entity.oldEnt && level.time > om->warnTimer
			&& G_InsideBase(entity.oldEnt, true) && G_IsWarnableMOD(meansOfDeath)) {
		om->warnTimer = level.time + ATTACKWARN_NEARBY_PERIOD;
		G_BroadcastEvent(EV_WARN_ATTACK, 0, TEAM_ALIENS);
		Beacon::NewArea(BCT_DEFEND, entity.oldEnt->s.origin, entity.oldEnt->buildableTeam);
	}

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

void AlienBuildableComponent::Blast(int timeDelta) {
	float          splashDamage = (float)entity.oldEnt->splashDamage;
	float          splashRadius = (float)entity.oldEnt->splashRadius;
	meansOfDeath_t splashMOD    = (meansOfDeath_t)entity.oldEnt->splashMethodOfDeath;

	// Damage close humans.
	Utility::AntiHumanRadiusDamage(entity, splashDamage, splashRadius, splashMOD);

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
void AlienBuildableComponent::CreepRecede(int timeDelta) {
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

void AlienBuildableComponent::Remove(int timeDelta) {
	alienBuildableLogger.Debug("Removing alien buildable.");

	entity.FreeAt(DeferredFreeingComponent::FREE_AFTER_THINKING);
}

int AlienBuildableComponent::GetBlastDelay() {
	// TODO: Have one PRNG for all of sgame.
	std::default_random_engine generator(level.time);
	std::uniform_int_distribution<int> distribution(MIN_BLAST_DELAY, MAX_BLAST_DELAY);

	return distribution(generator);
}
