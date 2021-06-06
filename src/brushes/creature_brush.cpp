#include "creature_brush.h"

#include "../creature.h"

CreatureBrush::CreatureBrush(std::string name, uint32_t looktype)
    : Brush(name), creatureType(Creatures::creatureType(looktype))
{
    ;
    if (!creatureType)
    {
        ABORT_PROGRAM("Invalid creature type: " << looktype);
    }
}

bool CreatureBrush::erasesItem(uint32_t serverId) const
{
    return false;
}

void CreatureBrush::apply(MapView &mapView, const Position &position)
{
    // TODO
}

BrushResource CreatureBrush::brushResource() const
{
    BrushResource resource{};
    resource.type = BrushResourceType::Creature;
    resource.id = creatureType->looktype();
    resource.variant = to_underlying(CreatureDirection::South);
    return resource;
}

BrushType CreatureBrush::type() const
{
    return BrushType::Creature;
}

std::vector<ThingDrawInfo> CreatureBrush::getPreviewTextureInfo() const
{
    return std::vector<ThingDrawInfo>{DrawCreatureType(creatureType, PositionConstants::Zero)};
}

const std::string CreatureBrush::getDisplayId() const
{
    return std::to_string(creatureType->looktype());
}
