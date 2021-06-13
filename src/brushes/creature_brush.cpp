#include "creature_brush.h"

#include "../creature.h"
#include "../map_view.h"

CreatureBrush::CreatureBrush(CreatureType *creatureType)
    : Brush(creatureType->name()), creatureType(creatureType)
{
    if (!creatureType)
    {
        ABORT_PROGRAM("CreatureBrush::CreatureBrush was passed nullptr.");
    }
}

bool CreatureBrush::erasesItem(uint32_t serverId) const
{
    return false;
}

void CreatureBrush::apply(MapView &mapView, const Position &position)
{
    mapView.addCreature(position, Creature(*creatureType));
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

const std::string &CreatureBrush::id() const noexcept
{
    return creatureType->id();
}
