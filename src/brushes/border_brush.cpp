#include "border_brush.h"

#include <algorithm>

#include "../map_view.h"
#include "../util.h"
#include "ground_brush.h"
#include "raw_brush.h"

TileQuadrant BorderBrush::currQuadrant;
std::optional<TileQuadrant> BorderBrush::previousQuadrant;

BorderExpandDirection BorderBrush::currDir;
std::optional<BorderExpandDirection> BorderBrush::prevDir;

namespace TileCoverShortHands
{
    constexpr TileCover None = TileCovers::None;
    constexpr TileCover Full = TileCovers::Full;
    constexpr TileCover North = TileCovers::North;
    constexpr TileCover East = TileCovers::East;
    constexpr TileCover South = TileCovers::South;
    constexpr TileCover West = TileCovers::West;
    constexpr TileCover NorthWest = TileCovers::NorthWest;
    constexpr TileCover NorthEast = TileCovers::NorthEast;
    constexpr TileCover SouthWest = TileCovers::SouthWest;
    constexpr TileCover SouthEast = TileCovers::SouthEast;
    constexpr TileCover NorthWestCorner = TileCovers::NorthWestCorner;
    constexpr TileCover NorthEastCorner = TileCovers::NorthEastCorner;
    constexpr TileCover SouthWestCorner = TileCovers::SouthWestCorner;
    constexpr TileCover SouthEastCorner = TileCovers::SouthEastCorner;

    constexpr TileCover Corners = TileCovers::Corners;
    constexpr TileCover FullNorth = TileCovers::FullNorth;
    constexpr TileCover FullEast = TileCovers::FullEast;
    constexpr TileCover FullSouth = TileCovers::FullSouth;
    constexpr TileCover FullWest = TileCovers::FullWest;
    constexpr TileCover Diagonals = TileCovers::Diagonals;
}; // namespace TileCoverShortHands

bool isRight(TileQuadrant quadrant);
bool isTop(TileQuadrant quadrant);

BorderBrush::BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, Brush *centerBrush)
    : Brush(name), id(id), borderData(borderIds, centerBrush)
{
    initialize();
}

BorderBrush::BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, GroundBrush *centerBrush)
    : BorderBrush(id, name, borderIds, static_cast<Brush *>(centerBrush)) {}

BorderBrush::BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, RawBrush *centerBrush)
    : BorderBrush(id, name, borderIds, static_cast<Brush *>(centerBrush)) {}

BorderBrush::BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds)
    : BorderBrush(id, name, borderIds, static_cast<Brush *>(nullptr)) {}

void BorderBrush::initialize()
{
    for (uint32_t id : borderData.getBorderIds())
    {
        sortedServerIds.emplace_back(id);
    }

    std::sort(sortedServerIds.begin(), sortedServerIds.end());

    // TODO TEMP for debugging, remove!
    borderData.centerBrush = Brush::getGroundBrush("normal_grass");
}

bool BorderBrush::presentAt(const Map &map, const Position position) const
{
    Tile *tile = map.getTile(position);
    if (!tile)
    {
        return false;
    }

    return tile->containsBorder(this);
}

uint32_t BorderBrush::borderItemAt(const Map &map, const Position position) const
{
    Tile *tile = map.getTile(position);
    if (!tile)
    {
        return 0;
    }

    return tile->getBorderServerId(this);
}

TileCover BorderBrush::getTileCoverAt(const Map &map, const Position position) const
{
    Tile *tile = map.getTile(position);
    if (!tile)
    {
        return TILE_COVER_NONE;
    }

    return tile->getTileCover(this);
}

void BorderBrush::apply(MapView &mapView, const Position &position, Direction direction)
{
    const Map &map = *mapView.map();

    NeighborMap neighbors;

    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            TileCover tileCover = getTileCoverAt(map, position + Position(dx, dy, 0));
            neighbors.set(dx, dy, tileCover);
        }
    }

    std::optional<Position> lastPos = mapView.getLastBrushDragPosition();
    if (!lastPos)
    {
        expandCenter(neighbors, *mapView.getLastClickedTileQuadrant());
    }
    else
    {
        previousQuadrant = currQuadrant;
        prevDir = currDir;
        Position expandDirection = position - *lastPos;
        DEBUG_ASSERT(expandDirection != PositionConstants::Zero, "Should not be able to be zero here");

        if (expandDirection.y == -1)
        {
            expandNorth(neighbors);
        }
        else if (expandDirection.x == -1)
        {
            expandWest(neighbors);
        }
    }

    fixBorders(mapView, position, neighbors);
}

