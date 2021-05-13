#include "item_animation_component.h"

#include <chrono>
#include <numeric>

#include "../debug.h"
#include "../logger.h"
#include "../random.h"
#include "../type_trait.h"
#include "ecs.h"

constexpr TypeList<ItemAnimationComponent> requiredComponents;

using Direction = item_animation::AnimationDirection;

void ItemAnimationComponent::synchronizePhase()
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

void ItemAnimationComponent::initializeStartPhase()
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

ItemAnimationComponent::ItemAnimationComponent(SpriteAnimation *animationInfo)
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
            state.info = static_cast<uint32_t>(0);
            break;
        default:
            VME_LOG_D("Unknown AnimationLoopType");
            break;
    }

    initializeStartPhase();
    setPhase(state.phaseIndex, state.lastUpdateTime);
}

uint32_t ItemAnimationComponent::getNextPhaseMaxDuration() const
{
    SpritePhase phase = animationInfo->phases.at(state.phaseIndex);
    return phase.maxDuration;
}

void ItemAnimationComponent::setPhase(size_t phaseIndex, TimePoint updateTime)
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

std::vector<const char *> ItemAnimationSystem::getRequiredComponents() const
{
    return requiredComponents.typeNames();
}

void ItemAnimationSystem::update()
{
    TimePoint currentTime = TimePoint::now();

    // TODO
    // Only visible entities need to have their animtions updated. There's no
    // system in place for this yet (2021-01-01).
    for (const auto &entity : entities)
    {
        auto &animation = *g_ecs.getComponent<ItemAnimationComponent>(entity);

        auto elapsedTimeMs = currentTime.timeSince<std::chrono::milliseconds>(animation.state.lastUpdateTime);
        if (elapsedTimeMs <= animation.state.phaseDurationMs)
        {
            continue;
        }

        if (animation.animationInfo->synchronized)
        {
            auto phaseDuration = static_cast<time_t>(animation.state.phaseDurationMs);
            auto nextPhaseDuration = animation.animationInfo->phases.at(animation.nextPhase()).maxDuration;

            bool outOfSync = elapsedTimeMs >= phaseDuration + nextPhaseDuration;
            if (outOfSync)
            {
                animation.synchronizePhase();
                continue;
            }
        }

        switch (animation.animationInfo->loopType)
        {
            case AnimationLoopType::Infinite:
                if (animation.animationInfo->synchronized)
                {
                    auto updateTime = animation.state.lastUpdateTime.forwardMs(animation.state.phaseDurationMs);
                    updateInfinite(animation, updateTime);
                }
                else
                {
                    updateInfinite(animation, currentTime);
                }
                break;
            case AnimationLoopType::PingPong:
                updatePingPong(animation);
                break;
            case AnimationLoopType::Counted:
                updateCounted(animation);
                break;
        }
    }
}

void ItemAnimationSystem::updateInfinite(ItemAnimationComponent &animation, TimePoint updateTime)
{
    animation.setPhase(animation.nextPhase(), updateTime);
}

void ItemAnimationSystem::updatePingPong(ItemAnimationComponent &animation)
{
    TimePoint currentTime = TimePoint::now();

    // Last phase, reverse direction
    if (animation.state.phaseIndex == 0)
    {
        animation.state.info = Direction::Forward;
    }
    else if (animation.state.phaseIndex == animation.animationInfo->phases.size() - 1)
    {
        animation.state.info = Direction::Backward;
    }

    Direction direction = std::get<Direction>(animation.state.info);

    int indexChange = direction == Direction::Forward ? 1 : -1;
    size_t newPhase = static_cast<size_t>(animation.state.phaseIndex) + indexChange;
    animation.setPhase(newPhase, currentTime);
}

void ItemAnimationSystem::updateCounted(ItemAnimationComponent &animation)
{
    TimePoint currentTime = TimePoint::now();

    uint32_t currentLoop = std::get<uint32_t>(animation.state.info);
    if (currentLoop != animation.animationInfo->loopCount)
    {
        if (animation.state.phaseIndex == animation.animationInfo->phases.size() - 1)
        {
            animation.state.info = currentLoop + 1;
            DEBUG_ASSERT(std::get<uint32_t>(animation.state.info) <= animation.animationInfo->loopCount, "The current loop for an animation cannot be higher than its loopCount.");
        }

        animation.setPhase(0, currentTime);
    }
}