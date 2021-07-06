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

TileQuadrant BorderBrush::getQuadrant(int dx, int dy)
{
    if (dx == 0 && dy == 0)
        return currQuadrant;

    if (dx < 0)
    {
        return dy < 0 ? TileQuadrant::BottomRight : TileQuadrant::TopRight;
    }
    else
    {
        return dy < 0 ? TileQuadrant::BottomLeft : TileQuadrant::TopLeft;
    }
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
            TileQuadrant quadrant = getQuadrant(dx, dy);
            tileCover = TileCovers::unifyTileCover(tileCover, quadrant);
            auto diagonals = tileCover & TileCovers::Diagonals;
            DEBUG_ASSERT((diagonals == TileCovers::None) || TileCovers::exactlyOneSet(diagonals & TileCovers::Diagonals), "Preferred diagonal must be None or have exactly one diagonal set.");

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

        if (expandDirection.x == -1)
        {
            expandWest(neighbors);
        }
        else if (expandDirection.x == 1)
        {
            expandEast(neighbors);
        }
        else if (expandDirection.y == -1)
        {
            expandNorth(neighbors);
        }
        else if (expandDirection.y == 1)
        {
            expandSouth(neighbors);
        }
    }

    fixBorders(mapView, position, neighbors);
}

void BorderBrush::expandCenter(NeighborMap &neighbors, TileQuadrant tileQuadrant)
{
    using namespace TileCoverShortHands;

    prevDir.reset();
    currDir = BorderExpandDirection::None;

    previousQuadrant.reset();
    currQuadrant = tileQuadrant;

    TileCover &cover = neighbors.at(0, 0);
    TileCover originalCover = cover;

    TileCover expanded = None;

    switch (tileQuadrant)
    {
        case TileQuadrant::TopLeft:
            expanded |= NorthWestCorner;
            break;
        case TileQuadrant::TopRight:
            expanded |= NorthEastCorner;
            break;
        case TileQuadrant::BottomRight:
            expanded |= SouthEastCorner;
            break;
        case TileQuadrant::BottomLeft:
            expanded |= SouthWestCorner;
            break;
    }

    if (expanded != None)
    {
        cover |= expanded;
        neighbors.addExpandedCover(0, 0, originalCover);
    }
}

void BorderBrush::expandNorth(NeighborMap &neighbors)
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;
    using TileQuadrant::TopLeft, TileQuadrant::TopRight, TileQuadrant::BottomRight, TileQuadrant::BottomLeft;

    TileQuadrant prevQuadrant = *previousQuadrant;
    currQuadrant = isRight(prevQuadrant) ? BottomRight : BottomLeft;

    currDir = Dir::North;

    TileCover &southCover = neighbors.at(0, 1);
    TileCover originalCover = southCover;

    TileCover expanded = None;

    if (prevQuadrant == BottomRight)
    {
        if (prevDir == Dir::West && !(southCover & FullEast))
        {
            expanded |= SouthWest;
            currQuadrant = BottomLeft;
        }
        else if (southCover & South)
        {
            expanded |= SouthEast;
        }
        else if (southCover & (SouthWest | SouthEastCorner))
        {
            expanded |= East;
        }
    }
    else if (prevQuadrant == BottomLeft)
    {
        if (prevDir == Dir::East & !(southCover & FullWest))
        {
            expanded |= SouthEast;
            currQuadrant = BottomRight;
        }
        else if (southCover & South)
        {
            expanded |= SouthWest;
        }
        else if (southCover & (SouthEast | SouthWestCorner))
        {
            expanded |= West;
        }
    }

    if (expanded != None)
    {
        southCover |= expanded;
        southCover = TileCovers::unifyTileCover(southCover, currQuadrant);
        neighbors.addExpandedCover(0, 1, originalCover);
    }
}

