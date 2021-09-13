#include "wall_brush.h"

#include "../items.h"
#include "../map_view.h"
#include "../random.h"
#include "../settings.h"

WallBrush::WallBrush(std::string id, const std::string &name, StraightPart &&horizontal, StraightPart &&vertical, Part &&corner, Part &&pole)
    : Brush(name), id(id), horizontal(std::move(horizontal)), vertical(std::move(vertical)), corner(std::move(corner)), pole(std::move(pole))
{
#define add_to_ids(xs)    \
    for (auto &item : xs) \
        _serverIds.emplace(item.id);

    add_to_ids(this->horizontal.doors);
    add_to_ids(this->horizontal.windows);

    add_to_ids(this->vertical.doors);
    add_to_ids(this->vertical.windows);
#undef add_to_ids

    auto initializeWeights = [this](Part *part) {
        for (auto &entry : part->items)
        {
            part->totalWeight += entry.weight;
            entry.weight = part->totalWeight;
            _serverIds.emplace(entry.id);
        }
    };

    initializeWeights(&this->horizontal);
    initializeWeights(&this->vertical);
    initializeWeights(&this->corner);
    initializeWeights(&this->pole);
}

void WallBrush::erase(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);

    mapView.removeItems(tile, [this](const Item &item) { return this->erasesItem(item.serverId()); });

    if (Settings::AUTO_BORDER)
    {
        auto neighbors = WallNeighborMap(this, position, *mapView.map());
        // neighbors.set(0, 0, false);

        connectWalls(mapView, neighbors, position);
    }
}

void WallBrush::apply(MapView &mapView, const Position &position)
{
    if (Settings::AUTO_BORDER)
    {
        auto neighbors = WallNeighborMap(this, position, *mapView.map());
        neighbors.set(0, 0, true);

        connectWalls(mapView, neighbors, position);
    }
    else
    {
        uint32_t serverId = sampleServerId(WallType::Pole);
        if (Items::items.getItemTypeByServerId(serverId)->isBottom())
        {
            mapView.setBottomItem(position, serverId);
        }
        else
        {
            mapView.removeItems(position, [this](const Item &item) {
                return this->includes(item.serverId());
            });

            mapView.addItem(position, serverId);
        }
    }
}

void WallBrush::connectWalls(MapView &mapView, WallNeighborMap &neighbors, const Position &center)
{
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            if (!neighbors.at(x, y))
            {
                continue;
            }

            auto pos = center + Position(x, y, 0);

            WallType wallType = WallType::Pole;
            bool hasLeft = neighbors.at(x - 1, y);
            if (hasLeft)
            {
                bool hasTop = neighbors.at(x, y - 1);
                wallType = hasTop ? WallType::Corner : WallType::Horizontal;
            }
            else
            {
                bool hasTop = neighbors.at(x, y - 1);
                if (hasTop)
                {
                    wallType = WallType::Vertical;
                }
            }

            uint32_t serverId = sampleServerId(wallType);
            if (Items::items.getItemTypeByServerId(serverId)->isBottom())
            {
                mapView.setBottomItem(pos, serverId);
            }
            else
            {
                mapView.removeItems(pos, [this](const Item &item) {
                    return this->includes(item.serverId());
                });

                mapView.addItem(pos, serverId);
            }
        }
    }
}

const WallBrush::Part &WallBrush::getPart(WallType type) const
{
    switch (type)
    {
        case WallType::Horizontal:
            return horizontal;
        case WallType::Vertical:
            return vertical;
        case WallType::Corner:
            return corner;
        case WallType::Pole:
            return pole;
        default:
            return pole;
    }
}

uint32_t WallBrush::sampleServerId(WallType type) const
{
    const Part &part = getPart(type);

    uint32_t weight = Random::global().nextInt<uint32_t>(static_cast<uint32_t>(0), part.totalWeight);

    for (const auto &entry : part.items)
    {
        if (weight < entry.weight)
        {
            return entry.id;
        }
    }

    // If we get here, something is off with `weight` for some weighted ID or with `totalWeight`.
    VME_LOG_ERROR(
        "[WallBrush::sampleServerId] Brush " << _name
                                             << ": Could not find matching weight for randomly generated weight "
                                             << weight << " (totalWeight: " << part.totalWeight << ".");

    // No match in for-loop. Use the first ID.
    return part.items.at(0).id;
}

