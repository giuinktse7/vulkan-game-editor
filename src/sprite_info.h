#pragma once

#include "ecs/item_animation.h"
#include <memory>
#include <vector>

struct SpriteInfo
{
    SpriteInfo();
    SpriteInfo(SpriteAnimation &&animation);

    inline bool hasAnimation() const noexcept;

    SpriteAnimation *animation() const;

    std::vector<uint32_t> spriteIds;
    uint32_t boundingSquare;

    uint8_t patternWidth = 1;
    uint8_t patternHeight = 1;
    uint8_t patternDepth = 1;
    uint8_t patternSize = 1;
    uint8_t layers = 1;

    bool isOpaque;

  private:
    friend class ObjectAppearance;

    std::unique_ptr<SpriteAnimation> _animation;
};

inline bool SpriteInfo::hasAnimation() const noexcept
{
    return _animation != nullptr;
}