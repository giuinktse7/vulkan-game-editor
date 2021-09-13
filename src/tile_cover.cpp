#include "tile_cover.h"

#include <sstream>

void TileCovers::clearCoverFlags(TileCover &cover, TileCover flags)
{
    cover &= ~flags;
}

bool TileCovers::contains(TileCover source, TileCover test) noexcept
{
    return ((source & test) == test);
}

void TileCovers::eraseSide(TileCover &cover, TileCover side, TileCover preferredDiagonal)
{
    DEBUG_ASSERT((preferredDiagonal == None) || exactlyOneSet(preferredDiagonal & Diagonals), "Preferred diagonal must be None or have exactly one diagonal set.");
    DEBUG_ASSERT(side == North || side == East || side == South || side == West, "Invalid side.");
    // Handle cases where cover is full tile
    if (cover & Full)
    {
        cover = None;
        if (side == North)
        {
            cover |= (preferredDiagonal & (NorthWest | SouthWest)) ? SouthWest | East : SouthEast | West;
        }
        else if (side == East)
        {
            cover |= (preferredDiagonal & (NorthEast | NorthWest)) ? NorthWest | South : SouthWest | North;
        }
        else if (side == South)
        {
            cover |= (preferredDiagonal & (NorthWest | SouthWest)) ? NorthWest | East : NorthEast | West;
        }
        else if (side == West)
        {
            cover |= (preferredDiagonal & (NorthEast | NorthWest)) ? NorthEast | South : SouthEast | North;
        }
    }
    else
    {
        if (side == North)
        {
            if (cover & NorthEast)
            {
                cover |= East;
            }
            else if (cover & NorthWest)
            {
                cover |= West;
            }

            clearCoverFlags(cover, FullNorth);
        }
        else if (side == East)
        {
            if (cover & NorthEast)
            {
                cover |= North;
            }
            else if (cover & SouthEast)
            {
                cover |= South;
            }

            clearCoverFlags(cover, FullEast);
        }
        else if (side == South)
        {
            if (cover & SouthEast)
            {
                cover |= East;
            }
            else if (cover & SouthWest)
            {
                cover |= West;
            }

            clearCoverFlags(cover, FullSouth);
        }
        else if (side == West)
        {
            if (cover & SouthWest)
            {
                cover |= South;
            }
            else if (cover & NorthWest)
            {
                cover |= North;
            }

            clearCoverFlags(cover, FullWest);
        }
    }
}

TileCover TileCovers::mergeTileCover(TileCover a, TileCover b)
{
    if (!(a & Diagonals))
    {
        return a | b;
    }
    else
    {
        a |= North * (b & (NorthWest | NorthEast));
        a |= East * (b & (NorthEast | SouthEast));
        a |= South * (b & (SouthEast | SouthWest));
        a |= West * (b & (SouthWest | NorthWest));
        return a | (b & (~Diagonals));
    }
}

