#include "frame_group.h"

uint32_t FrameGroup::getSpriteId(uint32_t spriteIndex) const
{
    return spriteInfo.spriteIds.at(spriteIndex);
}