std::vector<ThingDrawInfo> WallBrush::getPreviewTextureInfo(int variation) const
{
    return std::vector<ThingDrawInfo>{DrawItemType(_iconServerId, PositionConstants::Zero)};
}

std::vector<ThingDrawInfo> WallBrush::getPreviewTextureInfo(Position from, Position to) const
{
    std::vector<ThingDrawInfo> result;

    auto add = [&result, &from](uint32_t serverId, Position position) {
        result.emplace_back(DrawItemType(serverId, position - from));
    };

    uint32_t verticalServerId = vertical.items.at(0).id;
    uint32_t horizontalServerId = horizontal.items.at(0).id;
    uint32_t poleServerId = pole.items.at(0).id;
    uint32_t cornerServerId = corner.items.at(0).id;

    if (from == to)
    {
        add(poleServerId, from);
        return result;
    }

    int minX = std::min(from.x, to.x);
    int maxX = std::max(from.x, to.x);

    int minY = std::min(from.y, to.y);
    int maxY = std::max(from.y, to.y);

    int z = from.z;

    for (int x = minX + 1; x < maxX; ++x)
    {
        add(horizontalServerId, Position(x, from.y, z));
        add(horizontalServerId, Position(x, to.y, z));
    }

    for (int y = minY + 1; y < maxY; ++y)
    {
        add(verticalServerId, Position(minX, y, z));
        add(verticalServerId, Position(maxX, y, z));
    }

    bool verticalLine = minX == maxX;
    bool horizontalLine = minY == maxY;

    // Pole (top-left)
    add(poleServerId, Position(minX, minY, z));

    // Horizontal (top-right)
    if (!verticalLine)
    {
        add(horizontalServerId, Position(maxX, minY, z));
    }

    // Corner (bottom-right)
    add(horizontalLine ? horizontalServerId : cornerServerId, Position(maxX, maxY, z));

    // Vertical (bottom-left)
    if (!horizontalLine)
    {
        add(verticalServerId, Position(minX, maxY, z));
    }

    return result;
}

void WallBrush::applyInRectangleArea(MapView &mapView, const Position &from, const Position &to)
{
    int minX = std::min(from.x, to.x);
    int maxX = std::max(from.x, to.x);

    int minY = std::min(from.y, to.y);
    int maxY = std::max(from.y, to.y);

    int z = from.z;

    // Top
    for (int x = minX; x <= maxX; ++x)
    {
        apply(mapView, Position(x, minY, z));
    }

    // Right
    for (int y = minY + 1; y < maxY; ++y)
    {
        apply(mapView, Position(maxX, y, z));
    }

    // Bottom
    for (int x = minX; x <= maxX; ++x)
    {
        apply(mapView, Position(x, maxY, z));
    }

    // Left
    for (int y = minY + 1; y < maxY; ++y)
    {
        apply(mapView, Position(minX, y, z));
    }
}

const std::string WallBrush::getDisplayId() const
{
    return id;
}

bool WallBrush::erasesItem(uint32_t serverId) const
{
    return _serverIds.contains(serverId);
}

bool WallBrush::includes(uint32_t serverId) const
{
    return erasesItem(serverId);
}

BrushType WallBrush::type() const
{
    return BrushType::Wall;
}

std::string WallBrush::brushId() const noexcept
{
    return id;
}

void WallBrush::finalize()
{
    for (auto serverId : _serverIds)
    {
        ItemType *itemType = Items::items.getItemTypeByServerId(serverId);
        if (itemType)
        {
            itemType->addBrush(this);
        }
    }
}

void WallBrush::setIconServerId(uint32_t serverId)
{
    _iconServerId = serverId;
}

uint32_t WallBrush::iconServerId() const
{
    return _iconServerId;
}

WallNeighborMap::WallNeighborMap(const WallBrush *brush, const Position &position, const Map &map)
{
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            Tile *tile = map.getTile(position + Position(dx, dy, 0));
            if (tile && tile->hasWall(brush))
            {
                set(dx, dy, true);
            }
        }
    }
}

bool WallNeighborMap::at(int x, int y) const
{
    return data[index(x, y)];
}

void WallNeighborMap::set(int x, int y, bool value)
{
    data[index(x, y)] = value;
}

int WallNeighborMap::index(int x, int y) const
{
    return (y + 2) * 5 + (x + 2);
}