#include "border_brush.h"

#include <algorithm>

#include "../config.h"
#include "../map_view.h"
#include "../util.h"
#include "ground_brush.h"
#include "raw_brush.h"

BorderBrush::TileInfo BorderBrush::tileInfo = {};

BorderExpandDirection BorderBrush::currDir;
std::optional<BorderExpandDirection> BorderBrush::prevDir;

using TileQuadrant::TopLeft, TileQuadrant::TopRight, TileQuadrant::BottomRight, TileQuadrant::BottomLeft;

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

inline std::ostream &operator<<(std::ostream &os, const TileQuadrant &quadrant)
{
    switch (quadrant)
    {
        case TopLeft:
            os << "TopLeft";
            break;
        case TopRight:
            os << "TopRight";
            break;
        case BottomRight:
            os << "BottomRight";
            break;
        case BottomLeft:
            os << "BottomLeft";
            break;
    }

    return os;
}

TileQuadrant mirrorQuadrant(TileQuadrant quadrant, bool horizontal = true)
{
    switch (quadrant)
    {
        case TileQuadrant::TopLeft:
            return horizontal ? TopRight : BottomLeft;
        case TileQuadrant::TopRight:
            return horizontal ? TopLeft : BottomRight;
        case TileQuadrant::BottomRight:
            return horizontal ? BottomLeft : TopRight;
        case TileQuadrant::BottomLeft:
            return horizontal ? BottomRight : TopLeft;
        default:
            ABORT_PROGRAM("Bad quadrant.");
    }
}

std::pair<int, int> quadrantDiff(TileQuadrant lhs, TileQuadrant rhs)
{

    int x0 = to_underlying(lhs & (TopLeft | BottomLeft)) ? 0 : 1;
    int x1 = to_underlying(rhs & (TopLeft | BottomLeft)) ? 0 : 1;

    int y0 = to_underlying(lhs & (TopLeft | TopRight)) ? 0 : 1;
    int y1 = to_underlying(rhs & (TopLeft | TopRight)) ? 0 : 1;

    return {x0 - x1, y0 - y1};
}

bool clockwise(TileQuadrant a, TileQuadrant b)
{

    return (a == TopLeft && b == TopRight) || (a == TopRight && b == BottomRight) || (a == BottomRight && b == BottomLeft) || (a == BottomLeft && b == TopLeft);
}

TileQuadrant oppositeQuadrant(TileQuadrant quadrant)
{

    if (quadrant == TopLeft)
        return BottomRight;
    else if (quadrant == TopRight)
        return BottomLeft;
    else if (quadrant == BottomLeft)
        return TopRight;
    else
        return TopLeft;
}

bool isOpposite(TileQuadrant a, TileQuadrant b)
{

    TileQuadrant both = (a | b);
    return (both == (BottomLeft | TopRight)) || (both == (TopLeft | BottomRight));
}

// bool movingTowards(TileQuadrant from, TileQuadrant to, TileQuadrant target)
// {

//     bool isTarget = to == target;
//     switch (target)
//     {
//         case TopLeft:
//             return isTarget || from == BottomRight;
//         case TopRight:
//             return isTarget || from == BottomLeft;
//         case BottomRight:
//             return isTarget || from == TopLeft;
//         case BottomLeft:
//             return isTarget || from == TopRight;
//     }
// }

BorderExpandDirection getTravelledSide(TileQuadrant from, TileQuadrant to)
{

    TileQuadrant quadrants = from | to;
    if (quadrants == (TopLeft | TopRight))
    {
        return BorderExpandDirection::North;
    }
    else if (quadrants == (TopRight | BottomRight))
    {
        return BorderExpandDirection::East;
    }
    else if (quadrants == (BottomRight | BottomLeft))
    {
        return BorderExpandDirection::South;
    }
    else if (quadrants == (BottomLeft | TopLeft))
    {
        return BorderExpandDirection::West;
    }
    else
    {
        return BorderExpandDirection::None;
    }
}

bool isTop(TileQuadrant quadrant);
bool isLeft(TileQuadrant quadrant);
bool isRight(TileQuadrant quadrant);

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

TileCover NeighborMap::getTileCoverAt(BorderBrush *brush, const Map &map, const Position position) const
{
    Tile *tile = map.getTile(position);
    if (!tile)
    {
        return TILE_COVER_NONE;
    }

    return tile->getTileCover(brush);
}

