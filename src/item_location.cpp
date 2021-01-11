#include "item_location.h"

#include "debug.h"
#include "item.h"
#include "map_view.h"

ItemLocation::ItemLocation(Position position, uint16_t tileIndex, std::vector<uint16_t> containerIndices)
    : position(position), tileIndex(tileIndex), containerIndices(containerIndices) {}

ItemLocation::ItemLocation(Position position, uint16_t tileIndex)
    : position(position), tileIndex(tileIndex) {}

Item *ItemLocation::item(MapView &mapView)
{
    auto tile = mapView.getTile(position);

    DEBUG_ASSERT(tile != nullptr, "There should (probably) always be a tile here.");

    Item *current = tile->itemAt(tileIndex);
    for (const auto containerIndex : containerIndices)
    {
        current = &current->getOrCreateContainer()->itemAt(containerIndex);
    }

    return current;
}

ContainerLocation::ContainerLocation(Position position, uint16_t tileIndex, const std::vector<uint16_t> &indices)
    : position(position), tileIndex(tileIndex), indices(indices) {}

ContainerLocation::ContainerLocation(Position position, uint16_t tileIndex, std::vector<uint16_t> &&indices)
    : position(position), tileIndex(tileIndex), indices(std::move(indices)) {}

Container *ContainerLocation::container(MapView &mapView)
{
    auto tile = mapView.getTile(position);
    DEBUG_ASSERT(tile != nullptr, "No tile.");

    auto current = tile->itemAt(tileIndex);

    // Skip final container index (it's an index to the item being moved)
    for (auto it = indices.begin(); it < indices.end() - 1; ++it)
    {
        DEBUG_ASSERT(current->isContainer(), "Must be container");
        uint16_t index = *it;
        current = &current->getDataAs<Container>()->itemAt(index);
    }

    return current->getDataAs<Container>();
}

uint16_t ContainerLocation::containerIndex() const
{
    return indices.back();
}