void BorderBrush::expandSouth(NeighborMap &neighbors)
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;
    using TileQuadrant::TopLeft, TileQuadrant::TopRight, TileQuadrant::BottomRight, TileQuadrant::BottomLeft;

    TileQuadrant prevQuadrant = *previousQuadrant;
    currQuadrant = isRight(prevQuadrant) ? TopRight : TopLeft;

    currDir = Dir::South;

    TileCover &northCover = neighbors.at(0, -1);
    TileCover originalCover = northCover;

    TileCover expanded = None;

    if (prevQuadrant == TopRight)
    {
        if (prevDir == Dir::West && !(northCover & FullEast))
        {
            expanded |= NorthWest;
            currQuadrant = TopLeft;
        }
        else if (northCover & North)
        {
            expanded |= NorthEast;
        }
        else if (northCover & (NorthWest | NorthEastCorner))
        {
            expanded |= East;
        }
    }
    else if (prevQuadrant == TopLeft)
    {
        if (prevDir == Dir::East && !(northCover & FullWest))
        {
            expanded |= NorthEast;
            currQuadrant = TopRight;
        }
        else if (northCover & North)
        {
            expanded |= NorthWest;
        }
        else if (northCover & (NorthEast | NorthWestCorner))
        {
            expanded |= West;
        }
    }

    if (expanded != None)
    {
        northCover |= expanded;
        northCover = TileCovers::unifyTileCover(northCover, currQuadrant);
        neighbors.addExpandedCover(0, -1, originalCover);
    }
}

void BorderBrush::expandEast(NeighborMap &neighbors)
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;
    using TileQuadrant::TopLeft, TileQuadrant::TopRight, TileQuadrant::BottomRight, TileQuadrant::BottomLeft;

    TileQuadrant prevQuadrant = *previousQuadrant;
    currQuadrant = isTop(prevQuadrant) ? TopLeft : BottomLeft;

    currDir = Dir::East;

    TileCover &westCover = neighbors.at(-1, 0);
    TileCover originalCover = westCover;

    TileCover expanded = None;
    if (prevQuadrant == TopLeft)
    {
        if (prevDir == Dir::South && !(westCover & FullSouth))
        {
            expanded |= SouthWest;
            currQuadrant = BottomLeft;
        }
        else if (westCover & West)
        {
            expanded |= NorthWest;
        }
        else if (westCover & (SouthWest | NorthWestCorner))
        {
            expanded |= North;
        }
    }
    else if (prevQuadrant == BottomLeft)
    {
        if (prevDir == Dir::North && !(westCover & FullNorth))
        {
            expanded |= NorthWest;
            currQuadrant = TopLeft;
        }
        else if (westCover & West)
        {
            expanded |= SouthWest;
        }
        else if (westCover & (NorthWest | SouthWestCorner))
        {
            expanded |= South;
        }
    }

    if (expanded != None)
    {
        westCover |= expanded;
        westCover = TileCovers::unifyTileCover(westCover, currQuadrant);
        neighbors.addExpandedCover(-1, 0, originalCover);
    }
}