TileQuadrant BorderBrush::getNeighborQuadrant(int dx, int dy)
{
    if (dx == 0 && dy == 0)
        return tileInfo.quadrant;

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
    NeighborMap neighbors(position, this, map);

    std::string dirName = "None";

    std::optional<Position> lastPos = mapView.getLastBrushDragPosition();
    auto mouseQuadrant = mapView.getMouseDownTileQuadrant();

    if (!lastPos && mouseQuadrant)
    {
        expandCenter(neighbors, *mouseQuadrant);
    }
    else
    {
        prevDir = currDir;
        Position expandDirection = position - *lastPos;
        DEBUG_ASSERT(expandDirection != PositionConstants::Zero, "Should not be able to be zero here");

        if (expandDirection.x == -1)
        {
            dirName = "West";
            expandWest(neighbors);
        }
        else if (expandDirection.x == 1)
        {
            dirName = "East";
            expandEast(neighbors);
        }
        else if (expandDirection.y == -1)
        {
            dirName = "North";
            expandNorth(neighbors);
        }
        else if (expandDirection.y == 1)
        {
            dirName = "South";
            expandSouth(neighbors);
        }
    }

    VME_LOG_D("Apply " << dirName << " :: " << position << "\tborderStart: " << tileInfo.borderStartQuadrant);

    updateCenter(neighbors);

    fixBorders(mapView, position, neighbors);

    tileInfo.cover = neighbors.center();
    tileInfo.prevQuadrant = tileInfo.quadrant;

    VME_LOG_D("Set to " << tileInfo.quadrant << " in apply");

    if (mouseQuadrant && tileInfo.borderStartQuadrant != *mouseQuadrant)
    {
        quadrantChanged(mapView, position, tileInfo.borderStartQuadrant, *mouseQuadrant);
    }
}

void BorderBrush::quadrantChanged(MapView &mapView, const Position &position, TileQuadrant oldQuadrant, TileQuadrant newQuadrant)
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    bool diagonal = (oldQuadrant | newQuadrant) == (BottomLeft | TopRight) || (oldQuadrant | newQuadrant) == (TopLeft | BottomRight);
    // std::string diagText = diagonal ? " (diagonal)" : "";
    // VME_LOG_D("quadrantChanged " << position << " :: " << oldQuadrant << " -> " << newQuadrant << diagText);

    const Map &map = *mapView.map();
    NeighborMap neighbors(position, this, map);

    TileCover &cover = neighbors.center();
    TileCover originalCover = cover;

    if (cover == Full)
    {
        return;
    }

    TileCover expanded = TileCovers::mergeTileCover(cover, tileInfo.cover);
