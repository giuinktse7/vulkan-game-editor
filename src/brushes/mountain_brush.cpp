#include "mountain_brush.h"

#include "../map.h"
#include "../map_view.h"
#include "ground_brush.h"
#include "raw_brush.h"

MountainBrush::MountainBrush(std::string id, std::string name, LazyGroundBrush ground, MountainPart::InnerWall innerWall, uint32_t iconServerId)
    : Brush(name), _id(id), _ground(ground), innerWall(innerWall), _iconServerId(iconServerId)
{
    initialize();
}

void MountainBrush::initialize()
{
    _serverIds.insert(innerWall.east);
    _serverIds.insert(innerWall.south);
    _serverIds.insert(innerWall.southEast);

    // All innerwall parts must not be set, remove the 0 created by one if it was not set
    _serverIds.erase(0);

    auto brush = ground();
    switch (brush->type())
    {
        case BrushType::Raw:
            _serverIds.insert(static_cast<RawBrush *>(brush)->serverId());
            break;
        case BrushType::Ground:
            for (const uint32_t serverId : static_cast<GroundBrush *>(brush)->serverIds())
            {
                _serverIds.insert(serverId);
            }
            break;
        default:
            break;
    }
}

const std::unordered_set<uint32_t> &MountainBrush::serverIds() const
{
    return _serverIds;
}

void MountainBrush::apply(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);
    mapView.setGround(tile, Item(nextGroundServerId()));

    MountainNeighborMap neighbors = MountainNeighborMap(_ground.get(), position, *mapView.map());

    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            borderize(mapView, neighbors, dx, dy);
        }
    }
}

Brush *MountainBrush::ground() const noexcept
{
    return _ground.get();
}

uint32_t MountainBrush::iconServerId() const
{
    return _iconServerId;
}

uint32_t MountainBrush::nextGroundServerId()
{
    Brush *groundBrush = ground();
    switch (groundBrush->type())
    {
        case BrushType::Raw:
            return static_cast<RawBrush *>(groundBrush)->serverId();
        case BrushType::Ground:
            return static_cast<GroundBrush *>(groundBrush)->nextServerId();
        default:
            ABORT_PROGRAM("Invalid ground brush type in MountainBrush");
    }
}

void MountainBrush::borderize(MapView &mapView, MountainNeighborMap &neighbors, int dx, int dy)
{
    const auto &center = neighbors.at(dx, dy);

    Position pos = neighbors.position + Position(dx, dy, 0);

    if (mapView.hasTile(pos))
    {
        mapView.removeItems(pos, [this](const Item &item) { return innerWall.contains(item.serverId()); });
    }

    if (center.hasMountainGround)
    {
        // Make sure the tile is created
        if (!mapView.hasTile(pos))
        {
            mapView.createTile(pos);
        }

        bool innerEast = !neighbors.at(dx + 1, dy).hasMountainGround;
        bool innerSouth = !neighbors.at(dx, dy + 1).hasMountainGround;

        if (innerEast && innerSouth)
        {
            mapView.addItem(pos, innerWall.southEast);
        }
        else if (innerEast)
        {
            mapView.addItem(pos, innerWall.east);
        }
        else if (innerSouth)
        {
            mapView.addItem(pos, innerWall.south);
        }
    }
}

void MountainBrush::erase(MapView &mapView, const Position &position)
{
    // TODO
}

bool MountainBrush::erasesItem(uint32_t serverId) const
{
    // TODO
    return innerWall.contains(serverId);
}

BrushType MountainBrush::type() const
{
    return BrushType::Mountain;
}

std::vector<ThingDrawInfo> MountainBrush::getPreviewTextureInfo(int variation) const
{
    return ground()->getPreviewTextureInfo(variation);
}

const std::string MountainBrush::getDisplayId() const
{
    return _id;
}

const std::string &MountainBrush::id() const noexcept
{
    return _id;
}

void MountainBrush::setGround(RawBrush *brush)
{
    this->_ground = brush;
}

void MountainBrush::setGround(GroundBrush *brush)
{
    this->_ground = brush;
}

void MountainBrush::setGround(const std::string &groundBrushId)
{
}

MountainNeighborMap::MountainNeighborMap(Brush *ground, const Position &position, const Map &map)
    : position(position), ground(ground)
{
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            Tile *tile = map.getTile(position + Position(dx, dy, 0));
            Entry entry;

            entry.hasMountainGround = tile && tile->hasGround() && ground->erasesItem(tile->ground()->itemType->id);
            set(dx, dy, entry);
        }
    }
}

void MountainNeighborMap::set(int x, int y, value_type entry)
{
    data[index(x, y)] = entry;
}

bool MountainPart::InnerWall::contains(uint32_t serverId) const noexcept
{
    return south == serverId || east == serverId || southEast == serverId;
}

LazyGroundBrush::LazyGroundBrush(RawBrush *brush)
{
    data = brush;
}

LazyGroundBrush::LazyGroundBrush(GroundBrush *brush)
{
    data = brush;
}

LazyGroundBrush::LazyGroundBrush(std::string groundBrushId)
{
    data = groundBrushId;
}

Brush *LazyGroundBrush::get() const
{
    if (std::holds_alternative<std::string>(data))
    {
        const std::string &id = std::get<std::string>(data);
        GroundBrush *brush = Brush::getGroundBrush(id);
        if (!brush)
        {
            ABORT_PROGRAM(std::format("Attempted to retrieve a GroundBrush with id '{}' from a LazyGroundBrush, but the brush did not exist.", id));
        }

        data = brush;
        return brush;
    }
    else if (std::holds_alternative<RawBrush *>(data))
    {
        return std::get<RawBrush *>(data);
    }
    else if (std::holds_alternative<GroundBrush *>(data))
    {
        return std::get<GroundBrush *>(data);
    }
    else
    {
        ABORT_PROGRAM("Unknown variant in LazyGroundBrush");
    }
}

MountainNeighborMap::value_type MountainNeighborMap::at(int x, int y) const
{
    return data[index(x, y)];
}

MountainNeighborMap::value_type &MountainNeighborMap::at(int x, int y)
{
    return data[index(x, y)];
}

int MountainNeighborMap::index(int x, int y) const
{
    return (y + 2) * 5 + (x + 2);
}