void BorderBrush::expandWest(NeighborMap &neighbors)
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;
    using TileQuadrant::TopLeft, TileQuadrant::TopRight, TileQuadrant::BottomRight, TileQuadrant::BottomLeft;

    TileQuadrant prevQuadrant = *previousQuadrant;
    currQuadrant = isTop(prevQuadrant) ? TopRight : BottomRight;

    currDir = Dir::West;

    TileCover &cover = neighbors.at(1, 0);
    TileCover originalCover = cover;

    TileCover expanded = None;
    if (prevQuadrant == TopRight)
    {
        if (prevDir == Dir::South && !(cover & FullNorth))
        {
            expanded |= SouthEast;
            currQuadrant = BottomRight;
        }
        else if (cover & East)
        {
            expanded |= NorthEast;
        }
        else if (cover & (SouthEast | NorthEastCorner))
        {
            expanded |= North;
        }
    }
    else if (prevQuadrant == BottomRight)
    {
        if (prevDir == Dir::North && !(cover & FullSouth))
        {
            expanded |= NorthEast;
            currQuadrant = TopRight;
        }
        else if (cover & East)
        {
            expanded |= SouthEast;
        }
        else if (cover & (NorthEast | SouthEastCorner))
        {
            expanded |= South;
        }
    }

    if (expanded != None)
    {
        cover |= expanded;
        cover = TileCovers::unifyTileCover(cover, currQuadrant);
        neighbors.addExpandedCover(1, 0, originalCover);
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

void BorderBrush::fixBorderEdgeCases(int x, int y, TileCover &cover, const NeighborMap &neighbors)
{
    // Two of the same diagonal in a row
    if (cover & TILE_COVER_SOUTH_WEST)
    {
        if (neighbors.at(x, y - 1) & TILE_COVER_SOUTH_WEST)
        {
            TileCovers::clearCoverFlags(cover, TILE_COVER_SOUTH_WEST | TILE_COVER_NORTH);
            cover |= TILE_COVER_NORTH_WEST;
            cover |= TILE_COVER_SOUTH;
        }
    }
}

void BorderBrush::fixBordersAtOffset(MapView &mapView, const Position &position, NeighborMap &neighbors, int x, int y)
{
    using namespace TileCoverShortHands;

    bool debug = position.x == 6 && position.y == 15 && x == 0 && y == 0;
    auto pos = position + Position(x, y, 0);

    auto currentCover = neighbors.at(x, y);
    if (currentCover == None && !(x == 0 && y == 0))
    {
        return;
    }

    TileCover cover = None;

    cover |= TileCovers::mirrorNorth(neighbors.at(x, y + 1));
    cover |= TileCovers::mirrorEast(cover, neighbors.at(x - 1, y));
    cover |= TileCovers::mirrorSouth(neighbors.at(x, y - 1));
    cover |= TileCovers::mirrorWest(cover, neighbors.at(x + 1, y));

    cover = TileCovers::mergeTileCover(cover, currentCover);

    // Compute preferred diagonal
    TileCover preferredDiagonal = currentCover & Diagonals;
    if (x == 0 && y == 0)
    {
        switch (currQuadrant)
        {
            case TileQuadrant::TopLeft:
                preferredDiagonal = NorthWest;
                break;
            case TileQuadrant::TopRight:
                preferredDiagonal = NorthEast;
                break;
            case TileQuadrant::BottomRight:
                preferredDiagonal = SouthEast;
                break;
            case TileQuadrant::BottomLeft:
                preferredDiagonal = SouthWest;
                break;
        }
    }
    TileQuadrant quadrant = getQuadrant(x, y);
    cover = TileCovers::unifyTileCover(cover, quadrant, preferredDiagonal);
    fixBorderEdgeCases(x, y, cover, neighbors);

    if (cover == currentCover && (!neighbors.isExpanded(x, y)))
    {
        // No change necessary
        return;
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

    if (cover & Full)
    {
        borders.emplace_back(BorderType::Center);
    }
    else
    {
        if (cover & North)
        {
            borders.emplace_back(BorderType::North);
        }
        if (cover & East)
        {
            borders.emplace_back(BorderType::East);
        }
        if (cover & South)
        {
            borders.emplace_back(BorderType::South);
        }
        if (cover & West)
        {
            borders.emplace_back(BorderType::West);
        }

        // Diagonals
        if (cover & Diagonals)
        {
            if (cover & NorthWest)
            {
                borders.emplace_back(BorderType::NorthWestDiagonal);
            }
            else if (cover & NorthEast)
            {
                borders.emplace_back(BorderType::NorthEastDiagonal);
            }
            else if (cover & SouthEast)
            {
                borders.emplace_back(BorderType::SouthEastDiagonal);
            }
            else if (cover & SouthWest)
            {
                borders.emplace_back(BorderType::SouthWestDiagonal);
            }
        }

        // Corners
        if (cover & Corners)
        {
            if (cover & NorthEastCorner)
            {
                borders.emplace_back(BorderType::NorthEastCorner);
            }
            if (cover & NorthWestCorner)
            {
                borders.emplace_back(BorderType::NorthWestCorner);
            }
            if (cover & SouthEastCorner)
            {
                borders.emplace_back(BorderType::SouthEastCorner);
            }
            if (cover & SouthWestCorner)
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

TileCover NeighborMap::at(int x, int y) const
{
    return data[index(x, y)];
}

void NeighborMap::set(int x, int y, TileCover tileCover)
{
    data[index(x, y)] = tileCover;
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