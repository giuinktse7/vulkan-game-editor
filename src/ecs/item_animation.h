#pragma once

#include "ecs.h"
#include "../graphics/appearances.h"
#include "../time_point.h"

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
	void update() override;

private:
	std::vector<const char *> getRequiredComponents() override;

	void updateInfinite(ItemAnimationComponent &animation, long long elapsedTimeMs);
	void updatePingPong(ItemAnimationComponent &animation);
	void updateCounted(ItemAnimationComponent &animation);
};

struct ItemAnimationComponent
{
	ItemAnimationComponent(SpriteAnimation *animationInfo);

	void setPhase(size_t phaseIndex, TimePoint updateTime);

	SpriteAnimation *animationInfo;
	// Should not be changed after construction
	uint32_t startPhase;

	uint32_t loopTime = 0;

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

	void initializeStartPhase();
	void synchronizePhase();
};