TileCover TileCovers::unifyTileCover(TileCover cover, TileQuadrant quadrant, TileCover preferredDiagonal)
{
    // DEBUG_ASSERT((preferredDiagonal == None) || exactlyOneSet(preferredDiagonal & Diagonals), "Preferred diagonal must be None or have exactly one diagonal set.");

    if (cover & Full)
        return Full;

    if (exactlyOneSet(cover))
    {
        return cover;
    }

    bool north = cover & FullNorth;
    bool east = cover & FullEast;
    bool south = cover & FullSouth;
    bool west = cover & FullWest;

    if (north && east && south && west)
    {
        return Full;
    }

    TileCover clear = None;

    if (north)
    {
        clear |= North * (cover & (NorthEast | NorthWest));
        clear |= NorthEastCorner | NorthWestCorner;
    }
    if (east)
    {
        clear |= East * (cover & (NorthEast | SouthEast));
        clear |= NorthEastCorner | SouthEastCorner;
    }
    if (south)
    {
        clear |= South * (cover & (SouthEast | SouthWest));
        clear |= SouthEastCorner | SouthWestCorner;
    }
    if (west)
    {
        clear |= West * (cover & (NorthWest | SouthWest));
        clear |= NorthWestCorner | SouthWestCorner;
    }
    clearCoverFlags(cover, clear);
    clear = None;

    TileCover diagonals = cover & Diagonals;
    if (diagonals)
    {
        // Exactly two diagonals
        if (!(TileCovers::exactlyOneSet(diagonals)))
        {
            // Double north
            if ((diagonals & NorthWest) && (diagonals & NorthEast))
            {
                bool left = (preferredDiagonal != NorthEast) && ((preferredDiagonal == NorthWest) || quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::BottomLeft);
                clear |= left ? NorthEast : NorthWest;
                cover |= left ? East : West;
            }
            // Double east
            else if ((diagonals & NorthEast) && (diagonals & SouthEast))
            {
                bool top = (preferredDiagonal != SouthEast) && ((preferredDiagonal == NorthEast) || quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::TopRight);
                clear |= top ? SouthEast : NorthEast;
                cover |= top ? South : North;
            }
            // Double south
            else if ((diagonals & SouthEast) && (diagonals & SouthWest))
            {
                bool left = (preferredDiagonal != SouthEast) && ((preferredDiagonal == SouthWest) || quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::BottomLeft);
                clear |= left ? SouthEast : SouthWest;
                cover |= left ? East : West;
            }
            // Double west
            else
            {
                bool top = (preferredDiagonal != SouthWest) && ((preferredDiagonal == NorthWest) || quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::TopRight);
                clear |= top ? SouthWest : NorthWest;
                cover |= top ? South : North;
            }

            clearCoverFlags(cover, clear);
            clear = None;
        }
    }
    else
    {
        int sides = static_cast<bool>(cover & North) + static_cast<bool>(cover & East) + static_cast<bool>(cover & South) + static_cast<bool>(cover & West);
        if (sides == 2)
        {
            if (cover & North)
            {
                if (cover & East)
                {
                    cover |= NorthEast;
                    clear |= North | East;
                }
                else if (cover & West)
                {
                    cover |= NorthWest;
                    clear |= North | West;
                }
            }
            else if (cover & South)
            {
                if (cover & East)
                {
                    cover |= SouthEast;
                    clear |= South | East;
                }
                else if (cover & West)
                {
                    cover |= SouthWest;
                    clear |= South | West;
                }
            }
        }
        else
        {
            if (preferredDiagonal != None)
            {
                if (preferredDiagonal == NorthWest && (cover & North) && (cover & West))
                {
                    cover |= NorthWest;
                    clear |= North | West;
                }
                else if (preferredDiagonal == NorthEast && (cover & North) && (cover & East))
                {
                    cover |= NorthEast;
                    clear |= North | East;
                }
                else if (preferredDiagonal == SouthEast && (cover & South) && (cover & East))
                {
                    cover |= SouthEast;
                    clear |= South | East;
                }
                else if (preferredDiagonal == SouthWest && (cover & South) && (cover & West))
                {
                    cover |= SouthWest;
                    clear |= South | West;
                }
            }
        }

        if (clear != None)
        {
            clearCoverFlags(cover, clear);
        }
    }

    auto finalDiagonals = cover & Diagonals;
    DEBUG_ASSERT((finalDiagonals == None) || exactlyOneSet(finalDiagonals), "Preferred diagonal must be None or have exactly one diagonal set.");

    return cover;
}

TileCover TileCovers::mirrorEast(TileCover cover)
{
    if (cover & FullEast)
    {
        return West;
    }

    TileCover result = None;
    if (cover & (North | NorthEastCorner | NorthWest))
    {
        result |= NorthWestCorner;
    }
    if (cover & (South | SouthEastCorner | SouthWest))
    {
        result |= SouthWestCorner;
    }

    return result;
}

TileCover TileCovers::mirrorEast(TileCover source, TileCover cover)
{
    if (cover & SouthEast)
    {
        return (source & South) ? SouthWest : West;
    }
    else if (cover & NorthEast)
    {
        return (source & North) ? NorthWest : West;
    }
    else
    {
        if (cover & FullEast)
        {
            return West;
        }

        TileCover result = None;
        if (cover & (North | NorthEastCorner | NorthWest))
        {
            result |= NorthWestCorner;
        }
        if (cover & (South | SouthEastCorner | SouthWest))
        {
            result |= SouthWestCorner;
        }

        return result;
    }
}

TileCover TileCovers::mirrorWest(TileCover source, TileCover cover)
{
    if (cover & SouthWest)
    {
        return (source & South) ? SouthEast : East;
    }
    else if (cover & NorthWest)
    {
        return (source & North) ? NorthEast : East;
    }
    else
    {
        if (cover & FullWest)
        {
            return East;
        }

        TileCover result = None;
        if (cover & (North | NorthWestCorner | NorthEast))
        {
            result |= NorthEastCorner;
        }
        if (cover & (South | SouthWestCorner | SouthEast))
        {
            result |= SouthEastCorner;
        }

        return result;
    }
}

