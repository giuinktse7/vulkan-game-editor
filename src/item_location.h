#pragma once

#include <vector>

#include "position.h"

class Item;
class MapView;
struct Container;

struct ItemLocation
{
    ItemLocation(Position position, uint16_t tileIndex, std::vector<uint16_t> containerIndices);
    ItemLocation(Position position, uint16_t tileIndex);

    Item *item(MapView &mapView);

    Position position;
    uint16_t tileIndex;
    std::vector<uint16_t> containerIndices;
};

struct ContainerLocation
{
    ContainerLocation(Position position, uint16_t tileIndex, const std::vector<uint16_t> &indices);
    ContainerLocation(Position position, uint16_t tileIndex, std::vector<uint16_t> &&indices);

    Position position;
    uint16_t tileIndex;
    /*
          The last index is the index of the item in the final container.
        */
    std::vector<uint16_t> indices;

    uint16_t containerIndex() const;

    Container *container(MapView &mapView);
};
