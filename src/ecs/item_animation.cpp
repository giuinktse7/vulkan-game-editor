#include "item_animation.h"

#include <chrono>
#include <numeric>

#include "../type_trait.h"
#include "../debug.h"
#include "../logger.h"
#include "../random.h"

constexpr TypeList<ItemAnimationComponent> requiredComponents;

using Direction = item_animation::AnimationDirection;

void ItemAnimationComponent::synchronizePhase()
{
  DEBUG_ASSERT(animationInfo->synchronized, "BUG! synchronizePhase should only be called on an animation that is marked as synchronized.");
  TimePoint startTime = TimePoint::sinceStart();

  auto elapsedTimeMs = startTime.elapsedMillis();

  long long timeDiff = elapsedTimeMs % loopTime;

  size_t phaseIndex = 0;
  while (timeDiff >= 0)
  {
    timeDiff -= animationInfo->phases[phaseIndex].maxDuration;
    if (timeDiff < 0)
      break;
    ++phaseIndex;
  }
  this->startPhase = static_cast<uint32_t>(phaseIndex);

  long long res = elapsedTimeMs - (animationInfo->phases[phaseIndex].maxDuration + timeDiff);
  this->state.lastUpdateTime = startTime.forwardMs(res);

  // std::cout << "lastUpdateTime: " << res << ", elapsedTimeMs: " << elapsedTimeMs << ", startTimeDiff: " << startTimeDiff << ", timeDiff: " << timeDiff << ", startPhase: " << phaseIndex << std::endl;

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
    return;
  }

  if (animationInfo->randomStartPhase)
  {
    this->startPhase = Random::global().nextInt<uint32_t>(0, static_cast<int>(animationInfo->phases.size()));
    this->state.lastUpdateTime = currentTime;

    return;
  }

  this->startPhase = animationInfo->defaultStartPhase;
  this->state.lastUpdateTime = currentTime;
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
    state.info = 0;
    break;
  default:
    VME_LOG_D("Hmm");
    break;
  }

  initializeStartPhase();
  state.phaseIndex = this->startPhase;
  setPhase(state.phaseIndex, state.lastUpdateTime);
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

std::vector<const char *> ItemAnimationSystem::getRequiredComponents()
{
  return requiredComponents.typeNames();
}

void ItemAnimationSystem::update()
{
  TimePoint currentTime = TimePoint::now();

  for (const auto &entity : entities)
  {
    auto &animation = *g_ecs.getComponent<ItemAnimationComponent>(entity);

    auto elapsedTimeMs = currentTime.timeSince<std::chrono::milliseconds>(animation.state.lastUpdateTime);

    if (elapsedTimeMs >= animation.state.phaseDurationMs)
    {
      switch (animation.animationInfo->loopType)
      {
      case AnimationLoopType::Infinite:
        updateInfinite(animation, elapsedTimeMs);
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
}

void ItemAnimationSystem::updateInfinite(ItemAnimationComponent &animation, long long elapsedTimeMs)
{
  size_t nextPhase = (static_cast<size_t>(animation.state.phaseIndex) + 1) % animation.animationInfo->phases.size();

  if (animation.animationInfo->synchronized)
  {
    bool outOfSync = elapsedTimeMs >= static_cast<long long>(animation.state.phaseDurationMs) + animation.animationInfo->phases.at(nextPhase).maxDuration;
    if (outOfSync)
    {
      animation.synchronizePhase();
    }
    else
    {
      auto updateTime = animation.state.lastUpdateTime.forwardMs(animation.state.phaseDurationMs);
      animation.setPhase(nextPhase, updateTime);
    }
  }
  else
  {
    auto updateTime = animation.state.lastUpdateTime.forwardMs(animation.state.phaseDurationMs);
    animation.setPhase(nextPhase, updateTime);
  }
}

void ItemAnimationSystem::updatePingPong(ItemAnimationComponent &animation)
{
  TimePoint currentTime = TimePoint::now();

  auto direction = std::get<Direction>(animation.state.info);
  // Last phase, reverse direction
  if (animation.state.phaseIndex == 0 || animation.state.phaseIndex == animation.animationInfo->phases.size() - 1)
  {
    direction = direction == Direction::Forward ? Direction::Backward : Direction::Forward;
  }

  uint32_t indexChange = direction == Direction::Forward ? 1 : -1;
  animation.setPhase(static_cast<size_t>(animation.state.phaseIndex) + indexChange, currentTime);
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