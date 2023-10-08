// This header includes the rest of the CBSE system.
// It also provides to the CBSE backend all types that it needs to know about.

#ifndef CBSE_H_
#define CBSE_H_

#include "sg_local.h"

template<typename T> constexpr int ComponentPriority(); // forward declaring from CBSEBackend.h

// Component creating/destruction callbacks. These are used to enable iterating
// over all entities with a given component.
void RegisterComponentCreate(int entityNum, int componentNum);
void RegisterComponentDestroy(int entityNum, int componentNum);

template<typename T>
void OnComponentCreate(T* component)
{
	RegisterComponentCreate(component->entity.oldEnt - g_entities, ComponentPriority<T>());
}

template<typename T>
void OnComponentDestroy(T* component)
{
	RegisterComponentDestroy(component->entity.oldEnt - g_entities, ComponentPriority<T>());
}

// Add here any definitions, forward declarations and includes that provide all
// the types used in the entities definition file (and thus the CBSE backend).
// Make sure none of the includes in this file includes any header that is part
// of the CBSE system.
// You can also define helper macros for use in all components here.
// ----------------

/** A helper to register component thinkers. */
#define REGISTER_THINKER(METHOD, SCHEDULER, PERIOD) \
	GetThinkingComponent().RegisterThinker([this](int i){this->METHOD(i);}, (SCHEDULER), (PERIOD))

// ----------------

#endif // CBSE_H_

// Include the backend. These should be the last lines in this header.
#ifndef CBSE_INCLUDE_TYPES_ONLY
#include "backend/CBSEEntities.h"
#endif // CBSE_INCLUDE_TYPES_ONLY
