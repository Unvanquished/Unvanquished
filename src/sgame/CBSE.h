// This header includes the rest of the CBSE system.
// It also provides to the CBSE backend all types that it needs to know about.

#ifndef CBSE_H_
#define CBSE_H_

// Add here any definitions, forward declarations and includes that provide all
// the types used in the entities definition file (and thus the CBSE backend).
// Make sure none of the includes in this file includes any header that is part
// of the CBSE system.
// You can also define helper macros for use in all components here.
// ----------------

#include "sg_local.h"

/** A helper to register component thinkers. */
#define REGISTER_THINKER(METHOD, SCHEDULER, PERIOD) \
	GetThinkingComponent().RegisterThinker([this](int i){this->METHOD(i);}, SCHEDULER, PERIOD)

// ----------------

// Include the backend. These should be the last lines in this header.
#ifndef CBSE_INCLUDE_TYPES_ONLY
#include "backend/CBSEEntities.h"
#endif // CBSE_INCLUDE_TYPES_ONLY

#endif // CBSE_H_
