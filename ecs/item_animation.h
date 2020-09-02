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

	struct
	{
		uint32_t phaseIndex;
		/*
			Phase duration in milliseconds.
		*/
		uint32_t phaseDurationMs;

		using loop_t = uint32_t;
		/*
			Holds necessary information about the animation state based on the
			animation type.
			AnimationLoopType::Infinte -> total time for one loop
			AnimationLoopType::PingPong -> ItemAnimator::Direction
			AnimationLoopType::Counted -> current loop
		*/
		std::variant<uint32_t, item_animation::AnimationDirection> info;
		/*
			The time that the latest phase change occurred.
		*/
		TimePoint lastUpdateTime;
	} state;

	void initializeStartPhase();
	void synchronizePhase();
};