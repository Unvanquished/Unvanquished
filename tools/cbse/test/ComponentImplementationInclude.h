class HealthComponent : public HealthComponentBase {
    public:
        HealthComponent(Entity& entity, int maxHealth, int startHealth): HealthComponentBase(entity, maxHealth, startHealth) {
        }

        void HandleHeal(int amount) {
        }

        void HandleDamage(int amount) {
        }
};

class MonsterDieComponent : public MonsterDieComponentBase {
    public:
        MonsterDieComponent(Entity& entity): MonsterDieComponentBase(entity) {
        }

        void HandleDie() {
        }
};
