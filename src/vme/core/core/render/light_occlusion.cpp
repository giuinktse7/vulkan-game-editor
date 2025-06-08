#include "light_occlusion.h"

#include <algorithm>

// -1 for x to signify that it's not initialized yet
OcclusionData::OcclusionData()
    : viewport{.x = -1, .y = 0, .z = GROUND_FLOOR, .width = 0, .height = 0, .zoom = 1.0F}
{
    reset();
}

void OcclusionData::reset()
{
    // TODO Can be optimized - We only need to reset the part that is visible
    std::ranges::fill(occlusionUbo.occlusion, 0);
}
