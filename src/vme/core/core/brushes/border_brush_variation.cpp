#include "border_brush_variation.h"

#include "border_brush.h"
#include "brush.h"

GeneralBorderBrush GeneralBorderBrush::instance;
DetailedBorderBrush DetailedBorderBrush::instance;

using TileQuadrant::TopLeft, TileQuadrant::TopRight, TileQuadrant::BottomRight, TileQuadrant::BottomLeft;

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

bool isTop(TileQuadrant quadrant);
bool isLeft(TileQuadrant quadrant);
bool isRight(TileQuadrant quadrant);

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>GeneralBorderBrush>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

TileCover GeneralBorderBrush::quadrantChanged(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection currDir, TileQuadrant prevQuadrant, TileQuadrant currQuadrant) const
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

#define whenTarget(targetQuad, newCover)         \
    if (currQuadrant == (targetQuad))            \
    {                                            \
        nextCover = tileInfo.cover | (newCover); \
    }

    TileCover cover = neighbors.center();

    if (cover == Full)
    {
        return Full;
    }

    TileCover nextCover = TileCovers::mergeTileCover(cover, tileInfo.cover);

    if (currQuadrant == tileInfo.borderStartQuadrant)
    {
        nextCover = tileInfo.cover;
    }
    else if (currDir == Dir::None)
    {
        BorderBrush::quadrantChangeInFirstTile(cover, nextCover, prevQuadrant, currQuadrant);
    }
    else if (currDir == Dir::North)
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
            if (currQuadrant == TopRight)
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

    return nextCover;

#undef whenTarget
}

void GeneralBorderBrush::expandCenter(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, TileQuadrant tileQuadrant) const
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
}