#define whenTarget(targetQuad, newCover)        \
    if (newQuadrant == targetQuad)              \
    {                                           \
        expanded = tileInfo.cover | (newCover); \
    }

    if (newQuadrant == tileInfo.borderStartQuadrant)
    {
        expanded = tileInfo.cover;
    }
    else if (currDir == Dir::None)
    {

        TileCover horizontalCover = isTop(tileInfo.borderStartQuadrant) ? North : South;
        TileCover verticalCover = isLeft(tileInfo.borderStartQuadrant) ? West : East;
        TileQuadrant opposite = oppositeQuadrant(tileInfo.borderStartQuadrant);
        TileQuadrant siblingX = mirrorQuadrant(tileInfo.borderStartQuadrant);

        if (newQuadrant == opposite)
        {
            if (oldQuadrant != tileInfo.borderStartQuadrant)
            {
                if (oldQuadrant == siblingX)
                {
                    auto side = verticalCover == West ? East : West;
                    if (expanded & side)
                    {
                        expanded &= ~side;
                    }
                    else
                    {
                        expanded |= side;
                    }
                }
                else
                {
                    auto side = horizontalCover == North ? South : North;
                    if (expanded & side)
                    {
                        expanded &= ~side;
                    }
                    else
                    {
                        expanded |= side;
                    }
                }
            }
        }
        else if (oldQuadrant == opposite)
        {
            if (newQuadrant == siblingX)
            {
                auto side = verticalCover == West ? East : West;
                if (TileCovers::hasFullSide(cover, horizontalCover))
                {
                    expanded &= ~TileCovers::getFull(side);
                    expanded |= horizontalCover;
                }
                else
                {
                    expanded |= side;
                }
            }
            else
            {
                auto side = horizontalCover == North ? South : North;
                if (TileCovers::hasFullSide(cover, verticalCover))
                {
                    expanded &= ~TileCovers::getFull(side);
                    expanded |= verticalCover;
                }
                else
                {
                    expanded |= side;
                }
            }
        }
        else if (oldQuadrant == tileInfo.borderStartQuadrant)
        {
            expanded |= newQuadrant == siblingX ? horizontalCover : verticalCover;
        }

        expanded |= tileInfo.cover;
    }
    else
    {
        if (currDir == Dir::North)
        {
            if (tileInfo.borderStartQuadrant == BottomLeft)
            {
                whenTarget(TopLeft, West)                                                  // ◰
                    else whenTarget(TopRight, NorthWest)                                   // ◳
                    else whenTarget(BottomRight, tileInfo.cover & West ? West : NorthWest) // ◲
            }
            else if (tileInfo.borderStartQuadrant == BottomRight)
            {
                whenTarget(TopLeft, NorthEast)                                            // ◰
                    else whenTarget(TopRight, East)                                       // ◳
                    else whenTarget(BottomLeft, tileInfo.cover & East ? East : NorthEast) // ◱
            }
        }
        else if (currDir == Dir::East)
        {
            if (tileInfo.borderStartQuadrant == BottomLeft)
            {
                whenTarget(TopLeft, tileInfo.cover & South ? South : SouthEast) // ◰
                    else whenTarget(TopRight, SouthEast)                        // ◳
                    else whenTarget(BottomRight, South)                         // ◲
            }
            else if (tileInfo.borderStartQuadrant == TopLeft)
            {
                whenTarget(TopRight, North)                                                 // ◳
                    else whenTarget(BottomRight, NorthEast)                                 // ◲
                    else whenTarget(BottomLeft, tileInfo.cover & North ? North : NorthEast) // ◱
            }
        }
        else if (currDir == Dir::West)
        {
            if (tileInfo.borderStartQuadrant == TopRight)
            {
                whenTarget(TopLeft, North)                                                   // ◰
                    else whenTarget(BottomLeft, NorthWest)                                   // ◱
                    else whenTarget(BottomRight, tileInfo.cover & North ? North : NorthWest) // ◲
            }
            else if (tileInfo.borderStartQuadrant == BottomRight)
            {
                whenTarget(TopLeft, SouthWest)                                            // ◰
                    else whenTarget(TopRight, tileInfo.cover & South ? South : SouthWest) // ◳
                    else whenTarget(BottomLeft, South)                                    // ◱
            }
        }
        else if (currDir == Dir::South)
        {
            if (tileInfo.borderStartQuadrant == TopLeft)
            {
                if (newQuadrant == TopRight)
                {
                    auto x = true;
                }
                whenTarget(TopRight, tileInfo.cover & West ? West : SouthWest) // ◳
                    else whenTarget(BottomRight, SouthWest)                    // ◲
                    else whenTarget(BottomLeft, West)                          // ◱
            }
            else if (tileInfo.borderStartQuadrant == TopRight)
            {
                whenTarget(TopLeft, tileInfo.cover & East ? East : SouthEast) // ◰
                    else whenTarget(BottomRight, East)                        // ◲
                    else whenTarget(BottomLeft, SouthEast)                    // ◱
            }
        }
    }

    if (expanded != cover)
    {
        cover = TileCovers::unifyTileCover(expanded, newQuadrant);
        neighbors.addExpandedCover(0, 0);

        fixBordersAtOffset(mapView, position, neighbors, 0, 0);
    }

    // VME_LOG_D("Set to " << oldQuadrant << " in quadrantChanged");
    tileInfo.prevQuadrant = oldQuadrant;
    tileInfo.quadrant = newQuadrant;

#undef whenTarget
}

