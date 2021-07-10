#pragma once

#include <memory>
#include <vector>

#include "item_animation.h"

struct SpriteInfo
{
    SpriteInfo();
    SpriteInfo(SpriteAnimation &&animation);

    inline bool hasAnimation() const noexcept;

    SpriteAnimation *animation() const;

    std::vector<uint32_t> spriteIds;
    uint32_t boundingSquare = 0;

    // For a creature SpriteInfo: maybe patternWidth is the amount of directions?
    uint8_t patternWidth = 1;
    // For a creature SpriteInfo: maybe patternHeight is the amount of addons(base_outfit, addon1, addon2)?
    uint8_t patternHeight = 1;
    // For a creature SpriteInfo: maybe patternDepth is the amount of distinct player positions (standing, mounted)?
    uint8_t patternDepth = 1;
    uint8_t patternSize = 1;
    uint8_t layers = 1;

    bool isOpaque = false;

  private:
    friend class ObjectAppearance;

    // TODO Investigate: Why unique_ptr? Can it not just be "SpriteAnimation _animation;"?
    std::unique_ptr<SpriteAnimation> _animation;
};

inline bool SpriteInfo::hasAnimation() const noexcept
{
    return _animation != nullptr;
}