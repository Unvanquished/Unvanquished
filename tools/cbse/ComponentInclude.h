class HealthComponent : public HealthComponentBase {
public:
    HealthComponent(Entity* entity, int maxHealth, int startHealth, const int& Health): HealthComponentBase(entity, maxHealth, startHealth, Health) {
    }

    void OnHeal(int amount) {
    }
    void OnDamage(int amount) {
    }
    void AttribHealth(int health) {
    }
};
class MonsterDieComponent : public MonsterDieComponentBase {
public:
    MonsterDieComponent(Entity* entity): MonsterDieComponentBase(entity) {
    }

    void OnDie() {
    }
};