void BorderBrush::updateCenter(NeighborMap &neighbors)
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    TileCover &self = neighbors.at(0, 0);
    TileCover originalSelf = self;

    TileQuadrant quadrant = tileInfo.quadrant;

    // Update diagonal
    if (self & TileCovers::Diagonals)
    {
        if (quadrant == TileQuadrant::TopLeft)
        {
            if ((self & SouthWest) && (self & North))
            {
                self &= ~SouthWest;
                self |= NorthWest | South;
            }
            else if ((self & NorthEast) && (self & West))
            {
                self &= ~(NorthEast | West);
                self |= NorthWest | East;
            }
        }
        else if (quadrant == TileQuadrant::TopRight)
        {
            if ((self & NorthWest) && (self & East))
            {
                self &= ~NorthWest;
                self |= NorthEast | West;
            }
            else if ((self & SouthEast) && (self & North))
            {
                self &= ~SouthEast;
                self |= NorthEast | South;
            }
        }
        else if (quadrant == TileQuadrant::BottomRight)
        {
            if ((self & NorthEast) && (self & South))
            {
                self &= ~NorthEast;
                self |= SouthEast | North;
            }
            else if ((self & SouthWest) && (self & East))
            {
                self &= ~SouthWest;
                self |= SouthEast | West;
            }
        }
        else if (quadrant == TileQuadrant::BottomLeft)
        {
            if ((self & NorthWest) && (self & South))
            {
                self &= ~NorthWest;
                self |= SouthWest | North;
            }
            else if ((self & SouthEast) && (self & West))
            {
                self &= ~SouthEast;
                self |= SouthWest | East;
            }
        }
    }

    if (!neighbors.isExpanded(0, 0) && self != originalSelf)
    {
        neighbors.addExpandedCover(0, 0);
    }
}

void BorderBrush::expandCenter(NeighborMap &neighbors, TileQuadrant tileQuadrant)
{
    using namespace TileCoverShortHands;

    TileCover &centerCover = neighbors.at(0, 0);
    TileCover nextCenterCover = centerCover;

    if (centerCover == Full)
        return;

    switch (tileQuadrant)
    {
        case TileQuadrant::TopLeft:
            nextCenterCover |= NorthWestCorner;
            break;
        case TileQuadrant::TopRight:
            nextCenterCover |= NorthEastCorner;
            break;
        case TileQuadrant::BottomRight:
            nextCenterCover |= SouthEastCorner;
            break;
        case TileQuadrant::BottomLeft:
            nextCenterCover |= SouthWestCorner;
            break;
    }

    if (centerCover != nextCenterCover)
    {
        centerCover = nextCenterCover;
        neighbors.addExpandedCover(0, 0);
    }

    prevDir.reset();

    currDir = BorderExpandDirection::None;

    tileInfo.quadrant = tileQuadrant;
    tileInfo.borderStartQuadrant = tileQuadrant;
}