TileCover TileCovers::mirrorWest(TileCover cover)
{
    if (cover & FullWest)
    {
        return East;
    }

    TileCover result = None;
    if (cover & (North | NorthWestCorner | NorthEast))
    {
        result |= NorthEastCorner;
    }
    if (cover & (South | SouthWestCorner | SouthEast))
    {
        result |= SouthEastCorner;
    }

    return result;
}

TileCover TileCovers::mirrorNorth(TileCover tileCover, bool corner)
{
    if (tileCover & FullNorth)
    {
        return South;
    }

    TileCover result = None;
    if (tileCover & (East | NorthEastCorner | SouthEast))
    {
        result |= corner ? SouthEastCorner : NorthEast;
    }
    if (tileCover & (West | NorthWestCorner | SouthWest))
    {
        result |= corner ? SouthWestCorner : NorthWest;
    }

    return result;
}

TileCover TileCovers::mirrorSouth(TileCover tileCover)
{
    if (tileCover & FullSouth)
    {
        return North;
    }

    TileCover result = None;
    if (tileCover & (East | SouthEastCorner | NorthEast))
    {
        result |= NorthEastCorner;
    }
    else if (tileCover & (West | SouthWestCorner | NorthWest))
    {
        result |= NorthWestCorner;
    }

    return result;
}

bool TileCovers::hasFullSide(TileCover cover, TileCover side)
{
    DEBUG_ASSERT(side & (North | East | South | West), "Invalid side");
    switch (side)
    {
        case North:
            return cover & FullNorth;
        case East:
            return cover & FullEast;
        case South:
            return cover & FullSouth;
        case West:
            return cover & FullWest;
        default:
            return false;
    }
}

TileCover TileCovers::mirrorXY(TileCover cover)
{
    DEBUG_ASSERT(exactlyOneSet(cover), "Must be exactly one set.");

    switch (cover)
    {
        case None:
            return None;
        case Full:
            return Full;
        case North:
            return South;
        case East:
            return West;
        case South:
            return North;
        case West:
            return East;
        case NorthWest:
            return SouthEastCorner;
        case NorthEast:
            return SouthWestCorner;
        case SouthWest:
            return NorthEastCorner;
        case SouthEast:
            return NorthWestCorner;
        case NorthWestCorner:
            return SouthEast;
        case NorthEastCorner:
            return SouthWest;
        case SouthWestCorner:
            return NorthEast;
        case SouthEastCorner:
            return NorthWest;
        default:
            ABORT_PROGRAM("Should never happen");
    }
}

TileCover TileCovers::getFull(TileCover side)
{
    DEBUG_ASSERT(side & (North | East | South | West), "Invalid side");
    switch (side)
    {
        case North:
            return FullNorth;
        case East:
            return FullEast;
        case South:
            return FullSouth;
        case West:
            return FullWest;
        default:
            ABORT_PROGRAM("Should never happen");
    }
}

TileCover TileCovers::addNonMasked(TileCover current, TileCover committed, TileCover mask)
{
    auto m = current & (~mask);
    return committed | m;
}

TileCover TileCovers::fromBorderType(BorderType borderType)
{
    return borderTypeToTileCover[to_underlying(borderType)];
}

std::string TileCovers::show(TileCover cover)
{
    std::ostringstream s;

    if (cover & None)
    {
        s << "None"
          << " | ";
    }
    if (cover & Full)
    {
        s << "Full"
          << " | ";
    }
    if (cover & North)
    {
        s << "North"
          << " | ";
    }
    if (cover & East)
    {
        s << "East"
          << " | ";
    }
    if (cover & South)
    {
        s << "South"
          << " | ";
    }
    if (cover & West)
    {
        s << "West"
          << " | ";
    }
    if (cover & NorthWest)
    {
        s << "NorthWest"
          << " | ";
    }
    if (cover & NorthEast)
    {
        s << "NorthEast"
          << " | ";
    }
    if (cover & SouthWest)
    {
        s << "SouthWest"
          << " | ";
    }
    if (cover & SouthEast)
    {
        s << "SouthEast"
          << " | ";
    }
    if (cover & NorthWestCorner)
    {
        s << "NorthWestCorner"
          << " | ";
    }
    if (cover & NorthEastCorner)
    {
        s << "NorthEastCorner"
          << " | ";
    }
    if (cover & SouthWestCorner)
    {
        s << "SouthWestCorner"
          << " | ";
    }
    if (cover & SouthEastCorner)
    {
        s << "SouthEastCorner"
          << " | ";
    }

    return s.str();
}
