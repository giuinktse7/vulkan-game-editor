#include "mountain_brush.h"

#include "../map.h"
#include "../map_view.h"
#include "../settings.h"
#include "../tile_cover.h"
#include "ground_brush.h"
#include "raw_brush.h"

MountainBrush::MountainBrush(std::string id, std::string name, LazyGroundBrush ground, MountainPart::InnerWall innerWall, BorderData mountainBorders, uint32_t iconServerId)
    : Brush(name), _id(id), _ground(ground), innerWall(innerWall), mountainBorders(mountainBorders), _iconServerId(iconServerId)
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

    for (const uint32_t serverId : mountainBorders.getBorderIds())
    {
        _serverIds.insert(serverId);
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

    if (Settings::AUTO_BORDER)
    {
        MountainGroundNeighborMap neighbors = MountainGroundNeighborMap(_ground.get(), position, *mapView.map());

        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                borderize(mapView, neighbors, dx, dy);
            }
        }
    }

    // postBorderizePass(mapView, position);
}

// void MountainBrush::postBorderizePass(MapView &mapView, const Position &position)
// {
//     using namespace TileCoverShortHands;
//     auto neighbors = MountainWallNeighborMap(this, position, *mapView.map());

//     for (int dy = -2; dy <= 2; ++dy)
//     {
//         for (int dx = -2; dx <= 2; ++dx)
//         {
//             Position pos = position + Position(dx, dy, 0);
//             TileCover &center = neighbors.at(dx, dy);
//             if ((center & SouthWest) && dx < 2)
//             {
//                 TileCover east = neighbors.at(dx + 1, dy);
//                 if (!(east & (SouthEast | South | SouthWestCorner)))
//                 {
//                     center &= ~SouthWest;
//                     center |= South;
//                     center |= West;

//                     Tile *tile = mapView.getTile(pos);
//                     mapView.removeItems(*tile, [this](const Item &item) {
//                         return item.serverId() == mountainBorders.getServerId(BorderType::SouthWestDiagonal);
//                     });

//                     apply(mapView, pos, BorderType::West);
//                     apply(mapView, pos, BorderType::South);
//                 }
//             }
//         }
//     }
// }

void MountainBrush::apply(MapView &mapView, const Position &position, BorderType borderType)
{
    auto borderItemId = mountainBorders.getServerId(borderType);
    if (borderItemId)
    {
        mapView.addItem(position, *borderItemId);
    }
}

BorderType MountainBrush::getBorderType(uint32_t serverId) const
{
    return mountainBorders.getBorderType(serverId);
}

TileCover MountainBrush::getTileCover(uint32_t serverId) const
{
    BorderType borderType = getBorderType(serverId);
    return BorderData::borderTypeToTileCover[to_underlying(borderType)];
}