void BorderBrush::expandNorth(NeighborMap &neighbors)
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    currDir = Dir::North;

    TileInfo prevTile = tileInfo;

    tileInfo.quadrant = isRight(prevTile.quadrant) ? BottomRight : BottomLeft;
    tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant;

    if (prevDir == Dir::None)
    {
        tileInfo.borderStartQuadrant = isRight(prevTile.prevQuadrant) ? BottomRight : BottomLeft;
    }
    else if (prevDir == Dir::North)
    {
        tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant;
    }
    else if (prevDir == Dir::East)
    {
        tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant == TopLeft ? BottomLeft : BottomRight;
    }
    else if (prevDir == Dir::West)
    {
        tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant == BottomRight ? BottomLeft : BottomRight;
    }

    TileCover &southCover = neighbors.at(0, 1);
    TileCover nextSouthCover = southCover;

    TileCover &self = neighbors.center();
    TileCover originalSelf = self;
    if (nextSouthCover == Full)
        return;

    self |= tileInfo.borderStartQuadrant == BottomLeft ? SouthWestCorner : SouthEastCorner;

    if (Settings::LOCK_BORDER_BRUSH_SIDE)
    {
        TileCover mask = isLeft(tileInfo.borderStartQuadrant) ? (North | NorthEast) : (North | NorthWest);
        nextSouthCover = TileCovers::addNonMasked(nextSouthCover, prevTile.cover, mask);

        if (nextSouthCover & FullNorth)
        {
            TileCovers::eraseSide(nextSouthCover, North);
        }

        if (prevDir == Dir::None)
        {
            nextSouthCover |= isLeft(tileInfo.borderStartQuadrant) ? West : East;
        }
        else if (prevDir == Dir::North)
        {
            bool left = tileInfo.borderStartQuadrant == BottomLeft;
            nextSouthCover |= left ? West : East;
        }
        else if (prevDir == Dir::East && tileInfo.borderStartQuadrant == BottomRight)
        {
            nextSouthCover |= SouthEast;
        }
        else if (prevDir == Dir::West && tileInfo.borderStartQuadrant == BottomLeft)
        {
            nextSouthCover |= SouthWest;
        }
    }
    else
    {
        if (prevTile.quadrant == BottomRight)
        {
            if (prevDir == Dir::West && !(nextSouthCover & FullEast))
            {
                nextSouthCover |= SouthWest;
                tileInfo.quadrant = BottomLeft;
            }
            else if (nextSouthCover & South)
            {
                nextSouthCover |= SouthEast;
            }
            else if (nextSouthCover & (SouthWest | SouthEastCorner))
            {
                nextSouthCover |= East;
            }
        }
        else if (prevTile.quadrant == BottomLeft)
        {
            if (prevDir == Dir::East & !(nextSouthCover & FullWest))
            {
                nextSouthCover |= SouthEast;
                tileInfo.quadrant = BottomRight;
            }
            else if (nextSouthCover & South)
            {
                nextSouthCover |= SouthWest;
            }
            else if (nextSouthCover & (SouthEast | SouthWestCorner))
            {
                nextSouthCover |= West;
            }
        }
        else if (prevTile.quadrant == TopLeft)
        {
            if (prevDir == Dir::South)
            {
                nextSouthCover |= West;
                nextSouthCover |= SouthEast;
                tileInfo.quadrant = BottomRight;
            }
        }
    }

    if (self != originalSelf)
    {
        self = TileCovers::unifyTileCover(self, tileInfo.quadrant);
        neighbors.addExpandedCover(0, 0);
    }

    if (southCover != nextSouthCover)
    {
        southCover = TileCovers::unifyTileCover(nextSouthCover, tileInfo.quadrant);
        neighbors.addExpandedCover(0, 1);
    }
}

void BorderBrush::expandSouth(NeighborMap &neighbors)
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    currDir = Dir::South;

    TileInfo prevTile = tileInfo;

    tileInfo.quadrant = isRight(prevTile.quadrant) ? TopRight : TopLeft;

    if (prevDir == Dir::None)
    {
        bool forceWest = prevTile.prevQuadrant == TopLeft && prevTile.quadrant == BottomRight && isLeft(prevTile.borderStartQuadrant);
        tileInfo.borderStartQuadrant = (isRight(prevTile.prevQuadrant) && !forceWest) ? TopRight : TopLeft;
    }
    else if (prevDir == Dir::North)
    {
        tileInfo.borderStartQuadrant = isLeft(tileInfo.quadrant) ? TopLeft : TopRight;
    }
    else if (prevDir == Dir::East)
    {
        tileInfo.borderStartQuadrant = isTop(prevTile.borderStartQuadrant) ? TopRight : TopLeft;
    }
    else if (prevDir == Dir::South)
    {
        tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant;
    }
    else if (prevDir == Dir::West)
    {
        tileInfo.borderStartQuadrant = isTop(prevTile.borderStartQuadrant) ? TopLeft : TopRight;
    }
    TileCover &self = neighbors.at(0, 0);
    TileCover originalSelf = self;

    TileCover &northCover = neighbors.at(0, -1);
    TileCover nextNorthCover = northCover;
    if (northCover == Full)
    {
        return;
    }

    self |= tileInfo.borderStartQuadrant == TopLeft ? NorthWestCorner : NorthEastCorner;

    if (Settings::LOCK_BORDER_BRUSH_SIDE)
    {
        TileCover mask = isLeft(tileInfo.borderStartQuadrant) ? (South | SouthEast) : (South | SouthWest);
        nextNorthCover = TileCovers::addNonMasked(nextNorthCover, prevTile.cover, mask);

        TileQuadrant borderStart = tileInfo.borderStartQuadrant;

        if (northCover & FullSouth)
        {
            TileCovers::eraseSide(nextNorthCover, South);
        }

        if (prevDir == Dir::None && isTop(prevTile.quadrant))
        {
            nextNorthCover |= isLeft(borderStart) ? West : East;
        }
        else if (prevDir == Dir::North)
        {
            if (prevTile.borderStartQuadrant == BottomLeft && borderStart == TopRight)
            {
                nextNorthCover |= NorthWest | East;
            }
            else if (prevTile.borderStartQuadrant == BottomRight && borderStart == TopLeft)
            {
                nextNorthCover |= NorthEast | West;
            }
        }
        else if (prevDir == Dir::East && borderStart == TopRight)
        {
            nextNorthCover |= NorthEast;
        }
        else if (prevDir == Dir::South && isTop(prevTile.quadrant))
        {
            nextNorthCover |= isLeft(borderStart) ? West : East;
        }
        else if (prevDir == Dir::West && borderStart == TopLeft)
        {
            nextNorthCover |= NorthWest;
        }
    }
    else
    {
        if (prevTile.quadrant == TopRight)
        {
            if (prevDir == Dir::West && !(northCover & FullEast))
            {
                nextNorthCover |= NorthWest;
                tileInfo.quadrant = TopLeft;
            }
            else if (northCover & North)
            {
                nextNorthCover |= NorthEast;
            }
            else if (northCover & (NorthWest | NorthEastCorner))
            {
                nextNorthCover |= East;
            }
        }
        else if (prevTile.quadrant == TopLeft)
        {
            if (prevDir == Dir::East && !(northCover & FullWest))
            {
                nextNorthCover |= NorthEast;
                tileInfo.quadrant = TopRight;
            }
            else if (northCover & North)
            {
                nextNorthCover |= NorthWest;
            }
            else if (northCover & (NorthEast | NorthWestCorner))
            {
                nextNorthCover |= West;
            }
        }
    }

    if (self != originalSelf)
    {
        self = TileCovers::unifyTileCover(self, tileInfo.quadrant);
        neighbors.addExpandedCover(0, 0);
    }

    if (northCover != nextNorthCover)
    {
        northCover = TileCovers::unifyTileCover(nextNorthCover, tileInfo.quadrant);
        neighbors.addExpandedCover(0, -1);
    }
}

