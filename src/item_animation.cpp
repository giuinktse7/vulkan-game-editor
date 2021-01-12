#include "item_animation.h"

#include <chrono>
#include <numeric>

#include "debug.h"
#include "logger.h"
#include "random.h"
#include "type_trait.h"

using Direction = item_animation::AnimationDirection;

void ItemAnimation::synchronizePhase()
{
    DEBUG_ASSERT(animationInfo->synchronized, "BUG! synchronizePhase should only be called on an animation that is marked as synchronized.");
    DEBUG_ASSERT(loopTime != 0, "Looptime must be greater than 0");
    TimePoint startTime = TimePoint::sinceStart();

    auto elapsedTimeMs = startTime.elapsedMillis();
    time_t timeDiff = elapsedTimeMs % loopTime;

    size_t phaseIndex = 0;
    while (timeDiff >= 0)
    {
        timeDiff -= animationInfo->phases[phaseIndex].maxDuration;
        if (timeDiff < 0)
            break;
        ++phaseIndex;
    }

    state.phaseIndex = static_cast<uint32_t>(phaseIndex);

    time_t res = elapsedTimeMs - (animationInfo->phases[phaseIndex].maxDuration + timeDiff);
    this->state.lastUpdateTime = startTime.forwardMs(res);

    return;
}

void ItemAnimation::initializeStartPhase()
{
    TimePoint currentTime = TimePoint::now();
    // This assumes that minDuration == maxDuration (for all phases) if the animation is synchronized.
    // TODO Check whether this assumption is correct.
    if (animationInfo->synchronized)
    {
        synchronizePhase();
    }
    else if (animationInfo->randomStartPhase)
    {
        state.phaseIndex = Random::global().nextInt<uint32_t>(0, static_cast<int>(animationInfo->phases.size()));
        this->state.lastUpdateTime = currentTime;
    }
    else
    {
        state.phaseIndex = animationInfo->defaultStartPhase;
        this->state.lastUpdateTime = currentTime;
    }
}

ItemAnimation::ItemAnimation(SpriteAnimation *animationInfo)
    : animationInfo(animationInfo)
{
    if (animationInfo->synchronized)
    {
        loopTime = std::accumulate(
            animationInfo->phases.begin(),
            animationInfo->phases.end(),
            0,
            [](uint32_t acc, SpritePhase next) { return acc + next.maxDuration; });
    }
    switch (animationInfo->loopType)
    {
        case AnimationLoopType::Infinite:
            break;
        case AnimationLoopType::PingPong:
            state.info = Direction::Forward;
            break;
        case AnimationLoopType::Counted:
            state.info = 0;
            break;
        default:
            VME_LOG_D("Unknown AnimationLoopType");
            break;
    }

    initializeStartPhase();
    setPhase(state.phaseIndex, state.lastUpdateTime);
}

uint32_t ItemAnimation::getNextPhaseMaxDuration() const
{
    SpritePhase phase = animationInfo->phases.at(state.phaseIndex);
    return phase.maxDuration;
}

void ItemAnimation::setPhase(size_t phaseIndex, TimePoint updateTime)
{
    SpritePhase phase = animationInfo->phases.at(phaseIndex);

    state.phaseIndex = static_cast<uint32_t>(phaseIndex);

    if (phase.minDuration != phase.maxDuration)
    {
        state.phaseDurationMs = Random::global().nextInt<uint32_t>(phase.minDuration, phase.maxDuration + 1);
    }
    else
    {
        state.phaseDurationMs = phase.minDuration;
    }

    state.lastUpdateTime = updateTime;
}

void ItemAnimation::update()
{
    TimePoint currentTime = TimePoint::now();

    auto elapsedTimeMs = currentTime.timeSince<std::chrono::milliseconds>(state.lastUpdateTime);
    if (elapsedTimeMs <= state.phaseDurationMs)
    {
        return;
    }

    if (animationInfo->synchronized)
    {
        auto phaseDuration = static_cast<time_t>(state.phaseDurationMs);
        auto nextPhaseDuration = animationInfo->phases.at(nextPhase()).maxDuration;

        bool outOfSync = elapsedTimeMs >= phaseDuration + nextPhaseDuration;
        if (outOfSync)
        {
            synchronizePhase();
            return;
        }
    }

    switch (animationInfo->loopType)
    {
        case AnimationLoopType::Infinite:
            if (animationInfo->synchronized)
            {
                auto updateTime = state.lastUpdateTime.forwardMs(state.phaseDurationMs);
                updateInfinite(updateTime);
            }
            else
            {
                updateInfinite(currentTime);
            }
            break;
        case AnimationLoopType::PingPong:
            updatePingPong();
            break;
        case AnimationLoopType::Counted:
            updateCounted();
            break;
    }
}

void ItemAnimation::updateInfinite(TimePoint updateTime)
{
    DEBUG_ASSERT(animationInfo->loopType == AnimationLoopType::Infinite, "Expected Infinite loop type.");
    setPhase(nextPhase(), updateTime);
}

void ItemAnimation::updatePingPong()
{
    DEBUG_ASSERT(animationInfo->loopType == AnimationLoopType::PingPong, "Expected PingPong loop type.");
    TimePoint currentTime = TimePoint::now();

    // Last phase, reverse direction
    if (state.phaseIndex == 0)
    {
        state.info = Direction::Forward;
    }
    else if (state.phaseIndex == animationInfo->phases.size() - 1)
    {
        state.info = Direction::Backward;
    }

    Direction direction = std::get<Direction>(state.info);

    int indexChange = direction == Direction::Forward ? 1 : -1;
    size_t newPhase = static_cast<size_t>(state.phaseIndex) + indexChange;
    setPhase(newPhase, currentTime);
}

void ItemAnimation::updateCounted()
{
    DEBUG_ASSERT(animationInfo->loopType == AnimationLoopType::Counted, "Expected Counted loop type.");

    TimePoint currentTime = TimePoint::now();

    uint32_t currentLoop = std::get<uint32_t>(state.info);
    if (currentLoop != animationInfo->loopCount)
    {
        if (state.phaseIndex == animationInfo->phases.size() - 1)
        {
            state.info = currentLoop + 1;
            DEBUG_ASSERT(std::get<uint32_t>(state.info) <= animationInfo->loopCount, "The current loop for an animation cannot be higher than its loopCount.");
        }

        setPhase(0, currentTime);
    }
}