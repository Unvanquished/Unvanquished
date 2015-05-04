// This header provides all types that the CBSE backend needs to know about.

#ifndef CBSE_TYPES_H_
#define CBSE_TYPES_H_

#include "sg_local.h"

/** A helper to register component thinkers. */
#define REGISTER_THINKER(METHOD, SCHEDULER, PERIOD) \
	GetThinkingComponent().RegisterThinker(std::bind(&IgnitableComponent::METHOD, this, \
	std::placeholders::_1), SCHEDULER, PERIOD)

#endif // CBSE_TYPES_H_