void BorderBrush::expandEast(NeighborMap &neighbors)
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    currDir = Dir::East;

    TileInfo prevTile = tileInfo;

    tileInfo.quadrant = isTop(prevTile.quadrant) ? TopLeft : BottomLeft;
    tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant;

    // Get border start
    if (prevDir == Dir::None)
    {
        bool forceSouth = prevTile.prevQuadrant == BottomLeft && prevTile.quadrant == TopRight && !isTop(prevTile.borderStartQuadrant);
        tileInfo.borderStartQuadrant = (isTop(prevTile.prevQuadrant) && !forceSouth) ? TopLeft : BottomLeft;
    }
    else if (prevDir == Dir::North)
    {
        tileInfo.borderStartQuadrant = isLeft(prevTile.borderStartQuadrant) ? TopLeft : BottomLeft;
    }
    else if (prevDir == Dir::East)
    {
        tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant;
    }
    else if (prevDir == Dir::South)
    {
        tileInfo.borderStartQuadrant = isLeft(prevTile.borderStartQuadrant) ? BottomLeft : TopLeft;
    }
    else if (prevDir == Dir::West)
    {
        bool prevStartTop = isTop(prevTile.borderStartQuadrant);
        bool currTop = isTop(tileInfo.quadrant);

        // Loop around the previous tile if prevTop & currTop differ, either from top-right (←, ↓, →) or from bottom-left (←, ↑, →)
        tileInfo.borderStartQuadrant = currTop ^ (prevStartTop != currTop) ? TopLeft : BottomLeft;
    }

    TileCover &self = neighbors.center();
    TileCover originalSelf = self;

    TileCover &westCover = neighbors.at(-1, 0);
    TileCover nextWestCover = westCover;

    if (westCover == Full)
    {
        return;
    }

    self |= isTop(tileInfo.borderStartQuadrant) ? NorthWestCorner : SouthWestCorner;

    if (Settings::LOCK_BORDER_BRUSH_SIDE)
    {
        TileCover mask = isTop(tileInfo.borderStartQuadrant) ? (East | SouthEast) : (East | NorthEast);
        nextWestCover = TileCovers::addNonMasked(nextWestCover, prevTile.cover, mask);

        TileQuadrant borderStart = tileInfo.borderStartQuadrant;
        if (westCover & FullEast)
        {
            TileCovers::eraseSide(nextWestCover, East);
        }

        if (prevDir == Dir::None && isLeft(prevTile.quadrant))
        {
            nextWestCover |= isTop(borderStart) ? North : South;
        }
        else if (prevDir == Dir::North && borderStart == TopLeft)
        {
            nextWestCover |= NorthWest;
        }
        else if (prevDir == Dir::East)
        {
            bool left = borderStart == BottomLeft;
            nextWestCover |= left ? South : North;
        }
        else if (prevDir == Dir::South && borderStart == BottomLeft)
        {
            nextWestCover |= SouthWest;
        }
    }
    else
    {
        if (prevTile.quadrant == TopLeft)
        {
            if (prevDir == Dir::South && !(nextWestCover & FullSouth))
            {
                nextWestCover |= SouthWest;
            }
            else if (nextWestCover & West)
            {
                nextWestCover |= NorthWest;
            }
            else if (nextWestCover & (SouthWest | NorthWestCorner))
            {
                nextWestCover |= North;
            }
        }
        else if (prevTile.quadrant == BottomLeft)
        {
            if (prevDir == Dir::North && !(nextWestCover & FullNorth))
            {
                nextWestCover |= NorthWest;
                tileInfo.quadrant = TopLeft;
            }
            else if (nextWestCover & West)
            {
                nextWestCover |= SouthWest;
            }
            else if (nextWestCover & (NorthWest | SouthWestCorner))
            {
                nextWestCover |= South;
            }
        }
    }

    if (self != originalSelf)
    {
        self = TileCovers::unifyTileCover(self, tileInfo.quadrant);
        neighbors.addExpandedCover(0, 0);
    }
    if (westCover != nextWestCover)
    {
        westCover = TileCovers::unifyTileCover(nextWestCover, tileInfo.quadrant);
        neighbors.addExpandedCover(-1, 0);
    }
}