void BorderBrush::expandCenter(NeighborMap &neighbors, TileQuadrant tileQuadrant)
{
    using Direction::North, Direction::East, Direction::South, Direction::West;

    prevDir.reset();
    currDir = BorderExpandDirection::None;

    previousQuadrant.reset();
    currQuadrant = tileQuadrant;

    TileCover &cover = neighbors.at(0, 0);
    TileCover originalCover = cover;

    TileCover expanded = TILE_COVER_NONE;

    switch (tileQuadrant)
    {
        case TileQuadrant::TopLeft:
            expanded |= TILE_COVER_NORTH_WEST_CORNER;
            break;
        case TileQuadrant::TopRight:
            expanded |= TILE_COVER_NORTH_EAST_CORNER;
            break;
        case TileQuadrant::BottomRight:
            expanded |= TILE_COVER_SOUTH_EAST_CORNER;
            break;
        case TileQuadrant::BottomLeft:
            expanded |= TILE_COVER_SOUTH_WEST_CORNER;
            break;
    }

    if (expanded != TILE_COVER_NONE)
    {
        cover |= expanded;
        neighbors.expanded = {0, 0, originalCover, true};
    }
}

void BorderBrush::expandNorth(NeighborMap &neighbors)
{
    using BorderExpandDirection::North, BorderExpandDirection::East, BorderExpandDirection::South, BorderExpandDirection::West, BorderExpandDirection::None;
    using TileQuadrant::TopLeft, TileQuadrant::TopRight, TileQuadrant::BottomRight, TileQuadrant::BottomLeft;

    TileQuadrant prevQuadrant = *previousQuadrant;
    currQuadrant = isRight(prevQuadrant) ? BottomRight : BottomLeft;

    auto prevDir = currDir;
    currDir = North;

    TileCover &southCover = neighbors.at(0, 1);
    TileCover originalCover = southCover;

    TileCover expanded = TILE_COVER_NONE;

    if (prevQuadrant == BottomRight)
    {
        if (prevDir == West && !(southCover & TileCoverEast))
        {
            expanded |= TILE_COVER_SOUTH_WEST;
            currQuadrant = BottomLeft;
        }
        else if (southCover & TILE_COVER_SOUTH)
        {
            expanded |= TILE_COVER_SOUTH_EAST;
        }
        else if (southCover & (TILE_COVER_SOUTH_WEST | TILE_COVER_SOUTH_EAST_CORNER))
        {
            expanded |= TILE_COVER_EAST;
        }
    }
    else if (prevQuadrant == BottomLeft)
    {
        if (southCover & TILE_COVER_SOUTH)
        {
            expanded |= TILE_COVER_SOUTH_WEST;
        }
        if (southCover & (TILE_COVER_SOUTH_EAST | TILE_COVER_SOUTH_WEST_CORNER))
        {
            expanded |= TILE_COVER_WEST;
        }
    }

    if (expanded != TILE_COVER_NONE)
    {
        southCover |= expanded;
        neighbors.expanded = {0, 1, originalCover, true};
    }
}

void BorderBrush::expandWest(NeighborMap &neighbors)
{
    using BorderExpandDirection::North, BorderExpandDirection::East, BorderExpandDirection::South, BorderExpandDirection::West, BorderExpandDirection::None;
    using TileQuadrant::TopLeft, TileQuadrant::TopRight, TileQuadrant::BottomRight, TileQuadrant::BottomLeft;

    TileQuadrant prevQuadrant = *previousQuadrant;
    currQuadrant = isTop(prevQuadrant) ? TopRight : BottomRight;

    auto prevDir = currDir;
    currDir = West;

    TileCover &cover = neighbors.at(1, 0);
    TileCover originalCover = cover;

    TileCover expanded = TILE_COVER_NONE;

    if (prevQuadrant == TopRight)
    {
        if (cover & TILE_COVER_EAST)
        {
            expanded |= TILE_COVER_NORTH_EAST;
        }
        else if (cover & (TILE_COVER_SOUTH_EAST | TILE_COVER_NORTH_EAST_CORNER))
        {
            expanded |= TILE_COVER_NORTH;
        }
    }
    else if (prevQuadrant == BottomRight)
    {
        if (cover & TILE_COVER_EAST)
        {
            expanded |= TILE_COVER_SOUTH_EAST;
        }
        else if (cover & (TILE_COVER_NORTH_EAST | TILE_COVER_SOUTH_EAST_CORNER))
        {
            expanded |= TILE_COVER_SOUTH;
        }
    }

    if (expanded != TILE_COVER_NONE)
    {
        cover |= expanded;
        neighbors.expanded = {1, 0, originalCover, true};
    }
}