void GeneralBorderBrush::expandNorth(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    TileBorderInfo prevTile = tileInfo;

    tileInfo.quadrant = isRight(prevTile.quadrant) ? BottomRight : BottomLeft;

    // Set border start quadrant
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
    else if (prevDir == Dir::South)
    {
        tileInfo.borderStartQuadrant = isLeft(tileInfo.quadrant) ? BottomLeft : BottomRight;
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

    TileCover mask = isLeft(tileInfo.borderStartQuadrant) ? (North | NorthEast) : (North | NorthWest);
    nextSouthCover = TileCovers::addNonMasked(nextSouthCover, prevTile.cover, mask);

    TileQuadrant borderStart = tileInfo.borderStartQuadrant;

    // Erase side if necessary
    if (nextSouthCover & FullNorth)
    {
        TileCovers::eraseSide(nextSouthCover, North);
    }

    // Update cover
    if (prevDir == Dir::None)
    {
        nextSouthCover |= isLeft(borderStart) ? West : East;
    }
    else if (prevDir == Dir::North)
    {
        bool left = borderStart == BottomLeft;
        nextSouthCover |= left ? West : East;
    }
    else if (prevDir == Dir::East && borderStart == BottomRight)
    {
        nextSouthCover |= SouthEast;
    }
    else if (prevDir == Dir::South)
    {
        if (prevTile.borderStartQuadrant == TopLeft && borderStart == BottomRight)
        {
            nextSouthCover |= SouthWest | East;
        }
        else if (prevTile.borderStartQuadrant == TopRight && borderStart == BottomLeft)
        {
            nextSouthCover |= SouthEast | West;
        }
    }
    else if (prevDir == Dir::West && borderStart == BottomLeft)
    {
        nextSouthCover |= SouthWest;
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

void GeneralBorderBrush::expandSouth(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    TileBorderInfo prevTile = tileInfo;

    tileInfo.quadrant = isRight(prevTile.quadrant) ? TopRight : TopLeft;

    // Set border start quadrant
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

    TileCover mask = isLeft(tileInfo.borderStartQuadrant) ? (South | SouthEast) : (South | SouthWest);
    nextNorthCover = TileCovers::addNonMasked(nextNorthCover, prevTile.cover, mask);

    TileQuadrant borderStart = tileInfo.borderStartQuadrant;

    // Erase side if necessary
    if (northCover & FullSouth)
    {
        TileCovers::eraseSide(nextNorthCover, South);
    }

    // Update cover
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

void GeneralBorderBrush::expandEast(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    TileBorderInfo prevTile = tileInfo;

    tileInfo.quadrant = isTop(prevTile.quadrant) ? TopLeft : BottomLeft;
    tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant;

    // Set border start quadrant
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

    TileCover mask = isTop(tileInfo.borderStartQuadrant) ? (East | SouthEast) : (East | NorthEast);
    nextWestCover = TileCovers::addNonMasked(nextWestCover, prevTile.cover, mask);

    TileQuadrant borderStart = tileInfo.borderStartQuadrant;

    // Erase side if necessary
    if (westCover & FullEast)
    {
        TileCovers::eraseSide(nextWestCover, East);
    }

    // Update cover
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
    else if (prevDir == Dir::West)
    {
        if (prevTile.borderStartQuadrant == BottomRight && borderStart == TopLeft)
        {
            nextWestCover |= SouthWest | North;
        }
        else if (prevTile.borderStartQuadrant == TopRight && borderStart == BottomLeft)
        {
            nextWestCover |= NorthWest | South;
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

void GeneralBorderBrush::expandWest(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    TileBorderInfo prevTile = tileInfo;

    tileInfo.quadrant = isTop(prevTile.quadrant) ? TopRight : BottomRight;
    tileInfo.borderStartQuadrant = prevTile.borderStartQuadrant;

    // Set border start quadrant
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
    {
        return;
    }

    TileCover &self = neighbors.center();
    TileCover originalSelf = self;

    TileCover nextEastCover = eastCover;

    self |= tileInfo.borderStartQuadrant == BottomRight ? SouthEastCorner : NorthEastCorner;

    TileCover mask = isTop(tileInfo.borderStartQuadrant) ? (West | SouthWest) : (West | NorthWest);
    nextEastCover = TileCovers::addNonMasked(eastCover, prevTile.cover, mask);

    TileQuadrant borderStart = tileInfo.borderStartQuadrant;

    // Erase side if necessary
    if (eastCover & FullWest)
    {
        TileCovers::eraseSide(nextEastCover, West);
    }

    // Update cover
    if (prevDir == Dir::None && isRight(prevTile.quadrant))
    {
        nextEastCover |= isTop(borderStart) ? North : South;
    }
    else if (prevDir == Dir::North && borderStart == TopRight)
    {
        nextEastCover |= NorthEast;
    }
    else if (prevDir == Dir::East)
    {
        if (prevTile.borderStartQuadrant == TopLeft && borderStart == BottomRight)
        {
            nextEastCover |= NorthEast | South;
        }
        else if (prevTile.borderStartQuadrant == BottomLeft && borderStart == TopRight)
        {
            nextEastCover |= SouthEast | North;
        }
    }
    else if (prevDir == Dir::South && borderStart == BottomRight)
    {
        nextEastCover |= SouthEast;
    }
    else if (prevDir == Dir::West && isRight(prevTile.quadrant))
    {
        nextEastCover |= isTop(prevTile.borderStartQuadrant) ? North : South;
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

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>DetailedBorderBrush>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

TileCover DetailedBorderBrush::quadrantChanged(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection currDir, TileQuadrant prevQuadrant, TileQuadrant currQuadrant) const
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    TileCover cover = neighbors.center();

    if (cover == Full)
    {
        return Full;
    }

    TileCover nextCover = TileCovers::mergeTileCover(cover, tileInfo.cover);

    if (currDir == Dir::None)
    {
        BorderBrush::quadrantChangeInFirstTile(cover, nextCover, prevQuadrant, currQuadrant);
    }
    else
    {
        TileQuadrant borderStart = tileInfo.borderStartQuadrant;
        TileQuadrant quadrants = prevQuadrant | currQuadrant;
        if (quadrants == (TopLeft | TopRight))
        {
            bool valid = currDir != Dir::South || (neighbors.at(1, 0) & (FullSouth | SouthWestCorner));

            if (currDir != Dir::South)
            {
                nextCover |= North;
            }
        }
        else if (quadrants == (TopRight | BottomRight))
        {
            if (currDir != Dir::West)
            {
                nextCover |= East;
            }
        }
        else if (quadrants == (BottomRight | BottomLeft))
        {
            TileCover part = South;
            if (currDir == Dir::North)
            {
                bool prevLeft = isLeft(prevQuadrant);
                TileCover corner = prevLeft ? SouthEastCorner : SouthWestCorner;
                int dx = prevLeft ? -1 : 1;

                if (!(neighbors.at(dx, 0) & (FullSouth | corner)))
                {
                    part = corner;
                }
            }

            nextCover |= part;
        }
        else if (quadrants == (BottomLeft | TopLeft))
        {
            if (currDir != Dir::East)
            {
                nextCover |= West;
            }
        }
    }

    return nextCover;
}

void DetailedBorderBrush::expandCenter(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, TileQuadrant tileQuadrant) const
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
}

void DetailedBorderBrush::expandNorth(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    TileBorderInfo prevTile = tileInfo;

    tileInfo.quadrant = isRight(prevTile.quadrant) ? BottomRight : BottomLeft;
    tileInfo.borderStartQuadrant = tileInfo.quadrant;

    TileCover &southCover = neighbors.at(0, 1);
    TileCover nextSouthCover = southCover;

    TileCover &self = neighbors.center();
    TileCover originalSelf = self;

    if (nextSouthCover == Full)
    {
        return;
    }

    if (self != Full)
    {
        self |= tileInfo.borderStartQuadrant == BottomLeft ? SouthWestCorner : SouthEastCorner;
    }

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
        if ((prevDir == Dir::East) & !(nextSouthCover & FullWest))
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

void DetailedBorderBrush::expandEast(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    TileBorderInfo prevTile = tileInfo;

    tileInfo.quadrant = isTop(prevTile.quadrant) ? TopLeft : BottomLeft;
    tileInfo.borderStartQuadrant = tileInfo.quadrant;

    TileCover &self = neighbors.center();
    TileCover originalSelf = self;

    TileCover &westCover = neighbors.at(-1, 0);
    TileCover nextWestCover = westCover;

    if (westCover == Full)
    {
        return;
    }

    if (self != Full)
    {
        self |= isTop(tileInfo.borderStartQuadrant) ? NorthWestCorner : SouthWestCorner;
    }

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

void DetailedBorderBrush::expandSouth(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    TileBorderInfo prevTile = tileInfo;

    tileInfo.quadrant = isRight(prevTile.quadrant) ? TopRight : TopLeft;
    tileInfo.borderStartQuadrant = tileInfo.quadrant;

    TileCover &self = neighbors.at(0, 0);
    TileCover originalSelf = self;

    TileCover &northCover = neighbors.at(0, -1);
    TileCover nextNorthCover = northCover;
    if (northCover == Full)
    {
        return;
    }

    if (self != Full)
    {
        self |= tileInfo.borderStartQuadrant == TopLeft ? NorthWestCorner : NorthEastCorner;
    }

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

void DetailedBorderBrush::expandWest(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    TileBorderInfo prevTile = tileInfo;

    tileInfo.quadrant = isTop(prevTile.quadrant) ? TopRight : BottomRight;
    tileInfo.borderStartQuadrant = tileInfo.quadrant;

    TileCover &eastCover = neighbors.at(1, 0);
    if (eastCover == Full)
        return;

    TileCover &self = neighbors.center();
    TileCover originalSelf = self;

    TileCover nextEastCover = eastCover;

    if (self != Full)
    {
        self |= tileInfo.borderStartQuadrant == BottomRight ? SouthEastCorner : NorthEastCorner;
    }

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