void BorderBrush::expandWest(NeighborMap &neighbors)
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    currDir = Dir::West;

    TileInfo prevTile = tileInfo;

    tileInfo.quadrant = isTop(prevTile.quadrant) ? TopRight : BottomRight;
    tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant;

    if (prevDir == Dir::None)
    {
        tileInfo.borderStartQuadrant = isTop(prevTile.prevQuadrant) ? TopRight : BottomRight;
    }
    else if (prevDir == Dir::North)
    {
        tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant == BottomLeft ? BottomRight : TopRight;
    }
    else if (prevDir == Dir::East)
    {
        bool prevStartTop = isTop(prevTile.borderStartQuadrant);
        bool currTop = isTop(tileInfo.quadrant);

        // Loop around the previous tile if prevTop & currTop differ, either from top-left (→, ↓, ←)  or from bottom-left (→, ↑, ←)
        tileInfo.borderStartQuadrant = currTop ^ (prevStartTop != currTop) ? BottomRight : TopRight;
    }
    else if (prevDir == Dir::South)
    {
        tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant == TopLeft ? TopRight : BottomRight;
    }
    else if (prevDir == Dir::West)
    {
        tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant;
    }

    TileCover &eastCover = neighbors.at(1, 0);
    if (eastCover == Full)
        return;

    TileCover &self = neighbors.center();
    TileCover originalSelf = self;

    TileCover nextEastCover = eastCover;

    self |= tileInfo.borderStartQuadrant == BottomRight ? SouthEastCorner : NorthEastCorner;

    if (Settings::LOCK_BORDER_BRUSH_SIDE)
    {
        TileCover mask = isTop(tileInfo.borderStartQuadrant) ? (West | SouthWest) : (West | NorthWest);
        nextEastCover = TileCovers::addNonMasked(eastCover, prevTile.cover, mask);

        TileQuadrant borderStart = tileInfo.borderStartQuadrant;

        if (eastCover & FullWest)
        {
            TileCovers::eraseSide(nextEastCover, West);
        }

        if (prevDir == Dir::None && isRight(prevTile.quadrant))
        {
            nextEastCover |= isTop(borderStart) ? North : South;
        }
        else if (prevDir == Dir::North && borderStart == TopRight)
        {
            nextEastCover |= NorthEast;
        }
        else if (prevDir == Dir::East && isRight(prevTile.quadrant))
        {
            nextEastCover |= isTop(prevTile.borderStartQuadrant) ? North : South;
        }
        else if (prevDir == Dir::South && borderStart == BottomRight)
        {
            nextEastCover |= SouthEast;
        }
        else if (prevDir == Dir::West && isRight(prevTile.quadrant))
        {
            nextEastCover |= isTop(prevTile.borderStartQuadrant) ? North : South;
        }
    }
    else
    {

        if (prevTile.quadrant == TopRight)
        {
            if (prevDir == Dir::South && !(eastCover & FullNorth))
            {
                nextEastCover |= SouthEast;
                tileInfo.quadrant = BottomRight;
            }
            else if (eastCover & East)
            {
                nextEastCover |= NorthEast;
            }
            else if (eastCover & (SouthEast | NorthEastCorner))
            {
                nextEastCover |= North;
            }
        }
        else if (prevTile.quadrant == BottomRight)
        {
            if (prevDir == Dir::North && !(eastCover & FullSouth))
            {
                nextEastCover |= NorthEast;
                tileInfo.quadrant = TopRight;
            }
            else if (eastCover & East)
            {
                nextEastCover |= SouthEast;
            }
            else if (eastCover & (NorthEast | SouthEastCorner))
            {
                nextEastCover |= South;
            }
        }
    }

    if (self != originalSelf)
    {
        self = TileCovers::unifyTileCover(self, tileInfo.quadrant);
        neighbors.addExpandedCover(0, 0);
    }

    if (eastCover != nextEastCover)
    {
        eastCover = TileCovers::unifyTileCover(nextEastCover, tileInfo.quadrant);
        neighbors.addExpandedCover(1, 0);
    }
}