void BorderBrush::fixBorders(MapView &mapView, const Position &position, NeighborMap &neighbors)
{
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            fixBordersAtOffset(mapView, position, neighbors, dx, dy);
        }
    }
}

void BorderBrush::fixBordersAtOffset(MapView &mapView, const Position &position, NeighborMap &neighbors, int x, int y)
{
    bool debug = position.x == 6 && position.y == 15 && x == 0 && y == 0;
    auto pos = position + Position(x, y, 0);

    auto currentCover = neighbors.at(x, y);
    if (currentCover == TILE_COVER_NONE && !(x == 0 && y == 0))
    {
        return;
    }

    TileCover cover = TILE_COVER_NONE;

    cover |= mirrorNorth(neighbors.at(x, y + 1));
    cover |= mirrorEast(neighbors.at(x - 1, y));
    cover |= mirrorSouth(neighbors.at(x, y - 1));
    cover |= mirrorWest(neighbors.at(x + 1, y));

    cover = unifyTileCover(currentCover | cover);

    if (cover == currentCover)
    {
        bool isExpanded = neighbors.hasExpandedCover() && neighbors.expanded.x == x && neighbors.expanded.y == y;
        if (!isExpanded)
        {
            return;
        }
    }

    neighbors.set(x, y, cover);

    if (cover == TILE_COVER_NONE)
    {
        Tile &tile = mapView.getOrCreateTile(pos);
        mapView.removeItems(tile, [this](const Item &item) {
            return this->includes(item.serverId());
        });

        return;
    }

    std::vector<BorderType> borders;

    if (cover & TILE_COVER_FULL)
    {
        borders.emplace_back(BorderType::Center);
    }
    else
    {
        if (cover & TILE_COVER_NORTH)
        {
            borders.emplace_back(BorderType::North);
        }
        if (cover & TILE_COVER_EAST)
        {
            borders.emplace_back(BorderType::East);
        }
        if (cover & TILE_COVER_SOUTH)
        {
            borders.emplace_back(BorderType::South);
        }
        if (cover & TILE_COVER_WEST)
        {
            borders.emplace_back(BorderType::West);
        }

        // Diagonals
        if (cover & TileCoverDiagonals)
        {
            bool north = cover & TileCoverNorth;
            bool east = cover & TileCoverEast;
            bool south = cover & TileCoverSouth;
            bool west = cover & TileCoverWest;

            if (north && east)
            {
                if (south)
                {
                    borders.emplace_back(BorderType::South);
                    borders.emplace_back(BorderType::NorthEastDiagonal);
                }
                else if (west)
                {
                    borders.emplace_back(BorderType::East);
                    borders.emplace_back(BorderType::NorthWestDiagonal);
                }
                else
                {
                    borders.emplace_back(BorderType::NorthEastDiagonal);
                }
            }
            else if (south && west)
            {
                if (north)
                {
                    borders.emplace_back(BorderType::South);
                    borders.emplace_back(BorderType::NorthWestDiagonal);
                }
                else if (east)
                {
                    borders.emplace_back(BorderType::West);
                    borders.emplace_back(BorderType::SouthEastDiagonal);
                }
                else
                {
                    borders.emplace_back(BorderType::SouthWestDiagonal);
                }
            }
            else if (north && west)
            {
                borders.emplace_back(BorderType::NorthWestDiagonal);
            }
            else if (south && east)
            {
                borders.emplace_back(BorderType::SouthEastDiagonal);
            }
        }

        // Corners
        if (cover & TileCoverCorner)
        {
            if (cover & TILE_COVER_NORTH_EAST_CORNER)
            {
                borders.emplace_back(BorderType::NorthEastCorner);
            }
            if (cover & TILE_COVER_NORTH_WEST_CORNER)
            {
                borders.emplace_back(BorderType::NorthWestCorner);
            }
            if (cover & TILE_COVER_SOUTH_EAST_CORNER)
            {
                borders.emplace_back(BorderType::SouthEastCorner);
            }
            if (cover & TILE_COVER_SOUTH_WEST_CORNER)
            {
                borders.emplace_back(BorderType::SouthWestCorner);
            }
        }
    }

    DEBUG_ASSERT(borders.size() > 0, "Should always have at least one border here?");
    Tile &tile = mapView.getOrCreateTile(pos);
    mapView.removeItems(tile, [this](const Item &item) {
        return this->includes(item.serverId());
    });

    for (auto border : borders)
    {
        apply(mapView, pos, border);
    }
}

