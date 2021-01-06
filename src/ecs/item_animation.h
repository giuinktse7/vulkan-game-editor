#pragma once

#include <variant>

#include "../time_point.h"
#include "ecs_system.h"

enum class AnimationLoopType
{
    PingPong = -1,
    Infinite = 0,
    Counted = 1
};

struct SpritePhase
{
    uint32_t minDuration;
    uint32_t maxDuration;
};

struct SpriteAnimation
{
    uint32_t defaultStartPhase;
    bool synchronized;
    /*
    If true, the animation can start in any phase.
  */
    bool randomStartPhase;
    AnimationLoopType loopType;

    // Amount of times to loop. Only relevant if the loopType is AnimationLoopType::Counted.
    uint32_t loopCount;
    std::vector<SpritePhase> phases;

    friend struct SpriteInfo;
};

namespace item_animation
{
    enum class AnimationDirection
    {
        Forward,
        Backward
    };
}

struct ItemAnimationComponent;

class ItemAnimationSystem : public ecs::System
{
  public:
    void update();

  private:
    std::vector<const char *> getRequiredComponents() const override;

    void updateInfinite(ItemAnimationComponent &animation, TimePoint updateTime);
    void updatePingPong(ItemAnimationComponent &animation);
    void updateCounted(ItemAnimationComponent &animation);
};

struct ItemAnimationComponent
{
    ItemAnimationComponent(SpriteAnimation *animationInfo);

    SpriteAnimation *animationInfo;

    struct
    {
        uint32_t phaseIndex = 0;
        /*
					Phase duration in milliseconds.
				*/
        uint32_t phaseDurationMs = 0;

        /*
					Holds necessary information about the animation state based on the
					animation type.
					AnimationLoopType::Infinte -> Nothing
					AnimationLoopType::PingPong -> ItemAnimator::Direction
					AnimationLoopType::Counted -> current loop
				*/
        std::variant<std::monostate, item_animation::AnimationDirection, uint32_t> info;

        /*
					The time that the latest phase change occurred.
				*/
        TimePoint lastUpdateTime;
    } state;

    void synchronizePhase();
    void setPhase(size_t phaseIndex, TimePoint updateTime);

    uint32_t getNextPhaseMaxDuration() const;

    inline size_t nextPhase() const
    {
        return (static_cast<size_t>(state.phaseIndex) + 1) % animationInfo->phases.size();
    }

  private:
    uint32_t loopTime = 0;

    void initializeStartPhase();
};