void BorderBrush::fixBorders(MapView &mapView, const Position &position, NeighborMap &neighbors)
{
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            if (neighbors.isExpanded(dx, dy))
            {
                fixBordersAtOffset(mapView, position, neighbors, dx, dy);
            }
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

    // bool debug = position.x == 6 && position.y == 15 && x == 0 && y == 0;
    auto pos = position + Position(x, y, 0);

    auto currentCover = neighbors.at(x, y);

    TileCover cover = None;
    cover |= TileCovers::mirrorNorth(neighbors.at(x, y + 1));
    cover |= TileCovers::mirrorEast(cover, neighbors.at(x - 1, y));
    cover |= TileCovers::mirrorSouth(neighbors.at(x, y - 1));
    cover |= TileCovers::mirrorWest(cover, neighbors.at(x + 1, y));

    // Do not use a mirrored diagonal if we already have a diagonal.
    if (currentCover & Diagonals)
    {
        cover &= ~(Diagonals);
    }

    cover = TileCovers::mergeTileCover(cover, currentCover);

    // Compute preferred diagonal
    TileCover preferredDiagonal = currentCover & Diagonals;
    if (x == 0 && y == 0)
    {
        switch (tileInfo.quadrant)
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
    TileQuadrant quadrant = getNeighborQuadrant(x, y);
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

NeighborMap::NeighborMap(const Position &position, BorderBrush *brush, const Map &map)
{
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            TileCover tileCover = getTileCoverAt(brush, map, position + Position(dx, dy, 0));
            TileQuadrant quadrant = brush->getNeighborQuadrant(dx, dy);
            tileCover = TileCovers::unifyTileCover(tileCover, quadrant);
            auto diagonals = tileCover & TileCovers::Diagonals;
            DEBUG_ASSERT((diagonals == TileCovers::None) || TileCovers::exactlyOneSet(diagonals & TileCovers::Diagonals), "Preferred diagonal must be None or have exactly one diagonal set.");

            set(dx, dy, tileCover);
        }
    }
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

void NeighborMap::addExpandedCover(int x, int y)
{
    expandedCovers.emplace_back(ExpandedTileBlock{x, y});
}

TileCover &NeighborMap::at(int x, int y)
{
    return data[index(x, y)];
}

TileCover &NeighborMap::center()
{
    return data[12];
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
bool isLeft(TileQuadrant quadrant)
{
    return quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::BottomLeft;
}
bool isTop(TileQuadrant quadrant)
{
    return quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::TopRight;
}