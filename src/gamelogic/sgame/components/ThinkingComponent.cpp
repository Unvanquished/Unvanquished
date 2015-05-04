#include "ThinkingComponent.h"

static Log::Logger thinkLogger("sgame.thinking");

ThinkingComponent::ThinkingComponent(Entity& entity)
	: ThinkingComponentBase(entity), averageFrameTime(0)
{}

void ThinkingComponent::Think() {
	int time = level.time;
	int frameTime = level.time - level.previousTime;

	if (!averageFrameTime) {
		averageFrameTime = frameTime;
	} else {
		averageFrameTime = averageFrameTime * (1.0f - averageChangeRate) + frameTime * averageChangeRate;
	}

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
		record.thinker(timeDelta);
	}
}

void ThinkingComponent::RegisterThinker(thinker_t thinker, thinkScheduler_t scheduler, int period) {
	thinkers.emplace_back(thinkRecord_t{thinker, scheduler, period, level.time, 0});

	thinkLogger.Debug("Registered thinker of period %i.", period);
}