void BorderBrush::apply(MapView &mapView, const Position &position, BorderType borderType)
{
    if (borderType == BorderType::Center)
    {
        if (borderData.centerBrush)
        {
            borderData.centerBrush->apply(mapView, position, Direction::South);
        }
    }
    else
    {
        mapView.addItem(position, borderData.getServerId(borderType));
    }
}

bool BorderBrush::includes(uint32_t serverId) const
{
    bool asBorder = std::binary_search(sortedServerIds.begin(), sortedServerIds.end(), serverId);
    return asBorder || (borderData.centerBrush && borderData.centerBrush->erasesItem(serverId));
}

bool BorderBrush::erasesItem(uint32_t serverId) const
{
    return includes(serverId);
}

BrushType BorderBrush::type() const
{
    return BrushType::Border;
}

const std::string BorderBrush::getDisplayId() const
{
    return id;
}

std::vector<ThingDrawInfo> BorderBrush::getPreviewTextureInfo(Direction direction) const
{
    // TODO improve preview
    return std::vector<ThingDrawInfo>{DrawItemType(_iconServerId, PositionConstants::Zero)};
}

uint32_t BorderBrush::iconServerId() const
{
    return _iconServerId;
}

void BorderBrush::setIconServerId(uint32_t serverId)
{
    _iconServerId = serverId;
}

std::string BorderBrush::brushId() const noexcept
{
    return id;
}

const std::vector<uint32_t> &BorderBrush::serverIds() const
{
    return sortedServerIds;
}

Brush *BorderBrush::centerBrush() const
{
    return borderData.centerBrush;
}

BorderType BorderData::getBorderType(uint32_t serverId) const
{
    for (int i = 0; i < borderIds.size(); ++i)
    {
        if (borderIds[i] == serverId)
        {
            return static_cast<BorderType>(i + 1);
        }
    }

    if (centerBrush && centerBrush->erasesItem(serverId))
    {
        return BorderType::Center;
    }
    else
    {
        return BorderType::None;
    }
}

BorderType BorderBrush::getBorderType(uint32_t serverId) const
{
    return borderData.getBorderType(serverId);
}

TileCover BorderBrush::getTileCover(uint32_t serverId) const
{
    BorderType borderType = getBorderType(serverId);
    return borderTypeToTileCover[to_underlying(borderType)];
}

uint32_t BorderData::getServerId(BorderType borderType) const
{
    // -1 because index zero in BorderType is BorderType::None
    return borderIds[to_underlying(borderType) - 1];
}

std::array<uint32_t, 12> BorderData::getBorderIds() const
{
    return borderIds;
}

bool BorderData::is(uint32_t serverId, BorderType borderType) const
{
    if (borderType == BorderType::Center)
    {
        return centerBrush && centerBrush->erasesItem(serverId);
    }

    return getServerId(borderType) == serverId;
}

bool NeighborMap::isExpanded(int x, int y) const
{
    auto found = std::find_if(expandedCovers.begin(), expandedCovers.end(), [x, y](const ExpandedTileBlock &block) { return block.x == x && block.y == y; });
    return found != expandedCovers.end();
}

bool NeighborMap::hasExpandedCover() const noexcept
{
    return !expandedCovers.empty();
}

void NeighborMap::addExpandedCover(int x, int y, TileCover originalCover)
{
    expandedCovers.emplace_back(ExpandedTileBlock{x, y, originalCover});
}

TileCover &NeighborMap::at(int x, int y)
{
    return data[index(x, y)];
}

TileCover &NeighborMap::at(int x, int y)
{
    return data[index(x, y)];
}

void NeighborMap::set(int x, int y, TileCover tileCover)
{
    data[index(x, y)] = unifyTileCover(tileCover);
}

int NeighborMap::index(int x, int y) const
{
    return (y + 2) * 5 + (x + 2);
}

bool isRight(TileQuadrant quadrant)
{
    return quadrant == TileQuadrant::TopRight || quadrant == TileQuadrant::BottomRight;
}
bool isTop(TileQuadrant quadrant)
{
    return quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::TopRight;
}