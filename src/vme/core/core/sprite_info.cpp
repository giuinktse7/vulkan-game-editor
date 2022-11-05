#include "sprite_info.h"

SpriteInfo::SpriteInfo() {}

SpriteInfo::SpriteInfo(SpriteAnimation &&animation)
    : _animation(std::make_unique<SpriteAnimation>(std::move(animation)))
{
}