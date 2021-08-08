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

void CreatureBrush::erase(MapView &mapView, const Position &position)
{
    Tile *tile = mapView.getTile(position);
    if (!tile)
    {
        return;
    }

    Creature *creature = tile->creature();
    if (creature && (&creature->creatureType == this->creatureType))
    {
        tile->removeCreature();
    }
}

void CreatureBrush::apply(MapView &mapView, const Position &position)
{
    auto creature = Creature(*creatureType);
    Direction direction = MapView::getDirection(mapView.getBrushVariation());
    creature.setDirection(direction);
    mapView.addCreature(position, std::move(creature));
}

BrushType CreatureBrush::type() const
{
    return BrushType::Creature;
}

int CreatureBrush::variationCount() const
{
    // 4 directions
    return 4;
}

std::vector<ThingDrawInfo> CreatureBrush::getPreviewTextureInfo(int variation) const
{
    auto direction = MapView::getDirection(variation);
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
