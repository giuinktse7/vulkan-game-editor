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

void CreatureBrush::apply(MapView &mapView, const Position &position, Direction direction)
{
    auto creature = Creature(*creatureType);
    creature.setDirection(direction);
    mapView.addCreature(position, std::move(creature));
}

BrushResource CreatureBrush::brushResource() const
{
    BrushResource resource{};
    resource.type = BrushResourceType::Creature;
    resource.id = creatureType->looktype();
    resource.variant = to_underlying(Direction::South);
    return resource;
}

BrushType CreatureBrush::type() const
{
    return BrushType::Creature;
}

std::vector<ThingDrawInfo> CreatureBrush::getPreviewTextureInfo(Direction direction) const
{
    return std::vector<ThingDrawInfo>{DrawCreatureType(creatureType, PositionConstants::Zero, direction)};
}

const std::string CreatureBrush::getDisplayId() const
{
    return std::to_string(creatureType->looktype());
}

const std::string &CreatureBrush::id() const noexcept
{
    return creatureType->id();
}
