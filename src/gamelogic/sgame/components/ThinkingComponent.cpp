#include "ThinkingComponent.h"

static Log::Logger thinkLogger("sgame.thinking");

ThinkingComponent::ThinkingComponent(Entity& entity, DeferredFreeingComponent& r_DeferredFreeingComponent)
	: ThinkingComponentBase(entity, r_DeferredFreeingComponent), iteratingThinkers(false), averageFrameTime(0)
{}

void ThinkingComponent::Think() {
	int time = level.time;
	int frameTime = level.time - level.previousTime;

	if (!averageFrameTime) {
		averageFrameTime = frameTime;
	} else {
		averageFrameTime = averageFrameTime * (1.0f - averageChangeRate) + frameTime * averageChangeRate;
	}

	iteratingThinkers = true;
	for (thinkRecord_t &record : thinkers) {
		int timeDelta = time - record.timestamp;
		int thisFrameExecutionLateness = timeDelta - record.period;
		int nextFrameExecutionLateness = timeDelta + averageFrameTime - record.period;

		switch (record.scheduler) {
			case SCHEDULER_AFTER:
				if (thisFrameExecutionLateness < 0) continue;
				break;

			case SCHEDULER_BEFORE:
				if (nextFrameExecutionLateness <= 0) continue;
				break;

			case SCHEDULER_CLOSEST:
				if (std::abs(nextFrameExecutionLateness) <
				    std::abs(thisFrameExecutionLateness)) continue;
				break;

			case SCHEDULER_AVERAGE:
				if (std::abs(nextFrameExecutionLateness + record.delay) <
				    std::abs(thisFrameExecutionLateness + record.delay)) continue;
				record.delay += thisFrameExecutionLateness;
				break;
		}

		thinkLogger.Debug("Calling thinker of period %i with lateness %i.",
		                  record.period, thisFrameExecutionLateness);

		record.timestamp = time;

		unregisterActiveThinker = false;
		record.thinker(timeDelta);
		record.unregister = unregisterActiveThinker;
	}
	iteratingThinkers = false;

	// Remove thinkers that unregistered themselves during iteration.
	thinkers.erase(std::remove_if(thinkers.begin(), thinkers.end(),
	                              [](thinkRecord_t &record){return record.unregister;}),
	               thinkers.end());

	// Add thinkers that were registered during iteration.
	thinkers.insert(thinkers.end(), newThinkers.begin(), newThinkers.end());
	newThinkers.clear();
}

void ThinkingComponent::RegisterThinker(thinker_t thinker, thinkScheduler_t scheduler, int period) {
	// When thinkers are being executed, add new ones to a temporary container so the iterator isn't
	// invalidated.
	std::vector<thinkRecord_t> *addTo = iteratingThinkers ? &newThinkers : &thinkers;

	addTo->emplace_back(thinkRecord_t{thinker, scheduler, period, level.time, 0, false});

	thinkLogger.Notice("Registered thinker of period %i.", period);
}

void ThinkingComponent::UnregisterActiveThinker() {
	unregisterActiveThinker = true;

	thinkLogger.Notice("Unregistered the active thinker.");
}
