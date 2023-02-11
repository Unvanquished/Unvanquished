# CBSE Generator

Code generator that produces the plumbing for a component-based gamelogic for the game Unvanquished.
This allows to have a flexible gamelogic where entities are defined as sets of behavior vs. inheritance or master-entity like in Quake3.

The only dependencies of the script are python3 and python3-yaml as well as a C++11 compiler to compile the output.

## Rational and Terminology

In Quake3 an entity is represented by always the same C structure, for most entities this is a waste of space as they use only a subset of the fields.
Worse, different entity types sometimes use the same fields for different purpose which is horrible confusing; so the gamelogic programmer is torn between adding new fields for his purpose and increase the memory footprint or reuse fields or work around them.

Some games use an inheritance scheme to make the gamelogic more flexible, but it quickly gets limiting .
For example considering the following entity type tree, ``MonsterChest`` (the thing that bites you when you loot it) would have to be both a ``Monster`` and a ``Chest`` which is not possible:
```
- Entity
    - StaticEntity
        - Door
        - Chest
    - DynamicEntity
        - Monster
            - Lion
            - Trolloc
```

Component-based software engineering helps solve that problem by agregating multiple behaviors together to make the entity.
In the case of the ``MonsterChest`` it would have both a ``Monster`` behavior and a ``Lootable`` behavior.

We say that an object of the game is an *entity* that contains a number of behaviors called *components* that interact using entity-wide broadcast *messages*.
In addition components can depend from one another when they need tighter coupling while still being logically separate components.
We will eventually have component inheritance, so that each behavior can be an inheritance tree.
For example physics can take advantage of this by having a virtual ``PhysicsComponent`` implemented differently by ``StaticPhysicsComponent`` and ``RagdollPhysicsComponent``.
Obviously care will be needed to balance between inheritance and dependencies.

### How the generation works

The definition of the components and entities used by the gamelogic is parsed from a YAML file by the python generator which will then use Jinja2 templates to "render" C++ files.
As part of the processing, consistency checks will be made (TODO).
As part of the processing, the correctness of the definition should be checked (for example the dependency-inheritance graph must be acyclic) and each component will gather its "own" attributes/messages... for rendering.

## Using the generated code

### The Entity object

Sending messages to components handling them can be done from ``Entity``: suppose we have defined a ``Damage`` message taking ``int`` and an ``AttackType`` argument, then the following code sends a damage message that can be used like and event:
```cpp
Entity* entity = FindEntityHitByAttack(attackOrigin, attackDirection);
if (entity) {
    bool damagedHandled = entity->Damage(attackDamage, attackType);
    if (damageHandled) {
        PlayHitSound();
    }
}
```
The function that sends the message returns ``true`` if there was at least one component that handled the message; in this case this tells us that the entity was hit in some way so we should play a hit sound.

Messages are great for broadcasting but sometimes given an ``Entity`` all you need is call a method of a specific component; defining a message handled only by that component would be wasteful.
To avoid that anti-pattern the ``Entity`` can also be queried for a specific component, returning ``nullptr`` if it doesn't exist:
```cpp
for (Entity* grabber : GetEntitiesColliding(pointsBonus->hitbox)) {
    ScoreComponent* score = grabber->Get<ScoreComponent>();
    if (score != nullptr) {
        score->acquire(pointsBonus);
        break;
    }
}
```

Likewise it is possible to iterate over all entities with a certain component to help process them in batch:
```cpp
ForEntities<FireComponent>([] (Entity* entity, FireComponent& fire) {
   fire.Spread();
});
```

### Defining a component

Messages and components are defined in a YAML document. Messages are simply defined as list of parameters of the form (name, type) and the list of message handled is under the ``messages`` key of the dictionnary defining a component.
```yaml
messages:
    Damage:
        - name: amount
          type: int
        - name: flags
          type: int
    Heal:
        - name: amount
          type: int
components:
    Physics:
        messages:
            - Damage # for example for knockback
    Health:
        messages:
            - Damage
            - Heal
# Definition of entities
```

From this the generator will create both base classes for the components that implements helper functions and a skeletons for the implementation of the components. The component constructor is forced from the definition file (see parameters and dependencies) as are the message handling functions, but stubs are generated in the skeletons. Message handling function look like the following:
```cpp
class HealthComponent : public HealthComponentBase {
public:
    # Constructor skipped
    void HandleDamage(int amount, int flags);
    void HandleHeal(int amount);
    
    # Regular member functions can be called directly
    # for example after a 
    void SetMaxHealth(int maximum);
};
```

In this example, ``SetMaxHealth`` is a member function that can be called directly, for example after a ``entity.Get<HealthComponent>()``. However the maximum health is the same for all entities of the same entity type so it can be put directly in the entity definition.

### Defining entities