Brush *MountainBrush::ground() const noexcept
{
    if (!_ground.evaluated)
    {
        auto groundBrush = _ground.get();
        switch (groundBrush->type())
        {
            case BrushType::Raw:
                _serverIds.insert(static_cast<RawBrush *>(groundBrush)->serverId());
                break;
            case BrushType::Ground:
                for (uint32_t serverId : static_cast<GroundBrush *>(groundBrush)->serverIds())
                {
                    _serverIds.insert(serverId);
                }
                break;
            default:
                break;
        }

        return groundBrush;
    }

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

TileCover unifyTileCover(TileCover cover)
{
    using namespace TileCoverShortHands;

    TileCover clear = TILE_COVER_NONE;

    auto clearIf = [&clear, &cover](TileCover req, TileCover flags) {
        if (cover & req)
            clear |= flags;
    };

    auto replace = [&clear, &cover](TileCover a, TileCover b) {
        if ((cover & a) == a)
        {
            clear |= a;
            cover |= b;
        }
    };

    clearIf(FullNorth, NorthWestCorner | NorthEastCorner);
    clearIf(FullEast, NorthEastCorner | SouthEastCorner);
    clearIf(FullSouth, SouthWestCorner | SouthEastCorner);
    clearIf(FullWest, NorthWestCorner | SouthWestCorner);

    replace(North | West, NorthWest);
    replace(North | East, NorthEast);
    replace(South | East, SouthEast);
    replace(South | West, SouthWest);

    cover &= ~clear;
    return cover;
}

void MountainBrush::borderize(MapView &mapView, MountainGroundNeighborMap &neighbors, int dx, int dy)
{
    using namespace TileCoverShortHands;

    const auto &center = neighbors.at(dx, dy);

    Position pos = neighbors.position + Position(dx, dy, 0);

    if (mapView.hasTile(pos))
    {
        // Remove current mountain borders on the tile
        mapView.removeItems(pos, [this](const Item &item) {
            uint32_t serverId = item.serverId();
            return erasesItem(serverId) && !ground()->erasesItem(serverId); });
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
    else
    {
        TileCover cover = neighbors.getTileCover(dx, dy);
        cover = unifyTileCover(cover);

        // Special cases
        if (!Settings::PLACE_MOUNTAIN_FEATURES)
        {
            cover &= ~MountainBrush::features;

            if ((cover & SouthWest) && !(neighbors.at(dx + 1, dy + 1).hasMountainGround || neighbors.at(dx + 1, dy).hasMountainGround))
            {
                cover &= ~SouthWest;
                if (!(cover & FullSouth))
                {
                    cover |= South;
                }
            }

            if ((cover & NorthEast) && !(neighbors.at(dx + 1, dy + 1).hasMountainGround || neighbors.at(dx, dy + 1).hasMountainGround))
            {
                cover &= ~NorthEast;
                if (!(cover & FullEast))
                {
                    cover |= East;
                }
            }
        }

        bool hasFeature = false;

        auto applyIf = [this, &cover, &mapView, &pos, &hasFeature](TileCover req, BorderType borderType, bool feature = false) {
            if (cover & req)
            {
                if (feature)
                {
                    if (!hasFeature)
                    {
                        apply(mapView, pos, borderType);
                    }

                    hasFeature = true;
                }
                else
                {
                    apply(mapView, pos, borderType);
                }
            }
        };

        // Sides
        applyIf(North, BorderType::North, true);
        applyIf(West, BorderType::West, true);

        applyIf(NorthWest, BorderType::NorthWestDiagonal, true);

        // Corners
        if (cover & Corners)
        {
            applyIf(NorthEastCorner, BorderType::NorthEastCorner, true);
            applyIf(NorthWestCorner, BorderType::NorthWestCorner, true);
            applyIf(SouthWestCorner, BorderType::SouthWestCorner, true);
        }

        // Diagonals
        if (cover & Diagonals)
        {
            applyIf(NorthEast, BorderType::NorthEastDiagonal);
            applyIf(SouthWest, BorderType::SouthWestDiagonal);
            applyIf(SouthEast, BorderType::SouthEastDiagonal);
        }

        applyIf(East, BorderType::East);
        applyIf(South, BorderType::South);

        applyIf(SouthEastCorner, BorderType::SouthEastCorner);
    }
}

bool MountainBrush::isFeature(TileCover cover)
{
    DEBUG_ASSERT(TileCovers::exactlyOneSet(cover), "MountainBrush::isFeature: cover must be exactly one cover type.");

    return MountainBrush::features & cover;
}

void MountainBrush::erase(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);
    mapView.removeItems(tile, [this](const Item &item) { return this->erasesItem(item.serverId()); });
}

bool MountainBrush::erasesItem(uint32_t serverId) const
{
    return _serverIds.contains(serverId);
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
    this->_ground = LazyGroundBrush(brush);
}

void MountainBrush::setGround(GroundBrush *brush)
{
    this->_ground = LazyGroundBrush(brush);
}

void MountainBrush::setGround(const std::string &groundBrushId)
{
    this->_ground = LazyGroundBrush(groundBrushId);
}

MountainWallNeighborMap::MountainWallNeighborMap(MountainBrush *mountainBrush, const Position &position, const Map &map)
    : position(position)
{
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            Tile *tile = map.getTile(position + Position(dx, dy, 0));
            if (!tile)
            {
                set(dx, dy, TILE_COVER_NONE);
            }
            else
            {
                TileCover cover = tile->getTileCover(mountainBrush);
                cover = unifyTileCover(cover);
                set(dx, dy, cover);
            }
        }
    }
}

TileCover MountainWallNeighborMap::at(int x, int y) const
{
    return data[index(x, y)];
}

TileCover &MountainWallNeighborMap::at(int x, int y)
{
    return data[index(x, y)];
}

void MountainWallNeighborMap::set(int x, int y, TileCover cover)
{
    data[index(x, y)] = cover;
}

int MountainWallNeighborMap::index(int x, int y) const
{
    return (y + 2) * 5 + (x + 2);
}

MountainGroundNeighborMap::MountainGroundNeighborMap(Brush *ground, const Position &position, const Map &map)
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

void MountainGroundNeighborMap::set(int x, int y, value_type entry)
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
    evaluated = true;
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

MountainGroundNeighborMap::value_type MountainGroundNeighborMap::at(int x, int y) const
{
    return data[index(x, y)];
}

MountainGroundNeighborMap::value_type &MountainGroundNeighborMap::at(int x, int y)
{
    return data[index(x, y)];
}

int MountainGroundNeighborMap::index(int x, int y) const
{
    return (y + 2) * 5 + (x + 2);
}

TileCover MountainGroundNeighborMap::getTileCover(int dx, int dy) const noexcept
{
    using namespace TileCoverShortHands;

    TileCover cover = TILE_COVER_NONE;

    auto includeIfGround = [this, dx, dy, &cover](int offsetX, int offsetY, TileCover part) {
        if (at(dx + offsetX, dy + offsetY).hasMountainGround)
        {
            cover |= part;
        }
    };

    includeIfGround(-1, -1, NorthWestCorner);
    includeIfGround(0, -1, North);
    includeIfGround(1, -1, NorthEastCorner);

    includeIfGround(-1, 0, West);
    includeIfGround(1, 0, East);

    includeIfGround(-1, 1, SouthWestCorner);
    includeIfGround(0, 1, South);
    includeIfGround(1, 1, SouthEastCorner);

    return cover;
}
